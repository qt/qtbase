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
#include <QtGui/qopenglcontext.h>

#include "qwasmwindow.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"
#include "qwasmeventdispatcher.h"

#include <iostream>

Q_GUI_EXPORT int qt_defaultDpiX();

QT_BEGIN_NAMESPACE

QWasmWindow::QWasmWindow(QWindow *w, QWasmCompositor *compositor, QWasmBackingStore *backingStore)
    : QPlatformWindow(w),
      m_window(w),
      m_compositor(compositor),
      m_backingStore(backingStore)
{
    m_needsCompositor = w->surfaceType() != QSurface::OpenGLSurface;
    static int serialNo = 0;
    m_winid = ++serialNo;

    m_compositor->addWindow(this);

    // Pure OpenGL windows draw directly using egl, disable the compositor.
    m_compositor->setEnabled(w->surfaceType() != QSurface::OpenGLSurface);
}

QWasmWindow::~QWasmWindow()
{
    m_compositor->removeWindow(this);
    if (m_requestAnimationFrameId > -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);
}

void QWasmWindow::destroy()
{
    if (m_backingStore)
        m_backingStore->destroy();
}

void QWasmWindow::initialize()
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
    if (window()->isTopLevel())
        setWindowIcon(window()->icon());
    m_normalGeometry = rect;
}

QWasmScreen *QWasmWindow::platformScreen() const
{
    return static_cast<QWasmScreen *>(window()->screen()->handle());
}

void QWasmWindow::setGeometry(const QRect &rect)
{
    QRect r = rect;
    if (m_needsCompositor) {
        int yMin = window()->geometry().top() - window()->frameGeometry().top();

        if (r.y() < yMin)
            r.moveTop(yMin);
    }
    QPlatformWindow::setGeometry(r);
    QWindowSystemInterface::handleGeometryChange(window(), r);
    invalidate();
}

void QWasmWindow::setVisible(bool visible)
{
    if (visible)
        applyWindowState();
    m_compositor->setVisible(this, visible);
}

bool QWasmWindow::isVisible()
{
    return window()->isVisible();
}

QMargins QWasmWindow::frameMargins() const
{
    int border = hasTitleBar() ? 4. * (qreal(qt_defaultDpiX()) / 96.0) : 0;
    int titleBarHeight = hasTitleBar() ? titleHeight() : 0;

    QMargins margins;
    margins.setLeft(border);
    margins.setRight(border);
    margins.setTop(2*border + titleBarHeight);
    margins.setBottom(border);

    return margins;
}

void QWasmWindow::raise()
{
    m_compositor->raise(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
    invalidate();
}

void QWasmWindow::lower()
{
    m_compositor->lower(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
    invalidate();
}

WId QWasmWindow::winId() const
{
    return m_winid;
}

void QWasmWindow::propagateSizeHints()
{
    QRect rect = windowGeometry();
    if (rect.size().width() < windowMinimumSize().width()
        && rect.size().height() < windowMinimumSize().height()) {
        rect.setSize(windowMinimumSize());
        setGeometry(rect);
    }
}

void QWasmWindow::injectMousePressed(const QPoint &local, const QPoint &global,
                                      Qt::MouseButton button, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);

    if (!hasTitleBar() || button != Qt::LeftButton)
        return;

    if (maxButtonRect().contains(global))
        m_activeControl = QWasmCompositor::SC_TitleBarMaxButton;
    else if (minButtonRect().contains(global))
        m_activeControl = QWasmCompositor::SC_TitleBarMinButton;
    else if (closeButtonRect().contains(global))
        m_activeControl = QWasmCompositor::SC_TitleBarCloseButton;
    else if (normButtonRect().contains(global))
        m_activeControl = QWasmCompositor::SC_TitleBarNormalButton;

    invalidate();
}

void QWasmWindow::injectMouseReleased(const QPoint &local, const QPoint &global,
                                       Qt::MouseButton button, Qt::KeyboardModifiers mods)
{
    Q_UNUSED(local);
    Q_UNUSED(mods);

    if (!hasTitleBar() || button != Qt::LeftButton)
        return;

    if (closeButtonRect().contains(global) && m_activeControl == QWasmCompositor::SC_TitleBarCloseButton) {
        window()->close();
        return;
    }

    if (maxButtonRect().contains(global) && m_activeControl == QWasmCompositor::SC_TitleBarMaxButton) {
        window()->setWindowState(Qt::WindowMaximized);
    }

    if (normButtonRect().contains(global) && m_activeControl == QWasmCompositor::SC_TitleBarNormalButton) {
        window()->setWindowState(Qt::WindowNoState);
    }

    m_activeControl = QWasmCompositor::SC_None;

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
    if (window()->flags().testFlag(Qt::Popup))
        return false;
    return resizeRegion().contains(point);
}

QWasmCompositor::ResizeMode QWasmWindow::resizeModeAtPoint(QPoint point) const
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
            return QWasmCompositor::ResizeTopLeft;
        if (center.contains(point))
            return QWasmCompositor::ResizeTop;
        if (right.contains(point))
            return QWasmCompositor::ResizeTopRight;
    } else if (middle.contains(point)) {
        // Middle
        if (left.contains(point))
            return QWasmCompositor::ResizeLeft;
        if (right.contains(point))
            return QWasmCompositor::ResizeRight;
    } else if (bottom.contains(point)) {
        // Bottom
        if (left.contains(point))
            return QWasmCompositor::ResizeBottomLeft;
        if (center.contains(point))
            return QWasmCompositor::ResizeBottom;
        if (right.contains(point))
            return QWasmCompositor::ResizeBottomRight;
    }

    return QWasmCompositor::ResizeNone;
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
    m_compositor->requestUpdateWindow(this);
}

QWasmCompositor::SubControls QWasmWindow::activeSubControl() const
{
    return m_activeControl;
}

void QWasmWindow::setWindowState(Qt::WindowStates newState)
{
    const Qt::WindowStates oldState = m_windowState;
    bool isActive = oldState.testFlag(Qt::WindowActive);

    if (newState.testFlag(Qt::WindowMinimized)) {
        newState.setFlag(Qt::WindowMinimized, false);
        qWarning("Qt::WindowMinimized is not implemented in wasm");
    }

    // Always keep OpenGL apps fullscreen
    if (!m_needsCompositor && !newState.testFlag(Qt::WindowFullScreen)) {
        newState.setFlag(Qt::WindowFullScreen, true);
        qWarning("Qt::WindowFullScreen must be set for OpenGL surfaces");
    }

    // Ignore WindowActive flag in comparison, as we want to preserve it either way
    if ((newState & ~Qt::WindowActive) == (oldState & ~Qt::WindowActive))
        return;

    newState.setFlag(Qt::WindowActive, isActive);

    m_previousWindowState = oldState;
    m_windowState = newState;

    if (isVisible()) {
        applyWindowState();
    }
}

void QWasmWindow::applyWindowState()
{
    QRect newGeom;

    if (m_windowState.testFlag(Qt::WindowFullScreen))
        newGeom = platformScreen()->geometry();
    else if (m_windowState.testFlag(Qt::WindowMaximized))
        newGeom = platformScreen()->availableGeometry();
    else
        newGeom = normalGeometry();

    QWindowSystemInterface::handleWindowStateChanged(window(), m_windowState, m_previousWindowState);
    setGeometry(newGeom);
}

QRect QWasmWindow::normalGeometry() const
{
    return m_normalGeometry;
}

qreal QWasmWindow::devicePixelRatio() const
{
    return screen()->devicePixelRatio();
}

void QWasmWindow::requestUpdate()
{
    if (m_compositor) {
        m_compositor->requestUpdateWindow(this, QWasmCompositor::UpdateRequestDelivery);
        return;
    }

    static auto frame = [](double time, void *context) -> int {
        Q_UNUSED(time);
        QWasmWindow *window = static_cast<QWasmWindow *>(context);
        window->m_requestAnimationFrameId = -1;
        window->deliverUpdateRequest();
        return 0;
    };
    m_requestAnimationFrameId = emscripten_request_animation_frame(frame, this);
}

bool QWasmWindow::hasTitleBar() const
{
    Qt::WindowFlags flags = window()->flags();
    return !(m_windowState & Qt::WindowFullScreen)
        && flags.testFlag(Qt::WindowTitleHint)
        && !(windowIsPopupType(flags))
        && m_needsCompositor;
}

bool QWasmWindow::windowIsPopupType(Qt::WindowFlags flags) const
{
    if (flags.testFlag(Qt::Tool))
        return false; // Qt::Tool has the Popup bit set but isn't

    return (flags.testFlag(Qt::Popup));
}

void QWasmWindow::requestActivateWindow()
{
    if (window()->isTopLevel())
        raise();
    QPlatformWindow::requestActivateWindow();
}

QT_END_NAMESPACE
