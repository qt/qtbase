// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qrandomaccessasyncfile_p_p.h"

#include "qiooperation_p.h"
#include "qiooperation_p_p.h"

#include <QtCore/qfile.h> // QtPrivate::toFilesystemPath
#include <QtCore/qtypes.h>
#include <QtCore/private/qioring_p.h>

#include <QtCore/q26numeric.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcQRandomAccessIORing, "qt.core.qrandomaccessasyncfile.ioring",
                          QtCriticalMsg);

QRandomAccessAsyncFileNativeBackend::QRandomAccessAsyncFileNativeBackend(QRandomAccessAsyncFile *owner)
    : QRandomAccessAsyncFileBackend(owner)
{}

QRandomAccessAsyncFileNativeBackend::~QRandomAccessAsyncFileNativeBackend() = default;

bool QRandomAccessAsyncFileNativeBackend::init()
{
    m_ioring = QIORing::sharedInstance();
    if (!m_ioring)
        qCWarning(lcQRandomAccessIORing, "QRandomAccessAsyncFile: ioring failed to initialize");
    return m_ioring != nullptr;
}

QIORing::RequestHandle QRandomAccessAsyncFileNativeBackend::cancel(QIORing::RequestHandle handle)
{
    if (handle) {
        QIORingRequest<QIORing::Operation::Cancel> cancelRequest;
        cancelRequest.handle = handle;
        return m_ioring->queueRequest(std::move(cancelRequest));
    }
    return nullptr;
}

void QRandomAccessAsyncFileNativeBackend::cancelAndWait(QIOOperation *op)
{
    auto *opHandle = m_opHandleMap.value(op);
    if (auto *handle = cancel(opHandle)) {
        m_ioring->waitForRequest(handle);
        m_ioring->waitForRequest(opHandle);
    }
}

void QRandomAccessAsyncFileNativeBackend::queueCompletion(QIOOperationPrivate *priv, QIOOperation::Error error)
{
    // Remove the handle now in case the user cancels or deletes the io-operation
    // before operationComplete is called - the null-handle will protect from
    // nasty issues that may occur when trying to cancel an operation that's no
    // longer in the queue:
    m_opHandleMap.remove(priv->q_func());
    // @todo: Look into making it emit only if synchronously completed
    QMetaObject::invokeMethod(priv->q_ptr, [priv, error](){
        priv->operationComplete(error);
    }, Qt::QueuedConnection);
}

QIOOperation *QRandomAccessAsyncFileNativeBackend::open(const QString &path, QIODeviceBase::OpenMode mode)
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage();

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->type = QIOOperation::Type::Open;

    auto *op = new QIOOperation(*priv, m_owner);
    if (m_fileState != FileState::Closed) {
        queueCompletion(priv, QIOOperation::Error::Open);
        return op;
    }
    m_operations.append(op);
    m_fileState = FileState::OpenPending;

    QIORingRequest<QIORing::Operation::Open> openOperation;
    openOperation.path = QtPrivate::toFilesystemPath(path);
    openOperation.flags = mode;
    openOperation.setCallback([this, op,
                               priv](const QIORingRequest<QIORing::Operation::Open> &request) {
        if (const auto *err = std::get_if<QFileDevice::FileError>(&request.result)) {
            if (m_fileState != FileState::Opened) {
                // We assume there was only one pending open() in flight.
                m_fd = -1;
                m_fileState = FileState::Closed;
            }
            if (priv->error == QIOOperation::Error::Aborted || *err == QFileDevice::AbortError)
                queueCompletion(priv, QIOOperation::Error::Aborted);
            else
                queueCompletion(priv, QIOOperation::Error::Open);
        } else if (const auto *result = std::get_if<QIORingResult<QIORing::Operation::Open>>(
                           &request.result)) {
            if (m_fileState == FileState::OpenPending) {
                m_fileState = FileState::Opened;
                m_fd = result->fd;
                queueCompletion(priv, QIOOperation::Error::None);
            } else { // Something went wrong, we did not expect a callback:
                // So we close the new handle:
                QIORingRequest<QIORing::Operation::Close> closeRequest;
                closeRequest.fd = result->fd;
                QIORing::RequestHandle handle = m_ioring->queueRequest(std::move(closeRequest));
                // Since the user issued multiple open() calls they get to wait for the close() to
                // finish:
                m_ioring->waitForRequest(handle);
                queueCompletion(priv, QIOOperation::Error::Open);
            }
        }
        m_operations.removeOne(op);
    });
    m_opHandleMap.insert(priv->q_func(), m_ioring->queueRequest(std::move(openOperation)));

    return op;
}

void QRandomAccessAsyncFileNativeBackend::close()
{
    // all the operations should be aborted
    const auto ops = std::exchange(m_operations, {});
    QList<QIORing::RequestHandle> tasksToAwait;
    // Request to cancel all of the in-flight operations:
    for (const auto &op : ops) {
        if (op) {
            QIOOperationPrivate::get(op)->error = QIOOperation::Error::Aborted;
            if (auto *opHandle = m_opHandleMap.value(op)) {
                tasksToAwait.append(cancel(opHandle));
                tasksToAwait.append(opHandle);
            }
        }
    }

    QIORingRequest<QIORing::Operation::Close> closeRequest;
    closeRequest.fd = m_fd;
    tasksToAwait.append(m_ioring->queueRequest(std::move(closeRequest)));

    // Wait for completion:
    for (const QIORing::RequestHandle &handle : tasksToAwait)
        m_ioring->waitForRequest(handle);
    m_fileState = FileState::Closed;
    m_fd = -1;
}

qint64 QRandomAccessAsyncFileNativeBackend::size() const
{
    QIORingRequest<QIORing::Operation::Stat> statRequest;
    statRequest.fd = m_fd;
    qint64 finalSize = 0;
    statRequest.setCallback([&finalSize](const QIORingRequest<QIORing::Operation::Stat> &request) {
        if (const auto *err = std::get_if<QFileDevice::FileError>(&request.result)) {
            Q_UNUSED(err);
            finalSize = -1;
        } else if (const auto *res = std::get_if<QIORingResult<QIORing::Operation::Stat>>(&request.result)) {
            finalSize = q26::saturate_cast<qint64>(res->size);
        }
    });
    auto *handle = m_ioring->queueRequest(std::move(statRequest));
    m_ioring->waitForRequest(handle);

    return finalSize;
}

QIOOperation *QRandomAccessAsyncFileNativeBackend::flush()
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage();

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->type = QIOOperation::Type::Flush;

    auto *op = new QIOOperation(*priv, m_owner);
    m_operations.append(op);

    QIORingRequest<QIORing::Operation::Flush> flushRequest;
    flushRequest.fd = m_fd;
    flushRequest.setCallback([this, op](const QIORingRequest<QIORing::Operation::Flush> &request) {
        auto *priv = QIOOperationPrivate::get(op);
        if (const auto *err = std::get_if<QFileDevice::FileError>(&request.result)) {
            if (priv->error == QIOOperation::Error::Aborted || *err == QFileDevice::AbortError)
                queueCompletion(priv, QIOOperation::Error::Aborted);
            else if (*err == QFileDevice::OpenError)
                queueCompletion(priv, QIOOperation::Error::FileNotOpen);
            else
                queueCompletion(priv, QIOOperation::Error::Flush);
        } else if (std::get_if<QIORingResult<QIORing::Operation::Flush>>(&request.result)) {
            queueCompletion(priv, QIOOperation::Error::None);
        }
        m_operations.removeOne(op);
    });
    m_opHandleMap.insert(priv->q_func(), m_ioring->queueRequest(std::move(flushRequest)));

    return op;
}

void QRandomAccessAsyncFileNativeBackend::startReadIntoSingle(QIOOperation *op,
                                                        const QSpan<std::byte> &to)
{
    QIORingRequest<QIORing::Operation::Read> readRequest;
    readRequest.fd = m_fd;
    auto *priv = QIOOperationPrivate::get(op);
    if (priv->offset < 0) { // The QIORing offset is unsigned, so error out now
        queueCompletion(priv, QIOOperation::Error::IncorrectOffset);
        m_operations.removeOne(op);
        return;
    }
    readRequest.offset = priv->offset;
    readRequest.destination = to;
    readRequest.setCallback([this, op](const QIORingRequest<QIORing::Operation::Read> &request) {
        auto *priv = QIOOperationPrivate::get(op);
        if (const auto *err = std::get_if<QFileDevice::FileError>(&request.result)) {
            if (priv->error == QIOOperation::Error::Aborted || *err == QFileDevice::AbortError)
                queueCompletion(priv, QIOOperation::Error::Aborted);
            else if (*err == QFileDevice::OpenError)
                queueCompletion(priv, QIOOperation::Error::FileNotOpen);
            else if (*err == QFileDevice::PositionError)
                queueCompletion(priv, QIOOperation::Error::IncorrectOffset);
            else
                queueCompletion(priv, QIOOperation::Error::Read);
        } else if (const auto *result = std::get_if<QIORingResult<QIORing::Operation::Read>>(
                           &request.result)) {
            priv->appendBytesProcessed(result->bytesRead);
            if (priv->dataStorage->containsReadSpans())
                priv->dataStorage->getReadSpans().first().slice(0, result->bytesRead);
            else
                priv->dataStorage->getByteArray().slice(0, result->bytesRead);

            queueCompletion(priv, QIOOperation::Error::None);
        }
        m_operations.removeOne(op);
    });
    m_opHandleMap.insert(priv->q_func(), m_ioring->queueRequest(std::move(readRequest)));
}

QIOReadOperation *QRandomAccessAsyncFileNativeBackend::read(qint64 offset, qint64 maxSize)
{
    QByteArray array;
    array.resizeForOverwrite(maxSize);
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(std::move(array));

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Read;

    auto *op = new QIOReadOperation(*priv, m_owner);
    m_operations.append(op);

    startReadIntoSingle(op, as_writable_bytes(QSpan(dataStorage->getByteArray())));

    return op;
}

QIOWriteOperation *QRandomAccessAsyncFileNativeBackend::write(qint64 offset, const QByteArray &data)
{
    return write(offset, QByteArray(data));
}

void QRandomAccessAsyncFileNativeBackend::startWriteFromSingle(QIOOperation *op,
                                                         const QSpan<const std::byte> &from)
{
    QIORingRequest<QIORing::Operation::Write> writeRequest;
    writeRequest.fd = m_fd;
    auto *priv = QIOOperationPrivate::get(op);
    if (priv->offset < 0) { // The QIORing offset is unsigned, so error out now
        queueCompletion(priv, QIOOperation::Error::IncorrectOffset);
        m_operations.removeOne(op);
        return;
    }
    writeRequest.offset = priv->offset;
    writeRequest.source = from;
    writeRequest.setCallback([this, op](const QIORingRequest<QIORing::Operation::Write> &request) {
        auto *priv = QIOOperationPrivate::get(op);
        if (const auto *err = std::get_if<QFileDevice::FileError>(&request.result)) {
            if (priv->error == QIOOperation::Error::Aborted || *err == QFileDevice::AbortError)
                queueCompletion(priv, QIOOperation::Error::Aborted);
            else if (*err == QFileDevice::OpenError)
                queueCompletion(priv, QIOOperation::Error::FileNotOpen);
            else if (*err == QFileDevice::PositionError)
                queueCompletion(priv, QIOOperation::Error::IncorrectOffset);
            else
                queueCompletion(priv, QIOOperation::Error::Write);
        } else if (const auto *result = std::get_if<QIORingResult<QIORing::Operation::Write>>(
                           &request.result)) {
            priv->appendBytesProcessed(result->bytesWritten);
            queueCompletion(priv, QIOOperation::Error::None);
        }
        m_operations.removeOne(op);
    });
    m_opHandleMap.insert(priv->q_func(), m_ioring->queueRequest(std::move(writeRequest)));
}

QIOWriteOperation *QRandomAccessAsyncFileNativeBackend::write(qint64 offset, QByteArray &&data)
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(std::move(data));

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Write;

    auto *op = new QIOWriteOperation(*priv, m_owner);
    m_operations.append(op);

    startWriteFromSingle(op, as_bytes(QSpan(dataStorage->getByteArray())));

    return op;
}

QIOVectoredReadOperation *QRandomAccessAsyncFileNativeBackend::readInto(qint64 offset,
                                                                  QSpan<std::byte> buffer)
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(
            QSpan<const QSpan<std::byte>>{ buffer });

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Read;

    auto *op = new QIOVectoredReadOperation(*priv, m_owner);
    m_operations.append(op);

    startReadIntoSingle(op, dataStorage->getReadSpans().first());

    return op;
}

QIOVectoredWriteOperation *QRandomAccessAsyncFileNativeBackend::writeFrom(qint64 offset,
                                                                    QSpan<const std::byte> buffer)
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(
            QSpan<const QSpan<const std::byte>>{ buffer });

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Write;

    auto *op = new QIOVectoredWriteOperation(*priv, m_owner);
    m_operations.append(op);

    startWriteFromSingle(op, dataStorage->getWriteSpans().first());

    return op;
}

QIOVectoredReadOperation *
QRandomAccessAsyncFileNativeBackend::readInto(qint64 offset, QSpan<const QSpan<std::byte>> buffers)
{
    if (!QIORing::supportsOperation(QtPrivate::Operation::VectoredRead))
        return nullptr;
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(buffers);

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Read;

    auto *op = new QIOVectoredReadOperation(*priv, m_owner);
    if (priv->offset < 0) { // The QIORing offset is unsigned, so error out now
        queueCompletion(priv, QIOOperation::Error::IncorrectOffset);
        return op;
    }
    m_operations.append(op);

    QIORingRequest<QIORing::Operation::VectoredRead> readRequest;
    readRequest.fd = m_fd;
    readRequest.offset = priv->offset;
    readRequest.destinations = dataStorage->getReadSpans();
    readRequest.setCallback([this,
                             op](const QIORingRequest<QIORing::Operation::VectoredRead> &request) {
        auto *priv = QIOOperationPrivate::get(op);
        if (const auto *err = std::get_if<QFileDevice::FileError>(&request.result)) {
            if (priv->error == QIOOperation::Error::Aborted || *err == QFileDevice::AbortError)
                queueCompletion(priv, QIOOperation::Error::Aborted);
            else
                queueCompletion(priv, QIOOperation::Error::Read);
        } else if (const auto
                           *result = std::get_if<QIORingResult<QIORing::Operation::VectoredRead>>(
                                   &request.result)) {
            priv->appendBytesProcessed(result->bytesRead);
            qint64 processed = result->bytesRead;
            for (auto &span : priv->dataStorage->getReadSpans()) {
                if (span.size() < processed) {
                    processed -= span.size();
                } else { // span.size >= processed
                    span.slice(0, processed);
                    processed = 0;
                }
            }
            queueCompletion(priv, QIOOperation::Error::None);
        }
        m_operations.removeOne(op);
    });
    m_opHandleMap.insert(priv->q_func(), m_ioring->queueRequest(std::move(readRequest)));

    return op;
}

QIOVectoredWriteOperation *
QRandomAccessAsyncFileNativeBackend::writeFrom(qint64 offset, QSpan<const QSpan<const std::byte>> buffers)
{
    if (!QIORing::supportsOperation(QtPrivate::Operation::VectoredWrite))
        return nullptr;
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(buffers);

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Write;

    auto *op = new QIOVectoredWriteOperation(*priv, m_owner);
    if (priv->offset < 0) { // The QIORing offset is unsigned, so error out now
        queueCompletion(priv, QIOOperation::Error::IncorrectOffset);
        return op;
    }
    m_operations.append(op);

    QIORingRequest<QIORing::Operation::VectoredWrite> writeRequest;
    writeRequest.fd = m_fd;
    writeRequest.offset = priv->offset;
    writeRequest.sources = dataStorage->getWriteSpans();
    writeRequest.setCallback(
            [this, op](const QIORingRequest<QIORing::Operation::VectoredWrite> &request) {
                auto *priv = QIOOperationPrivate::get(op);
                if (const auto *err = std::get_if<QFileDevice::FileError>(&request.result)) {
                    if (priv->error == QIOOperation::Error::Aborted || *err == QFileDevice::AbortError)
                        queueCompletion(priv, QIOOperation::Error::Aborted);
                    else
                        queueCompletion(priv, QIOOperation::Error::Write);
                } else if (const auto *result = std::get_if<
                                   QIORingResult<QIORing::Operation::VectoredWrite>>(
                                   &request.result)) {
                    priv->appendBytesProcessed(result->bytesWritten);
                    queueCompletion(priv, QIOOperation::Error::None);
                }
                m_operations.removeOne(op);
            });
    m_opHandleMap.insert(priv->q_func(), m_ioring->queueRequest(std::move(writeRequest)));

    return op;
}

QT_END_NAMESPACE
