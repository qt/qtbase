/****************************************************************************
**
** Copyright (C) 2013 Ruslan Nigmatullin <euroelessar@yandex.ru>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

class tst_QMessageAuthenticationCode : public QObject
{
    Q_OBJECT
private slots:
    void result_data();
    void result();
    void result_incremental_data();
    void result_incremental();
};

Q_DECLARE_METATYPE(QCryptographicHash::Algorithm)

void tst_QMessageAuthenticationCode::result_data()
{
    QTest::addColumn<QCryptographicHash::Algorithm>("algo");
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("message");
    QTest::addColumn<QByteArray>("code");

    // Empty values
    QTest::newRow("md5-empty") << QCryptographicHash::Md5
                               << QByteArray()
                               << QByteArray()
                               << QByteArray::fromHex("74e6f7298a9c2d168935f58c001bad88");
    QTest::newRow("sha1-empty") << QCryptographicHash::Sha1
                                << QByteArray()
                                << QByteArray()
                                << QByteArray::fromHex("fbdb1d1b18aa6c08324b7d64b71fb76370690e1d");
    QTest::newRow("sha256-empty") << QCryptographicHash::Sha256
                                  << QByteArray()
                                  << QByteArray()
                                  << QByteArray::fromHex("b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad");

    // Some not-empty
    QTest::newRow("md5") << QCryptographicHash::Md5
                         << QByteArray("key")
                         << QByteArray("The quick brown fox jumps over the lazy dog")
                         << QByteArray::fromHex("80070713463e7749b90c2dc24911e275");
    QTest::newRow("sha1") << QCryptographicHash::Sha1
                          << QByteArray("key")
                          << QByteArray("The quick brown fox jumps over the lazy dog")
                          << QByteArray::fromHex("de7c9b85b8b78aa6bc8a7a36f70a90701c9db4d9");
    QTest::newRow("sha256") << QCryptographicHash::Sha256
                            << QByteArray("key")
                            << QByteArray("The quick brown fox jumps over the lazy dog")
                            << QByteArray::fromHex("f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");

    // Some from rfc-2104
    QTest::newRow("rfc-md5-1") << QCryptographicHash::Md5
                               << QByteArray::fromHex("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b")
                               << QByteArray("Hi There")
                               << QByteArray::fromHex("9294727a3638bb1c13f48ef8158bfc9d");
    QTest::newRow("rfc-md5-2") << QCryptographicHash::Md5
                               << QByteArray("Jefe")
                               << QByteArray("what do ya want for nothing?")
                               << QByteArray::fromHex("750c783e6ab0b503eaa86e310a5db738");
    QTest::newRow("rfc-md5-3") << QCryptographicHash::Md5
                               << QByteArray::fromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")
                               << QByteArray(50, char(0xdd))
                               << QByteArray::fromHex("56be34521d144c88dbb8c733f0e8b3f6");
}

void tst_QMessageAuthenticationCode::result()
{
    QFETCH(QCryptographicHash::Algorithm, algo);
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, message);
    QFETCH(QByteArray, code);

    QMessageAuthenticationCode mac(algo);
    mac.setKey(key);
    mac.addData(message);
    QByteArray result = mac.result();

    QCOMPARE(result, code);
}

void tst_QMessageAuthenticationCode::result_incremental_data()
{
    result_data();
}

void tst_QMessageAuthenticationCode::result_incremental()
{
    QFETCH(QCryptographicHash::Algorithm, algo);
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, message);
    QFETCH(QByteArray, code);

    int index = message.length() / 2;
    QByteArray leftPart(message.mid(0, index));
    QByteArray rightPart(message.mid(index));

    QCOMPARE(leftPart + rightPart, message);

    QMessageAuthenticationCode mac(algo);
    mac.setKey(key);
    mac.addData(leftPart);
    mac.addData(rightPart);
    QByteArray result = mac.result();

    QCOMPARE(result, code);
}

QTEST_MAIN(tst_QMessageAuthenticationCode)
#include "tst_qmessageauthenticationcode.moc"
