// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qcoreapplication.h>

#include <QtCore/qthread.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>

#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qsslkey.h>

// Client and/or server presents a certificate signed by a system-trusted CA
// but the other side presents a certificate signed by a different CA.
constexpr bool TestServerPresentsIncorrectCa = true;
constexpr bool TestClientPresentsIncorrectCa = false;
// Decides whether or not to put the root CA into the global ssl configuration
// or into the socket's specific ssl configuration.
constexpr bool UseGlobalConfiguration = true;

/* No built-in QSslServer in this branch .... */
class QSslServer : public QTcpServer
{
    Q_OBJECT
public:
    QSslServer(QObject *parent = nullptr) : QTcpServer(parent) {}
    void setSslConfiguration(const QSslConfiguration &config)
    {
        m_config = config;
    }
    QSslConfiguration sslConfiguration() const
    {
        return m_config;
    }

protected:
    void incomingConnection(qintptr handle) override
    {
        QSslSocket *socket = new QSslSocket(this);
        socket->setSocketDescriptor(handle, QAbstractSocket::ConnectedState);
        socket->setSslConfiguration(m_config);
        socket->startServerEncryption();

        QObject::connect(socket, &QSslSocket::peerVerifyError, this, [this, socket](const QSslError &error) {
            emit peerVerifyError(socket, error);
        });

        emit startedEncryptionHandshake(socket);
        if (socket->waitForEncrypted()) {
            QTcpServer::addPendingConnection(socket);
            emit pendingConnectionAvailable(socket);
        } else {
            emit errorOccurred(socket, socket->error());
            socket->disconnectFromHost();
        }
    }

signals:
    void pendingConnectionAvailable(QSslSocket *socket);
    void startedEncryptionHandshake(QSslSocket *socket);
    void errorOccurred(QSslSocket *socket, QAbstractSocket::SocketError error);
    void peerVerifyError(QSslSocket *socket, const QSslError &error);

private:
    QSslConfiguration m_config;
};

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
            QSslSocket *socket = qobject_cast<QSslSocket *>(server.nextPendingConnection());
            qDebug() << "[s] newConnection" << socket->peerAddress() << socket->peerPort();
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

    if (!QFileInfo(":/rootCA.pem").exists())
        qFatal("rootCA.pem not found. Did you run generate.sh in the certs directory?");

    if (UseGlobalConfiguration) {
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        config.setCaCertificates(QSslCertificate::fromPath(":/rootCA.pem"));
        QSslConfiguration::setDefaultConfiguration(config);
    }

    ServerThread serverThread;
    serverThread.start();

    QSslSocket socket;
    QSslConfiguration config = socket.sslConfiguration();
    QString certificatePath;
    QString keyFileName;
    if (TestClientPresentsIncorrectCa) { // true: Present cert signed with incorrect CA: should fail
        certificatePath = ":/127.0.0.1-client.pem";
        keyFileName = ":/127.0.0.1-client-key.pem";
    } else { // false: Use correct CA: should succeed
        certificatePath = ":/accepted-client.pem";
        keyFileName = ":/accepted-client-key.pem";
    }
    config.setLocalCertificate(QSslCertificate::fromPath(certificatePath).first());
    if (!UseGlobalConfiguration && TestServerPresentsIncorrectCa) {
        // Verify server using incorrect CA: should fail
        config.setCaCertificates(QSslCertificate::fromPath(":/rootCA.pem"));
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
    QObject::connect(&socket, qOverload<const QList<QSslError> &>(&QSslSocket::sslErrors),
        [](const QList<QSslError> &errors) {
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
