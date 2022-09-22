// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include "qsystemsemaphore.h"
#include <qdir.h>
#include <qcryptographichash.h>
#include <qdebug.h>
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

#if defined(Q_OS_DARWIN)
#  include "qcore_mac_p.h"
#  if !defined(SHM_NAME_MAX)
     // Based on PSEMNAMLEN in XNU's posix_sem.c, which would
     // indicate the max length is 31, _excluding_ the zero
     // terminator. But in practice (possibly due to an off-
     // by-one bug in the kernel) the usable bytes are only 30.
#    define SHM_NAME_MAX 30
#  endif
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)
/*!
    \internal

    Generate a string from the key which can be any unicode string into
    the subset that the win/unix kernel allows.

    On Unix this will be a file name
  */
QString
QSharedMemoryPrivate::makePlatformSafeKey(const QString &key,
                                          const QString &prefix)
{
    if (key.isEmpty())
        return QString();

    QByteArray hex = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex();

#if defined(Q_OS_DARWIN) && defined(QT_POSIX_IPC)
    if (qt_apple_isSandboxed()) {
        // Sandboxed applications on Apple platforms require the shared memory name
        // to be in the form <application group identifier>/<custom identifier>.
        // Since we don't know which application group identifier the user wants
        // to apply, we instead document that requirement, and use the key directly.
        return key;
    } else {
        // The shared memory name limit on Apple platforms is very low (30 characters),
        // so we can't use the logic below of combining the prefix, key, and a hash,
        // to ensure a unique and valid name. Instead we use the first part of the
        // hash, which should still long enough to avoid collisions in practice.
        return u'/' + hex.left(SHM_NAME_MAX - 1);
    }
#endif

    QString result = prefix;
    for (QChar ch : key) {
        if ((ch >= u'a' && ch <= u'z') ||
           (ch >= u'A' && ch <= u'Z'))
           result += ch;
    }
    result.append(QLatin1StringView(hex));

#ifdef Q_OS_WIN
    return result;
#elif defined(QT_POSIX_IPC)
    return u'/' + result;
#else
    return QDir::tempPath() + u'/' + result;
#endif
}
#endif // QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)

#if QT_CONFIG(sharedmemory)

/*!
  \class QSharedMemory
  \inmodule QtCore
  \since 4.4

  \brief The QSharedMemory class provides access to a shared memory segment.

  QSharedMemory provides access to a shared memory segment by multiple
  threads and processes. It also provides a way for a single thread or
  process to lock the memory for exclusive access.

  When using this class, be aware of the following platform
  differences:

  \list

    \li Windows: QSharedMemory does not "own" the shared memory segment.
    When all threads or processes that have an instance of QSharedMemory
    attached to a particular shared memory segment have either destroyed
    their instance of QSharedMemory or exited, the Windows kernel
    releases the shared memory segment automatically.

    \li Unix: QSharedMemory "owns" the shared memory segment. When the
    last thread or process that has an instance of QSharedMemory
    attached to a particular shared memory segment detaches from the
    segment by destroying its instance of QSharedMemory, the destructor
    releases the shared memory segment. But if that last thread or
    process crashes without running the QSharedMemory destructor, the
    shared memory segment survives the crash.

    \li Unix: QSharedMemory can be implemented by one of two different
    backends, selected at Qt build time: System V or POSIX. Qt defaults to
    using the System V API if it is available, and POSIX if not. These two
    backends do not interoperate, so two applications must ensure they use the
    same one, even if the native key (see setNativeKey()) is the same.

    The POSIX backend can be explicitly selected using the
    \c{-feature-ipc_posix} option to the Qt configure script. If it is enabled,
    the \c{QT_POSIX_IPC} macro will be defined.

    \li Sandboxed applications on Apple platforms (including apps
    shipped through the Apple App Store): This environment requires
    the use of POSIX shared memory (instead of System V shared memory).

    Qt for iOS is built with support for POSIX shared memory out of the box.
    However, Qt for \macos builds (including those from the Qt installer) default
    to System V, making them unsuitable for App Store submission if QSharedMemory
    is needed. See above for instructions to explicitly select the POSIX backend
    when building Qt.

    In addition, in a sandboxed environment, the following caveats apply:

    \list
      \li The key must be in the form \c {<application group identifier>/<custom identifier>},
      as documented \l {https://developer.apple.com/library/archive/documentation/Security/Conceptual/AppSandboxDesignGuide/AppSandboxInDepth/AppSandboxInDepth.html#//apple_ref/doc/uid/TP40011183-CH3-SW24}
      {here} and \l {https://developer.apple.com/documentation/bundleresources/entitlements/com_apple_security_application-groups}
      {here}.

      \li The key length is limited to 30 characters.

      \li On process exit, the named shared memory entries are not
      cleaned up, so restarting the application and re-creating the
      shared memory under the same name will fail. To work around this,
      fall back to attaching to the existing shared memory entry:

      \code

          QSharedMemory shm("DEVTEAMID.app-group/shared");
          if (!shm.create(42) && shm.error() == QSharedMemory::AlreadyExists)
              shm.attach();

      \endcode

    \endlist

    \li Android: QSharedMemory is not supported.

  \endlist

  Remember to lock the shared memory with lock() before reading from
  or writing to the shared memory, and remember to release the lock
  with unlock() after you are done.

  QSharedMemory automatically destroys the shared memory segment when
  the last instance of QSharedMemory is detached from the segment, and
  no references to the segment remain.

  \warning QSharedMemory changes the key in a Qt-specific way, unless otherwise
  specified. Interoperation with non-Qt applications is achieved by first creating
  a default shared memory with QSharedMemory() and then setting a native key with
  setNativeKey(), after ensuring they use the same low-level API (System V or
  POSIX). When using native keys, shared memory is not protected against multiple
  accesses on it (for example, unable to lock()) and a user-defined mechanism
  should be used to achieve such protection.

  \section2 Alternative: Memory-Mapped File

  Another way to share memory between processes is by opening the same file
  using \l QFile and mapping it into memory using QFile::map() (without
  specifying the QFileDevice::MapPrivateOption option). Any writes to the
  mapped segment will be observed by all other processes that have mapped the
  same file. This solution has the major advantages of being independent of the
  backend API and of being simpler to interoperate with from non-Qt
  applications. And since \l{QTemporaryFile} is a \l{QFile}, applications can
  use that class to achieve clean-up semantics and to create unique shared
  memory segments too.

  To achieve locking of the shared memory segment, applications will need to
  deploy their own mechanisms. This can be achieved by using \l
  QBasicAtomicInteger or \c{std::atomic} in a pre-determined offset in the
  segment itself. Higher-level locking primitives may be available on some
  operating systems; for example, on Linux, \c{pthread_mutex_create()} can be
  passed a flag to indicate that the mutex resides in a shared memory segment.

  A major drawback of using file-backed shared memory is that the operating
  system will attempt to write the data to permanent storage, possibly causing
  noticeable performance penalties. To avoid this, applications should locate a
  RAM-backed filesystem, such as \c{tmpfs} on Linux (see
  QStorageInfo::fileSystemType()), or pass a flag to the native file-opening
  function to inform the OS to avoid committing the contents to storage.

  File-backed shared memory must be used with care if another process
  participating is untrusted. The files may be truncated/shrunk and cause
  applications accessing memory beyond the file's size to crash.

  \section3 Linux hints on memory-mapped files

  On modern Linux systems, while the \c{/tmp} directory is often a \c{tmpfs}
  mount point, that is not a requirement. However, the \c{/dev/shm} directory
  is required to be a \c{tmpfs} and exists for this very purpose. Do note that
  it is world-readable and writable (like \c{/tmp} and \c{/var/tmp}), so one
  must be careful of the contents revealed there. Another alternative is to use
  the XDG Runtime Directory (see QStandardPaths::writableLocation() and
  \l{QStandardPaths::RuntimeLocation}), which on Linux systems using systemd is
  a user-specific \c{tmpfs}.

  An even more secure solution is to create a "memfd" using \c{memfd_create(2)}
  and use interprocess communication to pass the file descriptor, like
  \l{QDBusUnixFileDescriptor} or by letting the child process of a \l{QProcess}
  inherit it. "memfds" can also be sealed against being shrunk, so they are
  safe to be used when communicating with processes with a different privilege
  level.

  \section3 FreeBSD hints on memory-mapped files

  FreeBSD also has \c{memfd_create(2)} and can pass file descriptors to other
  processes using the same techniques as Linux. It does not have temporary
  filesystems mounted by default.

  \section3 Windows hints on memory-mapped files

  On Windows, the application can request the operating system avoid committing
  the file's contents to permanent storage. This request is performed by
  passing the \c{FILE_ATTRIBUTE_TEMPORARY} flag in the \c{dwFlagsAndAttributes}
  \c{CreateFile} Win32 function, the \c{_O_SHORT_LIVED} flag to \c{_open()}
  low-level function, or by including the modifier "T" to the \c{fopen()} C
  runtime function.

  There's also a flag to inform the operating system to delete the file when
  the last handle to it is closed (\c{FILE_FLAG_DELETE_ON_CLOSE},
  \c{_O_TEMPORARY}, and the "D" modifier), but do note that all processes
  attempting to open the file must agree on using this flag or not using it. A
  mismatch will likely cause a sharing violation and failure to open the file.
 */

/*!
  \overload QSharedMemory()

  Constructs a shared memory object with the given \a parent.  The
  shared memory object's key is not set by the constructor, so the
  shared memory object does not have an underlying shared memory
  segment attached. The key must be set with setKey() or setNativeKey()
  before create() or attach() can be used.

  \sa setKey()
 */

QSharedMemory::QSharedMemory(QObject *parent)
  : QObject(*new QSharedMemoryPrivate, parent)
{
}

/*!
  Constructs a shared memory object with the given \a parent and with
  its key set to \a key. Because its key is set, its create() and
  attach() functions can be called.

  \sa setKey(), create(), attach()
 */
QSharedMemory::QSharedMemory(const QString &key, QObject *parent)
    : QObject(*new QSharedMemoryPrivate, parent)
{
    setKey(key);
}

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
    setKey(QString());
}

/*!
  Sets the platform independent \a key for this shared memory object. If \a key
  is the same as the current key, the function returns without doing anything.

  You can call key() to retrieve the platform independent key. Internally,
  QSharedMemory converts this key into a platform specific key. If you instead
  call nativeKey(), you will get the platform specific, converted key.

  If the shared memory object is attached to an underlying shared memory
  segment, it will \l {detach()} {detach} from it before setting the new key.
  This function does not do an attach().

  \sa key(), nativeKey(), isAttached()
*/
void QSharedMemory::setKey(const QString &key)
{
    Q_D(QSharedMemory);
    if (key == d->key && d->makePlatformSafeKey(key) == d->nativeKey)
        return;

    if (isAttached())
        detach();
    d->cleanHandle();
    d->key = key;
    d->nativeKey = d->makePlatformSafeKey(key);
}

/*!
  \since 4.8

  Sets the native, platform specific, \a key for this shared memory object. If
  \a key is the same as the current native key, the function returns without
  doing anything. If all you want is to assign a key to a segment, you should
  call setKey() instead.

  You can call nativeKey() to retrieve the native key. If a native key has been
  assigned, calling key() will return a null string.

  If the shared memory object is attached to an underlying shared memory
  segment, it will \l {detach()} {detach} from it before setting the new key.
  This function does not do an attach().

  The application will not be portable if you set a native key.

  \sa nativeKey(), key(), isAttached()
*/
void QSharedMemory::setNativeKey(const QString &key)
{
    Q_D(QSharedMemory);
    if (key == d->nativeKey && d->key.isNull())
        return;

    if (isAttached())
        detach();
    d->cleanHandle();
    d->key = QString();
    d->nativeKey = key;
}

bool QSharedMemoryPrivate::initKey()
{
    if (!cleanHandle())
        return false;
#if QT_CONFIG(systemsemaphore)
    systemSemaphore.setKey(QString(), 1);
    systemSemaphore.setKey(key, 1);
    if (systemSemaphore.error() != QSystemSemaphore::NoError) {
        QString function = "QSharedMemoryPrivate::initKey"_L1;
        errorString = QSharedMemory::tr("%1: unable to set key on lock").arg(function);
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
#endif
    errorString = QString();
    error = QSharedMemory::NoError;
    return true;
}

/*!
  Returns the key assigned with setKey() to this shared memory, or a null key
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
    return d->key;
}

/*!
  \since 4.8

  Returns the native, platform specific, key for this shared memory object. The
  native key is the identifier used by the operating system to identify the
  shared memory segment.

  You can use the native key to access shared memory segments that have not
  been created by Qt, or to grant shared memory access to non-Qt applications.

  \sa setKey(), setNativeKey()
*/
QString QSharedMemory::nativeKey() const
{
    Q_D(const QSharedMemory);
    return d->nativeKey;
}

/*!
  Creates a shared memory segment of \a size bytes with the key passed to the
  constructor, set with setKey() or set with setNativeKey(), then attaches to
  the new shared memory segment with the given access \a mode and returns
  \tt true. If a shared memory segment identified by the key already exists,
  the attach operation is not performed and \tt false is returned. When the
  return value is \tt false, call error() to determine which error occurred.

  \sa error()
 */
bool QSharedMemory::create(qsizetype size, AccessMode mode)
{
    Q_D(QSharedMemory);

    if (!d->initKey())
        return false;

#if QT_CONFIG(systemsemaphore)
#ifndef Q_OS_WIN
    // Take ownership and force set initialValue because the semaphore
    // might have already existed from a previous crash.
    d->systemSemaphore.setKey(d->key, 1, QSystemSemaphore::Create);
#endif
#endif

    QString function = "QSharedMemory::create"_L1;
#if QT_CONFIG(systemsemaphore)
    QSharedMemoryLocker lock(this);
    if (!d->key.isNull() && !d->tryLocker(&lock, function))
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
  call to setKey() or setNativeKey(). The access \a mode is \l {QSharedMemory::}
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

    if (isAttached() || !d->initKey())
        return false;
#if QT_CONFIG(systemsemaphore)
    QSharedMemoryLocker lock(this);
    if (!d->key.isNull() && !d->tryLocker(&lock, "QSharedMemory::attach"_L1))
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
    if (!d->key.isNull() && !d->tryLocker(&lock, "QSharedMemory::detach"_L1))
        return false;
#endif

    return d->detach();
}

/*!
  Returns a pointer to the contents of the shared memory segment, if
  one is attached. Otherwise it returns null. Remember to lock the
  shared memory with lock() before reading from or writing to the
  shared memory, and remember to release the lock with unlock() after
  you are done.

  \sa attach()
 */
void *QSharedMemory::data()
{
    Q_D(QSharedMemory);
    return d->memory;
}

/*!
  Returns a const pointer to the contents of the shared memory
  segment, if one is attached. Otherwise it returns null. Remember to
  lock the shared memory with lock() before reading from or writing to
  the shared memory, and remember to release the lock with unlock()
  after you are done.

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

#endif // QT_CONFIG(sharedmemory)

QT_END_NAMESPACE

#include "moc_qsharedmemory.cpp"
