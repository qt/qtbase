// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtGui>
namespace image {
void wrapper0() {

//! [0]
QImage image;
QByteArray ba;
QBuffer buffer(&ba);
buffer.open(QIODevice::WriteOnly);
image.save(&buffer, "PNG"); // writes image into ba in PNG format
//! [0]

} // wrapper0


void wrapper1() {

//! [1]
QPixmap pixmap;
QByteArray bytes;
QBuffer buffer(&bytes);
buffer.open(QIODevice::WriteOnly);
pixmap.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format
//! [1]

} // wrapper1

} // image
