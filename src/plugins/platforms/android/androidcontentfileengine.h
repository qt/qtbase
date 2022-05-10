// Copyright (C) 2019 Volker Krause <vkrause@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDCONTENTFILEENGINE_H
#define ANDROIDCONTENTFILEENGINE_H

#include <private/qfsfileengine_p.h>
#include <QtCore/qjniobject.h>

class AndroidContentFileEngine : public QFSFileEngine
{
public:
    AndroidContentFileEngine(const QString &fileName);
    bool open(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions) override;
    bool close() override;
    qint64 size() const override;
    FileFlags fileFlags(FileFlags type = FileInfoAll) const override;
    QString fileName(FileName file = DefaultName) const override;
    QAbstractFileEngine::Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames) override;
    QAbstractFileEngine::Iterator *endEntryList() override;
private:
    QString m_file;
    QJniObject m_pfd;

};

class AndroidContentFileEngineHandler : public QAbstractFileEngineHandler
{
public:
    AndroidContentFileEngineHandler();
    ~AndroidContentFileEngineHandler();
    QAbstractFileEngine *create(const QString &fileName) const override;
};

class AndroidContentFileEngineIterator : public QAbstractFileEngineIterator
{
public:
    AndroidContentFileEngineIterator(QDir::Filters filters, const QStringList &filterNames);
    ~AndroidContentFileEngineIterator();
    QString next() override;
    bool hasNext() const override;
    QString currentFileName() const override;
private:
    mutable QStringList m_entries;
    mutable int m_index = -1;
};

#endif // ANDROIDCONTENTFILEENGINE_H
