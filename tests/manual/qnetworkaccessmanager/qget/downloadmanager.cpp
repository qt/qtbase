/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qget.h"
#include <QAuthenticator>
#include <QCoreApplication>
#include <QDebug>
#include <QSslError>

DownloadManager::DownloadManager()
    : queueMode (Parallel)
{
    connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)));
    connect(&nam, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(authenticationRequired(QNetworkReply*,QAuthenticator*)));
    connect(&nam, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), this, SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
#ifndef QT_NO_SSL
    connect(&nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
#endif
}

DownloadManager::~DownloadManager()
{

}

void DownloadManager::get(const QNetworkRequest &request, const QString &user, const QString &password)
{
    DownloadItem *dl = new DownloadItem(request, user, password, nam);
    transfers.append(dl);
    connect(dl, SIGNAL(downloadFinished(TransferItem*)), SLOT(downloadFinished(TransferItem*)));
}

void DownloadManager::upload(const QNetworkRequest &request, const QString &user, const QString &password, const QString &filename, TransferItem::Method method)
{
    QScopedPointer<QFile> file(new QFile(filename));
    if (!file->open(QFile::ReadOnly)) {
        qDebug() << "Can't open input file" << file->fileName() << file->errorString();
        return;
    }
    UploadItem *ul = new UploadItem(request, user, password, nam, file.take(), method);
    transfers.append(ul);
    connect(ul, SIGNAL(downloadFinished(TransferItem*)), SLOT(downloadFinished(TransferItem*)));
}

void DownloadManager::finished(QNetworkReply *)
{
}

void DownloadManager::downloadFinished(TransferItem *item)
{
    qDebug() << "finished " << item->reply->url() << " with http status: " << item->reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (item->reply->error() != QNetworkReply::NoError)
        qDebug() << "and error: " << item->reply->error() << item->reply->errorString();
    transfers.removeOne(item);
    item->deleteLater();
    checkForAllDone();
}

void DownloadManager::checkForAllDone()
{
    if (transfers.isEmpty()) {
        qDebug() << "All Done.";
        QCoreApplication::quit();
    }

    foreach (TransferItem *item, transfers) {
        if (!item->reply) {
            item->start();
            //by default multiple downloads are processed in parallel.
            //but in serial mode, only start one transfer at a time.
            if (queueMode == Serial)
                break;
        }
    }

}

void DownloadManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *auth)
{
    qDebug() << "authenticationRequired" << reply;
    TransferItem *transfer = findTransfer(reply);
    //provide the credentials exactly once, so that it fails if credentials are incorrect.
    if ((transfer && !transfer->user.isEmpty()) || !transfer->password.isEmpty()) {
        auth->setUser(transfer->user);
        auth->setPassword(transfer->password);
        transfer->user.clear();
        transfer->password.clear();
    }
}

void DownloadManager::proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
{
    //provide the credentials exactly once, so that it fails if credentials are incorrect.
    if (!proxyUser.isEmpty() || !proxyPassword.isEmpty()) {
        auth->setUser(proxyUser);
        auth->setPassword(proxyPassword);
        proxyUser.clear();
        proxyPassword.clear();
    }
}

#ifndef QT_NO_SSL
void DownloadManager::sslErrors(QNetworkReply *, const QList<QSslError> &errors)
{
    qDebug() << "sslErrors";
    foreach (const QSslError &error, errors) {
        qDebug() << error.errorString();
        qDebug() << error.certificate().toPem();
    }
}
#endif

TransferItem *DownloadManager::findTransfer(QNetworkReply *reply)
{
    foreach (TransferItem *item, transfers) {
        if (item->reply == reply)
            return item;
    }
    return 0;
}

void DownloadManager::setQueueMode(QueueMode mode)
{
    queueMode = mode;
}
