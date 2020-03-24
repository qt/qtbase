/****************************************************************************
 **
 ** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtNetwork/private/qsslsocket_openssl_symbols_p.h>
#include <QtNetwork/private/qsslsocket_openssl_p.h>

#include <QtNetwork/qsslcertificate.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qsslerror.h>
#include <QtNetwork/qsslkey.h>
#include <QtNetwork/qssl.h>

#include <QtCore/qsharedpointer.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qstring.h>
#include <QtCore/qfile.h>
#include <QtCore/qlist.h>
#include <QtCore/qdir.h>

#include <openssl/ocsp.h>

#include <algorithm>
#include <utility>

// NOTE: the word 'subject' in the code below means the subject of a status request,
// so in general it's our peer's certificate we are asking about.

using SslError = QT_PREPEND_NAMESPACE(QSslError);
using VectorOfErrors = QT_PREPEND_NAMESPACE(QVector<SslError>);
using Latin1String = QT_PREPEND_NAMESPACE(QLatin1String);

Q_DECLARE_METATYPE(SslError)
Q_DECLARE_METATYPE(VectorOfErrors)
Q_DECLARE_METATYPE(Latin1String)

QT_BEGIN_NAMESPACE

namespace {

using OcspResponse = QSharedPointer<OCSP_RESPONSE>;
using BasicResponse = QSharedPointer<OCSP_BASICRESP>;
using SingleResponse = QSharedPointer<OCSP_SINGLERESP>;
using CertId = QSharedPointer<OCSP_CERTID>;
using EvpKey = QSharedPointer<EVP_PKEY>;
using Asn1Time = QSharedPointer<ASN1_TIME>;
using CertificateChain = QList<QSslCertificate>;

using NativeX509Ptr = X509 *;

class X509Stack {
public:
    explicit X509Stack(const QList<QSslCertificate> &chain);

    ~X509Stack();

    int size() const;
    X509 *operator[](int index) const;
    operator STACK_OF(X509) *() const;

private:
    OPENSSL_STACK *stack = nullptr;

    Q_DISABLE_COPY(X509Stack)
};

X509Stack::X509Stack(const QList<QSslCertificate> &chain)
{
    if (!chain.size())
        return;

    stack = q_OPENSSL_sk_new_null();
    if (!stack)
        return;

    for (const QSslCertificate &cert : chain) {
        X509 *nativeCert = NativeX509Ptr(cert.handle());
        if (!nativeCert)
            continue;
        q_OPENSSL_sk_push(stack, nativeCert);
        q_X509_up_ref(nativeCert);
    }
}

X509Stack::~X509Stack()
{
    if (stack)
        q_OPENSSL_sk_pop_free(stack, reinterpret_cast<void(*)(void*)>(q_X509_free));
}

int X509Stack::size() const
{
    if (stack)
        return q_OPENSSL_sk_num(stack);
    return 0;
}

X509 *X509Stack::operator[](int index) const
{
    return NativeX509Ptr(q_OPENSSL_sk_value(stack, index));
}

X509Stack::operator STACK_OF(X509) *() const
{
    return reinterpret_cast<STACK_OF(X509)*>(stack);
}

struct OcspTimeStamp
{
    OcspTimeStamp() = default;
    OcspTimeStamp(long secondsBeforeNow, long secondsAfterNow);

    static Asn1Time timeToAsn1Time(long adjustment);

    Asn1Time thisUpdate;
    Asn1Time nextUpdate;
};

OcspTimeStamp::OcspTimeStamp(long secondsBeforeNow, long secondsAfterNow)
{
    Asn1Time start = timeToAsn1Time(secondsBeforeNow);
    Asn1Time end = timeToAsn1Time(secondsAfterNow);
    if (start.data() && end.data()) {
        thisUpdate.swap(start);
        nextUpdate.swap(end);
    }
}

Asn1Time OcspTimeStamp::timeToAsn1Time(long adjustment)
{
    if (ASN1_TIME *adjusted = q_X509_gmtime_adj(nullptr, adjustment))
        return Asn1Time(adjusted, q_ASN1_TIME_free);
    return Asn1Time{};
}

struct OcspResponder
{
    OcspResponder(const OcspTimeStamp &stamp, const CertificateChain &subjs,
                  const CertificateChain &respChain, const QSslKey &respPKey);

    QByteArray buildResponse(int responseStatus, int certificateStatus) const;
    static EvpKey privateKeyToEVP_PKEY(const QSslKey &privateKey);
    static CertId certificateToCertId(X509 *subject, X509 *issuer);
    static QByteArray responseToDer(OCSP_RESPONSE *response);

    OcspTimeStamp timeStamp;
    // Plural, we can send a 'wrong' BasicResponse containing more than
    // 1 SingleResponse.
    X509Stack subjects;
    X509Stack responderChain;
    QSslKey responderKey;
};

OcspResponder::OcspResponder(const OcspTimeStamp &stamp, const CertificateChain &subjs,
                             const CertificateChain &respChain, const QSslKey &respPKey)
    : timeStamp(stamp),
      subjects(subjs),
      responderChain(respChain),
      responderKey(respPKey)
{
}

QByteArray OcspResponder::buildResponse(int responseStatus, int certificateStatus) const
{
    if (responseStatus != OCSP_RESPONSE_STATUS_SUCCESSFUL) {
        OCSP_RESPONSE *response = q_OCSP_response_create(responseStatus, nullptr);
        if (!response)
            return {};
        const OcspResponse rGuard(response, q_OCSP_RESPONSE_free);
        return responseToDer(response);
    }

    Q_ASSERT(subjects.size() && responderChain.size() && responderKey.handle());

    const EvpKey nativeKey = privateKeyToEVP_PKEY(responderKey);
    if (!nativeKey.data())
        return {};

    OCSP_BASICRESP *basicResponse = q_OCSP_BASICRESP_new();
    if (!basicResponse)
        return {};
    const BasicResponse brGuard(basicResponse, q_OCSP_BASICRESP_free);

    for (int i = 0, e = subjects.size(); i < e; ++i) {
        X509 *subject = subjects[i];
        Q_ASSERT(subject);
        CertId certId = certificateToCertId(subject, responderChain[0]);
        if (!certId.data())
            return {};

        // NOTE: we do not own this 'singleResponse':
        ASN1_TIME *revisionTime = certificateStatus == V_OCSP_CERTSTATUS_REVOKED ?
                                                       timeStamp.thisUpdate.data() : nullptr;

        if (!q_OCSP_basic_add1_status(basicResponse, certId.data(), certificateStatus, 0, revisionTime,
                                      timeStamp.thisUpdate.data(), timeStamp.nextUpdate.data())) {
            return {};
        }
    }

    if (q_OCSP_basic_sign(basicResponse, responderChain[0], nativeKey.data(), q_EVP_sha1(),
                          responderChain, 0) != 1) {
        return {};
    }

    OCSP_RESPONSE *ocspResponse = q_OCSP_response_create(OCSP_RESPONSE_STATUS_SUCCESSFUL, basicResponse);
    if (!ocspResponse)
        return {};
    const OcspResponse rGuard(ocspResponse, q_OCSP_RESPONSE_free);
    return responseToDer(ocspResponse);
}

EvpKey OcspResponder::privateKeyToEVP_PKEY(const QSslKey &privateKey)
{
    const EvpKey nullKey;
    if (privateKey.isNull() || privateKey.algorithm() != QSsl::Rsa) {
        // We use only RSA keys in this auto-test, since we test OCSP only,
        // not handshake/TLS in general.
        return nullKey;
    }

    EVP_PKEY *nativeKey = q_EVP_PKEY_new();
    if (!nativeKey)
        return nullKey;

    const EvpKey keyGuard(nativeKey, q_EVP_PKEY_free);
    if (!q_EVP_PKEY_set1_RSA(nativeKey, reinterpret_cast<RSA *>(privateKey.handle())))
        return nullKey;

    return keyGuard;
}

CertId OcspResponder::certificateToCertId(X509 *subject, X509 *issuer)
{
    const CertId nullId;
    if (!subject || !issuer)
        return nullId;

    const EVP_MD *digest = q_EVP_sha1();
    if (!digest)
        return nullId;

    OCSP_CERTID *certId = q_OCSP_cert_to_id(digest, subject, issuer);
    if (!certId)
        return nullId;

    return CertId(certId, q_OCSP_CERTID_free);
}

QByteArray OcspResponder::responseToDer(OCSP_RESPONSE *response)
{
    if (!response)
        return {};

    const int derSize = q_i2d_OCSP_RESPONSE(response, nullptr);
    if (derSize <= 0)
        return {};

    QByteArray derData(derSize, Qt::Uninitialized);
    unsigned char *pData = reinterpret_cast<unsigned char *>(derData.data());
    const int serializedSize = q_i2d_OCSP_RESPONSE(response, &pData);
    if (serializedSize != derSize)
        return {};

    return derData;
}

// The QTcpServer capable of sending OCSP status responses.
class OcspServer : public QTcpServer
{
    Q_OBJECT

public:
    OcspServer(const CertificateChain &serverChain, const QSslKey &privateKey);

    void configureResponse(const QByteArray &responseDer);
    QString hostName() const;
    QString peerVerifyName() const;

Q_SIGNALS:
    void internalServerError();

private:
    void incomingConnection(qintptr descriptor) override;

public:
    QSslConfiguration serverConfig;
    QSslSocket serverSocket;
};

OcspServer::OcspServer(const CertificateChain &serverChain, const QSslKey &privateKey)
{
    Q_ASSERT(serverChain.size());
    Q_ASSERT(!privateKey.isNull());

    serverConfig = QSslConfiguration::defaultConfiguration();
    serverConfig.setLocalCertificateChain(serverChain);
    serverConfig.setPrivateKey(privateKey);
}

void OcspServer::configureResponse(const QByteArray &responseDer)
{
    serverConfig.setBackendConfigurationOption("Qt-OCSP-response", responseDer);
}

QString OcspServer::hostName() const
{
    // It's 'name' and not 'address' to be consistent with QSslSocket's naming style,
    // where it's connectToHostEncrypted(hostName, ...)
    const QHostAddress &addr = serverAddress();
    if (addr == QHostAddress::Any || addr == QHostAddress::AnyIPv4)
        return QStringLiteral("127.0.0.1");
    if (addr == QHostAddress::AnyIPv6)
        return QStringLiteral("::1");
    return addr.toString();
}

QString OcspServer::peerVerifyName() const
{
    const CertificateChain &localChain = serverConfig.localCertificateChain();
    if (localChain.isEmpty())
        return {};
    const auto cert = localChain.first();
    if (cert.isNull())
        return {};

    const QStringList &names = cert.subjectInfo(QSslCertificate::CommonName);
    return names.isEmpty() ? QString{} : names.first();
}

void OcspServer::incomingConnection(qintptr socketDescriptor)
{
    close();

    if (!serverSocket.setSocketDescriptor(socketDescriptor)) {
        emit internalServerError();
        return;
    }

    serverSocket.setSslConfiguration(serverConfig);
    // Since we test a client, not a server, we don't care about any
    // possible errors on the server (QAbstractSocket or QSslSocket-related).
    // Thus, we don't connect to any error signal.
    serverSocket.startServerEncryption();
}

} // unnamed namespace

class tst_QOcsp : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();

private slots:
    void connectSelfSigned();
    void badStatus_data();
    void badStatus();
    void multipleSingleResponses();
    void malformedResponse();
    void expiredResponse_data();
    void expiredResponse();
    void noNextUpdate();
    void wrongCertificateInResponse_data();
    void wrongCertificateInResponse();
    void untrustedResponder();

    // OCSPTODO: more tests in future ...

private:
    void setupOcspClient(QSslSocket &clientSocket, const CertificateChain &trustedCAs,
                         const QString &peerName);
    bool containsOcspErrors(const QList<QSslError> &errorsFound) const;
    static bool containsError(const QList<QSslError> &errors, QSslError::SslError code);
    static QByteArray goodResponse(const CertificateChain &subject, const CertificateChain &responder,
                                   const QSslKey &privateKey, long beforeNow = -1000, long afterNow = 1000);
    static bool loadPrivateKey(const QString &keyName, QSslKey &key);
    static CertificateChain issuerToChain(const CertificateChain &chain);
    static CertificateChain subjectToChain(const CertificateChain &chain);

    static QString certDirPath;

    void (QSslSocket::*tlsErrorsSignal)(const QList<QSslError> &) = &QSslSocket::sslErrors;
    void (QTestEventLoop::*exitLoopSlot)() = &QTestEventLoop::exitLoop;

    const int handshakeTimeoutMS = 500;
    QTestEventLoop loop;

    std::vector<QSslError::SslError> ocspErrorCodes = {QSslError::OcspNoResponseFound,
                                                       QSslError::OcspMalformedRequest,
                                                       QSslError::OcspMalformedResponse,
                                                       QSslError::OcspInternalError,
                                                       QSslError::OcspTryLater,
                                                       QSslError::OcspSigRequred,
                                                       QSslError::OcspUnauthorized,
                                                       QSslError::OcspResponseCannotBeTrusted,
                                                       QSslError::OcspResponseCertIdUnknown,
                                                       QSslError::OcspResponseExpired,
                                                       QSslError::OcspStatusUnknown};
};

#define QCOMPARE_SINGLE_ERROR(sslSocket, expectedError) \
    const auto &tlsErrors = sslSocket.sslHandshakeErrors(); \
    QCOMPARE(tlsErrors.size(), 1); \
    QCOMPARE(tlsErrors[0].error(), expectedError)

#define QVERIFY_HANDSHAKE_WITHOUT_ERRORS(sslSocket) \
    QVERIFY(sslSocket.isEncrypted()); \
    QCOMPARE(sslSocket.state(), QAbstractSocket::ConnectedState); \
    QVERIFY(sslSocket.sslHandshakeErrors().isEmpty())

#define QDECLARE_CHAIN(object, chainFileName) \
    CertificateChain object = QSslCertificate::fromPath(certDirPath + QLatin1String(chainFileName)); \
    QVERIFY(object.size())

#define QDECLARE_PRIVATE_KEY(key, keyFileName) \
    QSslKey key; \
    QVERIFY(loadPrivateKey(QLatin1String(keyFileName), key))

QString tst_QOcsp::certDirPath;

void tst_QOcsp::initTestCase()
{
    QVERIFY(QSslSocket::supportsSsl());

    certDirPath = QFileInfo(QFINDTESTDATA("certs")).absolutePath();
    QVERIFY(certDirPath.size() > 0);
    certDirPath += QDir::separator() + QStringLiteral("certs") + QDir::separator();
}

void tst_QOcsp::connectSelfSigned()
{
    // This test may look a bit confusing, since we have essentially 1
    // self-signed certificate, which we trust for the purpose of this test,
    // but we also request its (the certificate's) status and then we sign
    // the status response using the same certificate and the corresponding
    // private key. Anyway, we test the very basic things here: we send
    // an OCSP status request, we verify the response (if server has sent it),
    // and detect errors (if any).
    QDECLARE_CHAIN(subjectChain, "ss1.crt");
    QDECLARE_CHAIN(responderChain, "ss1.crt");
    QDECLARE_PRIVATE_KEY(privateKey, "ss1-private.key");
    {
        // This server ignores our status request:
        const QSslError::SslError expectedError = QSslError::OcspNoResponseFound;

        OcspServer server(subjectChain, privateKey);
        QVERIFY(server.listen());
        connect(&server, &OcspServer::internalServerError, &loop, exitLoopSlot);

        QSslSocket clientSocket;
        QSslConfiguration clientConfig = QSslConfiguration::defaultConfiguration();
        auto roots = clientConfig.caCertificates();
        setupOcspClient(clientSocket, issuerToChain(subjectChain), server.peerVerifyName());
        clientSocket.connectToHostEncrypted(server.hostName(), server.serverPort());
        loop.enterLoopMSecs(handshakeTimeoutMS);

        QVERIFY(!clientSocket.isEncrypted());
        QCOMPARE_SINGLE_ERROR(clientSocket, expectedError);
    }
    {
        // Now the server will send a valid 'status: good' response.
        OcspServer server(subjectChain, privateKey);
        const QByteArray response(goodResponse(subjectChain, responderChain, privateKey));
        QVERIFY(response.size());
        server.configureResponse(response);
        QVERIFY(server.listen());

        QSslSocket clientSocket;
        setupOcspClient(clientSocket, issuerToChain(subjectChain), server.peerVerifyName());
        clientSocket.connectToHostEncrypted(server.hostName(), server.serverPort());
        loop.enterLoopMSecs(handshakeTimeoutMS);

        QVERIFY_HANDSHAKE_WITHOUT_ERRORS(clientSocket);
    }
}

void tst_QOcsp::badStatus_data()
{
    QTest::addColumn<int>("responseStatus");
    QTest::addColumn<int>("certificateStatus");
    QTest::addColumn<QSslError>("expectedError");

    QTest::addRow("malformed-request") << OCSP_RESPONSE_STATUS_MALFORMEDREQUEST << 1 << QSslError(QSslError::OcspMalformedRequest);
    QTest::addRow("internal-error") << OCSP_RESPONSE_STATUS_INTERNALERROR << 2 << QSslError(QSslError::OcspInternalError);
    QTest::addRow("try-later") << OCSP_RESPONSE_STATUS_TRYLATER << 3 << QSslError(QSslError::OcspTryLater);
    QTest::addRow("signed-request-require") << OCSP_RESPONSE_STATUS_SIGREQUIRED << 2 << QSslError(QSslError::OcspSigRequred);
    QTest::addRow("unauthorized-request") << OCSP_RESPONSE_STATUS_UNAUTHORIZED << 1 <<QSslError(QSslError::OcspUnauthorized);

    QTest::addRow("certificate-revoked") << OCSP_RESPONSE_STATUS_SUCCESSFUL << V_OCSP_CERTSTATUS_REVOKED
                                         << QSslError(QSslError::CertificateRevoked);
    QTest::addRow("status-unknown") << OCSP_RESPONSE_STATUS_SUCCESSFUL << V_OCSP_CERTSTATUS_UNKNOWN
                                    << QSslError(QSslError::OcspStatusUnknown);
}

void tst_QOcsp::badStatus()
{
    // This test works with two types of 'bad' responses:
    // 1. 'Error messages' (the response's status is anything but SUCCESSFUL,
    // no information about the certificate itself, no signature);
    // 2. 'REVOKED' or 'UNKNOWN' status for a certificate in question.
    QFETCH(const int, responseStatus);
    QFETCH(const int, certificateStatus);
    QFETCH(const QSslError, expectedError);

    QDECLARE_CHAIN(subjectChain, "infbobchain.crt");
    QCOMPARE(subjectChain.size(), 2);
    QDECLARE_CHAIN(responderChain, "ca1.crt");
    QDECLARE_PRIVATE_KEY(subjPrivateKey, "infbob.key");
    QDECLARE_PRIVATE_KEY(respPrivateKey, "ca1.key");

    OcspServer server(subjectChain, subjPrivateKey);
    const OcspTimeStamp stamp(-1000, 1000);
    OcspResponder builder(stamp, subjectToChain(subjectChain), responderChain, respPrivateKey);
    const QByteArray response(builder.buildResponse(responseStatus, certificateStatus));
    QVERIFY(response.size());
    server.configureResponse(response);
    QVERIFY(server.listen());
    connect(&server, &OcspServer::internalServerError, &loop, exitLoopSlot);

    QSslSocket clientSocket;
    setupOcspClient(clientSocket, issuerToChain(subjectChain), server.peerVerifyName());
    clientSocket.connectToHostEncrypted(server.hostName(), server.serverPort());
    loop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!clientSocket.isEncrypted());
    QCOMPARE_SINGLE_ERROR(clientSocket, expectedError.error());
}

void tst_QOcsp::multipleSingleResponses()
{
    // We handle a response with more than one SingleResponse as malformed:
    const QSslError::SslError expectedError = QSslError::OcspMalformedResponse;

    // Here we use subjectChain only to generate a response, the server
    // is configured with the responder chain (it's the same cert after all).
    QDECLARE_CHAIN(subjectChain, "ss1.crt");
    QDECLARE_CHAIN(responderChain, "ss1.crt");
    QDECLARE_PRIVATE_KEY(privateKey, "ss1-private.key");

    // Let's have more than 1 certificate in a chain:
    subjectChain.append(subjectChain[0]);

    OcspServer server(responderChain, privateKey);
    // Generate a BasicOCSPResponse containing 2 SingleResponses:
    const QByteArray response(goodResponse(subjectChain, responderChain, privateKey));
    QVERIFY(response.size());
    server.configureResponse(response);
    QVERIFY(server.listen());
    connect(&server, &OcspServer::internalServerError, &loop, exitLoopSlot);

    QSslSocket clientSocket;
    setupOcspClient(clientSocket, issuerToChain(responderChain), server.peerVerifyName());
    clientSocket.connectToHostEncrypted(server.hostName(), server.serverPort());
    loop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!clientSocket.isEncrypted());
    QCOMPARE_SINGLE_ERROR(clientSocket, expectedError);
}

void tst_QOcsp::malformedResponse()
{
    QDECLARE_CHAIN(serverChain, "ss1.crt");
    QDECLARE_PRIVATE_KEY(privateKey, "ss1-private.key");

    OcspServer server(serverChain, privateKey);
    // Let's send some arbitrary bytes instead of DER and see what happens next:
    server.configureResponse("Sure, you can trust me, this cert was not revoked (I don't say it was issued at all)!");
    QVERIFY(server.listen());
    connect(&server, &OcspServer::internalServerError, &loop, exitLoopSlot);

    QSslSocket clientSocket;
    setupOcspClient(clientSocket, issuerToChain(serverChain), server.peerVerifyName());
    clientSocket.connectToHostEncrypted(server.hostName(), server.serverPort());
    loop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!clientSocket.isEncrypted());
    QCOMPARE(clientSocket.error(), QAbstractSocket::SslHandshakeFailedError);
}

void tst_QOcsp::expiredResponse_data()
{
    QTest::addColumn<long>("beforeNow");
    QTest::addColumn<long>("afterNow");

    QTest::addRow("expired") << -2000L << -1000L;
    QTest::addRow("not-valid-yet") << 5000L << 10000L;
    QTest::addRow("next-before-this") << -1000L << -2000L;
}

void tst_QOcsp::expiredResponse()
{
    // We report different kinds of problems with [thisUpdate, nextUpdate]
    // as 'expired' (to keep it simple):
    const QSslError::SslError expectedError = QSslError::OcspResponseExpired;

    QFETCH(const long, beforeNow);
    QFETCH(const long, afterNow);

    QDECLARE_CHAIN(subjectChain, "ss1.crt");
    QDECLARE_CHAIN(responderChain, "ss1.crt");
    QDECLARE_PRIVATE_KEY(privateKey, "ss1-private.key");

    OcspServer server(subjectChain, privateKey);
    const QByteArray response(goodResponse(subjectChain, responderChain, privateKey, beforeNow, afterNow));
    QVERIFY(response.size());
    server.configureResponse(response);
    QVERIFY(server.listen());
    connect(&server, &OcspServer::internalServerError, &loop, exitLoopSlot);

    QSslSocket clientSocket;
    setupOcspClient(clientSocket, issuerToChain(subjectChain), server.peerVerifyName());
    clientSocket.connectToHostEncrypted(server.hostName(), server.serverPort());
    loop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!clientSocket.isEncrypted());
    QCOMPARE_SINGLE_ERROR(clientSocket, expectedError);
}

void tst_QOcsp::noNextUpdate()
{
    // RFC2560, 2.4:
    // "If nextUpdate is not set, the responder is indicating that newer
    // revocation information is available all the time."
    //
    // This test is just to verify that we correctly handle such responses.
    QDECLARE_CHAIN(subjectChain, "ss1.crt");
    QDECLARE_CHAIN(responderChain, "ss1.crt");
    QDECLARE_PRIVATE_KEY(privateKey, "ss1-private.key");

    OcspServer server(subjectChain, privateKey);
    OcspTimeStamp openRange(-1000, 0);
    openRange.nextUpdate.clear();
    const OcspResponder responder(openRange, subjectChain, responderChain, privateKey);
    const QByteArray response(responder.buildResponse(OCSP_RESPONSE_STATUS_SUCCESSFUL,
                                                      V_OCSP_CERTSTATUS_GOOD));
    QVERIFY(response.size());
    server.configureResponse(response);
    QVERIFY(server.listen());
    connect(&server, &OcspServer::internalServerError, &loop, exitLoopSlot);

    QSslSocket clientSocket;
    setupOcspClient(clientSocket, issuerToChain(subjectChain), server.peerVerifyName());
    clientSocket.connectToHostEncrypted(server.hostName(), server.serverPort());
    loop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY_HANDSHAKE_WITHOUT_ERRORS(clientSocket);
}

void tst_QOcsp::wrongCertificateInResponse_data()
{
    QTest::addColumn<QLatin1String>("respChainName");
    QTest::addColumn<QLatin1String>("respKeyName");
    QTest::addColumn<QLatin1String>("wrongChainName");

    QTest::addRow("same-CA-wrong-subject") << QLatin1String("ca1.crt") << QLatin1String("ca1.key")
                                           << QLatin1String("alice.crt");
    QTest::addRow("wrong-CA-same-subject") << QLatin1String("ss1.crt") << QLatin1String("ss1-private.key")
                                           << QLatin1String("alice.crt");
    QTest::addRow("wrong-CA-wrong-subject") << QLatin1String("ss1.crt") << QLatin1String("ss1-private.key")
                                            << QLatin1String("ss1.crt");
}

void tst_QOcsp::wrongCertificateInResponse()
{
    QFETCH(const QLatin1String, respChainName);
    QFETCH(const QLatin1String, respKeyName);
    QFETCH(const QLatin1String, wrongChainName);
    // In this test, the server will send a valid response (correctly signed
    // by a trusted key/cert) but for a wrong certificate (not the one the
    // server presented to the client in the server's 'Certificate' message).
    const QSslError::SslError expectedError = QSslError::OcspResponseCertIdUnknown;

    QDECLARE_CHAIN(subjectChain, "infbobchain.crt");
    QDECLARE_PRIVATE_KEY(subjectKey, "infbob.key");
    QDECLARE_CHAIN(responderChain, respChainName);
    QDECLARE_PRIVATE_KEY(responderKey, respKeyName);

    QDECLARE_CHAIN(wrongChain, wrongChainName);

    OcspServer server(subjectToChain(subjectChain), subjectKey);
    const QByteArray wrongResponse(goodResponse(wrongChain, responderChain, responderKey));
    QVERIFY(wrongResponse.size());
    server.configureResponse(wrongResponse);
    QVERIFY(server.listen());
    connect(&server, &OcspServer::internalServerError, &loop, exitLoopSlot);

    QSslSocket clientSocket;
    setupOcspClient(clientSocket, issuerToChain(subjectChain), server.peerVerifyName());
    clientSocket.connectToHostEncrypted(server.hostName(), server.serverPort());
    loop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!clientSocket.isEncrypted());
    QVERIFY(containsError(clientSocket.sslHandshakeErrors(), expectedError));
}

void tst_QOcsp::untrustedResponder()
{
    const QSslError::SslError expectedError = QSslError::OcspResponseCannotBeTrusted;

    QDECLARE_CHAIN(subjectChain, "infbobchain.crt");
    QDECLARE_PRIVATE_KEY(subjectKey, "infbob.key");
    QDECLARE_CHAIN(responderChain, "ca1.crt");
    QDECLARE_PRIVATE_KEY(responderKey, "ca1.key");

    OcspServer server(subjectChain, subjectKey);
    const QByteArray response(goodResponse(subjectToChain(subjectChain), responderChain, responderKey));
    QVERIFY(response.size());
    server.configureResponse(response);
    QVERIFY(server.listen());
    connect(&server, &OcspServer::internalServerError, &loop, exitLoopSlot);

    QSslSocket clientSocket;
    setupOcspClient(clientSocket, {}, server.peerVerifyName());
    clientSocket.connectToHostEncrypted(server.hostName(), server.serverPort());
    loop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!clientSocket.isEncrypted());
    QVERIFY(containsError(clientSocket.sslHandshakeErrors(), expectedError));
}

void tst_QOcsp::setupOcspClient(QSslSocket &clientSocket, const CertificateChain &caCerts, const QString &name)
{
    QSslConfiguration clientConfig = QSslConfiguration::defaultConfiguration();
    clientConfig.setOcspStaplingEnabled(true);

    if (caCerts.size()) {
        auto roots = clientConfig.caCertificates();
        roots.append(caCerts);
        clientConfig.setCaCertificates(roots);
    }

    clientSocket.setSslConfiguration(clientConfig);
    clientSocket.setPeerVerifyName(name);

    connect(&clientSocket, &QAbstractSocket::errorOccurred, &loop, exitLoopSlot);
    connect(&clientSocket, tlsErrorsSignal, &loop, exitLoopSlot);
    connect(&clientSocket, &QSslSocket::encrypted, &loop, exitLoopSlot);
}

bool tst_QOcsp::containsOcspErrors(const QList<QSslError> &errorsFound) const
{
    for (auto code : ocspErrorCodes) {
        if (containsError(errorsFound, code))
            return true;
    }
    return false;
}

bool tst_QOcsp::containsError(const QList<QSslError> &errors, QSslError::SslError code)
{
    const auto it = std::find_if(errors.begin(), errors.end(),
                                 [&code](const QSslError &other){return other.error() == code;});
    return it != errors.end();
}

QByteArray tst_QOcsp::goodResponse(const CertificateChain &subject, const CertificateChain &responder,
                                   const QSslKey &privateKey, long beforeNow, long afterNow)
{
    const OcspResponder builder(OcspTimeStamp(beforeNow, afterNow), subject, responder, privateKey);
    return builder.buildResponse(OCSP_RESPONSE_STATUS_SUCCESSFUL, V_OCSP_CERTSTATUS_GOOD);
}

bool tst_QOcsp::loadPrivateKey(const QString &keyFileName, QSslKey &key)
{
    QFile keyFile(certDirPath + keyFileName);
    if (!keyFile.open(QIODevice::ReadOnly))
        return false;
    key = QSslKey(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    return !key.isNull();
}

CertificateChain tst_QOcsp::issuerToChain(const CertificateChain &chain)
{
    // Here we presume that, if the chain isn't a single self-signed certificate, its second
    // entry is the issuer.
    const int length = chain.size();
    Q_ASSERT(length > 0);
    return CertificateChain() << chain[length > 1 ? 1 : 0];
}

CertificateChain tst_QOcsp::subjectToChain(const CertificateChain &chain)
{
    Q_ASSERT(chain.size());
    return CertificateChain() << chain[0];
}

QT_END_NAMESPACE

QTEST_MAIN(tst_QOcsp)

#include "tst_qocsp.moc"
