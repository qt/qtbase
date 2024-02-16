// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORCLUT_H
#define QCOLORCLUT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qlist.h>
#include <QtGui/private/qcolormatrix_p.h>

QT_BEGIN_NAMESPACE

// A 3-dimensional lookup table compatible with ICC lut8, lut16, mAB, and mBA formats.
class QColorCLUT
{
    inline static QColorVector interpolate(const QColorVector &a, const QColorVector &b, float t)
    {
        return a + (b - a) * t; // faster than std::lerp by assuming no super large or non-number floats
    }
    inline static void interpolateIn(QColorVector &a, const QColorVector &b, float t)
    {
        a += (b - a) * t;
    }
public:
    qsizetype gridPointsX = 0;
    qsizetype gridPointsY = 0;
    qsizetype gridPointsZ = 0;
    QList<QColorVector> table;

    bool isEmpty() const { return table.isEmpty(); }

    QColorVector apply(const QColorVector &v) const
    {
        Q_ASSERT(table.size() == gridPointsX * gridPointsY * gridPointsZ);
        const float x = std::clamp(v.x, 0.0f, 1.0f) * (gridPointsX - 1);
        const float y = std::clamp(v.y, 0.0f, 1.0f) * (gridPointsY - 1);
        const float z = std::clamp(v.z, 0.0f, 1.0f) * (gridPointsZ - 1);
        // Variables for trilinear interpolation
        const qsizetype lox = static_cast<qsizetype>(std::floor(x));
        const qsizetype hix = std::min(lox + 1, gridPointsX - 1);
        const qsizetype loy = static_cast<qsizetype>(std::floor(y));
        const qsizetype hiy = std::min(loy + 1, gridPointsY - 1);
        const qsizetype loz = static_cast<qsizetype>(std::floor(z));
        const qsizetype hiz = std::min(loz + 1, gridPointsZ - 1);
        const float fracx = x - static_cast<float>(lox);
        const float fracy = y - static_cast<float>(loy);
        const float fracz = z - static_cast<float>(loz);
        QColorVector tmp[4];
        auto index = [&](qsizetype x, qsizetype y, qsizetype z) { return x * gridPointsZ * gridPointsY + y * gridPointsZ + z; };
        // interpolate over z
        tmp[0] = interpolate(table[index(lox, loy, loz)],
                             table[index(lox, loy, hiz)], fracz);
        tmp[1] = interpolate(table[index(lox, hiy, loz)],
                             table[index(lox, hiy, hiz)], fracz);
        tmp[2] = interpolate(table[index(hix, loy, loz)],
                             table[index(hix, loy, hiz)], fracz);
        tmp[3] = interpolate(table[index(hix, hiy, loz)],
                             table[index(hix, hiy, hiz)], fracz);
        // interpolate over y
        interpolateIn(tmp[0], tmp[1], fracy);
        interpolateIn(tmp[2], tmp[3], fracy);
        // interpolate over x
        interpolateIn(tmp[0], tmp[2], fracx);
        return tmp[0];
    }
};

QT_END_NAMESPACE

#endif // QCOLORCLUT_H
