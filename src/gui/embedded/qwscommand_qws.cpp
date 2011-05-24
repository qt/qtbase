/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwscommand_qws_p.h"
#include "qtransportauth_qws.h"
#include "qtransportauth_qws_p.h"

#include <unistd.h>

// #define QWSCOMMAND_DEBUG 1 // Uncomment to debug client/server communication

#ifdef QWSCOMMAND_DEBUG
# include <qdebug.h>
# include "qfile.h"
# include <ctype.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef QWSCOMMAND_DEBUG
// QWSHexDump -[ start ]---------------------------------------------
# define QWSHEXDUMP_MAX 32
class QWSHexDump
{
public:

    QWSHexDump(const void *address, int len, int wrapAt = 16)
        : wrap(wrapAt), dataSize(len)
    {
        init();
        data = reinterpret_cast<const char*>(address);
        if (len < 0)
            dataSize = 0;
    }

    QWSHexDump(const char *str, int len = -1, int wrapAt = 16)
        : wrap(wrapAt), dataSize(len)
    {
        init();
        data = str;
        if (len == -1)
            dataSize = str ? strlen(str) : 0;
    }

    QWSHexDump(const QByteArray &array, int wrapAt = 16)
        : wrap(wrapAt)
    {
        init();
        data = array.data();
        dataSize = array.size();
    }

    // Sets a customized prefix for the hexdump
    void setPrefix(const char *str) { prefix = str; }

    // Sets number of bytes to cluster together
    void setClusterSize(uint num) { clustering = num; }

    // Output hexdump to a text stream
    void intoTextStream(QTextStream &strm) {
        outstrm = &strm;
        hexDump();
    }

    // Output hexdump to a QString
    QString toString();

protected:
    void init();
    void hexDump();
    void sideviewDump(int at);

private:
    uint wrap;
    uint clustering;
    uint dataSize;
    int dataWidth;
    const char *data;
    const char *prefix;
    bool dirty;

    char sideviewLayout[QWSHEXDUMP_MAX + 1];
    char sideview[15];

    QTextStream *outstrm;
};

void QWSHexDump::init()
{
    prefix = "> ";             // Standard line prefix
    clustering = 2;            // Word-size clustering by default
    if (wrap > QWSHEXDUMP_MAX) // No wider than QWSHexDump_MAX bytes
        wrap = QWSHEXDUMP_MAX;
}

void QWSHexDump::hexDump()
{
    *outstrm << '(' << dataSize << " bytes):\n" << prefix;
    sprintf(sideviewLayout, " [%%-%us]", wrap);
    dataWidth = (2 * wrap) + (wrap / clustering);

    dirty = false;
    uint wrapIndex = 0;
    for (uint i = 0; i < dataSize; i++) {
        uint c = static_cast<uchar>(data[i]);
        sideview[wrapIndex = i%wrap] = isprint(c) ? c : '.';

        if (wrapIndex && (wrapIndex % clustering == 0))
            *outstrm << ' ';

        outstrm->setFieldWidth(2);
        outstrm->setPadChar('0');
        outstrm->setNumberFlags( QTextStream::ShowBase );
        *outstrm << hex << c;
        dirty = true;

        if (wrapIndex == wrap-1) {
            sideviewDump(wrapIndex);
            wrapIndex = 0;
            if (i+1 < dataSize)
                *outstrm << endl << prefix;
        }

    }
    sideviewDump(wrapIndex);
}

void QWSHexDump::sideviewDump(int at)
{
    if (dirty) {
        dirty = false;
        ++at;
        sideview[at] = '\0';
        int currentWidth = (2 * at) + (at / clustering) - (at%clustering?0:1);
        int missing = qMax(dataWidth - currentWidth, 0);
        while (missing--)
            *outstrm << ' ';

        *outstrm << " [";
        outstrm->setPadChar(' ');
        outstrm->setFieldWidth(wrap);
        outstrm->setFieldAlignment( QTextStream::AlignLeft );
        *outstrm << sideview;
        *outstrm << ']';
    }
}

// Output hexdump to a QString
QString QWSHexDump::toString() {
    QString result;
    QTextStream strm(&result, QFile::WriteOnly);
    outstrm = &strm;
    hexDump();
    return result;
}

#ifndef QT_NO_DEBUG
QDebug &operator<<(QDebug &dbg, QWSHexDump *hd) {
    if (!hd)
        return dbg << "QWSHexDump(0x0)";
    QString result = hd->toString();
    dbg.nospace() << result;
    return dbg.space();
}

// GCC & Intel wont handle references here
QDebug operator<<(QDebug dbg, QWSHexDump hd) {
    return dbg << &hd;
}
#endif
// QWSHexDump -[ end ]-----------------------------------------------


QDebug &operator<<(QDebug &dbg, QWSCommand::Type tp)
{
    dbg << qws_getCommandTypeString( tp );
    return dbg;
}

#define N_EVENTS 19
const char * eventNames[N_EVENTS] =  {
        "NoEvent",
        "Connected",
        "Mouse", "Focus", "Key",
        "Region",
        "Creation",
        "PropertyNotify",
        "PropertyReply",
        "SelectionClear",
        "SelectionRequest",
        "SelectionNotify",
        "MaxWindowRect",
        "QCopMessage",
        "WindowOperation",
        "IMEvent",
        "IMQuery",
        "IMInit",
        "Font"
    };

class QWSServer;
extern QWSServer *qwsServer;
#endif

const char *qws_getCommandTypeString( QWSCommand::Type tp )
{
    const char *typeStr;
    switch(tp) {
        case QWSCommand::Create:
            typeStr = "Create";
            break;
        case QWSCommand::Shutdown:
            typeStr = "Shutdown";
            break;
        case QWSCommand::Region:
            typeStr = "Region";
            break;
        case QWSCommand::RegionMove:
            typeStr = "RegionMove";
            break;
        case QWSCommand::RegionDestroy:
            typeStr = "RegionDestroy";
            break;
        case QWSCommand::SetProperty:
            typeStr = "SetProperty";
            break;
        case QWSCommand::AddProperty:
            typeStr = "AddProperty";
            break;
        case QWSCommand::RemoveProperty:
            typeStr = "RemoveProperty";
            break;
        case QWSCommand::GetProperty:
            typeStr = "GetProperty";
            break;
        case QWSCommand::SetSelectionOwner:
            typeStr = "SetSelectionOwner";
            break;
        case QWSCommand::ConvertSelection:
            typeStr = "ConvertSelection";
            break;
        case QWSCommand::RequestFocus:
            typeStr = "RequestFocus";
            break;
        case QWSCommand::ChangeAltitude:
            typeStr = "ChangeAltitude";
            break;
        case QWSCommand::SetOpacity:
            typeStr = "SetOpacity";
            break;
        case QWSCommand::DefineCursor:
            typeStr = "DefineCursor";
            break;
        case QWSCommand::SelectCursor:
            typeStr = "SelectCursor";
            break;
        case QWSCommand::PositionCursor:
            typeStr = "PositionCursor";
            break;
        case QWSCommand::GrabMouse:
            typeStr = "GrabMouse";
            break;
        case QWSCommand::PlaySound:
            typeStr = "PlaySound";
            break;
        case QWSCommand::QCopRegisterChannel:
            typeStr = "QCopRegisterChannel";
            break;
        case QWSCommand::QCopSend:
            typeStr = "QCopSend";
            break;
        case QWSCommand::RegionName:
            typeStr = "RegionName";
            break;
        case QWSCommand::Identify:
            typeStr = "Identify";
            break;
        case QWSCommand::GrabKeyboard:
            typeStr = "GrabKeyboard";
            break;
        case QWSCommand::RepaintRegion:
            typeStr = "RepaintRegion";
            break;
        case QWSCommand::IMMouse:
            typeStr = "IMMouse";
            break;
        case QWSCommand::IMUpdate:
            typeStr = "IMUpdate";
            break;
        case QWSCommand::IMResponse:
            typeStr = "IMResponse";
            break;
        case QWSCommand::Font:
            typeStr = "Font";
            break;
        case QWSCommand::Unknown:
        default:
            typeStr = "Unknown";
            break;
    }
    return typeStr;
}


/*********************************************************************
 *
 * Functions to read/write commands on/from a socket
 *
 *********************************************************************/

#ifndef QT_NO_QWS_MULTIPROCESS
void qws_write_command(QIODevice *socket, int type, char *simpleData, int simpleLen,
                       char *rawData, int rawLen)
{
#ifdef QWSCOMMAND_DEBUG
    if (simpleLen) qDebug() << "WRITE simpleData " << QWSHexDump(simpleData, simpleLen);
    if (rawLen > 0) qDebug() << "WRITE rawData " << QWSHexDump(rawData, rawLen);
#endif

#ifndef QT_NO_SXE
    QTransportAuth *a = QTransportAuth::getInstance();
    // ###### as soon as public API can be modified get rid of horrible casts
    QIODevice *ad = a->passThroughByClient(reinterpret_cast<QWSClient*>(socket));
    if (ad)
        socket = ad;
#endif

    qws_write_uint(socket, type);

    if (rawLen > MAX_COMMAND_SIZE) {
        qWarning("qws_write_command: Message of size %d too big. "
                 "Truncated to %d", rawLen, MAX_COMMAND_SIZE);
        rawLen = MAX_COMMAND_SIZE;
    }

    qws_write_uint(socket, rawLen == -1 ? 0 : rawLen);

    if (simpleData && simpleLen)
        socket->write(simpleData, simpleLen);

    if (rawLen && rawData)
        socket->write(rawData, rawLen);
}

/*
  command format: [type][rawLen][simpleData][rawData]
  type is already read when entering this function
*/

bool qws_read_command(QIODevice *socket, char *&simpleData, int &simpleLen,
                      char *&rawData, int &rawLen, int &bytesRead)
{

    // read rawLen
    if (rawLen == -1) {
        rawLen = qws_read_uint(socket);
        if (rawLen == -1)
            return false;
    }

    // read simpleData, assumes socket is capable of buffering all the data
    if (simpleLen && !rawData) {
        if (socket->bytesAvailable() < uint(simpleLen))
            return false;
        int tmp = socket->read(simpleData, simpleLen);
        Q_ASSERT(tmp == simpleLen);
        Q_UNUSED(tmp);
    }

    if (rawLen > MAX_COMMAND_SIZE) {
        socket->close();
        qWarning("qws_read_command: Won't read command of length %d, "
                 "connection closed.", rawLen);
        return false;
    }

    // read rawData
    if (rawLen && !rawData) {
        rawData = new char[rawLen];
        bytesRead = 0;
    }
    if (bytesRead < rawLen && socket->bytesAvailable())
        bytesRead += socket->read(rawData + bytesRead, rawLen - bytesRead);

    return (bytesRead == rawLen);
}
#endif

/*********************************************************************
 *
 * QWSCommand base class - only use derived classes from that
 *
 *********************************************************************/
QWSProtocolItem::~QWSProtocolItem() {
    if (deleteRaw)
        delete []rawDataPtr;
}

#ifndef QT_NO_QWS_MULTIPROCESS
void QWSProtocolItem::write(QIODevice *s) {
#ifdef QWSCOMMAND_DEBUG
    if (!qwsServer)
        qDebug() << "QWSProtocolItem::write sending type " << static_cast<QWSCommand::Type>(type);
    else
        qDebug() << "QWSProtocolItem::write sending event " << (type < N_EVENTS ? eventNames[type] : "unknown");
#endif
    qws_write_command(s, type, simpleDataPtr, simpleLen, rawDataPtr, rawLen);
}

bool QWSProtocolItem::read(QIODevice *s) {
#ifdef QWSCOMMAND_DEBUG
    QLatin1String reread( (rawLen == -1) ? "" : "REREAD");
    if (qwsServer)
        qDebug() << "QWSProtocolItem::read reading type " << static_cast<QWSCommand::Type>(type) << reread;
    else
        qDebug() << "QWSProtocolItem::read reading event " << (type < N_EVENTS ? eventNames[type] : "unknown") << reread;
    //qDebug("QWSProtocolItem::read reading event %s", type < N_EVENTS ? eventNames[type] : "unknown");
#endif
    bool b = qws_read_command(s, simpleDataPtr, simpleLen, rawDataPtr, rawLen, bytesRead);
    if (b) {
        setData(rawDataPtr, rawLen, false);
        deleteRaw = true;
    }
#ifdef QWSCOMMAND_DEBUG
    else
    {
        qDebug() << "error in reading command " << static_cast<QWSCommand::Type>(type);
    }
#endif
    return b;
}
#endif // QT_NO_QWS_MULTIPROCESS

void QWSProtocolItem::copyFrom(const QWSProtocolItem *item) {
    if (this == item)
        return;
    simpleLen = item->simpleLen;
    memcpy(simpleDataPtr, item->simpleDataPtr, simpleLen);
    setData(item->rawDataPtr, item->rawLen);
}

void QWSProtocolItem::setData(const char *data, int len, bool allocateMem) {
    if (deleteRaw)
        delete [] rawDataPtr;
    if (!data || len <= 0) {
        rawDataPtr = 0;
        rawLen = 0;
        return;
    }
    if (allocateMem) {
        rawDataPtr = new char[len];
        memcpy(rawDataPtr, data, len);
        deleteRaw = true;
    } else {
        rawDataPtr = const_cast<char *>(data);
        deleteRaw = false;
    }
    rawLen = len;
}

QWSCommand *QWSCommand::factory(int type)
{
    QWSCommand *command = 0;
    switch (type) {
    case QWSCommand::Create:
        command = new QWSCreateCommand;
        break;
    case QWSCommand::Shutdown:
        command = new QWSCommand(type, 0, 0);
        break;
    case QWSCommand::Region:
        command = new QWSRegionCommand;
        break;
    case QWSCommand::RegionMove:
        command = new QWSRegionMoveCommand;
        break;
    case QWSCommand::RegionDestroy:
        command = new QWSRegionDestroyCommand;
        break;
    case QWSCommand::AddProperty:
        command = new QWSAddPropertyCommand;
        break;
    case QWSCommand::SetProperty:
        command = new QWSSetPropertyCommand;
        break;
    case QWSCommand::RemoveProperty:
        command = new QWSRemovePropertyCommand;
        break;
    case QWSCommand::GetProperty:
        command = new QWSGetPropertyCommand;
        break;
    case QWSCommand::SetSelectionOwner:
        command = new QWSSetSelectionOwnerCommand;
        break;
    case QWSCommand::RequestFocus:
        command = new QWSRequestFocusCommand;
        break;
    case QWSCommand::ChangeAltitude:
        command = new QWSChangeAltitudeCommand;
        break;
    case QWSCommand::SetOpacity:
        command = new QWSSetOpacityCommand;
        break;
    case QWSCommand::DefineCursor:
        command = new QWSDefineCursorCommand;
        break;
    case QWSCommand::SelectCursor:
        command = new QWSSelectCursorCommand;
        break;
    case QWSCommand::GrabMouse:
        command = new QWSGrabMouseCommand;
        break;
    case QWSCommand::GrabKeyboard:
        command = new QWSGrabKeyboardCommand;
        break;
#ifndef QT_NO_SOUND
    case QWSCommand::PlaySound:
        command = new QWSPlaySoundCommand;
        break;
#endif
#ifndef QT_NO_COP
    case QWSCommand::QCopRegisterChannel:
        command = new QWSQCopRegisterChannelCommand;
        break;
    case QWSCommand::QCopSend:
        command = new QWSQCopSendCommand;
        break;
#endif
    case QWSCommand::RegionName:
        command = new QWSRegionNameCommand;
        break;
    case QWSCommand::Identify:
        command = new QWSIdentifyCommand;
        break;
    case QWSCommand::RepaintRegion:
        command = new QWSRepaintRegionCommand;
        break;
#ifndef QT_NO_QWS_INPUTMETHODS
    case QWSCommand::IMUpdate:
        command = new QWSIMUpdateCommand;
        break;

    case QWSCommand::IMMouse:
        command = new QWSIMMouseCommand;
        break;

    case QWSCommand::IMResponse:
        command = new QWSIMResponseCommand;
        break;
#endif
    case QWSCommand::PositionCursor:
        command = new QWSPositionCursorCommand;
        break;
#ifndef QT_NO_QWSEMBEDWIDGET
    case QWSCommand::Embed:
        command = new QWSEmbedCommand;
        break;
#endif
    case QWSCommand::Font:
        command = new QWSFontCommand;
        break;
    case QWSCommand::ScreenTransform:
        command = new QWSScreenTransformCommand;
        break;
    default:
        qWarning("QWSCommand::factory : Type error - got %08x!", type);
    }
    return command;
}

QT_END_NAMESPACE
