// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QList>
#include "qrawvector.h"
#include <vector>

// Used as accumulator in tests:
double accumulate = 0;

QVector<double> qvector_fill_and_return_helper()
{
    QVector<double> v(million);
    for (int i = 0; i != million; ++i)
        v[i] = i;
    return v;
}

QVector<double> qrawvector_fill_and_return_helper()
{
    QRawVector<double> v(million);
    for (int i = 0; i != million; ++i)
        v[i] = i;
    return v.mutateToVector();
}

QVector<double> mixedvector_fill_and_return_helper()
{
    std::vector<double> v(million);
    for (int i = 0; i != million; ++i)
        v[i] = i;
    return QVector<double>(v.begin(), v.end());
}


std::vector<double> stdvector_fill_and_return_helper()
{
    std::vector<double> v(million);
    for (int i = 0; i != million; ++i)
        v[i] = i;
    return v;
}

const QVectorData QVectorData::shared_null = { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, false, 0 };

static inline int alignmentThreshold()
{
    // malloc on 32-bit platforms should return pointers that are 8-byte aligned or more
    // while on 64-bit platforms they should be 16-byte aligned or more
    return 2 * sizeof(void*);
}

QVectorData *QVectorData::allocate(int size, int alignment)
{
    return static_cast<QVectorData *>(alignment > alignmentThreshold() ? qMallocAligned(size, alignment) : ::malloc(size));
}

QT_BEGIN_NAMESPACE

QVectorData *QVectorData::reallocate(QVectorData *x, int newsize, int oldsize, int alignment)
{
    if (alignment > alignmentThreshold())
        return static_cast<QVectorData *>(qReallocAligned(x, newsize, oldsize, alignment));
    return static_cast<QVectorData *>(realloc(x, newsize));
}

void QVectorData::free(QVectorData *x, int alignment)
{
    if (alignment > alignmentThreshold())
        qFreeAligned(x);
    else
        ::free(x);
}

int QVectorData::grow(int sizeOfHeader, int size, int sizeOfT)
{
    return qCalculateGrowingBlockSize(size, sizeOfT, sizeOfHeader).elementCount;
}

QT_END_NAMESPACE
