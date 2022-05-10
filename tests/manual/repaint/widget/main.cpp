// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

