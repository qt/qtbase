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
