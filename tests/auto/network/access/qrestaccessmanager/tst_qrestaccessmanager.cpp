// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "httptestserver_p.h"

#if QT_CONFIG(http)
#include <QtNetwork/qhttpmultipart.h>
#endif
#include <QtNetwork/qrestaccessmanager.h>
#include <QtNetwork/qauthenticator.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequestfactory.h>
#include <QtNetwork/qrestreply.h>

#include <QTest>
#include <QtTest/qsignalspy.h>

#include <QtCore/private/qglobal_p.h> // for access to Qt's feature system
#include <QtCore/qbuffer.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qstringconverter.h>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

using Header = QHttpHeaders::WellKnownHeader;

class tst_QRestAccessManager : public QObject
{
    Q_OBJECT

private slots:
    void initialization();
    void destruction();
    void callbacks();
#if QT_CONFIG(http)
    void requests();
#endif
    void reply();
    void errors();
    void body();
    void json();
    void text();
    void textStreaming();

private:
    void memberHandler(QRestReply &reply);

    friend class Transient;
    QList<QNetworkReply*> m_expectedReplies;
    QList<QNetworkReply*> m_actualReplies;
};

void tst_QRestAccessManager::initialization()
{
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QRestAccessManager: QNetworkAccesManager is nullptr");
    QRestAccessManager manager1(nullptr);
    QVERIFY(!manager1.networkAccessManager());

    QNetworkAccessManager qnam;
    QRestAccessManager manager2(&qnam);
    QVERIFY(manager2.networkAccessManager());
}

void tst_QRestAccessManager::reply()
{
    QNetworkAccessManager qnam;

    QNetworkReply *nr = qnam.get(QNetworkRequest(QUrl{"someurl"}));
    QRestReply rr1(nr);
    QCOMPARE(rr1.networkReply(), nr);

    // Move-construct
    QRestReply rr2(std::move(rr1));
    QCOMPARE(rr2.networkReply(), nr);

    // Move-assign
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QRestReply: QNetworkReply is nullptr");
    QRestReply rr3(nullptr);
    rr3 = std::move(rr2);
    QCOMPARE(rr3.networkReply(), nr);
}

#define VERIFY_REPLY_OK(METHOD) \
{ \
    QTRY_VERIFY(networkReply);                  \
    QRestReply restReply(networkReply);         \
    QCOMPARE(serverSideRequest.method, METHOD); \
    QVERIFY(restReply.isSuccess());             \
    QVERIFY(!restReply.hasError());             \
    networkReply->deleteLater();                \
    networkReply = nullptr;                     \
}

#if QT_CONFIG(http)
void tst_QRestAccessManager::requests()
{
    // A basic test for each HTTP method against the local testserver.
    QNetworkAccessManager qnam;
    QRestAccessManager manager(&qnam);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    request.setRawHeader("Content-Type"_ba, "text/plain"); // To silence missing content-type warn
    QNetworkReply *networkReply = nullptr;
    std::unique_ptr<QHttpMultiPart> multiPart;
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
    part.setBody("multipart_text");
    QByteArray ioDeviceData{"io_device_data"_ba};
    QBuffer bufferIoDevice(&ioDeviceData);

    HttpData serverSideRequest;  // The request data the server received
    HttpData serverSideResponse; // The response data the server responds with
    serverSideResponse.status = 200;
    server.setHandler([&](const HttpData &request, HttpData &response, ResponseControl&) {
        serverSideRequest = request;
        response = serverSideResponse;

    });
    auto callback = [&](QRestReply &reply) { networkReply = reply.networkReply(); };
    const QByteArray byteArrayData{"some_data"_ba};
    const QJsonObject jsonObjectData{{"key1", "value1"}, {"key2", "value2"}};
    const QJsonArray jsonArrayData{{"arrvalue1", "arrvalue2", QJsonObject{{"key1", "value1"}}}};
    const QVariantMap variantMapData{{"key1", "value1"}, {"key2", "value2"}};
    const QByteArray methodDELETE{"DELETE"_ba};
    const QByteArray methodHEAD{"HEAD"_ba};
    const QByteArray methodPOST{"POST"_ba};
    const QByteArray methodGET{"GET"_ba};
    const QByteArray methodPUT{"PUT"_ba};
    const QByteArray methodPATCH{"PATCH"_ba};
    const QByteArray methodCUSTOM{"FOOBAR"_ba};

    // DELETE
    manager.deleteResource(request, this, callback);
    VERIFY_REPLY_OK(methodDELETE);
    QCOMPARE(serverSideRequest.body, ""_ba);

    // HEAD
    manager.head(request, this, callback);
    VERIFY_REPLY_OK(methodHEAD);
    QCOMPARE(serverSideRequest.body, ""_ba);

    // GET
    manager.get(request, this, callback);
    VERIFY_REPLY_OK(methodGET);
    QCOMPARE(serverSideRequest.body, ""_ba);

    manager.get(request, byteArrayData, this, callback);
    VERIFY_REPLY_OK(methodGET);
    QCOMPARE(serverSideRequest.body, byteArrayData);

    manager.get(request, QJsonDocument{jsonObjectData}, this, callback);
    VERIFY_REPLY_OK(methodGET);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    manager.get(request, &bufferIoDevice, this, callback);
    VERIFY_REPLY_OK(methodGET);
    QCOMPARE(serverSideRequest.body, ioDeviceData);

    // CUSTOM
    manager.sendCustomRequest(request, methodCUSTOM, byteArrayData, this, callback);
    VERIFY_REPLY_OK(methodCUSTOM);
    QCOMPARE(serverSideRequest.body, byteArrayData);

    manager.sendCustomRequest(request, methodCUSTOM, &bufferIoDevice, this, callback);
    VERIFY_REPLY_OK(methodCUSTOM);
    QCOMPARE(serverSideRequest.body, ioDeviceData);

    multiPart.reset(new QHttpMultiPart(QHttpMultiPart::FormDataType));
    multiPart->append(part);
    manager.sendCustomRequest(request, methodCUSTOM, multiPart.get(), this, callback);
    VERIFY_REPLY_OK(methodCUSTOM);
    QVERIFY(serverSideRequest.body.contains("--boundary"_ba));
    QVERIFY(serverSideRequest.body.contains("multipart_text"_ba));

    // POST
    manager.post(request, byteArrayData, this, callback);
    VERIFY_REPLY_OK(methodPOST);
    QCOMPARE(serverSideRequest.body, byteArrayData);

    manager.post(request, QJsonDocument{jsonObjectData}, this, callback);
    VERIFY_REPLY_OK(methodPOST);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    manager.post(request, QJsonDocument{jsonArrayData}, this, callback);
    VERIFY_REPLY_OK(methodPOST);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).array(), jsonArrayData);

    manager.post(request, variantMapData, this, callback);
    VERIFY_REPLY_OK(methodPOST);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    multiPart = std::make_unique<QHttpMultiPart>(QHttpMultiPart::FormDataType);
    multiPart->append(part);
    manager.post(request, multiPart.get(), this, callback);
    VERIFY_REPLY_OK(methodPOST);
    QVERIFY(serverSideRequest.body.contains("--boundary"_ba));
    QVERIFY(serverSideRequest.body.contains("multipart_text"_ba));

    manager.post(request, &bufferIoDevice, this, callback);
    VERIFY_REPLY_OK(methodPOST);
    QCOMPARE(serverSideRequest.body, ioDeviceData);

    // PUT
    manager.put(request, byteArrayData, this, callback);
    VERIFY_REPLY_OK(methodPUT);
    QCOMPARE(serverSideRequest.body, byteArrayData);

    manager.put(request, QJsonDocument{jsonObjectData}, this, callback);
    VERIFY_REPLY_OK(methodPUT);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    manager.put(request, QJsonDocument{jsonArrayData}, this, callback);
    VERIFY_REPLY_OK(methodPUT);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).array(), jsonArrayData);

    manager.put(request, variantMapData, this, callback);
    VERIFY_REPLY_OK(methodPUT);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    multiPart = std::make_unique<QHttpMultiPart>(QHttpMultiPart::FormDataType);
    multiPart->append(part);
    manager.put(request, multiPart.get(), this, callback);
    VERIFY_REPLY_OK(methodPUT);
    QVERIFY(serverSideRequest.body.contains("--boundary"_ba));
    QVERIFY(serverSideRequest.body.contains("multipart_text"_ba));

    manager.put(request, &bufferIoDevice, this, callback);
    VERIFY_REPLY_OK(methodPUT);
    QCOMPARE(serverSideRequest.body, ioDeviceData);

    // PATCH
    manager.patch(request, byteArrayData, this, callback);
    VERIFY_REPLY_OK(methodPATCH);
    QCOMPARE(serverSideRequest.body, byteArrayData);

    manager.patch(request, QJsonDocument{jsonObjectData}, this, callback);
    VERIFY_REPLY_OK(methodPATCH);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    manager.patch(request, QJsonDocument{jsonArrayData}, this, callback);
    VERIFY_REPLY_OK(methodPATCH);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).array(), jsonArrayData);

    manager.patch(request, variantMapData, this, callback);
    VERIFY_REPLY_OK(methodPATCH);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    manager.patch(request, &bufferIoDevice, this, callback);
    VERIFY_REPLY_OK(methodPATCH);
    QCOMPARE(serverSideRequest.body, ioDeviceData);

    //These must NOT compile
    //manager.get(request, [](){}); // callback without context object
    //manager.get(request, ""_ba, [](){}); // callback without context object
    //manager.get(request, QString()); // wrong datatype
    //manager.get(request, 123); // wrong datatype
    //manager.post(request, QString()); // wrong datatype
    //manager.put(request, 123); // wrong datatype
    //manager.post(request); // data is required
    //manager.put(request, QString()); // wrong datatype
    //manager.put(request); // data is required
    //manager.patch(request, 123); // wrong datatype
    //manager.patch(request, QString()); // wrong datatype
    //manager.patch(request); // data is required
    //manager.deleteResource(request, "f"_ba); // data not allowed
    //manager.head(request, "f"_ba); // data not allowed
    //manager.post(request, ""_ba, this, [](int param){}); // Wrong callback signature
    //manager.get(request, this, [](int param){}); // Wrong callback signature
    //manager.sendCustomRequest(request, this, [](){}); // No verb && no data
    //manager.sendCustomRequest(request, "FOOBAR", this, [](){}); // No verb || no data
}
#endif

void tst_QRestAccessManager::memberHandler(QRestReply &reply)
{
    m_actualReplies.append(reply.networkReply());
}

// Class that is destroyed during an active request.
// Used to test that the callbacks won't be called in these cases
class Transient : public QObject
{
    Q_OBJECT
public:
    explicit Transient(tst_QRestAccessManager *test) :  QObject(test), m_test(test) {}

    void memberHandler(QRestReply &reply)
    {
        m_test->m_actualReplies.append(reply.networkReply());
    }

private:
    tst_QRestAccessManager *m_test = nullptr;
};

template <typename Functor, std::enable_if_t<
          QtPrivate::AreFunctionsCompatible<void(*)(QRestReply&), Functor>::value, bool> = true>
inline constexpr bool isCompatibleCallback(Functor &&) { return true; }

template <typename Functor, std::enable_if_t<
          !QtPrivate::AreFunctionsCompatible<void(*)(QRestReply&), Functor>::value, bool> = true,
          typename = void>
inline constexpr bool isCompatibleCallback(Functor &&) { return false; }

void tst_QRestAccessManager::callbacks()
{
    QNetworkAccessManager qnam;
    QRestAccessManager manager(&qnam);

    QNetworkRequest request{u"i_dont_exist"_s}; // Will result in ProtocolUnknown error

    auto lambdaHandler = [this](QRestReply &reply) { m_actualReplies.append(reply.networkReply()); };
    Transient *transient = nullptr;
    QByteArray data{"some_data"};

    // Compile-time tests for callback signatures
    static_assert(isCompatibleCallback([](QRestReply&){})); // Correct signature
    static_assert(isCompatibleCallback(lambdaHandler));
    static_assert(isCompatibleCallback(&Transient::memberHandler));
    static_assert(isCompatibleCallback([](){})); // Less parameters are allowed

    static_assert(!isCompatibleCallback([](QString){})); // Wrong parameter type
    static_assert(!isCompatibleCallback([](QRestReply*){})); // Wrong parameter type
    static_assert(!isCompatibleCallback([](const QString &){})); // Wrong parameter type
    static_assert(!isCompatibleCallback([](QRestReply&, QString){})); // Too many parameters

    // -- Test without data
    // Without callback using signals and slot
    QNetworkReply* reply = manager.get(request);
    m_expectedReplies.append(reply);
    QObject::connect(reply, &QNetworkReply::finished, this,
                     [this, reply](){m_actualReplies.append(reply);});

    // With lambda callback, without context object
    m_expectedReplies.append(manager.get(request, nullptr, lambdaHandler));
    m_expectedReplies.append(manager.get(request, nullptr,
                                        [this](QRestReply &reply){m_actualReplies.append(reply.networkReply());}));
    // With lambda callback and context object
    m_expectedReplies.append(manager.get(request, this, lambdaHandler));
    m_expectedReplies.append(manager.get(request, this,
                                        [this](QRestReply &reply){m_actualReplies.append(reply.networkReply());}));
    // With member callback and context object
    m_expectedReplies.append(manager.get(request, this, &tst_QRestAccessManager::memberHandler));
    // With context object that is destroyed, there should be no callback or eg. crash.
    transient = new Transient(this);
    manager.get(request, transient, &Transient::memberHandler); // Reply not added to expecteds
    delete transient;

    // Let requests finish
    QTRY_COMPARE(m_actualReplies.size(), m_expectedReplies.size());
    for (auto reply: m_actualReplies) {
        QRestReply restReply(reply);
        QVERIFY(!restReply.isSuccess());
        QVERIFY(restReply.hasError());
        QCOMPARE(restReply.error(), QNetworkReply::ProtocolUnknownError);
        QCOMPARE(restReply.networkReply()->isFinished(), true);
        restReply.networkReply()->deleteLater();
    }
    m_actualReplies.clear();
    m_expectedReplies.clear();

    // -- Test with data
    // With lambda callback, without context object
    m_expectedReplies.append(manager.post(request, data, nullptr, lambdaHandler));
    m_expectedReplies.append(manager.post(request, data, nullptr,
                                        [this](QRestReply &reply){m_actualReplies.append(reply.networkReply());}));
    // With lambda callback and context object
    m_expectedReplies.append(manager.post(request, data, this, lambdaHandler));
    m_expectedReplies.append(manager.post(request, data, this,
                                        [this](QRestReply &reply){m_actualReplies.append(reply.networkReply());}));
    // With member callback and context object
    m_expectedReplies.append(manager.post(request, data,
                                         this, &tst_QRestAccessManager::memberHandler));
    // With context object that is destroyed, there should be no callback or eg. crash
    transient = new Transient(this);
    manager.post(request, data, transient, &Transient::memberHandler); // Note: reply not expected
    delete transient;

    // Let requests finish
    QTRY_COMPARE(m_actualReplies.size(), m_expectedReplies.size());
    for (auto reply: m_actualReplies) {
        QRestReply restReply(reply);
        QVERIFY(!restReply.isSuccess());
        QVERIFY(restReply.hasError());
        QCOMPARE(restReply.error(), QNetworkReply::ProtocolUnknownError);
        QCOMPARE(restReply.networkReply()->isFinished(), true);
        reply->deleteLater();
    }
    m_actualReplies.clear();
    m_expectedReplies.clear();

    // -- Test GET with data separately, as GET provides methods that are usable with and
    // without data, and fairly easy to get the qrestaccessmanager.h template SFINAE subtly wrong.
    // With lambda callback, without context object
    m_expectedReplies.append(manager.get(request, data, nullptr, lambdaHandler));
    m_expectedReplies.append(manager.get(request, data, nullptr,
                                        [this](QRestReply &reply){m_actualReplies.append(reply.networkReply());}));
    // With lambda callback and context object
    m_expectedReplies.append(manager.get(request, data, this, lambdaHandler));
    m_expectedReplies.append(manager.get(request, data, this,
                                        [this](QRestReply &reply){m_actualReplies.append(reply.networkReply());}));
    // With member callback and context object
    m_expectedReplies.append(manager.get(request, data,
                                          this, &tst_QRestAccessManager::memberHandler));
    // With context object that is destroyed, there should be no callback or eg. crash
    transient = new Transient(this);
    manager.get(request, data, transient, &Transient::memberHandler); // Reply not added
    delete transient;

    // Let requests finish
    QTRY_COMPARE(m_actualReplies.size(), m_expectedReplies.size());
    for (auto reply: m_actualReplies) {
        QRestReply restReply(reply);
        QVERIFY(!restReply.isSuccess());
        QVERIFY(restReply.hasError());
        QCOMPARE(restReply.error(), QNetworkReply::ProtocolUnknownError);
        QCOMPARE(restReply.networkReply()->isFinished(), true);
        restReply.networkReply()->deleteLater();
    }
    m_actualReplies.clear();
    m_expectedReplies.clear();
}

void tst_QRestAccessManager::destruction()
{
    std::unique_ptr<QNetworkAccessManager> qnam = std::make_unique<QNetworkAccessManager>();
    std::unique_ptr<QRestAccessManager> manager = std::make_unique<QRestAccessManager>(qnam.get());
    QNetworkRequest request{u"i_dont_exist"_s}; // Will result in ProtocolUnknown error
    m_expectedReplies.clear();
    m_actualReplies.clear();
    auto handler = [this](QRestReply &reply) { m_actualReplies.append(reply.networkReply()); };

    // Delete reply immediately, make sure nothing bad happens and that there is no callback
    QNetworkReply *networkReply = manager->get(request, this, handler);
    delete networkReply;
    QTest::qWait(20); // allow some time for the callback to arrive (it shouldn't)
    QCOMPARE(m_actualReplies.size(), m_expectedReplies.size()); // Both should be 0

    // Delete access manager immediately after request, make sure nothing bad happens
    manager->get(request, this, handler);
    manager->post(request, "data"_ba, this, handler);
    QTest::ignoreMessage(QtWarningMsg, "Access manager destroyed while 2 requests were still"
                                       " in progress");
    manager.reset();
    QTest::qWait(20);
    QCOMPARE(m_actualReplies.size(), m_expectedReplies.size()); // Both should be 0

    // Destroy the underlying QNAM while requests in progress
    manager = std::make_unique<QRestAccessManager>(qnam.get());
    manager->get(request, this, handler);
    manager->post(request, "data"_ba, this, handler);
    qnam.reset();
    QTest::qWait(20);
    QCOMPARE(m_actualReplies.size(), m_expectedReplies.size()); // Both should be 0
}

#define VERIFY_HTTP_ERROR_STATUS(STATUS) \
{ \
    serverSideResponse.status = STATUS; \
    QRestReply restReply(manager.get(request)); \
    QTRY_VERIFY(restReply.networkReply()->isFinished()); \
    QVERIFY(!restReply.hasError()); \
    QCOMPARE(restReply.httpStatus(), serverSideResponse.status); \
    QCOMPARE(restReply.error(), QNetworkReply::NetworkError::NoError); \
    QVERIFY(!restReply.isSuccess()); \
    restReply.networkReply()->deleteLater(); \
} \

void tst_QRestAccessManager::errors()
{
    // Tests the distinction between HTTP and other (network/protocol) errors
    QNetworkAccessManager qnam;
    QRestAccessManager manager(&qnam);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());

    HttpData serverSideResponse; // The response data the server responds with
    server.setHandler([&](const HttpData &, HttpData &response, ResponseControl &) {
        response  = serverSideResponse;
    });

    // Test few HTTP statuses in different categories
    VERIFY_HTTP_ERROR_STATUS(301) // QNetworkReply::ProtocolUnknownError
    VERIFY_HTTP_ERROR_STATUS(302) // QNetworkReply::ProtocolUnknownError
    VERIFY_HTTP_ERROR_STATUS(400) // QNetworkReply::ProtocolInvalidOperationError
    VERIFY_HTTP_ERROR_STATUS(401) // QNetworkReply::AuthenticationRequiredEror
    VERIFY_HTTP_ERROR_STATUS(402) // QNetworkReply::UnknownContentError
    VERIFY_HTTP_ERROR_STATUS(403) // QNetworkReply::ContentAccessDenied
    VERIFY_HTTP_ERROR_STATUS(404) // QNetworkReply::ContentNotFoundError
    VERIFY_HTTP_ERROR_STATUS(405) // QNetworkReply::ContentOperationNotPermittedError
    VERIFY_HTTP_ERROR_STATUS(406) // QNetworkReply::UnknownContentError
    VERIFY_HTTP_ERROR_STATUS(407) // QNetworkReply::ProxyAuthenticationRequiredError
    VERIFY_HTTP_ERROR_STATUS(408) // QNetworkReply::UnknownContentError
    VERIFY_HTTP_ERROR_STATUS(409) // QNetworkReply::ContentConflictError
    VERIFY_HTTP_ERROR_STATUS(410) // QNetworkReply::ContentGoneError
    VERIFY_HTTP_ERROR_STATUS(500) // QNetworkReply::InternalServerError
    VERIFY_HTTP_ERROR_STATUS(501) // QNetworkReply::OperationNotImplementedError
    VERIFY_HTTP_ERROR_STATUS(502) // QNetworkReply::UnknownServerError
    VERIFY_HTTP_ERROR_STATUS(503) // QNetworkReply::ServiceUnavailableError
    VERIFY_HTTP_ERROR_STATUS(504) // QNetworkReply::UnknownServerError
    VERIFY_HTTP_ERROR_STATUS(505) // QNetworkReply::UnknownServerError

    {
        // Test that actual network/protocol errors come through
        QRestReply restReply(manager.get({})); // Empty url
        QTRY_VERIFY(restReply.networkReply()->isFinished());
        QVERIFY(restReply.hasError());
        QVERIFY(!restReply.isSuccess());
        QCOMPARE(restReply.error(), QNetworkReply::ProtocolUnknownError);
        restReply.networkReply()->deleteLater();
    }

    {
        QRestReply restReply(manager.get(QNetworkRequest{{"http://non-existent.foo.bar.test"}}));
        QTRY_VERIFY(restReply.networkReply()->isFinished());
        QVERIFY(restReply.hasError());
        QVERIFY(!restReply.isSuccess());
        QCOMPARE(restReply.error(), QNetworkReply::HostNotFoundError);
        restReply.networkReply()->deleteLater();
    }

    {
        QRestReply restReply(manager.get(request));
        restReply.networkReply()->abort();
        QTRY_VERIFY(restReply.networkReply()->isFinished());
        QVERIFY(restReply.hasError());
        QVERIFY(!restReply.isSuccess());
        QCOMPARE(restReply.error(), QNetworkReply::OperationCanceledError);
        restReply.networkReply()->deleteLater();
    }
}

void tst_QRestAccessManager::body()
{
    // Test using QRestReply::body() data accessor
    QNetworkAccessManager qnam;
    QRestAccessManager manager(&qnam);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    QNetworkReply *networkReply = nullptr;

    HttpData serverSideRequest;  // The request data the server received
    HttpData serverSideResponse; // The response data the server responds with
    server.setHandler([&](const HttpData &request, HttpData &response, ResponseControl&) {
        serverSideRequest = request;
        response = serverSideResponse;
    });

    {
        serverSideResponse.status = 200;
        serverSideResponse.body = "some_data"_ba;
        manager.get(request, this, [&](QRestReply &reply) { networkReply = reply.networkReply(); });
        QTRY_VERIFY(networkReply);
        QRestReply restReply(networkReply);
        QCOMPARE(restReply.readBody(), serverSideResponse.body);
        QCOMPARE(restReply.httpStatus(), serverSideResponse.status);
        QVERIFY(!restReply.hasError());
        QVERIFY(restReply.isSuccess());
        networkReply->deleteLater();
        networkReply = nullptr;
    }

    {
        serverSideResponse.status = 200;
        serverSideResponse.body = ""_ba; // Empty
        manager.get(request, this, [&](QRestReply &reply) { networkReply = reply.networkReply(); });
        QTRY_VERIFY(networkReply);
        QRestReply restReply(networkReply);
        QCOMPARE(restReply.readBody(), serverSideResponse.body);
        networkReply->deleteLater();
        networkReply = nullptr;
    }

    {
        serverSideResponse.status = 500;
        serverSideResponse.body = "some_other_data"_ba;
        manager.get(request, this, [&](QRestReply &reply) { networkReply = reply.networkReply(); });
        QTRY_VERIFY(networkReply);
        QRestReply restReply(networkReply);
        QCOMPARE(restReply.readBody(), serverSideResponse.body);
        QCOMPARE(restReply.httpStatus(), serverSideResponse.status);
        QVERIFY(!restReply.hasError());
        QVERIFY(!restReply.isSuccess());
        networkReply->deleteLater();
        networkReply = nullptr;
    }
}

void tst_QRestAccessManager::json()
{
    // Tests using QRestReply::readJson()
    QNetworkAccessManager qnam;
    QRestAccessManager manager(&qnam);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    QNetworkReply *networkReply = nullptr;
    QJsonDocument responseJsonDocument;
    std::optional<QJsonDocument> json;
    QJsonParseError parseError;

    HttpData serverSideRequest;  // The request data the server received
    HttpData serverSideResponse; // The response data the server responds with
    serverSideResponse.status = 200;
    server.setHandler([&](const HttpData &request, HttpData &response, ResponseControl&) {
        serverSideRequest = request;
        response = serverSideResponse;
    });

    {
        // Test receiving valid json object
        serverSideResponse.body = "{\"key1\":\"value1\",""\"key2\":\"value2\"}\n"_ba;
        networkReply = manager.get(request);
        // Read unfinished reply
        QVERIFY(!networkReply->isFinished());
        QTest::ignoreMessage(QtWarningMsg, "readJson() called on an unfinished reply, ignoring");
        parseError.error = QJsonParseError::ParseError::DocumentTooLarge; // Reset to impossible value
        QRestReply restReply(networkReply);
        QVERIFY(!restReply.readJson(&parseError));
        QCOMPARE(parseError.error, QJsonParseError::ParseError::NoError);
        // Read finished reply
        QTRY_VERIFY(networkReply->isFinished());
        parseError.error = QJsonParseError::ParseError::DocumentTooLarge;
        json = restReply.readJson(&parseError);
        QVERIFY(json);
        QCOMPARE(parseError.error, QJsonParseError::ParseError::NoError);
        responseJsonDocument = *json;
        QVERIFY(responseJsonDocument.isObject());
        QCOMPARE(responseJsonDocument["key1"], "value1");
        QCOMPARE(responseJsonDocument["key2"], "value2");
        networkReply->deleteLater();
        networkReply = nullptr;
    }

    {
        // Test receiving an invalid json object
        serverSideResponse.body = "foobar"_ba;
        manager.get(request, this, [&](QRestReply &reply) { networkReply = reply.networkReply(); });
        QTRY_VERIFY(networkReply);
        QRestReply restReply(networkReply);
        parseError.error = QJsonParseError::ParseError::DocumentTooLarge;
        const auto json = restReply.readJson(&parseError);
        networkReply->deleteLater();
        networkReply = nullptr;
        QCOMPARE_EQ(json, std::nullopt);
        QCOMPARE_NE(parseError.error, QJsonParseError::ParseError::NoError);
        QCOMPARE_NE(parseError.error, QJsonParseError::ParseError::DocumentTooLarge);
        QCOMPARE_GT(parseError.offset, 0);
    }

    {
        // Test receiving valid json array
        serverSideResponse.body = "[\"foo\", \"bar\"]\n"_ba;
        manager.get(request, this, [&](QRestReply &reply) { networkReply = reply.networkReply(); });
        QTRY_VERIFY(networkReply);
        QRestReply restReply(networkReply);
        parseError.error = QJsonParseError::ParseError::DocumentTooLarge;
        json = restReply.readJson(&parseError);
        networkReply->deleteLater();
        networkReply = nullptr;
        QCOMPARE(parseError.error, QJsonParseError::ParseError::NoError);
        QVERIFY(json);
        responseJsonDocument = *json;
        QVERIFY(responseJsonDocument.isArray());
        QCOMPARE(responseJsonDocument.array().size(), 2);
        QCOMPARE(responseJsonDocument[0].toString(), "foo"_L1);
        QCOMPARE(responseJsonDocument[1].toString(), "bar"_L1);
    }
}

#define VERIFY_TEXT_REPLY_OK \
{ \
    manager.get(request, this, [&](QRestReply &reply) { networkReply = reply.networkReply(); }); \
    QTRY_VERIFY(networkReply); \
    QRestReply restReply(networkReply); \
    responseString = restReply.readText(); \
    networkReply->deleteLater(); \
    networkReply = nullptr; \
    QCOMPARE(responseString, sourceString); \
}

#define VERIFY_TEXT_REPLY_ERROR(WARNING_MESSAGE) \
{ \
    manager.get(request, this, [&](QRestReply &reply) { networkReply = reply.networkReply(); }); \
    QTRY_VERIFY(networkReply); \
    QTest::ignoreMessage(QtWarningMsg, WARNING_MESSAGE); \
    QRestReply restReply(networkReply); \
    responseString = restReply.readText(); \
    networkReply->deleteLater(); \
    networkReply = nullptr; \
    QVERIFY(responseString.isEmpty()); \
}

void tst_QRestAccessManager::text()
{
    // Test using QRestReply::text() data accessor with various text encodings
    QNetworkAccessManager qnam;
    QRestAccessManager manager(&qnam);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    QNetworkReply *networkReply = nullptr;
    QJsonObject responseJsonObject;

    QStringEncoder encUTF8("UTF-8");
    QStringEncoder encUTF16("UTF-16");
    QStringEncoder encUTF32("UTF-32");
    QString responseString;

    HttpData serverSideRequest;  // The request data the server received
    HttpData serverSideResponse; // The response data the server responds with
    serverSideResponse.status = 200;
    server.setHandler([&](const HttpData &request, HttpData &response, ResponseControl&) {
        serverSideRequest = request;
        response = serverSideResponse;
    });

    const QString sourceString("this is a string"_L1);

    // Charset parameter of Content-Type header may specify non-UTF-8 character encoding.
    //
    // QString is UTF-16, and in the tests below we encode the response data to various
    // charset encodings (into byte arrays). When we get the response data, the text()
    // should consider the indicated charset and convert it to an UTF-16 QString => the returned
    // QString from text() should match with the original (UTF-16) QString.

    // Successful UTF-8 (explicit)
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=UTF-8"_ba);
    serverSideResponse.body = encUTF8(sourceString);
    VERIFY_TEXT_REPLY_OK;

    // Successful UTF-8 (obfuscated)
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=\"UT\\F-8\""_ba);
    serverSideResponse.body = encUTF8(sourceString);
    VERIFY_TEXT_REPLY_OK;

    // Successful UTF-8 (empty charset)
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=\"\""_ba);
    serverSideResponse.body = encUTF8(sourceString);
    VERIFY_TEXT_REPLY_OK;

    // Successful UTF-8 (implicit)
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain"_ba);
    serverSideResponse.body = encUTF8(sourceString);
    VERIFY_TEXT_REPLY_OK;

    // Successful UTF-16
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=UTF-16"_ba);
    serverSideResponse.body = encUTF16(sourceString);
    VERIFY_TEXT_REPLY_OK;

    // Successful UTF-16, parameter case insensitivity
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; chARset=uTf-16"_ba);
    serverSideResponse.body = encUTF16(sourceString);
    VERIFY_TEXT_REPLY_OK;

    // Successful UTF-32
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=UTF-32"_ba);
    serverSideResponse.body = encUTF32(sourceString);
    VERIFY_TEXT_REPLY_OK;

    // Successful UTF-32 with spec-wise allowed extra trailing content in the Content-Type header value
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType,
                                      "text(this is a \\)comment)/ (this (too)) plain; charset = \"UTF-32\";extraparameter=bar"_ba);
    serverSideResponse.body = encUTF32(sourceString);
    VERIFY_TEXT_REPLY_OK;

    // Successful UTF-32 with spec-wise allowed extra leading content in the Content-Type header value
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType,
                                      "text/plain; extraparameter=bar;charset    =     \"UT\\F-32\""_ba);
    serverSideResponse.body = encUTF32(sourceString);
    VERIFY_TEXT_REPLY_OK;

    {
        // Unsuccessful UTF-32, wrong encoding indicated (indicated UTF-32 but data is UTF-8)
        serverSideResponse.headers.removeAll(Header::ContentType);
        serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=UTF-32"_ba);
        serverSideResponse.body = encUTF8(sourceString);
        manager.get(request, this, [&](QRestReply &reply) { networkReply = reply.networkReply(); });
        QTRY_VERIFY(networkReply);
        QRestReply restReply(networkReply);
        responseString = restReply.readText();
        QCOMPARE_NE(responseString, sourceString);
        networkReply->deleteLater();
        networkReply = nullptr;
    }

    // Unsupported encoding
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=foo"_ba);
    serverSideResponse.body = encUTF8(sourceString);
    VERIFY_TEXT_REPLY_ERROR("readText(): Charset \"foo\" is not supported")

    // Broken UTF-8
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=UTF-8"_ba);
    serverSideResponse.body = "\xF0\x28\x8C\x28\xA0\xB0\xC0\xD0"; // invalid characters
    VERIFY_TEXT_REPLY_ERROR("readText(): Decoding error occurred");
}

void tst_QRestAccessManager::textStreaming()
{
    // Tests textual data received in chunks
    QNetworkAccessManager qnam;
    QRestAccessManager manager(&qnam);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());

    // Create long text data
    const QString expectedData = u"사랑abcd€fghiklmnΩpqrstuvwx愛사랑A사랑BCD€FGHIJKLMNΩPQRsTUVWXYZ愛"_s;
    QString cumulativeReceivedText;
    QStringEncoder encUTF8("UTF-8");
    ResponseControl *responseControl = nullptr;

    HttpData serverSideResponse; // The response data the server responds with
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=UTF-8"_ba);
    serverSideResponse.body = encUTF8(expectedData);
    serverSideResponse.status = 200;

    server.setHandler([&](const HttpData &, HttpData &response, ResponseControl &control) {
        response = serverSideResponse;
        responseControl = &control; // store for later
        control.responseChunkSize = 5; // tell testserver to send data in chunks of this size
    });

    {
        QRestReply restReply(manager.get(request));
        QObject::connect(restReply.networkReply(), &QNetworkReply::readyRead, this, [&]() {
            cumulativeReceivedText += restReply.readText();
            // Tell testserver that test is ready for next chunk
            responseControl->readyForNextChunk = true;
        });
        QTRY_VERIFY(restReply.networkReply()->isFinished());
        QCOMPARE(cumulativeReceivedText, expectedData);
        restReply.networkReply()->deleteLater();
    }

    {
        cumulativeReceivedText.clear();
        // Broken UTF-8 characters after first five ok characters
        serverSideResponse.body =
                "12345"_ba + "\xF0\x28\x8C\x28\xA0\xB0\xC0\xD0" + "abcde"_ba;
        QRestReply restReply(manager.get(request));
        QObject::connect(restReply.networkReply(), &QNetworkReply::readyRead, this, [&]() {
            static bool firstTime = true;
            if (!firstTime) // First text part is without warnings
                QTest::ignoreMessage(QtWarningMsg, "readText(): Decoding error occurred");
            firstTime = false;
            cumulativeReceivedText += restReply.readText();
            // Tell testserver that test is ready for next chunk
            responseControl->readyForNextChunk = true;
        });
        QTRY_VERIFY(restReply.networkReply()->isFinished());
        QCOMPARE(cumulativeReceivedText, "12345"_ba);
        restReply.networkReply()->deleteLater();
    }
}

QTEST_MAIN(tst_QRestAccessManager)
#include "tst_qrestaccessmanager.moc"
