/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWAYLANDDISPLAY_H
#define QWAYLANDDISPLAY_H

#include <QtCore/QObject>
#include <QtCore/QRect>

#include <QtCore/QWaitCondition>

#include <wayland-client.h>

class QWaylandInputDevice;
class QSocketNotifier;
class QWaylandBuffer;
class QPlatformScreen;
class QWaylandScreen;
class QWaylandGLIntegration;
class QWaylandWindowManagerIntegration;

class QWaylandDisplay : public QObject {
    Q_OBJECT

public:
    QWaylandDisplay(void);
    ~QWaylandDisplay(void);

    QList<QPlatformScreen *> screens() const { return mScreens; }
    struct wl_surface *createSurface(void *handle);
    struct wl_buffer *createShmBuffer(int fd, int width, int height,
                                      uint32_t stride,
                                      struct wl_visual *visual);
    struct wl_visual *rgbVisual();
    struct wl_visual *argbVisual();
    struct wl_visual *argbPremultipliedVisual();

#ifdef QT_WAYLAND_GL_SUPPORT
    QWaylandGLIntegration *eglIntegration();
#endif

#ifdef QT_WAYLAND_WINDOWMANAGER_SUPPORT
    QWaylandWindowManagerIntegration *windowManagerIntegration();
#endif

    void setCursor(QWaylandBuffer *buffer, int32_t x, int32_t y);

    void syncCallback(wl_display_sync_func_t func, void *data);
    void frameCallback(wl_display_frame_func_t func, struct wl_surface *surface, void *data);

    struct wl_display *wl_display() const { return mDisplay; }
    struct wl_shell *wl_shell() const { return mShell; }

    QList<QWaylandInputDevice *> inputDevices() const { return mInputDevices; }

public slots:
    void createNewScreen(struct wl_output *output, QRect geometry);
    void readEvents();
    void blockingReadEvents();
    void flushRequests();

private:
    void waitForScreens();
    void displayHandleGlobal(uint32_t id,
                             const QByteArray &interface,
                             uint32_t version);

    struct wl_display *mDisplay;
    struct wl_compositor *mCompositor;
    struct wl_shm *mShm;
    struct wl_shell *mShell;
    QList<QPlatformScreen *> mScreens;
    QList<QWaylandInputDevice *> mInputDevices;

    QSocketNotifier *mReadNotifier;
    int mFd;
    bool mScreensInitialized;

    uint32_t mSocketMask;

    struct wl_visual *argb_visual, *premultiplied_argb_visual, *rgb_visual;

    static const struct wl_output_listener outputListener;
    static const struct wl_compositor_listener compositorListener;
    static int sourceUpdate(uint32_t mask, void *data);
    static void displayHandleGlobal(struct wl_display *display,
                                    uint32_t id,
                                    const char *interface,
                                    uint32_t version, void *data);
    static void outputHandleGeometry(void *data,
                                     struct wl_output *output,
                                     int32_t x, int32_t y,
                                     int32_t width, int32_t height,
                                     int subpixel,
                                     const char *make,
                                     const char *model);
    static void mode(void *data,
                     struct wl_output *wl_output,
                     uint32_t flags,
                     int width,
                     int height,
                     int refresh);

    static void handleVisual(void *data,
                                       struct wl_compositor *compositor,
                                       uint32_t id, uint32_t token);
#ifdef QT_WAYLAND_GL_SUPPORT
    QWaylandGLIntegration *mEglIntegration;
#endif

#ifdef QT_WAYLAND_WINDOWMANAGER_SUPPORT
    QWaylandWindowManagerIntegration *mWindowManagerIntegration;
#endif

    static void shellHandleConfigure(void *data, struct wl_shell *shell,
                                     uint32_t time, uint32_t edges,
                                     struct wl_surface *surface,
                                     int32_t width, int32_t height);

    static const struct wl_shell_listener shellListener;
};

#endif // QWAYLANDDISPLAY_H
