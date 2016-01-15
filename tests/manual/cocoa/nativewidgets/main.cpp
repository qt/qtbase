/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtWidgets>

class ColorWidget : public QWidget
{
    QColor color;
    int s;
    int v;

    void changeColor()
    {
        color.setHsv((qreal(qrand()) / RAND_MAX) * 50 + 200, s, s);
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

    void enterEvent(QEvent *)
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



