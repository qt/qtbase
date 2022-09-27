// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFILESYSTEMWATCHER_WIN_P_H
#define QFILESYSTEMWATCHER_WIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qfilesystemwatcher_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

class QWindowsFileSystemWatcherEngineThread;
class QWindowsRemovableDriveListener;

// Even though QWindowsFileSystemWatcherEngine is derived of QThread
// via QFileSystemWatcher, it does not start a thread.
// Instead QWindowsFileSystemWatcher creates QWindowsFileSystemWatcherEngineThreads
// to do the actually watching.
class QWindowsFileSystemWatcherEngine : public QFileSystemWatcherEngine
{
    Q_OBJECT
public:
    explicit QWindowsFileSystemWatcherEngine(QObject *parent);
    ~QWindowsFileSystemWatcherEngine();

    QStringList addPaths(const QStringList &paths, QStringList *files, QStringList *directories) override;
    QStringList removePaths(const QStringList &paths, QStringList *files, QStringList *directories) override;

    class Handle
    {
    public:
        Qt::HANDLE handle;
        uint flags;

        Handle();
    };

    class PathInfo {
    public:
        QString absolutePath;
        QString path;
        bool isDir;

        // fileinfo bits
        uint ownerId;
        uint groupId;
        QFile::Permissions permissions;
        QDateTime lastModified;

        PathInfo &operator=(const QFileInfo &fileInfo)
                           {
            ownerId = fileInfo.ownerId();
            groupId = fileInfo.groupId();
            permissions = fileInfo.permissions();
            lastModified = fileInfo.lastModified();
            return *this;
        }

        bool operator!=(const QFileInfo &fileInfo) const
        {
            return (ownerId != fileInfo.ownerId()
                    || groupId != fileInfo.groupId()
                    || permissions != fileInfo.permissions()
                    || lastModified != fileInfo.lastModified());
        }
    };

signals:
    void driveLockForRemoval(const QString &);
    void driveLockForRemovalFailed(const QString &);
    void driveRemoved(const QString &);

private:
    QList<QWindowsFileSystemWatcherEngineThread *> threads;
    QWindowsRemovableDriveListener *m_driveListener = nullptr;
};

class QFileSystemWatcherPathKey : public QString
{
public:
    QFileSystemWatcherPathKey() {}
    explicit QFileSystemWatcherPathKey(const QString &other) : QString(other) {}
    QFileSystemWatcherPathKey(const QFileSystemWatcherPathKey &other) : QString(other) {}
    bool operator==(const QFileSystemWatcherPathKey &other) const { return !compare(other, Qt::CaseInsensitive); }
};

Q_DECLARE_TYPEINFO(QFileSystemWatcherPathKey, Q_RELOCATABLE_TYPE);

inline size_t qHash(const QFileSystemWatcherPathKey &key, size_t seed = 0)
{
    return qHash(key.toCaseFolded(), seed);
}

class QWindowsFileSystemWatcherEngineThread : public QThread
{
    Q_OBJECT

public:
    typedef QHash<QFileSystemWatcherPathKey, QWindowsFileSystemWatcherEngine::Handle> HandleForDirHash;
    typedef QHash<QFileSystemWatcherPathKey, QWindowsFileSystemWatcherEngine::PathInfo> PathInfoHash;

    QWindowsFileSystemWatcherEngineThread();
    ~QWindowsFileSystemWatcherEngineThread();
    void run() override;
    void stop();
    void wakeup();

    QMutex mutex;
    QList<Qt::HANDLE> handles;
    int msg;

    HandleForDirHash handleForDir;

    QHash<Qt::HANDLE, PathInfoHash> pathInfoForHandle;

Q_SIGNALS:
    void fileChanged(const QString &path, bool removed);
    void directoryChanged(const QString &path, bool removed);
};

QT_END_NAMESPACE

#endif // QFILESYSTEMWATCHER_WIN_P_H
