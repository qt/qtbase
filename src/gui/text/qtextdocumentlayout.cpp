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

#include "qtextdocumentlayout_p.h"
#include "qtextdocument_p.h"
#include "qtextimagehandler_p.h"
#include "qtexttable.h"
#include "qtextlist.h"
#include "qtextengine_p.h"
#include "private/qcssutil_p.h"
#include "private/qguiapplication_p.h"

#include "qabstracttextdocumentlayout_p.h"
#include "qcssparser_p.h"

#include <qpainter.h>
#include <qmath.h>
#include <qrect.h>
#include <qpalette.h>
#include <qdebug.h>
#include <qvarlengtharray.h>
#include <limits.h>
#include <qbasictimer.h>
#include "private/qfunctions_p.h"
#include <qloggingcategory.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcDraw, "qt.text.drawing")
Q_LOGGING_CATEGORY(lcHit, "qt.text.hittest")
Q_LOGGING_CATEGORY(lcLayout, "qt.text.layout")
Q_LOGGING_CATEGORY(lcTable, "qt.text.layout.table")

// ################ should probably add frameFormatChange notification!

struct QTextLayoutStruct;

class QTextFrameData : public QTextFrameLayoutData
{
public:
    QTextFrameData();

    // relative to parent frame
    QFixedPoint position;
    QFixedSize size;

    // contents starts at (margin+border/margin+border)
    QFixed topMargin;
    QFixed bottomMargin;
    QFixed leftMargin;
    QFixed rightMargin;
    QFixed border;
    QFixed padding;
    // contents width includes padding (as we need to treat this on a per cell basis for tables)
    QFixed contentsWidth;
    QFixed contentsHeight;
    QFixed oldContentsWidth;

    // accumulated margins
    QFixed effectiveTopMargin;
    QFixed effectiveBottomMargin;

    QFixed minimumWidth;
    QFixed maximumWidth;

    QTextLayoutStruct *currentLayoutStruct;

    bool sizeDirty;
    bool layoutDirty;
    bool fullLayoutCompleted;

    QVector<QPointer<QTextFrame> > floats;
};

QTextFrameData::QTextFrameData()
    : maximumWidth(QFIXED_MAX),
      currentLayoutStruct(nullptr), sizeDirty(true), layoutDirty(true), fullLayoutCompleted(false)
{
}

struct QTextLayoutStruct {
    QTextLayoutStruct() : maximumWidth(QFIXED_MAX), fullLayout(false)
    {}
    QTextFrame *frame;
    QFixed x_left;
    QFixed x_right;
    QFixed frameY; // absolute y position of the current frame
    QFixed y; // always relative to the current frame
    QFixed contentsWidth;
    QFixed minimumWidth;
    QFixed maximumWidth;
    bool fullLayout;
    QList<QTextFrame *> pendingFloats;
    QFixed pageHeight;
    QFixed pageBottom;
    QFixed pageTopMargin;
    QFixed pageBottomMargin;
    QRectF updateRect;
    QRectF updateRectForFloats;

    inline void addUpdateRectForFloat(const QRectF &rect) {
        if (updateRectForFloats.isValid())
            updateRectForFloats |= rect;
        else
            updateRectForFloats = rect;
    }

    inline QFixed absoluteY() const
    { return frameY + y; }

    inline QFixed contentHeight() const
    { return pageHeight - pageBottomMargin - pageTopMargin; }

    inline int currentPage() const
    { return pageHeight == 0 ? 0 : (absoluteY() / pageHeight).truncate(); }

    inline void newPage()
    { if (pageHeight == QFIXED_MAX) return; pageBottom += pageHeight; y = qMax(y, pageBottom - pageHeight + pageBottomMargin + pageTopMargin - frameY); }
};

#ifndef QT_NO_CSSPARSER
// helper struct to collect edge data and priorize edges for border-collapse mode
struct EdgeData {

    enum EdgeClass {
        // don't change order, used for comparison
        ClassInvalid,     // queried (adjacent) cell does not exist
        ClassNone,        // no explicit border, no grid, no table border
        ClassGrid,        // 1px grid if drawGrid is true
        ClassTableBorder, // an outermost edge
        ClassExplicit     // set in cell's format
    };

    EdgeData(qreal width, const QTextTableCell &cell, QCss::Edge edge, EdgeClass edgeClass) :
        width(width), cell(cell), edge(edge), edgeClass(edgeClass) {}
    EdgeData() :
        width(0), edge(QCss::NumEdges), edgeClass(ClassInvalid) {}

    // used for priorization with qMax
    bool operator< (const EdgeData &other) const {
        if (width < other.width) return true;
        if (width > other.width) return false;
        if (edgeClass < other.edgeClass) return true;
        if (edgeClass > other.edgeClass) return false;
        if (edge == QCss::TopEdge && other.edge == QCss::BottomEdge) return true;
        if (edge == QCss::BottomEdge && other.edge == QCss::TopEdge) return false;
        if (edge == QCss::LeftEdge && other.edge == QCss::RightEdge) return true;
        return false;
    }
    bool operator> (const EdgeData &other) const {
        return other < *this;
    }

    qreal width;
    QTextTableCell cell;
    QCss::Edge edge;
    EdgeClass edgeClass;
};

// axisEdgeData is referenced by QTextTableData's inline methods, so predeclare
class QTextTableData;
static inline EdgeData axisEdgeData(QTextTable *table, const QTextTableData *td, const QTextTableCell &cell, QCss::Edge edge);
#endif

class QTextTableData : public QTextFrameData
{
public:
    QFixed cellSpacing, cellPadding;
    qreal deviceScale;
    QVector<QFixed> minWidths;
    QVector<QFixed> maxWidths;
    QVector<QFixed> widths;
    QVector<QFixed> heights;
    QVector<QFixed> columnPositions;
    QVector<QFixed> rowPositions;

    QVector<QFixed> cellVerticalOffsets;

    // without borderCollapse, those equal QTextFrameData::border;
    // otherwise the widest outermost cell edge will be used
    QFixed effectiveLeftBorder;
    QFixed effectiveTopBorder;
    QFixed effectiveRightBorder;
    QFixed effectiveBottomBorder;

    QFixed headerHeight;

    QFixed borderCell; // 0 if borderCollapse is enabled, QTextFrameData::border otherwise
    bool borderCollapse;
    bool drawGrid;

    // maps from cell index (row + col * rowCount) to child frames belonging to
    // the specific cell
    QMultiHash<int, QTextFrame *> childFrameMap;

    inline QFixed cellWidth(int column, int colspan) const
    { return columnPositions.at(column + colspan - 1) + widths.at(column + colspan - 1)
             - columnPositions.at(column); }

    inline void calcRowPosition(int row)
    {
        if (row > 0)
            rowPositions[row] = rowPositions.at(row - 1) + heights.at(row - 1) + borderCell + cellSpacing + borderCell;
    }

    QRectF cellRect(const QTextTableCell &cell) const;

    inline QFixed paddingProperty(const QTextFormat &format, QTextFormat::Property property) const
    {
        QVariant v = format.property(property);
        if (v.isNull()) {
            return cellPadding;
        } else {
            Q_ASSERT(v.userType() == QMetaType::Double || v.userType() == QMetaType::Float);
            return QFixed::fromReal(v.toReal() * deviceScale);
        }
    }

#ifndef QT_NO_CSSPARSER
    inline QFixed cellBorderWidth(QTextTable *table, const QTextTableCell &cell, QCss::Edge edge) const
    {
        qreal rv = axisEdgeData(table, this, cell, edge).width;
        if (borderCollapse)
            rv /= 2; // each cell has to add half of the border's width to its own padding
        return QFixed::fromReal(rv * deviceScale);
    }
#endif

    inline QFixed topPadding(QTextTable *table, const QTextTableCell &cell) const
    {
#ifdef QT_NO_CSSPARSER
        Q_UNUSED(table);
#endif
        return paddingProperty(cell.format(), QTextFormat::TableCellTopPadding)
#ifndef QT_NO_CSSPARSER
                + cellBorderWidth(table, cell, QCss::TopEdge)
#endif
        ;
    }

    inline QFixed bottomPadding(QTextTable *table, const QTextTableCell &cell) const
    {
#ifdef QT_NO_CSSPARSER
        Q_UNUSED(table);
#endif
        return paddingProperty(cell.format(), QTextFormat::TableCellBottomPadding)
#ifndef QT_NO_CSSPARSER
                + cellBorderWidth(table, cell, QCss::BottomEdge)
#endif
                ;
    }

    inline QFixed leftPadding(QTextTable *table, const QTextTableCell &cell) const
    {
#ifdef QT_NO_CSSPARSER
        Q_UNUSED(table);
#endif
        return paddingProperty(cell.format(), QTextFormat::TableCellLeftPadding)
#ifndef QT_NO_CSSPARSER
                + cellBorderWidth(table, cell, QCss::LeftEdge)
#endif
        ;
    }

    inline QFixed rightPadding(QTextTable *table, const QTextTableCell &cell) const
    {
#ifdef QT_NO_CSSPARSER
        Q_UNUSED(table);
#endif
        return paddingProperty(cell.format(), QTextFormat::TableCellRightPadding)
#ifndef QT_NO_CSSPARSER
                + cellBorderWidth(table, cell, QCss::RightEdge)
#endif
        ;
    }

    inline QFixedPoint cellPosition(QTextTable *table, const QTextTableCell &cell) const
    {
        return cellPosition(cell.row(), cell.column()) + QFixedPoint(leftPadding(table, cell), topPadding(table, cell));
    }

    void updateTableSize();

private:
    inline QFixedPoint cellPosition(int row, int col) const
    { return QFixedPoint(columnPositions.at(col), rowPositions.at(row) + cellVerticalOffsets.at(col + row * widths.size())); }
};

static QTextFrameData *createData(QTextFrame *f)
{
    QTextFrameData *data;
    if (qobject_cast<QTextTable *>(f))
        data = new QTextTableData;
    else
        data = new QTextFrameData;
    f->setLayoutData(data);
    return data;
}

static inline QTextFrameData *data(QTextFrame *f)
{
    QTextFrameData *data = static_cast<QTextFrameData *>(f->layoutData());
    if (!data)
        data = createData(f);
    return data;
}

static bool isFrameFromInlineObject(QTextFrame *f)
{
    return f->firstPosition() > f->lastPosition();
}

void QTextTableData::updateTableSize()
{
    const QFixed effectiveTopMargin = this->topMargin + effectiveTopBorder + padding;
    const QFixed effectiveBottomMargin = this->bottomMargin + effectiveBottomBorder + padding;
    const QFixed effectiveLeftMargin = this->leftMargin + effectiveLeftBorder + padding;
    const QFixed effectiveRightMargin = this->rightMargin + effectiveRightBorder + padding;
    size.height = contentsHeight == -1
                   ? rowPositions.constLast() + heights.constLast() + padding + border + cellSpacing + effectiveBottomMargin
                   : effectiveTopMargin + contentsHeight + effectiveBottomMargin;
    size.width = effectiveLeftMargin + contentsWidth + effectiveRightMargin;
}

QRectF QTextTableData::cellRect(const QTextTableCell &cell) const
{
    const int row = cell.row();
    const int rowSpan = cell.rowSpan();
    const int column = cell.column();
    const int colSpan = cell.columnSpan();

    return QRectF(columnPositions.at(column).toReal(),
                  rowPositions.at(row).toReal(),
                  (columnPositions.at(column + colSpan - 1) + widths.at(column + colSpan - 1) - columnPositions.at(column)).toReal(),
                  (rowPositions.at(row + rowSpan - 1) + heights.at(row + rowSpan - 1) - rowPositions.at(row)).toReal());
}

static inline bool isEmptyBlockBeforeTable(const QTextBlock &block, const QTextBlockFormat &format, const QTextFrame::Iterator &nextIt)
{
    return !nextIt.atEnd()
           && qobject_cast<QTextTable *>(nextIt.currentFrame())
           && block.isValid()
           && block.length() == 1
           && !format.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)
           && !format.hasProperty(QTextFormat::BackgroundBrush)
           && nextIt.currentFrame()->firstPosition() == block.position() + 1
           ;
}

static inline bool isEmptyBlockBeforeTable(const QTextFrame::Iterator &it)
{
    QTextFrame::Iterator next = it; ++next;
    if (it.currentFrame())
        return false;
    QTextBlock block = it.currentBlock();
    return isEmptyBlockBeforeTable(block, block.blockFormat(), next);
}

static inline bool isEmptyBlockAfterTable(const QTextBlock &block, const QTextFrame *previousFrame)
{
    return qobject_cast<const QTextTable *>(previousFrame)
           && block.isValid()
           && block.length() == 1
           && previousFrame->lastPosition() == block.position() - 1
           ;
}

static inline bool isLineSeparatorBlockAfterTable(const QTextBlock &block, const QTextFrame *previousFrame)
{
    return qobject_cast<const QTextTable *>(previousFrame)
           && block.isValid()
           && block.length() > 1
           && block.text().at(0) == QChar::LineSeparator
           && previousFrame->lastPosition() == block.position() - 1
           ;
}

/*

Optimization strategies:

HTML layout:

* Distinguish between normal and special flow. For normal flow the condition:
  y1 > y2 holds for all blocks with b1.key() > b2.key().
* Special flow is: floats, table cells

* Normal flow within table cells. Tables (not cells) are part of the normal flow.


* If blocks grows/shrinks in height and extends over whole page width at the end, move following blocks.
* If height doesn't change, no need to do anything

Table cells:

* If minWidth of cell changes, recalculate table width, relayout if needed.
* What about maxWidth when doing auto layout?

Floats:
* need fixed or proportional width, otherwise don't float!
* On width/height change relayout surrounding paragraphs.

Document width change:
* full relayout needed


Float handling:

* Floats are specified by a special format object.
* currently only floating images are implemented.

*/

/*

   On the table layouting:

   +---[ table border ]-------------------------
   |      [ cell spacing ]
   |  +------[ cell border ]-----+  +--------
   |  |                          |  |
   |  |
   |  |
   |  |
   |

   rowPositions[i] and columnPositions[i] point at the cell content
   position. So for example the left border is drawn at
   x = columnPositions[i] - fd->border and similar for y.

*/

struct QCheckPoint
{
    QFixed y;
    QFixed frameY; // absolute y position of the current frame
    int positionInFrame;
    QFixed minimumWidth;
    QFixed maximumWidth;
    QFixed contentsWidth;
};
Q_DECLARE_TYPEINFO(QCheckPoint, Q_PRIMITIVE_TYPE);

static bool operator<(const QCheckPoint &checkPoint, QFixed y)
{
    return checkPoint.y < y;
}

static bool operator<(const QCheckPoint &checkPoint, int pos)
{
    return checkPoint.positionInFrame < pos;
}

static void fillBackground(QPainter *p, const QRectF &rect, QBrush brush, const QPointF &origin, const QRectF &gradientRect = QRectF())
{
    p->save();
    if (brush.style() >= Qt::LinearGradientPattern && brush.style() <= Qt::ConicalGradientPattern) {
        if (!gradientRect.isNull()) {
            QTransform m;
            m.translate(gradientRect.left(), gradientRect.top());
            m.scale(gradientRect.width(), gradientRect.height());
            brush.setTransform(m);
            const_cast<QGradient *>(brush.gradient())->setCoordinateMode(QGradient::LogicalMode);
        }
    } else {
        p->setBrushOrigin(origin);
    }
    p->fillRect(rect, brush);
    p->restore();
}

class QTextDocumentLayoutPrivate : public QAbstractTextDocumentLayoutPrivate
{
    Q_DECLARE_PUBLIC(QTextDocumentLayout)
public:
    QTextDocumentLayoutPrivate();

    QTextOption::WrapMode wordWrapMode;
#ifdef LAYOUT_DEBUG
    mutable QString debug_indent;
#endif

    int fixedColumnWidth;
    int cursorWidth;

    QSizeF lastReportedSize;
    QRectF viewportRect;
    QRectF clipRect;

    mutable int currentLazyLayoutPosition;
    mutable int lazyLayoutStepSize;
    QBasicTimer layoutTimer;
    mutable QBasicTimer sizeChangedTimer;
    uint showLayoutProgress : 1;
    uint insideDocumentChange : 1;

    int lastPageCount;
    qreal idealWidth;
    bool contentHasAlignment;

    QFixed blockIndent(const QTextBlockFormat &blockFormat) const;

    void drawFrame(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                   QTextFrame *f) const;
    void drawFlow(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                  QTextFrame::Iterator it, const QList<QTextFrame *> &floats, QTextBlock *cursorBlockNeedingRepaint) const;
    void drawBlock(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                   const QTextBlock &bl, bool inRootFrame) const;
    void drawListItem(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                      const QTextBlock &bl, const QTextCharFormat *selectionFormat) const;
    void drawTableCellBorder(const QRectF &cellRect, QPainter *painter, QTextTable *table, QTextTableData *td, const QTextTableCell &cell) const;
    void drawTableCell(const QRectF &cellRect, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &cell_context,
                       QTextTable *table, QTextTableData *td, int r, int c,
                       QTextBlock *cursorBlockNeedingRepaint, QPointF *cursorBlockOffset) const;
    void drawBorder(QPainter *painter, const QRectF &rect, qreal topMargin, qreal bottomMargin, qreal border,
                    const QBrush &brush, QTextFrameFormat::BorderStyle style) const;
    void drawFrameDecoration(QPainter *painter, QTextFrame *frame, QTextFrameData *fd, const QRectF &clip, const QRectF &rect) const;

    enum HitPoint {
        PointBefore,
        PointAfter,
        PointInside,
        PointExact
    };
    HitPoint hitTest(QTextFrame *frame, const QFixedPoint &point, int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const;
    HitPoint hitTest(QTextFrame::Iterator it, HitPoint hit, const QFixedPoint &p,
                     int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const;
    HitPoint hitTest(QTextTable *table, const QFixedPoint &point, int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const;
    HitPoint hitTest(const QTextBlock &bl, const QFixedPoint &point, int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const;

    QTextLayoutStruct layoutCell(QTextTable *t, const QTextTableCell &cell, QFixed width,
                                 int layoutFrom, int layoutTo, QTextTableData *tableData, QFixed absoluteTableY,
                                 bool withPageBreaks);
    void setCellPosition(QTextTable *t, const QTextTableCell &cell, const QPointF &pos);
    QRectF layoutTable(QTextTable *t, int layoutFrom, int layoutTo, QFixed parentY);

    void positionFloat(QTextFrame *frame, QTextLine *currentLine = nullptr);

    // calls the next one
    QRectF layoutFrame(QTextFrame *f, int layoutFrom, int layoutTo, QFixed parentY = 0);
    QRectF layoutFrame(QTextFrame *f, int layoutFrom, int layoutTo, QFixed frameWidth, QFixed frameHeight, QFixed parentY = 0);

    void layoutBlock(const QTextBlock &bl, int blockPosition, const QTextBlockFormat &blockFormat,
                     QTextLayoutStruct *layoutStruct, int layoutFrom, int layoutTo, const QTextBlockFormat *previousBlockFormat);
    void layoutFlow(QTextFrame::Iterator it, QTextLayoutStruct *layoutStruct, int layoutFrom, int layoutTo, QFixed width = 0);

    void floatMargins(const QFixed &y, const QTextLayoutStruct *layoutStruct, QFixed *left, QFixed *right) const;
    QFixed findY(QFixed yFrom, const QTextLayoutStruct *layoutStruct, QFixed requiredWidth) const;

    QVector<QCheckPoint> checkPoints;

    QTextFrame::Iterator frameIteratorForYPosition(QFixed y) const;
    QTextFrame::Iterator frameIteratorForTextPosition(int position) const;

    void ensureLayouted(QFixed y) const;
    void ensureLayoutedByPosition(int position) const;
    inline void ensureLayoutFinished() const
    { ensureLayoutedByPosition(INT_MAX); }
    void layoutStep() const;

    QRectF frameBoundingRectInternal(QTextFrame *frame) const;

    qreal scaleToDevice(qreal value) const;
    QFixed scaleToDevice(QFixed value) const;
};

QTextDocumentLayoutPrivate::QTextDocumentLayoutPrivate()
    : fixedColumnWidth(-1),
      cursorWidth(1),
      currentLazyLayoutPosition(-1),
      lazyLayoutStepSize(1000),
      lastPageCount(-1)
{
    showLayoutProgress = true;
    insideDocumentChange = false;
    idealWidth = 0;
    contentHasAlignment = false;
}

QTextFrame::Iterator QTextDocumentLayoutPrivate::frameIteratorForYPosition(QFixed y) const
{
    QTextFrame *rootFrame = document->rootFrame();

    if (checkPoints.isEmpty()
        || y < 0 || y > data(rootFrame)->size.height)
        return rootFrame->begin();

    QVector<QCheckPoint>::ConstIterator checkPoint = std::lower_bound(checkPoints.begin(), checkPoints.end(), y);
    if (checkPoint == checkPoints.end())
        return rootFrame->begin();

    if (checkPoint != checkPoints.begin())
        --checkPoint;

    const int position = rootFrame->firstPosition() + checkPoint->positionInFrame;
    return frameIteratorForTextPosition(position);
}

QTextFrame::Iterator QTextDocumentLayoutPrivate::frameIteratorForTextPosition(int position) const
{
    QTextFrame *rootFrame = docPrivate->rootFrame();

    const QTextDocumentPrivate::BlockMap &map = docPrivate->blockMap();
    const int begin = map.findNode(rootFrame->firstPosition());
    const int end = map.findNode(rootFrame->lastPosition()+1);

    const int block = map.findNode(position);
    const int blockPos = map.position(block);

    QTextFrame::iterator it(rootFrame, block, begin, end);

    QTextFrame *containingFrame = docPrivate->frameAt(blockPos);
    if (containingFrame != rootFrame) {
        while (containingFrame->parentFrame() != rootFrame) {
            containingFrame = containingFrame->parentFrame();
            Q_ASSERT(containingFrame);
        }

        it.cf = containingFrame;
        it.cb = 0;
    }

    return it;
}

QTextDocumentLayoutPrivate::HitPoint
QTextDocumentLayoutPrivate::hitTest(QTextFrame *frame, const QFixedPoint &point, int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const
{
    QTextFrameData *fd = data(frame);
    // #########
    if (fd->layoutDirty)
        return PointAfter;
    Q_ASSERT(!fd->layoutDirty);
    Q_ASSERT(!fd->sizeDirty);
    const QFixedPoint relativePoint(point.x - fd->position.x, point.y - fd->position.y);

    QTextFrame *rootFrame = docPrivate->rootFrame();

    qCDebug(lcHit) << "checking frame" << frame->firstPosition() << "point=" << point.toPointF()
                   << "position" << fd->position.toPointF() << "size" << fd->size.toSizeF();
    if (frame != rootFrame) {
        if (relativePoint.y < 0 || relativePoint.x < 0) {
            *position = frame->firstPosition() - 1;
            qCDebug(lcHit) << "before pos=" << *position;
            return PointBefore;
        } else if (relativePoint.y > fd->size.height || relativePoint.x > fd->size.width) {
            *position = frame->lastPosition() + 1;
            qCDebug(lcHit) << "after pos=" << *position;
            return PointAfter;
        }
    }

    if (isFrameFromInlineObject(frame)) {
        *position = frame->firstPosition() - 1;
        return PointExact;
    }

    if (QTextTable *table = qobject_cast<QTextTable *>(frame)) {
        const int rows = table->rows();
        const int columns = table->columns();
        QTextTableData *td = static_cast<QTextTableData *>(data(table));

        if (!td->childFrameMap.isEmpty()) {
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < columns; ++c) {
                    QTextTableCell cell = table->cellAt(r, c);
                    if (cell.row() != r || cell.column() != c)
                        continue;

                    QRectF cellRect = td->cellRect(cell);
                    const QFixedPoint cellPos = QFixedPoint::fromPointF(cellRect.topLeft());
                    const QFixedPoint pointInCell = relativePoint - cellPos;

                    const QList<QTextFrame *> childFrames = td->childFrameMap.values(r + c * rows);
                    for (int i = 0; i < childFrames.size(); ++i) {
                        QTextFrame *child = childFrames.at(i);
                        if (isFrameFromInlineObject(child)
                            && child->frameFormat().position() != QTextFrameFormat::InFlow
                            && hitTest(child, pointInCell, position, l, accuracy) == PointExact)
                        {
                            return PointExact;
                        }
                    }
                }
            }
        }

        return hitTest(table, relativePoint, position, l, accuracy);
    }

    const QList<QTextFrame *> childFrames = frame->childFrames();
    for (int i = 0; i < childFrames.size(); ++i) {
        QTextFrame *child = childFrames.at(i);
        if (isFrameFromInlineObject(child)
            && child->frameFormat().position() != QTextFrameFormat::InFlow
            && hitTest(child, relativePoint, position, l, accuracy) == PointExact)
        {
            return PointExact;
        }
    }

    QTextFrame::Iterator it = frame->begin();

    if (frame == rootFrame) {
        it = frameIteratorForYPosition(relativePoint.y);

        Q_ASSERT(it.parentFrame() == frame);
    }

    if (it.currentFrame())
        *position = it.currentFrame()->firstPosition();
    else
        *position = it.currentBlock().position();

    return hitTest(it, PointBefore, relativePoint, position, l, accuracy);
}

QTextDocumentLayoutPrivate::HitPoint
QTextDocumentLayoutPrivate::hitTest(QTextFrame::Iterator it, HitPoint hit, const QFixedPoint &p,
                                    int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const
{
    for (; !it.atEnd(); ++it) {
        QTextFrame *c = it.currentFrame();
        HitPoint hp;
        int pos = -1;
        if (c) {
            hp = hitTest(c, p, &pos, l, accuracy);
        } else {
            hp = hitTest(it.currentBlock(), p, &pos, l, accuracy);
        }
        if (hp >= PointInside) {
            if (isEmptyBlockBeforeTable(it))
                continue;
            hit = hp;
            *position = pos;
            break;
        }
        if (hp == PointBefore && pos < *position) {
            *position = pos;
            hit = hp;
        } else if (hp == PointAfter && pos > *position) {
            *position = pos;
            hit = hp;
        }
    }

    qCDebug(lcHit) << "inside=" << hit << " pos=" << *position;
    return hit;
}

QTextDocumentLayoutPrivate::HitPoint
QTextDocumentLayoutPrivate::hitTest(QTextTable *table, const QFixedPoint &point,
                                    int *position, QTextLayout **l, Qt::HitTestAccuracy accuracy) const
{
    QTextTableData *td = static_cast<QTextTableData *>(data(table));

    QVector<QFixed>::ConstIterator rowIt = std::lower_bound(td->rowPositions.constBegin(), td->rowPositions.constEnd(), point.y);
    if (rowIt == td->rowPositions.constEnd()) {
        rowIt = td->rowPositions.constEnd() - 1;
    } else if (rowIt != td->rowPositions.constBegin()) {
        --rowIt;
    }

    QVector<QFixed>::ConstIterator colIt = std::lower_bound(td->columnPositions.constBegin(), td->columnPositions.constEnd(), point.x);
    if (colIt == td->columnPositions.constEnd()) {
        colIt = td->columnPositions.constEnd() - 1;
    } else if (colIt != td->columnPositions.constBegin()) {
        --colIt;
    }

    QTextTableCell cell = table->cellAt(rowIt - td->rowPositions.constBegin(),
                                        colIt - td->columnPositions.constBegin());
    if (!cell.isValid())
        return PointBefore;

    *position = cell.firstPosition();

    HitPoint hp = hitTest(cell.begin(), PointInside, point - td->cellPosition(table, cell), position, l, accuracy);

    if (hp == PointExact)
        return hp;
    if (hp == PointAfter)
        *position = cell.lastPosition();
    return PointInside;
}

QTextDocumentLayoutPrivate::HitPoint
QTextDocumentLayoutPrivate::hitTest(const QTextBlock &bl, const QFixedPoint &point, int *position, QTextLayout **l,
                                    Qt::HitTestAccuracy accuracy) const
{
    QTextLayout *tl = bl.layout();
    QRectF textrect = tl->boundingRect();
    textrect.translate(tl->position());
    qCDebug(lcHit) << "    checking block" << bl.position() << "point=" << point.toPointF() << "    tlrect" << textrect;
    *position = bl.position();
    if (point.y.toReal() < textrect.top()) {
        qCDebug(lcHit) << "    before pos=" << *position;
        return PointBefore;
    } else if (point.y.toReal() > textrect.bottom()) {
        *position += bl.length();
        qCDebug(lcHit) << "    after pos=" << *position;
        return PointAfter;
    }

    QPointF pos = point.toPointF() - tl->position();

    // ### rtl?

    HitPoint hit = PointInside;
    *l = tl;
    int off = 0;
    for (int i = 0; i < tl->lineCount(); ++i) {
        QTextLine line = tl->lineAt(i);
        const QRectF lr = line.naturalTextRect();
        if (lr.top() > pos.y()) {
            off = qMin(off, line.textStart());
        } else if (lr.bottom() <= pos.y()) {
            off = qMax(off, line.textStart() + line.textLength());
        } else {
            if (lr.left() <= pos.x() && lr.right() >= pos.x())
                hit = PointExact;
            // when trying to hit an anchor we want it to hit not only in the left
            // half
            if (accuracy == Qt::ExactHit)
                off = line.xToCursor(pos.x(), QTextLine::CursorOnCharacter);
            else
                off = line.xToCursor(pos.x(), QTextLine::CursorBetweenCharacters);
            break;
        }
    }
    *position += off;

    qCDebug(lcHit) << "    inside=" << hit << " pos=" << *position;
    return hit;
}

// ### could be moved to QTextBlock
QFixed QTextDocumentLayoutPrivate::blockIndent(const QTextBlockFormat &blockFormat) const
{
    qreal indent = blockFormat.indent();

    QTextObject *object = document->objectForFormat(blockFormat);
    if (object)
        indent += object->format().toListFormat().indent();

    if (qIsNull(indent))
        return 0;

    qreal scale = 1;
    if (paintDevice) {
        scale = qreal(paintDevice->logicalDpiY()) / qreal(qt_defaultDpi());
    }

    return QFixed::fromReal(indent * scale * document->indentWidth());
}

struct BorderPaginator
{
    BorderPaginator(QTextDocument *document, const QRectF &rect, qreal topMarginAfterPageBreak, qreal bottomMargin, qreal border) :
        pageHeight(document->pageSize().height()),
        topPage(pageHeight > 0 ? static_cast<int>(rect.top() / pageHeight) : 0),
        bottomPage(pageHeight > 0 ? static_cast<int>((rect.bottom() + border) / pageHeight) : 0),
        rect(rect),
        topMarginAfterPageBreak(topMarginAfterPageBreak),
        bottomMargin(bottomMargin), border(border)
    {}

    QRectF clipRect(int page) const
    {
        QRectF clipped = rect.toRect();

        if (topPage != bottomPage) {
            clipped.setTop(qMax(clipped.top(), page * pageHeight + topMarginAfterPageBreak - border));
            clipped.setBottom(qMin(clipped.bottom(), (page + 1) * pageHeight - bottomMargin));

            if (clipped.bottom() <= clipped.top())
                return QRectF();
        }

        return clipped;
    }

    qreal pageHeight;
    int topPage;
    int bottomPage;
    QRectF rect;
    qreal topMarginAfterPageBreak;
    qreal bottomMargin;
    qreal border;
};

void QTextDocumentLayoutPrivate::drawBorder(QPainter *painter, const QRectF &rect, qreal topMargin, qreal bottomMargin,
                                            qreal border, const QBrush &brush, QTextFrameFormat::BorderStyle style) const
{
    BorderPaginator paginator(document, rect, topMargin, bottomMargin, border);

#ifndef QT_NO_CSSPARSER
    QCss::BorderStyle cssStyle = static_cast<QCss::BorderStyle>(style + 1);
#else
    Q_UNUSED(style);
#endif //QT_NO_CSSPARSER

    bool turn_off_antialiasing = !(painter->renderHints() & QPainter::Antialiasing);
    painter->setRenderHint(QPainter::Antialiasing);

    for (int i = paginator.topPage; i <= paginator.bottomPage; ++i) {
        QRectF clipped = paginator.clipRect(i);
        if (!clipped.isValid())
            continue;

#ifndef QT_NO_CSSPARSER
        qDrawEdge(painter, clipped.left(), clipped.top(), clipped.left() + border, clipped.bottom() + border, 0, 0, QCss::LeftEdge, cssStyle, brush);
        qDrawEdge(painter, clipped.left() + border, clipped.top(), clipped.right() + border, clipped.top() + border, 0, 0, QCss::TopEdge, cssStyle, brush);
        qDrawEdge(painter, clipped.right(), clipped.top() + border, clipped.right() + border, clipped.bottom(), 0, 0, QCss::RightEdge, cssStyle, brush);
        qDrawEdge(painter, clipped.left() + border, clipped.bottom(), clipped.right() + border, clipped.bottom() + border, 0, 0, QCss::BottomEdge, cssStyle, brush);
#else
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(brush);
        painter->drawRect(QRectF(clipped.left(), clipped.top(), clipped.left() + border, clipped.bottom() + border));
        painter->drawRect(QRectF(clipped.left() + border, clipped.top(), clipped.right() + border, clipped.top() + border));
        painter->drawRect(QRectF(clipped.right(), clipped.top() + border, clipped.right() + border, clipped.bottom()));
        painter->drawRect(QRectF(clipped.left() + border, clipped.bottom(), clipped.right() + border, clipped.bottom() + border));
        painter->restore();
#endif //QT_NO_CSSPARSER
    }
    if (turn_off_antialiasing)
        painter->setRenderHint(QPainter::Antialiasing, false);
}

void QTextDocumentLayoutPrivate::drawFrameDecoration(QPainter *painter, QTextFrame *frame, QTextFrameData *fd, const QRectF &clip, const QRectF &rect) const
{

    const QBrush bg = frame->frameFormat().background();
    if (bg != Qt::NoBrush) {
        QRectF bgRect = rect;
        bgRect.adjust((fd->leftMargin + fd->border).toReal(),
                      (fd->topMargin + fd->border).toReal(),
                      - (fd->rightMargin + fd->border).toReal(),
                      - (fd->bottomMargin + fd->border).toReal());

        QRectF gradientRect; // invalid makes it default to bgRect
        QPointF origin = bgRect.topLeft();
        if (!frame->parentFrame()) {
            bgRect = clip;
            gradientRect.setWidth(painter->device()->width());
            gradientRect.setHeight(painter->device()->height());
        }
        fillBackground(painter, bgRect, bg, origin, gradientRect);
    }
    if (fd->border != 0) {
        painter->save();
        painter->setBrush(Qt::lightGray);
        painter->setPen(Qt::NoPen);

        const qreal leftEdge = rect.left() + fd->leftMargin.toReal();
        const qreal border = fd->border.toReal();
        const qreal topMargin = fd->topMargin.toReal();
        const qreal leftMargin = fd->leftMargin.toReal();
        const qreal bottomMargin = fd->bottomMargin.toReal();
        const qreal rightMargin = fd->rightMargin.toReal();
        const qreal w = rect.width() - 2 * border - leftMargin - rightMargin;
        const qreal h = rect.height() - 2 * border - topMargin - bottomMargin;

        drawBorder(painter, QRectF(leftEdge, rect.top() + topMargin, w + border, h + border),
                   fd->effectiveTopMargin.toReal(), fd->effectiveBottomMargin.toReal(),
                   border, frame->frameFormat().borderBrush(), frame->frameFormat().borderStyle());

        painter->restore();
    }
}

static void adjustContextSelectionsForCell(QAbstractTextDocumentLayout::PaintContext &cell_context,
                                           const QTextTableCell &cell,
                                           int r, int c,
                                           const int *selectedTableCells)
{
    for (int i = 0; i < cell_context.selections.size(); ++i) {
        int row_start = selectedTableCells[i * 4];
        int col_start = selectedTableCells[i * 4 + 1];
        int num_rows = selectedTableCells[i * 4 + 2];
        int num_cols = selectedTableCells[i * 4 + 3];

        if (row_start != -1) {
            if (r >= row_start && r < row_start + num_rows
                && c >= col_start && c < col_start + num_cols)
            {
                int firstPosition = cell.firstPosition();
                int lastPosition = cell.lastPosition();

                // make sure empty cells are still selected
                if (firstPosition == lastPosition)
                    ++lastPosition;

                cell_context.selections[i].cursor.setPosition(firstPosition);
                cell_context.selections[i].cursor.setPosition(lastPosition, QTextCursor::KeepAnchor);
            } else {
                cell_context.selections[i].cursor.clearSelection();
            }
        }

        // FullWidthSelection is not useful for tables
        cell_context.selections[i].format.clearProperty(QTextFormat::FullWidthSelection);
    }
}

static bool cellClipTest(QTextTable *table, QTextTableData *td,
                         const QAbstractTextDocumentLayout::PaintContext &cell_context,
                         const QTextTableCell &cell,
                         QRectF cellRect)
{
#ifdef QT_NO_CSSPARSER
    Q_UNUSED(table);
    Q_UNUSED(cell);
#endif

    if (!cell_context.clip.isValid())
        return false;

    if (td->borderCollapse) {
        // we need to account for the cell borders in the clipping test
#ifndef QT_NO_CSSPARSER
        cellRect.adjust(-axisEdgeData(table, td, cell, QCss::LeftEdge).width / 2,
                        -axisEdgeData(table, td, cell, QCss::TopEdge).width / 2,
                        axisEdgeData(table, td, cell, QCss::RightEdge).width / 2,
                        axisEdgeData(table, td, cell, QCss::BottomEdge).width / 2);
#endif
    } else {
        qreal border = td->border.toReal();
        cellRect.adjust(-border, -border, border, border);
    }

    if (!cellRect.intersects(cell_context.clip))
        return true;

    return false;
}

void QTextDocumentLayoutPrivate::drawFrame(const QPointF &offset, QPainter *painter,
                                           const QAbstractTextDocumentLayout::PaintContext &context,
                                           QTextFrame *frame) const
{
    QTextFrameData *fd = data(frame);
    // #######
    if (fd->layoutDirty)
        return;
    Q_ASSERT(!fd->sizeDirty);
    Q_ASSERT(!fd->layoutDirty);

    // floor the offset to avoid painting artefacts when drawing adjacent borders
    // we later also round table cell heights and widths
    const QPointF off = QPointF(QPointF(offset + fd->position.toPointF()).toPoint());

    if (context.clip.isValid()
        && (off.y() > context.clip.bottom() || off.y() + fd->size.height.toReal() < context.clip.top()
            || off.x() > context.clip.right() || off.x() + fd->size.width.toReal() < context.clip.left()))
        return;

     qCDebug(lcDraw) << "drawFrame" << frame->firstPosition() << "--" << frame->lastPosition() << "at" << offset;

    // if the cursor is /on/ a table border we may need to repaint it
    // afterwards, as we usually draw the decoration first
    QTextBlock cursorBlockNeedingRepaint;
    QPointF offsetOfRepaintedCursorBlock = off;

    QTextTable *table = qobject_cast<QTextTable *>(frame);
    const QRectF frameRect(off, fd->size.toSizeF());

    if (table) {
        const int rows = table->rows();
        const int columns = table->columns();
        QTextTableData *td = static_cast<QTextTableData *>(data(table));

        QVarLengthArray<int> selectedTableCells(context.selections.size() * 4);
        for (int i = 0; i < context.selections.size(); ++i) {
            const QAbstractTextDocumentLayout::Selection &s = context.selections.at(i);
            int row_start = -1, col_start = -1, num_rows = -1, num_cols = -1;

            if (s.cursor.currentTable() == table)
                s.cursor.selectedTableCells(&row_start, &num_rows, &col_start, &num_cols);

            selectedTableCells[i * 4] = row_start;
            selectedTableCells[i * 4 + 1] = col_start;
            selectedTableCells[i * 4 + 2] = num_rows;
            selectedTableCells[i * 4 + 3] = num_cols;
        }

        QFixed pageHeight = QFixed::fromReal(document->pageSize().height());
        if (pageHeight <= 0)
            pageHeight = QFIXED_MAX;

        QFixed absYPos = td->position.y;
        QTextFrame *parentFrame = table->parentFrame();
        while (parentFrame) {
            absYPos += data(parentFrame)->position.y;
            parentFrame = parentFrame->parentFrame();
        }
        const int tableStartPage = (absYPos / pageHeight).truncate();
        const int tableEndPage = ((absYPos + td->size.height) / pageHeight).truncate();

        // for borderCollapse draw frame decoration by drawing the outermost
        // cell edges with width = td->border
        if (!td->borderCollapse)
            drawFrameDecoration(painter, frame, fd, context.clip, frameRect);

        // draw the repeated table headers for table continuation after page breaks
        const int headerRowCount = qMin(table->format().headerRowCount(), rows - 1);
        int page = tableStartPage + 1;
        while (page <= tableEndPage) {
            const QFixed pageTop = page * pageHeight + td->effectiveTopMargin + td->cellSpacing + td->border;
            const qreal headerOffset = (pageTop - td->rowPositions.at(0)).toReal();
            for (int r = 0; r < headerRowCount; ++r) {
                for (int c = 0; c < columns; ++c) {
                    QTextTableCell cell = table->cellAt(r, c);
                    QAbstractTextDocumentLayout::PaintContext cell_context = context;
                    adjustContextSelectionsForCell(cell_context, cell, r, c, selectedTableCells.data());
                    QRectF cellRect = td->cellRect(cell);

                    cellRect.translate(off.x(), headerOffset);
                    if (cellClipTest(table, td, cell_context, cell, cellRect))
                        continue;

                    drawTableCell(cellRect, painter, cell_context, table, td, r, c, &cursorBlockNeedingRepaint,
                                  &offsetOfRepaintedCursorBlock);
                }
            }
            ++page;
        }

        int firstRow = 0;
        int lastRow = rows;

        if (context.clip.isValid()) {
            QVector<QFixed>::ConstIterator rowIt = std::lower_bound(td->rowPositions.constBegin(), td->rowPositions.constEnd(), QFixed::fromReal(context.clip.top() - off.y()));
            if (rowIt != td->rowPositions.constEnd() && rowIt != td->rowPositions.constBegin()) {
                --rowIt;
                firstRow = rowIt - td->rowPositions.constBegin();
            }

            rowIt = std::upper_bound(td->rowPositions.constBegin(), td->rowPositions.constEnd(), QFixed::fromReal(context.clip.bottom() - off.y()));
            if (rowIt != td->rowPositions.constEnd()) {
                ++rowIt;
                lastRow = rowIt - td->rowPositions.constBegin();
            }
        }

        for (int c = 0; c < columns; ++c) {
            QTextTableCell cell = table->cellAt(firstRow, c);
            firstRow = qMin(firstRow, cell.row());
        }

        for (int r = firstRow; r < lastRow; ++r) {
            for (int c = 0; c < columns; ++c) {
                QTextTableCell cell = table->cellAt(r, c);
                QAbstractTextDocumentLayout::PaintContext cell_context = context;
                adjustContextSelectionsForCell(cell_context, cell, r, c, selectedTableCells.data());
                QRectF cellRect = td->cellRect(cell);

                cellRect.translate(off);
                if (cellClipTest(table, td, cell_context, cell, cellRect))
                    continue;

                drawTableCell(cellRect, painter, cell_context, table, td, r, c, &cursorBlockNeedingRepaint,
                              &offsetOfRepaintedCursorBlock);
            }
        }

    } else {
        drawFrameDecoration(painter, frame, fd, context.clip, frameRect);

        QTextFrame::Iterator it = frame->begin();

        if (frame == docPrivate->rootFrame())
            it = frameIteratorForYPosition(QFixed::fromReal(context.clip.top()));

        QList<QTextFrame *> floats;
        const int numFloats = fd->floats.count();
        floats.reserve(numFloats);
        for (int i = 0; i < numFloats; ++i)
            floats.append(fd->floats.at(i));

        drawFlow(off, painter, context, it, floats, &cursorBlockNeedingRepaint);
    }

    if (cursorBlockNeedingRepaint.isValid()) {
        const QPen oldPen = painter->pen();
        painter->setPen(context.palette.color(QPalette::Text));
        const int cursorPos = context.cursorPosition - cursorBlockNeedingRepaint.position();
        cursorBlockNeedingRepaint.layout()->drawCursor(painter, offsetOfRepaintedCursorBlock,
                                                       cursorPos, cursorWidth);
        painter->setPen(oldPen);
    }

    return;
}

#ifndef QT_NO_CSSPARSER

static inline QTextFormat::Property borderPropertyForEdge(QCss::Edge edge)
{
    switch (edge) {
    case QCss::TopEdge:
        return QTextFormat::TableCellTopBorder;
    case QCss::BottomEdge:
        return QTextFormat::TableCellBottomBorder;
    case QCss::LeftEdge:
        return QTextFormat::TableCellLeftBorder;
    case QCss::RightEdge:
        return QTextFormat::TableCellRightBorder;
    default:
        Q_UNREACHABLE();
        return QTextFormat::UserProperty;
    }
}

static inline QTextFormat::Property borderStylePropertyForEdge(QCss::Edge edge)
{
    switch (edge) {
    case QCss::TopEdge:
        return QTextFormat::TableCellTopBorderStyle;
    case QCss::BottomEdge:
        return QTextFormat::TableCellBottomBorderStyle;
    case QCss::LeftEdge:
        return QTextFormat::TableCellLeftBorderStyle;
    case QCss::RightEdge:
        return QTextFormat::TableCellRightBorderStyle;
    default:
        Q_UNREACHABLE();
        return QTextFormat::UserProperty;
    }
}

static inline QCss::Edge adjacentEdge(QCss::Edge edge)
{
    switch (edge) {
    case QCss::TopEdge:
        return QCss::BottomEdge;
    case QCss::RightEdge:
        return QCss::LeftEdge;
    case QCss::BottomEdge:
        return QCss::TopEdge;
    case QCss::LeftEdge:
        return QCss::RightEdge;
    default:
        Q_UNREACHABLE();
        return QCss::NumEdges;
    }
}

static inline bool isSameAxis(QCss::Edge e1, QCss::Edge e2)
{
    return e1 == e2 || e1 == adjacentEdge(e2);
}

static inline bool isVerticalAxis(QCss::Edge e)
{
    return e % 2 > 0;
}

static inline QTextTableCell adjacentCell(QTextTable *table, const QTextTableCell &cell,
                                          QCss::Edge edge)
{
    int dc = 0;
    int dr = 0;

    switch (edge) {
    case QCss::LeftEdge:
        dc = -1;
        break;
    case QCss::RightEdge:
        dc = cell.columnSpan();
        break;
    case QCss::TopEdge:
        dr = -1;
        break;
    case QCss::BottomEdge:
        dr = cell.rowSpan();
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    // get sibling cell
    int col = cell.column() + dc;
    int row = cell.row() + dr;

    if (col < 0 || row < 0 || col >= table->columns() || row >= table->rows())
        return QTextTableCell();
    else
        return table->cellAt(cell.row() + dr, cell.column() + dc);
}

// returns true if the specified edges of both cells
// are "one the same line" aka axis.
//
// | C0
// |-----|-----|----|-----  < "axis"
// | C1  | C2  | C3 | C4
//
// cell    edge    competingCell competingEdge  result
// C0      Left    C1            Left           true
// C0      Left    C2            Left           false
// C0      Bottom  C2            Top            true
// C0      Bottom  C4            Left           INVALID
static inline bool sharesAxis(const QTextTableCell &cell, QCss::Edge edge,
                              const QTextTableCell &competingCell, QCss::Edge competingCellEdge)
{
    Q_ASSERT(isVerticalAxis(edge) == isVerticalAxis(competingCellEdge));

    switch (edge) {
    case QCss::TopEdge:
        return cell.row() ==
                competingCell.row() + (competingCellEdge == QCss::BottomEdge ? competingCell.rowSpan() : 0);
    case QCss::BottomEdge:
        return cell.row() + cell.rowSpan() ==
                competingCell.row() + (competingCellEdge == QCss::TopEdge ? 0 : competingCell.rowSpan());
    case QCss::LeftEdge:
        return cell.column() ==
                competingCell.column() + (competingCellEdge == QCss::RightEdge ? competingCell.columnSpan() : 0);
    case QCss::RightEdge:
        return cell.column() + cell.columnSpan() ==
                competingCell.column() + (competingCellEdge == QCss::LeftEdge ? 0 : competingCell.columnSpan());
    default:
        Q_UNREACHABLE();
        return false;
    }
}

// returns the applicable EdgeData for the given cell and edge.
// this is either set explicitly by the cell's format, an activated grid
// or the general table border width for outermost edges.
static inline EdgeData cellEdgeData(QTextTable *table, const QTextTableData *td,
                                    const QTextTableCell &cell, QCss::Edge edge)
{
    if (!cell.isValid()) {
        // e.g. non-existing adjacent cell
        return EdgeData();
    }

    QTextTableCellFormat f = cell.format().toTableCellFormat();
    if (f.hasProperty(borderStylePropertyForEdge(edge))) {
        // border style is set
        double width = 3; // default to 3 like browsers do
        if (f.hasProperty(borderPropertyForEdge(edge)))
            width = f.property(borderPropertyForEdge(edge)).toDouble();
        return EdgeData(width, cell, edge, EdgeData::ClassExplicit);
    } else if (td->drawGrid) {
        const bool outermost =
                (edge == QCss::LeftEdge && cell.column() == 0) ||
                (edge == QCss::TopEdge && cell.row() == 0) ||
                (edge == QCss::RightEdge && cell.column() + cell.columnSpan() >= table->columns()) ||
                (edge == QCss::BottomEdge && cell.row() + cell.rowSpan() >= table->rows());

        if (outermost) {
            qreal border = table->format().border();
            if (border > 1.0) {
                // table border
                return EdgeData(border, cell, edge, EdgeData::ClassTableBorder);
            }
        }
        // 1px clean grid
        return EdgeData(1.0, cell, edge, EdgeData::ClassGrid);
    }
    else {
        return EdgeData(0, cell, edge, EdgeData::ClassNone);
    }
}

// returns the EdgeData with the larger width of either the cell's edge its adjacent cell's edge
static inline EdgeData axisEdgeData(QTextTable *table, const QTextTableData *td,
                                    const QTextTableCell &cell, QCss::Edge edge)
{
    Q_ASSERT(cell.isValid());

    EdgeData result = cellEdgeData(table, td, cell, edge);
    if (!td->borderCollapse)
        return result;

    QTextTableCell ac = adjacentCell(table, cell, edge);
    result = qMax(result, cellEdgeData(table, td, ac, adjacentEdge(edge)));

    bool mustCheckThirdCell = false;
    if (ac.isValid()) {
        /* if C0 and C3 don't share the left/top axis, we must
         * also check C1.
         *
         * C0 and C4 don't share the left axis so we have
         * to take the top edge of C1 (T1) into account
         * because this might be wider than C0's bottom
         * edge (B0). For the sake of simplicity we skip
         * checking T2 and T3.
         *
         * | C0
         * |-----|-----|----|-----
         * | C1  | C2  | C3 | C4
         *
         * width(T4) = max(T4, B0, T1) (T2 and T3 won't be checked)
         */
        switch (edge) {
        case QCss::TopEdge:
        case QCss::BottomEdge:
            mustCheckThirdCell = !sharesAxis(cell, QCss::LeftEdge, ac, QCss::LeftEdge);
            break;
        case QCss::LeftEdge:
        case QCss::RightEdge:
            mustCheckThirdCell = !sharesAxis(cell, QCss::TopEdge, ac, QCss::TopEdge);
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    if (mustCheckThirdCell)
        result = qMax(result, cellEdgeData(table, td, adjacentCell(table, ac, adjacentEdge(edge)), edge));

    return result;
}

// checks an edge's joined competing edge according to priority rules and
// adjusts maxCompetingEdgeData and maxOrthogonalEdgeData
static inline void checkJoinedEdge(QTextTable *table, const QTextTableData *td, const QTextTableCell &cell,
                                   QCss::Edge competingEdge,
                                   const EdgeData &edgeData,
                                   bool couldHaveContinuation,
                                   EdgeData *maxCompetingEdgeData,
                                   EdgeData *maxOrthogonalEdgeData)
{
    EdgeData competingEdgeData = axisEdgeData(table, td, cell, competingEdge);

    if (competingEdgeData > edgeData) {
        *maxCompetingEdgeData = competingEdgeData;
    } else if (competingEdgeData.width == edgeData.width) {
        if ((isSameAxis(edgeData.edge, competingEdge) && couldHaveContinuation)
                || (!isVerticalAxis(edgeData.edge) && isVerticalAxis(competingEdge)) /* both widths are equal, vertical edge has priority */ ) {
            *maxCompetingEdgeData = competingEdgeData;
        }
    }

    if (maxOrthogonalEdgeData && competingEdgeData.width > maxOrthogonalEdgeData->width)
        *maxOrthogonalEdgeData = competingEdgeData;
}

// the offset to make adjacent edges overlap in border collapse mode
static inline qreal collapseOffset(const QTextDocumentLayoutPrivate *p, const EdgeData &w)
{
    return p->scaleToDevice(w.width) / 2.0;
}

// returns the offset that must be applied to the edge's
// anchor (start point or end point) to avoid overlapping edges.
//
// Example 1:
//       2
//       2
// 11111144444444      4 = top edge of cell, 4 pixels width
//       3             3 = right edge of cell, 3 pixels width
//       3 cell 4
//
// cell 4's top border is the widest border and will be
// drawn with horiz. offset = -3/2 whereas its left border
// of width 3 will be drawn with vert. offset = +4/2.
//
// Example 2:
//       2
//       2
// 11111143333333
//       4
//       4 cell 4
//
// cell 4's left border is the widest and will be drawn
// with vert. offset = -3/2 whereas its top border
// of of width 3 will be drawn with hor. offset = +4/2.
//
// couldHaveContinuation: true for "end" anchor of an edge:
//      C
// AAAAABBBBBB
//      D
// width(A) == width(B) we consider B to be a continuation of A, so that B wins
// and will be painted. A would only be painted including the right anchor if
// there was no edge B (due to a rowspan or the axis C-D being the table's right
// border).
//
// ignoreEdgesAbove: true if an egde (left, right or top) for the first row
// after a table page break should be painted. In this case the edges of the
// row above must be ignored.
static inline double prioritizedEdgeAnchorOffset(const QTextDocumentLayoutPrivate *p,
                                                 QTextTable *table, const QTextTableData *td,
                                                 const QTextTableCell &cell,
                                                 const EdgeData &edgeData,
                                                 QCss::Edge orthogonalEdge,
                                                 bool couldHaveContinuation,
                                                 bool ignoreEdgesAbove)
{
    EdgeData maxCompetingEdgeData;
    EdgeData maxOrthogonalEdgeData;
    QTextTableCell competingCell;

    // reference scenario for the inline comments:
    // - edgeData being the top "T0" edge of C0
    // - right anchor is '+', orthogonal edge is "R0"
    //   B C3 R|L C2 B
    //   ------+------
    //   T C0 R|L C1 T

    // C0: T0/B3
    // this is "edgeData"

    // C0: R0/L1
    checkJoinedEdge(table, td, cell, orthogonalEdge, edgeData, false,
                    &maxCompetingEdgeData, &maxOrthogonalEdgeData);

    if (td->borderCollapse) {
        // C1: T1/B2
        if (!isVerticalAxis(edgeData.edge) || !ignoreEdgesAbove) {
            competingCell = adjacentCell(table, cell, orthogonalEdge);
            if (competingCell.isValid()) {
                checkJoinedEdge(table, td, competingCell, edgeData.edge, edgeData, couldHaveContinuation,
                                &maxCompetingEdgeData, nullptr);
            }
        }

        // C3: R3/L2
        if (edgeData.edge != QCss::TopEdge || !ignoreEdgesAbove) {
            competingCell = adjacentCell(table, cell, edgeData.edge);
            if (competingCell.isValid() && sharesAxis(cell, orthogonalEdge, competingCell, orthogonalEdge)) {
                checkJoinedEdge(table, td, competingCell, orthogonalEdge, edgeData, false,
                                &maxCompetingEdgeData, &maxOrthogonalEdgeData);
            }
        }
    }

    // wider edge has priority
    bool hasPriority = edgeData > maxCompetingEdgeData;

    if (td->borderCollapse) {
        qreal offset = collapseOffset(p, maxOrthogonalEdgeData);
        return hasPriority ? -offset : offset;
    }
    else
        return hasPriority ? 0 : p->scaleToDevice(maxOrthogonalEdgeData.width);
}

// draw one edge of the given cell
//
// these options are for pagination / pagebreak handling:
//
// forceHeaderRow: true for all rows directly below a (repeated) header row.
//  if the table has headers the first row after a page break must check against
//  the last table header's row, not its actual predecessor.
//
// adjustTopAnchor: false for rows that are a continuation of a row after a page break
//   only evaluated for left/right edges
//
// adjustBottomAnchor: false for rows that will continue after a page break
//   only evaluated for left/right edges
//
// ignoreEdgesAbove: true if a row starts on top of the page and the
//   bottom edges of the prior row can therefore be ignored.
static inline
void drawCellBorder(const QTextDocumentLayoutPrivate *p, QPainter *painter,
                    QTextTable *table, const QTextTableData *td, const QTextTableCell &cell,
                    const QRectF &borderRect, QCss::Edge edge,
                    int forceHeaderRow, bool adjustTopAnchor, bool adjustBottomAnchor,
                    bool ignoreEdgesAbove)
{
    QPointF p1, p2;
    qreal wh = 0;
    qreal wv = 0;
    EdgeData edgeData = axisEdgeData(table, td, cell, edge);

    if (edgeData.width == 0)
        return;

    QTextTableCellFormat fmt = edgeData.cell.format().toTableCellFormat();
    QTextFrameFormat::BorderStyle borderStyle = QTextFrameFormat::BorderStyle_None;
    QBrush brush;

    if (edgeData.edgeClass != EdgeData::ClassExplicit && td->drawGrid) {
        borderStyle = QTextFrameFormat::BorderStyle_Solid;
        brush = table->format().borderBrush();
    }
    else {
        switch (edgeData.edge) {
        case QCss::TopEdge:
            brush = fmt.topBorderBrush();
            borderStyle = fmt.topBorderStyle();
            break;
        case QCss::BottomEdge:
            brush = fmt.bottomBorderBrush();
            borderStyle = fmt.bottomBorderStyle();
            break;
        case QCss::LeftEdge:
            brush = fmt.leftBorderBrush();
            borderStyle = fmt.leftBorderStyle();
            break;
        case QCss::RightEdge:
            brush = fmt.rightBorderBrush();
            borderStyle = fmt.rightBorderStyle();
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    if (borderStyle == QTextFrameFormat::BorderStyle_None)
        return;

    // assume black if not explicit brush is set
    if (brush.style() == Qt::NoBrush)
        brush = Qt::black;

    QTextTableCell cellOrHeader = cell;
    if (forceHeaderRow != -1)
        cellOrHeader = table->cellAt(forceHeaderRow, cell.column());

    // adjust start and end anchors (e.g. left/right for top) according to priority rules
    switch (edge) {
    case QCss::TopEdge:
        wv = p->scaleToDevice(edgeData.width);
        p1 = borderRect.topLeft()
                + QPointF(qFloor(prioritizedEdgeAnchorOffset(p, table, td, cell, edgeData, QCss::LeftEdge, false, ignoreEdgesAbove)), 0);
        p2 = borderRect.topRight()
                + QPointF(-qCeil(prioritizedEdgeAnchorOffset(p, table, td, cell, edgeData, QCss::RightEdge, true, ignoreEdgesAbove)), 0);
        break;
    case QCss::BottomEdge:
        wv = p->scaleToDevice(edgeData.width);
        p1 = borderRect.bottomLeft()
                + QPointF(qFloor(prioritizedEdgeAnchorOffset(p, table, td, cell, edgeData, QCss::LeftEdge, false, false)), -wv);
        p2 = borderRect.bottomRight()
                + QPointF(-qCeil(prioritizedEdgeAnchorOffset(p, table, td, cell, edgeData, QCss::RightEdge, true, false)), -wv);
        break;
    case QCss::LeftEdge:
        wh = p->scaleToDevice(edgeData.width);
        p1 = borderRect.topLeft()
                + QPointF(0, adjustTopAnchor ? qFloor(prioritizedEdgeAnchorOffset(p, table, td, cellOrHeader, edgeData,
                                                                                  forceHeaderRow != -1 ? QCss::BottomEdge : QCss::TopEdge,
                                                                                  false, ignoreEdgesAbove))
                                             : 0);
        p2 = borderRect.bottomLeft()
                + QPointF(0, adjustBottomAnchor ? -qCeil(prioritizedEdgeAnchorOffset(p, table, td, cell, edgeData, QCss::BottomEdge, true, false))
                                                : 0);
        break;
    case QCss::RightEdge:
        wh = p->scaleToDevice(edgeData.width);
        p1 = borderRect.topRight()
                + QPointF(-wh, adjustTopAnchor ? qFloor(prioritizedEdgeAnchorOffset(p, table, td, cellOrHeader, edgeData,
                                                                                    forceHeaderRow != -1 ? QCss::BottomEdge : QCss::TopEdge,
                                                                                    false, ignoreEdgesAbove))
                                               : 0);
        p2 = borderRect.bottomRight()
                + QPointF(-wh, adjustBottomAnchor ? -qCeil(prioritizedEdgeAnchorOffset(p, table, td, cell, edgeData, QCss::BottomEdge, true, false))
                                                  : 0);
        break;
    default: break;
    }

    // for borderCollapse move edge width/2 pixel out of the borderRect
    // so that it shares space with the adjacent cell's edge.
    // to avoid fractional offsets, qCeil/qFloor is used
    if (td->borderCollapse) {
        QPointF offset;
        switch (edge) {
        case QCss::TopEdge:
            offset = QPointF(0, -qCeil(collapseOffset(p, edgeData)));
            break;
        case QCss::BottomEdge:
            offset = QPointF(0, qFloor(collapseOffset(p, edgeData)));
            break;
        case QCss::LeftEdge:
            offset = QPointF(-qCeil(collapseOffset(p, edgeData)), 0);
            break;
        case QCss::RightEdge:
            offset = QPointF(qFloor(collapseOffset(p, edgeData)), 0);
            break;
        default: break;
        }
        p1 += offset;
        p2 += offset;
    }

    QCss::BorderStyle cssStyle = static_cast<QCss::BorderStyle>(borderStyle + 1);

// this reveals errors in the drawing logic
#ifdef COLLAPSE_DEBUG
    QColor c = brush.color();
    c.setAlpha(150);
    brush.setColor(c);
#endif

    qDrawEdge(painter, p1.x(), p1.y(), p2.x() + wh, p2.y() + wv, 0, 0, edge, cssStyle, brush);
}
#endif

void QTextDocumentLayoutPrivate::drawTableCellBorder(const QRectF &cellRect, QPainter *painter,
                                                     QTextTable *table, QTextTableData *td,
                                                     const QTextTableCell &cell) const
{
#ifndef QT_NO_CSSPARSER
    qreal topMarginAfterPageBreak = (td->effectiveTopMargin + td->cellSpacing + td->border).toReal();
    qreal bottomMargin = (td->effectiveBottomMargin + td->cellSpacing + td->border).toReal();

    const int headerRowCount = qMin(table->format().headerRowCount(), table->rows() - 1);
    if (headerRowCount > 0 && cell.row() >= headerRowCount)
        topMarginAfterPageBreak += td->headerHeight.toReal();

    BorderPaginator paginator(document, cellRect, topMarginAfterPageBreak, bottomMargin, 0);

    bool turn_off_antialiasing = !(painter->renderHints() & QPainter::Antialiasing);
    painter->setRenderHint(QPainter::Antialiasing);

    // paint cell borders for every page the cell appears on
    for (int page = paginator.topPage; page <= paginator.bottomPage; ++page) {
        const QRectF clipped = paginator.clipRect(page);
        if (!clipped.isValid())
            continue;

        const qreal offset = cellRect.top() - td->rowPositions.at(cell.row()).toReal();
        const int lastHeaderRow = table->format().headerRowCount() - 1;
        const bool tableHasHeader = table->format().headerRowCount() > 0;
        const bool isHeaderRow = cell.row() < table->format().headerRowCount();
        const bool isFirstRow = cell.row() == lastHeaderRow + 1;
        const bool isLastRow = cell.row() + cell.rowSpan() >= table->rows();
        const bool previousRowOnPreviousPage = !isFirstRow
                && !isHeaderRow
                && BorderPaginator(document,
                                   td->cellRect(adjacentCell(table, cell, QCss::TopEdge)).translated(0, offset),
                                   topMarginAfterPageBreak,
                                   bottomMargin,
                                   0).bottomPage < page;
        const bool nextRowOnNextPage = !isLastRow
                && BorderPaginator(document,
                                   td->cellRect(adjacentCell(table, cell, QCss::BottomEdge)).translated(0, offset),
                                   topMarginAfterPageBreak,
                                   bottomMargin,
                                   0).topPage > page;
        const bool rowStartsOnPage = page == paginator.topPage;
        const bool rowEndsOnPage = page == paginator.bottomPage;
        const bool rowStartsOnPageTop = !tableHasHeader
                && rowStartsOnPage
                && previousRowOnPreviousPage;
        const bool rowStartsOnPageBelowHeader = tableHasHeader
                && rowStartsOnPage
                && previousRowOnPreviousPage;

        const bool suppressTopBorder = td->borderCollapse
                ? !isHeaderRow && (!rowStartsOnPage || rowStartsOnPageBelowHeader)
                : !rowStartsOnPage;
        const bool suppressBottomBorder = td->borderCollapse
                ? !isHeaderRow && (!rowEndsOnPage || nextRowOnNextPage)
                : !rowEndsOnPage;
        const bool doNotAdjustTopAnchor = td->borderCollapse
                ? !tableHasHeader && !rowStartsOnPage
                : !rowStartsOnPage;
        const bool doNotAdjustBottomAnchor = suppressBottomBorder;

        if (!suppressTopBorder) {
            drawCellBorder(this, painter, table, td, cell, clipped, QCss::TopEdge,
                           -1, true, true, rowStartsOnPageTop);
        }

        drawCellBorder(this, painter, table, td, cell, clipped, QCss::LeftEdge,
                       suppressTopBorder ? lastHeaderRow : -1,
                       !doNotAdjustTopAnchor,
                       !doNotAdjustBottomAnchor,
                       rowStartsOnPageTop);
        drawCellBorder(this, painter, table, td, cell, clipped, QCss::RightEdge,
                       suppressTopBorder ? lastHeaderRow : -1,
                       !doNotAdjustTopAnchor,
                       !doNotAdjustBottomAnchor,
                       rowStartsOnPageTop);

        if (!suppressBottomBorder) {
            drawCellBorder(this, painter, table, td, cell, clipped, QCss::BottomEdge,
                           -1, true, true, false);
        }
    }

    if (turn_off_antialiasing)
        painter->setRenderHint(QPainter::Antialiasing, false);
#else
    Q_UNUSED(cell);
    Q_UNUSED(cellRect);
    Q_UNUSED(painter);
    Q_UNUSED(table);
    Q_UNUSED(td);
    Q_UNUSED(cell);
#endif
}

void QTextDocumentLayoutPrivate::drawTableCell(const QRectF &cellRect, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &cell_context,
                                               QTextTable *table, QTextTableData *td, int r, int c,
                                               QTextBlock *cursorBlockNeedingRepaint, QPointF *cursorBlockOffset) const
{
    QTextTableCell cell = table->cellAt(r, c);
    int rspan = cell.rowSpan();
    int cspan = cell.columnSpan();
    if (rspan != 1) {
        int cr = cell.row();
        if (cr != r)
            return;
    }
    if (cspan != 1) {
        int cc = cell.column();
        if (cc != c)
            return;
    }

    const QFixed leftPadding = td->leftPadding(table, cell);
    const QFixed topPadding = td->topPadding(table, cell);

    qreal topMargin = (td->effectiveTopMargin + td->cellSpacing + td->border).toReal();
    qreal bottomMargin = (td->effectiveBottomMargin + td->cellSpacing + td->border).toReal();

    const int headerRowCount = qMin(table->format().headerRowCount(), table->rows() - 1);
    if (r >= headerRowCount)
        topMargin += td->headerHeight.toReal();

    if (!td->borderCollapse && td->border != 0) {
        const QBrush oldBrush = painter->brush();
        const QPen oldPen = painter->pen();

        const qreal border = td->border.toReal();

        QRectF borderRect(cellRect.left() - border, cellRect.top() - border, cellRect.width() + border, cellRect.height() + border);

        // invert the border style for cells
        QTextFrameFormat::BorderStyle cellBorder = table->format().borderStyle();
        switch (cellBorder) {
        case QTextFrameFormat::BorderStyle_Inset:
            cellBorder = QTextFrameFormat::BorderStyle_Outset;
            break;
        case QTextFrameFormat::BorderStyle_Outset:
            cellBorder = QTextFrameFormat::BorderStyle_Inset;
            break;
        case QTextFrameFormat::BorderStyle_Groove:
            cellBorder = QTextFrameFormat::BorderStyle_Ridge;
            break;
        case QTextFrameFormat::BorderStyle_Ridge:
            cellBorder = QTextFrameFormat::BorderStyle_Groove;
            break;
        default:
            break;
        }

        drawBorder(painter, borderRect, topMargin, bottomMargin,
                   border, table->format().borderBrush(), cellBorder);

        painter->setBrush(oldBrush);
        painter->setPen(oldPen);
    }

    const QBrush bg = cell.format().background();
    const QPointF brushOrigin = painter->brushOrigin();
    if (bg.style() != Qt::NoBrush) {
        const qreal pageHeight = document->pageSize().height();
        const int topPage = pageHeight > 0 ? static_cast<int>(cellRect.top() / pageHeight) : 0;
        const int bottomPage = pageHeight > 0 ? static_cast<int>((cellRect.bottom()) / pageHeight) : 0;

        if (topPage == bottomPage)
            fillBackground(painter, cellRect, bg, cellRect.topLeft());
        else {
            for (int i = topPage; i <= bottomPage; ++i) {
                QRectF clipped = cellRect.toRect();

                if (topPage != bottomPage) {
                    const qreal top = qMax(i * pageHeight + topMargin, cell_context.clip.top());
                    const qreal bottom = qMin((i + 1) * pageHeight - bottomMargin, cell_context.clip.bottom());

                    clipped.setTop(qMax(clipped.top(), top));
                    clipped.setBottom(qMin(clipped.bottom(), bottom));

                    if (clipped.bottom() <= clipped.top())
                        continue;

                    fillBackground(painter, clipped, bg, cellRect.topLeft());
                }
            }
        }

        if (bg.style() > Qt::SolidPattern)
            painter->setBrushOrigin(cellRect.topLeft());
    }

    // paint over the background - otherwise we would have to adjust the background paint cellRect for the border values
    drawTableCellBorder(cellRect, painter, table, td, cell);

    const QFixed verticalOffset = td->cellVerticalOffsets.at(c + r * table->columns());

    const QPointF cellPos = QPointF(cellRect.left() + leftPadding.toReal(),
                                    cellRect.top() + (topPadding + verticalOffset).toReal());

    QTextBlock repaintBlock;
    drawFlow(cellPos, painter, cell_context, cell.begin(),
             td->childFrameMap.values(r + c * table->rows()),
             &repaintBlock);
    if (repaintBlock.isValid()) {
        *cursorBlockNeedingRepaint = repaintBlock;
        *cursorBlockOffset = cellPos;
    }

    if (bg.style() > Qt::SolidPattern)
        painter->setBrushOrigin(brushOrigin);
}

void QTextDocumentLayoutPrivate::drawFlow(const QPointF &offset, QPainter *painter, const QAbstractTextDocumentLayout::PaintContext &context,
                                          QTextFrame::Iterator it, const QList<QTextFrame *> &floats, QTextBlock *cursorBlockNeedingRepaint) const
{
    Q_Q(const QTextDocumentLayout);
    const bool inRootFrame = (!it.atEnd() && it.parentFrame() && it.parentFrame()->parentFrame() == nullptr);

    QVector<QCheckPoint>::ConstIterator lastVisibleCheckPoint = checkPoints.end();
    if (inRootFrame && context.clip.isValid()) {
        lastVisibleCheckPoint = std::lower_bound(checkPoints.begin(), checkPoints.end(), QFixed::fromReal(context.clip.bottom()));
    }

    QTextBlock previousBlock;
    QTextFrame *previousFrame = nullptr;

    for (; !it.atEnd(); ++it) {
        QTextFrame *c = it.currentFrame();

        if (inRootFrame && !checkPoints.isEmpty()) {
            int currentPosInDoc;
            if (c)
                currentPosInDoc = c->firstPosition();
            else
                currentPosInDoc = it.currentBlock().position();

            // if we're past what is already laid out then we're better off
            // not trying to draw things that may not be positioned correctly yet
            if (currentPosInDoc >= checkPoints.constLast().positionInFrame)
                break;

            if (lastVisibleCheckPoint != checkPoints.end()
                && context.clip.isValid()
                && currentPosInDoc >= lastVisibleCheckPoint->positionInFrame
               )
                break;
        }

        if (c)
            drawFrame(offset, painter, context, c);
        else {
            QAbstractTextDocumentLayout::PaintContext pc = context;
            if (isEmptyBlockAfterTable(it.currentBlock(), previousFrame))
                pc.selections.clear();
            drawBlock(offset, painter, pc, it.currentBlock(), inRootFrame);
        }

        // when entering a table and the previous block is empty
        // then layoutFlow 'hides' the block that just causes a
        // new line by positioning it /on/ the table border. as we
        // draw that block before the table itself the decoration
        // 'overpaints' the cursor and we need to paint it afterwards
        // again
        if (isEmptyBlockBeforeTable(previousBlock, previousBlock.blockFormat(), it)
            && previousBlock.contains(context.cursorPosition)
           ) {
            *cursorBlockNeedingRepaint = previousBlock;
        }

        previousBlock = it.currentBlock();
        previousFrame = c;
    }

    for (int i = 0; i < floats.count(); ++i) {
        QTextFrame *frame = floats.at(i);
        if (!isFrameFromInlineObject(frame)
            || frame->frameFormat().position() == QTextFrameFormat::InFlow)
            continue;

        const int pos = frame->firstPosition() - 1;
        QTextCharFormat format = const_cast<QTextDocumentLayout *>(q)->format(pos);
        QTextObjectInterface *handler = q->handlerForObject(format.objectType());
        if (handler) {
            QRectF rect = frameBoundingRectInternal(frame);
            handler->drawObject(painter, rect, document, pos, format);
        }
    }
}

void QTextDocumentLayoutPrivate::drawBlock(const QPointF &offset, QPainter *painter,
                                           const QAbstractTextDocumentLayout::PaintContext &context,
                                           const QTextBlock &bl, bool inRootFrame) const
{
    const QTextLayout *tl = bl.layout();
    QRectF r = tl->boundingRect();
    r.translate(offset + tl->position());
    if (!bl.isVisible() || (context.clip.isValid() && (r.bottom() < context.clip.y() || r.top() > context.clip.bottom())))
        return;
    qCDebug(lcDraw) << "drawBlock" << bl.position() << "at" << offset << "br" << tl->boundingRect();

    QTextBlockFormat blockFormat = bl.blockFormat();

    QBrush bg = blockFormat.background();
    if (bg != Qt::NoBrush) {
        QRectF rect = r;

        // extend the background rectangle if we're in the root frame with NoWrap,
        // as the rect of the text block will then be only the width of the text
        // instead of the full page width
        if (inRootFrame && document->pageSize().width() <= 0) {
            const QTextFrameData *fd = data(document->rootFrame());
            rect.setRight((fd->size.width - fd->rightMargin).toReal());
        }

        fillBackground(painter, rect, bg, r.topLeft());
    }

    QVector<QTextLayout::FormatRange> selections;
    int blpos = bl.position();
    int bllen = bl.length();
    const QTextCharFormat *selFormat = nullptr;
    for (int i = 0; i < context.selections.size(); ++i) {
        const QAbstractTextDocumentLayout::Selection &range = context.selections.at(i);
        const int selStart = range.cursor.selectionStart() - blpos;
        const int selEnd = range.cursor.selectionEnd() - blpos;
        if (selStart < bllen && selEnd > 0
             && selEnd > selStart) {
            QTextLayout::FormatRange o;
            o.start = selStart;
            o.length = selEnd - selStart;
            o.format = range.format;
            selections.append(o);
        } else if (! range.cursor.hasSelection() && range.format.hasProperty(QTextFormat::FullWidthSelection)
                   && bl.contains(range.cursor.position())) {
            // for full width selections we don't require an actual selection, just
            // a position to specify the line. that's more convenience in usage.
            QTextLayout::FormatRange o;
            QTextLine l = tl->lineForTextPosition(range.cursor.position() - blpos);
            o.start = l.textStart();
            o.length = l.textLength();
            if (o.start + o.length == bllen - 1)
                ++o.length; // include newline
            o.format = range.format;
            selections.append(o);
       }
        if (selStart < 0 && selEnd >= 1)
            selFormat = &range.format;
    }

    QTextObject *object = document->objectForFormat(bl.blockFormat());
    if (object && object->format().toListFormat().style() != QTextListFormat::ListStyleUndefined)
        drawListItem(offset, painter, context, bl, selFormat);

    QPen oldPen = painter->pen();
    painter->setPen(context.palette.color(QPalette::Text));

    tl->draw(painter, offset, selections, context.clip.isValid() ? (context.clip & clipRect) : clipRect);

    // if the block is empty and it precedes a table, do not draw the cursor.
    // the cursor is drawn later after the table has been drawn so no need
    // to draw it here.
    if (!isEmptyBlockBeforeTable(frameIteratorForTextPosition(blpos))
        && ((context.cursorPosition >= blpos && context.cursorPosition < blpos + bllen)
            || (context.cursorPosition < -1 && !tl->preeditAreaText().isEmpty()))) {
        int cpos = context.cursorPosition;
        if (cpos < -1)
            cpos = tl->preeditAreaPosition() - (cpos + 2);
        else
            cpos -= blpos;
        tl->drawCursor(painter, offset, cpos, cursorWidth);
    }

    if (blockFormat.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)) {
        const qreal width = blockFormat.lengthProperty(QTextFormat::BlockTrailingHorizontalRulerWidth).value(r.width());
        painter->setPen(context.palette.color(QPalette::Dark));
        qreal y = r.bottom();
        if (bl.length() == 1)
            y = r.top() + r.height() / 2;

        const qreal middleX = r.left() + r.width() / 2;
        painter->drawLine(QLineF(middleX - width / 2, y, middleX + width / 2, y));
    }

    painter->setPen(oldPen);
}


void QTextDocumentLayoutPrivate::drawListItem(const QPointF &offset, QPainter *painter,
                                              const QAbstractTextDocumentLayout::PaintContext &context,
                                              const QTextBlock &bl, const QTextCharFormat *selectionFormat) const
{
    Q_Q(const QTextDocumentLayout);
    const QTextBlockFormat blockFormat = bl.blockFormat();
    const QTextCharFormat charFormat = QTextCursor(bl).charFormat();
    QFont font(charFormat.font());
    if (q->paintDevice())
        font = QFont(font, q->paintDevice());

    const QFontMetrics fontMetrics(font);
    QTextObject * const object = document->objectForFormat(blockFormat);
    const QTextListFormat lf = object->format().toListFormat();
    int style = lf.style();
    QString itemText;
    QSizeF size;

    if (blockFormat.hasProperty(QTextFormat::ListStyle))
        style = QTextListFormat::Style(blockFormat.intProperty(QTextFormat::ListStyle));

    QTextLayout *layout = bl.layout();
    if (layout->lineCount() == 0)
        return;
    QTextLine firstLine = layout->lineAt(0);
    Q_ASSERT(firstLine.isValid());
    QPointF pos = (offset + layout->position()).toPoint();
    Qt::LayoutDirection dir = bl.textDirection();
    {
        QRectF textRect = firstLine.naturalTextRect();
        pos += textRect.topLeft().toPoint();
        if (dir == Qt::RightToLeft)
            pos.rx() += textRect.width();
    }

    switch (style) {
    case QTextListFormat::ListDecimal:
    case QTextListFormat::ListLowerAlpha:
    case QTextListFormat::ListUpperAlpha:
    case QTextListFormat::ListLowerRoman:
    case QTextListFormat::ListUpperRoman:
        itemText = static_cast<QTextList *>(object)->itemText(bl);
        size.setWidth(fontMetrics.horizontalAdvance(itemText));
        size.setHeight(fontMetrics.height());
        break;

    case QTextListFormat::ListSquare:
    case QTextListFormat::ListCircle:
    case QTextListFormat::ListDisc:
        size.setWidth(fontMetrics.lineSpacing() / 3);
        size.setHeight(size.width());
        break;

    case QTextListFormat::ListStyleUndefined:
        return;
    default: return;
    }

    QRectF r(pos, size);

    qreal xoff = fontMetrics.horizontalAdvance(QLatin1Char(' '));
    if (dir == Qt::LeftToRight)
        xoff = -xoff - size.width();
    r.translate( xoff, (fontMetrics.height() / 2) - (size.height() / 2));

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing);

    if (selectionFormat) {
        painter->setPen(QPen(selectionFormat->foreground(), 0));
        painter->fillRect(r, selectionFormat->background());
    } else {
        QBrush fg = charFormat.foreground();
        if (fg == Qt::NoBrush)
            fg = context.palette.text();
        painter->setPen(QPen(fg, 0));
    }

    QBrush brush = context.palette.brush(QPalette::Text);

    bool marker = bl.blockFormat().marker() != QTextBlockFormat::MarkerType::NoMarker;
    if (marker) {
        int adj = fontMetrics.lineSpacing() / 6;
        r.adjust(-adj, 0, -adj, 0);
        if (bl.blockFormat().marker() == QTextBlockFormat::MarkerType::Checked) {
            // ### Qt6: render with QStyle / PE_IndicatorCheckBox. We don't currently
            // have access to that here, because it would be a widget dependency.
            painter->setPen(QPen(painter->pen().color(), 2));
            painter->drawLine(r.topLeft(), r.bottomRight());
            painter->drawLine(r.topRight(), r.bottomLeft());
            painter->setPen(QPen(painter->pen().color(), 0));
        }
        painter->drawRect(r.adjusted(-adj, -adj, adj, adj));
    }

    switch (style) {
    case QTextListFormat::ListDecimal:
    case QTextListFormat::ListLowerAlpha:
    case QTextListFormat::ListUpperAlpha:
    case QTextListFormat::ListLowerRoman:
    case QTextListFormat::ListUpperRoman: {
        QTextLayout layout(itemText, font, q->paintDevice());
        layout.setCacheEnabled(true);
        QTextOption option(Qt::AlignLeft | Qt::AlignAbsolute);
        option.setTextDirection(dir);
        layout.setTextOption(option);
        layout.beginLayout();
        QTextLine line = layout.createLine();
        if (line.isValid())
            line.setLeadingIncluded(true);
        layout.endLayout();
        layout.draw(painter, QPointF(r.left(), pos.y()));
        break;
    }
    case QTextListFormat::ListSquare:
        if (!marker)
            painter->fillRect(r, brush);
        break;
    case QTextListFormat::ListCircle:
        if (!marker) {
            painter->setPen(QPen(brush, 0));
            painter->drawEllipse(r.translated(0.5, 0.5)); // pixel align for sharper rendering
        }
        break;
    case QTextListFormat::ListDisc:
        if (!marker) {
            painter->setBrush(brush);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(r);
        }
        break;
    case QTextListFormat::ListStyleUndefined:
        break;
    default:
        break;
    }

    painter->restore();
}

static QFixed flowPosition(const QTextFrame::iterator &it)
{
    if (it.atEnd())
        return 0;

    if (it.currentFrame()) {
        return data(it.currentFrame())->position.y;
    } else {
        QTextBlock block = it.currentBlock();
        QTextLayout *layout = block.layout();
        if (layout->lineCount() == 0)
            return QFixed::fromReal(layout->position().y());
        else
            return QFixed::fromReal(layout->position().y() + layout->lineAt(0).y());
    }
}

static QFixed firstChildPos(const QTextFrame *f)
{
    return flowPosition(f->begin());
}

QTextLayoutStruct QTextDocumentLayoutPrivate::layoutCell(QTextTable *t, const QTextTableCell &cell, QFixed width,
                                                        int layoutFrom, int layoutTo, QTextTableData *td,
                                                        QFixed absoluteTableY, bool withPageBreaks)
{
    qCDebug(lcTable) << "layoutCell";
    QTextLayoutStruct layoutStruct;
    layoutStruct.frame = t;
    layoutStruct.minimumWidth = 0;
    layoutStruct.maximumWidth = QFIXED_MAX;
    layoutStruct.y = 0;

    const QFixed topPadding = td->topPadding(t, cell);
    if (withPageBreaks) {
        layoutStruct.frameY = absoluteTableY + td->rowPositions.at(cell.row()) + topPadding;
    }
    layoutStruct.x_left = 0;
    layoutStruct.x_right = width;
    // we get called with different widths all the time (for example for figuring
    // out the min/max widths), so we always have to do the full layout ;(
    // also when for example in a table layoutFrom/layoutTo affect only one cell,
    // making that one cell grow the available width of the other cells may change
    // (shrink) and therefore when layoutCell gets called for them they have to
    // be re-laid out, even if layoutFrom/layoutTo is not in their range. Hence
    // this line:

    layoutStruct.pageHeight = QFixed::fromReal(document->pageSize().height());
    if (layoutStruct.pageHeight < 0 || !withPageBreaks)
        layoutStruct.pageHeight = QFIXED_MAX;
    const int currentPage = layoutStruct.currentPage();

    layoutStruct.pageTopMargin = td->effectiveTopMargin
            + td->cellSpacing
            + td->border
            + td->paddingProperty(cell.format(), QTextFormat::TableCellTopPadding); // top cell-border is not repeated

#ifndef QT_NO_CSSPARSER
    const int headerRowCount = t->format().headerRowCount();
    if (td->borderCollapse && headerRowCount > 0) {
        // consider the header row's bottom edge width
        qreal headerRowBottomBorderWidth = axisEdgeData(t, td, t->cellAt(headerRowCount - 1, cell.column()), QCss::BottomEdge).width;
        layoutStruct.pageTopMargin += QFixed::fromReal(scaleToDevice(headerRowBottomBorderWidth) / 2);
    }
#endif

    layoutStruct.pageBottomMargin = td->effectiveBottomMargin + td->cellSpacing + td->effectiveBottomBorder + td->bottomPadding(t, cell);
    layoutStruct.pageBottom = (currentPage + 1) * layoutStruct.pageHeight - layoutStruct.pageBottomMargin;

    layoutStruct.fullLayout = true;

    QFixed pageTop = currentPage * layoutStruct.pageHeight + layoutStruct.pageTopMargin - layoutStruct.frameY;
    layoutStruct.y = qMax(layoutStruct.y, pageTop);

    const QList<QTextFrame *> childFrames = td->childFrameMap.values(cell.row() + cell.column() * t->rows());
    for (int i = 0; i < childFrames.size(); ++i) {
        QTextFrame *frame = childFrames.at(i);
        QTextFrameData *cd = data(frame);
        cd->sizeDirty = true;
    }

    layoutFlow(cell.begin(), &layoutStruct, layoutFrom, layoutTo, width);

    QFixed floatMinWidth;

    // floats that are located inside the text (like inline images) aren't taken into account by
    // layoutFlow with regards to the cell height (layoutStruct->y), so for a safety measure we
    // do that here. For example with <td><img align="right" src="..." />blah</td>
    // when the image happens to be higher than the text
    for (int i = 0; i < childFrames.size(); ++i) {
        QTextFrame *frame = childFrames.at(i);
        QTextFrameData *cd = data(frame);

        if (frame->frameFormat().position() != QTextFrameFormat::InFlow)
            layoutStruct.y = qMax(layoutStruct.y, cd->position.y + cd->size.height);

        floatMinWidth = qMax(floatMinWidth, cd->minimumWidth);
    }

    // constraint the maximumWidth by the minimum width of the fixed size floats, to
    // keep them visible
    layoutStruct.maximumWidth = qMax(layoutStruct.maximumWidth, floatMinWidth);

    // as floats in cells get added to the table's float list but must not affect
    // floats in other cells we must clear the list here.
    data(t)->floats.clear();

//    qDebug("layoutCell done");

    return layoutStruct;
}

#ifndef QT_NO_CSSPARSER
static inline void findWidestOutermostBorder(QTextTable *table, QTextTableData *td,
                                             const QTextTableCell &cell, QCss::Edge edge,
                                             qreal *outerBorders)
{
    EdgeData w = cellEdgeData(table, td, cell, edge);
    if (w.width > outerBorders[edge])
        outerBorders[edge] = w.width;
}
#endif

QRectF QTextDocumentLayoutPrivate::layoutTable(QTextTable *table, int layoutFrom, int layoutTo, QFixed parentY)
{
    qCDebug(lcTable) << "layoutTable from" << layoutFrom << "to" << layoutTo << "parentY" << parentY;
    QTextTableData *td = static_cast<QTextTableData *>(data(table));
    Q_ASSERT(td->sizeDirty);
    const int rows = table->rows();
    const int columns = table->columns();

    const QTextTableFormat fmt = table->format();

    td->childFrameMap.clear();
    {
        const QList<QTextFrame *> children = table->childFrames();
        for (int i = 0; i < children.count(); ++i) {
            QTextFrame *frame = children.at(i);
            QTextTableCell cell = table->cellAt(frame->firstPosition());
            td->childFrameMap.insert(cell.row() + cell.column() * rows, frame);
        }
    }

    QVector<QTextLength> columnWidthConstraints = fmt.columnWidthConstraints();
    if (columnWidthConstraints.size() != columns)
        columnWidthConstraints.resize(columns);
    Q_ASSERT(columnWidthConstraints.count() == columns);

    // borderCollapse will disable drawing the html4 style table cell borders
    // and draw a 1px grid instead. This also sets a fixed cellspacing
    // of 1px if border > 0 (for the grid) and ignore any explicitly set
    // cellspacing.
    td->borderCollapse = fmt.borderCollapse();
    td->borderCell = td->borderCollapse ? 0 : td->border;
    const QFixed cellSpacing = td->cellSpacing = QFixed::fromReal(scaleToDevice(td->borderCollapse ? 0 : fmt.cellSpacing())).round();

    td->drawGrid = (td->borderCollapse && fmt.border() >= 1);

    td->effectiveTopBorder = td->effectiveBottomBorder = td->effectiveLeftBorder = td->effectiveRightBorder = td->border;

#ifndef QT_NO_CSSPARSER
    if (td->borderCollapse) {
        // find the widest borders of the outermost cells
        qreal outerBorders[QCss::NumEdges];
        for (int i = 0; i < QCss::NumEdges; ++i)
            outerBorders[i] = 0;

        for (int r = 0; r < rows; ++r) {
            if (r == 0) {
                for (int c = 0; c < columns; ++c)
                    findWidestOutermostBorder(table, td, table->cellAt(r, c), QCss::TopEdge, outerBorders);
            }
            if (r == rows - 1) {
                for (int c = 0; c < columns; ++c)
                    findWidestOutermostBorder(table, td, table->cellAt(r, c), QCss::BottomEdge, outerBorders);
            }
            findWidestOutermostBorder(table, td, table->cellAt(r, 0), QCss::LeftEdge, outerBorders);
            findWidestOutermostBorder(table, td, table->cellAt(r, columns - 1), QCss::RightEdge, outerBorders);
        }
        td->effectiveTopBorder = QFixed::fromReal(scaleToDevice(outerBorders[QCss::TopEdge] / 2)).round();
        td->effectiveBottomBorder = QFixed::fromReal(scaleToDevice(outerBorders[QCss::BottomEdge] / 2)).round();
        td->effectiveLeftBorder = QFixed::fromReal(scaleToDevice(outerBorders[QCss::LeftEdge] / 2)).round();
        td->effectiveRightBorder = QFixed::fromReal(scaleToDevice(outerBorders[QCss::RightEdge] / 2)).round();
    }
#endif

    td->deviceScale = scaleToDevice(qreal(1));
    td->cellPadding = QFixed::fromReal(scaleToDevice(fmt.cellPadding()));
    const QFixed leftMargin = td->leftMargin + td->padding + td->effectiveLeftBorder;
    const QFixed rightMargin = td->rightMargin + td->padding + td->effectiveRightBorder;
    const QFixed topMargin = td->topMargin + td->padding + td->effectiveTopBorder;

    const QFixed absoluteTableY = parentY + td->position.y;

    const QTextOption::WrapMode oldDefaultWrapMode = docPrivate->defaultTextOption.wrapMode();

recalc_minmax_widths:

    QFixed remainingWidth = td->contentsWidth;
    // two (vertical) borders per cell per column
    remainingWidth -= columns * 2 * td->borderCell;
    // inter-cell spacing
    remainingWidth -= (columns - 1) * cellSpacing;
    // cell spacing at the left and right hand side
    remainingWidth -= 2 * cellSpacing;

    if (td->borderCollapse) {
        remainingWidth -= td->effectiveLeftBorder;
        remainingWidth -= td->effectiveRightBorder;
    }

    // remember the width used to distribute to percentaged columns
    const QFixed initialTotalWidth = remainingWidth;

    td->widths.resize(columns);
    td->widths.fill(0);

    td->minWidths.resize(columns);
    // start with a minimum width of 0. totally empty
    // cells of default created tables are invisible otherwise
    // and therefore hardly editable
    td->minWidths.fill(1);

    td->maxWidths.resize(columns);
    td->maxWidths.fill(QFIXED_MAX);

    // calculate minimum and maximum sizes of the columns
    for (int i = 0; i < columns; ++i) {
        for (int row = 0; row < rows; ++row) {
            const QTextTableCell cell = table->cellAt(row, i);
            const int cspan = cell.columnSpan();

            if (cspan > 1 && i != cell.column())
                continue;

            const QFixed leftPadding = td->leftPadding(table, cell);
            const QFixed rightPadding = td->rightPadding(table, cell);
            const QFixed widthPadding = leftPadding + rightPadding;

            // to figure out the min and the max width lay out the cell at
            // maximum width. otherwise the maxwidth calculation sometimes
            // returns wrong values
            QTextLayoutStruct layoutStruct = layoutCell(table, cell, QFIXED_MAX, layoutFrom,
                                                        layoutTo, td, absoluteTableY,
                                                        /*withPageBreaks =*/false);

            // distribute the minimum width over all columns the cell spans
            QFixed widthToDistribute = layoutStruct.minimumWidth + widthPadding;
            for (int n = 0; n < cspan; ++n) {
                const int col = i + n;
                QFixed w = widthToDistribute / (cspan - n);
                // ceil to avoid going below minWidth when rounding all column widths later
                td->minWidths[col] = qMax(td->minWidths.at(col), w).ceil();
                widthToDistribute -= td->minWidths.at(col);
                if (widthToDistribute <= 0)
                    break;
            }

            QFixed maxW = td->maxWidths.at(i);
            if (layoutStruct.maximumWidth != QFIXED_MAX) {
                if (maxW == QFIXED_MAX)
                    maxW = layoutStruct.maximumWidth + widthPadding;
                else
                    maxW = qMax(maxW, layoutStruct.maximumWidth + widthPadding);
            }
            if (maxW == QFIXED_MAX)
                continue;

            // for variable columns the maxWidth will later be considered as the
            // column width (column width = content width). We must avoid that the
            // pixel-alignment rounding step floors this value and thus the text
            // rendering later erroneously wraps the content.
            maxW = maxW.ceil();

            widthToDistribute = maxW;
            for (int n = 0; n < cspan; ++n) {
                const int col = i + n;
                QFixed w = widthToDistribute / (cspan - n);
                td->maxWidths[col] = qMax(td->minWidths.at(col), w);
                widthToDistribute -= td->maxWidths.at(col);
                if (widthToDistribute <= 0)
                    break;
            }
        }
    }

    // set fixed values, figure out total percentages used and number of
    // variable length cells. Also assign the minimum width for variable columns.
    QFixed totalPercentage;
    int variableCols = 0;
    QFixed totalMinWidth = 0;
    for (int i = 0; i < columns; ++i) {
        const QTextLength &length = columnWidthConstraints.at(i);
        if (length.type() == QTextLength::FixedLength) {
            td->minWidths[i] = td->widths[i] = qMax(scaleToDevice(QFixed::fromReal(length.rawValue())), td->minWidths.at(i));
            remainingWidth -= td->widths.at(i);
            qCDebug(lcTable) << "column" << i << "has width constraint" << td->minWidths.at(i) << "px, remaining width now" << remainingWidth;
        } else if (length.type() == QTextLength::PercentageLength) {
            totalPercentage += QFixed::fromReal(length.rawValue());
        } else if (length.type() == QTextLength::VariableLength) {
            variableCols++;

            td->widths[i] = td->minWidths.at(i);
            remainingWidth -= td->minWidths.at(i);
            qCDebug(lcTable) << "column" << i << "has variable width, min" << td->minWidths.at(i) << "remaining width now" << remainingWidth;
        }
        totalMinWidth += td->minWidths.at(i);
    }

    // set percentage values
    {
        const QFixed totalPercentagedWidth = initialTotalWidth * totalPercentage / 100;
        QFixed remainingMinWidths = totalMinWidth;
        for (int i = 0; i < columns; ++i) {
            remainingMinWidths -= td->minWidths.at(i);
            if (columnWidthConstraints.at(i).type() == QTextLength::PercentageLength) {
                const QFixed allottedPercentage = QFixed::fromReal(columnWidthConstraints.at(i).rawValue());

                const QFixed percentWidth = totalPercentagedWidth * allottedPercentage / totalPercentage;
                if (percentWidth >= td->minWidths.at(i)) {
                    td->widths[i] = qBound(td->minWidths.at(i), percentWidth, remainingWidth - remainingMinWidths);
                } else {
                    td->widths[i] = td->minWidths.at(i);
                }
                qCDebug(lcTable) << "column" << i << "has width constraint" << columnWidthConstraints.at(i).rawValue()
                                 << "%, allocated width" << td->widths[i] << "remaining width now" << remainingWidth;
                remainingWidth -= td->widths.at(i);
            }
        }
    }

    // for variable columns distribute the remaining space
    if (variableCols > 0 && remainingWidth > 0) {
        QVarLengthArray<int> columnsWithProperMaxSize;
        for (int i = 0; i < columns; ++i)
            if (columnWidthConstraints.at(i).type() == QTextLength::VariableLength
                && td->maxWidths.at(i) != QFIXED_MAX)
                columnsWithProperMaxSize.append(i);

        QFixed lastRemainingWidth = remainingWidth;
        while (remainingWidth > 0) {
            for (int k = 0; k < columnsWithProperMaxSize.count(); ++k) {
                const int col = columnsWithProperMaxSize[k];
                const int colsLeft = columnsWithProperMaxSize.count() - k;
                const QFixed w = qMin(td->maxWidths.at(col) - td->widths.at(col), remainingWidth / colsLeft);
                td->widths[col] += w;
                remainingWidth -= w;
            }
            if (remainingWidth == lastRemainingWidth)
                break;
            lastRemainingWidth = remainingWidth;
        }

        if (remainingWidth > 0
            // don't unnecessarily grow variable length sized tables
            && fmt.width().type() != QTextLength::VariableLength) {
            const QFixed widthPerAnySizedCol = remainingWidth / variableCols;
            for (int col = 0; col < columns; ++col) {
                if (columnWidthConstraints.at(col).type() == QTextLength::VariableLength)
                    td->widths[col] += widthPerAnySizedCol;
            }
        }
    }

    // in order to get a correct border rendering we must ensure that the distance between
    // two cells is exactly 2 * td->cellBorder pixel. we do this by rounding the calculated width
    // values here.
    // to minimize the total rounding error we propagate the rounding error for each width
    // to its successor.
    QFixed error = 0;
    for (int i = 0; i < columns; ++i) {
        QFixed orig = td->widths[i];
        td->widths[i] = (td->widths[i] - error).round();
        error = td->widths[i] - orig;
    }

    td->columnPositions.resize(columns);
    td->columnPositions[0] = leftMargin /*includes table border*/ + cellSpacing + td->border;

    for (int i = 1; i < columns; ++i)
        td->columnPositions[i] = td->columnPositions.at(i-1) + td->widths.at(i-1) + 2 * td->borderCell + cellSpacing;

    // - margin to compensate the + margin in columnPositions[0]
    const QFixed contentsWidth = td->columnPositions.constLast() + td->widths.constLast() + td->padding + td->border + cellSpacing - leftMargin;

    // if the table is too big and causes an overflow re-do the layout with WrapAnywhere as wrap
    // mode
    if (docPrivate->defaultTextOption.wrapMode() == QTextOption::WrapAtWordBoundaryOrAnywhere
        && contentsWidth > td->contentsWidth) {
        docPrivate->defaultTextOption.setWrapMode(QTextOption::WrapAnywhere);
        // go back to the top of the function
        goto recalc_minmax_widths;
    }

    td->contentsWidth = contentsWidth;

    docPrivate->defaultTextOption.setWrapMode(oldDefaultWrapMode);

    td->heights.resize(rows);
    td->heights.fill(0);

    td->rowPositions.resize(rows);
    td->rowPositions[0] = topMargin /*includes table border*/ + cellSpacing + td->border;

    bool haveRowSpannedCells = false;

    // need to keep track of cell heights for vertical alignment
    QVector<QFixed> cellHeights;
    cellHeights.reserve(rows * columns);

    QFixed pageHeight = QFixed::fromReal(document->pageSize().height());
    if (pageHeight <= 0)
        pageHeight = QFIXED_MAX;

    QVector<QFixed> heightToDistribute;
    heightToDistribute.resize(columns);

    td->headerHeight = 0;
    const int headerRowCount = qMin(table->format().headerRowCount(), rows - 1);
    const QFixed originalTopMargin = td->effectiveTopMargin;
    bool hasDroppedTable = false;

    // now that we have the column widths we can lay out all cells with the right width.
    // spanning cells are only allowed to grow the last row spanned by the cell.
    //
    // ### this could be made faster by iterating over the cells array of QTextTable
    for (int r = 0; r < rows; ++r) {
        td->calcRowPosition(r);

        const int tableStartPage = (absoluteTableY / pageHeight).truncate();
        const int currentPage = ((td->rowPositions.at(r) + absoluteTableY) / pageHeight).truncate();
        const QFixed pageBottom = (currentPage + 1) * pageHeight - td->effectiveBottomMargin - absoluteTableY - cellSpacing - td->border;
        const QFixed pageTop = currentPage * pageHeight + td->effectiveTopMargin - absoluteTableY + cellSpacing + td->border;
        const QFixed nextPageTop = pageTop + pageHeight;

        if (td->rowPositions.at(r) > pageBottom)
            td->rowPositions[r] = nextPageTop;
        else if (td->rowPositions.at(r) < pageTop)
            td->rowPositions[r] = pageTop;

        bool dropRowToNextPage = true;
        int cellCountBeforeRow = cellHeights.size();

        // if we drop the row to the next page we need to subtract the drop
        // distance from any row spanning cells
        QFixed dropDistance = 0;

relayout:
        const int rowStartPage = ((td->rowPositions.at(r) + absoluteTableY) / pageHeight).truncate();
        // if any of the header rows or the first non-header row start on the next page
        // then the entire header should be dropped
        if (r <= headerRowCount && rowStartPage > tableStartPage && !hasDroppedTable) {
            td->rowPositions[0] = nextPageTop;
            cellHeights.clear();
            td->effectiveTopMargin = originalTopMargin;
            hasDroppedTable = true;
            r = -1;
            continue;
        }

        int rowCellCount = 0;
        for (int c = 0; c < columns; ++c) {
            QTextTableCell cell = table->cellAt(r, c);
            const int rspan = cell.rowSpan();
            const int cspan = cell.columnSpan();

            if (cspan > 1 && cell.column() != c)
                continue;

            if (rspan > 1) {
                haveRowSpannedCells = true;

                const int cellRow = cell.row();
                if (cellRow != r) {
                    // the last row gets all the remaining space
                    if (cellRow + rspan - 1 == r)
                        td->heights[r] = qMax(td->heights.at(r), heightToDistribute.at(c) - dropDistance).round();
                    continue;
                }
            }

            const QFixed topPadding = td->topPadding(table, cell);
            const QFixed bottomPadding = td->bottomPadding(table, cell);
            const QFixed leftPadding = td->leftPadding(table, cell);
            const QFixed rightPadding = td->rightPadding(table, cell);
            const QFixed widthPadding = leftPadding + rightPadding;

            ++rowCellCount;

            const QFixed width = td->cellWidth(c, cspan) - widthPadding;
            QTextLayoutStruct layoutStruct = layoutCell(table, cell, width,
                                                       layoutFrom, layoutTo,
                                                       td, absoluteTableY,
                                                       /*withPageBreaks =*/true);

            const QFixed height = (layoutStruct.y + bottomPadding + topPadding).round();

            if (rspan > 1)
                heightToDistribute[c] = height + dropDistance;
            else
                td->heights[r] = qMax(td->heights.at(r), height);

            cellHeights.append(layoutStruct.y);

            QFixed childPos = td->rowPositions.at(r) + topPadding + flowPosition(cell.begin());
            if (childPos < pageBottom)
                dropRowToNextPage = false;
        }

        if (rowCellCount > 0 && dropRowToNextPage) {
            dropDistance = nextPageTop - td->rowPositions.at(r);
            td->rowPositions[r] = nextPageTop;
            td->heights[r] = 0;
            dropRowToNextPage = false;
            cellHeights.resize(cellCountBeforeRow);
            if (r > headerRowCount)
                td->heights[r - 1] = pageBottom - td->rowPositions.at(r - 1);
            goto relayout;
        }

        if (haveRowSpannedCells) {
            const QFixed effectiveHeight = td->heights.at(r) + td->borderCell + cellSpacing + td->borderCell;
            for (int c = 0; c < columns; ++c)
                heightToDistribute[c] = qMax(heightToDistribute.at(c) - effectiveHeight - dropDistance, QFixed(0));
        }

        if (r == headerRowCount - 1) {
            td->headerHeight = td->rowPositions.at(r) + td->heights.at(r) - td->rowPositions.at(0) + td->cellSpacing + 2 * td->borderCell;
            td->headerHeight -= td->headerHeight * (td->headerHeight / pageHeight).truncate();
            td->effectiveTopMargin += td->headerHeight;
        }
    }

    td->effectiveTopMargin = originalTopMargin;

    // now that all cells have been properly laid out, we can compute the
    // vertical offsets for vertical alignment
    td->cellVerticalOffsets.resize(rows * columns);
    int cellIndex = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            QTextTableCell cell = table->cellAt(r, c);
            if (cell.row() != r || cell.column() != c)
                continue;

            const int rowSpan = cell.rowSpan();
            const QFixed availableHeight = td->rowPositions.at(r + rowSpan - 1) + td->heights.at(r + rowSpan - 1) - td->rowPositions.at(r);

            const QTextCharFormat cellFormat = cell.format();
            const QFixed cellHeight = cellHeights.at(cellIndex++) + td->topPadding(table, cell) + td->bottomPadding(table, cell);

            QFixed offset = 0;
            switch (cellFormat.verticalAlignment()) {
            case QTextCharFormat::AlignMiddle:
                offset = (availableHeight - cellHeight) / 2;
                break;
            case QTextCharFormat::AlignBottom:
                offset = availableHeight - cellHeight;
                break;
            default:
                break;
            };

            for (int rd = 0; rd < cell.rowSpan(); ++rd) {
                for (int cd = 0; cd < cell.columnSpan(); ++cd) {
                    const int index = (c + cd) + (r + rd) * columns;
                    td->cellVerticalOffsets[index] = offset;
                }
            }
        }
    }

    td->minimumWidth = td->columnPositions.at(0);
    for (int i = 0; i < columns; ++i) {
        td->minimumWidth += td->minWidths.at(i) + 2 * td->borderCell + cellSpacing;
    }
    td->minimumWidth += rightMargin - td->border;

    td->maximumWidth = td->columnPositions.at(0);
    for (int i = 0; i < columns; ++i) {
        if (td->maxWidths.at(i) != QFIXED_MAX)
            td->maximumWidth += td->maxWidths.at(i) + 2 * td->borderCell + cellSpacing;
        qCDebug(lcTable) << "column" << i << "has final width" << td->widths.at(i).toReal()
                         << "min" << td->minWidths.at(i).toReal() << "max" << td->maxWidths.at(i).toReal();
    }
    td->maximumWidth += rightMargin - td->border;

    td->updateTableSize();
    td->sizeDirty = false;
    return QRectF(); // invalid rect -> update everything
}

void QTextDocumentLayoutPrivate::positionFloat(QTextFrame *frame, QTextLine *currentLine)
{
    QTextFrameData *fd = data(frame);

    QTextFrame *parent = frame->parentFrame();
    Q_ASSERT(parent);
    QTextFrameData *pd = data(parent);
    Q_ASSERT(pd && pd->currentLayoutStruct);

    QTextLayoutStruct *layoutStruct = pd->currentLayoutStruct;

    if (!pd->floats.contains(frame))
        pd->floats.append(frame);
    fd->layoutDirty = true;
    Q_ASSERT(!fd->sizeDirty);

//     qDebug() << "positionFloat:" << frame << "width=" << fd->size.width;
    QFixed y = layoutStruct->y;
    if (currentLine) {
        QFixed left, right;
        floatMargins(y, layoutStruct, &left, &right);
//         qDebug() << "have line: right=" << right << "left=" << left << "textWidth=" << currentLine->width();
        if (right - left < QFixed::fromReal(currentLine->naturalTextWidth()) + fd->size.width) {
            layoutStruct->pendingFloats.append(frame);
//             qDebug("    adding to pending list");
            return;
        }
    }

    bool frameSpansIntoNextPage = (y + layoutStruct->frameY + fd->size.height > layoutStruct->pageBottom);
    if (frameSpansIntoNextPage && fd->size.height <= layoutStruct->pageHeight) {
        layoutStruct->newPage();
        y = layoutStruct->y;

        frameSpansIntoNextPage = false;
    }

    y = findY(y, layoutStruct, fd->size.width);

    QFixed left, right;
    floatMargins(y, layoutStruct, &left, &right);

    if (frame->frameFormat().position() == QTextFrameFormat::FloatLeft) {
        fd->position.x = left;
        fd->position.y = y;
    } else {
        fd->position.x = right - fd->size.width;
        fd->position.y = y;
    }

    layoutStruct->minimumWidth = qMax(layoutStruct->minimumWidth, fd->minimumWidth);
    layoutStruct->maximumWidth = qMin(layoutStruct->maximumWidth, fd->maximumWidth);

//     qDebug()<< "float positioned at " << fd->position.x << fd->position.y;
    fd->layoutDirty = false;

    // If the frame is a table, then positioning it will affect the size if it covers more than
    // one page, because of page breaks and repeating the header.
    if (qobject_cast<QTextTable *>(frame) != 0)
        fd->sizeDirty = frameSpansIntoNextPage;
}

QRectF QTextDocumentLayoutPrivate::layoutFrame(QTextFrame *f, int layoutFrom, int layoutTo, QFixed parentY)
{
    qCDebug(lcLayout, "layoutFrame (%d--%d), parent=%p", f->firstPosition(), f->lastPosition(), f->parentFrame());
    Q_ASSERT(data(f)->sizeDirty);

    QTextFrameFormat fformat = f->frameFormat();

    QTextFrame *parent = f->parentFrame();
    const QTextFrameData *pd = parent ? data(parent) : nullptr;

    const qreal maximumWidth = qMax(qreal(0), pd ? pd->contentsWidth.toReal() : document->pageSize().width());
    QFixed width = QFixed::fromReal(fformat.width().value(maximumWidth));
    if (fformat.width().type() == QTextLength::FixedLength)
        width = scaleToDevice(width);

    const QFixed maximumHeight = pd ? pd->contentsHeight : -1;
    const QFixed height = (maximumHeight != -1 || fformat.height().type() != QTextLength::PercentageLength)
                            ? QFixed::fromReal(fformat.height().value(maximumHeight.toReal()))
                            : -1;

    return layoutFrame(f, layoutFrom, layoutTo, width, height, parentY);
}

QRectF QTextDocumentLayoutPrivate::layoutFrame(QTextFrame *f, int layoutFrom, int layoutTo, QFixed frameWidth, QFixed frameHeight, QFixed parentY)
{
    qCDebug(lcLayout, "layoutFrame (%d--%d), parent=%p", f->firstPosition(), f->lastPosition(), f->parentFrame());
    Q_ASSERT(data(f)->sizeDirty);

    QTextFrameData *fd = data(f);
    QFixed newContentsWidth;

    bool fullLayout = (f == document->rootFrame() && !fd->fullLayoutCompleted);
    {
        QTextFrameFormat fformat = f->frameFormat();
        // set sizes of this frame from the format
        QFixed tm = QFixed::fromReal(scaleToDevice(fformat.topMargin())).round();
        if (tm != fd->topMargin) {
            fd->topMargin = tm;
            fullLayout = true;
        }
        QFixed bm = QFixed::fromReal(scaleToDevice(fformat.bottomMargin())).round();
        if (bm != fd->bottomMargin) {
            fd->bottomMargin = bm;
            fullLayout = true;
        }
        fd->leftMargin = QFixed::fromReal(scaleToDevice(fformat.leftMargin())).round();
        fd->rightMargin = QFixed::fromReal(scaleToDevice(fformat.rightMargin())).round();
        QFixed b = QFixed::fromReal(scaleToDevice(fformat.border())).round();
        if (b != fd->border) {
            fd->border = b;
            fullLayout = true;
        }
        QFixed p = QFixed::fromReal(scaleToDevice(fformat.padding())).round();
        if (p != fd->padding) {
            fd->padding = p;
            fullLayout = true;
        }

        QTextFrame *parent = f->parentFrame();
        const QTextFrameData *pd = parent ? data(parent) : nullptr;

        // accumulate top and bottom margins
        if (parent) {
            fd->effectiveTopMargin = pd->effectiveTopMargin + fd->topMargin + fd->border + fd->padding;
            fd->effectiveBottomMargin = pd->effectiveBottomMargin + fd->topMargin + fd->border + fd->padding;

            if (qobject_cast<QTextTable *>(parent)) {
                const QTextTableData *td = static_cast<const QTextTableData *>(pd);
                fd->effectiveTopMargin += td->cellSpacing + td->border + td->cellPadding;
                fd->effectiveBottomMargin += td->cellSpacing + td->border + td->cellPadding;
            }
        } else {
            fd->effectiveTopMargin = fd->topMargin + fd->border + fd->padding;
            fd->effectiveBottomMargin = fd->bottomMargin + fd->border + fd->padding;
        }

        newContentsWidth = frameWidth - 2*(fd->border + fd->padding)
                           - fd->leftMargin - fd->rightMargin;

        if (frameHeight != -1) {
            fd->contentsHeight = frameHeight - 2*(fd->border + fd->padding)
                                 - fd->topMargin - fd->bottomMargin;
        } else {
            fd->contentsHeight = frameHeight;
        }
    }

    if (isFrameFromInlineObject(f)) {
        // never reached, handled in resizeInlineObject/positionFloat instead
        return QRectF();
    }

    if (QTextTable *table = qobject_cast<QTextTable *>(f)) {
        fd->contentsWidth = newContentsWidth;
        return layoutTable(table, layoutFrom, layoutTo, parentY);
    }

    // set fd->contentsWidth temporarily, so that layoutFrame for the children
    // picks the right width. We'll initialize it properly at the end of this
    // function.
    fd->contentsWidth = newContentsWidth;

    QTextLayoutStruct layoutStruct;
    layoutStruct.frame = f;
    layoutStruct.x_left = fd->leftMargin + fd->border + fd->padding;
    layoutStruct.x_right = layoutStruct.x_left + newContentsWidth;
    layoutStruct.y = fd->topMargin + fd->border + fd->padding;
    layoutStruct.frameY = parentY + fd->position.y;
    layoutStruct.contentsWidth = 0;
    layoutStruct.minimumWidth = 0;
    layoutStruct.maximumWidth = QFIXED_MAX;
    layoutStruct.fullLayout = fullLayout || (fd->oldContentsWidth != newContentsWidth);
    layoutStruct.updateRect = QRectF(QPointF(0, 0), QSizeF(qreal(INT_MAX), qreal(INT_MAX)));
    qCDebug(lcLayout) << "layoutStruct: x_left" << layoutStruct.x_left << "x_right" << layoutStruct.x_right
                      << "fullLayout" << layoutStruct.fullLayout;
    fd->oldContentsWidth = newContentsWidth;

    layoutStruct.pageHeight = QFixed::fromReal(document->pageSize().height());
    if (layoutStruct.pageHeight < 0)
        layoutStruct.pageHeight = QFIXED_MAX;

    const int currentPage = layoutStruct.pageHeight == 0 ? 0 : (layoutStruct.frameY / layoutStruct.pageHeight).truncate();
    layoutStruct.pageTopMargin = fd->effectiveTopMargin;
    layoutStruct.pageBottomMargin = fd->effectiveBottomMargin;
    layoutStruct.pageBottom = (currentPage + 1) * layoutStruct.pageHeight - layoutStruct.pageBottomMargin;

    if (!f->parentFrame())
        idealWidth = 0; // reset

    QTextFrame::Iterator it = f->begin();
    layoutFlow(it, &layoutStruct, layoutFrom, layoutTo);

    QFixed maxChildFrameWidth = 0;
    QList<QTextFrame *> children = f->childFrames();
    for (int i = 0; i < children.size(); ++i) {
        QTextFrame *c = children.at(i);
        QTextFrameData *cd = data(c);
        maxChildFrameWidth = qMax(maxChildFrameWidth, cd->size.width);
    }

    const QFixed marginWidth = 2*(fd->border + fd->padding) + fd->leftMargin + fd->rightMargin;
    if (!f->parentFrame()) {
        idealWidth = qMax(maxChildFrameWidth, layoutStruct.contentsWidth).toReal();
        idealWidth += marginWidth.toReal();
    }

    QFixed actualWidth = qMax(newContentsWidth, qMax(maxChildFrameWidth, layoutStruct.contentsWidth));
    fd->contentsWidth = actualWidth;
    if (newContentsWidth <= 0) { // nowrap layout?
        fd->contentsWidth = newContentsWidth;
    }

    fd->minimumWidth = layoutStruct.minimumWidth;
    fd->maximumWidth = layoutStruct.maximumWidth;

    fd->size.height = fd->contentsHeight == -1
                 ? layoutStruct.y + fd->border + fd->padding + fd->bottomMargin
                 : fd->contentsHeight + 2*(fd->border + fd->padding) + fd->topMargin + fd->bottomMargin;
    fd->size.width = actualWidth + marginWidth;
    fd->sizeDirty = false;
    if (layoutStruct.updateRectForFloats.isValid())
        layoutStruct.updateRect |= layoutStruct.updateRectForFloats;
    return layoutStruct.updateRect;
}

void QTextDocumentLayoutPrivate::layoutFlow(QTextFrame::Iterator it, QTextLayoutStruct *layoutStruct,
                                            int layoutFrom, int layoutTo, QFixed width)
{
    qCDebug(lcLayout) << "layoutFlow from=" << layoutFrom << "to=" << layoutTo;
    QTextFrameData *fd = data(layoutStruct->frame);

    fd->currentLayoutStruct = layoutStruct;

    QTextFrame::Iterator previousIt;

    const bool inRootFrame = (it.parentFrame() == document->rootFrame());
    if (inRootFrame) {
        bool redoCheckPoints = layoutStruct->fullLayout || checkPoints.isEmpty();

        if (!redoCheckPoints) {
            QVector<QCheckPoint>::Iterator checkPoint = std::lower_bound(checkPoints.begin(), checkPoints.end(), layoutFrom);
            if (checkPoint != checkPoints.end()) {
                if (checkPoint != checkPoints.begin())
                    --checkPoint;

                layoutStruct->y = checkPoint->y;
                layoutStruct->frameY = checkPoint->frameY;
                layoutStruct->minimumWidth = checkPoint->minimumWidth;
                layoutStruct->maximumWidth = checkPoint->maximumWidth;
                layoutStruct->contentsWidth = checkPoint->contentsWidth;

                if (layoutStruct->pageHeight > 0) {
                    int page = layoutStruct->currentPage();
                    layoutStruct->pageBottom = (page + 1) * layoutStruct->pageHeight - layoutStruct->pageBottomMargin;
                }

                it = frameIteratorForTextPosition(checkPoint->positionInFrame);
                checkPoints.resize(checkPoint - checkPoints.begin() + 1);

                if (checkPoint != checkPoints.begin()) {
                    previousIt = it;
                    --previousIt;
                }
            } else {
                redoCheckPoints = true;
            }
        }

        if (redoCheckPoints) {
            checkPoints.clear();
            QCheckPoint cp;
            cp.y = layoutStruct->y;
            cp.frameY = layoutStruct->frameY;
            cp.positionInFrame = 0;
            cp.minimumWidth = layoutStruct->minimumWidth;
            cp.maximumWidth = layoutStruct->maximumWidth;
            cp.contentsWidth = layoutStruct->contentsWidth;
            checkPoints.append(cp);
        }
    }

    QTextBlockFormat previousBlockFormat = previousIt.currentBlock().blockFormat();

    QFixed maximumBlockWidth = 0;
    while (!it.atEnd()) {
        QTextFrame *c = it.currentFrame();

        int docPos;
        if (it.currentFrame())
            docPos = it.currentFrame()->firstPosition();
        else
            docPos = it.currentBlock().position();

        if (inRootFrame) {
            if (qAbs(layoutStruct->y - checkPoints.constLast().y) > 2000) {
                QFixed left, right;
                floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                if (left == layoutStruct->x_left && right == layoutStruct->x_right) {
                    QCheckPoint p;
                    p.y = layoutStruct->y;
                    p.frameY = layoutStruct->frameY;
                    p.positionInFrame = docPos;
                    p.minimumWidth = layoutStruct->minimumWidth;
                    p.maximumWidth = layoutStruct->maximumWidth;
                    p.contentsWidth = layoutStruct->contentsWidth;
                    checkPoints.append(p);

                    if (currentLazyLayoutPosition != -1
                        && docPos > currentLazyLayoutPosition + lazyLayoutStepSize)
                        break;

                }
            }
        }

        if (c) {
            // position child frame
            QTextFrameData *cd = data(c);

            QTextFrameFormat fformat = c->frameFormat();

            if (fformat.position() == QTextFrameFormat::InFlow) {
                if (fformat.pageBreakPolicy() & QTextFormat::PageBreak_AlwaysBefore)
                    layoutStruct->newPage();

                QFixed left, right;
                floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                left = qMax(left, layoutStruct->x_left);
                right = qMin(right, layoutStruct->x_right);

                if (right - left < cd->size.width) {
                    layoutStruct->y = findY(layoutStruct->y, layoutStruct, cd->size.width);
                    floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                }

                QFixedPoint pos(left, layoutStruct->y);

                Qt::Alignment align = Qt::AlignLeft;

                QTextTable *table = qobject_cast<QTextTable *>(c);

                if (table)
                    align = table->format().alignment() & Qt::AlignHorizontal_Mask;

                // detect whether we have any alignment in the document that disallows optimizations,
                // such as not laying out the document again in a textedit with wrapping disabled.
                if (inRootFrame && !(align & Qt::AlignLeft))
                    contentHasAlignment = true;

                cd->position = pos;

                if (document->pageSize().height() > 0.0f)
                    cd->sizeDirty = true;

                if (cd->sizeDirty) {
                    if (width != 0)
                        layoutFrame(c, layoutFrom, layoutTo, width, -1, layoutStruct->frameY);
                    else
                        layoutFrame(c, layoutFrom, layoutTo, layoutStruct->frameY);

                    QFixed absoluteChildPos = table ? pos.y + static_cast<QTextTableData *>(data(table))->rowPositions.at(0) : pos.y + firstChildPos(c);
                    absoluteChildPos += layoutStruct->frameY;

                    // drop entire frame to next page if first child of frame is on next page
                    if (absoluteChildPos > layoutStruct->pageBottom) {
                        layoutStruct->newPage();
                        pos.y = layoutStruct->y;

                        cd->position = pos;
                        cd->sizeDirty = true;

                        if (width != 0)
                            layoutFrame(c, layoutFrom, layoutTo, width, -1, layoutStruct->frameY);
                        else
                            layoutFrame(c, layoutFrom, layoutTo, layoutStruct->frameY);
                    }
                }

                // align only if there is space for alignment
                if (right - left > cd->size.width) {
                    if (align & Qt::AlignRight)
                        pos.x += layoutStruct->x_right - cd->size.width;
                    else if (align & Qt::AlignHCenter)
                        pos.x += (layoutStruct->x_right - cd->size.width) / 2;
                }

                cd->position = pos;

                layoutStruct->y += cd->size.height;
                const int page = layoutStruct->currentPage();
                layoutStruct->pageBottom = (page + 1) * layoutStruct->pageHeight - layoutStruct->pageBottomMargin;

                cd->layoutDirty = false;

                if (c->frameFormat().pageBreakPolicy() & QTextFormat::PageBreak_AlwaysAfter)
                    layoutStruct->newPage();
            } else {
                QRectF oldFrameRect(cd->position.toPointF(), cd->size.toSizeF());
                QRectF updateRect;

                if (cd->sizeDirty)
                    updateRect = layoutFrame(c, layoutFrom, layoutTo);

                positionFloat(c);

                // If the size was made dirty when the position was set, layout again
                if (cd->sizeDirty)
                    updateRect = layoutFrame(c, layoutFrom, layoutTo);

                QRectF frameRect(cd->position.toPointF(), cd->size.toSizeF());

                if (frameRect == oldFrameRect && updateRect.isValid())
                    updateRect.translate(cd->position.toPointF());
                else
                    updateRect = frameRect;

                layoutStruct->addUpdateRectForFloat(updateRect);
                if (oldFrameRect.isValid())
                    layoutStruct->addUpdateRectForFloat(oldFrameRect);
            }

            layoutStruct->minimumWidth = qMax(layoutStruct->minimumWidth, cd->minimumWidth);
            layoutStruct->maximumWidth = qMin(layoutStruct->maximumWidth, cd->maximumWidth);

            previousIt = it;
            ++it;
        } else {
            QTextFrame::Iterator lastIt;
            if (!previousIt.atEnd() && previousIt != it)
                lastIt = previousIt;
            previousIt = it;
            QTextBlock block = it.currentBlock();
            ++it;

            const QTextBlockFormat blockFormat = block.blockFormat();

            if (blockFormat.pageBreakPolicy() & QTextFormat::PageBreak_AlwaysBefore)
                layoutStruct->newPage();

            const QFixed origY = layoutStruct->y;
            const QFixed origPageBottom = layoutStruct->pageBottom;
            const QFixed origMaximumWidth = layoutStruct->maximumWidth;
            layoutStruct->maximumWidth = 0;

            const QTextBlockFormat *previousBlockFormatPtr = nullptr;
            if (lastIt.currentBlock().isValid())
                previousBlockFormatPtr = &previousBlockFormat;

            // layout and position child block
            layoutBlock(block, docPos, blockFormat, layoutStruct, layoutFrom, layoutTo, previousBlockFormatPtr);

            // detect whether we have any alignment in the document that disallows optimizations,
            // such as not laying out the document again in a textedit with wrapping disabled.
            if (inRootFrame && !(block.layout()->textOption().alignment() & Qt::AlignLeft))
                contentHasAlignment = true;

            // if the block right before a table is empty 'hide' it by
            // positioning it into the table border
            if (isEmptyBlockBeforeTable(block, blockFormat, it)) {
                const QTextBlock lastBlock = lastIt.currentBlock();
                const qreal lastBlockBottomMargin = lastBlock.isValid() ? lastBlock.blockFormat().bottomMargin() : 0.0f;
                layoutStruct->y = origY + QFixed::fromReal(qMax(lastBlockBottomMargin, block.blockFormat().topMargin()));
                layoutStruct->pageBottom = origPageBottom;
            } else {
                // if the block right after a table is empty then 'hide' it, too
                if (isEmptyBlockAfterTable(block, lastIt.currentFrame())) {
                    QTextTableData *td = static_cast<QTextTableData *>(data(lastIt.currentFrame()));
                    QTextLayout *layout = block.layout();

                    QPointF pos((td->position.x + td->size.width).toReal(),
                                (td->position.y + td->size.height).toReal() - layout->boundingRect().height());

                    layout->setPosition(pos);
                    layoutStruct->y = origY;
                    layoutStruct->pageBottom = origPageBottom;
                }

                // if the block right after a table starts with a line separator, shift it up by one line
                if (isLineSeparatorBlockAfterTable(block, lastIt.currentFrame())) {
                    QTextTableData *td = static_cast<QTextTableData *>(data(lastIt.currentFrame()));
                    QTextLayout *layout = block.layout();

                    QFixed height = layout->lineCount() > 0 ? QFixed::fromReal(layout->lineAt(0).height()) : QFixed();

                    if (layoutStruct->pageBottom == origPageBottom) {
                        layoutStruct->y -= height;
                        layout->setPosition(layout->position() - QPointF(0, height.toReal()));
                    } else {
                        // relayout block to correctly handle page breaks
                        layoutStruct->y = origY - height;
                        layoutStruct->pageBottom = origPageBottom;
                        layoutBlock(block, docPos, blockFormat, layoutStruct, layoutFrom, layoutTo, previousBlockFormatPtr);
                    }

                    if (layout->lineCount() > 0) {
                        QPointF linePos((td->position.x + td->size.width).toReal(),
                                        (td->position.y + td->size.height - height).toReal());

                        layout->lineAt(0).setPosition(linePos - layout->position());
                    }
                }

                if (blockFormat.pageBreakPolicy() & QTextFormat::PageBreak_AlwaysAfter)
                    layoutStruct->newPage();
            }

            maximumBlockWidth = qMax(maximumBlockWidth, layoutStruct->maximumWidth);
            layoutStruct->maximumWidth = origMaximumWidth;
            previousBlockFormat = blockFormat;
        }
    }
    if (layoutStruct->maximumWidth == QFIXED_MAX && maximumBlockWidth > 0)
        layoutStruct->maximumWidth = maximumBlockWidth;
    else
        layoutStruct->maximumWidth = qMax(layoutStruct->maximumWidth, maximumBlockWidth);

    // a float at the bottom of a frame may make it taller, hence the qMax() for layoutStruct->y.
    // we don't need to do it for tables though because floats in tables are per table
    // and not per cell and layoutCell already takes care of doing the same as we do here
    if (!qobject_cast<QTextTable *>(layoutStruct->frame)) {
        QList<QTextFrame *> children = layoutStruct->frame->childFrames();
        for (int i = 0; i < children.count(); ++i) {
            QTextFrameData *fd = data(children.at(i));
            if (!fd->layoutDirty && children.at(i)->frameFormat().position() != QTextFrameFormat::InFlow)
                layoutStruct->y = qMax(layoutStruct->y, fd->position.y + fd->size.height);
        }
    }

    if (inRootFrame) {
        // we assume that any float is aligned in a way that disallows the optimizations that rely
        // on unaligned content.
        if (!fd->floats.isEmpty())
            contentHasAlignment = true;

        if (it.atEnd()) {
            //qDebug("layout done!");
            currentLazyLayoutPosition = -1;
            QCheckPoint cp;
            cp.y = layoutStruct->y;
            cp.positionInFrame = docPrivate->length();
            cp.minimumWidth = layoutStruct->minimumWidth;
            cp.maximumWidth = layoutStruct->maximumWidth;
            cp.contentsWidth = layoutStruct->contentsWidth;
            checkPoints.append(cp);
            checkPoints.reserve(checkPoints.size());
            fd->fullLayoutCompleted = true;
        } else {
            currentLazyLayoutPosition = checkPoints.constLast().positionInFrame;
            // #######
            //checkPoints.last().positionInFrame = q->document()->docHandle()->length();
        }
    }


    fd->currentLayoutStruct = nullptr;
}

static inline void getLineHeightParams(const QTextBlockFormat &blockFormat, const QTextLine &line, qreal scaling,
                                       QFixed *lineAdjustment, QFixed *lineBreakHeight, QFixed *lineHeight, QFixed *lineBottom)
{
    qreal rawHeight = qCeil(line.ascent() + line.descent() + line.leading());
    *lineHeight = QFixed::fromReal(blockFormat.lineHeight(rawHeight, scaling));
    *lineBottom = QFixed::fromReal(blockFormat.lineHeight(line.height(), scaling));

    if (blockFormat.lineHeightType() == QTextBlockFormat::FixedHeight || blockFormat.lineHeightType() == QTextBlockFormat::MinimumHeight) {
        *lineBreakHeight = *lineBottom;
        if (blockFormat.lineHeightType() == QTextBlockFormat::FixedHeight)
            *lineAdjustment = QFixed::fromReal(line.ascent() + qMax(line.leading(), qreal(0.0))) - ((*lineHeight * 4) / 5);
        else
            *lineAdjustment = QFixed::fromReal(line.height()) - *lineHeight;
    }
    else {
        *lineBreakHeight = QFixed::fromReal(line.height());
        *lineAdjustment = 0;
    }
}

void QTextDocumentLayoutPrivate::layoutBlock(const QTextBlock &bl, int blockPosition, const QTextBlockFormat &blockFormat,
                                             QTextLayoutStruct *layoutStruct, int layoutFrom, int layoutTo, const QTextBlockFormat *previousBlockFormat)
{
    Q_Q(QTextDocumentLayout);
    if (!bl.isVisible())
        return;

    QTextLayout *tl = bl.layout();
    const int blockLength = bl.length();

    qCDebug(lcLayout) << "layoutBlock from=" << layoutFrom << "to=" << layoutTo
                      << "; width" << layoutStruct->x_right - layoutStruct->x_left << "(maxWidth is btw" << tl->maximumWidth() << ')';

    if (previousBlockFormat) {
        qreal margin = qMax(blockFormat.topMargin(), previousBlockFormat->bottomMargin());
        if (margin > 0 && q->paintDevice()) {
            margin *= qreal(q->paintDevice()->logicalDpiY()) / qreal(qt_defaultDpi());
        }
        layoutStruct->y += QFixed::fromReal(margin);
    }

    //QTextFrameData *fd = data(layoutStruct->frame);

    Qt::LayoutDirection dir = bl.textDirection();

    QFixed extraMargin;
    if (docPrivate->defaultTextOption.flags() & QTextOption::AddSpaceForLineAndParagraphSeparators) {
        QFontMetricsF fm(bl.charFormat().font());
        extraMargin = QFixed::fromReal(fm.horizontalAdvance(QChar(QChar(0x21B5))));
    }

    const QFixed indent = this->blockIndent(blockFormat);
    const QFixed totalLeftMargin = QFixed::fromReal(blockFormat.leftMargin()) + (dir == Qt::RightToLeft ? extraMargin : indent);
    const QFixed totalRightMargin = QFixed::fromReal(blockFormat.rightMargin()) + (dir == Qt::RightToLeft ? indent : extraMargin);

    const QPointF oldPosition = tl->position();
    tl->setPosition(QPointF(layoutStruct->x_left.toReal(), layoutStruct->y.toReal()));

    if (layoutStruct->fullLayout
        || (blockPosition + blockLength > layoutFrom && blockPosition <= layoutTo)
        // force relayout if we cross a page boundary
        || (layoutStruct->pageHeight != QFIXED_MAX && layoutStruct->absoluteY() + QFixed::fromReal(tl->boundingRect().height()) > layoutStruct->pageBottom)) {

        qCDebug(lcLayout) << "do layout";
        QTextOption option = docPrivate->defaultTextOption;
        option.setTextDirection(dir);
        option.setTabs( blockFormat.tabPositions() );

        Qt::Alignment align = docPrivate->defaultTextOption.alignment();
        if (blockFormat.hasProperty(QTextFormat::BlockAlignment))
            align = blockFormat.alignment();
        option.setAlignment(QGuiApplicationPrivate::visualAlignment(dir, align)); // for paragraph that are RTL, alignment is auto-reversed;

        if (blockFormat.nonBreakableLines() || document->pageSize().width() < 0) {
            option.setWrapMode(QTextOption::ManualWrap);
        }

        tl->setTextOption(option);

        const bool haveWordOrAnyWrapMode = (option.wrapMode() == QTextOption::WrapAtWordBoundaryOrAnywhere);

//         qDebug() << "    layouting block at" << bl.position();
        const QFixed cy = layoutStruct->y;
        const QFixed l = layoutStruct->x_left  + totalLeftMargin;
        const QFixed r = layoutStruct->x_right - totalRightMargin;
        QFixed bottom;

        tl->beginLayout();
        bool firstLine = true;
        while (1) {
            QTextLine line = tl->createLine();
            if (!line.isValid())
                break;
            line.setLeadingIncluded(true);

            QFixed left, right;
            floatMargins(layoutStruct->y, layoutStruct, &left, &right);
            left = qMax(left, l);
            right = qMin(right, r);
            QFixed text_indent;
            if (firstLine) {
                text_indent = QFixed::fromReal(blockFormat.textIndent());
                if (dir == Qt::LeftToRight)
                    left += text_indent;
                else
                    right -= text_indent;
                firstLine = false;
            }
//         qDebug() << "layout line y=" << currentYPos << "left=" << left << "right=" <<right;

            if (fixedColumnWidth != -1)
                line.setNumColumns(fixedColumnWidth, (right - left).toReal());
            else
                line.setLineWidth((right - left).toReal());

//        qDebug() << "layoutBlock; layouting line with width" << right - left << "->textWidth" << line.textWidth();
            floatMargins(layoutStruct->y, layoutStruct, &left, &right);
            left = qMax(left, l);
            right = qMin(right, r);
            if (dir == Qt::LeftToRight)
                left += text_indent;
            else
                right -= text_indent;

            if (fixedColumnWidth == -1 && QFixed::fromReal(line.naturalTextWidth()) > right-left) {
                // float has been added in the meantime, redo
                layoutStruct->pendingFloats.clear();

                line.setLineWidth((right-left).toReal());
                if (QFixed::fromReal(line.naturalTextWidth()) > right-left) {
                    if (haveWordOrAnyWrapMode) {
                        option.setWrapMode(QTextOption::WrapAnywhere);
                        tl->setTextOption(option);
                    }

                    layoutStruct->pendingFloats.clear();
                    // lines min width more than what we have
                    layoutStruct->y = findY(layoutStruct->y, layoutStruct, QFixed::fromReal(line.naturalTextWidth()));
                    floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                    left = qMax(left, l);
                    right = qMin(right, r);
                    if (dir == Qt::LeftToRight)
                        left += text_indent;
                    else
                        right -= text_indent;
                    line.setLineWidth(qMax<qreal>(line.naturalTextWidth(), (right-left).toReal()));

                    if (haveWordOrAnyWrapMode) {
                        option.setWrapMode(QTextOption::WordWrap);
                        tl->setTextOption(option);
                    }
                }

            }

            QFixed lineBreakHeight, lineHeight, lineAdjustment, lineBottom;
            qreal scaling = (q->paintDevice() && q->paintDevice()->logicalDpiY() != qt_defaultDpi()) ?
                            qreal(q->paintDevice()->logicalDpiY()) / qreal(qt_defaultDpi()) : 1;
            getLineHeightParams(blockFormat, line, scaling, &lineAdjustment, &lineBreakHeight, &lineHeight, &lineBottom);

            while (layoutStruct->pageHeight > 0 && layoutStruct->absoluteY() + lineBreakHeight > layoutStruct->pageBottom &&
                layoutStruct->contentHeight() >= lineBreakHeight) {
                layoutStruct->newPage();

                floatMargins(layoutStruct->y, layoutStruct, &left, &right);
                left = qMax(left, l);
                right = qMin(right, r);
                if (dir == Qt::LeftToRight)
                    left += text_indent;
                else
                    right -= text_indent;
            }

            line.setPosition(QPointF((left - layoutStruct->x_left).toReal(), (layoutStruct->y - cy - lineAdjustment).toReal()));
            bottom = layoutStruct->y + lineBottom;
            layoutStruct->y += lineHeight;
            layoutStruct->contentsWidth
                = qMax<QFixed>(layoutStruct->contentsWidth, QFixed::fromReal(line.x() + line.naturalTextWidth()) + totalRightMargin);

            // position floats
            for (int i = 0; i < layoutStruct->pendingFloats.size(); ++i) {
                QTextFrame *f = layoutStruct->pendingFloats.at(i);
                positionFloat(f);
            }
            layoutStruct->pendingFloats.clear();
        }
        layoutStruct->y = qMax(layoutStruct->y, bottom);
        tl->endLayout();
    } else {
        const int cnt = tl->lineCount();
        QFixed bottom;
        for (int i = 0; i < cnt; ++i) {
            qCDebug(lcLayout) << "going to move text line" << i;
            QTextLine line = tl->lineAt(i);
            layoutStruct->contentsWidth
                = qMax(layoutStruct->contentsWidth, QFixed::fromReal(line.x() + tl->lineAt(i).naturalTextWidth()) + totalRightMargin);

            QFixed lineBreakHeight, lineHeight, lineAdjustment, lineBottom;
            qreal scaling = (q->paintDevice() && q->paintDevice()->logicalDpiY() != qt_defaultDpi()) ?
                            qreal(q->paintDevice()->logicalDpiY()) / qreal(qt_defaultDpi()) : 1;
            getLineHeightParams(blockFormat, line, scaling, &lineAdjustment, &lineBreakHeight, &lineHeight, &lineBottom);

            if (layoutStruct->pageHeight != QFIXED_MAX) {
                if (layoutStruct->absoluteY() + lineBreakHeight > layoutStruct->pageBottom)
                    layoutStruct->newPage();
                line.setPosition(QPointF(line.position().x(), (layoutStruct->y - lineAdjustment).toReal() - tl->position().y()));
            }
            bottom = layoutStruct->y + lineBottom;
            layoutStruct->y += lineHeight;
        }
        layoutStruct->y = qMax(layoutStruct->y, bottom);
        if (layoutStruct->updateRect.isValid()
            && blockLength > 1) {
            if (layoutFrom >= blockPosition + blockLength) {
                // if our height didn't change and the change in the document is
                // in one of the later paragraphs, then we don't need to repaint
                // this one
                layoutStruct->updateRect.setTop(qMax(layoutStruct->updateRect.top(), layoutStruct->y.toReal()));
            } else if (layoutTo < blockPosition) {
                if (oldPosition == tl->position())
                    // if the change in the document happened earlier in the document
                    // and our position did /not/ change because none of the earlier paragraphs
                    // or frames changed their height, then we don't need to repaint
                    // this one
                    layoutStruct->updateRect.setBottom(qMin(layoutStruct->updateRect.bottom(), tl->position().y()));
                else
                    layoutStruct->updateRect.setBottom(qreal(INT_MAX)); // reset
            }
        }
    }

    // ### doesn't take floats into account. would need to do it per line. but how to retrieve then? (Simon)
    const QFixed margins = totalLeftMargin + totalRightMargin;
    layoutStruct->minimumWidth = qMax(layoutStruct->minimumWidth, QFixed::fromReal(tl->minimumWidth()) + margins);

    const QFixed maxW = QFixed::fromReal(tl->maximumWidth()) + margins;

    if (maxW > 0) {
        if (layoutStruct->maximumWidth == QFIXED_MAX)
            layoutStruct->maximumWidth = maxW;
        else
            layoutStruct->maximumWidth = qMax(layoutStruct->maximumWidth, maxW);
    }
}

void QTextDocumentLayoutPrivate::floatMargins(const QFixed &y, const QTextLayoutStruct *layoutStruct,
                                              QFixed *left, QFixed *right) const
{
//     qDebug() << "floatMargins y=" << y;
    *left = layoutStruct->x_left;
    *right = layoutStruct->x_right;
    QTextFrameData *lfd = data(layoutStruct->frame);
    for (int i = 0; i < lfd->floats.size(); ++i) {
        QTextFrameData *fd = data(lfd->floats.at(i));
        if (!fd->layoutDirty) {
            if (fd->position.y <= y && fd->position.y + fd->size.height > y) {
//                 qDebug() << "adjusting with float" << f << fd->position.x()<< fd->size.width();
                if (lfd->floats.at(i)->frameFormat().position() == QTextFrameFormat::FloatLeft)
                    *left = qMax(*left, fd->position.x + fd->size.width);
                else
                    *right = qMin(*right, fd->position.x);
            }
        }
    }
//     qDebug() << "floatMargins: left="<<*left<<"right="<<*right<<"y="<<y;
}

QFixed QTextDocumentLayoutPrivate::findY(QFixed yFrom, const QTextLayoutStruct *layoutStruct, QFixed requiredWidth) const
{
    QFixed right, left;
    requiredWidth = qMin(requiredWidth, layoutStruct->x_right - layoutStruct->x_left);

//     qDebug() << "findY:" << yFrom;
    while (1) {
        floatMargins(yFrom, layoutStruct, &left, &right);
//         qDebug() << "    yFrom=" << yFrom<<"right=" << right << "left=" << left << "requiredWidth=" << requiredWidth;
        if (right-left >= requiredWidth)
            break;

        // move float down until we find enough space
        QFixed newY = QFIXED_MAX;
        QTextFrameData *lfd = data(layoutStruct->frame);
        for (int i = 0; i < lfd->floats.size(); ++i) {
            QTextFrameData *fd = data(lfd->floats.at(i));
            if (!fd->layoutDirty) {
                if (fd->position.y <= yFrom && fd->position.y + fd->size.height > yFrom)
                    newY = qMin(newY, fd->position.y + fd->size.height);
            }
        }
        if (newY == QFIXED_MAX)
            break;
        yFrom = newY;
    }
    return yFrom;
}

QTextDocumentLayout::QTextDocumentLayout(QTextDocument *doc)
    : QAbstractTextDocumentLayout(*new QTextDocumentLayoutPrivate, doc)
{
    registerHandler(QTextFormat::ImageObject, new QTextImageHandler(this));
}


void QTextDocumentLayout::draw(QPainter *painter, const PaintContext &context)
{
    Q_D(QTextDocumentLayout);
    QTextFrame *frame = d->document->rootFrame();
    QTextFrameData *fd = data(frame);

    if(fd->sizeDirty)
        return;

    if (context.clip.isValid()) {
        d->ensureLayouted(QFixed::fromReal(context.clip.bottom()));
    } else {
        d->ensureLayoutFinished();
    }

    QFixed width = fd->size.width;
    if (d->document->pageSize().width() == 0 && d->viewportRect.isValid()) {
        // we're in NoWrap mode, meaning the frame should expand to the viewport
        // so that backgrounds are drawn correctly
        fd->size.width = qMax(width, QFixed::fromReal(d->viewportRect.right()));
    }

    // Make sure we conform to the root frames bounds when drawing.
    d->clipRect = QRectF(fd->position.toPointF(), fd->size.toSizeF()).adjusted(fd->leftMargin.toReal(), 0, -fd->rightMargin.toReal(), 0);
    d->drawFrame(QPointF(), painter, context, frame);
    fd->size.width = width;
}

void QTextDocumentLayout::setViewport(const QRectF &viewport)
{
    Q_D(QTextDocumentLayout);
    d->viewportRect = viewport;
}

static void markFrames(QTextFrame *current, int from, int oldLength, int length)
{
    int end = qMax(oldLength, length) + from;

    if (current->firstPosition() >= end || current->lastPosition() < from)
        return;

    QTextFrameData *fd = data(current);
    // float got removed in editing operation
    QTextFrame *null = nullptr; // work-around for (at least) MSVC 2012 emitting
                                // warning C4100 for its own header <algorithm>
                                // when passing nullptr directly to std::remove
    fd->floats.erase(std::remove(fd->floats.begin(), fd->floats.end(), null),
                     fd->floats.end());

    fd->layoutDirty = true;
    fd->sizeDirty = true;

//     qDebug("    marking frame (%d--%d) as dirty", current->firstPosition(), current->lastPosition());
    QList<QTextFrame *> children = current->childFrames();
    for (int i = 0; i < children.size(); ++i)
        markFrames(children.at(i), from, oldLength, length);
}

void QTextDocumentLayout::documentChanged(int from, int oldLength, int length)
{
    Q_D(QTextDocumentLayout);

    QTextBlock blockIt = document()->findBlock(from);
    QTextBlock endIt = document()->findBlock(qMax(0, from + length - 1));
    if (endIt.isValid())
        endIt = endIt.next();
     for (; blockIt.isValid() && blockIt != endIt; blockIt = blockIt.next())
         blockIt.clearLayout();

    if (d->docPrivate->pageSize.isNull())
        return;

    QRectF updateRect;

    d->lazyLayoutStepSize = 1000;
    d->sizeChangedTimer.stop();
    d->insideDocumentChange = true;

    const int documentLength = d->docPrivate->length();
    const bool fullLayout = (oldLength == 0 && length == documentLength);
    const bool smallChange = documentLength > 0
                             && (qMax(length, oldLength) * 100 / documentLength) < 5;

    // don't show incremental layout progress (avoid scroll bar flicker)
    // if we see only a small change in the document and we're either starting
    // a layout run or we're already in progress for that and we haven't seen
    // any bigger change previously (showLayoutProgress already false)
    if (smallChange
        && (d->currentLazyLayoutPosition == -1 || d->showLayoutProgress == false))
        d->showLayoutProgress = false;
    else
        d->showLayoutProgress = true;

    if (fullLayout) {
        d->contentHasAlignment = false;
        d->currentLazyLayoutPosition = 0;
        d->checkPoints.clear();
        data(d->docPrivate->rootFrame())->fullLayoutCompleted = false;
        d->layoutStep();
    } else {
        d->ensureLayoutedByPosition(from);
        updateRect = doLayout(from, oldLength, length);
    }

    if (!d->layoutTimer.isActive() && d->currentLazyLayoutPosition != -1)
        d->layoutTimer.start(10, this);

    d->insideDocumentChange = false;

    if (d->showLayoutProgress) {
        const QSizeF newSize = dynamicDocumentSize();
        if (newSize != d->lastReportedSize) {
            d->lastReportedSize = newSize;
            emit documentSizeChanged(newSize);
        }
    }

    if (!updateRect.isValid()) {
        // don't use the frame size, it might have shrunken
        updateRect = QRectF(QPointF(0, 0), QSizeF(qreal(INT_MAX), qreal(INT_MAX)));
    }

    emit update(updateRect);
}

QRectF QTextDocumentLayout::doLayout(int from, int oldLength, int length)
{
    Q_D(QTextDocumentLayout);

//     qDebug("documentChange: from=%d, oldLength=%d, length=%d", from, oldLength, length);

    // mark all frames between f_start and f_end as dirty
    markFrames(d->docPrivate->rootFrame(), from, oldLength, length);

    QRectF updateRect;

    QTextFrame *root = d->docPrivate->rootFrame();
    if(data(root)->sizeDirty)
        updateRect = d->layoutFrame(root, from, from + length);
    data(root)->layoutDirty = false;

    if (d->currentLazyLayoutPosition == -1)
        layoutFinished();
    else if (d->showLayoutProgress)
        d->sizeChangedTimer.start(0, this);

    return updateRect;
}

int QTextDocumentLayout::hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const
{
    Q_D(const QTextDocumentLayout);
    d->ensureLayouted(QFixed::fromReal(point.y()));
    QTextFrame *f = d->docPrivate->rootFrame();
    int position = 0;
    QTextLayout *l = nullptr;
    QFixedPoint pointf;
    pointf.x = QFixed::fromReal(point.x());
    pointf.y = QFixed::fromReal(point.y());
    QTextDocumentLayoutPrivate::HitPoint p = d->hitTest(f, pointf, &position, &l, accuracy);
    if (accuracy == Qt::ExactHit && p < QTextDocumentLayoutPrivate::PointExact)
        return -1;

    // ensure we stay within document bounds
    int lastPos = f->lastPosition();
    if (l && !l->preeditAreaText().isEmpty())
        lastPos += l->preeditAreaText().length();
    if (position > lastPos)
        position = lastPos;
    else if (position < 0)
        position = 0;

    return position;
}

void QTextDocumentLayout::resizeInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format)
{
    Q_D(QTextDocumentLayout);
    QTextCharFormat f = format.toCharFormat();
    Q_ASSERT(f.isValid());
    QTextObjectHandler handler = d->handlers.value(f.objectType());
    if (!handler.component)
        return;

    QSizeF intrinsic = handler.iface->intrinsicSize(d->document, posInDocument, format);

    QTextFrameFormat::Position pos = QTextFrameFormat::InFlow;
    QTextFrame *frame = qobject_cast<QTextFrame *>(d->document->objectForFormat(f));
    if (frame) {
        pos = frame->frameFormat().position();
        QTextFrameData *fd = data(frame);
        fd->sizeDirty = false;
        fd->size = QFixedSize::fromSizeF(intrinsic);
        fd->minimumWidth = fd->maximumWidth = fd->size.width;
    }

    QSizeF inlineSize = (pos == QTextFrameFormat::InFlow ? intrinsic : QSizeF(0, 0));
    item.setWidth(inlineSize.width());

    if (f.verticalAlignment() == QTextCharFormat::AlignMiddle) {
        QFontMetrics m(f.font());
        qreal halfX = m.xHeight()/2.;
        item.setAscent((inlineSize.height() + halfX) / 2.);
        item.setDescent((inlineSize.height() - halfX) / 2.);
    } else {
        item.setDescent(0);
        item.setAscent(inlineSize.height());
    }
}

void QTextDocumentLayout::positionInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format)
{
    Q_D(QTextDocumentLayout);
    Q_UNUSED(posInDocument);
    if (item.width() != 0)
        // inline
        return;

    QTextCharFormat f = format.toCharFormat();
    Q_ASSERT(f.isValid());
    QTextObjectHandler handler = d->handlers.value(f.objectType());
    if (!handler.component)
        return;

    QTextFrame *frame = qobject_cast<QTextFrame *>(d->document->objectForFormat(f));
    if (!frame)
        return;

    QTextBlock b = d->document->findBlock(frame->firstPosition());
    QTextLine line;
    if (b.position() <= frame->firstPosition() && b.position() + b.length() > frame->lastPosition())
        line = b.layout()->lineAt(b.layout()->lineCount()-1);
//     qDebug() << "layoutObject: line.isValid" << line.isValid() << b.position() << b.length() <<
//         frame->firstPosition() << frame->lastPosition();
    d->positionFloat(frame, line.isValid() ? &line : nullptr);
}

void QTextDocumentLayout::drawInlineObject(QPainter *p, const QRectF &rect, QTextInlineObject item,
                                           int posInDocument, const QTextFormat &format)
{
    Q_D(QTextDocumentLayout);
    QTextCharFormat f = format.toCharFormat();
    Q_ASSERT(f.isValid());
    QTextFrame *frame = qobject_cast<QTextFrame *>(d->document->objectForFormat(f));
    if (frame && frame->frameFormat().position() != QTextFrameFormat::InFlow)
        return; // don't draw floating frames from inline objects here but in drawFlow instead

//    qDebug() << "drawObject at" << r;
    QAbstractTextDocumentLayout::drawInlineObject(p, rect, item, posInDocument, format);
}

int QTextDocumentLayout::dynamicPageCount() const
{
    Q_D(const QTextDocumentLayout);
    const QSizeF pgSize = d->document->pageSize();
    if (pgSize.height() < 0)
        return 1;
    return qCeil(dynamicDocumentSize().height() / pgSize.height());
}

QSizeF QTextDocumentLayout::dynamicDocumentSize() const
{
    Q_D(const QTextDocumentLayout);
    return data(d->docPrivate->rootFrame())->size.toSizeF();
}

int QTextDocumentLayout::pageCount() const
{
    Q_D(const QTextDocumentLayout);
    d->ensureLayoutFinished();
    return dynamicPageCount();
}

QSizeF QTextDocumentLayout::documentSize() const
{
    Q_D(const QTextDocumentLayout);
    d->ensureLayoutFinished();
    return dynamicDocumentSize();
}

void QTextDocumentLayoutPrivate::ensureLayouted(QFixed y) const
{
    Q_Q(const QTextDocumentLayout);
    if (currentLazyLayoutPosition == -1)
        return;
    const QSizeF oldSize = q->dynamicDocumentSize();
    Q_UNUSED(oldSize);

    if (checkPoints.isEmpty())
        layoutStep();

    while (currentLazyLayoutPosition != -1
           && checkPoints.last().y < y)
        layoutStep();
}

void QTextDocumentLayoutPrivate::ensureLayoutedByPosition(int position) const
{
    if (currentLazyLayoutPosition == -1)
        return;
    if (position < currentLazyLayoutPosition)
        return;
    while (currentLazyLayoutPosition != -1
           && currentLazyLayoutPosition < position) {
        const_cast<QTextDocumentLayout *>(q_func())->doLayout(currentLazyLayoutPosition, 0, INT_MAX - currentLazyLayoutPosition);
    }
}

void QTextDocumentLayoutPrivate::layoutStep() const
{
    ensureLayoutedByPosition(currentLazyLayoutPosition + lazyLayoutStepSize);
    lazyLayoutStepSize = qMin(200000, lazyLayoutStepSize * 2);
}

void QTextDocumentLayout::setCursorWidth(int width)
{
    Q_D(QTextDocumentLayout);
    d->cursorWidth = width;
}

int QTextDocumentLayout::cursorWidth() const
{
    Q_D(const QTextDocumentLayout);
    return d->cursorWidth;
}

void QTextDocumentLayout::setFixedColumnWidth(int width)
{
    Q_D(QTextDocumentLayout);
    d->fixedColumnWidth = width;
}

QRectF QTextDocumentLayout::tableCellBoundingRect(QTextTable *table, const QTextTableCell &cell) const
{
    if (!cell.isValid())
        return QRectF();

    QTextTableData *td = static_cast<QTextTableData *>(data(table));

    QRectF tableRect = tableBoundingRect(table);
    QRectF cellRect = td->cellRect(cell);

    return cellRect.translated(tableRect.topLeft());
}

QRectF QTextDocumentLayout::tableBoundingRect(QTextTable *table) const
{
    Q_D(const QTextDocumentLayout);
    if (d->docPrivate->pageSize.isNull())
        return QRectF();
    d->ensureLayoutFinished();

    QPointF pos;
    const int framePos = table->firstPosition();
    QTextFrame *f = table;
    while (f) {
        QTextFrameData *fd = data(f);
        pos += fd->position.toPointF();

        if (f != table) {
            if (QTextTable *table = qobject_cast<QTextTable *>(f)) {
                QTextTableCell cell = table->cellAt(framePos);
                if (cell.isValid())
                    pos += static_cast<QTextTableData *>(fd)->cellPosition(table, cell).toPointF();
            }
        }

        f = f->parentFrame();
    }
    return QRectF(pos, data(table)->size.toSizeF());
}

QRectF QTextDocumentLayout::frameBoundingRect(QTextFrame *frame) const
{
    Q_D(const QTextDocumentLayout);
    if (d->docPrivate->pageSize.isNull())
        return QRectF();
    d->ensureLayoutFinished();
    return d->frameBoundingRectInternal(frame);
}

QRectF QTextDocumentLayoutPrivate::frameBoundingRectInternal(QTextFrame *frame) const
{
    QPointF pos;
    const int framePos = frame->firstPosition();
    QTextFrame *f = frame;
    while (f) {
        QTextFrameData *fd = data(f);
        pos += fd->position.toPointF();

        if (QTextTable *table = qobject_cast<QTextTable *>(f)) {
            QTextTableCell cell = table->cellAt(framePos);
            if (cell.isValid())
                pos += static_cast<QTextTableData *>(fd)->cellPosition(table, cell).toPointF();
        }

        f = f->parentFrame();
    }
    return QRectF(pos, data(frame)->size.toSizeF());
}

QRectF QTextDocumentLayout::blockBoundingRect(const QTextBlock &block) const
{
    Q_D(const QTextDocumentLayout);
    if (d->docPrivate->pageSize.isNull() || !block.isValid() || !block.isVisible())
        return QRectF();
    d->ensureLayoutedByPosition(block.position() + block.length());
    QTextFrame *frame = d->document->frameAt(block.position());
    QPointF offset;
    const int blockPos = block.position();

    while (frame) {
        QTextFrameData *fd = data(frame);
        offset += fd->position.toPointF();

        if (QTextTable *table = qobject_cast<QTextTable *>(frame)) {
            QTextTableCell cell = table->cellAt(blockPos);
            if (cell.isValid())
                offset += static_cast<QTextTableData *>(fd)->cellPosition(table, cell).toPointF();
        }

        frame = frame->parentFrame();
    }

    const QTextLayout *layout = block.layout();
    QRectF rect = layout->boundingRect();
    rect.moveTopLeft(layout->position() + offset);
    return rect;
}

int QTextDocumentLayout::layoutStatus() const
{
    Q_D(const QTextDocumentLayout);
    int pos = d->currentLazyLayoutPosition;
    if (pos == -1)
        return 100;
    return pos * 100 / d->document->docHandle()->length();
}

void QTextDocumentLayout::timerEvent(QTimerEvent *e)
{
    Q_D(QTextDocumentLayout);
    if (e->timerId() == d->layoutTimer.timerId()) {
        if (d->currentLazyLayoutPosition != -1)
            d->layoutStep();
    } else if (e->timerId() == d->sizeChangedTimer.timerId()) {
        d->lastReportedSize = dynamicDocumentSize();
        emit documentSizeChanged(d->lastReportedSize);
        d->sizeChangedTimer.stop();

        if (d->currentLazyLayoutPosition == -1) {
            const int newCount = dynamicPageCount();
            if (newCount != d->lastPageCount) {
                d->lastPageCount = newCount;
                emit pageCountChanged(newCount);
            }
        }
    } else {
        QAbstractTextDocumentLayout::timerEvent(e);
    }
}

void QTextDocumentLayout::layoutFinished()
{
    Q_D(QTextDocumentLayout);
    d->layoutTimer.stop();
    if (!d->insideDocumentChange)
        d->sizeChangedTimer.start(0, this);
    // reset
    d->showLayoutProgress = true;
}

void QTextDocumentLayout::ensureLayouted(qreal y)
{
    d_func()->ensureLayouted(QFixed::fromReal(y));
}

qreal QTextDocumentLayout::idealWidth() const
{
    Q_D(const QTextDocumentLayout);
    d->ensureLayoutFinished();
    return d->idealWidth;
}

bool QTextDocumentLayout::contentHasAlignment() const
{
    Q_D(const QTextDocumentLayout);
    return d->contentHasAlignment;
}

qreal QTextDocumentLayoutPrivate::scaleToDevice(qreal value) const
{
    if (!paintDevice)
        return value;
    return value * paintDevice->logicalDpiY() / qreal(qt_defaultDpi());
}

QFixed QTextDocumentLayoutPrivate::scaleToDevice(QFixed value) const
{
    if (!paintDevice)
        return value;
    return value * QFixed(paintDevice->logicalDpiY()) / QFixed(qt_defaultDpi());
}

QT_END_NAMESPACE

#include "moc_qtextdocumentlayout_p.cpp"
