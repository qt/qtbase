// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QImage>
#include <QImageWriter>

namespace src_gui_image_qimagewriter {

void wrapper0() {

//! [0]
QImageWriter writer;
writer.setFormat("png"); // same as writer.setFormat("PNG");
//! [0]

} // wrapper0


void wrapper1() {

//! [1]
QImage image("some/image.jpeg");
QImageWriter writer("images/outimage.png", "png");
writer.setText("Author", "John Smith");
writer.write(image);
//! [1]

} // wrapper1


void wrapper2() {
QString fileName;

//! [2]
QImageWriter writer(fileName);
if (writer.supportsOption(QImageIOHandler::Description))
    writer.setText("Author", "John Smith");
//! [2]

} // wrapper 2


void wrapper3() {
QImage image;

//! [3]
QImageWriter writer("some/image.dds");
if (writer.supportsOption(QImageIOHandler::SubType))
    writer.setSubType("A8R8G8B8");
writer.write(image);
//! [3]

} // wrapper3
} // src_gui_image_qimagewriter
