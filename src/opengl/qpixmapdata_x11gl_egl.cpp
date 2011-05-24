/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include <QDebug>

#include <QtGui/private/qt_x11_p.h>
#include <QtGui/private/qegl_p.h>
#include <QtGui/private/qeglproperties_p.h>
#include <QtGui/private/qeglcontext_p.h>

#if !defined(QT_OPENGL_ES_1)
#include <QtOpenGL/private/qpaintengineex_opengl2_p.h>
#endif

#ifndef QT_OPENGL_ES_2
#include <QtOpenGL/private/qpaintengine_opengl_p.h>
#endif

#include <QtOpenGL/private/qgl_p.h>
#include <QtOpenGL/private/qgl_egl_p.h>

#include "qpixmapdata_x11gl_p.h"

QT_BEGIN_NAMESPACE


class QX11GLSharedContexts
{
public:
    QX11GLSharedContexts()
        : rgbContext(0)
        , argbContext(0)
        , sharedQGLContext(0)
        , sharePixmap(0)
    {
        EGLint rgbConfigId;
        EGLint argbConfigId;

        do {
            EGLConfig rgbConfig = QEgl::defaultConfig(QInternal::Pixmap, QEgl::OpenGL, QEgl::Renderable);
            EGLConfig argbConfig = QEgl::defaultConfig(QInternal::Pixmap, QEgl::OpenGL,
                                                       QEgl::Renderable | QEgl::Translucent);

            eglGetConfigAttrib(QEgl::display(), rgbConfig, EGL_CONFIG_ID, &rgbConfigId);
            eglGetConfigAttrib(QEgl::display(), argbConfig, EGL_CONFIG_ID, &argbConfigId);

            rgbContext = new QEglContext;
            rgbContext->setConfig(rgbConfig);
            rgbContext->createContext();

            if (!rgbContext->isValid())
                break;

            // If the RGB & ARGB configs are the same, use the same egl context for both:
            if (rgbConfig == argbConfig)
                argbContext = rgbContext;

            // Otherwise, create a separate context to be used for ARGB pixmaps:
            if (!argbContext) {
                argbContext = new QEglContext;
                argbContext->setConfig(argbConfig);
                bool success = argbContext->createContext(rgbContext);
                if (!success) {
                    qWarning("QX11GLPixmapData - RGB & ARGB contexts aren't shared");
                    success = argbContext->createContext();
                    if (!success)
                        argbContext = rgbContext; // Might work, worth a shot at least.
                }
            }

            if (!argbContext->isValid())
                break;

            // Create the pixmap which will be used to create the egl surface for the share QGLContext
            QX11PixmapData *rgbPixmapData = new QX11PixmapData(QPixmapData::PixmapType);
            rgbPixmapData->resize(8, 8);
            rgbPixmapData->fill(Qt::red);
            sharePixmap = new QPixmap(rgbPixmapData);
            EGLSurface sharePixmapSurface = QEgl::createSurface(sharePixmap, rgbConfig);
            rgbPixmapData->gl_surface = (void*)sharePixmapSurface;

            // Create the actual QGLContext which will be used for sharing
            sharedQGLContext = new QGLContext(QX11GLPixmapData::glFormat());
            sharedQGLContext->d_func()->eglContext = rgbContext;
            sharedQGLContext->d_func()->eglSurface = sharePixmapSurface;
            sharedQGLContext->d_func()->valid = true;
            qt_glformat_from_eglconfig(sharedQGLContext->d_func()->glFormat, rgbConfig);


            valid = rgbContext->makeCurrent(sharePixmapSurface);

            // If the ARGB & RGB configs are different, check ARGB works too:
            if (argbConfig != rgbConfig) {
                QX11PixmapData *argbPixmapData = new QX11PixmapData(QPixmapData::PixmapType);
                argbPixmapData->resize(8, 8);
                argbPixmapData->fill(Qt::transparent); // Force ARGB
                QPixmap argbPixmap(argbPixmapData); // destroys pixmap data when goes out of scope
                EGLSurface argbPixmapSurface = QEgl::createSurface(&argbPixmap, argbConfig);
                valid = argbContext->makeCurrent(argbPixmapSurface);
                argbContext->doneCurrent();
                eglDestroySurface(QEgl::display(), argbPixmapSurface);
                argbPixmapData->gl_surface = 0;
            }

            if (!valid) {
                qWarning() << "Unable to make pixmap surface current:" << QEgl::errorString();
                break;
            }

            // The pixmap surface destruction hooks are installed by QGLTextureCache, so we
            // must make sure this is instanciated:
            QGLTextureCache::instance();
        } while(0);

        if (!valid)
            cleanup();
        else
            qDebug("Using QX11GLPixmapData with EGL config %d for ARGB and config %d for RGB", argbConfigId, rgbConfigId);

    }

    ~QX11GLSharedContexts() {
        cleanup();
    }

    void cleanup() {
        if (sharedQGLContext) {
            delete sharedQGLContext;
            sharedQGLContext = 0;
        }
        if (argbContext && argbContext != rgbContext)
            delete argbContext;
        argbContext = 0;

        if (rgbContext) {
            delete rgbContext;
            rgbContext = 0;
        }

        // Deleting the QPixmap will fire the pixmap destruction cleanup hooks which in turn
        // will destroy the egl surface:
        if (sharePixmap) {
            delete sharePixmap;
            sharePixmap = 0;
        }
    }

    bool isValid() { return valid;}

    // On 16bpp systems, RGB & ARGB pixmaps are different bit-depths and therefore need
    // different contexts:
    QEglContext *rgbContext;
    QEglContext *argbContext;

    // The share context wraps the rgbContext and is used as the master of the context share
    // group. As all other contexts will have the same egl context (or a shared one if rgb != argb)
    // all QGLContexts will actually be sharing and can be in the same context group.
    QGLContext  *sharedQGLContext;
private:
    QPixmap     *sharePixmap;
    bool         valid;
};

static void qt_cleanup_x11gl_share_contexts();

Q_GLOBAL_STATIC_WITH_INITIALIZER(QX11GLSharedContexts, qt_x11gl_share_contexts,
                                 {
                                     qAddPostRoutine(qt_cleanup_x11gl_share_contexts);
                                 })

static void qt_cleanup_x11gl_share_contexts()
{
    qt_x11gl_share_contexts()->cleanup();
}


QX11GLSharedContexts* QX11GLPixmapData::sharedContexts()
{
    return qt_x11gl_share_contexts();
}

bool QX11GLPixmapData::hasX11GLPixmaps()
{
    static bool checkedForX11GLPixmaps = false;
    static bool haveX11GLPixmaps = false;

    if (checkedForX11GLPixmaps)
        return haveX11GLPixmaps;

    haveX11GLPixmaps = qt_x11gl_share_contexts()->isValid();
    checkedForX11GLPixmaps = true;

    return haveX11GLPixmaps;
}

QX11GLPixmapData::QX11GLPixmapData()
    : QX11PixmapData(QPixmapData::PixmapType),
      ctx(0)
{
}

QX11GLPixmapData::~QX11GLPixmapData()
{
    if (ctx)
        delete ctx;
}


void QX11GLPixmapData::fill(const QColor &color)
{
    if (ctx) {
        ctx->makeCurrent();
        glFinish();
        eglWaitClient();
    }

    QX11PixmapData::fill(color);
    XSync(X11->display, False);

    if (ctx) {
        ctx->makeCurrent();
        eglWaitNative(EGL_CORE_NATIVE_ENGINE);
    }
}

void QX11GLPixmapData::copy(const QPixmapData *data, const QRect &rect)
{
    if (ctx) {
        ctx->makeCurrent();
        glFinish();
        eglWaitClient();
    }

    QX11PixmapData::copy(data, rect);
    XSync(X11->display, False);

    if (ctx) {
        ctx->makeCurrent();
        eglWaitNative(EGL_CORE_NATIVE_ENGINE);
    }
}

bool QX11GLPixmapData::scroll(int dx, int dy, const QRect &rect)
{
    if (ctx) {
        ctx->makeCurrent();
        glFinish();
        eglWaitClient();
    }

    bool success = QX11PixmapData::scroll(dx, dy, rect);
    XSync(X11->display, False);

    if (ctx) {
        ctx->makeCurrent();
        eglWaitNative(EGL_CORE_NATIVE_ENGINE);
    }

    return success;
}

#if !defined(QT_OPENGL_ES_1)
Q_GLOBAL_STATIC(QGL2PaintEngineEx, qt_gl_pixmap_2_engine)
#endif

#ifndef QT_OPENGL_ES_2
Q_GLOBAL_STATIC(QOpenGLPaintEngine, qt_gl_pixmap_engine)
#endif


QPaintEngine* QX11GLPixmapData::paintEngine() const
{
    // We need to create the context before beginPaint - do it here:
    if (!ctx) {
        ctx = new QGLContext(glFormat());
        Q_ASSERT(ctx->d_func()->eglContext == 0);
        ctx->d_func()->eglContext = hasAlphaChannel() ? sharedContexts()->argbContext : sharedContexts()->rgbContext;

        // While we use a separate QGLContext for each pixmap, the underlying QEglContext is
        // the same. So we must use a "fake" QGLContext and fool the texture cache into thinking
        // each pixmap's QGLContext is sharing with this central one. The only place this is
        // going to fail is where we the underlying EGL RGB and ARGB contexts aren't sharing.
        ctx->d_func()->sharing = true;
        QGLContextGroup::addShare(ctx, sharedContexts()->sharedQGLContext);

        // Update the glFormat for the QGLContext:
        qt_glformat_from_eglconfig(ctx->d_func()->glFormat, ctx->d_func()->eglContext->config());
    }

    QPaintEngine* engine;

#if defined(QT_OPENGL_ES_1)
    engine = qt_gl_pixmap_engine();
#elif defined(QT_OPENGL_ES_2)
    engine = qt_gl_pixmap_2_engine();
#else
    if (qt_gl_preferGL2Engine())
        engine = qt_gl_pixmap_2_engine();
    else
        engine = qt_gl_pixmap_engine();
#endif



    // Support multiple painters on multiple pixmaps simultaniously
    if (engine->isActive()) {
        qWarning("Pixmap paint engine already active");

#if defined(QT_OPENGL_ES_1)
        engine = new QOpenGLPaintEngine;
#elif defined(QT_OPENGL_ES_2)
        engine = new QGL2PaintEngineEx;
#else
        if (qt_gl_preferGL2Engine())
            engine = new QGL2PaintEngineEx;
        else
            engine = new QOpenGLPaintEngine;
#endif

        engine->setAutoDestruct(true);
        return engine;
    }

    return engine;
}

void QX11GLPixmapData::beginPaint()
{
//    qDebug("QX11GLPixmapData::beginPaint()");
    // TODO: Check to see if the surface is renderable
    if ((EGLSurface)gl_surface == EGL_NO_SURFACE) {
        QPixmap tmpPixmap(this);
        EGLConfig cfg = ctx->d_func()->eglContext->config();
        Q_ASSERT(cfg != QEGL_NO_CONFIG);

//        qDebug("QX11GLPixmapData - using EGL Config ID %d", ctx->d_func()->eglContext->configAttrib(EGL_CONFIG_ID));
        EGLSurface surface = QEgl::createSurface(&tmpPixmap, cfg);
        if (surface == EGL_NO_SURFACE) {
            qWarning() << "Error creating EGL surface for pixmap:" << QEgl::errorString();
            return;
        }
        gl_surface = (void*)surface;
        ctx->d_func()->eglSurface = surface;
        ctx->d_func()->valid = true;
    }
    QGLPaintDevice::beginPaint();
}

QGLContext* QX11GLPixmapData::context() const
{
    return ctx;
}

QSize QX11GLPixmapData::size() const
{
    return QSize(w, h);
}


QGLFormat QX11GLPixmapData::glFormat()
{
    return QGLFormat::defaultFormat(); //###
}

QT_END_NAMESPACE
