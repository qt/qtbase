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

#ifndef QQNXWINDOW_H
#define QQNXWINDOW_H

#include <qpa/qplatformwindow.h>

#include "qqnxbuffer.h"

#include <QtGui/QImage>
#include <QtCore/QMutex>

#if !defined(QT_NO_OPENGL)
#include <EGL/egl.h>
#endif

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

// all surfaces double buffered
#define MAX_BUFFER_COUNT    2

#if !defined(QT_NO_OPENGL)
class QQnxGLContext;
#endif
class QQnxScreen;

class QSurfaceFormat;

class QQnxWindow : public QPlatformWindow
{
friend class QQnxScreen;
public:
    QQnxWindow(QWindow *window, screen_context_t context);
    virtual ~QQnxWindow();

    void setGeometry(const QRect &rect);
    void setVisible(bool visible);
    void setOpacity(qreal level);

    bool isExposed() const;

    WId winId() const { return (WId)m_window; }
    screen_window_t nativeHandle() const { return m_window; }

    // Called by QQnxGLContext::createSurface()
    QSize requestedBufferSize() const;

    void adjustBufferSize();
    void setBufferSize(const QSize &size);
    QSize bufferSize() const { return m_bufferSize; }
    bool hasBuffers() const { return !m_bufferSize.isEmpty(); }

    QQnxBuffer &renderBuffer();
    void scroll(const QRegion &region, int dx, int dy, bool flush=false);
    void post(const QRegion &dirty);

    void setScreen(QQnxScreen *platformScreen);

    void setParent(const QPlatformWindow *window);
    void raise();
    void lower();
    void requestActivateWindow();
    void setWindowState(Qt::WindowState state);

    void gainedFocus();

    QQnxScreen *screen() const { return m_screen; }
    const QList<QQnxWindow*>& children() const { return m_childWindows; }

#if !defined(QT_NO_OPENGL)
    void setPlatformOpenGLContext(QQnxGLContext *platformOpenGLContext);
    QQnxGLContext *platformOpenGLContext() const { return m_platformOpenGLContext; }
#endif

    QQnxWindow *findWindow(screen_window_t windowHandle);

    void blitFrom(QQnxWindow *sourceWindow, const QPoint &sourceOffset, const QRegion &targetRegion);
    void minimize();

private:
    QRect setGeometryHelper(const QRect &rect);
    void removeFromParent();
    void setOffset(const QPoint &setOffset);
    void updateVisibility(bool parentVisible);
    void updateZorder(int &topZorder);
    void applyWindowState();

    void fetchBuffers();

    // Copies content from the previous buffer (back buffer) to the current buffer (front buffer)
    void blitPreviousToCurrent(const QRegion &region, int dx, int dy, bool flush=false);

    void blitHelper(QQnxBuffer &source, QQnxBuffer &target, const QPoint &sourceOffset,
                    const QPoint &targetOffset, const QRegion &region, bool flush = false);

    static int platformWindowFormatToNativeFormat(const QSurfaceFormat &format);

    screen_context_t m_screenContext;
    screen_window_t m_window;
    QSize m_bufferSize;
    QQnxBuffer m_buffers[MAX_BUFFER_COUNT];
    int m_currentBufferIndex;
    int m_previousBufferIndex;
    QRegion m_previousDirty;
    QRegion m_scrolled;

#if !defined(QT_NO_OPENGL)
    QQnxGLContext *m_platformOpenGLContext;
#endif
    QQnxScreen *m_screen;
    QList<QQnxWindow*> m_childWindows;
    QQnxWindow *m_parentWindow;
    bool m_visible;
    QRect m_unmaximizedGeometry;
    Qt::WindowState m_windowState;

    // This mutex is used to protect access to the m_requestedBufferSize
    // member. This member is used in conjunction with QQnxGLContext::requestNewSurface()
    // to coordinate recreating the EGL surface which involves destroying any
    // existing EGL surface; resizing the native window buffers; and creating a new
    // EGL surface. All of this has to be done from the thread that is calling
    // QQnxGLContext::makeCurrent()
    mutable QMutex m_mutex;
    QSize m_requestedBufferSize;
};

QT_END_NAMESPACE

#endif // QQNXWINDOW_H
