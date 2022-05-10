// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QRegion>

namespace src_gui_painting_qregion_unix {

void wrapper() {
//! [0]
QRegion r1(10, 10, 20, 20);
r1.isEmpty();               // false

QRegion r3;
r3.isEmpty();               // true

QRegion r2(40, 40, 20, 20);
r3 = r1.intersected(r2);    // r3: intersection of r1 and r2
r3.isEmpty();               // true

r3 = r1.united(r2);         // r3: union of r1 and r2
r3.isEmpty();               // false
//! [0]

} // wrapper
} // src_gui_painting_qregion_unix
