/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#include "qqnxscreen.h"
#include "qqnxwindow.h"

#include <QtCore/QThread>
#ifdef QQNXSCREEN_DEBUG
#    include <QtCore/QDebug>
#endif
#include <QtGui/QWindowSystemInterface>

#include <errno.h>

QT_BEGIN_NAMESPACE

QQnxScreen::QQnxScreen(screen_context_t screenContext, screen_display_t display, bool primaryScreen)
    : m_screenContext(screenContext),
      m_display(display),
      m_rootWindow(),
      m_primaryScreen(primaryScreen),
      m_posted(false),
      m_keyboardHeight(0),
      m_platformContext(0)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // Cache initial orientation of this display
    // TODO: use ORIENTATION environment variable?
    errno = 0;
    int result = screen_get_display_property_iv(m_display, SCREEN_PROPERTY_ROTATION, &m_initialRotation);
    if (result != 0) {
        qFatal("QQnxScreen: failed to query display rotation, errno=%d", errno);
    }
    m_currentRotation = m_initialRotation;

    // Cache size of this display in pixels
    // TODO: use WIDTH and HEIGHT environment variables?
    errno = 0;
    int val[2];
    result = screen_get_display_property_iv(m_display, SCREEN_PROPERTY_SIZE, val);
    if (result != 0) {
        qFatal("QQnxScreen: failed to query display size, errno=%d", errno);
    }

    m_currentGeometry = m_initialGeometry = QRect(0, 0, val[0], val[1]);

    // Cache size of this display in millimeters
    errno = 0;
    result = screen_get_display_property_iv(m_display, SCREEN_PROPERTY_PHYSICAL_SIZE, val);
    if (result != 0) {
        qFatal("QQnxScreen: failed to query display physical size, errno=%d", errno);
    }

    m_currentPhysicalSize = m_initialPhysicalSize = QSize(val[0], val[1]);

    // We only create the root window if we are the primary display.
    if (primaryScreen)
        m_rootWindow = QSharedPointer<QQnxRootWindow>(new QQnxRootWindow(this));
}

QQnxScreen::~QQnxScreen()
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
}

static int defaultDepth()
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    static int defaultDepth = 0;
    if (defaultDepth == 0) {
        // check if display depth was specified in environment variable;
        // use default value if no valid value found
        defaultDepth = qgetenv("QQNX_DISPLAY_DEPTH").toInt();
        if (defaultDepth != 16 && defaultDepth != 32) {
            defaultDepth = 32;
        }
    }
    return defaultDepth;
}

QRect QQnxScreen::availableGeometry() const
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // available geometry = total geometry - keyboard
    return QRect(m_currentGeometry.x(), m_currentGeometry.y(),
                 m_currentGeometry.width(), m_currentGeometry.height() - m_keyboardHeight);
}

int QQnxScreen::depth() const
{
    return defaultDepth();
}

/*!
    Check if the supplied angles are perpendicular to each other.
*/
static bool isOrthogonal(int angle1, int angle2)
{
    return ((angle1 - angle2) % 180) != 0;
}

void QQnxScreen::setRotation(int rotation)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "orientation =" << rotation;
#endif
    // Check if rotation changed
    if (m_currentRotation != rotation) {
        // Update rotation of root window
        if (m_rootWindow)
            m_rootWindow->setRotation(rotation);

        // Swap dimensions if we've rotated 90 or 270 from initial orientation
        if (isOrthogonal(m_initialRotation, rotation)) {
            m_currentGeometry = QRect(0, 0, m_initialGeometry.height(), m_initialGeometry.width());
            m_currentPhysicalSize = QSize(m_initialPhysicalSize.height(), m_initialPhysicalSize.width());
        } else {
            m_currentGeometry = QRect(0, 0, m_initialGeometry.width(), m_initialGeometry.height());
            m_currentPhysicalSize = m_initialPhysicalSize;
        }

        // Resize root window if we've rotated 90 or 270 from previous orientation
        if (isOrthogonal(m_currentRotation, rotation)) {
#if defined(QQNXSCREEN_DEBUG)
            qDebug() << Q_FUNC_INFO << "resize, size =" << m_currentGeometry.size();
#endif
            if (m_rootWindow)
                m_rootWindow->resize(m_currentGeometry.size());
        } else {
            // TODO: Find one global place to flush display updates
            // Force immediate display update if no geometry changes required
            if (m_rootWindow)
                m_rootWindow->flush();
        }

        // Save new rotation
        m_currentRotation = rotation;

        // TODO: check if other screens are supposed to rotate as well and/or whether this depends
        // on if clone mode is being used.
        // Rotating only the primary screen is what we had in the navigator event handler before refactoring
        if (m_primaryScreen)
            QWindowSystemInterface::handleScreenGeometryChange(screen(), m_currentGeometry);
    }
}

QQnxWindow *QQnxScreen::findWindow(screen_window_t windowHandle)
{
    Q_FOREACH (QQnxWindow *window, m_childWindows) {
        QQnxWindow * const result = window->findWindow(windowHandle);
        if (result)
            return result;
    }

    return 0;
}

void QQnxScreen::addWindow(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "window =" << window;
#endif

    if (m_childWindows.contains(window))
        return;

    m_childWindows.push_back(window);
    updateHierarchy();
}

void QQnxScreen::removeWindow(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "window =" << window;
#endif

    const int numWindowsRemoved = m_childWindows.removeAll(window);
    if (numWindowsRemoved > 0)
        updateHierarchy();
}

void QQnxScreen::raiseWindow(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "window =" << window;
#endif

    removeWindow(window);
    m_childWindows.push_back(window);
    updateHierarchy();
}

void QQnxScreen::lowerWindow(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "window =" << window;
#endif

    removeWindow(window);
    m_childWindows.push_front(window);
    updateHierarchy();
}

void QQnxScreen::updateHierarchy()
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    QList<QQnxWindow*>::const_iterator it;
    int topZorder = 1; // root window is z-order 0, all "top" level windows are "above" it

    for (it = m_childWindows.constBegin(); it != m_childWindows.constEnd(); ++it)
        (*it)->updateZorder(topZorder);

    topZorder++;
    Q_FOREACH (screen_window_t overlay, m_overlays) {
        // Do nothing when this fails. This can happen if we have stale windows in mOverlays,
        // which in turn can happen because a window was removed but we didn't get a notification
        // yet.
        screen_set_window_property_iv(overlay, SCREEN_PROPERTY_ZORDER, &topZorder);
        topZorder++;
    }

    // After a hierarchy update, we need to force a flush on all screens.
    // Right now, all screens share a context.
    screen_flush_context( m_screenContext, 0 );
}

void QQnxScreen::onWindowPost(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    Q_UNUSED(window)

    // post app window (so navigator will show it) after first child window
    // has posted; this only needs to happen once as the app window's content
    // never changes
    if (!m_posted && m_rootWindow) {
        m_rootWindow->post();
        m_posted = true;
    }
}

void QQnxScreen::keyboardHeightChanged(int height)
{
    if (height == m_keyboardHeight)
        return;

    m_keyboardHeight = height;

    QWindowSystemInterface::handleScreenAvailableGeometryChange(screen(), availableGeometry());
}

void QQnxScreen::addOverlayWindow(screen_window_t window)
{
    m_overlays.append(window);
    updateHierarchy();
}

void QQnxScreen::removeOverlayWindow(screen_window_t window)
{
    const int numOverlaysRemoved = m_overlays.removeAll(window);
    if (numOverlaysRemoved > 0)
        updateHierarchy();
}

void QQnxScreen::newWindowCreated(void *window)
{
    Q_ASSERT(thread() == QThread::currentThread());
    const screen_window_t windowHandle = reinterpret_cast<screen_window_t>(window);
    screen_display_t display = NULL;
    if (screen_get_window_property_pv(windowHandle, SCREEN_PROPERTY_DISPLAY, (void**)&display) != 0) {
        qWarning("QQnx: Failed to get screen for window, errno=%d", errno);
        return;
    }

    if (display == nativeDisplay()) {
        // A window was created on this screen. If we don't know about this window yet, it means
        // it was not created by Qt, but by some foreign library like the multimedia renderer, which
        // creates an overlay window when playing a video.
        // Treat all foreign windows as overlays here.
        if (!findWindow(windowHandle))
            addOverlayWindow(windowHandle);
    }
}

void QQnxScreen::windowClosed(void *window)
{
    Q_ASSERT(thread() == QThread::currentThread());
    const screen_window_t windowHandle = reinterpret_cast<screen_window_t>(window);
    removeOverlayWindow(windowHandle);
}

void QQnxScreen::activateWindowGroup(const QByteArray &id)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!rootWindow() || id != rootWindow()->groupName())
        return;

    if (!m_childWindows.isEmpty()) {
        // We're picking up the last window of the list here
        // because this list is ordered by stacking order.
        // Last window is effectively the one on top.
        QWindow * const window = m_childWindows.last()->window();
        QWindowSystemInterface::handleWindowActivated(window);
    }
}

void QQnxScreen::deactivateWindowGroup(const QByteArray &id)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (!rootWindow() || id != rootWindow()->groupName())
        return;

    QWindowSystemInterface::handleWindowActivated(0);
}

QT_END_NAMESPACE
