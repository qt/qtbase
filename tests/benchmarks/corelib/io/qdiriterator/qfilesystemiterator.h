// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QFILESYSTEMITERATOR_H
#define QFILESYSTEMITERATOR_H

#include <QtCore/qdir.h>

QT_BEGIN_NAMESPACE

namespace QDirIteratorTest {

class QFileSystemIteratorPrivate;
class //Q_CORE_EXPORT
QFileSystemIterator
{
public:
    enum IteratorFlag {
        NoIteratorFlags = 0x0,
        FollowSymlinks = 0x1,
        Subdirectories = 0x2
    };
    Q_DECLARE_FLAGS(IteratorFlags, IteratorFlag)

    QFileSystemIterator(const QDir &dir, IteratorFlags flags = NoIteratorFlags);
    QFileSystemIterator(const QString &path,
                 IteratorFlags flags = NoIteratorFlags);
    QFileSystemIterator(const QString &path,
                 QDir::Filters filter,
                 IteratorFlags flags = NoIteratorFlags);
    QFileSystemIterator(const QString &path,
                 const QStringList &nameFilters,
                 QDir::Filters filters = QDir::NoFilter,
                 IteratorFlags flags = NoIteratorFlags);

    virtual ~QFileSystemIterator();

    void next();
    bool atEnd() const;

    QString fileName() const;
    QString filePath() const;
    QFileInfo fileInfo() const;
    QString path() const;

private:
    Q_DISABLE_COPY(QFileSystemIterator)

    QFileSystemIteratorPrivate *d;
    friend class QDir;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFileSystemIterator::IteratorFlags)

} // namespace QDirIteratorTest

QT_END_NAMESPACE

#endif
