/***************************************************************************
**
** Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxglobal.h"

#include "qqnxwindow.h"
#include "qqnxintegration.h"
#include "qqnxscreen.h"
#include "qqnxlgmon.h"

#include <QUuid>

#include <QtGui/QWindow>
#include <qpa/qwindowsysteminterface.h>

#include "private/qguiapplication_p.h"

#include <QtCore/QDebug>

#include <errno.h>

#if defined(QQNXWINDOW_DEBUG)
#define qWindowDebug qDebug
#else
#define qWindowDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

#define DECLARE_DEBUG_VAR(variable) \
    static bool debug_ ## variable() \
    { static bool value = qgetenv("QNX_SCREEN_DEBUG").contains(QT_STRINGIFY(variable)); return value; }
DECLARE_DEBUG_VAR(fps)
DECLARE_DEBUG_VAR(posts)
DECLARE_DEBUG_VAR(blits)
DECLARE_DEBUG_VAR(updates)
DECLARE_DEBUG_VAR(cpu_time)
DECLARE_DEBUG_VAR(gpu_time)
DECLARE_DEBUG_VAR(statistics)
#undef DECLARE_DEBUG_VAR

/*!
    \class QQnxWindow
    \brief The QQnxWindow is the base class of the various classes used as instances of
    QPlatformWindow in the QNX QPA plugin.

    The standard properties and methods available in Qt are not a perfect match for the
    features provided by the QNX screen service. While for the majority of applications
    the default behavior suffices, some circumstances require greater control over the
    interaction with screen.

    \section1 Window Types

    The QNX QPA plugin can operate in two modes, with or without a root window. The
    selection of mode is made via the \e rootwindow and \e no-rootwindow options to the
    plugin. The default mode is rootwindow for BlackBerry builds and no-rootwindow for
    non-BlackBerry builds.

    Windows with parents are always created as child windows, the difference in the modes
    is in the treatment of parentless windows. In no-rootwindow mode, these windows are
    created as application windows while in rootwindow mode, the first window on a screen
    is created as an application window while subsequent windows are created as child
    windows. The only exception to this is any window of type Qt::Desktop or Qt::CoverWindow;
    these are created as application windows, but will never become the root window,
    even if they are the first window created.

    It is also possible to create a parentless child window. These may be useful to
    create windows that are parented by windows from other processes. To do this, you
    attach a dynamic property \e qnxInitialWindowGroup to the QWindow though this must be done
    prior to the platform window class (this class) being created which typically happens
    when the window is made visible. When the window is created in QML, it is acceptable
    to have the \e visible property hardcoded to true so long as the qnxInitialWindowGroup
    is also set.

    \section1 Joining Window Groups

    Window groups may be joined in a number of ways, some are automatic based on
    predefined rules though an application is also able to provide explicit control.

    A QWindow that has a parent will join its parent's window group. When rootwindow mode
    is in effect, all but the first parentless window on a screen will be child windows
    and join the window group of the first parentless window, the root window.

    If a QWindow has a valid dynamic property called \e qnxInitialWindowGroup at the time the
    QQnxWindow is created, the window will be created as a child window and, if the
    qnxInitialWindowGroup property is a non-empty string, an attempt will be made to join that
    window group. This has an effect only when the QQnxWindow is created, subsequent
    changes to this property are ignored. Setting the property to an empty string
    provides a means to create 'top level' child windows without automatically joining
    any group. Typically when this property is used \e qnxWindowId should be used as well
    so that the process that owns the window group being joined has some means to
    identify the window.

    At any point following the creation of the QQnxWindow object, an application can
    change the window group it has joined. This is done by using the \e
    setWindowProperty function of the native interface to set the \e qnxWindowGroup property
    to the desired value, for example:

    \snippet code/src_plugins_platforms_qnx_qqnxwindow.cpp 0

    To leave the current window group, one passes a null value for the property value,
    for example:

    \snippet code/src_plugins_platforms_qnx_qqnxwindow.cpp 1

    \section1 Window Id

    The screen window id string property can be set on a window by assigning the desired
    value to a dynamic property \e qnxWindowId on the QWindow prior to the QQnxWindow having
    been created. This is often wanted when one joins a window group belonging to a
    different process.

*/
QQnxWindow::QQnxWindow(QWindow *window, screen_context_t context, bool needRootWindow)
    : QPlatformWindow(window),
      m_screenContext(context),
      m_window(0),
      m_screen(0),
      m_parentWindow(0),
      m_visible(false),
      m_exposed(true),
      m_foreign(false),
      m_windowState(Qt::WindowNoState),
      m_mmRendererWindow(0),
      m_firstActivateHandled(false)
{
    qWindowDebug() << "window =" << window << ", size =" << window->size();

    QQnxScreen *platformScreen = static_cast<QQnxScreen *>(window->screen()->handle());

    // If a qnxInitialWindowGroup property is set on the window we'll take this as an
    // indication that we want to create a child window and join that window group.
    QVariant windowGroup = window->property("qnxInitialWindowGroup");
    if (!windowGroup.isValid())
        windowGroup = window->property("_q_platform_qnxParentGroup");

    if (window->type() == Qt::CoverWindow) {
        // Cover windows have to be top level to be accessible to window delegate (i.e. navigator)
        // Desktop windows also need to be toplevel because they are not
        // supposed to be part of the window hierarchy tree
        m_isTopLevel = true;
    } else if (parent() || windowGroup.isValid()) {
        // If we have a parent we are a child window.  Sometimes we have to be a child even if we
        // don't have a parent e.g. our parent might be in a different process.
        m_isTopLevel = false;
    } else {
        // We're parentless.  If we're not using a root window, we'll always be a top-level window
        // otherwise only the first window is.
        m_isTopLevel = !needRootWindow || !platformScreen->rootWindow();
    }

    if (window->type() == Qt::Desktop)  // A desktop widget does not need a libscreen window
        return;

    QVariant type = window->property("_q_platform_qnxWindowType");
    if (type.isValid() && type.canConvert<int>()) {
        Q_SCREEN_CHECKERROR(
                screen_create_window_type(&m_window, m_screenContext, type.value<int>()),
                "Could not create window");
    } else if (m_isTopLevel) {
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

    // If the window has a qnxWindowId property, set this as the string id property. This generally
    // needs to be done prior to joining any group as it might be used by the owner of the
    // group to identify the window.
    QVariant windowId = window->property("qnxWindowId");
    if (!windowId.isValid())
        windowId = window->property("_q_platform_qnxWindowId");
    if (windowId.isValid() && windowId.canConvert<QByteArray>()) {
        QByteArray id = windowId.toByteArray();
        Q_SCREEN_CHECKERROR(screen_set_window_property_cv(m_window, SCREEN_PROPERTY_ID_STRING,
                            id.size(), id), "Failed to set id");
    }

    // If a window group has been provided join it now. If it's an empty string that's OK too,
    // it'll cause us not to join a group (the app will presumably join at some future time).
    if (windowGroup.isValid() && windowGroup.canConvert<QByteArray>())
        joinWindowGroup(windowGroup.toByteArray());

    int debug = 0;
    if (Q_UNLIKELY(debug_fps())) {
        debug |= SCREEN_DEBUG_GRAPH_FPS;
    }
    if (Q_UNLIKELY(debug_posts())) {
        debug |= SCREEN_DEBUG_GRAPH_POSTS;
    }
    if (Q_UNLIKELY(debug_blits())) {
        debug |= SCREEN_DEBUG_GRAPH_BLITS;
    }
    if (Q_UNLIKELY(debug_updates())) {
        debug |= SCREEN_DEBUG_GRAPH_UPDATES;
    }
    if (Q_UNLIKELY(debug_cpu_time())) {
        debug |= SCREEN_DEBUG_GRAPH_CPU_TIME;
    }
    if (Q_UNLIKELY(debug_gpu_time())) {
        debug |= SCREEN_DEBUG_GRAPH_GPU_TIME;
    }
    if (Q_UNLIKELY(debug_statistics())) {
        debug = SCREEN_DEBUG_STATISTICS;
    }

    if (debug > 0) {
        Q_SCREEN_CHECKERROR(screen_set_window_property_iv(nativeHandle(), SCREEN_PROPERTY_DEBUG, &debug),
                            "Could not set SCREEN_PROPERTY_DEBUG");
        qWindowDebug() << "window SCREEN_PROPERTY_DEBUG= " << debug;
    }
}

QQnxWindow::QQnxWindow(QWindow *window, screen_context_t context, screen_window_t screenWindow)
    : QPlatformWindow(window)
    , m_screenContext(context)
    , m_window(screenWindow)
    , m_screen(0)
    , m_parentWindow(0)
    , m_visible(false)
    , m_exposed(true)
    , m_foreign(true)
    , m_windowState(Qt::WindowNoState)
    , m_mmRendererWindow(0)
    , m_parentGroupName(256, 0)
    , m_isTopLevel(false)
{
    qWindowDebug() << "window =" << window << ", size =" << window->size();

    collectWindowGroup();

    screen_get_window_property_cv(m_window,
                                  SCREEN_PROPERTY_PARENT,
                                  m_parentGroupName.size(),
                                  m_parentGroupName.data());
    m_parentGroupName.resize(strlen(m_parentGroupName.constData()));

    // If a window group has been provided join it now. If it's an empty string that's OK too,
    // it'll cause us not to join a group (the app will presumably join at some future time).
    QVariant parentGroup = window->property("qnxInitialWindowGroup");
    if (!parentGroup.isValid())
        parentGroup = window->property("_q_platform_qnxParentGroup");
    if (parentGroup.isValid() && parentGroup.canConvert<QByteArray>())
        joinWindowGroup(parentGroup.toByteArray());
}

QQnxWindow::~QQnxWindow()
{
    qWindowDebug() << "window =" << window();

    // Qt should have already deleted the children before deleting the parent.
    Q_ASSERT(m_childWindows.size() == 0);

    // Remove from plugin's window mapper
    QQnxIntegration::instance()->removeWindow(m_window);

    // Remove from parent's Hierarchy.
    removeFromParent();
    if (m_screen)
        m_screen->updateHierarchy();

    // Cleanup QNX window and its buffers
    // Foreign windows are cleaned up externally after the CLOSE event has been handled.
    if (m_foreign)
        removeContextPermission();
    else
        screen_destroy_window(m_window);
}

void QQnxWindow::setGeometry(const QRect &rect)
{
    QRect newGeometry = rect;
    if (shouldMakeFullScreen())
        newGeometry = screen()->geometry();

    if (window()->type() != Qt::Desktop)
        setGeometryHelper(newGeometry);

    if (isExposed())
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), newGeometry.size()));
}

void QQnxWindow::setGeometryHelper(const QRect &rect)
{
    qWindowDebug() << "window =" << window()
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

    QWindowSystemInterface::handleGeometryChange(window(), rect);
}

void QQnxWindow::setVisible(bool visible)
{
    qWindowDebug() << "window =" << window() << "visible =" << visible;

    if (m_visible == visible || window()->type() == Qt::Desktop)
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

    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), window()->geometry().size()));

    if (visible) {
        applyWindowState();
    } else {
        if (showWithoutActivating() && focusable() && m_firstActivateHandled) {
            m_firstActivateHandled = false;
            int val = SCREEN_SENSITIVITY_NO_FOCUS;
            Q_SCREEN_CHECKERROR(
                screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SENSITIVITY, &val),
                "Failed to set window sensitivity");
        }

        // Flush the context, otherwise it won't disappear immediately
        screen_flush_context(m_screenContext, 0);
    }
}

void QQnxWindow::updateVisibility(bool parentVisible)
{
    qWindowDebug() << "parentVisible =" << parentVisible << "window =" << window();
    // Set window visibility
    int val = (m_visible && parentVisible) ? 1 : 0;
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_VISIBLE, &val),
                        "Failed to set window visibility");

    Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
        childWindow->updateVisibility(m_visible && parentVisible);
}

void QQnxWindow::setOpacity(qreal level)
{
    qWindowDebug() << "window =" << window() << "opacity =" << level;
    // Set window global alpha
    int val = (int)(level * 255);
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_GLOBAL_ALPHA, &val),
                        "Failed to set global alpha");

    screen_flush_context(m_screenContext, 0);
}

void QQnxWindow::setExposed(bool exposed)
{
    qWindowDebug() << "window =" << window() << "expose =" << exposed;

    if (m_exposed != exposed) {
        m_exposed = exposed;
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), window()->geometry().size()));
    }
}

bool QQnxWindow::isExposed() const
{
    return m_visible && m_exposed;
}

void QQnxWindow::setBufferSize(const QSize &size)
{
    qWindowDebug() << "window =" << window() << "size =" << size;

    // libscreen fails when creating empty buffers
    const QSize nonEmptySize = size.isEmpty() ? QSize(1, 1) : size;
    int format = pixelFormat();

    if (nonEmptySize == m_bufferSize || format == -1)
        return;

    Q_SCREEN_CRITICALERROR(
            screen_set_window_property_iv(m_window, SCREEN_PROPERTY_FORMAT, &format),
            "Failed to set window format");

    if (m_bufferSize.isValid()) {
        // destroy buffers first, if resized
        Q_SCREEN_CRITICALERROR(screen_destroy_window_buffers(m_window),
                               "Failed to destroy window buffers");
    }

    int val[2] = { nonEmptySize.width(), nonEmptySize.height() };
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_BUFFER_SIZE, val),
                        "Failed to set window buffer size");

    Q_SCREEN_CRITICALERROR(screen_create_window_buffers(m_window, MAX_BUFFER_COUNT),
                           "Failed to create window buffers");

    // check if there are any buffers available
    int bufferCount = 0;
    Q_SCREEN_CRITICALERROR(
        screen_get_window_property_iv(m_window, SCREEN_PROPERTY_RENDER_BUFFER_COUNT, &bufferCount),
        "Failed to query render buffer count");

    if (Q_UNLIKELY(bufferCount != MAX_BUFFER_COUNT)) {
        qFatal("QQnxWindow: invalid buffer count. Expected = %d, got = %d.",
                MAX_BUFFER_COUNT, bufferCount);
    }

    // Set the transparency. According to QNX technical support, setting the window
    // transparency property should always be done *after* creating the window
    // buffers in order to guarantee the property is paid attention to.
    if (size.isEmpty()) {
        // We can't create 0x0 buffers and instead make them 1x1.  But to allow these windows to
        // still be 'visible' (thus allowing their children to be visible), we need to allow
        // them to be posted but still not show up.
        val[0] = SCREEN_TRANSPARENCY_DISCARD;
    } else if (window()->requestedFormat().alphaBufferSize() == 0) {
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
    qWindowDebug() << "window =" << window() << "platformScreen =" << platformScreen;

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
        qWindowDebug("Moving window to different screen");
        m_screen->removeWindow(this);

        if ((QQnxIntegration::instance()->options() & QQnxIntegration::RootWindow)) {
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
    qWindowDebug() << "window =" << window();
    // Remove from old Hierarchy position
    if (m_parentWindow) {
        if (Q_UNLIKELY(!m_parentWindow->m_childWindows.removeAll(this)))
            qFatal("QQnxWindow: Window Hierarchy broken; window has parent, but parent hasn't got child.");
        else
            m_parentWindow = 0;
    } else if (m_screen) {
        m_screen->removeWindow(this);
    }
}

void QQnxWindow::setParent(const QPlatformWindow *window)
{
    qWindowDebug() << "window =" << this->window() << "platformWindow =" << window;
    // Cast away the const, we need to modify the hierarchy.
    QQnxWindow* const newParent = static_cast<QQnxWindow*>(const_cast<QPlatformWindow*>(window));

    if (newParent == m_parentWindow)
        return;

    if (static_cast<QQnxScreen *>(screen())->rootWindow() == this) {
        qWarning("Application window cannot be reparented");
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
    qWindowDebug() << "window =" << window();

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
    qWindowDebug() << "window =" << window();

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

    if (static_cast<QQnxScreen *>(screen())->rootWindow() == this ||
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
            auto platformScreen = static_cast<QQnxScreen *>(screen());
            windowList.prepend(currentWindow);
            // If we find the focus window, we don't have to go further
            if (currentWindow == focusWindow)
                break;

            if (currentWindow->parent()){
                currentWindow = static_cast<QQnxWindow*>(currentWindow->parent());
            } else if (platformScreen->rootWindow() &&
                  platformScreen->rootWindow()->m_windowGroupName == currentWindow->m_parentGroupName) {
                currentWindow = platformScreen->rootWindow();
            } else {
                currentWindow = 0;
            }
        }

        // We have to apply the focus from parent to child windows
        for (int i = 1; i < windowList.size(); ++i)
            windowList.at(i-1)->setFocus(windowList.at(i)->nativeHandle());

        windowList.last()->setFocus(windowList.constLast()->nativeHandle());
    }

    screen_flush_context(m_screenContext, 0);
}

void QQnxWindow::setFocus(screen_window_t newFocusWindow)
{
    screen_window_t temporaryFocusWindow = nullptr;

    screen_group_t screenGroup = 0;
    Q_SCREEN_CHECKERROR(screen_get_window_property_pv(nativeHandle(), SCREEN_PROPERTY_GROUP,
                                                      reinterpret_cast<void **>(&screenGroup)),
                        "Failed to retrieve window group");

    if (showWithoutActivating() && focusable() && !m_firstActivateHandled) {
        m_firstActivateHandled = true;
        int val = SCREEN_SENSITIVITY_TEST;
        Q_SCREEN_CHECKERROR(
            screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SENSITIVITY, &val),
            "Failed to set window sensitivity");

#if _SCREEN_VERSION < _SCREEN_MAKE_VERSION(1, 0, 0)
        // For older versions of screen, the window may still have group
        // focus even though it was marked NO_FOCUS when it was hidden.
        // In that situation, focus has to be given to another window
        // so that this window can take focus back from it.
        screen_window_t oldFocusWindow = nullptr;
        Q_SCREEN_CHECKERROR(
            screen_get_group_property_pv(screenGroup, SCREEN_PROPERTY_FOCUS,
                                         reinterpret_cast<void **>(&oldFocusWindow)),
            "Failed to retrieve group focus");
        if (newFocusWindow == oldFocusWindow) {
            char groupName[256];
            memset(groupName, 0, sizeof(groupName));
            Q_SCREEN_CHECKERROR(screen_get_group_property_cv(screenGroup, SCREEN_PROPERTY_NAME,
                                                             sizeof(groupName) - 1, groupName),
                                "Failed to retrieve group name");

            Q_SCREEN_CHECKERROR(screen_create_window_type(&temporaryFocusWindow,
                                                          m_screenContext, SCREEN_CHILD_WINDOW),
                                "Failed to create temporary focus window");
            Q_SCREEN_CHECKERROR(screen_join_window_group(temporaryFocusWindow, groupName),
                                "Temporary focus window failed to join window group");
            Q_SCREEN_CHECKERROR(
                screen_set_group_property_pv(screenGroup, SCREEN_PROPERTY_FOCUS,
                                             reinterpret_cast<void **>(&temporaryFocusWindow)),
                "Temporary focus window failed to take focus");
            screen_flush_context(m_screenContext, 0);
        }
#endif
    }

    Q_SCREEN_CHECKERROR(screen_set_group_property_pv(screenGroup, SCREEN_PROPERTY_FOCUS,
                                                     reinterpret_cast<void **>(&newFocusWindow)),
                        "Failed to set group focus");

    screen_destroy_window(temporaryFocusWindow);
}

void QQnxWindow::setWindowState(Qt::WindowStates state)
{
    qWindowDebug() << "state =" << state;

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
    qWindowDebug("ignored");
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

QPlatformScreen *QQnxWindow::screen() const
{
    return m_screen;
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
    qWarning("Qt::WindowMinimized is not supported by this OS version");
}

void QQnxWindow::setRotation(int rotation)
{
    qWindowDebug() << "angle =" << rotation;
    Q_SCREEN_CHECKERROR(
            screen_set_window_property_iv(m_window, SCREEN_PROPERTY_ROTATION, &rotation),
            "Failed to set window rotation");
}

void QQnxWindow::initWindow()
{
    if (window()->type() == Qt::Desktop)
        return;

    // Alpha channel is always pre-multiplied if present
    int val = SCREEN_PRE_MULTIPLIED_ALPHA;
    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(m_window, SCREEN_PROPERTY_ALPHA_MODE, &val),
                        "Failed to set alpha mode");

    // Set the window swap interval
    val = 1;
    Q_SCREEN_CHECKERROR(
            screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SWAP_INTERVAL, &val),
            "Failed to set swap interval");

    if (showWithoutActivating() || !focusable()) {
        // NO_FOCUS is temporary for showWithoutActivating (and pop-up) windows.
        // Using NO_FOCUS ensures that screen doesn't activate the window because
        // it was just created.  Sensitivity will be changed to TEST when the
        // window is clicked or touched.
        val = SCREEN_SENSITIVITY_NO_FOCUS;
        Q_SCREEN_CHECKERROR(
                screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SENSITIVITY, &val),
                "Failed to set window sensitivity");
    }

    QQnxScreen *platformScreen = static_cast<QQnxScreen *>(window()->screen()->handle());
    setScreen(platformScreen);

    if (window()->type() == Qt::CoverWindow)
        m_exposed = false;

    // Add window to plugin's window mapper
    QQnxIntegration::instance()->addWindow(m_window, window());

    // Qt never calls these setters after creating the window, so we need to do that ourselves here
    setWindowState(window()->windowState());
    setOpacity(window()->opacity());

    if (window()->parent() && window()->parent()->handle())
        setParent(window()->parent()->handle());

    setGeometryHelper(shouldMakeFullScreen() ? screen()->geometry() : window()->geometry());
}

void QQnxWindow::collectWindowGroup()
{
    QByteArray groupName(256, 0);
    Q_SCREEN_CHECKERROR(screen_get_window_property_cv(m_window,
                                                      SCREEN_PROPERTY_GROUP,
                                                      groupName.size(),
                                                      groupName.data()),
                        "Failed to retrieve window group");
    groupName.resize(strlen(groupName.constData()));
    m_windowGroupName = groupName;
}

void QQnxWindow::createWindowGroup()
{
    Q_SCREEN_CHECKERROR(screen_create_window_group(m_window, nullptr),
                        "Failed to create window group");

    collectWindowGroup();
}

void QQnxWindow::joinWindowGroup(const QByteArray &groupName)
{
    bool changed = false;

    qWindowDebug() << "group:" << groupName;

    // screen has this annoying habit of generating a CLOSE/CREATE when the owner context of
    // the parent group moves a foreign window to another group that it also owns.  The
    // CLOSE/CREATE changes the identity of the foreign window.  Usually, this is undesirable.
    // To prevent this CLOSE/CREATE when changing the parent group, we temporarily add a
    // context permission for the Qt context.  screen won't send a CLOSE/CREATE when the
    // context has some permission other than the PARENT permission.  If there isn't a new
    // group (the window has no parent), this context permission is left in place.

    if (m_foreign && !m_parentGroupName.isEmpty())\
        addContextPermission();

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

    if (m_foreign && !groupName.isEmpty())
        removeContextPermission();

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
    if (m_windowState & Qt::WindowMinimized) {
        minimize();

        if (m_unmaximizedGeometry.isValid())
            setGeometry(m_unmaximizedGeometry);
        else
            setGeometry(m_screen->geometry());
    } else if (m_windowState & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
        m_unmaximizedGeometry = geometry();
        setGeometry(m_windowState & Qt::WindowFullScreen ? m_screen->geometry()
                                                         : m_screen->availableGeometry());
    } else if (m_unmaximizedGeometry.isValid()) {
        setGeometry(m_unmaximizedGeometry);
    }
}

void QQnxWindow::windowPosted()
{
    if (m_cover)
        m_cover->updateCover();

    qqnxLgmonFramePosted(m_cover);  // for performance measurements
}

bool QQnxWindow::shouldMakeFullScreen() const
{
    return ((static_cast<QQnxScreen *>(screen())->rootWindow() == this)
            && (QQnxIntegration::instance()->options() & QQnxIntegration::FullScreenApplication));
}


void QQnxWindow::handleActivationEvent()
{
    if (showWithoutActivating() && focusable() && !m_firstActivateHandled)
        requestActivateWindow();
}

bool QQnxWindow::showWithoutActivating() const
{
    return (window()->flags() & Qt::Popup) == Qt::Popup
        || window()->property("_q_showWithoutActivating").toBool();
}

bool QQnxWindow::focusable() const
{
    return (window()->flags() & Qt::WindowDoesNotAcceptFocus) != Qt::WindowDoesNotAcceptFocus;
}

void QQnxWindow::addContextPermission()
{
    QByteArray grantString("context:");
    grantString.append(QQnxIntegration::instance()->screenContextId());
    grantString.append(":rw-");
    screen_set_window_property_cv(m_window,
            SCREEN_PROPERTY_PERMISSIONS,
            grantString.length(),
            grantString.data());
}

void QQnxWindow::removeContextPermission()
{
    QByteArray revokeString("context:");
    revokeString.append(QQnxIntegration::instance()->screenContextId());
    revokeString.append(":---");
    screen_set_window_property_cv(m_window,
            SCREEN_PROPERTY_PERMISSIONS,
            revokeString.length(),
            revokeString.data());
}

QT_END_NAMESPACE
