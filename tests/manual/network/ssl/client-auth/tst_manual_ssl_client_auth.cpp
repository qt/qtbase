// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qcoreapplication.h>

#include <QtCore/qthread.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>

#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qsslserver.h>
#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qsslkey.h>

// Client and/or server presents a certificate signed by a system-trusted CA
// but the other side presents a certificate signed by a different CA.
constexpr bool TestServerPresentsIncorrectCa = false;
constexpr bool TestClientPresentsIncorrectCa = true;
// Decides whether or not to put the root CA into the global ssl configuration
// or into the socket's specific ssl configuration.
constexpr bool UseGlobalConfiguration = true;

class ServerThread : public QThread
{
    Q_OBJECT
public:
    void run() override
    {
        QSslServer server;

        QSslConfiguration config = server.sslConfiguration();
        if (!UseGlobalConfiguration) {
            QList<QSslCertificate> certs = QSslCertificate::fromPath(QStringLiteral(":/rootCA.pem"));
            config.setCaCertificates(certs);
        }
        config.setLocalCertificate(QSslCertificate::fromPath(QStringLiteral(":/127.0.0.1.pem"))
                                           .first());
        QFile keyFile(QStringLiteral(":/127.0.0.1-key.pem"));
        if (!keyFile.open(QIODevice::ReadOnly))
            qFatal("Failed to open key file");
        config.setPrivateKey(QSslKey(&keyFile, QSsl::Rsa));
        config.setPeerVerifyMode(QSslSocket::VerifyPeer);
        server.setSslConfiguration(config);

        connect(&server, &QSslServer::pendingConnectionAvailable, [&server]() {
            QSslSocket *socket = static_cast<QSslSocket *>(server.nextPendingConnection());
            qDebug() << "[s] newConnection" << socket->peerAddress() << socket->peerPort();
            socket->disconnectFromHost();
            qApp->quit();
        });
        connect(&server, &QSslServer::startedEncryptionHandshake, [](QSslSocket *socket) {
            qDebug() << "[s] new handshake" << socket->peerAddress() << socket->peerPort();
        });
        connect(&server, &QSslServer::errorOccurred,
                [](QSslSocket *socket, QAbstractSocket::SocketError error) {
                    qDebug() << "[s] errorOccurred" << socket->peerAddress() << socket->peerPort()
                             << error << socket->errorString();
                });
        connect(&server, &QSslServer::peerVerifyError,
                [](QSslSocket *socket, const QSslError &error) {
                    qDebug() << "[s] peerVerifyError" << socket->peerAddress() << socket->peerPort()
                             << error;
                });
        server.listen(QHostAddress::LocalHost, 24242);

        exec();

        server.close();
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    using namespace Qt::StringLiterals;

    if (!QFileInfo(u":/rootCA.pem"_s).exists())
        qFatal("rootCA.pem not found. Did you run generate.sh in the certs directory?");

    if (UseGlobalConfiguration) {
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        config.setCaCertificates(QSslCertificate::fromPath(u":/rootCA.pem"_s));
        QSslConfiguration::setDefaultConfiguration(config);
    }

    ServerThread serverThread;
    serverThread.start();

    QSslSocket socket;
    QSslConfiguration config = socket.sslConfiguration();
    QString certificatePath;
    QString keyFileName;
    if constexpr (TestClientPresentsIncorrectCa) { // true: Present cert signed with incorrect CA: should fail
        certificatePath = u":/127.0.0.1-client.pem"_s;
        keyFileName = u":/127.0.0.1-client-key.pem"_s;
    } else { // false: Use correct CA: should succeed
        certificatePath = u":/accepted-client.pem"_s;
        keyFileName = u":/accepted-client-key.pem"_s;
    }
    config.setLocalCertificate(QSslCertificate::fromPath(certificatePath).first());
    if (!UseGlobalConfiguration && TestServerPresentsIncorrectCa) {
        // Verify server using incorrect CA: should fail
        config.setCaCertificates(QSslCertificate::fromPath(u":/rootCA.pem"_s));
    } else if (UseGlobalConfiguration && !TestServerPresentsIncorrectCa) {
        // Verify server using correct CA, we need to explicitly set the
        // system CAs when the global config is overridden.
        config.setCaCertificates(QSslConfiguration::systemCaCertificates());
    }
    QFile keyFile(keyFileName);
    if (!keyFile.open(QIODevice::ReadOnly))
        qFatal("Failed to open key file");
    config.setPrivateKey(QSslKey(&keyFile, QSsl::Rsa));

    socket.setSslConfiguration(config);

    QObject::connect(&socket, &QSslSocket::encrypted, []() { qDebug() << "[c] encrypted"; });
    QObject::connect(&socket, &QSslSocket::errorOccurred,
            [&socket](QAbstractSocket::SocketError error) {
                qDebug() << "[c] errorOccurred" << error << socket.errorString();
                qApp->quit();
            });
    QObject::connect(&socket, &QSslSocket::sslErrors, [](const QList<QSslError> &errors) {
        qDebug() << "[c] sslErrors" << errors;
    });
    QObject::connect(&socket, &QSslSocket::connected, []() { qDebug() << "[c] connected"; });

    socket.connectToHostEncrypted(QStringLiteral("127.0.0.1"), 24242);

    const int res = app.exec();
    serverThread.quit();
    serverThread.wait();
    return res;
}

#include "tst_manual_ssl_client_auth.moc"
