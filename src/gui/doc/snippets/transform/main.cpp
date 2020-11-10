// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QtMath>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>


class SimpleTransformation : public QWidget
{
    void paintEvent(QPaintEvent *) override;
};

//! [0]
void SimpleTransformation::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
    painter.drawRect(0, 0, 100, 100);

    painter.rotate(45);

    painter.setFont(QFont("Helvetica", 24));
    painter.setPen(QPen(Qt::black, 1));
    painter.drawText(20, 10, "QTransform");
}
//! [0]

class CombinedTransformation : public QWidget
{
    void paintEvent(QPaintEvent *) override;
};

//! [1]
void CombinedTransformation::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
    painter.drawRect(0, 0, 100, 100);

    QTransform transform;
    transform.translate(50, 50);
    transform.rotate(45);
    transform.scale(0.5, 1.0);
    painter.setTransform(transform);

    painter.setFont(QFont("Helvetica", 24));
    painter.setPen(QPen(Qt::black, 1));
    painter.drawText(20, 10, "QTransform");
}
//! [1]

class BasicOperations : public QWidget
{
    void paintEvent(QPaintEvent *) override;
};

//! [2]
void BasicOperations::paintEvent(QPaintEvent *)
{
    const double a = qDegreesToRadians(45.0);
    double sina = sin(a);
    double cosa = cos(a);

    QTransform scale(0.5, 0, 0, 1.0, 0, 0);
    QTransform rotate(cosa, sina, -sina, cosa, 0, 0);
    QTransform translate(1, 0, 0, 1, 50.0, 50.0);

    QTransform transform = scale * rotate * translate;

    QPainter painter(this);
    painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
    painter.drawRect(0, 0, 100, 100);

    painter.setTransform(transform);

    painter.setFont(QFont("Helvetica", 24));
    painter.setPen(QPen(Qt::black, 1));
    painter.drawText(20, 10, "QTransform");
}
//! [2]

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QWidget widget;

    SimpleTransformation *simpleWidget = new SimpleTransformation;
    CombinedTransformation *combinedWidget = new CombinedTransformation;
    BasicOperations *basicWidget = new BasicOperations;

    QVBoxLayout *layout = new QVBoxLayout(&widget);
    layout->addWidget(simpleWidget);
    layout->addWidget(combinedWidget);
    layout->addWidget(basicWidget);

    widget.show();
    widget.resize(130, 350);
    return app.exec();
}
