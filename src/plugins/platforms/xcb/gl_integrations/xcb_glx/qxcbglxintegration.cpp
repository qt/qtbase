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

#include "qxcbglxintegration.h"

#if QT_CONFIG(xcb_glx)
#include <xcb/glx.h>
#endif

#include "qxcbnativeinterface.h"
#include "qxcbglxwindow.h"
#include "qxcbscreen.h"
#include "qglxintegration.h"

#include <QtGui/QOpenGLContext>

#include "qxcbglxnativeinterfacehandler.h"

#define register        /* C++17 deprecated register */
#include <X11/Xlibint.h>
#undef register

QT_BEGIN_NAMESPACE

#if QT_CONFIG(xcb_glx)
  #if XCB_GLX_MAJOR_VERSION == 1 && XCB_GLX_MINOR_VERSION < 4
    #define XCB_GLX_BUFFER_SWAP_COMPLETE 1
    typedef struct xcb_glx_buffer_swap_complete_event_t {
        uint8_t            response_type;
        uint8_t            pad0;
        uint16_t           sequence;
        uint16_t           event_type;
        uint8_t            pad1[2];
        xcb_glx_drawable_t drawable;
        uint32_t           ust_hi;
        uint32_t           ust_lo;
        uint32_t           msc_hi;
        uint32_t           msc_lo;
        uint32_t           sbc;
    } xcb_glx_buffer_swap_complete_event_t;
  #endif
  typedef struct {
      int type;
      unsigned long serial;       /* # of last request processed by server */
      Bool send_event;            /* true if this came from a SendEvent request */
      Display *display;           /* Display the event was read from */
      Drawable drawable;  /* drawable on which event was requested in event mask */
      int event_type;
      int64_t ust;
      int64_t msc;
      int64_t sbc;
  } QGLXBufferSwapComplete;
#endif

QXcbGlxIntegration::QXcbGlxIntegration()
    : m_connection(nullptr)
    , m_glx_first_event(0)
{
    qCDebug(lcQpaGl) << "Xcb GLX gl-integration created";
}

QXcbGlxIntegration::~QXcbGlxIntegration()
{
}

bool QXcbGlxIntegration::initialize(QXcbConnection *connection)
{
    m_connection = connection;
#if QT_CONFIG(xcb_glx)

    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_connection->xcb_connection(), &xcb_glx_id);
    if (!reply || !reply->present)
        return false;

    m_glx_first_event = reply->first_event;

    auto xglx_query = Q_XCB_REPLY(xcb_glx_query_version, m_connection->xcb_connection(),
                                  XCB_GLX_MAJOR_VERSION,
                                  XCB_GLX_MINOR_VERSION);
    if (!xglx_query) {
        qCWarning(lcQpaGl) << "QXcbConnection: Failed to initialize GLX";
        return false;
    }
#endif

    m_native_interface_handler.reset(new QXcbGlxNativeInterfaceHandler(connection->nativeInterface()));

    qCDebug(lcQpaGl) << "Xcb GLX gl-integration successfully initialized";
    return true;
}

bool QXcbGlxIntegration::handleXcbEvent(xcb_generic_event_t *event, uint responseType)
{
    bool handled = false;
    // Check if a custom XEvent constructor was registered in xlib for this event type, and call it discarding the constructed XEvent if any.
    // XESetWireToEvent might be used by libraries to intercept messages from the X server e.g. the OpenGL lib waiting for DRI2 events.
    Display *xdisplay = static_cast<Display *>(m_connection->xlib_display());
    XLockDisplay(xdisplay);
    bool locked = true;
    Bool (*proc)(Display*, XEvent*, xEvent*) = XESetWireToEvent(xdisplay, responseType, 0);
    if (proc) {
        XESetWireToEvent(xdisplay, responseType, proc);
        XEvent dummy;
        event->sequence = LastKnownRequestProcessed(xdisplay);
        if (proc(xdisplay, &dummy, (xEvent*)event)) {
#if QT_CONFIG(xcb_glx)
            // DRI2 clients don't receive GLXBufferSwapComplete events on the wire.
            // Instead the GLX event is synthesized from the DRI2BufferSwapComplete event
            // by DRI2WireToEvent(). For an application to be able to see the event
            // we have to convert it to an xcb_glx_buffer_swap_complete_event_t and
            // pass it to the native event filter.
            const uint swap_complete = m_glx_first_event + XCB_GLX_BUFFER_SWAP_COMPLETE;
            QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
            if (dispatcher && uint(dummy.type) == swap_complete && responseType != swap_complete) {
                QGLXBufferSwapComplete *xev = reinterpret_cast<QGLXBufferSwapComplete *>(&dummy);
                xcb_glx_buffer_swap_complete_event_t ev;
                memset(&ev, 0, sizeof(xcb_glx_buffer_swap_complete_event_t));
                ev.response_type = xev->type;
                ev.sequence = xev->serial;
                ev.event_type = xev->event_type;
                ev.drawable = xev->drawable;
                ev.ust_hi = xev->ust >> 32;
                ev.ust_lo = xev->ust & 0xffffffff;
                ev.msc_hi = xev->msc >> 32;
                ev.msc_lo = xev->msc & 0xffffffff;
                ev.sbc = xev->sbc & 0xffffffff;
                // Unlock the display before calling the native event filter
                XUnlockDisplay(xdisplay);
                locked = false;
                QByteArray genericEventFilterType = m_connection->nativeInterface()->genericEventFilterType();
                long result = 0;
                handled = dispatcher->filterNativeEvent(genericEventFilterType, &ev, &result);
            }
#endif
        }
    }
    if (locked)
        XUnlockDisplay(xdisplay);
    return handled;
}

QXcbWindow *QXcbGlxIntegration::createWindow(QWindow *window) const
{
    return new QXcbGlxWindow(window);
}

QPlatformOpenGLContext *QXcbGlxIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(context->screen()->handle());
    QGLXContext *platformContext = new QGLXContext(screen, screen->surfaceFormatFor(context->format()),
                                                   context->shareHandle(), context->nativeHandle());
    context->setNativeHandle(platformContext->nativeHandle());
    return platformContext;
}

QPlatformOffscreenSurface *QXcbGlxIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    static bool vendorChecked = false;
    static bool glxPbufferUsable = true;
    if (!vendorChecked) {
        vendorChecked = true;
        Display *display = glXGetCurrentDisplay();
        if (!display)
            display = static_cast<Display *>(m_connection->xlib_display());

        const char *glxvendor = glXGetClientString(display, GLX_VENDOR);
        if (glxvendor) {
            if (!strcmp(glxvendor, "ATI") || !strcmp(glxvendor, "Chromium"))
                glxPbufferUsable = false;
        }
    }
    if (glxPbufferUsable)
        return new QGLXPbuffer(surface);
    else
        return 0; // trigger fallback to hidden QWindow

}

bool QXcbGlxIntegration::supportsThreadedOpenGL() const
{
    return QGLXContext::supportsThreading();
}

bool QXcbGlxIntegration::supportsSwitchableWidgetComposition() const
{
    static bool vendorChecked = false;
    static bool isSwitchableWidgetCompositionAvailable = true;
    if (!vendorChecked) {
        vendorChecked = true;
        Display *display = glXGetCurrentDisplay();
        if (!display)
            display = static_cast<Display *>(m_connection->xlib_display());

        const char *glxvendor = glXGetClientString(display, GLX_VENDOR);
        if (glxvendor) {
            if (!strcmp(glxvendor, "Parallels Inc"))
                isSwitchableWidgetCompositionAvailable = false;
        }
    }

    return isSwitchableWidgetCompositionAvailable;
}


QT_END_NAMESPACE
