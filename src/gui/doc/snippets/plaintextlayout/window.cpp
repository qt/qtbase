// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QPainter>
#include <QTextLayout>
#include <QWidget>

namespace plaintextlayout {
class Window : public QWidget
{
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QFont font;
    QString text;
};

void Window::paintEvent(QPaintEvent *event)
{

//! [0]
QTextLayout textLayout(text, font);
qreal margin = 10;
qreal radius = qMin(width()/2.0, height()/2.0) - margin;
QFontMetrics fm(font);

qreal lineHeight = fm.height();
qreal y = 0;

textLayout.beginLayout();

while (1) {
    // create a new line
    QTextLine line = textLayout.createLine();
    if (!line.isValid())
        break;

    qreal x1 = qMax(0.0, pow(pow(radius,2)-pow(radius-y,2), 0.5));
    qreal x2 = qMax(0.0, pow(pow(radius,2)-pow(radius-(y+lineHeight),2), 0.5));
    qreal x = qMax(x1, x2) + margin;
    qreal lineWidth = (width() - margin) - x;

    line.setLineWidth(lineWidth);
    line.setPosition(QPointF(x, margin+y));
    y += line.height();
}

textLayout.endLayout();

QPainter painter;
painter.begin(this);
painter.setRenderHint(QPainter::Antialiasing);
painter.fillRect(rect(), Qt::white);
painter.setBrush(QBrush(Qt::black));
painter.setPen(QPen(Qt::black));
textLayout.draw(&painter, QPoint(0,0));

painter.setBrush(QBrush(QColor("#a6ce39")));
painter.setPen(QPen(Qt::black));
painter.drawEllipse(QRectF(-radius, margin, 2*radius, 2*radius));
painter.end();
//! [0]

Q_UNUSED(event);
}
} // plaintextlayout
