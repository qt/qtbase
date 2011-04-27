/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
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

#include "qwindowsurface_vgegl_p.h"
#include "qpaintengine_vg_p.h"
#include "qpixmapdata_vg_p.h"
#include "qvgimagepool_p.h"
#include "qvg_p.h"

#if !defined(QT_NO_EGL)

QT_BEGIN_NAMESPACE

// Turn off "direct to window" rendering if EGL cannot support it.
#if !defined(EGL_RENDER_BUFFER) || !defined(EGL_SINGLE_BUFFER)
#if defined(QVG_DIRECT_TO_WINDOW)
#undef QVG_DIRECT_TO_WINDOW
#endif
#endif

// Determine if preserved window contents should be used.
#if !defined(EGL_SWAP_BEHAVIOR) || !defined(EGL_BUFFER_PRESERVED)
#if !defined(QVG_NO_PRESERVED_SWAP)
#define QVG_NO_PRESERVED_SWAP 1
#endif
#endif

VGImageFormat qt_vg_config_to_vg_format(QEglContext *context)
{
    return qt_vg_image_to_vg_format
        (qt_vg_config_to_image_format(context));
}

QImage::Format qt_vg_config_to_image_format(QEglContext *context)
{
    EGLint red = context->configAttrib(EGL_RED_SIZE);
    EGLint green = context->configAttrib(EGL_GREEN_SIZE);
    EGLint blue = context->configAttrib(EGL_BLUE_SIZE);
    EGLint alpha = context->configAttrib(EGL_ALPHA_SIZE);
    QImage::Format argbFormat;
#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
    EGLint type = context->configAttrib(EGL_SURFACE_TYPE);
    if ((type & EGL_VG_ALPHA_FORMAT_PRE_BIT) != 0)
        argbFormat = QImage::Format_ARGB32_Premultiplied;
    else
        argbFormat = QImage::Format_ARGB32;
#else
    argbFormat = QImage::Format_ARGB32;
#endif
    if (red == 8 && green == 8 && blue == 8 && alpha == 8)
        return argbFormat;
    else if (red == 8 && green == 8 && blue == 8 && alpha == 0)
        return QImage::Format_RGB32;
    else if (red == 5 && green == 6 && blue == 5 && alpha == 0)
        return QImage::Format_RGB16;
    else if (red == 4 && green == 4 && blue == 4 && alpha == 4)
        return QImage::Format_ARGB4444_Premultiplied;
    else
        return argbFormat;       // XXX
}

#if !defined(QVG_NO_SINGLE_CONTEXT)

class QVGSharedContext
{
public:
    QVGSharedContext();
    ~QVGSharedContext();

    QEglContext *context;
    int refCount;
    int widgetRefCount;
    QVGPaintEngine *engine;
    EGLSurface surface;
    QVGPixmapData *firstPixmap;
};

QVGSharedContext::QVGSharedContext()
    : context(0)
    , refCount(0)
    , widgetRefCount(0)
    , engine(0)
    , surface(EGL_NO_SURFACE)
    , firstPixmap(0)
{
}

QVGSharedContext::~QVGSharedContext()
{
    // Don't accidentally destroy the QEglContext if the reference
    // count falls to zero while deleting the paint engine.
    ++refCount;

    if (context)
        context->makeCurrent(qt_vg_shared_surface());
    delete engine;
    if (context)
        context->doneCurrent();
    if (context && surface != EGL_NO_SURFACE)
        context->destroySurface(surface);
    delete context;
}

Q_GLOBAL_STATIC(QVGSharedContext, sharedContext);

QVGPaintEngine *qt_vg_create_paint_engine(void)
{
    QVGSharedContext *shared = sharedContext();
    if (!shared->engine)
        shared->engine = new QVGPaintEngine();
    return shared->engine;
}

void qt_vg_destroy_paint_engine(QVGPaintEngine *engine)
{
    Q_UNUSED(engine);
}

void qt_vg_register_pixmap(QVGPixmapData *pd)
{
    QVGSharedContext *shared = sharedContext();
    pd->next = shared->firstPixmap;
    pd->prev = 0;
    if (shared->firstPixmap)
        shared->firstPixmap->prev = pd;
    shared->firstPixmap = pd;
}

void qt_vg_unregister_pixmap(QVGPixmapData *pd)
{
    if (pd->next)
        pd->next->prev = pd->prev;
    if (pd->prev) {
        pd->prev->next = pd->next;
    } else {
        QVGSharedContext *shared = sharedContext();
        if (shared)
           shared->firstPixmap = pd->next;
    }
}

#else

QVGPaintEngine *qt_vg_create_paint_engine(void)
{
    return new QVGPaintEngine();
}

void qt_vg_destroy_paint_engine(QVGPaintEngine *engine)
{
    delete engine;
}

void qt_vg_register_pixmap(QVGPixmapData *pd)
{
    Q_UNUSED(pd);
}

void qt_vg_unregister_pixmap(QVGPixmapData *pd)
{
    Q_UNUSED(pd);
}

#endif

#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT

static bool isPremultipliedContext(const QEglContext *context)
{
    return context->configAttrib(EGL_SURFACE_TYPE) & EGL_VG_ALPHA_FORMAT_PRE_BIT;
}

#endif

static QEglContext *createContext(QPaintDevice *device)
{
    QEglContext *context;

    // Create the context object and open the display.
    context = new QEglContext();
    context->setApi(QEgl::OpenVG);

    // Set the swap interval for the display.
    QByteArray interval = qgetenv("QT_VG_SWAP_INTERVAL");
    if (!interval.isEmpty())
        eglSwapInterval(QEgl::display(), interval.toInt());
    else
        eglSwapInterval(QEgl::display(), 1);

#ifdef EGL_RENDERABLE_TYPE
    // Has the user specified an explicit EGL configuration to use?
    QByteArray configId = qgetenv("QT_VG_EGL_CONFIG");
    if (!configId.isEmpty()) {
        EGLint cfgId = configId.toInt();
        EGLint properties[] = {
            EGL_CONFIG_ID, cfgId,
            EGL_NONE
        };
        EGLint matching = 0;
        EGLConfig cfg;
        if (eglChooseConfig
                    (QEgl::display(), properties, &cfg, 1, &matching) &&
                matching > 0) {
            // Check that the selected configuration actually supports OpenVG
            // and then create the context with it.
            EGLint id = 0;
            EGLint type = 0;
            eglGetConfigAttrib
                (QEgl::display(), cfg, EGL_CONFIG_ID, &id);
            eglGetConfigAttrib
                (QEgl::display(), cfg, EGL_RENDERABLE_TYPE, &type);
            if (cfgId == id && (type & EGL_OPENVG_BIT) != 0) {
                context->setConfig(cfg);
                if (!context->createContext()) {
                    delete context;
                    return 0;
                }
                return context;
            } else {
                qWarning("QT_VG_EGL_CONFIG: %d is not a valid OpenVG configuration", int(cfgId));
            }
        }
    }
#endif

    // Choose an appropriate configuration for rendering into the device.
    QEglProperties configProps;
    configProps.setPaintDeviceFormat(device);
    int redSize = configProps.value(EGL_RED_SIZE);
    if (redSize == EGL_DONT_CARE || redSize == 0)
        configProps.setPixelFormat(QImage::Format_ARGB32);  // XXX
    configProps.setValue(EGL_ALPHA_MASK_SIZE, 1);
#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
    configProps.setValue(EGL_SURFACE_TYPE, EGL_WINDOW_BIT |
                         EGL_VG_ALPHA_FORMAT_PRE_BIT);
    configProps.setRenderableType(QEgl::OpenVG);
    if (!context->chooseConfig(configProps)) {
        // Try again without the "pre" bit.
        configProps.setValue(EGL_SURFACE_TYPE, EGL_WINDOW_BIT);
        if (!context->chooseConfig(configProps)) {
            delete context;
            return 0;
        }
    }
#else
    configProps.setValue(EGL_SURFACE_TYPE, EGL_WINDOW_BIT);
    configProps.setRenderableType(QEgl::OpenVG);
    if (!context->chooseConfig(configProps)) {
        delete context;
        return 0;
    }
#endif

    // Construct a new EGL context for the selected configuration.
    if (!context->createContext()) {
        delete context;
        return 0;
    }

    return context;
}

#if !defined(QVG_NO_SINGLE_CONTEXT)

QEglContext *qt_vg_create_context(QPaintDevice *device, int devType)
{
    QVGSharedContext *shared = sharedContext();
    if (devType == QInternal::Widget)
        ++(shared->widgetRefCount);
    if (shared->context) {
        ++(shared->refCount);
        return shared->context;
    } else {
        shared->context = createContext(device);
        shared->refCount = 1;
        return shared->context;
    }
}

static void qt_vg_destroy_shared_context(QVGSharedContext *shared)
{
    shared->context->makeCurrent(qt_vg_shared_surface());
    delete shared->engine;
    shared->engine = 0;
    shared->context->doneCurrent();
    if (shared->surface != EGL_NO_SURFACE) {
        eglDestroySurface(QEgl::display(), shared->surface);
        shared->surface = EGL_NO_SURFACE;
    }
    delete shared->context;
    shared->context = 0;
}

void qt_vg_hibernate_pixmaps(QVGSharedContext *shared)
{
    // Artificially increase the reference count to prevent the
    // context from being destroyed until after we have finished
    // the hibernation process.
    ++(shared->refCount);

    // We need a context current to hibernate the VGImage objects.
    shared->context->makeCurrent(qt_vg_shared_surface());

    // Scan all QVGPixmapData objects in the system and hibernate them.
    QVGPixmapData *pd = shared->firstPixmap;
    while (pd != 0) {
        pd->hibernate();
        pd = pd->next;
    }

    // Hibernate any remaining VGImage's in the image pool.
    QVGImagePool::instance()->hibernate();

    // Don't need the current context any more.
    shared->context->lazyDoneCurrent();

    // Decrease the reference count and destroy the context if necessary.
    if (--(shared->refCount) <= 0)
        qt_vg_destroy_shared_context(shared);
}

void qt_vg_destroy_context(QEglContext *context, int devType)
{
    QVGSharedContext *shared = sharedContext();
    if (shared->context != context) {
        // This is not the shared context.  Shouldn't happen!
        delete context;
        return;
    }
    if (devType == QInternal::Widget)
        --(shared->widgetRefCount);
    if (--(shared->refCount) <= 0) {
        qt_vg_destroy_shared_context(shared);
    } else if (shared->widgetRefCount <= 0 && devType == QInternal::Widget) {
        // All of the widget window surfaces have been destroyed
        // but we still have VG pixmaps active.  Ask them to hibernate
        // to free up GPU resources until a widget is shown again.
        // This may eventually cause the EGLContext to be destroyed
        // because nothing in the system needs a context, which will
        // free up even more GPU resources.
        qt_vg_hibernate_pixmaps(shared);
    }
}

EGLSurface qt_vg_shared_surface(void)
{
    QVGSharedContext *shared = sharedContext();
    if (shared->surface == EGL_NO_SURFACE) {
        EGLint attribs[7];
        attribs[0] = EGL_WIDTH;
        attribs[1] = 16;
        attribs[2] = EGL_HEIGHT;
        attribs[3] = 16;
#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
        if (isPremultipliedContext(shared->context)) {
            attribs[4] = EGL_VG_ALPHA_FORMAT;
            attribs[5] = EGL_VG_ALPHA_FORMAT_PRE;
            attribs[6] = EGL_NONE;
        } else
#endif
        {
            attribs[4] = EGL_NONE;
        }
        shared->surface = eglCreatePbufferSurface
            (QEgl::display(), shared->context->config(), attribs);
    }
    return shared->surface;
}

#else

QEglContext *qt_vg_create_context(QPaintDevice *device, int devType)
{
    Q_UNUSED(devType);
    return createContext(device);
}

void qt_vg_destroy_context(QEglContext *context, int devType)
{
    Q_UNUSED(devType);
    delete context;
}

EGLSurface qt_vg_shared_surface(void)
{
    return EGL_NO_SURFACE;
}

#endif

QVGEGLWindowSurfacePrivate::QVGEGLWindowSurfacePrivate(QWindowSurface *win)
{
    winSurface = win;
    engine = 0;
}

QVGEGLWindowSurfacePrivate::~QVGEGLWindowSurfacePrivate()
{
    // Destroy the paint engine if it hasn't been destroyed already.
    destroyPaintEngine();
}

QVGPaintEngine *QVGEGLWindowSurfacePrivate::paintEngine()
{
    if (!engine)
        engine = qt_vg_create_paint_engine();
    return engine;
}

VGImage QVGEGLWindowSurfacePrivate::surfaceImage() const
{
    return VG_INVALID_HANDLE;
}

void QVGEGLWindowSurfacePrivate::destroyPaintEngine()
{
    if (engine) {
        qt_vg_destroy_paint_engine(engine);
        engine = 0;
    }
}

QSize QVGEGLWindowSurfacePrivate::windowSurfaceSize(QWidget *widget) const
{
    Q_UNUSED(widget);

    QRect rect = winSurface->geometry();
    QSize newSize = rect.size();

#if defined(Q_WS_QWS)
    // Account for the widget mask, if any.
    if (widget && !widget->mask().isEmpty()) {
        const QRegion region = widget->mask()
                               & rect.translated(-widget->geometry().topLeft());
        newSize = region.boundingRect().size();
    }
#endif

    return newSize;
}

#if defined(QVG_VGIMAGE_BACKBUFFERS)

QVGEGLWindowSurfaceVGImage::QVGEGLWindowSurfaceVGImage(QWindowSurface *win)
    : QVGEGLWindowSurfacePrivate(win)
    , context(0)
    , backBuffer(VG_INVALID_HANDLE)
    , backBufferSurface(EGL_NO_SURFACE)
    , recreateBackBuffer(false)
    , isPaintingActive(false)
    , windowSurface(EGL_NO_SURFACE)
{
}

QVGEGLWindowSurfaceVGImage::~QVGEGLWindowSurfaceVGImage()
{
    destroyPaintEngine();
    if (context) {
        if (backBufferSurface != EGL_NO_SURFACE) {
            // We need a current context to be able to destroy the image.
            // We use the shared surface because the native window handle
            // associated with "windowSurface" may have been destroyed already.
            context->makeCurrent(qt_vg_shared_surface());
            context->destroySurface(backBufferSurface);
            vgDestroyImage(backBuffer);
            context->doneCurrent();
        }
        if (windowSurface != EGL_NO_SURFACE)
            context->destroySurface(windowSurface);
        qt_vg_destroy_context(context, QInternal::Widget);
    }
}

QEglContext *QVGEGLWindowSurfaceVGImage::ensureContext(QWidget *widget)
{
    QSize newSize = windowSurfaceSize(widget);
    if (context && size != newSize) {
        // The surface size has changed, so we need to recreate
        // the back buffer.  Keep the same context and paint engine.
        size = newSize;
        if (isPaintingActive)
            context->doneCurrent();
        isPaintingActive = false;
        recreateBackBuffer = true;
    }
    if (!context) {
        // Create a new EGL context.  We create the surface in beginPaint().
        size = newSize;
        context = qt_vg_create_context(widget, QInternal::Widget);
        if (!context)
            return 0;
        isPaintingActive = false;
    }
    return context;
}

void QVGEGLWindowSurfaceVGImage::beginPaint(QWidget *widget)
{
    QEglContext *context = ensureContext(widget);
    if (context) {
        if (recreateBackBuffer || backBufferSurface == EGL_NO_SURFACE) {
            // Create a VGImage object to act as the back buffer
            // for this window.  We have to create the VGImage with a
            // current context, so activate the main surface for the window.
            context->makeCurrent(mainSurface());
            recreateBackBuffer = false;
            if (backBufferSurface != EGL_NO_SURFACE) {
                eglDestroySurface(QEgl::display(), backBufferSurface);
                backBufferSurface = EGL_NO_SURFACE;
            }
            if (backBuffer != VG_INVALID_HANDLE) {
                vgDestroyImage(backBuffer);
            }
            VGImageFormat format = qt_vg_config_to_vg_format(context);
            backBuffer = vgCreateImage
                (format, size.width(), size.height(),
                 VG_IMAGE_QUALITY_FASTER);
            if (backBuffer != VG_INVALID_HANDLE) {
                // Create an EGL surface for rendering into the VGImage.
                backBufferSurface = eglCreatePbufferFromClientBuffer
                    (QEgl::display(), EGL_OPENVG_IMAGE,
                     (EGLClientBuffer)(backBuffer),
                     context->config(), NULL);
                if (backBufferSurface == EGL_NO_SURFACE) {
                    vgDestroyImage(backBuffer);
                    backBuffer = VG_INVALID_HANDLE;
                }
            }
        }
        if (backBufferSurface != EGL_NO_SURFACE)
            context->makeCurrent(backBufferSurface);
        else
            context->makeCurrent(mainSurface());
        isPaintingActive = true;
    }
}

void QVGEGLWindowSurfaceVGImage::endPaint
        (QWidget *widget, const QRegion& region, QImage *image)
{
    Q_UNUSED(region);
    Q_UNUSED(image);
    QEglContext *context = ensureContext(widget);
    if (context) {
        if (backBufferSurface != EGL_NO_SURFACE) {
            if (isPaintingActive)
                vgFlush();
            context->lazyDoneCurrent();
        }
        isPaintingActive = false;
    }
}

VGImage QVGEGLWindowSurfaceVGImage::surfaceImage() const
{
    return backBuffer;
}

EGLSurface QVGEGLWindowSurfaceVGImage::mainSurface() const
{
    if (windowSurface != EGL_NO_SURFACE)
        return windowSurface;
    else
        return qt_vg_shared_surface();
}

#endif // QVG_VGIMAGE_BACKBUFFERS

QVGEGLWindowSurfaceDirect::QVGEGLWindowSurfaceDirect(QWindowSurface *win)
    : QVGEGLWindowSurfacePrivate(win)
    , context(0)
    , isPaintingActive(false)
    , needToSwap(false)
    , windowSurface(EGL_NO_SURFACE)
{
}

QVGEGLWindowSurfaceDirect::~QVGEGLWindowSurfaceDirect()
{
    destroyPaintEngine();
    if (context) {
        if (windowSurface != EGL_NO_SURFACE)
            context->destroySurface(windowSurface);
        qt_vg_destroy_context(context, QInternal::Widget);
    }
}

QEglContext *QVGEGLWindowSurfaceDirect::ensureContext(QWidget *widget)
{
    QSize newSize = windowSurfaceSize(widget);
    QEglProperties surfaceProps;

#if defined(QVG_RECREATE_ON_SIZE_CHANGE)
#if !defined(QVG_NO_SINGLE_CONTEXT)
    if (context && size != newSize) {
        // The surface size has changed, so we need to recreate it.
        // We can keep the same context and paint engine.
        size = newSize;
        if (isPaintingActive)
            context->doneCurrent();
        context->destroySurface(windowSurface);
#if defined(EGL_VG_ALPHA_FORMAT_PRE_BIT)
        if (isPremultipliedContext(context)) {
            surfaceProps.setValue
                (EGL_VG_ALPHA_FORMAT, EGL_VG_ALPHA_FORMAT_PRE);
        } else {
            surfaceProps.removeValue(EGL_VG_ALPHA_FORMAT);
        }
#endif
        windowSurface = context->createSurface(widget, &surfaceProps);
        isPaintingActive = false;
        needToSwap = true;
    }
#else
    if (context && size != newSize) {
        // The surface size has changed, so we need to recreate
        // the EGL context for the widget.  We also need to recreate
        // the surface's paint engine if context sharing is not
        // enabled because we cannot reuse the existing paint objects
        // in the new context.
        qt_vg_destroy_paint_engine(engine);
        engine = 0;
        context->destroySurface(windowSurface);
        qt_vg_destroy_context(context, QInternal::Widget);
        context = 0;
        windowSurface = EGL_NO_SURFACE;
    }
#endif
#endif
    if (!context) {
        // Create a new EGL context and bind it to the widget surface.
        size = newSize;
        context = qt_vg_create_context(widget, QInternal::Widget);
        if (!context)
            return 0;
        // We want a direct to window rendering surface if possible.
#if defined(QVG_DIRECT_TO_WINDOW)
        surfaceProps.setValue(EGL_RENDER_BUFFER, EGL_SINGLE_BUFFER);
#endif
#if defined(EGL_VG_ALPHA_FORMAT_PRE_BIT)
        if (isPremultipliedContext(context)) {
            surfaceProps.setValue
                (EGL_VG_ALPHA_FORMAT, EGL_VG_ALPHA_FORMAT_PRE);
        } else {
            surfaceProps.removeValue(EGL_VG_ALPHA_FORMAT);
        }
#endif
        EGLSurface surface = context->createSurface(widget, &surfaceProps);
        if (surface == EGL_NO_SURFACE) {
            qt_vg_destroy_context(context, QInternal::Widget);
            context = 0;
            return 0;
        }
        needToSwap = true;
#if defined(QVG_DIRECT_TO_WINDOW)
        // Did we get a direct to window rendering surface?
        EGLint buffer = 0;
        if (eglQueryContext(QEgl::display(), context->context(),
                            EGL_RENDER_BUFFER, &buffer) &&
                buffer == EGL_SINGLE_BUFFER) {
            needToSwap = false;
        }
#endif
        windowSurface = surface;
        isPaintingActive = false;
    }

#if !defined(QVG_NO_PRESERVED_SWAP)
    // Try to force the surface back buffer to preserve its contents.
    if (needToSwap) {
        bool succeeded = eglSurfaceAttrib(QEgl::display(), windowSurface,
                EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
        if (!succeeded && eglGetError() != EGL_SUCCESS) {
            qWarning("QVG: could not enable preserved swap");
        }
    }
#endif
    return context;
}

void QVGEGLWindowSurfaceDirect::beginPaint(QWidget *widget)
{
    QEglContext *context = ensureContext(widget);
    if (context) {
        context->makeCurrent(windowSurface);
        isPaintingActive = true;
    }
}

void QVGEGLWindowSurfaceDirect::endPaint
        (QWidget *widget, const QRegion& region, QImage *image)
{
    Q_UNUSED(region);
    Q_UNUSED(image);
    QEglContext *context = ensureContext(widget);
    if (context) {
        if (needToSwap) {
            if (!isPaintingActive)
                context->makeCurrent(windowSurface);
            context->swapBuffers(windowSurface);
            context->lazyDoneCurrent();
        } else if (isPaintingActive) {
            vgFlush();
            context->lazyDoneCurrent();
        }
        isPaintingActive = false;
    }
}

bool QVGEGLWindowSurfaceDirect::supportsStaticContents() const
{
#if defined(QVG_BUFFER_SCROLLING) && !defined(QVG_NO_PRESERVED_SWAP)
    return true;
#else
    return QVGEGLWindowSurfacePrivate::supportsStaticContents();
#endif
}

bool QVGEGLWindowSurfaceDirect::scroll(QWidget *widget, const QRegion& area, int dx, int dy)
{
#ifdef QVG_BUFFER_SCROLLING
    QEglContext *context = ensureContext(widget);
    if (context) {
        context->makeCurrent(windowSurface);
        QRect scrollRect = area.boundingRect();
        int sx = scrollRect.x();
        int sy = size.height() - scrollRect.y() - scrollRect.height();
        vgSeti(VG_SCISSORING, VG_FALSE);
        vgCopyPixels(sx + dx, sy - dy, sx, sy, scrollRect.width(), scrollRect.height());
        context->lazyDoneCurrent();
        return true;
    }
#else
    Q_UNUSED(widget);
    Q_UNUSED(area);
    Q_UNUSED(dx);
    Q_UNUSED(dy);
#endif
    return false;
}

QT_END_NAMESPACE

#endif
