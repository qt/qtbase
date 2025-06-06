// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2013 Samuel Gaist <samuel.gaist@deltech.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlistview.h"

#include <qabstractitemdelegate.h>
#if QT_CONFIG(accessibility)
#include <qaccessible.h>
#endif
#include <qapplication.h>
#include <qstylepainter.h>
#include <qbitmap.h>
#include <qdebug.h>
#if QT_CONFIG(draganddrop)
#include <qdrag.h>
#endif
#include <qevent.h>
#include <qlist.h>
#if QT_CONFIG(rubberband)
#include <qrubberband.h>
#endif
#include <qscrollbar.h>
#include <qstyle.h>
#include <private/qapplication_p.h>
#include <private/qlistview_p.h>
#include <private/qscrollbar_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

extern bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event);

/*!
    \class QListView

    \brief The QListView class provides a list or icon view onto a model.

    \ingroup model-view
    \ingroup advanced
    \inmodule QtWidgets

    \image fusion-listview.png

    A QListView presents items stored in a model, either as a simple
    non-hierarchical list, or as a collection of icons. This class is used
    to provide lists and icon views that were previously provided by the
    \c QListBox and \c QIconView classes, but using the more flexible
    approach provided by Qt's model/view architecture.

    The QListView class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    This view does not display horizontal or vertical headers; to display
    a list of items with a horizontal header, use QTreeView instead.

    QListView implements the interfaces defined by the
    QAbstractItemView class to allow it to display data provided by
    models derived from the QAbstractItemModel class.

    Items in a list view can be displayed using one of two view modes:
    In \l ListMode, the items are displayed in the form of a simple list;
    in \l IconMode, the list view takes the form of an \e{icon view} in
    which the items are displayed with icons like files in a file manager.
    By default, the list view is in \l ListMode. To change the view mode,
    use the setViewMode() function, and to determine the current view mode,
    use viewMode().

    Items in these views are laid out in the direction specified by the
    flow() of the list view. The items may be fixed in place, or allowed
    to move, depending on the view's movement() state.

    If the items in the model cannot be completely laid out in the
    direction of flow, they can be wrapped at the boundary of the view
    widget; this depends on isWrapping(). This property is useful when the
    items are being represented by an icon view.

    The resizeMode() and layoutMode() govern how and when the items are
    laid out. Items are spaced according to their spacing(), and can exist
    within a notional grid of size specified by gridSize(). The items can
    be rendered as large or small icons depending on their iconSize().

    \section1 Improving Performance

    It is possible to give the view hints about the data it is handling in order
    to improve its performance when displaying large numbers of items. One approach
    that can be taken for views that are intended to display items with equal sizes
    is to set the \l uniformItemSizes property to true.

    \sa {View Classes}, QTreeView, QTableView, QListWidget
*/

/*!
    \enum QListView::ViewMode

    \value ListMode The items are laid out using TopToBottom flow, with Small size and Static movement
    \value IconMode The items are laid out using LeftToRight flow, with Large size and Free movement
*/

/*!
  \enum QListView::Movement

  \value Static The items cannot be moved by the user.
  \value Free The items can be moved freely by the user.
  \value Snap The items snap to the specified grid when moved; see
  setGridSize().
*/

/*!
  \enum QListView::Flow

  \value LeftToRight The items are laid out in the view from the left
  to the right.
  \value TopToBottom The items are laid out in the view from the top
  to the bottom.
*/

/*!
  \enum QListView::ResizeMode

  \value Fixed The items will only be laid out the first time the view is shown.
  \value Adjust The items will be laid out every time the view is resized.
*/

/*!
  \enum QListView::LayoutMode

  \value SinglePass The items are laid out all at once.
  \value Batched The items are laid out in batches of \l batchSize items.
  \sa batchSize
*/

/*!
  \fn void QListView::indexesMoved(const QModelIndexList &indexes)

  This signal is emitted when the specified \a indexes are moved in the view.
*/

/*!
    Creates a new QListView with the given \a parent to view a model.
    Use setModel() to set the model.
*/
QListView::QListView(QWidget *parent)
    : QAbstractItemView(*new QListViewPrivate, parent)
{
    setViewMode(ListMode);
    setSelectionMode(SingleSelection);
    setAttribute(Qt::WA_MacShowFocusRect);
    Q_D(QListView);               // We rely on a qobject_cast for PM_DefaultFrameWidth to change
    d->updateStyledFrameWidths(); // hence we have to force an update now that the object has been constructed
}

/*!
  \internal
*/
QListView::QListView(QListViewPrivate &dd, QWidget *parent)
    : QAbstractItemView(dd, parent)
{
    setViewMode(ListMode);
    setSelectionMode(SingleSelection);
    setAttribute(Qt::WA_MacShowFocusRect);
    Q_D(QListView);               // We rely on a qobject_cast for PM_DefaultFrameWidth to change
    d->updateStyledFrameWidths(); // hence we have to force an update now that the object has been constructed
}

/*!
  Destroys the view.
*/
QListView::~QListView()
{
}

/*!
    \property QListView::movement
    \brief whether the items can be moved freely, are snapped to a
    grid, or cannot be moved at all.

    This property determines how the user can move the items in the
    view. \l Static means that the items can't be moved by the user.
    \l Free means that the user can drag and drop the items to any
    position in the view. \l Snap means that the user can drag and
    drop the items, but only to the positions in a notional grid
    signified by the gridSize property.

    Setting this property when the view is visible will cause the
    items to be laid out again.

    By default, this property is set to \l Static.

    \sa gridSize, resizeMode, viewMode
*/
void QListView::setMovement(Movement movement)
{
    Q_D(QListView);
    d->modeProperties |= uint(QListViewPrivate::Movement);
    d->movement = movement;

#if QT_CONFIG(draganddrop)
    bool movable = (movement != Static);
    setDragEnabled(movable);
    d->viewport->setAcceptDrops(movable);
#endif
    d->doDelayedItemsLayout();
}

QListView::Movement QListView::movement() const
{
    Q_D(const QListView);
    return d->movement;
}

/*!
    \property QListView::flow
    \brief which direction the items layout should flow.

    If this property is \l LeftToRight, the items will be laid out left
    to right. If the \l isWrapping property is \c true, the layout will wrap
    when it reaches the right side of the visible area. If this
    property is \l TopToBottom, the items will be laid out from the top
    of the visible area, wrapping when it reaches the bottom.

    Setting this property when the view is visible will cause the
    items to be laid out again.

    By default, this property is set to \l TopToBottom.

    \sa viewMode
*/
void QListView::setFlow(Flow flow)
{
    Q_D(QListView);
    d->modeProperties |= uint(QListViewPrivate::Flow);
    d->flow = flow;
    d->doDelayedItemsLayout();
}

QListView::Flow QListView::flow() const
{
    Q_D(const QListView);
    return d->flow;
}

/*!
    \property QListView::isWrapping
    \brief whether the items layout should wrap.

    This property holds whether the layout should wrap when there is
    no more space in the visible area. The point at which the layout wraps
    depends on the \l flow property.

    Setting this property when the view is visible will cause the
    items to be laid out again.

    By default, this property is \c false.

    \sa viewMode
*/
void QListView::setWrapping(bool enable)
{
    Q_D(QListView);
    d->modeProperties |= uint(QListViewPrivate::Wrap);
    d->setWrapping(enable);
    d->doDelayedItemsLayout();
}

bool QListView::isWrapping() const
{
    Q_D(const QListView);
    return d->isWrapping();
}

/*!
    \property QListView::resizeMode
    \brief whether the items are laid out again when the view is resized.

    If this property is \l Adjust, the items will be laid out again
    when the view is resized. If the value is \l Fixed, the items will
    not be laid out when the view is resized.

    By default, this property is set to \l Fixed.

    \sa movement, gridSize, viewMode
*/
void QListView::setResizeMode(ResizeMode mode)
{
    Q_D(QListView);
    d->modeProperties |= uint(QListViewPrivate::ResizeMode);
    d->resizeMode = mode;
}

QListView::ResizeMode QListView::resizeMode() const
{
    Q_D(const QListView);
    return d->resizeMode;
}

/*!
    \property QListView::layoutMode
    \brief determines whether the layout of items should happen immediately or be delayed.

    This property holds the layout mode for the items. When the mode
    is \l SinglePass (the default), the items are laid out all in one go.
    When the mode is \l Batched, the items are laid out in batches of \l batchSize
    items, while processing events. This makes it possible to
    instantly view and interact with the visible items while the rest
    are being laid out.

    \sa viewMode
*/
void QListView::setLayoutMode(LayoutMode mode)
{
    Q_D(QListView);
    d->layoutMode = mode;
}

QListView::LayoutMode QListView::layoutMode() const
{
    Q_D(const QListView);
    return d->layoutMode;
}

/*!
    \property QListView::spacing
    \brief the space around the items in the layout

    This property is the size of the empty space that is padded around
    an item in the layout.

    Setting this property when the view is visible will cause the
    items to be laid out again.

    By default, this property contains a value of 0.

    \sa viewMode
*/
void QListView::setSpacing(int space)
{
    Q_D(QListView);
    d->modeProperties |= uint(QListViewPrivate::Spacing);
    d->setSpacing(space);
    d->doDelayedItemsLayout();
}

int QListView::spacing() const
{
    Q_D(const QListView);
    return d->spacing();
}

/*!
    \property QListView::batchSize
    \brief the number of items laid out in each batch if \l layoutMode is
    set to \l Batched.

    The default value is 100.
*/

void QListView::setBatchSize(int batchSize)
{
    Q_D(QListView);
    if (Q_UNLIKELY(batchSize <= 0)) {
        qWarning("Invalid batchSize (%d)", batchSize);
        return;
    }
    d->batchSize = batchSize;
}

int QListView::batchSize() const
{
    Q_D(const QListView);
    return d->batchSize;
}

/*!
    \property QListView::gridSize
    \brief the size of the layout grid

    This property is the size of the grid in which the items are laid
    out. The default is an empty size which means that there is no
    grid and the layout is not done in a grid. Setting this property
    to a non-empty size switches on the grid layout. (When a grid
    layout is in force the \l spacing property is ignored.)

    Setting this property when the view is visible will cause the
    items to be laid out again.

    \sa viewMode
*/
void QListView::setGridSize(const QSize &size)
{
    Q_D(QListView);
    d->modeProperties |= uint(QListViewPrivate::GridSize);
    d->setGridSize(size);
    d->doDelayedItemsLayout();
}

QSize QListView::gridSize() const
{
    Q_D(const QListView);
    return d->gridSize();
}

/*!
    \property QListView::viewMode
    \brief the view mode of the QListView.

    This property will change the other unset properties to conform
    with the set view mode. QListView-specific properties that have already been set
    will not be changed, unless clearPropertyFlags() has been called.

    Setting the view mode will enable or disable drag and drop based on the
    selected movement. For ListMode, the default movement is \l Static
    (drag and drop disabled); for IconMode, the default movement is
    \l Free (drag and drop enabled).

    \sa isWrapping, spacing, gridSize, flow, movement, resizeMode
*/
void QListView::setViewMode(ViewMode mode)
{
    Q_D(QListView);
    if (d->commonListView && d->viewMode == mode)
        return;
    d->viewMode = mode;

    delete d->commonListView;
    if (mode == ListMode) {
        d->commonListView = new QListModeViewBase(this, d);
        if (!(d->modeProperties & QListViewPrivate::Wrap))
            d->setWrapping(false);
        if (!(d->modeProperties & QListViewPrivate::Spacing))
            d->setSpacing(0);
        if (!(d->modeProperties & QListViewPrivate::GridSize))
            d->setGridSize(QSize());
        if (!(d->modeProperties & QListViewPrivate::Flow))
            d->flow = TopToBottom;
        if (!(d->modeProperties & QListViewPrivate::Movement))
            d->movement = Static;
        if (!(d->modeProperties & QListViewPrivate::ResizeMode))
            d->resizeMode = Fixed;
        if (!(d->modeProperties & QListViewPrivate::SelectionRectVisible))
            d->showElasticBand = false;
    } else {
        d->commonListView = new QIconModeViewBase(this, d);
        if (!(d->modeProperties & QListViewPrivate::Wrap))
            d->setWrapping(true);
        if (!(d->modeProperties & QListViewPrivate::Spacing))
            d->setSpacing(0);
        if (!(d->modeProperties & QListViewPrivate::GridSize))
            d->setGridSize(QSize());
        if (!(d->modeProperties & QListViewPrivate::Flow))
            d->flow = LeftToRight;
        if (!(d->modeProperties & QListViewPrivate::Movement))
            d->movement = Free;
        if (!(d->modeProperties & QListViewPrivate::ResizeMode))
            d->resizeMode = Fixed;
        if (!(d->modeProperties & QListViewPrivate::SelectionRectVisible))
            d->showElasticBand = true;
    }

#if QT_CONFIG(draganddrop)
    bool movable = (d->movement != Static);
    setDragEnabled(movable);
    setAcceptDrops(movable);
#endif
    d->clear();
    d->doDelayedItemsLayout();
}

QListView::ViewMode QListView::viewMode() const
{
    Q_D(const QListView);
    return d->viewMode;
}

/*!
    Clears the QListView-specific property flags. See \l{viewMode}.

    Properties inherited from QAbstractItemView are not covered by the
    property flags. Specifically, \l{QAbstractItemView::dragEnabled}
    {dragEnabled} and \l{QAbstractItemView::acceptDrops}
    {acceptsDrops} are computed by QListView when calling
    setMovement() or setViewMode().
*/
void QListView::clearPropertyFlags()
{
    Q_D(QListView);
    d->modeProperties = 0;
}

/*!
    Returns \c true if the \a row is hidden; otherwise returns \c false.
*/
bool QListView::isRowHidden(int row) const
{
    Q_D(const QListView);
    return d->isHidden(row);
}

/*!
    If \a hide is true, the given \a row will be hidden; otherwise
    the \a row will be shown.
*/
void QListView::setRowHidden(int row, bool hide)
{
    Q_D(QListView);
    const bool hidden = d->isHidden(row);
    if (hide && !hidden)
        d->commonListView->appendHiddenRow(row);
    else if (!hide && hidden)
        d->commonListView->removeHiddenRow(row);
    d->doDelayedItemsLayout();
    d->viewport->update();
}

/*!
  \reimp
*/
QRect QListView::visualRect(const QModelIndex &index) const
{
    Q_D(const QListView);
    return d->mapToViewport(rectForIndex(index));
}

/*!
  \reimp
*/
void QListView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    Q_D(QListView);

    if (index.parent() != d->root || index.column() != d->column)
        return;

    const QRect rect = visualRect(index);
    if (!rect.isValid())
        return;
    if (hint == EnsureVisible && d->viewport->rect().contains(rect)) {
        d->viewport->update(rect);
        return;
    }

    if (d->flow == QListView::TopToBottom || d->isWrapping()) // vertical
        verticalScrollBar()->setValue(d->verticalScrollToValue(index, rect, hint));

    if (d->flow == QListView::LeftToRight || d->isWrapping()) // horizontal
        horizontalScrollBar()->setValue(d->horizontalScrollToValue(index, rect, hint));
}

int QListViewPrivate::horizontalScrollToValue(const QModelIndex &index, const QRect &rect,
                                              QListView::ScrollHint hint) const
{
    Q_Q(const QListView);
    const QRect area = viewport->rect();
    const bool leftOf = q->isRightToLeft()
                        ? (rect.left() < area.left()) && (rect.right() < area.right())
                        : rect.left() < area.left();
    const bool rightOf = q->isRightToLeft()
                         ? rect.right() > area.right()
                         : (rect.right() > area.right()) && (rect.left() > area.left());
    return commonListView->horizontalScrollToValue(q->visualIndex(index), hint, leftOf, rightOf, area, rect);
}

int QListViewPrivate::verticalScrollToValue(const QModelIndex &index, const QRect &rect,
                                            QListView::ScrollHint hint) const
{
    Q_Q(const QListView);
    const QRect area = viewport->rect();
    const bool above = (hint == QListView::EnsureVisible && rect.top() < area.top());
    const bool below = (hint == QListView::EnsureVisible && rect.bottom() > area.bottom());
    return commonListView->verticalScrollToValue(q->visualIndex(index), hint, above, below, area, rect);
}

void QListViewPrivate::selectAll(QItemSelectionModel::SelectionFlags command)
{
    if (!selectionModel)
        return;

    QItemSelection selection;
    QModelIndex topLeft;
    int row = 0;
    const int colCount = model->columnCount(root);
    const int rowCount = model->rowCount(root);
    for ( ; row < rowCount; ++row) {
        if (isHidden(row)) {
            //it might be the end of a selection range
            if (topLeft.isValid()) {
                QModelIndex bottomRight = model->index(row - 1, colCount - 1, root);
                selection.append(QItemSelectionRange(topLeft, bottomRight));
                topLeft = QModelIndex();
            }
            continue;
        }

        if (!topLeft.isValid()) //start of a new selection range
            topLeft = model->index(row, 0, root);
    }

    if (topLeft.isValid()) {
        //last selected range
        QModelIndex bottomRight = model->index(row - 1, colCount - 1, root);
        selection.append(QItemSelectionRange(topLeft, bottomRight));
    }

    if (!selection.isEmpty())
        selectionModel->select(selection, command);
}

/*!
  \reimp

  We have a QListView way of knowing what elements are on the viewport
  through the intersectingSet function
*/
QItemViewPaintPairs QListViewPrivate::draggablePaintPairs(const QModelIndexList &indexes, QRect *r) const
{
    Q_ASSERT(r);
    Q_Q(const QListView);
    QRect &rect = *r;
    const QRect viewportRect = viewport->rect();
    QItemViewPaintPairs ret;
    QList<QModelIndex> visibleIndexes =
            intersectingSet(viewportRect.translated(q->horizontalOffset(), q->verticalOffset()));
    std::sort(visibleIndexes.begin(), visibleIndexes.end());
    for (const auto &index : indexes) {
        if (std::binary_search(visibleIndexes.cbegin(), visibleIndexes.cend(), index)) {
            const QRect current = q->visualRect(index);
            ret.append({current, index});
            rect |= current;
        }
    }
    QRect clipped = rect & viewportRect;
    rect.setLeft(clipped.left());
    rect.setRight(clipped.right());
    return ret;
}

/*!
  \internal
*/
void QListView::reset()
{
    Q_D(QListView);
    d->clear();
    d->hiddenRows.clear();
    QAbstractItemView::reset();
}

/*!
  \reimp
*/
void QListView::setRootIndex(const QModelIndex &index)
{
    Q_D(QListView);
    d->column = qMax(0, qMin(d->column, d->model->columnCount(index) - 1));
    QAbstractItemView::setRootIndex(index);
    // sometimes we get an update before reset() is called
    d->clear();
    d->hiddenRows.clear();
}

/*!
    \reimp

    Scroll the view contents by \a dx and \a dy.
*/

void QListView::scrollContentsBy(int dx, int dy)
{
    Q_D(QListView);
    d->delayedAutoScroll.stop(); // auto scroll was canceled by the user scrolling
    d->commonListView->scrollContentsBy(dx, dy, d->state == QListView::DragSelectingState);
}

/*!
    \internal

    Resize the internal contents to \a width and \a height and set the
    scroll bar ranges accordingly.
*/
void QListView::resizeContents(int width, int height)
{
    Q_D(QListView);
    d->setContentsSize(width, height);
}

/*!
    \internal
*/
QSize QListView::contentsSize() const
{
    Q_D(const QListView);
    return d->contentsSize();
}

/*!
  \reimp
*/
void QListView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                            const QList<int> &roles)
{
    d_func()->commonListView->dataChanged(topLeft, bottomRight);
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
}

/*!
  \reimp
*/
void QListView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_D(QListView);
    // ### be smarter about inserted items
    d->clear();
    d->doDelayedItemsLayout();
    QAbstractItemView::rowsInserted(parent, start, end);
}

/*!
  \reimp
*/
void QListView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_D(QListView);
    // if the parent is above d->root in the tree, nothing will happen
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
    if (parent == d->root) {
        QSet<QPersistentModelIndex>::iterator it = d->hiddenRows.begin();
        while (it != d->hiddenRows.end()) {
            int hiddenRow = it->row();
            if (hiddenRow >= start && hiddenRow <= end) {
                it = d->hiddenRows.erase(it);
            } else {
                ++it;
            }
        }
    }
    d->clear();
    d->doDelayedItemsLayout();
}

/*!
  \reimp
*/
void QListView::mouseMoveEvent(QMouseEvent *e)
{
    if (!isVisible())
        return;
    Q_D(QListView);
    QAbstractItemView::mouseMoveEvent(e);
    if (state() == DragSelectingState
        && d->showElasticBand
        && d->selectionMode != SingleSelection
        && d->selectionMode != NoSelection) {
        QRect rect(d->pressedPosition, e->position().toPoint() + QPoint(horizontalOffset(), verticalOffset()));
        rect = rect.normalized();
        const int margin = 2 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
        const QRect viewPortRect = rect.united(d->elasticBand)
                                       .adjusted(-margin, -margin, margin, margin);
        d->viewport->update(d->mapToViewport(viewPortRect));
        d->elasticBand = rect;
    }
}

/*!
  \reimp
*/
void QListView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QListView);
    QAbstractItemView::mouseReleaseEvent(e);
    // #### move this implementation into a dynamic class
    if (d->showElasticBand && d->elasticBand.isValid()) {
        const int margin = 2 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
        const QRect viewPortRect = d->elasticBand.adjusted(-margin, -margin, margin, margin);
        d->viewport->update(d->mapToViewport(viewPortRect));
        d->elasticBand = QRect();
    }
}

#if QT_CONFIG(wheelevent)
/*!
  \reimp
*/
void QListView::wheelEvent(QWheelEvent *e)
{
    Q_D(QListView);
    if (qAbs(e->angleDelta().y()) > qAbs(e->angleDelta().x())) {
        if (e->angleDelta().x() == 0
                && ((d->flow == TopToBottom && d->wrap) || (d->flow == LeftToRight && !d->wrap))
                && d->vbar->minimum() == 0 && d->vbar->maximum() == 0) {
            QPoint pixelDelta(e->pixelDelta().y(), e->pixelDelta().x());
            QPoint angleDelta(e->angleDelta().y(), e->angleDelta().x());
            QWheelEvent hwe(e->position(), e->globalPosition(), pixelDelta, angleDelta,
                            e->buttons(), e->modifiers(), e->phase(), e->inverted(), e->source());
            if (e->spontaneous())
                qt_sendSpontaneousEvent(d->hbar, &hwe);
            else
                QCoreApplication::sendEvent(d->hbar, &hwe);
            e->setAccepted(hwe.isAccepted());
        } else {
            QCoreApplication::sendEvent(d->vbar, e);
        }
    } else {
        QCoreApplication::sendEvent(d->hbar, e);
    }
}
#endif // QT_CONFIG(wheelevent)

/*!
  \reimp
*/
void QListView::timerEvent(QTimerEvent *e)
{
    Q_D(QListView);
    if (e->timerId() == d->batchLayoutTimer.timerId()) {
        if (d->doItemsLayout(d->batchSize)) { // layout is done
            d->batchLayoutTimer.stop();
            updateGeometries();
            d->viewport->update();
        }
    }
    QAbstractItemView::timerEvent(e);
}

/*!
  \reimp
*/
void QListView::resizeEvent(QResizeEvent *e)
{
    Q_D(QListView);
    if (d->delayedPendingLayout)
        return;

    QSize delta = e->size() - e->oldSize();

    if (delta.isNull())
      return;

    bool listWrap = (d->viewMode == ListMode) && d->wrapItemText;
    bool flowDimensionChanged = (d->flow == LeftToRight && delta.width() != 0)
                                || (d->flow == TopToBottom && delta.height() != 0);

    // We post a delayed relayout in the following cases :
    // - we're wrapping
    // - the state is NoState, we're adjusting and the size has changed in the flowing direction
    if (listWrap
        || (state() == NoState && d->resizeMode == Adjust && flowDimensionChanged)) {
        d->doDelayedItemsLayout(100); // wait 1/10 sec before starting the layout
    } else {
        QAbstractItemView::resizeEvent(e);
    }
}

#if QT_CONFIG(draganddrop)

/*!
  \reimp
*/
void QListView::dragMoveEvent(QDragMoveEvent *e)
{
    Q_D(QListView);
    if (!d->commonListView->filterDragMoveEvent(e)) {
        if (viewMode() == QListView::ListMode && flow() == QListView::LeftToRight)
            static_cast<QListModeViewBase *>(d->commonListView)->dragMoveEvent(e);
        else
            QAbstractItemView::dragMoveEvent(e);
    }
}


/*!
  \reimp
*/
void QListView::dragLeaveEvent(QDragLeaveEvent *e)
{
    if (!d_func()->commonListView->filterDragLeaveEvent(e))
        QAbstractItemView::dragLeaveEvent(e);
}

/*!
  \reimp
*/
void QListView::dropEvent(QDropEvent *event)
{
    Q_D(QListView);

    const bool moveAction = event->dropAction() == Qt::MoveAction
                         || dragDropMode() == QAbstractItemView::InternalMove;
    if (event->source() == this && moveAction) {
        QModelIndex topIndex;
        bool topIndexDropped = false;
        int col = -1;
        int row = -1;
        // check whether a subclass has already accepted the event, ie. moved the data
        if (!event->isAccepted() && d->dropOn(event, &row, &col, &topIndex)) {
            const QList<QModelIndex> selIndexes = selectedIndexes();
            QList<QPersistentModelIndex> persIndexes;
            persIndexes.reserve(selIndexes.size());

            for (const auto &index : selIndexes) {
                persIndexes.append(index);
                if (index == topIndex) {
                    topIndexDropped = true;
                    break;
                }
            }

            if (!topIndexDropped && !topIndex.isValid()) {
                std::sort(persIndexes.begin(), persIndexes.end()); // The dropped items will remain in the same visual order.

                QPersistentModelIndex dropRow = model()->index(row, col, topIndex);

                int r = row == -1 ? model()->rowCount() : (dropRow.row() >= 0 ? dropRow.row() : row);
                bool dataMoved = false;
                for (const QPersistentModelIndex &pIndex : std::as_const(persIndexes)) {
                    // only generate a move when not same row or behind itself
                    if (r != pIndex.row() && r != pIndex.row() + 1) {
                        // try to move (preserves selection)
                        const bool moved = model()->moveRow(QModelIndex(), pIndex.row(), QModelIndex(), r);
                        if (!moved)
                            continue; // maybe it'll work for other rows
                        dataMoved = true; // success
                    } else {
                        // move onto itself is blocked, don't delete anything
                        dataMoved = true;
                    }
                    r = pIndex.row() + 1;   // Dropped items are inserted contiguously and in the right order.
                }
                if (dataMoved)
                    event->accept();
            }
        }

        // either we or a subclass accepted the move event, so assume that the data was
        // moved and that QAbstractItemView shouldn't remove the source when QDrag::exec returns
        if (event->isAccepted())
            d->dropEventMoved = true;
    }

    if (!d->commonListView->filterDropEvent(event) || !d->dropEventMoved) {
        // icon view didn't move the data, and moveRows not implemented, so fall back to default
        if (!d->dropEventMoved && moveAction)
            event->ignore();
        QAbstractItemView::dropEvent(event);
    }
}

/*!
  \reimp
*/
void QListView::startDrag(Qt::DropActions supportedActions)
{
    if (!d_func()->commonListView->filterStartDrag(supportedActions))
        QAbstractItemView::startDrag(supportedActions);
}

#endif // QT_CONFIG(draganddrop)

/*!
  \reimp
*/
void QListView::initViewItemOption(QStyleOptionViewItem *option) const
{
    Q_D(const QListView);
    QAbstractItemView::initViewItemOption(option);
    if (!d->iconSize.isValid()) { // otherwise it was already set in abstractitemview
        int pm = (d->viewMode == QListView::ListMode
                  ? style()->pixelMetric(QStyle::PM_ListViewIconSize, nullptr, this)
                  : style()->pixelMetric(QStyle::PM_IconViewIconSize, nullptr, this));
        option->decorationSize = QSize(pm, pm);
    }
    if (d->viewMode == QListView::IconMode) {
        option->showDecorationSelected = false;
        option->decorationPosition = QStyleOptionViewItem::Top;
        option->displayAlignment = Qt::AlignCenter;
    } else {
        option->decorationPosition = QStyleOptionViewItem::Left;
    }

    if (d->gridSize().isValid()) {
        option->rect.setSize(d->gridSize());
    }
    option->viewItemPosition = QStyleOptionViewItem::OnlyOne;
}


/*!
  \reimp
*/
void QListView::paintEvent(QPaintEvent *e)
{
    Q_D(QListView);
    if (!d->itemDelegate)
        return;
    QStyleOptionViewItem option;
    initViewItemOption(&option);
    QStylePainter painter(d->viewport);

    const QList<QModelIndex> toBeRendered =
            d->intersectingSet(e->rect().translated(horizontalOffset(), verticalOffset()), false);

    const QModelIndex current = currentIndex();
    const QModelIndex hover = d->hover;
    const QAbstractItemModel *itemModel = d->model;
    const QItemSelectionModel *selections = d->selectionModel;
    const bool focus = (hasFocus() || d->viewport->hasFocus()) && current.isValid();
    const bool alternate = d->alternatingColors;
    const QStyle::State state = option.state;
    const QAbstractItemView::State viewState = this->state();
    const bool enabled = (state & QStyle::State_Enabled) != 0;

    bool alternateBase = false;
    int previousRow = -2; // trigger the alternateBase adjustment on first pass

    int maxSize = (flow() == TopToBottom)
        ? qMax(viewport()->size().width(), d->contentsSize().width()) - 2 * d->spacing()
        : qMax(viewport()->size().height(), d->contentsSize().height()) - 2 * d->spacing();

    QList<QModelIndex>::const_iterator end = toBeRendered.constEnd();
    for (QList<QModelIndex>::const_iterator it = toBeRendered.constBegin(); it != end; ++it) {
        Q_ASSERT((*it).isValid());
        option.rect = visualRect(*it);

        if (flow() == TopToBottom)
            option.rect.setWidth(qMin(maxSize, option.rect.width()));
        else
            option.rect.setHeight(qMin(maxSize, option.rect.height()));

        option.state = state;
        if (selections && selections->isSelected(*it))
            option.state |= QStyle::State_Selected;
        if (enabled) {
            QPalette::ColorGroup cg;
            if ((itemModel->flags(*it) & Qt::ItemIsEnabled) == 0) {
                option.state &= ~QStyle::State_Enabled;
                cg = QPalette::Disabled;
            } else {
                cg = QPalette::Normal;
            }
            option.palette.setCurrentColorGroup(cg);
        }
        if (focus && current == *it) {
            option.state |= QStyle::State_HasFocus;
            if (viewState == EditingState)
                option.state |= QStyle::State_Editing;
        }
        option.state.setFlag(QStyle::State_MouseOver, *it == hover);

        if (alternate) {
            int row = (*it).row();
            if (row != previousRow + 1) {
                // adjust alternateBase according to rows in the "gap"
                if (!d->hiddenRows.isEmpty()) {
                    for (int r = qMax(previousRow + 1, 0); r < row; ++r) {
                        if (!d->isHidden(r))
                            alternateBase = !alternateBase;
                    }
                } else {
                    alternateBase = (row & 1) != 0;
                }
            }
            option.features.setFlag(QStyleOptionViewItem::Alternate, alternateBase);

            // draw background of the item (only alternate row). rest of the background
            // is provided by the delegate
            QStyle::State oldState = option.state;
            option.state &= ~QStyle::State_Selected;
            painter.drawPrimitive(QStyle::PE_PanelItemViewRow, option);
            option.state = oldState;

            alternateBase = !alternateBase;
            previousRow = row;
        }

        itemDelegateForIndex(*it)->paint(&painter, option, *it);
    }

#if QT_CONFIG(draganddrop)
    d->commonListView->paintDragDrop(&painter);
#endif

#if QT_CONFIG(rubberband)
    // #### move this implementation into a dynamic class
    if (d->showElasticBand && d->elasticBand.isValid()) {
        QStyleOptionRubberBand opt;
        opt.initFrom(this);
        opt.shape = QRubberBand::Rectangle;
        opt.opaque = false;
        opt.rect = d->mapToViewport(d->elasticBand, false).intersected(
            d->viewport->rect().adjusted(-16, -16, 16, 16));
        painter.save();
        painter.drawControl(QStyle::CE_RubberBand, opt);
        painter.restore();
    }
#endif
}

/*!
  \reimp
*/
QModelIndex QListView::indexAt(const QPoint &p) const
{
    Q_D(const QListView);
    QRect rect(p.x() + horizontalOffset(), p.y() + verticalOffset(), 1, 1);
    const QList<QModelIndex> intersectVector = d->intersectingSet(rect);
    QModelIndex index = intersectVector.size() > 0
                        ? intersectVector.last() : QModelIndex();
    if (index.isValid() && visualRect(index).contains(p))
        return index;
    return QModelIndex();
}

/*!
  \reimp
*/
int QListView::horizontalOffset() const
{
    return d_func()->commonListView->horizontalOffset();
}

/*!
  \reimp
*/
int QListView::verticalOffset() const
{
    return d_func()->commonListView->verticalOffset();
}

/*!
  \reimp
*/
QModelIndex QListView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    Q_D(QListView);
    Q_UNUSED(modifiers);

    auto findAvailableRowBackward = [d](int row) {
        while (row >= 0 && d->isHiddenOrDisabled(row))
            --row;
        return row;
    };

    auto findAvailableRowForward = [d](int row) {
        int rowCount = d->model->rowCount(d->root);
        if (!rowCount)
            return -1;
        while (row < rowCount && d->isHiddenOrDisabled(row))
            ++row;
        if (row >= rowCount)
            return -1;
        return row;
    };

    QModelIndex current = currentIndex();
    if (!current.isValid()) {
        int row = findAvailableRowForward(0);
        if (row == -1)
            return QModelIndex();
        return d->model->index(row, d->column, d->root);
    }

    if ((d->flow == LeftToRight && cursorAction == MoveLeft) ||
            (d->flow == TopToBottom && (cursorAction == MoveUp || cursorAction == MovePrevious))) {
        const int row = findAvailableRowBackward(current.row() - 1);
        if (row == -1)
            return current;
        return d->model->index(row, d->column, d->root);
    } else if ((d->flow == LeftToRight && cursorAction == MoveRight) ||
               (d->flow == TopToBottom && (cursorAction == MoveDown || cursorAction == MoveNext))) {
        const int row = findAvailableRowForward(current.row() + 1);
        if (row == -1)
            return current;
        return d->model->index(row, d->column, d->root);
    }

    const QRect initialRect = rectForIndex(current);
    QRect rect = initialRect;
    if (rect.isEmpty()) {
        return d->model->index(0, d->column, d->root);
    }
    if (d->gridSize().isValid()) rect.setSize(d->gridSize());

    QSize contents = d->contentsSize();
    QList<QModelIndex> intersectVector;

    switch (cursorAction) {
    case MoveLeft:
        while (intersectVector.isEmpty()) {
            rect.translate(-rect.width(), 0);
            if (rect.right() <= 0)
                return current;
            if (rect.left() < 0)
                rect.setLeft(0);
            intersectVector = d->intersectingSet(rect);
            d->removeCurrentAndDisabled(&intersectVector, current);
        }
        return d->closestIndex(initialRect, intersectVector);
    case MoveRight:
        while (intersectVector.isEmpty()) {
            rect.translate(rect.width(), 0);
            if (rect.left() >= contents.width())
                return current;
            if (rect.right() > contents.width())
                rect.setRight(contents.width());
            intersectVector = d->intersectingSet(rect);
            d->removeCurrentAndDisabled(&intersectVector, current);
        }
        return d->closestIndex(initialRect, intersectVector);
    case MovePageUp: {
        if (rect.height() >= d->viewport->height())
           return moveCursor(QAbstractItemView::MoveUp, modifiers);

        rect.moveTop(rect.top() - d->viewport->height() + 1);
        if (rect.top() < rect.height()) {
            rect.setTop(0);
            rect.setBottom(1);
        }
        QModelIndex findindex = current;
        while (intersectVector.isEmpty()
               || rectForIndex(findindex).top() <= (rectForIndex(current).bottom() - d->viewport->rect().height())
               || rect.top() <= 0) {
            rect.translate(0, 1);
            if (rect.bottom() <= 0) {
                return current;
            }
            intersectVector = d->intersectingSet(rect);
            findindex = d->closestIndex(initialRect, intersectVector);
        }
        return findindex;
    }
    case MovePrevious:
    case MoveUp:
        while (intersectVector.isEmpty()) {
            rect.translate(0, -rect.height());
            if (rect.bottom() <= 0) {
#ifdef QT_KEYPAD_NAVIGATION
                if (QApplicationPrivate::keypadNavigationEnabled()) {
                    int row = d->batchStartRow() - 1;
                    while (row >= 0 && d->isHiddenOrDisabled(row))
                        --row;
                    if (row >= 0)
                        return d->model->index(row, d->column, d->root);
                }
#endif
                return current;
            }
            if (rect.top() < 0)
                rect.setTop(0);
            intersectVector = d->intersectingSet(rect);
            d->removeCurrentAndDisabled(&intersectVector, current);
        }
        return d->closestIndex(initialRect, intersectVector);
    case MovePageDown: {
        if (rect.height() >= d->viewport->height())
           return moveCursor(QAbstractItemView::MoveDown, modifiers);

        rect.moveTop(rect.top() + d->viewport->height() - 1);
        if (rect.bottom() > contents.height() - rect.height()) {
            rect.setTop(contents.height() - 1);
            rect.setBottom(contents.height());
        }
        QModelIndex index = current;
        // index's bottom() - current's top() always <=  (d->viewport->rect().height()
        while (intersectVector.isEmpty()
               || rectForIndex(index).bottom() >= (d->viewport->rect().height() + rectForIndex(current).top())
               || rect.bottom() > contents.height()) {
            rect.translate(0, -1);
            if (rect.top() >= contents.height()) {
                return current;
            }
            intersectVector = d->intersectingSet(rect);
            index = d->closestIndex(initialRect, intersectVector);
        }
        return index;
    }
    case MoveNext:
    case MoveDown:
        while (intersectVector.isEmpty()) {
            rect.translate(0, rect.height());
            if (rect.top() >= contents.height()) {
#ifdef QT_KEYPAD_NAVIGATION
                if (QApplicationPrivate::keypadNavigationEnabled()) {
                    int rowCount = d->model->rowCount(d->root);
                    int row = 0;
                    while (row < rowCount && d->isHiddenOrDisabled(row))
                        ++row;
                    if (row < rowCount)
                        return d->model->index(row, d->column, d->root);
                }
#endif
                return current;
            }
            if (rect.bottom() > contents.height())
                rect.setBottom(contents.height());
            intersectVector = d->intersectingSet(rect);
            d->removeCurrentAndDisabled(&intersectVector, current);
        }
        return d->closestIndex(initialRect, intersectVector);
    case MoveHome:
        return d->model->index(0, d->column, d->root);
    case MoveEnd:
        return d->model->index(d->batchStartRow() - 1, d->column, d->root);}

    return current;
}

/*!
    Returns the rectangle of the item at position \a index in the
    model. The rectangle is in contents coordinates.

    \sa visualRect()
*/
QRect QListView::rectForIndex(const QModelIndex &index) const
{
    return d_func()->rectForIndex(index);
}

/*!
    Sets the contents position of the item at \a index in the model to the given
    \a position.
    If the list view's movement mode is Static or its view mode is ListView,
    this function will have no effect.
*/
void QListView::setPositionForIndex(const QPoint &position, const QModelIndex &index)
{
    Q_D(QListView);
    if (d->movement == Static
        || !d->isIndexValid(index)
        || index.parent() != d->root
        || index.column() != d->column)
        return;

    d->executePostedLayout();
    d->commonListView->setPositionForIndex(position, index);
}

/*!
  \reimp
*/
void QListView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    Q_D(QListView);
    if (!d->selectionModel)
        return;

    // if we are wrapping, we can only select inside the contents rectangle
    int w = qMax(d->contentsSize().width(), d->viewport->width());
    int h = qMax(d->contentsSize().height(), d->viewport->height());
    if (d->wrap && !QRect(0, 0, w, h).intersects(rect))
        return;

    QItemSelection selection;

    if (rect.width() == 1 && rect.height() == 1) {
        const QList<QModelIndex> intersectVector =
                d->intersectingSet(rect.translated(horizontalOffset(), verticalOffset()));
        QModelIndex tl;
        if (!intersectVector.isEmpty())
            tl = intersectVector.last(); // special case for mouse press; only select the top item
        if (tl.isValid() && d->isIndexEnabled(tl))
            selection.select(tl, tl);
    } else {
        if (state() == DragSelectingState) { // visual selection mode (rubberband selection)
            selection = d->selection(rect.translated(horizontalOffset(), verticalOffset()));
        } else { // logical selection mode (key and mouse click selection)
            QModelIndex tl, br;
            // get the first item
            const QRect topLeft(rect.left() + horizontalOffset(), rect.top() + verticalOffset(), 1, 1);
            QList<QModelIndex> intersectVector = d->intersectingSet(topLeft);
            if (!intersectVector.isEmpty())
                tl = intersectVector.last();
            // get the last item
            const QRect bottomRight(rect.right() + horizontalOffset(), rect.bottom() + verticalOffset(), 1, 1);
            intersectVector = d->intersectingSet(bottomRight);
            if (!intersectVector.isEmpty())
                br = intersectVector.last();

            // get the ranges
            if (tl.isValid() && br.isValid()
                && d->isIndexEnabled(tl)
                && d->isIndexEnabled(br)) {
                QRect first = d->cellRectForIndex(tl);
                QRect last = d->cellRectForIndex(br);
                QRect middle;
                if (d->flow == LeftToRight) {
                    QRect &top = first;
                    QRect &bottom = last;
                    // if bottom is above top, swap them
                    if (top.center().y() > bottom.center().y()) {
                        QRect tmp = top;
                        top = bottom;
                        bottom = tmp;
                    }
                    // if the rect are on different lines, expand
                    if (top.top() != bottom.top()) {
                        // top rectangle
                        if (isRightToLeft())
                            top.setLeft(0);
                        else
                            top.setRight(contentsSize().width());
                        // bottom rectangle
                        if (isRightToLeft())
                            bottom.setRight(contentsSize().width());
                        else
                            bottom.setLeft(0);
                    } else if (top.left() > bottom.right()) {
                        if (isRightToLeft())
                            bottom.setLeft(top.right());
                        else
                            bottom.setRight(top.left());
                    } else {
                        if (isRightToLeft())
                            top.setLeft(bottom.right());
                        else
                            top.setRight(bottom.left());
                    }
                    // middle rectangle
                    if (top.bottom() < bottom.top()) {
                        if (gridSize().isValid() && !gridSize().isNull())
                            middle.setTop(top.top() + gridSize().height());
                        else
                            middle.setTop(top.bottom() + 1);
                        middle.setLeft(qMin(top.left(), bottom.left()));
                        middle.setBottom(bottom.top() - 1);
                        middle.setRight(qMax(top.right(), bottom.right()));
                    }
                } else {    // TopToBottom
                    QRect &left = first;
                    QRect &right = last;
                    if (left.center().x() > right.center().x())
                        qSwap(left, right);

                    int ch = contentsSize().height();
                    if (left.left() != right.left()) {
                        // left rectangle
                        if (isRightToLeft())
                            left.setTop(0);
                        else
                            left.setBottom(ch);

                        // top rectangle
                        if (isRightToLeft())
                            right.setBottom(ch);
                        else
                            right.setTop(0);
                        // only set middle if the
                        middle.setTop(0);
                        middle.setBottom(ch);
                        if (gridSize().isValid() && !gridSize().isNull())
                            middle.setLeft(left.left() + gridSize().width());
                        else
                            middle.setLeft(left.right() + 1);
                        middle.setRight(right.left() - 1);
                    } else if (left.bottom() < right.top()) {
                        left.setBottom(right.top() - 1);
                    } else {
                        right.setBottom(left.top() - 1);
                    }
                }

                // do the selections
                QItemSelection topSelection = d->selection(first);
                QItemSelection middleSelection = d->selection(middle);
                QItemSelection bottomSelection = d->selection(last);
                // merge
                selection.merge(topSelection, QItemSelectionModel::Select);
                selection.merge(middleSelection, QItemSelectionModel::Select);
                selection.merge(bottomSelection, QItemSelectionModel::Select);
            }
        }
    }

    d->selectionModel->select(selection, command);
}

/*!
  \reimp

  Since 4.7, the returned region only contains rectangles intersecting
  (or included in) the viewport.
*/
QRegion QListView::visualRegionForSelection(const QItemSelection &selection) const
{
    Q_D(const QListView);
    // ### NOTE: this is a potential bottleneck in non-static mode
    int c = d->column;
    QRegion selectionRegion;
    const QRect &viewportRect = d->viewport->rect();
    for (const auto &elem : selection) {
        if (!elem.isValid())
            continue;
        QModelIndex parent = elem.topLeft().parent();
        //we only display the children of the root in a listview
        //we're not interested in the other model indexes
        if (parent != d->root)
            continue;
        int t = elem.topLeft().row();
        int b = elem.bottomRight().row();
        if (d->viewMode == IconMode || d->isWrapping()) { // in non-static mode, we have to go through all selected items
            for (int r = t; r <= b; ++r) {
                const QRect &rect = visualRect(d->model->index(r, c, parent));
                if (viewportRect.intersects(rect))
                    selectionRegion += rect;
            }
        } else { // in static mode, we can optimize a bit
            while (t <= b && d->isHidden(t)) ++t;
            while (b >= t && d->isHidden(b)) --b;
            const QModelIndex top = d->model->index(t, c, parent);
            const QModelIndex bottom = d->model->index(b, c, parent);
            QRect rect(visualRect(top).topLeft(),
                       visualRect(bottom).bottomRight());
            if (viewportRect.intersects(rect))
                selectionRegion += rect;
        }
    }

    return selectionRegion;
}

/*!
  \reimp
*/
QModelIndexList QListView::selectedIndexes() const
{
    Q_D(const QListView);
    if (!d->selectionModel)
        return QModelIndexList();

    QModelIndexList viewSelected = d->selectionModel->selectedIndexes();
    auto ignorable = [this, d](const QModelIndex &index) {
        return index.column() != d->column || index.parent() != d->root || isIndexHidden(index);
    };
    viewSelected.removeIf(ignorable);
    return viewSelected;
}

/*!
    \internal

    Layout the items according to the flow and wrapping properties.
*/
void QListView::doItemsLayout()
{
    Q_D(QListView);
    // showing the scroll bars will trigger a resize event,
    // so we set the state to expanding to avoid
    // triggering another layout
    QAbstractItemView::State oldState = state();
    setState(ExpandingState);
    if (d->model->columnCount(d->root) > 0) { // no columns means no contents
        d->resetBatchStartRow();
        if (layoutMode() == SinglePass) {
            d->doItemsLayout(d->model->rowCount(d->root)); // layout everything
        } else if (!d->batchLayoutTimer.isActive()) {
            if (!d->doItemsLayout(d->batchSize)) // layout is done
                d->batchLayoutTimer.start(0, this); // do a new batch as fast as possible
        }
    } else { // clear the QBspTree generated by the last layout
        d->clear();
    }
    QAbstractItemView::doItemsLayout();
    setState(oldState);        // restoring the oldState
}

/*!
  \reimp
*/
void QListView::updateGeometries()
{
    Q_D(QListView);
    if (geometry().isEmpty() || d->model->rowCount(d->root) <= 0 || d->model->columnCount(d->root) <= 0) {
        horizontalScrollBar()->setRange(0, 0);
        verticalScrollBar()->setRange(0, 0);
    } else {
        QModelIndex index = d->model->index(0, d->column, d->root);
        QStyleOptionViewItem option;
        initViewItemOption(&option);
        QSize step = d->itemSize(option, index);
        d->commonListView->updateHorizontalScrollBar(step);
        d->commonListView->updateVerticalScrollBar(step);
    }

    QAbstractItemView::updateGeometries();

    // if the scroll bars are turned off, we resize the contents to the viewport
    if (d->movement == Static && !d->isWrapping()) {
        d->layoutChildren(); // we need the viewport size to be updated
        if (d->flow == TopToBottom) {
            if (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff) {
                d->setContentsSize(viewport()->width(), contentsSize().height());
                horizontalScrollBar()->setRange(0, 0); // we see all the contents anyway
            }
        } else { // LeftToRight
            if (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff) {
                d->setContentsSize(contentsSize().width(), viewport()->height());
                verticalScrollBar()->setRange(0, 0); // we see all the contents anyway
            }
        }
    }

}

/*!
  \reimp
*/
bool QListView::isIndexHidden(const QModelIndex &index) const
{
    Q_D(const QListView);
    return (d->isHidden(index.row())
            && (index.parent() == d->root)
            && index.column() == d->column);
}

/*!
    \property QListView::modelColumn
    \brief the column in the model that is visible

    By default, this property contains 0, indicating that the first
    column in the model will be shown.
*/
void QListView::setModelColumn(int column)
{
    Q_D(QListView);
    if (column < 0 || column >= d->model->columnCount(d->root))
        return;
    d->column = column;
    d->doDelayedItemsLayout();
#if QT_CONFIG(accessibility)
    if (QAccessible::isActive()) {
        QAccessibleTableModelChangeEvent event(this, QAccessibleTableModelChangeEvent::ModelReset);
        QAccessible::updateAccessibility(&event);
    }
#endif
}

int QListView::modelColumn() const
{
    Q_D(const QListView);
    return d->column;
}

/*!
    \property QListView::uniformItemSizes
    \brief whether all items in the listview have the same size

    This property should only be set to true if it is guaranteed that all items
    in the view have the same size. This enables the view to do some
    optimizations for performance purposes.

    By default, this property is \c false.
*/
void QListView::setUniformItemSizes(bool enable)
{
    Q_D(QListView);
    d->uniformItemSizes = enable;
}

bool QListView::uniformItemSizes() const
{
    Q_D(const QListView);
    return d->uniformItemSizes;
}

/*!
    \property QListView::wordWrap
    \brief the item text word-wrapping policy

    If this property is \c true then the item text is wrapped where
    necessary at word-breaks; otherwise it is not wrapped at all.
    This property is \c false by default.

    Please note that even if wrapping is enabled, the cell will not be
    expanded to make room for the text. It will print ellipsis for
    text that cannot be shown, according to the view's
    \l{QAbstractItemView::}{textElideMode}.
*/
void QListView::setWordWrap(bool on)
{
    Q_D(QListView);
    if (d->wrapItemText == on)
        return;
    d->wrapItemText = on;
    d->doDelayedItemsLayout();
}

bool QListView::wordWrap() const
{
    Q_D(const QListView);
    return d->wrapItemText;
}

/*!
    \property QListView::selectionRectVisible
    \brief if the selection rectangle should be visible

    If this property is \c true then the selection rectangle is visible;
    otherwise it will be hidden.

    \note The selection rectangle will only be visible if the selection mode
    is in a mode where more than one item can be selected; i.e., it will not
    draw a selection rectangle if the selection mode is
    QAbstractItemView::SingleSelection.

    By default, this property is \c false.
*/
void QListView::setSelectionRectVisible(bool show)
{
    Q_D(QListView);
    d->modeProperties |= uint(QListViewPrivate::SelectionRectVisible);
    d->setSelectionRectVisible(show);
}

bool QListView::isSelectionRectVisible() const
{
    Q_D(const QListView);
    return d->isSelectionRectVisible();
}

/*!
    \property QListView::itemAlignment
    \brief the alignment of each item in its cell
    \since 5.12

    This is only supported in ListMode with TopToBottom flow
    and with wrapping enabled.
    The default alignment is 0, which means that an item fills
    its cell entirely.
*/
void QListView::setItemAlignment(Qt::Alignment alignment)
{
    Q_D(QListView);
    if (d->itemAlignment == alignment)
        return;
    d->itemAlignment = alignment;
    if (viewMode() == ListMode && flow() == QListView::TopToBottom && isWrapping())
        d->doDelayedItemsLayout();
}

Qt::Alignment QListView::itemAlignment() const
{
    Q_D(const QListView);
    return d->itemAlignment;
}

/*!
    \reimp
*/
bool QListView::event(QEvent *e)
{
    return QAbstractItemView::event(e);
}

/*
 * private object implementation
 */

QListViewPrivate::QListViewPrivate()
    : QAbstractItemViewPrivate(),
      commonListView(nullptr),
      wrap(false),
      space(0),
      flow(QListView::TopToBottom),
      movement(QListView::Static),
      resizeMode(QListView::Fixed),
      layoutMode(QListView::SinglePass),
      viewMode(QListView::ListMode),
      modeProperties(0),
      column(0),
      uniformItemSizes(false),
      batchSize(100),
      showElasticBand(false),
      itemAlignment(Qt::Alignment())
{
}

QListViewPrivate::~QListViewPrivate()
{
    delete commonListView;
}

void QListViewPrivate::clear()
{
    // initialization of data structs
    cachedItemSize = QSize();
    commonListView->clear();
}

void QListViewPrivate::prepareItemsLayout()
{
    Q_Q(QListView);
    clear();

    //take the size as if there were scrollbar in order to prevent scrollbar to blink
    layoutBounds = QRect(QPoint(), q->maximumViewportSize());

    int frameAroundContents = 0;
    if (q->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, nullptr, q)) {
        QStyleOption option;
        option.initFrom(q);
        frameAroundContents = q->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, q) * 2;
    }

    // maximumViewportSize() already takes scrollbar into account if policy is
    // Qt::ScrollBarAlwaysOn but scrollbar extent must be deduced if policy
    // is Qt::ScrollBarAsNeeded
    int verticalMargin = (vbarpolicy == Qt::ScrollBarAsNeeded) && (flow == QListView::LeftToRight || vbar->isVisible())
                        && !q->style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarOverlap, nullptr, vbar)
        ? q->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, vbar) + frameAroundContents
        : 0;
    int horizontalMargin =  hbarpolicy==Qt::ScrollBarAsNeeded
        ? q->style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, hbar) + frameAroundContents
        : 0;

    layoutBounds.adjust(0, 0, -verticalMargin, -horizontalMargin);

    int rowCount = model->columnCount(root) <= 0 ? 0 : model->rowCount(root);
    commonListView->setRowCount(rowCount);
}

/*!
  \internal
*/
bool QListViewPrivate::doItemsLayout(int delta)
{
    int max = model->rowCount(root) - 1;
    int first = batchStartRow();
    int last = qMin(first + delta - 1, max);

    if (first == 0) {
        layoutChildren(); // make sure the viewport has the right size
        prepareItemsLayout();
    }

    if (max < 0 || last < first) {
        return true; // nothing to do
    }

    QListViewLayoutInfo info;
    info.bounds = layoutBounds;
    info.grid = gridSize();
    info.spacing = (info.grid.isValid() ? 0 : spacing());
    info.first = first;
    info.last = last;
    info.wrap = isWrapping();
    info.flow = flow;
    info.max = max;

    return commonListView->doBatchedItemLayout(info, max);
}

QListViewItem QListViewPrivate::indexToListViewItem(const QModelIndex &index) const
{
    if (!index.isValid() || isHidden(index.row()))
        return QListViewItem();

    return commonListView->indexToListViewItem(index);
}

QRect QListViewPrivate::mapToViewport(const QRect &rect, bool extend) const
{
    Q_Q(const QListView);
    if (!rect.isValid())
        return rect;

    QRect result = extend ? commonListView->mapToViewport(rect) : rect;
    int dx = -q->horizontalOffset();
    int dy = -q->verticalOffset();
    return result.adjusted(dx, dy, dx, dy);
}

QModelIndex QListViewPrivate::closestIndex(const QRect &target,
                                           const QList<QModelIndex> &candidates) const
{
    int distance = 0;
    int shortest = INT_MAX;
    QModelIndex closest;
    QList<QModelIndex>::const_iterator it = candidates.begin();

    for (; it != candidates.end(); ++it) {
        if (!(*it).isValid())
            continue;

        const QRect indexRect = indexToListViewItem(*it).rect();

        //if the center x (or y) position of an item is included in the rect of the other item,
        //we define the distance between them as the difference in x (or y) of their respective center.
        // Otherwise, we use the nahattan  length between the 2 items
        if ((target.center().x() >= indexRect.x() && target.center().x() < indexRect.right())
            || (indexRect.center().x() >= target.x() && indexRect.center().x() < target.right())) {
                //one item's center is at the vertical of the other
                distance = qAbs(indexRect.center().y() - target.center().y());
        } else if ((target.center().y() >= indexRect.y() && target.center().y() < indexRect.bottom())
            || (indexRect.center().y() >= target.y() && indexRect.center().y() < target.bottom())) {
                //one item's center is at the vertical of the other
                distance = qAbs(indexRect.center().x() - target.center().x());
        } else {
            distance = (indexRect.center() - target.center()).manhattanLength();
        }
        if (distance < shortest) {
            shortest = distance;
            closest = *it;
        }
    }
    return closest;
}

QSize QListViewPrivate::itemSize(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_Q(const QListView);
    if (!uniformItemSizes) {
        const QAbstractItemDelegate *delegate = q->itemDelegateForIndex(index);
        return delegate ? delegate->sizeHint(option, index) : QSize();
    }
    if (!cachedItemSize.isValid()) { // the last item is probably the largest, so we use its size
        int row = model->rowCount(root) - 1;
        QModelIndex sample = model->index(row, column, root);
        const QAbstractItemDelegate *delegate = q->itemDelegateForIndex(sample);
        cachedItemSize = delegate ? delegate->sizeHint(option, sample) : QSize();
    }
    return cachedItemSize;
}

QItemSelection QListViewPrivate::selection(const QRect &rect) const
{
    QItemSelection selection;
    QModelIndex tl, br;
    const QList<QModelIndex> intersectVector = intersectingSet(rect);
    QList<QModelIndex>::const_iterator it = intersectVector.begin();
    for (; it != intersectVector.end(); ++it) {
        if (!tl.isValid() && !br.isValid()) {
            tl = br = *it;
        } else if ((*it).row() == (tl.row() - 1)) {
            tl = *it; // expand current range
        } else if ((*it).row() == (br.row() + 1)) {
            br = (*it); // expand current range
        } else {
            selection.select(tl, br); // select current range
            tl = br = *it; // start new range
        }
    }

    if (tl.isValid() && br.isValid())
        selection.select(tl, br);
    else if (tl.isValid())
        selection.select(tl, tl);
    else if (br.isValid())
        selection.select(br, br);

    return selection;
}

#if QT_CONFIG(draganddrop)
QAbstractItemView::DropIndicatorPosition QListViewPrivate::position(const QPoint &pos, const QRect &rect, const QModelIndex &idx) const
{
    if (viewMode == QListView::ListMode && flow == QListView::LeftToRight)
        return static_cast<QListModeViewBase *>(commonListView)->position(pos, rect, idx);
    else
        return QAbstractItemViewPrivate::position(pos, rect, idx);
}

bool QListViewPrivate::dropOn(QDropEvent *event, int *dropRow, int *dropCol, QModelIndex *dropIndex)
{
    if (viewMode == QListView::ListMode && flow == QListView::LeftToRight)
        return static_cast<QListModeViewBase *>(commonListView)->dropOn(event, dropRow, dropCol, dropIndex);
    else
        return QAbstractItemViewPrivate::dropOn(event, dropRow, dropCol, dropIndex);
}
#endif

void QListViewPrivate::removeCurrentAndDisabled(QList<QModelIndex> *indexes,
                                                const QModelIndex &current) const
{
    auto isCurrentOrDisabled = [this, current](const QModelIndex &index) {
        return !isIndexEnabled(index) || index == current;
    };
    indexes->removeIf(isCurrentOrDisabled);
}

/*
 * Common ListView Implementation
*/

void QCommonListViewBase::appendHiddenRow(int row)
{
    dd->hiddenRows.insert(dd->model->index(row, 0, qq->rootIndex()));
}

void QCommonListViewBase::removeHiddenRow(int row)
{
    dd->hiddenRows.remove(dd->model->index(row, 0, qq->rootIndex()));
}

#if QT_CONFIG(draganddrop)
void QCommonListViewBase::paintDragDrop(QPainter *painter)
{
    // FIXME: Until the we can provide a proper drop indicator
    // in IconMode, it makes no sense to show it
    dd->paintDropIndicator(painter);
}
#endif

QSize QListModeViewBase::viewportSize(const QAbstractItemView *v)
{
    return v->contentsRect().marginsRemoved(v->viewportMargins()).size();
}

void QCommonListViewBase::updateHorizontalScrollBar(const QSize &step)
{
    horizontalScrollBar()->d_func()->itemviewChangeSingleStep(step.width() + spacing());
    horizontalScrollBar()->setPageStep(viewport()->width());

    // If both scroll bars are set to auto, we might end up in a situation with enough space
    // for the actual content. But still one of the scroll bars will become enabled due to
    // the other one using the space. The other one will become invisible in the same cycle.
    // -> Infinite loop, QTBUG-39902
    const bool bothScrollBarsAuto = qq->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded &&
                                    qq->horizontalScrollBarPolicy() == Qt::ScrollBarAsNeeded;

    const QSize viewportSize = QListModeViewBase::viewportSize(qq);

    bool verticalWantsToShow = contentsSize.height() > viewportSize.height();
    bool horizontalWantsToShow;
    if (verticalWantsToShow)
        horizontalWantsToShow = contentsSize.width() > viewportSize.width() - qq->verticalScrollBar()->width();
    else
        horizontalWantsToShow = contentsSize.width() > viewportSize.width();

    if (bothScrollBarsAuto && !horizontalWantsToShow) {
        // break the infinite loop described above by setting the range to 0, 0.
        // QAbstractScrollArea will then hide the scroll bar for us
        horizontalScrollBar()->setRange(0, 0);
    } else {
        horizontalScrollBar()->setRange(0, contentsSize.width() - viewport()->width());
    }
}

void QCommonListViewBase::updateVerticalScrollBar(const QSize &step)
{
    verticalScrollBar()->d_func()->itemviewChangeSingleStep(step.height() + spacing());
    verticalScrollBar()->setPageStep(viewport()->height());

    // If both scroll bars are set to auto, we might end up in a situation with enough space
    // for the actual content. But still one of the scroll bars will become enabled due to
    // the other one using the space. The other one will become invisible in the same cycle.
    // -> Infinite loop, QTBUG-39902
    const bool bothScrollBarsAuto = qq->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded &&
                                    qq->horizontalScrollBarPolicy() == Qt::ScrollBarAsNeeded;

    const QSize viewportSize = QListModeViewBase::viewportSize(qq);

    bool horizontalWantsToShow = contentsSize.width() > viewportSize.width();
    bool verticalWantsToShow;
    if (horizontalWantsToShow)
        verticalWantsToShow = contentsSize.height() > viewportSize.height() - qq->horizontalScrollBar()->height();
    else
        verticalWantsToShow = contentsSize.height() > viewportSize.height();

    if (bothScrollBarsAuto && !verticalWantsToShow) {
        // break the infinite loop described above by setting the range to 0, 0.
        // QAbstractScrollArea will then hide the scroll bar for us
        verticalScrollBar()->setRange(0, 0);
    } else {
        verticalScrollBar()->setRange(0, contentsSize.height() - viewport()->height());
    }
}

void QCommonListViewBase::scrollContentsBy(int dx, int dy, bool /*scrollElasticBand*/)
{
    dd->scrollContentsBy(isRightToLeft() ? -dx : dx, dy);
}

int QCommonListViewBase::verticalScrollToValue(int /*index*/, QListView::ScrollHint hint,
                                          bool above, bool below, const QRect &area, const QRect &rect) const
{
    int verticalValue = verticalScrollBar()->value();
    QRect adjusted = rect.adjusted(-spacing(), -spacing(), spacing(), spacing());
    if (hint == QListView::PositionAtTop || above)
        verticalValue += adjusted.top();
    else if (hint == QListView::PositionAtBottom || below)
        verticalValue += qMin(adjusted.top(), adjusted.bottom() - area.height() + 1);
    else if (hint == QListView::PositionAtCenter)
        verticalValue += adjusted.top() - ((area.height() - adjusted.height()) / 2);
    return verticalValue;
}

int QCommonListViewBase::horizontalOffset() const
{
    return (isRightToLeft() ? horizontalScrollBar()->maximum() - horizontalScrollBar()->value() : horizontalScrollBar()->value());
}

int QCommonListViewBase::horizontalScrollToValue(const int /*index*/, QListView::ScrollHint hint,
                                            bool leftOf, bool rightOf, const QRect &area, const QRect &rect) const
{
    int horizontalValue = horizontalScrollBar()->value();
    if (isRightToLeft()) {
        if (hint == QListView::PositionAtCenter) {
            horizontalValue += ((area.width() - rect.width()) / 2) - rect.left();
        } else {
            if (leftOf)
                horizontalValue -= rect.left();
            else if (rightOf)
                horizontalValue += qMin(rect.left(), area.width() - rect.right());
        }
    } else {
        if (hint == QListView::PositionAtCenter) {
            horizontalValue += rect.left() - ((area.width()- rect.width()) / 2);
        } else {
            if (leftOf)
                horizontalValue += rect.left();
            else if (rightOf)
                horizontalValue += qMin(rect.left(), rect.right() - area.width());
        }
    }
    return horizontalValue;
}

/*
 * ListMode ListView Implementation
*/
QListModeViewBase::QListModeViewBase(QListView *q, QListViewPrivate *d)
    : QCommonListViewBase(q, d)
{
#if QT_CONFIG(draganddrop)
    dd->defaultDropAction = Qt::CopyAction;
#endif
}

#if QT_CONFIG(draganddrop)
QAbstractItemView::DropIndicatorPosition QListModeViewBase::position(const QPoint &pos, const QRect &rect, const QModelIndex &index) const
{
    QAbstractItemView::DropIndicatorPosition r = QAbstractItemView::OnViewport;
    if (!dd->overwrite) {
        const int margin = 2;
        if (pos.x() - rect.left() < margin) {
            r = QAbstractItemView::AboveItem;   // Visually, on the left
        } else if (rect.right() - pos.x() < margin) {
            r = QAbstractItemView::BelowItem;   // Visually, on the right
        } else if (rect.contains(pos, true)) {
            r = QAbstractItemView::OnItem;
        }
    } else {
        QRect touchingRect = rect;
        touchingRect.adjust(-1, -1, 1, 1);
        if (touchingRect.contains(pos, false)) {
            r = QAbstractItemView::OnItem;
        }
    }

    if (r == QAbstractItemView::OnItem && (!(dd->model->flags(index) & Qt::ItemIsDropEnabled)))
        r = pos.x() < rect.center().x() ? QAbstractItemView::AboveItem : QAbstractItemView::BelowItem;

    return r;
}

void QListModeViewBase::dragMoveEvent(QDragMoveEvent *event)
{
    if (qq->dragDropMode() == QAbstractItemView::InternalMove
        && (event->source() != qq || !(event->possibleActions() & Qt::MoveAction)))
        return;

    // ignore by default
    event->ignore();

    // can't use indexAt, doesn't account for spacing.
    QPoint p = event->position().toPoint();
    QRect rect(p.x() + horizontalOffset(), p.y() + verticalOffset(), 1, 1);
    rect.adjust(-dd->spacing(), -dd->spacing(), dd->spacing(), dd->spacing());
    const QList<QModelIndex> intersectVector = dd->intersectingSet(rect);
    QModelIndex index = intersectVector.size() > 0
                        ? intersectVector.last() : QModelIndex();
    dd->hover = index;
    if (!dd->droppingOnItself(event, index)
        && dd->canDrop(event)) {

        if (index.isValid() && dd->showDropIndicator) {
            QRect rect = qq->visualRect(index);
            dd->dropIndicatorPosition = position(event->position().toPoint(), rect, index);
            // if spacing, should try to draw between items, not just next to item.
            switch (dd->dropIndicatorPosition) {
            case QAbstractItemView::AboveItem:
                if (dd->isIndexDropEnabled(index.parent())) {
                    dd->dropIndicatorRect = QRect(rect.left()-dd->spacing(), rect.top(), 0, rect.height());
                    event->accept();
                } else {
                    dd->dropIndicatorRect = QRect();
                }
                break;
            case QAbstractItemView::BelowItem:
                if (dd->isIndexDropEnabled(index.parent())) {
                    dd->dropIndicatorRect = QRect(rect.right()+dd->spacing(), rect.top(), 0, rect.height());
                    event->accept();
                } else {
                    dd->dropIndicatorRect = QRect();
                }
                break;
            case QAbstractItemView::OnItem:
                if (dd->isIndexDropEnabled(index)) {
                    dd->dropIndicatorRect = rect;
                    event->accept();
                } else {
                    dd->dropIndicatorRect = QRect();
                }
                break;
            case QAbstractItemView::OnViewport:
                dd->dropIndicatorRect = QRect();
                if (dd->isIndexDropEnabled(qq->rootIndex())) {
                    event->accept(); // allow dropping in empty areas
                }
                break;
            }
        } else {
            dd->dropIndicatorRect = QRect();
            dd->dropIndicatorPosition = QAbstractItemView::OnViewport;
            if (dd->isIndexDropEnabled(qq->rootIndex())) {
                event->accept(); // allow dropping in empty areas
            }
        }
        dd->viewport->update();
    } // can drop

    if (dd->shouldAutoScroll(event->position().toPoint()))
        qq->startAutoScroll();
}

/*!
    If the event hasn't already been accepted, determines the index to drop on.

    if (row == -1 && col == -1)
        // append to this drop index
    else
        // place at row, col in drop index

    If it returns \c true a drop can be done, and dropRow, dropCol and dropIndex reflects the position of the drop.
    \internal
  */
bool QListModeViewBase::dropOn(QDropEvent *event, int *dropRow, int *dropCol, QModelIndex *dropIndex)
{
    if (event->isAccepted())
        return false;

    QModelIndex index;
    if (dd->viewport->rect().contains(event->position().toPoint())) {
        // can't use indexAt, doesn't account for spacing.
        QPoint p = event->position().toPoint();
        QRect rect(p.x() + horizontalOffset(), p.y() + verticalOffset(), 1, 1);
        rect.adjust(-dd->spacing(), -dd->spacing(), dd->spacing(), dd->spacing());
        const QList<QModelIndex> intersectVector = dd->intersectingSet(rect);
        index = intersectVector.size() > 0
            ? intersectVector.last() : QModelIndex();
        if (!index.isValid())
            index = dd->root;
    }

    // If we are allowed to do the drop
    if (dd->model->supportedDropActions() & event->dropAction()) {
        int row = -1;
        int col = -1;
        if (index != dd->root) {
            dd->dropIndicatorPosition = position(event->position().toPoint(), qq->visualRect(index), index);
            switch (dd->dropIndicatorPosition) {
            case QAbstractItemView::AboveItem:
                row = index.row();
                col = index.column();
                index = index.parent();
                break;
            case QAbstractItemView::BelowItem:
                row = index.row() + 1;
                col = index.column();
                index = index.parent();
                break;
            case QAbstractItemView::OnItem:
            case QAbstractItemView::OnViewport:
                break;
            }
        } else {
            dd->dropIndicatorPosition = QAbstractItemView::OnViewport;
        }
        *dropIndex = index;
        *dropRow = row;
        *dropCol = col;
        if (!dd->droppingOnItself(event, index))
            return true;
    }
    return false;
}

#endif //QT_CONFIG(draganddrop)

void QListModeViewBase::updateVerticalScrollBar(const QSize &step)
{
    if (verticalScrollMode() == QAbstractItemView::ScrollPerItem
        && ((flow() == QListView::TopToBottom && !isWrapping())
        || (flow() == QListView::LeftToRight && isWrapping()))) {
            const int steps = (flow() == QListView::TopToBottom ? scrollValueMap : segmentPositions).size() - 1;
            if (steps > 0) {
                const int pageSteps = perItemScrollingPageSteps(viewport()->height(), contentsSize.height(), isWrapping());
                verticalScrollBar()->setSingleStep(1);
                verticalScrollBar()->setPageStep(pageSteps);
                verticalScrollBar()->setRange(0, steps - pageSteps);
            } else {
                verticalScrollBar()->setRange(0, 0);
            }
            // } else if (vertical && d->isWrapping() && d->movement == Static) {
            // ### wrapped scrolling in flow direction
    } else {
        QCommonListViewBase::updateVerticalScrollBar(step);
    }
}

void QListModeViewBase::updateHorizontalScrollBar(const QSize &step)
{
    if (horizontalScrollMode() == QAbstractItemView::ScrollPerItem
        && ((flow() == QListView::TopToBottom && isWrapping())
        || (flow() == QListView::LeftToRight && !isWrapping()))) {
            int steps = (flow() == QListView::TopToBottom ? segmentPositions : scrollValueMap).size() - 1;
            if (steps > 0) {
                const int pageSteps = perItemScrollingPageSteps(viewport()->width(), contentsSize.width(), isWrapping());
                horizontalScrollBar()->setSingleStep(1);
                horizontalScrollBar()->setPageStep(pageSteps);
                horizontalScrollBar()->setRange(0, steps - pageSteps);
            } else {
                horizontalScrollBar()->setRange(0, 0);
            }
    } else {
        QCommonListViewBase::updateHorizontalScrollBar(step);
    }
}

int QListModeViewBase::verticalScrollToValue(int index, QListView::ScrollHint hint,
                                          bool above, bool below, const QRect &area, const QRect &rect) const
{
    if (verticalScrollMode() == QAbstractItemView::ScrollPerItem) {
        int value;
        if (scrollValueMap.isEmpty()) {
            value = 0;
        } else {
            int scrollBarValue = verticalScrollBar()->value();
            int numHidden = 0;
            for (const auto &idx : std::as_const(dd->hiddenRows))
                if (idx.row() <= scrollBarValue)
                    ++numHidden;
            value = qBound(0, scrollValueMap.at(verticalScrollBar()->value()) - numHidden, flowPositions.size() - 1);
        }
        if (above)
            hint = QListView::PositionAtTop;
        else if (below)
            hint = QListView::PositionAtBottom;
        if (hint == QListView::EnsureVisible)
            return value;

        return perItemScrollToValue(index, value, area.height(), hint, Qt::Vertical, isWrapping(), rect.height());
    }

    return QCommonListViewBase::verticalScrollToValue(index, hint, above, below, area, rect);
}

int QListModeViewBase::horizontalOffset() const
{
    if (horizontalScrollMode() == QAbstractItemView::ScrollPerItem) {
        if (isWrapping()) {
            if (flow() == QListView::TopToBottom && !segmentPositions.isEmpty()) {
                const int max = segmentPositions.size() - 1;
                int currentValue = qBound(0, horizontalScrollBar()->value(), max);
                int position = segmentPositions.at(currentValue);
                int maximumValue = qBound(0, horizontalScrollBar()->maximum(), max);
                int maximum = segmentPositions.at(maximumValue);
                return (isRightToLeft() ? maximum - position : position);
            }
        } else if (flow() == QListView::LeftToRight && !flowPositions.isEmpty()) {
            int position = flowPositions.at(scrollValueMap.at(horizontalScrollBar()->value()));
            int maximum = flowPositions.at(scrollValueMap.at(horizontalScrollBar()->maximum()));
            return (isRightToLeft() ? maximum - position : position);
        }
    }
    return QCommonListViewBase::horizontalOffset();
}

int QListModeViewBase::verticalOffset() const
{
    if (verticalScrollMode() == QAbstractItemView::ScrollPerItem) {
        if (isWrapping()) {
            if (flow() == QListView::LeftToRight && !segmentPositions.isEmpty()) {
                int value = verticalScrollBar()->value();
                if (value >= segmentPositions.size())
                    return 0;
                return segmentPositions.at(value) - spacing();
            }
        } else if (flow() == QListView::TopToBottom && !flowPositions.isEmpty()) {
            int value = verticalScrollBar()->value();
            if (value > scrollValueMap.size())
                return 0;
            return flowPositions.at(scrollValueMap.at(value)) - spacing();
        }
    }
    return QCommonListViewBase::verticalOffset();
}

int QListModeViewBase::horizontalScrollToValue(int index, QListView::ScrollHint hint,
                                            bool leftOf, bool rightOf, const QRect &area, const QRect &rect) const
{
    if (horizontalScrollMode() != QAbstractItemView::ScrollPerItem)
        return QCommonListViewBase::horizontalScrollToValue(index, hint, leftOf, rightOf, area, rect);

    int value;
    if (scrollValueMap.isEmpty())
        value = 0;
    else
        value = qBound(0, scrollValueMap.at(horizontalScrollBar()->value()), flowPositions.size() - 1);
    if (leftOf)
        hint = QListView::PositionAtTop;
    else if (rightOf)
        hint = QListView::PositionAtBottom;
    if (hint == QListView::EnsureVisible)
        return value;

    return perItemScrollToValue(index, value, area.width(), hint, Qt::Horizontal, isWrapping(), rect.width());
}

void QListModeViewBase::scrollContentsBy(int dx, int dy, bool scrollElasticBand)
{
    // ### reorder this logic
    const int verticalValue = verticalScrollBar()->value();
    const int horizontalValue = horizontalScrollBar()->value();
    const bool vertical = (verticalScrollMode() == QAbstractItemView::ScrollPerItem);
    const bool horizontal = (horizontalScrollMode() == QAbstractItemView::ScrollPerItem);

    if (isWrapping()) {
        if (segmentPositions.isEmpty())
            return;
        const int max = segmentPositions.size() - 1;
        if (horizontal && flow() == QListView::TopToBottom && dx != 0) {
            int currentValue = qBound(0, horizontalValue, max);
            int previousValue = qBound(0, currentValue + dx, max);
            int currentCoordinate = segmentPositions.at(currentValue) - spacing();
            int previousCoordinate = segmentPositions.at(previousValue) - spacing();
            dx = previousCoordinate - currentCoordinate;
        } else if (vertical && flow() == QListView::LeftToRight && dy != 0) {
            int currentValue = qBound(0, verticalValue, max);
            int previousValue = qBound(0, currentValue + dy, max);
            int currentCoordinate = segmentPositions.at(currentValue) - spacing();
            int previousCoordinate = segmentPositions.at(previousValue) - spacing();
            dy = previousCoordinate - currentCoordinate;
        }
    } else {
        if (flowPositions.isEmpty())
            return;
        const int max = scrollValueMap.size() - 1;
        if (vertical && flow() == QListView::TopToBottom && dy != 0) {
            int currentValue = qBound(0, verticalValue, max);
            int previousValue = qBound(0, currentValue + dy, max);
            int currentCoordinate = flowPositions.at(scrollValueMap.at(currentValue));
            int previousCoordinate = flowPositions.at(scrollValueMap.at(previousValue));
            dy = previousCoordinate - currentCoordinate;
        } else if (horizontal && flow() == QListView::LeftToRight && dx != 0) {
            int currentValue = qBound(0, horizontalValue, max);
            int previousValue = qBound(0, currentValue + dx, max);
            int currentCoordinate = flowPositions.at(scrollValueMap.at(currentValue));
            int previousCoordinate = flowPositions.at(scrollValueMap.at(previousValue));
            dx = previousCoordinate - currentCoordinate;
        }
    }
    QCommonListViewBase::scrollContentsBy(dx, dy, scrollElasticBand);
}

bool QListModeViewBase::doBatchedItemLayout(const QListViewLayoutInfo &info, int max)
{
    doStaticLayout(info);
    return batchStartRow > max; // returning true stops items layout
}

QListViewItem QListModeViewBase::indexToListViewItem(const QModelIndex &index) const
{
    if (flowPositions.isEmpty()
        || segmentPositions.isEmpty()
        || index.row() >= flowPositions.size() - 1)
        return QListViewItem();

    const int segment = qBinarySearch<int>(segmentStartRows, index.row(),
                                           0, segmentStartRows.size() - 1);


    QStyleOptionViewItem options;
    initViewItemOption(&options);
    options.rect.setSize(contentsSize);
    QSize size = (uniformItemSizes() && cachedItemSize().isValid())
                 ? cachedItemSize() : itemSize(options, index);
    QSize cellSize = size;

    QPoint pos;
    if (flow() == QListView::LeftToRight) {
        pos.setX(flowPositions.at(index.row()));
        pos.setY(segmentPositions.at(segment));
    } else { // TopToBottom
        pos.setY(flowPositions.at(index.row()));
        pos.setX(segmentPositions.at(segment));
        if (isWrapping()) { // make the items as wide as the segment
            int right = (segment + 1 >= segmentPositions.size()
                     ? contentsSize.width()
                     : segmentPositions.at(segment + 1));
            cellSize.setWidth(right - pos.x());
        } else { // make the items as wide as the viewport
            cellSize.setWidth(qMax(size.width(), viewport()->width() - 2 * spacing()));
        }
    }

    if (dd->itemAlignment & Qt::AlignHorizontal_Mask) {
        size.setWidth(qMin(size.width(), cellSize.width()));
        if (dd->itemAlignment & Qt::AlignRight)
            pos.setX(pos.x() + cellSize.width() - size.width());
        if (dd->itemAlignment & Qt::AlignHCenter)
            pos.setX(pos.x() + (cellSize.width() - size.width()) / 2);
    } else {
        size.setWidth(cellSize.width());
    }

    return QListViewItem(QRect(pos, size), index.row());
}

QPoint QListModeViewBase::initStaticLayout(const QListViewLayoutInfo &info)
{
    int x, y;
    if (info.first == 0) {
        flowPositions.clear();
        segmentPositions.clear();
        segmentStartRows.clear();
        segmentExtents.clear();
        scrollValueMap.clear();
        x = info.bounds.left() + info.spacing;
        y = info.bounds.top() + info.spacing;
        segmentPositions.append(info.flow == QListView::LeftToRight ? y : x);
        segmentStartRows.append(0);
    } else if (info.wrap) {
        if (info.flow == QListView::LeftToRight) {
            x = batchSavedPosition;
            y = segmentPositions.constLast();
        } else { // flow == QListView::TopToBottom
            x = segmentPositions.constLast();
            y = batchSavedPosition;
        }
    } else { // not first and not wrap
        if (info.flow == QListView::LeftToRight) {
            x = batchSavedPosition;
            y = info.bounds.top() + info.spacing;
        } else { // flow == QListView::TopToBottom
            x = info.bounds.left() + info.spacing;
            y = batchSavedPosition;
        }
    }
    return QPoint(x, y);
}

/*!
  \internal
*/
void QListModeViewBase::doStaticLayout(const QListViewLayoutInfo &info)
{
    const bool useItemSize = !info.grid.isValid();
    const QPoint topLeft = initStaticLayout(info);
    QStyleOptionViewItem option;
    initViewItemOption(&option);
    option.rect = info.bounds;
    option.rect.adjust(info.spacing, info.spacing, -info.spacing, -info.spacing);

    // The static layout data structures are as follows:
    // One vector contains the coordinate in the direction of layout flow.
    // Another vector contains the coordinates of the segments.
    // A third vector contains the index (model row) of the first item
    // of each segment.

    int segStartPosition;
    int segEndPosition;
    int deltaFlowPosition;
    int deltaSegPosition;
    int deltaSegHint;
    int flowPosition;
    int segPosition;

    if (info.flow == QListView::LeftToRight) {
        segStartPosition = info.bounds.left();
        segEndPosition = info.bounds.width();
        flowPosition = topLeft.x();
        segPosition = topLeft.y();
        deltaFlowPosition = info.grid.width(); // dx
        deltaSegPosition = useItemSize ? batchSavedDeltaSeg : info.grid.height(); // dy
        deltaSegHint = info.grid.height();
    } else { // flow == QListView::TopToBottom
        segStartPosition = info.bounds.top();
        segEndPosition = info.bounds.height();
        flowPosition = topLeft.y();
        segPosition = topLeft.x();
        deltaFlowPosition = info.grid.height(); // dy
        deltaSegPosition = useItemSize ? batchSavedDeltaSeg : info.grid.width(); // dx
        deltaSegHint = info.grid.width();
    }

    for (int row = info.first; row <= info.last; ++row) {
        if (isHidden(row)) { // ###
            flowPositions.append(flowPosition);
        } else {
            // if we are not using a grid, we need to find the deltas
            if (useItemSize) {
                QSize hint = itemSize(option, modelIndex(row));
                if (info.flow == QListView::LeftToRight) {
                    deltaFlowPosition = hint.width() + info.spacing;
                    deltaSegHint = hint.height() + info.spacing;
                } else { // TopToBottom
                    deltaFlowPosition = hint.height() + info.spacing;
                    deltaSegHint = hint.width() + info.spacing;
                }
            }
            // create new segment
            if (info.wrap && (flowPosition + deltaFlowPosition >= segEndPosition)) {
                segmentExtents.append(flowPosition);
                flowPosition = info.spacing + segStartPosition;
                segPosition += info.spacing + deltaSegPosition;
                segmentPositions.append(segPosition);
                segmentStartRows.append(row);
                deltaSegPosition = 0;
            }
            // save the flow position of this item
            scrollValueMap.append(flowPositions.size());
            flowPositions.append(flowPosition);
            // prepare for the next item
            deltaSegPosition = qMax(deltaSegHint, deltaSegPosition);
            flowPosition += info.spacing + deltaFlowPosition;
        }
    }
    // used when laying out next batch
    batchSavedPosition = flowPosition;
    batchSavedDeltaSeg = deltaSegPosition;
    batchStartRow = info.last + 1;
    if (info.last == info.max)
        flowPosition -= info.spacing; // remove extra spacing
    // set the contents size
    QRect rect = info.bounds;
    if (info.flow == QListView::LeftToRight) {
        rect.setRight(segmentPositions.size() == 1 ? flowPosition : info.bounds.right());
        rect.setBottom(segPosition + deltaSegPosition);
    } else { // TopToBottom
        rect.setRight(segPosition + deltaSegPosition);
        rect.setBottom(segmentPositions.size() == 1 ? flowPosition : info.bounds.bottom());
    }
    contentsSize = QSize(rect.right(), rect.bottom());
    // if it is the last batch, save the end of the segments
    if (info.last == info.max) {
        segmentExtents.append(flowPosition);
        scrollValueMap.append(flowPositions.size());
        flowPositions.append(flowPosition);
        segmentPositions.append(info.wrap ? segPosition + deltaSegPosition : INT_MAX);
    }
    // if the new items are visible, update the viewport
    QRect changedRect(topLeft, rect.bottomRight());
    if (clipRect().intersects(changedRect))
        viewport()->update();
}

/*!
  \internal
  Finds the set of items intersecting with \a area.
  In this function, itemsize is counted from topleft to the start of the next item.
*/
QList<QModelIndex> QListModeViewBase::intersectingSet(const QRect &area) const
{
    QList<QModelIndex> ret;
    int segStartPosition;
    int segEndPosition;
    int flowStartPosition;
    int flowEndPosition;
    if (flow() == QListView::LeftToRight) {
        segStartPosition = area.top();
        segEndPosition = area.bottom();
        flowStartPosition = area.left();
        flowEndPosition = area.right();
    } else {
        segStartPosition = area.left();
        segEndPosition = area.right();
        flowStartPosition = area.top();
        flowEndPosition = area.bottom();
    }
    if (segmentPositions.size() < 2 || flowPositions.isEmpty())
        return ret;
    // the last segment position is actually the edge of the last segment
    const int segLast = segmentPositions.size() - 2;
    int seg = qBinarySearch<int>(segmentPositions, segStartPosition, 0, segLast + 1);
    for (; seg <= segLast && segmentPositions.at(seg) <= segEndPosition; ++seg) {
        int first = segmentStartRows.at(seg);
        int last = (seg < segLast ? segmentStartRows.at(seg + 1) : batchStartRow) - 1;
        if (segmentExtents.at(seg) < flowStartPosition)
            continue;
        int row = qBinarySearch<int>(flowPositions, flowStartPosition, first, last);
        for (; row <= last && flowPositions.at(row) <= flowEndPosition; ++row) {
            if (isHidden(row))
                continue;
            QModelIndex index = modelIndex(row);
            if (index.isValid()) {
                if (flow() == QListView::LeftToRight || dd->itemAlignment == Qt::Alignment()) {
                    ret += index;
                } else {
                    const auto viewItem = indexToListViewItem(index);
                    const int iw = viewItem.width();
                    const int startPos = qMax(segStartPosition, segmentPositions.at(seg));
                    const int endPos = qMin(segmentPositions.at(seg + 1), segEndPosition);
                    if (endPos >= viewItem.x && startPos < viewItem.x + iw)
                        ret += index;
                }
            }
#if 0 // for debugging
            else
                qWarning("intersectingSet: row %d was invalid", row);
#endif
        }
    }
    return ret;
}

void QListModeViewBase::dataChanged(const QModelIndex &, const QModelIndex &)
{
    dd->doDelayedItemsLayout();
}


QRect QListModeViewBase::mapToViewport(const QRect &rect) const
{
    if (isWrapping())
        return rect;
    // If the listview is in "listbox-mode", the items are as wide as the view.
    // But we don't shrink the items.
    QRect result = rect;
    if (flow() == QListView::TopToBottom) {
        result.setLeft(spacing());
        result.setWidth(qMax(rect.width(), qMax(contentsSize.width(), viewport()->width()) - 2 * spacing()));
    } else { // LeftToRight
        result.setTop(spacing());
        result.setHeight(qMax(rect.height(), qMax(contentsSize.height(), viewport()->height()) - 2 * spacing()));
    }
    return result;
}

int QListModeViewBase::perItemScrollingPageSteps(int length, int bounds, bool wrap) const
{
    QList<int> positions;
    if (wrap)
        positions = segmentPositions;
    else if (!flowPositions.isEmpty()) {
        positions.reserve(scrollValueMap.size());
        for (int itemShown : scrollValueMap)
            positions.append(flowPositions.at(itemShown));
    }
    if (positions.isEmpty() || bounds <= length)
        return positions.size();
    if (uniformItemSizes()) {
        for (int i = 1; i < positions.size(); ++i)
            if (positions.at(i) > 0)
                return length / positions.at(i);
        return 0; // all items had height 0
    }
    int pageSteps = 0;
    int steps = positions.size() - 1;
    int max = qMax(length, bounds);
    int min = qMin(length, bounds);
    int pos = min - (max - positions.constLast());

    while (pos >= 0 && steps > 0) {
        pos -= (positions.at(steps) - positions.at(steps - 1));
        if (pos >= 0) //this item should be visible
            ++pageSteps;
        --steps;
    }

    // at this point we know that positions has at least one entry
    return qMax(pageSteps, 1);
}

int QListModeViewBase::perItemScrollToValue(int index, int scrollValue, int viewportSize,
                                                 QAbstractItemView::ScrollHint hint,
                                                 Qt::Orientation orientation, bool wrap, int itemExtent) const
{
    if (index < 0)
        return scrollValue;

    itemExtent += spacing();
    QList<int> hiddenRows = dd->hiddenRowIds();
    std::sort(hiddenRows.begin(), hiddenRows.end());
    int hiddenRowsBefore = 0;
    for (int i = 0; i < hiddenRows.size() - 1; ++i)
        if (hiddenRows.at(i) > index + hiddenRowsBefore)
            break;
        else
            ++hiddenRowsBefore;
    if (!wrap) {
        int topIndex = index;
        const int bottomIndex = topIndex;
        const int bottomCoordinate = flowPositions.at(index + hiddenRowsBefore);
        while (topIndex > 0 &&
               (bottomCoordinate - flowPositions.at(topIndex + hiddenRowsBefore - 1) + itemExtent) <= (viewportSize)) {
            topIndex--;
            // will the next one be a hidden row -> skip
            while (hiddenRowsBefore > 0 && hiddenRows.at(hiddenRowsBefore - 1) >= topIndex + hiddenRowsBefore - 1)
                hiddenRowsBefore--;
        }

        const int itemCount = bottomIndex - topIndex + 1;
        switch (hint) {
        case QAbstractItemView::PositionAtTop:
            return index;
        case QAbstractItemView::PositionAtBottom:
            return index - itemCount + 1;
        case QAbstractItemView::PositionAtCenter:
            return index - (itemCount / 2);
        default:
            break;
        }
    } else { // wrapping
        Qt::Orientation flowOrientation = (flow() == QListView::LeftToRight
                                           ? Qt::Horizontal : Qt::Vertical);
        if (flowOrientation == orientation) { // scrolling in the "flow" direction
            // ### wrapped scrolling in the flow direction
            return flowPositions.at(index + hiddenRowsBefore); // ### always pixel based for now
        } else if (!segmentStartRows.isEmpty()) { // we are scrolling in the "segment" direction
            int segment = qBinarySearch<int>(segmentStartRows, index, 0, segmentStartRows.size() - 1);
            int leftSegment = segment;
            const int rightSegment = leftSegment;
            const int bottomCoordinate = segmentPositions.at(segment);

            while (leftSegment > scrollValue &&
                (bottomCoordinate - segmentPositions.at(leftSegment-1) + itemExtent) <= (viewportSize)) {
                    leftSegment--;
            }

            const int segmentCount = rightSegment - leftSegment + 1;
            switch (hint) {
            case QAbstractItemView::PositionAtTop:
                return segment;
            case QAbstractItemView::PositionAtBottom:
                return segment - segmentCount + 1;
            case QAbstractItemView::PositionAtCenter:
                return segment - (segmentCount / 2);
            default:
                break;
            }
        }
    }
    return scrollValue;
}

void QListModeViewBase::clear()
{
    flowPositions.clear();
    segmentPositions.clear();
    segmentStartRows.clear();
    segmentExtents.clear();
    batchSavedPosition = 0;
    batchStartRow = 0;
    batchSavedDeltaSeg = 0;
}

/*
 * IconMode ListView Implementation
*/

void QIconModeViewBase::setPositionForIndex(const QPoint &position, const QModelIndex &index)
{
    if (index.row() >= items.size())
        return;
    const QSize oldContents = contentsSize;
    qq->update(index); // update old position
    moveItem(index.row(), position);
    qq->update(index); // update new position

    if (contentsSize != oldContents)
        dd->viewUpdateGeometries(); // update the scroll bars
}

void QIconModeViewBase::appendHiddenRow(int row)
{
    if (row >= 0 && row < items.size()) //remove item
        tree.removeLeaf(items.at(row).rect(), row);
    QCommonListViewBase::appendHiddenRow(row);
}

void QIconModeViewBase::removeHiddenRow(int row)
{
    QCommonListViewBase::removeHiddenRow(row);
    if (row >= 0 && row < items.size()) //insert item
        tree.insertLeaf(items.at(row).rect(), row);
}

#if QT_CONFIG(draganddrop)
bool QIconModeViewBase::filterStartDrag(Qt::DropActions supportedActions)
{
    // This function does the same thing as in QAbstractItemView::startDrag(),
    // plus adding viewitems to the draggedItems list.
    // We need these items to draw the drag items
    QModelIndexList indexes = dd->selectionModel->selectedIndexes();
    if (indexes.size() > 0 ) {
        if (viewport()->acceptDrops()) {
            QModelIndexList::ConstIterator it = indexes.constBegin();
            for (; it != indexes.constEnd(); ++it)
                if (dd->model->flags(*it) & Qt::ItemIsDragEnabled
                    && (*it).column() == dd->column)
                    draggedItems.push_back(*it);
        }

        QRect rect;
        QPixmap pixmap = dd->renderToPixmap(indexes, &rect);
        rect.adjust(horizontalOffset(), verticalOffset(), 0, 0);
        QDrag *drag = new QDrag(qq);
        drag->setMimeData(dd->model->mimeData(indexes));
        drag->setPixmap(pixmap);
        drag->setHotSpot(dd->pressedPosition - rect.topLeft());
        dd->dropEventMoved = false;
        Qt::DropAction action = drag->exec(supportedActions, dd->defaultDropAction);
        draggedItems.clear();
        // delete item, unless it has already been moved internally (see filterDropEvent)
        if (action == Qt::MoveAction && !dd->dropEventMoved) {
            if (dd->dragDropMode != QAbstractItemView::InternalMove || drag->target() == qq->viewport())
                dd->clearOrRemove();
        }
        dd->dropEventMoved = false;
    }
    return true;
}

bool QIconModeViewBase::filterDropEvent(QDropEvent *e)
{
    if (e->source() != qq)
        return false;

    const QSize contents = contentsSize;
    QPoint offset(horizontalOffset(), verticalOffset());
    QPoint end = e->position().toPoint() + offset;
    if (qq->acceptDrops()) {
        const Qt::ItemFlags dropableFlags = Qt::ItemIsDropEnabled|Qt::ItemIsEnabled;
        const QList<QModelIndex> &dropIndices = intersectingSet(QRect(end, QSize(1, 1)));
        for (const QModelIndex &index : dropIndices)
            if ((index.flags() & dropableFlags) == dropableFlags)
                return false;
    }
    QPoint start = dd->pressedPosition;
    QPoint delta = (dd->movement == QListView::Snap ? snapToGrid(end) - snapToGrid(start) : end - start);
    const QList<QModelIndex> indexes = dd->selectionModel->selectedIndexes();
    for (const auto &index : indexes) {
        QRect rect = dd->rectForIndex(index);
        viewport()->update(dd->mapToViewport(rect, false));
        QPoint dest = rect.topLeft() + delta;
        if (qq->isRightToLeft())
            dest.setX(dd->flipX(dest.x()) - rect.width());
        moveItem(index.row(), dest);
        qq->update(index);
    }
    dd->stopAutoScroll();
    draggedItems.clear();
    dd->emitIndexesMoved(indexes);
    // do not delete item on internal move, see filterStartDrag()
    dd->dropEventMoved = true;
    e->accept(); // we have handled the event
    // if the size has not grown, we need to check if it has shrunk
    if (contentsSize != contents) {
        if ((contentsSize.width() <= contents.width()
            || contentsSize.height() <= contents.height())) {
                updateContentsSize();
        }
        dd->viewUpdateGeometries();
    }
    return true;
}

bool QIconModeViewBase::filterDragLeaveEvent(QDragLeaveEvent *e)
{
    viewport()->update(draggedItemsRect()); // erase the area
    draggedItemsPos = QPoint(-1, -1); // don't draw the dragged items
    return QCommonListViewBase::filterDragLeaveEvent(e);
}

bool QIconModeViewBase::filterDragMoveEvent(QDragMoveEvent *e)
{
    const bool wasAccepted = e->isAccepted();

    // ignore by default
    e->ignore();

    if (e->source() != qq || !dd->canDrop(e)) {
        // restore previous acceptance on failure
        e->setAccepted(wasAccepted);
        return false;
    }

    // get old dragged items rect
    QRect itemsRect = this->itemsRect(draggedItems);
    viewport()->update(itemsRect.translated(draggedItemsDelta()));
    // update position
    draggedItemsPos = e->position().toPoint();
    // get new items rect
    viewport()->update(itemsRect.translated(draggedItemsDelta()));
    // set the item under the cursor to current
    QModelIndex index;
    if (movement() == QListView::Snap) {
        QRect rect(snapToGrid(e->position().toPoint() + offset()), gridSize());
        const QList<QModelIndex> intersectVector = intersectingSet(rect);
        index = intersectVector.size() > 0 ? intersectVector.last() : QModelIndex();
    } else {
        index = qq->indexAt(e->position().toPoint());
    }
    // check if we allow drops here
    if (draggedItems.contains(index))
        e->accept(); // allow changing item position
    else if (dd->model->flags(index) & Qt::ItemIsDropEnabled)
        e->accept(); // allow dropping on dropenabled items
    else if (!index.isValid())
        e->accept(); // allow dropping in empty areas

    // the event was treated. do autoscrolling
    if (dd->shouldAutoScroll(e->position().toPoint()))
        dd->startAutoScroll();
    return true;
}
#endif // QT_CONFIG(draganddrop)

void QIconModeViewBase::setRowCount(int rowCount)
{
    tree.create(qMax(rowCount - hiddenCount(), 0));
}

void QIconModeViewBase::scrollContentsBy(int dx, int dy, bool scrollElasticBand)
{
    if (scrollElasticBand)
        dd->scrollElasticBandBy(isRightToLeft() ? -dx : dx, dy);

    QCommonListViewBase::scrollContentsBy(dx, dy, scrollElasticBand);
    if (!draggedItems.isEmpty())
        viewport()->update(draggedItemsRect().translated(dx, dy));
}

void QIconModeViewBase::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (column() >= topLeft.column() && column() <= bottomRight.column())  {
        QStyleOptionViewItem option;
        initViewItemOption(&option);
        const int bottom = qMin(items.size(), bottomRight.row() + 1);
        const bool useItemSize = !dd->grid.isValid();
        for (int row = topLeft.row(); row < bottom; ++row)
        {
            QSize s = itemSize(option, modelIndex(row));
            if (!useItemSize)
            {
                s.setWidth(qMin(dd->grid.width(), s.width()));
                s.setHeight(qMin(dd->grid.height(), s.height()));
            }
            items[row].resize(s);
        }
    }
}

bool QIconModeViewBase::doBatchedItemLayout(const QListViewLayoutInfo &info, int max)
{
    if (info.last >= items.size()) {
        //first we create the items
        QStyleOptionViewItem option;
        initViewItemOption(&option);
        for (int row = items.size(); row <= info.last; ++row) {
            QSize size = itemSize(option, modelIndex(row));
            QListViewItem item(QRect(0, 0, size.width(), size.height()), row); // default pos
            items.append(item);
        }
        doDynamicLayout(info);
    }
    return (batchStartRow > max); // done
}

QListViewItem QIconModeViewBase::indexToListViewItem(const QModelIndex &index) const
{
    if (index.isValid() && index.row() < items.size())
        return items.at(index.row());
    return QListViewItem();
}

void QIconModeViewBase::initBspTree(const QSize &contents)
{
    // remove all items from the tree
    int leafCount = tree.leafCount();
    for (int l = 0; l < leafCount; ++l)
        tree.leaf(l).clear();
    // we have to get the bounding rect of the items before we can initialize the tree
    QBspTree::Node::Type type = QBspTree::Node::Both; // 2D
    // simple heuristics to get better bsp
    if (contents.height() / contents.width() >= 3)
        type = QBspTree::Node::HorizontalPlane;
    else if (contents.width() / contents.height() >= 3)
        type = QBspTree::Node::VerticalPlane;
    // build tree for the bounding rect (not just the contents rect)
    tree.init(QRect(0, 0, contents.width(), contents.height()), type);
}

QPoint QIconModeViewBase::initDynamicLayout(const QListViewLayoutInfo &info)
{
    int x, y;
    if (info.first == 0) {
        x = info.bounds.x() + info.spacing;
        y = info.bounds.y() + info.spacing;
        items.reserve(rowCount() - hiddenCount());
    } else {
        int idx = info.first - 1;
        while (idx > 0 && !items.at(idx).isValid())
            --idx;
        const QListViewItem &item = items.at(idx);
        x = item.x;
        y = item.y;
        if (info.flow == QListView::LeftToRight)
            x += (info.grid.isValid() ? info.grid.width() : item.w) + info.spacing;
        else
            y += (info.grid.isValid() ? info.grid.height() : item.h) + info.spacing;
    }
    return QPoint(x, y);
}

/*!
  \internal
*/
void QIconModeViewBase::doDynamicLayout(const QListViewLayoutInfo &info)
{
    const bool useItemSize = !info.grid.isValid();
    const QPoint topLeft = initDynamicLayout(info);

    int segStartPosition;
    int segEndPosition;
    int deltaFlowPosition;
    int deltaSegPosition;
    int deltaSegHint;
    int flowPosition;
    int segPosition;

    if (info.flow == QListView::LeftToRight) {
        segStartPosition = info.bounds.left() + info.spacing;
        segEndPosition = info.bounds.right();
        deltaFlowPosition = info.grid.width(); // dx
        deltaSegPosition = (useItemSize ? batchSavedDeltaSeg : info.grid.height()); // dy
        deltaSegHint = info.grid.height();
        flowPosition = topLeft.x();
        segPosition = topLeft.y();
    } else { // flow == QListView::TopToBottom
        segStartPosition = info.bounds.top() + info.spacing;
        segEndPosition = info.bounds.bottom();
        deltaFlowPosition = info.grid.height(); // dy
        deltaSegPosition = (useItemSize ? batchSavedDeltaSeg : info.grid.width()); // dx
        deltaSegHint = info.grid.width();
        flowPosition = topLeft.y();
        segPosition = topLeft.x();
    }

    if (moved.size() != items.size())
        moved.resize(items.size());

    QRect rect(QPoint(), topLeft);
    QListViewItem *item = nullptr;
    Q_ASSERT(info.first <= info.last);
    for (int row = info.first; row <= info.last; ++row) {
        item = &items[row];
        if (isHidden(row)) {
            item->invalidate();
        } else {
            // if we are not using a grid, we need to find the deltas
            if (useItemSize) {
                if (info.flow == QListView::LeftToRight)
                    deltaFlowPosition = item->w + info.spacing;
                else
                    deltaFlowPosition = item->h + info.spacing;
            } else {
                item->w = qMin<int>(info.grid.width(), item->w);
                item->h = qMin<int>(info.grid.height(), item->h);
            }

            // create new segment
            if (info.wrap
                && flowPosition + deltaFlowPosition > segEndPosition
                && flowPosition > segStartPosition) {
                flowPosition = segStartPosition;
                segPosition += deltaSegPosition;
                if (useItemSize)
                    deltaSegPosition = 0;
            }
            // We must delay calculation of the seg adjustment, as this item
            // may have caused a wrap to occur
            if (useItemSize) {
                if (info.flow == QListView::LeftToRight)
                    deltaSegHint = item->h + info.spacing;
                else
                    deltaSegHint = item->w + info.spacing;
                deltaSegPosition = qMax(deltaSegPosition, deltaSegHint);
            }

            // set the position of the item
            // ### idealy we should have some sort of alignment hint for the item
            // ### (normally that would be a point between the icon and the text)
            if (!moved.testBit(row)) {
                if (info.flow == QListView::LeftToRight) {
                    if (useItemSize) {
                        item->x = flowPosition;
                        item->y = segPosition;
                    } else { // use grid
                        item->x = flowPosition + ((deltaFlowPosition - item->w) / 2);
                        item->y = segPosition;
                    }
                } else { // TopToBottom
                    if (useItemSize) {
                        item->y = flowPosition;
                        item->x = segPosition;
                    } else { // use grid
                        item->y = flowPosition + ((deltaFlowPosition - item->h) / 2);
                        item->x = segPosition;
                    }
                }
            }

            // let the contents contain the new item
            if (useItemSize)
                rect |= item->rect();
            else if (info.flow == QListView::LeftToRight)
                rect |= QRect(flowPosition, segPosition, deltaFlowPosition, deltaSegPosition);
            else // flow == TopToBottom
                rect |= QRect(segPosition, flowPosition, deltaSegPosition, deltaFlowPosition);

            // prepare for next item
            flowPosition += deltaFlowPosition; // current position + item width + gap
        }
    }
    Q_ASSERT(item);
    batchSavedDeltaSeg = deltaSegPosition;
    batchStartRow = info.last + 1;
    bool done = (info.last >= rowCount() - 1);
    // resize the content area
    if (done || !info.bounds.contains(item->rect())) {
        contentsSize = rect.size();
        if (info.flow == QListView::LeftToRight)
            contentsSize.rheight() += info.spacing;
        else
            contentsSize.rwidth() += info.spacing;
    }
    if (rect.size().isEmpty())
        return;
    // resize tree
    int insertFrom = info.first;
    if (done || info.first == 0) {
        initBspTree(rect.size());
        insertFrom = 0;
    }
    // insert items in tree
    for (int row = insertFrom; row <= info.last; ++row)
        tree.insertLeaf(items.at(row).rect(), row);
    // if the new items are visible, update the viewport
    QRect changedRect(topLeft, rect.bottomRight());
    if (clipRect().intersects(changedRect))
        viewport()->update();
}

QList<QModelIndex> QIconModeViewBase::intersectingSet(const QRect &area) const
{
    QIconModeViewBase *that = const_cast<QIconModeViewBase*>(this);
    QBspTree::Data data(static_cast<void*>(that));
    QList<QModelIndex> res;
    that->interSectingVector = &res;
    that->tree.climbTree(area, &QIconModeViewBase::addLeaf, data);
    that->interSectingVector = nullptr;
    return res;
}

QRect QIconModeViewBase::itemsRect(const QList<QModelIndex> &indexes) const
{
    QRect rect;
    for (const auto &index : indexes)
        rect |= viewItemRect(indexToListViewItem(index));
    return rect;
}

int QIconModeViewBase::itemIndex(const QListViewItem &item) const
{
    if (!item.isValid())
        return -1;
    int i = item.indexHint;
    if (i < items.size()) {
        if (items.at(i) == item)
            return i;
    } else {
        i = items.size() - 1;
    }

    int j = i;
    int c = items.size();
    bool a = true;
    bool b = true;

    while (a || b) {
        if (a) {
            if (items.at(i) == item) {
                items.at(i).indexHint = i;
                return i;
            }
            a = ++i < c;
        }
        if (b) {
            if (items.at(j) == item) {
                items.at(j).indexHint = j;
                return j;
            }
            b = --j > -1;
        }
    }
    return -1;
}

void QIconModeViewBase::addLeaf(QList<int> &leaf, const QRect &area, uint visited,
                                QBspTree::Data data)
{
    QListViewItem *vi;
    QIconModeViewBase *_this = static_cast<QIconModeViewBase *>(data.ptr);
    for (int i = 0; i < leaf.size(); ++i) {
        int idx = leaf.at(i);
        if (idx < 0 || idx >= _this->items.size())
            continue;
        vi = &_this->items[idx];
        Q_ASSERT(vi);
        if (vi->isValid() && vi->rect().intersects(area) && vi->visited != visited) {
            QModelIndex index  = _this->dd->listViewItemToIndex(*vi);
            Q_ASSERT(index.isValid());
            _this->interSectingVector->append(index);
            vi->visited = visited;
        }
    }
}

void QIconModeViewBase::moveItem(int index, const QPoint &dest)
{
    // does not impact on the bintree itself or the contents rect
    QListViewItem *item = &items[index];
    QRect rect = item->rect();

    // move the item without removing it from the tree
    tree.removeLeaf(rect, index);
    item->move(dest);
    tree.insertLeaf(QRect(dest, rect.size()), index);

    // resize the contents area
    contentsSize = (QRect(QPoint(0, 0), contentsSize)|QRect(dest, rect.size())).size();

    // mark the item as moved
    if (moved.size() != items.size())
        moved.resize(items.size());
    moved.setBit(index, true);
}

QPoint QIconModeViewBase::snapToGrid(const QPoint &pos) const
{
    int x = pos.x() - (pos.x() % gridSize().width());
    int y = pos.y() - (pos.y() % gridSize().height());
    return QPoint(x, y);
}

QPoint QIconModeViewBase::draggedItemsDelta() const
{
    if (movement() == QListView::Snap) {
        QPoint snapdelta = QPoint((offset().x() % gridSize().width()),
                                  (offset().y() % gridSize().height()));
        return snapToGrid(draggedItemsPos + snapdelta) - snapToGrid(pressedPosition()) - snapdelta;
    }
    return draggedItemsPos - pressedPosition();
}

QRect QIconModeViewBase::draggedItemsRect() const
{
    QRect rect = itemsRect(draggedItems);
    rect.translate(draggedItemsDelta());
    return rect;
}

void QListViewPrivate::scrollElasticBandBy(int dx, int dy)
{
    if (dx > 0) // right
        elasticBand.moveRight(elasticBand.right() + dx);
    else if (dx < 0) // left
        elasticBand.moveLeft(elasticBand.left() - dx);
    if (dy > 0) // down
        elasticBand.moveBottom(elasticBand.bottom() + dy);
    else if (dy < 0) // up
        elasticBand.moveTop(elasticBand.top() - dy);
}

void QIconModeViewBase::clear()
{
    tree.destroy();
    items.clear();
    moved.clear();
    batchStartRow = 0;
    batchSavedDeltaSeg = 0;
}

void QIconModeViewBase::updateContentsSize()
{
    QRect bounding;
    for (int i = 0; i < items.size(); ++i)
        bounding |= items.at(i).rect();
    contentsSize = bounding.size();
}

/*!
  \reimp
*/
void QListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QAbstractItemView::currentChanged(current, previous);
#if QT_CONFIG(accessibility)
    if (QAccessible::isActive()) {
        if (current.isValid() && hasFocus()) {
            int entry = visualIndex(current);
            QAccessibleEvent event(this, QAccessible::Focus);
            event.setChild(entry);
            QAccessible::updateAccessibility(&event);
        }
    }
#endif
}

/*!
  \reimp
*/
void QListView::selectionChanged(const QItemSelection &selected,
                                 const QItemSelection &deselected)
{
#if QT_CONFIG(accessibility)
    if (QAccessible::isActive()) {
        // ### does not work properly for selection ranges.
        QModelIndex sel = selected.indexes().value(0);
        if (sel.isValid()) {
            int entry = visualIndex(sel);
            QAccessibleEvent event(this, QAccessible::SelectionAdd);
            event.setChild(entry);
            QAccessible::updateAccessibility(&event);
        }
        QModelIndex desel = deselected.indexes().value(0);
        if (desel.isValid()) {
            int entry = visualIndex(desel);
            QAccessibleEvent event(this, QAccessible::SelectionRemove);
            event.setChild(entry);
            QAccessible::updateAccessibility(&event);
        }
    }
#endif
    QAbstractItemView::selectionChanged(selected, deselected);
}

int QListView::visualIndex(const QModelIndex &index) const
{
    Q_D(const QListView);
    d->executePostedLayout();
    QListViewItem itm = d->indexToListViewItem(index);
    int visualIndex = d->commonListView->itemIndex(itm);
    for (const auto &idx : std::as_const(d->hiddenRows)) {
        if (idx.row() <= index.row())
            --visualIndex;
    }
    return visualIndex;
}


/*!
    \since 5.2
    \reimp
*/
QSize QListView::viewportSizeHint() const
{
    Q_D(const QListView);
    // We don't have a nice simple size hint for invalid or wrapping list views.
    if (!d->model)
        return QAbstractItemView::viewportSizeHint();
    const int rc = d->model->rowCount();
    if (rc == 0 || isWrapping())
        return QAbstractItemView::viewportSizeHint();

    QStyleOptionViewItem option;
    initViewItemOption(&option);

    if (uniformItemSizes()) {
        QSize sz = d->cachedItemSize;
        if (!sz.isValid()) {
            QModelIndex idx = d->model->index(0, d->column, d->root);
            sz = d->itemSize(option, idx);
        }
        sz.setHeight(rc * sz.height());
        return sz;
    }

    // Using AdjustToContents with a high number of rows will normally not make sense, so we limit
    // this to default 1000 (that is btw the default for QHeaderView::resizeContentsPrecision())
    // (By setting the property _q_resizeContentPrecision the user can however override this).
    int maximumRows = 1000;
    const QVariant userOverrideValue = property("_q_resizeContentPrecision");
    if (userOverrideValue.isValid() && userOverrideValue.toInt() > 0) {
      maximumRows = userOverrideValue.toInt();
    }
    const int rowCount = qMin(rc, maximumRows);

    int h = 0;
    int w = 0;

    for (int row = 0; row < rowCount; ++row) {
        QModelIndex idx = d->model->index(row, d->column, d->root);
        QSize itemSize = d->itemSize(option, idx);
        h += itemSize.height();
        w = qMax(w, itemSize.width());
    }
    return QSize(w, h);
}

QT_END_NAMESPACE

#include "moc_qlistview.cpp"
