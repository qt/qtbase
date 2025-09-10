// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT_CTFSERVER_H
#define QT_CTFSERVER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
//

#include <qbytearray.h>
#include <qdatastream.h>
#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <qeventloop.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>
#include <qcborstreamreader.h>
#include <qcborstreamwriter.h>
#include <qlist.h>

typedef struct ZSTD_CCtx_s ZSTD_CCtx;

QT_BEGIN_NAMESPACE

class QCtfServer;
struct TracePacket
{
    static constexpr quint32 PacketMagicNumber = 0x100924da;
    static constexpr quint32 PacketSize = 4096 + 9;
    QByteArray stream_name;
    QByteArray stream_data;
    quint32 flags = 0;

    TracePacket() = default;

    TracePacket(const TracePacket &t)
    {
        stream_name = t.stream_name;
        stream_data = t.stream_data;
        flags = t.flags;
    }
    TracePacket &operator = (const TracePacket &t)
    {
        stream_name = t.stream_name;
        stream_data = t.stream_data;
        flags = t.flags;
        return *this;
    }
    TracePacket(TracePacket &&t)
    {
        stream_name = std::move(t.stream_name);
        stream_data = std::move(t.stream_data);
        flags = t.flags;
    }
    TracePacket &operator = (TracePacket &&t)
    {
        stream_name = std::move(t.stream_name);
        stream_data = std::move(t.stream_data);
        flags = t.flags;
        return *this;
    }
};

auto constexpr operator""_MB(quint64 s) -> quint64
{
    return s * 1024ul * 1024ul;
}

struct TraceRequest
{
    quint32 clientId;
    quint32 clientVersion;
    quint32 flags;
    quint32 bufferSize;
    QString sessionName;
    QString sessionTracepoints;

    static constexpr quint32 MaxBufferSize = 1024_MB;

    bool isValid() const
    {
        if (clientId != 0 && clientVersion != 0 && !sessionName.isEmpty()
            && !sessionTracepoints.isEmpty() && bufferSize < MaxBufferSize)
            return true;
        return false;
    }
};

struct TraceResponse
{
    quint32 serverId;
    quint32 serverVersion;
    QString serverName;
};

class QCtfServer : public QThread
{
    Q_OBJECT
public:
    enum ServerStatus
    {
        Uninitialized,
        Idle,
        Connected,
        Error,
    };
    enum ServerFlags
    {
        CompressionMask = 255,
        DontBufferOnIdle = 256,  // not set -> the server is buffering even without client connection
                                 // set -> the server is buffering only when client is connected
    };
    enum RequestIds
    {
        RequestClientId = 0,
        RequestClientVersion,
        RequestSessionName,
        RequestSessionTracepoints,
        RequestFlags,
        RequestBufferSize,
        RequestCompressionScheme,
    };

    struct ServerCallback
    {
        virtual void handleSessionChange() = 0;
        virtual void handleStatusChange(ServerStatus status) = 0;
    };
    QCtfServer(QObject *parent = nullptr);
    ~QCtfServer();
    void setCallback(ServerCallback *cb);
    void setHost(const QString &address);
    void setPort(int port);
    void run() override;
    void startServer();
    void stopServer();
    void bufferData(const QString &stream, const QByteArray &data, quint32 flags);
    QString sessionName() const;
    QString sessionTracepoints() const;
    bool bufferOnIdle() const;
    ServerStatus status() const;
private:

    void initWrite();
    void bytesWritten(qint64 size);
    bool waitSocket();
    void readCbor(QCborStreamReader &cbor);
    void handleString(QCborStreamReader &cbor);
    void handleFixedWidth(QCborStreamReader &cbor);
    bool recognizedCompressionScheme() const;
    void setStatusAndNotify(ServerStatus status);
    void writePacket(TracePacket &packet, QCborStreamWriter &cbor);

    QMutex m_mutex;
    QWaitCondition m_bufferHasData;
    QList<TracePacket> m_packets;
    QString m_address;
    QTcpServer *m_server = nullptr;
    QTcpSocket *m_socket = nullptr;
    QEventLoop *m_eventLoop = nullptr;
    QList<QString> m_keySet;
    TraceRequest m_req;
    ServerCallback *m_cb = nullptr;
    ServerStatus m_status = Uninitialized;
    qint64 m_waitWriteSize = 0;
    qint64 m_writtenSize = 0;
    int m_port;
    int m_compression = 0;
    int m_maxPackets = DefaultMaxPackets;
    QAtomicInt m_stopping;
    bool m_bufferOnIdle = true;
    QString m_currentKey;
    QString m_requestedCompressionScheme;
#if QT_CONFIG(zstd)
    ZSTD_CCtx *m_zstdCCtx = nullptr;
#endif

    static constexpr quint32 ServerId = 1;
    static constexpr quint32 DefaultMaxPackets = 256; // 1 MB
};

QT_END_NAMESPACE

#endif
