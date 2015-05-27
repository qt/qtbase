/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef QRINGBUFFER_P_H
#define QRINGBUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class QRingBuffer
{
public:
    explicit inline QRingBuffer(int growth = 4096) :
        head(0), tail(0), tailBuffer(0), basicBlockSize(growth), bufferSize(0) {
        buffers.append(QByteArray());
    }

    inline qint64 nextDataBlockSize() const {
        return (tailBuffer == 0 ? tail : buffers.first().size()) - head;
    }

    inline const char *readPointer() const {
        return bufferSize == 0 ? Q_NULLPTR : (buffers.first().constData() + head);
    }

    Q_CORE_EXPORT const char *readPointerAtPosition(qint64 pos, qint64 &length) const;
    Q_CORE_EXPORT void free(qint64 bytes);
    Q_CORE_EXPORT char *reserve(qint64 bytes);
    Q_CORE_EXPORT char *reserveFront(qint64 bytes);

    inline void truncate(qint64 pos) {
        if (pos < size())
            chop(size() - pos);
    }

    Q_CORE_EXPORT void chop(qint64 bytes);

    inline bool isEmpty() const {
        return bufferSize == 0;
    }

    inline int getChar() {
        if (isEmpty())
            return -1;
        char c = *readPointer();
        free(1);
        return int(uchar(c));
    }

    inline void putChar(char c) {
        char *ptr = reserve(1);
        *ptr = c;
    }

    void ungetChar(char c)
    {
        if (head > 0) {
            --head;
            buffers.first()[head] = c;
            ++bufferSize;
        } else {
            char *ptr = reserveFront(1);
            *ptr = c;
        }
    }


    inline qint64 size() const {
        return bufferSize;
    }

    Q_CORE_EXPORT void clear();
    inline qint64 indexOf(char c) const { return indexOf(c, size()); }
    Q_CORE_EXPORT qint64 indexOf(char c, qint64 maxLength) const;
    Q_CORE_EXPORT qint64 read(char *data, qint64 maxLength);
    Q_CORE_EXPORT QByteArray read();
    Q_CORE_EXPORT qint64 peek(char *data, qint64 maxLength, qint64 pos = 0) const;
    Q_CORE_EXPORT void append(const QByteArray &qba);

    inline qint64 skip(qint64 length) {
        return read(0, length);
    }

    Q_CORE_EXPORT qint64 readLine(char *data, qint64 maxLength);

    inline bool canReadLine() const {
        return indexOf('\n') >= 0;
    }

private:
    QList<QByteArray> buffers;
    int head, tail;
    int tailBuffer; // always buffers.size() - 1
    const int basicBlockSize;
    qint64 bufferSize;
};

QT_END_NAMESPACE

#endif // QRINGBUFFER_P_H
