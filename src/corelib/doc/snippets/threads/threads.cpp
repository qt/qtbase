// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCache>
#include <QMutex>
#include <QThreadStorage>

#define Counter ReentrantCounter

//! [3]
class Counter
//! [3] //! [4]
{
public:
    Counter() { n = 0; }

    void increment() { ++n; }
    void decrement() { --n; }
    int value() const { return n; }

private:
    int n;
};
//! [4]

#undef Counter
#define Counter ThreadSafeCounter

//! [5]
class Counter
//! [5] //! [6]
{
public:
    Counter() { n = 0; }

    void increment() { QMutexLocker locker(&mutex); ++n; }
    void decrement() { QMutexLocker locker(&mutex); --n; }
    int value() const { QMutexLocker locker(&mutex); return n; }

private:
    mutable QMutex mutex;
    int n;
};
//! [6]

typedef int SomeClass;

//! [7]
QThreadStorage<QCache<QString, SomeClass> > caches;

void cacheObject(const QString &key, SomeClass *object)
//! [7] //! [8]
{
    caches.localData().insert(key, object);
}

void removeFromCache(const QString &key)
//! [8] //! [9]
{
    if (!caches.hasLocalData())
        return;

    caches.localData().remove(key);
}
//! [9]

int main()
{
    return 0;
}
