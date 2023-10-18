// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//#define QIODEVICE_DEBUG

#include "qbytearray.h"
#include "qdebug.h"
#include "qiodevice_p.h"
#include "qfile.h"
#include "qstringlist.h"
#include "qdir.h"
#include "private/qbytearray_p.h"
#include "private/qtools_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtMiscUtils;

[[maybe_unused]]
static void debugBinaryString(const char *input, qint64 maxlen)
{
    QByteArray tmp;
    qlonglong startOffset = 0;
    for (qint64 i = 0; i < maxlen; ++i) {
        tmp += input[i];

        if ((i % 16) == 15 || i == (maxlen - 1)) {
            printf("\n%15lld:", startOffset);
            startOffset += tmp.size();

            for (qsizetype j = 0; j < tmp.size(); ++j)
                printf(" %02x", int(uchar(tmp[j])));
            for (qsizetype j = tmp.size(); j < 16 + 1; ++j)
                printf("   ");
            for (qsizetype j = 0; j < tmp.size(); ++j)
                printf("%c", isAsciiPrintable(tmp[j]) ? tmp[j] : '.');
            tmp.clear();
        }
    }
    printf("\n\n");
}

#define Q_VOID

Q_DECL_COLD_FUNCTION
static void checkWarnMessage(const QIODevice *device, const char *function, const char *what)
{
#ifndef QT_NO_WARNING_OUTPUT
    QDebug d = qWarning();
    d.noquote();
    d.nospace();
    d << "QIODevice::" << function;
#ifndef QT_NO_QOBJECT
    d << " (" << device->metaObject()->className();
    if (!device->objectName().isEmpty())
        d << ", \"" << device->objectName() << '"';
    if (const QFile *f = qobject_cast<const QFile *>(device))
        d << ", \"" << QDir::toNativeSeparators(f->fileName()) << '"';
    d << ')';
#else
    Q_UNUSED(device);
#endif // !QT_NO_QOBJECT
    d << ": " << what;
#else
    Q_UNUSED(device);
    Q_UNUSED(function);
    Q_UNUSED(what);
#endif // QT_NO_WARNING_OUTPUT
}

#define CHECK_MAXLEN(function, returnType) \
    do { \
        if (maxSize < 0) { \
            checkWarnMessage(this, #function, "Called with maxSize < 0"); \
            return returnType; \
        } \
    } while (0)

#define CHECK_LINEMAXLEN(function, returnType) \
    do { \
        if (maxSize < 2) { \
            checkWarnMessage(this, #function, "Called with maxSize < 2"); \
            return returnType; \
        } \
    } while (0)

#define CHECK_MAXBYTEARRAYSIZE(function) \
    do { \
        if (maxSize >= MaxByteArraySize) { \
            checkWarnMessage(this, #function, "maxSize argument exceeds QByteArray size limit"); \
            maxSize = MaxByteArraySize - 1; \
        } \
    } while (0)

#define CHECK_WRITABLE(function, returnType) \
   do { \
       if ((d->openMode & WriteOnly) == 0) { \
           if (d->openMode == NotOpen) { \
               checkWarnMessage(this, #function, "device not open"); \
               return returnType; \
           } \
           checkWarnMessage(this, #function, "ReadOnly device"); \
           return returnType; \
       } \
   } while (0)

#define CHECK_READABLE(function, returnType) \
   do { \
       if ((d->openMode & ReadOnly) == 0) { \
           if (d->openMode == NotOpen) { \
               checkWarnMessage(this, #function, "device not open"); \
               return returnType; \
           } \
           checkWarnMessage(this, #function, "WriteOnly device"); \
           return returnType; \
       } \
   } while (0)

/*!
    \internal
 */
QIODevicePrivate::QIODevicePrivate()
{
}

/*!
    \internal
 */
QIODevicePrivate::~QIODevicePrivate()
{
}

/*!
    \class QIODevice
    \inmodule QtCore
    \reentrant

    \brief The QIODevice class is the base interface class of all I/O
    devices in Qt.

    \ingroup io

    QIODevice provides both a common implementation and an abstract
    interface for devices that support reading and writing of blocks
    of data, such as QFile, QBuffer and QTcpSocket. QIODevice is
    abstract and cannot be instantiated, but it is common to use the
    interface it defines to provide device-independent I/O features.
    For example, Qt's XML classes operate on a QIODevice pointer,
    allowing them to be used with various devices (such as files and
    buffers).

    Before accessing the device, open() must be called to set the
    correct OpenMode (such as ReadOnly or ReadWrite). You can then
    write to the device with write() or putChar(), and read by calling
    either read(), readLine(), or readAll(). Call close() when you are
    done with the device.

    QIODevice distinguishes between two types of devices:
    random-access devices and sequential devices.

    \list
    \li Random-access devices support seeking to arbitrary
    positions using seek(). The current position in the file is
    available by calling pos(). QFile and QBuffer are examples of
    random-access devices.

    \li Sequential devices don't support seeking to arbitrary
    positions. The data must be read in one pass. The functions
    pos() and size() don't work for sequential devices.
    QTcpSocket and QProcess are examples of sequential devices.
    \endlist

    You can use isSequential() to determine the type of device.

    QIODevice emits readyRead() when new data is available for
    reading; for example, if new data has arrived on the network or if
    additional data is appended to a file that you are reading
    from. You can call bytesAvailable() to determine the number of
    bytes that are currently available for reading. It's common to use
    bytesAvailable() together with the readyRead() signal when
    programming with asynchronous devices such as QTcpSocket, where
    fragments of data can arrive at arbitrary points in
    time. QIODevice emits the bytesWritten() signal every time a
    payload of data has been written to the device. Use bytesToWrite()
    to determine the current amount of data waiting to be written.

    Certain subclasses of QIODevice, such as QTcpSocket and QProcess,
    are asynchronous. This means that I/O functions such as write()
    or read() always return immediately, while communication with the
    device itself may happen when control goes back to the event loop.
    QIODevice provides functions that allow you to force these
    operations to be performed immediately, while blocking the
    calling thread and without entering the event loop. This allows
    QIODevice subclasses to be used without an event loop, or in
    a separate thread:

    \list
    \li waitForReadyRead() - This function suspends operation in the
    calling thread until new data is available for reading.

    \li waitForBytesWritten() - This function suspends operation in the
    calling thread until one payload of data has been written to the
    device.

    \li waitFor....() - Subclasses of QIODevice implement blocking
    functions for device-specific operations. For example, QProcess
    has a function called \l {QProcess::}{waitForStarted()} which suspends operation in
    the calling thread until the process has started.
    \endlist

    Calling these functions from the main, GUI thread, may cause your
    user interface to freeze. Example:

    \snippet code/src_corelib_io_qiodevice.cpp 0

    By subclassing QIODevice, you can provide the same interface to
    your own I/O devices. Subclasses of QIODevice are only required to
    implement the protected readData() and writeData() functions.
    QIODevice uses these functions to implement all its convenience
    functions, such as getChar(), readLine() and write(). QIODevice
    also handles access control for you, so you can safely assume that
    the device is opened in write mode if writeData() is called.

    Some subclasses, such as QFile and QTcpSocket, are implemented
    using a memory buffer for intermediate storing of data. This
    reduces the number of required device accessing calls, which are
    often very slow. Buffering makes functions like getChar() and
    putChar() fast, as they can operate on the memory buffer instead
    of directly on the device itself. Certain I/O operations, however,
    don't work well with a buffer. For example, if several users open
    the same device and read it character by character, they may end
    up reading the same data when they meant to read a separate chunk
    each. For this reason, QIODevice allows you to bypass any
    buffering by passing the Unbuffered flag to open(). When
    subclassing QIODevice, remember to bypass any buffer you may use
    when the device is open in Unbuffered mode.

    Usually, the incoming data stream from an asynchronous device is
    fragmented, and chunks of data can arrive at arbitrary points in time.
    To handle incomplete reads of data structures, use the transaction
    mechanism implemented by QIODevice. See startTransaction() and related
    functions for more details.

    Some sequential devices support communicating via multiple channels. These
    channels represent separate streams of data that have the property of
    independently sequenced delivery. Once the device is opened, you can
    determine the number of channels by calling the readChannelCount() and
    writeChannelCount() functions. To switch between channels, call
    setCurrentReadChannel() and setCurrentWriteChannel(), respectively.
    QIODevice also provides additional signals to handle asynchronous
    communication on a per-channel basis.

    \sa QBuffer, QFile, QTcpSocket
*/

/*!
    \class QIODeviceBase
    \inheaderfile QIODevice
    \inmodule QtCore
    \brief Base class for QIODevice that provides flags describing the mode in
           which a device is opened.
*/

/*!
    \enum QIODeviceBase::OpenModeFlag

    This enum is used with QIODevice::open() to describe the mode in which a
    device is opened. It is also returned by QIODevice::openMode().

    \value NotOpen   The device is not open.
    \value ReadOnly  The device is open for reading.
    \value WriteOnly The device is open for writing. Note that, for file-system
                     subclasses (e.g. QFile), this mode implies Truncate unless
                     combined with ReadOnly, Append or NewOnly.
    \value ReadWrite The device is open for reading and writing.
    \value Append    The device is opened in append mode so that all data is
                     written to the end of the file.
    \value Truncate  If possible, the device is truncated before it is opened.
                     All earlier contents of the device are lost.
    \value Text      When reading, the end-of-line terminators are
                     translated to '\\n'. When writing, the end-of-line
                     terminators are translated to the local encoding, for
                     example '\\r\\n' for Win32.
    \value Unbuffered Any buffer in the device is bypassed.
    \value NewOnly   Fail if the file to be opened already exists. Create and
                     open the file only if it does not exist. There is a
                     guarantee from the operating system that you are the only
                     one creating and opening the file. Note that this mode
                     implies WriteOnly, and combining it with ReadWrite is
                     allowed. This flag currently only affects QFile. Other
                     classes might use this flag in the future, but until then
                     using this flag with any classes other than QFile may
                     result in undefined behavior. (since Qt 5.11)
    \value ExistingOnly Fail if the file to be opened does not exist. This flag
                     must be specified alongside ReadOnly, WriteOnly, or
                     ReadWrite. Note that using this flag with ReadOnly alone
                     is redundant, as ReadOnly already fails when the file does
                     not exist. This flag currently only affects QFile. Other
                     classes might use this flag in the future, but until then
                     using this flag with any classes other than QFile may
                     result in undefined behavior. (since Qt 5.11)

    Certain flags, such as \c Unbuffered and \c Truncate, are
    meaningless when used with some subclasses. Some of these
    restrictions are implied by the type of device that is represented
    by a subclass. In other cases, the restriction may be due to the
    implementation, or may be imposed by the underlying platform; for
    example, QTcpSocket does not support \c Unbuffered mode, and
    limitations in the native API prevent QFile from supporting \c
    Unbuffered on Windows.
*/

/*!     \fn QIODevice::bytesWritten(qint64 bytes)

    This signal is emitted every time a payload of data has been
    written to the device's current write channel. The \a bytes argument is
    set to the number of bytes that were written in this payload.

    bytesWritten() is not emitted recursively; if you reenter the event loop
    or call waitForBytesWritten() inside a slot connected to the
    bytesWritten() signal, the signal will not be reemitted (although
    waitForBytesWritten() may still return true).

    \sa readyRead()
*/

/*!
    \fn QIODevice::channelBytesWritten(int channel, qint64 bytes)
    \since 5.7

    This signal is emitted every time a payload of data has been written to
    the device. The \a bytes argument is set to the number of bytes that were
    written in this payload, while \a channel is the channel they were written
    to. Unlike bytesWritten(), it is emitted regardless of the
    \l{currentWriteChannel()}{current write channel}.

    channelBytesWritten() can be emitted recursively - even for the same
    channel.

    \sa bytesWritten(), channelReadyRead()
*/

/*!
    \fn QIODevice::readyRead()

    This signal is emitted once every time new data is available for
    reading from the device's current read channel. It will only be emitted
    again once new data is available, such as when a new payload of network
    data has arrived on your network socket, or when a new block of data has
    been appended to your device.

    readyRead() is not emitted recursively; if you reenter the event loop or
    call waitForReadyRead() inside a slot connected to the readyRead() signal,
    the signal will not be reemitted (although waitForReadyRead() may still
    return true).

    Note for developers implementing classes derived from QIODevice:
    you should always emit readyRead() when new data has arrived (do not
    emit it only because there's data still to be read in your
    buffers). Do not emit readyRead() in other conditions.

    \sa bytesWritten()
*/

/*!
    \fn QIODevice::channelReadyRead(int channel)
    \since 5.7

    This signal is emitted when new data is available for reading from the
    device. The \a channel argument is set to the index of the read channel on
    which the data has arrived. Unlike readyRead(), it is emitted regardless of
    the \l{currentReadChannel()}{current read channel}.

    channelReadyRead() can be emitted recursively - even for the same channel.

    \sa readyRead(), channelBytesWritten()
*/

/*! \fn QIODevice::aboutToClose()

    This signal is emitted when the device is about to close. Connect
    this signal if you have operations that need to be performed
    before the device closes (e.g., if you have data in a separate
    buffer that needs to be written to the device).
*/

/*!
    \fn QIODevice::readChannelFinished()
    \since 4.4

    This signal is emitted when the input (reading) stream is closed
    in this device. It is emitted as soon as the closing is detected,
    which means that there might still be data available for reading
    with read().

    \sa atEnd(), read()
*/

#ifdef QT_NO_QOBJECT
QIODevice::QIODevice()
    : d_ptr(new QIODevicePrivate)
{
    d_ptr->q_ptr = this;
}

/*!
    \internal
*/
QIODevice::QIODevice(QIODevicePrivate &dd)
    : d_ptr(&dd)
{
    d_ptr->q_ptr = this;
}
#else

/*!
    Constructs a QIODevice object.
*/

QIODevice::QIODevice()
    : QObject(*new QIODevicePrivate, nullptr)
{
#if defined QIODEVICE_DEBUG
    QFile *file = qobject_cast<QFile *>(this);
    printf("%p QIODevice::QIODevice(\"%s\") %s\n", this, metaObject()->className(),
           qPrintable(file ? file->fileName() : QString()));
#endif
}

/*!
    Constructs a QIODevice object with the given \a parent.
*/

QIODevice::QIODevice(QObject *parent)
    : QObject(*new QIODevicePrivate, parent)
{
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::QIODevice(%p \"%s\")\n", this, parent, metaObject()->className());
#endif
}

/*!
    \internal
*/
QIODevice::QIODevice(QIODevicePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}
#endif


/*!
  The destructor is virtual, and QIODevice is an abstract base
  class. This destructor does not call close(), but the subclass
  destructor might. If you are in doubt, call close() before
  destroying the QIODevice.
*/
QIODevice::~QIODevice()
{
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::~QIODevice()\n", this);
#endif
}

/*!
    Returns \c true if this device is sequential; otherwise returns
    false.

    Sequential devices, as opposed to a random-access devices, have no
    concept of a start, an end, a size, or a current position, and they
    do not support seeking. You can only read from the device when it
    reports that data is available. The most common example of a
    sequential device is a network socket. On Unix, special files such
    as /dev/zero and fifo pipes are sequential.

    Regular files, on the other hand, do support random access. They
    have both a size and a current position, and they also support
    seeking backwards and forwards in the data stream. Regular files
    are non-sequential.

    \sa bytesAvailable()
*/
bool QIODevice::isSequential() const
{
    return false;
}

/*!
    Returns the mode in which the device has been opened;
    i.e. ReadOnly or WriteOnly.

    \sa OpenMode
*/
QIODeviceBase::OpenMode QIODevice::openMode() const
{
    return d_func()->openMode;
}

/*!
    Sets the OpenMode of the device to \a openMode. Call this
    function to set the open mode if the flags change after the device
    has been opened.

    \sa openMode(), OpenMode
*/
void QIODevice::setOpenMode(QIODeviceBase::OpenMode openMode)
{
    Q_D(QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::setOpenMode(0x%x)\n", this, openMode.toInt());
#endif
    d->openMode = openMode;
    d->accessMode = QIODevicePrivate::Unset;
    d->setReadChannelCount(isReadable() ? qMax(d->readChannelCount, 1) : 0);
    d->setWriteChannelCount(isWritable() ? qMax(d->writeChannelCount, 1) : 0);
}

/*!
    If \a enabled is true, this function sets the \l Text flag on the device;
    otherwise the \l Text flag is removed. This feature is useful for classes
    that provide custom end-of-line handling on a QIODevice.

    The IO device should be opened before calling this function.

    \sa open(), setOpenMode()
 */
void QIODevice::setTextModeEnabled(bool enabled)
{
    Q_D(QIODevice);
    if (!isOpen()) {
        checkWarnMessage(this, "setTextModeEnabled", "The device is not open");
        return;
    }
    if (enabled)
        d->openMode |= Text;
    else
        d->openMode &= ~Text;
}

/*!
    Returns \c true if the \l Text flag is enabled; otherwise returns \c false.

    \sa setTextModeEnabled()
*/
bool QIODevice::isTextModeEnabled() const
{
    return d_func()->openMode.testAnyFlag(Text);
}

/*!
    Returns \c true if the device is open; otherwise returns \c false. A
    device is open if it can be read from and/or written to. By
    default, this function returns \c false if openMode() returns
    \c NotOpen.

    \sa openMode(), QIODeviceBase::OpenMode
*/
bool QIODevice::isOpen() const
{
    return d_func()->openMode != NotOpen;
}

/*!
    Returns \c true if data can be read from the device; otherwise returns
    false. Use bytesAvailable() to determine how many bytes can be read.

    This is a convenience function which checks if the OpenMode of the
    device contains the ReadOnly flag.

    \sa openMode(), OpenMode
*/
bool QIODevice::isReadable() const
{
    return (openMode() & ReadOnly) != 0;
}

/*!
    Returns \c true if data can be written to the device; otherwise returns
    false.

    This is a convenience function which checks if the OpenMode of the
    device contains the WriteOnly flag.

    \sa openMode(), OpenMode
*/
bool QIODevice::isWritable() const
{
    return (openMode() & WriteOnly) != 0;
}

/*!
    \since 5.7

    Returns the number of available read channels if the device is open;
    otherwise returns 0.

    \sa writeChannelCount(), QProcess
*/
int QIODevice::readChannelCount() const
{
    return d_func()->readChannelCount;
}

/*!
    \since 5.7

    Returns the number of available write channels if the device is open;
    otherwise returns 0.

    \sa readChannelCount()
*/
int QIODevice::writeChannelCount() const
{
    return d_func()->writeChannelCount;
}

/*!
    \since 5.7

    Returns the index of the current read channel.

    \sa setCurrentReadChannel(), readChannelCount(), QProcess
*/
int QIODevice::currentReadChannel() const
{
    return d_func()->currentReadChannel;
}

/*!
    \since 5.7

    Sets the current read channel of the QIODevice to the given \a
    channel. The current input channel is used by the functions
    read(), readAll(), readLine(), and getChar(). It also determines
    which channel triggers QIODevice to emit readyRead().

    \sa currentReadChannel(), readChannelCount(), QProcess
*/
void QIODevice::setCurrentReadChannel(int channel)
{
    Q_D(QIODevice);

    if (d->transactionStarted) {
        checkWarnMessage(this, "setReadChannel", "Failed due to read transaction being in progress");
        return;
    }

#if defined QIODEVICE_DEBUG
    qDebug("%p QIODevice::setCurrentReadChannel(%d), d->currentReadChannel = %d, d->readChannelCount = %d\n",
           this, channel, d->currentReadChannel, d->readChannelCount);
#endif

    d->setCurrentReadChannel(channel);
}

/*!
    \internal
*/
void QIODevicePrivate::setReadChannelCount(int count)
{
    if (count > readBuffers.size()) {
        readBuffers.reserve(count);

        // If readBufferChunkSize is zero, we should bypass QIODevice's
        // read buffers, even if the QIODeviceBase::Unbuffered flag is not
        // set when opened. However, if a read transaction is started or
        // ungetChar() is called, we still have to use the internal buffer.
        // To support these cases, pass a default value to the QRingBuffer
        // constructor.

        while (readBuffers.size() < count)
            readBuffers.emplace_back(readBufferChunkSize != 0 ? readBufferChunkSize
                                                              : QIODEVICE_BUFFERSIZE);
    } else {
        readBuffers.resize(count);
    }
    readChannelCount = count;
    setCurrentReadChannel(currentReadChannel);
}

/*!
    \since 5.7

    Returns the index of the current write channel.

    \sa setCurrentWriteChannel(), writeChannelCount()
*/
int QIODevice::currentWriteChannel() const
{
    return d_func()->currentWriteChannel;
}

/*!
    \since 5.7

    Sets the current write channel of the QIODevice to the given \a
    channel. The current output channel is used by the functions
    write(), putChar(). It also determines  which channel triggers
    QIODevice to emit bytesWritten().

    \sa currentWriteChannel(), writeChannelCount()
*/
void QIODevice::setCurrentWriteChannel(int channel)
{
    Q_D(QIODevice);

#if defined QIODEVICE_DEBUG
    qDebug("%p QIODevice::setCurrentWriteChannel(%d), d->currentWriteChannel = %d, d->writeChannelCount = %d\n",
           this, channel, d->currentWriteChannel, d->writeChannelCount);
#endif

    d->setCurrentWriteChannel(channel);
}

/*!
    \internal
*/
void QIODevicePrivate::setWriteChannelCount(int count)
{
    if (count > writeBuffers.size()) {
        // If writeBufferChunkSize is zero (default value), we don't use
        // QIODevice's write buffers.
        if (writeBufferChunkSize != 0) {
            writeBuffers.reserve(count);
            while (writeBuffers.size() < count)
                writeBuffers.emplace_back(writeBufferChunkSize);
        }
    } else {
        writeBuffers.resize(count);
    }
    writeChannelCount = count;
    setCurrentWriteChannel(currentWriteChannel);
}

/*!
    \internal
*/
bool QIODevicePrivate::allWriteBuffersEmpty() const
{
    for (const QRingBuffer &ringBuffer : writeBuffers) {
        if (!ringBuffer.isEmpty())
            return false;
    }
    return true;
}

/*!
    Opens the device and sets its OpenMode to \a mode. Returns \c true if successful;
    otherwise returns \c false. This function should be called from any
    reimplementations of open() or other functions that open the device.

    \sa openMode(), QIODeviceBase::OpenMode
*/
bool QIODevice::open(QIODeviceBase::OpenMode mode)
{
    Q_D(QIODevice);
    d->openMode = mode;
    d->pos = (mode & Append) ? size() : qint64(0);
    d->accessMode = QIODevicePrivate::Unset;
    d->readBuffers.clear();
    d->writeBuffers.clear();
    d->setReadChannelCount(isReadable() ? 1 : 0);
    d->setWriteChannelCount(isWritable() ? 1 : 0);
    d->errorString.clear();
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::open(0x%x)\n", this, mode.toInt());
#endif
    return true;
}

/*!
    First emits aboutToClose(), then closes the device and sets its
    OpenMode to NotOpen. The error string is also reset.

    \sa setOpenMode(), QIODeviceBase::OpenMode
*/
void QIODevice::close()
{
    Q_D(QIODevice);
    if (d->openMode == NotOpen)
        return;

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::close()\n", this);
#endif

#ifndef QT_NO_QOBJECT
    emit aboutToClose();
#endif
    d->openMode = NotOpen;
    d->pos = 0;
    d->transactionStarted = false;
    d->transactionPos = 0;
    d->setReadChannelCount(0);
    // Do not clear write buffers to allow delayed close in sockets
    d->writeChannelCount = 0;
}

/*!
    For random-access devices, this function returns the position that
    data is written to or read from. For sequential devices or closed
    devices, where there is no concept of a "current position", 0 is
    returned.

    The current read/write position of the device is maintained internally by
    QIODevice, so reimplementing this function is not necessary. When
    subclassing QIODevice, use QIODevice::seek() to notify QIODevice about
    changes in the device position.

    \sa isSequential(), seek()
*/
qint64 QIODevice::pos() const
{
    Q_D(const QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::pos() == %lld\n", this, d->pos);
#endif
    return d->pos;
}

/*!
    For open random-access devices, this function returns the size of the
    device. For open sequential devices, bytesAvailable() is returned.

    If the device is closed, the size returned will not reflect the actual
    size of the device.

    \sa isSequential(), pos()
*/
qint64 QIODevice::size() const
{
    return d_func()->isSequential() ? bytesAvailable() : qint64(0);
}

/*!
    For random-access devices, this function sets the current position
    to \a pos, returning true on success, or false if an error occurred.
    For sequential devices, the default behavior is to produce a warning
    and return false.

    When subclassing QIODevice, you must call QIODevice::seek() at the
    start of your function to ensure integrity with QIODevice's
    built-in buffer.

    \sa pos(), isSequential()
*/
bool QIODevice::seek(qint64 pos)
{
    Q_D(QIODevice);
    if (d->isSequential()) {
        checkWarnMessage(this, "seek", "Cannot call seek on a sequential device");
        return false;
    }
    if (d->openMode == NotOpen) {
        checkWarnMessage(this, "seek", "The device is not open");
        return false;
    }
    if (pos < 0) {
        qWarning("QIODevice::seek: Invalid pos: %lld", pos);
        return false;
    }

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::seek(%lld), before: d->pos = %lld, d->buffer.size() = %lld\n",
           this, pos, d->pos, d->buffer.size());
#endif

    d->devicePos = pos;
    d->seekBuffer(pos);

#if defined QIODEVICE_DEBUG
    printf("%p \tafter: d->pos == %lld, d->buffer.size() == %lld\n", this, d->pos,
           d->buffer.size());
#endif
    return true;
}

/*!
    \internal
*/
void QIODevicePrivate::seekBuffer(qint64 newPos)
{
    const qint64 offset = newPos - pos;
    pos = newPos;

    if (offset < 0 || offset >= buffer.size()) {
        // When seeking backwards, an operation that is only allowed for
        // random-access devices, the buffer is cleared. The next read
        // operation will then refill the buffer.
        buffer.clear();
    } else {
        buffer.free(offset);
    }
}

/*!
    Returns \c true if the current read and write position is at the end
    of the device (i.e. there is no more data available for reading on
    the device); otherwise returns \c false.

    For some devices, atEnd() can return true even though there is more data
    to read. This special case only applies to devices that generate data in
    direct response to you calling read() (e.g., \c /dev or \c /proc files on
    Unix and \macos, or console input / \c stdin on all platforms).

    \sa bytesAvailable(), read(), isSequential()
*/
bool QIODevice::atEnd() const
{
    Q_D(const QIODevice);
    const bool result = (d->openMode == NotOpen || (d->isBufferEmpty()
                                                    && bytesAvailable() == 0));
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::atEnd() returns %s, d->openMode == %d, d->pos == %lld\n", this,
           result ? "true" : "false", d->openMode.toInt(), d->pos);
#endif
    return result;
}

/*!
    Seeks to the start of input for random-access devices. Returns
    true on success; otherwise returns \c false (for example, if the
    device is not open).

    Note that when using a QTextStream on a QFile, calling reset() on
    the QFile will not have the expected result because QTextStream
    buffers the file. Use the QTextStream::seek() function instead.

    \sa seek()
*/
bool QIODevice::reset()
{
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::reset()\n", this);
#endif
    return seek(0);
}

/*!
    Returns the number of bytes that are available for reading. This
    function is commonly used with sequential devices to determine the
    number of bytes to allocate in a buffer before reading.

    Subclasses that reimplement this function must call the base
    implementation in order to include the size of the buffer of QIODevice. Example:

    \snippet code/src_corelib_io_qiodevice.cpp 1

    \sa bytesToWrite(), readyRead(), isSequential()
*/
qint64 QIODevice::bytesAvailable() const
{
    Q_D(const QIODevice);
    if (!d->isSequential())
        return qMax(size() - d->pos, qint64(0));
    return d->buffer.size() - d->transactionPos;
}

/*!  For buffered devices, this function returns the number of bytes
    waiting to be written. For devices with no buffer, this function
    returns 0.

    Subclasses that reimplement this function must call the base
    implementation in order to include the size of the buffer of QIODevice.

    \sa bytesAvailable(), bytesWritten(), isSequential()
*/
qint64 QIODevice::bytesToWrite() const
{
    return d_func()->writeBuffer.size();
}

/*!
    Reads at most \a maxSize bytes from the device into \a data, and
    returns the number of bytes read. If an error occurs, such as when
    attempting to read from a device opened in WriteOnly mode, this
    function returns -1.

    0 is returned when no more data is available for reading. However,
    reading past the end of the stream is considered an error, so this
    function returns -1 in those cases (that is, reading on a closed
    socket or after a process has died).

    \sa readData(), readLine(), write()
*/
qint64 QIODevice::read(char *data, qint64 maxSize)
{
    Q_D(QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::read(%p, %lld), d->pos = %lld, d->buffer.size() = %lld\n",
           this, data, maxSize, d->pos, d->buffer.size());
#endif

    CHECK_READABLE(read, qint64(-1));
    const bool sequential = d->isSequential();

    // Short-cut for getChar(), unless we need to keep the data in the buffer.
    if (maxSize == 1 && !(sequential && d->transactionStarted)) {
        int chint;
        while ((chint = d->buffer.getChar()) != -1) {
            if (!sequential)
                ++d->pos;

            char c = char(uchar(chint));
            if (c == '\r' && (d->openMode & Text))
                continue;
            *data = c;
#if defined QIODEVICE_DEBUG
            printf("%p \tread 0x%hhx (%c) returning 1 (shortcut)\n", this,
                   int(c), isAsciiPrintable(c) ? c : '?');
#endif
            if (d->buffer.isEmpty())
                readData(data, 0);
            return qint64(1);
        }
    }

    CHECK_MAXLEN(read, qint64(-1));
    const qint64 readBytes = d->read(data, maxSize);

#if defined QIODEVICE_DEBUG
    printf("%p \treturning %lld, d->pos == %lld, d->buffer.size() == %lld\n", this,
           readBytes, d->pos, d->buffer.size());
    if (readBytes > 0)
        debugBinaryString(data - readBytes, readBytes);
#endif

    return readBytes;
}

/*!
    \internal
*/
qint64 QIODevicePrivate::read(char *data, qint64 maxSize, bool peeking)
{
    Q_Q(QIODevice);

    const bool buffered = (readBufferChunkSize != 0 && (openMode & QIODevice::Unbuffered) == 0);
    const bool sequential = isSequential();
    const bool keepDataInBuffer = sequential
                                  ? peeking || transactionStarted
                                  : peeking && buffered;
    const qint64 savedPos = pos;
    qint64 readSoFar = 0;
    bool madeBufferReadsOnly = true;
    bool deviceAtEof = false;
    char *readPtr = data;
    qint64 bufferPos = (sequential && transactionStarted) ? transactionPos : Q_INT64_C(0);
    forever {
        // Try reading from the buffer.
        qint64 bufferReadChunkSize = keepDataInBuffer
                                     ? buffer.peek(data, maxSize, bufferPos)
                                     : buffer.read(data, maxSize);
        if (bufferReadChunkSize > 0) {
            bufferPos += bufferReadChunkSize;
            if (!sequential)
                pos += bufferReadChunkSize;
#if defined QIODEVICE_DEBUG
            printf("%p \treading %lld bytes from buffer into position %lld\n", q,
                   bufferReadChunkSize, readSoFar);
#endif
            readSoFar += bufferReadChunkSize;
            data += bufferReadChunkSize;
            maxSize -= bufferReadChunkSize;
        }

        if (maxSize > 0 && !deviceAtEof) {
            qint64 readFromDevice = 0;
            // Make sure the device is positioned correctly.
            if (sequential || pos == devicePos || q->seek(pos)) {
                madeBufferReadsOnly = false; // fix readData attempt
                if ((!buffered || maxSize >= readBufferChunkSize) && !keepDataInBuffer) {
                    // Read big chunk directly to output buffer
                    readFromDevice = q->readData(data, maxSize);
                    deviceAtEof = (readFromDevice != maxSize);
#if defined QIODEVICE_DEBUG
                    printf("%p \treading %lld bytes from device (total %lld)\n", q,
                           readFromDevice, readSoFar);
#endif
                    if (readFromDevice > 0) {
                        readSoFar += readFromDevice;
                        data += readFromDevice;
                        maxSize -= readFromDevice;
                        if (!sequential) {
                            pos += readFromDevice;
                            devicePos += readFromDevice;
                        }
                    }
                } else {
                    // Do not read more than maxSize on unbuffered devices
                    const qint64 bytesToBuffer = (!buffered && maxSize < buffer.chunkSize())
                            ? maxSize
                            : qint64(buffer.chunkSize());
                    // Try to fill QIODevice buffer by single read
                    readFromDevice = q->readData(buffer.reserve(bytesToBuffer), bytesToBuffer);
                    deviceAtEof = (readFromDevice != bytesToBuffer);
                    buffer.chop(bytesToBuffer - qMax(Q_INT64_C(0), readFromDevice));
                    if (readFromDevice > 0) {
                        if (!sequential)
                            devicePos += readFromDevice;
#if defined QIODEVICE_DEBUG
                        printf("%p \treading %lld from device into buffer\n", q,
                               readFromDevice);
#endif
                        continue;
                    }
                }
            } else {
                readFromDevice = -1;
            }

            if (readFromDevice < 0 && readSoFar == 0) {
                // error and we haven't read anything: return immediately
                return qint64(-1);
            }
        }

        if ((openMode & QIODevice::Text) && readPtr < data) {
            const char *endPtr = data;

            // optimization to avoid initial self-assignment
            while (*readPtr != '\r') {
                if (++readPtr == endPtr)
                    break;
            }

            char *writePtr = readPtr;

            while (readPtr < endPtr) {
                char ch = *readPtr++;
                if (ch != '\r')
                    *writePtr++ = ch;
                else {
                    --readSoFar;
                    --data;
                    ++maxSize;
                }
            }

            // Make sure we get more data if there is room for more. This
            // is very important for when someone seeks to the start of a
            // '\r\n' and reads one character - they should get the '\n'.
            readPtr = data;
            continue;
        }

        break;
    }

    // Restore positions after reading
    if (keepDataInBuffer) {
        if (peeking)
            pos = savedPos; // does nothing on sequential devices
        else
            transactionPos = bufferPos;
    } else if (peeking) {
        seekBuffer(savedPos); // unbuffered random-access device
    }

    if (madeBufferReadsOnly && isBufferEmpty())
        q->readData(data, 0);

    return readSoFar;
}

/*!
    \overload

    Reads at most \a maxSize bytes from the device, and returns the
    data read as a QByteArray.

    This function has no way of reporting errors; returning an empty
    QByteArray can mean either that no data was currently available
    for reading, or that an error occurred.
*/

QByteArray QIODevice::read(qint64 maxSize)
{
    Q_D(QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::read(%lld), d->pos = %lld, d->buffer.size() = %lld\n",
           this, maxSize, d->pos, d->buffer.size());
#endif

    QByteArray result;
    CHECK_READABLE(read, result);

    // Try to prevent the data from being copied, if we have a chunk
    // with the same size in the read buffer.
    if (maxSize == d->buffer.nextDataBlockSize() && !d->transactionStarted
        && (d->openMode & QIODevice::Text) == 0) {
        result = d->buffer.read();
        if (!d->isSequential())
            d->pos += maxSize;
        if (d->buffer.isEmpty())
            readData(nullptr, 0);
        return result;
    }

    CHECK_MAXLEN(read, result);
    CHECK_MAXBYTEARRAYSIZE(read);

    result.resize(qsizetype(maxSize));
    qint64 readBytes = d->read(result.data(), result.size());

    if (readBytes <= 0)
        result.clear();
    else
        result.resize(qsizetype(readBytes));

    return result;
}

/*!
    Reads all remaining data from the device, and returns it as a
    byte array.

    This function has no way of reporting errors; returning an empty
    QByteArray can mean either that no data was currently available
    for reading, or that an error occurred. This function also has no
    way of indicating that more data may have been available and
    couldn't be read.
*/
QByteArray QIODevice::readAll()
{
    Q_D(QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::readAll(), d->pos = %lld, d->buffer.size() = %lld\n",
           this, d->pos, d->buffer.size());
#endif

    QByteArray result;
    CHECK_READABLE(read, result);

    qint64 readBytes = (d->isSequential() ? Q_INT64_C(0) : size());
    if (readBytes == 0) {
        // Size is unknown, read incrementally.
        qint64 readChunkSize = qMax(qint64(d->buffer.chunkSize()),
                                    d->isSequential() ? (d->buffer.size() - d->transactionPos)
                                                      : d->buffer.size());
        qint64 readResult;
        do {
            if (readBytes + readChunkSize >= MaxByteArraySize) {
                // If resize would fail, don't read more, return what we have.
                break;
            }
            result.resize(readBytes + readChunkSize);
            readResult = d->read(result.data() + readBytes, readChunkSize);
            if (readResult > 0 || readBytes == 0) {
                readBytes += readResult;
                readChunkSize = d->buffer.chunkSize();
            }
        } while (readResult > 0);
    } else {
        // Read it all in one go.
        readBytes -= d->pos;
        if (readBytes >= MaxByteArraySize)
            readBytes = MaxByteArraySize;
        result.resize(readBytes);
        readBytes = d->read(result.data(), readBytes);
    }

    if (readBytes <= 0)
        result.clear();
    else
        result.resize(qsizetype(readBytes));

    return result;
}

/*!
    This function reads a line of ASCII characters from the device, up
    to a maximum of \a maxSize - 1 bytes, stores the characters in \a
    data, and returns the number of bytes read. If a line could not be
    read but no error occurred, this function returns 0. If an error
    occurs, this function returns the length of what could be read, or
    -1 if nothing was read.

    A terminating '\\0' byte is always appended to \a data, so \a
    maxSize must be larger than 1.

    Data is read until either of the following conditions are met:

    \list
    \li The first '\\n' character is read.
    \li \a maxSize - 1 bytes are read.
    \li The end of the device data is detected.
    \endlist

    For example, the following code reads a line of characters from a
    file:

    \snippet code/src_corelib_io_qiodevice.cpp 2

    The newline character ('\\n') is included in the buffer. If a
    newline is not encountered before maxSize - 1 bytes are read, a
    newline will not be inserted into the buffer. On windows newline
    characters are replaced with '\\n'.

    Note that on sequential devices, data may not be immediately available,
    which may result in a partial line being returned. By calling the
    canReadLine() function before reading, you can check whether a complete
    line (including the newline character) can be read.

    This function calls readLineData(), which is implemented using
    repeated calls to getChar(). You can provide a more efficient
    implementation by reimplementing readLineData() in your own
    subclass.

    \sa getChar(), read(), canReadLine(), write()
*/
qint64 QIODevice::readLine(char *data, qint64 maxSize)
{
    Q_D(QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::readLine(%p, %lld), d->pos = %lld, d->buffer.size() = %lld\n",
           this, data, maxSize, d->pos, d->buffer.size());
#endif

    CHECK_READABLE(readLine, qint64(-1));
    CHECK_LINEMAXLEN(readLine, qint64(-1));
    const qint64 readBytes = d->readLine(data, maxSize);

#if defined QIODEVICE_DEBUG
    printf("%p \treturning %lld, d->pos = %lld, d->buffer.size() = %lld, size() = %lld\n",
           this, readBytes, d->pos, d->buffer.size(), size());
    debugBinaryString(data, readBytes);
#endif

    return readBytes;
}

/*!
    \internal
*/
qint64 QIODevicePrivate::readLine(char *data, qint64 maxSize)
{
    Q_Q(QIODevice);
    Q_ASSERT(maxSize >= 2);

    // Leave room for a '\0'
    --maxSize;

    const bool sequential = isSequential();
    const bool keepDataInBuffer = sequential && transactionStarted;

    qint64 readSoFar = 0;
    if (keepDataInBuffer) {
        if (transactionPos < buffer.size()) {
            // Peek line from the specified position
            const qint64 i = buffer.indexOf('\n', maxSize, transactionPos);
            readSoFar = buffer.peek(data, i >= 0 ? (i - transactionPos + 1) : maxSize,
                                    transactionPos);
            transactionPos += readSoFar;
            if (transactionPos == buffer.size())
                q->readData(data, 0);
        }
    } else if (!buffer.isEmpty()) {
        // QRingBuffer::readLine() terminates the line with '\0'
        readSoFar = buffer.readLine(data, maxSize + 1);
        if (buffer.isEmpty())
            q->readData(data, 0);
        if (!sequential)
            pos += readSoFar;
    }

    if (readSoFar) {
#if defined QIODEVICE_DEBUG
        printf("%p \tread from buffer: %lld bytes, last character read: %hhx\n", q,
               readSoFar, data[readSoFar - 1]);
        debugBinaryString(data, readSoFar);
#endif
        if (data[readSoFar - 1] == '\n') {
            if (openMode & QIODevice::Text) {
                // QRingBuffer::readLine() isn't Text aware.
                if (readSoFar > 1 && data[readSoFar - 2] == '\r') {
                    --readSoFar;
                    data[readSoFar - 1] = '\n';
                }
            }
            data[readSoFar] = '\0';
            return readSoFar;
        }
    }

    if (pos != devicePos && !sequential && !q->seek(pos))
        return qint64(-1);
    baseReadLineDataCalled = false;
    // Force base implementation for transaction on sequential device
    // as it stores the data in internal buffer automatically.
    qint64 readBytes = keepDataInBuffer
                       ? q->QIODevice::readLineData(data + readSoFar, maxSize - readSoFar)
                       : q->readLineData(data + readSoFar, maxSize - readSoFar);
#if defined QIODEVICE_DEBUG
    printf("%p \tread from readLineData: %lld bytes, readSoFar = %lld bytes\n", q,
           readBytes, readSoFar);
    if (readBytes > 0) {
        debugBinaryString(data, readSoFar + readBytes);
    }
#endif
    if (readBytes < 0) {
        data[readSoFar] = '\0';
        return readSoFar ? readSoFar : -1;
    }
    readSoFar += readBytes;
    if (!baseReadLineDataCalled && !sequential) {
        pos += readBytes;
        // If the base implementation was not called, then we must
        // assume the device position is invalid and force a seek.
        devicePos = qint64(-1);
    }
    data[readSoFar] = '\0';

    if (openMode & QIODevice::Text) {
        if (readSoFar > 1 && data[readSoFar - 1] == '\n' && data[readSoFar - 2] == '\r') {
            data[readSoFar - 2] = '\n';
            data[readSoFar - 1] = '\0';
            --readSoFar;
        }
    }

    return readSoFar;
}

/*!
    \overload

    Reads a line from the device, but no more than \a maxSize characters,
    and returns the result as a byte array.

    This function has no way of reporting errors; returning an empty
    QByteArray can mean either that no data was currently available
    for reading, or that an error occurred.
*/
QByteArray QIODevice::readLine(qint64 maxSize)
{
    Q_D(QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::readLine(%lld), d->pos = %lld, d->buffer.size() = %lld\n",
           this, maxSize, d->pos, d->buffer.size());
#endif

    QByteArray result;
    CHECK_READABLE(readLine, result);

    qint64 readBytes = 0;
    if (maxSize == 0) {
        // Size is unknown, read incrementally.
        maxSize = MaxByteArraySize - 1;

        // The first iteration needs to leave an extra byte for the terminating null
        result.resize(1);

        qint64 readResult;
        do {
            result.resize(qsizetype(qMin(maxSize, qint64(result.size() + d->buffer.chunkSize()))));
            readResult = d->readLine(result.data() + readBytes, result.size() - readBytes);
            if (readResult > 0 || readBytes == 0)
                readBytes += readResult;
        } while (readResult == d->buffer.chunkSize()
                && result[qsizetype(readBytes - 1)] != '\n');
    } else {
        CHECK_LINEMAXLEN(readLine, result);
        CHECK_MAXBYTEARRAYSIZE(readLine);

        result.resize(maxSize);
        readBytes = d->readLine(result.data(), result.size());
    }

    if (readBytes <= 0)
        result.clear();
    else
        result.resize(readBytes);

    result.squeeze();
    return result;
}

/*!
    Reads up to \a maxSize characters into \a data and returns the
    number of characters read.

    This function is called by readLine(), and provides its base
    implementation, using getChar(). Buffered devices can improve the
    performance of readLine() by reimplementing this function.

    readLine() appends a '\\0' byte to \a data; readLineData() does not
    need to do this.

    If you reimplement this function, be careful to return the correct
    value: it should return the number of bytes read in this line,
    including the terminating newline, or 0 if there is no line to be
    read at this point. If an error occurs, it should return -1 if and
    only if no bytes were read. Reading past EOF is considered an error.
*/
qint64 QIODevice::readLineData(char *data, qint64 maxSize)
{
    Q_D(QIODevice);
    qint64 readSoFar = 0;
    char c;
    qint64 lastReadReturn = 0;
    d->baseReadLineDataCalled = true;

    while (readSoFar < maxSize && (lastReadReturn = read(&c, 1)) == 1) {
        *data++ = c;
        ++readSoFar;
        if (c == '\n')
            break;
    }

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::readLineData(%p, %lld), d->pos = %lld, d->buffer.size() = %lld, "
           "returns %lld\n", this, data, maxSize, d->pos, d->buffer.size(), readSoFar);
#endif
    if (lastReadReturn != 1 && readSoFar == 0)
        return isSequential() ? lastReadReturn : -1;
    return readSoFar;
}

/*!
    Returns \c true if a complete line of data can be read from the device;
    otherwise returns \c false.

    Note that unbuffered devices, which have no way of determining what
    can be read, always return false.

    This function is often called in conjunction with the readyRead()
    signal.

    Subclasses that reimplement this function must call the base
    implementation in order to include the contents of the QIODevice's buffer. Example:

    \snippet code/src_corelib_io_qiodevice.cpp 3

    \sa readyRead(), readLine()
*/
bool QIODevice::canReadLine() const
{
    Q_D(const QIODevice);
    return d->buffer.indexOf('\n', d->buffer.size(),
                             d->isSequential() ? d->transactionPos : Q_INT64_C(0)) >= 0;
}

/*!
    \since 5.7

    Starts a new read transaction on the device.

    Defines a restorable point within the sequence of read operations. For
    sequential devices, read data will be duplicated internally to allow
    recovery in case of incomplete reads. For random-access devices,
    this function saves the current position. Call commitTransaction() or
    rollbackTransaction() to finish the transaction.

    \note Nesting transactions is not supported.

    \sa commitTransaction(), rollbackTransaction()
*/
void QIODevice::startTransaction()
{
    Q_D(QIODevice);
    if (d->transactionStarted) {
        checkWarnMessage(this, "startTransaction", "Called while transaction already in progress");
        return;
    }
    d->transactionPos = d->pos;
    d->transactionStarted = true;
}

/*!
    \since 5.7

    Completes a read transaction.

    For sequential devices, all data recorded in the internal buffer during
    the transaction will be discarded.

    \sa startTransaction(), rollbackTransaction()
*/
void QIODevice::commitTransaction()
{
    Q_D(QIODevice);
    if (!d->transactionStarted) {
        checkWarnMessage(this, "commitTransaction", "Called while no transaction in progress");
        return;
    }
    if (d->isSequential())
        d->buffer.free(d->transactionPos);
    d->transactionStarted = false;
    d->transactionPos = 0;
}

/*!
    \since 5.7

    Rolls back a read transaction.

    Restores the input stream to the point of the startTransaction() call.
    This function is commonly used to rollback the transaction when an
    incomplete read was detected prior to committing the transaction.

    \sa startTransaction(), commitTransaction()
*/
void QIODevice::rollbackTransaction()
{
    Q_D(QIODevice);
    if (!d->transactionStarted) {
        checkWarnMessage(this, "rollbackTransaction", "Called while no transaction in progress");
        return;
    }
    if (!d->isSequential())
        d->seekBuffer(d->transactionPos);
    d->transactionStarted = false;
    d->transactionPos = 0;
}

/*!
    \since 5.7

    Returns \c true if a transaction is in progress on the device, otherwise
    \c false.

    \sa startTransaction()
*/
bool QIODevice::isTransactionStarted() const
{
    return d_func()->transactionStarted;
}

/*!
    Writes at most \a maxSize bytes of data from \a data to the
    device. Returns the number of bytes that were actually written, or
    -1 if an error occurred.

    \sa read(), writeData()
*/
qint64 QIODevice::write(const char *data, qint64 maxSize)
{
    Q_D(QIODevice);
    CHECK_WRITABLE(write, qint64(-1));
    CHECK_MAXLEN(write, qint64(-1));

    const bool sequential = d->isSequential();
    // Make sure the device is positioned correctly.
    if (d->pos != d->devicePos && !sequential && !seek(d->pos))
        return qint64(-1);

#ifdef Q_OS_WIN
    if (d->openMode & Text) {
        const char *endOfData = data + maxSize;
        const char *startOfBlock = data;

        qint64 writtenSoFar = 0;
        const qint64 savedPos = d->pos;

        forever {
            const char *endOfBlock = startOfBlock;
            while (endOfBlock < endOfData && *endOfBlock != '\n')
                ++endOfBlock;

            qint64 blockSize = endOfBlock - startOfBlock;
            if (blockSize > 0) {
                qint64 ret = writeData(startOfBlock, blockSize);
                if (ret <= 0) {
                    if (writtenSoFar && !sequential)
                        d->buffer.skip(d->pos - savedPos);
                    return writtenSoFar ? writtenSoFar : ret;
                }
                if (!sequential) {
                    d->pos += ret;
                    d->devicePos += ret;
                }
                writtenSoFar += ret;
            }

            if (endOfBlock == endOfData)
                break;

            qint64 ret = writeData("\r\n", 2);
            if (ret <= 0) {
                if (writtenSoFar && !sequential)
                    d->buffer.skip(d->pos - savedPos);
                return writtenSoFar ? writtenSoFar : ret;
            }
            if (!sequential) {
                d->pos += ret;
                d->devicePos += ret;
            }
            ++writtenSoFar;

            startOfBlock = endOfBlock + 1;
        }

        if (writtenSoFar && !sequential)
            d->buffer.skip(d->pos - savedPos);
        return writtenSoFar;
    }
#endif

    qint64 written = writeData(data, maxSize);
    if (!sequential && written > 0) {
        d->pos += written;
        d->devicePos += written;
        d->buffer.skip(written);
    }
    return written;
}

/*!
    \since 4.5

    \overload

    Writes data from a zero-terminated string of 8-bit characters to the
    device. Returns the number of bytes that were actually written, or
    -1 if an error occurred. This is equivalent to
    \code
    ...
    QIODevice::write(data, qstrlen(data));
    ...
    \endcode

    \sa read(), writeData()
*/
qint64 QIODevice::write(const char *data)
{
    return write(data, qstrlen(data));
}

/*!
    \overload

    Writes the content of \a data to the device. Returns the number of
    bytes that were actually written, or -1 if an error occurred.

    \sa read(), writeData()
*/

qint64 QIODevice::write(const QByteArray &data)
{
    Q_D(QIODevice);

    // Keep the chunk pointer for further processing in
    // QIODevicePrivate::write(). To reduce fragmentation,
    // the chunk size must be sufficiently large.
    if (data.size() >= QRINGBUFFER_CHUNKSIZE)
        d->currentWriteChunk = &data;

    const qint64 ret = write(data.constData(), data.size());

    d->currentWriteChunk = nullptr;
    return ret;
}

/*!
    \internal
*/
void QIODevicePrivate::write(const char *data, qint64 size)
{
    if (isWriteChunkCached(data, size)) {
        // We are called from write(const QByteArray &) overload.
        // So, we can make a shallow copy of chunk.
        writeBuffer.append(*currentWriteChunk);
    } else {
        writeBuffer.append(data, size);
    }
}

/*!
    Puts the character \a c back into the device, and decrements the
    current position unless the position is 0. This function is
    usually called to "undo" a getChar() operation, such as when
    writing a backtracking parser.

    If \a c was not previously read from the device, the behavior is
    undefined.

    \note This function is not available while a transaction is in progress.
*/
void QIODevice::ungetChar(char c)
{
    Q_D(QIODevice);
    CHECK_READABLE(read, Q_VOID);

    if (d->transactionStarted) {
        checkWarnMessage(this, "ungetChar", "Called while transaction is in progress");
        return;
    }

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::ungetChar(0x%hhx '%c')\n", this, c, isAsciiPrintable(c) ? c : '?');
#endif

    d->buffer.ungetChar(c);
    if (!d->isSequential())
        --d->pos;
}

/*! \fn bool QIODevice::putChar(char c)

    Writes the character \a c to the device. Returns \c true on success;
    otherwise returns \c false.

    \sa write(), getChar(), ungetChar()
*/
bool QIODevice::putChar(char c)
{
    return d_func()->putCharHelper(c);
}

/*!
    \internal
*/
bool QIODevicePrivate::putCharHelper(char c)
{
    return q_func()->write(&c, 1) == 1;
}

/*!
    \internal
*/
qint64 QIODevicePrivate::peek(char *data, qint64 maxSize)
{
    return read(data, maxSize, true);
}

/*!
    \internal
*/
QByteArray QIODevicePrivate::peek(qint64 maxSize)
{
    QByteArray result(maxSize, Qt::Uninitialized);

    const qint64 readBytes = read(result.data(), maxSize, true);

    if (readBytes < maxSize) {
        if (readBytes <= 0)
            result.clear();
        else
            result.resize(readBytes);
    }

    return result;
}

/*! \fn bool QIODevice::getChar(char *c)

    Reads one character from the device and stores it in \a c. If \a c
    is \nullptr, the character is discarded. Returns \c true on success;
    otherwise returns \c false.

    \sa read(), putChar(), ungetChar()
*/
bool QIODevice::getChar(char *c)
{
    // readability checked in read()
    char ch;
    return (1 == read(c ? c : &ch, 1));
}

/*!
    \since 4.1

    Reads at most \a maxSize bytes from the device into \a data, without side
    effects (i.e., if you call read() after peek(), you will get the same
    data).  Returns the number of bytes read. If an error occurs, such as
    when attempting to peek a device opened in WriteOnly mode, this function
    returns -1.

    0 is returned when no more data is available for reading.

    Example:

    \snippet code/src_corelib_io_qiodevice.cpp 4

    \sa read()
*/
qint64 QIODevice::peek(char *data, qint64 maxSize)
{
    Q_D(QIODevice);

    CHECK_MAXLEN(peek, qint64(-1));
    CHECK_READABLE(peek, qint64(-1));

    return d->peek(data, maxSize);
}

/*!
    \since 4.1
    \overload

    Peeks at most \a maxSize bytes from the device, returning the data peeked
    as a QByteArray.

    Example:

    \snippet code/src_corelib_io_qiodevice.cpp 5

    This function has no way of reporting errors; returning an empty
    QByteArray can mean either that no data was currently available
    for peeking, or that an error occurred.

    \sa read()
*/
QByteArray QIODevice::peek(qint64 maxSize)
{
    Q_D(QIODevice);

    CHECK_MAXLEN(peek, QByteArray());
    CHECK_MAXBYTEARRAYSIZE(peek);
    CHECK_READABLE(peek, QByteArray());

    return d->peek(maxSize);
}

/*!
    \since 5.10

    Skips up to \a maxSize bytes from the device. Returns the number of bytes
    actually skipped, or -1 on error.

    This function does not wait and only discards the data that is already
    available for reading.

    If the device is opened in text mode, end-of-line terminators are
    translated to '\n' symbols and count as a single byte identically to the
    read() and peek() behavior.

    This function works for all devices, including sequential ones that cannot
    seek(). It is optimized to skip unwanted data after a peek() call.

    For random-access devices, skip() can be used to seek forward from the
    current position. Negative \a maxSize values are not allowed.

    \sa skipData(), peek(), seek(), read()
*/
qint64 QIODevice::skip(qint64 maxSize)
{
    Q_D(QIODevice);
    CHECK_MAXLEN(skip, qint64(-1));
    CHECK_READABLE(skip, qint64(-1));

    const bool sequential = d->isSequential();

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::skip(%lld), d->pos = %lld, d->buffer.size() = %lld\n",
           this, maxSize, d->pos, d->buffer.size());
#endif

    if ((sequential && d->transactionStarted) || (d->openMode & QIODevice::Text) != 0)
        return d->skipByReading(maxSize);

    // First, skip over any data in the internal buffer.
    qint64 skippedSoFar = 0;
    if (!d->buffer.isEmpty()) {
        skippedSoFar = d->buffer.skip(maxSize);
#if defined QIODEVICE_DEBUG
        printf("%p \tskipping %lld bytes in buffer\n", this, skippedSoFar);
#endif
        if (!sequential)
            d->pos += skippedSoFar;
        if (d->buffer.isEmpty())
            readData(nullptr, 0);
        if (skippedSoFar == maxSize)
            return skippedSoFar;

        maxSize -= skippedSoFar;
    }

    // Try to seek on random-access device. At this point,
    // the internal read buffer is empty.
    if (!sequential) {
        const qint64 bytesToSkip = qMin(size() - d->pos, maxSize);

        // If the size is unknown or file position is at the end,
        // fall back to reading below.
        if (bytesToSkip > 0) {
            if (!seek(d->pos + bytesToSkip))
                return skippedSoFar ? skippedSoFar : Q_INT64_C(-1);
            if (bytesToSkip == maxSize)
                return skippedSoFar + bytesToSkip;

            skippedSoFar += bytesToSkip;
            maxSize -= bytesToSkip;
        }
    }

    const qint64 skipResult = skipData(maxSize);
    if (skippedSoFar == 0)
        return skipResult;

    if (skipResult == -1)
        return skippedSoFar;

    return skippedSoFar + skipResult;
}

/*!
    \internal
*/
qint64 QIODevicePrivate::skipByReading(qint64 maxSize)
{
    qint64 readSoFar = 0;
    do {
        char dummy[4096];
        const qint64 readBytes = qMin<qint64>(maxSize, sizeof(dummy));
        const qint64 readResult = read(dummy, readBytes);

        // Do not try again, if we got less data.
        if (readResult != readBytes) {
            if (readSoFar == 0)
                return readResult;

            if (readResult == -1)
                return readSoFar;

            return readSoFar + readResult;
        }

        readSoFar += readResult;
        maxSize -= readResult;
    } while (maxSize > 0);

    return readSoFar;
}

/*!
    \since 6.0

    Skips up to \a maxSize bytes from the device. Returns the number of bytes
    actually skipped, or -1 on error.

    This function is called by QIODevice. Consider reimplementing it
    when creating a subclass of QIODevice.

    The base implementation discards the data by reading into a dummy buffer.
    This is slow, but works for all types of devices. Subclasses can
    reimplement this function to improve on that.

    \sa skip(), peek(), seek(), read()
*/
qint64 QIODevice::skipData(qint64 maxSize)
{
    return d_func()->skipByReading(maxSize);
}

/*!
    Blocks until new data is available for reading and the readyRead()
    signal has been emitted, or until \a msecs milliseconds have
    passed. If msecs is -1, this function will not time out.

    Returns \c true if new data is available for reading; otherwise returns
    false (if the operation timed out or if an error occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    If called from within a slot connected to the readyRead() signal,
    readyRead() will not be reemitted.

    Reimplement this function to provide a blocking API for a custom
    device. The default implementation does nothing, and returns \c false.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    \sa waitForBytesWritten()
*/
bool QIODevice::waitForReadyRead(int msecs)
{
    Q_UNUSED(msecs);
    return false;
}

/*!
    For buffered devices, this function waits until a payload of
    buffered written data has been written to the device and the
    bytesWritten() signal has been emitted, or until \a msecs
    milliseconds have passed. If msecs is -1, this function will
    not time out. For unbuffered devices, it returns immediately.

    Returns \c true if a payload of data was written to the device;
    otherwise returns \c false (i.e. if the operation timed out, or if an
    error occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    If called from within a slot connected to the bytesWritten() signal,
    bytesWritten() will not be reemitted.

    Reimplement this function to provide a blocking API for a custom
    device. The default implementation does nothing, and returns \c false.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    \sa waitForReadyRead()
*/
bool QIODevice::waitForBytesWritten(int msecs)
{
    Q_UNUSED(msecs);
    return false;
}

/*!
    Sets the human readable description of the last device error that
    occurred to \a str.

    \sa errorString()
*/
void QIODevice::setErrorString(const QString &str)
{
    d_func()->errorString = str;
}

/*!
    Returns a human-readable description of the last device error that
    occurred.

    \sa setErrorString()
*/
QString QIODevice::errorString() const
{
    Q_D(const QIODevice);
    if (d->errorString.isEmpty()) {
#ifdef QT_NO_QOBJECT
        return QLatin1StringView(QT_TRANSLATE_NOOP(QIODevice, "Unknown error"));
#else
        return tr("Unknown error");
#endif
    }
    return d->errorString;
}

/*!
    \fn qint64 QIODevice::readData(char *data, qint64 maxSize)

    Reads up to \a maxSize bytes from the device into \a data, and
    returns the number of bytes read or -1 if an error occurred.

    If there are no bytes to be read and there can never be more bytes
    available (examples include socket closed, pipe closed, sub-process
    finished), this function returns -1.

    This function is called by QIODevice. Reimplement this function
    when creating a subclass of QIODevice.

    When reimplementing this function it is important that this function
    reads all the required data before returning. This is required in order
    for QDataStream to be able to operate on the class. QDataStream assumes
    all the requested information was read and therefore does not retry reading
    if there was a problem.

    This function might be called with a maxSize of 0, which can be used to
    perform post-reading operations.

    \sa read(), readLine(), writeData()
*/

/*!
    \fn qint64 QIODevice::writeData(const char *data, qint64 maxSize)

    Writes up to \a maxSize bytes from \a data to the device. Returns
    the number of bytes written, or -1 if an error occurred.

    This function is called by QIODevice. Reimplement this function
    when creating a subclass of QIODevice.

    When reimplementing this function it is important that this function
    writes all the data available before returning. This is required in order
    for QDataStream to be able to operate on the class. QDataStream assumes
    all the information was written and therefore does not retry writing if
    there was a problem.

    \sa read(), write()
*/

/*!
  \internal
  \fn int qt_subtract_from_timeout(int timeout, int elapsed)

  Reduces the \a timeout by \a elapsed, taking into account that -1 is a
  special value for timeouts.
*/

int qt_subtract_from_timeout(int timeout, int elapsed)
{
    if (timeout == -1)
        return -1;

    timeout = timeout - elapsed;
    return timeout < 0 ? 0 : timeout;
}


#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug debug, QIODevice::OpenMode modes)
{
    debug << "OpenMode(";
    QStringList modeList;
    if (modes == QIODevice::NotOpen) {
        modeList << "NotOpen"_L1;
    } else {
        if (modes & QIODevice::ReadOnly)
            modeList << "ReadOnly"_L1;
        if (modes & QIODevice::WriteOnly)
            modeList << "WriteOnly"_L1;
        if (modes & QIODevice::Append)
            modeList << "Append"_L1;
        if (modes & QIODevice::Truncate)
            modeList << "Truncate"_L1;
        if (modes & QIODevice::Text)
            modeList << "Text"_L1;
        if (modes & QIODevice::Unbuffered)
            modeList << "Unbuffered"_L1;
    }
    std::sort(modeList.begin(), modeList.end());
    debug << modeList.join(u'|');
    debug << ')';
    return debug;
}
#endif

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qiodevice.cpp"
#endif
