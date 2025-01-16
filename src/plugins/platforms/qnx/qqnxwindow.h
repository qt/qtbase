// Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXWINDOW_H
#define QQNXWINDOW_H

#include <qpa/qplatformwindow.h>
#include "qqnxabstractcover.h"

#include <QtCore/QScopedPointer>
#include <QtCore/QLoggingCategory>

#if !defined(QT_NO_OPENGL)
#include <EGL/egl.h>
#endif

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindow);

// all surfaces double buffered
#define MAX_BUFFER_COUNT    2

class QQnxScreen;

class QSurfaceFormat;

class QQnxWindow : public QPlatformWindow
{
friend class QQnxScreen;
public:
    explicit QQnxWindow(QWindow *window, screen_context_t context, bool needRootWindow);
    explicit QQnxWindow(QWindow *window, screen_context_t context, screen_window_t screenWindow);
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

    QPlatformScreen *screen() const override;
    const QList<QQnxWindow*>& children() const { return m_childWindows; }

    QQnxWindow *findWindow(screen_window_t windowHandle);

    void minimize();

    void setRotation(int rotation);

    QByteArray groupName() const { return m_windowGroupName; }
    void joinWindowGroup(const QByteArray &groupName);

    bool shouldMakeFullScreen() const;

    void windowPosted();
    void handleActivationEvent();

    void setWindowTitle(const QString &title);

protected:
    virtual int pixelFormat() const = 0;
    virtual void resetBuffers() = 0;

    void initWindow();

    screen_context_t m_screenContext;

private:
    void collectWindowGroup();
    void createWindowGroup();
    void setGeometryHelper(const QRect &rect);
    void removeFromParent();
    void updateVisibility(bool parentVisible);
    void updateZorder(int &topZorder);
    void updateZorder(screen_window_t window, int &zOrder);
    void applyWindowState();
    void setFocus(screen_window_t newFocusWindow);
    bool showWithoutActivating() const;
    bool focusable() const;
    void notifyManager(const QString &msg);

    void addContextPermission();
    void removeContextPermission();

    screen_window_t m_window;
    QSize m_bufferSize;

    QQnxScreen *m_screen;
    QQnxWindow *m_parentWindow;
    QList<QQnxWindow*> m_childWindows;
    QScopedPointer<QQnxAbstractCover> m_cover;
    bool m_visible;
    bool m_exposed;
    bool m_foreign;
    QRect m_unmaximizedGeometry;
    Qt::WindowStates m_windowState;

    // Group name of window group headed by this window
    QByteArray m_windowGroupName;
    // Group name that we have joined or "" if we've not joined any group.
    QByteArray m_parentGroupName;

    bool m_isTopLevel;
    bool m_firstActivateHandled;
    int m_desktopNotify;

    enum {
        DesktopNotifyTitle = 0x1,
        DesktopNotifyPosition = 0x2,
        DesktopNotifyVisible = 0x2
    };
};

QT_END_NAMESPACE

#endif // QQNXWINDOW_H
