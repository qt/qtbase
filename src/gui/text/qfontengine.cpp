// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <qdebug.h>
#include <private/qfontengine_p.h>
#include <private/qfontengineglyphcache_p.h>
#include <private/qguiapplication_p.h>

#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformintegration.h>

#include "qbitmap.h"
#include "qpainter.h"
#include "qpainterpath.h"
#include "qvarlengtharray.h"
#include "qtextengine_p.h"
#include <qmath.h>
#include <qendian.h>
#include <private/qstringiterator_p.h>

#if QT_CONFIG(harfbuzz)
#  include "qharfbuzzng_p.h"
#  include <hb-ot.h>
#endif

#include <algorithm>
#include <limits.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcColrv1, "qt.text.font.colrv1")

using namespace Qt::StringLiterals;

static inline bool qtransform_equals_no_translate(const QTransform &a, const QTransform &b)
{
    if (a.type() <= QTransform::TxTranslate && b.type() <= QTransform::TxTranslate) {
        return true;
    } else {
        // We always use paths for perspective text anyway, so no
        // point in checking the full matrix...
        Q_ASSERT(a.type() < QTransform::TxProject);
        Q_ASSERT(b.type() < QTransform::TxProject);

        return a.m11() == b.m11()
            && a.m12() == b.m12()
            && a.m21() == b.m21()
            && a.m22() == b.m22();
    }
}

template<typename T>
static inline bool qSafeFromBigEndian(const uchar *source, const uchar *end, T *output)
{
    if (source + sizeof(T) > end)
        return false;

    *output = qFromBigEndian<T>(source);
    return true;
}

int QFontEngine::getPointInOutline(glyph_t glyph, int flags, quint32 point, QFixed *xpos, QFixed *ypos, quint32 *nPoints)
{
    Q_UNUSED(glyph);
    Q_UNUSED(flags);
    Q_UNUSED(point);
    Q_UNUSED(xpos);
    Q_UNUSED(ypos);
    Q_UNUSED(nPoints);
    return Err_Not_Covered;
}

static bool qt_get_font_table_default(void *user_data, uint tag, uchar *buffer, uint *length)
{
    QFontEngine *fe = (QFontEngine *)user_data;
    return fe->getSfntTableData(tag, buffer, length);
}


#ifdef QT_BUILD_INTERNAL
// for testing purpose only, not thread-safe!
static QList<QFontEngine *> *enginesCollector = nullptr;

Q_AUTOTEST_EXPORT void QFontEngine_startCollectingEngines()
{
    delete enginesCollector;
    enginesCollector = new QList<QFontEngine *>();
}

Q_AUTOTEST_EXPORT QList<QFontEngine *> QFontEngine_stopCollectingEngines()
{
    Q_ASSERT(enginesCollector);
    QList<QFontEngine *> ret = *enginesCollector;
    delete enginesCollector;
    enginesCollector = nullptr;
    return ret;
}
#endif // QT_BUILD_INTERNAL


// QFontEngine

#define kBearingNotInitialized std::numeric_limits<qreal>::max()

QFontEngine::QFontEngine(Type type)
    : m_type(type), ref(0),
      font_(),
      face_(),
      m_heightMetricsQueried(false),
      m_minLeftBearing(kBearingNotInitialized),
      m_minRightBearing(kBearingNotInitialized)
{
    faceData.user_data = this;
    faceData.get_font_table = qt_get_font_table_default;

    cache_cost = 0;
    fsType = 0;
    symbol = false;
    isSmoothlyScalable = false;

    glyphFormat = Format_None;
    m_subPixelPositionCount = 0;

#ifdef QT_BUILD_INTERNAL
    if (enginesCollector)
        enginesCollector->append(this);
#endif
}

QFontEngine::~QFontEngine()
{
#ifdef QT_BUILD_INTERNAL
    if (enginesCollector)
        enginesCollector->removeOne(this);
#endif
}

QFixed QFontEngine::lineThickness() const
{
    // ad hoc algorithm
    int score = fontDef.weight * fontDef.pixelSize / 10;
    int lw = score / 700;

    // looks better with thicker line for small pointsizes
    if (lw < 2 && score >= 1050) lw = 2;
    if (lw == 0) lw = 1;

    return lw;
}

QFixed QFontEngine::underlinePosition() const
{
    return ((lineThickness() * 2) + 3) / 6;
}

void *QFontEngine::harfbuzzFont() const
{
    Q_ASSERT(type() != QFontEngine::Multi);
#if QT_CONFIG(harfbuzz)
    return hb_qt_font_get_for_engine(const_cast<QFontEngine *>(this));
#else
    return nullptr;
#endif
}

void *QFontEngine::harfbuzzFace() const
{
    Q_ASSERT(type() != QFontEngine::Multi);
#if QT_CONFIG(harfbuzz)
     return hb_qt_face_get_for_engine(const_cast<QFontEngine *>(this));
#else
    return nullptr;
#endif
}

bool QFontEngine::supportsScript(QChar::Script script) const
{
    if (type() <= QFontEngine::Multi)
        return true;

    // ### TODO: This only works for scripts that require OpenType. More generally
    // for scripts that do not require OpenType we should just look at the list of
    // supported writing systems in the font's OS/2 table.
    if (!scriptRequiresOpenType(script))
        return true;

#if QT_CONFIG(harfbuzz)
    // in AAT fonts, 'gsub' table is effectively replaced by 'mort'/'morx' table
    uint lenMort = 0, lenMorx = 0;
    if (getSfntTableData(QFont::Tag("mort").value(), nullptr, &lenMort)
     || getSfntTableData(QFont::Tag("morx").value(), nullptr, &lenMorx)) {
        return true;
    }

    if (hb_face_t *face = hb_qt_face_get_for_engine(const_cast<QFontEngine *>(this))) {
        unsigned int script_count = HB_OT_MAX_TAGS_PER_SCRIPT;
        hb_tag_t script_tags[HB_OT_MAX_TAGS_PER_SCRIPT];

        hb_ot_tags_from_script_and_language(hb_qt_script_to_script(script), HB_LANGUAGE_INVALID,
                                            &script_count, script_tags,
                                            nullptr, nullptr);

        if (hb_ot_layout_table_select_script(face, HB_OT_TAG_GSUB, script_count, script_tags, nullptr, nullptr))
            return true;
    }
#endif
    return false;
}

bool QFontEngine::canRender(const QChar *str, int len) const
{
    QStringIterator it(str, str + len);
    while (it.hasNext()) {
        if (glyphIndex(it.next()) == 0)
            return false;
    }

    return true;
}

glyph_metrics_t QFontEngine::boundingBox(glyph_t glyph, const QTransform &matrix)
{
    glyph_metrics_t metrics = boundingBox(glyph);

    if (matrix.type() > QTransform::TxTranslate) {
        return metrics.transformed(matrix);
    }
    return metrics;
}

QFixed QFontEngine::calculatedCapHeight() const
{
    const glyph_t glyph = glyphIndex('H');
    glyph_metrics_t bb = const_cast<QFontEngine *>(this)->boundingBox(glyph);
    return bb.height;
}

QFixed QFontEngine::xHeight() const
{
    const glyph_t glyph = glyphIndex('x');
    glyph_metrics_t bb = const_cast<QFontEngine *>(this)->boundingBox(glyph);
    return bb.height;
}

QFixed QFontEngine::averageCharWidth() const
{
    const glyph_t glyph = glyphIndex('x');
    glyph_metrics_t bb = const_cast<QFontEngine *>(this)->boundingBox(glyph);
    return bb.xoff;
}

bool QFontEngine::supportsTransformation(const QTransform &transform) const
{
    return transform.type() < QTransform::TxProject;
}

bool QFontEngine::expectsGammaCorrectedBlending() const
{
    return true;
}

void QFontEngine::getGlyphPositions(const QGlyphLayout &glyphs, const QTransform &matrix, QTextItem::RenderFlags flags,
                                    QVarLengthArray<glyph_t> &glyphs_out, QVarLengthArray<QFixedPoint> &positions)
{
    QFixed xpos;
    QFixed ypos;

    const bool transform = matrix.m11() != 1.
                           || matrix.m12() != 0.
                           || matrix.m21() != 0.
                           || matrix.m22() != 1.;
    if (!transform) {
        xpos = QFixed::fromReal(matrix.dx());
        ypos = QFixed::fromReal(matrix.dy());
    }

    int current = 0;
    if (flags & QTextItem::RightToLeft) {
        int i = glyphs.numGlyphs;
        int totalKashidas = 0;
        while(i--) {
            if (glyphs.attributes[i].dontPrint)
                continue;
            xpos += glyphs.advances[i] + QFixed::fromFixed(glyphs.justifications[i].space_18d6);
            totalKashidas += glyphs.justifications[i].nKashidas;
        }
        positions.resize(glyphs.numGlyphs+totalKashidas);
        glyphs_out.resize(glyphs.numGlyphs+totalKashidas);

        i = 0;
        while(i < glyphs.numGlyphs) {
            if (glyphs.attributes[i].dontPrint) {
                ++i;
                continue;
            }
            xpos -= glyphs.advances[i];

            QFixed gpos_x = xpos + glyphs.offsets[i].x;
            QFixed gpos_y = ypos + glyphs.offsets[i].y;
            if (transform) {
                QPointF gpos(gpos_x.toReal(), gpos_y.toReal());
                gpos = gpos * matrix;
                gpos_x = QFixed::fromReal(gpos.x());
                gpos_y = QFixed::fromReal(gpos.y());
            }
            positions[current].x = gpos_x;
            positions[current].y = gpos_y;
            glyphs_out[current] = glyphs.glyphs[i];
            ++current;
            if (glyphs.justifications[i].nKashidas) {
                QChar ch = u'\x640'; // Kashida character

                glyph_t kashidaGlyph = glyphIndex(ch.unicode());
                QFixed kashidaWidth;

                QGlyphLayout g;
                g.numGlyphs = 1;
                g.glyphs = &kashidaGlyph;
                g.advances = &kashidaWidth;
                recalcAdvances(&g, { });

                for (uint k = 0; k < glyphs.justifications[i].nKashidas; ++k) {
                    xpos -= kashidaWidth;

                    QFixed gpos_x = xpos + glyphs.offsets[i].x;
                    QFixed gpos_y = ypos + glyphs.offsets[i].y;
                    if (transform) {
                        QPointF gpos(gpos_x.toReal(), gpos_y.toReal());
                        gpos = gpos * matrix;
                        gpos_x = QFixed::fromReal(gpos.x());
                        gpos_y = QFixed::fromReal(gpos.y());
                    }
                    positions[current].x = gpos_x;
                    positions[current].y = gpos_y;
                    glyphs_out[current] = kashidaGlyph;
                    ++current;
                }
            } else {
                xpos -= QFixed::fromFixed(glyphs.justifications[i].space_18d6);
            }
            ++i;
        }
    } else {
        positions.resize(glyphs.numGlyphs);
        glyphs_out.resize(glyphs.numGlyphs);
        int i = 0;
        if (!transform) {
            while (i < glyphs.numGlyphs) {
                if (!glyphs.attributes[i].dontPrint) {
                    positions[current].x = xpos + glyphs.offsets[i].x;
                    positions[current].y = ypos + glyphs.offsets[i].y;
                    glyphs_out[current] = glyphs.glyphs[i];
                    xpos += glyphs.advances[i] + QFixed::fromFixed(glyphs.justifications[i].space_18d6);
                    ++current;
                }
                ++i;
            }
        } else {
            while (i < glyphs.numGlyphs) {
                if (!glyphs.attributes[i].dontPrint) {
                    QFixed gpos_x = xpos + glyphs.offsets[i].x;
                    QFixed gpos_y = ypos + glyphs.offsets[i].y;
                    QPointF gpos(gpos_x.toReal(), gpos_y.toReal());
                    gpos = gpos * matrix;
                    positions[current].x = QFixed::fromReal(gpos.x());
                    positions[current].y = QFixed::fromReal(gpos.y());
                    glyphs_out[current] = glyphs.glyphs[i];
                    xpos += glyphs.advances[i] + QFixed::fromFixed(glyphs.justifications[i].space_18d6);
                    ++current;
                }
                ++i;
            }
        }
    }
    positions.resize(current);
    glyphs_out.resize(current);
    Q_ASSERT(positions.size() == glyphs_out.size());
}

void QFontEngine::getGlyphBearings(glyph_t glyph, qreal *leftBearing, qreal *rightBearing)
{
    glyph_metrics_t gi = boundingBox(glyph);
    if (leftBearing != nullptr)
        *leftBearing = gi.leftBearing().toReal();
    if (rightBearing != nullptr)
        *rightBearing = gi.rightBearing().toReal();
}

bool QFontEngine::processHheaTable() const
{
    QByteArray hhea = getSfntTable(QFont::Tag("hhea").value());
    if (hhea.size() >= 10) {
        auto ptr = hhea.constData();
        qint16 ascent = qFromBigEndian<qint16>(ptr + 4);
        qint16 descent = qFromBigEndian<qint16>(ptr + 6);
        qint16 leading = qFromBigEndian<qint16>(ptr + 8);

        // Some fonts may have invalid HHEA data. We detect this and bail out.
        if (ascent == 0 && descent == 0)
            return false;

        const qreal unitsPerEm = emSquareSize().toReal();
        // Bail out if values are too large for QFixed
        const auto limitForQFixed = std::numeric_limits<int>::max() / (fontDef.pixelSize * 64);
        if (ascent > limitForQFixed || descent > limitForQFixed || leading > limitForQFixed)
            return false;
        m_ascent = QFixed::fromReal(ascent * fontDef.pixelSize / unitsPerEm);
        m_descent = -QFixed::fromReal(descent * fontDef.pixelSize / unitsPerEm);
        m_leading = QFixed::fromReal(leading * fontDef.pixelSize / unitsPerEm);

        return true;
    }

    return false;
}

void QFontEngine::initializeHeightMetrics() const
{
    bool hasEmbeddedBitmaps =
            !getSfntTable(QFont::Tag("EBLC").value()).isEmpty()
            || !getSfntTable(QFont::Tag("CBLC").value()).isEmpty()
            || !getSfntTable(QFont::Tag("bdat").value()).isEmpty();

    // When porting applications from Qt 5 to Qt 6, users have noticed differences in line
    // metrics due to the effort to consolidate these across platforms instead of using the
    // system values directly. This environment variable gives a "last resort" for those users
    // to tell Qt to prefer the metrics we get from the system, despite the fact these being
    // inconsistent across platforms.
    static bool useSystemLineMetrics = qEnvironmentVariableIntValue("QT_USE_SYSTEM_LINE_METRICS") > 0;
    if (!hasEmbeddedBitmaps && !useSystemLineMetrics) {
        // Get HHEA table values if available
        processHheaTable();

        // Allow OS/2 metrics to override if present
        processOS2Table();

        if (!supportsSubPixelPositions()) {
            const QFixed actualHeight = m_ascent + m_descent + m_leading;
            m_ascent = m_ascent.round();
            m_descent = m_descent.round();
            m_leading = actualHeight.round() - m_ascent - m_descent;
        }
    }

    m_heightMetricsQueried = true;
}

bool QFontEngine::preferTypoLineMetrics() const
{
    return (fontDef.styleStrategy & QFont::PreferTypoLineMetrics) != 0;
}

bool QFontEngine::processOS2Table() const
{
    QByteArray os2 = getSfntTable(QFont::Tag("OS/2").value());
    if (os2.size() >= 78) {
        auto ptr = os2.constData();
        quint16 fsSelection = qFromBigEndian<quint16>(ptr + 62);
        qint16 typoAscent = qFromBigEndian<qint16>(ptr + 68);
        qint16 typoDescent = qFromBigEndian<qint16>(ptr + 70);
        qint16 typoLineGap = qFromBigEndian<qint16>(ptr + 72);
        quint16 winAscent = qFromBigEndian<quint16>(ptr + 74);
        quint16 winDescent = qFromBigEndian<quint16>(ptr + 76);

        enum { USE_TYPO_METRICS = 0x80 };
        const qreal unitsPerEm = emSquareSize().toReal();
        if (preferTypoLineMetrics() || fsSelection & USE_TYPO_METRICS) {
            // Some fonts may have invalid OS/2 data. We detect this and bail out.
            if (typoAscent == 0 && typoDescent == 0)
                return false;
            // Bail out if values are too large for QFixed
            const auto limitForQFixed = std::numeric_limits<int>::max() / (fontDef.pixelSize * 64);
            if (typoAscent > limitForQFixed || typoDescent > limitForQFixed
                    || typoLineGap > limitForQFixed)
                return false;
            m_ascent = QFixed::fromReal(typoAscent * fontDef.pixelSize / unitsPerEm);
            m_descent = -QFixed::fromReal(typoDescent * fontDef.pixelSize / unitsPerEm);
            m_leading = QFixed::fromReal(typoLineGap * fontDef.pixelSize / unitsPerEm);
        } else {
            // Some fonts may have invalid OS/2 data. We detect this and bail out.
            if (winAscent == 0 && winDescent == 0)
                return false;
            const auto limitForQFixed = std::numeric_limits<int>::max() / (fontDef.pixelSize * 64);
            if (winAscent > limitForQFixed || winDescent > limitForQFixed)
                return false;
            m_ascent = QFixed::fromReal(winAscent * fontDef.pixelSize / unitsPerEm);
            m_descent = QFixed::fromReal(winDescent * fontDef.pixelSize / unitsPerEm);
            m_leading = QFixed{};
        }

        return true;
    }

    return false;
}

QFixed QFontEngine::leading() const
{
    if (!m_heightMetricsQueried)
        initializeHeightMetrics();

    return m_leading;
}


QFixed QFontEngine::emSquareSize() const
{
    qCWarning(lcQpaFonts) << "Font engine does not reimplement emSquareSize(). Returning minimum value.";
    return 16;
}

QFixed QFontEngine::ascent() const
{
    if (!m_heightMetricsQueried)
        initializeHeightMetrics();

    return m_ascent;
}

QFixed QFontEngine::descent() const
{
    if (!m_heightMetricsQueried)
        initializeHeightMetrics();

    return m_descent;
}

qreal QFontEngine::minLeftBearing() const
{
    if (m_minLeftBearing == kBearingNotInitialized)
        minRightBearing(); // Initializes both (see below)

    return m_minLeftBearing;
}

#define q16Dot16ToFloat(i) ((i) / 65536.0)

#define kMinLeftSideBearingOffset 12
#define kMinRightSideBearingOffset 14

qreal QFontEngine::minRightBearing() const
{
    if (m_minRightBearing == kBearingNotInitialized) {

        // Try the 'hhea' font table first, which covers the entire font
        QByteArray hheaTable = getSfntTable(QFont::Tag("hhea").value());
        if (hheaTable.size() >= int(kMinRightSideBearingOffset + sizeof(qint16))) {
            const uchar *tableData = reinterpret_cast<const uchar *>(hheaTable.constData());
            Q_ASSERT(q16Dot16ToFloat(qFromBigEndian<quint32>(tableData)) == 1.0);

            qint16 minLeftSideBearing = qFromBigEndian<qint16>(tableData + kMinLeftSideBearingOffset);
            qint16 minRightSideBearing = qFromBigEndian<qint16>(tableData + kMinRightSideBearingOffset);

            // The table data is expressed as FUnits, meaning we have to take the number
            // of units per em into account. Since pixelSize already has taken DPI into
            // account we can use that directly instead of the point size.
            int unitsPerEm = emSquareSize().toInt();
            qreal funitToPixelFactor = fontDef.pixelSize / unitsPerEm;

            // Some fonts on OS X (such as Gurmukhi Sangam MN, Khmer MN, Lao Sangam MN, etc.), have
            // invalid values for their NBSPACE left bearing, causing the 'hhea' minimum bearings to
            // be way off. We detect this by assuming that the minimum bearsings are within a certain
            // range of the em square size.
            static const int largestValidBearing = 4 * unitsPerEm;

            if (qAbs(minLeftSideBearing) < largestValidBearing)
                m_minLeftBearing = minLeftSideBearing * funitToPixelFactor;
            if (qAbs(minRightSideBearing) < largestValidBearing)
                m_minRightBearing = minRightSideBearing * funitToPixelFactor;
        }

        // Fallback in case of missing 'hhea' table (bitmap fonts e.g.) or broken 'hhea' values
        if (m_minLeftBearing == kBearingNotInitialized || m_minRightBearing == kBearingNotInitialized) {

            // To balance performance and correctness we only look at a subset of the
            // possible glyphs in the font, based on which characters are more likely
            // to have a left or right bearing.
            static const ushort characterSubset[] = {
                '(', 'C', 'F', 'K', 'V', 'X', 'Y', ']', '_', 'f', 'r', '|',
                127, 205, 645, 884, 922, 1070, 12386
            };

            // The font may have minimum bearings larger than 0, so we have to start at the max
            m_minLeftBearing = m_minRightBearing = std::numeric_limits<qreal>::max();

            for (uint i = 0; i < (sizeof(characterSubset) / sizeof(ushort)); ++i) {
                const glyph_t glyph = glyphIndex(characterSubset[i]);
                if (!glyph)
                    continue;

                glyph_metrics_t glyphMetrics = const_cast<QFontEngine *>(this)->boundingBox(glyph);

                // Glyphs with no contours shouldn't contribute to bearings
                if (!glyphMetrics.width || !glyphMetrics.height)
                    continue;

                m_minLeftBearing = qMin(m_minLeftBearing, glyphMetrics.leftBearing().toReal());
                m_minRightBearing = qMin(m_minRightBearing, glyphMetrics.rightBearing().toReal());
            }
        }

        if (m_minLeftBearing == kBearingNotInitialized || m_minRightBearing == kBearingNotInitialized)
            qWarning() << "Failed to compute left/right minimum bearings for"
                       << fontDef.families.first();
    }

    return m_minRightBearing;
}

glyph_metrics_t QFontEngine::boundingBox(const QGlyphLayout &glyphs)
{
    QFixed w;
    for (int i = 0; i < glyphs.numGlyphs; ++i)
        w += glyphs.effectiveAdvance(i);
    const QFixed leftBearing = firstLeftBearing(glyphs);
    const QFixed rightBearing = lastRightBearing(glyphs);
    return glyph_metrics_t(leftBearing, -(ascent()), w - leftBearing - rightBearing, ascent() + descent(), w, 0);
}

glyph_metrics_t QFontEngine::tightBoundingBox(const QGlyphLayout &glyphs)
{
    glyph_metrics_t overall;

    QFixed ymax = 0;
    QFixed xmax = 0;
    for (int i = 0; i < glyphs.numGlyphs; i++) {
        // If shaping has found this should be ignored, ignore it.
        if (!glyphs.advances[i] || glyphs.attributes[i].dontPrint)
            continue;
        glyph_metrics_t bb = boundingBox(glyphs.glyphs[i]);
        QFixed x = overall.xoff + glyphs.offsets[i].x + bb.x;
        QFixed y = overall.yoff + glyphs.offsets[i].y + bb.y;
        overall.x = qMin(overall.x, x);
        overall.y = qMin(overall.y, y);
        xmax = qMax(xmax, x.ceil() + bb.width);
        ymax = qMax(ymax, y.ceil() + bb.height);
        overall.xoff += glyphs.effectiveAdvance(i);
        overall.yoff += bb.yoff;
    }
    overall.height = qMax(overall.height, ymax - overall.y);
    overall.width = xmax - overall.x;

    return overall;
}


void QFontEngine::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path,
                                   QTextItem::RenderFlags flags)
{
    if (!glyphs.numGlyphs)
        return;

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> positioned_glyphs;
    QTransform matrix = QTransform::fromTranslate(x, y);
    getGlyphPositions(glyphs, matrix, flags, positioned_glyphs, positions);
    addGlyphsToPath(positioned_glyphs.data(), positions.data(), positioned_glyphs.size(), path, flags);
}

#define GRID(x, y) grid[(y)*(w+1) + (x)]
#define SET(x, y) (*(image_data + (y)*bpl + ((x) >> 3)) & (0x80 >> ((x) & 7)))

enum { EdgeRight = 0x1,
       EdgeDown = 0x2,
       EdgeLeft = 0x4,
       EdgeUp = 0x8
};

static void collectSingleContour(qreal x0, qreal y0, uint *grid, int x, int y, int w, int h, QPainterPath *path)
{
    Q_UNUSED(h);

    path->moveTo(x + x0, y + y0);
    while (GRID(x, y)) {
        if (GRID(x, y) & EdgeRight) {
            while (GRID(x, y) & EdgeRight) {
                GRID(x, y) &= ~EdgeRight;
                ++x;
            }
            Q_ASSERT(x <= w);
            path->lineTo(x + x0, y + y0);
            continue;
        }
        if (GRID(x, y) & EdgeDown) {
            while (GRID(x, y) & EdgeDown) {
                GRID(x, y) &= ~EdgeDown;
                ++y;
            }
            Q_ASSERT(y <= h);
            path->lineTo(x + x0, y + y0);
            continue;
        }
        if (GRID(x, y) & EdgeLeft) {
            while (GRID(x, y) & EdgeLeft) {
                GRID(x, y) &= ~EdgeLeft;
                --x;
            }
            Q_ASSERT(x >= 0);
            path->lineTo(x + x0, y + y0);
            continue;
        }
        if (GRID(x, y) & EdgeUp) {
            while (GRID(x, y) & EdgeUp) {
                GRID(x, y) &= ~EdgeUp;
                --y;
            }
            Q_ASSERT(y >= 0);
            path->lineTo(x + x0, y + y0);
            continue;
        }
    }
    path->closeSubpath();
}

Q_GUI_EXPORT void qt_addBitmapToPath(qreal x0, qreal y0, const uchar *image_data, int bpl, int w, int h, QPainterPath *path)
{
    uint *grid = new uint[(w+1)*(h+1)];
    // set up edges
    for (int y = 0; y <= h; ++y) {
        for (int x = 0; x <= w; ++x) {
            bool topLeft = (x == 0 || y == 0) ? false : SET(x - 1, y - 1);
            bool topRight = (x == w || y == 0) ? false : SET(x, y - 1);
            bool bottomLeft = (x == 0 || y == h) ? false : SET(x - 1, y);
            bool bottomRight = (x == w || y == h) ? false : SET(x, y);

            GRID(x, y) = 0;
            if ((!topRight) & bottomRight)
                GRID(x, y) |= EdgeRight;
            if ((!bottomRight) & bottomLeft)
                GRID(x, y) |= EdgeDown;
            if ((!bottomLeft) & topLeft)
                GRID(x, y) |= EdgeLeft;
            if ((!topLeft) & topRight)
                GRID(x, y) |= EdgeUp;
        }
    }

    // collect edges
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (!GRID(x, y))
                continue;
            // found start of a contour, follow it
            collectSingleContour(x0, y0, grid, x, y, w, h, path);
        }
    }
    delete [] grid;
}

#undef GRID
#undef SET


void QFontEngine::addBitmapFontToPath(qreal x, qreal y, const QGlyphLayout &glyphs,
                                      QPainterPath *path, QTextItem::RenderFlags flags)
{
// TODO what to do with 'flags' ??
    Q_UNUSED(flags);
    QFixed advanceX = QFixed::fromReal(x);
    QFixed advanceY = QFixed::fromReal(y);
    for (int i=0; i < glyphs.numGlyphs; ++i) {
        glyph_metrics_t metrics = boundingBox(glyphs.glyphs[i]);
        if (metrics.width.value() == 0 || metrics.height.value() == 0) {
            advanceX += glyphs.advances[i];
            continue;
        }
        const QImage alphaMask = alphaMapForGlyph(glyphs.glyphs[i]);

        const int w = alphaMask.width();
        const int h = alphaMask.height();
        const qsizetype srcBpl = alphaMask.bytesPerLine();
        QImage bitmap;
        if (alphaMask.depth() == 1) {
            bitmap = alphaMask;
        } else {
            bitmap = QImage(w, h, QImage::Format_Mono);
            const uchar *imageData = alphaMask.bits();
            const qsizetype destBpl = bitmap.bytesPerLine();
            uchar *bitmapData = bitmap.bits();

            for (int yi = 0; yi < h; ++yi) {
                const uchar *src = imageData + yi*srcBpl;
                uchar *dst = bitmapData + yi*destBpl;
                for (int xi = 0; xi < w; ++xi) {
                    const int byte = xi / 8;
                    const int bit = xi % 8;
                    if (bit == 0)
                        dst[byte] = 0;
                    if (src[xi])
                        dst[byte] |= 128 >> bit;
                }
            }
        }
        const uchar *bitmap_data = bitmap.constBits();
        QFixedPoint offset = glyphs.offsets[i];
        advanceX += offset.x;
        advanceY += offset.y;
        qt_addBitmapToPath((advanceX + metrics.x).toReal(), (advanceY + metrics.y).toReal(), bitmap_data, bitmap.bytesPerLine(), w, h, path);
        advanceX += glyphs.advances[i];
    }
}

void QFontEngine::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nGlyphs,
                                  QPainterPath *path, QTextItem::RenderFlags flags)
{
    qreal x = positions[0].x.toReal();
    qreal y = positions[0].y.toReal();
    QVarLengthGlyphLayoutArray g(nGlyphs);

    for (int i = 0; i < nGlyphs - 1; ++i) {
        g.glyphs[i] = glyphs[i];
        g.advances[i] = positions[i + 1].x - positions[i].x;
    }
    g.glyphs[nGlyphs - 1] = glyphs[nGlyphs - 1];
    g.advances[nGlyphs - 1] = QFixed::fromReal(maxCharWidth());

    addBitmapFontToPath(x, y, g, path, flags);
}

QImage QFontEngine::alphaMapForGlyph(glyph_t glyph, const QFixedPoint &/*subPixelPosition*/)
{
    // For font engines don't support subpixel positioning
    return alphaMapForGlyph(glyph);
}

QImage QFontEngine::alphaMapForGlyph(glyph_t glyph, const QTransform &t)
{
    QImage i = alphaMapForGlyph(glyph);
    if (t.type() > QTransform::TxTranslate)
        i = i.transformed(t).convertToFormat(QImage::Format_Alpha8);
    Q_ASSERT(i.depth() <= 8); // To verify that transformed didn't change the format...

    return i;
}

QImage QFontEngine::alphaMapForGlyph(glyph_t glyph, const QFixedPoint &subPixelPosition, const QTransform &t)
{
    if (!supportsHorizontalSubPixelPositions() && !supportsVerticalSubPixelPositions())
        return alphaMapForGlyph(glyph, t);

    QImage i = alphaMapForGlyph(glyph, subPixelPosition);
    if (t.type() > QTransform::TxTranslate)
        i = i.transformed(t).convertToFormat(QImage::Format_Alpha8);
    Q_ASSERT(i.depth() <= 8); // To verify that transformed didn't change the format...

    return i;
}

QImage QFontEngine::alphaRGBMapForGlyph(glyph_t glyph, const QFixedPoint &/*subPixelPosition*/, const QTransform &t)
{
    const QImage alphaMask = alphaMapForGlyph(glyph, t);
    QImage rgbMask(alphaMask.width(), alphaMask.height(), QImage::Format_RGB32);

    for (int y=0; y<alphaMask.height(); ++y) {
        uint *dst = (uint *) rgbMask.scanLine(y);
        const uchar *src = alphaMask.constScanLine(y);
        for (int x=0; x<alphaMask.width(); ++x) {
            int val = src[x];
            dst[x] = qRgb(val, val, val);
        }
    }

    return rgbMask;
}

QImage QFontEngine::bitmapForGlyph(glyph_t, const QFixedPoint &subPixelPosition, const QTransform&, const QColor &)
{
    Q_UNUSED(subPixelPosition);

    return QImage();
}

QFixedPoint QFontEngine::subPixelPositionFor(const QFixedPoint &position) const
{
    if (m_subPixelPositionCount <= 1
            || (!supportsHorizontalSubPixelPositions()
                && !supportsVerticalSubPixelPositions())) {
        return QFixedPoint();
    }

    auto f = [&](QFixed v) {
        if (v != 0) {
            v = v - v.floor() + QFixed::fromFixed(1);
            QFixed fraction = (v * m_subPixelPositionCount).floor();
            v = fraction / QFixed(m_subPixelPositionCount);
        }
        return v;
    };

    return QFixedPoint(f(position.x), f(position.y));
}

QFontEngine::Glyph *QFontEngine::glyphData(glyph_t,
                                           const QFixedPoint &,
                                           QFontEngine::GlyphFormat,
                                           const QTransform &)
{
    return nullptr;
}

#if QT_CONFIG(harfbuzz)
template <typename Functor>
auto queryHarfbuzz(const QFontEngine *engine, Functor &&func)
{
    decltype(func(nullptr)) result = {};

    hb_face_t *hbFace = hb_qt_face_get_for_engine(const_cast<QFontEngine *>(engine));
    if (hb_font_t *hbFont = hb_font_create(hbFace)) {
        // explicitly use OpenType handlers for this font
        hb_ot_font_set_funcs(hbFont);

        result = func(hbFont);

        hb_font_destroy(hbFont);
    }

    return result;
}
#endif

QString QFontEngine::glyphName(glyph_t index) const
{
    QString result;
    if (index >= glyph_t(glyphCount()))
        return result;

#if QT_CONFIG(harfbuzz)
    result = queryHarfbuzz(this, [index](hb_font_t *hbFont){
        QString result;
        // According to the OpenType specification, glyph names are limited to 63
        // characters and can only contain (a subset of) ASCII.
        char name[64];
        if (hb_font_get_glyph_name(hbFont, index, name, sizeof(name)))
            result = QString::fromLatin1(name);
        return result;
    });
#endif

    if (result.isEmpty())
        result = index ? u"gid%1"_s.arg(index) : u".notdef"_s;
    return result;
}

glyph_t QFontEngine::findGlyph(QLatin1StringView name) const
{
    glyph_t result = 0;

#if QT_CONFIG(harfbuzz)
    result = queryHarfbuzz(this, [name](hb_font_t *hbFont){
        // glyph names are all ASCII, so latin1 is fine here.
        hb_codepoint_t glyph;
        if (hb_font_get_glyph_from_name(hbFont, name.constData(), name.size(), &glyph))
            return glyph_t(glyph);
        return glyph_t(0);
    });
#else // if we are here, no point in trying again if we already tried harfbuzz
    if (!result) {
        for (glyph_t index = 0; index < uint(glyphCount()); ++index) {
            if (name == glyphName(index))
                return index;
        }
    }
#endif

    if (!result) {
        constexpr auto gid = "gid"_L1;
        constexpr auto uni = "uni"_L1;
        if (name.startsWith(gid)) {
            bool ok;
            result = name.slice(gid.size()).toUInt(&ok);
            if (ok && result < glyph_t(glyphCount()))
                return result;
        } else if (name.startsWith(uni)) {
            bool ok;
            const uint ucs4 = name.slice(uni.size()).toUInt(&ok, 16);
            if (ok) {
                result = glyphIndex(ucs4);
                if (result > 0 && result < glyph_t(glyphCount()))
                    return result;
            }
        }
    }

    return result;
}

QImage QFontEngine::renderedPathForGlyph(glyph_t glyph, const QColor &color)
{
    glyph_metrics_t gm = boundingBox(glyph);
    int glyph_x = qFloor(gm.x.toReal());
    int glyph_y = qFloor(gm.y.toReal());
    int glyph_width = qCeil((gm.x + gm.width).toReal()) -  glyph_x;
    int glyph_height = qCeil((gm.y + gm.height).toReal()) - glyph_y;

    if (glyph_width <= 0 || glyph_height <= 0)
        return QImage();
    QFixedPoint pt;
    pt.x = -glyph_x;
    pt.y = -glyph_y; // the baseline
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    QImage im(glyph_width, glyph_height, QImage::Format_ARGB32_Premultiplied);
    im.fill(Qt::transparent);
    QPainter p(&im);
    p.setRenderHint(QPainter::Antialiasing);
    addGlyphsToPath(&glyph, &pt, 1, &path, { });
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawPath(path);
    p.end();

    return im;
}

QImage QFontEngine::alphaMapForGlyph(glyph_t glyph)
{
    QImage im = renderedPathForGlyph(glyph, Qt::black);
    QImage alphaMap(im.width(), im.height(), QImage::Format_Alpha8);

    for (int y=0; y<im.height(); ++y) {
        uchar *dst = (uchar *) alphaMap.scanLine(y);
        const uint *src = reinterpret_cast<const uint *>(im.constScanLine(y));
        for (int x=0; x<im.width(); ++x)
            dst[x] = qAlpha(src[x]);
    }

    return alphaMap;
}

void QFontEngine::removeGlyphFromCache(glyph_t)
{
}

QFontEngine::Properties QFontEngine::properties() const
{
    Properties p;
    p.postscriptName =
            QFontEngine::convertToPostscriptFontFamilyName(fontDef.families.first().toUtf8()) + '-'
            + QByteArray::number(fontDef.style) + '-' + QByteArray::number(fontDef.weight);
    p.ascent = ascent();
    p.descent = descent();
    p.leading = leading();
    p.emSquare = p.ascent;
    p.boundingBox = QRectF(0, -p.ascent.toReal(), maxCharWidth(), (p.ascent + p.descent).toReal());
    p.italicAngle = 0;
    p.capHeight = p.ascent;
    p.lineWidth = lineThickness();
    return p;
}

void QFontEngine::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics)
{
    *metrics = boundingBox(glyph);
    QFixedPoint p;
    p.x = 0;
    p.y = 0;
    addGlyphsToPath(&glyph, &p, 1, path, QFlag(0));
}

/*!
    Returns \c true if the font table idetified by \a tag exists in the font;
    returns \c false otherwise.

    If \a buffer is \nullptr, stores the size of the buffer required for the font table data,
    in bytes, in \a length. If \a buffer is not \nullptr and the capacity
    of the buffer, passed in \a length, is sufficient to store the font table data,
    also copies the font table data to \a buffer.

    Note: returning \c false when the font table exists could lead to an undefined behavior.
*/
bool QFontEngine::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    Q_UNUSED(tag);
    Q_UNUSED(buffer);
    Q_UNUSED(length);
    return false;
}

QByteArray QFontEngine::getSfntTable(uint tag) const
{
    QByteArray table;
    uint len = 0;
    if (!getSfntTableData(tag, nullptr, &len))
        return table;
    table.resize(len);
    if (!getSfntTableData(tag, reinterpret_cast<uchar *>(table.data()), &len))
        return QByteArray();
    return table;
}

void QFontEngine::clearGlyphCache(const void *context)
{
    m_glyphCaches.remove(context);
}

void QFontEngine::setGlyphCache(const void *context, QFontEngineGlyphCache *cache)
{
    Q_ASSERT(cache);

    GlyphCaches &caches = m_glyphCaches[context];
    for (auto & e : caches) {
        if (cache == e.cache.data())
            return;
    }

    // Limit the glyph caches to 4 per context. This covers all 90 degree rotations,
    // and limits memory use when there is continuous or random rotation
    if (caches.size() == 4)
        caches.pop_back();

    GlyphCacheEntry entry;
    entry.cache = cache;
    caches.push_front(entry);

}

QFontEngineGlyphCache *QFontEngine::glyphCache(const void *context,
                                               GlyphFormat format,
                                               const QTransform &transform,
                                               const QColor &color) const
{
    const QHash<const void*, GlyphCaches>::const_iterator caches = m_glyphCaches.constFind(context);
    if (caches == m_glyphCaches.cend())
        return nullptr;

    for (auto &e : *caches) {
        QFontEngineGlyphCache *cache = e.cache.data();
        if (format == cache->glyphFormat()
                && (format != Format_ARGB || color == cache->color())
                && qtransform_equals_no_translate(cache->m_transform, transform)) {
            return cache;
        }
    }

    return nullptr;
}

static inline QFixed kerning(int left, int right, const QFontEngine::KernPair *pairs, int numPairs)
{
    uint left_right = (left << 16) + right;

    left = 0, right = numPairs - 1;
    while (left <= right) {
        int middle = left + ( ( right - left ) >> 1 );

        if (pairs[middle].left_right == left_right)
            return pairs[middle].adjust;

        if (pairs[middle].left_right < left_right)
            left = middle + 1;
        else
            right = middle - 1;
    }
    return 0;
}

void QFontEngine::doKerning(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    int numPairs = kerning_pairs.size();
    if (!numPairs)
        return;

    const KernPair *pairs = kerning_pairs.constData();

    if (flags & DesignMetrics) {
        for(int i = 0; i < glyphs->numGlyphs - 1; ++i)
            glyphs->advances[i] += kerning(glyphs->glyphs[i], glyphs->glyphs[i+1] , pairs, numPairs);
    } else {
        for(int i = 0; i < glyphs->numGlyphs - 1; ++i)
            glyphs->advances[i] += qRound(kerning(glyphs->glyphs[i], glyphs->glyphs[i+1] , pairs, numPairs));
    }
}

void QFontEngine::loadKerningPairs(QFixed scalingFactor)
{
    kerning_pairs.clear();

    QByteArray tab = getSfntTable(QFont::Tag("kern").value());
    if (tab.isEmpty())
        return;

    const uchar *table = reinterpret_cast<const uchar *>(tab.constData());
    const uchar *end = table + tab.size();

    quint16 version;
    if (!qSafeFromBigEndian(table, end, &version))
        return;

    if (version != 0) {
//        qDebug("wrong version");
       return;
    }

    quint16 numTables;
    if (!qSafeFromBigEndian(table + 2, end, &numTables))
        return;

    {
        int offset = 4;
        for(int i = 0; i < numTables; ++i) {
            const uchar *header = table + offset;

            quint16 version;
            if (!qSafeFromBigEndian(header, end, &version))
                goto end;

            quint16 length;
            if (!qSafeFromBigEndian(header + 2, end, &length))
                goto end;

            quint16 coverage;
            if (!qSafeFromBigEndian(header + 4, end, &coverage))
                goto end;

//            qDebug("subtable: version=%d, coverage=%x",version, coverage);
            if (version == 0 && coverage == 0x0001) {
                if (offset + length > tab.size()) {
//                    qDebug("length ouf ot bounds");
                    goto end;
                }
                const uchar *data = table + offset + 6;

                quint16 nPairs;
                if (!qSafeFromBigEndian(data, end, &nPairs))
                    goto end;

                if (nPairs * 6 + 8 > length - 6) {
//                    qDebug("corrupt table!");
                    // corrupt table
                    goto end;
                }

                int off = 8;
                for(int i = 0; i < nPairs; ++i) {
                    QFontEngine::KernPair p;

                    quint16 tmp;
                    if (!qSafeFromBigEndian(data + off, end, &tmp))
                        goto end;

                    p.left_right = uint(tmp) << 16;
                    if (!qSafeFromBigEndian(data + off + 2, end, &tmp))
                        goto end;

                    p.left_right |= tmp;

                    if (!qSafeFromBigEndian(data + off + 4, end, &tmp))
                        goto end;

                    p.adjust = QFixed(int(short(tmp))) / scalingFactor;
                    kerning_pairs.append(p);
                    off += 6;
                }
            }
            offset += length;
        }
    }
end:
    std::sort(kerning_pairs.begin(), kerning_pairs.end());
//    for (int i = 0; i < kerning_pairs.count(); ++i)
//        qDebug() << 'i' << i << "left_right" << Qt::hex << kerning_pairs.at(i).left_right;
}


int QFontEngine::glyphCount() const
{
    QByteArray maxpTable = getSfntTable(QFont::Tag("maxp").value());
    if (maxpTable.size() < 6)
        return 0;

    const uchar *source = reinterpret_cast<const uchar *>(maxpTable.constData() + 4);
    const uchar *end = source + maxpTable.size();

    quint16 count = 0;
    qSafeFromBigEndian(source, end, &count);
    return count;
}

Qt::HANDLE QFontEngine::handle() const
{
    return nullptr;
}

const uchar *QFontEngine::getCMap(const uchar *table, uint tableSize, bool *isSymbolFont, int *cmapSize)
{
    const uchar *header = table;
    const uchar *endPtr = table + tableSize;

    // version check
    quint16 version;
    if (!qSafeFromBigEndian(header, endPtr, &version) || version != 0)
        return nullptr;

    quint16 numTables;
    if (!qSafeFromBigEndian(header + 2, endPtr, &numTables))
        return nullptr;

    const uchar *maps = table + 4;

    enum {
        Invalid,
        AppleRoman,
        Symbol,
        Unicode11,
        Unicode,
        MicrosoftUnicode,
        MicrosoftUnicodeExtended
    };

    int symbolTable = -1;
    int tableToUse = -1;
    int score = Invalid;
    for (int n = 0; n < numTables; ++n) {
        quint16 platformId = 0;
        if (!qSafeFromBigEndian(maps + 8 * n, endPtr, &platformId))
            return nullptr;

        quint16 platformSpecificId = 0;
        if (!qSafeFromBigEndian(maps + 8 * n + 2, endPtr, &platformSpecificId))
            return nullptr;

        switch (platformId) {
        case 0: // Unicode
            if (score < Unicode &&
                (platformSpecificId == 0 ||
                 platformSpecificId == 2 ||
                 platformSpecificId == 3)) {
                tableToUse = n;
                score = Unicode;
            } else if (score < Unicode11 && platformSpecificId == 1) {
                tableToUse = n;
                score = Unicode11;
            }
            break;
        case 1: // Apple
            if (score < AppleRoman && platformSpecificId == 0) { // Apple Roman
                tableToUse = n;
                score = AppleRoman;
            }
            break;
        case 3: // Microsoft
            switch (platformSpecificId) {
            case 0:
                symbolTable = n;
                if (score < Symbol) {
                    tableToUse = n;
                    score = Symbol;
                }
                break;
            case 1:
                if (score < MicrosoftUnicode) {
                    tableToUse = n;
                    score = MicrosoftUnicode;
                }
                break;
            case 0xa:
                if (score < MicrosoftUnicodeExtended) {
                    tableToUse = n;
                    score = MicrosoftUnicodeExtended;
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
    if (tableToUse < 0)
        return nullptr;

resolveTable:
    *isSymbolFont = (symbolTable > -1);

    quint32 unicode_table = 0;
    if (!qSafeFromBigEndian(maps + 8 * tableToUse + 4, endPtr, &unicode_table))
        return nullptr;

    if (!unicode_table)
        return nullptr;

    // get the header of the unicode table
    header = table + unicode_table;

    quint16 format;
    if (!qSafeFromBigEndian(header, endPtr, &format))
        return nullptr;

    quint32 length;
    if (format < 8) {
        quint16 tmp;
        if (!qSafeFromBigEndian(header + 2, endPtr, &tmp))
            return nullptr;
        length = tmp;
    } else {
        if (!qSafeFromBigEndian(header + 4, endPtr, &length))
            return nullptr;
    }

    if (table + unicode_table + length > endPtr)
        return nullptr;
    *cmapSize = length;

    // To support symbol fonts that contain a unicode table for the symbol area
    // we check the cmap tables and fall back to symbol font unless that would
    // involve losing information from the unicode table
    if (symbolTable > -1 && ((score == Unicode) || (score == Unicode11))) {
        const uchar *selectedTable = table + unicode_table;

        // Check that none of the latin1 range are in the unicode table
        bool unicodeTableHasLatin1 = false;
        for (int uc=0x00; uc<0x100; ++uc) {
            if (getTrueTypeGlyphIndex(selectedTable, length, uc) != 0) {
                unicodeTableHasLatin1 = true;
                break;
            }
        }

        // Check that at least one symbol char is in the unicode table
        bool unicodeTableHasSymbols = false;
        if (!unicodeTableHasLatin1) {
            for (int uc=0xf000; uc<0xf100; ++uc) {
                if (getTrueTypeGlyphIndex(selectedTable, length, uc) != 0) {
                    unicodeTableHasSymbols = true;
                    break;
                }
            }
        }

        // Fall back to symbol table
        if (!unicodeTableHasLatin1 && unicodeTableHasSymbols) {
            tableToUse = symbolTable;
            score = Symbol;
            goto resolveTable;
        }
    }

    return table + unicode_table;
}

quint32 QFontEngine::getTrueTypeGlyphIndex(const uchar *cmap, int cmapSize, uint unicode)
{
    const uchar *end = cmap + cmapSize;
    quint16 format = 0;
    if (!qSafeFromBigEndian(cmap, end, &format))
        return 0;

    if (format == 0) {
        const uchar *ptr = cmap + 6 + unicode;
        if (unicode < 256 && ptr < end)
            return quint32(*ptr);
    } else if (format == 4) {
        /* some fonts come with invalid cmap tables, where the last segment
           specified end = start = rangeoffset = 0xffff, delta = 0x0001
           Since 0xffff is never a valid Unicode char anyway, we just get rid of the issue
           by returning 0 for 0xffff
        */
        if (unicode >= 0xffff)
            return 0;

        quint16 segCountX2 = 0;
        if (!qSafeFromBigEndian(cmap + 6, end, &segCountX2))
            return 0;

        const unsigned char *ends = cmap + 14;

        int i = 0;
        for (; i < segCountX2/2; ++i) {
            quint16 codePoint = 0;
            if (!qSafeFromBigEndian(ends + 2 * i, end, &codePoint))
                return 0;
            if (codePoint >= unicode)
                break;
        }

        const unsigned char *idx = ends + segCountX2 + 2 + 2*i;

        quint16 startIndex = 0;
        if (!qSafeFromBigEndian(idx, end, &startIndex))
            return 0;
        if (startIndex > unicode)
            return 0;

        idx += segCountX2;

        quint16 tmp = 0;
        if (!qSafeFromBigEndian(idx, end, &tmp))
            return 0;
        qint16 idDelta = qint16(tmp);

        idx += segCountX2;

        quint16 idRangeoffset_t = 0;
        if (!qSafeFromBigEndian(idx, end, &idRangeoffset_t))
            return 0;

        quint16 glyphIndex = 0;
        if (idRangeoffset_t) {
            quint16 id = 0;
            if (!qSafeFromBigEndian(idRangeoffset_t + 2 * (unicode - startIndex) + idx, end, &id))
                return 0;

            if (id)
                glyphIndex = (idDelta + id) % 0x10000;
            else
                glyphIndex = 0;
        } else {
            glyphIndex = (idDelta + unicode) % 0x10000;
        }
        return glyphIndex;
    } else if (format == 6) {
        quint16 tableSize = 0;
        if (!qSafeFromBigEndian(cmap + 2, end, &tableSize))
            return 0;

        quint16 firstCode6 = 0;
        if (!qSafeFromBigEndian(cmap + 6, end, &firstCode6))
            return 0;
        if (unicode < firstCode6)
            return 0;

        quint16 entryCount6 = 0;
        if (!qSafeFromBigEndian(cmap + 8, end, &entryCount6))
            return 0;
        if (entryCount6 * 2 + 10 > tableSize)
            return 0;

        quint16 sentinel6 = firstCode6 + entryCount6;
        if (unicode >= sentinel6)
            return 0;

        quint16 entryIndex6 = unicode - firstCode6;

        quint16 index = 0;
        qSafeFromBigEndian(cmap + 10 + (entryIndex6 * 2), end, &index);
        return index;
    } else if (format == 12) {
        quint32 nGroups = 0;
        if (!qSafeFromBigEndian(cmap + 12, end, &nGroups))
            return 0;

        cmap += 16; // move to start of groups

        int left = 0, right = nGroups - 1;
        while (left <= right) {
            int middle = left + ( ( right - left ) >> 1 );

            quint32 startCharCode = 0;
            if (!qSafeFromBigEndian(cmap + 12 * middle, end, &startCharCode))
                return 0;

            if (unicode < startCharCode)
                right = middle - 1;
            else {
                quint32 endCharCode = 0;
                if (!qSafeFromBigEndian(cmap + 12 * middle + 4, end, &endCharCode))
                    return 0;

                if (unicode <= endCharCode) {
                    quint32 index = 0;
                    if (!qSafeFromBigEndian(cmap + 12 * middle + 8, end, &index))
                        return 0;

                    return index + unicode - startCharCode;
                }
                left = middle + 1;
            }
        }
    } else {
        qDebug("cmap table of format %d not implemented", format);
    }

    return 0;
}

QByteArray QFontEngine::convertToPostscriptFontFamilyName(const QByteArray &family)
{
    QByteArray f = family;
    f.replace(' ', "");
    f.replace('(', "");
    f.replace(')', "");
    f.replace('<', "");
    f.replace('>', "");
    f.replace('[', "");
    f.replace(']', "");
    f.replace('{', "");
    f.replace('}', "");
    f.replace('/', "");
    f.replace('%', "");
    return f;
}

// Allow font engines (e.g. Windows) that can not reliably create
// outline paths for distance-field rendering to switch the scene
// graph over to native text rendering.
bool QFontEngine::hasUnreliableGlyphOutline() const
{
    // Color glyphs (Emoji) are generally not suited for outlining
    return glyphFormat == QFontEngine::Format_ARGB;
}

QFixed QFontEngine::firstLeftBearing(const QGlyphLayout &glyphs)
{
    for (int i = 0; i < glyphs.numGlyphs; ++i) {
        glyph_t glyph = glyphs.glyphs[i];
        glyph_metrics_t gi = boundingBox(glyph);
        if (gi.isValid() && gi.width > 0)
            return gi.leftBearing();
    }
    return 0;
}

QFixed QFontEngine::lastRightBearing(const QGlyphLayout &glyphs)
{
    if (glyphs.numGlyphs >= 1) {
        glyph_t glyph = glyphs.glyphs[glyphs.numGlyphs - 1];
        glyph_metrics_t gi = boundingBox(glyph);
        if (gi.isValid())
            return gi.rightBearing();
    }
    return 0;
}

QList<QFontVariableAxis> QFontEngine::variableAxes() const
{
    return QList<QFontVariableAxis>{};
}

QFontEngine::GlyphCacheEntry::GlyphCacheEntry()
{
}

QFontEngine::GlyphCacheEntry::GlyphCacheEntry(const GlyphCacheEntry &o)
    : cache(o.cache)
{
}

QFontEngine::GlyphCacheEntry::~GlyphCacheEntry()
{
}

QFontEngine::GlyphCacheEntry &QFontEngine::GlyphCacheEntry::operator=(const GlyphCacheEntry &o)
{
    cache = o.cache;
    return *this;
}

bool QFontEngine::disableEmojiSegmenter()
{
#if defined(QT_NO_EMOJISEGMENTER)
    return true;
#else
    static const bool sDisableEmojiSegmenter = qEnvironmentVariableIntValue("QT_DISABLE_EMOJI_SEGMENTER") > 0;
    return sDisableEmojiSegmenter;
#endif
}

// ------------------------------------------------------------------
// The box font engine
// ------------------------------------------------------------------

QFontEngineBox::QFontEngineBox(int size)
    : QFontEngine(Box),
      _size(size)
{
    cache_cost = sizeof(QFontEngineBox);
}

QFontEngineBox::QFontEngineBox(Type type, int size)
    : QFontEngine(type),
      _size(size)
{
    cache_cost = sizeof(QFontEngineBox);
}

QFontEngineBox::~QFontEngineBox()
{
}

glyph_t QFontEngineBox::glyphIndex(uint ucs4) const
{
    Q_UNUSED(ucs4);
    return 1;
}

int QFontEngineBox::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QFontEngine::ShaperFlags flags) const
{
    Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
    if (*nglyphs < len) {
        *nglyphs = len;
        return -1;
    }

    int ucs4Length = 0;
    QStringIterator it(str, str + len);
    while (it.hasNext()) {
        it.advance();
        glyphs->glyphs[ucs4Length++] = 1;
    }

    *nglyphs = ucs4Length;
    glyphs->numGlyphs = ucs4Length;

    if (!(flags & GlyphIndicesOnly))
        recalcAdvances(glyphs, flags);

    return *nglyphs;
}

void QFontEngineBox::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags) const
{
    for (int i = 0; i < glyphs->numGlyphs; i++)
        glyphs->advances[i] = _size;
}

void QFontEngineBox::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags)
{
    if (!glyphs.numGlyphs)
        return;

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> positioned_glyphs;
    QTransform matrix = QTransform::fromTranslate(x, y - _size);
    getGlyphPositions(glyphs, matrix, flags, positioned_glyphs, positions);

    QSize s(_size - 3, _size - 3);
    for (int k = 0; k < positions.size(); k++)
        path->addRect(QRectF(positions[k].toPointF(), s));
}

glyph_metrics_t QFontEngineBox::boundingBox(const QGlyphLayout &glyphs)
{
    glyph_metrics_t overall;
    overall.width = _size*glyphs.numGlyphs;
    overall.height = _size;
    overall.xoff = overall.width;
    return overall;
}

void QFontEngineBox::draw(QPaintEngine *p, qreal x, qreal y, const QTextItemInt &ti)
{
    if (!ti.glyphs.numGlyphs)
        return;

    // any fixes here should probably also be done in QPaintEnginePrivate::drawBoxTextItem
    QSize s(_size - 3, _size - 3);

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix = QTransform::fromTranslate(x, y - _size);
    ti.fontEngine->getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;


    QPainter *painter = p->painter();
    painter->save();
    painter->setBrush(Qt::NoBrush);
    QPen pen = painter->pen();
    pen.setWidthF(lineThickness().toReal());
    painter->setPen(pen);
    for (int k = 0; k < positions.size(); k++)
        painter->drawRect(QRectF(positions[k].toPointF(), s));
    painter->restore();
}

glyph_metrics_t QFontEngineBox::boundingBox(glyph_t)
{
    return glyph_metrics_t(0, -_size, _size, _size, _size, 0);
}

QFontEngine *QFontEngineBox::cloneWithSize(qreal pixelSize) const
{
    QFontEngineBox *fe = new QFontEngineBox(pixelSize);
    return fe;
}

QFixed QFontEngineBox::ascent() const
{
    return _size;
}

QFixed QFontEngineBox::capHeight() const
{
    return _size;
}

QFixed QFontEngineBox::descent() const
{
    return 0;
}

QFixed QFontEngineBox::leading() const
{
    QFixed l = _size * QFixed::fromReal(qreal(0.15));
    return l.ceil();
}

qreal QFontEngineBox::maxCharWidth() const
{
    return _size;
}

bool QFontEngineBox::canRender(const QChar *, int) const
{
    return true;
}

QImage QFontEngineBox::alphaMapForGlyph(glyph_t)
{
    QImage image(_size, _size, QImage::Format_Alpha8);
    image.fill(0);

    uchar *bits = image.bits();
    for (int i=2; i <= _size-3; ++i) {
        bits[i + 2 * image.bytesPerLine()] = 255;
        bits[i + (_size - 3) * image.bytesPerLine()] = 255;
        bits[2 + i * image.bytesPerLine()] = 255;
        bits[_size - 3 + i * image.bytesPerLine()] = 255;
    }
    return image;
}

// ------------------------------------------------------------------
// Multi engine
// ------------------------------------------------------------------

uchar QFontEngineMulti::highByte(glyph_t glyph)
{ return glyph >> 24; }

// strip high byte from glyph
static inline glyph_t stripped(glyph_t glyph)
{ return glyph & 0x00ffffff; }

QFontEngineMulti::QFontEngineMulti(QFontEngine *engine, int script, const QStringList &fallbackFamilies)
    : QFontEngine(Multi),
      m_fallbackFamilies(fallbackFamilies),
      m_script(script),
      m_fallbackFamiliesQueried(!m_fallbackFamilies.isEmpty())
{
    Q_ASSERT(engine && engine->type() != QFontEngine::Multi);

    if (m_fallbackFamilies.isEmpty()) {
        // defer obtaining the fallback families until loadEngine(1)
        m_fallbackFamilies << QString();
    }

    m_engines.resize(m_fallbackFamilies.size() + 1);

    engine->ref.ref();
    m_engines[0] = engine;

    fontDef = engine->fontDef;
    cache_cost = engine->cache_cost;
}

QFontEngineMulti::~QFontEngineMulti()
{
    for (int i = 0; i < m_engines.size(); ++i) {
        QFontEngine *fontEngine = m_engines.at(i);
        if (fontEngine && !fontEngine->ref.deref())
            delete fontEngine;
    }
}

QStringList qt_fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QFontDatabasePrivate::ExtendedScript script);

void QFontEngineMulti::ensureFallbackFamiliesQueried()
{
    QFont::StyleHint styleHint = QFont::StyleHint(fontDef.styleHint);
    if (styleHint == QFont::AnyStyle && fontDef.fixedPitch)
        styleHint = QFont::TypeWriter;

    setFallbackFamiliesList(qt_fallbacksForFamily(fontDef.families.constFirst(),
                                                  QFont::Style(fontDef.style), styleHint,
                                                  QFontDatabasePrivate::ExtendedScript(m_script)));
}

void QFontEngineMulti::setFallbackFamiliesList(const QStringList &fallbackFamilies)
{
    Q_ASSERT(!m_fallbackFamiliesQueried);

    m_fallbackFamilies = fallbackFamilies;
    if (m_fallbackFamilies.isEmpty()) {
        // turns out we lied about having any fallback at all
        Q_ASSERT(m_engines.size() == 2); // see c-tor for details
        QFontEngine *engine = m_engines.at(0);
        engine->ref.ref();
        m_engines[1] = engine;
        m_fallbackFamilies << fontDef.families.constFirst();
    } else {
        m_engines.resize(m_fallbackFamilies.size() + 1);
    }

    m_fallbackFamiliesQueried = true;
}

void QFontEngineMulti::ensureEngineAt(int at)
{
    if (!m_fallbackFamiliesQueried && at > 0)
        ensureFallbackFamiliesQueried();
    Q_ASSERT(at < m_engines.size());
    if (!m_engines.at(at)) {
        QFontEngine *engine = loadEngine(at);
        if (!engine)
            engine = new QFontEngineBox(fontDef.pixelSize);
        Q_ASSERT(engine && engine->type() != QFontEngine::Multi);
        engine->ref.ref();
        m_engines[at] = engine;
    }
}

QFontEngine *QFontEngineMulti::loadEngine(int at)
{
    QFontDef request(fontDef);
    request.styleStrategy |= QFont::NoFontMerging;
    request.families = QStringList(fallbackFamilyAt(at - 1));

    // At this point, the main script of the text has already been considered
    // when fetching the list of fallback families from the database, and the
    // info about the actual script of the characters may have been discarded,
    // so we do not check for writing system support, but instead just load
    // the family indiscriminately.
    if (QFontEngine *engine = QFontDatabasePrivate::findFont(request, QFontDatabasePrivate::Script_Common)) {
        engine->fontDef.weight = request.weight;
        if (request.style > QFont::StyleNormal)
            engine->fontDef.style = request.style;
        return engine;
    }

    return nullptr;
}

glyph_t QFontEngineMulti::glyphIndex(uint ucs4) const
{
    glyph_t glyph = engine(0)->glyphIndex(ucs4);
    if (glyph == 0 && !isIgnorableChar(ucs4)) {
        if (!m_fallbackFamiliesQueried)
            const_cast<QFontEngineMulti *>(this)->ensureFallbackFamiliesQueried();
        for (int x = 1, n = qMin(m_engines.size(), 256); x < n; ++x) {
            QFontEngine *engine = m_engines.at(x);
            if (!engine) {
                if (!shouldLoadFontEngineForCharacter(x, ucs4))
                    continue;
                const_cast<QFontEngineMulti *>(this)->ensureEngineAt(x);
                engine = m_engines.at(x);
            }
            Q_ASSERT(engine != nullptr);
            if (engine->type() == Box)
                continue;

            glyph = engine->glyphIndex(ucs4);
            if (glyph != 0) {
                // set the high byte to indicate which engine the glyph came from
                glyph |= (x << 24);
                break;
            }
        }
    }

    return glyph;
}

QString QFontEngineMulti::glyphName(glyph_t glyph) const
{
    const int which = highByte(glyph);
    const_cast<QFontEngineMulti *>(this)->ensureEngineAt(which);
    return engine(which)->glyphName(stripped(glyph));
}

glyph_t QFontEngineMulti::findGlyph(QLatin1StringView name) const
{
    return engine(0)->findGlyph(name);
}

int QFontEngineMulti::stringToCMap(const QChar *str, int len,
                                   QGlyphLayout *glyphs, int *nglyphs,
                                   QFontEngine::ShaperFlags flags) const
{
    const int originalNumGlyphs = glyphs->numGlyphs;
    int mappedGlyphCount = engine(0)->stringToCMap(str, len, glyphs, nglyphs, flags);
    if (mappedGlyphCount < 0)
        return -1;

    // If ContextFontMerging is set and the match for the string was incomplete, we try all
    // fallbacks on the full string until we find the best match.
    bool contextFontMerging = mappedGlyphCount < *nglyphs && (fontDef.styleStrategy & QFont::ContextFontMerging);
    if (contextFontMerging) {
        QVarLengthGlyphLayoutArray tempLayout(len);
        if (!m_fallbackFamiliesQueried)
            const_cast<QFontEngineMulti *>(this)->ensureFallbackFamiliesQueried();

        int maxGlyphCount = 0;
        uchar engineIndex = 0;
        for (int x = 1, n = qMin(m_engines.size(), 256); x < n; ++x) {
            int numGlyphs = len;
            const_cast<QFontEngineMulti *>(this)->ensureEngineAt(x);
            maxGlyphCount = engine(x)->stringToCMap(str, len, &tempLayout, &numGlyphs, flags);

            // If we found a better match, we copy data into the main QGlyphLayout
            if (maxGlyphCount > mappedGlyphCount) {
                *nglyphs = numGlyphs;
                glyphs->numGlyphs = originalNumGlyphs;
                glyphs->copy(&tempLayout);
                engineIndex = x;
                if (maxGlyphCount == numGlyphs)
                    break;
            }
        }

        if (engineIndex > 0) {
            for (int y = 0; y < glyphs->numGlyphs; ++y) {
                if (glyphs->glyphs[y] != 0)
                    glyphs->glyphs[y] |= (engineIndex << 24);
            }
        } else {
            contextFontMerging = false;
        }

        mappedGlyphCount = maxGlyphCount;
    }

    // Fill in missing glyphs by going through string one character at the time and finding
    // the first viable fallback.
    int glyph_pos = 0;
    QStringIterator it(str, str + len);

    const bool enableVariationSelectorHack = disableEmojiSegmenter();
    char32_t previousUcs4 = 0;

    int lastFallback = -1;
    while (it.hasNext()) {
        const char32_t ucs4 = it.peekNext();

        // If we applied a fallback font to previous glyph, and the current is either
        // ZWJ or ZWNJ, we should also try applying the same fallback font to that, in order
        // to get the correct shaping rules applied.
        if (lastFallback >= 0 && (ucs4 == 0x200d || ucs4 == 0x200c)) {
            QFontEngine *engine = m_engines.at(lastFallback);
            glyph_t glyph = engine->glyphIndex(ucs4);
            if (glyph != 0) {
                glyphs->glyphs[glyph_pos] = glyph;
                if (!(flags & GlyphIndicesOnly)) {
                    QGlyphLayout g = glyphs->mid(glyph_pos, 1);
                    engine->recalcAdvances(&g, flags);
                }

                // set the high byte to indicate which engine the glyph came from
                glyphs->glyphs[glyph_pos] |= (lastFallback << 24);
            } else {
                lastFallback = -1;
            }
        } else {
            lastFallback = -1;
        }

        if (glyphs->glyphs[glyph_pos] == 0 && !isIgnorableChar(ucs4)) {
            if (!m_fallbackFamiliesQueried)
                const_cast<QFontEngineMulti *>(this)->ensureFallbackFamiliesQueried();
            for (int x = contextFontMerging ? 0 : 1, n = qMin(m_engines.size(), 256); x < n; ++x) {
                QFontEngine *engine = m_engines.at(x);
                if (!engine) {
                    if (!shouldLoadFontEngineForCharacter(x, ucs4))
                        continue;
                    const_cast<QFontEngineMulti *>(this)->ensureEngineAt(x);
                    engine = m_engines.at(x);
                    if (!engine)
                        continue;
                }
                Q_ASSERT(engine != nullptr);
                if (engine->type() == Box)
                    continue;

                glyph_t glyph = engine->glyphIndex(ucs4);
                if (glyph != 0) {
                    glyphs->glyphs[glyph_pos] = glyph;
                    if (!(flags & GlyphIndicesOnly)) {
                        QGlyphLayout g = glyphs->mid(glyph_pos, 1);
                        engine->recalcAdvances(&g, flags);
                    }

                    lastFallback = x;

                    // set the high byte to indicate which engine the glyph came from
                    glyphs->glyphs[glyph_pos] |= (x << 24);
                    break;
                }
            }

            // For variant-selectors, they are modifiers to the previous character. If we
            // end up with different font selections for the selector and the character it
            // modifies, we try applying the selector font to the preceding character as well
            const int variantSelectorBlock = 0xFE00;
            if (enableVariationSelectorHack && (ucs4 & 0xFFF0) == variantSelectorBlock && glyph_pos > 0) {
                int selectorFontEngine = glyphs->glyphs[glyph_pos] >> 24;
                int precedingCharacterFontEngine = glyphs->glyphs[glyph_pos - 1] >> 24;

                if (selectorFontEngine != precedingCharacterFontEngine) {
                    // Emoji variant selectors are specially handled and should affect font
                    // selection. If VS-16 is used, then this means we want to select a color
                    // font. If the selected font is already a color font, we do not need search
                    // again. If the VS-15 is used, then this means we want to select a non-color
                    // font. If the selected font is not a color font, we don't do anything.
                    const QFontEngine *selectedEngine = m_engines.at(precedingCharacterFontEngine);
                    const bool colorFont = selectedEngine->isColorFont();
                    const char32_t vs15 = 0xFE0E;
                    const char32_t vs16 = 0xFE0F;
                    bool adaptVariantSelector = ucs4 < vs15
                                                || (ucs4 == vs15 && colorFont)
                                                || (ucs4 == vs16 && !colorFont);

                    if (adaptVariantSelector) {
                        QFontEngine *engine = m_engines.at(selectorFontEngine);
                        glyph_t glyph = engine->glyphIndex(previousUcs4);
                        if (glyph != 0) {
                            glyphs->glyphs[glyph_pos - 1] = glyph;
                            if (!(flags & GlyphIndicesOnly)) {
                                QGlyphLayout g = glyphs->mid(glyph_pos - 1, 1);
                                engine->recalcAdvances(&g, flags);
                            }

                            // set the high byte to indicate which engine the glyph came from
                            glyphs->glyphs[glyph_pos - 1] |= (selectorFontEngine << 24);
                        }
                    }
                }
            }
        }

        it.advance();
        ++glyph_pos;

        previousUcs4 = ucs4;
    }

    *nglyphs = glyph_pos;
    glyphs->numGlyphs = glyph_pos;
    return mappedGlyphCount;
}

bool QFontEngineMulti::shouldLoadFontEngineForCharacter(int at, uint ucs4) const
{
    Q_UNUSED(at);
    Q_UNUSED(ucs4);
    return true;
}

glyph_metrics_t QFontEngineMulti::boundingBox(const QGlyphLayout &glyphs)
{
    if (glyphs.numGlyphs <= 0)
        return glyph_metrics_t();

    glyph_metrics_t overall;

    int which = highByte(glyphs.glyphs[0]);
    int start = 0;
    int end, i;
    for (end = 0; end < glyphs.numGlyphs; ++end) {
        const int e = highByte(glyphs.glyphs[end]);
        if (e == which)
            continue;

        // set the high byte to zero
        for (i = start; i < end; ++i)
            glyphs.glyphs[i] = stripped(glyphs.glyphs[i]);

        // merge the bounding box for this run
        const glyph_metrics_t gm = engine(which)->boundingBox(glyphs.mid(start, end - start));

        overall.x = qMin(overall.x, gm.x);
        overall.y = qMin(overall.y, gm.y);
        overall.width = overall.xoff + gm.width;
        overall.height = qMax(overall.height + overall.y, gm.height + gm.y) -
                         qMin(overall.y, gm.y);
        overall.xoff += gm.xoff;
        overall.yoff += gm.yoff;

        // reset the high byte for all glyphs
        const int hi = which << 24;
        for (i = start; i < end; ++i)
            glyphs.glyphs[i] = hi | glyphs.glyphs[i];

        // change engine
        start = end;
        which = e;
    }

    // set the high byte to zero
    for (i = start; i < end; ++i)
        glyphs.glyphs[i] = stripped(glyphs.glyphs[i]);

    // merge the bounding box for this run
    const glyph_metrics_t gm = engine(which)->boundingBox(glyphs.mid(start, end - start));

    overall.x = qMin(overall.x, gm.x);
    overall.y = qMin(overall.y, gm.y);
    overall.width = overall.xoff + gm.width;
    overall.height = qMax(overall.height + overall.y, gm.height + gm.y) -
                     qMin(overall.y, gm.y);
    overall.xoff += gm.xoff;
    overall.yoff += gm.yoff;

    // reset the high byte for all glyphs
    const int hi = which << 24;
    for (i = start; i < end; ++i)
        glyphs.glyphs[i] = hi | glyphs.glyphs[i];

    return overall;
}

void QFontEngineMulti::getGlyphBearings(glyph_t glyph, qreal *leftBearing, qreal *rightBearing)
{
    int which = highByte(glyph);
    ensureEngineAt(which);
    engine(which)->getGlyphBearings(stripped(glyph), leftBearing, rightBearing);
}

void QFontEngineMulti::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs,
                                        QPainterPath *path, QTextItem::RenderFlags flags)
{
    if (glyphs.numGlyphs <= 0)
        return;

    int which = highByte(glyphs.glyphs[0]);
    int start = 0;
    int end, i;
    if (flags & QTextItem::RightToLeft) {
        for (int gl = 0; gl < glyphs.numGlyphs; gl++)
            x += glyphs.advances[gl].toReal();
    }
    for (end = 0; end < glyphs.numGlyphs; ++end) {
        const int e = highByte(glyphs.glyphs[end]);
        if (e == which)
            continue;

        if (flags & QTextItem::RightToLeft) {
            for (i = start; i < end; ++i)
                x -= glyphs.advances[i].toReal();
        }

        // set the high byte to zero
        for (i = start; i < end; ++i)
            glyphs.glyphs[i] = stripped(glyphs.glyphs[i]);
        engine(which)->addOutlineToPath(x, y, glyphs.mid(start, end - start), path, flags);
        // reset the high byte for all glyphs and update x and y
        const int hi = which << 24;
        for (i = start; i < end; ++i)
            glyphs.glyphs[i] = hi | glyphs.glyphs[i];

        if (!(flags & QTextItem::RightToLeft)) {
            for (i = start; i < end; ++i)
                x += glyphs.advances[i].toReal();
        }

        // change engine
        start = end;
        which = e;
    }

    if (flags & QTextItem::RightToLeft) {
        for (i = start; i < end; ++i)
            x -= glyphs.advances[i].toReal();
    }

    // set the high byte to zero
    for (i = start; i < end; ++i)
        glyphs.glyphs[i] = stripped(glyphs.glyphs[i]);

    engine(which)->addOutlineToPath(x, y, glyphs.mid(start, end - start), path, flags);

    // reset the high byte for all glyphs
    const int hi = which << 24;
    for (i = start; i < end; ++i)
        glyphs.glyphs[i] = hi | glyphs.glyphs[i];
}

void QFontEngineMulti::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    if (glyphs->numGlyphs <= 0)
        return;

    int which = highByte(glyphs->glyphs[0]);
    int start = 0;
    int end, i;
    for (end = 0; end < glyphs->numGlyphs; ++end) {
        const int e = highByte(glyphs->glyphs[end]);
        if (e == which)
            continue;

        // set the high byte to zero
        for (i = start; i < end; ++i)
            glyphs->glyphs[i] = stripped(glyphs->glyphs[i]);

        QGlyphLayout offs = glyphs->mid(start, end - start);
        engine(which)->recalcAdvances(&offs, flags);

        // reset the high byte for all glyphs and update x and y
        const int hi = which << 24;
        for (i = start; i < end; ++i)
            glyphs->glyphs[i] = hi | glyphs->glyphs[i];

        // change engine
        start = end;
        which = e;
    }

    // set the high byte to zero
    for (i = start; i < end; ++i)
        glyphs->glyphs[i] = stripped(glyphs->glyphs[i]);

    QGlyphLayout offs = glyphs->mid(start, end - start);
    engine(which)->recalcAdvances(&offs, flags);

    // reset the high byte for all glyphs
    const int hi = which << 24;
    for (i = start; i < end; ++i)
        glyphs->glyphs[i] = hi | glyphs->glyphs[i];
}

void QFontEngineMulti::doKerning(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    if (glyphs->numGlyphs <= 0)
        return;

    int which = highByte(glyphs->glyphs[0]);
    int start = 0;
    int end, i;
    for (end = 0; end < glyphs->numGlyphs; ++end) {
        const int e = highByte(glyphs->glyphs[end]);
        if (e == which)
            continue;

        // set the high byte to zero
        for (i = start; i < end; ++i)
            glyphs->glyphs[i] = stripped(glyphs->glyphs[i]);

        QGlyphLayout offs = glyphs->mid(start, end - start);
        engine(which)->doKerning(&offs, flags);

        // reset the high byte for all glyphs and update x and y
        const int hi = which << 24;
        for (i = start; i < end; ++i)
            glyphs->glyphs[i] = hi | glyphs->glyphs[i];

        // change engine
        start = end;
        which = e;
    }

    // set the high byte to zero
    for (i = start; i < end; ++i)
        glyphs->glyphs[i] = stripped(glyphs->glyphs[i]);

    QGlyphLayout offs = glyphs->mid(start, end - start);
    engine(which)->doKerning(&offs, flags);

    // reset the high byte for all glyphs
    const int hi = which << 24;
    for (i = start; i < end; ++i)
        glyphs->glyphs[i] = hi | glyphs->glyphs[i];
}

glyph_metrics_t QFontEngineMulti::boundingBox(glyph_t glyph)
{
    const int which = highByte(glyph);
    return engine(which)->boundingBox(stripped(glyph));
}

QFixed QFontEngineMulti::emSquareSize() const
{ return engine(0)->emSquareSize(); }

QFixed QFontEngineMulti::ascent() const
{ return engine(0)->ascent(); }

QFixed QFontEngineMulti::capHeight() const
{ return engine(0)->capHeight(); }

QFixed QFontEngineMulti::descent() const
{ return engine(0)->descent(); }

QFixed QFontEngineMulti::leading() const
{
    return engine(0)->leading();
}

QFixed QFontEngineMulti::xHeight() const
{
    return engine(0)->xHeight();
}

QFixed QFontEngineMulti::averageCharWidth() const
{
    return engine(0)->averageCharWidth();
}

QFixed QFontEngineMulti::lineThickness() const
{
    return engine(0)->lineThickness();
}

QFixed QFontEngineMulti::underlinePosition() const
{
    return engine(0)->underlinePosition();
}

qreal QFontEngineMulti::maxCharWidth() const
{
    return engine(0)->maxCharWidth();
}

qreal QFontEngineMulti::minLeftBearing() const
{
    return engine(0)->minLeftBearing();
}

qreal QFontEngineMulti::minRightBearing() const
{
    return engine(0)->minRightBearing();
}

bool QFontEngineMulti::canRender(const QChar *string, int len) const
{
    if (engine(0)->canRender(string, len))
        return true;

    int nglyphs = len;

    QVarLengthArray<glyph_t> glyphs(nglyphs);

    QGlyphLayout g;
    g.numGlyphs = nglyphs;
    g.glyphs = glyphs.data();
    if (stringToCMap(string, len, &g, &nglyphs, GlyphIndicesOnly) < 0)
        Q_UNREACHABLE();

    for (int i = 0; i < nglyphs; i++) {
        if (glyphs[i] == 0)
            return false;
    }

    return true;
}

/* Implement alphaMapForGlyph() which is called by QPA Windows code.
 * Ideally, that code should be fixed to correctly handle QFontEngineMulti. */

QImage QFontEngineMulti::alphaMapForGlyph(glyph_t glyph)
{
    const int which = highByte(glyph);
    return engine(which)->alphaMapForGlyph(stripped(glyph));
}

QImage QFontEngineMulti::alphaMapForGlyph(glyph_t glyph, const QFixedPoint &subPixelPosition)
{
    const int which = highByte(glyph);
    return engine(which)->alphaMapForGlyph(stripped(glyph), subPixelPosition);
}

QImage QFontEngineMulti::alphaMapForGlyph(glyph_t glyph, const QTransform &t)
{
    const int which = highByte(glyph);
    return engine(which)->alphaMapForGlyph(stripped(glyph), t);
}

QImage QFontEngineMulti::alphaMapForGlyph(glyph_t glyph,
                                          const QFixedPoint &subPixelPosition,
                                          const QTransform &t)
{
    const int which = highByte(glyph);
    return engine(which)->alphaMapForGlyph(stripped(glyph), subPixelPosition, t);
}

QImage QFontEngineMulti::alphaRGBMapForGlyph(glyph_t glyph,
                                             const QFixedPoint &subPixelPosition,
                                             const QTransform &t)
{
    const int which = highByte(glyph);
    return engine(which)->alphaRGBMapForGlyph(stripped(glyph), subPixelPosition, t);
}

QList<QFontVariableAxis> QFontEngineMulti::variableAxes() const
{
    return engine(0)->variableAxes();
}

/*
  This is used indirectly by Qt WebKit when using QTextLayout::setRawFont

  The purpose of this is to provide the necessary font fallbacks when drawing complex
  text. Since Qt WebKit ends up repeatedly creating QTextLayout instances and passing them
  the same raw font over and over again, we want to cache the corresponding multi font engine
  as it may contain fallback font engines already.
*/
QFontEngine *QFontEngineMulti::createMultiFontEngine(QFontEngine *fe, int script)
{
    QFontEngine *engine = nullptr;
    QFontCache::Key key(fe->fontDef, script, /*multi = */true);
    QFontCache *fc = QFontCache::instance();
    //  We can't rely on the fontDef (and hence the cache Key)
    //  alone to distinguish webfonts, since these should not be
    //  accidentally shared, even if the resulting fontcache key
    //  is strictly identical. See:
    //   http://www.w3.org/TR/css3-fonts/#font-face-rule
    const bool faceIsLocal = !fe->faceId().filename.isEmpty();
    QFontCache::EngineCache::Iterator it = fc->engineCache.find(key),
            end = fc->engineCache.end();
    while (it != end && it.key() == key) {
        Q_ASSERT(it.value().data->type() == QFontEngine::Multi);
        QFontEngineMulti *cachedEngine = static_cast<QFontEngineMulti *>(it.value().data);
        if (fe == cachedEngine->engine(0) || (faceIsLocal && fe->faceId().filename == cachedEngine->engine(0)->faceId().filename)) {
            engine = cachedEngine;
            fc->updateHitCountAndTimeStamp(it.value());
            break;
        }
        ++it;
    }
    if (!engine) {
        engine = QGuiApplicationPrivate::instance()->platformIntegration()->fontDatabase()->fontEngineMulti(fe, QFontDatabasePrivate::ExtendedScript(script));
        fc->insertEngine(key, engine, /* insertMulti */ !faceIsLocal);
    }
    Q_ASSERT(engine);
    return engine;
}

QTestFontEngine::QTestFontEngine(int size)
    : QFontEngineBox(TestFontEngine, size)
{}

QT_END_NAMESPACE
