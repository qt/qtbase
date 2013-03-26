/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include <private/qglpaintdevice_p.h>
#include <private/qgl_p.h>
#include <private/qglpixelbuffer_p.h>
#include <private/qglframebufferobject_p.h>
#include <qopenglfunctions.h>
#include <qwindow.h>

QT_BEGIN_NAMESPACE

QGLPaintDevice::QGLPaintDevice()
    : m_thisFBO(0)
{
}

QGLPaintDevice::~QGLPaintDevice()
{
}

int QGLPaintDevice::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    switch(metric) {
    case PdmWidth:
        return size().width();
    case PdmHeight:
        return size().height();
    case PdmDepth: {
        const QGLFormat f = format();
        return f.redBufferSize() + f.greenBufferSize() + f.blueBufferSize() + f.alphaBufferSize();
    }
    case PdmDevicePixelRatio:
        return 1;
    default:
        qWarning("QGLPaintDevice::metric() - metric %d not known", metric);
        return 0;
    }
}

void QGLPaintDevice::beginPaint()
{
    // Make sure our context is the current one:
    QGLContext *ctx = context();
    ctx->makeCurrent();

    // Record the currently bound FBO so we can restore it again
    // in endPaint() and bind this device's FBO
    //
    // Note: m_thisFBO could be zero if the paint device is not
    // backed by an FBO (e.g. window back buffer).  But there could
    // be a previous FBO bound to the context which we need to
    // explicitly unbind.  Otherwise the painting will go into
    // the previous FBO instead of to the window.
    m_previousFBO = ctx->d_func()->current_fbo;

    if (m_previousFBO != m_thisFBO) {
        ctx->d_ptr->current_fbo = m_thisFBO;
        ctx->contextHandle()->functions()->glBindFramebuffer(GL_FRAMEBUFFER, m_thisFBO);
    }

    // Set the default fbo for the context to m_thisFBO so that
    // if some raw GL code between beginNativePainting() and
    // endNativePainting() calls QGLFramebufferObject::release(),
    // painting will revert to the window surface's fbo.
    ctx->d_ptr->default_fbo = m_thisFBO;
}

void QGLPaintDevice::ensureActiveTarget()
{
    QGLContext* ctx = context();
    if (ctx != QGLContext::currentContext())
        ctx->makeCurrent();

    if (ctx->d_ptr->current_fbo != m_thisFBO) {
        ctx->d_ptr->current_fbo = m_thisFBO;
        ctx->contextHandle()->functions()->glBindFramebuffer(GL_FRAMEBUFFER, m_thisFBO);
    }

    ctx->d_ptr->default_fbo = m_thisFBO;
}

void QGLPaintDevice::endPaint()
{
    // Make sure the FBO bound at beginPaint is re-bound again here:
    QGLContext *ctx = context();
    if (m_previousFBO != ctx->d_func()->current_fbo) {
        ctx->d_ptr->current_fbo = m_previousFBO;
        ctx->contextHandle()->functions()->glBindFramebuffer(GL_FRAMEBUFFER, m_previousFBO);
    }

    ctx->d_ptr->default_fbo = 0;
}

QGLFormat QGLPaintDevice::format() const
{
    return context()->format();
}

bool QGLPaintDevice::alphaRequested() const
{
    return context()->d_func()->reqFormat.alpha();
}

bool QGLPaintDevice::isFlipped() const
{
    return false;
}

////////////////// QGLWidgetGLPaintDevice //////////////////

QGLWidgetGLPaintDevice::QGLWidgetGLPaintDevice()
{
}

QPaintEngine* QGLWidgetGLPaintDevice::paintEngine() const
{
    return glWidget->paintEngine();
}

void QGLWidgetGLPaintDevice::setWidget(QGLWidget* w)
{
    glWidget = w;
}

void QGLWidgetGLPaintDevice::beginPaint()
{
    QGLPaintDevice::beginPaint();
    if (!glWidget->d_func()->disable_clear_on_painter_begin && glWidget->autoFillBackground()) {
        if (glWidget->testAttribute(Qt::WA_TranslucentBackground))
            glClearColor(0.0, 0.0, 0.0, 0.0);
        else {
            const QColor &c = glWidget->palette().brush(glWidget->backgroundRole()).color();
            float alpha = c.alphaF();
            glClearColor(c.redF() * alpha, c.greenF() * alpha, c.blueF() * alpha, alpha);
        }
        if (context()->d_func()->workaround_needsFullClearOnEveryFrame)
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        else
            glClear(GL_COLOR_BUFFER_BIT);
    }
}

void QGLWidgetGLPaintDevice::endPaint()
{
    if (glWidget->autoBufferSwap())
        glWidget->swapBuffers();
    QGLPaintDevice::endPaint();
}


QSize QGLWidgetGLPaintDevice::size() const
{
#ifdef Q_OS_MAC
    return glWidget->size() * (glWidget->windowHandle() ?
                               glWidget->windowHandle()->devicePixelRatio() : qApp->devicePixelRatio());
#else
    return glWidget->size();
#endif
}

QGLContext* QGLWidgetGLPaintDevice::context() const
{
    return const_cast<QGLContext*>(glWidget->context());
}

// returns the QGLPaintDevice for the given QPaintDevice
QGLPaintDevice* QGLPaintDevice::getDevice(QPaintDevice* pd)
{
    QGLPaintDevice* glpd = 0;

    switch(pd->devType()) {
        case QInternal::Widget:
            // Should not be called on a non-gl widget:
            Q_ASSERT(qobject_cast<QGLWidget*>(static_cast<QWidget*>(pd)));
            glpd = &(static_cast<QGLWidget*>(pd)->d_func()->glDevice);
            break;
        case QInternal::Pbuffer:
            glpd = &(static_cast<QGLPixelBuffer*>(pd)->d_func()->glDevice);
            break;
        case QInternal::FramebufferObject:
            glpd = &(static_cast<QGLFramebufferObject*>(pd)->d_func()->glDevice);
            break;
        case QInternal::Pixmap: {
            qWarning("Pixmap type not supported for GL rendering");
            break;
        }
        default:
            qWarning("QGLPaintDevice::getDevice() - Unknown device type %d", pd->devType());
            break;
    }

    return glpd;
}

QT_END_NAMESPACE
