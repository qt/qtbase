/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    ~QWinRTFileEngineHandler();
    QAbstractFileEngine *create(const QString &fileName) const Q_DECL_OVERRIDE;

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
    ~QWinRTFileEngine();

    bool open(QIODevice::OpenMode openMode) Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;
    bool flush() Q_DECL_OVERRIDE;
    qint64 size() const Q_DECL_OVERRIDE;
    qint64 pos() const Q_DECL_OVERRIDE;
    bool seek(qint64 pos) Q_DECL_OVERRIDE;
    bool remove() Q_DECL_OVERRIDE;
    bool copy(const QString &newName) Q_DECL_OVERRIDE;
    bool rename(const QString &newName) Q_DECL_OVERRIDE;
    bool renameOverwrite(const QString &newName) Q_DECL_OVERRIDE;
    FileFlags fileFlags(FileFlags type=FileInfoAll) const Q_DECL_OVERRIDE;
    bool setPermissions(uint perms) Q_DECL_OVERRIDE;
    QString fileName(FileName type=DefaultName) const Q_DECL_OVERRIDE;
    QDateTime fileTime(FileTime type) const Q_DECL_OVERRIDE;

    qint64 read(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 write(const char *data, qint64 len) Q_DECL_OVERRIDE;

private:
    QScopedPointer<QWinRTFileEnginePrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTFileEngine)
};

QT_END_NAMESPACE

#endif // QWINRTFILEENGINE_H
