/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef HTTP2SRV_H
#define HTTP2SRV_H

#include <QtNetwork/private/qhttpnetworkrequest_p.h>
#include <QtNetwork/private/qhttpnetworkreply_p.h>
#include <QtNetwork/private/http2protocol_p.h>
#include <QtNetwork/private/http2frames_p.h>
#include <QtNetwork/private/hpack_p.h>

#include <QtNetwork/qabstractsocket.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qscopedpointer.h>
#include <QtNetwork/qtcpserver.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qglobal.h>

#include <vector>
#include <map>
#include <set>

QT_BEGIN_NAMESPACE

// At the moment we do not have any public API parsing HTTP headers. Even worse -
// the code that can do this exists only in QHttpNetworkReplyPrivate class.
// To be able to access reply's d_func() we have these classes:
class Http11ReplyPrivate : public QHttpNetworkReplyPrivate
{
};

class Http11Reply : public QHttpNetworkReply
{
public:
    Q_DECLARE_PRIVATE(Http11Reply)
};

class Http2Server : public QTcpServer
{
    Q_OBJECT
public:
    Http2Server(bool clearText, const Http2::RawSettings &serverSettings,
                const Http2::RawSettings &clientSettings);

    ~Http2Server();

    // To be called before server started:
    void enablePushPromise(bool enabled, const QByteArray &path = QByteArray());
    void setResponseBody(const QByteArray &body);
    void emulateGOAWAY(int timeout);
    void redirectOpenStream(quint16 targetPort);

    // Invokables, since we can call them from the main thread,
    // but server (can) work on its own thread.
    Q_INVOKABLE void startServer();
    bool sendProtocolSwitchReply();
    Q_INVOKABLE void sendServerSettings();
    Q_INVOKABLE void sendGOAWAY(quint32 streamID, quint32 error,
                                quint32 lastStreamID);
    Q_INVOKABLE void sendRST_STREAM(quint32 streamID, quint32 error);
    Q_INVOKABLE void sendDATA(quint32 streamID, quint32 windowSize);
    Q_INVOKABLE void sendWINDOW_UPDATE(quint32 streamID, quint32 delta);

    Q_INVOKABLE void handleProtocolUpgrade();
    Q_INVOKABLE void handleConnectionPreface();
    Q_INVOKABLE void handleIncomingFrame();
    Q_INVOKABLE void handleSETTINGS();
    Q_INVOKABLE void handleDATA();
    Q_INVOKABLE void handleWINDOW_UPDATE();

    Q_INVOKABLE void sendResponse(quint32 streamID, bool emptyBody);

private:
    void processRequest();

Q_SIGNALS:
    void serverStarted(quint16 port);
    // Error/success notifications:
    void clientPrefaceOK();
    void clientPrefaceError();
    void serverSettingsAcked();
    void invalidFrame();
    void invalidRequest(quint32 streamID);
    void decompressionFailed(quint32 streamID);
    void receivedRequest(quint32 streamID);
    void receivedData(quint32 streamID);
    void windowUpdate(quint32 streamID);

private slots:
    void connectionEstablished();
    void readReady();

private:
    void incomingConnection(qintptr socketDescriptor) override;

    quint32 clientSetting(Http2::Settings identifier, quint32 defaultValue);
    bool readMethodLine();
    bool verifyProtocolUpgradeRequest();
    void triggerGOAWAYEmulation();

    QScopedPointer<QAbstractSocket> socket;

    // Connection preface:
    bool waitingClientPreface = false;
    bool waitingClientSettings = false;
    bool settingsSent = false;
    bool waitingClientAck = false;

    Http2::RawSettings serverSettings;
    Http2::RawSettings expectedClientSettings;

    bool connectionError = false;

    Http2::FrameReader reader;
    Http2::Frame inboundFrame;
    Http2::FrameWriter writer;

    using FrameSequence = std::vector<Http2::Frame>;
    FrameSequence continuedRequest;

    std::map<quint32, quint32> streamWindows;

    HPack::Decoder decoder{HPack::FieldLookupTable::DefaultSize};
    HPack::Encoder encoder{HPack::FieldLookupTable::DefaultSize, true};

    using Http2Requests = std::map<quint32, HPack::HttpHeader>;
    Http2Requests activeRequests;
    // 'remote half-closed' streams to keep
    // track of streams with END_STREAM set:
    std::set<quint32> closedStreams;
    // streamID + offset in response body to send.
    std::map<quint32, quint32> suspendedStreams;

    // We potentially reset this once (see sendServerSettings)
    // and do not change later:
    quint32 sessionRecvWindowSize = Http2::defaultSessionWindowSize;
    // This changes in the range [0, sessionRecvWindowSize]
    // while handling DATA frames:
    quint32 sessionCurrRecvWindow = sessionRecvWindowSize;
    // This we potentially update only once (sendServerSettings).
    quint32 streamRecvWindowSize = Http2::defaultSessionWindowSize;

    QByteArray responseBody;
    bool clearTextHTTP2 = false;
    bool pushPromiseEnabled = false;
    quint32 lastPromisedStream = 0;
    QByteArray pushPath;

    bool testingGOAWAY = false;
    int goawayTimeout = 0;

    // Clear text HTTP/2, we have to deal with the protocol upgrade request
    // from the initial HTTP/1.1 request.
    bool upgradeProtocol = false;
    QByteArray requestLine;
    QHttpNetworkRequest::Operation requestType;
    // We need QHttpNetworkReply (actually its private d-object) to handle the
    // first HTTP/1.1 request. QHttpNetworkReplyPrivate does parsing + in case
    // of POST it is also reading the body for us.
    QScopedPointer<Http11Reply> protocolUpgradeHandler;
    // We need it for PUSH_PROMISE, with the correct port number appended,
    // when replying to essentially 1.1 request.
    QByteArray authority;
    // Redirect, with status code 308, as soon as we've seen headers, while client
    // may still be sending DATA frames.  See tst_Http2::earlyResponse().
    bool redirectWhileReading = false;
    quint16 targetPort = 0;
protected slots:
    void ignoreErrorSlot();
};

QT_END_NAMESPACE

#endif

