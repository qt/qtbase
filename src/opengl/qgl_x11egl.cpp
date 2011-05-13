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

#include "qgl.h"
#include <private/qt_x11_p.h>
#include <private/qpixmap_x11_p.h>
#include <private/qgl_p.h>
#include <private/qpaintengine_opengl_p.h>
#include "qgl_egl_p.h"
#include "qcolormap.h"
#include <QDebug>
#include <QPixmap>


QT_BEGIN_NAMESPACE


/*
    QGLTemporaryContext implementation
*/

class QGLTemporaryContextPrivate
{
public:
    bool initialized;
    Window window;
    EGLContext context;
    EGLSurface surface;
    EGLDisplay display;
};

QGLTemporaryContext::QGLTemporaryContext(bool, QWidget *)
    : d(new QGLTemporaryContextPrivate)
{
    d->initialized = false;
    d->window = 0;
    d->context = 0;
    d->surface = 0;
    int screen = 0;

    d->display = QEgl::display();

    EGLConfig config;
    int numConfigs = 0;
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
#ifdef QT_OPENGL_ES_2
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#endif
        EGL_NONE
    };

    eglChooseConfig(d->display, attribs, &config, 1, &numConfigs);
    if (!numConfigs) {
        qWarning("QGLTemporaryContext: No EGL configurations available.");
        return;
    }

    XVisualInfo visualInfo;
    XVisualInfo *vi;
    int numVisuals;

    visualInfo.visualid = QEgl::getCompatibleVisualId(config);
    vi = XGetVisualInfo(X11->display, VisualIDMask, &visualInfo, &numVisuals);
    if (!vi || numVisuals < 1) {
        qWarning("QGLTemporaryContext: Unable to get X11 visual info id.");
        return;
    }

    XSetWindowAttributes attr;
    unsigned long mask;
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(X11->display, DefaultRootWindow(X11->display), vi->visual, AllocNone);
    attr.event_mask = StructureNotifyMask | ExposureMask;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    d->window = XCreateWindow(X11->display, RootWindow(X11->display, screen),
                              0, 0, 1, 1, 0,
                              vi->depth, InputOutput, vi->visual,
                              mask, &attr);

    d->surface = eglCreateWindowSurface(d->display, config, (EGLNativeWindowType) d->window, NULL);

    if (d->surface == EGL_NO_SURFACE) {
        qWarning("QGLTemporaryContext: Error creating EGL surface.");
        XFree(vi);
        XDestroyWindow(X11->display, d->window);
        return;
    }

    EGLint contextAttribs[] = {
#ifdef QT_OPENGL_ES_2
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };
    d->context = eglCreateContext(d->display, config, 0, contextAttribs);
    if (d->context != EGL_NO_CONTEXT
        && eglMakeCurrent(d->display, d->surface, d->surface, d->context))
    {
        d->initialized = true;
    } else {
        qWarning("QGLTemporaryContext: Error creating EGL context.");
        eglDestroySurface(d->display, d->surface);
        XDestroyWindow(X11->display, d->window);
    }
    XFree(vi);
}

QGLTemporaryContext::~QGLTemporaryContext()
{
    if (d->initialized) {
        eglMakeCurrent(d->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(d->display, d->context);
        eglDestroySurface(d->display, d->surface);
        XDestroyWindow(X11->display, d->window);
    }
}

bool QGLFormat::hasOpenGLOverlays()
{
    return false;
}

// Chooses the EGL config and creates the EGL context
bool QGLContext::chooseContext(const QGLContext* shareContext)
{
    Q_D(QGLContext);

    if (!device())
        return false;

    int devType = device()->devType();

    QX11PixmapData *x11PixmapData = 0;
    if (devType == QInternal::Pixmap) {
        QPixmapData *pmd = static_cast<QPixmap*>(device())->data_ptr().data();
        if (pmd->classId() == QPixmapData::X11Class)
            x11PixmapData = static_cast<QX11PixmapData*>(pmd);
        else {
            // TODO: Replace the pixmap's data with a new QX11PixmapData
            qWarning("WARNING: Creating a QGLContext on a QPixmap is only supported for X11 pixmap backend");
            return false;
        }
    } else if ((devType != QInternal::Widget) && (devType != QInternal::Pbuffer)) {
        qWarning("WARNING: Creating a QGLContext not supported on device type %d", devType);
        return false;
    }

    // Only create the eglContext if we don't already have one:
    if (d->eglContext == 0) {
        d->eglContext = new QEglContext();
        d->ownsEglContext = true;
        d->eglContext->setApi(QEgl::OpenGL);

        // If the device is a widget with WA_TranslucentBackground set, make sure the glFormat
        // has the alpha channel option set:
        if (devType == QInternal::Widget) {
            QWidget* widget = static_cast<QWidget*>(device());
            if (widget->testAttribute(Qt::WA_TranslucentBackground))
                d->glFormat.setAlpha(true);
        }

        // Construct the configuration we need for this surface.
        QEglProperties configProps;
        configProps.setDeviceType(devType);
        configProps.setRenderableType(QEgl::OpenGL);
        qt_eglproperties_set_glformat(configProps, d->glFormat);

        // Set buffer preserved for regular QWidgets, QGLWidgets are ok with either preserved or destroyed:
        if ((devType == QInternal::Widget) && qobject_cast<QGLWidget*>(static_cast<QWidget*>(device())) == 0)
            configProps.setValue(EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);

        if (!d->eglContext->chooseConfig(configProps, QEgl::BestPixelFormat)) {
            delete d->eglContext;
            d->eglContext = 0;
            return false;
        }

        // Create a new context for the configuration.
        QEglContext* eglSharedContext = shareContext ? shareContext->d_func()->eglContext : 0;
        if (!d->eglContext->createContext(eglSharedContext)) {
            delete d->eglContext;
            d->eglContext = 0;
            return false;
        }
        d->sharing = d->eglContext->isSharing();
        if (d->sharing && shareContext)
            const_cast<QGLContext *>(shareContext)->d_func()->sharing = true;
    }

    // Inform the higher layers about the actual format properties
    qt_glformat_from_eglconfig(d->glFormat, d->eglContext->config());

    // Do don't create the EGLSurface for everything.
    //    QWidget - yes, create the EGLSurface and store it in QGLContextPrivate::eglSurface
    //    QGLWidget - yes, create the EGLSurface and store it in QGLContextPrivate::eglSurface
    //    QPixmap - yes, create the EGLSurface but store it in QX11PixmapData::gl_surface
    //    QGLPixelBuffer - no, it creates the surface itself and stores it in QGLPixelBufferPrivate::pbuf

    if (devType == QInternal::Widget) {
        if (d->eglSurface != EGL_NO_SURFACE)
            eglDestroySurface(d->eglContext->display(), d->eglSurface);
        // extraWindowSurfaceCreationProps default to NULL unless were specifically set before
        d->eglSurface = QEgl::createSurface(device(), d->eglContext->config(), d->extraWindowSurfaceCreationProps);
        XFlush(X11->display);
        setWindowCreated(true);
    }

    if (x11PixmapData) {
        // TODO: Actually check to see if the existing surface can be re-used
        if (x11PixmapData->gl_surface)
            eglDestroySurface(d->eglContext->display(), (EGLSurface)x11PixmapData->gl_surface);

        x11PixmapData->gl_surface = (void*)QEgl::createSurface(device(), d->eglContext->config());
    }

    return true;
}

void *QGLContext::chooseVisual()
{
    qFatal("QGLContext::chooseVisual - this method must not be called as Qt is built with EGL support");
    return 0;
}

void *QGLContext::tryVisual(const QGLFormat& f, int bufDepth)
{
    Q_UNUSED(f);
    Q_UNUSED(bufDepth);
    qFatal("QGLContext::tryVisual - this method must not be called as Qt is built with EGL support");
    return 0;
}

void QGLWidget::resizeEvent(QResizeEvent *)
{
    Q_D(QGLWidget);
    if (!isValid())
        return;
    makeCurrent();
    if (!d->glcx->initialized())
        glInit();
    resizeGL(width(), height());
    //handle overlay
}

const QGLContext* QGLWidget::overlayContext() const
{
    return 0;
}

void QGLWidget::makeOverlayCurrent()
{
    //handle overlay
}

void QGLWidget::updateOverlayGL()
{
    //handle overlay
}

void QGLWidget::setContext(QGLContext *context, const QGLContext* shareContext, bool deleteOldContext)
{
    Q_D(QGLWidget);
    if (context == 0) {
        qWarning("QGLWidget::setContext: Cannot set null context");
        return;
    }
    if (!context->deviceIsPixmap() && context->device() != this) {
        qWarning("QGLWidget::setContext: Context must refer to this widget");
        return;
    }

    if (d->glcx)
        d->glcx->doneCurrent();
    QGLContext* oldcx = d->glcx;
    d->glcx = context;

    bool createFailed = false;
    if (!d->glcx->isValid()) {
        // Create the QGLContext here, which in turn chooses the EGL config
        // and creates the EGL context:
        if (!d->glcx->create(shareContext ? shareContext : oldcx))
            createFailed = true;
    }
    if (createFailed) {
        if (deleteOldContext)
            delete oldcx;
        return;
    }


    d->eglSurfaceWindowId = winId(); // Remember the window id we created the surface for
}

void QGLWidgetPrivate::init(QGLContext *context, const QGLWidget* shareWidget)
{
    Q_Q(QGLWidget);

    initContext(context, shareWidget);

    if (q->isValid() && glcx->format().hasOverlay()) {
        //no overlay
        qWarning("QtOpenGL ES doesn't currently support overlays");
    }
}

void QGLWidgetPrivate::cleanupColormaps()
{
}

const QGLColormap & QGLWidget::colormap() const
{
    return d_func()->cmap;
}

void QGLWidget::setColormap(const QGLColormap &)
{
}

// Re-creates the EGL surface if the window ID has changed or if there isn't a surface
void QGLWidgetPrivate::recreateEglSurface()
{
    Q_Q(QGLWidget);

    Window currentId = q->winId();

    // If the window ID has changed since the surface was created, we need to delete the
    // old surface before re-creating a new one. Note: This should not be the case as the
    // surface should be deleted before the old window id.
    if (glcx->d_func()->eglSurface != EGL_NO_SURFACE && (currentId != eglSurfaceWindowId)) {
        qWarning("EGL surface for deleted window %lx was not destroyed", uint(eglSurfaceWindowId));
        glcx->d_func()->destroyEglSurfaceForDevice();
    }

    if (glcx->d_func()->eglSurface == EGL_NO_SURFACE) {
        glcx->d_func()->eglSurface = glcx->d_func()->eglContext->createSurface(q);
        eglSurfaceWindowId = currentId;
    }
}


QGLTexture *QGLContextPrivate::bindTextureFromNativePixmap(QPixmap *pixmap, const qint64 key,
                                                           QGLContext::BindOptions options)
{
    Q_Q(QGLContext);

    // The EGL texture_from_pixmap has no facility to invert the y coordinate
    if (!(options & QGLContext::CanFlipNativePixmapBindOption))
        return 0;


    static bool checkedForTFP = false;
    static bool haveTFP = false;
    static bool checkedForEglImageTFP = false;
    static bool haveEglImageTFP = false;


    if (!checkedForEglImageTFP) {
        checkedForEglImageTFP = true;

        // We need to be able to create an EGLImage from a native pixmap, which was split
        // into a separate EGL extension, EGL_KHR_image_pixmap. It is possible to have
        // eglCreateImageKHR & eglDestroyImageKHR without support for pixmaps, so we must
        // check we have the EGLImage from pixmap functionality.
        if (QEgl::hasExtension("EGL_KHR_image") || QEgl::hasExtension("EGL_KHR_image_pixmap")) {

            // Being able to create an EGLImage from a native pixmap is also pretty useless
            // without the ability to bind that EGLImage as a texture, which is provided by
            // the GL_OES_EGL_image extension, which we try to resolve here:
            haveEglImageTFP = qt_resolve_eglimage_gl_extensions(q);

            if (haveEglImageTFP)
                qDebug("Found EGL_KHR_image_pixmap & GL_OES_EGL_image extensions (preferred method)!");
        }
    }

    if (!checkedForTFP) {
        // Check for texture_from_pixmap egl extension
        checkedForTFP = true;
        if (QEgl::hasExtension("EGL_NOKIA_texture_from_pixmap") ||
            QEgl::hasExtension("EGL_EXT_texture_from_pixmap"))
        {
            qDebug("Found texture_from_pixmap EGL extension!");
            haveTFP = true;
        }
    }

    if (!haveTFP && !haveEglImageTFP)
        return 0;


    QX11PixmapData *pixmapData = static_cast<QX11PixmapData*>(pixmap->data_ptr().data());
    Q_ASSERT(pixmapData->classId() == QPixmapData::X11Class);
    bool hasAlpha = pixmapData->hasAlphaChannel();
    bool pixmapHasValidSurface = false;
    bool textureIsBound = false;
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    if (haveTFP && pixmapData->gl_surface &&
        hasAlpha == (pixmapData->flags & QX11PixmapData::GlSurfaceCreatedWithAlpha))
    {
        pixmapHasValidSurface = true;
    }

    // If we already have a valid EGL surface for the pixmap, we should use it
    if (pixmapHasValidSurface) {
        EGLBoolean success;
        success = eglBindTexImage(QEgl::display(), (EGLSurface)pixmapData->gl_surface, EGL_BACK_BUFFER);
        if (success == EGL_FALSE) {
            qWarning() << "eglBindTexImage() failed:" << QEgl::errorString();
            eglDestroySurface(QEgl::display(), (EGLSurface)pixmapData->gl_surface);
            pixmapData->gl_surface = (void*)EGL_NO_SURFACE;
        } else
            textureIsBound = true;
    }

    // If the pixmap doesn't already have a valid surface, try binding it via EGLImage
    // first, as going through EGLImage should be faster and better supported:
    if (!textureIsBound && haveEglImageTFP) {
        EGLImageKHR eglImage;

        EGLint attribs[] = {
            EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
            EGL_NONE
        };
        eglImage = QEgl::eglCreateImageKHR(QEgl::display(), EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR,
                                     (EGLClientBuffer)QEgl::nativePixmap(pixmap), attribs);

        QGLContext* ctx = q;
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage);

        GLint err = glGetError();
        if (err == GL_NO_ERROR)
            textureIsBound = true;

        // Once the egl image is bound, the texture becomes a new sibling image and we can safely
        // destroy the EGLImage we created for the pixmap:
        if (eglImage != EGL_NO_IMAGE_KHR)
            QEgl::eglDestroyImageKHR(QEgl::display(), eglImage);
    }

    if (!textureIsBound && haveTFP) {
        // Check to see if the surface is still valid
        if (pixmapData->gl_surface &&
            hasAlpha != (pixmapData->flags & QX11PixmapData::GlSurfaceCreatedWithAlpha))
        {
            // Surface is invalid!
            destroyGlSurfaceForPixmap(pixmapData);
        }

        if (pixmapData->gl_surface == 0) {
            EGLConfig config = QEgl::defaultConfig(QInternal::Pixmap,
                                                   QEgl::OpenGL,
                                                   hasAlpha ? QEgl::Translucent : QEgl::NoOptions);

            pixmapData->gl_surface = (void*)QEgl::createSurface(pixmap, config);
            if (pixmapData->gl_surface == (void*)EGL_NO_SURFACE)
                return false;
        }

        EGLBoolean success;
        success = eglBindTexImage(QEgl::display(), (EGLSurface)pixmapData->gl_surface, EGL_BACK_BUFFER);
        if (success == EGL_FALSE) {
            qWarning() << "eglBindTexImage() failed:" << QEgl::errorString();
            eglDestroySurface(QEgl::display(), (EGLSurface)pixmapData->gl_surface);
            pixmapData->gl_surface = (void*)EGL_NO_SURFACE;
            haveTFP = false; // If TFP isn't working, disable it's use
        } else
            textureIsBound = true;
    }

    QGLTexture *texture = 0;

    if (textureIsBound) {
        texture = new QGLTexture(q, textureId, GL_TEXTURE_2D, options);
        pixmapData->flags |= QX11PixmapData::InvertedWhenBoundToTexture;

        // We assume the cost of bound pixmaps is zero
        QGLTextureCache::instance()->insert(q, key, texture, 0);

        glBindTexture(GL_TEXTURE_2D, textureId);
    } else
        glDeleteTextures(1, &textureId);

    return texture;
}


void QGLContextPrivate::destroyGlSurfaceForPixmap(QPixmapData* pmd)
{
    Q_ASSERT(pmd->classId() == QPixmapData::X11Class);
    QX11PixmapData *pixmapData = static_cast<QX11PixmapData*>(pmd);
    if (pixmapData->gl_surface) {
        EGLBoolean success;
        success = eglDestroySurface(QEgl::display(), (EGLSurface)pixmapData->gl_surface);
        if (success == EGL_FALSE) {
            qWarning() << "destroyGlSurfaceForPixmap() - Error deleting surface: "
                       << QEgl::errorString();
        }
        pixmapData->gl_surface = 0;
    }
}

void QGLContextPrivate::unbindPixmapFromTexture(QPixmapData* pmd)
{
    Q_ASSERT(pmd->classId() == QPixmapData::X11Class);
    QX11PixmapData *pixmapData = static_cast<QX11PixmapData*>(pmd);
    if (pixmapData->gl_surface) {
        EGLBoolean success;
        success = eglReleaseTexImage(QEgl::display(),
                                     (EGLSurface)pixmapData->gl_surface,
                                     EGL_BACK_BUFFER);
        if (success == EGL_FALSE) {
            qWarning() << "unbindPixmapFromTexture() - Unable to release bound texture: "
                       << QEgl::errorString();
        }
    }
}

QT_END_NAMESPACE
