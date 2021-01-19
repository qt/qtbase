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

#ifndef QFILESYSTEMWATCHER_INOTIFY_P_H
#define QFILESYSTEMWATCHER_INOTIFY_P_H

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

QT_REQUIRE_CONFIG(filesystemwatcher);

#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>
#include <QtCore/qsocketnotifier.h>

QT_BEGIN_NAMESPACE

class QInotifyFileSystemWatcherEngine : public QFileSystemWatcherEngine
{
    Q_OBJECT

public:
    ~QInotifyFileSystemWatcherEngine();

    static QInotifyFileSystemWatcherEngine *create(QObject *parent);

    QStringList addPaths(const QStringList &paths, QStringList *files, QStringList *directories) override;
    QStringList removePaths(const QStringList &paths, QStringList *files, QStringList *directories) override;

private Q_SLOTS:
    void readFromInotify();

private:
    QString getPathFromID(int id) const;

private:
    QInotifyFileSystemWatcherEngine(int fd, QObject *parent);
    int inotifyFd;
    QHash<QString, int> pathToID;
    QMultiHash<int, QString> idToPath;
    QSocketNotifier notifier;
};


QT_END_NAMESPACE
#endif // QFILESYSTEMWATCHER_INOTIFY_P_H
