/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "utils.h"

#include <assert.h>
#include <qglobal.h>

#include "qnum.h"

#define FloatToXFixed(i) (int)((i) * 65536)
#define IntToXFixed(i) ((i) << 16)

static double compute_x_at(XFixed y, XPointFixed p1, XPointFixed p2)
{
    double d = XFixedToDouble(p2.x - p1.x);
    return
        XFixedToDouble(p1.x) + d*XFixedToDouble(y - p1.y)/XFixedToDouble(p2.y - p1.y);
}

double compute_area(XTrapezoid *trap)
{
    double x1 = compute_x_at(trap->top, trap->left.p1, trap->left.p2);
    double x2 = compute_x_at(trap->top, trap->right.p1, trap->right.p2);
    double x3 = compute_x_at(trap->bottom, trap->left.p1, trap->left.p2);
    double x4 = compute_x_at(trap->bottom, trap->right.p1, trap->right.p2);

    double top = XFixedToDouble(trap->top);
    double bottom = XFixedToDouble(trap->bottom);
    double h = bottom - top;

    double top_base = x2 - x1;
    double bottom_base = x4 - x3;

    if ((top_base < 0 && bottom_base > 0)
        || (top_base > 0 && bottom_base < 0)) {
        double y0 = top_base*h/(top_base - bottom_base) + top;
        double area = qAbs(top_base * (y0 - top) / 2.);
        area += qAbs(bottom_base * (bottom - y0) /2.);
        return area;
    }


    return 0.5 * h * qAbs(top_base + bottom_base);
}

double compute_area_for_x(const QVector<XTrapezoid> &traps)
{
    double area = 0;

    for (int i = 0; i < traps.size(); ++i) {
        XTrapezoid trap = traps[i];
        area += compute_area(&trap);
    }
    return area;
}
