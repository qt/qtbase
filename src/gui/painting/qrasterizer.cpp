// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrasterizer_p.h"

#include <QPoint>
#include <QRect>

#include <private/qmath_p.h>
#include <private/qdatabuffer_p.h>
#include <private/qdrawhelper_p.h>

#include <QtGui/qpainterpath.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

#if Q_PROCESSOR_WORDSIZE == 8
typedef qint64 QScFixed;
#else
typedef int QScFixed;
#endif
#define QScFixedToFloat(i) ((i) * (1./65536.))
#define FloatToQScFixed(i) (QScFixed)((i) * 65536.)
#define IntToQScFixed(i) ((QScFixed)(i) * (1 << 16))
#define QScFixedToInt(i) ((i) >> 16)
#define QScFixedFactor 65536
#define FTPosToQScFixed(i) ((QScFixed)(i) * (1 << 10))

#define QScFixedMultiply(x, y) (QScFixed)((qlonglong(x) * qlonglong(y)) >> 16)
#define QScFixedFastMultiply(x, y) (((x) * (y)) >> 16)

#define SPAN_BUFFER_SIZE 256

#define COORD_ROUNDING 1 // 0: round up, 1: round down
#define COORD_OFFSET 32 // 26.6, 32 is half a pixel

static inline QT_FT_Vector PointToVector(const QPointF &p)
{
    QT_FT_Vector result = { QT_FT_Pos(p.x() * 64), QT_FT_Pos(p.y() * 64) };
    return result;
}

class QSpanBuffer {
public:
    QSpanBuffer(ProcessSpans blend, void *data, const QRect &clipRect)
        : m_spanCount(0)
        , m_blend(blend)
        , m_data(data)
        , m_clipRect(clipRect)
    {
    }

    ~QSpanBuffer()
    {
        flushSpans();
    }

    void addSpan(int x, int len, int y, int coverage)
    {
        if (!coverage || !len)
            return;

        Q_ASSERT(coverage >= 0 && coverage <= 255);
        Q_ASSERT(y >= m_clipRect.top());
        Q_ASSERT(y <= m_clipRect.bottom());
        Q_ASSERT(x >= m_clipRect.left());
        Q_ASSERT(x + int(len) - 1 <= m_clipRect.right());

        m_spans[m_spanCount].x = x;
        m_spans[m_spanCount].len = len;
        m_spans[m_spanCount].y = y;
        m_spans[m_spanCount].coverage = coverage;

        if (++m_spanCount == SPAN_BUFFER_SIZE)
            flushSpans();
    }

private:
    void flushSpans()
    {
        m_blend(m_spanCount, m_spans, m_data);
        m_spanCount = 0;
    }

    QT_FT_Span m_spans[SPAN_BUFFER_SIZE];
    int m_spanCount;

    ProcessSpans m_blend;
    void *m_data;

    QRect m_clipRect;
};

#define CHUNK_SIZE 64
class QScanConverter
{
public:
    QScanConverter();
    ~QScanConverter();

    void begin(int top, int bottom, int left, int right,
               Qt::FillRule fillRule, QSpanBuffer *spanBuffer);
    void end();

    void mergeCurve(const QT_FT_Vector &a, const QT_FT_Vector &b,
                    const QT_FT_Vector &c, const QT_FT_Vector &d);
    void mergeLine(QT_FT_Vector a, QT_FT_Vector b);

    struct Line
    {
        QScFixed x;
        QScFixed delta;

        int top, bottom;

        int winding;
    };

private:
    struct Intersection
    {
        int x;
        int winding;

        int left, right;
    };

    inline bool clip(QScFixed &xFP, int &iTop, int &iBottom, QScFixed slopeFP, QScFixed edgeFP, int winding);
    inline void mergeIntersection(Intersection *head, const Intersection &isect);

    void prepareChunk();

    void emitNode(const Intersection *node);
    void emitSpans(int chunk);

    inline void allocate(int size);

    QDataBuffer<Line> m_lines;

    int m_alloc;
    int m_size;

    int m_top;
    int m_bottom;

    QScFixed m_leftFP;
    QScFixed m_rightFP;

    int m_fillRuleMask;

    int m_x;
    int m_y;
    int m_winding;

    Intersection *m_intersections;

    QSpanBuffer *m_spanBuffer;

    QDataBuffer<Line *> m_active;

    template <bool AllVertical>
    void scanConvert();
};

class QRasterizerPrivate
{
public:
    bool antialiased;
    ProcessSpans blend;
    void *data;
    QRect clipRect;

    QScanConverter scanConverter;
};

QScanConverter::QScanConverter()
   : m_lines(0)
   , m_alloc(0)
   , m_size(0)
   , m_intersections(nullptr)
   , m_active(0)
{
}

QScanConverter::~QScanConverter()
{
    if (m_intersections)
        free(m_intersections);
}

void QScanConverter::begin(int top, int bottom, int left, int right,
                           Qt::FillRule fillRule,
                           QSpanBuffer *spanBuffer)
{
    m_top = top;
    m_bottom = bottom;
    m_leftFP = IntToQScFixed(left);
    m_rightFP = IntToQScFixed(right + 1);

    m_lines.reset();

    m_fillRuleMask = fillRule == Qt::WindingFill ? ~0x0 : 0x1;
    m_spanBuffer = spanBuffer;
}

void QScanConverter::prepareChunk()
{
    m_size = CHUNK_SIZE;

    allocate(CHUNK_SIZE);
    memset(m_intersections, 0, CHUNK_SIZE * sizeof(Intersection));
}

void QScanConverter::emitNode(const Intersection *node)
{
tail_call:
    if (node->left)
        emitNode(node + node->left);

    if (m_winding & m_fillRuleMask)
        m_spanBuffer->addSpan(m_x, node->x - m_x, m_y, 0xff);

    m_x = node->x;
    m_winding += node->winding;

    if (node->right) {
        node += node->right;
        goto tail_call;
    }
}

void QScanConverter::emitSpans(int chunk)
{
    for (int dy = 0; dy < CHUNK_SIZE; ++dy) {
        m_x = 0;
        m_y = chunk + dy;
        m_winding = 0;

        emitNode(&m_intersections[dy]);
    }
}

// split control points b[0] ... b[3] into
// left (b[0] ... b[3]) and right (b[3] ... b[6])
static void split(QT_FT_Vector *b)
{
    b[6] = b[3];

    {
        const QT_FT_Pos temp = (b[1].x + b[2].x)/2;

        b[1].x = (b[0].x + b[1].x)/2;
        b[5].x = (b[2].x + b[3].x)/2;
        b[2].x = (b[1].x + temp)/2;
        b[4].x = (b[5].x + temp)/2;
        b[3].x = (b[2].x + b[4].x)/2;
    }
    {
        const QT_FT_Pos temp = (b[1].y + b[2].y)/2;

        b[1].y = (b[0].y + b[1].y)/2;
        b[5].y = (b[2].y + b[3].y)/2;
        b[2].y = (b[1].y + temp)/2;
        b[4].y = (b[5].y + temp)/2;
        b[3].y = (b[2].y + b[4].y)/2;
    }
}

template <bool AllVertical>
void QScanConverter::scanConvert()
{
    if (!m_lines.size()) {
        m_active.reset();
        return;
    }
    constexpr auto topOrder = [](const Line &a, const Line &b) {
        return a.top < b.top;
    };
    constexpr auto xOrder = [](const Line *a, const Line *b) {
        return a->x < b->x;
    };

    std::sort(m_lines.data(), m_lines.data() + m_lines.size(), topOrder);
    int line = 0;
    for (int y = m_lines.first().top; y <= m_bottom; ++y) {
        for (; line < m_lines.size() && m_lines.at(line).top == y; ++line) {
            // add node to active list
            if constexpr(AllVertical) {
                QScanConverter::Line *l = &m_lines.at(line);
                m_active.resize(m_active.size() + 1);
                int j;
                for (j = m_active.size() - 2; j >= 0 && xOrder(l, m_active.at(j)); --j)
                    m_active.at(j+1) = m_active.at(j);
                m_active.at(j+1) = l;
            } else {
                m_active << &m_lines.at(line);
            }
        }

        int numActive = m_active.size();
        if constexpr(!AllVertical) {
            // use insertion sort instead of std::sort, as the active edge list is quite small
            // and in the average case already sorted
            for (int i = 1; i < numActive; ++i) {
                QScanConverter::Line *l = m_active.at(i);
                int j;
                for (j = i-1; j >= 0 && xOrder(l, m_active.at(j)); --j)
                    m_active.at(j+1) = m_active.at(j);
                m_active.at(j+1) = l;
            }
        }

        int x = 0;
        int winding = 0;
        for (int i = 0; i < numActive; ++i) {
            QScanConverter::Line *node = m_active.at(i);

            const int current = QScFixedToInt(node->x);
            if (winding & m_fillRuleMask)
                m_spanBuffer->addSpan(x, current - x, y, 0xff);

            x = current;
            winding += node->winding;

            if (node->bottom == y) {
                // remove node from active list
                for (int j = i; j < numActive - 1; ++j)
                    m_active.at(j) = m_active.at(j+1);

                m_active.resize(--numActive);
                --i;
            } else {
                if constexpr(!AllVertical)
                    node->x += node->delta;
            }
        }
    }
    m_active.reset();
}

void QScanConverter::end()
{
    if (m_lines.isEmpty())
        return;

    if (m_lines.size() <= 32) {
        bool allVertical = true;
        for (int i = 0; i < m_lines.size(); ++i) {
            if (m_lines.at(i).delta) {
                allVertical = false;
                break;
            }
        }
        if (allVertical)
            scanConvert<true>();
        else
            scanConvert<false>();
    } else {
        for (int chunkTop = m_top; chunkTop <= m_bottom; chunkTop += CHUNK_SIZE) {
            prepareChunk();

            Intersection isect = { 0, 0, 0, 0 };

            const int chunkBottom = chunkTop + CHUNK_SIZE;
            for (int i = 0; i < m_lines.size(); ++i) {
                Line &line = m_lines.at(i);

                if ((line.bottom < chunkTop) || (line.top > chunkBottom))
                    continue;

                const int top = qMax(0, line.top - chunkTop);
                const int bottom = qMin(CHUNK_SIZE, line.bottom + 1 - chunkTop);
                allocate(m_size + bottom - top);

                isect.winding = line.winding;

                Intersection *it = m_intersections + top;
                Intersection *end = m_intersections + bottom;

                if (line.delta) {
                    for (; it != end; ++it) {
                        isect.x = QScFixedToInt(line.x);
                        line.x += line.delta;
                        mergeIntersection(it, isect);
                    }
                } else {
                    isect.x = QScFixedToInt(line.x);
                    for (; it != end; ++it)
                        mergeIntersection(it, isect);
                }
            }

            emitSpans(chunkTop);
        }
    }

    if (m_alloc > 1024) {
        free(m_intersections);
        m_alloc = 0;
        m_size = 0;
        m_intersections = nullptr;
    }

    if (m_lines.size() > 1024)
        m_lines.shrink(1024);
}

inline void QScanConverter::allocate(int size)
{
    if (m_alloc < size) {
        int newAlloc = qMax(size, 2 * m_alloc);
        m_intersections = q_check_ptr((Intersection *)realloc(m_intersections, newAlloc * sizeof(Intersection)));
        m_alloc = newAlloc;
    }
}

inline void QScanConverter::mergeIntersection(Intersection *it, const Intersection &isect)
{
    Intersection *current = it;

    while (isect.x != current->x) {
        int &next = isect.x < current->x ? current->left : current->right;
        if (next)
            current += next;
        else {
            Intersection *last = m_intersections + m_size;
            next = last - current;
            *last = isect;
            ++m_size;
            return;
        }
    }

    current->winding += isect.winding;
}

void QScanConverter::mergeCurve(const QT_FT_Vector &pa, const QT_FT_Vector &pb,
                                const QT_FT_Vector &pc, const QT_FT_Vector &pd)
{
    // make room for 32 splits
    QT_FT_Vector beziers[4 + 3 * 32];

    QT_FT_Vector *b = beziers;

    b[0] = pa;
    b[1] = pb;
    b[2] = pc;
    b[3] = pd;

    const QT_FT_Pos flatness = 16;

    while (b >= beziers) {
        QT_FT_Vector delta = { b[3].x - b[0].x, b[3].y - b[0].y };
        QT_FT_Pos l = qAbs(delta.x) + qAbs(delta.y);

        bool belowThreshold;
        if (l > 64) {
            qlonglong d2 = qAbs(qlonglong(b[1].x-b[0].x) * qlonglong(delta.y) -
                                qlonglong(b[1].y-b[0].y) * qlonglong(delta.x));
            qlonglong d3 = qAbs(qlonglong(b[2].x-b[0].x) * qlonglong(delta.y) -
                                qlonglong(b[2].y-b[0].y) * qlonglong(delta.x));

            qlonglong d = d2 + d3;

            belowThreshold = (d <= qlonglong(flatness) * qlonglong(l));
        } else {
            QT_FT_Pos d = qAbs(b[0].x-b[1].x) + qAbs(b[0].y-b[1].y) +
                          qAbs(b[0].x-b[2].x) + qAbs(b[0].y-b[2].y);

            belowThreshold = (d <= flatness);
        }

        if (belowThreshold || b == beziers + 3 * 32) {
            mergeLine(b[0], b[3]);
            b -= 3;
            continue;
        }

        split(b);
        b += 3;
    }
}

inline bool QScanConverter::clip(QScFixed &xFP, int &iTop, int &iBottom, QScFixed slopeFP, QScFixed edgeFP, int winding)
{
    bool right = edgeFP == m_rightFP;

    if (xFP == edgeFP) {
        if ((slopeFP > 0) ^ right)
            return false;
        else {
            Line line = { edgeFP, 0, iTop, iBottom, winding };
            m_lines.add(line);
            return true;
        }
    }

    QScFixed lastFP = xFP + slopeFP * (iBottom - iTop);

    if (lastFP == edgeFP) {
        if ((slopeFP < 0) ^ right)
            return false;
        else {
            Line line = { edgeFP, 0, iTop, iBottom, winding };
            m_lines.add(line);
            return true;
        }
    }

    // does line cross edge?
    if ((lastFP < edgeFP) ^ (xFP < edgeFP)) {
        QScFixed deltaY = QScFixed((edgeFP - xFP) / QScFixedToFloat(slopeFP));

        if ((xFP < edgeFP) ^ right) {
            // top segment needs to be clipped
            int iHeight = QScFixedToInt(deltaY + 1);
            int iMiddle = iTop + iHeight;

            Line line = { edgeFP, 0, iTop, iMiddle, winding };
            m_lines.add(line);

            if (iMiddle != iBottom) {
                xFP += slopeFP * (iHeight + 1);
                iTop = iMiddle + 1;
            } else
                return true;
        } else {
            // bottom segment needs to be clipped
            int iHeight = QScFixedToInt(deltaY);
            int iMiddle = iTop + iHeight;

            if (iMiddle != iBottom) {
                Line line = { edgeFP, 0, iMiddle + 1, iBottom, winding };
                m_lines.add(line);

                iBottom = iMiddle;
            }
        }
        return false;
    } else if ((xFP < edgeFP) ^ right) {
        Line line = { edgeFP, 0, iTop, iBottom, winding };
        m_lines.add(line);
        return true;
    }

    return false;
}

void QScanConverter::mergeLine(QT_FT_Vector a, QT_FT_Vector b)
{
    int winding = 1;

    if (a.y > b.y) {
        qSwap(a, b);
        winding = -1;
    }

    int iTop = qMax(m_top, int((a.y + 32) >> 6));
    int iBottom = qMin(m_bottom, int((b.y - 32) >> 6));

    if (iTop <= iBottom) {
        QScFixed aFP = QScFixedFactor/2 + FTPosToQScFixed(a.x);

        if (b.x == a.x) {
            Line line = { qBound(m_leftFP, aFP, m_rightFP), 0, iTop, iBottom, winding };
            m_lines.add(line);
        } else {
            const qreal slope = (b.x - a.x) / qreal(b.y - a.y);

            const QScFixed slopeFP = FloatToQScFixed(slope);

            QScFixed xFP = aFP + QScFixedMultiply(slopeFP,
                                                  IntToQScFixed(iTop)
                                                  + QScFixedFactor/2 - FTPosToQScFixed(a.y));

            if (clip(xFP, iTop, iBottom, slopeFP, m_leftFP, winding))
                return;

            if (clip(xFP, iTop, iBottom, slopeFP, m_rightFP, winding))
                return;

            Q_ASSERT(xFP >= m_leftFP);

            Line line = { xFP, slopeFP, iTop, iBottom, winding };
            m_lines.add(line);
        }
    }
}

QRasterizer::QRasterizer()
    : d(new QRasterizerPrivate)
{
}

QRasterizer::~QRasterizer()
{
    delete d;
}

void QRasterizer::setAntialiased(bool antialiased)
{
    d->antialiased = antialiased;
}

void QRasterizer::initialize(ProcessSpans blend, void *data)
{
    d->blend = blend;
    d->data = data;
}

void QRasterizer::setClipRect(const QRect &clipRect)
{
    d->clipRect = clipRect;
}

static QScFixed intersectPixelFP(int x, QScFixed top, QScFixed bottom, QScFixed leftIntersectX, QScFixed rightIntersectX, QScFixed slope, QScFixed invSlope)
{
    QScFixed leftX = IntToQScFixed(x);
    QScFixed rightX = IntToQScFixed(x) + QScFixedFactor;

    QScFixed leftIntersectY, rightIntersectY;
    auto computeIntersectY = [&]() {
        if (slope > 0) {
            leftIntersectY = top + QScFixedMultiply(leftX - leftIntersectX, invSlope);
            rightIntersectY = leftIntersectY + invSlope;
        } else {
            leftIntersectY = top + QScFixedMultiply(leftX - rightIntersectX, invSlope);
            rightIntersectY = leftIntersectY + invSlope;
        }
    };

    if (leftIntersectX >= leftX && rightIntersectX <= rightX) {
        return QScFixedMultiply(bottom - top, leftIntersectX - leftX + ((rightIntersectX - leftIntersectX) >> 1));
    } else if (leftIntersectX >= rightX) {
        return bottom - top;
    } else if (leftIntersectX >= leftX) {
        computeIntersectY();
        if (slope > 0) {
            return (bottom - top) - QScFixedFastMultiply((rightX - leftIntersectX) >> 1, rightIntersectY - top);
        } else {
            return (bottom - top) - QScFixedFastMultiply((rightX - leftIntersectX) >> 1, bottom - rightIntersectY);
        }
    } else if (rightIntersectX <= leftX) {
        return 0;
    } else if (rightIntersectX <= rightX) {
        computeIntersectY();
        if (slope > 0) {
            return QScFixedFastMultiply((rightIntersectX - leftX) >> 1, bottom - leftIntersectY);
        } else {
            return QScFixedFastMultiply((rightIntersectX - leftX) >> 1, leftIntersectY - top);
        }
    } else {
        computeIntersectY();
        if (slope > 0) {
            return (bottom - rightIntersectY) + ((rightIntersectY - leftIntersectY) >> 1);
        } else {
            return (rightIntersectY - top) + ((leftIntersectY - rightIntersectY) >> 1);
        }
    }
}

static inline bool q26Dot6Compare(qreal p1, qreal p2)
{
    return int((p2  - p1) * 64.) == 0;
}

static inline QPointF snapTo26Dot6Grid(const QPointF &p)
{
    return QPointF(std::floor(p.x() * 64) * (1 / qreal(64)),
                   std::floor(p.y() * 64) * (1 / qreal(64)));
}

/*
   The rasterize line function relies on some div by zero which should
   result in +/-inf values. However, when floating point exceptions are
   enabled, this will cause crashes, so we return high numbers instead.
   As the returned value is used in further arithmetic, returning
   FLT_MAX/DBL_MAX will also cause values, so instead return a value
   that is well outside the int-range.
 */
static inline qreal qSafeDivide(qreal x, qreal y)
{
    if (y == 0)
        return x > 0 ? 1e20 : -1e20;
    return x / y;
}

/* Conversion to int fails if the value is too large to fit into INT_MAX or
   too small to fit into INT_MIN, so we need this slightly safer conversion
   when floating point exceptions are enabled
 */
static inline QScFixed qSafeFloatToQScFixed(qreal x)
{
    qreal tmp = x * QScFixedFactor;
#if Q_PROCESSOR_WORDSIZE == 8
    if (tmp > qreal(INT_MAX) * QScFixedFactor)
        return QScFixed(INT_MAX) * QScFixedFactor;
    else if (tmp < qreal(INT_MIN) * QScFixedFactor)
        return QScFixed(INT_MIN) * QScFixedFactor;
#else
    if (tmp > qreal(INT_MAX))
        return INT_MAX;
    else if (tmp < qreal(INT_MIN))
        return -INT_MAX;
#endif
    return QScFixed(tmp);
}

// returns true if the whole line gets clipped away
static inline bool qClipLine(QPointF *pt1, QPointF *pt2, const QRectF &clip)
{
    qreal &x1 = pt1->rx();
    qreal &y1 = pt1->ry();
    qreal &x2 = pt2->rx();
    qreal &y2 = pt2->ry();

    if (!qIsFinite(x1) || !qIsFinite(y1) || !qIsFinite(x2) || !qIsFinite(y2))
        return true;

    const qreal xmin = clip.left();
    const qreal xmax = clip.right();
    const qreal ymin = clip.top();
    const qreal ymax = clip.bottom();

    if (x1 < xmin) {
        if (x2 <= xmin)
            return true;
        y1 += (y2 - y1) / (x2 - x1) * (xmin - x1);
        x1 = xmin;
    } else if (x1 > xmax) {
        if (x2 >= xmax)
            return true;
        y1 += (y2 - y1) / (x2 - x1) * (xmax - x1);
        x1 = xmax;
    }
    if (x2 < xmin) {
        y2 += (y2 - y1) / (x2 - x1) * (xmin - x2);
        x2 = xmin;
    } else if (x2 > xmax) {
        y2 += (y2 - y1) / (x2 - x1) * (xmax - x2);
        x2 = xmax;
    }

    if (y1 < ymin) {
        if (y2 <= ymin)
            return true;
        x1 += (x2 - x1) / (y2 - y1) * (ymin - y1);
        y1 = ymin;
    } else if (y1 > ymax) {
        if (y2 >= ymax)
            return true;
        x1 += (x2 - x1) / (y2 - y1) * (ymax - y1);
        y1 = ymax;
    }
    if (y2 < ymin) {
        x2 += (x2 - x1) / (y2 - y1) * (ymin - y2);
        y2 = ymin;
    } else if (y2 > ymax) {
        x2 += (x2 - x1) / (y2 - y1) * (ymax - y2);
        y2 = ymax;
    }

    return false;
}

void QRasterizer::rasterizeLine(const QPointF &a, const QPointF &b, qreal width, bool squareCap)
{
    if (a == b || !(width > 0.0) || d->clipRect.isEmpty())
        return;

    QPointF pa = a;
    QPointF pb = b;

    if (squareCap) {
        QPointF delta = pb - pa;
        pa -= (0.5f * width) * delta;
        pb += (0.5f * width) * delta;
    }

    QPointF offs = QPointF(qAbs(b.y() - a.y()), qAbs(b.x() - a.x())) * width * 0.5;
    const QRectF clip(d->clipRect.topLeft() - offs, d->clipRect.bottomRight() + QPoint(1, 1) + offs);
    if (qClipLine(&pa, &pb, clip))
        return;

    {
        // old delta
        const QPointF d0 = a - b;
        const qreal w0 = d0.x() * d0.x() + d0.y() * d0.y();

        // new delta
        const QPointF d = pa - pb;
        const qreal w = d.x() * d.x() + d.y() * d.y();

        if (w == 0)
            return;

        // adjust width which is given relative to |b - a|
        width *= qSqrt(w0 / w);
    }

    QSpanBuffer buffer(d->blend, d->data, d->clipRect);

    if (q26Dot6Compare(pa.y(), pb.y())) {
        const qreal x = (pa.x() + pb.x()) * 0.5f;
        const qreal dx = qAbs(pb.x() - pa.x()) * 0.5f;

        const qreal y = pa.y();
        const qreal dy = width * dx;

        pa = QPointF(x, y - dy);
        pb = QPointF(x, y + dy);

        width = 1 / width;
    }

    if (q26Dot6Compare(pa.x(), pb.x())) {
        if (pa.y() > pb.y())
            qSwap(pa, pb);

        const qreal dy = pb.y() - pa.y();
        const qreal halfWidth = 0.5f * width * dy;

        qreal left = pa.x() - halfWidth;
        qreal right = pa.x() + halfWidth;

        left = qBound(qreal(d->clipRect.left()), left, qreal(d->clipRect.right() + 1));
        right = qBound(qreal(d->clipRect.left()), right, qreal(d->clipRect.right() + 1));

        pa.ry() = qBound(qreal(d->clipRect.top()), pa.y(), qreal(d->clipRect.bottom() + 1));
        pb.ry() = qBound(qreal(d->clipRect.top()), pb.y(), qreal(d->clipRect.bottom() + 1));

        if (q26Dot6Compare(left, right) || q26Dot6Compare(pa.y(), pb.y()))
            return;

        if (d->antialiased) {
            const int iLeft = int(left);
            const int iRight = int(right);
            const QScFixed leftWidth = IntToQScFixed(iLeft + 1)
                                       - qSafeFloatToQScFixed(left);
            const QScFixed rightWidth = qSafeFloatToQScFixed(right)
                                        - IntToQScFixed(iRight);

            QScFixed coverage[3];
            int x[3];
            int len[3];

            int n = 1;
            if (iLeft == iRight) {
                if (leftWidth == QScFixedFactor)
                    coverage[0] = rightWidth * 255;
                else
                    coverage[0] = (rightWidth + leftWidth - QScFixedFactor) * 255;
                x[0] = iLeft;
                len[0] = 1;
            } else {
                coverage[0] = leftWidth * 255;
                x[0] = iLeft;
                len[0] = 1;
                if (leftWidth == QScFixedFactor) {
                    len[0] = iRight - iLeft;
                } else if (iRight - iLeft > 1) {
                    coverage[1] = IntToQScFixed(255);
                    x[1] = iLeft + 1;
                    len[1] = iRight - iLeft - 1;
                    ++n;
                }
                if (rightWidth) {
                    coverage[n] = rightWidth * 255;
                    x[n] = iRight;
                    len[n] = 1;
                    ++n;
                }
            }

            const QScFixed iTopFP = IntToQScFixed(int(pa.y()));
            const QScFixed iBottomFP = IntToQScFixed(int(pb.y()));
            const QScFixed yPa = qSafeFloatToQScFixed(pa.y());
            const QScFixed yPb = qSafeFloatToQScFixed(pb.y());
            for (QScFixed yFP = iTopFP; yFP <= iBottomFP; yFP += QScFixedFactor) {
                const QScFixed rowHeight = qMin(yFP + QScFixedFactor, yPb)
                                           - qMax(yFP, yPa);
                const int y = QScFixedToInt(yFP);
                if (y > d->clipRect.bottom())
                    break;
                for (int i = 0; i < n; ++i) {
                    buffer.addSpan(x[i], len[i], y,
                                   QScFixedToInt(QScFixedMultiply(rowHeight, coverage[i])));
                }
            }
        } else { // aliased
            int iTop = int(pa.y() + 0.5f);
            int iBottom = pb.y() < 0.5f ? -1 : int(pb.y() - 0.5f);
            int iLeft = int(left + 0.5f);
            int iRight = right < 0.5f ? -1 : int(right - 0.5f);

            int iWidth = iRight - iLeft + 1;
            for (int y = iTop; y <= iBottom; ++y)
                buffer.addSpan(iLeft, iWidth, y, 255);
        }
    } else {
        if (pa.y() > pb.y())
            qSwap(pa, pb);

        QPointF delta = pb - pa;
        delta *= 0.5f * width;
        const QPointF perp(delta.y(), -delta.x());

        QPointF top;
        QPointF left;
        QPointF right;
        QPointF bottom;

        if (pa.x() < pb.x()) {
            top = pa + perp;
            left = pa - perp;
            right = pb + perp;
            bottom = pb - perp;
        } else {
            top = pa - perp;
            left = pb - perp;
            right = pa + perp;
            bottom = pb + perp;
        }

        top = snapTo26Dot6Grid(top);
        bottom = snapTo26Dot6Grid(bottom);
        left = snapTo26Dot6Grid(left);
        right = snapTo26Dot6Grid(right);

        const qreal topBound = qBound(qreal(d->clipRect.top()), top.y(), qreal(d->clipRect.bottom()));
        const qreal bottomBound = qBound(qreal(d->clipRect.top()), bottom.y(), qreal(d->clipRect.bottom()));

        const QPointF topLeftEdge = left - top;
        const QPointF topRightEdge = right - top;
        const QPointF bottomLeftEdge = bottom - left;
        const QPointF bottomRightEdge = bottom - right;

        const qreal topLeftSlope = qSafeDivide(topLeftEdge.x(), topLeftEdge.y());
        const qreal bottomLeftSlope = qSafeDivide(bottomLeftEdge.x(), bottomLeftEdge.y());

        const qreal topRightSlope = qSafeDivide(topRightEdge.x(), topRightEdge.y());
        const qreal bottomRightSlope = qSafeDivide(bottomRightEdge.x(), bottomRightEdge.y());

        const QScFixed topLeftSlopeFP = qSafeFloatToQScFixed(topLeftSlope);
        const QScFixed topRightSlopeFP = qSafeFloatToQScFixed(topRightSlope);

        const QScFixed bottomLeftSlopeFP = qSafeFloatToQScFixed(bottomLeftSlope);
        const QScFixed bottomRightSlopeFP = qSafeFloatToQScFixed(bottomRightSlope);

        const QScFixed invTopLeftSlopeFP = qSafeFloatToQScFixed(qSafeDivide(1, topLeftSlope));
        const QScFixed invTopRightSlopeFP = qSafeFloatToQScFixed(qSafeDivide(1, topRightSlope));

        const QScFixed invBottomLeftSlopeFP = qSafeFloatToQScFixed(qSafeDivide(1, bottomLeftSlope));
        const QScFixed invBottomRightSlopeFP = qSafeFloatToQScFixed(qSafeDivide(1, bottomRightSlope));

        if (d->antialiased) {
            const QScFixed iTopFP = IntToQScFixed(int(topBound));
            const QScFixed iLeftFP = IntToQScFixed(int(left.y()));
            const QScFixed iRightFP = IntToQScFixed(int(right.y()));
            const QScFixed iBottomFP = IntToQScFixed(int(bottomBound));

            QScFixed leftIntersectAf = qSafeFloatToQScFixed(top.x() + (int(topBound) - top.y()) * topLeftSlope);
            QScFixed rightIntersectAf = qSafeFloatToQScFixed(top.x() + (int(topBound) - top.y()) * topRightSlope);
            QScFixed leftIntersectBf = 0;
            QScFixed rightIntersectBf = 0;

            if (iLeftFP < iTopFP)
                leftIntersectBf = qSafeFloatToQScFixed(left.x() + (int(topBound) - left.y()) * bottomLeftSlope);

            if (iRightFP < iTopFP)
                rightIntersectBf = qSafeFloatToQScFixed(right.x() + (int(topBound) - right.y()) * bottomRightSlope);

            QScFixed rowTop, rowBottomLeft, rowBottomRight, rowTopLeft, rowTopRight, rowBottom;
            QScFixed topLeftIntersectAf, topLeftIntersectBf, topRightIntersectAf, topRightIntersectBf;
            QScFixed bottomLeftIntersectAf, bottomLeftIntersectBf, bottomRightIntersectAf, bottomRightIntersectBf;

            int leftMin, leftMax, rightMin, rightMax;

            const QScFixed yTopFP = qSafeFloatToQScFixed(top.y());
            const QScFixed yLeftFP = qSafeFloatToQScFixed(left.y());
            const QScFixed yRightFP = qSafeFloatToQScFixed(right.y());
            const QScFixed yBottomFP = qSafeFloatToQScFixed(bottom.y());

            rowTop = qMax(iTopFP, yTopFP);
            topLeftIntersectAf = leftIntersectAf +
                                 QScFixedMultiply(topLeftSlopeFP, rowTop - iTopFP);
            topRightIntersectAf = rightIntersectAf +
                                  QScFixedMultiply(topRightSlopeFP, rowTop - iTopFP);

            QScFixed yFP = iTopFP;
            while (yFP <= iBottomFP) {
                rowBottomLeft = qMin(yFP + QScFixedFactor, yLeftFP);
                rowBottomRight = qMin(yFP + QScFixedFactor, yRightFP);
                rowTopLeft = qMax(yFP, yLeftFP);
                rowTopRight = qMax(yFP, yRightFP);
                rowBottom = qMin(yFP + QScFixedFactor, yBottomFP);

                if (yFP == iLeftFP) {
                    const int y = QScFixedToInt(yFP);
                    leftIntersectBf = qSafeFloatToQScFixed(left.x() + (y - left.y()) * bottomLeftSlope);
                    topLeftIntersectBf = leftIntersectBf + QScFixedMultiply(bottomLeftSlopeFP, rowTopLeft - yFP);
                    bottomLeftIntersectAf = leftIntersectAf + QScFixedMultiply(topLeftSlopeFP, rowBottomLeft - yFP);
                } else {
                    topLeftIntersectBf = leftIntersectBf;
                    bottomLeftIntersectAf = leftIntersectAf + topLeftSlopeFP;
                }

                if (yFP == iRightFP) {
                    const int y = QScFixedToInt(yFP);
                    rightIntersectBf = qSafeFloatToQScFixed(right.x() + (y - right.y()) * bottomRightSlope);
                    topRightIntersectBf = rightIntersectBf + QScFixedMultiply(bottomRightSlopeFP, rowTopRight - yFP);
                    bottomRightIntersectAf = rightIntersectAf + QScFixedMultiply(topRightSlopeFP, rowBottomRight - yFP);
                } else {
                    topRightIntersectBf = rightIntersectBf;
                    bottomRightIntersectAf = rightIntersectAf + topRightSlopeFP;
                }

                if (yFP == iBottomFP) {
                    bottomLeftIntersectBf = leftIntersectBf + QScFixedMultiply(bottomLeftSlopeFP, rowBottom - yFP);
                    bottomRightIntersectBf = rightIntersectBf + QScFixedMultiply(bottomRightSlopeFP, rowBottom - yFP);
                } else {
                    bottomLeftIntersectBf = leftIntersectBf + bottomLeftSlopeFP;
                    bottomRightIntersectBf = rightIntersectBf + bottomRightSlopeFP;
                }

                if (yFP < iLeftFP) {
                    leftMin = QScFixedToInt(bottomLeftIntersectAf);
                    leftMax = QScFixedToInt(topLeftIntersectAf);
                } else if (yFP == iLeftFP) {
                    leftMin = QScFixedToInt(qMax(bottomLeftIntersectAf, topLeftIntersectBf));
                    leftMax = QScFixedToInt(qMax(topLeftIntersectAf, bottomLeftIntersectBf));
                } else {
                    leftMin = QScFixedToInt(topLeftIntersectBf);
                    leftMax = QScFixedToInt(bottomLeftIntersectBf);
                }

                leftMin = qBound(d->clipRect.left(), leftMin, d->clipRect.right());
                leftMax = qBound(d->clipRect.left(), leftMax, d->clipRect.right());

                if (yFP < iRightFP) {
                    rightMin = QScFixedToInt(topRightIntersectAf);
                    rightMax = QScFixedToInt(bottomRightIntersectAf);
                } else if (yFP == iRightFP) {
                    rightMin = QScFixedToInt(qMin(topRightIntersectAf, bottomRightIntersectBf));
                    rightMax = QScFixedToInt(qMin(bottomRightIntersectAf, topRightIntersectBf));
                } else {
                    rightMin = QScFixedToInt(bottomRightIntersectBf);
                    rightMax = QScFixedToInt(topRightIntersectBf);
                }

                rightMin = qBound(d->clipRect.left(), rightMin, d->clipRect.right());
                rightMax = qBound(d->clipRect.left(), rightMax, d->clipRect.right());

                if (leftMax > rightMax)
                    leftMax = rightMax;
                if (rightMin < leftMin)
                    rightMin = leftMin;

                QScFixed rowHeight = rowBottom - rowTop;

                int x = leftMin;
                while (x <= leftMax) {
                    QScFixed excluded = 0;

                    if (yFP <= iLeftFP && rowBottomLeft > rowTop)
                        excluded += intersectPixelFP(x, rowTop, rowBottomLeft,
                                                     bottomLeftIntersectAf, topLeftIntersectAf,
                                                     topLeftSlopeFP, invTopLeftSlopeFP);
                    if (yFP >= iLeftFP && rowBottom > rowTopLeft)
                        excluded += intersectPixelFP(x, rowTopLeft, rowBottom,
                                                     topLeftIntersectBf, bottomLeftIntersectBf,
                                                     bottomLeftSlopeFP, invBottomLeftSlopeFP);
                    if (x >= rightMin) {
                        if (yFP <= iRightFP && rowBottomRight > rowTop)
                            excluded += (rowBottomRight - rowTop) - intersectPixelFP(x, rowTop, rowBottomRight,
                                                                                     topRightIntersectAf, bottomRightIntersectAf,
                                                                                     topRightSlopeFP, invTopRightSlopeFP);
                        if (yFP >= iRightFP && rowBottom > rowTopRight)
                            excluded += (rowBottom - rowTopRight) - intersectPixelFP(x, rowTopRight, rowBottom,
                                                                                     bottomRightIntersectBf, topRightIntersectBf,
                                                                                     bottomRightSlopeFP, invBottomRightSlopeFP);
                    }

                    Q_ASSERT(excluded >= 0 && excluded <= rowHeight);
                    QScFixed coverage = rowHeight - excluded;
                    buffer.addSpan(x, 1, QScFixedToInt(yFP),
                                   QScFixedToInt(255 * coverage));
                    ++x;
                }
                if (x < rightMin) {
                    buffer.addSpan(x, rightMin - x, QScFixedToInt(yFP),
                                   QScFixedToInt(255 * rowHeight));
                    x = rightMin;
                }
                while (x <= rightMax) {
                    QScFixed excluded = 0;
                    if (yFP <= iRightFP && rowBottomRight > rowTop)
                        excluded += (rowBottomRight - rowTop) - intersectPixelFP(x, rowTop, rowBottomRight,
                                                                                 topRightIntersectAf, bottomRightIntersectAf,
                                                                                 topRightSlopeFP, invTopRightSlopeFP);
                    if (yFP >= iRightFP && rowBottom > rowTopRight)
                        excluded += (rowBottom - rowTopRight) - intersectPixelFP(x, rowTopRight, rowBottom,
                                                                                 bottomRightIntersectBf, topRightIntersectBf,
                                                                                 bottomRightSlopeFP, invBottomRightSlopeFP);

                    Q_ASSERT(excluded >= 0 && excluded <= rowHeight);
                    QScFixed coverage = rowHeight - excluded;
                    buffer.addSpan(x, 1, QScFixedToInt(yFP),
                                   QScFixedToInt(255 * coverage));
                    ++x;
                }

                leftIntersectAf += topLeftSlopeFP;
                leftIntersectBf += bottomLeftSlopeFP;
                rightIntersectAf += topRightSlopeFP;
                rightIntersectBf += bottomRightSlopeFP;
                topLeftIntersectAf = leftIntersectAf;
                topRightIntersectAf = rightIntersectAf;

                yFP += QScFixedFactor;
                rowTop = yFP;
            }
        } else { // aliased
            int iTop = int(top.y() + 0.5f);
            int iLeft = left.y() < 0.5f ? -1 : int(left.y() - 0.5f);
            int iRight = right.y() < 0.5f ? -1 : int(right.y() - 0.5f);
            int iBottom = bottom.y() < 0.5f? -1 : int(bottom.y() - 0.5f);
            int iMiddle = qMin(iLeft, iRight);

            QScFixed leftIntersectAf = qSafeFloatToQScFixed(top.x() + 0.5f + (iTop + 0.5f - top.y()) * topLeftSlope);
            QScFixed leftIntersectBf = qSafeFloatToQScFixed(left.x() + 0.5f + (iLeft + 1.5f - left.y()) * bottomLeftSlope);
            QScFixed rightIntersectAf = qSafeFloatToQScFixed(top.x() - 0.5f + (iTop + 0.5f - top.y()) * topRightSlope);
            QScFixed rightIntersectBf = qSafeFloatToQScFixed(right.x() - 0.5f + (iRight + 1.5f - right.y()) * bottomRightSlope);

            int ny;
            int y = iTop;
#define DO_SEGMENT(next, li, ri, ls, rs) \
            ny = qMin(next + 1, d->clipRect.top()); \
            if (y < ny) { \
                li += ls * (ny - y); \
                ri += rs * (ny - y); \
                y = ny; \
            } \
            if (next > d->clipRect.bottom()) \
                next = d->clipRect.bottom(); \
            for (; y <= next; ++y) { \
                const int x1 = qMax(QScFixedToInt(li), d->clipRect.left()); \
                const int x2 = qMin(QScFixedToInt(ri), d->clipRect.right()); \
                if (x2 >= x1) \
                    buffer.addSpan(x1, x2 - x1 + 1, y, 255); \
                li += ls; \
                ri += rs; \
             }

            DO_SEGMENT(iMiddle, leftIntersectAf, rightIntersectAf, topLeftSlopeFP, topRightSlopeFP)
            DO_SEGMENT(iRight, leftIntersectBf, rightIntersectAf, bottomLeftSlopeFP, topRightSlopeFP)
            DO_SEGMENT(iLeft, leftIntersectAf, rightIntersectBf, topLeftSlopeFP, bottomRightSlopeFP);
            DO_SEGMENT(iBottom, leftIntersectBf, rightIntersectBf, bottomLeftSlopeFP, bottomRightSlopeFP);
#undef DO_SEGMENT
        }
    }
}

void QRasterizer::rasterize(const QT_FT_Outline *outline, Qt::FillRule fillRule)
{
    if (outline->n_points < 3 || outline->n_contours == 0)
        return;

    const QT_FT_Vector *points = outline->points;

    QSpanBuffer buffer(d->blend, d->data, d->clipRect);

    // ### QT_FT_Outline already has a bounding rect which is
    // ### precomputed at this point, so we should probably just be
    // ### using that instead...
    QT_FT_Pos min_y = points[0].y, max_y = points[0].y;
    for (int i = 1; i < outline->n_points; ++i) {
        const QT_FT_Vector &p = points[i];
        min_y = qMin(p.y, min_y);
        max_y = qMax(p.y, max_y);
    }

    int iTopBound = qMax(d->clipRect.top(), int((min_y + 32) >> 6));
    int iBottomBound = qMin(d->clipRect.bottom(), int((max_y - 32) >> 6));

    if (iTopBound > iBottomBound)
        return;

    d->scanConverter.begin(iTopBound, iBottomBound, d->clipRect.left(), d->clipRect.right(), fillRule, &buffer);

    int first = 0;
    for (int i = 0; i < outline->n_contours; ++i) {
        const int last = outline->contours[i];
        for (int j = first; j < last; ++j) {
            if (outline->tags[j+1] == QT_FT_CURVE_TAG_CUBIC) {
                Q_ASSERT(outline->tags[j+2] == QT_FT_CURVE_TAG_CUBIC);
                d->scanConverter.mergeCurve(points[j], points[j+1], points[j+2], points[j+3]);
                j += 2;
            } else {
                d->scanConverter.mergeLine(points[j], points[j+1]);
            }
        }

        first = last + 1;
    }

    d->scanConverter.end();
}

void QRasterizer::rasterize(const QPainterPath &path, Qt::FillRule fillRule)
{
    if (path.isEmpty())
        return;

    QSpanBuffer buffer(d->blend, d->data, d->clipRect);

    QRectF bounds = path.controlPointRect();

    int iTopBound = qMax(d->clipRect.top(), int(bounds.top() + 0.5));
    int iBottomBound = qMin(d->clipRect.bottom(), int(bounds.bottom() - 0.5));

    if (iTopBound > iBottomBound)
        return;

    d->scanConverter.begin(iTopBound, iBottomBound, d->clipRect.left(), d->clipRect.right(), fillRule, &buffer);

    int subpathStart = 0;
    QT_FT_Vector last = { 0, 0 };
    for (int i = 0; i < path.elementCount(); ++i) {
        switch (path.elementAt(i).type) {
        case QPainterPath::LineToElement:
            {
                QT_FT_Vector p1 = last;
                QT_FT_Vector p2 = PointToVector(path.elementAt(i));
                d->scanConverter.mergeLine(p1, p2);
                last = p2;
                break;
            }
        case QPainterPath::MoveToElement:
            {
                if (i != 0) {
                    QT_FT_Vector first = PointToVector(path.elementAt(subpathStart));
                    // close previous subpath
                    if (first.x != last.x || first.y != last.y)
                        d->scanConverter.mergeLine(last, first);
                }
                subpathStart = i;
                last = PointToVector(path.elementAt(i));
                break;
            }
        case QPainterPath::CurveToElement:
            {
                QT_FT_Vector p1 = last;
                QT_FT_Vector p2 = PointToVector(path.elementAt(i));
                QT_FT_Vector p3 = PointToVector(path.elementAt(++i));
                QT_FT_Vector p4 = PointToVector(path.elementAt(++i));
                d->scanConverter.mergeCurve(p1, p2, p3, p4);
                last = p4;
                break;
            }
        default:
            Q_ASSERT(false);
            break;
        }
    }

    QT_FT_Vector first = PointToVector(path.elementAt(subpathStart));

    // close path
    if (first.x != last.x || first.y != last.y)
        d->scanConverter.mergeLine(last, first);

    d->scanConverter.end();
}

QT_END_NAMESPACE
