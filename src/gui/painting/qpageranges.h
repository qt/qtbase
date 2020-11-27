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

#ifndef QPAGERANGES_H
#define QPAGERANGES_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QDataStream;
class QPageRangesPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QPageRangesPrivate, Q_GUI_EXPORT)

class Q_GUI_EXPORT QPageRanges
{
public:
    QPageRanges();
    ~QPageRanges();

    QPageRanges(const QPageRanges &other) noexcept;
    QPageRanges &operator=(const QPageRanges &other) noexcept;

    QPageRanges(QPageRanges &&other) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QPageRanges)
    void swap(QPageRanges &other) noexcept
    { qSwap(d, other.d); }

    friend bool operator==(const QPageRanges &lhs, const QPageRanges &rhs) noexcept
    { return lhs.isEqual(rhs); }
    friend bool operator!=(const QPageRanges &lhs, const QPageRanges &rhs) noexcept
    { return !lhs.isEqual(rhs); }

    struct Range {
        int from = -1;
        int to = -1;
        bool contains(int pageNumber) const noexcept
        { return from <= pageNumber && to >= pageNumber; }
        friend bool operator==(Range lhs, Range rhs) noexcept
        { return lhs.from == rhs.from && lhs.to == rhs.to; }
        friend bool operator!=(Range lhs, Range rhs) noexcept
        { return !(lhs == rhs); }
        friend bool operator<(Range lhs, Range rhs) noexcept
        { return lhs.from < rhs.from || (!(rhs.from < lhs.from) && lhs.to < rhs.to); }
    };

    void addPage(int pageNumber);
    void addRange(int from, int to);
    QList<Range> toRangeList() const;
    void clear();

    QString toString() const;
    static QPageRanges fromString(const QString &ranges);

    bool contains(int pageNumber) const;
    bool isEmpty() const;
    int firstPage() const;
    int lastPage() const;

    void detach();

private:
    bool isEqual(const QPageRanges &other) const noexcept;

    QExplicitlySharedDataPointer<QPageRangesPrivate> d;
};

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QPageRanges &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPageRanges &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QPageRanges &pageRanges);
#endif

Q_DECLARE_SHARED(QPageRanges)
Q_DECLARE_TYPEINFO(QPageRanges::Range, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QPageRanges)

#endif // QPAGERANGES_H
