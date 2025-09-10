// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopengltextureuploader_p.h"

#include <qimage.h>
#include <qmath.h>
#include <qopenglfunctions.h>
#include <private/qopenglcontext_p.h>
#include <private/qopenglextensions_p.h>

#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT                     0x140B
#endif

#ifndef GL_RED
#define GL_RED                            0x1903
#endif

#ifndef GL_GREEN
#define GL_GREEN                          0x1904
#endif

#ifndef GL_BLUE
#define GL_BLUE                           0x1905
#endif

#ifndef GL_RGB10_A2
#define GL_RGB10_A2                       0x8059
#endif

#ifndef GL_RGBA16
#define GL_RGBA16                         0x805B
#endif

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#endif

#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#endif

#ifndef GL_RGBA16F
#define GL_RGBA16F                        0x881A
#endif

#ifndef GL_RGBA32F
#define GL_RGBA32F                        0x8814
#endif

#ifndef GL_TEXTURE_SWIZZLE_R
#define GL_TEXTURE_SWIZZLE_R              0x8E42
#endif

#ifndef GL_TEXTURE_SWIZZLE_G
#define GL_TEXTURE_SWIZZLE_G              0x8E43
#endif

#ifndef GL_TEXTURE_SWIZZLE_B
#define GL_TEXTURE_SWIZZLE_B              0x8E44
#endif

#ifndef GL_TEXTURE_SWIZZLE_A
#define GL_TEXTURE_SWIZZLE_A              0x8E45
#endif

#ifndef GL_SRGB
#define GL_SRGB                           0x8C40
#endif
#ifndef GL_SRGB_ALPHA
#define GL_SRGB_ALPHA                     0x8C42
#endif

QT_BEGIN_NAMESPACE

qsizetype QOpenGLTextureUploader::textureImage(GLenum target, const QImage &image, QOpenGLTextureUploader::BindOptions options, QSize maxSize)
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    QOpenGLExtensions *funcs = static_cast<QOpenGLExtensions*>(context->functions());

    QImage tx;
    GLenum externalFormat;
    GLenum internalFormat;
    GLuint pixelType;
    QImage::Format targetFormat = QImage::Format_Invalid;
    const bool isOpenGL12orBetter = !context->isOpenGLES() && (context->format().majorVersion() >= 2 || context->format().minorVersion() >= 2);
    const bool isOpenGLES3orBetter = context->isOpenGLES() && context->format().majorVersion() >= 3;
    const bool sRgbBinding = (options & SRgbBindOption);
    Q_ASSERT(isOpenGL12orBetter || context->isOpenGLES());
    Q_ASSERT((options & (SRgbBindOption | UseRedForAlphaAndLuminanceBindOption)) != (SRgbBindOption | UseRedForAlphaAndLuminanceBindOption));

    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        if (isOpenGL12orBetter) {
            externalFormat = GL_BGRA;
            internalFormat = GL_RGBA;
            pixelType = GL_UNSIGNED_INT_8_8_8_8_REV;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        // Without GL_UNSIGNED_INT_8_8_8_8_REV, BGRA only matches ARGB on little endian:
        } else if (funcs->hasOpenGLExtension(QOpenGLExtensions::BGRATextureFormat) && !sRgbBinding) {
            // The GL_EXT_texture_format_BGRA8888 extension requires the internal format to match the external.
            externalFormat = internalFormat = GL_BGRA;
            pixelType = GL_UNSIGNED_BYTE;
        } else if (context->isOpenGLES() && context->hasExtension(QByteArrayLiteral("GL_APPLE_texture_format_BGRA8888"))) {
            // Is only allowed as an external format like OpenGL.
            externalFormat = GL_BGRA;
            internalFormat = GL_RGBA;
            pixelType = GL_UNSIGNED_BYTE;
#endif
        } else if (funcs->hasOpenGLExtension(QOpenGLExtensions::TextureSwizzle)) {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
#else
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_GREEN);
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_ALPHA);
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_RED);
#endif
            externalFormat = internalFormat = GL_RGBA;
            pixelType = GL_UNSIGNED_BYTE;
        } else {
            // No support for direct ARGB32 upload.
            break;
        }
        targetFormat = image.format();
        break;
    case QImage::Format_BGR30:
    case QImage::Format_A2BGR30_Premultiplied:
        if (sRgbBinding) {
            // Always needs conversion
            break;
        } else if (isOpenGL12orBetter || isOpenGLES3orBetter) {
            pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
            externalFormat = GL_RGBA;
            internalFormat = GL_RGB10_A2;
            targetFormat =  image.format();
        }
        break;
    case QImage::Format_RGB30:
    case QImage::Format_A2RGB30_Premultiplied:
        if (sRgbBinding) {
            // Always needs conversion
            break;
        } else if (isOpenGL12orBetter) {
            pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
            externalFormat = GL_BGRA;
            internalFormat = GL_RGB10_A2;
            targetFormat = image.format();
        } else if (funcs->hasOpenGLExtension(QOpenGLExtensions::TextureSwizzle)) {
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
            pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
            externalFormat = GL_RGBA;
            internalFormat = GL_RGB10_A2;
            targetFormat = image.format();
        }
        break;
    case QImage::Format_RGB444:
    case QImage::Format_RGB555:
    case QImage::Format_RGB16:
        if (isOpenGL12orBetter || context->isOpenGLES()) {
            externalFormat = internalFormat = GL_RGB;
            pixelType = GL_UNSIGNED_SHORT_5_6_5;
            targetFormat = QImage::Format_RGB16;
        }
        break;
    case QImage::Format_RGB666:
    case QImage::Format_RGB888:
        externalFormat = internalFormat = GL_RGB;
        pixelType = GL_UNSIGNED_BYTE;
        targetFormat = QImage::Format_RGB888;
        break;
    case QImage::Format_BGR888:
        if (isOpenGL12orBetter) {
            externalFormat = GL_BGR;
            internalFormat = GL_RGB;
            pixelType = GL_UNSIGNED_BYTE;
            targetFormat = QImage::Format_BGR888;
        } else if (funcs->hasOpenGLExtension(QOpenGLExtensions::TextureSwizzle)) {
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
            funcs->glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
            externalFormat = internalFormat = GL_RGB;
            pixelType = GL_UNSIGNED_BYTE;
            targetFormat = QImage::Format_BGR888;
        }
        break;
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
        externalFormat = internalFormat = GL_RGBA;
        pixelType = GL_UNSIGNED_BYTE;
        targetFormat = image.format();
        break;
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
        externalFormat = internalFormat = GL_RGBA;
        if (isOpenGL12orBetter || (context->isOpenGLES() && context->format().majorVersion() >= 3))
            internalFormat = GL_RGBA16;
        pixelType = GL_UNSIGNED_SHORT;
        targetFormat =  image.format();
        break;
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
        if (context->format().majorVersion() >= 3) {
            externalFormat = GL_RGBA;
            internalFormat = GL_RGBA16F;
            pixelType = GL_HALF_FLOAT;
            targetFormat =  image.format();
        }
        break;
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        externalFormat = internalFormat = GL_RGBA;
        if (context->format().majorVersion() >= 3)
            internalFormat = GL_RGBA32F;
        pixelType = GL_FLOAT;
        targetFormat =  image.format();
        break;
    case QImage::Format_Indexed8:
        if (sRgbBinding) {
            // Always needs conversion
            break;
        } else if (options & UseRedForAlphaAndLuminanceBindOption) {
            externalFormat = internalFormat = GL_RED;
            pixelType = GL_UNSIGNED_BYTE;
            targetFormat = image.format();
        }
        break;
    case QImage::Format_Alpha8:
        if (sRgbBinding) {
            // Always needs conversion
            break;
        } else if (options & UseRedForAlphaAndLuminanceBindOption) {
            externalFormat = internalFormat = GL_RED;
            pixelType = GL_UNSIGNED_BYTE;
            targetFormat = image.format();
        } else if (context->isOpenGLES() || context->format().profile() != QSurfaceFormat::CoreProfile) {
            externalFormat = internalFormat = GL_ALPHA;
            pixelType = GL_UNSIGNED_BYTE;
            targetFormat = image.format();
        } else if (funcs->hasOpenGLExtension(QOpenGLExtensions::TextureSwizzle)) {
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ZERO);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ZERO);
            externalFormat = internalFormat = GL_RED;
            pixelType = GL_UNSIGNED_BYTE;
            targetFormat = image.format();
        }
        break;
    case QImage::Format_Grayscale8:
        if (sRgbBinding) {
            // Always needs conversion
            break;
        } else if (options & UseRedForAlphaAndLuminanceBindOption) {
            externalFormat = internalFormat = GL_RED;
            pixelType = GL_UNSIGNED_BYTE;
            targetFormat = image.format();
        } else if (context->isOpenGLES() || context->format().profile() != QSurfaceFormat::CoreProfile) {
            externalFormat = internalFormat = GL_LUMINANCE;
            pixelType = GL_UNSIGNED_BYTE;
            targetFormat = image.format();
        } else if (funcs->hasOpenGLExtension(QOpenGLExtensions::TextureSwizzle)) {
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            externalFormat = internalFormat = GL_RED;
            pixelType = GL_UNSIGNED_BYTE;
            targetFormat = image.format();
        }
        break;
    case QImage::Format_Grayscale16:
        if (sRgbBinding) {
            // Always needs conversion
            break;
        } else if (options & UseRedForAlphaAndLuminanceBindOption) {
            externalFormat = internalFormat = GL_RED;
            pixelType = GL_UNSIGNED_SHORT;
            targetFormat = image.format();
        } else if (context->isOpenGLES() || context->format().profile() != QSurfaceFormat::CoreProfile) {
            externalFormat = internalFormat = GL_LUMINANCE;
            pixelType = GL_UNSIGNED_SHORT;
            targetFormat = image.format();
        } else if (funcs->hasOpenGLExtension(QOpenGLExtensions::TextureSwizzle)) {
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
            externalFormat = internalFormat = GL_RED;
            pixelType = GL_UNSIGNED_SHORT;
            targetFormat = image.format();
        }
        break;
    default:
        break;
    }

    // If no direct upload was detected above, convert to RGBA8888 and upload that
    if (targetFormat == QImage::Format_Invalid) {
        externalFormat = internalFormat = GL_RGBA;
        pixelType = GL_UNSIGNED_BYTE;
        if (!image.hasAlphaChannel())
            targetFormat = QImage::Format_RGBX8888;
        else
            targetFormat = QImage::Format_RGBA8888;
    }

    if (options & PremultipliedAlphaBindOption) {
        if (targetFormat == QImage::Format_ARGB32)
            targetFormat = QImage::Format_ARGB32_Premultiplied;
        else if (targetFormat == QImage::Format_RGBA8888)
            targetFormat = QImage::Format_RGBA8888_Premultiplied;
        else if (targetFormat == QImage::Format_RGBA64)
            targetFormat = QImage::Format_RGBA64_Premultiplied;
        else if (targetFormat == QImage::Format_RGBA16FPx4)
            targetFormat = QImage::Format_RGBA16FPx4_Premultiplied;
        else if (targetFormat == QImage::Format_RGBA32FPx4)
            targetFormat = QImage::Format_RGBA32FPx4_Premultiplied;
    } else {
        if (targetFormat == QImage::Format_ARGB32_Premultiplied)
            targetFormat = QImage::Format_ARGB32;
        else if (targetFormat == QImage::Format_RGBA8888_Premultiplied)
            targetFormat = QImage::Format_RGBA8888;
        else if (targetFormat == QImage::Format_RGBA64_Premultiplied)
            targetFormat = QImage::Format_RGBA64;
        else if (targetFormat == QImage::Format_RGBA16FPx4_Premultiplied)
            targetFormat = QImage::Format_RGBA16FPx4;
        else if (targetFormat == QImage::Format_RGBA32FPx4_Premultiplied)
            targetFormat = QImage::Format_RGBA32FPx4;
    }

    if (sRgbBinding) {
        Q_ASSERT(internalFormat == GL_RGBA || internalFormat == GL_RGB);
        if (image.hasAlphaChannel())
            internalFormat = GL_SRGB_ALPHA;
        else
            internalFormat = GL_SRGB;
    }

    if (image.format() != targetFormat)
        tx = image.convertToFormat(targetFormat);
    else
        tx = image;

    QSize newSize = tx.size();
    if (!maxSize.isEmpty())
        newSize = newSize.boundedTo(maxSize);
    if (options & PowerOfTwoBindOption) {
        newSize.setWidth(qNextPowerOfTwo(newSize.width() - 1));
        newSize.setHeight(qNextPowerOfTwo(newSize.height() - 1));
    }

    if (newSize != tx.size())
        tx = tx.scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Handle cases where the QImage is actually a sub image of its image data:
    qsizetype naturalBpl = ((qsizetype(tx.width()) * tx.depth() + 31) >> 5) << 2;
    if (tx.bytesPerLine() != naturalBpl)
        tx = tx.copy(tx.rect());

    funcs->glTexImage2D(target, 0, internalFormat, tx.width(), tx.height(), 0, externalFormat, pixelType, tx.constBits());

    qsizetype cost = qint64(tx.width()) * tx.height() * tx.depth() / 8;

    return cost;
}

QT_END_NAMESPACE
