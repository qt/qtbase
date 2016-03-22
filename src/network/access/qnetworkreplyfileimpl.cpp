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

#include "qnetworkreplyfileimpl_p.h"

#include "QtCore/qdatetime.h"
#include "qnetworkaccessmanager_p.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include "qnetworkfile_p.h"
#include "qnetworkrequest.h"

QT_BEGIN_NAMESPACE

QNetworkReplyFileImplPrivate::QNetworkReplyFileImplPrivate()
    : QNetworkReplyPrivate(), managerPrivate(0), realFile(0)
{
    qRegisterMetaType<QNetworkRequest::KnownHeaders>();
    qRegisterMetaType<QNetworkReply::NetworkError>();
}

QNetworkReplyFileImpl::~QNetworkReplyFileImpl()
{
    QNetworkReplyFileImplPrivate *d = (QNetworkReplyFileImplPrivate*) d_func();
    if (d->realFile) {
        if (d->realFile->thread() == QThread::currentThread())
            delete d->realFile;
        else
            QMetaObject::invokeMethod(d->realFile, "deleteLater", Qt::QueuedConnection);
    }
}

QNetworkReplyFileImpl::QNetworkReplyFileImpl(QNetworkAccessManager *manager, const QNetworkRequest &req, const QNetworkAccessManager::Operation op)
    : QNetworkReply(*new QNetworkReplyFileImplPrivate(), manager)
{
    setRequest(req);
    setUrl(req.url());
    setOperation(op);
    QNetworkReply::open(QIODevice::ReadOnly);

    QNetworkReplyFileImplPrivate *d = (QNetworkReplyFileImplPrivate*) d_func();

    d->managerPrivate = manager->d_func();

    QUrl url = req.url();
    if (url.host() == QLatin1String("localhost"))
        url.setHost(QString());

#if !defined(Q_OS_WIN)
    // do not allow UNC paths on Unix
    if (!url.host().isEmpty()) {
        // we handle only local files
        QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Request for opening non-local file %1").arg(url.toString());
        setError(QNetworkReply::ProtocolInvalidOperationError, msg);
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
            Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ProtocolInvalidOperationError));
        fileOpenFinished(false);
        return;
    }
#endif
    if (url.path().isEmpty())
        url.setPath(QLatin1String("/"));
    setUrl(url);

    QString fileName = url.toLocalFile();
    if (fileName.isEmpty()) {
        const QString scheme = url.scheme();
        if (scheme == QLatin1String("qrc")) {
            fileName = QLatin1Char(':') + url.path();
        } else {
#if defined(Q_OS_ANDROID)
            if (scheme == QLatin1String("assets"))
                fileName = QLatin1String("assets:") + url.path();
            else
#endif
                fileName = url.toString(QUrl::RemoveAuthority | QUrl::RemoveFragment | QUrl::RemoveQuery);
        }
    }

    if (req.attribute(QNetworkRequest::BackgroundRequestAttribute).toBool()) { // Asynchronous open
        auto realFile = new QNetworkFile(fileName);
        connect(realFile, &QNetworkFile::headerRead, this, &QNetworkReplyFileImpl::setHeader,
                Qt::QueuedConnection);
        connect(realFile, &QNetworkFile::error, this, &QNetworkReplyFileImpl::setError,
                Qt::QueuedConnection);
        connect(realFile, SIGNAL(finished(bool)), SLOT(fileOpenFinished(bool)),
                Qt::QueuedConnection);

        realFile->moveToThread(d->managerPrivate->createThread());
        QMetaObject::invokeMethod(realFile, "open", Qt::QueuedConnection);

        d->realFile = realFile;
    } else { // Synch open
        setFinished(true);

        QFileInfo fi(fileName);
        if (fi.isDir()) {
            QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Cannot open %1: Path is a directory").arg(url.toString());
            setError(QNetworkReply::ContentOperationNotPermittedError, msg);
            QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentOperationNotPermittedError));
            QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
            return;
        }
        d->realFile = new QFile(fileName, this);
        bool opened = d->realFile->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

        // could we open the file?
        if (!opened) {
            QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Error opening %1: %2")
                    .arg(d->realFile->fileName(), d->realFile->errorString());

            if (fi.exists()) {
                setError(QNetworkReply::ContentAccessDenied, msg);
                QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                    Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentAccessDenied));
            } else {
                setError(QNetworkReply::ContentNotFoundError, msg);
                QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                    Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentNotFoundError));
            }
            QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
            return;
        }
        setHeader(QNetworkRequest::LastModifiedHeader, fi.lastModified());
        setHeader(QNetworkRequest::ContentLengthHeader, fi.size());

        QMetaObject::invokeMethod(this, "metaDataChanged", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "downloadProgress", Qt::QueuedConnection,
            Q_ARG(qint64, fi.size()), Q_ARG(qint64, fi.size()));
        QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    }
}

void QNetworkReplyFileImpl::close()
{
    Q_D(QNetworkReplyFileImpl);
    QNetworkReply::close();
    if (d->realFile) {
        if (d->realFile->thread() == thread())
            d->realFile->close();
        else
            QMetaObject::invokeMethod(d->realFile, "close", Qt::QueuedConnection);
    }
}

void QNetworkReplyFileImpl::abort()
{
    close();
}

qint64 QNetworkReplyFileImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyFileImpl);
    if (!d->isFinished || !d->realFile || !d->realFile->isOpen())
        return QNetworkReply::bytesAvailable();
    return QNetworkReply::bytesAvailable() + d->realFile->bytesAvailable();
}

bool QNetworkReplyFileImpl::isSequential () const
{
    return true;
}

qint64 QNetworkReplyFileImpl::size() const
{
    bool ok;
    int size = header(QNetworkRequest::ContentLengthHeader).toInt(&ok);
    return ok ? size : 0;
}

/*!
    \internal
*/
qint64 QNetworkReplyFileImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyFileImpl);
    if (!d->isFinished || !d->realFile || !d->realFile->isOpen())
        return -1;
    qint64 ret = d->realFile->read(data, maxlen);
    if (bytesAvailable() == 0)
        d->realFile->close();
    if (ret == 0 && bytesAvailable() == 0)
        return -1;
    else {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QLatin1String("OK"));
        return ret;
    }
}

void QNetworkReplyFileImpl::fileOpenFinished(bool isOpen)
{
    setFinished(true);
    if (isOpen) {
        const auto fileSize = size();
        Q_EMIT metaDataChanged();
        Q_EMIT downloadProgress(fileSize, fileSize);
        Q_EMIT readyRead();
    }
    Q_EMIT finished();
}

QT_END_NAMESPACE

#include "moc_qnetworkreplyfileimpl_p.cpp"

