/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <private/qopenglpaintdevice_p.h>
#include <private/qopenglcontext_p.h>
#include <private/qopenglframebufferobject_p.h>

#include <qopenglfunctions.h>

QT_BEGIN_NAMESPACE

QOpenGLPaintDevice::QOpenGLPaintDevice()
    : m_thisFBO(0)
{
}

QOpenGLPaintDevice::~QOpenGLPaintDevice()
{
}

int QOpenGLPaintDevice::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    switch(metric) {
    case PdmWidth:
        return size().width();
    case PdmHeight:
        return size().height();
    case PdmDepth: {
        const QSurfaceFormat f = format();
        return f.redBufferSize() + f.greenBufferSize() + f.blueBufferSize() + f.alphaBufferSize();
    }
    default:
        qWarning("QOpenGLPaintDevice::metric() - metric %d not known", metric);
        return 0;
    }
}

void QOpenGLPaintDevice::beginPaint()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();

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
        ctx->d_func()->current_fbo = m_thisFBO;
        QOpenGLFunctions(ctx).glBindFramebuffer(GL_FRAMEBUFFER, m_thisFBO);
    }

    // Set the default fbo for the context to m_thisFBO so that
    // if some raw GL code between beginNativePainting() and
    // endNativePainting() calls QOpenGLFramebufferObject::release(),
    // painting will revert to the window surface's fbo.
    ctx->d_func()->default_fbo = m_thisFBO;
}

void QOpenGLPaintDevice::ensureActiveTarget()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();

    if (ctx->d_func()->current_fbo != m_thisFBO) {
        ctx->d_func()->current_fbo = m_thisFBO;
        QOpenGLFunctions(ctx).glBindFramebuffer(GL_FRAMEBUFFER, m_thisFBO);
    }

    ctx->d_func()->default_fbo = m_thisFBO;
}

void QOpenGLPaintDevice::endPaint()
{
    // Make sure the FBO bound at beginPaint is re-bound again here:
    QOpenGLContext *ctx = QOpenGLContext::currentContext();

    if (m_previousFBO != ctx->d_func()->current_fbo) {
        ctx->d_func()->current_fbo = m_previousFBO;
        QOpenGLFunctions(ctx).glBindFramebuffer(GL_FRAMEBUFFER, m_previousFBO);
    }

    ctx->d_func()->default_fbo = 0;
}

bool QOpenGLPaintDevice::isFlipped() const
{
    return false;
}

// returns the QOpenGLPaintDevice for the given QPaintDevice
QOpenGLPaintDevice* QOpenGLPaintDevice::getDevice(QPaintDevice* pd)
{
    QOpenGLPaintDevice* glpd = 0;

    switch(pd->devType()) {
        case QInternal::FramebufferObject:
            glpd = &(static_cast<QOpenGLFramebufferObject*>(pd)->d_func()->glDevice);
            break;
        case QInternal::Pixmap: {
            qWarning("Pixmap type not supported for GL rendering");
            break;
        }
        default:
            qWarning("QOpenGLPaintDevice::getDevice() - Unknown device type %d", pd->devType());
            break;
    }

    return glpd;
}

QT_END_NAMESPACE
