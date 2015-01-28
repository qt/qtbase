/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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



