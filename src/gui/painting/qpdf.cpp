// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpdf_p.h"

#ifndef QT_NO_PDF

#include "qplatformdefs.h"

#include <private/qcmyk_p.h>
#include <private/qfont_p.h>
#include <private/qmath_p.h>
#include <private/qpainter_p.h>

#include <qbuffer.h>
#include <qcryptographichash.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qfile.h>
#include <qimagewriter.h>
#include <qnumeric.h>
#include <qtemporaryfile.h>
#include <qtimezone.h>
#include <quuid.h>
#include <qxmlstream.h>

#include <cstdio>
#include <map>

#ifndef QT_NO_COMPRESS
#include <zlib.h>
#endif

#ifdef QT_NO_COMPRESS
static const bool do_compress = false;
#else
static const bool do_compress = true;
#endif

// might be helpful for smooth transforms of images
// Can't use it though, as gs generates completely wrong images if this is true.
static const bool interpolateImages = false;

static void initResources()
{
    Q_INIT_RESOURCE(qpdf);
}

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

constexpr QPaintEngine::PaintEngineFeatures qt_pdf_decide_features()
{
    QPaintEngine::PaintEngineFeatures f = QPaintEngine::AllFeatures;
    f &= ~(QPaintEngine::PorterDuff
           | QPaintEngine::PerspectiveTransform
           | QPaintEngine::ObjectBoundingModeGradients
           | QPaintEngine::ConicalGradientFill);
    return f;
}

extern bool qt_isExtendedRadialGradient(const QBrush &brush);

// helper function to remove transparency from brush in PDF/A-1b mode
static void removeTransparencyFromBrush(QBrush &brush)
{
    if (brush.style() == Qt::SolidPattern) {
        QColor color = brush.color();
        if (color.alpha() != 255) {
            color.setAlpha(255);
            brush.setColor(color);
        }

        return;
    }

    if (qt_isExtendedRadialGradient(brush)) {
        brush = QBrush(Qt::black); // the safest we can do so far...
        return;
    }

    if (brush.style() == Qt::LinearGradientPattern
        || brush.style() == Qt::RadialGradientPattern
        || brush.style() == Qt::ConicalGradientPattern) {

        QGradientStops stops = brush.gradient()->stops();
        for (int i = 0; i < stops.size(); ++i) {
            if (stops[i].second.alpha() != 255)
                stops[i].second.setAlpha(255);
        }

        const_cast<QGradient*>(brush.gradient())->setStops(stops);
        return;
    }

    if (brush.style() == Qt::TexturePattern) {
        // handled inside QPdfEnginePrivate::addImage() already
        return;
    }
}


/* also adds a space at the end of the number */
const char *qt_real_to_string(qreal val, char *buf) {
    const char *ret = buf;

    if (!qIsFinite(val) || std::abs(val) > std::numeric_limits<quint32>::max()) {
        *(buf++) = '0';
        *(buf++) = ' ';
        *buf = 0;
        return ret;
    }

    if (val < 0) {
        *(buf++) = '-';
        val = -val;
    }
    qreal frac = std::modf(val, &val);
    quint32 ival(val);

    int ifrac = (int)(frac * 1000000000);
    if (ifrac == 1000000000) {
        ++ival;
        ifrac = 0;
    }
    char output[256];
    int i = 0;
    while (ival) {
        output[i] = '0' + (ival % 10);
        ++i;
        ival /= 10;
    }
    int fact = 100000000;
    if (i == 0) {
        *(buf++) = '0';
    } else {
        while (i) {
            *(buf++) = output[--i];
            fact /= 10;
            ifrac /= 10;
        }
    }

    if (ifrac) {
        *(buf++) =  '.';
        while (fact) {
            *(buf++) = '0' + ((ifrac/fact) % 10);
            fact /= 10;
        }
    }
    *(buf++) = ' ';
    *buf = 0;
    return ret;
}

const char *qt_int_to_string(int val, char *buf) {
    const char *ret = buf;
    if (val < 0) {
        *(buf++) = '-';
        val = -val;
    }
    char output[256];
    int i = 0;
    while (val) {
        output[i] = '0' + (val % 10);
        ++i;
        val /= 10;
    }
    if (i == 0) {
        *(buf++) = '0';
    } else {
        while (i)
            *(buf++) = output[--i];
    }
    *(buf++) = ' ';
    *buf = 0;
    return ret;
}


namespace QPdf {
    ByteStream::ByteStream(QByteArray *byteArray, bool fileBacking)
            : dev(new QBuffer(byteArray)),
            fileBackingEnabled(fileBacking),
            fileBackingActive(false),
            handleDirty(false)
    {
        dev->open(QIODevice::ReadWrite | QIODevice::Append);
    }

    ByteStream::ByteStream(bool fileBacking)
            : dev(new QBuffer(&ba)),
            fileBackingEnabled(fileBacking),
            fileBackingActive(false),
            handleDirty(false)
    {
        dev->open(QIODevice::ReadWrite);
    }

    ByteStream::~ByteStream()
    {
        delete dev;
    }

    ByteStream &ByteStream::operator <<(char chr)
    {
        if (handleDirty) prepareBuffer();
        dev->write(&chr, 1);
        return *this;
    }

    ByteStream &ByteStream::operator <<(const char *str)
    {
        if (handleDirty) prepareBuffer();
        dev->write(str, strlen(str));
        return *this;
    }

    ByteStream &ByteStream::operator <<(const QByteArray &str)
    {
        if (handleDirty) prepareBuffer();
        dev->write(str);
        return *this;
    }

    ByteStream &ByteStream::operator <<(const ByteStream &src)
    {
        Q_ASSERT(!src.dev->isSequential());
        if (handleDirty) prepareBuffer();
        // We do play nice here, even though it looks ugly.
        // We save the position and restore it afterwards.
        ByteStream &s = const_cast<ByteStream&>(src);
        qint64 pos = s.dev->pos();
        s.dev->reset();
        while (!s.dev->atEnd()) {
            QByteArray buf = s.dev->read(chunkSize());
            dev->write(buf);
        }
        s.dev->seek(pos);
        return *this;
    }

    ByteStream &ByteStream::operator <<(qreal val) {
        char buf[256];
        qt_real_to_string(val, buf);
        *this << buf;
        return *this;
    }

    ByteStream &ByteStream::operator <<(int val) {
        char buf[256];
        qt_int_to_string(val, buf);
        *this << buf;
        return *this;
    }

    ByteStream &ByteStream::operator <<(const QPointF &p) {
        char buf[256];
        qt_real_to_string(p.x(), buf);
        *this << buf;
        qt_real_to_string(p.y(), buf);
        *this << buf;
        return *this;
    }

    QIODevice *ByteStream::stream()
    {
        dev->reset();
        handleDirty = true;
        return dev;
    }

    void ByteStream::clear()
    {
        dev->open(QIODevice::ReadWrite | QIODevice::Truncate);
    }

    void ByteStream::prepareBuffer()
    {
        Q_ASSERT(!dev->isSequential());
        qint64 size = dev->size();
        if (fileBackingEnabled && !fileBackingActive
                && size > maxMemorySize()) {
            // Switch to file backing.
            QTemporaryFile *newFile = new QTemporaryFile;
            if (newFile->open()) {
                dev->reset();
                while (!dev->atEnd()) {
                    QByteArray buf = dev->read(chunkSize());
                    newFile->write(buf);
                }
                delete dev;
                dev = newFile;
                ba.clear();
                fileBackingActive = true;
            }
        }
        if (dev->pos() != size) {
            dev->seek(size);
            handleDirty = false;
        }
    }
}

#define QT_PATH_ELEMENT(elm)

QByteArray QPdf::generatePath(const QPainterPath &path, const QTransform &matrix, PathFlags flags)
{
    QByteArray result;
    if (!path.elementCount())
        return result;

    ByteStream s(&result);

    int start = -1;
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &elm = path.elementAt(i);
        switch (elm.type) {
        case QPainterPath::MoveToElement:
            if (start >= 0
                && path.elementAt(start).x == path.elementAt(i-1).x
                && path.elementAt(start).y == path.elementAt(i-1).y)
                s << "h\n";
            s << matrix.map(QPointF(elm.x, elm.y)) << "m\n";
            start = i;
                break;
        case QPainterPath::LineToElement:
            s << matrix.map(QPointF(elm.x, elm.y)) << "l\n";
            break;
        case QPainterPath::CurveToElement:
            Q_ASSERT(path.elementAt(i+1).type == QPainterPath::CurveToDataElement);
            Q_ASSERT(path.elementAt(i+2).type == QPainterPath::CurveToDataElement);
            s << matrix.map(QPointF(elm.x, elm.y))
              << matrix.map(QPointF(path.elementAt(i+1).x, path.elementAt(i+1).y))
              << matrix.map(QPointF(path.elementAt(i+2).x, path.elementAt(i+2).y))
              << "c\n";
            i += 2;
            break;
        default:
            qFatal("QPdf::generatePath(), unhandled type: %d", elm.type);
        }
    }
    if (start >= 0
        && path.elementAt(start).x == path.elementAt(path.elementCount()-1).x
        && path.elementAt(start).y == path.elementAt(path.elementCount()-1).y)
        s << "h\n";

    Qt::FillRule fillRule = path.fillRule();

    const char *op = "";
    switch (flags) {
    case ClipPath:
        op = (fillRule == Qt::WindingFill) ? "W n\n" : "W* n\n";
        break;
    case FillPath:
        op = (fillRule == Qt::WindingFill) ? "f\n" : "f*\n";
        break;
    case StrokePath:
        op = "S\n";
        break;
    case FillAndStrokePath:
        op = (fillRule == Qt::WindingFill) ? "B\n" : "B*\n";
        break;
    }
    s << op;
    return result;
}

QByteArray QPdf::generateMatrix(const QTransform &matrix)
{
    QByteArray result;
    ByteStream s(&result);
    s << matrix.m11()
      << matrix.m12()
      << matrix.m21()
      << matrix.m22()
      << matrix.dx()
      << matrix.dy()
      << "cm\n";
    return result;
}

QByteArray QPdf::generateDashes(const QPen &pen)
{
    QByteArray result;
    ByteStream s(&result);
    s << '[';

    QList<qreal> dasharray = pen.dashPattern();
    qreal w = pen.widthF();
    if (w < 0.001)
        w = 1;
    for (int i = 0; i < dasharray.size(); ++i) {
        qreal dw = dasharray.at(i)*w;
        if (dw < 0.0001) dw = 0.0001;
        s << dw;
    }
    s << ']';
    s << pen.dashOffset() * w;
    s << " d\n";
    return result;
}



static const char* const pattern_for_brush[] = {
    nullptr, // NoBrush
    nullptr, // SolidPattern
    "0 J\n"
    "6 w\n"
    "[] 0 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "0 4 m\n"
    "8 4 l\n"
    "S\n", // Dense1Pattern

    "0 J\n"
    "2 w\n"
    "[6 2] 1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[] 0 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[6 2] -3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense2Pattern

    "0 J\n"
    "2 w\n"
    "[6 2] 1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 2] -1 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[6 2] -3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense3Pattern

    "0 J\n"
    "2 w\n"
    "[2 2] 1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 2] -1 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[2 2] 1 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense4Pattern

    "0 J\n"
    "2 w\n"
    "[2 6] -1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 2] 1 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[2 6] 3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense5Pattern

    "0 J\n"
    "2 w\n"
    "[2 6] -1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 6] 3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense6Pattern

    "0 J\n"
    "2 w\n"
    "[2 6] -1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n", // Dense7Pattern

    "1 w\n"
    "0 4 m\n"
    "8 4 l\n"
    "S\n", // HorPattern

    "1 w\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // VerPattern

    "1 w\n"
    "4 0 m\n"
    "4 8 l\n"
    "0 4 m\n"
    "8 4 l\n"
    "S\n", // CrossPattern

    "1 w\n"
    "-1 5 m\n"
    "5 -1 l\n"
    "3 9 m\n"
    "9 3 l\n"
    "S\n", // BDiagPattern

    "1 w\n"
    "-1 3 m\n"
    "5 9 l\n"
    "3 -1 m\n"
    "9 5 l\n"
    "S\n", // FDiagPattern

    "1 w\n"
    "-1 3 m\n"
    "5 9 l\n"
    "3 -1 m\n"
    "9 5 l\n"
    "-1 5 m\n"
    "5 -1 l\n"
    "3 9 m\n"
    "9 3 l\n"
    "S\n", // DiagCrossPattern
};

QByteArray QPdf::patternForBrush(const QBrush &b)
{
    int style = b.style();
    if (style > Qt::DiagCrossPattern)
        return QByteArray();
    return pattern_for_brush[style];
}


static void moveToHook(qfixed x, qfixed y, void *data)
{
    QPdf::Stroker *t = (QPdf::Stroker *)data;
    if (!t->first)
        *t->stream << "h\n";
    if (!t->cosmeticPen)
        t->matrix.map(x, y, &x, &y);
    *t->stream << x << y << "m\n";
    t->first = false;
}

static void lineToHook(qfixed x, qfixed y, void *data)
{
    QPdf::Stroker *t = (QPdf::Stroker *)data;
    if (!t->cosmeticPen)
        t->matrix.map(x, y, &x, &y);
    *t->stream << x << y << "l\n";
}

static void cubicToHook(qfixed c1x, qfixed c1y,
                        qfixed c2x, qfixed c2y,
                        qfixed ex, qfixed ey,
                        void *data)
{
    QPdf::Stroker *t = (QPdf::Stroker *)data;
    if (!t->cosmeticPen) {
        t->matrix.map(c1x, c1y, &c1x, &c1y);
        t->matrix.map(c2x, c2y, &c2x, &c2y);
        t->matrix.map(ex, ey, &ex, &ey);
    }
    *t->stream << c1x << c1y
               << c2x << c2y
               << ex << ey
               << "c\n";
}

QPdf::Stroker::Stroker()
    : stream(nullptr),
    first(true),
    dashStroker(&basicStroker)
{
    stroker = &basicStroker;
    basicStroker.setMoveToHook(moveToHook);
    basicStroker.setLineToHook(lineToHook);
    basicStroker.setCubicToHook(cubicToHook);
    cosmeticPen = true;
    basicStroker.setStrokeWidth(.1);
}

void QPdf::Stroker::setPen(const QPen &pen, QPainter::RenderHints)
{
    if (pen.style() == Qt::NoPen) {
        stroker = nullptr;
        return;
    }
    qreal w = pen.widthF();
    bool zeroWidth = w < 0.0001;
    cosmeticPen = pen.isCosmetic();
    if (zeroWidth)
        w = .1;

    basicStroker.setStrokeWidth(w);
    basicStroker.setCapStyle(pen.capStyle());
    basicStroker.setJoinStyle(pen.joinStyle());
    basicStroker.setMiterLimit(pen.miterLimit());

    QList<qreal> dashpattern = pen.dashPattern();
    if (zeroWidth) {
        for (int i = 0; i < dashpattern.size(); ++i)
            dashpattern[i] *= 10.;
    }
    if (!dashpattern.isEmpty()) {
        dashStroker.setDashPattern(dashpattern);
        dashStroker.setDashOffset(pen.dashOffset());
        stroker = &dashStroker;
    } else {
        stroker = &basicStroker;
    }
}

void QPdf::Stroker::strokePath(const QPainterPath &path)
{
    if (!stroker)
        return;
    first = true;

    stroker->strokePath(path, this, cosmeticPen ? matrix : QTransform());
    *stream << "h f\n";
}

QByteArray QPdf::ascii85Encode(const QByteArray &input)
{
    int isize = input.size()/4*4;
    QByteArray output;
    output.resize(input.size()*5/4+7);
    char *out = output.data();
    const uchar *in = (const uchar *)input.constData();
    for (int i = 0; i < isize; i += 4) {
        uint val = (((uint)in[i])<<24) + (((uint)in[i+1])<<16) + (((uint)in[i+2])<<8) + (uint)in[i+3];
        if (val == 0) {
            *out = 'z';
            ++out;
        } else {
            char base[5];
            base[4] = val % 85;
            val /= 85;
            base[3] = val % 85;
            val /= 85;
            base[2] = val % 85;
            val /= 85;
            base[1] = val % 85;
            val /= 85;
            base[0] = val % 85;
            *(out++) = base[0] + '!';
            *(out++) = base[1] + '!';
            *(out++) = base[2] + '!';
            *(out++) = base[3] + '!';
            *(out++) = base[4] + '!';
        }
    }
    //write the last few bytes
    int remaining = input.size() - isize;
    if (remaining) {
        uint val = 0;
        for (int i = isize; i < input.size(); ++i)
            val = (val << 8) + in[i];
        val <<= 8*(4-remaining);
        char base[5];
        base[4] = val % 85;
        val /= 85;
        base[3] = val % 85;
        val /= 85;
        base[2] = val % 85;
        val /= 85;
        base[1] = val % 85;
        val /= 85;
        base[0] = val % 85;
        for (int i = 0; i < remaining+1; ++i)
            *(out++) = base[i] + '!';
    }
    *(out++) = '~';
    *(out++) = '>';
    output.resize(out-output.data());
    return output;
}

const char *QPdf::toHex(ushort u, char *buffer)
{
    int i = 3;
    while (i >= 0) {
        ushort hex = (u & 0x000f);
        if (hex < 0x0a)
            buffer[i] = '0'+hex;
        else
            buffer[i] = 'A'+(hex-0x0a);
        u = u >> 4;
        i--;
    }
    buffer[4] = '\0';
    return buffer;
}

const char *QPdf::toHex(uchar u, char *buffer)
{
    int i = 1;
    while (i >= 0) {
        ushort hex = (u & 0x000f);
        if (hex < 0x0a)
            buffer[i] = '0'+hex;
        else
            buffer[i] = 'A'+(hex-0x0a);
        u = u >> 4;
        i--;
    }
    buffer[2] = '\0';
    return buffer;
}


QPdfPage::QPdfPage()
    : QPdf::ByteStream(true) // Enable file backing
{
}

void QPdfPage::streamImage(int w, int h, uint object)
{
    *this << w << "0 0 " << -h << "0 " << h << "cm /Im" << object << " Do\n";
    if (!images.contains(object))
        images.append(object);
}


QPdfEngine::QPdfEngine(QPdfEnginePrivate &dd)
    : QPaintEngine(dd, qt_pdf_decide_features())
{
}

QPdfEngine::QPdfEngine()
    : QPaintEngine(*new QPdfEnginePrivate(), qt_pdf_decide_features())
{
}

void QPdfEngine::setOutputFilename(const QString &filename)
{
    Q_D(QPdfEngine);
    d->outputFileName = filename;
}


void QPdfEngine::drawPoints (const QPointF *points, int pointCount)
{
    if (!points)
        return;

    Q_D(QPdfEngine);
    QPainterPath p;
    for (int i=0; i!=pointCount;++i) {
        p.moveTo(points[i]);
        p.lineTo(points[i] + QPointF(0, 0.001));
    }

    bool hadBrush = d->hasBrush;
    d->hasBrush = false;
    drawPath(p);
    d->hasBrush = hadBrush;
}

void QPdfEngine::drawLines (const QLineF *lines, int lineCount)
{
    if (!lines)
        return;

    Q_D(QPdfEngine);
    QPainterPath p;
    for (int i=0; i!=lineCount;++i) {
        p.moveTo(lines[i].p1());
        p.lineTo(lines[i].p2());
    }
    bool hadBrush = d->hasBrush;
    d->hasBrush = false;
    drawPath(p);
    d->hasBrush = hadBrush;
}

void QPdfEngine::drawRects (const QRectF *rects, int rectCount)
{
    if (!rects)
        return;

    Q_D(QPdfEngine);

    if (d->clipEnabled && d->allClipped)
        return;
    if (!d->hasPen && !d->hasBrush)
        return;

    if ((d->simplePen && !d->needsTransform) || !d->hasPen) {
        // draw natively in this case for better output
        if (!d->hasPen && d->needsTransform) // i.e. this is just a fillrect
            *d->currentPage << "q\n" << QPdf::generateMatrix(d->stroker.matrix);
        for (int i = 0; i < rectCount; ++i)
            *d->currentPage << rects[i].x() << rects[i].y() << rects[i].width() << rects[i].height() << "re\n";
        *d->currentPage << (d->hasPen ? (d->hasBrush ? "B\n" : "S\n") : "f\n");
        if (!d->hasPen && d->needsTransform)
            *d->currentPage << "Q\n";
    } else {
        QPainterPath p;
        for (int i=0; i!=rectCount; ++i)
            p.addRect(rects[i]);
        drawPath(p);
    }
}

void QPdfEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QPdfEngine);

    if (!points || !pointCount)
        return;

    bool hb = d->hasBrush;
    QPainterPath p;

    switch(mode) {
        case OddEvenMode:
            p.setFillRule(Qt::OddEvenFill);
            break;
        case ConvexMode:
        case WindingMode:
            p.setFillRule(Qt::WindingFill);
            break;
        case PolylineMode:
            d->hasBrush = false;
            break;
        default:
            break;
    }

    p.moveTo(points[0]);
    for (int i = 1; i < pointCount; ++i)
        p.lineTo(points[i]);

    if (mode != PolylineMode)
        p.closeSubpath();
    drawPath(p);

    d->hasBrush = hb;
}

void QPdfEngine::drawPath (const QPainterPath &p)
{
    Q_D(QPdfEngine);

    if (d->clipEnabled && d->allClipped)
        return;
    if (!d->hasPen && !d->hasBrush)
        return;

    if (d->simplePen) {
        // draw strokes natively in this case for better output
        *d->currentPage << QPdf::generatePath(p, d->needsTransform ? d->stroker.matrix : QTransform(),
                                              d->hasBrush ? QPdf::FillAndStrokePath : QPdf::StrokePath);
    } else {
        if (d->hasBrush)
            *d->currentPage << QPdf::generatePath(p, d->stroker.matrix, QPdf::FillPath);
        if (d->hasPen) {
            *d->currentPage << "q\n";
            QBrush b = d->brush;
            d->brush = d->pen.brush();
            setBrush();
            d->stroker.strokePath(p);
            *d->currentPage << "Q\n";
            d->brush = b;
        }
    }
}

void QPdfEngine::drawPixmap (const QRectF &rectangle, const QPixmap &pixmap, const QRectF &sr)
{
    if (sr.isEmpty() || rectangle.isEmpty() || pixmap.isNull())
        return;
    Q_D(QPdfEngine);

    QBrush b = d->brush;

    QRect sourceRect = sr.toRect();
    QPixmap pm = sourceRect != pixmap.rect() ? pixmap.copy(sourceRect) : pixmap;
    QImage image = pm.toImage();
    bool bitmap = true;
    const bool lossless = painter()->testRenderHint(QPainter::LosslessImageRendering);
    const int object = d->addImage(image, &bitmap, lossless, pm.cacheKey());
    if (object < 0)
        return;

    *d->currentPage << "q\n";

    if ((d->pdfVersion != QPdfEngine::Version_A1b) && (d->opacity != 1.0)) {
        int stateObject = d->addConstantAlphaObject(qRound(255 * d->opacity), qRound(255 * d->opacity));
        if (stateObject)
            *d->currentPage << "/GState" << stateObject << "gs\n";
        else
            *d->currentPage << "/GSa gs\n";
    } else {
        *d->currentPage << "/GSa gs\n";
    }

    *d->currentPage
        << QPdf::generateMatrix(QTransform(rectangle.width() / sr.width(), 0, 0, rectangle.height() / sr.height(),
                                           rectangle.x(), rectangle.y()) * (!d->needsTransform ? QTransform() : d->stroker.matrix));
    if (bitmap) {
        // set current pen as d->brush
        d->brush = d->pen.brush();
    }
    setBrush();
    d->currentPage->streamImage(image.width(), image.height(), object);
    *d->currentPage << "Q\n";

    d->brush = b;
}

void QPdfEngine::drawImage(const QRectF &rectangle, const QImage &image, const QRectF &sr, Qt::ImageConversionFlags)
{
    if (sr.isEmpty() || rectangle.isEmpty() || image.isNull())
        return;
    Q_D(QPdfEngine);

    QRect sourceRect = sr.toRect();
    QImage im = sourceRect != image.rect() ? image.copy(sourceRect) : image;
    bool bitmap = true;
    const bool lossless = painter()->testRenderHint(QPainter::LosslessImageRendering);
    const int object = d->addImage(im, &bitmap, lossless, im.cacheKey());
    if (object < 0)
        return;

    *d->currentPage << "q\n";

    if ((d->pdfVersion != QPdfEngine::Version_A1b) && (d->opacity != 1.0)) {
        int stateObject = d->addConstantAlphaObject(qRound(255 * d->opacity), qRound(255 * d->opacity));
        if (stateObject)
            *d->currentPage << "/GState" << stateObject << "gs\n";
        else
            *d->currentPage << "/GSa gs\n";
    } else {
        *d->currentPage << "/GSa gs\n";
    }

    *d->currentPage
        << QPdf::generateMatrix(QTransform(rectangle.width() / sr.width(), 0, 0, rectangle.height() / sr.height(),
                                           rectangle.x(), rectangle.y()) * (!d->needsTransform ? QTransform() : d->stroker.matrix));
    setBrush();
    d->currentPage->streamImage(im.width(), im.height(), object);
    *d->currentPage << "Q\n";
}

void QPdfEngine::drawTiledPixmap (const QRectF &rectangle, const QPixmap &pixmap, const QPointF &point)
{
    Q_D(QPdfEngine);

    bool bitmap = (pixmap.depth() == 1);
    QBrush b = d->brush;
    QPointF bo = d->brushOrigin;
    bool hp = d->hasPen;
    d->hasPen = false;
    bool hb = d->hasBrush;
    d->hasBrush = true;

    d->brush = QBrush(pixmap);
    if (bitmap)
        // #### fix bitmap case where we have a brush pen
        d->brush.setColor(d->pen.color());

    d->brushOrigin = -point;
    *d->currentPage << "q\n";
    setBrush();

    drawRects(&rectangle, 1);
    *d->currentPage << "Q\n";

    d->hasPen = hp;
    d->hasBrush = hb;
    d->brush = b;
    d->brushOrigin = bo;
}

void QPdfEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    Q_D(QPdfEngine);

    if (!d->hasPen || (d->clipEnabled && d->allClipped))
        return;

    if (d->stroker.matrix.type() >= QTransform::TxProject) {
        QPaintEngine::drawTextItem(p, textItem);
        return;
    }

    *d->currentPage << "q\n";
    if (d->needsTransform)
        *d->currentPage << QPdf::generateMatrix(d->stroker.matrix);

    bool hp = d->hasPen;
    d->hasPen = false;
    QBrush b = d->brush;
    d->brush = d->pen.brush();
    setBrush();

    const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);
    Q_ASSERT(ti.fontEngine->type() != QFontEngine::Multi);
    d->drawTextItem(p, ti);
    d->hasPen = hp;
    d->brush = b;
    *d->currentPage << "Q\n";
}

// Used by QtWebKit
void QPdfEngine::drawHyperlink(const QRectF &r, const QUrl &url)
{
    Q_D(QPdfEngine);

    // PDF/X-4 (§ 6.17) does not allow annotations that don't lie
    // outside the BleedBox/TrimBox, so don't emit an hyperlink
    // annotation at all.
    if (d->pdfVersion == QPdfEngine::Version_X4)
        return;

    const uint annot = d->addXrefEntry(-1);
    const QByteArray urlascii = url.toEncoded();
    int len = urlascii.size();
    QVarLengthArray<char> url_esc;
    url_esc.reserve(len + 1);
    for (int j = 0; j < len; j++) {
        if (urlascii[j] == '(' || urlascii[j] == ')' || urlascii[j] == '\\')
            url_esc.append('\\');
        url_esc.append(urlascii[j]);
    }
    url_esc.append('\0');

    char buf[256];
    const QRectF rr = d->pageMatrix().mapRect(r);
    d->xprintf("<<\n/Type /Annot\n/Subtype /Link\n");

    if (d->pdfVersion == QPdfEngine::Version_A1b)
        d->xprintf("/F 4\n"); // enable print flag, disable all other

    d->xprintf("/Rect [");
    d->xprintf("%s ", qt_real_to_string(rr.left(), buf));
    d->xprintf("%s ", qt_real_to_string(rr.top(), buf));
    d->xprintf("%s ", qt_real_to_string(rr.right(), buf));
    d->xprintf("%s", qt_real_to_string(rr.bottom(), buf));
    d->xprintf("]\n/Border [0 0 0]\n/A <<\n");
    d->xprintf("/Type /Action\n/S /URI\n/URI (%s)\n", url_esc.constData());
    d->xprintf(">>\n>>\n");
    d->xprintf("endobj\n");
    d->currentPage->annotations.append(annot);
}

void QPdfEngine::updateState(const QPaintEngineState &state)
{
    Q_D(QPdfEngine);

    QPaintEngine::DirtyFlags flags = state.state();

    if (flags & DirtyHints)
        flags |= DirtyBrush;

    if (flags & DirtyTransform)
        d->stroker.matrix = state.transform();

    if (flags & DirtyPen) {
        if (d->pdfVersion == QPdfEngine::Version_A1b) {
            QPen pen = state.pen();

            QColor penColor = pen.color();
            if (penColor.alpha() != 255)
                penColor.setAlpha(255);
            pen.setColor(penColor);

            QBrush penBrush = pen.brush();
            removeTransparencyFromBrush(penBrush);
            pen.setBrush(penBrush);

            d->pen = pen;
        } else {
            d->pen = state.pen();
        }
        d->hasPen = d->pen.style() != Qt::NoPen;
        bool oldCosmetic = d->stroker.cosmeticPen;
        d->stroker.setPen(d->pen, state.renderHints());
        QBrush penBrush = d->pen.brush();
        bool oldSimple = d->simplePen;
        d->simplePen = (d->hasPen && (penBrush.style() == Qt::SolidPattern) && penBrush.isOpaque() && d->opacity == 1.0);
        if (oldSimple != d->simplePen || oldCosmetic != d->stroker.cosmeticPen)
            flags |= DirtyTransform;
    } else if (flags & DirtyHints) {
        d->stroker.setPen(d->pen, state.renderHints());
    }
    if (flags & DirtyBrush) {
        if (d->pdfVersion == QPdfEngine::Version_A1b) {
            QBrush brush = state.brush();
            removeTransparencyFromBrush(brush);
            d->brush = brush;
        } else {
            d->brush = state.brush();
        }
        if (d->brush.color().alpha() == 0 && d->brush.style() == Qt::SolidPattern)
            d->brush.setStyle(Qt::NoBrush);
        d->hasBrush = d->brush.style() != Qt::NoBrush;
    }
    if (flags & DirtyBrushOrigin) {
        d->brushOrigin = state.brushOrigin();
        flags |= DirtyBrush;
    }
    if (flags & DirtyOpacity) {
        d->opacity = state.opacity();
        if (d->simplePen && d->opacity != 1.0) {
            d->simplePen = false;
            flags |= DirtyTransform;
        }
    }

    bool ce = d->clipEnabled;
    if (flags & DirtyClipPath) {
        d->clipEnabled = true;
        updateClipPath(state.clipPath(), state.clipOperation());
    } else if (flags & DirtyClipRegion) {
        d->clipEnabled = true;
        QPainterPath path;
        for (const QRect &rect : state.clipRegion())
            path.addRect(rect);
        updateClipPath(path, state.clipOperation());
        flags |= DirtyClipPath;
    } else if (flags & DirtyClipEnabled) {
        d->clipEnabled = state.isClipEnabled();
    }

    if (ce != d->clipEnabled)
        flags |= DirtyClipPath;
    else if (!d->clipEnabled)
        flags &= ~DirtyClipPath;

    setupGraphicsState(flags);
}

void QPdfEngine::setupGraphicsState(QPaintEngine::DirtyFlags flags)
{
    Q_D(QPdfEngine);
    if (flags & DirtyClipPath)
        flags |= DirtyTransform|DirtyPen|DirtyBrush;

    if (flags & DirtyTransform) {
        *d->currentPage << "Q\n";
        flags |= DirtyPen|DirtyBrush;
    }

    if (flags & DirtyClipPath) {
        *d->currentPage << "Q q\n";

        d->allClipped = false;
        if (d->clipEnabled && !d->clips.isEmpty()) {
            for (int i = 0; i < d->clips.size(); ++i) {
                if (d->clips.at(i).isEmpty()) {
                    d->allClipped = true;
                    break;
                }
            }
            if (!d->allClipped) {
                for (int i = 0; i < d->clips.size(); ++i) {
                    *d->currentPage << QPdf::generatePath(d->clips.at(i), QTransform(), QPdf::ClipPath);
                }
            }
        }
    }

    if (flags & DirtyTransform) {
        *d->currentPage << "q\n";
        d->needsTransform = false;
        if (!d->stroker.matrix.isIdentity()) {
            if (d->simplePen && !d->stroker.cosmeticPen)
                *d->currentPage << QPdf::generateMatrix(d->stroker.matrix);
            else
                d->needsTransform = true; // I.e. page-wide xf not set, local xf needed
        }
    }
    if (flags & DirtyBrush)
        setBrush();
    if (d->simplePen && (flags & DirtyPen))
        setPen();
}

void QPdfEngine::updateClipPath(const QPainterPath &p, Qt::ClipOperation op)
{
    Q_D(QPdfEngine);
    QPainterPath path = d->stroker.matrix.map(p);
    //qDebug() << "updateClipPath: " << d->stroker.matrix << p.boundingRect() << path.boundingRect() << op;

    switch (op) {
    case Qt::NoClip:
        d->clipEnabled = false;
        d->clips.clear();
        break;
    case Qt::ReplaceClip:
        d->clips.clear();
        d->clips.append(path);
        break;
    case Qt::IntersectClip:
        d->clips.append(path);
        break;
    }
}

void QPdfEngine::setPen()
{
    Q_D(QPdfEngine);
    if (d->pen.style() == Qt::NoPen)
        return;
    QBrush b = d->pen.brush();
    Q_ASSERT(b.style() == Qt::SolidPattern && b.isOpaque());

    d->writeColor(QPdfEnginePrivate::ColorDomain::Stroking, b.color());
    *d->currentPage << "SCN\n";
    *d->currentPage << d->pen.widthF() << "w ";

    int pdfCapStyle = 0;
    switch(d->pen.capStyle()) {
    case Qt::FlatCap:
        pdfCapStyle = 0;
        break;
    case Qt::SquareCap:
        pdfCapStyle = 2;
        break;
    case Qt::RoundCap:
        pdfCapStyle = 1;
        break;
    default:
        break;
    }
    *d->currentPage << pdfCapStyle << "J ";

    int pdfJoinStyle = 0;
    switch(d->pen.joinStyle()) {
    case Qt::MiterJoin:
    case Qt::SvgMiterJoin:
        *d->currentPage << qMax(qreal(1.0), d->pen.miterLimit()) << "M ";
        pdfJoinStyle = 0;
        break;
    case Qt::BevelJoin:
        pdfJoinStyle = 2;
        break;
    case Qt::RoundJoin:
        pdfJoinStyle = 1;
        break;
    default:
        break;
    }
    *d->currentPage << pdfJoinStyle << "j ";

    *d->currentPage << QPdf::generateDashes(d->pen);
}


void QPdfEngine::setBrush()
{
    Q_D(QPdfEngine);
    Qt::BrushStyle style = d->brush.style();
    if (style == Qt::NoBrush)
        return;

    bool specifyColor;
    int gStateObject = 0;
    int patternObject = d->addBrushPattern(d->stroker.matrix, &specifyColor, &gStateObject);
    if (!patternObject && !specifyColor)
        return;

    const auto domain = patternObject ? QPdfEnginePrivate::ColorDomain::NonStrokingPattern
                                      : QPdfEnginePrivate::ColorDomain::NonStroking;
    d->writeColor(domain, specifyColor ? d->brush.color() : QColor());
    if (patternObject)
        *d->currentPage << "/Pat" << patternObject;
    *d->currentPage << "scn\n";

    if (gStateObject)
        *d->currentPage << "/GState" << gStateObject << "gs\n";
    else
        *d->currentPage << "/GSa gs\n";
}


bool QPdfEngine::newPage()
{
    Q_D(QPdfEngine);
    if (!isActive())
        return false;
    d->newPage();

    setupGraphicsState(DirtyBrush|DirtyPen|DirtyClipPath);
    QFile *outfile = qobject_cast<QFile*> (d->outDevice);
    if (outfile && outfile->error() != QFile::NoError)
        return false;
    return true;
}

QPaintEngine::Type QPdfEngine::type() const
{
    return QPaintEngine::Pdf;
}

void QPdfEngine::setResolution(int resolution)
{
    Q_D(QPdfEngine);
    d->resolution = resolution;
}

int QPdfEngine::resolution() const
{
    Q_D(const QPdfEngine);
    return d->resolution;
}

void QPdfEngine::setPdfVersion(PdfVersion version)
{
    Q_D(QPdfEngine);
    d->pdfVersion = version;
}

void QPdfEngine::setDocumentXmpMetadata(const QByteArray &xmpMetadata)
{
    Q_D(QPdfEngine);
    d->xmpDocumentMetadata = xmpMetadata;
}

QByteArray QPdfEngine::documentXmpMetadata() const
{
    Q_D(const QPdfEngine);
    return d->xmpDocumentMetadata;
}

void QPdfEngine::setPageLayout(const QPageLayout &pageLayout)
{
    Q_D(QPdfEngine);
    d->m_pageLayout = pageLayout;
}

void QPdfEngine::setPageSize(const QPageSize &pageSize)
{
    Q_D(QPdfEngine);
    d->m_pageLayout.setPageSize(pageSize);
}

void QPdfEngine::setPageOrientation(QPageLayout::Orientation orientation)
{
    Q_D(QPdfEngine);
    d->m_pageLayout.setOrientation(orientation);
}

void QPdfEngine::setPageMargins(const QMarginsF &margins, QPageLayout::Unit units)
{
    Q_D(QPdfEngine);
    d->m_pageLayout.setUnits(units);
    d->m_pageLayout.setMargins(margins);
}

QPageLayout QPdfEngine::pageLayout() const
{
    Q_D(const QPdfEngine);
    return d->m_pageLayout;
}

// Metrics are in Device Pixels
int QPdfEngine::metric(QPaintDevice::PaintDeviceMetric metricType) const
{
    Q_D(const QPdfEngine);
    int val;
    switch (metricType) {
    case QPaintDevice::PdmWidth:
        val = d->m_pageLayout.paintRectPixels(d->resolution).width();
        break;
    case QPaintDevice::PdmHeight:
        val = d->m_pageLayout.paintRectPixels(d->resolution).height();
        break;
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmDpiY:
        val = d->resolution;
        break;
    case QPaintDevice::PdmPhysicalDpiX:
    case QPaintDevice::PdmPhysicalDpiY:
        val = 1200;
        break;
    case QPaintDevice::PdmWidthMM:
        val = qRound(d->m_pageLayout.paintRect(QPageLayout::Millimeter).width());
        break;
    case QPaintDevice::PdmHeightMM:
        val = qRound(d->m_pageLayout.paintRect(QPageLayout::Millimeter).height());
        break;
    case QPaintDevice::PdmNumColors:
        val = INT_MAX;
        break;
    case QPaintDevice::PdmDepth:
        val = 32;
        break;
    case QPaintDevice::PdmDevicePixelRatio:
        val = 1;
        break;
    case QPaintDevice::PdmDevicePixelRatioScaled:
        val = 1 * QPaintDevice::devicePixelRatioFScale();
        break;
    default:
        qWarning("QPdfWriter::metric: Invalid metric command");
        return 0;
    }
    return val;
}

QPdfEnginePrivate::QPdfEnginePrivate()
    : clipEnabled(false), allClipped(false), hasPen(true), hasBrush(false), simplePen(false),
      needsTransform(false), pdfVersion(QPdfEngine::Version_1_4),
      colorModel(QPdfEngine::ColorModel::Auto),
      outDevice(nullptr), ownsDevice(false),
      embedFonts(true),
      m_pageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(10, 10, 10, 10))
{
    initResources();
    resolution = 1200;
    currentObject = 1;
    currentPage = nullptr;
    stroker.stream = nullptr;

    streampos = 0;

    stream = new QDataStream;
}

bool QPdfEngine::begin(QPaintDevice *pdev)
{
    Q_D(QPdfEngine);
    d->pdev = pdev;

    if (!d->outDevice) {
        if (!d->outputFileName.isEmpty()) {
            QFile *file = new QFile(d->outputFileName);
            if (!file->open(QFile::WriteOnly|QFile::Truncate)) {
                delete file;
                return false;
            }
            d->outDevice = file;
        } else {
            return false;
        }
        d->ownsDevice = true;
    }

    d->currentObject = 1;

    d->currentPage = new QPdfPage;
    d->stroker.stream = d->currentPage;
    d->opacity = 1.0;

    d->stream->setDevice(d->outDevice);

    d->streampos = 0;
    d->hasPen = true;
    d->hasBrush = false;
    d->clipEnabled = false;
    d->allClipped = false;

    d->xrefPositions.clear();
    d->pageRoot = 0;
    d->namesRoot = 0;
    d->destsRoot = 0;
    d->attachmentsRoot = 0;
    d->catalog = 0;
    d->info = 0;
    d->graphicsState = 0;
    d->patternColorSpaceRGB = 0;
    d->patternColorSpaceGrayscale = 0;
    d->patternColorSpaceCMYK = 0;
    d->simplePen = false;
    d->needsTransform = false;

    d->pages.clear();
    d->imageCache.clear();
    d->alphaCache.clear();

    setActive(true);
    d->writeHeader();
    newPage();

    return true;
}

bool QPdfEngine::end()
{
    Q_D(QPdfEngine);
    d->writeTail();

    d->stream->setDevice(nullptr);

    qDeleteAll(d->fonts);
    d->fonts.clear();
    delete d->currentPage;
    d->currentPage = nullptr;

    if (d->outDevice && d->ownsDevice) {
        d->outDevice->close();
        delete d->outDevice;
        d->outDevice = nullptr;
    }

    d->destCache.clear();
    d->fileCache.clear();

    setActive(false);
    return true;
}

void QPdfEngine::addFileAttachment(const QString &fileName, const QByteArray &data, const QString &mimeType)
{
    Q_D(QPdfEngine);
    d->fileCache.push_back({fileName, data, mimeType});
}

QPdfEnginePrivate::~QPdfEnginePrivate()
{
    qDeleteAll(fonts);
    delete currentPage;
    delete stream;
}

void QPdfEnginePrivate::writeHeader()
{
    addXrefEntry(0,false);

    // Keep in sync with QPdfEngine::PdfVersion!
    static const char mapping[][4] = {
        "1.4", // Version_1_4
        "1.4", // Version_A1b
        "1.6", // Version_1_6
        "1.6", // Version_X4
    };
    static const size_t numMappings = sizeof mapping / sizeof *mapping;
    const char *verStr = mapping[size_t(pdfVersion) < numMappings ? pdfVersion : 0];

    xprintf("%%PDF-%s\n", verStr);
    xprintf("%%\303\242\303\243\n");

#if QT_CONFIG(timezone)
    const QDateTime now = QDateTime::currentDateTime(QTimeZone::systemTimeZone());
#else
    const QDateTime now = QDateTime::currentDateTimeUtc();
#endif

    writeInfo(now);

    const int metaDataObj = writeXmpDocumentMetaData(now);
    const int outputIntentObj = [&]() {
        switch (pdfVersion) {
        case QPdfEngine::Version_1_4:
        case QPdfEngine::Version_1_6:
            break;
        case QPdfEngine::Version_A1b:
        case QPdfEngine::Version_X4:
            return writeOutputIntent();
        }

        return -1;
    }();

    catalog = addXrefEntry(-1);
    pageRoot = requestObject();
    namesRoot = requestObject();

    // catalog
    {
        QByteArray catalog;
        QPdf::ByteStream s(&catalog);
        s << "<<\n"
          << "/Type /Catalog\n"
          << "/Pages " << pageRoot << "0 R\n"
          << "/Names " << namesRoot << "0 R\n";

        s << "/Metadata " << metaDataObj << "0 R\n";

        if (outputIntentObj >= 0)
            s << "/OutputIntents [" << outputIntentObj << "0 R]\n";

        s << ">>\n"
          << "endobj\n";

        write(catalog);
    }

    // graphics state
    graphicsState = addXrefEntry(-1);
    xprintf("<<\n"
            "/Type /ExtGState\n"
            "/SA true\n"
            "/SM 0.02\n"
            "/ca 1.0\n"
            "/CA 1.0\n"
            "/AIS false\n"
            "/SMask /None"
            ">>\n"
            "endobj\n");

    // color spaces for pattern
    patternColorSpaceRGB = addXrefEntry(-1);
    xprintf("[/Pattern /DeviceRGB]\n"
            "endobj\n");
    patternColorSpaceGrayscale = addXrefEntry(-1);
    xprintf("[/Pattern /DeviceGray]\n"
            "endobj\n");
    patternColorSpaceCMYK = addXrefEntry(-1);
    xprintf("[/Pattern /DeviceCMYK]\n"
            "endobj\n");
}

QPdfEngine::ColorModel QPdfEnginePrivate::colorModelForColor(const QColor &color) const
{
    switch (colorModel) {
    case QPdfEngine::ColorModel::RGB:
    case QPdfEngine::ColorModel::Grayscale:
    case QPdfEngine::ColorModel::CMYK:
        return colorModel;
    case QPdfEngine::ColorModel::Auto:
        switch (color.spec()) {
        case QColor::Invalid:
        case QColor::Rgb:
        case QColor::Hsv:
        case QColor::Hsl:
        case QColor::ExtendedRgb:
            return QPdfEngine::ColorModel::RGB;
        case QColor::Cmyk:
            return QPdfEngine::ColorModel::CMYK;
        }

        break;
    }

    Q_UNREACHABLE_RETURN(QPdfEngine::ColorModel::RGB);
}

void QPdfEnginePrivate::writeColor(ColorDomain domain, const QColor &color)
{
    // Switch to the right colorspace.
    // For simplicity: do it even if it redundant (= already in that colorspace)
    const QPdfEngine::ColorModel actualColorModel = colorModelForColor(color);

    switch (actualColorModel) {
    case QPdfEngine::ColorModel::RGB:
        switch (domain) {
        case ColorDomain::Stroking:
            *currentPage << "/CSp CS\n"; break;
        case ColorDomain::NonStroking:
            *currentPage << "/CSp cs\n"; break;
        case ColorDomain::NonStrokingPattern:
            *currentPage << "/PCSp cs\n"; break;
        }
        break;
    case QPdfEngine::ColorModel::Grayscale:
        switch (domain) {
        case ColorDomain::Stroking:
            *currentPage << "/CSpg CS\n"; break;
        case ColorDomain::NonStroking:
            *currentPage << "/CSpg cs\n"; break;
        case ColorDomain::NonStrokingPattern:
            *currentPage << "/PCSpg cs\n"; break;
        }
        break;
    case QPdfEngine::ColorModel::CMYK:
        switch (domain) {
        case ColorDomain::Stroking:
            *currentPage << "/CSpcmyk CS\n"; break;
        case ColorDomain::NonStroking:
            *currentPage << "/CSpcmyk cs\n"; break;
        case ColorDomain::NonStrokingPattern:
            *currentPage << "/PCSpcmyk cs\n"; break;
        }
        break;
    case QPdfEngine::ColorModel::Auto:
        Q_UNREACHABLE_RETURN();
    }

    // If we also have a color specified, write it out.
    if (!color.isValid())
        return;

    switch (actualColorModel) {
    case QPdfEngine::ColorModel::RGB:
        *currentPage << color.redF()
                     << color.greenF()
                     << color.blueF();
        break;
    case QPdfEngine::ColorModel::Grayscale: {
        const qreal gray = qGray(color.rgba()) / 255.;
        *currentPage << gray;
        break;
    }
    case QPdfEngine::ColorModel::CMYK:
        *currentPage << color.cyanF()
                     << color.magentaF()
                     << color.yellowF()
                     << color.blackF();
        break;
    case QPdfEngine::ColorModel::Auto:
        Q_UNREACHABLE_RETURN();
    }
}

void QPdfEnginePrivate::writeInfo(const QDateTime &date)
{
    info = addXrefEntry(-1);
    write("<<\n/Title ");
    printString(title);
    write("\n/Creator ");
    printString(creator);
    write("\n/Author ");
    printString(author);
    write("\n/Producer ");
    printString(QString::fromLatin1("Qt " QT_VERSION_STR));

    const QTime t = date.time();
    const QDate d = date.date();
    // (D:YYYYMMDDHHmmSSOHH'mm')
    constexpr size_t formattedDateSize = 26;
    char formattedDate[formattedDateSize];
    const int year = qBound(0, d.year(), 9999); // ASN.1, max 4 digits
    auto printedSize = std::snprintf(formattedDate,
                                     formattedDateSize,
                                     "(D:%04d%02d%02d%02d%02d%02d",
                                     year,
                                     d.month(),
                                     d.day(),
                                     t.hour(),
                                     t.minute(),
                                     t.second());
    const int offset = date.offsetFromUtc();
    const int hours  = (offset / 60) / 60;
    const int mins   = (offset / 60) % 60;
    if (offset < 0) {
        std::snprintf(formattedDate + printedSize,
                      formattedDateSize - printedSize,
                      "-%02d'%02d')", -hours, -mins);
    } else if (offset > 0) {
        std::snprintf(formattedDate + printedSize,
                      formattedDateSize - printedSize,
                      "+%02d'%02d')", hours, mins);
    } else {
        std::snprintf(formattedDate + printedSize,
                      formattedDateSize - printedSize,
                      "Z)");
    }

    write("\n/CreationDate ");
    write(formattedDate);
    write("\n/ModDate ");
    write(formattedDate);

    write("\n/Trapped /False\n"
          ">>\n"
          "endobj\n");
}

int QPdfEnginePrivate::writeXmpDocumentMetaData(const QDateTime &date)
{
    const int metaDataObj = addXrefEntry(-1);
    QByteArray metaDataContent;

    if (!xmpDocumentMetadata.isEmpty()) {
        metaDataContent = xmpDocumentMetadata;
    } else {
        const QString producer(QString::fromLatin1("Qt " QT_VERSION_STR));
        const QString metaDataDate = date.toString(Qt::ISODate);

        using namespace Qt::Literals;
        constexpr QLatin1String xmlNS = "http://www.w3.org/XML/1998/namespace"_L1;

        constexpr QLatin1String adobeNS = "adobe:ns:meta/"_L1;
        constexpr QLatin1String rdfNS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#"_L1;
        constexpr QLatin1String dcNS = "http://purl.org/dc/elements/1.1/"_L1;
        constexpr QLatin1String xmpNS = "http://ns.adobe.com/xap/1.0/"_L1;
        constexpr QLatin1String xmpMMNS = "http://ns.adobe.com/xap/1.0/mm/"_L1;
        constexpr QLatin1String pdfNS = "http://ns.adobe.com/pdf/1.3/"_L1;
        constexpr QLatin1String pdfaidNS = "http://www.aiim.org/pdfa/ns/id/"_L1;
        constexpr QLatin1String pdfxidNS = "http://www.npes.org/pdfx/ns/id/"_L1;

        QBuffer output(&metaDataContent);
        output.open(QIODevice::WriteOnly);
        output.write("<?xpacket begin='' id='W5M0MpCehiHzreSzNTczkc9d'?>");

        QXmlStreamWriter w(&output);
        w.setAutoFormatting(true);
        w.writeNamespace(adobeNS, "x");
        w.writeNamespace(rdfNS, "rdf");
        w.writeNamespace(dcNS, "dc");
        w.writeNamespace(xmpNS, "xmp");
        w.writeNamespace(xmpMMNS, "xmpMM");
        w.writeNamespace(pdfNS, "pdf");
        w.writeNamespace(pdfaidNS, "pdfaid");
        w.writeNamespace(pdfxidNS, "pdfxid");

        w.writeStartElement(adobeNS, "xmpmeta");
        w.writeStartElement(rdfNS, "RDF");

        /*
            XMP says: "The recommended approach is to have either a
            single rdf:Description element containing all XMP
            properties or a separate rdf:Description element for each
            XMP property namespace."
            We do the the latter.
        */

        // DC
        w.writeStartElement(rdfNS, "Description");
        w.writeAttribute(rdfNS, "about", "");
            w.writeStartElement(dcNS, "title");
                w.writeStartElement(rdfNS, "Alt");
                    w.writeStartElement(rdfNS, "li");
                    w.writeAttribute(xmlNS, "lang", "x-default");
                        w.writeCharacters(title);
                    w.writeEndElement();
                w.writeEndElement();
            w.writeEndElement();
            w.writeStartElement(dcNS, "creator");
                w.writeStartElement(rdfNS, "Seq");
                    w.writeStartElement(rdfNS, "li");
                        w.writeCharacters(author);
                    w.writeEndElement();
                w.writeEndElement();
            w.writeEndElement();
        w.writeEndElement();

        // PDF
        w.writeStartElement(rdfNS, "Description");
        w.writeAttribute(rdfNS, "about", "");
        w.writeAttribute(pdfNS, "Producer", producer);
        w.writeAttribute(pdfNS, "Trapped", "False");
        w.writeEndElement();

        // XMP
        w.writeStartElement(rdfNS, "Description");
        w.writeAttribute(rdfNS, "about", "");
        w.writeAttribute(xmpNS, "CreatorTool", creator);
        w.writeAttribute(xmpNS, "CreateDate", metaDataDate);
        w.writeAttribute(xmpNS, "ModifyDate", metaDataDate);
        w.writeAttribute(xmpNS, "MetadataDate", metaDataDate);
        w.writeEndElement();

        // XMPMM
        w.writeStartElement(rdfNS, "Description");
        w.writeAttribute(rdfNS, "about", "");
        w.writeAttribute(xmpMMNS, "DocumentID", "uuid:"_L1 + documentId.toString(QUuid::WithoutBraces));
        w.writeAttribute(xmpMMNS, "VersionID", "1");
        w.writeAttribute(xmpMMNS, "RenditionClass", "default");
        w.writeEndElement();

        // Version-specific
        switch (pdfVersion) {
        case QPdfEngine::Version_1_4:
            break;
        case QPdfEngine::Version_A1b:
            w.writeStartElement(rdfNS, "Description");
            w.writeAttribute(rdfNS, "about", "");
            w.writeAttribute(pdfaidNS, "part", "1");
            w.writeAttribute(pdfaidNS, "conformance", "B");
            w.writeEndElement();
            break;
        case QPdfEngine::Version_1_6:
            break;
        case QPdfEngine::Version_X4:
            w.writeStartElement(rdfNS, "Description");
            w.writeAttribute(rdfNS, "about", "");
            w.writeAttribute(pdfxidNS, "GTS_PDFXVersion", "PDF/X-4");
            w.writeEndElement();
            break;
        }

        w.writeEndElement(); // </RDF>
        w.writeEndElement(); // </xmpmeta>

        w.writeEndDocument();
        output.write("<?xpacket end='w'?>");
    }

    xprintf("<<\n"
            "/Type /Metadata /Subtype /XML\n"
            "/Length %" PRIdQSIZETYPE "\n"
            ">>\n"
            "stream\n", metaDataContent.size());
    write(metaDataContent);
    xprintf("\nendstream\n"
            "endobj\n");

    return metaDataObj;
}

int QPdfEnginePrivate::writeOutputIntent()
{
    const int colorProfileEntry = addXrefEntry(-1);
    {
        const QColorSpace profile = outputIntent.outputProfile();
        const QByteArray colorProfileData = profile.iccProfile();

        QByteArray data;
        QPdf::ByteStream s(&data);
        int length_object = requestObject();

        s << "<<\n";

        switch (profile.colorModel()) {
        case QColorSpace::ColorModel::Undefined:
            qWarning("QPdfEngine: undefined color model in the output intent profile, assuming RGB");
            [[fallthrough]];
        case QColorSpace::ColorModel::Rgb:
            s << "/N 3\n";
            s << "/Alternate /DeviceRGB\n";
            break;
        case QColorSpace::ColorModel::Gray:
            s << "/N 1\n";
            s << "/Alternate /DeviceGray\n";
            break;
        case QColorSpace::ColorModel::Cmyk:
            s << "/N 4\n";
            s << "/Alternate /DeviceCMYK\n";
            break;
        }

        s << "/Length " << length_object << "0 R\n";
        if (do_compress)
            s << "/Filter /FlateDecode\n";
        s << ">>\n";
        s << "stream\n";
        write(data);
        const int len = writeCompressed(colorProfileData);
        write("\nendstream\n"
              "endobj\n");
        addXrefEntry(length_object);
        xprintf("%d\n"
                "endobj\n", len);
    }

    const int outputIntentEntry = addXrefEntry(-1);
    {
        write("<<\n");
        write("/Type /OutputIntent\n");

        switch (pdfVersion) {
        case QPdfEngine::Version_1_4:
        case QPdfEngine::Version_1_6:
            Q_UNREACHABLE(); // no output intent for these versions
            break;
        case QPdfEngine::Version_A1b:
            write("/S/GTS_PDFA1\n");
            break;
        case QPdfEngine::Version_X4:
            write("/S/GTS_PDFX\n");
            break;
        }

        xprintf("/DestOutputProfile %d 0 R\n", colorProfileEntry);
        write("/OutputConditionIdentifier ");
        printString(outputIntent.outputConditionIdentifier());
        write("\n");

        write("/Info ");
        printString(outputIntent.outputCondition());
        write("\n");

        write("/OutputCondition ");
        printString(outputIntent.outputCondition());
        write("\n");

        if (const auto registryName = outputIntent.registryName(); !registryName.isEmpty()) {
            write("/RegistryName ");
            printString(registryName.toString());
            write("\n");
        }

        write(">>\n");
        write("endobj\n");
    }

    return outputIntentEntry;
}

void QPdfEnginePrivate::writePageRoot()
{
    addXrefEntry(pageRoot);

    xprintf("<<\n"
            "/Type /Pages\n"
            "/Kids \n"
            "[\n");
    int size = pages.size();
    for (int i = 0; i < size; ++i)
        xprintf("%d 0 R\n", pages[i]);
    xprintf("]\n");

    //xprintf("/Group <</S /Transparency /I true /K false>>\n");
    xprintf("/Count %" PRIdQSIZETYPE "\n", pages.size());

    xprintf("/ProcSet [/PDF /Text /ImageB /ImageC]\n"
            ">>\n"
            "endobj\n");
}

void QPdfEnginePrivate::writeDestsRoot()
{
    if (destCache.isEmpty())
        return;

    std::map<QString, int> destObjects;
    QByteArray xs, ys;
    for (const DestInfo &destInfo : std::as_const(destCache)) {
        int destObj = addXrefEntry(-1);
        xs.setNum(static_cast<double>(destInfo.coords.x()), 'f');
        ys.setNum(static_cast<double>(destInfo.coords.y()), 'f');
        xprintf("[%d 0 R /XYZ %s %s 0]\n", destInfo.pageObj, xs.constData(), ys.constData());
        xprintf("endobj\n");
        destObjects.insert_or_assign(destInfo.anchor, destObj);
    }

    // names
    destsRoot = addXrefEntry(-1);
    xprintf("<<\n/Limits [");
    printString(destObjects.begin()->first);
    xprintf(" ");
    printString(destObjects.rbegin()->first);
    xprintf("]\n/Names [\n");
    for (const auto &[anchor, destObject] : destObjects) {
        printString(anchor);
        xprintf(" %d 0 R\n", destObject);
    }
    xprintf("]\n>>\n"
            "endobj\n");
}

void QPdfEnginePrivate::writeAttachmentRoot()
{
    if (fileCache.isEmpty())
        return;

    QList<int> attachments;
    const int size = fileCache.size();
    for (int i = 0; i < size; ++i) {
        auto attachment = fileCache.at(i);
        const int attachmentID = addXrefEntry(-1);
        xprintf("<<\n");
        if (do_compress)
            xprintf("/Filter /FlateDecode\n");

        const int lenobj = requestObject();
        xprintf("/Length %d 0 R\n", lenobj);
        int len = 0;
        xprintf(">>\nstream\n");
        len = writeCompressed(attachment.data);
        xprintf("\nendstream\n"
                "endobj\n");
        addXrefEntry(lenobj);
        xprintf("%d\n"
                "endobj\n", len);

        attachments.push_back(addXrefEntry(-1));
        xprintf("<<\n"
                "/F ");
        printString(attachment.fileName);

        xprintf("\n/EF <</F %d 0 R>>\n"
                "/Type/Filespec\n"
                 , attachmentID);
        if (!attachment.mimeType.isEmpty())
            xprintf("/Subtype/%s\n",
                    attachment.mimeType.replace("/"_L1, "#2F"_L1).toLatin1().constData());
        xprintf(">>\nendobj\n");
    }

    // names
    attachmentsRoot = addXrefEntry(-1);
    xprintf("<</Names[");
    for (int i = 0; i < size; ++i) {
        auto attachment = fileCache.at(i);
        printString(attachment.fileName);
        xprintf("%d 0 R\n", attachments.at(i));
    }
    xprintf("]>>\n"
            "endobj\n");
}

void QPdfEnginePrivate::writeNamesRoot()
{
    addXrefEntry(namesRoot);
    xprintf("<<\n");

    if (attachmentsRoot)
        xprintf("/EmbeddedFiles %d 0 R\n", attachmentsRoot);

    if (destsRoot)
        xprintf("/Dests %d 0 R\n", destsRoot);

    xprintf(">>\n");
    xprintf("endobj\n");
}

void QPdfEnginePrivate::embedFont(QFontSubset *font)
{
    //qDebug() << "embedFont" << font->object_id;
    int fontObject = font->object_id;
    QByteArray fontData = font->toTruetype();
#ifdef FONT_DUMP
    static int i = 0;
    QString fileName("font%1.ttf");
    fileName = fileName.arg(i++);
    QFile ff(fileName);
    ff.open(QFile::WriteOnly);
    ff.write(fontData);
    ff.close();
#endif

    int fontDescriptor = requestObject();
    int fontstream = requestObject();
    int cidfont = requestObject();
    int toUnicode = requestObject();
    int cidset = requestObject();

    QFontEngine::Properties properties = font->fontEngine->properties();
    QByteArray postscriptName = properties.postscriptName.replace(' ', '_');

    {
        qreal scale = 1000/properties.emSquare.toReal();
        addXrefEntry(fontDescriptor);
        QByteArray descriptor;
        QPdf::ByteStream s(&descriptor);
        s << "<< /Type /FontDescriptor\n"
            "/FontName /Q";
        int tag = fontDescriptor;
        for (int i = 0; i < 5; ++i) {
            s << (char)('A' + (tag % 26));
            tag /= 26;
        }
        s <<  '+' << postscriptName << "\n"
            "/Flags " << 4 << "\n"
            "/FontBBox ["
          << properties.boundingBox.x()*scale
          << -(properties.boundingBox.y() + properties.boundingBox.height())*scale
          << (properties.boundingBox.x() + properties.boundingBox.width())*scale
          << -properties.boundingBox.y()*scale  << "]\n"
            "/ItalicAngle " << properties.italicAngle.toReal() << "\n"
            "/Ascent " << properties.ascent.toReal()*scale << "\n"
            "/Descent " << -properties.descent.toReal()*scale << "\n"
            "/CapHeight " << properties.capHeight.toReal()*scale << "\n"
            "/StemV " << properties.lineWidth.toReal()*scale << "\n"
            "/FontFile2 " << fontstream << "0 R\n"
            "/CIDSet " << cidset << "0 R\n"
            ">>\nendobj\n";
        write(descriptor);
    }
    {
        addXrefEntry(fontstream);
        QByteArray header;
        QPdf::ByteStream s(&header);

        int length_object = requestObject();
        s << "<<\n"
            "/Length1 " << fontData.size() << "\n"
            "/Length " << length_object << "0 R\n";
        if (do_compress)
            s << "/Filter /FlateDecode\n";
        s << ">>\n"
            "stream\n";
        write(header);
        int len = writeCompressed(fontData);
        write("\nendstream\n"
              "endobj\n");
        addXrefEntry(length_object);
        xprintf("%d\n"
                "endobj\n", len);
    }
    {
        addXrefEntry(cidfont);
        QByteArray cid;
        QPdf::ByteStream s(&cid);
        s << "<< /Type /Font\n"
            "/Subtype /CIDFontType2\n"
            "/BaseFont /" << postscriptName << "\n"
            "/CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >>\n"
            "/FontDescriptor " << fontDescriptor << "0 R\n"
            "/CIDToGIDMap /Identity\n"
          << font->widthArray() <<
            ">>\n"
            "endobj\n";
        write(cid);
    }
    {
        addXrefEntry(toUnicode);
        QByteArray touc = font->createToUnicodeMap();
        xprintf("<< /Length %" PRIdQSIZETYPE " >>\n"
                "stream\n", touc.size());
        write(touc);
        write("\nendstream\n"
              "endobj\n");
    }
    {
        addXrefEntry(fontObject);
        QByteArray font;
        QPdf::ByteStream s(&font);
        s << "<< /Type /Font\n"
            "/Subtype /Type0\n"
            "/BaseFont /" << postscriptName << "\n"
            "/Encoding /Identity-H\n"
            "/DescendantFonts [" << cidfont << "0 R]\n"
            "/ToUnicode " << toUnicode << "0 R"
            ">>\n"
            "endobj\n";
        write(font);
    }
    {
        QByteArray cidSetStream(font->nGlyphs() / 8 + 1, 0);
        int byteCounter = 0;
        int bitCounter = 0;
        for (qsizetype i = 0; i < font->nGlyphs(); ++i) {
            cidSetStream.data()[byteCounter] |= (1 << (7 - bitCounter));

            bitCounter++;
            if (bitCounter == 8) {
                bitCounter = 0;
                byteCounter++;
            }
        }

        addXrefEntry(cidset);
        xprintf("<<\n");
        xprintf("/Length %" PRIdQSIZETYPE "\n", cidSetStream.size());
        xprintf(">>\n");
        xprintf("stream\n");
        write(cidSetStream);
        xprintf("\nendstream\n");
        xprintf("endobj\n");
    }
}

qreal QPdfEnginePrivate::calcUserUnit() const
{
    // PDF standards < 1.6 support max 200x200in pages (no UserUnit)
    if (pdfVersion < QPdfEngine::Version_1_6)
        return 1.0;

    const int maxLen = qMax(currentPage->pageSize.width(), currentPage->pageSize.height());
    if (maxLen <= 14400)
        return 1.0; // for pages up to 200x200in (14400x14400 units) use default scaling

    // for larger pages, rescale units so we can have up to 381x381km
    return qMin(maxLen / 14400.0, 75000.0);
}

void QPdfEnginePrivate::writeFonts()
{
    for (QHash<QFontEngine::FaceId, QFontSubset *>::iterator it = fonts.begin(); it != fonts.end(); ++it) {
        embedFont(*it);
        delete *it;
    }
    fonts.clear();
}

void QPdfEnginePrivate::writePage()
{
    if (pages.empty())
        return;

    *currentPage << "Q Q\n";

    uint pageStream = requestObject();
    uint pageStreamLength = requestObject();
    uint resources = requestObject();
    uint annots = requestObject();

    qreal userUnit = calcUserUnit();

    addXrefEntry(pages.constLast());

    // make sure we use the pagesize from when we started the page, since the user may have changed it
    const QByteArray formattedPageWidth = QByteArray::number(currentPage->pageSize.width() / userUnit, 'f');
    const QByteArray formattedPageHeight = QByteArray::number(currentPage->pageSize.height() / userUnit, 'f');

    xprintf("<<\n"
            "/Type /Page\n"
            "/Parent %d 0 R\n"
            "/Contents %d 0 R\n"
            "/Resources %d 0 R\n"
            "/Annots %d 0 R\n"
            "/MediaBox [0 0 %s %s]\n"
            "/TrimBox [0 0 %s %s]\n",
            pageRoot, pageStream, resources, annots,
            formattedPageWidth.constData(),
            formattedPageHeight.constData(),
            formattedPageWidth.constData(),
            formattedPageHeight.constData());

    if (pdfVersion >= QPdfEngine::Version_1_6)
        xprintf("/UserUnit %s\n", QByteArray::number(userUnit, 'f').constData());

    xprintf(">>\n"
            "endobj\n");

    addXrefEntry(resources);
    xprintf("<<\n"
            "/ColorSpace <<\n"
            "/PCSp %d 0 R\n"
            "/PCSpg %d 0 R\n"
            "/PCSpcmyk %d 0 R\n"
            "/CSp /DeviceRGB\n"
            "/CSpg /DeviceGray\n"
            "/CSpcmyk /DeviceCMYK\n"
            ">>\n"
            "/ExtGState <<\n"
            "/GSa %d 0 R\n",
            patternColorSpaceRGB,
            patternColorSpaceGrayscale,
            patternColorSpaceCMYK,
            graphicsState);

    for (int i = 0; i < currentPage->graphicStates.size(); ++i)
        xprintf("/GState%d %d 0 R\n", currentPage->graphicStates.at(i), currentPage->graphicStates.at(i));
    xprintf(">>\n");

    xprintf("/Pattern <<\n");
    for (int i = 0; i < currentPage->patterns.size(); ++i)
        xprintf("/Pat%d %d 0 R\n", currentPage->patterns.at(i), currentPage->patterns.at(i));
    xprintf(">>\n");

    xprintf("/Font <<\n");
    for (int i = 0; i < currentPage->fonts.size();++i)
        xprintf("/F%d %d 0 R\n", currentPage->fonts[i], currentPage->fonts[i]);
    xprintf(">>\n");

    xprintf("/XObject <<\n");
    for (int i = 0; i<currentPage->images.size(); ++i) {
        xprintf("/Im%d %d 0 R\n", currentPage->images.at(i), currentPage->images.at(i));
    }
    xprintf(">>\n");

    xprintf(">>\n"
            "endobj\n");

    addXrefEntry(annots);
    xprintf("[ ");
    for (int i = 0; i<currentPage->annotations.size(); ++i) {
        xprintf("%d 0 R ", currentPage->annotations.at(i));
    }
    xprintf("]\nendobj\n");

    addXrefEntry(pageStream);
    xprintf("<<\n"
            "/Length %d 0 R\n", pageStreamLength); // object number for stream length object
    if (do_compress)
        xprintf("/Filter /FlateDecode\n");

    xprintf(">>\n");
    xprintf("stream\n");
    QIODevice *content = currentPage->stream();
    int len = writeCompressed(content);
    xprintf("\nendstream\n"
            "endobj\n");

    addXrefEntry(pageStreamLength);
    xprintf("%d\nendobj\n",len);
}

void QPdfEnginePrivate::writeTail()
{
    writePage();
    writeFonts();
    writePageRoot();
    writeDestsRoot();
    writeAttachmentRoot();
    writeNamesRoot();

    addXrefEntry(xrefPositions.size(),false);
    xprintf("xref\n"
            "0 %" PRIdQSIZETYPE "\n"
            "%010d 65535 f \n", xrefPositions.size()-1, xrefPositions[0]);

    for (int i = 1; i < xrefPositions.size()-1; ++i)
        xprintf("%010d 00000 n \n", xrefPositions[i]);

    {
        QByteArray trailer;
        QPdf::ByteStream s(&trailer);

        s << "trailer\n"
          << "<<\n"
          << "/Size " << xrefPositions.size() - 1 << "\n"
          << "/Info " << info << "0 R\n"
          << "/Root " << catalog << "0 R\n";

        const QByteArray id = documentId.toString(QUuid::WithoutBraces).toUtf8().toHex();
        s << "/ID [ <" << id << "> <" << id << "> ]\n";

        s << ">>\n"
          << "startxref\n" << xrefPositions.constLast() << "\n"
          << "%%EOF\n";

        write(trailer);
    }
}

int QPdfEnginePrivate::addXrefEntry(int object, bool printostr)
{
    if (object < 0)
        object = requestObject();

    if (object>=xrefPositions.size())
        xrefPositions.resize(object+1);

    xrefPositions[object] = streampos;
    if (printostr)
        xprintf("%d 0 obj\n",object);

    return object;
}

void QPdfEnginePrivate::printString(QStringView string)
{
    if (string.isEmpty()) {
        write("()");
        return;
    }

    // The 'text string' type in PDF is encoded either as PDFDocEncoding, or
    // Unicode UTF-16 with a Unicode byte order mark as the first character
    // (0xfeff), with the high-order byte first.
    QByteArray array("(\xfe\xff");
    const char16_t *utf16 = string.utf16();

    for (qsizetype i = 0; i < string.size(); ++i) {
        char part[2] = {char((*(utf16 + i)) >> 8), char((*(utf16 + i)) & 0xff)};
        for(int j=0; j < 2; ++j) {
            if (part[j] == '(' || part[j] == ')' || part[j] == '\\')
                array.append('\\');
            array.append(part[j]);
        }
    }
    array.append(')');
    write(array);
}


void QPdfEnginePrivate::xprintf(const char* fmt, ...)
{
    if (!stream)
        return;

    const int msize = 10000;
    char buf[msize];

    va_list args;
    va_start(args, fmt);
    int bufsize = std::vsnprintf(buf, msize, fmt, args);
    va_end(args);

    if (Q_LIKELY(bufsize < msize)) {
        stream->writeRawData(buf, bufsize);
    } else {
        // Fallback for abnormal cases
        QScopedArrayPointer<char> tmpbuf(new char[bufsize + 1]);
        va_start(args, fmt);
        bufsize = std::vsnprintf(tmpbuf.data(), bufsize + 1, fmt, args);
        va_end(args);
        stream->writeRawData(tmpbuf.data(), bufsize);
    }
    streampos += bufsize;
}

int QPdfEnginePrivate::writeCompressed(QIODevice *dev)
{
#ifndef QT_NO_COMPRESS
    if (do_compress) {
        int size = QPdfPage::chunkSize();
        int sum = 0;
        ::z_stream zStruct;
        zStruct.zalloc = Z_NULL;
        zStruct.zfree = Z_NULL;
        zStruct.opaque = Z_NULL;
        if (::deflateInit(&zStruct, Z_DEFAULT_COMPRESSION) != Z_OK) {
            qWarning("QPdfStream::writeCompressed: Error in deflateInit()");
            return sum;
        }
        zStruct.avail_in = 0;
        QByteArray in, out;
        out.resize(size);
        while (!dev->atEnd() || zStruct.avail_in != 0) {
            if (zStruct.avail_in == 0) {
                in = dev->read(size);
                zStruct.avail_in = in.size();
                zStruct.next_in = reinterpret_cast<unsigned char*>(in.data());
                if (in.size() <= 0) {
                    qWarning("QPdfStream::writeCompressed: Error in read()");
                    ::deflateEnd(&zStruct);
                    return sum;
                }
            }
            zStruct.next_out = reinterpret_cast<unsigned char*>(out.data());
            zStruct.avail_out = out.size();
            if (::deflate(&zStruct, 0) != Z_OK) {
                qWarning("QPdfStream::writeCompressed: Error in deflate()");
                ::deflateEnd(&zStruct);
                return sum;
            }
            int written = out.size() - zStruct.avail_out;
            stream->writeRawData(out.constData(), written);
            streampos += written;
            sum += written;
        }
        int ret;
        do {
            zStruct.next_out = reinterpret_cast<unsigned char*>(out.data());
            zStruct.avail_out = out.size();
            ret = ::deflate(&zStruct, Z_FINISH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                qWarning("QPdfStream::writeCompressed: Error in deflate()");
                ::deflateEnd(&zStruct);
                return sum;
            }
            int written = out.size() - zStruct.avail_out;
            stream->writeRawData(out.constData(), written);
            streampos += written;
            sum += written;
        } while (ret == Z_OK);

        ::deflateEnd(&zStruct);

        return sum;
    } else
#endif
    {
        QByteArray arr;
        int sum = 0;
        while (!dev->atEnd()) {
            arr = dev->read(QPdfPage::chunkSize());
            stream->writeRawData(arr.constData(), arr.size());
            streampos += arr.size();
            sum += arr.size();
        }
        return sum;
    }
}

int QPdfEnginePrivate::writeCompressed(const char *src, int len)
{
#ifndef QT_NO_COMPRESS
    if (do_compress) {
        const QByteArray data = qCompress(reinterpret_cast<const uchar *>(src), len);
        constexpr qsizetype HeaderSize = 4;
        if (!data.isNull()) {
            stream->writeRawData(data.data() + HeaderSize, data.size() - HeaderSize);
            len = data.size() - HeaderSize;
        } else {
            qWarning("QPdfStream::writeCompressed: Error in compress()");
            len = 0;
        }
    } else
#endif
    {
        stream->writeRawData(src,len);
    }
    streampos += len;
    return len;
}

int QPdfEnginePrivate::writeImage(const QByteArray &data, int width, int height, WriteImageOption option,
                                  int maskObject, int softMaskObject, bool dct, bool isMono)
{
    int image = addXrefEntry(-1);
    xprintf("<<\n"
            "/Type /XObject\n"
            "/Subtype /Image\n"
            "/Width %d\n"
            "/Height %d\n", width, height);

    switch (option) {
    case WriteImageOption::Monochrome:
        if (!isMono) {
            xprintf("/ImageMask true\n"
                    "/Decode [1 0]\n");
        } else {
            xprintf("/BitsPerComponent 1\n"
                    "/ColorSpace /DeviceGray\n");
        }
        break;
    case WriteImageOption::Grayscale:
        xprintf("/BitsPerComponent 8\n"
                "/ColorSpace /DeviceGray\n");
        break;
    case WriteImageOption::RGB:
        xprintf("/BitsPerComponent 8\n"
                "/ColorSpace /DeviceRGB\n");
        break;
    case WriteImageOption::CMYK:
        xprintf("/BitsPerComponent 8\n"
                "/ColorSpace /DeviceCMYK\n");
        break;
    }

    if (maskObject > 0)
        xprintf("/Mask %d 0 R\n", maskObject);
    if (softMaskObject > 0)
        xprintf("/SMask %d 0 R\n", softMaskObject);

    int lenobj = requestObject();
    xprintf("/Length %d 0 R\n", lenobj);
    if (interpolateImages)
        xprintf("/Interpolate true\n");
    int len = 0;
    if (dct) {
        //qDebug("DCT");
        xprintf("/Filter /DCTDecode\n>>\nstream\n");
        write(data);
        len = data.size();
    } else {
        if (do_compress)
            xprintf("/Filter /FlateDecode\n>>\nstream\n");
        else
            xprintf(">>\nstream\n");
        len = writeCompressed(data);
    }
    xprintf("\nendstream\n"
            "endobj\n");
    addXrefEntry(lenobj);
    xprintf("%d\n"
            "endobj\n", len);
    return image;
}

struct QGradientBound {
    qreal start;
    qreal stop;
    int function;
    bool reverse;
};
Q_DECLARE_TYPEINFO(QGradientBound, Q_PRIMITIVE_TYPE);

void QPdfEnginePrivate::ShadingFunctionResult::writeColorSpace(QPdf::ByteStream *stream) const
{
    *stream << "/ColorSpace ";
    switch (colorModel) {
    case QPdfEngine::ColorModel::RGB:
        *stream << "/DeviceRGB\n"; break;
    case QPdfEngine::ColorModel::Grayscale:
        *stream << "/DeviceGray\n"; break;
    case QPdfEngine::ColorModel::CMYK:
        *stream << "/DeviceCMYK\n"; break;
    case QPdfEngine::ColorModel::Auto:
        Q_UNREACHABLE(); break;
    }
}

QPdfEnginePrivate::ShadingFunctionResult
QPdfEnginePrivate::createShadingFunction(const QGradient *gradient, int from, int to, bool reflect, bool alpha)
{
    QGradientStops stops = gradient->stops();
    if (stops.isEmpty()) {
        stops << QGradientStop(0, Qt::black);
        stops << QGradientStop(1, Qt::white);
    }
    if (stops.at(0).first > 0)
        stops.prepend(QGradientStop(0, stops.at(0).second));
    if (stops.at(stops.size() - 1).first < 1)
        stops.append(QGradientStop(1, stops.at(stops.size() - 1).second));

    // Color to use which colorspace to use
    const QColor referenceColor = stops.constFirst().second;

    switch (colorModel) {
    case QPdfEngine::ColorModel::RGB:
    case QPdfEngine::ColorModel::Grayscale:
    case QPdfEngine::ColorModel::CMYK:
        break;
    case QPdfEngine::ColorModel::Auto: {
        // Make sure that all the stops have the same color spec
        // (we don't support anything else)
        const QColor::Spec referenceSpec = referenceColor.spec();
        bool warned = false;
        for (QGradientStop &stop : stops) {
            if (stop.second.spec() != referenceSpec) {
                if (!warned) {
                    qWarning("QPdfEngine: unable to create a gradient between colors of different spec");
                    warned = true;
                }
                stop.second = stop.second.convertTo(referenceSpec);
            }
        }
        break;
    }
    }

    ShadingFunctionResult result;
    result.colorModel = colorModelForColor(referenceColor);

    QList<int> functions;
    const int numStops = stops.size();
    functions.reserve(numStops - 1);
    for (int i = 0; i < numStops - 1; ++i) {
        int f = addXrefEntry(-1);
        QByteArray data;
        QPdf::ByteStream s(&data);
        s << "<<\n"
             "/FunctionType 2\n"
             "/Domain [0 1]\n"
             "/N 1\n";
        if (alpha) {
            s << "/C0 [" << stops.at(i).second.alphaF() << "]\n"
                 "/C1 [" << stops.at(i + 1).second.alphaF() << "]\n";
        } else {
            switch (result.colorModel) {
            case QPdfEngine::ColorModel::RGB:
                s << "/C0 [" << stops.at(i).second.redF() << stops.at(i).second.greenF() <<  stops.at(i).second.blueF() << "]\n"
                     "/C1 [" << stops.at(i + 1).second.redF() << stops.at(i + 1).second.greenF() <<  stops.at(i + 1).second.blueF() << "]\n";
                break;
            case QPdfEngine::ColorModel::Grayscale: {
                constexpr qreal normalisationFactor = 1. / 255.;
                s << "/C0 [" << (qGray(stops.at(i).second.rgba()) * normalisationFactor) << "]\n"
                     "/C1 [" << (qGray(stops.at(i + 1).second.rgba()) * normalisationFactor) << "]\n";
                break;
            }
            case QPdfEngine::ColorModel::CMYK:
                s << "/C0 [" << stops.at(i).second.cyanF()
                             << stops.at(i).second.magentaF()
                             << stops.at(i).second.yellowF()
                             << stops.at(i).second.blackF() << "]\n"
                     "/C1 [" << stops.at(i + 1).second.cyanF()
                             << stops.at(i + 1).second.magentaF()
                             << stops.at(i + 1).second.yellowF()
                             << stops.at(i + 1).second.blackF() << "]\n";
                break;

            case QPdfEngine::ColorModel::Auto:
                Q_UNREACHABLE();
                break;
            }

        }
        s << ">>\n"
             "endobj\n";
        write(data);
        functions << f;
    }

    QList<QGradientBound> gradientBounds;
    gradientBounds.reserve((to - from) * (numStops - 1));

    for (int step = from; step < to; ++step) {
        if (reflect && step % 2) {
            for (int i = numStops - 1; i > 0; --i) {
                QGradientBound b;
                b.start = step + 1 - qBound(qreal(0.), stops.at(i).first, qreal(1.));
                b.stop = step + 1 - qBound(qreal(0.), stops.at(i - 1).first, qreal(1.));
                b.function = functions.at(i - 1);
                b.reverse = true;
                gradientBounds << b;
            }
        } else {
            for (int i = 0; i < numStops - 1; ++i) {
                QGradientBound b;
                b.start = step + qBound(qreal(0.), stops.at(i).first, qreal(1.));
                b.stop = step + qBound(qreal(0.), stops.at(i + 1).first, qreal(1.));
                b.function = functions.at(i);
                b.reverse = false;
                gradientBounds << b;
            }
        }
    }

    // normalize bounds to [0..1]
    qreal bstart = gradientBounds.at(0).start;
    qreal bend = gradientBounds.at(gradientBounds.size() - 1).stop;
    qreal norm = 1./(bend - bstart);
    for (int i = 0; i < gradientBounds.size(); ++i) {
        gradientBounds[i].start = (gradientBounds[i].start - bstart)*norm;
        gradientBounds[i].stop = (gradientBounds[i].stop - bstart)*norm;
    }

    int function;
    if (gradientBounds.size() > 1) {
        function = addXrefEntry(-1);
        QByteArray data;
        QPdf::ByteStream s(&data);
        s << "<<\n"
             "/FunctionType 3\n"
             "/Domain [0 1]\n"
             "/Bounds [";
        for (int i = 1; i < gradientBounds.size(); ++i)
            s << gradientBounds.at(i).start;
        s << "]\n"
             "/Encode [";
        for (int i = 0; i < gradientBounds.size(); ++i)
            s << (gradientBounds.at(i).reverse ? "1 0 " : "0 1 ");
        s << "]\n"
             "/Functions [";
        for (int i = 0; i < gradientBounds.size(); ++i)
            s << gradientBounds.at(i).function << "0 R ";
        s << "]\n"
             ">>\n"
             "endobj\n";
        write(data);
    } else {
        function = functions.at(0);
    }
    result.function = function;
    return result;
}

int QPdfEnginePrivate::generateLinearGradientShader(const QLinearGradient *gradient, const QTransform &matrix, bool alpha)
{
    QPointF start = gradient->start();
    QPointF stop = gradient->finalStop();
    QPointF offset = stop - start;
    Q_ASSERT(gradient->coordinateMode() == QGradient::LogicalMode);

    int from = 0;
    int to = 1;
    bool reflect = false;
    switch (gradient->spread()) {
    case QGradient::PadSpread:
        break;
    case QGradient::ReflectSpread:
        reflect = true;
        Q_FALLTHROUGH();
    case QGradient::RepeatSpread: {
        // calculate required bounds
        QRectF pageRect = m_pageLayout.fullRectPixels(resolution);
        QTransform inv = matrix.inverted();
        QPointF page_rect[4] = { inv.map(pageRect.topLeft()),
                                 inv.map(pageRect.topRight()),
                                 inv.map(pageRect.bottomLeft()),
                                 inv.map(pageRect.bottomRight()) };

        qreal length = offset.x()*offset.x() + offset.y()*offset.y();

        // find the max and min values in offset and orth direction that are needed to cover
        // the whole page
        from = INT_MAX;
        to = INT_MIN;
        for (int i = 0; i < 4; ++i) {
            qreal off = ((page_rect[i].x() - start.x()) * offset.x() + (page_rect[i].y() - start.y()) * offset.y())/length;
            from = qMin(from, qFloor(off));
            to = qMax(to, qCeil(off));
        }

        stop = start + to * offset;
        start = start + from * offset;\
        break;
    }
    }

    const auto shadingFunctionResult = createShadingFunction(gradient, from, to, reflect, alpha);

    QByteArray shader;
    QPdf::ByteStream s(&shader);
    s << "<<\n"
         "/ShadingType 2\n";

    if (alpha)
        s << "/ColorSpace /DeviceGray\n";
    else
        shadingFunctionResult.writeColorSpace(&s);

    s << "/AntiAlias true\n"
        "/Coords [" << start.x() << start.y() << stop.x() << stop.y() << "]\n"
        "/Extend [true true]\n"
        "/Function " << shadingFunctionResult.function << "0 R\n"
        ">>\n"
        "endobj\n";
    int shaderObject = addXrefEntry(-1);
    write(shader);
    return shaderObject;
}

int QPdfEnginePrivate::generateRadialGradientShader(const QRadialGradient *gradient, const QTransform &matrix, bool alpha)
{
    QPointF p1 = gradient->center();
    qreal r1 = gradient->centerRadius();
    QPointF p0 = gradient->focalPoint();
    qreal r0 = gradient->focalRadius();

    Q_ASSERT(gradient->coordinateMode() == QGradient::LogicalMode);

    int from = 0;
    int to = 1;
    bool reflect = false;
    switch (gradient->spread()) {
    case QGradient::PadSpread:
        break;
    case QGradient::ReflectSpread:
        reflect = true;
        Q_FALLTHROUGH();
    case QGradient::RepeatSpread: {
        Q_ASSERT(qFuzzyIsNull(r0)); // QPainter emulates if this is not 0

        QRectF pageRect = m_pageLayout.fullRectPixels(resolution);
        QTransform inv = matrix.inverted();
        QPointF page_rect[4] = { inv.map(pageRect.topLeft()),
                                 inv.map(pageRect.topRight()),
                                 inv.map(pageRect.bottomLeft()),
                                 inv.map(pageRect.bottomRight()) };

        // increase to until the whole page fits into it
        bool done = false;
        while (!done) {
            QPointF center = QPointF(p0.x() + to*(p1.x() - p0.x()), p0.y() + to*(p1.y() - p0.y()));
            double radius = r0 + to*(r1 - r0);
            double r2 = radius*radius;
            done = true;
            for (int i = 0; i < 4; ++i) {
                QPointF off = page_rect[i] - center;
                if (off.x()*off.x() + off.y()*off.y() > r2) {
                    ++to;
                    done = false;
                    break;
                }
            }
        }
        p1 = QPointF(p0.x() + to*(p1.x() - p0.x()), p0.y() + to*(p1.y() - p0.y()));
        r1 = r0 + to*(r1 - r0);
        break;
    }
    }

    const auto shadingFunctionResult = createShadingFunction(gradient, from, to, reflect, alpha);

    QByteArray shader;
    QPdf::ByteStream s(&shader);
    s << "<<\n"
         "/ShadingType 3\n";

    if (alpha)
        s << "/ColorSpace /DeviceGray\n";
    else
        shadingFunctionResult.writeColorSpace(&s);

    s << "/AntiAlias true\n"
        "/Domain [0 1]\n"
        "/Coords [" << p0.x() << p0.y() << r0 << p1.x() << p1.y() << r1 << "]\n"
        "/Extend [true true]\n"
        "/Function " << shadingFunctionResult.function << "0 R\n"
        ">>\n"
        "endobj\n";
    int shaderObject = addXrefEntry(-1);
    write(shader);
    return shaderObject;
}

int QPdfEnginePrivate::generateGradientShader(const QGradient *gradient, const QTransform &matrix, bool alpha)
{
    switch (gradient->type()) {
    case QGradient::LinearGradient:
        return generateLinearGradientShader(static_cast<const QLinearGradient *>(gradient), matrix, alpha);
    case QGradient::RadialGradient:
        return generateRadialGradientShader(static_cast<const QRadialGradient *>(gradient), matrix, alpha);
    case QGradient::ConicalGradient:
        Q_UNIMPLEMENTED(); // ### Implement me!
        break;
    case QGradient::NoGradient:
        break;
    }
    return 0;
}

int QPdfEnginePrivate::gradientBrush(const QBrush &b, const QTransform &matrix, int *gStateObject)
{
    const QGradient *gradient = b.gradient();

    if (!gradient || gradient->coordinateMode() != QGradient::LogicalMode)
        return 0;

    QRectF pageRect = m_pageLayout.fullRectPixels(resolution);

    QTransform m = b.transform() * matrix;
    int shaderObject = generateGradientShader(gradient, m);

    QByteArray str;
    QPdf::ByteStream s(&str);
    s << "<<\n"
        "/Type /Pattern\n"
        "/PatternType 2\n"
        "/Shading " << shaderObject << "0 R\n"
        "/Matrix ["
      << m.m11()
      << m.m12()
      << m.m21()
      << m.m22()
      << m.dx()
      << m.dy() << "]\n";
    s << ">>\n"
        "endobj\n";

    int patternObj = addXrefEntry(-1);
    write(str);
    currentPage->patterns.append(patternObj);

    if (!b.isOpaque()) {
        bool ca = true;
        QGradientStops stops = gradient->stops();
        int a = stops.at(0).second.alpha();
        for (int i = 1; i < stops.size(); ++i) {
            if (stops.at(i).second.alpha() != a) {
                ca = false;
                break;
            }
        }
        if (ca) {
            *gStateObject = addConstantAlphaObject(stops.at(0).second.alpha());
        } else {
            int alphaShaderObject = generateGradientShader(gradient, m, true);

            QByteArray content;
            QPdf::ByteStream c(&content);
            c << "/Shader" << alphaShaderObject << "sh\n";

            QByteArray form;
            QPdf::ByteStream f(&form);
            f << "<<\n"
                "/Type /XObject\n"
                "/Subtype /Form\n"
                "/BBox [0 0 " << pageRect.width() << pageRect.height() << "]\n"
                "/Group <</S /Transparency >>\n"
                "/Resources <<\n"
                "/Shading << /Shader" << alphaShaderObject << alphaShaderObject << "0 R >>\n"
                ">>\n";

            f << "/Length " << content.size() << "\n"
                ">>\n"
                "stream\n"
              << content
              << "\nendstream\n"
                "endobj\n";

            int softMaskFormObject = addXrefEntry(-1);
            write(form);
            *gStateObject = addXrefEntry(-1);
            xprintf("<< /SMask << /S /Alpha /G %d 0 R >> >>\n"
                    "endobj\n", softMaskFormObject);
            currentPage->graphicStates.append(*gStateObject);
        }
    }

    return patternObj;
}

int QPdfEnginePrivate::addConstantAlphaObject(int brushAlpha, int penAlpha)
{
    if (brushAlpha == 255 && penAlpha == 255)
        return 0;
    uint object = alphaCache.value(std::pair<uint, uint>(brushAlpha, penAlpha), 0);
    if (!object) {
        object = addXrefEntry(-1);
        QByteArray alphaDef;
        QPdf::ByteStream s(&alphaDef);
        s << "<<\n/ca " << (brushAlpha/qreal(255.)) << '\n';
        s << "/CA " << (penAlpha/qreal(255.)) << "\n>>";
        xprintf("%s\nendobj\n", alphaDef.constData());
        alphaCache.insert(std::pair<uint, uint>(brushAlpha, penAlpha), object);
    }
    if (currentPage->graphicStates.indexOf(object) < 0)
        currentPage->graphicStates.append(object);

    return object;
}


int QPdfEnginePrivate::addBrushPattern(const QTransform &m, bool *specifyColor, int *gStateObject)
{
    Q_Q(QPdfEngine);

    int paintType = 2; // Uncolored tiling
    int w = 8;
    int h = 8;

    *specifyColor = true;
    *gStateObject = 0;

    const Qt::BrushStyle style = brush.style();
    const bool isCosmetic = style >= Qt::Dense1Pattern && style <= Qt::DiagCrossPattern
                            && !q->painter()->testRenderHint(QPainter::NonCosmeticBrushPatterns);
    QTransform matrix;
    if (!isCosmetic)
        matrix = m;
    matrix.translate(brushOrigin.x(), brushOrigin.y());
    matrix = matrix * pageMatrix();

    if (style == Qt::LinearGradientPattern || style == Qt::RadialGradientPattern) {// && style <= Qt::ConicalGradientPattern) {
        *specifyColor = false;
        return gradientBrush(brush, matrix, gStateObject);
    }

    if (!isCosmetic)
        matrix = brush.transform() * matrix;

    if ((!brush.isOpaque() && brush.style() < Qt::LinearGradientPattern) || opacity != 1.0)
        *gStateObject = addConstantAlphaObject(qRound(brush.color().alpha() * opacity),
                                               qRound(pen.color().alpha() * opacity));

    int imageObject = -1;
    QByteArray pattern = QPdf::patternForBrush(brush);
    if (pattern.isEmpty()) {
        if (brush.style() != Qt::TexturePattern)
            return 0;
        QImage image = brush.textureImage();
        bool bitmap = true;
        const bool lossless = q->painter()->testRenderHint(QPainter::LosslessImageRendering);
        imageObject = addImage(image, &bitmap, lossless, image.cacheKey());
        if (imageObject != -1) {
            QImage::Format f = image.format();
            if (f != QImage::Format_MonoLSB && f != QImage::Format_Mono) {
                paintType = 1; // Colored tiling
                *specifyColor = false;
            }
            w = image.width();
            h = image.height();
            QTransform m(w, 0, 0, -h, 0, h);
            QPdf::ByteStream s(&pattern);
            s << QPdf::generateMatrix(m);
            s << "/Im" << imageObject << " Do\n";
        }
    }

    QByteArray str;
    QPdf::ByteStream s(&str);
    s << "<<\n"
        "/Type /Pattern\n"
        "/PatternType 1\n"
        "/PaintType " << paintType << "\n"
        "/TilingType 1\n"
        "/BBox [0 0 " << w << h << "]\n"
        "/XStep " << w << "\n"
        "/YStep " << h << "\n"
        "/Matrix ["
      << matrix.m11()
      << matrix.m12()
      << matrix.m21()
      << matrix.m22()
      << matrix.dx()
      << matrix.dy() << "]\n"
        "/Resources \n<< "; // open resource tree
    if (imageObject > 0) {
        s << "/XObject << /Im" << imageObject << ' ' << imageObject << "0 R >> ";
    }
    s << ">>\n"
        "/Length " << pattern.size() << "\n"
        ">>\n"
        "stream\n"
      << pattern
      << "\nendstream\n"
        "endobj\n";

    int patternObj = addXrefEntry(-1);
    write(str);
    currentPage->patterns.append(patternObj);
    return patternObj;
}

static inline bool is_monochrome(const QList<QRgb> &colorTable)
{
    return colorTable.size() == 2
        && colorTable.at(0) == QColor(Qt::black).rgba()
        && colorTable.at(1) == QColor(Qt::white).rgba()
        ;
}

/*!
 * Adds an image to the pdf and return the pdf-object id. Returns -1 if adding the image failed.
 */
int QPdfEnginePrivate::addImage(const QImage &img, bool *bitmap, bool lossless, qint64 serial_no)
{
    if (img.isNull())
        return -1;

    int object = imageCache.value(serial_no);
    if (object)
        return object;

    QImage image = img;
    QImage::Format format = image.format();
    const bool grayscale = (colorModel == QPdfEngine::ColorModel::Grayscale);

    if (pdfVersion == QPdfEngine::Version_A1b) {
        if (image.hasAlphaChannel()) {
            // transparent images are not allowed in PDF/A-1b, so we convert it to
            // a format without alpha channel first

            QImage alphaLessImage(image.width(), image.height(), QImage::Format_RGB32);
            alphaLessImage.fill(Qt::white);

            QPainter p(&alphaLessImage);
            p.drawImage(0, 0, image);

            image = alphaLessImage;
            format = image.format();
        }
    }

    if (image.depth() == 1 && *bitmap && is_monochrome(img.colorTable())) {
        if (format == QImage::Format_MonoLSB)
            image = image.convertToFormat(QImage::Format_Mono);
        format = QImage::Format_Mono;
    } else {
        *bitmap = false;
        if (format != QImage::Format_RGB32 && format != QImage::Format_ARGB32 && format != QImage::Format_CMYK8888) {
            image = image.convertToFormat(QImage::Format_ARGB32);
            format = QImage::Format_ARGB32;
        }
    }

    int w = image.width();
    int h = image.height();

    if (format == QImage::Format_Mono) {
        int bytesPerLine = (w + 7) >> 3;
        QByteArray data;
        data.resize(bytesPerLine * h);
        char *rawdata = data.data();
        for (int y = 0; y < h; ++y) {
            memcpy(rawdata, image.constScanLine(y), bytesPerLine);
            rawdata += bytesPerLine;
        }
        object = writeImage(data, w, h, WriteImageOption::Monochrome, 0, 0, false, is_monochrome(img.colorTable()));
    } else {
        QByteArray softMaskData;
        bool dct = false;
        QByteArray imageData;
        bool hasAlpha = false;
        bool hasMask = false;

        if (QImageWriter::supportedImageFormats().contains("jpeg") && !grayscale && !lossless) {
            QBuffer buffer(&imageData);
            QImageWriter writer(&buffer, "jpeg");
            writer.setQuality(94);
            if (format == QImage::Format_CMYK8888) {
                // PDFs require CMYK colors not to be inverted in the JPEG encoding
                writer.setSubType("CMYK");
            }
            writer.write(image);
            dct = true;

            if (format != QImage::Format_RGB32 && format != QImage::Format_CMYK8888) {
                softMaskData.resize(w * h);
                uchar *sdata = (uchar *)softMaskData.data();
                for (int y = 0; y < h; ++y) {
                    const QRgb *rgb = (const QRgb *)image.constScanLine(y);
                    for (int x = 0; x < w; ++x) {
                        uchar alpha = qAlpha(*rgb);
                        *sdata++ = alpha;
                        hasMask |= (alpha < 255);
                        hasAlpha |= (alpha != 0 && alpha != 255);
                        ++rgb;
                    }
                }
            }
        } else {
            if (format == QImage::Format_CMYK8888) {
                imageData.resize(grayscale ? w * h : w * h * 4);
                uchar *data = (uchar *)imageData.data();
                const qsizetype bytesPerLine = image.bytesPerLine();
                if (grayscale) {
                    for (int y = 0; y < h; ++y) {
                        const uint *cmyk = (const uint *)image.constScanLine(y);
                        for (int x = 0; x < w; ++x)
                            *data++ = qGray(QCmyk32::fromCmyk32(*cmyk++).toColor().rgba());
                    }
                } else {
                    for (int y = 0; y < h; ++y) {
                        uchar *start = data + y * w * 4;
                        memcpy(start, image.constScanLine(y), bytesPerLine);
                    }
                }
            } else {
                imageData.resize(grayscale ? w * h : 3 * w * h);
                uchar *data = (uchar *)imageData.data();
                softMaskData.resize(w * h);
                uchar *sdata = (uchar *)softMaskData.data();
                for (int y = 0; y < h; ++y) {
                    const QRgb *rgb = (const QRgb *)image.constScanLine(y);
                    if (grayscale) {
                        for (int x = 0; x < w; ++x) {
                            *(data++) = qGray(*rgb);
                            uchar alpha = qAlpha(*rgb);
                            *sdata++ = alpha;
                            hasMask |= (alpha < 255);
                            hasAlpha |= (alpha != 0 && alpha != 255);
                            ++rgb;
                        }
                    } else {
                        for (int x = 0; x < w; ++x) {
                            *(data++) = qRed(*rgb);
                            *(data++) = qGreen(*rgb);
                            *(data++) = qBlue(*rgb);
                            uchar alpha = qAlpha(*rgb);
                            *sdata++ = alpha;
                            hasMask |= (alpha < 255);
                            hasAlpha |= (alpha != 0 && alpha != 255);
                            ++rgb;
                        }
                    }
                }
            }
            if (format == QImage::Format_RGB32 || format == QImage::Format_CMYK8888)
                hasAlpha = hasMask = false;
        }
        int maskObject = 0;
        int softMaskObject = 0;
        if (hasAlpha) {
            softMaskObject = writeImage(softMaskData, w, h, WriteImageOption::Grayscale, 0, 0);
        } else if (hasMask) {
            // dither the soft mask to 1bit and add it. This also helps PDF viewers
            // without transparency support
            int bytesPerLine = (w + 7) >> 3;
            QByteArray mask(bytesPerLine * h, 0);
            uchar *mdata = (uchar *)mask.data();
            const uchar *sdata = (const uchar *)softMaskData.constData();
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (*sdata)
                        mdata[x>>3] |= (0x80 >> (x&7));
                    ++sdata;
                }
                mdata += bytesPerLine;
            }
            maskObject = writeImage(mask, w, h, WriteImageOption::Monochrome, 0, 0);
        }

        const WriteImageOption option = [&]() {
            if (grayscale)
                return WriteImageOption::Grayscale;
            if (format == QImage::Format_CMYK8888)
                return WriteImageOption::CMYK;
            return WriteImageOption::RGB;
        }();

        object = writeImage(imageData, w, h, option,
                            maskObject, softMaskObject, dct);
    }
    imageCache.insert(serial_no, object);
    return object;
}

void QPdfEnginePrivate::drawTextItem(const QPointF &p, const QTextItemInt &ti)
{
    Q_Q(QPdfEngine);

    const bool isLink = ti.charFormat.hasProperty(QTextFormat::AnchorHref);
    const bool isAnchor = ti.charFormat.hasProperty(QTextFormat::AnchorName);
    // PDF/X-4 (§ 6.17) does not allow annotations that don't lie
    // outside the BleedBox/TrimBox, so don't emit an hyperlink
    // annotation at all.
    const bool isX4 = pdfVersion == QPdfEngine::Version_X4;
    if ((isLink && !isX4) || isAnchor) {
        qreal size = ti.fontEngine->fontDef.pixelSize;
        int synthesized = ti.fontEngine->synthesized();
        qreal stretch = synthesized & QFontEngine::SynthesizedStretch ? ti.fontEngine->fontDef.stretch/100. : 1.;
        Q_ASSERT(stretch > qreal(0));

        QTransform trans;
        // Build text rendering matrix (Trm). We need it to map the text area to user
        // space units on the PDF page.
        trans = QTransform(size*stretch, 0, 0, size, 0, 0);
        // Apply text matrix (Tm).
        trans *= QTransform(1,0,0,-1,p.x(),p.y());
        // Apply page displacement (Identity for first page).
        trans *= stroker.matrix;
        // Apply Current Transformation Matrix (CTM)
        trans *= pageMatrix();
        qreal x1, y1, x2, y2;
        trans.map(0, 0, &x1, &y1);
        trans.map(ti.width.toReal()/size, (ti.ascent.toReal()-ti.descent.toReal())/size, &x2, &y2);

        if (isLink) {
            uint annot = addXrefEntry(-1);
            QByteArray x1s, y1s, x2s, y2s;
            x1s.setNum(static_cast<double>(x1), 'f');
            y1s.setNum(static_cast<double>(y1), 'f');
            x2s.setNum(static_cast<double>(x2), 'f');
            y2s.setNum(static_cast<double>(y2), 'f');
            QByteArray rectData = x1s + ' ' + y1s + ' ' + x2s + ' ' + y2s;
            xprintf("<<\n/Type /Annot\n/Subtype /Link\n");

            if (pdfVersion == QPdfEngine::Version_A1b)
                xprintf("/F 4\n"); // enable print flag, disable all other

            xprintf("/Rect [");
            write(rectData.constData());
#ifdef Q_DEBUG_PDF_LINKS
            xprintf("]\n/Border [16 16 1]\n");
#else
            xprintf("]\n/Border [0 0 0]\n");
#endif
            const QString link = ti.charFormat.anchorHref();
            const bool isInternal = link.startsWith(QLatin1Char('#'));
            if (!isInternal) {
                xprintf("/A <<\n");
                xprintf("/Type /Action\n/S /URI\n/URI (%s)\n", link.toLatin1().constData());
                xprintf(">>\n");
            } else {
                xprintf("/Dest ");
                printString(link.sliced(1));
                xprintf("\n");
            }
            xprintf(">>\n");
            xprintf("endobj\n");

            if (!currentPage->annotations.contains(annot)) {
                currentPage->annotations.append(annot);
            }
        } else {
            const QString anchor = ti.charFormat.anchorNames().constFirst();
            const uint curPage = pages.last();
            destCache.append(DestInfo({ anchor, curPage, QPointF(x1, y2) }));
        }
    }

    QFontEngine *fe = ti.fontEngine;

    QFontEngine::FaceId face_id = fe->faceId();
    bool noEmbed = false;
    if (!embedFonts
        || face_id.filename.isEmpty()
        || fe->fsType & 0x200 /* bitmap embedding only */
        || fe->fsType == 2 /* no embedding allowed */) {
        *currentPage << "Q\n";
        q->QPaintEngine::drawTextItem(p, ti);
        *currentPage << "q\n";
        if (face_id.filename.isEmpty())
            return;
        noEmbed = true;
    }

    QFontSubset *font = fonts.value(face_id, nullptr);
    if (!font) {
        font = new QFontSubset(fe, requestObject());
        font->noEmbed = noEmbed;
    }
    fonts.insert(face_id, font);

    if (!currentPage->fonts.contains(font->object_id))
        currentPage->fonts.append(font->object_id);

    qreal size = ti.fontEngine->fontDef.pixelSize;

    QVarLengthArray<glyph_t> glyphs;
    QVarLengthArray<QFixedPoint> positions;
    QTransform m = QTransform::fromTranslate(p.x(), p.y());
    ti.fontEngine->getGlyphPositions(ti.glyphs, m, ti.flags,
                                     glyphs, positions);
    if (glyphs.size() == 0)
        return;
    int synthesized = ti.fontEngine->synthesized();
    qreal stretch = synthesized & QFontEngine::SynthesizedStretch ? ti.fontEngine->fontDef.stretch/100. : 1.;
    Q_ASSERT(stretch > qreal(0));

    *currentPage << "BT\n"
                 << "/F" << font->object_id << size << "Tf "
                 << stretch << (synthesized & QFontEngine::SynthesizedItalic
                                ? "0 .3 -1 0 0 Tm\n"
                                : "0 0 -1 0 0 Tm\n");


#if 0
    // #### implement actual text for complex languages
    const unsigned short *logClusters = ti.logClusters;
    int pos = 0;
    do {
        int end = pos + 1;
        while (end < ti.num_chars && logClusters[end] == logClusters[pos])
            ++end;
        *currentPage << "/Span << /ActualText <FEFF";
        for (int i = pos; i < end; ++i) {
            s << toHex((ushort)ti.chars[i].unicode(), buf);
        }
        *currentPage << "> >>\n"
            "BDC\n"
            "<";
        int ge = end == ti.num_chars ? ti.num_glyphs : logClusters[end];
        for (int gs = logClusters[pos]; gs < ge; ++gs)
            *currentPage << toHex((ushort)ti.glyphs[gs].glyph, buf);
        *currentPage << "> Tj\n"
            "EMC\n";
        pos = end;
    } while (pos < ti.num_chars);
#else
    qreal last_x = 0.;
    qreal last_y = 0.;
    for (int i = 0; i < glyphs.size(); ++i) {
        qreal x = positions[i].x.toReal();
        qreal y = positions[i].y.toReal();
        if (synthesized & QFontEngine::SynthesizedItalic)
            x += .3*y;
        x /= stretch;
        char buf[5];
        qsizetype g = font->addGlyph(glyphs[i]);
        *currentPage << x - last_x << last_y - y << "Td <"
                     << QPdf::toHex((ushort)g, buf) << "> Tj\n";
        last_x = x;
        last_y = y;
    }
    if (synthesized & QFontEngine::SynthesizedBold) {
        *currentPage << stretch << (synthesized & QFontEngine::SynthesizedItalic
                            ? "0 .3 -1 0 0 Tm\n"
                            : "0 0 -1 0 0 Tm\n");
        *currentPage << "/Span << /ActualText <> >> BDC\n";
        last_x = 0.5*fe->lineThickness().toReal();
        last_y = 0.;
        for (int i = 0; i < glyphs.size(); ++i) {
            qreal x = positions[i].x.toReal();
            qreal y = positions[i].y.toReal();
            if (synthesized & QFontEngine::SynthesizedItalic)
                x += .3*y;
            x /= stretch;
            char buf[5];
            qsizetype g = font->addGlyph(glyphs[i]);
            *currentPage << x - last_x << last_y - y << "Td <"
                        << QPdf::toHex((ushort)g, buf) << "> Tj\n";
            last_x = x;
            last_y = y;
        }
        *currentPage << "EMC\n";
    }
#endif

    *currentPage << "ET\n";
}

QTransform QPdfEnginePrivate::pageMatrix() const
{
    qreal userUnit = calcUserUnit();
    qreal scale = 72. / userUnit / resolution;
    QTransform tmp(scale, 0.0, 0.0, -scale, 0.0, m_pageLayout.fullRectPoints().height() / userUnit);
    if (m_pageLayout.mode() != QPageLayout::FullPageMode) {
        QRect r = m_pageLayout.paintRectPixels(resolution);
        tmp.translate(r.left(), r.top());
    }
    return tmp;
}

void QPdfEnginePrivate::newPage()
{
    if (currentPage && currentPage->pageSize.isEmpty())
        currentPage->pageSize = m_pageLayout.fullRectPoints().size();
    writePage();

    delete currentPage;
    currentPage = new QPdfPage;
    currentPage->pageSize = m_pageLayout.fullRectPoints().size();
    stroker.stream = currentPage;
    pages.append(requestObject());

    *currentPage << "/GSa gs /CSp cs /CSp CS\n"
                 << QPdf::generateMatrix(pageMatrix())
                 << "q q\n";
}

QT_END_NAMESPACE

#endif // QT_NO_PDF
