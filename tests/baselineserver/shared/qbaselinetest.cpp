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

#include "qbaselinetest.h"
#include "baselineprotocol.h"
#include <QtCore/QProcess>
#include <QtCore/QDir>

#define MAXCMDLINEARGS 128

namespace QBaselineTest {

static char *fargv[MAXCMDLINEARGS];
static bool simfail = false;
static PlatformInfo customInfo;
static bool customAutoModeSet = false;

static BaselineProtocol proto;
static bool connected = false;
static bool triedConnecting = false;
static bool dryRunMode = false;

static QByteArray curFunction;
static ImageItemList itemList;
static bool gotBaselines;

static QString definedTestProject;
static QString definedTestCase;


void handleCmdLineArgs(int *argcp, char ***argvp)
{
    if (!argcp || !argvp)
        return;

    bool showHelp = false;

    int fargc = 0;
    int numArgs = *argcp;

    for (int i = 0; i < numArgs; i++) {
        QByteArray arg = (*argvp)[i];
        QByteArray nextArg = (i+1 < numArgs) ? (*argvp)[i+1] : 0;

        if (arg == "-simfail") {
            simfail = true;
        } else if (arg == "-auto") {
            customAutoModeSet = true;
            customInfo.setAdHocRun(false);
        } else if (arg == "-adhoc") {
            customAutoModeSet = true;
            customInfo.setAdHocRun(true);
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
        } else {
            if ( (arg == "-help") || (arg == "--help") )
                showHelp = true;
            if (fargc >= MAXCMDLINEARGS) {
                qWarning() << "Too many command line arguments!";
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
        out << " -simfail            : Force an image comparison mismatch. For testing purposes.\n";
        out << " -auto               : Inform server that this run is done by a daemon, CI system or similar.\n";
        out << " -adhoc (default)    : The inverse of -auto; this run is done by human, e.g. for testing.\n";
        out << " -compareto KEY=VAL  : Force comparison to baselines from a different client,\n";
        out << "                       for example: -compareto QtVersion=4.8.0\n";
        out << "                       Multiple -compareto client specifications may be given.\n";
        out << "\n";
    }
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
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#else
    QString script = "hostinfo.sh";  //### TBD: Windows implementation (hostinfo.bat)

    QProcess runScript;
    runScript.setWorkingDirectory(QCoreApplication::applicationDirPath());
    runScript.start("sh", QStringList() << script, QIODevice::ReadOnly);
    if (!runScript.waitForFinished(5000) || runScript.error() != QProcess::UnknownError) {
        qWarning() << "QBaselineTest: Error running script" << runScript.workingDirectory() + QDir::separator() + script << ":" << runScript.errorString();
        qDebug() << "  stderr:" << runScript.readAllStandardError().trimmed();
    }
    while (!runScript.atEnd()) {
        QByteArray line = runScript.readLine().trimmed();   // ###local8bit? utf8?
        QString key, val;
        int colonPos = line.indexOf(':');
        if (colonPos > 0) {
            key = line.left(colonPos).simplified().replace(' ', '_');
            val = line.mid(colonPos+1).trimmed();
        }
        if (!key.isEmpty() && key.length() < 64 && val.length() < 256)  // ###TBD: maximum 256 chars in value?
            addClientProperty(key, val);
        else
            qDebug() << "Unparseable script output ignored:" << line;
    }
#endif // !QT_NO_PROCESS
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
    foreach (QString key, defaultInfo.keys()) {
        if (!clientInfo.contains(key))
            clientInfo.insert(key, defaultInfo.value(key));
    }
    if (!customAutoModeSet)
        clientInfo.setAdHocRun(defaultInfo.isAdHocRun());

    if (!definedTestProject.isEmpty())
        clientInfo.insert(PI_Project, definedTestProject);

    QString testCase = definedTestCase;
    if (testCase.isEmpty() && QTest::testObject() && QTest::testObject()->metaObject()) {
        //qDebug() << "Trying to Read TestCaseName from Testlib!";
        testCase = QTest::testObject()->metaObject()->className();
    }
    if (testCase.isEmpty()) {
        qWarning("QBaselineTest::connect: No test case name specified, cannot connect.");
        return false;
    }

    if (!proto.connect(testCase, &dryRunMode, clientInfo)) {
        *msg += "Failed to connect to baseline server: " + proto.errorMessage().toLatin1();
        *error = true;
        return false;
    }
    connected = true;
    return true;
}

bool disconnectFromBaselineServer()
{
    if (proto.disconnect()) {
        connected = false;
        triedConnecting = false;
        return true;
    }

    return false;
}

bool connectToBaselineServer(QByteArray *msg, const QString &testProject, const QString &testCase)
{
    bool dummy;
    QByteArray dummyMsg;

    definedTestProject = testProject;
    definedTestCase = testCase;

    return connect(msg ? msg : &dummyMsg, &dummy);
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
        if (!customInfo.overrides().isEmpty()) {
            qWarning() << "Cannot compare to other system's baseline: No such baseline found on server.";
            return true;
        }
        if (proto.submitNewBaseline(item, &srvMsg))
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
    *error = false;
    // The actual comparison of the given image with the baseline:
    if (baseline.imageChecksums.contains(item.imageChecksums.at(0))) {
        if (!proto.submitMatch(item, &srvMsg))
            qWarning() << "Failed to report image match to server:" << srvMsg;
        return true;
    }
    bool fuzzyMatch = false;
    bool res = proto.submitMismatch(item, &srvMsg, &fuzzyMatch);
    if (res && fuzzyMatch) {
        *error = true;          // To force a QSKIP/debug output; somewhat kludgy
        *msg += srvMsg;
        return true;            // The server decides: a fuzzy match means no mismatch
    }
    *msg += "Mismatch. See report:\n   " + srvMsg;
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
    ImageItemList list;
    list.append(item);
    if (!proto.requestBaselineChecksums(QLatin1String(QTest::currentTestFunction()), &list) || list.isEmpty()) {
        *msg = "Communication with baseline server failed: " + proto.errorMessage().toLatin1();
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


bool testImage(const QImage& img, QByteArray *msg, bool *error)
{
    if (!connected && !connect(msg, error))
        return true;

    if (QTest::currentTestFunction() != curFunction || itemList.isEmpty()) {
        qWarning() << "Usage error: QBASELINE_TEST used without corresponding QBaselineTest::newRow()";
        return true;
    }

    if (!gotBaselines) {
        if (!proto.requestBaselineChecksums(QString::fromLatin1(QTest::currentTestFunction()), &itemList) || itemList.isEmpty()) {
            *msg = "Communication with baseline server failed: " + proto.errorMessage().toLatin1();
            *error = true;
            return true;
        }
        gotBaselines = true;
    }

    QString curTag = QString::fromLatin1(QTest::currentDataTag());
    ImageItemList::const_iterator it = itemList.constBegin();
    while (it != itemList.constEnd() && it->itemName != curTag)
        ++it;
    if (it == itemList.constEnd()) {
        qWarning() << "Usage error: QBASELINE_TEST used without corresponding QBaselineTest::newRow() for row" << curTag;
        return true;
    }
    return compareItem(*it, img, msg, error);
}

}
