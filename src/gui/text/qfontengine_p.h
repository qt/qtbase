/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QFONTENGINE_P_H
#define QFONTENGINE_P_H

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

#include "QtCore/qglobal.h"
#include "QtCore/qatomic.h"
#include <QtCore/qvarlengtharray.h>
#include <QtCore/QLinkedList>
#include "private/qtextengine_p.h"
#include "private/qfont_p.h"

#include <private/qfontengineglyphcache_p.h>

QT_BEGIN_NAMESPACE

class QPainterPath;

struct QGlyphLayout;

#define MAKE_TAG(ch1, ch2, ch3, ch4) (\
    (((quint32)(ch1)) << 24) | \
    (((quint32)(ch2)) << 16) | \
    (((quint32)(ch3)) << 8) | \
    ((quint32)(ch4)) \
   )

typedef void (*qt_destroy_func_t) (void *user_data);

class Q_GUI_EXPORT QFontEngine : public QObject
{
    Q_OBJECT
public:
    enum Type {
        Box,
        Multi,

        // MS Windows types
        Win,

        // Apple Mac OS types
        Mac,

        // QWS types
        Freetype,
        QPF1,
        QPF2,
        Proxy,

        DirectWrite,

        TestFontEngine = 0x1000
    };

    enum GlyphFormat {
        Format_None,
        Format_Render = Format_None,
        Format_Mono,
        Format_A8,
        Format_A32,
        Format_ARGB
    };

    enum ShaperFlag {
        RightToLeft = 0x0001,
        DesignMetrics = 0x0002,
        GlyphIndicesOnly = 0x0004
    };
    Q_DECLARE_FLAGS(ShaperFlags, ShaperFlag)

    QFontEngine();
    virtual ~QFontEngine();

    // all of these are in unscaled metrics if the engine supports uncsaled metrics,
    // otherwise in design metrics
    struct Properties {
        QByteArray postscriptName;
        QByteArray copyright;
        QRectF boundingBox;
        QFixed emSquare;
        QFixed ascent;
        QFixed descent;
        QFixed leading;
        QFixed italicAngle;
        QFixed capHeight;
        QFixed lineWidth;
    };
    virtual Properties properties() const;
    virtual void getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics);
    QByteArray getSfntTable(uint /*tag*/) const;
    virtual bool getSfntTableData(uint /*tag*/, uchar * /*buffer*/, uint * /*length*/) const { return false; }

    struct FaceId {
        FaceId() : index(0), encoding(0) {}
        QByteArray filename;
        QByteArray uuid;
        int index;
        int encoding;
    };
    virtual FaceId faceId() const { return FaceId(); }
    enum SynthesizedFlags {
        SynthesizedItalic = 0x1,
        SynthesizedBold = 0x2,
        SynthesizedStretch = 0x4
    };
    virtual int synthesized() const { return 0; }
    virtual bool supportsSubPixelPositions() const { return false; }
    virtual QFixed subPixelPositionForX(QFixed x) const;

    virtual QFixed emSquareSize() const { return ascent(); }

    /* returns 0 as glyph index for non existent glyphs */
    virtual bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const = 0;
    virtual void recalcAdvances(QGlyphLayout *, ShaperFlags) const {}
    virtual void doKerning(QGlyphLayout *, ShaperFlags) const;

    virtual void addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                                 QPainterPath *path, QTextItem::RenderFlags flags);

    void getGlyphPositions(const QGlyphLayout &glyphs, const QTransform &matrix, QTextItem::RenderFlags flags,
                           QVarLengthArray<glyph_t> &glyphs_out, QVarLengthArray<QFixedPoint> &positions);

    virtual void addOutlineToPath(qreal, qreal, const QGlyphLayout &, QPainterPath *, QTextItem::RenderFlags flags);
    void addBitmapFontToPath(qreal x, qreal y, const QGlyphLayout &, QPainterPath *, QTextItem::RenderFlags);
    /**
     * Create a qimage with the alpha values for the glyph.
     * Returns an image indexed_8 with index values ranging from 0=fully transparent to 255=opaque
     */
    // ### Refactor this into a smaller and more flexible API.
    virtual QImage alphaMapForGlyph(glyph_t);
    virtual QImage alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition);
    virtual QImage alphaMapForGlyph(glyph_t, const QTransform &t);
    virtual QImage alphaMapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t);
    virtual QImage alphaRGBMapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t);
    virtual QImage bitmapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t);
    virtual QImage *lockedAlphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition,
                                           GlyphFormat neededFormat,
                                           const QTransform &t = QTransform(),
                                           QPoint *offset = 0);
    virtual void unlockAlphaMapForGlyph();
    virtual bool hasInternalCaching() const { return false; }

    virtual glyph_metrics_t alphaMapBoundingBox(glyph_t glyph, QFixed /*subPixelPosition*/, const QTransform &matrix, GlyphFormat /*format*/)
    {
        return boundingBox(glyph, matrix);
    }

    virtual void removeGlyphFromCache(glyph_t);

    virtual glyph_metrics_t boundingBox(const QGlyphLayout &glyphs) = 0;
    virtual glyph_metrics_t boundingBox(glyph_t glyph) = 0;
    virtual glyph_metrics_t boundingBox(glyph_t glyph, const QTransform &matrix);
    glyph_metrics_t tightBoundingBox(const QGlyphLayout &glyphs);

    virtual QFixed ascent() const = 0;
    virtual QFixed descent() const = 0;
    virtual QFixed leading() const = 0;
    virtual QFixed xHeight() const;
    virtual QFixed averageCharWidth() const;

    virtual QFixed lineThickness() const;
    virtual QFixed underlinePosition() const;

    virtual qreal maxCharWidth() const = 0;
    virtual qreal minLeftBearing() const { return qreal(); }
    virtual qreal minRightBearing() const { return qreal(); }

    virtual void getGlyphBearings(glyph_t glyph, qreal *leftBearing = 0, qreal *rightBearing = 0);

    virtual const char *name() const = 0;

    virtual bool canRender(const QChar *string, int len) = 0;
    inline bool canRender(uint ucs4) {
        QChar utf16[2];
        int utf16len = 1;
        if (QChar::requiresSurrogates(ucs4)) {
            utf16[0] = QChar::highSurrogate(ucs4);
            utf16[1] = QChar::lowSurrogate(ucs4);
            ++utf16len;
        } else {
            utf16[0] = QChar(ucs4);
        }
        return canRender(utf16, utf16len);
    }

    virtual bool supportsTransformation(const QTransform &transform) const;

    virtual Type type() const = 0;

    virtual int glyphCount() const;
    virtual int glyphMargin(QFontEngineGlyphCache::Type type) { return type == QFontEngineGlyphCache::Raster_RGBMask ? 2 : 0; }

    virtual QFontEngine *cloneWithSize(qreal /*pixelSize*/) const { return 0; }

    void *harfbuzzFont() const;
    void *harfbuzzFace() const;
    bool supportsScript(QChar::Script script) const;

    virtual int getPointInOutline(glyph_t glyph, int flags, quint32 point, QFixed *xpos, QFixed *ypos, quint32 *nPoints);

    void setGlyphCache(const void *key, QFontEngineGlyphCache *data);
    QFontEngineGlyphCache *glyphCache(const void *key, QFontEngineGlyphCache::Type type, const QTransform &transform) const;

    static const uchar *getCMap(const uchar *table, uint tableSize, bool *isSymbolFont, int *cmapSize);
    static quint32 getTrueTypeGlyphIndex(const uchar *cmap, uint unicode);

    static QByteArray convertToPostscriptFontFamilyName(const QByteArray &fontFamily);

    enum HintStyle {
        HintNone,
        HintLight,
        HintMedium,
        HintFull
    };
    virtual void setDefaultHintStyle(HintStyle) { }

    QAtomicInt ref;
    QFontDef fontDef;

    mutable void *font_;
    mutable qt_destroy_func_t font_destroy_func;
    mutable void *face_;
    mutable qt_destroy_func_t face_destroy_func;

    uint cache_cost; // amount of mem used in kb by the font
    uint fsType : 16;
    bool symbol;
    struct KernPair {
        uint left_right;
        QFixed adjust;

        inline bool operator<(const KernPair &other) const
        {
            return left_right < other.left_right;
        }
    };
    QVector<KernPair> kerning_pairs;
    void loadKerningPairs(QFixed scalingFactor);

    int glyphFormat;
    QImage currentlyLockedAlphaMap;
    int m_subPixelPositionCount; // Number of positions within a single pixel for this cache

protected:
    QFixed lastRightBearing(const QGlyphLayout &glyphs, bool round = false);

private:
    struct GlyphCacheEntry {
        const void *context;
        QExplicitlySharedDataPointer<QFontEngineGlyphCache> cache;
        bool operator==(const GlyphCacheEntry &other) const { return context == other.context && cache == other.cache; }
    };

    mutable QLinkedList<GlyphCacheEntry> m_glyphCaches;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFontEngine::ShaperFlags)

inline bool operator ==(const QFontEngine::FaceId &f1, const QFontEngine::FaceId &f2)
{
    return (f1.index == f2.index) && (f1.encoding == f2.encoding) && (f1.filename == f2.filename);
}

inline uint qHash(const QFontEngine::FaceId &f)
{
    return qHash((f.index << 16) + f.encoding) + qHash(f.filename + f.uuid);
}


class QGlyph;



class QFontEngineBox : public QFontEngine
{
public:
    QFontEngineBox(int size);
    ~QFontEngineBox();

    virtual bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const;
    virtual void recalcAdvances(QGlyphLayout *, ShaperFlags) const;

    void draw(QPaintEngine *p, qreal x, qreal y, const QTextItemInt &si);
    virtual void addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags);

    virtual glyph_metrics_t boundingBox(const QGlyphLayout &glyphs);
    virtual glyph_metrics_t boundingBox(glyph_t glyph);
    virtual QFontEngine *cloneWithSize(qreal pixelSize) const;

    virtual QFixed ascent() const;
    virtual QFixed descent() const;
    virtual QFixed leading() const;
    virtual qreal maxCharWidth() const;
    virtual qreal minLeftBearing() const { return 0; }
    virtual qreal minRightBearing() const { return 0; }
    virtual QImage alphaMapForGlyph(glyph_t);

    virtual const char *name() const;

    virtual bool canRender(const QChar *string, int len);

    virtual Type type() const;
    inline int size() const { return _size; }

private:
    friend class QFontPrivate;
    int _size;
};

class Q_GUI_EXPORT QFontEngineMulti : public QFontEngine
{
    Q_OBJECT
public:
    explicit QFontEngineMulti(int engineCount);
    ~QFontEngineMulti();

    virtual bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const;

    virtual glyph_metrics_t boundingBox(const QGlyphLayout &glyphs);
    virtual glyph_metrics_t boundingBox(glyph_t glyph);

    virtual void recalcAdvances(QGlyphLayout *, ShaperFlags) const;
    virtual void doKerning(QGlyphLayout *, ShaperFlags) const;
    virtual void addOutlineToPath(qreal, qreal, const QGlyphLayout &, QPainterPath *, QTextItem::RenderFlags flags);
    virtual void getGlyphBearings(glyph_t glyph, qreal *leftBearing = 0, qreal *rightBearing = 0);

    virtual QFixed ascent() const;
    virtual QFixed descent() const;
    virtual QFixed leading() const;
    virtual QFixed xHeight() const;
    virtual QFixed averageCharWidth() const;
    virtual QImage alphaMapForGlyph(glyph_t);
    virtual QImage alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition);
    virtual QImage alphaMapForGlyph(glyph_t, const QTransform &t);
    virtual QImage alphaMapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t);
    virtual QImage alphaRGBMapForGlyph(glyph_t, QFixed subPixelPosition, const QTransform &t);

    virtual QFixed lineThickness() const;
    virtual QFixed underlinePosition() const;
    virtual qreal maxCharWidth() const;
    virtual qreal minLeftBearing() const;
    virtual qreal minRightBearing() const;

    virtual inline Type type() const
    { return QFontEngine::Multi; }

    virtual bool canRender(const QChar *string, int len);
    inline virtual const char *name() const
    { return "Multi"; }

    QFontEngine *engine(int at) const
    {Q_ASSERT(at < engines.size()); return engines.at(at); }

    inline void ensureEngineAt(int at)
    {
        if (at >= engines.size() || engines.at(at) == 0)
            loadEngine(at);
    }

    virtual bool shouldLoadFontEngineForCharacter(int at, uint ucs4) const;
    virtual void setFallbackFamiliesList(const QStringList &) {}

protected:
    friend class QRawFont;
    virtual void loadEngine(int at) = 0;
    virtual void ensureFallbackFamiliesQueried() {}
    QVector<QFontEngine *> engines;
};

class QTestFontEngine : public QFontEngineBox
{
public:
    QTestFontEngine(int size) : QFontEngineBox(size) {}
    virtual Type type() const { return TestFontEngine; }
};

QT_END_NAMESPACE



#endif // QFONTENGINE_P_H
