// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qhttpheaders.h>

#include <QtCore/qstring.h>
#include <QTest>

using namespace Qt::StringLiterals;

class tst_QHttpHeaders : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void append_enum();
    void append_string_data();
    void append_string();

    void contains_enum_data() { setupBasicEnumData(); }
    void contains_enum();
    void contains_string_data() { setupBasicStringData(); }
    void contains_string();

    void values_enum_data() { setupBasicEnumData(); }
    void values_enum();
    void values_string_data() { setupBasicStringData(); }
    void values_string();

    void combinedValue_enum_data() { setupBasicEnumData(); }
    void combinedValue_enum();
    void combinedValue_string_data() { setupBasicStringData(); }
    void combinedValue_string();

    void removeAll_enum_data() { setupBasicEnumData(); }
    void removeAll_enum();
    void removeAll_string_data() { setupBasicStringData(); }
    void removeAll_string();

private:
    using WKH = QHttpHeaders::WellKnownHeader;

    void setupBasicEnumData()
    {
        QTest::addColumn<WKH>("name");
        QTest::addColumn<bool>("exists");

        QTest::newRow("Nonexistent enum") << WKH::CDNLoop << false;
        QTest::newRow("Existent enum") << WKH::Accept << true;
    }

    void setupBasicStringData()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("exists");

        QTest::newRow("Nonexistent string") << "cdn-loop" << false;
        QTest::newRow("Existent wellknown string") << "accept" << true;
        QTest::newRow("Existent custom string") << "customheader" << true;
    }

    const QString customNameLowCase = u"customm-name"_s; // Not a typo, same length as knownName
    const QString customNameMixCase = u"Customm-Name"_s;
    const QString knownNameLowCase =  u"content-type"_s;
    const QString knownNameMixCase =  u"Content-Type"_s;
    static constexpr QAnyStringView value1{"value1"};
    static constexpr QAnyStringView value2{"value2"};
    static constexpr QAnyStringView value3{"value3"};
    static constexpr QAnyStringView value4{"value4"};

    QHttpHeaders m_complexHeaders;
    QHttpHeaders m_simpleHeaders;
};

void tst_QHttpHeaders::initTestCase()
{
    // A mix of known and custom headers in non-alphabetical order.
    // "accept" (wellknown header)  and "customheader" (custom header)
    // have four values and in same order: value3, value1, value2, value4
    m_complexHeaders.append(WKH::Upgrade, value1);
    m_complexHeaders.append("timeout", value1);
    m_complexHeaders.append(WKH::KeepAlive, value1);
    m_complexHeaders.append("nel", value1);
    m_complexHeaders.append(WKH::Date, value1);
    m_complexHeaders.append("accept", value3);
    m_complexHeaders.append("customheader", value3);
    m_complexHeaders.append(WKH::SoapAction, value1);
    m_complexHeaders.append("foobar", value1);
    m_complexHeaders.append(WKH::Accept, value1);
    m_complexHeaders.append("customheader", value1);
    m_complexHeaders.append(WKH::Cookie, value1);
    m_complexHeaders.append("cookie", value1);
    m_complexHeaders.append(WKH::ProxyStatus, value1);
    m_complexHeaders.append("cookie", value1);
    m_complexHeaders.append(WKH::Accept, value2);
    m_complexHeaders.append("customheader", value2);
    m_complexHeaders.append(WKH::Authorization, value1);
    m_complexHeaders.append("content-type", value1);
    m_complexHeaders.append(WKH::Destination, value1);
    m_complexHeaders.append("rover", value1);
    m_complexHeaders.append("accept", value4);
    m_complexHeaders.append("customheader", value4);
    m_complexHeaders.append(WKH::Range, value1);
    m_complexHeaders.append("destination", value1);
    m_complexHeaders.append(WKH::Location, value1);
    m_complexHeaders.append("font", value1);

    // Simple headers with one value-less entry; having at least one entry ensures
    // we don't include initial memory allocation into a test, and having
    // as few as possible reduces cost of copying (header re-set)
    m_simpleHeaders.append(WKH::Accept, "");
}

void tst_QHttpHeaders::append_enum()
{
    QHttpHeaders headers;
    QBENCHMARK {
        headers = m_simpleHeaders; // (Re-)set as we'll modify the headers
        headers.append(WKH::Link, value1);
    }
    QCOMPARE(headers.size(), m_simpleHeaders.size() + 1);
    QCOMPARE(headers.nameAt(headers.size() - 1), "link");
}

void tst_QHttpHeaders::append_string_data()
{
    QTest::addColumn<QString>("name");
    QTest::newRow("Custom lowcase string") << customNameLowCase;
    QTest::newRow("WellKnown lowcase string") << knownNameLowCase;
    QTest::newRow("Custom mixcase string") << customNameMixCase;
    QTest::newRow("WellKnown mixcase string") << knownNameMixCase;
}

void tst_QHttpHeaders::append_string()
{
    QFETCH(QString, name);

    QHttpHeaders headers;
    QBENCHMARK {
        headers = m_simpleHeaders; // (Re-)set as we'll modify the headers
        headers.append(name, value1);
    }
    QCOMPARE(headers.size(), m_simpleHeaders.size() + 1);
    QCOMPARE(headers.nameAt(headers.size() - 1), name.toLower());
}

void tst_QHttpHeaders::contains_enum()
{
    QFETCH(WKH, name);
    QFETCH(bool, exists);

    bool result;
    QBENCHMARK {
        result = m_complexHeaders.contains(name);
    }
    QCOMPARE(result, exists);
}

void tst_QHttpHeaders::contains_string()
{
    QFETCH(QString, name);
    QFETCH(bool, exists);

    bool result;
    QBENCHMARK {
        result = m_complexHeaders.contains(name);
    }
    QCOMPARE(result, exists);
}

void tst_QHttpHeaders::values_enum()
{
    QFETCH(WKH, name);
    QFETCH(bool, exists);

    QList<QByteArray> result;
    const QList<QByteArray> expectedResult{"value3", "value1", "value2", "value4"};
    QBENCHMARK {
        result = m_complexHeaders.values(name);
    }
    if (exists)
        QCOMPARE(result, expectedResult);
    else
        QVERIFY(result.isEmpty());
}

void tst_QHttpHeaders::values_string()
{
    QFETCH(QString, name);
    QFETCH(bool, exists);

    QList<QByteArray> result;
    const QList<QByteArray> expectedResult{"value3", "value1", "value2", "value4"};
    QBENCHMARK {
        result = m_complexHeaders.values(name);
    }
    if (exists)
        QCOMPARE(result, expectedResult);
    else
        QVERIFY(result.isEmpty());
}

void tst_QHttpHeaders::combinedValue_enum()
{
    QFETCH(WKH, name);
    QFETCH(bool, exists);

    QByteArray result;
    constexpr QByteArrayView expectedResult = "value3, value1, value2, value4";
    QBENCHMARK {
        result = m_complexHeaders.combinedValue(name);
    }
    if (exists)
        QCOMPARE(result, expectedResult);
    else
        QVERIFY(result.isEmpty());
}

void tst_QHttpHeaders::combinedValue_string()
{
    QFETCH(QString, name);
    QFETCH(bool, exists);

    QByteArray result;
    constexpr QByteArrayView expectedResult = "value3, value1, value2, value4";
    QBENCHMARK {
        result = m_complexHeaders.combinedValue(name);
    }
    if (exists)
        QCOMPARE(result, expectedResult);
    else
        QVERIFY(result.isEmpty());
}

void tst_QHttpHeaders::removeAll_enum()
{
    QFETCH(WKH, name);
    QFETCH(bool, exists);

    QHttpHeaders headers = m_complexHeaders;
    QCOMPARE(headers.contains(name), exists);
    QBENCHMARK {
        headers = m_complexHeaders; // Restore as we'll modify the headers
        headers.removeAll(name);
    }
    QVERIFY(!headers.contains(name));
}

void tst_QHttpHeaders::removeAll_string()
{
    QFETCH(QString, name);
    QFETCH(bool, exists);

    QHttpHeaders headers = m_complexHeaders;
    QCOMPARE(headers.contains(name), exists);
    QBENCHMARK {
        headers = m_complexHeaders; // Restore as we'll modify the headers
        headers.removeAll(name);
    }
    QVERIFY(!headers.contains(name));
}

QTEST_MAIN(tst_QHttpHeaders)

#include "tst_bench_qhttpheaders.moc"
