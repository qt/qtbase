// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QMainWindow>
#include <QPaintEvent>
#include <QDebug>

class MyWidget : public QWidget
{
public:
    MyWidget() : QWidget()
    {
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_StaticContents);
    }

protected:
    void paintEvent(QPaintEvent *e) { qDebug() << e->rect(); }
};

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    MyWidget w;
    w.show();
    return a.exec();
}
