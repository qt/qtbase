// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QLinearGradient>

namespace brush {
void wrapper() {


//! [0]
QLinearGradient linearGrad(QPointF(100, 100), QPointF(200, 200));
linearGrad.setColorAt(0, Qt::black);
linearGrad.setColorAt(1, Qt::white);
//! [0]


//! [1]
QRadialGradient radialGrad(QPointF(100, 100), 100);
radialGrad.setColorAt(0, Qt::red);
radialGrad.setColorAt(0.5, Qt::blue);
radialGrad.setColorAt(1, Qt::green);
//! [1]

} // wrapper
} // brush
