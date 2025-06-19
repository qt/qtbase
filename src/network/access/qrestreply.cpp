// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qrestreply.h"
#include "qrestreply_p.h"

#include <QtNetwork/private/qnetworkreply_p.h>
#include <QtNetwork/private/qrestaccessmanager_p.h>

#include <QtCore/qbytearrayview.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qlatin1stringmatcher.h>
#include <QtCore/qlatin1stringview.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstringconverter.h>

#include <QtCore/qxpfunctional.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QRestReply
    \since 6.7
    \brief QRestReply is a convenience wrapper for QNetworkReply.

    \reentrant
    \ingroup network
    \inmodule QtNetwork

    QRestReply wraps a QNetworkReply and provides convenience methods for data
    and status handling. The methods provide convenience for typical RESTful
    client applications.

    QRestReply doesn't take ownership of the wrapped QNetworkReply, and the
    lifetime and ownership of the reply is as defined by QNetworkAccessManager
    documentation.

    QRestReply object is not copyable, but is movable.

    \sa QRestAccessManager, QNetworkReply, QNetworkAccessManager,
        QNetworkAccessManager::setAutoDeleteReplies()
*/

/*!
    Creates a QRestReply and initializes the wrapped QNetworkReply to \a reply.
*/
QRestReply::QRestReply(QNetworkReply *reply)
    : wrapped(reply)
{
    if (!wrapped)
        qCWarning(lcQrest, "QRestReply: QNetworkReply is nullptr");
}

/*!
    Destroys this QRestReply object.
*/
QRestReply::~QRestReply()
{
    delete d;
}

/*!
    \fn QRestReply::QRestReply(QRestReply &&other) noexcept

    Move-constructs the reply from \a other.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
*/

/*!
    \fn QRestReply &QRestReply::operator=(QRestReply &&other) noexcept

    Move-assigns \a other and returns a reference to this reply.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
*/

/*!
    Returns a pointer to the underlying QNetworkReply wrapped by this object.
*/
QNetworkReply *QRestReply::networkReply() const
{
    return wrapped;
}

/*!
    Returns the received data as a QJsonDocument.

    The returned value is wrapped in \c std::optional. If the conversion
    from the received data fails (empty data or JSON parsing error),
    \c std::nullopt is returned, and \a error is filled with details.

    Calling this function consumes the received data, and any further calls
    to get response data will return empty.

    This function returns \c {std::nullopt} and will not consume
    any data if the reply is not finished. If \a error is passed, it will be
    set to QJsonParseError::NoError to distinguish this case from an actual
    error.

    \sa readBody(), readText()
*/
std::optional<QJsonDocument> QRestReply::readJson(QJsonParseError *error)
{
    if (!wrapped) {
        if (error)
            *error = {0, QJsonParseError::ParseError::NoError};
        return std::nullopt;
    }

    if (!wrapped->isFinished()) {
        qCWarning(lcQrest, "readJson() called on an unfinished reply, ignoring");
        if (error)
            *error = {0, QJsonParseError::ParseError::NoError};
        return std::nullopt;
    }
    QJsonParseError parseError;
    const QByteArray data = wrapped->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (error)
        *error = parseError;
    if (parseError.error)
        return std::nullopt;
    return doc;
}

/*!
    Returns the received data as a QByteArray.

    Calling this function consumes the data received so far, and any further
    calls to get response data will return empty until further data has been
    received.

    \sa readJson(), readText(), QNetworkReply::bytesAvailable(),
        QNetworkReply::readyRead()
*/
QByteArray QRestReply::readBody()
{
    return wrapped ? wrapped->readAll() : QByteArray{};
}

/*!
    Returns the received data as a QString.

    The received data is decoded into a QString (UTF-16). If available, the decoding
    uses the \e Content-Type header's \e charset parameter to determine the
    source encoding. If the encoding information is not available or not supported
    by \l QStringConverter, UTF-8 is used by default.

    Calling this function consumes the data received so far. Returns
    a default constructed value if no new data is available, or if the
    decoding is not supported by \l QStringConverter, or if the decoding
    has errors (for example invalid characters).

    \sa readJson(), readBody(), QNetworkReply::readyRead()
*/
QString QRestReply::readText()
{
    QString result;
    if (!wrapped)
        return result;

    QByteArray data = wrapped->readAll();
    if (data.isEmpty())
        return result;

    // Text decoding needs to persist decoding state across calls to this function,
    // so allocate decoder if not yet allocated.
    if (!d)
        d = new QRestReplyPrivate;

    if (!d->decoder) {
        const QByteArray charset = QRestReplyPrivate::contentCharset(wrapped);
        d->decoder.emplace(charset.constData());
        if (!d->decoder->isValid()) { // the decoder may not support the mimetype's charset
            qCWarning(lcQrest, "readText(): Charset \"%s\" is not supported", charset.constData());
            return result;
        }
    }
    // Check if the decoder already had an error, or has errors after decoding current data chunk
    if (d->decoder->hasError() || (result = (*d->decoder)(data), d->decoder->hasError())) {
        qCWarning(lcQrest, "readText(): Decoding error occurred");
        return {};
    }
    return result;
}

/*!
    Returns the HTTP status received in the server response.
    The value is \e 0 if not available (the status line has not been received,
    yet).

    \note The HTTP status is reported as indicated by the received HTTP
    response. An error() may occur after receiving the status, for instance
    due to network disconnection while receiving a long response.
    These potential subsequent errors are not represented by the reported
    HTTP status.

    \sa isSuccess(), hasError(), error()
*/
int QRestReply::httpStatus() const
{
    return wrapped ? wrapped->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() : 0;
}

/*!
    \fn bool QRestReply::isSuccess() const

    Returns whether the HTTP status is between 200..299 and no
    further errors have occurred while receiving the response (for example,
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
    if (!wrapped)
        return false;

    const int status = httpStatus();
    if (status > 0) {
        // The HTTP status is set upon receiving the response headers, but the
        // connection might still fail later while receiving the body data.
        return wrapped->error() == QNetworkReply::RemoteHostClosedError;
    }
    return wrapped->error() != QNetworkReply::NoError;
}

/*!
    Returns the last error, if any. The errors include
    errors such as network and protocol errors, but exclude
    cases when the server successfully responded with an HTTP status.

    \sa httpStatus(), isSuccess(), hasError(), errorString()
*/
QNetworkReply::NetworkError QRestReply::error() const
{
    if (!hasError())
        return QNetworkReply::NetworkError::NoError;
    return wrapped->error();
}

/*!
    Returns a human-readable description of the last network error.

    \sa httpStatus(), isSuccess(), hasError(), error()
*/
QString QRestReply::errorString() const
{
    if (hasError())
        return wrapped->errorString();
    return {};
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
    \fn QDebug QRestReply::operator<<(QDebug debug, const QRestReply &reply)

    Writes the \a reply into the \a debug object for debugging purposes.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, const QRestReply &reply)
{
    const QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    if (!reply.networkReply()) {
        debug << "QRestReply(no network reply)";
        return debug;
    }
    debug << "QRestReply(isSuccess = " << reply.isSuccess()
          << ", httpStatus = " << reply.httpStatus()
          << ", isHttpStatusSuccess = " << reply.isHttpStatusSuccess()
          << ", hasError = " << reply.hasError()
          << ", errorString = " << reply.errorString()
          << ", error = " << reply.error()
          << ", isFinished = " << reply.networkReply()->isFinished()
          << ", bytesAvailable = " << reply.networkReply()->bytesAvailable()
          << ", url " << reply.networkReply()->url()
          << ", operation = " << operationName(reply.networkReply()->operation())
          << ", reply headers = " << reply.networkReply()->headers()
          << ")";
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

static constexpr auto parse_OWS(QByteArrayView data) noexcept
{
    struct R {
        QByteArrayView ows, tail;
    };

    constexpr auto is_OWS_char = [](auto ch) { return ch == ' ' || ch == '\t'; };

    qsizetype i = 0;
    while (i < data.size() && is_OWS_char(data[i]))
        ++i;

    return R{data.first(i), data.sliced(i)};
}

static constexpr void eat_OWS(QByteArrayView &data) noexcept
{
    data = parse_OWS(data).tail;
}

static constexpr auto parse_quoted_string(QByteArrayView data, qxp::function_ref<void(char) const> yield)
{
    struct R {
        QByteArrayView quotedString, tail;
        constexpr explicit operator bool() const noexcept { return !quotedString.isEmpty(); }
    };

    if (!data.startsWith('"'))
        return R{{}, data};

    qsizetype i = 1; // one past initial DQUOTE
    while (i < data.size()) {
        switch (auto ch = data[i++]) {
        case '"':  // final DQUOTE -> end of string
            return R{data.first(i), data.sliced(i)};
        case '\\': // quoted-pair
            // https://www.rfc-editor.org/rfc/rfc9110.html#section-5.6.4-3:
            //   Recipients that process the value of a quoted-string MUST handle a
            //   quoted-pair as if it were replaced by the octet following the backslash.
            if (i == data.size())
                break; // premature end
            ch = data[i++]; // eat '\\'
            [[fallthrough]];
        default:
            // we don't validate quoted-string octets to be only qdtext (Postel's Law)
            yield(ch);
        }
    }

    return R{{}, data}; // premature end
}

static constexpr bool is_tchar(char ch) noexcept
{
    // ### optimize
    switch (ch) {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
        return true;
    default:
        return (ch >= 'a' && ch <= 'z')
            || (ch >= '0' && ch <= '9')
            || (ch >= 'A' && ch <= 'Z');
    }
}

static constexpr auto parse_comment(QByteArrayView data) noexcept
{
    struct R {
        QByteArrayView comment, tail;
        constexpr explicit operator bool() const noexcept { return !comment.isEmpty(); }
    };

    const auto invalid = R{{}, data}; // preserves original `data`

    // comment        = "(" *( ctext / quoted-pair / comment ) ")"
    // ctext          = HTAB / SP / %x21-27 / %x2A-5B / %x5D-7E / obs-text

    if (!data.startsWith('('))
        return invalid;

    qsizetype i = 1;
    qsizetype level = 1;
    while (i < data.size()) {
        switch (data[i++]) {
        case '(': // nested comment
            ++level;
            break;
        case ')': // end of comment
            if (--level == 0)
                return R{data.first(i), data.sliced(i)};
            break;
        case '\\': // quoted-pair
            if (i == data.size())
                return invalid; // premature end
            ++i; // eat escaped character
            break;
        default:
            ; // don't validate ctext - accept everything (Postel's Law)
        }
    }

    return invalid; // premature end / unbalanced nesting levels
}

static constexpr void eat_CWS(QByteArrayView &data) noexcept
{
    eat_OWS(data);
    while (const auto comment = parse_comment(data)) {
        data = comment.tail;
        eat_OWS(data);
    }
}

static constexpr auto parse_token(QByteArrayView data) noexcept
{
    struct R {
        QByteArrayView token, tail;
        constexpr explicit operator bool() const noexcept { return !token.isEmpty(); }
    };

    qsizetype i = 0;
    while (i < data.size() && is_tchar(data[i]))
        ++i;

    return R{data.first(i), data.sliced(i)};
}

static constexpr auto parse_parameter(QByteArrayView data, qxp::function_ref<void(char) const> yield)
{
    struct R {
        QLatin1StringView name; QByteArrayView value; QByteArrayView tail;
        constexpr explicit operator bool() const noexcept { return !name.isEmpty(); }
    };

    const auto invalid = R{{}, {}, data}; // preserves original `data`

    // parameter       = parameter-name "=" parameter-value
    // parameter-name  = token
    // parameter-value = ( token / quoted-string )

    const auto name = parse_token(data);
    if (!name)
        return invalid;
    data = name.tail;

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    if (!data.startsWith('='))
        return invalid;
    data = data.sliced(1);

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    if (Q_UNLIKELY(data.startsWith('"'))) { // value is a quoted-string

        const auto value = parse_quoted_string(data, yield);
        if (!value)
            return invalid;
        data = value.tail;

        return R{QLatin1StringView{name.token}, value.quotedString, data};

    } else { // value is a token

        const auto value = parse_token(data);
        if (!value)
            return invalid;
        data = value.tail;

        return R{QLatin1StringView{name.token}, value.token, data};
    }
}

static auto parse_content_type(QByteArrayView data)
{
    struct R {
        QLatin1StringView type, subtype;
        std::string charset;
        constexpr explicit operator bool() const noexcept { return !type.isEmpty(); }
    };

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    const auto type = parse_token(data);
    if (!type)
        return R{};
    data = type.tail;

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    if (!data.startsWith('/'))
        return R{};
    data = data.sliced(1);

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    const auto subtype = parse_token(data);
    if (!subtype)
        return R{};
    data = subtype.tail;

    eat_CWS(data);

    auto r = R{QLatin1StringView{type.token}, QLatin1StringView{subtype.token}, {}};

    while (data.startsWith(';')) {

        data = data.sliced(1); // eat ';'

        eat_CWS(data);

        const auto param = parse_parameter(data, [&](char ch) { r.charset.append(1, ch); });
        if (param.name.compare("charset"_L1, Qt::CaseInsensitive) == 0) {
            if (r.charset.empty() && !param.value.startsWith('"')) // wasn't a quoted-string
                r.charset.assign(param.value.begin(), param.value.end());
            return r; // charset found
        }
        r.charset.clear(); // wasn't an actual charset
        if (param.tail.size() == data.size()) // no progress was made
            break; // returns {type, subtype}
        // otherwise, continue (accepting e.g. `;;`)
        data = param.tail;

        eat_CWS(data);
    }

    return r; // no charset found
}

QByteArray QRestReplyPrivate::contentCharset(const QNetworkReply* reply)
{
    // Content-type consists of mimetype and optional parameters, of which one may be 'charset'
    // Example values and their combinations below are all valid, see RFC 7231 section 3.1.1.5
    // and RFC 2045 section 5.1
    //
    // text/plain; charset=utf-8
    // text/plain; charset=utf-8;version=1.7
    // text/plain; charset = utf-8
    // text/plain; charset ="utf-8"

    const QByteArray contentTypeValue =
            reply->headers().value(QHttpHeaders::WellKnownHeader::ContentType).toByteArray();

    const auto r = parse_content_type(contentTypeValue);
    if (r && !r.charset.empty())
        return QByteArrayView(r.charset).toByteArray();
    else
        return "UTF-8"_ba; // Default to the most commonly used UTF-8.
}

QT_END_NAMESPACE
