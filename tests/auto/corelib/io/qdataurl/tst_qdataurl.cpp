// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "private/qdataurl_p.h"
#include <QTest>
#include <QtCore/QDebug>

using namespace Qt::Literals;

class tst_QDataUrl : public QObject
{
    Q_OBJECT

private slots:
    void decode_data();
    void decode();
};

void tst_QDataUrl::decode_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("result");
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<QByteArray>("payload");

    auto row = [](const char *tag, const char *url, bool success, QString mimeType = {}, QByteArray payload = {}) {
        QTest::newRow(tag) << url << success <<mimeType << payload;
    };

    row("nonData", "http://test.com", false);
    row("emptyData", "data:text/plain", true,
        "text/plain;charset=US-ASCII"_L1);
    row("alreadyPercentageEncoded", "data:text/plain,%E2%88%9A", true,
        "text/plain"_L1, QByteArray::fromPercentEncoding("%E2%88%9A"));
    row("everythingIsCaseInsensitive", "Data:texT/PlaiN;charSet=iSo-8859-1;Base64,SGVsbG8=", true,
        "texT/PlaiN;charSet=iSo-8859-1"_L1, QByteArrayLiteral("Hello"));
}

void tst_QDataUrl::decode()
{
    QFETCH(const QString, input);
    QFETCH(const bool, result);
    QFETCH(const QString, mimeType);
    QFETCH(const QByteArray, payload);

    QString actualMimeType;
    QByteArray actualPayload;

    QUrl url(input);
    const bool actualResult = qDecodeDataUrl(url, actualMimeType, actualPayload);

    QCOMPARE(actualResult, result);
    QCOMPARE(actualMimeType, mimeType);
    QCOMPARE(actualPayload, payload);
    QCOMPARE(actualPayload.isNull(), payload.isNull()); // assume nullness is significant
}

QTEST_MAIN(tst_QDataUrl)
#include "tst_qdataurl.moc"
