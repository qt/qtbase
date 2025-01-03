// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFILESYSTEMWATCHER_KQUEUE_P_H
#define QFILESYSTEMWATCHER_KQUEUE_P_H

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

#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qthread.h>

QT_REQUIRE_CONFIG(filesystemwatcher);
struct kevent;

QT_BEGIN_NAMESPACE

class QKqueueFileSystemWatcherEngine : public QFileSystemWatcherEngine
{
    Q_OBJECT
public:
    ~QKqueueFileSystemWatcherEngine();

    static QKqueueFileSystemWatcherEngine *create(QObject *parent);

    QStringList addPaths(const QStringList &paths, QStringList *files,
                         QStringList *directories) override;
    QStringList removePaths(const QStringList &paths, QStringList *files,
                            QStringList *directories) override;

private Q_SLOTS:
    void readFromKqueue();

private:
    QKqueueFileSystemWatcherEngine(int kqfd, QObject *parent);

    int kqfd;

    QHash<QString, int> pathToID;
    QHash<int, QString> idToPath;
    QSocketNotifier notifier;
};

QT_END_NAMESPACE

#endif // QFILESYSTEMWATCHER_KQUEUE_P_H
