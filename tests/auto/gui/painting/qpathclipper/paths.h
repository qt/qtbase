/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#ifndef PATHS_H
#define PATHS_H

#include <QPainterPath>

namespace Paths
{
    QPainterPath rect();
    QPainterPath heart();
    QPainterPath body();
    QPainterPath mailbox();
    QPainterPath deer();
    QPainterPath fire();
    QPainterPath lips();

    QPainterPath bezier1();
    QPainterPath bezier2();
    QPainterPath bezier3();
    QPainterPath bezier4();

    QPainterPath random1();
    QPainterPath random2();

    QPainterPath heart2();
    QPainterPath rect2();
    QPainterPath rect3();
    QPainterPath rect4();
    QPainterPath rect5();
    QPainterPath rect6();

    QPainterPath simpleCurve();
    QPainterPath simpleCurve2();
    QPainterPath simpleCurve3();

    QPainterPath frame1();
    QPainterPath frame2();
    QPainterPath frame3();
    QPainterPath frame4();

    QPainterPath triangle1();
    QPainterPath triangle2();

    QPainterPath node();
    QPainterPath interRect();

    QPainterPath bezierFlower();
    QPainterPath clover();
    QPainterPath ellipses();
    QPainterPath windingFill();
    QPainterPath oddEvenFill();
    QPainterPath squareWithHole();
    QPainterPath circleWithHole();
    QPainterPath bezierQuadrant();
}
#endif
