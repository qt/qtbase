/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "baselineprotocol.h"
#include <QLibraryInfo>
#include <QImage>
#include <QBuffer>
#include <QHostInfo>
#include <QSysInfo>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QTime>
#include <QPointer>

const QString PI_Project(QLS("Project"));
const QString PI_TestCase(QLS("TestCase"));
const QString PI_HostName(QLS("HostName"));
const QString PI_HostAddress(QLS("HostAddress"));
const QString PI_OSName(QLS("OSName"));
const QString PI_OSVersion(QLS("OSVersion"));
const QString PI_QtVersion(QLS("QtVersion"));
const QString PI_QtBuildMode(QLS("QtBuildMode"));
const QString PI_GitCommit(QLS("GitCommit"));
const QString PI_QMakeSpec(QLS("QMakeSpec"));
const QString PI_PulseGitBranch(QLS("PulseGitBranch"));
const QString PI_PulseTestrBranch(QLS("PulseTestrBranch"));

#ifndef QMAKESPEC
#define QMAKESPEC "Unknown"
#endif

#if defined(Q_OS_WIN)
#include <QtCore/qt_windows.h>
#endif
#if defined(Q_OS_UNIX)
#include <time.h>
#endif
void BaselineProtocol::sysSleep(int ms)
{
#if defined(Q_OS_WIN)
#  ifndef Q_OS_WINRT
    Sleep(DWORD(ms));
#  else
    WaitForSingleObjectEx(GetCurrentThread(), ms, false);
#  endif
#else
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#endif
}

PlatformInfo::PlatformInfo()
    : QMap<QString, QString>(), adHoc(true)
{
}

PlatformInfo PlatformInfo::localHostInfo()
{
    PlatformInfo pi;
    pi.insert(PI_HostName, QHostInfo::localHostName());
    pi.insert(PI_QtVersion, QLS(qVersion()));
    pi.insert(PI_QMakeSpec, QString(QLS(QMAKESPEC)).remove(QRegExp(QLS("^.*mkspecs/"))));
#if QT_VERSION >= 0x050000
    pi.insert(PI_QtBuildMode, QLibraryInfo::isDebugBuild() ? QLS("QtDebug") : QLS("QtRelease"));
#endif
#if defined(Q_OS_LINUX)
    pi.insert(PI_OSName, QLS("Linux"));
    QProcess uname;
    uname.start(QLS("uname"), QStringList() << QLS("-r"));
    if (uname.waitForFinished(3000))
        pi.insert(PI_OSVersion, QString::fromLocal8Bit(uname.readAllStandardOutput().constData()).simplified());
#elif defined(Q_OS_WINCE)
    pi.insert(PI_OSName, QLS("WinCE"));
    pi.insert(PI_OSVersion, QString::number(QSysInfo::windowsVersion()));
#elif defined(Q_OS_WIN)
    pi.insert(PI_OSName, QLS("Windows"));
    pi.insert(PI_OSVersion, QString::number(QSysInfo::windowsVersion()));
#elif defined(Q_OS_MAC)
    pi.insert(PI_OSName, QLS("MacOS"));
    pi.insert(PI_OSVersion, QString::number(QSysInfo::macVersion()));
#else
    pi.insert(PI_OSName, QLS("Other"));
#endif

#ifndef QT_NO_PROCESS
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
        pi.insert(PI_GitCommit, QString::fromLocal8Bit(git.readAllStandardOutput().constData()).simplified());
    else
        pi.insert(PI_GitCommit, QLS("Unknown"));

    QByteArray gb = qgetenv("PULSE_GIT_BRANCH");
    if (!gb.isEmpty()) {
        pi.insert(PI_PulseGitBranch, QString::fromLatin1(gb));
        pi.setAdHocRun(false);
    }
    QByteArray tb = qgetenv("PULSE_TESTR_BRANCH");
    if (!tb.isEmpty()) {
        pi.insert(PI_PulseTestrBranch, QString::fromLatin1(tb));
        pi.setAdHocRun(false);
    }
    if (!qgetenv("JENKINS_HOME").isEmpty()) {
        pi.setAdHocRun(false);
        gb = qgetenv("GIT_BRANCH");
        if (!gb.isEmpty()) {
            // FIXME: the string "Pulse" should be eliminated, since that is not the used tool.
            pi.insert(PI_PulseGitBranch, QString::fromLatin1(gb));
        }
    }
#endif // !QT_NO_PROCESS

    return pi;
}


PlatformInfo::PlatformInfo(const PlatformInfo &other)
    : QMap<QString, QString>(other)
{
    orides = other.orides;
    adHoc = other.adHoc;
}


PlatformInfo &PlatformInfo::operator=(const PlatformInfo &other)
{
    QMap<QString, QString>::operator=(other);
    orides = other.orides;
    adHoc = other.adHoc;
    return *this;
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


ImageItem &ImageItem::operator=(const ImageItem &other)
{
    testFunction = other.testFunction;
    itemName = other.itemName;
    itemChecksum = other.itemChecksum;
    status = other.status;
    image = other.image;
    imageChecksums = other.imageChecksums;
    return *this;
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
    const int bpl = img.bytesPerLine();
    const int padBytes = bpl - (img.width() * img.depth() / 8);
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
    hashword2((const quint32 *)img.constBits(), img.byteCount()/4, &h1, &h2);
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
    out << qCompress((const uchar *)image.constBits(), image.byteCount());
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
        sysSleep(3000);  // Wait a bit and try again, the server might just be restarting
        if (!socket.waitForConnected(Timeout)) {
            errMsg += QLS("TCP connectToHost failed. Host:") + serverName + QLS(" port:") + QString::number(ServerPort);
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

    for(ImageItemList::iterator it = itemList->begin(); it != itemList->end(); it++)
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
    smallItem.image = QImage();  // No need to waste bandwith sending image (identical to baseline) to server
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

