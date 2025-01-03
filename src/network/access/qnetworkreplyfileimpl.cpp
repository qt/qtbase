// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetworkreplyfileimpl_p.h"

#include "QtCore/qdatetime.h"
#include "qnetworkaccessmanager_p.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include "qnetworkfile_p.h"
#include "qnetworkrequest.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QT_IMPL_METATYPE_EXTERN_TAGGED(QNetworkRequest::KnownHeaders, QNetworkRequest__KnownHeaders)

QNetworkReplyFileImplPrivate::QNetworkReplyFileImplPrivate()
    : QNetworkReplyPrivate(), managerPrivate(nullptr), realFile(nullptr)
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
    if (url.host() == "localhost"_L1)
        url.setHost(QString());

#if !defined(Q_OS_WIN)
    // do not allow UNC paths on Unix
    if (!url.host().isEmpty()) {
        // we handle only local files
        QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Request for opening non-local file %1").arg(url.toString());
        setError(QNetworkReply::ProtocolInvalidOperationError, msg);
        setFinished(true); // We're finished, will emit finished() after ctor is done.
        QMetaObject::invokeMethod(this, "errorOccurred", Qt::QueuedConnection,
            Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ProtocolInvalidOperationError));
        QMetaObject::invokeMethod(this, [this](){ fileOpenFinished(false); }, Qt::QueuedConnection);
        return;
    }
#endif
    if (url.path().isEmpty())
        url.setPath("/"_L1);
    setUrl(url);

    QString fileName = url.toLocalFile();
    if (fileName.isEmpty()) {
        const QString scheme = url.scheme();
        if (scheme == "qrc"_L1) {
            fileName = u':' + url.path();
        } else {
#if defined(Q_OS_ANDROID)
            if (scheme == "assets"_L1)
                fileName = "assets:"_L1 + url.path();
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
            QMetaObject::invokeMethod(this, "errorOccurred", Qt::QueuedConnection,
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
                QMetaObject::invokeMethod(this, "errorOccurred", Qt::QueuedConnection,
                    Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentAccessDenied));
            } else {
                setError(QNetworkReply::ContentNotFoundError, msg);
                QMetaObject::invokeMethod(this, "errorOccurred", Qt::QueuedConnection,
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
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, "OK"_L1);
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

