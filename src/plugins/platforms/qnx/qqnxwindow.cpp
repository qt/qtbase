/***************************************************************************
**
** Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qqnxwindow.h"
#include "qqnxintegration.h"
#include "qqnxscreen.h"

#include <QUuid>

#include <QtGui/QWindow>
#include <qpa/qwindowsysteminterface.h>

#include "private/qguiapplication_p.h"

#include <QtCore/QDebug>

#if defined(Q_OS_BLACKBERRY)
#if !defined(Q_OS_BLACKBERRY_TABLET)
#include "qqnxnavigatorcover.h"
#endif
#include <sys/pps.h>
#include <bps/navigator.h>
#endif

#include <errno.h>

#if defined(QQNXWINDOW_DEBUG)
#define qWindowDebug qDebug
#else
#define qWindowDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxWindow::QQnxWindow(QWindow *window, screen_context_t context, bool needRootWindow)
    : QPlatformWindow(window),
      m_screenContext(context),
      m_parentWindow(0),
      m_window(0),
      m_screen(0),
      m_visible(false),
      m_exposed(true),
      m_windowState(Qt::WindowNoState),
      m_mmRendererWindow(0)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window << ", size =" << window->size();
    int result;

    QQnxScreen *platformScreen = static_cast<QQnxScreen *>(window->screen()->handle());

    m_isTopLevel = ( needRootWindow && !platformScreen->rootWindow())
                || (!needRootWindow && !parent())
                || window->type() == Qt::CoverWindow;

    errno = 0;
    if (m_isTopLevel) {
        result = screen_create_window(&m_window, m_screenContext); // Creates an application window
        if (window->type() != Qt::CoverWindow) {
            if (needRootWindow)
                platformScreen->setRootWindow(this);
            createWindowGroup();
        }
    } else {
        result = screen_create_window_type(&m_window, m_screenContext, SCREEN_CHILD_WINDOW);
    }
    if (result != 0)
        qFatal("QQnxWindow: failed to create window, errno=%d", errno);
}

QQnxWindow::~QQnxWindow()
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();

    // Qt should have already deleted the children before deleting the parent.
    Q_ASSERT(m_childWindows.size() == 0);

    // Remove from plugin's window mapper
    QQnxIntegration::removeWindow(m_window);

    // Remove from parent's Hierarchy.
    removeFromParent();
    if (m_screen)
        m_screen->updateHierarchy();

    // Cleanup QNX window and its buffers
    screen_destroy_window(m_window);
}

void QQnxWindow::setGeometry(const QRect &rect)
{
    QRect newGeometry = rect;
    if (screen()->rootWindow() == this) //If this is the root window, it has to be shown fullscreen
        newGeometry = screen()->geometry();

    const QRect oldGeometry = setGeometryHelper(newGeometry);

    // Send a geometry change event to Qt (triggers resizeEvent() in QWindow/QWidget).

    // Calling flushWindowSystemEvents() here would flush input events which
    // could result in re-entering QQnxWindow::setGeometry() again.
    QWindowSystemInterface::setSynchronousWindowsSystemEvents(true);
    QWindowSystemInterface::handleGeometryChange(window(), newGeometry);
    QWindowSystemInterface::handleExposeEvent(window(), newGeometry);
    QWindowSystemInterface::setSynchronousWindowsSystemEvents(false);

    // Now move all children.
    if (!oldGeometry.isEmpty()) {
        const QPoint offset = newGeometry.topLeft() - oldGeometry.topLeft();
        Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
            childWindow->setOffset(offset);
    }
}

QRect QQnxWindow::setGeometryHelper(const QRect &rect)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window()
                   << ", (" << rect.x() << "," << rect.y()
                   << "," << rect.width() << "," << rect.height() << ")";

    // Call base class method
    QRect oldGeometry = QPlatformWindow::geometry();
    QPlatformWindow::setGeometry(rect);

    // Set window geometry equal to widget geometry
    errno = 0;
    int val[2];
    val[0] = rect.x();
    val[1] = rect.y();
    int result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_POSITION, val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window position, errno=%d", errno);

    errno = 0;
    val[0] = rect.width();
    val[1] = rect.height();
    result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SIZE, val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window size, errno=%d", errno);

    // Set viewport size equal to window size
    errno = 0;
    result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SOURCE_SIZE, val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window source size, errno=%d", errno);

    return oldGeometry;
}

void QQnxWindow::setOffset(const QPoint &offset)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();
    // Move self and then children.
    QRect newGeometry = geometry();
    newGeometry.translate(offset);

    // Call the base class
    QPlatformWindow::setGeometry(newGeometry);

    int val[2];

    errno = 0;
    val[0] = newGeometry.x();
    val[1] = newGeometry.y();
    int result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_POSITION, val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window position, errno=%d", errno);

    Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
        childWindow->setOffset(offset);
}

void QQnxWindow::setVisible(bool visible)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window() << "visible =" << visible;

    if (m_visible == visible)
        return;

    m_visible = visible;

    QQnxWindow *root = this;
    while (root->m_parentWindow)
        root = root->m_parentWindow;

    root->updateVisibility(root->m_visible);

    window()->requestActivate();

    QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());

    if (visible) {
        applyWindowState();
    } else {
        // Flush the context, otherwise it won't disappear immediately
        screen_flush_context(m_screenContext, 0);
    }
}

void QQnxWindow::updateVisibility(bool parentVisible)
{
    qWindowDebug() << Q_FUNC_INFO << "parentVisible =" << parentVisible << "window =" << window();
    // Set window visibility
    errno = 0;
    int val = (m_visible && parentVisible) ? 1 : 0;
    int result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_VISIBLE, &val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window visibility, errno=%d", errno);

    Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
        childWindow->updateVisibility(m_visible && parentVisible);
}

void QQnxWindow::setOpacity(qreal level)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window() << "opacity =" << level;
    // Set window global alpha
    errno = 0;
    int val = (int)(level * 255);
    int result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_GLOBAL_ALPHA, &val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window global alpha, errno=%d", errno);

    // TODO: How to handle children of this window? If we change all the visibilities, then
    //       the transparency will look wrong...
}

void QQnxWindow::setExposed(bool exposed)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window() << "expose =" << exposed;

    if (m_exposed != exposed) {
        m_exposed = exposed;
        QWindowSystemInterface::handleExposeEvent(window(), window()->geometry());
    }
}

bool QQnxWindow::isExposed() const
{
    return m_visible && m_exposed;
}

void QQnxWindow::setBufferSize(const QSize &size)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window() << "size =" << size;

    // Set window buffer size
    errno = 0;

    // libscreen fails when creating empty buffers
    const QSize nonEmptySize = size.isEmpty() ? QSize(1, 1) : size;

    int val[2] = { nonEmptySize.width(), nonEmptySize.height() };
    int result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_BUFFER_SIZE, val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window buffer size, errno=%d", errno);

    // Create window buffers if they do not exist
    if (m_bufferSize.isEmpty()) {
        val[0] = pixelFormat();
        if (val[0] == -1) // The platform GL context was not set yet on the window, so we can't procede
            return;

        errno = 0;
        result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_FORMAT, val);
        if (result != 0)
            qFatal("QQnxWindow: failed to set window pixel format, errno=%d", errno);

        errno = 0;
        result = screen_create_window_buffers(m_window, MAX_BUFFER_COUNT);
        if (result != 0) {
            qWarning() << "QQnxWindow: Buffer size was" << size;
            qFatal("QQnxWindow: failed to create window buffers, errno=%d", errno);
        }

        // check if there are any buffers available
        int bufferCount = 0;
        result = screen_get_window_property_iv(m_window, SCREEN_PROPERTY_RENDER_BUFFER_COUNT, &bufferCount);

        if (result != 0)
            qFatal("QQnxWindow: failed to query window buffer count, errno=%d", errno);

        if (bufferCount != MAX_BUFFER_COUNT) {
            qFatal("QQnxWindow: invalid buffer count. Expected = %d, got = %d. You might experience problems.",
                    MAX_BUFFER_COUNT, bufferCount);
        }
    }

    // Cache new buffer size
    m_bufferSize = nonEmptySize;
    resetBuffers();
}

void QQnxWindow::setScreen(QQnxScreen *platformScreen)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window() << "platformScreen =" << platformScreen;

    if (platformScreen == 0) { // The screen has been destroyed
        m_screen = 0;
        Q_FOREACH (QQnxWindow *childWindow, m_childWindows) {
            childWindow->setScreen(0);
        }
        return;
    }

    if (m_screen == platformScreen)
        return;

    if (m_screen) {
        qWindowDebug() << Q_FUNC_INFO << "Moving window to different screen";
        m_screen->removeWindow(this);
        QQnxIntegration *platformIntegration = static_cast<QQnxIntegration*>(QGuiApplicationPrivate::platformIntegration());

        if ((platformIntegration->options() & QQnxIntegration::RootWindow)) {
            screen_leave_window_group(m_window);
        }
    }

    m_screen = platformScreen;
    if (!m_parentWindow) {
        platformScreen->addWindow(this);
    }
    if (m_isTopLevel) {
          // Move window to proper screen/display
        errno = 0;
        screen_display_t display = platformScreen->nativeDisplay();
        int result = screen_set_window_property_pv(m_window, SCREEN_PROPERTY_DISPLAY, (void **)&display);
        if (result != 0)
            qFatal("QQnxWindow: failed to set window display, errno=%d", errno);
    } else {
        errno = 0;
        int result;
        if (!parent()) {
            result = screen_join_window_group(m_window, platformScreen->windowGroupName());
            if (result != 0)
                qFatal("QQnxWindow: failed to join window group, errno=%d", errno);
        } else {
            result = screen_join_window_group(m_window, static_cast<QQnxWindow*>(parent())->groupName().constData());
            if (result != 0)
                qFatal("QQnxWindow: failed to join window group, errno=%d", errno);
        }

        Q_FOREACH (QQnxWindow *childWindow, m_childWindows) {
            // Only subwindows and tooltips need necessarily be moved to another display with the window.
            if (window()->type() == Qt::SubWindow || window()->type() == Qt::ToolTip)
                childWindow->setScreen(platformScreen);
        }
    }

    m_screen->updateHierarchy();
}

void QQnxWindow::removeFromParent()
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();
    // Remove from old Hierarchy position
    if (m_parentWindow) {
        if (m_parentWindow->m_childWindows.removeAll(this))
            m_parentWindow = 0;
        else
            qFatal("QQnxWindow: Window Hierarchy broken; window has parent, but parent hasn't got child.");
    } else if (m_screen) {
        m_screen->removeWindow(this);
    }
}

void QQnxWindow::setParent(const QPlatformWindow *window)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << this->window() << "platformWindow =" << window;
    // Cast away the const, we need to modify the hierarchy.
    QQnxWindow* const newParent = static_cast<QQnxWindow*>(const_cast<QPlatformWindow*>(window));

    if (newParent == m_parentWindow)
        return;

    removeFromParent();
    m_parentWindow = newParent;

    // Add to new hierarchy position.
    if (m_parentWindow) {
        if (m_parentWindow->m_screen != m_screen)
            setScreen(m_parentWindow->m_screen);

        m_parentWindow->m_childWindows.push_back(this);
    } else {
        m_screen->addWindow(this);
    }

    adjustBufferSize();

    m_screen->updateHierarchy();
}

void QQnxWindow::raise()
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();

    if (m_parentWindow) {
        m_parentWindow->m_childWindows.removeAll(this);
        m_parentWindow->m_childWindows.push_back(this);
    } else {
        m_screen->raiseWindow(this);
    }

    m_screen->updateHierarchy();
}

void QQnxWindow::lower()
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();

    if (m_parentWindow) {
        m_parentWindow->m_childWindows.removeAll(this);
        m_parentWindow->m_childWindows.push_front(this);
    } else {
        m_screen->lowerWindow(this);
    }

    m_screen->updateHierarchy();
}

void QQnxWindow::requestActivateWindow()
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();

    // TODO: Tell screen to set keyboard focus to this window.

    // Notify that we gained focus.
    gainedFocus();
}


void QQnxWindow::setWindowState(Qt::WindowState state)
{
    qWindowDebug() << Q_FUNC_INFO << "state =" << state;

    // Prevent two calls with Qt::WindowFullScreen from changing m_unmaximizedGeometry
    if (m_windowState == state)
        return;

    m_windowState = state;

    if (m_visible)
        applyWindowState();
}

void QQnxWindow::propagateSizeHints()
{
    // nothing to do; silence base class warning
    qWindowDebug() << Q_FUNC_INFO << ": ignored";
}

void QQnxWindow::gainedFocus()
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();

    // Got focus
    QWindowSystemInterface::handleWindowActivated(window());
}

void QQnxWindow::setMMRendererWindowName(const QString &name)
{
    m_mmRendererWindowName = name;
}

void QQnxWindow::setMMRendererWindow(screen_window_t handle)
{
    m_mmRendererWindow = handle;
}

void QQnxWindow::clearMMRendererWindow()
{
    m_mmRendererWindowName.clear();
    m_mmRendererWindow = 0;
}

QQnxWindow *QQnxWindow::findWindow(screen_window_t windowHandle)
{
    if (m_window == windowHandle)
        return this;

    Q_FOREACH (QQnxWindow *window, m_childWindows) {
        QQnxWindow * const result = window->findWindow(windowHandle);
        if (result)
            return result;
    }

    return 0;
}

void QQnxWindow::minimize()
{
#if defined(Q_OS_BLACKBERRY) && !defined(Q_OS_BLACKBERRY_TABLET)
    qWindowDebug() << Q_FUNC_INFO;

    pps_encoder_t encoder;

    pps_encoder_initialize(&encoder, false);
    pps_encoder_add_string(&encoder, "msg", "minimizeWindow");

    if (navigator_raw_write(pps_encoder_buffer(&encoder),
                pps_encoder_length(&encoder)) != BPS_SUCCESS) {
        qWindowDebug() << Q_FUNC_INFO << "navigator_raw_write failed:" << strerror(errno);
    }

    pps_encoder_cleanup(&encoder);
#else
    qWarning("Qt::WindowMinimized is not supported by this OS version");
#endif
}

void QQnxWindow::setRotation(int rotation)
{
    qWindowDebug() << Q_FUNC_INFO << "angle =" << rotation;
    errno = 0;
    int result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_ROTATION, &rotation);
    if (result != 0)
        qFatal("QQnxRootWindow: failed to set window rotation, errno=%d", errno);
}

void QQnxWindow::initWindow()
{
    // Alpha channel is always pre-multiplied if present
    errno = 0;
    int val = SCREEN_PRE_MULTIPLIED_ALPHA;
    int result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_ALPHA_MODE, &val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window alpha mode, errno=%d", errno);

    // Blend the window with Source Over Porter-Duff behavior onto whatever's
    // behind it.
    //
    // If the desired use-case is opaque, the Widget painting framework will
    // already fill in the alpha channel with full opacity.
    errno = 0;
    val = SCREEN_TRANSPARENCY_SOURCE_OVER;
    result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_TRANSPARENCY, &val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window transparency, errno=%d", errno);

    // Set the window swap interval
    errno = 0;
    val = 1;
    result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SWAP_INTERVAL, &val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window swap interval, errno=%d", errno);

    if (window()->flags() & Qt::WindowDoesNotAcceptFocus) {
        errno = 0;
        val = SCREEN_SENSITIVITY_NO_FOCUS;
        result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SENSITIVITY, &val);
        if (result != 0)
            qFatal("QQnxWindow: failed to set window sensitivity, errno=%d", errno);
    }

    setScreen(static_cast<QQnxScreen *>(window()->screen()->handle()));

    if (window()->type() == Qt::CoverWindow) {
#if defined(Q_OS_BLACKBERRY) && !defined(Q_OS_BLACKBERRY_TABLET)
        screen_set_window_property_pv(m_screen->rootWindow()->nativeHandle(),
                                      SCREEN_PROPERTY_ALTERNATE_WINDOW, (void**)&m_window);
        m_cover.reset(new QQnxNavigatorCover);
#endif
        m_exposed = false;
    }

    // Add window to plugin's window mapper
    QQnxIntegration::addWindow(m_window, window());

    // Qt never calls these setters after creating the window, so we need to do that ourselves here
    setWindowState(window()->windowState());
    if (window()->parent() && window()->parent()->handle())
        setParent(window()->parent()->handle());

    if (screen()->rootWindow() == this) {
        setGeometryHelper(screen()->geometry());
        QWindowSystemInterface::handleGeometryChange(window(), screen()->geometry());
    } else {
        setGeometryHelper(window()->geometry());
    }
}

void QQnxWindow::createWindowGroup()
{
    // Generate a random window group name
    m_windowGroupName = QUuid::createUuid().toString().toLatin1();

    // Create window group so child windows can be parented by container window
    errno = 0;
    int result = screen_create_window_group(m_window, m_windowGroupName.constData());
    if (result != 0)
        qFatal("QQnxRootWindow: failed to create app window group, errno=%d", errno);
}

void QQnxWindow::updateZorder(int &topZorder)
{
    updateZorder(m_window, topZorder);

    if (m_mmRendererWindow)
        updateZorder(m_mmRendererWindow, topZorder);

    Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
        childWindow->updateZorder(topZorder);
}

void QQnxWindow::updateZorder(screen_window_t window, int &topZorder)
{
    errno = 0;
    int result = screen_set_window_property_iv(window, SCREEN_PROPERTY_ZORDER, &topZorder);
    topZorder++;

    if (result != 0)
        qFatal("QQnxWindow: failed to set window z-order=%d, errno=%d, mWindow=%p", topZorder, errno, window);
}

void QQnxWindow::applyWindowState()
{
    switch (m_windowState) {

    // WindowActive is not an accepted parameter according to the docs
    case Qt::WindowActive:
        return;

    case Qt::WindowMinimized:
        minimize();

        if (m_unmaximizedGeometry.isValid())
            setGeometry(m_unmaximizedGeometry);
        else
            setGeometry(m_screen->geometry());

        break;

    case Qt::WindowMaximized:
    case Qt::WindowFullScreen:
        m_unmaximizedGeometry = geometry();
        setGeometry(m_windowState == Qt::WindowMaximized ? m_screen->availableGeometry() : m_screen->geometry());
        break;

    case Qt::WindowNoState:
        if (m_unmaximizedGeometry.isValid())
            setGeometry(m_unmaximizedGeometry);
        break;
    }
}


QT_END_NAMESPACE
