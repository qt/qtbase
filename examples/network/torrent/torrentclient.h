/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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

#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include <QBitArray>
#include <QHostAddress>
#include <QList>

class MetaInfo;
class PeerWireClient;
class TorrentClientPrivate;
class TorrentPeer;
class TorrentPiece;
QT_BEGIN_NAMESPACE
class QTimerEvent;
QT_END_NAMESPACE

class TorrentPeer {
public:
    QHostAddress address;
    quint16 port;
    QString id;
    bool interesting;
    bool seed;
    uint lastVisited;
    uint connectStart;
    uint connectTime;
    QBitArray pieces;
    int numCompletedPieces;

    inline bool operator==(const TorrentPeer &other)
    {
        return port == other.port
            && address == other.address
            && id == other.id;
    }
};

class TorrentClient : public QObject
{
    Q_OBJECT

public:
    enum State {
        Idle,
        Paused,
        Stopping,
        Preparing,
        Searching,
        Connecting,
        WarmingUp,
        Downloading,
        Endgame,
        Seeding
    };
    enum Error {
        UnknownError,
        TorrentParseError,
        InvalidTrackerError,
        FileError,
        ServerError
    };

    TorrentClient(QObject *parent = 0);
    ~TorrentClient();

    bool setTorrent(const QString &fileName);
    bool setTorrent(const QByteArray &torrentData);
    MetaInfo metaInfo() const;

    void setMaxConnections(int connections);
    int maxConnections() const;

    void setDestinationFolder(const QString &directory);
    QString destinationFolder() const;

    void setDumpedState(const QByteArray &dumpedState);
    QByteArray dumpedState() const;

    // Progress and stats for download feedback.
    qint64 progress() const;
    void setDownloadedBytes(qint64 bytes);
    qint64 downloadedBytes() const;
    void setUploadedBytes(qint64 bytes);
    qint64 uploadedBytes() const;
    int connectedPeerCount() const;
    int seedCount() const;

    // Accessors for the tracker
    QByteArray peerId() const;
    QByteArray infoHash() const;
    quint16 serverPort() const;

    // State and error.
    State state() const;
    QString stateString() const;
    Error error() const;
    QString errorString() const;

signals:
    void stateChanged(TorrentClient::State state);
    void error(TorrentClient::Error error);

    void downloadCompleted();
    void peerInfoUpdated();

    void dataSent(int uploadedBytes);
    void dataReceived(int downloadedBytes);
    void progressUpdated(int percentProgress);
    void downloadRateUpdated(int bytesPerSecond);
    void uploadRateUpdated(int bytesPerSecond);

    void stopped();

public slots:
    void start();
    void stop();
    void setPaused(bool paused);
    void setupIncomingConnection(PeerWireClient *client);

protected slots:
    void timerEvent(QTimerEvent *event) override;

private slots:
    // File management
    void sendToPeer(int readId, int pieceIndex, int begin, const QByteArray &data);
    void fullVerificationDone();
    void pieceVerified(int pieceIndex, bool ok);
    void handleFileError();

    // Connection handling
    void connectToPeers();
    QList<TorrentPeer *> weighedFreePeers() const;
    void setupOutgoingConnection();
    void initializeConnection(PeerWireClient *client);
    void removeClient();
    void peerPiecesAvailable(const QBitArray &pieces);
    void peerRequestsBlock(int pieceIndex, int begin, int length);
    void blockReceived(int pieceIndex, int begin, const QByteArray &data);
    void peerWireBytesWritten(qint64 bytes);
    void peerWireBytesReceived(qint64 bytes);
    int blocksLeftForPiece(const TorrentPiece *piece) const;

    // Scheduling
    void scheduleUploads();
    void scheduleDownloads();
    void schedulePieceForClient(PeerWireClient *client);
    void requestMore(PeerWireClient *client);
    int requestBlocks(PeerWireClient *client, TorrentPiece *piece, int maxBlocks);
    void peerChoked();
    void peerUnchoked();

    // Tracker handling
    void addToPeerList(const QList<TorrentPeer> &peerList);
    void trackerStopped();

    // Progress
    void updateProgress(int progress = -1);

private:
    TorrentClientPrivate *d;
    friend class TorrentClientPrivate;
};

#endif
