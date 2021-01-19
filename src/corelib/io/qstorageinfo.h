/****************************************************************************
**
** Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
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

#ifndef QSTORAGEINFO_H
#define QSTORAGEINFO_H

#include <QtCore/qbytearray.h>
#include <QtCore/qdir.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QDebug;

class QStorageInfoPrivate;
class Q_CORE_EXPORT QStorageInfo
{
public:
    QStorageInfo();
    explicit QStorageInfo(const QString &path);
    explicit QStorageInfo(const QDir &dir);
    QStorageInfo(const QStorageInfo &other);
    ~QStorageInfo();

    QStorageInfo &operator=(const QStorageInfo &other);
    QStorageInfo &operator=(QStorageInfo &&other) noexcept { swap(other); return *this; }

    inline void swap(QStorageInfo &other) noexcept
    { qSwap(d, other.d); }

    void setPath(const QString &path);

    QString rootPath() const;
    QByteArray device() const;
    QByteArray subvolume() const;
    QByteArray fileSystemType() const;
    QString name() const;
    QString displayName() const;

    qint64 bytesTotal() const;
    qint64 bytesFree() const;
    qint64 bytesAvailable() const;
    int blockSize() const;

    inline bool isRoot() const;
    bool isReadOnly() const;
    bool isReady() const;
    bool isValid() const;

    void refresh();

    static QList<QStorageInfo> mountedVolumes();
    static QStorageInfo root();

private:
    friend class QStorageInfoPrivate;
    friend bool operator==(const QStorageInfo &first, const QStorageInfo &second);
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QStorageInfo &);
    QExplicitlySharedDataPointer<QStorageInfoPrivate> d;
};

inline bool operator==(const QStorageInfo &first, const QStorageInfo &second)
{
    if (first.d == second.d)
        return true;
    return first.device() == second.device() && first.rootPath() == second.rootPath();
}

inline bool operator!=(const QStorageInfo &first, const QStorageInfo &second)
{
    return !(first == second);
}

inline bool QStorageInfo::isRoot() const
{ return *this == QStorageInfo::root(); }

Q_DECLARE_SHARED(QStorageInfo)

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug debug, const QStorageInfo &);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QStorageInfo)

#endif // QSTORAGEINFO_H
