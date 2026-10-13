// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qhttpnetworkrequest_p.h"
#include "private/qnoncontiguousbytedevice_p.h"

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QHttpNetworkRequest)

QHttpNetworkRequestPrivate::QHttpNetworkRequestPrivate(QHttpNetworkRequest::Operation op,
        QHttpNetworkRequest::Priority pri, const QUrl &newUrl)
    : QHttpNetworkHeaderPrivate(newUrl), operation(op), priority(pri), uploadByteDevice(nullptr),
      autoDecompress(false), pipeliningAllowed(false), http2Allowed(true),
      http2Direct(false), withCredentials(true), preConnect(false), redirectCount(0),
      redirectPolicy(QNetworkRequest::ManualRedirectPolicy)
{
}

QHttpNetworkRequestPrivate::QHttpNetworkRequestPrivate(const QHttpNetworkRequestPrivate &other) // = default
    : QHttpNetworkHeaderPrivate(other),
      operation(other.operation),
      customVerb(other.customVerb),
      fullLocalServerName(other.fullLocalServerName),
      priority(other.priority),
      uploadByteDevice(other.uploadByteDevice),
      autoDecompress(other.autoDecompress),
      pipeliningAllowed(other.pipeliningAllowed),
      http2Allowed(other.http2Allowed),
      http2Direct(other.http2Direct),
      h2cAllowed(other.h2cAllowed),
      withCredentials(other.withCredentials),
      ssl(other.ssl),
      preConnect(other.preConnect),
      needResendWithCredentials(other.needResendWithCredentials),
      redirectCount(other.redirectCount),
      redirectPolicy(other.redirectPolicy),
      peerVerifyName(other.peerVerifyName)
{
}

QHttpNetworkRequestPrivate::~QHttpNetworkRequestPrivate()
{
}

bool QHttpNetworkRequestPrivate::operator==(const QHttpNetworkRequestPrivate &other) const
{
    return QHttpNetworkHeaderPrivate::operator==(other)
        && (operation == other.operation)
        && (fullLocalServerName == other.fullLocalServerName)
        && (priority == other.priority)
        && (uploadByteDevice == other.uploadByteDevice)
        && (autoDecompress == other.autoDecompress)
        && (pipeliningAllowed == other.pipeliningAllowed)
        && (http2Allowed == other.http2Allowed)
        && (http2Direct == other.http2Direct)
        && (h2cAllowed == other.h2cAllowed)
        // we do not clear the customVerb in setOperation
        && (operation != QHttpNetworkRequest::Custom || (customVerb == other.customVerb))
        && (withCredentials == other.withCredentials)
        && (ssl == other.ssl)
        && (preConnect == other.preConnect)
        && (redirectPolicy == other.redirectPolicy)
        && (peerVerifyName == other.peerVerifyName)
        && (needResendWithCredentials == other.needResendWithCredentials)
        ;
}

QByteArray QHttpNetworkRequest::methodName() const
{
    switch (d->operation) {
    case QHttpNetworkRequest::Get:
        return "GET";
    case QHttpNetworkRequest::Head:
        return "HEAD";
    case QHttpNetworkRequest::Post:
        return "POST";
    case QHttpNetworkRequest::Options:
        return "OPTIONS";
    case QHttpNetworkRequest::Put:
        return "PUT";
    case QHttpNetworkRequest::Delete:
        return "DELETE";
    case QHttpNetworkRequest::Trace:
        return "TRACE";
    case QHttpNetworkRequest::Connect:
        return "CONNECT";
    case QHttpNetworkRequest::Custom:
        return d->customVerb;
    default:
        break;
    }
    return QByteArray();
}

QByteArray QHttpNetworkRequest::uri(bool throughProxy) const
{
    QUrl::FormattingOptions format(QUrl::RemoveFragment | QUrl::RemoveUserInfo | QUrl::FullyEncoded);

    // for POST, query data is sent as content
    if (d->operation == QHttpNetworkRequest::Post && !d->uploadByteDevice)
        format |= QUrl::RemoveQuery;
    // for requests through proxy, the Request-URI contains full url
    if (!throughProxy)
        format |= QUrl::RemoveScheme | QUrl::RemoveAuthority;
    QUrl copy = d->url;
    if (copy.path().isEmpty())
        copy.setPath(QStringLiteral("/"));
    else
        format |= QUrl::NormalizePathSegments;
    QByteArray uri = copy.toEncoded(format);
    return uri;
}

QByteArray QHttpNetworkRequestPrivate::header(const QHttpNetworkRequest &request, bool throughProxy)
{
    const QHttpHeaders headers = request.header();
    QByteArray ba;
    ba.reserve(40 + headers.size() * 25); // very rough lower bound estimation

    ba += request.methodName();
    ba += ' ';
    ba += request.uri(throughProxy);

    ba += " HTTP/";
    ba += QByteArray::number(request.majorVersion());
    ba += '.';
    ba += QByteArray::number(request.minorVersion());
    ba += "\r\n";

    for (qsizetype i = 0; i < headers.size(); ++i) {
        ba += headers.nameAt(i);
        ba += ": ";
        ba += headers.valueAt(i);
        ba += "\r\n";
    }
    if (request.d->operation == QHttpNetworkRequest::Post) {
        // add content type, if not set in the request
        if (request.headerField("content-type").isEmpty() && ((request.d->uploadByteDevice && request.d->uploadByteDevice->size() > 0) || request.d->url.hasQuery())) {
            //Content-Type is mandatory. We can't say anything about the encoding, but x-www-form-urlencoded is the most likely to work.
            //This warning indicates a bug in application code not setting a required header.
            //Note that if using QHttpMultipart, the content-type is set in QNetworkAccessManagerPrivate::prepareMultipart already
            qWarning("content-type missing in HTTP POST, defaulting to application/x-www-form-urlencoded. Use QNetworkRequest::setHeader() to fix this problem.");
            ba += "Content-Type: application/x-www-form-urlencoded\r\n";
        }
        if (!request.d->uploadByteDevice && request.d->url.hasQuery()) {
            QByteArray query = request.d->url.query(QUrl::FullyEncoded).toLatin1();
            ba += "Content-Length: ";
            ba += QByteArray::number(query.size());
            ba += "\r\n\r\n";
            ba += query;
        } else {
            ba += "\r\n";
        }
    } else {
        ba += "\r\n";
    }
     return ba;
}


// QHttpNetworkRequest

QHttpNetworkRequest::QHttpNetworkRequest(const QUrl &url, Operation operation, Priority priority)
    : d(new QHttpNetworkRequestPrivate(operation, priority, url))
{
}

QHttpNetworkRequest::QHttpNetworkRequest(const QHttpNetworkRequest &other)
    : QHttpNetworkHeader(other), d(other.d)
{
}

QHttpNetworkRequest::~QHttpNetworkRequest()
{
}

QUrl QHttpNetworkRequest::url() const
{
    return d->url;
}
void QHttpNetworkRequest::setUrl(const QUrl &url)
{
    d->url = url;
}

bool QHttpNetworkRequest::isSsl() const
{
    return d->ssl;
}
void QHttpNetworkRequest::setSsl(bool s)
{
    d->ssl = s;
}

bool QHttpNetworkRequest::isPreConnect() const
{
    return d->preConnect;
}
void QHttpNetworkRequest::setPreConnect(bool preConnect)
{
    d->preConnect = preConnect;
}

bool QHttpNetworkRequest::isFollowRedirects() const
{
    return d->redirectPolicy != QNetworkRequest::ManualRedirectPolicy;
}

void QHttpNetworkRequest::setRedirectPolicy(QNetworkRequest::RedirectPolicy policy)
{
    d->redirectPolicy = policy;
}

QNetworkRequest::RedirectPolicy QHttpNetworkRequest::redirectPolicy() const
{
    return d->redirectPolicy;
}

int QHttpNetworkRequest::redirectCount() const
{
    return d->redirectCount;
}

void QHttpNetworkRequest::setRedirectCount(int count)
{
    d->redirectCount = count;
}

qint64 QHttpNetworkRequest::contentLength() const
{
    return d->contentLength();
}

void QHttpNetworkRequest::setContentLength(qint64 length)
{
    d->setContentLength(length);
}

QHttpHeaders QHttpNetworkRequest::header() const
{
    return d->parser.headers();
}

QByteArray QHttpNetworkRequest::headerField(QByteArrayView name, const QByteArray &defaultValue) const
{
    return d->headerField(name, defaultValue);
}

void QHttpNetworkRequest::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    d->setHeaderField(name, data);
}

void QHttpNetworkRequest::prependHeaderField(const QByteArray &name, const QByteArray &data)
{
    d->prependHeaderField(name, data);
}

void QHttpNetworkRequest::clearHeaders()
{
    d->clearHeaders();
}

QHttpNetworkRequest &QHttpNetworkRequest::operator=(const QHttpNetworkRequest &other)
{
    QHttpNetworkHeader::operator=(other);
    d = other.d;
    return *this;
}

bool QHttpNetworkRequest::operator==(const QHttpNetworkRequest &other) const
{
    return d->operator==(*other.d);
}

QHttpNetworkRequest::Operation QHttpNetworkRequest::operation() const
{
    return d->operation;
}

void QHttpNetworkRequest::setOperation(Operation operation)
{
    d->operation = operation;
}

QByteArray QHttpNetworkRequest::customVerb() const
{
    return d->customVerb;
}

void QHttpNetworkRequest::setCustomVerb(const QByteArray &customVerb)
{
    d->customVerb = customVerb;
}

QHttpNetworkRequest::Priority QHttpNetworkRequest::priority() const
{
    return d->priority;
}

void QHttpNetworkRequest::setPriority(Priority priority)
{
    d->priority = priority;
}

bool QHttpNetworkRequest::isPipeliningAllowed() const
{
    return d->pipeliningAllowed;
}

void QHttpNetworkRequest::setPipeliningAllowed(bool b)
{
    d->pipeliningAllowed = b;
}

bool QHttpNetworkRequest::isHTTP2Allowed() const
{
    return d->http2Allowed;
}

void QHttpNetworkRequest::setHTTP2Allowed(bool b)
{
    d->http2Allowed = b;
}

bool QHttpNetworkRequest::isHTTP2Direct() const
{
    return d->http2Direct;
}

void QHttpNetworkRequest::setHTTP2Direct(bool b)
{
    d->http2Direct = b;
}

bool QHttpNetworkRequest::isH2cAllowed() const
{
    return d->h2cAllowed;
}

void QHttpNetworkRequest::setH2cAllowed(bool b)
{
    d->h2cAllowed = b;
}

bool QHttpNetworkRequest::withCredentials() const
{
    return d->withCredentials;
}

void QHttpNetworkRequest::setWithCredentials(bool b)
{
    d->withCredentials = b;
}

void QHttpNetworkRequest::setUploadByteDevice(QNonContiguousByteDevice *bd)
{
    d->uploadByteDevice = bd;
}

QNonContiguousByteDevice* QHttpNetworkRequest::uploadByteDevice() const
{
    return d->uploadByteDevice;
}

int QHttpNetworkRequest::majorVersion() const
{
    return 1;
}

int QHttpNetworkRequest::minorVersion() const
{
    return 1;
}

QString QHttpNetworkRequest::peerVerifyName() const
{
    return d->peerVerifyName;
}

void QHttpNetworkRequest::setPeerVerifyName(const QString &peerName)
{
    d->peerVerifyName = peerName;
}

QString QHttpNetworkRequest::fullLocalServerName() const
{
    return d->fullLocalServerName;
}

void QHttpNetworkRequest::setFullLocalServerName(const QString &fullServerName)
{
    d->fullLocalServerName = fullServerName;
}

bool QHttpNetworkRequest::methodIsIdempotent() const
{
    using Op = Operation;
    constexpr auto knownSafe = std::array{ Op::Get, Op::Head, Op::Put, Op::Trace, Op::Options };
    return std::any_of(knownSafe.begin(), knownSafe.end(),
                       [currentOp = d->operation](auto op) { return op == currentOp; });
}

QT_END_NAMESPACE

