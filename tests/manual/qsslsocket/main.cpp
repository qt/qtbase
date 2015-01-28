/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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
#include <QtNetwork/qsslsocket.h>
#include <QtTest/QtTest>

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

    QList<QString> hosts = QList<QString>()
            << QStringLiteral("www.google.com")
            << QStringLiteral("www.facebook.com")
            << QStringLiteral("www.twitter.com")
            << QStringLiteral("graph.facebook.com")
            << QStringLiteral("api.twitter.com");

    foreach (QString host, hosts) {
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
        tag.append("-spdy/3");
        QTest::newRow(tag)
                << true
                << host
                << (QList<QByteArray>() << QSslConfiguration::NextProtocolSpdy3_0)
                << QByteArray(QSslConfiguration::NextProtocolSpdy3_0)
                << QSslConfiguration::NextProtocolNegotiationNegotiated;

        tag = host.toLocal8Bit();
        tag.append("-spdy/3-and-http/1.1");
        QTest::newRow(tag)
                << true
                << host
                << (QList<QByteArray>() << QSslConfiguration::NextProtocolSpdy3_0 << QSslConfiguration::NextProtocolHttp1_1)
                << QByteArray(QSslConfiguration::NextProtocolSpdy3_0)
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
