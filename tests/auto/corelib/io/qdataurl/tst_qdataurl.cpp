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

#include "private/qdataurl_p.h"
#include <QTest>
#include <QtCore/QDebug>

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
        QLatin1String("text/plain;charset=US-ASCII"));
    row("alreadyPercentageEncoded", "data:text/plain,%E2%88%9A", true,
        QLatin1String("text/plain"), QByteArray::fromPercentEncoding("%E2%88%9A"));
    row("everythingIsCaseInsensitive", "Data:texT/PlaiN;charSet=iSo-8859-1;Base64,SGVsbG8=", true,
        QLatin1String("texT/PlaiN;charSet=iSo-8859-1"), QByteArrayLiteral("Hello"));
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
