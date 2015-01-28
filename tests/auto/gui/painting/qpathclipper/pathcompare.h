/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef PATHCOMPARE_H
#define PATHCOMPARE_H

#include <qmath.h>

namespace QPathCompare {

static const int precision = 8;
static const qreal epsilon = qPow(0.1, precision);

static inline bool fuzzyIsZero(qreal x, qreal relative)
{
    if (qAbs(relative) < epsilon)
        return qAbs(x) < epsilon;
    else
        return qAbs(x / relative) < epsilon;
}

static bool fuzzyCompare(const QPointF &a, const QPointF &b)
{
    const QPointF delta = a - b;

    const qreal x = qMax(qAbs(a.x()), qAbs(b.x()));
    const qreal y = qMax(qAbs(a.y()), qAbs(b.y()));

    return fuzzyIsZero(delta.x(), x) && fuzzyIsZero(delta.y(), y);
}

static bool isClosed(const QPainterPath &path)
{
    if (path.elementCount() == 0)
        return false;

    QPointF first = path.elementAt(0);
    QPointF last = path.elementAt(path.elementCount() - 1);

    return fuzzyCompare(first, last);
}

// rotation and direction independent path comparison
// allows paths to be shifted or reversed relative to each other
static bool comparePaths(const QPainterPath &actual, const QPainterPath &expected)
{
    const int endActual = isClosed(actual) ? actual.elementCount() - 1 : actual.elementCount();
    const int endExpected = isClosed(expected) ? expected.elementCount() - 1 : expected.elementCount();

    if (endActual != endExpected)
        return false;

    for (int i = 0; i < endActual; ++i) {
        int k = 0;
        for (k = 0; k < endActual; ++k) {
            int i1 = k;
            int i2 = (i + k) % endActual;

            QPointF a = actual.elementAt(i1);
            QPointF b = expected.elementAt(i2);

            if (!fuzzyCompare(a, b))
                break;
        }

        if (k == endActual)
            return true;

        for (k = 0; k < endActual; ++k) {
            int i1 = k;
            int i2 = (i + endActual - k) % endActual;

            QPointF a = actual.elementAt(i1);
            QPointF b = expected.elementAt(i2);

            if (!fuzzyCompare(a, b))
                break;
        }

        if (k == endActual)
            return true;
    }

    return false;
}

}

#endif
