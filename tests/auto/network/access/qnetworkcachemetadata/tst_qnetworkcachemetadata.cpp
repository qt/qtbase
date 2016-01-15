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
#include <qabstractnetworkcache.h>

#define EXAMPLE_URL "http://user:pass@www.example.com/#foo"

class tst_QNetworkCacheMetaData : public QObject
{
    Q_OBJECT

private slots:
    void qnetworkcachemetadata_data();
    void qnetworkcachemetadata();

    void expirationDate_data();
    void expirationDate();
    void isValid_data();
    void isValid();
    void lastModified_data();
    void lastModified();
    void operatorEqual_data();
    void operatorEqual();
    void operatorEqualEqual_data();
    void operatorEqualEqual();
    void rawHeaders_data();
    void rawHeaders();
    void saveToDisk_data();
    void saveToDisk();
    void url_data();
    void url();

    void stream();
};

// Subclass that exposes the protected functions.
class SubQNetworkCacheMetaData : public QNetworkCacheMetaData
{
public:};

void tst_QNetworkCacheMetaData::qnetworkcachemetadata_data()
{
}

void tst_QNetworkCacheMetaData::qnetworkcachemetadata()
{
    QNetworkCacheMetaData data;
    QCOMPARE(data.expirationDate(), QDateTime());
    QCOMPARE(data.isValid(), false);
    QCOMPARE(data.lastModified(), QDateTime());
    QCOMPARE(data.operator!=(QNetworkCacheMetaData()), false);
    QNetworkCacheMetaData metaData;
    QCOMPARE(data.operator=(metaData), QNetworkCacheMetaData());
    QCOMPARE(data.operator==(QNetworkCacheMetaData()), true);
    QCOMPARE(data.rawHeaders(), QNetworkCacheMetaData::RawHeaderList());
    QCOMPARE(data.saveToDisk(), true);
    QCOMPARE(data.url(), QUrl());
    data.setExpirationDate(QDateTime());
    data.setLastModified(QDateTime());
    data.setRawHeaders(QNetworkCacheMetaData::RawHeaderList());
    data.setSaveToDisk(false);
    data.setUrl(QUrl());
}

void tst_QNetworkCacheMetaData::expirationDate_data()
{
    QTest::addColumn<QDateTime>("expirationDate");
    QTest::newRow("null") << QDateTime();
    QTest::newRow("now") << QDateTime::currentDateTime();
}

// public QDateTime expirationDate() const
void tst_QNetworkCacheMetaData::expirationDate()
{
    QFETCH(QDateTime, expirationDate);

    SubQNetworkCacheMetaData data;

    data.setExpirationDate(expirationDate);
    QCOMPARE(data.expirationDate(), expirationDate);
}

Q_DECLARE_METATYPE(QNetworkCacheMetaData)
void tst_QNetworkCacheMetaData::isValid_data()
{
    QTest::addColumn<QNetworkCacheMetaData>("data");
    QTest::addColumn<bool>("isValid");

    QNetworkCacheMetaData metaData;
    QTest::newRow("null") << metaData << false;

    QNetworkCacheMetaData data1;
    data1.setUrl(QUrl(EXAMPLE_URL));
    QTest::newRow("valid-1") << data1 << true;

    QNetworkCacheMetaData data2;
    QNetworkCacheMetaData::RawHeaderList headers;
    headers.append(QNetworkCacheMetaData::RawHeader("foo", "Bar"));
    data2.setRawHeaders(headers);
    QTest::newRow("valid-2") << data2 << true;

    QNetworkCacheMetaData data3;
    data3.setLastModified(QDateTime::currentDateTime());
    QTest::newRow("valid-3") << data3 << true;

    QNetworkCacheMetaData data4;
    data4.setExpirationDate(QDateTime::currentDateTime());
    QTest::newRow("valid-4") << data4 << true;

    QNetworkCacheMetaData data5;
    data5.setSaveToDisk(false);
    QTest::newRow("valid-5") << data5 << true;
}

// public bool isValid() const
void tst_QNetworkCacheMetaData::isValid()
{
    QFETCH(QNetworkCacheMetaData, data);
    QFETCH(bool, isValid);

    QCOMPARE(data.isValid(), isValid);
}

void tst_QNetworkCacheMetaData::lastModified_data()
{
    QTest::addColumn<QDateTime>("lastModified");
    QTest::newRow("null") << QDateTime();
    QTest::newRow("now") << QDateTime::currentDateTime();
}

// public QDateTime lastModified() const
void tst_QNetworkCacheMetaData::lastModified()
{
    QFETCH(QDateTime, lastModified);

    SubQNetworkCacheMetaData data;

    data.setLastModified(lastModified);
    QCOMPARE(data.lastModified(), lastModified);
}

void tst_QNetworkCacheMetaData::operatorEqual_data()
{
    QTest::addColumn<QNetworkCacheMetaData>("other");
    QTest::newRow("null") << QNetworkCacheMetaData();

    QNetworkCacheMetaData data;
    data.setUrl(QUrl(EXAMPLE_URL));
    QNetworkCacheMetaData::RawHeaderList headers;
    headers.append(QNetworkCacheMetaData::RawHeader("foo", "Bar"));
    data.setRawHeaders(headers);
    data.setLastModified(QDateTime::currentDateTime());
    data.setExpirationDate(QDateTime::currentDateTime());
    data.setSaveToDisk(false);
    QTest::newRow("valid") << data;
}

// public QNetworkCacheMetaData& operator=(QNetworkCacheMetaData const& other)
void tst_QNetworkCacheMetaData::operatorEqual()
{
    QFETCH(QNetworkCacheMetaData, other);

    QNetworkCacheMetaData data = other;

    QCOMPARE(data, other);
}

void tst_QNetworkCacheMetaData::operatorEqualEqual_data()
{
    QTest::addColumn<QNetworkCacheMetaData>("a");
    QTest::addColumn<QNetworkCacheMetaData>("b");
    QTest::addColumn<bool>("operatorEqualEqual");
    QTest::newRow("null") << QNetworkCacheMetaData() << QNetworkCacheMetaData() << true;

    QNetworkCacheMetaData data1;
    data1.setUrl(QUrl(EXAMPLE_URL));
    QTest::newRow("valid-1-1") << data1 << QNetworkCacheMetaData() << false;
    QTest::newRow("valid-1-2") << data1 << data1 << true;

    QNetworkCacheMetaData data2;
    QNetworkCacheMetaData::RawHeaderList headers;
    headers.append(QNetworkCacheMetaData::RawHeader("foo", "Bar"));
    data2.setRawHeaders(headers);
    QTest::newRow("valid-2-1") << data2 << QNetworkCacheMetaData() << false;
    QTest::newRow("valid-2-2") << data2 << data2 << true;
    QTest::newRow("valid-2-3") << data2 << data1 << false;

    QNetworkCacheMetaData data3;
    data3.setLastModified(QDateTime::currentDateTime());
    QTest::newRow("valid-3-1") << data3 << QNetworkCacheMetaData() << false;
    QTest::newRow("valid-3-2") << data3 << data3 << true;
    QTest::newRow("valid-3-3") << data3 << data1 << false;
    QTest::newRow("valid-3-4") << data3 << data2 << false;

    QNetworkCacheMetaData data4;
    data4.setExpirationDate(QDateTime::currentDateTime());
    QTest::newRow("valid-4-1") << data4 << QNetworkCacheMetaData() << false;
    QTest::newRow("valid-4-2") << data4 << data4 << true;
    QTest::newRow("valid-4-3") << data4 << data1 << false;
    QTest::newRow("valid-4-4") << data4 << data2 << false;
    QTest::newRow("valid-4-5") << data4 << data3 << false;

    QNetworkCacheMetaData data5;
    data5.setSaveToDisk(false);
    QTest::newRow("valid-5-1") << data5 << QNetworkCacheMetaData() << false;
    QTest::newRow("valid-5-2") << data5 << data5 << true;
    QTest::newRow("valid-5-3") << data5 << data1 << false;
    QTest::newRow("valid-5-4") << data5 << data2 << false;
    QTest::newRow("valid-5-5") << data5 << data3 << false;
    QTest::newRow("valid-5-6") << data5 << data4 << false;
}

// public bool operator==(QNetworkCacheMetaData const& other) const
void tst_QNetworkCacheMetaData::operatorEqualEqual()
{
    QFETCH(QNetworkCacheMetaData, a);
    QFETCH(QNetworkCacheMetaData, b);
    QFETCH(bool, operatorEqualEqual);

    QCOMPARE(a == b, operatorEqualEqual);
}

Q_DECLARE_METATYPE(QNetworkCacheMetaData::RawHeaderList)
void tst_QNetworkCacheMetaData::rawHeaders_data()
{
    QTest::addColumn<QNetworkCacheMetaData::RawHeaderList>("rawHeaders");
    QTest::newRow("null") << QNetworkCacheMetaData::RawHeaderList();
    QNetworkCacheMetaData::RawHeaderList headers;
    headers.append(QNetworkCacheMetaData::RawHeader("foo", "Bar"));
    QTest::newRow("valie") << headers;
}

// public QNetworkCacheMetaData::RawHeaderList rawHeaders() const
void tst_QNetworkCacheMetaData::rawHeaders()
{
    QFETCH(QNetworkCacheMetaData::RawHeaderList, rawHeaders);

    SubQNetworkCacheMetaData data;

    data.setRawHeaders(rawHeaders);
    QCOMPARE(data.rawHeaders(), rawHeaders);
}

void tst_QNetworkCacheMetaData::saveToDisk_data()
{
    QTest::addColumn<bool>("saveToDisk");
    QTest::newRow("false") << false;
    QTest::newRow("true") << true;
}

// public bool saveToDisk() const
void tst_QNetworkCacheMetaData::saveToDisk()
{
    QFETCH(bool, saveToDisk);

    SubQNetworkCacheMetaData data;

    data.setSaveToDisk(saveToDisk);
    QCOMPARE(data.saveToDisk(), saveToDisk);
}

void tst_QNetworkCacheMetaData::url_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QUrl>("expected");
    QTest::newRow("null") << QUrl() << QUrl();
    QTest::newRow("valid") << QUrl(EXAMPLE_URL) << QUrl("http://user@www.example.com/");
}

// public QUrl url() const
void tst_QNetworkCacheMetaData::url()
{
    QFETCH(QUrl, url);
    QFETCH(QUrl, expected);

    SubQNetworkCacheMetaData data;
    data.setUrl(url);
    QCOMPARE(data.url(), expected);
}

void tst_QNetworkCacheMetaData::stream()
{
    QNetworkCacheMetaData data;
    data.setUrl(QUrl(EXAMPLE_URL));
    QNetworkCacheMetaData::RawHeaderList headers;
    headers.append(QNetworkCacheMetaData::RawHeader("foo", "Bar"));
    data.setRawHeaders(headers);
    data.setLastModified(QDateTime::currentDateTime());
    data.setExpirationDate(QDateTime::currentDateTime());
    data.setSaveToDisk(false);

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QDataStream stream(&buffer);
    stream << data;

    buffer.seek(0);
    QNetworkCacheMetaData data2;
    stream >> data2;
    QCOMPARE(data2, data);
}

QTEST_MAIN(tst_QNetworkCacheMetaData)
#include "tst_qnetworkcachemetadata.moc"

