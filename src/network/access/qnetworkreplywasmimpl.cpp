// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetworkreplywasmimpl_p.h"
#include "qnetworkrequest.h"

#include <QtCore/qtimer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qthread.h>
#include <QtCore/private/qoffsetstringarray_p.h>
#include <QtCore/private/qtools_p.h>

#include <private/qnetworkaccessmanager_p.h>
#include <private/qnetworkfile_p.h>

#include <emscripten.h>
#include <emscripten/fetch.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {

static constexpr auto BannedHeaders = qOffsetStringArray(
    "accept-charset",
    "accept-encoding",
    "access-control-request-headers",
    "access-control-request-method",
    "connection",
    "content-length",
    "cookie",
    "cookie2",
    "date",
    "dnt",
    "expect",
    "host",
    "keep-alive",
    "origin",
    "referer",
    "te",
    "trailer",
    "transfer-encoding",
    "upgrade",
    "via"
);

bool isUnsafeHeader(QLatin1StringView header) noexcept
{
    return header.startsWith("proxy-"_L1, Qt::CaseInsensitive)
        || header.startsWith("sec-"_L1, Qt::CaseInsensitive)
        || BannedHeaders.contains(header, Qt::CaseInsensitive);
}
} // namespace

QNetworkReplyWasmImplPrivate::QNetworkReplyWasmImplPrivate()
    : QNetworkReplyPrivate()
    , managerPrivate(0)
    , downloadBufferReadPosition(0)
    , downloadBufferCurrentSize(0)
    , totalDownloadSize(0)
    , percentFinished(0)
    , m_fetch(0)
{
}

QNetworkReplyWasmImplPrivate::~QNetworkReplyWasmImplPrivate()
{
}

QNetworkReplyWasmImpl::QNetworkReplyWasmImpl(QObject *parent)
    : QNetworkReply(*new QNetworkReplyWasmImplPrivate(), parent)
{
    Q_D( QNetworkReplyWasmImpl);
    d->state = QNetworkReplyPrivate::Idle;
}

QNetworkReplyWasmImpl::~QNetworkReplyWasmImpl()
{
}

QByteArray QNetworkReplyWasmImpl::methodName() const
{
    const Q_D( QNetworkReplyWasmImpl);
    switch (operation()) {
    case QNetworkAccessManager::HeadOperation:
        return "HEAD";
    case QNetworkAccessManager::GetOperation:
        return "GET";
    case QNetworkAccessManager::PutOperation:
        return "PUT";
    case QNetworkAccessManager::PostOperation:
        return "POST";
    case QNetworkAccessManager::DeleteOperation:
        return "DELETE";
    case QNetworkAccessManager::CustomOperation:
        return d->request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray();
    default:
        break;
    }
    return QByteArray();
}

void QNetworkReplyWasmImpl::close()
{
    Q_D(QNetworkReplyWasmImpl);

    if (d->state != QNetworkReplyPrivate::Aborted &&
        d->state != QNetworkReplyPrivate::Finished &&
        d->state != QNetworkReplyPrivate::Idle) {
            d->state = QNetworkReplyPrivate::Finished;
            d->setCanceled();
    }

    QNetworkReply::close();
}

void QNetworkReplyWasmImpl::abort()
{
    Q_D(QNetworkReplyWasmImpl);

    if (d->state == QNetworkReplyPrivate::Finished || d->state == QNetworkReplyPrivate::Aborted)
        return;

    d->state = QNetworkReplyPrivate::Aborted;
    d->setCanceled();
}

void QNetworkReplyWasmImplPrivate::setCanceled()
{
    Q_Q(QNetworkReplyWasmImpl);
    m_fetch->userData = nullptr;

    emitReplyError(QNetworkReply::OperationCanceledError, QStringLiteral("Operation canceled"));
    q->setFinished(true);
    emit q->finished();
}

qint64 QNetworkReplyWasmImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyWasmImpl);

    return QNetworkReply::bytesAvailable() + d->downloadBufferCurrentSize - d->downloadBufferReadPosition;
}

bool QNetworkReplyWasmImpl::isSequential() const
{
    return true;
}

qint64 QNetworkReplyWasmImpl::size() const
{
    return QNetworkReply::size();
}

/*!
    \internal
*/
qint64 QNetworkReplyWasmImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyWasmImpl);

    qint64 howMuch = qMin(maxlen, (d->downloadBuffer.size() - d->downloadBufferReadPosition));
    memcpy(data, d->downloadBuffer.constData() + d->downloadBufferReadPosition, howMuch);
    d->downloadBufferReadPosition += howMuch;

    return howMuch;
}

void QNetworkReplyWasmImplPrivate::setup(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *data)
{
    Q_Q(QNetworkReplyWasmImpl);

    outgoingData = data;
    request = req;
    url = request.url();
    operation = op;

    q->QIODevice::open(QIODevice::ReadOnly);
    if (outgoingData && outgoingData->isSequential()) {
        bool bufferingDisallowed =
            request.attribute(QNetworkRequest::DoNotBufferUploadDataAttribute, false).toBool();

        if (bufferingDisallowed) {
            // if a valid content-length header for the request was supplied, we can disable buffering
            // if not, we will buffer anyway
            if (!request.header(QNetworkRequest::ContentLengthHeader).isValid()) {
                state = Buffering;
                _q_bufferOutgoingData();
                return;
            }
        } else {
            // doSendRequest will be called when the buffering has finished.
            state = Buffering;
            _q_bufferOutgoingData();
            return;
        }
    }
    // No outgoing data (POST, ..)
    doSendRequest();
}

void QNetworkReplyWasmImplPrivate::setReplyAttributes(quintptr data, int statusCode, const QString &statusReason)
{
    QNetworkReplyWasmImplPrivate *handler = reinterpret_cast<QNetworkReplyWasmImplPrivate*>(data);
    Q_ASSERT(handler);

    handler->q_func()->setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
    if (!statusReason.isEmpty())
        handler->q_func()->setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, statusReason);
}

constexpr int getArraySize (int factor) {
    return 2 * factor + 1;
}

void QNetworkReplyWasmImplPrivate::doSendRequest()
{
    Q_Q(QNetworkReplyWasmImpl);
    totalDownloadSize = 0;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, q->methodName().constData());

    QList<QByteArray> headersData = request.rawHeaderList();
    int arrayLength = getArraySize(headersData.count());
    const char *customHeaders[arrayLength];
    QStringList trimmedHeaders;

    if (headersData.count() > 0) {
        int i = 0;
        for (const auto &headerName : headersData) {
            if (isUnsafeHeader(QLatin1StringView(headerName.constData()))) {
                trimmedHeaders.push_back(QString::fromLatin1(headerName));
            } else {
                customHeaders[i++] = headerName.constData();
                customHeaders[i++] = request.rawHeader(headerName).constData();
            }
        }
        if (!trimmedHeaders.isEmpty()) {
            qWarning() << "Qt has trimmed the following forbidden headers from the request:"
                       << trimmedHeaders.join(QLatin1StringView(", "));
        }
        customHeaders[i] = nullptr;
        attr.requestHeaders = customHeaders;
    }

    if (outgoingData) { // data from post request
        // handle extra data
        requestData = outgoingData->readAll(); // is there a size restriction here?
        if (!requestData.isEmpty()) {
            attr.requestData = requestData.data();
            attr.requestDataSize = requestData.size();
        }
    }

    QByteArray userName, password;
    // username & password
    if (!request.url().userInfo().isEmpty()) {
        userName = request.url().userName().toUtf8();
        password = request.url().password().toUtf8();
        attr.userName = userName.constData();
        attr.password = password.constData();
    }

    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    QNetworkRequest::CacheLoadControl CacheLoadControlAttribute =
        (QNetworkRequest::CacheLoadControl)request.attribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork).toInt();

    if (CacheLoadControlAttribute == QNetworkRequest::AlwaysCache) {
        attr.attributes += EMSCRIPTEN_FETCH_NO_DOWNLOAD;
    }
    if (CacheLoadControlAttribute == QNetworkRequest::PreferCache) {
         attr.attributes += EMSCRIPTEN_FETCH_APPEND;
    }

    if (CacheLoadControlAttribute == QNetworkRequest::AlwaysNetwork ||
            request.attribute(QNetworkRequest::CacheSaveControlAttribute, false).toBool()) {
        attr.attributes -= EMSCRIPTEN_FETCH_PERSIST_FILE;
    }

    attr.withCredentials = request.attribute(QNetworkRequest::UseCredentialsAttribute, false).toBool();
    attr.onsuccess = QNetworkReplyWasmImplPrivate::downloadSucceeded;
    attr.onerror = QNetworkReplyWasmImplPrivate::downloadFailed;
    attr.onprogress = QNetworkReplyWasmImplPrivate::downloadProgress;
    attr.onreadystatechange = QNetworkReplyWasmImplPrivate::stateChange;
    attr.timeoutMSecs = request.transferTimeout();
    attr.userData = reinterpret_cast<void *>(this);

    QString dPath = QStringLiteral("/home/web_user/") + request.url().fileName();
    QByteArray destinationPath = dPath.toUtf8();
    attr.destinationPath = destinationPath.constData();

    m_fetch = emscripten_fetch(&attr, request.url().toString().toUtf8());
    state = Working;
}

void QNetworkReplyWasmImplPrivate::emitReplyError(QNetworkReply::NetworkError errorCode, const QString &errorString)
{
    Q_Q(QNetworkReplyWasmImpl);

    q->setError(errorCode, errorString);
    emit q->errorOccurred(errorCode);
}

void QNetworkReplyWasmImplPrivate::emitDataReadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_Q(QNetworkReplyWasmImpl);

    totalDownloadSize = bytesTotal;

    percentFinished = bytesTotal ? (bytesReceived / bytesTotal) * 100 : 100;

    emit q->downloadProgress(bytesReceived, bytesTotal);
}

void QNetworkReplyWasmImplPrivate::dataReceived(const QByteArray &buffer, int bufferSize)
{
    Q_Q(QNetworkReplyWasmImpl);

    if (bufferSize > 0)
        q->setReadBufferSize(bufferSize);

    bytesDownloaded = bufferSize;

    if (percentFinished != 100)
        downloadBufferCurrentSize += bufferSize;
    else
        downloadBufferCurrentSize = bufferSize;

    totalDownloadSize = downloadBufferCurrentSize;

    downloadBuffer.append(buffer, bufferSize);

    emit q->readyRead();
}

//taken from qnetworkrequest.cpp
static int parseHeaderName(const QByteArray &headerName)
{
    if (headerName.isEmpty())
        return -1;

    auto is = [&](const char *what) {
        return qstrnicmp(headerName.data(), headerName.size(), what) == 0;
    };

    switch (QtMiscUtils::toAsciiLower(headerName.front())) {
    case 'c':
        if (is("content-type"))
            return QNetworkRequest::ContentTypeHeader;
        else if (is("content-length"))
            return QNetworkRequest::ContentLengthHeader;
        else if (is("cookie"))
            return QNetworkRequest::CookieHeader;
        break;

    case 'l':
        if (is("location"))
            return QNetworkRequest::LocationHeader;
        else if (is("last-modified"))
            return QNetworkRequest::LastModifiedHeader;
        break;

    case 's':
        if (is("set-cookie"))
            return QNetworkRequest::SetCookieHeader;
        else if (is("server"))
            return QNetworkRequest::ServerHeader;
        break;

    case 'u':
        if (is("user-agent"))
            return QNetworkRequest::UserAgentHeader;
        break;
    }

    return -1; // nothing found
}


void QNetworkReplyWasmImplPrivate::headersReceived(const QByteArray &buffer)
{
    Q_Q(QNetworkReplyWasmImpl);

    if (!buffer.isEmpty()) {
        QList<QByteArray> headers = buffer.split('\n');

        for (int i = 0; i < headers.size(); i++) {
            if (headers.at(i).contains(':')) { // headers include final \x00, so skip
                QByteArray headerName = headers.at(i).split(':').at(0).trimmed();
                QByteArray headersValue = headers.at(i).split(':').at(1).trimmed();

                if (headerName.isEmpty() || headersValue.isEmpty())
                    continue;

                int headerIndex = parseHeaderName(headerName);

                if (headerIndex == -1)
                    q->setRawHeader(headerName, headersValue);
                else
                    q->setHeader(static_cast<QNetworkRequest::KnownHeaders>(headerIndex), (QVariant)headersValue);
            }
        }
    }
    emit q->metaDataChanged();
}

void QNetworkReplyWasmImplPrivate::_q_bufferOutgoingDataFinished()
{
    Q_Q(QNetworkReplyWasmImpl);

    // make sure this is only called once, ever.
    //_q_bufferOutgoingData may call it or the readChannelFinished emission
    if (state != Buffering)
        return;

    // disconnect signals
    QObject::disconnect(outgoingData, SIGNAL(readyRead()), q, SLOT(_q_bufferOutgoingData()));
    QObject::disconnect(outgoingData, SIGNAL(readChannelFinished()), q, SLOT(_q_bufferOutgoingDataFinished()));

    // finally, start the request
    doSendRequest();
}

void QNetworkReplyWasmImplPrivate::_q_bufferOutgoingData()
{
    Q_Q(QNetworkReplyWasmImpl);

    if (!outgoingDataBuffer) {
        // first call, create our buffer
        outgoingDataBuffer = std::make_shared<QRingBuffer>();

        QObject::connect(outgoingData, SIGNAL(readyRead()), q, SLOT(_q_bufferOutgoingData()));
        QObject::connect(outgoingData, SIGNAL(readChannelFinished()), q, SLOT(_q_bufferOutgoingDataFinished()));
    }

    qint64 bytesBuffered = 0;
    qint64 bytesToBuffer = 0;

    // read data into our buffer
    forever {
        bytesToBuffer = outgoingData->bytesAvailable();
        // unknown? just try 2 kB, this also ensures we always try to read the EOF
        if (bytesToBuffer <= 0)
            bytesToBuffer = 2*1024;

        char *dst = outgoingDataBuffer->reserve(bytesToBuffer);
        bytesBuffered = outgoingData->read(dst, bytesToBuffer);

        if (bytesBuffered == -1) {
            // EOF has been reached.
            outgoingDataBuffer->chop(bytesToBuffer);

            _q_bufferOutgoingDataFinished();
            break;
        } else if (bytesBuffered == 0) {
            // nothing read right now, just wait until we get called again
            outgoingDataBuffer->chop(bytesToBuffer);

            break;
        } else {
            // don't break, try to read() again
            outgoingDataBuffer->chop(bytesToBuffer - bytesBuffered);
        }
    }
}

void QNetworkReplyWasmImplPrivate::downloadSucceeded(emscripten_fetch_t *fetch)
{
    auto reply = reinterpret_cast<QNetworkReplyWasmImplPrivate*>(fetch->userData);
    if (reply) {
        if (reply->state != QNetworkReplyPrivate::Aborted) {
            QByteArray buffer(fetch->data, fetch->numBytes);
            reply->dataReceived(buffer, buffer.size());
            QByteArray statusText(fetch->statusText);
            reply->setStatusCode(fetch->status, statusText);
            reply->setReplyFinished();
        }
        reply->m_fetch = nullptr;
    }
    emscripten_fetch_close(fetch);
}

void QNetworkReplyWasmImplPrivate::setReplyFinished()
{
    Q_Q(QNetworkReplyWasmImpl);
    q->setFinished(true);
    emit q->readChannelFinished();
    emit q->finished();
}

void QNetworkReplyWasmImplPrivate::setStatusCode(int status, const QByteArray &statusText)
{
    Q_Q(QNetworkReplyWasmImpl);
    q->setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
    q->setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, statusText);
}

void QNetworkReplyWasmImplPrivate::stateChange(emscripten_fetch_t *fetch)
{
    auto reply = reinterpret_cast<QNetworkReplyWasmImplPrivate*>(fetch->userData);
    if (reply && reply->state != QNetworkReplyPrivate::Aborted) {
        if (fetch->readyState == /*HEADERS_RECEIVED*/ 2) {
            size_t headerLength = emscripten_fetch_get_response_headers_length(fetch);
            QByteArray str(headerLength, Qt::Uninitialized);
            emscripten_fetch_get_response_headers(fetch, str.data(), str.size());
            reply->headersReceived(str);
        }
    }
}

void QNetworkReplyWasmImplPrivate::downloadProgress(emscripten_fetch_t *fetch)
{
    auto reply = reinterpret_cast<QNetworkReplyWasmImplPrivate*>(fetch->userData);
    if (reply && reply->state != QNetworkReplyPrivate::Aborted) {
        if (fetch->status < 400) {
            uint64_t bytes = fetch->dataOffset + fetch->numBytes;
            uint64_t tBytes = fetch->totalBytes; // totalBytes can be 0 if server not reporting content length
            if (tBytes == 0)
                tBytes = bytes;
            reply->emitDataReadProgress(bytes, tBytes);
        }
    }
}

void QNetworkReplyWasmImplPrivate::downloadFailed(emscripten_fetch_t *fetch)
{
    auto reply = reinterpret_cast<QNetworkReplyWasmImplPrivate*>(fetch->userData);
    if (reply) {
        if (reply->state != QNetworkReplyPrivate::Aborted) {
            QString reasonStr;
            if (fetch->status > 600)
                reasonStr = QStringLiteral("Operation canceled");
            else
                reasonStr = QString::fromUtf8(fetch->statusText);
            QByteArray buffer(fetch->data, fetch->numBytes);
            reply->dataReceived(buffer, buffer.size());
            QByteArray statusText(fetch->statusText);
            reply->setStatusCode(fetch->status, statusText);
            reply->emitReplyError(reply->statusCodeFromHttp(fetch->status, reply->request.url()), reasonStr);
            reply->setReplyFinished();
        }
        reply->m_fetch = nullptr;
    }
    emscripten_fetch_close(fetch);
}

//taken from qhttpthreaddelegate.cpp
QNetworkReply::NetworkError QNetworkReplyWasmImplPrivate::statusCodeFromHttp(int httpStatusCode, const QUrl &url)
{
    QNetworkReply::NetworkError code;
    // we've got an error
    switch (httpStatusCode) {
    case 400:               // Bad Request
        code = QNetworkReply::ProtocolInvalidOperationError;
        break;

    case 401:               // Authorization required
        code = QNetworkReply::AuthenticationRequiredError;
        break;

    case 403:               // Access denied
        code = QNetworkReply::ContentAccessDenied;
        break;

    case 404:               // Not Found
        code = QNetworkReply::ContentNotFoundError;
        break;

    case 405:               // Method Not Allowed
        code = QNetworkReply::ContentOperationNotPermittedError;
        break;

    case 407:
        code = QNetworkReply::ProxyAuthenticationRequiredError;
        break;

    case 409:               // Resource Conflict
        code = QNetworkReply::ContentConflictError;
        break;

    case 410:               // Content no longer available
        code = QNetworkReply::ContentGoneError;
        break;

    case 418:               // I'm a teapot
        code = QNetworkReply::ProtocolInvalidOperationError;
        break;

    case 500:               // Internal Server Error
        code = QNetworkReply::InternalServerError;
        break;

    case 501:               // Server does not support this functionality
        code = QNetworkReply::OperationNotImplementedError;
        break;

    case 503:               // Service unavailable
        code = QNetworkReply::ServiceUnavailableError;
        break;

    case 65535: //emscripten reply when aborted
        code =  QNetworkReply::OperationCanceledError;
        break;
    default:
        if (httpStatusCode > 500) {
            // some kind of server error
            code = QNetworkReply::UnknownServerError;
        } else if (httpStatusCode >= 400) {
            // content error we did not handle above
            code = QNetworkReply::UnknownContentError;
        } else {
            qWarning("QNetworkAccess: got HTTP status code %d which is not expected from url: \"%s\"",
                     httpStatusCode, qPrintable(url.toString()));
            code = QNetworkReply::ProtocolFailure;
        }
    };

    return code;
}

QT_END_NAMESPACE

#include "moc_qnetworkreplywasmimpl_p.cpp"
