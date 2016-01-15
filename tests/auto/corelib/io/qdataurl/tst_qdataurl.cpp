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
#include <QtTest/QtTest>
#include <QtCore/QDebug>

class tst_QDataUrl : public QObject
{
    Q_OBJECT

private slots:
    void nonData();
    void emptyData();
    void alreadyPercentageEncoded();
};

void tst_QDataUrl::nonData()
{
    QLatin1String data("http://test.com");
    QUrl url(data);
    QString mimeType;
    QByteArray payload;
    bool result = qDecodeDataUrl(url, mimeType, payload);
    QVERIFY(!result);
}

void tst_QDataUrl::emptyData()
{
    QLatin1String data("data:text/plain");
    QUrl url(data);
    QString mimeType;
    QByteArray payload;
    bool result = qDecodeDataUrl(url, mimeType, payload);
    QVERIFY(result);
    QCOMPARE(mimeType, QLatin1String("text/plain;charset=US-ASCII"));
    QVERIFY(payload.isNull());
}

void tst_QDataUrl::alreadyPercentageEncoded()
{
    QLatin1String data("data:text/plain,%E2%88%9A");
    QUrl url(data);
    QString mimeType;
    QByteArray payload;
    bool result = qDecodeDataUrl(url, mimeType, payload);
    QVERIFY(result);
    QCOMPARE(mimeType, QLatin1String("text/plain"));
    QCOMPARE(payload, QByteArray::fromPercentEncoding("%E2%88%9A"));
}

QTEST_MAIN(tst_QDataUrl)
#include "tst_qdataurl.moc"
