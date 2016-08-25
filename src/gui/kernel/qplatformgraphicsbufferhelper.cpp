/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPlatformSupport module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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

#ifndef GL_RGB10_A2
#define GL_RGB10_A2                       0x8059
#endif

#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#endif

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
                                                         bool *swizzle, bool *premultiplied,
                                                         const QRect &rect)
{
    if (graphicsBuffer->lock(QPlatformGraphicsBuffer::TextureAccess)) {
        if (!graphicsBuffer->bindToTexture(rect)) {
            qWarning("Failed to bind %sgraphicsbuffer to texture", "");
            return false;
        }
        if (swizzle)
            *swizzle = false;
        if (premultiplied)
            *premultiplied = false;
    } else if (graphicsBuffer->lock(QPlatformGraphicsBuffer::SWReadAccess)) {
        if (!bindSWToTexture(graphicsBuffer, swizzle, premultiplied, rect)) {
            qWarning("Failed to bind %sgraphicsbuffer to texture", "SW ");
            return false;
        }
    } else {
        qWarning("Failed to lock");
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
                                                    bool *swizzleRandB, bool *premultipliedB,
                                                    const QRect &subRect)
{
#ifndef QT_NO_OPENGL
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return false;

    if (!(graphicsBuffer->isLocked() & QPlatformGraphicsBuffer::SWReadAccess))
        return false;

    QSize size = graphicsBuffer->size();

    Q_ASSERT(subRect.isEmpty() || QRect(QPoint(0,0), size).contains(subRect));

    GLenum internalFormat = GL_RGBA;
    GLuint pixelType = GL_UNSIGNED_BYTE;

    bool needsConversion = false;
    bool swizzle = false;
    bool premultiplied = false;
    QImage::Format imageformat = QImage::toImageFormat(graphicsBuffer->format());
    QImage image(graphicsBuffer->data(), size.width(), size.height(), graphicsBuffer->bytesPerLine(), imageformat);
    if (graphicsBuffer->bytesPerLine() != (size.width() * 4)) {
        needsConversion = true;
    } else {
        switch (imageformat) {
        case QImage::Format_ARGB32_Premultiplied:
            premultiplied = true;
            Q_FALLTHROUGH();
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32:
            swizzle = true;
            break;
        case QImage::Format_RGBA8888_Premultiplied:
            premultiplied = true;
            Q_FALLTHROUGH();
        case QImage::Format_RGBX8888:
        case QImage::Format_RGBA8888:
            break;
        case QImage::Format_BGR30:
        case QImage::Format_A2BGR30_Premultiplied:
            if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
                pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
                internalFormat = GL_RGB10_A2;
                premultiplied = true;
            } else {
                needsConversion = true;
            }
            break;
        case QImage::Format_RGB30:
        case QImage::Format_A2RGB30_Premultiplied:
            if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
                pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
                internalFormat = GL_RGB10_A2;
                premultiplied = true;
                swizzle = true;
            } else {
                needsConversion = true;
            }
            break;
        default:
            needsConversion = true;
            break;
        }
    }
    if (needsConversion)
        image = image.convertToFormat(QImage::Format_RGBA8888);

    QOpenGLFunctions *funcs = ctx->functions();

    QRect rect = subRect;
    if (rect.isNull() || rect == QRect(QPoint(0,0),size)) {
        funcs->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.width(), size.height(), 0, GL_RGBA, pixelType, image.constBits());
    } else {
#ifndef QT_OPENGL_ES_2
        if (!ctx->isOpenGLES()) {
            funcs->glPixelStorei(GL_UNPACK_ROW_LENGTH, image.width());
            funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, pixelType,
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
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, pixelType,
                                       image.constScanLine(rect.y()));
            } else {
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, pixelType,
                                       image.copy(rect).constBits());
            }
        }
    }
    if (swizzleRandB)
        *swizzleRandB = swizzle;
    if (premultipliedB)
        *premultipliedB = premultiplied;

    return true;

#else
    Q_UNUSED(graphicsBuffer)
    Q_UNUSED(swizzleRandB)
    Q_UNUSED(premultipliedB)
    Q_UNUSED(subRect)
    return false;
#endif // QT_NO_OPENGL
}

QT_END_NAMESPACE
