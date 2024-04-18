// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "httptestserver_p.h"

#include <QtNetwork/qtcpsocket.h>

#include <QtCore/qcoreapplication.h>

#include <private/qlocale_p.h>

using namespace Qt::StringLiterals;

static constexpr char CRLF[] = "\r\n";

HttpTestServer::HttpTestServer(QObject *parent) : QTcpServer(parent)
{
    QObject::connect(this, &QTcpServer::newConnection, this, &HttpTestServer::handleConnected);
    const auto ok = listen(QHostAddress::LocalHost);
    Q_ASSERT(ok);
};

HttpTestServer::~HttpTestServer()
{
    if (isListening())
        close();
}

QUrl HttpTestServer::url()
{
    return QUrl(u"http://127.0.0.1:%1"_s.arg(serverPort()));
}

void HttpTestServer::handleConnected()
{
    Q_ASSERT(!m_socket); // No socket must exist previously, this is a single-connection server
    m_socket = nextPendingConnection();
    Q_ASSERT(m_socket);
    QObject::connect(m_socket, &QTcpSocket::readyRead,
                     this, &HttpTestServer::handleDataAvailable);
}

void HttpTestServer::handleDataAvailable()
{
    Q_ASSERT(m_socket);
    bool ok = true;

    // Parse the incoming request data into the HttpData object
    while (m_socket->bytesAvailable()) {
        if (state == State::ReadingMethod && !(ok = readMethod(m_socket)))
            qWarning("Invalid Method");
        if (ok && state == State::ReadingUrl && !(ok = readUrl(m_socket)))
            qWarning("Invalid URL");
        if (ok && state == State::ReadingStatus && !(ok = readStatus(m_socket)))
            qWarning("Invalid Status");
        if (ok && state == State::ReadingHeader && !(ok = readHeaders(m_socket)))
            qWarning("Invalid Header");
        if (ok && state == State::ReadingBody && !(ok = readBody(m_socket)))
            qWarning("Invalid Body");
    } // while bytes available

    Q_ASSERT(ok);
    Q_ASSERT(m_handler);
    Q_ASSERT(state == State::AllDone);

    if (auto values = m_request.headers.values(
                    QHttpHeaders::WellKnownHeader::Host); !values.empty()) {
        const auto parts = values.first().split(':');
        m_request.url.setHost(parts.at(0));
        if (parts.size() == 2)
            m_request.url.setPort(parts.at(1).toUInt());
    }
    HttpData response;
    ResponseControl control;
    // Inform the testcase about request and ask for response data
    m_handler(m_request, response, control);

    QByteArray responseMessage;
    responseMessage += "HTTP/1.1 ";
    responseMessage += QByteArray::number(response.status);
    responseMessage += CRLF;
    // Insert headers if any
    for (const auto &[name,value] : response.headers.toListOfPairs()) {
        responseMessage += name;
        responseMessage += ": ";
        responseMessage += value;
        responseMessage += CRLF;
    }
    responseMessage += CRLF;
    /*
    qDebug() << "HTTPTestServer received request"
             << "\nMethod:" << m_request.method
             << "\nHeaders:" << m_request.headers
             << "\nBody:" << m_request.body;
    */
    if (control.respond) {
        if (control.responseChunkSize <= 0) {
            responseMessage += response.body;
            // qDebug() << "HTTPTestServer response:" << responseMessage;
            m_socket->write(responseMessage);
        } else {
            // Respond in chunks, first write the headers
            // qDebug() << "HTTPTestServer response:" << responseMessage;
            m_socket->write(responseMessage);
            // Then write bodydata in chunks, while allowing the testcase to process as well
            QByteArray chunk;
            while (!response.body.isEmpty()) {
                chunk = response.body.left(control.responseChunkSize);
                response.body.remove(0, control.responseChunkSize);
                // qDebug() << "SERVER writing chunk" << chunk;
                m_socket->write(chunk);
                m_socket->flush();
                m_socket->waitForBytesWritten();
                // Process events until testcase indicates it's ready for next chunk.
                // This way we can control the bytes the testcase gets in each chunk
                control.readyForNextChunk = false;
                while (!control.readyForNextChunk)
                    QCoreApplication::processEvents();
            }
        }
    }
    m_socket->disconnectFromHost();
    m_request = {};
    m_socket = nullptr; // deleted by QTcpServer during destruction
    state = State::ReadingMethod;
    fragment.clear();
}

bool HttpTestServer::readMethod(QTcpSocket *socket)
{
    bool finished = false;
    while (socket->bytesAvailable() && !finished) {
        const auto c = socket->read(1).at(0);
        if (ascii_isspace(c))
            finished = true;
        else if (std::isupper(c) && fragment.size() < 8)
            fragment += c;
        else
            return false;
    }
    if (finished) {
        if (fragment == "HEAD")
            method = Method::Head;
        else if (fragment == "GET")
            method = Method::Get;
        else if (fragment == "PUT")
            method = Method::Put;
        else if (fragment == "PATCH")
            method = Method::Patch;
        else if (fragment == "POST")
            method = Method::Post;
        else if (fragment == "DELETE")
            method = Method::Delete;
        else if (fragment == "FOOBAR") // used by custom verb/method tests
            method = Method::Custom;
        else
            qWarning("Invalid operation %s", fragment.data());

        state = State::ReadingUrl;
        m_request.method = fragment;
        fragment.clear();

        return method != Method::Unknown;
    }
    return true;
}

bool HttpTestServer::readUrl(QTcpSocket *socket)
{
    bool finished = false;
    while (socket->bytesAvailable() && !finished) {
        const auto c = socket->read(1).at(0);
        if (std::isspace(c))
            finished = true;
        else
            fragment += c;
    }
    if (finished) {
        if (!fragment.startsWith('/')) {
            qWarning("Invalid URL path %s", fragment.constData());
            return false;
        }
        m_request.url = QStringLiteral("http://127.0.0.1:") + QString::number(m_request.port) +
                QString::fromUtf8(fragment);
        state = State::ReadingStatus;
        if (!m_request.url.isValid()) {
            qWarning("Invalid URL %s", fragment.constData());
            return false;
        }
        fragment.clear();
    }
    return true;
}

bool HttpTestServer::readStatus(QTcpSocket *socket)
{
    bool finished = false;
    while (socket->bytesAvailable() && !finished) {
        fragment += socket->read(1);
        if (fragment.endsWith(CRLF)) {
            finished = true;
            fragment.resize(fragment.size() - 2);
        }
    }
    if (finished) {
        if (!std::isdigit(fragment.at(fragment.size() - 3)) ||
            fragment.at(fragment.size() - 2) != '.' ||
            !std::isdigit(fragment.at(fragment.size() - 1))) {
            qWarning("Invalid version");
            return false;
        }
        m_request.version = std::pair(fragment.at(fragment.size() - 3) - '0',
                                      fragment.at(fragment.size() - 1) - '0');
        state = State::ReadingHeader;
        fragment.clear();
    }
    return true;
}

bool HttpTestServer::readHeaders(QTcpSocket *socket)
{
    while (socket->bytesAvailable()) {
        fragment += socket->read(1);
        if (fragment.endsWith(CRLF)) {
            if (fragment == CRLF) {
                state = State::ReadingBody;
                fragment.clear();
                return true;
            } else {
                fragment.chop(2);
                const int index = fragment.indexOf(':');
                if (index == -1)
                    return false;

                QByteArray key = fragment.sliced(0, index).trimmed();
                QByteArray value = fragment.sliced(index + 1).trimmed();
                m_request.headers.append(key, value);
                fragment.clear();
            }
        }
    }
    return true;
}

bool HttpTestServer::readBody(QTcpSocket *socket)
{
    qint64 bytesLeft = 0;
    if (auto values = m_request.headers.values(
                QHttpHeaders::WellKnownHeader::ContentLength); !values.empty()) {
        bool conversionResult;
        bytesLeft = values.first().toInt(&conversionResult);
        if (!conversionResult)
            return false;
        fragment.resize(bytesLeft);
    }
    while (bytesLeft) {
        qint64 got = socket->read(&fragment.data()[fragment.size() - bytesLeft], bytesLeft);
        if (got < 0)
            return false; // error
        bytesLeft -= got;
        if (bytesLeft)
            qApp->processEvents();
    }
    fragment.swap(m_request.body);
    state = State::AllDone;
    return true;
}

