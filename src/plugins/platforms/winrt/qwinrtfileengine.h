/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINRTFILEENGINE_H
#define QWINRTFILEENGINE_H

#include <private/qabstractfileengine_p.h>

namespace ABI {
    namespace Windows {
        namespace Storage {
            struct IStorageItem;
        }
    }
}

QT_BEGIN_NAMESPACE

class QWinRTFileEngineHandlerPrivate;
class QWinRTFileEngineHandler : public QAbstractFileEngineHandler
{
public:
    QWinRTFileEngineHandler();
    ~QWinRTFileEngineHandler() override = default;
    QAbstractFileEngine *create(const QString &fileName) const override;

    static void registerFile(const QString &fileName, ABI::Windows::Storage::IStorageItem *file);
    static ABI::Windows::Storage::IStorageItem *registeredFile(const QString &fileName);

private:
    QScopedPointer<QWinRTFileEngineHandlerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTFileEngineHandler)
};

class QWinRTFileEnginePrivate;
class QWinRTFileEngine : public QAbstractFileEngine
{
public:
    QWinRTFileEngine(const QString &fileName, ABI::Windows::Storage::IStorageItem *file);
    ~QWinRTFileEngine() override = default;

    bool open(QIODevice::OpenMode openMode) override;
    bool close() override;
    bool flush() override;
    qint64 size() const override;
    bool setSize(qint64 size) override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    bool remove() override;
    bool copy(const QString &newName) override;
    bool rename(const QString &newName) override;
    bool renameOverwrite(const QString &newName) override;
    FileFlags fileFlags(FileFlags type=FileInfoAll) const override;
    bool setPermissions(uint perms) override;
    QString fileName(FileName type=DefaultName) const override;
    QDateTime fileTime(FileTime type) const override;

    qint64 read(char *data, qint64 maxlen) override;
    qint64 write(const char *data, qint64 len) override;

private:
    QScopedPointer<QWinRTFileEnginePrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTFileEngine)
};

QT_END_NAMESPACE

#endif // QWINRTFILEENGINE_H
