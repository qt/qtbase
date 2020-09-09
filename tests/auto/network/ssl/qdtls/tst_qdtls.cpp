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

#include <QtNetwork/qsslpresharedkeyauthenticator.h>
#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qsslcipher.h>
#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qsslerror.h>
#include <QtNetwork/qsslkey.h>
#include <QtNetwork/qdtls.h>
#include <QtNetwork/qssl.h>

#include <QtCore/qcryptographichash.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvector.h>
#include <QtCore/qstring.h>
#include <QtCore/qobject.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace
{

bool dtlsErrorIsCleared(const QDtls &dtls)
{
    return dtls.dtlsError() == QDtlsError::NoError && dtls.dtlsErrorString().isEmpty();
}

using DtlsPtr = QScopedPointer<QDtls>;

bool dtlsErrorIsCleared(DtlsPtr &dtls)
{
    return dtlsErrorIsCleared(*dtls);
}

} // unnamed namespace

#define QDTLS_VERIFY_NO_ERROR(obj) QVERIFY(dtlsErrorIsCleared(obj))

#define QDTLS_VERIFY_HANDSHAKE_SUCCESS(obj) \
    QVERIFY(obj->isConnectionEncrypted()); \
    QCOMPARE(obj->handshakeState(), QDtls::HandshakeComplete); \
    QDTLS_VERIFY_NO_ERROR(obj); \
    QCOMPARE(obj->peerVerificationErrors().size(), 0)

class tst_QDtls : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void init();

private slots:
    // Tests:
    void construction_data();
    void construction();
    void configuration_data();
    void configuration();
    void invalidConfiguration();
    void setPeer_data();
    void setPeer();
    void handshake_data();
    void handshake();
    void handshakeWithRetransmission();
    void sessionCipher();
    void cipherPreferences_data();
    void cipherPreferences();
    void protocolVersionMatching_data();
    void protocolVersionMatching();
    void verificationErrors_data();
    void verificationErrors();
    void presetExpectedErrors_data();
    void presetExpectedErrors();
    void verifyServerCertificate_data();
    void verifyServerCertificate();
    void verifyClientCertificate_data();
    void verifyClientCertificate();
    void blacklistedCerificate();
    void readWriteEncrypted_data();
    void readWriteEncrypted();
    void datagramFragmentation();

protected slots:
    void handshakeReadyRead();
    void encryptedReadyRead();
    void pskRequested(QSslPreSharedKeyAuthenticator *auth);
    void handleHandshakeTimeout();

private:
    void clientServerData();
    void connectHandshakeReadingSlots();
    void connectEncryptedReadingSlots();
    bool verificationErrorDetected(QSslError::SslError code) const;

    static QHostAddress toNonAny(const QHostAddress &addr);

    QUdpSocket serverSocket;
    QHostAddress serverAddress;
    quint16 serverPort = 0;
    QSslConfiguration defaultServerConfig;
    QSslCertificate selfSignedCert;
    QString hostName;
    QSslKey serverKeySS;
    bool serverDropDgram = false;
    const QByteArray serverExpectedPlainText = "Hello W ... hmm, I mean DTLS server!";
    QByteArray serverReceivedPlainText;

    QUdpSocket clientSocket;
    QHostAddress clientAddress;
    quint16 clientPort = 0;
    bool clientDropDgram = false;
    const QByteArray clientExpectedPlainText = "Hello DTLS client.";
    QByteArray clientReceivedPlainText;

    DtlsPtr serverCrypto;
    DtlsPtr clientCrypto;

    QTestEventLoop testLoop;
    const int handshakeTimeoutMS = 5000;
    const int dataExchangeTimeoutMS = 1000;

    const QByteArray presharedKey = "DEADBEEFDEADBEEF";
    QString certDirPath;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSsl::SslProtocol)
Q_DECLARE_METATYPE(QSslSocket::SslMode)
Q_DECLARE_METATYPE(QSslSocket::PeerVerifyMode)
Q_DECLARE_METATYPE(QList<QSslCertificate>)
Q_DECLARE_METATYPE(QSslKey)
Q_DECLARE_METATYPE(QVector<QSslError>)

QT_BEGIN_NAMESPACE

void tst_QDtls::initTestCase()
{
    certDirPath = QFileInfo(QFINDTESTDATA("certs")).absolutePath();
    QVERIFY(certDirPath.size() > 0);
    certDirPath += QDir::separator() + QStringLiteral("certs") + QDir::separator();

    QVERIFY(QSslSocket::supportsSsl());

    QFile keyFile(certDirPath + QStringLiteral("ss-srv-key.pem"));
    QVERIFY(keyFile.open(QIODevice::ReadOnly));
    serverKeySS = QSslKey(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, "foobar");
    QVERIFY(!serverKeySS.isNull());

    QList<QSslCertificate> certificates = QSslCertificate::fromPath(certDirPath + QStringLiteral("ss-srv-cert.pem"));
    QVERIFY(!certificates.isEmpty());
    QVERIFY(!certificates.first().isNull());
    selfSignedCert = certificates.first();

    defaultServerConfig = QSslConfiguration::defaultDtlsConfiguration();
    defaultServerConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    defaultServerConfig.setDtlsCookieVerificationEnabled(false);

    hostName = QStringLiteral("bob.org");

    void qt_ForceTlsSecurityLevel();
    qt_ForceTlsSecurityLevel();
}

void tst_QDtls::init()
{
    if (serverSocket.state() != QAbstractSocket::UnconnectedState) {
        serverSocket.close();
        // disconnect signals/slots:
        serverSocket.disconnect();
    }

    QVERIFY(serverSocket.bind());
    serverAddress = toNonAny(serverSocket.localAddress());
    serverPort = serverSocket.localPort();

    if (clientSocket.localPort()) {
        clientSocket.close();
        // disconnect signals/slots:
        clientSocket.disconnect();
    }

    clientAddress = {};
    clientPort = 0;

    serverCrypto.reset(new QDtls(QSslSocket::SslServerMode));
    serverDropDgram = false;
    serverReceivedPlainText.clear();

    clientCrypto.reset(new QDtls(QSslSocket::SslClientMode));
    clientDropDgram = false;
    clientReceivedPlainText.clear();

    connect(clientCrypto.data(), &QDtls::handshakeTimeout,
            this, &tst_QDtls::handleHandshakeTimeout);
    connect(serverCrypto.data(), &QDtls::handshakeTimeout,
            this, &tst_QDtls::handleHandshakeTimeout);
}

void tst_QDtls::construction_data()
{
    clientServerData();
}

void tst_QDtls::construction()
{
    QFETCH(const QSslSocket::SslMode, mode);

    QDtls dtls(mode);
    QCOMPARE(dtls.peerAddress(), QHostAddress());
    QCOMPARE(dtls.peerPort(), quint16());
    QCOMPARE(dtls.peerVerificationName(), QString());
    QCOMPARE(dtls.sslMode(), mode);

    QCOMPARE(dtls.mtuHint(), quint16());

    const auto params = dtls.cookieGeneratorParameters();
    QVERIFY(params.secret.size() > 0);
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
    QCOMPARE(params.hash, QCryptographicHash::Sha1);
#else
    QCOMPARE(params.hash, QCryptographicHash::Sha256);
#endif

    QCOMPARE(dtls.dtlsConfiguration(), QSslConfiguration::defaultDtlsConfiguration());

    QCOMPARE(dtls.handshakeState(), QDtls::HandshakeNotStarted);
    QCOMPARE(dtls.isConnectionEncrypted(), false);
    QCOMPARE(dtls.sessionCipher(), QSslCipher());
    QCOMPARE(dtls.sessionProtocol(), QSsl::UnknownProtocol);

    QCOMPARE(dtls.dtlsError(), QDtlsError::NoError);
    QCOMPARE(dtls.dtlsErrorString(), QString());
    QCOMPARE(dtls.peerVerificationErrors().size(), 0);
}

void tst_QDtls::configuration_data()
{
    clientServerData();
}

void tst_QDtls::configuration()
{
    // There is a proper auto-test for QSslConfiguration in our TLS test suite,
    // here we only test several DTLS-related details.
    auto config = QSslConfiguration::defaultDtlsConfiguration();
    QCOMPARE(config.protocol(), QSsl::DtlsV1_2OrLater);

    const QList<QSslCipher> ciphers = config.ciphers();
    QVERIFY(ciphers.size() > 0);
    for (const auto &cipher : ciphers)
        QVERIFY(cipher.usedBits() >= 128);

    QCOMPARE(config.dtlsCookieVerificationEnabled(), true);

    QFETCH(const QSslSocket::SslMode, mode);
    QDtls dtls(mode);
    QCOMPARE(dtls.dtlsConfiguration(), config);
    config.setProtocol(QSsl::DtlsV1_0OrLater);
    config.setDtlsCookieVerificationEnabled(false);
    QCOMPARE(config.dtlsCookieVerificationEnabled(), false);

    QVERIFY(dtls.setDtlsConfiguration(config));
    QDTLS_VERIFY_NO_ERROR(dtls);
    QCOMPARE(dtls.dtlsConfiguration(), config);

    if (mode == QSslSocket::SslClientMode) {
        // Testing a DTLS server would be more complicated, we'd need a DTLS
        // client sending ClientHello(s), running an event loop etc. - way too
        // much dancing for a simple  setter/getter test.
        QVERIFY(dtls.setPeer(serverAddress, serverPort));
        QDTLS_VERIFY_NO_ERROR(dtls);

        QUdpSocket clientSocket;
        QVERIFY(dtls.doHandshake(&clientSocket));
        QDTLS_VERIFY_NO_ERROR(dtls);
        QCOMPARE(dtls.handshakeState(), QDtls::HandshakeInProgress);
        // As soon as handshake started, it's not allowed to change configuration:
        QVERIFY(!dtls.setDtlsConfiguration(QSslConfiguration::defaultDtlsConfiguration()));
        QCOMPARE(dtls.dtlsError(), QDtlsError::InvalidOperation);
        QCOMPARE(dtls.dtlsConfiguration(), config);
    }
}

void tst_QDtls::invalidConfiguration()
{
    QUdpSocket socket;
    QDtls crypto(QSslSocket::SslClientMode);
    QVERIFY(crypto.setPeer(serverAddress, serverPort));
    // Note: not defaultDtlsConfiguration(), so the protocol is TLS (without D):
    QVERIFY(crypto.setDtlsConfiguration(QSslConfiguration::defaultConfiguration()));
    QDTLS_VERIFY_NO_ERROR(crypto);
    QCOMPARE(crypto.dtlsConfiguration(), QSslConfiguration::defaultConfiguration());
    // Try to start the handshake:
    QCOMPARE(crypto.doHandshake(&socket), false);
    QCOMPARE(crypto.dtlsError(), QDtlsError::TlsInitializationError);
}

void tst_QDtls::setPeer_data()
{
    clientServerData();
}

void tst_QDtls::setPeer()
{
    static const QHostAddress invalid[] = {QHostAddress(),
                                           QHostAddress(QHostAddress::Broadcast),
                                           QHostAddress(QStringLiteral("224.0.0.0"))};
    static const QString peerName = QStringLiteral("does not matter actually");

    QFETCH(const QSslSocket::SslMode, mode);
    QDtls dtls(mode);

    for (const auto &addr : invalid) {
        QCOMPARE(dtls.setPeer(addr, 100, peerName), false);
        QCOMPARE(dtls.dtlsError(), QDtlsError::InvalidInputParameters);
        QCOMPARE(dtls.peerAddress(), QHostAddress());
        QCOMPARE(dtls.peerPort(), quint16());
        QCOMPARE(dtls.peerVerificationName(), QString());
    }

    QVERIFY(dtls.setPeer(serverAddress, serverPort, peerName));
    QDTLS_VERIFY_NO_ERROR(dtls);
    QCOMPARE(dtls.peerAddress(), serverAddress);
    QCOMPARE(dtls.peerPort(), serverPort);
    QCOMPARE(dtls.peerVerificationName(), peerName);

    if (mode == QSslSocket::SslClientMode) {
        // We test for client mode only, for server mode we'd have to run event
        // loop etc. too much work for a simple setter/getter test.
        QUdpSocket clientSocket;
        QVERIFY(dtls.doHandshake(&clientSocket));
        QDTLS_VERIFY_NO_ERROR(dtls);
        QCOMPARE(dtls.handshakeState(), QDtls::HandshakeInProgress);
        QCOMPARE(dtls.setPeer(serverAddress, serverPort), false);
        QCOMPARE(dtls.dtlsError(), QDtlsError::InvalidOperation);
    }
}

void tst_QDtls::handshake_data()
{
    QTest::addColumn<bool>("withCertificate");

    QTest::addRow("no-cert") << false;
    QTest::addRow("with-cert") << true;
}

void tst_QDtls::handshake()
{
    connectHandshakeReadingSlots();

    QFETCH(const bool, withCertificate);

    auto serverConfig = defaultServerConfig;
    auto clientConfig = QSslConfiguration::defaultDtlsConfiguration();

    if (!withCertificate) {
        connect(serverCrypto.data(), &QDtls::pskRequired, this, &tst_QDtls::pskRequested);
        connect(clientCrypto.data(), &QDtls::pskRequired, this, &tst_QDtls::pskRequested);
        clientConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        QVERIFY(clientConfig.peerCertificate().isNull());
    } else {
        serverConfig.setPrivateKey(serverKeySS);
        serverConfig.setLocalCertificate(selfSignedCert);
        clientConfig.setCaCertificates({selfSignedCert});
    }

    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));
    QVERIFY(clientCrypto->setDtlsConfiguration(clientConfig));

    // Some early checks before we run event loop.
    // Remote was not set yet:
    QVERIFY(!clientCrypto->doHandshake(&clientSocket));
    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidOperation);
    QVERIFY(!serverCrypto->doHandshake(&serverSocket, QByteArray("ClientHello")));
    QCOMPARE(serverCrypto->dtlsError(), QDtlsError::InvalidOperation);

    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort, hostName));

    // Invalid socket:
    QVERIFY(!clientCrypto->doHandshake(nullptr));
    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidInputParameters);

    // Now we are ready for handshake:
    QVERIFY(clientCrypto->doHandshake(&clientSocket));
    QDTLS_VERIFY_NO_ERROR(clientCrypto);
    QCOMPARE(clientCrypto->handshakeState(), QDtls::HandshakeInProgress);

    testLoop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!testLoop.timeout());

    QVERIFY(serverCrypto->isConnectionEncrypted());
    QDTLS_VERIFY_NO_ERROR(serverCrypto);
    QCOMPARE(serverCrypto->handshakeState(), QDtls::HandshakeComplete);
    QCOMPARE(serverCrypto->peerVerificationErrors().size(), 0);

    QVERIFY(clientCrypto->isConnectionEncrypted());
    QDTLS_VERIFY_NO_ERROR(clientCrypto);
    QCOMPARE(clientCrypto->handshakeState(), QDtls::HandshakeComplete);
    QCOMPARE(clientCrypto->peerVerificationErrors().size(), 0);

    if (withCertificate) {
        const auto serverCert = clientCrypto->dtlsConfiguration().peerCertificate();
        QVERIFY(!serverCert.isNull());
        QCOMPARE(serverCert, selfSignedCert);
    }

    // Already in 'HandshakeComplete' state/encrypted.
    QVERIFY(!clientCrypto->doHandshake(&clientSocket));
    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidOperation);
    QVERIFY(!serverCrypto->doHandshake(&serverSocket, {"ServerHello"}));
    QCOMPARE(serverCrypto->dtlsError(), QDtlsError::InvalidOperation);
    // Cannot change a remote without calling shutdown first.
    QVERIFY(!clientCrypto->setPeer(serverAddress, serverPort));
    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidOperation);
    QVERIFY(!serverCrypto->setPeer(clientAddress, clientPort));
    QCOMPARE(serverCrypto->dtlsError(), QDtlsError::InvalidOperation);
}

void tst_QDtls::handshakeWithRetransmission()
{
    connectHandshakeReadingSlots();

    auto serverConfig = defaultServerConfig;
    serverConfig.setPrivateKey(serverKeySS);
    serverConfig.setLocalCertificate(selfSignedCert);
    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));

    auto clientConfig = QSslConfiguration::defaultDtlsConfiguration();
    clientConfig.setCaCertificates({selfSignedCert});
    QVERIFY(clientCrypto->setDtlsConfiguration(clientConfig));
    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort, hostName));

    // Now we are ready for handshake:
    QVERIFY(clientCrypto->doHandshake(&clientSocket));
    QDTLS_VERIFY_NO_ERROR(clientCrypto);
    QCOMPARE(clientCrypto->handshakeState(), QDtls::HandshakeInProgress);

    serverDropDgram = true;
    clientDropDgram = true;
    // Every failed re-transmission doubles the next timeout. We don't want to
    // slow down the test just to check the re-transmission ability, so we'll
    // drop only the first 'ClientHello' and 'ServerHello' datagrams. The
    // arithmetic is approximately this: the first ClientHello to be dropped -
    // client will re-transmit in 1s., the first part of 'ServerHello' to be
    // dropped, the client then will re-transmit after another 2 s. Thus it's ~3.
    // We err on safe side and double our (already quite generous) 5s.
    testLoop.enterLoopMSecs(handshakeTimeoutMS * 2);

    QVERIFY(!testLoop.timeout());
    QDTLS_VERIFY_HANDSHAKE_SUCCESS(serverCrypto);
    QDTLS_VERIFY_HANDSHAKE_SUCCESS(clientCrypto);
}

void tst_QDtls::sessionCipher()
{
    connectHandshakeReadingSlots();

    auto serverConfig = defaultServerConfig;
    serverConfig.setPrivateKey(serverKeySS);
    serverConfig.setLocalCertificate(selfSignedCert);
    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));

    auto clientConfig = QSslConfiguration::defaultDtlsConfiguration();
    clientConfig.setCaCertificates({selfSignedCert});
    QVERIFY(clientCrypto->setDtlsConfiguration(clientConfig));

    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort, hostName));
    QVERIFY(clientCrypto->doHandshake(&clientSocket));

    testLoop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!testLoop.timeout());
    QDTLS_VERIFY_HANDSHAKE_SUCCESS(clientCrypto);
    QDTLS_VERIFY_HANDSHAKE_SUCCESS(serverCrypto);

    const auto defaultDtlsConfig = QSslConfiguration::defaultDtlsConfiguration();

    const auto clCipher = clientCrypto->sessionCipher();
    QVERIFY(!clCipher.isNull());
    QVERIFY(defaultDtlsConfig.ciphers().contains(clCipher));

    const auto srvCipher = serverCrypto->sessionCipher();
    QVERIFY(!srvCipher.isNull());
    QVERIFY(defaultDtlsConfig.ciphers().contains(srvCipher));

    QCOMPARE(clCipher, srvCipher);
}

void tst_QDtls::cipherPreferences_data()
{
    QTest::addColumn<bool>("preferClient");

    QTest::addRow("prefer-server") << true;
    QTest::addRow("prefer-client") << false;
}

void tst_QDtls::cipherPreferences()
{
    // This test is based on the similar case in tst_QSslSocket. We test it for QDtls
    // because it's possible to set ciphers and corresponding ('server preferred')
    // options via QSslConfiguration.
    const QSslCipher aes128(QStringLiteral("AES128-SHA"));
    const QSslCipher aes256(QStringLiteral("AES256-SHA"));

    auto serverConfig = defaultServerConfig;
    const QList<QSslCipher> ciphers = serverConfig.ciphers();
    if (!ciphers.contains(aes128) || !ciphers.contains(aes256))
        QSKIP("The ciphers needed by this test were not found in the default DTLS configuration");

    serverConfig.setCiphers({aes128, aes256});
    serverConfig.setLocalCertificate(selfSignedCert);
    serverConfig.setPrivateKey(serverKeySS);

    QFETCH(const bool, preferClient);
    if (preferClient)
        serverConfig.setSslOption(QSsl::SslOptionDisableServerCipherPreference, true);

    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));
    QDTLS_VERIFY_NO_ERROR(serverCrypto);

    auto clientConfig = QSslConfiguration::defaultDtlsConfiguration();
    clientConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    clientConfig.setCiphers({aes256, aes128});
    QVERIFY(clientCrypto->setDtlsConfiguration(clientConfig));
    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort));
    QDTLS_VERIFY_NO_ERROR(clientCrypto);

    connectHandshakeReadingSlots();

    QVERIFY(clientCrypto->doHandshake(&clientSocket));
    QDTLS_VERIFY_NO_ERROR(clientCrypto);

    testLoop.enterLoopMSecs(handshakeTimeoutMS);
    QVERIFY(!testLoop.timeout());
    QDTLS_VERIFY_HANDSHAKE_SUCCESS(clientCrypto);
    QDTLS_VERIFY_HANDSHAKE_SUCCESS(serverCrypto);

    if (preferClient) {
        QCOMPARE(clientCrypto->sessionCipher(), aes256);
        QCOMPARE(serverCrypto->sessionCipher(), aes256);
    } else {
        QCOMPARE(clientCrypto->sessionCipher(), aes128);
        QCOMPARE(serverCrypto->sessionCipher(), aes128);
    }
}

void tst_QDtls::protocolVersionMatching_data()
{
    QTest::addColumn<QSsl::SslProtocol>("serverProtocol");
    QTest::addColumn<QSsl::SslProtocol>("clientProtocol");
    QTest::addColumn<bool>("works");

    QTest::addRow("DtlsV1_0 <-> DtlsV1_0") << QSsl::DtlsV1_0 << QSsl::DtlsV1_0 << true;
    QTest::addRow("DtlsV1_0OrLater <-> DtlsV1_0") << QSsl::DtlsV1_0OrLater << QSsl::DtlsV1_0 << true;
    QTest::addRow("DtlsV1_0 <-> DtlsV1_0OrLater") << QSsl::DtlsV1_0 << QSsl::DtlsV1_0OrLater << true;
    QTest::addRow("DtlsV1_0OrLater <-> DtlsV1_0OrLater") << QSsl::DtlsV1_0OrLater << QSsl::DtlsV1_0OrLater << true;

    QTest::addRow("DtlsV1_2 <-> DtlsV1_2") << QSsl::DtlsV1_2 << QSsl::DtlsV1_2 << true;
    QTest::addRow("DtlsV1_2OrLater <-> DtlsV1_2") << QSsl::DtlsV1_2OrLater << QSsl::DtlsV1_2 << true;
    QTest::addRow("DtlsV1_2 <-> DtlsV1_2OrLater") << QSsl::DtlsV1_2 << QSsl::DtlsV1_2OrLater << true;
    QTest::addRow("DtlsV1_2OrLater <-> DtlsV1_2OrLater") << QSsl::DtlsV1_2OrLater << QSsl::DtlsV1_2OrLater << true;

    QTest::addRow("DtlsV1_0 <-> DtlsV1_2") << QSsl::DtlsV1_0 << QSsl::DtlsV1_2 << false;
    QTest::addRow("DtlsV1_0 <-> DtlsV1_2OrLater") << QSsl::DtlsV1_0 << QSsl::DtlsV1_2OrLater << false;
    QTest::addRow("DtlsV1_2 <-> DtlsV1_0") << QSsl::DtlsV1_2 << QSsl::DtlsV1_0 << false;
    QTest::addRow("DtlsV1_2OrLater <-> DtlsV1_0") << QSsl::DtlsV1_2OrLater << QSsl::DtlsV1_0 << false;
}

void tst_QDtls::protocolVersionMatching()
{
    QFETCH(const QSsl::SslProtocol, serverProtocol);
    QFETCH(const QSsl::SslProtocol, clientProtocol);
    QFETCH(const bool, works);

    connectHandshakeReadingSlots();

    connect(serverCrypto.data(), &QDtls::pskRequired, this, &tst_QDtls::pskRequested);
    connect(clientCrypto.data(), &QDtls::pskRequired, this, &tst_QDtls::pskRequested);

    auto serverConfig = defaultServerConfig;
    serverConfig.setProtocol(serverProtocol);
    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));

    auto clientConfig = QSslConfiguration::defaultDtlsConfiguration();
    clientConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    clientConfig.setProtocol(clientProtocol);
    QVERIFY(clientCrypto->setDtlsConfiguration(clientConfig));

    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort));
    QVERIFY(clientCrypto->doHandshake(&clientSocket));

    testLoop.enterLoopMSecs(handshakeTimeoutMS);

    if (works) {
        QDTLS_VERIFY_HANDSHAKE_SUCCESS(serverCrypto);
        QDTLS_VERIFY_HANDSHAKE_SUCCESS(clientCrypto);
    } else {
        QCOMPARE(serverCrypto->isConnectionEncrypted(), false);
        QVERIFY(serverCrypto->handshakeState() != QDtls::HandshakeComplete);
        QCOMPARE(clientCrypto->isConnectionEncrypted(), false);
        QVERIFY(clientCrypto->handshakeState() != QDtls::HandshakeComplete);
    }
}

void tst_QDtls::verificationErrors_data()
{
    QTest::addColumn<bool>("abortHandshake");

    QTest::addRow("abort-handshake") << true;
    QTest::addRow("ignore-errors") << false;
}

void tst_QDtls::verificationErrors()
{
    connectHandshakeReadingSlots();

    auto serverConfig = defaultServerConfig;
    serverConfig.setPrivateKey(serverKeySS);
    serverConfig.setLocalCertificate(selfSignedCert);
    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));
    // And our client already has the default DTLS configuration.

    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort));
    // Now we are ready for handshake:
    QVERIFY(clientCrypto->doHandshake(&clientSocket));

    testLoop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!testLoop.timeout());
    QDTLS_VERIFY_NO_ERROR(serverCrypto);

    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::PeerVerificationError);
    QCOMPARE(clientCrypto->handshakeState(), QDtls::PeerVerificationFailed);
    QVERIFY(!clientCrypto->isConnectionEncrypted());

    QVERIFY(verificationErrorDetected(QSslError::HostNameMismatch));
    QVERIFY(verificationErrorDetected(QSslError::SelfSignedCertificate));

    const auto serverCert = clientCrypto->dtlsConfiguration().peerCertificate();
    QVERIFY(!serverCert.isNull());
    QCOMPARE(selfSignedCert, serverCert);

    QFETCH(const bool, abortHandshake);

    if (abortHandshake) {
        QVERIFY(!clientCrypto->abortHandshake(nullptr));
        QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidInputParameters);
        QVERIFY(clientCrypto->abortHandshake(&clientSocket));
        QDTLS_VERIFY_NO_ERROR(clientCrypto);
        QVERIFY(!clientCrypto->isConnectionEncrypted());
        QCOMPARE(clientCrypto->handshakeState(), QDtls::HandshakeNotStarted);
        QCOMPARE(clientCrypto->sessionCipher(), QSslCipher());
        QCOMPARE(clientCrypto->sessionProtocol(), QSsl::UnknownProtocol);
        const auto config = clientCrypto->dtlsConfiguration();
        QVERIFY(config.peerCertificate().isNull());
        QCOMPARE(config.peerCertificateChain().size(), 0);
        QCOMPARE(clientCrypto->peerVerificationErrors().size(), 0);
    } else {
        clientCrypto->ignoreVerificationErrors(clientCrypto->peerVerificationErrors());
        QVERIFY(!clientCrypto->resumeHandshake(nullptr));
        QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidInputParameters);
        QVERIFY(clientCrypto->resumeHandshake(&clientSocket));
        QDTLS_VERIFY_HANDSHAKE_SUCCESS(clientCrypto);
        QVERIFY(clientCrypto->isConnectionEncrypted());
        QCOMPARE(clientCrypto->handshakeState(), QDtls::HandshakeComplete);
        QCOMPARE(clientCrypto->peerVerificationErrors().size(), 0);
    }
}

void tst_QDtls::presetExpectedErrors_data()
{
    QTest::addColumn<QVector<QSslError>>("expectedTlsErrors");
    QTest::addColumn<bool>("works");

    QVector<QSslError> expectedErrors{{QSslError::HostNameMismatch, selfSignedCert}};
    QTest::addRow("unexpected-self-signed") << expectedErrors << false;
    expectedErrors.push_back({QSslError::SelfSignedCertificate, selfSignedCert});
    QTest::addRow("all-errors-ignored") << expectedErrors << true;
}

void tst_QDtls::presetExpectedErrors()
{
    QFETCH(const QVector<QSslError>, expectedTlsErrors);
    QFETCH(const bool, works);

    connectHandshakeReadingSlots();

    auto serverConfig = defaultServerConfig;
    serverConfig.setPrivateKey(serverKeySS);
    serverConfig.setLocalCertificate(selfSignedCert);
    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));

    clientCrypto->ignoreVerificationErrors(expectedTlsErrors);
    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort));
    QVERIFY(clientCrypto->doHandshake(&clientSocket));

    testLoop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!testLoop.timeout());

    if (works) {
        QDTLS_VERIFY_HANDSHAKE_SUCCESS(serverCrypto);
        QCOMPARE(clientCrypto->handshakeState(), QDtls::HandshakeComplete);
        QVERIFY(clientCrypto->isConnectionEncrypted());
    } else {
        QCOMPARE(clientCrypto->dtlsError(), QDtlsError::PeerVerificationError);
        QVERIFY(!clientCrypto->isConnectionEncrypted());
        QCOMPARE(clientCrypto->handshakeState(), QDtls::PeerVerificationFailed);
    }
}

void tst_QDtls::verifyServerCertificate_data()
{
    QTest::addColumn<QSslSocket::PeerVerifyMode>("verifyMode");
    QTest::addColumn<QList<QSslCertificate>>("serverCerts");
    QTest::addColumn<QSslKey>("serverKey");
    QTest::addColumn<QString>("peerName");
    QTest::addColumn<bool>("encrypted");

    {
        // A special case - null key (but with certificate):
        const auto chain = QSslCertificate::fromPath(certDirPath + QStringLiteral("bogus-server.crt"));
        QCOMPARE(chain.size(), 1);

        QSslKey nullKey;
        // Only one row - server must fail to start handshake immediately.
        QTest::newRow("valid-server-cert-no-key : VerifyPeer") << QSslSocket::VerifyPeer << chain << nullKey << QString() << false;
    }
    {
        // Valid certificate:
        auto chain = QSslCertificate::fromPath(certDirPath + QStringLiteral("bogus-server.crt"));
        QCOMPARE(chain.size(), 1);

        const auto caCert = QSslCertificate::fromPath(certDirPath + QStringLiteral("bogus-ca.crt"));
        QCOMPARE(caCert.size(), 1);
        chain += caCert;

        QFile keyFile(certDirPath + QStringLiteral("bogus-server.key"));
        QVERIFY(keyFile.open(QIODevice::ReadOnly));
        const QSslKey key(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull());

        auto cert = chain.first();
        const QString name(cert.subjectInfo(QSslCertificate::CommonName).first());
        QTest::newRow("valid-server-cert : AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << chain << key << name << true;
        QTest::newRow("valid-server-cert : QueryPeer") << QSslSocket::QueryPeer << chain << key << name << true;
        QTest::newRow("valid-server-cert : VerifyNone") << QSslSocket::VerifyNone << chain << key << name << true;
        QTest::newRow("valid-server-cert : VerifyPeer (add CA)") << QSslSocket::VerifyPeer << chain << key << name << true;
        QTest::newRow("valid-server-cert : VerifyPeer (no CA)") << QSslSocket::VerifyPeer << chain << key << name << false;
        QTest::newRow("valid-server-cert : VerifyPeer (name mismatch)") << QSslSocket::VerifyPeer << chain << key << QString() << false;
    }
}

void tst_QDtls::verifyServerCertificate()
{
    QFETCH(const QSslSocket::PeerVerifyMode, verifyMode);
    QFETCH(const QList<QSslCertificate>, serverCerts);
    QFETCH(const QSslKey, serverKey);
    QFETCH(const QString, peerName);
    QFETCH(const bool, encrypted);

    auto serverConfig = defaultServerConfig;
    serverConfig.setLocalCertificateChain(serverCerts);
    serverConfig.setPrivateKey(serverKey);
    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));

    auto clientConfig = QSslConfiguration::defaultDtlsConfiguration();

    if (serverCerts.size() == 2 && encrypted) {
        auto caCerts = clientConfig.caCertificates();
        caCerts.append(serverCerts.at(1));
        clientConfig.setCaCertificates(caCerts);
    }

    clientConfig.setPeerVerifyMode(verifyMode);

    QVERIFY(clientCrypto->setDtlsConfiguration(clientConfig));
    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort, peerName));

    connectHandshakeReadingSlots();

    QVERIFY(clientCrypto->doHandshake(&clientSocket));

    testLoop.enterLoopMSecs(handshakeTimeoutMS);
    QVERIFY(!testLoop.timeout());

    if (serverKey.isNull() && !serverCerts.isEmpty()) {
        QDTLS_VERIFY_NO_ERROR(clientCrypto);
        QCOMPARE(clientCrypto->handshakeState(), QDtls::HandshakeInProgress);
        QCOMPARE(serverCrypto->dtlsError(), QDtlsError::TlsInitializationError);
        QCOMPARE(serverCrypto->handshakeState(), QDtls::HandshakeNotStarted);
        return;
    }

    if (encrypted) {
        QDTLS_VERIFY_HANDSHAKE_SUCCESS(serverCrypto);
        QDTLS_VERIFY_HANDSHAKE_SUCCESS(clientCrypto);
    } else {
        QVERIFY(!clientCrypto->isConnectionEncrypted());
        QCOMPARE(clientCrypto->handshakeState(), QDtls::PeerVerificationFailed);
        QVERIFY(clientCrypto->peerVerificationErrors().size());
        QVERIFY(clientCrypto->writeDatagramEncrypted(&clientSocket, "something") < 0);
        QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidOperation);
    }
}

void tst_QDtls::verifyClientCertificate_data()
{
    QTest::addColumn<QSslSocket::PeerVerifyMode>("verifyMode");
    QTest::addColumn<QList<QSslCertificate>>("clientCerts");
    QTest::addColumn<QSslKey>("clientKey");
    QTest::addColumn<bool>("encrypted");
    {
        // No certficates, no key:
        QList<QSslCertificate> chain;
        QSslKey key;
        QTest::newRow("no-cert : AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << chain << key << true;
        QTest::newRow("no-cert : QueryPeer") << QSslSocket::QueryPeer << chain << key << true;
        QTest::newRow("no-cert : VerifyNone") << QSslSocket::VerifyNone << chain << key << true;
        QTest::newRow("no-cert : VerifyPeer") << QSslSocket::VerifyPeer << chain << key << false;
    }
    {
        const auto chain = QSslCertificate::fromPath(certDirPath + QStringLiteral("fluke.cert"));
        QCOMPARE(chain.size(), 1);

        QFile keyFile(certDirPath + QStringLiteral("fluke.key"));
        QVERIFY(keyFile.open(QIODevice::ReadOnly));
        const QSslKey key(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull());

        QTest::newRow("self-signed-cert : AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << chain << key << true;
        QTest::newRow("self-signed-cert : QueryPeer") << QSslSocket::QueryPeer << chain << key << true;
        QTest::newRow("self-signed-cert : VerifyNone") << QSslSocket::VerifyNone << chain << key << true;
        QTest::newRow("self-signed-cert : VerifyPeer") << QSslSocket::VerifyPeer << chain << key << false;
    }
    {
        // Valid certificate, but wrong usage (server certificate):
        const auto chain = QSslCertificate::fromPath(certDirPath + QStringLiteral("bogus-server.crt"));
        QCOMPARE(chain.size(), 1);

        QFile keyFile(certDirPath + QStringLiteral("bogus-server.key"));
        QVERIFY(keyFile.open(QIODevice::ReadOnly));
        const QSslKey key(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull());

        QTest::newRow("valid-server-cert : AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << chain << key << true;
        QTest::newRow("valid-server-cert : QueryPeer") << QSslSocket::QueryPeer << chain << key << true;
        QTest::newRow("valid-server-cert : VerifyNone") << QSslSocket::VerifyNone << chain << key << true;
        QTest::newRow("valid-server-cert : VerifyPeer") << QSslSocket::VerifyPeer << chain << key << false;
    }
    {
        // Valid certificate, correct usage (client certificate):
        auto chain = QSslCertificate::fromPath(certDirPath + QStringLiteral("bogus-client.crt"));
        QCOMPARE(chain.size(), 1);

        QFile keyFile(certDirPath + QStringLiteral("bogus-client.key"));
        QVERIFY(keyFile.open(QIODevice::ReadOnly));
        const QSslKey key(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull());

        QTest::newRow("valid-client-cert : AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << chain << key << true;
        QTest::newRow("valid-client-cert : QueryPeer") << QSslSocket::QueryPeer << chain << key << true;
        QTest::newRow("valid-client-cert : VerifyNone") << QSslSocket::VerifyNone << chain << key << true;
        QTest::newRow("valid-client-cert : VerifyPeer") << QSslSocket::VerifyPeer << chain << key << true;

        // Valid certificate, correct usage (client certificate), with chain:
        chain += QSslCertificate::fromPath(certDirPath + QStringLiteral("bogus-ca.crt"));
        QCOMPARE(chain.size(), 2);

        QTest::newRow("valid-client-chain : AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << chain << key << true;
        QTest::newRow("valid-client-chain : QueryPeer") << QSslSocket::QueryPeer << chain << key << true;
        QTest::newRow("valid-client-chain : VerifyNone") << QSslSocket::VerifyNone << chain << key << true;
        QTest::newRow("valid-client-chain : VerifyPeer") << QSslSocket::VerifyPeer << chain << key << true;
    }
}

void tst_QDtls::verifyClientCertificate()
{
    connectHandshakeReadingSlots();

    QFETCH(const QSslSocket::PeerVerifyMode, verifyMode);
    QFETCH(const QList<QSslCertificate>, clientCerts);
    QFETCH(const QSslKey, clientKey);
    QFETCH(const bool, encrypted);

    QSslConfiguration serverConfig = defaultServerConfig;
    serverConfig.setLocalCertificate(selfSignedCert);
    serverConfig.setPrivateKey(serverKeySS);
    serverConfig.setPeerVerifyMode(verifyMode);

    if (verifyMode == QSslSocket::VerifyPeer && clientCerts.size()) {
        // Not always needed even if these conditions met, but does not hurt
        // either.
        const auto certs = QSslCertificate::fromPath(certDirPath + QStringLiteral("bogus-ca.crt"));
        QCOMPARE(certs.size(), 1);
        serverConfig.setCaCertificates(serverConfig.caCertificates() + certs);
    }

    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));
    serverConfig = serverCrypto->dtlsConfiguration();
    QVERIFY(serverConfig.peerCertificate().isNull());
    QCOMPARE(serverConfig.peerCertificateChain().size(), 0);

    auto clientConfig = QSslConfiguration::defaultDtlsConfiguration();
    clientConfig.setLocalCertificateChain(clientCerts);
    clientConfig.setPrivateKey(clientKey);
    clientConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    QVERIFY(clientCrypto->setDtlsConfiguration(clientConfig));
    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort));

    QVERIFY(clientCrypto->doHandshake(&clientSocket));
    QDTLS_VERIFY_NO_ERROR(clientCrypto);

    testLoop.enterLoopMSecs(handshakeTimeoutMS);

    serverConfig = serverCrypto->dtlsConfiguration();

    if (verifyMode == QSslSocket::VerifyNone || clientCerts.isEmpty()) {
        QVERIFY(serverConfig.peerCertificate().isNull());
        QCOMPARE(serverConfig.peerCertificateChain().size(), 0);
    } else {
        QCOMPARE(serverConfig.peerCertificate(), clientCerts.first());
        QCOMPARE(serverConfig.peerCertificateChain(), clientCerts);
    }

    if (encrypted) {
        QDTLS_VERIFY_HANDSHAKE_SUCCESS(serverCrypto);
        QDTLS_VERIFY_HANDSHAKE_SUCCESS(clientCrypto);
    } else {
        QVERIFY(!serverCrypto->isConnectionEncrypted());
        QCOMPARE(serverCrypto->handshakeState(), QDtls::PeerVerificationFailed);
        QVERIFY(serverCrypto->dtlsErrorString().size() > 0);
        QVERIFY(serverCrypto->peerVerificationErrors().size() > 0);

        QVERIFY(!clientCrypto->isConnectionEncrypted());
        QDTLS_VERIFY_NO_ERROR(clientCrypto);
        QCOMPARE(clientCrypto->handshakeState(), QDtls::HandshakeInProgress);
    }
}

void tst_QDtls::blacklistedCerificate()
{
    const auto serverChain = QSslCertificate::fromPath(certDirPath + QStringLiteral("fake-login.live.com.pem"));
    QCOMPARE(serverChain.size(), 1);

    QFile keyFile(certDirPath + QStringLiteral("fake-login.live.com.key"));
    QVERIFY(keyFile.open(QIODevice::ReadOnly));
    const QSslKey key(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!key.isNull());

    auto serverConfig = defaultServerConfig;
    serverConfig.setLocalCertificateChain(serverChain);
    serverConfig.setPrivateKey(key);
    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));

    connectHandshakeReadingSlots();
    const QString name(serverChain.first().subjectInfo(QSslCertificate::CommonName).first());
    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort, name));
    QVERIFY(clientCrypto->doHandshake(&clientSocket));

    testLoop.enterLoopMSecs(handshakeTimeoutMS);
    QVERIFY(!testLoop.timeout());
    QCOMPARE(clientCrypto->handshakeState(), QDtls::PeerVerificationFailed);
    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::PeerVerificationError);
    QVERIFY(!clientCrypto->isConnectionEncrypted());
    QVERIFY(verificationErrorDetected(QSslError::CertificateBlacklisted));
}

void tst_QDtls::readWriteEncrypted_data()
{
    QTest::addColumn<bool>("serverSideShutdown");

    QTest::addRow("client-shutdown") << false;
    QTest::addRow("server-shutdown") << true;
}

void tst_QDtls::readWriteEncrypted()
{
    connectHandshakeReadingSlots();

    auto serverConfig = defaultServerConfig;
    serverConfig.setLocalCertificate(selfSignedCert);
    serverConfig.setPrivateKey(serverKeySS);
    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));

    auto clientConfig = QSslConfiguration::defaultDtlsConfiguration();
    clientConfig.setCaCertificates({selfSignedCert});
    QVERIFY(clientCrypto->setDtlsConfiguration(clientConfig));
    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort, hostName));

    // 0. Verify we cannot write any encrypted message without handshake done
    QDTLS_VERIFY_NO_ERROR(clientCrypto);
    QVERIFY(clientCrypto->writeDatagramEncrypted(&clientSocket, serverExpectedPlainText) <= 0);
    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidOperation);
    QVERIFY(!clientCrypto->shutdown(&clientSocket));
    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidOperation);
    QDTLS_VERIFY_NO_ERROR(serverCrypto);
    QVERIFY(serverCrypto->writeDatagramEncrypted(&serverSocket, clientExpectedPlainText) <= 0);
    QCOMPARE(serverCrypto->dtlsError(), QDtlsError::InvalidOperation);
    QVERIFY(!serverCrypto->shutdown(&serverSocket));
    QCOMPARE(serverCrypto->dtlsError(), QDtlsError::InvalidOperation);

    // 1. Initiate a handshake:
    QVERIFY(clientCrypto->doHandshake(&clientSocket));
    QDTLS_VERIFY_NO_ERROR(clientCrypto);
    // 1.1 Verify we cannot read yet. What the datagram is - not really important,
    // invalid state/operation - is what we verify:
    const QByteArray dummy = clientCrypto->decryptDatagram(&clientSocket, "BS dgram");
    QCOMPARE(dummy.size(), 0);
    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidOperation);

    // 1.2 Finish the handshake:
    testLoop.enterLoopMSecs(handshakeTimeoutMS);
    QVERIFY(!testLoop.timeout());

    QDTLS_VERIFY_HANDSHAKE_SUCCESS(clientCrypto);
    QDTLS_VERIFY_HANDSHAKE_SUCCESS(serverCrypto);

    // 2. Change reading slots:
    connectEncryptedReadingSlots();

    // 3. Test parameter validation:
    QVERIFY(clientCrypto->writeDatagramEncrypted(nullptr, serverExpectedPlainText) <= 0);
    QCOMPARE(clientCrypto->dtlsError(), QDtlsError::InvalidInputParameters);
    // 4. Write the client's message:
    qint64 clientBytesWritten = clientCrypto->writeDatagramEncrypted(&clientSocket, serverExpectedPlainText);
    QDTLS_VERIFY_NO_ERROR(clientCrypto);
    QVERIFY(clientBytesWritten > 0);

    // 5. Exchange client/server messages:
    testLoop.enterLoopMSecs(dataExchangeTimeoutMS);
    QVERIFY(!testLoop.timeout());

    QCOMPARE(serverExpectedPlainText, serverReceivedPlainText);
    QCOMPARE(clientExpectedPlainText, clientReceivedPlainText);

    QFETCH(const bool, serverSideShutdown);
    DtlsPtr &crypto = serverSideShutdown ? serverCrypto : clientCrypto;
    QUdpSocket *socket = serverSideShutdown ? &serverSocket : &clientSocket;
    // 6. Parameter validation:
    QVERIFY(!crypto->shutdown(nullptr));
    QCOMPARE(crypto->dtlsError(), QDtlsError::InvalidInputParameters);
    // 7. Send shutdown alert:
    QVERIFY(crypto->shutdown(socket));
    QDTLS_VERIFY_NO_ERROR(crypto);
    QCOMPARE(crypto->handshakeState(), QDtls::HandshakeNotStarted);
    QVERIFY(!crypto->isConnectionEncrypted());
    // 8. Receive this read notification and handle it:
    testLoop.enterLoopMSecs(dataExchangeTimeoutMS);
    QVERIFY(!testLoop.timeout());

    DtlsPtr &peerCrypto = serverSideShutdown ? clientCrypto : serverCrypto;
    QVERIFY(!peerCrypto->isConnectionEncrypted());
    QCOMPARE(peerCrypto->handshakeState(), QDtls::HandshakeNotStarted);
    QCOMPARE(peerCrypto->dtlsError(), QDtlsError::RemoteClosedConnectionError);
}

void tst_QDtls::datagramFragmentation()
{
    connectHandshakeReadingSlots();

    auto serverConfig = defaultServerConfig;
    serverConfig.setLocalCertificate(selfSignedCert);
    serverConfig.setPrivateKey(serverKeySS);
    QVERIFY(serverCrypto->setDtlsConfiguration(serverConfig));

    auto clientConfig = QSslConfiguration::defaultDtlsConfiguration();
    clientConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    QVERIFY(clientCrypto->setDtlsConfiguration(clientConfig));
    QVERIFY(clientCrypto->setPeer(serverAddress, serverPort));

    QVERIFY(clientCrypto->doHandshake(&clientSocket));

    testLoop.enterLoopMSecs(handshakeTimeoutMS);
    QVERIFY(!testLoop.timeout());

    QDTLS_VERIFY_HANDSHAKE_SUCCESS(clientCrypto);
    QDTLS_VERIFY_HANDSHAKE_SUCCESS(serverCrypto);

    // Done with handshake, reconnect readyRead:
    connectEncryptedReadingSlots();

    // Verify our dgram is not fragmented and some error set (either UnderlyingSocketError
    // if OpenSSL somehow had attempted a write or TlsFatalError in case OpenSSL
    // noticed how big the chunk is).
    QVERIFY(clientCrypto->writeDatagramEncrypted(&clientSocket, QByteArray(1024 * 17, Qt::Uninitialized)) <= 0);
    QVERIFY(clientCrypto->dtlsError() != QDtlsError::NoError);
    // Error to write does not mean QDtls is broken:
    QVERIFY(clientCrypto->isConnectionEncrypted());
    QVERIFY(clientCrypto->writeDatagramEncrypted(&clientSocket, "Hello, I'm a tiny datagram") > 0);
    QDTLS_VERIFY_NO_ERROR(clientCrypto);
}

void tst_QDtls::handshakeReadyRead()
{
    QUdpSocket *socket = qobject_cast<QUdpSocket *>(sender());
    Q_ASSERT(socket);

    if (socket->pendingDatagramSize() <= 0)
        return;

    const bool isServer = socket == &serverSocket;
    DtlsPtr &crypto = isServer ? serverCrypto : clientCrypto;
    DtlsPtr &peerCrypto = isServer ? clientCrypto : serverCrypto;
    QHostAddress addr;
    quint16 port = 0;

    QByteArray dgram(socket->pendingDatagramSize(), Qt::Uninitialized);
    const qint64 size = socket->readDatagram(dgram.data(), dgram.size(), &addr, &port);
    if (size != dgram.size())
        return;

    if (isServer) {
        if (!clientPort) {
            // It's probably an initial 'ClientHello' message. Let's set remote's
            // address/port. But first we make sure it is, indeed, 'ClientHello'.
            if (int(dgram.constData()[0]) != 22)
                return;

            if (addr.isNull() || addr.isBroadcast()) // Could never be us (client), bail out
                return;

            if (!crypto->setPeer(addr, port))
                return testLoop.exitLoop();

            // Check parameter validation:
            if (crypto->doHandshake(nullptr, dgram) || crypto->dtlsError() != QDtlsError::InvalidInputParameters)
                return testLoop.exitLoop();

            if (crypto->doHandshake(&serverSocket, {}) || crypto->dtlsError() != QDtlsError::InvalidInputParameters)
                return testLoop.exitLoop();

            // Make sure we cannot decrypt yet:
            const QByteArray dummyDgram = crypto->decryptDatagram(&serverSocket, dgram);
            if (dummyDgram.size() > 0 || crypto->dtlsError() != QDtlsError::InvalidOperation)
                return testLoop.exitLoop();

            clientAddress = addr;
            clientPort = port;
        } else if (clientPort != port || clientAddress != addr) {
            return;
        }

        if (serverDropDgram) {
            serverDropDgram = false;
            return;
        }
    } else if (clientDropDgram) {
        clientDropDgram = false;
        return;
    }

    if (!crypto->doHandshake(socket, dgram))
        return testLoop.exitLoop();

    const auto state = crypto->handshakeState();
    if (state != QDtls::HandshakeInProgress && state != QDtls::HandshakeComplete)
        return testLoop.exitLoop();

    if (state == QDtls::HandshakeComplete && peerCrypto->handshakeState() == QDtls::HandshakeComplete)
        testLoop.exitLoop();
}

void tst_QDtls::encryptedReadyRead()
{
    QUdpSocket *socket = qobject_cast<QUdpSocket *>(sender());
    Q_ASSERT(socket);

    if (socket->pendingDatagramSize() <= 0)
        return;

    QByteArray dtlsMessage(int(socket->pendingDatagramSize()), Qt::Uninitialized);
    QHostAddress addr;
    quint16 port = 0;
    const qint64 bytesRead = socket->readDatagram(dtlsMessage.data(), dtlsMessage.size(), &addr, &port);
    if (bytesRead <= 0)
        return;

    dtlsMessage.resize(int(bytesRead));

    if (socket == &serverSocket) {
        if (addr != clientAddress || port != clientPort)
            return;

        if (serverExpectedPlainText == dtlsMessage) // No way it can happen!
            return testLoop.exitLoop();

        serverReceivedPlainText = serverCrypto->decryptDatagram(nullptr, dtlsMessage);
        if (serverReceivedPlainText.size() > 0 || serverCrypto->dtlsError() != QDtlsError::InvalidInputParameters)
            return testLoop.exitLoop();

        serverReceivedPlainText = serverCrypto->decryptDatagram(&serverSocket, dtlsMessage);

        const int messageType = dtlsMessage.data()[0];
        if (serverReceivedPlainText != serverExpectedPlainText
            && (messageType == 23 || messageType == 21)) {
            // Type 23 is for application data, 21 is shutdown alert. Here we test
            // write/read operations and shutdown alerts, not expecting and thus
            // ignoring any other types of messages.
            return testLoop.exitLoop();
        }

        if (serverCrypto->dtlsError() != QDtlsError::NoError)
            return testLoop.exitLoop();

        // Verify it cannot be done twice:
        const QByteArray replayed = serverCrypto->decryptDatagram(&serverSocket, dtlsMessage);
        if (replayed.size() > 0)
            return testLoop.exitLoop();

        if (serverCrypto->writeDatagramEncrypted(&serverSocket, clientExpectedPlainText) <= 0)
            testLoop.exitLoop();
    } else {
        if (port != serverPort)
            return;

        if (clientExpectedPlainText == dtlsMessage) // What a disaster!
            return testLoop.exitLoop();

        clientReceivedPlainText = clientCrypto->decryptDatagram(&clientSocket, dtlsMessage);
        testLoop.exitLoop();
    }
}

void tst_QDtls::pskRequested(QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);

    auth->setPreSharedKey(presharedKey);
}

void tst_QDtls::handleHandshakeTimeout()
{
    auto crypto = qobject_cast<QDtls *>(sender());
    Q_ASSERT(crypto);

    if (!crypto->handleTimeout(&clientSocket))
        testLoop.exitLoop();
}

void tst_QDtls::clientServerData()
{
    QTest::addColumn<QSslSocket::SslMode>("mode");

    QTest::addRow("client") << QSslSocket::SslClientMode;
    QTest::addRow("server") << QSslSocket::SslServerMode;
}

void tst_QDtls::connectHandshakeReadingSlots()
{
    connect(&serverSocket, &QUdpSocket::readyRead, this, &tst_QDtls::handshakeReadyRead);
    connect(&clientSocket, &QUdpSocket::readyRead, this, &tst_QDtls::handshakeReadyRead);
}

void tst_QDtls::connectEncryptedReadingSlots()
{
    serverSocket.disconnect();
    clientSocket.disconnect();
    connect(&serverSocket, &QUdpSocket::readyRead, this, &tst_QDtls::encryptedReadyRead);
    connect(&clientSocket, &QUdpSocket::readyRead, this, &tst_QDtls::encryptedReadyRead);
}

bool tst_QDtls::verificationErrorDetected(QSslError::SslError code) const
{
    Q_ASSERT(clientCrypto.data());

    const auto errors = clientCrypto->peerVerificationErrors();
    for (const QSslError &error : errors) {
        if (error.error() == code)
            return true;
    }

    return false;
}

QHostAddress tst_QDtls::toNonAny(const QHostAddress &addr)
{
    if (addr == QHostAddress::Any || addr == QHostAddress::AnyIPv4)
        return QHostAddress::LocalHost;
    if (addr == QHostAddress::AnyIPv6)
        return QHostAddress::LocalHostIPv6;
    return addr;
}

QT_END_NAMESPACE

QTEST_MAIN(tst_QDtls)

#include "tst_qdtls.moc"
