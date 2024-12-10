// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:network-protocol

#include "qhttpnetworkreply_p.h"
#include "qhttpnetworkconnection_p.h"

#ifndef QT_NO_SSL
#    include <QtNetwork/qsslkey.h>
#    include <QtNetwork/qsslcipher.h>
#    include <QtNetwork/qsslconfiguration.h>
#endif

#include <private/qdecompresshelper_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QHttpNetworkReply::QHttpNetworkReply(const QUrl &url, QObject *parent)
    : QObject(*new QHttpNetworkReplyPrivate(url), parent)
{
}

QHttpNetworkReply::~QHttpNetworkReply()
{
    Q_D(QHttpNetworkReply);
    if (d->connection) {
        d->connection->d_func()->removeReply(this);
    }
}

QUrl QHttpNetworkReply::url() const
{
    return d_func()->url;
}
void QHttpNetworkReply::setUrl(const QUrl &url)
{
    Q_D(QHttpNetworkReply);
    d->url = url;
}

QUrl QHttpNetworkReply::redirectUrl() const
{
    return d_func()->redirectUrl;
}

void QHttpNetworkReply::setRedirectUrl(const QUrl &url)
{
    Q_D(QHttpNetworkReply);
    d->redirectUrl = url;
}

bool QHttpNetworkReply::isHttpRedirect(int statusCode)
{
    return (statusCode == 301 || statusCode == 302 || statusCode == 303
            || statusCode == 305 || statusCode == 307 || statusCode == 308);
}

qint64 QHttpNetworkReply::contentLength() const
{
    return d_func()->contentLength();
}

void QHttpNetworkReply::setContentLength(qint64 length)
{
    Q_D(QHttpNetworkReply);
    d->setContentLength(length);
}

QHttpHeaders QHttpNetworkReply::header() const
{
    return d_func()->parser.headers();
}

QByteArray QHttpNetworkReply::headerField(QByteArrayView name, const QByteArray &defaultValue) const
{
    return d_func()->headerField(name, defaultValue);
}

void QHttpNetworkReply::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    Q_D(QHttpNetworkReply);
    d->setHeaderField(name, data);
}

void QHttpNetworkReply::appendHeaderField(const QByteArray &name, const QByteArray &data)
{
    Q_D(QHttpNetworkReply);
    d->appendHeaderField(name, data);
}

void QHttpNetworkReply::parseHeader(QByteArrayView header)
{
    Q_D(QHttpNetworkReply);
    d->parseHeader(header);
}

QHttpNetworkRequest QHttpNetworkReply::request() const
{
    return d_func()->request;
}

void QHttpNetworkReply::setRequest(const QHttpNetworkRequest &request)
{
    Q_D(QHttpNetworkReply);
    d->request = request;
    d->ssl = request.isSsl();
}

int QHttpNetworkReply::statusCode() const
{
    return d_func()->parser.getStatusCode();
}

void QHttpNetworkReply::setStatusCode(int code)
{
    Q_D(QHttpNetworkReply);
    d->parser.setStatusCode(code);
}

QString QHttpNetworkReply::errorString() const
{
    return d_func()->errorString;
}

QNetworkReply::NetworkError QHttpNetworkReply::errorCode() const
{
    return d_func()->httpErrorCode;
}

QString QHttpNetworkReply::reasonPhrase() const
{
    return d_func()->parser.getReasonPhrase();
}

void QHttpNetworkReply::setReasonPhrase(const QString &reason)
{
    d_func()->parser.setReasonPhrase(reason);
}

void QHttpNetworkReply::setErrorString(const QString &error)
{
    Q_D(QHttpNetworkReply);
    d->errorString = error;
}

int QHttpNetworkReply::majorVersion() const
{
    return d_func()->parser.getMajorVersion();
}

int QHttpNetworkReply::minorVersion() const
{
    return d_func()->parser.getMinorVersion();
}

void QHttpNetworkReply::setMajorVersion(int version)
{
    d_func()->parser.setMajorVersion(version);
}

void QHttpNetworkReply::setMinorVersion(int version)
{
    d_func()->parser.setMinorVersion(version);
}

qint64 QHttpNetworkReply::bytesAvailable() const
{
    Q_D(const QHttpNetworkReply);
    if (d->connection)
        return d->connection->d_func()->uncompressedBytesAvailable(*this);
    else
        return -1;
}

qint64 QHttpNetworkReply::bytesAvailableNextBlock() const
{
    Q_D(const QHttpNetworkReply);
    if (d->connection)
        return d->connection->d_func()->uncompressedBytesAvailableNextBlock(*this);
    else
        return -1;
}

bool QHttpNetworkReply::readAnyAvailable() const
{
    Q_D(const QHttpNetworkReply);
    return (d->responseData.bufferCount() > 0);
}

QByteArray QHttpNetworkReply::readAny()
{
    Q_D(QHttpNetworkReply);
    if (d->responseData.bufferCount() == 0)
        return QByteArray();

    // we'll take the last buffer, so schedule another read from http
    if (d->downstreamLimited && d->responseData.bufferCount() == 1 && !isFinished())
        d->connection->d_func()->readMoreLater(this);
    return d->responseData.read();
}

QByteArray QHttpNetworkReply::readAll()
{
    Q_D(QHttpNetworkReply);
    return d->responseData.readAll();
}

QByteArray QHttpNetworkReply::read(qint64 amount)
{
    Q_D(QHttpNetworkReply);
    return d->responseData.read(amount);
}


qint64 QHttpNetworkReply::sizeNextBlock()
{
    Q_D(QHttpNetworkReply);
    return d->responseData.sizeNextBlock();
}

void QHttpNetworkReply::setDownstreamLimited(bool dsl)
{
    Q_D(QHttpNetworkReply);
    d->downstreamLimited = dsl;
    d->connection->d_func()->readMoreLater(this);
}

void QHttpNetworkReply::setReadBufferSize(qint64 size)
{
    Q_D(QHttpNetworkReply);
    d->readBufferMaxSize = size;
}

bool QHttpNetworkReply::supportsUserProvidedDownloadBuffer()
{
    Q_D(QHttpNetworkReply);
    return !d->isChunked() && !d->autoDecompress &&
            d->bodyLength > 0 && d->parser.getStatusCode() == 200;
}

void QHttpNetworkReply::setUserProvidedDownloadBuffer(char* b)
{
    Q_D(QHttpNetworkReply);
    if (supportsUserProvidedDownloadBuffer())
        d->userProvidedDownloadBuffer = b;
}

char* QHttpNetworkReply::userProvidedDownloadBuffer()
{
    Q_D(QHttpNetworkReply);
    return d->userProvidedDownloadBuffer;
}

void QHttpNetworkReply::abort()
{
    Q_D(QHttpNetworkReply);
    d->state = QHttpNetworkReplyPrivate::Aborted;
}

bool QHttpNetworkReply::isAborted() const
{
    return d_func()->state == QHttpNetworkReplyPrivate::Aborted;
}

bool QHttpNetworkReply::isFinished() const
{
    return d_func()->state == QHttpNetworkReplyPrivate::AllDoneState;
}

bool QHttpNetworkReply::isPipeliningUsed() const
{
    return d_func()->pipeliningUsed;
}

bool QHttpNetworkReply::isHttp2Used() const
{
    return d_func()->h2Used;
}

void QHttpNetworkReply::setHttp2WasUsed(bool h2)
{
    d_func()->h2Used = h2;
}

qint64 QHttpNetworkReply::removedContentLength() const
{
    return d_func()->removedContentLength;
}

bool QHttpNetworkReply::isRedirecting() const
{
    return d_func()->isRedirecting();
}

QHttpNetworkConnection* QHttpNetworkReply::connection()
{
    return d_func()->connection;
}


QHttpNetworkReplyPrivate::QHttpNetworkReplyPrivate(const QUrl &newUrl)
    : QHttpNetworkHeaderPrivate(newUrl)
    , state(NothingDoneState)
    , ssl(false),
      bodyLength(0), contentRead(0), totalProgress(0),
      chunkedTransferEncoding(false),
      connectionCloseEnabled(true),
      forceConnectionCloseEnabled(false),
      lastChunkRead(false),
      currentChunkSize(0), currentChunkRead(0), readBufferMaxSize(0),
      totallyUploadedData(0),
      removedContentLength(-1),
      connection(nullptr),
      autoDecompress(false), responseData(), requestIsPrepared(false)
      ,pipeliningUsed(false), h2Used(false), downstreamLimited(false)
      ,userProvidedDownloadBuffer(nullptr)

{
    QString scheme = newUrl.scheme();
    if (scheme == "preconnect-http"_L1 || scheme == "preconnect-https"_L1)
        // make sure we do not close the socket after preconnecting
        connectionCloseEnabled = false;
}

QHttpNetworkReplyPrivate::~QHttpNetworkReplyPrivate() = default;

void QHttpNetworkReplyPrivate::clearHttpLayerInformation()
{
    state = NothingDoneState;
    bodyLength = 0;
    contentRead = 0;
    totalProgress = 0;
    currentChunkSize = 0;
    currentChunkRead = 0;
    lastChunkRead = false;
    connectionCloseEnabled = true;
    parser.clear();
}

// TODO: Isn't everything HTTP layer related? We don't need to set connection and connectionChannel to 0 at all
void QHttpNetworkReplyPrivate::clear()
{
    connection = nullptr;
    connectionChannel = nullptr;
    autoDecompress = false;
    clearHttpLayerInformation();
}

// QHttpNetworkReplyPrivate
qint64 QHttpNetworkReplyPrivate::bytesAvailable() const
{
    return (state != ReadingDataState ? 0 : fragment.size());
}

bool QHttpNetworkReplyPrivate::isCompressed() const
{
    return QDecompressHelper::isSupportedEncoding(headerField("content-encoding"));
}

bool QHttpNetworkReply::isCompressed() const
{
    Q_D(const QHttpNetworkReply);
    return d->isCompressed();
}

void QHttpNetworkReplyPrivate::removeAutoDecompressHeader()
{
    // The header "Content-Encoding  = gzip" is retained.
    // Content-Length is removed since the actual one sent by the server is for compressed data
    constexpr auto name = QByteArrayView("content-length");
    QByteArray contentLength = parser.firstHeaderField(name);
    bool parseOk = false;
    qint64 value = contentLength.toLongLong(&parseOk);
    if (parseOk) {
        removedContentLength = value;
        parser.removeHeaderField(name);
    }
}

qint64 QHttpNetworkReplyPrivate::readStatus(QIODevice *socket)
{
    if (fragment.isEmpty()) {
        // reserve bytes for the status line. This is better than always append() which reallocs the byte array
        fragment.reserve(32);
    }

    qint64 bytes = 0;
    char c;
    qint64 haveRead = 0;

    do {
        haveRead = socket->read(&c, 1);
        if (haveRead == -1)
            return -1; // unexpected EOF
        else if (haveRead == 0)
            break; // read more later
        else if (haveRead == 1 && fragment.size() == 0 && (c == 11 || c == '\n' || c == '\r' || c == ' ' || c == 31))
            continue; // Ignore all whitespace that was trailing froma previous request on that socket

        bytes++;

        // allow both CRLF & LF (only) line endings
        if (c == '\n') {
            // remove the CR at the end
            if (fragment.endsWith('\r')) {
                fragment.truncate(fragment.size()-1);
            }
            bool ok = parseStatus(fragment);
            state = ReadingHeaderState;
            fragment.clear();
            if (!ok) {
                return -1;
            }
            break;
        } else {
            fragment.append(c);
        }

        // is this a valid reply?
        if (fragment.size() == 5 && !fragment.startsWith("HTTP/")) {
            fragment.clear();
            return -1;
        }
    } while (haveRead == 1);

    return bytes;
}

bool QHttpNetworkReplyPrivate::parseStatus(QByteArrayView status)
{
    return parser.parseStatus(status);
}

qint64 QHttpNetworkReplyPrivate::readHeader(QIODevice *socket)
{
    if (fragment.isEmpty()) {
        // according to http://dev.opera.com/articles/view/mama-http-headers/ the average size of the header
        // block is 381 bytes.
        // reserve bytes. This is better than always append() which reallocs the byte array.
        fragment.reserve(512);
    }

    qint64 bytes = 0;
    char c = 0;
    bool allHeaders = false;
    qint64 haveRead = 0;
    do {
        haveRead = socket->read(&c, 1);
        if (haveRead == 0) {
            // read more later
            break;
        } else if (haveRead == -1) {
            // connection broke down
            return -1;
        } else {
            fragment.append(c);
            bytes++;

            if (c == '\n') {
                // check for possible header endings. As per HTTP rfc,
                // the header endings will be marked by CRLFCRLF. But
                // we will allow CRLFCRLF, CRLFLF, LFCRLF, LFLF
                if (fragment.endsWith("\n\r\n")
                    || fragment.endsWith("\n\n"))
                    allHeaders = true;

                // there is another case: We have no headers. Then the fragment equals just the line ending
                if ((fragment.size() == 2 && fragment.endsWith("\r\n"))
                    || (fragment.size() == 1 && fragment.endsWith("\n")))
                    allHeaders = true;
            }
        }
    } while (!allHeaders && haveRead > 0);

    // we received all headers now parse them
    if (allHeaders) {
        parseHeader(fragment);
        state = ReadingDataState;
        fragment.clear(); // next fragment
        bodyLength = contentLength(); // cache the length

        // cache isChunked() since it is called often
        chunkedTransferEncoding = headerField("transfer-encoding").toLower().contains("chunked");

        // cache isConnectionCloseEnabled since it is called often
        QByteArray connectionHeaderField = headerField("connection");
        // check for explicit indication of close or the implicit connection close of HTTP/1.0
        connectionCloseEnabled = (connectionHeaderField.toLower().contains("close") ||
            headerField("proxy-connection").toLower().contains("close")) ||
            (parser.getMajorVersion() == 1 && parser.getMinorVersion() == 0 &&
            (connectionHeaderField.isEmpty() && !headerField("proxy-connection").toLower().contains("keep-alive")));
    }
    return bytes;
}

void QHttpNetworkReplyPrivate::parseHeader(QByteArrayView header)
{
    parser.parseHeaders(header);
}

void QHttpNetworkReplyPrivate::appendHeaderField(const QByteArray &name, const QByteArray &data)
{
    parser.appendHeaderField(name, data);
}

bool QHttpNetworkReplyPrivate::isChunked()
{
    return chunkedTransferEncoding;
}

bool QHttpNetworkReplyPrivate::isConnectionCloseEnabled()
{
    return connectionCloseEnabled || forceConnectionCloseEnabled;
}

// note this function can only be used for non-chunked, non-compressed with
// known content length
qint64 QHttpNetworkReplyPrivate::readBodyVeryFast(QIODevice *socket, char *b)
{
    // This first read is to flush the buffer inside the socket
    qint64 haveRead = 0;
    haveRead = socket->read(b, bodyLength - contentRead);
    if (haveRead == -1) {
        return -1;
    }
    contentRead += haveRead;

    if (contentRead == bodyLength) {
        state = AllDoneState;
    }

    return haveRead;
}

// note this function can only be used for non-chunked, non-compressed with
// known content length
qint64 QHttpNetworkReplyPrivate::readBodyFast(QIODevice *socket, QByteDataBuffer *rb)
{

    qint64 toBeRead = qMin(socket->bytesAvailable(), bodyLength - contentRead);
    if (readBufferMaxSize)
        toBeRead = qMin(toBeRead, readBufferMaxSize);

    if (!toBeRead)
        return 0;

    QByteArray bd;
    bd.resize(toBeRead);
    qint64 haveRead = socket->read(bd.data(), toBeRead);
    if (haveRead == -1) {
        bd.clear();
        return 0; // ### error checking here;
    }
    bd.resize(haveRead);

    rb->append(bd);

    if (contentRead + haveRead == bodyLength) {
        state = AllDoneState;
    }

    contentRead += haveRead;
    return haveRead;
}


qint64 QHttpNetworkReplyPrivate::readBody(QIODevice *socket, QByteDataBuffer *out)
{
    qint64 bytes = 0;

    if (isChunked()) {
        // chunked transfer encoding (rfc 2616, sec 3.6)
        bytes += readReplyBodyChunked(socket, out);
    } else if (bodyLength > 0) {
        // we have a Content-Length
        bytes += readReplyBodyRaw(socket, out, bodyLength - contentRead);
        if (contentRead + bytes == bodyLength)
            state = AllDoneState;
    } else {
        // no content length. just read what's possible
        bytes += readReplyBodyRaw(socket, out, socket->bytesAvailable());
    }
    contentRead += bytes;
    return bytes;
}

qint64 QHttpNetworkReplyPrivate::readReplyBodyRaw(QIODevice *socket, QByteDataBuffer *out, qint64 size)
{
    // FIXME get rid of this function and just use readBodyFast and give it socket->bytesAvailable()
    qint64 bytes = 0;
    Q_ASSERT(socket);
    Q_ASSERT(out);

    int toBeRead = qMin<qint64>(128*1024, qMin<qint64>(size, socket->bytesAvailable()));

    if (readBufferMaxSize)
        toBeRead = qMin<qint64>(toBeRead, readBufferMaxSize);

    while (toBeRead > 0) {
        QByteArray byteData;
        byteData.resize(toBeRead);
        qint64 haveRead = socket->read(byteData.data(), byteData.size());
        if (haveRead <= 0) {
            // ### error checking here
            byteData.clear();
            return bytes;
        }

        byteData.resize(haveRead);
        out->append(byteData);
        bytes += haveRead;
        size -= haveRead;

        toBeRead = qMin<qint64>(128*1024, qMin<qint64>(size, socket->bytesAvailable()));
    }
    return bytes;

}

qint64 QHttpNetworkReplyPrivate::readReplyBodyChunked(QIODevice *socket, QByteDataBuffer *out)
{
    qint64 bytes = 0;
    while (socket->bytesAvailable()) {

        if (readBufferMaxSize && (bytes > readBufferMaxSize))
            break;

        if (!lastChunkRead && currentChunkRead >= currentChunkSize) {
            // For the first chunk and when we're done with a chunk
            currentChunkSize = 0;
            currentChunkRead = 0;
            if (bytes) {
                // After a chunk
                char crlf[2];
                // read the "\r\n" after the chunk
                qint64 haveRead = socket->read(crlf, 2);
                // FIXME: This code is slightly broken and not optimal. What if the 2 bytes are not available yet?!
                // For nice reasons (the toLong in getChunkSize accepting \n at the beginning
                // it right now still works, but we should definitely fix this.

                if (haveRead != 2)
                    return bytes; // FIXME
                bytes += haveRead;
            }
            // Note that chunk size gets stored in currentChunkSize, what is returned is the bytes read
            bytes += getChunkSize(socket, &currentChunkSize);
            if (currentChunkSize == -1)
                break;
        }
        // if the chunk size is 0, end of the stream
        if (currentChunkSize == 0 || lastChunkRead) {
            lastChunkRead = true;
            // try to read the "\r\n" after the chunk
            char crlf[2];
            qint64 haveRead = socket->read(crlf, 2);
            if (haveRead > 0)
                bytes += haveRead;

            if ((haveRead == 2 && crlf[0] == '\r' && crlf[1] == '\n') || (haveRead == 1 && crlf[0] == '\n'))
                state = AllDoneState;
            else if (haveRead == 1 && crlf[0] == '\r')
                break; // Still waiting for the last \n
            else if (haveRead > 0) {
                // If we read something else then CRLF, we need to close the channel.
                forceConnectionCloseEnabled = true;
                state = AllDoneState;
            }
            break;
        }

        // otherwise, try to begin reading this chunk / to read what is missing for this chunk
        qint64 haveRead = readReplyBodyRaw (socket, out, currentChunkSize - currentChunkRead);
        currentChunkRead += haveRead;
        bytes += haveRead;

        // ### error checking here

    }
    return bytes;
}

qint64 QHttpNetworkReplyPrivate::getChunkSize(QIODevice *socket, qint64 *chunkSize)
{
    qint64 bytes = 0;
    char crlf[2];
    *chunkSize = -1;

    int bytesAvailable = socket->bytesAvailable();
    // FIXME rewrite to permanent loop without bytesAvailable
    while (bytesAvailable > bytes) {
        qint64 sniffedBytes = socket->peek(crlf, 2);
        int fragmentSize = fragment.size();

        // check the next two bytes for a "\r\n", skip blank lines
        if ((fragmentSize && sniffedBytes == 2 && crlf[0] == '\r' && crlf[1] == '\n')
           ||(fragmentSize > 1 && fragment.endsWith('\r')  && crlf[0] == '\n'))
        {
            bytes += socket->read(crlf, 1);     // read the \r or \n
            if (crlf[0] == '\r')
                bytes += socket->read(crlf, 1); // read the \n
            bool ok = false;
            // ignore the chunk-extension
            const auto fragmentView = QByteArrayView(fragment).mid(0, fragment.indexOf(';')).trimmed();
            *chunkSize = fragmentView.toLong(&ok, 16);
            fragment.clear();
            break; // size done
        } else {
            // read the fragment to the buffer
            char c = 0;
            qint64 haveRead = socket->read(&c, 1);
            if (haveRead < 0) {
                return -1; // FIXME
            }
            bytes += haveRead;
            fragment.append(c);
        }
    }

    return bytes;
}

bool QHttpNetworkReplyPrivate::isRedirecting() const
{
    // We're in the process of redirecting - if the HTTP status code says so and
    // followRedirect is switched on
    return (QHttpNetworkReply::isHttpRedirect(parser.getStatusCode())
            && request.isFollowRedirects());
}

bool QHttpNetworkReplyPrivate::shouldEmitSignals()
{
    // for 401 & 407 don't emit the data signals. Content along with these
    // responses are sent only if the authentication fails.
    return parser.getStatusCode() != 401 && parser.getStatusCode() != 407;
}

bool QHttpNetworkReplyPrivate::expectContent()
{
    int statusCode = parser.getStatusCode();
    // check whether we can expect content after the headers (rfc 2616, sec4.4)
    if ((statusCode >= 100 && statusCode < 200)
        || statusCode == 204 || statusCode == 304)
        return false;
    if (request.operation() == QHttpNetworkRequest::Head)
        return false; // no body expected for HEAD request
    qint64 expectedContentLength = contentLength();
    if (expectedContentLength == 0)
        return false;
    if (expectedContentLength == -1 && bodyLength == 0) {
        // The content-length header was stripped, but its value was 0.
        // This would be the case for an explicitly zero-length compressed response.
        return false;
    }
    return true;
}

void QHttpNetworkReplyPrivate::eraseData()
{
    responseData.clear();
}


// SSL support below
#ifndef QT_NO_SSL

QSslConfiguration QHttpNetworkReply::sslConfiguration() const
{
    Q_D(const QHttpNetworkReply);

    if (!d->connectionChannel)
        return QSslConfiguration();

    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(d->connectionChannel->socket);
    if (!sslSocket)
        return QSslConfiguration();

    return sslSocket->sslConfiguration();
}

void QHttpNetworkReply::setSslConfiguration(const QSslConfiguration &config)
{
    Q_D(QHttpNetworkReply);
    if (d->connection)
        d->connection->setSslConfiguration(config);
}

void QHttpNetworkReply::ignoreSslErrors()
{
    Q_D(QHttpNetworkReply);
    if (d->connection)
        d->connection->ignoreSslErrors();
}

void QHttpNetworkReply::ignoreSslErrors(const QList<QSslError> &errors)
{
    Q_D(QHttpNetworkReply);
    if (d->connection)
        d->connection->ignoreSslErrors(errors);
}


#endif //QT_NO_SSL


QT_END_NAMESPACE

#include "moc_qhttpnetworkreply_p.cpp"
