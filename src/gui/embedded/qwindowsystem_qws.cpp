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

#include "qplatformdefs.h"

#include "qwindowsystem_qws.h"
#include "qwsevent_qws.h"
#include "qwscommand_qws_p.h"
#include "qtransportauth_qws_p.h"
#include "qwsutils_qws.h"
#include "qwscursor_qws.h"
#include "qwsdisplay_qws.h"
#include "qmouse_qws.h"
#include "qcopchannel_qws.h"
#include "qwssocket_qws.h"

#include "qapplication.h"
#include "private/qapplication_p.h"
#include "qsocketnotifier.h"
#include "qpolygon.h"
#include "qimage.h"
#include "qcursor.h"
#include <private/qpaintengine_raster_p.h>
#include "qscreen_qws.h"
#include "qwindowdefs.h"
#include "private/qlock_p.h"
#include "qwslock_p.h"
#include "qfile.h"
#include "qtimer.h"
#include "qpen.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qinputcontext.h"
#include "qpainter.h"

#include <qdebug.h>

#include "qkbddriverfactory_qws.h"
#include "qmousedriverfactory_qws.h"

#include <qbuffer.h>
#include <qdir.h>

#include <private/qwindowsurface_qws_p.h>
#include <private/qfontengine_qpf_p.h>

#include "qwindowsystem_p.h"


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifndef QT_NO_QWS_MULTIPROCESS
#include <sys/param.h>
#include <sys/mount.h>
#endif

#if !defined(QT_NO_SOUND) && !defined(Q_OS_DARWIN)
#ifdef QT_USE_OLD_QWS_SOUND
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#else
#include "qsoundqss_qws.h"
#endif
#endif

//#define QWS_DEBUG_FONTCLEANUP

QT_BEGIN_NAMESPACE

QWSServer Q_GUI_EXPORT *qwsServer=0;
static QWSServerPrivate *qwsServerPrivate=0;

#define MOUSE 0
#define KEY 1
//#define EVENT_BLOCK_DEBUG

QWSScreenSaver::~QWSScreenSaver()
{
}

extern QByteArray qws_display_spec;
extern void qt_init_display(); //qapplication_qws.cpp
extern QString qws_qtePipeFilename();

extern void qt_client_enqueue(const QWSEvent *); //qapplication_qws.cpp
extern QList<QWSCommand*> *qt_get_server_queue();

Q_GLOBAL_STATIC_WITH_ARGS(QString, defaultMouse, (QLatin1String("Auto")))
Q_GLOBAL_STATIC_WITH_ARGS(QString, defaultKeyboard, (QLatin1String("TTY")))
static const int FontCleanupInterval = 60 * 1000;

static int qws_keyModifiers = 0;

static QWSWindow *keyboardGrabber;
static bool keyboardGrabbing;

static int get_object_id(int count = 1)
{
    static int next=1000;
    int n = next;
    next += count;
    return n;
}
#ifndef QT_NO_QWS_INPUTMETHODS
static QWSInputMethod *current_IM = 0;

static QWSWindow *current_IM_composing_win = 0;
static int current_IM_winId = -1;
static bool force_reject_strokeIM = false;
#endif

static void cleanupFontsDir();

//#define QWS_REGION_DEBUG

/*!
    \class QWSScreenSaver
    \ingroup qws

    \brief The QWSScreenSaver class is a base class for screensavers
    in Qt for Embedded Linux.

    When running \l{Qt for Embedded Linux} applications, it is the server
    application that installs and controls the screensaver.
    \l{Qt for Embedded Linux} supports multilevel screen saving; i.e., it is possible to
    specify several different levels of screen responsiveness. For
    example, you can choose to first turn off the light before you
    fully activate the screensaver.

    Note that there exists no default screensaver implementation.

    To create a custom screensaver, derive from this class and
    reimplement the restore() and save() functions. These functions
    are called whenever the screensaver is activated or deactivated,
    respectively. Once an instance of your custom screensaver is
    created, you can use the QWSServer::setScreenSaver() function to
    install it.

    \sa QWSServer, QScreen, {Qt for Embedded Linux}
*/

/*!
    \fn QWSScreenSaver::~QWSScreenSaver()

    Reimplement this function to destroy the screensaver.
*/

/*!
    \fn QWSScreenSaver::restore()

    Implement this function to deactivate the screensaver, restoring
    the previously saved screen.

    \sa save(), QWSServer::screenSaverActivate()
*/

/*!
    \fn QWSScreenSaver::save(int level)

    Implement this function to activate the screensaver, saving the
    current screen.

    \l{Qt for Embedded Linux} supports multilevel screen saving; i.e., it is
    possible to specify several different levels of screen
    responsiveness. For example, you can choose to first turn off the
    light before you fully activate the screensaver. Use the
    QWSServer::setScreenSaverIntervals() to specify the time intervals
    between the different levels.

    This function should return true if the screensaver successfully
    enters the given \a level; otherwise it should return false.

    \sa restore(), QWSServer::screenSaverActivate()
*/

class QWSWindowPrivate
{
public:
    QWSWindowPrivate();

#ifdef QT_QWS_CLIENTBLIT
    QRegion directPaintRegion;
#endif
    QRegion allocatedRegion;
#ifndef QT_NO_QWSEMBEDWIDGET
    QList<QWSWindow*> embedded;
    QWSWindow *embedder;
#endif
    QWSWindow::State state;
    Qt::WindowFlags windowFlags;
    QRegion dirtyOnScreen;
    bool painted;
};

QWSWindowPrivate::QWSWindowPrivate()
    :
#ifndef QT_NO_QWSEMBEDWIDGET
    embedder(0), state(QWSWindow::NoState),
#endif
    painted(false)
{
}

/*!
    \class QWSWindow
    \ingroup qws

    \brief The QWSWindow class encapsulates a top-level window in
    Qt for Embedded Linux.

    When you run a \l{Qt for Embedded Linux} application, it either runs as a
    server or connects to an existing server. As applications add and
    remove windows, the server process maintains information about
    each window. In \l{Qt for Embedded Linux}, top-level windows are
    encapsulated as QWSWindow objects. Note that you should never
    construct the QWSWindow class yourself; the current top-level
    windows can be retrieved using the QWSServer::clientWindows()
    function.

    With a window at hand, you can retrieve its caption, name, opacity
    and ID using the caption(), name(), opacity() and winId()
    functions, respectively. Use the client() function to retrieve a
    pointer to the client that owns the window.

    Use the isVisible() function to find out if the window is
    visible. You can find out if the window is completely obscured by
    another window or by the bounds of the screen, using the
    isFullyObscured() function. The isOpaque() function returns true
    if the window has an alpha channel equal to 255. Finally, the
    requestedRegion() function returns the region of the display the
    window wants to draw on.

    \sa QWSServer, QWSClient, {Qt for Embedded Linux Architecture}
*/

/*!
    \fn int QWSWindow::winId() const

    Returns the window's ID.

    \sa name(), caption()
*/

/*!
    \fn const QString &QWSWindow::name() const

    Returns the window's name, which is taken from the \l {QWidget::}{objectName()}
    at the time of \l {QWidget::}{show()}.

    \sa caption(), winId()
*/

/*!
    \fn const QString &QWSWindow::caption() const

    Returns the window's caption.

    \sa name(), winId()
*/

/*!
    \fn QWSClient* QWSWindow::client() const

    Returns a reference to the QWSClient object that owns this window.

    \sa requestedRegion()
*/

/*!
    \fn QRegion QWSWindow::requestedRegion() const

    Returns the region that the window has requested to draw onto,
    including any window decorations.

    \sa client()
*/

/*!
    \fn bool QWSWindow::isVisible() const

    Returns true if the window is visible; otherwise returns false.

    \sa isFullyObscured()
*/

/*!
    \fn bool QWSWindow::isOpaque() const

    Returns true if the window is opaque, i.e., if its alpha channel
    equals 255; otherwise returns false.

    \sa opacity()
*/

/*!
    \fn uint QWSWindow::opacity () const

    Returns the window's alpha channel value.

    \sa isOpaque()
*/

/*!
    \fn bool QWSWindow::isPartiallyObscured() const
    \internal

    Returns true if the window is partially obsured by another window
    or by the bounds of the screen; otherwise returns false.
*/

/*!
    \fn bool QWSWindow::isFullyObscured() const

    Returns true if the window is completely obsured by another window
    or by the bounds of the screen; otherwise returns false.

    \sa isVisible()
*/

/*!
    \fn QWSWindowSurface* QWSWindow::windowSurface() const
    \internal
*/

QWSWindow::QWSWindow(int i, QWSClient* client)
        : id(i), modified(false),
          onTop(false), c(client), last_focus_time(0), _opacity(255),
          opaque(true), d(new QWSWindowPrivate)
{
    surface = 0;
}


/*!
    \enum QWSWindow::State

    This enum describes the state of a window. Most of the
    transitional states are set just before a call to
    QScreen::exposeRegion() and reset immediately afterwards.

    \value NoState Initial state before the window is properly initialized.
    \value Hidden The window is not visible.
    \value Showing The window is being shown.
    \value Visible The window is visible, and not in a transition.
    \value Hiding The window is being hidden.
    \value Raising The windoe is being raised.
    \value Lowering The window is being raised.
    \value Moving The window is being moved.
    \value ChangingGeometry The window's geometry is being changed.
    \value Destroyed  The window is destroyed.

    \sa state(), QScreen::exposeRegion()
*/

/*!
  Returns the current state of the window.

  \since 4.3
*/
QWSWindow::State QWSWindow::state() const
{
    return d->state;
}

/*!
  Returns the window flags of the window. This value is only available
  after the first paint event.

  \since 4.3
*/
Qt::WindowFlags QWSWindow::windowFlags() const
{
    return d->windowFlags;
}

/*!
  Returns the region that has been repainted since the previous
  QScreen::exposeRegion(), and needs to be copied to the screen.
  \since 4.3
*/
QRegion QWSWindow::dirtyOnScreen() const
{
    return d->dirtyOnScreen;
}

void QWSWindow::createSurface(const QString &key, const QByteArray &data)
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (surface && !surface->isBuffered())
        c->removeUnbufferedSurface();
#endif

    delete surface;
    surface = qt_screen->createSurface(key);
    surface->setPermanentState(data);

#ifndef QT_NO_QWS_MULTIPROCESS
    if (!surface->isBuffered())
        c->addUnbufferedSurface();
#endif
}

/*!
    \internal
    Raises the window above all other windows except "Stay on top" windows.
*/
void QWSWindow::raise()
{
    qwsServerPrivate->raiseWindow(this);
#ifndef QT_NO_QWSEMBEDWIDGET
    const int n = d->embedded.size();
    for (int i = 0; i < n; ++i)
        d->embedded.at(i)->raise();
#endif
}

/*!
    \internal
    Lowers the window below other windows.
*/
void QWSWindow::lower()
{
    qwsServerPrivate->lowerWindow(this);
#ifndef QT_NO_QWSEMBEDWIDGET
    const int n = d->embedded.size();
    for (int i = 0; i < n; ++i)
        d->embedded.at(i)->lower();
#endif
}

/*!
    \internal
    Shows the window.
*/
void QWSWindow::show()
{
    operation(QWSWindowOperationEvent::Show);
#ifndef QT_NO_QWSEMBEDWIDGET
    const int n = d->embedded.size();
    for (int i = 0; i < n; ++i)
        d->embedded.at(i)->show();
#endif
}

/*!
    \internal
    Hides the window.
*/
void QWSWindow::hide()
{
    operation(QWSWindowOperationEvent::Hide);
#ifndef QT_NO_QWSEMBEDWIDGET
    const int n = d->embedded.size();
    for (int i = 0; i < n; ++i)
        d->embedded.at(i)->hide();
#endif
}

/*!
    \internal
    Make this the active window (i.e., sets the keyboard focus to this
    window).
*/
void QWSWindow::setActiveWindow()
{
    qwsServerPrivate->setFocus(this, true);
#ifndef QT_NO_QWSEMBEDWIDGET
    const int n = d->embedded.size();
    for (int i = 0; i < n; ++i)
        d->embedded.at(i)->setActiveWindow();
#endif
}

void QWSWindow::setName(const QString &n)
{
    rgnName = n;
}

/*!
  \internal
  Sets the window's caption to \a c.
*/
void QWSWindow::setCaption(const QString &c)
{
    rgnCaption = c;
}


static int global_focus_time_counter=100;

void QWSWindow::focus(bool get)
{
    if (get)
        last_focus_time = global_focus_time_counter++;
    if (c) {
        QWSFocusEvent event;
        event.simpleData.window = id;
        event.simpleData.get_focus = get;
        c->sendEvent(&event);
    }
}

void QWSWindow::operation(QWSWindowOperationEvent::Operation o)
{
    if (!c)
        return;
    QWSWindowOperationEvent event;
    event.simpleData.window = id;
    event.simpleData.op = o;
    c->sendEvent(&event);
}

/*!
    \internal
    Destructor.
*/
QWSWindow::~QWSWindow()
{
#ifndef QT_NO_QWS_INPUTMETHODS
    if (current_IM_composing_win == this)
        current_IM_composing_win = 0;
#endif
#ifndef QT_NO_QWSEMBEDWIDGET
    QWSWindow *embedder = d->embedder;
    if (embedder) {
        embedder->d->embedded.removeAll(this);
        d->embedder = 0;
    }
    while (!d->embedded.isEmpty())
        stopEmbed(d->embedded.first());
#endif

#ifndef QT_NO_QWS_MULTIPROCESS
    if (surface && !surface->isBuffered()) {
        if (c && c->d_func()) // d_func() will be 0 if client is deleted
            c->removeUnbufferedSurface();
    }
#endif

    delete surface;
    delete d;
}

/*!
    \internal

    Returns the region that the window is allowed to draw onto,
    including any window decorations but excluding regions covered by
    other windows.

    \sa paintedRegion(), requestedRegion()
*/
QRegion QWSWindow::allocatedRegion() const
{
    return d->allocatedRegion;
}

#ifdef QT_QWS_CLIENTBLIT
QRegion QWSWindow::directPaintRegion() const
{
    return d->directPaintRegion;
}

inline void QWSWindow::setDirectPaintRegion(const QRegion &r)
{
    d->directPaintRegion = r;
}
#endif

/*!
    \internal

    Returns the region that the window is known to have drawn into.

    \sa allocatedRegion(), requestedRegion()
*/
QRegion QWSWindow::paintedRegion() const
{
    return (d->painted ? d->allocatedRegion : QRegion());
}

inline void QWSWindow::setAllocatedRegion(const QRegion &region)
{
    d->allocatedRegion = region;
}

#ifndef QT_NO_QWSEMBEDWIDGET
inline void QWSWindow::startEmbed(QWSWindow *w)
{
    d->embedded.append(w);
    w->d->embedder = this;
}

inline void QWSWindow::stopEmbed(QWSWindow *w)
{
    w->d->embedder = 0;
    w->client()->sendEmbedEvent(w->winId(), QWSEmbedEvent::Region, QRegion());
    d->embedded.removeAll(w);
}
#endif // QT_NO_QWSEMBEDWIDGET

/*********************************************************************
 *
 * Class: QWSClient
 *
 *********************************************************************/

class QWSClientPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QWSClient)

public:
    QWSClientPrivate();
    ~QWSClientPrivate();

    void setLockId(int id);
    void unlockCommunication();

private:
#ifndef QT_NO_QWS_MULTIPROCESS
    QWSLock *clientLock;
    bool shutdown;
    int numUnbufferedSurfaces;
#endif
    QSet<QByteArray> usedFonts;
    friend class QWSServerPrivate;
};

QWSClientPrivate::QWSClientPrivate()
{
#ifndef QT_NO_QWS_MULTIPROCESS
    clientLock = 0;
    shutdown = false;
    numUnbufferedSurfaces = 0;
#endif
}

QWSClientPrivate::~QWSClientPrivate()
{
#ifndef QT_NO_QWS_MULTIPROCESS
    delete clientLock;
#endif
}

void QWSClientPrivate::setLockId(int id)
{
#ifdef QT_NO_QWS_MULTIPROCESS
    Q_UNUSED(id);
#else
    clientLock = new QWSLock(id);
#endif
}

void QWSClientPrivate::unlockCommunication()
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (clientLock)
        clientLock->unlock(QWSLock::Communication);
#endif
}

/*!
    \class QWSClient
    \ingroup qws

    \brief The QWSClient class encapsulates a client process in Qt for Embedded Linux.

    When you run a \l{Qt for Embedded Linux} application, it either runs as a
    server or connects to an existing server. The server and client
    processes have different responsibilities: The client process
    performs all application specific operations. The server process
    is responsible for managing the clients as well as taking care of
    the pointer handling, character input, and screen output. In
    addition, the server provides functionality to handle input
    methods.

    As applications add and remove windows, the server process
    maintains information about each window. In \l{Qt for Embedded Linux},
    top-level windows are encapsulated as QWSWindow objects. A list of
    the current windows can be retrieved using the
    QWSServer::clientWindows() function, and each window can tell
    which client that owns it through its QWSWindow::client()
    function.

    A QWSClient object has an unique ID that can be retrieved using
    its clientId() function. QWSClient also provides the identity()
    function which typically returns the name of this client's running
    application.

    \sa QWSServer, QWSWindow, {Qt for Embedded Linux Architecture}
*/

/*!
   \internal
*/
//always use frame buffer
QWSClient::QWSClient(QObject* parent, QWS_SOCK_BASE* sock, int id)
    : QObject(*new QWSClientPrivate, parent), command(0), cid(id)
{
#ifdef QT_NO_QWS_MULTIPROCESS
    Q_UNUSED(sock);
    isClosed = false;
#else
    csocket = 0;
    if (!sock) {
        socketDescriptor = -1;
        isClosed = false;
    } else {
        csocket = static_cast<QWSSocket*>(sock); //###
        isClosed = false;

        csocket->flush();
        socketDescriptor = csocket->socketDescriptor();
        connect(csocket, SIGNAL(readyRead()), this, SIGNAL(readyRead()));
        connect(csocket, SIGNAL(disconnected()), this, SLOT(closeHandler()));
        connect(csocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorHandler()));
    }
#endif //QT_NO_QWS_MULTIPROCESS
}

/*!
   \internal
*/
QWSClient::~QWSClient()
{
    qDeleteAll(cursors);
    delete command;
#ifndef QT_NO_QWS_MULTIPROCESS
    delete csocket;
#endif
}

#ifndef QT_NO_QWS_MULTIPROCESS
void QWSClient::removeUnbufferedSurface()
{
    Q_D(QWSClient);
    --d->numUnbufferedSurfaces;
}

void QWSClient::addUnbufferedSurface()
{
    Q_D(QWSClient);
    ++d->numUnbufferedSurfaces;
}
#endif // QT_NO_QWS_MULTIPROCESS

/*!
   \internal
*/
void QWSClient::setIdentity(const QString& i)
{
    id = i;
}

void QWSClient::closeHandler()
{
    isClosed = true;
    emit connectionClosed();
}

void QWSClient::errorHandler()
{
#if defined(QWS_SOCKET_DEBUG)
    qDebug("Client %p error %s", this, csocket ? csocket->errorString().toLatin1().constData() : "(no socket)");
#endif
    isClosed = true;
//####Do we need to clean out the pipes?

    emit connectionClosed();
}

/*!
   \internal
*/
int QWSClient::socket() const
{
    return socketDescriptor;
}

/*!
   \internal
*/
void QWSClient::sendEvent(QWSEvent* event)
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (csocket) {
        // qDebug() << "QWSClient::sendEvent type " << event->type << " socket state " << csocket->state();
        if ((QAbstractSocket::SocketState)(csocket->state()) == QAbstractSocket::ConnectedState) {
            event->write(csocket);
        }
    }
    else
#endif
    {
        qt_client_enqueue(event);
    }
}

/*!
   \internal
*/
void QWSClient::sendRegionEvent(int winid, QRegion rgn, int type
#ifdef QT_QWS_CLIENTBLIT
        , int id
#endif
        )
{
#ifndef QT_NO_QWS_MULTIPROCESS
    Q_D(QWSClient);
    if (d->clientLock)
        d->clientLock->lock(QWSLock::RegionEvent);
#endif

    QWSRegionEvent event;
    event.setData(winid, rgn, type);
#ifdef QT_QWS_CLIENTBLIT
    event.simpleData.id = id;
#endif

//    qDebug() << "Sending Region event to" << winid << "rgn" << rgn << "type" << type;

    sendEvent(&event);
}

extern int qt_servershmid;

/*!
   \internal
*/
void QWSClient::sendConnectedEvent(const char *display_spec)
{
    QWSConnectedEvent event;
    event.simpleData.window = 0;
    event.simpleData.len = strlen(display_spec) + 1;
    event.simpleData.clientId = cid;
    event.simpleData.servershmid = qt_servershmid;
    char * tmp=(char *)display_spec;
    event.setData(tmp, event.simpleData.len);
    sendEvent(&event);
}

/*!
   \internal
*/
void QWSClient::sendMaxWindowRectEvent(const QRect &rect)
{
    QWSMaxWindowRectEvent event;
    event.simpleData.window = 0;
    event.simpleData.rect = rect;
    sendEvent(&event);
}

/*!
   \internal
*/
#ifndef QT_NO_QWS_PROPERTIES
void QWSClient::sendPropertyNotifyEvent(int property, int state)
{
    QWSPropertyNotifyEvent event;
    event.simpleData.window = 0; // not used yet
    event.simpleData.property = property;
    event.simpleData.state = state;
    sendEvent(&event);
}

/*!
   \internal
*/
void QWSClient::sendPropertyReplyEvent(int property, int len, const char *data)
{
    QWSPropertyReplyEvent event;
    event.simpleData.window = 0; // not used yet
    event.simpleData.property = property;
    event.simpleData.len = len;
    event.setData(data, len);
    sendEvent(&event);
}
#endif //QT_NO_QWS_PROPERTIES

/*!
   \internal
*/
void QWSClient::sendSelectionClearEvent(int windowid)
{
    QWSSelectionClearEvent event;
    event.simpleData.window = windowid;
    sendEvent(&event);
}

/*!
   \internal
*/
void QWSClient::sendSelectionRequestEvent(QWSConvertSelectionCommand *cmd, int windowid)
{
    QWSSelectionRequestEvent event;
    event.simpleData.window = windowid;
    event.simpleData.requestor = cmd->simpleData.requestor;
    event.simpleData.property = cmd->simpleData.selection;
    event.simpleData.mimeTypes = cmd->simpleData.mimeTypes;
    sendEvent(&event);
}

#ifndef QT_NO_QWSEMBEDWIDGET
/*!
   \internal
*/
void QWSClient::sendEmbedEvent(int windowid, QWSEmbedEvent::Type type,
                               const QRegion &region)
{
    QWSEmbedEvent event;
    event.setData(windowid, type, region);
    sendEvent(&event);
}
#endif // QT_NO_QWSEMBEDWIDGET

/*!
   \fn void QWSClient::connectionClosed()
   \internal
*/

/*!
    \fn void QWSClient::readyRead();
    \internal
*/

/*!
   \fn int QWSClient::clientId () const

   Returns an integer uniquely identfying this client.
*/

/*!
   \fn QString QWSClient::identity () const

   Returns the name of this client's running application.
*/
/*********************************************************************
 *
 * Class: QWSServer
 *
 *********************************************************************/

/*!
    \class QWSServer
    \brief The QWSServer class encapsulates a server process in Qt for Embedded Linux.

    \ingroup qws

    When you run a \l{Qt for Embedded Linux} application, it either runs as a
    server or connects to an existing server. The server and client
    processes have different responsibilities: The client process
    performs all application specific operations. The server process
    is responsible for managing the clients as well as taking care of
    the pointer handling, character input, and screen output. In
    addition, the server provides functionality to handle input
    methods.

    In \l{Qt for Embedded Linux}, all system generated events are passed to the
    server application which then propagates the event to the
    appropriate client. See the \l{Qt for Embedded Linux Architecture}
    documentation for details.

    Note that this class is instantiated by QApplication for
    \l{Qt for Embedded Linux} server processes; you should never construct this
    class yourself. Use the instance() function to retrieve a pointer
    to the server object.

    Note that the static functions of the QWSServer class can only be
    used in the server process.

    \tableofcontents

    \section1 Client Administration

    As applications add and remove windows, the server process
    maintains information about each window. In \l{Qt for Embedded Linux},
    top-level windows are encapsulated as QWSWindow objects. Each
    window can tell which client that owns it through its
    QWSWindow::client() function. Use the clientWindows() function to
    retrieve a list of the current top-level windows. Given a
    particular position on the display, the window containing it can
    be retrieved using the windowAt() function.

    QWSServer also provides the windowEvent() signal which is emitted
    whenever something happens to a top level window; the WindowEvent
    enum describes the various types of events that the signal
    recognizes. In addition, the server class provides the
    markedText() signal which is emitted whenever some text has been
    selected in any of the windows, passing the selection as
    parameter.

    The QCopChannel class and the QCOP communication protocol enable
    transfer of messages between clients. QWSServer provides the
    newChannel() and removedChannel() signals that is emitted whenever
    a new QCopChannel object is created or destroyed, respectively.

    See also: QWSWindow, QWSClient and QCopChannel.


    \section1 Mouse Handling

    The mouse driver (represented by an instance of the
    QWSMouseHandler class) is loaded by the server application when it
    starts running, using Qt's \l {How to Create Qt Plugins}{plugin
    system}. A mouse driver receives mouse events from the device and
    encapsulates each event with an instance of the QWSEvent class
    which it then passes to the server.

    The openMouse() function opens the mouse devices specified by the
    QWS_MOUSE_PROTO environment variable, and the setMouseHandler()
    functions sets the primary mouse driver. Alternatively, the static
    setDefaultMouse() function provides means of specifying the mouse
    driver to use if the QWS_MOUSE_PROTO variable is not defined (note
    that the default is otherwise platform dependent). The primary
    mouse driver can be retrieved using the static mouseHandler()
    function. Use the closeMouse() function to delete the mouse
    drivers.

    In addition, the QWSServer class can control the flow of mouse
    input using the suspendMouse() and resumeMouse() functions.

    See also: QWSMouseHandler and \l{Qt for Embedded Linux Pointer Handling}.

    \section1 Keyboard Handling

    The keyboard driver (represented by an instance of the
    QWSKeyboardHandler class) is loaded by the server application when
    it starts running, using Qt's \l {How to Create Qt Plugins}{plugin
    system}. A keyboard driver receives keyboard events from the
    device and encapsulates each event with an instance of the
    QWSEvent class which it then passes to the server.

    The openKeyboard() function opens the keyboard devices specified
    by the QWS_KEYBOARD environment variable, and the
    setKeyboardHandler() functions sets the primary keyboard
    driver. Alternatively, the static setDefaultKeyboard() function
    provides means of specifying the keyboard driver to use if the
    QWS_KEYBOARD variable is not defined (note again that the default
    is otherwise platform dependent). The primary keyboard driver can
    be retrieved using the static keyboardHandler() function. Use the
    closeKeyboard() function to delete the keyboard drivers.

    In addition, the QWSServer class can handle key events from both
    physical and virtual keyboards using the processKeyEvent() and
    sendKeyEvent() functions, respectively. Use the
    addKeyboardFilter() function to filter the key events from
    physical keyboard drivers, the most recently added filter can be
    removed and deleted using the removeKeyboardFilter() function.

    See also: QWSKeyboardHandler and \l{Qt for Embedded Linux Character Input}.

    \section1 Display Handling

    When a screen update is required, the server runs through all the
    top-level windows that intersect with the region that is about to
    be updated, and ensures that the associated clients have updated
    their memory buffer. Then the server uses the screen driver
    (represented by an instance of the QScreen class) to copy the
    content of the memory to the screen.

    In addition, the QWSServer class provides some means of managing
    the screen output: Use the refresh() function to refresh the
    entire display, or alternatively a specified region of it. The
    enablePainting() function can be used to disable (and enable)
    painting onto the screen. QWSServer also provide the
    setMaxWindowRect() function restricting the area of the screen
    which \l{Qt for Embedded Linux} applications will consider to be the
    maximum area to use for windows. To set the brush used as the
    background in the absence of obscuring windows, QWSServer provides
    the static setBackground() function. The corresponding
    backgroundBrush() function returns the currently set brush.

    QWSServer also controls the screen saver: Use the setScreenSaver()
    to install a custom screen saver derived from the QWSScreenSaver
    class. Once installed, the screensaver can be activated using the
    screenSaverActivate() function, and the screenSaverActive()
    function returns its current status. Use the
    setScreenSaverInterval() function to specify the timeout interval.
    \l{Qt for Embedded Linux} also supports multilevel screen saving, use the
    setScreenSaverIntervals() function to specify the various levels
    and their timeout intervals.

    Finally, the QWSServer class controls the cursor's appearance,
    i.e., use the setCursorVisible() function to hide or show the
    cursor, and the isCursorVisible() function to determine whether
    the cursor is visible on the display or not.

    See also: QScreen and \l{Qt for Embedded Linux Display Management}.

    \section1 Input Method Handling

    Whenever the server receives an event, it queries its stack of
    top-level windows to find the window containing the event's
    position (each window can identify the client application that
    created it). Then the server forwards the event to the appropriate
    client. If an input method is installed, it is used as a filter
    between the server and the client application.

    Derive from the QWSInputMethod class to create custom input
    methods, and use the server's setCurrentInputMethod() function to
    install it. Use the sendIMEvent() and sendIMQuery() functions to
    send input method events and queries.

    QWSServer provides the IMMouse enum describing the various mouse
    events recognized by the QWSInputMethod::mouseHandler()
    function. The latter function allows subclasses of QWSInputMethod
    to handle mouse events within the preedit text.

    \sa QWSInputMethod
*/

/*!
    \enum QWSServer::IMState
    \obsolete

    This enum describes the various states of an input method.

    \value IMCompose Composing.
    \value IMStart Equivalent to IMCompose.
    \value IMEnd Finished composing.

    \sa QWSInputMethod::sendIMEvent()
*/

/*!
    \enum QWSServer::IMMouse

    This enum describes the various types of mouse events recognized
    by the QWSInputMethod::mouseHandler() function.

    \value MousePress An event generated by pressing a mouse button.
    \value MouseRelease An event generated by relasing a mouse button.
    \value MouseMove An event generated by moving the mouse cursor.
    \value MouseOutside This value is only reserved, i.e., it is not used in
                                    current implementations.

    \sa QWSInputMethod, setCurrentInputMethod()
*/

/*!
    \enum QWSServer::ServerFlags
    \internal

    This enum is used to pass various options to the window system
    server.

    \value DisableKeyboard Ignore all keyboard input.
    \value DisableMouse Ignore all mouse input.
*/

/*!
    \enum QWSServer::WindowEvent

    This enum specifies the various events that can occur in a
    top-level window.

    \value Create A new window has been created (by the QWidget constructor).
    \value Destroy The window has been closed and deleted (by the QWidget destructor).
    \value Hide The window has been hidden using the QWidget::hide() function.
    \value Show The window has been shown using the QWidget::show() function or similar.
    \value Raise The window has been raised to the top of the desktop.
    \value Lower The window has been lowered.
    \value Geometry The window has changed size or position.
    \value Active The window has become the active window (i.e., it has keyboard focus).
    \value Name The window has been named.

    \sa windowEvent()
*/

/*!
    \fn void QWSServer::markedText(const QString &selection)

    This signal is emitted whenever some text is selected in any of
    the running applications, passing the selected text in the \a
    selection parameter.

    \sa windowEvent()
*/

/*!
    \fn const QList<QWSWindow*> &QWSServer::clientWindows()

    Returns the list of current top-level windows.

    Note that the collection of top-level windows changes as
    applications add and remove widgets so it should not be stored for
    future use. The windows are sorted in stacking order from top-most
    to bottom-most.

    Use the QWSWindow::client() function to retrieve the client
    application that owns a given window.

    \sa windowAt(), instance()
*/

/*!
    \fn void QWSServer::newChannel(const QString& channel)

    This signal is emitted whenever a new QCopChannel object is
    created, passing the channel's name in the \a channel parameter.

    \sa removedChannel()
*/

/*!
    \fn void QWSServer::removedChannel(const QString& channel)

    This signal is emitted immediately after the given the QCopChannel
    object specified by \a channel, is destroyed.

    Note that a channel is not destroyed until all its listeners have
    been unregistered.

    \sa newChannel()
*/

/*!
    \fn QWSServer::QWSServer(int flags, QObject *parent)
    \internal

    Construct a QWSServer object with the given \a parent.  The \a
    flags are used for keyboard and mouse settings.

    \warning This class is instantiated by QApplication for
    \l{Qt for Embedded Linux} server processes. You should never construct
    this class yourself.

    \sa {Running Applications}
*/

/*!
    \fn static QWSServer* QWSServer::instance()
    \since 4.2

    Returns a pointer to the server instance.

    Note that the pointer will be 0 if the application is not the
    server, i.e., if the QApplication::type() function doesn't return
    QApplication::GuiServer.

    \sa clientWindows(), windowAt()
*/

struct QWSCommandStruct
{
    QWSCommandStruct(QWSCommand *c, QWSClient *cl) :command(c),client(cl){}
    ~QWSCommandStruct() { delete command; }

    QWSCommand *command;
    QWSClient *client;

};

QWSServer::QWSServer(int flags, QObject *parent) :
    QObject(*new QWSServerPrivate, parent)
{
    Q_D(QWSServer);
    QT_TRY {
        d->initServer(flags);
    } QT_CATCH(...) {
        qwsServer = 0;
        qwsServerPrivate = 0;
        QT_RETHROW;
    }
}

#ifdef QT3_SUPPORT
/*!
    Use the two-argument overload and call the
    QObject::setObjectName() function instead.
*/
QWSServer::QWSServer(int flags, QObject *parent, const char *name) :
    QObject(*new QWSServerPrivate, parent)
{
    Q_D(QWSServer);
    setObjectName(QString::fromAscii(name));
    d->initServer(flags);
}
#endif


#ifndef QT_NO_QWS_MULTIPROCESS
static void ignoreSignal(int) {} // Used to eat SIGPIPE signals below
#endif

bool QWSServerPrivate::screensaverblockevent( int index, int *screensaverinterval, bool isDown )
{
    static bool ignoreEvents[2] = { false, false };
    if ( isDown ) {
        if ( !ignoreEvents[index] ) {
            bool wake = false;
            if ( screensaverintervals ) {
                if ( screensaverinterval != screensaverintervals ) {
                    wake = true;
                }
            }
            if ( screensaverblockevents && wake ) {
#ifdef EVENT_BLOCK_DEBUG
                qDebug( "waking the screen" );
#endif
                ignoreEvents[index] = true;
            } else if ( !screensaverblockevents ) {
#ifdef EVENT_BLOCK_DEBUG
                qDebug( "the screen was already awake" );
#endif
                ignoreEvents[index] = false;
            }
        }
    } else {
        if ( ignoreEvents[index] ) {
#ifdef EVENT_BLOCK_DEBUG
            qDebug( "mouseup?" );
#endif
            ignoreEvents[index] = false;
            return true;
        }
    }
    return ignoreEvents[index];
}

void QWSServerPrivate::initServer(int flags)
{
    Q_Q(QWSServer);
    Q_ASSERT(!qwsServer);
    qwsServer = q;
    qwsServerPrivate = this;
    disablePainting = false;
#ifndef QT_NO_QWS_MULTIPROCESS
    ssocket = new QWSServerSocket(qws_qtePipeFilename(), q);
    QObject::connect(ssocket, SIGNAL(newConnection()), q, SLOT(_q_newConnection()));

    if ( !ssocket->isListening()) {
        perror("QWSServerPrivate::initServer: server socket not listening");
        qFatal("Failed to bind to %s", qws_qtePipeFilename().toLatin1().constData());
    }

    struct linger tmp;
    tmp.l_onoff=1;
    tmp.l_linger=0;
    setsockopt(ssocket->socketDescriptor(),SOL_SOCKET,SO_LINGER,(char *)&tmp,sizeof(tmp));


    signal(SIGPIPE, ignoreSignal); //we get it when we read
#endif
    focusw = 0;
    mouseGrabber = 0;
    mouseGrabbing = false;
    inputMethodMouseGrabbed = false;
    keyboardGrabber = 0;
    keyboardGrabbing = false;
#ifndef QT_NO_QWS_CURSOR
    haveviscurs = false;
    cursor = 0;
    nextCursor = 0;
#endif

#ifndef QT_NO_QWS_MULTIPROCESS

    if (!geteuid()) {
#if defined(Q_OS_LINUX) && !defined(QT_LINUXBASE)
        if(mount(0,"/var/shm", "shm", 0, 0)) {
            /* This just confuses people with 2.2 kernels
            if (errno != EBUSY)
                qDebug("Failed mounting shm fs on /var/shm: %s",strerror(errno));
            */
        }
#endif
    }
#endif

    // no selection yet
    selectionOwner.windowid = -1;
    selectionOwner.time.set(-1, -1, -1, -1);

    cleanupFontsDir();

    // initialize the font database
    // from qfontdatabase_qws.cpp
    extern void qt_qws_init_fontdb();
    qt_qws_init_fontdb();

    openDisplay();

    screensavertimer = new QTimer(q);
    screensavertimer->setSingleShot(true);
    QObject::connect(screensavertimer, SIGNAL(timeout()), q, SLOT(_q_screenSaverTimeout()));
    _q_screenSaverWake();

    clientMap[-1] = new QWSClient(q, 0, 0);

    if (!bgBrush)
        bgBrush = new QBrush(QColor(0x20, 0xb0, 0x50));

    initializeCursor();

    // input devices
    if (!(flags&QWSServer::DisableMouse)) {
        q->openMouse();
    }
#ifndef QT_NO_QWS_KEYBOARD
    if (!(flags&QWSServer::DisableKeyboard)) {
        q->openKeyboard();
    }
#endif

#if !defined(QT_NO_SOUND) && !defined(QT_EXTERNAL_SOUND_SERVER) && !defined(Q_OS_DARWIN)
    soundserver = new QWSSoundServer(q);
#endif
}

/*!
    \internal
    Destructs this server.
*/
QWSServer::~QWSServer()
{
    closeMouse();
#ifndef QT_NO_QWS_KEYBOARD
    closeKeyboard();
#endif
    d_func()->cleanupFonts(/*force =*/true);
}

/*!
  \internal
 */
void QWSServer::timerEvent(QTimerEvent *e)
{
    Q_D(QWSServer);
    if (e->timerId() == d->fontCleanupTimer.timerId()) {
        d->cleanupFonts();
        d->fontCleanupTimer.stop();
    } else {
        QObject::timerEvent(e);
    }
}

const QList<QWSWindow*> &QWSServer::clientWindows()
{
    Q_D(QWSServer);
    return d->windows;
}

/*!
  \internal
*/
void QWSServerPrivate::releaseMouse(QWSWindow* w)
{
    if (w && mouseGrabber == w) {
        mouseGrabber = 0;
        mouseGrabbing = false;
#ifndef QT_NO_QWS_CURSOR
        if (nextCursor) {
            // Not grabbing -> set the correct cursor
            setCursor(nextCursor);
            nextCursor = 0;
        }
#endif
    }
}

/*!
  \internal
*/
void QWSServerPrivate::releaseKeyboard(QWSWindow* w)
{
    if (keyboardGrabber == w) {
        keyboardGrabber = 0;
        keyboardGrabbing = false;
    }
}

void QWSServerPrivate::handleWindowClose(QWSWindow *w)
{
    w->shuttingDown();
    if (focusw == w)
        setFocus(w,false);
    if (mouseGrabber == w)
        releaseMouse(w);
    if (keyboardGrabber == w)
        releaseKeyboard(w);
}


#ifndef QT_NO_QWS_MULTIPROCESS
/*!
  \internal
*/
void QWSServerPrivate::_q_newConnection()
{
    Q_Q(QWSServer);
    while (QWS_SOCK_BASE *sock = ssocket->nextPendingConnection()) {
        int socket = sock->socketDescriptor();
        sock->setParent(0);

        QWSClient *client = new QWSClient(q,sock, get_object_id());
        clientMap[socket] = client;

#ifndef QT_NO_SXE
#ifdef QTRANSPORTAUTH_DEBUG
        qDebug( "Transport auth connected: unix stream socket %d", socket );
#endif
        // get a handle to the per-process authentication service
        QTransportAuth *a = QTransportAuth::getInstance();

        // assert that this transport is trusted
        QTransportAuth::Data *d = a->connectTransport(
                QTransportAuth::UnixStreamSock |
                QTransportAuth::Trusted, socket );

        QAuthDevice *ad = a->recvBuf( d, sock );
        ad->setClient(client);

        QObject::connect(ad, SIGNAL(readyRead()),
                q, SLOT(_q_doClient()));

        QObject::connect(client, SIGNAL(connectionClosed()),
                q, SLOT(_q_clientClosed()));
#else
        QObject::connect(client, SIGNAL(readyRead()),
                         q, SLOT(_q_doClient()));
        QObject::connect(client, SIGNAL(connectionClosed()),
                         q, SLOT(_q_clientClosed()));
#endif // QT_NO_SXE

        client->sendConnectedEvent(qws_display_spec.constData());

        if (clientMap.contains(socket)) {
            QList<QScreen*> screens = qt_screen->subScreens();
            if (screens.isEmpty())
                screens.append(qt_screen);
            for (int i = 0; i < screens.size(); ++i) {
                const QApplicationPrivate *ap = QApplicationPrivate::instance();
                QScreen *screen = screens.at(i);
                const QRect rect = ap->maxWindowRect(screen);
                if (!rect.isEmpty())
                    client->sendMaxWindowRectEvent(rect);
                if (screen->isTransformed()) {
                    QWSScreenTransformationEvent event;
                    event.simpleData.screen = i;
                    event.simpleData.transformation = screen->transformOrientation();
                    client->sendEvent(&event);
                }
            }
        }

        // pre-provide some object id's
        QWSCreateCommand cmd(30);
        invokeCreate(&cmd, client);
    }
}
/*!
  \internal
*/
void QWSServerPrivate::_q_clientClosed()
{
    Q_Q(QWSServer);
    QWSClient* cl = (QWSClient*)q->sender();

    // Remove any queued commands for this client
    int i = 0;
    while (i < commandQueue.size()) {
        QWSCommandStruct *cs = commandQueue.at(i);
        if (cs->client == cl) {
            commandQueue.removeAt(i);
            delete cs;
        } else {
            ++i;
        }
    }

#ifndef QT_NO_COP
    // Enfore unsubscription from all channels.
    QCopChannel::detach(cl);
#endif

    // Shut down all windows for this client
    for (int i = 0; i < windows.size(); ++i) {
        QWSWindow* w = windows.at(i);
        if (w->forClient(cl))
            w->shuttingDown();
    }

    // Delete all windows for this client
    QRegion exposed;
    i = 0;
    while (i < windows.size()) {
        QWSWindow* w = windows.at(i);
        if (w->forClient(cl)) {
            windows.takeAt(i);
            w->c = 0; //so we don't send events to it anymore
            releaseMouse(w);
            releaseKeyboard(w);
            exposed += w->allocatedRegion();
//                rgnMan->remove(w->allocationIndex());
            if (focusw == w)
                setFocus(focusw,0);
            if (mouseGrabber == w)
                releaseMouse(w);
            if (i < nReserved)
                --nReserved;
#ifndef QT_NO_QWS_PROPERTIES
            propertyManager.removeProperties(w->winId());
#endif
            emit q->windowEvent(w, QWSServer::Destroy);
            w->d->state = QWSWindow::Destroyed; //???
            deletedWindows.append(w);
        } else {
            ++i;
        }
    }
    if (deletedWindows.count())
        QTimer::singleShot(0, q, SLOT(_q_deleteWindowsLater()));

    QWSClientPrivate *clientPrivate = cl->d_func();
    if (!clientPrivate->shutdown) {
#if defined(QWS_DEBUG_FONTCLEANUP)
        qDebug() << "client" << cl->clientId() << "crashed";
#endif
        // this would be the place to emit a signal to notify about the
        // crash of a client
        crashedClientIds.append(cl->clientId());
        fontCleanupTimer.start(10, q_func());
    }
    clientPrivate->shutdown = true;

    while (!clientPrivate->usedFonts.isEmpty()) {
        const QByteArray font = *clientPrivate->usedFonts.begin();
#if defined(QWS_DEBUG_FONTCLEANUP)
        qDebug() << "dereferencing font" << font << "from disconnected client";
#endif
        dereferenceFont(clientPrivate, font);
    }
    clientPrivate->usedFonts.clear();

    //qDebug("removing client %d with socket %d", cl->clientId(), cl->socket());
    clientMap.remove(cl->socket());
    if (cl == cursorClient)
        cursorClient = 0;
    if (qt_screen->clearCacheFunc)
        (qt_screen->clearCacheFunc)(qt_screen, cl->clientId());  // remove any remaining cache entries.
    cl->deleteLater();

    update_regions();
    exposeRegion(exposed);
}

void QWSServerPrivate::_q_deleteWindowsLater()
{
    qDeleteAll(deletedWindows);
    deletedWindows.clear();
}

#endif //QT_NO_QWS_MULTIPROCESS

void QWSServerPrivate::referenceFont(QWSClientPrivate *client, const QByteArray &font)
{
    if (!client->usedFonts.contains(font)) {
        client->usedFonts.insert(font);

        ++fontReferenceCount[font];
#if defined(QWS_DEBUG_FONTCLEANUP)
        qDebug() << "Client" << client->q_func()->clientId() << "added font" << font;
        qDebug() << "Refcount is" << fontReferenceCount[font];
#endif
    }
}

void QWSServerPrivate::dereferenceFont(QWSClientPrivate *client, const QByteArray &font)
{
    if (client->usedFonts.contains(font)) {
        client->usedFonts.remove(font);

        Q_ASSERT(fontReferenceCount[font]);
        if (!--fontReferenceCount[font] && !fontCleanupTimer.isActive())
            fontCleanupTimer.start(FontCleanupInterval, q_func());

#if defined(QWS_DEBUG_FONTCLEANUP)
        qDebug() << "Client" << client->q_func()->clientId() << "removed font" << font;
        qDebug() << "Refcount is" << fontReferenceCount[font];
#endif
    }
}

static void cleanupFontsDir()
{
    static bool dontDelete = !qgetenv("QWS_KEEP_FONTS").isEmpty();
    if (dontDelete)
        return;

    extern QString qws_fontCacheDir();
    QDir dir(qws_fontCacheDir(), QLatin1String("*.qsf"));
    for (uint i = 0; i < dir.count(); ++i) {
#if defined(QWS_DEBUG_FONTCLEANUP)
        qDebug() << "removing stale font file" << dir[i];
#endif
        dir.remove(dir[i]);
    }
}

void QWSServerPrivate::cleanupFonts(bool force)
{
    static bool dontDelete = !qgetenv("QWS_KEEP_FONTS").isEmpty();
    if (dontDelete)
        return;

#if defined(QWS_DEBUG_FONTCLEANUP)
    qDebug() << "cleanupFonts()";
#endif
    if (!fontReferenceCount.isEmpty()) {
        QMap<QByteArray, int>::Iterator it = fontReferenceCount.begin();
        while (it != fontReferenceCount.end()) {
            if (it.value() && !force) {
                ++it;
                continue;
            }

            const QByteArray &fontName = it.key();
#if defined(QWS_DEBUG_FONTCLEANUP)
            qDebug() << "removing unused font file" << fontName;
#endif
            QT_TRY {
                QFile::remove(QFile::decodeName(fontName));
                sendFontRemovedEvent(fontName);

                it = fontReferenceCount.erase(it);
            } QT_CATCH(...) {
                // so we were not able to remove the font.
                // don't be angry and just continue with the next ones.
                ++it;
            }
        }
    }

    if (crashedClientIds.isEmpty())
        return;

    QList<QByteArray> removedFonts;
#if !defined(QT_NO_QWS_QPF2) && !defined(QT_FONTS_ARE_RESOURCES)
    removedFonts = QFontEngineQPF::cleanUpAfterClientCrash(crashedClientIds);
#endif
    crashedClientIds.clear();

    for (int i = 0; i < removedFonts.count(); ++i)
        sendFontRemovedEvent(removedFonts.at(i));
}

void QWSServerPrivate::sendFontRemovedEvent(const QByteArray &font)
{
    QWSFontEvent event;
    event.simpleData.type = QWSFontEvent::FontRemoved;
    event.setData(font.constData(), font.length(), false);

    QMap<int,QWSClient*>::const_iterator it = clientMap.constBegin();
    for (; it != clientMap.constEnd(); ++it)
        (*it)->sendEvent(&event);
}

/*!
   \internal
*/
QWSCommand* QWSClient::readMoreCommand()
{
#ifndef QT_NO_QWS_MULTIPROCESS
    QIODevice *socket = 0;
#endif
#ifndef QT_NO_SXE
    if (socketDescriptor != -1)  // not server socket
        socket = QTransportAuth::getInstance()->passThroughByClient( this );
#if QTRANSPORTAUTH_DEBUG
    if (socket) {
        char displaybuf[1024];
        qint64 bytes = socket->bytesAvailable();
        if ( bytes > 511 ) bytes = 511;
        hexstring( displaybuf, ((unsigned char *)(reinterpret_cast<QAuthDevice*>(socket)->buffer().constData())), bytes );
        qDebug( "readMoreCommand: %lli bytes - %s", socket->bytesAvailable(), displaybuf );
    }
#endif
#endif // QT_NO_SXE

#ifndef QT_NO_QWS_MULTIPROCESS
    if (!socket)
        socket = csocket;   // server socket
    if (socket) {
        // read next command
        if (!command) {
            int command_type = qws_read_uint(socket);

            if (command_type >= 0)
                command = QWSCommand::factory(command_type);
        }
        if (command) {
            if (command->read(socket)) {
                // Finished reading a whole command.
                QWSCommand* result = command;
                command = 0;
                return result;
            }
        }

        // Not finished reading a whole command.
        return 0;
    } else
#endif // QT_NO_QWS_MULTIPROCESS
    {
        QList<QWSCommand*> *serverQueue = qt_get_server_queue();
        return serverQueue->isEmpty() ? 0 : serverQueue->takeFirst();
    }
}


/*!
  \internal
*/
void QWSServer::processEventQueue()
{
    if (qwsServerPrivate)
        qwsServerPrivate->doClient(qwsServerPrivate->clientMap.value(-1));
}


#ifndef QT_NO_QWS_MULTIPROCESS
void QWSServerPrivate::_q_doClient()
{
    Q_Q(QWSServer);

    QWSClient* client;
#ifndef QT_NO_SXE
    QAuthDevice *ad = qobject_cast<QAuthDevice*>(q->sender());
    if (ad)
        client = (QWSClient*)ad->client();
    else
#endif
        client = (QWSClient*)q->sender();

    if (doClientIsActive) {
        pendingDoClients.append(client);
        return;
    }
    doClientIsActive = true;

    doClient(client);

    while (!pendingDoClients.isEmpty()) {
        doClient(pendingDoClients.takeFirst());
    }

    doClientIsActive = false;
}
#endif // QT_NO_QWS_MULTIPROCESS

void QWSServerPrivate::doClient(QWSClient *client)
{
    QWSCommand* command=client->readMoreCommand();

    while (command) {
        QWSCommandStruct *cs = new QWSCommandStruct(command, client);
        commandQueue.append(cs);
        // Try for some more...
        command=client->readMoreCommand();
    }

    while (!commandQueue.isEmpty()) {
        QWSCommandStruct *cs = commandQueue.takeAt(0);
        switch (cs->command->type) {
        case QWSCommand::Identify:
            invokeIdentify((QWSIdentifyCommand*)cs->command, cs->client);
            break;
        case QWSCommand::Create:
            invokeCreate((QWSCreateCommand*)cs->command, cs->client);
            break;
#ifndef QT_NO_QWS_MULTIPROCESS
        case QWSCommand::Shutdown:
            cs->client->d_func()->shutdown = true;
            break;
#endif
        case QWSCommand::RegionName:
            invokeRegionName((QWSRegionNameCommand*)cs->command, cs->client);
            break;
        case QWSCommand::Region:
            invokeRegion((QWSRegionCommand*)cs->command, cs->client);
            cs->client->d_func()->unlockCommunication();
            break;
        case QWSCommand::RegionMove:
            invokeRegionMove((QWSRegionMoveCommand*)cs->command, cs->client);
            cs->client->d_func()->unlockCommunication();
            break;
        case QWSCommand::RegionDestroy:
            invokeRegionDestroy((QWSRegionDestroyCommand*)cs->command, cs->client);
            break;
#ifndef QT_NO_QWS_PROPERTIES
        case QWSCommand::AddProperty:
            invokeAddProperty((QWSAddPropertyCommand*)cs->command);
            break;
        case QWSCommand::SetProperty:
            invokeSetProperty((QWSSetPropertyCommand*)cs->command);
            break;
        case QWSCommand::RemoveProperty:
            invokeRemoveProperty((QWSRemovePropertyCommand*)cs->command);
            break;
        case QWSCommand::GetProperty:
            invokeGetProperty((QWSGetPropertyCommand*)cs->command, cs->client);
            break;
#endif
        case QWSCommand::SetSelectionOwner:
            invokeSetSelectionOwner((QWSSetSelectionOwnerCommand*)cs->command);
            break;
        case QWSCommand::RequestFocus:
            invokeSetFocus((QWSRequestFocusCommand*)cs->command, cs->client);
            break;
        case QWSCommand::ChangeAltitude:
            invokeSetAltitude((QWSChangeAltitudeCommand*)cs->command,
                               cs->client);
            cs->client->d_func()->unlockCommunication();
            break;
        case QWSCommand::SetOpacity:
            invokeSetOpacity((QWSSetOpacityCommand*)cs->command,
                               cs->client);
            break;

#ifndef QT_NO_QWS_CURSOR
        case QWSCommand::DefineCursor:
            invokeDefineCursor((QWSDefineCursorCommand*)cs->command, cs->client);
            break;
        case QWSCommand::SelectCursor:
            invokeSelectCursor((QWSSelectCursorCommand*)cs->command, cs->client);
            break;
        case QWSCommand::PositionCursor:
            invokePositionCursor((QWSPositionCursorCommand*)cs->command, cs->client);
            break;
#endif
        case QWSCommand::GrabMouse:
            invokeGrabMouse((QWSGrabMouseCommand*)cs->command, cs->client);
            break;
        case QWSCommand::GrabKeyboard:
            invokeGrabKeyboard((QWSGrabKeyboardCommand*)cs->command, cs->client);
            break;
#if !defined(QT_NO_SOUND) && !defined(Q_OS_DARWIN)
        case QWSCommand::PlaySound:
            invokePlaySound((QWSPlaySoundCommand*)cs->command, cs->client);
            break;
#endif
#ifndef QT_NO_COP
        case QWSCommand::QCopRegisterChannel:
            invokeRegisterChannel((QWSQCopRegisterChannelCommand*)cs->command,
                                   cs->client);
            break;
        case QWSCommand::QCopSend:
            invokeQCopSend((QWSQCopSendCommand*)cs->command, cs->client);
            break;
#endif
#ifndef QT_NO_QWS_INPUTMETHODS
        case QWSCommand::IMUpdate:
            invokeIMUpdate((QWSIMUpdateCommand*)cs->command, cs->client);
            break;
        case QWSCommand::IMResponse:
            invokeIMResponse((QWSIMResponseCommand*)cs->command, cs->client);
            break;
        case QWSCommand::IMMouse:
            {
                if (current_IM) {
                    QWSIMMouseCommand *cmd = (QWSIMMouseCommand *) cs->command;
                    current_IM->mouseHandler(cmd->simpleData.index,
                                              cmd->simpleData.state);
                }
            }
            break;
#endif
        case QWSCommand::Font:
            invokeFont((QWSFontCommand *)cs->command, cs->client);
            break;
        case QWSCommand::RepaintRegion:
            invokeRepaintRegion((QWSRepaintRegionCommand*)cs->command,
                                cs->client);
            cs->client->d_func()->unlockCommunication();
            break;
#ifndef QT_NO_QWSEMBEDWIDGET
        case QWSCommand::Embed:
            invokeEmbed(static_cast<QWSEmbedCommand*>(cs->command),
                        cs->client);
            break;
#endif
        case QWSCommand::ScreenTransform:
            invokeScreenTransform(static_cast<QWSScreenTransformCommand*>(cs->command),
                                  cs->client);
            break;
        }
        delete cs;
    }
}


void QWSServerPrivate::showCursor()
{
#ifndef QT_NO_QWS_CURSOR
    if (qt_screencursor)
        qt_screencursor->show();
#endif
}

void QWSServerPrivate::hideCursor()
{
#ifndef QT_NO_QWS_CURSOR
    if (qt_screencursor)
        qt_screencursor->hide();
#endif
}

/*!
    \fn void QWSServer::enablePainting(bool enable)

    Enables painting onto the screen if \a enable is true; otherwise
    painting is disabled.

    \sa {Qt for Embedded Linux Architecture#Drawing on Screen}{Qt for Embedded Linux
    Architecture}
*/
void QWSServer::enablePainting(bool enable)
{
    Q_D(QWSServer);

    if (d->disablePainting == !enable)
        return;

    d->disablePainting = !enable;

    if (enable) {
        // Reset the server side allocated regions to ensure update_regions()
        // will send out region events.
        for (int i = 0; i < d->windows.size(); ++i) {
            QWSWindow *w = d->windows.at(i);
            w->setAllocatedRegion(QRegion());
#ifdef QT_QWS_CLIENTBLIT
            w->setDirectPaintRegion(QRegion());
#endif
        }
        d->update_regions();
        d->showCursor();
    } else {
        // Disable painting by clients by taking away their allocated region.
        // To ensure mouse events are still delivered to the correct windows,
        // the allocated regions are not modified on the server.
        for (int i = 0; i < d->windows.size(); ++i) {
            QWSWindow *w = d->windows.at(i);
            w->client()->sendRegionEvent(w->winId(), QRegion(),
                                         QWSRegionEvent::Allocation);
#ifdef QT_QWS_CLIENTBLIT
            w->client()->sendRegionEvent(w->winId(), QRegion(),
                                         QWSRegionEvent::DirectPaint);
#endif
        }
        d->hideCursor();
    }
}

/*!
    Refreshes the display by making the screen driver update the
    entire display.

    \sa QScreen::exposeRegion()
*/
void QWSServer::refresh()
{
    Q_D(QWSServer);
    d->exposeRegion(QScreen::instance()->region());
//### send repaint to non-buffered windows
}

/*!
    \fn void QWSServer::refresh(QRegion & region)
    \overload

    Refreshes the given \a region of the display.
*/
void QWSServer::refresh(QRegion & r)
{
    Q_D(QWSServer);
    d->exposeRegion(r);
//### send repaint to non-buffered windows
}

/*!
    \fn void QWSServer::setMaxWindowRect(const QRect& rectangle)

    Sets the maximum area of the screen that \l{Qt for Embedded Linux}
    applications can use, to be the given \a rectangle.

    Note that this function can only be used in the server process.

    \sa QWidget::showMaximized()
*/
void QWSServer::setMaxWindowRect(const QRect &rect)
{
    QList<QScreen*> subScreens = qt_screen->subScreens();
    if (subScreens.isEmpty() && qt_screen != 0)
        subScreens.append(qt_screen);

    for (int i = 0; i < subScreens.size(); ++i) {
        const QScreen *screen = subScreens.at(i);
        const QRect r = (screen->region() & rect).boundingRect();
        if (r.isEmpty())
            continue;

        QApplicationPrivate *ap = QApplicationPrivate::instance();
        if (ap->maxWindowRect(screen) != r) {
            ap->setMaxWindowRect(screen, i, r);
            qwsServerPrivate->sendMaxWindowRectEvents(r);
        }
    }
}

/*!
  \internal
*/
void QWSServerPrivate::sendMaxWindowRectEvents(const QRect &rect)
{
    QMap<int,QWSClient*>::const_iterator it = clientMap.constBegin();
    for (; it != clientMap.constEnd(); ++it)
        (*it)->sendMaxWindowRectEvent(rect);
}

/*!
    \fn void QWSServer::setDefaultMouse(const char *mouseDriver)

    Sets the mouse driver that will be used if the QWS_MOUSE_PROTO
    environment variable is not defined, to be the given \a
    mouseDriver.

    Note that the default is platform-dependent. This function can
    only be used in the server process.


    \sa setMouseHandler(), {Qt for Embedded Linux Pointer Handling}
*/
void QWSServer::setDefaultMouse(const char *m)
{
    *defaultMouse() = QString::fromAscii(m);
}

/*!
    \fn void QWSServer::setDefaultKeyboard(const char *keyboardDriver)

    Sets the keyboard driver that will be used if the QWS_KEYBOARD
    environment variable is not defined, to be the given \a
    keyboardDriver.

    Note that the default is platform-dependent. This function can
    only be used in the server process.

    \sa setKeyboardHandler(), {Qt for Embedded Linux Character Input}
*/
void QWSServer::setDefaultKeyboard(const char *k)
{
    *defaultKeyboard() = QString::fromAscii(k);
}

#ifndef QT_NO_QWS_CURSOR
static bool prevWin;
#endif


extern int *qt_last_x,*qt_last_y;


/*!
  \internal

  Send a mouse event. \a pos is the screen position where the mouse
  event occurred and \a state is a mask indicating which buttons are
  pressed.

  \a pos is in device coordinates
*/
void QWSServer::sendMouseEvent(const QPoint& pos, int state, int wheel)
{
    bool block = qwsServerPrivate->screensaverblockevent(MOUSE, qwsServerPrivate->screensaverinterval, state);
#ifdef EVENT_BLOCK_DEBUG
    qDebug() << "sendMouseEvent" << pos.x() << pos.y() << state << (block ? "block" : "pass");
#endif

    if (state || wheel)
        qwsServerPrivate->_q_screenSaverWake();

    if ( block )
        return;

    QPoint tpos;
    // transformations
    if (qt_screen->isTransformed()) {
	QSize s = QSize(qt_screen->deviceWidth(), qt_screen->deviceHeight());
	tpos = qt_screen->mapFromDevice(pos, s);
    } else {
	tpos = pos;
    }

    if (qt_last_x) {
         *qt_last_x = tpos.x();
         *qt_last_y = tpos.y();
    }
    QWSServer::mousePosition = tpos;
    qwsServerPrivate->mouseState = state;

#ifndef QT_NO_QWS_INPUTMETHODS
    const int btnMask = Qt::LeftButton | Qt::RightButton | Qt::MidButton;
    int stroke_count; // number of strokes to keep shown.
    if (force_reject_strokeIM || !current_IM)
    {
	stroke_count = 0;
    } else {
	stroke_count = current_IM->filter(tpos, state, wheel);
    }

    if (stroke_count == 0) {
	if (state&btnMask)
	    force_reject_strokeIM = true;
        QWSServerPrivate::sendMouseEventUnfiltered(tpos, state, wheel);
    }
    // stop force reject after stroke ends.
    if (state&btnMask && force_reject_strokeIM)
	force_reject_strokeIM = false;
    // on end of stroke, force_rejct
    // and once a stroke is rejected, do not try again till pen is lifted
#else
    QWSServerPrivate::sendMouseEventUnfiltered(tpos, state, wheel);
#endif // end QT_NO_QWS_FSIM
}

void QWSServerPrivate::sendMouseEventUnfiltered(const QPoint &pos, int state, int wheel)
{
    const int btnMask = Qt::LeftButton | Qt::RightButton | Qt::MidButton;
    QWSMouseEvent event;

    QWSWindow *win = qwsServer->windowAt(pos);

    QWSClient *serverClient = qwsServerPrivate->clientMap.value(-1);
    QWSClient *winClient = win ? win->client() : 0;


    bool imMouse = false;
#ifndef QT_NO_QWS_INPUTMETHODS
    // check for input method window
    if (current_IM && current_IM_winId != -1) {
        QWSWindow *kbw = keyboardGrabber ? keyboardGrabber :
                         qwsServerPrivate->focusw;

        imMouse = kbw == win;
        if ( !imMouse ) {
            QWidget *target = winClient == serverClient ?
                              QApplication::widgetAt(pos) : 0;
            imMouse = target && (target->testAttribute(Qt::WA_InputMethodTransparent));
        }
    }
#endif

    //If grabbing window disappears, grab is still active until
    //after mouse release.
    if ( qwsServerPrivate->mouseGrabber && (!imMouse || qwsServerPrivate->inputMethodMouseGrabbed)) {
        win = qwsServerPrivate->mouseGrabber;
        winClient = win ? win->client() : 0;
    }
    event.simpleData.window = win ? win->id : 0;

#ifndef QT_NO_QWS_CURSOR
    if (qt_screencursor)
        qt_screencursor->move(pos.x(),pos.y());

    // Arrow cursor over desktop
    // prevWin remembers if the last event was over a window
    if (!win && prevWin) {
        if (!qwsServerPrivate->mouseGrabber)
            qwsServerPrivate->setCursor(QWSCursor::systemCursor(Qt::ArrowCursor));
        else
            qwsServerPrivate->nextCursor = QWSCursor::systemCursor(Qt::ArrowCursor);
        prevWin = false;
    }
    // reset prevWin
    if (win && !prevWin)
        prevWin = true;
#endif

    if ((state&btnMask) && !qwsServerPrivate->mouseGrabbing) {
        qwsServerPrivate->mouseGrabber = win;
        if (imMouse)
            qwsServerPrivate->inputMethodMouseGrabbed = true;
    }
    if (!(state&btnMask))
        qwsServerPrivate->inputMethodMouseGrabbed = false;

    event.simpleData.x_root=pos.x();
    event.simpleData.y_root=pos.y();
    event.simpleData.state=state | qws_keyModifiers;
    event.simpleData.delta = wheel;
    event.simpleData.time=qwsServerPrivate->timer.elapsed();

    static int oldstate = 0;

#ifndef QT_NO_QWS_INPUTMETHODS
    //tell the input method if we click on a different window that is not IM transparent
    bool isPress = state > oldstate;
    if (isPress && !imMouse && current_IM && current_IM_winId != -1)
        current_IM->mouseHandler(-1, QWSServer::MouseOutside);
#endif

    if (serverClient)
       serverClient->sendEvent(&event);
    if (winClient && winClient != serverClient)
       winClient->sendEvent(&event);

    if ( !imMouse ) {
        // Make sure that if we leave a window, that window gets one last mouse
        // event so that it knows the mouse has left.
        QWSClient *oldClient = qwsServer->d_func()->cursorClient;
        if (oldClient && oldClient != winClient && oldClient != serverClient) {
            event.simpleData.state = oldstate | qws_keyModifiers;
            oldClient->sendEvent(&event);
        }
    }

    oldstate = state;
    if ( !imMouse )
        qwsServer->d_func()->cursorClient = winClient;

    if (!(state&btnMask) && !qwsServerPrivate->mouseGrabbing)
        qwsServerPrivate->releaseMouse(qwsServerPrivate->mouseGrabber);
}

/*!
    Returns the primary mouse driver.

    Note that this function can only be used in the server process.

    \sa setMouseHandler(), openMouse(), closeMouse()
*/
QWSMouseHandler *QWSServer::mouseHandler()
{
    if (qwsServerPrivate->mousehandlers.empty())
        return 0;
    return qwsServerPrivate->mousehandlers.first();
}

/*!
    \since 4.5

    Returns list of all mouse handlers

    Note that this function can only be used in the server process.

    \sa mouseHandler(), setMouseHandler(), openMouse(), closeMouse()
*/
const QList<QWSMouseHandler*>& QWSServer::mouseHandlers()
{
    return qwsServerPrivate->mousehandlers;
}


// called by QWSMouseHandler constructor, not user code.
/*!
    \fn void QWSServer::setMouseHandler(QWSMouseHandler* driver)

    Sets the primary mouse driver to be the given \a driver.

    \l{Qt for Embedded Linux} provides several ready-made mouse drivers, and
    custom drivers are typically added using Qt's plugin
    mechanism. See the \l{Qt for Embedded Linux Pointer Handling} documentation
    for details.

    Note that this function can only be used in the server process.

    \sa mouseHandler(), setDefaultMouse()
*/
void QWSServer::setMouseHandler(QWSMouseHandler* mh)
{
    if (!mh)
        return;
    qwsServerPrivate->mousehandlers.removeAll(mh);
    qwsServerPrivate->mousehandlers.prepend(mh);
}

/*!
  \internal
  \obsolete
  Caller owns data in list, and must delete contents
*/
QList<QWSInternalWindowInfo*> * QWSServer::windowList()
{
    QList<QWSInternalWindowInfo*> * ret=new QList<QWSInternalWindowInfo*>;
    for (int i=0; i < qwsServerPrivate->windows.size(); ++i) {
        QWSWindow *window = qwsServerPrivate->windows.at(i);
        QWSInternalWindowInfo * qwi=new QWSInternalWindowInfo();
        qwi->winid=window->winId();
        qwi->clientid=window->client()->clientId();
        ret->append(qwi);
    }
    return ret;
}

#ifndef QT_NO_COP
/*!
  \internal
*/
void QWSServerPrivate::sendQCopEvent(QWSClient *c, const QString &ch,
                               const QString &msg, const QByteArray &data,
                               bool response)
{
    Q_ASSERT(c);

    QWSQCopMessageEvent event;
    event.channel = ch.toLatin1();
    event.message = msg.toLatin1();
    event.data = data;
    event.simpleData.is_response = response;
    event.simpleData.lchannel = ch.length();
    event.simpleData.lmessage = msg.length();
    event.simpleData.ldata = data.size();
    int l = event.simpleData.lchannel + event.simpleData.lmessage +
            event.simpleData.ldata;

    // combine channel, message and data into one block of raw bytes
    char *tmp = new char [l];
    char *d = tmp;
    memcpy(d, event.channel.constData(), event.simpleData.lchannel);
    d += event.simpleData.lchannel;
    memcpy(d, event.message.constData(), event.simpleData.lmessage);
    d += event.simpleData.lmessage;
    memcpy(d, data.constData(), event.simpleData.ldata);

    event.setDataDirect(tmp, l);

    c->sendEvent(&event);
}
#endif

/*!
    \fn QWSWindow *QWSServer::windowAt(const QPoint& position)

    Returns the window containing the given \a position.

    Note that if there is no window under the specified point this
    function returns 0.

    \sa clientWindows(), instance()
*/
QWSWindow *QWSServer::windowAt(const QPoint& pos)
{
    Q_D(QWSServer);
    for (int i=0; i<d->windows.size(); ++i) {
        QWSWindow* w = d->windows.at(i);
        if (w->allocatedRegion().contains(pos))
            return w;
    }
    return 0;
}

#ifndef QT_NO_QWS_KEYBOARD
static int keyUnicode(int keycode)
{
    int code = 0xffff;

    if (keycode >= Qt::Key_A && keycode <= Qt::Key_Z)
        code = keycode - Qt::Key_A + 'a';
    else if (keycode >= Qt::Key_0 && keycode <= Qt::Key_9)
        code = keycode - Qt::Key_0 + '0';

    return code;
}
#endif

/*!
    Sends the given key event. The key is identified by its \a unicode
    value and the given \a keycode, \a modifiers, \a isPress and \a
    autoRepeat parameters.

    Use this function to send key events generated by "virtual
    keyboards" (note that the processKeyEvent() function is
    impelemented using this function).

    The \a keycode parameter is the Qt keycode value as defined by the
    Qt::Key enum. The \a modifiers is an OR combination of
    Qt::KeyboardModifier values, indicating whether \gui
    Shift/Alt/Ctrl keys are pressed. The \a isPress parameter is true
    if the event is a key press event and \a autoRepeat is true if the
    event is caused by an auto-repeat mechanism and not an actual key
    press.

    Note that this function can only be used in the server process.

    \sa processKeyEvent(), {Qt for Embedded Linux Character Input}
*/
void QWSServer::sendKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                             bool isPress, bool autoRepeat)
{
    qws_keyModifiers = modifiers;

    if (isPress) {
        if (keycode != Qt::Key_F34 && keycode != Qt::Key_F35)
            qwsServerPrivate->_q_screenSaverWake();
    }

#ifndef QT_NO_QWS_INPUTMETHODS

    if (!current_IM || !current_IM->filter(unicode, keycode, modifiers, isPress, autoRepeat))
        QWSServerPrivate::sendKeyEventUnfiltered(unicode, keycode, modifiers, isPress, autoRepeat);
#else
    QWSServerPrivate::sendKeyEventUnfiltered(unicode, keycode, modifiers, isPress, autoRepeat);
#endif
}

void QWSServerPrivate::sendKeyEventUnfiltered(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                                       bool isPress, bool autoRepeat)
{

    QWSKeyEvent event;
    QWSWindow *win = keyboardGrabber ? keyboardGrabber :
        qwsServerPrivate->focusw;

    event.simpleData.window = win ? win->winId() : 0;

    event.simpleData.unicode =
#ifndef QT_NO_QWS_KEYBOARD
        unicode < 0 ? keyUnicode(keycode) :
#endif
        unicode;
    event.simpleData.keycode = keycode;
    event.simpleData.modifiers = modifiers;
    event.simpleData.is_press = isPress;
    event.simpleData.is_auto_repeat = autoRepeat;

    QWSClient *serverClient = qwsServerPrivate->clientMap.value(-1);
    QWSClient *winClient = win ? win->client() : 0;
    if (serverClient)
        serverClient->sendEvent(&event);
    if (winClient && winClient != serverClient)
        winClient->sendEvent(&event);
}

/*!
    \internal
*/
void QWSServer::beginDisplayReconfigure()
{
    qwsServer->enablePainting(false);
#ifndef QT_NO_QWS_CURSOR
    if (qt_screencursor)
        qt_screencursor->hide();
#endif
    QWSDisplay::grab(true);
    qt_screen->disconnect();
}

/*!
    \internal
*/
void QWSServer::endDisplayReconfigure()
{
    qt_screen->connect(QString());
    qwsServerPrivate->swidth = qt_screen->deviceWidth();
    qwsServerPrivate->sheight = qt_screen->deviceHeight();

    QWSDisplay::ungrab();
#ifndef QT_NO_QWS_CURSOR
    if (qt_screencursor)
        qt_screencursor->show();
#endif
    QApplicationPrivate *ap = QApplicationPrivate::instance();
    ap->setMaxWindowRect(qt_screen, 0,
                         QRect(0, 0, qt_screen->width(), qt_screen->height()));
    QSize olds = qApp->desktop()->size();
    qApp->desktop()->resize(qt_screen->width(), qt_screen->height());
    qApp->postEvent(qApp->desktop(), new QResizeEvent(qApp->desktop()->size(), olds));
    qwsServer->enablePainting(true);
    qwsServer->refresh();
    qDebug("Desktop size: %dx%d", qApp->desktop()->width(), qApp->desktop()->height());
}

void QWSServerPrivate::resetEngine()
{
#ifndef QT_NO_QWS_CURSOR
    if (!qt_screencursor)
        return;
    qt_screencursor->hide();
    qt_screencursor->show();
#endif
}


#ifndef QT_NO_QWS_CURSOR
/*!
    \fn void QWSServer::setCursorVisible(bool visible)

    Shows the cursor if \a visible is true: otherwise the cursor is
    hidden.

    Note that this function can only be used in the server process.

    \sa isCursorVisible()
*/
void QWSServer::setCursorVisible(bool vis)
{
    if (qwsServerPrivate && qwsServerPrivate->haveviscurs != vis) {
        QWSCursor* c = qwsServerPrivate->cursor;
        qwsServerPrivate->setCursor(QWSCursor::systemCursor(Qt::BlankCursor));
        qwsServerPrivate->haveviscurs = vis;
        qwsServerPrivate->setCursor(c);
    }
}

/*!
    Returns true if the cursor is visible; otherwise returns false.

    Note that this function can only be used in the server process.

    \sa setCursorVisible()
*/
bool QWSServer::isCursorVisible()
{
    return qwsServerPrivate ? qwsServerPrivate->haveviscurs : true;
}
#endif

#ifndef QT_NO_QWS_INPUTMETHODS


/*!
    \fn void QWSServer::sendIMEvent(const QInputMethodEvent *event)

    Sends the given input method \a event.

    The \c QInputMethodEvent class is derived from QWSEvent, i.e., it
    is a QWSEvent object of the QWSEvent::IMEvent type.

    If there is a window actively composing the preedit string, the
    event is sent to that window. Otherwise, the event is sent to the
    window currently in focus.

    \sa sendIMQuery(), QWSInputMethod::sendEvent()
*/
void QWSServer::sendIMEvent(const QInputMethodEvent *ime)
{
    QWSIMEvent event;

    QWSWindow *win = keyboardGrabber ? keyboardGrabber :
                     qwsServerPrivate->focusw;

    //if currently composing then event must go to the composing window

    if (current_IM_composing_win)
        win = current_IM_composing_win;

    event.simpleData.window = win ? win->winId() : 0;
    event.simpleData.replaceFrom = ime->replacementStart();;
    event.simpleData.replaceLength = ime->replacementLength();

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream out(&buffer);

    out << ime->preeditString();
    out << ime->commitString();

    const QList<QInputMethodEvent::Attribute> &attributes = ime->attributes();
    for (int i = 0; i < attributes.count(); ++i) {
        const QInputMethodEvent::Attribute &a = attributes.at(i);
        out << a.type << a.start << a.length << a.value;
    }
    event.setData(buffer.data(), buffer.size());
    QWSClient *serverClient = qwsServerPrivate->clientMap.value(-1);
    if (serverClient)
        serverClient->sendEvent(&event);
    if (win && win->client() && win->client() != serverClient)
        win->client()->sendEvent(&event);

    current_IM_composing_win = ime->preeditString().isEmpty() ? 0 : win;
    current_IM_winId = win ? win->winId() : 0;
}


/*!
    Sends an input method query for the given \a property.

    To receive responses to input method queries, the virtual
    QWSInputMethod::queryResponse() function must be reimplemented in
    a QWSInputMethod subclass that is activated using the
    setCurrentInputMethod() function.

    \sa sendIMEvent(), setCurrentInputMethod()
*/
void QWSServer::sendIMQuery(int property)
{
    QWSIMQueryEvent event;

    QWSWindow *win = keyboardGrabber ? keyboardGrabber :
                     qwsServerPrivate->focusw;
    if (current_IM_composing_win)
        win = current_IM_composing_win;

    event.simpleData.window = win ? win->winId() : 0;
    event.simpleData.property = property;
    if (win && win->client())
        win->client()->sendEvent(&event);
}



/*!
    \fn void QWSServer::setCurrentInputMethod(QWSInputMethod *method)

    Sets the current input method to be the given \a method.

    Note that this function can only be used in the server process.

    \sa sendIMQuery(), sendIMEvent()
*/
void QWSServer::setCurrentInputMethod(QWSInputMethod *im)
{
    if (current_IM)
        current_IM->reset(); //??? send an update event instead ?
    current_IM = im;
}

/*!
    \fn static void QWSServer::resetInputMethod()

    \internal
*/

#endif //QT_NO_QWS_INPUTMETHODS

#ifndef QT_NO_QWS_PROPERTIES
/*!
  \internal
*/
void QWSServer::sendPropertyNotifyEvent(int property, int state)
{
    Q_D(QWSServer);
    QWSServerPrivate::ClientIterator it = d->clientMap.begin();
    while (it != d->clientMap.end()) {
        QWSClient *cl = *it;
        ++it;
        cl->sendPropertyNotifyEvent(property, state);
    }
}
#endif

void QWSServerPrivate::invokeIdentify(const QWSIdentifyCommand *cmd, QWSClient *client)
{
    client->setIdentity(cmd->id);
#ifndef QT_NO_QWS_MULTIPROCESS
    if (client->clientId() > 0)
        client->d_func()->setLockId(cmd->simpleData.idLock);
#endif
}

void QWSServerPrivate::invokeCreate(QWSCreateCommand *cmd, QWSClient *client)
{
    QWSCreationEvent event;
    event.simpleData.objectid = get_object_id(cmd->count);
    event.simpleData.count = cmd->count;
    client->sendEvent(&event);
}

void QWSServerPrivate::invokeRegionName(const QWSRegionNameCommand *cmd, QWSClient *client)
{
    Q_Q(QWSServer);
    QWSWindow* changingw = findWindow(cmd->simpleData.windowid, client);
    if (changingw && (changingw->name() != cmd->name || changingw->caption() !=cmd->caption)) {
        changingw->setName(cmd->name);
        changingw->setCaption(cmd->caption);
        emit q->windowEvent(changingw, QWSServer::Name);
    }
}

void QWSServerPrivate::invokeRegion(QWSRegionCommand *cmd, QWSClient *client)
{
#ifdef QWS_REGION_DEBUG
    qDebug("QWSServer::invokeRegion %d rects (%d)",
            cmd->simpleData.nrectangles, cmd->simpleData.windowid);
#endif

    QWSWindow* changingw = findWindow(cmd->simpleData.windowid, 0);
    if (!changingw) {
        qWarning("Invalid window handle %08x",cmd->simpleData.windowid);
        return;
    }
    if (!changingw->forClient(client)) {
        qWarning("Disabled: clients changing other client's window region");
        return;
    }

    request_region(cmd->simpleData.windowid, cmd->surfaceKey, cmd->surfaceData,
                   cmd->region);
}

void QWSServerPrivate::invokeRegionMove(const QWSRegionMoveCommand *cmd, QWSClient *client)
{
    Q_Q(QWSServer);
    QWSWindow* changingw = findWindow(cmd->simpleData.windowid, 0);
    if (!changingw) {
        qWarning("invokeRegionMove: Invalid window handle %d",cmd->simpleData.windowid);
        return;
    }
    if (!changingw->forClient(client)) {
        qWarning("Disabled: clients changing other client's window region");
        return;
    }

//    changingw->setNeedAck(true);
    moveWindowRegion(changingw, cmd->simpleData.dx, cmd->simpleData.dy);
    emit q->windowEvent(changingw, QWSServer::Geometry);
}

void QWSServerPrivate::invokeRegionDestroy(const QWSRegionDestroyCommand *cmd, QWSClient *client)
{
    Q_Q(QWSServer);
    QWSWindow* changingw = findWindow(cmd->simpleData.windowid, 0);
    if (!changingw) {
        qWarning("invokeRegionDestroy: Invalid window handle %d",cmd->simpleData.windowid);
        return;
    }
    if (!changingw->forClient(client)) {
        qWarning("Disabled: clients changing other client's window region");
        return;
    }

    setWindowRegion(changingw, QRegion());
//    rgnMan->remove(changingw->allocationIndex());
    for (int i = 0; i < windows.size(); ++i) {
        if (windows.at(i) == changingw) {
            windows.takeAt(i);
            if (i < nReserved)
                --nReserved;
            break;
        }
    }

    handleWindowClose(changingw);
#ifndef QT_NO_QWS_PROPERTIES
    propertyManager.removeProperties(changingw->winId());
#endif
    emit q->windowEvent(changingw, QWSServer::Destroy);
    delete changingw;
}

void QWSServerPrivate::invokeSetFocus(const QWSRequestFocusCommand *cmd, QWSClient *client)
{
    int winId = cmd->simpleData.windowid;
    int gain = cmd->simpleData.flag;

    if (gain != 0 && gain != 1) {
        qWarning("Only 0(lose) and 1(gain) supported");
        return;
    }

    QWSWindow* changingw = findWindow(winId, 0);
    if (!changingw)
        return;

    if (!changingw->forClient(client)) {
       qWarning("Disabled: clients changing other client's focus");
        return;
    }

    setFocus(changingw, gain);
}

void QWSServerPrivate::setFocus(QWSWindow* changingw, bool gain)
{
    Q_Q(QWSServer);
#ifndef QT_NO_QWS_INPUTMETHODS
    /*
      This is the logic:
      QWSWindow *loser = 0;
      if (gain && focusw != changingw)
         loser = focusw;
      else if (!gain && focusw == changingw)
         loser = focusw;
      But these five lines can be reduced to one:
    */
    if (current_IM) {
        QWSWindow *loser =  (!gain == (focusw==changingw)) ? focusw : 0;
        if (loser && loser->winId() == current_IM_winId)
            current_IM->updateHandler(QWSInputMethod::FocusOut);
    }
#endif
    if (gain) {
        if (focusw != changingw) {
            if (focusw) focusw->focus(0);
            focusw = changingw;
            focusw->focus(1);
            emit q->windowEvent(focusw, QWSServer::Active);
        }
    } else if (focusw == changingw) {
        if (changingw->client())
            changingw->focus(0);
        focusw = 0;
        // pass focus to window which most recently got it...
        QWSWindow* bestw=0;
        for (int i=0; i<windows.size(); ++i) {
            QWSWindow* w = windows.at(i);
            if (w != changingw && !w->hidden() &&
                    (!bestw || bestw->focusPriority() < w->focusPriority()))
                bestw = w;
        }
        if (!bestw && changingw->focusPriority()) { // accept focus back?
            bestw = changingw; // must be the only one
        }
        focusw = bestw;
        if (focusw) {
            focusw->focus(1);
            emit q->windowEvent(focusw, QWSServer::Active);
        }
    }
}



void QWSServerPrivate::invokeSetOpacity(const QWSSetOpacityCommand *cmd, QWSClient *client)
{
    Q_UNUSED( client );
    int winId = cmd->simpleData.windowid;
    int opacity = cmd->simpleData.opacity;

    QWSWindow* changingw = findWindow(winId, 0);

    if (!changingw) {
        qWarning("invokeSetOpacity: Invalid window handle %d", winId);
        return;
    }

    int altitude = windows.indexOf(changingw);
    const bool wasOpaque = changingw->isOpaque();
    changingw->_opacity = opacity;
    if (wasOpaque != changingw->isOpaque())
        update_regions();
    exposeRegion(changingw->allocatedRegion(), altitude);
}

void QWSServerPrivate::invokeSetAltitude(const QWSChangeAltitudeCommand *cmd,
                                   QWSClient *client)
{
    Q_UNUSED(client);

    int winId = cmd->simpleData.windowid;
    int alt = cmd->simpleData.altitude;
    bool fixed = cmd->simpleData.fixed;
#if 0
    qDebug("QWSServer::invokeSetAltitude winId %d alt %d)", winId, alt);
#endif

    if (alt < -1 || alt > 1) {
        qWarning("QWSServer::invokeSetAltitude Only lower, raise and stays-on-top supported");
        return;
    }

    QWSWindow* changingw = findWindow(winId, 0);
    if (!changingw) {
        qWarning("invokeSetAltitude: Invalid window handle %d", winId);
        return;
    }

    if (fixed && alt >= 1) {
        changingw->onTop = true;
    }
    if (alt == QWSChangeAltitudeCommand::Lower)
        changingw->lower();
    else
        changingw->raise();

//      if (!changingw->forClient(client)) {
//         refresh();
//     }
}

#ifndef QT_NO_QWS_PROPERTIES
void QWSServerPrivate::invokeAddProperty(QWSAddPropertyCommand *cmd)
{
    propertyManager.addProperty(cmd->simpleData.windowid, cmd->simpleData.property);
}

void QWSServerPrivate::invokeSetProperty(QWSSetPropertyCommand *cmd)
{
    Q_Q(QWSServer);
    if (propertyManager.setProperty(cmd->simpleData.windowid,
                                    cmd->simpleData.property,
                                    cmd->simpleData.mode,
                                    cmd->data,
                                    cmd->rawLen)) {
        q->sendPropertyNotifyEvent(cmd->simpleData.property,
                                 QWSPropertyNotifyEvent::PropertyNewValue);
#ifndef QT_NO_QWS_INPUTMETHODS
        if (cmd->simpleData.property == QT_QWS_PROPERTY_MARKEDTEXT) {
            QString s((const QChar*)cmd->data, cmd->rawLen/2);
            emit q->markedText(s);
        }
#endif
    }
}

void QWSServerPrivate::invokeRemoveProperty(QWSRemovePropertyCommand *cmd)
{
    Q_Q(QWSServer);
    if (propertyManager.removeProperty(cmd->simpleData.windowid,
                                       cmd->simpleData.property)) {
        q->sendPropertyNotifyEvent(cmd->simpleData.property,
                                 QWSPropertyNotifyEvent::PropertyDeleted);
    }
}


bool QWSServerPrivate:: get_property(int winId, int property, const char *&data, int &len)
{
    return propertyManager.getProperty(winId, property, data, len);
}


void QWSServerPrivate::invokeGetProperty(QWSGetPropertyCommand *cmd, QWSClient *client)
{
    const char *data;
    int len;

    if (propertyManager.getProperty(cmd->simpleData.windowid,
                                    cmd->simpleData.property,
                                    data, len)) {
        client->sendPropertyReplyEvent(cmd->simpleData.property, len, data);
    } else {
        client->sendPropertyReplyEvent(cmd->simpleData.property, -1, 0);
    }
}
#endif //QT_NO_QWS_PROPERTIES

void QWSServerPrivate::invokeSetSelectionOwner(QWSSetSelectionOwnerCommand *cmd)
{
    qDebug("QWSServer::invokeSetSelectionOwner");

    SelectionOwner so;
    so.windowid = cmd->simpleData.windowid;
    so.time.set(cmd->simpleData.hour, cmd->simpleData.minute,
                 cmd->simpleData.sec, cmd->simpleData.ms);

    if (selectionOwner.windowid != -1) {
        QWSWindow *win = findWindow(selectionOwner.windowid, 0);
        if (win)
            win->client()->sendSelectionClearEvent(selectionOwner.windowid);
        else
            qDebug("couldn't find window %d", selectionOwner.windowid);
    }

    selectionOwner = so;
}

void QWSServerPrivate::invokeConvertSelection(QWSConvertSelectionCommand *cmd)
{
    qDebug("QWSServer::invokeConvertSelection");

    if (selectionOwner.windowid != -1) {
        QWSWindow *win = findWindow(selectionOwner.windowid, 0);
        if (win)
            win->client()->sendSelectionRequestEvent(cmd, selectionOwner.windowid);
        else
            qDebug("couldn't find window %d", selectionOwner.windowid);
    }
}

#ifndef QT_NO_QWS_CURSOR
void QWSServerPrivate::invokeDefineCursor(QWSDefineCursorCommand *cmd, QWSClient *client)
{
    if (cmd->simpleData.height > 64 || cmd->simpleData.width > 64) {
        qDebug("Cannot define cursor size > 64x64");
        return;
    }

    delete client->cursors.take(cmd->simpleData.id);

    int dataLen = cmd->simpleData.height * ((cmd->simpleData.width+7) / 8);

    if (dataLen > 0 && cmd->data) {
        QWSCursor *curs = new QWSCursor(cmd->data, cmd->data + dataLen,
                                        cmd->simpleData.width, cmd->simpleData.height,
                                        cmd->simpleData.hotX, cmd->simpleData.hotY);
        client->cursors.insert(cmd->simpleData.id, curs);
    }
}

void QWSServerPrivate::invokeSelectCursor(QWSSelectCursorCommand *cmd, QWSClient *client)
{
    int id = cmd->simpleData.id;
    QWSCursor *curs = 0;
    if (id <= Qt::LastCursor) {
        curs = QWSCursor::systemCursor(id);
    }
    else {
        QWSCursorMap cursMap = client->cursors;
        QWSCursorMap::Iterator it = cursMap.find(id);
        if (it != cursMap.end()) {
            curs = it.value();
        }
    }
    if (curs == 0) {
        curs = QWSCursor::systemCursor(Qt::ArrowCursor);
    }

    QWSWindow* win = findWindow(cmd->simpleData.windowid, 0);
    if (mouseGrabber) {
        // If the mouse is being grabbed, we don't want just anyone to
        // be able to change the cursor.  We do want the cursor to be set
        // correctly once mouse grabbing is stopped though.
        if (win != mouseGrabber)
            nextCursor = curs;
        else
            setCursor(curs);
    } else if (win && win->allocatedRegion().contains(QWSServer::mousePosition)) { //##################### cursor
        // A non-grabbing window can only set the cursor shape if the
        // cursor is within its allocated region.
        setCursor(curs);
    }
}

void QWSServerPrivate::invokePositionCursor(QWSPositionCursorCommand *cmd, QWSClient *)
{
    Q_Q(QWSServer);
    QPoint newPos(cmd->simpleData.newX, cmd->simpleData.newY);
    if (newPos != QWSServer::mousePosition)
        q->sendMouseEvent(newPos, qwsServer->d_func()->mouseState);
}
#endif

void QWSServerPrivate::invokeGrabMouse(QWSGrabMouseCommand *cmd, QWSClient *client)
{
    QWSWindow* win = findWindow(cmd->simpleData.windowid, 0);
    if (!win)
        return;

    if (cmd->simpleData.grab) {
        if (!mouseGrabber || mouseGrabber->client() == client) {
            mouseGrabbing = true;
            mouseGrabber = win;
        }
    } else {
        releaseMouse(mouseGrabber);
    }
}

void QWSServerPrivate::invokeGrabKeyboard(QWSGrabKeyboardCommand *cmd, QWSClient *client)
{
    QWSWindow* win = findWindow(cmd->simpleData.windowid, 0);
    if (!win)
        return;

    if (cmd->simpleData.grab) {
        if (!keyboardGrabber || (keyboardGrabber->client() == client)) {
            keyboardGrabbing = true;
            keyboardGrabber = win;
        }
    } else {
        releaseKeyboard(keyboardGrabber);
    }
}

#if !defined(QT_NO_SOUND)
void QWSServerPrivate::invokePlaySound(QWSPlaySoundCommand *cmd, QWSClient *)
{
#if !defined(QT_EXTERNAL_SOUND_SERVER) && !defined(Q_OS_DARWIN)
    soundserver->playFile( 1, cmd->filename );
#else
    Q_UNUSED(cmd);
#endif
}
#endif

#ifndef QT_NO_COP
void QWSServerPrivate::invokeRegisterChannel(QWSQCopRegisterChannelCommand *cmd,
                                       QWSClient *client)
{
  // QCopChannel will force us to emit the newChannel signal if this channel
  // didn't already exist.
  QCopChannel::registerChannel(cmd->channel, client);
}

void QWSServerPrivate::invokeQCopSend(QWSQCopSendCommand *cmd, QWSClient *client)
{
    QCopChannel::answer(client, cmd->channel, cmd->message, cmd->data);
}

#endif

#ifndef QT_NO_QWS_INPUTMETHODS
void QWSServer::resetInputMethod()
{
    if (current_IM && qwsServer) {
      current_IM->reset();
    }
}

void QWSServerPrivate::invokeIMResponse(const QWSIMResponseCommand *cmd,
                                 QWSClient *)
{
    if (current_IM)
        current_IM->queryResponse(cmd->simpleData.property, cmd->result);
}

void QWSServerPrivate::invokeIMUpdate(const QWSIMUpdateCommand *cmd,
                                 QWSClient *)
{
    if (cmd->simpleData.type == QWSInputMethod::FocusIn)
        current_IM_winId = cmd->simpleData.windowid;

    if (current_IM && (current_IM_winId == cmd->simpleData.windowid || cmd->simpleData.windowid == -1))
        current_IM->updateHandler(cmd->simpleData.type);
}

#endif

void QWSServerPrivate::invokeFont(const QWSFontCommand *cmd, QWSClient *client)
{
    QWSClientPrivate *priv = client->d_func();
    if (cmd->simpleData.type == QWSFontCommand::StartedUsingFont) {
        referenceFont(priv, cmd->fontName);
    } else if (cmd->simpleData.type == QWSFontCommand::StoppedUsingFont) {
        dereferenceFont(priv, cmd->fontName);
    }
}

void QWSServerPrivate::invokeRepaintRegion(QWSRepaintRegionCommand * cmd,
                                           QWSClient *)
{
    QRegion r;
    r.setRects(cmd->rectangles,cmd->simpleData.nrectangles);
    repaint_region(cmd->simpleData.windowid, cmd->simpleData.windowFlags, cmd->simpleData.opaque, r);
}

#ifndef QT_NO_QWSEMBEDWIDGET
void QWSServerPrivate::invokeEmbed(QWSEmbedCommand *cmd, QWSClient *client)
{
    // Should find these two windows in a single loop
    QWSWindow *embedder = findWindow(cmd->simpleData.embedder, client);
    QWSWindow *embedded = findWindow(cmd->simpleData.embedded);

    if (!embedder) {
        qWarning("QWSServer: Embed command from window %i failed: No such id.",
                 static_cast<int>(cmd->simpleData.embedder));
        return;
    }

    if (!embedded) {
        qWarning("QWSServer: Embed command on window %i failed: No such id.",
                 static_cast<int>(cmd->simpleData.embedded));
        return;
    }

    switch (cmd->simpleData.type) {
    case QWSEmbedEvent::StartEmbed:
        embedder->startEmbed(embedded);
        windows.removeAll(embedded);
        windows.insert(windows.indexOf(embedder), embedded);
        break;
    case QWSEmbedEvent::StopEmbed:
        embedder->stopEmbed(embedded);
        break;
    case QWSEmbedEvent::Region:
        break;
    }

    embedded->client()->sendEmbedEvent(embedded->winId(),
                                       cmd->simpleData.type, cmd->region);
    const QRegion oldAllocated = embedded->allocatedRegion();
    update_regions();
    exposeRegion(oldAllocated - embedded->allocatedRegion(),
                 windows.indexOf(embedded));
}
#endif // QT_NO_QWSEMBEDWIDGET

void QWSServerPrivate::invokeScreenTransform(const QWSScreenTransformCommand *cmd,
                                             QWSClient *client)
{
    Q_UNUSED(client);

    QWSScreenTransformationEvent event;
    event.simpleData.screen = cmd->simpleData.screen;
    event.simpleData.transformation = cmd->simpleData.transformation;

    QMap<int, QWSClient*>::const_iterator it = clientMap.constBegin();
    for (; it != clientMap.constEnd(); ++it)
        (*it)->sendEvent(&event);
}

QWSWindow* QWSServerPrivate::newWindow(int id, QWSClient* client)
{
    Q_Q(QWSServer);
    // Make a new window, put it on top.
    QWSWindow* w = new QWSWindow(id,client);

    // insert after "stays on top" windows
    bool added = false;
    for (int i = nReserved; i < windows.size(); ++i) {
        QWSWindow *win = windows.at(i);
        if (!win->onTop) {
            windows.insert(i, w);
            added = true;
            break;
        }
    }
    if (!added)
        windows.append(w);
    emit q->windowEvent(w, QWSServer::Create);
    return w;
}

QWSWindow* QWSServerPrivate::findWindow(int windowid, QWSClient* client)
{
    for (int i=0; i<windows.size(); ++i) {
        QWSWindow* w = windows.at(i);
        if (w->winId() == windowid)
            return w;
    }
    if (client)
        return newWindow(windowid,client);
    else
        return 0;
}

void QWSServerPrivate::raiseWindow(QWSWindow *changingw, int /*alt*/)
{
    Q_Q(QWSServer);
    if (changingw == windows.first())
        return;
    QWSWindow::State oldstate = changingw->d->state;
    changingw->d->state = QWSWindow::Raising;
    // Expose regions previously overlapped by transparent windows
    const QRegion bound = changingw->allocatedRegion();
    QRegion expose;
    int windowPos = 0;

    //change position in list:
    for (int i = 0; i < windows.size(); ++i) {
        QWSWindow *w = windows.at(i);
        if (w == changingw) {
            windowPos = i;
            windows.takeAt(i);
            break;
        }
        if (!w->isOpaque())
            expose += (w->allocatedRegion() & bound);
    }

    bool onTop = changingw->onTop;

#ifndef QT_NO_QWSEMBEDWIDGET
    // an embedded window is on top if the embedder is on top
    QWSWindow *embedder = changingw->d->embedder;
    while (!onTop && embedder) {
        onTop = embedder->onTop;
        embedder = embedder->d->embedder;
    }
#endif

    int newPos = -1;
    if (onTop) {
        windows.insert(nReserved, changingw);
        newPos = nReserved;
    } else {
        // insert after "stays on top" windows
        bool in = false;
        for (int i = nReserved; i < windows.size(); ++i) {
            QWSWindow *w = windows.at(i);
            if (!w->onTop) {
                windows.insert(i, changingw);
                in = true;
                newPos = i;
                break;
            }
        }
        if (!in) {
            windows.append(changingw);
            newPos = windows.size()-1;
        }
    }

    if (windowPos != newPos) {
        update_regions();
        if (!expose.isEmpty())
            exposeRegion(expose, newPos);
    }
    changingw->d->state = oldstate;
    emit q->windowEvent(changingw, QWSServer::Raise);
}

void QWSServerPrivate::lowerWindow(QWSWindow *changingw, int /*alt*/)
{
    Q_Q(QWSServer);
    if (changingw == windows.last())
        return;
    QWSWindow::State oldstate = changingw->d->state;
    changingw->d->state = QWSWindow::Lowering;

    int i = windows.indexOf(changingw);
    int newIdx = windows.size()-1;
    windows.move(i, newIdx);

    const QRegion bound = changingw->allocatedRegion();

    update_regions();

    // Expose regions previously overlapped by transparent window
    if (!changingw->isOpaque()) {
        QRegion expose;
        for (int j = i; j < windows.size() - 1; ++j)
            expose += (windows.at(j)->allocatedRegion() & bound);
        if (!expose.isEmpty())
            exposeRegion(expose, newIdx);
    }

    changingw->d->state = oldstate;
    emit q->windowEvent(changingw, QWSServer::Lower);
}

void QWSServerPrivate::update_regions()
{
    if (disablePainting)
        return;

    QRegion available = QRect(0, 0, qt_screen->width(), qt_screen->height());
    QRegion transparentRegion;

    // only really needed if there are unbuffered surfaces...
    const bool doLock = (clientMap.size() > 1);
    if (doLock)
        QWSDisplay::grab(true);

    for (int i = 0; i < windows.count(); ++i) {
        QWSWindow *w = windows.at(i);
        QRegion r = (w->requested_region & available);

#ifndef QT_NO_QWSEMBEDWIDGET
        // Subtract regions needed for embedded windows
        const int n = w->d->embedded.size();
        for (int i = 0; i < n; ++i)
            r -= w->d->embedded.at(i)->allocatedRegion();

        // Limited to the embedder region
        if (w->d->embedder)
            r &= w->d->embedder->requested_region;
#endif // QT_NO_QWSEMBEDWIDGET

        QWSWindowSurface *surface = w->windowSurface();
        const bool opaque = w->isOpaque()
                            && (w->d->painted || !surface || !surface->isBuffered());

        if (!opaque) {
            transparentRegion += r;
        } else {
            if (surface && (surface->isRegionReserved() || !surface->isBuffered()))
                r -= transparentRegion;
            available -= r;
        }

        if (r != w->allocatedRegion()) {
            w->setAllocatedRegion(r);
            w->client()->sendRegionEvent(w->winId(), r,
                                         QWSRegionEvent::Allocation);
        }

#ifdef QT_QWS_CLIENTBLIT
#ifdef QT_NO_QWS_CURSOR
        // This optimization only really works when there isn't a crazy cursor
        // wizzing around.
        QRegion directPaint = (r - transparentRegion); // in gloal coords
        if(directPaint != w->directPaintRegion()) {
            w->setDirectPaintRegion(directPaint);
            static int id = 0;
            surface->setDirectRegion(directPaint, ++id);
            w->client()->sendRegionEvent(w->winId(), directPaint,
                                         QWSRegionEvent::DirectPaint, id);
        }
#endif
#endif
    }

    if (doLock)
        QWSDisplay::ungrab();
}

void QWSServerPrivate::moveWindowRegion(QWSWindow *changingw, int dx, int dy)
{
    if (!changingw)
        return;

    QWSWindow::State oldState = changingw->d->state;
    changingw->d->state = QWSWindow::Moving;
    const QRegion oldRegion(changingw->allocatedRegion());
    changingw->requested_region.translate(dx, dy);

    // hw: Even if the allocated region doesn't change, the requested region
    // region has changed and we need to send region events.
    // Resetting the allocated region to force update_regions to send events.
    changingw->setAllocatedRegion(QRegion());
    update_regions();
    const QRegion newRegion(changingw->allocatedRegion());

    QWSWindowSurface *surface = changingw->windowSurface();
    QRegion expose;
    if (surface)
        expose = surface->move(QPoint(dx, dy), changingw->allocatedRegion());
    else
        expose = oldRegion + newRegion;

    if (!changingw->d->painted && !expose.isEmpty())
        expose = oldRegion - newRegion;

    int idx = windows.indexOf(changingw);
    exposeRegion(expose, idx);
    changingw->d->state = oldState;
}

/*!
    Changes the requested region of window \a changingw to \a r
    If \a changingw is 0, the server's reserved region is changed.
*/
void QWSServerPrivate::setWindowRegion(QWSWindow* changingw, const QRegion &r)
{
    if (!changingw) {
        qWarning("Not implemented in this release");
        return;
    }

    if (changingw->requested_region == r)
        return;

    const QRegion oldRegion(changingw->allocatedRegion());
    changingw->requested_region = r;
    update_regions();
    const QRegion newRegion(changingw->allocatedRegion());

    int idx = windows.indexOf(changingw);
    exposeRegion(oldRegion - newRegion, idx);
}


void QWSServerPrivate::exposeRegion(const QRegion &r, int changing)
{
    if (disablePainting)
        return;

    if (r.isEmpty())
        return;

    static bool initial = true;
    if (initial) {
        changing = 0;
        initial = false;
        qt_screen->exposeRegion(qt_screen->region(), changing);
    } else {
        qt_screen->exposeRegion(r, changing);
    }
}

/*!
    Closes all pointer devices (specified by the QWS_MOUSE_PROTO
    environment variable) by deleting the associated mouse drivers.

    \sa openMouse(), mouseHandler()
*/
void QWSServer::closeMouse()
{
    Q_D(QWSServer);
    qDeleteAll(d->mousehandlers);
    d->mousehandlers.clear();
}

/*!
    Opens the mouse devices specified by the QWS_MOUSE_PROTO
    environment variable. Be advised that closeMouse() is called first
    to delete all the existing mouse handlers. This behaviour could be
    the cause of problems if you were not expecting it.

    \sa closeMouse(), mouseHandler()
*/
void QWSServer::openMouse()
{
    Q_D(QWSServer);
    QString mice = QString::fromLatin1(qgetenv("QWS_MOUSE_PROTO"));
#if defined(QT_QWS_CASSIOPEIA)
    if (mice.isEmpty())
        mice = QLatin1String("TPanel:/dev/tpanel");
#endif
    if (mice.isEmpty())
        mice = *defaultMouse();
    closeMouse();
    bool needviscurs = true;
    if (mice != QLatin1String("None")) {
        const QStringList mouse = mice.split(QLatin1Char(' '));
        for (int i = mouse.size() - 1; i >= 0; --i) {
            QWSMouseHandler *handler = d->newMouseHandler(mouse.at(i));
            setMouseHandler(handler);
            /* XXX handle mouse cursor visibility sensibly
               if (!h->inherits("QCalibratedMouseHandler"))
               needviscurs = true;
            */
        }
    }
#ifndef QT_NO_QWS_CURSOR
    setCursorVisible(needviscurs);
#else
    Q_UNUSED(needviscurs)
#endif
}

/*!
    Suspends pointer handling by deactivating all the mouse drivers
    registered by the QWS_MOUSE_PROTO environment variable.


    \sa resumeMouse(), QWSMouseHandler::suspend()
*/
void QWSServer::suspendMouse()
{
    Q_D(QWSServer);
    for (int i=0; i < d->mousehandlers.size(); ++i)
        d->mousehandlers.at(i)->suspend();
}

/*!
    Resumes pointer handling by reactivating all the mouse drivers
    registered by the QWS_MOUSE_PROTO environment variable.

    \sa suspendMouse(), QWSMouseHandler::resume()
*/
void QWSServer::resumeMouse()
{
    Q_D(QWSServer);
    for (int i=0; i < d->mousehandlers.size(); ++i)
        d->mousehandlers.at(i)->resume();
}



QWSMouseHandler* QWSServerPrivate::newMouseHandler(const QString& spec)
{
    int c = spec.indexOf(QLatin1Char(':'));
    QString mouseProto;
    QString mouseDev;
    if (c >= 0) {
        mouseProto = spec.left(c);
        mouseDev = spec.mid(c+1);
    } else {
        mouseProto = spec;
    }

    int screen = -1;
    const QList<QRegExp> regexps = QList<QRegExp>()
                                   << QRegExp(QLatin1String(":screen=(\\d+)\\b"))
                                   << QRegExp(QLatin1String("\\bscreen=(\\d+):"));
    for (int i = 0; i < regexps.size(); ++i) {
        QRegExp regexp = regexps.at(i);
        if (regexp.indexIn(mouseDev) == -1)
            continue;
        screen = regexp.cap(1).toInt();
        mouseDev.remove(regexp.pos(0), regexp.matchedLength());
        break;
    }

    QWSMouseHandler *handler = 0;
    handler = QMouseDriverFactory::create(mouseProto, mouseDev);
    if (screen != -1)
        handler->setScreen(qt_screen->subScreens().at(screen));

    return handler;
}

#ifndef QT_NO_QWS_KEYBOARD

/*!
    Closes all the keyboard devices (specified by the QWS_KEYBOARD
    environment variable) by deleting the associated keyboard
    drivers.

    \sa openKeyboard(),  keyboardHandler()
*/
void QWSServer::closeKeyboard()
{
    Q_D(QWSServer);
    qDeleteAll(d->keyboardhandlers);
    d->keyboardhandlers.clear();
}

/*!
    Returns the primary keyboard driver.

    Note that this function can only be used in the server process.

    \sa setKeyboardHandler(), openKeyboard(), closeKeyboard()
*/
QWSKeyboardHandler* QWSServer::keyboardHandler()
{
    return qwsServerPrivate->keyboardhandlers.first();
}

/*!
    \fn void QWSServer::setKeyboardHandler(QWSKeyboardHandler* driver)

    Sets the primary keyboard driver to be the given \a driver.

    \l{Qt for Embedded Linux} provides several ready-made keyboard drivers, and
    custom drivers are typically added using Qt's plugin
    mechanism. See the \l{Qt for Embedded Linux Character Input} documentation
    for details.

    Note that this function can only be used in the server process.

    \sa keyboardHandler(), setDefaultKeyboard()
*/
void QWSServer::setKeyboardHandler(QWSKeyboardHandler* kh)
{
    if (!kh)
        return;
    qwsServerPrivate->keyboardhandlers.removeAll(kh);
    qwsServerPrivate->keyboardhandlers.prepend(kh);
}

/*!
    Opens the keyboard devices specified by the QWS_KEYBOARD
    environment variable.

    \sa closeKeyboard(), keyboardHandler()
*/
void QWSServer::openKeyboard()
{
    QString keyboards = QString::fromLatin1(qgetenv("QWS_KEYBOARD"));
#if defined(QT_QWS_CASSIOPEIA)
    if (keyboards.isEmpty())
        keyboards = QLatin1String("Buttons");
#endif
    if (keyboards.isEmpty())
        keyboards = *defaultKeyboard();

    closeKeyboard();
    if (keyboards == QLatin1String("None"))
        return;

    QString device;
    QString type;
    QStringList keyboard = keyboards.split(QLatin1Char(' '));
    for (int i = keyboard.size() - 1; i >= 0; --i) {
        const QString spec = keyboard.at(i);
        int colon=spec.indexOf(QLatin1Char(':'));
        if (colon>=0) {
            type = spec.left(colon);
            device = spec.mid(colon+1);
        } else {
            type = spec;
            device = QString();
        }
        QWSKeyboardHandler *handler = QKbdDriverFactory::create(type, device);
        setKeyboardHandler(handler);
    }
}

#endif //QT_NO_QWS_KEYBOARD

QPoint QWSServer::mousePosition;
QBrush *QWSServerPrivate::bgBrush = 0;

void QWSServerPrivate::move_region(const QWSRegionMoveCommand *cmd)
{
    QWSClient *serverClient = clientMap.value(-1);
    invokeRegionMove(cmd, serverClient);
}

void QWSServerPrivate::set_altitude(const QWSChangeAltitudeCommand *cmd)
{
    QWSClient *serverClient = clientMap.value(-1);
    invokeSetAltitude(cmd, serverClient);
}

void QWSServerPrivate::set_opacity(const QWSSetOpacityCommand *cmd)
{
    QWSClient *serverClient = clientMap.value(-1);
    invokeSetOpacity(cmd, serverClient);
}


void QWSServerPrivate::request_focus(const QWSRequestFocusCommand *cmd)
{
    invokeSetFocus(cmd, clientMap.value(-1));
}

void QWSServerPrivate::set_identity(const QWSIdentifyCommand *cmd)
{
    invokeIdentify(cmd, clientMap.value(-1));
}

void QWSServerPrivate::repaint_region(int wid, int windowFlags, bool opaque,
                                      const QRegion &region)
{
    QWSWindow* changingw = findWindow(wid, 0);
    if (!changingw) {
        return;
    }

    const bool isOpaque = changingw->opaque;
    const bool wasPainted = changingw->d->painted;
    changingw->opaque = opaque;
    changingw->d->windowFlags = QFlag(windowFlags);
    changingw->d->dirtyOnScreen |= region;
    changingw->d->painted = true;
    if (isOpaque != opaque || !wasPainted)
        update_regions();

    int level = windows.indexOf(changingw);
    exposeRegion(region, level);
    changingw->d->dirtyOnScreen = QRegion();
}

QRegion QWSServerPrivate::reserve_region(QWSWindow *win, const QRegion &region)
{
    QRegion r = region;

    int oldPos = windows.indexOf(win);
    int newPos = oldPos < nReserved ? nReserved - 1 : nReserved;
    for (int i = 0; i < nReserved; ++i) {
        if (i != oldPos) {
            QWSWindow *w = windows.at(i);
            r -= w->requested_region;
        }
    }
    windows.move(oldPos, newPos);
    nReserved = newPos + 1;

    return r;
}

void QWSServerPrivate::request_region(int wid, const QString &surfaceKey,
                                      const QByteArray &surfaceData,
                                      const QRegion &region)
{
    QWSWindow *changingw = findWindow(wid, 0);
    if (!changingw)
        return;

    Q_Q(QWSServer);
    QWSWindow::State windowState = QWSWindow::NoState;

    if (region.isEmpty()) {
        windowState = QWSWindow::Hiding;
        emit q->windowEvent(changingw, QWSServer::Hide);
    }

    const bool wasOpaque = changingw->opaque;

    changingw->createSurface(surfaceKey, surfaceData);
    QWSWindowSurface *surface = changingw->windowSurface();

    changingw->opaque = surface->isOpaque();

    QRegion r;
    if (surface->isRegionReserved())
        r = reserve_region(changingw, region);
    else
        r = region;

    if (!region.isEmpty())  {
        if (changingw->isVisible())
            windowState = QWSWindow::ChangingGeometry;
        else
            windowState = QWSWindow::Showing;
    }
    changingw->d->state = windowState;

    if (!r.isEmpty() && wasOpaque != changingw->opaque && surface->isBuffered())
        changingw->requested_region = QRegion(); // XXX: force update_regions

    const QRegion oldAllocated = changingw->allocatedRegion();
    setWindowRegion(changingw, r);
    if (oldAllocated == changingw->allocatedRegion()) {
        // Always send region event to the requesting window even if the
        // region didn't change. This is necessary as the client will reset
        // the clip region until an event is received.
        changingw->client()->sendRegionEvent(wid, changingw->allocatedRegion(),
                                             QWSRegionEvent::Allocation);
    }

    surface->QWindowSurface::setGeometry(r.boundingRect());

    if (windowState == QWSWindow::Showing)
        emit q->windowEvent(changingw, QWSServer::Show);
    else if (windowState == QWSWindow::ChangingGeometry)
        emit q->windowEvent(changingw, QWSServer::Geometry);
    if (windowState == QWSWindow::Hiding) {
        handleWindowClose(changingw);
        changingw->d->state = QWSWindow::Hidden;
        changingw->d->painted = false;
    } else {
        changingw->d->state = QWSWindow::Visible;
    }
}

void QWSServerPrivate::destroy_region(const QWSRegionDestroyCommand *cmd)
{
    invokeRegionDestroy(cmd, clientMap.value(-1));
}

void QWSServerPrivate::name_region(const QWSRegionNameCommand *cmd)
{
    invokeRegionName(cmd, clientMap.value(-1));
}

#ifndef QT_NO_QWS_INPUTMETHODS
void QWSServerPrivate::im_response(const QWSIMResponseCommand *cmd)
 {
     invokeIMResponse(cmd, clientMap.value(-1));
}

void QWSServerPrivate::im_update(const QWSIMUpdateCommand *cmd)
{
    invokeIMUpdate(cmd, clientMap.value(-1));
}

void QWSServerPrivate::send_im_mouse(const QWSIMMouseCommand *cmd)
{
    if (current_IM)
        current_IM->mouseHandler(cmd->simpleData.index, cmd->simpleData.state);
}
#endif

void QWSServerPrivate::openDisplay()
{
    qt_init_display();

//    rgnMan = qt_fbdpy->regionManager();
    swidth = qt_screen->deviceWidth();
    sheight = qt_screen->deviceHeight();
}

void QWSServerPrivate::closeDisplay()
{
    if (qt_screen)
        qt_screen->shutdownDevice();
}

/*!
    Returns the brush used as background in the absence of obscuring
    windows.

    \sa setBackground()
*/
const QBrush &QWSServer::backgroundBrush() const
{
    return *QWSServerPrivate::bgBrush;
}

/*!
    Sets the brush used as background in the absence of obscuring
    windows, to be the given \a brush.

    Note that this function can only be used in the server process.

    \sa backgroundBrush()
*/
void QWSServer::setBackground(const QBrush &brush)
{
    if (!QWSServerPrivate::bgBrush)
        QWSServerPrivate::bgBrush = new QBrush(brush);
    else
        *QWSServerPrivate::bgBrush = brush;
    if (!qwsServer)
        return;
    qt_screen->exposeRegion(QRect(0,0,qt_screen->width(), qt_screen->height()), 0);
}


#ifdef QT3_SUPPORT
/*!
    \fn void QWSServer::setDesktopBackground(const QImage &image)

    Sets the image used as background in the absence of obscuring
    windows, to be the given \a image.

    Use the setBackground() function instead.

    \oldcode
        QImage image;
        setDesktopBackground(image);
    \newcode
        QImage image;
        setBackground(QBrush(image));
    \endcode
*/
void QWSServer::setDesktopBackground(const QImage &img)
{
    if (img.isNull())
        setBackground(Qt::NoBrush);
    else
        setBackground(QBrush(QPixmap::fromImage(img)));
}

/*!
    \fn void QWSServer::setDesktopBackground(const QColor &color)
    \overload

    Sets the color used as background in the absence of obscuring
    windows, to be the given \a color.

    Use the setBackground() function instead.

    \oldcode
        QColor color;
        setDesktopBackground(color);
    \newcode
        QColor color;
        setBackground(QBrush(color));
    \endcode
*/
void QWSServer::setDesktopBackground(const QColor &c)
{
    setBackground(QBrush(c));
}
#endif //QT3_SUPPORT

/*!
  \internal
 */
void QWSServer::startup(int flags)
{
    if (qwsServer)
        return;
    unlink(qws_qtePipeFilename().toLatin1().constData());
    (void)new QWSServer(flags);
}

/*!
  \internal
*/

void QWSServer::closedown()
{
    QScopedPointer<QWSServer> server(qwsServer);
    qwsServer = 0;
    QT_TRY {
        unlink(qws_qtePipeFilename().toLatin1().constData());
    } QT_CATCH(const std::bad_alloc &) {
        // ### TODO - what to do when we run out of memory
        // when calling toLatin1?
    }
}

void QWSServerPrivate::emergency_cleanup()
{
#ifndef QT_NO_QWS_KEYBOARD
    if (qwsServer)
        qwsServer->closeKeyboard();
#endif
}

#ifndef QT_NO_QWS_KEYBOARD
static QList<QWSServer::KeyboardFilter*> *keyFilters = 0;

/*!
    Processes the given key event. The key is identified by its \a
    unicode value and the given \a keycode, \a modifiers, \a isPress
    and \a autoRepeat parameters.

    The \a keycode parameter is the Qt keycode value as defined by the
    Qt::Key enum. The \a modifiers is an OR combination of
    Qt::KeyboardModifier values, indicating whether \gui
    Shift/Alt/Ctrl keys are pressed. The \a isPress parameter is true
    if the event is a key press event and \a autoRepeat is true if the
    event is caused by an auto-repeat mechanism and not an actual key
    press.

    This function is typically called internally by keyboard drivers.
    Note that this function can only be used in the server process.

    \sa sendKeyEvent(), {Qt for Embedded Linux Character Input}
*/
void QWSServer::processKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                                bool isPress, bool autoRepeat)
{
    bool block;
    // Don't block the POWER or LIGHT keys
    if ( keycode == Qt::Key_F34 || keycode == Qt::Key_F35 )
        block = false;
    else
        block = qwsServerPrivate->screensaverblockevent(KEY, qwsServerPrivate->screensaverinterval, isPress);

#ifdef EVENT_BLOCK_DEBUG
    qDebug() << "processKeyEvent" << unicode << keycode << modifiers << isPress << autoRepeat << (block ? "block" : "pass");
#endif

    // If we press a key and it's going to be blocked, wake up the screen
    if ( block && isPress )
        qwsServerPrivate->_q_screenSaverWake();

    if ( block )
        return;

    if (keyFilters) {
        for (int i = 0; i < keyFilters->size(); ++i) {
            QWSServer::KeyboardFilter *keyFilter = keyFilters->at(i);
            if (keyFilter->filter(unicode, keycode, modifiers, isPress, autoRepeat))
                return;
        }
    }
    sendKeyEvent(unicode, keycode, modifiers, isPress, autoRepeat);
}

/*!
    \fn void QWSServer::addKeyboardFilter(KeyboardFilter *filter)

    Activates the given keyboard \a filter all key events generated by
    physical keyboard drivers (i.e., events sent using the
    processKeyEvent() function).

    Note that the filter is not invoked for keys generated by \e
    virtual keyboard drivers (i.e., events sent using the
    sendKeyEvent() function).

    Note that this function can only be used in the server process.

    \sa removeKeyboardFilter()
*/
void QWSServer::addKeyboardFilter(KeyboardFilter *f)
{
     if (!keyFilters)
        keyFilters = new QList<QWSServer::KeyboardFilter*>;
     if (f) {
        keyFilters->prepend(f);
     }
}

/*
//#######
 We should probably obsolete the whole keyboard filter thing since
 it's not useful for input methods anyway

 We could do removeKeyboardFilter(KeyboardFilter *f), but
 the "remove and delete the filter" concept does not match "user
 remembers the pointer".
*/

/*!
    Removes and deletes the most recently added filter.

    Note that the programmer is responsible for removing each added
    keyboard filter.

    Note that this function can only be used in the server process.

    \sa addKeyboardFilter()
*/
void QWSServer::removeKeyboardFilter()
{
     if (!keyFilters || keyFilters->isEmpty())
         return;
     delete keyFilters->takeAt(0);
}
#endif // QT_NO_QWS_KEYBOARD

/*!
    \fn void QWSServer::setScreenSaverIntervals(int* intervals)

    Specifies the time \a intervals (in milliseconds) between the
    different levels of screen responsiveness.

    \l{Qt for Embedded Linux} supports multilevel screen saving, i.e., it is
    possible to specify several different levels of screen
    responsiveness by implementing the QWSScreenSaver::save()
    function. For example, you can choose to first turn off the light
    before you fully activate the screensaver. See the QWSScreenSaver
    documentation for details.

    Note that an interval of 0 milliseconds will turn off the
    screensaver, and that the \a intervals array must be 0-terminated.
    This function can only be used in the server process.

    \sa setScreenSaverInterval(), setScreenSaverBlockLevel()
*/
void QWSServer::setScreenSaverIntervals(int* ms)
{
    if (!qwsServerPrivate)
        return;

    delete [] qwsServerPrivate->screensaverintervals;
    if (ms) {
        int* t=ms;
        int n=0;
        while (*t++) n++;
        if (n) {
            n++; // the 0
            qwsServerPrivate->screensaverintervals = new int[n];
            memcpy(qwsServerPrivate->screensaverintervals, ms, n*sizeof(int));
        } else {
            qwsServerPrivate->screensaverintervals = 0;
        }
    } else {
        qwsServerPrivate->screensaverintervals = 0;
    }
    qwsServerPrivate->screensaverinterval = 0;

    qwsServerPrivate->screensavertimer->stop();
    qt_screen->blank(false);
    qwsServerPrivate->_q_screenSaverWake();
}

/*!
    \fn void QWSServer::setScreenSaverInterval(int milliseconds)

    Sets the timeout interval for the screensaver to the specified \a
    milliseconds. To turn off the screensaver, set the timout interval
    to 0.

    Note that this function can only be used in the server process.

    \sa setScreenSaverIntervals(), setScreenSaverBlockLevel()
*/
void QWSServer::setScreenSaverInterval(int ms)
{
    int v[2];
    v[0] = ms;
    v[1] = 0;
    setScreenSaverIntervals(v);
}

/*!
  Block the key or mouse event that wakes the system from level \a eventBlockLevel or higher.
  To completely disable event blocking (the default behavior), set \a eventBlockLevel to -1.

  The algorithm blocks the "down", "up" as well as any "repeat" events for the same key
  but will not block other key events after the initial "down" event. For mouse events, the
  algorithm blocks all mouse events until an event with no buttons pressed is received.

  There are 2 keys that are never blocked, Qt::Key_F34 (POWER) and Qt::Key_F35 (LIGHT).

  Example usage:

  \snippet doc/src/snippets/code/src_gui_embedded_qwindowsystem_qws.cpp 0

    Note that this function can only be used in the server process.

  \sa setScreenSaverIntervals(), setScreenSaverInterval()
*/
void QWSServer::setScreenSaverBlockLevel(int eventBlockLevel)
{
    if (!qwsServerPrivate)
        return;
    qwsServerPrivate->screensavereventblocklevel = eventBlockLevel;
#ifdef EVENT_BLOCK_DEBUG
    qDebug() << "QWSServer::setScreenSaverBlockLevel() " << eventBlockLevel;
#endif
}

extern bool qt_disable_lowpriority_timers; //in qeventloop_unix.cpp

void QWSServerPrivate::_q_screenSaverWake()
{
    if (screensaverintervals) {
        if (screensaverinterval != screensaverintervals) {
            if (saver) saver->restore();
            screensaverinterval = screensaverintervals;
            screensaverblockevents = false;
        } else {
            if (!screensavertimer->isActive()) {
                qt_screen->blank(false);
                if (saver) saver->restore();
            }
        }
        screensavertimer->start(*screensaverinterval);
        screensavertime.start();
    }
    qt_disable_lowpriority_timers=false;
}

void QWSServerPrivate::_q_screenSaverSleep()
{
    qt_screen->blank(true);
#if !defined(QT_QWS_IPAQ) && !defined(QT_QWS_EBX)
    screensavertimer->stop();
#else
    if (screensaverinterval) {
        screensavertimer->start(*screensaverinterval);
        screensavertime.start();
    } else {
        screensavertimer->stop();
    }
#endif
    qt_disable_lowpriority_timers=true;
}

/*!
    \fn void QWSServer::setScreenSaver(QWSScreenSaver* screenSaver)

    Installs the given \a screenSaver, deleting the current screen
    saver.

    Note that this function can only be used in the server process.

    \sa screenSaverActivate(), setScreenSaverInterval(), setScreenSaverIntervals(), setScreenSaverBlockLevel()
*/
void QWSServer::setScreenSaver(QWSScreenSaver* ss)
{
    QWSServerPrivate *qd = qwsServer->d_func();
    delete qd->saver;
    qd->saver = ss;
}

void QWSServerPrivate::screenSave(int level)
{
    if (saver) {
        // saver->save() may call QCoreApplication::processEvents,
        // block event before calling saver->save().
        bool oldScreensaverblockevents = screensaverblockevents;
        if (*screensaverinterval >= 1000) {
            screensaverblockevents = (screensavereventblocklevel >= 0 && screensavereventblocklevel <= level);
#ifdef EVENT_BLOCK_DEBUG
            if (screensaverblockevents)
                qDebug("ready to block events");
#endif
        }
        int *oldScreensaverinterval = screensaverinterval;
        if (saver->save(level)) {
            // only update screensaverinterval if it hasn't already changed
            if (oldScreensaverinterval == screensaverinterval) {
                if (screensaverinterval && screensaverinterval[1]) {
                    screensavertimer->start(*++screensaverinterval);
                    screensavertime.start();
                } else {
                    screensaverinterval = 0;
                }
            }
        } else {
            // restore previous state
            screensaverblockevents = oldScreensaverblockevents;

            // for some reason, the saver don't want us to change to the
            // next level, so we'll stay at this level for another interval
            if (screensaverinterval && *screensaverinterval) {
                screensavertimer->start(*screensaverinterval);
                screensavertime.start();
            }
        }
    } else {
        screensaverinterval = 0;//screensaverintervals;
        screensaverblockevents = false;
        _q_screenSaverSleep();
    }
}

void QWSServerPrivate::_q_screenSaverTimeout()
{
    if (screensaverinterval) {
        if (screensavertime.elapsed() > *screensaverinterval*2) {
            // bogus (eg. unsuspend, system time changed)
            _q_screenSaverWake(); // try again
            return;
        }
        screenSave(screensaverinterval - screensaverintervals);
    }
}

/*!
    Returns true if the screen saver is active; otherwise returns
    false.

    Note that this function can only be used in the server process.

    \sa screenSaverActivate()
*/
bool QWSServer::screenSaverActive()
{
    return qwsServerPrivate->screensaverinterval
        && !qwsServerPrivate->screensavertimer->isActive();
}

/*!
    \internal
*/
void QWSServer::updateWindowRegions() const
{
    qwsServerPrivate->update_regions();
}

/*!
    Activates the screen saver if \a activate is true; otherwise it is
    deactivated.

    Note that this function can only be used in the server process.

    \sa screenSaverActive(), setScreenSaver()
*/
void QWSServer::screenSaverActivate(bool activate)
{
    if (activate)
        qwsServerPrivate->_q_screenSaverSleep();
    else
        qwsServerPrivate->_q_screenSaverWake();
}

void QWSServerPrivate::disconnectClient(QWSClient *c)
{
    QTimer::singleShot(0, c, SLOT(closeHandler()));
}

void QWSServerPrivate::updateClientCursorPos()
{
    Q_Q(QWSServer);
    QWSWindow *win = qwsServerPrivate->mouseGrabber ? qwsServerPrivate->mouseGrabber : qwsServer->windowAt(QWSServer::mousePosition);
    QWSClient *winClient = win ? win->client() : 0;
    if (winClient && winClient != cursorClient)
        q->sendMouseEvent(QWSServer::mousePosition, mouseState);
}

#ifndef QT_NO_QWS_INPUTMETHODS

/*!
    \class QWSInputMethod
    \preliminary
    \ingroup qws

    \brief The QWSInputMethod class provides international input methods
    in Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    A \l{Qt for Embedded Linux} application requires a server application to be
    running, or to be the server application itself. All system
    generated events, including keyboard and mouse events, are passed
    to the server application which then propagates the event to the
    appropriate client.

    An input method consists of a filter and optionally a graphical
    interface, and is used to filter input events between the server
    and the client application.

    \tableofcontents

    \section1 Creating Custom Input Methods

    To implement a custom input method, derive from the QWSInputMethod
    class, and use the server's \l
    {QWSServer::}{setCurrentInputMethod()} function to install it.

    When subclassing QWSInputMethod, you can reimplement the filter()
    functions to handle input from both physical and virtual keyboards
    as well as mouse devices. Note that the default implementations do
    nothing. Use the setInputResolution() function to control the
    number of bits shifted when filtering mouse input, i.e., when
    going from pointer resolution to screen resolution (the current
    resolution can be retrieved using the inputResolutionShift()
    function).

    Reimplement the reset() function to restore the state of the input
    method. Note that the default implementation calls the sendEvent()
    function with empty preedit and commit strings if the input method
    is in compose mode (i.e., if the input method is actively
    composing a preedit string).

    To receive replies to an input method query (sent using the
    sendQuery() function), you must reimplement the queryResponse()
    function, while the mouseHandler() function must be reimplemented
    if you want to handle mouse events within the preedit
    text. Reimplement the updateHandler() function to handle update
    events including resets and focus changes. The UpdateType enum
    describes the various types of update events recognized by the
    input method.

    \section1 Using Input Methods

    In addition to the filter(), reset(), queryResponse(),
    mouseHandler() and updateHandler() function mentioned in the
    previous section, the QWSInputMethod provides several other
    functions helping the window system to manage the installed input
    methods.

    The sendEvent() function sends the given event to the focus
    widget, while the sendPreeditString() function sends the given
    preedit text (encapsulated by an event). QWSInputMethod also
    provides the sendCommitString() convenience function which sends
    an event encapsulating the given commit string to the current
    focus widget, and the sendMouseEvent() function which sends the
    given mouse event.

    Finally, the QWSInputMethod class provides the sendQuery()
    function for sending input method queries. This function
    encapsulates the event with a QWSEvent instance of the \l
    {QWSEvent::}{IMQuery} type.

    \sa QWSServer, {Qt for Embedded Linux Architecture}
*/

/*!
    Constructs a new input method.

    Use the QWSServer::setCurrentInputMethod() function to install it.
*/

QWSInputMethod::QWSInputMethod()
{

}

/*!
    Destroys this input method, uninstalling it if it is installed.
*/
QWSInputMethod::~QWSInputMethod()
{
    if (current_IM == this)
        current_IM = 0;
}

/*!
    Filters the key input identified by the given \a unicode, \a
    keycode, \a modifiers, \a isPress and \a autoRepeat parameters.

    Note that the default implementation does nothing; reimplement
    this function to handle input from both physical and virtual
    devices.

    The \a keycode is a Qt::Key value, and the \a modifiers is an OR
    combination of Qt::KeyboardModifiers. The \a isPress parameter is
    telling whether the input is a key press or key release, and the
    \a autoRepeat parameter determines whether the input is
    autorepeated ( i.e., in which case the
    QWSKeyboardHandler::beginAutoRepeat() function has been called).

    To block the event from further processing, return true when
    reimplementing this function; the default implementation returns
    false.

    \sa setInputResolution(), inputResolutionShift()
*/
bool QWSInputMethod::filter(int unicode, int keycode, int modifiers, bool isPress, bool autoRepeat)
{
    Q_UNUSED(unicode);
    Q_UNUSED(keycode);
    Q_UNUSED(modifiers);
    Q_UNUSED(isPress);
    Q_UNUSED(autoRepeat);
    return false;
}

/*!
    \overload

    Filters the mouse input identified by the given \a position, \a
    state, and \a wheel parameters.
*/
bool QWSInputMethod::filter(const QPoint &position, int state, int wheel)
{
    Q_UNUSED(position);
    Q_UNUSED(state);
    Q_UNUSED(wheel);
    return false;
}

/*!
    Resets the state of the input method.

    If the input method is in compose mode, i.e., the input method is
    actively composing a preedit string, the default implementation
    calls sendEvent() with empty preedit and commit strings; otherwise
    it does nothing. Reimplement this function to alter this behavior.

    \sa sendEvent()
*/
void QWSInputMethod::reset()
{
    if (current_IM_composing_win) {
        QInputMethodEvent ime;
        sendEvent(&ime);
    }
}

/*!
    \enum QWSInputMethod::UpdateType

    This enum describes the various types of update events recognized
    by the input method.

    \value Update    The input widget is updated in some way; use sendQuery() with
                            Qt::ImMicroFocus as an argument for more information.
    \value FocusIn   A new input widget receives focus.
    \value FocusOut  The input widget loses focus.
    \value Reset       The input method should be reset.
    \value Destroyed The input widget is destroyed.

    \sa updateHandler()
*/

/*!
    Handles update events including resets and focus changes. The
    update events are specified by the given \a type which is one of
    the UpdateType enum values.

    Note that reimplementations of this function must call the base
    implementation for all cases that it does not handle itself.

    \sa UpdateType
*/
void QWSInputMethod::updateHandler(int type)
{
    switch (type) {
    case FocusOut:
    case Reset:
        reset();
        break;

    default:
        break;
    }
}


/*!
    Receive replies to an input method query.

    Note that the default implementation does nothing; reimplement
    this function to receive such replies.

    Internally, an input method query is passed encapsulated by an \l
    {QWSEvent::IMQuery}{IMQuery} event generated by the sendQuery()
    function. The queried property and the result is passed in the \a
    property and \a result parameters.

    \sa sendQuery(), QWSServer::sendIMQuery()
*/
void QWSInputMethod::queryResponse(int property, const QVariant &result)
{
    Q_UNUSED(property);
    Q_UNUSED(result);
}



/*!
    \fn void QWSInputMethod::mouseHandler(int offset, int state)

    Handles mouse events within the preedit text.

    Note that the default implementation resets the input method on
    all mouse presses; reimplement this function to alter this
    behavior.

    The \a offset parameter specifies the position of the mouse event
    within the string, and \a state specifies the type of the mouse
    event as described by the QWSServer::IMMouse enum. If \a state is
    less than 0, the mouse event is inside the associated widget, but
    outside the preedit text. When clicking in a different widget, the
    \a state is QWSServer::MouseOutside.

    \sa sendPreeditString(), reset()
*/
void QWSInputMethod::mouseHandler(int, int state)
{
    if (state == QWSServer::MousePress || state == QWSServer::MouseOutside)
        reset();
}


/*!
    Sends an event encapsulating the given \a preeditString, to the
    focus widget.

    The specified \a selectionLength is the number of characters to be
    marked as selected (starting at the given \a cursorPosition). If
    \a selectionLength is negative, the text \e before \a
    cursorPosition is marked.

    The preedit string is marked with QInputContext::PreeditFormat,
    and the selected part is marked with
    QInputContext::SelectionFormat.

    Sending an input method event with a non-empty preedit string will
    cause the input method to enter compose mode.  Sending an input
    method event with an empty preedit string will cause the input
    method to leave compose mode, i.e., the input method will no longer
    be actively composing the preedit string.

    Internally, the event is represented by a QWSEvent object of the
    \l {QWSEvent::IMEvent}{IMEvent} type.

    \sa sendEvent(), sendCommitString()
*/

void QWSInputMethod::sendPreeditString(const QString &preeditString, int cursorPosition, int selectionLength)
{
    QList<QInputMethodEvent::Attribute> attributes;

    int selPos = cursorPosition;
    if (selectionLength == 0) {
        selPos = 0;
    } else if (selectionLength < 0) {
        selPos += selectionLength;
        selectionLength = -selectionLength;
    }
    if (selPos > 0)
        attributes += QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 0, selPos,
                                                   QVariant(int(QInputContext::PreeditFormat)));

    if (selectionLength)
        attributes += QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, selPos, selectionLength,
                                                   QVariant(int(QInputContext::SelectionFormat)));

    if (selPos + selectionLength < preeditString.length())
        attributes += QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                                   selPos + selectionLength,
                                                   preeditString.length() - selPos - selectionLength,
                                                   QVariant(int(QInputContext::PreeditFormat)));

    attributes += QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, cursorPosition,  0, QVariant());

    QInputMethodEvent ime(preeditString, attributes);
    qwsServer->sendIMEvent(&ime);
}

/*!
    \fn void QWSInputMethod::sendCommitString(const QString &commitString, int replaceFromPosition, int replaceLength)

    Sends an event encapsulating the given \a commitString, to the
    focus widget.

    Note that this will cause the input method to leave compose mode,
    i.e., the input method will no longer be actively composing the
    preedit string.

    If the specified \a replaceLength is greater than 0, the commit
    string will replace the given number of characters of the
    receiving widget's previous text, starting at the given \a
    replaceFromPosition relative to the start of the current preedit
    string.

    Internally, the event is represented by a QWSEvent object of the
    \l {QWSEvent::IMEvent}{IMEvent} type.

    \sa sendEvent(), sendPreeditString()
*/
void QWSInputMethod::sendCommitString(const QString &commitString, int replaceFrom, int replaceLength)
{
    QInputMethodEvent ime;
    ime.setCommitString(commitString, replaceFrom, replaceLength);
    qwsServer->sendIMEvent(&ime);
}

/*!
    \fn QWSInputMethod::sendIMEvent(QWSServer::IMState state, const QString &text, int cursorPosition, int selectionLength)
    \obsolete

    Sends a QInputMethodEvent object to the focus widget.

    If the specified \a state is QWSServer::IMCompose, \a text is a
    preedit string, \a cursorPosition is the cursor's position within
    the preedit string, and \a selectionLength is the number of
    characters (starting at \a cursorPosition) that should be marked
    as selected by the input widget receiving the event. If the
    specified \a state is QWSServer::IMEnd, \a text is a commit
    string.

    Use sendEvent(), sendPreeditString() or sendCommitString() instead.
*/

/*!
    \fn QWSInputMethod::sendEvent(const QInputMethodEvent *event)

    Sends the given \a event to the focus widget.

    The \c QInputMethodEvent class is derived from QWSEvent, i.e., the
    given \a event is a QWSEvent object of the \l
    {QWSEvent::IMEvent}{IMEvent} type.

    \sa sendPreeditString(), sendCommitString(), reset()
*/


/*!
    \fn void QWSInputMethod::sendQuery(int property)

    Sends an input method query (internally encapsulated by a QWSEvent
    of the \l {QWSEvent::IMQuery}{IMQuery} type) for the specified \a
    property.

    To receive responses to input method queries, the virtual
    queryResponse() function must be reimplemented.

    \sa queryResponse(), QWSServer::sendIMQuery()
*/

/*!
    Sets and returns the number of bits shifted to go from pointer
    resolution to screen resolution when filtering mouse input.

    If \a isHigh is true and the device has a pointer device
    resolution twice or more of the screen resolution, the positions
    passed to the filter() function will be presented at the higher
    resolution; otherwise the resolution will be equal to that of the
    screen resolution.

    \sa inputResolutionShift(), filter()
*/
uint QWSInputMethod::setInputResolution(bool isHigh)
{
    mIResolution = isHigh;
    return inputResolutionShift();
}

/*!
    Returns the number of bits shifted to go from pointer resolution
    to screen resolution when filtering mouse input.

    \sa setInputResolution(), filter()
*/
uint QWSInputMethod::inputResolutionShift() const
{
    return 0; // default for devices with single resolution.
}

/*!
    \fn void QWSInputMethod::sendMouseEvent( const QPoint &position, int state, int wheel )

    Sends a mouse event specified by the given \a position, \a state
    and \a wheel parameters.

    The given \a position will be transformed if the screen
    coordinates do not match the pointer device coordinates.

    Note that the event will be not be tested by the active input
    method, but calling the QWSServer::sendMouseEvent() function will
    make the current input method filter the event.

    \sa mouseHandler(), sendEvent()
*/
void QWSInputMethod::sendMouseEvent( const QPoint &pos, int state, int wheel )
{
        if (qt_last_x) {
         *qt_last_x = pos.x();
         *qt_last_y = pos.y();
    }
    QWSServer::mousePosition = pos;
    qwsServerPrivate->mouseState = state;
    QWSServerPrivate::sendMouseEventUnfiltered(pos, state, wheel);
}
#endif // QT_NO_QWS_INPUTMETHODS

/*!
    \fn  QWSWindow::QWSWindow(int i, QWSClient * client)
    \internal

    Constructs a new top-level window, associated with the client \a
    client and giving it the id \a i.
*/

/*!
    \fn QWSServer::windowEvent(QWSWindow * window, QWSServer::WindowEvent eventType)

    This signal is emitted whenever something happens to a top-level
    window (e.g., it's created or destroyed), passing a pointer to the
    window and the event's type in the \a window and \a eventType
    parameters, respectively.

    \sa markedText()
*/

/*!
    \class QWSServer::KeyboardFilter
    \ingroup qws

    \brief The KeyboardFilter class is a base class for global
    keyboard event filters in Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    In \l{Qt for Embedded Linux}, all system generated events, including
    keyboard events, are passed to the server application which then
    propagates the event to the appropriate client. The KeyboardFilter
    class is used to implement a global, low-level filter on the
    server side. The server applies the filter to all keyboard events
    before passing them on to the clients:

    \image qwsserver_keyboardfilter.png

    This feature can, for example, be used to filter things like APM
    (advanced power management) suspended from a button without having
    to filter for it in all applications.

    To add a new keyboard filter you must first create the filter by
    deriving from this class, reimplementing the pure virtual filter()
    function. Then you can install the filter on the server using
    QWSServer's \l {QWSServer::}{addKeyboardFilter()}
    function. QWSServer also provides a \l
    {QWSServer::}{removeKeyboardFilter()} function.

    \sa {Qt for Embedded Linux Architecture}, QWSServer, QWSInputMethod
*/

/*!
    \fn QWSServer::KeyboardFilter::~KeyboardFilter()

    Destroys the keyboard filter.
*/

/*!
    \fn bool QWSServer::KeyboardFilter::filter(int unicode, int keycode, int modifiers, bool isPress, bool autoRepeat)

    Implement this function to return true if a given key event should
    be stopped from being processed any further; otherwise it should
    return false.

    A key event can be identified by the given \a unicode value and
    the \a keycode, \a modifiers, \a isPress and \a autoRepeat
    parameters.

    The \a keycode parameter is the Qt keycode value as defined by the
    Qt::Key enum. The \a modifiers is an OR combination of
    Qt::KeyboardModifier values, indicating whether \gui
    Shift/Alt/Ctrl keys are pressed. The \a isPress parameter is true
    if the event is a key press event and \a autoRepeat is true if the
    event is caused by an auto-repeat mechanism and not an actual key
    press.
*/

QT_END_NAMESPACE

#include "moc_qwindowsystem_qws.cpp"
