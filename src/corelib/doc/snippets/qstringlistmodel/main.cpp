// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>

class Widget : public QWidget
{
public:
    Widget(QWidget *parent = nullptr);
};

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
//! [0]
    QStringListModel *model = new QStringListModel();
    QStringList list;
    list << "a" << "b" << "c";
    model->setStringList(list);
//! [0]
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Widget widget;
    widget.show();
    return app.exec();
}
