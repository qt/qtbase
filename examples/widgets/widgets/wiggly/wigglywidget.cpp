// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "wigglywidget.h"

#include <QFontMetrics>
#include <QPainter>
#include <QTimerEvent>

//! [0]
WigglyWidget::WigglyWidget(QWidget *parent)
    : QWidget(parent), step(0)
{
    setBackgroundRole(QPalette::Midlight);
    setAutoFillBackground(true);

    QFont newFont = font();
    newFont.setPointSize(newFont.pointSize() + 20);
    setFont(newFont);

    timer.start(60, this);
}
//! [0]

//! [1]
void WigglyWidget::paintEvent(QPaintEvent * /* event */)
//! [1] //! [2]
{
    static constexpr int sineTable[16] = {
        0, 38, 71, 92, 100, 92, 71, 38, 0, -38, -71, -92, -100, -92, -71, -38
    };

    QFontMetrics metrics(font());
    int x = (width() - metrics.horizontalAdvance(text)) / 2;
    int y = (height() + metrics.ascent() - metrics.descent()) / 2;
    QColor color;
//! [2]

//! [3]
    QPainter painter(this);
//! [3] //! [4]
    for (int i = 0; i < text.size(); ++i) {
        int index = (step + i) % 16;
        color.setHsv((15 - index) * 16, 255, 191);
        painter.setPen(color);
        painter.drawText(x, y - ((sineTable[index] * metrics.height()) / 400),
                         QString(text[i]));
        x += metrics.horizontalAdvance(text[i]);
    }
}
//! [4]

//! [5]
void WigglyWidget::timerEvent(QTimerEvent *event)
//! [5] //! [6]
{
    if (event->timerId() == timer.timerId()) {
        ++step;
        update();
    } else {
        QWidget::timerEvent(event);
    }
//! [6]
}
