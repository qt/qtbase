/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2015 Alex Trotsenko <alex1973tr@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "private/qringbuffer_p.h"
#include "private/qbytearray_p.h"
#include <string.h>

QT_BEGIN_NAMESPACE

void QRingChunk::allocate(int alloc)
{
    Q_ASSERT(alloc > 0 && size() == 0);

    if (chunk.size() < alloc || isShared())
        chunk = QByteArray(alloc, Qt::Uninitialized);
}

void QRingChunk::detach()
{
    Q_ASSERT(isShared());

    const int chunkSize = size();
    QByteArray x(chunkSize, Qt::Uninitialized);
    ::memcpy(x.data(), chunk.constData() + headOffset, chunkSize);
    chunk = std::move(x);
    headOffset = 0;
    tailOffset = chunkSize;
}

QByteArray QRingChunk::toByteArray()
{
    if (headOffset != 0 || tailOffset != chunk.size()) {
        if (isShared())
            return chunk.mid(headOffset, size());

        if (headOffset != 0) {
            char *ptr = chunk.data();
            ::memmove(ptr, ptr + headOffset, size());
            tailOffset -= headOffset;
            headOffset = 0;
        }

        chunk.reserve(0); // avoid that resizing needlessly reallocates
        chunk.resize(tailOffset);
    }

    return chunk;
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
                Q_ASSERT(bytes < MaxByteArraySize);
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
    Q_ASSERT(bytes > 0 && bytes < MaxByteArraySize);

    const int chunkSize = qMax(basicBlockSize, int(bytes));
    int tail = 0;
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
    Q_ASSERT(bytes > 0 && bytes < MaxByteArraySize);

    const int chunkSize = qMax(basicBlockSize, int(bytes));
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
        const qint64 chunkSize = buffers.constLast().size();

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
                Q_ASSERT(bytes < MaxByteArraySize);
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

qint64 QRingBuffer::readLine(char *data, qint64 maxLength)
{
    Q_ASSERT(data != nullptr && maxLength > 1);

    --maxLength;
    qint64 i = indexOf('\n', maxLength);
    i = read(data, i >= 0 ? (i + 1) : maxLength);

    // Terminate it.
    data[i] = '\0';
    return i;
}

QT_END_NAMESPACE
