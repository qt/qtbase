/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef HIGHDPI_H
#define HIGHDPI_H

#include <QtCore/qbytearray.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>

// Helpers for comparing geometries a that may go through scaling in the
// platform plugin with fuzz (pass rounded-down device pixel ratios or
// scaling factors). Error message for use with QVERIFY2() are also provided.

class HighDpi
{
public:
    HighDpi() = delete;
    HighDpi(const HighDpi &) = delete;
    HighDpi &operator=(const HighDpi &) = delete;
    HighDpi(HighDpi &&) = delete;
    HighDpi &operator=(HighDpi &&) = delete;
    ~HighDpi() = delete;

    static int manhattanDelta(const QPoint &p1, const QPoint p2)
    {
        return (p1 - p2).manhattanLength();
    }

    static bool fuzzyCompare(const QPoint &p1, const QPoint p2, int fuzz)
    {
        return manhattanDelta(p1, p2) <= fuzz;
    }

    static QByteArray msgPointMismatch(const QPoint &p1, const QPoint p2)
    {
        return QByteArray::number(p1.x()) + ',' + QByteArray::number(p1.y())
                + " != " + QByteArray::number(p2.x()) + ',' + QByteArray::number(p2.y())
                + ", manhattanLength=" + QByteArray::number(manhattanDelta(p1, p2));
    }

    // Compare a size that may go through scaling in the platform plugin with fuzz.

    static inline int manhattanDelta(const QSize &s1, const QSize &s2)
    {
        return qAbs(s1.width() - s2.width()) + qAbs(s1.height() - s2.height());
    }

    static inline bool fuzzyCompare(const QSize &s1, const QSize &s2, int fuzz)
    {
        return manhattanDelta(s1, s2) <= fuzz;
    }

    static QByteArray msgSizeMismatch(const QSize &s1, const QSize &s2)
    {
        return QByteArray::number(s1.width()) + 'x' + QByteArray::number(s1.height())
                + " != " + QByteArray::number(s2.width()) + 'x' + QByteArray::number(s2.height())
                + ", manhattanLength=" + QByteArray::number(manhattanDelta(s1, s2));
    }

    // Compare a geometry that may go through scaling in the platform plugin with fuzz.

    static inline bool fuzzyCompare(const QRect &r1, const QRect &r2, int fuzz)
    {
        return manhattanDelta(r1.topLeft(), r2.topLeft()) <= fuzz
                && manhattanDelta(r1.size(), r2.size()) <= fuzz;
    }

    static QByteArray msgRectMismatch(const QRect &r1, const QRect &r2)
    {
        return formatRect(r1) + " != " + formatRect(r2);
    }

private:
    static QByteArray formatRect(const QRect &r)
    {
        return QByteArray::number(r.width()) + 'x' + QByteArray::number(r.height())
                + (r.left() < 0 ? '-' : '+') + QByteArray::number(r.left())
                + (r.top() < 0 ? '-' : '+') + QByteArray::number(r.top());
    }
};

#endif // HIGHDPI_H
