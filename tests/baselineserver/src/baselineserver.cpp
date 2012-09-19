/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

// extra fields, for use in image metadata storage
const QString PI_ImageChecksum(QLS("ImageChecksum"));
const QString PI_RunId(QLS("RunId"));
const QString PI_CreationDate(QLS("CreationDate"));

QString BaselineServer::storage;
QString BaselineServer::url;
QString BaselineServer::settingsFile;

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

QString BaselineServer::settingsFilePath()
{
    if (settingsFile.isEmpty()) {
        QString exeName = QCoreApplication::applicationFilePath().section(QLC('/'), -1);
        settingsFile = storagePath() + QLC('/') + exeName + QLS(".ini");
    }
    return settingsFile;
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
    : QObject(), runId(runId), connectionEstablished(false)
{
    settings = new QSettings(BaselineServer::settingsFilePath(), QSettings::IniFormat, this);

    if (socketDescriptor == -1)
        return;

    connect(&proto.socket, SIGNAL(readyRead()), this, SLOT(receiveRequest()));
    connect(&proto.socket, SIGNAL(disconnected()), this, SLOT(receiveDisconnect()));
    proto.socket.setSocketDescriptor(socketDescriptor);
}

const char *BaselineHandler::logtime()
{
    return 0;
    //return QTime::currentTime().toString(QLS("mm:ss.zzz"));
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
             << "[" << qPrintable(clientInfo.value(PI_HostAddress)) << "]" << logMsg
             << "Overrides:" << clientInfo.overrides() << "AdHoc-Run:" << clientInfo.isAdHocRun();

    //### Temporarily override the client setting, for client compatibility:
    if (!clientInfo.isAdHocRun())
        clientInfo.setAdHocRun(clientInfo.value(PI_PulseGitBranch).isEmpty() && clientInfo.value(PI_PulseTestrBranch).isEmpty());

    settings->beginGroup("ClientFilters");
    if (!clientInfo.isAdHocRun()) {         // for CI runs, allow filtering of clients. TBD: different filters (settings file) per testCase
        foreach (QString filterKey, settings->childKeys()) {
            QString filter = settings->value(filterKey).toString();
            QString platVal = clientInfo.value(filterKey);
            if (filter.isEmpty())
                continue;  // tbd: add a syntax for specifying a "value-must-be-present" filter
            if (!platVal.contains(filter)) {
                qDebug() << runId << logtime() << "Did not pass client filter on" << filterKey << "; disconnecting.";
                proto.sendBlock(BaselineProtocol::Abort, QByteArray("Configured to not do testing for this client or repo, ref. ") + BaselineServer::settingsFilePath().toLatin1());
                proto.socket.disconnectFromHost();
                return false;
            }
        }
    }
    settings->endGroup();

    proto.sendBlock(BaselineProtocol::Ack, QByteArray());

    report.init(this, runId, clientInfo);
    return true;
}

void BaselineHandler::receiveRequest()
{
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

    QString prefix = pathForItem(item, isBaseline);
    qDebug() << runId << logtime() << "Received" << (isBaseline ? "baseline" : "mismatched") << "image for:" << item.itemName << "Storing in" << prefix;

    QString msg;
    if (isBaseline)
        msg = QLS("New baseline image stored: ") + pathForItem(item, true, true) + QLS(FileFormat);
    else
        msg = BaselineServer::baseUrl() + report.filePath();
    proto.sendBlock(BaselineProtocol::Ack, msg.toLatin1());

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

    if (!isBaseline)
        report.addMismatch(item);
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
    if (!file.open(QIODevice::ReadOnly))
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


void BaselineHandler::receiveDisconnect()
{
    qDebug() << runId << logtime() << "Client disconnected.";
    report.end();
    QThread::currentThread()->exit(0);
}


PlatformInfo BaselineHandler::mapPlatformInfo(const PlatformInfo& orig) const
{
    PlatformInfo mapped = orig;

    // Map hostname
    QString host = orig.value(PI_HostName).section(QLC('.'), 0, 0);  // Filter away domain, if any
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
        host = QLS("unknownhost");
    mapped.insert(PI_HostName, host);

    // Map qmakespec
    QString mkspec = orig.value(PI_QMakeSpec);
    mapped.insert(PI_QMakeSpec, mkspec.replace(QLC('/'), QLC('_')));

    // Map Qt version
    QString ver = orig.value(PI_QtVersion);
    mapped.insert(PI_QtVersion, ver.prepend(QLS("Qt-")));

    return mapped;
}


QString BaselineHandler::pathForItem(const ImageItem &item, bool isBaseline, bool absolute) const
{
    if (mappedClientInfo.isEmpty()) {
        mappedClientInfo = mapPlatformInfo(clientInfo);
        PlatformInfo oraw = clientInfo;
        for (int i = 0; i < clientInfo.overrides().size()-1; i+=2)
            oraw.insert(clientInfo.overrides().at(i), clientInfo.overrides().at(i+1));
        overriddenMappedClientInfo = mapPlatformInfo(oraw);
    }

    const PlatformInfo& mapped = isBaseline ? overriddenMappedClientInfo : mappedClientInfo;

    QString itemName = item.itemName.simplified();
    itemName.replace(QLC(' '), QLC('_'));
    itemName.replace(QLC('.'), QLC('_'));
    itemName.append(QLC('_'));
    itemName.append(QString::number(item.itemChecksum, 16).rightJustified(4, QLC('0')));

    QStringList path;
    if (absolute)
        path += BaselineServer::storagePath();
    path += mapped.value(PI_TestCase);
    path += QLS(isBaseline ? "baselines" : "mismatches");
    path += item.testFunction;
    path += mapped.value(PI_QtVersion);
    path += mapped.value(PI_QMakeSpec);
    path += mapped.value(PI_HostName);
    if (!isBaseline)
        path += runId;
    path += itemName + QLC('.');

    return path.join(QLS("/"));
}


QString BaselineHandler::view(const QString &baseline, const QString &rendered, const QString &compared)
{
    QFile f(":/templates/view.html");
    f.open(QIODevice::ReadOnly);
    return QString::fromLatin1(f.readAll()).arg('/'+baseline, '/'+rendered, '/'+compared);
}


QString BaselineHandler::clearAllBaselines(const QString &context)
{
    int tot = 0;
    int failed = 0;
    QDirIterator it(BaselineServer::storagePath() + QLC('/') + context,
                    QStringList() << QLS("*.") + QLS(FileFormat) << QLS("*.") + QLS(MetadataFileExt));
    while (it.hasNext()) {
        tot++;
        if (!QFile::remove(it.next()))
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
    QDirIterator it(storagePrefix + mismatchContext, QStringList() << filter + QLS(FileFormat) << filter + QLS(MetadataFileExt));
    while (it.hasNext()) {
        tot++;
        it.next();
        QString oldFile = storagePrefix + context + QLC('/') + it.fileName();
        QFile::remove(oldFile);                       // Remove existing baseline file
        if (!QFile::copy(it.filePath(), oldFile))     // and replace it with the mismatch
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
          << QLS("osl-mac-master-5.test.qt.nokia.com")
          << QLS("osl-mac-master-6.test.qt.nokia.com")
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
        return QLS("[No score, incomparable images.]");
    if (baseline.depth() != 32)
        return QLS("[Score computation not implemented for format.]");

    int w = baseline.width();
    int h = baseline.height();

    uint ncd = 0; // number of differing color pixels
    uint nad = 0; // number of differing alpha pixels
    uint scd = 0; // sum of color pixel difference
    uint sad = 0; // sum of alpha pixel difference

    for (int y=0; y<h; ++y) {
        const QRgb *bl = (const QRgb *) baseline.constScanLine(y);
        const QRgb *rl = (const QRgb *) rendered.constScanLine(y);
        for (int x=0; x<w; ++x) {
            QRgb b = bl[x];
            QRgb r = rl[x];
            if (r != b) {
                int dr = qAbs(qRed(b) - qRed(r));
                int dg = qAbs(qGreen(b) - qGreen(r));
                int db = qAbs(qBlue(b) - qBlue(r));
                int ds = dr + dg + db;
                int da = qAbs(qAlpha(b) - qAlpha(r));
                if (ds) {
                    ncd++;
                    scd += ds;
                }
                if (da) {
                    nad++;
                    sad += da;
                }
            }
        }
    }

    double pcd = 100.0 * ncd / (w*h);  // percent of pixels that differ
    double acd = ncd ? double(scd) / (3*ncd) : 0;         // avg. difference
    QString res = QString(QLS("Diffscore: %1% (Num:%2 Avg:%3)")).arg(pcd, 0, 'g', 2).arg(ncd).arg(acd, 0, 'g', 2);
    if (baseline.hasAlphaChannel()) {
        double pad = 100.0 * nad / (w*h);  // percent of pixels that differ
        double aad = nad ? double(sad) / (3*nad) : 0;         // avg. difference
        res += QString(QLS(" Alpha-diffscore: %1% (Num:%2 Avg:%3)")).arg(pad, 0, 'g', 2).arg(nad).arg(aad, 0, 'g', 2);
    }
    return res;
}
