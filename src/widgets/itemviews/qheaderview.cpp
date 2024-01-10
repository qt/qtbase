// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qheaderview.h"

#include <qabstractitemdelegate.h>
#include <qapplication.h>
#include <qbitarray.h>
#include <qbrush.h>
#include <qdebug.h>
#include <qevent.h>
#include <qlist.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <qstyle.h>
#include <qstyleoption.h>
#if QT_CONFIG(tooltip)
#include <qtooltip.h>
#endif
#include <qvarlengtharray.h>
#include <qvariant.h>
#if QT_CONFIG(whatsthis)
#include <qwhatsthis.h>
#endif
#include <private/qheaderview_p.h>
#include <private/qabstractitemmodel_p.h>
#include <private/qabstractitemdelegate_p.h>

#ifndef QT_NO_DATASTREAM
#include <qdatastream.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &out, const QHeaderViewPrivate::SectionItem &section)
{
    section.write(out);
    return out;
}

QDataStream &operator>>(QDataStream &in, QHeaderViewPrivate::SectionItem &section)
{
    section.read(in);
    return in;
}
#endif // QT_NO_DATASTREAM

static const int maxSizeSection = 1048575; // since section size is in a bitfield (uint 20). See qheaderview_p.h
                                           // if this is changed then the docs in maximumSectionSize should be changed.

/*!
    \class QHeaderView

    \brief The QHeaderView class provides a header row or header column for
    item views.

    \ingroup model-view
    \inmodule QtWidgets

    A QHeaderView displays the headers used in item views such as the
    QTableView and QTreeView classes. It takes the place of Qt3's \c QHeader
    class previously used for the same purpose, but uses the Qt's model/view
    architecture for consistency with the item view classes.

    The QHeaderView class is one of the \l{Model/View Classes} and is part of
    Qt's \l{Model/View Programming}{model/view framework}.

    The header gets the data for each section from the model using the
    QAbstractItemModel::headerData() function. You can set the data by using
    QAbstractItemModel::setHeaderData().

    Each header has an orientation() and a number of sections, given by the
    count() function. A section refers to a part of the header - either a row
    or a column, depending on the orientation.

    Sections can be moved and resized using moveSection() and resizeSection();
    they can also be hidden and shown with hideSection() and showSection().

    Each section of a header is described by a section ID, specified by its
    section(), and can be located at a particular visualIndex() in the header.
    A section can have a sort indicator set with setSortIndicator(); this
    indicates whether the items in the associated item view will be sorted in
    the order given by the section.

    For a horizontal header the section is equivalent to a column in the model,
    and for a vertical header the section is equivalent to a row in the model.

    \section1 Moving Header Sections

    A header can be fixed in place, or made movable with setSectionsMovable(). It can
    be made clickable with setSectionsClickable(), and has resizing behavior in
    accordance with setSectionResizeMode().

    \note Double-clicking on a header to resize a section only applies for
    visible rows.

    A header will emit sectionMoved() if the user moves a section,
    sectionResized() if the user resizes a section, and sectionClicked() as
    well as sectionHandleDoubleClicked() in response to mouse clicks. A header
    will also emit sectionCountChanged().

    You can identify a section using the logicalIndex() and logicalIndexAt()
    functions, or by its index position, using the visualIndex() and
    visualIndexAt() functions. The visual index will change if a section is
    moved, but the logical index will not change.

    \section1 Appearance

    QTableWidget and QTableView create default headers. If you want
    the headers to be visible, you can use \l{QFrame::}{setVisible()}.

    Not all \l{Qt::}{ItemDataRole}s will have an effect on a
    QHeaderView. If you need to draw other roles, you can subclass
    QHeaderView and reimplement \l{QHeaderView::}{paintEvent()}.
    QHeaderView respects the following item data roles, unless they are
    in conflict with the style (which can happen for styles that follow
    the desktop theme):

    \l{Qt::}{TextAlignmentRole}, \l{Qt::}{DisplayRole},
    \l{Qt::}{FontRole}, \l{Qt::}{DecorationRole},
    \l{Qt::}{ForegroundRole}, and \l{Qt::}{BackgroundRole}.

    \note Each header renders the data for each section itself, and does not
    rely on a delegate. As a result, calling a header's setItemDelegate()
    function will have no effect.

    \sa {Model/View Programming}, QListView, QTableView, QTreeView
*/

/*!
    \enum QHeaderView::ResizeMode

    The resize mode specifies the behavior of the header sections. It can be
    set on the entire header view or on individual sections using
    setSectionResizeMode().

    \value Interactive The user can resize the section. The section can also be
           resized programmatically using resizeSection().  The section size
           defaults to \l defaultSectionSize. (See also
           \l cascadingSectionResizes.)

    \value Fixed The user cannot resize the section. The section can only be
           resized programmatically using resizeSection(). The section size
           defaults to \l defaultSectionSize.

    \value Stretch QHeaderView will automatically resize the section to fill
           the available space. The size cannot be changed by the user or
           programmatically.

    \value ResizeToContents QHeaderView will automatically resize the section
           to its optimal size based on the contents of the entire column or
           row. The size cannot be changed by the user or programmatically.
           (This value was introduced in 4.2)

    The following values are obsolete:
    \value Custom Use Fixed instead.

    \sa setSectionResizeMode(), stretchLastSection, minimumSectionSize
*/

/*!
    \fn void QHeaderView::sectionMoved(int logicalIndex, int oldVisualIndex,
    int newVisualIndex)

    This signal is emitted when a section is moved. The section's logical index
    is specified by \a logicalIndex, the old index by \a oldVisualIndex, and
    the new index position by \a newVisualIndex.

    \sa moveSection()
*/

/*!
    \fn void QHeaderView::sectionResized(int logicalIndex, int oldSize,
    int newSize)

    This signal is emitted when a section is resized. The section's logical
    number is specified by \a logicalIndex, the old size by \a oldSize, and the
    new size by \a newSize.

    \sa resizeSection()
*/

/*!
    \fn void QHeaderView::sectionPressed(int logicalIndex)

    This signal is emitted when a section is pressed. The section's logical
    index is specified by \a logicalIndex.

    \sa setSectionsClickable()
*/

/*!
    \fn void QHeaderView::sectionClicked(int logicalIndex)

    This signal is emitted when a section is clicked. The section's logical
    index is specified by \a logicalIndex.

    Note that the sectionPressed signal will also be emitted.

    \sa setSectionsClickable(), sectionPressed()
*/

/*!
    \fn void QHeaderView::sectionEntered(int logicalIndex)
    \since 4.3

    This signal is emitted when the cursor moves over the section and the left
    mouse button is pressed. The section's logical index is specified by
    \a logicalIndex.

    \sa setSectionsClickable(), sectionPressed()
*/

/*!
    \fn void QHeaderView::sectionDoubleClicked(int logicalIndex)

    This signal is emitted when a section is double-clicked. The section's
    logical index is specified by \a logicalIndex.

    \sa setSectionsClickable()
*/

/*!
    \fn void QHeaderView::sectionCountChanged(int oldCount, int newCount)

    This signal is emitted when the number of sections changes, i.e., when
    sections are added or deleted. The original count is specified by
    \a oldCount, and the new count by \a newCount.

    \sa count(), length(), headerDataChanged()
*/

/*!
    \fn void QHeaderView::sectionHandleDoubleClicked(int logicalIndex)

    This signal is emitted when a section is double-clicked. The section's
    logical index is specified by \a logicalIndex.

    \sa setSectionsClickable()
*/

/*!
    \fn void QHeaderView::sortIndicatorChanged(int logicalIndex,
    Qt::SortOrder order)
    \since 4.3

    This signal is emitted when the section containing the sort indicator or
    the order indicated is changed. The section's logical index is specified
    by \a logicalIndex and the sort order is specified by \a order.

    \sa setSortIndicator()
*/

/*!
    \fn void QHeaderView::geometriesChanged()
    \since 4.2

    This signal is emitted when the header's geometries have changed.
*/

/*!
    \property QHeaderView::highlightSections
    \brief whether the sections containing selected items are highlighted

    By default, this property is \c false.
*/

/*!
    Creates a new generic header with the given \a orientation and \a parent.
*/
QHeaderView::QHeaderView(Qt::Orientation orientation, QWidget *parent)
    : QAbstractItemView(*new QHeaderViewPrivate, parent)
{
    Q_D(QHeaderView);
    d->setDefaultValues(orientation);
    initialize();
}

/*!
  \internal
*/
QHeaderView::QHeaderView(QHeaderViewPrivate &dd,
                         Qt::Orientation orientation, QWidget *parent)
    : QAbstractItemView(dd, parent)
{
    Q_D(QHeaderView);
    d->setDefaultValues(orientation);
    initialize();
}

/*!
  Destroys the header.
*/

QHeaderView::~QHeaderView()
{
}

/*!
  \internal
*/
void QHeaderView::initialize()
{
    Q_D(QHeaderView);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(NoFrame);
    setFocusPolicy(Qt::NoFocus);
    d->viewport->setMouseTracking(true);
    d->viewport->setBackgroundRole(QPalette::Button);
    d->textElideMode = Qt::ElideNone;
    delete d->itemDelegate;
}

/*!
  \reimp
*/
void QHeaderView::setModel(QAbstractItemModel *model)
{
    if (model == this->model())
        return;
    Q_D(QHeaderView);
    d->layoutChangePersistentSections.clear();
    if (d->model && d->model != QAbstractItemModelPrivate::staticEmptyModel()) {
        if (d->orientation == Qt::Horizontal) {
            QObject::disconnect(d->model, SIGNAL(columnsInserted(QModelIndex,int,int)),
                                this, SLOT(sectionsInserted(QModelIndex,int,int)));
            QObject::disconnect(d->model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                                this, SLOT(sectionsAboutToBeRemoved(QModelIndex,int,int)));
            QObject::disconnect(d->model, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                                this, SLOT(_q_sectionsRemoved(QModelIndex,int,int)));
            QObject::disconnect(d->model, SIGNAL(columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                                this, SLOT(_q_sectionsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
            QObject::disconnect(d->model, SIGNAL(columnsMoved(QModelIndex,int,int,QModelIndex,int)),
                                this, SLOT(_q_sectionsMoved(QModelIndex,int,int,QModelIndex,int)));
        } else {
            QObject::disconnect(d->model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                                this, SLOT(sectionsInserted(QModelIndex,int,int)));
            QObject::disconnect(d->model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                                this, SLOT(sectionsAboutToBeRemoved(QModelIndex,int,int)));
            QObject::disconnect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                                this, SLOT(_q_sectionsRemoved(QModelIndex,int,int)));
            QObject::disconnect(d->model, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                                this, SLOT(_q_sectionsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
            QObject::disconnect(d->model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
                                this, SLOT(_q_sectionsMoved(QModelIndex,int,int,QModelIndex,int)));
        }
        QObject::disconnect(d->model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                            this, SLOT(headerDataChanged(Qt::Orientation,int,int)));
        QObject::disconnect(d->model, SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)),
                            this, SLOT(_q_sectionsAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
        QObject::disconnect(d->model, SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)),
                            this, SLOT(_q_sectionsChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
    }

    if (model && model != QAbstractItemModelPrivate::staticEmptyModel()) {
        if (d->orientation == Qt::Horizontal) {
            QObject::connect(model, SIGNAL(columnsInserted(QModelIndex,int,int)),
                             this, SLOT(sectionsInserted(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                             this, SLOT(sectionsAboutToBeRemoved(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                             this, SLOT(_q_sectionsRemoved(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                             this, SLOT(_q_sectionsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
            QObject::connect(model, SIGNAL(columnsMoved(QModelIndex,int,int,QModelIndex,int)),
                             this, SLOT(_q_sectionsMoved(QModelIndex,int,int,QModelIndex,int)));
        } else {
            QObject::connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                             this, SLOT(sectionsInserted(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                             this, SLOT(sectionsAboutToBeRemoved(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                             this, SLOT(_q_sectionsRemoved(QModelIndex,int,int)));
            QObject::connect(model, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                             this, SLOT(_q_sectionsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
            QObject::connect(model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
                             this, SLOT(_q_sectionsMoved(QModelIndex,int,int,QModelIndex,int)));
        }
        QObject::connect(model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                         this, SLOT(headerDataChanged(Qt::Orientation,int,int)));
        QObject::connect(model, SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)),
                         this, SLOT(_q_sectionsAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
        QObject::connect(model, SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)),
                         this, SLOT(_q_sectionsChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
    }

    d->state = QHeaderViewPrivate::NoClear;
    QAbstractItemView::setModel(model);
    d->state = QHeaderViewPrivate::NoState;

    // Users want to set sizes and modes before the widget is shown.
    // Thus, we have to initialize when the model is set,
    // and not lazily like we do in the other views.
    initializeSections();
}

/*!
    Returns the orientation of the header.

    \sa Qt::Orientation
*/

Qt::Orientation QHeaderView::orientation() const
{
    Q_D(const QHeaderView);
    return d->orientation;
}

/*!
    Returns the offset of the header: this is the header's left-most (or
    top-most for vertical headers) visible pixel.

    \sa setOffset()
*/

int QHeaderView::offset() const
{
    Q_D(const QHeaderView);
    return d->offset;
}

/*!
    \fn void QHeaderView::setOffset(int offset)

    Sets the header's offset to \a offset.

    \sa offset(), length()
*/

void QHeaderView::setOffset(int newOffset)
{
    Q_D(QHeaderView);
    if (d->offset == (int)newOffset)
        return;
    int ndelta = d->offset - newOffset;
    d->offset = newOffset;
    if (d->orientation == Qt::Horizontal)
        d->viewport->scroll(isRightToLeft() ? -ndelta : ndelta, 0);
    else
        d->viewport->scroll(0, ndelta);
    if (d->state == QHeaderViewPrivate::ResizeSection && !d->preventCursorChangeInSetOffset) {
        QPoint cursorPos = QCursor::pos();
        if (d->orientation == Qt::Horizontal)
            QCursor::setPos(cursorPos.x() + ndelta, cursorPos.y());
        else
            QCursor::setPos(cursorPos.x(), cursorPos.y() + ndelta);
        d->firstPos += ndelta;
        d->lastPos += ndelta;
    }
}

/*!
    \since 4.2
    Sets the offset to the start of the section at the given \a visualSectionNumber.
    \a visualSectionNumber is the actual visible section when hiddenSections are
    not considered. That is not always the same as visualIndex().

    \sa setOffset(), sectionPosition()
*/
void QHeaderView::setOffsetToSectionPosition(int visualSectionNumber)
{
    Q_D(QHeaderView);
    if (visualSectionNumber > -1 && visualSectionNumber < d->sectionCount()) {
        int position = d->headerSectionPosition(d->adjustedVisualIndex(visualSectionNumber));
        setOffset(position);
    }
}

/*!
    \since 4.2
    Sets the offset to make the last section visible.

    \sa setOffset(), sectionPosition(), setOffsetToSectionPosition()
*/
void QHeaderView::setOffsetToLastSection()
{
    Q_D(const QHeaderView);
    int size = (d->orientation == Qt::Horizontal ? viewport()->width() : viewport()->height());
    int position = length() - size;
    setOffset(position);
}

/*!
    Returns the length along the orientation of the header.

    \sa sizeHint(), setSectionResizeMode(), offset()
*/

int QHeaderView::length() const
{
    Q_D(const QHeaderView);
    d->executePostedLayout();
    d->executePostedResize();
    //Q_ASSERT(d->headerLength() == d->length);
    return d->length;
}

/*!
    Returns a suitable size hint for this header.

    \sa sectionSizeHint()
*/

QSize QHeaderView::sizeHint() const
{
    Q_D(const QHeaderView);
    if (d->cachedSizeHint.isValid())
        return d->cachedSizeHint;
    d->cachedSizeHint = QSize(0, 0); //reinitialize the cached size hint
    const int sectionCount = count();

    // get size hint for the first n sections
    int i = 0;
    for (int checked = 0; checked < 100 && i < sectionCount; ++i) {
        if (isSectionHidden(i))
            continue;
        checked++;
        QSize hint = sectionSizeFromContents(i);
        d->cachedSizeHint = d->cachedSizeHint.expandedTo(hint);
    }
    // get size hint for the last n sections
    i = qMax(i, sectionCount - 100 );
    for (int j = sectionCount - 1, checked = 0; j >= i && checked < 100; --j) {
        if (isSectionHidden(j))
            continue;
        checked++;
        QSize hint = sectionSizeFromContents(j);
        d->cachedSizeHint = d->cachedSizeHint.expandedTo(hint);
    }
    return d->cachedSizeHint;
}

/*!
    \reimp
*/

void QHeaderView::setVisible(bool v)
{
    bool actualChange = (v != isVisible());
    QAbstractItemView::setVisible(v);
    if (actualChange) {
        QAbstractScrollArea *parent = qobject_cast<QAbstractScrollArea*>(parentWidget());
        if (parent)
            parent->updateGeometry();
    }
}


/*!
    Returns a suitable size hint for the section specified by \a logicalIndex.

    \sa sizeHint(), defaultSectionSize(), minimumSectionSize(), maximumSectionSize()
    Qt::SizeHintRole
*/

int QHeaderView::sectionSizeHint(int logicalIndex) const
{
    Q_D(const QHeaderView);
    if (isSectionHidden(logicalIndex))
        return 0;
    if (logicalIndex < 0 || logicalIndex >= count())
        return -1;
    QSize size;
    QVariant value = d->model->headerData(logicalIndex, d->orientation, Qt::SizeHintRole);
    if (value.isValid())
        size = qvariant_cast<QSize>(value);
    else
        size = sectionSizeFromContents(logicalIndex);
    int hint = d->orientation == Qt::Horizontal ? size.width() : size.height();
    return qBound(minimumSectionSize(), hint, maximumSectionSize());
}

/*!
    Returns the visual index of the section that covers the given \a position
    in the viewport.

    \sa logicalIndexAt()
*/

int QHeaderView::visualIndexAt(int position) const
{
    Q_D(const QHeaderView);
    int vposition = position;
    d->executePostedLayout();
    d->executePostedResize();
    const int count = d->sectionCount();
    if (count < 1)
        return -1;

    if (d->reverse())
        vposition = d->viewport->width() - vposition - 1;
    vposition += d->offset;

    if (vposition > d->length)
        return -1;
    int visual = d->headerVisualIndexAt(vposition);
    if (visual < 0)
        return -1;

    while (d->isVisualIndexHidden(visual)){
        ++visual;
        if (visual >= count)
            return -1;
    }
    return visual;
}

/*!
    Returns the section that covers the given \a position in the viewport.

    \sa visualIndexAt(), isSectionHidden()
*/

int QHeaderView::logicalIndexAt(int position) const
{
    const int visual = visualIndexAt(position);
    if (visual > -1)
        return logicalIndex(visual);
    return -1;
}

/*!
    Returns the width (or height for vertical headers) of the given
    \a logicalIndex.

    \sa length(), setSectionResizeMode(), defaultSectionSize()
*/

int QHeaderView::sectionSize(int logicalIndex) const
{
    Q_D(const QHeaderView);
    if (isSectionHidden(logicalIndex))
        return 0;
    if (logicalIndex < 0 || logicalIndex >= count())
        return 0;
    int visual = visualIndex(logicalIndex);
    if (visual == -1)
        return 0;
    d->executePostedResize();
    return d->headerSectionSize(visual);
}

/*!

    Returns the section position of the given \a logicalIndex, or -1
    if the section is hidden. The position is measured in pixels from
    the first visible item's top-left corner to the top-left corner of
    the item with \a logicalIndex. The measurement is along the x-axis
    for horizontal headers and along the y-axis for vertical headers.

    \sa sectionViewportPosition()
*/

int QHeaderView::sectionPosition(int logicalIndex) const
{
    Q_D(const QHeaderView);
    int visual = visualIndex(logicalIndex);
    // in some cases users may change the selections
    // before we have a chance to do the layout
    if (visual == -1)
        return -1;
    d->executePostedResize();
    return d->headerSectionPosition(visual);
}

/*!
    Returns the section viewport position of the given \a logicalIndex.

    If the section is hidden, the return value is undefined.

    \sa sectionPosition(), isSectionHidden()
*/

int QHeaderView::sectionViewportPosition(int logicalIndex) const
{
    Q_D(const QHeaderView);
    if (logicalIndex >= count())
        return -1;
    int position = sectionPosition(logicalIndex);
    if (position < 0)
        return position; // the section was hidden
    int offsetPosition = position - d->offset;
    if (d->reverse())
        return d->viewport->width() - (offsetPosition + sectionSize(logicalIndex));
    return offsetPosition;
}

/*!
    \fn int QHeaderView::logicalIndexAt(int x, int y) const

    Returns the logical index of the section at the given coordinate. If the
    header is horizontal \a x will be used, otherwise \a y will be used to
    find the logical index.
*/

/*!
    \fn int QHeaderView::logicalIndexAt(const QPoint &pos) const

    Returns the logical index of the section at the position given in \a pos.
    If the header is horizontal the x-coordinate will be used, otherwise the
    y-coordinate will be used to find the logical index.

    \sa sectionPosition()
*/

template<typename Container>
static void qMoveRange(Container& c,
               typename Container::size_type rangeStart,
               typename Container::size_type rangeEnd,
               typename Container::size_type targetPosition)
{
    Q_ASSERT(targetPosition <= c.size());
    Q_ASSERT(targetPosition < rangeStart || targetPosition >= rangeEnd);

    const bool forwardMove = targetPosition > rangeStart;
    typename Container::size_type first = std::min(rangeStart, targetPosition);
    typename Container::size_type mid = forwardMove ? rangeEnd : rangeStart;
    typename Container::size_type last = forwardMove ? targetPosition + 1 : rangeEnd;
    std::rotate(c.begin() + first, c.begin() + mid, c.begin() + last);
}

/*!
    Moves the section at visual index \a from to occupy visual index \a to.

    \sa sectionsMoved()
*/

void QHeaderView::moveSection(int from, int to)
{
    Q_D(QHeaderView);

    d->executePostedLayout();
    if (from < 0 || from >= d->sectionCount() || to < 0 || to >= d->sectionCount())
        return;

    if (from == to) {
        int logical = logicalIndex(from);
        Q_ASSERT(logical != -1);
        updateSection(logical);
        return;
    }

    d->initializeIndexMapping();

    int *visualIndices = d->visualIndices.data();
    int *logicalIndices = d->logicalIndices.data();
    int logical = logicalIndices[from];
    int visual = from;

    if (to > from) {
        while (visual < to) {
            visualIndices[logicalIndices[visual + 1]] = visual;
            logicalIndices[visual] = logicalIndices[visual + 1];
            ++visual;
        }
    } else {
        while (visual > to) {
            visualIndices[logicalIndices[visual - 1]] = visual;
            logicalIndices[visual] = logicalIndices[visual - 1];
            --visual;
        }
    }
    visualIndices[logical] = to;
    logicalIndices[to] = logical;

    qMoveRange(d->sectionItems, from, from + 1, to);

    d->sectionStartposRecalc = true;

    if (d->hasAutoResizeSections())
        d->doDelayedResizeSections();
    d->viewport->update();

    emit sectionMoved(logical, from, to);

    if (stretchLastSection()) {
        const int lastSectionVisualIdx = visualIndex(d->lastSectionLogicalIdx);
        if (from >= lastSectionVisualIdx || to >= lastSectionVisualIdx)
            d->maybeRestorePrevLastSectionAndStretchLast();
    }
}

/*!
    \since 4.2
    Swaps the section at visual index \a first with the section at visual
    index \a second.

    \sa moveSection()
*/
void QHeaderView::swapSections(int first, int second)
{
    Q_D(QHeaderView);

    if (first == second)
        return;
    d->executePostedLayout();
    if (first < 0 || first >= d->sectionCount() || second < 0 || second >= d->sectionCount())
        return;

    int firstSize = d->headerSectionSize(first);
    ResizeMode firstMode = d->headerSectionResizeMode(first);
    int firstLogical = d->logicalIndex(first);

    int secondSize = d->headerSectionSize(second);
    ResizeMode secondMode = d->headerSectionResizeMode(second);
    int secondLogical = d->logicalIndex(second);

    if (d->state == QHeaderViewPrivate::ResizeSection)
        d->preventCursorChangeInSetOffset = true;

    d->createSectionItems(second, second, firstSize, firstMode);
    d->createSectionItems(first, first, secondSize, secondMode);

    d->initializeIndexMapping();

    d->visualIndices[firstLogical] = second;
    d->logicalIndices[second] = firstLogical;

    d->visualIndices[secondLogical] = first;
    d->logicalIndices[first] = secondLogical;

    if (!d->hiddenSectionSize.isEmpty()) {
        bool firstHidden = d->isVisualIndexHidden(first);
        bool secondHidden = d->isVisualIndexHidden(second);
        d->setVisualIndexHidden(first, secondHidden);
        d->setVisualIndexHidden(second, firstHidden);
    }

    d->viewport->update();
    emit sectionMoved(firstLogical, first, second);
    emit sectionMoved(secondLogical, second, first);

    if (stretchLastSection()) {
        const int lastSectionVisualIdx = visualIndex(d->lastSectionLogicalIdx);
        if (first >= lastSectionVisualIdx || second >= lastSectionVisualIdx)
            d->maybeRestorePrevLastSectionAndStretchLast();
    }
}

/*!
    \fn void QHeaderView::resizeSection(int logicalIndex, int size)

    Resizes the section specified by \a logicalIndex to \a size measured in
    pixels. The size parameter must be a value larger or equal to zero. A
    size equal to zero is however not recommended. In that situation hideSection
    should be used instead.

    \sa sectionResized(), sectionSize(), hideSection()
*/

void QHeaderView::resizeSection(int logical, int size)
{
    Q_D(QHeaderView);
    if (logical < 0 || logical >= count() || size < 0 || size > maxSizeSection)
        return;

    // make sure to not exceed bounds when setting size programmatically
    if (size > 0)
        size = qBound(minimumSectionSize(), size, maximumSectionSize());

    if (isSectionHidden(logical)) {
        d->hiddenSectionSize.insert(logical, size);
        return;
    }

    int visual = visualIndex(logical);
    if (visual == -1)
        return;

    if (d->state == QHeaderViewPrivate::ResizeSection && !d->cascadingResizing && logical != d->section)
        d->preventCursorChangeInSetOffset = true;

    int oldSize = d->headerSectionSize(visual);
    if (oldSize == size)
        return;

    d->executePostedLayout();
    d->invalidateCachedSizeHint();

    if (stretchLastSection() && logical == d->lastSectionLogicalIdx)
        d->lastSectionSize = size;

    d->createSectionItems(visual, visual, size, d->headerSectionResizeMode(visual));

    if (!updatesEnabled()) {
        if (d->hasAutoResizeSections())
            d->doDelayedResizeSections();
        emit sectionResized(logical, oldSize, size);
        return;
    }

    int w = d->viewport->width();
    int h = d->viewport->height();
    int pos = sectionViewportPosition(logical);
    QRect r;
    if (d->orientation == Qt::Horizontal)
        if (isRightToLeft())
            r.setRect(0, 0, pos + size, h);
        else
            r.setRect(pos, 0, w - pos, h);
    else
        r.setRect(0, pos, w, h - pos);

    if (d->hasAutoResizeSections()) {
        d->doDelayedResizeSections();
        r = d->viewport->rect();
    }

    // If the parent is a QAbstractScrollArea with QAbstractScrollArea::AdjustToContents
    // then we want to change the geometry on that widget. Not doing it at once can/will
    // cause scrollbars flicker as they would be shown at first but then removed.
    // In the same situation it will also allow shrinking the whole view when stretchLastSection is set
    // (It is default on QTreeViews - and it wouldn't shrink since the last stretch was made before the
    // viewport was resized)

    QAbstractScrollArea *parent = qobject_cast<QAbstractScrollArea *>(parentWidget());
    if (parent && parent->sizeAdjustPolicy() == QAbstractScrollArea::AdjustToContents)
        parent->updateGeometry();

    d->viewport->update(r.normalized());
    emit sectionResized(logical, oldSize, size);
}

/*!
    Resizes the sections according to the given \a mode, ignoring the current
    resize mode.

    \sa sectionResized()
*/

void QHeaderView::resizeSections(QHeaderView::ResizeMode mode)
{
    Q_D(QHeaderView);
    d->resizeSections(mode, true);
}

/*!
    \fn void QHeaderView::hideSection(int logicalIndex)
    Hides the section specified by \a logicalIndex.

    \sa showSection(), isSectionHidden(), hiddenSectionCount(),
    setSectionHidden()
*/

/*!
    \fn void QHeaderView::showSection(int logicalIndex)
    Shows the section specified by \a logicalIndex.

    \sa hideSection(), isSectionHidden(), hiddenSectionCount(),
    setSectionHidden()
*/

/*!
    Returns \c true if the section specified by \a logicalIndex is explicitly
    hidden from the user; otherwise returns \c false.

    \sa hideSection(), showSection(), setSectionHidden(), hiddenSectionCount()
*/

bool QHeaderView::isSectionHidden(int logicalIndex) const
{
    Q_D(const QHeaderView);
    d->executePostedLayout();
    if (d->hiddenSectionSize.isEmpty() || logicalIndex < 0 || logicalIndex >= d->sectionCount())
        return false;
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);
    return d->isVisualIndexHidden(visual);
}

/*!
    \since 4.1

    Returns the number of sections in the header that has been hidden.

    \sa setSectionHidden(), isSectionHidden()
*/
int QHeaderView::hiddenSectionCount() const
{
    Q_D(const QHeaderView);
    return d->hiddenSectionSize.size();
}

/*!
  If \a hide is true the section specified by \a logicalIndex is hidden;
  otherwise the section is shown.

  \sa isSectionHidden(), hiddenSectionCount()
*/

void QHeaderView::setSectionHidden(int logicalIndex, bool hide)
{
    Q_D(QHeaderView);
    if (logicalIndex < 0 || logicalIndex >= count())
        return;

    d->executePostedLayout();
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);
    if (hide == d->isVisualIndexHidden(visual))
        return;
    if (hide) {
        const bool isHidingLastSection = (stretchLastSection() && logicalIndex == d->lastSectionLogicalIdx);
        if (isHidingLastSection)
            d->restoreSizeOnPrevLastSection(); // Restore here/now to get the right restore size.
        int size = d->headerSectionSize(visual);
        if (!d->hasAutoResizeSections())
            resizeSection(logicalIndex, 0);
        d->hiddenSectionSize.insert(logicalIndex, size);
        d->setVisualIndexHidden(visual, true);
        if (isHidingLastSection)
            d->setNewLastSection(d->lastVisibleVisualIndex());
        if (d->hasAutoResizeSections())
            d->doDelayedResizeSections();
    } else {
        int size = d->hiddenSectionSize.value(logicalIndex, d->defaultSectionSize);
        d->hiddenSectionSize.remove(logicalIndex);
        d->setVisualIndexHidden(visual, false);
        resizeSection(logicalIndex, size);

        const bool newLastSection = (stretchLastSection() && visual > visualIndex(d->lastSectionLogicalIdx));
        if (newLastSection) {
            d->restoreSizeOnPrevLastSection();
            d->setNewLastSection(visual);
        }
    }
}

/*!
    Returns the number of sections in the header.

    \sa sectionCountChanged(), length()
*/

int QHeaderView::count() const
{
    Q_D(const QHeaderView);
    //Q_ASSERT(d->sectionCount == d->headerSectionCount());
    // ### this may affect the lazy layout
    d->executePostedLayout();
    return d->sectionCount();
}

/*!
    Returns the visual index position of the section specified by the given
    \a logicalIndex, or -1 otherwise.

    Hidden sections still have valid visual indexes.

    \sa logicalIndex()
*/

int QHeaderView::visualIndex(int logicalIndex) const
{
    Q_D(const QHeaderView);
    if (logicalIndex < 0)
        return -1;
    d->executePostedLayout();
    if (d->visualIndices.isEmpty()) { // nothing has been moved, so we have no mapping
        if (logicalIndex < d->sectionCount())
            return logicalIndex;
    } else if (logicalIndex < d->visualIndices.size()) {
        int visual = d->visualIndices.at(logicalIndex);
        Q_ASSERT(visual < d->sectionCount());
        return visual;
    }
    return -1;
}

/*!
    Returns the logicalIndex for the section at the given \a visualIndex
    position, or -1 if visualIndex < 0 or visualIndex >= QHeaderView::count().

    Note that the visualIndex is not affected by hidden sections.

    \sa visualIndex(), sectionPosition()
*/

int QHeaderView::logicalIndex(int visualIndex) const
{
    Q_D(const QHeaderView);
    if (visualIndex < 0 || visualIndex >= d->sectionCount())
        return -1;
    return d->logicalIndex(visualIndex);
}

/*!
    \since 5.0

    If \a movable is true, the header sections may be moved by the user;
    otherwise they are fixed in place.

    When used in combination with QTreeView, the first column is not
    movable (since it contains the tree structure), by default.
    You can make it movable with setFirstSectionMovable(true).

    \sa sectionsMovable(), sectionMoved()
    \sa setFirstSectionMovable()
*/

void QHeaderView::setSectionsMovable(bool movable)
{
    Q_D(QHeaderView);
    d->movableSections = movable;
}

/*!
    \since 5.0

    Returns \c true if the header can be moved by the user; otherwise returns
    false.

    By default, sections are movable in QTreeView (except for the first one),
    and not movable in QTableView.

    \sa setSectionsMovable()
*/

bool QHeaderView::sectionsMovable() const
{
    Q_D(const QHeaderView);
    return d->movableSections;
}

/*!
    \property QHeaderView::firstSectionMovable
    \brief Whether the first column can be moved by the user

    This property controls whether the first column can be moved by the user.
    In a QTreeView, the first column holds the tree structure and is
    therefore non-movable by default, even after setSectionsMovable(true).

    It can be made movable again, for instance in the case of flat lists
    without a tree structure, by calling this method.
    In such a scenario, it is recommended to call QTreeView::setRootIsDecorated(false)
    as well.

    \code
    treeView->setRootIsDecorated(false);
    treeView->header()->setFirstSectionMovable(true);
    \endcode

    Setting it to true has no effect unless setSectionsMovable(true) is called
    as well.

    \sa setSectionsMovable()
    \since 5.11
*/
void QHeaderView::setFirstSectionMovable(bool movable)
{
    Q_D(QHeaderView);
    d->allowUserMoveOfSection0 = movable;
}

bool QHeaderView::isFirstSectionMovable() const
{
    Q_D(const QHeaderView);
    return d->allowUserMoveOfSection0;
}

/*!
    \since 5.0

    If \a clickable is true, the header will respond to single clicks.

    \sa sectionsClickable(), sectionClicked(), sectionPressed(),
    setSortIndicatorShown()
*/

void QHeaderView::setSectionsClickable(bool clickable)
{
    Q_D(QHeaderView);
    d->clickableSections = clickable;
}

/*!
    \since 5.0

    Returns \c true if the header is clickable; otherwise returns \c false. A
    clickable header could be set up to allow the user to change the
    representation of the data in the view related to the header.

    \sa setSectionsClickable()
*/

bool QHeaderView::sectionsClickable() const
{
    Q_D(const QHeaderView);
    return d->clickableSections;
}

void QHeaderView::setHighlightSections(bool highlight)
{
    Q_D(QHeaderView);
    d->highlightSelected = highlight;
}

bool QHeaderView::highlightSections() const
{
    Q_D(const QHeaderView);
    return d->highlightSelected;
}

/*!
    \since 5.0

    Sets the constraints on how the header can be resized to those described
    by the given \a mode.

    \sa length(), sectionResized()
*/

void QHeaderView::setSectionResizeMode(ResizeMode mode)
{
    Q_D(QHeaderView);
    initializeSections();
    d->stretchSections = (mode == Stretch ? count() : 0);
    d->contentsSections =  (mode == ResizeToContents ? count() : 0);
    d->setGlobalHeaderResizeMode(mode);
    if (d->hasAutoResizeSections())
        d->doDelayedResizeSections(); // section sizes may change as a result of the new mode
}

/*!
    \since 5.0

    Sets the constraints on how the section specified by \a logicalIndex in
    the header can be resized to those described by the given \a mode. The logical
    index should exist at the time this function is called.

    \note This setting will be ignored for the last section if the stretchLastSection
    property is set to true. This is the default for the horizontal headers provided
    by QTreeView.

    \sa setStretchLastSection(), resizeContentsPrecision()
*/

void QHeaderView::setSectionResizeMode(int logicalIndex, ResizeMode mode)
{
    Q_D(QHeaderView);
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);

    ResizeMode old = d->headerSectionResizeMode(visual);
    d->setHeaderSectionResizeMode(visual, mode);

    if (mode == Stretch && old != Stretch)
        ++d->stretchSections;
    else if (mode == ResizeToContents && old != ResizeToContents)
        ++d->contentsSections;
    else if (mode != Stretch && old == Stretch)
        --d->stretchSections;
    else if (mode != ResizeToContents && old == ResizeToContents)
        --d->contentsSections;

    if (d->hasAutoResizeSections() && d->state == QHeaderViewPrivate::NoState)
        d->doDelayedResizeSections(); // section sizes may change as a result of the new mode
}

/*!
    \since 5.0

    Returns the resize mode that applies to the section specified by the given
    \a logicalIndex.

    \sa setSectionResizeMode()
*/

QHeaderView::ResizeMode QHeaderView::sectionResizeMode(int logicalIndex) const
{
    Q_D(const QHeaderView);
    int visual = visualIndex(logicalIndex);
    if (visual == -1)
        return Fixed; //the default value
    return d->headerSectionResizeMode(visual);
}

/*!
   \since 5.2
   Sets how precise QHeaderView should calculate the size when ResizeToContents is used.
   A low value will provide a less accurate but fast auto resize while a higher
   value will provide a more accurate resize that however can be slow.

   The number \a precision specifies how many sections that should be consider
   when calculating the preferred size.

   The default value is 1000 meaning that a horizontal column with auto-resize will look
   at maximum 1000 rows on calculating when doing an auto resize.

   Special value 0 means that it will look at only the visible area.
   Special value -1 will imply looking at all elements.

   This value is used in QTableView::sizeHintForColumn(), QTableView::sizeHintForRow()
   and QTreeView::sizeHintForColumn(). Reimplementing these functions can make this
   function not having an effect.

    \sa resizeContentsPrecision(), setSectionResizeMode(), resizeSections(), QTableView::sizeHintForColumn(), QTableView::sizeHintForRow(), QTreeView::sizeHintForColumn()
*/

void QHeaderView::setResizeContentsPrecision(int precision)
{
    Q_D(QHeaderView);
    d->resizeContentsPrecision = precision;
}

/*!
  \since 5.2
  Returns how precise QHeaderView will calculate on ResizeToContents.

  \sa setResizeContentsPrecision(), setSectionResizeMode()

*/

int QHeaderView::resizeContentsPrecision() const
{
    Q_D(const QHeaderView);
    return d->resizeContentsPrecision;
}

/*!
    \since 4.1

    Returns the number of sections that are set to resize mode stretch. In
    views, this can be used to see if the headerview needs to resize the
    sections when the view's geometry changes.

    \sa stretchLastSection
*/

int QHeaderView::stretchSectionCount() const
{
    Q_D(const QHeaderView);
    return d->stretchSections;
}

/*!
  \property QHeaderView::showSortIndicator
  \brief whether the sort indicator is shown

  By default, this property is \c false.

  \sa setSectionsClickable()
*/

void QHeaderView::setSortIndicatorShown(bool show)
{
    Q_D(QHeaderView);
    if (d->sortIndicatorShown == show)
        return;

    d->sortIndicatorShown = show;

    if (sortIndicatorSection() < 0 || sortIndicatorSection() > count())
        return;

    if (d->headerSectionResizeMode(sortIndicatorSection()) == ResizeToContents)
        resizeSections();

    d->viewport->update();
}

bool QHeaderView::isSortIndicatorShown() const
{
    Q_D(const QHeaderView);
    return d->sortIndicatorShown;
}

/*!
    Sets the sort indicator for the section specified by the given
    \a logicalIndex in the direction specified by \a order, and removes the
    sort indicator from any other section that was showing it.

    \a logicalIndex may be -1, in which case no sort indicator will be shown
    and the model will return to its natural, unsorted order. Note that not
    all models support this and may even crash in this case.

    \sa sortIndicatorSection(), sortIndicatorOrder()
*/

void QHeaderView::setSortIndicator(int logicalIndex, Qt::SortOrder order)
{
    Q_D(QHeaderView);

    // This is so that people can set the position of the sort indicator before the fill the model
    int old = d->sortIndicatorSection;
    if (old == logicalIndex && order == d->sortIndicatorOrder)
        return;
    d->sortIndicatorSection = logicalIndex;
    d->sortIndicatorOrder = order;

    if (logicalIndex >= d->sectionCount()) {
        emit sortIndicatorChanged(logicalIndex, order);
        return; // nothing to do
    }

    if (old != logicalIndex
        && ((logicalIndex >= 0 && sectionResizeMode(logicalIndex) == ResizeToContents)
            || old >= d->sectionCount() || (old >= 0 && sectionResizeMode(old) == ResizeToContents))) {
        resizeSections();
        d->viewport->update();
    } else {
        if (old >= 0 && old != logicalIndex)
            updateSection(old);
        if (logicalIndex >= 0)
            updateSection(logicalIndex);
    }

    emit sortIndicatorChanged(logicalIndex, order);
}

/*!
    Returns the logical index of the section that has a sort indicator.
    By default this is section 0.

    \sa setSortIndicator(), sortIndicatorOrder(), setSortIndicatorShown()
*/

int QHeaderView::sortIndicatorSection() const
{
    Q_D(const QHeaderView);
    return d->sortIndicatorSection;
}

/*!
    Returns the order for the sort indicator. If no section has a sort
    indicator the return value of this function is undefined.

    \sa setSortIndicator(), sortIndicatorSection()
*/

Qt::SortOrder QHeaderView::sortIndicatorOrder() const
{
    Q_D(const QHeaderView);
    return d->sortIndicatorOrder;
}

/*!
    \property QHeaderView::sortIndicatorClearable
    \brief Whether the sort indicator can be cleared by clicking on a section multiple times
    \since 6.1

    This property controls whether the user is able to remove the
    sorting indicator on a given section by clicking on the section
    multiple times. Normally, clicking on a section will simply change
    the sorting order for that section. By setting this property to
    true, the sorting indicator will be cleared after alternating to
    ascending and descending; this will typically restore the original
    sorting of a model.

    Setting this property to true has no effect unless
    sectionsClickable() is also true (which is the default for certain
    views, for instance QTableView, or is automatically set when making
    a view sortable, for instance by calling
    QTreeView::setSortingEnabled).
*/

void QHeaderView::setSortIndicatorClearable(bool clearable)
{
    Q_D(QHeaderView);
    if (d->sortIndicatorClearable == clearable)
        return;
    d->sortIndicatorClearable = clearable;
    emit sortIndicatorClearableChanged(clearable);
}

bool QHeaderView::isSortIndicatorClearable() const
{
    Q_D(const QHeaderView);
    return d->sortIndicatorClearable;
}

/*!
    \property QHeaderView::stretchLastSection
    \brief whether the last visible section in the header takes up all the
    available space

    The default value is false.

    \note The horizontal headers provided by QTreeView are configured with this
    property set to true, ensuring that the view does not waste any of the
    space assigned to it for its header. If this value is set to true, this
    property will override the resize mode set on the last section in the
    header.

    \sa setSectionResizeMode()
*/
bool QHeaderView::stretchLastSection() const
{
    Q_D(const QHeaderView);
    return d->stretchLastSection;
}

void QHeaderView::setStretchLastSection(bool stretch)
{
    Q_D(QHeaderView);
    if (d->stretchLastSection == stretch)
        return;
    d->stretchLastSection = stretch;
    if (d->state != QHeaderViewPrivate::NoState)
        return;
    if (stretch) {
        d->setNewLastSection(d->lastVisibleVisualIndex());
        resizeSections();
    } else {
        d->restoreSizeOnPrevLastSection();
    }
}

/*!
    \since 4.2
    \property QHeaderView::cascadingSectionResizes
    \brief whether interactive resizing will be cascaded to the following
    sections once the section being resized by the user has reached its
    minimum size

    This property only affects sections that have \l Interactive as their
    resize mode.

    The default value is false.

    \sa setSectionResizeMode()
*/
bool QHeaderView::cascadingSectionResizes() const
{
    Q_D(const QHeaderView);
    return d->cascadingResizing;
}

void QHeaderView::setCascadingSectionResizes(bool enable)
{
    Q_D(QHeaderView);
    d->cascadingResizing = enable;
}

/*!
    \property QHeaderView::defaultSectionSize
    \brief the default size of the header sections before resizing.

    This property only affects sections that have \l Interactive or \l Fixed
    as their resize mode.

    By default, the value of this property is style dependent.
    Thus, when the style changes, this property updates from it.
    Calling setDefaultSectionSize() stops the updates, calling
    resetDefaultSectionSize() will restore default behavior.

    \sa setSectionResizeMode(), minimumSectionSize
*/
int QHeaderView::defaultSectionSize() const
{
    Q_D(const QHeaderView);
    return d->defaultSectionSize;
}

void QHeaderView::setDefaultSectionSize(int size)
{
    Q_D(QHeaderView);
    if (size < 0 || size > maxSizeSection)
        return;
    d->setDefaultSectionSize(size);
}

void QHeaderView::resetDefaultSectionSize()
{
    Q_D(QHeaderView);
    if (d->customDefaultSectionSize) {
        d->updateDefaultSectionSizeFromStyle();
        d->customDefaultSectionSize = false;
    }
}

/*!
    \since 4.2
    \property QHeaderView::minimumSectionSize
    \brief the minimum size of the header sections.

    The minimum section size is the smallest section size allowed. If the
    minimum section size is set to -1, QHeaderView will use the
    \l{fontMetrics()}{font metrics} size.

    This property is honored by all \l{ResizeMode}{resize modes}.

    \sa setSectionResizeMode(), defaultSectionSize
*/
int QHeaderView::minimumSectionSize() const
{
    Q_D(const QHeaderView);
    if (d->minimumSectionSize == -1) {
        int margin = 2 * style()->pixelMetric(QStyle::PM_HeaderMargin, nullptr, this);
        if (d->orientation == Qt::Horizontal)
            return fontMetrics().maxWidth() + margin;
        return fontMetrics().height() + margin;
    }
    return d->minimumSectionSize;
}

void QHeaderView::setMinimumSectionSize(int size)
{
    Q_D(QHeaderView);
    if (size < -1 || size > maxSizeSection)
        return;
    // larger new min size - check current section sizes
    const bool needSizeCheck = size > d->minimumSectionSize;
    d->minimumSectionSize = size;
    if (d->minimumSectionSize > maximumSectionSize())
        setMaximumSectionSize(size);

    if (needSizeCheck) {
        if (d->hasAutoResizeSections()) {
            d->doDelayedResizeSections();
        } else {
            for (int visual = 0; visual < d->sectionCount(); ++visual) {
                if (d->isVisualIndexHidden(visual))
                    continue;
                if (d->headerSectionSize(visual) < d->minimumSectionSize)
                    resizeSection(logicalIndex(visual), size);
            }
        }
    }

}

/*!
    \since 5.2
    \property QHeaderView::maximumSectionSize
    \brief the maximum size of the header sections.

    The maximum section size is the largest section size allowed.
    The default value for this property is 1048575, which is also the largest
    possible size for a section. Setting maximum to -1 will reset the value to
    the largest section size.

    With exception of stretch this property is honored by all \l{ResizeMode}{resize modes}

    \sa setSectionResizeMode(), defaultSectionSize
*/
int QHeaderView::maximumSectionSize() const
{
    Q_D(const QHeaderView);
    if (d->maximumSectionSize == -1)
        return maxSizeSection;
    return d->maximumSectionSize;
}

void QHeaderView::setMaximumSectionSize(int size)
{
    Q_D(QHeaderView);
    if (size == -1) {
        d->maximumSectionSize = maxSizeSection;
        return;
    }
    if (size < 0 || size > maxSizeSection)
        return;
    if (minimumSectionSize() > size)
        d->minimumSectionSize = size;

    // smaller new max size - check current section sizes
    const bool needSizeCheck = size < d->maximumSectionSize;
    d->maximumSectionSize = size;

    if (needSizeCheck) {
        if (d->hasAutoResizeSections()) {
            d->doDelayedResizeSections();
        } else {
            for (int visual = 0; visual < d->sectionCount(); ++visual) {
                if (d->isVisualIndexHidden(visual))
                    continue;
                if (d->headerSectionSize(visual) > d->maximumSectionSize)
                    resizeSection(logicalIndex(visual), size);
            }
        }
    }
}


/*!
    \since 4.1
    \property QHeaderView::defaultAlignment
    \brief the default alignment of the text in each header section
*/

Qt::Alignment QHeaderView::defaultAlignment() const
{
    Q_D(const QHeaderView);
    return d->defaultAlignment;
}

void QHeaderView::setDefaultAlignment(Qt::Alignment alignment)
{
    Q_D(QHeaderView);
    if (d->defaultAlignment == alignment)
        return;

    d->defaultAlignment = alignment;
    d->viewport->update();
}

/*!
    \internal
*/
void QHeaderView::doItemsLayout()
{
    initializeSections();
    QAbstractItemView::doItemsLayout();
}

/*!
    Returns \c true if sections in the header has been moved; otherwise returns
    false;

    \sa moveSection()
*/
bool QHeaderView::sectionsMoved() const
{
    Q_D(const QHeaderView);
    return !d->visualIndices.isEmpty();
}

/*!
    \since 4.1

    Returns \c true if sections in the header has been hidden; otherwise returns
    false;

    \sa setSectionHidden()
*/
bool QHeaderView::sectionsHidden() const
{
    Q_D(const QHeaderView);
    return !d->hiddenSectionSize.isEmpty();
}

#ifndef QT_NO_DATASTREAM
/*!
    \since 4.3

    Saves the current state of this header view.

    To restore the saved state, pass the return value to restoreState().

    \sa restoreState()
*/
QByteArray QHeaderView::saveState() const
{
    Q_D(const QHeaderView);
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_0);
    stream << QHeaderViewPrivate::VersionMarker;
    stream << 0; // current version is 0
    d->write(stream);
    return data;
}

/*!
    \since 4.3
    Restores the \a state of this header view.
    This function returns \c true if the state was restored; otherwise returns
    false.

    \sa saveState()
*/
bool QHeaderView::restoreState(const QByteArray &state)
{
    Q_D(QHeaderView);
    if (state.isEmpty())
        return false;

    for (const auto dataStreamVersion : {QDataStream::Qt_5_0, QDataStream::Qt_6_0}) {

        QByteArray data = state;
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream.setVersion(dataStreamVersion);
        int marker;
        int ver;
        stream >> marker;
        stream >> ver;
        if (stream.status() != QDataStream::Ok
        || marker != QHeaderViewPrivate::VersionMarker
        || ver != 0) { // current version is 0
            return false;
        }

        if (d->read(stream)) {
            emit sortIndicatorChanged(d->sortIndicatorSection, d->sortIndicatorOrder );
            d->viewport->update();
            return true;
        }
    }
    return false;
}
#endif // QT_NO_DATASTREAM

/*!
  \reimp
*/
void QHeaderView::reset()
{
    Q_D(QHeaderView);
    QAbstractItemView::reset();
    // it would be correct to call clear, but some apps rely
    // on the header keeping the sections, even after calling reset
    //d->clear();
    initializeSections();
    d->invalidateCachedSizeHint();
}

/*!
    Updates the changed header sections with the given \a orientation, from
    \a logicalFirst to \a logicalLast inclusive.
*/
void QHeaderView::headerDataChanged(Qt::Orientation orientation, int logicalFirst, int logicalLast)
{
    Q_D(QHeaderView);
    if (d->orientation != orientation)
        return;

    if (logicalFirst < 0 || logicalLast < 0 || logicalFirst >= count() || logicalLast >= count())
        return;

    d->invalidateCachedSizeHint();

    int firstVisualIndex = INT_MAX, lastVisualIndex = -1;

    for (int section = logicalFirst; section <= logicalLast; ++section) {
        const int visual = visualIndex(section);
        firstVisualIndex = qMin(firstVisualIndex, visual);
        lastVisualIndex =  qMax(lastVisualIndex,  visual);
    }

    d->executePostedResize();
    const int first = d->headerSectionPosition(firstVisualIndex),
              last = d->headerSectionPosition(lastVisualIndex)
                        + d->headerSectionSize(lastVisualIndex);

    if (orientation == Qt::Horizontal) {
        d->viewport->update(first, 0, last - first, d->viewport->height());
    } else {
        d->viewport->update(0, first, d->viewport->width(), last - first);
    }
}

/*!
    \internal
    \since 4.2

    Updates the section specified by the given \a logicalIndex.
*/

void QHeaderView::updateSection(int logicalIndex)
{
    Q_D(QHeaderView);
    if (d->orientation == Qt::Horizontal)
        d->viewport->update(QRect(sectionViewportPosition(logicalIndex),
                                  0, sectionSize(logicalIndex), d->viewport->height()));
    else
        d->viewport->update(QRect(0, sectionViewportPosition(logicalIndex),
                                  d->viewport->width(), sectionSize(logicalIndex)));
}

/*!
    Resizes the sections according to their size hints. Normally, you do not
    have to call this function.
*/

void QHeaderView::resizeSections()
{
    Q_D(QHeaderView);
    if (d->hasAutoResizeSections())
        d->resizeSections(Interactive, false); // no global resize mode
}

/*!
    This slot is called when sections are inserted into the \a parent.
    \a logicalFirst and \a logicalLast indices signify where the new sections
    were inserted.

    If only one section is inserted, \a logicalFirst and \a logicalLast will
    be the same.
*/

void QHeaderView::sectionsInserted(const QModelIndex &parent,
                                   int logicalFirst, int logicalLast)
{
    Q_D(QHeaderView);
    // only handle root level changes and return on no-op
    if (parent != d->root || d->modelSectionCount() == d->sectionCount())
        return;
    int oldCount = d->sectionCount();

    d->invalidateCachedSizeHint();

    if (d->state == QHeaderViewPrivate::ResizeSection)
        d->preventCursorChangeInSetOffset = true;

    // add the new sections
    int insertAt = logicalFirst;
    int insertCount = logicalLast - logicalFirst + 1;

    bool lastSectionActualChange = false;
    if (stretchLastSection()) {

        int visualIndexForStretch = d->lastSectionLogicalIdx;
        if (d->lastSectionLogicalIdx >= 0 && d->lastSectionLogicalIdx < d->visualIndices.size())
            visualIndexForStretch = d->visualIndices[d->lastSectionLogicalIdx]; // We cannot call visualIndex since it executes executePostedLayout()
                                                                                // and it is likely to bypass initializeSections() and we may end up here again. Doing the insert twice.

        if (d->lastSectionLogicalIdx < 0 || insertAt >= visualIndexForStretch)
            lastSectionActualChange = true;

        if (d->lastSectionLogicalIdx >= logicalFirst)
            d->lastSectionLogicalIdx += insertCount; // We do not want to emit resize before we have fixed the count
    }

    QHeaderViewPrivate::SectionItem section(d->defaultSectionSize, d->globalResizeMode);
    d->sectionStartposRecalc = true;

    if (d->sectionItems.isEmpty() || insertAt >= d->sectionItems.size()) {
        int insertLength = d->defaultSectionSize * insertCount;
        d->length += insertLength;
        d->sectionItems.insert(d->sectionItems.size(), insertCount, section); // append
    } else {
        // separate them out into their own sections
        int insertLength = d->defaultSectionSize * insertCount;
        d->length += insertLength;
        d->sectionItems.insert(insertAt, insertCount, section);
    }

    // update sorting column
    if (d->sortIndicatorSection >= logicalFirst)
        d->sortIndicatorSection += insertCount;

    // update resize mode section counts
    if (d->globalResizeMode == Stretch)
        d->stretchSections = d->sectionCount();
    else if (d->globalResizeMode == ResizeToContents)
        d->contentsSections = d->sectionCount();

    // clear selection cache
    d->sectionSelected.clear();

    // update mapping
    if (!d->visualIndices.isEmpty() && !d->logicalIndices.isEmpty()) {
        Q_ASSERT(d->visualIndices.size() == d->logicalIndices.size());
        int mappingCount = d->visualIndices.size();
        for (int i = 0; i < mappingCount; ++i) {
            if (d->visualIndices.at(i) >= logicalFirst)
               d->visualIndices[i] += insertCount;
            if (d->logicalIndices.at(i) >= logicalFirst)
                d->logicalIndices[i] += insertCount;
        }
        for (int j = logicalFirst; j <= logicalLast; ++j) {
            d->visualIndices.insert(j, j);
            d->logicalIndices.insert(j, j);
        }
    }

    // insert sections into hiddenSectionSize
    QHash<int, int> newHiddenSectionSize; // from logical index to section size
    for (QHash<int, int>::const_iterator it = d->hiddenSectionSize.cbegin(),
         end = d->hiddenSectionSize.cend(); it != end; ++it) {
        const int oldIndex = it.key();
        const int newIndex = (oldIndex < logicalFirst) ? oldIndex : oldIndex + insertCount;
        newHiddenSectionSize[newIndex] = it.value();
    }
    d->hiddenSectionSize.swap(newHiddenSectionSize);

    d->doDelayedResizeSections();
    emit sectionCountChanged(oldCount, count());

    if (lastSectionActualChange)
        d->maybeRestorePrevLastSectionAndStretchLast();

    // if the new sections were not updated by resizing, we need to update now
    if (!d->hasAutoResizeSections())
        d->viewport->update();
}

/*!
    This slot is called when sections are removed from the \a parent.
    \a logicalFirst and \a logicalLast signify where the sections were removed.

    If only one section is removed, \a logicalFirst and \a logicalLast will
    be the same.
*/

void QHeaderView::sectionsAboutToBeRemoved(const QModelIndex &parent,
                                           int logicalFirst, int logicalLast)
{
    Q_UNUSED(parent);
    Q_UNUSED(logicalFirst);
    Q_UNUSED(logicalLast);
}

void QHeaderViewPrivate::updateHiddenSections(int logicalFirst, int logicalLast)
{
    Q_Q(QHeaderView);
    const int changeCount = logicalLast - logicalFirst + 1;

    // remove sections from hiddenSectionSize
    QHash<int, int> newHiddenSectionSize; // from logical index to section size
    for (int i = 0; i < logicalFirst; ++i)
        if (q->isSectionHidden(i))
            newHiddenSectionSize[i] = hiddenSectionSize[i];
    for (int j = logicalLast + 1; j < sectionCount(); ++j)
        if (q->isSectionHidden(j))
            newHiddenSectionSize[j - changeCount] = hiddenSectionSize[j];
    hiddenSectionSize = newHiddenSectionSize;
}

void QHeaderViewPrivate::_q_sectionsRemoved(const QModelIndex &parent,
                                            int logicalFirst, int logicalLast)
{
    Q_Q(QHeaderView);
    if (parent != root)
        return; // we only handle changes in the root level
    if (qMin(logicalFirst, logicalLast) < 0
        || qMax(logicalLast, logicalFirst) >= sectionCount())
        return;
    int oldCount = q->count();
    int changeCount = logicalLast - logicalFirst + 1;

    if (state == QHeaderViewPrivate::ResizeSection)
        preventCursorChangeInSetOffset = true;

    updateHiddenSections(logicalFirst, logicalLast);

    if (visualIndices.isEmpty() && logicalIndices.isEmpty()) {
        //Q_ASSERT(headerSectionCount() == sectionCount);
        removeSectionsFromSectionItems(logicalFirst, logicalLast);
    } else {
        if (logicalFirst == logicalLast) { // Remove just one index.
            int l = logicalFirst;
            int visual = visualIndices.at(l);
            Q_ASSERT(sectionCount() == logicalIndices.size());
            for (int v = 0; v < sectionCount(); ++v) {
                if (v > visual) {
                    int logical = logicalIndices.at(v);
                    --(visualIndices[logical]);
                }
                if (logicalIndex(v) > l) // no need to move the positions before l
                    --(logicalIndices[v]);
            }
            logicalIndices.remove(visual);
            visualIndices.remove(l);
            //Q_ASSERT(headerSectionCount() == sectionCount);
            removeSectionsFromSectionItems(visual, visual);
        } else {
            sectionStartposRecalc = true; // We will need to recalc positions after removing items
            for (int u = 0; u < sectionItems.size(); ++u)  // Store section info
                sectionItems.at(u).tmpLogIdx = logicalIndices.at(u);
            for (int v = sectionItems.size() - 1; v >= 0; --v) {  // Remove the sections
                if (logicalFirst <= sectionItems.at(v).tmpLogIdx && sectionItems.at(v).tmpLogIdx <= logicalLast)
                    removeSectionsFromSectionItems(v, v);
            }
            visualIndices.resize(sectionItems.size());
            logicalIndices.resize(sectionItems.size());
            int* visual_data = visualIndices.data();
            int* logical_data = logicalIndices.data();
            for (int w = 0; w < sectionItems.size(); ++w) { // Restore visual and logical indexes
                int logindex = sectionItems.at(w).tmpLogIdx;
                if (logindex > logicalFirst)
                    logindex -= changeCount;
                visual_data[logindex] = w;
                logical_data[w] = logindex;
            }
        }
        // ### handle sectionSelection (sectionHidden is handled by updateHiddenSections)
    }

    // update sorting column
    if (sortIndicatorSection >= logicalFirst) {
        if (sortIndicatorSection <= logicalLast)
            sortIndicatorSection = -1;
        else
            sortIndicatorSection -= changeCount;
    }

    // if we only have the last section (the "end" position) left, the header is empty
    if (sectionCount() <= 0)
        clear();
    invalidateCachedSizeHint();
    emit q->sectionCountChanged(oldCount, q->count());

    if (q->stretchLastSection()) {
        const bool lastSectionRemoved = lastSectionLogicalIdx >= logicalFirst && lastSectionLogicalIdx <= logicalLast;
        if (lastSectionRemoved)
            setNewLastSection(lastVisibleVisualIndex());
        else
            lastSectionLogicalIdx = logicalIndex(lastVisibleVisualIndex()); // Just update the last log index.
        doDelayedResizeSections();
    }

    viewport->update();
}

void QHeaderViewPrivate::_q_sectionsAboutToBeMoved(const QModelIndex &sourceParent, int logicalStart, int logicalEnd, const QModelIndex &destinationParent, int logicalDestination)
{
    if (sourceParent != root || destinationParent != root)
        return; // we only handle changes in the root level
    Q_UNUSED(logicalStart);
    Q_UNUSED(logicalEnd);
    Q_UNUSED(logicalDestination);
    _q_sectionsAboutToBeChanged();
}

void QHeaderViewPrivate::_q_sectionsMoved(const QModelIndex &sourceParent, int logicalStart, int logicalEnd, const QModelIndex &destinationParent, int logicalDestination)
{
    if (sourceParent != root || destinationParent != root)
        return; // we only handle changes in the root level
    Q_UNUSED(logicalStart);
    Q_UNUSED(logicalEnd);
    Q_UNUSED(logicalDestination);
    _q_sectionsChanged();
}

void QHeaderViewPrivate::_q_sectionsAboutToBeChanged(const QList<QPersistentModelIndex> &,
                                                     QAbstractItemModel::LayoutChangeHint hint)
{
    if ((hint == QAbstractItemModel::VerticalSortHint && orientation == Qt::Horizontal) ||
        (hint == QAbstractItemModel::HorizontalSortHint && orientation == Qt::Vertical))
        return;

    //if there is no row/column we can't have mapping for columns
    //because no QModelIndex in the model would be valid
    // ### this is far from being bullet-proof and we would need a real system to
    // ### map columns or rows persistently
    if ((orientation == Qt::Horizontal && model->rowCount(root) == 0)
        || model->columnCount(root) == 0)
        return;

    layoutChangePersistentSections.clear();
    layoutChangePersistentSections.reserve(std::min(10, int(sectionItems.size())));
    // after layoutChanged another section can be last stretched section
    if (stretchLastSection && lastSectionLogicalIdx >= 0 && lastSectionLogicalIdx < sectionItems.size()) {
        const int visual = visualIndex(lastSectionLogicalIdx);
        if (visual >= 0 && visual < sectionItems.size()) {
            auto &itemRef = sectionItems[visual];
            if (itemRef.size != lastSectionSize) {
                length += lastSectionSize - itemRef.size;
                itemRef.size = lastSectionSize;
            }
        }
    }
    for (int i = 0; i < sectionItems.size(); ++i) {
        auto s = sectionItems.at(i);
        // only add if the section is not default and not visually moved
        if (s.size == defaultSectionSize && !s.isHidden && s.resizeMode == globalResizeMode)
            continue;

        const int logical = logicalIndex(i);
        if (s.isHidden)
            s.size = hiddenSectionSize.value(logical);

        // ### note that we are using column or row 0
        layoutChangePersistentSections.append({orientation == Qt::Horizontal
                                                  ? model->index(0, logical, root)
                                                  : model->index(logical, 0, root),
                                              s});
    }
}

void QHeaderViewPrivate::_q_sectionsChanged(const QList<QPersistentModelIndex> &,
                                            QAbstractItemModel::LayoutChangeHint hint)
{
    if ((hint == QAbstractItemModel::VerticalSortHint && orientation == Qt::Horizontal) ||
        (hint == QAbstractItemModel::HorizontalSortHint && orientation == Qt::Vertical))
        return;

    Q_Q(QHeaderView);
    viewport->update();

    const auto oldPersistentSections = layoutChangePersistentSections;
    layoutChangePersistentSections.clear();

    const int newCount = modelSectionCount();
    const int oldCount = sectionItems.size();
    if (newCount == 0) {
        clear();
        if (oldCount != 0)
            emit q->sectionCountChanged(oldCount, 0);
        return;
    }

    bool hasPersistantIndexes = false;
    for (const auto &item : oldPersistentSections) {
        if (item.index.isValid()) {
            hasPersistantIndexes = true;
            break;
        }
    }

    // Though far from perfect we here try to retain earlier/existing behavior
    // ### See QHeaderViewPrivate::_q_layoutAboutToBeChanged()
    // When we don't have valid hasPersistantIndexes it can be due to
    // - all sections are default sections
    // - the row/column 0 which is used for persistent indexes is gone
    // - all non-default sections were removed
    // case one is trivial, in case two we assume nothing else changed (it's the best
    // guess we can do - everything else can not be handled correctly for now)
    // case three can not be handled correctly with layoutChanged - removeSections
    // should be used instead for this
    if (!hasPersistantIndexes) {
        if (oldCount != newCount)
            q->initializeSections();
        return;
    }

    // adjust section size
    if (newCount != oldCount) {
        const int min = qBound(0, oldCount, newCount - 1);
        q->initializeSections(min, newCount - 1);
    }
    // reset sections
    sectionItems.fill(SectionItem(defaultSectionSize, globalResizeMode), newCount);

    // all hidden sections are in oldPersistentSections
    hiddenSectionSize.clear();

    for (const auto &item : oldPersistentSections) {
        const auto &index = item.index;
        if (!index.isValid())
            continue;

        const int newLogicalIndex = (orientation == Qt::Horizontal
                                     ? index.column()
                                     : index.row());
        // the new visualIndices are already adjusted / reset by initializeSections()
        const int newVisualIndex = visualIndex(newLogicalIndex);
        if (newVisualIndex < sectionItems.size()) {
            auto &newSection = sectionItems[newVisualIndex];
            newSection = item.section;

            if (newSection.isHidden) {
                // otherwise setSectionHidden will return without doing anything
                newSection.isHidden = false;
                q->setSectionHidden(newLogicalIndex, true);
            }
        }
    }

    recalcSectionStartPos();
    length = headerLength();

    if (stretchLastSection) {
        // force rebuild of stretched section later on
        lastSectionLogicalIdx = -1;
        maybeRestorePrevLastSectionAndStretchLast();
    }
}

/*!
  \internal
*/

void QHeaderView::initializeSections()
{
    Q_D(QHeaderView);
    const int oldCount = d->sectionCount();
    const int newCount = d->modelSectionCount();
    if (newCount <= 0) {
        d->clear();
        emit sectionCountChanged(oldCount, 0);
    } else if (newCount != oldCount) {
        const int min = qBound(0, oldCount, newCount - 1);
        initializeSections(min, newCount - 1);
        if (stretchLastSection())   // we've already gotten the size hint
            d->maybeRestorePrevLastSectionAndStretchLast();

        // make sure we update the hidden sections
        // simulate remove from newCount to oldCount
        if (newCount < oldCount)
            d->updateHiddenSections(newCount, oldCount);
    }
}

/*!
    \internal
*/

void QHeaderView::initializeSections(int start, int end)
{
    Q_D(QHeaderView);

    Q_ASSERT(start >= 0);
    Q_ASSERT(end >= 0);

    d->invalidateCachedSizeHint();
    int oldCount = d->sectionCount();

    if (end + 1 < d->sectionCount()) {
        int newCount = end + 1;
        d->removeSectionsFromSectionItems(newCount, d->sectionCount() - 1);
        if (!d->hiddenSectionSize.isEmpty()) {
            if (oldCount - newCount > d->hiddenSectionSize.size()) {
                for (int i = end + 1; i < d->sectionCount(); ++i)
                    d->hiddenSectionSize.remove(i);
            } else {
                QHash<int, int>::iterator it = d->hiddenSectionSize.begin();
                while (it != d->hiddenSectionSize.end()) {
                    if (it.key() > end)
                        it = d->hiddenSectionSize.erase(it);
                    else
                        ++it;
                }
            }
        }
    }

    int newSectionCount = end + 1;

    if (!d->logicalIndices.isEmpty()) {
        if (oldCount <= newSectionCount) {
            d->logicalIndices.resize(newSectionCount);
            d->visualIndices.resize(newSectionCount);
            for (int i = oldCount; i < newSectionCount; ++i) {
                d->logicalIndices[i] = i;
                d->visualIndices[i] = i;
            }
        } else {
            int j = 0;
            for (int i = 0; i < oldCount; ++i) {
                int v = d->logicalIndices.at(i);
                if (v < newSectionCount) {
                    d->logicalIndices[j] = v;
                    d->visualIndices[v] = j;
                    j++;
                }
            }
            d->logicalIndices.resize(newSectionCount);
            d->visualIndices.resize(newSectionCount);
        }
    }

    if (d->globalResizeMode == Stretch)
        d->stretchSections = newSectionCount;
    else if (d->globalResizeMode == ResizeToContents)
         d->contentsSections = newSectionCount;

    if (newSectionCount > oldCount)
        d->createSectionItems(start, end, d->defaultSectionSize, d->globalResizeMode);
    //Q_ASSERT(d->headerLength() == d->length);

    if (d->sectionCount() != oldCount)
        emit sectionCountChanged(oldCount,  d->sectionCount());
    d->viewport->update();
}

/*!
  \reimp
*/

void QHeaderView::currentChanged(const QModelIndex &current, const QModelIndex &old)
{
    Q_D(QHeaderView);

    if (d->orientation == Qt::Horizontal && current.column() != old.column()) {
        if (old.isValid() && old.parent() == d->root)
            d->viewport->update(QRect(sectionViewportPosition(old.column()), 0,
                                    sectionSize(old.column()), d->viewport->height()));
        if (current.isValid() && current.parent() == d->root)
            d->viewport->update(QRect(sectionViewportPosition(current.column()), 0,
                                    sectionSize(current.column()), d->viewport->height()));
    } else if (d->orientation == Qt::Vertical && current.row() != old.row()) {
        if (old.isValid() && old.parent() == d->root)
            d->viewport->update(QRect(0, sectionViewportPosition(old.row()),
                                    d->viewport->width(), sectionSize(old.row())));
        if (current.isValid() && current.parent() == d->root)
            d->viewport->update(QRect(0, sectionViewportPosition(current.row()),
                                    d->viewport->width(), sectionSize(current.row())));
    }
}


/*!
  \reimp
*/

bool QHeaderView::event(QEvent *e)
{
    Q_D(QHeaderView);
    switch (e->type()) {
    case QEvent::HoverEnter: {
        QHoverEvent *he = static_cast<QHoverEvent*>(e);
        d->hover = logicalIndexAt(he->position().toPoint());
        if (d->hover != -1)
            updateSection(d->hover);
        break; }
    case QEvent::Leave:
    case QEvent::HoverLeave: {
        if (d->hover != -1)
            updateSection(d->hover);
        d->hover = -1;
        break; }
    case QEvent::HoverMove: {
        QHoverEvent *he = static_cast<QHoverEvent*>(e);
        int oldHover = d->hover;
        d->hover = logicalIndexAt(he->position().toPoint());
        if (d->hover != oldHover) {
            if (oldHover != -1)
                updateSection(oldHover);
            if (d->hover != -1)
                updateSection(d->hover);
        }
        break; }
    case QEvent::Timer: {
        QTimerEvent *te = static_cast<QTimerEvent*>(e);
        if (te->timerId() == d->delayedResize.timerId()) {
            d->delayedResize.stop();
            resizeSections();
        }
        break; }
    case QEvent::StyleChange:
        if (!d->customDefaultSectionSize)
            d->updateDefaultSectionSizeFromStyle();
        break;
    default:
        break;
    }
    return QAbstractItemView::event(e);
}

/*!
  \reimp
*/

void QHeaderView::paintEvent(QPaintEvent *e)
{
    Q_D(QHeaderView);

    if (count() == 0)
        return;

    QPainter painter(d->viewport);
    const QPoint offset = d->scrollDelayOffset;
    QRect translatedEventRect = e->rect();
    translatedEventRect.translate(offset);

    int start = -1;
    int end = -1;
    if (d->orientation == Qt::Horizontal) {
        start = visualIndexAt(translatedEventRect.left());
        end = visualIndexAt(translatedEventRect.right());
    } else {
        start = visualIndexAt(translatedEventRect.top());
        end = visualIndexAt(translatedEventRect.bottom());
    }

    if (d->reverse()) {
        start = (start == -1 ? count() - 1 : start);
        end = (end == -1 ? 0 : end);
    } else {
        start = (start == -1 ? 0 : start);
        end = (end == -1 ? count() - 1 : end);
    }

    int tmp = start;
    start = qMin(start, end);
    end = qMax(tmp, end);

    d->prepareSectionSelected(); // clear and resize the bit array

    QRect currentSectionRect;
    const int width = d->viewport->width();
    const int height = d->viewport->height();
    const int rtlHorizontalOffset = d->reverse() ? 1 : 0;
    for (int i = start; i <= end; ++i) {
        if (d->isVisualIndexHidden(i))
            continue;
        painter.save();
        const int logical = logicalIndex(i);
        if (d->orientation == Qt::Horizontal) {
            currentSectionRect.setRect(sectionViewportPosition(logical) + rtlHorizontalOffset,
                                       0, sectionSize(logical), height);
        } else {
            currentSectionRect.setRect(0, sectionViewportPosition(logical),
                                       width, sectionSize(logical));
        }
        currentSectionRect.translate(offset);

        QVariant variant = d->model->headerData(logical, d->orientation,
                                                Qt::FontRole);
        if (variant.isValid() && variant.canConvert<QFont>()) {
            QFont sectionFont = qvariant_cast<QFont>(variant);
            painter.setFont(sectionFont);
        }
        paintSection(&painter, currentSectionRect, logical);
        painter.restore();
    }

    QStyleOption opt;
    opt.initFrom(this);
    // Paint the area beyond where there are indexes
    if (d->reverse()) {
        opt.state |= QStyle::State_Horizontal;
        if (currentSectionRect.left() > translatedEventRect.left()) {
            opt.rect = QRect(translatedEventRect.left(), 0,
                             currentSectionRect.left() - translatedEventRect.left(), height);
            style()->drawControl(QStyle::CE_HeaderEmptyArea, &opt, &painter, this);
        }
    } else if (currentSectionRect.right() < translatedEventRect.right()) {
        // paint to the right
        opt.state |= QStyle::State_Horizontal;
        opt.rect = QRect(currentSectionRect.right() + 1, 0,
                         translatedEventRect.right() - currentSectionRect.right(), height);
        style()->drawControl(QStyle::CE_HeaderEmptyArea, &opt, &painter, this);
    } else if (currentSectionRect.bottom() < translatedEventRect.bottom()) {
        // paint the bottom section
        opt.state &= ~QStyle::State_Horizontal;
        opt.rect = QRect(0, currentSectionRect.bottom() + 1,
                         width, height - currentSectionRect.bottom() - 1);
        style()->drawControl(QStyle::CE_HeaderEmptyArea, &opt, &painter, this);
    }

#if 0
    // ### visualize sections
    for (int a = 0, i = 0; i < d->sectionItems.count(); ++i) {
        QColor color((i & 4 ? 255 : 0), (i & 2 ? 255 : 0), (i & 1 ? 255 : 0));
        if (d->orientation == Qt::Horizontal)
            painter.fillRect(a - d->offset, 0, d->sectionItems.at(i).size, 4, color);
        else
            painter.fillRect(0, a - d->offset, 4, d->sectionItems.at(i).size, color);
        a += d->sectionItems.at(i).size;
    }

#endif
}

/*!
  \reimp
*/

void QHeaderView::mousePressEvent(QMouseEvent *e)
{
    Q_D(QHeaderView);
    if (d->state != QHeaderViewPrivate::NoState || e->button() != Qt::LeftButton)
        return;
    int pos = d->orientation == Qt::Horizontal ? e->position().toPoint().x() : e->position().toPoint().y();
    int handle = d->sectionHandleAt(pos);
    d->originalSize = -1; // clear the stored original size
    if (handle == -1) {
        d->firstPressed = d->pressed = logicalIndexAt(pos);
        if (d->clickableSections)
            emit sectionPressed(d->pressed);

        bool acceptMoveSection = d->movableSections;
        if (acceptMoveSection && d->pressed == 0 && !d->allowUserMoveOfSection0)
            acceptMoveSection = false; // Do not allow moving the tree nod

        if (acceptMoveSection) {
            d->target = -1;
            d->section = d->pressed;
            if (d->section == -1)
                return;
            d->state = QHeaderViewPrivate::MoveSection;
            d->setupSectionIndicator(d->section, pos);
        } else if (d->clickableSections && d->pressed != -1) {
            updateSection(d->pressed);
            d->state = QHeaderViewPrivate::SelectSections;
        }
    } else if (sectionResizeMode(handle) == Interactive) {
        d->originalSize = sectionSize(handle);
        d->state = QHeaderViewPrivate::ResizeSection;
        d->section = handle;
        d->preventCursorChangeInSetOffset = false;
    }

    d->firstPos = pos;
    d->lastPos = pos;

    d->clearCascadingSections();
}

/*!
  \reimp
*/

void QHeaderView::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QHeaderView);
    const int pos = d->orientation == Qt::Horizontal ? e->position().toPoint().x() : e->position().toPoint().y();
    if (pos < 0 && d->state != QHeaderViewPrivate::SelectSections)
        return;
    if (e->buttons() == Qt::NoButton) {
        // Under Cocoa, when the mouse button is released, may include an extra
        // simulated mouse moved event. The state of the buttons when this event
        // is generated is already "no button" and the code below gets executed
        // just before the mouseReleaseEvent and resets the state. This prevents
        // column dragging from working. So this code is disabled under Cocoa.
        d->state = QHeaderViewPrivate::NoState;
        d->firstPressed = d->pressed = -1;
    }
    switch (d->state) {
        case QHeaderViewPrivate::ResizeSection: {
            Q_ASSERT(d->originalSize != -1);
            if (d->cascadingResizing) {
                int delta = d->reverse() ? d->lastPos - pos : pos - d->lastPos;
                int visual = visualIndex(d->section);
                d->cascadingResize(visual, d->headerSectionSize(visual) + delta);
            } else {
                int delta = d->reverse() ? d->firstPos - pos : pos - d->firstPos;
                int newsize = qBound(minimumSectionSize(), d->originalSize + delta, maximumSectionSize());
                resizeSection(d->section, newsize);
            }
            d->lastPos = pos;
            return;
        }
        case QHeaderViewPrivate::MoveSection: {
            if (d->shouldAutoScroll(e->position().toPoint())) {
                d->draggedPosition = e->pos();
                d->startAutoScroll();
            }
            if (qAbs(pos - d->firstPos) >= QApplication::startDragDistance()
#if QT_CONFIG(label)
                || !d->sectionIndicator->isHidden()
#endif
                ) {
                int visual = visualIndexAt(pos);
                if (visual == -1)
                    return;
                if (visual == 0 && logicalIndex(0) == 0 && !d->allowUserMoveOfSection0)
                    return;

                const int posThreshold = d->headerSectionPosition(visual) - d->offset + d->headerSectionSize(visual) / 2;
                const int checkPos = d->reverse() ? d->viewport->width() - pos : pos;
                int moving = visualIndex(d->section);
                int oldTarget = d->target;
                if (visual < moving) {
                    if (checkPos < posThreshold)
                        d->target = d->logicalIndex(visual);
                    else
                        d->target = d->logicalIndex(visual + 1);
                } else if (visual > moving) {
                    if (checkPos > posThreshold)
                        d->target = d->logicalIndex(visual);
                    else
                        d->target = d->logicalIndex(visual - 1);
                } else {
                    d->target = d->section;
                }
                if (oldTarget != d->target || oldTarget == -1)
                    d->updateSectionsBeforeAfter(d->target);
                d->updateSectionIndicator(d->section, pos);
            }
            return;
        }
        case QHeaderViewPrivate::SelectSections: {
            int logical = logicalIndexAt(qMax(-d->offset, pos));
            if (logical == -1 && pos > 0)
                logical = logicalIndex(d->lastVisibleVisualIndex());
            if (logical == d->pressed)
                return; // nothing to do
            else if (d->pressed != -1)
                updateSection(d->pressed);
            d->pressed = logical;
            if (d->clickableSections && logical != -1) {
                emit sectionEntered(d->pressed);
                updateSection(d->pressed);
            }
            return;
        }
        case QHeaderViewPrivate::NoState: {
#ifndef QT_NO_CURSOR
            int handle = d->sectionHandleAt(pos);
            bool hasCursor = testAttribute(Qt::WA_SetCursor);
            if (handle != -1 && (sectionResizeMode(handle) == Interactive)) {
                if (!hasCursor)
                    setCursor(d->orientation == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);
            } else {
                if (hasCursor)
                    unsetCursor();
#ifndef QT_NO_STATUSTIP
                int logical = logicalIndexAt(pos);
                QString statusTip;
                if (logical != -1)
                    statusTip = d->model->headerData(logical, d->orientation, Qt::StatusTipRole).toString();
                if (d->shouldClearStatusTip || !statusTip.isEmpty()) {
                    QStatusTipEvent tip(statusTip);
                    QCoreApplication::sendEvent(d->parent ? d->parent : this, &tip);
                    d->shouldClearStatusTip = !statusTip.isEmpty();
                }
#endif // !QT_NO_STATUSTIP
            }
#endif
            return;
        }
        default:
            break;
    }
}

/*!
  \reimp
*/

void QHeaderView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QHeaderView);
    int pos = d->orientation == Qt::Horizontal ? e->position().toPoint().x() : e->position().toPoint().y();
    switch (d->state) {
    case QHeaderViewPrivate::MoveSection:
        if (true
#if QT_CONFIG(label)
            && !d->sectionIndicator->isHidden()
#endif
      ) { // moving
            int from = visualIndex(d->section);
            Q_ASSERT(from != -1);
            int to = visualIndex(d->target);
            Q_ASSERT(to != -1);
            moveSection(from, to);
            d->section = d->target = -1;
            d->updateSectionIndicator(d->section, pos);
            if (from == to)
                d->updateSectionsBeforeAfter(from);
            break;
        } // not moving
        Q_FALLTHROUGH();
    case QHeaderViewPrivate::SelectSections:
        if (!d->clickableSections) {
            int section = logicalIndexAt(pos);
            updateSection(section);
        }
        Q_FALLTHROUGH();
    case QHeaderViewPrivate::NoState:
        if (d->clickableSections) {
            int section = logicalIndexAt(pos);
            if (section != -1 && section == d->firstPressed) {
                QRect firstPressedSectionRect;
                switch (d->orientation) {
                case Qt::Horizontal:
                    firstPressedSectionRect.setRect(sectionViewportPosition(d->firstPressed),
                                                    0,
                                                    sectionSize(d->firstPressed),
                                                    d->viewport->height());
                    break;
                case Qt::Vertical:
                    firstPressedSectionRect.setRect(0,
                                                    sectionViewportPosition(d->firstPressed),
                                                    d->viewport->width(),
                                                    sectionSize(d->firstPressed));
                    break;
                };

                if (firstPressedSectionRect.contains(e->position().toPoint())) {
                    d->flipSortIndicator(section);
                    emit sectionClicked(section);
                }
            }
            if (d->pressed != -1)
                updateSection(d->pressed);
        }
        break;
    case QHeaderViewPrivate::ResizeSection:
        d->originalSize = -1;
        d->clearCascadingSections();
        break;
    default:
        break;
    }
    d->state = QHeaderViewPrivate::NoState;
    d->firstPressed = d->pressed = -1;
}

/*!
  \reimp
*/

void QHeaderView::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_D(QHeaderView);
    int pos = d->orientation == Qt::Horizontal ? e->position().toPoint().x() : e->position().toPoint().y();
    int handle = d->sectionHandleAt(pos);
    if (handle > -1 && sectionResizeMode(handle) == Interactive) {
        emit sectionHandleDoubleClicked(handle);
#ifndef QT_NO_CURSOR
        Qt::CursorShape splitCursor = (d->orientation == Qt::Horizontal)
                                      ? Qt::SplitHCursor : Qt::SplitVCursor;
        if (cursor().shape() == splitCursor) {
            // signal handlers may have changed the section size
            handle = d->sectionHandleAt(pos);
            if (!(handle > -1 && sectionResizeMode(handle) == Interactive))
                setCursor(Qt::ArrowCursor);
        }
#endif
    } else {
        emit sectionDoubleClicked(logicalIndexAt(e->position().toPoint()));
    }
}

/*!
  \reimp
*/

bool QHeaderView::viewportEvent(QEvent *e)
{
    Q_D(QHeaderView);
    switch (e->type()) {
#if QT_CONFIG(tooltip)
    case QEvent::ToolTip: {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int logical = logicalIndexAt(he->pos());
        if (logical != -1) {
            QVariant variant = d->model->headerData(logical, d->orientation, Qt::ToolTipRole);
            if (variant.isValid()) {
                QToolTip::showText(he->globalPos(), variant.toString(), this);
                return true;
            }
        }
        break; }
#endif
#if QT_CONFIG(whatsthis)
    case QEvent::QueryWhatsThis: {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int logical = logicalIndexAt(he->pos());
        if (logical != -1
            && d->model->headerData(logical, d->orientation, Qt::WhatsThisRole).isValid())
            return true;
        break; }
    case QEvent::WhatsThis: {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int logical = logicalIndexAt(he->pos());
        if (logical != -1) {
             QVariant whatsthis = d->model->headerData(logical, d->orientation,
                                                      Qt::WhatsThisRole);
             if (whatsthis.isValid()) {
                 QWhatsThis::showText(he->globalPos(), whatsthis.toString(), this);
                 return true;
             }
        }
        break; }
#endif // QT_CONFIG(whatsthis)
#if QT_CONFIG(statustip)
    case QEvent::StatusTip: {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        int logical = logicalIndexAt(he->pos());
        if (logical != -1) {
            QString statustip = d->model->headerData(logical, d->orientation,
                                                    Qt::StatusTipRole).toString();
            if (!statustip.isEmpty())
                setStatusTip(statustip);
        }
        return true; }
#endif // QT_CONFIG(statustip)
    case QEvent::Resize:
    case QEvent::FontChange:
    case QEvent::StyleChange:
        d->invalidateCachedSizeHint();
        Q_FALLTHROUGH();
    case QEvent::Hide:
    case QEvent::Show: {
        QAbstractScrollArea *parent = qobject_cast<QAbstractScrollArea *>(parentWidget());
        if (parent && parent->isVisible()) // Only resize if we have a visible parent
            resizeSections();
        emit geometriesChanged();
        break;}
    case QEvent::ContextMenu: {
        d->state = QHeaderViewPrivate::NoState;
        d->pressed = d->section = d->target = -1;
        d->updateSectionIndicator(d->section, -1);
        break; }
    case QEvent::Wheel: {
        QAbstractScrollArea *asa = qobject_cast<QAbstractScrollArea *>(parentWidget());
        if (asa)
            return QCoreApplication::sendEvent(asa->viewport(), e);
        break; }
    default:
        break;
    }
    return QAbstractItemView::viewportEvent(e);
}

/*!
    \fn void QHeaderView::initStyleOptionForIndex(QStyleOptionHeader *option, int logicalIndex) const
    \since 6.0

    Initializes the style \a option from the specified \a logicalIndex.
    This function is called by the default implementation of paintSection after
    initStyleOption has been called.

    \sa paintSection(), initStyleOption()
*/

void QHeaderView::initStyleOptionForIndex(QStyleOptionHeader *option, int logicalIndex) const
{
    Q_D(const QHeaderView);

    if (!option)
        return;
    QStyleOptionHeader &opt = *option;
    QStyleOptionHeaderV2 *optV2 = qstyleoption_cast<QStyleOptionHeaderV2*>(option);

    QStyle::State state = QStyle::State_None;
    if (window()->isActiveWindow())
        state |= QStyle::State_Active;
    if (d->clickableSections) {
        if (logicalIndex == d->hover)
            state |= QStyle::State_MouseOver;
        if (logicalIndex == d->pressed)
            state |= QStyle::State_Sunken;
        else if (d->highlightSelected) {
            if (d->sectionIntersectsSelection(logicalIndex))
                state |= QStyle::State_On;
            if (d->isSectionSelected(logicalIndex))
                state |= QStyle::State_Sunken;
        }
    }
    if (isSortIndicatorShown() && sortIndicatorSection() == logicalIndex)
        opt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder)
                            ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;

    // setup the style options structure
    QVariant textAlignment = d->model->headerData(logicalIndex, d->orientation,
                                                  Qt::TextAlignmentRole);
    opt.section = logicalIndex;
    opt.state |= state;
    opt.textAlignment = textAlignment.isValid()
                        ? QtPrivate::legacyFlagValueFromModelData<Qt::Alignment>(textAlignment)
                        : d->defaultAlignment;

    opt.text = d->model->headerData(logicalIndex, d->orientation,
                                    Qt::DisplayRole).toString();

    const QVariant variant = d->model->headerData(logicalIndex, d->orientation,
                                                  Qt::DecorationRole);
    opt.icon = qvariant_cast<QIcon>(variant);
    if (opt.icon.isNull())
        opt.icon = qvariant_cast<QPixmap>(variant);

    QVariant var = d->model->headerData(logicalIndex, d->orientation,
                                        Qt::FontRole);
    if (var.isValid() && var.canConvert<QFont>())
        opt.fontMetrics = QFontMetrics(qvariant_cast<QFont>(var));

    QVariant foregroundBrush = d->model->headerData(logicalIndex, d->orientation,
                                                    Qt::ForegroundRole);
    if (foregroundBrush.canConvert<QBrush>())
        opt.palette.setBrush(QPalette::ButtonText, qvariant_cast<QBrush>(foregroundBrush));

    QVariant backgroundBrush = d->model->headerData(logicalIndex, d->orientation,
                                                    Qt::BackgroundRole);
    if (backgroundBrush.canConvert<QBrush>()) {
        opt.palette.setBrush(QPalette::Button, qvariant_cast<QBrush>(backgroundBrush));
        opt.palette.setBrush(QPalette::Window, qvariant_cast<QBrush>(backgroundBrush));
    }

    // the section position
    int visual = visualIndex(logicalIndex);
    Q_ASSERT(visual != -1);
    bool first = d->isFirstVisibleSection(visual);
    bool last = d->isLastVisibleSection(visual);
    if (first && last)
        opt.position = QStyleOptionHeader::OnlyOneSection;
    else if (first)
        opt.position = d->reverse() ? QStyleOptionHeader::End : QStyleOptionHeader::Beginning;
    else if (last)
        opt.position = d->reverse() ? QStyleOptionHeader::Beginning : QStyleOptionHeader::End;
    else
        opt.position = QStyleOptionHeader::Middle;
    opt.orientation = d->orientation;
    // the selected position
    bool previousSelected = d->isSectionSelected(this->logicalIndex(visual - 1));
    bool nextSelected =  d->isSectionSelected(this->logicalIndex(visual + 1));
    if (previousSelected && nextSelected)
        opt.selectedPosition = QStyleOptionHeader::NextAndPreviousAreSelected;
    else if (previousSelected)
        opt.selectedPosition = QStyleOptionHeader::PreviousIsSelected;
    else if (nextSelected)
        opt.selectedPosition = QStyleOptionHeader::NextIsSelected;
    else
        opt.selectedPosition = QStyleOptionHeader::NotAdjacent;
    if (optV2)
        optV2->isSectionDragTarget = d->target == logicalIndex;
}

/*!
    Paints the section specified by the given \a logicalIndex, using the given
    \a painter and \a rect.

    Normally, you do not have to call this function.
*/

void QHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    if (!rect.isValid())
        return;

    QStyleOptionHeaderV2 opt;
    QPointF oldBO = painter->brushOrigin();

    initStyleOption(&opt);

    QBrush oBrushButton = opt.palette.brush(QPalette::Button);
    QBrush oBrushWindow = opt.palette.brush(QPalette::Window);

    initStyleOptionForIndex(&opt, logicalIndex);
    // We set rect here. If it needs to be changed it can be changed by overriding this function
    opt.rect = rect;

    QBrush nBrushButton = opt.palette.brush(QPalette::Button);
    QBrush nBrushWindow = opt.palette.brush(QPalette::Window);

    // If relevant brushes are not the same as from the regular widgets we set the brush origin
    if (oBrushButton != nBrushButton || oBrushWindow != nBrushWindow) {
        painter->setBrushOrigin(opt.rect.topLeft());
    }

    // draw the section.
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
    painter->setBrushOrigin(oldBO);
}

/*!
    Returns the size of the contents of the section specified by the given
    \a logicalIndex.

    \sa defaultSectionSize()
*/

QSize QHeaderView::sectionSizeFromContents(int logicalIndex) const
{
    Q_D(const QHeaderView);
    Q_ASSERT(logicalIndex >= 0);

    ensurePolished();

    // use SizeHintRole
    QVariant variant = d->model->headerData(logicalIndex, d->orientation, Qt::SizeHintRole);
    if (variant.isValid())
        return qvariant_cast<QSize>(variant);

    // otherwise use the contents
    QStyleOptionHeaderV2 opt;
    initStyleOption(&opt);
    opt.section = logicalIndex;
    QVariant var = d->model->headerData(logicalIndex, d->orientation,
                                            Qt::FontRole);
    QFont fnt;
    if (var.isValid() && var.canConvert<QFont>())
        fnt = qvariant_cast<QFont>(var);
    else
        fnt = font();
    fnt.setBold(true);
    opt.fontMetrics = QFontMetrics(fnt);
    opt.text = d->model->headerData(logicalIndex, d->orientation,
                                    Qt::DisplayRole).toString();
    variant = d->model->headerData(logicalIndex, d->orientation, Qt::DecorationRole);
    opt.icon = qvariant_cast<QIcon>(variant);
    if (opt.icon.isNull())
        opt.icon = qvariant_cast<QPixmap>(variant);
    if (isSortIndicatorShown())
        opt.sortIndicator = QStyleOptionHeader::SortDown;
    return style()->sizeFromContents(QStyle::CT_HeaderSection, &opt, QSize(), this);
}

/*!
    Returns the horizontal offset of the header. This is 0 for vertical
    headers.

    \sa offset()
*/

int QHeaderView::horizontalOffset() const
{
    Q_D(const QHeaderView);
    if (d->orientation == Qt::Horizontal)
        return d->offset;
    return 0;
}

/*!
    Returns the vertical offset of the header. This is 0 for horizontal
    headers.

    \sa offset()
*/

int QHeaderView::verticalOffset() const
{
    Q_D(const QHeaderView);
    if (d->orientation == Qt::Vertical)
        return d->offset;
    return 0;
}

/*!
    \reimp
    \internal
*/

void QHeaderView::updateGeometries()
{
    Q_D(QHeaderView);
    d->layoutChildren();
    if (d->hasAutoResizeSections())
        d->doDelayedResizeSections();
}

/*!
    \reimp
    \internal
*/

void QHeaderView::scrollContentsBy(int dx, int dy)
{
    Q_D(QHeaderView);
    d->scrollDirtyRegion(dx, dy);
}

/*!
    \reimp
    \internal
*/
void QHeaderView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                              const QList<int> &roles)
{
    Q_D(QHeaderView);
    if (!roles.isEmpty()) {
        const auto doesRoleAffectSize = [](int role) -> bool {
            switch (role) {
            case Qt::DisplayRole:
            case Qt::DecorationRole:
            case Qt::SizeHintRole:
            case Qt::FontRole:
                return true;
            default:
                // who knows what a subclass or custom style might do
                return role >= Qt::UserRole;
            }
        };
        if (std::none_of(roles.begin(), roles.end(), doesRoleAffectSize))
            return;
    }
    d->invalidateCachedSizeHint();
    if (d->hasAutoResizeSections()) {
        bool resizeRequired = d->globalResizeMode == ResizeToContents;
        int first = orientation() == Qt::Horizontal ? topLeft.column() : topLeft.row();
        int last = orientation() == Qt::Horizontal ? bottomRight.column() : bottomRight.row();
        for (int i = first; i <= last && !resizeRequired; ++i)
            resizeRequired = (sectionResizeMode(i) == ResizeToContents);
        if (resizeRequired)
            d->doDelayedResizeSections();
    }
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/
void QHeaderView::rowsInserted(const QModelIndex &, int, int)
{
    // do nothing
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

QRect QHeaderView::visualRect(const QModelIndex &) const
{
    return QRect();
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

void QHeaderView::scrollTo(const QModelIndex &, ScrollHint)
{
    // do nothing - the header only displays sections
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

QModelIndex QHeaderView::indexAt(const QPoint &) const
{
    return QModelIndex();
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

bool QHeaderView::isIndexHidden(const QModelIndex &) const
{
    return true; // the header view has no items, just sections
}

/*!
    \reimp
    \internal

    Empty implementation because the header doesn't show QModelIndex items.
*/

QModelIndex QHeaderView::moveCursor(CursorAction, Qt::KeyboardModifiers)
{
    return QModelIndex();
}

/*!
    \reimp

    Selects the items in the given \a rect according to the specified
    \a flags.

    The base class implementation does nothing.
*/

void QHeaderView::setSelection(const QRect&, QItemSelectionModel::SelectionFlags)
{
    // do nothing
}

/*!
    \internal
*/

QRegion QHeaderView::visualRegionForSelection(const QItemSelection &selection) const
{
    Q_D(const QHeaderView);
    const int max = d->modelSectionCount();

    if (d->orientation == Qt::Horizontal) {
        int logicalLeft = max;
        int logicalRight = 0;

        if (d->visualIndices.empty()) {
            // If no reordered sections, skip redundant visual-to-logical transformations
            for (const auto &r : selection) {
                if (r.parent().isValid() || !r.isValid())
                    continue; // we only know about toplevel items and we don't want invalid ranges
                if (r.left() < logicalLeft)
                    logicalLeft = r.left();
                if (r.right() > logicalRight)
                    logicalRight = r.right();
            }
        } else {
            int left = max;
            int right = 0;
            for (const auto &r : selection) {
                if (r.parent().isValid() || !r.isValid())
                    continue; // we only know about toplevel items and we don't want invalid ranges
                for (int k = r.left(); k <= r.right(); ++k) {
                    int visual = visualIndex(k);
                    if (visual == -1)   // in some cases users may change the selections
                        continue;       // before we have a chance to do the layout
                    if (visual < left)
                        left = visual;
                    if (visual > right)
                        right = visual;
                }
            }
            logicalLeft = logicalIndex(left);
            logicalRight = logicalIndex(right);
        }

        if (logicalLeft < 0  || logicalLeft >= count() ||
            logicalRight < 0 || logicalRight >= count())
            return QRegion();

        int leftPos = sectionViewportPosition(logicalLeft);
        int rightPos = sectionViewportPosition(logicalRight);
        rightPos += sectionSize(logicalRight);
        return QRect(leftPos, 0, rightPos - leftPos, height());
    }
    // orientation() == Qt::Vertical
    int logicalTop = max;
    int logicalBottom = 0;

    if (d->visualIndices.empty()) {
        // If no reordered sections, skip redundant visual-to-logical transformations
        for (const auto &r : selection) {
            if (r.parent().isValid() || !r.isValid())
                continue; // we only know about toplevel items and we don't want invalid ranges
            if (r.top() < logicalTop)
                logicalTop = r.top();
            if (r.bottom() > logicalBottom)
                logicalBottom = r.bottom();
        }
    } else {
        int top = max;
        int bottom = 0;

        for (const auto &r : selection) {
            if (r.parent().isValid() || !r.isValid())
                continue; // we only know about toplevel items and we don't want invalid ranges
            for (int k = r.top(); k <= r.bottom(); ++k) {
                int visual = visualIndex(k);
                if (visual == -1)   // in some cases users may change the selections
                    continue;       // before we have a chance to do the layout
                if (visual < top)
                    top = visual;
                if (visual > bottom)
                    bottom = visual;
            }
        }

        logicalTop = logicalIndex(top);
        logicalBottom = logicalIndex(bottom);
    }

    if (logicalTop < 0 || logicalTop >= count() ||
        logicalBottom < 0 || logicalBottom >= count())
        return QRegion();

    int topPos = sectionViewportPosition(logicalTop);
    int bottomPos = sectionViewportPosition(logicalBottom) + sectionSize(logicalBottom);

    return QRect(0, topPos, width(), bottomPos - topPos);
}


// private implementation

int QHeaderViewPrivate::sectionHandleAt(int position)
{
    Q_Q(QHeaderView);
    int visual = q->visualIndexAt(position);
    if (visual == -1)
        return -1;
    int log = logicalIndex(visual);
    int pos = q->sectionViewportPosition(log);
    int grip = q->style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, q);

    bool atLeft = position < pos + grip;
    bool atRight = (position > pos + q->sectionSize(log) - grip);
    if (reverse())
        qSwap(atLeft, atRight);

    if (atLeft) {
        //grip at the beginning of the section
        while(visual > -1) {
            int logical = q->logicalIndex(--visual);
            if (!q->isSectionHidden(logical))
                return logical;
        }
    } else if (atRight) {
        //grip at the end of the section
        return log;
    }
    return -1;
}

void QHeaderViewPrivate::setupSectionIndicator(int section, int position)
{
    Q_Q(QHeaderView);
#if QT_CONFIG(label)
    if (!sectionIndicator) {
        sectionIndicator = new QLabel(viewport);
    }
#endif

    int w, h;
    int p = q->sectionViewportPosition(section);
    if (orientation == Qt::Horizontal) {
        w = q->sectionSize(section);
        h = viewport->height();
    } else {
        w = viewport->width();
        h = q->sectionSize(section);
    }
#if QT_CONFIG(label)
    sectionIndicator->resize(w, h);
#endif

    const qreal pixmapDevicePixelRatio = q->devicePixelRatio();
    QPixmap pm(QSize(w, h) * pixmapDevicePixelRatio);
    pm.setDevicePixelRatio(pixmapDevicePixelRatio);
    pm.fill(QColor(0, 0, 0, 45));
    QRect rect(0, 0, w, h);

    QPainter painter(&pm);
    const QVariant variant = model->headerData(section, orientation,
                                               Qt::FontRole);
    if (variant.isValid() && variant.canConvert<QFont>()) {
        const QFont sectionFont = qvariant_cast<QFont>(variant);
        painter.setFont(sectionFont);
    } else {
        painter.setFont(q->font());
    }

    painter.setOpacity(0.75);
    q->paintSection(&painter, rect, section);
    painter.end();

#if QT_CONFIG(label)
    sectionIndicator->setPixmap(pm);
#endif
    sectionIndicatorOffset = position - qMax(p, 0);
}

void QHeaderViewPrivate::updateSectionIndicator(int section, int position)
{
#if QT_CONFIG(label)
    if (!sectionIndicator)
        return;

    if (section == -1 || target == -1) {
        sectionIndicator->hide();
        return;
    }

    if (orientation == Qt::Horizontal)
        sectionIndicator->move(position - sectionIndicatorOffset, 0);
    else
        sectionIndicator->move(0, position - sectionIndicatorOffset);

    sectionIndicator->show();
#endif
}

/*!
    Initialize \a option with the values from this QHeaderView. This method is
    useful for subclasses when they need a QStyleOptionHeader, but do not want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom(), initStyleOptionForIndex()
*/
void QHeaderView::initStyleOption(QStyleOptionHeader *option) const
{
    Q_D(const QHeaderView);
    option->initFrom(this);
    option->state = QStyle::State_None | QStyle::State_Raised;
    option->orientation = d->orientation;
    option->iconAlignment = Qt::AlignVCenter;
    if (d->orientation == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;
    if (isEnabled())
        option->state |= QStyle::State_Enabled;
    if (QStyleOptionHeaderV2 *optV2 = qstyleoption_cast<QStyleOptionHeaderV2 *>(option))
        optV2->textElideMode = d->textElideMode;

    option->section = 0;
}

void QHeaderView::initStyleOption(QStyleOptionFrame *option) const
{
    // The QFrame version is only here to avoid compiler warnings.
    // If invoked we just pass it on to the base class.
    QFrame::initStyleOption(option);
}

bool QHeaderViewPrivate::isSectionSelected(int section) const
{
    int i = section * 2;
    if (i < 0 || i >= sectionSelected.size())
        return false;
    if (sectionSelected.testBit(i)) // if the value was cached
        return sectionSelected.testBit(i + 1);
    bool s = false;
    if (orientation == Qt::Horizontal)
        s = isColumnSelected(section);
    else
        s = isRowSelected(section);
    sectionSelected.setBit(i + 1, s); // selection state
    sectionSelected.setBit(i, true); // cache state
    return s;
}

bool QHeaderViewPrivate::isFirstVisibleSection(int section) const
{
    if (sectionStartposRecalc)
        recalcSectionStartPos();
    const SectionItem &item = sectionItems.at(section);
    return item.size > 0 && item.calculated_startpos == 0;
}

bool QHeaderViewPrivate::isLastVisibleSection(int section) const
{
    if (sectionStartposRecalc)
        recalcSectionStartPos();
    const SectionItem &item = sectionItems.at(section);
    return item.size > 0 && item.calculatedEndPos() == length;
}

/*!
    \internal
    Returns the last visible (ie. not hidden) visual index
*/
int QHeaderViewPrivate::lastVisibleVisualIndex() const
{
    Q_Q(const QHeaderView);
    for (int visual = q->count()-1; visual >= 0; --visual) {
        if (!q->isSectionHidden(q->logicalIndex(visual)))
            return visual;
    }

    //default value if no section is actually visible
    return -1;
}

void QHeaderViewPrivate::restoreSizeOnPrevLastSection()
{
    Q_Q(QHeaderView);
    if (lastSectionLogicalIdx < 0)
        return;
    int resizeLogIdx = lastSectionLogicalIdx;
    lastSectionLogicalIdx = -1; // We do not want resize to catch it as the last section.
    q->resizeSection(resizeLogIdx, lastSectionSize);
}

void QHeaderViewPrivate::setNewLastSection(int visualIndexForLastSection)
{
    Q_Q(QHeaderView);
    lastSectionSize = -1;
    lastSectionLogicalIdx = q->logicalIndex(visualIndexForLastSection);
    lastSectionSize = headerSectionSize(visualIndexForLastSection); // pick size directly since ...
    //  q->sectionSize(lastSectionLogicalIdx) may do delayed resize and stretch it before we get the value.
}

void QHeaderViewPrivate::maybeRestorePrevLastSectionAndStretchLast()
{
    Q_Q(const QHeaderView);
    if (!q->stretchLastSection())
        return;

    int nowLastVisualSection = lastVisibleVisualIndex();
    if (lastSectionLogicalIdx == q->logicalIndex(nowLastVisualSection))
        return;

    // restore old last section.
    restoreSizeOnPrevLastSection();
    setNewLastSection(nowLastVisualSection);
    doDelayedResizeSections(); // Do stretch of last section soon (but not now).
}


/*!
    \internal
    Go through and resize all of the sections applying stretchLastSection,
    manual stretches, sizes, and useGlobalMode.

    The different resize modes are:
    Interactive - the user decides the size
    Stretch - take up whatever space is left
    Fixed - the size is set programmatically outside the header
    ResizeToContentes - the size is set based on the contents of the row or column in the parent view

    The resize mode will not affect the last section if stretchLastSection is true.
*/
void QHeaderViewPrivate::resizeSections(QHeaderView::ResizeMode globalMode, bool useGlobalMode)
{
    Q_Q(QHeaderView);
    //stop the timer in case it is delayed
    delayedResize.stop();

    executePostedLayout();
    if (sectionCount() == 0)
        return;

    if (resizeRecursionBlock)
        return;
    resizeRecursionBlock = true;

    invalidateCachedSizeHint();
    const int lastSectionVisualIdx = q->visualIndex(lastSectionLogicalIdx);

    // find stretchLastSection if we have it
    int stretchSection = -1;
    if (stretchLastSection && !useGlobalMode)
        stretchSection = lastSectionVisualIdx;

    // count up the number of stretched sections and how much space left for them
    int lengthToStretch = (orientation == Qt::Horizontal ? viewport->width() : viewport->height());
    int numberOfStretchedSections = 0;
    QList<int> section_sizes;
    for (int i = 0; i < sectionCount(); ++i) {
        if (isVisualIndexHidden(i))
            continue;

        QHeaderView::ResizeMode resizeMode;
        if (useGlobalMode && (i != stretchSection))
            resizeMode = globalMode;
        else
            resizeMode = (i == stretchSection ? QHeaderView::Stretch : headerSectionResizeMode(i));

        if (resizeMode == QHeaderView::Stretch) {
            ++numberOfStretchedSections;
            section_sizes.append(headerSectionSize(i));
            continue;
        }

        // because it isn't stretch, determine its width and remove that from lengthToStretch
        int sectionSize = 0;
        if (resizeMode == QHeaderView::Interactive || resizeMode == QHeaderView::Fixed) {
            sectionSize = qBound(q->minimumSectionSize(), headerSectionSize(i), q->maximumSectionSize());
        } else { // resizeMode == QHeaderView::ResizeToContents
            int logicalIndex = q->logicalIndex(i);
            sectionSize = qMax(viewSectionSizeHint(logicalIndex),
                               q->sectionSizeHint(logicalIndex));
        }
        sectionSize = qBound(q->minimumSectionSize(),
                             sectionSize,
                             q->maximumSectionSize());

        section_sizes.append(sectionSize);
        lengthToStretch -= sectionSize;
    }

    // calculate the new length for all of the stretched sections
    int stretchSectionLength = -1;
    int pixelReminder = 0;
    if (numberOfStretchedSections > 0 && lengthToStretch > 0) { // we have room to stretch in
        int hintLengthForEveryStretchedSection = lengthToStretch / numberOfStretchedSections;
        stretchSectionLength = qMax(hintLengthForEveryStretchedSection, q->minimumSectionSize());
        pixelReminder = lengthToStretch % numberOfStretchedSections;
    }

    // ### The code below would be nicer if it was cleaned up a bit (since spans has been replaced with items)
    int spanStartSection = 0;
    int previousSectionLength = 0;

    QHeaderView::ResizeMode previousSectionResizeMode = QHeaderView::Interactive;

    // resize each section along the total length
    for (int i = 0; i < sectionCount(); ++i) {
        int oldSectionLength = headerSectionSize(i);
        int newSectionLength = -1;
        QHeaderView::ResizeMode newSectionResizeMode = headerSectionResizeMode(i);

        if (isVisualIndexHidden(i)) {
            newSectionLength = 0;
        } else {
            QHeaderView::ResizeMode resizeMode;
            if (useGlobalMode)
                resizeMode = globalMode;
            else
                resizeMode = (i == stretchSection
                              ? QHeaderView::Stretch
                              : newSectionResizeMode);
            if (resizeMode == QHeaderView::Stretch && stretchSectionLength != -1) {
                if (i == lastSectionVisualIdx)
                    newSectionLength = qMax(stretchSectionLength, lastSectionSize);
                else
                    newSectionLength = stretchSectionLength;
                if (pixelReminder > 0) {
                    newSectionLength += 1;
                    --pixelReminder;
                }
                section_sizes.removeFirst();
            } else {
                newSectionLength = section_sizes.takeFirst();
            }
        }

        //Q_ASSERT(newSectionLength > 0);
        if ((previousSectionResizeMode != newSectionResizeMode
            || previousSectionLength != newSectionLength) && i > 0) {
            createSectionItems(spanStartSection, i - 1, previousSectionLength, previousSectionResizeMode);
            //Q_ASSERT(headerLength() == length);
            spanStartSection = i;
        }

        if (newSectionLength != oldSectionLength)
            emit q->sectionResized(logicalIndex(i), oldSectionLength, newSectionLength);

        previousSectionLength = newSectionLength;
        previousSectionResizeMode = newSectionResizeMode;
    }

    createSectionItems(spanStartSection, sectionCount() - 1,
                       previousSectionLength, previousSectionResizeMode);
    //Q_ASSERT(headerLength() == length);
    resizeRecursionBlock = false;
    viewport->update();
}

void QHeaderViewPrivate::createSectionItems(int start, int end, int sizePerSection, QHeaderView::ResizeMode mode)
{
    if (end >= sectionItems.size()) {
        sectionItems.resize(end + 1);
        sectionStartposRecalc = true;
    }
    SectionItem *sectiondata = sectionItems.data();
    for (int i = start; i <= end; ++i) {
        length += (sizePerSection - sectiondata[i].size);
        sectionStartposRecalc |= (sectiondata[i].size != sizePerSection);
        sectiondata[i].size = sizePerSection;
        sectiondata[i].resizeMode = mode;
    }
}

void QHeaderViewPrivate::removeSectionsFromSectionItems(int start, int end)
{
    // remove sections
    sectionStartposRecalc |= (end != sectionItems.size() - 1);
    int removedlength = 0;
    for (int u = start; u <= end; ++u)
        removedlength += sectionItems.at(u).size;
    length -= removedlength;
    sectionItems.remove(start, end - start + 1);
}

void QHeaderViewPrivate::clear()
{
    if (state != NoClear) {
        length = 0;
        visualIndices.clear();
        logicalIndices.clear();
        sectionSelected.clear();
        hiddenSectionSize.clear();
        sectionItems.clear();
        lastSectionLogicalIdx = -1;
        invalidateCachedSizeHint();
    }
}

static Qt::SortOrder flipOrder(Qt::SortOrder order)
{
    switch (order) {
    case Qt::AscendingOrder:
        return Qt::DescendingOrder;
    case Qt::DescendingOrder:
        return Qt::AscendingOrder;
    };
    Q_UNREACHABLE_RETURN(Qt::AscendingOrder);
};

void QHeaderViewPrivate::flipSortIndicator(int section)
{
    Q_Q(QHeaderView);
    Qt::SortOrder sortOrder;
    if (sortIndicatorSection == section) {
        if (sortIndicatorClearable) {
            const Qt::SortOrder defaultSortOrder = defaultSortOrderForSection(section);
            if (sortIndicatorOrder == defaultSortOrder) {
                sortOrder = flipOrder(sortIndicatorOrder);
            } else {
                section = -1;
                sortOrder = Qt::AscendingOrder;
            }
        } else {
            sortOrder = flipOrder(sortIndicatorOrder);
        }
    } else {
        sortOrder = defaultSortOrderForSection(section);
    }
    q->setSortIndicator(section, sortOrder);
}

Qt::SortOrder QHeaderViewPrivate::defaultSortOrderForSection(int section) const
{
    const QVariant value = model->headerData(section, orientation, Qt::InitialSortOrderRole);
    if (value.canConvert<int>())
        return static_cast<Qt::SortOrder>(value.toInt());
    return Qt::AscendingOrder;
}

void QHeaderViewPrivate::cascadingResize(int visual, int newSize)
{
    Q_Q(QHeaderView);
    const int minimumSize = q->minimumSectionSize();
    const int oldSize = headerSectionSize(visual);
    int delta = newSize - oldSize;

    if (delta > 0) { // larger
        bool sectionResized = false;

        // restore old section sizes
        for (int i = firstCascadingSection; i < visual; ++i) {
            if (cascadingSectionSize.contains(i)) {
                int currentSectionSize = headerSectionSize(i);
                int originalSectionSize = cascadingSectionSize.value(i);
                if (currentSectionSize < originalSectionSize) {
                    int newSectionSize = currentSectionSize + delta;
                    resizeSectionItem(i, currentSectionSize, newSectionSize);
                    if (newSectionSize >= originalSectionSize && false)
                        cascadingSectionSize.remove(i); // the section is now restored
                    sectionResized = true;
                    break;
                }
            }

        }

        // resize the section
        if (!sectionResized) {
            newSize = qMax(newSize, minimumSize);
            if (oldSize != newSize)
                resizeSectionItem(visual, oldSize, newSize);
        }

        // cascade the section size change
        for (int i = visual + 1; i < sectionCount(); ++i) {
            if (isVisualIndexHidden(i))
                continue;
            if (!sectionIsCascadable(i))
                continue;
            int currentSectionSize = headerSectionSize(i);
            if (currentSectionSize <= minimumSize)
                continue;
            int newSectionSize = qMax(currentSectionSize - delta, minimumSize);
            resizeSectionItem(i, currentSectionSize, newSectionSize);
            saveCascadingSectionSize(i, currentSectionSize);
            delta = delta - (currentSectionSize - newSectionSize);
            if (delta <= 0)
                break;
        }
    } else { // smaller
        bool sectionResized = false;

        // restore old section sizes
        for (int i = lastCascadingSection; i > visual; --i) {
            if (!cascadingSectionSize.contains(i))
                continue;
            int currentSectionSize = headerSectionSize(i);
            int originalSectionSize = cascadingSectionSize.value(i);
            if (currentSectionSize >= originalSectionSize)
                continue;
            int newSectionSize = currentSectionSize - delta;
            resizeSectionItem(i, currentSectionSize, newSectionSize);
            if (newSectionSize >= originalSectionSize && false) {
                cascadingSectionSize.remove(i); // the section is now restored
            }
            sectionResized = true;
            break;
        }

        // resize the section
        resizeSectionItem(visual, oldSize, qMax(newSize, minimumSize));

        // cascade the section size change
        if (delta < 0 && newSize < minimumSize) {
            for (int i = visual - 1; i >= 0; --i) {
                if (isVisualIndexHidden(i))
                    continue;
                if (!sectionIsCascadable(i))
                    continue;
                int sectionSize = headerSectionSize(i);
                if (sectionSize <= minimumSize)
                    continue;
                resizeSectionItem(i, sectionSize, qMax(sectionSize + delta, minimumSize));
                saveCascadingSectionSize(i, sectionSize);
                break;
            }
        }

        // let the next section get the space from the resized section
        if (!sectionResized) {
            for (int i = visual + 1; i < sectionCount(); ++i) {
                if (isVisualIndexHidden(i))
                    continue;
                if (!sectionIsCascadable(i))
                    continue;
                int currentSectionSize = headerSectionSize(i);
                int newSectionSize = qMax(currentSectionSize - delta, minimumSize);
                resizeSectionItem(i, currentSectionSize, newSectionSize);
                break;
            }
        }
    }

    if (hasAutoResizeSections())
        doDelayedResizeSections();

    viewport->update();
}

void QHeaderViewPrivate::setDefaultSectionSize(int size)
{
    Q_Q(QHeaderView);
    size = qBound(q->minimumSectionSize(), size, q->maximumSectionSize());
    executePostedLayout();
    invalidateCachedSizeHint();
    defaultSectionSize = size;
    customDefaultSectionSize = true;
    if (state == QHeaderViewPrivate::ResizeSection)
        preventCursorChangeInSetOffset = true;
    for (int i = 0; i < sectionItems.size(); ++i) {
        QHeaderViewPrivate::SectionItem &section = sectionItems[i];
        if (hiddenSectionSize.isEmpty() || !isVisualIndexHidden(i)) { // resize on not hidden.
            const int newSize = size;
            if (newSize != section.size) {
                length += newSize - section.size; //the whole length is changed
                const int oldSectionSize = section.sectionSize();
                section.size = size;
                emit q->sectionResized(logicalIndex(i), oldSectionSize, size);
            }
        }
    }
    sectionStartposRecalc = true;
    if (hasAutoResizeSections())
        doDelayedResizeSections();
    viewport->update();
}

void QHeaderViewPrivate::updateDefaultSectionSizeFromStyle()
{
    Q_Q(QHeaderView);
    if (orientation == Qt::Horizontal) {
        defaultSectionSize = q->style()->pixelMetric(QStyle::PM_HeaderDefaultSectionSizeHorizontal, nullptr, q);
    } else {
        defaultSectionSize = qMax(q->minimumSectionSize(),
                                  q->style()->pixelMetric(QStyle::PM_HeaderDefaultSectionSizeVertical, nullptr, q));
    }
}

void QHeaderViewPrivate::recalcSectionStartPos() const // linear (but fast)
{
    int pixelpos = 0;
    for (const SectionItem &i : sectionItems) {
        i.calculated_startpos = pixelpos; // write into const mutable
        pixelpos += i.size;
    }
    sectionStartposRecalc = false;
}

void QHeaderViewPrivate::resizeSectionItem(int visualIndex, int oldSize, int newSize)
{
    Q_Q(QHeaderView);
    QHeaderView::ResizeMode mode = headerSectionResizeMode(visualIndex);
    createSectionItems(visualIndex, visualIndex, newSize, mode);
    emit q->sectionResized(logicalIndex(visualIndex), oldSize, newSize);
}

int QHeaderViewPrivate::headerSectionSize(int visual) const
{
    if (visual < sectionCount() && visual >= 0)
        return sectionItems.at(visual).sectionSize();
    return -1;
}

int QHeaderViewPrivate::headerSectionPosition(int visual) const
{
    if (visual < sectionCount() && visual >= 0) {
        if (sectionStartposRecalc)
            recalcSectionStartPos();
        return sectionItems.at(visual).calculated_startpos;
    }
    return -1;
}

int QHeaderViewPrivate::headerVisualIndexAt(int position) const
{
    if (sectionStartposRecalc)
        recalcSectionStartPos();
    int startidx = 0;
    int endidx = sectionItems.size() - 1;
    while (startidx <= endidx) {
        int middle = (endidx + startidx) / 2;
        if (sectionItems.at(middle).calculated_startpos > position) {
            endidx = middle - 1;
        } else {
            if (sectionItems.at(middle).calculatedEndPos() <= position)
                startidx = middle + 1;
            else // we found it.
                return middle;
        }
    }
    return -1;
}

void QHeaderViewPrivate::setHeaderSectionResizeMode(int visual, QHeaderView::ResizeMode mode)
{
    int size = headerSectionSize(visual);
    createSectionItems(visual, visual, size, mode);
}

QHeaderView::ResizeMode QHeaderViewPrivate::headerSectionResizeMode(int visual) const
{
    if (visual < 0 || visual >= sectionItems.size())
        return globalResizeMode;
    return static_cast<QHeaderView::ResizeMode>(sectionItems.at(visual).resizeMode);
}

void QHeaderViewPrivate::setGlobalHeaderResizeMode(QHeaderView::ResizeMode mode)
{
    globalResizeMode = mode;
    for (int i = 0; i < sectionItems.size(); ++i)
        sectionItems[i].resizeMode = mode;
}

int QHeaderViewPrivate::viewSectionSizeHint(int logical) const
{
    if (QAbstractItemView *view = qobject_cast<QAbstractItemView*>(parent)) {
        return (orientation == Qt::Horizontal
                ? view->sizeHintForColumn(logical)
                : view->sizeHintForRow(logical));
    }
    return 0;
}

int QHeaderViewPrivate::adjustedVisualIndex(int visualIndex) const
{
    if (!hiddenSectionSize.isEmpty()) {
        int adjustedVisualIndex = visualIndex;
        int currentVisualIndex = 0;
        for (int i = 0; i < sectionItems.size(); ++i) {
            if (isVisualIndexHidden(i))
                ++adjustedVisualIndex;
            else
                ++currentVisualIndex;
            if (currentVisualIndex >= visualIndex)
                break;
        }
        visualIndex = adjustedVisualIndex;
    }
    return visualIndex;
}

void QHeaderViewPrivate::setScrollOffset(const QScrollBar *scrollBar, QAbstractItemView::ScrollMode scrollMode)
{
    Q_Q(QHeaderView);
    if (scrollMode == QAbstractItemView::ScrollPerItem) {
        if (scrollBar->maximum() > 0 && scrollBar->value() == scrollBar->maximum())
            q->setOffsetToLastSection();
        else
            q->setOffsetToSectionPosition(scrollBar->value());
    } else {
        q->setOffset(scrollBar->value());
    }
}

void QHeaderViewPrivate::updateSectionsBeforeAfter(int logical)
{
    Q_Q(QHeaderView);
    const int visual = visualIndex(logical);
    int from = logicalIndex(visual > 1 ? visual - 1 : 0);
    int to = logicalIndex(visual + 1 >= sectionCount() ? visual : visual + 1);
    QRect updateRect;
    if (orientation == Qt::Horizontal) {
        if (reverse())
            std::swap(from, to);
        updateRect = QRect(QPoint(q->sectionViewportPosition(from), 0),
                           QPoint(q->sectionViewportPosition(to) + headerSectionSize(to), viewport->height()));
    } else {
        updateRect = QRect(QPoint(0, q->sectionViewportPosition(from)),
                           QPoint(viewport->width(), q->sectionViewportPosition(to) + headerSectionSize(to)));
    }
    viewport->update(updateRect);
}

#ifndef QT_NO_DATASTREAM
void QHeaderViewPrivate::write(QDataStream &out) const
{
    out << int(orientation);
    out << int(sortIndicatorOrder);
    out << sortIndicatorSection;
    out << sortIndicatorShown;

    out << visualIndices;
    out << logicalIndices;

    out << sectionsHiddenToBitVector();
    out << hiddenSectionSize;

    out << length;
    out << sectionCount();
    out << movableSections;
    out << clickableSections;
    out << highlightSelected;
    out << stretchLastSection;
    out << cascadingResizing;
    out << stretchSections;
    out << contentsSections;
    out << defaultSectionSize;
    out << minimumSectionSize;

    out << int(defaultAlignment);
    out << int(globalResizeMode);

    out << sectionItems;
    out << resizeContentsPrecision;
    out << customDefaultSectionSize;
    out << lastSectionSize;
    out << int(sortIndicatorClearable);
}

bool QHeaderViewPrivate::read(QDataStream &in)
{
    Q_Q(QHeaderView);
    int orient, order, align, global;
    int sortIndicatorSectionIn;
    bool sortIndicatorShownIn;
    int lengthIn;
    QList<int> visualIndicesIn;
    QList<int> logicalIndicesIn;
    QHash<int, int> hiddenSectionSizeIn;
    bool movableSectionsIn;
    bool clickableSectionsIn;
    bool highlightSelectedIn;
    bool stretchLastSectionIn;
    bool cascadingResizingIn;
    int stretchSectionsIn;
    int contentsSectionsIn;
    int defaultSectionSizeIn;
    int minimumSectionSizeIn;
    QList<SectionItem> sectionItemsIn;

    in >> orient;
    in >> order;

    in >> sortIndicatorSectionIn;
    in >> sortIndicatorShownIn;

    in >> visualIndicesIn;
    in >> logicalIndicesIn;

    QBitArray sectionHidden;
    in >> sectionHidden;
    in >> hiddenSectionSizeIn;
    in >> lengthIn;

    int unusedSectionCount; // For compatibility
    in >> unusedSectionCount;

    if (in.status() != QDataStream::Ok || lengthIn < 0)
        return false;

    in >> movableSectionsIn;
    in >> clickableSectionsIn;
    in >> highlightSelectedIn;
    in >> stretchLastSectionIn;
    in >> cascadingResizingIn;
    in >> stretchSectionsIn;
    in >> contentsSectionsIn;
    in >> defaultSectionSizeIn;
    in >> minimumSectionSizeIn;

    in >> align;

    in >> global;

    // Check parameter consistency
    // Global orientation out of bounds?
    if (global < 0 || global > QHeaderView::ResizeToContents)
        return false;

    // Alignment out of bounds?
    if (align < 0 || align > Qt::AlignVertical_Mask)
        return false;

    in >> sectionItemsIn;
    // In Qt4 we had a vector of spans where one span could hold information on more sections.
    // Now we have an itemvector where one items contains information about one section
    // For backward compatibility with Qt4 we do the following
    QList<SectionItem> newSectionItems;
    for (int u = 0; u < sectionItemsIn.size(); ++u) {
        int count = sectionItemsIn.at(u).tmpDataStreamSectionCount;
        if (count > 1)
            sectionItemsIn[u].size /= count;
        for (int n = 0; n < count; ++n)
            newSectionItems.append(sectionItemsIn[u]);
    }

    int sectionItemsLengthTotal = 0;
    for (const SectionItem &section : std::as_const(newSectionItems))
        sectionItemsLengthTotal += section.size;
    if (sectionItemsLengthTotal != lengthIn)
        return false;

    const int currentCount = (orient == Qt::Horizontal ? model->columnCount(root) : model->rowCount(root));
    if (newSectionItems.size() < currentCount) {
        // we have sections not in the saved state, give them default settings
        if (!visualIndicesIn.isEmpty() && !logicalIndicesIn.isEmpty()) {
            for (int i = newSectionItems.size(); i < currentCount; ++i) {
                visualIndicesIn.append(i);
                logicalIndicesIn.append(i);
            }
        }
        const int insertCount = currentCount - newSectionItems.size();
        const int insertLength = defaultSectionSizeIn * insertCount;
        lengthIn += insertLength;
        SectionItem section(defaultSectionSizeIn, globalResizeMode);
        newSectionItems.insert(newSectionItems.size(), insertCount, section); // append
    }

    orientation = static_cast<Qt::Orientation>(orient);
    sortIndicatorOrder = static_cast<Qt::SortOrder>(order);
    sortIndicatorSection = sortIndicatorSectionIn;
    sortIndicatorShown = sortIndicatorShownIn;
    visualIndices = visualIndicesIn;
    logicalIndices = logicalIndicesIn;
    hiddenSectionSize = hiddenSectionSizeIn;
    length = lengthIn;

    movableSections = movableSectionsIn;
    clickableSections = clickableSectionsIn;
    highlightSelected = highlightSelectedIn;
    stretchLastSection = stretchLastSectionIn;
    cascadingResizing = cascadingResizingIn;
    stretchSections = stretchSectionsIn;
    contentsSections = contentsSectionsIn;
    defaultSectionSize = defaultSectionSizeIn;
    minimumSectionSize = minimumSectionSizeIn;

    defaultAlignment = Qt::Alignment(align);
    globalResizeMode = static_cast<QHeaderView::ResizeMode>(global);

    sectionItems = newSectionItems;
    setHiddenSectionsFromBitVector(sectionHidden);
    recalcSectionStartPos();

    int tmpint;
    in >> tmpint;
    if (in.status() == QDataStream::Ok)  // we haven't read past end
        resizeContentsPrecision = tmpint;

    bool tmpbool;
    in >> tmpbool;
    if (in.status() == QDataStream::Ok) {  // we haven't read past end
        customDefaultSectionSize = tmpbool;
        if (!customDefaultSectionSize)
            updateDefaultSectionSizeFromStyle();
    }

    lastSectionSize = -1;
    int inLastSectionSize;
    in >> inLastSectionSize;
    if (in.status() == QDataStream::Ok)
        lastSectionSize = inLastSectionSize;

    lastSectionLogicalIdx = -1;
    if (stretchLastSection) {
        lastSectionLogicalIdx = q->logicalIndex(lastVisibleVisualIndex());
        doDelayedResizeSections();
    }

    int inSortIndicatorClearable;
    in >> inSortIndicatorClearable;
    if (in.status() == QDataStream::Ok)  // we haven't read past end
        sortIndicatorClearable = inSortIndicatorClearable;

    return true;
}

#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE

#include "moc_qheaderview.cpp"
