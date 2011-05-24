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

#define GL_GLEXT_PROTOTYPES
#include "shivavgwindowsurface.h"
#include <QtOpenVG/private/qpaintengine_vg_p.h>
#if defined(Q_WS_X11)
#include "private/qt_x11_p.h"
#include "qx11info_x11.h"
#include <GL/glx.h>

extern QX11Info *qt_x11Info(const QPaintDevice *pd);
#endif

// Define this to use framebuffer objects.
//#define QVG_USE_FBO 1

#include <vg/openvg.h>

QT_BEGIN_NAMESPACE

class QShivaContext
{
public:
    QShivaContext();
    ~QShivaContext();

    bool makeCurrent(ShivaVGWindowSurfacePrivate *surface);
    void doneCurrent();

    bool initialized;
    QSize currentSize;
    ShivaVGWindowSurfacePrivate *currentSurface;
};

Q_GLOBAL_STATIC(QShivaContext, shivaContext);

class ShivaVGWindowSurfacePrivate
{
public:
    ShivaVGWindowSurfacePrivate()
        : isCurrent(false)
        , needsResize(true)
        , engine(0)
#if defined(QVG_USE_FBO)
        , fbo(0)
        , texture(0)
#endif
#if defined(Q_WS_X11)
        , drawable(0)
        , context(0)
#endif
    {
    }
    ~ShivaVGWindowSurfacePrivate();

    void ensureContext(QWidget *widget);

    QSize size;
    bool isCurrent;
    bool needsResize;
    QVGPaintEngine *engine;
#if defined(QVG_USE_FBO)
    GLuint fbo;
    GLuint texture;
#endif
#if defined(Q_WS_X11)
    GLXDrawable drawable;
    GLXContext context;
#endif
};

QShivaContext::QShivaContext()
    : initialized(false)
    , currentSurface(0)
{
}

QShivaContext::~QShivaContext()
{
    if (initialized)
        vgDestroyContextSH();
}

bool QShivaContext::makeCurrent(ShivaVGWindowSurfacePrivate *surface)
{
    if (currentSurface)
        currentSurface->isCurrent = false;
    surface->isCurrent = true;
    currentSurface = surface;
    currentSize = surface->size;
#if defined(Q_WS_X11)
    glXMakeCurrent(X11->display, surface->drawable, surface->context);
#endif
    if (!initialized) {
        if (!vgCreateContextSH(currentSize.width(), currentSize.height())) {
            qWarning("vgCreateContextSH(%d, %d): could not create context", currentSize.width(), currentSize.height());
            return false;
        }
        initialized = true;
    } else {
        vgResizeSurfaceSH(currentSize.width(), currentSize.height());
    }
#if defined(QVG_USE_FBO)
    if (surface->fbo)
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, surface->fbo);
    else
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif
    return true;
}

void QShivaContext::doneCurrent()
{
    if (currentSurface) {
        currentSurface->isCurrent = false;
        currentSurface = 0;
    }
#if defined(Q_WS_X11)
    glXMakeCurrent(X11->display, 0, 0);
#endif
}

ShivaVGWindowSurfacePrivate::~ShivaVGWindowSurfacePrivate()
{
#if defined(QVG_USE_FBO)
    if (fbo) {
        glDeleteTextures(1, &texture);
        glDeleteFramebuffersEXT(1, &fbo);
    }
#endif
}

void ShivaVGWindowSurfacePrivate::ensureContext(QWidget *widget)
{
#if defined(Q_WS_X11)
    Window win = widget->winId();
    if (win != drawable) {
        if (context)
            glXDestroyContext(X11->display, context);
        drawable = win;
    }
    if (context == 0) {
        const QX11Info *xinfo = qt_x11Info(widget);
        int spec[64];
        int i = 0;
        spec[i++] = GLX_DOUBLEBUFFER;
        spec[i++] = GLX_DEPTH_SIZE;
        spec[i++] = 1;
        spec[i++] = GLX_STENCIL_SIZE;
        spec[i++] = 1;
        spec[i++] = GLX_RGBA;
        spec[i++] = GLX_RED_SIZE;
        spec[i++] = 1;
        spec[i++] = GLX_GREEN_SIZE;
        spec[i++] = 1;
        spec[i++] = GLX_BLUE_SIZE;
        spec[i++] = 1;
        spec[i++] = GLX_SAMPLE_BUFFERS_ARB;
        spec[i++] = 1;
        spec[i++] = GLX_SAMPLES_ARB;
        spec[i++] = 4;
        spec[i] = XNone;
        XVisualInfo *visual = glXChooseVisual
            (xinfo->display(), xinfo->screen(), spec);
        context = glXCreateContext(X11->display, visual, 0, True);
        if (!context)
            qWarning("glXCreateContext: could not create GL context for VG rendering");
    }
#else
    Q_UNUSED(widget);
#endif
#if defined(QVG_USE_FBO)
    if (needsResize && fbo) {
#if defined(Q_WS_X11)
        glXMakeCurrent(X11->display, drawable, context);
#endif
        glDeleteTextures(1, &texture);
        glDeleteFramebuffersEXT(1, &fbo);
#if defined(Q_WS_X11)
        glXMakeCurrent(X11->display, 0, 0);
#endif
        fbo = 0;
        texture = 0;
    }
    if (!fbo) {
#if defined(Q_WS_X11)
        glXMakeCurrent(X11->display, drawable, context);
#endif
        glGenFramebuffersEXT(1, &fbo);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.width(), size.height(), 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2DEXT
            (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
             texture, 0);

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#if defined(Q_WS_X11)
        glXMakeCurrent(X11->display, 0, 0);
#endif
    }
#endif
    needsResize = false;
}

ShivaVGWindowSurface::ShivaVGWindowSurface(QWidget *window)
    : QWindowSurface(window), d_ptr(new ShivaVGWindowSurfacePrivate)
{
}

ShivaVGWindowSurface::~ShivaVGWindowSurface()
{
    if (d_ptr->isCurrent) {
        shivaContext()->doneCurrent();
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }
#if defined(Q_WS_X11)
    if (d_ptr->context)
        glXDestroyContext(X11->display, d_ptr->context);
#endif
    delete d_ptr;
}

QPaintDevice *ShivaVGWindowSurface::paintDevice()
{
    d_ptr->ensureContext(window());
    shivaContext()->makeCurrent(d_ptr);
    glClearDepth(0.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    return this;
}

void ShivaVGWindowSurface::flush(QWidget *widget, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);
    QWidget *parent = widget->internalWinId() ? widget : widget->nativeParentWidget();
    d_ptr->ensureContext(parent);
    QShivaContext *context = shivaContext();
    if (!d_ptr->isCurrent)
        context->makeCurrent(d_ptr);
#if defined(QVG_USE_FBO)
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    if (d_ptr->fbo) {
        static GLfloat const vertices[][2] = {
            {-1, -1}, {1, -1}, {1, 1}, {-1, 1}
        };
        static GLfloat const texCoords[][2] = {
            {0, 0}, {1, 0}, {1, 1}, {0, 1}
        };
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glVertexPointer(2, GL_FLOAT, 0, vertices);
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
        glBindTexture(GL_TEXTURE_2D, d_ptr->texture);
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
#endif
#if defined(Q_WS_X11)
    glXSwapBuffers(X11->display, d_ptr->drawable);
#endif
    context->doneCurrent();
}

void ShivaVGWindowSurface::setGeometry(const QRect &rect)
{
    QWindowSurface::setGeometry(rect);
    d_ptr->needsResize = true;
    d_ptr->size = rect.size();
}

bool ShivaVGWindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    return QWindowSurface::scroll(area, dx, dy);
}

void ShivaVGWindowSurface::beginPaint(const QRegion &region)
{
    // Nothing to do here.
    Q_UNUSED(region);
}

void ShivaVGWindowSurface::endPaint(const QRegion &region)
{
    // Nothing to do here.
    Q_UNUSED(region);
}

Q_GLOBAL_STATIC(QVGPaintEngine, sharedPaintEngine);

QPaintEngine *ShivaVGWindowSurface::paintEngine() const
{
    if (!d_ptr->engine)
        d_ptr->engine = sharedPaintEngine();
    return d_ptr->engine;
}

int ShivaVGWindowSurface::metric(PaintDeviceMetric met) const
{
    return qt_paint_device_metric(window(), met);
}

QT_END_NAMESPACE
