/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QApplication>
#include <QtGui/QColormap>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPaintDevice>
#include <QtGui/QWidget>

#include <qglframebufferobject.h>
#include <qglpixelbuffer.h>
#include <qcolormap.h>
#include <qdesktopwidget.h>
#include <private/qwidget_p.h>
#include "qdebug.h"

#ifdef Q_WS_X11
#include <private/qt_x11_p.h>
#include <qx11info_x11.h>

#ifndef QT_OPENGL_ES
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif
#endif //Q_WS_X11

#include <private/qglextensions_p.h>
#include <private/qwindowsurface_gl_p.h>

#include <private/qgl_p.h>

#include <private/qglpixelbuffer_p.h>
#include <private/qgraphicssystem_gl_p.h>

#include <private/qpaintengineex_opengl2_p.h>
#include <private/qpixmapdata_gl_p.h>

#ifndef QT_OPENGL_ES_2
#include <private/qpaintengine_opengl_p.h>
#endif

#ifndef GLX_ARB_multisample
#define GLX_SAMPLE_BUFFERS_ARB  100000
#define GLX_SAMPLES_ARB         100001
#endif

#ifndef QT_NO_EGL
#include <private/qeglcontext_p.h>
#endif

QT_BEGIN_NAMESPACE

//
// QGLGraphicsSystem
//
#ifdef Q_WS_WIN
extern Q_GUI_EXPORT bool qt_win_owndc_required;
#endif
QGLGraphicsSystem::QGLGraphicsSystem(bool useX11GL)
    : QGraphicsSystem(), m_useX11GL(useX11GL)
{
#if defined(Q_WS_X11) && !defined(QT_OPENGL_ES)
    // only override the system defaults if the user hasn't already
    // picked a visual
    if (X11->visual == 0 && X11->visual_id == -1 && X11->visual_class == -1) {
        // find a double buffered, RGBA visual that supports OpenGL
        // and set that as the default visual for windows in Qt
        int i = 0;
        int spec[16];
        spec[i++] = GLX_RGBA;
        spec[i++] = GLX_DOUBLEBUFFER;

        if (!qgetenv("QT_GL_SWAPBUFFER_PRESERVE").isNull()) {
            spec[i++] = GLX_DEPTH_SIZE;
            spec[i++] = 8;
            spec[i++] = GLX_STENCIL_SIZE;
            spec[i++] = 8;
            spec[i++] = GLX_SAMPLE_BUFFERS_ARB;
            spec[i++] = 1;
            spec[i++] = GLX_SAMPLES_ARB;
            spec[i++] = 4;
        }

        spec[i++] = XNone;

        XVisualInfo *vi = glXChooseVisual(X11->display, X11->defaultScreen, spec);
        if (vi) {
            X11->visual_id = vi->visualid;
            X11->visual_class = vi->c_class;

            QGLFormat format;
            int res;
            glXGetConfig(X11->display, vi, GLX_LEVEL, &res);
            format.setPlane(res);
            glXGetConfig(X11->display, vi, GLX_DOUBLEBUFFER, &res);
            format.setDoubleBuffer(res);
            glXGetConfig(X11->display, vi, GLX_DEPTH_SIZE, &res);
            format.setDepth(res);
            if (format.depth())
                format.setDepthBufferSize(res);
            glXGetConfig(X11->display, vi, GLX_RGBA, &res);
            format.setRgba(res);
            glXGetConfig(X11->display, vi, GLX_RED_SIZE, &res);
            format.setRedBufferSize(res);
            glXGetConfig(X11->display, vi, GLX_GREEN_SIZE, &res);
            format.setGreenBufferSize(res);
            glXGetConfig(X11->display, vi, GLX_BLUE_SIZE, &res);
            format.setBlueBufferSize(res);
            glXGetConfig(X11->display, vi, GLX_ALPHA_SIZE, &res);
            format.setAlpha(res);
            if (format.alpha())
                format.setAlphaBufferSize(res);
            glXGetConfig(X11->display, vi, GLX_ACCUM_RED_SIZE, &res);
            format.setAccum(res);
            if (format.accum())
                format.setAccumBufferSize(res);
            glXGetConfig(X11->display, vi, GLX_STENCIL_SIZE, &res);
            format.setStencil(res);
            if (format.stencil())
                format.setStencilBufferSize(res);
            glXGetConfig(X11->display, vi, GLX_STEREO, &res);
            format.setStereo(res);
            glXGetConfig(X11->display, vi, GLX_SAMPLE_BUFFERS_ARB, &res);
            format.setSampleBuffers(res);
            if (format.sampleBuffers()) {
                glXGetConfig(X11->display, vi, GLX_SAMPLES_ARB, &res);
                format.setSamples(res);
            }

            QGLWindowSurface::surfaceFormat = format;
            XFree(vi);

            printf("using visual class %x, id %x\n", X11->visual_class, X11->visual_id);
        }
    }
#elif defined(Q_WS_WIN)
    QGLWindowSurface::surfaceFormat.setDoubleBuffer(true);

    qt_win_owndc_required = true;
#endif
}

//
// QGLWindowSurface
//
class QGLGlobalShareWidget
{
public:
    QGLGlobalShareWidget() : firstPixmap(0), widgetRefCount(0), widget(0), initializing(false) {}

    QGLWidget *shareWidget() {
        if (!initializing && !widget && !cleanedUp) {
            initializing = true;
            widget = new QGLWidget(QGLFormat(QGL::SingleBuffer | QGL::NoDepthBuffer | QGL::NoStencilBuffer));
            widget->resize(1, 1);

            // We don't need this internal widget to appear in QApplication::topLevelWidgets()
            if (QWidgetPrivate::allWidgets)
                QWidgetPrivate::allWidgets->remove(widget);
            initializing = false;
        }
        return widget;
    }

    // destroys the share widget and prevents recreation
    void cleanup() {
        QGLWidget *w = widget;
        cleanedUp = true;
        widget = 0;
        delete w;
    }

    // destroys the share widget, but allows it to be recreated later on
    void destroy() {
        if (cleanedUp)
            return;

        QGLWidget *w = widget;

        // prevent potential recursions
        cleanedUp = true;
        widget = 0;
        delete w;
        cleanedUp = false;
    }

    static bool cleanedUp;

    QGLPixmapData *firstPixmap;
    int widgetRefCount;

private:
    QGLWidget *widget;
    bool initializing;
};

bool QGLGlobalShareWidget::cleanedUp = false;

static void qt_cleanup_gl_share_widget();
Q_GLOBAL_STATIC_WITH_INITIALIZER(QGLGlobalShareWidget, _qt_gl_share_widget,
                                 {
                                     qAddPostRoutine(qt_cleanup_gl_share_widget);
                                 })

static void qt_cleanup_gl_share_widget()
{
    _qt_gl_share_widget()->cleanup();
}

QGLWidget* qt_gl_share_widget()
{
    if (QGLGlobalShareWidget::cleanedUp)
        return 0;
    return _qt_gl_share_widget()->shareWidget();
}

void qt_destroy_gl_share_widget()
{
    _qt_gl_share_widget()->destroy();
}

const QGLContext *qt_gl_share_context()
{
    QGLWidget *widget = qt_gl_share_widget();
    if (widget)
        return widget->context();
    return 0;
}

#ifdef QGL_USE_TEXTURE_POOL
void qt_gl_register_pixmap(QGLPixmapData *pd)
{
    QGLGlobalShareWidget *shared = _qt_gl_share_widget();
    pd->next = shared->firstPixmap;
    pd->prev = 0;
    if (shared->firstPixmap)
        shared->firstPixmap->prev = pd;
    shared->firstPixmap = pd;
}

void qt_gl_unregister_pixmap(QGLPixmapData *pd)
{
    if (pd->next)
        pd->next->prev = pd->prev;
    if (pd->prev) {
        pd->prev->next = pd->next;
    } else {
        QGLGlobalShareWidget *shared = _qt_gl_share_widget();
        if (shared)
           shared->firstPixmap = pd->next;
    }
}

void qt_gl_hibernate_pixmaps()
{
    QGLGlobalShareWidget *shared = _qt_gl_share_widget();

    // Scan all QGLPixmapData objects in the system and hibernate them.
    QGLPixmapData *pd = shared->firstPixmap;
    while (pd != 0) {
        pd->hibernate();
        pd = pd->next;
    }
}
#endif

struct QGLWindowSurfacePrivate
{
    QGLFramebufferObject *fbo;
    QGLPixelBuffer *pb;
    GLuint tex_id;
    GLuint pb_tex_id;

    int tried_fbo : 1;
    int tried_pb : 1;
    int destructive_swap_buffers : 1;
    int geometry_updated : 1;
    int did_paint : 1;

    QGLContext *ctx;

    QList<QGLContext **> contexts;

    QRegion paintedRegion;
    QSize size;

    QSize textureSize;

    QList<QImage> buffers;
    QGLWindowSurfaceGLPaintDevice glDevice;
    QGLWindowSurface* q_ptr;

    bool swap_region_support;
};

QGLFormat QGLWindowSurface::surfaceFormat;
QGLWindowSurface::SwapMode QGLWindowSurface::swapBehavior = QGLWindowSurface::AutomaticSwap;

void QGLWindowSurfaceGLPaintDevice::endPaint()
{
    glFlush();
    QGLPaintDevice::endPaint();
}

QSize QGLWindowSurfaceGLPaintDevice::size() const
{
    return d->size;
}

QGLContext* QGLWindowSurfaceGLPaintDevice::context() const
{
    return d->ctx;
}


int QGLWindowSurfaceGLPaintDevice::metric(PaintDeviceMetric m) const
{
    return qt_paint_device_metric(d->q_ptr->window(), m);
}

QPaintEngine *QGLWindowSurfaceGLPaintDevice::paintEngine() const
{
    return qt_qgl_paint_engine();
}

QGLWindowSurface::QGLWindowSurface(QWidget *window)
    : QWindowSurface(window), d_ptr(new QGLWindowSurfacePrivate)
{
//    Q_ASSERT(window->isTopLevel());
    d_ptr->pb = 0;
    d_ptr->fbo = 0;
    d_ptr->ctx = 0;
    d_ptr->tex_id = 0;
#if defined (QT_OPENGL_ES_2)
    d_ptr->tried_fbo = true;
    d_ptr->tried_pb = true;
#else
    d_ptr->tried_fbo = false;
    d_ptr->tried_pb = false;
#endif
    d_ptr->destructive_swap_buffers = qgetenv("QT_GL_SWAPBUFFER_PRESERVE").isNull();
    d_ptr->glDevice.d = d_ptr;
    d_ptr->q_ptr = this;
    d_ptr->geometry_updated = false;
    d_ptr->did_paint = false;
    d_ptr->swap_region_support = false;
}

QGLWindowSurface::~QGLWindowSurface()
{
    if (d_ptr->ctx)
        glDeleteTextures(1, &d_ptr->tex_id);
#ifndef Q_WS_QPA // Dont delete the contexts. Destroying the window does that for us
    foreach(QGLContext **ctx, d_ptr->contexts) {
        delete *ctx;
        *ctx = 0;
    }
#endif
    delete d_ptr->pb;
    delete d_ptr->fbo;
    delete d_ptr;

    if (QGLGlobalShareWidget::cleanedUp)
        return;

    --(_qt_gl_share_widget()->widgetRefCount);

#ifdef QGL_USE_TEXTURE_POOL
    if (_qt_gl_share_widget()->widgetRefCount <= 0) {
        // All of the widget window surfaces have been destroyed
        // but we still have GL pixmaps active.  Ask them to hibernate
        // to free up GPU resources until a widget is shown again.
        // This may eventually cause the EGLContext to be destroyed
        // because nothing in the system needs a context, which will
        // free up even more GPU resources.
        qt_gl_hibernate_pixmaps();

        // Destroy the context if necessary.
        if (!qt_gl_share_widget()->context()->isSharing())
            qt_destroy_gl_share_widget();
    }
#endif // QGL_USE_TEXTURE_POOL
}

void QGLWindowSurface::deleted(QObject *object)
{
    QWidget *widget = qobject_cast<QWidget *>(object);
    if (widget) {
        if (widget == window()) {
            // Make sure that the fbo is destroyed before destroying its context.
            delete d_ptr->fbo;
            d_ptr->fbo = 0;
        }

#ifndef Q_WS_QPA //no need to specifically delete the QGLContext as it will be deleted by QWidget
        QWidgetPrivate *widgetPrivate = widget->d_func();
        if (widgetPrivate->extraData()) {
            union { QGLContext **ctxPtrPtr; void **voidPtrPtr; };
            voidPtrPtr = &widgetPrivate->extraData()->glContext;
            int index = d_ptr->contexts.indexOf(ctxPtrPtr);
            if (index != -1) {
                delete *ctxPtrPtr;
                *ctxPtrPtr = 0;
                d_ptr->contexts.removeAt(index);
            }
        }
#endif
    }
}

void QGLWindowSurface::hijackWindow(QWidget *widget)
{
    QWidgetPrivate *widgetPrivate = widget->d_func();
    widgetPrivate->createExtra();
    if (widgetPrivate->extraData()->glContext)
        return;

    QGLContext *ctx = NULL;

    // For translucent top-level widgets we need alpha in the format.
    if (widget->testAttribute(Qt::WA_TranslucentBackground)) {
        QGLFormat modFormat(surfaceFormat);
        modFormat.setSampleBuffers(false);
        modFormat.setSamples(0);
        modFormat.setAlpha(true);
        ctx = new QGLContext(modFormat, widget);
    } else
        ctx = new QGLContext(surfaceFormat, widget);

    ctx->create(qt_gl_share_context());

    if (widget != qt_gl_share_widget())
        ++(_qt_gl_share_widget()->widgetRefCount);

#ifndef QT_NO_EGL
    static bool checkedForNOKSwapRegion = false;
    static bool haveNOKSwapRegion = false;

    if (!checkedForNOKSwapRegion) {
        haveNOKSwapRegion = QEgl::hasExtension("EGL_NOK_swap_region2");
        checkedForNOKSwapRegion = true;

        if (haveNOKSwapRegion)
            qDebug() << "Found EGL_NOK_swap_region2 extension. Using partial updates.";
    }

    d_ptr->destructive_swap_buffers = true;
    if (ctx->d_func()->eglContext->configAttrib(EGL_SURFACE_TYPE)&EGL_SWAP_BEHAVIOR_PRESERVED_BIT) {
        EGLint swapBehavior;
        if (eglQuerySurface(ctx->d_func()->eglContext->display(), ctx->d_func()->eglSurface
                            , EGL_SWAP_BEHAVIOR, &swapBehavior)) {
            d_ptr->destructive_swap_buffers = (swapBehavior != EGL_BUFFER_PRESERVED);
        }
    }

    d_ptr->swap_region_support = haveNOKSwapRegion;
#endif

    widgetPrivate->extraData()->glContext = ctx;

    union { QGLContext **ctxPtrPtr; void **voidPtrPtr; };

    connect(widget, SIGNAL(destroyed(QObject*)), this, SLOT(deleted(QObject*)));

    voidPtrPtr = &widgetPrivate->extraData()->glContext;
    d_ptr->contexts << ctxPtrPtr;
#ifndef Q_OS_SYMBIAN
    qDebug() << "hijackWindow() context created for" << widget << d_ptr->contexts.size();
#endif
}

QGLContext *QGLWindowSurface::context() const
{
    return d_ptr->ctx;
}

QPaintDevice *QGLWindowSurface::paintDevice()
{
    updateGeometry();

    if (d_ptr->pb)
        return d_ptr->pb;

    if (d_ptr->ctx)
        return &d_ptr->glDevice;

    QGLContext *ctx = reinterpret_cast<QGLContext *>(window()->d_func()->extraData()->glContext);
    ctx->makeCurrent();

    Q_ASSERT(d_ptr->fbo);
    return d_ptr->fbo;
}

static void drawTexture(const QRectF &rect, GLuint tex_id, const QSize &texSize, const QRectF &src = QRectF());

void QGLWindowSurface::beginPaint(const QRegion &)
{
    d_ptr->did_paint = true;
    updateGeometry();

    if (!context())
        return;

    int clearFlags = 0;

    if (context()->d_func()->workaround_needsFullClearOnEveryFrame)
        clearFlags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    else if (context()->format().alpha())
        clearFlags = GL_COLOR_BUFFER_BIT;

    if (clearFlags) {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(clearFlags);
    }
}

void QGLWindowSurface::endPaint(const QRegion &rgn)
{
    if (context())
        d_ptr->paintedRegion |= rgn;

    d_ptr->buffers.clear();
}

static void blitTexture(QGLContext *ctx, GLuint texture, const QSize &viewport, const QSize &texSize, const QRect &targetRect, const QRect &sourceRect)
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, viewport.width(), viewport.height());

    QGLShaderProgram *blitProgram =
        QGLEngineSharedShaders::shadersForContext(ctx)->blitProgram();
    blitProgram->bind();
    blitProgram->setUniformValue("imageTexture", 0 /*QT_IMAGE_TEXTURE_UNIT*/);

    // The shader manager's blit program does not multiply the
    // vertices by the pmv matrix, so we need to do the effect
    // of the orthographic projection here ourselves.
    QRectF r;
    qreal w = viewport.width();
    qreal h = viewport.height();
    r.setLeft((targetRect.left() / w) * 2.0f - 1.0f);
    if (targetRect.right() == (viewport.width() - 1))
        r.setRight(1.0f);
    else
        r.setRight((targetRect.right() / w) * 2.0f - 1.0f);
    r.setBottom((targetRect.top() / h) * 2.0f - 1.0f);
    if (targetRect.bottom() == (viewport.height() - 1))
        r.setTop(1.0f);
    else
        r.setTop((targetRect.bottom() / w) * 2.0f - 1.0f);

    drawTexture(r, texture, texSize, sourceRect);
}


void QGLWindowSurface::flush(QWidget *widget, const QRegion &rgn, const QPoint &offset)
{
    //### Find out why d_ptr->geometry_updated isn't always false.
    // flush() should not be called when d_ptr->geometry_updated is true. It assumes that either
    // d_ptr->fbo or d_ptr->pb is allocated and has the correct size.
    if (d_ptr->geometry_updated)
        return;

    // did_paint is set to true in ::beginPaint. ::beginPaint means that we
    // at least cleared the background (= painted something). In EGL API it's a
    // mistake to call swapBuffers if nothing was painted unless
    // EGL_BUFFER_PRESERVED is set. This check protects the flush func from
    // being executed if it's for nothing.
    if (!d_ptr->destructive_swap_buffers && !d_ptr->did_paint)
        return;

#ifdef Q_OS_SYMBIAN
    if (window() != widget) {
        // For performance reasons we don't support
        // flushing native child widgets on Symbian.
        // It breaks overlapping native child widget 
        // rendering in some cases but we prefer performance.
        return;
    }
#endif


    QWidget *parent = widget->internalWinId() ? widget : widget->nativeParentWidget();
    Q_ASSERT(parent);

#if !defined(Q_WS_QPA)
    if (!geometry().isValid())
        return;
#else
    if (!size().isValid())
        return;
#endif

    // Needed to support native child-widgets...
    hijackWindow(parent);

    QRect br = rgn.boundingRect().translated(offset);
    br = br.intersected(window()->rect());
    QPoint wOffset = qt_qwidget_data(parent)->wrect.topLeft();
    QRect rect = br.translated(-offset - wOffset);

    const GLenum target = GL_TEXTURE_2D;
    Q_UNUSED(target);

    if (QGLWindowSurface::swapBehavior == QGLWindowSurface::KillSwap)
        return;

    if (context()) {
        context()->makeCurrent();

        if (context()->format().doubleBuffer()) {
#if !defined(QT_OPENGL_ES_2)
            if (d_ptr->destructive_swap_buffers) {
                glBindTexture(target, d_ptr->tex_id);

                QVector<QRect> rects = d_ptr->paintedRegion.rects();
                for (int i = 0; i < rects.size(); ++i) {
                    QRect br = rects.at(i);
                    if (br.isEmpty())
                        continue;

                    const uint bottom = window()->height() - (br.y() + br.height());
                    glCopyTexSubImage2D(target, 0, br.x(), bottom, br.x(), bottom, br.width(), br.height());
                }

                glBindTexture(target, 0);

                QRegion dirtyRegion = QRegion(window()->rect()) - d_ptr->paintedRegion;

                if (!dirtyRegion.isEmpty()) {
                    glMatrixMode(GL_MODELVIEW);
                    glLoadIdentity();

                    glMatrixMode(GL_PROJECTION);
                    glLoadIdentity();
#ifndef QT_OPENGL_ES
                    glOrtho(0, window()->width(), window()->height(), 0, -999999, 999999);
#else
                    glOrthof(0, window()->width(), window()->height(), 0, -999999, 999999);
#endif
                    glViewport(0, 0, window()->width(), window()->height());

                    QVector<QRect> rects = dirtyRegion.rects();
                    glColor4f(1, 1, 1, 1);
                    for (int i = 0; i < rects.size(); ++i) {
                        QRect rect = rects.at(i);
                        if (rect.isEmpty())
                            continue;

                        drawTexture(rect, d_ptr->tex_id, window()->size(), rect);
                    }
                }
            }
#endif
            bool doingPartialUpdate = false;
            if (d_ptr->swap_region_support) {
                if (QGLWindowSurface::swapBehavior == QGLWindowSurface::AutomaticSwap)
                    doingPartialUpdate = br.width() * br.height() < parent->geometry().width() * parent->geometry().height() * 0.2;
                else if (QGLWindowSurface::swapBehavior == QGLWindowSurface::AlwaysPartialSwap)
                    doingPartialUpdate = true;
            }

            QGLContext *ctx = reinterpret_cast<QGLContext *>(parent->d_func()->extraData()->glContext);
            if (widget != window()) {
                if (initializeOffscreenTexture(window()->size()))
                    qWarning() << "QGLWindowSurface: Flushing to native child widget, may lead to significant performance loss";
                glBindTexture(target, d_ptr->tex_id);

                const uint bottom = window()->height() - (br.y() + br.height());
                glCopyTexSubImage2D(target, 0, br.x(), bottom, br.x(), bottom, br.width(), br.height());

                glBindTexture(target, 0);

                ctx->makeCurrent();
                if (doingPartialUpdate)
                    blitTexture(ctx, d_ptr->tex_id, parent->size(), window()->size(), rect, br);
                else
                    blitTexture(ctx, d_ptr->tex_id, parent->size(), window()->size(), parent->rect(), parent->rect().translated(offset + wOffset));
            }

            if (doingPartialUpdate)
                ctx->d_func()->swapRegion(br);
            else
                ctx->swapBuffers();

            d_ptr->paintedRegion = QRegion();
        } else {
            glFlush();
        }
        return;
    }

    QGLContext *previous_ctx = const_cast<QGLContext *>(QGLContext::currentContext());
    QGLContext *ctx = reinterpret_cast<QGLContext *>(parent->d_func()->extraData()->glContext);

    // QPainter::end() should have unbound the fbo, otherwise something is very wrong...
    Q_ASSERT(!d_ptr->fbo || !d_ptr->fbo->isBound());

    if (ctx != previous_ctx) {
        ctx->makeCurrent();
    }

    QSize size = widget->rect().size();
    if (d_ptr->destructive_swap_buffers && ctx->format().doubleBuffer()) {
        rect = parent->rect();
        br = rect.translated(wOffset + offset);
        size = parent->size();
    }

    glDisable(GL_SCISSOR_TEST);

    if (d_ptr->fbo && (QGLExtensions::glExtensions() & QGLExtensions::FramebufferBlit)) {
        const int h = d_ptr->fbo->height();

        const int sx0 = br.left();
        const int sx1 = br.left() + br.width();
        const int sy0 = h - (br.top() + br.height());
        const int sy1 = h - br.top();

        const int tx0 = rect.left();
        const int tx1 = rect.left() + rect.width();
        const int ty0 = parent->height() - (rect.top() + rect.height());
        const int ty1 = parent->height() - rect.top();

        if (window() == parent || d_ptr->fbo->format().samples() <= 1) {
            if (ctx->d_ptr->current_fbo != 0)
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, 0);

            glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, d_ptr->fbo->handle());

            glBlitFramebufferEXT(sx0, sy0, sx1, sy1,
                    tx0, ty0, tx1, ty1,
                    GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);

            glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0);
        } else {
            // can't do sub-region blits with multisample FBOs
            QGLFramebufferObject *temp = qgl_fbo_pool()->acquire(d_ptr->fbo->size(), QGLFramebufferObjectFormat());

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, temp->handle());
            glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, d_ptr->fbo->handle());

            glBlitFramebufferEXT(0, 0, d_ptr->fbo->width(), d_ptr->fbo->height(),
                    0, 0, d_ptr->fbo->width(), d_ptr->fbo->height(),
                    GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);

            glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, temp->handle());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, 0);

            glBlitFramebufferEXT(sx0, sy0, sx1, sy1,
                    tx0, ty0, tx1, ty1,
                    GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);

            glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0);

            qgl_fbo_pool()->release(temp);
        }

        ctx->d_ptr->current_fbo = 0;
    }
#if !defined(QT_OPENGL_ES_2)
    else {
        GLuint texture;
    if (d_ptr->fbo) {
        texture = d_ptr->fbo->texture();
    } else {
        d_ptr->pb->makeCurrent();
        glBindTexture(target, d_ptr->pb_tex_id);
        const uint bottom = window()->height() - (br.y() + br.height());
        glCopyTexSubImage2D(target, 0, br.x(), bottom, br.x(), bottom, br.width(), br.height());
        texture = d_ptr->pb_tex_id;
        glBindTexture(target, 0);
    }

        glDisable(GL_DEPTH_TEST);

        if (d_ptr->fbo) {
            d_ptr->fbo->release();
        } else {
            ctx->makeCurrent();
        }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
#ifndef QT_OPENGL_ES
        glOrtho(0, size.width(), size.height(), 0, -999999, 999999);
#else
        glOrthof(0, size.width(), size.height(), 0, -999999, 999999);
#endif
        glViewport(0, 0, size.width(), size.height());

        glColor4f(1, 1, 1, 1);
        drawTexture(rect, texture, window()->size(), br);

        if (d_ptr->fbo)
            d_ptr->fbo->bind();
    }
#else
    // OpenGL/ES 2.0 version of the fbo blit.
    else if (d_ptr->fbo) {
        Q_UNUSED(target);

        if (d_ptr->fbo->isBound())
            d_ptr->fbo->release();

        blitTexture(ctx, d_ptr->fbo->texture(), size, window()->size(), rect, br);
    }
#endif

    if (ctx->format().doubleBuffer())
        ctx->swapBuffers();
    else
        glFlush();

    d_ptr->did_paint = false;
}


#if !defined(Q_WS_QPA)
void QGLWindowSurface::setGeometry(const QRect &rect)
{
    QWindowSurface::setGeometry(rect);
    d_ptr->geometry_updated = true;
}
#else
void QGLWindowSurface::resize(const QSize &size)
{
    QWindowSurface::resize(size);
    d_ptr->geometry_updated = true;
}
#endif

void QGLWindowSurface::updateGeometry() {
    if (!d_ptr->geometry_updated)
        return;
    d_ptr->geometry_updated = false;

    bool hijack(true);
    QWidgetPrivate *wd = window()->d_func();
    if (wd->extraData() && wd->extraData()->glContext) {
#ifdef Q_OS_SYMBIAN // Symbian needs to recreate the context when native window size changes
        if (d_ptr->size != geometry().size()) {
            if (window() != qt_gl_share_widget())
                --(_qt_gl_share_widget()->widgetRefCount);

            delete wd->extraData()->glContext;
            wd->extraData()->glContext = 0;
            d_ptr->ctx = 0;
        }
        else
#endif
        {
            hijack = false; // we already have gl context for widget
        }
    }

    if (hijack)
        hijackWindow(window());

    QGLContext *ctx = reinterpret_cast<QGLContext *>(wd->extraData()->glContext);

#ifdef Q_WS_MAC
    ctx->updatePaintDevice();
#endif

    QSize surfSize = geometry().size();

    if (surfSize.width() <= 0 || surfSize.height() <= 0)
        return;

    if (d_ptr->size == surfSize)
        return;

    d_ptr->size = surfSize;

    if (d_ptr->ctx) {
#ifndef QT_OPENGL_ES_2
        if (d_ptr->destructive_swap_buffers)
            initializeOffscreenTexture(surfSize);
#endif
        return;
    }

    const GLenum target = GL_TEXTURE_2D;
    if (d_ptr->destructive_swap_buffers
        && (QGLExtensions::glExtensions() & QGLExtensions::FramebufferObject)
        && (d_ptr->fbo || !d_ptr->tried_fbo)
        && qt_gl_preferGL2Engine())
    {
        d_ptr->tried_fbo = true;
        ctx->d_ptr->internal_context = true;
        ctx->makeCurrent();
        delete d_ptr->fbo;

        QGLFramebufferObjectFormat format;
        format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
        format.setInternalTextureFormat(GLenum(GL_RGBA));
        format.setTextureTarget(target);

        if (QGLExtensions::glExtensions() & QGLExtensions::FramebufferBlit)
            format.setSamples(8);

        d_ptr->fbo = new QGLFramebufferObject(surfSize, format);

        if (d_ptr->fbo->isValid()) {
            qDebug() << "Created Window Surface FBO" << surfSize
                     << "with samples" << d_ptr->fbo->format().samples();
            return;
        } else {
            qDebug() << "QGLWindowSurface: Failed to create valid FBO, falling back";
            delete d_ptr->fbo;
            d_ptr->fbo = 0;
        }
    }

#if !defined(QT_OPENGL_ES_2) && !defined(Q_WS_QPA) //QPA doesn't support pixelbuffers
    if (d_ptr->destructive_swap_buffers && (d_ptr->pb || !d_ptr->tried_pb)) {
        d_ptr->tried_pb = true;

        if (d_ptr->pb) {
            d_ptr->pb->makeCurrent();
            glDeleteTextures(1, &d_ptr->pb_tex_id);
        }

        delete d_ptr->pb;

        d_ptr->pb = new QGLPixelBuffer(surfSize.width(), surfSize.height(),
                                        QGLFormat(QGL::SampleBuffers | QGL::StencilBuffer | QGL::DepthBuffer),
                                        qt_gl_share_widget());

        if (d_ptr->pb->isValid()) {
            qDebug() << "Created Window Surface Pixelbuffer, Sample buffers:" << d_ptr->pb->format().sampleBuffers();
            d_ptr->pb->makeCurrent();

            glGenTextures(1, &d_ptr->pb_tex_id);
            glBindTexture(target, d_ptr->pb_tex_id);
            glTexImage2D(target, 0, GL_RGBA, surfSize.width(), surfSize.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

            glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(target, 0);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, d_ptr->pb->width(), d_ptr->pb->height(), 0, -999999, 999999);

            d_ptr->pb->d_ptr->qctx->d_func()->internal_context = true;
            return;
        } else {
            qDebug() << "QGLWindowSurface: Failed to create valid pixelbuffer, falling back";
            delete d_ptr->pb;
            d_ptr->pb = 0;
        }
    }
#endif // !defined(QT_OPENGL_ES_2) !defined(Q_WS_QPA)

    ctx->makeCurrent();

#ifndef QT_OPENGL_ES_2
    if (d_ptr->destructive_swap_buffers)
        initializeOffscreenTexture(surfSize);
#endif
#ifndef Q_OS_SYMBIAN
    qDebug() << "QGLWindowSurface: Using plain widget as window surface" << this;
#endif
    d_ptr->ctx = ctx;
    d_ptr->ctx->d_ptr->internal_context = true;
}

bool QGLWindowSurface::initializeOffscreenTexture(const QSize &size)
{
    if (size == d_ptr->textureSize)
        return false;

    glGenTextures(1, &d_ptr->tex_id);
    glBindTexture(GL_TEXTURE_2D, d_ptr->tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.width(), size.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    d_ptr->textureSize = size;
    return true;
}

bool QGLWindowSurface::scroll(const QRegion &area, int dx, int dy)
{
    // this code randomly fails currently for unknown reasons
    return false;

    if (!d_ptr->pb)
        return false;

    d_ptr->pb->makeCurrent();

    QRect br = area.boundingRect();

#if 0
    // ## workaround driver issue (scrolling by these deltas is unbearably slow for some reason)
    // ## maybe we should use glCopyTexSubImage insteadk
    if (dx == 1 || dx == -1 || dy == 1 || dy == -1 || dy == 2)
        return false;

    glRasterPos2i(br.x() + dx, br.y() + br.height() + dy);
    glCopyPixels(br.x(), d_ptr->pb->height() - (br.y() + br.height()), br.width(), br.height(), GL_COLOR);
    return true;
#endif

    const GLenum target = GL_TEXTURE_2D;

    glBindTexture(target, d_ptr->tex_id);
    glCopyTexImage2D(target, 0, GL_RGBA, br.x(), d_ptr->pb->height() - (br.y() + br.height()), br.width(), br.height(), 0);
    glBindTexture(target, 0);

    drawTexture(br.translated(dx, dy), d_ptr->tex_id, window()->size());

    return true;
}

static void drawTexture(const QRectF &rect, GLuint tex_id, const QSize &texSize, const QRectF &br)
{
    const GLenum target = GL_TEXTURE_2D;
    QRectF src = br.isEmpty()
        ? QRectF(QPointF(), texSize)
        : QRectF(QPointF(br.x(), texSize.height() - br.bottom()), br.size());

    if (target == GL_TEXTURE_2D) {
        qreal width = texSize.width();
        qreal height = texSize.height();

        src.setLeft(src.left() / width);
        src.setRight(src.right() / width);
        src.setTop(src.top() / height);
        src.setBottom(src.bottom() / height);
    }

    const GLfloat tx1 = src.left();
    const GLfloat tx2 = src.right();
    const GLfloat ty1 = src.top();
    const GLfloat ty2 = src.bottom();

    GLfloat texCoordArray[4*2] = {
        tx1, ty2, tx2, ty2, tx2, ty1, tx1, ty1
    };

    GLfloat vertexArray[4*2];
    extern void qt_add_rect_to_array(const QRectF &r, GLfloat *array); // qpaintengine_opengl.cpp
    qt_add_rect_to_array(rect, vertexArray);

#if !defined(QT_OPENGL_ES_2)
    glVertexPointer(2, GL_FLOAT, 0, vertexArray);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoordArray);

    glBindTexture(target, tex_id);
    glEnable(target);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glDisable(target);
    glBindTexture(target, 0);
#else
    glVertexAttribPointer(QT_VERTEX_COORDS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, vertexArray);
    glVertexAttribPointer(QT_TEXTURE_COORDS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, texCoordArray);

    glBindTexture(target, tex_id);

    glEnableVertexAttribArray(QT_VERTEX_COORDS_ATTR);
    glEnableVertexAttribArray(QT_TEXTURE_COORDS_ATTR);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(QT_VERTEX_COORDS_ATTR);
    glDisableVertexAttribArray(QT_TEXTURE_COORDS_ATTR);

    glBindTexture(target, 0);
#endif
}

QImage *QGLWindowSurface::buffer(const QWidget *widget)
{
    QImage image;

    if (d_ptr->pb)
        image = d_ptr->pb->toImage();
    else if (d_ptr->fbo)
        image = d_ptr->fbo->toImage();

    if (image.isNull())
        return 0;

    QRect rect = widget->rect();
    rect.translate(widget->mapTo(widget->window(), QPoint()));

    QImage subImage = image.copy(rect);
    d_ptr->buffers << subImage;
    return &d_ptr->buffers.last();
}

QWindowSurface::WindowSurfaceFeatures QGLWindowSurface::features() const
{
    WindowSurfaceFeatures features = 0;
    if (!d_ptr->destructive_swap_buffers || d_ptr->swap_region_support)
        features |= PartialUpdates;
    if (!d_ptr->destructive_swap_buffers)
        features |= PreservedContents;
    return features;
}

QT_END_NAMESPACE

