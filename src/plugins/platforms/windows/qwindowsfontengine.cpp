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

#if _WIN32_WINNT < 0x0500
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include "qwindowsintegration.h"
#include "qwindowsfontengine.h"
#include "qwindowsnativeimage.h"
#include "qwindowscontext.h"
#include "qwindowsfontdatabase.h"
#include "qtwindows_additional.h"
#include "qwindowsfontenginedirectwrite.h"

#include <QtGui/private/qtextengine_p.h> // glyph_metrics_t
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/QPaintDevice>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>
#include <QtGui/private/qpainter_p.h>
#include <QtGui/QPaintEngine>
#include <QtGui/private/qpaintengine_raster_p.h>

#include <QtCore/QtEndian>
#include <QtCore/qmath.h>
#include <QtCore/QThreadStorage>
#include <QtCore/private/qsystemlibrary_p.h>

#include <QtCore/QDebug>

#include <limits.h>

#ifdef Q_OS_WINCE
#  include "qplatformfunctions_wince.h"
#endif

#if !defined(QT_NO_DIRECTWRITE)
#  include <dwrite.h>
#endif

QT_BEGIN_NAMESPACE

//### mingw needed define
#ifndef TT_PRIM_CSPLINE
#define TT_PRIM_CSPLINE 3
#endif

#ifdef MAKE_TAG
#undef MAKE_TAG
#endif
// GetFontData expects the tags in little endian ;(
#define MAKE_TAG(ch1, ch2, ch3, ch4) (\
    (((quint32)(ch4)) << 24) | \
    (((quint32)(ch3)) << 16) | \
    (((quint32)(ch2)) << 8) | \
    ((quint32)(ch1)) \
   )

// common DC for all fonts

typedef BOOL (WINAPI *PtrGetCharWidthI)(HDC, UINT, UINT, LPWORD, LPINT);
static PtrGetCharWidthI ptrGetCharWidthI = 0;
static bool resolvedGetCharWidthI = false;

static void resolveGetCharWidthI()
{
    if (resolvedGetCharWidthI)
        return;
    resolvedGetCharWidthI = true;
    ptrGetCharWidthI = (PtrGetCharWidthI)QSystemLibrary::resolve(QStringLiteral("gdi32"), "GetCharWidthI");
}

// defined in qtextengine_win.cpp
typedef void *SCRIPT_CACHE;
typedef HRESULT (WINAPI *fScriptFreeCache)(SCRIPT_CACHE *);
extern fScriptFreeCache ScriptFreeCache;

static inline quint32 getUInt(unsigned char *p)
{
    quint32 val;
    val = *p++ << 24;
    val |= *p++ << 16;
    val |= *p++ << 8;
    val |= *p;

    return val;
}

static inline quint16 getUShort(unsigned char *p)
{
    quint16 val;
    val = *p++ << 8;
    val |= *p;

    return val;
}

// general font engine

QFixed QWindowsFontEngine::lineThickness() const
{
    if(lineWidth > 0)
        return lineWidth;

    return QFontEngine::lineThickness();
}

static OUTLINETEXTMETRIC *getOutlineTextMetric(HDC hdc)
{
    int size;
    size = GetOutlineTextMetrics(hdc, 0, 0);
    OUTLINETEXTMETRIC *otm = (OUTLINETEXTMETRIC *)malloc(size);
    GetOutlineTextMetrics(hdc, size, otm);
    return otm;
}

bool QWindowsFontEngine::hasCFFTable() const
{
    HDC hdc = m_fontEngineData->hdc;
    SelectObject(hdc, hfont);
    return GetFontData(hdc, MAKE_TAG('C', 'F', 'F', ' '), 0, 0, 0) != GDI_ERROR;
}

void QWindowsFontEngine::getCMap()
{
    ttf = (bool)(tm.tmPitchAndFamily & TMPF_TRUETYPE);

    // TMPF_TRUETYPE is not set for fonts with CFF tables
    cffTable = !ttf && hasCFFTable();

    HDC hdc = m_fontEngineData->hdc;
    SelectObject(hdc, hfont);
    bool symb = false;
    if (ttf) {
        cmapTable = getSfntTable(qbswap<quint32>(MAKE_TAG('c', 'm', 'a', 'p')));
        int size = 0;
        cmap = QFontEngine::getCMap(reinterpret_cast<const uchar *>(cmapTable.constData()),
                       cmapTable.size(), &symb, &size);
    }
    if (!cmap) {
        ttf = false;
        symb = false;
    }
    symbol = symb;
    designToDevice = 1;
    _faceId.index = 0;
    if(cmap) {
        OUTLINETEXTMETRIC *otm = getOutlineTextMetric(hdc);
        designToDevice = QFixed((int)otm->otmEMSquare)/int(otm->otmTextMetrics.tmHeight);
        unitsPerEm = otm->otmEMSquare;
        x_height = (int)otm->otmsXHeight;
        loadKerningPairs(designToDevice);
        _faceId.filename = QFile::encodeName(QString::fromWCharArray((wchar_t *)((char *)otm + (quintptr)otm->otmpFullName)));
        lineWidth = otm->otmsUnderscoreSize;
        fsType = otm->otmfsType;
        free(otm);
    } else {
        unitsPerEm = tm.tmHeight;
    }
}

// ### Qt 5.1: replace with QStringIterator
inline unsigned int getChar(const QChar *str, int &i, const int len)
{
    uint uc = str[i].unicode();
    if (QChar::isHighSurrogate(uc) && i < len-1) {
        uint low = str[i+1].unicode();
        if (QChar::isLowSurrogate(low)) {
            uc = QChar::surrogateToUcs4(uc, low);
            ++i;
        }
    }
    return uc;
}

int QWindowsFontEngine::getGlyphIndexes(const QChar *str, int numChars, QGlyphLayout *glyphs, bool mirrored) const
{
    int i = 0;
    int glyph_pos = 0;
    if (mirrored) {
#if defined(Q_OS_WINCE)
        {
#else
        if (symbol) {
            for (; i < numChars; ++i, ++glyph_pos) {
                unsigned int uc = getChar(str, i, numChars);
                glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc);
                if (!glyphs->glyphs[glyph_pos] && uc < 0x100)
                    glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc + 0xf000);
            }
        } else if (ttf) {
            for (; i < numChars; ++i, ++glyph_pos) {
                unsigned int uc = getChar(str, i, numChars);
                glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, QChar::mirroredChar(uc));
            }
        } else {
#endif
            wchar_t first = tm.tmFirstChar;
            wchar_t last = tm.tmLastChar;

            for (; i < numChars; ++i, ++glyph_pos) {
                uint ucs = QChar::mirroredChar(getChar(str, i, numChars));
                if (
#ifdef Q_WS_WINCE
                    tm.tmFirstChar > 60000 ||
#endif
                         ucs >= first && ucs <= last)
                    glyphs->glyphs[glyph_pos] = ucs;
                else
                    glyphs->glyphs[glyph_pos] = 0;
            }
        }
    } else {
#if defined(Q_OS_WINCE)
        {
#else
        if (symbol) {
            for (; i < numChars; ++i, ++glyph_pos) {
                unsigned int uc = getChar(str, i, numChars);
                glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc);
                if(!glyphs->glyphs[glyph_pos] && uc < 0x100)
                    glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc + 0xf000);
            }
        } else if (ttf) {
            for (; i < numChars; ++i, ++glyph_pos) {
                unsigned int uc = getChar(str, i, numChars);
                glyphs->glyphs[glyph_pos] = getTrueTypeGlyphIndex(cmap, uc);
            }
        } else {
#endif
            wchar_t first = tm.tmFirstChar;
            wchar_t last = tm.tmLastChar;

            for (; i < numChars; ++i, ++glyph_pos) {
                uint uc = getChar(str, i, numChars);
                if (
#ifdef Q_WS_WINCE
                    tm.tmFirstChar > 60000 ||
#endif
                         uc >= first && uc <= last)
                    glyphs->glyphs[glyph_pos] = uc;
                else
                    glyphs->glyphs[glyph_pos] = 0;
            }
        }
    }
    glyphs->numGlyphs = glyph_pos;
    return glyph_pos;
}

/*!
    \class QWindowsFontEngine
    \brief Standard Windows font engine.
    \internal
    \ingroup qt-lighthouse-win

    Will probably be superseded by a common Free Type font engine in Qt 5.X.
*/

QWindowsFontEngine::QWindowsFontEngine(const QString &name,
                               HFONT _hfont, bool stockFontIn, LOGFONT lf,
                               const QSharedPointer<QWindowsFontEngineData> &fontEngineData) :
    m_fontEngineData(fontEngineData),
    _name(name),
    hfont(_hfont),
    m_logfont(lf),
    stockFont(stockFontIn),
    ttf(0),
    hasOutline(0),
    lw(0),
    cmap(0),
    lbearing(SHRT_MIN),
    rbearing(SHRT_MIN),
    x_height(-1),
    synthesized_flags(-1),
    lineWidth(-1),
    widthCache(0),
    widthCacheSize(0),
    designAdvances(0),
    designAdvancesSize(0)
{
    if (QWindowsContext::verboseFonts)
        qDebug("%s: font='%s', size=%ld", __FUNCTION__, qPrintable(name), lf.lfHeight);
    HDC hdc = m_fontEngineData->hdc;
    SelectObject(hdc, hfont);
    fontDef.pixelSize = -lf.lfHeight;
    const BOOL res = GetTextMetrics(hdc, &tm);
    fontDef.fixedPitch = !(tm.tmPitchAndFamily & TMPF_FIXED_PITCH);
    if (!res) {
        qErrnoWarning("%s: GetTextMetrics failed", __FUNCTION__);
        ZeroMemory(&tm, sizeof(TEXTMETRIC));
    }

    cache_cost = tm.tmHeight * tm.tmAveCharWidth * 2000;
    getCMap();

    if (!resolvedGetCharWidthI)
        resolveGetCharWidthI();
}

QWindowsFontEngine::~QWindowsFontEngine()
{
    if (designAdvances)
        free(designAdvances);

    if (widthCache)
        free(widthCache);

    // make sure we aren't by accident still selected
    SelectObject(m_fontEngineData->hdc, (HFONT)GetStockObject(SYSTEM_FONT));

    if (!stockFont) {
        if (!DeleteObject(hfont))
            qErrnoWarning("%s: QFontEngineWin: failed to delete non-stock font... failed", __FUNCTION__);
    }
    if (QWindowsContext::verboseFonts)
        if (QWindowsContext::verboseFonts)
            qDebug("%s: font='%s", __FUNCTION__, qPrintable(_name));

    if (!uniqueFamilyName.isEmpty()) {
        QPlatformFontDatabase *pfdb = QWindowsIntegration::instance()->fontDatabase();
        static_cast<QWindowsFontDatabase *>(pfdb)->derefUniqueFont(uniqueFamilyName);
    }
}

HGDIOBJ QWindowsFontEngine::selectDesignFont() const
{
    LOGFONT f = m_logfont;
    f.lfHeight = unitsPerEm;
    HFONT designFont = CreateFontIndirect(&f);
    return SelectObject(m_fontEngineData->hdc, designFont);
}

bool QWindowsFontEngine::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QFontEngine::ShaperFlags flags) const
{
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    glyphs->numGlyphs = *nglyphs;
    *nglyphs = getGlyphIndexes(str, len, glyphs, flags & RightToLeft);

    if (!(flags & GlyphIndicesOnly))
        recalcAdvances(glyphs, flags);

    return true;
}

inline void calculateTTFGlyphWidth(HDC hdc, UINT glyph, int &width)
{
#if defined(Q_OS_WINCE)
    GetCharWidth32(hdc, glyph, glyph, &width);
#else
    if (ptrGetCharWidthI)
        ptrGetCharWidthI(hdc, glyph, 1, 0, &width);
#endif
}

void QWindowsFontEngine::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    HGDIOBJ oldFont = 0;
    HDC hdc = m_fontEngineData->hdc;
    if (ttf && (flags & DesignMetrics)) {
        for(int i = 0; i < glyphs->numGlyphs; i++) {
            unsigned int glyph = glyphs->glyphs[i];
            if(int(glyph) >= designAdvancesSize) {
                int newSize = (glyph + 256) >> 8 << 8;
                designAdvances = q_check_ptr((QFixed *)realloc(designAdvances,
                            newSize*sizeof(QFixed)));
                for(int i = designAdvancesSize; i < newSize; ++i)
                    designAdvances[i] = -1000000;
                designAdvancesSize = newSize;
            }
            if (designAdvances[glyph] < -999999) {
                if (!oldFont)
                    oldFont = selectDesignFont();

                int width = 0;
                calculateTTFGlyphWidth(hdc, glyph, width);
                designAdvances[glyph] = QFixed(width) / designToDevice;
            }
            glyphs->advances_x[i] = designAdvances[glyph];
            glyphs->advances_y[i] = 0;
        }
        if(oldFont)
            DeleteObject(SelectObject(hdc, oldFont));
    } else {
        for(int i = 0; i < glyphs->numGlyphs; i++) {
            unsigned int glyph = glyphs->glyphs[i];

            glyphs->advances_y[i] = 0;

            if (glyph >= widthCacheSize) {
                int newSize = (glyph + 256) >> 8 << 8;
                widthCache = q_check_ptr((unsigned char *)realloc(widthCache,
                            newSize*sizeof(QFixed)));
                memset(widthCache + widthCacheSize, 0, newSize - widthCacheSize);
                widthCacheSize = newSize;
            }
            glyphs->advances_x[i] = widthCache[glyph];
            // font-width cache failed
            if (glyphs->advances_x[i] == 0) {
                int width = 0;
                if (!oldFont)
                    oldFont = SelectObject(hdc, hfont);

                if (!ttf) {
                    QChar ch[2] = { ushort(glyph), 0 };
                    int chrLen = 1;
                    if (QChar::requiresSurrogates(glyph)) {
                        ch[0] = QChar::highSurrogate(glyph);
                        ch[1] = QChar::lowSurrogate(glyph);
                        ++chrLen;
                    }
                    SIZE size = {0, 0};
                    GetTextExtentPoint32(hdc, (wchar_t *)ch, chrLen, &size);
                    width = size.cx;
                } else {
                    calculateTTFGlyphWidth(hdc, glyph, width);
                }
                glyphs->advances_x[i] = width;
                // if glyph's within cache range, store it for later
                if (width > 0 && width < 0x100)
                    widthCache[glyph] = width;
            }
        }

        if (oldFont)
            SelectObject(hdc, oldFont);
    }
}

glyph_metrics_t QWindowsFontEngine::boundingBox(const QGlyphLayout &glyphs)
{
    if (glyphs.numGlyphs == 0)
        return glyph_metrics_t();

    QFixed w = 0;
    for (int i = 0; i < glyphs.numGlyphs; ++i)
        w += glyphs.effectiveAdvance(i);

    return glyph_metrics_t(0, -tm.tmAscent, w - lastRightBearing(glyphs), tm.tmHeight, w, 0);
}
#ifndef Q_OS_WINCE
bool QWindowsFontEngine::getOutlineMetrics(glyph_t glyph, const QTransform &t, glyph_metrics_t *metrics) const
{
    Q_ASSERT(metrics != 0);

    HDC hdc = m_fontEngineData->hdc;

    GLYPHMETRICS gm;
    DWORD res = 0;
    MAT2 mat;
    mat.eM11.value = mat.eM22.value = 1;
    mat.eM11.fract = mat.eM22.fract = 0;
    mat.eM21.value = mat.eM12.value = 0;
    mat.eM21.fract = mat.eM12.fract = 0;

    if (t.type() > QTransform::TxTranslate) {
        // We need to set the transform using the HDC's world
        // matrix rather than using the MAT2 above, because the
        // results provided when transforming via MAT2 does not
        // match the glyphs that are drawn using a WorldTransform
        XFORM xform;
        xform.eM11 = t.m11();
        xform.eM12 = t.m12();
        xform.eM21 = t.m21();
        xform.eM22 = t.m22();
        xform.eDx = 0;
        xform.eDy = 0;
        SetGraphicsMode(hdc, GM_ADVANCED);
        SetWorldTransform(hdc, &xform);
    }

    uint format = GGO_METRICS;
    if (ttf)
        format |= GGO_GLYPH_INDEX;
    res = GetGlyphOutline(hdc, glyph, format, &gm, 0, 0, &mat);

    if (t.type() > QTransform::TxTranslate) {
        XFORM xform;
        xform.eM11 = xform.eM22 = 1;
        xform.eM12 = xform.eM21 = xform.eDx = xform.eDy = 0;
        SetWorldTransform(hdc, &xform);
        SetGraphicsMode(hdc, GM_COMPATIBLE);
    }

    if (res != GDI_ERROR) {
        *metrics = glyph_metrics_t(gm.gmptGlyphOrigin.x, -gm.gmptGlyphOrigin.y,
                                  (int)gm.gmBlackBoxX, (int)gm.gmBlackBoxY, gm.gmCellIncX, gm.gmCellIncY);
        return true;
    } else {
        return false;
    }
}
#endif

glyph_metrics_t QWindowsFontEngine::boundingBox(glyph_t glyph, const QTransform &t)
{
#ifndef Q_OS_WINCE
    HDC hdc = m_fontEngineData->hdc;
    SelectObject(hdc, hfont);

    glyph_metrics_t glyphMetrics;
    bool success = getOutlineMetrics(glyph, t, &glyphMetrics);

    if (!ttf && !success) {
        // Bitmap fonts
        wchar_t ch = glyph;
        ABCFLOAT abc;
        GetCharABCWidthsFloat(hdc, ch, ch, &abc);
        int width = qRound(abc.abcfB);

        return glyph_metrics_t(QFixed::fromReal(abc.abcfA), -tm.tmAscent, width, tm.tmHeight, width, 0).transformed(t);
    }

    return glyphMetrics;
#else
    HDC hdc = m_fontEngineData->hdc;
    HGDIOBJ oldFont = SelectObject(hdc, hfont);

    ABC abc;
    int width;
    int advance;
#ifdef GWES_MGTT    // true type fonts
    if (GetCharABCWidths(hdc, glyph, glyph, &abc)) {
        width = qAbs(abc.abcA) + abc.abcB + qAbs(abc.abcC);
        advance = abc.abcA + abc.abcB + abc.abcC;
    }
    else
#endif
#if defined(GWES_MGRAST) || defined(GWES_MGRAST2)   // raster fonts
    if (GetCharWidth32(hdc, glyph, glyph, &width)) {
        advance = width;
    }
    else
#endif
    {   // fallback
        width = tm.tmMaxCharWidth;
        advance = width;
    }

    SelectObject(hdc, oldFont);
    return glyph_metrics_t(0, -tm.tmAscent, width, tm.tmHeight, advance, 0).transformed(t);
#endif
}

QFixed QWindowsFontEngine::ascent() const
{
    return tm.tmAscent;
}

QFixed QWindowsFontEngine::descent() const
{
    return tm.tmDescent;
}

QFixed QWindowsFontEngine::leading() const
{
    return tm.tmExternalLeading;
}


QFixed QWindowsFontEngine::xHeight() const
{
    if(x_height >= 0)
        return x_height;
    return QFontEngine::xHeight();
}

QFixed QWindowsFontEngine::averageCharWidth() const
{
    return tm.tmAveCharWidth;
}

qreal QWindowsFontEngine::maxCharWidth() const
{
    return tm.tmMaxCharWidth;
}

enum { max_font_count = 256 };
static const ushort char_table[] = {
        40,
        67,
        70,
        75,
        86,
        88,
        89,
        91,
        102,
        114,
        124,
        127,
        205,
        645,
        884,
        922,
        1070,
        12386,
        0
};

static const int char_table_entries = sizeof(char_table)/sizeof(ushort);

#ifndef Q_CC_MINGW
void QWindowsFontEngine::getGlyphBearings(glyph_t glyph, qreal *leftBearing, qreal *rightBearing)
{
    HDC hdc = m_fontEngineData->hdc;
    SelectObject(hdc, hfont);

#ifndef Q_OS_WINCE
    if (ttf)
#endif
    {
        ABC abcWidths;
        GetCharABCWidthsI(hdc, glyph, 1, 0, &abcWidths);
        if (leftBearing)
            *leftBearing = abcWidths.abcA;
        if (rightBearing)
            *rightBearing = abcWidths.abcC;
    }
#ifndef Q_OS_WINCE
    else {
        QFontEngine::getGlyphBearings(glyph, leftBearing, rightBearing);
    }
#endif
}
#endif // Q_CC_MINGW

qreal QWindowsFontEngine::minLeftBearing() const
{
    if (lbearing == SHRT_MIN)
        minRightBearing(); // calculates both

    return lbearing;
}

qreal QWindowsFontEngine::minRightBearing() const
{
#ifndef Q_OS_WINCE
    if (rbearing == SHRT_MIN) {
        int ml = 0;
        int mr = 0;
        HDC hdc = m_fontEngineData->hdc;
        SelectObject(hdc, hfont);
        if (ttf) {
            ABC *abc = 0;
            int n = tm.tmLastChar - tm.tmFirstChar;
            if (n <= max_font_count) {
                abc = new ABC[n+1];
                GetCharABCWidths(hdc, tm.tmFirstChar, tm.tmLastChar, abc);
            } else {
                abc = new ABC[char_table_entries+1];
                for(int i = 0; i < char_table_entries; i++)
                    GetCharABCWidths(hdc, char_table[i], char_table[i], abc + i);
                n = char_table_entries;
            }
            ml = abc[0].abcA;
            mr = abc[0].abcC;
            for (int i = 1; i < n; i++) {
                if (abc[i].abcA + abc[i].abcB + abc[i].abcC != 0) {
                    ml = qMin(ml,abc[i].abcA);
                    mr = qMin(mr,abc[i].abcC);
                }
            }
            delete [] abc;
        } else {
            ABCFLOAT *abc = 0;
            int n = tm.tmLastChar - tm.tmFirstChar+1;
            if (n <= max_font_count) {
                abc = new ABCFLOAT[n];
                GetCharABCWidthsFloat(hdc, tm.tmFirstChar, tm.tmLastChar, abc);
            } else {
                abc = new ABCFLOAT[char_table_entries];
                for(int i = 0; i < char_table_entries; i++)
                    GetCharABCWidthsFloat(hdc, char_table[i], char_table[i], abc+i);
                n = char_table_entries;
            }
            float fml = abc[0].abcfA;
            float fmr = abc[0].abcfC;
            for (int i=1; i<n; i++) {
                if (abc[i].abcfA + abc[i].abcfB + abc[i].abcfC != 0) {
                    fml = qMin(fml,abc[i].abcfA);
                    fmr = qMin(fmr,abc[i].abcfC);
                }
            }
            ml = int(fml - 0.9999);
            mr = int(fmr - 0.9999);
            delete [] abc;
        }
        lbearing = ml;
        rbearing = mr;
    }

    return rbearing;
#else // !Q_OS_WINCE
    if (rbearing == SHRT_MIN) {
        int ml = 0;
        int mr = 0;
        HDC hdc = m_fontEngineData->hdc;
        SelectObject(hdc, hfont);
        if (ttf) {
            ABC *abc = 0;
            int n = tm.tmLastChar - tm.tmFirstChar;
            if (n <= max_font_count) {
                abc = new ABC[n+1];
                GetCharABCWidths(hdc, tm.tmFirstChar, tm.tmLastChar, abc);
            } else {
                abc = new ABC[char_table_entries+1];
                for (int i = 0; i < char_table_entries; i++)
                    GetCharABCWidths(hdc, char_table[i], char_table[i], abc+i);
                n = char_table_entries;
            }
            ml = abc[0].abcA;
            mr = abc[0].abcC;
            for (int i = 1; i < n; i++) {
                if (abc[i].abcA + abc[i].abcB + abc[i].abcC != 0) {
                    ml = qMin(ml,abc[i].abcA);
                    mr = qMin(mr,abc[i].abcC);
                }
            }
            delete [] abc;
        }
        lbearing = ml;
        rbearing = mr;
    }

    return rbearing;
#endif // Q_OS_WINCE
}


const char *QWindowsFontEngine::name() const
{
    return 0;
}

bool QWindowsFontEngine::canRender(const QChar *string,  int len)
{
    if (symbol) {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(string, i, len);
            if (getTrueTypeGlyphIndex(cmap, uc) == 0) {
                if (uc < 0x100) {
                    if (getTrueTypeGlyphIndex(cmap, uc + 0xf000) == 0)
                        return false;
                } else {
                    return false;
                }
            }
        }
    } else if (ttf) {
        for (int i = 0; i < len; ++i) {
            unsigned int uc = getChar(string, i, len);
            if (getTrueTypeGlyphIndex(cmap, uc) == 0)
                return false;
        }
    } else {
        while(len--) {
            if (tm.tmFirstChar > string->unicode() || tm.tmLastChar < string->unicode())
                return false;
        }
    }
    return true;
}

QFontEngine::Type QWindowsFontEngine::type() const
{
    return QFontEngine::Win;
}

static inline double qt_fixed_to_double(const FIXED &p) {
    return ((p.value << 16) + p.fract) / 65536.0;
}

static inline QPointF qt_to_qpointf(const POINTFX &pt, qreal scale) {
    return QPointF(qt_fixed_to_double(pt.x) * scale, -qt_fixed_to_double(pt.y) * scale);
}

#ifndef GGO_UNHINTED
#define GGO_UNHINTED 0x0100
#endif

static bool addGlyphToPath(glyph_t glyph, const QFixedPoint &position, HDC hdc,
                           QPainterPath *path, bool ttf, glyph_metrics_t *metric = 0, qreal scale = 1)
{
    MAT2 mat;
    mat.eM11.value = mat.eM22.value = 1;
    mat.eM11.fract = mat.eM22.fract = 0;
    mat.eM21.value = mat.eM12.value = 0;
    mat.eM21.fract = mat.eM12.fract = 0;
    uint glyphFormat = GGO_NATIVE;

    if (ttf)
        glyphFormat |= GGO_GLYPH_INDEX;

    GLYPHMETRICS gMetric;
    memset(&gMetric, 0, sizeof(GLYPHMETRICS));
    int bufferSize = GDI_ERROR;
    bufferSize = GetGlyphOutline(hdc, glyph, glyphFormat, &gMetric, 0, 0, &mat);
    if ((DWORD)bufferSize == GDI_ERROR) {
        return false;
    }

    void *dataBuffer = new char[bufferSize];
    DWORD ret = GDI_ERROR;
    ret = GetGlyphOutline(hdc, glyph, glyphFormat, &gMetric, bufferSize, dataBuffer, &mat);
    if (ret == GDI_ERROR) {
        delete [](char *)dataBuffer;
        return false;
    }

    if(metric) {
        // #### obey scale
        *metric = glyph_metrics_t(gMetric.gmptGlyphOrigin.x, -gMetric.gmptGlyphOrigin.y,
                                  (int)gMetric.gmBlackBoxX, (int)gMetric.gmBlackBoxY,
                                  gMetric.gmCellIncX, gMetric.gmCellIncY);
    }

    int offset = 0;
    int headerOffset = 0;
    TTPOLYGONHEADER *ttph = 0;

    QPointF oset = position.toPointF();
    while (headerOffset < bufferSize) {
        ttph = (TTPOLYGONHEADER*)((char *)dataBuffer + headerOffset);

        QPointF lastPoint(qt_to_qpointf(ttph->pfxStart, scale));
        path->moveTo(lastPoint + oset);
        offset += sizeof(TTPOLYGONHEADER);
        TTPOLYCURVE *curve;
        while (offset<int(headerOffset + ttph->cb)) {
            curve = (TTPOLYCURVE*)((char*)(dataBuffer) + offset);
            switch (curve->wType) {
            case TT_PRIM_LINE: {
                for (int i=0; i<curve->cpfx; ++i) {
                    QPointF p = qt_to_qpointf(curve->apfx[i], scale) + oset;
                    path->lineTo(p);
                }
                break;
            }
            case TT_PRIM_QSPLINE: {
                const QPainterPath::Element &elm = path->elementAt(path->elementCount()-1);
                QPointF prev(elm.x, elm.y);
                QPointF endPoint;
                for (int i=0; i<curve->cpfx - 1; ++i) {
                    QPointF p1 = qt_to_qpointf(curve->apfx[i], scale) + oset;
                    QPointF p2 = qt_to_qpointf(curve->apfx[i+1], scale) + oset;
                    if (i < curve->cpfx - 2) {
                        endPoint = QPointF((p1.x() + p2.x()) / 2, (p1.y() + p2.y()) / 2);
                    } else {
                        endPoint = p2;
                    }

                    path->quadTo(p1, endPoint);
                    prev = endPoint;
                }

                break;
            }
            case TT_PRIM_CSPLINE: {
                for (int i=0; i<curve->cpfx; ) {
                    QPointF p2 = qt_to_qpointf(curve->apfx[i++], scale) + oset;
                    QPointF p3 = qt_to_qpointf(curve->apfx[i++], scale) + oset;
                    QPointF p4 = qt_to_qpointf(curve->apfx[i++], scale) + oset;
                    path->cubicTo(p2, p3, p4);
                }
                break;
            }
            default:
                qWarning("QFontEngineWin::addOutlineToPath, unhandled switch case");
            }
            offset += sizeof(TTPOLYCURVE) + (curve->cpfx-1) * sizeof(POINTFX);
        }
        path->closeSubpath();
        headerOffset += ttph->cb;
    }
    delete [] (char*)dataBuffer;

    return true;
}

void QWindowsFontEngine::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                                     QPainterPath *path, QTextItem::RenderFlags)
{
    LOGFONT lf = m_logfont;
    // The sign must be negative here to make sure we match against character height instead of
    // hinted cell height. This ensures that we get linear matching, and we need this for
    // paths since we later on apply a scaling transform to the glyph outline to get the
    // font at the correct pixel size.
    lf.lfHeight = -unitsPerEm;
    lf.lfWidth = 0;
    HFONT hf = CreateFontIndirect(&lf);
    HDC hdc = m_fontEngineData->hdc;
    HGDIOBJ oldfont = SelectObject(hdc, hf);

    for(int i = 0; i < nglyphs; ++i) {
        if (!addGlyphToPath(glyphs[i], positions[i], hdc, path, ttf, /*metric*/0,
                            qreal(fontDef.pixelSize) / unitsPerEm)) {
            // Some windows fonts, like "Modern", are vector stroke
            // fonts, which are reported as TMPF_VECTOR but do not
            // support GetGlyphOutline, and thus we set this bit so
            // that addOutLineToPath can check it and return safely...
            hasOutline = false;
            break;
        }
    }
    DeleteObject(SelectObject(hdc, oldfont));
}

void QWindowsFontEngine::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs,
                                      QPainterPath *path, QTextItem::RenderFlags flags)
{
    if(tm.tmPitchAndFamily & (TMPF_TRUETYPE | TMPF_VECTOR)) {
        hasOutline = true;
        QFontEngine::addOutlineToPath(x, y, glyphs, path, flags);
        if (hasOutline)  {
            // has_outline is set to false if addGlyphToPath gets
            // false from GetGlyphOutline, meaning its not an outline
            // font.
            return;
        }
    }
    QFontEngine::addBitmapFontToPath(x, y, glyphs, path, flags);
}

QFontEngine::FaceId QWindowsFontEngine::faceId() const
{
    return _faceId;
}

QT_BEGIN_INCLUDE_NAMESPACE
#include <qdebug.h>
QT_END_INCLUDE_NAMESPACE

int QWindowsFontEngine::synthesized() const
{
    if(synthesized_flags == -1) {
        synthesized_flags = 0;
        if(ttf) {
            const DWORD HEAD = MAKE_TAG('h', 'e', 'a', 'd');
            HDC hdc = m_fontEngineData->hdc;
            SelectObject(hdc, hfont);
            uchar data[4];
            GetFontData(hdc, HEAD, 44, &data, 4);
            USHORT macStyle = getUShort(data);
            if (tm.tmItalic && !(macStyle & 2))
                synthesized_flags = SynthesizedItalic;
            if (fontDef.stretch != 100 && ttf)
                synthesized_flags |= SynthesizedStretch;
            if (tm.tmWeight >= 500 && !(macStyle & 1))
                synthesized_flags |= SynthesizedBold;
            //qDebug() << "font is" << _name <<
            //    "it=" << (macStyle & 2) << fontDef.style << "flags=" << synthesized_flags;
        }
    }
    return synthesized_flags;
}

QFixed QWindowsFontEngine::emSquareSize() const
{
    return unitsPerEm;
}

QFontEngine::Properties QWindowsFontEngine::properties() const
{
    LOGFONT lf = m_logfont;
    lf.lfHeight = unitsPerEm;
    HFONT hf = CreateFontIndirect(&lf);
    HDC hdc = m_fontEngineData->hdc;
    HGDIOBJ oldfont = SelectObject(hdc, hf);
    OUTLINETEXTMETRIC *otm = getOutlineTextMetric(hdc);
    Properties p;
    p.emSquare = unitsPerEm;
    p.italicAngle = otm->otmItalicAngle;
    p.postscriptName = QString::fromWCharArray((wchar_t *)((char *)otm + (quintptr)otm->otmpFamilyName)).toLatin1();
    p.postscriptName += QString::fromWCharArray((wchar_t *)((char *)otm + (quintptr)otm->otmpStyleName)).toLatin1();
    p.postscriptName = QFontEngine::convertToPostscriptFontFamilyName(p.postscriptName);
    p.boundingBox = QRectF(otm->otmrcFontBox.left, -otm->otmrcFontBox.top,
                           otm->otmrcFontBox.right - otm->otmrcFontBox.left,
                           otm->otmrcFontBox.top - otm->otmrcFontBox.bottom);
    p.ascent = otm->otmAscent;
    p.descent = -otm->otmDescent;
    p.leading = (int)otm->otmLineGap;
    p.capHeight = 0;
    p.lineWidth = otm->otmsUnderscoreSize;
    free(otm);
    DeleteObject(SelectObject(hdc, oldfont));
    return p;
}

void QWindowsFontEngine::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics)
{
    LOGFONT lf = m_logfont;
    lf.lfHeight = unitsPerEm;
    int flags = synthesized();
    if(flags & SynthesizedItalic)
        lf.lfItalic = false;
    lf.lfWidth = 0;
    HFONT hf = CreateFontIndirect(&lf);
    HDC hdc = m_fontEngineData->hdc;
    HGDIOBJ oldfont = SelectObject(hdc, hf);
    QFixedPoint p;
    p.x = 0;
    p.y = 0;
    addGlyphToPath(glyph, p, hdc, path, ttf, metrics);
    DeleteObject(SelectObject(hdc, oldfont));
}

bool QWindowsFontEngine::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    if (!ttf && !cffTable)
        return false;
    HDC hdc = m_fontEngineData->hdc;
    SelectObject(hdc, hfont);
    DWORD t = qbswap<quint32>(tag);
    *length = GetFontData(hdc, t, 0, buffer, *length);
    return *length != GDI_ERROR;
}

#if !defined(CLEARTYPE_QUALITY)
#    define CLEARTYPE_QUALITY       5
#endif

QWindowsNativeImage *QWindowsFontEngine::drawGDIGlyph(HFONT font, glyph_t glyph, int margin,
                                                  const QTransform &t,
                                                  QImage::Format mask_format)
{
    Q_UNUSED(mask_format)
    glyph_metrics_t gm = boundingBox(glyph);

//     printf(" -> for glyph %4x\n", glyph);

    int gx = gm.x.toInt();
    int gy = gm.y.toInt();
    int iw = gm.width.toInt();
    int ih = gm.height.toInt();

    if (iw <= 0 || iw <= 0)
        return 0;

    bool has_transformation = t.type() > QTransform::TxTranslate;

#ifndef Q_OS_WINCE
    unsigned int options = ttf ? ETO_GLYPH_INDEX : 0;
    XFORM xform;

    if (has_transformation) {
        xform.eM11 = t.m11();
        xform.eM12 = t.m12();
        xform.eM21 = t.m21();
        xform.eM22 = t.m22();
        xform.eDx = margin;
        xform.eDy = margin;

        const HDC hdc = m_fontEngineData->hdc;

        SetGraphicsMode(hdc, GM_ADVANCED);
        SetWorldTransform(hdc, &xform);
        HGDIOBJ old_font = SelectObject(hdc, font);

        int ggo_options = GGO_METRICS | (ttf ? GGO_GLYPH_INDEX : 0);
        GLYPHMETRICS tgm;
        MAT2 mat;
        memset(&mat, 0, sizeof(mat));
        mat.eM11.value = mat.eM22.value = 1;

        const DWORD result = GetGlyphOutline(hdc, glyph, ggo_options, &tgm, 0, 0, &mat);

        XFORM identity = {1, 0, 0, 1, 0, 0};
        SetWorldTransform(hdc, &identity);
        SetGraphicsMode(hdc, GM_COMPATIBLE);
        SelectObject(hdc, old_font);

        if (result == GDI_ERROR) {
            const int errorCode = GetLastError();
            qErrnoWarning(errorCode, "QWinFontEngine: unable to query transformed glyph metrics (GetGlyphOutline() failed, error %d)...", errorCode);
            return 0;
        }

        iw = tgm.gmBlackBoxX;
        ih = tgm.gmBlackBoxY;

        xform.eDx -= tgm.gmptGlyphOrigin.x;
        xform.eDy += tgm.gmptGlyphOrigin.y;
    }
#else // else wince
    unsigned int options = 0;
    if (has_transformation) {
        qWarning() << "QWindowsFontEngine is unable to apply transformations other than translations for fonts on Windows CE."
                   << "If you need them anyway, start your application with -platform windows:fontengine=freetype.";
   }
#endif // wince
    QWindowsNativeImage *ni = new QWindowsNativeImage(iw + 2 * margin + 4,
                                                      ih + 2 * margin + 4,
                                                      QWindowsNativeImage::systemFormat());

    /*If cleartype is enabled we use the standard system format even on Windows CE
      and not the special textbuffer format we have to use if cleartype is disabled*/

    ni->image().fill(0xffffffff);

    HDC hdc = ni->hdc();

    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    SetTextColor(hdc, RGB(0,0,0));
    SetBkMode(hdc, TRANSPARENT);
    SetTextAlign(hdc, TA_BASELINE);

    HGDIOBJ old_font = SelectObject(hdc, font);

#ifndef Q_OS_WINCE
    if (has_transformation) {
        SetGraphicsMode(hdc, GM_ADVANCED);
        SetWorldTransform(hdc, &xform);
        ExtTextOut(hdc, 0, 0, options, 0, (LPCWSTR) &glyph, 1, 0);
    } else
#endif // !Q_OS_WINCE
    {
        ExtTextOut(hdc, -gx + margin, -gy + margin, options, 0, (LPCWSTR) &glyph, 1, 0);
    }

    SelectObject(hdc, old_font);
    return ni;
}

QImage QWindowsFontEngine::alphaMapForGlyph(glyph_t glyph, const QTransform &xform)
{
    HFONT font = hfont;
    if (m_fontEngineData->clearTypeEnabled) {
        LOGFONT lf = m_logfont;
        lf.lfQuality = ANTIALIASED_QUALITY;
        font = CreateFontIndirect(&lf);
    }
    QImage::Format mask_format = QWindowsNativeImage::systemFormat();
    mask_format = QImage::Format_RGB32;

    QWindowsNativeImage *mask = drawGDIGlyph(font, glyph, 0, xform, mask_format);
    if (mask == 0) {
        if (m_fontEngineData->clearTypeEnabled)
            DeleteObject(font);
        return QImage();
    }

    QImage indexed(mask->width(), mask->height(), QImage::Format_Indexed8);

    // ### This part is kinda pointless, but we'll crash later if we don't because some
    // code paths expects there to be colortables for index8-bit...
    QVector<QRgb> colors(256);
    for (int i=0; i<256; ++i)
        colors[i] = qRgba(0, 0, 0, i);
    indexed.setColorTable(colors);

    // Copy data... Cannot use QPainter here as GDI has messed up the
    // Alpha channel of the ni.image pixels...
    for (int y=0; y<mask->height(); ++y) {
        uchar *dest = indexed.scanLine(y);
        if (mask->image().format() == QImage::Format_RGB16) {
            const qint16 *src = (qint16 *) ((const QImage &) mask->image()).scanLine(y);
            for (int x=0; x<mask->width(); ++x)
                dest[x] = 255 - qGray(src[x]);
        } else {
            const uint *src = (uint *) ((const QImage &) mask->image()).scanLine(y);
            for (int x=0; x<mask->width(); ++x) {
                if (QWindowsNativeImage::systemFormat() == QImage::Format_RGB16)
                    dest[x] = 255 - qGray(src[x]);
                else
                    dest[x] = 255 - (m_fontEngineData->pow_gamma[qGray(src[x])] * 255. / 2047.);
            }
        }
    }

    // Cleanup...
    delete mask;
    if (m_fontEngineData->clearTypeEnabled) {
        DeleteObject(font);
    }

    return indexed;
}

#define SPI_GETFONTSMOOTHINGCONTRAST           0x200C
#define SPI_SETFONTSMOOTHINGCONTRAST           0x200D

QImage QWindowsFontEngine::alphaRGBMapForGlyph(glyph_t glyph, QFixed, const QTransform &t)
{
    HFONT font = hfont;

    UINT contrast;
    SystemParametersInfo(SPI_GETFONTSMOOTHINGCONTRAST, 0, &contrast, 0);
    SystemParametersInfo(SPI_SETFONTSMOOTHINGCONTRAST, 0, (void *) 1000, 0);

    int margin = glyphMargin(QFontEngineGlyphCache::Raster_RGBMask);
    QWindowsNativeImage *mask = drawGDIGlyph(font, glyph, margin, t, QImage::Format_RGB32);
    SystemParametersInfo(SPI_SETFONTSMOOTHINGCONTRAST, 0, (void *) quintptr(contrast), 0);

    if (mask == 0)
        return QImage();

    // Gracefully handle the odd case when the display is 16-bit
    const QImage source = mask->image().depth() == 32
                          ? mask->image()
                          : mask->image().convertToFormat(QImage::Format_RGB32);

    QImage rgbMask(mask->width(), mask->height(), QImage::Format_RGB32);
    for (int y=0; y<mask->height(); ++y) {
        uint *dest = (uint *) rgbMask.scanLine(y);
        const uint *src = (uint *) source.scanLine(y);
        for (int x=0; x<mask->width(); ++x) {
            dest[x] = 0xffffffff - (0x00ffffff & src[x]);
        }
    }

    delete mask;

    return rgbMask;
}

QFontEngine *QWindowsFontEngine::cloneWithSize(qreal pixelSize) const
{
    QFontDef request = fontDef;
    QString actualFontName = request.family;
    if (!uniqueFamilyName.isEmpty())
        request.family = uniqueFamilyName;
    request.pixelSize = pixelSize;
    // Disable font merging, as otherwise createEngine will return a multi-engine
    // instance instead of the specific engine we wish to clone.
    request.styleStrategy |= QFont::NoFontMerging;

    QFontEngine *fontEngine =
        QWindowsFontDatabase::createEngine(QChar::Script_Common, request, 0,
                                           QWindowsContext::instance()->defaultDPI(),
                                           false,
                                           QStringList(), m_fontEngineData);
    if (fontEngine) {
        fontEngine->fontDef.family = actualFontName;
        if (!uniqueFamilyName.isEmpty()) {
            static_cast<QWindowsFontEngine *>(fontEngine)->setUniqueFamilyName(uniqueFamilyName);
            QPlatformFontDatabase *pfdb = QWindowsIntegration::instance()->fontDatabase();
            static_cast<QWindowsFontDatabase *>(pfdb)->refUniqueFont(uniqueFamilyName);
        }
    }
    return fontEngine;
}

void QWindowsFontEngine::initFontInfo(const QFontDef &request,
                                      HDC fontHdc,
                                      int dpi)
{
    fontDef = request; // most settings are equal
    HDC dc = ((request.styleStrategy & QFont::PreferDevice) && fontHdc) ? fontHdc : m_fontEngineData->hdc;
    SelectObject(dc, hfont);
    wchar_t n[64];
    GetTextFace(dc, 64, n);
    fontDef.family = QString::fromWCharArray(n);
    fontDef.fixedPitch = !(tm.tmPitchAndFamily & TMPF_FIXED_PITCH);
    if (fontDef.pointSize < 0) {
        fontDef.pointSize = fontDef.pixelSize * 72. / dpi;
    } else if (fontDef.pixelSize == -1) {
        fontDef.pixelSize = qRound(fontDef.pointSize * dpi / 72.);
    }
}

/*!
    \class QWindowsMultiFontEngine
    \brief Standard Windows Multi font engine.
    \internal
    \ingroup qt-lighthouse-win

    "Merges" several font engines that have gaps in the
    supported writing systems.

    Will probably be superseded by a common Free Type font engine in Qt 5.X.
*/

QWindowsMultiFontEngine::QWindowsMultiFontEngine(QFontEngine *first, const QStringList &fallbacks)
        : QFontEngineMulti(fallbacks.size()+1),
          fallbacks(fallbacks)
{
    if (QWindowsContext::verboseFonts)
        qDebug() <<  __FUNCTION__ << engines.size() << first << first->fontDef.family << fallbacks;
    engines[0] = first;
    first->ref.ref();
    fontDef = engines[0]->fontDef;
    cache_cost = first->cache_cost;
}

QWindowsMultiFontEngine::~QWindowsMultiFontEngine()
{
    if (QWindowsContext::verboseFonts)
        qDebug("%s", __FUNCTION__);
}

void QWindowsMultiFontEngine::loadEngine(int at)
{
    Q_ASSERT(at < engines.size());
    Q_ASSERT(engines.at(at) == 0);

    QFontEngine *fontEngine = engines.at(0);
    QSharedPointer<QWindowsFontEngineData> data;
    LOGFONT lf;

#ifndef QT_NO_DIRECTWRITE
    if (fontEngine->type() == QFontEngine::DirectWrite) {
        QWindowsFontEngineDirectWrite *fe = static_cast<QWindowsFontEngineDirectWrite *>(fontEngine);
        lf = QWindowsFontDatabase::fontDefToLOGFONT(fe->fontDef);

        data = fe->fontEngineData();
    } else
#endif
    {
        QWindowsFontEngine *fe = static_cast<QWindowsFontEngine*>(fontEngine);
        lf = fe->logFont();

        data = fe->fontEngineData();
    }

    const QString fam = fallbacks.at(at-1);
    memcpy(lf.lfFaceName, fam.utf16(), sizeof(wchar_t) * qMin(fam.length() + 1, 32));  // 32 = Windows hard-coded

#ifndef QT_NO_DIRECTWRITE
    if (fontEngine->type() == QFontEngine::DirectWrite) {
        const QString nameSubstitute = QWindowsFontEngineDirectWrite::fontNameSubstitute(QString::fromWCharArray(lf.lfFaceName));
        memcpy(lf.lfFaceName, nameSubstitute.utf16(),
               sizeof(wchar_t) * qMin(nameSubstitute.length() + 1, LF_FACESIZE));

        IDWriteFont *directWriteFont = 0;
        HRESULT hr = data->directWriteGdiInterop->CreateFontFromLOGFONT(&lf, &directWriteFont);
        if (FAILED(hr)) {
            qErrnoWarning("%s: CreateFontFromLOGFONT failed", __FUNCTION__);
        } else {
            IDWriteFontFace *directWriteFontFace = NULL;
            HRESULT hr = directWriteFont->CreateFontFace(&directWriteFontFace);
            if (SUCCEEDED(hr)) {
                QWindowsFontEngineDirectWrite *fedw = new QWindowsFontEngineDirectWrite(directWriteFontFace,
                                                                                        fontEngine->fontDef.pixelSize,
                                                                                        data);
                fedw->setObjectName(QStringLiteral("QWindowsFontEngineDirectWrite_") + fontEngine->fontDef.family);

                fedw->fontDef = fontDef;
                fedw->ref.ref();
                engines[at] = fedw;

                if (QWindowsContext::verboseFonts)
                    qDebug("%s %d %s", __FUNCTION__, at, qPrintable(fam));

                return;
            } else {
                qErrnoWarning("%s: CreateFontFace failed", __FUNCTION__);
            }

        }
    }
#endif

    // Get here if original font is not DirectWrite or DirectWrite creation failed for some
    // reason
    HFONT hfont = CreateFontIndirect(&lf);

    bool stockFont = false;
    if (hfont == 0) {
        hfont = (HFONT)GetStockObject(ANSI_VAR_FONT);
        stockFont = true;
    }
    engines[at] = new QWindowsFontEngine(fam, hfont, stockFont, lf, data);
    engines[at]->ref.ref();
    engines[at]->fontDef = fontDef;
    if (QWindowsContext::verboseFonts)
        qDebug("%s %d %s", __FUNCTION__, at, qPrintable(fam));


    // TODO: increase cost in QFontCache for the font engine loaded here
}

bool QWindowsFontEngine::supportsTransformation(const QTransform &transform) const
{
    // Support all transformations for ttf files, and translations for raster fonts
    return ttf || transform.type() <= QTransform::TxTranslate;
}

QT_END_NAMESPACE

