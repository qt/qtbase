// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qbaselinetest.h"
#include "baselineprotocol.h"
#include <QtCore/QDir>
#include <QBuffer>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <memory>

#define MAXCMDLINEARGS 128

namespace QBaselineTest {

namespace {

// A test reaches the baseline server either over the baseline protocol, or
// over the server's optional REST API (see baselineserver/docs in the
// lancelot repository). Both carry the same conversation.
class BaselineTransport
{
public:
    virtual ~BaselineTransport() = default;

    virtual bool connect(const QString &testCase, bool *dryrun,
                         const PlatformInfo &clientInfo) = 0;
    virtual bool disconnect() = 0;
    virtual bool requestBaselineChecksums(const QString &testFunction,
                                          ImageItemList *itemList) = 0;
    virtual bool submitMatch(const ImageItem &item, QByteArray *serverMsg) = 0;
    virtual bool submitNewBaseline(const ImageItem &item, QByteArray *serverMsg) = 0;
    virtual bool submitMismatch(const ImageItem &item, QByteArray *serverMsg,
                                bool *fuzzyMatch) = 0;
    virtual bool finalizeTesting(QByteArray *serverMsg) = 0;
    virtual QString errorMessage() = 0;
};

class ProtocolTransport : public BaselineTransport
{
public:
    explicit ProtocolTransport(const QString &server) : server(server) {}

    bool connect(const QString &testCase, bool *dryrun, const PlatformInfo &clientInfo) override
    { return proto.connect(testCase, dryrun, clientInfo, server); }
    bool disconnect() override { return proto.disconnect(); }
    bool requestBaselineChecksums(const QString &testFunction, ImageItemList *itemList) override
    { return proto.requestBaselineChecksums(testFunction, itemList); }
    bool submitMatch(const ImageItem &item, QByteArray *serverMsg) override
    { return proto.submitMatch(item, serverMsg); }
    bool submitNewBaseline(const ImageItem &item, QByteArray *serverMsg) override
    { return proto.submitNewBaseline(item, serverMsg); }
    bool submitMismatch(const ImageItem &item, QByteArray *serverMsg, bool *fuzzyMatch) override
    { return proto.submitMismatch(item, serverMsg, fuzzyMatch); }
    bool finalizeTesting(QByteArray *serverMsg) override
    { return proto.finalizeTesting(serverMsg); }
    QString errorMessage() override { return proto.errorMessage(); }

private:
    BaselineProtocol proto;
    QString server;
};

class RestTransport : public BaselineTransport
{
public:
    RestTransport(const QString &baseUrl, const QString &token)
        : baseUrl(baseUrl), token(token.toUtf8())
    {
        while (this->baseUrl.endsWith(QLC('/')))
            this->baseUrl.chop(1);
    }

    ~RestTransport() override
    {
        // Closing the run is what makes the server write its report, as a
        // closed socket does for the baseline protocol.
        if (!runId.isEmpty() && QCoreApplication::instance())
            request("DELETE", QLS("/v1/runs/") + runId, {}, {}, nullptr, ShutdownTimeout);
    }

    bool connect(const QString &testCase, bool *dryrun, const PlatformInfo &clientInfo) override
    {
        QJsonObject properties;
        for (auto it = clientInfo.constBegin(); it != clientInfo.constEnd(); ++it)
            properties.insert(it.key(), it.value());
        properties.insert(PI_TestCase, testCase);

        QJsonObject body;
        body.insert(QLS("properties"), properties);
        body.insert(QLS("overrides"), QJsonArray::fromStringList(clientInfo.overrides()));
        body.insert(QLS("adhoc"), clientInfo.isAdHocRun());

        QJsonObject response;
        if (!request("POST", QLS("/v1/runs"), QJsonDocument(body).toJson(QJsonDocument::Compact),
                     "application/json", &response)) {
            return false;
        }
        runId = response.value(QLS("runId")).toString();
        if (runId.isEmpty()) {
            errMsg = QLS("The baseline server did not return a run id.");
            return false;
        }
        if (dryrun)
            *dryrun = response.value(QLS("dryRun")).toBool();
        return true;
    }

    bool disconnect() override
    {
        if (!runId.isEmpty())
            request("DELETE", QLS("/v1/runs/") + runId, {}, {}, nullptr);
        runId.clear();
        return true;
    }

    bool requestBaselineChecksums(const QString &testFunction, ImageItemList *itemList) override
    {
        // The item checksum is part of the item's identity on the server
        QJsonArray wanted;
        for (ImageItem &item : *itemList) {
            item.testFunction = testFunction;
            wanted.append(QJsonObject{ { QLS("itemName"), item.itemName },
                                       { QLS("itemChecksum"), int(item.itemChecksum) } });
        }
        const QJsonObject body{ { QLS("testFunction"), testFunction },
                                { QLS("items"), wanted } };

        QJsonObject response;
        if (!request("POST", runPath(QLS("/checksums")),
                     QJsonDocument(body).toJson(QJsonDocument::Compact),
                     "application/json", &response)) {
            return false;
        }

        const QJsonArray results = response.value(QLS("items")).toArray();
        if (results.size() != itemList->size()) {
            errMsg = QStringLiteral("The baseline server answered for %1 of %2 items.")
                         .arg(results.size()).arg(itemList->size());
            return false;
        }
        for (const QJsonValue &value : results) {
            const QJsonObject entry = value.toObject();
            const QString name = entry.value(QLS("itemName")).toString();
            const quint16 itemChecksum = quint16(entry.value(QLS("itemChecksum")).toInt());
            for (ImageItem &item : *itemList) {
                if (item.itemName != name || item.itemChecksum != itemChecksum)
                    continue;
                item.status = ImageItem::ItemStatus(entry.value(QLS("statusCode")).toInt());
                item.imageChecksums.clear();
                const QJsonArray checksums = entry.value(QLS("checksums")).toArray();
                for (const QJsonValue &checksum : checksums) {
                    bool ok = false;
                    const quint64 parsed = checksum.toString().toULongLong(&ok, 16);
                    if (ok)
                        item.imageChecksums.append(parsed);
                }
                break;
            }
        }
        return true;
    }

    bool submitMatch(const ImageItem &item, QByteArray *serverMsg) override
    { return submitItem("match", item, serverMsg, nullptr); }
    bool submitNewBaseline(const ImageItem &item, QByteArray *serverMsg) override
    { return submitItem("baseline", item, serverMsg, nullptr); }
    bool submitMismatch(const ImageItem &item, QByteArray *serverMsg, bool *fuzzyMatch) override
    { return submitItem("mismatch", item, serverMsg, fuzzyMatch); }

    bool finalizeTesting(QByteArray *serverMsg) override
    {
        QJsonObject response;
        if (runId.isEmpty() || !request("POST", runPath(QLS("/finalize")), {}, {}, &response))
            return false;
        runId.clear();
        *serverMsg = response.value(QLS("reportUrl")).toString().toUtf8();
        return true;
    }

    QString errorMessage() override { return errMsg; }

private:
    static constexpr int RequestTimeout = 120 * 1000;
    static constexpr int ShutdownTimeout = 5 * 1000;

    QString runPath(const QString &suffix) const
    { return QLS("/v1/runs/") + runId + suffix; }

    bool submitItem(const char *kind, const ImageItem &item, QByteArray *serverMsg,
                    bool *fuzzyMatch)
    {
        QUrlQuery query;
        query.addQueryItem(QLS("testFunction"), item.testFunction);
        query.addQueryItem(QLS("itemName"), item.itemName);
        query.addQueryItem(QLS("kind"), QLatin1StringView(kind));
        query.addQueryItem(QLS("status"), QString::number(int(item.status)));
        if (item.itemChecksum)
            query.addQueryItem(QLS("itemChecksum"), QString::number(item.itemChecksum));
        // The server must store the checksum computed over our in-memory image,
        // since that is what later runs compare against.
        if (!item.imageChecksums.isEmpty())
            query.addQueryItem(QLS("checksum"), QString::number(item.imageChecksums.at(0), 16));

        QByteArray body;
        QByteArray contentType;
        if (qstrcmp(kind, "match") != 0) {
            QImage image = item.image;
            for (auto it = item.metaData.constBegin(); it != item.metaData.constEnd(); ++it)
                image.setText(it.key(), it.value().toString());
            QBuffer buffer(&body);
            buffer.open(QIODevice::WriteOnly);
            if (image.isNull() || !image.save(&buffer, "png")) {
                errMsg = QLS("Could not encode ") + item.itemName + QLS(" as PNG.");
                return false;
            }
            contentType = "image/png";
        }

        QJsonObject response;
        if (!request("POST", runPath(QLS("/items?") + query.toString(QUrl::FullyEncoded)),
                     body, contentType, &response)) {
            return false;
        }
        if (fuzzyMatch)
            *fuzzyMatch = response.value(QLS("status")).toString() == QLS("fuzzyMatch");
        *serverMsg = response.value(QLS("reportUrl")).toString().toUtf8();
        return true;
    }

    bool request(const QByteArray &verb, const QString &path, const QByteArray &body,
                 const QByteArray &contentType, QJsonObject *response,
                 int timeout = RequestTimeout)
    {
        errMsg.clear();

        QNetworkRequest req{ QUrl(baseUrl + path) };
        req.setTransferTimeout(timeout);
        if (!token.isEmpty())
            req.setRawHeader("Authorization", "Bearer " + token);
        req.setHeader(QNetworkRequest::ContentTypeHeader,
                      contentType.isEmpty() ? QByteArray("application/octet-stream")
                                            : contentType);

        std::unique_ptr<QNetworkReply> reply(manager.sendCustomRequest(req, verb, body));
        QEventLoop loop;
        QObject::connect(reply.get(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray payload = reply->readAll();
        const QJsonObject object = QJsonDocument::fromJson(payload).object();
        if (response)
            *response = object;
        if (status >= 200 && status < 300)
            return true;

        if (status == 0) {
            errMsg = QLS("Could not reach ") + baseUrl + QLS(": ") + reply->errorString();
            return false;
        }
        errMsg = QLS("The baseline server returned ") + QString::number(status);
        const QString detail = object.value(QLS("error")).toString();
        if (!detail.isEmpty())
            errMsg += QLS(": ") + detail;
        if (status == 401)
            errMsg += QLS(" (set QT_LANCELOT_TOKEN, or pass -token)");
        return false;
    }

    QNetworkAccessManager manager;
    QString baseUrl;
    QByteArray token;
    QString runId;
    QString errMsg;
};

} // namespace

static char *fargv[MAXCMDLINEARGS];
static QString server;
static QString token;
static bool simfail = false;
static PlatformInfo customInfo;
static bool customAutoModeSet = false;

static std::unique_ptr<BaselineTransport> transport;
static bool connected = false;
static bool triedConnecting = false;
static bool dryRunMode = false;
static enum { UploadMissing, UploadAll, UploadNone } baselinePolicy = UploadMissing;
static bool abortIfUnstable = true;

static QByteArray curFunction;
static ImageItemList itemList;
static bool gotBaselines;

static int pauseOnCompare = 0;

void handleCmdLineArgs(int *argcp, char ***argvp)
{
    if (!argcp || !argvp)
        return;

    bool showHelp = false;
    bool abortOnHelp = true;

    int fargc = 0;
    int numArgs = *argcp;

    for (int i = 0; i < numArgs; i++) {
        QByteArray arg = (*argvp)[i];
        QByteArray nextArg = (i+1 < numArgs) ? (*argvp)[i+1] : nullptr;

        if (arg == "-server") {
            i++;
            if (!nextArg.isEmpty()) {
                server = QString::fromLocal8Bit(nextArg);
            } else {
                qWarning() << "-server requires parameter";
                showHelp = true;
                break;
            }
        } else if (arg == "-token") {
            i++;
            if (nextArg.isEmpty()) {
                qWarning() << "-token requires parameter";
                showHelp = true;
                break;
            }
            token = QString::fromLocal8Bit(nextArg);
        } else if (arg == "-token-file") {
            i++;
            QFile tokenFile(QString::fromLocal8Bit(nextArg));
            if (nextArg.isEmpty() || !tokenFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning() << "-token-file requires a readable file";
                showHelp = true;
                break;
            }
            token = QString::fromLatin1(tokenFile.readLine()).trimmed();
        } else if (arg == "-simfail") {
            simfail = true;
        } else if (arg == "-fuzzlevel") {
            i++;
            bool ok = false;
            (void)nextArg.toInt(&ok);
            if (!ok) {
                qWarning() << "-fuzzlevel requires integer parameter";
                showHelp = true;
                break;
            }
            customInfo.insert("FuzzLevel", QString::fromLatin1(nextArg));
        } else if (arg == "-auto") {
            customAutoModeSet = true;
            customInfo.setAdHocRun(false);
        } else if (arg == "-adhoc") {
            customAutoModeSet = true;
            customInfo.setAdHocRun(true);
        } else if (arg == "-setbaselines") {
            baselinePolicy = UploadAll;
        } else if (arg == "-keeprunning") {
            abortIfUnstable = false;
        } else if (arg == "-nosetbaselines") {
            baselinePolicy = UploadNone;
        } else if (arg == "-compareto") {
            i++;
            int split = qMax(0, nextArg.indexOf('='));
            QByteArray key = nextArg.left(split).trimmed();
            QByteArray value = nextArg.mid(split+1).trimmed();
            if (key.isEmpty() || value.isEmpty()) {
                qWarning() << "-compareto requires parameter of the form <key>=<value>";
                showHelp = true;
                break;
            }
            customInfo.addOverride(key, value);
        } else if (arg == "-pause-compare") {
            i++;
            bool ok = false;
            pauseOnCompare = nextArg.toInt(&ok);
            if (!ok) {
                qWarning() << "-pause-compare requires integer parameter";
                showHelp = true;
                break;
            }
        } else {
            if ( (arg == "-help") || (arg == "--help") ) {
                showHelp = true;
                abortOnHelp = false;
            }
            if (fargc >= MAXCMDLINEARGS) {
                qWarning() << "Too many command line arguments!";
                showHelp = true;
                break;
            }
            fargv[fargc++] = (*argvp)[i];
        }
    }
    *argcp = fargc;
    *argvp = fargv;

    if (showHelp) {
        // TBD: arrange for this to be printed *after* QTest's help
        QTextStream out(stdout);
        out << "\n Baseline testing (lancelot) options:\n";
        out << " -server <host|url>  : Set the network baseline server to connect to.\n";
        out << "                       The default is taken from the environment variable QT_LANCELOT_SERVER.\n";
        out << "                       A bare host name uses the baseline protocol on port 54129.\n";
        out << "                       A URL, e.g. https://lancelot.example.com, uses the server's\n";
        out << "                       REST API instead.\n";
        out << " -token <token>      : Bearer token for the REST API, if the server requires one.\n";
        out << "                       The default is taken from the environment variable QT_LANCELOT_TOKEN.\n";
        out << " -token-file <path>  : Read the token from the first line of a file instead, so it\n";
        out << "                       does not appear in the process list.\n";
        out << " -simfail            : Force an image comparison mismatch. For development purposes.\n";
        out << " -fuzzlevel <int>    : Specify the percentage of fuzziness in comparison. Overrides server default. 0 means exact match.\n";
        out << " -auto               : Inform server that this run is done by a daemon, CI system or similar.\n";
        out << " -adhoc (default)    : The inverse of -auto; this run is done by human, e.g. for testing.\n";
        out << " -keeprunning        : Run all tests even if the system is unstable \n";
        out << " -setbaselines       : Store ALL rendered images as new baselines. Forces replacement of previous baselines.\n";
        out << " -nosetbaselines     : Do not store rendered images as new baselines when previous baselines are missing.\n";
        out << " -compareto KEY=VAL  : Force comparison to baselines from a different client,\n";
        out << "                       for example: -compareto QtVersion=4.8.0\n";
        out << "                       Multiple -compareto client specifications may be given.\n";
        out << " -pause-compare <ms> : Pauses for the given number of ms before each compare\n";
        out << "\n";
        out.flush();
        if (abortOnHelp)
            std::exit(1);
    }
}

bool shouldAbortIfUnstable()
{
    return abortIfUnstable;
}

void addClientProperty(const QString& key, const QString& value)
{
    customInfo.insert(key, value);
}


/*
  If a client property script is present, run it and accept its output
  in the form of one 'key: value' property per line
*/
void fetchCustomClientProperties()
{
    QFile file("hostinfo.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();   // ###local8bit? utf8?
        if (line.startsWith(QLatin1Char('#')))  // Ignore comments in file
            continue;
        QString key, val;
        int colonPos = line.indexOf(':');
        if (colonPos > 0) {
            key = line.left(colonPos).simplified().replace(' ', '_');
            val = line.mid(colonPos+1).trimmed();
        }
        if (!key.isEmpty() && key.size() < 64 && val.size() < 256)  // ###TBD: maximum 256 chars in value?
            addClientProperty(key, val);
        else
            qDebug() << "Unparseable script output ignored:" << line;
    }
}


bool connect(QByteArray *msg, bool *error)
{
    if (connected) {
        return true;
    }
    else if (triedConnecting) {
        // Avoid repeated connection attempts, to avoid the program using Timeout * #testItems seconds before giving up
        *msg = "Not connected to baseline server.";
        *error = true;
        return false;
    }

    triedConnecting = true;
    fetchCustomClientProperties();
    // Merge the platform info set by the program with the protocols default info
    PlatformInfo clientInfo = customInfo;
    PlatformInfo defaultInfo = PlatformInfo::localHostInfo();
    const auto &defaultInfoKeys = defaultInfo.keys();
    for (const QString &key : defaultInfoKeys) {
        if (!clientInfo.contains(key))
            clientInfo.insert(key, defaultInfo.value(key));
    }
    if (!customAutoModeSet)
        clientInfo.setAdHocRun(defaultInfo.isAdHocRun());

    QString testCase = clientInfo.value(PI_TestCase);
    if (testCase.isEmpty() && QTest::testObject() && QTest::testObject()->metaObject()) {
        //qDebug() << "Trying to Read TestCaseName from Testlib!";
        testCase = QTest::testObject()->metaObject()->className();
    }
    if (testCase.isEmpty()) {
        qWarning("QBaselineTest::connect: No test case name specified, cannot connect.");
        return false;
    }

    QString serverAddress = server;
    if (serverAddress.isEmpty())
        serverAddress = qEnvironmentVariable("QT_LANCELOT_SERVER");
    if (serverAddress.isEmpty())
        serverAddress = QStringLiteral("lancelot.test.qt-project.org");

    QString serverToken = token;
    if (serverToken.isEmpty())
        serverToken = qEnvironmentVariable("QT_LANCELOT_TOKEN");

    // A URL selects the REST API; a bare host name the baseline protocol
    if (serverAddress.contains(QLS("://")))
        transport = std::make_unique<RestTransport>(serverAddress, serverToken);
    else
        transport = std::make_unique<ProtocolTransport>(serverAddress);
    static bool cleanupRegistered = false;
    if (!cleanupRegistered) {
        qAddPostRoutine([] { transport.reset(); });
        cleanupRegistered = true;
    }
    if (!transport->connect(testCase, &dryRunMode, clientInfo)) {
        *msg += "Failed to connect to baseline server: " + transport->errorMessage().toLatin1();
        transport.reset();
        *error = true;
        return false;
    }
    connected = true;
    return true;
}

bool disconnectFromBaselineServer()
{
    if (!transport)
        return false;
    if (transport->disconnect()) {
        transport.reset();
        connected = false;
        triedConnecting = false;
        return true;
    }

    return false;
}

bool connectToBaselineServer(QByteArray *msg)
{
    bool dummy;
    QByteArray dummyMsg;
    return connect(msg ? msg : &dummyMsg, &dummy);
}

bool finalizeTesting(QByteArray *msg) {
    QByteArray dummyMsg;
    return transport && transport->finalizeTesting(msg ? msg : &dummyMsg);
}

void finalizeAndDisconnect()
{
    if (QByteArray msg; finalizeTesting(&msg)) {
        if (msg.isEmpty())
            qInfo() << "No baseline server report produced.";
        else
            qInfo() << "Baseline server report:" << msg.data();
    }

    disconnectFromBaselineServer();
}

void setAutoMode(bool mode)
{
    customInfo.setAdHocRun(!mode);
    customAutoModeSet = true;
}

void setSimFail(bool fail)
{
    simfail = fail;
}

void setProject(const QString &projectName)
{
    addClientProperty(PI_Project, projectName);
}

void setProjectImageKeys(const QStringList &keys)
{
    QString keyList = keys.join(QLC(','));
    addClientProperty(PI_ProjectImageKeys, keyList);
}

void modifyImage(QImage *img)
{
    uint c0 = 0x0000ff00;
    uint c1 = 0x0080ff00;
    img->setPixel(1,1,c0);
    img->setPixel(2,1,c1);
    img->setPixel(3,1,c0);
    img->setPixel(1,2,c1);
    img->setPixel(1,3,c0);
    img->setPixel(2,3,c1);
    img->setPixel(3,3,c0);
    img->setPixel(1,4,c1);
    img->setPixel(1,5,c0);
}


bool compareItem(const ImageItem &baseline, const QImage &img, QByteArray *msg, bool *error)
{
    if (pauseOnCompare) {
        qDebug() << "Pausing for" << pauseOnCompare << "ms...";
        QTest::qWait(pauseOnCompare);
    }

    *error = false;
    ImageItem item = baseline;
    if (simfail) {
        // Simulate test failure by forcing image mismatch; for testing purposes
        QImage misImg = img;
        modifyImage(&misImg);
        item.image = misImg;
        simfail = false;                      // One failure is typically enough
    } else {
        item.image = img;
    }
    bool isNewItem = false;
    item.imageChecksums.clear();
    item.imageChecksums.prepend(ImageItem::computeChecksum(item.image));
    QByteArray srvMsg;
    switch (baseline.status) {
    case ImageItem::Ok:
        break;
    case ImageItem::IgnoreItem :
        qDebug() << msg->constData() << "Ignored, blacklisted on server.";
        return true;
        break;
    case ImageItem::BaselineNotFound:
        if (!customInfo.overrides().isEmpty())
            return true;
        if (baselinePolicy == UploadNone) {
            isNewItem = true;
            break;
        }
        if (transport->submitNewBaseline(item, &srvMsg))
            qDebug() << msg->constData() << "Baseline not found on server. New baseline uploaded.";
        else
            qDebug() << msg->constData() << "Baseline not found on server. Uploading of new baseline failed:" << srvMsg;
        return true;
        break;
    default:
        qWarning() << "Unexpected reply from baseline server.";
        return true;
        break;
    }
    // The actual comparison of the given image with the baseline:
    if (baseline.imageChecksums.contains(item.imageChecksums.at(0))) {
        if (!transport->submitMatch(item, &srvMsg))
            qWarning() << "Failed to report image match to server:" << srvMsg;
        return true;
    }
    // At this point, we have established a legitimate mismatch
    if (baselinePolicy == UploadAll) {
        if (transport->submitNewBaseline(item, &srvMsg))
            qDebug() << msg->constData() << "Forcing new baseline; uploaded ok.";
        else
            qDebug() << msg->constData() << "Forcing new baseline; uploading failed:" << srvMsg;
        return true;
    }
    bool fuzzyMatch = false;
    bool res = transport->submitMismatch(item, &srvMsg, &fuzzyMatch);
    if (res && fuzzyMatch) {
        qInfo() << "Baseline server reports:" << srvMsg;
        return true;            // The server decides: a fuzzy match means no mismatch
    }
    if (isNewItem)
        *msg += "No baseline on server, so cannot compare.";
    else
        *msg += "Mismatch.";
    *msg += " See report:\n   " + srvMsg;
    if (dryRunMode) {
        qDebug() << "Dryrun, so ignoring" << *msg;
        return true;
    }
    return false;
}

bool checkImage(const QImage &img, const char *name, quint16 checksum, QByteArray *msg, bool *error, int manualdatatag)
{
    if (!connected && !connect(msg, error))
        return true;

    QByteArray itemName;
    bool hasName = qstrlen(name);

    const char *tag = QTest::currentDataTag();
    if (qstrlen(tag)) {
        itemName = tag;
        if (hasName)
            itemName.append('_').append(name);
    } else {
            itemName = hasName ? name : "default_name";
    }

    if (manualdatatag > 0)
    {
        itemName.prepend("_");
        itemName.prepend(QByteArray::number(manualdatatag));
    }

    *msg = "Baseline check of image '" + itemName + "': ";


    ImageItem item;
    item.itemName = QString::fromLatin1(itemName);
    item.itemChecksum = checksum;
    item.testFunction = QString::fromLatin1(QTest::currentTestFunction());

    for (auto key: img.textKeys())
        item.metaData[key] = img.text(key);

    ImageItemList list;
    list.append(item);
    if (!transport->requestBaselineChecksums(QLatin1String(QTest::currentTestFunction()), &list)
        || list.isEmpty()) {
        *msg = "Communication with baseline server failed: " + transport->errorMessage().toLatin1();
        *error = true;
        return true;
    }

    return compareItem(list.at(0), img, msg, error);
}


QTestData &newRow(const char *dataTag, quint16 checksum)
{
    if (QTest::currentTestFunction() != curFunction) {
        curFunction = QTest::currentTestFunction();
        itemList.clear();
        gotBaselines = false;
    }
    ImageItem item;
    item.itemName = QString::fromLatin1(dataTag);
    item.itemChecksum = checksum;
    item.testFunction = QString::fromLatin1(QTest::currentTestFunction());
    itemList.append(item);

    return QTest::newRow(dataTag);
}

const ImageItem *findCurrentItem(QByteArray *msg, bool *error)
{
    if (!connected && !connect(msg, error))
        return nullptr;

    if (QTest::currentTestFunction() != curFunction || itemList.isEmpty()) {
        qWarning() << "Usage error: QBASELINE_ macro used without corresponding QBaselineTest::newRow()";
        return nullptr;
    }

    if (!gotBaselines) {
        if (!transport->requestBaselineChecksums(QString::fromLatin1(QTest::currentTestFunction()),
                                                 &itemList) || itemList.isEmpty()) {
            *msg = "Communication with baseline server failed: "
                    + transport->errorMessage().toLatin1();
            *error = true;
            return nullptr;
        }
        gotBaselines = true;
    }

    QString curTag = QString::fromLatin1(QTest::currentDataTag());
    ImageItemList::const_iterator it = itemList.constBegin();
    while (it != itemList.constEnd() && it->itemName != curTag)
        ++it;
    if (it == itemList.constEnd()) {
        qWarning() << "Usage error: QBASELINE_ macro used without corresponding QBaselineTest::newRow() for row" << curTag;
        return nullptr;
    }
    return &(*it);
}

bool testImage(const QImage &img, QByteArray *msg, bool *error)
{
    const ImageItem *item = findCurrentItem(msg, error);
    return item ? compareItem(*item, img, msg, error) : true;
}

bool isCurrentItemBlacklisted()
{
    QByteArray msg;
    bool error = false;
    const ImageItem *item = findCurrentItem(&msg, &error);
    return item ? (item->status == ImageItem::IgnoreItem) : false;
}

}
