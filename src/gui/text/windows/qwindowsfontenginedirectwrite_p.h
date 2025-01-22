// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSFONTENGINEDIRECTWRITE_H
#define QWINDOWSFONTENGINEDIRECTWRITE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qtguiglobal.h>
#include <QtGui/private/qtgui-config_p.h>

QT_REQUIRE_CONFIG(directwrite);

#include <QtGui/private/qfontengine_p.h>
#include <QtCore/QSharedPointer>

#include <dwrite_2.h>

struct IDWriteFont;
struct IDWriteFontFace;
struct IDWriteFontFile;
struct IDWriteFactory;
struct IDWriteBitmapRenderTarget;
struct IDWriteGdiInterop;
struct IDWriteGlyphRunAnalysis;
struct IDWriteColorGlyphRunEnumerator;

#if QT_CONFIG(directwritecolrv1)
struct IDWriteFontFace7;
struct IDWritePaintReader;
struct DWRITE_PAINT_ELEMENT;
#endif

QT_BEGIN_NAMESPACE

class QWindowsFontEngineData;
class QColrPaintGraphRenderer;

class Q_GUI_EXPORT QWindowsFontEngineDirectWrite : public QFontEngine
{
    Q_DISABLE_COPY_MOVE(QWindowsFontEngineDirectWrite)
public:
    explicit QWindowsFontEngineDirectWrite(IDWriteFontFace *directWriteFontFace,
                                    qreal pixelSize,
                                    const QSharedPointer<QWindowsFontEngineData> &d);
    ~QWindowsFontEngineDirectWrite() override;

    void initFontInfo(const QFontDef &request, int dpi);

    QFixed lineThickness() const override;
    QFixed underlinePosition() const override;
    bool getSfntTableData(uint tag, uchar *buffer, uint *length) const override;
    QFixed emSquareSize() const override;

    glyph_t glyphIndex(uint ucs4) const override;
    int stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs,
                     ShaperFlags flags) const override;
    void recalcAdvances(QGlyphLayout *glyphs, ShaperFlags) const override;

    void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                         QPainterPath *path, QTextItem::RenderFlags flags) override;

    glyph_metrics_t boundingBox(const QGlyphLayout &glyphs) override;
    glyph_metrics_t boundingBox(glyph_t g) override;
    glyph_metrics_t alphaMapBoundingBox(glyph_t glyph, const QFixedPoint&,
                                        const QTransform &matrix, GlyphFormat) override;

    QFixed capHeight() const override;
    QFixed xHeight() const override;
    qreal maxCharWidth() const override;
    FaceId faceId() const override;

    bool supportsHorizontalSubPixelPositions() const override;

    HFONT createHFONT() const;

    QImage alphaMapForGlyph(glyph_t glyph, const QFixedPoint &subPixelPosition) override;
    QImage alphaMapForGlyph(glyph_t glyph,
                            const QFixedPoint &subPixelPosition,
                            const QTransform &t) override;
    QImage alphaRGBMapForGlyph(glyph_t t,
                               const QFixedPoint &subPixelPosition,
                               const QTransform &xform) override;
    QImage bitmapForGlyph(glyph_t,
                          const QFixedPoint &subPixelPosition,
                          const QTransform &t,
                          const QColor &color) override;

    QFontEngine *cloneWithSize(qreal pixelSize) const override;
    Qt::HANDLE handle() const override;

    const QSharedPointer<QWindowsFontEngineData> &fontEngineData() const { return m_fontEngineData; }

    static QString fontNameSubstitute(const QString &familyName);

    IDWriteFontFace *directWriteFontFace() const { return m_directWriteFontFace; }

    void setUniqueFamilyName(const QString &newName) { m_uniqueFamilyName = newName; }

    void initializeHeightMetrics() const override;

    Properties properties() const override;

    QPainterPath unscaledGlyph(glyph_t glyph) const;
    void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics) override;

    QList<QFontVariableAxis> variableAxes() const override;

private:
#if QT_CONFIG(directwritecolrv1)
    bool traverseColr1(IDWritePaintReader *paintReader,
                       IDWriteFontFace7 *face7,
                       const DWRITE_PAINT_ELEMENT *paintElement,
                       QColrPaintGraphRenderer *graphRenderer) const;
    bool renderColr1GlyphRun(QImage *image,
                             const DWRITE_GLYPH_RUN *glyphRun,
                             const DWRITE_MATRIX &transform,
                             QColor color) const;
#endif

    QRect paintGraphBounds(glyph_t glyph, const DWRITE_MATRIX &transform) const;
    QRect alphaTextureBounds(glyph_t glyph, const DWRITE_MATRIX &transform);
    QRect colorBitmapBounds(glyph_t glyph, const DWRITE_MATRIX &transform);
    bool renderColr0GlyphRun(QImage *image,
                             const DWRITE_COLOR_GLYPH_RUN *colorGlyphRun,
                             const DWRITE_MATRIX &transform,
                             DWRITE_RENDERING_MODE renderMode,
                             DWRITE_MEASURING_MODE measureMode,
                             DWRITE_GRID_FIT_MODE gridFitMode,
                             QColor color,
                             QRect boundingRect) const;
    QImage renderColorGlyph(DWRITE_GLYPH_RUN *glyphRun,
                            const DWRITE_MATRIX &transform,
                            DWRITE_RENDERING_MODE renderMode,
                            DWRITE_MEASURING_MODE measureMode,
                            DWRITE_GRID_FIT_MODE gridFitMode,
                            QColor color,
                            QRect boundingRect) const;
    QImage imageForGlyph(glyph_t t,
                         const QFixedPoint &subPixelPosition,
                         int margin,
                         const QTransform &xform,
                         const QColor &color = QColor());
    void collectMetrics();
    void renderGlyphRun(QImage *destination,
                        float r,
                        float g,
                        float b,
                        float a,
                        IDWriteGlyphRunAnalysis *glyphAnalysis,
                        const QRect &boundingRect,
                        DWRITE_RENDERING_MODE renderMode) const;
    static QString filenameFromFontFile(IDWriteFontFile *fontFile);
    DWRITE_RENDERING_MODE hintingPreferenceToRenderingMode(const QFontDef &fontDef) const;

    const QSharedPointer<QWindowsFontEngineData> m_fontEngineData;

    IDWriteFontFace *m_directWriteFontFace;
    IDWriteBitmapRenderTarget *m_directWriteBitmapRenderTarget;

    QFixed m_lineThickness;
    QFixed m_underlinePosition;
    int m_unitsPerEm;
    QFixed m_capHeight;
    QFixed m_xHeight;
    QFixed m_maxAdvanceWidth;
    FaceId m_faceId;
    QString m_uniqueFamilyName;
    QList<QFontVariableAxis> m_variableAxes;
    DWRITE_PIXEL_GEOMETRY m_pixelGeometry = DWRITE_PIXEL_GEOMETRY_RGB;
};

QT_END_NAMESPACE

#endif // QWINDOWSFONTENGINEDIRECTWRITE_H
