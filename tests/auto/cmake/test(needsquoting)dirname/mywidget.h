// Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>

namespace Ui
{
class MyWidget;
}

class MyWidget : public QWidget
{
    Q_OBJECT
public:
    MyWidget(QWidget *parent = nullptr);

signals:
    void someSignal();

private:
    Ui::MyWidget *ui;
};

#endif
