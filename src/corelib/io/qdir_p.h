/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDIR_P_H
#define QDIR_P_H

#include "qfilesystementry_p.h"
#include "qfilesystemmetadata_p.h"

QT_BEGIN_NAMESPACE

class QDirPrivate : public QSharedData
{
public:
    explicit QDirPrivate(const QString &path, const QStringList &nameFilters_ = QStringList(),
                         QDir::SortFlags sort_ = QDir::SortFlags(QDir::Name | QDir::IgnoreCase),
                         QDir::Filters filters_ = QDir::AllEntries);

    explicit QDirPrivate(const QDirPrivate &copy);

    bool exists() const;

    void initFileEngine();
    void initFileLists(const QDir &dir) const;

    static void sortFileList(QDir::SortFlags, QFileInfoList &, QStringList *, QFileInfoList *);

    static inline QChar getFilterSepChar(const QString &nameFilter);

    static inline QStringList splitFilters(const QString &nameFilter, QChar sep = 0);

    void setPath(const QString &path);

    void clearFileLists();

    void resolveAbsoluteEntry() const;

    QStringList nameFilters;
    QDir::SortFlags sort;
    QDir::Filters filters;

    QScopedPointer<QAbstractFileEngine> fileEngine;

    mutable bool fileListsInitialized;
    mutable QStringList files;
    mutable QFileInfoList fileInfos;

    QFileSystemEntry dirEntry;
    mutable QFileSystemEntry absoluteDirEntry;
    mutable QFileSystemMetaData metaData;
};

QT_END_NAMESPACE

#endif
