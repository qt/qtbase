// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QMessageBox>
#include <QtNetwork>

QT_REQUIRE_CONFIG(ssl);

#include "sslclient.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    if (!QSslSocket::supportsSsl()) {
        QMessageBox::information(nullptr, "Secure Socket Client",
                                 "This system does not support TLS.");
        return -1;
    }

    SslClient client;
    client.show();

    return app.exec();
}
