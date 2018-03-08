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
#include <QtNetwork/QtNetwork>
#include <QtCore/QDateTime>
#include <QtCore/QTextStream>
#include <QtCore/QRandomGenerator>
#include <QtCore/QStandardPaths>
#include <QtCore/private/qiodevice_p.h>

#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/qnetworkconfigmanager.h>
#include <QtNetwork/qnetworkconfiguration.h>
#include <QtNetwork/qnetworksession.h>
#endif

#include "../../network-settings.h"

class tst_NetworkSelfTest: public QObject
{
    Q_OBJECT
    QHostAddress cachedIpAddress;
public:
    tst_NetworkSelfTest();
    virtual ~tst_NetworkSelfTest();

    QHostAddress serverIpAddress();

private slots:
    void initTestCase();
    void hostTest();
    void dnsResolution_data();
    void dnsResolution();
    void serverReachability();
    void remotePortsOpen_data();
    void remotePortsOpen();

    // specific protocol tests
    void ftpServer();
    void ftpProxyServer();
    void imapServer();
    void httpServer();
    void httpServerFiles_data();
    void httpServerFiles();
    void httpServerCGI_data();
    void httpServerCGI();
#ifndef QT_NO_SSL
    void httpsServer();
#endif
    void httpProxy();
    void httpProxyBasicAuth();
    void httpProxyNtlmAuth();
    void socks5Proxy();
    void socks5ProxyAuth();
    void smbServer();

    // ssl supported test
    void supportsSsl();
private:
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkConfigurationManager *netConfMan;
    QNetworkConfiguration networkConfiguration;
    QScopedPointer<QNetworkSession> networkSession;
#endif
};

class Chat
{
public:
    enum Type {
        Reconnect,
        Send,
        Expect,
        SkipBytes,
        DiscardUntil,
        DiscardUntilDisconnect,
        Disconnect,
        RemoteDisconnect,
        StartEncryption
    };
    Chat(Type t, const QByteArray &d)
        : data(d), type(t)
    {
    }
    Chat(Type t, int val = 0)
        : value(val), type(t)
    {
    }

    static inline Chat send(const QByteArray &data)
    { return Chat(Send, data); }
    static inline Chat expect(const QByteArray &data)
    { return Chat(Expect, data); }
    static inline Chat discardUntil(const QByteArray &data)
    { return Chat(DiscardUntil, data); }
    static inline Chat skipBytes(int count)
    { return Chat(SkipBytes, count); }

    QByteArray data;
    int value;
    Type type;
};

static QString prettyByteArray(const QByteArray &array)
{
    // any control chars?
    QString result;
    result.reserve(array.length() + array.length() / 3);
    for (int i = 0; i < array.length(); ++i) {
        char c = array.at(i);
        switch (c) {
        case '\n':
            result += "\\n";
            continue;
        case '\r':
            result += "\\r";
            continue;
        case '\t':
            result += "\\t";
            continue;
        case '"':
            result += "\\\"";
            continue;
        default:
            break;
        }

        if (c < 0x20 || uchar(c) >= 0x7f) {
            result += '\\';
            result += QString::number(uchar(c), 8);
        } else {
            result += c;
        }
    }
    return result;
}

enum { defaultReadTimeoutMS = 4000 };

static bool doSocketRead(QTcpSocket *socket, int minBytesAvailable, int timeout = defaultReadTimeoutMS)
{
    QElapsedTimer timer;
    timer.start();
    int t = timeout;
    forever {
        if (socket->bytesAvailable() >= minBytesAvailable)
            return true;
        if (socket->state() == QAbstractSocket::UnconnectedState)
            return false;
        if (!socket->waitForReadyRead(t))
            return false;
        t = qt_subtract_from_timeout(timeout, timer.elapsed());
        if (t == 0)
            return false;
    }
}

static QByteArray msgDoSocketReadFailed(const QString &host, quint16 port,
                                        int step, int minBytesAvailable)
{
    return "Failed to receive "
        + QByteArray::number(minBytesAvailable) + " bytes from "
        + host.toLatin1() + ':' + QByteArray::number(port)
        + " in step " + QByteArray::number(step) + ": timeout";
}

static bool doSocketFlush(QTcpSocket *socket, int timeout = 4000)
{
#ifndef QT_NO_SSL
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket);
#endif
    QTime timer;
    timer.start();
    int t = timeout;
    forever {
        if (socket->bytesToWrite() == 0
#ifndef QT_NO_SSL
            && sslSocket->encryptedBytesToWrite() == 0
#endif
            )
            return true;
        if (socket->state() == QAbstractSocket::UnconnectedState)
            return false;
        if (!socket->waitForBytesWritten(t))
            return false;
        t = qt_subtract_from_timeout(timeout, timer.elapsed());
        if (t == 0)
            return false;
    }
}

static void netChat(int port, const QList<Chat> &chat)
{
#ifndef QT_NO_SSL
    QSslSocket socket;
#else
    QTcpSocket socket;
#endif

    socket.connectToHost(QtNetworkSettings::serverName(), port);
    qDebug() << 0 << "Connecting to server on port" << port;
    QVERIFY2(socket.waitForConnected(10000),
             QString("Failed to connect to server in step 0: %1").arg(socket.errorString()).toLocal8Bit());

    // now start the chat
    QList<Chat>::ConstIterator it = chat.constBegin();
    for (int i = 1; it != chat.constEnd(); ++it, ++i) {
        switch (it->type) {
            case Chat::Expect: {
                    qDebug() << i << "Expecting" << prettyByteArray(it->data);
                    if (!doSocketRead(&socket, it->data.length(), 3 * defaultReadTimeoutMS))
                        QFAIL(msgDoSocketReadFailed(QtNetworkSettings::serverName(), port, i, it->data.length()));

                    // pop that many bytes off the socket
                    QByteArray received = socket.read(it->data.length());

                    // is it what we expected?
                    QVERIFY2(received == it->data,
                             QString("Did not receive expected data in step %1: data received was:\n%2")
                             .arg(i).arg(prettyByteArray(received)).toLocal8Bit());

                    break;
                }

            case Chat::DiscardUntil:
                qDebug() << i << "Discarding until" << prettyByteArray(it->data);
                while (true) {
                    // scan the buffer until we have our string
                    if (!doSocketRead(&socket, it->data.length()))
                        QFAIL(msgDoSocketReadFailed(QtNetworkSettings::serverName(), port, i, it->data.length()));

                    QByteArray buffer;
                    buffer.resize(socket.bytesAvailable());
                    socket.peek(buffer.data(), socket.bytesAvailable());

                    int pos = buffer.indexOf(it->data);
                    if (pos == -1) {
                        // data not found, keep trying
                        continue;
                    }

                    buffer = socket.read(pos + it->data.length());
                    qDebug() << i << "Discarded" << prettyByteArray(buffer);
                    break;
                }
                break;

            case Chat::SkipBytes: {
                    qDebug() << i << "Skipping" << it->value << "bytes";
                    if (!doSocketRead(&socket, it->value))
                        QFAIL(msgDoSocketReadFailed(QtNetworkSettings::serverName(), port, i, it->value));

                    // now discard the bytes
                    QByteArray buffer = socket.read(it->value);
                    qDebug() << i << "Skipped" << prettyByteArray(buffer);
                    break;
                }

            case Chat::Send: {
                    qDebug() << i << "Sending" << prettyByteArray(it->data);
                    socket.write(it->data);
                    if (!doSocketFlush(&socket)) {
                        QVERIFY2(socket.state() == QAbstractSocket::ConnectedState,
                                 QString("Socket disconnected while sending data in step %1").arg(i).toLocal8Bit());
                        QFAIL(QString("Failed to send data in step %1: timeout").arg(i).toLocal8Bit());
                    }
                    break;
                }

            case Chat::Disconnect:
                qDebug() << i << "Disconnecting from host";
                socket.disconnectFromHost();

                // is this the last command?
                if (it + 1 != chat.constEnd())
                    break;

                // fall through:
            case Chat::RemoteDisconnect:
            case Chat::DiscardUntilDisconnect:
                qDebug() << i << "Waiting for remote disconnect";
                if (socket.state() != QAbstractSocket::UnconnectedState)
                    socket.waitForDisconnected(10000);
                QVERIFY2(socket.state() == QAbstractSocket::UnconnectedState,
                         QString("Socket did not disconnect as expected in step %1").arg(i).toLocal8Bit());

                // any data left?
                if (it->type == Chat::DiscardUntilDisconnect) {
                    QByteArray buffer = socket.readAll();
                    qDebug() << i << "Discarded in the process:" << prettyByteArray(buffer);
                }

                if (socket.bytesAvailable() != 0)
                    QFAIL(QString("Unexpected bytes still on buffer when disconnecting in step %1:\n%2")
                          .arg(i).arg(prettyByteArray(socket.readAll())).toLocal8Bit());
                break;

            case Chat::Reconnect:
                qDebug() << i << "Reconnecting to server on port" << port;
                socket.connectToHost(QtNetworkSettings::serverName(), port);
                QVERIFY2(socket.waitForConnected(10000),
                         QString("Failed to reconnect to server in step %1: %2").arg(i).arg(socket.errorString()).toLocal8Bit());
                break;

            case Chat::StartEncryption:
#ifdef QT_NO_SSL
                QFAIL("Internal error: SSL required for this test");
#else
                qDebug() << i << "Starting client encryption";
                socket.ignoreSslErrors();
                socket.startClientEncryption();
                QVERIFY2(socket.waitForEncrypted(5000),
                         QString("Failed to start client encryption in step %1: %2").arg(i)
                         .arg(socket.errorString()).toLocal8Bit());
                break;
#endif
            }
    }
}

tst_NetworkSelfTest::tst_NetworkSelfTest()
{
}

tst_NetworkSelfTest::~tst_NetworkSelfTest()
{
}

QHostAddress tst_NetworkSelfTest::serverIpAddress()
{
    if (cachedIpAddress.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol) {
        // need resolving
        QHostInfo resolved = QHostInfo::fromName(QtNetworkSettings::serverName());
        if(resolved.error() != QHostInfo::NoError ||
            resolved.addresses().isEmpty()) {
            qWarning("QHostInfo::fromName failed (%d).", resolved.error());
            return QHostAddress(QHostAddress::Null);
        }
        cachedIpAddress = resolved.addresses().first();
    }
    return cachedIpAddress;
}

void tst_NetworkSelfTest::initTestCase()
{
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
#ifndef QT_NO_BEARERMANAGEMENT
    netConfMan = new QNetworkConfigurationManager(this);
    networkConfiguration = netConfMan->defaultConfiguration();
    networkSession.reset(new QNetworkSession(networkConfiguration));
    if (!networkSession->isOpen()) {
        networkSession->open();
        QVERIFY(networkSession->waitForOpened(30000));
    }
#endif
}

void tst_NetworkSelfTest::hostTest()
{
    // this is a localhost self-test
    QHostInfo localhost = QHostInfo::fromName("localhost");
    QCOMPARE(localhost.error(), QHostInfo::NoError);
    QVERIFY(!localhost.addresses().isEmpty());

    QTcpServer server;
    QVERIFY(server.listen());

    QTcpSocket socket;
    socket.connectToHost("127.0.0.1", server.serverPort());
    QVERIFY(socket.waitForConnected(10000));
}

void tst_NetworkSelfTest::dnsResolution_data()
{
    QTest::addColumn<QString>("hostName");
    QTest::newRow("local-name") << QtNetworkSettings::serverLocalName();
    QTest::newRow("fqdn") << QtNetworkSettings::serverName();
}

void tst_NetworkSelfTest::dnsResolution()
{
    QFETCH(QString, hostName);
    QHostInfo resolved = QHostInfo::fromName(hostName);
    QVERIFY2(resolved.error() == QHostInfo::NoError,
             QString("Failed to resolve hostname %1: %2").arg(hostName, resolved.errorString()).toLocal8Bit());
    QVERIFY2(resolved.addresses().size() > 0, "Got 0 addresses for server IP");

    cachedIpAddress = resolved.addresses().first();
}

void tst_NetworkSelfTest::serverReachability()
{
    // check that we get a proper error connecting to port 12346
    QTcpSocket socket;
    socket.connectToHost(QtNetworkSettings::serverName(), 12346);

    QTime timer;
    timer.start();
    socket.waitForConnected(10000);
    QVERIFY2(timer.elapsed() < 9900, "Connection to closed port timed out instead of refusing, something is wrong");

    QVERIFY2(socket.state() == QAbstractSocket::UnconnectedState, "Socket connected unexpectedly!");
    QVERIFY2(socket.error() == QAbstractSocket::ConnectionRefusedError,
             QString("Could not reach server: %1").arg(socket.errorString()).toLocal8Bit());
}

void tst_NetworkSelfTest::remotePortsOpen_data()
{
    QTest::addColumn<int>("portNumber");
    QTest::newRow("echo") << 7;
    QTest::newRow("daytime") << 13;
    QTest::newRow("ftp") << 21;
    QTest::newRow("ssh") << 22;
    QTest::newRow("imap") << 143;
    QTest::newRow("http") << 80;
    QTest::newRow("https") << 443;
    QTest::newRow("http-proxy") << 3128;
    QTest::newRow("http-proxy-auth-basic") << 3129;
    QTest::newRow("http-proxy-auth-ntlm") << 3130;
    QTest::newRow("socks5-proxy") << 1080;
    QTest::newRow("socks5-proxy-auth") << 1081;
    QTest::newRow("ftp-proxy") << 2121;
    QTest::newRow("smb") << 139;
}

void tst_NetworkSelfTest::remotePortsOpen()
{
    QFETCH(int, portNumber);
    QTcpSocket socket;
    socket.connectToHost(QtNetworkSettings::serverName(), portNumber);

    if (!socket.waitForConnected(10000)) {
        if (socket.error() == QAbstractSocket::SocketTimeoutError)
            QFAIL(QString("Network timeout connecting to the server on port %1").arg(portNumber).toLocal8Bit());
        else
            QFAIL(QString("Error connecting to server on port %1: %2").arg(portNumber).arg(socket.errorString()).toLocal8Bit());
    }
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
}

static QList<Chat> ftpChat(const QByteArray &userSuffix = QByteArray())
{
    QList<Chat> rv;
    rv << Chat::expect("220")
            << Chat::discardUntil("\r\n")
            << Chat::send("USER anonymous" + userSuffix + "\r\n")
            << Chat::expect("331")
            << Chat::discardUntil("\r\n")
            << Chat::send("PASS user@hostname\r\n")
            << Chat::expect("230")
            << Chat::discardUntil("\r\n")

            << Chat::send("CWD pub\r\n")
            << Chat::expect("250")
            << Chat::discardUntil("\r\n")
            << Chat::send("CWD dir-not-readable\r\n")
            << Chat::expect("550")
            << Chat::discardUntil("\r\n")
            << Chat::send("PWD\r\n")
            << Chat::expect("257 \"/pub\"\r\n")
            << Chat::send("SIZE file-not-readable.txt\r\n")
            << Chat::expect("213 41\r\n")
            << Chat::send("CWD qxmlquery\r\n")
            << Chat::expect("250")
            << Chat::discardUntil("\r\n")

            << Chat::send("CWD /qtest\r\n")
            << Chat::expect("250")
            << Chat::discardUntil("\r\n")
            << Chat::send("SIZE bigfile\r\n")
            << Chat::expect("213 519240\r\n")
            << Chat::send("SIZE rfc3252\r\n")
            << Chat::expect("213 25962\r\n")
            << Chat::send("SIZE rfc3252.txt\r\n")
            << Chat::expect("213 25962\r\n")
//            << Chat::send("SIZE nonASCII/german_\344\366\374\304\326\334\337\r\n")
//            << Chat::expect("213 40\r\n")

            << Chat::send("QUIT\r\n");
        rv  << Chat::expect("221")
            << Chat::discardUntil("\r\n");

    rv << Chat::RemoteDisconnect;
    return rv;
}

void tst_NetworkSelfTest::ftpServer()
{
    netChat(21, ftpChat());
}

void tst_NetworkSelfTest::ftpProxyServer()
{
    netChat(2121, ftpChat("@" + QtNetworkSettings::serverName().toLatin1()));
}

void tst_NetworkSelfTest::imapServer()
{
    netChat(143, QList<Chat>()
            << Chat::expect("* OK ")
            << Chat::discardUntil("\r\n")
            << Chat::send("1 CAPABILITY\r\n")
            << Chat::expect("* CAPABILITY ")
            << Chat::discardUntil("1 OK")
            << Chat::discardUntil("\r\n")
            << Chat::send("2 LOGOUT\r\n")
            << Chat::discardUntil("2 OK")
            << Chat::discardUntil("\r\n")
            << Chat::RemoteDisconnect);
}

void tst_NetworkSelfTest::httpServer()
{
    QByteArray uniqueExtension = QByteArray::number((qulonglong)this) +
                                 QByteArray::number((qulonglong)QRandomGenerator::global()->generate()) +
                                 QByteArray::number(QDateTime::currentSecsSinceEpoch());

    netChat(80, QList<Chat>()
            // HTTP/0.9 chat:
            << Chat::send("GET /\r\n")
            << Chat::DiscardUntilDisconnect

            // HTTP/1.0 chat:
            << Chat::Reconnect
            << Chat::send("GET / HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Connection: close\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::DiscardUntilDisconnect

            // HTTP/1.0 POST:
            << Chat::Reconnect
            << Chat::send("POST / HTTP/1.0\r\n"
                          "Content-Length: 5\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "Hello")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::DiscardUntilDisconnect

            // HTTP protected area
            << Chat::Reconnect
            << Chat::send("GET /qtest/protected/rfc3252.txt HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Connection: close\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("401 ")
            << Chat::DiscardUntilDisconnect

            << Chat::Reconnect
            << Chat::send("HEAD /qtest/protected/rfc3252.txt HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Connection: close\r\n"
                          "Authorization: Basic cXNvY2tzdGVzdDpwYXNzd29yZA==\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::DiscardUntilDisconnect

            // DAV area
            << Chat::Reconnect
            << Chat::send("HEAD /dav/ HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Connection: close\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::DiscardUntilDisconnect

            // HTTP/1.0 PUT
            << Chat::Reconnect
            << Chat::send("PUT /dav/networkselftest-" + uniqueExtension + ".txt HTTP/1.0\r\n"
                          "Content-Length: 5\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "Hello")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("201 ")
            << Chat::DiscardUntilDisconnect

            // check that the file did get uploaded
            << Chat::Reconnect
            << Chat::send("HEAD /dav/networkselftest-" + uniqueExtension + ".txt HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Connection: close\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::discardUntil("\r\nContent-Length: 5\r\n")
            << Chat::DiscardUntilDisconnect

            // HTTP/1.0 DELETE
            << Chat::Reconnect
            << Chat::send("DELETE /dav/networkselftest-" + uniqueExtension + ".txt HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Connection: close\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("204 ")
            << Chat::DiscardUntilDisconnect
            );
}

void tst_NetworkSelfTest::httpServerFiles_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<int>("size");

    QTest::newRow("fluke.gif") << "/qtest/fluke.gif" << -1;
    QTest::newRow("bigfile") << "/qtest/bigfile" << 519240;
    QTest::newRow("rfc3252.txt") << "/qtest/rfc3252.txt" << 25962;
    QTest::newRow("protected/rfc3252.txt") << "/qtest/protected/rfc3252.txt" << 25962;
    QTest::newRow("completelyEmptyQuery.xq") << "/qtest/qxmlquery/completelyEmptyQuery.xq" << -1;
    QTest::newRow("notWellformedViaHttps.xml") << "/qtest/qxmlquery/notWellformedViaHttps.xml" << -1;
    QTest::newRow("notWellformed.xml") << "/qtest/qxmlquery/notWellformed.xml" << -1;
    QTest::newRow("viaHttp.xq") << "/qtest/qxmlquery/viaHttp.xq" << -1;
    QTest::newRow("wellFormedViaHttps.xml") << "/qtest/qxmlquery/wellFormedViaHttps.xml" << -1;
    QTest::newRow("wellFormed.xml") << "/qtest/qxmlquery/wellFormed.xml" << -1;
}

void tst_NetworkSelfTest::httpServerFiles()
{
    QFETCH(QString, uri);
    QFETCH(int, size);
    QUrl url(uri);

    QList<Chat> chat;
    chat << Chat::send("HEAD " + url.toEncoded() + " HTTP/1.0\r\n"
                       "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                       "Connection: close\r\n"
                       "Authorization: Basic cXNvY2tzdGVzdDpwYXNzd29yZA==\r\n"
                       "\r\n")
         << Chat::expect("HTTP/1.")
         << Chat::skipBytes(1) // HTTP/1.0 or 1.1 reply
         << Chat::expect(" 200 ");
    if (size != -1)
        chat << Chat::discardUntil("\r\nContent-Length: " + QByteArray::number(size) + "\r\n");
    chat << Chat::DiscardUntilDisconnect;
    netChat(80, chat);
}

void tst_NetworkSelfTest::httpServerCGI_data()
{
    QTest::addColumn<QByteArray>("request");
    QTest::addColumn<QByteArray>("result");
    QTest::addColumn<QByteArray>("additionalHeader");

    QTest::newRow("echo.cgi")
            << QByteArray("GET /qtest/cgi-bin/echo.cgi?Hello+World HTTP/1.0\r\n"
                          "Connection: close\r\n"
                          "\r\n")
            << QByteArray("Hello+World")
            << QByteArray();

    QTest::newRow("echo.cgi(POST)")
            << QByteArray("POST /qtest/cgi-bin/echo.cgi?Hello+World HTTP/1.0\r\n"
                          "Connection: close\r\n"
                          "Content-Length: 15\r\n"
                          "\r\n"
                          "Hello, World!\r\n")
            << QByteArray("Hello, World!\r\n")
            << QByteArray();

    QTest::newRow("md5sum.cgi")
            << QByteArray("POST /qtest/cgi-bin/md5sum.cgi HTTP/1.0\r\n"
                          "Connection: close\r\n"
                          "Content-Length: 15\r\n"
                          "\r\n"
                          "Hello, World!\r\n")
            << QByteArray("29b933a8d9a0fcef0af75f1713f4940e\n")
            << QByteArray();

    QTest::newRow("protected/md5sum.cgi")
            << QByteArray("POST /qtest/protected/cgi-bin/md5sum.cgi HTTP/1.0\r\n"
                          "Connection: close\r\n"
                          "Authorization: Basic cXNvY2tzdGVzdDpwYXNzd29yZA==\r\n"
                          "Content-Length: 15\r\n"
                          "\r\n"
                          "Hello, World!\r\n")
            << QByteArray("29b933a8d9a0fcef0af75f1713f4940e\n")
            << QByteArray();

    QTest::newRow("set-cookie.cgi")
            << QByteArray("POST /qtest/cgi-bin/set-cookie.cgi HTTP/1.0\r\n"
                          "Connection: close\r\n"
                          "Content-Length: 8\r\n"
                          "\r\n"
                          "foo=bar\n")
            << QByteArray("Success\n")
            << QByteArray("\r\nSet-Cookie: foo=bar\r\n");
}

void tst_NetworkSelfTest::httpServerCGI()
{
    QFETCH(QByteArray, request);
    QFETCH(QByteArray, result);
    QFETCH(QByteArray, additionalHeader);
    QList<Chat> chat;
    chat << Chat::send(request)
         << Chat::expect("HTTP/1.") << Chat::skipBytes(1)
         << Chat::expect(" 200 ");

    if (!additionalHeader.isEmpty())
        chat << Chat::discardUntil(additionalHeader);

    chat << Chat::discardUntil("\r\n\r\n")
         << Chat::expect(result)
         << Chat::RemoteDisconnect;
    netChat(80, chat);
}

#ifndef QT_NO_SSL
void tst_NetworkSelfTest::httpsServer()
{
    netChat(443, QList<Chat>()
            << Chat::StartEncryption
            << Chat::send("GET / HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Connection: close\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::DiscardUntilDisconnect);
}
#endif

void tst_NetworkSelfTest::httpProxy()
{
    netChat(3128, QList<Chat>()
            // proxy GET by IP
            << Chat::send("GET http://" + serverIpAddress().toString().toLatin1() + "/ HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Proxy-connection: close\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::DiscardUntilDisconnect

            // proxy GET by hostname
            << Chat::Reconnect
            << Chat::send("GET http://" + QtNetworkSettings::serverName().toLatin1() + "/ HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Proxy-connection: close\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::DiscardUntilDisconnect

            // proxy CONNECT by IP
            << Chat::Reconnect
            << Chat::send("CONNECT " + serverIpAddress().toString().toLatin1() + ":21 HTTP/1.0\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::discardUntil("\r\n\r\n")
            << ftpChat()

            // proxy CONNECT by hostname
            << Chat::Reconnect
            << Chat::send("CONNECT " + QtNetworkSettings::serverName().toLatin1() + ":21 HTTP/1.0\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::discardUntil("\r\n\r\n")
            << ftpChat()
            );
}

void tst_NetworkSelfTest::httpProxyBasicAuth()
{
    netChat(3129, QList<Chat>()
            // test auth required response
            << Chat::send("GET http://" + QtNetworkSettings::serverName().toLatin1() + "/ HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Proxy-connection: close\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("407 ")
            << Chat::discardUntil("\r\nProxy-Authenticate: Basic realm=\"")
            << Chat::DiscardUntilDisconnect

            // now try sending our credentials
            << Chat::Reconnect
            << Chat::send("GET http://" + QtNetworkSettings::serverName().toLatin1() + "/ HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Proxy-connection: close\r\n"
                          "Proxy-Authorization: Basic cXNvY2tzdGVzdDpwYXNzd29yZA==\r\n"
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("200 ")
            << Chat::DiscardUntilDisconnect);
}

void tst_NetworkSelfTest::httpProxyNtlmAuth()
{
    netChat(3130, QList<Chat>()
            // test auth required response
            << Chat::send("GET http://" + QtNetworkSettings::serverName().toLatin1() + "/ HTTP/1.0\r\n"
                          "Host: " + QtNetworkSettings::serverName().toLatin1() + "\r\n"
                          "Proxy-connection: keep-alive\r\n" // NTLM auth will disconnect
                          "\r\n")
            << Chat::expect("HTTP/1.")
            << Chat::discardUntil(" ")
            << Chat::expect("407 ")
            << Chat::discardUntil("\r\nProxy-Authenticate: NTLM\r\n")
            << Chat::DiscardUntilDisconnect
            );
}

// SOCKSv5 is a binary protocol
static const char handshakeNoAuth[] = "\5\1\0";
static const char handshakeOkNoAuth[] = "\5\0";
static const char handshakeAuthPassword[] = "\5\1\2\1\12qsockstest\10password";
static const char handshakeOkPasswdAuth[] = "\5\2\1\0";
static const char handshakeAuthNotOk[] = "\5\377";
static const char connect1[] = "\5\1\0\1\177\0\0\1\0\25"; // Connect IPv4 127.0.0.1 port 21
static const char connect1a[] = "\5\1\0\1"; // just "Connect to IPv4"
static const char connect1b[] = "\0\25"; // just "port 21"
static const char connect2[] = "\5\1\0\3\11localhost\0\25"; // Connect hostname localhost 21
static const char connect2a[] = "\5\1\0\3"; // just "Connect to hostname"
static const char connected[] = "\5\0\0";

#define QBA(x) (QByteArray::fromRawData(x, int(sizeof(x)) - 1))

void tst_NetworkSelfTest::socks5Proxy()
{
    union {
        char buf[4];
        quint32 data;
    } ip4Address;
    ip4Address.data = qToBigEndian(serverIpAddress().toIPv4Address());

    const QByteArray handshakeNoAuthData = QByteArray(handshakeNoAuth, int(sizeof handshakeNoAuth) - 1);
    const QByteArray handshakeOkNoAuthData = QByteArray(handshakeOkNoAuth, int(sizeof handshakeOkNoAuth) - 1);
    const QByteArray connect1Data = QByteArray(connect1, int(sizeof connect1) - 1);
    const QByteArray connectedData = QByteArray(connected, int(sizeof connected) - 1);
    const QByteArray connect2Data = QByteArray(connect2, int(sizeof connect2) - 1);

    netChat(1080, QList<Chat>()
            // IP address connection
            << Chat::send(handshakeNoAuthData)
            << Chat::expect(handshakeOkNoAuthData)
            << Chat::send(connect1Data)
            << Chat::expect(connectedData)
            << Chat::expect("\1") // IPv4 address following
            << Chat::skipBytes(6) // the server's local address and port
            << ftpChat()

            // connect by IP
            << Chat::Reconnect
            << Chat::send(handshakeNoAuthData)
            << Chat::expect(handshakeOkNoAuthData)
            << Chat::send(QBA(connect1a) + QByteArray::fromRawData(ip4Address.buf, 4) + QBA(connect1b))
            << Chat::expect(connectedData)
            << Chat::expect("\1") // IPv4 address following
            << Chat::skipBytes(6) // the server's local address and port
            << ftpChat()

            // connect to "localhost" by hostname
            << Chat::Reconnect
            << Chat::send(handshakeNoAuthData)
            << Chat::expect(handshakeOkNoAuthData)
            << Chat::send(connect2Data)
            << Chat::expect(connectedData)
            << Chat::expect("\1") // IPv4 address following
            << Chat::skipBytes(6) // the server's local address and port
            << ftpChat()

            // connect to server by its official name
            << Chat::Reconnect
            << Chat::send(handshakeNoAuthData)
            << Chat::expect(handshakeOkNoAuthData)
            << Chat::send(QBA(connect2a) + char(QtNetworkSettings::serverName().size()) + QtNetworkSettings::serverName().toLatin1() + QBA(connect1b))
            << Chat::expect(connectedData)
            << Chat::expect("\1") // IPv4 address following
            << Chat::skipBytes(6) // the server's local address and port
            << ftpChat()
            );
}

void tst_NetworkSelfTest::socks5ProxyAuth()
{
    const QByteArray handshakeNoAuthData = QByteArray(handshakeNoAuth, int(sizeof handshakeNoAuth) - 1);
    const QByteArray connect1Data = QByteArray(connect1, int(sizeof connect1) - 1);
    const QByteArray connectedData = QByteArray(connected, int(sizeof connected) - 1);
    const QByteArray handshakeAuthNotOkData = QByteArray(handshakeAuthNotOk, int(sizeof(handshakeAuthNotOk)) - 1);
    const QByteArray handshakeAuthPasswordData = QByteArray(handshakeAuthPassword, int(sizeof(handshakeAuthPassword)) - 1);
    const QByteArray handshakeOkPasswdAuthData = QByteArray(handshakeOkPasswdAuth, int(sizeof(handshakeOkPasswdAuth)) - 1);

    netChat(1081, QList<Chat>()
            // unauthenticated connect -- will get error
            << Chat::send(handshakeNoAuthData)
            << Chat::expect(handshakeAuthNotOkData)
            << Chat::RemoteDisconnect

            // now try to connect with authentication
            << Chat::Reconnect
            << Chat::send(handshakeAuthPasswordData)
            << Chat::expect(handshakeOkPasswdAuthData)
            << Chat::send(connect1Data)
            << Chat::expect(connectedData)
            << Chat::expect("\1") // IPv4 address following
            << Chat::skipBytes(6) // the server's local address and port
            << ftpChat()
            );
}

void tst_NetworkSelfTest::supportsSsl()
{
#ifdef QT_NO_SSL
    QSKIP("SSL not compiled in");
#else
    QVERIFY2(QSslSocket::supportsSsl(), "Could not load SSL libraries");
#endif
}

#if QT_CONFIG(process)
static const QByteArray msgProcessError(const QProcess &process, const char *what)
{
    QString result;
    QTextStream(&result) << what << ": \"" << process.program() << ' '
        << process.arguments().join(QLatin1Char(' ')) << "\": " << process.errorString();
    return result.toLocal8Bit();
}

static void ensureTermination(QProcess &process)
{
    if (process.state() == QProcess::Running) {
        process.terminate();
        if (!process.waitForFinished(300))
            process.kill();
    }
}
#endif // QT_CONFIG(process)

void tst_NetworkSelfTest::smbServer()
{
    static const char contents[] = "This is 34 bytes. Do not change...";
#ifdef Q_OS_WIN
    // use Windows's native UNC support to try and open a file on the server
    QByteArray filepath = "\\\\" + QtNetworkSettings::winServerName().toLatin1() + "\\testshare\\test.pri";
    FILE *f = fopen(filepath.constData(), "rb");
    QVERIFY2(f, qt_error_string().toLocal8Bit());

    char buf[128];
    size_t ret = fread(buf, 1, sizeof buf, f);
    fclose(f);

    QCOMPARE(ret, strlen(contents));
    QVERIFY(memcmp(buf, contents, strlen(contents)) == 0);
#else
#if QT_CONFIG(process)
    enum { sambaTimeOutSecs = 5 };
    // try to use Samba
    const QString progname = "smbclient";
    const QString binary = QStandardPaths::findExecutable(progname);
    if (binary.isEmpty())
        QSKIP("Could not find smbclient (from Samba), cannot continue testing");

    // try listing the server
    const QStringList timeOutArguments = QStringList()
        << "--timeout" << QString::number(sambaTimeOutSecs);
    QStringList arguments = timeOutArguments;
    arguments << "-g" << "-N" << "-L" << QtNetworkSettings::winServerName();
    QProcess smbclient;
    smbclient.start(binary, arguments, QIODevice::ReadOnly);
    QVERIFY2(smbclient.waitForStarted(), msgProcessError(smbclient, "Unable to start"));
    const bool listFinished = smbclient.waitForFinished((1 + sambaTimeOutSecs) * 1000);
    ensureTermination(smbclient);
    QVERIFY2(listFinished, msgProcessError(smbclient, "Listing servers timed out"));
    if (smbclient.exitStatus() != QProcess::NormalExit)
        QSKIP("smbclient crashed");
    QVERIFY2(smbclient.exitCode() == 0, "Test server not found");

    QByteArray output = smbclient.readAll();
    QVERIFY(output.contains("Disk|testshare|"));
    QVERIFY(output.contains("Disk|testsharewritable|"));
    QVERIFY(output.contains("Disk|testsharelargefile|"));
    qDebug() << "Test server found and shares are correct";

    // try getting a file
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PAGER", "/bin/cat"); // just in case
    smbclient.setProcessEnvironment(env);
    arguments = timeOutArguments;
    arguments << "-N" << "-c" << "more test.pri"
        << ("\\\\" + QtNetworkSettings::winServerName() + "\\testshare");
    smbclient.start(binary, arguments, QIODevice::ReadOnly);
    const bool fileFinished = smbclient.waitForFinished((1 + sambaTimeOutSecs) * 1000);
    ensureTermination(smbclient);
    QVERIFY2(fileFinished, msgProcessError(smbclient, "Timed out"));
    if (smbclient.exitStatus() != QProcess::NormalExit)
        QSKIP("smbclient crashed");
    QVERIFY2(smbclient.exitCode() == 0, "File //qt-test-server/testshare/test.pri not found");

    output = smbclient.readAll();
    QCOMPARE(output.constData(), contents);
    qDebug() << "Test file is correct";
#else
    QSKIP( "No QProcess support", SkipAll);
#endif
#endif
}

QTEST_MAIN(tst_NetworkSelfTest)
#include "tst_networkselftest.moc"
