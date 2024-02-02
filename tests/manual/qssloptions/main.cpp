// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qsslconfiguration.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <stdio.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 3) {
        QTextStream out(stdout);
        out << "Usage: " << argv[0] << " host port [options]" << Qt::endl;
        out << "The options can be one or more of the following:" << Qt::endl;
        out << "enable_empty_fragments" << Qt::endl;
        out << "disable_session_tickets" << Qt::endl;
        out << "disable_compression" << Qt::endl;
        out << "disable_sni" << Qt::endl;
        out << "enable_unsafe_reneg" << Qt::endl;
        return 1;
    }

    QString host = QString::fromLocal8Bit(argv[1]);
    int port = QString::fromLocal8Bit(argv[2]).toInt();

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();

    for (int i=3; i < argc; i++) {
        QString option = QString::fromLocal8Bit(argv[i]);

        if (option == QStringLiteral("enable_empty_fragments"))
            config.setSslOption(QSsl::SslOptionDisableEmptyFragments, false);
        else if (option == QStringLiteral("disable_session_tickets"))
            config.setSslOption(QSsl::SslOptionDisableSessionTickets, true);
        else if (option == QStringLiteral("disable_compression"))
            config.setSslOption(QSsl::SslOptionDisableCompression, true);
        else if (option == QStringLiteral("disable_sni"))
            config.setSslOption(QSsl::SslOptionDisableServerNameIndication, true);
        else if (option == QStringLiteral("enable_unsafe_reneg"))
            config.setSslOption(QSsl::SslOptionDisableLegacyRenegotiation, false);
    }

    QSslConfiguration::setDefaultConfiguration(config);

    QSslSocket socket;
    //socket.setSslConfiguration(config);
    socket.connectToHostEncrypted(host, port);

    if ( !socket.waitForEncrypted() ) {
        qDebug() << socket.errorString();
        return 1;
    }

    return 0;
}
