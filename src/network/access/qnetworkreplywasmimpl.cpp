/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnetworkreplywasmimpl_p.h"
#include "qnetworkrequest.h"

#include <QtCore/qtimer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qthread.h>

#include <private/qnetworkaccessmanager_p.h>
#include <private/qnetworkfile_p.h>

#include <emscripten.h>
#include <emscripten/fetch.h>

QT_BEGIN_NAMESPACE

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
    if (m_fetch) {
        emscripten_fetch_close(m_fetch);
        m_fetch = 0;
    }
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
    QNetworkReply::close();
    setFinished(true);
    emit finished();
}

void QNetworkReplyWasmImpl::abort()
{
    Q_D( QNetworkReplyWasmImpl);
    if (d->state == QNetworkReplyPrivate::Finished || d->state == QNetworkReplyPrivate::Aborted)
        return;

    d->state = QNetworkReplyPrivate::Aborted;
    d->doAbort();
    close();
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

void QNetworkReplyWasmImplPrivate::doAbort() const
{
    emscripten_fetch_close(m_fetch);
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
    const char* customHeaders[arrayLength];

    if (headersData.count() > 0) {
        int i = 0;
        for (int j = 0; j < headersData.count(); j++) {
            customHeaders[i] = headersData[j].constData();
            i += 1;
            customHeaders[i] = request.rawHeader(headersData[j]).constData();
            i += 1;
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

    // username & password
    if (!request.url().userInfo().isEmpty()) {
        attr.userName = request.url().userName().toUtf8();
        attr.password = request.url().password().toUtf8();
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

    attr.onsuccess = QNetworkReplyWasmImplPrivate::downloadSucceeded;
    attr.onerror = QNetworkReplyWasmImplPrivate::downloadFailed;
    attr.onprogress = QNetworkReplyWasmImplPrivate::downloadProgress;
    attr.onreadystatechange = QNetworkReplyWasmImplPrivate::stateChange;
    attr.timeoutMSecs = request.transferTimeout();
    attr.userData = reinterpret_cast<void *>(this);

    QString dPath = QStringLiteral("/home/web_user/") + request.url().fileName();
    attr.destinationPath = dPath.toUtf8();

    m_fetch = emscripten_fetch(&attr, request.url().toString().toUtf8());
}

void QNetworkReplyWasmImplPrivate::emitReplyError(QNetworkReply::NetworkError errorCode, const QString &errorString)
{
    Q_Q(QNetworkReplyWasmImpl);

    q->setError(errorCode, errorString);
    emit q->errorOccurred(errorCode);
    emit q->finished();
}

void QNetworkReplyWasmImplPrivate::emitDataReadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_Q(QNetworkReplyWasmImpl);

    totalDownloadSize = bytesTotal;

    percentFinished = (bytesReceived / bytesTotal) * 100;

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

    switch (tolower(headerName.at(0))) {
    case 'c':
        if (qstricmp(headerName.constData(), "content-type") == 0)
            return QNetworkRequest::ContentTypeHeader;
        else if (qstricmp(headerName.constData(), "content-length") == 0)
            return QNetworkRequest::ContentLengthHeader;
        else if (qstricmp(headerName.constData(), "cookie") == 0)
            return QNetworkRequest::CookieHeader;
        break;

    case 'l':
        if (qstricmp(headerName.constData(), "location") == 0)
            return QNetworkRequest::LocationHeader;
        else if (qstricmp(headerName.constData(), "last-modified") == 0)
            return QNetworkRequest::LastModifiedHeader;
        break;

    case 's':
        if (qstricmp(headerName.constData(), "set-cookie") == 0)
            return QNetworkRequest::SetCookieHeader;
        else if (qstricmp(headerName.constData(), "server") == 0)
            return QNetworkRequest::ServerHeader;
        break;

    case 'u':
        if (qstricmp(headerName.constData(), "user-agent") == 0)
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
        outgoingDataBuffer = QSharedPointer<QRingBuffer>::create();

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
    QNetworkReplyWasmImplPrivate *reply =
            reinterpret_cast<QNetworkReplyWasmImplPrivate*>(fetch->userData);
    if (reply) {
        QByteArray buffer(fetch->data, fetch->numBytes);
        reply->dataReceived(buffer, buffer.size());

        QByteArray statusText(fetch->statusText);
        reply->setStatusCode(fetch->status, statusText);
        reply->setReplyFinished();
    }
}

void QNetworkReplyWasmImplPrivate::setStatusCode(int status, const QByteArray &statusText)
{
    Q_Q(QNetworkReplyWasmImpl);
    q->setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
    q->setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, statusText);
}

void QNetworkReplyWasmImplPrivate::setReplyFinished()
{
    Q_Q(QNetworkReplyWasmImpl);
    q->setFinished(true);
    emit q->readChannelFinished();
    emit q->finished();
}

void QNetworkReplyWasmImplPrivate::stateChange(emscripten_fetch_t *fetch)
{
    if (fetch->readyState == /*HEADERS_RECEIVED*/ 2) {
        size_t headerLength = emscripten_fetch_get_response_headers_length(fetch);
        QByteArray str(headerLength, Qt::Uninitialized);
        emscripten_fetch_get_response_headers(fetch, str.data(), str.size());
        QNetworkReplyWasmImplPrivate *reply =
                reinterpret_cast<QNetworkReplyWasmImplPrivate*>(fetch->userData);
        reply->headersReceived(str);
    }
}

void QNetworkReplyWasmImplPrivate::downloadProgress(emscripten_fetch_t *fetch)
{
    QNetworkReplyWasmImplPrivate *reply =
            reinterpret_cast<QNetworkReplyWasmImplPrivate*>(fetch->userData);
    Q_ASSERT(reply);

    if (fetch->status < 400) {
        uint64_t bytes = fetch->dataOffset + fetch->numBytes;
        uint64_t tBytes = fetch->totalBytes; // totalBytes can be 0 if server not reporting content length
        if (tBytes == 0)
            tBytes = bytes;
        reply->emitDataReadProgress(bytes, tBytes);
    }
}

void QNetworkReplyWasmImplPrivate::downloadFailed(emscripten_fetch_t *fetch)
{
    QNetworkReplyWasmImplPrivate *reply = reinterpret_cast<QNetworkReplyWasmImplPrivate*>(fetch->userData);
    if (reply) {
        QString reasonStr;
        if (fetch->status > 600 ||  reply->state == QNetworkReplyPrivate::Aborted)
            reasonStr = QStringLiteral("Operation canceled");
        else
            reasonStr = QString::fromUtf8(fetch->statusText);

        QByteArray statusText(fetch->statusText);
        reply->setStatusCode(fetch->status, statusText);
        reply->emitReplyError(reply->statusCodeFromHttp(fetch->status, reply->request.url()), reasonStr);
    }

    if (fetch->status >= 400)
        emscripten_fetch_close(fetch); // Also free data on failure.
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
