/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWSCOMMAND_QWS_P_H
#define QWSCOMMAND_QWS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// When reading commands "off the wire" in the server, the rawLen is read
// and then that many bytes are allocated.  If the rawLen is corrupted (or
// the protocol is being attacked) too many bytes can be allocated.  Set
// a hard limit here for security.
#define MAX_COMMAND_SIZE (16 * 1024)

#include <QtCore/qbytearray.h>
#include <QtGui/qwsutils_qws.h>
#include <QtGui/qfont.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qvariant.h>
#include <QtCore/qrect.h>
#include <QtGui/qregion.h>
#include <QtCore/qvector.h>
#include <QtCore/qvarlengtharray.h>
#include <QtGui/qwsevent_qws.h>
#include "qwsprotocolitem_qws.h"

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QRect;

/*********************************************************************
 *
 * Functions to read/write commands on/from a socket
 *
 *********************************************************************/
#ifndef QT_NO_QWS_MULTIPROCESS
void qws_write_command(QIODevice *socket, int type, char *simpleData, int simpleLen, char *rawData, int rawLen);
bool qws_read_command(QIODevice *socket, char *&simpleData, int &simpleLen, char *&rawData, int &rawLen, int &bytesRead);
#endif

struct QWSCommand : QWSProtocolItem
{
    QWSCommand(int t, int len, char *ptr) : QWSProtocolItem(t,len,ptr) {}

    enum Type {
        Unknown = 0,
        Create,
        Shutdown,
        Region,
        RegionMove,
        RegionDestroy,
        SetProperty,
        AddProperty,
        RemoveProperty,
        GetProperty,
        SetSelectionOwner,
        ConvertSelection,
        RequestFocus,
        ChangeAltitude,
        SetOpacity,
        DefineCursor,
        SelectCursor,
        PositionCursor,
        GrabMouse,
        PlaySound,
        QCopRegisterChannel,
        QCopSend,
        RegionName,
        Identify,
        GrabKeyboard,
        RepaintRegion,
        IMMouse,
        IMUpdate,
        IMResponse,
        Embed,
        Font,
        ScreenTransform
    };
    static QWSCommand *factory(int type);
};

const char *qws_getCommandTypeString( QWSCommand::Type tp );

#ifndef QT_NO_DEBUG
class QDebug;
QDebug &operator<<(QDebug &dbg, QWSCommand::Type tp);
#endif // QT_NO_DEBUG

/*********************************************************************
 *
 * Commands
 *
 *********************************************************************/

struct QWSIdentifyCommand : public QWSCommand
{
    QWSIdentifyCommand() :
        QWSCommand(QWSCommand::Identify,
                   sizeof(simpleData), reinterpret_cast<char *>(&simpleData))
    {
        simpleData.idLen = 0;
        simpleData.idLock = -1;
    }

    void setData(const char *d, int len, bool allocateMem) {
        QWSCommand::setData(d, len, allocateMem);
        if ( simpleData.idLen > MAX_COMMAND_SIZE )
        {
            qWarning( "Identify command - name length %d - too big!", simpleData.idLen );
            simpleData.idLen = MAX_COMMAND_SIZE;
        }
        if ( simpleData.idLen * int(sizeof(QChar)) > len )
        {
            qWarning( "Identify command - name length %d - buffer size %d - buffer overrun!", simpleData.idLen, len );
        }
        else
        {
            id = QString(reinterpret_cast<const QChar*>(d), simpleData.idLen);
        }
    }

    void setId(const QString& i, int lock)
    {
        id = i;
        simpleData.idLen = id.length();
        simpleData.idLock = lock;
        setData(reinterpret_cast<const char*>(id.unicode()), simpleData.idLen*2, true);
    }

    struct SimpleData {
        int idLen;
        int idLock;
    } simpleData;
    QString id;
};

struct QWSCreateCommand : public QWSCommand
{
    QWSCreateCommand(int n = 1) :
        QWSCommand(QWSCommand::Create, sizeof(count),
                   reinterpret_cast<char *>(&count)), count(n) {}
    int count;
};

struct QWSRegionNameCommand : public QWSCommand
{
    QWSRegionNameCommand() :
        QWSCommand(QWSCommand::RegionName,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem) {
        QWSCommand::setData(d, len, allocateMem);
        if ( simpleData.nameLen > MAX_COMMAND_SIZE )
        {
            qWarning( "region name command - name length too big!" );
            simpleData.nameLen = MAX_COMMAND_SIZE;
        }
        if ( simpleData.captionLen > MAX_COMMAND_SIZE )
        {
            qWarning( "region name command - caption length too big!" );
            simpleData.captionLen = MAX_COMMAND_SIZE;
        }
        if ( simpleData.nameLen + simpleData.captionLen > len )
        {
            qWarning( "region name command - name length %d - caption length %d - buffer size %d - buffer overrun!",
                      simpleData.nameLen, simpleData.captionLen, len );

        }
        else
        {
            name = QString(reinterpret_cast<const QChar*>(d), simpleData.nameLen/2);
            d += simpleData.nameLen;
            caption = QString(reinterpret_cast<const QChar*>(d), simpleData.captionLen/2);
        }
    }

    void setName(const QString& n, const QString &c)
    {
        name = n;
        caption = c;
        int l = simpleData.nameLen = name.length()*2;
        l += simpleData.captionLen = caption.length()*2;
        char *d = new char[l];
        memcpy(d, name.unicode(), simpleData.nameLen);
        memcpy(d+simpleData.nameLen, caption.unicode(), simpleData.captionLen);
        setData(d, l, true);
        delete[] d;
    }

    struct SimpleData {
        int windowid;
        int nameLen;
        int captionLen;
    } simpleData;
    QString name;
    QString caption;
};

struct QWSRegionCommand : public QWSCommand
{
    QWSRegionCommand() :
        QWSCommand(QWSCommand::Region, sizeof(simpleData),
                    reinterpret_cast<char*>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSCommand::setData(d, len, allocateMem);

        if( simpleData.nrectangles * int(sizeof(QRect)) + simpleData.surfacekeylength * int(sizeof(QChar)) + simpleData.surfacedatalength * int(sizeof(char)) > len )
        {
            qWarning( "region command - rectangle count %d - surface key length %d - region data size %d - buffer size %d - buffer overrun!",
                      simpleData.nrectangles, simpleData.surfacekeylength, simpleData.surfacedatalength, len );
        }
        else
        {
            char *ptr = rawDataPtr;

            region.setRects(reinterpret_cast<QRect*>(ptr), simpleData.nrectangles);
            ptr += simpleData.nrectangles * sizeof(QRect);

            surfaceKey = QString(reinterpret_cast<QChar*>(ptr),
                             simpleData.surfacekeylength);
            ptr += simpleData.surfacekeylength * sizeof(QChar);

            surfaceData = QByteArray(ptr, simpleData.surfacedatalength);
        }
    }

    void setData(int id, const QString &key, const QByteArray &data,
                 const QRegion &reg)
    {
        surfaceKey = key;
        surfaceData = data;
        region = reg;

        const QVector<QRect> rects = reg.rects();

        simpleData.windowid = id;
        simpleData.surfacekeylength = key.size();
        simpleData.surfacedatalength = data.size();
        simpleData.nrectangles = rects.count();

        QVarLengthArray<char, 256> buffer;
        buffer.append(reinterpret_cast<const char*>(rects.constData()),
                      rects.count() * sizeof(QRect));
        buffer.append(reinterpret_cast<const char*>(key.constData()),
                      key.size() * sizeof(QChar));
        buffer.append(data, data.size());

        QWSCommand::setData(buffer.constData(), buffer.size(), true);
    }

    /* XXX this will pad out in a compiler dependent way,
       should move nrectangles to before windowtype, and
       add reserved bytes.
       Symptom will be valgrind reported uninitialized memory usage
       */
    struct SimpleData {
        int windowid;
        int surfacekeylength;
        int surfacedatalength;
        int nrectangles;
    } simpleData;

    QString surfaceKey;
    QByteArray surfaceData;
    QRegion region;
};

struct QWSSetOpacityCommand : public QWSCommand
{
    QWSSetOpacityCommand() :
        QWSCommand(QWSCommand::SetOpacity, sizeof(simpleData),
                    reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int windowid;
        uchar opacity;
    } simpleData;
};

struct QWSRegionMoveCommand : public QWSCommand
{
    QWSRegionMoveCommand() :
        QWSCommand(QWSCommand::RegionMove, sizeof(simpleData),
                    reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int windowid;
        int dx;
        int dy;
    } simpleData;

};

struct QWSRegionDestroyCommand : public QWSCommand
{
    QWSRegionDestroyCommand() :
        QWSCommand(QWSCommand::RegionDestroy, sizeof(simpleData),
                    reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int windowid;
    } simpleData;

};

struct QWSRequestFocusCommand : public QWSCommand
{
    QWSRequestFocusCommand() :
        QWSCommand(QWSCommand::RequestFocus, sizeof(simpleData), reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int windowid;
        int flag;
    } simpleData;
};

struct QWSChangeAltitudeCommand : public QWSCommand
{
    QWSChangeAltitudeCommand() :
        QWSCommand(QWSCommand::ChangeAltitude, sizeof(simpleData), reinterpret_cast<char*>(&simpleData)) {}

    enum Altitude {
        Lower = -1,
        Raise = 0,
        StaysOnTop = 1
    };

    struct SimpleData {
        int windowid;
        Altitude altitude;
        bool fixed;
    } simpleData;

};


struct QWSAddPropertyCommand : public QWSCommand
{
    QWSAddPropertyCommand() :
        QWSCommand(QWSCommand::AddProperty, sizeof(simpleData), reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int windowid, property;
    } simpleData;

};

struct QWSSetPropertyCommand : public QWSCommand
{
    QWSSetPropertyCommand() :
        QWSCommand(QWSCommand::SetProperty, sizeof(simpleData),
                    reinterpret_cast<char*>(&simpleData)) { data = 0; }

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSCommand::setData(d, len, allocateMem);
        data = rawDataPtr;
    }

    struct SimpleData {
        int windowid, property, mode;
    } simpleData;

    char *data;
};

struct QWSRepaintRegionCommand : public QWSCommand
{
    QWSRepaintRegionCommand() :
        QWSCommand(QWSCommand::RepaintRegion, sizeof(simpleData),
                    reinterpret_cast<char*>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSCommand::setData(d, len, allocateMem);

        if( simpleData.nrectangles * int(sizeof(QRect)) > len )
        {
            qWarning( "repaint region command - region rectangle count %d - buffer size %d - buffer overrun",
                      simpleData.nrectangles, len );

            simpleData.nrectangles = len / sizeof(QRect);
        }
        rectangles = reinterpret_cast<QRect *>(rawDataPtr);
    }

    struct SimpleData {
        int windowid;
        int windowFlags;
        bool opaque;
        int nrectangles;
    } simpleData;

    QRect * rectangles;

};

struct QWSRemovePropertyCommand : public QWSCommand
{
    QWSRemovePropertyCommand() :
        QWSCommand(QWSCommand::RemoveProperty, sizeof(simpleData), reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int windowid, property;
    } simpleData;

};

struct QWSGetPropertyCommand : public QWSCommand
{
    QWSGetPropertyCommand() :
        QWSCommand(QWSCommand::GetProperty, sizeof(simpleData), reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int windowid, property;
    } simpleData;

};

struct QWSSetSelectionOwnerCommand : public QWSCommand
{
    QWSSetSelectionOwnerCommand() :
        QWSCommand(QWSCommand::SetSelectionOwner,
                    sizeof(simpleData), reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int windowid;
        int hour, minute, sec, ms; // time
    } simpleData;

};

struct QWSConvertSelectionCommand : public QWSCommand
{
    QWSConvertSelectionCommand() :
        QWSCommand(QWSCommand::ConvertSelection,
                    sizeof(simpleData), reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int requestor; // requestor window of the selection
        int selection; // property on requestor into which the selection should be stored
        int mimeTypes; // property ion requestor in which the mimetypes, in which the selection may be, are stored
    } simpleData;

};

struct QWSDefineCursorCommand : public QWSCommand
{
    QWSDefineCursorCommand() :
        QWSCommand(QWSCommand::DefineCursor,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSCommand::setData(d, len, allocateMem);
        data = reinterpret_cast<unsigned char *>(rawDataPtr);
        if (simpleData.height * ((simpleData.width+7) / 8) > len) {
            qWarning("define cursor command - width %d height %d- buffer size %d - buffer overrun",
                     simpleData.width, simpleData.height, len );
            simpleData.width = simpleData.height = 0;
        }
    }

    struct SimpleData {
        int width;
        int height;
        int hotX;
        int hotY;
        int id;
    } simpleData;

    unsigned char *data;
};

struct QWSSelectCursorCommand : public QWSCommand
{
    QWSSelectCursorCommand() :
        QWSCommand(QWSCommand::SelectCursor,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    struct SimpleData {
        int windowid;
        int id;
    } simpleData;
};

struct QWSPositionCursorCommand : public QWSCommand
{
    QWSPositionCursorCommand() :
        QWSCommand(QWSCommand::PositionCursor,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    struct SimpleData {
        int newX;
        int newY;
    } simpleData;
};

struct QWSGrabMouseCommand : public QWSCommand
{
    QWSGrabMouseCommand() :
        QWSCommand(QWSCommand::GrabMouse,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    struct SimpleData {
        int windowid;
        bool grab;  // grab or ungrab?
    } simpleData;
};

struct QWSGrabKeyboardCommand : public QWSCommand
{
    QWSGrabKeyboardCommand() :
        QWSCommand(QWSCommand::GrabKeyboard,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    struct SimpleData {
        int windowid;
        bool grab;  // grab or ungrab?
    } simpleData;
};

#ifndef QT_NO_SOUND
struct QWSPlaySoundCommand : public QWSCommand
{
    QWSPlaySoundCommand() :
        QWSCommand(QWSCommand::PlaySound,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem) {
        QWSCommand::setData(d, len, allocateMem);
        filename = QString(reinterpret_cast<QChar*>(rawDataPtr),len/2);
    }
    void setFileName(const QString& n)
    {
        setData(reinterpret_cast<const char*>(n.unicode()), n.length()*2, true);
    }

    struct SimpleData {
        int windowid;
    } simpleData;
    QString filename;
};
#endif


#ifndef QT_NO_COP
struct QWSQCopRegisterChannelCommand : public QWSCommand
{
    QWSQCopRegisterChannelCommand() :
        QWSCommand(QWSCommand::QCopRegisterChannel,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem) {
        QWSCommand::setData(d, len, allocateMem);
        if ( simpleData.chLen > MAX_COMMAND_SIZE )
        {
            qWarning( "Command channel name too large!" );
            simpleData.chLen = MAX_COMMAND_SIZE;
        }
        if( simpleData.chLen * int(sizeof(QChar)) > len )
        {
            qWarning( "register qcop channel command - channel name length %d - buffer size %d - buffer overrun!", simpleData.chLen, len );
        }
        else
        {
            channel = QString(reinterpret_cast<const QChar*>(d), simpleData.chLen);
        }
    }

    void setChannel(const QString& n)
    {
        channel = n;
        simpleData.chLen = channel.length();
        setData(reinterpret_cast<const char*>(channel.unicode()), simpleData.chLen*2, true);
    }

    struct SimpleData {
        int chLen;
    } simpleData;
    QString channel;
};

struct QWSQCopSendCommand : public QWSCommand
{
    QWSQCopSendCommand() :
        QWSCommand(QWSCommand::QCopSend,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem) {
        QWSCommand::setData(d, len, allocateMem);

        if( simpleData.clen * int(sizeof(QChar)) + simpleData.mlen * int(sizeof(QChar)) + simpleData.dlen * int(sizeof(char)) > len )
        {
            qWarning( "qcop send command - channel name length %d - message name length %d - data size %d - buffer size %d - buffer overrun!",
                      simpleData.clen, simpleData.mlen, simpleData.dlen, len );
        }
        else
        {
            const QChar *cd = reinterpret_cast<const QChar*>(d);
            channel = QString(cd,simpleData.clen); cd += simpleData.clen;
            message = QString(cd,simpleData.mlen);
            d += simpleData.clen*sizeof(QChar) + simpleData.mlen*sizeof(QChar);
            data = QByteArray(d, simpleData.dlen);
        }
    }

    void setMessage(const QString &c, const QString &m,
                     const QByteArray &data)
    {
        this->channel = c;
        this->message = m;
        this->data = data;
        simpleData.clen = c.length();
        simpleData.mlen = m.length();
        simpleData.dlen = data.size();
        int l = simpleData.clen*sizeof(QChar);
        l += simpleData.mlen*sizeof(QChar);
        l += simpleData.dlen;
        char *tmp = new char[l];
        char *d = tmp;
        memcpy(d, c.unicode(), simpleData.clen*sizeof(QChar));
        d += simpleData.clen*sizeof(QChar);
        memcpy(d, m.unicode(), simpleData.mlen*sizeof(QChar));
        d += simpleData.mlen*sizeof(QChar);
        memcpy(d, data.data(), simpleData.dlen);
        QWSCommand::setData(tmp, l, false);
        deleteRaw = true;
    }

    struct SimpleData {
        int clen;
        int mlen;
        int dlen;
    } simpleData;
    QString channel;
    QString message;
    QByteArray data;
};

#endif


#ifndef QT_NO_QWS_INPUTMETHODS

struct QWSIMMouseCommand : public QWSCommand
{
    QWSIMMouseCommand() :
        QWSCommand(QWSCommand::IMMouse,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    struct SimpleData {
        int windowid;
        int state;
        int index;
    } simpleData;
};


struct QWSIMResponseCommand : public QWSCommand
{
    QWSIMResponseCommand() :
        QWSCommand(QWSCommand::IMResponse,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem) {
        QWSCommand::setData(d, len, allocateMem);

        QByteArray tmp = QByteArray::fromRawData(d, len);
        QDataStream s(tmp);
        s >> result;
    }

    void setResult(const QVariant & v)
    {
        QByteArray tmp;
        QDataStream s(&tmp, QIODevice::WriteOnly);
        s << v;
        setData(tmp.data(), tmp.size(), true);
    }

    struct SimpleData {
        int windowid;
        int property;
    } simpleData;

    QVariant result;
};

struct QWSIMUpdateCommand: public QWSCommand
{
    QWSIMUpdateCommand() :
        QWSCommand(QWSCommand::IMUpdate,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    struct SimpleData {
        int windowid;
        int type;
        int widgetid;
    } simpleData;
};

#endif

#ifndef QT_NO_QWSEMBEDWIDGET
struct QWSEmbedCommand : public QWSCommand
{
    QWSEmbedCommand() : QWSCommand(QWSCommand::Embed,
                                   sizeof(simpleData),
                                   reinterpret_cast<char*>(&simpleData))
    {}

    void setData(const char *d, int len, bool allocateMem = true)
    {
        QWSCommand::setData(d, len, allocateMem);

        if( simpleData.rects * int(sizeof(QRect)) > len )
        {
            qWarning( "embed command - region rectangle count %d - buffer size %d - buffer overrun!",
                      simpleData.rects, len );
        }
        else
        {
            region.setRects(reinterpret_cast<QRect*>(rawDataPtr),
                            simpleData.rects);
        }
    }

    void setData(WId embedder, WId embedded, QWSEmbedEvent::Type type,
                 const QRegion reg = QRegion())
    {
        simpleData.embedder = embedder;
        simpleData.embedded = embedded;
        simpleData.type = type;

        region = reg;
        const QVector<QRect> rects = reg.rects();
        simpleData.rects = rects.count();

        QWSCommand::setData(reinterpret_cast<const char*>(rects.constData()),
                            rects.count() * sizeof(QRect));
    }

    struct {
        WId embedder;
        WId embedded;
        QWSEmbedEvent::Type type;
        int rects;
    } simpleData;

    QRegion region;
};
#endif // QT_NO_QWSEMBEDWIDGET

struct QWSFontCommand : public QWSCommand
{
    enum CommandType {
        StartedUsingFont,
        StoppedUsingFont
    };

    QWSFontCommand() :
        QWSCommand(QWSCommand::Font,
                    sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem) {
        QWSCommand::setData(d, len, allocateMem);

        fontName = QByteArray(d, len);
    }

    void setFontName(const QByteArray &name)
    {
        setData(name.constData(), name.size(), true);
    }

    struct SimpleData {
        int type;
    } simpleData;

    QByteArray fontName;
};

struct QWSScreenTransformCommand : public QWSCommand
{
    QWSScreenTransformCommand() :
        QWSCommand(QWSCommand::ScreenTransform,
                   sizeof(simpleData), reinterpret_cast<char *>(&simpleData)) {}

    void setTransformation(int screen, int transformation)
    {
        simpleData.screen = screen;
        simpleData.transformation = transformation;
    }

    struct SimpleData {
        int screen;
        int transformation;
    } simpleData;
};

QT_END_NAMESPACE

#endif // QWSCOMMAND_QWS_P_H
