// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtCore>

// This test shows how to asyncify enables blocking
// the main thread on QEventLoop::exec(), while event
// provessing continues.
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

#ifdef QT_HAVE_EMSCRIPTEN_ASYNCIFY
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
#else
        qDebug() << "This test requires Emscripten asyncify. To enable,"
                 << "configure Qt with -device-option QT_EMSCRIPTEN_ASYNCIFY=1";
#endif

    app.exec();
}
