// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QLabel>
#include <QMovie>

namespace src_gui_image_qmovie {

void wrapper0() {


//! [0]
QLabel label;
QMovie *movie = new QMovie("animations/fire.gif");

label.setMovie(movie);
movie->start();
//! [0]

} // wrapper0


void wrapper1() {

//! [1]
QMovie movie("racecar.gif");
movie.setSpeed(200); // 2x speed
//! [1]

} // wrapper1
} // src_gui_image_qmovie
