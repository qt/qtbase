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

#ifndef QFILEINFO_H
#define QFILEINFO_H

#include <QtCore/qfile.h>
#include <QtCore/qlist.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


class QDir;
class QDirIteratorPrivate;
class QDateTime;
class QFileInfoPrivate;

class Q_CORE_EXPORT QFileInfo
{
    friend class QDirIteratorPrivate;
public:
    explicit QFileInfo(QFileInfoPrivate *d);

    QFileInfo();
    QFileInfo(const QString &file);
    QFileInfo(const QFile &file);
    QFileInfo(const QDir &dir, const QString &file);
    QFileInfo(const QFileInfo &fileinfo);
    ~QFileInfo();

    QFileInfo &operator=(const QFileInfo &fileinfo);
    QFileInfo &operator=(QFileInfo &&other) noexcept { swap(other); return *this; }

    void swap(QFileInfo &other) noexcept
    { qSwap(d_ptr, other.d_ptr); }

    bool operator==(const QFileInfo &fileinfo) const;
    inline bool operator!=(const QFileInfo &fileinfo) const { return !(operator==(fileinfo)); }

    void setFile(const QString &file);
    void setFile(const QFile &file);
    void setFile(const QDir &dir, const QString &file);
    bool exists() const;
    static bool exists(const QString &file);
    void refresh();

    QString filePath() const;
    QString absoluteFilePath() const;
    QString canonicalFilePath() const;
    QString fileName() const;
    QString baseName() const;
    QString completeBaseName() const;
    QString suffix() const;
    QString bundleName() const;
    QString completeSuffix() const;

    QString path() const;
    QString absolutePath() const;
    QString canonicalPath() const;
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

#if QT_DEPRECATED_SINCE(5, 13)
    QT_DEPRECATED_X("Use QFileInfo::symLinkTarget() instead")
    QString readLink() const;
#endif
    QString symLinkTarget() const;

    QString owner() const;
    uint ownerId() const;
    QString group() const;
    uint groupId() const;

    bool permission(QFile::Permissions permissions) const;
    QFile::Permissions permissions() const;

    qint64 size() const;

    // ### Qt6: inline these functions
#if QT_DEPRECATED_SINCE(5, 10)
    QT_DEPRECATED_X("Use either birthTime() or metadataChangeTime()")
    QDateTime created() const;
#endif
    QDateTime birthTime() const;
    QDateTime metadataChangeTime() const;
    QDateTime lastModified() const;
    QDateTime lastRead() const;
    QDateTime fileTime(QFile::FileTime time) const;

    bool caching() const;
    void setCaching(bool on);

protected:
    QSharedDataPointer<QFileInfoPrivate> d_ptr;

private:
    friend class QFileInfoGatherer;
    void stat();
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
