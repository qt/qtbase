// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <QtNetwork/private/qhttp2connection_p.h>
#include <QtNetwork/private/hpack_p.h>
#include <QtNetwork/private/bitstreams_p.h>

#include <limits>

using namespace Qt::StringLiterals;

class tst_QHttp2Connection : public QObject
{
    Q_OBJECT

private slots:
    void construct();
    void constructStream();
    void testSETTINGSFrame();
    void maxHeaderTableSize();
    void testPING();
    void testRSTServerSide();
    void testRSTClientSide();
    void testRSTReplyOnDATAEND();
    void resetAfterClose();
    void testBadFrameSize_data();
    void testBadFrameSize();
    void testDataFrameAfterRSTIncoming();
    void testDataFrameAfterRSTOutgoing();
    void headerFrameAfterRSTOutgoing_data();
    void headerFrameAfterRSTOutgoing();
    void connectToServer();
    void WINDOW_UPDATE();
    void testCONTINUATIONFrame();

private:
    enum PeerType { Client, Server };
    [[nodiscard]] auto makeFakeConnectedSockets();
    [[nodiscard]] auto getRequiredHeaders();
    [[nodiscard]] QHttp2Connection *makeHttp2Connection(QIODevice *socket,
                                                        QHttp2Configuration config, PeerType type);
    [[nodiscard]] bool waitForSettingsExchange(QHttp2Connection *client, QHttp2Connection *server);
};

class IOBuffer : public QIODevice
{
    Q_OBJECT
public:
    IOBuffer(QObject *parent, std::shared_ptr<QBuffer> _in, std::shared_ptr<QBuffer> _out)
        : QIODevice(parent), in(std::move(_in)), out(std::move(_out))
    {
        connect(in.get(), &QIODevice::readyRead, this, &IOBuffer::readyRead);
        connect(out.get(), &QIODevice::bytesWritten, this, &IOBuffer::bytesWritten);
        connect(out.get(), &QIODevice::aboutToClose, this, &IOBuffer::readChannelFinished);
        connect(out.get(), &QIODevice::aboutToClose, this, &IOBuffer::aboutToClose);
    }

    bool open(OpenMode mode) override
    {
        QIODevice::open(mode);
        Q_ASSERT(in->isOpen());
        Q_ASSERT(out->isOpen());
        return false;
    }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return in->pos() - readHead; }
    qint64 bytesToWrite() const override { return 0; }

    qint64 readData(char *data, qint64 maxlen) override
    {
        qint64 temp = in->pos();
        in->seek(readHead);
        qint64 res = in->read(data, std::min(maxlen, temp - readHead));
        readHead += res;
        if (readHead == temp) {
            // Reached end of buffer, reset
            in->seek(0);
            in->buffer().resize(0);
            readHead = 0;
        } else {
            in->seek(temp);
        }
        return res;
    }

    qint64 writeData(const char *data, qint64 len) override
    {
        return out->write(data, len);
    }

    std::shared_ptr<QBuffer> in;
    std::shared_ptr<QBuffer> out;

    qint64 readHead = 0;
};

auto tst_QHttp2Connection::makeFakeConnectedSockets()
{
    auto clientIn = std::make_shared<QBuffer>();
    auto serverIn = std::make_shared<QBuffer>();
    clientIn->open(QIODevice::ReadWrite);
    serverIn->open(QIODevice::ReadWrite);

    auto client = std::make_unique<IOBuffer>(this, clientIn, serverIn);
    auto server = std::make_unique<IOBuffer>(this, std::move(serverIn), std::move(clientIn));

    client->open(QIODevice::ReadWrite);
    server->open(QIODevice::ReadWrite);

    return std::pair{ std::move(client), std::move(server) };
}

auto tst_QHttp2Connection::getRequiredHeaders()
{
    return HPack::HttpHeader{
        { ":authority", "example.com" },
        { ":method", "GET" },
        { ":path", "/" },
        { ":scheme", "https" },
    };
}

QHttp2Connection *tst_QHttp2Connection::makeHttp2Connection(QIODevice *socket,
                                                            QHttp2Configuration config,
                                                            PeerType type)
{
    QHttp2Connection *connection = nullptr;
    if (type == PeerType::Server)
        connection = QHttp2Connection::createDirectServerConnection(socket, config);
    else
        connection = QHttp2Connection::createDirectConnection(socket, config);
    connect(socket, &QIODevice::readyRead, connection, &QHttp2Connection::handleReadyRead);
    return connection;
}

bool tst_QHttp2Connection::waitForSettingsExchange(QHttp2Connection *client,
                                                   QHttp2Connection *server)
{
    bool settingsFrameReceived = false;
    bool serverSettingsFrameReceived = false;

    QMetaObject::Connection c = connect(client, &QHttp2Connection::settingsFrameReceived, client,
                                        [&settingsFrameReceived]() {
                                            settingsFrameReceived = true;
                                        });
    QMetaObject::Connection s = connect(server, &QHttp2Connection::settingsFrameReceived, server,
                                        [&serverSettingsFrameReceived]() {
                                            serverSettingsFrameReceived = true;
                                        });

    client->handleReadyRead(); // handle incoming frames, send response

    bool success = QTest::qWaitFor([&]() {
        return settingsFrameReceived && serverSettingsFrameReceived
                && !client->waitingForSettingsACK && !server->waitingForSettingsACK;
    });

    disconnect(c);
    disconnect(s);

    return success;
}

void tst_QHttp2Connection::construct()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    auto *connection = QHttp2Connection::createDirectConnection(&buffer, {});
    QVERIFY(!connection->isGoingAway());
    QCOMPARE(connection->maxConcurrentStreams(), 100u);
    QCOMPARE(connection->maxHeaderListSize(), std::numeric_limits<quint32>::max());
    QVERIFY(!connection->isUpgradedConnection());
    QVERIFY(!connection->getStream(1)); // No stream has been created yet

    auto *upgradedConnection = QHttp2Connection::createUpgradedConnection(&buffer, {});
    QVERIFY(upgradedConnection->isUpgradedConnection());
    // Stream 1 is created by default for an upgraded connection
    QVERIFY(upgradedConnection->getStream(1));
}

void tst_QHttp2Connection::constructStream()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    auto connection = QHttp2Connection::createDirectConnection(&buffer, {});
    QHttp2Stream *stream = connection->createStream().unwrap();
    QVERIFY(stream);
    QCOMPARE(stream->isPromisedStream(), false);
    QCOMPARE(stream->isActive(), false);
    QCOMPARE(stream->RST_STREAMCodeReceived(), 0u);
    QCOMPARE(stream->streamID(), 1u);
    QCOMPARE(stream->receivedHeaders(), {});
    QCOMPARE(stream->state(), QHttp2Stream::State::Idle);
    QCOMPARE(stream->isUploadBlocked(), false);
    QCOMPARE(stream->isUploadingDATA(), false);
}

void tst_QHttp2Connection::testSETTINGSFrame()
{
    constexpr qint32 PrefaceLength = 24;
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QHttp2Configuration config;
    constexpr quint32 MaxFrameSize = 16394;
    constexpr bool ServerPushEnabled = false;
    constexpr quint32 StreamReceiveWindowSize = 50000;
    constexpr quint32 SessionReceiveWindowSize = 50001;
    config.setMaxFrameSize(MaxFrameSize);
    config.setServerPushEnabled(ServerPushEnabled);
    config.setStreamReceiveWindowSize(StreamReceiveWindowSize);
    config.setSessionReceiveWindowSize(SessionReceiveWindowSize);
    auto connection = QHttp2Connection::createDirectConnection(&buffer, config);
    Q_UNUSED(connection);
    QHttp2Stream *stream = connection->createStream().unwrap();
    QVERIFY(stream);
    QCOMPARE_GE(buffer.size(), PrefaceLength);

    // Preface
    QByteArray preface = buffer.data().first(PrefaceLength);
    QCOMPARE(preface, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n");

    // SETTINGS
    buffer.seek(PrefaceLength);
    const quint32 maxSize = buffer.size() - PrefaceLength;
    Http2::FrameReader reader;
    Http2::FrameStatus status = reader.read(buffer);
    QCOMPARE(status, Http2::FrameStatus::goodFrame);
    Http2::Frame f = reader.inboundFrame();
    QCOMPARE(f.type(), Http2::FrameType::SETTINGS);
    QCOMPARE_LT(f.payloadSize(), maxSize);

    const qint32 settingsReceived = f.dataSize() / 6;
    QCOMPARE_GT(settingsReceived, 0);
    QCOMPARE_LE(settingsReceived, 6);

    struct ExpectedSetting
    {
        Http2::Settings identifier;
        quint32 value;
    };
    // Commented-out settings are not sent since they are defaults
    ExpectedSetting expectedSettings[]{
        // { Http2::Settings::HEADER_TABLE_SIZE_ID, HPack::FieldLookupTable::DefaultSize },
        { Http2::Settings::ENABLE_PUSH_ID, ServerPushEnabled ? 1 : 0 },
        // { Http2::Settings::MAX_CONCURRENT_STREAMS_ID, Http2::maxConcurrentStreams },
        { Http2::Settings::INITIAL_WINDOW_SIZE_ID, StreamReceiveWindowSize },
        { Http2::Settings::MAX_FRAME_SIZE_ID, MaxFrameSize },
        // { Http2::Settings::MAX_HEADER_LIST_SIZE_ID, ??? },
    };

    QCOMPARE(quint32(settingsReceived), std::size(expectedSettings));
    for (qint32 i = 0; i < settingsReceived; ++i) {
        const uchar *it = f.dataBegin() + i * 6;
        const quint16 ident = qFromBigEndian<quint16>(it);
        const quint32 intVal = qFromBigEndian<quint32>(it + 2);

        ExpectedSetting expectedSetting = expectedSettings[i];
        QVERIFY2(ident == quint16(expectedSetting.identifier),
                 qPrintable("ident: %1, expected: %2, index: %3"_L1
                                    .arg(QString::number(ident),
                                         QString::number(quint16(expectedSetting.identifier)),
                                         QString::number(i))));
        QVERIFY2(intVal == expectedSetting.value,
                 qPrintable("intVal: %1, expected: %2, index: %3"_L1
                                    .arg(QString::number(intVal),
                                         QString::number(expectedSetting.value),
                                         QString::number(i))));
    }
}

void tst_QHttp2Connection::maxHeaderTableSize()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QSignalSpy incomingRequestSpy(serverConnection, &QHttp2Connection::newIncomingStream);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    // Test defaults:
    // encoder:
    QCOMPARE(connection->encoder.dynamicTableSize(), 0u);
    QCOMPARE(connection->encoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(connection->encoder.dynamicTableCapacity(), 4096u);
    QCOMPARE(serverConnection->encoder.dynamicTableSize(), 0u);
    QCOMPARE(serverConnection->encoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(serverConnection->encoder.dynamicTableCapacity(), 4096u);

    // decoder:
    QCOMPARE(connection->decoder.dynamicTableSize(), 0u);
    QCOMPARE(connection->decoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(connection->decoder.dynamicTableCapacity(), 4096u);
    QCOMPARE(serverConnection->decoder.dynamicTableSize(), 0u);
    QCOMPARE(serverConnection->decoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(serverConnection->decoder.dynamicTableCapacity(), 4096u);

    // Send a HEADER block with a custom header, to add something to the dynamic table:
    HPack::HttpHeader headers = getRequiredHeaders();
    headers.emplace_back("x-test", "test");
    clientStream->sendHEADERS(headers, true);

    QVERIFY(incomingRequestSpy.wait());

    // Test that the size has been updated:
    // encoder:
    QCOMPARE_GT(connection->encoder.dynamicTableSize(), 0u); // now > 0
    QCOMPARE(connection->encoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(connection->encoder.dynamicTableCapacity(), 4096u);
    QCOMPARE(serverConnection->encoder.dynamicTableSize(), 0u);
    QCOMPARE(serverConnection->encoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(serverConnection->encoder.dynamicTableCapacity(), 4096u);

    // decoder:
    QCOMPARE(connection->decoder.dynamicTableSize(), 0u);
    QCOMPARE(connection->decoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(connection->decoder.dynamicTableCapacity(), 4096u);
    QCOMPARE_GT(serverConnection->decoder.dynamicTableSize(), 0u); // now > 0
    QCOMPARE(serverConnection->decoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(serverConnection->decoder.dynamicTableCapacity(), 4096u);

    const quint32 initialTableSize = connection->encoder.dynamicTableSize();
    QCOMPARE(initialTableSize, serverConnection->decoder.dynamicTableSize());

    // Notify from server that we want a smaller size, reset to 0 first:
    for (quint32 nextSize : {0, 2048}) {
        // @note: currently we don't have an API for this so just do it by hand:
        serverConnection->waitingForSettingsACK = true;
        using namespace Http2;
        FrameWriter builder(FrameType::SETTINGS, FrameFlag::EMPTY, connectionStreamID);
        builder.append(Settings::HEADER_TABLE_SIZE_ID);
        builder.append(nextSize);
        builder.write(*server->out);

        QVERIFY(QTest::qWaitFor([&]() { return !serverConnection->waitingForSettingsACK; }));
    }

    // Now we have to send another HEADER block without extra field, so we can see that the size of
    // the dynamic table has decreased after we cleared it:
    headers = getRequiredHeaders();
    QHttp2Stream *clientStream2 = connection->createStream().unwrap();
    clientStream2->sendHEADERS(headers, true);

    QVERIFY(incomingRequestSpy.wait());

    // Test that the size has been updated:
    // encoder:
    QCOMPARE_LT(connection->encoder.dynamicTableSize(), initialTableSize);

    QCOMPARE(connection->encoder.maxDynamicTableCapacity(), 2048u);
    QCOMPARE(connection->encoder.dynamicTableCapacity(), 2048u);
    QCOMPARE(serverConnection->encoder.dynamicTableSize(), 0u);
    QCOMPARE(serverConnection->encoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(serverConnection->encoder.dynamicTableCapacity(), 4096u);

    // decoder:
    QCOMPARE(connection->decoder.dynamicTableSize(), 0u);
    QCOMPARE(connection->decoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(connection->decoder.dynamicTableCapacity(), 4096u);

    QCOMPARE_LT(serverConnection->decoder.dynamicTableSize(), initialTableSize);
    // If we add an API for this then this should also be updated at some stage:
    QCOMPARE(serverConnection->decoder.maxDynamicTableCapacity(), 4096u);
    QCOMPARE(serverConnection->decoder.dynamicTableCapacity(), 2048u);

    quint32 newTableSize = connection->encoder.dynamicTableSize();
    QCOMPARE(newTableSize, serverConnection->decoder.dynamicTableSize());
}

void tst_QHttp2Connection::testPING()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy serverPingSpy{ serverConnection, &QHttp2Connection::pingFrameReceived };
    QSignalSpy clientPingSpy{ connection, &QHttp2Connection::pingFrameReceived };

    QByteArray data{"pingpong"};
    connection->sendPing(data);

    QVERIFY(serverPingSpy.wait());
    QVERIFY(clientPingSpy.wait());

    QCOMPARE(serverPingSpy.last().at(0).toInt(), int(QHttp2Connection::PingState::Ping));
    QCOMPARE(clientPingSpy.last().at(0).toInt(), int(QHttp2Connection::PingState::PongSignatureIdentical));

    serverConnection->sendPing();

    QVERIFY(clientPingSpy.wait());
    QVERIFY(serverPingSpy.wait());

    QCOMPARE(clientPingSpy.last().at(0).toInt(), int(QHttp2Connection::PingState::Ping));
    QCOMPARE(serverPingSpy.last().at(0).toInt(), int(QHttp2Connection::PingState::PongSignatureIdentical));
}


void tst_QHttp2Connection::testRSTServerSide()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };

    QSignalSpy clientHeaderReceivedSpy{ clientStream, &QHttp2Stream::headersReceived };
    HPack::HttpHeader headers = getRequiredHeaders();
    clientStream->sendHEADERS(headers, false);

    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    QCOMPARE(clientStream->streamID(), serverStream->streamID());

    QSignalSpy rstClientSpy{ clientStream, &QHttp2Stream::rstFrameReceived };
    QSignalSpy rstServerSpy{ serverStream, &QHttp2Stream::rstFrameReceived };

    QCOMPARE(clientStream->state(), QHttp2Stream::State::Open);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::Open);

    serverStream->sendRST_STREAM(Http2::INTERNAL_ERROR);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::Closed);
    QVERIFY(rstClientSpy.wait());
    QCOMPARE(rstServerSpy.count(), 0);
    QCOMPARE(clientStream->state(), QHttp2Stream::State::Closed);
}

void tst_QHttp2Connection::testRSTClientSide()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };

    QSignalSpy clientHeaderReceivedSpy{ clientStream, &QHttp2Stream::headersReceived };
    HPack::HttpHeader headers = getRequiredHeaders();
    clientStream->sendHEADERS(headers, false);

    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    QCOMPARE(clientStream->streamID(), serverStream->streamID());

    QSignalSpy rstClientSpy{ clientStream, &QHttp2Stream::rstFrameReceived };
    QSignalSpy rstServerSpy{ serverStream, &QHttp2Stream::rstFrameReceived };

    QCOMPARE(clientStream->state(), QHttp2Stream::State::Open);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::Open);

    clientStream->sendRST_STREAM(Http2::INTERNAL_ERROR);
    QCOMPARE(clientStream->state(), QHttp2Stream::State::Closed);
    QVERIFY(rstServerSpy.wait());
    QCOMPARE(rstClientSpy.count(), 0);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::Closed);
}

void tst_QHttp2Connection::testRSTReplyOnDATAEND()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };

    QSignalSpy clientHeaderReceivedSpy{ clientStream, &QHttp2Stream::headersReceived };
    HPack::HttpHeader headers = getRequiredHeaders();
    clientStream->sendHEADERS(headers, false);

    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    QCOMPARE(clientStream->streamID(), serverStream->streamID());

    QSignalSpy rstClientSpy{ clientStream, &QHttp2Stream::rstFrameReceived };
    QSignalSpy rstServerSpy{ serverStream, &QHttp2Stream::rstFrameReceived };
    QSignalSpy endServerSpy{ serverConnection, &QHttp2Connection::receivedEND_STREAM };
    QSignalSpy errrorServerSpy{ serverStream, &QHttp2Stream::errorOccurred };

    QCOMPARE(clientStream->state(), QHttp2Stream::State::Open);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::Open);

    // Send data with END_STREAM = true

    QBuffer *buffer = new QBuffer(clientStream);
    QByteArray uploadedData = "Hello World"_ba.repeated(10);
    buffer->setData(uploadedData);
    buffer->open(QIODevice::ReadWrite);
    // send data with endstream true
    clientStream->sendDATA(buffer, true);

    QVERIFY(endServerSpy.wait());

    QCOMPARE(clientStream->state(), QHttp2Stream::State::HalfClosedLocal);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::HalfClosedRemote);

    clientStream->setState(QHttp2Stream::State::Open);
    buffer = new QBuffer(clientStream);
    buffer->setData(uploadedData);
    buffer->open(QIODevice::ReadWrite);
    // send data on closed stream
    clientStream->sendDATA(buffer, true);

    QVERIFY(rstClientSpy.wait());
    QCOMPARE(rstServerSpy.count(), 0);
    QCOMPARE(errrorServerSpy.count(), 1);
}

void tst_QHttp2Connection::resetAfterClose()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };

    QSignalSpy clientHeaderReceivedSpy{ clientStream, &QHttp2Stream::headersReceived };
    HPack::HttpHeader headers = getRequiredHeaders();
    clientStream->sendHEADERS(headers, true);

    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    QCOMPARE(clientStream->streamID(), serverStream->streamID());

    QSignalSpy errorSpy(clientStream, &QHttp2Stream::errorOccurred);

    const HPack::HttpHeader StatusOKHeaders{ { ":status", "200" } };
    serverStream->sendHEADERS(StatusOKHeaders, true);

    // Write the RST_STREAM frame manually because we guard against sending RST_STREAM on closed
    // streams
    auto &frameWriter = serverConnection->frameWriter;
    frameWriter.start(Http2::FrameType::RST_STREAM, Http2::FrameFlag::EMPTY,
                      serverStream->streamID());
    frameWriter.append(quint32(Http2::Http2Error::STREAM_CLOSED));
    QVERIFY(frameWriter.write(*serverConnection->getSocket()));

    QVERIFY(clientHeaderReceivedSpy.wait());
    QCOMPARE(clientStream->state(), QHttp2Stream::State::Closed);

    QTest::qWait(10); // Just needs to process events basically
    QCOMPARE(errorSpy.count(), 0);
}

void tst_QHttp2Connection::testBadFrameSize_data()
{
    QTest::addColumn<uchar>("frametype");
    QTest::addColumn<int>("loadsize");
    QTest::addColumn<bool>("rst_received");
    QTest::addColumn<int>("goaway_received");

    QTest::newRow("priority_correct") << uchar(Http2::FrameType::PRIORITY) << 5 << false << 0;
    QTest::newRow("priority_bad") << uchar(Http2::FrameType::PRIORITY) << 6 << true << 0;
    QTest::newRow("ping_correct") << uchar(Http2::FrameType::PING) << 8 << false << 0;
    QTest::newRow("ping_bad") << uchar(Http2::FrameType::PING) << 13 << false << 1;
}

void tst_QHttp2Connection::testBadFrameSize()
{
    QFETCH(uchar, frametype);
    QFETCH(int, loadsize);
    QFETCH(bool, rst_received);
    QFETCH(int, goaway_received);

    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };

    QSignalSpy clientHeaderReceivedSpy{ clientStream, &QHttp2Stream::headersReceived };
    HPack::HttpHeader headers = getRequiredHeaders();
    clientStream->sendHEADERS(headers, false);

    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    QCOMPARE(clientStream->streamID(), serverStream->streamID());

    QSignalSpy rstClientSpy{ clientStream, &QHttp2Stream::rstFrameReceived };
    QSignalSpy rstServerSpy{ serverStream, &QHttp2Stream::rstFrameReceived };
    QSignalSpy goawayClientSpy{ connection, &QHttp2Connection::receivedGOAWAY };
    QSignalSpy goawayServerSpy{ serverConnection, &QHttp2Connection::receivedGOAWAY };

    QCOMPARE(clientStream->state(), QHttp2Stream::State::Open);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::Open);

    {
        auto type = frametype;
        auto flags = uchar(Http2::FrameFlag::EMPTY);
        quint32 streamID = clientStream->streamID();

        std::vector<uchar> buffer;

        buffer.resize(Http2::frameHeaderSize);
        //012 Length (24) = 0x05 (set below),
        //3   Type (8) = 0x02,
        //4   Unused Flags (8),
        //    Reserved (1),
        //5   Stream Identifier (31),
        buffer[3] = type;
        buffer[4] = flags;
        qToBigEndian(type == uchar(Http2::FrameType::PING) ? 0 : streamID, &buffer[5]);

        buffer.resize(buffer.size() + loadsize);
        // RFC9113 4.1: The 9 octets of the frame header are not included in this value.
        quint32 size = quint32(buffer.size() - Http2::frameHeaderSize);
        buffer[0] = size >> 16;
        buffer[1] = size >> 8;
        buffer[2] = size;

        auto writtenN = connection->getSocket()->write(reinterpret_cast<const char *>(&buffer[0]), buffer.size());
        QCOMPARE(writtenN, qint64(buffer.size()));
    }

    QCOMPARE(rstClientSpy.wait(), rst_received);
    QCOMPARE(rstServerSpy.count(), 0);
    QCOMPARE(goawayClientSpy.count(), goaway_received);
}

void tst_QHttp2Connection::testDataFrameAfterRSTIncoming()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };

    QSignalSpy clientHeaderReceivedSpy{ clientStream, &QHttp2Stream::headersReceived };
    HPack::HttpHeader headers = getRequiredHeaders();
    clientStream->sendHEADERS(headers, false);

    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    QCOMPARE(clientStream->streamID(), serverStream->streamID());

    QSignalSpy rstServerSpy{ serverStream, &QHttp2Stream::rstFrameReceived };

    QCOMPARE(clientStream->state(), QHttp2Stream::State::Open);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::Open);

    // Reset the stream and wait until it the RST_STREAM frame is received
    clientStream->sendRST_STREAM(Http2::Http2Error::STREAM_CLOSED);
    QVERIFY(rstServerSpy.wait());

    QSignalSpy closedClientSpy{ connection, &QHttp2Connection::connectionClosed };
    // Send data as if we didn't receive the RST_STREAM
    // It should not trigger an error since we could have not received the RST_STREAM frame yet
    serverStream->setState(QHttp2Stream::State::Open);
    QBuffer *buffer = new QBuffer(clientStream);
    QByteArray uploadedData = "Hello World"_ba.repeated(10);
    buffer->setData(uploadedData);
    buffer->open(QIODevice::ReadWrite);
    serverStream->sendDATA(buffer, false);

    QVERIFY(!closedClientSpy.wait(std::chrono::seconds(1)));
}

void tst_QHttp2Connection::testDataFrameAfterRSTOutgoing()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };

    QSignalSpy clientHeaderReceivedSpy{ clientStream, &QHttp2Stream::headersReceived };
    QVERIFY(clientStream);
    HPack::HttpHeader headers = getRequiredHeaders();
    clientStream->sendHEADERS(headers, false);

    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    QCOMPARE(clientStream->streamID(), serverStream->streamID());

    QSignalSpy rstServerSpy{ serverStream, &QHttp2Stream::rstFrameReceived };

    QCOMPARE(clientStream->state(), QHttp2Stream::State::Open);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::Open);

    // Reset the stream and wait until it the RST_STREAM frame is received
    clientStream->sendRST_STREAM(Http2::Http2Error::STREAM_CLOSED);
    QVERIFY(rstServerSpy.wait());

    QSignalSpy closedServerSpy{ serverConnection, &QHttp2Connection::connectionClosed };
    // Send data as if we didn't send the RST_STREAM
    // It should trigger an error since we send the RST_STREAM frame ourself
    clientStream->setState(QHttp2Stream::State::Open);
    QBuffer *buffer = new QBuffer(serverStream);
    QByteArray uploadedData = "Hello World"_ba.repeated(10);
    buffer->setData(uploadedData);
    buffer->open(QIODevice::ReadWrite);
    clientStream->sendDATA(buffer, false);

    QVERIFY(closedServerSpy.wait());
}

void tst_QHttp2Connection::headerFrameAfterRSTOutgoing_data()
{
    QTest::addColumn<bool>("deleteStream");
    QTest::addRow("retain-stream") << false;
    QTest::addRow("delete-stream") << true;
}

void tst_QHttp2Connection::headerFrameAfterRSTOutgoing()
{
    QFETCH(const bool, deleteStream);
    auto [client, server] = makeFakeConnectedSockets();
    auto *connection = makeHttp2Connection(client.get(), {}, Client);
    auto *serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QSignalSpy client1HeadersSpy{ clientStream, &QHttp2Stream::headersReceived};
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };
    HPack::HttpHeader headers = getRequiredHeaders();

    // Send some headers to let the server know about the stream
    clientStream->sendHEADERS(headers, false);

    // Wait for the stream on the server side
    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    newIncomingStreamSpy.clear();

    QSignalSpy serverRSTReceivedSpy{ serverStream, &QHttp2Stream::rstFrameReceived };

    // Send an RST frame from the client, but we don't process it yet
    clientStream->sendRST_STREAM(Http2::CANCEL);
    if (deleteStream)
        delete std::exchange(clientStream, nullptr);

    // The server sends a reply, not knowing about the inbound RST frame
    const HPack::HttpHeader StandardReply{ { ":status", "200" }, { "x-whatever", "some info" } };
    serverStream->sendHEADERS(StandardReply, true);

    // With the bug in QTBUG-135800 we would ignore the RST frame, not processing it at all.
    // This caused the HPACK lookup tables to be out of sync between server and client, eventually
    // causing an error on Qt's side.
    QVERIFY(serverRSTReceivedSpy.wait());

    // We don't emit any headers for a reset stream
    QVERIFY(!client1HeadersSpy.count());

    // Create a new stream then send and handle a new request!
    QHttp2Stream *clientStream2 = connection->createStream().unwrap();
    QSignalSpy client2HeaderReceivedSpy{ clientStream2, &QHttp2Stream::headersReceived };
    QSignalSpy client2ErrorOccurredSpy{ clientStream2, &QHttp2Stream::errorOccurred };
    clientStream2->sendHEADERS(headers, true);
    QVERIFY(newIncomingStreamSpy.wait());
    QHttp2Stream *serverStream2 = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    serverStream2->sendHEADERS(StandardReply, true);
    QVERIFY(client2HeaderReceivedSpy.wait());
    QCOMPARE(client2ErrorOccurredSpy.count(), 0);
}

void tst_QHttp2Connection::connectToServer()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };
    QSignalSpy clientIncomingStreamSpy{ connection, &QHttp2Connection::newIncomingStream };

    QSignalSpy clientHeaderReceivedSpy{ clientStream, &QHttp2Stream::headersReceived };
    HPack::HttpHeader headers = getRequiredHeaders();
    clientStream->sendHEADERS(headers, false);

    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    QVERIFY(serverStream);
    const HPack::HttpHeader ExpectedResponseHeaders{ { ":status", "200" } };
    serverStream->sendHEADERS(ExpectedResponseHeaders, true);

    QVERIFY(clientHeaderReceivedSpy.wait());
    const HPack::HttpHeader
            headersReceived = clientHeaderReceivedSpy.front().front().value<HPack::HttpHeader>();
    QCOMPARE(headersReceived, ExpectedResponseHeaders);

    QCOMPARE(clientIncomingStreamSpy.count(), 0);
}

void tst_QHttp2Connection::WINDOW_UPDATE()
{
    auto [client, server] = makeFakeConnectedSockets();
    auto connection = makeHttp2Connection(client.get(), {}, Client);

    QHttp2Configuration config;
    config.setStreamReceiveWindowSize(1024); // Small window on server to provoke WINDOW_UPDATE
    auto serverConnection = makeHttp2Connection(server.get(), config, Server);

    QHttp2Stream *clientStream = connection->createStream().unwrap();
    QVERIFY(clientStream);
    QVERIFY(waitForSettingsExchange(connection, serverConnection));

    QSignalSpy newIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream };

    QSignalSpy clientHeaderReceivedSpy{ clientStream, &QHttp2Stream::headersReceived };
    QSignalSpy clientDataReceivedSpy{ clientStream, &QHttp2Stream::dataReceived };
    HPack::HttpHeader expectedRequestHeaders = HPack::HttpHeader{
        { ":authority", "example.com" },
        { ":method", "POST" },
        { ":path", "/" },
        { ":scheme", "https" },
    };
    clientStream->sendHEADERS(expectedRequestHeaders, false);

    QVERIFY(newIncomingStreamSpy.wait());
    auto *serverStream = newIncomingStreamSpy.front().front().value<QHttp2Stream *>();
    QVERIFY(serverStream);
    QSignalSpy serverDataReceivedSpy{ serverStream, &QHttp2Stream::dataReceived };

    // Since a stream is only opened on the remote side when the header is received,
    // we can check the headers now immediately
    QCOMPARE(serverStream->receivedHeaders(), expectedRequestHeaders);

    QByteArray uploadedData = "Hello World"_ba.repeated(1000);
    clientStream->sendDATA(uploadedData, true);

    bool streamEnd = false;
    QByteArray serverReceivedData;
    while (!streamEnd) { // The window is too small to receive all data at once, so loop
        QVERIFY(serverDataReceivedSpy.wait());
        auto latestEmission = serverDataReceivedSpy.back();
        serverReceivedData += latestEmission.front().value<QByteArray>();
        streamEnd = latestEmission.back().value<bool>();
    }
    QCOMPARE(serverReceivedData.size(), uploadedData.size());
    QCOMPARE(serverReceivedData, uploadedData);

    QCOMPARE(clientStream->state(), QHttp2Stream::State::HalfClosedLocal);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::HalfClosedRemote);

    const HPack::HttpHeader ExpectedResponseHeaders{ { ":status", "200" } };
    serverStream->sendHEADERS(ExpectedResponseHeaders, false);
    serverStream->sendDATA(uploadedData, true);

    QVERIFY(clientHeaderReceivedSpy.wait());
    const HPack::HttpHeader
            headersReceived = clientHeaderReceivedSpy.front().front().value<HPack::HttpHeader>();
    QCOMPARE(headersReceived, ExpectedResponseHeaders);

    QTRY_COMPARE_GT(clientDataReceivedSpy.count(), 0);
    QCOMPARE(clientDataReceivedSpy.count(), 1); // Only one DATA frame since our window is larger
    QCOMPARE(clientDataReceivedSpy.front().front().value<QByteArray>(), uploadedData);

    QCOMPARE(clientStream->state(), QHttp2Stream::State::Closed);
    QCOMPARE(serverStream->state(), QHttp2Stream::State::Closed);
}

namespace {

void sendHEADERSFrame(HPack::Encoder &encoder,
                      Http2::FrameWriter &frameWriter,
                      quint32 streamId,
                      const HPack::HttpHeader &headers,
                      Http2::FrameFlags flags,
                      QIODevice &socket)
{
    frameWriter.start(Http2::FrameType::HEADERS,
                      flags,
                      streamId);
    frameWriter.append(quint32());
    frameWriter.append(QHttp2Stream::DefaultPriority);

    HPack::BitOStream outputStream(frameWriter.outboundFrame().buffer);
    QVERIFY(encoder.encodeRequest(outputStream, headers));
    frameWriter.setPayloadSize(static_cast<quint32>(frameWriter.outboundFrame().buffer.size()
                               - Http2::Http2PredefinedParameters::frameHeaderSize));
    frameWriter.write(socket);
}

void sendDATAFrame(Http2::FrameWriter &frameWriter,
                   quint32 streamId,
                   Http2::FrameFlags flags,
                   QIODevice &socket)
{
    frameWriter.start(Http2::FrameType::DATA,
                      flags,
                      streamId);
    frameWriter.write(socket);
}

void sendCONTINUATIONFrame(HPack::Encoder &encoder,
                           Http2::FrameWriter &frameWriter,
                           quint32 streamId,
                           const HPack::HttpHeader &headers,
                           Http2::FrameFlags flags,
                           QIODevice &socket)
{
    frameWriter.start(Http2::FrameType::CONTINUATION,
                      flags,
                      streamId);

    HPack::BitOStream outputStream(frameWriter.outboundFrame().buffer);
    QVERIFY(encoder.encodeRequest(outputStream, headers));
    frameWriter.setPayloadSize(static_cast<quint32>(frameWriter.outboundFrame().buffer.size()
                               - Http2::Http2PredefinedParameters::frameHeaderSize));
    frameWriter.write(socket);
}

}

void tst_QHttp2Connection::testCONTINUATIONFrame()
{
    static const HPack::HttpHeader headers = HPack::HttpHeader {
        { ":authority", "example.com" },
        { ":method", "GET" },
        { ":path", "/" },
        { ":scheme", "https" },
        { "n1", "v1" },
        { "n2", "v2" },
        { "n3", "v3" }
    };

    #define CREATE_CONNECTION() \
    auto [client, server] = makeFakeConnectedSockets(); \
    auto clientConnection = makeHttp2Connection(client.get(), {}, Client); \
    auto serverConnection = makeHttp2Connection(server.get(), {}, Server); \
    QHttp2Stream *clientStream = clientConnection->createStream().unwrap(); \
    QVERIFY(clientStream); \
    QVERIFY(waitForSettingsExchange(clientConnection, serverConnection)); \
    \
    HPack::Encoder encoder = HPack::Encoder(HPack::FieldLookupTable::DefaultSize, true); \
    Http2::FrameWriter frameWriter; \
    \
    QSignalSpy serverIncomingStreamSpy{ serverConnection, &QHttp2Connection::newIncomingStream }; \
    QSignalSpy receivedGOAWAYSpy{ clientConnection, &QHttp2Connection::receivedGOAWAY }; \
    \

    // Send multiple CONTINUATION frames
    {
        CREATE_CONNECTION();

        frameWriter.start(Http2::FrameType::HEADERS,
                          Http2::FrameFlag::PRIORITY,
                          clientStream->streamID());
        frameWriter.append(quint32());
        frameWriter.append(QHttp2Stream::DefaultPriority);
        HPack::BitOStream outputStream(frameWriter.outboundFrame().buffer);
        QVERIFY(encoder.encodeRequest(outputStream, headers));

        // split headers into multiple CONTINUATION frames
        const auto sizeLimit = static_cast<qint32>(frameWriter.outboundFrame().buffer.size() / 5);
        frameWriter.writeHEADERS(*client, sizeLimit);

        QVERIFY(serverIncomingStreamSpy.wait());
        auto *serverStream = serverIncomingStreamSpy.front().front().value<QHttp2Stream *>();
        QVERIFY(serverStream);
        // correct behavior accepted and handled sensibly
        QCOMPARE(serverStream->receivedHeaders(), headers);
    }

    // A DATA frame between a HEADERS frame and a CONTINUATION frame.
    {
        CREATE_CONNECTION();

        sendHEADERSFrame(encoder, frameWriter, clientStream->streamID(),
                         headers, Http2::FrameFlag::PRIORITY, *client);
        QVERIFY(serverIncomingStreamSpy.wait());

        sendDATAFrame(frameWriter, clientStream->streamID(), Http2::FrameFlag::EMPTY, *client);
        // the client correctly rejected our malformed stream contents by telling us to GO AWAY
        QVERIFY(receivedGOAWAYSpy.wait());
    }

    // A CONTINUATION frame after a frame with the END_HEADERS set.
    {
        CREATE_CONNECTION();

        sendHEADERSFrame(encoder, frameWriter, clientStream->streamID(), headers,
                         Http2::FrameFlag::PRIORITY | Http2::FrameFlag::END_HEADERS, *client);
        QVERIFY(serverIncomingStreamSpy.wait());

        sendCONTINUATIONFrame(encoder, frameWriter, clientStream->streamID(),
                              headers, Http2::FrameFlag::EMPTY, *client);
        // the client correctly rejected our malformed stream contents by telling us to GO AWAY
        QVERIFY(receivedGOAWAYSpy.wait());
    }

    // A CONTINUATION frame with the stream id 0x00.
    {
        CREATE_CONNECTION();

        sendHEADERSFrame(encoder, frameWriter, clientStream->streamID(),
                         headers, Http2::FrameFlag::PRIORITY, *client);
        QVERIFY(serverIncomingStreamSpy.wait());

        sendCONTINUATIONFrame(encoder, frameWriter, 0,
                              headers, Http2::FrameFlag::EMPTY, *client);
        // the client correctly rejected our malformed stream contents by telling us to GO AWAY
        QVERIFY(receivedGOAWAYSpy.wait());
    }

    // A CONTINUATION frame with a different stream id then the previous frame.
    {
        CREATE_CONNECTION();

        sendHEADERSFrame(encoder, frameWriter, clientStream->streamID(),
                         headers, Http2::FrameFlag::PRIORITY, *client);
        QVERIFY(serverIncomingStreamSpy.wait());

        QHttp2Stream *newClientStream = clientConnection->createStream().unwrap();
        QVERIFY(newClientStream);

        sendCONTINUATIONFrame(encoder, frameWriter, newClientStream->streamID(),
                              headers, Http2::FrameFlag::EMPTY, *client);
        // the client correctly rejected our malformed stream contents by telling us to GO AWAY
        QVERIFY(receivedGOAWAYSpy.wait());
    }
}

QTEST_MAIN(tst_QHttp2Connection)

#include "tst_qhttp2connection.moc"
