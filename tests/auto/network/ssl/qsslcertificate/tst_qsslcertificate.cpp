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
#include <qsslcertificate.h>
#include <qsslkey.h>
#include <qsslsocket.h>
#include <qsslcertificateextension.h>

class tst_QSslCertificate : public QObject
{
    Q_OBJECT

    struct CertInfo {
        QFileInfo fileInfo;
        QFileInfo fileInfo_digest_md5;
        QFileInfo fileInfo_digest_sha1;
        QSsl::EncodingFormat format;
        CertInfo(const QFileInfo &fileInfo, QSsl::EncodingFormat format)
            : fileInfo(fileInfo), format(format) {}
    };

    QList<CertInfo> certInfoList;
    QMap<QString, QString> subjAltNameMap;
    QMap<QString, QString> pubkeyMap;
    QMap<QString, QString> md5Map;
    QMap<QString, QString> sha1Map;

    void createTestRows();
#ifndef QT_NO_SSL
    void compareCertificates(const QSslCertificate & cert1, const QSslCertificate & cert2);
#endif

    QString oldCurrentDir;

public slots:
    void initTestCase();
    void cleanupTestCase();

#ifndef QT_NO_SSL
private slots:
    void emptyConstructor();
    void constructor_data();
    void constructor();
    void constructor_device();
    void constructingGarbage();
    void copyAndAssign_data();
    void copyAndAssign();
    void digest_data();
    void digest();
    void subjectAlternativeNames_data();
    void utf8SubjectNames();
    void subjectAlternativeNames();
    void publicKey_data();
    void publicKey();
    void toPemOrDer_data();
    void toPemOrDer();
    void fromDevice();
    void fromPath_data();
    void fromPath();
    void certInfo();
    void certInfoQByteArray();
    void task256066toPem();
    void nulInCN();
    void nulInSan();
    void largeSerialNumber();
    void largeExpirationDate();
    void blacklistedCertificates();
    void toText();
    void multipleCommonNames();
    void subjectAndIssuerAttributes();
    void verify();
    void extensions();
    void threadSafeConstMethods();
    void version_data();
    void version();

    // helper for verbose test failure messages
    QString toString(const QList<QSslError>&);

// ### add tests for certificate bundles (multiple certificates concatenated into a single
//     structure); both PEM and DER formatted
#endif
private:
    QString testDataDir;
};

void tst_QSslCertificate::initTestCase()
{
    testDataDir = QFileInfo(QFINDTESTDATA("certificates")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();

    if (QDir::current().absolutePath() != testDataDir) {
        oldCurrentDir = QDir::current().absolutePath();
        QVERIFY2(QDir::setCurrent(testDataDir),
                 qPrintable(QString("Cannot change directory to %1").arg(testDataDir)));
    }

    QDir dir(testDataDir + "/certificates");
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::Readable);
    QRegExp rxCert(QLatin1String("^.+\\.(pem|der)$"));
    QRegExp rxSan(QLatin1String("^(.+\\.(?:pem|der))\\.san$"));
    QRegExp rxPubKey(QLatin1String("^(.+\\.(?:pem|der))\\.pubkey$"));
    QRegExp rxDigest(QLatin1String("^(.+\\.(?:pem|der))\\.digest-(md5|sha1)$"));
    foreach (QFileInfo fileInfo, fileInfoList) {
        if (rxCert.indexIn(fileInfo.fileName()) >= 0)
            certInfoList <<
                CertInfo(fileInfo,
                         rxCert.cap(1) == QLatin1String("pem") ? QSsl::Pem : QSsl::Der);
        if (rxSan.indexIn(fileInfo.fileName()) >= 0)
            subjAltNameMap.insert(rxSan.cap(1), fileInfo.absoluteFilePath());
        if (rxPubKey.indexIn(fileInfo.fileName()) >= 0)
            pubkeyMap.insert(rxPubKey.cap(1), fileInfo.absoluteFilePath());
        if (rxDigest.indexIn(fileInfo.fileName()) >= 0) {
            if (rxDigest.cap(2) == QLatin1String("md5"))
                md5Map.insert(rxDigest.cap(1), fileInfo.absoluteFilePath());
            else
                sha1Map.insert(rxDigest.cap(1), fileInfo.absoluteFilePath());
        }
    }
}

void tst_QSslCertificate::cleanupTestCase()
{
    if (!oldCurrentDir.isEmpty()) {
        QDir::setCurrent(oldCurrentDir);
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

void tst_QSslCertificate::emptyConstructor()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslCertificate certificate;
    QVERIFY(certificate.isNull());
    //make sure none of the functions crash  (task 203035)
    QVERIFY(!certificate.isBlacklisted());
    QCOMPARE(certificate.version() , QByteArray());
    QCOMPARE(certificate.serialNumber(), QByteArray());
    QCOMPARE(certificate.digest(), QCryptographicHash::hash(QByteArray(), QCryptographicHash::Md5));
    QCOMPARE(certificate.issuerInfo(QSslCertificate::Organization), QStringList());
    QCOMPARE(certificate.subjectInfo(QSslCertificate::Organization), QStringList());
    QCOMPARE(certificate.subjectAlternativeNames(),(QMultiMap<QSsl::AlternativeNameEntryType, QString>()));
    QCOMPARE(certificate.effectiveDate(), QDateTime());
    QCOMPARE(certificate.expiryDate(), QDateTime());
}

Q_DECLARE_METATYPE(QSsl::EncodingFormat);

void tst_QSslCertificate::createTestRows()
{
    QTest::addColumn<QString>("absFilePath");
    QTest::addColumn<QSsl::EncodingFormat>("format");
    foreach (CertInfo certInfo, certInfoList) {
        QTest::newRow(certInfo.fileInfo.fileName().toLatin1())
            << certInfo.fileInfo.absoluteFilePath() << certInfo.format;
    }
}

void tst_QSslCertificate::constructor_data()
{
    createTestRows();
}

void tst_QSslCertificate::constructor()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QSslCertificate certificate(encoded, format);
    QVERIFY(!certificate.isNull());
}

void tst_QSslCertificate::constructor_device()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFile f(testDataDir + "/verify-certs/test-ocsp-good-cert.pem");
    bool ok = f.open(QIODevice::ReadOnly);
    QVERIFY(ok);

    QSslCertificate cert(&f);
    QVERIFY(!cert.isNull());
    f.close();

    // Check opening a DER as a PEM fails
    QFile f2(testDataDir + "/certificates/cert.der");
    ok = f2.open(QIODevice::ReadOnly);
    QVERIFY(ok);

    QSslCertificate cert2(&f2);
    QVERIFY(cert2.isNull());
    f2.close();

    // Check opening a DER as a DER works
    QFile f3(testDataDir + "/certificates/cert.der");
    ok = f3.open(QIODevice::ReadOnly);
    QVERIFY(ok);

    QSslCertificate cert3(&f3, QSsl::Der);
    QVERIFY(!cert3.isNull());
    f3.close();

    // Check opening a PEM as a DER fails
    QFile f4(testDataDir + "/verify-certs/test-ocsp-good-cert.pem");
    ok = f4.open(QIODevice::ReadOnly);
    QVERIFY(ok);

    QSslCertificate cert4(&f4, QSsl::Der);
    QVERIFY(cert4.isNull());
    f4.close();
}

void tst_QSslCertificate::constructingGarbage()
{
    if (!QSslSocket::supportsSsl())
        return;

    QByteArray garbage("garbage");
    QSslCertificate certificate(garbage);
    QVERIFY(certificate.isNull());
}

void tst_QSslCertificate::copyAndAssign_data()
{
    createTestRows();
}

void tst_QSslCertificate::compareCertificates(
    const QSslCertificate & cert1, const QSslCertificate & cert2)
{
    QCOMPARE(cert1.isNull(), cert2.isNull());
    // Note: in theory, the next line could fail even if the certificates are identical!
    QCOMPARE(cert1.isBlacklisted(), cert2.isBlacklisted());
    QCOMPARE(cert1.version(), cert2.version());
    QCOMPARE(cert1.serialNumber(), cert2.serialNumber());
    QCOMPARE(cert1.digest(), cert2.digest());
    QCOMPARE(cert1.toPem(), cert2.toPem());
    QCOMPARE(cert1.toDer(), cert2.toDer());
    for (int info = QSslCertificate::Organization;
         info <= QSslCertificate::StateOrProvinceName; info++) {
        const QSslCertificate::SubjectInfo subjectInfo = (QSslCertificate::SubjectInfo)info;
        QCOMPARE(cert1.issuerInfo(subjectInfo), cert2.issuerInfo(subjectInfo));
        QCOMPARE(cert1.subjectInfo(subjectInfo), cert2.subjectInfo(subjectInfo));
    }
    QCOMPARE(cert1.subjectAlternativeNames(), cert2.subjectAlternativeNames());
    QCOMPARE(cert1.effectiveDate(), cert2.effectiveDate());
    QCOMPARE(cert1.expiryDate(), cert2.expiryDate());
    QCOMPARE(cert1.version(), cert2.version());
    QCOMPARE(cert1.serialNumber(), cert2.serialNumber());
    // ### add more functions here ...
}

void tst_QSslCertificate::copyAndAssign()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QSslCertificate certificate(encoded, format);

    QVERIFY(!certificate.isNull());

    QSslCertificate copied(certificate);
    compareCertificates(certificate, copied);

    QSslCertificate assigned = certificate;
    compareCertificates(certificate, assigned);
}

void tst_QSslCertificate::digest_data()
{
    QTest::addColumn<QString>("absFilePath");
    QTest::addColumn<QSsl::EncodingFormat>("format");
    QTest::addColumn<QString>("absFilePath_digest_md5");
    QTest::addColumn<QString>("absFilePath_digest_sha1");
    foreach (CertInfo certInfo, certInfoList) {
        QString certName = certInfo.fileInfo.fileName();
        QTest::newRow(certName.toLatin1())
            << certInfo.fileInfo.absoluteFilePath()
            << certInfo.format
            << md5Map.value(certName)
            << sha1Map.value(certName);
    }
}

// Converts a digest of the form '{MD5|SHA1} Fingerprint=AB:B8:32...' to binary format.
static QByteArray convertDigest(const QByteArray &input)
{
    QByteArray result;
    QRegExp rx(QLatin1String("(?:=|:)([0-9A-Fa-f]{2})"));
    int pos = 0;
    while ((pos = rx.indexIn(input, pos)) != -1) {
        result.append(rx.cap(1).toLatin1());
        pos += rx.matchedLength();
    }
    return QByteArray::fromHex(result);
}

void tst_QSslCertificate::digest()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::EncodingFormat, format);
    QFETCH(QString, absFilePath_digest_md5);
    QFETCH(QString, absFilePath_digest_sha1);

    QByteArray encoded = readFile(absFilePath);
    QSslCertificate certificate(encoded, format);
    QVERIFY(!certificate.isNull());

    if (!absFilePath_digest_md5.isEmpty())
        QCOMPARE(convertDigest(readFile(absFilePath_digest_md5)),
                 certificate.digest(QCryptographicHash::Md5));

    if (!absFilePath_digest_sha1.isEmpty())
        QCOMPARE(convertDigest(readFile(absFilePath_digest_sha1)),
                 certificate.digest(QCryptographicHash::Sha1));
}

void tst_QSslCertificate::subjectAlternativeNames_data()
{
    QTest::addColumn<QString>("certFilePath");
    QTest::addColumn<QSsl::EncodingFormat>("format");
    QTest::addColumn<QString>("subjAltNameFilePath");

    foreach (CertInfo certInfo, certInfoList) {
        QString certName = certInfo.fileInfo.fileName();
        if (subjAltNameMap.contains(certName))
            QTest::newRow(certName.toLatin1())
                << certInfo.fileInfo.absoluteFilePath()
                << certInfo.format
                << subjAltNameMap.value(certName);
    }
}

void tst_QSslCertificate::subjectAlternativeNames()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, certFilePath);
    QFETCH(QSsl::EncodingFormat, format);
    QFETCH(QString, subjAltNameFilePath);

    QByteArray encodedCert = readFile(certFilePath);
    QSslCertificate certificate(encodedCert, format);
    QVERIFY(!certificate.isNull());

    QByteArray fileContents = readFile(subjAltNameFilePath);

    const QMultiMap<QSsl::AlternativeNameEntryType, QString> altSubjectNames =
        certificate.subjectAlternativeNames();

    // verify that each entry in subjAltNames is present in fileContents
    QMapIterator<QSsl::AlternativeNameEntryType, QString> it(altSubjectNames);
    while (it.hasNext()) {
        it.next();
        QString type;
        if (it.key() == QSsl::EmailEntry)
            type = QLatin1String("email");
        else if (it.key() == QSsl::DnsEntry)
            type = QLatin1String("DNS");
        else
            QFAIL("unsupported alternative name type");
        QString entry = QString("%1:%2").arg(type).arg(it.value());
        QVERIFY(fileContents.contains(entry.toLatin1()));
    }

    // verify that each entry in fileContents is present in subjAltNames
    QRegExp rx(QLatin1String("(email|DNS):([^,\\r\\n]+)"));
    for (int pos = 0; (pos = rx.indexIn(fileContents, pos)) != -1; pos += rx.matchedLength()) {
        QSsl::AlternativeNameEntryType key;
        if (rx.cap(1) == QLatin1String("email"))
            key = QSsl::EmailEntry;
        else if (rx.cap(1) == QLatin1String("DNS"))
            key = QSsl::DnsEntry;
        else
            QFAIL("unsupported alternative name type");
        QVERIFY(altSubjectNames.contains(key, rx.cap(2)));
    }
}

void tst_QSslCertificate::utf8SubjectNames()
{
    QSslCertificate cert = QSslCertificate::fromPath("certificates/cert-ss-san-utf8.pem", QSsl::Pem,
                                                     QRegExp::FixedString).first();
    QVERIFY(!cert.isNull());

    // O is "Heavy Metal Records" with heavy use of "decorations" like accents, umlauts etc.,
    // OU uses arabian / asian script letters near codepoint 64K.
    // strings split where the compiler would otherwise find three-digit hex numbers
    static const char *o = "H\xc4\x95\xc4\x82\xc6\xb2\xc3\xbf \xca\x8d\xe1\xba\xbf\xca\x88\xe1\xba"
            "\xb7\xe1\xb8\xbb R\xc3\xa9" "c" "\xc3\xb6rd\xc5\x9d";
    static const char *ou = "\xe3\x88\xa7" "A" "\xe3\x89\x81\xef\xbd\xab" "BC";

    // the following two tests should help find "\x"-literal encoding bugs in the test itself
    QCOMPARE(cert.subjectInfo("O")[0].length(), QString::fromUtf8(o).length());
    QCOMPARE (cert.subjectInfo("O")[0].toUtf8().toHex(), QByteArray(o).toHex());

    QCOMPARE(cert.subjectInfo("O")[0], QString::fromUtf8(o));
    QCOMPARE(cert.subjectInfo("OU")[0], QString::fromUtf8(ou));
}

void tst_QSslCertificate::publicKey_data()
{
    QTest::addColumn<QString>("certFilePath");
    QTest::addColumn<QSsl::EncodingFormat>("format");
    QTest::addColumn<QString>("pubkeyFilePath");

    foreach (CertInfo certInfo, certInfoList) {
        QString certName = certInfo.fileInfo.fileName();
        if (pubkeyMap.contains(certName))
            QTest::newRow(certName.toLatin1())
                << certInfo.fileInfo.absoluteFilePath()
                << certInfo.format
                << pubkeyMap.value(certName);
    }
}

void tst_QSslCertificate::publicKey()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, certFilePath);
    QFETCH(QSsl::EncodingFormat, format);
    QFETCH(QString, pubkeyFilePath);

    QByteArray encodedCert = readFile(certFilePath);
    QSslCertificate certificate(encodedCert, format);
    QVERIFY(!certificate.isNull());

    QByteArray encodedPubkey = readFile(pubkeyFilePath);
    QSslKey pubkey(encodedPubkey, QSsl::Rsa, format, QSsl::PublicKey); // ### support DSA as well!
    QVERIFY(!pubkey.isNull());

    QCOMPARE(certificate.publicKey(), pubkey);
}

void tst_QSslCertificate::toPemOrDer_data()
{
    createTestRows();
}

static const char BeginCertString[] = "-----BEGIN CERTIFICATE-----";
static const char EndCertString[] = "-----END CERTIFICATE-----";

// Returns, in Pem-format, the first certificate found in a Pem-formatted block
// (Note that such a block may contain e.g. a private key at the end).
static QByteArray firstPemCertificateFromPem(const QByteArray &pem)
{
    int startPos = pem.indexOf(BeginCertString);
    int endPos = pem.indexOf(EndCertString);
    if (startPos == -1 || endPos == -1)
        return QByteArray();
    return pem.mid(startPos, endPos + sizeof(EndCertString) - startPos);
}

void tst_QSslCertificate::toPemOrDer()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QSslCertificate certificate(encoded, format);
    QVERIFY(!certificate.isNull());
    if (format == QSsl::Pem) {
        encoded.replace('\r',"");
        QByteArray firstPem = firstPemCertificateFromPem(encoded);
        QCOMPARE(certificate.toPem(), firstPem);
    } else {
        // ### for now, we assume that DER-encoded certificates don't contain bundled stuff
        QCOMPARE(certificate.toDer(), encoded);
    }
}

void tst_QSslCertificate::fromDevice()
{
    QTest::ignoreMessage(QtWarningMsg, "QSslCertificate::fromDevice: cannot read from a null device");
    QList<QSslCertificate> certs = QSslCertificate::fromDevice(0); // don't crash
    QVERIFY(certs.isEmpty());
}

void tst_QSslCertificate::fromPath_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("syntax");
    QTest::addColumn<bool>("pemencoding");
    QTest::addColumn<int>("numCerts");

    QTest::newRow("empty fixed pem") << QString() << int(QRegExp::FixedString) << true << 0;
    QTest::newRow("empty fixed der") << QString() << int(QRegExp::FixedString) << false << 0;
    QTest::newRow("empty regexp pem") << QString() << int(QRegExp::RegExp) << true << 0;
    QTest::newRow("empty regexp der") << QString() << int(QRegExp::RegExp) << false << 0;
    QTest::newRow("empty wildcard pem") << QString() << int(QRegExp::Wildcard) << true << 0;
    QTest::newRow("empty wildcard der") << QString() << int(QRegExp::Wildcard) << false << 0;
    QTest::newRow("\"certificates\" fixed pem") << QString("certificates") << int(QRegExp::FixedString) << true << 0;
    QTest::newRow("\"certificates\" fixed der") << QString("certificates") << int(QRegExp::FixedString) << false << 0;
    QTest::newRow("\"certificates\" regexp pem") << QString("certificates") << int(QRegExp::RegExp) << true << 0;
    QTest::newRow("\"certificates\" regexp der") << QString("certificates") << int(QRegExp::RegExp) << false << 0;
    QTest::newRow("\"certificates\" wildcard pem") << QString("certificates") << int(QRegExp::Wildcard) << true << 0;
    QTest::newRow("\"certificates\" wildcard der") << QString("certificates") << int(QRegExp::Wildcard) << false << 0;
    QTest::newRow("\"certificates/cert.pem\" fixed pem") << QString("certificates/cert.pem") << int(QRegExp::FixedString) << true << 1;
    QTest::newRow("\"certificates/cert.pem\" fixed der") << QString("certificates/cert.pem") << int(QRegExp::FixedString) << false << 0;
    QTest::newRow("\"certificates/cert.pem\" regexp pem") << QString("certificates/cert.pem") << int(QRegExp::RegExp) << true << 1;
    QTest::newRow("\"certificates/cert.pem\" regexp der") << QString("certificates/cert.pem") << int(QRegExp::RegExp) << false << 0;
    QTest::newRow("\"certificates/cert.pem\" wildcard pem") << QString("certificates/cert.pem") << int(QRegExp::Wildcard) << true << 1;
    QTest::newRow("\"certificates/cert.pem\" wildcard der") << QString("certificates/cert.pem") << int(QRegExp::Wildcard) << false << 0;
    QTest::newRow("\"certificates/*\" fixed pem") << QString("certificates/*") << int(QRegExp::FixedString) << true << 0;
    QTest::newRow("\"certificates/*\" fixed der") << QString("certificates/*") << int(QRegExp::FixedString) << false << 0;
    QTest::newRow("\"certificates/*\" regexp pem") << QString("certificates/*") << int(QRegExp::RegExp) << true << 0;
    QTest::newRow("\"certificates/*\" regexp der") << QString("certificates/*") << int(QRegExp::RegExp) << false << 0;
    QTest::newRow("\"certificates/*\" wildcard pem") << QString("certificates/*") << int(QRegExp::Wildcard) << true << 5;
    QTest::newRow("\"certificates/ca*\" wildcard pem") << QString("certificates/ca*") << int(QRegExp::Wildcard) << true << 1;
    QTest::newRow("\"certificates/cert*\" wildcard pem") << QString("certificates/cert*") << int(QRegExp::Wildcard) << true << 4;
    QTest::newRow("\"certificates/cert-[sure]*\" wildcard pem") << QString("certificates/cert-[sure]*") << int(QRegExp::Wildcard) << true << 3;
    QTest::newRow("\"certificates/cert-[not]*\" wildcard pem") << QString("certificates/cert-[not]*") << int(QRegExp::Wildcard) << true << 0;
    QTest::newRow("\"certificates/*\" wildcard der") << QString("certificates/*") << int(QRegExp::Wildcard) << false << 2;
    QTest::newRow("\"c*/c*.pem\" fixed pem") << QString("c*/c*.pem") << int(QRegExp::FixedString) << true << 0;
    QTest::newRow("\"c*/c*.pem\" fixed der") << QString("c*/c*.pem") << int(QRegExp::FixedString) << false << 0;
    QTest::newRow("\"c*/c*.pem\" regexp pem") << QString("c*/c*.pem") << int(QRegExp::RegExp) << true << 0;
    QTest::newRow("\"c*/c*.pem\" regexp der") << QString("c*/c*.pem") << int(QRegExp::RegExp) << false << 0;
    QTest::newRow("\"c*/c*.pem\" wildcard pem") << QString("c*/c*.pem") << int(QRegExp::Wildcard) << true << 5;
    QTest::newRow("\"c*/c*.pem\" wildcard der") << QString("c*/c*.pem") << int(QRegExp::Wildcard) << false << 0;
    QTest::newRow("\"d*/c*.pem\" fixed pem") << QString("d*/c*.pem") << int(QRegExp::FixedString) << true << 0;
    QTest::newRow("\"d*/c*.pem\" fixed der") << QString("d*/c*.pem") << int(QRegExp::FixedString) << false << 0;
    QTest::newRow("\"d*/c*.pem\" regexp pem") << QString("d*/c*.pem") << int(QRegExp::RegExp) << true << 0;
    QTest::newRow("\"d*/c*.pem\" regexp der") << QString("d*/c*.pem") << int(QRegExp::RegExp) << false << 0;
    QTest::newRow("\"d*/c*.pem\" wildcard pem") << QString("d*/c*.pem") << int(QRegExp::Wildcard) << true << 0;
    QTest::newRow("\"d*/c*.pem\" wildcard der") << QString("d*/c*.pem") << int(QRegExp::Wildcard) << false << 0;
    QTest::newRow("\"c.*/c.*.pem\" fixed pem") << QString("c.*/c.*.pem") << int(QRegExp::FixedString) << true << 0;
    QTest::newRow("\"c.*/c.*.pem\" fixed der") << QString("c.*/c.*.pem") << int(QRegExp::FixedString) << false << 0;
    QTest::newRow("\"c.*/c.*.pem\" regexp pem") << QString("c.*/c.*.pem") << int(QRegExp::RegExp) << true << 5;
    QTest::newRow("\"c.*/c.*.pem\" regexp der") << QString("c.*/c.*.pem") << int(QRegExp::RegExp) << false << 0;
    QTest::newRow("\"c.*/c.*.pem\" wildcard pem") << QString("c.*/c.*.pem") << int(QRegExp::Wildcard) << true << 0;
    QTest::newRow("\"c.*/c.*.pem\" wildcard der") << QString("c.*/c.*.pem") << int(QRegExp::Wildcard) << false << 0;
    QTest::newRow("\"d.*/c.*.pem\" fixed pem") << QString("d.*/c.*.pem") << int(QRegExp::FixedString) << true << 0;
    QTest::newRow("\"d.*/c.*.pem\" fixed der") << QString("d.*/c.*.pem") << int(QRegExp::FixedString) << false << 0;
    QTest::newRow("\"d.*/c.*.pem\" regexp pem") << QString("d.*/c.*.pem") << int(QRegExp::RegExp) << true << 0;
    QTest::newRow("\"d.*/c.*.pem\" regexp der") << QString("d.*/c.*.pem") << int(QRegExp::RegExp) << false << 0;
    QTest::newRow("\"d.*/c.*.pem\" wildcard pem") << QString("d.*/c.*.pem") << int(QRegExp::Wildcard) << true << 0;
    QTest::newRow("\"d.*/c.*.pem\" wildcard der") << QString("d.*/c.*.pem") << int(QRegExp::Wildcard) << false << 0;
#ifdef Q_OS_LINUX
    QTest::newRow("absolute path wildcard pem") << (testDataDir + "/certificates/*.pem") << int(QRegExp::Wildcard) << true << 5;
#endif

    QTest::newRow("trailing-whitespace") << QString("more-certificates/trailing-whitespace.pem") << int(QRegExp::FixedString) << true << 1;
    QTest::newRow("no-ending-newline") << QString("more-certificates/no-ending-newline.pem") << int(QRegExp::FixedString) << true << 1;
    QTest::newRow("malformed-just-begin") << QString("more-certificates/malformed-just-begin.pem") << int(QRegExp::FixedString) << true << 0;
    QTest::newRow("malformed-just-begin-no-newline") << QString("more-certificates/malformed-just-begin-no-newline.pem") << int(QRegExp::FixedString) << true << 0;
}

void tst_QSslCertificate::fromPath()
{
    QFETCH(QString, path);
    QFETCH(int, syntax);
    QFETCH(bool, pemencoding);
    QFETCH(int, numCerts);

    QCOMPARE(QSslCertificate::fromPath(path,
                                       pemencoding ? QSsl::Pem : QSsl::Der,
                                       QRegExp::PatternSyntax(syntax)).size(),
             numCerts);
}

void tst_QSslCertificate::certInfo()
{
// MD5 Fingerprint=B6:CF:57:34:DA:A9:73:21:82:F7:CF:4D:3D:85:31:88
// SHA1 Fingerprint=B6:D1:51:82:E0:29:CA:59:96:38:BD:B6:F9:40:05:91:6D:49:09:60
// Certificate:
//     Data:
//         Version: 1 (0x0)
//         Serial Number: 17 (0x11)
//         Signature Algorithm: sha1WithRSAEncryption
//         Issuer: C=AU, ST=Queensland, O=CryptSoft Pty Ltd, CN=Test CA (1024 bit)
//         Validity
//             Not Before: Apr 17 07:40:26 2007 GMT
//             Not After : May 17 07:40:26 2007 GMT
//         Subject: CN=name/with/slashes, C=NO
//         Subject Public Key Info:
//             Public Key Algorithm: rsaEncryption
//             RSA Public Key: (1024 bit)
//                 Modulus (1024 bit):
//                     00:eb:9d:e9:03:ac:30:4f:a9:58:03:44:c7:18:26:
//                     2f:48:93:d5:ac:a0:fb:e8:53:c4:7b:2a:01:89:e6:
//                     fc:5a:0c:c5:f5:21:f8:d7:4a:92:02:67:db:f1:9f:
//                     36:9a:62:9d:f3:ce:48:8e:ba:ed:5a:a8:9d:4f:bb:
//                     24:16:43:4c:b5:79:08:f6:d9:22:8f:5f:15:0a:43:
//                     25:03:7a:9d:a7:af:e3:26:b1:53:55:5e:60:57:c8:
//                     ed:2f:1c:f3:36:0a:78:64:91:f9:17:a7:34:d7:8b:
//                     bd:f1:fc:d1:8c:4f:a5:96:75:b2:7b:fc:21:f0:c7:
//                     d9:5f:0c:57:18:b2:af:b9:4b
//                 Exponent: 65537 (0x10001)
//     Signature Algorithm: sha1WithRSAEncryption
//         95:e6:94:e2:98:33:57:a2:98:fa:af:50:b9:76:a9:51:83:2c:
//         0b:61:a2:36:d0:e6:90:6d:e4:f8:c4:c7:50:ef:17:94:4e:21:
//         a8:fa:c8:33:aa:d1:7f:bc:ca:41:d6:7d:e7:44:76:c0:bf:45:
//         4a:76:25:42:6d:53:76:fd:fc:74:29:1a:ea:2b:cc:06:ab:d1:
//         b8:eb:7d:6b:11:f7:9b:41:bb:9f:31:cb:ed:4d:f3:68:26:ed:
//         13:1d:f2:56:59:fe:6f:7c:98:b6:25:69:4e:ea:b4:dc:c2:eb:
//         b7:bb:50:18:05:ba:ad:af:08:49:fe:98:63:55:ba:e7:fb:95:
//         5d:91
    static const char pem[] =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIB8zCCAVwCAREwDQYJKoZIhvcNAQEFBQAwWzELMAkGA1UEBhMCQVUxEzARBgNV\n"
        "BAgTClF1ZWVuc2xhbmQxGjAYBgNVBAoTEUNyeXB0U29mdCBQdHkgTHRkMRswGQYD\n"
        "VQQDExJUZXN0IENBICgxMDI0IGJpdCkwHhcNMDcwNDE3MDc0MDI2WhcNMDcwNTE3\n"
        "MDc0MDI2WjApMRowGAYDVQQDExFuYW1lL3dpdGgvc2xhc2hlczELMAkGA1UEBhMC\n"
        "Tk8wgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAOud6QOsME+pWANExxgmL0iT\n"
        "1ayg++hTxHsqAYnm/FoMxfUh+NdKkgJn2/GfNppinfPOSI667VqonU+7JBZDTLV5\n"
        "CPbZIo9fFQpDJQN6naev4yaxU1VeYFfI7S8c8zYKeGSR+RenNNeLvfH80YxPpZZ1\n"
        "snv8IfDH2V8MVxiyr7lLAgMBAAEwDQYJKoZIhvcNAQEFBQADgYEAleaU4pgzV6KY\n"
        "+q9QuXapUYMsC2GiNtDmkG3k+MTHUO8XlE4hqPrIM6rRf7zKQdZ950R2wL9FSnYl\n"
        "Qm1Tdv38dCka6ivMBqvRuOt9axH3m0G7nzHL7U3zaCbtEx3yVln+b3yYtiVpTuq0\n"
        "3MLrt7tQGAW6ra8ISf6YY1W65/uVXZE=\n"
        "-----END CERTIFICATE-----\n";
    static const char der[] =   // hex encoded
        "30:82:01:f3:30:82:01:5c:02:01:11:30:0d:06:09:2a"
        "86:48:86:f7:0d:01:01:05:05:00:30:5b:31:0b:30:09"
        "06:03:55:04:06:13:02:41:55:31:13:30:11:06:03:55"
        "04:08:13:0a:51:75:65:65:6e:73:6c:61:6e:64:31:1a"
        "30:18:06:03:55:04:0a:13:11:43:72:79:70:74:53:6f"
        "66:74:20:50:74:79:20:4c:74:64:31:1b:30:19:06:03"
        "55:04:03:13:12:54:65:73:74:20:43:41:20:28:31:30"
        "32:34:20:62:69:74:29:30:1e:17:0d:30:37:30:34:31"
        "37:30:37:34:30:32:36:5a:17:0d:30:37:30:35:31:37"
        "30:37:34:30:32:36:5a:30:29:31:1a:30:18:06:03:55"
        "04:03:13:11:6e:61:6d:65:2f:77:69:74:68:2f:73:6c"
        "61:73:68:65:73:31:0b:30:09:06:03:55:04:06:13:02"
        "4e:4f:30:81:9f:30:0d:06:09:2a:86:48:86:f7:0d:01"
        "01:01:05:00:03:81:8d:00:30:81:89:02:81:81:00:eb"
        "9d:e9:03:ac:30:4f:a9:58:03:44:c7:18:26:2f:48:93"
        "d5:ac:a0:fb:e8:53:c4:7b:2a:01:89:e6:fc:5a:0c:c5"
        "f5:21:f8:d7:4a:92:02:67:db:f1:9f:36:9a:62:9d:f3"
        "ce:48:8e:ba:ed:5a:a8:9d:4f:bb:24:16:43:4c:b5:79"
        "08:f6:d9:22:8f:5f:15:0a:43:25:03:7a:9d:a7:af:e3"
        "26:b1:53:55:5e:60:57:c8:ed:2f:1c:f3:36:0a:78:64"
        "91:f9:17:a7:34:d7:8b:bd:f1:fc:d1:8c:4f:a5:96:75"
        "b2:7b:fc:21:f0:c7:d9:5f:0c:57:18:b2:af:b9:4b:02"
        "03:01:00:01:30:0d:06:09:2a:86:48:86:f7:0d:01:01"
        "05:05:00:03:81:81:00:95:e6:94:e2:98:33:57:a2:98"
        "fa:af:50:b9:76:a9:51:83:2c:0b:61:a2:36:d0:e6:90"
        "6d:e4:f8:c4:c7:50:ef:17:94:4e:21:a8:fa:c8:33:aa"
        "d1:7f:bc:ca:41:d6:7d:e7:44:76:c0:bf:45:4a:76:25"
        "42:6d:53:76:fd:fc:74:29:1a:ea:2b:cc:06:ab:d1:b8"
        "eb:7d:6b:11:f7:9b:41:bb:9f:31:cb:ed:4d:f3:68:26"
        "ed:13:1d:f2:56:59:fe:6f:7c:98:b6:25:69:4e:ea:b4"
        "dc:c2:eb:b7:bb:50:18:05:ba:ad:af:08:49:fe:98:63"
        "55:ba:e7:fb:95:5d:91";

    QSslCertificate cert =  QSslCertificate::fromPath("certificates/cert.pem", QSsl::Pem,
                                                      QRegExp::FixedString).first();
    QVERIFY(!cert.isNull());

    QCOMPARE(cert.issuerInfo(QSslCertificate::Organization)[0], QString("CryptSoft Pty Ltd"));
    QCOMPARE(cert.issuerInfo(QSslCertificate::CommonName)[0], QString("Test CA (1024 bit)"));
    QCOMPARE(cert.issuerInfo(QSslCertificate::LocalityName), QStringList());
    QCOMPARE(cert.issuerInfo(QSslCertificate::OrganizationalUnitName), QStringList());
    QCOMPARE(cert.issuerInfo(QSslCertificate::CountryName)[0], QString("AU"));
    QCOMPARE(cert.issuerInfo(QSslCertificate::StateOrProvinceName)[0], QString("Queensland"));

    QCOMPARE(cert.issuerInfo("O")[0], QString("CryptSoft Pty Ltd"));
    QCOMPARE(cert.issuerInfo("CN")[0], QString("Test CA (1024 bit)"));
    QCOMPARE(cert.issuerInfo("L"), QStringList());
    QCOMPARE(cert.issuerInfo("OU"), QStringList());
    QCOMPARE(cert.issuerInfo("C")[0], QString("AU"));
    QCOMPARE(cert.issuerInfo("ST")[0], QString("Queensland"));

    QCOMPARE(cert.subjectInfo(QSslCertificate::Organization), QStringList());
    QCOMPARE(cert.subjectInfo(QSslCertificate::CommonName)[0], QString("name/with/slashes"));
    QCOMPARE(cert.subjectInfo(QSslCertificate::LocalityName), QStringList());
    QCOMPARE(cert.subjectInfo(QSslCertificate::OrganizationalUnitName), QStringList());
    QCOMPARE(cert.subjectInfo(QSslCertificate::CountryName)[0], QString("NO"));
    QCOMPARE(cert.subjectInfo(QSslCertificate::StateOrProvinceName), QStringList());

    QCOMPARE(cert.subjectInfo("O"), QStringList());
    QCOMPARE(cert.subjectInfo("CN")[0], QString("name/with/slashes"));
    QCOMPARE(cert.subjectInfo("L"), QStringList());
    QCOMPARE(cert.subjectInfo("OU"), QStringList());
    QCOMPARE(cert.subjectInfo("C")[0], QString("NO"));
    QCOMPARE(cert.subjectInfo("ST"), QStringList());

    QCOMPARE(cert.version(), QByteArray::number(1));
    QCOMPARE(cert.serialNumber(), QByteArray("11"));

    QCOMPARE(cert.toPem().constData(), (const char*)pem);
    QCOMPARE(cert.toDer(), QByteArray::fromHex(der));

    QCOMPARE(cert.digest(QCryptographicHash::Md5),
             QByteArray::fromHex("B6:CF:57:34:DA:A9:73:21:82:F7:CF:4D:3D:85:31:88"));
    QCOMPARE(cert.digest(QCryptographicHash::Sha1),
             QByteArray::fromHex("B6:D1:51:82:E0:29:CA:59:96:38:BD:B6:F9:40:05:91:6D:49:09:60"));

    QCOMPARE(cert.effectiveDate().toUTC(), QDateTime(QDate(2007, 4, 17), QTime(7,40,26), Qt::UTC));
    QCOMPARE(cert.expiryDate().toUTC(), QDateTime(QDate(2007, 5, 17), QTime(7,40,26), Qt::UTC));
    QVERIFY(cert.expiryDate() < QDateTime::currentDateTime());   // cert has expired

    QSslCertificate copy = cert;
    QVERIFY(cert == copy);
    QVERIFY(!(cert != copy));

    QCOMPARE(cert, QSslCertificate(pem, QSsl::Pem));
    QCOMPARE(cert, QSslCertificate(QByteArray::fromHex(der), QSsl::Der));
}

void tst_QSslCertificate::certInfoQByteArray()
{
    QSslCertificate cert =  QSslCertificate::fromPath("certificates/cert.pem", QSsl::Pem,
                                                      QRegExp::FixedString).first();
    QVERIFY(!cert.isNull());

    // in this test, check the bytearray variants before the enum variants to see if
    // we fixed a bug we had with lazy initialization of the values.
    QCOMPARE(cert.issuerInfo("CN")[0], QString("Test CA (1024 bit)"));
    QCOMPARE(cert.subjectInfo("CN")[0], QString("name/with/slashes"));
}

void tst_QSslCertificate::task256066toPem()
{
    // a certificate whose PEM encoding's length is a multiple of 64
    const char *mycert = "-----BEGIN CERTIFICATE-----\n" \
                         "MIIEGjCCAwKgAwIBAgIESikYSjANBgkqhkiG9w0BAQUFADBbMQswCQYDVQQGEwJF\n" \
                         "RTEiMCAGA1UEChMZQVMgU2VydGlmaXRzZWVyaW1pc2tlc2t1czEPMA0GA1UECxMG\n" \
                         "RVNURUlEMRcwFQYDVQQDEw5FU1RFSUQtU0sgMjAwNzAeFw0wOTA2MDUxMzA2MTha\n" \
                         "Fw0xNDA2MDkyMTAwMDBaMIGRMQswCQYDVQQGEwJFRTEPMA0GA1UEChMGRVNURUlE\n" \
                         "MRcwFQYDVQQLEw5hdXRoZW50aWNhdGlvbjEhMB8GA1UEAxMYSEVJQkVSRyxTVkVO\n" \
                         "LDM3NzA5MjcwMjg1MRAwDgYDVQQEEwdIRUlCRVJHMQ0wCwYDVQQqEwRTVkVOMRQw\n" \
                         "EgYDVQQFEwszNzcwOTI3MDI4NTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEA\n" \
                         "k2Euwhm34vu1jOFp02J5fQRx9LW2C7x78CbJ7yInoAKn7QR8UdxTU7mJk90Opejo\n" \
                         "71RUi2/aYl4jCr9gr99v2YoLufMRwAuqdmwmwqH1WAHRUtIcD0oPdKyelmmn9ig0\n" \
                         "RV+yJLNT3dnyrwPw+uuzDe3DeKepGKE4lxexliCaAx0CAyCMW6OCATEwggEtMA4G\n" \
                         "A1UdDwEB/wQEAwIEsDAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwQwPAYD\n" \
                         "VR0fBDUwMzAxoC+gLYYraHR0cDovL3d3dy5zay5lZS9jcmxzL2VzdGVpZC9lc3Rl\n" \
                         "aWQyMDA3LmNybDAgBgNVHREEGTAXgRVzdmVuLmhlaWJlcmdAZWVzdGkuZWUwUQYD\n" \
                         "VR0gBEowSDBGBgsrBgEEAc4fAQEBATA3MBIGCCsGAQUFBwICMAYaBG5vbmUwIQYI\n" \
                         "KwYBBQUHAgEWFWh0dHA6Ly93d3cuc2suZWUvY3BzLzAfBgNVHSMEGDAWgBRIBt6+\n" \
                         "jIdXlYB4Y/qcIysroDoYdTAdBgNVHQ4EFgQUKCjpDf+LcvL6AH0QOiW6rMTtB/0w\n" \
                         "CQYDVR0TBAIwADANBgkqhkiG9w0BAQUFAAOCAQEABRyRuUm9zt8V27WuNeXtCDmU\n" \
                         "MGzA6g4QXNAd2nxFzT3k+kNzzQTOcgRdmjiEPuK49On+GWnBr/5MSBNhbCJVPWr/\n" \
                         "yym1UYTBisaqhRt/N/kwZqd0bHeLJk+ZxSePXRyqkp9H8KPWqz7H+O/FxRS4ffxo\n" \
                         "Q9Clem+e0bcjNlL5xXiRGycBeZq8cKj+0+A/UuattznQlvHdlCEsSeu1fPOORqFV\n" \
                         "fZur4HC31lQD7xVvETLiL83CtOQC78+29XPD6Zlrrc5OF2yibSVParY19b8Zh6yu\n" \
                         "p1dNvN8pBgXGrsyxRonwHooV2ghGNmGILkpdvlQfnxeCUg4erfHjDdSY9vmT7w==\n" \
                         "-----END CERTIFICATE-----\n";

    QByteArray pem1(mycert);
    QSslCertificate cert1(pem1);
    QVERIFY(!cert1.isNull());
    QByteArray pem2(cert1.toPem());
    QSslCertificate cert2(pem2);
    QVERIFY(!cert2.isNull());
    QCOMPARE(pem1, pem2);
}

void tst_QSslCertificate::nulInCN()
{
    QList<QSslCertificate> certList =
        QSslCertificate::fromPath(testDataDir + "/more-certificates/badguy-nul-cn.crt");
    QCOMPARE(certList.size(), 1);

    const QSslCertificate &cert = certList.at(0);
    QVERIFY(!cert.isNull());

    QString cn = cert.subjectInfo(QSslCertificate::CommonName)[0];
    QVERIFY(cn != "www.bank.com");

    static const char realCN[] = "www.bank.com\0.badguy.com";
    QCOMPARE(cn, QString::fromLatin1(realCN, sizeof realCN - 1));
}

void tst_QSslCertificate::nulInSan()
{
    QList<QSslCertificate> certList =
        QSslCertificate::fromPath(testDataDir + "/more-certificates/badguy-nul-san.crt");
    QCOMPARE(certList.size(), 1);

    const QSslCertificate &cert = certList.at(0);
    QVERIFY(!cert.isNull());

    QMultiMap<QSsl::AlternativeNameEntryType, QString> san = cert.subjectAlternativeNames();
    QVERIFY(!san.isEmpty());

    QString dnssan = san.value(QSsl::DnsEntry);
    QVERIFY(!dnssan.isEmpty());
    QVERIFY(dnssan != "www.bank.com");

    static const char realSAN[] = "www.bank.com\0www.badguy.com";
    QCOMPARE(dnssan, QString::fromLatin1(realSAN, sizeof realSAN - 1));
}

void tst_QSslCertificate::largeSerialNumber()
{
    QList<QSslCertificate> certList =
        QSslCertificate::fromPath(testDataDir + "/more-certificates/cert-large-serial-number.pem");

    QCOMPARE(certList.size(), 1);

    const QSslCertificate &cert = certList.at(0);
    QVERIFY(!cert.isNull());
    QCOMPARE(cert.serialNumber(), QByteArray("01:02:03:04:05:06:07:08:09:10:aa:bb:cc:dd:ee:ff:17:18:19:20"));
}

void tst_QSslCertificate::largeExpirationDate() // QTBUG-12489
{
    QList<QSslCertificate> certList =
        QSslCertificate::fromPath(testDataDir + "/more-certificates/cert-large-expiration-date.pem");

    QCOMPARE(certList.size(), 1);

    const QSslCertificate &cert = certList.at(0);
    QVERIFY(!cert.isNull());
    QCOMPARE(cert.effectiveDate().toUTC(), QDateTime(QDate(2010, 8, 4), QTime(9, 53, 41), Qt::UTC));
    // if the date is larger than 2049, then the generalized time format is used
    QCOMPARE(cert.expiryDate().toUTC(), QDateTime(QDate(2051, 8, 29), QTime(9, 53, 41), Qt::UTC));
}

void tst_QSslCertificate::blacklistedCertificates()
{
    QList<QSslCertificate> blacklistedCerts = QSslCertificate::fromPath("more-certificates/blacklisted*.pem", QSsl::Pem, QRegExp::Wildcard);
    QVERIFY2(blacklistedCerts.count() > 0, "Please run this test from the source directory");
    for (int a = 0; a < blacklistedCerts.count(); a++) {
        QVERIFY(blacklistedCerts.at(a).isBlacklisted());
    }
}

void tst_QSslCertificate::toText()
{
    QList<QSslCertificate> certList =
        QSslCertificate::fromPath(testDataDir + "/more-certificates/cert-large-expiration-date.pem");

    QCOMPARE(certList.size(), 1);
    const QSslCertificate &cert = certList.at(0);

    // Openssl's cert dump method changed slightly between 0.9.8, 1.0.0 and 1.01 versions, so we want it to match any output

    QFile f098(testDataDir + "/more-certificates/cert-large-expiration-date.txt.0.9.8");
    QVERIFY(f098.open(QIODevice::ReadOnly | QFile::Text));
    QByteArray txt098 = f098.readAll();

    QFile f100(testDataDir + "/more-certificates/cert-large-expiration-date.txt.1.0.0");
    QVERIFY(f100.open(QIODevice::ReadOnly | QFile::Text));
    QByteArray txt100 = f100.readAll();

    QFile f101(testDataDir + "/more-certificates/cert-large-expiration-date.txt.1.0.1");
    QVERIFY(f101.open(QIODevice::ReadOnly | QFile::Text));
    QByteArray txt101 = f101.readAll();

    QFile f101c(testDataDir + "/more-certificates/cert-large-expiration-date.txt.1.0.1c");
    QVERIFY(f101c.open(QIODevice::ReadOnly | QFile::Text));
    QByteArray txt101c = f101c.readAll();

    QString txtcert = cert.toText();

    QVERIFY(QString::fromLatin1(txt098) == txtcert ||
            QString::fromLatin1(txt100) == txtcert ||
            QString::fromLatin1(txt101) == txtcert ||
            QString::fromLatin1(txt101c) == txtcert );
}

void tst_QSslCertificate::multipleCommonNames()
{
    QList<QSslCertificate> certList =
        QSslCertificate::fromPath(testDataDir + "/more-certificates/test-cn-two-cns-cert.pem");
    QVERIFY2(certList.count() > 0, "Please run this test from the source directory");

    QStringList commonNames = certList[0].subjectInfo(QSslCertificate::CommonName);
    QVERIFY(commonNames.contains(QString("www.example.com")));
    QVERIFY(commonNames.contains(QString("www2.example.com")));
}

void tst_QSslCertificate::subjectAndIssuerAttributes()
{
    QList<QSslCertificate> certList =
        QSslCertificate::fromPath(testDataDir + "/more-certificates/test-cn-with-drink-cert.pem");
    QVERIFY2(certList.count() > 0, "Please run this test from the source directory");

    QList<QByteArray> attributes = certList[0].subjectInfoAttributes();
    QVERIFY(attributes.contains(QByteArray("favouriteDrink")));
    attributes.clear();

    certList = QSslCertificate::fromPath(testDataDir + "/more-certificates/natwest-banking.pem");
    QVERIFY2(certList.count() > 0, "Please run this test from the source directory");

    attributes = certList[0].subjectInfoAttributes();
    QVERIFY(attributes.contains(QByteArray("1.3.6.1.4.1.311.60.2.1.3")));
}

void tst_QSslCertificate::verify()
{
    QList<QSslError> errors;
    QList<QSslCertificate> toVerify;

    // Like QVERIFY, but be verbose about the content of `errors' when failing
#define VERIFY_VERBOSE(A)                                       \
    QVERIFY2((A),                                               \
        qPrintable(QString("errors: %1").arg(toString(errors))) \
    )

    // Empty chain is unspecified error
    errors = QSslCertificate::verify(toVerify);
    VERIFY_VERBOSE(errors.count() == 1);
    VERIFY_VERBOSE(errors[0] == QSslError(QSslError::UnspecifiedError));
    errors.clear();

    // Verify a valid cert signed by a CA
    QList<QSslCertificate> caCerts = QSslCertificate::fromPath(testDataDir + "/verify-certs/cacert.pem");
    QSslSocket::addDefaultCaCertificate(caCerts.first());

    toVerify = QSslCertificate::fromPath(testDataDir + "/verify-certs/test-ocsp-good-cert.pem");

    errors = QSslCertificate::verify(toVerify);
    VERIFY_VERBOSE(errors.count() == 0);
    errors.clear();

    // Test a blacklisted certificate
    toVerify = QSslCertificate::fromPath(testDataDir + "/verify-certs/test-addons-mozilla-org-cert.pem");
    errors = QSslCertificate::verify(toVerify);
    bool foundBlack = false;
    foreach (const QSslError &error, errors) {
        if (error.error() == QSslError::CertificateBlacklisted) {
            foundBlack = true;
            break;
        }
    }
    QVERIFY(foundBlack);
    errors.clear();

    // This one is expired and untrusted
    toVerify = QSslCertificate::fromPath(testDataDir + "/more-certificates/cert-large-serial-number.pem");
    errors = QSslCertificate::verify(toVerify);
    VERIFY_VERBOSE(errors.contains(QSslError(QSslError::SelfSignedCertificate, toVerify[0])));
    VERIFY_VERBOSE(errors.contains(QSslError(QSslError::CertificateExpired, toVerify[0])));
    errors.clear();
    toVerify.clear();

    // This one is signed by a valid cert, but the signer is not a valid CA
    toVerify << QSslCertificate::fromPath(testDataDir + "/verify-certs/test-intermediate-not-ca-cert.pem").first();
    toVerify << QSslCertificate::fromPath(testDataDir + "/verify-certs/test-ocsp-good-cert.pem").first();
    errors = QSslCertificate::verify(toVerify);
    VERIFY_VERBOSE(errors.contains(QSslError(QSslError::InvalidCaCertificate, toVerify[1])));
    toVerify.clear();

    // This one is signed by a valid cert, and the signer is a valid CA
    toVerify << QSslCertificate::fromPath(testDataDir + "/verify-certs/test-intermediate-is-ca-cert.pem").first();
    toVerify << QSslCertificate::fromPath(testDataDir + "/verify-certs/test-intermediate-ca-cert.pem").first();
    errors = QSslCertificate::verify(toVerify);
    VERIFY_VERBOSE(errors.count() == 0);

    // Recheck the above with hostname validation
    errors = QSslCertificate::verify(toVerify, QLatin1String("example.com"));
    VERIFY_VERBOSE(errors.count() == 0);

    // Recheck the above with a bad hostname
    errors = QSslCertificate::verify(toVerify, QLatin1String("fail.example.com"));
    VERIFY_VERBOSE(errors.contains(QSslError(QSslError::HostNameMismatch, toVerify[0])));
    toVerify.clear();

#undef VERIFY_VERBOSE
}

QString tst_QSslCertificate::toString(const QList<QSslError>& errors)
{
    QStringList errorStrings;

    foreach (const QSslError& error, errors) {
        errorStrings.append(QLatin1String("\"") + error.errorString() + QLatin1String("\""));
    }

    return QLatin1String("[ ") + errorStrings.join(QLatin1String(", ")) + QLatin1String(" ]");
}

void tst_QSslCertificate::extensions()
{
    QList<QSslCertificate> certList =
        QSslCertificate::fromPath(testDataDir + "/more-certificates/natwest-banking.pem");
    QVERIFY2(certList.count() > 0, "Please run this test from the source directory");

    QSslCertificate cert = certList[0];
    QList<QSslCertificateExtension> extensions = cert.extensions();
    QVERIFY(extensions.count() == 9);

    int unknown_idx = -1;
    int authority_info_idx = -1;
    int basic_constraints_idx = -1;
    int subject_key_idx = -1;
    int auth_key_idx = -1;

    for (int i=0; i < extensions.length(); ++i) {
        QSslCertificateExtension ext = extensions[i];

        //qDebug() << i << ":" << ext.name() << ext.oid();
        if (ext.oid() == QStringLiteral("1.3.6.1.5.5.7.1.12"))
            unknown_idx = i;
        if (ext.name() == QStringLiteral("authorityInfoAccess"))
            authority_info_idx = i;
        if (ext.name() == QStringLiteral("basicConstraints"))
            basic_constraints_idx = i;
        if (ext.name() == QStringLiteral("subjectKeyIdentifier"))
            subject_key_idx = i;
        if (ext.name() == QStringLiteral("authorityKeyIdentifier"))
            auth_key_idx = i;
    }

    QVERIFY(unknown_idx != -1);
    QVERIFY(authority_info_idx != -1);
    QVERIFY(basic_constraints_idx != -1);
    QVERIFY(subject_key_idx != -1);
    QVERIFY(auth_key_idx != -1);

    // Unknown
    QSslCertificateExtension unknown = extensions[unknown_idx];
    QVERIFY(unknown.oid() == QStringLiteral("1.3.6.1.5.5.7.1.12"));
    QVERIFY(unknown.name() == QStringLiteral("1.3.6.1.5.5.7.1.12"));

    QByteArray unknownValue = QByteArray::fromHex(
                        "3060A15EA05C305A305830561609696D6167652F6769663021301F300706052B0E03021A0414" \
                        "4B6BB92896060CBBD052389B29AC4B078B21051830261624687474703A2F2F6C6F676F2E7665" \
                        "72697369676E2E636F6D2F76736C6F676F312E676966");
    QCOMPARE(unknown.value().toByteArray(), unknownValue);

    // Authority Info Access
    QSslCertificateExtension aia = extensions[authority_info_idx];
    QVERIFY(aia.oid() == QStringLiteral("1.3.6.1.5.5.7.1.1"));
    QVERIFY(aia.name() == QStringLiteral("authorityInfoAccess"));

    QVariantMap aiaValue = aia.value().toMap();
    QString ocsp = aiaValue[QStringLiteral("OCSP")].toString();
    QString caIssuers = aiaValue[QStringLiteral("caIssuers")].toString();

    QVERIFY(ocsp == QStringLiteral("http://EVIntl-ocsp.verisign.com"));
    QVERIFY(caIssuers == QStringLiteral("http://EVIntl-aia.verisign.com/EVIntl2006.cer"));

    // Basic constraints
    QSslCertificateExtension basic = extensions[basic_constraints_idx];
    QVERIFY(basic.oid() == QStringLiteral("2.5.29.19"));
    QVERIFY(basic.name() == QStringLiteral("basicConstraints"));

    QVariantMap basicValue = basic.value().toMap();
    QVERIFY(basicValue[QStringLiteral("ca")].toBool() == false);

    // Subject key identifier
    QSslCertificateExtension subjectKey = extensions[subject_key_idx];
    QVERIFY(subjectKey.oid() == QStringLiteral("2.5.29.14"));
    QVERIFY(subjectKey.name() == QStringLiteral("subjectKeyIdentifier"));
    QVERIFY(subjectKey.value().toString() == QStringLiteral("5F:90:23:CD:24:CA:52:C9:36:29:F0:7E:9D:B1:FE:08:E0:EE:69:F0"));

    // Authority key identifier
    QSslCertificateExtension authKey = extensions[auth_key_idx];
    QVERIFY(authKey.oid() == QStringLiteral("2.5.29.35"));
    QVERIFY(authKey.name() == QStringLiteral("authorityKeyIdentifier"));

    QVariantMap authValue = authKey.value().toMap();
    QVERIFY(authValue[QStringLiteral("keyid")].toByteArray() ==
            QByteArray("4e43c81d76ef37537a4ff2586f94f338e2d5bddf"));

}

class TestThread : public QThread
{
public:
    void run()
    {
        effectiveDate = cert.effectiveDate();
        expiryDate = cert.expiryDate();
        extensions = cert.extensions();
        isBlacklisted = cert.isBlacklisted();
        issuerInfo = cert.issuerInfo(QSslCertificate::CommonName);
        issuerInfoAttributes = cert.issuerInfoAttributes();
        publicKey = cert.publicKey();
        serialNumber = cert.serialNumber();
        subjectInfo = cert.subjectInfo(QSslCertificate::CommonName);
        subjectInfoAttributes = cert.subjectInfoAttributes();
        toDer = cert.toDer();
        toPem = cert.toPem();
        toText = cert.toText();
        version = cert.version();
    }
    QSslCertificate cert;
    QDateTime effectiveDate;
    QDateTime expiryDate;
    QList<QSslCertificateExtension> extensions;
    bool isBlacklisted;
    QStringList issuerInfo;
    QList<QByteArray> issuerInfoAttributes;
    QSslKey publicKey;
    QByteArray serialNumber;
    QStringList subjectInfo;
    QList<QByteArray> subjectInfoAttributes;
    QByteArray toDer;
    QByteArray toPem;
    QString toText;
    QByteArray version;
};

void tst_QSslCertificate::threadSafeConstMethods()
{
    if (!QSslSocket::supportsSsl())
        return;

    QByteArray encoded = readFile(testDataDir + "/certificates/cert.pem");
    QSslCertificate certificate(encoded);
    QVERIFY(!certificate.isNull());

    TestThread t1;
    t1.cert = certificate; //shallow copy
    TestThread t2;
    t2.cert = certificate; //shallow copy
    t1.start();
    t2.start();
    QVERIFY(t1.wait(5000));
    QVERIFY(t2.wait(5000));
    QVERIFY(t1.cert == t2.cert);
    QVERIFY(t1.effectiveDate == t2.effectiveDate);
    QVERIFY(t1.expiryDate == t2.expiryDate);
    //QVERIFY(t1.extensions == t2.extensions); // no equality operator, so not tested
    QVERIFY(t1.isBlacklisted == t2.isBlacklisted);
    QVERIFY(t1.issuerInfo == t2.issuerInfo);
    QVERIFY(t1.issuerInfoAttributes == t2.issuerInfoAttributes);
    QVERIFY(t1.publicKey == t2.publicKey);
    QVERIFY(t1.serialNumber == t2.serialNumber);
    QVERIFY(t1.subjectInfo == t2.subjectInfo);
    QVERIFY(t1.subjectInfoAttributes == t2.subjectInfoAttributes);
    QVERIFY(t1.toDer == t2.toDer);
    QVERIFY(t1.toPem == t2.toPem);
    QVERIFY(t1.toText == t2.toText);
    QVERIFY(t1.version == t2.version);

}

void tst_QSslCertificate::version_data()
{
    QTest::addColumn<QSslCertificate>("certificate");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("null certificate") << QSslCertificate() << QByteArray();

    QList<QSslCertificate> certs;
    certs << QSslCertificate::fromPath(testDataDir + "/verify-certs/test-ocsp-good-cert.pem");

    QTest::newRow("v3 certificate") << certs.first() << QByteArrayLiteral("3");

    certs.clear();
    certs << QSslCertificate::fromPath(testDataDir + "/certificates/cert.pem");
    QTest::newRow("v1 certificate") << certs.first() << QByteArrayLiteral("1");
}

void tst_QSslCertificate::version()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QSslCertificate, certificate);
    QFETCH(QByteArray, result);
    QCOMPARE(certificate.version(), result);
}

#endif // QT_NO_SSL

QTEST_MAIN(tst_QSslCertificate)
#include "tst_qsslcertificate.moc"
