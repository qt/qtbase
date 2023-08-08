// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qsslsocket.h>
#include <QTest>

#ifndef QT_NO_SSL
Q_DECLARE_METATYPE(QSslConfiguration::NextProtocolNegotiationStatus)
#endif

class tst_QSslSocket : public QObject
{
    Q_OBJECT

#ifndef QT_NO_SSL
private slots:
    void nextProtocolNegotiation_data();
    void nextProtocolNegotiation();
#endif // QT_NO_SSL
};

#ifndef QT_NO_SSL
void tst_QSslSocket::nextProtocolNegotiation_data()
{
    QTest::addColumn<bool>("setConfiguration");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QList<QByteArray> >("allowedProtocols");
    QTest::addColumn<QByteArray>("expectedProtocol");
    QTest::addColumn<QSslConfiguration::NextProtocolNegotiationStatus>("expectedStatus");

    const QString hosts[] = {
        QStringLiteral("www.google.com"),
        QStringLiteral("www.facebook.com"),
        QStringLiteral("www.twitter.com"),
        QStringLiteral("graph.facebook.com"),
        QStringLiteral("api.twitter.com"),
    };

    for (const QString &host : hosts) {
        QByteArray tag = host.toLocal8Bit();
        tag.append("-none");
        QTest::newRow(tag)
                << false
                << host
                << QList<QByteArray>()
                << QByteArray()
                << QSslConfiguration::NextProtocolNegotiationNone;

        tag = host.toLocal8Bit();
        tag.append("-none-explicit");
        QTest::newRow(tag)
                << true
                << host
                << QList<QByteArray>()
                << QByteArray()
                << QSslConfiguration::NextProtocolNegotiationNone;

        tag = host.toLocal8Bit();
        tag.append("-http/1.1");
        QTest::newRow(tag)
                << true
                << host
                << (QList<QByteArray>() << QSslConfiguration::NextProtocolHttp1_1)
                << QByteArray(QSslConfiguration::NextProtocolHttp1_1)
                << QSslConfiguration::NextProtocolNegotiationNegotiated;

        tag = host.toLocal8Bit();
        tag.append("-h2");
        QTest::newRow(tag)
                << true
                << host
                << (QList<QByteArray>() << QSslConfiguration::ALPNProtocolHTTP2)
                << QByteArray(QSslConfiguration::ALPNProtocolHTTP2)
                << QSslConfiguration::NextProtocolNegotiationNegotiated;

        tag = host.toLocal8Bit();
        tag.append("-h2-and-http/1.1");
        QTest::newRow(tag)
                << true
                << host
                << (QList<QByteArray>() << QSslConfiguration::ALPNProtocolHTTP2 << QSslConfiguration::NextProtocolHttp1_1)
                << QByteArray(QSslConfiguration::ALPNProtocolHTTP2)
                << QSslConfiguration::NextProtocolNegotiationNegotiated;
    }
}

void tst_QSslSocket::nextProtocolNegotiation()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocket socket;

    QFETCH(bool, setConfiguration);

    if (setConfiguration) {
        QSslConfiguration conf = socket.sslConfiguration();
        QFETCH(QList<QByteArray>, allowedProtocols);
        conf.setAllowedNextProtocols(allowedProtocols);
        socket.setSslConfiguration(conf);
    }

    QFETCH(QString, host);

    socket.connectToHostEncrypted(host, 443);
    socket.ignoreSslErrors();

    QVERIFY(socket.waitForEncrypted(10000));

    QFETCH(QByteArray, expectedProtocol);
    QCOMPARE(socket.sslConfiguration().nextNegotiatedProtocol(), expectedProtocol);

    QFETCH(QSslConfiguration::NextProtocolNegotiationStatus, expectedStatus);
    QCOMPARE(socket.sslConfiguration().nextProtocolNegotiationStatus(), expectedStatus);

    socket.disconnectFromHost();
    QVERIFY(socket.waitForDisconnected());

}

#endif // QT_NO_SSL

QTEST_MAIN(tst_QSslSocket)

#include "main.moc"
