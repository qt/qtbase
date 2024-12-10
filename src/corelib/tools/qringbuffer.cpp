// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2015 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qringbuffer_p.h"

#include <type_traits>

#include <string.h>

QT_BEGIN_NAMESPACE

static_assert(std::is_nothrow_default_constructible_v<QRingChunk>);
static_assert(std::is_nothrow_move_constructible_v<QRingChunk>);
static_assert(std::is_nothrow_move_assignable_v<QRingChunk>);

void QRingChunk::allocate(qsizetype alloc)
{
    Q_ASSERT(alloc > 0 && size() == 0);

    if (chunk.size() < alloc || isShared())
        chunk = QByteArray(alloc, Qt::Uninitialized);
}

void QRingChunk::detach()
{
    Q_ASSERT(isShared());

    const qsizetype chunkSize = size();
    chunk = QByteArray(std::as_const(*this).data(), chunkSize);
    headOffset = 0;
    tailOffset = chunkSize;
}

QByteArray QRingChunk::toByteArray() &&
{
    // ### Replace with std::move(chunk).sliced(head(), size()) once sliced()&& is available
    if (headOffset != 0 || tailOffset != chunk.size()) {
        if (isShared())
            return chunk.sliced(head(), size());

        chunk.resize(tailOffset);
        chunk.remove(0, headOffset);
    }

    return std::move(chunk);
}

/*!
    \internal

    Access the bytes at a specified position the out-variable length will
    contain the amount of bytes readable from there, e.g. the amount still
    the same QByteArray
*/
const char *QRingBuffer::readPointerAtPosition(qint64 pos, qint64 &length) const
{
    Q_ASSERT(pos >= 0);

    for (const QRingChunk &chunk : buffers) {
        length = chunk.size();
        if (length > pos) {
            length -= pos;
            return chunk.data() + pos;
        }
        pos -= length;
    }

    length = 0;
    return nullptr;
}

void QRingBuffer::free(qint64 bytes)
{
    Q_ASSERT(bytes <= bufferSize);

    while (bytes > 0) {
        const qint64 chunkSize = buffers.constFirst().size();

        if (buffers.size() == 1 || chunkSize > bytes) {
            QRingChunk &chunk = buffers.first();
            // keep a single block around if it does not exceed
            // the basic block size, to avoid repeated allocations
            // between uses of the buffer
            if (bufferSize == bytes) {
                if (chunk.capacity() <= basicBlockSize && !chunk.isShared()) {
                    chunk.reset();
                    bufferSize = 0;
                } else {
                    clear(); // try to minify/squeeze us
                }
            } else {
                Q_ASSERT(bytes < QByteArray::maxSize());
                chunk.advance(bytes);
                bufferSize -= bytes;
            }
            return;
        }

        bufferSize -= chunkSize;
        bytes -= chunkSize;
        buffers.removeFirst();
    }
}

char *QRingBuffer::reserve(qint64 bytes)
{
    Q_ASSERT(bytes > 0 && bytes < QByteArray::maxSize());

    const qsizetype chunkSize = qMax(qint64(basicBlockSize), bytes);
    qsizetype tail = 0;
    if (bufferSize == 0) {
        if (buffers.isEmpty())
            buffers.append(QRingChunk(chunkSize));
        else
            buffers.first().allocate(chunkSize);
    } else {
        const QRingChunk &chunk = buffers.constLast();
        // if need a new buffer
        if (basicBlockSize == 0 || chunk.isShared() || bytes > chunk.available())
            buffers.append(QRingChunk(chunkSize));
        else
            tail = chunk.size();
    }

    buffers.last().grow(bytes);
    bufferSize += bytes;
    return buffers.last().data() + tail;
}

/*!
    \internal

    Allocate data at buffer head
*/
char *QRingBuffer::reserveFront(qint64 bytes)
{
    Q_ASSERT(bytes > 0 && bytes < QByteArray::maxSize());

    const qsizetype chunkSize = qMax(qint64(basicBlockSize), bytes);
    if (bufferSize == 0) {
        if (buffers.isEmpty())
            buffers.prepend(QRingChunk(chunkSize));
        else
            buffers.first().allocate(chunkSize);
        buffers.first().grow(chunkSize);
        buffers.first().advance(chunkSize - bytes);
    } else {
        const QRingChunk &chunk = buffers.constFirst();
        // if need a new buffer
        if (basicBlockSize == 0 || chunk.isShared() || bytes > chunk.head()) {
            buffers.prepend(QRingChunk(chunkSize));
            buffers.first().grow(chunkSize);
            buffers.first().advance(chunkSize - bytes);
        } else {
            buffers.first().advance(-bytes);
        }
    }

    bufferSize += bytes;
    return buffers.first().data();
}

void QRingBuffer::chop(qint64 bytes)
{
    Q_ASSERT(bytes <= bufferSize);

    while (bytes > 0) {
        const qsizetype chunkSize = buffers.constLast().size();

        if (buffers.size() == 1 || chunkSize > bytes) {
            QRingChunk &chunk = buffers.last();
            // keep a single block around if it does not exceed
            // the basic block size, to avoid repeated allocations
            // between uses of the buffer
            if (bufferSize == bytes) {
                if (chunk.capacity() <= basicBlockSize && !chunk.isShared()) {
                    chunk.reset();
                    bufferSize = 0;
                } else {
                    clear(); // try to minify/squeeze us
                }
            } else {
                Q_ASSERT(bytes < QByteArray::maxSize());
                chunk.grow(-bytes);
                bufferSize -= bytes;
            }
            return;
        }

        bufferSize -= chunkSize;
        bytes -= chunkSize;
        buffers.removeLast();
    }
}

void QRingBuffer::clear()
{
    if (buffers.isEmpty())
        return;

    buffers.erase(buffers.begin() + 1, buffers.end());
    buffers.first().clear();
    bufferSize = 0;
}

qint64 QRingBuffer::indexOf(char c, qint64 maxLength, qint64 pos) const
{
    Q_ASSERT(maxLength >= 0 && pos >= 0);

    if (maxLength == 0)
        return -1;

    qint64 index = -pos;
    for (const QRingChunk &chunk : buffers) {
        const qint64 nextBlockIndex = qMin(index + chunk.size(), maxLength);

        if (nextBlockIndex > 0) {
            const char *ptr = chunk.data();
            if (index < 0) {
                ptr -= index;
                index = 0;
            }

            const char *findPtr = reinterpret_cast<const char *>(memchr(ptr, c,
                                                                        nextBlockIndex - index));
            if (findPtr)
                return qint64(findPtr - ptr) + index + pos;

            if (nextBlockIndex == maxLength)
                return -1;
        }
        index = nextBlockIndex;
    }
    return -1;
}

qint64 QRingBuffer::read(char *data, qint64 maxLength)
{
    const qint64 bytesToRead = qMin(size(), maxLength);
    qint64 readSoFar = 0;
    while (readSoFar < bytesToRead) {
        const qint64 bytesToReadFromThisBlock = qMin(bytesToRead - readSoFar,
                                                     nextDataBlockSize());
        if (data)
            memcpy(data + readSoFar, readPointer(), bytesToReadFromThisBlock);
        readSoFar += bytesToReadFromThisBlock;
        free(bytesToReadFromThisBlock);
    }
    return readSoFar;
}

/*!
    \internal

    Read an unspecified amount (will read the first buffer)
*/
QByteArray QRingBuffer::read()
{
    if (bufferSize == 0)
        return QByteArray();

    bufferSize -= buffers.constFirst().size();
    return buffers.takeFirst().toByteArray();
}

/*!
    \internal

    Peek the bytes from a specified position
*/
qint64 QRingBuffer::peek(char *data, qint64 maxLength, qint64 pos) const
{
    Q_ASSERT(maxLength >= 0 && pos >= 0);

    qint64 readSoFar = 0;
    for (const QRingChunk &chunk : buffers) {
        if (readSoFar == maxLength)
            break;

        qint64 blockLength = chunk.size();
        if (pos < blockLength) {
            blockLength = qMin(blockLength - pos, maxLength - readSoFar);
            memcpy(data + readSoFar, chunk.data() + pos, blockLength);
            readSoFar += blockLength;
            pos = 0;
        } else {
            pos -= blockLength;
        }
    }

    return readSoFar;
}

/*!
    \internal

    Append bytes from data to the end
*/
void QRingBuffer::append(const char *data, qint64 size)
{
    Q_ASSERT(size >= 0);

    if (size == 0)
        return;

    char *writePointer = reserve(size);
    if (size == 1)
        *writePointer = *data;
    else
        ::memcpy(writePointer, data, size);
}

/*!
    \internal

    Append a new buffer to the end
*/
void QRingBuffer::append(const QByteArray &qba)
{
    if (bufferSize != 0 || buffers.isEmpty())
        buffers.append(QRingChunk(qba));
    else
        buffers.last().assign(qba);
    bufferSize += qba.size();
}

/*!
    \internal

    Append a new buffer to the end
*/
void QRingBuffer::append(QByteArray &&qba)
{
    const auto qbaSize = qba.size();
    if (bufferSize != 0 || buffers.isEmpty())
        buffers.emplace_back(std::move(qba));
    else
        buffers.last().assign(std::move(qba));
    bufferSize += qbaSize;
}

qint64 QRingBuffer::readLineWithoutTerminatingNull(char *data, qint64 maxLength)
{
    Q_ASSERT(data != nullptr && maxLength > 0);

    qint64 i = indexOf('\n', maxLength);
    i = read(data, i >= 0 ? (i + 1) : maxLength);

    return i;
}

qint64 QRingBuffer::readLine(char *data, qint64 maxLength)
{
    Q_ASSERT(maxLength > 1);

    qint64 i = readLineWithoutTerminatingNull(data, maxLength - 1);

    // Terminate it.
    data[i] = '\0';
    return i;
}

QT_END_NAMESPACE
