/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qnetworkreplyemscriptenimpl_p.h"

#include <QTimer>
#include "QtCore/qdatetime.h"
#include "qnetworkaccessmanager_p.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include "qnetworkfile_p.h"
#include "qnetworkrequest.h"
//#include "qnetworkaccessbackend_p.h"

#include <iostream>

QT_BEGIN_NAMESPACE

QNetworkReplyEmscriptenImplPrivate::QNetworkReplyEmscriptenImplPrivate()
    : QNetworkReplyPrivate(),
      managerPrivate(0),
      downloadBufferReadPosition(0),
      downloadBufferCurrentSize(0),
      totalDownloadSize(0),
      percentFinished(0)
{
}

QNetworkReplyEmscriptenImplPrivate::~QNetworkReplyEmscriptenImplPrivate()
{
}

QNetworkReplyEmscriptenImpl::~QNetworkReplyEmscriptenImpl()
{
}

QNetworkReplyEmscriptenImpl::QNetworkReplyEmscriptenImpl(QObject *parent)
    : QNetworkReply(*new QNetworkReplyEmscriptenImplPrivate(), parent)
{
}

QByteArray QNetworkReplyEmscriptenImpl::methodName() const
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
//    case QNetworkAccessManager::CustomOperation:
//        return d->customVerb;
    default:
        break;
    }
    return QByteArray();
}

void QNetworkReplyEmscriptenImpl::close()
{
    QNetworkReply::close();
}

void QNetworkReplyEmscriptenImpl::abort()
{
    close();
}

qint64 QNetworkReplyEmscriptenImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyEmscriptenImpl);

    if (!d->isFinished)
        return QNetworkReply::bytesAvailable();

    return QNetworkReply::bytesAvailable() + d->downloadBufferCurrentSize - d->downloadBufferReadPosition;
}

bool QNetworkReplyEmscriptenImpl::isSequential() const
{
    return true;
}

qint64 QNetworkReplyEmscriptenImpl::size() const
{
    return QNetworkReply::size();
}

/*!
    \internal
*/
qint64 QNetworkReplyEmscriptenImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyEmscriptenImpl);

    qint64 howMuch = qMin(maxlen, (d->downloadBuffer.size()- d->downloadBufferReadPosition));
    memcpy(data, d->downloadBuffer.constData(), howMuch);
    d->downloadBufferReadPosition += howMuch;

    return howMuch;
}

void QNetworkReplyEmscriptenImplPrivate::setup(QNetworkAccessManager::Operation op, const QNetworkRequest &req,
                                     QIODevice *data)
{
    Q_Q(QNetworkReplyEmscriptenImpl);

    outgoingData = data;
    request = req;
    url = request.url();
    operation = op;

    q->QIODevice::open(QIODevice::ReadOnly);
    if (outgoingData) {
        if (!outgoingData->isSequential()) {
            // fixed size non-sequential (random-access)
            // just start the operation
            doSendRequest();
            return;
        } else {
            bool bufferingDisallowed =
                    request.attribute(QNetworkRequest::DoNotBufferUploadDataAttribute,
                                      false).toBool();

            if (bufferingDisallowed) {
                // if a valid content-length header for the request was supplied, we can disable buffering
                // if not, we will buffer anyway
                if (request.header(QNetworkRequest::ContentLengthHeader).isValid()) {
                    doSendRequest();
                    return;
                } else {
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
    }
    // No outgoing data (POST, ..)
    doSendRequest();
}

void QNetworkReplyEmscriptenImplPrivate::onLoadCallback(void *data, int statusCode, int statusReason, int readyState, int buffer, int bufferSize)
{
    QNetworkReplyEmscriptenImplPrivate *handler = reinterpret_cast<QNetworkReplyEmscriptenImplPrivate*>(data);

    QString reasonStr = QString::fromUtf8((char *)statusReason);

    switch(readyState) {
    case 0://unsent
        break;
    case 1://opened
        break;
    case 2://headers received
        break;
    case 3://loading
        break;
    case 4: {//done
        handler->q_func()->setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
        if (!reasonStr.isEmpty())
            handler->q_func()->setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, reasonStr);

        if (statusCode >= 400) {
            if (!reasonStr.isEmpty())
                handler->emitReplyError(handler->statusCodeFromHttp(statusCode, handler->request.url()), reasonStr);
        } else {
            handler->dataReceived((char *)buffer, bufferSize);
        }
    }
     break;
    };
 }

void QNetworkReplyEmscriptenImplPrivate::onProgressCallback(void* data, int bytesWritten, int total, uint timestamp)
{
    Q_UNUSED(timestamp);

    QNetworkReplyEmscriptenImplPrivate *handler = reinterpret_cast<QNetworkReplyEmscriptenImplPrivate*>(data);
    handler->emitDataReadProgress(bytesWritten, total);
}

void QNetworkReplyEmscriptenImplPrivate::onRequestErrorCallback(void* data, int statusCode, int statusReason)
{
    QString reasonStr = QString::fromUtf8((char *)statusReason);

    QNetworkReplyEmscriptenImplPrivate *handler = reinterpret_cast<QNetworkReplyEmscriptenImplPrivate*>(data);

    handler->q_func()->setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
    if (!reasonStr.isEmpty())
        handler->q_func()->setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, reasonStr);

    if (statusCode >= 400) {
        if (!reasonStr.isEmpty())
            handler->emitReplyError(handler->statusCodeFromHttp(statusCode, handler->request.url()), reasonStr);
    }
}

void QNetworkReplyEmscriptenImplPrivate::onResponseHeadersCallback(void* data, int headers)
{
    QNetworkReplyEmscriptenImplPrivate *handler = reinterpret_cast<QNetworkReplyEmscriptenImplPrivate*>(data);
    handler->headersReceived((char *)headers);
}

void QNetworkReplyEmscriptenImplPrivate::doSendRequest()
{
    Q_Q(QNetworkReplyEmscriptenImpl);
    totalDownloadSize = 0;
    jsRequest(QString::fromUtf8(q->methodName()), // GET POST
              request.url().toString(),
              (void *)&onLoadCallback,
              (void *)&onProgressCallback,
              (void *)&onRequestErrorCallback,
              (void *)&onResponseHeadersCallback);
}

/* const QString &body, const QList<QPair<QByteArray, QByteArray> > &headers ,*/
void QNetworkReplyEmscriptenImplPrivate::jsRequest(const QString &verb, const QString &url, void *loadCallback, void *progressCallback, void *errorCallback, void *onResponseHeadersCallback)
{
    QString extraDataString;

    QByteArray extraData;
    if (outgoingData)
        extraData = outgoingData->readAll();

    if (extraData.size() > 0)
        extraDataString.fromUtf8(extraData);

    if (extraDataString.size() >= 0 && verb == QStringLiteral("POST") && extraDataString.startsWith(QStringLiteral("?")))
        extraDataString.remove(QStringLiteral("?"));

    // Probably a good idea to save any shared pointers as members in C++
    // so the objects they point to survive as long as you need them

    QList<QByteArray> headersData = request.rawHeaderList();
    QStringList headersList;
    for (int i = 0; i < headersData.size(); ++i) {
        headersList << QString::fromUtf8(headersData.at(i) + ":" + request.rawHeader(headersData.at(i)));
    }

    EM_ASM_ARGS({
        var verb = Pointer_stringify($0);
        var url = Pointer_stringify($1);
        var onLoadCallbackPointer = $2;
        var onProgressCallbackPointer = $3;
        var onErrorCallbackPointer = $4;
        var onHeadersCallback = $5;
        var handler = $8;

        var dataToSend;
        var extraRequestData = Pointer_stringify($6); // request parameters
        var headersData = Pointer_stringify($7);

        var xhr;
        xhr = new XMLHttpRequest();
        xhr.responseType = 'arraybuffer';

        xhr.open(verb, url, true); //async

        function handleError(xhrStatusCode, xhrStatusText) {
            var errorPtr = allocate(intArrayFromString(xhrStatusText), 'i8', ALLOC_NORMAL);
            Runtime.dynCall('viii', onErrorCallbackPointer, [handler, xhrStatusCode, errorPtr]);
            _free(errorPtr);
        }

        if (headersData) {
            var headers = headersData.split("&");
            for (var i = 0; i < headers.length; i++) {
                var header = headers[i].split(":")[0];
                var value =  headers[i].split(":")[1];

                if (verb === 'POST' && value.toLowerCase().includes('json')) {
                    if (extraRequestData) {
                        xhr.responseType = 'json';
                        dataToSend = extraRequestData;
                    }
                }
                if (verb === 'POST' && value.toLowerCase().includes('form')) {
                    if (extraRequestData) {
                        var formData = new FormData();
                        var extra = extraRequestData.split("&");
                        for (var i = 0; i < extra.length; i++) {
                            formData.append(extra[i].split("=")[0],extra[i].split("=")[1]);
                        }
                        dataToSend = formData;
                    }
                }
             xhr.setRequestHeader(header, value);
            }
        }

        xhr.onprogress = function(e) {
            switch(xhr.status) {
              case 200:
              case 206:
              case 300:
              case 301:
              case 302: {
                var date = xhr.getResponseHeader('Last-Modified');
                date = ((date != null) ? new Date(date).getTime() / 1000 : 0);
                Runtime.dynCall('viiii', onProgressCallbackPointer, [handler, e.loaded, e.total, date]);
              }
             break;
           }
        };

        xhr.onreadystatechange = function() {
            if (this.readyState == this.UNSENT) console.log("UNSENT: Client has been created. open() not called yet."); //0
            if (this.readyState == this.OPENED) console.log("OPENED: open() has been called."); //1
            if (this.readyState == this.HEADERS_RECEIVED) {
                    var responseStr = this.getAllResponseHeaders();
                    if (responseStr.length > 0) {
                        var ptr = allocate(intArrayFromString(responseStr), 'i8', ALLOC_NORMAL);
                        Runtime.dynCall('vii', onHeadersCallback, [handler, ptr]);
                        _free(ptr);
                    }
                }
         if (this.readyState == this.LOADING) console.log("LOADING: Downloading; responseText holds partial data.");//3
             if (this.readyState == this.DONE) {
                            console.log("DONE: The operation is complete.");//4

                        if (this.status < 300) { //success
                           if (this.status == 200 || this.status == 203) {
                                var datalength;
                                var byteArray = 0;
                                var buffer;

                                if (this.responseType.length === 0 || this.responseType === 'document') {
                                   console.log("xhr.responseText " + this.responseText);
                                   byteArray = new Uint8Array(this.responseText);
                                } else  if (this.responseType === 'json') {

                                   var jsonResponse = JSON.stringify(this.response);
                                   buffer = allocate(intArrayFromString(jsonResponse), 'i8', ALLOC_NORMAL);
                                   datalength = jsonResponse.length;
                                } else  if (this.responseType === 'arraybuffer') {
                                    byteArray  = new Uint8Array(xhr.response);
                                }
                                if (byteArray != 0 ) {
                                    datalength = byteArray.length;
                                    buffer = _malloc(datalength);
                                    HEAPU8.set(byteArray, buffer);
                                }
                                var reasonPtr = allocate(intArrayFromString(this.statusText), 'i8', ALLOC_NORMAL);
                                Runtime.dynCall('viiiiii', onLoadCallbackPointer, [handler, this.status, reasonPtr, this.readyState, buffer, datalength]);
                                _free(buffer);
                                _free(reasonPtr);
                               }
                            }
                        }
                    };

        xhr.onload = function(e) {
            if (xhr.status >= 300) { //error
                    handleError(xhr.status, xhr.statusText);
                }
        };

        xhr.onerror = function(e) {
            handleError(xhr.status, xhr.statusText);
        };
        //TODO other operations, handle user/pass, handle binary data, data streaming
        xhr.send(dataToSend);

      }, verb.toLatin1().data(),
                    url.toLatin1().data(),
                    loadCallback,
                    progressCallback,
                    errorCallback,
                    onResponseHeadersCallback,
                    extraDataString.size() > 0 ? extraDataString.toLatin1().data() : extraData.data(),
                    headersList.join(QStringLiteral("&")).toLatin1().data(),
                    this
                    );
}

void QNetworkReplyEmscriptenImplPrivate::emitReplyError(QNetworkReply::NetworkError errorCode, const QString &errorString)
{
    Q_UNUSED(errorCode)
    Q_Q(QNetworkReplyEmscriptenImpl);

    q->setError(errorCode, errorString);
    emit q->error(errorCode);

    q->setFinished(true);
    emit q->finished();
}

void QNetworkReplyEmscriptenImplPrivate::emitDataReadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_Q(QNetworkReplyEmscriptenImpl);

    totalDownloadSize = bytesTotal;

    percentFinished = (bytesReceived / bytesTotal) * 100;

    emit q->downloadProgress(bytesReceived, totalDownloadSize);
}

void QNetworkReplyEmscriptenImplPrivate::dataReceived(char *buffer, int bufferSize)
{
    Q_Q(QNetworkReplyEmscriptenImpl);

    if (bufferSize > 0)
        q->setReadBufferSize(bufferSize);

    bytesDownloaded = bufferSize;

    if (percentFinished != 100) {
        downloadBufferCurrentSize += bufferSize;
    } else {
        downloadBufferCurrentSize = bufferSize;
    }
    totalDownloadSize = downloadBufferCurrentSize;

    downloadBuffer.append(buffer, bufferSize);

    if (downloadBufferCurrentSize == totalDownloadSize) {
         q->setFinished(true);
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


void QNetworkReplyEmscriptenImplPrivate::headersReceived(char *buffer)
{
    Q_Q(QNetworkReplyEmscriptenImpl);

    QString bufferString = QString::fromUtf8(buffer);
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

void QNetworkReplyEmscriptenImplPrivate::_q_bufferOutgoingDataFinished()
{
    Q_Q(QNetworkReplyEmscriptenImpl);

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

void QNetworkReplyEmscriptenImplPrivate::_q_bufferOutgoingData()
{
    Q_Q(QNetworkReplyEmscriptenImpl);

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
QNetworkReply::NetworkError QNetworkReplyEmscriptenImplPrivate::statusCodeFromHttp(int httpStatusCode, const QUrl &url)
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
    }

    return code;
}


QT_END_NAMESPACE

#include "moc_qnetworkreplyemscriptenimpl_p.cpp"

