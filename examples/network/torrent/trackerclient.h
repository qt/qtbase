// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include <QBasicTimer>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAuthenticator>

#include "metainfo.h"
#include "torrentclient.h"

class TorrentClient;

class TrackerClient : public QObject
{
    Q_OBJECT

public:
    explicit TrackerClient(TorrentClient *downloader, QObject *parent = nullptr);

    void start(const MetaInfo &info);
    void stop();
    void startSeeding();

signals:
    void connectionError(QNetworkReply::NetworkError error);

    void failure(const QString &reason);
    void warning(const QString &message);
    void peerListUpdated(const QList<TorrentPeer> &peerList);

    void uploadCountUpdated(qint64 newUploadCount);
    void downloadCountUpdated(qint64 newDownloadCount);

    void stopped();

protected:
    void timerEvent(QTimerEvent *event) override;

private slots:
    void fetchPeerList();
    void httpRequestDone(QNetworkReply *reply);
    void provideAuthentication(QNetworkReply *reply, QAuthenticator *auth);

private:
    TorrentClient *torrentDownloader;

    QBasicTimer requestIntervalTimer;
    QNetworkAccessManager http;
    MetaInfo metaInfo;
    QByteArray trackerId;
    QList<TorrentPeer> peers;
    qint64 length = 0;
    QString uname;
    QString pwd;

    bool firstTrackerRequest = true;
    bool lastTrackerRequest = false;
    bool firstSeeding = true;
};

#endif
