// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFILESYSTEMENTRY_P_H
#define QFILESYSTEMENTRY_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class QFileSystemEntry
{
public:

#ifndef Q_OS_WIN
    typedef QByteArray NativePath;
#else
    typedef QString NativePath;
#endif
    struct FromNativePath{};
    struct FromInternalPath{};

    QFileSystemEntry();
    explicit QFileSystemEntry(const QString &filePath);

    QFileSystemEntry(const QString &filePath, FromInternalPath dummy);
    QFileSystemEntry(const NativePath &nativeFilePath, FromNativePath dummy);
    QFileSystemEntry(const QString &filePath, const NativePath &nativeFilePath);

    QString filePath() const;
    QString fileName() const;
    QString path() const;
    NativePath nativeFilePath() const;
    QString baseName() const;
    QString completeBaseName() const;
    QString suffix() const;
    QString completeSuffix() const;
    bool isAbsolute() const;
    bool isRelative() const;
    bool isClean() const;

#if defined(Q_OS_WIN)
    bool isDriveRoot() const;
    static bool isDriveRootPath(const QString &path);
    static QString removeUncOrLongPathPrefix(QString path);
#endif
    bool isRoot() const;

    bool isEmpty() const
    {
        return m_filePath.isEmpty() && m_nativeFilePath.isEmpty();
    }
    void clear()
    {
        *this = QFileSystemEntry();
    }

    Q_CORE_EXPORT static bool isRootPath(const QString &path);

private:
    // creates the QString version out of the bytearray version
    void resolveFilePath() const;
    // creates the bytearray version out of the QString version
    void resolveNativeFilePath() const;
    // resolves the separator
    void findLastSeparator() const;
    // resolves the dots and the separator
    void findFileNameSeparators() const;

    mutable QString m_filePath; // always has slashes as separator
    mutable NativePath m_nativeFilePath; // native encoding and separators

    mutable qint16 m_lastSeparator; // index in m_filePath of last separator
    mutable qint16 m_firstDotInFileName; // index after m_filePath for first dot (.)
    mutable qint16 m_lastDotInFileName; // index after m_firstDotInFileName for last dot (.)
};

QT_END_NAMESPACE

#endif // include guard
