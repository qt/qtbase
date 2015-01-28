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

#include <QtTest/QtTest>

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
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
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
     QList<QSslCertificate> list = QSslSocket::systemCaCertificates();
  }
}

QTEST_MAIN(tst_QSslSocket)
#include "tst_qsslsocket.moc"
