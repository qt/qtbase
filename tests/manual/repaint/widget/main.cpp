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

