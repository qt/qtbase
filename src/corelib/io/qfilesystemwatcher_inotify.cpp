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
#include "qfilesystemwatcher_inotify_p.h"

#include "private/qcore_unix_p.h"
#include "private/qsystemerror_p.h"

#include <qdebug.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qscopeguard.h>
#include <qsocketnotifier.h>
#include <qvarlengtharray.h>

#if defined(Q_OS_LINUX)
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#if defined(QT_NO_INOTIFY)

#if defined(Q_OS_QNX)
// These files should only be compiled on QNX if the inotify headers are found
#error "Should not get here."
#endif

#include <linux/types.h>

#if defined(__i386__)
# define __NR_inotify_init      291
# define __NR_inotify_add_watch 292
# define __NR_inotify_rm_watch  293
# define __NR_inotify_init1     332
#elif defined(__x86_64__)
# define __NR_inotify_init      253
# define __NR_inotify_add_watch 254
# define __NR_inotify_rm_watch  255
# define __NR_inotify_init1     294
#elif defined(__powerpc__) || defined(__powerpc64__)
# define __NR_inotify_init      275
# define __NR_inotify_add_watch 276
# define __NR_inotify_rm_watch  277
# define __NR_inotify_init1     318
#elif defined (__ia64__)
# define __NR_inotify_init      1277
# define __NR_inotify_add_watch 1278
# define __NR_inotify_rm_watch  1279
# define __NR_inotify_init1     1318
#elif defined (__s390__) || defined (__s390x__)
# define __NR_inotify_init      284
# define __NR_inotify_add_watch 285
# define __NR_inotify_rm_watch  286
# define __NR_inotify_init1     324
#elif defined (__alpha__)
# define __NR_inotify_init      444
# define __NR_inotify_add_watch 445
# define __NR_inotify_rm_watch  446
// no inotify_init1 for the Alpha
#elif defined (__sparc__) || defined (__sparc64__)
# define __NR_inotify_init      151
# define __NR_inotify_add_watch 152
# define __NR_inotify_rm_watch  156
# define __NR_inotify_init1     322
#elif defined (__arm__)
# define __NR_inotify_init      316
# define __NR_inotify_add_watch 317
# define __NR_inotify_rm_watch  318
# define __NR_inotify_init1     360
#elif defined (__sh__)
# define __NR_inotify_init      290
# define __NR_inotify_add_watch 291
# define __NR_inotify_rm_watch  292
# define __NR_inotify_init1     332
#elif defined (__sh64__)
# define __NR_inotify_init      318
# define __NR_inotify_add_watch 319
# define __NR_inotify_rm_watch  320
# define __NR_inotify_init1     360
#elif defined (__mips__)
# define __NR_inotify_init      284
# define __NR_inotify_add_watch 285
# define __NR_inotify_rm_watch  286
# define __NR_inotify_init1     329
#elif defined (__hppa__)
# define __NR_inotify_init      269
# define __NR_inotify_add_watch 270
# define __NR_inotify_rm_watch  271
# define __NR_inotify_init1     314
#elif defined (__avr32__)
# define __NR_inotify_init      240
# define __NR_inotify_add_watch 241
# define __NR_inotify_rm_watch  242
// no inotify_init1 for AVR32
#elif defined (__mc68000__)
# define __NR_inotify_init      284
# define __NR_inotify_add_watch 285
# define __NR_inotify_rm_watch  286
# define __NR_inotify_init1     328
#elif defined (__aarch64__)
# define __NR_inotify_init1     26
# define __NR_inotify_add_watch 27
# define __NR_inotify_rm_watch  28
// no inotify_init for aarch64
#else
# error "This architecture is not supported. Please see http://www.qt-project.org/"
#endif

#if !defined(IN_CLOEXEC) && defined(O_CLOEXEC) && defined(__NR_inotify_init1)
# define IN_CLOEXEC              O_CLOEXEC
#endif

QT_BEGIN_NAMESPACE

#ifdef QT_LINUXBASE
// ### the LSB doesn't standardize syscall, need to wait until glib2.4 is standardized
static inline int syscall(...) { return -1; }
#endif

static inline int inotify_init()
{
#ifdef __NR_inotify_init
    return syscall(__NR_inotify_init);
#else
    return syscall(__NR_inotify_init1, 0);
#endif
}

static inline int inotify_add_watch(int fd, const char *name, __u32 mask)
{
    return syscall(__NR_inotify_add_watch, fd, name, mask);
}

static inline int inotify_rm_watch(int fd, __u32 wd)
{
    return syscall(__NR_inotify_rm_watch, fd, wd);
}

#ifdef IN_CLOEXEC
static inline int inotify_init1(int flags)
{
    return syscall(__NR_inotify_init1, flags);
}
#endif

// the following struct and values are documented in linux/inotify.h
extern "C" {

struct inotify_event {
        __s32           wd;
        __u32           mask;
        __u32           cookie;
        __u32           len;
        char            name[0];
};

#define IN_ACCESS               0x00000001
#define IN_MODIFY               0x00000002
#define IN_ATTRIB               0x00000004
#define IN_CLOSE_WRITE          0x00000008
#define IN_CLOSE_NOWRITE        0x00000010
#define IN_OPEN                 0x00000020
#define IN_MOVED_FROM           0x00000040
#define IN_MOVED_TO             0x00000080
#define IN_CREATE               0x00000100
#define IN_DELETE               0x00000200
#define IN_DELETE_SELF          0x00000400
#define IN_MOVE_SELF            0x00000800
#define IN_UNMOUNT              0x00002000
#define IN_Q_OVERFLOW           0x00004000
#define IN_IGNORED              0x00008000

#define IN_CLOSE                (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)
#define IN_MOVE                 (IN_MOVED_FROM | IN_MOVED_TO)
}

QT_END_NAMESPACE

// --------- inotify.h end ----------

#else /* QT_NO_INOTIFY */

#include <sys/inotify.h>

// see https://github.com/android-ndk/ndk/issues/394
# if defined(Q_OS_ANDROID) && (__ANDROID_API__ < 21)
static inline int inotify_init1(int flags)
{
    return syscall(__NR_inotify_init1, flags);
}
# endif

#endif

QT_BEGIN_NAMESPACE

QInotifyFileSystemWatcherEngine *QInotifyFileSystemWatcherEngine::create(QObject *parent)
{
    int fd = -1;
#if defined(IN_CLOEXEC)
    fd = inotify_init1(IN_CLOEXEC);
#endif
    if (fd == -1) {
        fd = inotify_init();
        if (fd == -1)
            return nullptr;
    }
    return new QInotifyFileSystemWatcherEngine(fd, parent);
}

QInotifyFileSystemWatcherEngine::QInotifyFileSystemWatcherEngine(int fd, QObject *parent)
    : QFileSystemWatcherEngine(parent),
      inotifyFd(fd),
      notifier(fd, QSocketNotifier::Read, this)
{
    fcntl(inotifyFd, F_SETFD, FD_CLOEXEC);
    connect(&notifier, SIGNAL(activated(QSocketDescriptor)), SLOT(readFromInotify()));
}

QInotifyFileSystemWatcherEngine::~QInotifyFileSystemWatcherEngine()
{
    notifier.setEnabled(false);
    for (int id : qAsConst(pathToID))
        inotify_rm_watch(inotifyFd, id < 0 ? -id : id);

    ::close(inotifyFd);
}

QStringList QInotifyFileSystemWatcherEngine::addPaths(const QStringList &paths,
                                                      QStringList *files,
                                                      QStringList *directories)
{
    QStringList unhandled;
    for (const QString &path : paths) {
        QFileInfo fi(path);
        bool isDir = fi.isDir();
        auto sg = qScopeGuard([&]{ unhandled.push_back(path); });
        if (isDir) {
            if (directories->contains(path))
                continue;
        } else {
            if (files->contains(path))
                continue;
        }

        int wd = inotify_add_watch(inotifyFd,
                                   QFile::encodeName(path),
                                   (isDir
                                    ? (0
                                       | IN_ATTRIB
                                       | IN_MOVE
                                       | IN_CREATE
                                       | IN_DELETE
                                       | IN_DELETE_SELF
                                       )
                                    : (0
                                       | IN_ATTRIB
                                       | IN_MODIFY
                                       | IN_MOVE
                                       | IN_MOVE_SELF
                                       | IN_DELETE_SELF
                                       )));
        if (wd < 0) {
            if (errno != ENOENT)
                qErrnoWarning("inotify_add_watch(%ls) failed:", path.constData());
            continue;
        }

        sg.dismiss();

        int id = isDir ? -wd : wd;
        if (id < 0) {
            directories->append(path);
        } else {
            files->append(path);
        }

        pathToID.insert(path, id);
        idToPath.insert(id, path);
    }

    return unhandled;
}

QStringList QInotifyFileSystemWatcherEngine::removePaths(const QStringList &paths,
                                                         QStringList *files,
                                                         QStringList *directories)
{
    QStringList unhandled;
    for (const QString &path : paths) {
        int id = pathToID.take(path);

        auto sg = qScopeGuard([&]{ unhandled.push_back(path); });

        // Multiple paths could be associated to the same watch descriptor
        // when a file is moved and added with the new name.
        // So we should find and delete the correct one by using
        // both id and path
        auto path_range = idToPath.equal_range(id);
        auto path_it = std::find(path_range.first, path_range.second, path);
        if (path_it == idToPath.end())
            continue;

        const ssize_t num_elements = std::distance(path_range.first, path_range.second);
        idToPath.erase(path_it);

        // If there was only one path associated to the given id we should remove the watch
        if (num_elements == 1) {
            int wd = id < 0 ? -id : id;
            inotify_rm_watch(inotifyFd, wd);
        }

        sg.dismiss();

        if (id < 0) {
            directories->removeAll(path);
        } else {
            files->removeAll(path);
        }
    }

    return unhandled;
}

void QInotifyFileSystemWatcherEngine::readFromInotify()
{
    // qDebug("QInotifyFileSystemWatcherEngine::readFromInotify");

    int buffSize = 0;
    ioctl(inotifyFd, FIONREAD, (char *) &buffSize);
    QVarLengthArray<char, 4096> buffer(buffSize);
    buffSize = read(inotifyFd, buffer.data(), buffSize);
    char *at = buffer.data();
    char * const end = at + buffSize;

    QHash<int, inotify_event *> eventForId;
    while (at < end) {
        inotify_event *event = reinterpret_cast<inotify_event *>(at);

        if (eventForId.contains(event->wd))
            eventForId[event->wd]->mask |= event->mask;
        else
            eventForId.insert(event->wd, event);

        at += sizeof(inotify_event) + event->len;
    }

    QHash<int, inotify_event *>::const_iterator it = eventForId.constBegin();
    while (it != eventForId.constEnd()) {
        const inotify_event &event = **it;
        ++it;

        // qDebug() << "inotify event, wd" << event.wd << "mask" << Qt::hex << event.mask;

        int id = event.wd;
        QString path = getPathFromID(id);
        if (path.isEmpty()) {
            // perhaps a directory?
            id = -id;
            path = getPathFromID(id);
            if (path.isEmpty())
                continue;
        }

        // qDebug() << "event for path" << path;

        if ((event.mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)) != 0) {
            pathToID.remove(path);
            idToPath.remove(id, getPathFromID(id));
            if (!idToPath.contains(id))
                inotify_rm_watch(inotifyFd, event.wd);

            if (id < 0)
                emit directoryChanged(path, true);
            else
                emit fileChanged(path, true);
        } else {
            if (id < 0)
                emit directoryChanged(path, false);
            else
                emit fileChanged(path, false);
        }
    }
}

template <typename Hash, typename Key>
typename Hash::const_iterator
find_last_in_equal_range(const Hash &c, const Key &key)
{
    // find c.equal_range(key).second - 1 without backwards iteration:
    auto i = c.find(key);
    const auto end = c.cend();
    if (i == end)
        return end;
    decltype(i) prev;
    do {
        prev = i;
        ++i;
    } while (i != end && i.key() == key);
    return prev;
}

QString QInotifyFileSystemWatcherEngine::getPathFromID(int id) const
{
    auto i = find_last_in_equal_range(idToPath, id);
    return i == idToPath.cend() ? QString() : i.value() ;
}

QT_END_NAMESPACE

#include "moc_qfilesystemwatcher_inotify_p.cpp"
