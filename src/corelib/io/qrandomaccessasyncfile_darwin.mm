// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qrandomaccessasyncfile_p_p.h"

#include "qiooperation_p.h"
#include "qiooperation_p_p.h"
#include "qplatformdefs.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/private/qfilesystemengine_p.h>

QT_BEGIN_NAMESPACE

namespace {

static bool isBarrierOperation(QIOOperation::Type type)
{
    return type == QIOOperation::Type::Flush || type == QIOOperation::Type::Open;
}

} // anonymous namespace

// Fine to provide the definition here, because all the usages are in this file
// only!
template <typename Operation, typename ...Args>
Operation *
QRandomAccessAsyncFilePrivate::addOperation(QIOOperation::Type type, qint64 offset, Args &&...args)
{
    auto dataStorage = new QtPrivate::QIOOperationDataStorage(std::forward<Args>(args)...);
    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = type;

    Operation *op = new Operation(*priv, q_ptr);
    auto opId = getNextId();
    m_operations.push_back(OperationInfo(opId, op));
    startOperationsUntilBarrier();

    return op;
}

QRandomAccessAsyncFilePrivate::QRandomAccessAsyncFilePrivate()
    : QObjectPrivate()
{
}

QRandomAccessAsyncFilePrivate::~QRandomAccessAsyncFilePrivate()
        = default;

void QRandomAccessAsyncFilePrivate::init()
{
}

void QRandomAccessAsyncFilePrivate::cancelAndWait(QIOOperation *op)
{
    auto it = std::find_if(m_operations.cbegin(), m_operations.cend(),
                           [op](const auto &opInfo) {
                               return opInfo.operation.get() == op;
                           });
    // not found
    if (it == m_operations.cend())
        return;

    const auto opInfo = m_operations.takeAt(std::distance(m_operations.cbegin(), it));

    if (opInfo.state == OpState::Running) {
        // cancel this operation
        m_mutex.lock();
        if (m_runningOps.contains(opInfo.opId)) {
            m_opToCancel = opInfo.opId;
            closeIoChannel(opInfo.channel);
            m_cancellationCondition.wait(&m_mutex);
            m_opToCancel = kInvalidOperationId; // reset
        }
        m_mutex.unlock();
    } // otherwise it was not started yet

    // clean up the operation
    releaseIoChannel(opInfo.channel);
    auto *priv = QIOOperationPrivate::get(opInfo.operation);
    priv->setError(QIOOperation::Error::Aborted);

    // we could cancel a barrier operation, so try to execute next operations
    startOperationsUntilBarrier();
}

void QRandomAccessAsyncFilePrivate::close()
{
    if (m_fileState == FileState::Closed)
        return;

    // cancel all operations
    m_mutex.lock();
    m_opToCancel = kAllOperationIds;
    for (const auto &op : m_operations) {
        if (op.channel) {
            closeIoChannel(op.channel);
        }
    }
    closeIoChannel(m_ioChannel);
    // we're not interested in any results anymore
    if (!m_runningOps.isEmpty() || m_ioChannel)
        m_cancellationCondition.wait(&m_mutex);
    m_opToCancel = kInvalidOperationId; // reset
    m_mutex.unlock();

    // clean up all operations
    for (auto &opInfo : m_operations) {
        releaseIoChannel(opInfo.channel);
        auto *priv = QIOOperationPrivate::get(opInfo.operation);
        priv->setError(QIOOperation::Error::Aborted);
    }
    m_operations.clear();

    releaseIoChannel(m_ioChannel);

    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    m_fileState = FileState::Closed;
}

qint64 QRandomAccessAsyncFilePrivate::size() const
{
    if (m_fileState != FileState::Opened)
        return -1;

    QFileSystemMetaData metaData;
    if (QFileSystemEngine::fillMetaData(m_fd, metaData))
        return metaData.size();

    return -1;
}

QIOOperation *
QRandomAccessAsyncFilePrivate::open(const QString &path, QIODeviceBase::OpenMode mode)
{
    if (m_fileState == FileState::Closed) {
        m_filePath = path;
        m_openMode = mode;
        // Open is a barrier, so we won't have two open() operations running
        // in parallel
        m_fileState = FileState::OpenPending;
    }

    return addOperation<QIOOperation>(QIOOperation::Type::Open, 0);
}

QIOOperation *QRandomAccessAsyncFilePrivate::flush()
{
    return addOperation<QIOOperation>(QIOOperation::Type::Flush, 0);
}

QIOReadOperation *QRandomAccessAsyncFilePrivate::read(qint64 offset, qint64 maxSize)
{
    QByteArray array(maxSize, Qt::Uninitialized);
    return addOperation<QIOReadOperation>(QIOOperation::Type::Read, offset, std::move(array));
}

QIOWriteOperation *QRandomAccessAsyncFilePrivate::write(qint64 offset, const QByteArray &data)
{
    QByteArray copy = data;
    return write(offset, std::move(copy));
}

QIOWriteOperation *QRandomAccessAsyncFilePrivate::write(qint64 offset, QByteArray &&data)
{
    return addOperation<QIOWriteOperation>(QIOOperation::Type::Write, offset, std::move(data));
}

QIOVectoredReadOperation *
QRandomAccessAsyncFilePrivate::readInto(qint64 offset, QSpan<std::byte> buffer)
{
    return addOperation<QIOVectoredReadOperation>(QIOOperation::Type::Read, offset,
                                                  QSpan<const QSpan<std::byte>>{buffer});
}

QIOVectoredWriteOperation *
QRandomAccessAsyncFilePrivate::writeFrom(qint64 offset, QSpan<const std::byte> buffer)
{
    return addOperation<QIOVectoredWriteOperation>(QIOOperation::Type::Write, offset,
                                                   QSpan<const QSpan<const std::byte>>{buffer});
}

QIOVectoredReadOperation *
QRandomAccessAsyncFilePrivate::readInto(qint64 offset, QSpan<const QSpan<std::byte>> buffers)
{
    // GCD implementation does not have vectored read. Spawning several read
    // operations (each with an updated offset), is not ideal, because some
    // of them could fail, and it wouldn't be clear what would be the return
    // value in such case.
    // So, we'll just execute several reads one-after-another, and complete the
    // whole operation only when they all finish (or when an operation fails
    // at some point).

    return addOperation<QIOVectoredReadOperation>(QIOOperation::Type::Read, offset, buffers);
}

QIOVectoredWriteOperation *
QRandomAccessAsyncFilePrivate::writeFrom(qint64 offset, QSpan<const QSpan<const std::byte>> buffers)
{
    return addOperation<QIOVectoredWriteOperation>(QIOOperation::Type::Write, offset, buffers);
}

void QRandomAccessAsyncFilePrivate::notifyIfOperationsAreCompleted()
{
    QMutexLocker locker(&m_mutex);
    --m_numChannelsToClose;
    if (m_opToCancel == kAllOperationIds) {
        if (m_numChannelsToClose == 0 && m_runningOps.isEmpty())
            m_cancellationCondition.wakeOne();
    }
}

dispatch_io_t QRandomAccessAsyncFilePrivate::createMainChannel(int fd)
{
    auto sharedThis = this;
    auto channel =
            dispatch_io_create(DISPATCH_IO_RANDOM, fd,
                               dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
                               ^(int /*error*/) {
                                   // main I/O channel uses kInvalidOperationId
                                   // as its identifier
                                   sharedThis->notifyIfOperationsAreCompleted();
                               });
    if (channel) {
        QMutexLocker locker(&m_mutex);
        ++m_numChannelsToClose;
    }
    return channel;
}

dispatch_io_t QRandomAccessAsyncFilePrivate::duplicateIoChannel(OperationId opId)
{
    if (!m_ioChannel)
        return nullptr;
    // We need to create a new channel for each operation, because the only way
    // to cancel an operation is to call dispatch_io_close() with
    // DISPATCH_IO_STOP flag.
    auto sharedThis = this;
    auto channel =
            dispatch_io_create_with_io(DISPATCH_IO_RANDOM, m_ioChannel,
                                       dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
                                       ^(int /*error*/){
                                           sharedThis->notifyIfOperationsAreCompleted();
                                       });

    if (channel) {
        QMutexLocker locker(&m_mutex);
        m_runningOps.insert(opId);
        ++m_numChannelsToClose;
    }
    return channel;
}

void QRandomAccessAsyncFilePrivate::closeIoChannel(dispatch_io_t channel)
{
    if (channel)
        dispatch_io_close(channel, DISPATCH_IO_STOP);
}

void QRandomAccessAsyncFilePrivate::releaseIoChannel(dispatch_io_t channel)
{
    if (channel) {
        dispatch_release(channel);
        channel = nullptr;
    }
}

void QRandomAccessAsyncFilePrivate::handleOperationComplete(const OperationResult &opResult)
{
    // try to start next operations on return
    auto onReturn = qScopeGuard([this] {
        startOperationsUntilBarrier();
    });

    auto it = std::find_if(m_operations.cbegin(), m_operations.cend(),
                           [opId = opResult.opId](const auto &opInfo) {
                               return opInfo.opId == opId;
                           });
    if (it == m_operations.cend())
        return;
    qsizetype idx = std::distance(m_operations.cbegin(), it);

    const OperationInfo info = m_operations.takeAt(idx);
    closeIoChannel(info.channel);
    releaseIoChannel(info.channel);

    if (!info.operation)
        return;

    auto convertError = [](int error, QIOOperation::Type type) {
        if (error == 0) {
            return QIOOperation::Error::None;
        } else if (error == ECANCELED) {
            return QIOOperation::Error::Aborted;
        } else if (error == EBADF) {
            return QIOOperation::Error::FileNotOpen;
        } else if (error == EINVAL) {
            switch (type) {
            case QIOOperation::Type::Read:
            case QIOOperation::Type::Write:
                return QIOOperation::Error::IncorrectOffset;
            case QIOOperation::Type::Flush:
                return QIOOperation::Error::Flush;
            case QIOOperation::Type::Open:
                return QIOOperation::Error::Open;
            case QIOOperation::Type::Unknown:
                Q_UNREACHABLE_RETURN(QIOOperation::Error::FileNotOpen);
            }
        } else {
            switch (type) {
            case QIOOperation::Type::Read:
                return QIOOperation::Error::Read;
            case QIOOperation::Type::Write:
                return QIOOperation::Error::Write;
            case QIOOperation::Type::Flush:
                return QIOOperation::Error::Flush;
            case QIOOperation::Type::Open:
                return QIOOperation::Error::Open;
            case QIOOperation::Type::Unknown:
                Q_UNREACHABLE_RETURN(QIOOperation::Error::FileNotOpen);
            }
        }
    };

    auto *priv = QIOOperationPrivate::get(info.operation);
    switch (priv->type) {
    case QIOOperation::Type::Read:
    case QIOOperation::Type::Write:
        priv->appendBytesProcessed(opResult.result);
        // make sure that read buffers are truncated to the actual amount of
        // bytes read
        if (priv->type == QIOOperation::Type::Read) {
            auto dataStorage = priv->dataStorage.get();
            auto processed = priv->processed;
            if (dataStorage->containsByteArray()) {
                QByteArray &array = dataStorage->getByteArray();
                array.truncate(processed);
            } else if (dataStorage->containsReadSpans()) {
                qint64 left = processed;
                auto &readBuffers = dataStorage->getReadSpans();
                for (auto &s : readBuffers) {
                    const qint64 spanSize = qint64(s.size_bytes());
                    const qint64 newSize = (std::min)(left, spanSize);
                    if (newSize < spanSize)
                        s.chop(spanSize - newSize);
                    left -= newSize;
                }
            }
        }
        priv->operationComplete(convertError(opResult.error, priv->type));
        break;
    case QIOOperation::Type::Flush: {
        const QIOOperation::Error error = convertError(opResult.error, priv->type);
        priv->operationComplete(error);
        break;
    }
    case QIOOperation::Type::Open: {
        const QIOOperation::Error error = convertError(opResult.error, priv->type);
        if (opResult.result >= 0 && error == QIOOperation::Error::None) {
            m_fd = (int)opResult.result;
            m_ioChannel = createMainChannel(m_fd);
            m_fileState = FileState::Opened;
        } else {
            m_fileState = FileState::Closed;
        }
        priv->operationComplete(error);
        break;
    }
    case QIOOperation::Type::Unknown:
        Q_UNREACHABLE();
        break;
    }
}

void QRandomAccessAsyncFilePrivate::queueCompletion(OperationId opId, int error)
{
    const OperationResult res = { opId, 0LL, error };
    QMetaObject::invokeMethod(q_ptr, [this, res] {
        handleOperationComplete(res);
    }, Qt::QueuedConnection);
}

void QRandomAccessAsyncFilePrivate::startOperationsUntilBarrier()
{
    // starts all operations until barrier, or a barrier operation if it's the
    // first one
    bool first = true;
    for (auto &opInfo : m_operations) {
        const bool isBarrier = isBarrierOperation(opInfo.operation->type());
        const bool shouldExecute = (opInfo.state == OpState::Pending) && (!isBarrier || first);
        first = false;
        if (shouldExecute) {
            opInfo.state = OpState::Running;
            switch (opInfo.operation->type()) {
            case QIOOperation::Type::Read:
                executeRead(opInfo);
                break;
            case QIOOperation::Type::Write:
                executeWrite(opInfo);
                break;
            case QIOOperation::Type::Flush:
                executeFlush(opInfo);
                break;
            case QIOOperation::Type::Open:
                executeOpen(opInfo);
                break;
            case QIOOperation::Type::Unknown:
                Q_UNREACHABLE();
                break;
            }
        }
        if (isBarrier)
            break;
    }
}

void QRandomAccessAsyncFilePrivate::executeRead(OperationInfo &opInfo)
{
    opInfo.channel = duplicateIoChannel(opInfo.opId);
    if (!opInfo.channel) {
        queueCompletion(opInfo.opId, EBADF);
        return;
    }
    auto priv = QIOOperationPrivate::get(opInfo.operation);
    auto dataStorage = priv->dataStorage.get();
    if (dataStorage->containsByteArray()) {
        auto &array = dataStorage->getByteArray();
        char *bytesPtr = array.data();
        qint64 maxSize = array.size();
        readOneBufferHelper(opInfo.opId, opInfo.channel, priv->offset,
                            bytesPtr, maxSize,
                            0, 1, 0);
    } else {
        Q_ASSERT(dataStorage->containsReadSpans());
        auto &readBuffers = dataStorage->getReadSpans();
        const auto totalBuffers = readBuffers.size();
        if (totalBuffers == 0) {
            queueCompletion(opInfo.opId, 0);
            return;
        }
        auto buf = readBuffers[0];
        readOneBufferHelper(opInfo.opId, opInfo.channel, priv->offset,
                            buf.data(), buf.size(),
                            0, totalBuffers, 0);
    }
}

void QRandomAccessAsyncFilePrivate::executeWrite(OperationInfo &opInfo)
{
    opInfo.channel = duplicateIoChannel(opInfo.opId);
    if (!opInfo.channel) {
        queueCompletion(opInfo.opId, EBADF);
        return;
    }
    auto priv = QIOOperationPrivate::get(opInfo.operation);
    auto dataStorage = priv->dataStorage.get();
    if (dataStorage->containsByteArray()) {
        const auto &array = dataStorage->getByteArray();
        const char *dataPtr = array.constData();
        const qint64 dataSize = array.size();

        dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0);
        // We handle the bytes on our own, so we need to specify an empty block as
        // a destructor.
        // dataToWrite is retained, so should be properly cleaned up. We always do
        // it in the callback.
        dispatch_data_t dataToWrite = dispatch_data_create(dataPtr, dataSize, queue, ^{});

        writeHelper(opInfo.opId, opInfo.channel, priv->offset, dataToWrite, dataSize);
    } else {
        Q_ASSERT(dataStorage->containsWriteSpans());

        const auto &writeBuffers = dataStorage->getWriteSpans();
        const auto totalBuffers = writeBuffers.size();
        if (totalBuffers == 0) {
            queueCompletion(opInfo.opId, 0);
            return;
        }

        dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0);
        qsizetype idx = 0;
        dispatch_data_t dataToWrite = dispatch_data_empty;
        qint64 totalSize = 0;
        do {
            const std::byte *dataPtr = writeBuffers[idx].data();
            const qint64 dataSize = writeBuffers[idx].size();
            dispatch_data_t data = dispatch_data_create(dataPtr, dataSize, queue, ^{});
            dataToWrite = dispatch_data_create_concat(dataToWrite, data);
            [data release];
            totalSize += dataSize;
        } while (++idx < totalBuffers);

        writeHelper(opInfo.opId, opInfo.channel, priv->offset, dataToWrite, totalSize);
    }
}

void QRandomAccessAsyncFilePrivate::executeFlush(OperationInfo &opInfo)
{
    opInfo.channel = duplicateIoChannel(opInfo.opId);
    if (!opInfo.channel) {
        queueCompletion(opInfo.opId, EBADF);
        return;
    }

    // flush() is a barrier operation, but dispatch_io_barrier does not work
    // as documented with multiple channels :(
    auto sharedThis = this;
    const int fd = m_fd;
    const OperationId opId = opInfo.opId;
    dispatch_io_barrier(opInfo.channel, ^{
        const int err = fsync(fd);

        QMutexLocker locker(&sharedThis->m_mutex);
        sharedThis->m_runningOps.remove(opId);
        const auto cancelId = sharedThis->m_opToCancel;
        if (cancelId == kAllOperationIds || cancelId == opId) {
            if (cancelId == opId) {
                sharedThis->m_cancellationCondition.wakeOne();
            } else { /* kAllOperationIds */
                if (sharedThis->m_numChannelsToClose == 0
                    && sharedThis->m_runningOps.isEmpty()) {
                    sharedThis->m_cancellationCondition.wakeOne();
                }
            }
        } else {
            auto context = sharedThis->q_ptr;
            const OperationResult res = { opId, 0LL, err };
            QMetaObject::invokeMethod(context, [sharedThis](const OperationResult &r) {
                sharedThis->handleOperationComplete(r);
            }, Qt::QueuedConnection, res);
        }
    });
}

// stolen from qfsfileengine_unix.cpp
static inline int openModeToOpenFlags(QIODevice::OpenMode mode)
{
    int oflags = QT_OPEN_RDONLY;
#ifdef QT_LARGEFILE_SUPPORT
    oflags |= QT_OPEN_LARGEFILE;
#endif
    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite)
        oflags = QT_OPEN_RDWR;
    else if (mode & QIODevice::WriteOnly)
        oflags = QT_OPEN_WRONLY;
    if ((mode & QIODevice::WriteOnly)
        && !(mode & QIODevice::ExistingOnly)) // QFSFileEnginePrivate::openModeCanCreate(mode))
        oflags |= QT_OPEN_CREAT;
    if (mode & QIODevice::Truncate)
        oflags |= QT_OPEN_TRUNC;
    if (mode & QIODevice::Append)
        oflags |= QT_OPEN_APPEND;
    if (mode & QIODevice::NewOnly)
        oflags |= QT_OPEN_EXCL;
    return oflags;
}

void QRandomAccessAsyncFilePrivate::executeOpen(OperationInfo &opInfo)
{
    if (m_fileState != FileState::OpenPending) {
        queueCompletion(opInfo.opId, EINVAL);
        return;
    }

    const QByteArray nativeName = QFile::encodeName(QDir::toNativeSeparators(m_filePath));

    int openFlags = openModeToOpenFlags(m_openMode);
    openFlags |= O_NONBLOCK;

    auto sharedThis = this;
    const OperationId opId = opInfo.opId;

    // We don'd call duplicateIOChannel(), so need to update the running ops
    // explicitly.
    m_mutex.lock();
    m_runningOps.insert(opId);
    m_mutex.unlock();

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
                   ^{
                       int err = 0;
                       const int fd = ::open(nativeName.data(), openFlags);
                       if (fd < 0)
                           err = errno;

                       QMutexLocker locker(&sharedThis->m_mutex);
                       sharedThis->m_runningOps.remove(opId);
                       const auto cancelId = sharedThis->m_opToCancel;
                       if (cancelId == kAllOperationIds || cancelId == opId) {
                           // open() is a barrier operation, so it's always the
                           // only executing operation.
                           // Also, the main IO channel is not created yet.
                           // So we need to notify the condition variable in
                           // both cases.
                           Q_ASSERT(sharedThis->m_runningOps.isEmpty());
                           Q_ASSERT(sharedThis->m_numChannelsToClose == 0);
                           sharedThis->m_cancellationCondition.wakeOne();
                       } else {
                           auto context = sharedThis->q_ptr;
                           const OperationResult res = { opId, qint64(fd), err };
                           QMetaObject::invokeMethod(context,
                               [sharedThis](const OperationResult &r) {
                                   sharedThis->handleOperationComplete(r);
                               }, Qt::QueuedConnection, res);
                       }
                   });
}

void QRandomAccessAsyncFilePrivate::readOneBuffer(OperationId opId, qsizetype bufferIdx,
                                                  qint64 alreadyRead)
{
    // we need to lookup the operation again, because it could have beed removed
    // by the user...

    auto it = std::find_if(m_operations.cbegin(), m_operations.cend(),
                           [opId](const auto &opInfo) {
                               return opId == opInfo.opId;
                           });
    if (it == m_operations.cend())
        return;

    auto op = it->operation; // QPointer could be null
    if (!op) {
        closeIoChannel(it->channel);
        return;
    }

    auto *priv = QIOOperationPrivate::get(op);
    Q_ASSERT(priv->type == QIOOperation::Type::Read);
    Q_ASSERT(priv->dataStorage->containsReadSpans());

    auto &readBuffers = priv->dataStorage->getReadSpans();
    Q_ASSERT(readBuffers.size() > bufferIdx);

    qint64 newOffset = priv->offset;
    for (qsizetype idx = 0; idx < bufferIdx; ++idx)
        newOffset += readBuffers[idx].size();

    std::byte *bytesPtr = readBuffers[bufferIdx].data();
    qint64 maxSize = readBuffers[bufferIdx].size();

    readOneBufferHelper(opId, it->channel, newOffset, bytesPtr, maxSize, bufferIdx,
                        readBuffers.size(), alreadyRead);
}

void QRandomAccessAsyncFilePrivate::readOneBufferHelper(OperationId opId, dispatch_io_t channel,
                                                        qint64 offset, void *bytesPtr,
                                                        qint64 maxSize, qsizetype currentBufferIdx,
                                                        qsizetype totalBuffers, qint64 alreadyRead)
{
    auto sharedThis = this;
    __block size_t readFromBuffer = 0;
    dispatch_io_read(channel, offset, maxSize,
                     dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
                     ^(bool done, dispatch_data_t data, int error) {
                         // Handle data. If there's an error, handle as much as
                         // we can.
                         if (data) {
                             dispatch_data_apply(data, ^(dispatch_data_t /*region*/, size_t offset,
                                                         const void *buffer, size_t size) {
                                 const char *startPtr =
                                         reinterpret_cast<const char *>(buffer) + offset;
                                 // NOTE: This is a copy, but looks like we
                                 // cannot do better :(
                                 std::memcpy((std::byte *)bytesPtr + readFromBuffer,
                                             startPtr, size);
                                 readFromBuffer += size;
                                 return true;  // Keep processing if there is more data.
                             });
                         }

                         // We're interested in handling the results only when
                         // the operation is done. This can mean either
                         // successful completion or an error (including
                         // cancellation).
                         if (!done)
                             return;


                         QMutexLocker locker(&sharedThis->m_mutex);
                         const auto cancelId = sharedThis->m_opToCancel;
                         if (cancelId == kAllOperationIds || cancelId == opId) {
                             sharedThis->m_runningOps.remove(opId);
                             if (cancelId == opId) {
                                 sharedThis->m_cancellationCondition.wakeOne();
                             } else { /* kAllOperationIds */
                                 if (sharedThis->m_numChannelsToClose == 0
                                     && sharedThis->m_runningOps.isEmpty()) {
                                     sharedThis->m_cancellationCondition.wakeOne();
                                 }
                             }
                         } else {
                             sharedThis->m_runningOps.remove(opId);
                             auto context = sharedThis->q_ptr;
                             // if error, or last buffer, or read less than expected,
                             // report operation completion
                             qint64 totalRead = qint64(readFromBuffer) + alreadyRead;
                             qsizetype nextBufferIdx = currentBufferIdx + 1;
                             if (error || nextBufferIdx == totalBuffers
                                 || qint64(readFromBuffer) != maxSize) {
                                 const OperationResult res = { opId, totalRead, error };
                                 QMetaObject::invokeMethod(context,
                                     [sharedThis](const OperationResult &r) {
                                         sharedThis->handleOperationComplete(r);
                                     }, Qt::QueuedConnection, res);
                             } else {
                                 // else execute read for the next buffer
                                 QMetaObject::invokeMethod(context,
                                     [sharedThis, opId, nextBufferIdx, totalRead] {
                                         sharedThis->readOneBuffer(opId, nextBufferIdx, totalRead);
                                     }, Qt::QueuedConnection);
                             }
                         }
                     });
}

void QRandomAccessAsyncFilePrivate::writeHelper(OperationId opId, dispatch_io_t channel,
                                                qint64 offset, dispatch_data_t dataToWrite,
                                                qint64 dataSize)
{
    auto sharedThis = this;
    dispatch_io_write(channel, offset, dataToWrite,
                      dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
                      ^(bool done, dispatch_data_t data, int error) {
                          // We're interested in handling the results only when
                          // the operation is done. This can mean either
                          // successful completion or an error (including
                          // cancellation). In case of an error return the
                          // amount that we have written so far.
                          if (!done)
                              return;

                          QMutexLocker locker(&sharedThis->m_mutex);
                          const auto cancelId = sharedThis->m_opToCancel;
                          if (cancelId == kAllOperationIds || cancelId == opId) {
                              // Operation is canceled - do nothing
                              sharedThis->m_runningOps.remove(opId);
                              if (cancelId == opId) {
                                  sharedThis->m_cancellationCondition.wakeOne();
                              } else { /* kAllOperationIds */
                                  if (sharedThis->m_numChannelsToClose == 0
                                      && sharedThis->m_runningOps.isEmpty()) {
                                      sharedThis->m_cancellationCondition.wakeOne();
                                  }
                              }
                          } else {
                              sharedThis->m_runningOps.remove(opId);
                              // if no error, an attempt to access the data will
                              // crash, because it seems to have no buffer
                              // allocated (as everything was written)
                              const size_t toBeWritten =
                                      (error == 0) ? 0 : dispatch_data_get_size(data);
                              const size_t written = dataSize - toBeWritten;
                              [dataToWrite release];

                              auto context = sharedThis->q_ptr;
                              const OperationResult res = { opId, qint64(written), error };
                              QMetaObject::invokeMethod(context,
                                  [sharedThis](const OperationResult &r) {
                                      sharedThis->handleOperationComplete(r);
                                  }, Qt::QueuedConnection, res);
                          }
                      });
}

QRandomAccessAsyncFilePrivate::OperationId QRandomAccessAsyncFilePrivate::getNextId()
{
    // never return reserved values
    static OperationId opId = kInvalidOperationId;
    if (++opId == kAllOperationIds)
        opId = kInvalidOperationId + 1;
    return opId;
}

QT_END_NAMESPACE
