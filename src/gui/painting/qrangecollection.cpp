/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qrangecollection.h"
#include "qrangecollection_p.h"

#include <QtCore/qstack.h>

QT_BEGIN_NAMESPACE

void QRangeCollectionPrivate::mergeIntervals()
{
    const int count = intervals.count();

    if (count <= 0)
        return;

    std::sort(intervals.begin(), intervals.end());

    QStack<QPair<int, int>> stack;
    stack.push(intervals[0]);

    for (int i = 1; i < count; i++) {
        QPair<int, int> &top = stack.top();

        if (top.second < intervals[i].first)
            stack.push(intervals[i]);
        else if (top.second < intervals[i].second)
            top.second = intervals[i].second;
    }

    std::sort(intervals.begin(), intervals.end());

    intervals = stack;
}

/*!
    \class QRangeCollection
    \brief The QRangeCollection class represents a collection of decimal intervals.
    \inmodule QtGui
    \ingroup painting
    \since 6.0

    QRangeCollection manages a set of decimal intervals.

    Use QPrinter::rangeCollection() to access the collection of page ranges
    associated with a QPrinter.
*/

QRangeCollection::QRangeCollection()
    : d_ptr(new QRangeCollectionPrivate(this))
{
}

/*!
    Destroys the collection.
*/
QRangeCollection::~QRangeCollection()
{
}

/*!
    Inserts a single number into the collection.
 */
void QRangeCollection::addPage(int pageNumber)
{
    Q_D(QRangeCollection);
    if (pageNumber <= 0) {
        qWarning("QRangeCollection::addPage: 'pageNumber' must be greater than 0");
        return;
    }
    d->intervals.append(qMakePair(pageNumber, pageNumber));
    d->mergeIntervals();
}

/*!
    Inserts a range into the collection.
 */
void QRangeCollection::addRange(int from, int to)
{
    Q_D(QRangeCollection);
    if (from <= 0 || to <= 0) {
        qWarning("QRangeCollection::addRange: 'from' and 'to' must be greater than 0");
        return;
    }
    if (to < from) {
        qWarning("QRangeCollection::addRange: 'from' must be less than or equal to 'to'");
        std::swap(from, to);
    }
    d->intervals.append(qMakePair(from, to));
    d->mergeIntervals();
}

/*!
    Returns a list with the values of the ranges used in this collection.
 */
QList<QPair<int, int>> QRangeCollection::toList() const
{
    Q_D(const QRangeCollection);
    return d->intervals.toList();
}

/*!
 * Removes all ranges from this collection.
 */
void QRangeCollection::clear()
{
    Q_D(QRangeCollection);
    d->intervals.clear();
}

/*!
    Constructs the range collection from a string representation.

    \code
    QPrinter printer;
    printer->rangeCollection()->parse("1-3,6-7");
    \endcode
 */
bool QRangeCollection::parse(const QString &ranges)
{
    Q_D(QRangeCollection);
    const QStringList items = ranges.split(u',');
    for (const QString &item : items) {
        if (item.isEmpty()) {
            d->intervals.clear();
            return false;
        }

        if (item.contains(QLatin1Char('-'))) {
            const QStringList rangeItems = item.split(u'-');
            if (rangeItems.count() != 2) {
                d->intervals.clear();
                return false;
            }

            bool ok;
            const int number1 = rangeItems[0].toInt(&ok);
            if (!ok) {
                d->intervals.clear();
                return false;
            }

            const int number2 = rangeItems[1].toInt(&ok);
            if (!ok) {
                d->intervals.clear();
                return false;
            }

            if (number1 < 1 || number2 < 1 || number2 < number1) {
                d->intervals.clear();
                return false;
            }

            d->intervals.append(qMakePair(number1, number2));

        } else {
            bool ok;
            const int number = item.toInt(&ok);
            if (!ok) {
                d->intervals.clear();
                return false;
            }

            if (number < 1) {
                d->intervals.clear();
                return false;
            }

            d->intervals.append(qMakePair(number, number));
        }
    }

    d->mergeIntervals();
    return true;
}

/*!
    Returns the string representation of the ranges in the collection.
 */
QString QRangeCollection::toString() const
{
    Q_D(const QRangeCollection);
    QString result;

    for (const QPair<int, int> &pair : d->intervals) {
        if (!result.isEmpty())
            result += QLatin1Char(',');

        if (pair.first == pair.second)
            result += QString::number(pair.first);
        else
            result += QStringLiteral("%1-%2").arg(pair.first).arg(pair.second);
    }

    return result;
}

/*!
    Returns \c true if the collection contains an occurrence
    or a bounding range of \a pageNumber; otherwise returns
    \c false.
 */
bool QRangeCollection::contains(int pageNumber) const
{
    Q_D(const QRangeCollection);
    for (const QPair<int, int> &pair : d->intervals) {
        if (pair.first <= pageNumber && pair.second >= pageNumber)
            return true;
    }
    return false;
}

/*!
    Returns \c true if the collection is empty; otherwise returns \c false.
*/
bool QRangeCollection::isEmpty() const
{
    Q_D(const QRangeCollection);
    return d->intervals.isEmpty();
}

/*!
    Returns the index of the first page covered by the range collection.
*/
int QRangeCollection::firstPage() const
{
    Q_D(const QRangeCollection);
    if (d->intervals.isEmpty())
        return 0;
    return d->intervals.first().first;
}

/*!
    Returns the index of the last page covered by the range collection.
*/
int QRangeCollection::lastPage() const
{
    Q_D(const QRangeCollection);
    if (d->intervals.isEmpty())
        return 0;
    return d->intervals.last().second;
}

QT_END_NAMESPACE
