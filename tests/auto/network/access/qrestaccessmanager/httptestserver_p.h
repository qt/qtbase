// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QRESTACCESSSMANAGER_HTTPTESTSERVER_P_H
#define QRESTACCESSSMANAGER_HTTPTESTSERVER_P_H

#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qhttpheaders.h>

#include <QtCore/qmap.h>
#include <QtCore/qurl.h>

#include <functional>

// This struct is used for parsing the incoming network request data into, as well
// as getting the response data from the testcase
struct HttpData {
    QUrl url;
    int status = 0;
    QByteArray body;
    QByteArray method;
    quint16 port = 0;
    QPair<quint8, quint8> version;
    QHttpHeaders headers;
};

struct ResponseControl
{
    bool respond = true;
    qsizetype responseChunkSize = -1;
    bool readyForNextChunk = true;
};

// Simple HTTP server. Currently supports only one concurrent connection
class HttpTestServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit HttpTestServer(QObject *parent = nullptr);
    ~HttpTestServer() override;

    // Returns this server's URL for the testcase to send requests to
    QUrl url();

    enum class State {
        ReadingMethod,
        ReadingUrl,
        ReadingStatus,
        ReadingHeader,
        ReadingBody,
        AllDone
    } state = State::ReadingMethod;

    enum class Method {
        Unknown,
        Head,
        Get,
        Put,
        Patch,
        Post,
        Delete,
        Custom,
    } method = Method::Unknown;

    // Parsing helpers for incoming data => HttpData
    bool readMethod(QTcpSocket *socket);
    bool readUrl(QTcpSocket *socket);
    bool readStatus(QTcpSocket *socket);
    bool readHeaders(QTcpSocket *socket);
    bool readBody(QTcpSocket *socket);
    // Parsing-time buffer in case data is received a small chunk at a time (readyRead())
    QByteArray fragment;

    // Settable callback for testcase. Gives the received request data, and takes in response data
    using Handler = std::function<void(const HttpData &request, HttpData &response,
                                       ResponseControl &control)>;
    void setHandler(Handler handler) { m_handler = std::move(handler); }

private slots:
    void handleConnected();
    void handleDataAvailable();

private:
    QTcpSocket *m_socket = nullptr;
    HttpData m_request;
    Handler m_handler = nullptr;
};

#endif // QRESTACCESSSMANAGER_HTTPTESTSERVER_P_H
