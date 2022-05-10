// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QImage>
#include <QImageReader>

namespace src_gui_image_qimagereader {
void wrapper0() {

//! [0]
QImageReader reader;
reader.setFormat("png"); // same as reader.setFormat("PNG");
//! [0]

} // wrapper0


//! [1]
QImageReader reader("image.png");
// reader.format() == "png"
//! [1]


void wrapper1() {

//! [2]
QImage icon(64, 64, QImage::Format_RGB32);
QImageReader reader("icon_64x64.bmp");
if (reader.read(&icon)) {
    // Display icon
}
//! [2]

} // wrapper1


void wrapper2() {

//! [3]
QImageReader reader(":/image.png");
if (reader.supportsOption(QImageIOHandler::Size))
    qDebug() << "Size:" << reader.size();
//! [3]

} // wrapper2
} // src_gui_image_qimagereader
