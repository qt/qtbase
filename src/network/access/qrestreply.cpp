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

    \sa bytesAvailable(), readyRead()
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
    Returns the received data as a QJsonObject. Requires the reply to be
    finished.

    The returned value is wrapped in \c std::optional. If the conversion
    from the received data fails (empty data or JSON parsing error),
    \c std::nullopt is returned.

    Calling this function consumes the received data, and any further calls
    to get response data will return empty.

    This function returns \c {std::nullopt} and will not consume
    any data if the reply is not finished.

    \sa jsonArray(), body(), text(), finished(), isFinished()
*/
std::optional<QJsonObject> QRestReply::json()
{
    Q_D(QRestReply);
    if (!isFinished()) {
        qCWarning(lcQrest, "Attempt to read json() of an unfinished reply, ignoring.");
        return std::nullopt;
    }
    const QJsonDocument json = d->replyDataToJson();
    return json.isObject() ? std::optional{json.object()} : std::nullopt;
}

/*!
    Returns the received data as a QJsonArray. Requires the reply to be
    finished.

    The returned value is wrapped in \c std::optional. If the conversion
    from the received data fails (empty data or JSON parsing error),
    \c std::nullopt is returned.

    Calling this function consumes the received data, and any further calls
    to get response data will return empty.

    This function returns \c {std::nullopt} and will not consume
    any data if the reply is not finished.

    \sa json(), body(), text(), finished(), isFinished()
*/
std::optional<QJsonArray> QRestReply::jsonArray()
{
    Q_D(QRestReply);
    if (!isFinished()) {
        qCWarning(lcQrest, "Attempt to read jsonArray() of an unfinished reply, ignoring.");
        return std::nullopt;
    }
    const QJsonDocument json = d->replyDataToJson();
    return json.isArray() ? std::optional{json.array()} : std::nullopt;
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

QJsonDocument QRestReplyPrivate::replyDataToJson()
{
    QJsonParseError parseError;
    const QByteArray data = networkReply->readAll();
    const QJsonDocument json = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qCDebug(lcQrest) << "Response data not JSON:" << parseError.errorString()
                         << "at" << parseError.offset << data;
    }
    return json;
}

QT_END_NAMESPACE

#include "moc_qrestreply.cpp"
