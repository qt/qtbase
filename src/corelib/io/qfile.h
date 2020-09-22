/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#ifndef QFILE_H
#define QFILE_H

#include <QtCore/qfiledevice.h>
#include <QtCore/qstring.h>
#include <stdio.h>

#if QT_CONFIG(cxx17_filesystem)
#include <filesystem>
#elif defined(Q_CLANG_QDOC)
namespace std {
    namespace filesystem {
        class path {
        };
    };
};
#endif

#ifdef open
#error qfile.h must be included before any header file that defines open
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(cxx17_filesystem)
namespace QtPrivate {
inline QString fromFilesystemPath(const std::filesystem::path &path)
{
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.native());
#else
    return QString::fromStdString(path.native());
#endif
}

inline std::filesystem::path toFilesystemPath(const QString &path)
{
    return std::filesystem::path(reinterpret_cast<const char16_t *>(path.cbegin()),
                                 reinterpret_cast<const char16_t *>(path.cend()));
}

// Both std::filesystem::path and QString (without QT_NO_CAST_FROM_ASCII) can be implicitly
// constructed from string literals so we force the std::fs::path parameter to only
// accept std::fs::path with no implicit conversions.
template<typename T>
using ForceFilesystemPath = typename std::enable_if_t<std::is_same_v<std::filesystem::path, T>, int>;
}
#endif // QT_CONFIG(cxx17_filesystem)

class QTemporaryFile;
class QFilePrivate;

class Q_CORE_EXPORT QFile : public QFileDevice
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#endif
    Q_DECLARE_PRIVATE(QFile)

public:
    QFile();
    QFile(const QString &name);
#ifdef Q_CLANG_QDOC
    QFile(const std::filesystem::path &name);
#elif QT_CONFIG(cxx17_filesystem)
    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    QFile(const T &name) : QFile(QtPrivate::fromFilesystemPath(name))
    {
    }
#endif // QT_CONFIG(cxx17_filesystem)

#ifndef QT_NO_QOBJECT
    explicit QFile(QObject *parent);
    QFile(const QString &name, QObject *parent);

#ifdef Q_CLANG_QDOC
    QFile(const std::filesystem::path &path, QObject *parent);
#elif QT_CONFIG(cxx17_filesystem)
    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    QFile(const T &path, QObject *parent) : QFile(QtPrivate::fromFilesystemPath(path), parent)
    {
    }
#endif // QT_CONFIG(cxx17_filesystem)
#endif // !QT_NO_QOBJECT
    ~QFile();

    QString fileName() const override;
#if QT_CONFIG(cxx17_filesystem) || defined(Q_CLANG_QDOC)
    std::filesystem::path filesystemFileName() const
    { return QtPrivate::toFilesystemPath(fileName()); }
#endif
    void setFileName(const QString &name);
#ifdef Q_CLANG_QDOC
    void setFileName(const std::filesystem::path &name);
#elif QT_CONFIG(cxx17_filesystem)
    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    void setFileName(const T &name)
    {
        setFileName(QtPrivate::fromFilesystemPath(name));
    }
#endif // QT_CONFIG(cxx17_filesystem)

#if defined(Q_OS_DARWIN)
    // Mac always expects filenames in UTF-8... and decomposed...
    static inline QByteArray encodeName(const QString &fileName)
    {
        return fileName.normalized(QString::NormalizationForm_D).toUtf8();
    }
    static QString decodeName(const QByteArray &localFileName)
    {
        // note: duplicated in qglobal.cpp (qEnvironmentVariable)
        return QString::fromUtf8(localFileName).normalized(QString::NormalizationForm_C);
    }
    static inline QString decodeName(const char *localFileName)
    {
        return QString::fromUtf8(localFileName).normalized(QString::NormalizationForm_C);
    }
#else
    static inline QByteArray encodeName(const QString &fileName)
    {
        return fileName.toLocal8Bit();
    }
    static QString decodeName(const QByteArray &localFileName)
    {
        return QString::fromLocal8Bit(localFileName);
    }
    static inline QString decodeName(const char *localFileName)
    {
        return QString::fromLocal8Bit(localFileName);
    }
#endif

    bool exists() const;
    static bool exists(const QString &fileName);

    QString symLinkTarget() const;
    static QString symLinkTarget(const QString &fileName);

    bool remove();
    static bool remove(const QString &fileName);

    bool moveToTrash();
    static bool moveToTrash(const QString &fileName, QString *pathInTrash = nullptr);

    bool rename(const QString &newName);
#ifdef Q_CLANG_QDOC
    bool rename(const std::filesystem::path &newName);
#elif QT_CONFIG(cxx17_filesystem)
    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    bool rename(const T &newName)
    {
        return rename(QtPrivate::fromFilesystemPath(newName));
    }
#endif // QT_CONFIG(cxx17_filesystem)
    static bool rename(const QString &oldName, const QString &newName);

    bool link(const QString &newName);
#ifdef Q_CLANG_QDOC
    bool link(const std::filesystem::path &newName);
#elif QT_CONFIG(cxx17_filesystem)
    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    bool link(const T &newName)
    {
        return link(QtPrivate::fromFilesystemPath(newName));
    }
#endif // QT_CONFIG(cxx17_filesystem)
    static bool link(const QString &oldname, const QString &newName);

    bool copy(const QString &newName);
#ifdef Q_CLANG_QDOC
    bool copy(const std::filesystem::path &newName);
#elif QT_CONFIG(cxx17_filesystem)
    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    bool copy(const T &newName)
    {
        return copy(QtPrivate::fromFilesystemPath(newName));
    }
#endif // QT_CONFIG(cxx17_filesystem)
    static bool copy(const QString &fileName, const QString &newName);

    bool open(OpenMode flags) override;
    bool open(FILE *f, OpenMode ioFlags, FileHandleFlags handleFlags=DontCloseHandle);
    bool open(int fd, OpenMode ioFlags, FileHandleFlags handleFlags=DontCloseHandle);

    qint64 size() const override;

    bool resize(qint64 sz) override;
    static bool resize(const QString &filename, qint64 sz);

    Permissions permissions() const override;
    static Permissions permissions(const QString &filename);
    bool setPermissions(Permissions permissionSpec) override;
    static bool setPermissions(const QString &filename, Permissions permissionSpec);
#ifdef Q_CLANG_QDOC
    static Permissions permissions(const std::filesystem::path &filename);
    static bool setPermissions(const std::filesystem::path &filename, Permissions permissionSpec);
#elif QT_CONFIG(cxx17_filesystem)
    template<typename T,  QtPrivate::ForceFilesystemPath<T> = 0>
    static Permissions permissions(const T &filename)
    {
        return permissions(QtPrivate::fromFilesystemPath(filename));
    }
    template<typename T, QtPrivate::ForceFilesystemPath<T> = 0>
    static bool setPermissions(const T &filename, Permissions permissionSpec)
    {
        return setPermissions(QtPrivate::fromFilesystemPath(filename), permissionSpec);
    }
#endif // QT_CONFIG(cxx17_filesystem)

protected:
#ifdef QT_NO_QOBJECT
    QFile(QFilePrivate &dd);
#else
    QFile(QFilePrivate &dd, QObject *parent = nullptr);
#endif

private:
    friend class QTemporaryFile;
    Q_DISABLE_COPY(QFile)
};

QT_END_NAMESPACE

#endif // QFILE_H
