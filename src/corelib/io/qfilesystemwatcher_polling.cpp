// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qfilesystemwatcher_polling_p.h"

#include <QtCore/qlatin1stringview.h>
#include <QtCore/qscopeguard.h>

#include <chrono>

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static constexpr auto PollingInterval = 1s;

QPollingFileSystemWatcherEngine::QPollingFileSystemWatcherEngine(QObject *parent)
    : QFileSystemWatcherEngine(parent)
{
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
            if (!path.endsWith(u'/'))
                fi = QFileInfo(path + u'/');
            this->directories.insert(path, fi);
        } else {
            if (files->contains(path))
                continue;
            files->append(path);
            this->files.insert(path, fi);
        }
        sg.dismiss();
    }

    std::chrono::milliseconds interval = PollingInterval;
#ifdef QT_BUILD_INTERNAL
    if (Q_UNLIKELY(parent()->objectName().startsWith("_qt_autotest_force_engine_"_L1))) {
        interval = 10ms; // Special case to speed up the unittests
    }
#endif

    if ((!this->files.isEmpty() ||
         !this->directories.isEmpty()) &&
        !timer.isActive()) {
            timer.start(interval, this);
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

void QPollingFileSystemWatcherEngine::timerEvent(QTimerEvent *e)
{
    if (e->id() != timer.id())
        return QFileSystemWatcherEngine::timerEvent(e);

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
        if (!path.endsWith(u'/'))
            fi = QFileInfo(path + u'/');
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
