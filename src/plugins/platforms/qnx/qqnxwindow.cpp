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

#include "qqnxglobal.h"

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
      m_window(0),
      m_screen(0),
      m_parentWindow(0),
      m_visible(false),
      m_exposed(true),
      m_windowState(Qt::WindowNoState),
      m_mmRendererWindow(0)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window << ", size =" << window->size();

    QQnxScreen *platformScreen = static_cast<QQnxScreen *>(window->screen()->handle());

    if (window->type() == Qt::CoverWindow) {
        // Cover windows have to be top level to be accessible to window delegate (i.e. navigator)
        m_isTopLevel = true;
    } else if (parent() || (window->type() & Qt::Dialog) == Qt::Dialog) {
        // If we have a parent we are a child window.  Sometimes we have to be a child even if we
        // don't have a parent e.g. our parent might be in a different process.
        m_isTopLevel = false;
    } else {
        // We're parentless.  If we're not using a root window, we'll always be a top-level window
        // otherwise only the first window is.
        m_isTopLevel = !needRootWindow || !platformScreen->rootWindow();
    }

    if (m_isTopLevel) {
        Q_SCREEN_CRITICALERROR(screen_create_window(&m_window, m_screenContext),
                            "Could not create top level window"); // Creates an application window
        if (window->type() != Qt::CoverWindow) {
            if (needRootWindow)
                platformScreen->setRootWindow(this);
        }
    } else {
        Q_SCREEN_CHECKERROR(
                screen_create_window_type(&m_window, m_screenContext, SCREEN_CHILD_WINDOW),
                "Could not create child window");
    }

    createWindowGroup();
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

    setGeometryHelper(newGeometry);

    // Send a geometry change event to Qt (triggers resizeEvent() in QWindow/QWidget).

    // Calling flushWindowSystemEvents() here would flush input events which
    // could result in re-entering QQnxWindow::setGeometry() again.
    QWindowSystemInterface::setSynchronousWindowsSystemEvents(true);
    QWindowSystemInterface::handleGeometryChange(window(), newGeometry);
    QWindowSystemInterface::handleExposeEvent(window(), newGeometry);
    QWindowSystemInterface::setSynchronousWindowsSystemEvents(false);
}

void QQnxWindow::setGeometryHelper(const QRect &rect)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window()
                   << ", (" << rect.x() << "," << rect.y()
                   << "," << rect.width() << "," << rect.height() << ")";

    // Call base class method
    QPlatformWindow::setGeometry(rect);

    // Set window geometry equal to widget geometry
    int val[2];
    val[0] = rect.x();
    val[1] = rect.y();
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_POSITION, val),
                        "Failed to set window position");

    val[0] = rect.width();
    val[1] = rect.height();
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SIZE, val),
                        "Failed to set window size");

    // Set viewport size equal to window size
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SOURCE_SIZE, val),
                        "Failed to set window source size");

    screen_flush_context(m_screenContext, 0);
}

void QQnxWindow::setVisible(bool visible)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window() << "visible =" << visible;

    if (m_visible == visible)
        return;

    // The first time through we join a window group if appropriate.
    if (m_parentGroupName.isNull() && !m_isTopLevel) {
        joinWindowGroup(parent() ? static_cast<QQnxWindow*>(parent())->groupName()
                                 : QByteArray(m_screen->windowGroupName()));
    }

    m_visible = visible;

    QQnxWindow *root = this;
    while (root->m_parentWindow)
        root = root->m_parentWindow;

    root->updateVisibility(root->m_visible);

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
    int val = (m_visible && parentVisible) ? 1 : 0;
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_VISIBLE, &val),
                        "Failed to set window visibility");

    Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
        childWindow->updateVisibility(m_visible && parentVisible);
}

void QQnxWindow::setOpacity(qreal level)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window() << "opacity =" << level;
    // Set window global alpha
    int val = (int)(level * 255);
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_GLOBAL_ALPHA, &val),
                        "Failed to set global alpha");

    screen_flush_context(m_screenContext, 0);
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
    // libscreen fails when creating empty buffers
    const QSize nonEmptySize = size.isEmpty() ? QSize(1, 1) : size;

    int val[2] = { nonEmptySize.width(), nonEmptySize.height() };
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_BUFFER_SIZE, val),
                        "Failed to set window buffer size");

    // Create window buffers if they do not exist
    if (m_bufferSize.isEmpty()) {
        val[0] = pixelFormat();
        if (val[0] == -1) // The platform GL context was not set yet on the window, so we can't procede
            return;

        Q_SCREEN_CRITICALERROR(
                screen_set_window_property_iv(m_window, SCREEN_PROPERTY_FORMAT, val),
                "Failed to set window format");

        Q_SCREEN_CRITICALERROR(screen_create_window_buffers(m_window, MAX_BUFFER_COUNT),
                            "Failed to create window buffers");

        // check if there are any buffers available
        int bufferCount = 0;
        Q_SCREEN_CRITICALERROR(
                screen_get_window_property_iv(m_window, SCREEN_PROPERTY_RENDER_BUFFER_COUNT, &bufferCount),
                "Failed to query render buffer count");

        if (bufferCount != MAX_BUFFER_COUNT) {
            qFatal("QQnxWindow: invalid buffer count. Expected = %d, got = %d. You might experience problems.",
                    MAX_BUFFER_COUNT, bufferCount);
        }
    }

    // Set the transparency. According to QNX technical support, setting the window
    // transparency property should always be done *after* creating the window
    // buffers in order to guarantee the property is paid attention to.
    if (window()->requestedFormat().alphaBufferSize() == 0) {
        // To avoid overhead in the composition manager, disable blending
        // when the underlying window buffer doesn't have an alpha channel.
        val[0] = SCREEN_TRANSPARENCY_NONE;
    } else {
        // Normal alpha blending. This doesn't commit us to translucency; the
        // normal backfill during the painting will contain a fully opaque
        // alpha channel unless the user explicitly intervenes to make something
        // transparent.
        val[0] = SCREEN_TRANSPARENCY_SOURCE_OVER;
    }

    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_TRANSPARENCY, val),
                        "Failed to set window transparency");

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

        if ((QQnxIntegration::options() & QQnxIntegration::RootWindow)) {
            screen_leave_window_group(m_window);
        }
    }

    m_screen = platformScreen;
    if (!m_parentWindow) {
        platformScreen->addWindow(this);
    }
    if (m_isTopLevel) {
        // Move window to proper screen/display
        screen_display_t display = platformScreen->nativeDisplay();
        Q_SCREEN_CHECKERROR(
               screen_set_window_property_pv(m_window, SCREEN_PROPERTY_DISPLAY, (void **)&display),
               "Failed to set window display");
    } else {
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

    if (screen()->rootWindow() == this) {
        qWarning() << "Application window cannot be reparented";
        return;
    }

    removeFromParent();
    m_parentWindow = newParent;

    // Add to new hierarchy position.
    if (m_parentWindow) {
        if (m_parentWindow->m_screen != m_screen)
            setScreen(m_parentWindow->m_screen);

        m_parentWindow->m_childWindows.push_back(this);
        joinWindowGroup(m_parentWindow->groupName());
    } else {
        m_screen->addWindow(this);
        joinWindowGroup(QByteArray());
    }

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
    QQnxWindow *focusWindow = 0;
    if (QGuiApplication::focusWindow())
        focusWindow = static_cast<QQnxWindow*>(QGuiApplication::focusWindow()->handle());

    if (focusWindow == this)
        return;

    if (screen()->rootWindow() == this ||
            (focusWindow && findWindow(focusWindow->nativeHandle()))) {
        // If the focus window is a child, we can just set the focus of our own window
        // group to our window handle
        setFocus(nativeHandle());
    } else {
        // In order to receive focus the parent's window group has to give focus to the
        // child. If we have several hierarchy layers, we have to do that several times
        QQnxWindow *currentWindow = this;
        QList<QQnxWindow*> windowList;
        while (currentWindow) {
            windowList.prepend(currentWindow);
            // If we find the focus window, we don't have to go further
            if (currentWindow == focusWindow)
                break;

            if (currentWindow->parent()){
                currentWindow = static_cast<QQnxWindow*>(currentWindow->parent());
            } else if (screen()->rootWindow() &&
                  screen()->rootWindow()->m_windowGroupName == currentWindow->m_parentGroupName) {
                currentWindow = screen()->rootWindow();
            } else {
                currentWindow = 0;
            }
        }

        // We have to apply the focus from parent to child windows
        for (int i = 1; i < windowList.size(); ++i)
            windowList.at(i-1)->setFocus(windowList.at(i)->nativeHandle());

        windowList.last()->setFocus(windowList.last()->nativeHandle());
    }

    screen_flush_context(m_screenContext, 0);
}

void QQnxWindow::setFocus(screen_window_t newFocusWindow)
{
    screen_group_t screenGroup = 0;
    screen_get_window_property_pv(nativeHandle(), SCREEN_PROPERTY_GROUP,
                                  reinterpret_cast<void**>(&screenGroup));
    if (screenGroup) {
        screen_set_group_property_pv(screenGroup, SCREEN_PROPERTY_KEYBOARD_FOCUS,
                                 reinterpret_cast<void**>(&newFocusWindow));
    }
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
    Q_SCREEN_CHECKERROR(
            screen_set_window_property_iv(m_window, SCREEN_PROPERTY_ROTATION, &rotation),
            "Failed to set window rotation");
}

void QQnxWindow::initWindow()
{
    // Alpha channel is always pre-multiplied if present
    int val = SCREEN_PRE_MULTIPLIED_ALPHA;
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_ALPHA_MODE, &val),
                        "Failed to set alpha mode");

    // Set the window swap interval
    val = 1;
    Q_SCREEN_CHECKERROR(
            screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SWAP_INTERVAL, &val),
            "Failed to set swap interval");

    if (window()->flags() & Qt::WindowDoesNotAcceptFocus) {
        val = SCREEN_SENSITIVITY_NO_FOCUS;
        Q_SCREEN_CHECKERROR(
                screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SENSITIVITY, &val),
                "Failed to set window sensitivity");
    }

    QQnxScreen *platformScreen = static_cast<QQnxScreen *>(window()->screen()->handle());
    setScreen(platformScreen);

    if (window()->type() == Qt::CoverWindow) {
#if defined(Q_OS_BLACKBERRY) && !defined(Q_OS_BLACKBERRY_TABLET)
        if (platformScreen->rootWindow()) {
            screen_set_window_property_pv(m_screen->rootWindow()->nativeHandle(),
                                          SCREEN_PROPERTY_ALTERNATE_WINDOW, (void**)&m_window);
            m_cover.reset(new QQnxNavigatorCover);
        } else {
            qWarning("No root window for cover window");
        }
#endif
        m_exposed = false;
    }

    // Add window to plugin's window mapper
    QQnxIntegration::addWindow(m_window, window());

    // Qt never calls these setters after creating the window, so we need to do that ourselves here
    setWindowState(window()->windowState());
    setOpacity(window()->opacity());

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
    Q_SCREEN_CHECKERROR(screen_create_window_group(m_window, m_windowGroupName.constData()),
                        "Failed to create window group");
}

void QQnxWindow::joinWindowGroup(const QByteArray &groupName)
{
    bool changed = false;

    qWindowDebug() << Q_FUNC_INFO << "group:" << groupName;

    if (!groupName.isEmpty()) {
        if (groupName != m_parentGroupName) {
            screen_join_window_group(m_window, groupName);
            m_parentGroupName = groupName;
            changed = true;
        }
    } else {
        if (!m_parentGroupName.isEmpty()) {
            screen_leave_window_group(m_window);
            changed = true;
        }
        // By setting to an empty string we'll stop setVisible from trying to
        // change our group, we want that to happen only if joinWindowGroup has
        // never been called.  This allows windows to be created that are not initially
        // part of any group.
        m_parentGroupName = "";
    }

    if (changed)
        screen_flush_context(m_screenContext, 0);
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
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(window, SCREEN_PROPERTY_ZORDER, &topZorder),
                        "Failed to set window z-order");
    topZorder++;
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
