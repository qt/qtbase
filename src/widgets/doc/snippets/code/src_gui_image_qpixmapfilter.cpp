// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QPixmapColorizeFilter *myFilter = new QPixmapColorFilter;
myFilter->setColor(QColor(128, 0, 0));
myFilter->draw(painter, QPoint(0, 0), originalPixmap);
//! [0]

//! [1]
QPixmapConvolutionFilter *myFilter = new QPixmapConvolutionFilter;
qreal kernel[] = {
     0.0,-1.0, 0.0,
    -1.0, 5.0,-1.0,
     0.0,-1.0, 0.0
    };
myFilter->setConvolutionKernel(kernel, 3, 3);
myFilter->draw(painter, QPoint(0, 0), originalPixmap);
//! [1]

//! [2]
QPixmapDropShadowFilter *myFilter = new QPixmapDropShadowFilter;
myFilter->draw(painter, QPoint(0, 0), originalPixmap);
//! [2]

