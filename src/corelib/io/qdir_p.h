// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDIR_P_H
#define QDIR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qfilesystementry_p.h"
#include "qfilesystemmetadata_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

class QDirPrivate : public QSharedData
{
public:
    enum PathNormalization {
        DefaultNormalization = 0x00,
        AllowUncPaths = 0x01,
        RemotePath = 0x02
    };
    Q_DECLARE_FLAGS(PathNormalizations, PathNormalization)
    Q_FLAGS(PathNormalizations)

    explicit QDirPrivate(const QString &path, const QStringList &nameFilters_ = QStringList(),
                         QDir::SortFlags sort_ = QDir::SortFlags(QDir::Name | QDir::IgnoreCase),
                         QDir::Filters filters_ = QDir::AllEntries);

    explicit QDirPrivate(const QDirPrivate &copy);

    bool exists() const;

    void initFileEngine();
    void initFileLists(const QDir &dir) const;

    static void sortFileList(QDir::SortFlags, const QFileInfoList &, QStringList *, QFileInfoList *);

    static inline QChar getFilterSepChar(const QString &nameFilter);

    static inline QStringList splitFilters(const QString &nameFilter, QChar sep = {});

    void setPath(const QString &path);

    void clearFileLists();

    void resolveAbsoluteEntry() const;

    mutable bool fileListsInitialized;
    mutable QStringList files;
    mutable QFileInfoList fileInfos;

    QStringList nameFilters;
    QDir::SortFlags sort;
    QDir::Filters filters;

    std::unique_ptr<QAbstractFileEngine> fileEngine;

    QFileSystemEntry dirEntry;
    mutable QFileSystemEntry absoluteDirEntry;
    mutable QFileSystemMetaData metaData;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDirPrivate::PathNormalizations)

Q_AUTOTEST_EXPORT QString qt_normalizePathSegments(const QString &name, QDirPrivate::PathNormalizations flags, bool *ok = nullptr);

QT_END_NAMESPACE

#endif
