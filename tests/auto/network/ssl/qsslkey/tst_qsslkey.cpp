/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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


#include <QtTest/QtTest>
#include <qsslkey.h>
#include <qsslsocket.h>

#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qnetworkproxy.h>

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

    void createPlainTestRows();

public slots:
    void initTestCase();

#ifndef QT_NO_SSL

private slots:
    void emptyConstructor();
    void constructor_data();
    void constructor();
    void copyAndAssign_data();
    void copyAndAssign();
    void equalsOperator();
    void length_data();
    void length();
    void toPemOrDer_data();
    void toPemOrDer();
    void toEncryptedPemOrDer_data();
    void toEncryptedPemOrDer();

    void passphraseChecks();
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
    QRegExp rx(QLatin1String("^(rsa|dsa)-(pub|pri)-(\\d+)\\.(pem|der)$"));
    foreach (QFileInfo fileInfo, fileInfoList) {
        if (rx.indexIn(fileInfo.fileName()) >= 0)
            keyInfoList << KeyInfo(
                fileInfo,
                rx.cap(1) == QLatin1String("rsa") ? QSsl::Rsa : QSsl::Dsa,
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

void tst_QSslKey::createPlainTestRows()
{
    QTest::addColumn<QString>("absFilePath");
    QTest::addColumn<QSsl::KeyAlgorithm>("algorithm");
    QTest::addColumn<QSsl::KeyType>("type");
    QTest::addColumn<int>("length");
    QTest::addColumn<QSsl::EncodingFormat>("format");
    foreach (KeyInfo keyInfo, keyInfoList) {
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
            QString testName = QString("%1-%2-%3-%4-%5").arg(keyInfo.fileInfo.fileName())
                .arg(keyInfo.algorithm == QSsl::Rsa ? "RSA" : "DSA")
                .arg(keyInfo.type == QSsl::PrivateKey ? "PrivateKey" : "PublicKey")
                .arg(keyInfo.format == QSsl::Pem ? "PEM" : "DER")
                .arg(password);
            QTest::newRow(testName.toLatin1())
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
        QByteArray encryptedDer = key.toDer(pwBytes);
        // ### at this point, encryptedDer is invalid, hence the below QEXPECT_FAILs
        QVERIFY(!encryptedDer.isEmpty());
        QSslKey keyDer(encryptedDer, algorithm, QSsl::Der, type, pwBytes);
        if (type == QSsl::PrivateKey)
            QEXPECT_FAIL(
                QTest::currentDataTag(), "We're not able to decrypt these yet...", Continue);
        QVERIFY(!keyDer.isNull());
        if (type == QSsl::PrivateKey)
            QEXPECT_FAIL(
                QTest::currentDataTag(), "We're not able to decrypt these yet...", Continue);
        QCOMPARE(keyDer.toPem(), key.toPem());
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

void tst_QSslKey::passphraseChecks()
{
    {
        QString fileName(testDataDir + "/rsa-with-passphrase.pem");
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
}

#endif

QTEST_MAIN(tst_QSslKey)
#include "tst_qsslkey.moc"
