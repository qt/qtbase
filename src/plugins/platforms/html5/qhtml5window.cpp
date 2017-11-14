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

#include <qpa/qwindowsysteminterface.h>
#include <private/qguiapplication_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/QOpenGLContext>

#include "qhtml5window.h"
#include "qhtml5screen.h"
#include "qhtml5compositor.h"

#include <QDebug>

#include <iostream>

//#include "qhtml5compositor.h"
Q_GUI_EXPORT int qt_defaultDpiX();

QT_BEGIN_NAMESPACE

//static QHtml5Window *globalHtml5Window;
//QHtml5Window *QHtml5Window::get() { return globalHtml5Window; }

QHtml5Window::QHtml5Window(QWindow *w, QHtml5Compositor* compositor)
    : QPlatformWindow(w),
      mWindow(w),
      mWindowState(Qt::WindowNoState),
      mCompositor(compositor),
      m_raster(false),
      mActiveControl(QHtml5Compositor::SC_None),
      mNormalGeometry(0,0,0,0)
{
    //globalHtml5Window = this;
    static int serialNo = 0;
    m_winid  = ++serialNo;
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglWindow %p: %p 0x%x\n", this, w, uint(m_winid));
#endif

    // Save surface type which may be changed by the QHTML5BackingStore constructor
    QWindow::SurfaceType surfaceType = w->surfaceType();

    mCompositor->addWindow(this);

    // Pure OpenGL windows draw directly using egl, disable the compositor.
    mCompositor->setEnabled(surfaceType != QSurface::OpenGLSurface);
}

QHtml5Window::~QHtml5Window()
{
    mCompositor->removeWindow(this);
}

void QHtml5Window::create()
{
    QRect rect = windowGeometry();

    QPlatformWindow::setGeometry(rect);

    const QSize minimumSize = windowMinimumSize();
    if (rect.width() > 0 || rect.height() > 0) {
        rect.setWidth(qBound(1, rect.width(), 2000));
        rect.setHeight(qBound(1, rect.height(), 2000));
    } else if (minimumSize.width() > 0 || minimumSize.height() > 0) {
        rect.setSize(minimumSize);
    } else {
        /*
        rect.setWidth(QHighDpi::toNativePixels(int(defaultWindowWidth), platformScreen->QPlatformScreen::screen()));
        rect.setHeight(QHighDpi::toNativePixels(int(defaultWindowHeight), platformScreen->QPlatformScreen::screen()));
        */
    }

    setWindowState(window()->windowStates());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());

    if (window()->isTopLevel())
        setWindowIcon(window()->icon());
    mNormalGeometry = rect;
}

QHTML5Screen *QHtml5Window::platformScreen() const
{
    return static_cast<QHTML5Screen *>(window()->screen()->handle());
}

void QHtml5Window::setGeometry(const QRect &rect)
{
    QRect r = rect;

    int yMin = window()->geometry().top() - window()->frameGeometry().top();

    if (r.y() < yMin)
        r.moveTop(yMin);

    QWindowSystemInterface::handleGeometryChange(window(), r);
    QPlatformWindow::setGeometry(r);

    QWindowSystemInterface::flushWindowSystemEvents();
    invalidate();
}

void QHtml5Window::setVisible(bool visible)
{
    QRect newGeom;

    if (visible) {
        bool convOk = false;
        static bool envDisableForceFullScreen = qEnvironmentVariableIntValue("QT_QPA_HTML5_FORCE_FULLSCREEN", &convOk) == 0 && convOk;

        const bool forceFullScreen = !envDisableForceFullScreen && mCompositor->windowCount() == 0;

        if (forceFullScreen || (mWindowState & Qt::WindowFullScreen))
            newGeom = platformScreen()->geometry();
        else if (mWindowState & Qt::WindowMaximized)
            newGeom = platformScreen()->availableGeometry();
    }
    QPlatformWindow::setVisible(visible);

    mCompositor->setVisible(this, visible);

    if (!newGeom.isEmpty())
        setGeometry(newGeom); // may or may not generate an expose

    QWindowSystemInterface::handleExposeEvent(window(), visible ? QRect(QPoint(), geometry().size()) : QRect());
    QWindowSystemInterface::flushWindowSystemEvents();
    invalidate();
}

QMargins QHtml5Window::frameMargins() const
{
    bool hasTitle = window()->flags().testFlag(Qt::WindowTitleHint);

    int border = hasTitle ? 4. * (qreal(qt_defaultDpiX()) / 96.0) : 0;
    int titleBarHeight = hasTitle ? titleHeight() : 0;

    QMargins margins;
    margins.setLeft(border);
    margins.setRight(border);
    margins.setTop(2*border + titleBarHeight);
    margins.setBottom(border);

    return margins;
}

void QHtml5Window::raise()
{
    mCompositor->raise(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
    invalidate();
}

void QHtml5Window::lower()
{
    mCompositor->lower(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
    invalidate();
}

WId QHtml5Window::winId() const
{
    return m_winid;
}

void QHtml5Window::propagateSizeHints()
{
// get rid of base class warning
}

void QHtml5Window::injectMousePressed(const QPoint &local, const QPoint &global,
                                      Qt::MouseButton button, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);

    if (button != Qt::LeftButton)
        return;

    if (maxButtonRect().contains(global))
        mActiveControl = QHtml5Compositor::SC_TitleBarMaxButton;
    else if (minButtonRect().contains(global))
        mActiveControl = QHtml5Compositor::SC_TitleBarMinButton;
    else if (closeButtonRect().contains(global))
        mActiveControl = QHtml5Compositor::SC_TitleBarCloseButton;
    else if (normButtonRect().contains(global))
        mActiveControl = QHtml5Compositor::SC_TitleBarNormalButton;

    invalidate();
}

void QHtml5Window::injectMouseReleased(const QPoint &local, const QPoint &global,
                                       Qt::MouseButton button, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);

    if (button != Qt::LeftButton)
        return;

    if (closeButtonRect().contains(global) && mActiveControl == QHtml5Compositor::SC_TitleBarCloseButton)
        window()->close();

    if (maxButtonRect().contains(global) && mActiveControl == QHtml5Compositor::SC_TitleBarMaxButton) {
        window()->setWindowState(Qt::WindowMaximized);
        platformScreen()->resizeMaximizedWindows();
    }

    if (normButtonRect().contains(global) && mActiveControl == QHtml5Compositor::SC_TitleBarNormalButton) {
        window()->setWindowState(Qt::WindowNoState);
        setGeometry(normalGeometry());
    }

    mActiveControl = QHtml5Compositor::SC_None;

    invalidate();
}

int QHtml5Window::titleHeight() const
{
    return 18. * (qreal(qt_defaultDpiX()) / 96.0);//dpiScaled(18.);
}

int QHtml5Window::borderWidth() const
{
    return  4. * (qreal(qt_defaultDpiX()) / 96.0);// dpiScaled(4.);
}

QRegion QHtml5Window::titleGeometry() const
{
    int border = borderWidth();

    QRegion result(window()->frameGeometry().x() + border,
                   window()->frameGeometry().y() + border,
                   window()->frameGeometry().width() - 2*border,
                   titleHeight());

    result -= titleControlRegion();

    return result;
}

QRegion QHtml5Window::resizeRegion() const
{
    int border = borderWidth();
    QRegion result(window()->frameGeometry().adjusted(-border, -border, border, border));
    result -= window()->frameGeometry().adjusted(border, border, -border, -border);

    return result;
}

bool QHtml5Window::isPointOnTitle(QPoint point) const
{
    bool ok = titleGeometry().contains(point);
    return ok;
}

bool QHtml5Window::isPointOnResizeRegion(QPoint point) const
{
    return resizeRegion().contains(point);
}

QHtml5Window::ResizeMode QHtml5Window::resizeModeAtPoint(QPoint point) const
{
    QPoint p1 = window()->frameGeometry().topLeft() - QPoint(5, 5);
    QPoint p2 = window()->frameGeometry().bottomRight() + QPoint(5, 5);
    int corner = 20;

    QRect top(p1, QPoint(p2.x(), p1.y() + corner));
    QRect middle(QPoint(p1.x(), p1.y() + corner), QPoint(p2.x(), p2.y() - corner));
    QRect bottom(QPoint(p1.x(), p2.y() - corner), p2);

    QRect left(p1, QPoint(p1.x() + corner, p2.y()));
    QRect center(QPoint(p1.x() + corner, p1.y()), QPoint(p2.x() - corner, p2.y()));
    QRect right(QPoint(p2.x() - corner, p1.y()), p2);

    if (top.contains(point)) {
        // Top
        if (left.contains(point))
            return ResizeTopLeft;
        if (center.contains(point))
            return ResizeTop;
        if (right.contains(point))
            return ResizeTopRight;
    } else if (middle.contains(point)) {
        // Middle
        if (left.contains(point))
            return ResizeLeft;
        if (right.contains(point))
            return ResizeRight;
    } else if (bottom.contains(point)) {
        // Bottom
        if (left.contains(point))
            return ResizeBottomLeft;
        if (center.contains(point))
            return ResizeBottom;
        if (right.contains(point))
            return ResizeBottomRight;
    }

    return ResizeNone;
}

QRect getSubControlRect(const QHtml5Window *window, QHtml5Compositor::SubControls subControl)
{
   QHtml5Compositor::QHtml5TitleBarOptions options = QHtml5Compositor::makeTitleBarOptions(window);

   QRect r = QHtml5Compositor::titlebarRect(options, subControl);
   r.translate(window->window()->frameGeometry().x(), window->window()->frameGeometry().y());

   return r;
}

QRect QHtml5Window::maxButtonRect() const
{
    return getSubControlRect(this, QHtml5Compositor::SC_TitleBarMaxButton);
}

QRect QHtml5Window::minButtonRect() const
{
    return getSubControlRect(this, QHtml5Compositor::SC_TitleBarMinButton);
}

QRect QHtml5Window::closeButtonRect() const
{
    return getSubControlRect(this, QHtml5Compositor::SC_TitleBarCloseButton);
}

QRect QHtml5Window::normButtonRect() const
{
    return getSubControlRect(this, QHtml5Compositor::SC_TitleBarNormalButton);
}

QRect QHtml5Window::sysMenuRect() const
{
    return getSubControlRect(this, QHtml5Compositor::SC_TitleBarSysMenu);
}

QRegion QHtml5Window::titleControlRegion() const
{
    QRegion result;
    result += closeButtonRect();
    result += minButtonRect();
    result += maxButtonRect();
    result += sysMenuRect();

    return result;
}

void QHtml5Window::invalidate()
{
    mCompositor->requestRedraw();
}

QHtml5Compositor::SubControls QHtml5Window::activeSubControl() const
{
    return mActiveControl;
}

void QHtml5Window::setWindowState(Qt::WindowStates states)
{
    mWindowState = Qt::WindowNoState;
    if (states & Qt::WindowMinimized)
        mWindowState = Qt::WindowMinimized;
#warning FIXME ShowFullScreen
    else if (states & Qt::WindowFullScreen) // someone sets this initially as default
        mWindowState = Qt::WindowFullScreen;
    else if (states & Qt::WindowMaximized)
        mWindowState = Qt::WindowMaximized;
// update?
}

QRect QHtml5Window::normalGeometry() const
{
    return mNormalGeometry;
}
QT_END_NAMESPACE
