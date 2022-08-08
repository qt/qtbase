// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtCore>

// This test shows how to use asyncify to enable blocking the main
// thread on QEventLoop::exec(), while event processing continues.
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QTimer::singleShot(1000, []() {

        QEventLoop loop;
        QTimer::singleShot(2000, [&loop]() {
            qDebug() << "Calling QEventLoop::quit()";
            loop.quit();
        });

        qDebug() << "Calling QEventLoop::exec()";
        loop.exec();
        qDebug() << "Returned from QEventLoop::exec()";
    });

    app.exec();
}
