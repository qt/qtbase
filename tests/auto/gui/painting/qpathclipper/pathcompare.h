// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
