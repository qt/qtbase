// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QLabel>
#include <QPointer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

//! [0]
    QPointer<QLabel> label = new QLabel;
    label->setText("&Status:");
//! [0]

//! [1]
    if (label)
//! [1] //! [2]
        label->show();
//! [2]
    return 0;
}
