/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#include <QVector>
#include <vector>
#include "qrawvector.h"

const int N = 1000000;
double s = 0;

QVector<double> qvector_fill_and_return_helper()
{
    QVector<double> v(N);
    for (int i = 0; i != N; ++i)
        v[i] = i;
    return v;
}

QVector<double> qrawvector_fill_and_return_helper()
{
    QRawVector<double> v(N);
    for (int i = 0; i != N; ++i)
        v[i] = i;
    return v.mutateToVector();
}

QVector<double> mixedvector_fill_and_return_helper()
{
    std::vector<double> v(N);
    for (int i = 0; i != N; ++i)
        v[i] = i;
    return QVector<double>::fromStdVector(v);
}


std::vector<double> stdvector_fill_and_return_helper()
{
    std::vector<double> v(N);
    for (int i = 0; i != N; ++i)
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
    return qAllocMore(size * sizeOfT, sizeOfHeader) / sizeOfT;
}
