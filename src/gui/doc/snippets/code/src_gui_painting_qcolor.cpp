// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QPainter>

namespace src_gui_painting_qcolor {
int width() { return 1; };
int height() { return 1; };
QPainter painter;
void wrapper() {


//! [0]
// Specify semi-transparent red
painter.setBrush(QColor(255, 0, 0, 127));
painter.drawRect(0, 0, width() / 2, height());

// Specify semi-transparent blue
painter.setBrush(QColor(0, 0, 255, 127));
painter.drawRect(0, 0, width(), height() / 2);
//! [0]

//! [QRgb]
const QRgb rgb1 = 0x88112233;
const QRgb rgb2 = QColor("red").rgb();
const QRgb rgb3 = qRgb(qRed(rgb1), qGreen(rgb2), qBlue(rgb2));
const QRgb rgb4 = qRgba(qRed(rgb1), qGreen(rgb2), qBlue(rgb2), qAlpha(rgb1));
//! [QRgb]
Q_UNUSED(rgb3);
Q_UNUSED(rgb4);

} // wrapper
} // src_gui_painting_qcolor
