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

#include "private/qpixmapfilter_p.h"
#include "private/qpixmapdata_gl_p.h"
#include "private/qpaintengineex_opengl2_p.h"
#include "private/qglengineshadermanager_p.h"
#include "private/qpixmapdata_p.h"
#include "private/qimagepixmapcleanuphooks_p.h"
#include "qglpixmapfilter_p.h"
#include "qgraphicssystem_gl_p.h"
#include "qpaintengine_opengl_p.h"
#include "qcache.h"

#include "qglframebufferobject.h"
#include "qglshaderprogram.h"
#include "qgl_p.h"

#include "private/qapplication_p.h"
#include "private/qdrawhelper_p.h"
#include "private/qmemrotate_p.h"
#include "private/qmath_p.h"
#include "qmath.h"

QT_BEGIN_NAMESPACE

// qpixmapfilter.cpp
Q_GUI_EXPORT void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);
Q_GUI_EXPORT QImage qt_halfScaled(const QImage &source);

void QGLPixmapFilterBase::bindTexture(const QPixmap &src) const
{
    const_cast<QGLContext *>(QGLContext::currentContext())->d_func()->bindTexture(src, GL_TEXTURE_2D, GL_RGBA, QGLContext::BindOptions(QGLContext::DefaultBindOption | QGLContext::MemoryManagedBindOption));
}

void QGLPixmapFilterBase::drawImpl(QPainter *painter, const QPointF &pos, const QPixmap &src, const QRectF& source) const
{
    processGL(painter, pos, src, source);
}

class QGLPixmapColorizeFilter: public QGLCustomShaderStage, public QGLPixmapFilter<QPixmapColorizeFilter>
{
public:
    QGLPixmapColorizeFilter();

    void setUniforms(QGLShaderProgram *program);

protected:
    bool processGL(QPainter *painter, const QPointF &pos, const QPixmap &pixmap, const QRectF &srcRect) const;
};

class QGLPixmapConvolutionFilter: public QGLCustomShaderStage, public QGLPixmapFilter<QPixmapConvolutionFilter>
{
public:
    QGLPixmapConvolutionFilter();
    ~QGLPixmapConvolutionFilter();

    void setUniforms(QGLShaderProgram *program);

protected:
    bool processGL(QPainter *painter, const QPointF &pos, const QPixmap &src, const QRectF &srcRect) const;

private:
    QByteArray generateConvolutionShader() const;

    mutable QSize m_srcSize;
    mutable int m_prevKernelSize;
};

class QGLPixmapBlurFilter : public QGLCustomShaderStage, public QGLPixmapFilter<QPixmapBlurFilter>
{
public:
    QGLPixmapBlurFilter();

protected:
    bool processGL(QPainter *painter, const QPointF &pos, const QPixmap &src, const QRectF &srcRect) const;
};

class QGLPixmapDropShadowFilter : public QGLCustomShaderStage, public QGLPixmapFilter<QPixmapDropShadowFilter>
{
public:
    QGLPixmapDropShadowFilter();

    void setUniforms(QGLShaderProgram *program);

protected:
    bool processGL(QPainter *painter, const QPointF &pos, const QPixmap &src, const QRectF &srcRect) const;
};

extern const QGLContext *qt_gl_share_context();

QPixmapFilter *QGL2PaintEngineEx::pixmapFilter(int type, const QPixmapFilter *prototype)
{
    Q_D(QGL2PaintEngineEx);
    switch (type) {
    case QPixmapFilter::ColorizeFilter:
        if (!d->colorizeFilter)
            d->colorizeFilter.reset(new QGLPixmapColorizeFilter);
        return d->colorizeFilter.data();

    case QPixmapFilter::BlurFilter: {
        if (!d->blurFilter)
            d->blurFilter.reset(new QGLPixmapBlurFilter());
        return d->blurFilter.data();
        }

    case QPixmapFilter::DropShadowFilter: {
        if (!d->dropShadowFilter)
            d->dropShadowFilter.reset(new QGLPixmapDropShadowFilter());
        return d->dropShadowFilter.data();
        }

    case QPixmapFilter::ConvolutionFilter:
        if (!d->convolutionFilter)
            d->convolutionFilter.reset(new QGLPixmapConvolutionFilter);
        return d->convolutionFilter.data();

    default: break;
    }
    return QPaintEngineEx::pixmapFilter(type, prototype);
}

static const char *qt_gl_colorize_filter =
        "uniform lowp vec4 colorizeColor;"
        "uniform lowp float colorizeStrength;"
        "lowp vec4 customShader(lowp sampler2D src, highp vec2 srcCoords)"
        "{"
        "        lowp vec4 srcPixel = texture2D(src, srcCoords);"
        "        lowp float gray = dot(srcPixel.rgb, vec3(0.212671, 0.715160, 0.072169));"
        "        lowp vec3 colorized = 1.0-((1.0-gray)*(1.0-colorizeColor.rgb));"
        "        return vec4(mix(srcPixel.rgb, colorized * srcPixel.a, colorizeStrength), srcPixel.a);"
        "}";

QGLPixmapColorizeFilter::QGLPixmapColorizeFilter()
{
    setSource(qt_gl_colorize_filter);
}

bool QGLPixmapColorizeFilter::processGL(QPainter *painter, const QPointF &pos, const QPixmap &src, const QRectF &) const
{
    QGLPixmapColorizeFilter *filter = const_cast<QGLPixmapColorizeFilter *>(this);

    filter->setOnPainter(painter);
    painter->drawPixmap(pos, src);
    filter->removeFromPainter(painter);

    return true;
}

void QGLPixmapColorizeFilter::setUniforms(QGLShaderProgram *program)
{
    program->setUniformValue("colorizeColor", color());
    program->setUniformValue("colorizeStrength", float(strength()));
}

void QGLPixmapConvolutionFilter::setUniforms(QGLShaderProgram *program)
{
    const qreal *kernel = convolutionKernel();
    int kernelWidth = columns();
    int kernelHeight = rows();
    int kernelSize = kernelWidth * kernelHeight;

    QVarLengthArray<GLfloat> matrix(kernelSize);
    QVarLengthArray<GLfloat> offset(kernelSize * 2);

    for(int i = 0; i < kernelSize; ++i)
        matrix[i] = kernel[i];

    for(int y = 0; y < kernelHeight; ++y) {
        for(int x = 0; x < kernelWidth; ++x) {
            offset[(y * kernelWidth + x) * 2] = x - (kernelWidth / 2);
            offset[(y * kernelWidth + x) * 2 + 1] = (kernelHeight / 2) - y;
        }
    }

    const qreal iw = 1.0 / m_srcSize.width();
    const qreal ih = 1.0 / m_srcSize.height();
    program->setUniformValue("inv_texture_size", iw, ih);
    program->setUniformValueArray("matrix", matrix.constData(), kernelSize, 1);
    program->setUniformValueArray("offset", offset.constData(), kernelSize, 2);
}

// generates convolution filter code for arbitrary sized kernel
QByteArray QGLPixmapConvolutionFilter::generateConvolutionShader() const {
    QByteArray code;
    int kernelWidth = columns();
    int kernelHeight = rows();
    int kernelSize = kernelWidth * kernelHeight;
    code.append("uniform highp vec2 inv_texture_size;\n"
                "uniform mediump float matrix[");
    code.append(QByteArray::number(kernelSize));
    code.append("];\n"
                "uniform highp vec2 offset[");
    code.append(QByteArray::number(kernelSize));
    code.append("];\n");
    code.append("lowp vec4 customShader(lowp sampler2D src, highp vec2 srcCoords) {\n");

    code.append("  int i = 0;\n"
                "  lowp vec4 sum = vec4(0.0);\n"
                "  for (i = 0; i < ");
    code.append(QByteArray::number(kernelSize));
    code.append("; i++) {\n"
                "    sum += matrix[i] * texture2D(src,srcCoords+inv_texture_size*offset[i]);\n"
                "  }\n"
                "  return sum;\n"
                "}");
    return code;
}

QGLPixmapConvolutionFilter::QGLPixmapConvolutionFilter()
    : m_prevKernelSize(-1)
{
}

QGLPixmapConvolutionFilter::~QGLPixmapConvolutionFilter()
{
}

bool QGLPixmapConvolutionFilter::processGL(QPainter *painter, const QPointF &pos, const QPixmap &src, const QRectF &srcRect) const
{
    QGLPixmapConvolutionFilter *filter = const_cast<QGLPixmapConvolutionFilter *>(this);

    m_srcSize = src.size();

    int kernelSize = rows() * columns();
    if (m_prevKernelSize == -1 || m_prevKernelSize != kernelSize) {
        filter->setSource(generateConvolutionShader());
        m_prevKernelSize = kernelSize;
    }

    filter->setOnPainter(painter);
    painter->drawPixmap(pos, src, srcRect);
    filter->removeFromPainter(painter);

    return true;
}

QGLPixmapBlurFilter::QGLPixmapBlurFilter()
{
}

class QGLBlurTextureInfo
{
public:
    QGLBlurTextureInfo(const QImage &image, GLuint tex, qreal r)
        : m_texture(tex)
        , m_radius(r)
    {
        m_paddedImage << image;
    }

    ~QGLBlurTextureInfo()
    {
        glDeleteTextures(1, &m_texture);
    }

    QImage paddedImage(int scaleLevel = 0) const;
    GLuint texture() const { return m_texture; }
    qreal radius() const { return m_radius; }

private:
    mutable QList<QImage> m_paddedImage;
    GLuint m_texture;
    qreal m_radius;
};

QImage QGLBlurTextureInfo::paddedImage(int scaleLevel) const
{
    for (int i = m_paddedImage.size() - 1; i <= scaleLevel; ++i)
        m_paddedImage << qt_halfScaled(m_paddedImage.at(i));

    return m_paddedImage.at(scaleLevel);
}

class QGLBlurTextureCache : public QObject
{
public:
    static QGLBlurTextureCache *cacheForContext(const QGLContext *context);

    QGLBlurTextureCache(const QGLContext *);
    ~QGLBlurTextureCache();

    QGLBlurTextureInfo *takeBlurTextureInfo(const QPixmap &pixmap);
    bool hasBlurTextureInfo(quint64 cacheKey) const;
    void insertBlurTextureInfo(const QPixmap &pixmap, QGLBlurTextureInfo *info);
    void clearBlurTextureInfo(quint64 cacheKey);

    void timerEvent(QTimerEvent *event);

private:
    static void pixmapDestroyed(QPixmapData *pixmap);

    QCache<quint64, QGLBlurTextureInfo > cache;

    static QList<QGLBlurTextureCache *> blurTextureCaches;

    int timerId;
};

QList<QGLBlurTextureCache *> QGLBlurTextureCache::blurTextureCaches;
Q_GLOBAL_STATIC(QGLContextGroupResource<QGLBlurTextureCache>, qt_blur_texture_caches)

QGLBlurTextureCache::QGLBlurTextureCache(const QGLContext *)
    : timerId(0)
{
    cache.setMaxCost(4 * 1024 * 1024);
    blurTextureCaches.append(this);
}

QGLBlurTextureCache::~QGLBlurTextureCache()
{
    blurTextureCaches.removeAt(blurTextureCaches.indexOf(this));
}

void QGLBlurTextureCache::timerEvent(QTimerEvent *)
{
    killTimer(timerId);
    timerId = 0;

    cache.clear();
}

QGLBlurTextureCache *QGLBlurTextureCache::cacheForContext(const QGLContext *context)
{
    return qt_blur_texture_caches()->value(context);
}

QGLBlurTextureInfo *QGLBlurTextureCache::takeBlurTextureInfo(const QPixmap &pixmap)
{
    return cache.take(pixmap.cacheKey());
}

void QGLBlurTextureCache::clearBlurTextureInfo(quint64 cacheKey)
{
    cache.remove(cacheKey);
}

bool QGLBlurTextureCache::hasBlurTextureInfo(quint64 cacheKey) const
{
    return cache.contains(cacheKey);
}

void QGLBlurTextureCache::insertBlurTextureInfo(const QPixmap &pixmap, QGLBlurTextureInfo *info)
{
    static bool hookAdded = false;
    if (!hookAdded) {
        QImagePixmapCleanupHooks::instance()->addPixmapDataDestructionHook(pixmapDestroyed);
        QImagePixmapCleanupHooks::instance()->addPixmapDataModificationHook(pixmapDestroyed);
        hookAdded = true;
    }

    QImagePixmapCleanupHooks::enableCleanupHooks(pixmap);
    cache.insert(pixmap.cacheKey(), info, pixmap.width() * pixmap.height());

    if (timerId)
        killTimer(timerId);

    timerId = startTimer(8000);
}

void QGLBlurTextureCache::pixmapDestroyed(QPixmapData *pmd)
{
    foreach (QGLBlurTextureCache *cache, blurTextureCaches) {
        if (cache->hasBlurTextureInfo(pmd->cacheKey()))
            cache->clearBlurTextureInfo(pmd->cacheKey());
    }
}

static const int qAnimatedBlurLevelIncrement = 16;
static const int qMaxBlurHalfScaleLevel = 1;

static GLuint generateBlurTexture(const QSize &size, GLenum format = GL_RGBA)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, size.width(), size.height(), 0, format,
                 GL_UNSIGNED_BYTE, 0);
    return texture;
}

static inline uint nextMultiple(uint x, uint multiplier)
{
    uint mod = x % multiplier;
    if (mod == 0)
        return x;
    return x + multiplier - mod;
}

Q_GUI_EXPORT void qt_memrotate90_gl(const quint32 *src, int srcWidth, int srcHeight, int srcStride,
                       quint32 *dest, int dstStride);

bool QGLPixmapBlurFilter::processGL(QPainter *painter, const QPointF &pos, const QPixmap &src, const QRectF &) const
{
    if (radius() < 1) {
        painter->drawPixmap(pos, src);
        return true;
    }

    qreal actualRadius = radius();

    QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());

    QGLBlurTextureCache *blurTextureCache = QGLBlurTextureCache::cacheForContext(ctx);
    QGLBlurTextureInfo *info = 0;
    int padding = nextMultiple(qCeil(actualRadius), qAnimatedBlurLevelIncrement);
    QRect targetRect = src.rect().adjusted(-padding, -padding, padding, padding);

    // pad so that we'll be able to half-scale qMaxBlurHalfScaleLevel times
    targetRect.setWidth((targetRect.width() + (qMaxBlurHalfScaleLevel-1)) & ~(qMaxBlurHalfScaleLevel-1));
    targetRect.setHeight((targetRect.height() + (qMaxBlurHalfScaleLevel-1)) & ~(qMaxBlurHalfScaleLevel-1));

    QSize textureSize;

    info = blurTextureCache->takeBlurTextureInfo(src);
    if (!info || info->radius() < actualRadius) {
        QSize paddedSize = targetRect.size() / 2;

        QImage padded(paddedSize.height(), paddedSize.width(), QImage::Format_ARGB32_Premultiplied);
        padded.fill(0);

        if (info) {
            int oldPadding = qRound(info->radius());

            QPainter p(&padded);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.drawImage((padding - oldPadding) / 2, (padding - oldPadding) / 2, info->paddedImage());
            p.end();
        } else {
            // TODO: combine byteswapping and memrotating into one by declaring
            // custom GL_RGBA pixel type and qt_colorConvert template for it
            QImage prepadded = qt_halfScaled(src.toImage()).convertToFormat(QImage::Format_ARGB32_Premultiplied);

            // byte-swap and memrotates in one go
            qt_memrotate90_gl(reinterpret_cast<const quint32*>(prepadded.bits()),
                              prepadded.width(), prepadded.height(), prepadded.bytesPerLine(),
                              reinterpret_cast<quint32*>(padded.scanLine(padding / 2)) + padding / 2,
                              padded.bytesPerLine());
        }

        delete info;
        info = new QGLBlurTextureInfo(padded, generateBlurTexture(paddedSize), padding);

        textureSize = paddedSize;
    } else {
        textureSize = QSize(info->paddedImage().height(), info->paddedImage().width());
    }

    actualRadius *= qreal(0.5);
    int level = 1;
    for (; level < qMaxBlurHalfScaleLevel; ++level) {
        if (actualRadius <= 16)
            break;
        actualRadius *= qreal(0.5);
    }

    const int s = (1 << level);

    int prepadding = qRound(info->radius());
    padding = qMin(prepadding, qCeil(actualRadius) << level);
    targetRect = src.rect().adjusted(-padding, -padding, padding, padding);

    targetRect.setWidth(targetRect.width() & ~(s-1));
    targetRect.setHeight(targetRect.height() & ~(s-1));

    int paddingDelta = (prepadding - padding) >> level;

    QRect subRect(paddingDelta, paddingDelta, targetRect.width() >> level, targetRect.height() >> level);
    QImage sourceImage = info->paddedImage(level-1);

    QImage subImage(subRect.height(), subRect.width(), QImage::Format_ARGB32_Premultiplied);
    qt_rectcopy((QRgb *)subImage.bits(), ((QRgb *)sourceImage.scanLine(paddingDelta)) + paddingDelta,
                0, 0, subRect.height(), subRect.width(), subImage.bytesPerLine(), sourceImage.bytesPerLine());

    GLuint texture = info->texture();

    qt_blurImage(subImage, actualRadius, blurHints() & QGraphicsBlurEffect::QualityHint, 1);

    // subtract one pixel off the end to prevent the bilinear sampling from sampling uninitialized data
    QRect textureSubRect = subImage.rect().adjusted(0, 0, -1, -1);
    QRectF targetRectF = QRectF(targetRect).adjusted(0, 0, -targetRect.width() / qreal(textureSize.width()), -targetRect.height() / qreal(textureSize.height()));

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, subImage.width(), subImage.height(), GL_RGBA,
            GL_UNSIGNED_BYTE, const_cast<const QImage &>(subImage).bits());

    QGL2PaintEngineEx *engine = static_cast<QGL2PaintEngineEx *>(painter->paintEngine());
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    // texture is flipped on the y-axis
    targetRectF = QRectF(targetRectF.x(), targetRectF.bottom(), targetRectF.width(), -targetRectF.height());
    engine->drawTexture(targetRectF.translated(pos), texture, textureSize, textureSubRect);

    blurTextureCache->insertBlurTextureInfo(src, info);

    return true;
}

static const char *qt_gl_drop_shadow_filter =
        "uniform lowp vec4 shadowColor;"
        "lowp vec4 customShader(lowp sampler2D src, highp vec2 srcCoords)"
        "{"
        "    return shadowColor * texture2D(src, srcCoords.yx).a;"
        "}";


QGLPixmapDropShadowFilter::QGLPixmapDropShadowFilter()
{
    setSource(qt_gl_drop_shadow_filter);
}

bool QGLPixmapDropShadowFilter::processGL(QPainter *painter, const QPointF &pos, const QPixmap &src, const QRectF &srcRect) const
{
    QGLPixmapDropShadowFilter *filter = const_cast<QGLPixmapDropShadowFilter *>(this);

    qreal r = blurRadius();
    QRectF targetRectUnaligned = QRectF(src.rect()).translated(pos + offset()).adjusted(-r, -r, r, r);
    QRect targetRect = targetRectUnaligned.toAlignedRect();

    // ensure even dimensions (going to divide by two)
    targetRect.setWidth((targetRect.width() + 1) & ~1);
    targetRect.setHeight((targetRect.height() + 1) & ~1);

    QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());
    QGLBlurTextureCache *blurTextureCache = QGLBlurTextureCache::cacheForContext(ctx);

    QGLBlurTextureInfo *info = blurTextureCache->takeBlurTextureInfo(src);
    if (!info || info->radius() != r) {
        QImage half = qt_halfScaled(src.toImage().alphaChannel());

        qreal rx = r + targetRect.left() - targetRectUnaligned.left();
        qreal ry = r + targetRect.top() - targetRectUnaligned.top();

        QImage image = QImage(targetRect.size() / 2, QImage::Format_Indexed8);
        image.setColorTable(half.colorTable());
        image.fill(0);
        int dx = qRound(rx * qreal(0.5));
        int dy = qRound(ry * qreal(0.5));
        qt_rectcopy(image.bits(), half.bits(), dx, dy,
                    half.width(), half.height(),
                    image.bytesPerLine(), half.bytesPerLine());

        qt_blurImage(image, r * qreal(0.5), false, 1);

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, image.width(), image.height(),
                     0, GL_ALPHA, GL_UNSIGNED_BYTE, image.bits());

        info = new QGLBlurTextureInfo(image, texture, r);
    }

    GLuint texture = info->texture();

    filter->setOnPainter(painter);

    QGL2PaintEngineEx *engine = static_cast<QGL2PaintEngineEx *>(painter->paintEngine());
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    engine->drawTexture(targetRect, texture, info->paddedImage().size(), info->paddedImage().rect());

    filter->removeFromPainter(painter);

    // Now draw the actual pixmap over the top.
    painter->drawPixmap(pos, src, srcRect);

    blurTextureCache->insertBlurTextureInfo(src, info);

    return true;
}

void QGLPixmapDropShadowFilter::setUniforms(QGLShaderProgram *program)
{
    QColor col = color();
    qreal alpha = col.alphaF();
    program->setUniformValue("shadowColor", col.redF() * alpha,
                                            col.greenF() * alpha,
                                            col.blueF() * alpha,
                                            alpha);
}

QT_END_NAMESPACE
