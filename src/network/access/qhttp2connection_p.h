// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef HTTP2CONNECTION_P_H
#define HTTP2CONNECTION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the Network Access API. This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtnetworkglobal_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qxpfunctional.h>
#include <QtNetwork/qhttp2configuration.h>
#include <QtNetwork/qtcpsocket.h>

#include <private/http2protocol_p.h>
#include <private/http2streams_p.h>
#include <private/http2frames_p.h>
#include <private/hpack_p.h>

#include <variant>
#include <optional>
#include <type_traits>
#include <limits>

QT_BEGIN_NAMESPACE

template <typename T, typename Err>
class QH2Expected
{
    static_assert(!std::is_same_v<T, Err>, "T and Err must be different types");
public:
    // Rule Of Zero applies
    QH2Expected(T &&value) : m_data(std::move(value)) { }
    QH2Expected(const T &value) : m_data(value) { }
    QH2Expected(Err &&error) : m_data(std::move(error)) { }
    QH2Expected(const Err &error) : m_data(error) { }

    QH2Expected &operator=(T &&value)
    {
        m_data = std::move(value);
        return *this;
    }
    QH2Expected &operator=(const T &value)
    {
        m_data = value;
        return *this;
    }
    QH2Expected &operator=(Err &&error)
    {
        m_data = std::move(error);
        return *this;
    }
    QH2Expected &operator=(const Err &error)
    {
        m_data = error;
        return *this;
    }
    T unwrap() const
    {
        Q_ASSERT(ok());
        return std::get<T>(m_data);
    }
    Err error() const
    {
        Q_ASSERT(has_error());
        return std::get<Err>(m_data);
    }
    bool ok() const noexcept { return std::holds_alternative<T>(m_data); }
    bool has_value() const noexcept { return ok(); }
    bool has_error() const noexcept { return std::holds_alternative<Err>(m_data); }
    void clear() noexcept { m_data.reset(); }

private:
    std::variant<T, Err> m_data;
};

class QHttp2Connection;
class Q_NETWORK_EXPORT QHttp2Stream : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QHttp2Stream)

public:
    enum class State { Idle, ReservedRemote, Open, HalfClosedLocal, HalfClosedRemote, Closed };
    Q_ENUM(State)
    constexpr static quint8 DefaultPriority = 127;

    ~QHttp2Stream() noexcept;

    // HTTP2 things
    quint32 streamID() const noexcept { return m_streamID; }

    // Are we waiting for a larger send window before sending more data?
    bool isUploadBlocked() const noexcept;
    bool isUploadingDATA() const noexcept { return m_uploadByteDevice != nullptr; }
    State state() const noexcept { return m_state; }
    bool isActive() const noexcept { return m_state != State::Closed && m_state != State::Idle; }
    bool isPromisedStream() const noexcept { return m_isReserved; }
    bool wasReset() const noexcept { return m_RST_STREAM_code.has_value(); }
    quint32 RST_STREAM_code() const noexcept { return m_RST_STREAM_code.value_or(0); }
    // Just the list of headers, as received, may contain duplicates:
    HPack::HttpHeader receivedHeaders() const noexcept { return m_headers; }

    QByteDataBuffer downloadBuffer() const noexcept { return m_downloadBuffer; }

Q_SIGNALS:
    void headersReceived(const HPack::HttpHeader &headers, bool endStream);
    void headersUpdated();
    void errorOccurred(quint32 errorCode, const QString &errorString);
    void stateChanged(QHttp2Stream::State newState);
    void promisedStreamReceived(quint32 newStreamID);
    void uploadBlocked();
    void dataReceived(const QByteArray &data, bool endStream);

    void bytesWritten(qint64 bytesWritten);
    void uploadDeviceError(const QString &errorString);
    void uploadFinished();

public Q_SLOTS:
    bool sendRST_STREAM(quint32 errorCode);
    bool sendHEADERS(const HPack::HttpHeader &headers, bool endStream,
                     quint8 priority = DefaultPriority);
    void sendDATA(QIODevice *device, bool endStream);
    void sendDATA(QNonContiguousByteDevice *device, bool endStream);
    void sendWINDOW_UPDATE(quint32 delta);
    void uploadDeviceDestroyed();

private Q_SLOTS:
    void maybeResumeUpload();
    void uploadDeviceReadChannelFinished();

private:
    friend class QHttp2Connection;
    QHttp2Stream(QHttp2Connection *connection, quint32 streamID) noexcept;

    [[nodiscard]] QHttp2Connection *getConnection() const
    {
        return qobject_cast<QHttp2Connection *>(parent());
    }

    enum class StateTransition {
        Open,
        CloseLocal,
        CloseRemote,
        RST,
    };

    void setState(State newState);
    void transitionState(StateTransition transition);
    void internalSendDATA();
    void finishSendDATA();

    void handleDATA(const Http2::Frame &inboundFrame);
    void handleHEADERS(Http2::FrameFlags frameFlags, const HPack::HttpHeader &headers);
    void handleRST_STREAM(const Http2::Frame &inboundFrame);
    void handleWINDOW_UPDATE(const Http2::Frame &inboundFrame);

    void finishWithError(quint32 errorCode, const QString &message);
    void finishWithError(quint32 errorCode);

    // Keep it const since it never changes after creation
    const quint32 m_streamID = 0;
    qint32 m_recvWindow = 0;
    qint32 m_sendWindow = 0;
    bool m_endStreamAfterDATA = false;
    std::optional<quint32> m_RST_STREAM_code;

    QIODevice *m_uploadDevice = nullptr;
    QNonContiguousByteDevice *m_uploadByteDevice = nullptr;

    QByteDataBuffer m_downloadBuffer;
    State m_state = State::Idle;
    HPack::HttpHeader m_headers;
    bool m_isReserved = false;
};

class Q_NETWORK_EXPORT QHttp2Connection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QHttp2Connection)

public:
    enum class CreateStreamError {
        MaxConcurrentStreamsReached,
        StreamIdsExhausted,
        ReceivedGOAWAY,
    };
    Q_ENUM(CreateStreamError)

    // For a pre-established connection:
    [[nodiscard]] static QHttp2Connection *
    createUpgradedConnection(QIODevice *socket, const QHttp2Configuration &config);
    // For a new connection, potential TLS handshake must already be finished:
    [[nodiscard]] static QHttp2Connection *createDirectConnection(QIODevice *socket,
                                                                const QHttp2Configuration &config);
    [[nodiscard]] static QHttp2Connection *
    createDirectServerConnection(QIODevice *socket, const QHttp2Configuration &config);
    ~QHttp2Connection();

    [[nodiscard]] QH2Expected<QHttp2Stream *, CreateStreamError> createStream();

    QHttp2Stream *getStream(quint32 streamId) const;
    QHttp2Stream *promisedStream(const QUrl &streamKey) const
    {
        if (quint32 id = m_promisedStreams.value(streamKey, 0); id)
            return m_streams.value(id);
        return nullptr;
    }

    void close() { sendGOAWAY(Http2::HTTP2_NO_ERROR); }

    bool isGoingAway() const noexcept { return m_goingAway; }

    quint32 maxConcurrentStreams() const noexcept { return m_maxConcurrentStreams; }
    quint32 maxHeaderListSize() const noexcept { return m_maxHeaderListSize; }

    bool isUpgradedConnection() const noexcept { return m_upgradedConnection; }

Q_SIGNALS:
    void newIncomingStream(QHttp2Stream *stream);
    void newPromisedStream(QHttp2Stream *stream);
    void errorReceived(/*@future: add as needed?*/); // Connection errors only, no stream-specific errors
    void connectionClosed();
    void settingsFrameReceived();
    void errorOccurred(Http2::Http2Error errorCode, const QString &errorString);
    void receivedGOAWAY(quint32 errorCode, quint32 lastStreamID);
public Q_SLOTS:
    void handleReadyRead();
    void handleConnectionClosure();

private:
    friend class QHttp2Stream;
    [[nodiscard]] QIODevice *getSocket() const { return qobject_cast<QIODevice *>(parent()); }

    QH2Expected<QHttp2Stream *, QHttp2Connection::CreateStreamError> createStreamInternal();
    QHttp2Stream *createStreamInternal_impl(quint32 streamID);

    bool isInvalidStream(quint32 streamID) noexcept;
    bool streamWasReset(quint32 streamID) noexcept;

    void connectionError(Http2::Http2Error errorCode,
                         const char *message); // Connection failed to be established?
    void setH2Configuration(QHttp2Configuration config);
    void closeSession();
    qsizetype numActiveStreamsImpl(quint32 mask) const noexcept;
    qsizetype numActiveRemoteStreams() const noexcept;
    qsizetype numActiveLocalStreams() const noexcept;

    bool sendClientPreface();
    bool sendSETTINGS();
    bool sendServerPreface();
    bool serverCheckClientPreface();
    bool sendWINDOW_UPDATE(quint32 streamID, quint32 delta);
    bool sendGOAWAY(quint32 errorCode);
    bool sendSETTINGS_ACK();

    void handleDATA();
    void handleHEADERS();
    void handlePRIORITY();
    void handleRST_STREAM();
    void handleSETTINGS();
    void handlePUSH_PROMISE();
    void handlePING();
    void handleGOAWAY();
    void handleWINDOW_UPDATE();
    void handleCONTINUATION();

    void handleContinuedHEADERS();

    bool acceptSetting(Http2::Settings identifier, quint32 newValue);

    bool readClientPreface();

    explicit QHttp2Connection(QIODevice *socket);

    enum class Type { Client, Server } m_connectionType = Type::Client;

    bool waitingForSettingsACK = false;

    static constexpr quint32 maxAcceptableTableSize = 16 * HPack::FieldLookupTable::DefaultSize;
    // HTTP/2 4.3: Header compression is stateful. One compression context and
    // one decompression context are used for the entire connection.
    HPack::Decoder decoder = HPack::Decoder(HPack::FieldLookupTable::DefaultSize);
    HPack::Encoder encoder = HPack::Encoder(HPack::FieldLookupTable::DefaultSize, true);

    QHttp2Configuration m_config;
    QHash<quint32, QPointer<QHttp2Stream>> m_streams;
    QHash<QUrl, quint32> m_promisedStreams;
    QVarLengthArray<quint32> m_resetStreamIDs;
    quint32 m_nextStreamID = 1;

    // Peer's max frame size (this min is the default value
    // we start with, that can be updated by SETTINGS frame):
    quint32 maxFrameSize = Http2::minPayloadLimit;

    Http2::FrameReader frameReader;
    Http2::Frame inboundFrame;
    Http2::FrameWriter frameWriter;

    // Temporary storage to assemble HEADERS' block
    // from several CONTINUATION frames ...
    bool continuationExpected = false;
    std::vector<Http2::Frame> continuedFrames;

    // Control flow:

    // This is how many concurrent streams our peer allows us, 100 is the
    // initial value, can be updated by the server's SETTINGS frame(s):
    quint32 m_maxConcurrentStreams = Http2::maxConcurrentStreams;
    // While we allow sending SETTTINGS_MAX_CONCURRENT_STREAMS to limit our peer,
    // it's just a hint and we do not actually enforce it (and we can continue
    // sending requests and creating streams while maxConcurrentStreams allows).

    // This is our (client-side) maximum possible receive window size, we set
    // it in a ctor from QHttp2Configuration, it does not change after that.
    // The default is 64Kb:
    qint32 maxSessionReceiveWindowSize = Http2::defaultSessionWindowSize;

    // Our session current receive window size, updated in a ctor from
    // QHttp2Configuration. Signed integer since it can become negative
    // (it's still a valid window size).
    qint32 sessionReceiveWindowSize = Http2::defaultSessionWindowSize;
    // Our per-stream receive window size, default is 64 Kb, will be updated
    // from QHttp2Configuration. Again, signed - can become negative.
    qint32 streamInitialReceiveWindowSize = Http2::defaultSessionWindowSize;

    // These are our peer's receive window sizes, they will be updated by the
    // peer's SETTINGS and WINDOW_UPDATE frames, defaults presumed to be 64Kb.
    qint32 sessionSendWindowSize = Http2::defaultSessionWindowSize;
    qint32 streamInitialSendWindowSize = Http2::defaultSessionWindowSize;

    // Our peer's header size limitations. It's unlimited by default, but can
    // be changed via peer's SETTINGS frame.
    quint32 m_maxHeaderListSize = (std::numeric_limits<quint32>::max)();
    // While we can send SETTINGS_MAX_HEADER_LIST_SIZE value (our limit on
    // the headers size), we never enforce it, it's just a hint to our peer.

    bool m_upgradedConnection = false;
    bool m_goingAway = false;
    bool pushPromiseEnabled = false;
    quint32 m_lastIncomingStreamID = Http2::connectionStreamID;

    // Server-side only:
    bool m_waitingForClientPreface = false;
};

QT_END_NAMESPACE

#endif // HTTP2CONNECTION_P_H
