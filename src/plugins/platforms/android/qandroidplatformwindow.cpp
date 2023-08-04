// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformwindow.h"
#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformscreen.h"

#include "androidjnimain.h"

#include <qguiapplication.h>
#include <qpa/qwindowsysteminterface.h>
#include <private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

Q_CONSTINIT static QBasicAtomicInt winIdGenerator = Q_BASIC_ATOMIC_INITIALIZER(0);

QAndroidPlatformWindow::QAndroidPlatformWindow(QWindow *window)
    : QPlatformWindow(window)
{
    m_windowFlags = Qt::Widget;
    m_windowState = Qt::WindowNoState;
    m_windowId = winIdGenerator.fetchAndAddRelaxed(1) + 1;
    setWindowState(window->windowStates());

    // the following is in relation to the virtual geometry
    const bool forceMaximize = m_windowState & (Qt::WindowMaximized | Qt::WindowFullScreen);
    const QRect requestedNativeGeometry =
            forceMaximize ? QRect() : QHighDpi::toNativePixels(window->geometry(), window);
    const QRect availableDeviceIndependentGeometry = (window->parent())
            ? window->parent()->geometry()
            : QHighDpi::fromNativePixels(platformScreen()->availableGeometry(), window);

    // initialGeometry returns in native pixels
    const QRect finalNativeGeometry = QPlatformWindow::initialGeometry(
            window, requestedNativeGeometry, availableDeviceIndependentGeometry.width(),
            availableDeviceIndependentGeometry.height());

    if (requestedNativeGeometry != finalNativeGeometry)
        setGeometry(finalNativeGeometry);
}

void QAndroidPlatformWindow::lower()
{
    platformScreen()->lower(this);
}

void QAndroidPlatformWindow::raise()
{
    updateSystemUiVisibility();
    platformScreen()->raise(this);
}

QMargins QAndroidPlatformWindow::safeAreaMargins() const
{
    if ((m_windowState & Qt::WindowMaximized) && (window()->flags() & Qt::MaximizeUsingFullscreenGeometryHint)) {
        QRect availableGeometry = platformScreen()->availableGeometry();
        return QMargins(availableGeometry.left(), availableGeometry.top(),
                        availableGeometry.right(), availableGeometry.bottom());
    } else {
        return QPlatformWindow::safeAreaMargins();
    }
}

void QAndroidPlatformWindow::setGeometry(const QRect &rect)
{
    QPlatformWindow::setGeometry(rect);
    QWindowSystemInterface::handleGeometryChange(window(), rect);
}

void QAndroidPlatformWindow::setVisible(bool visible)
{
    if (visible)
        updateSystemUiVisibility();

    if (visible) {
        if ((m_windowState & Qt::WindowFullScreen)
                || ((m_windowState & Qt::WindowMaximized) && (window()->flags() & Qt::MaximizeUsingFullscreenGeometryHint))) {
            setGeometry(platformScreen()->geometry());
        } else if (m_windowState & Qt::WindowMaximized) {
            setGeometry(platformScreen()->availableGeometry());
        }
    }

    if (visible)
        platformScreen()->addWindow(this);
    else
        platformScreen()->removeWindow(this);

    QRect availableGeometry = screen()->availableGeometry();
    if (geometry().width() > 0 && geometry().height() > 0 && availableGeometry.width() > 0 && availableGeometry.height() > 0)
        QPlatformWindow::setVisible(visible);
}

void QAndroidPlatformWindow::setWindowState(Qt::WindowStates state)
{
    if (m_windowState == state)
        return;

    QPlatformWindow::setWindowState(state);
    m_windowState = state;

    if (window()->isVisible())
        updateSystemUiVisibility();
}

void QAndroidPlatformWindow::setWindowFlags(Qt::WindowFlags flags)
{
    if (m_windowFlags == flags)
        return;

    m_windowFlags = flags;
}

Qt::WindowFlags QAndroidPlatformWindow::windowFlags() const
{
    return m_windowFlags;
}

void QAndroidPlatformWindow::setParent(const QPlatformWindow *window)
{
    Q_UNUSED(window);
}

QAndroidPlatformScreen *QAndroidPlatformWindow::platformScreen() const
{
    return static_cast<QAndroidPlatformScreen *>(window()->screen()->handle());
}

void QAndroidPlatformWindow::propagateSizeHints()
{
    //shut up warning from default implementation
}

void QAndroidPlatformWindow::requestActivateWindow()
{
    platformScreen()->topWindowChanged(window());
}

void QAndroidPlatformWindow::updateSystemUiVisibility()
{
    Qt::WindowFlags flags = window()->flags();
    bool isNonRegularWindow = flags & (Qt::Popup | Qt::Dialog | Qt::Sheet) & ~Qt::Window;
    if (!isNonRegularWindow) {
        if (m_windowState & Qt::WindowFullScreen)
            QtAndroid::setSystemUiVisibility(QtAndroid::SYSTEM_UI_VISIBILITY_FULLSCREEN);
        else if (flags & Qt::MaximizeUsingFullscreenGeometryHint)
            QtAndroid::setSystemUiVisibility(QtAndroid::SYSTEM_UI_VISIBILITY_TRANSLUCENT);
        else
            QtAndroid::setSystemUiVisibility(QtAndroid::SYSTEM_UI_VISIBILITY_NORMAL);
    }
}

bool QAndroidPlatformWindow::isExposed() const
{
    return qApp->applicationState() > Qt::ApplicationHidden
            && window()->isVisible()
            && !window()->geometry().isEmpty();
}

void QAndroidPlatformWindow::applicationStateChanged(Qt::ApplicationState)
{
    QRegion region;
    if (isExposed())
        region = QRect(QPoint(), geometry().size());

    QWindowSystemInterface::handleExposeEvent(window(), region);
    QWindowSystemInterface::flushWindowSystemEvents();
}

QT_END_NAMESPACE
