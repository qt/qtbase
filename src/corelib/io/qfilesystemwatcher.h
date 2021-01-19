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

#ifndef QFILESYSTEMWATCHER_H
#define QFILESYSTEMWATCHER_H

#include <QtCore/qobject.h>

QT_REQUIRE_CONFIG(filesystemwatcher);

QT_BEGIN_NAMESPACE


class QFileSystemWatcherPrivate;

class Q_CORE_EXPORT QFileSystemWatcher : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFileSystemWatcher)

public:
    QFileSystemWatcher(QObject *parent = nullptr);
    QFileSystemWatcher(const QStringList &paths, QObject *parent = nullptr);
    ~QFileSystemWatcher();

    bool addPath(const QString &file);
    QStringList addPaths(const QStringList &files);
    bool removePath(const QString &file);
    QStringList removePaths(const QStringList &files);

    QStringList files() const;
    QStringList directories() const;

Q_SIGNALS:
    void fileChanged(const QString &path, QPrivateSignal);
    void directoryChanged(const QString &path, QPrivateSignal);

private:
    Q_PRIVATE_SLOT(d_func(), void _q_fileChanged(const QString &path, bool removed))
    Q_PRIVATE_SLOT(d_func(), void _q_directoryChanged(const QString &path, bool removed))
};

QT_END_NAMESPACE

#endif // QFILESYSTEMWATCHER_H
