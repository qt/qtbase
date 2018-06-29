/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qpa/qwindowsysteminterface.h>
#include <private/qguiapplication_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/QOpenGLContext>

#include "qwasmwindow.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"
#include "qwasmeventdispatcher.h"

#include <QDebug>

#include <iostream>

Q_GUI_EXPORT int qt_defaultDpiX();

QT_BEGIN_NAMESPACE

QWasmWindow::QWasmWindow(QWindow *w, QWasmCompositor* compositor, QWasmBackingStore *backingStore)
    : QPlatformWindow(w),
      mWindow(w),
      mCompositor(compositor),
      mBackingStore(backingStore)
{
    needsCompositor = w->surfaceType() != QSurface::OpenGLSurface;
    static int serialNo = 0;
    m_winid  = ++serialNo;
    qWarning("QWasmWindow %p: %p 0x%x\n", this, w, uint(m_winid));

    mCompositor->addWindow(this);

    // Pure OpenGL windows draw directly using egl, disable the compositor.
    mCompositor->setEnabled(w->surfaceType() != QSurface::OpenGLSurface);
}

QWasmWindow::~QWasmWindow()
{
    mCompositor->removeWindow(this);
}

void QWasmWindow::create()
{
    QRect rect = windowGeometry();

    QPlatformWindow::setGeometry(rect);

    const QSize minimumSize = windowMinimumSize();
    if (rect.width() > 0 || rect.height() > 0) {
        rect.setWidth(qBound(1, rect.width(), 2000));
        rect.setHeight(qBound(1, rect.height(), 2000));
    } else if (minimumSize.width() > 0 || minimumSize.height() > 0) {
        rect.setSize(minimumSize);
    }

    setWindowState(window()->windowStates());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());
    hasTitle = window()->flags().testFlag(Qt::WindowTitleHint) && needsCompositor;

    if (window()->isTopLevel())
        setWindowIcon(window()->icon());
    mNormalGeometry = rect;
}

QWasmScreen *QWasmWindow::platformScreen() const
{
    return static_cast<QWasmScreen *>(window()->screen()->handle());
}

void QWasmWindow::setGeometry(const QRect &rect)
{
    QRect r = rect;
    if (needsCompositor) {
        int yMin = window()->geometry().top() - window()->frameGeometry().top();

        if (r.y() < yMin)
            r.moveTop(yMin);
    }
    QWindowSystemInterface::handleGeometryChange(window(), r);
    QPlatformWindow::setGeometry(r);

    QWindowSystemInterface::flushWindowSystemEvents();
    invalidate();
}

void QWasmWindow::setVisible(bool visible)
{
    QRect newGeom;

    if (visible) {
        bool convOk = false;
        static bool envDisableForceFullScreen = qEnvironmentVariableIntValue("QT_QPA_WEBASSEMBLY_FORCE_FULLSCREEN", &convOk) == 0 && convOk;

        const bool forceFullScreen = /*!envDisableForceFullScreen && */!needsCompositor;//make gl apps fullscreen for now

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

QMargins QWasmWindow::frameMargins() const
{
    int border = hasTitle ? 4. * (qreal(qt_defaultDpiX()) / 96.0) : 0;
    int titleBarHeight = hasTitle ? titleHeight() : 0;

    QMargins margins;
    margins.setLeft(border);
    margins.setRight(border);
    margins.setTop(2*border + titleBarHeight);
    margins.setBottom(border);

    return margins;
}

void QWasmWindow::raise()
{
    mCompositor->raise(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
    invalidate();
}

void QWasmWindow::lower()
{
    mCompositor->lower(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
    invalidate();
}

WId QWasmWindow::winId() const
{
    return m_winid;
}

void QWasmWindow::propagateSizeHints()
{
// get rid of base class warning
}

void QWasmWindow::injectMousePressed(const QPoint &local, const QPoint &global,
                                      Qt::MouseButton button, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);

    if (!hasTitle || button != Qt::LeftButton)
        return;

    if (maxButtonRect().contains(global))
        mActiveControl = QWasmCompositor::SC_TitleBarMaxButton;
    else if (minButtonRect().contains(global))
        mActiveControl = QWasmCompositor::SC_TitleBarMinButton;
    else if (closeButtonRect().contains(global))
        mActiveControl = QWasmCompositor::SC_TitleBarCloseButton;
    else if (normButtonRect().contains(global))
        mActiveControl = QWasmCompositor::SC_TitleBarNormalButton;

    invalidate();
}

void QWasmWindow::injectMouseReleased(const QPoint &local, const QPoint &global,
                                       Qt::MouseButton button, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);

    if (!hasTitle || button != Qt::LeftButton)
        return;

    if (closeButtonRect().contains(global) && mActiveControl == QWasmCompositor::SC_TitleBarCloseButton)
        window()->close();

    if (maxButtonRect().contains(global) && mActiveControl == QWasmCompositor::SC_TitleBarMaxButton) {
        window()->setWindowState(Qt::WindowMaximized);
        platformScreen()->resizeMaximizedWindows();
    }

    if (normButtonRect().contains(global) && mActiveControl == QWasmCompositor::SC_TitleBarNormalButton) {
        window()->setWindowState(Qt::WindowNoState);
        setGeometry(normalGeometry());
    }

    mActiveControl = QWasmCompositor::SC_None;

    invalidate();
}

int QWasmWindow::titleHeight() const
{
    return 18. * (qreal(qt_defaultDpiX()) / 96.0);//dpiScaled(18.);
}

int QWasmWindow::borderWidth() const
{
    return  4. * (qreal(qt_defaultDpiX()) / 96.0);// dpiScaled(4.);
}

QRegion QWasmWindow::titleGeometry() const
{
    int border = borderWidth();

    QRegion result(window()->frameGeometry().x() + border,
                   window()->frameGeometry().y() + border,
                   window()->frameGeometry().width() - 2*border,
                   titleHeight());

    result -= titleControlRegion();

    return result;
}

QRegion QWasmWindow::resizeRegion() const
{
    int border = borderWidth();
    QRegion result(window()->frameGeometry().adjusted(-border, -border, border, border));
    result -= window()->frameGeometry().adjusted(border, border, -border, -border);

    return result;
}

bool QWasmWindow::isPointOnTitle(QPoint point) const
{
    bool ok = titleGeometry().contains(point);
    return ok;
}

bool QWasmWindow::isPointOnResizeRegion(QPoint point) const
{
    return resizeRegion().contains(point);
}

QWasmWindow::ResizeMode QWasmWindow::resizeModeAtPoint(QPoint point) const
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

QRect getSubControlRect(const QWasmWindow *window, QWasmCompositor::SubControls subControl)
{
   QWasmCompositor::QWasmTitleBarOptions options = QWasmCompositor::makeTitleBarOptions(window);

   QRect r = QWasmCompositor::titlebarRect(options, subControl);
   r.translate(window->window()->frameGeometry().x(), window->window()->frameGeometry().y());

   return r;
}

QRect QWasmWindow::maxButtonRect() const
{
    return getSubControlRect(this, QWasmCompositor::SC_TitleBarMaxButton);
}

QRect QWasmWindow::minButtonRect() const
{
    return getSubControlRect(this, QWasmCompositor::SC_TitleBarMinButton);
}

QRect QWasmWindow::closeButtonRect() const
{
    return getSubControlRect(this, QWasmCompositor::SC_TitleBarCloseButton);
}

QRect QWasmWindow::normButtonRect() const
{
    return getSubControlRect(this, QWasmCompositor::SC_TitleBarNormalButton);
}

QRect QWasmWindow::sysMenuRect() const
{
    return getSubControlRect(this, QWasmCompositor::SC_TitleBarSysMenu);
}

QRegion QWasmWindow::titleControlRegion() const
{
    QRegion result;
    result += closeButtonRect();
    result += minButtonRect();
    result += maxButtonRect();
    result += sysMenuRect();

    return result;
}

void QWasmWindow::invalidate()
{
    mCompositor->requestRedraw();
}

QWasmCompositor::SubControls QWasmWindow::activeSubControl() const
{
    return mActiveControl;
}

void QWasmWindow::setWindowState(Qt::WindowStates states)
{
    mWindowState = Qt::WindowNoState;
    if (states & Qt::WindowMinimized)
        mWindowState = Qt::WindowMinimized;
    else if (states & Qt::WindowFullScreen)
        mWindowState = Qt::WindowFullScreen;
    else if (states & Qt::WindowMaximized)
        mWindowState = Qt::WindowMaximized;
}

QRect QWasmWindow::normalGeometry() const
{
    return mNormalGeometry;
}

qreal QWasmWindow::devicePixelRatio() const
{
    return screen()->devicePixelRatio();
}

void QWasmWindow::requestUpdate()
{
    QPointer<QWindow> windowPointer(window());
    bool registered = QWasmEventDispatcher::registerRequestUpdateCallback([=](){
        if (windowPointer.isNull())
            return;

        QWindowPrivate *wp = static_cast<QWindowPrivate *>(QObjectPrivate::get(windowPointer));
        wp->deliverUpdateRequest();
    });

    if (!registered)
        QPlatformWindow::requestUpdate();
}

QT_END_NAMESPACE
