// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QtNetwork>
#include <QtCore>

#include "textprogressbar.h"

class DownloadManager: public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);

    void append(const QUrl &url);
    void append(const QStringList &urls);
    static QString saveFileName(const QUrl &url);

signals:
    void finished();

private slots:
    void startNextDownload();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void downloadReadyRead();

private:
    bool isHttpRedirect() const;
    void reportRedirect();

    QNetworkAccessManager manager;
    QQueue<QUrl> downloadQueue;
    QNetworkReply *currentDownload = nullptr;
    QFile output;
    QElapsedTimer downloadTimer;
    TextProgressBar progressBar;

    int downloadedCount = 0;
    int totalCount = 0;
};

#endif
