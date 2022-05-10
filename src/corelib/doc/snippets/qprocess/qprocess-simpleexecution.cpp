// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
//! [0]
    QObject *parent;
//! [0]
    parent = &app;

//! [1]
    QString program = "./path/to/Qt/examples/widgets/analogclock";
//! [1]
    program = "./../../../../examples/widgets/analogclock/analogclock";

//! [2]
    QStringList arguments;
    arguments << "-style" << "fusion";

    QProcess *myProcess = new QProcess(parent);
    myProcess->start(program, arguments);
//! [2]

    return app.exec();
}
