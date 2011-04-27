/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qdirectfbpaintengine.h"

#ifndef QT_NO_QWS_DIRECTFB

#include "qdirectfbwindowsurface.h"
#include "qdirectfbscreen.h"
#include "qdirectfbpixmap.h"
#include <directfb.h>
#include <qtransform.h>
#include <qvarlengtharray.h>
#include <qcache.h>
#include <qmath.h>
#include <private/qpixmapdata_p.h>
#include <private/qpixmap_raster_p.h>
#include <private/qimagepixmapcleanuphooks_p.h>


QT_BEGIN_NAMESPACE

class SurfaceCache;
class QDirectFBPaintEnginePrivate : public QRasterPaintEnginePrivate
{
public:
    enum TransformationTypeFlags {
        Matrix_NegativeScale = 0x100,
        Matrix_RectsUnsupported = (QTransform::TxRotate|QTransform::TxShear|QTransform::TxProject),
        Matrix_BlitsUnsupported = (Matrix_NegativeScale|Matrix_RectsUnsupported)
    };

    inline static uint getTransformationType(const QTransform &transform)
    {
        int ret = transform.type();
        if (qMin(transform.m11(), transform.m22()) < 0) {
            ret |= QDirectFBPaintEnginePrivate::Matrix_NegativeScale;
        }
        return ret;
    }

    enum CompositionModeStatus {
        PorterDuff_None = 0x0,
        PorterDuff_Supported = 0x1,
        PorterDuff_PremultiplyColors = 0x2,
        PorterDuff_AlwaysBlend = 0x4
    };

    enum ClipType {
        ClipUnset,
        NoClip,
        RectClip,
        RegionClip,
        ComplexClip
    };

    QDirectFBPaintEnginePrivate(QDirectFBPaintEngine *p);
    ~QDirectFBPaintEnginePrivate();

    inline void setTransform(const QTransform &transforma);
    inline void setPen(const QPen &pen);
    inline void setCompositionMode(QPainter::CompositionMode mode);
    inline void setRenderHints(QPainter::RenderHints hints);

    inline void setDFBColor(const QColor &color);

    inline void lock();
    inline void unlock();
    static inline void unlock(QDirectFBPaintDevice *device);

    inline bool isSimpleBrush(const QBrush &brush) const;

    void drawTiledPixmap(const QRectF &dest, const QPixmap &pixmap, const QPointF &pos, const QTransform &pixmapTransform);
    void blit(const QRectF &dest, IDirectFBSurface *surface, const QRectF &src);

    inline bool supportsStretchBlit() const;

    inline void updateClip();
    virtual void systemStateChanged();

    static IDirectFBSurface *getSurface(const QImage &img, bool *release);

#ifdef QT_DIRECTFB_IMAGECACHE
    static inline int cacheCost(const QImage &img) { return img.width() * img.height() * img.depth() / 8; }
#endif

    enum BlitFlag {
        HasAlpha = 0x1,
        Premultiplied = 0x2
    };
    void prepareForBlit(uint blitFlags);

    IDirectFBSurface *surface;

    bool antialiased;
    bool simplePen;

    uint transformationType; // this is QTransform::type() + Matrix_NegativeScale if qMin(transform.m11(), transform.m22()) < 0

    SurfaceCache *surfaceCache;
    IDirectFB *fb;
    quint8 opacity;

    ClipType clipType;
    QDirectFBPaintDevice *dfbDevice;
    uint compositionModeStatus;
    bool isPremultiplied;

    bool inClip;
    QRect currentClip;

    QDirectFBPaintEngine *q;
};

class SurfaceCache
{
public:
    SurfaceCache() : surface(0), buffer(0), bufsize(0) {}
    ~SurfaceCache() { clear(); }
    IDirectFBSurface *getSurface(const uint *buf, int size);
    void clear();
private:
    IDirectFBSurface *surface;
    uint *buffer;
    int bufsize;
};


#ifdef QT_DIRECTFB_IMAGECACHE
QT_BEGIN_INCLUDE_NAMESPACE
#include <private/qimage_p.h>
QT_END_INCLUDE_NAMESPACE
struct CachedImage
{
    IDirectFBSurface *surface;
    ~CachedImage()
    {
        if (surface && QDirectFBScreen::instance()) {
            QDirectFBScreen::instance()->releaseDFBSurface(surface);
        }
    }
};
static QCache<qint64, CachedImage> imageCache(4*1024*1024); // 4 MB
#endif

#define VOID_ARG() static_cast<bool>(false)
enum PaintOperation {
    DRAW_RECTS = 0x0001, DRAW_LINES = 0x0002, DRAW_IMAGE = 0x0004,
    DRAW_PIXMAP = 0x0008, DRAW_TILED_PIXMAP = 0x0010, STROKE_PATH = 0x0020,
    DRAW_PATH = 0x0040, DRAW_POINTS = 0x0080, DRAW_ELLIPSE = 0x0100,
    DRAW_POLYGON = 0x0200, DRAW_TEXT = 0x0400, FILL_PATH = 0x0800,
    FILL_RECT = 0x1000, DRAW_COLORSPANS = 0x2000, DRAW_ROUNDED_RECT = 0x4000,
    DRAW_STATICTEXT = 0x8000, ALL = 0xffff
};

enum { RasterWarn = 1, RasterDisable = 2 };
static inline uint rasterFallbacksMask(PaintOperation op)
{
    uint ret = 0;
#ifdef QT_DIRECTFB_WARN_ON_RASTERFALLBACKS
    if (op & QT_DIRECTFB_WARN_ON_RASTERFALLBACKS)
        ret |= RasterWarn;
#endif
#ifdef QT_DIRECTFB_DISABLE_RASTERFALLBACKS
    if (op & QT_DIRECTFB_DISABLE_RASTERFALLBACKS)
        ret |= RasterDisable;
#endif
    static int warningMask = -1;
    static int disableMask = -1;
    if (warningMask < 0) {
        struct {
            const char *name;
            PaintOperation operation;
        } const operations[] = {
            { "DRAW_RECTS", DRAW_RECTS },
            { "DRAW_LINES", DRAW_LINES },
            { "DRAW_IMAGE", DRAW_IMAGE },
            { "DRAW_PIXMAP", DRAW_PIXMAP },
            { "DRAW_TILED_PIXMAP", DRAW_TILED_PIXMAP },
            { "STROKE_PATH", STROKE_PATH },
            { "DRAW_PATH", DRAW_PATH },
            { "DRAW_POINTS", DRAW_POINTS },
            { "DRAW_ELLIPSE", DRAW_ELLIPSE },
            { "DRAW_POLYGON", DRAW_POLYGON },
            { "DRAW_TEXT", DRAW_TEXT },
            { "FILL_PATH", FILL_PATH },
            { "FILL_RECT", FILL_RECT },
            { "DRAW_COLORSPANS", DRAW_COLORSPANS },
            { "DRAW_ROUNDED_RECT", DRAW_ROUNDED_RECT },
            { "ALL", ALL },
            { 0, ALL }
        };

        QStringList warning = QString::fromLatin1(qgetenv("QT_DIRECTFB_WARN_ON_RASTERFALLBACKS")).toUpper().split(QLatin1Char('|'),
                                                                                                                  QString::SkipEmptyParts);
        QStringList disable = QString::fromLatin1(qgetenv("QT_DIRECTFB_DISABLE_RASTERFALLBACKS")).toUpper().split(QLatin1Char('|'),
                                                                                                                  QString::SkipEmptyParts);
        warningMask = 0;
        disableMask = 0;
        if (!warning.isEmpty() || !disable.isEmpty()) {
            for (int i=0; operations[i].name; ++i) {
                const QString name = QString::fromLatin1(operations[i].name);
                int idx = warning.indexOf(name);
                if (idx != -1) {
                    warningMask |= operations[i].operation;
                    warning.erase(warning.begin() + idx);
                }
                idx = disable.indexOf(name);
                if (idx != -1) {
                    disableMask |= operations[i].operation;
                    disable.erase(disable.begin() + idx);
                }
            }
        }
        if (!warning.isEmpty()) {
            qWarning("QDirectFBPaintEngine QT_DIRECTFB_WARN_ON_RASTERFALLBACKS Unknown operation(s): %s",
                     qPrintable(warning.join(QLatin1String("|"))));
        }
        if (!disable.isEmpty()) {
            qWarning("QDirectFBPaintEngine QT_DIRECTFB_DISABLE_RASTERFALLBACKS Unknown operation(s): %s",
                     qPrintable(disable.join(QLatin1String("|"))));
        }
    }
    if (op & warningMask)
        ret |= RasterWarn;
    if (op & disableMask)
        ret |= RasterDisable;
    return ret;
}

template <typename device, typename T1, typename T2, typename T3>
static void rasterFallbackWarn(const char *msg, const char *func, const device *dev,
                               uint transformationType, bool simplePen,
                               uint clipType, uint compositionModeStatus,
                               const char *nameOne, const T1 &one,
                               const char *nameTwo, const T2 &two,
                               const char *nameThree, const T3 &three);

#define RASTERFALLBACK(op, one, two, three)                             \
    {                                                                   \
        static const uint rasterFallbacks = rasterFallbacksMask(op);    \
        switch (rasterFallbacks) {                                      \
        case 0: break;                                                  \
        case RasterWarn:                                                \
            rasterFallbackWarn("Falling back to raster engine for",     \
                               __FUNCTION__,                            \
                               state()->painter->device(),              \
                               d_func()->transformationType,            \
                               d_func()->simplePen,                     \
                               d_func()->clipType,                      \
                               d_func()->compositionModeStatus,         \
                               #one, one, #two, two, #three, three);    \
            break;                                                      \
        case RasterDisable|RasterWarn:                                  \
            rasterFallbackWarn("Disabled raster engine operation",      \
                               __FUNCTION__,                            \
                               state()->painter->device(),              \
                               d_func()->transformationType,            \
                               d_func()->simplePen,                     \
                               d_func()->clipType,                      \
                               d_func()->compositionModeStatus,         \
                               #one, one, #two, two, #three, three);    \
        case RasterDisable:                                             \
            return;                                                     \
        }                                                               \
    }

template <class T>
static inline void drawLines(const T *lines, int n, const QTransform &transform, IDirectFBSurface *surface);
template <class T>
static inline void fillRects(const T *rects, int n, const QTransform &transform, IDirectFBSurface *surface);
template <class T>
static inline void drawRects(const T *rects, int n, const QTransform &transform, IDirectFBSurface *surface);

#define CLIPPED_PAINT(operation) {                                      \
        d->unlock();                                                    \
        DFBRegion clipRegion;                                           \
        switch (d->clipType) {                                          \
        case QDirectFBPaintEnginePrivate::NoClip:                       \
        case QDirectFBPaintEnginePrivate::RectClip:                     \
            operation;                                                  \
            break;                                                      \
        case QDirectFBPaintEnginePrivate::RegionClip: {                 \
            Q_ASSERT(d->clip());                                        \
            const QVector<QRect> cr = d->clip()->clipRegion.rects();    \
            const int size = cr.size();                                 \
            for (int i=0; i<size; ++i) {                                \
                d->currentClip = cr.at(i);                              \
                clipRegion.x1 = d->currentClip.x();                     \
                clipRegion.y1 = d->currentClip.y();                     \
                clipRegion.x2 = d->currentClip.right();                 \
                clipRegion.y2 = d->currentClip.bottom();                \
                d->surface->SetClip(d->surface, &clipRegion);           \
                operation;                                              \
            }                                                           \
            d->updateClip();                                            \
            break; }                                                    \
        case QDirectFBPaintEnginePrivate::ComplexClip:                  \
        case QDirectFBPaintEnginePrivate::ClipUnset:                    \
            qFatal("CLIPPED_PAINT internal error %d", d->clipType);     \
            break;                                                      \
        }                                                               \
    }


QDirectFBPaintEngine::QDirectFBPaintEngine(QPaintDevice *device)
    : QRasterPaintEngine(*(new QDirectFBPaintEnginePrivate(this)), device)
{
}

QDirectFBPaintEngine::~QDirectFBPaintEngine()
{
}

bool QDirectFBPaintEngine::begin(QPaintDevice *device)
{
    Q_D(QDirectFBPaintEngine);
    if (device->devType() == QInternal::CustomRaster) {
        d->dfbDevice = static_cast<QDirectFBPaintDevice*>(device);
    } else if (device->devType() == QInternal::Pixmap) {
        QPixmapData *data = static_cast<QPixmap*>(device)->pixmapData();
        Q_ASSERT(data->classId() == QPixmapData::DirectFBClass);
        QDirectFBPixmapData *dfbPixmapData = static_cast<QDirectFBPixmapData*>(data);
        QDirectFBPaintEnginePrivate::unlock(dfbPixmapData);
        d->dfbDevice = static_cast<QDirectFBPaintDevice*>(dfbPixmapData);
    }

    if (d->dfbDevice)
        d->surface = d->dfbDevice->directFBSurface();

    if (!d->surface) {
        qFatal("QDirectFBPaintEngine used on an invalid device: 0x%x",
               device->devType());
    }
    d->isPremultiplied = QDirectFBScreen::isPremultiplied(d->dfbDevice->format());

    d->prepare(d->dfbDevice);
    gccaps = AllFeatures;
    d->setCompositionMode(state()->composition_mode);

    return QRasterPaintEngine::begin(device);
}

bool QDirectFBPaintEngine::end()
{
    Q_D(QDirectFBPaintEngine);
    d->unlock();
    d->dfbDevice = 0;
#if (Q_DIRECTFB_VERSION >= 0x010000)
    d->surface->ReleaseSource(d->surface);
#endif
    d->currentClip = QRect();
    d->surface->SetClip(d->surface, NULL);
    d->surface = 0;
    return QRasterPaintEngine::end();
}

void QDirectFBPaintEngine::clipEnabledChanged()
{
    Q_D(QDirectFBPaintEngine);
    QRasterPaintEngine::clipEnabledChanged();
    d->updateClip();
}

void QDirectFBPaintEngine::penChanged()
{
    Q_D(QDirectFBPaintEngine);
    d->setPen(state()->pen);

    QRasterPaintEngine::penChanged();
}

void QDirectFBPaintEngine::opacityChanged()
{
    Q_D(QDirectFBPaintEngine);
    d->opacity = quint8(state()->opacity * 255);
    QRasterPaintEngine::opacityChanged();
}

void QDirectFBPaintEngine::compositionModeChanged()
{
    Q_D(QDirectFBPaintEngine);
    d->setCompositionMode(state()->compositionMode());
    QRasterPaintEngine::compositionModeChanged();
}

void QDirectFBPaintEngine::renderHintsChanged()
{
    Q_D(QDirectFBPaintEngine);
    d->setRenderHints(state()->renderHints);
    QRasterPaintEngine::renderHintsChanged();
}

void QDirectFBPaintEngine::transformChanged()
{
    Q_D(QDirectFBPaintEngine);
    d->setTransform(state()->matrix);
    QRasterPaintEngine::transformChanged();
}

void QDirectFBPaintEngine::setState(QPainterState *state)
{
    Q_D(QDirectFBPaintEngine);
    QRasterPaintEngine::setState(state);
    d->setPen(state->pen);
    d->opacity = quint8(state->opacity * 255);
    d->setCompositionMode(state->compositionMode());
    d->setTransform(state->transform());
    d->setRenderHints(state->renderHints);
    if (d->surface)
        d->updateClip();
}

void QDirectFBPaintEngine::clip(const QVectorPath &path, Qt::ClipOperation op)
{
    Q_D(QDirectFBPaintEngine);
    const bool wasInClip = d->inClip;
    d->inClip = true;
    QRasterPaintEngine::clip(path, op);
    if (!wasInClip) {
        d->inClip = false;
        d->updateClip();
    }
}

void QDirectFBPaintEngine::clip(const QRegion &region, Qt::ClipOperation op)
{
    Q_D(QDirectFBPaintEngine);
    const bool wasInClip = d->inClip;
    d->inClip = true;
    QRasterPaintEngine::clip(region, op);
    if (!wasInClip) {
        d->inClip = false;
        d->updateClip();
    }
}

void QDirectFBPaintEngine::clip(const QRect &rect, Qt::ClipOperation op)
{
    Q_D(QDirectFBPaintEngine);
    const bool wasInClip = d->inClip;
    d->inClip = true;
    QRasterPaintEngine::clip(rect, op);
    if (!wasInClip) {
        d->inClip = false;
        d->updateClip();
    }
}

void QDirectFBPaintEngine::drawRects(const QRect *rects, int rectCount)
{
    Q_D(QDirectFBPaintEngine);
    const QPen &pen = state()->pen;
    const QBrush &brush = state()->brush;
    if (brush.style() == Qt::NoBrush && pen.style() == Qt::NoPen)
        return;

    if ((d->transformationType & QDirectFBPaintEnginePrivate::Matrix_RectsUnsupported)
        || !d->simplePen
        || d->clipType == QDirectFBPaintEnginePrivate::ComplexClip
        || !d->isSimpleBrush(brush)
        || !(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(DRAW_RECTS, rectCount, VOID_ARG(), VOID_ARG());
        d->lock();
        QRasterPaintEngine::drawRects(rects, rectCount);
        return;
    }

    if (brush.style() != Qt::NoBrush) {
        d->setDFBColor(brush.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(fillRects<QRect>)(rects, rectCount, state()->matrix, d->surface));
    }

    if (pen.style() != Qt::NoPen) {
        d->setDFBColor(pen.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(drawRects<QRect>)(rects, rectCount, state()->matrix, d->surface));
    }
}

void QDirectFBPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    Q_D(QDirectFBPaintEngine);
    const QPen &pen = state()->pen;
    const QBrush &brush = state()->brush;
    if (brush.style() == Qt::NoBrush && pen.style() == Qt::NoPen)
        return;

    if ((d->transformationType & QDirectFBPaintEnginePrivate::Matrix_RectsUnsupported)
        || !d->simplePen
        || d->clipType == QDirectFBPaintEnginePrivate::ComplexClip
        || !d->isSimpleBrush(brush)
        || !(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(DRAW_RECTS, rectCount, VOID_ARG(), VOID_ARG());
        d->lock();
        QRasterPaintEngine::drawRects(rects, rectCount);
        return;
    }

    if (brush.style() != Qt::NoBrush) {
        d->setDFBColor(brush.color());
        CLIPPED_PAINT(fillRects<QRectF>(rects, rectCount, state()->matrix, d->surface));
    }

    if (pen.style() != Qt::NoPen) {
        d->setDFBColor(pen.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(drawRects<QRectF>)(rects, rectCount, state()->matrix, d->surface));
    }
}

void QDirectFBPaintEngine::drawLines(const QLine *lines, int lineCount)
{
    Q_D(QDirectFBPaintEngine);

    const QPen &pen = state()->pen;
    if (!d->simplePen
        || d->clipType == QDirectFBPaintEnginePrivate::ComplexClip
        || !(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(DRAW_LINES, lineCount, VOID_ARG(), VOID_ARG());
        d->lock();
        QRasterPaintEngine::drawLines(lines, lineCount);
        return;
    }

    if (pen.style() != Qt::NoPen) {
        d->setDFBColor(pen.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(drawLines<QLine>)(lines, lineCount, state()->matrix, d->surface));
    }
}

void QDirectFBPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
    Q_D(QDirectFBPaintEngine);

    const QPen &pen = state()->pen;
    if (!d->simplePen
        || d->clipType == QDirectFBPaintEnginePrivate::ComplexClip
        || !(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(DRAW_LINES, lineCount, VOID_ARG(), VOID_ARG());
        d->lock();
        QRasterPaintEngine::drawLines(lines, lineCount);
        return;
    }

    if (pen.style() != Qt::NoPen) {
        d->setDFBColor(pen.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(drawLines<QLineF>)(lines, lineCount, state()->matrix, d->surface));
    }
}

void QDirectFBPaintEngine::drawImage(const QRectF &r, const QImage &image,
                                     const QRectF &sr,
                                     Qt::ImageConversionFlags flags)
{
    Q_D(QDirectFBPaintEngine);
    Q_UNUSED(flags);

    /*  This is hard to read. The way it works is like this:

    - If you do not have support for preallocated surfaces and do not use an
    image cache we always fall back to raster engine.

    - If it's rotated/sheared/mirrored (negative scale) or we can't
    clip it we fall back to raster engine.

    - If we don't cache the image, but we do have support for
    preallocated surfaces we fall back to the raster engine if the
    image is in a format DirectFB can't handle.

    - If we do cache the image but don't have support for preallocated
    images and the cost of caching the image (bytes used) is higher
    than the max image cache size we fall back to raster engine.
    */

#if !defined QT_NO_DIRECTFB_PREALLOCATED || defined QT_DIRECTFB_IMAGECACHE
    if (!(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)
        || (d->transformationType & QDirectFBPaintEnginePrivate::Matrix_BlitsUnsupported)
        || (d->clipType == QDirectFBPaintEnginePrivate::ComplexClip)
        || (!d->supportsStretchBlit() && state()->matrix.mapRect(r).size() != sr.size())
#ifndef QT_DIRECTFB_IMAGECACHE
        || (QDirectFBScreen::getSurfacePixelFormat(image.format()) == DSPF_UNKNOWN)
#elif defined QT_NO_DIRECTFB_PREALLOCATED
        || (QDirectFBPaintEnginePrivate::cacheCost(image) > imageCache.maxCost())
#endif
        )
#endif
    {
        RASTERFALLBACK(DRAW_IMAGE, r, image.size(), sr);
        d->lock();
        QRasterPaintEngine::drawImage(r, image, sr, flags);
        return;
    }
#if !defined QT_NO_DIRECTFB_PREALLOCATED || defined QT_DIRECTFB_IMAGECACHE
    bool release;
    IDirectFBSurface *imgSurface = d->getSurface(image, &release);
    uint blitFlags = 0;
    if (image.hasAlphaChannel())
        blitFlags |= QDirectFBPaintEnginePrivate::HasAlpha;
    if (QDirectFBScreen::isPremultiplied(image.format()))
        blitFlags |= QDirectFBPaintEnginePrivate::Premultiplied;
    d->prepareForBlit(blitFlags);
    CLIPPED_PAINT(d->blit(r, imgSurface, sr));
    if (release) {
#if (Q_DIRECTFB_VERSION >= 0x010000)
        d->surface->ReleaseSource(d->surface);
#endif
        imgSurface->Release(imgSurface);
    }
#endif
}

void QDirectFBPaintEngine::drawImage(const QPointF &p, const QImage &img)
{
    drawImage(QRectF(p, img.size()), img, img.rect());
}

void QDirectFBPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pixmap,
                                      const QRectF &sr)
{
    Q_D(QDirectFBPaintEngine);

    if (pixmap.pixmapData()->classId() != QPixmapData::DirectFBClass) {
        RASTERFALLBACK(DRAW_PIXMAP, r, pixmap.size(), sr);
        d->lock();
        QRasterPaintEngine::drawPixmap(r, pixmap, sr);
    } else {
        QPixmapData *data = pixmap.pixmapData();
        Q_ASSERT(data->classId() == QPixmapData::DirectFBClass);
        QDirectFBPixmapData *dfbData = static_cast<QDirectFBPixmapData*>(data);
        if (!(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)
            || (d->transformationType & QDirectFBPaintEnginePrivate::Matrix_BlitsUnsupported)
            || (d->clipType == QDirectFBPaintEnginePrivate::ComplexClip)
            || (!d->supportsStretchBlit() && state()->matrix.mapRect(r).size() != sr.size())) {
            RASTERFALLBACK(DRAW_PIXMAP, r, pixmap.size(), sr);
            const QImage *img = dfbData->buffer();
            d->lock();
            QRasterPaintEngine::drawImage(r, *img, sr);
        } else {
            QDirectFBPaintEnginePrivate::unlock(dfbData);
            IDirectFBSurface *s = dfbData->directFBSurface();
            uint blitFlags = 0;
            if (pixmap.hasAlphaChannel())
                blitFlags |= QDirectFBPaintEnginePrivate::HasAlpha;
            if (QDirectFBScreen::isPremultiplied(dfbData->pixelFormat()))
                blitFlags |= QDirectFBPaintEnginePrivate::Premultiplied;

            d->prepareForBlit(blitFlags);
            CLIPPED_PAINT(d->blit(r, s, sr));
        }
    }
}

void QDirectFBPaintEngine::drawPixmap(const QPointF &p, const QPixmap &pm)
{
    drawPixmap(QRectF(p, pm.size()), pm, pm.rect());
}

void QDirectFBPaintEngine::drawTiledPixmap(const QRectF &r,
                                           const QPixmap &pixmap,
                                           const QPointF &offset)
{
    Q_D(QDirectFBPaintEngine);
    if (pixmap.pixmapData()->classId() != QPixmapData::DirectFBClass) {
        RASTERFALLBACK(DRAW_TILED_PIXMAP, r, pixmap.size(), offset);
        d->lock();
        QRasterPaintEngine::drawTiledPixmap(r, pixmap, offset);
    } else if (!(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)
               || (d->transformationType & QDirectFBPaintEnginePrivate::Matrix_BlitsUnsupported)
               || (d->clipType == QDirectFBPaintEnginePrivate::ComplexClip)
               || (!d->supportsStretchBlit() && state()->matrix.isScaling())) {
        RASTERFALLBACK(DRAW_TILED_PIXMAP, r, pixmap.size(), offset);
        QPixmapData *pixmapData = pixmap.pixmapData();
        Q_ASSERT(pixmapData->classId() == QPixmapData::DirectFBClass);
        QDirectFBPixmapData *dfbData = static_cast<QDirectFBPixmapData*>(pixmapData);
        const QImage *img = dfbData->buffer();
        d->lock();
        QRasterPixmapData *data = new QRasterPixmapData(QPixmapData::PixmapType);
        data->fromImage(*img, Qt::AutoColor);
        const QPixmap pix(data);
        QRasterPaintEngine::drawTiledPixmap(r, pix, offset);
    } else {
        QTransform transform(state()->matrix);
        CLIPPED_PAINT(d->drawTiledPixmap(r, pixmap, offset, transform));
    }
}


void QDirectFBPaintEngine::stroke(const QVectorPath &path, const QPen &pen)
{
    RASTERFALLBACK(STROKE_PATH, path, VOID_ARG(), VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::stroke(path, pen);
}

void QDirectFBPaintEngine::drawPath(const QPainterPath &path)
{
    RASTERFALLBACK(DRAW_PATH, path, VOID_ARG(), VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPath(path);
}

void QDirectFBPaintEngine::drawPoints(const QPointF *points, int pointCount)
{
    RASTERFALLBACK(DRAW_POINTS, pointCount, VOID_ARG(), VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPoints(points, pointCount);
}

void QDirectFBPaintEngine::drawPoints(const QPoint *points, int pointCount)
{
    RASTERFALLBACK(DRAW_POINTS, pointCount, VOID_ARG(), VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPoints(points, pointCount);
}

void QDirectFBPaintEngine::drawEllipse(const QRectF &rect)
{
    RASTERFALLBACK(DRAW_ELLIPSE, rect, VOID_ARG(), VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::drawEllipse(rect);
}

void QDirectFBPaintEngine::drawPolygon(const QPointF *points, int pointCount,
                                       PolygonDrawMode mode)
{
    RASTERFALLBACK(DRAW_POLYGON, pointCount, mode, VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPolygon(points, pointCount, mode);
}

void QDirectFBPaintEngine::drawPolygon(const QPoint *points, int pointCount,
                                       PolygonDrawMode mode)
{
    RASTERFALLBACK(DRAW_POLYGON, pointCount, mode, VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPolygon(points, pointCount, mode);
}

void QDirectFBPaintEngine::drawTextItem(const QPointF &p,
                                        const QTextItem &textItem)
{
    RASTERFALLBACK(DRAW_TEXT, p, textItem.text(), VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::drawTextItem(p, textItem);
}

void QDirectFBPaintEngine::fill(const QVectorPath &path, const QBrush &brush)
{
    if (brush.style() == Qt::NoBrush)
        return;
    RASTERFALLBACK(FILL_PATH, path, brush, VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::fill(path, brush);
}

void QDirectFBPaintEngine::drawRoundedRect(const QRectF &rect, qreal xrad, qreal yrad, Qt::SizeMode mode)
{
    RASTERFALLBACK(DRAW_ROUNDED_RECT, rect, xrad, yrad);
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::drawRoundedRect(rect, xrad, yrad, mode);
}

void QDirectFBPaintEngine::drawStaticTextItem(QStaticTextItem *item)
{
    RASTERFALLBACK(DRAW_STATICTEXT, item, VOID_ARG(), VOID_ARG());
    Q_D(QDirectFBPaintEngine);
    d->lock();
    QRasterPaintEngine::drawStaticTextItem(item);
}

void QDirectFBPaintEngine::fillRect(const QRectF &rect, const QBrush &brush)
{
    Q_D(QDirectFBPaintEngine);
    if (brush.style() == Qt::NoBrush)
        return;
    if (d->clipType != QDirectFBPaintEnginePrivate::ComplexClip) {
        switch (brush.style()) {
        case Qt::SolidPattern: {
            const QColor color = brush.color();
            if (!color.isValid())
                return;

            if (d->transformationType & QDirectFBPaintEnginePrivate::Matrix_RectsUnsupported
                || !(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)) {
                break;
            }
            d->setDFBColor(color);
            const QRect r = state()->matrix.mapRect(rect).toRect();
            CLIPPED_PAINT(d->surface->FillRectangle(d->surface, r.x(), r.y(), r.width(), r.height()));
            return; }

        case Qt::TexturePattern: {
            const QPointF &brushOrigin = state()->brushOrigin;
            const QTransform stateTransform = state()->matrix;
            QTransform transform(stateTransform);
            transform.translate(brushOrigin.x(), brushOrigin.y());
            transform = brush.transform() * transform;
            if (!(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)
                || (QDirectFBPaintEnginePrivate::getTransformationType(transform) & QDirectFBPaintEnginePrivate::Matrix_BlitsUnsupported)
                || (!d->supportsStretchBlit() && transform.isScaling())) {
                break;
            }

            const QPixmap texture = brush.texture();
            if (texture.pixmapData()->classId() != QPixmapData::DirectFBClass)
                break;

            CLIPPED_PAINT(d->drawTiledPixmap(stateTransform.mapRect(rect), texture, rect.topLeft() - brushOrigin, transform));
            return; }
        default:
            break;
        }
    }
    RASTERFALLBACK(FILL_RECT, rect, brush, VOID_ARG());
    d->lock();
    QRasterPaintEngine::fillRect(rect, brush);
}

void QDirectFBPaintEngine::fillRect(const QRectF &rect, const QColor &color)
{
    if (!color.isValid())
        return;
    Q_D(QDirectFBPaintEngine);
    if ((d->transformationType & QDirectFBPaintEnginePrivate::Matrix_RectsUnsupported)
        || (d->clipType == QDirectFBPaintEnginePrivate::ComplexClip)
        || !(d->compositionModeStatus & QDirectFBPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(FILL_RECT, rect, color, VOID_ARG());
        d->lock();
        QRasterPaintEngine::fillRect(rect, color);
    } else {
        d->setDFBColor(color);
        const QRect r = state()->matrix.mapRect(rect).toRect();
        CLIPPED_PAINT(d->surface->FillRectangle(d->surface, r.x(), r.y(), r.width(), r.height()));
    }
}

void QDirectFBPaintEngine::drawBufferSpan(const uint *buffer, int bufsize,
                                          int x, int y, int length,
                                          uint const_alpha)
{
    Q_D(QDirectFBPaintEngine);
    IDirectFBSurface *src = d->surfaceCache->getSurface(buffer, bufsize);
    // ### how does this play with setDFBColor
    src->SetColor(src, 0, 0, 0, const_alpha);
    const DFBRectangle rect = { 0, 0, length, 1 };
    d->surface->Blit(d->surface, src, &rect, x, y);
}

#ifdef QT_DIRECTFB_IMAGECACHE
static void cachedImageCleanupHook(qint64 key)
{
    delete imageCache.take(key);
}
void QDirectFBPaintEngine::initImageCache(int size)
{
    Q_ASSERT(size >= 0);
    imageCache.setMaxCost(size);
    QImagePixmapCleanupHooks::instance()->addImageHook(cachedImageCleanupHook);
}

#endif // QT_DIRECTFB_IMAGECACHE

// ---- QDirectFBPaintEnginePrivate ----


QDirectFBPaintEnginePrivate::QDirectFBPaintEnginePrivate(QDirectFBPaintEngine *p)
    : surface(0), antialiased(false), simplePen(false),
      transformationType(0), opacity(255),
      clipType(ClipUnset), dfbDevice(0),
      compositionModeStatus(0), isPremultiplied(false), inClip(false), q(p)
{
    fb = QDirectFBScreen::instance()->dfb();
    surfaceCache = new SurfaceCache;
}

QDirectFBPaintEnginePrivate::~QDirectFBPaintEnginePrivate()
{
    delete surfaceCache;
}

bool QDirectFBPaintEnginePrivate::isSimpleBrush(const QBrush &brush) const
{
    return (brush.style() == Qt::NoBrush) || (brush.style() == Qt::SolidPattern && !antialiased);
}

void QDirectFBPaintEnginePrivate::lock()
{
    // We will potentially get a new pointer to the buffer after a
    // lock so we need to call the base implementation of prepare so
    // it updates its rasterBuffer to point to the new buffer address.
    Q_ASSERT(dfbDevice);
    if (dfbDevice->lockSurface(DSLF_READ|DSLF_WRITE)) {
        prepare(dfbDevice);
    }
}

void QDirectFBPaintEnginePrivate::unlock()
{
    Q_ASSERT(dfbDevice);
#ifdef QT_DIRECTFB_SUBSURFACE
    dfbDevice->syncPending = true;
#else
    QDirectFBPaintEnginePrivate::unlock(dfbDevice);
#endif
}

void QDirectFBPaintEnginePrivate::unlock(QDirectFBPaintDevice *device)
{
#ifdef QT_NO_DIRECTFB_SUBSURFACE
    Q_ASSERT(device);
    device->unlockSurface();
#else
    Q_UNUSED(device);
#endif
}

void QDirectFBPaintEnginePrivate::setTransform(const QTransform &transform)
{
    transformationType = getTransformationType(transform);
    setPen(q->state()->pen);
}

void QDirectFBPaintEnginePrivate::setPen(const QPen &pen)
{
    if (pen.style() == Qt::NoPen) {
        simplePen = true;
    } else if (pen.style() == Qt::SolidLine
               && !antialiased
               && pen.brush().style() == Qt::SolidPattern
               && pen.widthF() <= 1.0
               && (transformationType < QTransform::TxScale || pen.isCosmetic())) {
        simplePen = true;
    } else {
        simplePen = false;
    }
}

void QDirectFBPaintEnginePrivate::setCompositionMode(QPainter::CompositionMode mode)
{
    if (!surface)
        return;

    static const bool forceRasterFallBack = qgetenv("QT_DIRECTFB_FORCE_RASTER").toInt() > 0;
    if (forceRasterFallBack) {
        compositionModeStatus = PorterDuff_None;
        return;
    }

    compositionModeStatus = PorterDuff_Supported|PorterDuff_PremultiplyColors|PorterDuff_AlwaysBlend;
    switch (mode) {
    case QPainter::CompositionMode_Clear:
        surface->SetPorterDuff(surface, DSPD_CLEAR);
        break;
    case QPainter::CompositionMode_Source:
        surface->SetPorterDuff(surface, DSPD_SRC);
        compositionModeStatus &= ~PorterDuff_AlwaysBlend;
        if (!isPremultiplied)
            compositionModeStatus &= ~PorterDuff_PremultiplyColors;
        break;
    case QPainter::CompositionMode_SourceOver:
        compositionModeStatus &= ~PorterDuff_AlwaysBlend;
        surface->SetPorterDuff(surface, DSPD_SRC_OVER);
        break;
    case QPainter::CompositionMode_DestinationOver:
        surface->SetPorterDuff(surface, DSPD_DST_OVER);
        break;
    case QPainter::CompositionMode_SourceIn:
        surface->SetPorterDuff(surface, DSPD_SRC_IN);
        if (!isPremultiplied)
            compositionModeStatus &= ~PorterDuff_PremultiplyColors;
        break;
    case QPainter::CompositionMode_DestinationIn:
        surface->SetPorterDuff(surface, DSPD_DST_IN);
        break;
    case QPainter::CompositionMode_SourceOut:
        surface->SetPorterDuff(surface, DSPD_SRC_OUT);
        break;
    case QPainter::CompositionMode_DestinationOut:
        surface->SetPorterDuff(surface, DSPD_DST_OUT);
        break;
    case QPainter::CompositionMode_Destination:
        surface->SetSrcBlendFunction(surface, DSBF_ZERO);
        surface->SetDstBlendFunction(surface, DSBF_ONE);
        break;
#if (Q_DIRECTFB_VERSION >= 0x010000)
    case QPainter::CompositionMode_SourceAtop:
        surface->SetPorterDuff(surface, DSPD_SRC_ATOP);
        break;
    case QPainter::CompositionMode_DestinationAtop:
        surface->SetPorterDuff(surface, DSPD_DST_ATOP);
        break;
    case QPainter::CompositionMode_Plus:
        surface->SetPorterDuff(surface, DSPD_ADD);
        break;
    case QPainter::CompositionMode_Xor:
        surface->SetPorterDuff(surface, DSPD_XOR);
        break;
#endif
    default:
        compositionModeStatus = PorterDuff_None;
        break;
    }
}

void QDirectFBPaintEnginePrivate::setRenderHints(QPainter::RenderHints hints)
{
    const bool old = antialiased;
    antialiased = bool(hints & QPainter::Antialiasing);
    if (old != antialiased) {
        setPen(q->state()->pen);
    }
}

void QDirectFBPaintEnginePrivate::prepareForBlit(uint flags)
{
    DFBSurfaceBlittingFlags blittingFlags = DSBLIT_NOFX;
    if (flags & Premultiplied)
        blittingFlags |= DSBLIT_SRC_PREMULTIPLY;
    if (flags & HasAlpha)
        blittingFlags |= DSBLIT_BLEND_ALPHACHANNEL;
    if (opacity != 255) {
        blittingFlags |= DSBLIT_BLEND_COLORALPHA;
        surface->SetColor(surface, 0xff, 0xff, 0xff, opacity);
    }

    surface->SetBlittingFlags(surface, blittingFlags);
}

static inline uint ALPHA_MUL(uint x, uint a)
{
    uint t = x * a;
    t = ((t + (t >> 8) + 0x80) >> 8) & 0xff;
    return t;
}

void QDirectFBPaintEnginePrivate::setDFBColor(const QColor &color)
{
    Q_ASSERT(surface);
    Q_ASSERT(compositionModeStatus & PorterDuff_Supported);
    const quint8 alpha = (opacity == 255 ?
                          color.alpha() : ALPHA_MUL(color.alpha(), opacity));
    QColor col;
    if (compositionModeStatus & PorterDuff_PremultiplyColors) {
        col = QColor(ALPHA_MUL(color.red(), alpha),
                     ALPHA_MUL(color.green(), alpha),
                     ALPHA_MUL(color.blue(), alpha),
                     alpha);
    } else {
        col = QColor(color.red(), color.green(), color.blue(), alpha);
    }
    surface->SetColor(surface, col.red(), col.green(), col.blue(), col.alpha());
    surface->SetDrawingFlags(surface, alpha == 255 && !(compositionModeStatus & PorterDuff_AlwaysBlend) ? DSDRAW_NOFX : DSDRAW_BLEND);
}

IDirectFBSurface *QDirectFBPaintEnginePrivate::getSurface(const QImage &img, bool *release)
{
#ifdef QT_NO_DIRECTFB_IMAGECACHE
    *release = true;
    return QDirectFBScreen::instance()->createDFBSurface(img, img.format(), QDirectFBScreen::DontTrackSurface);
#else
    const qint64 key = img.cacheKey();
    *release = false;
    if (imageCache.contains(key)) {
        return imageCache[key]->surface;
    }

    const int cost = cacheCost(img);
    const bool cache = cost <= imageCache.maxCost();
    QDirectFBScreen *screen = QDirectFBScreen::instance();
    const QImage::Format format = (img.format() == screen->alphaPixmapFormat() || QDirectFBPixmapData::hasAlphaChannel(img)
                                   ? screen->alphaPixmapFormat() : screen->pixelFormat());

    IDirectFBSurface *surface = screen->createDFBSurface(img, format,
                                                         cache
                                                         ? QDirectFBScreen::TrackSurface
                                                         : QDirectFBScreen::DontTrackSurface);
    if (cache) {
        CachedImage *cachedImage = new CachedImage;
        const_cast<QImage&>(img).data_ptr()->is_cached = true;
        cachedImage->surface = surface;
        imageCache.insert(key, cachedImage, cost);
    } else {
        *release = true;
    }
    return surface;
#endif
}


void QDirectFBPaintEnginePrivate::blit(const QRectF &dest, IDirectFBSurface *s, const QRectF &src)
{
    const QRect sr = src.toRect();
    const QRect dr = q->state()->matrix.mapRect(dest).toRect();
    if (dr.isEmpty())
        return;
    const DFBRectangle sRect = { sr.x(), sr.y(), sr.width(), sr.height() };
    DFBResult result;

    if (dr.size() == sr.size()) {
        result = surface->Blit(surface, s, &sRect, dr.x(), dr.y());
    } else {
        Q_ASSERT(supportsStretchBlit());
        const DFBRectangle dRect = { dr.x(), dr.y(), dr.width(), dr.height() };
        result = surface->StretchBlit(surface, s, &sRect, &dRect);
    }
    if (result != DFB_OK)
        DirectFBError("QDirectFBPaintEngine::drawPixmap()", result);
}

static inline qreal fixCoord(qreal rect_pos, qreal pixmapSize, qreal offset)
{
    qreal pos = rect_pos - offset;
    while (pos > rect_pos)
        pos -= pixmapSize;
    while (pos + pixmapSize < rect_pos)
        pos += pixmapSize;
    return pos;
}

void QDirectFBPaintEnginePrivate::drawTiledPixmap(const QRectF &dest, const QPixmap &pixmap,
                                                  const QPointF &off, const QTransform &pixmapTransform)
{
    const QTransform &transform = q->state()->matrix;
    Q_ASSERT(!(getTransformationType(transform) & Matrix_BlitsUnsupported) &&
             !(getTransformationType(pixmapTransform) & Matrix_BlitsUnsupported));
    const QRect destinationRect = transform.mapRect(dest).toRect().normalized();
    QRect newClip = destinationRect;
    if (!currentClip.isEmpty())
        newClip &= currentClip;

    if (newClip.isNull())
        return;

    const DFBRegion clip = {
        newClip.x(),
        newClip.y(),
        newClip.right(),
        newClip.bottom()
    };
    surface->SetClip(surface, &clip);

    QPointF offset = pixmapTransform.inverted().map(off);
    Q_ASSERT(transform.type() <= QTransform::TxScale);
    QPixmapData *data = pixmap.pixmapData();
    Q_ASSERT(data->classId() == QPixmapData::DirectFBClass);
    QDirectFBPixmapData *dfbData = static_cast<QDirectFBPixmapData*>(data);
    IDirectFBSurface *sourceSurface = dfbData->directFBSurface();
    uint blitFlags = 0;
    if (dfbData->hasAlphaChannel())
        blitFlags |= HasAlpha;
    if (QDirectFBScreen::isPremultiplied(dfbData->pixelFormat()))
        blitFlags |= Premultiplied;
    prepareForBlit(blitFlags);
    QDirectFBPaintEnginePrivate::unlock(dfbData);
    const QSize pixmapSize = dfbData->size();
    if (transform.isScaling() || pixmapTransform.isScaling()) {
        Q_ASSERT(supportsStretchBlit());
        Q_ASSERT(qMin(transform.m11(), transform.m22()) >= 0);
        offset.rx() *= transform.m11();
        offset.ry() *= transform.m22();

        const QSizeF mappedSize(pixmapSize.width() * pixmapTransform.m11(), pixmapSize.height() * pixmapTransform.m22());
        qreal y = fixCoord(destinationRect.y(), mappedSize.height(), offset.y());
        const qreal startX = fixCoord(destinationRect.x(), mappedSize.width(), offset.x());
        while (y <= destinationRect.bottom()) {
            qreal x = startX;
            while (x <= destinationRect.right()) {
                const DFBRectangle destination = { qRound(x), qRound(y), mappedSize.width(), mappedSize.height() };
                surface->StretchBlit(surface, sourceSurface, 0, &destination);
                x += mappedSize.width();
            }
            y += mappedSize.height();
        }
    } else {
        qreal y = fixCoord(destinationRect.y(), pixmapSize.height(), offset.y());
        const qreal startX = fixCoord(destinationRect.x(), pixmapSize.width(), offset.x());
        int horizontal = qMax(1, destinationRect.width() / pixmapSize.width()) + 1;
        if (startX != destinationRect.x())
            ++horizontal;
        int vertical = qMax(1, destinationRect.height() / pixmapSize.height()) + 1;
        if (y != destinationRect.y())
            ++vertical;

        const int maxCount = (vertical * horizontal);
        QVarLengthArray<DFBRectangle, 16> sourceRects(maxCount);
        QVarLengthArray<DFBPoint, 16> points(maxCount);

        int i = 0;
        while (y <= destinationRect.bottom()) {
            Q_ASSERT(i < maxCount);
            qreal x = startX;
            while (x <= destinationRect.right()) {
                points[i].x = qRound(x);
                points[i].y = qRound(y);
                sourceRects[i].x = 0;
                sourceRects[i].y = 0;
                sourceRects[i].w = int(pixmapSize.width());
                sourceRects[i].h = int(pixmapSize.height());
                x += pixmapSize.width();
                ++i;
            }
            y += pixmapSize.height();
        }
        surface->BatchBlit(surface, sourceSurface, sourceRects.constData(), points.constData(), i);
    }

    if (currentClip.isEmpty()) {
        surface->SetClip(surface, 0);
    } else {
        const DFBRegion clip = {
            currentClip.x(),
            currentClip.y(),
            currentClip.right(),
            currentClip.bottom()
        };
        surface->SetClip(surface, &clip);
    }
}

void QDirectFBPaintEnginePrivate::updateClip()
{
    Q_ASSERT(surface);
    currentClip = QRect();
    const QClipData *clipData = clip();
    if (!clipData || !clipData->enabled) {
        surface->SetClip(surface, NULL);
        clipType = NoClip;
    } else if (clipData->hasRectClip) {
        const DFBRegion r = {
            clipData->clipRect.x(),
            clipData->clipRect.y(),
            clipData->clipRect.right(),
            clipData->clipRect.bottom()
        };
        surface->SetClip(surface, &r);
        currentClip = clipData->clipRect.normalized();
        // ### is this guaranteed to always be normalized?
        clipType = RectClip;
    } else if (clipData->hasRegionClip) {
        clipType = RegionClip;
    } else {
        clipType = ComplexClip;
    }
}

bool QDirectFBPaintEnginePrivate::supportsStretchBlit() const
{
#ifdef QT_DIRECTFB_STRETCHBLIT
    return !(q->state()->renderHints & QPainter::SmoothPixmapTransform);
#else
    return false;
#endif
}


void QDirectFBPaintEnginePrivate::systemStateChanged()
{
    QRasterPaintEnginePrivate::systemStateChanged();
    updateClip();
}

IDirectFBSurface *SurfaceCache::getSurface(const uint *buf, int size)
{
    if (buffer == buf && bufsize == size)
        return surface;

    clear();

    const DFBSurfaceDescription description = QDirectFBScreen::getSurfaceDescription(buf, size);
    surface = QDirectFBScreen::instance()->createDFBSurface(description, QDirectFBScreen::TrackSurface, 0);
    if (!surface)
        qWarning("QDirectFBPaintEngine: SurfaceCache: Unable to create surface");

    buffer = const_cast<uint*>(buf);
    bufsize = size;

    return surface;
}

void SurfaceCache::clear()
{
    if (surface && QDirectFBScreen::instance())
        QDirectFBScreen::instance()->releaseDFBSurface(surface);
    surface = 0;
    buffer = 0;
    bufsize = 0;
}


static inline QRect mapRect(const QTransform &transform, const QRect &rect) { return transform.mapRect(rect); }
static inline QRect mapRect(const QTransform &transform, const QRectF &rect) { return transform.mapRect(rect).toRect(); }
static inline QLine map(const QTransform &transform, const QLine &line) { return transform.map(line); }
static inline QLine map(const QTransform &transform, const QLineF &line) { return transform.map(line).toLine(); }
template <class T>
static inline void drawLines(const T *lines, int n, const QTransform &transform, IDirectFBSurface *surface)
{
    if (n == 1) {
        const QLine l = map(transform, lines[0]);
        surface->DrawLine(surface, l.x1(), l.y1(), l.x2(), l.y2());
    } else {
        QVarLengthArray<DFBRegion, 32> lineArray(n);
        for (int i=0; i<n; ++i) {
            const QLine l = map(transform, lines[i]);
            lineArray[i].x1 = l.x1();
            lineArray[i].y1 = l.y1();
            lineArray[i].x2 = l.x2();
            lineArray[i].y2 = l.y2();
        }
        surface->DrawLines(surface, lineArray.constData(), n);
    }
}

template <class T>
static inline void fillRects(const T *rects, int n, const QTransform &transform, IDirectFBSurface *surface)
{
    if (n == 1) {
        const QRect r = mapRect(transform, rects[0]);
        surface->FillRectangle(surface, r.x(), r.y(), r.width(), r.height());
    } else {
        QVarLengthArray<DFBRectangle, 32> rectArray(n);
        for (int i=0; i<n; ++i) {
            const QRect r = mapRect(transform, rects[i]);
            rectArray[i].x = r.x();
            rectArray[i].y = r.y();
            rectArray[i].w = r.width();
            rectArray[i].h = r.height();
        }
        surface->FillRectangles(surface, rectArray.constData(), n);
    }
}

template <class T>
static inline void drawRects(const T *rects, int n, const QTransform &transform, IDirectFBSurface *surface)
{
    for (int i=0; i<n; ++i) {
        const QRect r = mapRect(transform, rects[i]);
        surface->DrawRectangle(surface, r.x(), r.y(), r.width(), r.height());
    }
}

template <typename T> inline const T *ptr(const T &t) { return &t; }
template <> inline const bool* ptr<bool>(const bool &) { return 0; }
template <typename device, typename T1, typename T2, typename T3>
static void rasterFallbackWarn(const char *msg, const char *func, const device *dev,
                               uint transformationType, bool simplePen,
                               uint clipType, uint compositionModeStatus,
                               const char *nameOne, const T1 &one,
                               const char *nameTwo, const T2 &two,
                               const char *nameThree, const T3 &three)
{
    QString out;
    QDebug dbg(&out);
    dbg << msg << (QByteArray(func) + "()")  << "painting on";
    if (dev->devType() == QInternal::Widget) {
        dbg << static_cast<const QWidget*>(dev);
    } else {
        dbg << dev << "of type" << dev->devType();
    }

    dbg << QString::fromLatin1("transformationType 0x%1").arg(transformationType, 3, 16, QLatin1Char('0'))
        << "simplePen" << simplePen
        << "clipType" << clipType
        << "compositionModeStatus" << compositionModeStatus;

    const T1 *t1 = ptr(one);
    const T2 *t2 = ptr(two);
    const T3 *t3 = ptr(three);

    if (t1) {
        dbg << nameOne << *t1;
        if (t2) {
            dbg << nameTwo << *t2;
            if (t3) {
                dbg << nameThree << *t3;
            }
        }
    }
    qWarning("%s", qPrintable(out));
}

QT_END_NAMESPACE

#endif // QT_NO_QWS_DIRECTFB
