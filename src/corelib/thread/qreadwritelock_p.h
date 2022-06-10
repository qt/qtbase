// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QREADWRITELOCK_P_H
#define QREADWRITELOCK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the implementation.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/private/qwaitcondition_p.h>
#include <QtCore/qvarlengtharray.h>

QT_REQUIRE_CONFIG(thread);

QT_BEGIN_NAMESPACE

class QReadWriteLockPrivate
{
public:
    explicit QReadWriteLockPrivate(bool isRecursive = false)
        : recursive(isRecursive) {}

    QtPrivate::mutex mutex;
    QtPrivate::condition_variable writerCond;
    QtPrivate::condition_variable readerCond;
    int readerCount = 0;
    int writerCount = 0;
    int waitingReaders = 0;
    int waitingWriters = 0;
    const bool recursive;

    //Called with the mutex locked
    bool lockForWrite(std::unique_lock<QtPrivate::mutex> &lock, int timeout);
    bool lockForRead(std::unique_lock<QtPrivate::mutex> &lock, int timeout);
    void unlock();

    //memory management
    int id = 0;
    void release();
    static QReadWriteLockPrivate *allocate();

    // Recursive mutex handling
    Qt::HANDLE currentWriter = {};

    struct Reader {
        Qt::HANDLE handle;
        int recursionLevel;
    };

    QVarLengthArray<Reader, 16> currentReaders;

    // called with the mutex unlocked
    bool recursiveLockForWrite(int timeout);
    bool recursiveLockForRead(int timeout);
    void recursiveUnlock();
};
Q_DECLARE_TYPEINFO(QReadWriteLockPrivate::Reader, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QREADWRITELOCK_P_H
