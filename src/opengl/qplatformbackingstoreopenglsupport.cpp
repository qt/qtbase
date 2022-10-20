/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QT_NO_OPENGL

#include "qplatformbackingstoreopenglsupport.h"

#include <QtGui/private/qwindow_p.h>

#include <qpa/qplatformgraphicsbuffer.h>
#include <qpa/qplatformgraphicsbufferhelper.h>

#include <QtOpenGL/QOpenGLTextureBlitter>
#include <QtGui/qopengl.h>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOffscreenSurface>

#ifndef GL_TEXTURE_BASE_LEVEL
#define GL_TEXTURE_BASE_LEVEL             0x813C
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL              0x813D
#endif
#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#endif
#ifndef GL_RGB10_A2
#define GL_RGB10_A2                       0x8059
#endif
#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#endif

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_FRAMEBUFFER_SRGB_CAPABLE
#define GL_FRAMEBUFFER_SRGB_CAPABLE 0x8DBA
#endif

QT_BEGIN_NAMESPACE

static inline QRect deviceRect(const QRect &rect, QWindow *window)
{
    QRect deviceRect(rect.topLeft() * window->devicePixelRatio(),
                     rect.size() * window->devicePixelRatio());
    return deviceRect;
}

static inline QPoint deviceOffset(const QPoint &pt, QWindow *window)
{
    return pt * window->devicePixelRatio();
}

static QRegion deviceRegion(const QRegion &region, QWindow *window, const QPoint &offset)
{
    if (offset.isNull() && window->devicePixelRatio() <= 1)
        return region;

    QVarLengthArray<QRect, 4> rects;
    rects.reserve(region.rectCount());
    for (const QRect &rect : region)
        rects.append(deviceRect(rect.translated(offset), window));

    QRegion deviceRegion;
    deviceRegion.setRects(rects.constData(), rects.count());
    return deviceRegion;
}

static inline QRect toBottomLeftRect(const QRect &topLeftRect, int windowHeight)
{
    return QRect(topLeftRect.x(), windowHeight - topLeftRect.bottomRight().y() - 1,
                 topLeftRect.width(), topLeftRect.height());
}

static void blitTextureForWidget(const QPlatformTextureList *textures, int idx, QWindow *window, const QRect &deviceWindowRect,
                                 QOpenGLTextureBlitter *blitter, const QPoint &offset, bool canUseSrgb)
{
    const QRect clipRect = textures->clipRect(idx);
    if (clipRect.isEmpty())
        return;

    QRect rectInWindow = textures->geometry(idx);
    // relative to the TLW, not necessarily our window (if the flush is for a native child widget), have to adjust
    rectInWindow.translate(-offset);

    const QRect clippedRectInWindow = rectInWindow & clipRect.translated(rectInWindow.topLeft());
    const QRect srcRect = toBottomLeftRect(clipRect, rectInWindow.height());

    const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(deviceRect(clippedRectInWindow, window),
                                                                     deviceWindowRect);

    const QMatrix3x3 source = QOpenGLTextureBlitter::sourceTransform(deviceRect(srcRect, window),
                                                                     deviceRect(rectInWindow, window).size(),
                                                                     QOpenGLTextureBlitter::OriginBottomLeft);

    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    const bool srgb = textures->flags(idx).testFlag(QPlatformTextureList::TextureIsSrgb);
    if (srgb && canUseSrgb)
        funcs->glEnable(GL_FRAMEBUFFER_SRGB);

    blitter->blit(textures->textureId(idx), target, source);

    if (srgb && canUseSrgb)
        funcs->glDisable(GL_FRAMEBUFFER_SRGB);
}

QPlatformBackingStoreOpenGLSupport::~QPlatformBackingStoreOpenGLSupport() {
    if (context) {
        QOffscreenSurface offscreenSurface;
        offscreenSurface.setFormat(context->format());
        offscreenSurface.create();
        context->makeCurrent(&offscreenSurface);
        if (textureId)
            context->functions()->glDeleteTextures(1, &textureId);
        if (blitter)
            blitter->destroy();
    }
    delete blitter;
}

void QPlatformBackingStoreOpenGLSupport::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset, QPlatformTextureList *textures, bool translucentBackground)
{
    if (!qt_window_private(window)->receivedExpose)
        return;

    if (!context) {
        context.reset(new QOpenGLContext);
        context->setFormat(window->requestedFormat());
        context->setScreen(window->screen());
        context->setShareContext(qt_window_private(window)->shareContext());
        if (!context->create()) {
            qCWarning(lcQpaBackingStore, "composeAndFlush: QOpenGLContext creation failed");
            return;
        }
    }

    bool current = context->makeCurrent(window);

    if (!current && !context->isValid()) {
        // release resources and attempt to reinitialize upon context loss
        delete blitter;
        blitter = nullptr;
        textureId = 0;
        current = context->create() && context->makeCurrent(window);
    }

    if (!current) {
        qCWarning(lcQpaBackingStore, "composeAndFlush: makeCurrent() failed");
        return;
    }

    qCDebug(lcQpaBackingStore) << "Composing and flushing" << region << "of" << window
        << "at offset" << offset << "with" << textures->count() << "texture(s) in" << textures;

    QWindowPrivate::get(window)->lastComposeTime.start();

    QOpenGLFunctions *funcs = context->functions();
    funcs->glViewport(0, 0, qRound(window->width() * window->devicePixelRatio()), qRound(window->height() * window->devicePixelRatio()));
    funcs->glClearColor(0, 0, 0, translucentBackground ? 0 : 1);
    funcs->glClear(GL_COLOR_BUFFER_BIT);

    if (!blitter) {
        blitter = new QOpenGLTextureBlitter;
        blitter->create();
    }

    blitter->bind();

    const QRect deviceWindowRect = deviceRect(QRect(QPoint(), window->size()), window);
    const QPoint deviceWindowOffset = deviceOffset(offset, window);

    bool canUseSrgb = false;
    // If there are any sRGB textures in the list, check if the destination
    // framebuffer is sRGB capable.
    for (int i = 0; i < textures->count(); ++i) {
        if (textures->flags(i).testFlag(QPlatformTextureList::TextureIsSrgb)) {
            GLint cap = 0;
            funcs->glGetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE, &cap);
            if (cap)
                canUseSrgb = true;
            break;
        }
    }

    // Textures for renderToTexture widgets.
    for (int i = 0; i < textures->count(); ++i) {
        if (!textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop))
            blitTextureForWidget(textures, i, window, deviceWindowRect, blitter, offset, canUseSrgb);
    }

    // Backingstore texture with the normal widgets.
    GLuint textureId = 0;
    QOpenGLTextureBlitter::Origin origin = QOpenGLTextureBlitter::OriginTopLeft;
    if (QPlatformGraphicsBuffer *graphicsBuffer = backingStore->graphicsBuffer()) {
        if (graphicsBuffer->size() != textureSize) {
            if (this->textureId)
                funcs->glDeleteTextures(1, &this->textureId);
            funcs->glGenTextures(1, &this->textureId);
            funcs->glBindTexture(GL_TEXTURE_2D, this->textureId);
            QOpenGLContext *ctx = QOpenGLContext::currentContext();
            if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
                funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            }
            funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            if (QPlatformGraphicsBufferHelper::lockAndBindToTexture(graphicsBuffer, &needsSwizzle, &premultiplied)) {
                textureSize = graphicsBuffer->size();
            } else {
                textureSize = QSize(0,0);
            }

            graphicsBuffer->unlock();
        } else if (!region.isEmpty()){
            funcs->glBindTexture(GL_TEXTURE_2D, this->textureId);
            QPlatformGraphicsBufferHelper::lockAndBindToTexture(graphicsBuffer, &needsSwizzle, &premultiplied);
            graphicsBuffer->unlock();
        }

        if (graphicsBuffer->origin() == QPlatformGraphicsBuffer::OriginBottomLeft)
            origin = QOpenGLTextureBlitter::OriginBottomLeft;
        textureId = this->textureId;
    } else {
        QPlatformBackingStore::TextureFlags flags;
        textureId = backingStore->toTexture(deviceRegion(region, window, offset), &textureSize, &flags);
        needsSwizzle = (flags & QPlatformBackingStore::TextureSwizzle) != 0;
        premultiplied = (flags & QPlatformBackingStore::TexturePremultiplied) != 0;
        if (flags & QPlatformBackingStore::TextureFlip)
            origin = QOpenGLTextureBlitter::OriginBottomLeft;
    }

    funcs->glEnable(GL_BLEND);
    if (premultiplied)
        funcs->glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    else
        funcs->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    if (textureId) {
        if (needsSwizzle)
            blitter->setRedBlueSwizzle(true);
        // The backingstore is for the entire tlw.
        // In case of native children offset tells the position relative to the tlw.
        const QRect srcRect = toBottomLeftRect(deviceWindowRect.translated(deviceWindowOffset), textureSize.height());
        const QMatrix3x3 source = QOpenGLTextureBlitter::sourceTransform(srcRect,
                                                                         textureSize,
                                                                         origin);
        blitter->blit(textureId, QMatrix4x4(), source);
        if (needsSwizzle)
            blitter->setRedBlueSwizzle(false);
    }

    // Textures for renderToTexture widgets that have WA_AlwaysStackOnTop set.
    bool blendIsPremultiplied = premultiplied;
    for (int i = 0; i < textures->count(); ++i) {
        const QPlatformTextureList::Flags flags = textures->flags(i);
        if (flags.testFlag(QPlatformTextureList::NeedsPremultipliedAlphaBlending)) {
            if (!blendIsPremultiplied) {
                funcs->glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                blendIsPremultiplied = true;
            }
        } else {
            if (blendIsPremultiplied) {
                funcs->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
                blendIsPremultiplied = false;
            }
        }
        if (flags.testFlag(QPlatformTextureList::StacksOnTop))
            blitTextureForWidget(textures, i, window, deviceWindowRect, blitter, offset, canUseSrgb);
    }

    funcs->glDisable(GL_BLEND);
    blitter->release();

    context->swapBuffers(window);
}

GLuint QPlatformBackingStoreOpenGLSupport::toTexture(const QRegion &dirtyRegion, QSize *textureSize, QPlatformBackingStore::TextureFlags *flags) const
{
    Q_ASSERT(textureSize);
    Q_ASSERT(flags);

    QImage image = backingStore->toImage();
    QSize imageSize = image.size();

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    GLenum internalFormat = GL_RGBA;
    GLuint pixelType = GL_UNSIGNED_BYTE;

    bool needsConversion = false;
    *flags = { };
    switch (image.format()) {
    case QImage::Format_ARGB32_Premultiplied:
        *flags |= QPlatformBackingStore::TexturePremultiplied;
        Q_FALLTHROUGH();
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        *flags |= QPlatformBackingStore::TextureSwizzle;
        break;
    case QImage::Format_RGBA8888_Premultiplied:
        *flags |= QPlatformBackingStore::TexturePremultiplied;
        Q_FALLTHROUGH();
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
        break;
    case QImage::Format_BGR30:
    case QImage::Format_A2BGR30_Premultiplied:
        if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
            pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
            internalFormat = GL_RGB10_A2;
            *flags |= QPlatformBackingStore::TexturePremultiplied;
        } else {
            needsConversion = true;
        }
        break;
    case QImage::Format_RGB30:
    case QImage::Format_A2RGB30_Premultiplied:
        if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
            pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
            internalFormat = GL_RGB10_A2;
            *flags |= QPlatformBackingStore::TextureSwizzle | QPlatformBackingStore::TexturePremultiplied;
        } else {
            needsConversion = true;
        }
        break;
    default:
        needsConversion = true;
        break;
    }
    if (imageSize.isEmpty()) {
        *textureSize = imageSize;
        return 0;
    }

    // Must rely on the input only, not d_ptr.
    // With the default composeAndFlush() textureSize is &d_ptr->textureSize.
    bool resized = *textureSize != imageSize;
    if (dirtyRegion.isEmpty() && !resized)
        return textureId;

    *textureSize = imageSize;

    if (needsConversion)
        image = image.convertToFormat(QImage::Format_RGBA8888);

    // The image provided by the backingstore may have a stride larger than width * 4, for
    // instance on platforms that manually implement client-side decorations.
    static const int bytesPerPixel = 4;
    const qsizetype strideInPixels = image.bytesPerLine() / bytesPerPixel;
    const bool hasUnpackRowLength = !ctx->isOpenGLES() || ctx->format().majorVersion() >= 3;

    QOpenGLFunctions *funcs = ctx->functions();

    if (hasUnpackRowLength) {
        funcs->glPixelStorei(GL_UNPACK_ROW_LENGTH, strideInPixels);
    } else if (strideInPixels != image.width()) {
        // No UNPACK_ROW_LENGTH on ES 2.0 and yet we would need it. This case is typically
        // hit with QtWayland which is rarely used in combination with a ES2.0-only GL
        // implementation.  Therefore, accept the performance hit and do a copy.
        image = image.copy();
    }

    if (resized) {
        if (textureId)
            funcs->glDeleteTextures(1, &textureId);
        funcs->glGenTextures(1, &textureId);
        funcs->glBindTexture(GL_TEXTURE_2D, textureId);
        if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        }
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        funcs->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, imageSize.width(), imageSize.height(), 0, GL_RGBA, pixelType,
                            const_cast<uchar*>(image.constBits()));
    } else {
        funcs->glBindTexture(GL_TEXTURE_2D, textureId);
        QRect imageRect = image.rect();
        QRect rect = dirtyRegion.boundingRect() & imageRect;

        if (hasUnpackRowLength) {
            funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, pixelType,
                                   image.constScanLine(rect.y()) + rect.x() * bytesPerPixel);
        } else {
            // if the rect is wide enough it's cheaper to just
            // extend it instead of doing an image copy
            if (rect.width() >= imageRect.width() / 2) {
                rect.setX(0);
                rect.setWidth(imageRect.width());
            }

            // if the sub-rect is full-width we can pass the image data directly to
            // OpenGL instead of copying, since there's no gap between scanlines

            if (rect.width() == imageRect.width()) {
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, pixelType,
                                       image.constScanLine(rect.y()));
            } else {
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, pixelType,
                                       image.copy(rect).constBits());
            }
        }
    }

    if (hasUnpackRowLength)
        funcs->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    return textureId;
}

void qt_registerDefaultPlatformBackingStoreOpenGLSupport()
{
    if (!QPlatformBackingStoreOpenGLSupportBase::factoryFunction()) {
        QPlatformBackingStoreOpenGLSupportBase::setFactoryFunction([]() -> QPlatformBackingStoreOpenGLSupportBase* {
            return new QPlatformBackingStoreOpenGLSupport;
        });
    }
}

#endif // QT_NO_OPENGL

QT_END_NAMESPACE
