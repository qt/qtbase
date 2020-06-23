/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdir.h"
#include "qmetatype.h"
#include "qtextstream.h"
#include "qvariant.h"
#include "qfontengine_ft_p.h"
#include "private/qimage_p.h"
#include <private/qstringiterator_p.h>
#include <qguiapplication.h>
#include <qscreen.h>
#include <qpa/qplatformscreen.h>
#include <QtCore/QUuid>
#include <QtGui/QPainterPath>

#ifndef QT_NO_FREETYPE

#include "qfile.h"
#include "qfileinfo.h"
#include <qscopedvaluerollback.h>
#include "qthreadstorage.h"
#include <qmath.h>
#include <qendian.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_SYNTHESIS_H
#include FT_TRUETYPE_TABLES_H
#include FT_TYPE1_TABLES_H
#include FT_GLYPH_H
#include FT_MODULE_H
#include FT_LCD_FILTER_H

#if defined(FT_CONFIG_OPTIONS_H)
#include FT_CONFIG_OPTIONS_H
#endif

#if defined(FT_FONT_FORMATS_H)
#include FT_FONT_FORMATS_H
#endif

#ifdef QT_LINUXBASE
#include FT_ERRORS_H
#endif

#if !defined(QT_MAX_CACHED_GLYPH_SIZE)
#  define QT_MAX_CACHED_GLYPH_SIZE 64
#endif

QT_BEGIN_NAMESPACE

#define FLOOR(x)    ((x) & -64)
#define CEIL(x)     (((x)+63) & -64)
#define TRUNC(x)    ((x) >> 6)
#define ROUND(x)    (((x)+32) & -64)

static bool ft_getSfntTable(void *user_data, uint tag, uchar *buffer, uint *length)
{
    FT_Face face = (FT_Face)user_data;

    bool result = false;
    if (FT_IS_SFNT(face)) {
        FT_ULong len = *length;
        result = FT_Load_Sfnt_Table(face, tag, 0, buffer, &len) == FT_Err_Ok;
        *length = len;
        Q_ASSERT(!result || int(*length) > 0);
    }

    return result;
}

static QFontEngineFT::Glyph emptyGlyph;

static const QFontEngine::HintStyle ftInitialDefaultHintStyle =
#ifdef Q_OS_WIN
    QFontEngineFT::HintFull;
#else
    QFontEngineFT::HintNone;
#endif

// -------------------------- Freetype support ------------------------------

class QtFreetypeData
{
public:
    QtFreetypeData()
        : library(nullptr)
    { }
    ~QtFreetypeData();

    FT_Library library;
    QHash<QFontEngine::FaceId, QFreetypeFace *> faces;
};

QtFreetypeData::~QtFreetypeData()
{
    for (QHash<QFontEngine::FaceId, QFreetypeFace *>::ConstIterator iter = faces.cbegin(); iter != faces.cend(); ++iter)
        iter.value()->cleanup();
    faces.clear();
    FT_Done_FreeType(library);
    library = nullptr;
}

Q_GLOBAL_STATIC(QThreadStorage<QtFreetypeData *>, theFreetypeData)

QtFreetypeData *qt_getFreetypeData()
{
    QtFreetypeData *&freetypeData = theFreetypeData()->localData();
    if (!freetypeData)
        freetypeData = new QtFreetypeData;
    if (!freetypeData->library) {
        FT_Init_FreeType(&freetypeData->library);
#if defined(FT_FONT_FORMATS_H)
        // Freetype defaults to disabling stem-darkening on CFF, we re-enable it.
        FT_Bool no_darkening = false;
        FT_Property_Set(freetypeData->library, "cff", "no-stem-darkening", &no_darkening);
#endif
    }
    return freetypeData;
}

FT_Library qt_getFreetype()
{
    QtFreetypeData *freetypeData = qt_getFreetypeData();
    Q_ASSERT(freetypeData->library);
    return freetypeData->library;
}

int QFreetypeFace::fsType() const
{
    int fsType = 0;
    TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    if (os2)
        fsType = os2->fsType;
    return fsType;
}

int QFreetypeFace::getPointInOutline(glyph_t glyph, int flags, quint32 point, QFixed *xpos, QFixed *ypos, quint32 *nPoints)
{
    if (int error = FT_Load_Glyph(face, glyph, flags))
        return error;

    if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
        return Err_Invalid_SubTable;

    *nPoints = face->glyph->outline.n_points;
    if (!(*nPoints))
        return Err_Ok;

    if (point > *nPoints)
        return Err_Invalid_SubTable;

    *xpos = QFixed::fromFixed(face->glyph->outline.points[point].x);
    *ypos = QFixed::fromFixed(face->glyph->outline.points[point].y);

    return Err_Ok;
}

bool QFreetypeFace::isScalableBitmap() const
{
#ifdef FT_HAS_COLOR
    return !FT_IS_SCALABLE(face) && FT_HAS_COLOR(face);
#else
    return false;
#endif
}

extern QByteArray qt_fontdata_from_index(int);

/*
 * One font file can contain more than one font (bold/italic for example)
 * find the right one and return it.
 *
 * Returns the freetype face or 0 in case of an empty file or any other problems
 * (like not being able to open the file)
 */
QFreetypeFace *QFreetypeFace::getFace(const QFontEngine::FaceId &face_id,
                                      const QByteArray &fontData)
{
    if (face_id.filename.isEmpty() && fontData.isEmpty())
        return nullptr;

    QtFreetypeData *freetypeData = qt_getFreetypeData();

    QFreetypeFace *freetype = freetypeData->faces.value(face_id, 0);
    if (freetype) {
        freetype->ref.ref();
    } else {
        QScopedPointer<QFreetypeFace> newFreetype(new QFreetypeFace);
        FT_Face face;
        if (!face_id.filename.isEmpty()) {
            QString fileName = QFile::decodeName(face_id.filename);
            if (face_id.filename.startsWith(":qmemoryfonts/")) {
                // from qfontdatabase.cpp
                QByteArray idx = face_id.filename;
                idx.remove(0, 14); // remove ':qmemoryfonts/'
                bool ok = false;
                newFreetype->fontData = qt_fontdata_from_index(idx.toInt(&ok));
                if (!ok)
                    newFreetype->fontData = QByteArray();
            } else if (!QFileInfo(fileName).isNativePath()) {
                QFile file(fileName);
                if (!file.open(QIODevice::ReadOnly)) {
                    return nullptr;
                }
                newFreetype->fontData = file.readAll();
            }
        } else {
            newFreetype->fontData = fontData;
        }
        if (!newFreetype->fontData.isEmpty()) {
            if (FT_New_Memory_Face(freetypeData->library, (const FT_Byte *)newFreetype->fontData.constData(), newFreetype->fontData.size(), face_id.index, &face)) {
                return nullptr;
            }
        } else if (FT_New_Face(freetypeData->library, face_id.filename, face_id.index, &face)) {
            return nullptr;
        }
        newFreetype->face = face;

        newFreetype->ref.storeRelaxed(1);
        newFreetype->xsize = 0;
        newFreetype->ysize = 0;
        newFreetype->matrix.xx = 0x10000;
        newFreetype->matrix.yy = 0x10000;
        newFreetype->matrix.xy = 0;
        newFreetype->matrix.yx = 0;
        newFreetype->unicode_map = nullptr;
        newFreetype->symbol_map = nullptr;

        memset(newFreetype->cmapCache, 0, sizeof(newFreetype->cmapCache));

        for (int i = 0; i < newFreetype->face->num_charmaps; ++i) {
            FT_CharMap cm = newFreetype->face->charmaps[i];
            switch(cm->encoding) {
            case FT_ENCODING_UNICODE:
                newFreetype->unicode_map = cm;
                break;
            case FT_ENCODING_APPLE_ROMAN:
            case FT_ENCODING_ADOBE_LATIN_1:
                if (!newFreetype->unicode_map || newFreetype->unicode_map->encoding != FT_ENCODING_UNICODE)
                    newFreetype->unicode_map = cm;
                break;
            case FT_ENCODING_ADOBE_CUSTOM:
            case FT_ENCODING_MS_SYMBOL:
                if (!newFreetype->symbol_map)
                    newFreetype->symbol_map = cm;
                break;
            default:
                break;
            }
        }

        if (!FT_IS_SCALABLE(newFreetype->face) && newFreetype->face->num_fixed_sizes == 1)
            FT_Set_Char_Size(face, newFreetype->face->available_sizes[0].x_ppem, newFreetype->face->available_sizes[0].y_ppem, 0, 0);

        FT_Set_Charmap(newFreetype->face, newFreetype->unicode_map);
        QT_TRY {
            freetypeData->faces.insert(face_id, newFreetype.data());
        } QT_CATCH(...) {
            newFreetype.take()->release(face_id);
            // we could return null in principle instead of throwing
            QT_RETHROW;
        }
        freetype = newFreetype.take();
    }
    return freetype;
}

void QFreetypeFace::cleanup()
{
    hbFace.reset();
    FT_Done_Face(face);
    face = nullptr;
}

void QFreetypeFace::release(const QFontEngine::FaceId &face_id)
{
    if (!ref.deref()) {
        if (face) {
            QtFreetypeData *freetypeData = qt_getFreetypeData();

            cleanup();

            auto it = freetypeData->faces.constFind(face_id);
            if (it != freetypeData->faces.constEnd())
                freetypeData->faces.erase(it);

            if (freetypeData->faces.isEmpty()) {
                FT_Done_FreeType(freetypeData->library);
                freetypeData->library = nullptr;
            }
        }

        delete this;
    }
}


void QFreetypeFace::computeSize(const QFontDef &fontDef, int *xsize, int *ysize, bool *outline_drawing, QFixed *scalableBitmapScaleFactor)
{
    *ysize = qRound(fontDef.pixelSize * 64);
    *xsize = *ysize * fontDef.stretch / 100;
    *scalableBitmapScaleFactor = 1;
    *outline_drawing = false;

    if (!(face->face_flags & FT_FACE_FLAG_SCALABLE)) {
        int best = 0;
        if (!isScalableBitmap()) {
            /*
             * Bitmap only faces must match exactly, so find the closest
             * one (height dominant search)
             */
            for (int i = 1; i < face->num_fixed_sizes; i++) {
                if (qAbs(*ysize -  face->available_sizes[i].y_ppem) <
                    qAbs(*ysize - face->available_sizes[best].y_ppem) ||
                    (qAbs(*ysize - face->available_sizes[i].y_ppem) ==
                     qAbs(*ysize - face->available_sizes[best].y_ppem) &&
                     qAbs(*xsize - face->available_sizes[i].x_ppem) <
                     qAbs(*xsize - face->available_sizes[best].x_ppem))) {
                    best = i;
                }
            }
        } else {
            // Select the shortest bitmap strike whose height is larger than the desired height
            for (int i = 1; i < face->num_fixed_sizes; i++) {
                if (face->available_sizes[i].y_ppem < *ysize) {
                    if (face->available_sizes[i].y_ppem > face->available_sizes[best].y_ppem)
                        best = i;
                } else if (face->available_sizes[best].y_ppem < *ysize) {
                    best = i;
                } else if (face->available_sizes[i].y_ppem < face->available_sizes[best].y_ppem) {
                    best = i;
                }
            }
        }

        // According to freetype documentation we must use FT_Select_Size
        // to make sure we can select the desired bitmap strike index
        if (FT_Select_Size(face, best) == 0) {
            if (isScalableBitmap())
                *scalableBitmapScaleFactor = QFixed::fromReal((qreal)fontDef.pixelSize / face->available_sizes[best].height);
            *xsize = face->available_sizes[best].x_ppem;
            *ysize = face->available_sizes[best].y_ppem;
        } else {
            *xsize = *ysize = 0;
        }
    } else {
        *outline_drawing = (*xsize > (QT_MAX_CACHED_GLYPH_SIZE<<6) || *ysize > (QT_MAX_CACHED_GLYPH_SIZE<<6));
    }
}

QFontEngine::Properties QFreetypeFace::properties() const
{
    QFontEngine::Properties p;
    p.postscriptName = FT_Get_Postscript_Name(face);
    PS_FontInfoRec font_info;
    if (FT_Get_PS_Font_Info(face, &font_info) == 0)
        p.copyright = font_info.notice;
    if (FT_IS_SCALABLE(face)) {
        p.ascent = face->ascender;
        p.descent = -face->descender;
        p.leading = face->height - face->ascender + face->descender;
        p.emSquare = face->units_per_EM;
        p.boundingBox = QRectF(face->bbox.xMin, -face->bbox.yMax,
                               face->bbox.xMax - face->bbox.xMin,
                               face->bbox.yMax - face->bbox.yMin);
    } else {
        p.ascent = QFixed::fromFixed(face->size->metrics.ascender);
        p.descent = QFixed::fromFixed(-face->size->metrics.descender);
        p.leading = QFixed::fromFixed(face->size->metrics.height - face->size->metrics.ascender + face->size->metrics.descender);
        p.emSquare = face->size->metrics.y_ppem;
//        p.boundingBox = QRectF(-p.ascent.toReal(), 0, (p.ascent + p.descent).toReal(), face->size->metrics.max_advance/64.);
        p.boundingBox = QRectF(0, -p.ascent.toReal(),
                               face->size->metrics.max_advance/64, (p.ascent + p.descent).toReal() );
    }
    p.italicAngle = 0;
    p.capHeight = p.ascent;
    p.lineWidth = face->underline_thickness;

    return p;
}

bool QFreetypeFace::getSfntTable(uint tag, uchar *buffer, uint *length) const
{
    return ft_getSfntTable(face, tag, buffer, length);
}

/* Some fonts (such as MingLiu rely on hinting to scale different
   components to their correct sizes. While this is really broken (it
   should be done in the component glyph itself, not the hinter) we
   will have to live with it.

   This means we can not use FT_LOAD_NO_HINTING to get the glyph
   outline. All we can do is to load the unscaled glyph and scale it
   down manually when required.
*/
static void scaleOutline(FT_Face face, FT_GlyphSlot g, FT_Fixed x_scale, FT_Fixed y_scale)
{
    x_scale = FT_MulDiv(x_scale, 1 << 10, face->units_per_EM);
    y_scale = FT_MulDiv(y_scale, 1 << 10, face->units_per_EM);
    FT_Vector *p = g->outline.points;
    const FT_Vector *e = p + g->outline.n_points;
    while (p < e) {
        p->x = FT_MulFix(p->x, x_scale);
        p->y = FT_MulFix(p->y, y_scale);
        ++p;
    }
}

#define GLYPH2PATH_DEBUG QT_NO_QDEBUG_MACRO // qDebug
void QFreetypeFace::addGlyphToPath(FT_Face face, FT_GlyphSlot g, const QFixedPoint &point, QPainterPath *path, FT_Fixed x_scale, FT_Fixed y_scale)
{
    const qreal factor = 1/64.;
    scaleOutline(face, g, x_scale, y_scale);

    QPointF cp = point.toPointF();

    // convert the outline to a painter path
    int i = 0;
    for (int j = 0; j < g->outline.n_contours; ++j) {
        int last_point = g->outline.contours[j];
        GLYPH2PATH_DEBUG() << "contour:" << i << "to" << last_point;
        QPointF start = QPointF(g->outline.points[i].x*factor, -g->outline.points[i].y*factor);
        if (!(g->outline.tags[i] & 1)) {               // start point is not on curve:
            if (!(g->outline.tags[last_point] & 1)) {  // end point is not on curve:
                GLYPH2PATH_DEBUG() << "  start and end point are not on curve";
                start = (QPointF(g->outline.points[last_point].x*factor,
                                -g->outline.points[last_point].y*factor) + start) / 2.0;
            } else {
                GLYPH2PATH_DEBUG() << "  end point is on curve, start is not";
                start = QPointF(g->outline.points[last_point].x*factor,
                               -g->outline.points[last_point].y*factor);
            }
            --i;   // to use original start point as control point below
        }
        start += cp;
        GLYPH2PATH_DEBUG() << "  start at" << start;

        path->moveTo(start);
        QPointF c[4];
        c[0] = start;
        int n = 1;
        while (i < last_point) {
            ++i;
            c[n] = cp + QPointF(g->outline.points[i].x*factor, -g->outline.points[i].y*factor);
            GLYPH2PATH_DEBUG() << "    " << i << c[n] << "tag =" << (int)g->outline.tags[i]
                               << ": on curve =" << (bool)(g->outline.tags[i] & 1);
            ++n;
            switch (g->outline.tags[i] & 3) {
            case 2:
                // cubic bezier element
                if (n < 4)
                    continue;
                c[3] = (c[3] + c[2])/2;
                --i;
                break;
            case 0:
                // quadratic bezier element
                if (n < 3)
                    continue;
                c[3] = (c[1] + c[2])/2;
                c[2] = (2*c[1] + c[3])/3;
                c[1] = (2*c[1] + c[0])/3;
                --i;
                break;
            case 1:
            case 3:
                if (n == 2) {
                    GLYPH2PATH_DEBUG() << "  lineTo" << c[1];
                    path->lineTo(c[1]);
                    c[0] = c[1];
                    n = 1;
                    continue;
                } else if (n == 3) {
                    c[3] = c[2];
                    c[2] = (2*c[1] + c[3])/3;
                    c[1] = (2*c[1] + c[0])/3;
                }
                break;
            }
            GLYPH2PATH_DEBUG() << "  cubicTo" << c[1] << c[2] << c[3];
            path->cubicTo(c[1], c[2], c[3]);
            c[0] = c[3];
            n = 1;
        }

        if (n == 1) {
            GLYPH2PATH_DEBUG() << "  closeSubpath";
            path->closeSubpath();
        } else {
            c[3] = start;
            if (n == 2) {
                c[2] = (2*c[1] + c[3])/3;
                c[1] = (2*c[1] + c[0])/3;
            }
            GLYPH2PATH_DEBUG() << "  close cubicTo" << c[1] << c[2] << c[3];
            path->cubicTo(c[1], c[2], c[3]);
        }
        ++i;
    }
}

extern void qt_addBitmapToPath(qreal x0, qreal y0, const uchar *image_data, int bpl, int w, int h, QPainterPath *path);

void QFreetypeFace::addBitmapToPath(FT_GlyphSlot slot, const QFixedPoint &point, QPainterPath *path)
{
    if (slot->format != FT_GLYPH_FORMAT_BITMAP
        || slot->bitmap.pixel_mode != FT_PIXEL_MODE_MONO)
        return;

    QPointF cp = point.toPointF();
    qt_addBitmapToPath(cp.x() + TRUNC(slot->metrics.horiBearingX), cp.y() - TRUNC(slot->metrics.horiBearingY),
                       slot->bitmap.buffer, slot->bitmap.pitch, slot->bitmap.width, slot->bitmap.rows, path);
}

static inline void convertRGBToARGB(const uchar *src, uint *dst, int width, int height, int src_pitch, bool bgr)
{
    const int offs = bgr ? -1 : 1;
    const int w = width * 3;
    while (height--) {
        uint *dd = dst;
        for (int x = 0; x < w; x += 3) {
            uchar red = src[x + 1 - offs];
            uchar green = src[x + 1];
            uchar blue = src[x + 1 + offs];
            *dd++ = (0xFFU << 24) | (red << 16) | (green << 8) | blue;
        }
        dst += width;
        src += src_pitch;
    }
}

static inline void convertRGBToARGB_V(const uchar *src, uint *dst, int width, int height, int src_pitch, bool bgr)
{
    const int offs = bgr ? -src_pitch : src_pitch;
    while (height--) {
        for (int x = 0; x < width; x++) {
            uchar red = src[x + src_pitch - offs];
            uchar green = src[x + src_pitch];
            uchar blue = src[x + src_pitch + offs];
            *dst++ = (0XFFU << 24) | (red << 16) | (green << 8) | blue;
        }
        src += 3*src_pitch;
    }
}

static QFontEngine::SubpixelAntialiasingType subpixelAntialiasingTypeHint()
{
    static int type = -1;
    if (type == -1) {
        if (QScreen *screen = QGuiApplication::primaryScreen())
            type = screen->handle()->subpixelAntialiasingTypeHint();
    }
    return static_cast<QFontEngine::SubpixelAntialiasingType>(type);
}

QFontEngineFT *QFontEngineFT::create(const QFontDef &fontDef, FaceId faceId, const QByteArray &fontData)
{
    QScopedPointer<QFontEngineFT> engine(new QFontEngineFT(fontDef));

    QFontEngineFT::GlyphFormat format = QFontEngineFT::Format_Mono;
    const bool antialias = !(fontDef.styleStrategy & QFont::NoAntialias);

    if (antialias) {
        QFontEngine::SubpixelAntialiasingType subpixelType = subpixelAntialiasingTypeHint();
        if (subpixelType == QFontEngine::Subpixel_None || (fontDef.styleStrategy & QFont::NoSubpixelAntialias)) {
            format = QFontEngineFT::Format_A8;
            engine->subpixelType = QFontEngine::Subpixel_None;
        } else {
            format = QFontEngineFT::Format_A32;
            engine->subpixelType = subpixelType;
        }
    }

    if (!engine->init(faceId, antialias, format, fontData) || engine->invalid()) {
        qWarning("QFontEngineFT: Failed to create FreeType font engine");
        return nullptr;
    }

    engine->setQtDefaultHintStyle(static_cast<QFont::HintingPreference>(fontDef.hintingPreference));
    return engine.take();
}

namespace {
    class QFontEngineFTRawData: public QFontEngineFT
    {
    public:
        QFontEngineFTRawData(const QFontDef &fontDef) : QFontEngineFT(fontDef)
        {
        }

        void updateFamilyNameAndStyle()
        {
            fontDef.family = QString::fromLatin1(freetype->face->family_name);

            if (freetype->face->style_flags & FT_STYLE_FLAG_ITALIC)
                fontDef.style = QFont::StyleItalic;

            if (freetype->face->style_flags & FT_STYLE_FLAG_BOLD)
                fontDef.weight = QFont::Bold;
        }

        bool initFromData(const QByteArray &fontData)
        {
            FaceId faceId;
            faceId.filename = "";
            faceId.index = 0;
            faceId.uuid = QUuid::createUuid().toByteArray();

            return init(faceId, true, Format_None, fontData);
        }
    };
}

QFontEngineFT *QFontEngineFT::create(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    QFontDef fontDef;
    fontDef.pixelSize = pixelSize;
    fontDef.stretch = QFont::Unstretched;
    fontDef.hintingPreference = hintingPreference;

    QFontEngineFTRawData *fe = new QFontEngineFTRawData(fontDef);
    if (!fe->initFromData(fontData)) {
        delete fe;
        return nullptr;
    }

    fe->updateFamilyNameAndStyle();
    fe->setQtDefaultHintStyle(static_cast<QFont::HintingPreference>(fontDef.hintingPreference));

    return fe;
}

QFontEngineFT::QFontEngineFT(const QFontDef &fd)
    : QFontEngine(Freetype)
{
    fontDef = fd;
    matrix.xx = 0x10000;
    matrix.yy = 0x10000;
    matrix.xy = 0;
    matrix.yx = 0;
    cache_cost = 100 * 1024;
    kerning_pairs_loaded = false;
    transform = false;
    embolden = false;
    obliquen = false;
    antialias = true;
    freetype = nullptr;
    default_load_flags = FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;
    default_hint_style = ftInitialDefaultHintStyle;
    subpixelType = Subpixel_None;
    lcdFilterType = (int)((quintptr) FT_LCD_FILTER_DEFAULT);
    defaultFormat = Format_None;
    embeddedbitmap = false;
    const QByteArray env = qgetenv("QT_NO_FT_CACHE");
    cacheEnabled = env.isEmpty() || env.toInt() == 0;
    m_subPixelPositionCount = 4;
    forceAutoHint = false;
    stemDarkeningDriver = false;
}

QFontEngineFT::~QFontEngineFT()
{
    if (freetype)
        freetype->release(face_id);
}

bool QFontEngineFT::init(FaceId faceId, bool antialias, GlyphFormat format,
                         const QByteArray &fontData)
{
    return init(faceId, antialias, format, QFreetypeFace::getFace(faceId, fontData));
}

static void dont_delete(void*) {}

bool QFontEngineFT::init(FaceId faceId, bool antialias, GlyphFormat format,
                         QFreetypeFace *freetypeFace)
{
    freetype = freetypeFace;
    if (!freetype) {
        xsize = 0;
        ysize = 0;
        return false;
    }
    defaultFormat = format;
    this->antialias = antialias;

    if (!antialias)
        glyphFormat = QFontEngine::Format_Mono;
    else
        glyphFormat = defaultFormat;

    face_id = faceId;

    symbol = freetype->symbol_map != nullptr;
    PS_FontInfoRec psrec;
    // don't assume that type1 fonts are symbol fonts by default
    if (FT_Get_PS_Font_Info(freetype->face, &psrec) == FT_Err_Ok) {
        symbol = bool(fontDef.family.contains(QLatin1String("symbol"), Qt::CaseInsensitive));
    }

    freetype->computeSize(fontDef, &xsize, &ysize, &defaultGlyphSet.outline_drawing, &scalableBitmapScaleFactor);

    FT_Face face = lockFace();

    if (FT_IS_SCALABLE(face)) {
        bool fake_oblique = (fontDef.style != QFont::StyleNormal) && !(face->style_flags & FT_STYLE_FLAG_ITALIC) && !qEnvironmentVariableIsSet("QT_NO_SYNTHESIZED_ITALIC");
        if (fake_oblique)
            obliquen = true;
        FT_Set_Transform(face, &matrix, nullptr);
        freetype->matrix = matrix;
        // fake bold
        if ((fontDef.weight >= QFont::Bold) && !(face->style_flags & FT_STYLE_FLAG_BOLD) && !FT_IS_FIXED_WIDTH(face) && !qEnvironmentVariableIsSet("QT_NO_SYNTHESIZED_BOLD")) {
            if (const TT_OS2 *os2 = reinterpret_cast<const TT_OS2 *>(FT_Get_Sfnt_Table(face, ft_sfnt_os2))) {
                if (os2->usWeightClass < 700 && fontDef.pixelSize < 64)
                    embolden = true;
            }
        }
        // underline metrics
        line_thickness =  QFixed::fromFixed(FT_MulFix(face->underline_thickness, face->size->metrics.y_scale));
        underline_position = QFixed::fromFixed(-FT_MulFix(face->underline_position, face->size->metrics.y_scale));
    } else {
        // ad hoc algorithm
        int score = fontDef.weight * fontDef.pixelSize;
        line_thickness = score / 700;
        // looks better with thicker line for small pointsizes
        if (line_thickness < 2 && score >= 1050)
            line_thickness = 2;
        underline_position =  ((line_thickness * 2) + 3) / 6;

        if (isScalableBitmap()) {
            glyphFormat = defaultFormat = GlyphFormat::Format_ARGB;
            cacheEnabled = false;
        }
    }
    if (line_thickness < 1)
        line_thickness = 1;

    metrics = face->size->metrics;

    /*
       TrueType fonts with embedded bitmaps may have a bitmap font specific
       ascent/descent in the EBLC table. There is no direct public API
       to extract those values. The only way we've found is to trick freetype
       into thinking that it's not a scalable font in FT_SelectSize so that
       the metrics are retrieved from the bitmap strikes.
    */
    if (FT_IS_SCALABLE(face)) {
        for (int i = 0; i < face->num_fixed_sizes; ++i) {
            if (xsize == face->available_sizes[i].x_ppem && ysize == face->available_sizes[i].y_ppem) {
                face->face_flags &= ~FT_FACE_FLAG_SCALABLE;

                FT_Select_Size(face, i);
                if (face->size->metrics.ascender + face->size->metrics.descender > 0) {
                    FT_Pos leading = metrics.height - metrics.ascender + metrics.descender;
                    metrics.ascender = face->size->metrics.ascender;
                    metrics.descender = face->size->metrics.descender;
                    if (metrics.descender > 0
                            && QString::fromUtf8(face->family_name) == QLatin1String("Courier New")) {
                        metrics.descender *= -1;
                    }
                    metrics.height = metrics.ascender - metrics.descender + leading;
                }
                FT_Set_Char_Size(face, xsize, ysize, 0, 0);

                face->face_flags |= FT_FACE_FLAG_SCALABLE;
                break;
            }
        }
    }
#if defined(FT_FONT_FORMATS_H)
    const char *fmt = FT_Get_Font_Format(face);
    if (fmt && qstrncmp(fmt, "CFF", 4) == 0) {
        FT_Bool no_stem_darkening = true;
        FT_Error err = FT_Property_Get(qt_getFreetype(), "cff", "no-stem-darkening", &no_stem_darkening);
        if (err == FT_Err_Ok)
            stemDarkeningDriver = !no_stem_darkening;
        else
            stemDarkeningDriver = false;
    }
#endif

    fontDef.styleName = QString::fromUtf8(face->style_name);

    if (!freetype->hbFace) {
        faceData.user_data = face;
        faceData.get_font_table = ft_getSfntTable;
        (void)harfbuzzFace(); // populates face_
        freetype->hbFace = std::move(face_);
    } else {
        Q_ASSERT(!face_);
    }
    // we share the HB face in QFreeTypeFace, so do not let ~QFontEngine() destroy it
    face_ = Holder(freetype->hbFace.get(), dont_delete);

    unlockFace();

    fsType = freetype->fsType();
    return true;
}

void QFontEngineFT::setQtDefaultHintStyle(QFont::HintingPreference hintingPreference)
{
    switch (hintingPreference) {
    case QFont::PreferNoHinting:
        setDefaultHintStyle(HintNone);
        break;
    case QFont::PreferFullHinting:
        setDefaultHintStyle(HintFull);
        break;
    case QFont::PreferVerticalHinting:
        setDefaultHintStyle(HintLight);
        break;
    case QFont::PreferDefaultHinting:
        setDefaultHintStyle(ftInitialDefaultHintStyle);
        break;
    }
}

void QFontEngineFT::setDefaultHintStyle(HintStyle style)
{
    default_hint_style = style;
}

bool QFontEngineFT::expectsGammaCorrectedBlending() const
{
    return stemDarkeningDriver;
}

int QFontEngineFT::loadFlags(QGlyphSet *set, GlyphFormat format, int flags,
                             bool &hsubpixel, int &vfactor) const
{
    int load_flags = FT_LOAD_DEFAULT | default_load_flags;
    int load_target = default_hint_style == HintLight
                      ? FT_LOAD_TARGET_LIGHT
                      : FT_LOAD_TARGET_NORMAL;

    if (format == Format_Mono) {
        load_target = FT_LOAD_TARGET_MONO;
    } else if (format == Format_A32) {
        if (subpixelType == Subpixel_RGB || subpixelType == Subpixel_BGR)
            hsubpixel = true;
        else if (subpixelType == Subpixel_VRGB || subpixelType == Subpixel_VBGR)
            vfactor = 3;
    } else if (format == Format_ARGB) {
#ifdef FT_LOAD_COLOR
        load_flags |= FT_LOAD_COLOR;
#endif
    }

    if (set && set->outline_drawing)
        load_flags |= FT_LOAD_NO_BITMAP;

    if (default_hint_style == HintNone || (flags & DesignMetrics) || (set && set->outline_drawing))
        load_flags |= FT_LOAD_NO_HINTING;
    else
        load_flags |= load_target;

    if (forceAutoHint)
        load_flags |= FT_LOAD_FORCE_AUTOHINT;

    return load_flags;
}

static inline bool areMetricsTooLarge(const QFontEngineFT::GlyphInfo &info)
{
    // false if exceeds QFontEngineFT::Glyph metrics
    return info.width > 0xFF || info.height > 0xFF;
}

static inline void transformBoundingBox(int *left, int *top, int *right, int *bottom, FT_Matrix *matrix)
{
    int l, r, t, b;
    FT_Vector vector;
    vector.x = *left;
    vector.y = *top;
    FT_Vector_Transform(&vector, matrix);
    l = r = vector.x;
    t = b = vector.y;
    vector.x = *right;
    vector.y = *top;
    FT_Vector_Transform(&vector, matrix);
    if (l > vector.x) l = vector.x;
    if (r < vector.x) r = vector.x;
    if (t < vector.y) t = vector.y;
    if (b > vector.y) b = vector.y;
    vector.x = *right;
    vector.y = *bottom;
    FT_Vector_Transform(&vector, matrix);
    if (l > vector.x) l = vector.x;
    if (r < vector.x) r = vector.x;
    if (t < vector.y) t = vector.y;
    if (b > vector.y) b = vector.y;
    vector.x = *left;
    vector.y = *bottom;
    FT_Vector_Transform(&vector, matrix);
    if (l > vector.x) l = vector.x;
    if (r < vector.x) r = vector.x;
    if (t < vector.y) t = vector.y;
    if (b > vector.y) b = vector.y;
    *left = l;
    *right = r;
    *top = t;
    *bottom = b;
}

QFontEngineFT::Glyph *QFontEngineFT::loadGlyph(QGlyphSet *set, uint glyph,
                                               QFixed subPixelPosition,
                                               GlyphFormat format,
                                               bool fetchMetricsOnly,
                                               bool disableOutlineDrawing) const
{
//     Q_ASSERT(freetype->lock == 1);

    if (format == Format_None)
        format = defaultFormat != Format_None ? defaultFormat : Format_Mono;
    Q_ASSERT(format != Format_None);

    Glyph *g = set ? set->getGlyph(glyph, subPixelPosition) : nullptr;
    if (g && g->format == format && (fetchMetricsOnly || g->data))
        return g;

    if (!g && set && set->isGlyphMissing(glyph))
        return &emptyGlyph;


    FT_Face face = freetype->face;

    FT_Matrix matrix = freetype->matrix;

    FT_Vector v;
    v.x = format == Format_Mono ? 0 : FT_Pos(subPixelPosition.value());
    v.y = 0;
    FT_Set_Transform(face, &matrix, &v);

    bool hsubpixel = false;
    int vfactor = 1;
    int load_flags = loadFlags(set, format, 0, hsubpixel, vfactor);

    bool transform = matrix.xx != 0x10000
                     || matrix.yy != 0x10000
                     || matrix.xy != 0
                     || matrix.yx != 0;

    if (transform || obliquen || (format != Format_Mono && !isScalableBitmap()))
        load_flags |= FT_LOAD_NO_BITMAP;

    FT_Error err = FT_Load_Glyph(face, glyph, load_flags);
    if (err && (load_flags & FT_LOAD_NO_BITMAP)) {
        load_flags &= ~FT_LOAD_NO_BITMAP;
        err = FT_Load_Glyph(face, glyph, load_flags);
    }
    if (err == FT_Err_Too_Few_Arguments) {
        // this is an error in the bytecode interpreter, just try to run without it
        load_flags |= FT_LOAD_FORCE_AUTOHINT;
        err = FT_Load_Glyph(face, glyph, load_flags);
    } else if (err == FT_Err_Execution_Too_Long) {
        // This is an error in the bytecode, probably a web font made by someone who
        // didn't test bytecode hinting at all so disable for it for all glyphs.
        qWarning("load glyph failed due to broken hinting bytecode in font, switching to auto hinting");
        default_load_flags |= FT_LOAD_FORCE_AUTOHINT;
        load_flags |= FT_LOAD_FORCE_AUTOHINT;
        err = FT_Load_Glyph(face, glyph, load_flags);
    }
    if (err != FT_Err_Ok) {
        qWarning("load glyph failed err=%x face=%p, glyph=%d", err, face, glyph);
        if (set)
            set->setGlyphMissing(glyph);
        return &emptyGlyph;
    }

    FT_GlyphSlot slot = face->glyph;

    if (embolden)
        FT_GlyphSlot_Embolden(slot);
    if (obliquen) {
        FT_GlyphSlot_Oblique(slot);

        // While Embolden alters the metrics of the slot, oblique does not, so we need
        // to fix this ourselves.
        transform = true;
        FT_Matrix m;
        m.xx = 0x10000;
        m.yx = 0x0;
        m.xy = 0x6000;
        m.yy = 0x10000;

        FT_Matrix_Multiply(&m, &matrix);
    }

    GlyphInfo info;
    info.linearAdvance = slot->linearHoriAdvance >> 10;
    info.xOff = TRUNC(ROUND(slot->advance.x));
    info.yOff = 0;

    if ((set && set->outline_drawing && !disableOutlineDrawing) || fetchMetricsOnly) {
        int left  = slot->metrics.horiBearingX;
        int right = slot->metrics.horiBearingX + slot->metrics.width;
        int top    = slot->metrics.horiBearingY;
        int bottom = slot->metrics.horiBearingY - slot->metrics.height;

        if (transform && slot->format != FT_GLYPH_FORMAT_BITMAP)
            transformBoundingBox(&left, &top, &right, &bottom, &matrix);

        left = FLOOR(left);
        right = CEIL(right);
        bottom = FLOOR(bottom);
        top = CEIL(top);

        info.x = TRUNC(left);
        info.y = TRUNC(top);
        info.width = TRUNC(right - left);
        info.height = TRUNC(top - bottom);

        // If any of the metrics are too large to fit, don't cache them
        if (areMetricsTooLarge(info))
            return nullptr;

        g = new Glyph;
        g->data = nullptr;
        g->linearAdvance = info.linearAdvance;
        g->width = info.width;
        g->height = info.height;
        g->x = info.x;
        g->y = info.y;
        g->advance = info.xOff;
        g->format = format;

        if (set)
            set->setGlyph(glyph, subPixelPosition, g);

        return g;
    }

    int glyph_buffer_size = 0;
    QScopedArrayPointer<uchar> glyph_buffer;
    FT_Render_Mode renderMode = (default_hint_style == HintLight) ? FT_RENDER_MODE_LIGHT : FT_RENDER_MODE_NORMAL;
    switch (format) {
    case Format_Mono:
        renderMode = FT_RENDER_MODE_MONO;
        break;
    case Format_A32:
        Q_ASSERT(hsubpixel || vfactor != 1);
        renderMode = hsubpixel ? FT_RENDER_MODE_LCD : FT_RENDER_MODE_LCD_V;
        break;
    case Format_A8:
    case Format_ARGB:
        break;
    default:
        Q_UNREACHABLE();
    }
    FT_Library_SetLcdFilter(slot->library, (FT_LcdFilter)lcdFilterType);

    err = FT_Render_Glyph(slot, renderMode);
    if (err != FT_Err_Ok)
        qWarning("render glyph failed err=%x face=%p, glyph=%d", err, face, glyph);

    FT_Library_SetLcdFilter(slot->library, FT_LCD_FILTER_NONE);

    info.height = slot->bitmap.rows;
    info.width = slot->bitmap.width;
    info.x = slot->bitmap_left;
    info.y = slot->bitmap_top;
    if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD)
        info.width = info.width / 3;
    if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V)
        info.height = info.height / vfactor;

    int pitch = (format == Format_Mono ? ((info.width + 31) & ~31) >> 3 :
                 (format == Format_A8 ? (info.width + 3) & ~3 : info.width * 4));

    glyph_buffer_size = info.height * pitch;
    glyph_buffer.reset(new uchar[glyph_buffer_size]);

    if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
        uchar *src = slot->bitmap.buffer;
        uchar *dst = glyph_buffer.data();
        int h = slot->bitmap.rows;
        // Some fonts return bitmaps even when we requested something else:
        if (format == Format_Mono) {
            int bytes = ((info.width + 7) & ~7) >> 3;
            while (h--) {
                memcpy (dst, src, bytes);
                dst += pitch;
                src += slot->bitmap.pitch;
            }
        } else if (format == Format_A8) {
            while (h--) {
                for (int x = 0; x < int{info.width}; x++)
                    dst[x] = ((src[x >> 3] & (0x80 >> (x & 7))) ? 0xff : 0x00);
                dst += pitch;
                src += slot->bitmap.pitch;
            }
        } else {
            while (h--) {
                uint *dd = reinterpret_cast<uint *>(dst);
                for (int x = 0; x < int{info.width}; x++)
                    dd[x] = ((src[x >> 3] & (0x80 >> (x & 7))) ? 0xffffffff : 0x00000000);
                dst += pitch;
                src += slot->bitmap.pitch;
            }
        }
    } else if (slot->bitmap.pixel_mode == 7 /*FT_PIXEL_MODE_BGRA*/) {
        Q_ASSERT(format == Format_ARGB);
        uchar *src = slot->bitmap.buffer;
        uchar *dst = glyph_buffer.data();
        int h = slot->bitmap.rows;
        while (h--) {
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            const quint32 *srcPixel = (const quint32 *)src;
            quint32 *dstPixel = (quint32 *)dst;
            for (int x = 0; x < static_cast<int>(slot->bitmap.width); x++, srcPixel++, dstPixel++) {
                const quint32 pixel = *srcPixel;
                *dstPixel = qbswap(pixel);
            }
#else
            memcpy(dst, src, slot->bitmap.width * 4);
#endif
            dst += slot->bitmap.pitch;
            src += slot->bitmap.pitch;
        }
        info.linearAdvance = info.xOff = slot->bitmap.width;
    } else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
        Q_ASSERT(format == Format_A8);
        uchar *src = slot->bitmap.buffer;
        uchar *dst = glyph_buffer.data();
        int h = slot->bitmap.rows;
        int bytes = info.width;
        while (h--) {
            memcpy (dst, src, bytes);
            dst += pitch;
            src += slot->bitmap.pitch;
        }
    } else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD) {
        Q_ASSERT(format == Format_A32);
        convertRGBToARGB(slot->bitmap.buffer, (uint *)glyph_buffer.data(), info.width, info.height, slot->bitmap.pitch, subpixelType != Subpixel_RGB);
    } else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V) {
        Q_ASSERT(format == Format_A32);
        convertRGBToARGB_V(slot->bitmap.buffer, (uint *)glyph_buffer.data(), info.width, info.height, slot->bitmap.pitch, subpixelType != Subpixel_VRGB);
    } else {
        qWarning("QFontEngine: Glyph rendered in unknown pixel_mode=%d", slot->bitmap.pixel_mode);
        return nullptr;
    }

    if (!g) {
        g = new Glyph;
        g->data = nullptr;
    }

    g->linearAdvance = info.linearAdvance;
    g->width = info.width;
    g->height = info.height;
    g->x = info.x;
    g->y = info.y;
    g->advance = info.xOff;
    g->format = format;
    delete [] g->data;
    g->data = glyph_buffer.take();

    if (set)
        set->setGlyph(glyph, subPixelPosition, g);

    return g;
}

QFontEngine::FaceId QFontEngineFT::faceId() const
{
    return face_id;
}

QFontEngine::Properties QFontEngineFT::properties() const
{
    Properties p = freetype->properties();
    if (p.postscriptName.isEmpty()) {
        p.postscriptName = QFontEngine::convertToPostscriptFontFamilyName(fontDef.family.toUtf8());
    }

    return freetype->properties();
}

QFixed QFontEngineFT::emSquareSize() const
{
    if (FT_IS_SCALABLE(freetype->face))
        return freetype->face->units_per_EM;
    else
        return freetype->face->size->metrics.y_ppem;
}

bool QFontEngineFT::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    return ft_getSfntTable(freetype->face, tag, buffer, length);
}

int QFontEngineFT::synthesized() const
{
    int s = 0;
    if ((fontDef.style != QFont::StyleNormal) && !(freetype->face->style_flags & FT_STYLE_FLAG_ITALIC))
        s = SynthesizedItalic;
    if ((fontDef.weight >= QFont::Bold) && !(freetype->face->style_flags & FT_STYLE_FLAG_BOLD))
        s |= SynthesizedBold;
    if (fontDef.stretch != 100 && FT_IS_SCALABLE(freetype->face))
        s |= SynthesizedStretch;
    return s;
}

QFixed QFontEngineFT::ascent() const
{
    QFixed v = QFixed::fromFixed(metrics.ascender);
    if (scalableBitmapScaleFactor != 1)
        v *= scalableBitmapScaleFactor;
    return v;
}

QFixed QFontEngineFT::capHeight() const
{
    TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(freetype->face, ft_sfnt_os2);
    if (os2 && os2->version >= 2) {
        lockFace();
        QFixed answer = QFixed::fromFixed(FT_MulFix(os2->sCapHeight, freetype->face->size->metrics.y_scale));
        unlockFace();
        return answer;
    }
    return calculatedCapHeight();
}

QFixed QFontEngineFT::descent() const
{
    QFixed v = QFixed::fromFixed(-metrics.descender);
    if (scalableBitmapScaleFactor != 1)
        v *= scalableBitmapScaleFactor;
    return v;
}

QFixed QFontEngineFT::leading() const
{
    QFixed v = QFixed::fromFixed(metrics.height - metrics.ascender + metrics.descender);
    if (scalableBitmapScaleFactor != 1)
        v *= scalableBitmapScaleFactor;
    return v;
}

QFixed QFontEngineFT::xHeight() const
{
    TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(freetype->face, ft_sfnt_os2);
    if (os2 && os2->sxHeight) {
        lockFace();
        QFixed answer = QFixed(os2->sxHeight * freetype->face->size->metrics.y_ppem) / emSquareSize();
        unlockFace();
        return answer;
    }

    return QFontEngine::xHeight();
}

QFixed QFontEngineFT::averageCharWidth() const
{
     TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(freetype->face, ft_sfnt_os2);
     if (os2 && os2->xAvgCharWidth) {
         lockFace();
         QFixed answer = QFixed(os2->xAvgCharWidth * freetype->face->size->metrics.x_ppem) / emSquareSize();
         unlockFace();
         return answer;
     }

     return QFontEngine::averageCharWidth();
}

qreal QFontEngineFT::maxCharWidth() const
{
    QFixed max_advance = QFixed::fromFixed(metrics.max_advance);
    if (scalableBitmapScaleFactor != 1)
        max_advance *= scalableBitmapScaleFactor;
    return max_advance.toReal();
}

QFixed QFontEngineFT::lineThickness() const
{
    return line_thickness;
}

QFixed QFontEngineFT::underlinePosition() const
{
    return underline_position;
}

void QFontEngineFT::doKerning(QGlyphLayout *g, QFontEngine::ShaperFlags flags) const
{
    if (!kerning_pairs_loaded) {
        kerning_pairs_loaded = true;
        lockFace();
        if (freetype->face->size->metrics.x_ppem != 0) {
            QFixed scalingFactor = emSquareSize() / QFixed(freetype->face->size->metrics.x_ppem);
            unlockFace();
            const_cast<QFontEngineFT *>(this)->loadKerningPairs(scalingFactor);
        } else {
            unlockFace();
        }
    }

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    if (shouldUseDesignMetrics(flags) && !(fontDef.styleStrategy & QFont::ForceIntegerMetrics))
QT_WARNING_POP
        flags |= DesignMetrics;
    else
        flags &= ~DesignMetrics;

    QFontEngine::doKerning(g, flags);
}

static inline FT_Matrix QTransformToFTMatrix(const QTransform &matrix)
{
    FT_Matrix m;

    m.xx = FT_Fixed(matrix.m11() * 65536);
    m.xy = FT_Fixed(-matrix.m21() * 65536);
    m.yx = FT_Fixed(-matrix.m12() * 65536);
    m.yy = FT_Fixed(matrix.m22() * 65536);

    return m;
}

QFontEngineFT::QGlyphSet *QFontEngineFT::loadGlyphSet(const QTransform &matrix)
{
    if (matrix.type() > QTransform::TxShear || !cacheEnabled)
        return nullptr;

    // FT_Set_Transform only supports scalable fonts
    if (!FT_IS_SCALABLE(freetype->face))
        return matrix.type() <= QTransform::TxTranslate ? &defaultGlyphSet : nullptr;

    FT_Matrix m = QTransformToFTMatrix(matrix);

    QGlyphSet *gs = nullptr;

    for (int i = 0; i < transformedGlyphSets.count(); ++i) {
        const QGlyphSet &g = transformedGlyphSets.at(i);
        if (g.transformationMatrix.xx == m.xx
            && g.transformationMatrix.xy == m.xy
            && g.transformationMatrix.yx == m.yx
            && g.transformationMatrix.yy == m.yy) {

            // found a match, move it to the front
            transformedGlyphSets.move(i, 0);
            gs = &transformedGlyphSets[0];
            break;
        }
    }

    if (!gs) {
        // don't cache more than 10 transformations
        if (transformedGlyphSets.count() >= 10) {
            transformedGlyphSets.move(transformedGlyphSets.size() - 1, 0);
        } else {
            transformedGlyphSets.prepend(QGlyphSet());
        }
        gs = &transformedGlyphSets[0];
        gs->clear();
        gs->transformationMatrix = m;
        gs->outline_drawing = fontDef.pixelSize * fontDef.pixelSize * qAbs(matrix.determinant()) > QT_MAX_CACHED_GLYPH_SIZE * QT_MAX_CACHED_GLYPH_SIZE;
    }
    Q_ASSERT(gs != nullptr);

    return gs;
}

void QFontEngineFT::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics)
{
    FT_Face face = lockFace(Unscaled);
    FT_Set_Transform(face, nullptr, nullptr);
    FT_Load_Glyph(face, glyph, FT_LOAD_NO_BITMAP);

    int left  = face->glyph->metrics.horiBearingX;
    int right = face->glyph->metrics.horiBearingX + face->glyph->metrics.width;
    int top    = face->glyph->metrics.horiBearingY;
    int bottom = face->glyph->metrics.horiBearingY - face->glyph->metrics.height;

    QFixedPoint p;
    p.x = 0;
    p.y = 0;

    metrics->width = QFixed::fromFixed(right-left);
    metrics->height = QFixed::fromFixed(top-bottom);
    metrics->x = QFixed::fromFixed(left);
    metrics->y = QFixed::fromFixed(-top);
    metrics->xoff = QFixed::fromFixed(face->glyph->advance.x);

    if (!FT_IS_SCALABLE(freetype->face))
        QFreetypeFace::addBitmapToPath(face->glyph, p, path);
    else
        QFreetypeFace::addGlyphToPath(face, face->glyph, p, path, face->units_per_EM << 6, face->units_per_EM << 6);

    FT_Set_Transform(face, &freetype->matrix, nullptr);
    unlockFace();
}

bool QFontEngineFT::supportsTransformation(const QTransform &transform) const
{
    return transform.type() <= QTransform::TxRotate;
}

void QFontEngineFT::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags)
{
    if (!glyphs.numGlyphs)
        return;

    if (FT_IS_SCALABLE(freetype->face)) {
        QFontEngine::addOutlineToPath(x, y, glyphs, path, flags);
    } else {
        QVarLengthArray<QFixedPoint> positions;
        QVarLengthArray<glyph_t> positioned_glyphs;
        QTransform matrix;
        matrix.translate(x, y);
        getGlyphPositions(glyphs, matrix, flags, positioned_glyphs, positions);

        FT_Face face = lockFace(Unscaled);
        for (int gl = 0; gl < glyphs.numGlyphs; gl++) {
            FT_UInt glyph = positioned_glyphs[gl];
            FT_Load_Glyph(face, glyph, FT_LOAD_TARGET_MONO);
            QFreetypeFace::addBitmapToPath(face->glyph, positions[gl], path);
        }
        unlockFace();
    }
}

void QFontEngineFT::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int numGlyphs,
                                    QPainterPath *path, QTextItem::RenderFlags)
{
    FT_Face face = lockFace(Unscaled);

    for (int gl = 0; gl < numGlyphs; gl++) {
        FT_UInt glyph = glyphs[gl];

        FT_Load_Glyph(face, glyph, FT_LOAD_NO_BITMAP);

        FT_GlyphSlot g = face->glyph;
        if (g->format != FT_GLYPH_FORMAT_OUTLINE)
            continue;
        if (embolden)
            FT_GlyphSlot_Embolden(g);
        if (obliquen)
            FT_GlyphSlot_Oblique(g);
        QFreetypeFace::addGlyphToPath(face, g, positions[gl], path, xsize, ysize);
    }
    unlockFace();
}

glyph_t QFontEngineFT::glyphIndex(uint ucs4) const
{
    glyph_t glyph = ucs4 < QFreetypeFace::cmapCacheSize ? freetype->cmapCache[ucs4] : 0;
    if (glyph == 0) {
        FT_Face face = freetype->face;
        glyph = FT_Get_Char_Index(face, ucs4);
        if (glyph == 0) {
            // Certain fonts don't have no-break space and tab,
            // while we usually want to render them as space
            if (ucs4 == QChar::Nbsp || ucs4 == QChar::Tabulation) {
                glyph = FT_Get_Char_Index(face, QChar::Space);
            } else if (freetype->symbol_map) {
                // Symbol fonts can have more than one CMAPs, FreeType should take the
                // correct one for us by default, so we always try FT_Get_Char_Index
                // first. If it didn't work (returns 0), we will explicitly set the
                // CMAP to symbol font one and try again. symbol_map is not always the
                // correct one because in certain fonts like Wingdings symbol_map only
                // contains PUA codepoints instead of the common ones.
                FT_Set_Charmap(face, freetype->symbol_map);
                glyph = FT_Get_Char_Index(face, ucs4);
                FT_Set_Charmap(face, freetype->unicode_map);
                if (!glyph && symbol && ucs4 < 0x100)
                    glyph = FT_Get_Char_Index(face, ucs4 + 0xf000);
            }
        }
        if (ucs4 < QFreetypeFace::cmapCacheSize)
            freetype->cmapCache[ucs4] = glyph;
    }

    return glyph;
}

bool QFontEngineFT::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs,
                                 QFontEngine::ShaperFlags flags) const
{
    Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    int glyph_pos = 0;
    if (freetype->symbol_map) {
        FT_Face face = freetype->face;
        QStringIterator it(str, str + len);
        while (it.hasNext()) {
            uint uc = it.next();
            glyphs->glyphs[glyph_pos] = uc < QFreetypeFace::cmapCacheSize ? freetype->cmapCache[uc] : 0;
            if ( !glyphs->glyphs[glyph_pos] ) {
                // Symbol fonts can have more than one CMAPs, FreeType should take the
                // correct one for us by default, so we always try FT_Get_Char_Index
                // first. If it didn't work (returns 0), we will explicitly set the
                // CMAP to symbol font one and try again. symbol_map is not always the
                // correct one because in certain fonts like Wingdings symbol_map only
                // contains PUA codepoints instead of the common ones.
                glyph_t glyph = FT_Get_Char_Index(face, uc);
                // Certain symbol fonts don't have no-break space (0xa0) and tab (0x9),
                // while we usually want to render them as space
                if (!glyph && (uc == 0xa0 || uc == 0x9)) {
                    uc = 0x20;
                    glyph = FT_Get_Char_Index(face, uc);
                }
                if (!glyph) {
                    FT_Set_Charmap(face, freetype->symbol_map);
                    glyph = FT_Get_Char_Index(face, uc);
                    FT_Set_Charmap(face, freetype->unicode_map);
                    if (!glyph && symbol && uc < 0x100)
                        glyph = FT_Get_Char_Index(face, uc + 0xf000);
                }
                glyphs->glyphs[glyph_pos] = glyph;
                if (uc < QFreetypeFace::cmapCacheSize)
                    freetype->cmapCache[uc] = glyph;
            }
            ++glyph_pos;
        }
    } else {
        FT_Face face = freetype->face;
        QStringIterator it(str, str + len);
        while (it.hasNext()) {
            uint uc = it.next();
            glyphs->glyphs[glyph_pos] = uc < QFreetypeFace::cmapCacheSize ? freetype->cmapCache[uc] : 0;
            if (!glyphs->glyphs[glyph_pos]) {
                {
                redo:
                    glyph_t glyph = FT_Get_Char_Index(face, uc);
                    if (!glyph && (uc == 0xa0 || uc == 0x9)) {
                        uc = 0x20;
                        goto redo;
                    }
                    glyphs->glyphs[glyph_pos] = glyph;
                    if (uc < QFreetypeFace::cmapCacheSize)
                        freetype->cmapCache[uc] = glyph;
                }
            }
            ++glyph_pos;
        }
    }

    *nglyphs = glyph_pos;
    glyphs->numGlyphs = glyph_pos;

    if (!(flags & GlyphIndicesOnly))
        recalcAdvances(glyphs, flags);

    return true;
}

bool QFontEngineFT::shouldUseDesignMetrics(QFontEngine::ShaperFlags flags) const
{
    if (!FT_IS_SCALABLE(freetype->face))
        return false;

    return default_hint_style == HintNone || default_hint_style == HintLight || (flags & DesignMetrics);
}

QFixed QFontEngineFT::scaledBitmapMetrics(QFixed m) const
{
    return m * scalableBitmapScaleFactor;
}

glyph_metrics_t QFontEngineFT::scaledBitmapMetrics(const glyph_metrics_t &m, const QTransform &t) const
{
    QTransform trans;
    trans.setMatrix(t.m11(), t.m12(), t.m13(),
                    t.m21(), t.m22(), t.m23(),
                    0, 0, t.m33());
    const qreal scaleFactor = scalableBitmapScaleFactor.toReal();
    trans.scale(scaleFactor, scaleFactor);

    QRectF rect(m.x.toReal(), m.y.toReal(), m.width.toReal(), m.height.toReal());
    QPointF offset(m.xoff.toReal(), m.yoff.toReal());

    rect = trans.mapRect(rect);
    offset = trans.map(offset);

    glyph_metrics_t metrics;
    metrics.x = QFixed::fromReal(rect.x());
    metrics.y = QFixed::fromReal(rect.y());
    metrics.width = QFixed::fromReal(rect.width());
    metrics.height = QFixed::fromReal(rect.height());
    metrics.xoff = QFixed::fromReal(offset.x());
    metrics.yoff = QFixed::fromReal(offset.y());
    return metrics;
}

void QFontEngineFT::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    FT_Face face = nullptr;
    bool design = shouldUseDesignMetrics(flags);
    for (int i = 0; i < glyphs->numGlyphs; i++) {
        Glyph *g = cacheEnabled ? defaultGlyphSet.getGlyph(glyphs->glyphs[i]) : nullptr;
        // Since we are passing Format_None to loadGlyph, use same default format logic as loadGlyph
        GlyphFormat acceptableFormat = (defaultFormat != Format_None) ? defaultFormat : Format_Mono;
        if (g && g->format == acceptableFormat) {
            glyphs->advances[i] = design ? QFixed::fromFixed(g->linearAdvance) : QFixed(g->advance);
        } else {
            if (!face)
                face = lockFace();
            g = loadGlyph(cacheEnabled ? &defaultGlyphSet : nullptr, glyphs->glyphs[i], 0, Format_None, true);
            if (g)
                glyphs->advances[i] = design ? QFixed::fromFixed(g->linearAdvance) : QFixed(g->advance);
            else
                glyphs->advances[i] = design ? QFixed::fromFixed(face->glyph->linearHoriAdvance >> 10)
                                             : QFixed::fromFixed(face->glyph->metrics.horiAdvance).round();
            if (!cacheEnabled && g != &emptyGlyph)
                delete g;
        }

        if (scalableBitmapScaleFactor != 1)
            glyphs->advances[i] *= scalableBitmapScaleFactor;
    }
    if (face)
        unlockFace();

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
QT_WARNING_POP
        for (int i = 0; i < glyphs->numGlyphs; ++i)
            glyphs->advances[i] = glyphs->advances[i].round();
    }
}

glyph_metrics_t QFontEngineFT::boundingBox(const QGlyphLayout &glyphs)
{
    FT_Face face = nullptr;

    glyph_metrics_t overall;
    // initialize with line height, we get the same behaviour on all platforms
    if (!isScalableBitmap()) {
        overall.y = -ascent();
        overall.height = ascent() + descent();
    } else {
        overall.y = QFixed::fromFixed(-metrics.ascender);
        overall.height = QFixed::fromFixed(metrics.ascender - metrics.descender);
    }

    QFixed ymax = 0;
    QFixed xmax = 0;
    for (int i = 0; i < glyphs.numGlyphs; i++) {
        // If shaping has found this should be ignored, ignore it.
        if (!glyphs.advances[i] || glyphs.attributes[i].dontPrint)
            continue;
        Glyph *g = cacheEnabled ? defaultGlyphSet.getGlyph(glyphs.glyphs[i]) : nullptr;
        if (!g) {
            if (!face)
                face = lockFace();
            g = loadGlyph(cacheEnabled ? &defaultGlyphSet : nullptr, glyphs.glyphs[i], 0, Format_None, true);
        }
        if (g) {
            QFixed x = overall.xoff + glyphs.offsets[i].x + g->x;
            QFixed y = overall.yoff + glyphs.offsets[i].y - g->y;
            overall.x = qMin(overall.x, x);
            overall.y = qMin(overall.y, y);
            xmax = qMax(xmax, x + g->width);
            ymax = qMax(ymax, y + g->height);
            overall.xoff += g->advance;
            if (!cacheEnabled && g != &emptyGlyph)
                delete g;
        } else {
            int left  = FLOOR(face->glyph->metrics.horiBearingX);
            int right = CEIL(face->glyph->metrics.horiBearingX + face->glyph->metrics.width);
            int top    = CEIL(face->glyph->metrics.horiBearingY);
            int bottom = FLOOR(face->glyph->metrics.horiBearingY - face->glyph->metrics.height);

            QFixed x = overall.xoff + glyphs.offsets[i].x - (-TRUNC(left));
            QFixed y = overall.yoff + glyphs.offsets[i].y - TRUNC(top);
            overall.x = qMin(overall.x, x);
            overall.y = qMin(overall.y, y);
            xmax = qMax(xmax, x + TRUNC(right - left));
            ymax = qMax(ymax, y + TRUNC(top - bottom));
            overall.xoff += int(TRUNC(ROUND(face->glyph->advance.x)));
        }
    }
    overall.height = qMax(overall.height, ymax - overall.y);
    overall.width = xmax - overall.x;

    if (face)
        unlockFace();

    if (isScalableBitmap())
        overall = scaledBitmapMetrics(overall, QTransform());
    return overall;
}

glyph_metrics_t QFontEngineFT::boundingBox(glyph_t glyph)
{
    FT_Face face = nullptr;
    glyph_metrics_t overall;
    Glyph *g = cacheEnabled ? defaultGlyphSet.getGlyph(glyph) : nullptr;
    if (!g) {
        face = lockFace();
        g = loadGlyph(cacheEnabled ? &defaultGlyphSet : nullptr, glyph, 0, Format_None, true);
    }
    if (g) {
        overall.x = g->x;
        overall.y = -g->y;
        overall.width = g->width;
        overall.height = g->height;
        overall.xoff = g->advance;
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        if (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
QT_WARNING_POP
            overall.xoff = overall.xoff.round();
        if (!cacheEnabled && g != &emptyGlyph)
            delete g;
    } else {
        int left  = FLOOR(face->glyph->metrics.horiBearingX);
        int right = CEIL(face->glyph->metrics.horiBearingX + face->glyph->metrics.width);
        int top    = CEIL(face->glyph->metrics.horiBearingY);
        int bottom = FLOOR(face->glyph->metrics.horiBearingY - face->glyph->metrics.height);

        overall.width = TRUNC(right-left);
        overall.height = TRUNC(top-bottom);
        overall.x = TRUNC(left);
        overall.y = -TRUNC(top);
        overall.xoff = TRUNC(ROUND(face->glyph->advance.x));
    }
    if (face)
        unlockFace();

    if (isScalableBitmap())
        overall = scaledBitmapMetrics(overall, QTransform());
    return overall;
}

glyph_metrics_t QFontEngineFT::boundingBox(glyph_t glyph, const QTransform &matrix)
{
    return alphaMapBoundingBox(glyph, 0, matrix, QFontEngine::Format_None);
}

glyph_metrics_t QFontEngineFT::alphaMapBoundingBox(glyph_t glyph, QFixed subPixelPosition, const QTransform &matrix, QFontEngine::GlyphFormat format)
{
    Glyph *g = loadGlyphFor(glyph, subPixelPosition, format, matrix, true);

    glyph_metrics_t overall;
    if (g) {
        overall.x = g->x;
        overall.y = -g->y;
        overall.width = g->width;
        overall.height = g->height;
        overall.xoff = g->advance;
        if (!cacheEnabled && g != &emptyGlyph)
            delete g;
    } else {
        FT_Face face = lockFace();
        int left  = FLOOR(face->glyph->metrics.horiBearingX);
        int right = CEIL(face->glyph->metrics.horiBearingX + face->glyph->metrics.width);
        int top    = CEIL(face->glyph->metrics.horiBearingY);
        int bottom = FLOOR(face->glyph->metrics.horiBearingY - face->glyph->metrics.height);

        overall.width = TRUNC(right-left);
        overall.height = TRUNC(top-bottom);
        overall.x = TRUNC(left);
        overall.y = -TRUNC(top);
        overall.xoff = TRUNC(ROUND(face->glyph->advance.x));
        unlockFace();
    }

    if (isScalableBitmap())
        overall = scaledBitmapMetrics(overall, matrix);
    return overall;
}

static inline QImage alphaMapFromGlyphData(QFontEngineFT::Glyph *glyph, QFontEngine::GlyphFormat glyphFormat)
{
    if (glyph == nullptr || glyph->height == 0 || glyph->width == 0)
        return QImage();

    QImage::Format format = QImage::Format_Invalid;
    int bytesPerLine = -1;
    switch (glyphFormat) {
    case QFontEngine::Format_Mono:
        format = QImage::Format_Mono;
        bytesPerLine = ((glyph->width + 31) & ~31) >> 3;
        break;
    case QFontEngine::Format_A8:
        format = QImage::Format_Alpha8;
        bytesPerLine = (glyph->width + 3) & ~3;
        break;
    case QFontEngine::Format_A32:
        format = QImage::Format_RGB32;
        bytesPerLine = glyph->width * 4;
        break;
    default:
        Q_UNREACHABLE();
    };

    QImage img(static_cast<const uchar *>(glyph->data), glyph->width, glyph->height, bytesPerLine, format);
    if (format == QImage::Format_Mono)
        img.setColor(1, QColor(Qt::white).rgba());  // Expands color table to 2 items; item 0 set to transparent.
    return img;
}

QFontEngine::Glyph *QFontEngineFT::glyphData(glyph_t glyphIndex, QFixed subPixelPosition,
                                             QFontEngine::GlyphFormat neededFormat, const QTransform &t)
{
    Q_ASSERT(cacheEnabled);

    if (isBitmapFont())
        neededFormat = Format_Mono;
    else if (neededFormat == Format_None && defaultFormat != Format_None)
        neededFormat = defaultFormat;
    else if (neededFormat == Format_None)
        neededFormat = Format_A8;

    Glyph *glyph = loadGlyphFor(glyphIndex, subPixelPosition, neededFormat, t);
    if (!glyph || !glyph->width || !glyph->height)
        return nullptr;

    return glyph;
}

static inline bool is2dRotation(const QTransform &t)
{
    return qFuzzyCompare(t.m11(), t.m22()) && qFuzzyCompare(t.m12(), -t.m21())
        && qFuzzyCompare(t.m11()*t.m22() - t.m12()*t.m21(), qreal(1.0));
}

QFontEngineFT::Glyph *QFontEngineFT::loadGlyphFor(glyph_t g,
                                                  QFixed subPixelPosition,
                                                  GlyphFormat format,
                                                  const QTransform &t,
                                                  bool fetchBoundingBox,
                                                  bool disableOutlineDrawing)
{
    QGlyphSet *glyphSet = loadGlyphSet(t);
    if (glyphSet != nullptr && glyphSet->outline_drawing && !disableOutlineDrawing && !fetchBoundingBox)
        return nullptr;

    Glyph *glyph = glyphSet != nullptr ? glyphSet->getGlyph(g, subPixelPosition) : nullptr;
    if (!glyph || glyph->format != format || (!fetchBoundingBox && !glyph->data)) {
        QScopedValueRollback<HintStyle> saved_default_hint_style(default_hint_style);
        if (t.type() >= QTransform::TxScale && !is2dRotation(t))
            default_hint_style = HintNone; // disable hinting if the glyphs are transformed

        lockFace();
        FT_Matrix m = this->matrix;
        FT_Matrix ftMatrix = glyphSet != nullptr ? glyphSet->transformationMatrix : QTransformToFTMatrix(t);
        FT_Matrix_Multiply(&ftMatrix, &m);
        freetype->matrix = m;
        glyph = loadGlyph(glyphSet, g, subPixelPosition, format, false, disableOutlineDrawing);
        unlockFace();
    }

    return glyph;
}

QImage QFontEngineFT::alphaMapForGlyph(glyph_t g, QFixed subPixelPosition)
{
    return alphaMapForGlyph(g, subPixelPosition, QTransform());
}

QImage QFontEngineFT::alphaMapForGlyph(glyph_t g, QFixed subPixelPosition, const QTransform &t)
{
    const GlyphFormat neededFormat = antialias ? Format_A8 : Format_Mono;

    Glyph *glyph = loadGlyphFor(g, subPixelPosition, neededFormat, t, false, true);

    QImage img = alphaMapFromGlyphData(glyph, neededFormat);
    img = img.copy();

    if (!cacheEnabled && glyph != &emptyGlyph)
        delete glyph;

    return img;
}

QImage QFontEngineFT::alphaRGBMapForGlyph(glyph_t g, QFixed subPixelPosition, const QTransform &t)
{
    if (t.type() > QTransform::TxRotate)
        return QFontEngine::alphaRGBMapForGlyph(g, subPixelPosition, t);

    const GlyphFormat neededFormat = Format_A32;

    Glyph *glyph = loadGlyphFor(g, subPixelPosition, neededFormat, t, false, true);

    QImage img = alphaMapFromGlyphData(glyph, neededFormat);
    img = img.copy();

    if (!cacheEnabled && glyph != &emptyGlyph)
        delete glyph;

    if (!img.isNull())
        return img;

    return QFontEngine::alphaRGBMapForGlyph(g, subPixelPosition, t);
}

QImage QFontEngineFT::bitmapForGlyph(glyph_t g, QFixed subPixelPosition, const QTransform &t, const QColor &color)
{
    Q_UNUSED(color);

    Glyph *glyph = loadGlyphFor(g, subPixelPosition, defaultFormat, t);
    if (glyph == nullptr)
        return QImage();

    QImage img;
    if (defaultFormat == GlyphFormat::Format_ARGB)
        img = QImage(glyph->data, glyph->width, glyph->height, QImage::Format_ARGB32_Premultiplied).copy();
    else if (defaultFormat == GlyphFormat::Format_Mono)
        img = QImage(glyph->data, glyph->width, glyph->height, QImage::Format_Mono).copy();

    if (!img.isNull() && (!t.isIdentity() || scalableBitmapScaleFactor != 1)) {
        QTransform trans(t);
        const qreal scaleFactor = scalableBitmapScaleFactor.toReal();
        trans.scale(scaleFactor, scaleFactor);
        img = img.transformed(trans, Qt::SmoothTransformation);
    }

    if (!cacheEnabled && glyph != &emptyGlyph)
        delete glyph;

    return img;
}

void QFontEngineFT::removeGlyphFromCache(glyph_t glyph)
{
    defaultGlyphSet.removeGlyphFromCache(glyph, 0);
}

int QFontEngineFT::glyphCount() const
{
    int count = 0;
    FT_Face face = lockFace();
    if (face) {
        count = face->num_glyphs;
        unlockFace();
    }
    return count;
}

FT_Face QFontEngineFT::lockFace(Scaling scale) const
{
    freetype->lock();
    FT_Face face = freetype->face;
    if (scale == Unscaled) {
        if (FT_Set_Char_Size(face, face->units_per_EM << 6, face->units_per_EM << 6, 0, 0) == 0) {
            freetype->xsize = face->units_per_EM << 6;
            freetype->ysize = face->units_per_EM << 6;
        }
    } else if (freetype->xsize != xsize || freetype->ysize != ysize) {
        FT_Set_Char_Size(face, xsize, ysize, 0, 0);
        freetype->xsize = xsize;
        freetype->ysize = ysize;
    }
    if (freetype->matrix.xx != matrix.xx ||
        freetype->matrix.yy != matrix.yy ||
        freetype->matrix.xy != matrix.xy ||
        freetype->matrix.yx != matrix.yx) {
        freetype->matrix = matrix;
        FT_Set_Transform(face, &freetype->matrix, nullptr);
    }

    return face;
}

void QFontEngineFT::unlockFace() const
{
    freetype->unlock();
}

FT_Face QFontEngineFT::non_locked_face() const
{
    return freetype->face;
}


QFontEngineFT::QGlyphSet::QGlyphSet()
    : outline_drawing(false)
{
    transformationMatrix.xx = 0x10000;
    transformationMatrix.yy = 0x10000;
    transformationMatrix.xy = 0;
    transformationMatrix.yx = 0;
    memset(fast_glyph_data, 0, sizeof(fast_glyph_data));
    fast_glyph_count = 0;
}

QFontEngineFT::QGlyphSet::~QGlyphSet()
{
    clear();
}

void QFontEngineFT::QGlyphSet::clear()
{
    if (fast_glyph_count > 0) {
        for (int i = 0; i < 256; ++i) {
            if (fast_glyph_data[i]) {
                delete fast_glyph_data[i];
                fast_glyph_data[i] = nullptr;
            }
        }
        fast_glyph_count = 0;
    }
    qDeleteAll(glyph_data);
    glyph_data.clear();
}

void QFontEngineFT::QGlyphSet::removeGlyphFromCache(glyph_t index, QFixed subPixelPosition)
{
    if (useFastGlyphData(index, subPixelPosition)) {
        if (fast_glyph_data[index]) {
            delete fast_glyph_data[index];
            fast_glyph_data[index] = nullptr;
            if (fast_glyph_count > 0)
                --fast_glyph_count;
        }
    } else {
        delete glyph_data.take(GlyphAndSubPixelPosition(index, subPixelPosition));
    }
}

void QFontEngineFT::QGlyphSet::setGlyph(glyph_t index, QFixed subPixelPosition, Glyph *glyph)
{
    if (useFastGlyphData(index, subPixelPosition)) {
        if (!fast_glyph_data[index])
            ++fast_glyph_count;
        fast_glyph_data[index] = glyph;
    } else {
        glyph_data.insert(GlyphAndSubPixelPosition(index, subPixelPosition), glyph);
    }
}

int QFontEngineFT::getPointInOutline(glyph_t glyph, int flags, quint32 point, QFixed *xpos, QFixed *ypos, quint32 *nPoints)
{
    lockFace();
    bool hsubpixel = true;
    int vfactor = 1;
    int load_flags = loadFlags(nullptr, Format_A8, flags, hsubpixel, vfactor);
    int result = freetype->getPointInOutline(glyph, load_flags, point, xpos, ypos, nPoints);
    unlockFace();
    return result;
}

bool QFontEngineFT::initFromFontEngine(const QFontEngineFT *fe)
{
    if (!init(fe->faceId(), fe->antialias, fe->defaultFormat, fe->freetype))
        return false;

    // Increase the reference of this QFreetypeFace since one more QFontEngineFT
    // will be using it
    freetype->ref.ref();

    default_load_flags = fe->default_load_flags;
    default_hint_style = fe->default_hint_style;
    antialias = fe->antialias;
    transform = fe->transform;
    embolden = fe->embolden;
    obliquen = fe->obliquen;
    subpixelType = fe->subpixelType;
    lcdFilterType = fe->lcdFilterType;
    embeddedbitmap = fe->embeddedbitmap;

    return true;
}

QFontEngine *QFontEngineFT::cloneWithSize(qreal pixelSize) const
{
    QFontDef fontDef(this->fontDef);
    fontDef.pixelSize = pixelSize;
    QFontEngineFT *fe = new QFontEngineFT(fontDef);
    if (!fe->initFromFontEngine(this)) {
        delete fe;
        return nullptr;
    } else {
        return fe;
    }
}

Qt::HANDLE QFontEngineFT::handle() const
{
    return non_locked_face();
}

QT_END_NAMESPACE

#endif // QT_NO_FREETYPE
