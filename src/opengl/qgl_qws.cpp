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

#include "qgl.h"
#include "qgl_egl_p.h"
#include "qglpixelbuffer.h"

#include <qglscreen_qws.h>
#include <qscreenproxy_qws.h>
#include <private/qglwindowsurface_qws_p.h>

#include <private/qbackingstore_p.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qgl_p.h>
#include <private/qpaintengine_opengl_p.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qstack.h>
#include <qdesktopwidget.h>
#include <qdebug.h>
#include <qvarlengtharray.h>

QT_BEGIN_NAMESPACE

static QGLScreen *glScreenForDevice(QPaintDevice *device)
{
    QScreen *screen = qt_screen;
    if (screen->classId() == QScreen::MultiClass) {
        int screenNumber;
        if (device && device->devType() == QInternal::Widget)
            screenNumber = qApp->desktop()->screenNumber(static_cast<QWidget *>(device));
        else
            screenNumber = 0;
        screen = screen->subScreens()[screenNumber];
    }
    while (screen->classId() == QScreen::ProxyClass ||
           screen->classId() == QScreen::TransformedClass) {
        screen = static_cast<QProxyScreen *>(screen)->screen();
    }
    if (screen->classId() == QScreen::GLClass)
        return static_cast<QGLScreen *>(screen);
    else
        return 0;
}

/*
    QGLTemporaryContext implementation
*/

class QGLTemporaryContextPrivate
{
public:
    QGLWidget *widget;
};

QGLTemporaryContext::QGLTemporaryContext(bool, QWidget *)
    : d(new QGLTemporaryContextPrivate)
{
    d->widget = new QGLWidget;
    d->widget->makeCurrent();
}

QGLTemporaryContext::~QGLTemporaryContext()
{
    delete d->widget;
}

/*****************************************************************************
  QOpenGL debug facilities
 *****************************************************************************/
//#define DEBUG_OPENGL_REGION_UPDATE

bool QGLFormat::hasOpenGLOverlays()
{
    QGLScreen *glScreen = glScreenForDevice(0);
    if (glScreen)
        return (glScreen->options() & QGLScreen::Overlays);
    else
        return false;
}

static EGLSurface qt_egl_create_surface
    (QEglContext *context, QPaintDevice *device,
     const QEglProperties *properties = 0)
{
    // Get the screen surface functions, which are used to create native ids.
    QGLScreen *glScreen = glScreenForDevice(device);
    if (!glScreen)
        return EGL_NO_SURFACE;
    QGLScreenSurfaceFunctions *funcs = glScreen->surfaceFunctions();
    if (!funcs)
        return EGL_NO_SURFACE;

    // Create the native drawable for the paint device.
    int devType = device->devType();
    EGLNativePixmapType pixmapDrawable = 0;
    EGLNativeWindowType windowDrawable = 0;
    bool ok;
    if (devType == QInternal::Pixmap) {
        ok = funcs->createNativePixmap(static_cast<QPixmap *>(device), &pixmapDrawable);
    } else if (devType == QInternal::Image) {
        ok = funcs->createNativeImage(static_cast<QImage *>(device), &pixmapDrawable);
    } else {
        ok = funcs->createNativeWindow(static_cast<QWidget *>(device), &windowDrawable);
    }
    if (!ok) {
        qWarning("QEglContext::createSurface(): Cannot create the native EGL drawable");
        return EGL_NO_SURFACE;
    }

    // Create the EGL surface to draw into, based on the native drawable.
    const int *props;
    if (properties)
        props = properties->properties();
    else
        props = 0;
    EGLSurface surf;
    if (devType == QInternal::Widget) {
        surf = eglCreateWindowSurface
            (context->display(), context->config(), windowDrawable, props);
    } else {
        surf = eglCreatePixmapSurface
            (context->display(), context->config(), pixmapDrawable, props);
    }
    if (surf == EGL_NO_SURFACE)
        qWarning("QEglContext::createSurface(): Unable to create EGL surface, error = 0x%x", eglGetError());
    return surf;
}

bool QGLContext::chooseContext(const QGLContext* shareContext)
{
    Q_D(QGLContext);

    // Validate the device.
    if (!device())
        return false;
    int devType = device()->devType();
    if (devType != QInternal::Pixmap && devType != QInternal::Image && devType != QInternal::Widget) {
        qWarning("QGLContext::chooseContext(): Cannot create QGLContext's for paint device type %d", devType);
        return false;
    }

    // Get the display and initialize it.
    d->eglContext = new QEglContext();
    d->ownsEglContext = true;
    d->eglContext->setApi(QEgl::OpenGL);

    // Construct the configuration we need for this surface.
    QEglProperties configProps;
    qt_eglproperties_set_glformat(configProps, d->glFormat);
    configProps.setDeviceType(devType);
    configProps.setPaintDeviceFormat(device());
    configProps.setRenderableType(QEgl::OpenGL);

    // Search for a matching configuration, reducing the complexity
    // each time until we get something that matches.
    if (!d->eglContext->chooseConfig(configProps)) {
        delete d->eglContext;
        d->eglContext = 0;
        return false;
    }

    // Inform the higher layers about the actual format properties.
    qt_glformat_from_eglconfig(d->glFormat, d->eglContext->config());

    // Create a new context for the configuration.
    if (!d->eglContext->createContext
            (shareContext ? shareContext->d_func()->eglContext : 0)) {
        delete d->eglContext;
        d->eglContext = 0;
        return false;
    }
    d->sharing = d->eglContext->isSharing();
    if (d->sharing && shareContext)
        const_cast<QGLContext *>(shareContext)->d_func()->sharing = true;

#if defined(EGL_VERSION_1_1)
    if (d->glFormat.swapInterval() != -1 && devType == QInternal::Widget)
        eglSwapInterval(d->eglContext->display(), d->glFormat.swapInterval());
#endif

    // Create the EGL surface to draw into.  We cannot use
    // QEglContext::createSurface() because it does not have
    // access to the QGLScreen.
    d->eglSurface = qt_egl_create_surface(d->eglContext, device());
    if (d->eglSurface == EGL_NO_SURFACE) {
        delete d->eglContext;
        d->eglContext = 0;
        return false;
    }

    return true;
}


bool QGLWidget::event(QEvent *e)
{
    return QWidget::event(e);
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
    if(context == 0) {
        qWarning("QGLWidget::setContext: Cannot set null context");
        return;
    }

    if(d->glcx)
        d->glcx->doneCurrent();
    QGLContext* oldcx = d->glcx;
    d->glcx = context;
    if(!d->glcx->isValid())
        d->glcx->create(shareContext ? shareContext : oldcx);
    if(deleteOldContext)
        delete oldcx;
}

void QGLWidgetPrivate::init(QGLContext *context, const QGLWidget* shareWidget)
{
    Q_Q(QGLWidget);

    QGLScreen *glScreen = glScreenForDevice(q);
    if (glScreen) {
        wsurf = static_cast<QWSGLWindowSurface*>(glScreen->createSurface(q));
        q->setWindowSurface(wsurf);
    }

    initContext(context, shareWidget);

    if(q->isValid() && glcx->format().hasOverlay()) {
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

QT_END_NAMESPACE
