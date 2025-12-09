// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QIOOPERATION_P_H
#define QIOOPERATION_P_H

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

#include <QtCore/private/qglobal_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qspan.h>

QT_BEGIN_NAMESPACE

class QIOOperationPrivate;
class Q_CORE_EXPORT QIOOperation : public QObject
{
    Q_OBJECT
public:
    // TODO: more specific error codes?
    enum class Error
    {
        None,
        FileNotOpen,
        IncorrectOffset,
        Read,
        Write,
        Flush,
        Open,
        Aborted,
    };
    Q_ENUM(Error)

    enum class Type : quint8
    {
        Unknown,
        Read,
        Write,
        Flush,
        Open,
    };
    Q_ENUM(Type)

    ~QIOOperation() override;

    Type type() const;
    Error error() const;
    bool isFinished() const;

Q_SIGNALS:
    void finished();
    void errorOccurred(Error err);

protected:
    QIOOperation() = delete;
    Q_DISABLE_COPY_MOVE(QIOOperation)
    explicit QIOOperation(QIOOperationPrivate &dd, QObject *parent = nullptr);

    void ensureCompleteOrCanceled();

    Q_DECLARE_PRIVATE(QIOOperation)

    friend class QRandomAccessAsyncFilePrivate;
    friend class QRandomAccessAsyncFileBackend;
    friend class QRandomAccessAsyncFileNativeBackend;
    friend class QRandomAccessAsyncFileThreadPoolBackend;
};

class Q_CORE_EXPORT QIOReadWriteOperationBase : public QIOOperation
{
public:
    ~QIOReadWriteOperationBase() override;

    qint64 offset() const;
    qint64 numBytesProcessed() const;

protected:
    QIOReadWriteOperationBase() = delete;
    Q_DISABLE_COPY_MOVE(QIOReadWriteOperationBase)
    explicit QIOReadWriteOperationBase(QIOOperationPrivate &dd, QObject *parent = nullptr);

    friend class QRandomAccessAsyncFilePrivate;
    friend class QRandomAccessAsyncFileBackend;
    friend class QRandomAccessAsyncFileNativeBackend;
    friend class QRandomAccessAsyncFileThreadPoolBackend;
};

class Q_CORE_EXPORT QIOReadOperation : public QIOReadWriteOperationBase
{
public:
    ~QIOReadOperation() override;

    QByteArray data() const;

protected:
    QIOReadOperation() = delete;
    Q_DISABLE_COPY_MOVE(QIOReadOperation)
    explicit QIOReadOperation(QIOOperationPrivate &dd, QObject *parent = nullptr);

    friend class QRandomAccessAsyncFilePrivate;
    friend class QRandomAccessAsyncFileBackend;
    friend class QRandomAccessAsyncFileNativeBackend;
    friend class QRandomAccessAsyncFileThreadPoolBackend;
};

class Q_CORE_EXPORT QIOWriteOperation : public QIOReadWriteOperationBase
{
public:
    ~QIOWriteOperation() override;

    QByteArray data() const;

protected:
    QIOWriteOperation() = delete;
    Q_DISABLE_COPY_MOVE(QIOWriteOperation)
    explicit QIOWriteOperation(QIOOperationPrivate &dd, QObject *parent = nullptr);

    friend class QRandomAccessAsyncFilePrivate;
    friend class QRandomAccessAsyncFileBackend;
    friend class QRandomAccessAsyncFileNativeBackend;
    friend class QRandomAccessAsyncFileThreadPoolBackend;
};

class Q_CORE_EXPORT QIOVectoredReadOperation : public QIOReadWriteOperationBase
{
public:
    ~QIOVectoredReadOperation() override;

    QSpan<const QSpan<std::byte>> data() const;

protected:
    QIOVectoredReadOperation() = delete;
    Q_DISABLE_COPY_MOVE(QIOVectoredReadOperation)
    explicit QIOVectoredReadOperation(QIOOperationPrivate &dd, QObject *parent = nullptr);

    friend class QRandomAccessAsyncFilePrivate;
    friend class QRandomAccessAsyncFileBackend;
    friend class QRandomAccessAsyncFileNativeBackend;
    friend class QRandomAccessAsyncFileThreadPoolBackend;
};

class Q_CORE_EXPORT QIOVectoredWriteOperation : public QIOReadWriteOperationBase
{
public:
    ~QIOVectoredWriteOperation() override;

    QSpan<const QSpan<const std::byte>> data() const;

protected:
    QIOVectoredWriteOperation() = delete;
    Q_DISABLE_COPY_MOVE(QIOVectoredWriteOperation)
    explicit QIOVectoredWriteOperation(QIOOperationPrivate &dd, QObject *parent = nullptr);

    friend class QRandomAccessAsyncFilePrivate;
    friend class QRandomAccessAsyncFileBackend;
    friend class QRandomAccessAsyncFileNativeBackend;
    friend class QRandomAccessAsyncFileThreadPoolBackend;
};

QT_END_NAMESPACE

#endif // QIOOPERATION_P_H
