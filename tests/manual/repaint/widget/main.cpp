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

#include "../shared/shared.h"

#include <QApplication>
#include <QPushButton>

class Child : public StaticWidget
{
Q_OBJECT
public:
    Child(QWidget *parent)
    :StaticWidget(parent)
    {
        hue = 0;
    }
};

QWidget *c;

class TopLevel : public StaticWidget
{
Q_OBJECT
public:
    TopLevel()
    {
        resizeButton  = new QPushButton("resize", this);
        connect(resizeButton, SIGNAL(clicked()), SLOT(buttonResizeClicked()));

        movebutton  = new QPushButton("move", this);
        connect(movebutton, SIGNAL(clicked()), SLOT(buttonMoveClicked()));
        movebutton->move(70, 0);

        moveResizebutton  = new QPushButton("move + resize", this);
        connect(moveResizebutton, SIGNAL(clicked()), SLOT(buttonMoveResizeClicked()));
        moveResizebutton->move(150, 0);

        scrollbutton  = new QPushButton("scroll", this);
        connect(scrollbutton, SIGNAL(clicked()), SLOT(buttonScrollClicked()));
        scrollbutton->move(280, 0);
    }

public slots:
    void buttonResizeClicked()
    {
        c->resize(c->size() + QSize(15, 15));
        qDebug() << "child new size" << c->size();
    }

    void buttonMoveClicked()
    {
        c->move(c->pos() + QPoint(15, 15));
        qDebug() << "child moved" << c->pos();
    }

    void buttonMoveResizeClicked()
    {
        QRect g = c->geometry();
        g.adjust(15,15,30,30);
        c->setGeometry(g);
        qDebug() << "child moved" << c->pos() << "rezied" << c->size();
    }


    void buttonScrollClicked()
    {
        c->scroll(10, 10);
    }

protected:
    QPushButton * resizeButton;
    QPushButton * movebutton;
    QPushButton * moveResizebutton;
    QPushButton * scrollbutton;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    TopLevel bc;
    bc.resize(500, 500);

    c = new Child(&bc);
    c->move(100, 100);
    c->resize(100, 100);

    QWidget *gc  = new StaticWidget(c);
    gc->move(20, 20);
    gc->resize(50,50);


    bc.show();
    return app.exec();
}

#include "main.moc"

