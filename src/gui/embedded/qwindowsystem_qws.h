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

#ifndef QWINDOWSYSTEM_QWS_H
#define QWINDOWSYSTEM_QWS_H

#include <QtCore/qbytearray.h>
#include <QtCore/qmap.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qlist.h>

#include <QtGui/qwsevent_qws.h>
#include <QtGui/qkbd_qws.h>
#include <QtGui/qregion.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

struct QWSWindowPrivate;
class QWSCursor;
class QWSClient;
class QWSRegionManager;
class QBrush;
class QVariant;
class QInputMethodEvent;
class QWSInputMethod;
class QWSBackingStore;
class QWSWindowSurface;

#ifdef QT3_SUPPORT
class QImage;
class QColor;
#endif

class QWSInternalWindowInfo
{
public:
    int winid;
    unsigned int clientid;
    QString name;   // Corresponds to QObject name of top-level widget
};


class Q_GUI_EXPORT QWSScreenSaver
{
public:
    virtual ~QWSScreenSaver();
    virtual void restore()=0;
    virtual bool save(int level)=0;
};


class Q_GUI_EXPORT QWSWindow
{
    friend class QWSServer;
    friend class QWSServerPrivate;

public:
    QWSWindow(int i, QWSClient* client);
    ~QWSWindow();

    int winId() const { return id; }
    const QString &name() const { return rgnName; }
    const QString &caption() const { return rgnCaption; }
    QWSClient* client() const { return c; }
    const QRegion &requestedRegion() const { return requested_region; }
    QRegion allocatedRegion() const;
    QRegion paintedRegion() const;
    bool isVisible() const { return !requested_region.isEmpty(); }
    bool isPartiallyObscured() const { return requested_region != allocatedRegion(); }
    bool isFullyObscured() const { return allocatedRegion().isEmpty(); }

    enum State { NoState, Hidden, Showing, Visible, Hiding, Raising, Lowering, Moving, ChangingGeometry, Destroyed };
    State state() const;
    Qt::WindowFlags windowFlags() const;
    QRegion dirtyOnScreen() const;

    void raise();
    void lower();
    void show();
    void hide();
    void setActiveWindow();

    bool isOpaque() const {return opaque && _opacity == 255;}
    uint opacity() const { return _opacity; }

    QWSWindowSurface* windowSurface() const { return surface; }

private:
    bool hidden() const { return requested_region.isEmpty(); }
    bool forClient(const QWSClient* cl) const { return cl==c; }

    void setName(const QString &n);
    void setCaption(const QString &c);

    void focus(bool get);
    int focusPriority() const { return last_focus_time; }
    void operation(QWSWindowOperationEvent::Operation o);
    void shuttingDown() { last_focus_time=0; }

#ifdef QT_QWS_CLIENTBLIT
    QRegion directPaintRegion() const;
    inline void setDirectPaintRegion(const QRegion &topmost);
#endif
    inline void setAllocatedRegion(const QRegion &region);

    void createSurface(const QString &key, const QByteArray &data);

#ifndef QT_NO_QWSEMBEDWIDGET
    void startEmbed(QWSWindow *window);
    void stopEmbed(QWSWindow *window);
#endif

private:
    int id;
    QString rgnName;
    QString rgnCaption;
    bool modified;
    bool onTop;
    QWSClient* c;
    QRegion requested_region;
    QRegion exposed;
    int last_focus_time;
    QWSWindowSurface *surface;
    uint _opacity;
    bool opaque;
    QWSWindowPrivate *d;
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT QRegion requested() const { return requested_region; }
//    inline QT3_SUPPORT QRegion allocation() const { return allocated_region; }
#endif
};


#ifndef QT_NO_SOUND
class QWSSoundServer;
#ifdef QT_USE_OLD_QWS_SOUND
class QWSSoundServerData;

class Q_GUI_EXPORT QWSSoundServer : public QObject {
    Q_OBJECT
public:
    QWSSoundServer(QObject* parent);
    ~QWSSoundServer();
    void playFile(const QString& filename);
private Q_SLOTS:
    void feedDevice(int fd);
private:
    QWSSoundServerData* d;
};
#endif
#endif


/*********************************************************************
 *
 * Class: QWSServer
 *
 *********************************************************************/

class QWSMouseHandler;
struct QWSCommandStruct;
class QWSServerPrivate;
class QWSServer;

extern Q_GUI_EXPORT QWSServer *qwsServer;

class Q_GUI_EXPORT QWSServer : public QObject
{
    friend class QCopChannel;
    friend class QWSMouseHandler;
    friend class QWSWindow;
    friend class QWSDisplay;
    friend class QWSInputMethod;
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWSServer)
public:
    explicit QWSServer(int flags = 0, QObject *parent=0);
#ifdef QT3_SUPPORT
    QT3_SUPPORT_CONSTRUCTOR QWSServer(int flags, QObject *parent, const char *name);
#endif
    ~QWSServer();
    enum ServerFlags { DisableKeyboard = 0x01,
                       DisableMouse = 0x02 };

    static void sendKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                             bool isPress, bool autoRepeat);
#ifndef QT_NO_QWS_KEYBOARD
    static void processKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                                bool isPress, bool autoRepeat);
#endif

    static QWSServer* instance() { return qwsServer; }

#ifndef QT_NO_QWS_INPUTMETHODS
#ifdef QT3_SUPPORT
    enum IMState { IMCompose, IMEnd, IMStart = IMCompose };
#endif
    enum IMMouse { MousePress, MouseRelease, MouseMove, MouseOutside }; //MouseMove reserved but not used
    void sendIMEvent(const QInputMethodEvent*);
    void sendIMQuery(int property);
#endif

#ifndef QT_NO_QWS_KEYBOARD
    class KeyboardFilter
    {
    public:
        virtual ~KeyboardFilter() {}
        virtual bool filter(int unicode, int keycode, int modifiers,
                            bool isPress, bool autoRepeat)=0;
    };
    static void addKeyboardFilter(KeyboardFilter *f);
    static void removeKeyboardFilter();
#endif

#ifndef QT_NO_QWS_INPUTMETHODS
    static void setCurrentInputMethod(QWSInputMethod *im);
    static void resetInputMethod();
#endif

    static void setDefaultMouse(const char *);
    static void setDefaultKeyboard(const char *);
    static void setMaxWindowRect(const QRect&);
    static void sendMouseEvent(const QPoint& pos, int state, int wheel = 0);

    static void setBackground(const QBrush &);
#ifdef QT3_SUPPORT
    static QT3_SUPPORT void setDesktopBackground(const QImage &img);
    static QT3_SUPPORT void setDesktopBackground(const QColor &);
#endif
    static QWSMouseHandler *mouseHandler();
    static const QList<QWSMouseHandler*>& mouseHandlers();
    static void setMouseHandler(QWSMouseHandler*);
#ifndef QT_NO_QWS_KEYBOARD
    static QWSKeyboardHandler* keyboardHandler();
    static void setKeyboardHandler(QWSKeyboardHandler* kh);
#endif
    QWSWindow *windowAt(const QPoint& pos);

    const QList<QWSWindow*> &clientWindows();

    void openMouse();
    void closeMouse();
    void suspendMouse();
    void resumeMouse();
#ifndef QT_NO_QWS_KEYBOARD
    void openKeyboard();
    void closeKeyboard();
#endif

    static void setScreenSaver(QWSScreenSaver*);
    static void setScreenSaverIntervals(int* ms);
    static void setScreenSaverInterval(int);
    static void setScreenSaverBlockLevel(int);
    static bool screenSaverActive();
    static void screenSaverActivate(bool);

    // the following are internal.
    void refresh();
    void refresh(QRegion &);

    void enablePainting(bool);
    static void processEventQueue();
    static QList<QWSInternalWindowInfo*> * windowList();

    void sendPropertyNotifyEvent(int property, int state);

    static QPoint mousePosition;

    static void startup(int flags);
    static void closedown();

    static void beginDisplayReconfigure();
    static void endDisplayReconfigure();

#ifndef QT_NO_QWS_CURSOR
    static void setCursorVisible(bool);
    static bool isCursorVisible();
#endif

    const QBrush &backgroundBrush() const;

    enum WindowEvent { Create=0x0001, Destroy=0x0002, Hide=0x0004, Show=0x0008,
                       Raise=0x0010, Lower=0x0020, Geometry=0x0040, Active = 0x0080,
                       Name=0x0100 };

Q_SIGNALS:
    void windowEvent(QWSWindow *w, QWSServer::WindowEvent e);

#ifndef QT_NO_COP
    void newChannel(const QString& channel);
    void removedChannel(const QString& channel);

#endif
#ifndef QT_NO_QWS_INPUTMETHODS
    void markedText(const QString &);
#endif

protected:
    void timerEvent(QTimerEvent *e);

private:
    friend class QApplicationPrivate;
    void updateWindowRegions() const;

#ifdef QT3_SUPPORT
#ifndef QT_NO_QWS_KEYBOARD
    static inline QT3_SUPPORT void setKeyboardFilter(QWSServer::KeyboardFilter *f)
        { if (f) addKeyboardFilter(f); else removeKeyboardFilter(); }
#endif
#endif

private:
#ifndef QT_NO_QWS_MULTIPROCESS
    Q_PRIVATE_SLOT(d_func(), void _q_clientClosed())
    Q_PRIVATE_SLOT(d_func(), void _q_doClient())
    Q_PRIVATE_SLOT(d_func(), void _q_deleteWindowsLater())
#endif

    Q_PRIVATE_SLOT(d_func(), void _q_screenSaverWake())
    Q_PRIVATE_SLOT(d_func(), void _q_screenSaverSleep())
    Q_PRIVATE_SLOT(d_func(), void _q_screenSaverTimeout())

#ifndef QT_NO_QWS_MULTIPROCESS
    Q_PRIVATE_SLOT(d_func(), void _q_newConnection())
#endif
};

#ifndef QT_NO_QWS_INPUTMETHODS
class Q_GUI_EXPORT QWSInputMethod : public QObject
{
    Q_OBJECT
public:
    QWSInputMethod();
    virtual ~QWSInputMethod();

    enum UpdateType {Update, FocusIn, FocusOut, Reset, Destroyed};

    virtual bool filter(int unicode, int keycode, int modifiers,
                        bool isPress, bool autoRepeat);

    virtual bool filter(const QPoint &, int state, int wheel);

    virtual void reset();
    virtual void updateHandler(int type);
    virtual void mouseHandler(int pos, int state);
    virtual void queryResponse(int property, const QVariant&);

protected:
    uint setInputResolution(bool isHigh);
    uint inputResolutionShift() const;
    // needed for required transform
    void sendMouseEvent(const QPoint &pos, int state, int wheel);

    void sendEvent(const QInputMethodEvent*);
    void sendPreeditString(const QString &preeditString, int cursorPosition, int selectionLength = 0);
    void sendCommitString(const QString &commitString, int replaceFrom = 0, int replaceLength = 0);
    void sendQuery(int property);

#ifdef QT3_SUPPORT
    inline void sendIMEvent(QWSServer::IMState, const QString& txt, int cpos, int selLen = 0);
#endif
private:
    bool mIResolution;
};

inline void QWSInputMethod::sendEvent(const QInputMethodEvent *ime)
{
    qwsServer->sendIMEvent(ime);
}
#ifdef QT3_SUPPORT
inline void QWSInputMethod::sendIMEvent(QWSServer::IMState state, const QString& txt, int cpos, int selLen)
{
    if (state == QWSServer::IMCompose) sendPreeditString(txt, cpos, selLen); else sendCommitString(txt);
}
#endif

inline void QWSInputMethod::sendQuery(int property)
{
    qwsServer->sendIMQuery(property);
}

// mouse events not inline as involve transformations.
#endif // QT_NO_QWS_INPUTMETHODS



/*********************************************************************
 *
 * Class: QWSClient
 *
 *********************************************************************/

struct QWSMouseEvent;

typedef QMap<int, QWSCursor*> QWSCursorMap;

class QWSClientPrivate;
class QWSCommand;
class QWSConvertSelectionCommand;

class Q_GUI_EXPORT QWSClient : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWSClient)
public:
    QWSClient(QObject* parent, QWS_SOCK_BASE *, int id);
    ~QWSClient();

    int socket() const;

    void setIdentity(const QString&);
    QString identity() const { return id; }

    void sendEvent(QWSEvent* event);
    void sendConnectedEvent(const char *display_spec);
    void sendMaxWindowRectEvent(const QRect &rect);
    void sendPropertyNotifyEvent(int property, int state);
    void sendPropertyReplyEvent(int property, int len, const char *data);
    void sendSelectionClearEvent(int windowid);
    void sendSelectionRequestEvent(QWSConvertSelectionCommand *cmd, int windowid);
#ifndef QT_QWS_CLIENTBLIT
    void sendRegionEvent(int winid, QRegion rgn, int type);
#else
    void sendRegionEvent(int winid, QRegion rgn, int type, int id = 0);
#endif
#ifndef QT_NO_QWSEMBEDWIDGET
    void sendEmbedEvent(int winid, QWSEmbedEvent::Type type,
                        const QRegion &region = QRegion());
#endif
    QWSCommand* readMoreCommand();

    int clientId() const { return cid; }

    QWSCursorMap cursors; // cursors defined by this client
Q_SIGNALS:
    void connectionClosed();
    void readyRead();
private Q_SLOTS:
    void closeHandler();
    void errorHandler();

private:
#ifndef QT_NO_QWS_MULTIPROCESS
    friend class QWSWindow;
    void removeUnbufferedSurface();
    void addUnbufferedSurface();
#endif

private:
    int socketDescriptor;
#ifndef QT_NO_QWS_MULTIPROCESS
    QWSSocket *csocket;
#endif
    QWSCommand* command;
    uint isClosed : 1;
    QString id;
    int cid;

    friend class QWSServerPrivate;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWINDOWSYSTEM_QWS_H
