/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QFONTENGINE_CORETEXT_P_H
#define QFONTENGINE_CORETEXT_P_H

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

#include <private/qfontengine_p.h>
#include <private/qcore_mac_p.h>
#include <QtCore/qloggingcategory.h>

#ifdef Q_OS_MACOS
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaFonts)

class QCoreTextFontEngine : public QFontEngine
{
    Q_GADGET

public:
    QCoreTextFontEngine(CTFontRef font, const QFontDef &def);
    QCoreTextFontEngine(CGFontRef font, const QFontDef &def);
    ~QCoreTextFontEngine();

    glyph_t glyphIndex(uint ucs4) const override;
    bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const override;
    void recalcAdvances(QGlyphLayout *, ShaperFlags) const override;

    glyph_metrics_t boundingBox(const QGlyphLayout &glyphs) override;
    glyph_metrics_t boundingBox(glyph_t glyph) override;

    QFixed ascent() const override;
    QFixed capHeight() const override;
    QFixed descent() const override;
    QFixed leading() const override;
    QFixed xHeight() const override;
    qreal maxCharWidth() const override;
    QFixed averageCharWidth() const override;

    void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int numGlyphs,
                         QPainterPath *path, QTextItem::RenderFlags) override;

    bool canRender(const QChar *string, int len) const override;

    int synthesized() const override { return synthesisFlags; }
    bool supportsSubPixelPositions() const override { return true; }

    QFixed lineThickness() const override;
    QFixed underlinePosition() const override;

    void draw(CGContextRef ctx, qreal x, qreal y, const QTextItemInt &ti, int paintDeviceHeight);

    FaceId faceId() const override;
    bool getSfntTableData(uint /*tag*/, uchar * /*buffer*/, uint * /*length*/) const override;
    void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics) override;
    QImage alphaMapForGlyph(glyph_t, QFixed subPixelPosition) override;
    QImage alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &t) override;
    QImage alphaRGBMapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t) override;
    glyph_metrics_t alphaMapBoundingBox(glyph_t glyph, QFixed, const QTransform &matrix, GlyphFormat) override;
    QImage bitmapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t, const QColor &color) override;
    QFixed emSquareSize() const override;
    void doKerning(QGlyphLayout *g, ShaperFlags flags) const override;

    bool supportsTransformation(const QTransform &transform) const override;
    bool expectsGammaCorrectedBlending() const override;

    QFontEngine *cloneWithSize(qreal pixelSize) const override;
    Qt::HANDLE handle() const override;
    int glyphMargin(QFontEngine::GlyphFormat format) override { Q_UNUSED(format); return 0; }

    QFontEngine::Properties properties() const override;

    enum FontSmoothing { Disabled, Subpixel, Grayscale };
    Q_ENUM(FontSmoothing);

    static FontSmoothing fontSmoothing();
    static qreal fontSmoothingGamma();

    static bool ct_getSfntTable(void *user_data, uint tag, uchar *buffer, uint *length);
    static QFont::Weight qtWeightFromCFWeight(float value);

    static QCoreTextFontEngine *create(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference);

protected:
    QCoreTextFontEngine(const QFontDef &def);
    void init();
    QImage imageForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &m, const QColor &color = QColor());
    void loadAdvancesForGlyphs(QVarLengthArray<CGGlyph> &cgGlyphs, QGlyphLayout *glyphs) const;
    bool hasColorGlyphs() const;
    bool shouldAntialias() const;
    bool shouldSmoothFont() const;

    QCFType<CTFontRef> ctfont;
    QCFType<CGFontRef> cgFont;
    int synthesisFlags;
    CGAffineTransform transform;
    QFixed avgCharWidth;
    QFixed underlineThickness;
    QFixed underlinePos;
    QFontEngine::FaceId face_id;
    mutable bool kerningPairsLoaded;
};

CGAffineTransform qt_transform_from_fontdef(const QFontDef &fontDef);

QT_END_NAMESPACE

#endif // QFONTENGINE_CORETEXT_P_H
