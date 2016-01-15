/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
