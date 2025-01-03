// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpageranges.h"
#include "qpageranges_p.h"

#include <QtCore/qstack.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QPageRanges)

void QPageRangesPrivate::mergeIntervals()
{
    const int count = intervals.size();

    if (count <= 1)
        return;

    std::sort(intervals.begin(), intervals.end());

    QStack<QPageRanges::Range> stack;
    stack.push(intervals[0]);

    for (int i = 1; i < count; ++i) {
        QPageRanges::Range &top = stack.top();

        if (top.to < intervals[i].from - 1)
            stack.push(intervals[i]);
        else if (top.to < intervals[i].to)
            top.to = intervals[i].to;
    }

    intervals = stack;
}

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QPageRangesPrivate)

/*!
    \class QPageRanges
    \brief The QPageRanges class represents a collection of page ranges.
    \inmodule QtGui
    \ingroup painting
    \ingroup printing
    \ingroup shared
    \since 6.0

    Use QPagedPaintDevice::pageRanges() to access the collection of page ranges
    associated with a paged device.
*/

/*!
    Constructs an empty QPageRanges object.
*/
QPageRanges::QPageRanges() = default;

/*!
    Constructs a QPageRanges object by copying \a other.
*/
QPageRanges::QPageRanges(const QPageRanges &other) noexcept = default;

/*!
    \fn QPageRanges::QPageRanges(QPageRanges &&other)

    Constructs a QPageRanges object by moving from \a other.
*/

/*!
    Destroys the page ranges.
*/
QPageRanges::~QPageRanges() = default;

/*!
    Assigns \a other to this QPageRanges object.
*/
QPageRanges &QPageRanges::operator=(const QPageRanges &other) noexcept = default;

/*!
    \fn QPageRanges &QPageRanges::operator=(QPageRanges &&other)
    Moves \a other into this QPageRanges object.
*/

/*!
    Adds the single page \a pageNumber to the ranges.

    \note Page numbers start with 1. Attempts to add page numbers
    smaller than 1 will be ignored with a warning.
*/
void QPageRanges::addPage(int pageNumber)
{
    if (pageNumber <= 0) {
        qWarning("QPageRanges::addPage: 'pageNumber' must be greater than 0");
        return;
    }

    detach();
    d->intervals.append({ pageNumber, pageNumber });
    d->mergeIntervals();
}

/*!
    Adds the range specified with \a from and \a to to the ranges.

    \note Page numbers start with 1. Attempts to add page numbers
    smaller than 1 will be ignored with a warning.
*/
void QPageRanges::addRange(int from, int to)
{
    if (from <= 0 || to <= 0) {
        qWarning("QPageRanges::addRange: 'from' and 'to' must be greater than 0");
        return;
    }
    if (to < from)
        std::swap(from, to);

    detach();
    d->intervals.append({from, to});
    d->mergeIntervals();
}

/*!
    Returns a list with the values of the ranges.
*/
QList<QPageRanges::Range> QPageRanges::toRangeList() const
{
    if (d)
        return d->intervals;
    return QList<QPageRanges::Range>{};
}

/*!
    Removes all page ranges.
*/
void QPageRanges::clear()
{
    d.reset();
}

/*!
    Constructs and returns a QPageRanges object populated with the
    \a ranges from the string representation.

    \code
    QPrinter printer;
    QPageRanges ranges = QPageRanges::fromString("1-3,6-7");
    printer.setPageRanges(ranges);
    \endcode

    In case of parsing error, returns an empty QPageRanges object.

    \sa isEmpty()
*/
QPageRanges QPageRanges::fromString(const QString &ranges)
{
    QList<Range> intervals;
    const QStringList items = ranges.split(u',');
    for (const QString &item : items) {
        if (item.isEmpty())
            return QPageRanges();

        if (item.contains(u'-')) {
            const QStringList rangeItems = item.split(u'-');
            if (rangeItems.size() != 2)
                return QPageRanges();

            bool ok;
            const int number1 = rangeItems[0].toInt(&ok);
            if (!ok)
                return QPageRanges();

            const int number2 = rangeItems[1].toInt(&ok);
            if (!ok)
                return QPageRanges();

            if (number1 < 1 || number2 < 1 || number2 < number1)
                return QPageRanges();

            intervals.append({number1, number2});

        } else {
            bool ok;
            const int number = item.toInt(&ok);
            if (!ok)
                return QPageRanges();

            if (number < 1)
                return QPageRanges();

            intervals.append({number, number});
        }
    }

    QPageRanges newRanges;
    newRanges.d.reset(new QPageRangesPrivate);
    newRanges.d->intervals = intervals;
    newRanges.d->mergeIntervals();
    return newRanges;
}

/*!
    Returns the string representation of the page ranges.
*/
QString QPageRanges::toString() const
{
    if (!d)
        return QString();

    QString result;
    for (const Range &range : d->intervals) {
        if (!result.isEmpty())
            result += u',';

        if (range.from == range.to)
            result += QString::number(range.from);
        else
            result += QStringLiteral("%1-%2").arg(range.from).arg(range.to);
    }

    return result;
}

/*!
    \fn bool QPageRanges::contains(int pageNumber) const

    Returns \c true if the ranges include the page \a pageNumber;
    otherwise returns \c false.
*/
bool QPageRanges::contains(int pageNumber) const
{
    if (!d)
        return false;

    for (const Range &range : d->intervals) {
        if (range.from <= pageNumber && range.to >= pageNumber)
            return true;
    }
    return false;
}

/*!
    Returns \c true if the ranges are empty; otherwise returns \c false.
*/
bool QPageRanges::isEmpty() const
{
    return !d || d->intervals.isEmpty();
}

/*!
    Returns the index of the first page covered by the page ranges,
    or 0 if the page ranges are empty.
*/
int QPageRanges::firstPage() const
{
    if (isEmpty())
        return 0;
    return d->intervals.first().from;
}

/*!
    Returns the index of the last page covered by the page ranges,
    or 0 if the page ranges are empty.
*/
int QPageRanges::lastPage() const
{
    if (isEmpty())
        return 0;
    return d->intervals.last().to;
}

/*!
    \internal
*/
bool QPageRanges::isEqual(const QPageRanges &other) const noexcept
{
    if (d == other.d)
        return true;
    if (!d || !other.d)
        return false;
    return d->intervals == other.d->intervals;
}

/*!
    \internal
*/
void QPageRanges::detach()
{
    if (d)
        d.detach();
    else
        d.reset(new QPageRangesPrivate);
}

#if !defined(QT_NO_DATASTREAM)
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QPageRanges &pageRanges)
    \relates QPageRanges

    Writes \a pageRanges to \a stream as a range string.

    \sa QPageRanges::toString
*/

QDataStream &operator<<(QDataStream &s, const QPageRanges &pageRanges)
{
    s << pageRanges.toString();
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QPageRanges &pageRanges)
    \relates QPageRanges

    Reads a page ranges string from \a stream and stores it in \a pageRanges.

    \sa QPageRanges::fromString
*/

QDataStream &operator>>(QDataStream &s, QPageRanges &pageRanges)
{
    QString rangesString;
    s >> rangesString;
    pageRanges = QPageRanges::fromString(rangesString);
    return s;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QPageRanges &pageRanges)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg.noquote();
    dbg << "QPageRanges(" << pageRanges.toString() << ")";

    return dbg;
}
#endif

/*!
    \struct QPageRanges::Range
    \inmodule QtGui
    \brief The QPageRanges::Range struct holds the \c from and \c to endpoints of a range.

    \sa QPageRanges::toRangeList()
*/

/*!
    \variable QPageRanges::Range::from
    \brief the lower endpoint of the range
*/

/*!
    \variable QPageRanges::Range::to
    \brief the upper endpoint of the range
*/

/*!
    \fn bool QPageRanges::Range::contains(int pageNumber) const

    Returns \c true if \a pageNumber is within the interval \c{[from, to]};
    otherwise returns \c false.
*/


QT_END_NAMESPACE
