// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
