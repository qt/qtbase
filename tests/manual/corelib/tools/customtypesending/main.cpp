// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include "message.h"
#include "window.h"

//! [main function]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStringList headers;
    headers << "Subject: Hello World"
            << "From: address@example.com";
    QString body = "This is a test.\r\n";
    Message message(body, headers);

    Window window1;
    window1.setMessage(message);

    Window window2;
    QObject::connect(&window1, &Window::messageSent,
                     &window2, &Window::setMessage);
    QObject::connect(&window2, &Window::messageSent,
                     &window1, &Window::setMessage);
    window1.show();
    window2.show();
    return app.exec();
}
//! [main function]
