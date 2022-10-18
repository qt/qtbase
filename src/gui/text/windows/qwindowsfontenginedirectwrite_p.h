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

struct IDWriteFont;
struct IDWriteFontFace;
struct IDWriteFontFile;
struct IDWriteFactory;
struct IDWriteBitmapRenderTarget;
struct IDWriteGdiInterop;
struct IDWriteGlyphRunAnalysis;

QT_BEGIN_NAMESPACE

class QWindowsFontEngineData;

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
    bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs,
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
    void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics) override;

private:
    QImage imageForGlyph(glyph_t t,
                         const QFixedPoint &subPixelPosition,
                         int margin,
                         const QTransform &xform,
                         const QColor &color = QColor());
    void collectMetrics();
    void renderGlyphRun(QImage *destination, float r, float g, float b, float a, IDWriteGlyphRunAnalysis *glyphAnalysis, const QRect &boundingRect);
    static QString filenameFromFontFile(IDWriteFontFile *fontFile);

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
};

QT_END_NAMESPACE

#endif // QWINDOWSFONTENGINEDIRECTWRITE_H
