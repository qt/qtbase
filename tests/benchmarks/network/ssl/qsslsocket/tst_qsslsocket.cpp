// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qcoreapplication.h>
#include <qsslconfiguration.h>
#include <qsslsocket.h>


#include "../../../../auto/network-settings.h"

class tst_QSslSocket : public QObject
{
    Q_OBJECT

public:
    tst_QSslSocket();
    virtual ~tst_QSslSocket();


public slots:
    void initTestCase();
    void init();
    void cleanup();
private slots:
    void rootCertLoading();
    void systemCaCertificates();
};

tst_QSslSocket::tst_QSslSocket()
{
}

tst_QSslSocket::~tst_QSslSocket()
{
}

void tst_QSslSocket::initTestCase()
{
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
}

void tst_QSslSocket::init()
{
}

void tst_QSslSocket::cleanup()
{
}

//----------------------------------------------------------------------------------

void tst_QSslSocket::rootCertLoading()
{
    QBENCHMARK_ONCE {
        QSslSocket socket;
        socket.connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
        socket.waitForEncrypted();
    }
}

void tst_QSslSocket::systemCaCertificates()
{
  // The results of this test change if the benchmarking system changes too much.
  // Therefore this benchmark is only good for manual regression checking between
  // Qt versions.
  QBENCHMARK_ONCE {
      QList<QSslCertificate> list = QSslConfiguration::defaultConfiguration().systemCaCertificates();
  }
}

QTEST_MAIN(tst_QSslSocket)
#include "tst_qsslsocket.moc"
