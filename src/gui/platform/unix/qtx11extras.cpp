// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Richard Moore <rich@kde.org>
// Copyright (C) 2016 David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtx11extras_p.h"

#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformscreen_p.h>
#include <qpa/qplatformscreen.h>
#include <qscreen.h>
#include <qwindow.h>
#include <qguiapplication.h>
#include <xcb/xcb.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QScreen *findScreenForVirtualDesktop(int virtualDesktopNumber)
{
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        auto *qxcbScreen = dynamic_cast<QNativeInterface::Private::QXcbScreen *>(screen->handle());
        if (qxcbScreen && qxcbScreen->virtualDesktopNumber() == virtualDesktopNumber)
            return screen;
    }
    return nullptr;
}

/*!
    \class QX11Info
    \inmodule QtGui
    \since 6.2
    \internal

    \brief Provides information about the X display configuration.

    The class provides two APIs: a set of non-static functions that
    provide information about a specific widget or pixmap, and a set
    of static functions that provide the default information for the
    application.

    \warning This class is only available on X11. For querying
    per-screen information in a portable way, use QScreen.
*/

/*!
    Constructs an empty QX11Info object.
*/
QX11Info::QX11Info()
{
}

/*!
    Returns true if the application is currently running on X11.

    \since 6.2
 */
bool QX11Info::isPlatformX11()
{
    return QGuiApplication::platformName() == "xcb"_L1;
}

/*!
    Returns the horizontal resolution of the given \a screen in terms of the
    number of dots per inch.

    The \a screen argument is an X screen number. Be aware that if
    the user's system uses Xinerama (as opposed to traditional X11
    multiscreen), there is only one X screen. Use QScreen to
    query for information about Xinerama screens.

    \sa appDpiY()
*/
int QX11Info::appDpiX(int screen)
{
    if (screen == -1) {
        const QScreen *scr = QGuiApplication::primaryScreen();
        if (!scr)
            return 75;
        return qRound(scr->logicalDotsPerInchX());
    }

    QScreen *scr = findScreenForVirtualDesktop(screen);
    if (!scr)
        return 0;

    return scr->logicalDotsPerInchX();
}

/*!
    Returns the vertical resolution of the given \a screen in terms of the
    number of dots per inch.

    The \a screen argument is an X screen number. Be aware that if
    the user's system uses Xinerama (as opposed to traditional X11
    multiscreen), there is only one X screen. Use QScreen to
    query for information about Xinerama screens.

    \sa appDpiX()
*/
int QX11Info::appDpiY(int screen)
{
    if (screen == -1) {
        const QScreen *scr = QGuiApplication::primaryScreen();
        if (!scr)
            return 75;
        return qRound(scr->logicalDotsPerInchY());
    }

    QScreen *scr = findScreenForVirtualDesktop(screen);
    if (!scr)
        return 0;

    return scr->logicalDotsPerInchY();
}

/*!
    Returns a handle for the applications root window on the given \a screen.

    The \a screen argument is an X screen number. Be aware that if
    the user's system uses Xinerama (as opposed to traditional X11
    multiscreen), there is only one X screen. Use QScreen to
    query for information about Xinerama screens.
*/
quint32 QX11Info::appRootWindow(int screen)
{
    if (!qApp)
        return 0;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return 0;
    QScreen *scr = screen == -1 ?  QGuiApplication::primaryScreen() : findScreenForVirtualDesktop(screen);
    if (!scr)
        return 0;
    return static_cast<xcb_window_t>(reinterpret_cast<quintptr>(native->nativeResourceForScreen(QByteArrayLiteral("rootwindow"), scr)));
}

/*!
    Returns the number of the screen where the application is being
    displayed.

    This method refers to screens in the original X11 meaning with a
    different DISPLAY environment variable per screen.
    This information is only useful if your application needs to know
    on which X screen it is running.

    In a typical multi-head configuration, multiple physical monitors
    are combined in one X11 screen. This means this method returns the
    same number for each of the physical monitors. In such a setup you
    are interested in the monitor information as provided by the X11
    RandR extension. This is available through QScreen.

    \sa display()
*/
int QX11Info::appScreen()
{
    if (!qApp)
        return 0;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return 0;
    return reinterpret_cast<qintptr>(native->nativeResourceForIntegration(QByteArrayLiteral("x11screen")));
}

/*!
    Returns the X11 time.

    \sa setAppTime(), appUserTime()
*/
quint32 QX11Info::appTime()
{
    if (!qApp)
        return 0;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return 0;
    QScreen* screen = QGuiApplication::primaryScreen();
    return static_cast<xcb_timestamp_t>(reinterpret_cast<quintptr>(native->nativeResourceForScreen("apptime", screen)));
}

/*!
    Returns the X11 user time.

    \sa setAppUserTime(), appTime()
*/
quint32 QX11Info::appUserTime()
{
    if (!qApp)
        return 0;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return 0;
    QScreen* screen = QGuiApplication::primaryScreen();
    return static_cast<xcb_timestamp_t>(reinterpret_cast<quintptr>(native->nativeResourceForScreen("appusertime", screen)));
}

/*!
    Sets the X11 time to the value specified by \a time.

    \sa appTime(), setAppUserTime()
*/
void QX11Info::setAppTime(quint32 time)
{
    if (!qApp)
        return;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return;
    typedef void (*SetAppTimeFunc)(QScreen *, xcb_timestamp_t);
    QScreen* screen = QGuiApplication::primaryScreen();
    SetAppTimeFunc func = reinterpret_cast<SetAppTimeFunc>(reinterpret_cast<void *>(native->nativeResourceFunctionForScreen("setapptime")));
    if (func)
        func(screen, time);
    else
        qWarning("Internal error: QPA plugin doesn't implement setAppTime");
}

/*!
    Sets the X11 user time as specified by \a time.

    \sa appUserTime(), setAppTime()
*/
void QX11Info::setAppUserTime(quint32 time)
{
    if (!qApp)
        return;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return;
    typedef void (*SetAppUserTimeFunc)(QScreen *, xcb_timestamp_t);
    QScreen* screen = QGuiApplication::primaryScreen();
    SetAppUserTimeFunc func = reinterpret_cast<SetAppUserTimeFunc>(reinterpret_cast<void *>(native->nativeResourceFunctionForScreen("setappusertime")));
    if (func)
        func(screen, time);
    else
        qWarning("Internal error: QPA plugin doesn't implement setAppUserTime");
}

/*!
    Fetches the current X11 time stamp from the X Server.

    This method creates a property notify event and blocks till it is
    received back from the X Server.
*/
quint32 QX11Info::getTimestamp()
{
    if (!qApp)
        return 0;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return 0;
    QScreen* screen = QGuiApplication::primaryScreen();
    return static_cast<xcb_timestamp_t>(reinterpret_cast<quintptr>(native->nativeResourceForScreen("gettimestamp", screen)));
}

/*!
    Returns the startup ID that will be used for the next window to be shown by this process.

    After the next window is shown, the next startup ID will be empty.

    http://standards.freedesktop.org/startup-notification-spec/startup-notification-latest.txt

    \sa setNextStartupId()
*/
QByteArray QX11Info::nextStartupId()
{
    if (!qApp)
        return QByteArray();
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return QByteArray();
    return static_cast<char *>(native->nativeResourceForIntegration("startupid"));
}

/*!
    Sets the next startup ID to \a id.

    This is the startup ID that will be used for the next window to be shown by this process.

    The startup ID of the first window comes from the environment variable DESKTOP_STARTUP_ID.
    This method is useful for subsequent windows, when the request comes from another process
    (e.g. via DBus).

    \sa nextStartupId()
*/
void QX11Info::setNextStartupId(const QByteArray &id)
{
    if (!qApp)
        return;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return;
    typedef void (*SetStartupIdFunc)(const char*);
    SetStartupIdFunc func = reinterpret_cast<SetStartupIdFunc>(reinterpret_cast<void *>(native->nativeResourceFunctionForIntegration("setstartupid")));
    if (func)
        func(id.constData());
    else
        qWarning("Internal error: QPA plugin doesn't implement setStartupId");
}

/*!
    Returns the default display for the application.

    \sa appScreen()
*/
Display *QX11Info::display()
{
    if (!qApp)
        return nullptr;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return nullptr;

    void *display = native->nativeResourceForIntegration(QByteArray("display"));
    return reinterpret_cast<Display *>(display);
}

/*!
    Returns the default XCB connection for the application.

    \sa display()
*/
xcb_connection_t *QX11Info::connection()
{
    if (!qApp)
        return nullptr;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return nullptr;

    void *connection = native->nativeResourceForIntegration(QByteArray("connection"));
    return reinterpret_cast<xcb_connection_t *>(connection);
}

/*!
    Returns true if there is a compositing manager running for the connection
    attached to \a screen.

    If \a screen equals -1, the application's primary screen is used.
*/
bool QX11Info::isCompositingManagerRunning(int screen)
{
    if (!qApp)
        return false;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return false;

    QScreen *scr = screen == -1 ?  QGuiApplication::primaryScreen() : findScreenForVirtualDesktop(screen);
    if (!scr) {
        qWarning() << "isCompositingManagerRunning: Could not find screen number" << screen;
        return false;
    }

    return native->nativeResourceForScreen(QByteArray("compositingEnabled"), scr);
}

/*!
    Returns a new peeker id or -1 if some internal error has occurred.
    Each peeker id is associated with an index in the buffered native
    event queue.

    For more details see QX11Info::PeekOption and peekEventQueue().

    \sa peekEventQueue(), removePeekerId()
*/
qint32 QX11Info::generatePeekerId()
{
    if (!qApp)
        return -1;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return -1;

    typedef qint32 (*GeneratePeekerIdFunc)(void);
    GeneratePeekerIdFunc generatepeekerid = reinterpret_cast<GeneratePeekerIdFunc>(
                reinterpret_cast<void *>(native->nativeResourceFunctionForIntegration("generatepeekerid")));
    if (!generatepeekerid) {
        qWarning("Internal error: QPA plugin doesn't implement generatePeekerId");
        return -1;
    }

    return generatepeekerid();
}

/*!
    Removes \a peekerId, which was earlier obtained via generatePeekerId().

    Returns \c true on success or \c false if unknown peeker id was
    provided or some internal error has occurred.

    \sa generatePeekerId()
*/
bool QX11Info::removePeekerId(qint32 peekerId)
{
    if (!qApp)
        return false;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return false;

    typedef bool (*RemovePeekerIdFunc)(qint32);
    RemovePeekerIdFunc removePeekerId = reinterpret_cast<RemovePeekerIdFunc>(
                reinterpret_cast<void *>(native->nativeResourceFunctionForIntegration("removepeekerid")));
    if (!removePeekerId) {
        qWarning("Internal error: QPA plugin doesn't implement removePeekerId");
        return false;
    }

    return removePeekerId(peekerId);
}

/*!
    \enum QX11Info::PeekOption
    \brief An enum to tune the behavior of QX11Info::peekEventQueue().

    \value PeekDefault
    Peek from the beginning of the buffered native event queue. A peeker
    id is optional with PeekDefault. If a peeker id is provided to
    peekEventQueue() when using PeekDefault, then peeking starts from
    the beginning of the queue, not from the cached index; thus, this
    can be used to manually reset a cached index to peek from the start
    of the queue. When this operation completes, the associated index
    will be updated to the new position in the queue.

    \value PeekFromCachedIndex
    QX11Info::peekEventQueue() can optimize the peeking algorithm by
    skipping events that it already has seen in earlier calls to
    peekEventQueue(). When control returns to the main event loop,
    which causes the buffered native event queue to be flushed to Qt's
    event queue, the cached indices are marked invalid and will be
    reset on the next access. The same is true if the program
    explicitly flushes the buffered native event queue by
    QCoreApplication::processEvents().
*/

/*!
    \typedef QX11Info::PeekerCallback
    Typedef for a pointer to a function with the following signature:

    \code
    bool (*PeekerCallback)(xcb_generic_event_t *event, void *peekerData);
    \endcode

    The \a event is a native XCB event.
    The \a peekerData is a pointer to data, passed in via peekEventQueue().

    Return \c true from this function to stop examining the buffered
    native event queue or \c false to continue.

    \note A non-capturing lambda can serve as a PeekerCallback.
*/

/*!
    \brief Peek into the buffered XCB event queue.

    You can call peekEventQueue() periodically, when your program is busy
    performing a long-running operation, to peek into the buffered native
    event queue. The more time the long-running operation blocks the
    program from returning control to the main event loop, the more
    events will accumulate in the buffered XCB event queue. Once control
    returns to the main event loop these events will be flushed to Qt's
    event queue, which is a separate event queue from the queue this
    function is peeking into.

    \note It is usually better to run CPU-intensive operations in a
    non-GUI thread, instead of blocking the main event loop.

    The buffered XCB event queue is populated from a non-GUI thread and
    therefore might be ahead of the current GUI state. To handle native
    events as they are processed by the GUI thread, see
    QAbstractNativeEventFilter::nativeEventFilter().

    The \a peeker is a callback function as documented in PeekerCallback.
    The \a peekerData can be used to pass in arbitrary data to the \a
    peeker callback.
    The \a option is an enum that tunes the behavior of peekEventQueue().
    The \a peekerId is used to track an index in the queue, for more
    details see QX11Info::PeekOption. There can be several indices,
    each tracked individually by a peeker id obtained via generatePeekerId().

    This function returns \c true when the peeker has stopped the event
    proccesing by returning \c true from the callback. If there were no
    events in the buffered native event queue to peek at or all the
    events have been processed by the peeker, this function returns \c
    false.

    \sa generatePeekerId(), QAbstractNativeEventFilter::nativeEventFilter()
*/
bool QX11Info::peekEventQueue(PeekerCallback peeker, void *peekerData, PeekOptions option,
                              qint32 peekerId)
{
    if (!peeker || !qApp)
        return false;
    QPlatformNativeInterface *native = qApp->platformNativeInterface();
    if (!native)
        return false;

    typedef bool (*PeekEventQueueFunc)(PeekerCallback, void *, PeekOptions, qint32);
    PeekEventQueueFunc peekeventqueue = reinterpret_cast<PeekEventQueueFunc>(
                reinterpret_cast<void *>(native->nativeResourceFunctionForIntegration("peekeventqueue")));
    if (!peekeventqueue) {
        qWarning("Internal error: QPA plugin doesn't implement peekEventQueue");
        return false;
    }

    return peekeventqueue(peeker, peekerData, option, peekerId);
}

QT_END_NAMESPACE
