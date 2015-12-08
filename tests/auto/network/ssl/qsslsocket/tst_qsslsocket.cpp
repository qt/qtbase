/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2014 Governikus GmbH & Co. KG.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qglobal.h>
#include <QtCore/qthread.h>
#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qhostinfo.h>
#include <QtNetwork/qnetworkproxy.h>
#include <QtNetwork/qsslcipher.h>
#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qsslkey.h>
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qsslpresharedkeyauthenticator.h>
#include <QtTest/QtTest>

#include <QNetworkProxy>
#include <QAuthenticator>

#include "private/qhostinfo_p.h"
#include "private/qiodevice_p.h" // for QIODEVICE_BUFFERSIZE

#include "../../../network-settings.h"

#ifndef QT_NO_SSL
#ifndef QT_NO_OPENSSL
#include "private/qsslsocket_openssl_p.h"
#include "private/qsslsocket_openssl_symbols_p.h"
#endif
#include "private/qsslsocket_p.h"
#include "private/qsslconfiguration_p.h"

Q_DECLARE_METATYPE(QSslSocket::SslMode)
typedef QList<QSslError::SslError> SslErrorList;
Q_DECLARE_METATYPE(SslErrorList)
Q_DECLARE_METATYPE(QSslError)
Q_DECLARE_METATYPE(QSslKey)
Q_DECLARE_METATYPE(QSsl::SslProtocol)
Q_DECLARE_METATYPE(QSslSocket::PeerVerifyMode);
typedef QSharedPointer<QSslSocket> QSslSocketPtr;

// Non-OpenSSL backends are not able to report a specific error code
// for self-signed certificates.
#ifndef QT_NO_OPENSSL
#define FLUKE_CERTIFICATE_ERROR QSslError::SelfSignedCertificate
#else
#define FLUKE_CERTIFICATE_ERROR QSslError::CertificateUntrusted
#endif
#endif // QT_NO_SSL

#if defined Q_OS_HPUX && defined Q_CC_GNU
// This error is delivered every time we try to use the fluke CA
// certificate. For now we work around this bug. Task 202317.
#define QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
#endif

// Use this cipher to force PSK key sharing.
// Also, it's a cipher w/o auth, to check that we emit the signals warning
// about the identity of the peer.
static const QString PSK_CIPHER_WITHOUT_AUTH = QStringLiteral("PSK-AES256-CBC-SHA");
static const quint16 PSK_SERVER_PORT = 4433;
static const QByteArray PSK_CLIENT_PRESHAREDKEY = QByteArrayLiteral("\x1a\x2b\x3c\x4d\x5e\x6f");
static const QByteArray PSK_SERVER_IDENTITY_HINT = QByteArrayLiteral("QtTestServerHint");
static const QByteArray PSK_CLIENT_IDENTITY = QByteArrayLiteral("Client_identity");

class tst_QSslSocket : public QObject
{
    Q_OBJECT

    int proxyAuthCalled;

public:
    tst_QSslSocket();
    virtual ~tst_QSslSocket();

    static void enterLoop(int secs)
    {
        ++loopLevel;
        QTestEventLoop::instance().enterLoop(secs);
    }

    static bool timeout()
    {
        return QTestEventLoop::instance().timeout();
    }

#ifndef QT_NO_SSL
    QSslSocketPtr newSocket();

#ifndef QT_NO_OPENSSL
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
#endif
#endif

public slots:
    void initTestCase_data();
    void initTestCase();
    void init();
    void cleanup();
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth);
#endif

#ifndef QT_NO_SSL
private slots:
    void constructing();
    void simpleConnect();
    void simpleConnectWithIgnore();

    // API tests
    void sslErrors_data();
    void sslErrors();
    void addCaCertificate();
    void addCaCertificates();
    void addCaCertificates2();
    void ciphers();
    void connectToHostEncrypted();
    void connectToHostEncryptedWithVerificationPeerName();
    void sessionCipher();
    void flush();
    void isEncrypted();
    void localCertificate();
    void mode();
    void peerCertificate();
    void peerCertificateChain();
    void privateKey();
#ifndef QT_NO_OPENSSL
    void privateKeyOpaque();
#endif
    void protocol();
    void protocolServerSide_data();
    void protocolServerSide();
#ifndef QT_NO_OPENSSL
    void serverCipherPreferences();
#endif // QT_NO_OPENSSL
    void setCaCertificates();
    void setLocalCertificate();
    void localCertificateChain();
    void setLocalCertificateChain();
    void setPrivateKey();
    void setSocketDescriptor();
    void setSslConfiguration_data();
    void setSslConfiguration();
    void waitForEncrypted();
    void waitForEncryptedMinusOne();
    void waitForConnectedEncryptedReadyRead();
    void startClientEncryption();
    void startServerEncryption();
    void addDefaultCaCertificate();
    void addDefaultCaCertificates();
    void addDefaultCaCertificates2();
    void defaultCaCertificates();
    void defaultCiphers();
    void resetDefaultCiphers();
    void setDefaultCaCertificates();
    void setDefaultCiphers();
    void supportedCiphers();
    void systemCaCertificates();
    void wildcardCertificateNames();
    void wildcard();
    void setEmptyKey();
    void spontaneousWrite();
    void setReadBufferSize();
    void setReadBufferSize_task_250027();
    void waitForMinusOne();
    void verifyMode();
    void verifyDepth();
    void disconnectFromHostWhenConnecting();
    void disconnectFromHostWhenConnected();
    void resetProxy();
    void ignoreSslErrorsList_data();
    void ignoreSslErrorsList();
    void ignoreSslErrorsListWithSlot_data();
    void ignoreSslErrorsListWithSlot();
    void readFromClosedSocket();
    void writeBigChunk();
    void blacklistedCertificates();
    void versionAccessors();
#ifndef QT_NO_OPENSSL
    void sslOptions();
#endif
    void encryptWithoutConnecting();
    void resume_data();
    void resume();
    void qtbug18498_peek();
    void qtbug18498_peek2();
    void dhServer();
    void ecdhServer();
    void verifyClientCertificate_data();
    void verifyClientCertificate();
    void setEmptyDefaultConfiguration(); // this test should be last

#ifndef QT_NO_OPENSSL
    void simplePskConnect_data();
    void simplePskConnect();
#endif

    static void exitLoop()
    {
        // Safe exit - if we aren't in an event loop, don't
        // exit one.
        if (loopLevel > 0) {
            --loopLevel;
            QTestEventLoop::instance().exitLoop();
        }
    }

protected slots:
    void ignoreErrorSlot()
    {
        socket->ignoreSslErrors();
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
#endif // QT_NO_SSL
private:
    static int loopLevel;
};

#ifndef QT_NO_SSL
#ifndef QT_NO_OPENSSL
Q_DECLARE_METATYPE(tst_QSslSocket::PskConnectTestType)
#endif
#endif

int tst_QSslSocket::loopLevel = 0;

tst_QSslSocket::tst_QSslSocket()
{
#ifndef QT_NO_SSL
    qRegisterMetaType<QList<QSslError> >("QList<QSslError>");
    qRegisterMetaType<QSslError>("QSslError");
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");

#ifndef QT_NO_OPENSSL
    qRegisterMetaType<QSslPreSharedKeyAuthenticator *>();
    qRegisterMetaType<tst_QSslSocket::PskConnectTestType>();
#endif
#endif
}

tst_QSslSocket::~tst_QSslSocket()
{
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
#ifndef QT_NO_SSL
    qDebug("Using SSL library %s (%ld)",
           qPrintable(QSslSocket::sslLibraryVersionString()),
           QSslSocket::sslLibraryVersionNumber());
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
#endif
}

void tst_QSslSocket::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        QString fluke = QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().first().toString();
        QNetworkProxy proxy;

        switch (proxyType) {
        case Socks5Proxy:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, fluke, 1080);
            break;

        case Socks5Proxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, fluke, 1081);
            break;

        case HttpProxy | NoAuth:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, fluke, 3128);
            break;

        case HttpProxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, fluke, 3129);
            break;

        case HttpProxy | AuthNtlm:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, fluke, 3130);
            break;
        }
        QNetworkProxy::setApplicationProxy(proxy);
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }

    qt_qhostinfo_clear_cache();
}

void tst_QSslSocket::cleanup()
{
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
#endif
}

#ifndef QT_NO_SSL
QSslSocketPtr tst_QSslSocket::newSocket()
{
    QSslSocket *socket = new QSslSocket;

    proxyAuthCalled = 0;
    connect(socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            Qt::DirectConnection);

    return QSslSocketPtr(socket);
}
#endif

#ifndef QT_NO_NETWORKPROXY
void tst_QSslSocket::proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
{
    ++proxyAuthCalled;
    auth->setUser("qsockstest");
    auth->setPassword("password");
}
#endif // !QT_NO_NETWORKPROXY

#ifndef QT_NO_SSL

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
    QTest::ignoreMessage(QtWarningMsg, readNotOpenMessage);
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
    QTest::ignoreMessage(QtWarningMsg, "QIODevice::readLine (QSslSocket): Called with maxSize < 2");
    QCOMPARE(socket.readLine(0, 0), qint64(-1));
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
#ifndef QT_NO_NETWORKPROXY
    QCOMPARE(socket.proxy().type(), QNetworkProxy::DefaultProxy);
#endif
    QCOMPARE(socket.readBufferSize(), qint64(0));
    QCOMPARE(socket.socketDescriptor(), (qintptr)-1);
    QCOMPARE(socket.socketType(), QAbstractSocket::TcpSocket);
    QVERIFY(!socket.waitForConnected(10));
    QTest::ignoreMessage(QtWarningMsg, "QSslSocket::waitForDisconnected() is not allowed in UnconnectedState");
    QVERIFY(!socket.waitForDisconnected(10));
    QCOMPARE(socket.protocol(), QSsl::SecureProtocols);

    QSslConfiguration savedDefault = QSslConfiguration::defaultConfiguration();

    // verify that changing the default config doesn't affect this socket
    // (on Unix, the ca certs might be empty, depending on whether we load
    // them on demand or not, so set them explicitly)
    socket.setCaCertificates(QSslSocket::systemCaCertificates());
    QSslSocket::setDefaultCaCertificates(QList<QSslCertificate>());
    QSslSocket::setDefaultCiphers(QList<QSslCipher>());
    QVERIFY(!socket.caCertificates().isEmpty());
    QVERIFY(!socket.ciphers().isEmpty());

    // verify the default as well:
    QVERIFY(QSslConfiguration::defaultConfiguration().caCertificates().isEmpty());
    QVERIFY(QSslConfiguration::defaultConfiguration().ciphers().isEmpty());

    QSslConfiguration::setDefaultConfiguration(savedDefault);
}

void tst_QSslSocket::simpleConnect()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QSslSocket socket;
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
    socket.connectToHost(QtNetworkSettings::serverName(), 993);
    QCOMPARE(socket.state(), QAbstractSocket::HostLookupState);
    enterLoop(10);

    // Entered connecting state
    QCOMPARE(socket.state(), QAbstractSocket::ConnectingState);
    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(hostFoundSpy.count(), 1);
    QCOMPARE(disconnectedSpy.count(), 0);
    enterLoop(10);

    // Entered connected state
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socket.mode(), QSslSocket::UnencryptedMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(hostFoundSpy.count(), 1);
    QCOMPARE(disconnectedSpy.count(), 0);

    // Enter encrypted mode
    socket.startClientEncryption();
    QCOMPARE(socket.mode(), QSslSocket::SslClientMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectionEncryptedSpy.count(), 0);
    QCOMPARE(sslErrorsSpy.count(), 0);

    // Starting handshake
    enterLoop(10);
    QCOMPARE(sslErrorsSpy.count(), 1);
    QCOMPARE(connectionEncryptedSpy.count(), 0);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(socket.state(), QAbstractSocket::UnconnectedState);
}

void tst_QSslSocket::simpleConnectWithIgnore()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QSslSocket socket;
    this->socket = &socket;
    QSignalSpy encryptedSpy(&socket, SIGNAL(encrypted()));
    QSignalSpy sslErrorsSpy(&socket, SIGNAL(sslErrors(QList<QSslError>)));

    connect(&socket, SIGNAL(readyRead()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(encrypted()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(connected()), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(exitLoop()));

    // Start connecting
    socket.connectToHost(QtNetworkSettings::serverName(), 993);
    QVERIFY(socket.state() != QAbstractSocket::UnconnectedState); // something must be in progress
    enterLoop(10);

    // Start handshake
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    socket.startClientEncryption();
    enterLoop(10);

    // Done; encryption should be enabled.
    QCOMPARE(sslErrorsSpy.count(), 1);
    QVERIFY(socket.isEncrypted());
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(encryptedSpy.count(), 1);

    // Wait for incoming data
    if (!socket.canReadLine())
        enterLoop(10);

    QByteArray data = socket.readAll();
    socket.disconnectFromHost();
    QVERIFY2(QtNetworkSettings::compareReplyIMAPSSL(data), data.constData());
}

void tst_QSslSocket::sslErrors_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");

    QString name = QtNetworkSettings::serverLocalName();
    QTest::newRow(qPrintable(name)) << name << 993;

    name = QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().first().toString();
    QTest::newRow(qPrintable(name)) << name << 443;
}

void tst_QSslSocket::sslErrors()
{
    QFETCH(QString, host);
    QFETCH(int, port);

    QSslSocketPtr socket = newSocket();
    QSignalSpy sslErrorsSpy(socket.data(), SIGNAL(sslErrors(QList<QSslError>)));
    QSignalSpy peerVerifyErrorSpy(socket.data(), SIGNAL(peerVerifyError(QSslError)));

    socket->connectToHostEncrypted(host, port);
    if (!socket->waitForConnected())
        QSKIP("Skipping flaky test - See QTBUG-29941");
    socket->waitForEncrypted(10000);

    // check the SSL errors contain HostNameMismatch and an error due to
    // the certificate being self-signed
    SslErrorList sslErrors;
    foreach (const QSslError &err, socket->sslErrors())
        sslErrors << err.error();
    qSort(sslErrors);
    QVERIFY(sslErrors.contains(QSslError::HostNameMismatch));
    QVERIFY(sslErrors.contains(FLUKE_CERTIFICATE_ERROR));

    // check the same errors were emitted by sslErrors
    QVERIFY(!sslErrorsSpy.isEmpty());
    SslErrorList emittedErrors;
    foreach (const QSslError &err, qvariant_cast<QList<QSslError> >(sslErrorsSpy.first().first()))
        emittedErrors << err.error();
    qSort(emittedErrors);
    QCOMPARE(sslErrors, emittedErrors);

    // check the same errors were emitted by peerVerifyError
    QVERIFY(!peerVerifyErrorSpy.isEmpty());
    SslErrorList peerErrors;
    const QList<QVariantList> &peerVerifyList = peerVerifyErrorSpy;
    foreach (const QVariantList &args, peerVerifyList)
        peerErrors << qvariant_cast<QSslError>(args.first()).error();
    qSort(peerErrors);
    QCOMPARE(sslErrors, peerErrors);
}

void tst_QSslSocket::addCaCertificate()
{
    if (!QSslSocket::supportsSsl())
        return;
}

void tst_QSslSocket::addCaCertificates()
{
    if (!QSslSocket::supportsSsl())
        return;
}

void tst_QSslSocket::addCaCertificates2()
{
    if (!QSslSocket::supportsSsl())
        return;
}

void tst_QSslSocket::ciphers()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocket socket;
    QCOMPARE(socket.ciphers(), QSslSocket::defaultCiphers());
    socket.setCiphers(QList<QSslCipher>());
    QVERIFY(socket.ciphers().isEmpty());
    socket.setCiphers(socket.defaultCiphers());
    QCOMPARE(socket.ciphers(), QSslSocket::defaultCiphers());
    socket.setCiphers(socket.defaultCiphers());
    QCOMPARE(socket.ciphers(), QSslSocket::defaultCiphers());

    // Task 164356
    socket.setCiphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
}

void tst_QSslSocket::connectToHostEncrypted()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();
    QVERIFY(socket->addCaCertificates(QLatin1String(SRCDIR "certs/qt-test-server-cacert.pem")));
#ifdef QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    connect(socket.data(), SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(untrustedWorkaroundSlot(QList<QSslError>)));
#endif

    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);

    // This should pass unconditionally when using fluke's CA certificate.
    // or use untrusted certificate workaround
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    socket->disconnectFromHost();
    QVERIFY(socket->waitForDisconnected());

    QCOMPARE(socket->mode(), QSslSocket::SslClientMode);

    socket->connectToHost(QtNetworkSettings::serverName(), 13);

    QCOMPARE(socket->mode(), QSslSocket::UnencryptedMode);

    QVERIFY(socket->waitForDisconnected());
}

void tst_QSslSocket::connectToHostEncryptedWithVerificationPeerName()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();

    socket->addCaCertificates(QLatin1String(SRCDIR "certs/qt-test-server-cacert.pem"));
#ifdef QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    connect(socket.data(), SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(untrustedWorkaroundSlot(QList<QSslError>)));
#endif

    // connect to the server with its local name, but use the full name for verification.
    socket->connectToHostEncrypted(QtNetworkSettings::serverLocalName(), 443, QtNetworkSettings::serverName());

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
    socket->connectToHost(QtNetworkSettings::serverName(), 443 /* https */);
    QVERIFY2(socket->waitForConnected(10000), qPrintable(socket->errorString()));
    QVERIFY(socket->sessionCipher().isNull());
    socket->startClientEncryption();
    if (!socket->waitForEncrypted(5000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QVERIFY(!socket->sessionCipher().isNull());

    qDebug() << "Supported Ciphers:" << QSslSocket::supportedCiphers();
    qDebug() << "Default Ciphers:" << QSslSocket::defaultCiphers();
    qDebug() << "Session Cipher:" << socket->sessionCipher();

    QVERIFY(QSslSocket::supportedCiphers().contains(socket->sessionCipher()));
    socket->disconnectFromHost();
    QVERIFY(socket->waitForDisconnected());
}

void tst_QSslSocket::flush()
{
}

void tst_QSslSocket::isEncrypted()
{
}

void tst_QSslSocket::localCertificate()
{
    if (!QSslSocket::supportsSsl())
        return;

    // This test does not make 100% sense yet. We just set some local CA/cert/key and use it
    // to authenticate ourselves against the server. The server does not actually check this
    // values. This test should just run the codepath inside qsslsocket_openssl.cpp

    QSslSocketPtr socket = newSocket();
    QList<QSslCertificate> localCert = QSslCertificate::fromPath(SRCDIR "certs/qt-test-server-cacert.pem");
    socket->setCaCertificates(localCert);
    socket->setLocalCertificate(QLatin1String(SRCDIR "certs/fluke.cert"));
    socket->setPrivateKey(QLatin1String(SRCDIR "certs/fluke.key"));

    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
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

    QList<QSslCertificate> caCertificates = QSslCertificate::fromPath(QLatin1String(SRCDIR "certs/qt-test-server-cacert.pem"));
    QCOMPARE(caCertificates.count(), 1);
    socket->addCaCertificates(caCertificates);
#ifdef QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    connect(socket.data(), SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(untrustedWorkaroundSlot(QList<QSslError>)));
#endif

    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
    QCOMPARE(socket->mode(), QSslSocket::UnencryptedMode);
    QVERIFY(socket->peerCertificateChain().isEmpty());
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");

    QList<QSslCertificate> certChain = socket->peerCertificateChain();
    QVERIFY(certChain.count() > 0);
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
    socket->connectToHost(QtNetworkSettings::serverName(), 443);
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

#ifndef QT_NO_OPENSSL
void tst_QSslSocket::privateKeyOpaque()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFile file(SRCDIR "certs/fluke.key");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!key.isNull());

    EVP_PKEY *pkey = q_EVP_PKEY_new();
    q_EVP_PKEY_set1_RSA(pkey, reinterpret_cast<RSA *>(key.handle()));

    // This test does not make 100% sense yet. We just set some local CA/cert/key and use it
    // to authenticate ourselves against the server. The server does not actually check this
    // values. This test should just run the codepath inside qsslsocket_openssl.cpp

    QSslSocketPtr socket = newSocket();
    QList<QSslCertificate> localCert = QSslCertificate::fromPath(SRCDIR "certs/qt-test-server-cacert.pem");
    socket->setCaCertificates(localCert);
    socket->setLocalCertificate(QLatin1String(SRCDIR "certs/fluke.cert"));
    socket->setPrivateKey(QSslKey(reinterpret_cast<Qt::HANDLE>(pkey)));

    socket->setPeerVerifyMode(QSslSocket::QueryPeer);
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
}
#endif

void tst_QSslSocket::protocol()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();
    QList<QSslCertificate> certs = QSslCertificate::fromPath(SRCDIR "certs/qt-test-server-cacert.pem");

    socket->setCaCertificates(certs);
#ifdef QSSLSOCKET_CERTUNTRUSTED_WORKAROUND
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(untrustedWorkaroundSlot(QList<QSslError>)));
#endif

    QCOMPARE(socket->protocol(), QSsl::SecureProtocols);
    QFETCH_GLOBAL(bool, setProxy);
    {
        // qt-test-server allows SSLv3.
        socket->setProtocol(QSsl::SslV3);
        QCOMPARE(socket->protocol(), QSsl::SslV3);
        socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::SslV3);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::SslV3);
        socket->connectToHost(QtNetworkSettings::serverName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::SslV3);
        socket->abort();
    }
    {
        // qt-test-server allows TLSV1.
        socket->setProtocol(QSsl::TlsV1_0);
        QCOMPARE(socket->protocol(), QSsl::TlsV1_0);
        socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1_0);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::TlsV1_0);
        socket->connectToHost(QtNetworkSettings::serverName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1_0);
        socket->abort();
    }
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
    {
        // qt-test-server probably doesn't allow TLSV1.1
        socket->setProtocol(QSsl::TlsV1_1);
        QCOMPARE(socket->protocol(), QSsl::TlsV1_1);
        socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1_1);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::TlsV1_1);
        socket->connectToHost(QtNetworkSettings::serverName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1_1);
        socket->abort();
    }
    {
        // qt-test-server probably doesn't allows TLSV1.2
        socket->setProtocol(QSsl::TlsV1_2);
        QCOMPARE(socket->protocol(), QSsl::TlsV1_2);
        socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1_2);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::TlsV1_2);
        socket->connectToHost(QtNetworkSettings::serverName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1_2);
        socket->abort();
    }
#endif
#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    {
        // qt-test-server allows SSLV2.
        socket->setProtocol(QSsl::SslV2);
        QCOMPARE(socket->protocol(), QSsl::SslV2);
        socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::SslV2);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::SslV2);
        socket->connectToHost(QtNetworkSettings::serverName(), 443);
        if (setProxy && !socket->waitForConnected())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        socket->abort();
    }
#endif
    {
        // qt-test-server allows SSLV3, so it allows AnyProtocol.
        socket->setProtocol(QSsl::AnyProtocol);
        QCOMPARE(socket->protocol(), QSsl::AnyProtocol);
        socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::AnyProtocol);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::AnyProtocol);
        socket->connectToHost(QtNetworkSettings::serverName(), 443);
        QVERIFY2(socket->waitForConnected(), qPrintable(socket->errorString()));
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::AnyProtocol);
        socket->abort();
    }
    {
        // qt-test-server allows SSLV3, so it allows NoSslV2
        socket->setProtocol(QSsl::TlsV1SslV3);
        QCOMPARE(socket->protocol(), QSsl::TlsV1SslV3);
        socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1SslV3);
        socket->abort();
        QCOMPARE(socket->protocol(), QSsl::TlsV1SslV3);
        socket->connectToHost(QtNetworkSettings::serverName(), 443);
        if (setProxy && !socket->waitForConnected())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        socket->startClientEncryption();
        if (setProxy && !socket->waitForEncrypted())
            QSKIP("Skipping flaky test - See QTBUG-29941");
        QCOMPARE(socket->protocol(), QSsl::TlsV1SslV3);
        socket->abort();
    }
}

class SslServer : public QTcpServer
{
    Q_OBJECT
public:
    SslServer(const QString &keyFile = SRCDIR "certs/fluke.key",
              const QString &certFile = SRCDIR "certs/fluke.cert",
              const QString &interFile = QString())
        : socket(0),
          config(QSslConfiguration::defaultConfiguration()),
          ignoreSslErrors(true),
          peerVerifyMode(QSslSocket::AutoVerifyPeer),
          protocol(QSsl::TlsV1_0),
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
    QString ciphers;

protected:
    void incomingConnection(qintptr socketDescriptor)
    {
        socket = new QSslSocket(this);
        socket->setSslConfiguration(config);
        socket->setPeerVerifyMode(peerVerifyMode);
        socket->setProtocol(protocol);
        if (ignoreSslErrors)
            connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));

        QFile file(m_keyFile);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull());
        socket->setPrivateKey(key);

        // Add CA certificates to verify client certificate
        if (!addCaCertificates.isEmpty()) {
            QList<QSslCertificate> caCert = QSslCertificate::fromPath(addCaCertificates);
            QVERIFY(!caCert.isEmpty());
            QVERIFY(!caCert.first().isNull());
            socket->addCaCertificates(caCert);
        }

        // If we have a cert issued directly from the CA
        if (m_interFile.isEmpty()) {
            QList<QSslCertificate> localCert = QSslCertificate::fromPath(m_certFile);
            QVERIFY(!localCert.isEmpty());
            QVERIFY(!localCert.first().isNull());
            socket->setLocalCertificate(localCert.first());
        }
        else {
            QList<QSslCertificate> localCert = QSslCertificate::fromPath(m_certFile);
            QVERIFY(!localCert.isEmpty());
            QVERIFY(!localCert.first().isNull());

            QList<QSslCertificate> interCert = QSslCertificate::fromPath(m_interFile);
            QVERIFY(!interCert.isEmpty());
            QVERIFY(!interCert.first().isNull());

            socket->setLocalCertificateChain(localCert + interCert);
        }

        if (!ciphers.isEmpty()) {
            socket->setCiphers(ciphers);
        }

        QVERIFY(socket->setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState));
        QVERIFY(!socket->peerAddress().isNull());
        QVERIFY(socket->peerPort() != 0);
        QVERIFY(!socket->localAddress().isNull());
        QVERIFY(socket->localPort() != 0);

        socket->startServerEncryption();
    }

protected slots:
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

#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    QTest::newRow("ssl2-ssl2") << QSsl::SslV2 << QSsl::SslV2 << false; // no idea why it does not work, but we don't care about SSL 2
#endif
    QTest::newRow("ssl3-ssl3") << QSsl::SslV3 << QSsl::SslV3 << true;
    QTest::newRow("tls1.0-tls1.0") << QSsl::TlsV1_0 << QSsl::TlsV1_0 << true;
    QTest::newRow("tls1ssl3-tls1ssl3") << QSsl::TlsV1SslV3 << QSsl::TlsV1SslV3 << true;
    QTest::newRow("any-any") << QSsl::AnyProtocol << QSsl::AnyProtocol << true;
    QTest::newRow("secure-secure") << QSsl::SecureProtocols << QSsl::SecureProtocols << true;

#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    QTest::newRow("ssl2-ssl3") << QSsl::SslV2 << QSsl::SslV3 << false;
    QTest::newRow("ssl2-tls1.0") << QSsl::SslV2 << QSsl::TlsV1_0 << false;
    QTest::newRow("ssl2-tls1ssl3") << QSsl::SslV2 << QSsl::TlsV1SslV3 << false;
    QTest::newRow("ssl2-secure") << QSsl::SslV2 << QSsl::SecureProtocols << false;
    QTest::newRow("ssl2-any") << QSsl::SslV2 << QSsl::AnyProtocol << false; // no idea why it does not work, but we don't care about SSL 2
#endif

#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    QTest::newRow("ssl3-ssl2") << QSsl::SslV3 << QSsl::SslV2 << false;
#endif
    QTest::newRow("ssl3-tls1.0") << QSsl::SslV3 << QSsl::TlsV1_0 << false;
    QTest::newRow("ssl3-tls1ssl3") << QSsl::SslV3 << QSsl::TlsV1SslV3 << true;
    QTest::newRow("ssl3-secure") << QSsl::SslV3 << QSsl::SecureProtocols << false;
#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    QTest::newRow("ssl3-any") << QSsl::SslV3 << QSsl::AnyProtocol << false; // we won't set a SNI header here because we connect to a
                                                                            // numerical IP, so OpenSSL will send a SSL 2 handshake
#else
    QTest::newRow("ssl3-any") << QSsl::SslV3 << QSsl::AnyProtocol << true;
#endif

#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    QTest::newRow("tls1.0-ssl2") << QSsl::TlsV1_0 << QSsl::SslV2 << false;
#endif
    QTest::newRow("tls1.0-ssl3") << QSsl::TlsV1_0 << QSsl::SslV3 << false;
    QTest::newRow("tls1-tls1ssl3") << QSsl::TlsV1_0 << QSsl::TlsV1SslV3 << true;
    QTest::newRow("tls1.0-secure") << QSsl::TlsV1_0 << QSsl::SecureProtocols << true;
#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    QTest::newRow("tls1.0-any") << QSsl::TlsV1_0 << QSsl::AnyProtocol << false; // we won't set a SNI header here because we connect to a
                                                                            // numerical IP, so OpenSSL will send a SSL 2 handshake
#else
    QTest::newRow("tls1.0-any") << QSsl::TlsV1_0 << QSsl::AnyProtocol << true;
#endif

#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    QTest::newRow("tls1ssl3-ssl2") << QSsl::TlsV1SslV3 << QSsl::SslV2 << false;
#endif
    QTest::newRow("tls1ssl3-ssl3") << QSsl::TlsV1SslV3 << QSsl::SslV3 << true;
    QTest::newRow("tls1ssl3-tls1.0") << QSsl::TlsV1SslV3 << QSsl::TlsV1_0 << true;
    QTest::newRow("tls1ssl3-secure") << QSsl::TlsV1SslV3 << QSsl::SecureProtocols << true;
    QTest::newRow("tls1ssl3-any") << QSsl::TlsV1SslV3 << QSsl::AnyProtocol << true;

#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    QTest::newRow("secure-ssl2") << QSsl::SecureProtocols << QSsl::SslV2 << false;
#endif
    QTest::newRow("secure-ssl3") << QSsl::SecureProtocols << QSsl::SslV3 << false;
    QTest::newRow("secure-tls1.0") << QSsl::SecureProtocols << QSsl::TlsV1_0 << true;
    QTest::newRow("secure-tls1ssl3") << QSsl::SecureProtocols << QSsl::TlsV1SslV3 << true;
    QTest::newRow("secure-any") << QSsl::SecureProtocols << QSsl::AnyProtocol << true;

#if !defined(OPENSSL_NO_SSL2) && !defined(QT_SECURETRANSPORT)
    QTest::newRow("any-ssl2") << QSsl::AnyProtocol << QSsl::SslV2 << false; // no idea why it does not work, but we don't care about SSL 2
#endif
    QTest::newRow("any-ssl3") << QSsl::AnyProtocol << QSsl::SslV3 << true;
    QTest::newRow("any-tls1.0") << QSsl::AnyProtocol << QSsl::TlsV1_0 << true;
    QTest::newRow("any-tls1ssl3") << QSsl::AnyProtocol << QSsl::TlsV1SslV3 << true;
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
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSslSocketPtr client(new QSslSocket);
    socket = client.data();
    QFETCH(QSsl::SslProtocol, clientProtocol);
    socket->setProtocol(clientProtocol);
    // upon SSL wrong version error, error will be triggered, not sslErrors
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();

    QFETCH(bool, works);
    QAbstractSocket::SocketState expectedState = (works) ? QAbstractSocket::ConnectedState : QAbstractSocket::UnconnectedState;
    QCOMPARE(int(client->state()), int(expectedState));
    QCOMPARE(client->isEncrypted(), works);
}

#ifndef QT_NO_OPENSSL

void tst_QSslSocket::serverCipherPreferences()
{
    if (!QSslSocket::supportsSsl()) {
        qWarning("SSL not supported, skipping test");
        return;
    }

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    // First using the default (server preference)
    {
        SslServer server;
        server.ciphers = QString("AES128-SHA:AES256-SHA");
        QVERIFY(server.listen());

        QEventLoop loop;
        QTimer::singleShot(5000, &loop, SLOT(quit()));

        QSslSocketPtr client(new QSslSocket);
        socket = client.data();
        socket->setCiphers("AES256-SHA:AES128-SHA");

        // upon SSL wrong version error, error will be triggered, not sslErrors
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
        connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
        connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

        client->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

        loop.exec();

        QVERIFY(client->isEncrypted());
        QCOMPARE(client->sessionCipher().name(), QString("AES128-SHA"));
    }

    {
        // Now using the client preferences
        SslServer server;
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        config.setSslOption(QSsl::SslOptionDisableServerCipherPreference, true);
        server.config = config;
        server.ciphers = QString("AES128-SHA:AES256-SHA");
        QVERIFY(server.listen());

        QEventLoop loop;
        QTimer::singleShot(5000, &loop, SLOT(quit()));

        QSslSocketPtr client(new QSslSocket);
        socket = client.data();
        socket->setCiphers("AES256-SHA:AES128-SHA");

        // upon SSL wrong version error, error will be triggered, not sslErrors
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
        connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
        connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

        client->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

        loop.exec();

        QVERIFY(client->isEncrypted());
        QCOMPARE(client->sessionCipher().name(), QString("AES256-SHA"));
    }
}

#endif // QT_NO_OPENSSL


void tst_QSslSocket::setCaCertificates()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocket socket;
    QCOMPARE(socket.caCertificates(), QSslSocket::defaultCaCertificates());
    socket.setCaCertificates(QSslCertificate::fromPath(SRCDIR "certs/qt-test-server-cacert.pem"));
    QCOMPARE(socket.caCertificates().size(), 1);
    socket.setCaCertificates(socket.defaultCaCertificates());
    QCOMPARE(socket.caCertificates(), QSslSocket::defaultCaCertificates());
}

void tst_QSslSocket::setLocalCertificate()
{
}

void tst_QSslSocket::localCertificateChain()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocket socket;
    socket.setLocalCertificate(QLatin1String(SRCDIR "certs/fluke.cert"));

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

    SslServer server(QLatin1String(SRCDIR "certs/leaf.key"),
                     QLatin1String(SRCDIR "certs/leaf.crt"),
                     QLatin1String(SRCDIR "certs/inter.crt"));

    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    socket = new QSslSocket();
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));

    socket->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());
    loop.exec();

    QList<QSslCertificate> chain = socket->peerCertificateChain();
    QCOMPARE(chain.size(), 2);
    QCOMPARE(chain[0].serialNumber(), QByteArray("10:a0:ad:77:58:f6:6e:ae:46:93:a3:43:f9:59:8a:9e"));
    QCOMPARE(chain[1].serialNumber(), QByteArray("3b:eb:99:c5:ea:d8:0b:5d:0b:97:5d:4f:06:75:4b:e1"));

    socket->deleteLater();
}

void tst_QSslSocket::setPrivateKey()
{
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

    QSslSocketPtr client(new QSslSocket);
    socket = client.data();;
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();

    QCOMPARE(client->state(), QAbstractSocket::ConnectedState);
    QVERIFY(client->isEncrypted());
    QVERIFY(!client->peerAddress().isNull());
    QVERIFY(client->peerPort() != 0);
    QVERIFY(!client->localAddress().isNull());
    QVERIFY(client->localPort() != 0);
}

void tst_QSslSocket::setSslConfiguration_data()
{
    QTest::addColumn<QSslConfiguration>("configuration");
    QTest::addColumn<bool>("works");

    QTest::newRow("empty") << QSslConfiguration() << false;
    QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
    QTest::newRow("default") << conf << false; // does not contain test server cert
    QList<QSslCertificate> testServerCert = QSslCertificate::fromPath(SRCDIR "certs/qt-test-server-cacert.pem");
    conf.setCaCertificates(testServerCert);
    QTest::newRow("set-root-cert") << conf << true;
    conf.setProtocol(QSsl::SecureProtocols);
    QTest::newRow("secure") << conf << true;
}

void tst_QSslSocket::setSslConfiguration()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    QFETCH(QSslConfiguration, configuration);
    socket->setSslConfiguration(configuration);
    this->socket = socket.data();
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
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
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);

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
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(-1))
        QSKIP("Skipping flaky test - See QTBUG-29941");
}

void tst_QSslSocket::waitForConnectedEncryptedReadyRead()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslSocketPtr socket = newSocket();
    this->socket = socket.data();

    connect(this->socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 993);

    QVERIFY2(socket->waitForConnected(10000), qPrintable(socket->errorString()));
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && !socket->waitForEncrypted(10000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
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
    QSslSocket::setDefaultCaCertificates(QSslSocket::systemCaCertificates());

    QList<QSslCertificate> flukeCerts = QSslCertificate::fromPath(SRCDIR "certs/qt-test-server-cacert.pem");
    QCOMPARE(flukeCerts.size(), 1);
    QList<QSslCertificate> globalCerts = QSslSocket::defaultCaCertificates();
    QVERIFY(!globalCerts.contains(flukeCerts.first()));
    QSslSocket::addDefaultCaCertificate(flukeCerts.first());
    QCOMPARE(QSslSocket::defaultCaCertificates().size(), globalCerts.size() + 1);
    QVERIFY(QSslSocket::defaultCaCertificates().contains(flukeCerts.first()));

    // Restore the global CA chain
    QSslSocket::setDefaultCaCertificates(QSslSocket::systemCaCertificates());
}

void tst_QSslSocket::addDefaultCaCertificates()
{
}

void tst_QSslSocket::addDefaultCaCertificates2()
{
}

void tst_QSslSocket::defaultCaCertificates()
{
    if (!QSslSocket::supportsSsl())
        return;

    QList<QSslCertificate> certs = QSslSocket::defaultCaCertificates();
    QVERIFY(certs.size() > 1);
    QCOMPARE(certs, QSslSocket::systemCaCertificates());
}

void tst_QSslSocket::defaultCiphers()
{
    if (!QSslSocket::supportsSsl())
        return;

    QList<QSslCipher> ciphers = QSslSocket::defaultCiphers();
    QVERIFY(ciphers.size() > 1);

    QSslSocket socket;
    QCOMPARE(socket.defaultCiphers(), ciphers);
    QCOMPARE(socket.ciphers(), ciphers);
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

    QList<QSslCipher> ciphers = QSslSocket::supportedCiphers();
    QVERIFY(ciphers.size() > 1);

    QSslSocket socket;
    QCOMPARE(socket.supportedCiphers(), ciphers);
}

void tst_QSslSocket::systemCaCertificates()
{
    if (!QSslSocket::supportsSsl())
        return;

    QList<QSslCertificate> certs = QSslSocket::systemCaCertificates();
    QVERIFY(certs.size() > 1);
    QCOMPARE(certs, QSslSocket::defaultCaCertificates());
}

void tst_QSslSocket::wildcardCertificateNames()
{
    // Passing CN matches
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("www.example.com"), QString("www.example.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("*.example.com"), QString("www.example.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("xxx*.example.com"), QString("xxxwww.example.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("f*.example.com"), QString("foo.example.com")), true );
    QCOMPARE( QSslSocketPrivate::isMatchingHostname(QString("192.168.0.0"), QString("192.168.0.0")), true );

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
    socket->addCaCertificates(QLatin1String("certs/aspiriniks.ca.crt"));
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
    void incomingConnection(qintptr socketDescriptor)
    {
        QSslSocket *socket = new QSslSocket(this);
        socket->ignoreSslErrors();

        // Only set the certificate
        QList<QSslCertificate> localCert = QSslCertificate::fromPath(SRCDIR "certs/fluke.cert");
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
    connect(receiver, SIGNAL(encrypted()), SLOT(exitLoop()));
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

    // SSL handshake:
    connect(receiver, SIGNAL(encrypted()), SLOT(exitLoop()));
    enterLoop(1);
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
        QTime t;
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
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
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
    socket->write(QtNetworkSettings::serverName().toLocal8Bit().constData());
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
    void incomingConnection(qintptr socketDescriptor)
    {
        socket = new QSslSocket(this);
        connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));

        QFile file(SRCDIR "certs/fluke.key");
        QVERIFY(file.open(QIODevice::ReadOnly));
        QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull());
        socket->setPrivateKey(key);

        QList<QSslCertificate> localCert = QSslCertificate::fromPath(SRCDIR "certs/fluke.cert");
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
    void run()
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
        if (!socket->waitForReadyRead(2000))
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
    QVERIFY(server.dataReadSemaphore.tryAcquire(1, 2000));

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
}

class VerifyServer : public QTcpServer
{
    Q_OBJECT
public:
    VerifyServer() : socket(0) { }
    QSslSocket *socket;

protected:
    void incomingConnection(qintptr socketDescriptor)
    {
        socket = new QSslSocket(this);

        socket->setPrivateKey(SRCDIR "certs/fluke.key");
        socket->setLocalCertificate(SRCDIR "certs/fluke.cert");
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

    socket.connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
    if (socket.waitForEncrypted())
        QSKIP("Skipping flaky test - See QTBUG-29941");

    QList<QSslError> expectedErrors = QList<QSslError>()
                                      << QSslError(FLUKE_CERTIFICATE_ERROR, socket.peerCertificate());
    QCOMPARE(socket.sslErrors(), expectedErrors);
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
    QVERIFY(server.socket->sslErrors().isEmpty());
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

void tst_QSslSocket::disconnectFromHostWhenConnecting()
{
    QSslSocketPtr socket = newSocket();
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 993);
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
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 993);
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

void tst_QSslSocket::resetProxy()
{
#ifndef QT_NO_NETWORKPROXY
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    // check fix for bug 199941

    QNetworkProxy goodProxy(QNetworkProxy::NoProxy);
    QNetworkProxy badProxy(QNetworkProxy::HttpProxy, "thisCannotWorkAbsolutelyNotForSure", 333);

    // make sure the connection works, and then set a nonsense proxy, and then
    // make sure it does not work anymore
    QSslSocket socket;
    socket.addCaCertificates(QLatin1String(SRCDIR "certs/qt-test-server-cacert.pem"));
    socket.setProxy(goodProxy);
    socket.connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
    QVERIFY2(socket.waitForConnected(10000), qPrintable(socket.errorString()));
    socket.abort();
    socket.setProxy(badProxy);
    socket.connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
    QVERIFY(! socket.waitForConnected(10000));

    // don't forget to login
    QCOMPARE((int) socket.write("USER ftptest\r\n"), 14);
    QCOMPARE((int) socket.write("PASS password\r\n"), 15);

    enterLoop(10);

    // now the other way round:
    // set the nonsense proxy and make sure the connection does not work,
    // and then set the right proxy and make sure it works
    QSslSocket socket2;
    socket2.addCaCertificates(QLatin1String(SRCDIR "certs/qt-test-server-cacert.pem"));
    socket2.setProxy(badProxy);
    socket2.connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
    QVERIFY(! socket2.waitForConnected(10000));
    socket2.abort();
    socket2.setProxy(goodProxy);
    socket2.connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
    QVERIFY2(socket2.waitForConnected(10000), qPrintable(socket.errorString()));
#endif // QT_NO_NETWORKPROXY
}

void tst_QSslSocket::ignoreSslErrorsList_data()
{
    QTest::addColumn<QList<QSslError> >("expectedSslErrors");
    QTest::addColumn<int>("expectedSslErrorSignalCount");

    // construct the list of errors that we will get with the SSL handshake and that we will ignore
    QList<QSslError> expectedSslErrors;
    // fromPath gives us a list of certs, but it actually only contains one
    QList<QSslCertificate> certs = QSslCertificate::fromPath(QLatin1String(SRCDIR "certs/qt-test-server-cacert.pem"));
    QSslError rightError(FLUKE_CERTIFICATE_ERROR, certs.at(0));
    QSslError wrongError(FLUKE_CERTIFICATE_ERROR);


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

//    this->socket = &socket;
    QSslCertificate cert;

    QFETCH(QList<QSslError>, expectedSslErrors);
    socket.ignoreSslErrors(expectedSslErrors);

    QFETCH(int, expectedSslErrorSignalCount);
    QSignalSpy sslErrorsSpy(&socket, SIGNAL(error(QAbstractSocket::SocketError)));

    socket.connectToHostEncrypted(QtNetworkSettings::serverName(), 443);

    bool expectEncryptionSuccess = (expectedSslErrorSignalCount == 0);
    if (socket.waitForEncrypted(10000) != expectEncryptionSuccess)
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QCOMPARE(sslErrorsSpy.count(), expectedSslErrorSignalCount);
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
    socket.connectToHostEncrypted(QtNetworkSettings::serverName(), 443);

    QFETCH(int, expectedSslErrorSignalCount);
    bool expectEncryptionSuccess = (expectedSslErrorSignalCount == 0);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && (socket.waitForEncrypted(10000) != expectEncryptionSuccess))
        QSKIP("Skipping flaky test - See QTBUG-29941");
}

// make sure a closed socket has no bytesAvailable()
// related to https://bugs.webkit.org/show_bug.cgi?id=28016
void tst_QSslSocket::readFromClosedSocket()
{
    QSslSocketPtr socket = newSocket();
    socket->ignoreSslErrors();
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
    socket->ignoreSslErrors();
    socket->waitForConnected();
    socket->waitForEncrypted();
    // provoke a response by sending a request
    socket->write("GET /qtest/fluke.gif HTTP/1.1\n");
    socket->write("Host: ");
    socket->write(QtNetworkSettings::serverName().toLocal8Bit().constData());
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
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);

    QByteArray data;
    data.resize(1024*1024*10); // 10 MB
    // init with garbage. needed so ssl cannot compress it in an efficient way.
    for (size_t i = 0; i < data.size() / sizeof(int); i++) {
        int r = qrand();
        data.data()[i*sizeof(int)] = r;
    }

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

    SslServer server(SRCDIR "certs/fake-login.live.com.key", SRCDIR "certs/fake-login.live.com.pem");
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
    QList<QSslError> sslErrors = receiver->sslErrors();
    QVERIFY(sslErrors.count() > 0);
    // there are more errors (self signed cert and hostname mismatch), but we only care about the blacklist error
    QCOMPARE(sslErrors.at(0).error(), QSslError::CertificateBlacklisted);
}

void tst_QSslSocket::versionAccessors()
{
    if (!QSslSocket::supportsSsl())
        return;

    qDebug() << QSslSocket::sslLibraryVersionString();
    qDebug() << QString::number(QSslSocket::sslLibraryVersionNumber(), 16);
}

#ifndef QT_NO_OPENSSL
void tst_QSslSocket::sslOptions()
{
    if (!QSslSocket::supportsSsl())
        return;

#ifdef SSL_OP_NO_COMPRESSION
    QCOMPARE(QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SecureProtocols,
                                                           QSslConfigurationPrivate::defaultSslOptions),
             long(SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_COMPRESSION|SSL_OP_CIPHER_SERVER_PREFERENCE));
#else
    QCOMPARE(QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SecureProtocols,
                                                           QSslConfigurationPrivate::defaultSslOptions),
             long(SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_CIPHER_SERVER_PREFERENCE));
#endif

    QCOMPARE(QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SecureProtocols,
                                                           QSsl::SslOptionDisableEmptyFragments
                                                           |QSsl::SslOptionDisableLegacyRenegotiation),
             long(SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_CIPHER_SERVER_PREFERENCE));

#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
    QCOMPARE(QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SecureProtocols,
                                                           QSsl::SslOptionDisableEmptyFragments),
             long((SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION|SSL_OP_CIPHER_SERVER_PREFERENCE)));
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
    QCOMPARE(QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SecureProtocols,
                                                           QSsl::SslOptionDisableLegacyRenegotiation),
             long((SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_CIPHER_SERVER_PREFERENCE) & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS));
#endif

#ifdef SSL_OP_NO_TICKET
    QCOMPARE(QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SecureProtocols,
                                                           QSsl::SslOptionDisableEmptyFragments
                                                           |QSsl::SslOptionDisableLegacyRenegotiation
                                                           |QSsl::SslOptionDisableSessionTickets),
             long((SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TICKET|SSL_OP_CIPHER_SERVER_PREFERENCE)));
#endif

#ifdef SSL_OP_NO_TICKET
#ifdef SSL_OP_NO_COMPRESSION
    QCOMPARE(QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SecureProtocols,
                                                           QSsl::SslOptionDisableEmptyFragments
                                                           |QSsl::SslOptionDisableLegacyRenegotiation
                                                           |QSsl::SslOptionDisableSessionTickets
                                                           |QSsl::SslOptionDisableCompression),
             long((SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TICKET|SSL_OP_NO_COMPRESSION|SSL_OP_CIPHER_SERVER_PREFERENCE)));
#endif
#endif
}
#endif

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
    QTest::addColumn<bool>("ignoreErrorsAfterPause");
    QTest::addColumn<QList<QSslError> >("errorsToIgnore");
    QTest::addColumn<bool>("expectSuccess");

    QList<QSslError> errorsList;
    QTest::newRow("DoNotIgnoreErrors") << false << QList<QSslError>() << false;
    QTest::newRow("ignoreAllErrors") << true << QList<QSslError>() << true;

    QList<QSslCertificate> certs = QSslCertificate::fromPath(QLatin1String(SRCDIR "certs/qt-test-server-cacert.pem"));
    QSslError rightError(FLUKE_CERTIFICATE_ERROR, certs.at(0));
    QSslError wrongError(FLUKE_CERTIFICATE_ERROR);
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
    QSslSocket::setDefaultCaCertificates(QSslSocket::systemCaCertificates());

    QFETCH(bool, ignoreErrorsAfterPause);
    QFETCH(QList<QSslError>, errorsToIgnore);
    QFETCH(bool, expectSuccess);

    QSslSocket socket;
    socket.setPauseMode(QAbstractSocket::PauseOnSslErrors);

    QSignalSpy sslErrorSpy(&socket, SIGNAL(sslErrors(QList<QSslError>)));
    QSignalSpy encryptedSpy(&socket, SIGNAL(encrypted()));
    QSignalSpy errorSpy(&socket, SIGNAL(error(QAbstractSocket::SocketError)));

    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    connect(&socket, SIGNAL(encrypted()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    connect(&socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            this, SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    socket.connectToHostEncrypted(QtNetworkSettings::serverName(), 993);
    QTestEventLoop::instance().enterLoop(10);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && QTestEventLoop::instance().timeout())
        QSKIP("Skipping flaky test - See QTBUG-29941");
    QCOMPARE(sslErrorSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(encryptedSpy.count(), 0);
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
        QCOMPARE(encryptedSpy.count(), 1);
        QVERIFY(socket.isEncrypted());
        QCOMPARE(errorSpy.count(), 0);
        socket.disconnectFromHost();
        QVERIFY(socket.waitForDisconnected(10000));
    } else {
        QCOMPARE(encryptedSpy.count(), 0);
        QVERIFY(!socket.isEncrypted());
        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(socket.error(), QAbstractSocket::SslHandshakeFailedError);
    }
}

class WebSocket : public QSslSocket
{
    Q_OBJECT
public:
    explicit WebSocket(qintptr socketDescriptor,
                       const QString &keyFile = SRCDIR "certs/fluke.key",
                       const QString &certFile = SRCDIR "certs/fluke.cert");

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
    SslServer4() : socket(0) {}
    WebSocket *socket;

protected:
    void incomingConnection(qintptr socketDescriptor)
    {
        socket =  new WebSocket(socketDescriptor);
    }
};

void tst_QSslSocket::qtbug18498_peek()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer4 server;
    QSslSocket *client = new QSslSocket(this);

    QVERIFY(server.listen(QHostAddress::LocalHost));
    client->connectToHost("127.0.0.1", server.serverPort());
    QVERIFY(client->waitForConnected(5000));
    QVERIFY(server.waitForNewConnection(1000));
    client->setObjectName("client");
    client->ignoreSslErrors();

    connect(client, SIGNAL(encrypted()), this, SLOT(exitLoop()));
    connect(client, SIGNAL(disconnected()), this, SLOT(exitLoop()));

    client->startClientEncryption();
    WebSocket *serversocket = server.socket;
    QVERIFY(serversocket);
    serversocket->setObjectName("server");

    enterLoop(1);
    QVERIFY(!timeout());
    QVERIFY(serversocket->isEncrypted());
    QVERIFY(client->isEncrypted());

    QByteArray data("abc123");
    client->write(data.data());

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
    void incomingConnection(qintptr socketDescriptor)
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
    QElapsedTimer stopwatch;
    stopwatch.start();
    while (client->bytesAvailable() < 7 && stopwatch.elapsed() < 5000)
        QTest::qWait(100);
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
    while (server->bytesAvailable() < bigblock.length() + 4 && stopwatch.elapsed() < 5000)
        QTest::qWait(100);
    QCOMPARE(server->read(4), QByteArray("head"));
    QCOMPARE(server->peek(bigblock.length()), bigblock);
    b.reserve(bigblock.length());
    b.resize(server->peek(b.data(), bigblock.length()));
    QCOMPARE(b, bigblock);

    //check oversized peek
    QCOMPARE(server->peek(bigblock.length() * 3), bigblock);
    b.reserve(bigblock.length() * 3);
    b.resize(server->peek(b.data(), bigblock.length() * 3));
    QCOMPARE(b, bigblock);

    QCOMPARE(server->readAll(), bigblock);

    QVERIFY(client->write("STARTTLS\r\n"));
    stopwatch.start();
    // ### Qt5 use QTRY_VERIFY
    while (server->bytesAvailable() < 10 && stopwatch.elapsed() < 5000)
        QTest::qWait(100);
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

    QFile file(SRCDIR "certs/fluke.key");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!key.isNull());
    server->setPrivateKey(key);

    QList<QSslCertificate> localCert = QSslCertificate::fromPath(SRCDIR "certs/fluke.cert");
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
    stopwatch.start();
    while (client->bytesAvailable() < 7 && stopwatch.elapsed() < 5000)
        QTest::qWait(100);
    QVERIFY(server->mode() == QSslSocket::SslServerMode && client->mode() == QSslSocket::SslClientMode);
    QCOMPARE(client->peek(&c,1), 1);
    QCOMPARE(c, 'h');
    QCOMPARE(client->read(&c,1), 1);
    QCOMPARE(c, 'h');
    b = client->peek(2);
    QCOMPARE(b, QByteArray("el"));
    QCOMPARE(client->readAll(), QByteArray("ello\r\n"));

    QVERIFY(client->write("goodbye\r\n"));
    stopwatch.start();
    while (server->bytesAvailable() < 9 && stopwatch.elapsed() < 5000)
        QTest::qWait(100);
    QCOMPARE(server->peek(&c,1), 1);
    QCOMPARE(c, 'g');
    QCOMPARE(server->readAll(), QByteArray("goodbye\r\n"));
    client->disconnectFromHost();
    QVERIFY(client->waitForDisconnected(5000));
}

void tst_QSslSocket::dhServer()
{
    if (!QSslSocket::supportsSsl()) {
        qWarning("SSL not supported, skipping test");
        return;
    }

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SslServer server;
    server.ciphers = QLatin1String("DHE-RSA-AES256-SHA:DHE-DSS-AES256-SHA");
    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSslSocketPtr client(new QSslSocket);
    socket = client.data();
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();
    QCOMPARE(client->state(), QAbstractSocket::ConnectedState);
}

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
    server.ciphers = QLatin1String("ECDHE-RSA-AES128-SHA");
    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSslSocketPtr client(new QSslSocket);
    socket = client.data();
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), &loop, SLOT(quit()));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();
    QCOMPARE(client->state(), QAbstractSocket::ConnectedState);
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
    QList<QSslCertificate> flukeCerts = QSslCertificate::fromPath(SRCDIR "certs/fluke.cert");
    QCOMPARE(flukeCerts.size(), 1);

    QFile flukeFile(SRCDIR "certs/fluke.key");
    QVERIFY(flukeFile.open(QIODevice::ReadOnly));
    QSslKey flukeKey(flukeFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!flukeKey.isNull());

    QTest::newRow("SelfSignedCert:AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << flukeCerts << flukeKey << true;
    QTest::newRow("SelfSignedCert:QueryPeer") << QSslSocket::QueryPeer << flukeCerts << flukeKey << true;
    QTest::newRow("SelfSignedCert:VerifyNone") << QSslSocket::VerifyNone << flukeCerts << flukeKey << true;
    QTest::newRow("SelfSignedCert:VerifyPeer") << QSslSocket::VerifyPeer << flukeCerts << flukeKey << false;

    // valid certificate, but wrong usage (server certificate)
    QList<QSslCertificate> serverCerts = QSslCertificate::fromPath(SRCDIR "certs/bogus-server.crt");
    QCOMPARE(serverCerts.size(), 1);

    QFile serverFile(SRCDIR "certs/bogus-server.key");
    QVERIFY(serverFile.open(QIODevice::ReadOnly));
    QSslKey serverKey(serverFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!serverKey.isNull());

    QTest::newRow("ValidServerCert:AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << serverCerts << serverKey << true;
    QTest::newRow("ValidServerCert:QueryPeer") << QSslSocket::QueryPeer << serverCerts << serverKey << true;
    QTest::newRow("ValidServerCert:VerifyNone") << QSslSocket::VerifyNone << serverCerts << serverKey << true;
    QTest::newRow("ValidServerCert:VerifyPeer") << QSslSocket::VerifyPeer << serverCerts << serverKey << false;

    // valid certificate, correct usage (client certificate)
    QList<QSslCertificate> validCerts = QSslCertificate::fromPath(SRCDIR "certs/bogus-client.crt");
    QCOMPARE(validCerts.size(), 1);

    QFile validFile(SRCDIR "certs/bogus-client.key");
    QVERIFY(validFile.open(QIODevice::ReadOnly));
    QSslKey validKey(validFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    QVERIFY(!validKey.isNull());

    QTest::newRow("ValidClientCert:AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << validCerts << validKey << true;
    QTest::newRow("ValidClientCert:QueryPeer") << QSslSocket::QueryPeer << validCerts << validKey << true;
    QTest::newRow("ValidClientCert:VerifyNone") << QSslSocket::VerifyNone << validCerts << validKey << true;
    QTest::newRow("ValidClientCert:VerifyPeer") << QSslSocket::VerifyPeer << validCerts << validKey << true;

    // valid certificate, correct usage (client certificate), with chain
    validCerts += QSslCertificate::fromPath(SRCDIR "certs/bogus-ca.crt");
    QCOMPARE(validCerts.size(), 2);

    QTest::newRow("ValidClientCert:AutoVerifyPeer") << QSslSocket::AutoVerifyPeer << validCerts << validKey << true;
    QTest::newRow("ValidClientCert:QueryPeer") << QSslSocket::QueryPeer << validCerts << validKey << true;
    QTest::newRow("ValidClientCert:VerifyNone") << QSslSocket::VerifyNone << validCerts << validKey << true;
    QTest::newRow("ValidClientCert:VerifyPeer") << QSslSocket::VerifyPeer << validCerts << validKey << true;
}

void tst_QSslSocket::verifyClientCertificate()
{
#ifdef QT_SECURETRANSPORT
    // We run both client and server on the same machine,
    // this means, client can update keychain with client's certificates,
    // and server later will use the same certificates from the same
    // keychain thus making tests fail (wrong number of certificates,
    // success instead of failure etc.).
    QSKIP("This test can not work with Secure Transport");
#endif
    if (!QSslSocket::supportsSsl()) {
        qWarning("SSL not supported, skipping test");
        return;
    }

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QFETCH(QSslSocket::PeerVerifyMode, peerVerifyMode);
    SslServer server;
    server.addCaCertificates = QLatin1String(SRCDIR "certs/bogus-ca.crt");
    server.ignoreSslErrors = false;
    server.peerVerifyMode = peerVerifyMode;
    QVERIFY(server.listen());

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QFETCH(QList<QSslCertificate>, clientCerts);
    QFETCH(QSslKey, clientKey);
    QSslSocketPtr client(new QSslSocket);
    client->setLocalCertificateChain(clientCerts);
    client->setPrivateKey(clientKey);
    socket = client.data();

    connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(ignoreErrorSlot()));
    connect(socket, SIGNAL(disconnected()), &loop, SLOT(quit()));
    connect(socket, SIGNAL(encrypted()), &loop, SLOT(quit()));

    client->connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());

    loop.exec();

    QFETCH(bool, works);
    QAbstractSocket::SocketState expectedState = (works) ? QAbstractSocket::ConnectedState : QAbstractSocket::UnconnectedState;

    // check server socket
    QVERIFY(server.socket);

    QCOMPARE(int(server.socket->state()), int(expectedState));
    QCOMPARE(server.socket->isEncrypted(), works);

    if (peerVerifyMode == QSslSocket::VerifyNone || clientCerts.isEmpty()) {
        QVERIFY(server.socket->peerCertificate().isNull());
        QVERIFY(server.socket->peerCertificateChain().isEmpty());
    } else {
        QCOMPARE(server.socket->peerCertificate(), clientCerts.first());
        QCOMPARE(server.socket->peerCertificateChain(), clientCerts);
    }

    // check client socket
    QCOMPARE(int(client->state()), int(expectedState));
    QCOMPARE(client->isEncrypted(), works);
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
    socket->connectToHostEncrypted(QtNetworkSettings::serverName(), 443);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy && socket->waitForEncrypted(4000))
        QSKIP("Skipping flaky test - See QTBUG-29941");
}

#ifndef QT_NO_OPENSSL
class PskProvider : public QObject
{
    Q_OBJECT

public:
    explicit PskProvider(QObject *parent = 0)
        : QObject(parent)
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

private:
    QByteArray m_identity;
    QByteArray m_psk;
};

void tst_QSslSocket::simplePskConnect_data()
{
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
    const QList<QSslCipher> supportedCiphers = QSslSocket::supportedCiphers();
    foreach (const QSslCipher &cipher, supportedCiphers) {
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

    QSignalSpy socketErrorsSpy(&socket, SIGNAL(error(QAbstractSocket::SocketError)));
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
    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(peerVerifyError(QSslError)), this, SLOT(exitLoop()));
    connect(&socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(exitLoop()));

    // force a PSK cipher w/o auth
    socket.setCiphers(PSK_CIPHER_WITHOUT_AUTH);

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
        provider.setIdentity(PSK_CLIENT_IDENTITY.left(PSK_CLIENT_IDENTITY.length() - 1));
        provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY.left(PSK_CLIENT_PRESHAREDKEY.length() - 1));
        connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
        break;

    case PskConnectWrongIdentity:
        // right PSK, wrong identity
        provider.setIdentity(PSK_CLIENT_IDENTITY.left(PSK_CLIENT_IDENTITY.length() - 1));
        provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY);
        connect(&socket, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), &provider, SLOT(providePsk(QSslPreSharedKeyAuthenticator*)));
        break;

    case PskConnectWrongPreSharedKey:
        // right identity, wrong PSK
        provider.setIdentity(PSK_CLIENT_IDENTITY);
        provider.setPreSharedKey(PSK_CLIENT_PRESHAREDKEY.left(PSK_CLIENT_PRESHAREDKEY.length() - 1));
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
    QCOMPARE(connectedSpy.count(), 0);
    QCOMPARE(hostFoundSpy.count(), 1);
    QCOMPARE(disconnectedSpy.count(), 0);
    enterLoop(10);

    // Entered connected state
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socket.mode(), QSslSocket::UnencryptedMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectedSpy.count(), 1);
    QCOMPARE(hostFoundSpy.count(), 1);
    QCOMPARE(disconnectedSpy.count(), 0);

    // Enter encrypted mode
    socket.startClientEncryption();
    QCOMPARE(socket.mode(), QSslSocket::SslClientMode);
    QVERIFY(!socket.isEncrypted());
    QCOMPARE(connectionEncryptedSpy.count(), 0);
    QCOMPARE(sslErrorsSpy.count(), 0);
    QCOMPARE(peerVerifyErrorSpy.count(), 0);

    // Start handshake.
    enterLoop(10);

    // We must get the PSK signal in all cases
    QCOMPARE(pskAuthenticationRequiredSpy.count(), 1);

    switch (pskTestType) {
    case PskConnectDoNotHandlePsk:
    case PskConnectEmptyCredentials:
    case PskConnectWrongCredentials:
    case PskConnectWrongIdentity:
    case PskConnectWrongPreSharedKey:
        // Handshake failure
        QCOMPARE(socketErrorsSpy.count(), 1);
        QCOMPARE(qvariant_cast<QAbstractSocket::SocketError>(socketErrorsSpy.at(0).at(0)), QAbstractSocket::SslHandshakeFailedError);
        QCOMPARE(sslErrorsSpy.count(), 0);
        QCOMPARE(peerVerifyErrorSpy.count(), 0);
        QCOMPARE(connectionEncryptedSpy.count(), 0);
        QVERIFY(!socket.isEncrypted());
        break;

    case PskConnectRightCredentialsPeerVerifyFailure:
        // Peer verification failure
        QCOMPARE(socketErrorsSpy.count(), 1);
        QCOMPARE(qvariant_cast<QAbstractSocket::SocketError>(socketErrorsSpy.at(0).at(0)), QAbstractSocket::SslHandshakeFailedError);
        QCOMPARE(sslErrorsSpy.count(), 1);
        QCOMPARE(peerVerifyErrorSpy.count(), 1);
        QCOMPARE(connectionEncryptedSpy.count(), 0);
        QVERIFY(!socket.isEncrypted());
        break;

    case PskConnectRightCredentialsVerifyPeer:
        // Peer verification failure, but ignore it and keep connecting
        QCOMPARE(socketErrorsSpy.count(), 0);
        QCOMPARE(sslErrorsSpy.count(), 1);
        QCOMPARE(peerVerifyErrorSpy.count(), 1);
        QCOMPARE(connectionEncryptedSpy.count(), 1);
        QVERIFY(socket.isEncrypted());
        QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
        break;

    case PskConnectRightCredentialsDoNotVerifyPeer:
        // No peer verification => no failure
        QCOMPARE(socketErrorsSpy.count(), 0);
        QCOMPARE(sslErrorsSpy.count(), 0);
        QCOMPARE(peerVerifyErrorSpy.count(), 0);
        QCOMPARE(connectionEncryptedSpy.count(), 1);
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
    QCOMPARE(disconnectedSpy.count(), 1);
}
#endif // QT_NO_OPENSSL

#endif // QT_NO_SSL

QTEST_MAIN(tst_QSslSocket)

#include "tst_qsslsocket.moc"
