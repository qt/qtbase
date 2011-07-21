/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "vibrationsurface.h"
#include <QtGui/QPainter>
#include <QtCore/QLine>
#include <QtGui/QMouseEvent>
#include <QtGui/QScreen>
#include <QtCore/QRect>
#include <QtGui/QColor>

#include "xqvibra.h"

//! [4]
const int NumberOfLevels = 10;
const double IntensityFactor = XQVibra::MaxIntensity / NumberOfLevels;
//! [4]

VibrationSurface::VibrationSurface(XQVibra *vibra, QWidget *parent) :
    QWidget(parent),
    vibra(vibra),
    lastIntensity(0)
{
}

//! [0]
void VibrationSurface::mousePressEvent(QMouseEvent *event)
{
    applyIntensity(event->x(), event->y());
    vibra->start();
}
//! [0]

//! [1]
void VibrationSurface::mouseMoveEvent(QMouseEvent *event)
{
    applyIntensity(event->x(), event->y());
}
//! [1]

//! [2]
void VibrationSurface::mouseReleaseEvent(QMouseEvent *)
{
    vibra->stop();
}
//! [2]

//! [5]
void VibrationSurface::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QRect rect = geometry();
    int dx = 0, dy = 0;

    if (height() > width()) {
        dy = height() / NumberOfLevels;
        rect.setHeight(dy);
    } else {
        dx = width() / NumberOfLevels;
        rect.setWidth(dx);
    }
//! [5]
//! [6] 
    for (int i = 0; i < NumberOfLevels; i++) {
        int x = i * dx;
        int y = i * dy;
        int intensity = getIntensity(x, y);
        QColor color = QColor(40, 80, 10).lighter(100 + intensity);

        rect.moveTo(x, y);
        painter.fillRect(rect, color);
        painter.setPen(color.darker());
        painter.drawText(rect, Qt::AlignCenter, QString::number(intensity));
    }
}
//! [6] 

//! [7] 
int VibrationSurface::getIntensity(int x, int y)
{
    int level;
    int coord;

    if (height() > width()) {
        level = height() / NumberOfLevels;
        coord = y;
    } else {
        level = width() / NumberOfLevels;
        coord = x;
    }

    if (level == 0) {
        return 0;
    }
//! [7] 
//! [8] 
    int intensity = (coord / level + 1) * IntensityFactor;

    if (intensity < 0) {
        intensity = 0;
    } else if (intensity > XQVibra::MaxIntensity) {
        intensity = XQVibra::MaxIntensity;
    }

    return intensity;
}
//! [8] 

//! [3]
void VibrationSurface::applyIntensity(int x, int y)
{
    int intensity = getIntensity(x, y);

    if (intensity != lastIntensity) {
        vibra->setIntensity(intensity);
        lastIntensity = intensity;
    }
}
//! [3]

