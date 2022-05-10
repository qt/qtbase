// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <qsslcipher.h>

class tst_QSslCipher : public QObject
{
    Q_OBJECT

#ifndef QT_NO_SSL

private slots:
    void constructing();

#endif // QT_NO_SSL
};

#ifndef QT_NO_SSL

void tst_QSslCipher::constructing()
{
    QSslCipher cipher;

    QVERIFY(cipher.isNull());
    QCOMPARE(cipher.name(), QString());
    QCOMPARE(cipher.supportedBits(), 0);
    QCOMPARE(cipher.usedBits(), 0);
    QCOMPARE(cipher.keyExchangeMethod(), QString());
    QCOMPARE(cipher.authenticationMethod(), QString());
    QCOMPARE(cipher.protocolString(), QString());
    QCOMPARE(cipher.protocol(), QSsl::UnknownProtocol);
}

#endif // QT_NO_SSL

QTEST_MAIN(tst_QSslCipher)
#include "tst_qsslcipher.moc"
