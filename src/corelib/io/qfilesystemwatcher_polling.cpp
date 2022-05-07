/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include "qfilesystemwatcher_polling_p.h"
#include <QtCore/qscopeguard.h>
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

QPollingFileSystemWatcherEngine::QPollingFileSystemWatcherEngine(QObject *parent)
    : QFileSystemWatcherEngine(parent),
      timer(this)
{
    connect(&timer, SIGNAL(timeout()), SLOT(timeout()));
}

QStringList QPollingFileSystemWatcherEngine::addPaths(const QStringList &paths,
                                                      QStringList *files,
                                                      QStringList *directories)
{
    QStringList unhandled;
    for (const QString &path : paths) {
        auto sg = qScopeGuard([&]{ unhandled.push_back(path); });
        QFileInfo fi(path);
        if (!fi.exists())
            continue;
        if (fi.isDir()) {
            if (directories->contains(path))
                continue;
            directories->append(path);
            if (!path.endsWith(QLatin1Char('/')))
                fi = QFileInfo(path + QLatin1Char('/'));
            this->directories.insert(path, fi);
        } else {
            if (files->contains(path))
                continue;
            files->append(path);
            this->files.insert(path, fi);
        }
        sg.dismiss();
    }

    if ((!this->files.isEmpty() ||
         !this->directories.isEmpty()) &&
        !timer.isActive()) {
        timer.start(PollingInterval);
    }

    return unhandled;
}

QStringList QPollingFileSystemWatcherEngine::removePaths(const QStringList &paths,
                                                         QStringList *files,
                                                         QStringList *directories)
{
    QStringList unhandled;
    for (const QString &path : paths) {
        if (this->directories.remove(path)) {
            directories->removeAll(path);
        } else if (this->files.remove(path)) {
            files->removeAll(path);
        } else {
            unhandled.push_back(path);
        }
    }

    if (this->files.isEmpty() &&
        this->directories.isEmpty()) {
        timer.stop();
    }

    return unhandled;
}

void QPollingFileSystemWatcherEngine::timeout()
{
    for (auto it = files.begin(), end = files.end(); it != end; /*erasing*/) {
        QString path = it.key();
        QFileInfo fi(path);
        if (!fi.exists()) {
            it = files.erase(it);
            emit fileChanged(path, true);
            continue;
        } else if (it.value() != fi) {
            it.value() = fi;
            emit fileChanged(path, false);
        }
        ++it;
    }

    for (auto it = directories.begin(), end = directories.end(); it != end; /*erasing*/) {
        QString path = it.key();
        QFileInfo fi(path);
        if (!path.endsWith(QLatin1Char('/')))
            fi = QFileInfo(path + QLatin1Char('/'));
        if (!fi.exists()) {
            it = directories.erase(it);
            emit directoryChanged(path, true);
            continue;
        } else if (it.value() != fi) {
            fi.refresh();
            if (!fi.exists()) {
                it = directories.erase(it);
                emit directoryChanged(path, true);
                continue;
            } else {
                it.value() = fi;
                emit directoryChanged(path, false);
            }
        }
        ++it;
    }
}

QT_END_NAMESPACE

#include "moc_qfilesystemwatcher_polling_p.cpp"
