// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QBrush>

namespace gradientcreationsnippet {

void wrapper() {

//! [0]
    QRadialGradient gradient(50, 50, 50, 50, 50);
    gradient.setColorAt(0, QColor::fromRgbF(0, 1, 0, 1));
    gradient.setColorAt(1, QColor::fromRgbF(0, 0, 0, 0));

    QBrush brush(gradient);
//! [0]

} // wrapper

} // gradientcreationsnippet
