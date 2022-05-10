// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtWidgets>

namespace picture {
void wrapper0()
{
//! [0]
QPicture picture;
QPainter painter;
painter.begin(&picture);           // paint in picture
painter.drawEllipse(10,20, 80,70); // draw an ellipse
painter.end();                     // painting done
picture.save("drawing.pic");       // save picture
//! [0]

} // wrapper0


void wrapper1() {
QImage myImage;

//! [1]
QPicture picture;
picture.load("drawing.pic");           // load picture
QPainter painter;
painter.begin(&myImage);               // paint in myImage
painter.drawPicture(0, 0, picture);    // draw the picture at (0,0)
painter.end();                         // painting done
//! [1]

} // wrapper1
} // picture
