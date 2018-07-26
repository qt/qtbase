/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtCore/QDebug>

#include "qxcbconnection.h"
#include "qxcbkeyboard.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbclipboard.h"
#if QT_CONFIG(draganddrop)
#include "qxcbdrag.h"
#endif
#include "qxcbwmsupport.h"
#include "qxcbnativeinterface.h"
#include "qxcbintegration.h"
#include "qxcbsystemtraytracker.h"
#include "qxcbglintegrationfactory.h"
#include "qxcbglintegration.h"
#include "qxcbbackingstore.h"

#include <QSocketNotifier>
#include <QAbstractEventDispatcher>
#include <QTimer>
#include <QByteArray>
#include <QScopedPointer>

#include <algorithm>

#include <stdio.h>
#include <errno.h>
#include <xcb/shm.h>
#include <xcb/sync.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>

#if QT_CONFIG(xcb_xlib)
#define register        /* C++17 deprecated register */
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#undef register
#endif

#if QT_CONFIG(xinput2)
#include <X11/extensions/XI2proto.h>
#endif

#if QT_CONFIG(xcb_render)
#include <xcb/render.h>
#endif

#if defined(Q_CC_GNU) && defined(Q_OF_ELF)
static xcb_generic_event_t *local_xcb_poll_for_queued_event(xcb_connection_t *c)
    __attribute__((weakref("xcb_poll_for_queued_event")));

static inline void checkXcbPollForQueuedEvent()
{ }
#else
#include <dlfcn.h>
typedef xcb_generic_event_t * (*XcbPollForQueuedEventFunctionPointer)(xcb_connection_t *c);
static XcbPollForQueuedEventFunctionPointer local_xcb_poll_for_queued_event;

static inline void checkXcbPollForQueuedEvent()
{
#ifdef RTLD_DEFAULT
    local_xcb_poll_for_queued_event = (XcbPollForQueuedEventFunctionPointer)dlsym(RTLD_DEFAULT, "xcb_poll_for_queued_event");
#endif
}
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaXInput, "qt.qpa.input")
Q_LOGGING_CATEGORY(lcQpaXInputDevices, "qt.qpa.input.devices")
Q_LOGGING_CATEGORY(lcQpaXInputEvents, "qt.qpa.input.events")
Q_LOGGING_CATEGORY(lcQpaScreen, "qt.qpa.screen")
Q_LOGGING_CATEGORY(lcQpaEvents, "qt.qpa.events")
Q_LOGGING_CATEGORY(lcQpaXcb, "qt.qpa.xcb") // for general (uncategorized) XCB logging
Q_LOGGING_CATEGORY(lcQpaPeeker, "qt.qpa.peeker")
Q_LOGGING_CATEGORY(lcQpaKeyboard, "qt.qpa.xkeyboard")

// this event type was added in libxcb 1.10,
// but we support also older version
#ifndef XCB_GE_GENERIC
#define XCB_GE_GENERIC 35
#endif

#if QT_CONFIG(xinput2)
// Starting from the xcb version 1.9.3 struct xcb_ge_event_t has changed:
// - "pad0" became "extension"
// - "pad1" and "pad" became "pad0"
// New and old version of this struct share the following fields:
typedef struct qt_xcb_ge_event_t {
    uint8_t  response_type;
    uint8_t  extension;
    uint16_t sequence;
    uint32_t length;
    uint16_t event_type;
} qt_xcb_ge_event_t;

static inline bool isXIEvent(xcb_generic_event_t *event, int opCode)
{
    qt_xcb_ge_event_t *e = reinterpret_cast<qt_xcb_ge_event_t *>(event);
    return e->extension == opCode;
}
#endif // QT_CONFIG(xinput2)

#if QT_CONFIG(xcb_xlib)
static const char * const xcbConnectionErrors[] = {
    "No error", /* Error 0 */
    "I/O error", /* XCB_CONN_ERROR */
    "Unsupported extension used", /* XCB_CONN_CLOSED_EXT_NOTSUPPORTED */
    "Out of memory", /* XCB_CONN_CLOSED_MEM_INSUFFICIENT */
    "Maximum allowed requested length exceeded", /* XCB_CONN_CLOSED_REQ_LEN_EXCEED */
    "Failed to parse display string", /* XCB_CONN_CLOSED_PARSE_ERR */
    "No such screen on display", /* XCB_CONN_CLOSED_INVALID_SCREEN */
    "Error during FD passing" /* XCB_CONN_CLOSED_FDPASSING_FAILED */
};

static int nullErrorHandler(Display *dpy, XErrorEvent *err)
{
#ifndef Q_XCB_DEBUG
    Q_UNUSED(dpy);
    Q_UNUSED(err);
#else
    const int buflen = 1024;
    char buf[buflen];

    XGetErrorText(dpy, err->error_code, buf, buflen);
    fprintf(stderr, "X Error: serial %lu error %d %s\n", err->serial, (int) err->error_code, buf);
#endif
    return 0;
}

static int ioErrorHandler(Display *dpy)
{
    xcb_connection_t *conn = XGetXCBConnection(dpy);
    if (conn != NULL) {
        /* Print a message with a textual description of the error */
        int code = xcb_connection_has_error(conn);
        const char *str = "Unknown error";
        int arrayLength = sizeof(xcbConnectionErrors) / sizeof(xcbConnectionErrors[0]);
        if (code >= 0 && code < arrayLength)
            str = xcbConnectionErrors[code];

        qWarning("The X11 connection broke: %s (code %d)", str, code);
    }
    return _XDefaultIOError(dpy);
}
#endif

QXcbScreen* QXcbConnection::findScreenForCrtc(xcb_window_t rootWindow, xcb_randr_crtc_t crtc) const
{
    for (QXcbScreen *screen : m_screens) {
        if (screen->root() == rootWindow && screen->crtc() == crtc)
            return screen;
    }

    return 0;
}

QXcbScreen* QXcbConnection::findScreenForOutput(xcb_window_t rootWindow, xcb_randr_output_t output) const
{
    for (QXcbScreen *screen : m_screens) {
        if (screen->root() == rootWindow && screen->output() == output)
            return screen;
    }

    return 0;
}

QXcbVirtualDesktop* QXcbConnection::virtualDesktopForRootWindow(xcb_window_t rootWindow) const
{
    for (QXcbVirtualDesktop *virtualDesktop : m_virtualDesktops) {
        if (virtualDesktop->screen()->root == rootWindow)
            return virtualDesktop;
    }

    return 0;
}

/*!
    \brief Synchronizes the screen list, adds new screens, removes deleted ones
*/
void QXcbConnection::updateScreens(const xcb_randr_notify_event_t *event)
{
    if (event->subCode == XCB_RANDR_NOTIFY_CRTC_CHANGE) {
        xcb_randr_crtc_change_t crtc = event->u.cc;
        QXcbVirtualDesktop *virtualDesktop = virtualDesktopForRootWindow(crtc.window);
        if (!virtualDesktop)
            // Not for us
            return;

        QXcbScreen *screen = findScreenForCrtc(crtc.window, crtc.crtc);
        qCDebug(lcQpaScreen) << "QXcbConnection: XCB_RANDR_NOTIFY_CRTC_CHANGE:" << crtc.crtc
                             << "mode" << crtc.mode << "relevant screen" << screen;
        // Only update geometry when there's a valid mode on the CRTC
        // CRTC with node mode could mean that output has been disabled, and we'll
        // get RRNotifyOutputChange notification for that.
        if (screen && crtc.mode) {
            if (crtc.rotation == XCB_RANDR_ROTATION_ROTATE_90 ||
                crtc.rotation == XCB_RANDR_ROTATION_ROTATE_270)
                std::swap(crtc.width, crtc.height);
            screen->updateGeometry(QRect(crtc.x, crtc.y, crtc.width, crtc.height), crtc.rotation);
            if (screen->mode() != crtc.mode)
                screen->updateRefreshRate(crtc.mode);
        }

    } else if (event->subCode == XCB_RANDR_NOTIFY_OUTPUT_CHANGE) {
        xcb_randr_output_change_t output = event->u.oc;
        QXcbVirtualDesktop *virtualDesktop = virtualDesktopForRootWindow(output.window);
        if (!virtualDesktop)
            // Not for us
            return;

        QXcbScreen *screen = findScreenForOutput(output.window, output.output);
        qCDebug(lcQpaScreen) << "QXcbConnection: XCB_RANDR_NOTIFY_OUTPUT_CHANGE:" << output.output;

        if (screen && output.connection == XCB_RANDR_CONNECTION_DISCONNECTED) {
            qCDebug(lcQpaScreen) << "screen" << screen->name() << "has been disconnected";
            destroyScreen(screen);
        } else if (!screen && output.connection == XCB_RANDR_CONNECTION_CONNECTED) {
            // New XRandR output is available and it's enabled
            if (output.crtc != XCB_NONE && output.mode != XCB_NONE) {
                auto outputInfo = Q_XCB_REPLY(xcb_randr_get_output_info, xcb_connection(),
                                              output.output, output.config_timestamp);
                // Find a fake screen
                const auto scrs = virtualDesktop->screens();
                for (QPlatformScreen *scr : scrs) {
                    QXcbScreen *xcbScreen = static_cast<QXcbScreen *>(scr);
                    if (xcbScreen->output() == XCB_NONE) {
                        screen = xcbScreen;
                        break;
                    }
                }

                if (screen) {
                    QString nameWas = screen->name();
                    // Transform the fake screen into a physical screen
                    screen->setOutput(output.output, outputInfo.get());
                    updateScreen(screen, output);
                    qCDebug(lcQpaScreen) << "output" << screen->name()
                                         << "is connected and enabled; was fake:" << nameWas;
                } else {
                    screen = createScreen(virtualDesktop, output, outputInfo.get());
                    qCDebug(lcQpaScreen) << "output" << screen->name() << "is connected and enabled";
                }
                QHighDpiScaling::updateHighDpiScaling();
            }
        } else if (screen) {
            if (output.crtc == XCB_NONE && output.mode == XCB_NONE) {
                // Screen has been disabled
                auto outputInfo = Q_XCB_REPLY(xcb_randr_get_output_info, xcb_connection(),
                                              output.output, output.config_timestamp);
                if (outputInfo->crtc == XCB_NONE) {
                    qCDebug(lcQpaScreen) << "output" << screen->name() << "has been disabled";
                    destroyScreen(screen);
                } else {
                    qCDebug(lcQpaScreen) << "output" << screen->name() << "has been temporarily disabled for the mode switch";
                    // Reset crtc to skip RRCrtcChangeNotify events,
                    // because they may be invalid in the middle of the mode switch
                    screen->setCrtc(XCB_NONE);
                }
            } else {
                updateScreen(screen, output);
                qCDebug(lcQpaScreen) << "output has changed" << screen;
            }
        }

        qCDebug(lcQpaScreen) << "primary output is" << qAsConst(m_screens).first()->name();
    }
}

bool QXcbConnection::checkOutputIsPrimary(xcb_window_t rootWindow, xcb_randr_output_t output)
{
    auto primary = Q_XCB_REPLY(xcb_randr_get_output_primary, xcb_connection(), rootWindow);
    if (!primary)
        qWarning("failed to get the primary output of the screen");

    const bool isPrimary = primary ? (primary->output == output) : false;

    return isPrimary;
}

void QXcbConnection::updateScreen(QXcbScreen *screen, const xcb_randr_output_change_t &outputChange)
{
    screen->setCrtc(outputChange.crtc); // Set the new crtc, because it can be invalid
    screen->updateGeometry(outputChange.config_timestamp);
    if (screen->mode() != outputChange.mode)
        screen->updateRefreshRate(outputChange.mode);
    // Only screen which belongs to the primary virtual desktop can be a primary screen
    if (screen->screenNumber() == m_primaryScreenNumber) {
        if (!screen->isPrimary() && checkOutputIsPrimary(outputChange.window, outputChange.output)) {
            screen->setPrimary(true);

            // If the screen became primary, reshuffle the order in QGuiApplicationPrivate
            const int idx = m_screens.indexOf(screen);
            if (idx > 0) {
                qAsConst(m_screens).first()->setPrimary(false);
                m_screens.swap(0, idx);
            }
            screen->virtualDesktop()->setPrimaryScreen(screen);
            QXcbIntegration::instance()->setPrimaryScreen(screen);
        }
    }
}

QXcbScreen *QXcbConnection::createScreen(QXcbVirtualDesktop *virtualDesktop,
                                         const xcb_randr_output_change_t &outputChange,
                                         xcb_randr_get_output_info_reply_t *outputInfo)
{
    QXcbScreen *screen = new QXcbScreen(this, virtualDesktop, outputChange.output, outputInfo);
    // Only screen which belongs to the primary virtual desktop can be a primary screen
    if (screen->screenNumber() == m_primaryScreenNumber)
        screen->setPrimary(checkOutputIsPrimary(outputChange.window, outputChange.output));

    if (screen->isPrimary()) {
        if (!m_screens.isEmpty())
            qAsConst(m_screens).first()->setPrimary(false);

        m_screens.prepend(screen);
    } else {
        m_screens.append(screen);
    }
    virtualDesktop->addScreen(screen);
    QXcbIntegration::instance()->screenAdded(screen, screen->isPrimary());

    return screen;
}

void QXcbConnection::destroyScreen(QXcbScreen *screen)
{
    QXcbVirtualDesktop *virtualDesktop = screen->virtualDesktop();
    if (virtualDesktop->screens().count() == 1) {
        // If there are no other screens on the same virtual desktop,
        // then transform the physical screen into a fake screen.
        const QString nameWas = screen->name();
        screen->setOutput(XCB_NONE, nullptr);
        qCDebug(lcQpaScreen) << "transformed" << nameWas << "to fake" << screen;
    } else {
        // There is more than one screen on the same virtual desktop, remove the screen
        m_screens.removeOne(screen);
        virtualDesktop->removeScreen(screen);

        // When primary screen is removed, set the new primary screen
        // which belongs to the primary virtual desktop.
        if (screen->isPrimary()) {
            QXcbScreen *newPrimary = static_cast<QXcbScreen *>(virtualDesktop->screens().at(0));
            newPrimary->setPrimary(true);
            const int idx = m_screens.indexOf(newPrimary);
            if (idx > 0)
                m_screens.swap(0, idx);
            QXcbIntegration::instance()->setPrimaryScreen(newPrimary);
        }

        QXcbIntegration::instance()->destroyScreen(screen);
    }
}

void QXcbConnection::initializeScreens()
{
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(m_setup);
    int xcbScreenNumber = 0;    // screen number in the xcb sense
    QXcbScreen *primaryScreen = nullptr;
    while (it.rem) {
        // Each "screen" in xcb terminology is a virtual desktop,
        // potentially a collection of separate juxtaposed monitors.
        // But we want a separate QScreen for each output (e.g. DVI-I-1, VGA-1, etc.)
        // which will become virtual siblings.
        xcb_screen_t *xcbScreen = it.data;
        QXcbVirtualDesktop *virtualDesktop = new QXcbVirtualDesktop(this, xcbScreen, xcbScreenNumber);
        m_virtualDesktops.append(virtualDesktop);
        QList<QPlatformScreen *> siblings;
        if (has_randr_extension) {
            // RRGetScreenResourcesCurrent is fast but it may return nothing if the
            // configuration is not initialized wrt to the hardware. We should call
            // RRGetScreenResources in this case.
            auto resources_current = Q_XCB_REPLY(xcb_randr_get_screen_resources_current,
                                                 xcb_connection(), xcbScreen->root);
            if (!resources_current) {
                qWarning("failed to get the current screen resources");
            } else {
                xcb_timestamp_t timestamp = 0;
                xcb_randr_output_t *outputs = nullptr;
                int outputCount = xcb_randr_get_screen_resources_current_outputs_length(resources_current.get());
                if (outputCount) {
                    timestamp = resources_current->config_timestamp;
                    outputs = xcb_randr_get_screen_resources_current_outputs(resources_current.get());
                } else {
                    auto resources = Q_XCB_REPLY(xcb_randr_get_screen_resources,
                                                 xcb_connection(), xcbScreen->root);
                    if (!resources) {
                        qWarning("failed to get the screen resources");
                    } else {
                        timestamp = resources->config_timestamp;
                        outputCount = xcb_randr_get_screen_resources_outputs_length(resources.get());
                        outputs = xcb_randr_get_screen_resources_outputs(resources.get());
                    }
                }

                if (outputCount) {
                    auto primary = Q_XCB_REPLY(xcb_randr_get_output_primary, xcb_connection(), xcbScreen->root);
                    if (!primary) {
                        qWarning("failed to get the primary output of the screen");
                    } else {
                        for (int i = 0; i < outputCount; i++) {
                            auto output = Q_XCB_REPLY_UNCHECKED(xcb_randr_get_output_info,
                                                                xcb_connection(), outputs[i], timestamp);
                            // Invalid, disconnected or disabled output
                            if (!output)
                                continue;

                            if (output->connection != XCB_RANDR_CONNECTION_CONNECTED) {
                                qCDebug(lcQpaScreen, "Output %s is not connected", qPrintable(
                                            QString::fromUtf8((const char*)xcb_randr_get_output_info_name(output.get()),
                                                              xcb_randr_get_output_info_name_length(output.get()))));
                                continue;
                            }

                            if (output->crtc == XCB_NONE) {
                                qCDebug(lcQpaScreen, "Output %s is not enabled", qPrintable(
                                            QString::fromUtf8((const char*)xcb_randr_get_output_info_name(output.get()),
                                                              xcb_randr_get_output_info_name_length(output.get()))));
                                continue;
                            }

                            QXcbScreen *screen = new QXcbScreen(this, virtualDesktop, outputs[i], output.get());
                            siblings << screen;
                            m_screens << screen;

                            // There can be multiple outputs per screen, use either
                            // the first or an exact match.  An exact match isn't
                            // always available if primary->output is XCB_NONE
                            // or currently disconnected output.
                            if (m_primaryScreenNumber == xcbScreenNumber) {
                                if (!primaryScreen || (primary && outputs[i] == primary->output)) {
                                    if (primaryScreen)
                                        primaryScreen->setPrimary(false);
                                    primaryScreen = screen;
                                    primaryScreen->setPrimary(true);
                                    siblings.prepend(siblings.takeLast());
                                }
                            }
                        }
                    }
                }
            }
        } else if (has_xinerama_extension) {
            // Xinerama is available
            auto screens = Q_XCB_REPLY(xcb_xinerama_query_screens, m_connection);
            if (screens) {
                xcb_xinerama_screen_info_iterator_t it = xcb_xinerama_query_screens_screen_info_iterator(screens.get());
                while (it.rem) {
                    xcb_xinerama_screen_info_t *screen_info = it.data;
                    QXcbScreen *screen = new QXcbScreen(this, virtualDesktop,
                                                        XCB_NONE, nullptr,
                                                        screen_info, it.index);
                    siblings << screen;
                    m_screens << screen;
                    xcb_xinerama_screen_info_next(&it);
                }
            }
        }
        if (siblings.isEmpty()) {
            // If there are no XRandR outputs or XRandR extension is missing,
            // then create a fake/legacy screen.
            QXcbScreen *screen = new QXcbScreen(this, virtualDesktop, XCB_NONE, nullptr);
            qCDebug(lcQpaScreen) << "created fake screen" << screen;
            m_screens << screen;
            if (m_primaryScreenNumber == xcbScreenNumber) {
                primaryScreen = screen;
                primaryScreen->setPrimary(true);
            }
            siblings << screen;
        }
        virtualDesktop->setScreens(siblings);
        xcb_screen_next(&it);
        ++xcbScreenNumber;
    } // for each xcb screen

    for (QXcbVirtualDesktop *virtualDesktop : qAsConst(m_virtualDesktops))
        virtualDesktop->subscribeToXFixesSelectionNotify();

    if (m_virtualDesktops.isEmpty()) {
        qFatal("QXcbConnection: no screens available");
    } else {
        // Ensure the primary screen is first on the list
        if (primaryScreen) {
            if (qAsConst(m_screens).first() != primaryScreen) {
                m_screens.removeOne(primaryScreen);
                m_screens.prepend(primaryScreen);
            }
        }

        // Push the screens to QGuiApplication
        for (QXcbScreen *screen : qAsConst(m_screens)) {
            qCDebug(lcQpaScreen) << "adding" << screen << "(Primary:" << screen->isPrimary() << ")";
            QXcbIntegration::instance()->screenAdded(screen, screen->isPrimary());
        }

        qCDebug(lcQpaScreen) << "primary output is" << qAsConst(m_screens).first()->name();
    }
}

QXcbConnection::QXcbConnection(QXcbNativeInterface *nativeInterface, bool canGrabServer, xcb_visualid_t defaultVisualId, const char *displayName)
    : m_canGrabServer(canGrabServer)
    , m_defaultVisualId(defaultVisualId)
    , m_displayName(displayName ? QByteArray(displayName) : qgetenv("DISPLAY"))
    , m_nativeInterface(nativeInterface)
{
#if QT_CONFIG(xcb_xlib)
    Display *dpy = XOpenDisplay(m_displayName.constData());
    if (dpy) {
        m_primaryScreenNumber = DefaultScreen(dpy);
        m_connection = XGetXCBConnection(dpy);
        XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
        XSetErrorHandler(nullErrorHandler);
        XSetIOErrorHandler(ioErrorHandler);
        m_xlib_display = dpy;
    }
#else
    m_connection = xcb_connect(m_displayName.constData(), &m_primaryScreenNumber);
#endif // QT_CONFIG(xcb_xlib)

    if (Q_UNLIKELY(!m_connection || xcb_connection_has_error(m_connection))) {
        qCWarning(lcQpaScreen, "QXcbConnection: Could not connect to display %s", m_displayName.constData());
        return;
    }

    m_reader = new QXcbEventReader(this);
    m_reader->start();

    xcb_extension_t *extensions[] = {
        &xcb_shm_id, &xcb_xfixes_id, &xcb_randr_id, &xcb_shape_id, &xcb_sync_id,
#if QT_CONFIG(xkb)
        &xcb_xkb_id,
#endif
#if QT_CONFIG(xcb_render)
        &xcb_render_id,
#endif
        0
    };

    for (xcb_extension_t **ext_it = extensions; *ext_it; ++ext_it)
        xcb_prefetch_extension_data (m_connection, *ext_it);

    m_setup = xcb_get_setup(xcb_connection());

    m_xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP").toLower();

    initializeAllAtoms();

    if (!qEnvironmentVariableIsSet("QT_XCB_NO_MITSHM"))
        initializeShm();
    if (!qEnvironmentVariableIsSet("QT_XCB_NO_XRANDR"))
        initializeXRandr();
    if (!has_randr_extension)
        initializeXinerama();
    initializeXFixes();
    initializeScreens();

    initializeXRender();
#if QT_CONFIG(xinput2)
    if (!qEnvironmentVariableIsSet("QT_XCB_NO_XI2"))
        initializeXInput2();
#endif
    initializeXShape();
    initializeXKB();

    m_wmSupport.reset(new QXcbWMSupport(this));
    m_keyboard = new QXcbKeyboard(this);
#ifndef QT_NO_CLIPBOARD
    m_clipboard = new QXcbClipboard(this);
#endif
#if QT_CONFIG(draganddrop)
    m_drag = new QXcbDrag(this);
#endif

    m_startupId = qgetenv("DESKTOP_STARTUP_ID");
    if (!m_startupId.isNull())
        qunsetenv("DESKTOP_STARTUP_ID");


    QStringList glIntegrationNames;
    glIntegrationNames << QStringLiteral("xcb_glx") << QStringLiteral("xcb_egl");
    QString glIntegrationName = QString::fromLocal8Bit(qgetenv("QT_XCB_GL_INTEGRATION"));
    if (!glIntegrationName.isEmpty()) {
        qCDebug(lcQpaGl) << "QT_XCB_GL_INTEGRATION is set to" << glIntegrationName;
        if (glIntegrationName != QLatin1String("none")) {
            glIntegrationNames.removeAll(glIntegrationName);
            glIntegrationNames.prepend(glIntegrationName);
        } else {
            glIntegrationNames.clear();
        }
    }

    if (!glIntegrationNames.isEmpty()) {
        qCDebug(lcQpaGl) << "Choosing xcb gl-integration based on following priority\n" << glIntegrationNames;
        for (int i = 0; i < glIntegrationNames.size() && !m_glIntegration; i++) {
            m_glIntegration = QXcbGlIntegrationFactory::create(glIntegrationNames.at(i));
            if (m_glIntegration && !m_glIntegration->initialize(this)) {
                qCDebug(lcQpaGl) << "Failed to initialize xcb gl-integration" << glIntegrationNames.at(i);
                delete m_glIntegration;
                m_glIntegration = nullptr;
            }
        }
        if (!m_glIntegration)
            qCDebug(lcQpaGl) << "Failed to create xcb gl-integration";
    }

    sync();
}

QXcbConnection::~QXcbConnection()
{
#ifndef QT_NO_CLIPBOARD
    delete m_clipboard;
#endif
#if QT_CONFIG(draganddrop)
    delete m_drag;
#endif
    if (m_reader && m_reader->isRunning()) {
        sendConnectionEvent(QXcbAtom::_QT_CLOSE_CONNECTION);
        m_reader->wait();
    }

    delete m_reader;

    QXcbIntegration *integration = QXcbIntegration::instance();
    // Delete screens in reverse order to avoid crash in case of multiple screens
    while (!m_screens.isEmpty())
        integration->destroyScreen(m_screens.takeLast());

    while (!m_virtualDesktops.isEmpty())
        delete m_virtualDesktops.takeLast();

    delete m_glIntegration;

    if (isConnected()) {
#if QT_CONFIG(xcb_xlib)
        XCloseDisplay(static_cast<Display *>(m_xlib_display));
#else
        xcb_disconnect(xcb_connection());
#endif
    }

    delete m_keyboard;
}

bool QXcbConnection::isConnected() const
{
    return m_connection && !xcb_connection_has_error(m_connection);
}

QXcbScreen *QXcbConnection::primaryScreen() const
{
    if (!m_screens.isEmpty()) {
        Q_ASSERT(m_screens.first()->screenNumber() == primaryScreenNumber());
        return m_screens.first();
    }

    return nullptr;
}

void QXcbConnection::addWindowEventListener(xcb_window_t id, QXcbWindowEventListener *eventListener)
{
    m_mapper.insert(id, eventListener);
}

void QXcbConnection::removeWindowEventListener(xcb_window_t id)
{
    m_mapper.remove(id);
}

QXcbWindowEventListener *QXcbConnection::windowEventListenerFromId(xcb_window_t id)
{
    return m_mapper.value(id, 0);
}

QXcbWindow *QXcbConnection::platformWindowFromId(xcb_window_t id)
{
    QXcbWindowEventListener *listener = m_mapper.value(id, 0);
    if (listener)
        return listener->toWindow();
    return 0;
}

#define HANDLE_PLATFORM_WINDOW_EVENT(event_t, windowMember, handler) \
{ \
    event_t *e = reinterpret_cast<event_t *>(event); \
    if (QXcbWindowEventListener *eventListener = windowEventListenerFromId(e->windowMember))  { \
        handled = eventListener->handleGenericEvent(event, &result); \
        if (!handled) \
            eventListener->handler(e); \
    } \
} \
break;

#define HANDLE_KEYBOARD_EVENT(event_t, handler) \
{ \
    event_t *e = reinterpret_cast<event_t *>(event); \
    if (QXcbWindowEventListener *eventListener = windowEventListenerFromId(e->event)) { \
        handled = eventListener->handleGenericEvent(event, &result); \
        if (!handled) \
            m_keyboard->handler(e); \
    } \
} \
break;

void QXcbConnection::printXcbEvent(const QLoggingCategory &log, const char *message,
                                   xcb_generic_event_t *event) const
{
    quint8 response_type = event->response_type & ~0x80;
    quint16 sequence = event->sequence;

#define PRINT_AND_RETURN(name) { \
    qCDebug(log, "%s | %s(%d) | sequence: %d", message, name, response_type, sequence); \
    return; \
}
#define CASE_PRINT_AND_RETURN(name) case name : PRINT_AND_RETURN(#name);

    switch (response_type) {
    CASE_PRINT_AND_RETURN( XCB_KEY_PRESS );
    CASE_PRINT_AND_RETURN( XCB_KEY_RELEASE );
    CASE_PRINT_AND_RETURN( XCB_BUTTON_PRESS );
    CASE_PRINT_AND_RETURN( XCB_BUTTON_RELEASE );
    CASE_PRINT_AND_RETURN( XCB_MOTION_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_ENTER_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_LEAVE_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_FOCUS_IN );
    CASE_PRINT_AND_RETURN( XCB_FOCUS_OUT );
    CASE_PRINT_AND_RETURN( XCB_KEYMAP_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_EXPOSE );
    CASE_PRINT_AND_RETURN( XCB_GRAPHICS_EXPOSURE );
    CASE_PRINT_AND_RETURN( XCB_NO_EXPOSURE );
    CASE_PRINT_AND_RETURN( XCB_VISIBILITY_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_CREATE_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_DESTROY_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_UNMAP_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_MAP_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_MAP_REQUEST );
    CASE_PRINT_AND_RETURN( XCB_REPARENT_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_CONFIGURE_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_CONFIGURE_REQUEST );
    CASE_PRINT_AND_RETURN( XCB_GRAVITY_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_RESIZE_REQUEST );
    CASE_PRINT_AND_RETURN( XCB_CIRCULATE_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_CIRCULATE_REQUEST );
    CASE_PRINT_AND_RETURN( XCB_PROPERTY_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_SELECTION_CLEAR );
    CASE_PRINT_AND_RETURN( XCB_SELECTION_REQUEST );
    CASE_PRINT_AND_RETURN( XCB_SELECTION_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_COLORMAP_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_CLIENT_MESSAGE );
    CASE_PRINT_AND_RETURN( XCB_MAPPING_NOTIFY );
    CASE_PRINT_AND_RETURN( XCB_GE_GENERIC );
    }
    // XFixes
    if (has_xfixes && response_type == xfixes_first_event + XCB_XFIXES_SELECTION_NOTIFY)
          PRINT_AND_RETURN("XCB_XFIXES_SELECTION_NOTIFY");
    // XRandR
    if (has_randr_extension) {
        if (response_type == xrandr_first_event + XCB_RANDR_NOTIFY)
            PRINT_AND_RETURN("XCB_RANDR_NOTIFY");
        if (response_type == xrandr_first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY)
            PRINT_AND_RETURN("XCB_RANDR_SCREEN_CHANGE_NOTIFY");
    }
    // XKB
    if (response_type == xkb_first_event)
        PRINT_AND_RETURN("XCB_XKB_* event");

    // UNKNOWN
    qCDebug(log, "%s | unknown(%d) | sequence: %d", message, response_type, sequence);

#undef PRINT_AND_RETURN
#undef CASE_PRINT_AND_RETURN
}

const char *xcb_errors[] =
{
    "Success",
    "BadRequest",
    "BadValue",
    "BadWindow",
    "BadPixmap",
    "BadAtom",
    "BadCursor",
    "BadFont",
    "BadMatch",
    "BadDrawable",
    "BadAccess",
    "BadAlloc",
    "BadColor",
    "BadGC",
    "BadIDChoice",
    "BadName",
    "BadLength",
    "BadImplementation",
    "Unknown"
};

const char *xcb_protocol_request_codes[] =
{
    "Null",
    "CreateWindow",
    "ChangeWindowAttributes",
    "GetWindowAttributes",
    "DestroyWindow",
    "DestroySubwindows",
    "ChangeSaveSet",
    "ReparentWindow",
    "MapWindow",
    "MapSubwindows",
    "UnmapWindow",
    "UnmapSubwindows",
    "ConfigureWindow",
    "CirculateWindow",
    "GetGeometry",
    "QueryTree",
    "InternAtom",
    "GetAtomName",
    "ChangeProperty",
    "DeleteProperty",
    "GetProperty",
    "ListProperties",
    "SetSelectionOwner",
    "GetSelectionOwner",
    "ConvertSelection",
    "SendEvent",
    "GrabPointer",
    "UngrabPointer",
    "GrabButton",
    "UngrabButton",
    "ChangeActivePointerGrab",
    "GrabKeyboard",
    "UngrabKeyboard",
    "GrabKey",
    "UngrabKey",
    "AllowEvents",
    "GrabServer",
    "UngrabServer",
    "QueryPointer",
    "GetMotionEvents",
    "TranslateCoords",
    "WarpPointer",
    "SetInputFocus",
    "GetInputFocus",
    "QueryKeymap",
    "OpenFont",
    "CloseFont",
    "QueryFont",
    "QueryTextExtents",
    "ListFonts",
    "ListFontsWithInfo",
    "SetFontPath",
    "GetFontPath",
    "CreatePixmap",
    "FreePixmap",
    "CreateGC",
    "ChangeGC",
    "CopyGC",
    "SetDashes",
    "SetClipRectangles",
    "FreeGC",
    "ClearArea",
    "CopyArea",
    "CopyPlane",
    "PolyPoint",
    "PolyLine",
    "PolySegment",
    "PolyRectangle",
    "PolyArc",
    "FillPoly",
    "PolyFillRectangle",
    "PolyFillArc",
    "PutImage",
    "GetImage",
    "PolyText8",
    "PolyText16",
    "ImageText8",
    "ImageText16",
    "CreateColormap",
    "FreeColormap",
    "CopyColormapAndFree",
    "InstallColormap",
    "UninstallColormap",
    "ListInstalledColormaps",
    "AllocColor",
    "AllocNamedColor",
    "AllocColorCells",
    "AllocColorPlanes",
    "FreeColors",
    "StoreColors",
    "StoreNamedColor",
    "QueryColors",
    "LookupColor",
    "CreateCursor",
    "CreateGlyphCursor",
    "FreeCursor",
    "RecolorCursor",
    "QueryBestSize",
    "QueryExtension",
    "ListExtensions",
    "ChangeKeyboardMapping",
    "GetKeyboardMapping",
    "ChangeKeyboardControl",
    "GetKeyboardControl",
    "Bell",
    "ChangePointerControl",
    "GetPointerControl",
    "SetScreenSaver",
    "GetScreenSaver",
    "ChangeHosts",
    "ListHosts",
    "SetAccessControl",
    "SetCloseDownMode",
    "KillClient",
    "RotateProperties",
    "ForceScreenSaver",
    "SetPointerMapping",
    "GetPointerMapping",
    "SetModifierMapping",
    "GetModifierMapping",
    "Unknown"
};

void QXcbConnection::handleXcbError(xcb_generic_error_t *error)
{
    long result = 0;
    QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
    if (dispatcher && dispatcher->filterNativeEvent(m_nativeInterface->genericEventFilterType(), error, &result))
        return;

    printXcbError("QXcbConnection: XCB error", error);
}

void QXcbConnection::printXcbError(const char *message, xcb_generic_error_t *error)
{
    uint clamped_error_code = qMin<uint>(error->error_code, (sizeof(xcb_errors) / sizeof(xcb_errors[0])) - 1);
    uint clamped_major_code = qMin<uint>(error->major_code, (sizeof(xcb_protocol_request_codes) / sizeof(xcb_protocol_request_codes[0])) - 1);

    qCWarning(lcQpaXcb, "%s: %d (%s), sequence: %d, resource id: %d, major code: %d (%s), minor code: %d",
             message,
             int(error->error_code), xcb_errors[clamped_error_code],
             int(error->sequence), int(error->resource_id),
             int(error->major_code), xcb_protocol_request_codes[clamped_major_code],
             int(error->minor_code));
}

static Qt::MouseButtons translateMouseButtons(int s)
{
    Qt::MouseButtons ret = 0;
    if (s & XCB_BUTTON_MASK_1)
        ret |= Qt::LeftButton;
    if (s & XCB_BUTTON_MASK_2)
        ret |= Qt::MidButton;
    if (s & XCB_BUTTON_MASK_3)
        ret |= Qt::RightButton;
    return ret;
}

void QXcbConnection::setButtonState(Qt::MouseButton button, bool down)
{
    m_buttonState.setFlag(button, down);
    m_button = button;
}

Qt::MouseButton QXcbConnection::translateMouseButton(xcb_button_t s)
{
    switch (s) {
    case 1: return Qt::LeftButton;
    case 2: return Qt::MidButton;
    case 3: return Qt::RightButton;
    // Button values 4-7 were already handled as Wheel events, and won't occur here.
    case 8: return Qt::BackButton;      // Also known as Qt::ExtraButton1
    case 9: return Qt::ForwardButton;   // Also known as Qt::ExtraButton2
    case 10: return Qt::ExtraButton3;
    case 11: return Qt::ExtraButton4;
    case 12: return Qt::ExtraButton5;
    case 13: return Qt::ExtraButton6;
    case 14: return Qt::ExtraButton7;
    case 15: return Qt::ExtraButton8;
    case 16: return Qt::ExtraButton9;
    case 17: return Qt::ExtraButton10;
    case 18: return Qt::ExtraButton11;
    case 19: return Qt::ExtraButton12;
    case 20: return Qt::ExtraButton13;
    case 21: return Qt::ExtraButton14;
    case 22: return Qt::ExtraButton15;
    case 23: return Qt::ExtraButton16;
    case 24: return Qt::ExtraButton17;
    case 25: return Qt::ExtraButton18;
    case 26: return Qt::ExtraButton19;
    case 27: return Qt::ExtraButton20;
    case 28: return Qt::ExtraButton21;
    case 29: return Qt::ExtraButton22;
    case 30: return Qt::ExtraButton23;
    case 31: return Qt::ExtraButton24;
    default: return Qt::NoButton;
    }
}

#if QT_CONFIG(xkb)
namespace {
    typedef union {
        /* All XKB events share these fields. */
        struct {
            uint8_t response_type;
            uint8_t xkbType;
            uint16_t sequence;
            xcb_timestamp_t time;
            uint8_t deviceID;
        } any;
        xcb_xkb_new_keyboard_notify_event_t new_keyboard_notify;
        xcb_xkb_map_notify_event_t map_notify;
        xcb_xkb_state_notify_event_t state_notify;
    } _xkb_event;
}
#endif

void QXcbConnection::handleXcbEvent(xcb_generic_event_t *event)
{
    long result = 0;
    QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
    bool handled = dispatcher && dispatcher->filterNativeEvent(m_nativeInterface->genericEventFilterType(), event, &result);

    uint response_type = event->response_type & ~0x80;

    if (!handled) {
        switch (response_type) {
        case XCB_EXPOSE:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_expose_event_t, window, handleExposeEvent);

        case XCB_BUTTON_PRESS: {
            xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
            m_keyboard->updateXKBStateFromCore(ev->state);
            // the event explicitly contains the state of the three first buttons,
            // the rest we need to manage ourselves
            m_buttonState = (m_buttonState & ~0x7) | translateMouseButtons(ev->state);
            setButtonState(translateMouseButton(ev->detail), true);
            if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
                qCDebug(lcQpaXInputEvents, "legacy mouse press, button %d state %X", ev->detail, static_cast<unsigned int>(m_buttonState));
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_button_press_event_t, event, handleButtonPressEvent);
        }
        case XCB_BUTTON_RELEASE: {
            xcb_button_release_event_t *ev = (xcb_button_release_event_t *)event;
            m_keyboard->updateXKBStateFromCore(ev->state);
            m_buttonState = (m_buttonState & ~0x7) | translateMouseButtons(ev->state);
            setButtonState(translateMouseButton(ev->detail), false);
            if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
                qCDebug(lcQpaXInputEvents, "legacy mouse release, button %d state %X", ev->detail, static_cast<unsigned int>(m_buttonState));
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_button_release_event_t, event, handleButtonReleaseEvent);
        }
        case XCB_MOTION_NOTIFY: {
            xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)event;
            m_keyboard->updateXKBStateFromCore(ev->state);
            m_buttonState = (m_buttonState & ~0x7) | translateMouseButtons(ev->state);
            if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
                qCDebug(lcQpaXInputEvents, "legacy mouse move %d,%d button %d state %X", ev->event_x, ev->event_y,
                        ev->detail, static_cast<unsigned int>(m_buttonState));
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_motion_notify_event_t, event, handleMotionNotifyEvent);
        }

        case XCB_CONFIGURE_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_configure_notify_event_t, event, handleConfigureNotifyEvent);
        case XCB_MAP_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_map_notify_event_t, event, handleMapNotifyEvent);
        case XCB_UNMAP_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_unmap_notify_event_t, event, handleUnmapNotifyEvent);
        case XCB_DESTROY_NOTIFY:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_destroy_notify_event_t, event, handleDestroyNotifyEvent);
        case XCB_CLIENT_MESSAGE:
            handleClientMessageEvent((xcb_client_message_event_t *)event);
            break;
        case XCB_ENTER_NOTIFY:
#if QT_CONFIG(xinput2)
            if (hasXInput2() && !xi2MouseEventsDisabled())
                break;
#endif
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_enter_notify_event_t, event, handleEnterNotifyEvent);
        case XCB_LEAVE_NOTIFY:
#if QT_CONFIG(xinput2)
            if (hasXInput2() && !xi2MouseEventsDisabled())
                break;
#endif
            m_keyboard->updateXKBStateFromCore(((xcb_leave_notify_event_t *)event)->state);
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_leave_notify_event_t, event, handleLeaveNotifyEvent);
        case XCB_FOCUS_IN:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_focus_in_event_t, event, handleFocusInEvent);
        case XCB_FOCUS_OUT:
            HANDLE_PLATFORM_WINDOW_EVENT(xcb_focus_out_event_t, event, handleFocusOutEvent);
        case XCB_KEY_PRESS:
        {
            xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;
            m_keyboard->updateXKBStateFromCore(kp->state);
            setTime(kp->time);
            HANDLE_KEYBOARD_EVENT(xcb_key_press_event_t, handleKeyPressEvent);
        }
        case XCB_KEY_RELEASE:
            m_keyboard->updateXKBStateFromCore(((xcb_key_release_event_t *)event)->state);
            HANDLE_KEYBOARD_EVENT(xcb_key_release_event_t, handleKeyReleaseEvent);
        case XCB_MAPPING_NOTIFY:
            m_keyboard->updateKeymap(reinterpret_cast<xcb_mapping_notify_event_t *>(event));
            break;
        case XCB_SELECTION_REQUEST:
        {
#if QT_CONFIG(draganddrop) || QT_CONFIG(clipboard)
            xcb_selection_request_event_t *sr = reinterpret_cast<xcb_selection_request_event_t *>(event);
#endif
#if QT_CONFIG(draganddrop)
            if (sr->selection == atom(QXcbAtom::XdndSelection))
                m_drag->handleSelectionRequest(sr);
            else
#endif
            {
#ifndef QT_NO_CLIPBOARD
                m_clipboard->handleSelectionRequest(sr);
#endif
            }
            break;
        }
        case XCB_SELECTION_CLEAR:
            setTime((reinterpret_cast<xcb_selection_clear_event_t *>(event))->time);
#ifndef QT_NO_CLIPBOARD
            m_clipboard->handleSelectionClearRequest(reinterpret_cast<xcb_selection_clear_event_t *>(event));
#endif
            handled = true;
            break;
        case XCB_SELECTION_NOTIFY:
            setTime((reinterpret_cast<xcb_selection_notify_event_t *>(event))->time);
            handled = false;
            break;
        case XCB_PROPERTY_NOTIFY:
        {
            xcb_property_notify_event_t *pn = reinterpret_cast<xcb_property_notify_event_t *>(event);
            if (pn->atom == atom(QXcbAtom::_NET_WORKAREA)) {
                QXcbVirtualDesktop *virtualDesktop = virtualDesktopForRootWindow(pn->window);
                if (virtualDesktop)
                    virtualDesktop->updateWorkArea();
            } else {
                HANDLE_PLATFORM_WINDOW_EVENT(xcb_property_notify_event_t, window, handlePropertyNotifyEvent);
            }
            break;
        }
#if QT_CONFIG(xinput2)
        case XCB_GE_GENERIC:
            // Here the windowEventListener is invoked from xi2HandleEvent()
            if (hasXInput2() && isXIEvent(event, m_xiOpCode))
                xi2HandleEvent(reinterpret_cast<xcb_ge_event_t *>(event));
            break;
#endif
        default:
            handled = false;
            break;
        }
    }

    if (!handled) {
        if (has_xfixes && response_type == xfixes_first_event + XCB_XFIXES_SELECTION_NOTIFY) {
            xcb_xfixes_selection_notify_event_t *notify_event = reinterpret_cast<xcb_xfixes_selection_notify_event_t *>(event);
            setTime(notify_event->timestamp);
#ifndef QT_NO_CLIPBOARD
            m_clipboard->handleXFixesSelectionRequest(notify_event);
#endif
            for (QXcbVirtualDesktop *virtualDesktop : qAsConst(m_virtualDesktops))
                virtualDesktop->handleXFixesSelectionNotify(notify_event);

            handled = true;
        } else if (has_randr_extension && response_type == xrandr_first_event + XCB_RANDR_NOTIFY) {
            updateScreens(reinterpret_cast<xcb_randr_notify_event_t *>(event));
            handled = true;
        } else if (has_randr_extension && response_type == xrandr_first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
            xcb_randr_screen_change_notify_event_t *change_event = reinterpret_cast<xcb_randr_screen_change_notify_event_t *>(event);
            if (auto *virtualDesktop = virtualDesktopForRootWindow(change_event->root))
                virtualDesktop->handleScreenChange(change_event);

            handled = true;
#if QT_CONFIG(xkb)
        } else if (response_type == xkb_first_event) { // https://bugs.freedesktop.org/show_bug.cgi?id=51295
            _xkb_event *xkb_event = reinterpret_cast<_xkb_event *>(event);
            if (xkb_event->any.deviceID == m_keyboard->coreDeviceId()) {
                switch (xkb_event->any.xkbType) {
                    // XkbNewKkdNotify and XkbMapNotify together capture all sorts of keymap
                    // updates (e.g. xmodmap, xkbcomp, setxkbmap), with minimal redundent recompilations.
                    case XCB_XKB_STATE_NOTIFY:
                        m_keyboard->updateXKBState(&xkb_event->state_notify);
                        handled = true;
                        break;
                    case XCB_XKB_MAP_NOTIFY:
                        m_keyboard->updateKeymap();
                        handled = true;
                        break;
                    case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
                        xcb_xkb_new_keyboard_notify_event_t *ev = &xkb_event->new_keyboard_notify;
                        if (ev->changed & XCB_XKB_NKN_DETAIL_KEYCODES)
                            m_keyboard->updateKeymap();
                        break;
                    }
                    default:
                        break;
                }
            }
#endif
        }
    }

    if (!handled && m_glIntegration)
        handled = m_glIntegration->handleXcbEvent(event, response_type);

#if 0
    if (Q_UNLIKELY(lcQpaEvents().isDebugEnabled()))
        printXcbEvent(lcQpaEvents(), handled ? "Handled" : "Unhandled", event);
#endif
}

void QXcbConnection::addPeekFunc(PeekFunc f)
{
    m_peekFuncs.append(f);
}

qint32 QXcbConnection::generatePeekerId()
{
    qint32 peekerId = m_peekerIdSource++;
    m_peekerToCachedIndex.insert(peekerId, 0);
    return peekerId;
}

bool QXcbConnection::removePeekerId(qint32 peekerId)
{
    if (!m_peekerToCachedIndex.contains(peekerId)) {
        qCWarning(lcQpaXcb, "failed to remove unknown peeker id: %d", peekerId);
        return false;
    }
    m_peekerToCachedIndex.remove(peekerId);
    if (m_peekerToCachedIndex.isEmpty()) {
        m_peekerIdSource = 0; // Once the hash becomes empty, we can start reusing IDs
        m_peekerIndexCacheDirty = false;
    }
    return true;
}

bool QXcbConnection::peekEventQueue(PeekerCallback peeker, void *peekerData,
                                    PeekOptions option, qint32 peekerId)
{
    bool peekerIdProvided = peekerId != -1;
    if (peekerIdProvided && !m_peekerToCachedIndex.contains(peekerId)) {
        qCWarning(lcQpaXcb, "failed to find index for unknown peeker id: %d", peekerId);
        return false;
    }

    bool peekFromCachedIndex = option.testFlag(PeekOption::PeekFromCachedIndex);
    if (peekFromCachedIndex && !peekerIdProvided) {
        qCWarning(lcQpaXcb, "PeekOption::PeekFromCachedIndex requires peeker id");
        return false;
    }

    if (peekerIdProvided && m_peekerIndexCacheDirty) {
        // When the main event loop has flushed the buffered XCB events into the window
        // system event queue, the cached indices are not valid anymore and need reset.
        auto it = m_peekerToCachedIndex.begin();
        while (it != m_peekerToCachedIndex.constEnd()) {
            (*it) = 0;
            ++it;
        }
        m_peekerIndexCacheDirty = false;
    }

    qint32 peekerIndex = peekFromCachedIndex ? m_peekerToCachedIndex.value(peekerId) : 0;
    qint32 startingIndex = peekerIndex;
    bool result = false;
    m_mainEventLoopFlushedQueue = false;

    QXcbEventArray *eventqueue = m_reader->lock();

    if (Q_UNLIKELY(lcQpaPeeker().isDebugEnabled())) {
        qCDebug(lcQpaPeeker, "[%d] peeker index: %d | mode: %s | queue size: %d", peekerId,
                peekerIndex, peekFromCachedIndex ? "cache" : "start", eventqueue->size());
    }
    while (peekerIndex < eventqueue->size() && !result && !m_mainEventLoopFlushedQueue) {
        xcb_generic_event_t *event = eventqueue->at(peekerIndex++);
        if (!event)
            continue;
        if (Q_UNLIKELY(lcQpaPeeker().isDebugEnabled())) {
            QString debug = QString((QLatin1String("[%1] peeking at index: %2")))
                            .arg(peekerId).arg(peekerIndex - 1);
            printXcbEvent(lcQpaPeeker(), debug.toLatin1(), event);
        }
        // A peeker may call QCoreApplication::processEvents(), which has two implications:
        // 1) We need to make the lock available for QXcbConnection::processXcbEvents(),
        //    otherwise we will deadlock;
        // 2) QXcbConnection::processXcbEvents() will flush the queue we are currently
        //    looping through;
        m_reader->unlock();
        result = peeker(event, peekerData);
        m_reader->lock();
    }

    m_reader->unlock();

    if (peekerIdProvided && peekerIndex != startingIndex && !m_mainEventLoopFlushedQueue) {
        auto it = m_peekerToCachedIndex.find(peekerId);
        // Make sure that a peeker callback did not remove the peeker id
        if (it != m_peekerToCachedIndex.constEnd())
            (*it) = peekerIndex;
    }

    return result;
}

QXcbEventReader::QXcbEventReader(QXcbConnection *connection)
    : m_connection(connection)
{
    checkXcbPollForQueuedEvent();
}

void QXcbEventReader::start()
{
    if (local_xcb_poll_for_queued_event) {
        connect(this, SIGNAL(eventPending()), m_connection, SLOT(processXcbEvents()), Qt::QueuedConnection);
        connect(this, SIGNAL(finished()), m_connection, SLOT(processXcbEvents()));
        QThread::start();
    } else {
        // Must be done after we have an event-dispatcher. By posting a method invocation
        // we are sure that by the time the method is called we have an event-dispatcher.
        QMetaObject::invokeMethod(this, "registerForEvents", Qt::QueuedConnection);
    }
}

void QXcbEventReader::registerForEvents()
{
    QSocketNotifier *notifier = new QSocketNotifier(xcb_get_file_descriptor(m_connection->xcb_connection()), QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), m_connection, SLOT(processXcbEvents()));

    QAbstractEventDispatcher *dispatcher = QGuiApplicationPrivate::eventDispatcher;
    connect(dispatcher, SIGNAL(aboutToBlock()), m_connection, SLOT(processXcbEvents()));
    connect(dispatcher, SIGNAL(awake()), m_connection, SLOT(processXcbEvents()));
}

void QXcbEventReader::registerEventDispatcher(QAbstractEventDispatcher *dispatcher)
{
    // flush the xcb connection before the EventDispatcher is going to block
    // In the non-threaded case processXcbEvents is called before going to block,
    // which flushes the connection.
    if (local_xcb_poll_for_queued_event)
        connect(dispatcher, SIGNAL(aboutToBlock()), m_connection, SLOT(flush()));
}

void QXcbEventReader::run()
{
    xcb_generic_event_t *event;
    while (m_connection && (event = xcb_wait_for_event(m_connection->xcb_connection()))) {
        m_mutex.lock();
        addEvent(event);
        while (m_connection && (event = local_xcb_poll_for_queued_event(m_connection->xcb_connection())))
            addEvent(event);
        m_mutex.unlock();
        emit eventPending();
    }

    m_mutex.lock();
    for (int i = 0; i < m_events.size(); ++i)
        free(m_events.at(i));
    m_events.clear();
    m_mutex.unlock();
}

void QXcbEventReader::addEvent(xcb_generic_event_t *event)
{
    if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE
        && (reinterpret_cast<xcb_client_message_event_t *>(event))->type == m_connection->atom(QXcbAtom::_QT_CLOSE_CONNECTION))
        m_connection = 0;
    m_events << event;
}

QXcbEventArray *QXcbEventReader::lock()
{
    m_mutex.lock();
    if (!local_xcb_poll_for_queued_event) {
        while (xcb_generic_event_t *event = xcb_poll_for_event(m_connection->xcb_connection()))
            m_events << event;
    }
    return &m_events;
}

void QXcbEventReader::unlock()
{
    m_mutex.unlock();
}

void QXcbConnection::setFocusWindow(QWindow *w)
{
    m_focusWindow = w ? static_cast<QXcbWindow *>(w->handle()) : nullptr;
}
void QXcbConnection::setMouseGrabber(QXcbWindow *w)
{
    m_mouseGrabber = w;
    m_mousePressWindow = nullptr;
}
void QXcbConnection::setMousePressWindow(QXcbWindow *w)
{
    m_mousePressWindow = w;
}

void QXcbConnection::grabServer()
{
    if (m_canGrabServer)
        xcb_grab_server(m_connection);
}

void QXcbConnection::ungrabServer()
{
    if (m_canGrabServer)
        xcb_ungrab_server(m_connection);
}

void QXcbConnection::sendConnectionEvent(QXcbAtom::Atom a, uint id)
{
    xcb_client_message_event_t event;
    memset(&event, 0, sizeof(event));

    const xcb_window_t eventListener = xcb_generate_id(m_connection);
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(m_setup);
    xcb_screen_t *screen = it.data;
    xcb_create_window(m_connection, XCB_COPY_FROM_PARENT,
                      eventListener, screen->root,
                      0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
                      screen->root_visual, 0, 0);

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.sequence = 0;
    event.window = eventListener;
    event.type = atom(a);
    event.data.data32[0] = id;

    xcb_send_event(xcb_connection(), false, eventListener, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<const char *>(&event));
    xcb_destroy_window(m_connection, eventListener);
    xcb_flush(xcb_connection());
}

namespace
{
    class PropertyNotifyEvent {
    public:
        PropertyNotifyEvent(xcb_window_t win, xcb_atom_t property)
            : window(win), type(XCB_PROPERTY_NOTIFY), atom(property) {}
        xcb_window_t window;
        int type;
        xcb_atom_t atom;
        bool checkEvent(xcb_generic_event_t *event) const {
            if (!event)
                return false;
            if ((event->response_type & ~0x80) != type) {
                return false;
            } else {
                xcb_property_notify_event_t *pn = reinterpret_cast<xcb_property_notify_event_t *>(event);
                if ((pn->window == window) && (pn->atom == atom))
                    return true;
            }
            return false;
        }
    };
}

xcb_timestamp_t QXcbConnection::getTimestamp()
{
    // send a dummy event to myself to get the timestamp from X server.
    xcb_window_t root_win = rootWindow();
    xcb_change_property(xcb_connection(), XCB_PROP_MODE_APPEND, root_win, atom(QXcbAtom::CLIP_TEMPORARY),
                        XCB_ATOM_INTEGER, 32, 0, NULL);

    connection()->flush();
    PropertyNotifyEvent checker(root_win, atom(QXcbAtom::CLIP_TEMPORARY));

    xcb_generic_event_t *event = 0;
    // lets keep this inside a loop to avoid a possible race condition, where
    // reader thread has not yet had the time to acquire the mutex in order
    // to add the new set of events to its event queue
    while (!event) {
        connection()->sync();
        event = checkEvent(checker);
    }

    xcb_property_notify_event_t *pn = reinterpret_cast<xcb_property_notify_event_t *>(event);
    xcb_timestamp_t timestamp = pn->time;
    free(event);

    xcb_delete_property(xcb_connection(), root_win, atom(QXcbAtom::CLIP_TEMPORARY));

    return timestamp;
}

xcb_window_t QXcbConnection::getSelectionOwner(xcb_atom_t atom) const
{
    return Q_XCB_REPLY(xcb_get_selection_owner, xcb_connection(), atom)->owner;
}

xcb_window_t QXcbConnection::getQtSelectionOwner()
{
    if (!m_qtSelectionOwner) {
        xcb_screen_t *xcbScreen = primaryVirtualDesktop()->screen();
        int16_t x = 0, y = 0;
        uint16_t w = 3, h = 3;
        m_qtSelectionOwner = xcb_generate_id(xcb_connection());
        xcb_create_window(xcb_connection(),
                          XCB_COPY_FROM_PARENT,               // depth -- same as root
                          m_qtSelectionOwner,                 // window id
                          xcbScreen->root,                    // parent window id
                          x, y, w, h,
                          0,                                  // border width
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,      // window class
                          xcbScreen->root_visual,             // visual
                          0,                                  // value mask
                          0);                                 // value list
    }
    return m_qtSelectionOwner;
}

xcb_window_t QXcbConnection::rootWindow()
{
    QXcbScreen *s = primaryScreen();
    return s ? s->root() : 0;
}

xcb_window_t QXcbConnection::clientLeader()
{
    if (m_clientLeader == 0) {
        m_clientLeader = xcb_generate_id(xcb_connection());
        QXcbScreen *screen = primaryScreen();
        xcb_create_window(xcb_connection(),
                          XCB_COPY_FROM_PARENT,
                          m_clientLeader,
                          screen->root(),
                          0, 0, 1, 1,
                          0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          screen->screen()->root_visual,
                          0, 0);
#ifndef QT_NO_DEBUG
        QByteArray ba("Qt client leader window");
        xcb_change_property(xcb_connection(),
                            XCB_PROP_MODE_REPLACE,
                            m_clientLeader,
                            atom(QXcbAtom::_NET_WM_NAME),
                            atom(QXcbAtom::UTF8_STRING),
                            8,
                            ba.length(),
                            ba.constData());
#endif
        xcb_change_property(xcb_connection(),
                            XCB_PROP_MODE_REPLACE,
                            m_clientLeader,
                            atom(QXcbAtom::WM_CLIENT_LEADER),
                            XCB_ATOM_WINDOW,
                            32,
                            1,
                            &m_clientLeader);

#if QT_CONFIG(xcb_sm)
        // If we are session managed, inform the window manager about it
        QByteArray session = qGuiApp->sessionId().toLatin1();
        if (!session.isEmpty()) {
            xcb_change_property(xcb_connection(),
                                XCB_PROP_MODE_REPLACE,
                                m_clientLeader,
                                atom(QXcbAtom::SM_CLIENT_ID),
                                XCB_ATOM_STRING,
                                8,
                                session.length(),
                                session.constData());
        }
#endif
    }
    return m_clientLeader;
}

#if QT_CONFIG(xcb_xlib)
void *QXcbConnection::xlib_display() const
{
    return m_xlib_display;
}

void *QXcbConnection::createVisualInfoForDefaultVisualId() const
{
    if (m_defaultVisualId == UINT_MAX)
        return 0;
    XVisualInfo info;
    memset(&info, 0, sizeof info);
    info.visualid = m_defaultVisualId;

    int count = 0;
    Display *dpy = static_cast<Display *>(connection()->xlib_display());
    XVisualInfo *retVisual = XGetVisualInfo(dpy, VisualIDMask, &info, &count);
    Q_ASSERT(count < 2);
    return retVisual;
}

#endif

#if QT_CONFIG(xinput2)
// it is safe to cast XI_* events here as long as we are only touching the first 32 bytes,
// after that position event needs memmove, see xi2PrepareXIGenericDeviceEvent
static inline bool isXIType(xcb_generic_event_t *event, int opCode, uint16_t type)
{
    if (!isXIEvent(event, opCode))
        return false;

    xXIGenericDeviceEvent *xiEvent = reinterpret_cast<xXIGenericDeviceEvent *>(event);
    return xiEvent->evtype == type;
}
#endif
static inline bool isValid(xcb_generic_event_t *event)
{
    return event && (event->response_type & ~0x80);
}

/*! \internal

    Compresses events of the same type to avoid swamping the event queue.
    If event compression is not desired there are several options what developers can do:

    1) Write responsive applications. We drop events that have been buffered in the event
       queue while waiting on unresponsive GUI thread.
    2) Use QAbstractNativeEventFilter to get all events from X connection. This is not optimal
       because it requires working with native event types.
    3) Or add public API to Qt for disabling event compression QTBUG-44964

*/
bool QXcbConnection::compressEvent(xcb_generic_event_t *event, int currentIndex, QXcbEventArray *eventqueue) const
{
    uint responseType = event->response_type & ~0x80;
    int nextIndex = currentIndex + 1;

    if (responseType == XCB_MOTION_NOTIFY) {
        // compress XCB_MOTION_NOTIFY notify events
        for (int j = nextIndex; j < eventqueue->size(); ++j) {
            xcb_generic_event_t *next = eventqueue->at(j);
            if (!isValid(next))
                continue;
            if (next->response_type == XCB_MOTION_NOTIFY)
                return true;
        }
        return false;
    }
#if QT_CONFIG(xinput2)
    // compress XI_* events
    if (responseType == XCB_GE_GENERIC) {
        if (!hasXInput2())
            return false;

        // compress XI_Motion
        if (isXIType(event, m_xiOpCode, XI_Motion)) {
#if QT_CONFIG(tabletevent)
            xXIDeviceEvent *xdev = reinterpret_cast<xXIDeviceEvent *>(event);
            // Xlib's XI2 events need memmove, see xi2PrepareXIGenericDeviceEvent()
            auto sourceId = *reinterpret_cast<uint16_t *>(reinterpret_cast<char *>(&xdev->sourceid) + 4);
            if (!QCoreApplication::testAttribute(Qt::AA_CompressTabletEvents) &&
                    const_cast<QXcbConnection *>(this)->tabletDataForDevice(sourceId))
                return false;
#endif // QT_CONFIG(tabletevent)
            for (int j = nextIndex; j < eventqueue->size(); ++j) {
                xcb_generic_event_t *next = eventqueue->at(j);
                if (!isValid(next))
                    continue;
                if (isXIType(next, m_xiOpCode, XI_Motion))
                    return true;
            }
            return false;
        }
#ifdef XCB_USE_XINPUT22
        // compress XI_TouchUpdate for the same touch point id
        if (isXIType(event, m_xiOpCode, XI_TouchUpdate)) {
            xXIDeviceEvent *xiDeviceEvent = reinterpret_cast<xXIDeviceEvent *>(event);
            uint32_t id = xiDeviceEvent->detail % INT_MAX;
            for (int j = nextIndex; j < eventqueue->size(); ++j) {
                xcb_generic_event_t *next = eventqueue->at(j);
                if (!isValid(next))
                    continue;
                if (isXIType(next, m_xiOpCode, XI_TouchUpdate)) {
                    xXIDeviceEvent *xiDeviceNextEvent = reinterpret_cast<xXIDeviceEvent *>(next);
                    if (id == xiDeviceNextEvent->detail % INT_MAX)
                        return true;
                }
            }
            return false;
        }
#endif
        return false;
    }
#endif
    if (responseType == XCB_CONFIGURE_NOTIFY) {
        // compress multiple configure notify events for the same window
        for (int j = nextIndex; j < eventqueue->size(); ++j) {
            xcb_generic_event_t *next = eventqueue->at(j);
            if (isValid(next) && next->response_type == XCB_CONFIGURE_NOTIFY
                && reinterpret_cast<xcb_configure_notify_event_t *>(next)->event == reinterpret_cast<xcb_configure_notify_event_t *>(event)->event)
            {
                return true;
            }
        }
        return false;
    }

    return false;
}

void QXcbConnection::processXcbEvents()
{
    int connection_error = xcb_connection_has_error(xcb_connection());
    if (connection_error) {
        qWarning("The X11 connection broke (error %d). Did the X11 server die?", connection_error);
        exit(1);
    }

    QXcbEventArray *eventqueue = m_reader->lock();

    for (int i = 0; i < eventqueue->size(); ++i) {
        xcb_generic_event_t *event = eventqueue->at(i);
        if (!event)
            continue;
        QScopedPointer<xcb_generic_event_t, QScopedPointerPodDeleter> eventGuard(event);
        (*eventqueue)[i] = 0;

        if (!(event->response_type & ~0x80)) {
            handleXcbError(reinterpret_cast<xcb_generic_error_t *>(event));
            continue;
        }

        if (Q_LIKELY(QCoreApplication::testAttribute(Qt::AA_CompressHighFrequencyEvents)) &&
                compressEvent(event, i, eventqueue))
            continue;

#ifndef QT_NO_CLIPBOARD
        bool accepted = false;
        if (clipboard()->processIncr())
            clipboard()->incrTransactionPeeker(event, accepted);
        if (accepted)
            continue;
#endif

        auto isWaitingFor = [=](PeekFunc peekFunc) {
            // These callbacks return true if the event is what they were
            // waiting for, remove them from the list in that case.
            return peekFunc(this, event);
        };
        m_peekFuncs.erase(std::remove_if(m_peekFuncs.begin(), m_peekFuncs.end(),
                                         isWaitingFor),
                          m_peekFuncs.end());
        m_reader->unlock();
        handleXcbEvent(event);
        m_reader->lock();
    }

    eventqueue->clear();

    m_reader->unlock();

    m_peekerIndexCacheDirty = m_mainEventLoopFlushedQueue = true;

    // Indicate with a null event that the event the callbacks are waiting for
    // is not in the queue currently.
    for (PeekFunc f : qAsConst(m_peekFuncs))
        f(this, 0);
    m_peekFuncs.clear();

    xcb_flush(xcb_connection());
}

void QXcbConnection::handleClientMessageEvent(const xcb_client_message_event_t *event)
{
    if (event->format != 32)
        return;

#if QT_CONFIG(draganddrop)
    if (event->type == atom(QXcbAtom::XdndStatus)) {
        drag()->handleStatus(event);
    } else if (event->type == atom(QXcbAtom::XdndFinished)) {
        drag()->handleFinished(event);
    }
#endif
    if (m_systemTrayTracker && event->type == atom(QXcbAtom::MANAGER))
        m_systemTrayTracker->notifyManagerClientMessageEvent(event);

    QXcbWindow *window = platformWindowFromId(event->window);
    if (!window)
        return;

    window->handleClientMessageEvent(event);
}

static const char * xcb_atomnames = {
    // window-manager <-> client protocols
    "WM_PROTOCOLS\0"
    "WM_DELETE_WINDOW\0"
    "WM_TAKE_FOCUS\0"
    "_NET_WM_PING\0"
    "_NET_WM_CONTEXT_HELP\0"
    "_NET_WM_SYNC_REQUEST\0"
    "_NET_WM_SYNC_REQUEST_COUNTER\0"
    "MANAGER\0"
    "_NET_SYSTEM_TRAY_OPCODE\0"

    // ICCCM window state
    "WM_STATE\0"
    "WM_CHANGE_STATE\0"
    "WM_CLASS\0"
    "WM_NAME\0"

    // Session management
    "WM_CLIENT_LEADER\0"
    "WM_WINDOW_ROLE\0"
    "SM_CLIENT_ID\0"

    // Clipboard
    "CLIPBOARD\0"
    "INCR\0"
    "TARGETS\0"
    "MULTIPLE\0"
    "TIMESTAMP\0"
    "SAVE_TARGETS\0"
    "CLIP_TEMPORARY\0"
    "_QT_SELECTION\0"
    "_QT_CLIPBOARD_SENTINEL\0"
    "_QT_SELECTION_SENTINEL\0"
    "CLIPBOARD_MANAGER\0"

    "RESOURCE_MANAGER\0"

    "_XSETROOT_ID\0"

    "_QT_SCROLL_DONE\0"
    "_QT_INPUT_ENCODING\0"

    "_QT_CLOSE_CONNECTION\0"

    "_MOTIF_WM_HINTS\0"

    "DTWM_IS_RUNNING\0"
    "ENLIGHTENMENT_DESKTOP\0"
    "_DT_SAVE_MODE\0"
    "_SGI_DESKS_MANAGER\0"

    // EWMH (aka NETWM)
    "_NET_SUPPORTED\0"
    "_NET_VIRTUAL_ROOTS\0"
    "_NET_WORKAREA\0"

    "_NET_MOVERESIZE_WINDOW\0"
    "_NET_WM_MOVERESIZE\0"

    "_NET_WM_NAME\0"
    "_NET_WM_ICON_NAME\0"
    "_NET_WM_ICON\0"

    "_NET_WM_PID\0"

    "_NET_WM_WINDOW_OPACITY\0"

    "_NET_WM_STATE\0"
    "_NET_WM_STATE_ABOVE\0"
    "_NET_WM_STATE_BELOW\0"
    "_NET_WM_STATE_FULLSCREEN\0"
    "_NET_WM_STATE_MAXIMIZED_HORZ\0"
    "_NET_WM_STATE_MAXIMIZED_VERT\0"
    "_NET_WM_STATE_MODAL\0"
    "_NET_WM_STATE_STAYS_ON_TOP\0"
    "_NET_WM_STATE_DEMANDS_ATTENTION\0"

    "_NET_WM_USER_TIME\0"
    "_NET_WM_USER_TIME_WINDOW\0"
    "_NET_WM_FULL_PLACEMENT\0"

    "_NET_WM_WINDOW_TYPE\0"
    "_NET_WM_WINDOW_TYPE_DESKTOP\0"
    "_NET_WM_WINDOW_TYPE_DOCK\0"
    "_NET_WM_WINDOW_TYPE_TOOLBAR\0"
    "_NET_WM_WINDOW_TYPE_MENU\0"
    "_NET_WM_WINDOW_TYPE_UTILITY\0"
    "_NET_WM_WINDOW_TYPE_SPLASH\0"
    "_NET_WM_WINDOW_TYPE_DIALOG\0"
    "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU\0"
    "_NET_WM_WINDOW_TYPE_POPUP_MENU\0"
    "_NET_WM_WINDOW_TYPE_TOOLTIP\0"
    "_NET_WM_WINDOW_TYPE_NOTIFICATION\0"
    "_NET_WM_WINDOW_TYPE_COMBO\0"
    "_NET_WM_WINDOW_TYPE_DND\0"
    "_NET_WM_WINDOW_TYPE_NORMAL\0"
    "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE\0"

    "_KDE_NET_WM_FRAME_STRUT\0"
    "_NET_FRAME_EXTENTS\0"

    "_NET_STARTUP_INFO\0"
    "_NET_STARTUP_INFO_BEGIN\0"

    "_NET_SUPPORTING_WM_CHECK\0"

    "_NET_WM_CM_S0\0"

    "_NET_SYSTEM_TRAY_VISUAL\0"

    "_NET_ACTIVE_WINDOW\0"

    // Property formats
    "TEXT\0"
    "UTF8_STRING\0"
    "CARDINAL\0"

    // xdnd
    "XdndEnter\0"
    "XdndPosition\0"
    "XdndStatus\0"
    "XdndLeave\0"
    "XdndDrop\0"
    "XdndFinished\0"
    "XdndTypeList\0"
    "XdndActionList\0"

    "XdndSelection\0"

    "XdndAware\0"
    "XdndProxy\0"

    "XdndActionCopy\0"
    "XdndActionLink\0"
    "XdndActionMove\0"
    "XdndActionPrivate\0"

    // Motif DND
    "_MOTIF_DRAG_AND_DROP_MESSAGE\0"
    "_MOTIF_DRAG_INITIATOR_INFO\0"
    "_MOTIF_DRAG_RECEIVER_INFO\0"
    "_MOTIF_DRAG_WINDOW\0"
    "_MOTIF_DRAG_TARGETS\0"

    "XmTRANSFER_SUCCESS\0"
    "XmTRANSFER_FAILURE\0"

    // Xkb
    "_XKB_RULES_NAMES\0"

    // XEMBED
    "_XEMBED\0"
    "_XEMBED_INFO\0"

    // XInput2
    "Button Left\0"
    "Button Middle\0"
    "Button Right\0"
    "Button Wheel Up\0"
    "Button Wheel Down\0"
    "Button Horiz Wheel Left\0"
    "Button Horiz Wheel Right\0"
    "Abs MT Position X\0"
    "Abs MT Position Y\0"
    "Abs MT Touch Major\0"
    "Abs MT Touch Minor\0"
    "Abs MT Orientation\0"
    "Abs MT Pressure\0"
    "Abs MT Tracking ID\0"
    "Max Contacts\0"
    "Rel X\0"
    "Rel Y\0"
    // XInput2 tablet
    "Abs X\0"
    "Abs Y\0"
    "Abs Pressure\0"
    "Abs Tilt X\0"
    "Abs Tilt Y\0"
    "Abs Wheel\0"
    "Abs Distance\0"
    "Wacom Serial IDs\0"
    "INTEGER\0"
    "Rel Horiz Wheel\0"
    "Rel Vert Wheel\0"
    "Rel Horiz Scroll\0"
    "Rel Vert Scroll\0"
    "_XSETTINGS_SETTINGS\0"
    "_COMPIZ_DECOR_PENDING\0"
    "_COMPIZ_DECOR_REQUEST\0"
    "_COMPIZ_DECOR_DELETE_PIXMAP\0"
    "_COMPIZ_TOOLKIT_ACTION\0"
    "_GTK_LOAD_ICONTHEMES\0"
    // \0\0 terminates loop.
};

QXcbAtom::Atom QXcbConnection::qatom(xcb_atom_t xatom) const
{
    return static_cast<QXcbAtom::Atom>(std::find(m_allAtoms, m_allAtoms + QXcbAtom::NAtoms, xatom) - m_allAtoms);
}

void QXcbConnection::initializeAllAtoms() {
    const char *names[QXcbAtom::NAtoms];
    const char *ptr = xcb_atomnames;

    int i = 0;
    while (*ptr) {
        names[i++] = ptr;
        while (*ptr)
            ++ptr;
        ++ptr;
    }

    Q_ASSERT(i == QXcbAtom::NPredefinedAtoms);

    const QByteArray settings_atom_name = "_QT_SETTINGS_TIMESTAMP_" + m_displayName;
    names[i++] = settings_atom_name;

    xcb_intern_atom_cookie_t cookies[QXcbAtom::NAtoms];

    Q_ASSERT(i == QXcbAtom::NAtoms);
    for (i = 0; i < QXcbAtom::NAtoms; ++i)
        cookies[i] = xcb_intern_atom(xcb_connection(), false, strlen(names[i]), names[i]);

    for (i = 0; i < QXcbAtom::NAtoms; ++i) {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(xcb_connection(), cookies[i], 0);
        m_allAtoms[i] = reply->atom;
        free(reply);
    }
}

xcb_atom_t QXcbConnection::internAtom(const char *name)
{
    if (!name || *name == 0)
        return XCB_NONE;

    return Q_XCB_REPLY(xcb_intern_atom, xcb_connection(), false, strlen(name), name)->atom;
}

QByteArray QXcbConnection::atomName(xcb_atom_t atom)
{
    if (!atom)
        return QByteArray();

    auto reply = Q_XCB_REPLY(xcb_get_atom_name, xcb_connection(), atom);
    if (!reply)
        qWarning() << "QXcbConnection::atomName: bad Atom" << atom;
    else
        return QByteArray(xcb_get_atom_name_name(reply.get()), xcb_get_atom_name_name_length(reply.get()));

    return QByteArray();
}

const xcb_format_t *QXcbConnection::formatForDepth(uint8_t depth) const
{
    xcb_format_iterator_t iterator =
        xcb_setup_pixmap_formats_iterator(m_setup);

    while (iterator.rem) {
        xcb_format_t *format = iterator.data;
        if (format->depth == depth)
            return format;
        xcb_format_next(&iterator);
    }

    qWarning() << "XCB failed to find an xcb_format_t for depth:" << depth;
    return nullptr;
}

void QXcbConnection::sync()
{
    // from xcb_aux_sync
    xcb_get_input_focus_cookie_t cookie = xcb_get_input_focus(xcb_connection());
    free(xcb_get_input_focus_reply(xcb_connection(), cookie, 0));
}

void QXcbConnection::initializeShm()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_shm_id);
    if (!reply || !reply->present) {
        qCDebug(lcQpaXcb, "MIT-SHM extension is not present on the X server");
        return;
    }
    has_shm = true;

    auto shm_query = Q_XCB_REPLY(xcb_shm_query_version, m_connection);
    if (shm_query) {
        has_shm_fd = (shm_query->major_version == 1 && shm_query->minor_version >= 2) ||
                      shm_query->major_version > 1;
    } else {
        qCWarning(lcQpaXcb, "QXcbConnection: Failed to request MIT-SHM version");
    }

    qCDebug(lcQpaXcb) << "Has MIT-SHM     :" << has_shm;
    qCDebug(lcQpaXcb) << "Has MIT-SHM FD  :" << has_shm_fd;

    // Temporary disable warnings (unless running in debug mode).
    auto logging = const_cast<QLoggingCategory*>(&lcQpaXcb());
    bool wasEnabled = logging->isEnabled(QtMsgType::QtWarningMsg);
    if (!logging->isEnabled(QtMsgType::QtDebugMsg))
        logging->setEnabled(QtMsgType::QtWarningMsg, false);
    if (!QXcbBackingStore::createSystemVShmSegment(this)) {
        qCDebug(lcQpaXcb, "failed to create System V shared memory segment (remote "
                          "X11 connection?), disabling SHM");
        has_shm = has_shm_fd = false;
    }
    if (wasEnabled)
        logging->setEnabled(QtMsgType::QtWarningMsg, true);
}

void QXcbConnection::initializeXFixes()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_xfixes_id);
    if (!reply || !reply->present)
        return;

    auto xfixes_query = Q_XCB_REPLY(xcb_xfixes_query_version, m_connection,
                                    XCB_XFIXES_MAJOR_VERSION,
                                    XCB_XFIXES_MINOR_VERSION);
    if (!xfixes_query || xfixes_query->major_version < 2) {
        qWarning("QXcbConnection: Failed to initialize XFixes");
        return;
    }
    xfixes_first_event = reply->first_event;
    has_xfixes = true;
}

void QXcbConnection::initializeXRender()
{
#if QT_CONFIG(xcb_render)
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_render_id);
    if (!reply || !reply->present) {
        qCDebug(lcQpaXcb, "XRender extension not present on the X server");
        return;
    }

    auto xrender_query = Q_XCB_REPLY(xcb_render_query_version, m_connection,
                                     XCB_RENDER_MAJOR_VERSION,
                                     XCB_RENDER_MINOR_VERSION);
    if (!xrender_query) {
        qCWarning(lcQpaXcb, "xcb_render_query_version failed");
        return;
    }

    has_render_extension = true;
    m_xrenderVersion.first = xrender_query->major_version;
    m_xrenderVersion.second = xrender_query->minor_version;
#endif
}

void QXcbConnection::initializeXRandr()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_randr_id);
    if (!reply || !reply->present)
        return;

    xrandr_first_event = reply->first_event;

    auto xrandr_query = Q_XCB_REPLY(xcb_randr_query_version, m_connection,
                                    XCB_RANDR_MAJOR_VERSION,
                                    XCB_RANDR_MINOR_VERSION);

    has_randr_extension = true;

    if (!xrandr_query || (xrandr_query->major_version < 1 || (xrandr_query->major_version == 1 && xrandr_query->minor_version < 2))) {
        qWarning("QXcbConnection: Failed to initialize XRandr");
        has_randr_extension = false;
    }

    xcb_screen_iterator_t rootIter = xcb_setup_roots_iterator(m_setup);
    for (; rootIter.rem; xcb_screen_next(&rootIter)) {
        xcb_randr_select_input(xcb_connection(),
            rootIter.data->root,
            XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE |
            XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE |
            XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE |
            XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY
        );
    }
}

void QXcbConnection::initializeXinerama()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_xinerama_id);
    if (!reply || !reply->present)
        return;

    auto xinerama_is_active = Q_XCB_REPLY(xcb_xinerama_is_active, m_connection);
    has_xinerama_extension = xinerama_is_active && xinerama_is_active->state;
}

void QXcbConnection::initializeXShape()
{
    const xcb_query_extension_reply_t *xshape_reply = xcb_get_extension_data(m_connection, &xcb_shape_id);
    if (!xshape_reply || !xshape_reply->present)
        return;

    has_shape_extension = true;
    auto shape_query = Q_XCB_REPLY(xcb_shape_query_version, m_connection);
    if (!shape_query) {
        qWarning("QXcbConnection: Failed to initialize SHAPE extension");
    } else if (shape_query->major_version > 1 || (shape_query->major_version == 1 && shape_query->minor_version >= 1)) {
        // The input shape is the only thing added in SHAPE 1.1
        has_input_shape = true;
    }
}

void QXcbConnection::initializeXKB()
{
#if QT_CONFIG(xkb)
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection, &xcb_xkb_id);
    if (!reply || !reply->present) {
        qWarning("Qt: XKEYBOARD extension not present on the X server.");
        xkb_first_event = 0;
        return;
    }
    xkb_first_event = reply->first_event;

    xcb_connection_t *c = connection()->xcb_connection();

    auto xkb_query = Q_XCB_REPLY(xcb_xkb_use_extension, c,
                                 XKB_X11_MIN_MAJOR_XKB_VERSION,
                                 XKB_X11_MIN_MINOR_XKB_VERSION);

    if (!xkb_query) {
        qWarning("Qt: Failed to initialize XKB extension");
        return;
    } else if (!xkb_query->supported) {
        qWarning("Qt: Unsupported XKB version (We want %d %d, but X server has %d %d)",
                 XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION,
                 xkb_query->serverMajor, xkb_query->serverMinor);
        return;
    }

    has_xkb = true;

    const uint16_t required_map_parts = (XCB_XKB_MAP_PART_KEY_TYPES |
        XCB_XKB_MAP_PART_KEY_SYMS |
        XCB_XKB_MAP_PART_MODIFIER_MAP |
        XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
        XCB_XKB_MAP_PART_KEY_ACTIONS |
        XCB_XKB_MAP_PART_KEY_BEHAVIORS |
        XCB_XKB_MAP_PART_VIRTUAL_MODS |
        XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP);

    const uint16_t required_events = (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
        XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
        XCB_XKB_EVENT_TYPE_STATE_NOTIFY);

    // XKB events are reported to all interested clients without regard
    // to the current keyboard input focus or grab state
    xcb_void_cookie_t select = xcb_xkb_select_events_checked(c,
                       XCB_XKB_ID_USE_CORE_KBD,
                       required_events,
                       0,
                       required_events,
                       required_map_parts,
                       required_map_parts,
                       0);

    xcb_generic_error_t *error = xcb_request_check(c, select);
    if (error) {
        free(error);
        qWarning("Qt: failed to select notify events from xcb-xkb");
        return;
    }
#endif
}

QXcbSystemTrayTracker *QXcbConnection::systemTrayTracker() const
{
    if (!m_systemTrayTracker) {
        QXcbConnection *self = const_cast<QXcbConnection *>(this);
        if ((self->m_systemTrayTracker = QXcbSystemTrayTracker::create(self))) {
            connect(m_systemTrayTracker, SIGNAL(systemTrayWindowChanged(QScreen*)),
                    QGuiApplication::platformNativeInterface(), SIGNAL(systemTrayWindowChanged(QScreen*)));
        }
    }
    return m_systemTrayTracker;
}

bool QXcbConnection::xEmbedSystemTrayAvailable()
{
    if (!QGuiApplicationPrivate::platformIntegration())
        return false;
    QXcbConnection *connection = static_cast<QXcbIntegration *>(QGuiApplicationPrivate::platformIntegration())->defaultConnection();
    return connection->systemTrayTracker();
}

bool QXcbConnection::xEmbedSystemTrayVisualHasAlphaChannel()
{
    if (!QGuiApplicationPrivate::platformIntegration())
        return false;
    QXcbConnection *connection = static_cast<QXcbIntegration *>(QGuiApplicationPrivate::platformIntegration())->defaultConnection();
    return connection->systemTrayTracker() && connection->systemTrayTracker()->visualHasAlphaChannel();
}

bool QXcbConnection::event(QEvent *e)
{
    if (e->type() == QEvent::User + 1) {
        QXcbSyncWindowRequest *ev = static_cast<QXcbSyncWindowRequest *>(e);
        QXcbWindow *w = ev->window();
        if (w) {
            w->updateSyncRequestCounter();
            ev->invalidate();
        }
        return true;
    }
    return QObject::event(e);
}

void QXcbSyncWindowRequest::invalidate()
{
    if (m_window) {
        m_window->clearSyncWindowRequest();
        m_window = 0;
    }
}

QXcbConnectionGrabber::QXcbConnectionGrabber(QXcbConnection *connection)
    :m_connection(connection)
{
    connection->grabServer();
}

QXcbConnectionGrabber::~QXcbConnectionGrabber()
{
    if (m_connection)
        m_connection->ungrabServer();
}

void QXcbConnectionGrabber::release()
{
    if (m_connection) {
        m_connection->ungrabServer();
        m_connection = 0;
    }
}

QT_END_NAMESPACE
