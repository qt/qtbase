/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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



