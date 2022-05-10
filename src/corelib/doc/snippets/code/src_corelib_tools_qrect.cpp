// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QRect r1(100, 200, 11, 16);
QRect r2(QPoint(100, 200), QSize(11, 16));
//! [0]


//! [1]
QRectF r1(100.0, 200.1, 11.2, 16.3);
QRectF r2(QPointF(100.0, 200.1), QSizeF(11.2, 16.3));
//! [1]

//! [2]
QRect r = {15, 51, 42, 24};
r = r.transposed(); // r == {15, 51, 24, 42}
//! [2]

//! [3]
QRectF r = {1.5, 5.1, 4.2, 2.4};
r = r.transposed(); // r == {1.5, 5.1, 2.4, 4.2}
//! [3]
