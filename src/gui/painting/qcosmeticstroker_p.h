#ifndef QCOSMETICSTROKER_P_H
#define QCOSMETICSTROKER_P_H

#include <private/qdrawhelper_p.h>
#include <private/qvectorpath_p.h>
#include <private/qpaintengine_raster_p.h>
#include <qpen.h>

class QCosmeticStroker;


typedef void (*StrokeLine)(QCosmeticStroker *stroker, qreal x1, qreal y1, qreal x2, qreal y2, int caps);

class QCosmeticStroker
{
public:
    struct Point {
        int x;
        int y;
    };
    struct PointF {
        qreal x;
        qreal y;
    };

    enum Caps {
        NoCaps = 0,
        CapBegin = 0x1,
        CapEnd = 0x2,
    };

    // used to avoid drop outs or duplicated points
    enum Direction {
        TopToBottom,
        BottomToTop,
        LeftToRight,
        RightToLeft
    };

    QCosmeticStroker(QRasterPaintEngineState *s, const QRect &dr)
        : state(s),
          clip(dr),
          pattern(0),
          reversePattern(0),
          patternSize(0),
          patternLength(0),
          patternOffset(0),
          current_span(0),
          lastDir(LeftToRight),
          lastAxisAligned(false)
    { setup(); }
    ~QCosmeticStroker() { free(pattern); free(reversePattern); }
    void drawLine(const QPointF &p1, const QPointF &p2);
    void drawPath(const QVectorPath &path);
    void drawPoints(const QPoint *points, int num);
    void drawPoints(const QPointF *points, int num);


    QRasterPaintEngineState *state;
    QRect clip;
    // clip bounds in real
    qreal xmin, xmax;
    qreal ymin, ymax;

    StrokeLine stroke;
    bool drawCaps;

    int *pattern;
    int *reversePattern;
    int patternSize;
    int patternLength;
    int patternOffset;

    enum { NSPANS = 255 };
    QT_FT_Span spans[NSPANS];
    int current_span;
    ProcessSpans blend;

    int opacity;

    uint color;
    uint *pixels;
    int ppl;

    Direction lastDir;
    Point lastPixel;
    bool lastAxisAligned;

private:
    void setup();

    void renderCubic(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, int caps);
    void renderCubicSubdivision(PointF *points, int level, int caps);
    // used for closed subpaths
    void calculateLastPoint(qreal rx1, qreal ry1, qreal rx2, qreal ry2);

public:
    bool clipLine(qreal &x1, qreal &y1, qreal &x2, qreal &y2);
};

#endif // QCOSMETICLINE_H
