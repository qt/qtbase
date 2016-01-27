/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSFONTENGINEDIRECTWRITE_H
#define QWINDOWSFONTENGINEDIRECTWRITE_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_DIRECTWRITE

#include <QtGui/private/qfontengine_p.h>
#include <QtCore/QSharedPointer>

struct IDWriteFont;
struct IDWriteFontFace;
struct IDWriteFactory;
struct IDWriteBitmapRenderTarget;
struct IDWriteGdiInterop;

QT_BEGIN_NAMESPACE

class QWindowsFontEngineData;

class QWindowsFontEngineDirectWrite : public QFontEngine
{
public:
    explicit QWindowsFontEngineDirectWrite(IDWriteFontFace *directWriteFontFace,
                                    qreal pixelSize,
                                    const QSharedPointer<QWindowsFontEngineData> &d);
    ~QWindowsFontEngineDirectWrite();

    void initFontInfo(const QFontDef &request, int dpi, IDWriteFont *font);

    QFixed lineThickness() const Q_DECL_OVERRIDE;
    QFixed underlinePosition() const Q_DECL_OVERRIDE;
    bool getSfntTableData(uint tag, uchar *buffer, uint *length) const Q_DECL_OVERRIDE;
    QFixed emSquareSize() const Q_DECL_OVERRIDE;

    glyph_t glyphIndex(uint ucs4) const Q_DECL_OVERRIDE;
    bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs,
                      ShaperFlags flags) const Q_DECL_OVERRIDE;
    void recalcAdvances(QGlyphLayout *glyphs, ShaperFlags) const Q_DECL_OVERRIDE;

    void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                         QPainterPath *path, QTextItem::RenderFlags flags) Q_DECL_OVERRIDE;

    glyph_metrics_t boundingBox(const QGlyphLayout &glyphs) Q_DECL_OVERRIDE;
    glyph_metrics_t boundingBox(glyph_t g) Q_DECL_OVERRIDE;
    glyph_metrics_t alphaMapBoundingBox(glyph_t glyph, QFixed,
                                        const QTransform &matrix, GlyphFormat) Q_DECL_OVERRIDE;

    QFixed ascent() const Q_DECL_OVERRIDE;
    QFixed descent() const Q_DECL_OVERRIDE;
    QFixed leading() const Q_DECL_OVERRIDE;
    QFixed xHeight() const Q_DECL_OVERRIDE;
    qreal maxCharWidth() const Q_DECL_OVERRIDE;

    bool supportsSubPixelPositions() const Q_DECL_OVERRIDE;

    QImage alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition) Q_DECL_OVERRIDE;
    QImage alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &t) Q_DECL_OVERRIDE;
    QImage alphaRGBMapForGlyph(glyph_t t, QFixed subPixelPosition, const QTransform &xform) Q_DECL_OVERRIDE;

    QFontEngine *cloneWithSize(qreal pixelSize) const Q_DECL_OVERRIDE;

    const QSharedPointer<QWindowsFontEngineData> &fontEngineData() const { return m_fontEngineData; }

    static QString fontNameSubstitute(const QString &familyName);

    IDWriteFontFace *directWriteFontFace() const { return m_directWriteFontFace; }

private:
    QImage imageForGlyph(glyph_t t, QFixed subPixelPosition, int margin, const QTransform &xform);
    void collectMetrics();

    const QSharedPointer<QWindowsFontEngineData> m_fontEngineData;

    IDWriteFontFace *m_directWriteFontFace;
    IDWriteBitmapRenderTarget *m_directWriteBitmapRenderTarget;

    QFixed m_lineThickness;
    QFixed m_underlinePosition;
    int m_unitsPerEm;
    QFixed m_ascent;
    QFixed m_descent;
    QFixed m_xHeight;
    QFixed m_lineGap;
    FaceId m_faceId;
};

QT_END_NAMESPACE

#endif // QT_NO_DIRECTWRITE

#endif // QWINDOWSFONTENGINEDIRECTWRITE_H
