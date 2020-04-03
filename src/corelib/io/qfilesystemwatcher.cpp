/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfilesystemwatcher.h"
#include "qfilesystemwatcher_p.h"

#include <qdatetime.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qloggingcategory.h>
#include <qset.h>
#include <qtimer.h>

#if (defined(Q_OS_LINUX) || defined(Q_OS_QNX)) && QT_CONFIG(inotify)
#define USE_INOTIFY
#endif

#include "qfilesystemwatcher_polling_p.h"
#if defined(Q_OS_WIN)
#  include "qfilesystemwatcher_win_p.h"
#elif defined(USE_INOTIFY)
#  include "qfilesystemwatcher_inotify_p.h"
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD) || defined(Q_OS_OPENBSD) || defined(QT_PLATFORM_UIKIT)
#  include "qfilesystemwatcher_kqueue_p.h"
#elif defined(Q_OS_MACOS)
#  include "qfilesystemwatcher_fsevents_p.h"
#endif

#include <algorithm>
#include <iterator>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcWatcher, "qt.core.filesystemwatcher")

QFileSystemWatcherEngine *QFileSystemWatcherPrivate::createNativeEngine(QObject *parent)
{
#if defined(Q_OS_WIN)
    return new QWindowsFileSystemWatcherEngine(parent);
#elif defined(USE_INOTIFY)
    // there is a chance that inotify may fail on Linux pre-2.6.13 (August
    // 2005), so we can't just new inotify directly.
    return QInotifyFileSystemWatcherEngine::create(parent);
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD) || defined(Q_OS_OPENBSD) || defined(QT_PLATFORM_UIKIT)
    return QKqueueFileSystemWatcherEngine::create(parent);
#elif defined(Q_OS_MACOS)
    return QFseventsFileSystemWatcherEngine::create(parent);
#else
    Q_UNUSED(parent);
    return 0;
#endif
}

QFileSystemWatcherPrivate::QFileSystemWatcherPrivate()
    : native(nullptr), poller(nullptr)
{
}

void QFileSystemWatcherPrivate::init()
{
    Q_Q(QFileSystemWatcher);
    native = createNativeEngine(q);
    if (native) {
        QObject::connect(native,
                         SIGNAL(fileChanged(QString,bool)),
                         q,
                         SLOT(_q_fileChanged(QString,bool)));
        QObject::connect(native,
                         SIGNAL(directoryChanged(QString,bool)),
                         q,
                         SLOT(_q_directoryChanged(QString,bool)));
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
        QObject::connect(static_cast<QWindowsFileSystemWatcherEngine *>(native),
                         &QWindowsFileSystemWatcherEngine::driveLockForRemoval,
                         q, [this] (const QString &p) { _q_winDriveLockForRemoval(p); });
        QObject::connect(static_cast<QWindowsFileSystemWatcherEngine *>(native),
                         &QWindowsFileSystemWatcherEngine::driveLockForRemovalFailed,
                         q, [this] (const QString &p) { _q_winDriveLockForRemovalFailed(p); });
        QObject::connect(static_cast<QWindowsFileSystemWatcherEngine *>(native),
                         &QWindowsFileSystemWatcherEngine::driveRemoved,
                         q, [this] (const QString &p) { _q_winDriveRemoved(p); });
#endif  // !Q_OS_WINRT
    }
}

void QFileSystemWatcherPrivate::initPollerEngine()
{
    if(poller)
        return;

    Q_Q(QFileSystemWatcher);
    poller = new QPollingFileSystemWatcherEngine(q); // that was a mouthful
    QObject::connect(poller,
                     SIGNAL(fileChanged(QString,bool)),
                     q,
                     SLOT(_q_fileChanged(QString,bool)));
    QObject::connect(poller,
                     SIGNAL(directoryChanged(QString,bool)),
                     q,
                     SLOT(_q_directoryChanged(QString,bool)));
}

void QFileSystemWatcherPrivate::_q_fileChanged(const QString &path, bool removed)
{
    Q_Q(QFileSystemWatcher);
    qCDebug(lcWatcher) << "file changed" << path << "removed?" << removed << "watching?" << files.contains(path);
    if (!files.contains(path)) {
        // the path was removed after a change was detected, but before we delivered the signal
        return;
    }
    if (removed)
        files.removeAll(path);
    emit q->fileChanged(path, QFileSystemWatcher::QPrivateSignal());
}

void QFileSystemWatcherPrivate::_q_directoryChanged(const QString &path, bool removed)
{
    Q_Q(QFileSystemWatcher);
    qCDebug(lcWatcher) << "directory changed" << path << "removed?" << removed << "watching?" << directories.contains(path);
    if (!directories.contains(path)) {
        // perhaps the path was removed after a change was detected, but before we delivered the signal
        return;
    }
    if (removed)
        directories.removeAll(path);
    emit q->directoryChanged(path, QFileSystemWatcher::QPrivateSignal());
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)

void QFileSystemWatcherPrivate::_q_winDriveLockForRemoval(const QString &path)
{
    // Windows: Request to lock a (removable/USB) drive for removal, release
    // its paths under watch, temporarily storing them should the lock fail.
    Q_Q(QFileSystemWatcher);
    QStringList pathsToBeRemoved;
    auto pred = [&path] (const QString &f) { return !f.startsWith(path, Qt::CaseInsensitive); };
    std::remove_copy_if(files.cbegin(), files.cend(),
                        std::back_inserter(pathsToBeRemoved), pred);
    std::remove_copy_if(directories.cbegin(), directories.cend(),
                        std::back_inserter(pathsToBeRemoved), pred);
    if (!pathsToBeRemoved.isEmpty()) {
        q->removePaths(pathsToBeRemoved);
        temporarilyRemovedPaths.insert(path.at(0), pathsToBeRemoved);
    }
}

void QFileSystemWatcherPrivate::_q_winDriveLockForRemovalFailed(const QString &path)
{
    // Windows: Request to lock a (removable/USB) drive failed (blocked by other
    // application), restore the watched paths.
    Q_Q(QFileSystemWatcher);
    if (!path.isEmpty()) {
        const auto it = temporarilyRemovedPaths.find(path.at(0));
        if (it != temporarilyRemovedPaths.end()) {
            q->addPaths(it.value());
            temporarilyRemovedPaths.erase(it);
        }
    }
}

void  QFileSystemWatcherPrivate::_q_winDriveRemoved(const QString &path)
{
    // Windows: Drive finally removed, clear out paths stored in lock request.
    if (!path.isEmpty())
        temporarilyRemovedPaths.remove(path.at(0));
}
#endif // Q_OS_WIN && !Q_OS_WINRT

/*!
    \class QFileSystemWatcher
    \inmodule QtCore
    \brief The QFileSystemWatcher class provides an interface for monitoring files and directories for modifications.
    \ingroup io
    \since 4.2
    \reentrant

    QFileSystemWatcher monitors the file system for changes to files
    and directories by watching a list of specified paths.

    Call addPath() to watch a particular file or directory. Multiple
    paths can be added using the addPaths() function. Existing paths can
    be removed by using the removePath() and removePaths() functions.

    QFileSystemWatcher examines each path added to it. Files that have
    been added to the QFileSystemWatcher can be accessed using the
    files() function, and directories using the directories() function.

    The fileChanged() signal is emitted when a file has been modified,
    renamed or removed from disk. Similarly, the directoryChanged()
    signal is emitted when a directory or its contents is modified or
    removed.  Note that QFileSystemWatcher stops monitoring files once
    they have been renamed or removed from disk, and directories once
    they have been removed from disk.

    \list
    \li \b Notes:
    \list
        \li On systems running a Linux kernel without inotify support,
        file systems that contain watched paths cannot be unmounted.

         \li The act of monitoring files and directories for
         modifications consumes system resources. This implies there is a
         limit to the number of files and directories your process can
         monitor simultaneously. On all BSD variants, for
         example, an open file descriptor is required for each monitored
         file. Some system limits the number of open file descriptors to 256
         by default. This means that addPath() and addPaths() will fail if
         your process tries to add more than 256 files or directories to
         the file system monitor. Also note that your process may have
         other file descriptors open in addition to the ones for files
         being monitored, and these other open descriptors also count in
         the total. \macos uses a different backend and does not
         suffer from this issue.
    \endlist
    \endlist

    \sa QFile, QDir
*/


/*!
    Constructs a new file system watcher object with the given \a parent.
*/
QFileSystemWatcher::QFileSystemWatcher(QObject *parent)
    : QObject(*new QFileSystemWatcherPrivate, parent)
{
    d_func()->init();
}

/*!
    Constructs a new file system watcher object with the given \a parent
    which monitors the specified \a paths list.
*/
QFileSystemWatcher::QFileSystemWatcher(const QStringList &paths, QObject *parent)
    : QObject(*new QFileSystemWatcherPrivate, parent)
{
    d_func()->init();
    addPaths(paths);
}

/*!
    Destroys the file system watcher.
*/
QFileSystemWatcher::~QFileSystemWatcher()
{ }

/*!
    Adds \a path to the file system watcher if \a path exists. The
    path is not added if it does not exist, or if it is already being
    monitored by the file system watcher.

    If \a path specifies a directory, the directoryChanged() signal
    will be emitted when \a path is modified or removed from disk;
    otherwise the fileChanged() signal is emitted when \a path is
    modified, renamed or removed.

    If the watch was successful, true is returned.

    Reasons for a watch failure are generally system-dependent, but
    may include the resource not existing, access failures, or the
    total watch count limit, if the platform has one.

    \note There may be a system dependent limit to the number of
    files and directories that can be monitored simultaneously.
    If this limit is been reached, \a path will not be monitored,
    and false is returned.

    \sa addPaths(), removePath()
*/
bool QFileSystemWatcher::addPath(const QString &path)
{
    if (path.isEmpty()) {
        qWarning("QFileSystemWatcher::addPath: path is empty");
        return true;
    }

    QStringList paths = addPaths(QStringList(path));
    return paths.isEmpty();
}

static QStringList empty_paths_pruned(const QStringList &paths)
{
    QStringList p;
    p.reserve(paths.size());
    const auto isEmpty = [](const QString &s) { return s.isEmpty(); };
    std::remove_copy_if(paths.begin(), paths.end(),
                        std::back_inserter(p),
                        isEmpty);
    return p;
}

/*!
    Adds each path in \a paths to the file system watcher. Paths are
    not added if they not exist, or if they are already being
    monitored by the file system watcher.

    If a path specifies a directory, the directoryChanged() signal
    will be emitted when the path is modified or removed from disk;
    otherwise the fileChanged() signal is emitted when the path is
    modified, renamed, or removed.

    The return value is a list of paths that could not be watched.

    Reasons for a watch failure are generally system-dependent, but
    may include the resource not existing, access failures, or the
    total watch count limit, if the platform has one.

    \note There may be a system dependent limit to the number of
    files and directories that can be monitored simultaneously.
    If this limit has been reached, the excess \a paths will not
    be monitored, and they will be added to the returned QStringList.

    \sa addPath(), removePaths()
*/
QStringList QFileSystemWatcher::addPaths(const QStringList &paths)
{
    Q_D(QFileSystemWatcher);

    QStringList p = empty_paths_pruned(paths);

    if (p.isEmpty()) {
        qWarning("QFileSystemWatcher::addPaths: list is empty");
        return p;
    }
    qCDebug(lcWatcher) << "adding" << paths;
    const auto selectEngine = [this, d]() -> QFileSystemWatcherEngine* {
#ifdef QT_BUILD_INTERNAL
        const QString on = objectName();

        if (Q_UNLIKELY(on.startsWith(QLatin1String("_qt_autotest_force_engine_")))) {
            // Autotest override case - use the explicitly selected engine only
            const QStringRef forceName = on.midRef(26);
            if (forceName == QLatin1String("poller")) {
                qCDebug(lcWatcher, "QFileSystemWatcher: skipping native engine, using only polling engine");
                d_func()->initPollerEngine();
                return d->poller;
            } else if (forceName == QLatin1String("native")) {
                qCDebug(lcWatcher, "QFileSystemWatcher: skipping polling engine, using only native engine");
                return d->native;
            }
            return nullptr;
        }
#endif
        // Normal runtime case - search intelligently for best engine
        if(d->native) {
            return d->native;
        } else {
            d_func()->initPollerEngine();
            return d->poller;
        }
    };

    if (auto engine = selectEngine())
        p = engine->addPaths(p, &d->files, &d->directories);

    return p;
}

/*!
    Removes the specified \a path from the file system watcher.

    If the watch is successfully removed, true is returned.

    Reasons for watch removal failing are generally system-dependent,
    but may be due to the path having already been deleted, for example.

    \sa removePaths(), addPath()
*/
bool QFileSystemWatcher::removePath(const QString &path)
{
    if (path.isEmpty()) {
        qWarning("QFileSystemWatcher::removePath: path is empty");
        return true;
    }

    QStringList paths = removePaths(QStringList(path));
    return paths.isEmpty();
}

/*!
    Removes the specified \a paths from the file system watcher.

    The return value is a list of paths which were not able to be
    unwatched successfully.

    Reasons for watch removal failing are generally system-dependent,
    but may be due to the path having already been deleted, for example.

    \sa removePath(), addPaths()
*/
QStringList QFileSystemWatcher::removePaths(const QStringList &paths)
{
    Q_D(QFileSystemWatcher);

    QStringList p = empty_paths_pruned(paths);

    if (p.isEmpty()) {
        qWarning("QFileSystemWatcher::removePaths: list is empty");
        return p;
    }
    qCDebug(lcWatcher) << "removing" << paths;

    if (d->native)
        p = d->native->removePaths(p, &d->files, &d->directories);
    if (d->poller)
        p = d->poller->removePaths(p, &d->files, &d->directories);

    return p;
}

/*!
    \fn void QFileSystemWatcher::fileChanged(const QString &path)

    This signal is emitted when the file at the specified \a path is
    modified, renamed or removed from disk.

    \note As a safety measure, many applications save an open file by
    writing a new file and then deleting the old one. In your slot
    function, you can check \c watcher.files().contains(path).
    If it returns \c false, check whether the file still exists
    and then call \c addPath() to continue watching it.

    \sa directoryChanged()
*/

/*!
    \fn void QFileSystemWatcher::directoryChanged(const QString &path)

    This signal is emitted when the directory at a specified \a path
    is modified (e.g., when a file is added or deleted) or removed
    from disk. Note that if there are several changes during a short
    period of time, some of the changes might not emit this signal.
    However, the last change in the sequence of changes will always
    generate this signal.

    \sa fileChanged()
*/

/*!
    \fn QStringList QFileSystemWatcher::directories() const

    Returns a list of paths to directories that are being watched.

    \sa files()
*/

/*!
    \fn QStringList QFileSystemWatcher::files() const

    Returns a list of paths to files that are being watched.

    \sa directories()
*/

QStringList QFileSystemWatcher::directories() const
{
    Q_D(const QFileSystemWatcher);
    return d->directories;
}

QStringList QFileSystemWatcher::files() const
{
    Q_D(const QFileSystemWatcher);
    return d->files;
}

QT_END_NAMESPACE

#include "moc_qfilesystemwatcher.cpp"
#include "moc_qfilesystemwatcher_p.cpp"

