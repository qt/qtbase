// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"

#if QT_CONFIG(systemsemaphore)
#include <QtCore/q20memory.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

using namespace QtIpcCommon;
using namespace Qt::StringLiterals;

inline void QSystemSemaphorePrivate::constructBackend()
{
    visit([](auto p) { q20::construct_at(p); });
}

inline void QSystemSemaphorePrivate::destructBackend()
{
    visit([](auto p) { std::destroy_at(p); });
}

/*!
  \class QSystemSemaphore
  \inmodule QtCore
  \since 4.4

  \brief The QSystemSemaphore class provides a general counting system semaphore.

  A system semaphore is a generalization of \l QSemaphore. Typically, a
  semaphore is used to protect a certain number of identical resources.

  Like its lighter counterpart, a QSystemSemaphore can be
  accessed from multiple \l {QThread} {threads}. Unlike QSemaphore, a
  QSystemSemaphore can also be accessed from multiple \l {QProcess}
  {processes}. This means QSystemSemaphore is a much heavier class, so
  if your application doesn't need to access your semaphores across
  multiple processes, you will probably want to use QSemaphore.

  Semaphores support two fundamental operations, acquire() and release():

  acquire() tries to acquire one resource. If there isn't a resource
  available, the call blocks until a resource becomes available. Then
  the resource is acquired and the call returns.

  release() releases one resource so it can be acquired by another
  process. The function can also be called with a parameter n > 1,
  which releases n resources.

  System semaphores are identified by a key, represented by \l QNativeIpcKey. A
  key can be created in a cross-platform manner by using platformSafeKey(). A
  system semaphore is created by the QSystemSemaphore constructor when passed
  an access mode parameter of AccessMode::Create. Once it is created, other
  processes may attach to the same semaphore using the same key and an access
  mode parameter of AccessMode::Open.

  Example: Create a system semaphore
  \snippet code/src_corelib_kernel_qsystemsemaphore.cpp 0

  For details on the key types, platform-specific limitations, and
  interoperability with older or non-Qt applications, see the \l{Native IPC
  Keys} documentation. That includes important information for sandboxed
  applications on Apple platforms, including all apps obtained via the Apple
  App Store.

  \sa {Inter-Process Communication}, QSharedMemory, QSemaphore
 */

/*!
  Requests a system semaphore identified by the legacy key \a key.
 */
QSystemSemaphore::QSystemSemaphore(const QString &key, int initialValue, AccessMode mode)
    : QSystemSemaphore(legacyNativeKey(key), initialValue, mode)
{
}

/*!
  Requests a system semaphore for the specified \a key. The parameters
  \a initialValue and \a mode are used according to the following
  rules, which are system dependent.

  In Unix, if the \a mode is \l {QSystemSemaphore::} {Open} and the
  system already has a semaphore identified by \a key, that semaphore
  is used, and the semaphore's resource count is not changed, i.e., \a
  initialValue is ignored. But if the system does not already have a
  semaphore identified by \a key, it creates a new semaphore for that
  key and sets its resource count to \a initialValue.

  In Unix, if the \a mode is \l {QSystemSemaphore::} {Create} and the
  system already has a semaphore identified by \a key, that semaphore
  is used, and its resource count is set to \a initialValue. If the
  system does not already have a semaphore identified by \a key, it
  creates a new semaphore for that key and sets its resource count to
  \a initialValue.

  In Windows, \a mode is ignored, and the system always tries to
  create a semaphore for the specified \a key. If the system does not
  already have a semaphore identified as \a key, it creates the
  semaphore and sets its resource count to \a initialValue. But if the
  system already has a semaphore identified as \a key it uses that
  semaphore and ignores \a initialValue.

  The \l {QSystemSemaphore::AccessMode} {mode} parameter is only used
  in Unix systems to handle the case where a semaphore survives a
  process crash. In that case, the next process to allocate a
  semaphore with the same \a key will get the semaphore that survived
  the crash, and unless \a mode is \l {QSystemSemaphore::} {Create},
  the resource count will not be reset to \a initialValue but will
  retain the initial value it had been given by the crashed process.

  \sa acquire(), key()
 */
QSystemSemaphore::QSystemSemaphore(const QNativeIpcKey &key, int initialValue, AccessMode mode)
    : d(new QSystemSemaphorePrivate(key.type()))
{
    setNativeKey(key, initialValue, mode);
}

/*!
  The destructor destroys the QSystemSemaphore object, but the
  underlying system semaphore is not removed from the system unless
  this instance of QSystemSemaphore is the last one existing for that
  system semaphore.

  Two important side effects of the destructor depend on the system.
  In Windows, if acquire() has been called for this semaphore but not
  release(), release() will not be called by the destructor, nor will
  the resource be released when the process exits normally. This would
  be a program bug which could be the cause of a deadlock in another
  process trying to acquire the same resource. In Unix, acquired
  resources that are not released before the destructor is called are
  automatically released when the process exits.
*/
QSystemSemaphore::~QSystemSemaphore()
{
    d->cleanHandle();
}

/*!
  \enum QSystemSemaphore::AccessMode

  This enum is used by the constructor and setKey(). Its purpose is to
  enable handling the problem in Unix implementations of semaphores
  that survive a crash. In Unix, when a semaphore survives a crash, we
  need a way to force it to reset its resource count, when the system
  reuses the semaphore. In Windows, where semaphores can't survive a
  crash, this enum has no effect.

  \value Open If the semaphore already exists, its initial resource
  count is not reset. If the semaphore does not already exist, it is
  created and its initial resource count set.

  \value Create QSystemSemaphore takes ownership of the semaphore and
  sets its resource count to the requested value, regardless of
  whether the semaphore already exists by having survived a crash.
  This value should be passed to the constructor, when the first
  semaphore for a particular key is constructed and you know that if
  the semaphore already exists it could only be because of a crash. In
  Windows, where a semaphore can't survive a crash, Create and Open
  have the same behavior.
*/

/*!
  This function works the same as the constructor. It reconstructs
  this QSystemSemaphore object. If the new \a key is different from
  the old key, calling this function is like calling the destructor of
  the semaphore with the old key, then calling the constructor to
  create a new semaphore with the new \a key. The \a initialValue and
  \a mode parameters are as defined for the constructor.

  This function is useful if the native key was shared from another process.
  See \l{Native IPC Keys} for more information.

  \sa QSystemSemaphore(), nativeIpcKey()
 */
void QSystemSemaphore::setNativeKey(const QNativeIpcKey &key, int initialValue, AccessMode mode)
{
    if (key == d->nativeKey && mode == Open)
        return;
    if (!isKeyTypeSupported(key.type())) {
        d->setError(KeyError, tr("%1: unsupported key type")
                    .arg("QSystemSemaphore::setNativeKey"_L1));
        return;
    }

    d->clearError();
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
    d->initialValue = initialValue;
    d->handle(mode);
}

/*!
  Returns the key assigned to this system semaphore. The key is the
  name by which the semaphore can be accessed from other processes.

  You can use the native key to access system semaphores that have not been
  created by Qt, or to grant access to non-Qt applications. See \l{Native IPC
  Keys} for more information.

  \sa setNativeKey()
 */
QNativeIpcKey QSystemSemaphore::nativeIpcKey() const
{
    return d->nativeKey;
}

/*!
  This function works the same as the constructor. It reconstructs
  this QSystemSemaphore object. If the new \a key is different from
  the old key, calling this function is like calling the destructor of
  the semaphore with the old key, then calling the constructor to
  create a new semaphore with the new \a key. The \a initialValue and
  \a mode parameters are as defined for the constructor.

  \sa QSystemSemaphore(), key()
 */
void QSystemSemaphore::setKey(const QString &key, int initialValue, AccessMode mode)
{
    setNativeKey(legacyNativeKey(key), initialValue, mode);
}

/*!
  Returns the legacy key assigned to this system semaphore. The key is the
  name by which the semaphore can be accessed from other processes.

  \sa setKey()
 */
QString QSystemSemaphore::key() const
{
    return QNativeIpcKeyPrivate::legacyKey(d->nativeKey);
}

/*!
  Acquires one of the resources guarded by this semaphore, if there is
  one available, and returns \c true. If all the resources guarded by this
  semaphore have already been acquired, the call blocks until one of
  them is released by another process or thread having a semaphore
  with the same key.

  If false is returned, a system error has occurred. Call error()
  to get a value of QSystemSemaphore::SystemSemaphoreError that
  indicates which error occurred.

  \sa release()
 */
bool QSystemSemaphore::acquire()
{
    return d->modifySemaphore(-1);
}

/*!
  Releases \a n resources guarded by the semaphore. Returns \c true
  unless there is a system error.

  Example: Create a system semaphore having five resources; acquire
  them all and then release them all.

  \snippet code/src_corelib_kernel_qsystemsemaphore.cpp 1

  This function can also "create" resources. For example, immediately
  following the sequence of statements above, suppose we add the
  statement:

  \snippet code/src_corelib_kernel_qsystemsemaphore.cpp 2

  Ten new resources are now guarded by the semaphore, in addition to
  the five that already existed. You would not normally use this
  function to create more resources.

  \sa acquire()
 */
bool QSystemSemaphore::release(int n)
{
    if (n == 0)
        return true;
    if (n < 0) {
        qWarning("QSystemSemaphore::release: n is negative.");
        return false;
    }
    return d->modifySemaphore(n);
}

/*!
  Returns a value indicating whether an error occurred, and, if so,
  which error it was.

  \sa errorString()
 */
QSystemSemaphore::SystemSemaphoreError QSystemSemaphore::error() const
{
    return d->error;
}

/*!
  \enum QSystemSemaphore::SystemSemaphoreError

  \value NoError No error occurred.

  \value PermissionDenied The operation failed because the caller
  didn't have the required permissions.

  \value KeyError The operation failed because of an invalid key.

  \value AlreadyExists The operation failed because a system
  semaphore with the specified key already existed.

  \value NotFound The operation failed because a system semaphore
  with the specified key could not be found.

  \value OutOfResources The operation failed because there was
  not enough memory available to fill the request.

  \value UnknownError Something else happened and it was bad.
*/

/*!
  Returns a text description of the last error that occurred. If
  error() returns an \l {QSystemSemaphore::SystemSemaphoreError} {error
  value}, call this function to get a text string that describes the
  error.

  \sa error()
 */
QString QSystemSemaphore::errorString() const
{
    return d->errorString;
}

void QSystemSemaphorePrivate::setUnixErrorString(QLatin1StringView function)
{
    // EINVAL is handled in functions so they can give better error strings
    switch (errno) {
    case EPERM:
    case EACCES:
        errorString = QSystemSemaphore::tr("%1: permission denied").arg(function);
        error = QSystemSemaphore::PermissionDenied;
        break;
    case EEXIST:
        errorString = QSystemSemaphore::tr("%1: already exists").arg(function);
        error = QSystemSemaphore::AlreadyExists;
        break;
    case ENOENT:
        errorString = QSystemSemaphore::tr("%1: does not exist").arg(function);
        error = QSystemSemaphore::NotFound;
        break;
    case ERANGE:
    case ENOSPC:
    case EMFILE:
        errorString = QSystemSemaphore::tr("%1: out of resources").arg(function);
        error = QSystemSemaphore::OutOfResources;
        break;
    case ENAMETOOLONG:
        errorString = QSystemSemaphore::tr("%1: key too long").arg(function);
        error = QSystemSemaphore::KeyError;
        break;
    default:
        errorString = QSystemSemaphore::tr("%1: unknown error: %2")
                .arg(function, qt_error_string(errno));
        error = QSystemSemaphore::UnknownError;
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug() << errorString << "key" << key << "errno" << errno << EINVAL;
#endif
    }
}

bool QSystemSemaphore::isKeyTypeSupported(QNativeIpcKey::Type type)
{
    if (!isIpcSupported(IpcType::SystemSemaphore, type))
        return false;
    using Variant = decltype(QSystemSemaphorePrivate::backend);
    return Variant::staticVisit(type, [](auto ptr) {
        using Impl = std::decay_t<decltype(*ptr)>;
        return Impl::runtimeSupportCheck();
    });
}

QNativeIpcKey QSystemSemaphore::platformSafeKey(const QString &key, QNativeIpcKey::Type type)
{
    return QtIpcCommon::platformSafeKey(key, IpcType::SystemSemaphore, type);
}

QNativeIpcKey QSystemSemaphore::legacyNativeKey(const QString &key, QNativeIpcKey::Type type)
{
    return QtIpcCommon::legacyPlatformSafeKey(key, IpcType::SystemSemaphore, type);
}

QT_END_NAMESPACE

#include "moc_qsystemsemaphore.cpp"

#endif // QT_CONFIG(systemsemaphore)
