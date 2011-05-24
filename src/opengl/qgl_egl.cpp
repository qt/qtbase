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

#include <QtCore/qdebug.h>
#include <QtOpenGL/qgl.h>
#include <QtOpenGL/qglpixelbuffer.h>
#include "qgl_p.h"
#include "qgl_egl_p.h"
#include "qglpixelbuffer_p.h"

#ifdef Q_WS_X11
#include <QtGui/private/qpixmap_x11_p.h>
#endif

QT_BEGIN_NAMESPACE

QEglProperties *QGLContextPrivate::extraWindowSurfaceCreationProps = NULL;

void qt_eglproperties_set_glformat(QEglProperties& eglProperties, const QGLFormat& glFormat)
{
    int redSize     = glFormat.redBufferSize();
    int greenSize   = glFormat.greenBufferSize();
    int blueSize    = glFormat.blueBufferSize();
    int alphaSize   = glFormat.alphaBufferSize();
    int depthSize   = glFormat.depthBufferSize();
    int stencilSize = glFormat.stencilBufferSize();
    int sampleCount = glFormat.samples();

    // QGLFormat uses a magic value of -1 to indicate "don't care", even when a buffer of that
    // type has been requested. So we must check QGLFormat's booleans too if size is -1:
    if (glFormat.alpha() && alphaSize <= 0)
        alphaSize = 1;
    if (glFormat.depth() && depthSize <= 0)
        depthSize = 1;
    if (glFormat.stencil() && stencilSize <= 0)
        stencilSize = 1;
    if (glFormat.sampleBuffers() && sampleCount <= 0)
        sampleCount = 1;

    // We want to make sure 16-bit configs are chosen over 32-bit configs as they will provide
    // the best performance. The EGL config selection algorithm is a bit stange in this regard:
    // The selection criteria for EGL_BUFFER_SIZE is "AtLeast", so we can't use it to discard
    // 32-bit configs completely from the selection. So it then comes to the sorting algorithm.
    // The red/green/blue sizes have a sort priority of 3, so they are sorted by first. The sort
    // order is special and described as "by larger _total_ number of color bits.". So EGL will
    // put 32-bit configs in the list before the 16-bit configs. However, the spec also goes on
    // to say "If the requested number of bits in attrib_list for a particular component is 0,
    // then the number of bits for that component is not considered". This part of the spec also
    // seems to imply that setting the red/green/blue bits to zero means none of the components
    // are considered and EGL disregards the entire sorting rule. It then looks to the next
    // highest priority rule, which is EGL_BUFFER_SIZE. Despite the selection criteria being
    // "AtLeast" for EGL_BUFFER_SIZE, it's sort order is "smaller" meaning 16-bit configs are
    // put in the list before 32-bit configs. So, to make sure 16-bit is preffered over 32-bit,
    // we must set the red/green/blue sizes to zero. This has an unfortunate consequence that
    // if the application sets the red/green/blue size to 5/6/5 on the QGLFormat, they will
    // probably get a 32-bit config, even when there's an RGB565 config available. Oh well.

    // Now normalize the values so -1 becomes 0
    redSize   = redSize   > 0 ? redSize   : 0;
    greenSize = greenSize > 0 ? greenSize : 0;
    blueSize  = blueSize  > 0 ? blueSize  : 0;
    alphaSize = alphaSize > 0 ? alphaSize : 0;
    depthSize = depthSize > 0 ? depthSize : 0;
    stencilSize = stencilSize > 0 ? stencilSize : 0;
    sampleCount = sampleCount > 0 ? sampleCount : 0;

    eglProperties.setValue(EGL_RED_SIZE,   redSize);
    eglProperties.setValue(EGL_GREEN_SIZE, greenSize);
    eglProperties.setValue(EGL_BLUE_SIZE,  blueSize);
    eglProperties.setValue(EGL_ALPHA_SIZE, alphaSize);
    eglProperties.setValue(EGL_DEPTH_SIZE, depthSize);
    eglProperties.setValue(EGL_STENCIL_SIZE, stencilSize);
    eglProperties.setValue(EGL_SAMPLES, sampleCount);
    eglProperties.setValue(EGL_SAMPLE_BUFFERS, sampleCount ? 1 : 0);
}

// Updates "format" with the parameters of the selected configuration.
void qt_glformat_from_eglconfig(QGLFormat& format, const EGLConfig config)
{
    EGLint redSize     = 0;
    EGLint greenSize   = 0;
    EGLint blueSize    = 0;
    EGLint alphaSize   = 0;
    EGLint depthSize   = 0;
    EGLint stencilSize = 0;
    EGLint sampleCount = 0;
    EGLint level       = 0;

    EGLDisplay display = QEgl::display();
    eglGetConfigAttrib(display, config, EGL_RED_SIZE,     &redSize);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE,   &greenSize);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE,    &blueSize);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE,   &alphaSize);
    eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE,   &depthSize);
    eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &stencilSize);
    eglGetConfigAttrib(display, config, EGL_SAMPLES,      &sampleCount);
    eglGetConfigAttrib(display, config, EGL_LEVEL,        &level);

    format.setRedBufferSize(redSize);
    format.setGreenBufferSize(greenSize);
    format.setBlueBufferSize(blueSize);
    format.setAlphaBufferSize(alphaSize);
    format.setDepthBufferSize(depthSize);
    format.setStencilBufferSize(stencilSize);
    format.setSamples(sampleCount);
    format.setPlane(level);
    format.setDirectRendering(true); // All EGL contexts are direct-rendered
    format.setRgba(true);            // EGL doesn't support colour index rendering
    format.setStereo(false);         // EGL doesn't support stereo buffers
    format.setAccumBufferSize(0);    // EGL doesn't support accululation buffers
    format.setDoubleBuffer(true);    // We don't support single buffered EGL contexts

    // Clear the EGL error state because some of the above may
    // have errored out because the attribute is not applicable
    // to the surface type.  Such errors don't matter.
    eglGetError();
}

bool QGLFormat::hasOpenGL()
{
    return true;
}

void QGLContext::reset()
{
    Q_D(QGLContext);
    if (!d->valid)
        return;
    d->cleanup();
    doneCurrent();
    if (d->eglContext && d->ownsEglContext) {
        d->destroyEglSurfaceForDevice();
        delete d->eglContext;
    }
    d->ownsEglContext = false;
    d->eglContext = 0;
    d->eglSurface = EGL_NO_SURFACE;
    d->crWin = false;
    d->sharing = false;
    d->valid = false;
    d->transpColor = QColor();
    d->initDone = false;
    QGLContextGroup::removeShare(this);
}

void QGLContext::makeCurrent()
{
    Q_D(QGLContext);
    if (!d->valid || !d->eglContext || d->eglSurfaceForDevice() == EGL_NO_SURFACE) {
        qWarning("QGLContext::makeCurrent(): Cannot make invalid context current");
        return;
    }

    if (d->eglContext->makeCurrent(d->eglSurfaceForDevice())) {
        QGLContextPrivate::setCurrentContext(this);
        if (!d->workaroundsCached) {
            d->workaroundsCached = true;
            const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
            if (renderer && (strstr(renderer, "SGX") || strstr(renderer, "MBX"))) {
                // PowerVR MBX/SGX chips needs to clear all buffers when starting to render
                // a new frame, otherwise there will be a performance penalty to pay for
                // each frame.
                qDebug() << "Found SGX/MBX driver, enabling FullClearOnEveryFrame";
                d->workaround_needsFullClearOnEveryFrame = true;

                // Older PowerVR SGX drivers (like the one in the N900) have a
                // bug which prevents glCopyTexSubImage2D() to work with a POT
                // or GL_ALPHA texture bound to an FBO. The only way to
                // identify that driver is to check the EGL version number for it.
                const char *egl_version = eglQueryString(d->eglContext->display(), EGL_VERSION);

                if (egl_version && strstr(egl_version, "1.3")) {
                    qDebug() << "Found v1.3 driver, enabling brokenFBOReadBack";
                    d->workaround_brokenFBOReadBack = true;
                } else if (egl_version && strstr(egl_version, "1.4")) {
                    qDebug() << "Found v1.4 driver, enabling brokenTexSubImage";
                    d->workaround_brokenTexSubImage = true;

                    // this is a bit complicated; 1.4 version SGX drivers from
                    // Nokia have fixed the brokenFBOReadBack problem, but
                    // official drivers from TI haven't, meaning that things
                    // like the beagleboard are broken unless we hack around it
                    // - but at the same time, we want to not reduce performance
                    // by not enabling this elsewhere.
                    //
                    // so, let's check for a Nokia-specific addon, and only
                    // enable if it isn't present.
                    // (see MeeGo bug #5616)
                    if (!QEgl::hasExtension("EGL_NOK_image_shared")) {
                        // no Nokia extension, this is probably a standard SGX
                        // driver, so enable the workaround
                        qDebug() << "Found non-Nokia v1.4 driver, enabling brokenFBOReadBack";
                        d->workaround_brokenFBOReadBack = true;
                    }
                }
            }
        }
    }
}

void QGLContext::doneCurrent()
{
    Q_D(QGLContext);
    if (d->eglContext)
        d->eglContext->doneCurrent();

    QGLContextPrivate::setCurrentContext(0);
}


void QGLContext::swapBuffers() const
{
    Q_D(const QGLContext);
    if (!d->valid || !d->eglContext)
        return;

    d->eglContext->swapBuffers(d->eglSurfaceForDevice());
}

void QGLContextPrivate::destroyEglSurfaceForDevice()
{
    if (eglSurface != EGL_NO_SURFACE) {
#if defined(Q_WS_X11) || defined(Q_OS_SYMBIAN)
        // Make sure we don't call eglDestroySurface on a surface which
        // was created for a different winId. This applies only to QGLWidget
        // paint device, so make sure this is the one we're operating on
        // (as opposed to a QGLWindowSurface use case).
        if (paintDevice && paintDevice->devType() == QInternal::Widget) {
            QWidget *w = static_cast<QWidget *>(paintDevice);
            if (QGLWidget *wgl = qobject_cast<QGLWidget *>(w)) {
                if (wgl->d_func()->eglSurfaceWindowId != wgl->winId()) {
                    qWarning("WARNING: Potential EGL surface leak! Not destroying surface.");
                    eglSurface = EGL_NO_SURFACE;
                    return;
                }
            }
        }
#endif
        eglDestroySurface(eglContext->display(), eglSurface);
        eglSurface = EGL_NO_SURFACE;
    }
}

EGLSurface QGLContextPrivate::eglSurfaceForDevice() const
{
    // If a QPixmapData had to create the QGLContext, we don't have a paintDevice
    if (!paintDevice)
        return eglSurface;

#ifdef Q_WS_X11
    if (paintDevice->devType() == QInternal::Pixmap) {
        QPixmapData *pmd = static_cast<QPixmap*>(paintDevice)->data_ptr().data();
        if (pmd->classId() == QPixmapData::X11Class) {
            QX11PixmapData* x11PixmapData = static_cast<QX11PixmapData*>(pmd);
            return (EGLSurface)x11PixmapData->gl_surface;
        }
    }
#endif

    if (paintDevice->devType() == QInternal::Pbuffer) {
        QGLPixelBuffer* pbuf = static_cast<QGLPixelBuffer*>(paintDevice);
        return pbuf->d_func()->pbuf;
    }

    return eglSurface;
}

void QGLContextPrivate::swapRegion(const QRegion &region)
{
    if (!valid || !eglContext)
        return;

    eglContext->swapBuffersRegion2NOK(eglSurfaceForDevice(), &region);
}

void QGLContextPrivate::setExtraWindowSurfaceCreationProps(QEglProperties *props)
{
    extraWindowSurfaceCreationProps = props;
}

void QGLWidget::setMouseTracking(bool enable)
{
    QWidget::setMouseTracking(enable);
}

QColor QGLContext::overlayTransparentColor() const
{
    return d_func()->transpColor;
}

uint QGLContext::colorIndex(const QColor &c) const
{
    Q_UNUSED(c);
    return 0;
}

void QGLContext::generateFontDisplayLists(const QFont & fnt, int listBase)
{
    Q_UNUSED(fnt);
    Q_UNUSED(listBase);
}

void *QGLContext::getProcAddress(const QString &proc) const
{
    return (void*)eglGetProcAddress(reinterpret_cast<const char *>(proc.toLatin1().data()));
}

bool QGLWidgetPrivate::renderCxPm(QPixmap*)
{
    return false;
}

QT_END_NAMESPACE
