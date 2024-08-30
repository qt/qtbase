// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "bencodeparser.h"
#include "connectionmanager.h"
#include "torrentclient.h"
#include "torrentserver.h"
#include "trackerclient.h"

#include <QtCore>
#include <QNetworkRequest>

TrackerClient::TrackerClient(TorrentClient *downloader, QObject *parent)
    : QObject(parent), torrentDownloader(downloader)
{
    connect(&http, &QNetworkAccessManager::finished,
            this, &TrackerClient::httpRequestDone);
}

void TrackerClient::start(const MetaInfo &info)
{
    metaInfo = info;
    QTimer::singleShot(0, this, &TrackerClient::fetchPeerList);

    if (metaInfo.fileForm() == MetaInfo::SingleFileForm) {
        length = metaInfo.singleFile().length;
    } else {
        QList<MetaInfoMultiFile> files = metaInfo.multiFiles();
        for (int i = 0; i < files.size(); ++i)
            length += files.at(i).length;
    }
}

void TrackerClient::startSeeding()
{
    firstSeeding = true;
    fetchPeerList();
}

void TrackerClient::stop()
{
    lastTrackerRequest = true;
    fetchPeerList();
}

void TrackerClient::timerEvent(QTimerEvent *event)
{
    if (event->id() == requestIntervalTimer.id()) {
        fetchPeerList();
    } else {
        QObject::timerEvent(event);
    }
}

void TrackerClient::fetchPeerList()
{
    if (metaInfo.announceUrl().isEmpty())
        return;
    QUrl url(metaInfo.announceUrl());

    // Base the query on announce url to include a passkey (if any)
    QUrlQuery query(url);

    // Percent encode the hash
    const QByteArray infoHash = torrentDownloader->infoHash();
    const QByteArray encodedSum = infoHash.toPercentEncoding();

    bool seeding = (torrentDownloader->state() == TorrentClient::Seeding);

    query.addQueryItem("info_hash", encodedSum);
    query.addQueryItem("peer_id", ConnectionManager::instance()->clientId());
    query.addQueryItem("port", QByteArray::number(TorrentServer::instance()->serverPort()));
    query.addQueryItem("compact", "1");
    query.addQueryItem("uploaded", QByteArray::number(torrentDownloader->uploadedBytes()));

    if (!firstSeeding) {
        query.addQueryItem("downloaded", "0");
        query.addQueryItem("left", "0");
    } else {
        query.addQueryItem("downloaded",
                           QByteArray::number(torrentDownloader->downloadedBytes()));
        int left = qMax<int>(0, metaInfo.totalSize() - torrentDownloader->downloadedBytes());
        query.addQueryItem("left", QByteArray::number(seeding ? 0 : left));
    }

    if (seeding && firstSeeding) {
        query.addQueryItem("event", "completed");
        firstSeeding = false;
    } else if (firstTrackerRequest) {
        firstTrackerRequest = false;
        query.addQueryItem("event", "started");
    } else if(lastTrackerRequest) {
        query.addQueryItem("event", "stopped");
    }

    if (!trackerId.isEmpty())
        query.addQueryItem("trackerid", trackerId);

    url.setQuery(query);

    QNetworkRequest req(url);
    if (!url.userName().isEmpty()) {
        uname = url.userName();
        pwd = url.password();
        connect(&http, &QNetworkAccessManager::authenticationRequired,
                this, &TrackerClient::provideAuthentication);
    }
    http.get(req);
}

void TrackerClient::httpRequestDone(QNetworkReply *reply)
{
    reply->deleteLater();
    if (lastTrackerRequest) {
        emit stopped();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit connectionError(reply->error());
        return;
    }

    QByteArray response = reply->readAll();
    reply->abort();

    BencodeParser parser;
    if (!parser.parse(response)) {
        qWarning("Error parsing bencode response from tracker: %s",
                 qPrintable(parser.errorString()));
        return;
    }

    QMap<QByteArray, QVariant> dict = parser.dictionary();

    if (dict.contains("failure reason")) {
        // no other items are present
        emit failure(QString::fromUtf8(dict.value("failure reason").toByteArray()));
        return;
    }

    if (dict.contains("warning message")) {
        // continue processing
        emit warning(QString::fromUtf8(dict.value("warning message").toByteArray()));
    }

    if (dict.contains("tracker id")) {
        // store it
        trackerId = dict.value("tracker id").toByteArray();
    }

    if (dict.contains("interval")) {
        // Mandatory item
        requestIntervalTimer.start(std::chrono::seconds(dict.value("interval").toInt()), this);
    }

    if (dict.contains("peers")) {
        // store it
        peers.clear();
        QVariant peerEntry = dict.value("peers");
        if (peerEntry.userType() == QMetaType::QVariantList) {
            QList<QVariant> peerTmp = peerEntry.toList();
            for (int i = 0; i < peerTmp.size(); ++i) {
                TorrentPeer tmp;
                QMap<QByteArray, QVariant> peer = qvariant_cast<QMap<QByteArray, QVariant> >(peerTmp.at(i));
                tmp.id = QString::fromUtf8(peer.value("peer id").toByteArray());
                tmp.address.setAddress(QString::fromUtf8(peer.value("ip").toByteArray()));
                tmp.port = peer.value("port").toInt();
                peers << tmp;
            }
        } else {
            QByteArray peerTmp = peerEntry.toByteArray();
            for (int i = 0; i < peerTmp.size(); i += 6) {
                TorrentPeer tmp;
                uchar *data = (uchar *)peerTmp.constData() + i;
                tmp.port = (int(data[4]) << 8) + data[5];
                uint ipAddress = 0;
                ipAddress += uint(data[0]) << 24;
                ipAddress += uint(data[1]) << 16;
                ipAddress += uint(data[2]) << 8;
                ipAddress += uint(data[3]);
                tmp.address.setAddress(ipAddress);
                peers << tmp;
            }
        }
        emit peerListUpdated(peers);
    }
}

void TrackerClient::provideAuthentication(QNetworkReply *reply, QAuthenticator *auth)
{
    Q_UNUSED(reply);
    auth->setUser(uname);
    auth->setPassword(pwd);
}
