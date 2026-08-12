// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QFILESYSTEMENGINE_P_H
#define QFILESYSTEMENGINE_P_H

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

#include "qfile.h"
#include "qfilesystementry_p.h"
#include "qfilesystemmetadata_p.h"
#include <QtCore/qhashfunctions.h>
#include <QtCore/private/qsystemerror_p.h>

#include <array>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

struct QFileSystemNativeId
{
#if defined(Q_OS_WIN)
    // FILE_ID_128 is 128-bit; store as two quint64 to avoid __int128.
    using FileIdType = std::array<quint64, 2>;
#else
    using FileIdType = std::array<quint64, 1>;  // 64-bit inode number
#endif

    quint64 volumeId = 0;       // st_dev (Unix) / volume serial (Windows)
    FileIdType fileId = {};     // st_ino (Unix) / file id (Windows)

    bool isValid() const noexcept { return volumeId != 0 || fileId != FileIdType{}; }

    Q_AUTOTEST_EXPORT QByteArray toByteArray() const;

    friend bool operator==(const QFileSystemNativeId &lhs,
                           const QFileSystemNativeId &rhs) noexcept
    { return lhs.volumeId == rhs.volumeId && lhs.fileId == rhs.fileId; }
    friend bool operator!=(const QFileSystemNativeId &lhs,
                           const QFileSystemNativeId &rhs) noexcept
    { return !(lhs == rhs); }

    friend size_t qHash(const QFileSystemNativeId &id, size_t seed = 0) noexcept
    { return qHashMulti(seed, id.volumeId, qHashRange(id.fileId.begin(), id.fileId.end())); }
};

Q_DECL_COLD_FUNCTION
bool qCheckFileNameFail(const char *msg, const char *file, int line, const char *function);

template <typename E>
bool qCheckFileName(const E &entry, const char *file, int line, const char *function)
{
    if constexpr (std::is_same_v<QFileSystemEntry, E>) {
        return qCheckFileName(entry.nativeFilePath(), file, line, function);
    } else {
        typename E::value_type null = {};
        if (Q_UNLIKELY(entry.isEmpty()))
            return qCheckFileNameFail("Empty filename passed to function", file, line, function);
        if (Q_UNLIKELY(entry.contains(null)))
            return qCheckFileNameFail("Broken filename passed to function", file, line, function);
        return true;
    }
}

#define Q_CHECK_FILE_NAME(name, result) \
    do { \
        if (!qCheckFileName(name, QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC)) \
            return (result); \
    } while (false)

Q_CORE_EXPORT bool qt_isCaseSensitive(const QFileSystemEntry &entry, QFileSystemMetaData &data);

class Q_AUTOTEST_EXPORT QFileSystemEngine
{
public:
    using TriStateResult = QAbstractFileEngine::TriStateResult;

    static bool isCaseSensitive(const QFileSystemEntry &entry, QFileSystemMetaData &data);

    static QFileSystemEntry getLinkTarget(const QFileSystemEntry &link, QFileSystemMetaData &data);
    static QFileSystemEntry getRawLinkPath(const QFileSystemEntry &link,
                                           QFileSystemMetaData &data);
    static QFileSystemEntry getJunctionTarget(const QFileSystemEntry &link, QFileSystemMetaData &data);
    static QFileSystemEntry canonicalName(const QFileSystemEntry &entry, QFileSystemMetaData &data);
    static QFileSystemEntry absoluteName(const QFileSystemEntry &entry);
    static QByteArray id(const QFileSystemEntry &entry) { return nativeId(entry).toByteArray(); }
    static QFileSystemNativeId nativeId(const QFileSystemEntry &entry);
    static QString resolveUserName(const QFileSystemEntry &entry, QFileSystemMetaData &data);
    static QString resolveGroupName(const QFileSystemEntry &entry, QFileSystemMetaData &data);

#if defined(Q_OS_UNIX)
    static QString resolveUserName(uint userId);
    static QString resolveGroupName(uint groupId);
#endif

#if defined(Q_OS_DARWIN)
    static QString bundleName(const QFileSystemEntry &entry);
#else
    static QString bundleName(const QFileSystemEntry &) { return QString(); }
#endif

    static bool fillMetaData(const QFileSystemEntry &entry, QFileSystemMetaData &data,
                             QFileSystemMetaData::MetaDataFlags what);
#if defined(Q_OS_UNIX)
    static TriStateResult cloneFile(int srcfd, int dstfd, const QFileSystemMetaData &knownData);
    static bool fillMetaData(int fd, QFileSystemMetaData &data); // what = PosixStatFlags
    static QByteArray id(int fd) { return nativeId(fd).toByteArray(); }
    static QFileSystemNativeId nativeId(int fd);
    static bool setFileTime(int fd, const QDateTime &newDate,
                            QFile::FileTime whatTime, QSystemError &error);
    static bool setPermissions(int fd, QFile::Permissions permissions, QSystemError &error);
#endif
#if defined(Q_OS_WIN)
    static QFileSystemEntry junctionTarget(const QFileSystemEntry &link, QFileSystemMetaData &data);
    static bool uncListSharesOnServer(const QString &server, QStringList *list); //Used also by QFSFileEngineIterator::hasNext()
    static bool fillMetaData(int fd, QFileSystemMetaData &data,
                             QFileSystemMetaData::MetaDataFlags what = QFileSystemMetaData::WinStatFlags);
    static bool fillMetaData(HANDLE fHandle, QFileSystemMetaData &data,
                             QFileSystemMetaData::MetaDataFlags what);
    static bool fillPermissions(const QFileSystemEntry &entry, QFileSystemMetaData &data,
                                QFileSystemMetaData::MetaDataFlags what);
    static QByteArray id(HANDLE fHandle) { return nativeId(fHandle).toByteArray(); }
    static QFileSystemNativeId nativeId(HANDLE fHandle);
    static bool setFileTime(HANDLE fHandle, const QDateTime &newDate,
                            QFile::FileTime whatTime, QSystemError &error);
    static QString owner(const QFileSystemEntry &entry, QAbstractFileEngine::FileOwner own);
    static QString nativeAbsoluteFilePath(const QString &path);
    static bool isDirPath(const QString &path, bool *existed);
#endif
    //homePath, rootPath and tempPath shall return clean paths
    static QString homePath();
    static QString rootPath();
    static QString tempPath();

    static bool createDirectory(const QFileSystemEntry &entry, bool createParents,
                                std::optional<QFile::Permissions> permissions = std::nullopt)
    {
        if (createParents)
            return mkpath(entry, permissions);
        return mkdir(entry, permissions);
    }

    static bool mkdir(const QFileSystemEntry &entry,
                      std::optional<QFile::Permissions> permissions = std::nullopt);
    static bool mkpath(const QFileSystemEntry &entry,
                       std::optional<QFile::Permissions> permissions = std::nullopt);

    static bool removeDirectory(const QFileSystemEntry &entry, bool removeEmptyParents)
    {
        if (removeEmptyParents)
            return rmpath(entry);
        return rmdir(entry);
    }

    static bool rmdir(const QFileSystemEntry &entry);
    static bool rmpath(const QFileSystemEntry &entry);
    static bool supportsRmdirRecursively() noexcept;
    static bool rmdirRecursively(const QFileSystemEntry &entry, QSystemError &error);

    static bool createLink(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error);

    static bool copyFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error);
    static bool supportsMoveFileToTrash();
    static bool moveFileToTrash(const QFileSystemEntry &source, QFileSystemEntry &newLocation, QSystemError &error);
    static bool renameFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error);
    static bool renameOverwriteFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error);
    static bool removeFile(const QFileSystemEntry &entry, QSystemError &error);

    static bool setPermissions(const QFileSystemEntry &entry, QFile::Permissions permissions,
                               QSystemError &error);

    // unused, therefore not implemented
    static bool setFileTime(const QFileSystemEntry &entry, const QDateTime &newDate,
                            QFile::FileTime whatTime, QSystemError &error);

    static bool setCurrentPath(const QFileSystemEntry &entry);
    static QFileSystemEntry currentPath();

    static std::unique_ptr<QAbstractFileEngine>
    createLegacyEngine(QFileSystemEntry &entry, QFileSystemMetaData &data);
    static std::unique_ptr<QAbstractFileEngine>
    createLegacyEngine(const QString &fileName);

private:
    static QString slowCanonicalized(const QString &path);
#if defined(Q_OS_WIN)
    static void clearWinStatData(QFileSystemMetaData &data);
#endif
};

QT_END_NAMESPACE

#endif // include guard
