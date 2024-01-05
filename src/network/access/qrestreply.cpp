// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrestreply.h"
#include "qrestreply_p.h"

#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qlatin1stringmatcher.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstringconverter.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
Q_DECLARE_LOGGING_CATEGORY(lcQrest)

/*!
    \class QRestReply
    \since 6.7
    \brief QRestReply is the class for following up the requests sent with
    QRestAccessManager.

    \reentrant
    \ingroup network
    \inmodule QtNetwork

    \preliminary

    QRestReply is a convenience class for typical RESTful client
    applications. It wraps the more detailed QNetworkReply and provides
    convenience methods for data and status handling.

    \sa QRestAccessManager, QNetworkReply
*/

/*!
    \fn void QRestReply::readyRead(QRestReply *reply)

    This signal is emitted when \a reply has received new data.

    \sa body(), bytesAvailable(), isFinished()
*/

/*!
    \fn void QRestReply::downloadProgress(qint64 bytesReceived,
                                          qint64 bytesTotal,
                                          QRestReply* reply)

    This signal is emitted to indicate the progress of the download part of
    this network \a reply.

    The \a bytesReceived parameter indicates the number of bytes received,
    while \a bytesTotal indicates the total number of bytes expected to be
    downloaded. If the number of bytes to be downloaded is not known, for
    instance due to a missing \c Content-Length header, \a bytesTotal
    will be -1.

    See \l QNetworkReply::downloadProgress() documentation for more details.

    \sa bytesAvailable(), readyRead(), uploadProgress()
*/

/*!
    \fn void QRestReply::uploadProgress(qint64 bytesSent, qint64 bytesTotal,
                                        QRestReply* reply)

    This signal is emitted to indicate the progress of the upload part of
    \a reply.

    The \a bytesSent parameter indicates the number of bytes already uploaded,
    while \a bytesTotal indicates the total number of bytes still to upload.

    If the number of bytes to upload is not known, \a bytesTotal will be -1.

    See \l QNetworkReply::uploadProgress() documentation for more details.

    \sa QNetworkReply::uploadProgress(), downloadProgress()
*/

/*!
    \fn void QRestReply::finished(QRestReply *reply)

    This signal is emitted when \a reply has finished processing. This
    signal is emitted also in cases when the reply finished due to network
    or protocol errors (the server did not reply with an HTTP status).

    \sa isFinished(), httpStatus(), error()
*/

/*!
    \fn void QRestReply::errorOccurred(QRestReply *reply)

    This signal is emitted if, while processing \a reply, an error occurs that
    is considered to be a network/protocol error. These errors are
    disctinct from HTTP error responses such as \c {500 Internal Server Error}.
    This signal is emitted together with the
    finished() signal, and often connecting to that is sufficient.

    \sa finished(), isFinished(), httpStatus(), error()
*/

QRestReply::QRestReply(QNetworkReply *reply, QObject *parent)
    : QObject(*new QRestReplyPrivate, parent)
{
    Q_D(QRestReply);
    Q_ASSERT(reply);
    d->networkReply = reply;
    // Reparent so that destruction of QRestReply destroys QNetworkReply
    reply->setParent(this);

    QObject::connect(reply, &QNetworkReply::readyRead, this, [this] {
        emit readyRead(this);
    });
    QObject::connect(reply, &QNetworkReply::downloadProgress, this,
                     [this](qint64 bytesReceived, qint64 bytesTotal) {
                         emit downloadProgress(bytesReceived, bytesTotal, this);
                     });
    QObject::connect(reply, &QNetworkReply::uploadProgress, this,
                     [this] (qint64 bytesSent, qint64 bytesTotal) {
                         emit uploadProgress(bytesSent, bytesTotal, this);
                     });
}

/*!
    Destroys this QRestReply object.

    \sa abort()
*/
QRestReply::~QRestReply()
    = default;

/*!
    Returns a pointer to the underlying QNetworkReply wrapped by this object.
*/
QNetworkReply *QRestReply::networkReply() const
{
    Q_D(const QRestReply);
    return d->networkReply;
}

/*!
    Aborts the network operation immediately. The finished() signal
    will be emitted.

    \sa QRestAccessManager::abortRequests() QNetworkReply::abort()
*/
void QRestReply::abort()
{
    Q_D(QRestReply);
    d->networkReply->abort();
}

/*!
    Returns the received data as a QJsonDocument.

    The returned value is wrapped in \c std::optional. If the conversion
    from the received data fails (empty data or JSON parsing error),
    \c std::nullopt is returned.

    Calling this function consumes the received data, and any further calls
    to get response data will return empty.

    This function returns \c {std::nullopt} and will not consume
    any data if the reply is not finished.

    \sa body(), text(), finished(), isFinished()
*/
std::optional<QJsonDocument> QRestReply::json()
{
    Q_D(QRestReply);
    if (!isFinished()) {
        qCWarning(lcQrest, "Attempt to read json() of an unfinished reply, ignoring.");
        return std::nullopt;
    }
    QJsonParseError parseError;
    const QByteArray data = d->networkReply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCDebug(lcQrest) << "Response data not JSON:" << parseError.errorString()
                         << "at" << parseError.offset << data;
        return std::nullopt;
    }
    return doc;
}

/*!
    Returns the received data as a QByteArray.

    Calling this function consumes the data received so far, and any further
    calls to get response data will return empty until further data has been
    received.

    \sa json(), text(), bytesAvailable(), readyRead()
*/
QByteArray QRestReply::body()
{
    Q_D(QRestReply);
    return d->networkReply->readAll();
}

/*!
    Returns the received data as a QString. Requires the reply to be finished.

    The received data is decoded into a QString (UTF-16). The decoding
    uses the \e Content-Type header's \e charset parameter to determine the
    source encoding, if available. If the encoding information is not
    available or not supported by \l QStringConverter, UTF-8 is used as a
    default.

    Calling this function consumes the received data, and any further calls
    to get response data will return empty.

    This function returns a default-constructed value and will not consume
    any data if the reply is not finished.

    \sa json(), body(), isFinished(), finished()
*/
QString QRestReply::text()
{
    Q_D(QRestReply);
    if (!isFinished()) {
        qCWarning(lcQrest, "Attempt to read text() of an unfinished reply, ignoring.");
        return {};
    }
    QByteArray data = d->networkReply->readAll();
    if (data.isEmpty())
        return {};

    const QByteArray charset = d->contentCharset();
    QStringDecoder decoder(charset);
    if (!decoder.isValid()) { // the decoder may not support the mimetype's charset
        qCWarning(lcQrest, "Charset \"%s\" is not supported, defaulting to UTF-8",
                  charset.constData());
        decoder = QStringDecoder(QStringDecoder::Utf8);
    }
    return decoder(data);
}

/*!
    Returns the HTTP status received in the server response.
    The value is \e 0 if not available (the status line has not been received,
    yet).

    \note The HTTP status is reported as indicated by the received HTTP
    response. It is possible that an error() occurs after receiving the status,
    for instance due to network disconnection while receiving a long response.
    These potential subsequent errors are not represented by the reported
    HTTP status.

    \sa isSuccess(), hasError(), error()
*/
int QRestReply::httpStatus() const
{
    Q_D(const QRestReply);
    return d->networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
}

/*!
    \fn bool QRestReply::isSuccess() const

    Returns whether the HTTP status is between 200..299 and no
    further errors have occurred while receiving the response (for example
    abrupt disconnection while receiving the body data). This function
    is a convenient way to check whether the response is considered successful.

    \sa httpStatus(), hasError(), error()
*/

/*!
    Returns whether the HTTP status is between 200..299.

    \sa isSuccess(), httpStatus(), hasError(), error()
*/
bool QRestReply::isHttpStatusSuccess() const
{
    const int status = httpStatus();
    return status >= 200 && status < 300;
}

/*!
    Returns whether an error has occurred. This includes errors such as
    network and protocol errors, but excludes cases where the server
    successfully responded with an HTTP error status (for example
    \c {500 Internal Server Error}). Use \l httpStatus() or
    \l isHttpStatusSuccess() to get the HTTP status information.

    \sa httpStatus(), isSuccess(), error(), errorString()
*/
bool QRestReply::hasError() const
{
    Q_D(const QRestReply);
    return d->hasNonHttpError();
}

/*!
    Returns the last error, if any. The errors include
    errors such as network and protocol errors, but exclude
    cases when the server successfully responded with an HTTP status.

    \sa httpStatus(), isSuccess(), hasError(), errorString()
*/
QNetworkReply::NetworkError QRestReply::error() const
{
    Q_D(const QRestReply);
    if (!hasError())
        return QNetworkReply::NetworkError::NoError;
    return d->networkReply->error();
}

/*!
    Returns a human-readable description of the last network error.

    \sa httpStatus(), isSuccess(), hasError(), error()
*/
QString QRestReply::errorString() const
{
    Q_D(const QRestReply);
    if (hasError())
        return d->networkReply->errorString();
    return {};
}

/*!
    Returns whether the network request has finished.
*/
bool QRestReply::isFinished() const
{
    Q_D(const QRestReply);
    return d->networkReply->isFinished();
}

/*!
    Returns the number of bytes available.

    \sa body
*/
qint64 QRestReply::bytesAvailable() const
{
    Q_D(const QRestReply);
    return d->networkReply->bytesAvailable();
}

QRestReplyPrivate::QRestReplyPrivate()
    = default;

QRestReplyPrivate::~QRestReplyPrivate()
    = default;

#ifndef QT_NO_DEBUG_STREAM
static QLatin1StringView operationName(QNetworkAccessManager::Operation operation)
{
    switch (operation) {
    case QNetworkAccessManager::Operation::GetOperation:
        return "GET"_L1;
    case QNetworkAccessManager::Operation::HeadOperation:
        return "HEAD"_L1;
    case QNetworkAccessManager::Operation::PostOperation:
        return "POST"_L1;
    case QNetworkAccessManager::Operation::PutOperation:
        return "PUT"_L1;
    case QNetworkAccessManager::Operation::DeleteOperation:
        return "DELETE"_L1;
    case QNetworkAccessManager::Operation::CustomOperation:
        return "CUSTOM"_L1;
    case QNetworkAccessManager::Operation::UnknownOperation:
        return "UNKNOWN"_L1;
    }
    Q_UNREACHABLE_RETURN({});
}

/*!
    \fn QDebug QRestReply::operator<<(QDebug debug, const QRestReply *reply)

    Writes the \a reply into the \a debug object for debugging purposes.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, const QRestReply *reply)
{
    const QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    if (!reply) {
        debug << "QRestReply(nullptr)";
        return debug;
    }

    debug << "QRestReply(isSuccess = " << reply->isSuccess()
          << ", httpStatus = " << reply->httpStatus()
          << ", isHttpStatusSuccess = " << reply->isHttpStatusSuccess()
          << ", hasError = " << reply->hasError()
          << ", errorString = " << reply->errorString()
          << ", error = " << reply->error()
          << ", isFinished = " << reply->isFinished()
          << ", bytesAvailable = " << reply->bytesAvailable()
          << ", url " << reply->networkReply()->url()
          << ", operation = " << operationName(reply->networkReply()->operation())
          << ", reply headers = " << reply->networkReply()->rawHeaderPairs()
          << ")";
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

QByteArray QRestReplyPrivate::contentCharset() const
{
    // Content-type consists of mimetype and optional parameters, of which one may be 'charset'
    // Example values and their combinations below are all valid, see RFC 7231 section 3.1.1.5
    // and RFC 2045 section 5.1
    //
    // text/plain; charset=utf-8
    // text/plain; charset=utf-8;version=1.7
    // text/plain; charset = utf-8
    // text/plain; charset ="utf-8"
    QByteArray contentTypeValue =
               networkReply->header(QNetworkRequest::KnownHeaders::ContentTypeHeader).toByteArray();
    // Default to the most commonly used UTF-8.
    QByteArray charset{"UTF-8"};

    QList<QByteArray> parameters = contentTypeValue.split(';');
    if (parameters.size() >= 2) { // Need at least one parameter in addition to the mimetype itself
        parameters.removeFirst(); // Exclude the mimetype itself, only interested in parameters
        QLatin1StringMatcher matcher("charset="_L1, Qt::CaseSensitivity::CaseInsensitive);
        qsizetype matchIndex = -1;
        for (auto &parameter : parameters) {
            // Remove whitespaces and parantheses
            const QByteArray curated = parameter.replace(" ", "").replace("\"","");
            // Check for match
            matchIndex = matcher.indexIn(QLatin1String(curated.constData()));
            if (matchIndex >= 0) {
                charset = curated.sliced(matchIndex + 8); // 8 is size of "charset="
                break;
            }
        }
    }
    return charset;
}

// Returns true if there's an error that isn't appropriately indicated by the HTTP status
bool QRestReplyPrivate::hasNonHttpError() const
{
    const int status = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status > 0) {
        // The HTTP status is set upon receiving the response headers, but the
        // connection might still fail later while receiving the body data.
        return networkReply->error() == QNetworkReply::RemoteHostClosedError;
    }
    return networkReply->error() != QNetworkReply::NoError;
}

QT_END_NAMESPACE

#include "moc_qrestreply.cpp"
