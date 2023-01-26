// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "analogclock.h"

#include <QPainter>
#include <QTime>
#include <QTimer>

//! [0] //! [1]
AnalogClock::AnalogClock(QWidget *parent)
//! [0] //! [2]
    : QWidget(parent)
//! [2] //! [3]
{
//! [3] //! [4]
    QTimer *timer = new QTimer(this);
//! [4] //! [5]
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&AnalogClock::update));
//! [5] //! [6]
    timer->start(1000);
//! [6]

    setWindowTitle(tr("Analog Clock"));
    resize(200, 200);
//! [7]
}
//! [1] //! [7]

//! [8] //! [9]
void AnalogClock::paintEvent(QPaintEvent *)
//! [8] //! [10]
{
    static const QPoint hourHand[3] = {
        QPoint(7, 8),
        QPoint(-7, 8),
        QPoint(0, -40)
    };
    static const QPoint minuteHand[3] = {
        QPoint(7, 8),
        QPoint(-7, 8),
        QPoint(0, -70)
    };

    QColor hourColor(127, 0, 127);
    QColor minuteColor(0, 127, 127, 191);

    int side = qMin(width(), height());
//! [10]

//! [11]
    QPainter painter(this);
//! [11] //! [12]
    painter.setRenderHint(QPainter::Antialiasing);
//! [12] //! [13]
    painter.translate(width() / 2, height() / 2);
//! [13] //! [14]
    painter.scale(side / 200.0, side / 200.0);
//! [9] //! [14]

//! [15]
    painter.setPen(Qt::NoPen);
//! [15] //! [16]
    painter.setBrush(hourColor);
//! [16]
//! [18]
    QTime time = QTime::currentTime();
//! [17]
    painter.save();
//! [17] //! [19]
    painter.rotate(30.0 * ((time.hour() + time.minute() / 60.0)));
    painter.drawConvexPolygon(hourHand, 3);
    painter.restore();
//! [18] //! [19]

//! [20]
    painter.setPen(hourColor);
//! [20] //! [21]

    for (int i = 0; i < 12; ++i) {
        painter.drawLine(88, 0, 96, 0);
        painter.rotate(30.0);
    }
//! [21]

//! [22]
    painter.setPen(Qt::NoPen);
//! [22] //! [23]
    painter.setBrush(minuteColor);

//! [24]
    painter.save();
    painter.rotate(6.0 * (time.minute() + time.second() / 60.0));
    painter.drawConvexPolygon(minuteHand, 3);
    painter.restore();
//! [23] //! [24]

//! [25]
    painter.setPen(minuteColor);
//! [25] //! [26]

//! [27]
    for (int j = 0; j < 60; ++j) {
        if ((j % 5) != 0)
            painter.drawLine(92, 0, 96, 0);
        painter.rotate(6.0);
    }
//! [27]
}
//! [26]
