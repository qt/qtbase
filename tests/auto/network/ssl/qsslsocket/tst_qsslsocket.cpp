// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Governikus GmbH & Co. KG.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/qthread.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qrandom.h>
#include <QtCore/qscopeguard.h>
#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qhostinfo.h>
#include <QtNetwork/qnetworkproxy.h>
#include <QtNetwork/qsslcipher.h>
#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qsslkey.h>
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qsslpresharedkeyauthenticator.h>

#include <QtTest/private/qemulationdetector_p.h>

#include <QTest>
#include <QNetworkProxy>
#include <QAuthenticator>
#include <QTestEventLoop>
#include <QSignalSpy>
#include <QSemaphore>

#include "private/qhostinfo_p.h"
#include "private/qiodevice_p.h" // for QIODEVICE_BUFFERSIZE

#include "../../../network-settings.h"
#include "../shared/tlshelpers.h"

#if QT_CONFIG(openssl)
#include "../shared/qopenssl_symbols.h"
#endif

#include "private/qtlsbackend_p.h"

#include "private/qsslsocket_p.h"
#include "private/qsslconfiguration_p.h"

using namespace std::chrono_literals;

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
// make these enum values available without causing deprecation warnings:
namespace Test {
#define COPY(tag, v) \
    constexpr auto tag ## V ## v = QSsl:: tag ## V ## v ; \
    constexpr auto tag ## V ## v ## OrLater = QSsl:: tag ## V ## v ## OrLater ; \
    /* end */
COPY(Tls, 1_0)
COPY(Dtls, 1_0)
COPY(Tls, 1_1)
#undef COPY
} // namespace Test
QT_WARNING_POP

Q_DECLARE_METATYPE(QSslSocket::SslMode)
typedef QList<QSslError::SslError> SslErrorList;
Q_DECLARE_METATYPE(SslErrorList)
Q_DECLARE_METATYPE(QSslError)
Q_DECLARE_METATYPE(QSslKey)
Q_DECLARE_METATYPE(QSsl::SslProtocol)
Q_DECLARE_METATYPE(QSslSocket::PeerVerifyMode);
typedef QSharedPointer<QSslSocket> QSslSocketPtr;

#if defined Q_OS_HPUX && defined Q_CC_GNU
// This error is delivered every time we try to use the fluke CA
// certificate. For now we work around this bug. Task 202317.
#define QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
#endif

// Use this cipher to force PSK key sharing.
// Also, it's a cipher w/o auth, to check that we emit the signals warning
// about the identity of the peer.
#if QT_CONFIG(openssl)
static const QString PSK_CIPHER_WITHOUT_AUTH = QStringLiteral("PSK-AES256-CBC-SHA");
static const quint16 PSK_SERVER_PORT = 4433;
static const QByteArray PSK_CLIENT_PRESHAREDKEY = QByteArrayLiteral("\x1a\x2b\x3c\x4d\x5e\x6f");
static const QByteArray PSK_SERVER_IDENTITY_HINT = QByteArrayLiteral("QtTestServerHint");
static const QByteArray PSK_CLIENT_IDENTITY = QByteArrayLiteral("Client_identity");
#endif  // QT_CONFIG(openssl)

QT_BEGIN_NAMESPACE
void qt_ForceTlsSecurityLevel();
QT_END_NAMESPACE

class tst_QSslSocket : public QObject
{
    Q_OBJECT

    int proxyAuthCalled;

public:
    tst_QSslSocket();

    static void enterLoop(int secs)
    {
        ++loopLevel;
        QTestEventLoop::instance().enterLoop(secs);
    }

    static bool timeout()
    {
        return QTestEventLoop::instance().timeout();
    }

#if QT_CONFIG(ssl)
    QSslSocketPtr newSocket();

#if QT_CONFIG(openssl)
    enum PskConnectTestType {
        PskConnectDoNotHandlePsk,
        PskConnectEmptyCredentials,
        PskConnectWrongCredentials,
        PskConnectWrongIdentity,
        PskConnectWrongPreSharedKey,
        PskConnectRightCredentialsPeerVerifyFailure,
        PskConnectRightCredentialsVerifyPeer,
        PskConnectRightCredentialsDoNotVerifyPeer,
    };
#endif // QT_CONFIG(openssl)
#endif // QT_CONFIG(ssl)

public slots:
    void initTestCase_data();
    void initTestCase();
    void init();
    void cleanup();
#if QT_CONFIG(networkproxy)
    void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth);
#endif

#if QT_CONFIG(ssl)
private slots:
    void activeBackend();
    void backends();
    void constructing();
    void configNoOnDemandLoad();
    void simpleConnect();
    void simpleConnectWithIgnore();

    // API tests
    void sslErrors_data();
    void sslErrors();
    void ciphers();
#if QT_CONFIG(securetransport)
    void tls13Ciphers();
#endif // QT_CONFIG(securetransport)
    void connectToHostEncrypted();
    void connectToHostEncryptedWithVerificationPeerName();
    void sessionCipher();
    void localCertificate();
    void mode();
    void peerCertificate();
    void peerCertificateChain();
    void privateKey();
#if QT_CONFIG(openssl)
    void privateKeyOpaque();
#endif
    void protocol();
    void protocolServerSide_data();
    void protocolServerSide();
#if QT_CONFIG(openssl)
    void serverCipherPreferences();
#endif
    void setCaCertificates();
    void setLocalCertificate();
    void localCertificateChain();
    void setLocalCertificateChain();
    void tlsConfiguration();
    void setSocketDescriptor();
    void setSslConfiguration_data();
    void setSslConfiguration();
    void waitForEncrypted();
    void waitForEncryptedMinusOne();
    void waitForConnectedEncryptedReadyRead();
    void startClientEncryption();
    void startServerEncryption();
    void addDefaultCaCertificate();
    void defaultCaCertificates();
    void defaultCiphers();
    void resetDefaultCiphers();
    void setDefaultCaCertificates();
    void setDefaultCiphers();
    void supportedCiphers();
    void systemCaCertificates();
    void wildcardCertificateNames();
    void isMatchingHostname();
    void wildcard();
    void setEmptyKey();
    void spontaneousWrite();
    void setReadBufferSize();
    void setReadBufferSize_task_250027();
    void waitForMinusOne();
    void verifyMode();
    void verifyDepth();
    void verifyAndDefaultConfiguration();
    void disconnectFromHostWhenConnecting();
    void disconnectFromHostWhenConnected();
#if QT_CONFIG(openssl)
    void closeWhileEmittingSocketError();
#endif
    void resetProxy();
    void ignoreSslErrorsList_data();
    void ignoreSslErrorsList();
    void ignoreSslErrorsListWithSlot_data();
    void ignoreSslErrorsListWithSlot();
    void abortOnSslErrors();
    void readFromClosedSocket();
    void writeBigChunk();
    void blacklistedCertificates();
    void versionAccessors();
    void encryptWithoutConnecting();
    void resume_data();
    void resume();
    void qtbug18498_peek();
    void qtbug18498_peek2();
    void dhServer();
#if QT_CONFIG(openssl)
    void dhServerCustomParamsNull();
    void dhServerCustomParams();
#endif // QT_CONFIG(openssl)
    void ecdhServer();
    void verifyClientCertificate_data();
    void verifyClientCertificate();
    void readBufferMaxSize();

    void allowedProtocolNegotiation();

#if QT_CONFIG(openssl)
    void simplePskConnect_data();
    void simplePskConnect();
    void ephemeralServerKey_data();
    void ephemeralServerKey();
    void pskServer();
    void forwardReadChannelFinished();
    void signatureAlgorithm_data();
    void signatureAlgorithm();
#endif // QT_CONFIG(openssl)

    void unsupportedProtocols_data();
    void unsupportedProtocols();

    void oldErrorsOnSocketReuse();
#if QT_CONFIG(openssl)
    void alertMissingCertificate();
    void alertInvalidCertificate();
    void selfSignedCertificates_data();
    void selfSignedCertificates();
    void pskHandshake_data();
    void pskHandshake();
#endif // openssl

    void setEmptyDefaultConfiguration(); // this test should be last

protected slots:

    static void exitLoop()
    {
        // Safe exit - if we aren't in an event loop, don't
        // exit one.
        if (loopLevel > 0) {
            --loopLevel;
            QTestEventLoop::instance().exitLoop();
        }
    }

    void ignoreErrorSlot()
    {
        socket->ignoreSslErrors();
    }
    void abortOnErrorSlot()
    {
        QSslSocket *sock = static_cast<QSslSocket *>(sender());
        sock->abort();
    }
    void untrustedWorkaroundSlot(const QList<QSslError> &errors)
    {
        if (errors.size() == 1 &&
                (errors.first().error() == QSslError::CertificateUntrusted ||
                        errors.first().error() == QSslError::SelfSignedCertificate))
            socket->ignoreSslErrors();
    }
    void ignoreErrorListSlot(const QList<QSslError> &errors);

private:
    QSslSocket *socket;
    QList<QSslError> storedExpectedSslErrors;
    bool isTestingOpenSsl = false;
    bool isSecurityLevel0Required = false;
    bool opensslResolved = false;
    bool isTestingSecureTransport = false;
    bool isTestingSchannel = false;
    QSslError::SslError flukeCertificateError = QSslError::CertificateUntrusted;
    bool hasServerAlpn = false;
#endif // QT_CONFIG(ssl)
private:
    static int loopLevel;
public:
    static QString testDataDir;

    bool supportsTls13() const
    {
        if (isTestingOpenSsl) {
#ifdef TLS1_3_VERSION
            return true;
#endif
        }
        if (isTestingSchannel) {
            // Copied from qtls_schannel.cpp #supportsTls13()
            static bool supported = []() {
                const auto current = QOperatingSystemVersion::current();
                const auto minimum =
                        QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 20221);
                return current >= minimum;
            }();
            return supported;
        }

        return false;
    }

};
QString tst_QSslSocket::testDataDir;

#if QT_CONFIG(ssl)
#if QT_CONFIG(openssl)
Q_DECLARE_METATYPE(tst_QSslSocket::PskConnectTestType)
#endif // QT_CONFIG(openssl)
#endif // QT_CONFIG(ssl)

int tst_QSslSocket::loopLevel = 0;

namespace {

QString httpServerCertChainPath()
{
    // DOCKERTODO: note how we use CA certificate on the real server. The docker container
    // is using a different cert with a "special" CN. Check if it's important!
#ifdef QT_TEST_SERVER
    return tst_QSslSocket::testDataDir + QStringLiteral("certs/qt-test-server-cert.pem");
#else
    return tst_QSslSocket::testDataDir + QStringLiteral("certs/qt-test-server-cacert.pem");
#endif // QT_TEST_SERVER
}

} // unnamed namespace

tst_QSslSocket::tst_QSslSocket()
{
#if QT_CONFIG(ssl)
    qRegisterMetaType<QList<QSslError> >("QList<QSslError>");
    qRegisterMetaType<QSslError>("QSslError");
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");

#if QT_CONFIG(openssl)
    qRegisterMetaType<QSslPreSharedKeyAuthenticator *>();
    qRegisterMetaType<tst_QSslSocket::PskConnectTestType>();
#endif // QT_CONFIG(openssl)

#endif // QT_CONFIG(ssl)
}

enum ProxyTests {
    NoProxy = 0x00,
    Socks5Proxy = 0x01,
    HttpProxy = 0x02,
    TypeMask = 0x0f,

    NoAuth = 0x00,
    AuthBasic = 0x10,
    AuthNtlm = 0x20,
    AuthMask = 0xf0
};

void tst_QSslSocket::initTestCase_data()
{
    QTest::addColumn<bool>("setProxy");
    QTest::addColumn<int>("proxyType");

    QTest::newRow("WithoutProxy") << false << 0;
    QTest::newRow("WithSocks5Proxy") << true << int(Socks5Proxy);
    QTest::newRow("WithSocks5ProxyAuth") << true << int(Socks5Proxy | AuthBasic);

    QTest::newRow("WithHttpProxy") << true << int(HttpProxy);
    QTest::newRow("WithHttpProxyBasicAuth") << true << int(HttpProxy | AuthBasic);
    // uncomment the line below when NTLM works
//    QTest::newRow("WithHttpProxyNtlmAuth") << true << int(HttpProxy | AuthNtlm);
}

void tst_QSslSocket::initTestCase()
{
    testDataDir = QFileInfo(QFINDTESTDATA("certs")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();
    if (!testDataDir.endsWith(QLatin1String("/")))
        testDataDir += QLatin1String("/");

    hasServerAlpn = QSslSocket::supportedFeatures().contains(QSsl::SupportedFeature::ServerSideAlpn);
    // Several plugins (TLS-backends) can co-exist. QSslSocket would implicitly
    // select 'openssl' if available, and if not: 'securetransport' (Darwin) or
    // 'schannel' (Windows). Check what we actually have:
    const auto &tlsBackends = QSslSocket::availableBackends();
    if (tlsBackends.contains(QTlsBackend::builtinBackendNames[QTlsBackend::nameIndexOpenSSL])) {
        isTestingOpenSsl = true;
        flukeCertificateError = QSslError::SelfSignedCertificate;
#if QT_CONFIG(openssl)
        opensslResolved = qt_auto_test_resolve_OpenSSL_symbols();
        // This is where OpenSSL moved several protocols under
        // non-default (0) security level (the default is 1).
        isSecurityLevel0Required = OPENSSL_VERSION_NUMBER >= 0x30100010;
#else
        opensslResolved = false; // Not 'unused variable' anymore.
#endif
    } else if (tlsBackends.contains(QTlsBackend::builtinBackendNames[QTlsBackend::nameIndexSchannel])) {
        isTestingSchannel = true;
    } else {
        QVERIFY(tlsBackends.contains(QTlsBackend::builtinBackendNames[QTlsBackend::nameIndexSecureTransport]));
        isTestingSecureTransport = true;
    }

#if QT_CONFIG(ssl)
    qDebug("Using SSL library %s (%ld)",
           qPrintable(QSslSocket::sslLibraryVersionString()),
           QSslSocket::sslLibraryVersionNumber());
#ifdef QT_TEST_SERVER
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::socksProxyServerName(), 1080));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::socksProxyServerName(), 1081));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::httpProxyServerName(), 3128));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::httpProxyServerName(), 3129));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::httpProxyServerName(), 3130));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::httpServerName(), 443));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::imapServerName(), 993));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::echoServerName(), 13));
#else
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
#endif // QT_TEST_SERVER
#endif // QT_CONFIG(ssl)
}

void tst_QSslSocket::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#if QT_CONFIG(networkproxy)
        QFETCH_GLOBAL(int, proxyType);
        const QString socksProxyAddr = QtNetworkSettings::socksProxyServerIp().toString();
        const QString httpProxyAddr = QtNetworkSettings::httpProxyServerIp().toString();
        QNetworkProxy proxy;

        switch (proxyType) {
        case Socks5Proxy:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, socksProxyAddr, 1080);
            break;

        case Socks5Proxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, socksProxyAddr, 1081);
            break;

        case HttpProxy | NoAuth:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, httpProxyAddr, 3128);
            break;

        case HttpProxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, httpProxyAddr, 3129);
            break;

        case HttpProxy | AuthNtlm:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, httpProxyAddr, 3130);
            break;
        }
        QNetworkProxy::setApplicationProxy(proxy);
#else
        QSKIP("No proxy support");
#endif // QT_CONFIG(networkproxy)
    }

    QT_PREPEND_NAMESPACE(qt_ForceTlsSecurityLevel)();

    qt_qhostinfo_clear_cache();
}

void tst_QSslSocket::cleanup()
{
#if QT_CONFIG(networkproxy)
    QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
#endif
}

#if QT_CONFIG(ssl)
QSslSocketPtr tst_QSslSocket::newSocket()
{
    const auto socket = QSslSocketPtr::create();

    proxyAuthCalled = 0;
    connect(socket.data(), SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            Qt::DirectConnection);

    return socket;
}
#endif // QT_CONFIG(ssl)

#if QT_CONFIG(networkproxy)
void tst_QSslSocket::proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
{
    ++proxyAuthCalled;
    auth->setUser("qsockstest");
    auth->setPassword("password");
}
#endif // QT_CONFIG(networkproxy)

#if QT_CONFIG(ssl)

void tst_QSslSocket::activeBackend()
{
    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy) // Not interesting for backend test.
        return;

    // We cannot set non-existing as active:
    const QString nonExistingBackend = QStringLiteral("TheQtTLS");
    QCOMPARE(QSslSocket::setActiveBackend(nonExistingBackend), false);
    QCOMPARE(QSslSocket::supportedProtocols(nonExistingBackend).size(), 0);
    QCOMPARE(QSslSocket::supportedFeatures(nonExistingBackend), QList<QSsl::SupportedFeature>());
    QCOMPARE(QSslSocket::implementedClasses(nonExistingBackend), QList<QSsl::ImplementedClass>());

    const QString backendName = QSslSocket::activeBackend();
    // Implemented by all our existing backends:
    const auto implemented = QSsl::ImplementedClass::Socket;
    const auto supportedFt = QSsl::SupportedFeature::ClientSideAlpn;

    QVERIFY(QSslSocket::availableBackends().contains(backendName));
    QCOMPARE(QSslSocket::setActiveBackend(backendName), true);
    QCOMPARE(QSslSocket::activeBackend(), backendName);
    QCOMPARE(QSslSocket::setActiveBackend(backendName), true); // We can do it again.
    QCOMPARE(QSslSocket::activeBackend(), backendName);

    const auto protocols = QSslSocket::supportedProtocols();
    QVERIFY(protocols.size() > 0);
    // 'Any' and 'Secure', since they are always present:
    QVERIFY(protocols.contains(QSsl::AnyProtocol));
    QVERIFY(protocols.contains(QSsl::SecureProtocols));

    const auto protocolsForNamed = QSslSocket::supportedProtocols(backendName);
    QCOMPARE(protocols, protocolsForNamed);
    // Any and secure, new versions are coming, old
    // go away, nothing more specific.
    QVERIFY(protocolsForNamed.contains(QSsl::AnyProtocol));
    QVERIFY(protocolsForNamed.contains(QSsl::SecureProtocols));
    QCOMPARE(QSslSocket::isProtocolSupported(QSsl::SecureProtocols), true);
    QCOMPARE(QSslSocket::isProtocolSupported(QSsl::SecureProtocols, backendName), true);

    const auto classes = QSslSocket::implementedClasses();
    QVERIFY(classes.contains(implemented));
    QVERIFY(QSslSocket::isClassImplemented(implemented));
    QVERIFY(QSslSocket::isClassImplemented(implemented, backendName));

    const auto features = QSslSocket::supportedFeatures();
    QVERIFY(features.contains(QSsl::SupportedFeature(supportedFt)));
    QVERIFY(QSslSocket::isFeatureSupported(QSsl::SupportedFeature(supportedFt)));
    QVERIFY(QSslSocket::isFeatureSupported(QSsl::SupportedFeature(supportedFt), backendName));
}

namespace {
struct MockTlsBackend : QTlsBackend
{
    MockTlsBackend(const QString &mockName) : name(mockName)
    {
    }
    QString backendName() const override
    {
        return name;
    }

    QList<QSsl::SupportedFeature> supportedFeatures() const override
    {
        return features;
    }
    QList<QSsl::SslProtocol> supportedProtocols() const override
    {
        return protocols;
    }
    QList<QSsl::ImplementedClass> implementedClasses() const override
    {
        return classes;
    }

    QString name;
    QList<QSsl::ImplementedClass> classes;
    QList<QSsl::SupportedFeature> features;
    QList<QSsl::SslProtocol> protocols;
};

} // Unnamed backend.

void tst_QSslSocket::backends()
{
    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy) // Not interesting for backend test.
        return;

    // We are here, protected by QT_CONFIG(ssl). Some backend must be pre-existing.
    // Let's test the 'real' backend:
    auto backendNames = QTlsBackend::availableBackendNames();
    const auto sizeBefore = backendNames.size();
    QVERIFY(sizeBefore > 0);

    const QString tlsBackend = QSslSocket::activeBackend();
    QVERIFY(tlsBackend == QTlsBackend::builtinBackendNames[QTlsBackend::nameIndexOpenSSL]
            || tlsBackend == QTlsBackend::builtinBackendNames[QTlsBackend::nameIndexSchannel]
            || tlsBackend == QTlsBackend::builtinBackendNames[QTlsBackend::nameIndexSecureTransport]);

    const auto builtinProtocols = QSslSocket::supportedProtocols(tlsBackend);
    QVERIFY(builtinProtocols.contains(QSsl::SecureProtocols));
    // Socket and ALPN are supported by all our backends:
    const auto builtinClasses = QSslSocket::implementedClasses(tlsBackend);
    QVERIFY(builtinClasses.contains(QSsl::ImplementedClass::Socket));
    const auto builtinFeatures = QSslSocket::supportedFeatures(tlsBackend);
    QVERIFY(builtinFeatures.contains(QSsl::SupportedFeature::ClientSideAlpn));

    // Verify that non-dummy backend can be found:
    auto *systemBackend = QTlsBackend::findBackend(tlsBackend);
    QVERIFY(systemBackend);

    const auto protocols = QList<QSsl::SslProtocol>{QSsl::SecureProtocols};
    const auto classes = QList<QSsl::ImplementedClass>{QSsl::ImplementedClass::Socket};
    const auto features = QList<QSsl::SupportedFeature>{QSsl::SupportedFeature::CertificateVerification};

    const QString nameA = QStringLiteral("backend A");
    const QString nameB = QStringLiteral("backend B");
    const QString nonExisting = QStringLiteral("non-existing backend");

    QVERIFY(!backendNames.contains(nameA));
    QVERIFY(!backendNames.contains(nameB));
    QVERIFY(!backendNames.contains(nonExisting));
    {
        MockTlsBackend factoryA(nameA);
        backendNames = QTlsBackend::availableBackendNames();
        QVERIFY(backendNames.contains(nameA));
        QVERIFY(!backendNames.contains(nameB));
        QVERIFY(!backendNames.contains(nonExisting));

        const auto *backendA = QTlsBackend::findBackend(nameA);
        QVERIFY(backendA);
        QCOMPARE(backendA->backendName(), nameA);

        QCOMPARE(factoryA.supportedFeatures().size(), 0);
        QCOMPARE(factoryA.supportedProtocols().size(), 0);
        QCOMPARE(factoryA.implementedClasses().size(), 0);

        factoryA.protocols = protocols;
        factoryA.classes = classes;
        factoryA.features = features;

        // It's an overrider in some re-implemented factory:
        QCOMPARE(factoryA.supportedProtocols(), protocols);
        QCOMPARE(factoryA.supportedFeatures(), features);
        QCOMPARE(factoryA.implementedClasses(), classes);

        // That's a helper function (static member function):
        QCOMPARE(QTlsBackend::supportedProtocols(nameA), protocols);
        QCOMPARE(QTlsBackend::supportedFeatures(nameA), features);
        QCOMPARE(QTlsBackend::implementedClasses(nameA), classes);

        MockTlsBackend factoryB(nameB);
        QVERIFY(QTlsBackend::availableBackendNames().contains(nameA));
        QVERIFY(QTlsBackend::availableBackendNames().contains(nameB));
        QVERIFY(!QTlsBackend::availableBackendNames().contains(nonExisting));

        const auto *nullBackend = QTlsBackend::findBackend(nonExisting);
        QCOMPARE(nullBackend, nullptr);
    }
    backendNames = QTlsBackend::availableBackendNames();
    QCOMPARE(backendNames.size(), sizeBefore);
    // Check we cleaned up our factories:
    QVERIFY(!backendNames.contains(nameA));
    QVERIFY(!backendNames.contains(nameB));
}

void tst_QSslSocket::constructing()
{
    const char readNotOpenMessage[] = "QIODevice::read (QSslSocket): device not open";
    const char writeNotOpenMessage[] = "QIODevice::write (QSslSocket): device not open";

    if (!QSslSocket::supportsSsl())
        return;

    QSslSocket socket;

    QCOMPARE(socket.state(), QSslSocket::UnconnectedState);
    QCOMPARE(socket.mode(), QSslSocket::UnencryptedMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(socket.bytesAvailable(), qint64(0));
    QCOMPARE(socket.bytesToWrite(), qint64(0));
    QVERIFY(!socket.canReadLine());
    QVERIFY(socket.atEnd());
    QCOMPARE(socket.localCertificate(), QSslCertificate());
    QCOMPARE(socket.sslConfiguration(), QSslConfiguration::defaultConfiguration());
    QCOMPARE(socket.errorString(), QString("Unknown error"));
    char c = '\0';
    QTest::ignoreMessage(QtWarningMsg, readNotOpenMessage);
    QVERIFY(!socket.getChar(&c));
    QCOMPARE(c, '\0');
    QVERIFY(!socket.isOpen());
    QVERIFY(!socket.isReadable());
    QVERIFY(socket.isSequential());
    QVERIFY(!socket.isTextModeEnabled());
    QVERIFY(!socket.isWritable());
    QCOMPARE(socket.openMode(), QIODevice::NotOpen);
    QTest::ignoreMessage(QtWarningMsg, "QIODevice::peek (QSslSocket): device not open");
    QVERIFY(socket.peek(2).isEmpty());
    QCOMPARE(socket.pos(), qint64(0));
    QTest::ignoreMessage(QtWarningMsg, writeNotOpenMessage);
    QVERIFY(!socket.putChar('c'));
    QTest::ignoreMessage(QtWarningMsg, readNotOpenMessage);
    QVERIFY(socket.read(2).isEmpty());
    QTest::ignoreMessage(QtWarningMsg, readNotOpenMessage);
    QCOMPARE(socket.read(0, 0), qint64(-1));
    QTest::ignoreMessage(QtWarningMsg, readNotOpenMessage);
    QVERIFY(socket.readAll().isEmpty());
    QTest::ignoreMessage(QtWarningMsg, "QIODevice::readLine (QSslSocket): device not open");
    char buf[10];
    QCOMPARE(socket.readLine(buf, sizeof(buf)), qint64(-1));
    QTest::ignoreMessage(QtWarningMsg, "QIODevice::seek (QSslSocket): Cannot call seek on a sequential device");
    QVERIFY(!socket.reset());
    QTest::ignoreMessage(QtWarningMsg, "QIODevice::seek (QSslSocket): Cannot call seek on a sequential device");
    QVERIFY(!socket.seek(2));
    QCOMPARE(socket.size(), qint64(0));
    QVERIFY(!socket.waitForBytesWritten(10));
    QVERIFY(!socket.waitForReadyRead(10));
    QTest::ignoreMessage(QtWarningMsg, writeNotOpenMessage);
    QCOMPARE(socket.write(0, 0), qint64(-1));
    QTest::ignoreMessage(QtWarningMsg, writeNotOpenMessage);
    QCOMPARE(socket.write(QByteArray()), qint64(-1));
    QCOMPARE(socket.error(), QAbstractSocket::UnknownSocketError);
    QVERIFY(!socket.flush());
    QVERIFY(!socket.isValid());
    QCOMPARE(socket.localAddress(), QHostAddress());
    QCOMPARE(socket.localPort(), quint16(0));
    QCOMPARE(socket.peerAddress(), QHostAddress());
    QVERIFY(socket.peerName().isEmpty());
    QCOMPARE(socket.peerPort(), quint16(0));
#if QT_CONFIG(networkproxy)
    QCOMPARE(socket.proxy().type(), QNetworkProxy::DefaultProxy);
#endif
    QCOMPARE(socket.readBufferSize(), qint64(0));
    QCOMPARE(socket.socketDescriptor(), qintptr(-1));
    QCOMPARE(socket.socketType(), QAbstractSocket::TcpSocket);
    QVERIFY(!socket.waitForConnected(10));
    QTest::ignoreMessage(QtWarningMsg, "QSslSocket::waitForDisconnected() is not allowed in UnconnectedState");
    QVERIFY(!socket.waitForDisconnected(10));
    QCOMPARE(socket.protocol(), QSsl::SecureProtocols);

    QSslConfiguration savedDefault = QSslConfiguration::defaultConfiguration();

    auto sslConfig = socket.sslConfiguration();
    sslConfig.setCaCertificates(QSslConfiguration::systemCaCertificates());
    socket.setSslConfiguration(sslConfig);

    auto defaultConfig = QSslConfiguration::defaultConfiguration();
    defaultConfig.setCaCertificates(QList<QSslCertificate>());
    defaultConfig.setCiphers(QList<QSslCipher>());
    QSslConfiguration::setDefaultConfiguration(defaultConfig);

    QVERIFY(!socket.sslConfiguration().caCertificates().isEmpty());
    QVERIFY(!socket.sslConfiguration().ciphers().isEmpty());

    // verify the default as well:
    QVERIFY(QSslConfiguration::defaultConfiguration().caCertificates().isEmpty());
    QVERIFY(QSslConfiguration::defaultConfiguration().ciphers().isEmpty());

    QSslConfiguration::setDefaultConfiguration(savedDefault);
}

void tst_QSslSocket::configNoOnDemandLoad()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return; // NoProxy is enough.

    // We noticed a peculiar situation, where a configuration
    // set on a socket is not equal to the configuration we
    // get back from the socket afterwards.
    auto customConfig = QSslConfiguration::defaultConfiguration();
    // Setting CA certificates disables loading root certificates
    // during verification:
    customConfig.setCaCertificates(customConfig.caCertificates());

    QSslSocket socket;
    socket.setSslConfiguration(customConfig);
    QCOMPARE(customConfig, socket.sslConfiguration());
}

static void downgrade_TLS_QTQAINFRA_4499(QSslSocket &socket)
{
    // Set TLS 1.0 or above because the server doesn't support TLS 1.2 or above
    // QTQAINFRA-4499
    QSslConfiguration config = socket.sslConfiguration();
    config.setProtocol(Test::TlsV1_0OrLater);
    socket.setSslConfiguration(config);
}

void tst_QSslSocket::simpleConnect()
{
    if (!QSslSocket::supportsSsl())
        return;

    // Starting from OpenSSL v 3.1.1 deprecated protocol versions (we want to use when connecting) are not available by default.
    if (isSecurityLevel0Required)
        QSKIP("Testing with OpenSSL backend, but security level 0 is required for TLS v1.1 or earlier");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QSslSocket socket;

    downgrade_TLS_QTQAINFRA_4499(socket);

    QSignalSpy connectedSpy(&socket, SIGNAL(connected()));
    QSignalSpy hostFoundSpy(&socket, SIGNAL(hostFound()));
    QSignalSpy disconnectedSpy(&socket, SIGNAL(disconnected()));
    QSignalSpy connectionEncryptedSpy(&socket, SIGNAL(encrypted()));
    QSignalSpy sslErrorsSpy(&socket, SIGNAL(sslErrors(QList<QSslError>)));

    connect(&socket, SIGNAL(connected()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(disconnected()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(modeChanged(QSslSocket::SslMode)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(encrypted()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(exitLoop()));

    // Start connecting
    socket.connectToHost(QtNetworkSettings::imapServerName(), 993);
    QCOMPARE(socket.state(), QAbstractSocket::HostLookupState);
    enterLoop(10);

    // Entered connecting state
    QCOMPARE(socket.state(), QAbstractSocket::ConnectingState);
    QCOMPARE(connectedSpy.size(), 0);
    QCOMPARE(hostFoundSpy.size(), 1);
    QCOMPARE(disconnectedSpy.size(), 0);
    enterLoop(10);

    // Entered connected state
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socket.mode(), QSslSocket::UnencryptedMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectedSpy.size(), 1);
    QCOMPARE(hostFoundSpy.size(), 1);
    QCOMPARE(disconnectedSpy.size(), 0);

    // Enter encrypted mode
    socket.startClientEncryption();
    QCOMPARE(socket.mode(), QSslSocket::SslClientMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectionEncryptedSpy.size(), 0);
    QCOMPARE(sslErrorsSpy.size(), 0);

    // Starting handshake
    enterLoop(10);
    QCOMPARE(sslErrorsSpy.size(), 1);
    QCOMPARE(connectionEncryptedSpy.size(), 0);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(socket.state(), QAbstractSocket::UnconnectedState);
}

void tst_QSslSocket::simpleConnectWithIgnore()
{
    if (!QSslSocket::supportsSsl())
        return;

    // Starting from OpenSSL v 3.1.1 deprecated protocol versions (we want to use when connecting) are not available by default.
    if (isSecurityLevel0Required)
        QSKIP("Testing with OpenSSL backend, but security level 0 is required for TLS v1.1 or earlier");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QSslSocket socket;
    this->socket = &socket;
    QSignalSpy encryptedSpy(&socket, SIGNAL(encrypted()));
    QSignalSpy sslErrorsSpy(&socket, SIGNAL(sslErrors(QList<QSslError>)));

    downgrade_TLS_QTQAINFRA_4499(socket);

    connect(&socket, SIGNAL(readyRead()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(encrypted()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(connected()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(exitLoop()));

    // Start connecting
    socket.connectToHost(QtNetworkSettings::imapServerName(), 993);
    QVERIFY(socket.state() != QAbstractSocket::UnconnectedState); // something must be in progress
    enterLoop(10);

    // Start handshake
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    socket.startClientEncryption();
    enterLoop(10);

    // Done; encryption should be enabled.
    QCOMPARE(sslErrorsSpy.size(), 1);
    QVERIFY(socket.isEncrypted());
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(encryptedSpy.size(), 1);

    // Wait for incoming data
    if (!socket.canReadLine())
        enterLoop(10);

    QByteArray data = socket.readAll();
    socket.disconnectFromHost();
    QVERIFY2(QtNetworkSettings::compareReplyIMAPSSL(data), data.constData());
}

void tst_QSslSocket::sslErrors_data()
{
    // Starting from OpenSSL v 3.1.1 deprecated protocol versions (we want to use in 'sslErrors' test) are not available by default.
    if (isSecurityLevel0Required)
        QSKIP("Testing with OpenSSL backend, but security level 0 is required for TLS v1.1 or earlier");

    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");

    QString name = QtNetworkSettings::serverLocalName();
    QTest::newRow(qPrintable(name)) << name << 993;

    name = QtNetworkSettings::httpServerIp().toString();
    QTest::newRow(qPrintable(name)) << name << 443;
}

void tst_QSslSocket::sslErrors()
{
    QFETCH(QString, host);
    QFETCH(int, port);

    QSslSocketPtr socket = newSocket();

    QVERIFY(socket);
    downgrade_TLS_QTQAINFRA_4499(*socket);

    QSignalSpy sslErrorsSpy(socket.data(), SIGNAL(sslErrors(QList<QSslError>)));
    QSignalSpy peerVerifyErrorSpy(socket.data(), SIGNAL(peerVerifyError(QSslError)));

#ifdef QT_TEST_SERVER
    // On the old test server we had the same certificate on different services.
    // The idea of this test is to fail with 'HostNameMismatch', when we're using
    // either serverLocalName() or IP address directly. With Docker we connect
    // to IMAP server, and we have to connect using imapServerName() and passing
    // 'host' as peerVerificationName to the overload of connectToHostEncrypted().
    if (port == 993) {
        socket->connectToHostEncrypted(QtNetworkSettings::imapServerName(), port, host);
    } else
#endif // QT_TEST_SERVER
    {
        socket->connectToHostEncrypted(host, port);
    }

    if (!socket->waitForConnected())
        QSKIP("Skipping flaky test - See QTBUG-29941");
    socket->waitForEncrypted(10000);

    // check the SSL errors contain HostNameMismatch and an error due to
    // the certificate being self-signed
    SslErrorList sslErrors;
    const auto socketSslErrors = socket->sslHandshakeErrors();
    for (const QSslError &err : socketSslErrors)
        sslErrors << err.error();
    std::sort(sslErrors.begin(), sslErrors.end());
    QVERIFY(sslErrors.contains(QSslError::HostNameMismatch));

    // Non-OpenSSL backends are not able to report a specific error code
    // for self-signed certificates.
    QVERIFY(sslErrors.contains(flukeCertificateError));

    // check the same errors were emitted by sslErrors
    QVERIFY(!sslErrorsSpy.isEmpty());
    SslErrorList emittedErrors;
    const auto sslErrorsSpyErrors = qvariant_cast<QList<QSslError> >(std::as_const(sslErrorsSpy).first().first());
    for (const QSslError &err : sslErrorsSpyErrors)
        emittedErrors << err.error();
    std::sort(emittedErrors.begin(), emittedErrors.end());
    QCOMPARE(sslErrors, emittedErrors);

    // check the same errors were emitted by peerVerifyError
    QVERIFY(!peerVerifyErrorSpy.isEmpty());
    SslErrorList peerErrors;
    const QList<QVariantList> &peerVerifyList = peerVerifyErrorSpy;
    for (const QVariantList &args : peerVerifyList)
        peerErrors << qvariant_cast<QSslError>(args.first()).error();
    std::sort(peerErrors.begin(), peerErrors.end());
    QCOMPARE(sslErrors, peerErrors);
}

void tst_QSslSocket::ciphers()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy) {
        // KISS(mart), we don't connect, no need to test the same thing
        // many times!
        return;
    }

    QSslSocket socket;
    QCOMPARE(socket.sslConfiguration().ciphers(), QSslConfiguration::defaultConfiguration().ciphers());

    auto sslConfig = socket.sslConfiguration();
    sslConfig.setCiphers(QList<QSslCipher>());
    socket.setSslConfiguration(sslConfig);
    QVERIFY(socket.sslConfiguration().ciphers().isEmpty());

    sslConfig.setCiphers(QSslConfiguration::defaultConfiguration().ciphers());
    socket.setSslConfiguration(sslConfig);
    QCOMPARE(socket.sslConfiguration().ciphers(), QSslConfiguration::defaultConfiguration().ciphers());

    sslConfig.setCiphers(QSslConfiguration::defaultConfiguration().ciphers());
    socket.setSslConfiguration(sslConfig);
    QCOMPARE(socket.sslConfiguration().ciphers(), QSslConfiguration::defaultConfiguration().ciphers());

    sslConfig = QSslConfiguration::defaultConfiguration();
    QList<QSslCipher> ciphers;
    QString ciphersAsString;
    const auto &supported = sslConfig.supportedCiphers();
    for (const auto &cipher : supported) {
        if (cipher.isNull() || !cipher.name().size())
            continue;
        if (ciphers.size() > 0)
            ciphersAsString += QStringLiteral(":");
        ciphersAsString += cipher.name();
        ciphers.append(cipher);
        if (ciphers.size() == 3) // 3 should be enough.
            break;
    }

    if (!ciphers.size())
        QSKIP("No proper ciphersuite was found to test 'setCiphers'");


    if (isTestingSchannel) {
        qWarning("Schannel doesn't support setting ciphers from a cipher-string.");
    } else {
        sslConfig.setCiphers(ciphersAsString);
        socket.setSslConfiguration(sslConfig);
        QCOMPARE(ciphers, socket.sslConfiguration().ciphers());
    }

    sslConfig.setCiphers(ciphers);
    socket.setSslConfiguration(sslConfig);
    QCOMPARE(ciphers, socket.sslConfiguration().ciphers());

    if (isTestingOpenSsl) {
        for (const auto &cipher : ciphers) {
            if (cipher.name().size() && cipher.protocol() != QSsl::UnknownProtocol) {
                const QSslCipher aCopy(cipher.name(), cipher.protocol());
                QCOMPARE(aCopy, cipher);
                break;
            }
        }
    }
}

#if QT_CONFIG(securetransport)
void tst_QSslSocket::tls13Ciphers()
{
    // SecureTransport introduced several new ciphers under
    // "TLS 1.3 ciphersuites" section. Since Qt 6 we respect
    // the ciphers from QSslConfiguration. In case of default
    // configuration, these are the same we report and we
    // were failing (for historical reasons) to report those
    // TLS 1.3 suites when creating default QSslConfiguration.
    // Check we now have them.
    if (!isTestingSecureTransport)
        QSKIP("The feature 'securetransport' was enabled, but active backend is not \"securetransport\"");

    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy)
        return;

    const auto suites = QSslConfiguration::defaultConfiguration().ciphers();
    QSslCipher ciph;
    // Check the one of reported and previously missed:
    for (const auto &suite : suites) {
        if (suite.encryptionMethod() == QStringLiteral("CHACHA20")) {
            // There are several ciphesuites using CHACHA20, the first one
            // is sufficient for the purpose of this test:
            ciph = suite;
            break;
        }
    }

    QVERIFY(!ciph.isNull());
    QCOMPARE(ciph.encryptionMethod(), QStringLiteral("CHACHA20"));
    QCOMPARE(ciph.supportedBits(), 256);
    QCOMPARE(ciph.usedBits(), 256);
}
#endif // QT_CONFIG(securetransport)

void tst_QSslSocket::connectToHostEncrypted()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();

    this->socket = socket.data();
    auto config = socket->sslConfiguration();
    QVERIFY(config.addCaCertificates(httpServerCertChainPath()));
    socket->setSslConfiguration(config);
#ifdef QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    connect(socket.data(), SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(untrustedWorkaroundSlot(QList<QSslError>)));
#endif

    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);

    // This should pass unconditionally when using fluke's CA certificate.
    // or use untrusted certificate workaround
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    socket->disconnectFromHost();
    QVERIFY(socket->waitForDisconnected());
    QVERIFY(!socket->isEncrypted());

    QCOMPARE(socket->mode(), QSslSocket::SslClientMode);

    socket->connectToHost(QtNetworkSettings::echoServerName(), 13);

    QCOMPARE(socket->mode(), QSslSocket::UnencryptedMode);

    QVERIFY(socket->waitForDisconnected());
}

void tst_QSslSocket::connectToHostEncryptedWithVerificationPeerName()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();

    this->socket = socket.data();

    auto config = socket->sslConfiguration();
    config.addCaCertificates(httpServerCertChainPath());
    socket->setSslConfiguration(config);
#ifdef QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    connect(socket.data(), SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(untrustedWorkaroundSlot(QList<QSslError>)));
#endif

#ifdef QT_TEST_SERVER
    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443, QtNetworkSettings::httpServerName());
#else
    // Connect to the server with its local name, but use the full name for verification.
    socket->connectToHostEncrypted(QtNetworkSettings::serverLocalName(), 443, QtNetworkSettings::httpServerName());
#endif

    // This should pass unconditionally when using fluke's CA certificate.
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    socket->disconnectFromHost();
    QVERIFY(socket->waitForDisconnected());

    QCOMPARE(socket->mode(), QSslSocket::SslClientMode);
}

void tst_QSslSocket::sessionCipher()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();
    connect(socket.data(), SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    QVERIFY(socket->sessionCipher().isNull());
    socket->connectToHost(QtNetworkSettings::httpServerName(), 443 /* https */);
    QVERIFY2(socket->waitForConnected(10000), qPrintable(socket->errorString()));
    QVERIFY(socket->sessionCipher().isNull());
    socket->startClientEncryption();
    if (!socket->waitForEncrypted(5000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QVERIFY(!socket->sessionCipher().isNull());

    qDebug() << "Supported Ciphers:" << QSslConfiguration::supportedCiphers();
    qDebug() << "Default Ciphers:" << QSslConfiguration::defaultConfiguration().ciphers();
    qDebug() << "Session Cipher:" << socket->sessionCipher();

    QVERIFY(QSslConfiguration::supportedCiphers().contains(socket->sessionCipher()));
    socket->disconnectFromHost();
    QVERIFY(socket->waitForDisconnected());
}

void tst_QSslSocket::localCertificate()
{
    if (!QSslSocket::supportsSsl())
        return;

    // This test does not make 100% sense yet. We just set some local CA/cert/key and use it
    // to authenticate ourselves against the server. The server does not actually check this
    // values. This test should just run the codepath inside qsslsocket_openssl.cpp

    QSslSocketPtr socket = newSocket();
    QList<QSslCertificate> localCert = QSslCertificate::fromPath(httpServerCertChainPath());

    auto sslConfig = socket->sslConfiguration();
    sslConfig.setCaCertificates(localCert);
    socket->setSslConfiguration(sslConfig);

    socket->setLocalCertificate(testDataDir + "certs/fluke.cert");
    socket->setPrivateKey(testDataDir + "certs/fluke.key");

    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
}

void tst_QSslSocket::mode()
{
}

void tst_QSslSocket::peerCertificate()
{
}

void tst_QSslSocket::peerCertificateChain()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSKIP("QTBUG-29941 - Unstable auto-test due to intermittently unreachable host");

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();
    QList<QSslCertificate> caCertificates = QSslCertificate::fromPath(httpServerCertChainPath());
    QCOMPARE(caCertificates.size(), 1);
    auto config = socket->sslConfiguration();
    config.addCaCertificates(caCertificates);
    socket->setSslConfiguration(config);
#ifdef QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    connect(socket.data(), SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(untrustedWorkaroundSlot(QList<QSslError>)));
#endif

    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    QCOMPARE(socket->mode(), QSslSocket::UnencryptedMode);
    QVERIFY(socket->peerCertificateChain().isEmpty());
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    QList<QSslCertificate> certChain = socket->peerCertificateChain();
    QVERIFY(certChain.size() > 0);
    QCOMPARE(certChain.first(), socket->peerCertificate());

    socket->disconnectFromHost();
    QVERIFY(socket->waitForDisconnected());

    // connect again to a different server
    socket->connectToHostEncrypted("www.qt.io", 443);
    socket->ignoreSslErrors();
    QCOMPARE(socket->mode(), QSslSocket::UnencryptedMode);
    QVERIFY(socket->peerCertificateChain().isEmpty());
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    QCOMPARE(socket->peerCertificateChain().first(), socket->peerCertificate());
    QVERIFY(socket->peerCertificateChain() != certChain);

    socket->disconnectFromHost();
    QVERIFY(socket->waitForDisconnected());

    // now do it again back to the original server
    socket->connectToHost(QtNetworkSettings::httpServerName(), 443);
    QCOMPARE(socket->mode(), QSslSocket::UnencryptedMode);
    QVERIFY(socket->peerCertificateChain().isEmpty());
    QVERIFY2(socket->waitForConnected(10000), qPrintable(socket->errorString()));

    socket->startClientEncryption();
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    QCOMPARE(socket->peerCertificateChain().first(), socket->peerCertificate());
    QCOMPARE(socket->peerCertificateChain(), certChain);

    socket->disconnectFromHost();
    QVERIFY(socket->waitForDisconnected());
}

void tst_QSslSocket::privateKey()
{
}

#if QT_CONFIG(openssl)
void tst_QSslSocket::privateKeyOpaque()
{
#ifndef OPENSSL_NO_DEPRECATED_3_0
    if (!isTestingOpenSsl)
        QSKIP("The active TLS backend does not support private opaque keys");

    if (!opensslResolved)
        QSKIP("Failed to resolve OpenSSL symbols, required by this test");

    QFile file(testDataDir + "certs/fluke.key");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!key.isNull());

    EVP_PKEY *pkey = q_EVP_PKEY_new();
    q_EVP_PKEY_set1_RSA(pkey, reinterpret_cast<RSA *>(key.handle()));

    // This test does not make 100% sense yet. We just set some local CA/cert/key and use it
    // to authenticate ourselves against the server. The server does not actually check this
    // values. This test should just run the codepath inside qsslsocket_openssl.cpp

    QSslSocketPtr socket = newSocket();
    QList<QSslCertificate> localCert = QSslCertificate::fromPath(httpServerCertChainPath());

    auto sslConfig = socket->sslConfiguration();
    sslConfig.setCaCertificates(localCert);
    socket->setSslConfiguration(sslConfig);

    socket->setLocalCertificate(testDataDir + "certs/fluke.cert");
    socket->setPrivateKey(QSslKey(reinterpret_cast<Qt::HANDLE>(pkey)));

    socket->setPeerVerifyMode(QSslSocket::QueryPeer);
    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
#endif // OPENSSL_NO_DEPRECATED_3_0
}
#endif // Feature 'openssl'.

void tst_QSslSocket::protocol()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();
    QList<QSslCertificate> certs = QSslCertificate::fromPath(httpServerCertChainPath());

    auto sslConfig = socket->sslConfiguration();
    sslConfig.setCaCertificates(certs);
    socket->setSslConfiguration(sslConfig);

#ifdef QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(untrustedWorkaroundSlot(QList<QSslError>)));
#endif

    QCOMPARE(socket->protocol(), QSsl::SecureProtocols);
    QFETCH_GLOBAL(bool, setProxy);
    {
        // qt-test-server allows TLSV1.
        socket->setProtocol(Test::TlsV1_0);
        QCOMPARE(socket->protocol(), Test::TlsV1_0);
        socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), Test::TlsV1_0);
        socket->abort();
        QCOMPARE(socket->protocol(), Test::TlsV1_0);
        socket->connectToHost(QtNetworkSettings::httpServerName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), Test::TlsV1_0);
        socket->abort();
    }
    {
        // qt-test-server probably doesn't allow TLSV1.1
        socket->setProtocol(Test::TlsV1_1);
        QCOMPARE(socket->protocol(), Test::TlsV1_1);
        socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), Test::TlsV1_1);
        socket->abort();
        QCOMPARE(socket->protocol(), Test::TlsV1_1);
        socket->connectToHost(QtNetworkSettings::httpServerName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), Test::TlsV1_1);
        socket->abort();
    }
    {
        // qt-test-server probably doesn't allows TLSV1.2
        socket->setProtocol(QSsl::TlsV1_2);
        QCOMPARE(socket->protocol(), QSsl::TlsV1_2);
        socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1_2);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::TlsV1_2);
        socket->connectToHost(QtNetworkSettings::httpServerName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1_2);
        socket->abort();
    }

    if (supportsTls13()) {
        // qt-test-server probably doesn't allow TLSV1.3
        socket->setProtocol(QSsl::TlsV1_3);
        QCOMPARE(socket->protocol(), QSsl::TlsV1_3);
        socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
        if (!socket->waitForEncrypted(10'000))
            QSKIP("TLS 1.3 is not supported by the test server or the test is flaky - see QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1_3);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::TlsV1_3);
        socket->connectToHost(QtNetworkSettings::httpServerName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (!socket->waitForEncrypted(10'000))
            QSKIP("TLS 1.3 is not supported by the test server or the test is flaky - see QTBUG-29941");
        QCOMPARE(socket->sessionProtocol(), QSsl::TlsV1_3);
        socket->abort();
    }
    {
        // qt-test-server allows SSLV3, so it allows AnyProtocol.
        socket->setProtocol(QSsl::AnyProtocol);
        QCOMPARE(socket->protocol(), QSsl::AnyProtocol);
        socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::AnyProtocol);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::AnyProtocol);
        socket->connectToHost(QtNetworkSettings::httpServerName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::AnyProtocol);
        socket->abort();
    }
}

class SslServer : public QTcpServer
{
    Q_OBJECT
public:
    SslServer(const QString &keyFile = tst_QSslSocket::testDataDir + "certs/fluke.key",
              const QString &certFile = tst_QSslSocket::testDataDir + "certs/fluke.cert",
              const QString &interFile = QString())
        : socket(0),
          config(QSslConfiguration::defaultConfiguration()),
          ignoreSslErrors(true),
          peerVerifyMode(QSslSocket::AutoVerifyPeer),
          protocol(QSsl::SecureProtocols),
          m_keyFile(keyFile),
          m_certFile(certFile),
          m_interFile(interFile)
          { }
    QSslSocket *socket;
    QSslConfiguration config;
    QString addCaCertificates;
    bool ignoreSslErrors;
    QSslSocket::PeerVerifyMode peerVerifyMode;
    QSsl::SslProtocol protocol;
    QString m_keyFile;
    QString m_certFile;
    QString m_interFile;
    QList<QSslCipher> ciphers;

signals:
    void sslErrors(const QList<QSslError> &errors);
    void socketError(QAbstractSocket::SocketError);
    void handshakeInterruptedOnError(const QSslError& rrror);
    void gotAlert(QSsl::AlertLevel level, QSsl::AlertType type, const QString &message);
    void alertSent(QSsl::AlertLevel level, QSsl::AlertType type, const QString &message);
    void socketEncrypted(QSslSocket *);

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        QSslConfiguration configuration = config;
        socket = new QSslSocket(this);
        configuration.setPeerVerifyMode(peerVerifyMode);
        configuration.setProtocol(protocol);
        if (ignoreSslErrors)
            connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
        else
            connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SIGNAL(sslErrors(QList<QSslError>)));
        connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SIGNAL(socketError(QAbstractSocket::SocketError)));
        connect(socket, &QSslSocket::handshakeInterruptedOnError, this, &SslServer::handshakeInterruptedOnError);
        connect(socket, &QSslSocket::alertReceived, this, &SslServer::gotAlert);
        connect(socket, &QSslSocket::alertSent, this, &SslServer::alertSent);
        connect(socket, &QSslSocket::preSharedKeyAuthenticationRequired, this, &SslServer::preSharedKeyAuthenticationRequired);
        connect(socket, &QSslSocket::encrypted, this, [this](){ emit socketEncrypted(socket); });

        QFile file(m_keyFile);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull());
        configuration.setPrivateKey(key);

        // Add CA certificates to verify client certificate
        if (!addCaCertificates.isEmpty()) {
            QList<QSslCertificate> caCert = QSslCertificate::fromPath(addCaCertificates);
            QVERIFY(!caCert.isEmpty());
            QVERIFY(!caCert.first().isNull());
            configuration.addCaCertificates(caCert);
        }

        // If we have a cert issued directly from the CA
        if (m_interFile.isEmpty()) {
            QList<QSslCertificate> localCert = QSslCertificate::fromPath(m_certFile);
            QVERIFY(!localCert.isEmpty());
            QVERIFY(!localCert.first().isNull());
            configuration.setLocalCertificate(localCert.first());
        } else {
            QList<QSslCertificate> localCert = QSslCertificate::fromPath(m_certFile);
            QVERIFY(!localCert.isEmpty());
            QVERIFY(!localCert.first().isNull());

            QList<QSslCertificate> interCert = QSslCertificate::fromPath(m_interFile);
            QVERIFY(!interCert.isEmpty());
            QVERIFY(!interCert.first().isNull());

            configuration.setLocalCertificateChain(localCert + interCert);
        }

        if (!ciphers.isEmpty())
            configuration.setCiphers(ciphers);
        socket->setSslConfiguration(configuration);

        QVERIFY(socket->setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState));
        QVERIFY(!socket->peerAddress().isNull());
        QVERIFY(socket->peerPort() != 0);
        QVERIFY(!socket->localAddress().isNull());
        QVERIFY(socket->localPort() != 0);

        socket->startServerEncryption();
    }

protected slots:
    void preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *authenticator)
    {
        authenticator->setPreSharedKey("123456");
    }

    void ignoreErrorSlot()
    {
        socket->ignoreSslErrors();
    }
};

void tst_QSslSocket::protocolServerSide_data()
{
    QTest::addColumn<QSsl::SslProtocol>("serverProtocol");
    QTest::addColumn<QSsl::SslProtocol>("clientProtocol");
    QTest::addColumn<bool>("works");

    QTest::newRow("any-any") << QSsl::AnyProtocol << QSsl::AnyProtocol << true;
    QTest::newRow("secure-secure") << QSsl::SecureProtocols << QSsl::SecureProtocols << true;

    QTest::newRow("tls1.0-secure") << Test::TlsV1_0 << QSsl::SecureProtocols << false;
    QTest::newRow("secure-tls1.0") << QSsl::SecureProtocols << Test::TlsV1_0 << false;
    QTest::newRow("secure-any") << QSsl::SecureProtocols << QSsl::AnyProtocol << true;

    QTest::newRow("tls1.0orlater-tls1.2") << Test::TlsV1_0OrLater << QSsl::TlsV1_2 << true;
    if (supportsTls13())
        QTest::newRow("tls1.0orlater-tls1.3") << Test::TlsV1_0OrLater << QSsl::TlsV1_3 << true;

    QTest::newRow("tls1.1orlater-tls1.0") << Test::TlsV1_1OrLater << Test::TlsV1_0 << false;
    QTest::newRow("tls1.1orlater-tls1.2") << Test::TlsV1_1OrLater << QSsl::TlsV1_2 << true;

    if (supportsTls13())
        QTest::newRow("tls1.1orlater-tls1.3") << Test::TlsV1_1OrLater << QSsl::TlsV1_3 << true;

    QTest::newRow("tls1.2orlater-tls1.0") << QSsl::TlsV1_2OrLater << Test::TlsV1_0 << false;
    QTest::newRow("tls1.2orlater-tls1.1") << QSsl::TlsV1_2OrLater << Test::TlsV1_1 << false;
    QTest::newRow("tls1.2orlater-tls1.2") << QSsl::TlsV1_2OrLater << QSsl::TlsV1_2 << true;
    if (supportsTls13()) {
        QTest::newRow("tls1.2orlater-tls1.3") << QSsl::TlsV1_2OrLater << QSsl::TlsV1_3 << true;
        QTest::newRow("tls1.3orlater-tls1.0") << QSsl::TlsV1_3OrLater << Test::TlsV1_0 << false;
        QTest::newRow("tls1.3orlater-tls1.1") << QSsl::TlsV1_3OrLater << Test::TlsV1_1 << false;
        QTest::newRow("tls1.3orlater-tls1.2") << QSsl::TlsV1_3OrLater << QSsl::TlsV1_2 << false;
        QTest::newRow("tls1.3orlater-tls1.3") << QSsl::TlsV1_3OrLater << QSsl::TlsV1_3 << true;
    }

    QTest::newRow("any-secure") << QSsl::AnyProtocol << QSsl::SecureProtocols << true;
}

void tst_QSslSocket::protocolServerSide()
{
    if (!QSslSocket::supportsSsl()) {
        qWarning("SSL not supported, skipping test");
        return;
    }

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QFETCH(QSsl::SslProtocol, serverProtocol);
    SslServer server;
    server.protocol = serverProtocol;
    QVERIFY(server.listen());

    QEventLoop loop;
    connect(&server, SIGNAL(socketError(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSslSocket client;
    socket = &client;
    QFETCH(QSsl::SslProtocol, clientProtocol);
    socket->setProtocol(clientProtocol);
    // upon SSL wrong version error, errorOccurred will be triggered, not sslErrors
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();

    QFETCH(bool, works);
    QAbstractSocket::SocketState expectedState = (works) ? QAbstractSocket::ConnectedState : QAbstractSocket::UnconnectedState;
    // Determine whether the client or the server caused the event loop
    // to quit due to a socket error, and investigate the culprit.
    if (client.error() != QAbstractSocket::UnknownSocketError) {
        // It can happen that the client, after TCP connection established, before
        // incomingConnection() slot fired, hits TLS initialization error and stops
        // the loop, so the server socket is not created yet.
        if (server.socket)
            QVERIFY(server.socket->error() == QAbstractSocket::UnknownSocketError);

        QCOMPARE(client.state(), expectedState);
    } else if (server.socket->error() != QAbstractSocket::UnknownSocketError) {
        QVERIFY(client.error() == QAbstractSocket::UnknownSocketError);
        QCOMPARE(server.socket->state(), expectedState);
    }

    QCOMPARE(client.isEncrypted(), works);
}

#if QT_CONFIG(openssl)

void tst_QSslSocket::serverCipherPreferences()
{
    if (!isTestingOpenSsl)
        QSKIP("The active TLS backend does not support server-side cipher preferences");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QSslCipher testedCiphers[2];
    {
        // First using the default (server preference)
        const auto supportedCiphers = QSslConfiguration::supportedCiphers();
        int nSet = 0;
        for (const auto &cipher : supportedCiphers) {
            // Ciphersuites from TLS 1.2 and 1.3 are set separately,
            // let's select 1.3 or above explicitly.
            if (cipher.protocol() < QSsl::TlsV1_3)
                continue;

            testedCiphers[nSet++] = cipher;
            if (nSet == 2)
                break;
        }

        if (nSet != 2)
            QSKIP("Failed to find two proper ciphersuites to test, bailing out.");

        SslServer server;
        server.protocol = QSsl::TlsV1_2OrLater;
        server.ciphers = {testedCiphers[0], testedCiphers[1]};
        QVERIFY(server.listen());

        QEventLoop loop;
        QTimer::singleShot(5000, &loop, SLOT(quit()));

        QSslSocket client;
        socket = &client;

        auto sslConfig = socket->sslConfiguration();
        sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
        sslConfig.setCiphers({testedCiphers[1], testedCiphers[0]});
        socket->setSslConfiguration(sslConfig);

        // upon SSL wrong version error, errorOccurred will be triggered, not sslErrors
        connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
        connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
        connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

        client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

        loop.exec();

        QVERIFY(client.isEncrypted());
        QCOMPARE(client.sessionCipher().name(), testedCiphers[0].name());
    }

    {
        if (QTestPrivate::isRunningArmOnX86())
            QSKIP("This test is known to crash on QEMU emulation for no good reason.");
        // Now using the client preferences
        SslServer server;
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        config.setSslOption(QSsl::SslOptionDisableServerCipherPreference, true);
        server.config = config;
        server.protocol = QSsl::TlsV1_2OrLater;
        server.ciphers = {testedCiphers[0], testedCiphers[1]};
        QVERIFY(server.listen());

        QEventLoop loop;
        QTimer::singleShot(5000, &loop, SLOT(quit()));

        QSslSocket client;
        socket = &client;

        auto sslConfig = socket->sslConfiguration();
        sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
        sslConfig.setCiphers({testedCiphers[1], testedCiphers[0]});
        socket->setSslConfiguration(sslConfig);

        // upon SSL wrong version error, errorOccurred will be triggered, not sslErrors
        connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
        connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
        connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

        client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

        loop.exec();

        QVERIFY(client.isEncrypted());
        QCOMPARE(client.sessionCipher().name(), testedCiphers[1].name());
    }
}

#endif // Feature 'openssl'.


void tst_QSslSocket::setCaCertificates()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocket socket;
    QCOMPARE(socket.sslConfiguration().caCertificates(),
             QSslConfiguration::defaultConfiguration().caCertificates());

    auto sslConfig = socket.sslConfiguration();
    sslConfig.setCaCertificates(
                QSslCertificate::fromPath(testDataDir + "certs/qt-test-server-cacert.pem"));
    socket.setSslConfiguration(sslConfig);
    QCOMPARE(socket.sslConfiguration().caCertificates().size(), 1);

    sslConfig.setCaCertificates(QSslConfiguration::defaultConfiguration().caCertificates());
    socket.setSslConfiguration(sslConfig);
    QCOMPARE(socket.sslConfiguration().caCertificates(),
             QSslConfiguration::defaultConfiguration().caCertificates());
}

void tst_QSslSocket::setLocalCertificate()
{
}

void tst_QSslSocket::localCertificateChain()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocket socket;
    socket.setLocalCertificate(testDataDir + "certs/fluke.cert");

    QSslConfiguration conf = socket.sslConfiguration();
    QList<QSslCertificate> chain = conf.localCertificateChain();
    QCOMPARE(chain.size(), 1);
    QCOMPARE(chain[0], conf.localCertificate());
    QCOMPARE(chain[0], socket.localCertificate());
}

void tst_QSslSocket::setLocalCertificateChain()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server(testDataDir + "certs/leaf.key",
                     testDataDir + "certs/leaf.crt",
                     testDataDir + "certs/inter.crt");

    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    const QScopedPointer<QSslSocket, QScopedPointerDeleteLater> client(new QSslSocket);
    socket = client.data();
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));

    socket->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());
    loop.exec();

    QList<QSslCertificate> chain = socket->peerCertificateChain();
    if (isTestingSchannel) {
        QEXPECT_FAIL("", "Schannel cannot send intermediate certificates not "
                         "located in a system certificate store",
                     Abort);
    }

    QCOMPARE(chain.size(), 2);
    QCOMPARE(chain[0].serialNumber(),
             QByteArray("58:df:33:c1:9b:bc:c5:51:7a:00:86:64:43:94:41:e2:26:ef:3f:89"));
    QCOMPARE(chain[1].serialNumber(),
             QByteArray("11:72:34:bc:21:e6:ca:04:24:13:f8:35:48:84:a6:e9:de:96:22:15"));
}

void tst_QSslSocket::tlsConfiguration()
{
    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy)
        return;
    // Test some things not covered by any other auto-test.
    QSslSocket socket;
    auto tlsConfig = socket.sslConfiguration();
    QVERIFY(tlsConfig.sessionCipher().isNull());
    QCOMPARE(tlsConfig.addCaCertificates(QStringLiteral("nonexisting/chain.crt")), false);
    QCOMPARE(tlsConfig.sessionProtocol(), QSsl::UnknownProtocol);
    QSslConfiguration nullConfig;
    QVERIFY(nullConfig.isNull());
#if QT_CONFIG(openssl)
    nullConfig.setEllipticCurves(tlsConfig.ellipticCurves());
    QCOMPARE(nullConfig.ellipticCurves(), tlsConfig.ellipticCurves());
#endif
    QMap<QByteArray, QVariant> backendConfig;
    backendConfig["DTLSMTU"] = QVariant::fromValue(1024);
    backendConfig["DTLSTIMEOUTMS"] = QVariant::fromValue(1000);
    nullConfig.setBackendConfiguration(backendConfig);
    QCOMPARE(nullConfig.backendConfiguration(), backendConfig);
    QTest::ignoreMessage(QtWarningMsg, "QSslConfiguration::setPeerVerifyDepth: cannot set negative depth of -1000");
    nullConfig.setPeerVerifyDepth(-1000);
    QVERIFY(nullConfig.peerVerifyDepth() != -1000);
    nullConfig.setPeerVerifyDepth(100);
    QCOMPARE(nullConfig.peerVerifyDepth(), 100);
}

void tst_QSslSocket::setSocketDescriptor()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSslSocket client;
    socket = &client;
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();

    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    QVERIFY(client.isEncrypted());
    QVERIFY(!client.peerAddress().isNull());
    QVERIFY(client.peerPort() != 0);
    QVERIFY(!client.localAddress().isNull());
    QVERIFY(client.localPort() != 0);
}

void tst_QSslSocket::setSslConfiguration_data()
{
    QTest::addColumn<QSslConfiguration>("configuration");
    QTest::addColumn<bool>("works");

    QTest::newRow("empty") << QSslConfiguration() << false;
    QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
    QTest::newRow("default") << conf << false; // does not contain test server cert

    if (!isTestingSecureTransport) {
        QList<QSslCertificate> testServerCert = QSslCertificate::fromPath(httpServerCertChainPath());
        conf.setCaCertificates(testServerCert);
        QTest::newRow("set-root-cert") << conf << true;
        conf.setProtocol(QSsl::SecureProtocols);
        QTest::newRow("secure") << conf << true;
    } else {
        qWarning("Skipping the cases with certificate, SecureTransport does not like old certificate on the test server");
    }
}

void tst_QSslSocket::setSslConfiguration()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    QFETCH(QSslConfiguration, configuration);
    socket->setSslConfiguration(configuration);

    this->socket = socket.data();
    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    QFETCH(bool, works);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && (socket->waitForEncrypted(10000) != works))
        QSKIP("Skipping flaky test - See QTBUG-29941");
    if (works) {
        socket->disconnectFromHost();
        QVERIFY2(socket->waitForDisconnected(), qPrintable(socket->errorString()));
    }
}

void tst_QSslSocket::waitForEncrypted()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();

    connect(this->socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
}

void tst_QSslSocket::waitForEncryptedMinusOne()
{
#ifdef Q_OS_WIN
    QSKIP("QTBUG-24451 - indefinite wait may hang");
#endif
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();

    connect(this->socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(-1))
        QSKIP("Skipping flaky test - See QTBUG-29941");
}

void tst_QSslSocket::waitForConnectedEncryptedReadyRead()
{
    if (!QSslSocket::supportsSsl())
        return;

    // Starting from OpenSSL v 3.1.1 deprecated protocol versions (we want to use here) are not available by default.
    if (isSecurityLevel0Required)
        QSKIP("Testing with OpenSSL backend, but security level 0 is required for TLS v1.1 or earlier");

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();

    // Set TLS 1.0 or above because the server doesn't support TLS 1.2 or above
    // QTQAINFRA-4499
    QSslConfiguration config = socket->sslConfiguration();
    config.setProtocol(Test::TlsV1_0OrLater);
    socket->setSslConfiguration(config);

    connect(this->socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    socket->connectToHostEncrypted(QtNetworkSettings::imapServerName(), 993);

    QVERIFY2(socket->waitForConnected(10000), qPrintable(socket->errorString()));
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    // We only do this if we have no bytes available to read already because readyRead will
    // not be emitted again.
    if (socket->bytesAvailable() == 0)
        QVERIFY(socket->waitForReadyRead(10000));

    QVERIFY(!socket->peerCertificate().isNull());
    QVERIFY(!socket->peerCertificateChain().isEmpty());
}

void tst_QSslSocket::startClientEncryption()
{
}

void tst_QSslSocket::startServerEncryption()
{
}

void tst_QSslSocket::addDefaultCaCertificate()
{
    if (!QSslSocket::supportsSsl())
        return;

    // Reset the global CA chain
    auto sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setCaCertificates(QSslConfiguration::systemCaCertificates());
    QSslConfiguration::setDefaultConfiguration(sslConfig);

    QList<QSslCertificate> flukeCerts = QSslCertificate::fromPath(httpServerCertChainPath());
    QCOMPARE(flukeCerts.size(), 1);
    QList<QSslCertificate> globalCerts = QSslConfiguration::defaultConfiguration().caCertificates();
    QVERIFY(!globalCerts.contains(flukeCerts.first()));
    sslConfig.addCaCertificate(flukeCerts.first());
    QSslConfiguration::setDefaultConfiguration(sslConfig);
    QCOMPARE(QSslConfiguration::defaultConfiguration().caCertificates().size(),
             globalCerts.size() + 1);
    QVERIFY(QSslConfiguration::defaultConfiguration().caCertificates()
            .contains(flukeCerts.first()));

    // Restore the global CA chain
    sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setCaCertificates(QSslConfiguration::systemCaCertificates());
    QSslConfiguration::setDefaultConfiguration(sslConfig);
}

void tst_QSslSocket::defaultCaCertificates()
{
    if (!QSslSocket::supportsSsl())
        return;

    QList<QSslCertificate> certs = QSslConfiguration::defaultConfiguration().caCertificates();
    QVERIFY(certs.size() > 1);
    QCOMPARE(certs, QSslConfiguration::systemCaCertificates());
}

void tst_QSslSocket::defaultCiphers()
{
    if (!QSslSocket::supportsSsl())
        return;

    QList<QSslCipher> ciphers = QSslConfiguration::defaultConfiguration().ciphers();
    QVERIFY(ciphers.size() > 1);

    QSslSocket socket;
    QCOMPARE(socket.sslConfiguration().defaultConfiguration().ciphers(), ciphers);
    QCOMPARE(socket.sslConfiguration().ciphers(), ciphers);
}

void tst_QSslSocket::resetDefaultCiphers()
{
}

void tst_QSslSocket::setDefaultCaCertificates()
{
}

void tst_QSslSocket::setDefaultCiphers()
{
}

void tst_QSslSocket::supportedCiphers()
{
    if (!QSslSocket::supportsSsl())
        return;

    QList<QSslCipher> ciphers = QSslConfiguration::supportedCiphers();
    QVERIFY(ciphers.size() > 1);

    QSslSocket socket;
    QCOMPARE(socket.sslConfiguration().supportedCiphers(), ciphers);
}

void tst_QSslSocket::systemCaCertificates()
{
    if (!QSslSocket::supportsSsl())
        return;

    QList<QSslCertificate> certs = QSslConfiguration::systemCaCertificates();
    QVERIFY(certs.size() > 1);
    QCOMPARE(certs, QSslConfiguration::defaultConfiguration().systemCaCertificates());
}

void tst_QSslSocket::wildcardCertificateNames()
{
    // Passing CN matches
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("www.example.com"), QString("www.example.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("WWW.EXAMPLE.COM"), QString("www.example.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.example.com"), QString("www.example.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("xxx*.example.com"), QString("xxxwww.example.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("f*.example.com"), QString("foo.example.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("192.168.0.0"), QString("192.168.0.0")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("foo.éxample.com"), QString("foo.xn--xample-9ua.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.éxample.com"), QString("foo.xn--xample-9ua.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("xn--kcry6tjko.example.org"), QString("xn--kcry6tjko.example.org")), true);
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.xn--kcry6tjko.example.org"), QString("xn--kcr.xn--kcry6tjko.example.org")), true);

    // Failing CN matches
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("xxx.example.com"), QString("www.example.com")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*"), QString("www.example.com")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.*.com"), QString("www.example.com")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.example.com"), QString("baa.foo.example.com")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("f*.example.com"), QString("baa.example.com")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.com"), QString("example.com")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*fail.com"), QString("example.com")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.example."), QString("www.example.")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.example."), QString("www.example")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString(""), QString("www")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*"), QString("www")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.168.0.0"), QString("192.168.0.0")), false );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("xn--kcry6tjko*.example.org"), QString("xn--kcry6tjkoanc.example.org")), false );  // RFC 6125 §7.2
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("å*.example.org"), QString("xn--la-xia.example.org")), false );
}

void tst_QSslSocket::isMatchingHostname()
{
    // with normalization:  (the certificate has *.SCHÄUFELE.DE as a CN)
    // openssl req -x509 -nodes -subj "/CN=*.SCHÄUFELE.DE" -newkey rsa:512 -keyout /dev/null -out xn--schufele-2za.crt
    QList<QSslCertificate> certs = QSslCertificate::fromPath(testDataDir + "certs/xn--schufele-2za.crt");
    QVERIFY(!certs.isEmpty());
    QSslCertificate cert = certs.first();

    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("WWW.SCHÄUFELE.DE")), true);
    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("www.xn--schufele-2za.de")), true);
    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("www.schäufele.de")), true);
    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("föo.schäufele.de")), true);

    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("foo.foo.xn--schufele-2za.de")), false);
    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("www.schaufele.de")), false);
    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("www.schufele.de")), false);

    /* Generated with the following command (only valid with openssl >= 1.1.1 due to "-addext"):
       openssl req -x509 -nodes -subj "/CN=example.org" \
            -addext "subjectAltName = IP:192.5.8.16, IP:fe80::3c29:2fa1:dd44:765" \
            -newkey rsa:2048 -keyout /dev/null -out subjectAltNameIP.crt
    */
    certs = QSslCertificate::fromPath(testDataDir + "certs/subjectAltNameIP.crt");
    QVERIFY(!certs.isEmpty());
    cert = certs.first();
    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("192.5.8.16")), true);
    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("fe80::3c29:2fa1:dd44:765")), true);

    /* openssl req -x509 -nodes -new -newkey rsa -keyout /dev/null -out 127-0-0-1-as-CN.crt \
            -subj "/CN=127.0.0.1"
    */
    certs = QSslCertificate::fromPath(testDataDir + "certs/127-0-0-1-as-CN.crt");
    QVERIFY(!certs.isEmpty());
    cert = certs.first();
    QCOMPARE(QSslSocketPrivate::isMatchingHostname(cert, QString::fromUtf8("127.0.0.1")), true);
}

void tst_QSslSocket::wildcard()
{
    QSKIP("TODO: solve wildcard problem");

    if (!QSslSocket::supportsSsl())
        return;

    // Fluke runs an apache server listening on port 4443, serving the
    // wildcard fluke.*.troll.no.  The DNS entry for
    // fluke.wildcard.dev.troll.no, served by ares (root for dev.troll.no),
    // returns the CNAME fluke.troll.no for this domain. The web server
    // responds with the wildcard, and QSslSocket should accept that as a
    // valid connection.  This was broken in 4.3.0.
    QSslSocketPtr socket = newSocket();
    auto config = socket->sslConfiguration();
    config.addCaCertificates(QLatin1String("certs/aspiriniks.ca.crt"));
    socket->setSslConfiguration(config);
    this->socket = socket.data();
#ifdef QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(untrustedWorkaroundSlot(QList<QSslError>)));
#endif
    socket->connectToHostEncrypted(QtNetworkSettings::wildcardServerName(), 4443);

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(3000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    QSslCertificate certificate = socket->peerCertificate();
    QVERIFY(certificate.subjectInfo(QSslCertificate::CommonName).contains(QString(QtNetworkSettings::serverLocalName() + ".*." + QtNetworkSettings::serverDomainName())));
    QVERIFY(certificate.issuerInfo(QSslCertificate::CommonName).contains(QtNetworkSettings::serverName()));

    socket->close();
}

class SslServer2 : public QTcpServer
{
protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        QSslSocket *socket = new QSslSocket(this);
        socket->ignoreSslErrors();

        // Only set the certificate
        QList<QSslCertificate> localCert = QSslCertificate::fromPath(tst_QSslSocket::testDataDir + "certs/fluke.cert");
        QVERIFY(!localCert.isEmpty());
        QVERIFY(!localCert.first().isNull());
        socket->setLocalCertificate(localCert.first());

        QVERIFY(socket->setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState));

        socket->startServerEncryption();
    }
};

void tst_QSslSocket::setEmptyKey()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer2 server;
    server.listen();

    QSslSocket socket;
    socket.connectToHostEncrypted("127.0.0.1", server.serverPort());

    QTestEventLoop::instance().enterLoop(2);

    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socket.error(), QAbstractSocket::UnknownSocketError);
}

void tst_QSslSocket::spontaneousWrite()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    QSslSocket *receiver = new QSslSocket(this);
    connect(receiver, SIGNAL(readyRead()), SLOT(exitLoop()));

    // connect two sockets to each other:
    QVERIFY(server.listen(QHostAddress::LocalHost));
    receiver->connectToHost("127.0.0.1", server.serverPort());
    QVERIFY(receiver->waitForConnected(5000));
    QVERIFY(server.waitForNewConnection(0));

    QSslSocket *sender = server.socket;
    QVERIFY(sender);
    QCOMPARE(sender->state(), QAbstractSocket::ConnectedState);
    receiver->setObjectName("receiver");
    sender->setObjectName("sender");
    receiver->ignoreSslErrors();
    receiver->startClientEncryption();

    // SSL handshake:
    // Need to wait for both sides to emit encrypted as the ordering of which
    // ones emits encrypted() changes depending on whether we use TLS 1.2 or 1.3
    int waitFor = 2;
    auto earlyQuitter = [&waitFor]() {
        if (!--waitFor)
            exitLoop();
    };
    connect(receiver, &QSslSocket::encrypted, this, earlyQuitter);
    connect(sender, &QSslSocket::encrypted, this, earlyQuitter);
    enterLoop(1);

    QVERIFY(!timeout());
    QVERIFY(sender->isEncrypted());
    QVERIFY(receiver->isEncrypted());

    // make sure there's nothing to be received on the sender:
    while (sender->waitForReadyRead(10) || receiver->waitForBytesWritten(10)) {}

    // spontaneously write something:
    QByteArray data("Hello World");
    sender->write(data);

    // check if the other side receives it:
    enterLoop(1);
    QVERIFY(!timeout());
    QCOMPARE(receiver->bytesAvailable(), qint64(data.size()));
    QCOMPARE(receiver->readAll(), data);
}

void tst_QSslSocket::setReadBufferSize()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    QSslSocket *receiver = new QSslSocket(this);
    connect(receiver, SIGNAL(readyRead()), SLOT(exitLoop()));

    // connect two sockets to each other:
    QVERIFY(server.listen(QHostAddress::LocalHost));
    receiver->connectToHost("127.0.0.1", server.serverPort());
    QVERIFY(receiver->waitForConnected(5000));
    QVERIFY(server.waitForNewConnection(0));

    QSslSocket *sender = server.socket;
    QVERIFY(sender);
    QCOMPARE(sender->state(), QAbstractSocket::ConnectedState);
    receiver->setObjectName("receiver");
    sender->setObjectName("sender");
    receiver->ignoreSslErrors();
    receiver->startClientEncryption();

    // Need to wait for both sides to emit encrypted as the ordering of which
    // ones emits encrypted() changes depending on whether we use TLS 1.2 or 1.3
    int waitFor = 2;
    auto earlyQuitter = [&waitFor]() {
        if (!--waitFor)
            exitLoop();
    };
    connect(receiver, &QSslSocket::encrypted, this, earlyQuitter);
    connect(sender, &QSslSocket::encrypted, this, earlyQuitter);

    enterLoop(1);
    if (!sender->isEncrypted()) {
        connect(sender, &QSslSocket::encrypted, this, &tst_QSslSocket::exitLoop);
        enterLoop(1);
    }
    QVERIFY(!timeout());
    QVERIFY(sender->isEncrypted());
    QVERIFY(receiver->isEncrypted());

    QByteArray data(2048, 'b');
    receiver->setReadBufferSize(39 * 1024); // make it a non-multiple of the data.size()

    // saturate the incoming buffer
    while (sender->state() == QAbstractSocket::ConnectedState &&
           receiver->state() == QAbstractSocket::ConnectedState &&
           receiver->bytesAvailable() < receiver->readBufferSize()) {
        sender->write(data);
        //qDebug() << receiver->bytesAvailable() << "<" << receiver->readBufferSize() << (receiver->bytesAvailable() < receiver->readBufferSize());

        while (sender->bytesToWrite())
            QVERIFY(sender->waitForBytesWritten(10));

        // drain it:
        while (receiver->bytesAvailable() < receiver->readBufferSize() &&
               receiver->waitForReadyRead(10)) {}
    }

    //qDebug() << sender->bytesToWrite() << "bytes to write";
    //qDebug() << receiver->bytesAvailable() << "bytes available";

    // send a bit more
    sender->write(data);
    sender->write(data);
    sender->write(data);
    sender->write(data);
    QVERIFY(sender->waitForBytesWritten(10));

    qint64 oldBytesAvailable = receiver->bytesAvailable();

    // now unset the read buffer limit and iterate
    receiver->setReadBufferSize(0);
    enterLoop(1);
    QVERIFY(!timeout());

    QVERIFY(receiver->bytesAvailable() > oldBytesAvailable);
}

class SetReadBufferSize_task_250027_handler : public QObject {
    Q_OBJECT
public slots:
    void readyReadSlot() {
        QTestEventLoop::instance().exitLoop();
    }
    void waitSomeMore(QSslSocket *socket) {
        QElapsedTimer t;
        t.start();
        while (!socket->encryptedBytesAvailable()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 250);
            if (t.elapsed() > 1000 || socket->state() != QAbstractSocket::ConnectedState)
                return;
        }
    }
};

void tst_QSslSocket::setReadBufferSize_task_250027()
{
    QSKIP("QTBUG-29730 - flakey test blocking integration");

    // do not execute this when a proxy is set.
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QSslSocketPtr socket = newSocket();
    socket->setReadBufferSize(1000); // limit to 1 kb/sec
    socket->ignoreSslErrors();
    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    socket->ignoreSslErrors();
    QVERIFY2(socket->waitForConnected(10*1000), qPrintable(socket->errorString()));
    if (setProxy && !socket->waitForEncrypted(10*1000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    // exit the event loop as soon as we receive a readyRead()
    SetReadBufferSize_task_250027_handler setReadBufferSize_task_250027_handler;
    connect(socket.data(), SIGNAL(readyRead()), &setReadBufferSize_task_250027_handler, SLOT(readyReadSlot()));

    // provoke a response by sending a request
    socket->write("GET /qtest/fluke.gif HTTP/1.0\n"); // this file is 27 KB
    socket->write("Host: ");
    socket->write(QtNetworkSettings::httpServerName().toLocal8Bit().constData());
    socket->write("\n");
    socket->write("Connection: close\n");
    socket->write("\n");
    socket->flush();

    QTestEventLoop::instance().enterLoop(10);
    setReadBufferSize_task_250027_handler.waitSomeMore(socket.data());
    QByteArray firstRead = socket->readAll();
    // First read should be some data, but not the whole file
    QVERIFY(firstRead.size() > 0 && firstRead.size() < 20*1024);

    QTestEventLoop::instance().enterLoop(10);
    setReadBufferSize_task_250027_handler.waitSomeMore(socket.data());
    QByteArray secondRead = socket->readAll();
    // second read should be some more data

    int secondReadSize = secondRead.size();

    if (secondReadSize <= 0) {
        QEXPECT_FAIL("", "QTBUG-29730", Continue);
    }

    QVERIFY(secondReadSize > 0);

    socket->close();
}

class SslServer3 : public QTcpServer
{
    Q_OBJECT
public:
    SslServer3() : socket(0) { }
    QSslSocket *socket;

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        socket = new QSslSocket(this);
        connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));

        QFile file(tst_QSslSocket::testDataDir + "certs/fluke.key");
        QVERIFY(file.open(QIODevice::ReadOnly));
        QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull());
        socket->setPrivateKey(key);

        QList<QSslCertificate> localCert = QSslCertificate::fromPath(tst_QSslSocket::testDataDir
                                                                     + "certs/fluke.cert");
        QVERIFY(!localCert.isEmpty());
        QVERIFY(!localCert.first().isNull());
        socket->setLocalCertificate(localCert.first());

        QVERIFY(socket->setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState));
        QVERIFY(!socket->peerAddress().isNull());
        QVERIFY(socket->peerPort() != 0);
        QVERIFY(!socket->localAddress().isNull());
        QVERIFY(socket->localPort() != 0);
    }

protected slots:
    void ignoreErrorSlot()
    {
        socket->ignoreSslErrors();
    }
};

class ThreadedSslServer: public QThread
{
    Q_OBJECT
public:
    QSemaphore dataReadSemaphore;
    int serverPort;
    bool ok;

    ThreadedSslServer() : serverPort(-1), ok(false)
    { }

    ~ThreadedSslServer()
    {
        if (isRunning()) wait(2000);
        QVERIFY(ok);
    }

signals:
    void listening();

protected:
    void run() override
    {
        // if all goes well (no timeouts), this thread will sleep for a total of 500 ms
        // (i.e., 5 times 100 ms, one sleep for each operation)

        SslServer3 server;
        server.listen(QHostAddress::LocalHost);
        serverPort = server.serverPort();
        emit listening();

        // delayed acceptance:
        QTest::qSleep(100);
        bool ret = server.waitForNewConnection(2000);
        Q_UNUSED(ret);

        // delayed start of encryption
        QTest::qSleep(100);
        QSslSocket *socket = server.socket;
        if (!socket || !socket->isValid())
            return;             // error
        socket->ignoreSslErrors();
        socket->startServerEncryption();
        if (!socket->waitForEncrypted(2000))
            return;             // error

        // delayed reading data
        QTest::qSleep(100);
        if (!socket->waitForReadyRead(2000) && socket->bytesAvailable() == 0)
            return;             // error
        socket->readAll();
        dataReadSemaphore.release();

        // delayed sending data
        QTest::qSleep(100);
        socket->write("Hello, World");
        while (socket->bytesToWrite())
            if (!socket->waitForBytesWritten(2000))
                return;         // error

        // delayed replying (reading then sending)
        QTest::qSleep(100);
        if (!socket->waitForReadyRead(2000))
            return;             // error
        socket->write("Hello, World");
        while (socket->bytesToWrite())
            if (!socket->waitForBytesWritten(2000))
                return;         // error

        // delayed disconnection:
        QTest::qSleep(100);
        socket->disconnectFromHost();
        if (!socket->waitForDisconnected(2000))
            return;             // error

        delete socket;
        ok = true;
    }
};

void tst_QSslSocket::waitForMinusOne()
{
#ifdef Q_OS_WIN
    QSKIP("QTBUG-24451 - indefinite wait may hang");
#endif
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    ThreadedSslServer server;
    connect(&server, SIGNAL(listening()), SLOT(exitLoop()));

    // start the thread and wait for it to be ready
    server.start();
    enterLoop(1);
    QVERIFY(!timeout());

    // connect to the server
    QSslSocket socket;
    QTest::qSleep(100);
    socket.connectToHost("127.0.0.1", server.serverPort);
    QVERIFY(socket.waitForConnected(-1));
    socket.ignoreSslErrors();
    socket.startClientEncryption();

    // first verification: this waiting should take 200 ms
    if (!socket.waitForEncrypted(-1))
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QVERIFY(socket.isEncrypted());
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socket.bytesAvailable(), Q_INT64_C(0));

    // second verification: write and make sure the other side got it (100 ms)
    socket.write("How are you doing?");
    QVERIFY(socket.bytesToWrite() != 0);
    QVERIFY(socket.waitForBytesWritten(-1));
    QVERIFY(server.dataReadSemaphore.tryAcquire(1, 2500));

    // third verification: it should wait for 100 ms:
    QVERIFY(socket.waitForReadyRead(-1));
    QVERIFY(socket.isEncrypted());
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QVERIFY(socket.bytesAvailable() != 0);

    // fourth verification: deadlock prevention:
    // we write and then wait for reading; the other side needs to receive before
    // replying (100 ms delay)
    socket.write("I'm doing just fine!");
    QVERIFY(socket.bytesToWrite() != 0);
    QVERIFY(socket.waitForReadyRead(-1));

    // fifth verification: it should wait for 200 ms more
    QVERIFY(socket.waitForDisconnected(-1));

    // sixth verification: reading from a disconnected socket returns -1
    //                     once we deplete the read buffer
    QCOMPARE(socket.state(), QAbstractSocket::UnconnectedState);
    socket.readAll();
    char aux;
    QCOMPARE(socket.read(&aux, 1), -1);
}

class VerifyServer : public QTcpServer
{
    Q_OBJECT
public:
    VerifyServer() : socket(0) { }
    QSslSocket *socket;

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        socket = new QSslSocket(this);

        socket->setPrivateKey(tst_QSslSocket::testDataDir + "certs/fluke.key");
        socket->setLocalCertificate(tst_QSslSocket::testDataDir + "certs/fluke.cert");
        socket->setSocketDescriptor(socketDescriptor);
        socket->startServerEncryption();
    }
};

void tst_QSslSocket::verifyMode()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QSslSocket socket;

    QCOMPARE(socket.peerVerifyMode(), QSslSocket::AutoVerifyPeer);
    socket.setPeerVerifyMode(QSslSocket::VerifyNone);
    QCOMPARE(socket.peerVerifyMode(), QSslSocket::VerifyNone);
    socket.setPeerVerifyMode(QSslSocket::VerifyNone);
    socket.setPeerVerifyMode(QSslSocket::VerifyPeer);
    QCOMPARE(socket.peerVerifyMode(), QSslSocket::VerifyPeer);

    socket.connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    if (socket.waitForEncrypted())
        QSKIP("Skipping flaky test - See QTBUG-29941");

    QList<QSslError> expectedErrors = QList<QSslError>()
                                      << QSslError(flukeCertificateError, socket.peerCertificate());
    QCOMPARE(socket.sslHandshakeErrors(), expectedErrors);
    socket.abort();

    VerifyServer server;
    server.listen();

    QSslSocket clientSocket;
    clientSocket.connectToHostEncrypted("127.0.0.1", server.serverPort());
    clientSocket.ignoreSslErrors();

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    connect(&clientSocket, SIGNAL(encrypted()), &loop, SLOT(quit()));
    loop.exec();

    QVERIFY(clientSocket.isEncrypted());
    QVERIFY(server.socket->sslHandshakeErrors().isEmpty());
}

void tst_QSslSocket::verifyDepth()
{
    QSslSocket socket;
    QCOMPARE(socket.peerVerifyDepth(), 0);
    socket.setPeerVerifyDepth(1);
    QCOMPARE(socket.peerVerifyDepth(), 1);
    QTest::ignoreMessage(QtWarningMsg, "QSslSocket::setPeerVerifyDepth: cannot set negative depth of -1");
    socket.setPeerVerifyDepth(-1);
    QCOMPARE(socket.peerVerifyDepth(), 1);
}

void tst_QSslSocket::verifyAndDefaultConfiguration()
{
    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy)
        return;
    if (!QSslSocket::supportedFeatures().contains(QSsl::SupportedFeature::CertificateVerification))
        QSKIP("This backend doesn't support manual certificate verification");
    const auto defaultCACertificates = QSslConfiguration::defaultConfiguration().caCertificates();
    const auto chainGuard = qScopeGuard([&defaultCACertificates]{
        auto conf = QSslConfiguration::defaultConfiguration();
        conf.setCaCertificates(defaultCACertificates);
        QSslConfiguration::setDefaultConfiguration(conf);
    });

    auto chain = QSslCertificate::fromPath(testDataDir + QStringLiteral("certs/qtiochain.crt"), QSsl::Pem);
    QCOMPARE(chain.size(), 2);
    QVERIFY(!chain.at(0).isNull());
    QVERIFY(!chain.at(1).isNull());
    auto errors = QSslCertificate::verify(chain);
    // At least, test that 'verify' did not alter the default configuration:
    QCOMPARE(defaultCACertificates, QSslConfiguration::defaultConfiguration().caCertificates());
    if (!errors.isEmpty())
        QSKIP("The certificate for qt.io could not be trusted, skipping the rest of the test");
#ifdef Q_OS_WINDOWS
    const auto fakeCaChain = QSslCertificate::fromPath(testDataDir + QStringLiteral("certs/fluke.cert"));
    QCOMPARE(fakeCaChain.size(), 1);
    const auto caCert = fakeCaChain.at(0);
    QVERIFY(!caCert.isNull());
    auto conf = QSslConfiguration::defaultConfiguration();
    conf.setCaCertificates({caCert});
    QSslConfiguration::setDefaultConfiguration(conf);
    errors = QSslCertificate::verify(chain);
    QVERIFY(errors.size() > 0);
    QCOMPARE(QSslConfiguration::defaultConfiguration().caCertificates(), QList{caCert});
#endif
}

void tst_QSslSocket::disconnectFromHostWhenConnecting()
{
    QSslSocketPtr socket = newSocket();
    socket->connectToHostEncrypted(QtNetworkSettings::imapServerName(), 993);
    socket->ignoreSslErrors();
    socket->write("XXXX LOGOUT\r\n");
    QAbstractSocket::SocketState state = socket->state();
    // without proxy, the state will be HostLookupState;
    // with    proxy, the state will be ConnectingState.
    QVERIFY(socket->state() == QAbstractSocket::HostLookupState ||
            socket->state() == QAbstractSocket::ConnectingState);
    socket->disconnectFromHost();
    // the state of the socket must be the same before and after calling
    // disconnectFromHost()
    QCOMPARE(state, socket->state());
    QVERIFY(socket->state() == QAbstractSocket::HostLookupState ||
            socket->state() == QAbstractSocket::ConnectingState);
    if (!socket->waitForDisconnected(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QCOMPARE(socket->state(), QAbstractSocket::UnconnectedState);
    // we did not call close, so the socket must be still open
    QVERIFY(socket->isOpen());
    QCOMPARE(socket->bytesToWrite(), qint64(0));

    // don't forget to login
    QCOMPARE((int) socket->write("USER ftptest\r\n"), 14);

}

void tst_QSslSocket::disconnectFromHostWhenConnected()
{
    QSslSocketPtr socket = newSocket();
    socket->connectToHostEncrypted(QtNetworkSettings::imapServerName(), 993);
    socket->ignoreSslErrors();
    if (!socket->waitForEncrypted(5000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
    socket->write("XXXX LOGOUT\r\n");
    QCOMPARE(socket->state(), QAbstractSocket::ConnectedState);
    socket->disconnectFromHost();
    QCOMPARE(socket->state(), QAbstractSocket::ClosingState);
    QVERIFY(socket->waitForDisconnected(5000));
    QCOMPARE(socket->bytesToWrite(), qint64(0));
}

#if QT_CONFIG(openssl)

class BrokenPskHandshake : public QTcpServer
{
public:
    void socketError(QAbstractSocket::SocketError error)
    {
        Q_UNUSED(error);
        QSslSocket *clientSocket = qobject_cast<QSslSocket *>(sender());
        Q_ASSERT(clientSocket);
        clientSocket->close();
        QTestEventLoop::instance().exitLoop();
    }
private:

    void incomingConnection(qintptr handle) override
    {
        if (!socket.setSocketDescriptor(handle))
            return;

        QSslConfiguration serverConfig(QSslConfiguration::defaultConfiguration());
        serverConfig.setPreSharedKeyIdentityHint("abcdefghijklmnop");
        socket.setSslConfiguration(serverConfig);
        socket.startServerEncryption();
    }

    QSslSocket socket;
};

void tst_QSslSocket::closeWhileEmittingSocketError()
{
    if (!isTestingOpenSsl)
        QSKIP("The active TLS backend does not support this test");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    BrokenPskHandshake handshake;
    if (!handshake.listen())
        QSKIP("failed to start TLS server");

    QSslSocket clientSocket;
    QSslConfiguration clientConfig(QSslConfiguration::defaultConfiguration());
    clientConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    clientSocket.setSslConfiguration(clientConfig);

    QSignalSpy socketErrorSpy(&clientSocket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)));
    connect(&clientSocket, &QSslSocket::errorOccurred, &handshake, &BrokenPskHandshake::socketError);

    clientSocket.connectToHostEncrypted(QStringLiteral("127.0.0.1"), handshake.serverPort());
    // Make sure we have some data buffered so that close will try to flush:
    clientSocket.write(QByteArray(1000000, Qt::Uninitialized));

    QTestEventLoop::instance().enterLoop(1s);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(socketErrorSpy.size(), 1);
}

#endif // Feature 'openssl'.

void tst_QSslSocket::resetProxy()
{
#if QT_CONFIG(networkproxy)
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    // check fix for bug 199941

    QNetworkProxy goodProxy(QNetworkProxy::NoProxy);
    QNetworkProxy badProxy(QNetworkProxy::HttpProxy, "thisCannotWorkAbsolutelyNotForSure", 333);

    // make sure the connection works, and then set a nonsense proxy, and then
    // make sure it does not work anymore
    QSslSocket socket;
    auto config = socket.sslConfiguration();
    config.addCaCertificates(httpServerCertChainPath());
    socket.setSslConfiguration(config);
    socket.setProxy(goodProxy);
    socket.connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    QVERIFY2(socket.waitForConnected(10000), qPrintable(socket.errorString()));
    socket.abort();
    socket.setProxy(badProxy);
    socket.connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    QVERIFY(! socket.waitForConnected(10000));

    // don't forget to login
    QCOMPARE((int) socket.write("USER ftptest\r\n"), 14);
    QCOMPARE((int) socket.write("PASS password\r\n"), 15);

    enterLoop(10);

    // now the other way round:
    // set the nonsense proxy and make sure the connection does not work,
    // and then set the right proxy and make sure it works
    QSslSocket socket2;
    auto config2 = socket.sslConfiguration();
    config2.addCaCertificates(httpServerCertChainPath());
    socket2.setSslConfiguration(config2);
    socket2.setProxy(badProxy);
    socket2.connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    QVERIFY(! socket2.waitForConnected(10000));
    socket2.abort();
    socket2.setProxy(goodProxy);
    socket2.connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    QVERIFY2(socket2.waitForConnected(10000), qPrintable(socket.errorString()));
#endif // QT_CONFIG(networkproxy)
}

void tst_QSslSocket::ignoreSslErrorsList_data()
{
    QTest::addColumn<QList<QSslError> >("expectedSslErrors");
    QTest::addColumn<int>("expectedSslErrorSignalCount");

    // construct the list of errors that we will get with the SSL handshake and that we will ignore
    QList<QSslError> expectedSslErrors;
    // fromPath gives us a list of certs, but it actually only contains one
    QList<QSslCertificate> certs = QSslCertificate::fromPath(httpServerCertChainPath());
    QSslError rightError(flukeCertificateError, certs.at(0));
    QSslError wrongError(flukeCertificateError);


    QTest::newRow("SSL-failure-empty-list") << expectedSslErrors << 1;
    expectedSslErrors.append(wrongError);
    QTest::newRow("SSL-failure-wrong-error") << expectedSslErrors << 1;
    expectedSslErrors.append(rightError);
    QTest::newRow("allErrorsInExpectedList1") << expectedSslErrors << 0;
    expectedSslErrors.removeAll(wrongError);
    QTest::newRow("allErrorsInExpectedList2") << expectedSslErrors << 0;
    expectedSslErrors.removeAll(rightError);
    QTest::newRow("SSL-failure-empty-list-again") << expectedSslErrors << 1;
}

void tst_QSslSocket::ignoreSslErrorsList()
{
    QSslSocket socket;
    connect(&socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            this, SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));

    QSslCertificate cert;

    QFETCH(QList<QSslError>, expectedSslErrors);
    socket.ignoreSslErrors(expectedSslErrors);

    QFETCH(int, expectedSslErrorSignalCount);
    QSignalSpy sslErrorsSpy(&socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)));

    socket.connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);

    bool expectEncryptionSuccess = (expectedSslErrorSignalCount == 0);
    if (socket.waitForEncrypted(10000) != expectEncryptionSuccess)
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QCOMPARE(sslErrorsSpy.size(), expectedSslErrorSignalCount);
}

void tst_QSslSocket::ignoreSslErrorsListWithSlot_data()
{
    ignoreSslErrorsList_data();
}

// this is not a test, just a slot called in the test below
void tst_QSslSocket::ignoreErrorListSlot(const QList<QSslError> &)
{
    socket->ignoreSslErrors(storedExpectedSslErrors);
}

void tst_QSslSocket::ignoreSslErrorsListWithSlot()
{
    QSslSocket socket;
    this->socket = &socket;

    QFETCH(QList<QSslError>, expectedSslErrors);
    // store the errors to ignore them later in the slot connected below
    storedExpectedSslErrors = expectedSslErrors;
    connect(&socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            this, SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(ignoreErrorListSlot(QList<QSslError>)));
    socket.connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);

    QFETCH(int, expectedSslErrorSignalCount);
    bool expectEncryptionSuccess = (expectedSslErrorSignalCount == 0);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && (socket.waitForEncrypted(10000) != expectEncryptionSuccess))
        QSKIP("Skipping flaky test - See QTBUG-29941");
}

void tst_QSslSocket::abortOnSslErrors()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    QVERIFY(server.listen());

    QSslSocket clientSocket;
    connect(&clientSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(abortOnErrorSlot()));
    clientSocket.connectToHostEncrypted("127.0.0.1", server.serverPort());
    clientSocket.ignoreSslErrors();

    QEventLoop loop;
    QTimer::singleShot(1000, &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(clientSocket.state(), QAbstractSocket::UnconnectedState);
}

// make sure a closed socket has no bytesAvailable()
// related to https://bugs.webkit.org/show_bug.cgi?id=28016
void tst_QSslSocket::readFromClosedSocket()
{
    QSslSocketPtr socket = newSocket();

    socket->ignoreSslErrors();
    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    socket->ignoreSslErrors();
    socket->waitForConnected();
    socket->waitForEncrypted();
    // provoke a response by sending a request
    socket->write("GET /qtest/fluke.gif HTTP/1.1\n");
    socket->write("Host: ");
    socket->write(QtNetworkSettings::httpServerName().toLocal8Bit().constData());
    socket->write("\n");
    socket->write("\n");
    socket->waitForBytesWritten();
    socket->waitForReadyRead();
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && (socket->state() != QAbstractSocket::ConnectedState))
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QVERIFY(socket->bytesAvailable());
    socket->close();
    QVERIFY(!socket->bytesAvailable());
    QVERIFY(!socket->bytesToWrite());
    QCOMPARE(socket->state(), QAbstractSocket::UnconnectedState);
}

void tst_QSslSocket::writeBigChunk()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();

    connect(this->socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);

    QByteArray data;
    // Originally, the test had this: '1024*1024*10; // 10 MB'
    data.resize(1024 * 1024 * 10);
    // Init with garbage. Needed so TLS cannot compress it in an efficient way.
    QRandomGenerator::global()->fillRange(reinterpret_cast<quint32 *>(data.data()),
                                          data.size() / int(sizeof(quint32)));

    if (!socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QString errorBefore = socket->errorString();

    int ret = socket->write(data.constData(), data.size());
    QCOMPARE(data.size(), ret);

    // spin the event loop once so QSslSocket::transmit() gets called
    QCoreApplication::processEvents();
    QString errorAfter = socket->errorString();

    // no better way to do this right now since the error is the same as the default error.
    if (socket->errorString().startsWith(QLatin1String("Unable to write data")))
    {
        qWarning() << socket->error() << socket->errorString();
        QFAIL("Error while writing! Check if the OpenSSL BIO size is limited?!");
    }
    // also check the error string. If another error (than UnknownError) occurred, it should be different than before
    QVERIFY2(errorBefore == errorAfter || socket->error() == QAbstractSocket::RemoteHostClosedError,
             QByteArray("unexpected error: ").append(qPrintable(errorAfter)));

    // check that everything has been written to OpenSSL
    QCOMPARE(socket->bytesToWrite(), 0);

    socket->close();
}

void tst_QSslSocket::blacklistedCertificates()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server(testDataDir + "certs/fake-login.live.com.key", testDataDir + "certs/fake-login.live.com.pem");
    QSslSocket *receiver = new QSslSocket(this);
    connect(receiver, SIGNAL(readyRead()), SLOT(exitLoop()));

    // connect two sockets to each other:
    QVERIFY(server.listen(QHostAddress::LocalHost));
    receiver->connectToHost("127.0.0.1", server.serverPort());
    QVERIFY(receiver->waitForConnected(5000));
    if (!server.waitForNewConnection(0))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    QSslSocket *sender = server.socket;
    QVERIFY(sender);
    QCOMPARE(sender->state(), QAbstractSocket::ConnectedState);
    receiver->setObjectName("receiver");
    sender->setObjectName("sender");
    receiver->startClientEncryption();

    connect(receiver, SIGNAL(sslErrors(QList<QSslError>)), SLOT(exitLoop()));
    connect(receiver, SIGNAL(encrypted()), SLOT(exitLoop()));
    enterLoop(1);
    QList<QSslError> sslErrors = receiver->sslHandshakeErrors();
    QVERIFY(sslErrors.size() > 0);
    // there are more errors (self signed cert and hostname mismatch), but we only care about the blacklist error
    std::optional<QSslError> blacklistedError;
    for (const QSslError &error : sslErrors) {
        if (error.error() == QSslError::CertificateBlacklisted) {
            blacklistedError = error;
            break;
        }
    }
    QVERIFY2(blacklistedError, "CertificateBlacklisted error not found!");
}

void tst_QSslSocket::versionAccessors()
{
    if (!QSslSocket::supportsSsl())
        return;

    qDebug() << QSslSocket::sslLibraryVersionString();
    qDebug() << QString::number(QSslSocket::sslLibraryVersionNumber(), 16);
}

void tst_QSslSocket::encryptWithoutConnecting()
{
    if (!QSslSocket::supportsSsl())
        return;

    QTest::ignoreMessage(QtWarningMsg,
                         "QSslSocket::startClientEncryption: cannot start handshake when not connected");

    QSslSocket sock;
    sock.startClientEncryption();
}

void tst_QSslSocket::resume_data()
{
    // Starting from OpenSSL v 3.1.1 deprecated protocol versions (we want to use in 'resume' test) are not available by default.
    if (isSecurityLevel0Required)
        QSKIP("Testing with OpenSSL backend, but security level 0 is required for TLS v1.1 or earlier");

    QTest::addColumn<bool>("ignoreErrorsAfterPause");
    QTest::addColumn<QList<QSslError> >("errorsToIgnore");
    QTest::addColumn<bool>("expectSuccess");

    QList<QSslError> errorsList;
    QTest::newRow("DoNotIgnoreErrors") << false << QList<QSslError>() << false;
    QTest::newRow("ignoreAllErrors") << true << QList<QSslError>() << true;

    // Note, httpServerCertChainPath() it's ... because we use the same certificate on
    // different services. We'll be actually connecting to IMAP server.
    QList<QSslCertificate> certs = QSslCertificate::fromPath(httpServerCertChainPath());
    QSslError rightError(flukeCertificateError, certs.at(0));
    QSslError wrongError(flukeCertificateError);
    errorsList.append(wrongError);
    QTest::newRow("ignoreSpecificErrors-Wrong") << true << errorsList << false;
    errorsList.clear();
    errorsList.append(rightError);
    QTest::newRow("ignoreSpecificErrors-Right") << true << errorsList << true;
}

void tst_QSslSocket::resume()
{
    // make sure the server certificate is not in the list of accepted certificates,
    // we want to trigger the sslErrors signal
    auto sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setCaCertificates(QSslConfiguration::systemCaCertificates());
    QSslConfiguration::setDefaultConfiguration(sslConfig);

    QFETCH(bool, ignoreErrorsAfterPause);
    QFETCH(QList<QSslError>, errorsToIgnore);
    QFETCH(bool, expectSuccess);

    QSslSocket socket;
    socket.setPauseMode(QAbstractSocket::PauseOnSslErrors);

    // Set TLS 1.0 or above because the server doesn't support TLS 1.2 or above
    // QTQAINFRA-4499
    QSslConfiguration config = socket.sslConfiguration();
    config.setProtocol(Test::TlsV1_0OrLater);
    socket.setSslConfiguration(config);

    QSignalSpy sslErrorSpy(&socket, SIGNAL(sslErrors(QList<QSslError>)));
    QSignalSpy encryptedSpy(&socket, SIGNAL(encrypted()));
    QSignalSpy errorSpy(&socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)));

    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    connect(&socket, SIGNAL(encrypted()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    connect(&socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            this, SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(&socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    socket.connectToHostEncrypted(QtNetworkSettings::imapServerName(), 993);
    QTestEventLoop::instance().enterLoop(10);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && QTestEventLoop::instance().timeout())
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QCOMPARE(sslErrorSpy.size(), 1);
    QCOMPARE(errorSpy.size(), 0);
    QCOMPARE(encryptedSpy.size(), 0);
    QVERIFY(!socket.isEncrypted());
    if (ignoreErrorsAfterPause) {
        if (errorsToIgnore.empty())
            socket.ignoreSslErrors();
        else
            socket.ignoreSslErrors(errorsToIgnore);
    }
    socket.resume();
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout()); // quit by encrypted() or error() signal
    if (expectSuccess) {
        QCOMPARE(encryptedSpy.size(), 1);
        QVERIFY(socket.isEncrypted());
        QCOMPARE(errorSpy.size(), 0);
        socket.disconnectFromHost();
        QVERIFY(socket.waitForDisconnected(10000));
    } else {
        QCOMPARE(encryptedSpy.size(), 0);
        QVERIFY(!socket.isEncrypted());
        QCOMPARE(errorSpy.size(), 1);
        QCOMPARE(socket.error(), QAbstractSocket::SslHandshakeFailedError);
    }
}

class WebSocket : public QSslSocket
{
    Q_OBJECT
public:
    explicit WebSocket(qintptr socketDescriptor,
                       const QString &keyFile = tst_QSslSocket::testDataDir + "certs/fluke.key",
                       const QString &certFile = tst_QSslSocket::testDataDir + "certs/fluke.cert");

protected slots:
    void onReadyReadFirstBytes(void);

private:
    void _startServerEncryption(void);

    QString m_keyFile;
    QString m_certFile;

private:
    Q_DISABLE_COPY(WebSocket)
};

WebSocket::WebSocket (qintptr socketDescriptor, const QString &keyFile, const QString &certFile)
    : m_keyFile(keyFile),
      m_certFile(certFile)
{
    QVERIFY(setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState, QIODevice::ReadWrite | QIODevice::Unbuffered));
    connect (this, SIGNAL(readyRead()), this, SLOT(onReadyReadFirstBytes()));
}

void WebSocket::_startServerEncryption (void)
{
    QFile file(m_keyFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!key.isNull());
    setPrivateKey(key);

    QList<QSslCertificate> localCert = QSslCertificate::fromPath(m_certFile);
    QVERIFY(!localCert.isEmpty());
    QVERIFY(!localCert.first().isNull());
    setLocalCertificate(localCert.first());

    QVERIFY(!peerAddress().isNull());
    QVERIFY(peerPort() != 0);
    QVERIFY(!localAddress().isNull());
    QVERIFY(localPort() != 0);

    setProtocol(QSsl::AnyProtocol);
    setPeerVerifyMode(QSslSocket::VerifyNone);
    ignoreSslErrors();
    startServerEncryption();
}

void WebSocket::onReadyReadFirstBytes (void)
{
    peek(1);
    disconnect(this,SIGNAL(readyRead()), this, SLOT(onReadyReadFirstBytes()));
    _startServerEncryption();
}

class SslServer4 : public QTcpServer
{
    Q_OBJECT
public:

    QScopedPointer<WebSocket> socket;

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        socket.reset(new WebSocket(socketDescriptor));
    }
};

void tst_QSslSocket::qtbug18498_peek()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer4 server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    QSslSocket client;
    client.connectToHost("127.0.0.1", server.serverPort());
    QVERIFY(client.waitForConnected(5000));
    QVERIFY(server.waitForNewConnection(1000));
    client.ignoreSslErrors();

    int encryptedCounter = 2;
    connect(&client, &QSslSocket::encrypted, this, [&encryptedCounter](){
        if (!--encryptedCounter)
            exitLoop();
    });
    WebSocket *serversocket = server.socket.data();
    connect(serversocket, &QSslSocket::encrypted, this, [&encryptedCounter](){
        if (!--encryptedCounter)
            exitLoop();
    });
    connect(&client, SIGNAL(disconnected()), this, SLOT(exitLoop()));

    client.startClientEncryption();
    QVERIFY(serversocket);

    enterLoop(1);
    QVERIFY(!timeout());
    QVERIFY(serversocket->isEncrypted());
    QVERIFY(client.isEncrypted());

    QByteArray data("abc123");
    client.write(data.data());

    connect(serversocket, SIGNAL(readyRead()), this, SLOT(exitLoop()));
    enterLoop(1);
    QVERIFY(!timeout());

    QByteArray peek1_data;
    peek1_data.reserve(data.size());
    QByteArray peek2_data;
    QByteArray read_data;

    int lngth = serversocket->peek(peek1_data.data(), 10);
    peek1_data.resize(lngth);

    peek2_data = serversocket->peek(10);
    read_data = serversocket->readAll();

    QCOMPARE(peek1_data, data);
    QCOMPARE(peek2_data, data);
    QCOMPARE(read_data, data);
}

class SslServer5 : public QTcpServer
{
    Q_OBJECT
public:
    SslServer5() : socket(0) {}
    QSslSocket *socket;

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        socket =  new QSslSocket;
        socket->setSocketDescriptor(socketDescriptor);
    }
};

void tst_QSslSocket::qtbug18498_peek2()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer5 listener;
    QVERIFY(listener.listen(QHostAddress::Any));
    QScopedPointer<QSslSocket> client(new QSslSocket);
    client->connectToHost(QHostAddress::LocalHost, listener.serverPort());
    QVERIFY(client->waitForConnected(5000));
    QVERIFY(listener.waitForNewConnection(1000));

    QScopedPointer<QSslSocket> server(listener.socket);

    QVERIFY(server->write("HELLO\r\n", 7));
    QTRY_COMPARE(client->bytesAvailable(), 7);
    char c;
    QCOMPARE(client->peek(&c,1), 1);
    QCOMPARE(c, 'H');
    QCOMPARE(client->read(&c,1), 1);
    QCOMPARE(c, 'H');
    QByteArray b = client->peek(2);
    QCOMPARE(b, QByteArray("EL"));
    char a[3];
    QVERIFY(client->peek(a, 2) == 2);
    QCOMPARE(a[0], 'E');
    QCOMPARE(a[1], 'L');
    QCOMPARE(client->readAll(), QByteArray("ELLO\r\n"));

    //check data split between QIODevice and plain socket buffers.
    QByteArray bigblock;
    bigblock.fill('#', QIODEVICE_BUFFERSIZE + 1024);
    QVERIFY(client->write(QByteArray("head")));
    QVERIFY(client->write(bigblock));
    QTRY_COMPARE(server->bytesAvailable(), bigblock.size() + 4);
    QCOMPARE(server->read(4), QByteArray("head"));
    QCOMPARE(server->peek(bigblock.size()), bigblock);
    b.reserve(bigblock.size());
    b.resize(server->peek(b.data(), bigblock.size()));
    QCOMPARE(b, bigblock);

    //check oversized peek
    QCOMPARE(server->peek(bigblock.size() * 3), bigblock);
    b.reserve(bigblock.size() * 3);
    b.resize(server->peek(b.data(), bigblock.size() * 3));
    QCOMPARE(b, bigblock);

    QCOMPARE(server->readAll(), bigblock);

    QVERIFY(client->write("STARTTLS\r\n"));
    QTRY_COMPARE(server->bytesAvailable(), 10);
    QCOMPARE(server->peek(&c,1), 1);
    QCOMPARE(c, 'S');
    b = server->peek(3);
    QCOMPARE(b, QByteArray("STA"));
    QCOMPARE(server->read(5), QByteArray("START"));
    QVERIFY(server->peek(a, 3) == 3);
    QCOMPARE(a[0], 'T');
    QCOMPARE(a[1], 'L');
    QCOMPARE(a[2], 'S');
    QCOMPARE(server->readAll(), QByteArray("TLS\r\n"));

    QFile file(testDataDir + "certs/fluke.key");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!key.isNull());
    server->setPrivateKey(key);

    QList<QSslCertificate> localCert = QSslCertificate::fromPath(testDataDir + "certs/fluke.cert");
    QVERIFY(!localCert.isEmpty());
    QVERIFY(!localCert.first().isNull());
    server->setLocalCertificate(localCert.first());

    server->setProtocol(QSsl::AnyProtocol);
    server->setPeerVerifyMode(QSslSocket::VerifyNone);

    server->ignoreSslErrors();
    client->ignoreSslErrors();

    server->startServerEncryption();
    client->startClientEncryption();

    QVERIFY(server->write("hello\r\n", 7));
    QTRY_COMPARE(client->bytesAvailable(), 7);
    QVERIFY(server->mode() == QSslSocket::SslServerMode && client->mode() == QSslSocket::SslClientMode);
    QCOMPARE(client->peek(&c,1), 1);
    QCOMPARE(c, 'h');
    QCOMPARE(client->read(&c,1), 1);
    QCOMPARE(c, 'h');
    b = client->peek(2);
    QCOMPARE(b, QByteArray("el"));
    QCOMPARE(client->readAll(), QByteArray("ello\r\n"));

    QVERIFY(client->write("goodbye\r\n"));
    QTRY_COMPARE(server->bytesAvailable(), 9);
    QCOMPARE(server->peek(&c,1), 1);
    QCOMPARE(c, 'g');
    QCOMPARE(server->readAll(), QByteArray("goodbye\r\n"));
    client->disconnectFromHost();
    QVERIFY(client->waitForDisconnected(5000));
}

void tst_QSslSocket::dhServer()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("No SSL support");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    server.ciphers = {QSslCipher("DHE-RSA-AES256-SHA"), QSslCipher("DHE-DSS-AES256-SHA")};
    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSslSocket client;
    socket = &client;
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
}

#if QT_CONFIG(openssl)
void tst_QSslSocket::dhServerCustomParamsNull()
{
    if (!isTestingOpenSsl)
        QSKIP("This test requires OpenSSL as the active TLS backend");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    server.ciphers = {QSslCipher("DHE-RSA-AES256-SHA"), QSslCipher("DHE-DSS-AES256-SHA")};
    server.protocol = Test::TlsV1_0;

    QSslConfiguration cfg = server.config;
    cfg.setDiffieHellmanParameters(QSslDiffieHellmanParameters());
    server.config = cfg;

    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSslSocket client;
    QSslConfiguration config = client.sslConfiguration();
    config.setProtocol(Test::TlsV1_0);
    client.setSslConfiguration(config);
    socket = &client;
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();

    QVERIFY(client.state() != QAbstractSocket::ConnectedState);
}

void tst_QSslSocket::dhServerCustomParams()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("No SSL support");
    if (!QSslSocket::isClassImplemented(QSsl::ImplementedClass::DiffieHellman))
        QSKIP("The current backend doesn't support diffie hellman parameters");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    server.ciphers = {QSslCipher("DHE-RSA-AES256-SHA"), QSslCipher("DHE-DSS-AES256-SHA")};

    QSslConfiguration cfg = server.config;

    // Custom 2048-bit DH parameters generated with 'openssl dhparam -outform DER -out out.der -check -2 2048'
    const auto dh = QSslDiffieHellmanParameters::fromEncoded(QByteArray::fromBase64(QByteArrayLiteral(
        "MIIBCAKCAQEAvVA7b8keTfjFutCtTJmP/pnQfw/prKa+GMed/pBWjrC4N1YwnI8h/A861d9WE/VWY7XMTjvjX3/0"
        "aaU8wEe0EXNpFdlTH+ZMQctQTSJOyQH0RCTwJfDGPCPT9L+c9GKwEKWORH38Earip986HJc0w3UbnfIwXUdsWHiXi"
        "Z6r3cpyBmTKlsXTFiDVAOUXSiO8d/zOb6zHZbDfyB/VbtZRmnA7TXVn9oMzC0g9+FXHdrV4K+XfdvNZdCegvoAZiy"
        "R6ZQgNG9aZ36/AQekhg060hp55f9HDPgXqYeNeXBiferjUtU7S9b3s83XhOJAr01/0Tf5dENwCfg2gK36TM8cC4wI"
        "BAg==")), QSsl::Der);
    cfg.setDiffieHellmanParameters(dh);

    server.config = cfg;

    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSslSocket client;
    socket = &client;
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();

    QVERIFY(client.state() == QAbstractSocket::ConnectedState);
}
#endif // QT_CONFIG(openssl)

void tst_QSslSocket::ecdhServer()
{
    if (!QSslSocket::supportsSsl()) {
        qWarning("SSL not supported, skipping test");
        return;
    }

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    server.ciphers = {QSslCipher("ECDHE-RSA-AES128-SHA")};
    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSslSocket client;
    socket = &client;
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
}

void tst_QSslSocket::verifyClientCertificate_data()
{
    QTest::addColumn<QSslSocket::PeerVerifyMode>("peerVerifyMode");
    QTest::addColumn<QList<QSslCertificate> >("clientCerts");
    QTest::addColumn<QSslKey>("clientKey");
    QTest::addColumn<bool>("works");

    // no certificate
    QList<QSslCertificate> noCerts;
    QSslKey noKey;

    QTest::newRow("NoCert:AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << noCerts << noKey << true;
    QTest::newRow("NoCert:QueryPeer") << QSslSocket::QueryPeer << noCerts << noKey << true;
    QTest::newRow("NoCert:VerifyNone") << QSslSocket::VerifyNone << noCerts << noKey << true;
    QTest::newRow("NoCert:VerifyPeer") << QSslSocket::VerifyPeer << noCerts << noKey << false;

    // self-signed certificate
    QList<QSslCertificate> flukeCerts = QSslCertificate::fromPath(testDataDir + "certs/fluke.cert");
    QCOMPARE(flukeCerts.size(), 1);

    QFile flukeFile(testDataDir + "certs/fluke.key");
    QVERIFY(flukeFile.open(QIODevice::ReadOnly));
    QSslKey flukeKey(flukeFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!flukeKey.isNull());

    QTest::newRow("SelfSignedCert:AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << flukeCerts << flukeKey << true;
    QTest::newRow("SelfSignedCert:QueryPeer") << QSslSocket::QueryPeer << flukeCerts << flukeKey << true;
    QTest::newRow("SelfSignedCert:VerifyNone") << QSslSocket::VerifyNone << flukeCerts << flukeKey << true;
    QTest::newRow("SelfSignedCert:VerifyPeer") << QSslSocket::VerifyPeer << flukeCerts << flukeKey << false;

    // valid certificate, but wrong usage (server certificate)
    QList<QSslCertificate> serverCerts = QSslCertificate::fromPath(testDataDir + "certs/bogus-server.crt");
    QCOMPARE(serverCerts.size(), 1);

    QFile serverFile(testDataDir + "certs/bogus-server.key");
    QVERIFY(serverFile.open(QIODevice::ReadOnly));
    QSslKey serverKey(serverFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!serverKey.isNull());

    QTest::newRow("ValidServerCert:AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << serverCerts << serverKey << true;
    QTest::newRow("ValidServerCert:QueryPeer") << QSslSocket::QueryPeer << serverCerts << serverKey << true;
    QTest::newRow("ValidServerCert:VerifyNone") << QSslSocket::VerifyNone << serverCerts << serverKey << true;
    QTest::newRow("ValidServerCert:VerifyPeer") << QSslSocket::VerifyPeer << serverCerts << serverKey << false;

    // valid certificate, correct usage (client certificate)
    QList<QSslCertificate> validCerts = QSslCertificate::fromPath(testDataDir + "certs/bogus-client.crt");
    QCOMPARE(validCerts.size(), 1);

    QFile validFile(testDataDir + "certs/bogus-client.key");
    QVERIFY(validFile.open(QIODevice::ReadOnly));
    QSslKey validKey(validFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!validKey.isNull());

    QTest::newRow("ValidClientCert:AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << validCerts << validKey << true;
    QTest::newRow("ValidClientCert:QueryPeer") << QSslSocket::QueryPeer << validCerts << validKey << true;
    QTest::newRow("ValidClientCert:VerifyNone") << QSslSocket::VerifyNone << validCerts << validKey << true;
    QTest::newRow("ValidClientCert:VerifyPeer") << QSslSocket::VerifyPeer << validCerts << validKey << true;

    // valid certificate, correct usage (client certificate), with chain
    validCerts += QSslCertificate::fromPath(testDataDir + "certs/bogus-ca.crt");
    QCOMPARE(validCerts.size(), 2);

    QTest::newRow("ValidChainedClientCert:AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << validCerts << validKey << true;
    QTest::newRow("ValidChainedClientCert:QueryPeer") << QSslSocket::QueryPeer << validCerts << validKey << true;
    QTest::newRow("ValidChainedClientCert:VerifyNone") << QSslSocket::VerifyNone << validCerts << validKey << true;
    QTest::newRow("ValidChainedClientCert:VerifyPeer") << QSslSocket::VerifyPeer << validCerts << validKey << true;
}

void tst_QSslSocket::verifyClientCertificate()
{
    if (isTestingSecureTransport) {
        // We run both client and server on the same machine,
        // this means, client can update keychain with client's certificates,
        // and server later will use the same certificates from the same
        // keychain thus making tests fail (wrong number of certificates,
        // success instead of failure etc.).
        QSKIP("This test can not work with Secure Transport");
    }

    if (!QSslSocket::supportsSsl()) {
        qWarning("SSL not supported, skipping test");
        return;
    }

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QFETCH(QSslSocket::PeerVerifyMode, peerVerifyMode);
    if (isTestingSchannel) {
        if (peerVerifyMode == QSslSocket::QueryPeer || peerVerifyMode == QSslSocket::AutoVerifyPeer)
            QSKIP("Schannel doesn't tackle requesting a certificate and not receiving one.");
    }

    SslServer server;
    server.protocol = QSsl::TlsV1_2;
    server.addCaCertificates = testDataDir + "certs/bogus-ca.crt";
    server.ignoreSslErrors = false;
    server.peerVerifyMode = peerVerifyMode;
    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QFETCH(QList<QSslCertificate>, clientCerts);
    QFETCH(QSslKey, clientKey);
    QSslSocket client;
    client.setLocalCertificateChain(clientCerts);
    client.setPrivateKey(clientKey);
    QSslConfiguration config = client.sslConfiguration();
    config.setProtocol(Test::TlsV1_0OrLater);
    client.setSslConfiguration(config);
    socket = &client;

    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(disconnected()), &loop, SLOT(quit()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();

    QFETCH(bool, works);
    QAbstractSocket::SocketState expectedState = (works) ? QAbstractSocket::ConnectedState : QAbstractSocket::UnconnectedState;

    // check server socket
    QVERIFY(server.socket);

    QCOMPARE(server.socket->state(), expectedState);
    QCOMPARE(server.socket->isEncrypted(), works);

    if (peerVerifyMode == QSslSocket::VerifyNone || clientCerts.isEmpty()) {
        QVERIFY(server.socket->peerCertificate().isNull());
        QVERIFY(server.socket->peerCertificateChain().isEmpty());
    } else {
        QCOMPARE(server.socket->peerCertificate(), clientCerts.first());
        if (isTestingSchannel) {
            if (clientCerts.size() == 1 && server.socket->peerCertificateChain().size() == 2) {
                QEXPECT_FAIL("",
                             "Schannel includes the entire chain, not just the leaf and intermediates",
                             Continue);
            }
        }
        QCOMPARE(server.socket->peerCertificateChain(), clientCerts);
    }

    // check client socket
    QCOMPARE(client.state(), expectedState);
    QCOMPARE(client.isEncrypted(), works);
}

void tst_QSslSocket::readBufferMaxSize()
{
    // QTBUG-94186: originally, only SecureTransport was
    // running this test (since it was a bug in that backend,
    // see comment below), after TLS plugins were introduced,
    // we enabled this test on all backends, as a part of
    // the clean up. This revealed the fact that this test
    // is flaky, and it started to fail on Windows.
    // TODO: this is a temporary solution, to be removed
    // as soon as 94186 fixed for good.
    if (!isTestingSecureTransport)
        QSKIP("This test is flaky with TLS backend other than SecureTransport");


    // QTBUG-55170:
    // SecureTransport back-end was ignoring read-buffer
    // size limit, resulting (potentially) in a constantly
    // growing internal buffer.
    // The test's logic is: we set a small read buffer size on a client
    // socket (to some ridiculously small value), server sends us
    // a bunch of bytes , we ignore readReady signal so
    // that socket's internal buffer size stays
    // >= readBufferMaxSize, we wait for a quite long time
    // (which previously would be enough to read completely)
    // and we check socket's bytesAvaiable to be less than sent.
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    QVERIFY(server.listen());

    QEventLoop loop;

    QSslSocketPtr client(new QSslSocket);
    socket = client.data();
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));

    client->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                   server.serverPort());

    int waitFor = 2;
    auto earlyQuitter = [&loop, &waitFor]() {
        if (!--waitFor)
            loop.exit();
    };

    connect(socket, &QSslSocket::encrypted, &loop, earlyQuitter);
    connect(&server, &SslServer::socketEncrypted, &loop, earlyQuitter);

    // Wait for 'encrypted' first:
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(client->state(), QAbstractSocket::ConnectedState);
    QCOMPARE(client->mode(), QSslSocket::SslClientMode);

    client->setReadBufferSize(10);
    const QByteArray message(int(0xffff), 'a');
    server.socket->write(message);

    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    int readSoFar = client->bytesAvailable();
    QVERIFY(readSoFar > 0 && readSoFar < message.size());
    // Now, let's check that we still can read the rest of it:
    QCOMPARE(client->readAll().size(), readSoFar);

    client->setReadBufferSize(0);

    QTimer::singleShot(1500, &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(client->bytesAvailable() + readSoFar, message.size());
}

void tst_QSslSocket::setEmptyDefaultConfiguration() // this test should be last, as it has some side effects
{
    // used to produce a crash in QSslConfigurationPrivate::deepCopyDefaultConfiguration, QTBUG-13265

    if (!QSslSocket::supportsSsl())
        return;

    QSslConfiguration emptyConf;
    QSslConfiguration::setDefaultConfiguration(emptyConf);

    QSslSocketPtr client = newSocket();
    socket = client.data();

    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    socket->connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && socket->waitForEncrypted(4000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
}

void tst_QSslSocket::allowedProtocolNegotiation()
{
    if (!hasServerAlpn)
        QSKIP("Server-side ALPN is unsupported, skipping test");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    const QByteArray expectedNegotiated("cool-protocol");
    QList<QByteArray> serverProtos;
    serverProtos << expectedNegotiated << "not-so-cool-protocol";
    QList<QByteArray> clientProtos;
    clientProtos << "uber-cool-protocol" << expectedNegotiated << "not-so-cool-protocol";


    SslServer server;
    server.config.setAllowedNextProtocols(serverProtos);
    QVERIFY(server.listen());

    QSslSocket clientSocket;
    auto configuration = clientSocket.sslConfiguration();
    configuration.setAllowedNextProtocols(clientProtos);
    clientSocket.setSslConfiguration(configuration);

    clientSocket.connectToHostEncrypted("127.0.0.1", server.serverPort());
    clientSocket.ignoreSslErrors();

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    // Need to wait for both sides to emit encrypted as the ordering of which
    // ones emits encrypted() changes depending on whether we use TLS 1.2 or 1.3
    int waitFor = 2;
    auto earlyQuitter = [&loop, &waitFor]() {
        if (!--waitFor)
            loop.exit();
    };
    connect(&clientSocket, &QSslSocket::encrypted, &loop, earlyQuitter);
    connect(&server, &SslServer::socketEncrypted, &loop, earlyQuitter);

    loop.exec();

    QCOMPARE(server.socket->sslConfiguration().nextNegotiatedProtocol(),
             clientSocket.sslConfiguration().nextNegotiatedProtocol());
    QCOMPARE(server.socket->sslConfiguration().nextNegotiatedProtocol(), expectedNegotiated);
}

#if QT_CONFIG(openssl)
class PskProvider : public QObject
{
    Q_OBJECT

public:
    bool m_server;
    QByteArray m_identity;
    QByteArray m_psk;

    explicit PskProvider(QObject *parent = nullptr)
        : QObject(parent), m_server(false)
    {
    }

    void setIdentity(const QByteArray &identity)
    {
        m_identity = identity;
    }

    void setPreSharedKey(const QByteArray &psk)
    {
        m_psk = psk;
    }

public slots:
    void providePsk(QSslPreSharedKeyAuthenticator *authenticator)
    {
        QVERIFY(authenticator);
        QCOMPARE(authenticator->identityHint(), PSK_SERVER_IDENTITY_HINT);
        if (m_server)
            QCOMPARE(authenticator->maximumIdentityLength(), 0);
        else
            QVERIFY(authenticator->maximumIdentityLength() > 0);

        QVERIFY(authenticator->maximumPreSharedKeyLength() > 0);

        if (!m_identity.isEmpty()) {
            authenticator->setIdentity(m_identity);
            QCOMPARE(authenticator->identity(), m_identity);
        }

        if (!m_psk.isEmpty()) {
            authenticator->setPreSharedKey(m_psk);
            QCOMPARE(authenticator->preSharedKey(), m_psk);
        }
    }
};

class PskServer : public QTcpServer
{
    Q_OBJECT
public:
    PskServer()
        : socket(0),
          config(QSslConfiguration::defaultConfiguration()),
          ignoreSslErrors(true),
          peerVerifyMode(QSslSocket::AutoVerifyPeer),
          protocol(QSsl::TlsV1_2),
          m_pskProvider()
    {
        m_pskProvider.m_server = true;
    }
    QSslSocket *socket;
    QSslConfiguration config;
    bool ignoreSslErrors;
    QSslSocket::PeerVerifyMode peerVerifyMode;
    QSsl::SslProtocol protocol;
    QList<QSslCipher> ciphers;
    PskProvider m_pskProvider;

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        socket = new QSslSocket(this);
        socket->setSslConfiguration(config);
        socket->setPeerVerifyMode(peerVerifyMode);
        socket->setProtocol(protocol);
        if (ignoreSslErrors)
            connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));

        if (!ciphers.isEmpty()) {
            auto sslConfig = socket->sslConfiguration();
            sslConfig.setCiphers(ciphers);
            socket->setSslConfiguration(sslConfig);
        }

        QVERIFY(socket->setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState));
        QVERIFY(!socket->peerAddress().isNull());
        QVERIFY(socket->peerPort() != 0);
        QVERIFY(!socket->localAddress().isNull());
        QVERIFY(socket->localPort() != 0);

        connect(socket, &QSslSocket::preSharedKeyAuthenticationRequired, &m_pskProvider, &PskProvider::providePsk);

        socket->startServerEncryption();
    }

protected slots:
    void ignoreErrorSlot()
    {
        socket->ignoreSslErrors();
    }
};

void tst_QSslSocket::simplePskConnect_data()
{
    if (!isTestingOpenSsl)
        QSKIP("The active TLS backend does support PSK");

    QTest::addColumn<PskConnectTestType>("pskTestType");
    QTest::newRow("PskConnectDoNotHandlePsk") << PskConnectDoNotHandlePsk;
    QTest::newRow("PskConnectEmptyCredentials") << PskConnectEmptyCredentials;
    QTest::newRow("PskConnectWrongCredentials") << PskConnectWrongCredentials;
    QTest::newRow("PskConnectWrongIdentity") << PskConnectWrongIdentity;
    QTest::newRow("PskConnectWrongPreSharedKey") << PskConnectWrongPreSharedKey;
    QTest::newRow("PskConnectRightCredentialsPeerVerifyFailure") << PskConnectRightCredentialsPeerVerifyFailure;
    QTest::newRow("PskConnectRightCredentialsVerifyPeer") << PskConnectRightCredentialsVerifyPeer;
    QTest::newRow("PskConnectRightCredentialsDoNotVerifyPeer") << PskConnectRightCredentialsDoNotVerifyPeer;
}

void tst_QSslSocket::simplePskConnect()
{
    QFETCH(PskConnectTestType, pskTestType);
    QSKIP("This test requires change 1f8cab2c3bcd91335684c95afa95ae71e00a94e4 on the network test server, QTQAINFRA-917");

    if (!QSslSocket::supportsSsl())
        QSKIP("No SSL support");

    bool pskCipherFound = false;
    const QList<QSslCipher> supportedCiphers = QSslConfiguration::supportedCiphers();
    for (const QSslCipher &cipher : supportedCiphers) {
        if (cipher.name() == PSK_CIPHER_WITHOUT_AUTH) {
            pskCipherFound = true;
            break;
        }
    }

    if (!pskCipherFound)
        QSKIP("SSL implementation does not support the necessary PSK cipher(s)");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP("This test must not be going through a proxy");

    QSslSocket socket;
    this->socket = &socket;

    QSignalSpy connectedSpy(&socket, SIGNAL(connected()));
    QVERIFY(connectedSpy.isValid());

    QSignalSpy hostFoundSpy(&socket, SIGNAL(hostFound()));
    QVERIFY(hostFoundSpy.isValid());

    QSignalSpy disconnectedSpy(&socket, SIGNAL(disconnected()));
    QVERIFY(disconnectedSpy.isValid());

    QSignalSpy connectionEncryptedSpy(&socket, SIGNAL(encrypted()));
    QVERIFY(connectionEncryptedSpy.isValid());

    QSignalSpy sslErrorsSpy(&socket, SIGNAL(sslErrors(QList<QSslError>)));
    QVERIFY(sslErrorsSpy.isValid());

    QSignalSpy socketErrorsSpy(&socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)));
    QVERIFY(socketErrorsSpy.isValid());

    QSignalSpy peerVerifyErrorSpy(&socket, SIGNAL(peerVerifyError(QSslError)));
    QVERIFY(peerVerifyErrorSpy.isValid());

    QSignalSpy pskAuthenticationRequiredSpy(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)));
    QVERIFY(pskAuthenticationRequiredSpy.isValid());

    connect(&socket, SIGNAL(connected()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(disconnected()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(modeChanged(QSslSocket::SslMode)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(encrypted()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(peerVerifyError(QSslError)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(exitLoop()));

    // force a PSK cipher w/o auth
    auto sslConfig = socket.sslConfiguration();
    sslConfig.setCiphers({QSslCipher(PSK_CIPHER_WITHOUT_AUTH)});
    socket.setSslConfiguration(sslConfig);

    PskProvider provider;

    switch (pskTestType) {
    case PskConnectDoNotHandlePsk:
        // don't connect to the provider
        break;

    case PskConnectEmptyCredentials:
        // connect to the psk provider, but don't actually provide any PSK nor identity
        connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
        break;

    case PskConnectWrongCredentials:
        // provide totally wrong credentials
        provider.setIdentity(PSK_CLIENT_IDENTITY.left(PSK_CLIENT_IDENTITY.size() - 1));
        provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY.left(PSK_CLIENT_PRESHAREDKEY.size() - 1));
        connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
        break;

    case PskConnectWrongIdentity:
        // right PSK, wrong identity
        provider.setIdentity(PSK_CLIENT_IDENTITY.left(PSK_CLIENT_IDENTITY.size() - 1));
        provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY);
        connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
        break;

    case PskConnectWrongPreSharedKey:
        // right identity, wrong PSK
        provider.setIdentity(PSK_CLIENT_IDENTITY);
        provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY.left(PSK_CLIENT_PRESHAREDKEY.size() - 1));
        connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
        break;

    case PskConnectRightCredentialsPeerVerifyFailure:
        // right identity, right PSK, but since we can't verify the other peer, we'll fail
        provider.setIdentity(PSK_CLIENT_IDENTITY);
        provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY);
        connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
        break;

    case PskConnectRightCredentialsVerifyPeer:
        // right identity, right PSK, verify the peer (but ignore the failure) and establish the connection
        provider.setIdentity(PSK_CLIENT_IDENTITY);
        provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY);
        connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
        connect(&socket, SIGNAL(peerVerifyError(QSslError)), this, SLOT(ignoreErrorSlot()));
        break;

    case PskConnectRightCredentialsDoNotVerifyPeer:
        // right identity, right PSK, do not verify the peer and establish the connection
        provider.setIdentity(PSK_CLIENT_IDENTITY);
        provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY);
        connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
        socket.setPeerVerifyMode(QSslSocket::VerifyNone);
        break;
    }

    // check the peer verification mode
    switch (pskTestType) {
    case PskConnectDoNotHandlePsk:
    case PskConnectEmptyCredentials:
    case PskConnectWrongCredentials:
    case PskConnectWrongIdentity:
    case PskConnectWrongPreSharedKey:
    case PskConnectRightCredentialsPeerVerifyFailure:
    case PskConnectRightCredentialsVerifyPeer:
        QCOMPARE(socket.peerVerifyMode(), QSslSocket::AutoVerifyPeer);
        break;

    case PskConnectRightCredentialsDoNotVerifyPeer:
        QCOMPARE(socket.peerVerifyMode(), QSslSocket::VerifyNone);
        break;
    }

    // Start connecting
    socket.connectToHost(QtNetworkSettings::serverName(), PSK_SERVER_PORT);
    QCOMPARE(socket.state(), QAbstractSocket::HostLookupState);
    enterLoop(10);

    // Entered connecting state
    QCOMPARE(socket.state(), QAbstractSocket::ConnectingState);
    QCOMPARE(connectedSpy.size(), 0);
    QCOMPARE(hostFoundSpy.size(), 1);
    QCOMPARE(disconnectedSpy.size(), 0);
    enterLoop(10);

    // Entered connected state
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socket.mode(), QSslSocket::UnencryptedMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectedSpy.size(), 1);
    QCOMPARE(hostFoundSpy.size(), 1);
    QCOMPARE(disconnectedSpy.size(), 0);

    // Enter encrypted mode
    socket.startClientEncryption();
    QCOMPARE(socket.mode(), QSslSocket::SslClientMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectionEncryptedSpy.size(), 0);
    QCOMPARE(sslErrorsSpy.size(), 0);
    QCOMPARE(peerVerifyErrorSpy.size(), 0);

    // Start handshake.
    enterLoop(10);

    // We must get the PSK signal in all cases
    QCOMPARE(pskAuthenticationRequiredSpy.size(), 1);

    switch (pskTestType) {
    case PskConnectDoNotHandlePsk:
    case PskConnectEmptyCredentials:
    case PskConnectWrongCredentials:
    case PskConnectWrongIdentity:
    case PskConnectWrongPreSharedKey:
        // Handshake failure
        QCOMPARE(socketErrorsSpy.size(), 1);
        QCOMPARE(qvariant_cast<QAbstractSocket::SocketError>(socketErrorsSpy.at(0).at(0)), QAbstractSocket::SslHandshakeFailedError);
        QCOMPARE(sslErrorsSpy.size(), 0);
        QCOMPARE(peerVerifyErrorSpy.size(), 0);
        QCOMPARE(connectionEncryptedSpy.size(), 0);
        QVERIFY(!socket.isEncrypted());
        break;

    case PskConnectRightCredentialsPeerVerifyFailure:
        // Peer verification failure
        QCOMPARE(socketErrorsSpy.size(), 1);
        QCOMPARE(qvariant_cast<QAbstractSocket::SocketError>(socketErrorsSpy.at(0).at(0)), QAbstractSocket::SslHandshakeFailedError);
        QCOMPARE(sslErrorsSpy.size(), 1);
        QCOMPARE(peerVerifyErrorSpy.size(), 1);
        QCOMPARE(connectionEncryptedSpy.size(), 0);
        QVERIFY(!socket.isEncrypted());
        break;

    case PskConnectRightCredentialsVerifyPeer:
        // Peer verification failure, but ignore it and keep connecting
        QCOMPARE(socketErrorsSpy.size(), 0);
        QCOMPARE(sslErrorsSpy.size(), 1);
        QCOMPARE(peerVerifyErrorSpy.size(), 1);
        QCOMPARE(connectionEncryptedSpy.size(), 1);
        QVERIFY(socket.isEncrypted());
        QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
        break;

    case PskConnectRightCredentialsDoNotVerifyPeer:
        // No peer verification => no failure
        QCOMPARE(socketErrorsSpy.size(), 0);
        QCOMPARE(sslErrorsSpy.size(), 0);
        QCOMPARE(peerVerifyErrorSpy.size(), 0);
        QCOMPARE(connectionEncryptedSpy.size(), 1);
        QVERIFY(socket.isEncrypted());
        QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
        break;
    }

    // check writing
    switch (pskTestType) {
    case PskConnectDoNotHandlePsk:
    case PskConnectEmptyCredentials:
    case PskConnectWrongCredentials:
    case PskConnectWrongIdentity:
    case PskConnectWrongPreSharedKey:
    case PskConnectRightCredentialsPeerVerifyFailure:
        break;

    case PskConnectRightCredentialsVerifyPeer:
    case PskConnectRightCredentialsDoNotVerifyPeer:
        socket.write("Hello from Qt TLS/PSK!");
        QVERIFY(socket.waitForBytesWritten());
        break;
    }

    // disconnect
    switch (pskTestType) {
    case PskConnectDoNotHandlePsk:
    case PskConnectEmptyCredentials:
    case PskConnectWrongCredentials:
    case PskConnectWrongIdentity:
    case PskConnectWrongPreSharedKey:
    case PskConnectRightCredentialsPeerVerifyFailure:
        break;

    case PskConnectRightCredentialsVerifyPeer:
    case PskConnectRightCredentialsDoNotVerifyPeer:
        socket.disconnectFromHost();
        enterLoop(10);
        break;
    }

    QCOMPARE(socket.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(disconnectedSpy.size(), 1);
}

void tst_QSslSocket::ephemeralServerKey_data()
{
    if (!isTestingOpenSsl)
        QSKIP("The active TLS backend does not support ephemeral keys");

    QTest::addColumn<QString>("cipher");
    QTest::addColumn<bool>("emptyKey");

    QTest::newRow("ForwardSecrecyCipher") << "ECDHE-RSA-AES256-SHA" << (QSslSocket::sslLibraryVersionNumber() < 0x10002000L);
}

void tst_QSslSocket::ephemeralServerKey()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (!QSslSocket::supportsSsl() || setProxy)
        return;

    QFETCH(QString, cipher);
    QFETCH(bool, emptyKey);
    SslServer server;
    server.config.setCiphers(QList<QSslCipher>() << QSslCipher(cipher));
    QVERIFY(server.listen());
    QSslSocketPtr client = newSocket();
    socket = client.data();
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    QSignalSpy spy(client.data(), &QSslSocket::encrypted);

    client->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());
    spy.wait();

    QCOMPARE(spy.size(), 1);
    QVERIFY(server.config.ephemeralServerKey().isNull());
    QCOMPARE(client->sslConfiguration().ephemeralServerKey().isNull(), emptyKey);
}

void tst_QSslSocket::pskServer()
{
    if (!isTestingOpenSsl)
        QSKIP("The active TLS-backend does not have PSK support implemented.");

    QFETCH_GLOBAL(bool, setProxy);
    if (!QSslSocket::supportsSsl() || setProxy)
        return;

    QSslSocket socket;
    this->socket = &socket;

    QSignalSpy connectedSpy(&socket, SIGNAL(connected()));
    QVERIFY(connectedSpy.isValid());

    QSignalSpy disconnectedSpy(&socket, SIGNAL(disconnected()));
    QVERIFY(disconnectedSpy.isValid());

    QSignalSpy connectionEncryptedSpy(&socket, SIGNAL(encrypted()));
    QVERIFY(connectionEncryptedSpy.isValid());

    QSignalSpy pskAuthenticationRequiredSpy(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)));
    QVERIFY(pskAuthenticationRequiredSpy.isValid());

    connect(&socket, SIGNAL(connected()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(disconnected()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(modeChanged(QSslSocket::SslMode)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(encrypted()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(peerVerifyError(QSslError)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(exitLoop()));

    // force a PSK cipher w/o auth
    auto sslConfig = socket.sslConfiguration();
    sslConfig.setCiphers({QSslCipher(PSK_CIPHER_WITHOUT_AUTH)});
    socket.setSslConfiguration(sslConfig);

    PskProvider provider;
    provider.setIdentity(PSK_CLIENT_IDENTITY);
    provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY);
    connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
    socket.setPeerVerifyMode(QSslSocket::VerifyNone);

    PskServer server;
    server.m_pskProvider.setIdentity(provider.m_identity);
    server.m_pskProvider.setPreSharedKey(provider.m_psk);
    server.config.setPreSharedKeyIdentityHint(PSK_SERVER_IDENTITY_HINT);
    QVERIFY(server.listen());

    // Start connecting
    socket.connectToHost(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());
    enterLoop(5);

    // Entered connected state
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socket.mode(), QSslSocket::UnencryptedMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectedSpy.size(), 1);
    QCOMPARE(disconnectedSpy.size(), 0);

    // Enter encrypted mode
    socket.startClientEncryption();
    QCOMPARE(socket.mode(), QSslSocket::SslClientMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectionEncryptedSpy.size(), 0);

    // Start handshake.
    enterLoop(10);

    // We must get the PSK signal in all cases
    QCOMPARE(pskAuthenticationRequiredSpy.size(), 1);

    QCOMPARE(connectionEncryptedSpy.size(), 1);
    QVERIFY(socket.isEncrypted());
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

    // check writing
    socket.write("Hello from Qt TLS/PSK!");
    QVERIFY(socket.waitForBytesWritten());

    // disconnect
    socket.disconnectFromHost();
    enterLoop(10);

    QCOMPARE(socket.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(disconnectedSpy.size(), 1);
}

void tst_QSslSocket::signatureAlgorithm_data()
{
    if (!isTestingOpenSsl)
        QSKIP("Signature algorithms cannot be tested with a non-OpenSSL TLS backend");

    if (QSslSocket::sslLibraryVersionNumber() >= 0x10101000L) {
        // FIXME: investigate if this test makes any sense with TLS 1.3.
        QSKIP("Test is not valid for TLS 1.3/OpenSSL 1.1.1");
    }

    QTest::addColumn<QByteArrayList>("serverSigAlgPairs");
    QTest::addColumn<QSsl::SslProtocol>("serverProtocol");
    QTest::addColumn<QByteArrayList>("clientSigAlgPairs");
    QTest::addColumn<QSsl::SslProtocol>("clientProtocol");
    QTest::addColumn<QAbstractSocket::SocketState>("state");

    const QByteArray dsaSha1("DSA+SHA1");
    const QByteArray ecdsaSha1("ECDSA+SHA1");
    const QByteArray ecdsaSha512("ECDSA+SHA512");
    const QByteArray rsaSha256("RSA+SHA256");
    const QByteArray rsaSha384("RSA+SHA384");
    const QByteArray rsaSha512("RSA+SHA512");

    QTest::newRow("match_TlsV1_2")
        << QByteArrayList({rsaSha256})
        << QSsl::TlsV1_2
        << QByteArrayList({rsaSha256})
        << QSsl::AnyProtocol
        << QAbstractSocket::ConnectedState;
    QTest::newRow("no_hashalg_match_TlsV1_2")
        << QByteArrayList({rsaSha256})
        << QSsl::TlsV1_2
        << QByteArrayList({rsaSha512})
        << QSsl::AnyProtocol
        << QAbstractSocket::UnconnectedState;
    QTest::newRow("no_sigalg_match_TlsV1_2")
        << QByteArrayList({ecdsaSha512})
        << QSsl::TlsV1_2
        << QByteArrayList({rsaSha512})
        << QSsl::AnyProtocol
        << QAbstractSocket::UnconnectedState;
    QTest::newRow("no_cipher_match_AnyProtocol")
        << QByteArrayList({rsaSha512})
        << QSsl::AnyProtocol
        << QByteArrayList({ecdsaSha512})
        << QSsl::AnyProtocol
        << QAbstractSocket::UnconnectedState;
    QTest::newRow("match_multiple-choice")
        << QByteArrayList({dsaSha1, rsaSha256, rsaSha384, rsaSha512})
        << QSsl::AnyProtocol
        << QByteArrayList({ecdsaSha1, rsaSha384, rsaSha512, ecdsaSha512})
        << QSsl::AnyProtocol
        << QAbstractSocket::ConnectedState;
    QTest::newRow("match_client_longer")
        << QByteArrayList({dsaSha1, rsaSha256})
        << QSsl::AnyProtocol
        << QByteArrayList({ecdsaSha1, ecdsaSha512, rsaSha256})
        << QSsl::AnyProtocol
        << QAbstractSocket::ConnectedState;
    QTest::newRow("match_server_longer")
        << QByteArrayList({ecdsaSha1, ecdsaSha512, rsaSha256})
        << QSsl::AnyProtocol
        << QByteArrayList({dsaSha1, rsaSha256})
        << QSsl::AnyProtocol
        << QAbstractSocket::ConnectedState;

    // signature algorithms do not match, but are ignored because the tls version is not v1.2
    QTest::newRow("client_ignore_TlsV1_1")
        << QByteArrayList({rsaSha256})
        << Test::TlsV1_1
        << QByteArrayList({rsaSha512})
        << QSsl::AnyProtocol
        << QAbstractSocket::ConnectedState;
    QTest::newRow("server_ignore_TlsV1_1")
        << QByteArrayList({rsaSha256})
        << QSsl::AnyProtocol
        << QByteArrayList({rsaSha512})
        << Test::TlsV1_1
        << QAbstractSocket::ConnectedState;
    QTest::newRow("client_ignore_TlsV1_0")
        << QByteArrayList({rsaSha256})
        << Test::TlsV1_0
        << QByteArrayList({rsaSha512})
        << QSsl::AnyProtocol
        << QAbstractSocket::ConnectedState;
    QTest::newRow("server_ignore_TlsV1_0")
        << QByteArrayList({rsaSha256})
        << QSsl::AnyProtocol
        << QByteArrayList({rsaSha512})
        << Test::TlsV1_0
        << QAbstractSocket::ConnectedState;
}

void tst_QSslSocket::signatureAlgorithm()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP("Test not adapted for use with proxying");

    QFETCH(QByteArrayList, serverSigAlgPairs);
    QFETCH(QSsl::SslProtocol, serverProtocol);
    QFETCH(QByteArrayList, clientSigAlgPairs);
    QFETCH(QSsl::SslProtocol, clientProtocol);
    QFETCH(QAbstractSocket::SocketState, state);

    SslServer server;
    server.protocol = serverProtocol;
    server.config.setCiphers({QSslCipher("ECDHE-RSA-AES256-SHA")});
    server.config.setBackendConfigurationOption(QByteArrayLiteral("SignatureAlgorithms"), serverSigAlgPairs.join(':'));
    QVERIFY(server.listen());

    QSslConfiguration clientConfig = QSslConfiguration::defaultConfiguration();
    clientConfig.setProtocol(clientProtocol);
    clientConfig.setBackendConfigurationOption(QByteArrayLiteral("SignatureAlgorithms"), clientSigAlgPairs.join(':'));
    QSslSocket client;
    client.setSslConfiguration(clientConfig);
    socket = &client;

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    connect(socket, &QAbstractSocket::errorOccurred, &loop, &QEventLoop::quit);
    connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &tst_QSslSocket::ignoreErrorSlot);
    connect(socket, &QSslSocket::encrypted, &loop, &QEventLoop::quit);

    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());
    loop.exec();
    socket = nullptr;
    QCOMPARE(client.state(), state);
}

void tst_QSslSocket::forwardReadChannelFinished()
{
    if (!isTestingOpenSsl)
        QSKIP("This test requires the OpenSSL backend");

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP("This test doesn't work via a proxy");

    QSslSocket socket;
    QSignalSpy readChannelFinishedSpy(&socket, &QAbstractSocket::readChannelFinished);
    connect(&socket, &QSslSocket::encrypted, [&socket]() {
        const auto data = QString("GET /ip HTTP/1.0\r\nHost: %1\r\n\r\nAccept: */*\r\n\r\n")
                .arg(QtNetworkSettings::serverLocalName()).toUtf8();
        socket.write(data);
    });
    connect(&socket, &QSslSocket::readChannelFinished,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    socket.connectToHostEncrypted(QtNetworkSettings::httpServerName(), 443);
    enterLoop(10);
    QVERIFY(readChannelFinishedSpy.size());
}

#endif // QT_CONFIG(openssl)

void tst_QSslSocket::unsupportedProtocols_data()
{
    QTest::addColumn<QSsl::SslProtocol>("unsupportedProtocol");
    QTest::newRow("DtlsV1_0") << Test::DtlsV1_0;
    QTest::newRow("DtlsV1_2") << QSsl::DtlsV1_2;
    QTest::newRow("DtlsV1_0OrLater") << Test::DtlsV1_0OrLater;
    QTest::newRow("DtlsV1_2OrLater") << QSsl::DtlsV1_2OrLater;
    QTest::newRow("UnknownProtocol") << QSsl::UnknownProtocol;
}

void tst_QSslSocket::unsupportedProtocols()
{
    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy)
        return;

    QFETCH(const QSsl::SslProtocol, unsupportedProtocol);
    constexpr auto timeout = 500ms;
    // Test a client socket.
    {
        // 0. connectToHostEncrypted: client-side, non-blocking API, error is discovered
        // early, preventing any real connection from ever starting.
        QSslSocket socket;
        socket.setProtocol(unsupportedProtocol);
        QCOMPARE(socket.error(), QAbstractSocket::UnknownSocketError);
        socket.connectToHostEncrypted(QStringLiteral("doesnotmatter.org"), 1010);
        QCOMPARE(socket.error(), QAbstractSocket::SslInvalidUserDataError);
        QCOMPARE(socket.state(), QAbstractSocket::UnconnectedState);
    }
    {
        // 1. startClientEncryption: client-side, non blocking API, but wants a socket in
        // the 'connected' state (otherwise just returns false not setting any error code).
        SslServer server;
        QVERIFY(server.listen());

        QSslSocket socket;
        QCOMPARE(socket.error(), QAbstractSocket::UnknownSocketError);

        socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
        QVERIFY(socket.waitForConnected(int(timeout.count())));

        socket.setProtocol(unsupportedProtocol);
        socket.startClientEncryption();
        QCOMPARE(socket.error(), QAbstractSocket::SslInvalidUserDataError);
    }
    {
        // 2. waitForEncrypted: client-side, blocking API plus requires from us
        // to call ... connectToHostEncrypted(), which will notice an error and
        // will prevent any connect at all. Nothing to test.
    }

    // Test a server side, relatively simple: server does not connect, it listens/accepts
    // and then calls startServerEncryption() (which must fall).
    {
        SslServer server;
        server.protocol = unsupportedProtocol;
        QVERIFY(server.listen());

        QTestEventLoop loop;
        connect(&server, &SslServer::socketError, [&loop](QAbstractSocket::SocketError)
                {loop.exitLoop();});

        QTcpSocket client;
        client.connectToHost(QHostAddress::LocalHost, server.serverPort());
        loop.enterLoop(timeout);
        QVERIFY(!loop.timeout());
        QVERIFY(server.socket);
        QCOMPARE(server.socket->error(), QAbstractSocket::SslInvalidUserDataError);
    }
}

void tst_QSslSocket::oldErrorsOnSocketReuse()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return; // not relevant
    SslServer server;
    if (!isTestingOpenSsl)
        server.protocol = Test::TlsV1_1;
    server.m_certFile = testDataDir + "certs/fluke.cert";
    server.m_keyFile = testDataDir + "certs/fluke.key";
    QVERIFY(server.listen(QHostAddress::SpecialAddress::LocalHost));

    QSslSocket socket;
    if (!isTestingOpenSsl)
        socket.setProtocol(Test::TlsV1_1);
    QList<QSslError> errorList;
    auto connection = connect(&socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors),
        [&socket, &errorList](const QList<QSslError> &errors) {
            errorList += errors;
            socket.ignoreSslErrors(errors);
            socket.resume();
    });

    socket.connectToHostEncrypted(QString::fromLatin1("localhost"), server.serverPort());
    QVERIFY(QTest::qWaitFor([&socket](){ return socket.isEncrypted(); }));
    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        QVERIFY(QTest::qWaitFor(
            [&socket](){
                return socket.state() == QAbstractSocket::UnconnectedState;
        }));
    }

    auto oldList = errorList;
    errorList.clear();
    server.close();
    server.m_certFile = testDataDir + "certs/bogus-client.crt";
    server.m_keyFile = testDataDir + "certs/bogus-client.key";
    QVERIFY(server.listen(QHostAddress::SpecialAddress::LocalHost));

    socket.connectToHostEncrypted(QString::fromLatin1("localhost"), server.serverPort());
    QVERIFY(QTest::qWaitFor([&socket](){ return socket.isEncrypted(); }));

    for (const auto &error : oldList) {
        QVERIFY2(!errorList.contains(error),
            "The new errors should not contain any of the old ones");
    }
}

#if QT_CONFIG(openssl)

void tst_QSslSocket::alertMissingCertificate()
{
    // In this test we want a server to abort the connection due to the failing
    // client authentication. The server expected to send an alert before closing
    // the connection, and the client expected to receive this alert and report it.
    if (!isTestingOpenSsl)
        QSKIP("This test requires the OpenSSL backend");

    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy) // Not what we test here, bail out.
        return;

    SslServer server;
    if (!server.listen(QHostAddress::LocalHost))
        QSKIP("SslServer::listen() returned false");

    // We want a certificate request to be sent to the client:
    server.peerVerifyMode = QSslSocket::VerifyPeer;
    // The only way we can force OpenSSL to send an alert - is to use
    // a special option (so we fail before handshake is finished):
    server.config.setMissingCertificateIsFatal(true);

    QSslSocket clientSocket;
    connect(&clientSocket, &QSslSocket::sslErrors, [&clientSocket](const QList<QSslError> &errors){
        clientSocket.ignoreSslErrors(errors);
    });

    QSignalSpy serverSpy(&server, &SslServer::alertSent);
    QSignalSpy clientSpy(&clientSocket, &QSslSocket::alertReceived);

    clientSocket.connectToHostEncrypted(server.serverAddress().toString(), server.serverPort());

    QTestEventLoop runner;
    auto *context = &runner;
    QTimer::singleShot(500, context, [&runner](){
        runner.exitLoop();
    });

    int waitFor = 2;
    auto earlyQuitter = [&runner, &waitFor](QAbstractSocket::SocketError) {
        if (!--waitFor)
            runner.exitLoop();
    };

    // Presumably, RemoteHostClosedError for the client and SslHandshakeError
    // for the server:
    connect(&clientSocket, &QAbstractSocket::errorOccurred, earlyQuitter);
    connect(&server, &SslServer::socketError, earlyQuitter);

    runner.enterLoop(1s);

    if (clientSocket.isEncrypted()) {
        // When using TLS 1.3 the client side thinks it is connected very
        // quickly, before the server has finished processing. So wait for the
        // inevitable disconnect.
        QCOMPARE(clientSocket.sessionProtocol(), QSsl::TlsV1_3);
        connect(&clientSocket, &QSslSocket::disconnected, &runner, &QTestEventLoop::exitLoop);
        runner.enterLoop(10s);
    }

    QVERIFY(serverSpy.size() > 0);
    QVERIFY(clientSpy.size() > 0);
    QVERIFY(server.socket && !server.socket->isEncrypted());
    QVERIFY(!clientSocket.isEncrypted());
}

void tst_QSslSocket::alertInvalidCertificate()
{
    // In this test a client will not ignore verification errors,
    // it also will do 'early' checks, meaning the reported and
    // not ignored _during_ the hanshake, not after. This ensures
    // OpenSSL sends an alert.
    if (!isTestingOpenSsl)
        QSKIP("This test requires the OpenSSL backend");

    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy) // Not what we test here, bail out.
        return;

    SslServer server;
    if (!server.listen(QHostAddress::LocalHost))
        QSKIP("SslServer::listen() returned false");

    QSslSocket clientSocket;
    auto configuration = QSslConfiguration::defaultConfiguration();
    configuration.setHandshakeMustInterruptOnError(true);
    QVERIFY(configuration.handshakeMustInterruptOnError());
    clientSocket.setSslConfiguration(configuration);

    QSignalSpy serverSpy(&server, &SslServer::gotAlert);
    QSignalSpy clientSpy(&clientSocket, &QSslSocket::alertSent);
    QSignalSpy interruptedSpy(&clientSocket, &QSslSocket::handshakeInterruptedOnError);

    clientSocket.connectToHostEncrypted(server.serverAddress().toString(), server.serverPort());

    QTestEventLoop runner;
    auto *context = &runner;
    QTimer::singleShot(500, context, [&runner](){
        runner.exitLoop();
    });

    int waitFor = 2;
    auto earlyQuitter = [&runner, &waitFor](QAbstractSocket::SocketError) {
        if (!--waitFor)
            runner.exitLoop();
    };

    // Presumably, RemoteHostClosedError for the server and SslHandshakeError
    // for the client:
    connect(&clientSocket, &QAbstractSocket::errorOccurred, earlyQuitter);
    connect(&server, &SslServer::socketError, earlyQuitter);

    runner.enterLoop(1s);

    QVERIFY(serverSpy.size() > 0);
    QVERIFY(clientSpy.size() > 0);
    QVERIFY(interruptedSpy.size() > 0);
    QVERIFY(server.socket && !server.socket->isEncrypted());
    QVERIFY(!clientSocket.isEncrypted());
}

void tst_QSslSocket::selfSignedCertificates_data()
{
    if (!isTestingOpenSsl)
        QSKIP("The active TLS backend does not detect the required error");

    QTest::addColumn<bool>("clientKnown");

    QTest::newRow("Client known") << true;
    QTest::newRow("Client unknown") << false;
}

void tst_QSslSocket::selfSignedCertificates()
{
    // In this test we want to check the behavior of the client/server when
    // self-signed certificates are used and the client is un/known to the server.
    QFETCH(bool, clientKnown);

    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy) // Not what we test here, bail out.
        return;

    SslServer server(testDataDir + "certs/selfsigned-server.key",
                     testDataDir + "certs/selfsigned-server.crt");
    server.protocol = QSsl::TlsV1_2;
    server.ignoreSslErrors = false;
    server.peerVerifyMode = QSslSocket::VerifyPeer;

    if (!server.listen(QHostAddress::LocalHost))
        QSKIP("SslServer::listen() returned false");

    QFile clientFile(testDataDir + "certs/selfsigned-client.key");
    QVERIFY(clientFile.open(QIODevice::ReadOnly));
    QSslKey clientKey(clientFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QSslCertificate clientCert
        = QSslCertificate::fromPath(testDataDir + "certs/selfsigned-client.crt").first();

    server.config.setCiphers({QSslCipher("DHE-RSA-AES256-SHA256")});
    server.config.setHandshakeMustInterruptOnError(true);
    server.config.setMissingCertificateIsFatal(true);
    if (clientKnown)
        server.config.setCaCertificates({clientCert});

    connect(&server, &SslServer::sslErrors,
            [](const QList<QSslError> &errors) {
                QCOMPARE(errors.size(), 1);
                QVERIFY(errors.first().error() == QSslError::SelfSignedCertificate);
            }
    );
    connect(&server, &SslServer::socketError,
            [](QAbstractSocket::SocketError socketError) {
                QVERIFY(socketError == QAbstractSocket::SslHandshakeFailedError);
            }
    );
    connect(&server, &SslServer::handshakeInterruptedOnError,
            [&server](const QSslError& error) {
                QVERIFY(error.error() == QSslError::SelfSignedCertificate);
                server.socket->continueInterruptedHandshake();
            }
    );

    QSslSocket clientSocket;
    auto configuration = QSslConfiguration::defaultConfiguration();
    configuration.setProtocol(QSsl::TlsV1_2);
    configuration.setCiphers({QSslCipher("DHE-RSA-AES256-SHA256")});
    configuration.setPrivateKey(clientKey);
    configuration.setLocalCertificate(clientCert);
    configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    configuration.setHandshakeMustInterruptOnError(true);
    configuration.setMissingCertificateIsFatal(true);
    clientSocket.setSslConfiguration(configuration);

    connect(&clientSocket, &QSslSocket::sslErrors,
            [&clientSocket](const QList<QSslError> &errors) {
                for (const auto &error : errors) {
                    if (error.error() == QSslError::HostNameMismatch) {
                        QVERIFY(errors.size() == 2);
                        clientSocket.ignoreSslErrors(errors);
                    } else {
                        QVERIFY(error.error() == QSslError::SelfSignedCertificate);
                    }
                }
            }
    );
    connect(&clientSocket, &QAbstractSocket::errorOccurred,
            [](QAbstractSocket::SocketError socketError) {
                QVERIFY(socketError == QAbstractSocket::RemoteHostClosedError);
            }
    );
    connect(&clientSocket, &QSslSocket::handshakeInterruptedOnError,
            [&clientSocket](const QSslError& error) {
                QVERIFY(error.error() == QSslError::SelfSignedCertificate);
                clientSocket.continueInterruptedHandshake();
            }
    );

    QSignalSpy serverSpy(&server, &SslServer::alertSent);
    QSignalSpy clientSpy(&clientSocket, &QSslSocket::alertReceived);

    clientSocket.connectToHostEncrypted(server.serverAddress().toString(), server.serverPort());

    QTestEventLoop runner;
    auto *context = &runner;
    QTimer::singleShot(500, context,
                       [&runner]() {
                           runner.exitLoop();
                       }
    );

    int waitFor = 2;
    auto earlyQuitter = [&runner, &waitFor](QAbstractSocket::SocketError) {
        if (!--waitFor)
            runner.exitLoop();
    };

    // Presumably, RemoteHostClosedError for the client and SslHandshakeError
    // for the server:
    connect(&clientSocket, &QAbstractSocket::errorOccurred, earlyQuitter);
    connect(&server, &SslServer::socketError, earlyQuitter);

    runner.enterLoop(1s);

    if (clientKnown) {
        QCOMPARE(serverSpy.size(), 0);
        QCOMPARE(clientSpy.size(), 0);
        QVERIFY(server.socket && server.socket->isEncrypted());
        QVERIFY(clientSocket.isEncrypted());
    } else {
        QVERIFY(serverSpy.size() > 0);
        QEXPECT_FAIL("", "Failing to trigger signal, QTBUG-81661", Continue);
        QVERIFY(clientSpy.size() > 0);
        QVERIFY(server.socket && !server.socket->isEncrypted());
        QVERIFY(!clientSocket.isEncrypted());
    }
}

void tst_QSslSocket::pskHandshake_data()
{
    if (!isTestingOpenSsl)
        QSKIP("The active TLS backend does not support PSK");

    QTest::addColumn<bool>("pskRight");

    QTest::newRow("Psk right") << true;
    QTest::newRow("Psk wrong") << false;
}

void tst_QSslSocket::pskHandshake()
{
    // In this test we want to check the behavior of the
    // client/server when a preshared key (right/wrong) is used.
    QFETCH(bool, pskRight);

    QFETCH_GLOBAL(const bool, setProxy);
    if (setProxy) // Not what we test here, bail out.
        return;

    SslServer server(testDataDir + "certs/selfsigned-server.key",
                     testDataDir + "certs/selfsigned-server.crt");
    server.protocol = QSsl::TlsV1_2;
    server.ignoreSslErrors = false;
    server.peerVerifyMode = QSslSocket::VerifyPeer;

    if (!server.listen(QHostAddress::LocalHost))
        QSKIP("SslServer::listen() returned false");

    QFile clientFile(testDataDir + "certs/selfsigned-client.key");
    QVERIFY(clientFile.open(QIODevice::ReadOnly));
    QSslKey clientKey(clientFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QSslCertificate clientCert
        = QSslCertificate::fromPath(testDataDir + "certs/selfsigned-client.crt").first();

    server.config.setCiphers({QSslCipher("RSA-PSK-AES128-CBC-SHA256")});
    server.config.setHandshakeMustInterruptOnError(true);
    server.config.setMissingCertificateIsFatal(true);

    connect(&server, &SslServer::sslErrors,
            [&server](const QList<QSslError> &errors) {
                QCOMPARE(errors.size(), 1);
                QVERIFY(errors.first().error() == QSslError::SelfSignedCertificate);
                server.socket->ignoreSslErrors(errors);
            }
    );
    connect(&server, &SslServer::socketError,
            [](QAbstractSocket::SocketError socketError) {
                QVERIFY(socketError == QAbstractSocket::SslHandshakeFailedError);
            }
    );
    connect(&server, &SslServer::handshakeInterruptedOnError,
            [&server](const QSslError& error) {
                QVERIFY(error.error() == QSslError::SelfSignedCertificate);
                server.socket->continueInterruptedHandshake();
            }
    );

    QSslSocket clientSocket;
    auto configuration = QSslConfiguration::defaultConfiguration();
    configuration.setProtocol(QSsl::TlsV1_2);
    configuration.setCiphers({QSslCipher("RSA-PSK-AES128-CBC-SHA256")});
    configuration.setPrivateKey(clientKey);
    configuration.setLocalCertificate(clientCert);
    configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    configuration.setHandshakeMustInterruptOnError(true);
    configuration.setMissingCertificateIsFatal(true);
    clientSocket.setSslConfiguration(configuration);

    connect(&clientSocket, &QSslSocket::preSharedKeyAuthenticationRequired,
            [pskRight](QSslPreSharedKeyAuthenticator *authenticator) {
                authenticator->setPreSharedKey(pskRight ? "123456": "654321");
            }
    );

    connect(&clientSocket, &QSslSocket::sslErrors,
            [&clientSocket](const QList<QSslError> &errors) {
                for (const auto &error : errors) {
                    if (error.error() == QSslError::HostNameMismatch) {
                        QVERIFY(errors.size() == 2);
                        clientSocket.ignoreSslErrors(errors);
                    } else {
                        QVERIFY(error.error() == QSslError::SelfSignedCertificate);
                    }
                }
            }
    );
    connect(&clientSocket, &QAbstractSocket::errorOccurred,
            [](QAbstractSocket::SocketError socketError) {
                QVERIFY(socketError == QAbstractSocket::SslHandshakeFailedError);
            }
    );
    connect(&clientSocket, &QSslSocket::handshakeInterruptedOnError,
            [&clientSocket](const QSslError& error) {
                QVERIFY(error.error() == QSslError::SelfSignedCertificate);
                clientSocket.continueInterruptedHandshake();
            }
    );

    QSignalSpy serverSpy(&server, &SslServer::alertSent);
    QSignalSpy clientSpy(&clientSocket, &QSslSocket::alertReceived);

    clientSocket.connectToHostEncrypted(server.serverAddress().toString(), server.serverPort());

    QTestEventLoop runner;
    auto *context = &runner;
    QTimer::singleShot(500, context, [&runner]() {
        runner.exitLoop();
    });

    int waitFor = 2;
    auto earlyQuitter = [&runner, &waitFor](QAbstractSocket::SocketError) {
        if (!--waitFor)
            runner.exitLoop();
    };

    // Presumably, RemoteHostClosedError for the client and SslHandshakeError
    // for the server:
    connect(&clientSocket, &QAbstractSocket::errorOccurred, earlyQuitter);
    connect(&server, &SslServer::socketError, earlyQuitter);

    runner.enterLoop(1s);

    if (pskRight) {
        QCOMPARE(serverSpy.size(), 0);
        QCOMPARE(clientSpy.size(), 0);
        QVERIFY(server.socket && server.socket->isEncrypted());
        QVERIFY(clientSocket.isEncrypted());
    } else {
        QVERIFY(serverSpy.size() > 0);
        QCOMPARE(serverSpy.first().at(0).toInt(), static_cast<int>(QSsl::AlertLevel::Fatal));
        QCOMPARE(serverSpy.first().at(1).toInt(), static_cast<int>(QSsl::AlertType::BadRecordMac));
        QVERIFY(clientSpy.size() > 0);
        QCOMPARE(clientSpy.first().at(0).toInt(), static_cast<int>(QSsl::AlertLevel::Fatal));
        QCOMPARE(clientSpy.first().at(1).toInt(), static_cast<int>(QSsl::AlertType::BadRecordMac));
        QVERIFY(server.socket && !server.socket->isEncrypted());
        QVERIFY(!clientSocket.isEncrypted());
    }
}

#endif // QT_CONFIG(openssl)
#endif // QT_CONFIG(ssl)


QTEST_MAIN(tst_QSslSocket)

#include "tst_qsslsocket.moc"
