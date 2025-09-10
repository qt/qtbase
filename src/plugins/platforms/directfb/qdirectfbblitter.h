// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDIRECTFBBLITTER_H
#define QDIRECTFBBLITTER_H

#include "qdirectfbconvenience.h"

#include <private/qblittable_p.h>

#include <directfb.h>

QT_BEGIN_NAMESPACE

class QDirectFbBlitter : public QBlittable
{
public:
    QDirectFbBlitter(const QSize &size, IDirectFBSurface *surface);
    QDirectFbBlitter(const QSize &size, bool alpha);
    virtual ~QDirectFbBlitter();

    void fillRect(const QRectF &rect, const QColor &color) override;
    void drawPixmap(const QRectF &rect, const QPixmap &pixmap, const QRectF &subrect) override;
    void alphaFillRect(const QRectF &rect, const QColor &color, QPainter::CompositionMode cmode) override;
    void drawPixmapOpacity(const QRectF &rect,
                           const QPixmap &pixmap,
                           const QRectF &subrect,
                           QPainter::CompositionMode cmode,
                           qreal opacity) override;
    bool drawCachedGlyphs(const QPaintEngineState *state,
                          QFontEngine::GlyphFormat glyphFormat,
                          int numGlyphs,
                          const glyph_t *glyphs,
                          const QFixedPoint *positions,
                          QFontEngine *fontEngine) override;

    IDirectFBSurface *dfbSurface() const;

    static DFBSurfacePixelFormat alphaPixmapFormat();
    static DFBSurfacePixelFormat pixmapFormat();
    static DFBSurfacePixelFormat selectPixmapFormat(bool withAlpha);

protected:
    QImage *doLock() override;
    void doUnlock() override;

    QDirectFBPointer<IDirectFBSurface> m_surface;
    QImage m_image;

    friend class QDirectFbConvenience;

private:
    void drawDebugRect(const QRect &rect, const QColor &color);

    bool m_premult;
    bool m_debugPaint;
};

class QDirectFbBlitterPlatformPixmap : public QBlittablePlatformPixmap
{
public:
    QBlittable *createBlittable(const QSize &size, bool alpha) const override;

    QDirectFbBlitter *dfbBlitter() const;

    bool fromFile(const QString &filename, const char *format,
                  Qt::ImageConversionFlags flags) override;

private:
    bool fromDataBufferDescription(const DFBDataBufferDescription &);
};

inline QBlittable *QDirectFbBlitterPlatformPixmap::createBlittable(const QSize& size, bool alpha) const
{
    return new QDirectFbBlitter(size, alpha);
}

inline QDirectFbBlitter *QDirectFbBlitterPlatformPixmap::dfbBlitter() const
{
    return static_cast<QDirectFbBlitter*>(blittable());
}

inline IDirectFBSurface *QDirectFbBlitter::dfbSurface() const
{
    return m_surface.data();
}

class QDirectFbTextureGlyphCache : public QImageTextureGlyphCache
{
public:
    QDirectFbTextureGlyphCache(QFontEngine::GlyphFormat format, const QTransform &matrix)
        : QImageTextureGlyphCache(format, matrix)
    {}

    void resizeTextureData(int width, int height) override;

    IDirectFBSurface *sourceSurface();

private:
    QDirectFBPointer<IDirectFBSurface> m_surface;
};

QT_END_NAMESPACE

#endif // QDIRECTFBBLITTER_H
