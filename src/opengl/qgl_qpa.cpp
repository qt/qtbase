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

#include <QApplication>
#include <QtGui/private/qapplication_p.h>
#include <QPixmap>
#include <QDebug>

#include <QtGui/private/qapplication_p.h>
#include <QtGui/QPlatformGLContext>
#include <QtGui/QPlatformWindow>
#include <QtGui/QWindowContext>

#include "qgl.h"
#include "qgl_p.h"

QT_BEGIN_NAMESPACE

QGLFormat QGLFormat::fromWindowFormat(const QWindowFormat &format)
{
    QGLFormat retFormat;
    if (format.alphaBufferSize() >= 0)
        retFormat.setAlphaBufferSize(format.alphaBufferSize());
    if (format.blueBufferSize() >= 0)
        retFormat.setBlueBufferSize(format.blueBufferSize());
    if (format.greenBufferSize() >= 0)
        retFormat.setGreenBufferSize(format.greenBufferSize());
    if (format.redBufferSize() >= 0)
        retFormat.setRedBufferSize(format.redBufferSize());
    if (format.depthBufferSize() >= 0)
        retFormat.setDepthBufferSize(format.depthBufferSize());
    if (format.samples() > 1) {
        retFormat.setSampleBuffers(format.samples());
        retFormat.setSamples(true);
    }
    if (format.stencilBufferSize() > 0) {
        retFormat.setStencil(true);
        retFormat.setStencilBufferSize(format.stencilBufferSize());
    }
    retFormat.setDoubleBuffer(format.swapBehavior() != QWindowFormat::SingleBuffer);
    retFormat.setStereo(format.stereo());
    return retFormat;
}

QWindowFormat QGLFormat::toWindowFormat(const QGLFormat &format)
{
    QWindowFormat retFormat;
    if (format.alphaBufferSize() >= 0)
        retFormat.setAlphaBufferSize(format.alphaBufferSize());
    if (format.blueBufferSize() >= 0)
        retFormat.setBlueBufferSize(format.blueBufferSize());
    if (format.greenBufferSize() >= 0)
        retFormat.setGreenBufferSize(format.greenBufferSize());
    if (format.redBufferSize() >= 0)
        retFormat.setRedBufferSize(format.redBufferSize());
    if (format.depthBufferSize() >= 0)
        retFormat.setDepthBufferSize(format.depthBufferSize());
    retFormat.setSwapBehavior(format.doubleBuffer() ? QWindowFormat::DoubleBuffer : QWindowFormat::DefaultSwapBehavior);
    if (format.sampleBuffers() && format.samples() > 1)
        retFormat.setSamples(format.samples());
    if (format.stencil() && format.stencilBufferSize() > 0)
        retFormat.setStencilBufferSize(format.stencilBufferSize());
    retFormat.setStereo(format.stereo());
    return retFormat;
}

void QGLContextPrivate::setupSharing() {
    Q_Q(QGLContext);
    QWindowContext *sharedContext = windowContext->handle()->windowFormat().sharedContext();
    if (sharedContext) {
        QGLContext *actualSharedContext = QGLContext::fromWindowContext(sharedContext);
        sharing = true;
        QGLContextGroup::addShare(q,actualSharedContext);
    }
}

bool QGLFormat::hasOpenGL()
{
    return QApplicationPrivate::platformIntegration()
            ->hasCapability(QPlatformIntegration::OpenGL);
}

void qDeleteQGLContext(void *handle)
{
    QGLContext *context = static_cast<QGLContext *>(handle);
    delete context;
}

bool QGLContext::chooseContext(const QGLContext* shareContext)
{
    Q_D(QGLContext);
    if(!d->paintDevice || d->paintDevice->devType() != QInternal::Widget) {
        d->valid = false;
    }else {
        QWidget *widget = static_cast<QWidget *>(d->paintDevice);
        if (!widget->windowHandle()->handle()) {
            QGLFormat glformat = format();
            QWindowFormat winFormat = QGLFormat::toWindowFormat(glformat);
            if (shareContext) {
                winFormat.setSharedContext(shareContext->d_func()->windowContext);
            }
            widget->windowHandle()->setSurfaceType(QWindow::OpenGLSurface);
            winFormat.setWindowSurface(false);
            widget->windowHandle()->setWindowFormat(winFormat);
            widget->winId();//make window
        }
        d->windowContext = widget->windowHandle()->glContext();
        Q_ASSERT(d->windowContext);
        d->glFormat = QGLFormat::fromWindowFormat(d->windowContext->handle()->windowFormat());
        d->valid =(bool) d->windowContext;
        if (d->valid) {
            d->windowContext->setQGLContextHandle(this,qDeleteQGLContext);
        }
        d->setupSharing();
    }


    return d->valid;
}

void QGLContext::reset()
{
    Q_D(QGLContext);
    if (!d->valid)
        return;
    d->cleanup();

    d->crWin = false;
    d->sharing = false;
    d->valid = false;
    d->transpColor = QColor();
    d->initDone = false;
    QGLContextGroup::removeShare(this);
    if (d->windowContext) {
        d->windowContext->setQGLContextHandle(0,0);
    }
}

void QGLContext::makeCurrent()
{
    Q_D(QGLContext);
    d->windowContext->makeCurrent();

    if (!d->workaroundsCached) {
        d->workaroundsCached = true;
        const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        if (renderer && strstr(renderer, "Mali")) {
            d->workaround_brokenFBOReadBack = true;
        }
    }

}

void QGLContext::doneCurrent()
{
    Q_D(QGLContext);
    d->windowContext->doneCurrent();
}

void QGLContext::swapBuffers() const
{
    Q_D(const QGLContext);
    d->windowContext->swapBuffers();
}

void *QGLContext::getProcAddress(const QString &procName) const
{
    Q_D(const QGLContext);
    return (void *)d->windowContext->getProcAddress(procName.toAscii());
}

void QGLWidget::setContext(QGLContext *context,
                            const QGLContext* shareContext,
                            bool deleteOldContext)
{
    Q_D(QGLWidget);
    if (context == 0) {
        qWarning("QGLWidget::setContext: Cannot set null context");
        return;
    }

    if (context->device() == 0) // a context may refere to more than 1 window.
        context->setDevice(this); //but its better to point to 1 of them than none of them.

    QGLContext* oldcx = d->glcx;
    d->glcx = context;

    if (!d->glcx->isValid())
        d->glcx->create(shareContext ? shareContext : oldcx);

    if (deleteOldContext)
        delete oldcx;
}

void QGLWidgetPrivate::init(QGLContext *context, const QGLWidget *shareWidget)
{
    initContext(context, shareWidget);
}

bool QGLFormat::hasOpenGLOverlays()
{
    return false;
}

QColor QGLContext::overlayTransparentColor() const
{
    return QColor(); // Invalid color
}

uint QGLContext::colorIndex(const QColor&) const
{
    return 0;
}

void QGLContext::generateFontDisplayLists(const QFont & fnt, int listBase)
{
    Q_UNUSED(fnt);
    Q_UNUSED(listBase);
}

/*
    QGLTemporaryContext implementation
*/
class QGLTemporaryContextPrivate
{
public:
    QWindow *window;
    QWindowContext *context;
};

QGLTemporaryContext::QGLTemporaryContext(bool, QWidget *)
    : d(new QGLTemporaryContextPrivate)
{
    d->context = const_cast<QWindowContext *>(QWindowContext::currentContext());
    if (d->context)
        d->context->doneCurrent();

    d->window = new QWindow;
    d->window->setGeometry(QRect(0, 0, 3, 3));
    d->window->setSurfaceType(QWindow::OpenGLSurface);
    d->window->create();

    d->window->glContext()->makeCurrent();
}

QGLTemporaryContext::~QGLTemporaryContext()
{
    if (d->context)
        d->context->makeCurrent();
    else
        d->window->glContext()->doneCurrent();
    delete d->window;
}


bool QGLWidgetPrivate::renderCxPm(QPixmap*)
{
    return false;
}

/*! \internal
  Free up any allocated colormaps. This fn is only called for
  top-level widgets.
*/
void QGLWidgetPrivate::cleanupColormaps()
{
}

void QGLWidget::setMouseTracking(bool enable)
{
    Q_UNUSED(enable);
}

bool QGLWidget::event(QEvent *e)
{
    return QWidget::event(e);
}

void QGLWidget::resizeEvent(QResizeEvent *e)
{
    Q_D(QGLWidget);

    QWidget::resizeEvent(e);
    if (!isValid())
        return;
    makeCurrent();
    if (!d->glcx->initialized())
        glInit();
    resizeGL(width(), height());
}


const QGLContext* QGLWidget::overlayContext() const
{
    return 0;
}

void QGLWidget::makeOverlayCurrent()
{
}


void QGLWidget::updateOverlayGL()
{
}

const QGLColormap & QGLWidget::colormap() const
{
    Q_D(const QGLWidget);
    return d->cmap;
}

void QGLWidget::setColormap(const QGLColormap & c)
{
    Q_UNUSED(c);
}

QGLContext::QGLContext(QWindowContext *windowContext)
    : d_ptr(new QGLContextPrivate(this))
{
    Q_D(QGLContext);
    d->init(0,QGLFormat::fromWindowFormat(windowContext->handle()->windowFormat()));
    d->windowContext = windowContext;
    d->windowContext->setQGLContextHandle(this,qDeleteQGLContext);
    d->valid = true;
    d->setupSharing();
}

QGLContext *QGLContext::fromWindowContext(QWindowContext *windowContext)
{
    if (!windowContext)
        return 0;
    if (windowContext->qGLContextHandle()) {
        return reinterpret_cast<QGLContext *>(windowContext->qGLContextHandle());
    }
    QGLContext *glContext = new QGLContext(windowContext);
    //Dont call create on context. This can cause the platformFormat to be set on the widget, which
    //will cause the platformWindow to be recreated.
    return glContext;
}

QT_END_NAMESPACE
