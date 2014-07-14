/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    inline TorrentBlock(int p, int o, int l)
        : pieceIndex(p), offset(o), length(l)
    {
    }
    inline bool operator==(const TorrentBlock &other) const
    {
        return pieceIndex == other.pieceIndex
                && offset == other.offset
                && length == other.length;
    }

    int pieceIndex;
    int offset;
    int length;
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

    explicit PeerWireClient(const QByteArray &peerId, QObject *parent = 0);
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
    qint64 bytesAvailable() const Q_DECL_OVERRIDE { return incomingBuffer.size() + QTcpSocket::bytesAvailable(); }
    qint64 socketBytesAvailable() const { return socket.bytesAvailable(); }
    qint64 socketBytesToWrite() const { return socket.bytesToWrite(); }

    void setReadBufferSize(qint64 size) Q_DECL_OVERRIDE;

    void connectToHost(const QHostAddress &address,
                       quint16 port, OpenMode openMode = ReadWrite) Q_DECL_OVERRIDE;
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
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

    qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 readLineData(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 writeData(const char *data, qint64 len) Q_DECL_OVERRIDE;

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
