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

#include "qnetworkreplyimpl_p.h"
#include "qnetworkaccessbackend_p.h"
#include "qnetworkcookie.h"
#include "qnetworkcookiejar.h"
#include "qabstractnetworkcache.h"
#include "QtCore/qcoreapplication.h"
#include "QtCore/qdatetime.h"
#include "QtNetwork/qsslconfiguration.h"
#include "qnetworkaccessmanager_p.h"

#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

inline QNetworkReplyImplPrivate::QNetworkReplyImplPrivate()
    : backend(nullptr), outgoingData(nullptr),
      copyDevice(nullptr),
      cacheEnabled(false), cacheSaveDevice(nullptr),
      notificationHandlingPaused(false),
      bytesDownloaded(0), bytesUploaded(-1),
      httpStatusCode(0),
      state(Idle)
      , downloadBufferReadPosition(0)
      , downloadBufferCurrentSize(0)
      , downloadBufferMaximumSize(0)
      , downloadBuffer(nullptr)
{
    if (request.attribute(QNetworkRequest::EmitAllUploadProgressSignalsAttribute).toBool() == true)
        emitAllUploadProgressSignals = true;
}

void QNetworkReplyImplPrivate::_q_startOperation()
{
    // ensure this function is only being called once
    if (state == Working || state == Finished) {
        qDebug() << "QNetworkReplyImpl::_q_startOperation was called more than once" << url;
        return;
    }
    state = Working;

    // note: if that method is called directly, it cannot happen that the backend is 0,
    // because we just checked via a qobject_cast that we got a http backend (see
    // QNetworkReplyImplPrivate::setup())
    if (!backend) {
        error(QNetworkReplyImpl::ProtocolUnknownError,
              QCoreApplication::translate("QNetworkReply", "Protocol \"%1\" is unknown").arg(url.scheme())); // not really true!;
        finished();
        return;
    }

    if (!backend->start()) {
        qWarning("Backend start failed");
        state = Working;
        error(QNetworkReplyImpl::UnknownNetworkError,
              QCoreApplication::translate("QNetworkReply", "backend start error."));
        finished();
        return;
    }

    // Prepare timer for progress notifications
    downloadProgressSignalChoke.start();
    uploadProgressSignalChoke.invalidate();

    if (backend && backend->isSynchronous()) {
        state = Finished;
        q_func()->setFinished(true);
    } else {
        if (state != Finished) {
            if (operation == QNetworkAccessManager::GetOperation)
                pendingNotifications.push_back(NotifyDownstreamReadyWrite);

            handleNotifications();
        }
    }
}

void QNetworkReplyImplPrivate::_q_copyReadyRead()
{
    Q_Q(QNetworkReplyImpl);
    if (state != Working)
        return;
    if (!copyDevice || !q->isOpen())
        return;

    // FIXME Optimize to use download buffer if it is a QBuffer.
    // Needs to be done where sendCacheContents() (?) of HTTP is emitting
    // metaDataChanged ?
    qint64 lastBytesDownloaded = bytesDownloaded;
    forever {
        qint64 bytesToRead = nextDownstreamBlockSize();
        if (bytesToRead == 0)
            // we'll be called again, eventually
            break;

        bytesToRead = qBound<qint64>(1, bytesToRead, copyDevice->bytesAvailable());
        qint64 bytesActuallyRead = copyDevice->read(buffer.reserve(bytesToRead), bytesToRead);
        if (bytesActuallyRead == -1) {
            buffer.chop(bytesToRead);
            break;
        }
        buffer.chop(bytesToRead - bytesActuallyRead);

        if (!copyDevice->isSequential() && copyDevice->atEnd()) {
            bytesDownloaded += bytesActuallyRead;
            break;
        }

        bytesDownloaded += bytesActuallyRead;
    }

    if (bytesDownloaded == lastBytesDownloaded) {
        // we didn't read anything
        return;
    }

    QVariant totalSize = cookedHeaders.value(QNetworkRequest::ContentLengthHeader);
    pauseNotificationHandling();
    // emit readyRead before downloadProgress incase this will cause events to be
    // processed and we get into a recursive call (as in QProgressDialog).
    emit q->readyRead();
    if (downloadProgressSignalChoke.elapsed() >= progressSignalInterval) {
        downloadProgressSignalChoke.restart();
        emit q->downloadProgress(bytesDownloaded,
                             totalSize.isNull() ? Q_INT64_C(-1) : totalSize.toLongLong());
    }
    resumeNotificationHandling();
}

void QNetworkReplyImplPrivate::_q_copyReadChannelFinished()
{
    _q_copyReadyRead();
}

void QNetworkReplyImplPrivate::_q_bufferOutgoingDataFinished()
{
    Q_Q(QNetworkReplyImpl);

    // make sure this is only called once, ever.
    //_q_bufferOutgoingData may call it or the readChannelFinished emission
    if (state != Buffering)
        return;

    // disconnect signals
    QObject::disconnect(outgoingData, SIGNAL(readyRead()), q, SLOT(_q_bufferOutgoingData()));
    QObject::disconnect(outgoingData, SIGNAL(readChannelFinished()), q, SLOT(_q_bufferOutgoingDataFinished()));

    // finally, start the request
    QMetaObject::invokeMethod(q, "_q_startOperation", Qt::QueuedConnection);
}

void QNetworkReplyImplPrivate::_q_bufferOutgoingData()
{
    Q_Q(QNetworkReplyImpl);

    if (!outgoingDataBuffer) {
        // first call, create our buffer
        outgoingDataBuffer = QSharedPointer<QRingBuffer>::create();

        QObject::connect(outgoingData, SIGNAL(readyRead()), q, SLOT(_q_bufferOutgoingData()));
        QObject::connect(outgoingData, SIGNAL(readChannelFinished()), q, SLOT(_q_bufferOutgoingDataFinished()));
    }

    qint64 bytesBuffered = 0;
    qint64 bytesToBuffer = 0;

    // read data into our buffer
    forever {
        bytesToBuffer = outgoingData->bytesAvailable();
        // unknown? just try 2 kB, this also ensures we always try to read the EOF
        if (bytesToBuffer <= 0)
            bytesToBuffer = 2*1024;

        char *dst = outgoingDataBuffer->reserve(bytesToBuffer);
        bytesBuffered = outgoingData->read(dst, bytesToBuffer);

        if (bytesBuffered == -1) {
            // EOF has been reached.
            outgoingDataBuffer->chop(bytesToBuffer);

            _q_bufferOutgoingDataFinished();
            break;
        } else if (bytesBuffered == 0) {
            // nothing read right now, just wait until we get called again
            outgoingDataBuffer->chop(bytesToBuffer);

            break;
        } else {
            // don't break, try to read() again
            outgoingDataBuffer->chop(bytesToBuffer - bytesBuffered);
        }
    }
}

void QNetworkReplyImplPrivate::setup(QNetworkAccessManager::Operation op, const QNetworkRequest &req,
                                     QIODevice *data)
{
    Q_Q(QNetworkReplyImpl);

    outgoingData = data;
    request = req;
    originalRequest = req;
    url = request.url();
    operation = op;

    q->QIODevice::open(QIODevice::ReadOnly);
    // Internal code that does a HTTP reply for the synchronous Ajax
    // in Qt WebKit.
    QVariant synchronousHttpAttribute = req.attribute(
            static_cast<QNetworkRequest::Attribute>(QNetworkRequest::SynchronousRequestAttribute));
    // The synchronous HTTP is a corner case, we will put all upload data in one big QByteArray in the outgoingDataBuffer.
    // Yes, this is not the most efficient thing to do, but on the other hand synchronous XHR needs to die anyway.
    if (synchronousHttpAttribute.toBool() && outgoingData) {
        outgoingDataBuffer = QSharedPointer<QRingBuffer>::create();
        qint64 previousDataSize = 0;
        do {
            previousDataSize = outgoingDataBuffer->size();
            outgoingDataBuffer->append(outgoingData->readAll());
        } while (outgoingDataBuffer->size() != previousDataSize);
    }

    if (backend)
        backend->setSynchronous(synchronousHttpAttribute.toBool());


    if (outgoingData && backend && !backend->isSynchronous()) {
        // there is data to be uploaded, e.g. HTTP POST.

        if (!backend->needsResetableUploadData() || !outgoingData->isSequential()) {
            // backend does not need upload buffering or
            // fixed size non-sequential
            // just start the operation
            QMetaObject::invokeMethod(q, "_q_startOperation", Qt::QueuedConnection);
        } else {
            bool bufferingDisallowed =
                    req.attribute(QNetworkRequest::DoNotBufferUploadDataAttribute,
                                  false).toBool();

            if (bufferingDisallowed) {
                // if a valid content-length header for the request was supplied, we can disable buffering
                // if not, we will buffer anyway
                if (req.header(QNetworkRequest::ContentLengthHeader).isValid()) {
                    QMetaObject::invokeMethod(q, "_q_startOperation", Qt::QueuedConnection);
                } else {
                    state = Buffering;
                    QMetaObject::invokeMethod(q, "_q_bufferOutgoingData", Qt::QueuedConnection);
                }
            } else {
                // _q_startOperation will be called when the buffering has finished.
                state = Buffering;
                QMetaObject::invokeMethod(q, "_q_bufferOutgoingData", Qt::QueuedConnection);
            }
        }
    } else {
        // for HTTP, we want to send out the request as fast as possible to the network, without
        // invoking methods in a QueuedConnection
        if (backend && backend->isSynchronous())
            _q_startOperation();
        else
            QMetaObject::invokeMethod(q, "_q_startOperation", Qt::QueuedConnection);
    }
}

void QNetworkReplyImplPrivate::backendNotify(InternalNotifications notification)
{
    Q_Q(QNetworkReplyImpl);
    const auto it = std::find(pendingNotifications.cbegin(), pendingNotifications.cend(), notification);
    if (it == pendingNotifications.cend())
        pendingNotifications.push_back(notification);

    if (pendingNotifications.size() == 1)
        QCoreApplication::postEvent(q, new QEvent(QEvent::NetworkReplyUpdated));
}

void QNetworkReplyImplPrivate::handleNotifications()
{
    if (notificationHandlingPaused)
        return;

     for (InternalNotifications notification : qExchange(pendingNotifications, {})) {
        if (state != Working)
            return;
        switch (notification) {
        case NotifyDownstreamReadyWrite:
            if (copyDevice) {
                _q_copyReadyRead();
            } else if (backend) {
                if (backend->bytesAvailable() > 0)
                    readFromBackend();
                else if (backend->wantToRead())
                    readFromBackend();
            }
            break;
        }
    }
}

// Do not handle the notifications while we are emitting downloadProgress
// or readyRead
void QNetworkReplyImplPrivate::pauseNotificationHandling()
{
    notificationHandlingPaused = true;
}

// Resume notification handling
void QNetworkReplyImplPrivate::resumeNotificationHandling()
{
    Q_Q(QNetworkReplyImpl);
    notificationHandlingPaused = false;
    if (pendingNotifications.size() >= 1)
        QCoreApplication::postEvent(q, new QEvent(QEvent::NetworkReplyUpdated));
}

QAbstractNetworkCache *QNetworkReplyImplPrivate::networkCache() const
{
    if (!backend)
        return nullptr;
    return backend->networkCache();
}

void QNetworkReplyImplPrivate::createCache()
{
    // check if we can save and if we're allowed to
    if (!networkCache()
        || !request.attribute(QNetworkRequest::CacheSaveControlAttribute, true).toBool())
        return;
    cacheEnabled = true;
}

bool QNetworkReplyImplPrivate::isCachingEnabled() const
{
    return (cacheEnabled && networkCache() != nullptr);
}

void QNetworkReplyImplPrivate::setCachingEnabled(bool enable)
{
    if (!enable && !cacheEnabled)
        return;                 // nothing to do
    if (enable && cacheEnabled)
        return;                 // nothing to do either!

    if (enable) {
        if (Q_UNLIKELY(bytesDownloaded)) {
            // refuse to enable in this case
            qCritical("QNetworkReplyImpl: backend error: caching was enabled after some bytes had been written");
            return;
        }

        createCache();
    } else {
        // someone told us to turn on, then back off?
        // ok... but you should make up your mind
        qDebug("QNetworkReplyImpl: setCachingEnabled(true) called after setCachingEnabled(false) -- "
               "backend %s probably needs to be fixed",
               backend->metaObject()->className());
        networkCache()->remove(url);
        cacheSaveDevice = nullptr;
        cacheEnabled = false;
    }
}

void QNetworkReplyImplPrivate::completeCacheSave()
{
    if (cacheEnabled && errorCode != QNetworkReplyImpl::NoError) {
        networkCache()->remove(url);
    } else if (cacheEnabled && cacheSaveDevice) {
        networkCache()->insert(cacheSaveDevice);
    }
    cacheSaveDevice = nullptr;
    cacheEnabled = false;
}

void QNetworkReplyImplPrivate::emitUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    Q_Q(QNetworkReplyImpl);
    bytesUploaded = bytesSent;

    if (!emitAllUploadProgressSignals) {
        //choke signal emissions, except the first and last signals which are unconditional
        if (uploadProgressSignalChoke.isValid()) {
            if (bytesSent != bytesTotal && uploadProgressSignalChoke.elapsed() < progressSignalInterval) {
                return;
            }
            uploadProgressSignalChoke.restart();
        } else {
            uploadProgressSignalChoke.start();
        }
    }

    pauseNotificationHandling();
    emit q->uploadProgress(bytesSent, bytesTotal);
    resumeNotificationHandling();
}


qint64 QNetworkReplyImplPrivate::nextDownstreamBlockSize() const
{
    enum { DesiredBufferSize = 32 * 1024 };
    if (readBufferMaxSize == 0)
        return DesiredBufferSize;

    return qMax<qint64>(0, readBufferMaxSize - buffer.size());
}

void QNetworkReplyImplPrivate::initCacheSaveDevice()
{
    Q_Q(QNetworkReplyImpl);

    // The disk cache does not support partial content, so don't even try to
    // save any such content into the cache.
    if (q->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 206) {
        cacheEnabled = false;
        return;
    }

    // save the meta data
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);
    // @todo @future: fetchCacheMetaData is not currently implemented in any backend, but can be useful again in the future
    // metaData = backend->fetchCacheMetaData(metaData);

    // save the redirect request also in the cache
    QVariant redirectionTarget = q->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectionTarget.isValid()) {
        QNetworkCacheMetaData::AttributesMap attributes = metaData.attributes();
        attributes.insert(QNetworkRequest::RedirectionTargetAttribute, redirectionTarget);
        metaData.setAttributes(attributes);
    }

    cacheSaveDevice = networkCache()->prepare(metaData);

    if (!cacheSaveDevice || (cacheSaveDevice && !cacheSaveDevice->isOpen())) {
        if (Q_UNLIKELY(cacheSaveDevice && !cacheSaveDevice->isOpen()))
            qCritical("QNetworkReplyImpl: network cache returned a device that is not open -- "
                  "class %s probably needs to be fixed",
                  networkCache()->metaObject()->className());

        networkCache()->remove(url);
        cacheSaveDevice = nullptr;
        cacheEnabled = false;
    }
}

// we received downstream data and send this to the cache
// and to our buffer (which in turn gets read by the user of QNetworkReply)
void QNetworkReplyImplPrivate::appendDownstreamData(QByteDataBuffer &data)
{
    Q_Q(QNetworkReplyImpl);
    if (!q->isOpen())
        return;

    if (cacheEnabled && !cacheSaveDevice) {
        initCacheSaveDevice();
    }

    qint64 bytesWritten = 0;
    for (int i = 0; i < data.bufferCount(); i++) {
        QByteArray const &item = data[i];

        if (cacheSaveDevice)
            cacheSaveDevice->write(item.constData(), item.size());
        buffer.append(item);

        bytesWritten += item.size();
    }
    data.clear();

    bytesDownloaded += bytesWritten;

    appendDownstreamDataSignalEmissions();
}

void QNetworkReplyImplPrivate::appendDownstreamDataSignalEmissions()
{
    Q_Q(QNetworkReplyImpl);

    QVariant totalSize = cookedHeaders.value(QNetworkRequest::ContentLengthHeader);
    pauseNotificationHandling();
    // important: At the point of this readyRead(), the data parameter list must be empty,
    // else implicit sharing will trigger memcpy when the user is reading data!
    emit q->readyRead();
    // emit readyRead before downloadProgress incase this will cause events to be
    // processed and we get into a recursive call (as in QProgressDialog).
    if (downloadProgressSignalChoke.elapsed() >= progressSignalInterval) {
        downloadProgressSignalChoke.restart();
        emit q->downloadProgress(bytesDownloaded,
                             totalSize.isNull() ? Q_INT64_C(-1) : totalSize.toLongLong());
    }

    resumeNotificationHandling();
    // do we still have room in the buffer?
    if (nextDownstreamBlockSize() > 0)
        backendNotify(QNetworkReplyImplPrivate::NotifyDownstreamReadyWrite);
}

// this is used when it was fetched from the cache, right?
void QNetworkReplyImplPrivate::appendDownstreamData(QIODevice *data)
{
    Q_Q(QNetworkReplyImpl);
    if (!q->isOpen())
        return;

    // read until EOF from data
    if (Q_UNLIKELY(copyDevice)) {
        qCritical("QNetworkReplyImpl: copy from QIODevice already in progress -- "
                  "backend probably needs to be fixed");
        return;
    }

    copyDevice = data;
    q->connect(copyDevice, SIGNAL(readyRead()), SLOT(_q_copyReadyRead()));
    q->connect(copyDevice, SIGNAL(readChannelFinished()), SLOT(_q_copyReadChannelFinished()));

    // start the copy:
    _q_copyReadyRead();
}

static void downloadBufferDeleter(char *ptr)
{
    delete[] ptr;
}

char* QNetworkReplyImplPrivate::getDownloadBuffer(qint64 size)
{
    Q_Q(QNetworkReplyImpl);

    if (!downloadBuffer) {
        // We are requested to create it
        // Check attribute() if allocating a buffer of that size can be allowed
        QVariant bufferAllocationPolicy = request.attribute(QNetworkRequest::MaximumDownloadBufferSizeAttribute);
        if (bufferAllocationPolicy.isValid() && bufferAllocationPolicy.toLongLong() >= size) {
            downloadBufferCurrentSize = 0;
            downloadBufferMaximumSize = size;
            downloadBuffer = new char[downloadBufferMaximumSize]; // throws if allocation fails
            downloadBufferPointer = QSharedPointer<char>(downloadBuffer, downloadBufferDeleter);

            q->setAttribute(QNetworkRequest::DownloadBufferAttribute, QVariant::fromValue<QSharedPointer<char> > (downloadBufferPointer));
        }
    }

    return downloadBuffer;
}

void QNetworkReplyImplPrivate::setDownloadBuffer(QSharedPointer<char> sp, qint64 size)
{
    Q_Q(QNetworkReplyImpl);

    downloadBufferPointer = sp;
    downloadBuffer = downloadBufferPointer.data();
    downloadBufferCurrentSize = 0;
    downloadBufferMaximumSize = size;
    q->setAttribute(QNetworkRequest::DownloadBufferAttribute, QVariant::fromValue<QSharedPointer<char> > (downloadBufferPointer));
}


void QNetworkReplyImplPrivate::appendDownstreamDataDownloadBuffer(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_Q(QNetworkReplyImpl);
    if (!q->isOpen())
        return;

    if (cacheEnabled && !cacheSaveDevice)
        initCacheSaveDevice();

    if (cacheSaveDevice && bytesReceived == bytesTotal) {
        // Write everything in one go if we use a download buffer. might be more performant.
        cacheSaveDevice->write(downloadBuffer, bytesTotal);
    }

    bytesDownloaded = bytesReceived;

    downloadBufferCurrentSize = bytesReceived;

    // Only emit readyRead when actual data is there
    // emit readyRead before downloadProgress incase this will cause events to be
    // processed and we get into a recursive call (as in QProgressDialog).
    if (bytesDownloaded > 0)
        emit q->readyRead();
    if (downloadProgressSignalChoke.elapsed() >= progressSignalInterval) {
        downloadProgressSignalChoke.restart();
        emit q->downloadProgress(bytesDownloaded, bytesTotal);
    }
}

void QNetworkReplyImplPrivate::finished()
{
    Q_Q(QNetworkReplyImpl);

    if (state == Finished || state == Aborted)
        return;

    pauseNotificationHandling();
    QVariant totalSize = cookedHeaders.value(QNetworkRequest::ContentLengthHeader);

    resumeNotificationHandling();

    state = Finished;
    q->setFinished(true);

    pendingNotifications.clear();

    pauseNotificationHandling();
    if (totalSize.isNull() || totalSize == -1) {
        emit q->downloadProgress(bytesDownloaded, bytesDownloaded);
    } else {
        emit q->downloadProgress(bytesDownloaded, totalSize.toLongLong());
    }

    if (bytesUploaded == -1 && (outgoingData || outgoingDataBuffer))
        emit q->uploadProgress(0, 0);
    resumeNotificationHandling();

    // if we don't know the total size of or we received everything save the cache
    if (totalSize.isNull() || totalSize == -1 || bytesDownloaded == totalSize)
        completeCacheSave();

    // note: might not be a good idea, since users could decide to delete us
    // which would delete the backend too...
    // maybe we should protect the backend
    pauseNotificationHandling();
    emit q->readChannelFinished();
    emit q->finished();
    resumeNotificationHandling();
}

void QNetworkReplyImplPrivate::error(QNetworkReplyImpl::NetworkError code, const QString &errorMessage)
{
    Q_Q(QNetworkReplyImpl);
    // Can't set and emit multiple errors.
    if (errorCode != QNetworkReply::NoError) {
        qWarning( "QNetworkReplyImplPrivate::error: Internal problem, this method must only be called once.");
        return;
    }

    errorCode = code;
    q->setErrorString(errorMessage);

    // note: might not be a good idea, since users could decide to delete us
    // which would delete the backend too...
    // maybe we should protect the backend
    emit q->errorOccurred(code);
}

void QNetworkReplyImplPrivate::metaDataChanged()
{
    Q_Q(QNetworkReplyImpl);
    // 1. do we have cookies?
    // 2. are we allowed to set them?
    if (!manager.isNull()) {
        const auto it = cookedHeaders.constFind(QNetworkRequest::SetCookieHeader);
        if (it != cookedHeaders.cend()
            && request.attribute(QNetworkRequest::CookieSaveControlAttribute,
                                 QNetworkRequest::Automatic).toInt() == QNetworkRequest::Automatic) {
            QNetworkCookieJar *jar = manager->cookieJar();
            if (jar) {
                QList<QNetworkCookie> cookies =
                    qvariant_cast<QList<QNetworkCookie> >(it.value());
                jar->setCookiesFromUrl(cookies, url);
            }
        }
    }

    emit q->metaDataChanged();
}

void QNetworkReplyImplPrivate::redirectionRequested(const QUrl &target)
{
    attributes.insert(QNetworkRequest::RedirectionTargetAttribute, target);
}

void QNetworkReplyImplPrivate::encrypted()
{
#ifndef QT_NO_SSL
    Q_Q(QNetworkReplyImpl);
    emit q->encrypted();
#endif
}

void QNetworkReplyImplPrivate::sslErrors(const QList<QSslError> &errors)
{
#ifndef QT_NO_SSL
    Q_Q(QNetworkReplyImpl);
    emit q->sslErrors(errors);
#else
    Q_UNUSED(errors);
#endif
}

void QNetworkReplyImplPrivate::readFromBackend()
{
    Q_Q(QNetworkReplyImpl);
    if (!backend)
        return;

    if (backend->ioFeatures() & QNetworkAccessBackend::IOFeature::ZeroCopy) {
        if (backend->bytesAvailable())
            emit q->readyRead();
    } else {
        while (backend->bytesAvailable()
               && (!readBufferMaxSize || buffer.size() < readBufferMaxSize)) {
            qint64 toRead = qMin(nextDownstreamBlockSize(), backend->bytesAvailable());
            if (toRead == 0)
                toRead = 16 * 1024; // try to read something
            char *data = buffer.reserve(toRead);
            qint64 bytesRead = backend->read(data, toRead);
            Q_ASSERT(bytesRead <= toRead);
            buffer.chop(toRead - bytesRead);
            emit q->readyRead();
        }
    }
}

QNetworkReplyImpl::QNetworkReplyImpl(QObject *parent)
    : QNetworkReply(*new QNetworkReplyImplPrivate, parent)
{
}

QNetworkReplyImpl::~QNetworkReplyImpl()
{
    Q_D(QNetworkReplyImpl);

    // This code removes the data from the cache if it was prematurely aborted.
    // See QNetworkReplyImplPrivate::completeCacheSave(), we disable caching there after the cache
    // save had been properly finished. So if it is still enabled it means we got deleted/aborted.
    if (d->isCachingEnabled())
        d->networkCache()->remove(url());
}

void QNetworkReplyImpl::abort()
{
    Q_D(QNetworkReplyImpl);
    if (d->state == QNetworkReplyPrivate::Finished || d->state == QNetworkReplyPrivate::Aborted)
        return;

    // stop both upload and download
    if (d->outgoingData)
        disconnect(d->outgoingData, nullptr, this, nullptr);
    if (d->copyDevice)
        disconnect(d->copyDevice, nullptr, this, nullptr);

    QNetworkReply::close();

    // call finished which will emit signals
    d->error(OperationCanceledError, tr("Operation canceled"));
    d->finished();
    d->state = QNetworkReplyPrivate::Aborted;

    // finished may access the backend
    if (d->backend) {
        d->backend->deleteLater();
        d->backend = nullptr;
    }
}

void QNetworkReplyImpl::close()
{
    Q_D(QNetworkReplyImpl);
    if (d->state == QNetworkReplyPrivate::Aborted ||
        d->state == QNetworkReplyPrivate::Finished)
        return;

    // stop the download
    if (d->backend)
        d->backend->close();
    if (d->copyDevice)
        disconnect(d->copyDevice, nullptr, this, nullptr);

    QNetworkReply::close();

    // call finished which will emit signals
    d->error(OperationCanceledError, tr("Operation canceled"));
    d->finished();
}

/*!
    Returns the number of bytes available for reading with
    QIODevice::read(). The number of bytes available may grow until
    the finished() signal is emitted.
*/
qint64 QNetworkReplyImpl::bytesAvailable() const
{
    // Special case for the "zero copy" download buffer
    Q_D(const QNetworkReplyImpl);
    if (d->downloadBuffer) {
        qint64 maxAvail = d->downloadBufferCurrentSize - d->downloadBufferReadPosition;
        return QNetworkReply::bytesAvailable() + maxAvail;
    }
    return QNetworkReply::bytesAvailable() + (d->backend ? d->backend->bytesAvailable() : 0);
}

void QNetworkReplyImpl::setReadBufferSize(qint64 size)
{
    Q_D(QNetworkReplyImpl);
    qint64 oldMaxSize = d->readBufferMaxSize;
    QNetworkReply::setReadBufferSize(size);
    if (size > oldMaxSize && size > d->buffer.size())
        d->readFromBackend();
}

#ifndef QT_NO_SSL
void QNetworkReplyImpl::sslConfigurationImplementation(QSslConfiguration &configuration) const
{
    Q_D(const QNetworkReplyImpl);
    if (d->backend)
        configuration = d->backend->sslConfiguration();
}

void QNetworkReplyImpl::setSslConfigurationImplementation(const QSslConfiguration &config)
{
    Q_D(QNetworkReplyImpl);
    if (d->backend && !config.isNull())
        d->backend->setSslConfiguration(config);
}

void QNetworkReplyImpl::ignoreSslErrors()
{
    Q_D(QNetworkReplyImpl);
    if (d->backend)
        d->backend->ignoreSslErrors();
}

void QNetworkReplyImpl::ignoreSslErrorsImplementation(const QList<QSslError> &errors)
{
    Q_D(QNetworkReplyImpl);
    if (d->backend)
        d->backend->ignoreSslErrors(errors);
}
#endif  // QT_NO_SSL

/*!
    \internal
*/
qint64 QNetworkReplyImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyImpl);

    if (d->backend
        && d->backend->ioFeatures().testFlag(QNetworkAccessBackend::IOFeature::ZeroCopy)) {
        qint64 bytesRead = 0;
        while (d->backend->bytesAvailable()) {
            QByteArrayView view = d->backend->readPointer();
            if (view.size()) {
                qint64 bytesToCopy = qMin(qint64(view.size()), maxlen - bytesRead);
                memcpy(data + bytesRead, view.data(), bytesToCopy); // from zero to one copy

                // We might have to cache this
                if (d->cacheEnabled && !d->cacheSaveDevice)
                    d->initCacheSaveDevice();
                if (d->cacheEnabled && d->cacheSaveDevice)
                    d->cacheSaveDevice->write(view.data(), view.size());

                bytesRead += bytesToCopy;
                d->backend->advanceReadPointer(bytesToCopy);
            } else {
                break;
            }
        }
        QVariant totalSize = d->cookedHeaders.value(QNetworkRequest::ContentLengthHeader);
        emit downloadProgress(bytesRead,
                              totalSize.isNull() ? Q_INT64_C(-1) : totalSize.toLongLong());
        return bytesRead;
    } else if (d->backend && d->backend->bytesAvailable()) {
        return d->backend->read(data, maxlen);
    }

    // Special case code if we have the "zero copy" download buffer
    if (d->downloadBuffer) {
        qint64 maxAvail = qMin<qint64>(d->downloadBufferCurrentSize - d->downloadBufferReadPosition, maxlen);
        if (maxAvail == 0)
            return d->state == QNetworkReplyPrivate::Finished ? -1 : 0;
        // FIXME what about "Aborted" state?
        memcpy(data, d->downloadBuffer + d->downloadBufferReadPosition, maxAvail);
        d->downloadBufferReadPosition += maxAvail;
        return maxAvail;
    }


    // FIXME what about "Aborted" state?
    if (d->state == QNetworkReplyPrivate::Finished)
        return -1;

    d->backendNotify(QNetworkReplyImplPrivate::NotifyDownstreamReadyWrite);
    return 0;
}

/*!
   \internal Reimplemented for internal purposes
*/
bool QNetworkReplyImpl::event(QEvent *e)
{
    if (e->type() == QEvent::NetworkReplyUpdated) {
        d_func()->handleNotifications();
        return true;
    }

    return QObject::event(e);
}

QT_END_NAMESPACE

#include "moc_qnetworkreplyimpl_p.cpp"

