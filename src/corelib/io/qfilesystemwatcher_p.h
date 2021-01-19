/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QFILESYSTEMWATCHER_P_H
#define QFILESYSTEMWATCHER_P_H

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

#include "qfilesystemwatcher.h"

QT_REQUIRE_CONFIG(filesystemwatcher);

#include <private/qobject_p.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

class QFileSystemWatcherEngine : public QObject
{
    Q_OBJECT

protected:
    inline QFileSystemWatcherEngine(QObject *parent)
        : QObject(parent)
    {
    }

public:
    // fills \a files and \a directories with the \a paths it could
    // watch, and returns a list of paths this engine could not watch
    virtual QStringList addPaths(const QStringList &paths,
                                 QStringList *files,
                                 QStringList *directories) = 0;
    // removes \a paths from \a files and \a directories, and returns
    // a list of paths this engine does not know about (either addPath
    // failed or wasn't called)
    virtual QStringList removePaths(const QStringList &paths,
                                    QStringList *files,
                                    QStringList *directories) = 0;

Q_SIGNALS:
    void fileChanged(const QString &path, bool removed);
    void directoryChanged(const QString &path, bool removed);
};

class QFileSystemWatcherPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QFileSystemWatcher)

    static QFileSystemWatcherEngine *createNativeEngine(QObject *parent);

public:
    QFileSystemWatcherPrivate();
    void init();
    void initPollerEngine();

    QFileSystemWatcherEngine *native, *poller;
    QStringList files, directories;

    // private slots
    void _q_fileChanged(const QString &path, bool removed);
    void _q_directoryChanged(const QString &path, bool removed);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    void _q_winDriveLockForRemoval(const QString &);
    void _q_winDriveLockForRemovalFailed(const QString &);
    void _q_winDriveRemoved(const QString &);

private:
    QHash<QChar, QStringList> temporarilyRemovedPaths;
#endif // Q_OS_WIN && !Q_OS_WINRT
};


QT_END_NAMESPACE
#endif // QFILESYSTEMWATCHER_P_H
