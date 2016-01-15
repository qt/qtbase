/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define QT_USE_FAST_CONCATENATION
#define QT_USE_FAST_OPERATOR_PLUS

#include "baselineserver.h"
#include <QBuffer>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHostInfo>
#include <QTextStream>
#include <QProcess>
#include <QDirIterator>
#include <QUrl>

// extra fields, for use in image metadata storage
const QString PI_ImageChecksum(QLS("ImageChecksum"));
const QString PI_RunId(QLS("RunId"));
const QString PI_CreationDate(QLS("CreationDate"));

QString BaselineServer::storage;
QString BaselineServer::url;
QStringList BaselineServer::pathKeys;

BaselineServer::BaselineServer(QObject *parent)
    : QTcpServer(parent), lastRunIdIdx(0)
{
    QFileInfo me(QCoreApplication::applicationFilePath());
    meLastMod = me.lastModified();
    heartbeatTimer = new QTimer(this);
    connect(heartbeatTimer, SIGNAL(timeout()), this, SLOT(heartbeat()));
    heartbeatTimer->start(HEARTBEAT*1000);
}

QString BaselineServer::storagePath()
{
    if (storage.isEmpty()) {
        storage = QLS(qgetenv("QT_LANCELOT_DIR"));
        if (storage.isEmpty())
            storage = QLS("/var/www");
    }
    return storage;
}

QString BaselineServer::baseUrl()
{
    if (url.isEmpty()) {
        url = QLS("http://")
                + QHostInfo::localHostName().toLatin1() + '.'
                + QHostInfo::localDomainName().toLatin1() + '/';
    }
    return url;
}

QStringList BaselineServer::defaultPathKeys()
{
    if (pathKeys.isEmpty())
        pathKeys << PI_QtVersion << PI_QMakeSpec << PI_HostName;
    return pathKeys;
}

void BaselineServer::incomingConnection(qintptr socketDescriptor)
{
    QString runId = QDateTime::currentDateTime().toString(QLS("MMMdd-hhmmss"));
    if (runId == lastRunId) {
        runId += QLC('-') + QString::number(++lastRunIdIdx);
    } else {
        lastRunId = runId;
        lastRunIdIdx = 0;
    }
    qDebug() << "Server: New connection! RunId:" << runId;
    BaselineThread *thread = new BaselineThread(runId, socketDescriptor, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

void BaselineServer::heartbeat()
{
    // The idea is to exit to be restarted when modified, as soon as not actually serving
    QFileInfo me(QCoreApplication::applicationFilePath());
    if (me.lastModified() == meLastMod)
        return;
    if (!me.exists() || !me.isExecutable())
        return;

    //# (could close() here to avoid accepting new connections, to avoid livelock)
    //# also, could check for a timeout to force exit, to avoid hung threads blocking
    bool isServing = false;
    foreach(BaselineThread *thread, findChildren<BaselineThread *>()) {
        if (thread->isRunning()) {
            isServing = true;
            break;
        }
    }

    if (!isServing)
        QCoreApplication::exit();
}

BaselineThread::BaselineThread(const QString &runId, int socketDescriptor, QObject *parent)
    : QThread(parent), runId(runId), socketDescriptor(socketDescriptor)
{
}

void BaselineThread::run()
{
    BaselineHandler handler(runId, socketDescriptor);
    exec();
}


BaselineHandler::BaselineHandler(const QString &runId, int socketDescriptor)
    : QObject(), runId(runId), connectionEstablished(false), settings(0), fuzzLevel(0)
{
    idleTimer = new QTimer(this);
    idleTimer->setSingleShot(true);
    idleTimer->setInterval(IDLE_CLIENT_TIMEOUT * 1000);
    connect(idleTimer, SIGNAL(timeout()), this, SLOT(idleClientTimeout()));
    idleTimer->start();

    if (socketDescriptor == -1)
        return;

    connect(&proto.socket, SIGNAL(readyRead()), this, SLOT(receiveRequest()));
    connect(&proto.socket, SIGNAL(disconnected()), this, SLOT(receiveDisconnect()));
    proto.socket.setSocketDescriptor(socketDescriptor);
    proto.socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
}

const char *BaselineHandler::logtime()
{
    return 0;
    //return QTime::currentTime().toString(QLS("mm:ss.zzz"));
}

QString BaselineHandler::projectPath(bool absolute) const
{
    QString p = clientInfo.value(PI_Project);
    return absolute ? BaselineServer::storagePath() + QLC('/') + p : p;
}

bool BaselineHandler::checkClient(QByteArray *errMsg, bool *dryRunMode)
{
    if (!errMsg)
        return false;
    if (clientInfo.value(PI_Project).isEmpty() || clientInfo.value(PI_TestCase).isEmpty()) {
        *errMsg = "No Project and/or TestCase specified in client info.";
        return false;
    }

    // Determine ad-hoc state ### hardcoded for now
    if (clientInfo.value(PI_TestCase) == QLS("tst_Lancelot")) {
        //### Todo: push this stuff out in a script
        if (!clientInfo.isAdHocRun()) {
            // ### comp. with earlier versions still running (4.8) (?)
            clientInfo.setAdHocRun(clientInfo.value(PI_PulseGitBranch).isEmpty() && clientInfo.value(PI_PulseTestrBranch).isEmpty());
        }
    }
    else {
        // TBD
    }

    if (clientInfo.isAdHocRun()) {
        if (dryRunMode)
            *dryRunMode = false;
        return true;
    }

    // Not ad hoc: filter the client
    settings->beginGroup("ClientFilters");
    bool matched = false;
    bool dryRunReq = false;
    foreach (const QString &rule, settings->childKeys()) {
        //qDebug() << "  > RULE" << rule;
        dryRunReq = false;
        QString ruleMode = settings->value(rule).toString().toLower();
        if (ruleMode == QLS("dryrun"))
            dryRunReq = true;
        else if (ruleMode != QLS("enabled"))
            continue;
        settings->beginGroup(rule);
        bool ruleMatched = true;
        foreach (const QString &filterKey, settings->childKeys()) {
            //qDebug() << "    > FILTER" << filterKey;
            QString filter = settings->value(filterKey).toString();
            if (filter.isEmpty())
                continue;
            QString platVal = clientInfo.value(filterKey);
            if (!platVal.contains(filter)) {
                ruleMatched = false;
                break;
            }
        }
        if (ruleMatched) {
            ruleName = rule;
            matched = true;
            break;
        }
        settings->endGroup();
    }

    if (!matched && errMsg)
        *errMsg = "Non-adhoc client did not match any filter rule in " + settings->fileName().toLatin1();

    if (matched && dryRunMode)
        *dryRunMode = dryRunReq;

    // NB! Must reset the settings object before returning
    while (!settings->group().isEmpty())
        settings->endGroup();

    return matched;
}

bool BaselineHandler::establishConnection()
{
    if (!proto.acceptConnection(&clientInfo)) {
        qWarning() << runId << logtime() << "Accepting new connection from" << proto.socket.peerAddress().toString() << "failed." << proto.errorMessage();
        proto.sendBlock(BaselineProtocol::Abort, proto.errorMessage().toLatin1());  // In case the client can hear us, tell it what's wrong.
        proto.socket.disconnectFromHost();
        return false;
    }
    QString logMsg;
    foreach (QString key, clientInfo.keys()) {
        if (key != PI_HostName && key != PI_HostAddress)
            logMsg += key + QLS(": '") + clientInfo.value(key) + QLS("', ");
    }
    qDebug() << runId << logtime() << "Connection established with" << clientInfo.value(PI_HostName)
             << '[' << qPrintable(clientInfo.value(PI_HostAddress)) << ']' << logMsg
             << "Overrides:" << clientInfo.overrides() << "AdHoc-Run:" << clientInfo.isAdHocRun();

    // ### Hardcoded backwards compatibility: add project field for certain existing clients that lack it
    if (clientInfo.value(PI_Project).isEmpty()) {
        QString tc = clientInfo.value(PI_TestCase);
        if (tc == QLS("tst_Lancelot"))
            clientInfo.insert(PI_Project, QLS("Raster"));
        else if (tc == QLS("tst_Scenegraph"))
            clientInfo.insert(PI_Project, QLS("SceneGraph"));
        else
            clientInfo.insert(PI_Project, QLS("Other"));
    }

    QString settingsFile = projectPath() + QLS("/config.ini");
    settings = new QSettings(settingsFile, QSettings::IniFormat, this);

    QByteArray errMsg;
    bool dryRunMode = false;
    if (!checkClient(&errMsg, &dryRunMode)) {
        qDebug() << runId << logtime() << "Rejecting connection:" << errMsg;
        proto.sendBlock(BaselineProtocol::Abort, errMsg);
        proto.socket.disconnectFromHost();
        return false;
    }

    fuzzLevel = qBound(0, settings->value("FuzzLevel").toInt(), 100);
    if (!clientInfo.isAdHocRun()) {
        qDebug() << runId << logtime() << "Client matches filter rule" << ruleName
                 << "Dryrun:" << dryRunMode
                 << "FuzzLevel:" << fuzzLevel
                 << "ReportMissingResults:" << settings->value("ReportMissingResults").toBool();
    }

    proto.sendBlock(dryRunMode ? BaselineProtocol::DoDryRun : BaselineProtocol::Ack, QByteArray());
    report.init(this, runId, clientInfo, settings);
    return true;
}

void BaselineHandler::receiveRequest()
{
    idleTimer->start();   // Restart idle client timeout

    if (!connectionEstablished) {
        connectionEstablished = establishConnection();
        return;
    }

    QByteArray block;
    BaselineProtocol::Command cmd;
    if (!proto.receiveBlock(&cmd, &block)) {
        qWarning() << runId << logtime() << "Command reception failed. "<< proto.errorMessage();
        QThread::currentThread()->exit(1);
        return;
    }

    switch(cmd) {
    case BaselineProtocol::RequestBaselineChecksums:
        provideBaselineChecksums(block);
        break;
    case BaselineProtocol::AcceptMatch:
        recordMatch(block);
        break;
    case BaselineProtocol::AcceptNewBaseline:
        storeImage(block, true);
        break;
    case BaselineProtocol::AcceptMismatch:
        storeImage(block, false);
        break;
    default:
        qWarning() << runId << logtime() << "Unknown command received. " << proto.errorMessage();
        proto.sendBlock(BaselineProtocol::UnknownError, QByteArray());
    }
}


void BaselineHandler::provideBaselineChecksums(const QByteArray &itemListBlock)
{
    ImageItemList itemList;
    QDataStream ds(itemListBlock);
    ds >> itemList;
    qDebug() << runId << logtime() << "Received request for checksums for" << itemList.count()
             << "items in test function" << itemList.at(0).testFunction;

    for (ImageItemList::iterator i = itemList.begin(); i != itemList.end(); ++i) {
        i->imageChecksums.clear();
        i->status = ImageItem::BaselineNotFound;
        QString prefix = pathForItem(*i, true);
        PlatformInfo itemData = fetchItemMetadata(prefix);
        if (itemData.contains(PI_ImageChecksum)) {
            bool ok = false;
            quint64 checksum = itemData.value(PI_ImageChecksum).toULongLong(&ok, 16);
            if (ok) {
                i->imageChecksums.prepend(checksum);
                i->status = ImageItem::Ok;
            }
        }
    }

    // Find and mark blacklisted items
    QString context = pathForItem(itemList.at(0), true, false).section(QLC('/'), 0, -2);
    if (itemList.count() > 0) {
        QFile file(BaselineServer::storagePath() + QLC('/') + context + QLS("/BLACKLIST"));
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            do {
                QString itemName = in.readLine();
                if (!itemName.isNull()) {
                    for (ImageItemList::iterator i = itemList.begin(); i != itemList.end(); ++i) {
                        if (i->itemName == itemName)
                            i->status = ImageItem::IgnoreItem;
                    }
                }
            } while (!in.atEnd());
        }
    }

    QByteArray block;
    QDataStream ods(&block, QIODevice::WriteOnly);
    ods << itemList;
    proto.sendBlock(BaselineProtocol::Ack, block);
    report.addItems(itemList);
}


void BaselineHandler::recordMatch(const QByteArray &itemBlock)
{
    QDataStream ds(itemBlock);
    ImageItem item;
    ds >> item;
    report.addResult(item);
    proto.sendBlock(BaselineProtocol::Ack, QByteArray());
}


void BaselineHandler::storeImage(const QByteArray &itemBlock, bool isBaseline)
{
    QDataStream ds(itemBlock);
    ImageItem item;
    ds >> item;

    if (isBaseline && !clientInfo.overrides().isEmpty()) {
        qDebug() << runId << logtime() << "Received baseline from client with override info, ignoring. Item:" << item.itemName;
        proto.sendBlock(BaselineProtocol::UnknownError, "New baselines not accepted from client with override info.");
        return;
    }

    QString blPrefix = pathForItem(item, true);
    QString mmPrefix = pathForItem(item, false);
    QString prefix = isBaseline ? blPrefix : mmPrefix;

    qDebug() << runId << logtime() << "Received" << (isBaseline ? "baseline" : "mismatched") << "image for:" << item.itemName << "Storing in" << prefix;

    // Reply to the client
    QString msg;
    if (isBaseline)
        msg = QLS("New baseline image stored: ") + blPrefix + QLS(FileFormat);
    else
        msg = BaselineServer::baseUrl() + report.filePath();

    if (isBaseline || !fuzzLevel)
        proto.sendBlock(BaselineProtocol::Ack, msg.toLatin1());  // Do early reply if possible: don't make the client wait longer than necessary

    // Store the image
    QString dir = prefix.section(QLC('/'), 0, -2);
    QDir cwd;
    if (!cwd.exists(dir))
        cwd.mkpath(dir);
    item.image.save(prefix + QLS(FileFormat), FileFormat);

    PlatformInfo itemData = clientInfo;
    itemData.insert(PI_ImageChecksum, QString::number(item.imageChecksums.at(0), 16));  //# Only the first is stored. TBD: get rid of list
    itemData.insert(PI_RunId, runId);
    itemData.insert(PI_CreationDate, QDateTime::currentDateTime().toString());
    storeItemMetadata(itemData, prefix);

    if (!isBaseline) {
        // Do fuzzy matching
        bool fuzzyMatch = false;
        if (fuzzLevel) {
            BaselineProtocol::Command cmd = BaselineProtocol::Ack;
            fuzzyMatch = fuzzyCompare(blPrefix, mmPrefix);
            if (fuzzyMatch) {
                msg.prepend(QString("Fuzzy match at fuzzlevel %1%. Report: ").arg(fuzzLevel));
                cmd = BaselineProtocol::FuzzyMatch;
            }
            proto.sendBlock(cmd, msg.toLatin1());  // We didn't reply earlier
        }

        // Add to report
        item.status = fuzzyMatch ? ImageItem::FuzzyMatch : ImageItem::Mismatch;
        report.addResult(item);
    }
}


void BaselineHandler::storeItemMetadata(const PlatformInfo &metadata, const QString &path)
{
    QFile file(path + QLS(MetadataFileExt));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << runId << logtime() << "ERROR: could not write to file" << file.fileName();
        return;
    }
    QTextStream out(&file);
    PlatformInfo::const_iterator it = metadata.constBegin();
    while (it != metadata.constEnd()) {
        out << it.key()  << ": " << it.value() << endl;
        ++it;
    }
    file.close();
}


PlatformInfo BaselineHandler::fetchItemMetadata(const QString &path)
{
    PlatformInfo res;
    QFile file(path + QLS(MetadataFileExt));
    if (!file.open(QIODevice::ReadOnly) || !QFile::exists(path + QLS(FileFormat)))
        return res;
    QTextStream in(&file);
    do {
        QString line = in.readLine();
        int idx = line.indexOf(QLS(": "));
        if (idx > 0)
            res.insert(line.left(idx), line.mid(idx+2));
    } while (!in.atEnd());
    return res;
}


void BaselineHandler::idleClientTimeout()
{
    qWarning() << runId << logtime() << "Idle client timeout: no request received for" << IDLE_CLIENT_TIMEOUT << "seconds, terminating connection.";
    proto.socket.disconnectFromHost();
}


void BaselineHandler::receiveDisconnect()
{
    qDebug() << runId << logtime() << "Client disconnected.";
    report.end();
    if (report.reportProduced() && !clientInfo.isAdHocRun())
        issueMismatchNotification();
    if (settings && settings->value("ProcessXmlResults").toBool() && !clientInfo.isAdHocRun()) {
        // ### TBD: actually execute the processing command. For now, just generate the xml files.
        QString xmlDir = report.writeResultsXmlFiles();
    }
    QThread::currentThread()->exit(0);
}


PlatformInfo BaselineHandler::mapPlatformInfo(const PlatformInfo& orig) const
{
    PlatformInfo mapped;
    foreach (const QString &key, orig.uniqueKeys()) {
        QString val = orig.value(key).simplified();
        val.replace(QLC('/'), QLC('_'));
        val.replace(QLC(' '), QLC('_'));
        mapped.insert(key, QUrl::toPercentEncoding(val, "+"));
        //qDebug() << "MAPPED" << key << "FROM" << orig.value(key) << "TO" << mapped.value(key);
    }

    // Special fixup for OS version
    if (mapped.value(PI_OSName) == QLS("MacOS")) {
        int ver = mapped.value(PI_OSVersion).toInt();
        if (ver > 1)
            mapped.insert(PI_OSVersion, QString("MV_10_%1").arg(ver-2));
    }
    else if (mapped.value(PI_OSName) == QLS("Windows")) {
        // TBD: map windows version numbers to names
    }

    // Special fixup for hostname
    QString host = mapped.value(PI_HostName).section(QLC('.'), 0, 0);  // Filter away domain, if any
    if (host.isEmpty() || host == QLS("localhost")) {
        host = orig.value(PI_HostAddress);
    } else {
        if (!orig.isAdHocRun()) {    // i.e. CI system run, so remove index postfix typical of vm hostnames
            host.remove(QRegExp(QLS("\\d+$")));
            if (host.endsWith(QLC('-')))
                host.chop(1);
        }
    }
    if (host.isEmpty())
        host = QLS("UNKNOWN-HOST");
    if (mapped.value(PI_OSName) == QLS("MacOS"))        // handle multiple os versions on same host
        host += QLC('-') + mapped.value(PI_OSVersion);
    mapped.insert(PI_HostName, host);

    // Special fixup for Qt version
    QString ver = mapped.value(PI_QtVersion);
    if (!ver.isEmpty())
        mapped.insert(PI_QtVersion, ver.prepend(QLS("Qt-")));

    return mapped;
}


QString BaselineHandler::pathForItem(const ImageItem &item, bool isBaseline, bool absolute) const
{
    if (mappedClientInfo.isEmpty()) {
        mappedClientInfo = mapPlatformInfo(clientInfo);
        PlatformInfo oraw = clientInfo;
        // ### simplify: don't map if no overrides!
        for (int i = 0; i < clientInfo.overrides().size()-1; i+=2)
            oraw.insert(clientInfo.overrides().at(i), clientInfo.overrides().at(i+1));
        overriddenMappedClientInfo = mapPlatformInfo(oraw);
    }

    const PlatformInfo& mapped = isBaseline ? overriddenMappedClientInfo : mappedClientInfo;

    QString itemName = safeName(item.itemName);
    itemName.append(QLC('_') + QString::number(item.itemChecksum, 16).rightJustified(4, QLC('0')));

    QStringList path;
    path += projectPath(absolute);
    path += mapped.value(PI_TestCase);
    path += QLS(isBaseline ? "baselines" : "mismatches");
    path += item.testFunction;
    QStringList itemPathKeys;
    if (settings)
        itemPathKeys = settings->value("ItemPathKeys").toStringList();
    if (itemPathKeys.isEmpty())
        itemPathKeys = BaselineServer::defaultPathKeys();
    foreach (const QString &key, itemPathKeys)
        path += mapped.value(key, QLS("UNSET-")+key);
    if (!isBaseline)
        path += runId;
    path += itemName + QLC('.');

    return path.join(QLS("/"));
}


QString BaselineHandler::view(const QString &baseline, const QString &rendered, const QString &compared)
{
    QFile f(":/templates/view.html");
    f.open(QIODevice::ReadOnly);
    return QString::fromLatin1(f.readAll()).arg('/'+baseline, '/'+rendered, '/'+compared, diffstats(baseline, rendered));
}

QString BaselineHandler::diffstats(const QString &baseline, const QString &rendered)
{
    QImage blImg(BaselineServer::storagePath() + QLC('/') + baseline);
    QImage mmImg(BaselineServer::storagePath() + QLC('/') + rendered);
    if (blImg.isNull() || mmImg.isNull())
        return QLS("Could not compute diffstats: image loading failed.");

    // ### TBD: cache the results
    return computeMismatchScore(blImg, mmImg);
}

QString BaselineHandler::clearAllBaselines(const QString &context)
{
    int tot = 0;
    int failed = 0;
    QDirIterator it(BaselineServer::storagePath() + QLC('/') + context,
                    QStringList() << QLS("*.") + QLS(FileFormat)
                                  << QLS("*.") + QLS(MetadataFileExt)
                                  << QLS("*.") + QLS(ThumbnailExt));
    while (it.hasNext()) {
        bool counting = !it.next().endsWith(QLS(ThumbnailExt));
        if (counting)
            tot++;
        if (!QFile::remove(it.filePath()) && counting)
            failed++;
    }
    return QString(QLS("%1 of %2 baselines cleared from context ")).arg((tot-failed)/2).arg(tot/2) + context;
}

QString BaselineHandler::updateBaselines(const QString &context, const QString &mismatchContext, const QString &itemFile)
{
    int tot = 0;
    int failed = 0;
    QString storagePrefix = BaselineServer::storagePath() + QLC('/');
    // If itemId is set, update just that one, otherwise, update all:
    QString filter = itemFile.isEmpty() ? QLS("*_????.") : itemFile;
    QDirIterator it(storagePrefix + mismatchContext,
                    QStringList() << filter + QLS(FileFormat)
                                  << filter + QLS(MetadataFileExt)
                                  << filter + QLS(ThumbnailExt));
    while (it.hasNext()) {
        bool counting = !it.next().endsWith(QLS(ThumbnailExt));
        if (counting)
            tot++;
        QString oldFile = storagePrefix + context + QLC('/') + it.fileName();
        QFile::remove(oldFile);                                   // Remove existing baseline file
        if (!QFile::copy(it.filePath(), oldFile) && counting)     // and replace it with the mismatch
            failed++;
    }
    return QString(QLS("%1 of %2 baselines updated in context %3 from context %4")).arg((tot-failed)/2).arg(tot/2).arg(context, mismatchContext);
}

QString BaselineHandler::blacklistTest(const QString &context, const QString &itemId, bool removeFromBlacklist)
{
    QFile file(BaselineServer::storagePath() + QLC('/') + context + QLS("/BLACKLIST"));
    QStringList blackList;
    if (file.open(QIODevice::ReadWrite)) {
        while (!file.atEnd())
            blackList.append(file.readLine().trimmed());

        if (removeFromBlacklist)
            blackList.removeAll(itemId);
        else if (!blackList.contains(itemId))
            blackList.append(itemId);

        file.resize(0);
        foreach (QString id, blackList)
            file.write(id.toLatin1() + '\n');
        file.close();
        return QLS(removeFromBlacklist ? "Whitelisted " : "Blacklisted ") + itemId + QLS(" in context ") + context;
    } else {
        return QLS("Unable to update blacklisted tests, failed to open ") + file.fileName();
    }
}


void BaselineHandler::testPathMapping()
{
    qDebug() << "Storage prefix:" << BaselineServer::storagePath();

    QStringList hosts;
    hosts << QLS("bq-ubuntu910-x86-01")
          << QLS("bq-ubuntu910-x86-15")
          << QLS("osl-mac-master-5.test.qt-project.org")
          << QLS("osl-mac-master-6.test.qt-project.org")
          << QLS("sv-xp-vs-010")
          << QLS("sv-xp-vs-011")
          << QLS("sv-solaris-sparc-008")
          << QLS("macbuilder-02.test.troll.no")
          << QLS("bqvm1164")
          << QLS("chimera")
          << QLS("localhost")
          << QLS("");

    ImageItem item;
    item.testFunction = QLS("testPathMapping");
    item.itemName = QLS("arcs.qps");
    item.imageChecksums << 0x0123456789abcdefULL;
    item.itemChecksum = 0x0123;

    clientInfo.insert(PI_QtVersion, QLS("5.0.0"));
    clientInfo.insert(PI_QMakeSpec, QLS("linux-g++"));
    clientInfo.insert(PI_PulseGitBranch, QLS("somebranch"));
    clientInfo.setAdHocRun(false);
    foreach(const QString& host, hosts) {
        mappedClientInfo.clear();
        clientInfo.insert(PI_HostName, host);
        qDebug() << "Baseline from" << host << "->" << pathForItem(item, true);
        qDebug() << "Mismatch from" << host << "->" << pathForItem(item, false);
    }
}


QString BaselineHandler::computeMismatchScore(const QImage &baseline, const QImage &rendered)
{
    if (baseline.size() != rendered.size() || baseline.format() != rendered.format())
        return QLS("[No diffstats, incomparable images.]");
    if (baseline.depth() != 32)
        return QLS("[Diffstats computation not implemented for format.]");

    int w = baseline.width();
    int h = baseline.height();

    uint ncd = 0; // number of differing color pixels
    uint nad = 0; // number of differing alpha pixels
    uint scd = 0; // sum of color pixel difference
    uint sad = 0; // sum of alpha pixel difference
    uint mind = 0; // minimum difference
    uint maxd = 0; // maximum difference

    for (int y=0; y<h; ++y) {
        const QRgb *bl = (const QRgb *) baseline.constScanLine(y);
        const QRgb *rl = (const QRgb *) rendered.constScanLine(y);
        for (int x=0; x<w; ++x) {
            QRgb b = bl[x];
            QRgb r = rl[x];
            if (r != b) {
                uint dr = qAbs(qRed(b) - qRed(r));
                uint dg = qAbs(qGreen(b) - qGreen(r));
                uint db = qAbs(qBlue(b) - qBlue(r));
                uint ds = (dr + dg + db) / 3;
                uint da = qAbs(qAlpha(b) - qAlpha(r));
                if (ds) {
                    ncd++;
                    scd += ds;
                    if (!mind || ds < mind)
                        mind = ds;
                    if (ds > maxd)
                        maxd = ds;
                }
                if (da) {
                    nad++;
                    sad += da;
                }
            }
        }
    }


    double pcd = 100.0 * ncd / (w*h);  // percent of pixels that differ
    double acd = ncd ? double(scd) / (ncd) : 0;         // avg. difference
/*
    if (baseline.hasAlphaChannel()) {
        double pad = 100.0 * nad / (w*h);  // percent of pixels that differ
        double aad = nad ? double(sad) / (3*nad) : 0;         // avg. difference
    }
*/
    QString res = "<table>\n";
    QString item = "<tr><td>%1</td><td align=right>%2</td></tr>\n";
    res += item.arg("Number of mismatching pixels").arg(ncd);
    res += item.arg("Percentage mismatching pixels").arg(pcd, 0, 'g', 2);
    res += item.arg("Minimum pixel distance").arg(mind);
    res += item.arg("Maximum pixel distance").arg(maxd);
    if (acd >= 10.0)
        res += item.arg("Average pixel distance").arg(qRound(acd));
    else
        res += item.arg("Average pixel distance").arg(acd, 0, 'g', 2);

    if (baseline.hasAlphaChannel())
        res += item.arg("Number of mismatching alpha values").arg(nad);

    res += "</table>\n";
    res += "<p>(Distances are normalized to the range 0-255)</p>\n";
    return res;
}


bool BaselineHandler::fuzzyCompare(const QString &baselinePath, const QString &mismatchPath)
{
    QProcess compareProc;
    QStringList args;
    args << "-fuzz" << QString("%1%").arg(fuzzLevel) << "-metric" << "AE";
    args << baselinePath + QLS(FileFormat) << mismatchPath + QLS(FileFormat) << "/dev/null";  // TBD: Should save output image, so report won't have to regenerate it

    compareProc.setProcessChannelMode(QProcess::MergedChannels);
    compareProc.start("compare", args, QIODevice::ReadOnly);
    if (compareProc.waitForFinished(3000) && compareProc.error() == QProcess::UnknownError) {
        bool ok = false;
        int metric = compareProc.readAll().trimmed().toInt(&ok);
        if (ok && metric == 0)
            return true;
    }
    return false;
}


void BaselineHandler::issueMismatchNotification()
{
    // KISS: hardcoded use of the "sendemail" utility. Make this configurable if and when demand arises.
    if (!settings)
        return;

    settings->beginGroup("Notification");
    QStringList receivers = settings->value("Receivers").toStringList();
    QString sender = settings->value("Sender").toString();
    QString server = settings->value("SMTPserver").toString();
    settings->endGroup();
    if (receivers.isEmpty() || sender.isEmpty() || server.isEmpty())
        return;

    QString msg = QString("\nResult summary for test run %1:\n").arg(runId);
    msg += report.summary();
    msg += "\nReport: " + BaselineServer::baseUrl() + report.filePath() + "\n";

    msg += "\nTest run platform properties:\n------------------\n";
    foreach (const QString &key, clientInfo.keys())
        msg += key + ":  " + clientInfo.value(key) + '\n';
    msg += "\nCheers,\n- Your friendly Lancelot Baseline Server\n";

    QProcess proc;
    QString cmd = "sendemail";
    QStringList args;
    args << "-s" << server << "-f" << sender << "-t" << receivers;
    args << "-u" << "[Lancelot] Mismatch report for project " + clientInfo.value(PI_Project) + ", test case " + clientInfo.value(PI_TestCase);
    args << "-m" << msg;

    //proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(cmd, args);
    if (!proc.waitForFinished(10 * 1000) || (proc.exitStatus() != QProcess::NormalExit) || proc.exitCode()) {
        qWarning() << "FAILED to issue notification. Command:" << cmd << args.mid(0, args.size()-2);
        qWarning() << "    Command standard output:" << proc.readAllStandardOutput();
        qWarning() << "    Command error output:" << proc.readAllStandardError();
    }
}


// Make an identifer safer for use as filename and URL
QString safeName(const QString& name)
{
    QString res = name.simplified();
    res.replace(QLC(' '), QLC('_'));
    res.replace(QLC('.'), QLC('_'));
    res.replace(QLC('/'), QLC('^'));
    return res;
}
