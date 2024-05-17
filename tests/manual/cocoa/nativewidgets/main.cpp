// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore>
#include <QtWidgets>

class ColorWidget : public QWidget
{
    QColor color;
    int s;
    int v;

    void changeColor()
    {
        color.setHsv(QRandomGenerator::global()->bounded(50) + 200, s, s);
    }

public:
    ColorWidget()
    {
        s = 150;
        v = 150;
        changeColor();
        setMouseTracking(true);
    }

    void mousePressEvent(QMouseEvent *)
    {
        changeColor();
        update();
    }

    void mouseMoveEvent(QMouseEvent *)
    {
        changeColor();
        update();
    }

    void enterEvent(QEnterEvent *)
    {
        s = 200;
        v = 200;
        changeColor();
        update();
    }

    void leaveEvent(QEvent *)
    {
        s = 75;
        v = 75;
        changeColor();
        update();
    }

    void paintEvent(QPaintEvent *){
        QPainter p(this);
        p.fillRect(QRect(QPoint(0, 0), size()), QBrush(color));
    }
};


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ColorWidget window;

    QWidget *w1 = new ColorWidget;
    QWidget *w2 = new ColorWidget;
    QWidget *w3 = new ColorWidget;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(w1);
    layout->addWidget(w2);
    layout->addWidget(w3);

    QWidget *w3_1 = new ColorWidget;
    QWidget *w3_2 = new ColorWidget;
    QWidget *w3_3 = new ColorWidget;

    QVBoxLayout *layout3 = new QVBoxLayout;
    layout3->setMargin(0);
    layout3->addWidget(w3_1);
    layout3->addWidget(w3_2);
    layout3->addWidget(w3_3);
    w3->setLayout(layout3);

    window.setLayout(layout);

    bool native = 1;

    if (native) {
        w1->winId();
        w2->winId();
        w3->winId();

        w3_1->winId();
        w3_2->winId();
        w3_3->winId();
    }

    window.resize(640, 480);
    window.show();

    return app.exec();
}



