// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QImage>
#include <QRgb>

namespace src_gui_image_qimage {
void wrapper0() {
//! [0]
QImage image(3, 3, QImage::Format_RGB32);
QRgb value;

value = qRgb(189, 149, 39); // 0xffbd9527
image.setPixel(1, 1, value);

value = qRgb(122, 163, 39); // 0xff7aa327
image.setPixel(0, 1, value);
image.setPixel(1, 0, value);

value = qRgb(237, 187, 51); // 0xffedba31
image.setPixel(2, 1, value);
//! [0]

} // wrapper0
void wrapper1() {


//! [1]
QImage image(3, 3, QImage::Format_Indexed8);
QRgb value;

value = qRgb(122, 163, 39); // 0xff7aa327
image.setColor(0, value);

value = qRgb(237, 187, 51); // 0xffedba31
image.setColor(1, value);

value = qRgb(189, 149, 39); // 0xffbd9527
image.setColor(2, value);

image.setPixel(0, 1, 0);
image.setPixel(1, 0, 0);
image.setPixel(1, 1, 2);
image.setPixel(2, 1, 1);
//! [1]


//! [2]
static const char * const start_xpm[] = {
    "16 15 8 1",
    "a c #cec6bd",
    // etc.
};
//! [2]


//! [scanLine]
for (int y = 0; y < image.height(); ++y) {
    QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
    for (int x = 0; x < image.width(); ++x) {
        QRgb &rgb = line[x];
        rgb = qRgba(qRed(rgb), qGreen(0), qBlue(rgb), qAlpha(rgb));
    }
}
//! [scanLine]

Q_UNUSED(start_xpm);
} // wrapper1
} // src_gui_image_qimage
