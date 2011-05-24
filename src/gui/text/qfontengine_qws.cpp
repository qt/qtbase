/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfontengine_p.h"
#include <private/qunicodetables_p.h>
#include <qwsdisplay_qws.h>
#include <qvarlengtharray.h>
#include <private/qpainter_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qpdf_p.h>
#include "qtextengine_p.h"
#include "private/qcore_unix_p.h" // overrides QT_OPEN

#include <qdebug.h>


#ifndef QT_NO_QWS_QPF

#include "qfile.h"
#include "qdir.h"

#define QT_USE_MMAP
#include <stdlib.h>

#ifdef QT_USE_MMAP
// for mmap
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#  if defined(QT_LINUXBASE) && !defined(MAP_FILE)
     // LSB 3.2 does not define MAP_FILE
#    define MAP_FILE 0
#  endif

#endif

#endif // QT_NO_QWS_QPF

QT_BEGIN_NAMESPACE

#ifndef QT_NO_QWS_QPF
QT_BEGIN_INCLUDE_NAMESPACE
#include "qplatformdefs.h"
QT_END_INCLUDE_NAMESPACE

static inline unsigned int getChar(const QChar *str, int &i, const int len)
{
    unsigned int uc = str[i].unicode();
    if (uc >= 0xd800 && uc < 0xdc00 && i < len-1) {
        uint low = str[i+1].unicode();
       if (low >= 0xdc00 && low < 0xe000) {
            uc = (uc - 0xd800)*0x400 + (low - 0xdc00) + 0x10000;
            ++i;
        }
    }
    return uc;
}

#define FM_SMOOTH 1


class Q_PACKED QPFGlyphMetrics {

public:
    quint8 linestep;
    quint8 width;
    quint8 height;
    quint8 flags;

    qint8 bearingx;      // Difference from pen position to glyph's left bbox
    quint8 advance;       // Difference between pen positions
    qint8 bearingy;      // Used for putting characters on baseline

    qint8 reserved;      // Do not use

    // Flags:
    // RendererOwnsData - the renderer is responsible for glyph data
    //                    memory deletion otherwise QPFGlyphTree must
    //                    delete [] the data when the glyph is deleted.
    enum Flags { RendererOwnsData=0x01 };
};

class QPFGlyph {
public:
    QPFGlyph() { metrics=0; data=0; }
    QPFGlyph(QPFGlyphMetrics* m, uchar* d) :
	metrics(m), data(d) { }
    ~QPFGlyph() {}

    QPFGlyphMetrics* metrics;
    uchar* data;
};

struct Q_PACKED QPFFontMetrics{
    qint8 ascent,descent;
    qint8 leftbearing,rightbearing;
    quint8 maxwidth;
    qint8 leading;
    quint8 flags;
    quint8 underlinepos;
    quint8 underlinewidth;
    quint8 reserved3;
};


class QPFGlyphTree {
public:
    /* reads in a tree like this:

       A-Z
       /   \
       0-9   a-z

       etc.

    */
    glyph_t min,max;
    QPFGlyphTree* less;
    QPFGlyphTree* more;
    QPFGlyph* glyph;
public:
#ifdef QT_USE_MMAP
    QPFGlyphTree(uchar*& data)
    {
        read(data);
    }
#else
    QPFGlyphTree(QIODevice& f)
    {
        read(f);
    }
#endif

    ~QPFGlyphTree()
    {
        // NOTE: does not delete glyph[*].metrics or .data.
        //       the caller does this (only they know who owns
        //       the data).  See clear().
        delete less;
        delete more;
        delete [] glyph;
    }

    bool inFont(glyph_t g) const
    {
        if ( g < min ) {
            if ( !less )
                return false;
            return less->inFont(g);
        } else if ( g > max ) {
            if ( !more )
                return false;
            return more->inFont(g);
        }
        return true;
    }

    QPFGlyph* get(glyph_t g)
    {
        if ( g < min ) {
            if ( !less )
                return 0;
            return less->get(g);
        } else if ( g > max ) {
            if ( !more )
                return 0;
            return more->get(g);
        }
        return &glyph[g - min];
    }
    int totalChars() const
    {
        if ( !this ) return 0;
        return max-min+1 + less->totalChars() + more->totalChars();
    }
    int weight() const
    {
        if ( !this ) return 0;
        return 1 + less->weight() + more->weight();
    }

    void dump(int indent=0)
    {
        for (int i=0; i<indent; i++) printf(" ");
        printf("%d..%d",min,max);
        //if ( indent == 0 )
        printf(" (total %d)",totalChars());
        printf("\n");
        if ( less ) less->dump(indent+1);
        if ( more ) more->dump(indent+1);
    }

private:
    QPFGlyphTree()
    {
    }

#ifdef QT_USE_MMAP
    void read(uchar*& data)
    {
        // All node data first
        readNode(data);
        // Then all non-video data
        readMetrics(data);
        // Then all video data
        readData(data);
    }
#else
    void read(QIODevice& f)
    {
        // All node data first
        readNode(f);
        // Then all non-video data
        readMetrics(f);
        // Then all video data
        readData(f);
    }
#endif

#ifdef QT_USE_MMAP
    void readNode(uchar*& data)
    {
        uchar rw = *data++;
        uchar cl = *data++;
        min = (rw << 8) | cl;
        rw = *data++;
        cl = *data++;
        max = (rw << 8) | cl;
        int flags = *data++;
        if ( flags & 1 )
            less = new QPFGlyphTree;
        else
            less = 0;
        if ( flags & 2 )
            more = new QPFGlyphTree;
        else
            more = 0;
        int n = max-min+1;
        glyph = new QPFGlyph[n];

        if ( less )
            less->readNode(data);
        if ( more )
            more->readNode(data);
    }
#else
    void readNode(QIODevice& f)
    {
        char rw;
        char cl;
        f.getChar(&rw);
        f.getChar(&cl);
        min = (rw << 8) | cl;
        f.getChar(&rw);
        f.getChar(&cl);
        max = (rw << 8) | cl;
        char flags;
        f.getChar(&flags);
        if ( flags & 1 )
            less = new QPFGlyphTree;
        else
            less = 0;
        if ( flags & 2 )
            more = new QPFGlyphTree;
        else
            more = 0;
        int n = max-min+1;
        glyph = new QPFGlyph[n];

        if ( less )
            less->readNode(f);
        if ( more )
            more->readNode(f);
    }
#endif

#ifdef QT_USE_MMAP
    void readMetrics(uchar*& data)
    {
        int n = max-min+1;
        for (int i=0; i<n; i++) {
            glyph[i].metrics = (QPFGlyphMetrics*)data;
            data += sizeof(QPFGlyphMetrics);
        }
        if ( less )
            less->readMetrics(data);
        if ( more )
            more->readMetrics(data);
    }
#else
    void readMetrics(QIODevice& f)
    {
        int n = max-min+1;
        for (int i=0; i<n; i++) {
            glyph[i].metrics = new QPFGlyphMetrics;
            f.read((char*)glyph[i].metrics, sizeof(QPFGlyphMetrics));
        }
        if ( less )
            less->readMetrics(f);
        if ( more )
            more->readMetrics(f);
    }
#endif

#ifdef QT_USE_MMAP
    void readData(uchar*& data)
    {
        int n = max-min+1;
        for (int i=0; i<n; i++) {
            QSize s( glyph[i].metrics->width, glyph[i].metrics->height );
            //######### s = qt_screen->mapToDevice( s );
            uint datasize = glyph[i].metrics->linestep * s.height();
            glyph[i].data = data; data += datasize;
        }
        if ( less )
            less->readData(data);
        if ( more )
            more->readData(data);
    }
#else
    void readData(QIODevice& f)
    {
        int n = max-min+1;
        for (int i=0; i<n; i++) {
            QSize s( glyph[i].metrics->width, glyph[i].metrics->height );
            //############### s = qt_screen->mapToDevice( s );
            uint datasize = glyph[i].metrics->linestep * s.height();
            glyph[i].data = new uchar[datasize]; // ### deleted?
            f.read((char*)glyph[i].data, datasize);
        }
        if ( less )
            less->readData(f);
        if ( more )
            more->readData(f);
    }
#endif

};

class QFontEngineQPF1Data
{
public:
    QPFFontMetrics fm;
    QPFGlyphTree *tree;
    void *mmapStart;
    size_t mmapLength;
};

#if defined(Q_OS_INTEGRITY)
static void *qt_mmap(void *start, size_t length, int /*prot*/, int /*flags*/, int fd, off_t offset)
{
    // INTEGRITY cannot mmap local files - load it into a local buffer
    if (::lseek(fd, offset, SEEK_SET) == -1) {
#  if defined(DEBUG_FONTENGINE)
        perror("lseek failed");
#  endif
    }
    void *buf = malloc(length);
    if (::read(fd, buf, length) != (ssize_t)length) {
#  if defined(DEBUG_FONTENGINE)
        perror("read failed");
#  endif
    }

    return buf;
}
#else
static inline void *qt_mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
    return mmap(start, length, prot, flags, fd, offset);
}
#endif



QFontEngineQPF1::QFontEngineQPF1(const QFontDef&, const QString &fn)
{
    cache_cost = 1;

    int f = QT_OPEN( QFile::encodeName(fn), O_RDONLY, 0);
    Q_ASSERT(f>=0);
    QT_STATBUF st;
    if ( QT_FSTAT( f, &st ) )
        qFatal("Failed to stat %s",QFile::encodeName(fn).data());
    uchar* data = (uchar*)qt_mmap( 0, // any address
                                st.st_size, // whole file
                                PROT_READ, // read-only memory
#if defined(Q_OS_INTEGRITY)
	                        0,
#elif !defined(Q_OS_SOLARIS) && !defined(Q_OS_QNX4) && !defined(Q_OS_VXWORKS)
                                MAP_FILE | MAP_PRIVATE, // swap-backed map from file
#else
                                MAP_PRIVATE,
#endif
                                f, 0 ); // from offset 0 of f
#if !defined(MAP_FAILED) && (defined(Q_OS_QNX4) || defined(Q_OS_INTEGRITY))
#define MAP_FAILED ((void *)-1)
#endif
    if ( !data || data == (uchar*)MAP_FAILED )
        qFatal("Failed to mmap %s",QFile::encodeName(fn).data());
    QT_CLOSE(f);

    d = new QFontEngineQPF1Data;
    d->mmapStart = data;
    d->mmapLength = st.st_size;
    memcpy(reinterpret_cast<char*>(&d->fm),data,sizeof(d->fm));

    data += sizeof(d->fm);
    d->tree = new QPFGlyphTree(data);
    glyphFormat = (d->fm.flags & FM_SMOOTH) ? QFontEngineGlyphCache::Raster_A8
                  : QFontEngineGlyphCache::Raster_Mono;
#if 0
    qDebug() << "font file" << fn
             << "ascent" << d->fm.ascent << "descent" << d->fm.descent
             << "leftbearing" << d->fm.leftbearing
             << "rightbearing" << d->fm.rightbearing
             << "maxwidth" << d->fm.maxwidth
             << "leading" << d->fm.leading
             << "flags" << d->fm.flags
             << "underlinepos" << d->fm.underlinepos
             << "underlinewidth" << d->fm.underlinewidth;
#endif
}

QFontEngineQPF1::~QFontEngineQPF1()
{
    if (d->mmapStart)
        munmap(d->mmapStart, d->mmapLength);
    delete d->tree;
    delete d;
}


bool QFontEngineQPF1::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    if(*nglyphs < len) {
        *nglyphs = len;
        return false;
    }
    *nglyphs = 0;

    bool mirrored = flags & QTextEngine::RightToLeft;
    for(int i = 0; i < len; i++) {
        unsigned int uc = getChar(str, i, len);
        if (mirrored)
            uc = QChar::mirroredChar(uc);
        glyphs->glyphs[*nglyphs] = uc < 0x10000 ? uc : 0;
        ++*nglyphs;
    }

    glyphs->numGlyphs = *nglyphs;

    if (flags & QTextEngine::GlyphIndicesOnly)
        return true;

    recalcAdvances(glyphs, flags);

    return true;
}

void QFontEngineQPF1::recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags) const
{
    for(int i = 0; i < glyphs->numGlyphs; i++) {
        QPFGlyph *glyph = d->tree->get(glyphs->glyphs[i]);

        glyphs->advances_x[i] = glyph ? glyph->metrics->advance : 0;
        glyphs->advances_y[i] = 0;

        if (!glyph)
            glyphs->glyphs[i] = 0;
    }
}

void QFontEngineQPF1::draw(QPaintEngine *p, qreal _x, qreal _y, const QTextItemInt &si)
{
    QPaintEngineState *pState = p->state;
    QRasterPaintEngine *paintEngine = static_cast<QRasterPaintEngine*>(p);

    QTransform matrix = pState->transform();
    matrix.translate(_x, _y);
    QFixed x = QFixed::fromReal(matrix.dx());
    QFixed y = QFixed::fromReal(matrix.dy());

    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    getGlyphPositions(si.glyphs, matrix, si.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;

    int depth = (d->fm.flags & FM_SMOOTH) ? 8 : 1;
    for(int i = 0; i < glyphs.size(); i++) {
        const QPFGlyph *glyph = d->tree->get(glyphs[i]);
        if (!glyph)
            continue;

        int bpl = glyph->metrics->linestep;

        if(glyph->data)
            paintEngine->alphaPenBlt(glyph->data, bpl, depth,
                                     qRound(positions[i].x) + glyph->metrics->bearingx,
                                     qRound(positions[i].y) - glyph->metrics->bearingy,
                                     glyph->metrics->width,glyph->metrics->height);
    }
}


QImage QFontEngineQPF1::alphaMapForGlyph(glyph_t g)
{
    const QPFGlyph *glyph = d->tree->get(g);
    if (!glyph)
	return QImage();

    int mono = !(d->fm.flags & FM_SMOOTH);

    const uchar *bits = glyph->data;//((const uchar *) glyph);

    QImage image;
    if (mono) {
        image = QImage((glyph->metrics->width+7)&~7, glyph->metrics->height, QImage::Format_Mono);
        image.setColor(0, qRgba(0, 0, 0, 0));
        image.setColor(1, qRgba(0, 0, 0, 255));
    } else {
        image = QImage(glyph->metrics->width, glyph->metrics->height, QImage::Format_Indexed8);
        for (int j=0; j<256; ++j)
            image.setColor(j, qRgba(0, 0, 0, j));
    }
    for (int i=0; i<glyph->metrics->height; ++i) {
        memcpy(image.scanLine(i), bits, glyph->metrics->linestep);
        bits += glyph->metrics->linestep;
    }
    return image;
}



void QFontEngineQPF1::addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags)
{
    addBitmapFontToPath(x, y, glyphs, path, flags);
}

glyph_metrics_t QFontEngineQPF1::boundingBox(const QGlyphLayout &glyphs)
{
   if (glyphs.numGlyphs == 0)
        return glyph_metrics_t();

    QFixed w = 0;
    for (int i = 0; i < glyphs.numGlyphs; ++i)
        w += glyphs.effectiveAdvance(i);
    return glyph_metrics_t(0, -ascent(), w - lastRightBearing(glyphs), ascent()+descent()+1, w, 0);
}

glyph_metrics_t QFontEngineQPF1::boundingBox(glyph_t glyph)
{
    const QPFGlyph *g = d->tree->get(glyph);
    if (!g)
        return glyph_metrics_t();
    Q_ASSERT(g);
    return glyph_metrics_t(g->metrics->bearingx, -g->metrics->bearingy,
                            g->metrics->width, g->metrics->height,
                            g->metrics->advance, 0);
}

QFixed QFontEngineQPF1::ascent() const
{
    return d->fm.ascent;
}

QFixed QFontEngineQPF1::descent() const
{
    return d->fm.descent;
}

QFixed QFontEngineQPF1::leading() const
{
    return d->fm.leading;
}

qreal QFontEngineQPF1::maxCharWidth() const
{
    return d->fm.maxwidth;
}
/*
const char *QFontEngineQPF1::name() const
{
    return "qt";
}
*/
bool QFontEngineQPF1::canRender(const QChar *str, int len)
{
    for(int i = 0; i < len; i++)
        if (!d->tree->inFont(str[i].unicode()))
            return false;
    return true;
}

QFontEngine::Type QFontEngineQPF1::type() const
{
    return QPF1;
}

qreal QFontEngineQPF1::minLeftBearing() const
{
    return d->fm.leftbearing;
}

qreal QFontEngineQPF1::minRightBearing() const
{
    return d->fm.rightbearing;
}

QFixed QFontEngineQPF1::underlinePosition() const
{
    return d->fm.underlinepos;
}

QFixed QFontEngineQPF1::lineThickness() const
{
    return d->fm.underlinewidth;
}

#endif //QT_NO_QWS_QPF

QT_END_NAMESPACE
