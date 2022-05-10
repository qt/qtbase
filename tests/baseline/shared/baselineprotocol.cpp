// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "baselineprotocol.h"
#include <QLibraryInfo>
#include <QImage>
#include <QBuffer>
#include <QHostInfo>
#include <QSysInfo>
#if QT_CONFIG(process)
# include <QProcess>
#endif
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <QTime>
#include <QPointer>
#include <QRegularExpression>

const QString PI_Project(QLS("Project"));
const QString PI_ProjectImageKeys(QLS("ProjectImageKeys"));
const QString PI_TestCase(QLS("TestCase"));
const QString PI_HostName(QLS("HostName"));
const QString PI_HostAddress(QLS("HostAddress"));
const QString PI_OSName(QLS("OSName"));
const QString PI_OSVersion(QLS("OSVersion"));
const QString PI_QtVersion(QLS("QtVersion"));
const QString PI_QtBuildMode(QLS("QtBuildMode"));
const QString PI_GitCommit(QLS("GitCommit"));
const QString PI_GitBranch(QLS("GitBranch"));

PlatformInfo PlatformInfo::localHostInfo()
{
    PlatformInfo pi;
    pi.insert(PI_HostName, QHostInfo::localHostName());
    pi.insert(PI_QtVersion, QLS(qVersion()));
    pi.insert(PI_QtBuildMode, QLibraryInfo::isDebugBuild() ? QLS("QtDebug") : QLS("QtRelease"));
#if defined(Q_OS_LINUX) && QT_CONFIG(process)
    pi.insert(PI_OSName, QLS("Linux"));
#elif defined(Q_OS_WIN)
    pi.insert(PI_OSName, QLS("Windows"));
#elif defined(Q_OS_DARWIN)
    pi.insert(PI_OSName, QLS("Darwin"));
#else
    pi.insert(PI_OSName, QLS("Other"));
#endif
    pi.insert(PI_OSVersion, QSysInfo::kernelVersion());

    QString gc = qEnvironmentVariable("BASELINE_GIT_COMMIT");
#if QT_CONFIG(process)
    if (gc.isEmpty()) {
        QProcess git;
        QString cmd;
        QStringList args;
    #if defined(Q_OS_WIN)
        cmd = QLS("cmd.exe");
        args << QLS("/c") << QLS("git");
    #else
        cmd = QLS("git");
    #endif
        args << QLS("log") << QLS("--max-count=1") << QLS("--pretty=%H [%an] [%ad] %s");
        git.start(cmd, args);
        git.waitForFinished(3000);
        if (!git.exitCode())
            gc = QString::fromLocal8Bit(git.readAllStandardOutput().constData()).simplified();
    }
#endif // QT_CONFIG(process)
    pi.insert(PI_GitCommit, gc.isEmpty() ? QLS("Unknown") : gc);

    if (qEnvironmentVariableIsSet("JENKINS_HOME"))
        pi.setAdHocRun(false);

    QString gb = qEnvironmentVariable("GIT_BRANCH");
    if (!gb.isEmpty())
        pi.insert(PI_GitBranch, gb);

    return pi;
}


void PlatformInfo::addOverride(const QString& key, const QString& value)
{
    orides.append(key);
    orides.append(value);
}


QStringList PlatformInfo::overrides() const
{
    return orides;
}


void PlatformInfo::setAdHocRun(bool isAdHoc)
{
    adHoc = isAdHoc;
}


bool PlatformInfo::isAdHocRun() const
{
    return adHoc;
}


QDataStream & operator<< (QDataStream &stream, const PlatformInfo &pi)
{
    stream << static_cast<const QMap<QString, QString>&>(pi);
    stream << pi.orides << pi.adHoc;
    return stream;
}


QDataStream & operator>> (QDataStream &stream, PlatformInfo &pi)
{
    stream >> static_cast<QMap<QString, QString>&>(pi);
    stream >> pi.orides >> pi.adHoc;
    return stream;
}


// Defined in lookup3.c:
void hashword2 (
const quint32 *k,         /* the key, an array of quint32 values */
size_t         length,    /* the length of the key, in quint32s */
quint32       *pc,        /* IN: seed OUT: primary hash value */
quint32       *pb);       /* IN: more seed OUT: secondary hash value */

quint64 ImageItem::computeChecksum(const QImage &image)
{
    QImage img(image);
    const qsizetype bpl = img.bytesPerLine();
    const int padBytes = bpl - (qsizetype(img.width()) * img.depth() / 8);
    if (padBytes) {
        uchar *p = img.bits() + bpl - padBytes;
        const int h = img.height();
        for (int y = 0; y < h; ++y) {
            memset(p, 0, padBytes);
            p += bpl;
        }
    }

    quint32 h1 = 0xfeedbacc;
    quint32 h2 = 0x21604894;
    hashword2((const quint32 *)img.constBits(), img.sizeInBytes()/4, &h1, &h2);
    return (quint64(h1) << 32) | h2;
}

#if 0
QString ImageItem::engineAsString() const
{
    switch (engine) {
    case Raster:
        return QLS("Raster");
        break;
    case OpenGL:
        return QLS("OpenGL");
        break;
    default:
        break;
    }
    return QLS("Unknown");
}

QString ImageItem::formatAsString() const
{
    static const int numFormats = 16;
    static const char *formatNames[numFormats] = {
        "Invalid",
        "Mono",
        "MonoLSB",
        "Indexed8",
        "RGB32",
        "ARGB32",
        "ARGB32-Premult",
        "RGB16",
        "ARGB8565-Premult",
        "RGB666",
        "ARGB6666-Premult",
        "RGB555",
        "ARGB8555-Premult",
        "RGB888",
        "RGB444",
        "ARGB4444-Premult"
    };
    if (renderFormat < 0 || renderFormat >= numFormats)
        return QLS("UnknownFormat");
    return QLS(formatNames[renderFormat]);
}
#endif

void ImageItem::writeImageToStream(QDataStream &out) const
{
    if (image.isNull() || image.format() == QImage::Format_Invalid) {
        out << quint8(0);
        return;
    }
    out << quint8('Q') << quint8(image.format());
    out << quint8(QSysInfo::ByteOrder) << quint8(0);       // pad to multiple of 4 bytes
    out << quint32(image.width()) << quint32(image.height()) << quint32(image.bytesPerLine());
    out << qCompress(reinterpret_cast<const uchar *>(image.constBits()), image.sizeInBytes());
    //# can be followed by colormap for formats that use it
}

void ImageItem::readImageFromStream(QDataStream &in)
{
    quint8 hdr, fmt, endian, pad;
    quint32 width, height, bpl;
    QByteArray data;

    in >> hdr;
    if (hdr != 'Q') {
        image = QImage();
        return;
    }
    in >> fmt >> endian >> pad;
    if (!fmt || fmt >= QImage::NImageFormats) {
        image = QImage();
        return;
    }
    if (endian != QSysInfo::ByteOrder) {
        qWarning("ImageItem cannot read streamed image with different endianness");
        image = QImage();
        return;
    }
    in >> width >> height >> bpl;
    in >> data;
    data = qUncompress(data);
    QImage res((const uchar *)data.constData(), width, height, bpl, QImage::Format(fmt));
    image = res.copy();  //# yuck, seems there is currently no way to avoid data copy
}

QDataStream & operator<< (QDataStream &stream, const ImageItem &ii)
{
    stream << ii.testFunction << ii.itemName << ii.itemChecksum << quint8(ii.status) << ii.imageChecksums << ii.misc;
    ii.writeImageToStream(stream);
    return stream;
}

QDataStream & operator>> (QDataStream &stream, ImageItem &ii)
{
    quint8 encStatus;
    stream >> ii.testFunction >> ii.itemName >> ii.itemChecksum >> encStatus >> ii.imageChecksums >> ii.misc;
    ii.status = ImageItem::ItemStatus(encStatus);
    ii.readImageFromStream(stream);
    return stream;
}

BaselineProtocol::BaselineProtocol()
{
}

BaselineProtocol::~BaselineProtocol()
{
    disconnect();
}

bool BaselineProtocol::disconnect()
{
    socket.close();
    return (socket.state() == QTcpSocket::UnconnectedState) ? true : socket.waitForDisconnected(Timeout);
}


bool BaselineProtocol::connect(const QString &testCase, bool *dryrun, const PlatformInfo& clientInfo)
{
    errMsg.clear();
    QByteArray serverName(qgetenv("QT_LANCELOT_SERVER"));
    if (serverName.isNull())
        serverName = "lancelot.test.qt-project.org";

    socket.connectToHost(serverName, ServerPort);
    if (!socket.waitForConnected(Timeout)) {
        QThread::msleep(3000);  // Wait a bit and try again, the server might just be restarting
        if (!socket.waitForConnected(Timeout)) {
            errMsg += QLS("TCP connectToHost failed. Host:") + QLS(serverName) + QLS(" port:") + QString::number(ServerPort);
            return false;
        }
    }

    PlatformInfo pi = clientInfo.isEmpty() ? PlatformInfo::localHostInfo() : clientInfo;
    pi.insert(PI_TestCase, testCase);
    QByteArray block;
    QDataStream ds(&block, QIODevice::ReadWrite);
    ds << pi;
    if (!sendBlock(AcceptPlatformInfo, block)) {
        errMsg += QLS("Failed to send data to server.");
        return false;
    }

    Command cmd = UnknownError;
    if (!receiveBlock(&cmd, &block)) {
        errMsg.prepend(QLS("Failed to get response from server. "));
        return false;
    }

    if (cmd == Abort) {
        errMsg += QLS("Server rejected connection. Reason: ") + QString::fromLatin1(block);
        return false;
    }

    if (dryrun)
        *dryrun = (cmd == DoDryRun);

    if (cmd != Ack && cmd != DoDryRun) {
        errMsg += QLS("Unexpected response from server.");
        return false;
    }

    return true;
}


bool BaselineProtocol::acceptConnection(PlatformInfo *pi)
{
    errMsg.clear();

    QByteArray block;
    Command cmd = AcceptPlatformInfo;
    if (!receiveBlock(&cmd, &block) || cmd != AcceptPlatformInfo)
        return false;

    if (pi) {
        QDataStream ds(block);
        ds >> *pi;
        pi->insert(PI_HostAddress, socket.peerAddress().toString());
    }

    return true;
}


bool BaselineProtocol::requestBaselineChecksums(const QString &testFunction, ImageItemList *itemList)
{
    errMsg.clear();
    if (!itemList)
        return false;

    for (ImageItemList::iterator it = itemList->begin(); it != itemList->end(); it++)
        it->testFunction = testFunction;

    QByteArray block;
    QDataStream ds(&block, QIODevice::WriteOnly);
    ds << *itemList;
    if (!sendBlock(RequestBaselineChecksums, block))
        return false;

    Command cmd;
    QByteArray rcvBlock;
    if (!receiveBlock(&cmd, &rcvBlock) || cmd != BaselineProtocol::Ack)
        return false;
    QDataStream rds(&rcvBlock, QIODevice::ReadOnly);
    rds >> *itemList;
    return true;
}


bool BaselineProtocol::submitMatch(const ImageItem &item, QByteArray *serverMsg)
{
    Command cmd;
    ImageItem smallItem = item;
    smallItem.image = QImage();  // No need to waste bandwidth sending image (identical to baseline) to server
    return (sendItem(AcceptMatch, smallItem) && receiveBlock(&cmd, serverMsg) && cmd == Ack);
}


bool BaselineProtocol::submitNewBaseline(const ImageItem &item, QByteArray *serverMsg)
{
    Command cmd;
    return (sendItem(AcceptNewBaseline, item) && receiveBlock(&cmd, serverMsg) && cmd == Ack);
}


bool BaselineProtocol::submitMismatch(const ImageItem &item, QByteArray *serverMsg, bool *fuzzyMatch)
{
    Command cmd;
    if (sendItem(AcceptMismatch, item) && receiveBlock(&cmd, serverMsg) && (cmd == Ack || cmd == FuzzyMatch)) {
        if (fuzzyMatch)
            *fuzzyMatch = (cmd == FuzzyMatch);
        return true;
    }
    return false;
}


bool BaselineProtocol::sendItem(Command cmd, const ImageItem &item)
{
    errMsg.clear();
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    QDataStream ds(&buf);
    ds << item;
    if (!sendBlock(cmd, buf.data())) {
        errMsg.prepend(QLS("Failed to submit image to server. "));
        return false;
    }
    return true;
}


bool BaselineProtocol::sendBlock(Command cmd, const QByteArray &block)
{
    QDataStream s(&socket);
    // TBD: set qds version as a constant
    s << quint16(ProtocolVersion) << quint16(cmd);
    s.writeBytes(block.constData(), block.size());
    return true;
}


bool BaselineProtocol::receiveBlock(Command *cmd, QByteArray *block)
{
    while (socket.bytesAvailable() < int(2*sizeof(quint16) + sizeof(quint32))) {
        if (!socket.waitForReadyRead(Timeout))
            return false;
    }
    QDataStream ds(&socket);
    quint16 rcvProtocolVersion, rcvCmd;
    ds >> rcvProtocolVersion >> rcvCmd;
    if (rcvProtocolVersion != ProtocolVersion) {
        errMsg = QLS("Baseline protocol version mismatch, received:") + QString::number(rcvProtocolVersion)
                + QLS(" expected:") + QString::number(ProtocolVersion);
        return false;
    }
    if (cmd)
        *cmd = Command(rcvCmd);

    QByteArray uMsg;
    quint32 remaining;
    ds >> remaining;
    uMsg.resize(remaining);
    int got = 0;
    char* uMsgBuf = uMsg.data();
    do {
        got = ds.readRawData(uMsgBuf, remaining);
        remaining -= got;
        uMsgBuf += got;
    } while (remaining && got >= 0 && socket.waitForReadyRead(Timeout));

    if (got < 0)
        return false;

    if (block)
        *block = uMsg;

    return true;
}


QString BaselineProtocol::errorMessage()
{
    QString ret = errMsg;
    if (socket.error() >= 0)
        ret += QLS(" Socket state: ") + socket.errorString();
    return ret;
}

