// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:network-protocol

#include <qloggingcategory.h>
#include "qctfserver_p.h"

#if QT_CONFIG(zstd)
#include <zstd.h>
#endif

using namespace Qt::Literals::StringLiterals;

Q_LOGGING_CATEGORY(lcCtfInfoTrace, "qt.core.ctfserver", QtWarningMsg)

#if QT_CONFIG(zstd)
static QByteArray zstdCompress(ZSTD_CCtx *&context, const QByteArray &data, int compression)
{
    if (context == nullptr)
        context = ZSTD_createCCtx();
    qsizetype size = data.size();
    size = ZSTD_COMPRESSBOUND(size);
    QByteArray compressed(size, Qt::Uninitialized);
    char *dst = compressed.data();
    size_t n = ZSTD_compressCCtx(context, dst, size,
                                 data.constData(), data.size(),
                                 compression);
    if (ZSTD_isError(n)) {
        qCWarning(lcCtfInfoTrace) << "Compression with zstd failed: " << QString::fromUtf8(ZSTD_getErrorName(n));
        return {};
    }
    compressed.truncate(n);
    return compressed;
}
#endif

QCtfServer::QCtfServer(QObject *parent)
    : QThread(parent)
{
    m_keySet << "cliendId"_L1
             << "clientVersion"_L1
             << "sessionName"_L1
             << "sessionTracepoints"_L1
             << "flags"_L1
             << "bufferSize"_L1
             << "compressionScheme"_L1;
}

QCtfServer::~QCtfServer()
{
#if QT_CONFIG(zstd)
    ZSTD_freeCCtx(m_zstdCCtx);
#endif
}

void QCtfServer::setHost(const QString &address)
{
    m_address = address;
}

void QCtfServer::setPort(int port)
{
    m_port = port;
}

void QCtfServer::setCallback(ServerCallback *cb)
{
    m_cb = cb;
}

QString QCtfServer::sessionName() const
{
    return m_req.sessionName;
}

QString QCtfServer::sessionTracepoints() const
{
    return m_req.sessionTracepoints;
}

bool QCtfServer::bufferOnIdle() const
{
    return m_bufferOnIdle;
}

QCtfServer::ServerStatus QCtfServer::status() const
{
    return m_status;
}

void QCtfServer::setStatusAndNotify(ServerStatus status)
{
    m_status = status;
    m_cb->handleStatusChange(status);
}

void QCtfServer::bytesWritten(qint64 size)
{
    m_writtenSize += size;
    if (m_writtenSize >= m_waitWriteSize && m_eventLoop)
        m_eventLoop->exit();
}

void QCtfServer::initWrite()
{
    m_waitWriteSize = 0;
    m_writtenSize = 0;
}

bool QCtfServer::waitSocket()
{
    if (m_eventLoop)
        m_eventLoop->exec();
    return m_socket->state() == QTcpSocket::ConnectedState;
}

void QCtfServer::handleString(QCborStreamReader &cbor)
{
    const auto readString = [](QCborStreamReader &cbor) -> QString {
        QString result;
        auto r = cbor.readString();
        while (r.status == QCborStreamReader::Ok) {
            result += r.data;
            r = cbor.readString();
        }

        if (r.status == QCborStreamReader::Error) {
            // handle error condition
            result.clear();
        }
        return result;
    };
    do {
        if (m_currentKey.isEmpty()) {
            m_currentKey = readString(cbor);
        } else {
            switch (m_keySet.indexOf(m_currentKey)) {
            case RequestSessionName:
                m_req.sessionName = readString(cbor);
                break;
            case RequestSessionTracepoints:
                m_req.sessionTracepoints = readString(cbor);
                break;
            case RequestCompressionScheme:
                m_requestedCompressionScheme = readString(cbor);
                break;
            default:
                // handle error
                break;
            }
            m_currentKey.clear();
        }
        if (cbor.lastError() == QCborError::EndOfFile) {
            if (!waitSocket())
                return;
            cbor.reparse();
        }
    } while (cbor.lastError() == QCborError::EndOfFile);
}

void QCtfServer::handleFixedWidth(QCborStreamReader &cbor)
{
    switch (m_keySet.indexOf(m_currentKey)) {
    case RequestClientId:
        if (!cbor.isUnsignedInteger())
            return;
        m_req.clientId = cbor.toUnsignedInteger();
        break;
    case RequestClientVersion:
        if (!cbor.isUnsignedInteger())
            return;
        m_req.clientVersion = cbor.toUnsignedInteger();
        break;
    case RequestFlags:
        if (!cbor.isUnsignedInteger())
            return;
        m_req.flags = cbor.toUnsignedInteger();
        break;
    case RequestBufferSize:
        if (!cbor.isUnsignedInteger())
            return;
        m_req.bufferSize = cbor.toUnsignedInteger();
        break;
    default:
        // handle error
        break;
    }
    m_currentKey.clear();
}

void QCtfServer::readCbor(QCborStreamReader &cbor)
{
    switch (cbor.type()) {
    case QCborStreamReader::UnsignedInteger:
    case QCborStreamReader::NegativeInteger:
    case QCborStreamReader::SimpleType:
    case QCborStreamReader::Float16:
    case QCborStreamReader::Float:
    case QCborStreamReader::Double:
        handleFixedWidth(cbor);
        cbor.next();
        break;
    case QCborStreamReader::ByteArray:
    case QCborStreamReader::String:
        handleString(cbor);
        break;
    case QCborStreamReader::Array:
    case QCborStreamReader::Map:
        cbor.enterContainer();
        while (cbor.lastError() == QCborError::NoError && cbor.hasNext())
            readCbor(cbor);
        if (cbor.lastError() == QCborError::NoError)
            cbor.leaveContainer();
    default:
        break;
    }
}

void QCtfServer::writePacket(TracePacket &packet, QCborStreamWriter &cbor)
{
    cbor.startMap(4);
    cbor.append("magic"_L1);
    cbor.append(packet.PacketMagicNumber);
    cbor.append("name"_L1);
    cbor.append(QString::fromUtf8(packet.stream_name));
    cbor.append("flags"_L1);
    cbor.append(packet.flags);

    cbor.append("data"_L1);
    if (m_compression > 0) {
        QByteArray compressed;
#if QT_CONFIG(zstd)
        if (m_requestedCompressionScheme == QStringLiteral("zstd"))
            compressed = zstdCompress(m_zstdCCtx, packet.stream_data, m_compression);
        else
#endif
        compressed = qCompress(packet.stream_data, m_compression);

        cbor.append(compressed);
    } else {
        cbor.append(packet.stream_data);
    }

    cbor.endMap();
}

bool QCtfServer::recognizedCompressionScheme() const
{
    if (m_requestedCompressionScheme.isEmpty())
        return true;
#if QT_CONFIG(zstd)
    if (m_requestedCompressionScheme == QStringLiteral("zstd"))
        return true;
#endif
    if (m_requestedCompressionScheme == QStringLiteral("zlib"))
        return true;
    return false;
}

void QCtfServer::run()
{
    m_server = new QTcpServer();
    QHostAddress addr;
    if (m_address.isEmpty())
        addr = QHostAddress(QHostAddress::Any);
    else
        addr = QHostAddress(m_address);

    qCInfo(lcCtfInfoTrace) << "Starting CTF server: " << m_address << ", port: " << m_port;

    while (m_stopping == 0) {
        if (!m_server->isListening()) {
            if (!m_server->listen(addr, m_port)) {
                qCInfo(lcCtfInfoTrace) << "Unable to start server";
                m_stopping = 1;
                setStatusAndNotify(Error);
            }
        }
        setStatusAndNotify(Idle);
        if (m_server->waitForNewConnection(-1)) {
            qCInfo(lcCtfInfoTrace) << "client connection";
            m_eventLoop = new QEventLoop();
            m_socket = m_server->nextPendingConnection();

            QObject::connect(m_socket, &QTcpSocket::readyRead, [&](){
                if (m_eventLoop) m_eventLoop->exit();
            });
            QObject::connect(m_socket, &QTcpSocket::bytesWritten, this, &QCtfServer::bytesWritten);
            QObject::connect(m_socket, &QTcpSocket::disconnected, [&](){
                if (m_eventLoop) m_eventLoop->exit();
            });

            m_server->close(); // Do not wait for more connections
            setStatusAndNotify(Connected);

            if (waitSocket())
            {
                QCborStreamReader cbor(m_socket);

                m_req = {};
                while (cbor.hasNext() && cbor.lastError() == QCborError::NoError)
                    readCbor(cbor);

                if (!m_req.isValid()) {
                    qCInfo(lcCtfInfoTrace) << "Invalid trace request.";
                    m_socket->close();
                } else {
                    m_compression = m_req.flags & CompressionMask;
#if QT_CONFIG(zstd)
                    m_compression = qMin(m_compression, ZSTD_maxCLevel());
#else
                    m_compression = qMin(m_compression, 9);
#endif
                    m_bufferOnIdle = !(m_req.flags & DontBufferOnIdle);

                    m_maxPackets = qMax(m_req.bufferSize / TracePacket::PacketSize, 16u);

                    if (!recognizedCompressionScheme()) {
                        qCWarning(lcCtfInfoTrace) << "Client requested unrecognized compression scheme: " << m_requestedCompressionScheme;
                        m_requestedCompressionScheme.clear();
                        m_compression = 0;
                    }

                    qCInfo(lcCtfInfoTrace) << "request received: " << m_req.sessionName << ", " << m_req.sessionTracepoints;

                    m_cb->handleSessionChange();
                    {
                        TraceResponse resp;
                        resp.serverId = ServerId;
                        resp.serverVersion = 1;
                        resp.serverName = QStringLiteral("Ctf Server");

                        QCborStreamWriter cbor(m_socket);
                        cbor.startMap(m_compression ? 4 : 3);
                        cbor.append("serverId"_L1);
                        cbor.append(resp.serverId);
                        cbor.append("serverVersion"_L1);
                        cbor.append(resp.serverVersion);
                        cbor.append("serverName"_L1);
                        cbor.append(resp.serverName);
                        if (m_compression) {
                            cbor.append("compressionScheme"_L1);
                            cbor.append(m_requestedCompressionScheme);
                        }
                        cbor.endMap();
                    }

                    qCInfo(lcCtfInfoTrace) << "response sent, sending data";
                    if (waitSocket()) {
                        while (m_socket->state() == QTcpSocket::ConnectedState) {
                            QList<TracePacket> packets;
                            {
                                QMutexLocker lock(&m_mutex);
                                while (m_packets.size() == 0)
                                    m_bufferHasData.wait(&m_mutex);
                                packets = std::exchange(m_packets, {});
                            }

                            {
                                QCborStreamWriter cbor(m_socket);
                                for (TracePacket &packet : packets) {
                                    writePacket(packet, cbor);
                                    if (!waitSocket())
                                        break;
                                }
                            }
                            qCInfo(lcCtfInfoTrace) << packets.size() << " packets written";
                        }
                    }

                    qCInfo(lcCtfInfoTrace) << "client connection closed";
                }
            }
            delete m_eventLoop;
            m_eventLoop = nullptr;
        } else {
            qCInfo(lcCtfInfoTrace) << "error: " << m_server->errorString();
            m_stopping = 1;
            setStatusAndNotify(Error);
        }
    }
}

void QCtfServer::startServer()
{
    start();
}
void QCtfServer::stopServer()
{
    this->m_stopping = 1;
    wait();
}

void QCtfServer::bufferData(const QString &stream, const QByteArray &data, quint32 flags)
{
    QMutexLocker lock(&m_mutex);
    TracePacket packet;
    packet.stream_name = stream.toUtf8();
    packet.stream_data = data;
    packet.flags = flags;
    m_packets.append(packet);
    if (m_packets.size() > m_maxPackets)
        m_packets.pop_front();
    m_bufferHasData.wakeOne();
}
