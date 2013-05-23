/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#include "qeglfshooks.h"

#include <qpa/qwindowsysteminterface.h>
#include <QThread>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>

QT_BEGIN_NAMESPACE

class EventReader : public QThread
{
public:
    EventReader(xcb_connection_t *connection)
        : m_connection(connection)
    {
    }

    void run();

    xcb_connection_t *connection() { return m_connection; }

private:
    xcb_connection_t *m_connection;
};

class QEglFSX11Hooks : public QEglFSHooks
{
public:
    QEglFSX11Hooks() : m_eventReader(0) {}

    virtual void platformInit();
    virtual void platformDestroy();
    virtual EGLNativeDisplayType platformDisplay() const;
    virtual QSize screenSize() const;
    virtual EGLNativeWindowType createNativeWindow(const QSize &size, const QSurfaceFormat &format);
    virtual void destroyNativeWindow(EGLNativeWindowType window);
    virtual bool hasCapability(QPlatformIntegration::Capability cap) const;

private:
    void sendConnectionEvent(xcb_atom_t a);

    EventReader *m_eventReader;
    xcb_connection_t *m_connection;
    xcb_window_t m_connectionEventListener;
};

static Display *display = 0;

QAtomicInt running;

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

static Qt::MouseButton translateMouseButton(xcb_button_t s)
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

void EventReader::run()
{
    Qt::MouseButtons buttons;

    xcb_generic_event_t *event;
    while (running.load() && (event = xcb_wait_for_event(m_connection))) {
        uint response_type = event->response_type & ~0x80;
        switch (response_type) {
        case XCB_BUTTON_PRESS: {
            xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
            QPoint p(press->event_x, press->event_y);
            buttons = (buttons & ~0x7) | translateMouseButtons(press->state);
            buttons |= translateMouseButton(press->detail);
            QWindowSystemInterface::handleMouseEvent(0, press->time, p, p, buttons);
            break;
            }
        case XCB_BUTTON_RELEASE: {
            xcb_button_release_event_t *release = (xcb_button_release_event_t *)event;
            QPoint p(release->event_x, release->event_y);
            buttons = (buttons & ~0x7) | translateMouseButtons(release->state);
            buttons &= ~translateMouseButton(release->detail);
            QWindowSystemInterface::handleMouseEvent(0, release->time, p, p, buttons);
            break;
            }
        case XCB_MOTION_NOTIFY: {
            xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
            QPoint p(motion->event_x, motion->event_y);
            QWindowSystemInterface::handleMouseEvent(0, motion->time, p, p, buttons);
            break;
            }
        default:
            break;
        }
    }
}

void QEglFSX11Hooks::sendConnectionEvent(xcb_atom_t a)
{
    xcb_client_message_event_t event;
    memset(&event, 0, sizeof(event));

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.sequence = 0;
    event.window = m_connectionEventListener;
    event.type = a;

    xcb_send_event(m_connection, false, m_connectionEventListener, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
    xcb_flush(m_connection);
}

void QEglFSX11Hooks::platformInit()
{
    display = XOpenDisplay(NULL);
    if (!display)
        qFatal("Could not open display");
    XSetEventQueueOwner(display, XCBOwnsEventQueue);

    running.ref();

    m_connection = XGetXCBConnection(display);

    xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(m_connection));

    m_connectionEventListener = xcb_generate_id(m_connection);
    xcb_create_window(m_connection, XCB_COPY_FROM_PARENT,
                      m_connectionEventListener, it.data->root,
                      0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
                      it.data->root_visual, 0, 0);

    m_eventReader = new EventReader(m_connection);
    m_eventReader->start();
}

void QEglFSX11Hooks::platformDestroy()
{
    running.deref();

    sendConnectionEvent(XCB_ATOM_NONE);

    XCloseDisplay(display);

    m_eventReader->wait();
    delete m_eventReader;
    m_eventReader = 0;
}

EGLNativeDisplayType QEglFSX11Hooks::platformDisplay() const
{
    return display;
}

QSize QEglFSX11Hooks::screenSize() const
{
    QList<QByteArray> env = qgetenv("EGLFS_X11_SIZE").split('x');
    if (env.length() != 2)
        return QSize(640, 480);
    return QSize(env.at(0).toInt(), env.at(1).toInt());
}

EGLNativeWindowType QEglFSX11Hooks::createNativeWindow(const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(format);

    Window root = DefaultRootWindow(display);
    XSetWindowAttributes swa;
    memset(&swa, 0, sizeof(swa));
    swa.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask;
    Window win = XCreateWindow(display, root, 0, 0, size.width(), size.height(), 0, CopyFromParent,
                               InputOutput, CopyFromParent, CWEventMask, &swa);
    XMapWindow(display, win);
    XStoreName(display, win, "EGLFS");

    return win;
}

void QEglFSX11Hooks::destroyNativeWindow(EGLNativeWindowType window)
{
    XDestroyWindow(display, window);
}

bool QEglFSX11Hooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    Q_UNUSED(cap);
    return false;
}

static QEglFSX11Hooks eglFSX11Hooks;
QEglFSHooks *platformHooks = &eglFSX11Hooks;

QT_END_NAMESPACE
