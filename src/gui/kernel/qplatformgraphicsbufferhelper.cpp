/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtPlatformSupport module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/qpa/qplatformgraphicsbuffer.h>

#include "qplatformgraphicsbufferhelper.h"
#include <QtCore/QDebug>
#include <QtGui/qopengl.h>
#include <QtGui/QImage>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

QT_BEGIN_NAMESPACE

/*!
    Convenience function to both lock and bind the buffer to a texture. This
    function will first try and lock with texture read and texture write
    access. If this succeeds it will use the bindToTexture function to bind the
    content to the currently bound texture. If this fail it will try and lock
    with SWReadAccess and then use the bindSWToTexture convenience function.

    \a swizzle is suppose to be used by the caller to figure out if the Red and
    Blue color channels need to be swizzled when rendering.

    \a rect is the subrect which is desired to be bounded to the texture. This
    argument has a no less than semantic, meaning more (if not all) of the buffer
    can be bounded to the texture. An empty QRect is interpreted as entire buffer
    should be bound.

    The user should use the AccessTypes returned by isLocked to figure out what
    lock has been obtained.

    returns true if the buffer has successfully been bound to the currently
    bound texture, otherwise returns false.
*/
bool QPlatformGraphicsBufferHelper::lockAndBindToTexture(QPlatformGraphicsBuffer *graphicsBuffer,
                                                         bool *swizzle,
                                                         const QRect &rect)
{
    if (graphicsBuffer->lock(QPlatformGraphicsBuffer::TextureAccess)) {
        if (!graphicsBuffer->bindToTexture(rect)) {
            qWarning() << Q_FUNC_INFO << "Failed to bind graphicsbuffer to texture";
            return false;
        }
        if (swizzle)
            *swizzle = false;
    } else if (graphicsBuffer->lock(QPlatformGraphicsBuffer::SWReadAccess)) {
        if (!bindSWToTexture(graphicsBuffer, swizzle, rect)) {
            qWarning() << Q_FUNC_INFO << "Failed to bind SW graphcisbuffer to texture";
            return false;
        }
    } else {
        qWarning() << Q_FUNC_INFO << "Failed to lock";
        return false;
    }
    return true;
}

/*!
    Convenience function that uploads the current raster content to the currently bound texture.

    \a swizzleRandB is suppose to be used by the caller to figure out if the Red and
    Blue color channels need to be swizzled when rendering. This is an
    optimization. Qt often renders to software buffers interpreting pixels as
    unsigned ints. When these buffers are uploaded to textures and each color
    channel per pixel is interpreted as a byte (read sequentially), then the
    Red and Blue channels are swapped. Conveniently the Alpha buffer will be
    correct since Qt historically has had the alpha channel as the first
    channel, while OpenGL typically expects the alpha channel to be the last
    channel.

    \a subRect is the subrect which is desired to be bounded to the texture. This
    argument has a no less than semantic, meaning more (if not all) of the buffer
    can be bounded to the texture. An empty QRect is interpreted as entire buffer
    should be bound.

    This function fails for buffers not capable of locking to SWAccess.

    Returns true on success, otherwise false.
*/
bool QPlatformGraphicsBufferHelper::bindSWToTexture(const QPlatformGraphicsBuffer *graphicsBuffer,
                                                    bool *swizzleRandB,
                                                    const QRect &subRect)
{
#ifndef QT_NO_OPENGL
    if (!QOpenGLContext::currentContext())
        return false;

    if (!(graphicsBuffer->isLocked() & QPlatformGraphicsBuffer::SWReadAccess))
        return false;

    QSize size = graphicsBuffer->size();

    Q_ASSERT(subRect.isEmpty() || QRect(QPoint(0,0), size).contains(subRect));

    bool swizzle = false;
    QImage::Format imageformat = QImage::toImageFormat(graphicsBuffer->format());
    QImage image(graphicsBuffer->data(), size.width(), size.height(), graphicsBuffer->bytesPerLine(), imageformat);
    if (graphicsBuffer->bytesPerLine() != (size.width() * 4)) {
        image = image.convertToFormat(QImage::Format_RGBA8888);
    } else if (imageformat == QImage::Format_RGB32) {
        swizzle = true;
    } else if (imageformat != QImage::Format_RGBA8888) {
        image = image.convertToFormat(QImage::Format_RGBA8888);
    }

    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();

    QRect rect = subRect;
    if (rect.isNull() || rect == QRect(QPoint(0,0),size)) {
        funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.constBits());
    } else {
#ifndef QT_OPENGL_ES_2
        if (!QOpenGLContext::currentContext()->isOpenGLES()) {
            funcs->glPixelStorei(GL_UNPACK_ROW_LENGTH, image.width());
            funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                                   image.constScanLine(rect.y()) + rect.x() * 4);
            funcs->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        } else
#endif
        {
            // if the rect is wide enough it's cheaper to just
            // extend it instead of doing an image copy
            if (rect.width() >= size.width() / 2) {
                rect.setX(0);
                rect.setWidth(size.width());
            }

            // if the sub-rect is full-width we can pass the image data directly to
            // OpenGL instead of copying, since there's no gap between scanlines

            if (rect.width() == size.width()) {
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                                       image.constScanLine(rect.y()));
            } else {
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                                       image.copy(rect).constBits());
            }
        }
    }
    if (swizzleRandB)
        *swizzleRandB = swizzle;

    return true;

#else
    Q_UNUSED(graphicsBuffer)
    Q_UNUSED(swizzleRandB)
    Q_UNUSED(subRect)
    return false;
#endif // QT_NO_OPENGL
}

QT_END_NAMESPACE
