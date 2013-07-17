/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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
#if !defined(QT_NO_OPENGL)
#include "qqnxglcontext.h"
#endif
#include "qqnxintegration.h"
#include "qqnxscreen.h"

#include <QtGui/QWindow>
#include <qpa/qwindowsysteminterface.h>

#include <QtCore/QDebug>

#include <errno.h>

#if defined(Q_OS_BLACKBERRY)
#include <sys/pps.h>
#include <bps/navigator.h>
#endif

#if defined(QQNXWINDOW_DEBUG)
#define qWindowDebug qDebug
#else
#define qWindowDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxWindow::QQnxWindow(QWindow *window, screen_context_t context)
    : QPlatformWindow(window),
      m_screenContext(context),
      m_window(0),
      m_currentBufferIndex(-1),
      m_previousBufferIndex(-1),
#if !defined(QT_NO_OPENGL)
      m_platformOpenGLContext(0),
#endif
      m_screen(0),
      m_parentWindow(0),
      m_visible(false),
      m_windowState(Qt::WindowNoState),
      m_requestedBufferSize(window->geometry().size())
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window << ", size =" << window->size();
    int result;

    // Create child QNX window
    errno = 0;
    result = screen_create_window_type(&m_window, m_screenContext, SCREEN_CHILD_WINDOW);
    if (result != 0)
        qFatal("QQnxWindow: failed to create window, errno=%d", errno);

    // Set window buffer usage based on rendering API
    int val;
    QSurface::SurfaceType surfaceType = window->surfaceType();
    switch (surfaceType) {
    case QSurface::RasterSurface:
        val = SCREEN_USAGE_NATIVE | SCREEN_USAGE_READ | SCREEN_USAGE_WRITE;
        break;
    case QSurface::OpenGLSurface:
        val = SCREEN_USAGE_OPENGL_ES2;
        break;
    default:
        qFatal("QQnxWindow: unsupported window API");
        break;
    }

    errno = 0;
    result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_USAGE, &val);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window buffer usage, errno=%d", errno);

    // Alpha channel is always pre-multiplied if present
    errno = 0;
    val = SCREEN_PRE_MULTIPLIED_ALPHA;
    result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_ALPHA_MODE, &val);
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

    if (window->flags() & Qt::WindowDoesNotAcceptFocus) {
        errno = 0;
        val = SCREEN_SENSITIVITY_NO_FOCUS;
        result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_SENSITIVITY, &val);
        if (result != 0)
            qFatal("QQnxWindow: failed to set window sensitivity, errno=%d", errno);
    }

    setScreen(static_cast<QQnxScreen *>(window->screen()->handle()));

    // Add window to plugin's window mapper
    QQnxIntegration::addWindow(m_window, window);

    // Qt never calls these setters after creating the window, so we need to do that ourselves here
    setWindowState(window->windowState());
    if (window->parent() && window->parent()->handle())
        setParent(window->parent()->handle());
    setGeometryHelper(window->geometry());
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
    const QRect oldGeometry = setGeometryHelper(rect);

#if !defined(QT_NO_OPENGL)
    // If this is an OpenGL window we need to request that the GL context updates
    // the EGLsurface on which it is rendering. The surface will be recreated the
    // next time QQnxGLContext::makeCurrent() is called.
    {
        // We want the setting of the atomic bool in the GL context to be atomic with
        // setting m_requestedBufferSize and therefore extended the scope to include
        // that test.
        const QMutexLocker locker(&m_mutex);
        m_requestedBufferSize = rect.size();
        if (m_platformOpenGLContext != 0 && bufferSize() != rect.size())
            m_platformOpenGLContext->requestSurfaceChange();
    }
#endif

    // Send a geometry change event to Qt (triggers resizeEvent() in QWindow/QWidget).

    // Calling flushWindowSystemEvents() here would flush input events which
    // could result in re-entering QQnxWindow::setGeometry() again.
    QWindowSystemInterface::setSynchronousWindowsSystemEvents(true);
    QWindowSystemInterface::handleGeometryChange(window(), rect);
    QWindowSystemInterface::handleExposeEvent(window(), rect);
    QWindowSystemInterface::setSynchronousWindowsSystemEvents(false);

    // Now move all children.
    if (!oldGeometry.isEmpty()) {
        const QPoint offset = rect.topLeft() - oldGeometry.topLeft();
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

bool QQnxWindow::isExposed() const
{
    return m_visible;
}

QSize QQnxWindow::requestedBufferSize() const
{
    const QMutexLocker locker(&m_mutex);
    return m_requestedBufferSize;
}

void QQnxWindow::adjustBufferSize()
{
    const QSize windowSize = window()->size();
    if (windowSize != bufferSize())
        setBufferSize(windowSize);
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
        val[0] = m_screen->nativeFormat();
#if !defined(QT_NO_OPENGL)
        // Get pixel format from EGL config if using OpenGL;
        // otherwise inherit pixel format of window's screen
        if (m_platformOpenGLContext != 0)
            val[0] = platformWindowFormatToNativeFormat(m_platformOpenGLContext->format());
#endif

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

        // If the child window has been configured for transparency, lazily create
        // a full-screen buffer to back the root window.
        if (window()->requestedFormat().hasAlpha()) {
            m_screen->rootWindow()->makeTranslucent();
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

    // Buffers were destroyed; reacquire them
    m_currentBufferIndex = -1;
    m_previousDirty = QRegion();
    m_scrolled = QRegion();

    const QMutexLocker locker(&m_mutex);
    m_requestedBufferSize = QSize();
}

QQnxBuffer &QQnxWindow::renderBuffer()
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();

    // Check if render buffer is invalid
    if (m_currentBufferIndex == -1) {
        // Get all buffers available for rendering
        errno = 0;
        screen_buffer_t buffers[MAX_BUFFER_COUNT];
        const int result = screen_get_window_property_pv(m_window, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)buffers);
        if (result != 0)
            qFatal("QQnxWindow: failed to query window buffers, errno=%d", errno);

        // Wrap each buffer
        for (int i = 0; i < MAX_BUFFER_COUNT; ++i) {
            m_buffers[i] = QQnxBuffer(buffers[i]);
        }

        // Use the first available render buffer
        m_currentBufferIndex = 0;
        m_previousBufferIndex = -1;
    }

    return m_buffers[m_currentBufferIndex];
}

void QQnxWindow::scroll(const QRegion &region, int dx, int dy, bool flush)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();
    blitPreviousToCurrent(region, dx, dy, flush);
    m_scrolled += region;
}

void QQnxWindow::post(const QRegion &dirty)
{
    // How double-buffering works
    // --------------------------
    //
    // The are two buffers, the previous one and the current one.
    // The previous buffer always contains the complete, full image of the whole window when it
    // was last posted.
    // The current buffer starts with the complete, full image of the second to last posting
    // of the window.
    //
    // During painting, Qt paints on the current buffer. Thus, when Qt has finished painting, the
    // current buffer contains the second to last image plus the newly painted regions.
    // Since the second to last image is too old, we copy over the image from the previous buffer, but
    // only for those regions that Qt didn't paint (because that would overwrite what Qt has just
    // painted). This is the copyPreviousToCurrent() call below.
    //
    // After the call to copyPreviousToCurrent(), the current buffer contains the complete, full image of the
    // whole window in its current state, and we call screen_post_window() to make the new buffer
    // available to libscreen (called "posting"). There, only the regions that Qt painted on are
    // posted, as nothing else has changed.
    //
    // After that, the previous and the current buffers are swapped, and the whole cycle starts anew.

    // Check if render buffer exists and something was rendered
    if (m_currentBufferIndex != -1 && !dirty.isEmpty()) {
        qWindowDebug() << Q_FUNC_INFO << "window =" << window();
        QQnxBuffer &currentBuffer = m_buffers[m_currentBufferIndex];

        // Copy unmodified region from old render buffer to new render buffer;
        // required to allow partial updates
        QRegion preserve = m_previousDirty - dirty - m_scrolled;
        blitPreviousToCurrent(preserve, 0, 0);

        // Calculate region that changed
        QRegion modified = preserve + dirty + m_scrolled;
        QRect rect = modified.boundingRect();
        int dirtyRect[4] = { rect.x(), rect.y(), rect.x() + rect.width(), rect.y() + rect.height() };

        // Update the display with contents of render buffer
        errno = 0;
        int result = screen_post_window(m_window, currentBuffer.nativeBuffer(), 1, dirtyRect, 0);
        if (result != 0)
            qFatal("QQnxWindow: failed to post window buffer, errno=%d", errno);

        // Advance to next nender buffer
        m_previousBufferIndex = m_currentBufferIndex++;
        if (m_currentBufferIndex >= MAX_BUFFER_COUNT)
            m_currentBufferIndex = 0;

        // Save modified region and clear scrolled region
        m_previousDirty = dirty;
        m_scrolled = QRegion();

        // Notify screen that window posted
        if (m_screen != 0)
            m_screen->onWindowPost(this);
    }
}

void QQnxWindow::setScreen(QQnxScreen *platformScreen)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window() << "platformScreen =" << platformScreen;

    if (platformScreen == 0) { // The screen has been destroyed
        m_screen = 0;
        return;
    }

    if (m_screen == platformScreen)
        return;

    if (m_screen)
        m_screen->removeWindow(this);
    platformScreen->addWindow(this);
    m_screen = platformScreen;

    // Move window to proper screen/display
    errno = 0;
    screen_display_t display = platformScreen->nativeDisplay();
    int result = screen_set_window_property_pv(m_window, SCREEN_PROPERTY_DISPLAY, (void **)&display);
    if (result != 0)
        qFatal("QQnxWindow: failed to set window display, errno=%d", errno);

    // Add window to display's window group
    errno = 0;
    result = screen_join_window_group(m_window, platformScreen->windowGroupName());
    if (result != 0)
        qFatal("QQnxWindow: failed to join window group, errno=%d", errno);

    Q_FOREACH (QQnxWindow *childWindow, m_childWindows) {
        // Only subwindows and tooltips need necessarily be moved to another display with the window.
        if ((window()->type() & Qt::WindowType_Mask) == Qt::SubWindow ||
            (window()->type() & Qt::WindowType_Mask) == Qt::ToolTip)
            childWindow->setScreen(platformScreen);
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

void QQnxWindow::gainedFocus()
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();

    // Got focus
    QWindowSystemInterface::handleWindowActivated(window());
}

#if !defined(QT_NO_OPENGL)
void QQnxWindow::setPlatformOpenGLContext(QQnxGLContext *platformOpenGLContext)
{
    // This function does not take ownership of the platform gl context.
    // It is owned by the frontend QOpenGLContext
    m_platformOpenGLContext = platformOpenGLContext;
}
#endif

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

void QQnxWindow::blitFrom(QQnxWindow *sourceWindow, const QPoint &sourceOffset, const QRegion &targetRegion)
{
    if (!sourceWindow || sourceWindow->m_previousBufferIndex == -1 || targetRegion.isEmpty())
        return;

    qWindowDebug() << Q_FUNC_INFO << window() << sourceWindow->window() << sourceOffset << targetRegion;

    QQnxBuffer &sourceBuffer = sourceWindow->m_buffers[sourceWindow->m_previousBufferIndex];
    QQnxBuffer &targetBuffer = renderBuffer();

    blitHelper(sourceBuffer, targetBuffer, sourceOffset, QPoint(0, 0), targetRegion, true);
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

void QQnxWindow::updateZorder(int &topZorder)
{
    errno = 0;
    int result = screen_set_window_property_iv(m_window, SCREEN_PROPERTY_ZORDER, &topZorder);
    topZorder++;

    if (result != 0)
        qFatal("QQnxWindow: failed to set window z-order=%d, errno=%d, mWindow=%p", topZorder, errno, m_window);

    Q_FOREACH (QQnxWindow *childWindow, m_childWindows)
        childWindow->updateZorder(topZorder);
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

void QQnxWindow::blitHelper(QQnxBuffer &source, QQnxBuffer &target, const QPoint &sourceOffset,
                            const QPoint &targetOffset, const QRegion &region, bool flush)
{
    // Break down region into non-overlapping rectangles
    const QVector<QRect> rects = region.rects();
    for (int i = rects.size() - 1; i >= 0; i--) {
        // Clip rectangle to bounds of target
        const QRect rect = rects[i].intersected(target.rect());

        if (rect.isEmpty())
            continue;

        // Setup blit operation
        int attribs[] = { SCREEN_BLIT_SOURCE_X, rect.x() + sourceOffset.x(),
                          SCREEN_BLIT_SOURCE_Y, rect.y() + sourceOffset.y(),
                          SCREEN_BLIT_SOURCE_WIDTH, rect.width(),
                          SCREEN_BLIT_SOURCE_HEIGHT, rect.height(),
                          SCREEN_BLIT_DESTINATION_X, rect.x() + targetOffset.x(),
                          SCREEN_BLIT_DESTINATION_Y, rect.y() + targetOffset.y(),
                          SCREEN_BLIT_DESTINATION_WIDTH, rect.width(),
                          SCREEN_BLIT_DESTINATION_HEIGHT, rect.height(),
                          SCREEN_BLIT_END };

        // Queue blit operation
        errno = 0;
        const int result = screen_blit(m_screenContext, target.nativeBuffer(),
                                       source.nativeBuffer(), attribs);
        if (result != 0)
            qFatal("QQnxWindow: failed to blit buffers, errno=%d", errno);
    }

    // Check if flush requested
    if (flush) {
        // Wait for all blits to complete
        errno = 0;
        const int result = screen_flush_blits(m_screenContext, SCREEN_WAIT_IDLE);
        if (result != 0)
            qFatal("QQnxWindow: failed to flush blits, errno=%d", errno);

        // Buffer was modified outside the CPU
        target.invalidateInCache();
    }
}

void QQnxWindow::blitPreviousToCurrent(const QRegion &region, int dx, int dy, bool flush)
{
    qWindowDebug() << Q_FUNC_INFO << "window =" << window();

    // Abort if previous buffer is invalid or if nothing to copy
    if (m_previousBufferIndex == -1 || region.isEmpty())
        return;

    QQnxBuffer &currentBuffer = m_buffers[m_currentBufferIndex];
    QQnxBuffer &previousBuffer = m_buffers[m_previousBufferIndex];

    blitHelper(previousBuffer, currentBuffer, QPoint(0, 0), QPoint(dx, dy), region, flush);
}

int QQnxWindow::platformWindowFormatToNativeFormat(const QSurfaceFormat &format)
{
    qWindowDebug() << Q_FUNC_INFO;
    // Extract size of colour channels from window format
    int redSize = format.redBufferSize();
    if (redSize == -1)
        qFatal("QQnxWindow: red size not defined");

    int greenSize = format.greenBufferSize();
    if (greenSize == -1)
        qFatal("QQnxWindow: green size not defined");

    int blueSize = format.blueBufferSize();
    if (blueSize == -1)
        qFatal("QQnxWindow: blue size not defined");

    // select matching native format
    if (redSize == 5 && greenSize == 6 && blueSize == 5) {
        return SCREEN_FORMAT_RGB565;
    } else if (redSize == 8 && greenSize == 8 && blueSize == 8) {
        return SCREEN_FORMAT_RGBA8888;
    } else {
        qFatal("QQnxWindow: unsupported pixel format");
        return 0;
    }
}

QT_END_NAMESPACE
