// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qthreadstorage.h"

#include "qthread.h"
#include "qthread_p.h"
#include "qmutex.h"

#include <string.h>

QT_BEGIN_NAMESPACE

// #define THREADSTORAGE_DEBUG
#ifdef THREADSTORAGE_DEBUG
#  define DEBUG_MSG qtsDebug

#  include <stdio.h>
#  include <stdarg.h>
void qtsDebug(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    fprintf(stderr, "QThreadStorage: ");
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");

    va_end(va);
}
#else
#  define DEBUG_MSG if (false)qDebug
#endif

Q_CONSTINIT static QBasicMutex destructorsMutex;
typedef QList<void (*)(void *)> DestructorMap;
Q_GLOBAL_STATIC(DestructorMap, destructors)

QThreadStorageData::QThreadStorageData(void (*func)(void *))
{
    QMutexLocker locker(&destructorsMutex);
    DestructorMap *destr = destructors();
    if (!destr) {
        /*
         the destructors vector has already been destroyed, yet a new
         QThreadStorage is being allocated. this can only happen during global
         destruction, at which point we assume that there is only one thread.
         in order to keep QThreadStorage working, we need somewhere to store
         the data, best place we have in this situation is at the tail of the
         current thread's tls vector. the destructor is ignored, since we have
         no where to store it, and no way to actually call it.
         */
        QThreadData *data = QThreadData::current();
        id = data->tls.size();
        DEBUG_MSG("QThreadStorageData: Allocated id %d, destructor %p cannot be stored", id, func);
        return;
    }
    for (id = 0; id < destr->size(); id++) {
        if (destr->at(id) == nullptr)
            break;
    }
    if (id == destr->size()) {
        destr->append(func);
    } else {
        (*destr)[id] = func;
    }
    DEBUG_MSG("QThreadStorageData: Allocated id %d, destructor %p", id, func);
}

QThreadStorageData::~QThreadStorageData()
{
    DEBUG_MSG("QThreadStorageData: Released id %d", id);
    QMutexLocker locker(&destructorsMutex);
    if (destructors())
        (*destructors())[id] = nullptr;
}

void **QThreadStorageData::get() const
{
    QThreadData *data = QThreadData::current();
    if (!data) {
        qWarning("QThreadStorage::get: QThreadStorage can only be used with threads started with QThread");
        return nullptr;
    }
    QList<void *> &tls = data->tls;
    if (tls.size() <= id)
        tls.resize(id + 1);
    void **v = &tls[id];

    DEBUG_MSG("QThreadStorageData: Returning storage %d, data %p, for thread %p",
          id,
          *v,
          data->thread.loadRelaxed());

    return *v ? v : nullptr;
}

void **QThreadStorageData::set(void *p)
{
    QThreadData *data = QThreadData::current();
    if (!data) {
        qWarning("QThreadStorage::set: QThreadStorage can only be used with threads started with QThread");
        return nullptr;
    }
    QList<void *> &tls = data->tls;
    if (tls.size() <= id)
        tls.resize(id + 1);

    void *&value = tls[id];
    // delete any previous data
    if (value != nullptr) {
        DEBUG_MSG("QThreadStorageData: Deleting previous storage %d, data %p, for thread %p",
                id,
                value,
                data->thread.loadRelaxed());

        QMutexLocker locker(&destructorsMutex);
        DestructorMap *destr = destructors();
        void (*destructor)(void *) = destr ? destr->value(id) : nullptr;
        locker.unlock();

        void *q = value;
        value = nullptr;

        if (destructor)
            destructor(q);
    }

    // store new data
    value = p;
    DEBUG_MSG("QThreadStorageData: Set storage %d for thread %p to %p", id, data->thread.loadRelaxed(), p);
    return &value;
}

void QThreadStorageData::finish(void **p)
{
    QList<void *> *tls = reinterpret_cast<QList<void *> *>(p);
    if (!tls || tls->isEmpty() || !destructors())
        return; // nothing to do

    DEBUG_MSG("QThreadStorageData: Destroying storage for thread %p", QThread::currentThread());
    while (!tls->isEmpty()) {
        void *&value = tls->last();
        void *q = value;
        value = nullptr;
        int i = tls->size() - 1;
        tls->resize(i);

        if (!q) {
            // data already deleted
            continue;
        }

        QMutexLocker locker(&destructorsMutex);
        void (*destructor)(void *) = destructors()->value(i);
        locker.unlock();

        if (!destructor) {
            if (QThread::currentThread())
                qWarning("QThreadStorage: Thread %p exited after QThreadStorage %d destroyed",
                         QThread::currentThread(), i);
            continue;
        }
        destructor(q); //crash here might mean the thread exited after qthreadstorage was destroyed

        if (tls->size() > i) {
            //re reset the tls in case it has been recreated by its own destructor.
            (*tls)[i] = nullptr;
        }
    }
    tls->clear();
}

/*!
    \class QThreadStorage
    \inmodule QtCore
    \brief The QThreadStorage class provides per-thread data storage.

    \threadsafe

    \ingroup thread

    QThreadStorage is a template class that provides per-thread data
    storage.

    The setLocalData() function stores a single thread-specific value
    for the calling thread. The data can be accessed later using
    localData().

    The hasLocalData() function allows the programmer to determine if
    data has previously been set using the setLocalData() function.
    This is also useful for lazy initialization.

    If T is a pointer type, QThreadStorage takes ownership of the data
    (which must be created on the heap with \c new) and deletes it when
    the thread exits, either normally or via termination.

    For example, the following code uses QThreadStorage to store a
    single cache for each thread that calls the cacheObject() and
    removeFromCache() functions. The cache is automatically
    deleted when the calling thread exits.

    \snippet threads/threads.cpp 7
    \snippet threads/threads.cpp 8
    \snippet threads/threads.cpp 9

    \section1 Caveats

    \list

    \li The QThreadStorage destructor does not delete per-thread data.
    QThreadStorage only deletes per-thread data when the thread exits
    or when setLocalData() is called multiple times.

    \li QThreadStorage can be used to store data for the \c main()
    thread. QThreadStorage deletes all data set for the \c main()
    thread when QApplication is destroyed, regardless of whether or
    not the \c main() thread has actually finished.

    \endlist

    \sa QThread
*/

/*!
    \fn template <class T> QThreadStorage<T>::QThreadStorage()

    Constructs a new per-thread data storage object.
*/

/*!
    \fn template <class T> QThreadStorage<T>::~QThreadStorage()

    Destroys the per-thread data storage object.

    Note: The per-thread data stored is not deleted. Any data left
    in QThreadStorage is leaked. Make sure that all threads using
    QThreadStorage have exited before deleting the QThreadStorage.

    \sa hasLocalData()
*/

/*!
    \fn template <class T> bool QThreadStorage<T>::hasLocalData() const

    If T is a pointer type, returns \c true if the calling thread has
    non-zero data available.

    If T is a value type, returns whether the data has already been
    constructed by calling setLocalData or localData.

    \sa localData()
*/

/*!
    \fn template <class T> T &QThreadStorage<T>::localData()

    Returns a reference to the data that was set by the calling
    thread.

    If no data has been set, this will create a default constructed
    instance of type T.

    \sa hasLocalData()
*/

/*!
    \fn template <class T> const T QThreadStorage<T>::localData() const
    \overload

    Returns a copy of the data that was set by the calling thread.

    \sa hasLocalData()
*/

/*!
    \fn template <class T> void QThreadStorage<T>::setLocalData(T data)

    Sets the local data for the calling thread to \a data. It can be
    accessed later using the localData() functions.

    If T is a pointer type, QThreadStorage takes ownership of the data
    and deletes it automatically either when the thread exits (either
    normally or via termination) or when setLocalData() is called again.

    \sa localData(), hasLocalData()
*/

QT_END_NAMESPACE
