// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qrandomaccessasyncfile_p.h"
#include "qrandomaccessasyncfile_p_p.h"

QT_BEGIN_NAMESPACE

QRandomAccessAsyncFileBackend::QRandomAccessAsyncFileBackend(QRandomAccessAsyncFile *owner)
    : m_owner(owner)
{
}
QRandomAccessAsyncFileBackend::~QRandomAccessAsyncFileBackend() = default;
QRandomAccessAsyncFilePrivate::QRandomAccessAsyncFilePrivate() = default;
QRandomAccessAsyncFilePrivate::~QRandomAccessAsyncFilePrivate() = default;

void QRandomAccessAsyncFilePrivate::init()
{
    Q_Q(QRandomAccessAsyncFile);

#if defined(QT_RANDOMACCESSASYNCFILE_QIORING) || defined(Q_OS_DARWIN)
    m_backend = std::make_unique<QRandomAccessAsyncFileNativeBackend>(q);
#endif
    if (!m_backend || !m_backend->init()) {
#if QT_CONFIG(thread) && QT_CONFIG(future)
        m_backend = std::make_unique<QRandomAccessAsyncFileThreadPoolBackend>(q);
        [[maybe_unused]]
        bool result = m_backend->init();
        Q_ASSERT(result); // it always succeeds
#endif
    }
}

QRandomAccessAsyncFile::QRandomAccessAsyncFile(QObject *parent)
    : QObject{*new QRandomAccessAsyncFilePrivate, parent}
{
    d_func()->init();
}

QRandomAccessAsyncFile::~QRandomAccessAsyncFile()
{
    close();
}

void QRandomAccessAsyncFile::close()
{
    Q_D(QRandomAccessAsyncFile);
    d->close();
}

qint64 QRandomAccessAsyncFile::size() const
{
    Q_D(const QRandomAccessAsyncFile);
    return d->size();
}

/*!
    \internal

    Attempts to open the file \a filePath with mode \a mode.

    \include qrandomaccessasyncfile.cpp returns-qiooperation
*/
QIOOperation *QRandomAccessAsyncFile::open(const QString &filePath, QIODeviceBase::OpenMode mode)
{
    Q_D(QRandomAccessAsyncFile);
    return d->open(filePath, mode);
}

/*!
    \internal

    Flushes any buffered data to the file.

    \include qrandomaccessasyncfile.cpp returns-qiooperation
*/
QIOOperation *QRandomAccessAsyncFile::flush()
{
    Q_D(QRandomAccessAsyncFile);
    return d->flush();
}

/*!
    \internal

    Reads at maximum \a maxSize bytes, starting from \a offset.

    The data is written to the internal buffer managed by the returned
    QIOOperation object.

//! [returns-qiooperation]
    Returns a QIOOperation object that would emit a QIOOperation::finished()
    signal once the operation is complete.
//! [returns-qiooperation]
*/
QIOReadOperation *QRandomAccessAsyncFile::read(qint64 offset, qint64 maxSize)
{
    Q_D(QRandomAccessAsyncFile);
    if (maxSize < 0) {
        qWarning("Using a negative maxSize in QRandomAccessAsyncFile::read() is incorrect. "
                 "Resetting to zero!");
        maxSize = 0;
    }
    return d->read(offset, maxSize);
}

/*!
    \internal
    \overload

    Reads the data from the file, starting from \a offset, and stores it into
    \a buffer.

    The amount of bytes to be read from the file is determined by the size of
    the buffer. Note that the actual amount of read bytes can be less than that.

    This operation does not take ownership of the provided buffer, so it is the
    user's responsibility to make sure that the buffer is valid until the
    returned QIOOperation completes.

    \note The buffer might be populated from different threads, so the user
    application should not access it until the returned QIOOperation completes.

    \include qrandomaccessasyncfile.cpp returns-qiooperation
*/
QIOVectoredReadOperation *
QRandomAccessAsyncFile::readInto(qint64 offset, QSpan<std::byte> buffer)
{
    Q_D(QRandomAccessAsyncFile);
    return d->readInto(offset, buffer);
}

/*!
    \internal

    Reads the data from the file, starting from \a offset, and stores it into
    \a buffers.

    The amount of bytes to be read from the file is determined by the sum of
    sizes of all buffers. Note that the actual amount of read bytes can be less
    than that.

    This operation does not take ownership of the provided buffers, so it is the
    user's responsibility to make sure that the buffers are valid until the
    returned QIOOperation completes.

    \note The buffers might be populated from different threads, so the user
    application should not access them until the returned QIOOperation completes.

    \include qrandomaccessasyncfile.cpp returns-qiooperation
*/
QIOVectoredReadOperation *
QRandomAccessAsyncFile::readInto(qint64 offset, QSpan<const QSpan<std::byte>> buffers)
{
    Q_D(QRandomAccessAsyncFile);
    return d->readInto(offset, buffers);
}

/*!
    \internal

    Writes \a data into the file, starting from \a offset.

    The \a data array is copied into the returned QIOOperation object.

    \include qrandomaccessasyncfile.cpp returns-qiooperation
*/
QIOWriteOperation *QRandomAccessAsyncFile::write(qint64 offset, const QByteArray &data)
{
    Q_D(QRandomAccessAsyncFile);
    return d->write(offset, data);
}

/*!
    \internal
    \overload

    Writes \a data into the file, starting from \a offset.

    The \a data array is moved into the returned QIOOperation object.

    \include qrandomaccessasyncfile.cpp returns-qiooperation
*/
QIOWriteOperation *QRandomAccessAsyncFile::write(qint64 offset, QByteArray &&data)
{
    Q_D(QRandomAccessAsyncFile);
    return d->write(offset, std::move(data));
}

/*!
    \internal
    \overload

    Writes the content of \a buffer into the file, starting from \a offset.

    This operation does not take ownership of the provided buffer, so it is the
    user's responsibility to make sure that the buffer is valid until the
    returned QIOOperation completes.

    \note The buffer might be accessed from different threads, so the user
    application should not modify it until the returned QIOOperation completes.
*/
QIOVectoredWriteOperation *
QRandomAccessAsyncFile::writeFrom(qint64 offset, QSpan<const std::byte> buffer)
{
    Q_D(QRandomAccessAsyncFile);
    return d->writeFrom(offset, buffer);
}

/*!
    \internal

    Writes the content of \a buffers into the file, starting from \a offset.

    This operation does not take ownership of the provided buffers, so it is the
    user's responsibility to make sure that the buffers are valid until the
    returned QIOOperation completes.

    \note The buffers might be accessed from different threads, so the user
    application should not modify them until the returned QIOOperation
    completes.
*/
QIOVectoredWriteOperation *
QRandomAccessAsyncFile::writeFrom(qint64 offset, QSpan<const QSpan<const std::byte>> buffers)
{
    Q_D(QRandomAccessAsyncFile);
    return d->writeFrom(offset, buffers);
}

QT_END_NAMESPACE

#include "moc_qrandomaccessasyncfile_p.cpp"
