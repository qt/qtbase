// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtNetwork/qnetworkrequestfactory.h>
#ifndef QT_NO_SSL
#include <QtNetwork/qsslconfiguration.h>
#endif
#include <QtCore/qurlquery.h>
#include <QtCore/qurl.h>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

class tst_QNetworkRequestFactory : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void urlAndPath_data();
    void urlAndPath();
    void queryParameters();
    void sslConfiguration();
    void headers();
    void bearerToken();
    void operators();
    void timeout();
    void userInfo();
    void priority();
    void attributes();

private:
    const QUrl url1{u"http://foo.io"_s};
    const QUrl url2{u"http://bar.io"_s};
    const QByteArray bearerToken1{"bearertoken1"};
    const QByteArray bearerToken2{"bearertoken2"};
};

void tst_QNetworkRequestFactory::urlAndPath_data()
{
    QTest::addColumn<QUrl>("baseUrl");
    QTest::addColumn<QString>("requestPath");
    QTest::addColumn<QUrl>("expectedRequestUrl");

    QUrl base{"http://xyz.io"};
    QUrl result{"http://xyz.io/path/to"};
    QTest::newRow("baseUrl_nopath_noslash_1") << base << u""_s << base;
    QTest::newRow("baseUrl_nopath_noslash_2") << base << u"/path/to"_s << result;
    QTest::newRow("baseUrl_nopath_noslash_3") << base << u"path/to"_s << result;

    base.setUrl("http://xyz.io/");
    result.setUrl("http://xyz.io/path/to");
    QTest::newRow("baseUrl_nopath_withslash_1") << base << u""_s << base;
    QTest::newRow("baseUrl_nopath_withslash_2") << base << u"/path/to"_s << result;
    QTest::newRow("baseUrl_nopath_withslash_3") << base << u"path/to"_s << result;

    base.setUrl("http://xyz.io/v1");
    result.setUrl("http://xyz.io/v1/path/to");
    QTest::newRow("baseUrl_withpath_noslash_1") << base << u""_s << base;
    QTest::newRow("baseUrl_withpath_noslash_2") << base << u"/path/to"_s << result;
    QTest::newRow("baseUrl_withpath_noslash_3") << base << u"path/to"_s  << result;

    base.setUrl("http://xyz.io/v1/");
    QTest::newRow("baseUrl_withpath_withslash_1") << base << u""_s << base;
    QTest::newRow("baseUrl_withpath_withslash_2") << base << u"/path/to"_s << result;
    QTest::newRow("baseUrl_withpath_withslash_3") << base << u"path/to"_s << result;

    base.setUrl("http://xyz.io/v1/");
    // if '!' isn't encoded, it stays not encoded.
    result.setUrl("http://xyz.io/v1/path/to/Hello%20World!.xml");
    QTest::newRow("baseUrl_withpath_not_encoded_1") << base << u"/path/to/Hello%20World!.xml"_s << result;

    // if '!' is encoded, then createRequest() should NOT decode it, see QTBUG-138878
    result.setUrl("http://xyz.io/v1/path/to/Hello%20World%21.xml");
    QTest::newRow("baseUrl_withpath_encoded_2") << base << u"/path/to/Hello%20World%21.xml"_s << result;

    // Currently we keep any double '//', but not sure if there is a use case for it, or could
    // it be corrected to a single '/'
    base.setUrl("http://xyz.io/v1//");
    result.setUrl("http://xyz.io/v1//path/to");
    QTest::newRow("baseUrl_withpath_doubleslash_1") << base << u""_s << base;
    QTest::newRow("baseUrl_withpath_doubleslash_2") << base << u"/path/to"_s << result;
    QTest::newRow("baseUrl_withpath_doubleslash_3") << base << u"path/to"_s << result;
}

void tst_QNetworkRequestFactory::urlAndPath()
{
    QFETCH(QUrl, baseUrl);
    QFETCH(QString, requestPath);
    QFETCH(QUrl, expectedRequestUrl);

    // Set with constructor
    QNetworkRequestFactory factory1{baseUrl};
    QCOMPARE(factory1.baseUrl(), baseUrl);

    // Set with setter calls
    QNetworkRequestFactory factory2{};
    factory2.setBaseUrl(baseUrl);
    QCOMPARE(factory2.baseUrl(), baseUrl);

    // Request path
    QNetworkRequest request = factory1.createRequest();
    QCOMPARE(request.url(), baseUrl); // No path was provided for createRequest(), expect baseUrl
    request = factory1.createRequest(requestPath);

    QCOMPARE(request.url(), expectedRequestUrl);
    QCOMPARE(request.url().toEncoded(), expectedRequestUrl.toEncoded());
    QCOMPARE(request.url().toString(), expectedRequestUrl.toString());

    // Check the request path didn't change base url
    QCOMPARE(factory1.baseUrl(), baseUrl);
}

void tst_QNetworkRequestFactory::queryParameters()
{
    QNetworkRequestFactory factory({"http://example.com"});
    const QUrlQuery query1{{"q1k", "q1v"}};
    const QUrlQuery query2{{"q2k", "q2v"}};

    // Set query parameters in createRequest() call
    QCOMPARE(factory.createRequest(query1).url(), QUrl{"http://example.com?q1k=q1v"});
    QCOMPARE(factory.createRequest(query2).url(), QUrl{"http://example.com?q2k=q2v"});

    // Set query parameters into the factory
    factory.setQueryParameters(query1);
    QUrlQuery resultQuery = factory.queryParameters();
    for (const auto &item: query1.queryItems()) {
        QVERIFY(resultQuery.hasQueryItem(item.first));
        QCOMPARE(resultQuery.queryItemValue(item.first), item.second);
    }
    QCOMPARE(factory.createRequest().url(), QUrl{"http://example.com?q1k=q1v"});

    // Set query parameters into both createRequest() and factory
    QCOMPARE(factory.createRequest(query2).url(), QUrl{"http://example.com?q2k=q2v&q1k=q1v"});

    // Clear query parameters
    factory.clearQueryParameters();
    QVERIFY(factory.queryParameters().isEmpty());
    QCOMPARE(factory.createRequest().url(), QUrl{"http://example.com"});

    const QString pathWithQuery{"content?raw=1"};
    // Set query parameters in per-request path
    QCOMPARE(factory.createRequest(pathWithQuery).url(),
             QUrl{"http://example.com/content?raw=1"});
    // Set query parameters in per-request path and the query parameter
    QCOMPARE(factory.createRequest(pathWithQuery, query1).url(),
             QUrl{"http://example.com/content?q1k=q1v&raw=1"});
    // Set query parameter in per-request path and into the factory
    factory.setQueryParameters(query2);
    QCOMPARE(factory.createRequest(pathWithQuery).url(),
             QUrl{"http://example.com/content?raw=1&q2k=q2v"});
    // Set query parameters in per-request, as additional parameters, and into the factory
    QCOMPARE(factory.createRequest(pathWithQuery, query1).url(),
             QUrl{"http://example.com/content?q1k=q1v&raw=1&q2k=q2v"});

    // Test that other than path and query items as part of path are ignored
    factory.setQueryParameters(query1);
    QRegularExpression re("The provided path*");
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, re);
    QCOMPARE(factory.createRequest("https://example2.com").url(), QUrl{"http://example.com?q1k=q1v"});
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, re);
    QCOMPARE(factory.createRequest("https://example2.com?q3k=q3v").url(),
             QUrl{"http://example.com?q3k=q3v&q1k=q1v"});
}

void tst_QNetworkRequestFactory::sslConfiguration()
{
#ifdef QT_NO_SSL
    QSKIP("Skipping SSL tests, not supported by build");
#else
    // Two initially equal factories
    QNetworkRequestFactory factory1{url1};
    QNetworkRequestFactory factory2{url1};

    // Make two differing SSL configurations (for this test it's irrelevant how they differ)
    QSslConfiguration config1;
    config1.setProtocol(QSsl::TlsV1_2);
    QSslConfiguration config2;
    config2.setProtocol(QSsl::DtlsV1_2);

    // Set configuration and verify that the same config is returned
    factory1.setSslConfiguration(config1);
    QCOMPARE(factory1.sslConfiguration(), config1);
    factory2.setSslConfiguration(config2);
    QCOMPARE(factory2.sslConfiguration(), config2);

    // Verify requests are set with appropriate SSL configs
    QNetworkRequest request1 = factory1.createRequest();
    QCOMPARE(request1.sslConfiguration(), config1);
    QNetworkRequest request2 = factory2.createRequest();
    QCOMPARE(request2.sslConfiguration(), config2);
#endif
}

void tst_QNetworkRequestFactory::headers()
{
    const QByteArray name1{"headername1"};
    const QByteArray name2{"headername2"};
    const QByteArray value1{"headervalue1"};
    const QByteArray value2{"headervalue2"};
    const QByteArray value3{"headervalue3"};

    QNetworkRequestFactory factory{url1};
    // Initial state when no headers are set
    QVERIFY(factory.commonHeaders().isEmpty());
    QVERIFY(factory.commonHeaders().values(name1).isEmpty());
    QVERIFY(!factory.commonHeaders().contains(name1));

    // Set headers
    QHttpHeaders h1;
    h1.append(name1, value1);
    factory.setCommonHeaders(h1);
    QVERIFY(factory.commonHeaders().contains(name1));
    QCOMPARE(factory.commonHeaders().combinedValue(name1), value1);
    QCOMPARE(factory.commonHeaders().size(), 1);
    QVERIFY(factory.commonHeaders().values("nonexistent").isEmpty());
    QNetworkRequest request = factory.createRequest();
    QVERIFY(request.hasRawHeader(name1));
    QCOMPARE(request.rawHeader(name1), value1);

    // Check that empty header does not match
    QVERIFY(!factory.commonHeaders().contains(""_ba));
    QVERIFY(factory.commonHeaders().values(""_ba).isEmpty());

    // Clear headers
    factory.clearCommonHeaders();
    QVERIFY(factory.commonHeaders().isEmpty());
    request = factory.createRequest();
    QVERIFY(!request.hasRawHeader(name1));

    // Set headers with more entries
    h1.clear();
    h1.append(name1, value1);
    h1.append(name2, value2);
    factory.setCommonHeaders(h1);
    QVERIFY(factory.commonHeaders().contains(name1));
    QVERIFY(factory.commonHeaders().contains(name2));
    QCOMPARE(factory.commonHeaders().combinedValue(name1), value1);
    QCOMPARE(factory.commonHeaders().combinedValue(name2), value2);
    QCOMPARE(factory.commonHeaders().size(), 2);
    request = factory.createRequest();
    QVERIFY(request.hasRawHeader(name1));
    QVERIFY(request.hasRawHeader(name2));
    QCOMPARE(request.rawHeader(name1), value1);
    QCOMPARE(request.rawHeader(name2), value2);
    // Append more values to pre-existing header name2
    h1.clear();
    h1.append(name1, value1);
    h1.append(name1, value2);
    h1.append(name1, value3);
    factory.setCommonHeaders(h1);
    QVERIFY(factory.commonHeaders().contains(name1));
    QCOMPARE(factory.commonHeaders().combinedValue(name1), value1 + ", " + value2 + ", " + value3);
    request = factory.createRequest();
    QVERIFY(request.hasRawHeader(name1));
    QCOMPARE(request.rawHeader(name1), value1 + ", " + value2 + ", " + value3);
}

void tst_QNetworkRequestFactory::bearerToken()
{
    const auto authHeader = "Authorization"_ba;
    QNetworkRequestFactory factory{url1};
    QVERIFY(factory.bearerToken().isEmpty());

    factory.setBearerToken(bearerToken1);
    QCOMPARE(factory.bearerToken(), bearerToken1);
    QNetworkRequest request = factory.createRequest();
    QVERIFY(request.hasRawHeader(authHeader));
    QCOMPARE(request.rawHeader(authHeader), "Bearer "_ba + bearerToken1);

    // Verify that bearerToken is not in debug output
    QString debugOutput;
    QDebug debug(&debugOutput);
    debug << factory;
    QVERIFY(debugOutput.contains("bearerToken = (is set)"));
    QVERIFY(!debugOutput.contains(bearerToken1));

    factory.setBearerToken(bearerToken2);
    QCOMPARE(factory.bearerToken(), bearerToken2);
    request = factory.createRequest();
    QVERIFY(request.hasRawHeader(authHeader));
    QCOMPARE(request.rawHeader(authHeader), "Bearer "_ba + bearerToken2);

    // Set authorization header manually
    const auto value = "headervalue"_ba;
    QHttpHeaders h1;
    h1.append(authHeader, value);
    factory.setCommonHeaders(h1);
    request = factory.createRequest();
    QVERIFY(request.hasRawHeader(authHeader));
    // bearerToken has precedence over manually set header
    QCOMPARE(request.rawHeader(authHeader), "Bearer "_ba + bearerToken2);
    // clear bearer token, the manually set header is now used
    factory.clearBearerToken();
    request = factory.createRequest();
    QVERIFY(request.hasRawHeader(authHeader));
    QCOMPARE(request.rawHeader(authHeader), value);
}

void tst_QNetworkRequestFactory::operators()
{
    QNetworkRequestFactory factory1(url1);

    // Copy ctor
    QNetworkRequestFactory factory2(factory1);
    QCOMPARE(factory2.baseUrl(), factory1.baseUrl());

    // Copy assignment
    QNetworkRequestFactory factory3;
    factory3 = factory2;
    QCOMPARE(factory3.baseUrl(), factory2.baseUrl());

    // Move assignment
    QNetworkRequestFactory factory4;
    factory4 = std::move(factory3);
    QCOMPARE(factory4.baseUrl(), factory2.baseUrl());

    // Verify implicit sharing
    factory1.setBaseUrl(url2);
    QCOMPARE(factory1.baseUrl(), url2); // changed
    QCOMPARE(factory2.baseUrl(), url1); // remains

    // Move ctor
    QNetworkRequestFactory factory5{std::move(factory4)};
    QCOMPARE(factory5.baseUrl(), factory2.baseUrl()); // the moved factory4 originates from factory2
    QCOMPARE(factory5.baseUrl(), url1);
}

void tst_QNetworkRequestFactory::timeout()
{
    constexpr auto defaultTimeout = 0ms;
    constexpr auto timeout = 150ms;

    QNetworkRequestFactory factory;
    QNetworkRequest request = factory.createRequest();
    QCOMPARE(factory.transferTimeout(), defaultTimeout);
    QCOMPARE(request.transferTimeoutAsDuration(), defaultTimeout);

    factory.setTransferTimeout(timeout);
    request = factory.createRequest();
    QCOMPARE(factory.transferTimeout(), timeout);
    QCOMPARE(request.transferTimeoutAsDuration(), timeout);
}

void tst_QNetworkRequestFactory::userInfo()
{
    QNetworkRequestFactory factory;
    QVERIFY(factory.userName().isEmpty());
    QVERIFY(factory.password().isEmpty());

    const auto uname = u"a_username"_s;
    const auto password = u"a_password"_s;
    factory.setUserName(uname);
    QCOMPARE(factory.userName(), uname);
    factory.setPassword(password);
    QCOMPARE(factory.password(), password);

    // Verify that debug output does not contain password
    QString debugOutput;
    QDebug debug(&debugOutput);
    debug << factory;
    QVERIFY(debugOutput.contains("password = (is set)"));
    QVERIFY(!debugOutput.contains(password));

    factory.clearUserName();
    factory.clearPassword();
    QVERIFY(factory.userName().isEmpty());
    QVERIFY(factory.password().isEmpty());
}

void tst_QNetworkRequestFactory::priority()
{
    QNetworkRequestFactory factory(u"http://example.com"_s);
    QCOMPARE(factory.priority(), QNetworkRequest::NormalPriority);
    auto request = factory.createRequest("/index.html");
    QCOMPARE(request.priority(), QNetworkRequest::NormalPriority);

    factory.setPriority(QNetworkRequest::HighPriority);
    QCOMPARE(factory.priority(), QNetworkRequest::HighPriority);
    request = factory.createRequest("/index.html");
    QCOMPARE(request.priority(), QNetworkRequest::HighPriority);
}

void tst_QNetworkRequestFactory::attributes()
{
    const auto attribute1 = QNetworkRequest::Attribute::BackgroundRequestAttribute;
    const auto attribute2 = QNetworkRequest::User;
    QNetworkRequestFactory factory;
    QNetworkRequest request;

    // Empty factory
    QVERIFY(!factory.attribute(attribute1).isValid());
    request = factory.createRequest();
    QVERIFY(!request.attribute(attribute1).isValid());

    // (Re-)set and clear individual attribute
    factory.setAttribute(attribute1, true);
    QVERIFY(factory.attribute(attribute1).isValid());
    QCOMPARE(factory.attribute(attribute1).toBool(), true);
    request = factory.createRequest();
    QVERIFY(request.attribute(attribute1).isValid());
    QCOMPARE(request.attribute(attribute1).toBool(), true);
    // Replace previous value
    factory.setAttribute(attribute1, false);
    QVERIFY(factory.attribute(attribute1).isValid());
    QCOMPARE(factory.attribute(attribute1).toBool(), false);
    request = factory.createRequest();
    QVERIFY(request.attribute(attribute1).isValid());
    QCOMPARE(request.attribute(attribute1).toBool(), false);
    // Clear individual attribute
    factory.clearAttribute(attribute1);
    QVERIFY(!factory.attribute(attribute1).isValid());

    // Getter default value
    QCOMPARE(factory.attribute(attribute2, 111).toInt(), 111); // default value returned
    factory.setAttribute(attribute2, 222);
    QCOMPARE(factory.attribute(attribute2, 111).toInt(), 222); // actual value returned
    factory.clearAttribute(attribute2);
    QCOMPARE(factory.attribute(attribute2, 111).toInt(), 111); // default value returned

    // Clear attributes
    factory.setAttribute(attribute1, true);
    factory.setAttribute(attribute2, 333);
    QVERIFY(factory.attribute(attribute1).isValid());
    QVERIFY(factory.attribute(attribute2).isValid());
    factory.clearAttributes();
    QVERIFY(!factory.attribute(attribute1).isValid());
    QVERIFY(!factory.attribute(attribute2).isValid());
    request = factory.createRequest();
    QVERIFY(!request.attribute(attribute1).isValid());
    QVERIFY(!request.attribute(attribute2).isValid());
}

QTEST_MAIN(tst_QNetworkRequestFactory)
#include "tst_qnetworkrequestfactory.moc"
