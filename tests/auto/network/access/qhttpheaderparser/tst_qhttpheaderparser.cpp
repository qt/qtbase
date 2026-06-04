// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QObject>
#include <QtNetwork/private/qhttpheaderparser_p.h>

class tst_QHttpHeaderParser : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void constructor();
    void limitsSetters();

    void adjustableLimits_data();
    void adjustableLimits();

    void parseStatus_data();
    void parseStatus();

    // general parsing tests can be found in tst_QHttpNetworkReply
};

void tst_QHttpHeaderParser::constructor()
{
    QHttpHeaderParser parser;
    QCOMPARE(parser.getStatusCode(), 100);
    QCOMPARE(parser.getMajorVersion(), 0);
    QCOMPARE(parser.getMinorVersion(), 0);
    QCOMPARE(parser.getReasonPhrase(), QByteArray());
    QCOMPARE(parser.combinedHeaderValue("Location"), QByteArray());
    QCOMPARE(parser.maxHeaderFields(), HeaderConstants::DEFAULT_MAX_HEADER_FIELDS);
    QCOMPARE(parser.maxHeaderFieldSize(), HeaderConstants::DEFAULT_MAX_HEADER_FIELD_SIZE);
    QCOMPARE(parser.maxTotalHeaderSize(), HeaderConstants::DEFAULT_MAX_TOTAL_HEADER_SIZE);
}

void tst_QHttpHeaderParser::limitsSetters()
{
    QHttpHeaderParser parser;
    parser.setMaxHeaderFields(10);
    QCOMPARE(parser.maxHeaderFields(), 10);
    parser.setMaxHeaderFieldSize(10);
    QCOMPARE(parser.maxHeaderFieldSize(), 10);
    parser.setMaxTotalHeaderSize(10);
    QCOMPARE(parser.maxTotalHeaderSize(), 10);
}

void tst_QHttpHeaderParser::adjustableLimits_data()
{
    QTest::addColumn<qsizetype>("maxFieldCount");
    QTest::addColumn<qsizetype>("maxFieldSize");
    QTest::addColumn<qsizetype>("maxTotalSize");
    QTest::addColumn<QByteArray>("headers");
    QTest::addColumn<bool>("success");

    // We pretend -1 means to not set a new limit.

    QTest::newRow("maxFieldCount-pass") << qsizetype(10) << qsizetype(-1) << qsizetype(-1)
                                        << QByteArray("Location: hi\r\n\r\n") << true;
    QTest::newRow("maxFieldCount-fail") << qsizetype(1) << qsizetype(-1) << qsizetype(-1)
                                        << QByteArray("Location: hi\r\nCookie: a\r\n\r\n") << false;

    QTest::newRow("maxFieldSize-pass") << qsizetype(-1) << qsizetype(50) << qsizetype(-1)
                                       << QByteArray("Location: hi\r\n\r\n") << true;
    constexpr char cookieHeader[] = "Cookie: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    static_assert(sizeof(cookieHeader) - 1 == 51);
    QByteArray fullHeader = QByteArray("Location: hi\r\n") + cookieHeader;
    QTest::newRow("maxFieldSize-fail") << qsizetype(-1) << qsizetype(50) << qsizetype(-1)
                                       << (fullHeader + "\r\n\r\n") << false;

    QTest::newRow("maxTotalSize-pass") << qsizetype(-1) << qsizetype(-1) << qsizetype(50)
                                       << QByteArray("Location: hi\r\n\r\n") << true;
    QTest::newRow("maxTotalSize-fail") << qsizetype(-1) << qsizetype(-1) << qsizetype(10)
                                       << QByteArray("Location: hi\r\n\r\n") << false;
}

void tst_QHttpHeaderParser::adjustableLimits()
{
    QFETCH(qsizetype, maxFieldCount);
    QFETCH(qsizetype, maxFieldSize);
    QFETCH(qsizetype, maxTotalSize);
    QFETCH(QByteArray, headers);
    QFETCH(bool, success);

    QHttpHeaderParser parser;
    if (maxFieldCount != qsizetype(-1))
        parser.setMaxHeaderFields(maxFieldCount);
    if (maxFieldSize != qsizetype(-1))
        parser.setMaxHeaderFieldSize(maxFieldSize);
    if (maxTotalSize != qsizetype(-1))
        parser.setMaxTotalHeaderSize(maxTotalSize);

    QCOMPARE(parser.parseHeaders(headers), success);
}

void tst_QHttpHeaderParser::parseStatus_data()
{
    QTest::addColumn<QByteArray>("status");
    QTest::addColumn<bool>("success");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("reasonPhrase");

    // --- Valid cases ---
    QTest::newRow("standard-200")
        << QByteArray("HTTP/1.1 200 OK")
        << true << 200 << QString("OK");

    QTest::newRow("standard-404")
        << QByteArray("HTTP/1.1 404 Not Found")
        << true << 404 << QString("Not Found");

    QTest::newRow("http-1.0")
        << QByteArray("HTTP/1.0 200 OK")
        << true << 200 << QString("OK");

    QTest::newRow("empty-reason-phrase")
        << QByteArray("HTTP/1.1 200")
        << true << 200 << QString("");

    QTest::newRow("reason-phrase-with-tab")
        << QByteArray("HTTP/1.1 200 OK\there")
        << true << 200 << QString("OK\there");

    QTest::newRow("reason-phrase-with-obsolete-text")
        << QByteArray("HTTP/1.1 200 OK\x80")
        << true << 200 << QString::fromLatin1("OK\x80");

    QTest::newRow("reason-phrase-at-length-limit")
        << (QByteArray("HTTP/1.1 200 ") + QByteArray(1024, 'a'))
        << true << 200 << QString(1024, 'a');

    // --- Control character injection ---
    QTest::newRow("reason-phrase-with-crlf-injection")
        << QByteArray("HTTP/1.1 200 OK\r\nX-Injected: evil")
        << false << 0 << QString();

    QTest::newRow("reason-phrase-with-lf")
        << QByteArray("HTTP/1.1 200 OK\nevil")
        << false << 0 << QString();

    QTest::newRow("reason-phrase-with-nul")
        << QByteArray("HTTP/1.1 200 OK\x00" "evil", 20)
        << false << 0 << QString();

    QTest::newRow("reason-phrase-with-del")
        << QByteArray("HTTP/1.1 200 OK\x7f")
        << false << 0 << QString();

    // --- Length limit ---
    QTest::newRow("reason-phrase-exceeds-limit")
        << (QByteArray("HTTP/1.1 200 ") + QByteArray(1025, 'a'))
        << false << 0 << QString();
}

void tst_QHttpHeaderParser::parseStatus()
{
    QFETCH(QByteArray, status);
    QFETCH(bool, success);
    QFETCH(int, statusCode);
    QFETCH(QString, reasonPhrase);

    QHttpHeaderParser parser;
    QCOMPARE(parser.parseStatus(status), success);
    if (success) {
        QCOMPARE(parser.getStatusCode(), statusCode);
        QCOMPARE(parser.getReasonPhrase(), reasonPhrase);
    }
}

QTEST_MAIN(tst_QHttpHeaderParser)
#include "tst_qhttpheaderparser.moc"
