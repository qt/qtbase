/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#include "qeglfshooks.h"
#include "qeglfscursor.h"

#include <QtDebug>

#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>

#include <bcm_host.h>

QT_BEGIN_NAMESPACE

static DISPMANX_DISPLAY_HANDLE_T dispman_display = 0;

static EGLNativeWindowType createDispmanxLayer(const QPoint &pos, const QSize &size, int z, DISPMANX_FLAGS_ALPHA_T flags)
{
    VC_RECT_T dst_rect;
    dst_rect.x = pos.x();
    dst_rect.y = pos.y();
    dst_rect.width = size.width();
    dst_rect.height = size.height();

    VC_RECT_T src_rect;
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = size.width() << 16;
    src_rect.height = size.height() << 16;

    DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);

    VC_DISPMANX_ALPHA_T alpha;
    alpha.flags = flags;
    alpha.opacity = 0xFF;
    alpha.mask = 0;

    DISPMANX_ELEMENT_HANDLE_T dispman_element = vc_dispmanx_element_add(
            dispman_update, dispman_display, z, &dst_rect, 0, &src_rect,
            DISPMANX_PROTECTION_NONE, &alpha, (DISPMANX_CLAMP_T *)NULL, (DISPMANX_TRANSFORM_T)0);

    vc_dispmanx_update_submit_sync(dispman_update);

    EGL_DISPMANX_WINDOW_T *eglWindow = new EGL_DISPMANX_WINDOW_T;
    eglWindow->element = dispman_element;
    eglWindow->width = size.width();
    eglWindow->height = size.height();

    return eglWindow;
}

// this function is not part of debian squeeze headers
extern "C" int VCHPOST_ vc_dispmanx_element_change_attributes(DISPMANX_UPDATE_HANDLE_T update,
    DISPMANX_ELEMENT_HANDLE_T element, uint32_t change_flags, int32_t layer,
    uint8_t opacity, const VC_RECT_T *dest_rect, const VC_RECT_T *src_rect,
    DISPMANX_RESOURCE_HANDLE_T mask, VC_IMAGE_TRANSFORM_T transform);

// these constants are not in any headers (yet)
#define ELEMENT_CHANGE_LAYER          (1<<0)
#define ELEMENT_CHANGE_OPACITY        (1<<1)
#define ELEMENT_CHANGE_DEST_RECT      (1<<2)
#define ELEMENT_CHANGE_SRC_RECT       (1<<3)
#define ELEMENT_CHANGE_MASK_RESOURCE  (1<<4)
#define ELEMENT_CHANGE_TRANSFORM      (1<<5)

static void moveDispmanxLayer(EGLNativeWindowType window, const QPoint &pos)
{
    EGL_DISPMANX_WINDOW_T *eglWindow = static_cast<EGL_DISPMANX_WINDOW_T *>(window);
    QSize size(eglWindow->width, eglWindow->height);

    VC_RECT_T dst_rect;
    dst_rect.x = pos.x();
    dst_rect.y = pos.y();
    dst_rect.width = size.width();
    dst_rect.height = size.height();

    VC_RECT_T src_rect;
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = size.width() << 16;
    src_rect.height = size.height() << 16;

    DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_attributes(dispman_update,
                                          eglWindow->element,
                                          ELEMENT_CHANGE_DEST_RECT /*change_flags*/,
                                          0,
                                          0,
                                          &dst_rect,
                                          NULL,
                                          0,
                                          (VC_IMAGE_TRANSFORM_T)0);

    vc_dispmanx_update_submit_sync(dispman_update);
}

static void destroyDispmanxLayer(EGLNativeWindowType window)
{
    EGL_DISPMANX_WINDOW_T *eglWindow = static_cast<EGL_DISPMANX_WINDOW_T *>(window);
    DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);
    vc_dispmanx_element_remove(dispman_update, eglWindow->element);
    vc_dispmanx_update_submit_sync(dispman_update);
    delete eglWindow;
}

class QEglFSPiCursor : public QEglFSCursor
{
public:
    QEglFSPiCursor(QEglFSScreen *screen) : QEglFSCursor(screen) {
        QSurfaceFormat platformFormat;
        platformFormat.setDepthBufferSize(24);
        platformFormat.setStencilBufferSize(8);
        platformFormat.setRedBufferSize(8);
        platformFormat.setGreenBufferSize(8);
        platformFormat.setBlueBufferSize(8);
        platformFormat.setAlphaBufferSize(8);
        m_config = q_configFromGLFormat(m_screen->display(), platformFormat);

        createSurface();
        createContext();
        drawInLayer();
    }

    ~QEglFSPiCursor() {
        eglDestroySurface(m_screen->display(), m_surface);
        destroyDispmanxLayer(m_window);
        eglDestroyContext(m_screen->display(), m_context);
    }

    void createSurface() {
        const QRect cr = cursorRect();
        m_window = createDispmanxLayer(cr.topLeft(), cr.size(), 50, DISPMANX_FLAGS_ALPHA_FROM_SOURCE);
        m_surface = eglCreateWindowSurface(m_screen->display(), m_config, m_window, NULL);
    }

    void createContext() {
        eglBindAPI(EGL_OPENGL_ES_API);
        QVector<EGLint> attrs;
        attrs.append(EGL_CONTEXT_CLIENT_VERSION);
        attrs.append(2);
        attrs.append(EGL_NONE);
        m_context = eglCreateContext(m_screen->display(), m_config, EGL_NO_CONTEXT, attrs.constData());
    }

    void drawInLayer() {
        eglMakeCurrent(m_screen->display(), m_surface, m_surface, m_context);

        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        draw(QRectF(QPointF(-1, 1), QPointF(1, -1)));

        eglSwapBuffers(m_screen->display(), m_surface);
        eglMakeCurrent(m_screen->display(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    void changeCursor(QCursor *cursor, QWindow *window) Q_DECL_OVERRIDE {
        if (!setCurrentCursor(cursor))
            return;

        EGL_DISPMANX_WINDOW_T *eglWindow = static_cast<EGL_DISPMANX_WINDOW_T *>(m_window);
        if (QSize(eglWindow->width, eglWindow->height) != m_cursor.size) {
            eglDestroySurface(m_screen->display(), m_surface);
            destroyDispmanxLayer(m_window);
            createSurface();
        }
        drawInLayer();
    }

    void setPos(const QPoint &pos) Q_DECL_OVERRIDE {
        m_cursor.pos = pos;
        moveDispmanxLayer(m_window, cursorRect().topLeft());
    }

    void pointerEvent(const QMouseEvent &event) Q_DECL_OVERRIDE {
        if (event.type() != QEvent::MouseMove)
            return;
        m_cursor.pos = event.pos();
        moveDispmanxLayer(m_window, cursorRect().topLeft());
    }

    void paintOnScreen() Q_DECL_OVERRIDE { }
private:
    EGLConfig m_config;
    EGLContext m_context;
    EGLNativeWindowType m_window;
    EGLSurface m_surface;
};

class QEglFSPiHooks : public QEglFSHooks
{
public:
    virtual void platformInit();
    virtual void platformDestroy();
    virtual EGLNativeDisplayType platformDisplay() const;
    virtual QSize screenSize() const;
    virtual EGLNativeWindowType createNativeWindow(const QSize &size, const QSurfaceFormat &format);
    virtual void destroyNativeWindow(EGLNativeWindowType window);
    virtual bool hasCapability(QPlatformIntegration::Capability cap) const;

    QEglFSCursor *createCursor(QEglFSScreen *screen) const {
        return new QEglFSPiCursor(screen);
    }
};

void QEglFSPiHooks::platformInit()
{
    bcm_host_init();
}

EGLNativeDisplayType QEglFSPiHooks::platformDisplay() const
{
    dispman_display = vc_dispmanx_display_open(0/* LCD */);
    return EGL_DEFAULT_DISPLAY;
}

void QEglFSPiHooks::platformDestroy()
{
    vc_dispmanx_display_close(dispman_display);
}

QSize QEglFSPiHooks::screenSize() const
{
    uint32_t width, height;
    graphics_get_display_size(0 /* LCD */, &width, &height);
    return QSize(width, height);
}

EGLNativeWindowType QEglFSPiHooks::createNativeWindow(const QSize &size, const QSurfaceFormat &format)
{
    return createDispmanxLayer(QPoint(0, 0), size, 1, format.hasAlpha() ? DISPMANX_FLAGS_ALPHA_FROM_SOURCE : DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS);
}

void QEglFSPiHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    destroyDispmanxLayer(window);
}

bool QEglFSPiHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
        case QPlatformIntegration::ThreadedPixmaps:
        case QPlatformIntegration::OpenGL:
        case QPlatformIntegration::ThreadedOpenGL:
        case QPlatformIntegration::BufferQueueingOpenGL:
            return true;
        default:
            return false;
    }
}

QEglFSPiHooks eglFSPiHooks;
QEglFSHooks *platformHooks = &eglFSPiHooks;

QT_END_NAMESPACE
