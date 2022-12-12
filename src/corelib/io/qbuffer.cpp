// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbuffer.h"
#include <QtCore/qmetaobject.h>
#include "private/qiodevice_p.h"

#include <limits>

QT_BEGIN_NAMESPACE

/** QBufferPrivate **/
class QBufferPrivate : public QIODevicePrivate
{
    Q_DECLARE_PUBLIC(QBuffer)

public:
    QBufferPrivate() = default;

    QByteArray *buf = nullptr;
    QByteArray defaultBuf;

    qint64 peek(char *data, qint64 maxSize) override;
    QByteArray peek(qint64 maxSize) override;

#ifndef QT_NO_QOBJECT
    // private slots
    void _q_emitSignals();

    qint64 writtenSinceLastEmit = 0;
    int signalConnectionCount = 0;
    bool signalsEmitted = false;
#endif
};

#ifndef QT_NO_QOBJECT
void QBufferPrivate::_q_emitSignals()
{
    Q_Q(QBuffer);
    emit q->bytesWritten(writtenSinceLastEmit);
    writtenSinceLastEmit = 0;
    emit q->readyRead();
    signalsEmitted = false;
}
#endif

qint64 QBufferPrivate::peek(char *data, qint64 maxSize)
{
    qint64 readBytes = qMin(maxSize, static_cast<qint64>(buf->size()) - pos);
    memcpy(data, buf->constData() + pos, readBytes);
    return readBytes;
}

QByteArray QBufferPrivate::peek(qint64 maxSize)
{
    qint64 readBytes = qMin(maxSize, static_cast<qint64>(buf->size()) - pos);
    if (pos == 0 && maxSize >= buf->size())
        return *buf;
    return QByteArray(buf->constData() + pos, readBytes);
}

/*!
    \class QBuffer
    \inmodule QtCore
    \reentrant
    \brief The QBuffer class provides a QIODevice interface for a QByteArray.

    \ingroup io

    QBuffer allows you to access a QByteArray using the QIODevice
    interface. The QByteArray is treated just as a standard random-accessed
    file. Example:

    \snippet buffer/buffer.cpp 0

    By default, an internal QByteArray buffer is created for you when
    you create a QBuffer. You can access this buffer directly by
    calling buffer(). You can also use QBuffer with an existing
    QByteArray by calling setBuffer(), or by passing your array to
    QBuffer's constructor.

    Call open() to open the buffer. Then call write() or
    putChar() to write to the buffer, and read(), readLine(),
    readAll(), or getChar() to read from it. size() returns the
    current size of the buffer, and you can seek to arbitrary
    positions in the buffer by calling seek(). When you are done with
    accessing the buffer, call close().

    The following code snippet shows how to write data to a
    QByteArray using QDataStream and QBuffer:

    \snippet buffer/buffer.cpp 1

    Effectively, we convert the application's QPalette into a byte
    array. Here's how to read the data from the QByteArray:

    \snippet buffer/buffer.cpp 2

    QTextStream and QDataStream also provide convenience constructors
    that take a QByteArray and that create a QBuffer behind the
    scenes.

    QBuffer emits readyRead() when new data has arrived in the
    buffer. By connecting to this signal, you can use QBuffer to
    store temporary data before processing it. QBuffer also emits
    bytesWritten() every time new data has been written to the buffer.

    \sa QFile, QDataStream, QTextStream, QByteArray
*/

#ifdef QT_NO_QOBJECT
QBuffer::QBuffer()
    : QIODevice(*new QBufferPrivate)
{
    Q_D(QBuffer);
    d->buf = &d->defaultBuf;
}
QBuffer::QBuffer(QByteArray *buf)
    : QIODevice(*new QBufferPrivate)
{
    Q_D(QBuffer);
    d->buf = buf ? buf : &d->defaultBuf;
    d->defaultBuf.clear();
}
#else
/*!
    Constructs an empty buffer with the given \a parent. You can call
    setData() to fill the buffer with data, or you can open it in
    write mode and use write().

    \sa open()
*/
QBuffer::QBuffer(QObject *parent)
    : QIODevice(*new QBufferPrivate, parent)
{
    Q_D(QBuffer);
    d->buf = &d->defaultBuf;
}

/*!
    Constructs a QBuffer that uses the QByteArray pointed to by \a
    byteArray as its internal buffer, and with the given \a parent.
    The caller is responsible for ensuring that \a byteArray remains
    valid until the QBuffer is destroyed, or until setBuffer() is
    called to change the buffer. QBuffer doesn't take ownership of
    the QByteArray.

    If you open the buffer in write-only mode or read-write mode and
    write something into the QBuffer, \a byteArray will be modified.

    Example:

    \snippet buffer/buffer.cpp 3

    \sa open(), setBuffer(), setData()
*/
QBuffer::QBuffer(QByteArray *byteArray, QObject *parent)
    : QIODevice(*new QBufferPrivate, parent)
{
    Q_D(QBuffer);
    d->buf = byteArray ? byteArray : &d->defaultBuf;
    d->defaultBuf.clear();
}
#endif

/*!
    Destroys the buffer.
*/

QBuffer::~QBuffer()
{
}

/*!
    Makes QBuffer use the QByteArray pointed to by \a
    byteArray as its internal buffer. The caller is responsible for
    ensuring that \a byteArray remains valid until the QBuffer is
    destroyed, or until setBuffer() is called to change the buffer.
    QBuffer doesn't take ownership of the QByteArray.

    Does nothing if isOpen() is true.

    If you open the buffer in write-only mode or read-write mode and
    write something into the QBuffer, \a byteArray will be modified.

    Example:

    \snippet buffer/buffer.cpp 4

    If \a byteArray is \nullptr, the buffer creates its own internal
    QByteArray to work on. This byte array is initially empty.

    \sa buffer(), setData(), open()
*/

void QBuffer::setBuffer(QByteArray *byteArray)
{
    Q_D(QBuffer);
    if (isOpen()) {
        qWarning("QBuffer::setBuffer: Buffer is open");
        return;
    }
    if (byteArray) {
        d->buf = byteArray;
    } else {
        d->buf = &d->defaultBuf;
    }
    d->defaultBuf.clear();
}

/*!
    Returns a reference to the QBuffer's internal buffer. You can use
    it to modify the QByteArray behind the QBuffer's back.

    \sa setBuffer(), data()
*/

QByteArray &QBuffer::buffer()
{
    Q_D(QBuffer);
    return *d->buf;
}

/*!
    \overload

    This is the same as data().
*/

const QByteArray &QBuffer::buffer() const
{
    Q_D(const QBuffer);
    return *d->buf;
}


/*!
    Returns the data contained in the buffer.

    This is the same as buffer().

    \sa setData(), setBuffer()
*/

const QByteArray &QBuffer::data() const
{
    Q_D(const QBuffer);
    return *d->buf;
}

/*!
    Sets the contents of the internal buffer to be \a data. This is
    the same as assigning \a data to buffer().

    Does nothing if isOpen() is true.

    \sa setBuffer()
*/
void QBuffer::setData(const QByteArray &data)
{
    Q_D(QBuffer);
    if (isOpen()) {
        qWarning("QBuffer::setData: Buffer is open");
        return;
    }
    *d->buf = data;
}

/*!
    \overload

    Sets the contents of the internal buffer to be the first \a size
    bytes of \a data.

    \note In Qt versions prior to 6.5, this function took the length as
    an \c{int} parameter, potentially truncating sizes.
*/
void QBuffer::setData(const char *data, qsizetype size)
{
    Q_D(QBuffer);
    if (isOpen()) {
        qWarning("QBuffer::setData: Buffer is open");
        return;
    }
    d->buf->replace(qsizetype(0), d->buf->size(), // ### QByteArray lacks assign(ptr, n)
                    data, size);
}

/*!
   \reimp

   Unlike QFile, opening a QBuffer QIODevice::WriteOnly does not truncate it.
   However, pos() is set to 0. Use QIODevice::Append or QIODevice::Truncate to
   change either behavior.
*/
bool QBuffer::open(OpenMode flags)
{
    Q_D(QBuffer);

    if ((flags & (Append | Truncate)) != 0)
        flags |= WriteOnly;
    if ((flags & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QBuffer::open: Buffer access not specified");
        return false;
    }

    if ((flags & Truncate) == Truncate)
        d->buf->resize(0);

    return QIODevice::open(flags | QIODevice::Unbuffered);
}

/*!
    \reimp
*/
void QBuffer::close()
{
    QIODevice::close();
}

/*!
    \reimp
*/
qint64 QBuffer::pos() const
{
    return QIODevice::pos();
}

/*!
    \reimp
*/
qint64 QBuffer::size() const
{
    Q_D(const QBuffer);
    return qint64(d->buf->size());
}

/*!
    \reimp
*/
bool QBuffer::seek(qint64 pos)
{
    Q_D(QBuffer);
    const auto oldBufSize = d->buf->size();
    constexpr qint64 MaxSeekPos = (std::numeric_limits<decltype(oldBufSize)>::max)();
    if (pos <= MaxSeekPos && pos > oldBufSize && isWritable()) {
        QT_TRY {
            d->buf->resize(qsizetype(pos), '\0');
        } QT_CATCH(const std::bad_alloc &) {} // swallow, failure case is handled below
        if (d->buf->size() != pos) {
            qWarning("QBuffer::seek: Unable to fill gap");
            return false;
        }
    }
    if (pos > d->buf->size() || pos < 0) {
        qWarning("QBuffer::seek: Invalid pos: %lld", pos);
        return false;
    }
    return QIODevice::seek(pos);
}

/*!
    \reimp
*/
bool QBuffer::atEnd() const
{
    return QIODevice::atEnd();
}

/*!
   \reimp
*/
bool QBuffer::canReadLine() const
{
    Q_D(const QBuffer);
    if (!isOpen())
        return false;

    return d->buf->indexOf('\n', int(pos())) != -1 || QIODevice::canReadLine();
}

/*!
    \reimp
*/
qint64 QBuffer::readData(char *data, qint64 len)
{
    Q_D(QBuffer);
    if ((len = qMin(len, qint64(d->buf->size()) - pos())) <= 0)
        return qint64(0);
    memcpy(data, d->buf->constData() + pos(), len);
    return len;
}

/*!
    \reimp
*/
qint64 QBuffer::writeData(const char *data, qint64 len)
{
    Q_D(QBuffer);
    const quint64 required = quint64(pos()) + quint64(len); // cannot overflow (pos() ≥ 0, len ≥ 0)

    if (required > quint64(d->buf->size())) { // capacity exceeded
        // The following must hold, since qsizetype covers half the virtual address space:
        Q_ASSUME(required <= quint64((std::numeric_limits<qsizetype>::max)()));
        d->buf->resize(qsizetype(required));
        if (quint64(d->buf->size()) != required) { // could not resize
            qWarning("QBuffer::writeData: Memory allocation error");
            return -1;
        }
    }

    memcpy(d->buf->data() + pos(), data, size_t(len));

#ifndef QT_NO_QOBJECT
    d->writtenSinceLastEmit += len;
    if (d->signalConnectionCount && !d->signalsEmitted && !signalsBlocked()) {
        d->signalsEmitted = true;
        QMetaObject::invokeMethod(this, "_q_emitSignals", Qt::QueuedConnection);
    }
#endif
    return len;
}

#ifndef QT_NO_QOBJECT
static bool is_tracked_signal(const QMetaMethod &signal)
{
    // dynamic initialization: minimize the number of guard variables:
    static const struct {
        QMetaMethod readyReadSignal = QMetaMethod::fromSignal(&QBuffer::readyRead);
        QMetaMethod bytesWrittenSignal = QMetaMethod::fromSignal(&QBuffer::bytesWritten);
    } sigs;
    return signal == sigs.readyReadSignal || signal == sigs.bytesWrittenSignal;
}
/*!
    \reimp
    \internal
*/
void QBuffer::connectNotify(const QMetaMethod &signal)
{
    if (is_tracked_signal(signal))
        d_func()->signalConnectionCount++;
}

/*!
    \reimp
    \internal
*/
void QBuffer::disconnectNotify(const QMetaMethod &signal)
{
    if (signal.isValid()) {
        if (is_tracked_signal(signal))
            d_func()->signalConnectionCount--;
    } else {
        d_func()->signalConnectionCount = 0;
    }
}
#endif

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
# include "moc_qbuffer.cpp"
#endif

