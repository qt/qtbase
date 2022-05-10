// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QBitmap>
#include <QPixmap>

namespace src_gui_image_qpixmap {

void wrapper0() {

//! [1]
QPixmap myPixmap;
myPixmap.setMask(myPixmap.createHeuristicMask());
//! [1]

} // wrapper0


void wrapper1() {

//! [2]
QPixmap pixmap("background.png");
QRegion exposed;
pixmap.scroll(10, 10, pixmap.rect(), &exposed);
//! [2]

} // wrapper1
} // src_gui_image_qpixmap
