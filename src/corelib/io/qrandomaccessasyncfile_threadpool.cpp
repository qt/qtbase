// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qrandomaccessasyncfile_p_p.h"

#include "qiooperation_p.h"
#include "qiooperation_p_p.h"

#include <QtCore/qfuture.h>
#include <QtCore/qthread.h>
#include <QtCore/qthreadpool.h>

QT_REQUIRE_CONFIG(thread);
QT_REQUIRE_CONFIG(future);

QT_BEGIN_NAMESPACE

namespace {

// We cannot use Q_GLOBAL_STATIC(QThreadPool, foo) because the Windows
// implementation raises a qWarning in its destructor when used as a global
// static, and this warning leads to a crash on Windows CI. Cannot reproduce
// the crash locally, so cannot really fix the issue :(
// This class should act like a global thread pool, but it'll have a sort of
// ref counting, and will be created/destroyed by QRAAFP instances.
class SharedThreadPool
{
public:
    void ref()
    {
        QMutexLocker locker(&m_mutex);
        if (m_refCount == 0) {
            Q_ASSERT(!m_pool);
            m_pool = new QThreadPool;
        }
        ++m_refCount;
    }

    void deref()
    {
        QMutexLocker locker(&m_mutex);
        Q_ASSERT(m_refCount);
        if (--m_refCount == 0) {
            delete m_pool;
            m_pool = nullptr;
        }
    }

    QThreadPool *operator()()
    {
        QMutexLocker locker(&m_mutex);
        Q_ASSERT(m_refCount > 0);
        return m_pool;
    }

private:
    QBasicMutex m_mutex;
    QThreadPool *m_pool = nullptr;
    quint64 m_refCount = 0;
};

static SharedThreadPool asyncFileThreadPool;

} // anonymous namespace

QRandomAccessAsyncFileThreadPoolBackend::QRandomAccessAsyncFileThreadPoolBackend(QRandomAccessAsyncFile *owner) :
    QRandomAccessAsyncFileBackend(owner)
{
    asyncFileThreadPool.ref();
}

QRandomAccessAsyncFileThreadPoolBackend::~QRandomAccessAsyncFileThreadPoolBackend()
{
    asyncFileThreadPool.deref();
}

bool QRandomAccessAsyncFileThreadPoolBackend::init()
{
    QObject::connect(&m_watcher, &QFutureWatcherBase::finished, m_owner, [this]{
        operationComplete();
    });
    QObject::connect(&m_watcher, &QFutureWatcherBase::canceled, m_owner, [this]{
        operationComplete();
    });
    return true;
}

void QRandomAccessAsyncFileThreadPoolBackend::cancelAndWait(QIOOperation *op)
{
    if (op == m_currentOperation) {
        m_currentOperation = nullptr; // to discard the result
        m_watcher.cancel(); // might have no effect
        m_watcher.waitForFinished();
    } else {
        m_operations.removeAll(op);
    }
}

QIOOperation *
QRandomAccessAsyncFileThreadPoolBackend::open(const QString &path, QIODeviceBase::OpenMode mode)
{
    // We generate the command in any case. But if the file is already opened,
    // it will finish with an error
    if (m_fileState == FileState::Closed) {
        m_filePath = path;
        m_openMode = mode;
        m_fileState = FileState::OpenPending;
    }

    auto *dataStorage = new QtPrivate::QIOOperationDataStorage();

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->type = QIOOperation::Type::Open;

    auto *op = new QIOOperation(*priv, m_owner);

    m_operations.append(op);
    executeNextOperation();
    return op;
}

void QRandomAccessAsyncFileThreadPoolBackend::close()
{
    // all the operations should be aborted
    for (const auto &op : std::as_const(m_operations)) {
        if (op) {
            auto *priv = QIOOperationPrivate::get(op.get());
            priv->setError(QIOOperation::Error::Aborted);
        }
    }
    m_operations.clear();

    // Wait until the current operation is complete
    if (m_currentOperation) {
        auto *priv = QIOOperationPrivate::get(m_currentOperation.get());
        priv->setError(QIOOperation::Error::Aborted);
        cancelAndWait(m_currentOperation.get());
    }

    {
        QMutexLocker locker(&m_engineMutex);
        if (m_engine) {
            m_engine->close();
            m_engine.reset();
        }
    }

    m_fileState = FileState::Closed;
}

qint64 QRandomAccessAsyncFileThreadPoolBackend::size() const
{
    QMutexLocker locker(&m_engineMutex);
    if (m_engine)
        return m_engine->size();
    return -1;
}

QIOOperation *QRandomAccessAsyncFileThreadPoolBackend::flush()
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage();

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->type = QIOOperation::Type::Flush;

    auto *op = new QIOOperation(*priv, m_owner);
    m_operations.append(op);
    executeNextOperation();
    return op;
}

QIOReadOperation *QRandomAccessAsyncFileThreadPoolBackend::read(qint64 offset, qint64 maxSize)
{
    QByteArray array;
    array.resizeForOverwrite(maxSize);
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(std::move(array));

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Read;

    auto *op = new QIOReadOperation(*priv, m_owner);
    m_operations.append(op);
    executeNextOperation();
    return op;
}

QIOWriteOperation *
QRandomAccessAsyncFileThreadPoolBackend::write(qint64 offset, const QByteArray &data)
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(data);

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Write;

    auto *op = new QIOWriteOperation(*priv, m_owner);
    m_operations.append(op);
    executeNextOperation();
    return op;
}

QIOWriteOperation *
QRandomAccessAsyncFileThreadPoolBackend::write(qint64 offset, QByteArray &&data)
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(std::move(data));

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Write;

    auto *op = new QIOWriteOperation(*priv, m_owner);
    m_operations.append(op);
    executeNextOperation();
    return op;
}

QIOVectoredReadOperation *
QRandomAccessAsyncFileThreadPoolBackend::readInto(qint64 offset, QSpan<std::byte> buffer)
{
    auto *dataStorage =
            new QtPrivate::QIOOperationDataStorage(QSpan<const QSpan<std::byte>>{buffer});

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Read;

    auto *op = new QIOVectoredReadOperation(*priv, m_owner);
    m_operations.append(op);
    executeNextOperation();
    return op;
}

QIOVectoredWriteOperation *
QRandomAccessAsyncFileThreadPoolBackend::writeFrom(qint64 offset, QSpan<const std::byte> buffer)
{
    auto *dataStorage =
            new QtPrivate::QIOOperationDataStorage(QSpan<const QSpan<const std::byte>>{buffer});

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Write;

    auto *op = new QIOVectoredWriteOperation(*priv, m_owner);
    m_operations.append(op);
    executeNextOperation();
    return op;
}

QIOVectoredReadOperation *
QRandomAccessAsyncFileThreadPoolBackend::readInto(qint64 offset, QSpan<const QSpan<std::byte>> buffers)
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(buffers);

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Read;

    auto *op = new QIOVectoredReadOperation(*priv, m_owner);
    m_operations.append(op);
    executeNextOperation();
    return op;
}

QIOVectoredWriteOperation *
QRandomAccessAsyncFileThreadPoolBackend::writeFrom(qint64 offset, QSpan<const QSpan<const std::byte>> buffers)
{
    auto *dataStorage = new QtPrivate::QIOOperationDataStorage(buffers);

    auto *priv = new QIOOperationPrivate(dataStorage);
    priv->offset = offset;
    priv->type = QIOOperation::Type::Write;

    auto *op = new QIOVectoredWriteOperation(*priv, m_owner);
    m_operations.append(op);
    executeNextOperation();
    return op;
}

static QRandomAccessAsyncFileThreadPoolBackend::OperationResult
executeRead(QFSFileEngine *engine, QBasicMutex *mutex, qint64 offset, char *buffer, qint64 maxSize)
{
    QRandomAccessAsyncFileThreadPoolBackend::OperationResult result{0, QIOOperation::Error::None};

    QMutexLocker locker(mutex);
    if (engine) {
        if (engine->seek(offset)) {
            qint64 bytesRead = engine->read(buffer, maxSize);
            if (bytesRead >= 0)
                result.bytesProcessed = bytesRead;
            else
                result.error = QIOOperation::Error::Read;
        } else {
            result.error = QIOOperation::Error::IncorrectOffset;
        }
    } else {
        result.error = QIOOperation::Error::FileNotOpen;
    }
    return result;
}

static QRandomAccessAsyncFileThreadPoolBackend::OperationResult
executeWrite(QFSFileEngine *engine, QBasicMutex *mutex, qint64 offset,
             const char *buffer, qint64 size)
{
    QRandomAccessAsyncFileThreadPoolBackend::OperationResult result{0, QIOOperation::Error::None};

    QMutexLocker locker(mutex);
    if (engine) {
        if (engine->seek(offset)) {
            qint64 written = engine->write(buffer, size);
            if (written >= 0)
                result.bytesProcessed = written;
            else
                result.error = QIOOperation::Error::Write;
        } else {
            result.error = QIOOperation::Error::IncorrectOffset;
        }
    } else {
        result.error = QIOOperation::Error::FileNotOpen;
    }
    return result;
}

void QRandomAccessAsyncFileThreadPoolBackend::executeNextOperation()
{
    if (m_currentOperation.isNull()) {
        // start next
        if (!m_operations.isEmpty()) {
            m_currentOperation = m_operations.takeFirst();
            switch (m_currentOperation->type()) {
            case QIOOperation::Type::Read:
            case QIOOperation::Type::Write:
                numProcessedBuffers = 0;
                processBufferAt(numProcessedBuffers);
                break;
            case QIOOperation::Type::Flush:
                processFlush();
                break;
            case QIOOperation::Type::Open:
                processOpen();
                break;
            case QIOOperation::Type::Unknown:
                Q_ASSERT_X(false, "executeNextOperation", "Operation of type Unknown!");
                // For release builds - directly complete the operation
                m_watcher.setFuture(QtFuture::makeReadyValueFuture(OperationResult{}));
                operationComplete();
                break;
            }
        }
    }
}

void QRandomAccessAsyncFileThreadPoolBackend::processBufferAt(qsizetype idx)
{
    Q_ASSERT(!m_currentOperation.isNull());
    auto *priv = QIOOperationPrivate::get(m_currentOperation.get());
    auto &dataStorage = priv->dataStorage;
    // if we do not use span buffers, we have only one buffer
    Q_ASSERT(dataStorage->containsReadSpans()
             || dataStorage->containsWriteSpans()
             || idx == 0);
    if (priv->type == QIOOperation::Type::Read) {
        qint64 maxSize = -1;
        char *buf = nullptr;
        if (dataStorage->containsReadSpans()) {
            auto &readBuffers = dataStorage->getReadSpans();
            Q_ASSERT(readBuffers.size() > idx);
            maxSize = readBuffers[idx].size_bytes();
            buf = reinterpret_cast<char *>(readBuffers[idx].data());
        } else {
            Q_ASSERT(dataStorage->containsByteArray());
            auto &array = dataStorage->getByteArray();
            maxSize = array.size();
            buf = array.data();
        }
        Q_ASSERT(maxSize >= 0);

        qint64 offset = priv->offset;
        if (idx != 0)
            offset += priv->processed;
        QBasicMutex *mutexPtr = &m_engineMutex;
        auto op = [engine = m_engine.get(), buf, maxSize, offset, mutexPtr] {
            return executeRead(engine, mutexPtr, offset, buf, maxSize);
        };

        QFuture<OperationResult> f =
                QtFuture::makeReadyVoidFuture().then(asyncFileThreadPool(), op);
        m_watcher.setFuture(f);
    } else if (priv->type == QIOOperation::Type::Write) {
        qint64 size = -1;
        const char *buf = nullptr;
        if (dataStorage->containsWriteSpans()) {
            const auto &writeBuffers = dataStorage->getWriteSpans();
            Q_ASSERT(writeBuffers.size() > idx);
            size = writeBuffers[idx].size_bytes();
            buf = reinterpret_cast<const char *>(writeBuffers[idx].data());
        } else {
            Q_ASSERT(dataStorage->containsByteArray());
            const auto &array = dataStorage->getByteArray();
            size = array.size();
            buf = array.constData();
        }
        Q_ASSERT(size >= 0);

        qint64 offset = priv->offset;
        if (idx != 0)
            offset += priv->processed;
        QBasicMutex *mutexPtr = &m_engineMutex;
        auto op = [engine = m_engine.get(), buf, size, offset, mutexPtr] {
            return executeWrite(engine, mutexPtr, offset, buf, size);
        };

        QFuture<OperationResult> f =
                QtFuture::makeReadyVoidFuture().then(asyncFileThreadPool(), op);
        m_watcher.setFuture(f);
    }
}

void QRandomAccessAsyncFileThreadPoolBackend::processFlush()
{
    Q_ASSERT(!m_currentOperation.isNull());
    auto *priv = QIOOperationPrivate::get(m_currentOperation.get());
    auto &dataStorage = priv->dataStorage;
    Q_ASSERT(dataStorage->isEmpty());

    QBasicMutex *mutexPtr = &m_engineMutex;
    auto op = [engine = m_engine.get(), mutexPtr] {
        QMutexLocker locker(mutexPtr);
        QRandomAccessAsyncFileThreadPoolBackend::OperationResult result{0, QIOOperation::Error::None};
        if (engine) {
            if (!engine->flush())
                result.error = QIOOperation::Error::Flush;
        } else {
            result.error = QIOOperation::Error::FileNotOpen;
        }
        return result;
    };

    QFuture<OperationResult> f =
            QtFuture::makeReadyVoidFuture().then(asyncFileThreadPool(), op);
    m_watcher.setFuture(f);
}

void QRandomAccessAsyncFileThreadPoolBackend::processOpen()
{
    Q_ASSERT(!m_currentOperation.isNull());
    auto *priv = QIOOperationPrivate::get(m_currentOperation.get());
    auto &dataStorage = priv->dataStorage;
    Q_ASSERT(dataStorage->isEmpty());

    QFuture<OperationResult> f;
    if (m_fileState == FileState::OpenPending) {
        // create the engine
        m_engineMutex.lock();
        m_engine = std::make_unique<QFSFileEngine>(m_filePath);
        m_engineMutex.unlock();
        QBasicMutex *mutexPtr = &m_engineMutex;
        auto op = [engine = m_engine.get(), mutexPtr, mode = m_openMode] {
            QRandomAccessAsyncFileThreadPoolBackend::OperationResult result{0, QIOOperation::Error::None};
            QMutexLocker locker(mutexPtr);
            const bool res =
                    engine && engine->open(mode | QIODeviceBase::Unbuffered, std::nullopt);
            if (!res)
                result.error = QIOOperation::Error::Open;
            return result;
        };
        f = QtFuture::makeReadyVoidFuture().then(asyncFileThreadPool(), op);
    } else {
        f = QtFuture::makeReadyVoidFuture().then(asyncFileThreadPool(), [] {
            return QRandomAccessAsyncFileThreadPoolBackend::OperationResult{0, QIOOperation::Error::Open};
        });
    }
    m_watcher.setFuture(f);
}

void QRandomAccessAsyncFileThreadPoolBackend::operationComplete()
{
    // TODO: if one of the buffers was read/written with an error,
    // stop processing immediately

    auto scheduleNextOperation = qScopeGuard([this]{
        m_currentOperation = nullptr;
        executeNextOperation();
    });

    if (m_currentOperation && !m_watcher.isCanceled()) {
        OperationResult res = m_watcher.future().result();
        auto *priv = QIOOperationPrivate::get(m_currentOperation.get());
        auto &dataStorage = priv->dataStorage;

        switch (priv->type) {
        case QIOOperation::Type::Read: {
            qsizetype expectedBuffersCount = 1;
            if (dataStorage->containsReadSpans()) {
                auto &readBuffers = dataStorage->getReadSpans();
                expectedBuffersCount = readBuffers.size();
                Q_ASSERT(numProcessedBuffers < expectedBuffersCount);
                const qsizetype unusedBytes =
                        readBuffers[numProcessedBuffers].size_bytes() - res.bytesProcessed;
                readBuffers[numProcessedBuffers].chop(unusedBytes);
            } else {
                Q_ASSERT(dataStorage->containsByteArray());
                Q_ASSERT(numProcessedBuffers == 0);
                auto &array = dataStorage->getByteArray();
                array.resize(res.bytesProcessed);
            }
            priv->appendBytesProcessed(res.bytesProcessed);
            if (++numProcessedBuffers < expectedBuffersCount) {
                // keep executing this command
                processBufferAt(numProcessedBuffers);
                scheduleNextOperation.dismiss();
            } else {
                priv->operationComplete(res.error);
            }
            break;
        }
        case QIOOperation::Type::Write: {
            qsizetype expectedBuffersCount = 1;
            if (dataStorage->containsWriteSpans())
                expectedBuffersCount = dataStorage->getWriteSpans().size();
            Q_ASSERT(numProcessedBuffers < expectedBuffersCount);
            priv->appendBytesProcessed(res.bytesProcessed);
            if (++numProcessedBuffers < expectedBuffersCount) {
                // keep executing this command
                processBufferAt(numProcessedBuffers);
                scheduleNextOperation.dismiss();
            } else {
                priv->operationComplete(res.error);
            }
            break;
        }
        case QIOOperation::Type::Flush:
            priv->operationComplete(res.error);
            break;
        case QIOOperation::Type::Open:
            if (m_fileState == FileState::OpenPending) {
                if (res.error == QIOOperation::Error::None) {
                    m_fileState = FileState::Opened;
                } else {
                    m_fileState = FileState::Closed;
                    m_engineMutex.lock();
                    m_engine.reset();
                    m_engineMutex.unlock();
                }
            }
            priv->operationComplete(res.error);
            break;
        case QIOOperation::Type::Unknown:
            priv->setError(QIOOperation::Error::Aborted);
            break;
        }
    }
}

QT_END_NAMESPACE
