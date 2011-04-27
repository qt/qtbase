/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenVG module of the Qt Toolkit.
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

#include "qpaintengine_vg_p.h"
#include "qpixmapdata_vg_p.h"
#include "qpixmapfilter_vg_p.h"
#include "qvgcompositionhelper_p.h"
#include "qvgimagepool_p.h"
#include "qvgfontglyphcache_p.h"
#if !defined(QT_NO_EGL)
#include <QtGui/private/qeglcontext_p.h>
#include "qwindowsurface_vgegl_p.h"
#endif
#include <QtCore/qvarlengtharray.h>
#include <QtGui/private/qdrawhelper_p.h>
#include <QtGui/private/qtextengine_p.h>
#include <QtGui/private/qfontengine_p.h>
#include <QtGui/private/qpainterpath_p.h>
#include <QtGui/private/qstatictext_p.h>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtCore/qmath.h>
#include <QDebug>
#include <QSet>

QT_BEGIN_NAMESPACE

// vgRenderToMask() only exists in OpenVG 1.1 and higher.
// Also, disable masking completely if we are using the scissor to clip.
#if !defined(OPENVG_VERSION_1_1) && !defined(QVG_NO_RENDER_TO_MASK)
#define QVG_NO_RENDER_TO_MASK 1
#endif
#if defined(QVG_SCISSOR_CLIP) && !defined(QVG_NO_RENDER_TO_MASK)
#define QVG_NO_RENDER_TO_MASK 1
#endif

// use the same rounding as in qrasterizer.cpp (6 bit fixed point)
static const qreal aliasedCoordinateDelta = 0.5 - 0.015625;

#if !defined(QVG_NO_DRAW_GLYPHS)

class QVGPaintEnginePrivate;

typedef QHash<QFontEngine*, QVGFontGlyphCache*> QVGFontCache;

#endif

class QVGFontEngineCleaner : public QObject
{
    Q_OBJECT
public:
    QVGFontEngineCleaner(QVGPaintEnginePrivate *d);
    ~QVGFontEngineCleaner();

public slots:
    void fontEngineDestroyed();

private:
    QVGPaintEnginePrivate *d_ptr;
};

class QVGPaintEnginePrivate : public QPaintEngineExPrivate
{
    Q_DECLARE_PUBLIC(QVGPaintEngine)
public:
    // Extra blending modes from VG_KHR_advanced_blending extension.
    // Use the QT_VG prefix to avoid conflicts with any definitions
    // that may come in via <VG/vgext.h>.
    enum AdvancedBlending {
        QT_VG_BLEND_OVERLAY_KHR       = 0x2010,
        QT_VG_BLEND_HARDLIGHT_KHR     = 0x2011,
        QT_VG_BLEND_SOFTLIGHT_SVG_KHR = 0x2012,
        QT_VG_BLEND_SOFTLIGHT_KHR     = 0x2013,
        QT_VG_BLEND_COLORDODGE_KHR    = 0x2014,
        QT_VG_BLEND_COLORBURN_KHR     = 0x2015,
        QT_VG_BLEND_DIFFERENCE_KHR    = 0x2016,
        QT_VG_BLEND_SUBTRACT_KHR      = 0x2017,
        QT_VG_BLEND_INVERT_KHR        = 0x2018,
        QT_VG_BLEND_EXCLUSION_KHR     = 0x2019,
        QT_VG_BLEND_LINEARDODGE_KHR   = 0x201a,
        QT_VG_BLEND_LINEARBURN_KHR    = 0x201b,
        QT_VG_BLEND_VIVIDLIGHT_KHR    = 0x201c,
        QT_VG_BLEND_LINEARLIGHT_KHR   = 0x201d,
        QT_VG_BLEND_PINLIGHT_KHR      = 0x201e,
        QT_VG_BLEND_HARDMIX_KHR       = 0x201f,
        QT_VG_BLEND_CLEAR_KHR         = 0x2020,
        QT_VG_BLEND_DST_KHR           = 0x2021,
        QT_VG_BLEND_SRC_OUT_KHR       = 0x2022,
        QT_VG_BLEND_DST_OUT_KHR       = 0x2023,
        QT_VG_BLEND_SRC_ATOP_KHR      = 0x2024,
        QT_VG_BLEND_DST_ATOP_KHR      = 0x2025,
        QT_VG_BLEND_XOR_KHR           = 0x2026
    };

    QVGPaintEnginePrivate(QVGPaintEngine *q_ptr);
    ~QVGPaintEnginePrivate();

    void init();
    void initObjects();
    void destroy();
    void setTransform(VGMatrixMode mode, const QTransform& transform);
    void updateTransform(QPaintDevice *pdev);
    void draw(VGPath path, const QPen& pen, const QBrush& brush, VGint rule = VG_EVEN_ODD);
    void stroke(VGPath path, const QPen& pen);
    void fill(VGPath path, const QBrush& brush, VGint rule = VG_EVEN_ODD);
    VGPath vectorPathToVGPath(const QVectorPath& path);
    VGPath painterPathToVGPath(const QPainterPath& path);
    VGPath roundedRectPath(const QRectF &rect, qreal xRadius, qreal yRadius, Qt::SizeMode mode);
    VGPaintType setBrush
        (VGPaint paint, const QBrush& brush, VGMatrixMode mode,
         VGPaintType prevPaintType);
    void setPenParams(const QPen& pen);
    void setBrushTransform(const QBrush& brush, VGMatrixMode mode);
    void setupColorRamp(const QGradient *grad, VGPaint paint);
    void setImageOptions();
    void systemStateChanged();
#if !defined(QVG_SCISSOR_CLIP)
    void ensureMask(QVGPaintEngine *engine, int width, int height);
    void modifyMask
        (QVGPaintEngine *engine, VGMaskOperation op, const QRegion& region);
    void modifyMask
        (QVGPaintEngine *engine, VGMaskOperation op, const QRect& rect);
#endif

    VGint maxScissorRects;  // Maximum scissor rectangles for clipping.

    VGPaint penPaint;       // Paint for currently active pen.
    VGPaint brushPaint;     // Paint for currently active brush.
    VGPaint opacityPaint;   // Paint for drawing images with opacity.
    VGPaint fillPaint;      // Current fill paint that is active.

    QPen currentPen;        // Current pen set in "penPaint".
    QBrush currentBrush;    // Current brush set in "brushPaint".

    bool forcePenChange;    // Force a pen change, even if the same.
    bool forceBrushChange;  // Force a brush change, even if the same.

    VGPaintType penType;    // Type of the last pen that was set.
    VGPaintType brushType;  // Type of the last brush that was set.

    QPointF brushOrigin;    // Current brush origin.

    VGint fillRule;         // Last fill rule that was set.

    qreal opacity;          // Current drawing opacity.
    qreal paintOpacity;     // Opacity in opacityPaint.

#if !defined(QVG_NO_MODIFY_PATH)
    VGPath rectPath;        // Cached path for quick drawing of rectangles.
    VGPath linePath;        // Cached path for quick drawing of lines.
    VGPath roundRectPath;   // Cached path for quick drawing of rounded rects.
#endif

    QTransform transform;   // Currently active transform.
    bool simpleTransform;   // True if the transform is simple (non-projective).
    qreal penScale;         // Pen scaling factor from "transform".

    QTransform pathTransform;  // Calculated VG path transformation.
    QTransform imageTransform; // Calculated VG image transformation.
    bool pathTransformSet;  // True if path transform set in the VG context.

    bool maskValid;         // True if vgMask() contains valid data.
    bool maskIsSet;         // True if mask would be fully set if it was valid.
    bool scissorMask;       // True if scissor is used in place of the mask.
    bool rawVG;             // True if processing a raw VG escape.

    QRect maskRect;         // Rectangle version of mask if it is simple.

    QTransform penTransform;   // Transform for the pen.
    QTransform brushTransform; // Transform for the brush.

    VGMatrixMode matrixMode;    // Last matrix mode that was set.
    VGImageMode imageMode;      // Last image mode that was set.

    QRegion scissorRegion;  // Currently active scissor region.
    bool scissorActive;     // True if scissor region is active.
    bool scissorDirty;      // True if scissor is dirty after native painting.

    QPaintEngine::DirtyFlags dirty;

    QColor clearColor;      // Last clear color that was set.
    VGfloat clearOpacity;   // Opacity during the last clear.

    VGBlendMode blendMode;  // Active blend mode.
    VGRenderingQuality renderingQuality; // Active rendering quality.
    VGImageQuality imageQuality;    // Active image quality.

#if !defined(QVG_NO_DRAW_GLYPHS)
    QVGFontCache fontCache;
    QVGFontEngineCleaner *fontEngineCleaner;
#endif

    bool hasAdvancedBlending;

    QScopedPointer<QPixmapFilter> convolutionFilter;
    QScopedPointer<QPixmapFilter> colorizeFilter;
    QScopedPointer<QPixmapFilter> dropShadowFilter;
    QScopedPointer<QPixmapFilter> blurFilter;

    // Ensure that the path transform is properly set in the VG context
    // before we perform a vgDrawPath() operation.
    inline void ensurePathTransform()
    {
        if (!pathTransformSet) {
            QTransform aliasedTransform = pathTransform;
            if (renderingQuality == VG_RENDERING_QUALITY_NONANTIALIASED && currentPen != Qt::NoPen)
                aliasedTransform = aliasedTransform
                    * QTransform::fromTranslate(aliasedCoordinateDelta, -aliasedCoordinateDelta);
            setTransform(VG_MATRIX_PATH_USER_TO_SURFACE, aliasedTransform);
            pathTransformSet = true;
        }
    }

    // Ensure that a specific pen has been set into penPaint.
    inline void ensurePen(const QPen& pen) {
        if (forcePenChange || pen != currentPen) {
            currentPen = pen;
            forcePenChange = false;
            penType = setBrush
                (penPaint, pen.brush(),
                 VG_MATRIX_STROKE_PAINT_TO_USER, penType);
            setPenParams(pen);
        }
    }

    // Ensure that a specific brush has been set into brushPaint.
    inline void ensureBrush(const QBrush& brush) {
        if (forceBrushChange || brush != currentBrush) {
            currentBrush = brush;
            forceBrushChange = false;
            brushType = setBrush
                (brushPaint, brush, VG_MATRIX_FILL_PAINT_TO_USER, brushType);
        }
        if (fillPaint != brushPaint) {
            vgSetPaint(brushPaint, VG_FILL_PATH);
            fillPaint = brushPaint;
        }
    }

    // Set various modes, but only if different.
    inline void setImageMode(VGImageMode mode);
    inline void setRenderingQuality(VGRenderingQuality mode);
    inline void setImageQuality(VGImageQuality mode);
    inline void setBlendMode(VGBlendMode mode);
    inline void setFillRule(VGint mode);

    // Clear all lazily-set modes.
    void clearModes();

private:
    QVGPaintEngine *q;
};

inline void QVGPaintEnginePrivate::setImageMode(VGImageMode mode)
{
    if (imageMode != mode) {
        imageMode = mode;
        vgSeti(VG_IMAGE_MODE, mode);
    }
}

inline void QVGPaintEnginePrivate::setRenderingQuality(VGRenderingQuality mode)
{
    if (renderingQuality != mode) {
        vgSeti(VG_RENDERING_QUALITY, mode);
        renderingQuality = mode;
        pathTransformSet = false; // need to tweak transform for aliased stroking
    }
}

inline void QVGPaintEnginePrivate::setImageQuality(VGImageQuality mode)
{
    if (imageQuality != mode) {
        vgSeti(VG_IMAGE_QUALITY, mode);
        imageQuality = mode;
    }
}

inline void QVGPaintEnginePrivate::setBlendMode(VGBlendMode mode)
{
    if (blendMode != mode) {
        vgSeti(VG_BLEND_MODE, mode);
        blendMode = mode;
    }
}

inline void QVGPaintEnginePrivate::setFillRule(VGint mode)
{
    if (fillRule != mode) {
        fillRule = mode;
        vgSeti(VG_FILL_RULE, mode);
    }
}

void QVGPaintEnginePrivate::clearModes()
{
    matrixMode = (VGMatrixMode)0;
    imageMode = (VGImageMode)0;
    blendMode = (VGBlendMode)0;
    renderingQuality = (VGRenderingQuality)0;
    imageQuality = (VGImageQuality)0;
}

QVGPaintEnginePrivate::QVGPaintEnginePrivate(QVGPaintEngine *q_ptr) : q(q_ptr)
{
    init();
}

void QVGPaintEnginePrivate::init()
{
    maxScissorRects = 0;

    penPaint = 0;
    brushPaint = 0;
    opacityPaint = 0;
    fillPaint = 0;

    forcePenChange = true;
    forceBrushChange = true;
    penType = (VGPaintType)0;
    brushType = (VGPaintType)0;

    brushOrigin = QPointF(0.0f, 0.0f);

    fillRule = 0;

    opacity = 1.0;
    paintOpacity = 1.0f;

#if !defined(QVG_NO_MODIFY_PATH)
    rectPath = 0;
    linePath = 0;
    roundRectPath = 0;
#endif

    simpleTransform = true;
    pathTransformSet = false;
    penScale = 1.0;

    maskValid = false;
    maskIsSet = false;
    scissorMask = false;
    rawVG = false;

    scissorActive = false;
    scissorDirty = false;

    dirty = 0;

    clearOpacity = 1.0f;

#if !defined(QVG_NO_DRAW_GLYPHS)
    fontEngineCleaner = 0;
#endif

    hasAdvancedBlending = false;

    clearModes();
}

QVGPaintEnginePrivate::~QVGPaintEnginePrivate()
{
    destroy();
}

void QVGPaintEnginePrivate::initObjects()
{
    maxScissorRects = vgGeti(VG_MAX_SCISSOR_RECTS);

    penPaint = vgCreatePaint();
    vgSetParameteri(penPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetPaint(penPaint, VG_STROKE_PATH);

    vgSeti(VG_MATRIX_MODE, VG_MATRIX_STROKE_PAINT_TO_USER);
    vgLoadIdentity();

    brushPaint = vgCreatePaint();
    vgSetParameteri(brushPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetPaint(brushPaint, VG_FILL_PATH);
    fillPaint = brushPaint;

    vgSeti(VG_MATRIX_MODE, VG_MATRIX_FILL_PAINT_TO_USER);
    vgLoadIdentity();
    matrixMode = VG_MATRIX_FILL_PAINT_TO_USER;

    opacityPaint = vgCreatePaint();
    vgSetParameteri(opacityPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    VGfloat values[4];
    values[0] = 1.0f;
    values[1] = 1.0f;
    values[2] = 1.0f;
    values[3] = paintOpacity;
    vgSetParameterfv(opacityPaint, VG_PAINT_COLOR, 4, values);

#if !defined(QVG_NO_MODIFY_PATH)
    // Create a dummy path for rectangle drawing, which we can
    // modify later with vgModifyPathCoords().  This should be
    // faster than constantly creating and destroying paths.
    rectPath = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                            VG_PATH_DATATYPE_F,
                            1.0f, // scale
                            0.0f, // bias
                            5,    // segmentCapacityHint
                            8,    // coordCapacityHint
                            VG_PATH_CAPABILITY_ALL);
    static VGubyte const segments[5] = {
        VG_MOVE_TO_ABS,
        VG_LINE_TO_ABS,
        VG_LINE_TO_ABS,
        VG_LINE_TO_ABS,
        VG_CLOSE_PATH
    };
    VGfloat coords[8];
    coords[0] = 0.0f;
    coords[1] = 0.0f;
    coords[2] = 100.0f;
    coords[3] = coords[1];
    coords[4] = coords[2];
    coords[5] = 100.0f;
    coords[6] = coords[0];
    coords[7] = coords[5];
    vgAppendPathData(rectPath, 5, segments, coords);

    // Create a dummy line drawing path as well.
    linePath = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                            VG_PATH_DATATYPE_F,
                            1.0f, // scale
                            0.0f, // bias
                            2,    // segmentCapacityHint
                            4,    // coordCapacityHint
                            VG_PATH_CAPABILITY_ALL);
    vgAppendPathData(linePath, 2, segments, coords);
#endif

    const char *extensions = reinterpret_cast<const char *>(vgGetString(VG_EXTENSIONS));
    if (extensions)
        hasAdvancedBlending = strstr(extensions, "VG_KHR_advanced_blending") != 0;
}

void QVGPaintEnginePrivate::destroy()
{
    if (penPaint)
        vgDestroyPaint(penPaint);
    if (brushPaint)
        vgDestroyPaint(brushPaint);
    if (opacityPaint)
        vgDestroyPaint(opacityPaint);

#if !defined(QVG_NO_MODIFY_PATH)
    if (rectPath)
        vgDestroyPath(rectPath);
    if (linePath)
        vgDestroyPath(linePath);
    if (roundRectPath)
        vgDestroyPath(roundRectPath);
#endif

#if !defined(QVG_NO_DRAW_GLYPHS)
    QVGFontCache::Iterator it;
    for (it = fontCache.begin(); it != fontCache.end(); ++it)
        delete it.value();
    fontCache.clear();
    delete fontEngineCleaner;
#endif
}

// Set a specific VG transformation matrix in the current VG context.
void QVGPaintEnginePrivate::setTransform
        (VGMatrixMode mode, const QTransform& transform)
{
    VGfloat mat[9];
    if (mode != matrixMode) {
        vgSeti(VG_MATRIX_MODE, mode);
        matrixMode = mode;
    }
    mat[0] = transform.m11();
    mat[1] = transform.m12();
    mat[2] = transform.m13();
    mat[3] = transform.m21();
    mat[4] = transform.m22();
    mat[5] = transform.m23();
    mat[6] = transform.m31();
    mat[7] = transform.m32();
    mat[8] = transform.m33();
    vgLoadMatrix(mat);
}

Q_GUI_EXPORT bool qt_scaleForTransform(const QTransform &transform, qreal *scale);

void QVGPaintEnginePrivate::updateTransform(QPaintDevice *pdev)
{
    VGfloat devh = pdev->height();

    // Construct the VG transform by combining the Qt transform with
    // the following viewport transformation:
    //        | 1  0  0   |
    //        | 0 -1 devh |
    //        | 0  0  1   |
    // The full VG transform is effectively:
    //      1. Apply the user's transformation matrix.
    //      2. Flip the co-ordinate system upside down.
    QTransform viewport(1.0f, 0.0f, 0.0f,
                        0.0f, -1.0f, 0.0f,
                        0.0f, devh, 1.0f);

    // Compute the path transform and determine if it is projective.
    pathTransform = transform * viewport;
    bool projective = (pathTransform.m13() != 0.0f ||
                       pathTransform.m23() != 0.0f ||
                       pathTransform.m33() != 1.0f);
    if (projective) {
        // The engine cannot do projective path transforms for us,
        // so we will have to convert the co-ordinates ourselves.
        // Change the matrix to just the viewport transformation.
        pathTransform = viewport;
        simpleTransform = false;
    } else {
        simpleTransform = true;
    }
    pathTransformSet = false;

    // The image transform is always the full transformation,
    imageTransform = transform * viewport;

    // Calculate the scaling factor to use for turning cosmetic pens
    // into ordinary non-cosmetic pens.
    qt_scaleForTransform(transform, &penScale);
}

VGPath QVGPaintEnginePrivate::vectorPathToVGPath(const QVectorPath& path)
{
    int count = path.elementCount();
    const qreal *points = path.points();
    const QPainterPath::ElementType *elements = path.elements();

    VGPath vgpath = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                                 VG_PATH_DATATYPE_F,
                                 1.0f,        // scale
                                 0.0f,        // bias
                                 count + 1,   // segmentCapacityHint
                                 count * 2,   // coordCapacityHint
                                 VG_PATH_CAPABILITY_ALL);

    // Size is sufficient segments for drawRoundedRect() paths.
    QVarLengthArray<VGubyte, 20> segments;

    if (sizeof(qreal) == sizeof(VGfloat) && elements && simpleTransform) {
        // If Qt was compiled with qreal the same size as VGfloat,
        // then convert the segment types and use the incoming
        // points array directly.
        for (int i = 0; i < count; ++i) {
            switch (elements[i]) {

            case QPainterPath::MoveToElement:
                segments.append(VG_MOVE_TO_ABS); break;

            case QPainterPath::LineToElement:
                segments.append(VG_LINE_TO_ABS); break;

            case QPainterPath::CurveToElement:
                segments.append(VG_CUBIC_TO_ABS); break;

            case QPainterPath::CurveToDataElement: break;

            }
        }
        if (path.hasImplicitClose())
            segments.append(VG_CLOSE_PATH);

        vgAppendPathData(vgpath, segments.count(), segments.constData(),
                         reinterpret_cast<const VGfloat *>(points));

        return vgpath;
    }

    // Sizes chosen so that drawRoundedRect() paths fit in these arrays.
    QVarLengthArray<VGfloat, 48> coords;

    int curvePos = 0;
    QPointF temp;

    if (elements && simpleTransform) {
        // Convert the members of the element array.
        for (int i = 0; i < count; ++i) {
            switch (elements[i]) {

            case QPainterPath::MoveToElement:
            {
                coords.append(points[0]);
                coords.append(points[1]);
                segments.append(VG_MOVE_TO_ABS);
            }
            break;

            case QPainterPath::LineToElement:
            {
                coords.append(points[0]);
                coords.append(points[1]);
                segments.append(VG_LINE_TO_ABS);
            }
            break;

            case QPainterPath::CurveToElement:
            {
                coords.append(points[0]);
                coords.append(points[1]);
                curvePos = 2;
            }
            break;

            case QPainterPath::CurveToDataElement:
            {
                coords.append(points[0]);
                coords.append(points[1]);
                curvePos += 2;
                if (curvePos == 6) {
                    curvePos = 0;
                    segments.append(VG_CUBIC_TO_ABS);
                }
            }
            break;

            }
            points += 2;
        }
    } else if (elements && !simpleTransform) {
        // Convert the members of the element array after applying the
        // current transform to the path locally.
        for (int i = 0; i < count; ++i) {
            switch (elements[i]) {

            case QPainterPath::MoveToElement:
            {
                temp = transform.map(QPointF(points[0], points[1]));
                coords.append(temp.x());
                coords.append(temp.y());
                segments.append(VG_MOVE_TO_ABS);
            }
            break;

            case QPainterPath::LineToElement:
            {
                temp = transform.map(QPointF(points[0], points[1]));
                coords.append(temp.x());
                coords.append(temp.y());
                segments.append(VG_LINE_TO_ABS);
            }
            break;

            case QPainterPath::CurveToElement:
            {
                temp = transform.map(QPointF(points[0], points[1]));
                coords.append(temp.x());
                coords.append(temp.y());
                curvePos = 2;
            }
            break;

            case QPainterPath::CurveToDataElement:
            {
                temp = transform.map(QPointF(points[0], points[1]));
                coords.append(temp.x());
                coords.append(temp.y());
                curvePos += 2;
                if (curvePos == 6) {
                    curvePos = 0;
                    segments.append(VG_CUBIC_TO_ABS);
                }
            }
            break;

            }
            points += 2;
        }
    } else if (count > 0 && simpleTransform) {
        // If there is no element array, then the path is assumed
        // to be a MoveTo followed by several LineTo's.
        coords.append(points[0]);
        coords.append(points[1]);
        segments.append(VG_MOVE_TO_ABS);
        while (count > 1) {
            points += 2;
            coords.append(points[0]);
            coords.append(points[1]);
            segments.append(VG_LINE_TO_ABS);
            --count;
        }
    } else if (count > 0 && !simpleTransform) {
        // Convert a simple path, and apply the transform locally.
        temp = transform.map(QPointF(points[0], points[1]));
        coords.append(temp.x());
        coords.append(temp.y());
        segments.append(VG_MOVE_TO_ABS);
        while (count > 1) {
            points += 2;
            temp = transform.map(QPointF(points[0], points[1]));
            coords.append(temp.x());
            coords.append(temp.y());
            segments.append(VG_LINE_TO_ABS);
            --count;
        }
    }

    // Close the path if specified.
    if (path.hasImplicitClose())
        segments.append(VG_CLOSE_PATH);

    vgAppendPathData(vgpath, segments.count(),
                     segments.constData(), coords.constData());

    return vgpath;
}

VGPath QVGPaintEnginePrivate::painterPathToVGPath(const QPainterPath& path)
{
    int count = path.elementCount();

    VGPath vgpath = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                                 VG_PATH_DATATYPE_F,
                                 1.0f,        // scale
                                 0.0f,        // bias
                                 count + 1,   // segmentCapacityHint
                                 count * 2,   // coordCapacityHint
                                 VG_PATH_CAPABILITY_ALL);

    if (count == 0)
        return vgpath;

    const QPainterPath::Element *elements = &(path.elementAt(0));

    // Sizes chosen so that drawRoundedRect() paths fit in these arrays.
    QVarLengthArray<VGfloat, 48> coords;
    QVarLengthArray<VGubyte, 20> segments;

    int curvePos = 0;
    QPointF temp;

    // Keep track of the start and end of each sub-path.  QPainterPath
    // does not have an "implicit close" flag like QVectorPath does.
    // We therefore have to detect closed paths by looking for a LineTo
    // element that connects back to the initial MoveTo element.
    qreal startx = 0.0;
    qreal starty = 0.0;
    qreal endx = 0.0;
    qreal endy = 0.0;
    bool haveStart = false;
    bool haveEnd = false;

    if (simpleTransform) {
        // Convert the members of the element array.
        for (int i = 0; i < count; ++i) {
            switch (elements[i].type) {

            case QPainterPath::MoveToElement:
            {
                if (haveStart && haveEnd && startx == endx && starty == endy) {
                    // Implicitly close the previous sub-path.
                    segments.append(VG_CLOSE_PATH);
                }
                startx = elements[i].x;
                starty = elements[i].y;
                coords.append(startx);
                coords.append(starty);
                haveStart = true;
                haveEnd = false;
                segments.append(VG_MOVE_TO_ABS);
            }
            break;

            case QPainterPath::LineToElement:
            {
                endx = elements[i].x;
                endy = elements[i].y;
                coords.append(endx);
                coords.append(endy);
                haveEnd = true;
                segments.append(VG_LINE_TO_ABS);
            }
            break;

            case QPainterPath::CurveToElement:
            {
                coords.append(elements[i].x);
                coords.append(elements[i].y);
                haveEnd = false;
                curvePos = 2;
            }
            break;

            case QPainterPath::CurveToDataElement:
            {
                coords.append(elements[i].x);
                coords.append(elements[i].y);
                haveEnd = false;
                curvePos += 2;
                if (curvePos == 6) {
                    curvePos = 0;
                    segments.append(VG_CUBIC_TO_ABS);
                }
            }
            break;

            }
        }
    } else {
        // Convert the members of the element array after applying the
        // current transform to the path locally.
        for (int i = 0; i < count; ++i) {
            switch (elements[i].type) {

            case QPainterPath::MoveToElement:
            {
                if (haveStart && haveEnd && startx == endx && starty == endy) {
                    // Implicitly close the previous sub-path.
                    segments.append(VG_CLOSE_PATH);
                }
                temp = transform.map(QPointF(elements[i].x, elements[i].y));
                startx = temp.x();
                starty = temp.y();
                coords.append(startx);
                coords.append(starty);
                haveStart = true;
                haveEnd = false;
                segments.append(VG_MOVE_TO_ABS);
            }
            break;

            case QPainterPath::LineToElement:
            {
                temp = transform.map(QPointF(elements[i].x, elements[i].y));
                endx = temp.x();
                endy = temp.y();
                coords.append(endx);
                coords.append(endy);
                haveEnd = true;
                segments.append(VG_LINE_TO_ABS);
            }
            break;

            case QPainterPath::CurveToElement:
            {
                temp = transform.map(QPointF(elements[i].x, elements[i].y));
                coords.append(temp.x());
                coords.append(temp.y());
                haveEnd = false;
                curvePos = 2;
            }
            break;

            case QPainterPath::CurveToDataElement:
            {
                temp = transform.map(QPointF(elements[i].x, elements[i].y));
                coords.append(temp.x());
                coords.append(temp.y());
                haveEnd = false;
                curvePos += 2;
                if (curvePos == 6) {
                    curvePos = 0;
                    segments.append(VG_CUBIC_TO_ABS);
                }
            }
            break;

            }
        }
    }

    if (haveStart && haveEnd && startx == endx && starty == endy) {
        // Implicitly close the last sub-path.
        segments.append(VG_CLOSE_PATH);
    }

    vgAppendPathData(vgpath, segments.count(),
                     segments.constData(), coords.constData());

    return vgpath;
}

VGPath QVGPaintEnginePrivate::roundedRectPath(const QRectF &rect, qreal xRadius, qreal yRadius, Qt::SizeMode mode)
{
    static VGubyte roundedrect_types[] = {
        VG_MOVE_TO_ABS,
        VG_LINE_TO_ABS,
        VG_CUBIC_TO_ABS,
        VG_LINE_TO_ABS,
        VG_CUBIC_TO_ABS,
        VG_LINE_TO_ABS,
        VG_CUBIC_TO_ABS,
        VG_LINE_TO_ABS,
        VG_CUBIC_TO_ABS,
        VG_CLOSE_PATH
    };

    qreal x1 = rect.left();
    qreal x2 = rect.right();
    qreal y1 = rect.top();
    qreal y2 = rect.bottom();

    if (mode == Qt::RelativeSize) {
        xRadius = xRadius * rect.width() / 200.;
        yRadius = yRadius * rect.height() / 200.;
    }

    xRadius = qMin(xRadius, rect.width() / 2);
    yRadius = qMin(yRadius, rect.height() / 2);

    VGfloat pts[] = {
        x1 + xRadius, y1,                   // MoveTo
        x2 - xRadius, y1,                   // LineTo
        x2 - (1 - KAPPA) * xRadius, y1,     // CurveTo
        x2, y1 + (1 - KAPPA) * yRadius,
        x2, y1 + yRadius,
        x2, y2 - yRadius,                   // LineTo
        x2, y2 - (1 - KAPPA) * yRadius,     // CurveTo
        x2 - (1 - KAPPA) * xRadius, y2,
        x2 - xRadius, y2,
        x1 + xRadius, y2,                   // LineTo
        x1 + (1 - KAPPA) * xRadius, y2,     // CurveTo
        x1, y2 - (1 - KAPPA) * yRadius,
        x1, y2 - yRadius,
        x1, y1 + yRadius,                   // LineTo
        x1, y1 + (1 - KAPPA) * yRadius,     // CurveTo
        x1 + (1 - KAPPA) * xRadius, y1,
        x1 + xRadius, y1
    };

#if !defined(QVG_NO_MODIFY_PATH)
    VGPath vgpath = roundRectPath;
    if (!vgpath) {
        vgpath = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                              VG_PATH_DATATYPE_F,
                              1.0f,        // scale
                              0.0f,        // bias
                              10,          // segmentCapacityHint
                              17 * 2,      // coordCapacityHint
                              VG_PATH_CAPABILITY_ALL);
        vgAppendPathData(vgpath, 10, roundedrect_types, pts);
        roundRectPath = vgpath;
    } else {
        vgModifyPathCoords(vgpath, 0, 9, pts);
    }
#else
    VGPath vgpath = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                                 VG_PATH_DATATYPE_F,
                                 1.0f,        // scale
                                 0.0f,        // bias
                                 10,          // segmentCapacityHint
                                 17 * 2,      // coordCapacityHint
                                 VG_PATH_CAPABILITY_ALL);
    vgAppendPathData(vgpath, 10, roundedrect_types, pts);
#endif

    return vgpath;
}

Q_GUI_EXPORT QImage qt_imageForBrush(int style, bool invert);

static QImage colorizeBitmap(const QImage &image, const QColor &color)
{
    QImage sourceImage = image.convertToFormat(QImage::Format_MonoLSB);
    QImage dest = QImage(sourceImage.size(), QImage::Format_ARGB32_Premultiplied);

    QRgb fg = PREMUL(color.rgba());
    QRgb bg = 0;

    int height = sourceImage.height();
    int width = sourceImage.width();
    for (int y=0; y<height; ++y) {
        const uchar *source = sourceImage.constScanLine(y);
        QRgb *target = reinterpret_cast<QRgb *>(dest.scanLine(y));
        for (int x=0; x < width; ++x)
            target[x] = (source[x>>3] >> (x&7)) & 1 ? fg : bg;
    }
    return dest;
}

static VGImage toVGImage
    (const QImage & image, Qt::ImageConversionFlags flags = Qt::AutoColor)
{
    QImage img(image);

    VGImageFormat format;
    switch (img.format()) {
    case QImage::Format_Mono:
        img = image.convertToFormat(QImage::Format_MonoLSB, flags);
        img.invertPixels();
        format = VG_BW_1;
        break;
    case QImage::Format_MonoLSB:
        img.invertPixels();
        format = VG_BW_1;
        break;
    case QImage::Format_RGB32:
        format = VG_sXRGB_8888;
        break;
    case QImage::Format_ARGB32:
        format = VG_sARGB_8888;
        break;
    case QImage::Format_ARGB32_Premultiplied:
        format = VG_sARGB_8888_PRE;
        break;
    case QImage::Format_RGB16:
        format = VG_sRGB_565;
        break;
    default:
        // Convert everything else into ARGB32_Premultiplied.
        img = image.convertToFormat(QImage::Format_ARGB32_Premultiplied, flags);
        format = VG_sARGB_8888_PRE;
        break;
    }

    const uchar *pixels = img.constBits();

    VGImage vgImg = QVGImagePool::instance()->createPermanentImage
        (format, img.width(), img.height(), VG_IMAGE_QUALITY_FASTER);
    vgImageSubData
        (vgImg, pixels, img.bytesPerLine(), format, 0, 0,
         img.width(), img.height());

    return vgImg;
}

static VGImage toVGImageSubRect
    (const QImage & image, const QRect& sr,
     Qt::ImageConversionFlags flags = Qt::AutoColor)
{
    QImage img(image);

    VGImageFormat format;
    int bpp = 4;

    switch (img.format()) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        return VG_INVALID_HANDLE;
    case QImage::Format_RGB32:
        format = VG_sXRGB_8888;
        break;
    case QImage::Format_ARGB32:
        format = VG_sARGB_8888;
        break;
    case QImage::Format_ARGB32_Premultiplied:
        format = VG_sARGB_8888_PRE;
        break;
    case QImage::Format_RGB16:
        format = VG_sRGB_565;
        bpp = 2;
        break;
    default:
        // Convert everything else into ARGB32_Premultiplied.
        img = image.convertToFormat(QImage::Format_ARGB32_Premultiplied, flags);
        format = VG_sARGB_8888_PRE;
        break;
    }

    const uchar *pixels = img.constBits() + bpp * sr.x() +
                          img.bytesPerLine() * sr.y();

    VGImage vgImg = QVGImagePool::instance()->createPermanentImage
        (format, sr.width(), sr.height(), VG_IMAGE_QUALITY_FASTER);
    vgImageSubData
        (vgImg, pixels, img.bytesPerLine(), format, 0, 0,
         sr.width(), sr.height());

    return vgImg;
}

static VGImage toVGImageWithOpacity(const QImage & image, qreal opacity)
{
    QImage img(image.size(), QImage::Format_ARGB32_Premultiplied);
    img.fill(0);
    QPainter painter;
    painter.begin(&img);
    painter.setOpacity(opacity);
    painter.drawImage(0, 0, image);
    painter.end();

    const uchar *pixels = img.constBits();

    VGImage vgImg = QVGImagePool::instance()->createPermanentImage
        (VG_sARGB_8888_PRE, img.width(), img.height(), VG_IMAGE_QUALITY_FASTER);
    vgImageSubData
        (vgImg, pixels, img.bytesPerLine(), VG_sARGB_8888_PRE, 0, 0,
         img.width(), img.height());

    return vgImg;
}

static VGImage toVGImageWithOpacitySubRect
    (const QImage & image, qreal opacity, const QRect& sr)
{
    QImage img(sr.size(), QImage::Format_ARGB32_Premultiplied);
    img.fill(0);
    QPainter painter;
    painter.begin(&img);
    painter.setOpacity(opacity);
    painter.drawImage(QPoint(0, 0), image, sr);
    painter.end();

    const uchar *pixels = img.constBits();

    VGImage vgImg = QVGImagePool::instance()->createPermanentImage
        (VG_sARGB_8888_PRE, img.width(), img.height(), VG_IMAGE_QUALITY_FASTER);
    vgImageSubData
        (vgImg, pixels, img.bytesPerLine(), VG_sARGB_8888_PRE, 0, 0,
         img.width(), img.height());

    return vgImg;
}

VGPaintType QVGPaintEnginePrivate::setBrush
        (VGPaint paint, const QBrush& brush, VGMatrixMode mode,
         VGPaintType prevType)
{
    VGfloat values[5];
    setBrushTransform(brush, mode);

    // Reset the paint pattern on the brush, which will discard
    // the previous VGImage if one was set.
    if (prevType == VG_PAINT_TYPE_PATTERN || prevType == (VGPaintType)0)
        vgPaintPattern(paint, VG_INVALID_HANDLE);

    switch (brush.style()) {

    case Qt::SolidPattern: {
        // The brush is a solid color.
        QColor color(brush.color());
        values[0] = color.redF();
        values[1] = color.greenF();
        values[2] = color.blueF();
        values[3] = color.alphaF() * opacity;
        if (prevType != VG_PAINT_TYPE_COLOR)
            vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
        vgSetParameterfv(paint, VG_PAINT_COLOR, 4, values);
        return VG_PAINT_TYPE_COLOR;
    }

    case Qt::LinearGradientPattern: {
        // The brush is a linear gradient.
        Q_ASSERT(brush.gradient()->type() == QGradient::LinearGradient);
        const QLinearGradient *grad =
            static_cast<const QLinearGradient*>(brush.gradient());
        values[0] = grad->start().x();
        values[1] = grad->start().y();
        values[2] = grad->finalStop().x();
        values[3] = grad->finalStop().y();
        if (prevType != VG_PAINT_TYPE_LINEAR_GRADIENT)
            vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT);
        vgSetParameterfv(paint, VG_PAINT_LINEAR_GRADIENT, 4, values);
        setupColorRamp(grad, paint);
        return VG_PAINT_TYPE_LINEAR_GRADIENT;
    }

    case Qt::RadialGradientPattern: {
        // The brush is a radial gradient.
        Q_ASSERT(brush.gradient()->type() == QGradient::RadialGradient);
        const QRadialGradient *grad =
            static_cast<const QRadialGradient*>(brush.gradient());
        values[0] = grad->center().x();
        values[1] = grad->center().y();
        values[2] = grad->focalPoint().x();
        values[3] = grad->focalPoint().y();
        values[4] = grad->radius();
        if (prevType != VG_PAINT_TYPE_RADIAL_GRADIENT)
            vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_RADIAL_GRADIENT);
        vgSetParameterfv(paint, VG_PAINT_RADIAL_GRADIENT, 5, values);
        setupColorRamp(grad, paint);
        return VG_PAINT_TYPE_RADIAL_GRADIENT;
    }

    case Qt::TexturePattern: {
        // The brush is a texture specified by a QPixmap/QImage.
        QPixmapData *pd = brush.texture().pixmapData();
        if (!pd)
            break;  // null QPixmap
        VGImage vgImg;
        bool deref = false;
        if (pd->pixelType() == QPixmapData::BitmapType) {
            // Colorize bitmaps using the brush color and opacity.
            QColor color = brush.color();
            if (opacity != 1.0)
                color.setAlphaF(color.alphaF() * opacity);
            QImage image = colorizeBitmap(*(pd->buffer()), color);
            vgImg = toVGImage(image);
            deref = true;
        } else if (opacity == 1.0) {
            if (pd->classId() == QPixmapData::OpenVGClass) {
                QVGPixmapData *vgpd = static_cast<QVGPixmapData *>(pd);
                vgImg = vgpd->toVGImage();

                // We don't want the pool to reclaim this image
                // because we cannot predict when the paint object
                // will stop using it.  Replacing the image with
                // new data will make the paint object invalid.
                vgpd->detachImageFromPool();
            } else {
                vgImg = toVGImage(*(pd->buffer()));
                deref = true;
            }
        } else if (pd->classId() == QPixmapData::OpenVGClass) {
            QVGPixmapData *vgpd = static_cast<QVGPixmapData *>(pd);
            vgImg = vgpd->toVGImage(opacity);
            vgpd->detachImageFromPool();
        } else {
            vgImg = toVGImageWithOpacity(*(pd->buffer()), opacity);
            deref = true;
        }
        if (vgImg == VG_INVALID_HANDLE)
            break;
        if (prevType != VG_PAINT_TYPE_PATTERN)
            vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
        vgSetParameteri(paint, VG_PAINT_PATTERN_TILING_MODE, VG_TILE_REPEAT);
        vgPaintPattern(paint, vgImg);
        if (deref)
            vgDestroyImage(vgImg); // Will be valid until pattern is destroyed.
        return VG_PAINT_TYPE_PATTERN;
    }

    case Qt::ConicalGradientPattern: {
        // Convert conical gradients into the first stop color.
        qWarning() << "QVGPaintEnginePrivate::setBrush: conical gradients are not supported by OpenVG";
        Q_ASSERT(brush.gradient()->type() == QGradient::ConicalGradient);
        const QConicalGradient *grad =
            static_cast<const QConicalGradient*>(brush.gradient());
        const QGradientStops stops = grad->stops();
        QColor color;
        if (stops.size() > 0)
            color = stops[0].second;
        values[0] = color.redF();
        values[1] = color.greenF();
        values[2] = color.blueF();
        values[3] = color.alphaF() * opacity;
        if (prevType != VG_PAINT_TYPE_COLOR)
            vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
        vgSetParameterfv(paint, VG_PAINT_COLOR, 4, values);
        return VG_PAINT_TYPE_COLOR;
    }

    case Qt::Dense1Pattern:
    case Qt::Dense2Pattern:
    case Qt::Dense3Pattern:
    case Qt::Dense4Pattern:
    case Qt::Dense5Pattern:
    case Qt::Dense6Pattern:
    case Qt::Dense7Pattern:
    case Qt::HorPattern:
    case Qt::VerPattern:
    case Qt::CrossPattern:
    case Qt::BDiagPattern:
    case Qt::FDiagPattern:
    case Qt::DiagCrossPattern: {
        // The brush is a traditional dotted or cross-hatched pattern brush.
        QColor color = brush.color();
        if (opacity != 1.0)
            color.setAlphaF(color.alphaF() * opacity);
        QImage image = colorizeBitmap
            (qt_imageForBrush(brush.style(), true), color);
        VGImage vgImg = toVGImage(image);
        if (prevType != VG_PAINT_TYPE_PATTERN)
            vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_PATTERN);
        vgSetParameteri(paint, VG_PAINT_PATTERN_TILING_MODE, VG_TILE_REPEAT);
        vgPaintPattern(paint, vgImg);
        vgDestroyImage(vgImg); // Will stay valid until pattern is destroyed.
        return VG_PAINT_TYPE_PATTERN;
    }

    default: break;
    }
    return (VGPaintType)0;
}

void QVGPaintEnginePrivate::setPenParams(const QPen& pen)
{
    // Note: OpenVG does not support zero-width or cosmetic pens,
    // so we have to simulate cosmetic pens by reversing the scale.
    VGfloat width = pen.widthF();
    if (width <= 0.0f)
        width = 1.0f;
    if (pen.isCosmetic()) {
        if (penScale != 1.0 && penScale != 0.0)
            width /= penScale;
    }
    vgSetf(VG_STROKE_LINE_WIDTH, width);

    if (pen.capStyle() == Qt::FlatCap)
        vgSetf(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
    else if (pen.capStyle() == Qt::SquareCap)
        vgSetf(VG_STROKE_CAP_STYLE, VG_CAP_SQUARE);
    else
        vgSetf(VG_STROKE_CAP_STYLE, VG_CAP_ROUND);

    if (pen.joinStyle() == Qt::MiterJoin) {
        vgSetf(VG_STROKE_JOIN_STYLE, VG_JOIN_MITER);
        vgSetf(VG_STROKE_MITER_LIMIT, pen.miterLimit());
    } else if (pen.joinStyle() == Qt::BevelJoin) {
        vgSetf(VG_STROKE_JOIN_STYLE, VG_JOIN_BEVEL);
    } else {
        vgSetf(VG_STROKE_JOIN_STYLE, VG_JOIN_ROUND);
    }

    if (pen.style() == Qt::SolidLine) {
        vgSetfv(VG_STROKE_DASH_PATTERN, 0, NULL);
    } else {
        const QVector<qreal> dashPattern = pen.dashPattern();
        QVector<VGfloat> currentDashPattern(dashPattern.count());
        for (int i = 0; i < dashPattern.count(); ++i)
            currentDashPattern[i] = dashPattern[i] * width;
        vgSetfv(VG_STROKE_DASH_PATTERN, currentDashPattern.count(), currentDashPattern.data());
        vgSetf(VG_STROKE_DASH_PHASE, pen.dashOffset());
        vgSetf(VG_STROKE_DASH_PHASE_RESET, VG_FALSE);
    }
}

void QVGPaintEnginePrivate::setBrushTransform
        (const QBrush& brush, VGMatrixMode mode)
{
    // Compute the new brush transformation matrix.
    QTransform transform(brush.transform());
    if (brushOrigin.x() != 0.0f || brushOrigin.y() != 0.0f)
        transform.translate(brushOrigin.x(), brushOrigin.y());

    // Bail out if the matrix is the same as last time, to avoid
    // updating the VG context state unless absolutely necessary.
    // Most applications won't have a brush transformation set,
    // which will leave the VG setting at its default of identity.
    // Always change the transform if coming out of raw VG mode.
    if (mode == VG_MATRIX_FILL_PAINT_TO_USER) {
        if (!rawVG && transform == brushTransform)
            return;
        brushTransform = transform;
    } else {
        if (!rawVG && transform == penTransform)
            return;
        penTransform = transform;
    }

    // Set the brush transformation matrix.
    if (mode != matrixMode) {
        vgSeti(VG_MATRIX_MODE, mode);
        matrixMode = mode;
    }
    if (transform.isIdentity()) {
        vgLoadIdentity();
    } else {
        VGfloat mat[9];
        mat[0] = transform.m11();
        mat[1] = transform.m12();
        mat[2] = transform.m13();
        mat[3] = transform.m21();
        mat[4] = transform.m22();
        mat[5] = transform.m23();
        mat[6] = transform.m31();
        mat[7] = transform.m32();
        mat[8] = transform.m33();
        vgLoadMatrix(mat);
    }
}

void QVGPaintEnginePrivate::setupColorRamp(const QGradient *grad, VGPaint paint)
{
    QGradient::Spread spread = grad->spread();
    VGColorRampSpreadMode spreadMode;
    if (spread == QGradient::ReflectSpread)
        spreadMode = VG_COLOR_RAMP_SPREAD_REFLECT;
    else if (spread == QGradient::RepeatSpread)
        spreadMode = VG_COLOR_RAMP_SPREAD_REPEAT;
    else
        spreadMode = VG_COLOR_RAMP_SPREAD_PAD;

    const QGradientStops stops = grad->stops();
    int n = 5*stops.size();
    QVector<VGfloat> fill_stops(n);

    for (int i = 0; i < stops.size(); ++i ) {
        QColor col = stops[i].second;
        fill_stops[i*5] = stops[i].first;
        fill_stops[i*5 + 1] = col.redF();
        fill_stops[i*5 + 2] = col.greenF();
        fill_stops[i*5 + 3] = col.blueF();
        fill_stops[i*5 + 4] = col.alphaF() * opacity;
    }

    vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, spreadMode);
    vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_PREMULTIPLIED, VG_FALSE);
    vgSetParameterfv(paint, VG_PAINT_COLOR_RAMP_STOPS, n, fill_stops.data());
}

QVGPainterState::QVGPainterState(QVGPainterState& other)
    : QPainterState(other),
      isNew(true), clipRegion(other.clipRegion),
      savedDirty(0)
{
}

QVGPainterState::QVGPainterState()
    : isNew(true), savedDirty(0)
{
}

QVGPainterState::~QVGPainterState()
{
}

QVGPaintEngine::QVGPaintEngine()
    : QPaintEngineEx(*new QVGPaintEnginePrivate(this))
{
}

QVGPaintEngine::QVGPaintEngine(QVGPaintEnginePrivate &data)
    : QPaintEngineEx(data)
{
}

QVGPaintEngine::~QVGPaintEngine()
{
}

QPainterState *QVGPaintEngine::createState(QPainterState *orig) const
{
    if (!orig) {
        return new QVGPainterState();
    } else {
        Q_D(const QVGPaintEngine);
        QVGPaintEnginePrivate *d2 = const_cast<QVGPaintEnginePrivate*>(d);
        QVGPainterState *origState = static_cast<QVGPainterState *>(orig);
        origState->savedDirty = d2->dirty;
        d2->dirty = 0;
        return new QVGPainterState(*origState);
    }
}

void QVGPaintEnginePrivate::draw
    (VGPath path, const QPen& pen, const QBrush& brush, VGint rule)
{
    VGbitfield mode = 0;
    if (qpen_style(pen) != Qt::NoPen && qbrush_style(qpen_brush(pen)) != Qt::NoBrush) {
        ensurePen(pen);
        mode |= VG_STROKE_PATH;
    }
    if (brush.style() != Qt::NoBrush) {
        ensureBrush(brush);
        setFillRule(rule);
        mode |= VG_FILL_PATH;
    }
    if (mode != 0) {
        ensurePathTransform();
        vgDrawPath(path, mode);
    }
}

void QVGPaintEnginePrivate::stroke(VGPath path, const QPen& pen)
{
    if (pen.style() == Qt::NoPen)
        return;
    ensurePen(pen);
    ensurePathTransform();
    vgDrawPath(path, VG_STROKE_PATH);
}

void QVGPaintEnginePrivate::fill(VGPath path, const QBrush& brush, VGint rule)
{
    if (brush.style() == Qt::NoBrush)
        return;
    ensureBrush(brush);
    setFillRule(rule);
    QPen savedPen = currentPen;
    currentPen = Qt::NoPen;
    ensurePathTransform();
    currentPen = savedPen;
    vgDrawPath(path, VG_FILL_PATH);
}

bool QVGPaintEngine::begin(QPaintDevice *pdev)
{
    Q_UNUSED(pdev);
    Q_D(QVGPaintEngine);

    // Initialize the VG painting objects if we haven't done it yet.
    if (!d->penPaint)
        d->initObjects();

    // The initial clip region is the entire device area.
    QVGPainterState *s = state();
    s->clipRegion = defaultClipRegion();

    // Initialize the VG state for this paint operation.
    restoreState(QPaintEngine::AllDirty);
    d->dirty = 0;
    d->rawVG = false;
    return true;
}

bool QVGPaintEngine::end()
{
    return true;
}

void QVGPaintEngine::draw(const QVectorPath &path)
{
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    VGPath vgpath = d->vectorPathToVGPath(path);
    if (!path.hasWindingFill())
        d->draw(vgpath, s->pen, s->brush, VG_EVEN_ODD);
    else
        d->draw(vgpath, s->pen, s->brush, VG_NON_ZERO);
    vgDestroyPath(vgpath);
}

void QVGPaintEngine::fill(const QVectorPath &path, const QBrush &brush)
{
    Q_D(QVGPaintEngine);
    VGPath vgpath = d->vectorPathToVGPath(path);
    if (!path.hasWindingFill())
        d->fill(vgpath, brush, VG_EVEN_ODD);
    else
        d->fill(vgpath, brush, VG_NON_ZERO);
    vgDestroyPath(vgpath);
}

void QVGPaintEngine::stroke(const QVectorPath &path, const QPen &pen)
{
    Q_D(QVGPaintEngine);
    VGPath vgpath = d->vectorPathToVGPath(path);
    d->stroke(vgpath, pen);
    vgDestroyPath(vgpath);
}

// Determine if a co-ordinate transform is simple enough to allow
// rectangle-based clipping with vgMask().  Simple transforms most
// often result from origin translations.
static inline bool clipTransformIsSimple(const QTransform& transform)
{
    QTransform::TransformationType type = transform.type();
    if (type == QTransform::TxNone || type == QTransform::TxTranslate)
        return true;
    if (type == QTransform::TxRotate) {
        // Check for 0, 90, 180, and 270 degree rotations.
        // (0 might happen after 4 rotations of 90 degrees).
        qreal m11 = transform.m11();
        qreal m12 = transform.m12();
        qreal m21 = transform.m21();
        qreal m22 = transform.m22();
        if (m11 == 0.0f && m22 == 0.0f) {
            if (m12 == 1.0f && m21 == -1.0f)
                return true;    // 90 degrees.
            else if (m12 == -1.0f && m21 == 1.0f)
                return true;    // 270 degrees.
        } else if (m12 == 0.0f && m21 == 0.0f) {
            if (m11 == -1.0f && m22 == -1.0f)
                return true;    // 180 degrees.
            else if (m11 == 1.0f && m22 == 1.0f)
                return true;    // 0 degrees.
        }
    }
    return false;
}

#if defined(QVG_SCISSOR_CLIP)

void QVGPaintEngine::clip(const QVectorPath &path, Qt::ClipOperation op)
{
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();

    d->dirty |= QPaintEngine::DirtyClipRegion;

    if (op == Qt::NoClip) {
        s->clipRegion = defaultClipRegion();
        updateScissor();
        return;
    }

    // We aren't using masking, so handle simple QRectF's only.
    if (path.shape() == QVectorPath::RectangleHint &&
            path.elementCount() == 4 && clipTransformIsSimple(d->transform)) {
        // Clipping region that resulted from QPainter::setClipRect(QRectF).
        // Convert it into a QRect and apply.
        const qreal *points = path.points();
        QRectF rect(points[0], points[1], points[2] - points[0],
                    points[5] - points[1]);
        clip(rect.toRect(), op);
        return;
    }

    // Try converting the path into a QRegion that tightly follows
    // the outline of the path we want to clip with.
    QRegion region;
    if (!path.isEmpty())
        region = QRegion(path.convertToPainterPath().toFillPolygon(QTransform()).toPolygon());

    switch (op) {
        case Qt::NoClip:
        {
            region = defaultClipRegion();
        }
        break;

        case Qt::ReplaceClip:
        {
            region = d->transform.map(region);
        }
        break;

        case Qt::IntersectClip:
        {
            region = s->clipRegion.intersect(d->transform.map(region));
        }
        break;

        case Qt::UniteClip:
        {
            region = s->clipRegion.unite(d->transform.map(region));
        }
        break;
    }
    if (region.numRects() <= d->maxScissorRects) {
        // We haven't reached the maximum scissor count yet, so we can
        // still make use of this region.
        s->clipRegion = region;
        updateScissor();
        return;
    }

    // The best we can do is clip to the bounding rectangle
    // of all control points.
    clip(path.controlPointRect().toRect(), op);
}

void QVGPaintEngine::clip(const QRect &rect, Qt::ClipOperation op)
{
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();

    d->dirty |= QPaintEngine::DirtyClipRegion;

    switch (op) {
        case Qt::NoClip:
        {
            s->clipRegion = defaultClipRegion();
        }
        break;

        case Qt::ReplaceClip:
        {
            s->clipRegion = d->transform.map(QRegion(rect));
        }
        break;

        case Qt::IntersectClip:
        {
            s->clipRegion = s->clipRegion.intersect(d->transform.map(QRegion(rect)));
        }
        break;

        case Qt::UniteClip:
        {
            s->clipRegion = s->clipRegion.unite(d->transform.map(QRegion(rect)));
        }
        break;
    }

    updateScissor();
}

void QVGPaintEngine::clip(const QRegion &region, Qt::ClipOperation op)
{
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();

    d->dirty |= QPaintEngine::DirtyClipRegion;

    switch (op) {
        case Qt::NoClip:
        {
            s->clipRegion = defaultClipRegion();
        }
        break;

        case Qt::ReplaceClip:
        {
            s->clipRegion = d->transform.map(region);
        }
        break;

        case Qt::IntersectClip:
        {
            s->clipRegion = s->clipRegion.intersect(d->transform.map(region));
        }
        break;

        case Qt::UniteClip:
        {
            s->clipRegion = s->clipRegion.unite(d->transform.map(region));
        }
        break;
    }

    updateScissor();
}

void QVGPaintEngine::clip(const QPainterPath &path, Qt::ClipOperation op)
{
    QPaintEngineEx::clip(path, op);
}

#else // !QVG_SCISSOR_CLIP

void QVGPaintEngine::clip(const QVectorPath &path, Qt::ClipOperation op)
{
    Q_D(QVGPaintEngine);

    d->dirty |= QPaintEngine::DirtyClipRegion;

    if (op == Qt::NoClip) {
        d->maskValid = false;
        d->maskIsSet = true;
        d->scissorMask = false;
        d->maskRect = QRect();
        vgSeti(VG_MASKING, VG_FALSE);
        return;
    }

    // We don't have vgRenderToMask(), so handle simple QRectF's only.
    if (path.shape() == QVectorPath::RectangleHint &&
            path.elementCount() == 4 && clipTransformIsSimple(d->transform)) {
        // Clipping region that resulted from QPainter::setClipRect(QRectF).
        // Convert it into a QRect and apply.
        const qreal *points = path.points();
        QRectF rect(points[0], points[1], points[2] - points[0],
                    points[5] - points[1]);
        clip(rect.toRect(), op);
        return;
    }

#if !defined(QVG_NO_RENDER_TO_MASK)
    QPaintDevice *pdev = paintDevice();
    int width = pdev->width();
    int height = pdev->height();

    if (op == Qt::ReplaceClip) {
        vgMask(VG_INVALID_HANDLE, VG_CLEAR_MASK, 0, 0, width, height);
        d->maskRect = QRect();
    } else if (!d->maskValid) {
        d->ensureMask(this, width, height);
    }

    d->ensurePathTransform();
    VGPath vgpath = d->vectorPathToVGPath(path);
    switch (op) {
        case Qt::ReplaceClip:
        case Qt::UniteClip:
            vgRenderToMask(vgpath, VG_FILL_PATH, VG_UNION_MASK);
            break;

        case Qt::IntersectClip:
            vgRenderToMask(vgpath, VG_FILL_PATH, VG_INTERSECT_MASK);
            break;

        default: break;
    }
    vgDestroyPath(vgpath);

    vgSeti(VG_MASKING, VG_TRUE);
    d->maskValid = true;
    d->maskIsSet = false;
    d->scissorMask = false;
#endif
}

void QVGPaintEngine::clip(const QRect &rect, Qt::ClipOperation op)
{
    Q_D(QVGPaintEngine);

    d->dirty |= QPaintEngine::DirtyClipRegion;

    // If we have a non-simple transform, then use path-based clipping.
    if (op != Qt::NoClip && !clipTransformIsSimple(d->transform)) {
        QPaintEngineEx::clip(rect, op);
        return;
    }

    switch (op) {
        case Qt::NoClip:
        {
            d->maskValid = false;
            d->maskIsSet = true;
            d->scissorMask = false;
            d->maskRect = QRect();
            vgSeti(VG_MASKING, VG_FALSE);
        }
        break;

        case Qt::ReplaceClip:
        {
            QRect r = d->transform.mapRect(rect);
            if (isDefaultClipRect(r)) {
                // Replacing the clip with a full-window region is the
                // same as turning off clipping.
                if (d->maskValid)
                    vgSeti(VG_MASKING, VG_FALSE);
                d->maskValid = false;
                d->maskIsSet = true;
                d->scissorMask = false;
                d->maskRect = QRect();
            } else {
                // Special case: if the intersection of the system
                // clip and "r" is a single rectangle, then use the
                // scissor for clipping.  We try to avoid allocating a
                // QRegion copy on the heap for the test if we can.
                QRegion clip = d->systemClip; // Reference-counted, no alloc.
                QRect clipRect;
                if (clip.rectCount() == 1) {
                    clipRect = clip.boundingRect().intersected(r);
                } else if (clip.isEmpty()) {
                    clipRect = r;
                } else {
                    clip = clip.intersect(r);
                    if (clip.rectCount() != 1) {
                        d->maskValid = false;
                        d->maskIsSet = false;
                        d->scissorMask = false;
                        d->maskRect = QRect();
                        d->modifyMask(this, VG_FILL_MASK, r);
                        break;
                    }
                    clipRect = clip.boundingRect();
                }
                d->maskValid = false;
                d->maskIsSet = false;
                d->scissorMask = true;
                d->maskRect = clipRect;
                vgSeti(VG_MASKING, VG_FALSE);
                updateScissor();
            }
        }
        break;

        case Qt::IntersectClip:
        {
            QRect r = d->transform.mapRect(rect);
            if (!d->maskValid) {
                // Mask has not been used yet, so intersect with
                // the previous scissor-based region in maskRect.
                if (d->scissorMask)
                    r = r.intersect(d->maskRect);
                if (isDefaultClipRect(r)) {
                    // The clip is the full window, so turn off clipping.
                    d->maskIsSet = true;
                    d->maskRect = QRect();
                } else {
                    // Activate the scissor on a smaller maskRect.
                    d->maskIsSet = false;
                    d->maskRect = r;
                }
                d->scissorMask = true;
                updateScissor();
            } else if (d->maskIsSet && isDefaultClipRect(r)) {
                // Intersecting a full-window clip with a full-window
                // region is the same as turning off clipping.
                if (d->maskValid)
                    vgSeti(VG_MASKING, VG_FALSE);
                d->maskValid = false;
                d->maskIsSet = true;
                d->scissorMask = false;
                d->maskRect = QRect();
            } else {
                d->modifyMask(this, VG_INTERSECT_MASK, r);
            }
        }
        break;

        case Qt::UniteClip:
        {
            // If we already have a full-window clip, then uniting a
            // region with it will do nothing.  Otherwise union.
            if (!(d->maskIsSet))
                d->modifyMask(this, VG_UNION_MASK, d->transform.mapRect(rect));
        }
        break;
    }
}

void QVGPaintEngine::clip(const QRegion &region, Qt::ClipOperation op)
{
    Q_D(QVGPaintEngine);

    // Use the QRect case if the region consists of a single rectangle.
    if (region.rectCount() == 1) {
        clip(region.boundingRect(), op);
        return;
    }

    d->dirty |= QPaintEngine::DirtyClipRegion;

    // If we have a non-simple transform, then use path-based clipping.
    if (op != Qt::NoClip && !clipTransformIsSimple(d->transform)) {
        QPaintEngineEx::clip(region, op);
        return;
    }

    switch (op) {
        case Qt::NoClip:
        {
            d->maskValid = false;
            d->maskIsSet = true;
            d->scissorMask = false;
            d->maskRect = QRect();
            vgSeti(VG_MASKING, VG_FALSE);
        }
        break;

        case Qt::ReplaceClip:
        {
            QRegion r = d->transform.map(region);
            if (isDefaultClipRegion(r)) {
                // Replacing the clip with a full-window region is the
                // same as turning off clipping.
                if (d->maskValid)
                    vgSeti(VG_MASKING, VG_FALSE);
                d->maskValid = false;
                d->maskIsSet = true;
                d->scissorMask = false;
                d->maskRect = QRect();
            } else {
                // Special case: if the intersection of the system
                // clip and the region is a single rectangle, then
                // use the scissor for clipping.
                QRegion clip = d->systemClip;
                if (clip.isEmpty())
                    clip = r;
                else
                    clip = clip.intersect(r);
                if (clip.rectCount() == 1) {
                    d->maskValid = false;
                    d->maskIsSet = false;
                    d->scissorMask = true;
                    d->maskRect = clip.boundingRect();
                    vgSeti(VG_MASKING, VG_FALSE);
                    updateScissor();
                } else {
                    d->maskValid = false;
                    d->maskIsSet = false;
                    d->scissorMask = false;
                    d->maskRect = QRect();
                    d->modifyMask(this, VG_FILL_MASK, r);
                }
            }
        }
        break;

        case Qt::IntersectClip:
        {
            if (region.rectCount() != 1) {
                // If there is more than one rectangle, then intersecting
                // the rectangles one by one in modifyMask() will not give
                // the desired result.  So fall back to path-based clipping.
                QPaintEngineEx::clip(region, op);
                return;
            }
            QRegion r = d->transform.map(region);
            if (d->maskIsSet && isDefaultClipRegion(r)) {
                // Intersecting a full-window clip with a full-window
                // region is the same as turning off clipping.
                if (d->maskValid)
                    vgSeti(VG_MASKING, VG_FALSE);
                d->maskValid = false;
                d->maskIsSet = true;
                d->scissorMask = false;
                d->maskRect = QRect();
            } else {
                d->modifyMask(this, VG_INTERSECT_MASK, r);
            }
        }
        break;

        case Qt::UniteClip:
        {
            // If we already have a full-window clip, then uniting a
            // region with it will do nothing.  Otherwise union.
            if (!(d->maskIsSet))
                d->modifyMask(this, VG_UNION_MASK, d->transform.map(region));
        }
        break;
    }
}

#if !defined(QVG_NO_RENDER_TO_MASK)

// Copied from qpathclipper.cpp.
static bool qt_vg_pathToRect(const QPainterPath &path, QRectF *rect)
{
    if (path.elementCount() != 5)
        return false;

    const bool mightBeRect = path.elementAt(0).isMoveTo()
        && path.elementAt(1).isLineTo()
        && path.elementAt(2).isLineTo()
        && path.elementAt(3).isLineTo()
        && path.elementAt(4).isLineTo();

    if (!mightBeRect)
        return false;

    const qreal x1 = path.elementAt(0).x;
    const qreal y1 = path.elementAt(0).y;

    const qreal x2 = path.elementAt(1).x;
    const qreal y2 = path.elementAt(2).y;

    if (path.elementAt(1).y != y1)
        return false;

    if (path.elementAt(2).x != x2)
        return false;

    if (path.elementAt(3).x != x1 || path.elementAt(3).y != y2)
        return false;

    if (path.elementAt(4).x != x1 || path.elementAt(4).y != y1)
        return false;

    if (rect)
        *rect = QRectF(QPointF(x1, y1), QPointF(x2, y2));

    return true;
}

#endif

void QVGPaintEngine::clip(const QPainterPath &path, Qt::ClipOperation op)
{
#if !defined(QVG_NO_RENDER_TO_MASK)
    Q_D(QVGPaintEngine);

    // If the path is a simple rectangle, then use clip(QRect) instead.
    QRectF simpleRect;
    if (qt_vg_pathToRect(path, &simpleRect)) {
        clip(simpleRect.toRect(), op);
        return;
    }

    d->dirty |= QPaintEngine::DirtyClipRegion;

    if (op == Qt::NoClip) {
        d->maskValid = false;
        d->maskIsSet = true;
        d->scissorMask = false;
        d->maskRect = QRect();
        vgSeti(VG_MASKING, VG_FALSE);
        return;
    }

    QPaintDevice *pdev = paintDevice();
    int width = pdev->width();
    int height = pdev->height();

    if (op == Qt::ReplaceClip) {
        vgMask(VG_INVALID_HANDLE, VG_CLEAR_MASK, 0, 0, width, height);
        d->maskRect = QRect();
    } else if (!d->maskValid) {
        d->ensureMask(this, width, height);
    }

    d->ensurePathTransform();
    VGPath vgpath = d->painterPathToVGPath(path);
    switch (op) {
        case Qt::ReplaceClip:
        case Qt::UniteClip:
            vgRenderToMask(vgpath, VG_FILL_PATH, VG_UNION_MASK);
            break;

        case Qt::IntersectClip:
            vgRenderToMask(vgpath, VG_FILL_PATH, VG_INTERSECT_MASK);
            break;

        default: break;
    }
    vgDestroyPath(vgpath);

    vgSeti(VG_MASKING, VG_TRUE);
    d->maskValid = true;
    d->maskIsSet = false;
    d->scissorMask = false;
#else
    QPaintEngineEx::clip(path, op);
#endif
}

void QVGPaintEnginePrivate::ensureMask
        (QVGPaintEngine *engine, int width, int height)
{
    scissorMask = false;
    if (maskIsSet) {
        vgMask(VG_INVALID_HANDLE, VG_FILL_MASK, 0, 0, width, height);
        maskRect = QRect();
    } else {
        vgMask(VG_INVALID_HANDLE, VG_CLEAR_MASK, 0, 0, width, height);
        if (maskRect.isValid()) {
            vgMask(VG_INVALID_HANDLE, VG_FILL_MASK,
                   maskRect.x(), height - maskRect.y() - maskRect.height(),
                   maskRect.width(), maskRect.height());
            maskRect = QRect();
            engine->updateScissor();
        }
    }
}

void QVGPaintEnginePrivate::modifyMask
        (QVGPaintEngine *engine, VGMaskOperation op, const QRegion& region)
{
    QPaintDevice *pdev = engine->paintDevice();
    int width = pdev->width();
    int height = pdev->height();

    if (!maskValid)
        ensureMask(engine, width, height);

    QVector<QRect> rects = region.rects();
    for (int i = 0; i < rects.size(); ++i) {
        vgMask(VG_INVALID_HANDLE, op,
               rects[i].x(), height - rects[i].y() - rects[i].height(),
               rects[i].width(), rects[i].height());
    }

    vgSeti(VG_MASKING, VG_TRUE);
    maskValid = true;
    maskIsSet = false;
    scissorMask = false;
}

void QVGPaintEnginePrivate::modifyMask
        (QVGPaintEngine *engine, VGMaskOperation op, const QRect& rect)
{
    QPaintDevice *pdev = engine->paintDevice();
    int width = pdev->width();
    int height = pdev->height();

    if (!maskValid)
        ensureMask(engine, width, height);

    if (rect.isValid()) {
        vgMask(VG_INVALID_HANDLE, op,
               rect.x(), height - rect.y() - rect.height(),
               rect.width(), rect.height());
    }

    vgSeti(VG_MASKING, VG_TRUE);
    maskValid = true;
    maskIsSet = false;
    scissorMask = false;
}

#endif // !QVG_SCISSOR_CLIP

void QVGPaintEngine::updateScissor()
{
    Q_D(QVGPaintEngine);

    QRegion region = d->systemClip;

#if defined(QVG_SCISSOR_CLIP)
    // Using the scissor to do clipping, so combine the systemClip
    // with the current painting clipRegion.

    if (d->maskValid) {
        vgSeti(VG_MASKING, VG_FALSE);
        d->maskValid = false;
    }

    QVGPainterState *s = state();
    if (s->clipEnabled) {
        if (region.isEmpty())
            region = s->clipRegion;
        else
            region = region.intersect(s->clipRegion);
        if (isDefaultClipRegion(region)) {
            // The scissor region is the entire drawing surface,
            // so there is no point doing any scissoring.
            vgSeti(VG_SCISSORING, VG_FALSE);
            d->scissorActive = false;
            d->scissorDirty = false;
            return;
        }
    } else
#endif
    {
#if !defined(QVG_SCISSOR_CLIP)
        // Combine the system clip with the simple mask rectangle.
        if (d->scissorMask) {
            if (region.isEmpty())
                region = d->maskRect;
            else
                region = region.intersect(d->maskRect);
            if (isDefaultClipRegion(region)) {
                // The scissor region is the entire drawing surface,
                // so there is no point doing any scissoring.
                vgSeti(VG_SCISSORING, VG_FALSE);
                d->scissorActive = false;
                d->scissorDirty = false;
                return;
            }
        } else
#endif

        // Disable the scissor completely if the system clip is empty.
        if (region.isEmpty()) {
            vgSeti(VG_SCISSORING, VG_FALSE);
            d->scissorActive = false;
            d->scissorDirty = false;
            return;
        }
    }

    if (d->scissorActive && region == d->scissorRegion && !d->scissorDirty)
        return;

    QVector<QRect> rects = region.rects();
    int count = rects.count();
    if (count > d->maxScissorRects) {
#if !defined(QVG_SCISSOR_CLIP)
         count = d->maxScissorRects;
#else
        // Use masking
        int width = paintDevice()->width();
        int height = paintDevice()->height();
        vgMask(VG_INVALID_HANDLE, VG_CLEAR_MASK,
                   0, 0, width, height);
        for (int i = 0; i < rects.size(); ++i) {
            vgMask(VG_INVALID_HANDLE, VG_FILL_MASK,
                   rects[i].x(), height - rects[i].y() - rects[i].height(),
                   rects[i].width(), rects[i].height());
        }

        vgSeti(VG_SCISSORING, VG_FALSE);
        vgSeti(VG_MASKING, VG_TRUE);
        d->maskValid = true;
        d->maskIsSet = false;
        d->scissorMask = false;
        d->scissorActive = false;
        d->scissorDirty = false;
        d->scissorRegion = region;
        return;
#endif
    }

    QVarLengthArray<VGint> params(count * 4);
    int height = paintDevice()->height();
    for (int i = 0; i < count; ++i) {
        params[i * 4 + 0] = rects[i].x();
        params[i * 4 + 1] = height - rects[i].y() - rects[i].height();
        params[i * 4 + 2] = rects[i].width();
        params[i * 4 + 3] = rects[i].height();
    }

    vgSetiv(VG_SCISSOR_RECTS, count * 4, params.data());
    vgSeti(VG_SCISSORING, VG_TRUE);
    d->scissorDirty = false;
    d->scissorActive = true;
    d->scissorRegion = region;
}

QRegion QVGPaintEngine::defaultClipRegion()
{
    // The default clip region for a paint device is the whole drawing area.
    QPaintDevice *pdev = paintDevice();
    return QRegion(0, 0, pdev->width(), pdev->height());
}

bool QVGPaintEngine::isDefaultClipRegion(const QRegion& region)
{
    if (region.rectCount() != 1)
        return false;

    QPaintDevice *pdev = paintDevice();
    int width = pdev->width();
    int height = pdev->height();

    QRect rect = region.boundingRect();
    return (rect.x() == 0 && rect.y() == 0 &&
            rect.width() == width && rect.height() == height);
}

bool QVGPaintEngine::isDefaultClipRect(const QRect& rect)
{
    QPaintDevice *pdev = paintDevice();
    int width = pdev->width();
    int height = pdev->height();

    return (rect.x() == 0 && rect.y() == 0 &&
            rect.width() == width && rect.height() == height);
}

void QVGPaintEngine::clipEnabledChanged()
{
#if defined(QVG_SCISSOR_CLIP)
    vgSeti(VG_MASKING, VG_FALSE); // disable mask fallback
    updateScissor();
#else
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    d->dirty |= QPaintEngine::DirtyClipEnabled;
    if (s->clipEnabled && s->clipOperation != Qt::NoClip) {
        // Replay the entire clip stack to put the mask into the right state.
        d->maskValid = false;
        d->maskIsSet = true;
        d->scissorMask = false;
        d->maskRect = QRect();
        s->clipRegion = defaultClipRegion();
        d->replayClipOperations();
        d->transform = s->transform();
        d->updateTransform(paintDevice());
    } else {
        vgSeti(VG_MASKING, VG_FALSE);
        d->maskValid = false;
        d->maskIsSet = false;
        d->scissorMask = false;
        d->maskRect = QRect();
    }
#endif
}

void QVGPaintEngine::penChanged()
{
    Q_D(QVGPaintEngine);
    d->dirty |= QPaintEngine::DirtyPen;
}

void QVGPaintEngine::brushChanged()
{
    Q_D(QVGPaintEngine);
    d->dirty |= QPaintEngine::DirtyBrush;
}

void QVGPaintEngine::brushOriginChanged()
{
    Q_D(QVGPaintEngine);
    d->dirty |= QPaintEngine::DirtyBrushOrigin;
    d->brushOrigin = state()->brushOrigin;
    d->forcePenChange = true;
    d->forceBrushChange = true;
}

void QVGPaintEngine::opacityChanged()
{
    Q_D(QVGPaintEngine);
    d->dirty |= QPaintEngine::DirtyOpacity;
    d->opacity = state()->opacity;
    d->forcePenChange = true;
    d->forceBrushChange = true;
}

void QVGPaintEngine::compositionModeChanged()
{
    Q_D(QVGPaintEngine);
    d->dirty |= QPaintEngine::DirtyCompositionMode;

    VGint vgMode = VG_BLEND_SRC_OVER;

    switch (state()->composition_mode) {
    case QPainter::CompositionMode_SourceOver:
        vgMode = VG_BLEND_SRC_OVER;
        break;
    case QPainter::CompositionMode_DestinationOver:
        vgMode = VG_BLEND_DST_OVER;
        break;
    case QPainter::CompositionMode_Source:
        vgMode = VG_BLEND_SRC;
        break;
    case QPainter::CompositionMode_SourceIn:
        vgMode = VG_BLEND_SRC_IN;
        break;
    case QPainter::CompositionMode_DestinationIn:
        vgMode = VG_BLEND_DST_IN;
        break;
    case QPainter::CompositionMode_Plus:
        vgMode = VG_BLEND_ADDITIVE;
        break;
    case QPainter::CompositionMode_Multiply:
        vgMode = VG_BLEND_MULTIPLY;
        break;
    case QPainter::CompositionMode_Screen:
        vgMode = VG_BLEND_SCREEN;
        break;
    case QPainter::CompositionMode_Darken:
        vgMode = VG_BLEND_DARKEN;
        break;
    case QPainter::CompositionMode_Lighten:
        vgMode = VG_BLEND_LIGHTEN;
        break;
    default:
        if (d->hasAdvancedBlending) {
            switch (state()->composition_mode) {
            case QPainter::CompositionMode_Overlay:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_OVERLAY_KHR;
                break;
            case QPainter::CompositionMode_ColorDodge:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_COLORDODGE_KHR;
                break;
            case QPainter::CompositionMode_ColorBurn:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_COLORBURN_KHR;
                break;
            case QPainter::CompositionMode_HardLight:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_HARDLIGHT_KHR;
                break;
            case QPainter::CompositionMode_SoftLight:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_SOFTLIGHT_KHR;
                break;
            case QPainter::CompositionMode_Difference:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_DIFFERENCE_KHR;
                break;
            case QPainter::CompositionMode_Exclusion:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_EXCLUSION_KHR;
                break;
            case QPainter::CompositionMode_SourceOut:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_SRC_OUT_KHR;
                break;
            case QPainter::CompositionMode_DestinationOut:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_DST_OUT_KHR;
                break;
            case QPainter::CompositionMode_SourceAtop:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_SRC_ATOP_KHR;
                break;
            case QPainter::CompositionMode_DestinationAtop:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_DST_ATOP_KHR;
                break;
            case QPainter::CompositionMode_Xor:
                vgMode = QVGPaintEnginePrivate::QT_VG_BLEND_XOR_KHR;
                break;
            default: break; // Fall back to VG_BLEND_SRC_OVER.
            }
        }
        if (vgMode == VG_BLEND_SRC_OVER)
            qWarning() << "QVGPaintEngine::compositionModeChanged unsupported mode" << state()->composition_mode;
        break;
    }

    d->setBlendMode(VGBlendMode(vgMode));
}

void QVGPaintEngine::renderHintsChanged()
{
    Q_D(QVGPaintEngine);
    d->dirty |= QPaintEngine::DirtyHints;

    QPainter::RenderHints hints = state()->renderHints;

    VGRenderingQuality rq =
            (hints & QPainter::Antialiasing)
                ? VG_RENDERING_QUALITY_BETTER
                : VG_RENDERING_QUALITY_NONANTIALIASED;
    VGImageQuality iq =
            (hints & QPainter::SmoothPixmapTransform)
                ? VG_IMAGE_QUALITY_BETTER
                : VG_IMAGE_QUALITY_NONANTIALIASED;

    d->setRenderingQuality(rq);
    d->setImageQuality(iq);
}

void QVGPaintEngine::transformChanged()
{
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    d->dirty |= QPaintEngine::DirtyTransform;
    d->transform = s->transform();
    qreal oldPenScale = d->penScale;
    d->updateTransform(paintDevice());
    if (d->penScale != oldPenScale)
        d->forcePenChange = true;
}

bool QVGPaintEngine::clearRect(const QRectF &rect, const QColor &color)
{
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    if (!s->clipEnabled || s->clipOperation == Qt::NoClip) {
        QRect r = d->transform.mapRect(rect).toRect();
        int height = paintDevice()->height();
        if (d->clearColor != color || d->clearOpacity != s->opacity) {
            VGfloat values[4];
            values[0] = color.redF();
            values[1] = color.greenF();
            values[2] = color.blueF();
            values[3] = color.alphaF() * s->opacity;
            vgSetfv(VG_CLEAR_COLOR, 4, values);
            d->clearColor = color;
            d->clearOpacity = s->opacity;
        }
        vgClear(r.x(), height - r.y() - r.height(),
                r.width(), r.height());
        return true;
    }
    return false;
}

void QVGPaintEngine::fillRect(const QRectF &rect, const QBrush &brush)
{
    Q_D(QVGPaintEngine);

    if (brush.style() == Qt::NoBrush)
        return;

    // Check to see if we can use vgClear() for faster filling.
    if (brush.style() == Qt::SolidPattern && brush.isOpaque() &&
            clipTransformIsSimple(d->transform) && d->opacity == 1.0f &&
            clearRect(rect, brush.color())) {
        return;
    }

#if !defined(QVG_NO_MODIFY_PATH)
    VGfloat coords[8];
    if (d->simpleTransform) {
        coords[0] = rect.x();
        coords[1] = rect.y();
        coords[2] = rect.x() + rect.width();
        coords[3] = coords[1];
        coords[4] = coords[2];
        coords[5] = rect.y() + rect.height();
        coords[6] = coords[0];
        coords[7] = coords[5];
    } else {
        QPointF tl = d->transform.map(rect.topLeft());
        QPointF tr = d->transform.map(rect.topRight());
        QPointF bl = d->transform.map(rect.bottomLeft());
        QPointF br = d->transform.map(rect.bottomRight());
        coords[0] = tl.x();
        coords[1] = tl.y();
        coords[2] = tr.x();
        coords[3] = tr.y();
        coords[4] = br.x();
        coords[5] = br.y();
        coords[6] = bl.x();
        coords[7] = bl.y();
    }
    vgModifyPathCoords(d->rectPath, 0, 4, coords);
    d->fill(d->rectPath, brush);
#else
    QPaintEngineEx::fillRect(rect, brush);
#endif
}

void QVGPaintEngine::fillRect(const QRectF &rect, const QColor &color)
{
    Q_D(QVGPaintEngine);

    // Check to see if we can use vgClear() for faster filling.
    if (clipTransformIsSimple(d->transform) && d->opacity == 1.0f && color.alpha() == 255 &&
            clearRect(rect, color)) {
        return;
    }

#if !defined(QVG_NO_MODIFY_PATH)
    VGfloat coords[8];
    if (d->simpleTransform) {
        coords[0] = rect.x();
        coords[1] = rect.y();
        coords[2] = rect.x() + rect.width();
        coords[3] = coords[1];
        coords[4] = coords[2];
        coords[5] = rect.y() + rect.height();
        coords[6] = coords[0];
        coords[7] = coords[5];
    } else {
        QPointF tl = d->transform.map(rect.topLeft());
        QPointF tr = d->transform.map(rect.topRight());
        QPointF bl = d->transform.map(rect.bottomLeft());
        QPointF br = d->transform.map(rect.bottomRight());
        coords[0] = tl.x();
        coords[1] = tl.y();
        coords[2] = tr.x();
        coords[3] = tr.y();
        coords[4] = br.x();
        coords[5] = br.y();
        coords[6] = bl.x();
        coords[7] = bl.y();
    }
    vgModifyPathCoords(d->rectPath, 0, 4, coords);
    d->fill(d->rectPath, QBrush(color));
#else
    QPaintEngineEx::fillRect(rect, QBrush(color));
#endif
}

void QVGPaintEngine::drawRoundedRect(const QRectF &rect, qreal xrad, qreal yrad, Qt::SizeMode mode)
{
    Q_D(QVGPaintEngine);
    if (d->simpleTransform) {
        QVGPainterState *s = state();
        VGPath vgpath = d->roundedRectPath(rect, xrad, yrad, mode);
        d->draw(vgpath, s->pen, s->brush);
#if defined(QVG_NO_MODIFY_PATH)
        vgDestroyPath(vgpath);
#endif
    } else {
        QPaintEngineEx::drawRoundedRect(rect, xrad, yrad, mode);
    }
}

void QVGPaintEngine::drawRects(const QRect *rects, int rectCount)
{
#if !defined(QVG_NO_MODIFY_PATH)
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    for (int i = 0; i < rectCount; ++i, ++rects) {
        VGfloat coords[8];
        if (d->simpleTransform) {
            coords[0] = rects->x();
            coords[1] = rects->y();
            coords[2] = rects->x() + rects->width();
            coords[3] = coords[1];
            coords[4] = coords[2];
            coords[5] = rects->y() + rects->height();
            coords[6] = coords[0];
            coords[7] = coords[5];
        } else {
            QPointF tl = d->transform.map(QPointF(rects->x(), rects->y()));
            QPointF tr = d->transform.map(QPointF(rects->x() + rects->width(),
                                                  rects->y()));
            QPointF bl = d->transform.map(QPointF(rects->x(),
                                                  rects->y() + rects->height()));
            QPointF br = d->transform.map(QPointF(rects->x() + rects->width(),
                                                  rects->y() + rects->height()));
            coords[0] = tl.x();
            coords[1] = tl.y();
            coords[2] = tr.x();
            coords[3] = tr.y();
            coords[4] = br.x();
            coords[5] = br.y();
            coords[6] = bl.x();
            coords[7] = bl.y();
        }
        vgModifyPathCoords(d->rectPath, 0, 4, coords);
        d->draw(d->rectPath, s->pen, s->brush);
    }
#else
    QPaintEngineEx::drawRects(rects, rectCount);
#endif
}

void QVGPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
#if !defined(QVG_NO_MODIFY_PATH)
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    for (int i = 0; i < rectCount; ++i, ++rects) {
        VGfloat coords[8];
        if (d->simpleTransform) {
            coords[0] = rects->x();
            coords[1] = rects->y();
            coords[2] = rects->x() + rects->width();
            coords[3] = coords[1];
            coords[4] = coords[2];
            coords[5] = rects->y() + rects->height();
            coords[6] = coords[0];
            coords[7] = coords[5];
        } else {
            QPointF tl = d->transform.map(rects->topLeft());
            QPointF tr = d->transform.map(rects->topRight());
            QPointF bl = d->transform.map(rects->bottomLeft());
            QPointF br = d->transform.map(rects->bottomRight());
            coords[0] = tl.x();
            coords[1] = tl.y();
            coords[2] = tr.x();
            coords[3] = tr.y();
            coords[4] = br.x();
            coords[5] = br.y();
            coords[6] = bl.x();
            coords[7] = bl.y();
        }
        vgModifyPathCoords(d->rectPath, 0, 4, coords);
        d->draw(d->rectPath, s->pen, s->brush);
    }
#else
    QPaintEngineEx::drawRects(rects, rectCount);
#endif
}

void QVGPaintEngine::drawLines(const QLine *lines, int lineCount)
{
#if !defined(QVG_NO_MODIFY_PATH)
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    for (int i = 0; i < lineCount; ++i, ++lines) {
        VGfloat coords[4];
        if (d->simpleTransform) {
            coords[0] = lines->x1();
            coords[1] = lines->y1();
            coords[2] = lines->x2();
            coords[3] = lines->y2();
        } else {
            QPointF p1 = d->transform.map(QPointF(lines->x1(), lines->y1()));
            QPointF p2 = d->transform.map(QPointF(lines->x2(), lines->y2()));
            coords[0] = p1.x();
            coords[1] = p1.y();
            coords[2] = p2.x();
            coords[3] = p2.y();
        }
        vgModifyPathCoords(d->linePath, 0, 2, coords);
        d->stroke(d->linePath, s->pen);
    }
#else
    QPaintEngineEx::drawLines(lines, lineCount);
#endif
}

void QVGPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
#if !defined(QVG_NO_MODIFY_PATH)
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    for (int i = 0; i < lineCount; ++i, ++lines) {
        VGfloat coords[4];
        if (d->simpleTransform) {
            coords[0] = lines->x1();
            coords[1] = lines->y1();
            coords[2] = lines->x2();
            coords[3] = lines->y2();
        } else {
            QPointF p1 = d->transform.map(lines->p1());
            QPointF p2 = d->transform.map(lines->p2());
            coords[0] = p1.x();
            coords[1] = p1.y();
            coords[2] = p2.x();
            coords[3] = p2.y();
        }
        vgModifyPathCoords(d->linePath, 0, 2, coords);
        d->stroke(d->linePath, s->pen);
    }
#else
    QPaintEngineEx::drawLines(lines, lineCount);
#endif
}

void QVGPaintEngine::drawEllipse(const QRectF &r)
{
    // Based on the description of vguEllipse() in the OpenVG specification.
    // We don't use vguEllipse(), to avoid unnecessary library dependencies.
    Q_D(QVGPaintEngine);
    if (d->simpleTransform) {
        QVGPainterState *s = state();
        VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                                   VG_PATH_DATATYPE_F,
                                   1.0f, // scale
                                   0.0f, // bias
                                   4,    // segmentCapacityHint
                                   12,   // coordCapacityHint
                                   VG_PATH_CAPABILITY_ALL);
        static VGubyte segments[4] = {
            VG_MOVE_TO_ABS,
            VG_SCCWARC_TO_REL,
            VG_SCCWARC_TO_REL,
            VG_CLOSE_PATH
        };
        VGfloat coords[12];
        VGfloat halfwid = r.width() / 2;
        VGfloat halfht = r.height() / 2;
        coords[0]  = r.x() + r.width();
        coords[1]  = r.y() + halfht;
        coords[2]  = halfwid;
        coords[3]  = halfht;
        coords[4]  = 0.0f;
        coords[5]  = -r.width();
        coords[6]  = 0.0f;
        coords[7]  = halfwid;
        coords[8]  = halfht;
        coords[9]  = 0.0f;
        coords[10] = r.width();
        coords[11] = 0.0f;
        vgAppendPathData(path, 4, segments, coords);
        d->draw(path, s->pen, s->brush);
        vgDestroyPath(path);
    } else {
        // The projective transform version of an ellipse is difficult.
        // Generate a QVectorPath containing cubic curves and transform that.
        QPaintEngineEx::drawEllipse(r);
    }
}

void QVGPaintEngine::drawEllipse(const QRect &r)
{
    drawEllipse(QRectF(r));
}

void QVGPaintEngine::drawPath(const QPainterPath &path)
{
    // Shortcut past the QPainterPath -> QVectorPath conversion,
    // converting the QPainterPath directly into a VGPath.
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    VGPath vgpath = d->painterPathToVGPath(path);
    if (path.fillRule() == Qt::OddEvenFill)
        d->draw(vgpath, s->pen, s->brush, VG_EVEN_ODD);
    else
        d->draw(vgpath, s->pen, s->brush, VG_NON_ZERO);
    vgDestroyPath(vgpath);
}

void QVGPaintEngine::drawPoints(const QPointF *points, int pointCount)
{
#if !defined(QVG_NO_MODIFY_PATH)
    Q_D(QVGPaintEngine);

    // Set up a new pen if necessary.
    QPen pen = state()->pen;
    if (pen.style() == Qt::NoPen)
        return;
    if (pen.capStyle() == Qt::FlatCap)
        pen.setCapStyle(Qt::SquareCap);

    for (int i = 0; i < pointCount; ++i, ++points) {
        VGfloat coords[4];
        if (d->simpleTransform) {
            coords[0] = points->x();
            coords[1] = points->y();
            coords[2] = coords[0];
            coords[3] = coords[1];
        } else {
            QPointF p = d->transform.map(*points);
            coords[0] = p.x();
            coords[1] = p.y();
            coords[2] = coords[0];
            coords[3] = coords[1];
        }
        vgModifyPathCoords(d->linePath, 0, 2, coords);
        d->stroke(d->linePath, pen);
    }
#else
    QPaintEngineEx::drawPoints(points, pointCount);
#endif
}

void QVGPaintEngine::drawPoints(const QPoint *points, int pointCount)
{
#if !defined(QVG_NO_MODIFY_PATH)
    Q_D(QVGPaintEngine);

    // Set up a new pen if necessary.
    QPen pen = state()->pen;
    if (pen.style() == Qt::NoPen)
        return;
    if (pen.capStyle() == Qt::FlatCap)
        pen.setCapStyle(Qt::SquareCap);

    for (int i = 0; i < pointCount; ++i, ++points) {
        VGfloat coords[4];
        if (d->simpleTransform) {
            coords[0] = points->x();
            coords[1] = points->y();
            coords[2] = coords[0];
            coords[3] = coords[1];
        } else {
            QPointF p = d->transform.map(QPointF(*points));
            coords[0] = p.x();
            coords[1] = p.y();
            coords[2] = coords[0];
            coords[3] = coords[1];
        }
        vgModifyPathCoords(d->linePath, 0, 2, coords);
        d->stroke(d->linePath, pen);
    }
#else
    QPaintEngineEx::drawPoints(points, pointCount);
#endif
}

void QVGPaintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                               VG_PATH_DATATYPE_F,
                               1.0f,             // scale
                               0.0f,             // bias
                               pointCount + 1,   // segmentCapacityHint
                               pointCount * 2,   // coordCapacityHint
                               VG_PATH_CAPABILITY_ALL);
    QVarLengthArray<VGfloat, 16> coords;
    QVarLengthArray<VGubyte, 10> segments;
    for (int i = 0; i < pointCount; ++i, ++points) {
        if (d->simpleTransform) {
            coords.append(points->x());
            coords.append(points->y());
        } else {
            QPointF temp = d->transform.map(*points);
            coords.append(temp.x());
            coords.append(temp.y());
        }
        if (i == 0)
            segments.append(VG_MOVE_TO_ABS);
        else
            segments.append(VG_LINE_TO_ABS);
    }
    if (mode != QPaintEngine::PolylineMode)
        segments.append(VG_CLOSE_PATH);
    vgAppendPathData(path, segments.count(),
                     segments.constData(), coords.constData());
    switch (mode) {
        case QPaintEngine::WindingMode:
            d->draw(path, s->pen, s->brush, VG_NON_ZERO);
            break;

        case QPaintEngine::PolylineMode:
            d->stroke(path, s->pen);
            break;

        default:
            d->draw(path, s->pen, s->brush, VG_EVEN_ODD);
            break;
    }
    vgDestroyPath(path);
}

void QVGPaintEngine::drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QVGPaintEngine);
    QVGPainterState *s = state();
    VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                               VG_PATH_DATATYPE_F,
                               1.0f,             // scale
                               0.0f,             // bias
                               pointCount + 1,   // segmentCapacityHint
                               pointCount * 2,   // coordCapacityHint
                               VG_PATH_CAPABILITY_ALL);
    QVarLengthArray<VGfloat, 16> coords;
    QVarLengthArray<VGubyte, 10> segments;
    for (int i = 0; i < pointCount; ++i, ++points) {
        if (d->simpleTransform) {
            coords.append(points->x());
            coords.append(points->y());
        } else {
            QPointF temp = d->transform.map(QPointF(*points));
            coords.append(temp.x());
            coords.append(temp.y());
        }
        if (i == 0)
            segments.append(VG_MOVE_TO_ABS);
        else
            segments.append(VG_LINE_TO_ABS);
    }
    if (mode != QPaintEngine::PolylineMode)
        segments.append(VG_CLOSE_PATH);
    vgAppendPathData(path, segments.count(),
                     segments.constData(), coords.constData());
    switch (mode) {
        case QPaintEngine::WindingMode:
            d->draw(path, s->pen, s->brush, VG_NON_ZERO);
            break;

        case QPaintEngine::PolylineMode:
            d->stroke(path, s->pen);
            break;

        default:
            d->draw(path, s->pen, s->brush, VG_EVEN_ODD);
            break;
    }
    vgDestroyPath(path);
}

void QVGPaintEnginePrivate::setImageOptions()
{
    if (opacity != 1.0f && simpleTransform) {
        if (opacity != paintOpacity) {
            VGfloat values[4];
            values[0] = 1.0f;
            values[1] = 1.0f;
            values[2] = 1.0f;
            values[3] = opacity;
            vgSetParameterfv(opacityPaint, VG_PAINT_COLOR, 4, values);
            paintOpacity = opacity;
        }
        if (fillPaint != opacityPaint) {
            vgSetPaint(opacityPaint, VG_FILL_PATH);
            fillPaint = opacityPaint;
        }
        setImageMode(VG_DRAW_IMAGE_MULTIPLY);
    } else {
        setImageMode(VG_DRAW_IMAGE_NORMAL);
    }
}

void QVGPaintEnginePrivate::systemStateChanged()
{
    q->updateScissor();
}

static void drawVGImage(QVGPaintEnginePrivate *d,
                        const QRectF& r, VGImage vgImg,
                        const QSize& imageSize, const QRectF& sr)
{
    if (vgImg == VG_INVALID_HANDLE)
        return;
    VGImage child = VG_INVALID_HANDLE;

    if (sr.topLeft().isNull() && sr.size() == imageSize) {
        child = vgImg;
    } else {
        QRect src = sr.toRect();
#if !defined(QT_SHIVAVG)
        child = vgChildImage(vgImg, src.x(), src.y(), src.width(), src.height());
#else
        child = vgImg;  // XXX: ShivaVG doesn't have vgChildImage().
#endif
    }

    QTransform transform(d->imageTransform);
    VGfloat scaleX = sr.width() == 0.0f ? 0.0f : r.width() / sr.width();
    VGfloat scaleY = sr.height() == 0.0f ? 0.0f : r.height() / sr.height();
    transform.translate(r.x(), r.y());
    transform.scale(scaleX, scaleY);
    d->setTransform(VG_MATRIX_IMAGE_USER_TO_SURFACE, transform);

    d->setImageOptions();
    vgDrawImage(child);

    if(child != vgImg)
        vgDestroyImage(child);
}

static void drawVGImage(QVGPaintEnginePrivate *d,
                        const QPointF& pos, VGImage vgImg)
{
    if (vgImg == VG_INVALID_HANDLE)
        return;

    QTransform transform(d->imageTransform);
    transform.translate(pos.x(), pos.y());
    d->setTransform(VG_MATRIX_IMAGE_USER_TO_SURFACE, transform);

    d->setImageOptions();
    vgDrawImage(vgImg);
}

static void drawImageTiled(QVGPaintEnginePrivate *d,
                           const QRectF &r,
                           const QImage &image,
                           const QRectF &sr = QRectF())
{
    const int minTileSize = 16;
    int tileWidth = 512;
    int tileHeight = tileWidth;

    VGImageFormat tileFormat = qt_vg_image_to_vg_format(image.format());
    VGImage tile = VG_INVALID_HANDLE;
    QVGImagePool *pool = QVGImagePool::instance();
    while (tile == VG_INVALID_HANDLE && tileWidth >= minTileSize) {
        tile = pool->createPermanentImage(tileFormat, tileWidth, tileHeight,
            VG_IMAGE_QUALITY_FASTER);
        if (tile == VG_INVALID_HANDLE) {
            tileWidth /= 2;
            tileHeight /= 2;
        }
    }
    if (tile == VG_INVALID_HANDLE) {
        qWarning("drawImageTiled: Failed to create %dx%d tile, giving up", tileWidth, tileHeight);
        return;
    }

    VGfloat opacityMatrix[20] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, d->opacity,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    VGImage tileWithOpacity = VG_INVALID_HANDLE;
    if (d->opacity != 1) {
        tileWithOpacity = pool->createPermanentImage(VG_sARGB_8888_PRE,
            tileWidth, tileHeight, VG_IMAGE_QUALITY_FASTER);
        if (tileWithOpacity == VG_INVALID_HANDLE)
            qWarning("drawImageTiled: Failed to create extra tile, ignoring opacity");
    }

    QRect sourceRect = sr.toRect();
    if (sourceRect.isNull())
        sourceRect = QRect(0, 0, image.width(), image.height());

    VGfloat scaleX = r.width() / sourceRect.width();
    VGfloat scaleY = r.height() / sourceRect.height();

    d->setImageOptions();

    for (int y = sourceRect.y(); y < sourceRect.height(); y += tileHeight) {
        int h = qMin(tileHeight, sourceRect.height() - y);
        if (h < 1)
            break;
        for (int x = sourceRect.x(); x < sourceRect.width(); x += tileWidth) {
            int w = qMin(tileWidth, sourceRect.width() - x);
            if (w < 1)
                break;

            int bytesPerPixel = image.depth() / 8;
            const uchar *sptr = image.constBits() + x * bytesPerPixel + y * image.bytesPerLine();
            vgImageSubData(tile, sptr, image.bytesPerLine(), tileFormat, 0, 0, w, h);

            QTransform transform(d->imageTransform);
            transform.translate(r.x() + x, r.y() + y);
            transform.scale(scaleX, scaleY);
            d->setTransform(VG_MATRIX_IMAGE_USER_TO_SURFACE, transform);

            VGImage actualTile = tile;
            if (tileWithOpacity != VG_INVALID_HANDLE) {
                vgColorMatrix(tileWithOpacity, actualTile, opacityMatrix);
                if (w < tileWidth || h < tileHeight)
                    actualTile = vgChildImage(tileWithOpacity, 0, 0, w, h);
                else
                    actualTile = tileWithOpacity;
            } else if (w < tileWidth || h < tileHeight) {
                actualTile = vgChildImage(tile, 0, 0, w, h);
            }
            vgDrawImage(actualTile);

            if (actualTile != tile && actualTile != tileWithOpacity)
                vgDestroyImage(actualTile);
        }
    }

    vgDestroyImage(tile);
    if (tileWithOpacity != VG_INVALID_HANDLE)
        vgDestroyImage(tileWithOpacity);
}

// Used by qpixmapfilter_vg.cpp to draw filtered VGImage's.
void qt_vg_drawVGImage(QPainter *painter, const QPointF& pos, VGImage vgImg)
{
    QVGPaintEngine *engine =
        static_cast<QVGPaintEngine *>(painter->paintEngine());
    drawVGImage(engine->vgPrivate(), pos, vgImg);
}

// Used by qpixmapfilter_vg.cpp to draw filtered VGImage's as a stencil.
void qt_vg_drawVGImageStencil
    (QPainter *painter, const QPointF& pos, VGImage vgImg, const QBrush& brush)
{
    QVGPaintEngine *engine =
        static_cast<QVGPaintEngine *>(painter->paintEngine());

    QVGPaintEnginePrivate *d = engine->vgPrivate();

    QTransform transform(d->imageTransform);
    transform.translate(pos.x(), pos.y());
    d->setTransform(VG_MATRIX_IMAGE_USER_TO_SURFACE, transform);

    d->ensureBrush(brush);
    d->setImageMode(VG_DRAW_IMAGE_STENCIL);
    vgDrawImage(vgImg);
}

bool QVGPaintEngine::canVgWritePixels(const QImage &image) const
{
    Q_D(const QVGPaintEngine);

    // qt_vg_image_to_vg_format returns VG_sARGB_8888 as
    // fallback case if no matching VG format is found.
    // If given image format is not Format_ARGB32 and returned
    // format is VG_sARGB_8888, it means that no match was
    // found. In that case vgWritePixels cannot be used.
    // Also 1-bit formats cannot be used directly either.
    if ((image.format() != QImage::Format_ARGB32
           && qt_vg_image_to_vg_format(image.format()) == VG_sARGB_8888)
           || image.depth() == 1) {
        return false;
    }

    // vgWritePixels ignores masking, blending and xforms so we can only use it if
    // ALL of the following conditions are true:
    // - It is a simple translate, or a scale of -1 on the y-axis (inverted)
    // - The opacity is totally opaque
    // - The composition mode is "source" OR "source over" provided the image is opaque
    return ( d->imageTransform.type() <= QTransform::TxScale
            && d->imageTransform.m11() == 1.0 && qAbs(d->imageTransform.m22()) == 1.0)
            && d->opacity == 1.0f
            && (d->blendMode == VG_BLEND_SRC || (d->blendMode == VG_BLEND_SRC_OVER &&
                                                !image.hasAlphaChannel()));
}

void QVGPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    QPixmapData *pd = pm.pixmapData();
    if (!pd)
        return; // null QPixmap
    if (pd->classId() == QPixmapData::OpenVGClass) {
        Q_D(QVGPaintEngine);
        QVGPixmapData *vgpd = static_cast<QVGPixmapData *>(pd);
        if (!vgpd->isValid())
            return;
        if (d->simpleTransform)
            drawVGImage(d, r, vgpd->toVGImage(), vgpd->size(), sr);
        else
            drawVGImage(d, r, vgpd->toVGImage(d->opacity), vgpd->size(), sr);

        if(!vgpd->failedToAlloc)
            return;

        // try to reallocate next time if reasonable small pixmap
        QSize screenSize = QApplication::desktop()->screenGeometry().size();
        if (pm.size().width() <= screenSize.width()
            && pm.size().height() <= screenSize.height())
            vgpd->failedToAlloc = false;

        vgpd->source.beginDataAccess();
        drawImage(r, vgpd->source.imageRef(), sr, Qt::AutoColor);
        vgpd->source.endDataAccess(true);
    } else {
        drawImage(r, *(pd->buffer()), sr, Qt::AutoColor);
    }
}

void QVGPaintEngine::drawPixmap(const QPointF &pos, const QPixmap &pm)
{
    QPixmapData *pd = pm.pixmapData();
    if (!pd)
        return; // null QPixmap
    if (pd->classId() == QPixmapData::OpenVGClass) {
        Q_D(QVGPaintEngine);
        QVGPixmapData *vgpd = static_cast<QVGPixmapData *>(pd);
        if (!vgpd->isValid())
            return;
        if (d->simpleTransform)
            drawVGImage(d, pos, vgpd->toVGImage());
        else
            drawVGImage(d, pos, vgpd->toVGImage(d->opacity));

        if (!vgpd->failedToAlloc)
            return;

        // try to reallocate next time if reasonable small pixmap
        QSize screenSize = QApplication::desktop()->screenGeometry().size();
        if (pm.size().width() <= screenSize.width()
            && pm.size().height() <= screenSize.height())
            vgpd->failedToAlloc = false;

        vgpd->source.beginDataAccess();
        drawImage(pos, vgpd->source.imageRef());
        vgpd->source.endDataAccess(true);
    } else {
        drawImage(pos, *(pd->buffer()));
    }
}

void QVGPaintEngine::drawImage
        (const QRectF &r, const QImage &image, const QRectF &sr,
         Qt::ImageConversionFlags flags)
{
    Q_D(QVGPaintEngine);
    if (image.isNull())
        return;
    VGImage vgImg;
    if (d->simpleTransform || d->opacity == 1.0f)
        vgImg = toVGImageSubRect(image, sr.toRect(), flags);
    else
        vgImg = toVGImageWithOpacitySubRect(image, d->opacity, sr.toRect());
    if (vgImg != VG_INVALID_HANDLE) {
        if (r.size() == sr.size()) {
            drawVGImage(d, r.topLeft(), vgImg);
        } else {
            drawVGImage(d, r, vgImg, sr.size().toSize(),
                        QRectF(QPointF(0, 0), sr.size()));
        }
    } else {
        if (canVgWritePixels(image) && (r.size() == sr.size()) && !flags) {
            // Optimization for straight blits, no blending
            int x = sr.x();
            int y = sr.y();
            int bpp = image.depth() >> 3; // bytes
            int offset = 0;
            int bpl = image.bytesPerLine();
            if (d->imageTransform.m22() < 0) {
                // inverted
                offset = ((y + sr.height()) * bpl) - ((image.width() - x) * bpp);
                bpl = -bpl;
            } else {
                offset = (y * bpl) + (x * bpp);
            }
            const uchar *bits = image.constBits() + offset;

            QPointF mapped = d->imageTransform.map(r.topLeft());
            vgWritePixels(bits, bpl, qt_vg_image_to_vg_format(image.format()),
                        mapped.x(), mapped.y() - sr.height(), r.width(), r.height());
            return;
        } else {
            // Monochrome images need to use the vgChildImage() path.
            vgImg = toVGImage(image, flags);
            if (vgImg == VG_INVALID_HANDLE)
                drawImageTiled(d, r, image, sr);
            else
                drawVGImage(d, r, vgImg, image.size(), sr);
        }
    }
    vgDestroyImage(vgImg);
}

void QVGPaintEngine::drawImage(const QPointF &pos, const QImage &image)
{
    Q_D(QVGPaintEngine);
    if (image.isNull())
        return;
    VGImage vgImg;
    if (canVgWritePixels(image)) {
        // Optimization for straight blits, no blending
        bool inverted = (d->imageTransform.m22() < 0);
        const uchar *bits = inverted ? image.constBits() + image.byteCount() : image.constBits();
        int bpl = inverted ? -image.bytesPerLine() : image.bytesPerLine();

        QPointF mapped = d->imageTransform.map(pos);
        vgWritePixels(bits, bpl, qt_vg_image_to_vg_format(image.format()),
                             mapped.x(), mapped.y() - image.height(), image.width(), image.height());
        return;
    } else if (d->simpleTransform || d->opacity == 1.0f) {
        vgImg = toVGImage(image);
    } else {
        vgImg = toVGImageWithOpacity(image, d->opacity);
    }
    if (vgImg == VG_INVALID_HANDLE)
        drawImageTiled(d, QRectF(pos, image.size()), image);
    else
        drawVGImage(d, pos, vgImg);
    vgDestroyImage(vgImg);
}

void QVGPaintEngine::drawTiledPixmap
        (const QRectF &r, const QPixmap &pixmap, const QPointF &s)
{
    QBrush brush(state()->pen.color(), pixmap);
    QTransform xform = QTransform::fromTranslate(r.x() - s.x(), r.y() - s.y());
    brush.setTransform(xform);
    fillRect(r, brush);
}

// Best performance will be achieved with QDrawPixmaps::OpaqueHint
// (i.e. no opacity), no rotation or scaling, and drawing the full
// pixmap rather than parts of the pixmap.  Even having just one of
// these conditions will improve performance.
void QVGPaintEngine::drawPixmapFragments(const QPainter::PixmapFragment *drawingData, int dataCount,
                                         const QPixmap &pixmap, QFlags<QPainter::PixmapFragmentHint> hints)
{
#if !defined(QT_SHIVAVG)
    Q_D(QVGPaintEngine);

    // If the pixmap is not VG, or the transformation is projective,
    // then fall back to the default implementation.
    QPixmapData *pd = pixmap.pixmapData();
    if (!pd)
        return; // null QPixmap
    if (pd->classId() != QPixmapData::OpenVGClass || !d->simpleTransform) {
        QPaintEngineEx::drawPixmapFragments(drawingData, dataCount, pixmap, hints);
        return;
    }

    // Bail out if nothing to do.
    if (dataCount <= 0)
        return;

    // Bail out if we don't have a usable VGImage for the pixmap.
    QVGPixmapData *vgpd = static_cast<QVGPixmapData *>(pd);
    if (!vgpd->isValid())
        return;
    VGImage vgImg = vgpd->toVGImage();
    if (vgImg == VG_INVALID_HANDLE)
        return;

    // We cache the results of any vgChildImage() calls because the
    // same child is very likely to be used over and over in particle
    // systems.  However, performance is even better if vgChildImage()
    // isn't needed at all, so use full source rects where possible.
    QVarLengthArray<VGImage> cachedImages;
    QVarLengthArray<QRect> cachedSources;

    // Select the opacity paint object.
    if ((hints & QPainter::OpaqueHint) != 0 && d->opacity == 1.0f) {
        d->setImageMode(VG_DRAW_IMAGE_NORMAL);
    }  else {
        hints = 0;
        if (d->fillPaint != d->opacityPaint) {
            vgSetPaint(d->opacityPaint, VG_FILL_PATH);
            d->fillPaint = d->opacityPaint;
        }
    }

    for (int i = 0; i < dataCount; ++i) {
        QTransform transform(d->imageTransform);
        transform.translate(drawingData[i].x, drawingData[i].y);
        transform.rotate(drawingData[i].rotation);

        VGImage child;
        QSize imageSize = vgpd->size();
        QRectF sr(drawingData[i].sourceLeft, drawingData[i].sourceTop,
                  drawingData[i].width, drawingData[i].height);
        if (sr.topLeft().isNull() && sr.size() == imageSize) {
            child = vgImg;
        } else {
            // Look for a previous child with the same source rectangle
            // to avoid constantly calling vgChildImage()/vgDestroyImage().
            QRect src = sr.toRect();
            int j;
            for (j = 0; j < cachedSources.size(); ++j) {
                if (cachedSources[j] == src)
                    break;
            }
            if (j < cachedSources.size()) {
                child = cachedImages[j];
            } else {
                child = vgChildImage
                    (vgImg, src.x(), src.y(), src.width(), src.height());
                cachedImages.append(child);
                cachedSources.append(src);
            }
        }

        VGfloat scaleX = drawingData[i].scaleX;
        VGfloat scaleY = drawingData[i].scaleY;
        transform.translate(-0.5 * scaleX * sr.width(),
                            -0.5 * scaleY * sr.height());
        transform.scale(scaleX, scaleY);
        d->setTransform(VG_MATRIX_IMAGE_USER_TO_SURFACE, transform);

        if ((hints & QPainter::OpaqueHint) == 0) {
            qreal opacity = d->opacity * drawingData[i].opacity;
            if (opacity != 1.0f) {
                if (d->paintOpacity != opacity) {
                    VGfloat values[4];
                    values[0] = 1.0f;
                    values[1] = 1.0f;
                    values[2] = 1.0f;
                    values[3] = opacity;
                    d->paintOpacity = opacity;
                    vgSetParameterfv
                        (d->opacityPaint, VG_PAINT_COLOR, 4, values);
                }
                d->setImageMode(VG_DRAW_IMAGE_MULTIPLY);
            } else {
                d->setImageMode(VG_DRAW_IMAGE_NORMAL);
            }
        }

        vgDrawImage(child);
    }

    // Destroy the cached child sub-images.
    for (int i = 0; i < cachedImages.size(); ++i)
        vgDestroyImage(cachedImages[i]);
#else
    QPaintEngineEx::drawPixmapFragments(drawingData, dataCount, pixmap, hints);
#endif
}

QVGFontEngineCleaner::QVGFontEngineCleaner(QVGPaintEnginePrivate *d)
    : QObject(), d_ptr(d)
{
}

QVGFontEngineCleaner::~QVGFontEngineCleaner()
{
}

void QVGFontEngineCleaner::fontEngineDestroyed()
{
#if !defined(QVG_NO_DRAW_GLYPHS)
    QFontEngine *engine = static_cast<QFontEngine *>(sender());
    QVGFontCache::Iterator it = d_ptr->fontCache.find(engine);
    if (it != d_ptr->fontCache.end()) {
        delete it.value();
        d_ptr->fontCache.erase(it);
    }
#endif
}

#if !defined(QVG_NO_DRAW_GLYPHS)

QVGFontGlyphCache::QVGFontGlyphCache()
{
    font = vgCreateFont(0);
    scaleX = scaleY = 0.0;
    invertedGlyphs = false;
    memset(cachedGlyphsMask, 0, sizeof(cachedGlyphsMask));
}

QVGFontGlyphCache::~QVGFontGlyphCache()
{
    if (font != VG_INVALID_HANDLE)
        vgDestroyFont(font);
}

void QVGFontGlyphCache::setScaleFromText(const QFont &font, QFontEngine *fontEngine)
{
    QFontInfo fi(font);
    qreal pixelSize = fi.pixelSize();
    qreal emSquare = fontEngine->properties().emSquare.toReal();
    scaleX = scaleY = static_cast<VGfloat>(pixelSize / emSquare);
}

void QVGFontGlyphCache::cacheGlyphs(QVGPaintEnginePrivate *d,
                                    QFontEngine *fontEngine,
                                    const glyph_t *g, int count)
{
    VGfloat origin[2];
    VGfloat escapement[2];
    glyph_metrics_t metrics;
    // Some Qt font engines don't set yoff in getUnscaledGlyph().
    // Zero the metric structure so that everything has a default value.
    memset(&metrics, 0, sizeof(metrics));
    while (count-- > 0) {
        // Skip this glyph if we have already cached it before.
        glyph_t glyph = *g++;
        if (glyph < 256) {
            if ((cachedGlyphsMask[glyph / 32] & (1 << (glyph % 32))) != 0)
                continue;
            cachedGlyphsMask[glyph / 32] |= (1 << (glyph % 32));
        } else if (cachedGlyphs.contains(glyph)) {
            continue;
        } else {
            cachedGlyphs.insert(glyph);
        }
#if !defined(QVG_NO_IMAGE_GLYPHS)
        Q_UNUSED(d);
        QImage scaledImage = fontEngine->alphaMapForGlyph(glyph);
        VGImage vgImage = VG_INVALID_HANDLE;
        metrics = fontEngine->boundingBox(glyph);
        if (!scaledImage.isNull()) {  // Not a space character
            if (scaledImage.format() == QImage::Format_Indexed8) {
                vgImage = vgCreateImage(VG_A_8, scaledImage.width(), scaledImage.height(), VG_IMAGE_QUALITY_FASTER);
                vgImageSubData(vgImage, scaledImage.constBits(), scaledImage.bytesPerLine(), VG_A_8, 0, 0, scaledImage.width(), scaledImage.height());
            } else if (scaledImage.format() == QImage::Format_Mono) {
                QImage img = scaledImage.convertToFormat(QImage::Format_Indexed8);
                vgImage = vgCreateImage(VG_A_8, img.width(), img.height(), VG_IMAGE_QUALITY_FASTER);
                vgImageSubData(vgImage, img.constBits(), img.bytesPerLine(), VG_A_8, 0, 0, img.width(), img.height());
            } else {
                QImage img = scaledImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
                vgImage = vgCreateImage(VG_sARGB_8888_PRE, img.width(), img.height(), VG_IMAGE_QUALITY_FASTER);
                vgImageSubData(vgImage, img.constBits(), img.bytesPerLine(), VG_sARGB_8888_PRE, 0, 0, img.width(), img.height());
            }
        }
        origin[0] = -metrics.x.toReal();
        origin[1] = -metrics.y.toReal();
        escapement[0] = 0;
        escapement[1] = 0;
        vgSetGlyphToImage(font, glyph, vgImage, origin, escapement);
        vgDestroyImage(vgImage);    // Reduce reference count.
#else
        // Calculate the path for the glyph and cache it.
        QPainterPath path;
        fontEngine->getUnscaledGlyph(glyph, &path, &metrics);
        VGPath vgPath;
        if (!path.isEmpty()) {
            vgPath = d->painterPathToVGPath(path);
        } else {
            // Probably a "space" character with no visible outline.
            vgPath = VG_INVALID_HANDLE;
        }
        origin[0] = 0;
        origin[1] = 0;
        escapement[0] = 0;
        escapement[1] = 0;
        vgSetGlyphToPath(font, glyph, vgPath, VG_FALSE, origin, escapement);
        vgDestroyPath(vgPath);      // Reduce reference count.
#endif // !defined(QVG_NO_IMAGE_GLYPHS)
    }
}

#endif // !defined(QVG_NO_DRAW_GLYPHS)

void QVGPaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
#if !defined(QVG_NO_DRAW_GLYPHS)
    Q_D(QVGPaintEngine);
    const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);

    // If we are not using a simple transform, then fall back
    // to the default Qt path stroking algorithm.
    if (!d->simpleTransform) {
        QPaintEngineEx::drawTextItem(p, textItem);
        return;
    }

    // Get the glyphs and positions associated with the text item.
    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix;
    ti.fontEngine->getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);

    if (!drawCachedGlyphs(glyphs.size(), glyphs.data(), ti.font(), ti.fontEngine, p, positions.data()))
        QPaintEngineEx::drawTextItem(p, textItem);
#else
    // OpenGL 1.0 does not have support for VGFont and glyphs,
    // so fall back to the default Qt path stroking algorithm.
    QPaintEngineEx::drawTextItem(p, textItem);
#endif
}

void QVGPaintEngine::drawStaticTextItem(QStaticTextItem *textItem)
{
    drawCachedGlyphs(textItem->numGlyphs, textItem->glyphs, textItem->font, textItem->fontEngine(),
                     QPointF(0, 0), textItem->glyphPositions);
}

 bool QVGPaintEngine::drawCachedGlyphs(int numGlyphs, const glyph_t *glyphs, const QFont &font,
                                       QFontEngine *fontEngine, const QPointF &p,
                                       const QFixedPoint *positions)
 {
#if !defined(QVG_NO_DRAW_GLYPHS)
    Q_D(QVGPaintEngine);

    // Find the glyph cache for this font.
    QVGFontCache::ConstIterator it = d->fontCache.constFind(fontEngine);
    QVGFontGlyphCache *glyphCache;
    if (it != d->fontCache.constEnd()) {
        glyphCache = it.value();
    } else {
#ifdef Q_OS_SYMBIAN
        glyphCache = new QSymbianVGFontGlyphCache();
#else
        glyphCache = new QVGFontGlyphCache();
#endif
        if (glyphCache->font == VG_INVALID_HANDLE) {
            qWarning("QVGPaintEngine::drawTextItem: OpenVG fonts are not supported by the OpenVG engine");
            delete glyphCache;
            return false;
        }
        glyphCache->setScaleFromText(font, fontEngine);
        d->fontCache.insert(fontEngine, glyphCache);
        if (!d->fontEngineCleaner)
            d->fontEngineCleaner = new QVGFontEngineCleaner(d);
        QObject::connect(fontEngine, SIGNAL(destroyed()),
                         d->fontEngineCleaner, SLOT(fontEngineDestroyed()));
    }

    // Set the transformation to use for drawing the current glyphs.
    QTransform glyphTransform(d->pathTransform);
    if (d->transform.type() <= QTransform::TxTranslate) {
        // Prevent blurriness of unscaled, unrotated text by forcing integer coordinates.
        glyphTransform.translate(
                floor(p.x() + glyphTransform.dx() + aliasedCoordinateDelta) - glyphTransform.dx(),
                floor(p.y() - glyphTransform.dy() + aliasedCoordinateDelta) + glyphTransform.dy());
    } else {
        glyphTransform.translate(p.x(), p.y());
    }
#if defined(QVG_NO_IMAGE_GLYPHS)
    glyphTransform.scale(glyphCache->scaleX, glyphCache->scaleY);
#endif

    // Some glyph caches can create the VGImage upright
    if (glyphCache->invertedGlyphs)
        glyphTransform.scale(1, -1);

    d->setTransform(VG_MATRIX_GLYPH_USER_TO_SURFACE, glyphTransform);

    // Add the glyphs from the text item into the glyph cache.
    glyphCache->cacheGlyphs(d, fontEngine, glyphs, numGlyphs);

    // Create the array of adjustments between glyphs
    QVarLengthArray<VGfloat> adjustments_x(numGlyphs);
    QVarLengthArray<VGfloat> adjustments_y(numGlyphs);
    for (int i = 1; i < numGlyphs; ++i) {
        adjustments_x[i-1] = (positions[i].x - positions[i-1].x).round().toReal();
        adjustments_y[i-1] = (positions[i].y - positions[i-1].y).round().toReal();
    }

    // Set the glyph drawing origin.
    VGfloat origin[2];
    origin[0] = positions[0].x.round().toReal();
    origin[1] = positions[0].y.round().toReal();
    vgSetfv(VG_GLYPH_ORIGIN, 2, origin);

    // Fast anti-aliasing for paths, better for images.
#if !defined(QVG_NO_IMAGE_GLYPHS)
    d->setImageQuality(VG_IMAGE_QUALITY_BETTER);
    d->setImageMode(VG_DRAW_IMAGE_STENCIL);
#else
    d->setRenderingQuality(VG_RENDERING_QUALITY_FASTER);
#endif

    // Draw the glyphs.  We need to fill with the brush associated with
    // the Qt pen, not the Qt brush.
    d->ensureBrush(state()->pen.brush());
    vgDrawGlyphs(glyphCache->font, numGlyphs, (VGuint*)glyphs,
                 adjustments_x.data(), adjustments_y.data(), VG_FILL_PATH, VG_TRUE);
    return true;
#else
    Q_UNUSED(numGlyphs);
    Q_UNUSED(glyphs);
    Q_UNUSED(font);
    Q_UNUSED(fontEngine);
    Q_UNUSED(p);
    Q_UNUSED(positions);
    return false;
#endif
}

void QVGPaintEngine::setState(QPainterState *s)
{
    Q_D(QVGPaintEngine);
    QPaintEngineEx::setState(s);
    QVGPainterState *ps = static_cast<QVGPainterState *>(s);
    if (ps->isNew) {
        // Newly created state object.  The call to setState()
        // will either be followed by a call to begin(), or we are
        // setting the state as part of a save().
        ps->isNew = false;
    } else {
        // This state object was set as part of a restore().
        restoreState(d->dirty);
        d->dirty = ps->savedDirty;
    }
}

void QVGPaintEngine::beginNativePainting()
{
    Q_D(QVGPaintEngine);

    // About to enter raw VG mode: flush pending changes and make
    // sure that all matrices are set to the current transformation.
    QVGPainterState *s = this->state();
    d->ensurePen(s->pen);
    d->ensureBrush(s->brush);
    d->ensurePathTransform();
    d->setTransform(VG_MATRIX_IMAGE_USER_TO_SURFACE, d->imageTransform);
#if !defined(QVG_NO_DRAW_GLYPHS)
    d->setTransform(VG_MATRIX_GLYPH_USER_TO_SURFACE, d->pathTransform);
#endif
    d->rawVG = true;
}

void QVGPaintEngine::endNativePainting()
{
    Q_D(QVGPaintEngine);
    // Exiting raw VG mode: force all state values to be
    // explicitly set on the VG engine to undo any changes
    // that were made by the raw VG function calls.
    QPaintEngine::DirtyFlags dirty = d->dirty;
    d->clearModes();
    d->forcePenChange = true;
    d->forceBrushChange = true;
    d->penType = (VGPaintType)0;
    d->brushType = (VGPaintType)0;
    d->clearColor = QColor();
    d->fillPaint = d->brushPaint;
    d->scissorDirty = true;
    restoreState(QPaintEngine::AllDirty);
    d->dirty = dirty;
    d->rawVG = false;
    vgSetPaint(d->penPaint, VG_STROKE_PATH);
    vgSetPaint(d->brushPaint, VG_FILL_PATH);
}

QPixmapFilter *QVGPaintEngine::pixmapFilter(int type, const QPixmapFilter *prototype)
{
#if !defined(QT_SHIVAVG)
    Q_D(QVGPaintEngine);
    switch (type) {
        case QPixmapFilter::ConvolutionFilter:
            if (!d->convolutionFilter)
                d->convolutionFilter.reset(new QVGPixmapConvolutionFilter);
            return d->convolutionFilter.data();
        case QPixmapFilter::ColorizeFilter:
            if (!d->colorizeFilter)
                d->colorizeFilter.reset(new QVGPixmapColorizeFilter);
            return d->colorizeFilter.data();
        case QPixmapFilter::DropShadowFilter:
            if (!d->dropShadowFilter)
                d->dropShadowFilter.reset(new QVGPixmapDropShadowFilter);
            return d->dropShadowFilter.data();
        case QPixmapFilter::BlurFilter:
            if (!d->blurFilter)
                d->blurFilter.reset(new QVGPixmapBlurFilter);
            return d->blurFilter.data();
        default: break;
    }
#endif
    return QPaintEngineEx::pixmapFilter(type, prototype);
}

void QVGPaintEngine::restoreState(QPaintEngine::DirtyFlags dirty)
{
    Q_D(QVGPaintEngine);

    // Restore the pen, brush, and other settings.
    if ((dirty & QPaintEngine::DirtyBrushOrigin) != 0)
        brushOriginChanged();
    d->fillRule = 0;
    if ((dirty & QPaintEngine::DirtyOpacity) != 0)
        opacityChanged();
    if ((dirty & QPaintEngine::DirtyTransform) != 0)
        transformChanged();
    if ((dirty & QPaintEngine::DirtyCompositionMode) != 0)
        compositionModeChanged();
    if ((dirty & QPaintEngine::DirtyHints) != 0)
        renderHintsChanged();
    if ((dirty & (QPaintEngine::DirtyClipRegion |
                  QPaintEngine::DirtyClipPath |
                  QPaintEngine::DirtyClipEnabled)) != 0) {
        d->maskValid = false;
        d->maskIsSet = false;
        d->scissorMask = false;
        d->maskRect = QRect();
        d->scissorDirty = true;
        clipEnabledChanged();
    }

#if defined(QVG_SCISSOR_CLIP)
    if ((dirty & (QPaintEngine::DirtyClipRegion |
                  QPaintEngine::DirtyClipPath |
                  QPaintEngine::DirtyClipEnabled)) == 0) {
        updateScissor();
    }
#else
    updateScissor();
#endif
}

void QVGPaintEngine::fillRegion
    (const QRegion& region, const QColor& color, const QSize& surfaceSize)
{
    Q_D(QVGPaintEngine);
    if (d->clearColor != color || d->clearOpacity != 1.0f) {
        VGfloat values[4];
        values[0] = color.redF();
        values[1] = color.greenF();
        values[2] = color.blueF();
        values[3] = color.alphaF();
        vgSetfv(VG_CLEAR_COLOR, 4, values);
        d->clearColor = color;
        d->clearOpacity = 1.0f;
    }
    if (region.rectCount() == 1) {
        QRect r = region.boundingRect();
        vgClear(r.x(), surfaceSize.height() - r.y() - r.height(),
                r.width(), r.height());
    } else {
        const QVector<QRect> rects = region.rects();
        for (int i = 0; i < rects.size(); ++i) {
            QRect r = rects.at(i);
            vgClear(r.x(), surfaceSize.height() - r.y() - r.height(),
                    r.width(), r.height());
        }
    }
}

#if !defined(QVG_NO_SINGLE_CONTEXT) && !defined(QT_NO_EGL)

QVGCompositionHelper::QVGCompositionHelper()
{
    d = qt_vg_create_paint_engine()->vgPrivate();
}

QVGCompositionHelper::~QVGCompositionHelper()
{
}

void QVGCompositionHelper::startCompositing(const QSize& screenSize)
{
    this->screenSize = screenSize;
    clearScissor();
    d->setBlendMode(VG_BLEND_SRC_OVER);
}

void QVGCompositionHelper::endCompositing()
{
    clearScissor();
}

void QVGCompositionHelper::blitWindow
    (VGImage image, const QSize& imageSize,
     const QRect& rect, const QPoint& topLeft, int opacity)
{
    if (image == VG_INVALID_HANDLE)
        return;

    // Determine which sub rectangle of the window to draw.
    QRect sr = rect.translated(-topLeft);

    if (opacity >= 255) {
        // Fully opaque: use vgSetPixels() to directly copy the sub-region.
        int y = screenSize.height() - (rect.bottom() + 1);
        vgSetPixels(rect.x(), y, image, sr.x(),
                    imageSize.height() - (sr.y() + sr.height()),
                    sr.width(), sr.height());
    } else {
        // Extract the child image that we want to draw.
        VGImage child;
        if (sr.topLeft().isNull() && sr.size() == imageSize)
            child = image;
        else {
            child = vgChildImage
                (image, sr.x(), imageSize.height() - (sr.y() + sr.height()),
                 sr.width(), sr.height());
        }

        // Set the image transform.
        QTransform transform;
        int y = screenSize.height() - (rect.bottom() + 1);
        transform.translate(rect.x() - 0.5f, y - 0.5f);
        d->setTransform(VG_MATRIX_IMAGE_USER_TO_SURFACE, transform);

        // Enable opacity for image drawing if necessary.
        if (opacity != d->paintOpacity) {
            VGfloat values[4];
            values[0] = 1.0f;
            values[1] = 1.0f;
            values[2] = 1.0f;
            values[3] = ((VGfloat)opacity) / 255.0f;
            vgSetParameterfv(d->opacityPaint, VG_PAINT_COLOR, 4, values);
            d->paintOpacity = values[3];
        }
        if (d->fillPaint != d->opacityPaint) {
            vgSetPaint(d->opacityPaint, VG_FILL_PATH);
            d->fillPaint = d->opacityPaint;
        }
        d->setImageMode(VG_DRAW_IMAGE_MULTIPLY);

        // Draw the child image.
        vgDrawImage(child);

        // Destroy the child image.
        if(child != image)
            vgDestroyImage(child);
    }
}

static void fillBackgroundRect(const QRect& rect, QVGPaintEnginePrivate *d)
{
    VGfloat coords[8];
    coords[0] = rect.x();
    coords[1] = rect.y();
    coords[2] = rect.x() + rect.width();
    coords[3] = coords[1];
    coords[4] = coords[2];
    coords[5] = rect.y() + rect.height();
    coords[6] = coords[0];
    coords[7] = coords[5];
#if !defined(QVG_NO_MODIFY_PATH)
    vgModifyPathCoords(d->rectPath, 0, 4, coords);
    vgDrawPath(d->rectPath, VG_FILL_PATH);
#else
    Q_UNUSED(d);
    VGPath rectPath = vgCreatePath
            (VG_PATH_FORMAT_STANDARD,
             VG_PATH_DATATYPE_F,
             1.0f, // scale
             0.0f, // bias
             5,    // segmentCapacityHint
             8,    // coordCapacityHint
             VG_PATH_CAPABILITY_ALL);
    static VGubyte const segments[5] = {
        VG_MOVE_TO_ABS,
        VG_LINE_TO_ABS,
        VG_LINE_TO_ABS,
        VG_LINE_TO_ABS,
        VG_CLOSE_PATH
    };
    vgAppendPathData(rectPath, 5, segments, coords);
    vgDrawPath(rectPath, VG_FILL_PATH);
    vgDestroyPath(rectPath);
#endif
}

void QVGCompositionHelper::fillBackground
    (const QRegion& region, const QBrush& brush)
{
    if (brush.style() == Qt::SolidPattern) {
        // Use vgClear() to quickly fill the background.
        QColor color = brush.color();
        if (d->clearColor != color || d->clearOpacity != 1.0f) {
            VGfloat values[4];
            values[0] = color.redF();
            values[1] = color.greenF();
            values[2] = color.blueF();
            values[3] = color.alphaF();
            vgSetfv(VG_CLEAR_COLOR, 4, values);
            d->clearColor = color;
            d->clearOpacity = 1.0f;
        }
        if (region.rectCount() == 1) {
            QRect r = region.boundingRect();
            vgClear(r.x(), screenSize.height() - r.y() - r.height(),
                    r.width(), r.height());
        } else {
            const QVector<QRect> rects = region.rects();
            for (int i = 0; i < rects.size(); ++i) {
                QRect r = rects.at(i);
                vgClear(r.x(), screenSize.height() - r.y() - r.height(),
                        r.width(), r.height());
            }
        }

    } else {
        // Set the path transform to the default viewport transformation.
        VGfloat devh = screenSize.height();
        QTransform viewport(1.0f, 0.0f, 0.0f,
                            0.0f, -1.0f, 0.0f,
                            0.0f, devh, 1.0f);
        d->setTransform(VG_MATRIX_PATH_USER_TO_SURFACE, viewport);

        // Set the brush to use to fill the background.
        d->ensureBrush(brush);
        d->setFillRule(VG_EVEN_ODD);

        if (region.rectCount() == 1) {
            fillBackgroundRect(region.boundingRect(), d);
        } else {
            const QVector<QRect> rects = region.rects();
            for (int i = 0; i < rects.size(); ++i)
                fillBackgroundRect(rects.at(i), d);
        }

        // We will need to reset the path transform during the next paint.
        d->pathTransformSet = false;
    }
}

void QVGCompositionHelper::drawCursorPixmap
    (const QPixmap& pixmap, const QPoint& offset)
{
    VGImage vgImage = VG_INVALID_HANDLE;

    // Fetch the VGImage from the pixmap if possible.
    QPixmapData *pd = pixmap.pixmapData();
    if (!pd)
        return; // null QPixmap
    if (pd->classId() == QPixmapData::OpenVGClass) {
        QVGPixmapData *vgpd = static_cast<QVGPixmapData *>(pd);
        if (vgpd->isValid())
            vgImage = vgpd->toVGImage();
    }

    // Set the image transformation and modes.
    VGfloat devh = screenSize.height();
    QTransform transform(1.0f, 0.0f, 0.0f,
                         0.0f, -1.0f, 0.0f,
                         0.0f, devh, 1.0f);
    transform.translate(offset.x(), offset.y());
    d->setTransform(VG_MATRIX_IMAGE_USER_TO_SURFACE, transform);
    d->setImageMode(VG_DRAW_IMAGE_NORMAL);

    // Draw the VGImage.
    if (vgImage != VG_INVALID_HANDLE) {
        vgDrawImage(vgImage);
    } else {
        QImage img = pixmap.toImage().convertToFormat
            (QImage::Format_ARGB32_Premultiplied);

        vgImage = vgCreateImage
            (VG_sARGB_8888_PRE, img.width(), img.height(),
             VG_IMAGE_QUALITY_FASTER);
        if (vgImage == VG_INVALID_HANDLE)
            return;
        vgImageSubData
            (vgImage, img.constBits() + img.bytesPerLine() * (img.height() - 1),
             -(img.bytesPerLine()), VG_sARGB_8888_PRE, 0, 0,
             img.width(), img.height());

        vgDrawImage(vgImage);
        vgDestroyImage(vgImage);
    }
}

void QVGCompositionHelper::setScissor(const QRegion& region)
{
    QVector<QRect> rects = region.rects();
    int count = rects.count();
    if (count > d->maxScissorRects)
        count = d->maxScissorRects;
    QVarLengthArray<VGint> params(count * 4);
    int height = screenSize.height();
    for (int i = 0; i < count; ++i) {
        params[i * 4 + 0] = rects[i].x();
        params[i * 4 + 1] = height - rects[i].y() - rects[i].height();
        params[i * 4 + 2] = rects[i].width();
        params[i * 4 + 3] = rects[i].height();
    }

    vgSetiv(VG_SCISSOR_RECTS, count * 4, params.data());
    vgSeti(VG_SCISSORING, VG_TRUE);
    d->scissorDirty = false;
    d->scissorActive = true;
    d->scissorRegion = region;
}

void QVGCompositionHelper::clearScissor()
{
    if (d->scissorActive || d->scissorDirty) {
        vgSeti(VG_SCISSORING, VG_FALSE);
        d->scissorActive = false;
        d->scissorDirty = false;
    }
}

#endif // !QVG_NO_SINGLE_CONTEXT && !QT_NO_EGL

VGImageFormat qt_vg_image_to_vg_format(QImage::Format format)
{
    switch (format) {
        case QImage::Format_MonoLSB:
            return VG_BW_1;
        case QImage::Format_Indexed8:
            return VG_sL_8;
        case QImage::Format_ARGB32_Premultiplied:
            return VG_sARGB_8888_PRE;
        case QImage::Format_RGB32:
            return VG_sXRGB_8888;
        case QImage::Format_ARGB32:
            return VG_sARGB_8888;
        case QImage::Format_RGB16:
            return VG_sRGB_565;
        case QImage::Format_ARGB4444_Premultiplied:
            return VG_sARGB_4444;
        default:
            break;
    }
    return VG_sARGB_8888;   // XXX
}

QT_END_NAMESPACE

#include "qpaintengine_vg.moc"
