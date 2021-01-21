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

#ifndef QNETWORKREQUEST_H
#define QNETWORKREQUEST_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QSslConfiguration;
class QHttp2Configuration;

class QNetworkRequestPrivate;
class Q_NETWORK_EXPORT QNetworkRequest
{
public:
    enum KnownHeaders {
        ContentTypeHeader,
        ContentLengthHeader,
        LocationHeader,
        LastModifiedHeader,
        CookieHeader,
        SetCookieHeader,
        ContentDispositionHeader,  // added for QMultipartMessage
        UserAgentHeader,
        ServerHeader,
        IfModifiedSinceHeader,
        ETagHeader,
        IfMatchHeader,
        IfNoneMatchHeader
    };
    enum Attribute {
        HttpStatusCodeAttribute,
        HttpReasonPhraseAttribute,
        RedirectionTargetAttribute,
        ConnectionEncryptedAttribute,
        CacheLoadControlAttribute,
        CacheSaveControlAttribute,
        SourceIsFromCacheAttribute,
        DoNotBufferUploadDataAttribute,
        HttpPipeliningAllowedAttribute,
        HttpPipeliningWasUsedAttribute,
        CustomVerbAttribute,
        CookieLoadControlAttribute,
        AuthenticationReuseAttribute,
        CookieSaveControlAttribute,
        MaximumDownloadBufferSizeAttribute, // internal
        DownloadBufferAttribute, // internal
        SynchronousRequestAttribute, // internal
        BackgroundRequestAttribute,
#if QT_DEPRECATED_SINCE(5, 15)
        SpdyAllowedAttribute,
        SpdyWasUsedAttribute,
#endif // QT_DEPRECATED_SINCE(5, 15)
        EmitAllUploadProgressSignalsAttribute = BackgroundRequestAttribute + 3,
        FollowRedirectsAttribute Q_DECL_ENUMERATOR_DEPRECATED_X("Use RedirectPolicyAttribute"),
        Http2AllowedAttribute,
        Http2WasUsedAttribute,
#if QT_DEPRECATED_SINCE(5, 15)
        HTTP2AllowedAttribute Q_DECL_ENUMERATOR_DEPRECATED_X("Use Http2AllowedAttribute") = Http2AllowedAttribute,
        HTTP2WasUsedAttribute Q_DECL_ENUMERATOR_DEPRECATED_X("Use Http2WasUsedAttribute"),
#endif // QT_DEPRECATED_SINCE(5, 15)
        OriginalContentLengthAttribute,
        RedirectPolicyAttribute,
        Http2DirectAttribute,
        ResourceTypeAttribute, // internal
        AutoDeleteReplyOnFinishAttribute,

        User = 1000,
        UserMax = 32767
    };
    enum CacheLoadControl {
        AlwaysNetwork,
        PreferNetwork,
        PreferCache,
        AlwaysCache
    };
    enum LoadControl {
        Automatic = 0,
        Manual
    };

    enum Priority {
        HighPriority = 1,
        NormalPriority = 3,
        LowPriority = 5
    };

    enum RedirectPolicy {
        ManualRedirectPolicy,
        NoLessSafeRedirectPolicy,
        SameOriginRedirectPolicy,
        UserVerifiedRedirectPolicy
    };

    enum TransferTimeoutConstant {
        DefaultTransferTimeoutConstant = 30000
    };

    QNetworkRequest();
    explicit QNetworkRequest(const QUrl &url);
    QNetworkRequest(const QNetworkRequest &other);
    ~QNetworkRequest();
    QNetworkRequest &operator=(QNetworkRequest &&other) noexcept { swap(other); return *this; }
    QNetworkRequest &operator=(const QNetworkRequest &other);

    void swap(QNetworkRequest &other) noexcept { qSwap(d, other.d); }

    bool operator==(const QNetworkRequest &other) const;
    inline bool operator!=(const QNetworkRequest &other) const
    { return !operator==(other); }

    QUrl url() const;
    void setUrl(const QUrl &url);

    // "cooked" headers
    QVariant header(KnownHeaders header) const;
    void setHeader(KnownHeaders header, const QVariant &value);

    // raw headers:
    bool hasRawHeader(const QByteArray &headerName) const;
    QList<QByteArray> rawHeaderList() const;
    QByteArray rawHeader(const QByteArray &headerName) const;
    void setRawHeader(const QByteArray &headerName, const QByteArray &value);

    // attributes
    QVariant attribute(Attribute code, const QVariant &defaultValue = QVariant()) const;
    void setAttribute(Attribute code, const QVariant &value);

#ifndef QT_NO_SSL
    QSslConfiguration sslConfiguration() const;
    void setSslConfiguration(const QSslConfiguration &configuration);
#endif

    void setOriginatingObject(QObject *object);
    QObject *originatingObject() const;

    Priority priority() const;
    void setPriority(Priority priority);

    // HTTP redirect related
    int maximumRedirectsAllowed() const;
    void setMaximumRedirectsAllowed(int maximumRedirectsAllowed);

    QString peerVerifyName() const;
    void setPeerVerifyName(const QString &peerName);
#if QT_CONFIG(http) || defined(Q_CLANG_QDOC)
    QHttp2Configuration http2Configuration() const;
    void setHttp2Configuration(const QHttp2Configuration &configuration);
#endif // QT_CONFIG(http) || defined(Q_CLANG_QDOC)
#if QT_CONFIG(http) || defined(Q_CLANG_QDOC) || defined (Q_OS_WASM)
    int transferTimeout() const;
    void setTransferTimeout(int timeout = DefaultTransferTimeoutConstant);
#endif // QT_CONFIG(http) || defined(Q_CLANG_QDOC) || defined (Q_OS_WASM)
private:
    QSharedDataPointer<QNetworkRequestPrivate> d;
    friend class QNetworkRequestPrivate;
};

Q_DECLARE_SHARED(QNetworkRequest)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QNetworkRequest)
Q_DECLARE_METATYPE(QNetworkRequest::RedirectPolicy)

#endif
