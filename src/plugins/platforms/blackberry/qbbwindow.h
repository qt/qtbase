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

#ifndef QBBWINDOW_H
#define QBBWINDOW_H

#include <QtGui/QPlatformWindow>

#include "qbbbuffer.h"

#include <QtGui/QImage>

#include <EGL/egl.h>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

// all surfaces double buffered
#define MAX_BUFFER_COUNT    2

class QBBGLContext;
class QBBScreen;

class QPlatformGLContext;
class QSurfaceFormat;

class QBBWindow : public QPlatformWindow
{
friend class QBBScreen;
public:
    QBBWindow(QWindow *window, screen_context_t context);
    virtual ~QBBWindow();

    virtual void setGeometry(const QRect &rect);
    virtual void setVisible(bool visible);
    virtual void setOpacity(qreal level);

    virtual WId winId() const { return (WId)m_window; }
    screen_window_t nativeHandle() const { return m_window; }

    void setBufferSize(const QSize &size);
    QSize bufferSize() const { return m_bufferSize; }
    bool hasBuffers() const { return !m_bufferSize.isEmpty(); }

    QBBBuffer &renderBuffer();
    void scroll(const QRegion &region, int dx, int dy, bool flush=false);
    void post(const QRegion &dirty);

    void setScreen(QBBScreen *platformScreen);

    virtual void setParent(const QPlatformWindow *window);
    virtual void raise();
    virtual void lower();
    virtual void requestActivateWindow();

    void gainedFocus();

    QBBScreen *screen() const { return m_screen; }
    const QList<QBBWindow*>& children() const { return m_childWindows; }

    void setPlatformOpenGLContext(QBBGLContext *platformOpenGLContext);
    QBBGLContext *platformOpenGLContext() const { return m_platformOpenGLContext; }

private:
    void removeFromParent();
    void offset(const QPoint &offset);
    void updateVisibility(bool parentVisible);
    void updateZorder(int &topZorder);

    void fetchBuffers();

    void copyBack(const QRegion &region, int dx, int dy, bool flush=false);

    static int platformWindowFormatToNativeFormat(const QSurfaceFormat &format);

    screen_context_t m_screenContext;
    screen_window_t m_window;
    QSize m_bufferSize;
    QBBBuffer m_buffers[MAX_BUFFER_COUNT];
    int m_currentBufferIndex;
    int m_previousBufferIndex;
    QRegion m_previousDirty;
    QRegion m_scrolled;

    QBBGLContext *m_platformOpenGLContext;
    QBBScreen *m_screen;
    QList<QBBWindow*> m_childWindows;
    QBBWindow *m_parentWindow;
    bool m_visible;
};

QT_END_NAMESPACE

#endif // QBBWINDOW_H
