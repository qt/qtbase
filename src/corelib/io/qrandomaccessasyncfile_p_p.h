// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRANDOMACCESSASYNCFILE_P_P_H
#define QRANDOMACCESSASYNCFILE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qrandomaccessasyncfile_p.h"

#include <QtCore/private/qobject_p.h>

#include <QtCore/qstring.h>

#if QT_CONFIG(future) && QT_CONFIG(thread)

#include <QtCore/private/qfsfileengine_p.h>

#include <QtCore/qfuturewatcher.h>
#include <QtCore/qmutex.h>
#include <QtCore/qqueue.h>

#endif // future && thread

#ifdef Q_OS_DARWIN

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qset.h>
#include <QtCore/qwaitcondition.h>

#include <dispatch/dispatch.h>

#endif // Q_OS_DARWIN

#ifdef QT_RANDOMACCESSASYNCFILE_QIORING
#include <QtCore/private/qioring_p.h>
#include <QtCore/qlist.h>
#endif

QT_BEGIN_NAMESPACE

class QRandomAccessAsyncFileBackend
{
    Q_DISABLE_COPY_MOVE(QRandomAccessAsyncFileBackend)
public:
    explicit QRandomAccessAsyncFileBackend(QRandomAccessAsyncFile *owner);
    virtual ~QRandomAccessAsyncFileBackend();

    virtual bool init() = 0;
    virtual void cancelAndWait(QIOOperation *op) = 0;

    virtual void close() = 0;
    virtual qint64 size() const = 0;

    [[nodiscard]] virtual QIOOperation *open(const QString &path, QIODeviceBase::OpenMode mode) = 0;
    [[nodiscard]] virtual QIOOperation *flush() = 0;

    [[nodiscard]] virtual QIOReadOperation *read(qint64 offset, qint64 maxSize) = 0;
    [[nodiscard]] virtual QIOWriteOperation *write(qint64 offset, const QByteArray &data) = 0;
    [[nodiscard]] virtual QIOWriteOperation *write(qint64 offset, QByteArray &&data) = 0;

    [[nodiscard]] virtual QIOVectoredReadOperation *
    readInto(qint64 offset, QSpan<std::byte> buffer) = 0;
    [[nodiscard]] virtual QIOVectoredWriteOperation *
    writeFrom(qint64 offset, QSpan<const std::byte> buffer) = 0;

    [[nodiscard]] virtual QIOVectoredReadOperation *
    readInto(qint64 offset, QSpan<const QSpan<std::byte>> buffers) = 0;
    [[nodiscard]] virtual QIOVectoredWriteOperation *
    writeFrom(qint64 offset, QSpan<const QSpan<const std::byte>> buffers) = 0;
protected:
    // common for all backends
    enum class FileState : quint8
    {
        Closed,
        OpenPending, // already got an open request
        Opened,
    };

    QString m_filePath;
    QRandomAccessAsyncFile *m_owner = nullptr;
    QIODeviceBase::OpenMode m_openMode;
    FileState m_fileState = FileState::Closed;
};

class QRandomAccessAsyncFilePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QRandomAccessAsyncFile)
    Q_DISABLE_COPY_MOVE(QRandomAccessAsyncFilePrivate)
public:
    QRandomAccessAsyncFilePrivate();
    ~QRandomAccessAsyncFilePrivate() override;

    static QRandomAccessAsyncFilePrivate *get(QRandomAccessAsyncFile *file)
    { return file->d_func(); }

    void init();
    void cancelAndWait(QIOOperation *op)
    {
        checkValid();
        m_backend->cancelAndWait(op);
    }

    void close()
    {
        checkValid();
        m_backend->close();
    }
    qint64 size() const
    {
        checkValid();
        return m_backend->size();
    }

    [[nodiscard]] QIOOperation *open(const QString &path, QIODeviceBase::OpenMode mode)
    {
        checkValid();
        return m_backend->open(path, mode);
    }
    [[nodiscard]] QIOOperation *flush()
    {
        checkValid();
        return m_backend->flush();
    }

    [[nodiscard]] QIOReadOperation *read(qint64 offset, qint64 maxSize)
    {
        checkValid();
        return m_backend->read(offset, maxSize);
    }
    [[nodiscard]] QIOWriteOperation *write(qint64 offset, const QByteArray &data)
    {
        checkValid();
        return m_backend->write(offset, data);
    }
    [[nodiscard]] QIOWriteOperation *write(qint64 offset, QByteArray &&data)
    {
        checkValid();
        return m_backend->write(offset, std::move(data));
    }

    [[nodiscard]] QIOVectoredReadOperation *readInto(qint64 offset, QSpan<std::byte> buffer)
    {
        checkValid();
        return m_backend->readInto(offset, buffer);
    }
    [[nodiscard]] QIOVectoredWriteOperation *writeFrom(qint64 offset, QSpan<const std::byte> buffer)
    {
        checkValid();
        return m_backend->writeFrom(offset, buffer);
    }

    [[nodiscard]] QIOVectoredReadOperation *readInto(qint64 offset,
                                                     QSpan<const QSpan<std::byte>> buffers)
    {
        checkValid();
        return m_backend->readInto(offset, buffers);
    }
    [[nodiscard]] QIOVectoredWriteOperation *writeFrom(qint64 offset,
                                                       QSpan<const QSpan<const std::byte>> buffers)
    {
        checkValid();
        return m_backend->writeFrom(offset, buffers);
    }

private:
    void checkValid() const { Q_ASSERT(m_backend); }
    std::unique_ptr<QRandomAccessAsyncFileBackend> m_backend;

};


#if defined(QT_RANDOMACCESSASYNCFILE_QIORING) || defined(Q_OS_DARWIN)
class QRandomAccessAsyncFileNativeBackend final : public QRandomAccessAsyncFileBackend
{
    Q_DISABLE_COPY_MOVE(QRandomAccessAsyncFileNativeBackend)
public:
    explicit QRandomAccessAsyncFileNativeBackend(QRandomAccessAsyncFile *owner);
    ~QRandomAccessAsyncFileNativeBackend();

    bool init() override;
    void cancelAndWait(QIOOperation *op) override;

    void close() override;
    qint64 size() const override;

    [[nodiscard]] QIOOperation *open(const QString &path, QIODeviceBase::OpenMode mode) override;
    [[nodiscard]] QIOOperation *flush() override;

    [[nodiscard]] QIOReadOperation *read(qint64 offset, qint64 maxSize) override;
    [[nodiscard]] QIOWriteOperation *write(qint64 offset, const QByteArray &data) override;
    [[nodiscard]] QIOWriteOperation *write(qint64 offset, QByteArray &&data) override;

    [[nodiscard]] QIOVectoredReadOperation *
    readInto(qint64 offset, QSpan<std::byte> buffer) override;
    [[nodiscard]] QIOVectoredWriteOperation *
    writeFrom(qint64 offset, QSpan<const std::byte> buffer) override;

    [[nodiscard]] QIOVectoredReadOperation *
    readInto(qint64 offset, QSpan<const QSpan<std::byte>> buffers) override;
    [[nodiscard]] QIOVectoredWriteOperation *
    writeFrom(qint64 offset, QSpan<const QSpan<const std::byte>> buffers) override;

private:
#if defined(QT_RANDOMACCESSASYNCFILE_QIORING)
    void queueCompletion(QIOOperationPrivate *priv, QIOOperation::Error error);
    void startReadIntoSingle(QIOOperation *op, const QSpan<std::byte> &to);
    void startWriteFromSingle(QIOOperation *op, const QSpan<const std::byte> &from);
    QIORing::RequestHandle cancel(QIORing::RequestHandle handle);
    QIORing *m_ioring = nullptr;
    qintptr m_fd = -1;
    QList<QPointer<QIOOperation>> m_operations;
    QHash<QIOOperation *, QIORing::RequestHandle> m_opHandleMap;
#endif
#ifdef Q_OS_DARWIN
    using OperationId = quint64;
    static constexpr OperationId kInvalidOperationId = 0;
    static constexpr OperationId kAllOperationIds = std::numeric_limits<OperationId>::max();

    struct OperationResult
    {
        OperationId opId;
        qint64 result; // num bytes processed or file descriptor
        int error;
    };

    enum class OpState : quint8
    {
        Pending,
        Running,
    };

    struct OperationInfo
    {
        OperationId opId;
        dispatch_io_t channel;
        QPointer<QIOOperation> operation;
        OpState state;

        OperationInfo(OperationId _id, QIOOperation *_op)
            : opId(_id),
              channel(nullptr),
              operation(_op),
              state(OpState::Pending)
        {}
    };

    // We need to maintain an actual queue of the operations, because
    // certain operations (i.e. flush) should act like barriers. The docs
    // for dispatch_io_barrier mention that it can synchronize between multiple
    // channels handling the same file descriptor, but that DOES NOT work in
    // practice. It works correctly only when there's a signle IO channel. But
    // with a signle IO channel we're not able to cancel individual operations.
    // As a result, we need to make sure that all previous operations are
    // completed before starting a barrier operation. Similarly, we cannot start
    // any other operation until a barrier operation is finished.
    QList<OperationInfo> m_operations;
    dispatch_io_t m_ioChannel = nullptr;
    int m_fd = -1;

    QMutex m_mutex;
    // the members below should only be accessed with the mutex
    OperationId m_opToCancel = kInvalidOperationId;
    QSet<OperationId> m_runningOps;
    qsizetype m_numChannelsToClose = 0;
    QWaitCondition m_cancellationCondition;

    static OperationId getNextId();

    template <typename Operation, typename ...Args>
    Operation *addOperation(QIOOperation::Type type, qint64 offset, Args &&...args);

    void notifyIfOperationsAreCompleted();
    dispatch_io_t createMainChannel(int fd);
    dispatch_io_t duplicateIoChannel(OperationId opId);
    void closeIoChannel(dispatch_io_t channel);
    void releaseIoChannel(dispatch_io_t channel);
    void handleOperationComplete(const OperationResult &opResult);

    void queueCompletion(OperationId opId, int error);

    void startOperationsUntilBarrier();
    void executeRead(OperationInfo &opInfo);
    void executeWrite(OperationInfo &opInfo);
    void executeFlush(OperationInfo &opInfo);
    void executeOpen(OperationInfo &opInfo);

    void readOneBuffer(OperationId opId, qsizetype bufferIdx, qint64 alreadyRead);
    void readOneBufferHelper(OperationId opId, dispatch_io_t channel, qint64 offset,
                             void *bytesPtr, qint64 maxSize, qsizetype currentBufferIdx,
                             qsizetype totalBuffers, qint64 alreadyRead);
    void writeHelper(OperationId opId, dispatch_io_t channel, qint64 offset,
                     dispatch_data_t dataToWrite, qint64 dataSize);
#endif
};
#endif // QIORing || macOS

#if QT_CONFIG(future) && QT_CONFIG(thread)
class QRandomAccessAsyncFileThreadPoolBackend : public QRandomAccessAsyncFileBackend
{
    Q_DISABLE_COPY_MOVE(QRandomAccessAsyncFileThreadPoolBackend)
public:
    explicit QRandomAccessAsyncFileThreadPoolBackend(QRandomAccessAsyncFile *owner);
    ~QRandomAccessAsyncFileThreadPoolBackend();

    bool init() override;
    void cancelAndWait(QIOOperation *op) override;

    void close() override;
    qint64 size() const override;

    [[nodiscard]] QIOOperation *open(const QString &path, QIODeviceBase::OpenMode mode) override;
    [[nodiscard]] QIOOperation *flush() override;

    [[nodiscard]] QIOReadOperation *read(qint64 offset, qint64 maxSize) override;
    [[nodiscard]] QIOWriteOperation *write(qint64 offset, const QByteArray &data) override;
    [[nodiscard]] QIOWriteOperation *write(qint64 offset, QByteArray &&data) override;

    [[nodiscard]] QIOVectoredReadOperation *
    readInto(qint64 offset, QSpan<std::byte> buffer) override;
    [[nodiscard]] QIOVectoredWriteOperation *
    writeFrom(qint64 offset, QSpan<const std::byte> buffer) override;

    [[nodiscard]] QIOVectoredReadOperation *
    readInto(qint64 offset, QSpan<const QSpan<std::byte>> buffers) override;
    [[nodiscard]] QIOVectoredWriteOperation *
    writeFrom(qint64 offset, QSpan<const QSpan<const std::byte>> buffers) override;

    struct OperationResult
    {
        qint64 bytesProcessed; // either read or written
        QIOOperation::Error error;
    };
private:

    mutable QBasicMutex m_engineMutex;
    std::unique_ptr<QFSFileEngine> m_engine;
    QFutureWatcher<OperationResult> m_watcher;

    QQueue<QPointer<QIOOperation>> m_operations;
    QPointer<QIOOperation> m_currentOperation;
    qsizetype numProcessedBuffers = 0;

    void executeNextOperation();
    void processBufferAt(qsizetype idx);
    void processFlush();
    void processOpen();
    void operationComplete();
};
#endif // future && thread

QT_END_NAMESPACE

#endif // QRANDOMACCESSASYNCFILE_P_P_H
