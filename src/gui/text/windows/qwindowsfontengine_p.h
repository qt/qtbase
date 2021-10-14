// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSFONTENGINE_H
#define QWINDOWSFONTENGINE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qfontengine_p.h>

#include <QtGui/QImage>
#include <QtCore/QSharedPointer>
#include <QtCore/QMetaType>

#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class QWindowsNativeImage;
class QWindowsFontEngineData;

class QWindowsFontEngine : public QFontEngine
{
    Q_DISABLE_COPY_MOVE(QWindowsFontEngine)
public:
    QWindowsFontEngine(const QString &name, LOGFONT lf,
                       const QSharedPointer<QWindowsFontEngineData> &fontEngineData);

    ~QWindowsFontEngine() override;
    void initFontInfo(const QFontDef &request,
                      int dpi);

    QFixed lineThickness() const override;
    Properties properties() const override;
    void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics) override;
    FaceId faceId() const override;
    bool getSfntTableData(uint tag, uchar *buffer, uint *length) const override;
    int synthesized() const override;
    QFixed emSquareSize() const override;

    glyph_t glyphIndex(uint ucs4) const override;
    bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const override;
    void recalcAdvances(QGlyphLayout *glyphs, ShaperFlags) const override;

    void addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags) override;
    void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                         QPainterPath *path, QTextItem::RenderFlags flags) override;

    HGDIOBJ selectDesignFont() const;

    glyph_metrics_t boundingBox(glyph_t g) override { return boundingBox(g, QTransform()); }
    glyph_metrics_t boundingBox(glyph_t g, const QTransform &t) override;


    QFixed xHeight() const override;
    QFixed capHeight() const override;
    QFixed averageCharWidth() const override;
    qreal maxCharWidth() const override;
    qreal minLeftBearing() const override;
    qreal minRightBearing() const override;

    QImage alphaMapForGlyph(glyph_t t) override { return alphaMapForGlyph(t, QTransform()); }
    QImage alphaMapForGlyph(glyph_t, const QTransform &xform) override;
    QImage alphaRGBMapForGlyph(glyph_t t,
                               const QFixedPoint &subPixelPosition,
                               const QTransform &xform) override;
    glyph_metrics_t alphaMapBoundingBox(glyph_t glyph,
                                        const QFixedPoint &,
                                        const QTransform &matrix,
                                        GlyphFormat) override;

    QFontEngine *cloneWithSize(qreal pixelSize) const override;
    Qt::HANDLE handle() const override;
    bool supportsTransformation(const QTransform &transform) const override;

#ifndef Q_CC_MINGW
    void getGlyphBearings(glyph_t glyph, qreal *leftBearing = nullptr, qreal *rightBearing = nullptr) override;
#endif

    bool hasUnreliableGlyphOutline() const override;

    int getGlyphIndexes(const QChar *ch, int numChars, QGlyphLayout *glyphs) const;
    void getCMap();

    bool getOutlineMetrics(glyph_t glyph, const QTransform &t, glyph_metrics_t *metrics) const;

    const QSharedPointer<QWindowsFontEngineData> &fontEngineData() const { return m_fontEngineData; }

    void setUniqueFamilyName(const QString &newName) { uniqueFamilyName = newName; }

protected:
    void initializeHeightMetrics() const override;

private:
    QWindowsNativeImage *drawGDIGlyph(HFONT font, glyph_t, int margin, const QTransform &xform,
                                      QImage::Format mask_format);
    bool hasCFFTable() const;
    bool hasCMapTable() const;

    const QSharedPointer<QWindowsFontEngineData> m_fontEngineData;

    const QString     _name;
    QString     uniqueFamilyName;
    HFONT       hfont = 0;
    const LOGFONT     m_logfont;
    uint        ttf        : 1;
    uint        hasOutline : 1;
    uint        hasUnreliableOutline : 1;
    uint        cffTable   : 1;
    TEXTMETRIC  tm;
    const unsigned char *cmap = nullptr;
    int cmapSize = 0;
    QByteArray cmapTable;
    mutable qreal lbearing = SHRT_MIN;
    mutable qreal rbearing = SHRT_MIN;
    QFixed designToDevice;
    int unitsPerEm = 0;
    QFixed x_height = -1;
    FaceId _faceId;

    mutable int synthesized_flags = -1;
    mutable QFixed lineWidth = -1;
    mutable unsigned char *widthCache = nullptr;
    mutable uint widthCacheSize = 0;
    mutable QFixed *designAdvances = nullptr;
    mutable int designAdvancesSize = 0;
};

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(HFONT, Q_GUI_EXPORT)
QT_DECL_METATYPE_EXTERN(LOGFONT, Q_GUI_EXPORT)

#endif // QWINDOWSFONTENGINE_H
