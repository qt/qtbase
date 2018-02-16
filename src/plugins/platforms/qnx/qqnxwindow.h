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

#ifndef QQNXWINDOW_H
#define QQNXWINDOW_H

#include <qpa/qplatformwindow.h>
#include "qqnxabstractcover.h"

#include <QtCore/QScopedPointer>

#if !defined(QT_NO_OPENGL)
#include <EGL/egl.h>
#endif

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

// all surfaces double buffered
#define MAX_BUFFER_COUNT    2

class QQnxScreen;

class QSurfaceFormat;

class QQnxWindow : public QPlatformWindow
{
friend class QQnxScreen;
public:
    QQnxWindow(QWindow *window, screen_context_t context, bool needRootWindow);
    virtual ~QQnxWindow();

    void setGeometry(const QRect &rect) override;
    void setVisible(bool visible) override;
    void setOpacity(qreal level) override;

    bool isExposed() const override;

    WId winId() const override { return window()->type() == Qt::Desktop ? -1 : (WId)m_window; }
    screen_window_t nativeHandle() const { return m_window; }

    void setBufferSize(const QSize &size);
    QSize bufferSize() const { return m_bufferSize; }

    void setScreen(QQnxScreen *platformScreen);

    void setParent(const QPlatformWindow *window) override;
    void raise() override;
    void lower() override;
    void requestActivateWindow() override;
    void setWindowState(Qt::WindowStates state) override;
    void setExposed(bool exposed);

    void propagateSizeHints() override;

    void setMMRendererWindowName(const QString &name);
    void setMMRendererWindow(screen_window_t handle);
    void clearMMRendererWindow();

    QPlatformScreen *screen() const override;
    const QList<QQnxWindow*>& children() const { return m_childWindows; }

    QQnxWindow *findWindow(screen_window_t windowHandle);

    void minimize();

    QString mmRendererWindowName() const { return m_mmRendererWindowName; }

    screen_window_t mmRendererWindow() const { return m_mmRendererWindow; }

    void setRotation(int rotation);

    QByteArray groupName() const { return m_windowGroupName; }
    void joinWindowGroup(const QByteArray &groupName);

    bool shouldMakeFullScreen() const;

    void windowPosted();

protected:
    virtual int pixelFormat() const = 0;
    virtual void resetBuffers() = 0;

    void initWindow();

    screen_context_t m_screenContext;

private:
    void createWindowGroup();
    void setGeometryHelper(const QRect &rect);
    void removeFromParent();
    void updateVisibility(bool parentVisible);
    void updateZorder(int &topZorder);
    void updateZorder(screen_window_t window, int &zOrder);
    void applyWindowState();
    void setFocus(screen_window_t newFocusWindow);

    screen_window_t m_window;
    QSize m_bufferSize;

    QQnxScreen *m_screen;
    QQnxWindow *m_parentWindow;
    QList<QQnxWindow*> m_childWindows;
    QScopedPointer<QQnxAbstractCover> m_cover;
    bool m_visible;
    bool m_exposed;
    QRect m_unmaximizedGeometry;
    Qt::WindowStates m_windowState;
    QString m_mmRendererWindowName;
    screen_window_t m_mmRendererWindow;

    // Group name of window group headed by this window
    QByteArray m_windowGroupName;
    // Group name that we have joined or "" if we've not joined any group.
    QByteArray m_parentGroupName;

    bool m_isTopLevel;
};

QT_END_NAMESPACE

#endif // QQNXWINDOW_H
