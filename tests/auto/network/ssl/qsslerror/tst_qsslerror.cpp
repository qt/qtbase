// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtNetwork/qtnetworkglobal.h>

#include <qsslcertificate.h>
#include <qsslerror.h>

#include <QTest>
#include <QTestEventLoop>
#include <QtCore/qmetaobject.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qset.h>

QT_USE_NAMESPACE

const QByteArray certificateBytes =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEjjCCBDOgAwIBAgIQCQsKtxCf9ik3vIVQ+PMa5TAKBggqhkjOPQQDAjBKMQsw\n"
    "CQYDVQQGEwJVUzEZMBcGA1UEChMQQ2xvdWRmbGFyZSwgSW5jLjEgMB4GA1UEAxMX\n"
    "Q2xvdWRmbGFyZSBJbmMgRUNDIENBLTMwHhcNMjAwODE2MDAwMDAwWhcNMjEwODE2\n"
    "MTIwMDAwWjBhMQswCQYDVQQGEwJVUzELMAkGA1UECBMCQ0ExFjAUBgNVBAcTDVNh\n"
    "biBGcmFuY2lzY28xGTAXBgNVBAoTEENsb3VkZmxhcmUsIEluYy4xEjAQBgNVBAMT\n"
    "CXd3dy5xdC5pbzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABP/r0xH22wdU8fLk\n"
    "RsXhxRj5fmUNUo7rxnUl3lyqYYp53cLvn3agQifXkegpE8Xv4pGmuyWZj85FtoeZ\n"
    "UZh8iyCjggLiMIIC3jAfBgNVHSMEGDAWgBSlzjfq67B1DpRniLRF+tkkEIeWHzAd\n"
    "BgNVHQ4EFgQU7qPYGi9VtC4/6MS+54LNEAXApBgwFAYDVR0RBA0wC4IJd3d3LnF0\n"
    "LmlvMA4GA1UdDwEB/wQEAwIHgDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUH\n"
    "AwIwewYDVR0fBHQwcjA3oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL0Ns\n"
    "b3VkZmxhcmVJbmNFQ0NDQS0zLmNybDA3oDWgM4YxaHR0cDovL2NybDQuZGlnaWNl\n"
    "cnQuY29tL0Nsb3VkZmxhcmVJbmNFQ0NDQS0zLmNybDBMBgNVHSAERTBDMDcGCWCG\n"
    "SAGG/WwBATAqMCgGCCsGAQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5jb20v\n"
    "Q1BTMAgGBmeBDAECAjB2BggrBgEFBQcBAQRqMGgwJAYIKwYBBQUHMAGGGGh0dHA6\n"
    "Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBABggrBgEFBQcwAoY0aHR0cDovL2NhY2VydHMu\n"
    "ZGlnaWNlcnQuY29tL0Nsb3VkZmxhcmVJbmNFQ0NDQS0zLmNydDAMBgNVHRMBAf8E\n"
    "AjAAMIIBBAYKKwYBBAHWeQIEAgSB9QSB8gDwAHYA9lyUL9F3MCIUVBgIMJRWjuNN\n"
    "Exkzv98MLyALzE7xZOMAAAFz90PlSQAABAMARzBFAiAhrrtxdmuxpCy8HAJJ5Qkg\n"
    "WNvlo8nZqfe6pqGUcz0dmwIhAOMqDtd5ZhcfRk96GAJxPm8bH4hDnmqDP/zJG2Mq\n"
    "nFpMAHYAXNxDkv7mq0VEsV6a1FbmEDf71fpH3KFzlLJe5vbHDsoAAAFz90PlewAA\n"
    "BAMARzBFAiB/EkdY10LDdaRcf6eSc/QxucxU+2PI+3pWjh/21A8ZUAIhAK2Qz9Kw\n"
    "onlRNyHpV3E6qyVydkXihj3c3q5UclpURYcmMAoGCCqGSM49BAMCA0kAMEYCIQDz\n"
    "K/lzLb2Rbeg1HErRLLm2HkJUmfOGU2+tbROSTGK8ugIhAKA+MKqaZ8VjPxQ+Ho4v\n"
    "fuwccvZfkU/fg8tAHTOzX23v\n"
    "-----END CERTIFICATE-----";

class tst_QSslError : public QObject
{
    Q_OBJECT
private slots:
    void constructing();
    void nonDefaultConstructors();
    void hash();
};

void tst_QSslError::constructing()
{
    const QSslError error;
    QCOMPARE(error.error(), QSslError::NoError);
    QCOMPARE(error.errorString(), QStringLiteral("No error"));
    QVERIFY(error.certificate().isNull());
}

void tst_QSslError::nonDefaultConstructors()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("This test requires a working TLS library");

    const auto chain = QSslCertificate::fromData(certificateBytes);
    QCOMPARE(chain.size(), 1);
    const auto certificate = chain.at(0);
    QVERIFY(!certificate.isNull());

    const auto visitor = QSslError::staticMetaObject;
    const int nEnums = visitor.enumeratorCount();
    QMetaEnum errorCodesEnum;
    for (int i = 0; i < nEnums; ++i) {
        const auto metaEnum = visitor.enumerator(i);
        if (metaEnum.enumName() == QStringLiteral("SslError")) {
            errorCodesEnum = metaEnum;
            break;
        }
    }

    QCOMPARE(errorCodesEnum.enumName(), QStringLiteral("SslError"));
    for (int i = 0, e = errorCodesEnum.keyCount(); i < e; ++i) {
        const int value = errorCodesEnum.value(i);
        if (value == -1) {
            QVERIFY(i);
            break;
        }
        const auto errorCode = QSslError::SslError(value);
        QSslError error(errorCode);

        const auto basicChecks = [](const QSslError &err, QSslError::SslError code) {
            QCOMPARE(err.error(), code);
            const auto errorString = err.errorString();
            if (code == QSslError::NoError)
                QCOMPARE(errorString, QStringLiteral("No error"));
            else
                QVERIFY(errorString != QStringLiteral("No error"));
        };

        basicChecks(error, errorCode);

        // ;)
        error = QSslError(errorCode, certificate);

        basicChecks(error, errorCode);
        QVERIFY(!error.certificate().isNull());
    }
}

void tst_QSslError::hash()
{
    // mostly a compile-only test, to check that qHash(QSslError) is found
    QSet<QSslError> errors;
    errors << QSslError();
    QCOMPARE(errors.size(), 1);
}

QTEST_MAIN(tst_QSslError)
#include "tst_qsslerror.moc"
