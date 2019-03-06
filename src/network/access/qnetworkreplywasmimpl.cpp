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
#include <emscripten/bind.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

using namespace emscripten;

static void q_requestErrorCallback(val event)
{
    val xhr = event["target"];

    quintptr func = xhr["data-handler"].as<quintptr>();
    QNetworkReplyWasmImplPrivate *reply = reinterpret_cast<QNetworkReplyWasmImplPrivate*>(func);
    Q_ASSERT(reply);

    int statusCode = xhr["status"].as<int>();

    QString reasonStr = QString::fromStdString(xhr["statusText"].as<std::string>());

    reply->setReplyAttributes(func, statusCode, reasonStr);

    if (statusCode >= 400 && !reasonStr.isEmpty())
        reply->emitReplyError(reply->statusCodeFromHttp(statusCode, reply->request.url()), reasonStr);
}

static void q_progressCallback(val event)
{
    val xhr = event["target"];

    QNetworkReplyWasmImplPrivate *reply =
            reinterpret_cast<QNetworkReplyWasmImplPrivate*>(xhr["data-handler"].as<quintptr>());
    Q_ASSERT(reply);

    if (xhr["lengthComputable"].as<bool>() && xhr["status"].as<int>() < 400)
        reply->emitDataReadProgress(xhr["loaded"].as<qint64>(), xhr["total"].as<qint64>());

}

static void q_loadCallback(val event)
{
    val xhr = event["target"];

    QNetworkReplyWasmImplPrivate *reply =
            reinterpret_cast<QNetworkReplyWasmImplPrivate*>(xhr["data-handler"].as<quintptr>());
    Q_ASSERT(reply);

    int status = xhr["status"].as<int>();
    if (status >= 300) {
        q_requestErrorCallback(event);
        return;
    }
    QString statusText = QString::fromStdString(xhr["statusText"].as<std::string>());
    int readyState = xhr["readyState"].as<int>();

    if (status == 200 || status == 203) {
        QString responseString;
        const std::string responseType = xhr["responseType"].as<std::string>();
        if (responseType.length() == 0 || responseType == "document" || responseType == "text") {
            responseString = QString::fromStdWString(xhr["responseText"].as<std::wstring>());
        } else if (responseType == "json") {
            responseString =
                    QString::fromStdWString(val::global("JSON").call<std::wstring>("stringify", xhr["response"]));
        } else if (responseType == "arraybuffer" || responseType == "blob") {
            // handle this data in the FileReader, triggered by the call to readAsArrayBuffer
            val blob = xhr["response"];

            val reader = val::global("FileReader").new_();
            reader.set("onload", val::module_property("QNetworkReplyWasmImplPrivate_readBinary"));
            reader.set("data-handler", xhr["data-handler"]);

            reader.call<void>("readAsArrayBuffer", blob);
        }


        if (readyState == 4) { // done
            reply->setReplyAttributes(xhr["data-handler"].as<quintptr>(), status, statusText);
            if (!responseString.isEmpty())
                reply->dataReceived(responseString.toUtf8(), responseString.size());
        }
    }
    if (status >= 400 && !statusText.isEmpty())
        reply->emitReplyError(reply->statusCodeFromHttp(status, reply->request.url()), statusText);
}

static void q_responseHeadersCallback(val event)
{
    val xhr = event["target"];

    if (xhr["readyState"].as<int>() == 2) { // HEADERS_RECEIVED
        std::string responseHeaders = xhr.call<std::string>("getAllResponseHeaders");
        if (!responseHeaders.empty()) {
            QNetworkReplyWasmImplPrivate *reply =
                    reinterpret_cast<QNetworkReplyWasmImplPrivate*>(xhr["data-handler"].as<quintptr>());
            Q_ASSERT(reply);

            reply->headersReceived(QString::fromStdString(responseHeaders));
        }
    }
}

static void q_readBinary(val event)
{
    val fileReader = event["target"];

    QNetworkReplyWasmImplPrivate *reply =
            reinterpret_cast<QNetworkReplyWasmImplPrivate*>(fileReader["data-handler"].as<quintptr>());
    Q_ASSERT(reply);

    // Set up source typed array
    val result = fileReader["result"]; // ArrayBuffer
    val Uint8Array = val::global("Uint8Array");
    val sourceTypedArray = Uint8Array.new_(result);

    // Allocate and set up destination typed array
    const quintptr size = result["byteLength"].as<quintptr>();
    QByteArray buffer(size, Qt::Uninitialized);

    val destinationTypedArray = Uint8Array.new_(val::module_property("HEAPU8")["buffer"],
                                                            reinterpret_cast<quintptr>(buffer.data()), size);
    destinationTypedArray.call<void>("set", sourceTypedArray);
    reply->dataReceived(buffer, buffer.size());
    QCoreApplication::processEvents();
}

EMSCRIPTEN_BINDINGS(network_module) {
    function("QNetworkReplyWasmImplPrivate_requestErrorCallback", q_requestErrorCallback);
    function("QNetworkReplyWasmImplPrivate_progressCallback", q_progressCallback);
    function("QNetworkReplyWasmImplPrivate_loadCallback", q_loadCallback);
    function("QNetworkReplyWasmImplPrivate_responseHeadersCallback", q_responseHeadersCallback);
    function("QNetworkReplyWasmImplPrivate_readBinary", q_readBinary);
}

QNetworkReplyWasmImplPrivate::QNetworkReplyWasmImplPrivate()
    : QNetworkReplyPrivate()
    , managerPrivate(0)
    , downloadBufferReadPosition(0)
    , downloadBufferCurrentSize(0)
    , totalDownloadSize(0)
    , percentFinished(0)
{
}

QNetworkReplyWasmImplPrivate::~QNetworkReplyWasmImplPrivate()
{
}

QNetworkReplyWasmImpl::~QNetworkReplyWasmImpl()
{
}

QNetworkReplyWasmImpl::QNetworkReplyWasmImpl(QObject *parent)
    : QNetworkReply(*new QNetworkReplyWasmImplPrivate(), parent)
{
}

QByteArray QNetworkReplyWasmImpl::methodName() const
{
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
    default:
        break;
    }
    return QByteArray();
}

void QNetworkReplyWasmImpl::close()
{
    setFinished(true);
    emit finished();

    QNetworkReply::close();
}

void QNetworkReplyWasmImpl::abort()
{
    Q_D(const QNetworkReplyWasmImpl);
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
    m_xhr.call<void>("abort");
}

void QNetworkReplyWasmImplPrivate::doSendRequest()
{
    Q_Q(QNetworkReplyWasmImpl);
    totalDownloadSize = 0;

    m_xhr = val::global("XMLHttpRequest").new_();
    std::string verb = q->methodName().toStdString();

    QString extraDataString;

    m_xhr.call<void>("open", verb, request.url().toString().toStdString());

    m_xhr.set("onerror", val::module_property("QNetworkReplyWasmImplPrivate_requestErrorCallback"));
    m_xhr.set("onload", val::module_property("QNetworkReplyWasmImplPrivate_loadCallback"));
    m_xhr.set("onprogress", val::module_property("QNetworkReplyWasmImplPrivate_progressCallback"));
    m_xhr.set("onreadystatechange", val::module_property("QNetworkReplyWasmImplPrivate_responseHeadersCallback"));

    m_xhr.set("data-handler", val(quintptr(reinterpret_cast<void *>(this))));

    QByteArray contentType = request.rawHeader("Content-Type");

    // handle extra data
    val dataToSend = val::null();
    QByteArray extraData;

    if (outgoingData) // data from post request
        extraData = outgoingData->readAll();

    if (contentType.contains("text") ||
            contentType.contains("json") ||
            contentType.contains("form")) {
        if (extraData.size() > 0)
            extraDataString.fromUtf8(extraData);
    }
    if (contentType.contains("json")) {
        if (!extraDataString.isEmpty()) {
            m_xhr.set("responseType", val("json"));
            dataToSend = val(extraDataString.toStdString());
        }
    } else if (contentType.contains("form")) { //construct form data
        if (!extraDataString.isEmpty()) {
            val formData = val::global("FormData").new_();
            QStringList formList = extraDataString.split('&');

            for (auto formEntry : formList) {
                formData.call<void>("append", formEntry.split('=')[0].toStdString(), formEntry.split('=')[1].toStdString());
            }
            dataToSend = formData;
        }
    } else {
        m_xhr.set("responseType", val("blob"));
    }
    // set request headers
    for (auto header : request.rawHeaderList()) {
        m_xhr.call<void>("setRequestHeader", header.toStdString(), request.rawHeader(header).toStdString());
    }
     m_xhr.call<void>("send", dataToSend);
}

void QNetworkReplyWasmImplPrivate::emitReplyError(QNetworkReply::NetworkError errorCode, const QString &errorString)
{
    Q_UNUSED(errorCode)
    Q_Q(QNetworkReplyWasmImpl);

    q->setError(errorCode, errorString);
    emit q->error(errorCode);

    q->setFinished(true);
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

    if (downloadBufferCurrentSize == totalDownloadSize) {
        q->setFinished(true);
        emit q->readChannelFinished();
        emit q->finished();
    }
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


void QNetworkReplyWasmImplPrivate::headersReceived(const QString &bufferString)
{
    Q_Q(QNetworkReplyWasmImpl);

    if (!bufferString.isEmpty()) {
        QStringList headers = bufferString.split(QString::fromUtf8("\r\n"), QString::SkipEmptyParts);

        for (int i = 0; i < headers.size(); i++) {
            QString headerName = headers.at(i).split(QString::fromUtf8(": ")).at(0);
            QString headersValue = headers.at(i).split(QString::fromUtf8(": ")).at(1);
            if (headerName.isEmpty() || headersValue.isEmpty())
                continue;

            int headerIndex = parseHeaderName(headerName.toLocal8Bit());

            if (headerIndex == -1)
                q->setRawHeader(headerName.toLocal8Bit(), headersValue.toLocal8Bit());
            else
                q->setHeader(static_cast<QNetworkRequest::KnownHeaders>(headerIndex), (QVariant)headersValue);
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
