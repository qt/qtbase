/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QDATABUFFER_P_H
#define QDATABUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "QtCore/qbytearray.h"

#include <stdlib.h>

QT_BEGIN_NAMESPACE

template <typename Type> class QDataBuffer
{
    Q_DISABLE_COPY_MOVE(QDataBuffer)
public:
    QDataBuffer(int res)
    {
        capacity = res;
        if (res) {
            QT_WARNING_PUSH
            QT_WARNING_DISABLE_GCC("-Walloc-size-larger-than=")
            buffer = (Type*) malloc(capacity * sizeof(Type));
            QT_WARNING_POP
            Q_CHECK_PTR(buffer);
        } else {
            buffer = nullptr;
        }
        siz = 0;
    }

    ~QDataBuffer()
    {
        if (buffer)
            free(buffer);
    }

    inline void reset() { siz = 0; }

    inline bool isEmpty() const { return siz==0; }

    inline int size() const { return siz; }
    inline Type *data() const { return buffer; }

    inline Type &at(int i) { Q_ASSERT(i >= 0 && i < siz); return buffer[i]; }
    inline const Type &at(int i) const { Q_ASSERT(i >= 0 && i < siz); return buffer[i]; }
    inline Type &last() { Q_ASSERT(!isEmpty()); return buffer[siz-1]; }
    inline const Type &last() const { Q_ASSERT(!isEmpty()); return buffer[siz-1]; }
    inline Type &first() { Q_ASSERT(!isEmpty()); return buffer[0]; }
    inline const Type &first() const { Q_ASSERT(!isEmpty()); return buffer[0]; }

    inline void add(const Type &t) {
        reserve(siz + 1);
        buffer[siz] = t;
        ++siz;
    }

    inline void pop_back() {
        Q_ASSERT(siz > 0);
        --siz;
    }

    inline void resize(int size) {
        reserve(size);
        siz = size;
    }

    inline void reserve(int size) {
        if (size > capacity) {
            if (capacity == 0)
                capacity = 1;
            while (capacity < size)
                capacity *= 2;
            buffer = (Type*) realloc(buffer, capacity * sizeof(Type));
            Q_CHECK_PTR(buffer);
        }
    }

    inline void shrink(int size) {
        capacity = size;
        if (size) {
            buffer = (Type*) realloc(buffer, capacity * sizeof(Type));
            Q_CHECK_PTR(buffer);
        } else {
            free(buffer);
            buffer = nullptr;
        }
    }

    inline void swap(QDataBuffer<Type> &other) {
        qSwap(capacity, other.capacity);
        qSwap(siz, other.siz);
        qSwap(buffer, other.buffer);
    }

    inline QDataBuffer &operator<<(const Type &t) { add(t); return *this; }

private:
    int capacity;
    int siz;
    Type *buffer;
};

QT_END_NAMESPACE

#endif // QDATABUFFER_P_H
