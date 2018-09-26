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
#include "qfilesystemwatcher_win_p.h"

#include <qdebug.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qset.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qtextstream.h>

#include <qt_windows.h>

#ifndef Q_OS_WINRT
#  include <qabstractnativeeventfilter.h>
#  include <qcoreapplication.h>
#  include <qdir.h>
#  include <private/qeventdispatcher_win_p.h>
#  include <dbt.h>
#  include <algorithm>
#  include <vector>
#endif // !Q_OS_WINRT

QT_BEGIN_NAMESPACE

// #define WINQFSW_DEBUG
#ifdef WINQFSW_DEBUG
#  define DEBUG qDebug
#else
#  define DEBUG if (false) qDebug
#endif

static Qt::HANDLE createChangeNotification(const QString &path, uint flags)
{
    // Volume and folder paths need a trailing slash for proper notification
    // (e.g. "c:" -> "c:/").
    QString nativePath = QDir::toNativeSeparators(path);
    if ((flags & FILE_NOTIFY_CHANGE_ATTRIBUTES) == 0 && !nativePath.endsWith(QLatin1Char('\\')))
        nativePath.append(QLatin1Char('\\'));
    const HANDLE result = FindFirstChangeNotification(reinterpret_cast<const wchar_t *>(nativePath.utf16()),
                                                      FALSE, flags);
    DEBUG() << __FUNCTION__ << nativePath << hex <<showbase << flags << "returns" << result;
    return result;
}

#ifndef Q_OS_WINRT
///////////
// QWindowsRemovableDriveListener
// Listen for the various WM_DEVICECHANGE message indicating drive addition/removal
// requests and removals.
///////////
class QWindowsRemovableDriveListener : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    // Device UUids as declared in ioevent.h (GUID_IO_VOLUME_LOCK, ...)
    enum VolumeUuid { UnknownUuid, UuidIoVolumeLock, UuidIoVolumeLockFailed,
                      UuidIoVolumeUnlock, UuidIoMediaRemoval };

    struct RemovableDriveEntry {
        HDEVNOTIFY devNotify;
        wchar_t drive;
    };

    explicit QWindowsRemovableDriveListener(QObject *parent = nullptr);
    ~QWindowsRemovableDriveListener();

    // Call from QFileSystemWatcher::addPaths() to set up notifications on drives
    void addPath(const QString &path);

    bool nativeEventFilter(const QByteArray &, void *messageIn, long *) override;

signals:
    void driveAdded();
    void driveRemoved(); // Some drive removed
    void driveRemoved(const QString &); // Watched/known drive removed
    void driveLockForRemoval(const QString &);
    void driveLockForRemovalFailed(const QString &);

private:
    static VolumeUuid volumeUuid(const UUID &needle);
    void handleDbtCustomEvent(const MSG *msg);
    void handleDbtDriveArrivalRemoval(const MSG *msg);

    std::vector<RemovableDriveEntry> m_removableDrives;
    quintptr m_lastMessageHash;
};

QWindowsRemovableDriveListener::QWindowsRemovableDriveListener(QObject *parent)
    : QObject(parent)
    , m_lastMessageHash(0)
{
}

static void stopDeviceNotification(QWindowsRemovableDriveListener::RemovableDriveEntry &e)
{
    UnregisterDeviceNotification(e.devNotify);
    e.devNotify = 0;
}

template <class Iterator> // Search sequence of RemovableDriveEntry for HDEVNOTIFY.
static inline Iterator findByHDevNotify(Iterator i1, Iterator i2, HDEVNOTIFY hdevnotify)
{
    return std::find_if(i1, i2,
                        [hdevnotify] (const QWindowsRemovableDriveListener::RemovableDriveEntry &e) { return e.devNotify == hdevnotify; });
}

QWindowsRemovableDriveListener::~QWindowsRemovableDriveListener()
{
    std::for_each(m_removableDrives.begin(), m_removableDrives.end(), stopDeviceNotification);
}

static QString pathFromEntry(const QWindowsRemovableDriveListener::RemovableDriveEntry &re)
{
    QString path = QStringLiteral("A:/");
    path[0] = QChar::fromLatin1(re.drive);
    return path;
}

// Handle WM_DEVICECHANGE+DBT_CUSTOMEVENT, which is sent based on the registration
// on the volume handle with QEventDispatcherWin32's message window in the class.
// Capture the GUID_IO_VOLUME_LOCK indicating the drive is to be removed.
QWindowsRemovableDriveListener::VolumeUuid QWindowsRemovableDriveListener::volumeUuid(const UUID &needle)
{
    static const struct VolumeUuidMapping // UUIDs from IoEvent.h (missing in MinGW)
    {
        VolumeUuid v;
        UUID uuid;
    } mapping[] = {
        { UuidIoVolumeLock, // GUID_IO_VOLUME_LOCK
          {0x50708874, 0xc9af, 0x11d1, {0x8f, 0xef, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x32}} },
        { UuidIoVolumeLockFailed, // GUID_IO_VOLUME_LOCK_FAILED
          {0xae2eed10, 0x0ba8, 0x11d2, {0x8f, 0xfb, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x32}} },
        { UuidIoVolumeUnlock, // GUID_IO_VOLUME_UNLOCK
          {0x9a8c3d68, 0xd0cb, 0x11d1, {0x8f, 0xef, 0x0, 0xa0, 0xc9, 0xa0, 0x6d, 0x32}} },
        { UuidIoMediaRemoval, // GUID_IO_MEDIA_REMOVAL
          {0xd07433c1, 0xa98e, 0x11d2, {0x91, 0x7a, 0x0, 0xa0, 0xc9, 0x06, 0x8f, 0xf3}} }
    };

    static const VolumeUuidMapping *end = mapping + sizeof(mapping) / sizeof(mapping[0]);
    const VolumeUuidMapping *m =
        std::find_if(mapping, end, [&needle] (const VolumeUuidMapping &m) { return IsEqualGUID(m.uuid, needle); });
    return m != end ? m->v : UnknownUuid;
}

inline void QWindowsRemovableDriveListener::handleDbtCustomEvent(const MSG *msg)
{
    const DEV_BROADCAST_HDR *broadcastHeader = reinterpret_cast<const DEV_BROADCAST_HDR *>(msg->lParam);
    if (broadcastHeader->dbch_devicetype != DBT_DEVTYP_HANDLE)
        return;
    const DEV_BROADCAST_HANDLE *broadcastHandle = reinterpret_cast<const DEV_BROADCAST_HANDLE *>(broadcastHeader);
    const auto it = findByHDevNotify(m_removableDrives.cbegin(), m_removableDrives.cend(),
                                     broadcastHandle->dbch_hdevnotify);
    if (it == m_removableDrives.cend())
        return;
    switch (volumeUuid(broadcastHandle->dbch_eventguid)) {
    case UuidIoVolumeLock: // Received for removable USB media
        emit driveLockForRemoval(pathFromEntry(*it));
        break;
    case UuidIoVolumeLockFailed:
        emit driveLockForRemovalFailed(pathFromEntry(*it));
        break;
    case UuidIoVolumeUnlock:
        break;
    case UuidIoMediaRemoval: // Received for optical drives
        break;
    default:
        break;
    }
}

// Handle WM_DEVICECHANGE+DBT_DEVICEARRIVAL/DBT_DEVICEREMOVECOMPLETE which are
// sent to all top level windows and cannot be registered for (that is, their
// triggering depends on top level windows being present)
inline void QWindowsRemovableDriveListener::handleDbtDriveArrivalRemoval(const MSG *msg)
{
    const DEV_BROADCAST_HDR *broadcastHeader = reinterpret_cast<const DEV_BROADCAST_HDR *>(msg->lParam);
    switch (broadcastHeader->dbch_devicetype) {
    case DBT_DEVTYP_HANDLE: // WM_DEVICECHANGE/DBT_DEVTYP_HANDLE is sent for our registered drives.
        if (msg->wParam == DBT_DEVICEREMOVECOMPLETE) {
            const DEV_BROADCAST_HANDLE *broadcastHandle = reinterpret_cast<const DEV_BROADCAST_HANDLE *>(broadcastHeader);
            const auto it = findByHDevNotify(m_removableDrives.begin(), m_removableDrives.end(),
                                           broadcastHandle->dbch_hdevnotify);
            // Emit for removable USB drives we were registered for.
            if (it != m_removableDrives.end()) {
                emit driveRemoved(pathFromEntry(*it));
                stopDeviceNotification(*it);
                m_removableDrives.erase(it);
            }
        }
        break;
    case DBT_DEVTYP_VOLUME: {
        const DEV_BROADCAST_VOLUME *broadcastVolume = reinterpret_cast<const DEV_BROADCAST_VOLUME *>(broadcastHeader);
        // WM_DEVICECHANGE/DBT_DEVTYP_VOLUME messages are sent to all toplevel windows. Compare a hash value to ensure
        // it is handled only once.
        const quintptr newHash = reinterpret_cast<quintptr>(broadcastVolume) + msg->wParam
            + quintptr(broadcastVolume->dbcv_flags) + quintptr(broadcastVolume->dbcv_unitmask);
        if (newHash == m_lastMessageHash)
            return;
        m_lastMessageHash = newHash;
        // Check for DBTF_MEDIA (inserted/Removed Optical drives). Ignore for now.
        if (broadcastVolume->dbcv_flags & DBTF_MEDIA)
            return;
        // Continue with plugged in USB media where dbcv_flags=0.
        switch (msg->wParam) {
        case DBT_DEVICEARRIVAL:
            emit driveAdded();
            break;
        case DBT_DEVICEREMOVECOMPLETE: // See above for handling of drives registered with watchers
            emit driveRemoved();
            break;
        }
    }
        break;
    }
}

bool QWindowsRemovableDriveListener::nativeEventFilter(const QByteArray &, void *messageIn, long *)
{
    const MSG *msg = reinterpret_cast<const MSG *>(messageIn);
    if (msg->message == WM_DEVICECHANGE) {
        switch (msg->wParam) {
        case DBT_CUSTOMEVENT:
            handleDbtCustomEvent(msg);
            break;
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
            handleDbtDriveArrivalRemoval(msg);
            break;
        }
    }
    return false;
}

// Set up listening for WM_DEVICECHANGE+DBT_CUSTOMEVENT for a removable drive path,
void QWindowsRemovableDriveListener::addPath(const QString &p)
{
    const wchar_t drive = p.size() >= 2 && p.at(0).isLetter() && p.at(1) == QLatin1Char(':')
        ? wchar_t(p.at(0).toUpper().unicode()) : L'\0';
    if (!drive)
        return;
    // Already listening?
    if (std::any_of(m_removableDrives.cbegin(), m_removableDrives.cend(),
                    [drive](const RemovableDriveEntry &e) { return e.drive == drive; })) {
        return;
    }

    wchar_t devicePath[8] = L"\\\\.\\A:\\";
    devicePath[4] = drive;
    RemovableDriveEntry re;
    re.drive = drive;
    if (GetDriveTypeW(devicePath + 4) != DRIVE_REMOVABLE)
        return;
    const HANDLE volumeHandle =
        CreateFile(devicePath, FILE_READ_ATTRIBUTES,
                   FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0,
                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, //  Volume requires BACKUP_SEMANTICS
                   0);
    if (volumeHandle == INVALID_HANDLE_VALUE) {
        qErrnoWarning("CreateFile %s failed.",
                      qPrintable(QString::fromWCharArray(devicePath)));
        return;
    }

    DEV_BROADCAST_HANDLE notify;
    ZeroMemory(&notify, sizeof(notify));
    notify.dbch_size = sizeof(notify);
    notify.dbch_devicetype = DBT_DEVTYP_HANDLE;
    notify.dbch_handle = volumeHandle;
    QEventDispatcherWin32 *winEventDispatcher = static_cast<QEventDispatcherWin32 *>(QAbstractEventDispatcher::instance());
    re.devNotify = RegisterDeviceNotification(winEventDispatcher->internalHwnd(),
                                              &notify, DEVICE_NOTIFY_WINDOW_HANDLE);
    // Empirically found: The notifications also work when the handle is immediately
    // closed. Do it here to avoid having to close/reopen in lock message handling.
    CloseHandle(volumeHandle);
    if (!re.devNotify) {
        qErrnoWarning("RegisterDeviceNotification %s failed.",
                      qPrintable(QString::fromWCharArray(devicePath)));
        return;
    }

    m_removableDrives.push_back(re);
}
#endif // !Q_OS_WINRT

///////////
// QWindowsFileSystemWatcherEngine
///////////
QWindowsFileSystemWatcherEngine::Handle::Handle()
    : handle(INVALID_HANDLE_VALUE), flags(0u)
{
}

QWindowsFileSystemWatcherEngine::QWindowsFileSystemWatcherEngine(QObject *parent)
    : QFileSystemWatcherEngine(parent)
{
#ifndef Q_OS_WINRT
    if (QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance()) {
        m_driveListener = new QWindowsRemovableDriveListener(this);
        eventDispatcher->installNativeEventFilter(m_driveListener);
        parent->setProperty("_q_driveListener",
                            QVariant::fromValue(static_cast<QObject *>(m_driveListener)));
        QObject::connect(m_driveListener, &QWindowsRemovableDriveListener::driveLockForRemoval,
                         this, &QWindowsFileSystemWatcherEngine::driveLockForRemoval);
        QObject::connect(m_driveListener, &QWindowsRemovableDriveListener::driveLockForRemovalFailed,
                         this, &QWindowsFileSystemWatcherEngine::driveLockForRemovalFailed);
        QObject::connect(m_driveListener,
                         QOverload<const QString &>::of(&QWindowsRemovableDriveListener::driveRemoved),
                         this, &QWindowsFileSystemWatcherEngine::driveRemoved);
    } else {
        qWarning("QFileSystemWatcher: Removable drive notification will not work"
                 " if there is no QCoreApplication instance.");
    }
#endif // !Q_OS_WINRT
}

QWindowsFileSystemWatcherEngine::~QWindowsFileSystemWatcherEngine()
{
    for (auto *thread : qAsConst(threads))
        thread->stop();
    for (auto *thread : qAsConst(threads))
        thread->wait();
    qDeleteAll(threads);
}

QStringList QWindowsFileSystemWatcherEngine::addPaths(const QStringList &paths,
                                                       QStringList *files,
                                                       QStringList *directories)
{
    DEBUG() << "Adding" << paths.count() << "to existing" << (files->count() + directories->count()) << "watchers";
    QStringList p = paths;
    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();
        QString normalPath = path;
        if ((normalPath.endsWith(QLatin1Char('/')) && !normalPath.endsWith(QLatin1String(":/")))
            || (normalPath.endsWith(QLatin1Char('\\')) && !normalPath.endsWith(QLatin1String(":\\")))) {
            normalPath.chop(1);
        }
        QFileInfo fileInfo(normalPath);
        if (!fileInfo.exists())
            continue;

        bool isDir = fileInfo.isDir();
        if (isDir) {
            if (directories->contains(path))
                continue;
        } else {
            if (files->contains(path))
                continue;
        }

        DEBUG() << "Looking for a thread/handle for" << normalPath;

        const QString absolutePath = isDir ? fileInfo.absoluteFilePath() : fileInfo.absolutePath();
        const uint flags = isDir
                           ? (FILE_NOTIFY_CHANGE_DIR_NAME
                              | FILE_NOTIFY_CHANGE_FILE_NAME)
                           : (FILE_NOTIFY_CHANGE_DIR_NAME
                              | FILE_NOTIFY_CHANGE_FILE_NAME
                              | FILE_NOTIFY_CHANGE_ATTRIBUTES
                              | FILE_NOTIFY_CHANGE_SIZE
                              | FILE_NOTIFY_CHANGE_LAST_WRITE
                              | FILE_NOTIFY_CHANGE_SECURITY);

        QWindowsFileSystemWatcherEngine::PathInfo pathInfo;
        pathInfo.absolutePath = absolutePath;
        pathInfo.isDir = isDir;
        pathInfo.path = path;
        pathInfo = fileInfo;

        // Look for a thread
        QWindowsFileSystemWatcherEngineThread *thread = 0;
        QWindowsFileSystemWatcherEngine::Handle handle;
        QList<QWindowsFileSystemWatcherEngineThread *>::const_iterator jt, end;
        end = threads.constEnd();
        for(jt = threads.constBegin(); jt != end; ++jt) {
            thread = *jt;
            QMutexLocker locker(&(thread->mutex));

            const auto hit = thread->handleForDir.find(QFileSystemWatcherPathKey(absolutePath));
            if (hit != thread->handleForDir.end() && hit.value().flags < flags) {
                // Requesting to add a file whose directory has been added previously.
                // Recreate the notification handle to add the missing notification attributes
                // for files (FILE_NOTIFY_CHANGE_ATTRIBUTES...)
                DEBUG() << "recreating" << absolutePath << hex << showbase << hit.value().flags
                    << "->" << flags;
                const Qt::HANDLE fileHandle = createChangeNotification(absolutePath, flags);
                if (fileHandle != INVALID_HANDLE_VALUE) {
                    const int index = thread->handles.indexOf(hit.value().handle);
                    const auto pit = thread->pathInfoForHandle.find(hit.value().handle);
                    Q_ASSERT(index != -1);
                    Q_ASSERT(pit != thread->pathInfoForHandle.end());
                    FindCloseChangeNotification(hit.value().handle);
                    thread->handles[index] = hit.value().handle = fileHandle;
                    hit.value().flags = flags;
                    thread->pathInfoForHandle.insert(fileHandle, pit.value());
                    thread->pathInfoForHandle.erase(pit);
                }
            }
            // In addition, check on flags for sufficient notification attributes
            if (hit != thread->handleForDir.end() && hit.value().flags >= flags) {
                handle = hit.value();
                // found a thread now insert...
                DEBUG() << "Found a thread" << thread;

                QWindowsFileSystemWatcherEngineThread::PathInfoHash &h =
                    thread->pathInfoForHandle[handle.handle];
                const QFileSystemWatcherPathKey key(fileInfo.absoluteFilePath());
                if (!h.contains(key)) {
                    thread->pathInfoForHandle[handle.handle].insert(key, pathInfo);
                    if (isDir)
                        directories->append(path);
                    else
                        files->append(path);
                }
                it.remove();
                thread->wakeup();
                break;
            }
        }

        // no thread found, first create a handle
        if (handle.handle == INVALID_HANDLE_VALUE) {
            DEBUG() << "No thread found";
            handle.handle = createChangeNotification(absolutePath, flags);
            handle.flags = flags;
            if (handle.handle == INVALID_HANDLE_VALUE)
                continue;

            // now look for a thread to insert
            bool found = false;
            for (QWindowsFileSystemWatcherEngineThread *thread : qAsConst(threads)) {
                QMutexLocker locker(&(thread->mutex));
                if (thread->handles.count() < MAXIMUM_WAIT_OBJECTS) {
                    DEBUG() << "Added handle" << handle.handle << "for" << absolutePath << "to watch" << fileInfo.absoluteFilePath()
                            << "to existing thread " << thread;
                    thread->handles.append(handle.handle);
                    thread->handleForDir.insert(QFileSystemWatcherPathKey(absolutePath), handle);

                    thread->pathInfoForHandle[handle.handle].insert(QFileSystemWatcherPathKey(fileInfo.absoluteFilePath()), pathInfo);
                    if (isDir)
                        directories->append(path);
                    else
                        files->append(path);

                    it.remove();
                    found = true;
                    thread->wakeup();
                    break;
                }
            }
            if (!found) {
                QWindowsFileSystemWatcherEngineThread *thread = new QWindowsFileSystemWatcherEngineThread();
                DEBUG() << "  ###Creating new thread" << thread << '(' << (threads.count()+1) << "threads)";
                thread->handles.append(handle.handle);
                thread->handleForDir.insert(QFileSystemWatcherPathKey(absolutePath), handle);

                thread->pathInfoForHandle[handle.handle].insert(QFileSystemWatcherPathKey(fileInfo.absoluteFilePath()), pathInfo);
                if (isDir)
                    directories->append(path);
                else
                    files->append(path);

                connect(thread, SIGNAL(fileChanged(QString,bool)),
                        this, SIGNAL(fileChanged(QString,bool)));
                connect(thread, SIGNAL(directoryChanged(QString,bool)),
                        this, SIGNAL(directoryChanged(QString,bool)));

                thread->msg = '@';
                thread->start();
                threads.append(thread);
                it.remove();
            }
        }
    }

#ifndef Q_OS_WINRT
    if (Q_LIKELY(m_driveListener)) {
        for (const QString &path : paths) {
            if (!p.contains(path))
                m_driveListener->addPath(path);
        }
    }
#endif // !Q_OS_WINRT
    return p;
}

QStringList QWindowsFileSystemWatcherEngine::removePaths(const QStringList &paths,
                                                          QStringList *files,
                                                          QStringList *directories)
{
    DEBUG() << "removePaths" << paths;
    QStringList p = paths;
    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();
        QString normalPath = path;
        if (normalPath.endsWith(QLatin1Char('/')) || normalPath.endsWith(QLatin1Char('\\')))
            normalPath.chop(1);
        QFileInfo fileInfo(normalPath);
        DEBUG() << "removing" << normalPath;
        QString absolutePath = fileInfo.absoluteFilePath();
        QList<QWindowsFileSystemWatcherEngineThread *>::iterator jt, end;
        end = threads.end();
        for(jt = threads.begin(); jt!= end; ++jt) {
            QWindowsFileSystemWatcherEngineThread *thread = *jt;
            if (*jt == 0)
                continue;

            QMutexLocker locker(&(thread->mutex));

            QWindowsFileSystemWatcherEngine::Handle handle = thread->handleForDir.value(QFileSystemWatcherPathKey(absolutePath));
            if (handle.handle == INVALID_HANDLE_VALUE) {
                // perhaps path is a file?
                absolutePath = fileInfo.absolutePath();
                handle = thread->handleForDir.value(QFileSystemWatcherPathKey(absolutePath));
            }
            if (handle.handle != INVALID_HANDLE_VALUE) {
                QWindowsFileSystemWatcherEngineThread::PathInfoHash &h =
                        thread->pathInfoForHandle[handle.handle];
                if (h.remove(QFileSystemWatcherPathKey(fileInfo.absoluteFilePath()))) {
                    // ###
                    files->removeAll(path);
                    directories->removeAll(path);
                    it.remove();

                    if (h.isEmpty()) {
                        DEBUG() << "Closing handle" << handle.handle;
                        FindCloseChangeNotification(handle.handle);    // This one might generate a notification

                        int indexOfHandle = thread->handles.indexOf(handle.handle);
                        Q_ASSERT(indexOfHandle != -1);
                        thread->handles.remove(indexOfHandle);

                        thread->handleForDir.remove(QFileSystemWatcherPathKey(absolutePath));
                        // h is now invalid

                        if (thread->handleForDir.isEmpty()) {
                            DEBUG() << "Stopping thread " << thread;
                            locker.unlock();
                            thread->stop();
                            thread->wait();
                            locker.relock();
                            // We can't delete the thread until the mutex locker is
                            // out of scope
                        }
                    }
                }
                // Found the file, go to next one
                break;
            }
        }
    }

    // Remove all threads that we stopped
    QList<QWindowsFileSystemWatcherEngineThread *>::iterator jt, end;
    end = threads.end();
    for(jt = threads.begin(); jt != end; ++jt) {
        if (!(*jt)->isRunning()) {
            delete *jt;
            *jt = 0;
        }
    }

    threads.removeAll(0);
    return p;
}

///////////
// QWindowsFileSystemWatcherEngineThread
///////////

QWindowsFileSystemWatcherEngineThread::QWindowsFileSystemWatcherEngineThread()
        : msg(0)
{
    if (HANDLE h = CreateEvent(0, false, false, 0)) {
        handles.reserve(MAXIMUM_WAIT_OBJECTS);
        handles.append(h);
    }
}


QWindowsFileSystemWatcherEngineThread::~QWindowsFileSystemWatcherEngineThread()
{
    CloseHandle(handles.at(0));
    handles[0] = INVALID_HANDLE_VALUE;

    for (HANDLE h : qAsConst(handles)) {
        if (h == INVALID_HANDLE_VALUE)
            continue;
        FindCloseChangeNotification(h);
    }
}

static inline QString msgFindNextFailed(const QWindowsFileSystemWatcherEngineThread::PathInfoHash &pathInfos)
{
    QString result;
    QTextStream str(&result);
    str << "QFileSystemWatcher: FindNextChangeNotification failed for";
    for (const QWindowsFileSystemWatcherEngine::PathInfo &pathInfo : pathInfos)
        str << " \"" << QDir::toNativeSeparators(pathInfo.absolutePath) << '"';
    str << ' ';
    return result;
}

void QWindowsFileSystemWatcherEngineThread::run()
{
    QMutexLocker locker(&mutex);
    forever {
        QVector<HANDLE> handlesCopy = handles;
        locker.unlock();
        DEBUG() << "QWindowsFileSystemWatcherThread" << this << "waiting on" << handlesCopy.count() << "handles";
        DWORD r = WaitForMultipleObjects(handlesCopy.count(), handlesCopy.constData(), false, INFINITE);
        locker.relock();
        do {
            if (r == WAIT_OBJECT_0) {
                int m = msg;
                msg = 0;
                if (m == 'q') {
                    DEBUG() << "thread" << this << "told to quit";
                    return;
                }
                if (m != '@')
                    DEBUG() << "QWindowsFileSystemWatcherEngine: unknown message sent to thread: " << char(m);
                break;
            } else if (r > WAIT_OBJECT_0 && r < WAIT_OBJECT_0 + uint(handlesCopy.count())) {
                int at = r - WAIT_OBJECT_0;
                Q_ASSERT(at < handlesCopy.count());
                HANDLE handle = handlesCopy.at(at);

                // When removing a path, FindCloseChangeNotification might actually fire a notification
                // for some reason, so we must check if the handle exist in the handles vector
                if (handles.contains(handle)) {
                    DEBUG() << "thread" << this << "Acknowledged handle:" << at << handle;
                    QWindowsFileSystemWatcherEngineThread::PathInfoHash &h = pathInfoForHandle[handle];
                    bool fakeRemove = false;

                    if (!FindNextChangeNotification(handle)) {
                        const DWORD error = GetLastError();

                        if (error == ERROR_ACCESS_DENIED) {
                            // for directories, our object's handle appears to be woken up when the target of a
                            // watch is deleted, before the watched thing is actually deleted...
                            // anyway.. we're given an error code of ERROR_ACCESS_DENIED in that case.
                            fakeRemove = true;
                        }

                        qErrnoWarning(error, "%s", qPrintable(msgFindNextFailed(h)));
                    }
                    QMutableHashIterator<QFileSystemWatcherPathKey, QWindowsFileSystemWatcherEngine::PathInfo> it(h);
                    while (it.hasNext()) {
                        QWindowsFileSystemWatcherEngineThread::PathInfoHash::iterator x = it.next();
                        QString absolutePath = x.value().absolutePath;
                        QFileInfo fileInfo(x.value().path);
                        DEBUG() << "checking" << x.key();

                        // i'm not completely sure the fileInfo.exist() check will ever work... see QTBUG-2331
                        // ..however, I'm not completely sure enough to remove it.
                        if (fakeRemove || !fileInfo.exists()) {
                            DEBUG() << x.key() << "removed!";
                            if (x.value().isDir)
                                emit directoryChanged(x.value().path, true);
                            else
                                emit fileChanged(x.value().path, true);
                            h.erase(x);

                            // close the notification handle if the directory has been removed
                            if (h.isEmpty()) {
                                DEBUG() << "Thread closing handle" << handle;
                                FindCloseChangeNotification(handle);    // This one might generate a notification

                                int indexOfHandle = handles.indexOf(handle);
                                Q_ASSERT(indexOfHandle != -1);
                                handles.remove(indexOfHandle);

                                handleForDir.remove(QFileSystemWatcherPathKey(absolutePath));
                                // h is now invalid
                            }
                        } else if (x.value().isDir) {
                            DEBUG() << x.key() << "directory changed!";
                            emit directoryChanged(x.value().path, false);
                            x.value() = fileInfo;
                        } else if (x.value() != fileInfo) {
                            DEBUG() << x.key() << "file changed!";
                            emit fileChanged(x.value().path, false);
                            x.value() = fileInfo;
                        }
                    }
                }
            } else {
                // qErrnoWarning("QFileSystemWatcher: error while waiting for change notification");
                break;  // avoid endless loop
            }
            handlesCopy = handles;
            r = WaitForMultipleObjects(handlesCopy.count(), handlesCopy.constData(), false, 0);
        } while (r != WAIT_TIMEOUT);
    }
}


void QWindowsFileSystemWatcherEngineThread::stop()
{
    msg = 'q';
    SetEvent(handles.at(0));
}

void QWindowsFileSystemWatcherEngineThread::wakeup()
{
    msg = '@';
    SetEvent(handles.at(0));
}

QT_END_NAMESPACE

#ifndef Q_OS_WINRT
#  include "qfilesystemwatcher_win.moc"
#endif
