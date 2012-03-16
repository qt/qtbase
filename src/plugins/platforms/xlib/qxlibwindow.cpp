/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/private/qguiapplication_p.h>
#include "qxlibwindow.h"

#include "qxlibintegration.h"
#include "qxlibscreen.h"
#include "qxlibkeyboard.h"
#include "qxlibstatic.h"
#include "qxlibdisplay.h"

#if !defined(QT_NO_OPENGL)
#if !defined(QT_OPENGL_ES_2)
#include "qglxintegration.h"
#include "private/qglxconvenience_p.h"
#else
#include "../eglconvenience/qeglconvenience.h"
#include "../eglconvenience/qeglplatformcontext.h"
#include "../eglconvenience/qxlibeglintegration.h"
#endif  //QT_OPENGL_ES_2
#endif //QT_NO_OPENGL


#include <QtGui/QWindowSystemInterface>
#include <QSocketNotifier>
#include <QGuiApplication>
#include <QStyleHints>
#include <QDebug>

//#define MYX11_DEBUG

QT_BEGIN_NAMESPACE

QHash<Window, QXlibWindow *> QXlibWindow::windowMap;


QXlibWindow::QXlibWindow(QWindow *window)
    : QPlatformWindow(window)
    , mGLContext(0)
    , mScreen(QXlibScreen::testLiteScreenForWidget(window))
{
    int x = window->x();
    int y = window->y();
    int w = window->width();
    int h = window->height();

    mSurfaceFormat = window->requestedFormat();

#if !defined(QT_NO_OPENGL)
    if(window->surfaceType() == QWindow::OpenGLSurface) {
#if !defined(QT_OPENGL_ES_2)
        XVisualInfo *visualInfo = qglx_findVisualInfo(mScreen->display()->nativeDisplay(), mScreen->xScreenNumber(), &mSurfaceFormat);
        if (!visualInfo)
            qFatal("Could not initialize GLX");
#else
        QPlatformWindowFormat windowFormat = correctColorBuffers(window->platformWindowFormat());

        EGLDisplay eglDisplay = mScreen->eglDisplay();
        EGLConfig eglConfig = q_configFromQPlatformWindowFormat(eglDisplay,windowFormat);
        VisualID id = QXlibEglIntegration::getCompatibleVisualId(mScreen->display()->nativeDisplay(), eglDisplay, eglConfig);

        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
        visualInfoTemplate.visualid = id;

        XVisualInfo *visualInfo;
        int matchingCount = 0;
        visualInfo = XGetVisualInfo(mScreen->display()->nativeDisplay(), VisualIDMask, &visualInfoTemplate, &matchingCount);
        if (!visualInfo)
            qFatal("Could not initialize EGL");
#endif //!defined(QT_OPENGL_ES_2)
        mDepth = visualInfo->depth;
        mFormat = (mDepth == 32) ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
        mVisual = visualInfo->visual;
        Colormap cmap = XCreateColormap(mScreen->display()->nativeDisplay(), mScreen->rootWindow(), visualInfo->visual, AllocNone);

        XSetWindowAttributes a;
        a.background_pixel = WhitePixel(mScreen->display()->nativeDisplay(), mScreen->xScreenNumber());
        a.border_pixel = BlackPixel(mScreen->display()->nativeDisplay(), mScreen->xScreenNumber());
        a.colormap = cmap;
        x_window = XCreateWindow(mScreen->display()->nativeDisplay(), mScreen->rootWindow(),x, y, w, h,
                                 0, visualInfo->depth, InputOutput, visualInfo->visual,
                                 CWBackPixel|CWBorderPixel|CWColormap, &a);
        XFree(visualInfo);
    } else
#endif //!defined(QT_NO_OPENGL)
    {
        mDepth = mScreen->depth();
        mFormat = (mDepth == 32) ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
        mVisual = mScreen->defaultVisual();

        x_window = XCreateSimpleWindow(mScreen->display()->nativeDisplay(), mScreen->rootWindow(),
                                       x, y, w, h, 0 /*border_width*/,
                                       mScreen->blackPixel(), mScreen->whitePixel());
    }

#ifdef MYX11_DEBUG
    qDebug() << "QTestLiteWindow::QTestLiteWindow creating" << hex << x_window << window;
#endif

    XSetWindowBackgroundPixmap(mScreen->display()->nativeDisplay(), x_window, XNone);

    XSelectInput(mScreen->display()->nativeDisplay(), x_window,
                 ExposureMask | KeyPressMask | KeyReleaseMask |
                 EnterWindowMask | LeaveWindowMask | FocusChangeMask |
                 PointerMotionMask | ButtonPressMask |  ButtonReleaseMask |
                 ButtonMotionMask | PropertyChangeMask |
                 StructureNotifyMask);

    gc = createGC();

    Atom protocols[5];
    int n = 0;
    protocols[n++] = QXlibStatic::atom(QXlibStatic::WM_DELETE_WINDOW);        // support del window protocol
    protocols[n++] = QXlibStatic::atom(QXlibStatic::WM_TAKE_FOCUS);                // support take focus window protocol
//    protocols[n++] = QXlibStatic::atom(QXlibStatic::_NET_WM_PING);                // support _NET_WM_PING protocol
#ifndef QT_NO_XSYNC
    protocols[n++] = QXlibStatic::atom(QXlibStatic::_NET_WM_SYNC_REQUEST);        // support _NET_WM_SYNC_REQUEST protocol
#endif // QT_NO_XSYNC
    if (window->windowFlags() & Qt::WindowContextHelpButtonHint)
        protocols[n++] = QXlibStatic::atom(QXlibStatic::_NET_WM_CONTEXT_HELP);
    XSetWMProtocols(mScreen->display()->nativeDisplay(), x_window, protocols, n);

    windowMap.insert(x_window, this);
}



QXlibWindow::~QXlibWindow()
{
#ifdef MYX11_DEBUG
    qDebug() << "~QTestLiteWindow" << hex << x_window;
#endif

    windowMap.remove(x_window);

    delete mGLContext;
    XFreeGC(mScreen->display()->nativeDisplay(), gc);
    XDestroyWindow(mScreen->display()->nativeDisplay(), x_window);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Mouse event stuff
static Qt::MouseButtons translateMouseButtons(int s)
{
    Qt::MouseButtons ret = 0;
    if (s & Button1Mask)
        ret |= Qt::LeftButton;
    if (s & Button2Mask)
        ret |= Qt::MidButton;
    if (s & Button3Mask)
        ret |= Qt::RightButton;
    return ret;
}



void QXlibWindow::handleMouseEvent(QEvent::Type type, XButtonEvent *e)
{
    static QPoint mousePoint;

    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = translateMouseButtons(e->state);
    Qt::KeyboardModifiers modifiers = mScreen->keyboard()->translateModifiers(e->state);
    if (type != QEvent::MouseMove) {
        switch (e->button) {
        case Button1: button = Qt::LeftButton; break;
        case Button2: button = Qt::MidButton; break;
        case Button3: button = Qt::RightButton; break;
        case Button4:
        case Button5:
        case 6:
        case 7: {
            //mouse wheel
            if (type == QEvent::MouseButtonPress) {
                //logic borrowed from qapplication_x11.cpp
                int delta = 120 * ((e->button == Button4 || e->button == 6) ? 1 : -1);
                bool hor = (((e->button == Button4 || e->button == Button5)
                             && (modifiers & Qt::AltModifier))
                            || (e->button == 6 || e->button == 7));
                QWindowSystemInterface::handleWheelEvent(window(), e->time,
                                                      QPoint(e->x, e->y),
                                                      QPoint(e->x_root, e->y_root),
                                                      delta, hor ? Qt::Horizontal : Qt::Vertical);
            }
            return;
        }
        case 8: button = Qt::BackButton; break;      // Also known as Qt::ExtraButton1
        case 9: button = Qt::ForwardButton; break;   // Also known as Qt::ExtraButton2
        case 10: button = Qt::ExtraButton3; break;
        case 11: button = Qt::ExtraButton4; break;
        case 12: button = Qt::ExtraButton5; break;
        case 13: button = Qt::ExtraButton6; break;
        case 14: button = Qt::ExtraButton7; break;
        case 15: button = Qt::ExtraButton8; break;
        case 16: button = Qt::ExtraButton9; break;
        case 17: button = Qt::ExtraButton10; break;
        case 18: button = Qt::ExtraButton11; break;
        case 19: button = Qt::ExtraButton12; break;
        case 20: button = Qt::ExtraButton13; break;
        case 21: button = Qt::ExtraButton14; break;
        case 22: button = Qt::ExtraButton15; break;
        case 23: button = Qt::ExtraButton16; break;
        case 24: button = Qt::ExtraButton17; break;
        case 25: button = Qt::ExtraButton18; break;
        case 26: button = Qt::ExtraButton19; break;
        case 27: button = Qt::ExtraButton20; break;
        case 28: button = Qt::ExtraButton21; break;
        case 29: button = Qt::ExtraButton22; break;
        case 30: button = Qt::ExtraButton23; break;
        case 31: button = Qt::ExtraButton24; break;
        default: break;
        }
    }

    buttons ^= button; // X event uses state *before*, Qt uses state *after*

    QWindowSystemInterface::handleMouseEvent(window(), e->time, QPoint(e->x, e->y),
                                          QPoint(e->x_root, e->y_root),
                                          buttons);

    mousePoint = QPoint(e->x_root, e->y_root);
}

void QXlibWindow::handleCloseEvent()
{
    QWindowSystemInterface::handleCloseEvent(window());
}


void QXlibWindow::handleEnterEvent()
{
    QWindowSystemInterface::handleEnterEvent(window());
}

void QXlibWindow::handleLeaveEvent()
{
    QWindowSystemInterface::handleLeaveEvent(window());
}

void QXlibWindow::handleFocusInEvent()
{
    QWindowSystemInterface::handleWindowActivated(window());
}

void QXlibWindow::handleFocusOutEvent()
{
    QWindowSystemInterface::handleWindowActivated(0);
}



void QXlibWindow::setGeometry(const QRect &rect)
{
    XMoveResizeWindow(mScreen->display()->nativeDisplay(), x_window, rect.x(), rect.y(), rect.width(), rect.height());
    QPlatformWindow::setGeometry(rect);
}


Qt::WindowFlags QXlibWindow::windowFlags() const
{
    return mWindowFlags;
}

WId QXlibWindow::winId() const
{
    return x_window;
}

void QXlibWindow::setParent(const QPlatformWindow *parent)
{
    QPoint topLeft = geometry().topLeft();
    WId parentWinId = parent ? parent->winId() : mScreen->rootWindow();
    XReparentWindow(mScreen->display()->nativeDisplay(),x_window,parentWinId,topLeft.x(),topLeft.y());
}

void QXlibWindow::raise()
{
    XRaiseWindow(mScreen->display()->nativeDisplay(), x_window);
}

void QXlibWindow::lower()
{
    XLowerWindow(mScreen->display()->nativeDisplay(), x_window);
}

void QXlibWindow::setWindowTitle(const QString &title)
{
    QByteArray ba = title.toLatin1(); //We're not making a general solution here...
    XTextProperty windowName;
    windowName.value    = (unsigned char *)ba.constData();
    windowName.encoding = XA_STRING;
    windowName.format   = 8;
    windowName.nitems   = ba.length();

    XSetWMName(mScreen->display()->nativeDisplay(), x_window, &windowName);
}

GC QXlibWindow::createGC()
{
    GC gc;

    gc = XCreateGC(mScreen->display()->nativeDisplay(), x_window, 0, 0);
    if (gc < 0) {
        qWarning("QTestLiteWindow::createGC() could not create GC");
    }
    return gc;
}

void QXlibWindow::requestActivateWindow()
{
    XSetInputFocus(mScreen->display()->nativeDisplay(), x_window, XRevertToParent, CurrentTime);
}

void QXlibWindow::resizeEvent(XConfigureEvent *e)
{
    int xpos = geometry().x();
    int ypos = geometry().y();
    if ((e->width != geometry().width() || e->height != geometry().height()) && e->x == 0 && e->y == 0) {
        //qDebug() << "resize with bogus pos" << e->x << e->y << e->width << e->height << "window"<< hex << window;
    } else {
        //qDebug() << "geometry change" << e->x << e->y << e->width << e->height << "window"<< hex << window;
        xpos = e->x;
        ypos = e->y;
    }
#ifdef MYX11_DEBUG
    qDebug() << hex << x_window << dec << "ConfigureNotify" << e->x << e->y << e->width << e->height <<
                "geometry" << xpos << ypos << e->width << e->height;
#endif

    QRect newRect(xpos, ypos, e->width, e->height);
    QWindowSystemInterface::handleGeometryChange(window(), newRect);
}

void QXlibWindow::mousePressEvent(XButtonEvent *e)
{
    static long prevTime = 0;
    static Window prevWindow;
    static int prevX = -999;
    static int prevY = -999;

    QEvent::Type type = QEvent::MouseButtonPress;

    const int doubleClickInterval = qApp->styleHints()->mouseDoubleClickInterval();
    if (e->window == prevWindow && long(e->time) - prevTime < doubleClickInterval
        && qAbs(e->x - prevX) < 5 && qAbs(e->y - prevY) < 5) {
        type = QEvent::MouseButtonDblClick;
        prevTime = e->time - doubleClickInterval; //no double click next time
    } else {
        prevTime = e->time;
    }
    prevWindow = e->window;
    prevX = e->x;
    prevY = e->y;

    handleMouseEvent(type, e);
}

QXlibMWMHints QXlibWindow::getMWMHints() const
{
    QXlibMWMHints mwmhints;

    Atom type;
    int format;
    ulong nitems, bytesLeft;
    uchar *data = 0;
    Atom atomForMotifWmHints = QXlibStatic::atom(QXlibStatic::_MOTIF_WM_HINTS);
    if ((XGetWindowProperty(mScreen->display()->nativeDisplay(), x_window, atomForMotifWmHints, 0, 5, false,
                            atomForMotifWmHints, &type, &format, &nitems, &bytesLeft,
                            &data) == Success)
        && (type == atomForMotifWmHints
            && format == 32
            && nitems >= 5)) {
        mwmhints = *(reinterpret_cast<QXlibMWMHints *>(data));
    } else {
        mwmhints.flags = 0L;
        mwmhints.functions = MWM_FUNC_ALL;
        mwmhints.decorations = MWM_DECOR_ALL;
        mwmhints.input_mode = 0L;
        mwmhints.status = 0L;
    }

    if (data)
        XFree(data);

    return mwmhints;
}

void QXlibWindow::setMWMHints(const QXlibMWMHints &mwmhints)
{
    Atom atomForMotifWmHints = QXlibStatic::atom(QXlibStatic::_MOTIF_WM_HINTS);
    if (mwmhints.flags != 0l) {
        XChangeProperty(mScreen->display()->nativeDisplay(), x_window,
                        atomForMotifWmHints, atomForMotifWmHints, 32,
                        PropModeReplace, (unsigned char *) &mwmhints, 5);
    } else {
        XDeleteProperty(mScreen->display()->nativeDisplay(), x_window, atomForMotifWmHints);
    }
}

// Returns true if we should set WM_TRANSIENT_FOR on \a w
static inline bool isTransient(const QWindow *w)
{
    return (w->windowType() == Qt::Dialog
             || w->windowType() == Qt::Sheet
             || w->windowType() == Qt::Tool
             || w->windowType() == Qt::SplashScreen
             || w->windowType() == Qt::ToolTip
             || w->windowType() == Qt::Drawer
             || w->windowType() == Qt::Popup);
}

QVector<Atom> QXlibWindow::getNetWmState() const
{
    QVector<Atom> returnValue;

    // Don't read anything, just get the size of the property data
    Atom actualType;
    int actualFormat;
    ulong propertyLength;
    ulong bytesLeft;
    uchar *propertyData = 0;
    if (XGetWindowProperty(mScreen->display()->nativeDisplay(), x_window, QXlibStatic::atom(QXlibStatic::_NET_WM_STATE), 0, 0,
                           False, XA_ATOM, &actualType, &actualFormat,
                           &propertyLength, &bytesLeft, &propertyData) == Success
        && actualType == XA_ATOM && actualFormat == 32) {
        returnValue.resize(bytesLeft / 4);
        XFree((char*) propertyData);

        // fetch all data
        if (XGetWindowProperty(mScreen->display()->nativeDisplay(), x_window, QXlibStatic::atom(QXlibStatic::_NET_WM_STATE), 0,
                               returnValue.size(), False, XA_ATOM, &actualType, &actualFormat,
                               &propertyLength, &bytesLeft, &propertyData) != Success) {
            returnValue.clear();
        } else if (propertyLength != (ulong)returnValue.size()) {
            returnValue.resize(propertyLength);
        }

        // put it into netWmState
        if (!returnValue.isEmpty()) {
            memcpy(returnValue.data(), propertyData, returnValue.size() * sizeof(Atom));
        }
        XFree((char*) propertyData);
    }

    return returnValue;
}

Qt::WindowFlags QXlibWindow::setWindowFlags(Qt::WindowFlags flags)
{
//    Q_ASSERT(flags & Qt::Window);
    mWindowFlags = flags;

#ifdef MYX11_DEBUG
    qDebug() << "QTestLiteWindow::setWindowFlags" << hex << x_window << "flags" << flags;
#endif
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    if (type == Qt::ToolTip)
        flags |= Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint;
    if (type == Qt::Popup)
        flags |= Qt::X11BypassWindowManagerHint;

    bool topLevel = (flags & Qt::Window);
    bool popup = (type == Qt::Popup);
    bool dialog = (type == Qt::Dialog
                   || type == Qt::Sheet);
    bool desktop = (type == Qt::Desktop);
    bool tool = (type == Qt::Tool || type == Qt::SplashScreen
                 || type == Qt::ToolTip || type == Qt::Drawer);

    Q_UNUSED(topLevel);
    Q_UNUSED(dialog);
    Q_UNUSED(desktop);

    bool tooltip = (type == Qt::ToolTip);

    XSetWindowAttributes wsa;

    QXlibMWMHints mwmhints;
    mwmhints.flags = 0L;
    mwmhints.functions = 0L;
    mwmhints.decorations = 0;
    mwmhints.input_mode = 0L;
    mwmhints.status = 0L;


    ulong wsa_mask = 0;
    if (type != Qt::SplashScreen) { // && customize) {
        mwmhints.flags |= MWM_HINTS_DECORATIONS;

        bool customize = flags & Qt::CustomizeWindowHint;
        if (!(flags & Qt::FramelessWindowHint) && !(customize && !(flags & Qt::WindowTitleHint))) {
            mwmhints.decorations |= MWM_DECOR_BORDER;
            mwmhints.decorations |= MWM_DECOR_RESIZEH;

            if (flags & Qt::WindowTitleHint)
                mwmhints.decorations |= MWM_DECOR_TITLE;

            if (flags & Qt::WindowSystemMenuHint)
                mwmhints.decorations |= MWM_DECOR_MENU;

            if (flags & Qt::WindowMinimizeButtonHint) {
                mwmhints.decorations |= MWM_DECOR_MINIMIZE;
                mwmhints.functions |= MWM_FUNC_MINIMIZE;
            }

            if (flags & Qt::WindowMaximizeButtonHint) {
                mwmhints.decorations |= MWM_DECOR_MAXIMIZE;
                mwmhints.functions |= MWM_FUNC_MAXIMIZE;
            }

            if (flags & Qt::WindowCloseButtonHint)
                mwmhints.functions |= MWM_FUNC_CLOSE;
        }
    } else {
        // if type == Qt::SplashScreen
        mwmhints.decorations = MWM_DECOR_ALL;
    }

    if (tool) {
        wsa.save_under = True;
        wsa_mask |= CWSaveUnder;
    }

    if (flags & Qt::X11BypassWindowManagerHint) {
        wsa.override_redirect = True;
        wsa_mask |= CWOverrideRedirect;
    }
#if 0
    if (wsa_mask && initializeWindow) {
        Q_ASSERT(id);
        XChangeWindowAttributes(dpy, id, wsa_mask, &wsa);
    }
#endif
    if (mwmhints.functions != 0) {
        mwmhints.flags |= MWM_HINTS_FUNCTIONS;
        mwmhints.functions |= MWM_FUNC_MOVE | MWM_FUNC_RESIZE;
    } else {
        mwmhints.functions = MWM_FUNC_ALL;
    }

    if (!(flags & Qt::FramelessWindowHint)
        && flags & Qt::CustomizeWindowHint
        && flags & Qt::WindowTitleHint
        && !(flags &
             (Qt::WindowMinimizeButtonHint
              | Qt::WindowMaximizeButtonHint
              | Qt::WindowCloseButtonHint))) {
        // a special case - only the titlebar without any button
        mwmhints.flags = MWM_HINTS_FUNCTIONS;
        mwmhints.functions = MWM_FUNC_MOVE | MWM_FUNC_RESIZE;
        mwmhints.decorations = 0;
    }

    if (window()->windowModality() == Qt::WindowModal) {
        mwmhints.input_mode = MWM_INPUT_PRIMARY_APPLICATION_MODAL;
    } else if (window()->windowModality() == Qt::ApplicationModal) {
        mwmhints.input_mode = MWM_INPUT_FULL_APPLICATION_MODAL;
    }

    setMWMHints(mwmhints);

    QVector<Atom> netWmState = getNetWmState();

    if (flags & Qt::WindowStaysOnTopHint) {
        if (flags & Qt::WindowStaysOnBottomHint)
            qWarning() << "QWindow: Incompatible window flags: the window can't be on top and on bottom at the same time";
        if (!netWmState.contains(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_ABOVE)))
            netWmState.append(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_ABOVE));
        if (!netWmState.contains(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_STAYS_ON_TOP)))
            netWmState.append(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_STAYS_ON_TOP));
    } else if (flags & Qt::WindowStaysOnBottomHint) {
        if (!netWmState.contains(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_BELOW)))
            netWmState.append(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_BELOW));
    }
    if (window()->windowState() & Qt::WindowFullScreen) {
        if (!netWmState.contains(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_FULLSCREEN)))
            netWmState.append(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_FULLSCREEN));
    }
    if (window()->windowState() & Qt::WindowMaximized) {
        if (!netWmState.contains(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_MAXIMIZED_HORZ)))
            netWmState.append(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_MAXIMIZED_HORZ));
        if (!netWmState.contains(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_MAXIMIZED_VERT)))
            netWmState.append(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_MAXIMIZED_VERT));
    }
    if (window()->windowModality() != Qt::NonModal) {
        if (!netWmState.contains(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_MODAL)))
            netWmState.append(QXlibStatic::atom(QXlibStatic::_NET_WM_STATE_MODAL));
    }

    if (!netWmState.isEmpty()) {
        XChangeProperty(mScreen->display()->nativeDisplay(), x_window,
                        QXlibStatic::atom(QXlibStatic::_NET_WM_STATE), XA_ATOM, 32, PropModeReplace,
                        (unsigned char *) netWmState.data(), netWmState.size());
    } else {
        XDeleteProperty(mScreen->display()->nativeDisplay(), x_window, QXlibStatic::atom(QXlibStatic::_NET_WM_STATE));
    }

//##### only if initializeWindow???

    if (popup || tooltip) {                        // popup widget
#ifdef MYX11_DEBUG
        qDebug() << "Doing XChangeWindowAttributes for popup" << wsa.override_redirect;
#endif
        // set EWMH window types
        // setNetWmWindowTypes();

        wsa.override_redirect = True;
        wsa.save_under = True;
        XChangeWindowAttributes(mScreen->display()->nativeDisplay(), x_window, CWOverrideRedirect | CWSaveUnder,
                                &wsa);
    } else {
#ifdef MYX11_DEBUG
        qDebug() << "Doing XChangeWindowAttributes for non-popup";
#endif
    }

    return flags;
}

Qt::WindowState QXlibWindow::setWindowState(Qt::WindowState state)
{
    // ####
    return state;
}

void QXlibWindow::setVisible(bool visible)
{
#ifdef MYX11_DEBUG
    qDebug() << "QTestLiteWindow::setVisible" << visible << hex << x_window;
#endif
    if (isTransient(window())) {
        Window parentXWindow = x_window;
        QWindow *parent = window()->parent();
        if (parent && parent->handle()) {
            QXlibWindow *xlibParent = static_cast<QXlibWindow *>(parent->handle());
            parentXWindow = xlibParent->x_window;
        }
        XSetTransientForHint(mScreen->display()->nativeDisplay(),x_window,parentXWindow);
    }

    if (visible) {
        qDebug() << ">>> mapping";
        //ensure that the window is viewed in correct position.
        doSizeHints();
        XMapWindow(mScreen->display()->nativeDisplay(), x_window);
    } else {
        XUnmapWindow(mScreen->display()->nativeDisplay(), x_window);
    }
}

void QXlibWindow::setCursor(const Cursor &cursor)
{
    XDefineCursor(mScreen->display()->nativeDisplay(), x_window, cursor);
    mScreen->display()->flush();
}

QSurfaceFormat QXlibWindow::format() const
{
    return mSurfaceFormat;
}


Window QXlibWindow::xWindow() const
{
    return x_window;
}

GC QXlibWindow::graphicsContext() const
{
    return gc;
}

void QXlibWindow::doSizeHints()
{
//    Q_ASSERT(window()->testAttribute(Qt::WA_WState_Created));
    XSizeHints s;
    s.flags = 0;
    QRect g = geometry();
    s.x = g.x();
    s.y = g.y();
    s.width = g.width();
    s.height = g.height();
    s.flags |= USPosition;
    s.flags |= PPosition;
    s.flags |= USSize;
    s.flags |= PSize;
    s.flags |= PWinGravity;
    s.win_gravity = QGuiApplication::isRightToLeft() ? NorthEastGravity : NorthWestGravity;
    XSetWMNormalHints(mScreen->display()->nativeDisplay(), x_window, &s);
}


QXlibWindow *QXlibWindow::platformWindowForXWindow(Window window)
{
    return windowMap.value(window);
}

QT_END_NAMESPACE
