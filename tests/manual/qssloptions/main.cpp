/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtNetwork/qsslconfiguration.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <stdio.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 3) {
        QTextStream out(stdout);
        out << "Usage: " << argv[0] << " host port [options]" << endl;
        out << "The options can be one or more of the following:" << endl;
        out << "enable_empty_fragments" << endl;
        out << "disable_session_tickets" << endl;
        out << "disable_compression" << endl;
        out << "disable_sni" << endl;
        out << "enable_unsafe_reneg" << endl;
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
