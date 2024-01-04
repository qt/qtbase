// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "httptestserver_p.h"

#include <QtNetwork/qhttpmultipart.h>
#include <QtNetwork/qrestaccessmanager.h>
#include <QtNetwork/qauthenticator.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequestfactory.h>
#include <QtNetwork/qrestreply.h>

#include <QTest>
#include <QtTest/qsignalspy.h>

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
    void initTestCase();

    void initialization();
    void destruction();
    void callbacks();
    void threading();
    void networkRequestReply();
    void abort();
    void authentication();
    void userInfo();
    void errors();
    void body();
    void json();
    void text();
    void textStreaming();
    void download();
    void upload();
    void timeout();

private:
    void memberHandler(QRestReply *reply);

    friend class Transient;
    QList<QRestReply*> m_expectedReplies;
    QList<QRestReply*> m_actualReplies;
};

void tst_QRestAccessManager::initTestCase()
{
    qRegisterMetaType<QRestReply*>(); // For QSignalSpy
}

void tst_QRestAccessManager::initialization()
{
    QRestAccessManager manager;
    QVERIFY(manager.networkAccessManager());
    QCOMPARE(manager.deletesRepliesOnFinished(), true);
}

#define VERIFY_REPLY_OK(METHOD)                 \
    QTRY_VERIFY(replyFromServer);               \
    QCOMPARE(serverSideRequest.method, METHOD); \
    QVERIFY(replyFromServer->isSuccess());      \
    QVERIFY(!replyFromServer->hasError());      \
    replyFromServer->deleteLater();             \
    replyFromServer = nullptr;                  \

void tst_QRestAccessManager::networkRequestReply()
{
    // A basic test for each HTTP method against the local testserver.
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    request.setRawHeader("Content-Type"_ba, "text/plain"); // To silence missing content-type warn
    QRestReply *replyFromServer = nullptr;
    std::unique_ptr<QHttpMultiPart> multiPart;
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
    part.setBody("multipart_text");
    QByteArray ioDeviceData{"io_device_data"_ba};
    QBuffer bufferIoDevice(&ioDeviceData);

    HttpData serverSideRequest;  // The request data the server received
    HttpData serverSideResponse; // The response data the server responds with
    serverSideResponse.status = 200;
    server.setHandler([&](HttpData request, HttpData &response, ResponseControl&) {
        serverSideRequest = request;
        response = serverSideResponse;

    });
    auto callback = [&](QRestReply *reply) { replyFromServer = reply; };
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

    manager.get(request, jsonObjectData, this, callback);
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

    manager.post(request, jsonObjectData, this, callback);
    VERIFY_REPLY_OK(methodPOST);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    manager.post(request, jsonArrayData, this, callback);
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

    manager.put(request, jsonObjectData, this, callback);
    VERIFY_REPLY_OK(methodPUT);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    manager.put(request, jsonArrayData, this, callback);
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

    manager.patch(request, jsonObjectData, this, callback);
    VERIFY_REPLY_OK(methodPATCH);
    QCOMPARE(QJsonDocument::fromJson(serverSideRequest.body).object(), jsonObjectData);

    manager.patch(request, jsonArrayData, this, callback);
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

void tst_QRestAccessManager::abort()
{
    // Test aborting requests
    QRestAccessManager manager;
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());

    QSignalSpy finishedSpy(&manager, &QRestAccessManager::requestFinished);
    int callbackCount = 0;
    auto callback = [&](QRestReply*) {
        callbackCount++;
    };

    // Abort without any requests
    manager.abortRequests();
    QTest::qWait(20);
    QCOMPARE(finishedSpy.size(), 0);

    // Abort immediately after requesting
    manager.get(request, this, callback);
    manager.abortRequests();
    QTRY_COMPARE(callbackCount, 1);
    QTRY_COMPARE(finishedSpy.size(), 1);

    // Abort after request has been sent out
    server.setHandler([&](HttpData, HttpData&, ResponseControl &control) {
        control.respond = false;
        manager.abortRequests();
    });
    manager.get(request, this, callback);
    QTRY_COMPARE(callbackCount, 2);
    QTRY_COMPARE(finishedSpy.size(), 2);
}

void tst_QRestAccessManager::memberHandler(QRestReply *reply)
{
    m_actualReplies.append(reply);
}

// Class that is destroyed during an active request.
// Used to test that the callbacks won't be called in these cases
class Transient : public QObject
{
    Q_OBJECT
public:
    explicit Transient(tst_QRestAccessManager *test) :  QObject(test), m_test(test) {}

    void memberHandler(QRestReply *reply)
    {
        m_test->m_actualReplies.append(reply);
    }

private:
    tst_QRestAccessManager *m_test = nullptr;
};

template <typename Functor, std::enable_if_t<
          QtPrivate::AreFunctionsCompatible<void(*)(QRestReply*), Functor>::value, bool> = true>
inline constexpr bool isCompatibleCallback(Functor &&) { return true; }

template <typename Functor, std::enable_if_t<
          !QtPrivate::AreFunctionsCompatible<void(*)(QRestReply*), Functor>::value, bool> = true,
          typename = void>
inline constexpr bool isCompatibleCallback(Functor &&) { return false; }

void tst_QRestAccessManager::callbacks()
{
    QRestAccessManager manager;

    manager.setDeletesRepliesOnFinished(false); // Don't autodelete so we can compare results later
    QNetworkRequest request{u"i_dont_exist"_s}; // Will result in ProtocolUnknown error
    QSignalSpy managerFinishedSpy(&manager, &QRestAccessManager::requestFinished);

    auto lambdaHandler = [this](QRestReply *reply) { m_actualReplies.append(reply); };
    QRestReply *reply = nullptr;
    Transient *transient = nullptr;
    QByteArray data{"some_data"};

    // Compile-time tests for callback signatures
    static_assert(isCompatibleCallback([](QRestReply*){})); // Correct signature
    static_assert(isCompatibleCallback(lambdaHandler));
    static_assert(isCompatibleCallback(&Transient::memberHandler));
    static_assert(isCompatibleCallback([](){})); // Less parameters are allowed

    static_assert(!isCompatibleCallback([](QString){}));              // Wrong parameter type
    static_assert(!isCompatibleCallback([](QNetworkReply*){}));       // Wrong parameter type
    static_assert(!isCompatibleCallback([](const QString &){}));      // Wrong parameter type
    static_assert(!isCompatibleCallback([](QRestReply*, QString){})); // Too many parameters

    // -- Test without data
    // Without callback
    reply = manager.get(request);
    QCOMPARE(reply->isFinished(), false); // Test this once here
    m_expectedReplies.append(reply);
    QObject::connect(reply, &QRestReply::finished, lambdaHandler);

    // With lambda callback, without context object
    m_expectedReplies.append(manager.get(request, nullptr, lambdaHandler));
    m_expectedReplies.append(manager.get(request, nullptr,
                                        [this](QRestReply *reply){m_actualReplies.append(reply);}));
    // With lambda callback and context object
    m_expectedReplies.append(manager.get(request, this, lambdaHandler));
    m_expectedReplies.append(manager.get(request, this,
                                        [this](QRestReply *reply){m_actualReplies.append(reply);}));
    // With member callback and context object
    m_expectedReplies.append(manager.get(request, this, &tst_QRestAccessManager::memberHandler));
    // With context object that is destroyed, there should be no callback or eg. crash.
    transient = new Transient(this);
    manager.get(request, transient, &Transient::memberHandler); // Reply not added to expecteds
    delete transient;

    // Let requests finish
    QTRY_COMPARE(m_actualReplies.size(), m_expectedReplies.size());
    QTRY_COMPARE(managerFinishedSpy.size(), m_actualReplies.size());
    for (auto reply: m_actualReplies) {
        QVERIFY(!reply->isSuccess());
        QVERIFY(reply->hasError());
        QCOMPARE(reply->error(), QNetworkReply::ProtocolUnknownError);
        QCOMPARE(reply->isFinished(), true);
        reply->deleteLater();
    }
    m_actualReplies.clear();
    m_expectedReplies.clear();
    managerFinishedSpy.clear();

    // -- Test with data
    reply = manager.post(request, data);
    m_expectedReplies.append(reply);
    QObject::connect(reply, &QRestReply::finished, lambdaHandler);

    // With lambda callback, without context object
    m_expectedReplies.append(manager.post(request, data, nullptr, lambdaHandler));
    m_expectedReplies.append(manager.post(request, data, nullptr,
                                        [this](QRestReply *reply){m_actualReplies.append(reply);}));
    // With lambda callback and context object
    m_expectedReplies.append(manager.post(request, data, this, lambdaHandler));
    m_expectedReplies.append(manager.post(request, data, this,
                                        [this](QRestReply *reply){m_actualReplies.append(reply);}));
    // With member callback and context object
    m_expectedReplies.append(manager.post(request, data,
                                         this, &tst_QRestAccessManager::memberHandler));
    // With context object that is destroyed, there should be no callback or eg. crash
    transient = new Transient(this);
    manager.post(request, data, transient, &Transient::memberHandler); // Note: reply not expected
    delete transient;

    // Let requests finish
    QTRY_COMPARE(m_actualReplies.size(), m_expectedReplies.size());
    QTRY_COMPARE(managerFinishedSpy.size(), m_actualReplies.size());
    for (auto reply: m_actualReplies) {
        QVERIFY(!reply->isSuccess());
        QVERIFY(reply->hasError());
        QCOMPARE(reply->error(), QNetworkReply::ProtocolUnknownError);
        QCOMPARE(reply->isFinished(), true);
        reply->deleteLater();
    }
    m_actualReplies.clear();
    m_expectedReplies.clear();
    managerFinishedSpy.clear();

    // -- Test GET with data separately, as GET provides methods that are usable with and
    // without data, and fairly easy to get the qrestaccessmanager.h template SFINAE subtly wrong
    reply = manager.get(request, data);
    m_expectedReplies.append(reply);
    QObject::connect(reply, &QRestReply::finished, lambdaHandler);
    // With lambda callback, without context object
    m_expectedReplies.append(manager.get(request, data, nullptr, lambdaHandler));
    m_expectedReplies.append(manager.get(request, data, nullptr,
                                        [this](QRestReply *reply){m_actualReplies.append(reply);}));
    // With lambda callback and context object
    m_expectedReplies.append(manager.get(request, data, this, lambdaHandler));
    m_expectedReplies.append(manager.get(request, data, this,
                                        [this](QRestReply *reply){m_actualReplies.append(reply);}));
    // With member callback and context object
    m_expectedReplies.append(manager.get(request, data,
                                          this, &tst_QRestAccessManager::memberHandler));
    // With context object that is destroyed, there should be no callback or eg. crash
    transient = new Transient(this);
    manager.get(request, data, transient, &Transient::memberHandler); // Reply not added
    delete transient;

    // Let requests finish
    QTRY_COMPARE(m_actualReplies.size(), m_expectedReplies.size());
    QTRY_COMPARE(managerFinishedSpy.size(), m_actualReplies.size());
    for (auto reply: m_actualReplies) {
        QVERIFY(!reply->isSuccess());
        QVERIFY(reply->hasError());
        QCOMPARE(reply->error(), QNetworkReply::ProtocolUnknownError);
        QCOMPARE(reply->isFinished(), true);
        reply->deleteLater();
    }
    m_actualReplies.clear();
    m_expectedReplies.clear();
    managerFinishedSpy.clear();
}

class RestWorker : public QObject
{
    Q_OBJECT
public:
    explicit RestWorker(QObject *parent = nullptr) : QObject(parent)
    {
        m_manager = new QRestAccessManager(this);
    }
    QRestAccessManager *m_manager;

public slots:
    void issueRestRequest()
    {
        QNetworkRequest request{u"i_dont_exist"_s};
        m_manager->get(request, this, [this](QRestReply *reply){
            emit result(reply->body());
        });
    }
signals:
    void result(const QByteArray &data);
};

void tst_QRestAccessManager::threading()
{
    // QRestAccessManager and QRestReply are only allowed to use in the thread they live in.

    // A "sanity test" for checking that there are no problems with running the QRestAM
    // in another thread.
    QThread restWorkThread;
    RestWorker restWorker;
    restWorker.moveToThread(&restWorkThread);

    QList<QByteArray> results;
    QObject::connect(&restWorker, &RestWorker::result, this, [&](const QByteArray &data){
        results.append(data);
    });
    restWorkThread.start();

    QMetaObject::invokeMethod(&restWorker, &RestWorker::issueRestRequest);
    QTRY_COMPARE(results.size(), 1);
    restWorkThread.quit();
    restWorkThread.wait();
}

void tst_QRestAccessManager::destruction()
{
    QRestAccessManager *manager = new QRestAccessManager;
    manager->setDeletesRepliesOnFinished(false); // Don't autodelete so we can compare results later
    QNetworkRequest request{u"i_dont_exist"_s}; // Will result in ProtocolUnknown error
    m_expectedReplies.clear();
    m_actualReplies.clear();
    auto handler = [this](QRestReply *reply) { m_actualReplies.append(reply); };

    // Delete reply immediately, make sure nothing bad happens and that there is no callback
    QRestReply *reply = manager->get(request, this, handler);
    delete reply;
    QTest::qWait(20); // allow some time for the callback to arrive (it shouldn't)
    QCOMPARE(m_actualReplies.size(), m_expectedReplies.size()); // Both should be 0

    // Delete access manager immediately after request, make sure nothing bad happens
    manager->get(request, this, handler);
    manager->post(request, "data"_ba, this, handler);
    QTest::ignoreMessage(QtWarningMsg, "Access manager destroyed while 2 requests were still"
                                       " in progress");
    delete manager;
    QTest::qWait(20);
    QCOMPARE(m_actualReplies.size(), m_expectedReplies.size()); // Both should be 0
}

void tst_QRestAccessManager::authentication()
{
    // Test the case where server responds with '401' (authentication required).
    // The QRestAM emits an authenticationRequired signal, which is used to the username/password.
    // The QRestAM/QNAM underneath then automatically resends the request.
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    QRestReply *replyFromServer = nullptr;

    HttpData serverSideRequest;
    server.setHandler([&](HttpData request, HttpData &response, ResponseControl&) {
        if (!request.headers.contains(Header::Authorization)) {
            response.status = 401;
            response.headers.append(Header::WWWAuthenticate, "Basic realm=\"secret_place\""_ba);
        } else {
            response.status = 200;
        }
        serverSideRequest = request; // store for checking later the 'Authorization' header value
    });

    QObject::connect(&manager, &QRestAccessManager::authenticationRequired, this,
                     [](QRestReply*, QAuthenticator *authenticator) {
                         authenticator->setUser(u"a_user"_s);
                         authenticator->setPassword(u"a_password"_s);
                     });

    // Issue a GET request without any authorization data.
    int finishedCount = 0;
    manager.get(request, this, [&](QRestReply *reply) {
        finishedCount++;
        replyFromServer = reply;
    });
    QTRY_VERIFY(replyFromServer);
    // Server and QRestAM/QNAM exchange req/res twice, but finished() should be emitted just once
    QCOMPARE(finishedCount, 1);
    const auto resultHeaders = serverSideRequest.headers.values(Header::Authorization);
    QVERIFY(!resultHeaders.empty());
    QCOMPARE(resultHeaders.first(), "Basic YV91c2VyOmFfcGFzc3dvcmQ="_ba);
}

void tst_QRestAccessManager::userInfo()
{
    // Tests setting of username and password into the request factory
    using ReplyPtr = std::unique_ptr<QRestReply, QScopedPointerDeleteLater>;
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());

    QNetworkRequestFactory factory(server.url());
    factory.setUserName(u"a_user"_s);
    const auto password = u"a_password"_s;
    factory.setPassword(password);

    HttpData serverSideRequest;
    server.setHandler([&](HttpData request, HttpData& response, ResponseControl&) {
        if (!request.headers.contains(Header::Authorization)) {
            response.status = 401;
            response.headers.append(Header::WWWAuthenticate,"Basic realm=\"secret_place\""_ba);
        } else {
            response.status = 200;
        }
        serverSideRequest = request; // store for checking later the 'Authorization' header value
    });

    ReplyPtr reply(manager.get(factory.createRequest()));
    QTRY_VERIFY(reply.get()->isFinished());
    QVERIFY(reply.get()->isSuccess());
    QCOMPARE(reply.get()->httpStatus(), 200);
    const auto resultHeaders = serverSideRequest.headers.values(Header::Authorization);
    QVERIFY(!resultHeaders.empty());
    QCOMPARE(resultHeaders.first(), "Basic YV91c2VyOmFfcGFzc3dvcmQ="_ba);

    // Verify that debug output does not contain password
    QString debugOutput;
    QDebug debug(&debugOutput);
    debug << factory;
    QVERIFY(debugOutput.contains("password = (is set)"));
    QVERIFY(!debugOutput.contains(password));
}

#define VERIFY_HTTP_ERROR_STATUS(STATUS) \
    serverSideResponse.status = STATUS; \
    reply = manager.get(request); \
    QObject::connect(reply, &QRestReply::errorOccurred, this, \
                    [&](){ errorSignalReceived = true; }); \
    QTRY_VERIFY(reply->isFinished()); \
    QVERIFY(!errorSignalReceived); \
    QVERIFY(!reply->hasError()); \
    QCOMPARE(reply->httpStatus(), serverSideResponse.status); \
    QCOMPARE(reply->error(), QNetworkReply::NetworkError::NoError); \
    QVERIFY(!reply->isSuccess()); \
    reply->deleteLater(); \

void tst_QRestAccessManager::errors()
{
    // Tests the distinction between HTTP and other (network/protocol) errors
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    QRestReply *reply = nullptr;
    bool errorSignalReceived = false;

    HttpData serverSideResponse; // The response data the server responds with
    server.setHandler([&](HttpData, HttpData &response, ResponseControl &) {
        response  = serverSideResponse;
    });

    // Test few HTTP statuses in different categories
    VERIFY_HTTP_ERROR_STATUS(103) // QNetworkReply::NoError
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

    // Test that actual network/protocol errors come through
    reply = manager.get({}); // Empty url
    QObject::connect(reply, &QRestReply::errorOccurred, this, [&](){ errorSignalReceived = true; });
    QTRY_VERIFY(reply->isFinished());
    QTRY_VERIFY(errorSignalReceived);
    QVERIFY(reply->hasError());
    QVERIFY(!reply->isSuccess());
    QCOMPARE(reply->error(), QNetworkReply::ProtocolUnknownError);
    reply->deleteLater();
    errorSignalReceived = false;

    reply = manager.get(QNetworkRequest{{"http://non-existent.foo.bar.test"}});
    QObject::connect(reply, &QRestReply::errorOccurred, this, [&](){ errorSignalReceived = true; });
    QTRY_VERIFY(reply->isFinished());
    QTRY_VERIFY(errorSignalReceived);
    QVERIFY(reply->hasError());
    QVERIFY(!reply->isSuccess());
    QCOMPARE(reply->error(), QNetworkReply::HostNotFoundError);
    reply->deleteLater();
    errorSignalReceived = false;

    reply = manager.get(request);
    QObject::connect(reply, &QRestReply::errorOccurred, this, [&](){ errorSignalReceived = true; });
    reply->abort();
    QTRY_VERIFY(reply->isFinished());
    QTRY_VERIFY(errorSignalReceived);
    QVERIFY(reply->hasError());
    QVERIFY(!reply->isSuccess());
    QCOMPARE(reply->error(), QNetworkReply::OperationCanceledError);
    reply->deleteLater();
    errorSignalReceived = false;
}

void tst_QRestAccessManager::body()
{
    // Test using QRestReply::body() data accessor
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    QRestReply *replyFromServer = nullptr;

    HttpData serverSideRequest;  // The request data the server received
    HttpData serverSideResponse; // The response data the server responds with
    server.setHandler([&](HttpData request, HttpData &response, ResponseControl&) {
        serverSideRequest = request;
        response = serverSideResponse;
    });

    serverSideResponse.status = 200;
    serverSideResponse.body = "some_data"_ba;
    manager.get(request, this, [&](QRestReply *reply) { replyFromServer = reply; });
    QTRY_VERIFY(replyFromServer);
    QCOMPARE(replyFromServer->body(), serverSideResponse.body);
    QCOMPARE(replyFromServer->httpStatus(), serverSideResponse.status);
    QVERIFY(!replyFromServer->hasError());
    QVERIFY(replyFromServer->isSuccess());
    replyFromServer->deleteLater();
    replyFromServer = nullptr;

    serverSideResponse.body = ""_ba; // Empty
    manager.get(request, this, [&](QRestReply *reply) { replyFromServer = reply; });
    QTRY_VERIFY(replyFromServer);
    QCOMPARE(replyFromServer->body(), serverSideResponse.body);
    replyFromServer->deleteLater();
    replyFromServer = nullptr;

    serverSideResponse.status = 500;
    serverSideResponse.body = "some_other_data"_ba;
    manager.get(request, this, [&](QRestReply *reply) { replyFromServer = reply; });
    QTRY_VERIFY(replyFromServer);
    QCOMPARE(replyFromServer->body(), serverSideResponse.body);
    QCOMPARE(replyFromServer->httpStatus(), serverSideResponse.status);
    QVERIFY(!replyFromServer->hasError());
    QVERIFY(!replyFromServer->isSuccess());
    replyFromServer->deleteLater();
    replyFromServer = nullptr;
}

void tst_QRestAccessManager::json()
{
    // Test using QRestReply::json() and jsonArray() data accessors
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    QRestReply *replyFromServer = nullptr;
    QJsonDocument responseJsonDocument;
    QJsonParseError parseError;

    HttpData serverSideRequest;  // The request data the server received
    HttpData serverSideResponse; // The response data the server responds with
    serverSideResponse.status = 200;
    server.setHandler([&](HttpData request, HttpData &response, ResponseControl&) {
        serverSideRequest = request;
        response = serverSideResponse;
    });

    // Test receiving valid json object
    serverSideResponse.body = "{\"key1\":\"value1\",""\"key2\":\"value2\"}\n"_ba;
    replyFromServer = manager.get(request);
    // Read unfinished reply
    QVERIFY(!replyFromServer->isFinished());
    QTest::ignoreMessage(QtWarningMsg, "Attempt to read json() of an unfinished reply, ignoring.");
    parseError.error = QJsonParseError::ParseError::DocumentTooLarge; // Reset to impossible value
    QVERIFY(!replyFromServer->json(&parseError));
    QCOMPARE(parseError.error, QJsonParseError::ParseError::NoError);
    // Read finished reply
    QTRY_VERIFY(replyFromServer->isFinished());
    parseError.error = QJsonParseError::ParseError::DocumentTooLarge;
    std::optional json = replyFromServer->json(&parseError);
    QVERIFY(json);
    QCOMPARE(parseError.error, QJsonParseError::ParseError::NoError);
    responseJsonDocument = *json;
    QVERIFY(responseJsonDocument.isObject());
    QCOMPARE(responseJsonDocument["key1"], "value1");
    QCOMPARE(responseJsonDocument["key2"], "value2");
    replyFromServer->deleteLater();
    replyFromServer = nullptr;

    // Test receiving an invalid json object
    serverSideResponse.body = "foobar"_ba;
    manager.get(request, this, [&](QRestReply *reply) { replyFromServer = reply; });
    QTRY_VERIFY(replyFromServer);
    parseError.error = QJsonParseError::ParseError::DocumentTooLarge;
    QVERIFY(!replyFromServer->json(&parseError).has_value()); // std::nullopt returned
    QCOMPARE_NE(parseError.error, QJsonParseError::ParseError::NoError);
    QCOMPARE_NE(parseError.error, QJsonParseError::ParseError::DocumentTooLarge);
    QCOMPARE_GT(parseError.offset, 0);
    replyFromServer->deleteLater();
    replyFromServer = nullptr;

    // Test receiving valid json array
    serverSideResponse.body = "[\"foo\", \"bar\"]\n"_ba;
    manager.get(request, this, [&](QRestReply *reply) { replyFromServer = reply; });
    QTRY_VERIFY(replyFromServer);
    parseError.error = QJsonParseError::ParseError::DocumentTooLarge;
    json = replyFromServer->json(&parseError);
    QCOMPARE(parseError.error, QJsonParseError::ParseError::NoError);
    QVERIFY(json);
    responseJsonDocument = *json;
    QVERIFY(responseJsonDocument.isArray());
    QCOMPARE(responseJsonDocument.array().size(), 2);
    QCOMPARE(responseJsonDocument[0].toString(), "foo"_L1);
    QCOMPARE(responseJsonDocument[1].toString(), "bar"_L1);
    replyFromServer->deleteLater();
    replyFromServer = nullptr;

    // Test receiving an invalid json array
    serverSideResponse.body = "foobar"_ba;
    manager.get(request, this, [&](QRestReply *reply) { replyFromServer = reply; });
    QTRY_VERIFY(replyFromServer);
    parseError.error = QJsonParseError::ParseError::DocumentTooLarge;
    QVERIFY(!replyFromServer->json(&parseError).has_value()); // std::nullopt returned
    QCOMPARE_NE(parseError.error, QJsonParseError::ParseError::NoError);
    QCOMPARE_NE(parseError.error, QJsonParseError::ParseError::DocumentTooLarge);
    QCOMPARE_GT(parseError.offset, 0);
    replyFromServer->deleteLater();
    replyFromServer = nullptr;
}

#define VERIFY_TEXT_REPLY_OK \
    manager.get(request, this, [&](QRestReply *reply) { replyFromServer = reply; }); \
    QTRY_VERIFY(replyFromServer); \
    responseString = replyFromServer->text(); \
    QCOMPARE(responseString, sourceString); \
    replyFromServer->deleteLater(); \
    replyFromServer = nullptr; \

#define VERIFY_TEXT_REPLY_ERROR(WARNING_MESSAGE) \
    manager.get(request, this, [&](QRestReply *reply) { replyFromServer = reply; }); \
    QTRY_VERIFY(replyFromServer); \
    QTest::ignoreMessage(QtWarningMsg, WARNING_MESSAGE); \
    responseString = replyFromServer->text(); \
    QVERIFY(responseString.isEmpty()); \
    replyFromServer->deleteLater(); \
    replyFromServer = nullptr; \

void tst_QRestAccessManager::text()
{
    // Test using QRestReply::text() data accessor with various text encodings
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    QRestReply *replyFromServer = nullptr;
    QJsonObject responseJsonObject;

    QStringEncoder encUTF8("UTF-8");
    QStringEncoder encUTF16("UTF-16");
    QStringEncoder encUTF32("UTF-32");
    QString responseString;

    HttpData serverSideRequest;  // The request data the server received
    HttpData serverSideResponse; // The response data the server responds with
    serverSideResponse.status = 200;
    server.setHandler([&](HttpData request, HttpData &response, ResponseControl&) {
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

    // Successful UTF-8
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=UTF-8"_ba);
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

    // Successful UTF-32 with spec-wise allowed extra content in the Content-Type header value
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType,
                                      "text/plain; charset = \"UTF-32\";extraparameter=bar"_ba);
    serverSideResponse.body = encUTF32(sourceString);
    VERIFY_TEXT_REPLY_OK;

    // Unsuccessful UTF-32, wrong encoding indicated (indicated charset UTF-32 but data is UTF-8)
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=UTF-32"_ba);
    serverSideResponse.body = encUTF8(sourceString);
    manager.get(request, this, [&](QRestReply *reply) { replyFromServer = reply; });
    QTRY_VERIFY(replyFromServer);
    responseString = replyFromServer->text();
    QCOMPARE_NE(responseString, sourceString);
    replyFromServer->deleteLater();
    replyFromServer = nullptr;

    // Unsupported encoding
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=foo"_ba);
    serverSideResponse.body = encUTF8(sourceString);
    VERIFY_TEXT_REPLY_ERROR("text(): Charset \"foo\" is not supported")

    // Broken UTF-8
    serverSideResponse.headers.removeAll(Header::ContentType);
    serverSideResponse.headers.append(Header::ContentType, "text/plain; charset=UTF-8"_ba);
    serverSideResponse.body = "\xF0\x28\x8C\x28\xA0\xB0\xC0\xD0"; // invalid characters
    VERIFY_TEXT_REPLY_ERROR("text() Decoding error occurred");
}

void tst_QRestAccessManager::textStreaming()
{
    // Tests textual data received in chunks
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());

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

    server.setHandler([&](HttpData, HttpData &response, ResponseControl &control) {
        response = serverSideResponse;
        responseControl = &control; // store for later
        control.responseChunkSize = 5; // tell testserver to send data in chunks of this size
    });

    QNetworkRequest request(server.url());
    QRestReply *reply = manager.get(request);
    QObject::connect(reply, &QRestReply::readyRead, this, [&](QRestReply *reply) {
        cumulativeReceivedText += reply->text();
        // Tell testserver that test is ready for next chunk
        responseControl->readyForNextChunk = true;
    });
    QTRY_VERIFY(reply->isFinished());
    QCOMPARE(cumulativeReceivedText, expectedData);

    cumulativeReceivedText.clear();
    // Broken UTF-8 characters after first five ok characters
    serverSideResponse.body =
            "12345"_ba + "\xF0\x28\x8C\x28\xA0\xB0\xC0\xD0" + "abcde"_ba;
    reply = manager.get(request);
    QObject::connect(reply, &QRestReply::readyRead, this, [&](QRestReply *reply) {
        static bool firstTime = true;
        if (!firstTime) // First text part is without warnings
            QTest::ignoreMessage(QtWarningMsg, "text() Decoding error occurred");
        firstTime = false;
        cumulativeReceivedText += reply->text();
        // Tell testserver that test is ready for next chunk
        responseControl->readyForNextChunk = true;
    });
    QTRY_VERIFY(reply->isFinished());
    QCOMPARE(cumulativeReceivedText, "12345"_ba);
}

void tst_QRestAccessManager::download()
{
    // Test case where data is received in chunks.
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    HttpData serverSideResponse; // The response data the server responds with
    constexpr qsizetype dataSize = 1 * 1024 * 1024;  // 1 MB
    QByteArray expectedData{dataSize, 0};
    for (qsizetype i = 0; i < dataSize; ++i) // initialize the data we download
        expectedData[i] = i % 100;
    QByteArray cumulativeReceivedData;
    qsizetype cumulativeReceivedBytesAvailable = 0;

    serverSideResponse.body = expectedData;
    ResponseControl *responseControl = nullptr;
    serverSideResponse.status = 200;
    // Set content-length header so that underlying QNAM is able to report bytesTotal correctly
    serverSideResponse.headers.append(Header::ContentType,
                                       QString::number(expectedData.size()).toLatin1());
    server.setHandler([&](HttpData, HttpData &response, ResponseControl &control) {
        response = serverSideResponse;
        responseControl = &control; // store for later
        control.responseChunkSize = 1024; // tell testserver to send data in chunks of this size
    });

    QRestReply* reply = manager.get(request, this, [&responseControl](QRestReply */*reply*/){
        responseControl = nullptr; // all finished, no more need for controlling the response
    });

    QObject::connect(reply, &QRestReply::readyRead, this, [&](QRestReply *reply) {
        static bool testOnce = true;
        if (!reply->isFinished() && testOnce) {
            // Test once that reading json of an unfinished reply will not work
            testOnce = false;
            QTest::ignoreMessage(QtWarningMsg, "Attempt to read json() of an unfinished"
                                               " reply, ignoring.");
            reply->json();
        }

        cumulativeReceivedBytesAvailable += reply->bytesAvailable();
        cumulativeReceivedData += reply->body();
        // Tell testserver that test is ready for next chunk
        responseControl->readyForNextChunk = true;
    });

    qint64 totalBytes = 0;
    qint64 receivedBytes = 0;
    QObject::connect(reply, &QRestReply::downloadProgress, this,
                     [&](qint64 bytesReceived, qint64 bytesTotal) {
                         if (totalBytes == 0 && bytesTotal > 0)
                             totalBytes = bytesTotal;
                         receivedBytes = bytesReceived;
                     });
    QTRY_VERIFY(reply->isFinished());
    reply->deleteLater();
    reply = nullptr;
    // Checks specific for readyRead() and bytesAvailable()
    QCOMPARE(cumulativeReceivedData, expectedData);
    QCOMPARE(cumulativeReceivedBytesAvailable, expectedData.size());
    // Checks specific for downloadProgress()
    QCOMPARE(totalBytes, expectedData.size());
    QCOMPARE(receivedBytes, expectedData.size());
}

void tst_QRestAccessManager::upload()
{
    // This test tests uploadProgress signal
    QRestAccessManager manager;
    manager.setDeletesRepliesOnFinished(false);
    HttpTestServer server;
    QTRY_VERIFY(server.isListening());
    QNetworkRequest request(server.url());
    request.setRawHeader("Content-Type"_ba, "text/plain"); // To silence missing content-type warn
    QByteArray expectedData{1 * 1024 * 1024, 0}; // 1 MB
    server.setHandler([&](HttpData, HttpData &, ResponseControl &) {});

    QRestReply* reply = manager.post(request, expectedData);
    QSignalSpy uploadProgressSpy(reply, &QRestReply::uploadProgress);
    QTRY_VERIFY(reply->isFinished());
    QVERIFY(!uploadProgressSpy.isEmpty());
    reply->deleteLater();

    // Check that bytesTotal is correct already in the first signal
    const QList<QVariant> first = uploadProgressSpy.first();
    QCOMPARE(first.size(), 3);
    QCOMPARE(first.at(1).toLongLong(), expectedData.size());

    // Check that we sent all bytes
    const QList<QVariant> last = uploadProgressSpy.last();
    QCOMPARE(last.size(), 3);
    QEXPECT_FAIL("", "Fails due to QTBUG-44782", Continue);
    QCOMPARE(last.at(0).toLongLong(), expectedData.size());
}

void tst_QRestAccessManager::timeout()
{
    constexpr auto defaultTimeout = 0ms;
    constexpr auto timeout = 150ms;

    QRestAccessManager manager;
    QCOMPARE(manager.transferTimeout(), defaultTimeout);
    QCOMPARE(manager.networkAccessManager()->transferTimeoutAsDuration(), defaultTimeout);

    manager.setTransferTimeout(timeout);
    QCOMPARE(manager.transferTimeout(), timeout);
    QCOMPARE(manager.networkAccessManager()->transferTimeoutAsDuration(), timeout);
}

QTEST_MAIN(tst_QRestAccessManager)
#include "tst_qrestaccessmanager.moc"
