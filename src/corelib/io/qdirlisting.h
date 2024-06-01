// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2024 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDILISTING_H
#define QDILISTING_H

#include <QtCore/qfiledevice.h>
#include <QtCore/qflags.h>
#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qtcoreexports.h>
#include <QtCore/qdatetime.h>

#include <iterator>
#include <memory>

QT_BEGIN_NAMESPACE

class QDirListingPrivate;
class QFileInfo;
class QDir;
class QTimeZone;

class Q_CORE_EXPORT QDirListing
{
public:
    enum class IteratorFlag {
        Default =               0x000000,
        ExcludeFiles =          0x000004,
        ExcludeDirs =           0x000008,
        ExcludeSpecial =        0x000010,
        ResolveSymlinks =       0x000020,
        FilesOnly =             ExcludeDirs  | ExcludeSpecial,
        DirsOnly =              ExcludeFiles | ExcludeSpecial,
        IncludeHidden =         0x000040,
        IncludeDotAndDotDot =   0x000080,
        CaseSensitive =         0x000100,
        Recursive =             0x000400,
        FollowDirSymlinks =     0x000800,
    };
    Q_DECLARE_FLAGS(IteratorFlags, IteratorFlag)

    explicit QDirListing(const QString &path, IteratorFlags flags = IteratorFlag::Default);
    explicit QDirListing(const QString &path, const QStringList &nameFilters,
                         IteratorFlags flags = IteratorFlag::Default);

    QDirListing(QDirListing &&);
    QDirListing &operator=(QDirListing &&);

    ~QDirListing();

    QString iteratorPath() const;
    IteratorFlags iteratorFlags() const;
    QStringList nameFilters() const;

    class Q_CORE_EXPORT DirEntry
    {
        friend class QDirListing;
        QDirListingPrivate *dirListPtr = nullptr;
    public:
        QString fileName() const;
        QString baseName() const;
        QString completeBaseName() const;
        QString suffix() const;
        QString bundleName() const;
        QString completeSuffix() const;
        QString filePath() const;
        bool isDir() const;
        bool isFile() const;
        bool isSymLink() const;
        bool exists() const;
        bool isHidden() const;
        bool isReadable() const;
        bool isWritable() const;
        bool isExecutable() const;
        QFileInfo fileInfo() const;
        QString canonicalFilePath() const;
        QString absoluteFilePath() const;
        QString absolutePath() const;
        qint64 size() const;

        QDateTime birthTime(const QTimeZone &tz) const
        { return fileTime(QFileDevice::FileBirthTime, tz); }
        QDateTime metadataChangeTime(const QTimeZone &tz) const
        { return fileTime(QFileDevice::FileMetadataChangeTime, tz); }
        QDateTime lastModified(const QTimeZone &tz) const
        { return fileTime(QFileDevice::FileModificationTime, tz); }
        QDateTime lastRead(const QTimeZone &tz) const
        { return fileTime(QFileDevice::FileAccessTime, tz); }
        QDateTime fileTime(QFileDevice::FileTime type, const QTimeZone &tz) const;
    };

    class const_iterator
    {
        friend class QDirListing;
        const_iterator(QDirListingPrivate *dp) : dirListPtr(dp) { dirEntry.dirListPtr = dp; }
        QDirListingPrivate *dirListPtr = nullptr;
        DirEntry dirEntry;
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = DirEntry;
        using difference_type = qint64;
        using pointer = const value_type *;
        using reference = const value_type &;

        const_iterator() = default;
        reference operator*() const { return dirEntry; }
        pointer operator->() const { return &dirEntry; }
        Q_CORE_EXPORT const_iterator &operator++();
        const_iterator operator++(int) { auto tmp = *this; operator++(); return tmp; };
        friend bool operator==(const const_iterator &lhs, const const_iterator &rhs)
        {
            // This is only used for the sentinel end iterator
            return lhs.dirListPtr == nullptr && rhs.dirListPtr == nullptr;
        }
        friend bool operator!=(const const_iterator &lhs, const const_iterator &rhs)
        { return !(lhs == rhs); }
    };

    const_iterator begin() const;
    const_iterator cbegin() const { return begin(); }
    const_iterator end() const { return {}; }
    const_iterator cend() const { return end(); }

    // Qt compatibility
    const_iterator constBegin() const { return begin(); }
    const_iterator constEnd() const { return end(); }

private:
    Q_DISABLE_COPY(QDirListing)

    // Private constructor that is used in deprecated code paths.
    // `uint` instead of QDir::Filters and QDirIterator::IteratorFlags
    // because qdir.h can't be included here; qdiriterator.h can't included
    // either, because it includes qdir.h
    QDirListing(const QString &path, const QStringList &nameFilters, uint dirFilters,
                uint qdirIteratorFlags = 0); // QDirIterator::NoIteratorFlags == 0x0

    std::unique_ptr<QDirListingPrivate> d;
    friend class QDir;
    friend class QDirPrivate;
    friend class QDirIteratorPrivate;
    friend class QAbstractFileEngine;
    friend class QFileInfoGatherer;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDirListing::IteratorFlags)

QT_END_NAMESPACE

#endif // QDILISTING_H
