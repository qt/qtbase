// Copyright (C) 2024 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDIRENTRYINFO_P_H
#define QDIRENTRYINFO_P_H

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

#include <QtCore/private/qfileinfo_p.h>
#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>

QT_BEGIN_NAMESPACE

namespace QDirEntryInfoPrivate {
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
};

class QDirEntryInfo
{
    template<typename QueryNative, typename QueryFileInfo, typename QueryIterator>
    auto query(
            QueryNative &&queryNative, QueryFileInfo &&queryFileInfo, QueryIterator &&queryIterator)
    {
        return std::visit(QDirEntryInfoPrivate::overloaded {
            std::forward<QueryNative>(queryNative),
            std::forward<QueryFileInfo>(queryFileInfo),
            [&](const Iterator &iterator) { return queryIterator(iterator.iterator); }
        }, content);
    }

    template<typename QueryNative, typename QueryFileInfo>
    auto query(QueryNative &&queryNative, const QueryFileInfo &queryFileInfo)
    {
        return std::visit(QDirEntryInfoPrivate::overloaded {
            std::forward<QueryNative>(queryNative),
            [&](Iterator &iterator) { return queryFileInfo(iterator.ensureFileInfo()); },
            queryFileInfo,
        }, content);
    }

public:
    QDirEntryInfo() = default;

    explicit QDirEntryInfo(const QAbstractFileEngineIterator *iterator)
        : content(Iterator {iterator, {}})
    {
    }

    explicit QDirEntryInfo(QFileSystemEntry &&e, QFileSystemMetaData &&md)
        : content(Native { std::move(e), std::move(md) })
    {
    }

    explicit QDirEntryInfo(QFileInfo &&info)
        : content(std::move(info))
    {
    }

    const QFileInfo &fileInfo()
    {
        return std::visit(QDirEntryInfoPrivate::overloaded {
            [](Iterator &iterator) -> const QFileInfo & { return iterator.ensureFileInfo(); },
            [this](const Native &native) -> const QFileInfo & {
                content = QFileInfo(new QFileInfoPrivate(native.entry, native.metaData));
                return std::get<QFileInfo>(content);
            },
            [](const QFileInfo &fileInfo) -> const QFileInfo & { return fileInfo; }
        }, content);
    }

    QString fileName()
    {
        return query(
                [](const Native &native) { return native.entry.fileName(); },
                [](const QFileInfo &info) { return info.fileName(); },
                [](const QAbstractFileEngineIterator *it) { return it->currentFileName(); });
    }

    QString baseName()
    {
        return query(
                [](const Native &native) { return native.entry.baseName(); },
                [](const QFileInfo &info) { return info.baseName(); });
    }

    QString completeBaseName()
    {
        return query(
                [](const Native &native) { return native.entry.completeBaseName(); },
                [](const QFileInfo &info) { return info.completeBaseName(); });
    }

    QString suffix()
    {
        return query(
                [](const Native &native) { return native.entry.suffix(); },
                [](const QFileInfo &info) { return info.suffix(); });
    }

    QString completeSuffix()
    {
        return query(
                [](const Native &native) { return native.entry.completeSuffix(); },
                [](const QFileInfo &info) { return info.completeSuffix(); });
    }

    QString filePath()
    {
        return query(
                [](const Native &native) { return native.entry.filePath(); },
                [](const QFileInfo &info) { return info.filePath(); },
                [](const QAbstractFileEngineIterator *it) { return it->currentFilePath(); });
    }

    QString bundleName() { return fileInfo().bundleName(); }

    QString canonicalFilePath()
    {
        // QFileInfo caches these strings
        return fileInfo().canonicalFilePath();
    }

    QString absoluteFilePath()  {
        // QFileInfo caches these strings
        return fileInfo().absoluteFilePath();
    }

    QString absolutePath()  {
        // QFileInfo caches these strings
        return fileInfo().absolutePath();
    }


    bool isDir() {
        return query(
            [](Native &native) {
                return native.ensureFilled(QFileSystemMetaData::DirectoryType).isDirectory();
            },
            [](const QFileInfo &fileInfo) {return fileInfo.isDir(); });
    }

    bool isFile() {
        return query(
            [](Native &native) {
                return native.ensureFilled(QFileSystemMetaData::FileType).isFile();
            },
            [](const QFileInfo &fileInfo) {return fileInfo.isFile(); });
    }

    bool isSymLink() {
        return query(
            [](Native &native) {
                return native.ensureFilled(QFileSystemMetaData::LegacyLinkType).isLegacyLink();
            },
            [](const QFileInfo &fileInfo) {return fileInfo.isSymLink(); });
    }

    bool isSymbolicLink() {
        return query(
            [](Native &native) {
                return native.ensureFilled(QFileSystemMetaData::LinkType).isLink();
            },
            [](const QFileInfo &fileInfo) {return fileInfo.isSymbolicLink(); });
    }

    bool exists() {
        return query(
            [](Native &native) {
                return native.ensureFilled(QFileSystemMetaData::ExistsAttribute).exists();
            },
            [](const QFileInfo &fileInfo) {return fileInfo.exists(); });
    }

    bool isHidden() {
        return query(
            [](Native &native) {
                return native.ensureFilled(QFileSystemMetaData::HiddenAttribute).isHidden();
            },
            [](const QFileInfo &fileInfo) {return fileInfo.isHidden(); });
    }

    bool isReadable() {
        return query(
            [](Native &native) {
                return native.ensureFilled(QFileSystemMetaData::UserReadPermission).isReadable();
            },
            [](const QFileInfo &fileInfo) {return fileInfo.isReadable(); });
    }

    bool isWritable() {
        return query(
            [](Native &native) {
                return native.ensureFilled(QFileSystemMetaData::UserWritePermission).isWritable();
            },
            [](const QFileInfo &fileInfo) {return fileInfo.isWritable(); });
    }

    bool isExecutable() {
        return query(
            [](Native &native) {
                return native.ensureFilled(QFileSystemMetaData::UserExecutePermission)
                            .isExecutable();
            },
            [](const QFileInfo &fileInfo) {return fileInfo.isExecutable(); });
    }

    qint64 size() { return fileInfo().size(); }

    QDateTime fileTime(QFile::FileTime type, const QTimeZone &tz)
    {
        return fileInfo().fileTime(type, tz);
    }

private:
    friend class QDirListingPrivate;
    friend class QDirListing;

    struct Iterator
    {
        // The data we can query from both the QFileInfo and the iterator is cheaper to retrieve
        // from the iterator. Therefore, keep the iterator around even if we create a the QFileInfo.
        const QAbstractFileEngineIterator *iterator = nullptr;
        std::optional<QFileInfo> fileInfo;

        const QFileInfo &ensureFileInfo()
        {
            if (!fileInfo.has_value())
                fileInfo = iterator->currentFileInfo();
            return *fileInfo;
        }
    };

    struct Native
    {
        QFileSystemEntry entry;
        QFileSystemMetaData metaData;

        const QFileSystemMetaData &ensureFilled(QFileSystemMetaData::MetaDataFlag what)
        {
            if (!metaData.hasFlags(what))
                QFileSystemEngine::fillMetaData(entry, metaData, what);
            return metaData;
        }
    };

    std::variant<Native, QFileInfo, Iterator> content;
};

QT_END_NAMESPACE

#endif // QDIRENTRYINFO_P_H
