/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QNETWORKREPLY_H
#define QNETWORKREPLY_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>

QT_BEGIN_NAMESPACE


class QUrl;
class QVariant;
class QAuthenticator;
class QSslConfiguration;
class QSslError;
class QSslPreSharedKeyAuthenticator;

class QNetworkReplyPrivate;
class Q_NETWORK_EXPORT QNetworkReply: public QIODevice
{
    Q_OBJECT
public:
    enum NetworkError {
        NoError = 0,

        // network layer errors [relating to the destination server] (1-99):
        ConnectionRefusedError = 1,
        RemoteHostClosedError,
        HostNotFoundError,
        TimeoutError,
        OperationCanceledError,
        SslHandshakeFailedError,
        TemporaryNetworkFailureError,
        NetworkSessionFailedError,
        BackgroundRequestNotAllowedError,
        TooManyRedirectsError,
        InsecureRedirectError,
        UnknownNetworkError = 99,

        // proxy errors (101-199):
        ProxyConnectionRefusedError = 101,
        ProxyConnectionClosedError,
        ProxyNotFoundError,
        ProxyTimeoutError,
        ProxyAuthenticationRequiredError,
        UnknownProxyError = 199,

        // content errors (201-299):
        ContentAccessDenied = 201,
        ContentOperationNotPermittedError,
        ContentNotFoundError,
        AuthenticationRequiredError,
        ContentReSendError,
        ContentConflictError,
        ContentGoneError,
        UnknownContentError = 299,

        // protocol errors
        ProtocolUnknownError = 301,
        ProtocolInvalidOperationError,
        ProtocolFailure = 399,

        // Server side errors (401-499)
        InternalServerError = 401,
        OperationNotImplementedError,
        ServiceUnavailableError,
        UnknownServerError = 499
    };
    Q_ENUM(NetworkError)

    ~QNetworkReply();

    // reimplemented from QIODevice
    virtual void close() override;
    virtual bool isSequential() const override;

    // like QAbstractSocket:
    qint64 readBufferSize() const;
    virtual void setReadBufferSize(qint64 size);

    QNetworkAccessManager *manager() const;
    QNetworkAccessManager::Operation operation() const;
    QNetworkRequest request() const;
    NetworkError error() const;
    bool isFinished() const;
    bool isRunning() const;
    QUrl url() const;

    // "cooked" headers
    QVariant header(QNetworkRequest::KnownHeaders header) const;

    // raw headers:
    bool hasRawHeader(const QByteArray &headerName) const;
    QList<QByteArray> rawHeaderList() const;
    QByteArray rawHeader(const QByteArray &headerName) const;

    typedef QPair<QByteArray, QByteArray> RawHeaderPair;
    const QList<RawHeaderPair>& rawHeaderPairs() const;

    // attributes
    QVariant attribute(QNetworkRequest::Attribute code) const;

#if QT_CONFIG(ssl)
    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &configuration);
    void ignoreSslErrors(const QList<QSslError> &errors);
#endif

public Q_SLOTS:
    virtual void abort() = 0;
    virtual void ignoreSslErrors();

Q_SIGNALS:
    void metaDataChanged();
    void finished();
#if QT_DEPRECATED_SINCE(5,15)
    QT_DEPRECATED_NETWORK_API_5_15_X("Use QNetworkReply::errorOccurred(QNetworkReply::NetworkError) instead")
    void error(QNetworkReply::NetworkError);
#endif
    void errorOccurred(QNetworkReply::NetworkError);
#if QT_CONFIG(ssl)
    void encrypted();
    void sslErrors(const QList<QSslError> &errors);
    void preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *authenticator);
#endif
    void redirected(const QUrl &url);
    void redirectAllowed();

    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

protected:
    explicit QNetworkReply(QObject *parent = nullptr);
    QNetworkReply(QNetworkReplyPrivate &dd, QObject *parent);
    virtual qint64 writeData(const char *data, qint64 len) override;

    void setOperation(QNetworkAccessManager::Operation operation);
    void setRequest(const QNetworkRequest &request);
    void setError(NetworkError errorCode, const QString &errorString);
    void setFinished(bool);
    void setUrl(const QUrl &url);
    void setHeader(QNetworkRequest::KnownHeaders header, const QVariant &value);
    void setRawHeader(const QByteArray &headerName, const QByteArray &value);
    void setAttribute(QNetworkRequest::Attribute code, const QVariant &value);

#if QT_CONFIG(ssl)
    virtual void sslConfigurationImplementation(QSslConfiguration &) const;
    virtual void setSslConfigurationImplementation(const QSslConfiguration &);
    virtual void ignoreSslErrorsImplementation(const QList<QSslError> &);
#endif

private:
    Q_DECLARE_PRIVATE(QNetworkReply)
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QNetworkReply::NetworkError)

#endif
