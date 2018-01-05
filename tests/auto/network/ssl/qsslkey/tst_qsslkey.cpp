/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qsslkey.h>
#include <qsslsocket.h>

#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qnetworkproxy.h>

#ifdef QT_BUILD_INTERNAL
    #ifndef QT_NO_SSL
        #include "private/qsslkey_p.h"
        #define TEST_CRYPTO
    #endif
    #ifndef QT_NO_OPENSSL
        #include "private/qsslsocket_openssl_symbols_p.h"
    #endif
#endif

class tst_QSslKey : public QObject
{
    Q_OBJECT

    struct KeyInfo {
        QFileInfo fileInfo;
        QSsl::KeyAlgorithm algorithm;
        QSsl::KeyType type;
        int length;
        QSsl::EncodingFormat format;
        KeyInfo(
            const QFileInfo &fileInfo, QSsl::KeyAlgorithm algorithm, QSsl::KeyType type,
            int length, QSsl::EncodingFormat format)
            : fileInfo(fileInfo), algorithm(algorithm), type(type), length(length)
            , format(format) {}
    };

    QList<KeyInfo> keyInfoList;

    void createPlainTestRows(bool filter = false, QSsl::EncodingFormat format = QSsl::EncodingFormat::Pem);

public slots:
    void initTestCase();

#ifndef QT_NO_SSL

private slots:
    void emptyConstructor();
    void constructor_data();
    void constructor();
#ifndef QT_NO_OPENSSL
    void constructorHandle_data();
    void constructorHandle();
#endif
    void copyAndAssign_data();
    void copyAndAssign();
    void equalsOperator();
    void length_data();
    void length();
    void toPemOrDer_data();
    void toPemOrDer();
    void toEncryptedPemOrDer_data();
    void toEncryptedPemOrDer();

    void passphraseChecks_data();
    void passphraseChecks();
    void noPassphraseChecks();
#ifdef TEST_CRYPTO
    void encrypt_data();
    void encrypt();
#endif

#endif
private:
    QString testDataDir;
};

void tst_QSslKey::initTestCase()
{
    testDataDir = QFileInfo(QFINDTESTDATA("rsa-without-passphrase.pem")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();

    QDir dir(testDataDir + "/keys");
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::Readable);
    QRegExp rx(QLatin1String("^(rsa|dsa|ec)-(pub|pri)-(\\d+)-?\\w*\\.(pem|der)$"));
    foreach (QFileInfo fileInfo, fileInfoList) {
        if (rx.indexIn(fileInfo.fileName()) >= 0)
            keyInfoList << KeyInfo(
                fileInfo,
                rx.cap(1) == QLatin1String("rsa") ? QSsl::Rsa :
                (rx.cap(1) == QLatin1String("dsa") ? QSsl::Dsa : QSsl::Ec),
                rx.cap(2) == QLatin1String("pub") ? QSsl::PublicKey : QSsl::PrivateKey,
                rx.cap(3).toInt(),
                rx.cap(4) == QLatin1String("pem") ? QSsl::Pem : QSsl::Der);
    }
}

#ifndef QT_NO_SSL

static QByteArray readFile(const QString &absFilePath)
{
    QFile file(absFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QWARN("failed to open file");
        return QByteArray();
    }
    return file.readAll();
}

void tst_QSslKey::emptyConstructor()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslKey key;
    QVERIFY(key.isNull());
    QVERIFY(key.length() < 0);

    QSslKey key2;
    QCOMPARE(key, key2);
}

Q_DECLARE_METATYPE(QSsl::KeyAlgorithm)
Q_DECLARE_METATYPE(QSsl::KeyType)
Q_DECLARE_METATYPE(QSsl::EncodingFormat)

void tst_QSslKey::createPlainTestRows(bool filter, QSsl::EncodingFormat format)
{
    QTest::addColumn<QString>("absFilePath");
    QTest::addColumn<QSsl::KeyAlgorithm>("algorithm");
    QTest::addColumn<QSsl::KeyType>("type");
    QTest::addColumn<int>("length");
    QTest::addColumn<QSsl::EncodingFormat>("format");
    foreach (KeyInfo keyInfo, keyInfoList) {
        if (filter && keyInfo.format != format)
            continue;

        QTest::newRow(keyInfo.fileInfo.fileName().toLatin1())
            << keyInfo.fileInfo.absoluteFilePath() << keyInfo.algorithm << keyInfo.type
            << keyInfo.length << keyInfo.format;
    }
}

void tst_QSslKey::constructor_data()
{
    createPlainTestRows();
}

void tst_QSslKey::constructor()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QSslKey key(encoded, algorithm, format, type);
    QVERIFY(!key.isNull());
}

#ifndef QT_NO_OPENSSL

void tst_QSslKey::constructorHandle_data()
{
    createPlainTestRows(true);
}

void tst_QSslKey::constructorHandle()
{
#ifndef QT_BUILD_INTERNAL
    QSKIP("This test requires -developer-build.");
#else
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(int, length);

    QByteArray pem = readFile(absFilePath);
    auto func = (type == QSsl::KeyType::PublicKey
                 ? q_PEM_read_bio_PUBKEY
                 : q_PEM_read_bio_PrivateKey);

    BIO* bio = q_BIO_new(q_BIO_s_mem());
    q_BIO_write(bio, pem.constData(), pem.length());
    QSslKey key(func(bio, nullptr, nullptr, nullptr), type);
    q_BIO_free(bio);

    QVERIFY(!key.isNull());
    QCOMPARE(key.algorithm(), algorithm);
    QCOMPARE(key.type(), type);
    QCOMPARE(key.length(), length);
#endif
}

#endif

void tst_QSslKey::copyAndAssign_data()
{
    createPlainTestRows();
}

void tst_QSslKey::copyAndAssign()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QSslKey key(encoded, algorithm, format, type);

    QSslKey copied(key);
    QCOMPARE(key, copied);
    QCOMPARE(key.algorithm(), copied.algorithm());
    QCOMPARE(key.type(), copied.type());
    QCOMPARE(key.length(), copied.length());
    QCOMPARE(key.toPem(), copied.toPem());
    QCOMPARE(key.toDer(), copied.toDer());

    QSslKey assigned = key;
    QCOMPARE(key, assigned);
    QCOMPARE(key.algorithm(), assigned.algorithm());
    QCOMPARE(key.type(), assigned.type());
    QCOMPARE(key.length(), assigned.length());
    QCOMPARE(key.toPem(), assigned.toPem());
    QCOMPARE(key.toDer(), assigned.toDer());
}

void tst_QSslKey::equalsOperator()
{
    // ### unimplemented
}

void tst_QSslKey::length_data()
{
    createPlainTestRows();
}

void tst_QSslKey::length()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(int, length);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QSslKey key(encoded, algorithm, format, type);
    QVERIFY(!key.isNull());
    QCOMPARE(key.length(), length);
}

void tst_QSslKey::toPemOrDer_data()
{
    createPlainTestRows();
}

void tst_QSslKey::toPemOrDer()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QSslKey key(encoded, algorithm, format, type);
    QVERIFY(!key.isNull());
    if (format == QSsl::Pem)
        encoded.replace('\r', "");
    QCOMPARE(format == QSsl::Pem ? key.toPem() : key.toDer(), encoded);
}

void tst_QSslKey::toEncryptedPemOrDer_data()
{
    QTest::addColumn<QString>("absFilePath");
    QTest::addColumn<QSsl::KeyAlgorithm>("algorithm");
    QTest::addColumn<QSsl::KeyType>("type");
    QTest::addColumn<QSsl::EncodingFormat>("format");
    QTest::addColumn<QString>("password");

    QStringList passwords;
    passwords << " " << "foobar" << "foo bar"
              << "aAzZ`1234567890-=~!@#$%^&*()_+[]{}\\|;:'\",.<>/?"; // ### add more (?)
    foreach (KeyInfo keyInfo, keyInfoList) {
        foreach (QString password, passwords) {
            const QByteArray testName = keyInfo.fileInfo.fileName().toLatin1()
            + '-' + (keyInfo.algorithm == QSsl::Rsa ? "RSA" :
                                                      (keyInfo.algorithm == QSsl::Dsa ? "DSA" : "EC"))
            + '-' + (keyInfo.type == QSsl::PrivateKey ? "PrivateKey" : "PublicKey")
            + '-' + (keyInfo.format == QSsl::Pem ? "PEM" : "DER")
            + password.toLatin1();
            QTest::newRow(testName.constData())
                << keyInfo.fileInfo.absoluteFilePath() << keyInfo.algorithm << keyInfo.type
                << keyInfo.format << password;
        }
    }
}

void tst_QSslKey::toEncryptedPemOrDer()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(QSsl::EncodingFormat, format);
    QFETCH(QString, password);

    QByteArray plain = readFile(absFilePath);
    QSslKey key(plain, algorithm, format, type);
    QVERIFY(!key.isNull());

    QByteArray pwBytes(password.toLatin1());

    if (type == QSsl::PrivateKey) {
        QByteArray encryptedPem = key.toPem(pwBytes);
        QVERIFY(!encryptedPem.isEmpty());
        QSslKey keyPem(encryptedPem, algorithm, QSsl::Pem, type, pwBytes);
        QVERIFY(!keyPem.isNull());
        QCOMPARE(keyPem, key);
        QCOMPARE(keyPem.toPem(), key.toPem());
    } else {
        // verify that public keys are never encrypted by toPem()
        QByteArray encryptedPem = key.toPem(pwBytes);
        QVERIFY(!encryptedPem.isEmpty());
        QByteArray plainPem = key.toPem();
        QVERIFY(!plainPem.isEmpty());
        QCOMPARE(encryptedPem, plainPem);
    }

    if (type == QSsl::PrivateKey) {
        // verify that private keys are never "encrypted" by toDer() and
        // instead an empty string is returned, see QTBUG-41038.
        QByteArray encryptedDer = key.toDer(pwBytes);
        QVERIFY(encryptedDer.isEmpty());
    } else {
        // verify that public keys are never encrypted by toDer()
        QByteArray encryptedDer = key.toDer(pwBytes);
        QVERIFY(!encryptedDer.isEmpty());
        QByteArray plainDer = key.toDer();
        QVERIFY(!plainDer.isEmpty());
        QCOMPARE(encryptedDer, plainDer);
    }

    // ### add a test to verify that public keys are _decrypted_ correctly (by the ctor)
}

void tst_QSslKey::passphraseChecks_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::newRow("DES") << QString(testDataDir + "/rsa-with-passphrase-des.pem");
    QTest::newRow("3DES") << QString(testDataDir + "/rsa-with-passphrase-3des.pem");
    QTest::newRow("RC2") << QString(testDataDir + "/rsa-with-passphrase-rc2.pem");
}

void tst_QSslKey::passphraseChecks()
{
    QFETCH(QString, fileName);

    QFile keyFile(fileName);
    QVERIFY(keyFile.exists());
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(key.isNull()); // null passphrase => should not be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, "");
        QVERIFY(key.isNull()); // empty passphrase => should not be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, "WRONG!");
        QVERIFY(key.isNull()); // wrong passphrase => should not be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, "123");
        QVERIFY(!key.isNull()); // correct passphrase
    }
}

void tst_QSslKey::noPassphraseChecks()
{
    // be sure and check a key without passphrase too
    QString fileName(testDataDir + "/rsa-without-passphrase.pem");
    QFile keyFile(fileName);
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull()); // null passphrase => should be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, "");
        QVERIFY(!key.isNull()); // empty passphrase => should be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, "xxx");
        QVERIFY(!key.isNull()); // passphrase given but key is not encrypted anyway => should work
    }
}

#ifdef TEST_CRYPTO
Q_DECLARE_METATYPE(QSslKeyPrivate::Cipher)

void tst_QSslKey::encrypt_data()
{
    QTest::addColumn<QSslKeyPrivate::Cipher>("cipher");
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("plainText");
    QTest::addColumn<QByteArray>("cipherText");

    QTest::newRow("DES-CBC, length 0")
        << QSslKeyPrivate::DesCbc << QByteArray("01234567")
        << QByteArray()
        << QByteArray::fromHex("956585228BAF9B1F");
    QTest::newRow("DES-CBC, length 1")
        << QSslKeyPrivate::DesCbc << QByteArray("01234567")
        << QByteArray(1, 'a')
        << QByteArray::fromHex("E6880AF202BA3C12");
    QTest::newRow("DES-CBC, length 2")
        << QSslKeyPrivate::DesCbc << QByteArray("01234567")
        << QByteArray(2, 'a')
        << QByteArray::fromHex("A82492386EED6026");
    QTest::newRow("DES-CBC, length 3")
        << QSslKeyPrivate::DesCbc << QByteArray("01234567")
        << QByteArray(3, 'a')
        << QByteArray::fromHex("90B76D5B79519CBA");
    QTest::newRow("DES-CBC, length 4")
        << QSslKeyPrivate::DesCbc << QByteArray("01234567")
        << QByteArray(4, 'a')
        << QByteArray::fromHex("63E3DD6FED87052A");
    QTest::newRow("DES-CBC, length 5")
        << QSslKeyPrivate::DesCbc << QByteArray("01234567")
        << QByteArray(5, 'a')
        << QByteArray::fromHex("03ACDB0EACBDFA94");
    QTest::newRow("DES-CBC, length 6")
        << QSslKeyPrivate::DesCbc << QByteArray("01234567")
        << QByteArray(6, 'a')
        << QByteArray::fromHex("7D95024E42A3A88A");
    QTest::newRow("DES-CBC, length 7")
        << QSslKeyPrivate::DesCbc << QByteArray("01234567")
        << QByteArray(7, 'a')
        << QByteArray::fromHex("5003436B8A8E42E9");
    QTest::newRow("DES-CBC, length 8")
        << QSslKeyPrivate::DesCbc << QByteArray("01234567")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("E4C1F054BF5521C0A4A0FD4A2BC6C1B1");

    QTest::newRow("DES-EDE3-CBC, length 0")
        << QSslKeyPrivate::DesEde3Cbc << QByteArray("0123456789abcdefghijklmn")
        << QByteArray()
        << QByteArray::fromHex("3B2B4CD0B0FD495F");
    QTest::newRow("DES-EDE3-CBC, length 8")
        << QSslKeyPrivate::DesEde3Cbc << QByteArray("0123456789abcdefghijklmn")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("F2A5A87763C54A72A3224103D90CDB03");

    QTest::newRow("RC2-40-CBC, length 0")
        << QSslKeyPrivate::Rc2Cbc << QByteArray("01234")
        << QByteArray()
        << QByteArray::fromHex("6D05D52392FF6E7A");
    QTest::newRow("RC2-40-CBC, length 8")
        << QSslKeyPrivate::Rc2Cbc << QByteArray("01234")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("75768E64C5749072A5D168F3AFEB0005");

    QTest::newRow("RC2-64-CBC, length 0")
        << QSslKeyPrivate::Rc2Cbc << QByteArray("01234567")
        << QByteArray()
        << QByteArray::fromHex("ADAE6BF70F420130");
    QTest::newRow("RC2-64-CBC, length 8")
        << QSslKeyPrivate::Rc2Cbc << QByteArray("01234567")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("C7BF5C80AFBE9FBEFBBB9FD935F6D0DF");

    QTest::newRow("RC2-128-CBC, length 0")
        << QSslKeyPrivate::Rc2Cbc << QByteArray("012345679abcdefg")
        << QByteArray()
        << QByteArray::fromHex("1E965D483A13C8FB");
    QTest::newRow("RC2-128-CBC, length 8")
        << QSslKeyPrivate::Rc2Cbc << QByteArray("012345679abcdefg")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("5AEC1A5B295660B02613454232F7DECE");
}

void tst_QSslKey::encrypt()
{
    QFETCH(QSslKeyPrivate::Cipher, cipher);
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, plainText);
    QFETCH(QByteArray, cipherText);
    QByteArray iv("abcdefgh");

#ifdef Q_OS_WINRT
    QEXPECT_FAIL("RC2-40-CBC, length 0", "WinRT treats RC2 as 128-bit", Abort);
    QEXPECT_FAIL("RC2-40-CBC, length 8", "WinRT treats RC2 as 128-bit", Abort);
    QEXPECT_FAIL("RC2-64-CBC, length 0", "WinRT treats RC2 as 128-bit", Abort);
    QEXPECT_FAIL("RC2-64-CBC, length 8", "WinRT treats RC2 as 128-bit", Abort);
#endif
    QByteArray encrypted = QSslKeyPrivate::encrypt(cipher, plainText, key, iv);
    QCOMPARE(encrypted, cipherText);

    QByteArray decrypted = QSslKeyPrivate::decrypt(cipher, cipherText, key, iv);
    QCOMPARE(decrypted, plainText);
}
#endif

#endif

QTEST_MAIN(tst_QSslKey)
#include "tst_qsslkey.moc"
