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
#include <QDebug>

TransferItem::TransferItem(const QNetworkRequest &r, const QString &u, const QString &p, QNetworkAccessManager &n, Method m)
    : method(m), request(r), reply(0), nam(n), inputFile(0), outputFile(0), user(u), password(p)
{
}

void TransferItem::progress(qint64 sent, qint64 total)
{
    if (total > 0)
        qDebug() << (sent*100/total) << "%";
    else
        qDebug() << sent << "B";
}

void TransferItem::start()
{
    switch (method) {
    case Get:
        reply = nam.get(request);
        connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
        connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(progress(qint64,qint64)));
        break;
    case Put:
        reply = nam.put(request, inputFile);
        connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(progress(qint64,qint64)));
        break;
    case Post:
        reply = nam.post(request, inputFile);
        connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(progress(qint64,qint64)));
        break;
    }
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

DownloadItem::DownloadItem(const QNetworkRequest &r, const QString &user, const QString &password, QNetworkAccessManager &manager)
    : TransferItem(r, user, password, manager, Get)
{
}

DownloadItem::~DownloadItem()
{
}

void DownloadItem::readyRead()
{
    if (!outputFile)
        outputFile = new QFile(this);
    if (!outputFile->isOpen()) {
        qDebug() << reply->header(QNetworkRequest::ContentTypeHeader) << reply->header(QNetworkRequest::ContentLengthHeader);
        QString path = reply->url().path();
        path = path.mid(path.lastIndexOf('/') + 1);
        if (path.isEmpty())
            path = QLatin1String("index.html");
        outputFile->setFileName(path);
        for (int i=1;i<1000;i++) {
            if (!outputFile->exists() && outputFile->open(QIODevice::WriteOnly | QIODevice::Truncate))
                break;
            outputFile->setFileName(QString(QLatin1String("%1.%2")).arg(path).arg(i));
        }
        if (!outputFile->isOpen()) {
            qDebug() << "couldn't open output file";
            reply->abort();
            return;
        }
        qDebug() << reply->url() << " -> " << outputFile->fileName();
    }
    outputFile->write(reply->readAll());
}

void DownloadItem::finished()
{
    if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
        QUrl url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        url = reply->url().resolved(url);
        qDebug() << reply->url() << "redirected to " << url;
        if (redirects.contains(url)) {
            qDebug() << "redirect loop detected";
        } else if (redirects.count() > 10) {
            qDebug() << "too many redirects";
        } else {
            //follow redirect
            if (outputFile && outputFile->isOpen()) {
                if (!outputFile->seek(0) || !outputFile->resize(0)) {
                    outputFile->close();
                    outputFile->remove();
                }
            }
            reply->deleteLater();
            reply = nam.get(QNetworkRequest(url));
            reply->setParent(this);
            connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
            connect(reply, SIGNAL(finished()), this, SLOT(finished()));
            redirects.append(url);
            return;
        }
    }
    if (outputFile && outputFile->isOpen()) {
        outputFile->write(reply->readAll());
        outputFile->close();
    }
    emit downloadFinished(this);
}

UploadItem::UploadItem(const QNetworkRequest &r, const QString &user, const QString &password, QNetworkAccessManager &n, QFile *f, TransferItem::Method method)
    : TransferItem(r,user, password, n,method)
{
    inputFile = f;
    f->setParent(this);
    qDebug() << f->fileName() << f->isOpen() << f->size();
}

UploadItem::~UploadItem()
{
}

void UploadItem::finished()
{
    emit downloadFinished(this);
}
