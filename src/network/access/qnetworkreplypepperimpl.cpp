/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnetworkreplypepperimpl_p.h"

#include "QtCore/qdatetime.h"
#include <QtCore/QCoreApplication>
#include <QDebug>
#include <qglobal.h>
#include <cstring>

#include "geturl_handler.h"

using namespace pp;

QT_BEGIN_NAMESPACE

QNetworkReplyPepperImplPrivate::QNetworkReplyPepperImplPrivate(QNetworkReplyPepperImpl *q)
    : QNetworkReplyPrivate()
    ,dataPosition(0)
{

}

QNetworkReplyPepperImpl::~QNetworkReplyPepperImpl()
{
}

void pepperLoadData(void *context)
{
    reinterpret_cast<QNetworkReplyPepperImpl *>(context)->loadData();
}

void QNetworkReplyPepperImpl::loadData()
{
    Q_D(QNetworkReplyPepperImpl);

    extern void *qtPepperInstance; // qglobal.cpp

    // in order to simplify development and start from known-good code I'm
    // using the GetUrlHandler from the photo naclports example here.
    GetURLHandler* handler = GetURLHandler::Create(reinterpret_cast<pp::Instance *>(qtPepperInstance),
                                                   std::string(url().toString().toLatin1()),
                                                   d);
    handler->Start();
}

void QNetworkReplyPepperImpl::didCompleteIO(int32_t result)
{
    Q_D(QNetworkReplyPepperImpl);
    //qDebug() << "didCompleteIO" << result;
}

QNetworkReplyPepperImpl::QNetworkReplyPepperImpl(QObject *parent, const QNetworkRequest &req, const QNetworkAccessManager::Operation op)
    : QNetworkReply(*new QNetworkReplyPepperImplPrivate(this), parent)
{
    QNetworkReplyPepperImplPrivate *d = (QNetworkReplyPepperImplPrivate*) d_func();

    setRequest(req);
    setUrl(req.url());
    setOperation(op);

    qDebug() << "NetworkReplyPepperImpl::loading" << req.url();

    // Block and load data. At the time of writing ppapi
    // does not support calling the API on secondary threads,
    // schedule the call on the pepper thread and wait.
    //QMutexLocker lock(&d->mutex);
    //qtRunOnPepperThread(pepperLoadData, this);
    //d->dataReady.wait(&d->mutex);
    pepperLoadData(this);

    qDebug() << "NetworkReplyPepperImpl::loaded" <<req.url() << d->data.count();

    setFinished(true);
    QNetworkReply::open(QIODevice::ReadOnly);

    if (d->state == PP_OK && d->data.count() > 0) {
        //qDebug() << "QNetworkReplyPepperImpl::got file";

        //setHeader(QNetworkRequest::LastModifiedHeader, fi.lastModified());
        setHeader(QNetworkRequest::ContentLengthHeader, d->data.count());

        QMetaObject::invokeMethod(this, "metaDataChanged", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "downloadProgress", Qt::QueuedConnection,
            Q_ARG(qint64, d->data.count()), Q_ARG(qint64, d->data.count()));
        QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    } else {
        //qDebug() << "QNetworkReplyPepperImpl error";

        QString msg = QCoreApplication::translate("QNetworkAccessPepperBackend", "Error opening %1")
                .arg(d->url.toString());
        setError(QNetworkReply::ContentNotFoundError, msg);
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
            Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentNotFoundError));
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    }
}

void QNetworkReplyPepperImpl::close()
{
    Q_D(QNetworkReplyPepperImpl);
    QNetworkReply::close();

}

void QNetworkReplyPepperImpl::abort()
{
    Q_D(QNetworkReplyPepperImpl);
    QNetworkReply::close();
}

qint64 QNetworkReplyPepperImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyPepperImpl);
    quint64 available = QNetworkReply::bytesAvailable() + d->data.count() - d->dataPosition;
    //qDebug() << "QNetworkReplyPepperImpl::bytesAvailable" << available;
    return available;
}

bool QNetworkReplyPepperImpl::isSequential () const
{
    return true;
}

qint64 QNetworkReplyPepperImpl::size() const
{
    Q_D(const QNetworkReplyPepperImpl);
    return d->data.count();
}

/*!
    \internal
*/
qint64 QNetworkReplyPepperImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyPepperImpl);

    qint64 available = d->data.count() - d->dataPosition;

    if (available <= 0)
        return -1;

    qint64 toRead = qMin(maxlen, available);
    std::memcpy(data, d->data.constData() + d->dataPosition, toRead);
    d->dataPosition += toRead;

    //qDebug() << "QNetworkReplyPepperImpl::readData" << toRead;

    return toRead;
}


QT_END_NAMESPACE

