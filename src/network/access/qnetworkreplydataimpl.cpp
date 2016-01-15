/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnetworkreplydataimpl_p.h"
#include "private/qdataurl_p.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>

QT_BEGIN_NAMESPACE

QNetworkReplyDataImplPrivate::QNetworkReplyDataImplPrivate()
    : QNetworkReplyPrivate()
{
}

QNetworkReplyDataImplPrivate::~QNetworkReplyDataImplPrivate()
{
}

QNetworkReplyDataImpl::~QNetworkReplyDataImpl()
{
}

QNetworkReplyDataImpl::QNetworkReplyDataImpl(QObject *parent, const QNetworkRequest &req, const QNetworkAccessManager::Operation op)
    : QNetworkReply(*new QNetworkReplyDataImplPrivate(), parent)
{
    Q_D(QNetworkReplyDataImpl);
    setRequest(req);
    setUrl(req.url());
    setOperation(op);
    setFinished(true);
    QNetworkReply::open(QIODevice::ReadOnly);

    QUrl url = req.url();
    QString mimeType;
    QByteArray payload;
    if (qDecodeDataUrl(url, mimeType, payload)) {
        qint64 size = payload.size();
        setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
        setHeader(QNetworkRequest::ContentLengthHeader, size);
        QMetaObject::invokeMethod(this, "metaDataChanged", Qt::QueuedConnection);

        d->decodedData.setData(payload);
        d->decodedData.open(QIODevice::ReadOnly);

        QMetaObject::invokeMethod(this, "downloadProgress", Qt::QueuedConnection,
                                  Q_ARG(qint64,size), Q_ARG(qint64, size));
        QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    } else {
        // something wrong with this URI
        const QString msg = QCoreApplication::translate("QNetworkAccessDataBackend",
                                                        "Invalid URI: %1").arg(url.toString());
        setError(QNetworkReply::ProtocolFailure, msg);
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ProtocolFailure));
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    }
}

void QNetworkReplyDataImpl::close()
{
    QNetworkReply::close();
}

void QNetworkReplyDataImpl::abort()
{
    QNetworkReply::close();
}

qint64 QNetworkReplyDataImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyDataImpl);
    return QNetworkReply::bytesAvailable() + d->decodedData.bytesAvailable();
}

bool QNetworkReplyDataImpl::isSequential () const
{
    return true;
}

qint64 QNetworkReplyDataImpl::size() const
{
    Q_D(const QNetworkReplyDataImpl);
    return d->decodedData.size();
}

/*!
    \internal
*/
qint64 QNetworkReplyDataImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyDataImpl);

    // TODO idea:
    // Instead of decoding the whole data into new memory, we could decode on demand.
    // Note that this might be tricky to do.

    return d->decodedData.read(data, maxlen);
}


QT_END_NAMESPACE

#include "moc_qnetworkreplydataimpl_p.cpp"

