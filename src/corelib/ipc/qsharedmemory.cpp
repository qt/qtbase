// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"

#include "qtipccommon_p.h"
#include "qsystemsemaphore.h"

#include <q20memory.h>
#include <qdebug.h>
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

#ifndef MAX_PATH
#  define MAX_PATH PATH_MAX
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(sharedmemory)

using namespace QtIpcCommon;
using namespace Qt::StringLiterals;

QSharedMemoryPrivate::~QSharedMemoryPrivate()
{
    destructBackend();
}

inline void QSharedMemoryPrivate::constructBackend()
{
    using namespace q20;
    visit([](auto p) { construct_at(p); });
}

inline void QSharedMemoryPrivate::destructBackend()
{
    visit([](auto p) { std::destroy_at(p); });
}

#if QT_CONFIG(systemsemaphore)
inline QNativeIpcKey QSharedMemoryPrivate::semaphoreNativeKey() const
{
    if (isIpcSupported(IpcType::SharedMemory, QNativeIpcKey::Type::Windows)
            && nativeKey.type() == QNativeIpcKey::Type::Windows) {
        // native keys are plain kernel object names, limited to MAX_PATH
        auto suffix = "_sem"_L1;
        QString semkey = nativeKey.nativeKey();
        semkey.truncate(MAX_PATH - suffix.size() - 1);
        semkey += suffix;
        return { semkey, QNativeIpcKey::Type::Windows };
    }

    // System V and POSIX keys appear to operate in different namespaces, so we
    // can just use the same native key
    return nativeKey;
}
#endif

/*!
  \class QSharedMemory
  \inmodule QtCore
  \since 4.4

  \brief The QSharedMemory class provides access to a shared memory segment.

  QSharedMemory provides access to a \l{Shared Memory}{shared memory segment}
  by multiple threads and processes. Shared memory segments are identified by a
  key, represented by \l QNativeIpcKey. A key can be created in a
  cross-platform manner by using platformSafeKey().

  One QSharedMemory object must create() the segment and this call specifies
  the size of the segment. All other processes simply attach() to the segment
  that must already exist. After either operation is successful, the
  application may call data() to obtain a pointer to the data.

  To support non-atomic operations, QSharedMemory provides API to gain
  exclusive access: you may lock the shared memory with lock() before reading
  from or writing to the shared memory, but remember to release the lock with
  unlock() after you are done.

  By default, QSharedMemory automatically destroys the shared memory segment
  when the last instance of QSharedMemory is \l{detach()}{detached} from the
  segment, and no references to the segment remain.

  For details on the key types, platform-specific limitations, and
  interoperability with older or non-Qt applications, see the \l{Native IPC
  Keys} documentation. That includes important information for sandboxed
  applications on Apple platforms, including all apps obtained via the Apple
  App Store.

  \sa {Inter-Process Communication}, QSystemSemaphore
 */

/*!
  \overload QSharedMemory()

  Constructs a shared memory object with the given \a parent. The shared memory
  object's key is not set by the constructor, so the shared memory object does
  not have an underlying shared memory segment attached. The key must be set
  with setNativeKey() before create() or attach() can be used.

  \sa setNativeKey()
 */

QSharedMemory::QSharedMemory(QObject *parent)
    : QSharedMemory(QNativeIpcKey(), parent)
{
}

/*!
  \overload

  Constructs a shared memory object with the given \a parent and with
  its key set to \a key. Because its key is set, its create() and
  attach() functions can be called.

  \sa setNativeKey(), create(), attach()
 */
QSharedMemory::QSharedMemory(const QNativeIpcKey &key, QObject *parent)
    : QObject(*new QSharedMemoryPrivate(key.type()), parent)
{
    setNativeKey(key);
}

#if QT_DEPRECATED_SINCE(6, 10)
/*!
  \deprecated

  Constructs a shared memory object with the given \a parent and with
  the legacy key set to \a key. Because its key is set, its create() and
  attach() functions can be called.

  Legacy keys are deprecated. See \l{Native IPC Keys} for more information.

  \sa setKey(), create(), attach()
 */
QSharedMemory::QSharedMemory(const QString &key, QObject *parent)
    : QSharedMemory(legacyNativeKey(key), parent)
{
}
#endif

/*!
  The destructor clears the key, which forces the shared memory object
  to \l {detach()} {detach} from its underlying shared memory
  segment. If this shared memory object is the last one connected to
  the shared memory segment, the detach() operation destroys the
  shared memory segment.

  \sa detach(), isAttached()
 */
QSharedMemory::~QSharedMemory()
{
    Q_D(QSharedMemory);
    if (isAttached())
        detach();
    d->cleanHandle();
}

#if QT_DEPRECATED_SINCE(6, 10)
/*!
  \deprecated
  \overload

  Sets the legacy \a key for this shared memory object. If \a key is the same
  as the current key, the function returns without doing anything. Otherwise,
  if the shared memory object is attached to an underlying shared memory
  segment, it will \l {detach()} {detach} from it before setting the new key.
  This function does not do an attach().

  You can call key() to retrieve the legacy key. This function is mostly the
  same as:

  \code
    shm.setNativeKey(QSharedMemory::legacyNativeKey(key));
  \endcode

  except that it enables obtaining the legacy key using key().

  \sa key(), nativeKey(), isAttached()
*/
void QSharedMemory::setKey(const QString &key)
{
    setNativeKey(legacyNativeKey(key));
}
#endif

/*!
  \since 4.8
  \fn void QSharedMemory::setNativeKey(const QString &key, QNativeIpcKey::Type type)

  Sets the native, platform specific, \a key for this shared memory object of
  type \a type (the type parameter has been available since Qt 6.6). If \a key
  is the same as the current native key, the function returns without doing
  anything. Otherwise, if the shared memory object is attached to an underlying
  shared memory segment, it will \l {detach()} {detach} from it before setting
  the new key. This function does not do an attach().

  This function is useful if the native key was shared from another process,
  though the application must take care to ensure the key type matches what the
  other process expects. See \l{Native IPC Keys} for more information.

  Portable native keys can be obtained using platformSafeKey().

  You can call nativeKey() to retrieve the native key.

  \sa nativeKey(), nativeIpcKey(), isAttached()
*/

/*!
  \since 6.6

  Sets the native, platform specific, \a key for this shared memory object. If
  \a key is the same as the current native key, the function returns without
  doing anything. Otherwise, if the shared memory object is attached to an
  underlying shared memory segment, it will \l {detach()} {detach} from it
  before setting the new key. This function does not do an attach().

  This function is useful if the native key was shared from another process.
  See \l{Native IPC Keys} for more information.

  Portable native keys can be obtained using platformSafeKey().

  You can call nativeKey() to retrieve the native key.

  \sa nativeKey(), nativeIpcKey(), isAttached()
*/
void QSharedMemory::setNativeKey(const QNativeIpcKey &key)
{
    Q_D(QSharedMemory);
    if (key == d->nativeKey && key.isEmpty())
        return;
    if (!isKeyTypeSupported(key.type())) {
        d->setError(KeyError, tr("%1: unsupported key type")
                    .arg("QSharedMemory::setNativeKey"_L1));
        return;
    }

    if (isAttached())
        detach();
    d->cleanHandle();
    if (key.type() == d->nativeKey.type()) {
        // we can reuse the backend
        d->nativeKey = key;
    } else {
        // we must recreate the backend
        d->destructBackend();
        d->nativeKey = key;
        d->constructBackend();
    }
}

bool QSharedMemoryPrivate::initKey(SemaphoreAccessMode mode)
{
    if (!cleanHandle())
        return false;
#if QT_CONFIG(systemsemaphore)
    const QString legacyKey = QNativeIpcKeyPrivate::legacyKey(nativeKey);
    const QNativeIpcKey semKey = legacyKey.isEmpty()
            ? semaphoreNativeKey()
            : QSystemSemaphore::legacyNativeKey(legacyKey, nativeKey.type());
    systemSemaphore.setNativeKey(semKey, 1, mode);
    if (systemSemaphore.error() != QSystemSemaphore::NoError) {
        QString function = "QSharedMemoryPrivate::initKey"_L1;
        errorString = QSharedMemory::tr("%1: unable to set key on lock (%2)")
                .arg(function, systemSemaphore.errorString());
        switch(systemSemaphore.error()) {
        case QSystemSemaphore::PermissionDenied:
            error = QSharedMemory::PermissionDenied;
            break;
        case QSystemSemaphore::KeyError:
            error = QSharedMemory::KeyError;
            break;
        case QSystemSemaphore::AlreadyExists:
            error = QSharedMemory::AlreadyExists;
            break;
        case QSystemSemaphore::NotFound:
            error = QSharedMemory::NotFound;
            break;
        case QSystemSemaphore::OutOfResources:
            error = QSharedMemory::OutOfResources;
            break;
        case QSystemSemaphore::UnknownError:
        default:
            error = QSharedMemory::UnknownError;
            break;
        }
        return false;
    }
#else
    Q_UNUSED(mode);
#endif
    errorString = QString();
    error = QSharedMemory::NoError;
    return true;
}

#if QT_DEPRECATED_SINCE(6, 10)
/*!
  \deprecated
  Returns the legacy key assigned with setKey() to this shared memory, or a null key
  if no key has been assigned, or if the segment is using a nativeKey(). The
  key is the identifier used by Qt applications to identify the shared memory
  segment.

  You can find the native, platform specific, key used by the operating system
  by calling nativeKey().

  \sa setKey(), setNativeKey()
 */
QString QSharedMemory::key() const
{
    Q_D(const QSharedMemory);
    return QNativeIpcKeyPrivate::legacyKey(d->nativeKey);
}
#endif

/*!
  \since 4.8

  Returns the native, platform specific, key for this shared memory object. The
  native key is the identifier used by the operating system to identify the
  shared memory segment.

  You can use the native key to access shared memory segments that have not
  been created by Qt, or to grant shared memory access to non-Qt applications.
  See \l{Native IPC Keys} for more information.

  \sa setNativeKey(), nativeIpcKey()
*/
QString QSharedMemory::nativeKey() const
{
    Q_D(const QSharedMemory);
    return d->nativeKey.nativeKey();
}

/*!
  \since 6.6

  Returns the key type for this shared memory object. The key type complements
  the nativeKey() as the identifier used by the operating system to identify
  the shared memory segment.

  You can use the native key to access shared memory segments that have not
  been created by Qt, or to grant shared memory access to non-Qt applications.
  See \l{Native IPC Keys} for more information.

  \sa nativeKey(), setNativeKey()
*/
QNativeIpcKey QSharedMemory::nativeIpcKey() const
{
    Q_D(const QSharedMemory);
    return d->nativeKey;
}

/*!
  Creates a shared memory segment of \a size bytes with the key passed to the
  constructor or set with setNativeKey(), then attaches to
  the new shared memory segment with the given access \a mode and returns
  \tt true. If a shared memory segment identified by the key already exists,
  the attach operation is not performed and \tt false is returned. When the
  return value is \tt false, call error() to determine which error occurred.

  \sa error()
 */
bool QSharedMemory::create(qsizetype size, AccessMode mode)
{
    Q_D(QSharedMemory);
    QLatin1StringView function = "QSharedMemory::create"_L1;

#if QT_CONFIG(systemsemaphore)
    if (!d->initKey(QSystemSemaphore::Create))
        return false;
    QSharedMemoryLocker lock(this);
    if (!d->nativeKey.isEmpty() && !d->tryLocker(&lock, function))
        return false;
#else
    if (!d->initKey({}))
        return false;
#endif

    if (size <= 0) {
        d->error = QSharedMemory::InvalidSize;
        d->errorString =
            QSharedMemory::tr("%1: create size is less then 0").arg(function);
        return false;
    }

    if (!d->create(size))
        return false;

    return d->attach(mode);
}

/*!
  Returns the size of the attached shared memory segment. If no shared
  memory segment is attached, 0 is returned.

  \note The size of the segment may be larger than the requested size that was
  passed to create().

  \sa create(), attach()
 */
qsizetype QSharedMemory::size() const
{
    Q_D(const QSharedMemory);
    return d->size;
}

/*!
  \enum QSharedMemory::AccessMode

  \value ReadOnly The shared memory segment is read-only. Writing to
  the shared memory segment is not allowed. An attempt to write to a
  shared memory segment created with ReadOnly causes the program to
  abort.

  \value ReadWrite Reading and writing the shared memory segment are
  both allowed.
*/

/*!
  Attempts to attach the process to the shared memory segment
  identified by the key that was passed to the constructor or to a
  call to setNativeKey(). The access \a mode is \l {QSharedMemory::}
  {ReadWrite} by default. It can also be \l {QSharedMemory::}
  {ReadOnly}. Returns \c true if the attach operation is successful. If
  false is returned, call error() to determine which error occurred.
  After attaching the shared memory segment, a pointer to the shared
  memory can be obtained by calling data().

  \sa isAttached(), detach(), create()
 */
bool QSharedMemory::attach(AccessMode mode)
{
    Q_D(QSharedMemory);

    if (isAttached() || !d->initKey({}))
        return false;
#if QT_CONFIG(systemsemaphore)
    QSharedMemoryLocker lock(this);
    if (!d->nativeKey.isEmpty() && !d->tryLocker(&lock, "QSharedMemory::attach"_L1))
        return false;
#endif

    if (isAttached() || !d->handle())
        return false;

    return d->attach(mode);
}

/*!
  Returns \c true if this process is attached to the shared memory
  segment.

  \sa attach(), detach()
 */
bool QSharedMemory::isAttached() const
{
    Q_D(const QSharedMemory);
    return (nullptr != d->memory);
}

/*!
  Detaches the process from the shared memory segment. If this was the
  last process attached to the shared memory segment, then the shared
  memory segment is released by the system, i.e., the contents are
  destroyed. The function returns \c true if it detaches the shared
  memory segment. If it returns \c false, it usually means the segment
  either isn't attached, or it is locked by another process.

  \sa attach(), isAttached()
 */
bool QSharedMemory::detach()
{
    Q_D(QSharedMemory);
    if (!isAttached())
        return false;

#if QT_CONFIG(systemsemaphore)
    QSharedMemoryLocker lock(this);
    if (!d->nativeKey.isEmpty() && !d->tryLocker(&lock, "QSharedMemory::detach"_L1))
        return false;
#endif

    return d->detach();
}

/*!
  Returns a pointer to the contents of the shared memory segment, if one is
  attached. Otherwise it returns null. The value returned by this function will
  not change until a \l {detach()}{detach} happens, so it is safe to store this
  pointer.

  If the memory operations are not atomic, you may lock the shared memory with
  lock() before reading from or writing, but remember to release the lock with
  unlock() after you are done.

  \sa attach()
 */
void *QSharedMemory::data()
{
    Q_D(QSharedMemory);
    return d->memory;
}

/*!
  Returns a const pointer to the contents of the shared memory segment, if one
  is attached. Otherwise it returns null. The value returned by this function
  will not change until a \l {detach()}{detach} happens, so it is safe to store
  this pointer.

  If the memory operations are not atomic, you may lock the shared memory with
  lock() before reading from or writing, but remember to release the lock with
  unlock() after you are done.

  \sa attach(), create()
 */
const void *QSharedMemory::constData() const
{
    Q_D(const QSharedMemory);
    return d->memory;
}

/*!
  \overload data()
 */
const void *QSharedMemory::data() const
{
    Q_D(const QSharedMemory);
    return d->memory;
}

#if QT_CONFIG(systemsemaphore)
/*!
  This is a semaphore that locks the shared memory segment for access
  by this process and returns \c true. If another process has locked the
  segment, this function blocks until the lock is released. Then it
  acquires the lock and returns \c true. If this function returns \c false,
  it means that you have ignored a false return from create() or attach(),
  that you have set the key with setNativeKey() or that
  QSystemSemaphore::acquire() failed due to an unknown system error.

  \sa unlock(), data(), QSystemSemaphore::acquire()
 */
bool QSharedMemory::lock()
{
    Q_D(QSharedMemory);
    if (d->lockedByMe) {
        qWarning("QSharedMemory::lock: already locked");
        return true;
    }
    if (d->systemSemaphore.acquire()) {
        d->lockedByMe = true;
        return true;
    }
    const auto function = "QSharedMemory::lock"_L1;
    d->errorString = QSharedMemory::tr("%1: unable to lock").arg(function);
    d->error = QSharedMemory::LockError;
    return false;
}

/*!
  Releases the lock on the shared memory segment and returns \c true, if
  the lock is currently held by this process. If the segment is not
  locked, or if the lock is held by another process, nothing happens
  and false is returned.

  \sa lock()
 */
bool QSharedMemory::unlock()
{
    Q_D(QSharedMemory);
    if (!d->lockedByMe)
        return false;
    d->lockedByMe = false;
    if (d->systemSemaphore.release())
        return true;
    const auto function = "QSharedMemory::unlock"_L1;
    d->errorString = QSharedMemory::tr("%1: unable to unlock").arg(function);
    d->error = QSharedMemory::LockError;
    return false;
}
#endif // QT_CONFIG(systemsemaphore)

/*!
  \enum QSharedMemory::SharedMemoryError

  \value NoError No error occurred.

  \value PermissionDenied The operation failed because the caller
  didn't have the required permissions.

  \value InvalidSize A create operation failed because the requested
  size was invalid.

  \value KeyError The operation failed because of an invalid key.

  \value AlreadyExists A create() operation failed because a shared
  memory segment with the specified key already existed.

  \value NotFound An attach() failed because a shared memory segment
  with the specified key could not be found.

  \value LockError The attempt to lock() the shared memory segment
  failed because create() or attach() failed and returned false, or
  because a system error occurred in QSystemSemaphore::acquire().

  \value OutOfResources A create() operation failed because there was
  not enough memory available to fill the request.

  \value UnknownError Something else happened and it was bad.
*/

/*!
  Returns a value indicating whether an error occurred, and, if so,
  which error it was.

  \sa errorString()
 */
QSharedMemory::SharedMemoryError QSharedMemory::error() const
{
    Q_D(const QSharedMemory);
    return d->error;
}

/*!
  Returns a text description of the last error that occurred. If
  error() returns an \l {QSharedMemory::SharedMemoryError} {error
  value}, call this function to get a text string that describes the
  error.

  \sa error()
 */
QString QSharedMemory::errorString() const
{
    Q_D(const QSharedMemory);
    return d->errorString;
}

void QSharedMemoryPrivate::setUnixErrorString(QLatin1StringView function)
{
    // EINVAL is handled in functions so they can give better error strings
    switch (errno) {
    case EACCES:
        errorString = QSharedMemory::tr("%1: permission denied").arg(function);
        error = QSharedMemory::PermissionDenied;
        break;
    case EEXIST:
        errorString = QSharedMemory::tr("%1: already exists").arg(function);
        error = QSharedMemory::AlreadyExists;
        break;
    case ENOENT:
        errorString = QSharedMemory::tr("%1: doesn't exist").arg(function);
        error = QSharedMemory::NotFound;
        break;
    case EMFILE:
    case ENOMEM:
    case ENOSPC:
        errorString = QSharedMemory::tr("%1: out of resources").arg(function);
        error = QSharedMemory::OutOfResources;
        break;
    default:
        errorString = QSharedMemory::tr("%1: unknown error: %2")
                .arg(function, qt_error_string(errno));
        error = QSharedMemory::UnknownError;
#if defined QSHAREDMEMORY_DEBUG
        qDebug() << errorString << "key" << key << "errno" << errno << EINVAL;
#endif
    }
}

bool QSharedMemory::isKeyTypeSupported(QNativeIpcKey::Type type)
{
    if (!isIpcSupported(IpcType::SharedMemory, type))
        return false;
    using Variant = decltype(QSharedMemoryPrivate::backend);
    return Variant::staticVisit(type, [](auto ptr) {
        using Impl = std::decay_t<decltype(*ptr)>;
        return Impl::runtimeSupportCheck();
    });
}

QNativeIpcKey QSharedMemory::platformSafeKey(const QString &key, QNativeIpcKey::Type type)
{
    return QtIpcCommon::platformSafeKey(key, IpcType::SharedMemory, type);
}

QNativeIpcKey QSharedMemory::legacyNativeKey(const QString &key, QNativeIpcKey::Type type)
{
    return QtIpcCommon::legacyPlatformSafeKey(key, IpcType::SharedMemory, type);
}

#endif // QT_CONFIG(sharedmemory)

QT_END_NAMESPACE

#include "moc_qsharedmemory.cpp"
