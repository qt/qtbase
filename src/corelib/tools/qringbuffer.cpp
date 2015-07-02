/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2015 Alex Trotsenko <alex1973tr@gmail.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qringbuffer_p.h"
#include "private/qbytearray_p.h"
#include <string.h>

QT_BEGIN_NAMESPACE

/*!
    \internal

    Access the bytes at a specified position the out-variable length will
    contain the amount of bytes readable from there, e.g. the amount still
    the same QByteArray
*/
const char *QRingBuffer::readPointerAtPosition(qint64 pos, qint64 &length) const
{
    if (pos >= 0) {
        pos += head;
        for (int i = 0; i < buffers.size(); ++i) {
            length = (i == tailBuffer ? tail : buffers[i].size());
            if (length > pos) {
                length -= pos;
                return buffers[i].constData() + pos;
            }
            pos -= length;
        }
    }

    length = 0;
    return 0;
}

void QRingBuffer::free(qint64 bytes)
{
    while (bytes > 0) {
        const qint64 blockSize = buffers.first().size() - head;

        if (tailBuffer == 0 || blockSize > bytes) {
            // keep a single block around if it does not exceed
            // the basic block size, to avoid repeated allocations
            // between uses of the buffer
            if (bufferSize <= bytes) {
                if (buffers.first().size() <= basicBlockSize) {
                    bufferSize = 0;
                    head = tail = 0;
                } else {
                    clear(); // try to minify/squeeze us
                }
            } else {
                Q_ASSERT(bytes < MaxByteArraySize);
                head += int(bytes);
                bufferSize -= bytes;
            }
            return;
        }

        bufferSize -= blockSize;
        bytes -= blockSize;
        buffers.removeFirst();
        --tailBuffer;
        head = 0;
    }
}

char *QRingBuffer::reserve(qint64 bytes)
{
    if (bytes <= 0 || bytes >= MaxByteArraySize)
        return 0;

    const qint64 newSize = bytes + tail;
    // if need buffer reallocation
    if (newSize > buffers.last().size()) {
        if (newSize > buffers.last().capacity() && (tail >= basicBlockSize
                || newSize >= MaxByteArraySize)) {
            // shrink this buffer to its current size
            buffers.last().resize(tail);

            // create a new QByteArray
            buffers.append(QByteArray());
            ++tailBuffer;
            tail = 0;
        }
        buffers.last().resize(qMax(basicBlockSize, tail + int(bytes)));
    }

    char *writePtr = buffers.last().data() + tail;
    bufferSize += bytes;
    Q_ASSERT(bytes < MaxByteArraySize);
    tail += int(bytes);
    return writePtr;
}

/*!
    \internal

    Allocate data at buffer head
*/
char *QRingBuffer::reserveFront(qint64 bytes)
{
    if (bytes <= 0 || bytes >= MaxByteArraySize)
        return 0;

    if (head < bytes) {
        buffers.first().remove(0, head);
        if (tailBuffer == 0)
            tail -= head;

        head = qMax(basicBlockSize, int(bytes));
        if (bufferSize == 0) {
            tail = head;
        } else {
            buffers.prepend(QByteArray());
            ++tailBuffer;
        }
        buffers.first().resize(head);
    }

    head -= int(bytes);
    bufferSize += bytes;
    return buffers.first().data() + head;
}

void QRingBuffer::chop(qint64 bytes)
{
    while (bytes > 0) {
        if (tailBuffer == 0 || tail > bytes) {
            // keep a single block around if it does not exceed
            // the basic block size, to avoid repeated allocations
            // between uses of the buffer
            if (bufferSize <= bytes) {
                if (buffers.first().size() <= basicBlockSize) {
                    bufferSize = 0;
                    head = tail = 0;
                } else {
                    clear(); // try to minify/squeeze us
                }
            } else {
                Q_ASSERT(bytes < MaxByteArraySize);
                tail -= int(bytes);
                bufferSize -= bytes;
            }
            return;
        }

        bufferSize -= tail;
        bytes -= tail;
        buffers.removeLast();
        --tailBuffer;
        tail = buffers.last().size();
    }
}

void QRingBuffer::clear()
{
    buffers.erase(buffers.begin() + 1, buffers.end());
    buffers.first().clear();

    head = tail = 0;
    tailBuffer = 0;
    bufferSize = 0;
}

qint64 QRingBuffer::indexOf(char c, qint64 maxLength) const
{
    qint64 index = 0;
    qint64 j = head;
    for (int i = 0; index < maxLength && i < buffers.size(); ++i) {
        const char *ptr = buffers[i].constData() + j;
        j = qMin(index + (i == tailBuffer ? tail : buffers[i].size()) - j, maxLength);

        while (index < j) {
            if (*ptr++ == c)
                return index;
            ++index;
        }
        j = 0;
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

    QByteArray qba(buffers.takeFirst());

    qba.reserve(0); // avoid that resizing needlessly reallocates
    if (tailBuffer == 0) {
        qba.resize(tail);
        tail = 0;
        buffers.append(QByteArray());
    } else {
        --tailBuffer;
    }
    qba.remove(0, head); // does nothing if head is 0
    head = 0;
    bufferSize -= qba.size();
    return qba;
}

/*!
    \internal

    Peek the bytes from a specified position
*/
qint64 QRingBuffer::peek(char *data, qint64 maxLength, qint64 pos) const
{
    qint64 readSoFar = 0;

    if (pos >= 0) {
        pos += head;
        for (int i = 0; readSoFar < maxLength && i < buffers.size(); ++i) {
            qint64 blockLength = (i == tailBuffer ? tail : buffers[i].size());

            if (pos < blockLength) {
                blockLength = qMin(blockLength - pos, maxLength - readSoFar);
                memcpy(data + readSoFar, buffers[i].constData() + pos, blockLength);
                readSoFar += blockLength;
                pos = 0;
            } else {
                pos -= blockLength;
            }
        }
    }

    return readSoFar;
}

/*!
    \internal

    Append a new buffer to the end
*/
void QRingBuffer::append(const QByteArray &qba)
{
    if (tail == 0) {
        buffers.last() = qba;
    } else {
        buffers.last().resize(tail);
        buffers.append(qba);
        ++tailBuffer;
    }
    tail = qba.size();
    bufferSize += tail;
}

qint64 QRingBuffer::readLine(char *data, qint64 maxLength)
{
    if (!data || --maxLength <= 0)
        return -1;

    qint64 i = indexOf('\n', maxLength);
    i = read(data, i >= 0 ? (i + 1) : maxLength);

    // Terminate it.
    data[i] = '\0';
    return i;
}

QT_END_NAMESPACE
