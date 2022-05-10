// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QLabel>
#include <QPushButton>

#include "qtcast.h"

MyWidget::MyWidget()
{
//! [0]
    QObject *obj = new MyWidget;
//! [0]

//! [1]
    QWidget *widget = qobject_cast<QWidget *>(obj);
//! [1]

//! [2]
    MyWidget *myWidget = qobject_cast<MyWidget *>(obj);
//! [2]

//! [3]
    QLabel *label = qobject_cast<QLabel *>(obj);
//! [3] //! [4]
    // label is 0
//! [4]

//! [5]
    if (QLabel *label = qobject_cast<QLabel *>(obj)) {
//! [5] //! [6]
        label->setText(tr("Ping"));
    } else if (QPushButton *button = qobject_cast<QPushButton *>(obj)) {
        button->setText(tr("Pong!"));
    }
//! [6]
}

int main()
{
    return 0;
}
