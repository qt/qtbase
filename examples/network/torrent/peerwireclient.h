// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PEERWIRECLIENT_H
#define PEERWIRECLIENT_H

#include <QBitArray>
#include <QList>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
class QHostAddress;
class QTimerEvent;
QT_END_NAMESPACE
class TorrentPeer;

struct TorrentBlock
{
    inline TorrentBlock(qint32 p, qint32 o, qint32 l)
        : pieceIndex(p), offset(o), length(l)
    {
    }
    inline bool operator==(const TorrentBlock &other) const
    {
        return pieceIndex == other.pieceIndex
                && offset == other.offset
                && length == other.length;
    }

    qint32 pieceIndex;
    qint32 offset;
    qint32 length;
};

class PeerWireClient : public QTcpSocket
{
    Q_OBJECT

public:
    enum PeerWireStateFlag {
        ChokingPeer = 0x1,
        InterestedInPeer = 0x2,
        ChokedByPeer = 0x4,
        PeerIsInterested = 0x8
    };
    Q_DECLARE_FLAGS(PeerWireState, PeerWireStateFlag)

    explicit PeerWireClient(const QByteArray &peerId, QObject *parent = nullptr);
    void initialize(const QByteArray &infoHash, int pieceCount);

    void setPeer(TorrentPeer *peer);
    TorrentPeer *peer() const;

    // State
    inline PeerWireState peerWireState() const { return pwState; }
    QBitArray availablePieces() const;
    QList<TorrentBlock> incomingBlocks() const;

    // Protocol
    void chokePeer();
    void unchokePeer();
    void sendInterested();
    void sendKeepAlive();
    void sendNotInterested();
    void sendPieceNotification(int piece);
    void sendPieceList(const QBitArray &bitField);
    void requestBlock(int piece, int offset, int length);
    void cancelRequest(int piece, int offset, int length);
    void sendBlock(int piece, int offset, const QByteArray &data);

    // Rate control
    qint64 writeToSocket(qint64 bytes);
    qint64 readFromSocket(qint64 bytes);
    qint64 downloadSpeed() const;
    qint64 uploadSpeed() const;

    bool canTransferMore() const;
    qint64 bytesAvailable() const override { return incomingBuffer.size() + QTcpSocket::bytesAvailable(); }
    qint64 socketBytesAvailable() const { return socket.bytesAvailable(); }
    qint64 socketBytesToWrite() const { return socket.bytesToWrite(); }

    void setReadBufferSize(qint64 size) override;

    using QTcpSocket::connectToHost;
    void connectToHost(const QString &address, quint16 port, OpenMode openMode = ReadWrite,
                       NetworkLayerProtocol protocol = AnyIPProtocol) override;
    void diconnectFromHost();

signals:
    void infoHashReceived(const QByteArray &infoHash);
    void readyToTransfer();

    void choked();
    void unchoked();
    void interested();
    void notInterested();

    void piecesAvailable(const QBitArray &pieces);
    void blockRequested(int pieceIndex, int begin, int length);
    void blockReceived(int pieceIndex, int begin, const QByteArray &data);

    void bytesReceived(qint64 size);

protected:
    void timerEvent(QTimerEvent *event) override;

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 readLineData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private slots:
    void sendHandShake();
    void processIncomingData();
    void socketStateChanged(QAbstractSocket::SocketState state);

private:
    // Data waiting to be read/written
    QByteArray incomingBuffer;
    QByteArray outgoingBuffer;

    struct BlockInfo {
        int pieceIndex;
        int offset;
        int length;
        QByteArray block;
    };
    QList<BlockInfo> pendingBlocks;
    int pendingBlockSizes;
    QList<TorrentBlock> incoming;

    enum PacketType {
        ChokePacket = 0,
        UnchokePacket = 1,
        InterestedPacket = 2,
        NotInterestedPacket = 3,
        HavePacket = 4,
        BitFieldPacket = 5,
        RequestPacket = 6,
        PiecePacket = 7,
        CancelPacket = 8
    };

    // State
    PeerWireState pwState;
    bool receivedHandShake;
    bool gotPeerId;
    bool sentHandShake;
    int nextPacketLength;

    // Upload/download speed records
    qint64 uploadSpeedData[8];
    qint64 downloadSpeedData[8];
    int transferSpeedTimer;

    // Timeout handling
    int timeoutTimer;
    int pendingRequestTimer;
    bool invalidateTimeout;
    int keepAliveTimer;

    // Checksum, peer ID and set of available pieces
    QByteArray infoHash;
    QByteArray peerIdString;
    QBitArray peerPieces;
    TorrentPeer *torrentPeer;

    QTcpSocket socket;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PeerWireClient::PeerWireState)

#endif
