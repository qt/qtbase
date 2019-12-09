/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
#include "qxcbconnection_basic.h"
#include "qxcbbackingstore.h" // for createSystemVShmSegment()

#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/sync.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>
#include <xcb/render.h>
#include <xcb/xinput.h>
#define explicit dont_use_cxx_explicit
#include <xcb/xkb.h>
#undef explicit

#if QT_CONFIG(xcb_xlib)
#define register        /* C++17 deprecated register */
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#undef register
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaXcb, "qt.qpa.xcb")

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
    if (conn != nullptr) {
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

QXcbBasicConnection::QXcbBasicConnection(const char *displayName)
    : m_displayName(displayName ? QByteArray(displayName) : qgetenv("DISPLAY"))
{
#if QT_CONFIG(xcb_xlib)
    Display *dpy = XOpenDisplay(m_displayName.constData());
    if (dpy) {
        m_primaryScreenNumber = DefaultScreen(dpy);
        m_xcbConnection = XGetXCBConnection(dpy);
        XSetEventQueueOwner(dpy, XCBOwnsEventQueue);
        XSetErrorHandler(nullErrorHandler);
        XSetIOErrorHandler(ioErrorHandler);
        m_xlibDisplay = dpy;
    }
#else
    m_xcbConnection = xcb_connect(m_displayName.constData(), &m_primaryScreenNumber);
#endif
    if (Q_UNLIKELY(!isConnected())) {
        qCWarning(lcQpaXcb, "could not connect to display %s", m_displayName.constData());
        return;
    }

    m_setup = xcb_get_setup(m_xcbConnection);
    m_xcbAtom.initialize(m_xcbConnection);
    m_maximumRequestLength = xcb_get_maximum_request_length(m_xcbConnection);

    xcb_extension_t *extensions[] = {
        &xcb_shm_id, &xcb_xfixes_id, &xcb_randr_id, &xcb_shape_id, &xcb_sync_id,
        &xcb_render_id, &xcb_xkb_id, &xcb_input_id, nullptr
    };

    for (xcb_extension_t **ext_it = extensions; *ext_it; ++ext_it)
        xcb_prefetch_extension_data (m_xcbConnection, *ext_it);

    initializeXSync();
    if (!qEnvironmentVariableIsSet("QT_XCB_NO_MITSHM"))
        initializeShm();
    if (!qEnvironmentVariableIsSet("QT_XCB_NO_XRANDR"))
        initializeXRandr();
    if (!m_hasXRandr)
        initializeXinerama();
    initializeXFixes();
    initializeXRender();
    if (!qEnvironmentVariableIsSet("QT_XCB_NO_XI2"))
        initializeXInput2();
    initializeXShape();
    initializeXKB();
}

QXcbBasicConnection::~QXcbBasicConnection()
{
    if (isConnected()) {
#if QT_CONFIG(xcb_xlib)
        XCloseDisplay(static_cast<Display *>(m_xlibDisplay));
#else
        xcb_disconnect(m_xcbConnection);
#endif
    }
}

size_t QXcbBasicConnection::maxRequestDataBytes(size_t requestSize) const
{
    if (hasBigRequest())
        requestSize += 4; // big-request encoding adds 4 bytes

    return m_maximumRequestLength * 4 - requestSize;
}

xcb_atom_t QXcbBasicConnection::internAtom(const char *name)
{
    if (!name || *name == 0)
        return XCB_NONE;

    return Q_XCB_REPLY(xcb_intern_atom, m_xcbConnection, false, strlen(name), name)->atom;
}

QByteArray QXcbBasicConnection::atomName(xcb_atom_t atom)
{
    if (!atom)
        return QByteArray();

    auto reply = Q_XCB_REPLY(xcb_get_atom_name, m_xcbConnection, atom);
    if (reply)
        return QByteArray(xcb_get_atom_name_name(reply.get()), xcb_get_atom_name_name_length(reply.get()));

    qCWarning(lcQpaXcb) << "atomName: bad atom" << atom;
    return QByteArray();
}

bool QXcbBasicConnection::hasBigRequest() const
{
    return m_maximumRequestLength > m_setup->maximum_request_length;
}

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

bool QXcbBasicConnection::isXIEvent(xcb_generic_event_t *event) const
{
    qt_xcb_ge_event_t *e = reinterpret_cast<qt_xcb_ge_event_t *>(event);
    return e->extension == m_xiOpCode;
}

bool QXcbBasicConnection::isXIType(xcb_generic_event_t *event, uint16_t type) const
{
    if (!isXIEvent(event))
        return false;

    auto *e = reinterpret_cast<qt_xcb_ge_event_t *>(event);
    return e->event_type == type;
}

bool QXcbBasicConnection::isXFixesType(uint responseType, int eventType) const
{
    return m_hasXFixes && responseType == m_xfixesFirstEvent + eventType;
}

bool QXcbBasicConnection::isXRandrType(uint responseType, int eventType) const
{
    return m_hasXRandr && responseType == m_xrandrFirstEvent + eventType;
}

bool QXcbBasicConnection::isXkbType(uint responseType) const
{
    return m_hasXkb && responseType == m_xkbFirstEvent;
}

void QXcbBasicConnection::initializeXSync()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_xcbConnection, &xcb_sync_id);
    if (!reply || !reply->present)
        return;

    m_hasXSync = true;
}

void QXcbBasicConnection::initializeShm()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_xcbConnection, &xcb_shm_id);
    if (!reply || !reply->present) {
        qCDebug(lcQpaXcb, "MIT-SHM extension is not present on the X server");
        return;
    }

    auto shmQuery = Q_XCB_REPLY(xcb_shm_query_version, m_xcbConnection);
    if (!shmQuery) {
        qCWarning(lcQpaXcb, "failed to request MIT-SHM version");
        return;
    }

    m_hasShm = true;
    m_hasShmFd = (shmQuery->major_version == 1 && shmQuery->minor_version >= 2) ||
                  shmQuery->major_version > 1;

    qCDebug(lcQpaXcb) << "Has MIT-SHM     :" << m_hasShm;
    qCDebug(lcQpaXcb) << "Has MIT-SHM FD  :" << m_hasShmFd;

    // Temporary disable warnings (unless running in debug mode).
    auto logging = const_cast<QLoggingCategory*>(&lcQpaXcb());
    bool wasEnabled = logging->isEnabled(QtMsgType::QtWarningMsg);
    if (!logging->isEnabled(QtMsgType::QtDebugMsg))
        logging->setEnabled(QtMsgType::QtWarningMsg, false);
    if (!QXcbBackingStore::createSystemVShmSegment(m_xcbConnection)) {
        qCDebug(lcQpaXcb, "failed to create System V shared memory segment (remote "
                          "X11 connection?), disabling SHM");
        m_hasShm = m_hasShmFd = false;
    }
    if (wasEnabled)
        logging->setEnabled(QtMsgType::QtWarningMsg, true);
}

void QXcbBasicConnection::initializeXRender()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_xcbConnection, &xcb_render_id);
    if (!reply || !reply->present) {
        qCDebug(lcQpaXcb, "XRender extension not present on the X server");
        return;
    }

    auto xrenderQuery = Q_XCB_REPLY(xcb_render_query_version, m_xcbConnection,
                                     XCB_RENDER_MAJOR_VERSION,
                                     XCB_RENDER_MINOR_VERSION);
    if (!xrenderQuery) {
        qCWarning(lcQpaXcb, "xcb_render_query_version failed");
        return;
    }

    m_hasXRender = true;
    m_xrenderVersion.first = xrenderQuery->major_version;
    m_xrenderVersion.second = xrenderQuery->minor_version;
}

void QXcbBasicConnection::initializeXinerama()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_xcbConnection, &xcb_xinerama_id);
    if (!reply || !reply->present)
        return;

    auto xineramaActive = Q_XCB_REPLY(xcb_xinerama_is_active, m_xcbConnection);
    if (xineramaActive && xineramaActive->state)
        m_hasXinerama = true;
}

void QXcbBasicConnection::initializeXFixes()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_xcbConnection, &xcb_xfixes_id);
    if (!reply || !reply->present)
        return;

    auto xfixesQuery = Q_XCB_REPLY(xcb_xfixes_query_version, m_xcbConnection,
                                    XCB_XFIXES_MAJOR_VERSION,
                                    XCB_XFIXES_MINOR_VERSION);
    if (!xfixesQuery || xfixesQuery->major_version < 2) {
        qCWarning(lcQpaXcb, "failed to initialize XFixes");
        return;
    }

    m_hasXFixes = true;
    m_xfixesFirstEvent = reply->first_event;
}

void QXcbBasicConnection::initializeXRandr()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_xcbConnection, &xcb_randr_id);
    if (!reply || !reply->present)
        return;

    auto xrandrQuery = Q_XCB_REPLY(xcb_randr_query_version, m_xcbConnection,
                                    XCB_RANDR_MAJOR_VERSION,
                                    XCB_RANDR_MINOR_VERSION);
    if (!xrandrQuery || (xrandrQuery->major_version < 1 ||
                        (xrandrQuery->major_version == 1 && xrandrQuery->minor_version < 2))) {
        qCWarning(lcQpaXcb, "failed to initialize XRandr");
        return;
    }

    m_hasXRandr = true;
    m_xrandrFirstEvent = reply->first_event;
}

void QXcbBasicConnection::initializeXInput2()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_xcbConnection, &xcb_input_id);
    if (!reply || !reply->present) {
        qCDebug(lcQpaXcb, "XInput extension is not present on the X server");
        return;
    }

    auto xinputQuery = Q_XCB_REPLY(xcb_input_xi_query_version, m_xcbConnection, 2, 2);
    if (!xinputQuery || xinputQuery->major_version != 2) {
        qCWarning(lcQpaXcb, "X server does not support XInput 2");
        return;
    }

    qCDebug(lcQpaXcb, "Using XInput version %d.%d",
            xinputQuery->major_version, xinputQuery->minor_version);

    m_xi2Enabled = true;
    m_xiOpCode = reply->major_opcode;
    m_xinputFirstEvent = reply->first_event;
    m_xi2Minor = xinputQuery->minor_version;
}

void QXcbBasicConnection::initializeXShape()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_xcbConnection, &xcb_shape_id);
    if (!reply || !reply->present)
        return;

    m_hasXhape = true;

    auto shapeQuery = Q_XCB_REPLY(xcb_shape_query_version, m_xcbConnection);
    if (!shapeQuery) {
        qCWarning(lcQpaXcb, "failed to initialize XShape extension");
        return;
    }

    if (shapeQuery->major_version > 1 || (shapeQuery->major_version == 1 && shapeQuery->minor_version >= 1)) {
        // The input shape is the only thing added in SHAPE 1.1
        m_hasInputShape = true;
    }
}

void QXcbBasicConnection::initializeXKB()
{
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_xcbConnection, &xcb_xkb_id);
    if (!reply || !reply->present) {
        qCWarning(lcQpaXcb, "XKeyboard extension not present on the X server");
        return;
    }

    int wantMajor = 1;
    int wantMinor = 0;
    auto xkbQuery = Q_XCB_REPLY(xcb_xkb_use_extension, m_xcbConnection, wantMajor, wantMinor);
    if (!xkbQuery) {
        qCWarning(lcQpaXcb, "failed to initialize XKeyboard extension");
        return;
    }
    if (!xkbQuery->supported) {
        qCWarning(lcQpaXcb, "unsupported XKB version (we want %d.%d, but X server has %d.%d)",
                  wantMajor, wantMinor, xkbQuery->serverMajor, xkbQuery->serverMinor);
        return;
    }

    m_hasXkb = true;
    m_xkbFirstEvent = reply->first_event;
}

QT_END_NAMESPACE
