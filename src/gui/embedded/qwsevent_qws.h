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

#ifndef QWSEVENT_QWS_H
#define QWSEVENT_QWS_H

#include <QtGui/qwsutils_qws.h>
#include <QtGui/qwsprotocolitem_qws.h>
#include <QtCore/qrect.h>
#include <QtGui/qregion.h>
#include <QtCore/qvector.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

struct QWSMouseEvent;

struct QWSEvent : QWSProtocolItem {

    QWSEvent(int t, int len, char *ptr) : QWSProtocolItem(t,len,ptr) {}



    enum Type {
        NoEvent,
        Connected,
        Mouse,
        Focus,
        Key,
        Region,
        Creation,
        PropertyNotify,
        PropertyReply,
        SelectionClear,
        SelectionRequest,
        SelectionNotify,
        MaxWindowRect,
        QCopMessage,
        WindowOperation,
        IMEvent,
        IMQuery,
        IMInit,
        Embed,
        Font,
        ScreenTransformation,
        NEvent
    };

    QWSMouseEvent *asMouse()
        { return type == Mouse ? reinterpret_cast<QWSMouseEvent*>(this) : 0; }
    int window() { return *(reinterpret_cast<int*>(simpleDataPtr)); }
    int window() const { return *(reinterpret_cast<int*>(simpleDataPtr)); }
    static QWSEvent *factory(int type);
};


//All events must start with windowID

struct QWSConnectedEvent : QWSEvent {
    QWSConnectedEvent()
        : QWSEvent(QWSEvent::Connected, sizeof(simpleData),
                reinterpret_cast<char*>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSEvent::setData(d, len, allocateMem);
        display = reinterpret_cast<char*>(rawDataPtr);
    }

    struct SimpleData {
        int window;
        int len;
        int clientId;
        int servershmid;
    } simpleData;

    char *display;
};

struct QWSMaxWindowRectEvent : QWSEvent {
    QWSMaxWindowRectEvent()
        : QWSEvent(MaxWindowRect, sizeof(simpleData), reinterpret_cast<char*>(&simpleData)) { }
    struct SimpleData {
        int window;
        QRect rect;
    } simpleData;
};

struct QWSMouseEvent : QWSEvent {
    QWSMouseEvent()
        : QWSEvent(QWSEvent::Mouse, sizeof(simpleData),
                reinterpret_cast<char*>(&simpleData)) {}
    struct SimpleData {
        int window;
        int x_root, y_root, state, delta;
        int time; // milliseconds
    } simpleData;
};

struct QWSFocusEvent : QWSEvent {
    QWSFocusEvent()
        : QWSEvent(QWSEvent::Focus, sizeof(simpleData), reinterpret_cast<char*>(&simpleData))
        { memset(reinterpret_cast<char*>(&simpleData),0,sizeof(simpleData)); }
    struct SimpleData {
        int window;
        uint get_focus:1;
    } simpleData;
};

struct QWSKeyEvent: QWSEvent {
    QWSKeyEvent()
        : QWSEvent(QWSEvent::Key, sizeof(simpleData),
              reinterpret_cast<char*>(&simpleData)) {}
    struct SimpleData {
        int window;
        uint keycode;
        Qt::KeyboardModifiers modifiers;
        ushort unicode;
        uint is_press:1;
        uint is_auto_repeat:1;
    } simpleData;
};


struct QWSCreationEvent : QWSEvent {
    QWSCreationEvent()
        : QWSEvent(QWSEvent::Creation, sizeof(simpleData),
              reinterpret_cast<char*>(&simpleData)) {}
    struct SimpleData {
        int objectid;
        int count;
    } simpleData;
};

#ifndef QT_NO_QWS_PROPERTIES
struct QWSPropertyNotifyEvent : QWSEvent {
    QWSPropertyNotifyEvent()
        : QWSEvent(QWSEvent::PropertyNotify, sizeof(simpleData),
              reinterpret_cast<char*>(&simpleData)) {}
    enum State {
        PropertyNewValue,
        PropertyDeleted
    };
    struct SimpleData {
        int window;
        int property;
        int state;
    } simpleData;
};
#endif

struct QWSSelectionClearEvent : QWSEvent {
    QWSSelectionClearEvent()
        : QWSEvent(QWSEvent::SelectionClear, sizeof(simpleData),
              reinterpret_cast<char*>(&simpleData)) {}
    struct SimpleData {
        int window;
    } simpleData;
};

struct QWSSelectionRequestEvent : QWSEvent {
    QWSSelectionRequestEvent()
        : QWSEvent(QWSEvent::SelectionRequest, sizeof(simpleData),
              reinterpret_cast<char*>(&simpleData)) {}
    struct SimpleData {
        int window;
        int requestor; // window which wants the selection
        int property; // property on requestor into which the selection should be stored, normally QWSProperty::PropSelection
        int mimeTypes; // Value is stored in the property mimeType on the requestor window. This value may contain
        // multiple mimeTypes separated by ;; where the order reflects the priority
    } simpleData;
};

struct QWSSelectionNotifyEvent : QWSEvent {
    QWSSelectionNotifyEvent()
        : QWSEvent(QWSEvent::SelectionNotify, sizeof(simpleData),
              reinterpret_cast<char*>(&simpleData)) {}
    struct SimpleData {
        int window;
        int requestor; // the window which wanted the selection and to which this event is sent
        int property; // property of requestor in which the data of the selection is stored
        int mimeType; // a property on the requestor in which the mime type in which the selection is, is stored
    } simpleData;
};

//complex events:

struct QWSRegionEvent : QWSEvent {
    QWSRegionEvent()
        : QWSEvent(QWSEvent::Region, sizeof(simpleData),
                reinterpret_cast<char*>(&simpleData))
        { memset(reinterpret_cast<char*>(&simpleData),0,sizeof(simpleData)); }

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSEvent::setData(d, len, allocateMem);
        rectangles = reinterpret_cast<QRect*>(rawDataPtr);
    }

    void setData(int winId, const QRegion &region, uint type) {
        const QVector<QRect> rects = region.rects();
        setData(reinterpret_cast<const char*>(rects.constData()),
            rects.size() * sizeof(QRect));
        simpleData.window = winId;
        simpleData.nrectangles = rects.size();
        simpleData.type = type;
#ifdef QT_QWS_CLIENTBLIT
        simpleData.id = 0;
#endif
    }

    enum Type {Allocation
#ifdef QT_QWS_CLIENTBLIT
        , DirectPaint
#endif
    };
    struct SimpleData {
        int window;
        int nrectangles;
#ifdef QT_QWS_CLIENTBLIT
        int id;
#endif
        uint type:8;
    } simpleData;

    QRect *rectangles;
};

#ifndef QT_NO_QWSEMBEDWIDGET
struct QWSEmbedEvent : QWSEvent
{
    QWSEmbedEvent() : QWSEvent(QWSEvent::Embed, sizeof(simpleData),
                               reinterpret_cast<char*>(&simpleData))
    {}

    enum Type { StartEmbed = 1, StopEmbed = 2, Region = 4 };

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSEvent::setData(d, len, allocateMem);
        region.setRects(reinterpret_cast<const QRect *>(rawDataPtr),
                        simpleData.nrectangles);
    }

    void setData(int winId, Type type, const QRegion &reg = QRegion()) {
        simpleData.window = winId;
        simpleData.nrectangles = reg.rects().size();
        simpleData.type = type;
        region = reg;
        const QVector<QRect> rects = reg.rects();
        QWSEvent::setData(reinterpret_cast<const char*>(rects.data()),
                          rects.size() * sizeof(QRect));
    }

    struct SimpleData {
        int window;
        int nrectangles;
        Type type;
    } simpleData;

    QRegion region;
};
#endif // QT_NO_QWSEMBEDWIDGET

#ifndef QT_NO_QWS_PROPERTIES
struct QWSPropertyReplyEvent : QWSEvent {
    QWSPropertyReplyEvent()
        : QWSEvent(QWSEvent::PropertyReply, sizeof(simpleData),
                reinterpret_cast<char*>(&simpleData)) {}

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSEvent::setData(d, len, allocateMem);
        data = reinterpret_cast<char*>(rawDataPtr);
    }

    struct SimpleData {
        int window;
        int property;
        int len;
    } simpleData;
    char *data;
};
#endif //QT_NO_QWS_PROPERTIES

#ifndef QT_NO_COP
struct QWSQCopMessageEvent : QWSEvent {
    QWSQCopMessageEvent()
        : QWSEvent(QWSEvent::QCopMessage, sizeof(simpleData),
                reinterpret_cast<char*>(&simpleData))
        { memset(reinterpret_cast<char*>(&simpleData),0,sizeof(simpleData)); }

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSEvent::setData(d, len, allocateMem);
        char* p = rawDataPtr;
	channel = QByteArray(p, simpleData.lchannel);
        p += simpleData.lchannel;
        message = QByteArray(p, simpleData.lmessage);
        p += simpleData.lmessage;
        data = QByteArray(p, simpleData.ldata);
    }

    void setDataDirect(const char *d, int len) {
        QWSEvent::setData(d, len, false);
        deleteRaw = true;
    }

    struct SimpleData {
        bool is_response;
        int lchannel;
        int lmessage;
        int ldata;
    } simpleData;

    QByteArray channel;
    QByteArray message;
    QByteArray data;
};

#endif

struct QWSWindowOperationEvent : QWSEvent {
    QWSWindowOperationEvent()
        : QWSEvent(WindowOperation, sizeof(simpleData), reinterpret_cast<char*>(&simpleData)) { }

    enum Operation { Show, Hide, ShowMaximized, ShowNormal, ShowMinimized, Close };
    struct SimpleData {
        int window;
        Operation op;
    } simpleData;
};

#ifndef QT_NO_QWS_INPUTMETHODS


struct QWSIMEvent : QWSEvent {
    QWSIMEvent()
        : QWSEvent(IMEvent, sizeof(simpleData), reinterpret_cast<char*>(&simpleData))
   { memset(reinterpret_cast<char*>(&simpleData),0,sizeof(simpleData)); }

    struct SimpleData {
        int window;
        int replaceFrom;
        int replaceLength;
    } simpleData;

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSEvent::setData(d, len, allocateMem);
        streamingData = QByteArray::fromRawData(rawDataPtr, len);
    }
    QByteArray streamingData;
};


struct QWSIMInitEvent : QWSEvent {
    QWSIMInitEvent()
        : QWSEvent(IMInit, sizeof(simpleData), reinterpret_cast<char*>(&simpleData))
   { memset(reinterpret_cast<char*>(&simpleData),0,sizeof(simpleData)); }

    struct SimpleData {
        int window;
        int existence;
    } simpleData;

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSEvent::setData(d, len, allocateMem);
        streamingData = QByteArray::fromRawData(rawDataPtr, len);
    }
    QByteArray streamingData;
};


struct QWSIMQueryEvent : QWSEvent {
    QWSIMQueryEvent()
        : QWSEvent(QWSEvent::IMQuery, sizeof(simpleData),
              reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int window;
        int property;
    } simpleData;

};

#endif

struct QWSFontEvent : QWSEvent {
    QWSFontEvent()
        : QWSEvent(QWSEvent::Font, sizeof(simpleData),
                reinterpret_cast<char*>(&simpleData)) {}

    enum EventType {
        FontRemoved
    };

    void setData(const char *d, int len, bool allocateMem = true) {
        QWSEvent::setData(d, len, allocateMem);
        fontName = QByteArray::fromRawData(rawDataPtr, len);
    }

    struct SimpleData {
        uchar type;
    } simpleData;
    QByteArray fontName;
};

struct QWSScreenTransformationEvent : QWSEvent {
    QWSScreenTransformationEvent()
        : QWSEvent(QWSEvent::ScreenTransformation, sizeof(simpleData),
                   reinterpret_cast<char*>(&simpleData)) {}

    struct SimpleData {
        int screen;
        int transformation;
    } simpleData;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWSEVENT_QWS_H
