/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFILEINFO_H
#define QFILEINFO_H

#include <QtCore/qfile.h>
#include <QtCore/qlist.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE


class QDir;
class QDirIteratorPrivate;
class QFileInfoPrivate;

class Q_CORE_EXPORT QFileInfo
{
    friend class QDirIteratorPrivate;
public:
    explicit QFileInfo(QFileInfoPrivate *d);

#ifdef QT_IMPLICIT_QFILEINFO_CONSTRUCTION
#define QFILEINFO_MAYBE_EXPLICIT Q_IMPLICIT
#else
#define QFILEINFO_MAYBE_EXPLICIT explicit
#endif

    QFileInfo();
    QFILEINFO_MAYBE_EXPLICIT QFileInfo(const QString &file);
    QFILEINFO_MAYBE_EXPLICIT QFileInfo(const QFileDevice &file);
    QFILEINFO_MAYBE_EXPLICIT QFileInfo(const QDir &dir, const QString &file);
    QFileInfo(const QFileInfo &fileinfo);
#ifdef Q_CLANG_QDOC
    QFileInfo(const std::filesystem::path &file);
    QFileInfo(const QDir &dir, const std::filesystem::path &file);
#elif QT_CONFIG(cxx17_filesystem)
    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    QFILEINFO_MAYBE_EXPLICIT QFileInfo(const T &file) : QFileInfo(QtPrivate::fromFilesystemPath(file)) { }

    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    QFILEINFO_MAYBE_EXPLICIT QFileInfo(const QDir &dir, const T &file) : QFileInfo(dir, QtPrivate::fromFilesystemPath(file))
    {
    }
#endif // QT_CONFIG(cxx17_filesystem)

#undef QFILEINFO_MAYBE_EXPLICIT

    ~QFileInfo();

    QFileInfo &operator=(const QFileInfo &fileinfo);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QFileInfo)

    void swap(QFileInfo &other) noexcept
    { qSwap(d_ptr, other.d_ptr); }

    bool operator==(const QFileInfo &fileinfo) const;
    inline bool operator!=(const QFileInfo &fileinfo) const { return !(operator==(fileinfo)); }

    void setFile(const QString &file);
    void setFile(const QFileDevice &file);
    void setFile(const QDir &dir, const QString &file);
#ifdef Q_CLANG_QDOC
    void setFile(const std::filesystem::path &file);
#elif QT_CONFIG(cxx17_filesystem)
    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    void setFile(const T &file) { setFile(QtPrivate::fromFilesystemPath(file)); }
#endif // QT_CONFIG(cxx17_filesystem)

    bool exists() const;
    static bool exists(const QString &file);
    void refresh();

    QString filePath() const;
    QString absoluteFilePath() const;
    QString canonicalFilePath() const;
#if QT_CONFIG(cxx17_filesystem) || defined(Q_CLANG_QDOC)
    std::filesystem::path filesystemFilePath() const
    { return QtPrivate::toFilesystemPath(filePath()); }
    std::filesystem::path filesystemAbsoluteFilePath() const
    { return QtPrivate::toFilesystemPath(absoluteFilePath()); }
    std::filesystem::path filesystemCanonicalFilePath() const
    { return QtPrivate::toFilesystemPath(canonicalFilePath()); }
#endif // QT_CONFIG(cxx17_filesystem)
    QString fileName() const;
    QString baseName() const;
    QString completeBaseName() const;
    QString suffix() const;
    QString bundleName() const;
    QString completeSuffix() const;

    QString path() const;
    QString absolutePath() const;
    QString canonicalPath() const;
#if QT_CONFIG(cxx17_filesystem) || defined(Q_CLANG_QDOC)
    std::filesystem::path filesystemPath() const { return QtPrivate::toFilesystemPath(path()); }
    std::filesystem::path filesystemAbsolutePath() const
    { return QtPrivate::toFilesystemPath(absolutePath()); }
    std::filesystem::path filesystemCanonicalPath() const
    { return QtPrivate::toFilesystemPath(canonicalPath()); }
#endif // QT_CONFIG(cxx17_filesystem)
    QDir dir() const;
    QDir absoluteDir() const;

    bool isReadable() const;
    bool isWritable() const;
    bool isExecutable() const;
    bool isHidden() const;
    bool isNativePath() const;

    bool isRelative() const;
    inline bool isAbsolute() const { return !isRelative(); }
    bool makeAbsolute();

    bool isFile() const;
    bool isDir() const;
    bool isSymLink() const;
    bool isSymbolicLink() const;
    bool isShortcut() const;
    bool isJunction() const;
    bool isRoot() const;
    bool isBundle() const;

    QString symLinkTarget() const;
#if QT_CONFIG(cxx17_filesystem) || defined(Q_CLANG_QDOC)
    std::filesystem::path filesystemSymLinkTarget() const
    { return QtPrivate::toFilesystemPath(symLinkTarget()); }
#endif // QT_CONFIG(cxx17_filesystem)

    QString owner() const;
    uint ownerId() const;
    QString group() const;
    uint groupId() const;

    bool permission(QFile::Permissions permissions) const;
    QFile::Permissions permissions() const;

    qint64 size() const;

    QDateTime birthTime() const { return fileTime(QFile::FileBirthTime); }
    QDateTime metadataChangeTime() const { return fileTime(QFile::FileMetadataChangeTime); }
    QDateTime lastModified() const { return fileTime(QFile::FileModificationTime); }
    QDateTime lastRead() const { return fileTime(QFile::FileAccessTime); }
    QDateTime fileTime(QFile::FileTime time) const;

    bool caching() const;
    void setCaching(bool on);
    void stat();

protected:
    QSharedDataPointer<QFileInfoPrivate> d_ptr;

private:
    QFileInfoPrivate* d_func();
    inline const QFileInfoPrivate* d_func() const
    {
        return d_ptr.constData();
    }
};

Q_DECLARE_SHARED(QFileInfo)

typedef QList<QFileInfo> QFileInfoList;

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QFileInfo &);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QFileInfo)

#endif // QFILEINFO_H
