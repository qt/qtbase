// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef QGET_H
#define QGET_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

class TransferItem : public QObject
{
    Q_OBJECT
public:
    enum Method {Get,Put,Post};
    TransferItem(const QNetworkRequest &r, const QString &u, const QString &p, QNetworkAccessManager &n, Method m);
    void start();
signals:
    void downloadFinished(TransferItem *self);
public slots:
    void progress(qint64,qint64);
public:
    Method method;
    QNetworkRequest request;
    QNetworkReply *reply;
    QNetworkAccessManager &nam;
    QFile *inputFile;
    QFile *outputFile;
    QList<QUrl> redirects;
    QString user;
    QString password;
};

class DownloadItem : public TransferItem
{
    Q_OBJECT
public:
    DownloadItem(const QNetworkRequest &r, const QString &user, const QString &password, QNetworkAccessManager &nam);
    ~DownloadItem();

private slots:
    void readyRead();
    void finished();
private:
};

class UploadItem : public TransferItem
{
    Q_OBJECT
public:
    UploadItem(const QNetworkRequest &r, const QString &user, const QString &password, QNetworkAccessManager &nam, QFile *f, TransferItem::Method method);
    ~UploadItem();
private slots:
    void finished();
};

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    DownloadManager();
    ~DownloadManager();
    void get(const QNetworkRequest &request, const QString &user, const QString &password);
    void upload(const QNetworkRequest &request, const QString &user, const QString &password, const QString &filename, TransferItem::Method method);
    void setProxy(const QNetworkProxy &proxy) { nam.setProxy(proxy); }
    void setProxyUser(const QString &user) { proxyUser = user; }
    void setProxyPassword(const QString &password) { proxyPassword = password; }
    enum QueueMode { Parallel, Serial };
    void setQueueMode(QueueMode mode);

public slots:
    void checkForAllDone();

private slots:
    void finished(QNetworkReply *reply);
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
#endif
    void downloadFinished(TransferItem *item);
private:
    TransferItem *findTransfer(QNetworkReply *reply);

    QNetworkAccessManager nam;
    QList<TransferItem*> transfers;
    QString proxyUser;
    QString proxyPassword;
    QueueMode queueMode;
};

#endif // QGET_H
