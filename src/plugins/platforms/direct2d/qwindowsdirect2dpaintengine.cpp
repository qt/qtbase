/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwindowsdirect2dpaintengine.h"
#include "qwindowsdirect2dplatformpixmap.h"
#include "qwindowsdirect2dpaintdevice.h"
#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dhelpers.h"
#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2ddevicecontext.h"

#include "qwindowsfontengine.h"
#include "qwindowsfontenginedirectwrite.h"
#include "qwindowsfontdatabase.h"
#include "qwindowsintegration.h"

#include <QtCore/QStack>
#include <QtGui/private/qpaintengine_p.h>
#include <QtGui/private/qtextengine_p.h>
#include <QtGui/private/qfontengine_p.h>

#include <dwrite_1.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

QT_BEGIN_NAMESPACE

enum {
    D2DDebugDrawInitialStateTag = -1,
    D2DDebugDrawEllipseTag = 1,
    D2DDebugDrawImageTag,
    D2DDebugDrawLinesTag,
    D2DDebugDrawPathTag,
    D2DDebugDrawPixmapTag,
    D2DDebugDrawPointsTag,
    D2DDebugDrawPolygonTag,
    D2DDebugDrawRectsTag,
    D2DDebugDrawTextItemTag,
    D2DDebugDrawTiledPixmap
};
#define D2D_TAG(tag) d->dc()->SetTags(tag, tag)

Q_GUI_EXPORT QImage qt_imageForBrush(int brushStyle, bool invert);

static inline ID2D1Factory1 *factory()
{
    return QWindowsDirect2DContext::instance()->d2dFactory();
}

static const qreal defaultOpacity = 1.0;
static const qreal defaultPenWidth = 1.0;

static ComPtr<ID2D1PathGeometry> painterPathToPathGeometry(const QPainterPath &path)
{
    ComPtr<ID2D1PathGeometry> geometry;
    ComPtr<ID2D1GeometrySink> sink;

    HRESULT hr = factory()->CreatePathGeometry(&geometry);
    if (FAILED(hr)) {
        qWarning("%s: Could not create path geometry: %#x", __FUNCTION__, hr);
        return NULL;
    }

    hr = geometry->Open(&sink);
    if (FAILED(hr)) {
        qWarning("%s: Could not create geometry sink: %#x", __FUNCTION__, hr);
        return NULL;
    }

    switch (path.fillRule()) {
    case Qt::WindingFill:
        sink->SetFillMode(D2D1_FILL_MODE_WINDING);
        break;
    case Qt::OddEvenFill:
        sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
        break;
    }

    bool inFigure = false;

    for (int i = 0; i < path.elementCount(); i++) {
        const QPainterPath::Element element = path.elementAt(i);

        switch (element.type) {
        case QPainterPath::MoveToElement:
            if (inFigure)
                sink->EndFigure(D2D1_FIGURE_END_OPEN);

            sink->BeginFigure(to_d2d_point_2f(element), D2D1_FIGURE_BEGIN_FILLED);
            inFigure = true;
            break;

        case QPainterPath::LineToElement:
            sink->AddLine(to_d2d_point_2f(element));
            break;

        case QPainterPath::CurveToElement:
        {
            const QPainterPath::Element data1 = path.elementAt(++i);
            const QPainterPath::Element data2 = path.elementAt(++i);

            Q_ASSERT(i < path.elementCount());

            Q_ASSERT(data1.type == QPainterPath::CurveToDataElement);
            Q_ASSERT(data2.type == QPainterPath::CurveToDataElement);

            D2D1_BEZIER_SEGMENT segment;

            segment.point1 = to_d2d_point_2f(element);
            segment.point2 = to_d2d_point_2f(data1);
            segment.point3 = to_d2d_point_2f(data2);

            sink->AddBezier(segment);
        }
            break;

        case QPainterPath::CurveToDataElement:
            qWarning("%s: Unhandled Curve Data Element", __FUNCTION__);
            break;
        }
    }

    if (inFigure)
        sink->EndFigure(D2D1_FIGURE_END_OPEN);

    sink->Close();

    return geometry;
}

static ComPtr<ID2D1PathGeometry> regionToPathGeometry(const QRegion &region)
{
    QPainterPath ppath;
    ppath.addRegion(region);
    return painterPathToPathGeometry(ppath);
}

class QWindowsDirect2DPaintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QWindowsDirect2DPaintEngine)
public:
    QWindowsDirect2DPaintEnginePrivate(QWindowsDirect2DBitmap *bm)
        : bitmap(bm)
        , clipPushed(false)
        , hasPerspectiveTransform(false)
        , opacity(1.0)
        , renderHints(QPainter::TextAntialiasing)
    {
        pen.reset();

        HRESULT hr = factory()->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND,
                                                                              D2D1_CAP_STYLE_ROUND,
                                                                              D2D1_CAP_STYLE_ROUND),
                                                  NULL, 0,
                                                  pointStrokeStyle.ReleaseAndGetAddressOf());
        if (FAILED(hr))
            qWarning("%s: Could not create stroke style for points and zero length lines: %#x", __FUNCTION__, hr);

        dc()->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    }

    QWindowsDirect2DBitmap *bitmap;

    QPainterPath clipPath;
    bool clipPushed;

    ComPtr<ID2D1StrokeStyle> pointStrokeStyle;
    QPointF currentBrushOrigin;

    bool hasPerspectiveTransform;

    qreal opacity;

    struct {
        qreal width;
        QColor color;
        bool isNull;
        ComPtr<ID2D1Brush> brush;
        ComPtr<ID2D1StrokeStyle1> strokeStyle;

        inline void reset() {
            width = defaultPenWidth;
            color = QColor(Qt::black);
            isNull = true;
            brush.Reset();
            strokeStyle.Reset();
        }
    } pen;

    struct {
        ComPtr<ID2D1Brush> brush;
    } brush;

    QPainter::RenderHints renderHints;

    inline ID2D1DeviceContext *dc() const
    {
        Q_ASSERT(bitmap);
        return bitmap->deviceContext()->get();
    }

    inline D2D1_INTERPOLATION_MODE interpolationMode() const
    {
        return (renderHints & QPainter::SmoothPixmapTransform) ? D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC
                                                               : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
    }

    inline D2D1_ANTIALIAS_MODE antialiasMode() const
    {
        return (renderHints & QPainter::Antialiasing) ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
                                                      : D2D1_ANTIALIAS_MODE_ALIASED;
    }

    void updateState(const QPaintEngineState &state, QPaintEngine::DirtyFlags dirty)
    {
        if (dirty & QPaintEngine::DirtyPen)
            updatePen(state.pen());

        if (dirty & QPaintEngine::DirtyBrush)
            updateBrush(state.brush());

        if (dirty & QPaintEngine::DirtyBrushOrigin)
            updateBrushOrigin(state.brushOrigin());

        if (dirty & QPaintEngine::DirtyHints)
            updateHints(state.renderHints());

        if (dirty & QPaintEngine::DirtyTransform)
            updateTransform(state.transform());

        if (dirty & QPaintEngine::DirtyClipEnabled)
            updateClipEnabled(state.isClipEnabled());

        if (dirty & QPaintEngine::DirtyClipPath)
            updateClipPath(state.clipPath(), state.clipOperation());

        if (dirty & QPaintEngine::DirtyClipRegion)
            updateClipRegion(state.clipRegion(), state.clipOperation());

        if (dirty & QPaintEngine::DirtyOpacity)
            updateOpacity(state.opacity());
    }

    void updateTransform(const QTransform &t)
    {
        dc()->SetTransform(to_d2d_matrix_3x2_f(t));
        hasPerspectiveTransform = !t.isAffine();
    }

    void updateOpacity(qreal o)
    {
        opacity = o;
        if (brush.brush)
            brush.brush.Get()->SetOpacity(o);
        if (pen.brush)
            pen.brush.Get()->SetOpacity(o);
    }

    void pushClip()
    {
        ComPtr<ID2D1Geometry> geometricMask = painterPathToPathGeometry(clipPath);
        if (!geometricMask) {
            qWarning("%s: Could not convert painter path, not pushing clip path!", __FUNCTION__);
            return;
        }

        popClip();
        dc()->PushLayer(D2D1::LayerParameters1(D2D1::InfiniteRect(),
                                               geometricMask.Get(),
                                               antialiasMode(),
                                               D2D1::IdentityMatrix(),
                                               1.0,
                                               NULL,
                                               D2D1_LAYER_OPTIONS1_INITIALIZE_FROM_BACKGROUND),
                        NULL);
        clipPushed = true;
    }

    void popClip()
    {
        if (clipPushed) {
            dc()->PopLayer();
            clipPushed = false;
        }
    }

    void updateClipEnabled(bool enabled)
    {
        if (!enabled)
            popClip();
        else if (!clipPushed)
            pushClip();
    }

    void updateClipPath(const QPainterPath &path, Qt::ClipOperation operation)
    {
        switch (operation) {
        case Qt::NoClip:
            popClip();
            break;
        case Qt::ReplaceClip:
            clipPath = path;
            pushClip();
            break;
        case Qt::IntersectClip:
            clipPath &= path;
            pushClip();
            break;
        }
    }

    void updateClipRegion(const QRegion &region, Qt::ClipOperation operation)
    {
        QPainterPath p;
        p.addRegion(region);
        updateClipPath(p, operation);
    }

    void updateBrush(const QBrush &newBrush)
    {
        brush.brush = to_d2d_brush(newBrush);
        if (brush.brush) {
            brush.brush->SetOpacity(opacity);
            applyBrushOrigin(currentBrushOrigin);
        }
    }

    void updateBrushOrigin(const QPointF &origin)
    {
        negateCurrentBrushOrigin();
        applyBrushOrigin(origin);
    }

    void negateCurrentBrushOrigin()
    {
        if (brush.brush && !currentBrushOrigin.isNull()) {
            D2D1_MATRIX_3X2_F transform;
            brush.brush->GetTransform(&transform);

            brush.brush->SetTransform(*(D2D1::Matrix3x2F::ReinterpretBaseType(&transform))
                                      * D2D1::Matrix3x2F::Translation(-currentBrushOrigin.x(),
                                                                      -currentBrushOrigin.y()));
        }
    }

    void applyBrushOrigin(const QPointF &origin)
    {
        if (brush.brush && !origin.isNull()) {
            D2D1_MATRIX_3X2_F transform;
            brush.brush->GetTransform(&transform);

            brush.brush->SetTransform(*(D2D1::Matrix3x2F::ReinterpretBaseType(&transform))
                                      * D2D1::Matrix3x2F::Translation(origin.x(), origin.y()));
        }

        currentBrushOrigin = origin;
    }

    void updatePen(const QPen &newPen)
    {
        pen.reset();

        if (newPen.style() == Qt::NoPen)
            return;

        pen.isNull = false;
        pen.brush = to_d2d_brush(newPen.brush());
        if (!pen.brush)
            return;

        pen.width = newPen.widthF();
        pen.color = newPen.color();
        pen.brush->SetOpacity(opacity);

        D2D1_STROKE_STYLE_PROPERTIES1 props = {};

        // Try and match Qt's raster engine in output as closely as possible
        switch (newPen.style()) {
        case Qt::DotLine:
        case Qt::DashDotLine:
        case Qt::DashDotDotLine:
            if (pen.width <= 1.0) {
                props.startCap = props.endCap = props.dashCap = D2D1_CAP_STYLE_FLAT;
                break;
            }
            // fall through
        default:
            switch (newPen.capStyle()) {
            case Qt::SquareCap:
                props.startCap = props.endCap = props.dashCap = D2D1_CAP_STYLE_SQUARE;
                break;
            case Qt::RoundCap:
                props.startCap = props.endCap = props.dashCap = D2D1_CAP_STYLE_ROUND;
            case Qt::FlatCap:
            default:
                props.startCap = props.endCap = props.dashCap = D2D1_CAP_STYLE_FLAT;
                break;
            }
            break;
        }

        switch (newPen.joinStyle()) {
        case Qt::BevelJoin:
            props.lineJoin = D2D1_LINE_JOIN_BEVEL;
            break;
        case Qt::RoundJoin:
            props.lineJoin = D2D1_LINE_JOIN_ROUND;
            break;
        case Qt::MiterJoin:
        default:
            props.lineJoin = D2D1_LINE_JOIN_MITER;
            break;
        }

        props.miterLimit = newPen.miterLimit() * qreal(2.0); // D2D and Qt miter specs differ
        props.dashOffset = newPen.dashOffset();
        props.transformType = qIsNull(newPen.widthF()) ? D2D1_STROKE_TRANSFORM_TYPE_HAIRLINE
                                                       : newPen.isCosmetic() ? D2D1_STROKE_TRANSFORM_TYPE_FIXED
                                                                             : D2D1_STROKE_TRANSFORM_TYPE_NORMAL;

        switch (newPen.style()) {
        case Qt::SolidLine:
            props.dashStyle = D2D1_DASH_STYLE_SOLID;
            break;
        default:
            props.dashStyle = D2D1_DASH_STYLE_CUSTOM;
            break;
        }

        HRESULT hr;

        if (props.dashStyle == D2D1_DASH_STYLE_CUSTOM) {
            QVector<qreal> dashes = newPen.dashPattern();
            QVector<FLOAT> converted(dashes.size());

            for (int i = 0; i < dashes.size(); i++) {
                converted[i] = dashes[i];
            }

            hr = factory()->CreateStrokeStyle(props, converted.constData(), converted.size(), &(pen.strokeStyle));
        } else {
            hr = factory()->CreateStrokeStyle(props, NULL, 0, &(pen.strokeStyle));
        }

        if (FAILED(hr))
            qWarning("%s: Could not create stroke style: %#x", __FUNCTION__, hr);
    }

    ComPtr<ID2D1Brush> to_d2d_brush(const QBrush &newBrush)
    {
        HRESULT hr;
        ComPtr<ID2D1Brush> result;

        switch (newBrush.style()) {
        case Qt::NoBrush:
            break;

        case Qt::SolidPattern:
        {
            ComPtr<ID2D1SolidColorBrush> solid;

            hr = dc()->CreateSolidColorBrush(to_d2d_color_f(newBrush.color()), &solid);
            if (FAILED(hr)) {
                qWarning("%s: Could not create solid color brush: %#x", __FUNCTION__, hr);
                break;
            }

            hr = solid.As(&result);
            if (FAILED(hr))
                qWarning("%s: Could not convert solid color brush: %#x", __FUNCTION__, hr);
        }
            break;

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
        case Qt::DiagCrossPattern:
        {
            ComPtr<ID2D1BitmapBrush1> bitmapBrush;
            D2D1_BITMAP_BRUSH_PROPERTIES1 bitmapBrushProperties = {
                D2D1_EXTEND_MODE_WRAP,
                D2D1_EXTEND_MODE_WRAP,
                interpolationMode()
            };

            QImage brushImg = qt_imageForBrush(newBrush.style(), false);
            brushImg.setColor(0, newBrush.color().rgba());
            brushImg.setColor(1, qRgba(0, 0, 0, 0));

            QWindowsDirect2DBitmap bitmap;
            bool success = bitmap.fromImage(brushImg, Qt::AutoColor);
            if (!success) {
                qWarning("%s: Could not create Direct2D bitmap from Qt pattern brush image", __FUNCTION__);
                break;
            }

            hr = dc()->CreateBitmapBrush(bitmap.bitmap(),
                                         bitmapBrushProperties,
                                         &bitmapBrush);
            if (FAILED(hr)) {
                qWarning("%s: Could not create Direct2D bitmap brush for Qt pattern brush: %#x", __FUNCTION__, hr);
                break;
            }

            hr = bitmapBrush.As(&result);
            if (FAILED(hr))
                qWarning("%s: Could not convert Direct2D bitmap brush for Qt pattern brush: %#x", __FUNCTION__, hr);
        }
            break;

        case Qt::LinearGradientPattern:
        case Qt::RadialGradientPattern:
        case Qt::ConicalGradientPattern:
            break;

        case Qt::TexturePattern:
        {
            ComPtr<ID2D1BitmapBrush1> bitmapBrush;
            D2D1_BITMAP_BRUSH_PROPERTIES1 bitmapBrushProperties = {
                D2D1_EXTEND_MODE_WRAP,
                D2D1_EXTEND_MODE_WRAP,
                interpolationMode()
            };

            QWindowsDirect2DPlatformPixmap *pp = static_cast<QWindowsDirect2DPlatformPixmap *>(newBrush.texture().handle());
            QWindowsDirect2DBitmap *bitmap = pp->bitmap();
            hr = dc()->CreateBitmapBrush(bitmap->bitmap(),
                                         bitmapBrushProperties,
                                         &bitmapBrush);

            if (FAILED(hr)) {
                qWarning("%s: Could not create texture brush: %#x", __FUNCTION__, hr);
                break;
            }

            hr = bitmapBrush.As(&result);
            if (FAILED(hr))
                qWarning("%s: Could not convert texture brush: %#x", __FUNCTION__, hr);
        }
            break;
        }

        if (result && !newBrush.transform().isIdentity())
            result->SetTransform(to_d2d_matrix_3x2_f(newBrush.transform()));

        return result;
    }

    void updateHints(QPainter::RenderHints newHints)
    {
        renderHints = newHints;
        dc()->SetAntialiasMode(antialiasMode());
    }

    template <typename T>
    void drawLines(const T* lines, int lineCount)
    {
        if (!pen.brush)
            return;

        for (int i = 0; i < lineCount; i++) {
            const T &line = lines[i];

            // Try to fit Qt's and Direct2D's idea of zero length line
            // handling together nicely.

            if (line.p1() == line.p2() && pen.strokeStyle.Get()->GetDashCap() != D2D1_CAP_STYLE_SQUARE) {
                if (pen.width <= 1.0) {
                    dc()->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
                    // Note that we use pointStrokeStyle here, not the pen's stroke style!
                    dc()->DrawLine(to_d2d_point_2f(line.p1()), to_d2d_point_2f(line.p2()), pen.brush.Get(), pen.width, pointStrokeStyle.Get());
                    dc()->SetAntialiasMode(antialiasMode());
                }
            } else
                dc()->DrawLine(to_d2d_point_2f(line.p1()), to_d2d_point_2f(line.p2()), pen.brush.Get(), pen.width, pen.strokeStyle.Get());
        }
    }

    template <typename T>
    void drawRects(const T* rects, int rectCount)
    {
        if (!brush.brush && !pen.brush)
            return;

        for (int i = 0; i < rectCount; i++) {
            if (brush.brush)
                dc()->FillRectangle(to_d2d_rect_f(rects[i]), brush.brush.Get());

            // Direct2D for some reason uses different geometry in FillRectangle and DrawRectangle.
            // We have to adjust the rect right and down here by one pixel to paint the rectangle properly.
            if (pen.brush)
                dc()->DrawRectangle(to_d2d_rect_f(rects[i].adjusted(1, 1, 1, 1)), pen.brush.Get(), pen.width, pen.strokeStyle.Get());
        }
    }

    template <typename T>
    void drawPolygon(const T* points, int pointCount, QPaintEngine::PolygonDrawMode mode)
    {
        if (pointCount < 3)
            return;

        if (!brush.brush && !pen.brush)
            return;

        QVector<D2D1_POINT_2F> converted(pointCount);
        for (int i = 0; i < pointCount; i++) {
            const T &p = points[i];
            converted[i].x = p.x();
            converted[i].y = p.y();
        }

        drawPolygon(converted.constData(), converted.size(), mode);
    }

    void drawPolygon(const D2D1_POINT_2F *points, int pointCount, QPaintEngine::PolygonDrawMode mode)
    {
        ComPtr<ID2D1PathGeometry> geometry;
        ComPtr<ID2D1GeometrySink> sink;
        const bool is_polyline = mode == QPaintEngine::PolylineMode;

        HRESULT hr = factory()->CreatePathGeometry(&geometry);
        if (FAILED(hr))
            return;

        hr = geometry->Open(&sink);
        if (FAILED(hr))
            return;

        switch (mode) {
        case QPaintEngine::OddEvenMode:
            sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
            break;
        case QPaintEngine::WindingMode:
            sink->SetFillMode(D2D1_FILL_MODE_WINDING);
            break;
        case QPaintEngine::ConvexMode:
        case QPaintEngine::PolylineMode:
            // XXX
            break;
        }

        sink->BeginFigure(points[0], is_polyline ? D2D1_FIGURE_BEGIN_HOLLOW
                                                 : D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLines(points + 1, pointCount - 1);
        sink->EndFigure(is_polyline ? D2D1_FIGURE_END_OPEN
                                    : D2D1_FIGURE_END_CLOSED);
        sink->Close();

        if (brush.brush)
            dc()->FillGeometry(geometry.Get(), brush.brush.Get());

        if (pen.brush)
            dc()->DrawGeometry(geometry.Get(), pen.brush.Get(), pen.width, pen.strokeStyle.Get());
    }
};

QWindowsDirect2DPaintEngine::QWindowsDirect2DPaintEngine(QWindowsDirect2DBitmap *bitmap)
    : QPaintEngine(PrimitiveTransform
                   | PatternTransform
                   | PixmapTransform
                   | PatternBrush
                   | AlphaBlend
                   | PainterPaths
                   | Antialiasing
                   | BrushStroke
                   | ConstantOpacity
                   | MaskedBrush
                   | ObjectBoundingModeGradients

                   // Although Direct2D 1.1 contains support for both linear and radial gradients,
                   // there unfortunately is no support for repeating or reflecting versions of them
                   //| LinearGradientFill
                   //| RadialGradientFill

                   // Unsupported entirely by Direct2D 1.1
                   //| ConicalGradientFill

                   // We might be able to support this using Direct2D effects
                   //| PorterDuff

                   // Direct2D currently only supports affine transforms directly
                   // We might be able to support this using Direct2D effects
                   //| PerspectiveTransform

                   // We might be able to support this using Direct2D effects
                   //| BlendModes

                   // We might be able to support this using Direct2D effects
                   //| RasterOpModes

                   // We have to inform Direct2D when we start and end drawing
                   //| PaintOutsidePaintEvent
                   )
    , d_ptr(new QWindowsDirect2DPaintEnginePrivate(bitmap))
{

}

bool QWindowsDirect2DPaintEngine::begin(QPaintDevice * pdev)
{
    Q_D(QWindowsDirect2DPaintEngine);

    d->bitmap->deviceContext()->begin();
    d->dc()->SetTransform(D2D1::Matrix3x2F::Identity());

    QRect clip(0, 0, pdev->width(), pdev->height());
    if (!systemClip().isEmpty()) {
        clip &= systemClip().boundingRect();
    }
    d->dc()->PushAxisAlignedClip(to_d2d_rect_f(clip), d->antialiasMode());
    updateState(*state);

    D2D_TAG(D2DDebugDrawInitialStateTag);

    return true;
}

bool QWindowsDirect2DPaintEngine::end()
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->popClip();
    d->dc()->PopAxisAlignedClip();
    return d->bitmap->deviceContext()->end();
}

void QWindowsDirect2DPaintEngine::updateState(const QPaintEngineState &state)
{
    Q_D(QWindowsDirect2DPaintEngine);
    d->updateState(state, state.state());
}

QPaintEngine::Type QWindowsDirect2DPaintEngine::type() const
{
    return QPaintEngine::Direct2D;
}

void QWindowsDirect2DPaintEngine::drawEllipse(const QRectF &rect)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawEllipseTag);

    D2D1_ELLIPSE ellipse = {
        to_d2d_point_2f(rect.center()),
        rect.width() / 2,
        rect.height() / 2
    };

    if (d->brush.brush)
        d->dc()->FillEllipse(ellipse, d->brush.brush.Get());

    if (d->pen.brush) {
        d->dc()->DrawEllipse(ellipse, d->pen.brush.Get(), d->pen.width, d->pen.strokeStyle.Get());
    }
}

void QWindowsDirect2DPaintEngine::drawEllipse(const QRect &rect)
{
    drawEllipse(QRectF(rect));
}

void QWindowsDirect2DPaintEngine::drawImage(const QRectF &rectangle, const QImage &image,
                                            const QRectF &sr, Qt::ImageConversionFlags flags)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawImageTag);

    QPixmap pixmap = QPixmap::fromImage(image, flags);
    drawPixmap(rectangle, pixmap, sr);
}

void QWindowsDirect2DPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawLinesTag);
    d->drawLines(lines, lineCount);
}

void QWindowsDirect2DPaintEngine::drawLines(const QLine *lines, int lineCount)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawLinesTag);
    d->drawLines(lines, lineCount);
}

void QWindowsDirect2DPaintEngine::drawPath(const QPainterPath &path)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawPathTag);

    if (path.elementCount() == 0)
        return;

    if (!d->brush.brush && !d->pen.brush)
        return;

    ComPtr<ID2D1PathGeometry> geometry = painterPathToPathGeometry(path);
    if (!geometry)
        return;

    if (d->brush.brush)
        d->dc()->FillGeometry(geometry.Get(), d->brush.brush.Get());

    if (d->pen.brush)
        d->dc()->DrawGeometry(geometry.Get(), d->pen.brush.Get(), d->pen.width, d->pen.strokeStyle.Get());
}

void QWindowsDirect2DPaintEngine::drawPixmap(const QRectF &r,
                                             const QPixmap &pm,
                                             const QRectF &sr)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawPixmapTag);

    if (pm.isNull())
        return;

    if (pm.handle()->pixelType() == QPlatformPixmap::BitmapType) {
        QImage i = pm.toImage();
        i.setColor(0, qRgba(0, 0, 0, 0));
        i.setColor(1, d->pen.color.rgba());
        drawImage(r, i, sr);
        return;
    }

    QWindowsDirect2DPlatformPixmap *pp = static_cast<QWindowsDirect2DPlatformPixmap *>(pm.handle());
    QWindowsDirect2DBitmap *bitmap = pp->bitmap();

    if (bitmap->bitmap() != d->bitmap->bitmap()) {
        // Good, src bitmap != dst bitmap
        if (sr.isValid())
            d->dc()->DrawBitmap(bitmap->bitmap(),
                                to_d2d_rect_f(r), d->opacity,
                                d->interpolationMode(),
                                to_d2d_rect_f(sr));
        else
            d->dc()->DrawBitmap(bitmap->bitmap(),
                                to_d2d_rect_f(r), d->opacity,
                                d->interpolationMode());
    } else {
        // Ok, so the source pixmap and destination pixmap is the same.
        // D2D is not fond of this scenario, deal with it through
        // an intermediate bitmap
        QWindowsDirect2DBitmap intermediate;

        if (sr.isValid()) {
            bool r = intermediate.resize(sr.width(), sr.height());
            if (!r) {
                qDebug("%s: Could not resize intermediate bitmap to source rect size", __FUNCTION__);
                return;
            }

            D2D1_RECT_U d2d_sr =  to_d2d_rect_u(sr.toRect());
            HRESULT hr = intermediate.bitmap()->CopyFromBitmap(NULL,
                                                               bitmap->bitmap(),
                                                               &d2d_sr);
            if (FAILED(hr)) {
                qWarning("%s: Could not copy source rect area from source bitmap to intermediate bitmap: %#x", __FUNCTION__, hr);
                return;
            }
        } else {
            bool r = intermediate.resize(bitmap->size().width(),
                                         bitmap->size().height());
            if (!r) {
                qDebug("%s: Could not resize intermediate bitmap to source bitmap size", __FUNCTION__);
                return;
            }

            HRESULT hr = intermediate.bitmap()->CopyFromBitmap(NULL,
                                                               bitmap->bitmap(),
                                                               NULL);
            if (FAILED(hr)) {
                qWarning("%s: Could not copy source bitmap to intermediate bitmap: %#x", __FUNCTION__, hr);
                return;
            }
        }

        d->dc()->DrawBitmap(intermediate.bitmap(),
                            to_d2d_rect_f(r), d->opacity,
                            d->interpolationMode());
    }
}

void QWindowsDirect2DPaintEngine::drawPolygon(const QPointF *points,
                                              int pointCount,
                                              QPaintEngine::PolygonDrawMode mode)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawPolygonTag);
    d->drawPolygon(points, pointCount, mode);
}

void QWindowsDirect2DPaintEngine::drawPolygon(const QPoint *points,
                                              int pointCount,
                                              QPaintEngine::PolygonDrawMode mode)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawPolygonTag);
    d->drawPolygon(points, pointCount, mode);
}

void QWindowsDirect2DPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawRectsTag);
    d->drawRects(rects, rectCount);
}

void QWindowsDirect2DPaintEngine::drawRects(const QRect *rects, int rectCount)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawRectsTag);
    d->drawRects(rects, rectCount);
}

// Points (1/72 inches) to Microsoft's Device Independent Pixels (1/96 inches)
inline static Q_DECL_CONSTEXPR FLOAT pointSizeToDIP(qreal pointSize)
{
    return pointSize + (pointSize / qreal(3.0));
}

inline static FLOAT pixelSizeToDIP(int pixelSize)
{
    FLOAT dpiX, dpiY;
    factory()->GetDesktopDpi(&dpiX, &dpiY);

    return FLOAT(pixelSize) / (dpiY / 96.0f);
}

inline static FLOAT fontSizeInDIP(const QFont &font)
{
    // Microsoft wants the font size in DIPs (Device Independent Pixels), each of which is 1/96 inches.
    if (font.pixelSize() == -1) {
        // font size was set as points
        return pointSizeToDIP(font.pointSizeF());
    } else {
        // font size was set as pixels
        return pixelSizeToDIP(font.pixelSize());
    }
}

void QWindowsDirect2DPaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawTextItemTag);

    if (d->pen.isNull)
        return;

    // If we can't support the current configuration with Direct2D, fall back to slow path
    // Most common cases are perspective transform and gradient brush as pen
    if (d->hasPerspectiveTransform || !d->pen.brush) {
        QPaintEngine::drawTextItem(p, textItem);
        return;
    }

    const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);
    ComPtr<IDWriteFontFace> fontFace;

    switch (ti.fontEngine->type()) {
    case QFontEngine::Win:
    {
        QWindowsFontEngine *wfe = static_cast<QWindowsFontEngine *>(ti.fontEngine);
        QSharedPointer<QWindowsFontEngineData> wfed = wfe->fontEngineData();

        HGDIOBJ oldfont = wfe->selectDesignFont();
        HRESULT hr = QWindowsDirect2DContext::instance()->dwriteGdiInterop()->CreateFontFaceFromHdc(wfed->hdc, &fontFace);
        DeleteObject(SelectObject(wfed->hdc, oldfont));
        if (FAILED(hr))
            qWarning("%s: Could not create DirectWrite fontface from HDC: %#x", __FUNCTION__, hr);

    }
        break;

#ifndef QT_NO_DIRECTWRITE

    case QFontEngine::DirectWrite:
    {
        QWindowsFontEngineDirectWrite *wfedw = static_cast<QWindowsFontEngineDirectWrite *>(ti.fontEngine);
        fontFace = wfedw->directWriteFontFace();
    }
        break;

#endif // QT_NO_DIRECTWRITE

    default:
        qDebug("%s: Unknown font engine!", __FUNCTION__);
        break;
    }

    if (!fontFace) {
        qDebug("%s: Could not find font - falling back to slow text rendering path.", __FUNCTION__);
        QPaintEngine::drawTextItem(p, textItem);
        return;
    }

    QVector<UINT16> glyphIndices(ti.glyphs.numGlyphs);
    QVector<FLOAT> glyphAdvances(ti.glyphs.numGlyphs);
    QVector<DWRITE_GLYPH_OFFSET> glyphOffsets(ti.glyphs.numGlyphs);

    // Imperfect conversion here
    for (int i = 0; i < ti.glyphs.numGlyphs; i++) {
        glyphIndices[i] = UINT16(ti.glyphs.glyphs[i]);
        glyphAdvances[i] = ti.glyphs.effectiveAdvance(i).toReal();
        glyphOffsets[i].advanceOffset = ti.glyphs.offsets[i].x.toReal();
        glyphOffsets[i].ascenderOffset = ti.glyphs.offsets[i].y.toReal();
    }

    const bool rtl = (ti.flags & QTextItem::RightToLeft);
    const UINT32 bidiLevel = rtl ? 1 : 0;

    DWRITE_GLYPH_RUN glyphRun = {
        fontFace.Get(),             //    IDWriteFontFace           *fontFace;
        fontSizeInDIP(ti.font()),   //    FLOAT                     fontEmSize;
        ti.glyphs.numGlyphs,        //    UINT32                    glyphCount;
        glyphIndices.constData(),   //    const UINT16              *glyphIndices;
        glyphAdvances.constData(),  //    const FLOAT               *glyphAdvances;
        glyphOffsets.constData(),   //    const DWRITE_GLYPH_OFFSET *glyphOffsets;
        FALSE,                      //    BOOL                      isSideways;
        bidiLevel                   //    UINT32                    bidiLevel;
    };

    const bool antiAlias = bool((d->renderHints & QPainter::TextAntialiasing)
                                && !(ti.font().styleStrategy() & QFont::NoAntialias));
    d->dc()->SetTextAntialiasMode(antiAlias ? D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE
                                            : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);

    const QPointF offset(rtl ? ti.width.toReal() : 0, 0);
    d->dc()->DrawGlyphRun(to_d2d_point_2f(p + offset),
                          &glyphRun,
                          NULL,
                          d->pen.brush.Get(),
                          DWRITE_MEASURING_MODE_GDI_CLASSIC);
}

static void qt_draw_tile(QPaintEngine *gc, qreal x, qreal y, qreal w, qreal h,
                         const QPixmap &pixmap, qreal xOffset, qreal yOffset)
{
    qreal yPos, xPos, drawH, drawW, yOff, xOff;
    yPos = y;
    yOff = yOffset;
    while (yPos < y + h) {
        drawH = pixmap.height() - yOff;    // Cropping first row
        if (yPos + drawH > y + h)           // Cropping last row
            drawH = y + h - yPos;
        xPos = x;
        xOff = xOffset;
        while (xPos < x + w) {
            drawW = pixmap.width() - xOff; // Cropping first column
            if (xPos + drawW > x + w)           // Cropping last column
                drawW = x + w - xPos;
            if (drawW > 0 && drawH > 0)
                gc->drawPixmap(QRectF(xPos, yPos, drawW, drawH), pixmap, QRectF(xOff, yOff, drawW, drawH));
            xPos += drawW;
            xOff = 0;
        }
        yPos += drawH;
        yOff = 0;
    }
}

void QWindowsDirect2DPaintEngine::drawTiledPixmap(const QRectF &rect, const QPixmap &pixmap, const QPointF &p)
{
    Q_D(QWindowsDirect2DPaintEngine);
    D2D_TAG(D2DDebugDrawTextItemTag);
    qt_draw_tile(this, rect.x(), rect.y(), rect.width(), rect.height(), pixmap, p.x(), p.y());
}

QT_END_NAMESPACE
