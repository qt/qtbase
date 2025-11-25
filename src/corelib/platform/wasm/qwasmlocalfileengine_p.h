// Copyright (C) 2024 Qt Group
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWASMLOCALFILEENGINE_P_H
#define QWASMLOCALFILEENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qabstractfileengine_p.h>
#include <QtCore/private/qstdweb_p.h>

QT_BEGIN_NAMESPACE

class QWasmFileEngine;

class QWasmFileEngineHandler: public QAbstractFileEngineHandler
{
public:
    Q_DISABLE_COPY_MOVE(QWasmFileEngineHandler)

    QWasmFileEngineHandler();
    virtual ~QWasmFileEngineHandler() override;
    virtual std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const override;

    static bool isWasmFileName(const QString& fileName);
    static QString makeWasmFileName(const QString &fileName);
    static QString nativeFileName(const QString &wasmFileName);

    static QString addFile(qstdweb::File file);
    static QString addFile(qstdweb::FileSystemFileHandle file);
    static void removeFile(const QString fileName);
    qstdweb::File getFile(const QString fileName);
    qstdweb::FileSystemFileHandle getFileSystemFile(const QString fileName);

private:
    QHash<QString, qstdweb::File> m_files;
    QHash<QString, qstdweb::FileSystemFileHandle> m_fileSystemFiles;
};

class QWasmFileEngine: public QAbstractFileEngine
{
public:
    explicit QWasmFileEngine(const QString &fileName, qstdweb::File file);
    explicit QWasmFileEngine(const QString &fileName, qstdweb::FileSystemFileHandle file);
    ~QWasmFileEngine() override;

    virtual bool open(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions) override;
    virtual bool close() override;
    virtual bool flush() override;
    virtual bool syncToDisk() override;
    virtual qint64 size() const override;
    virtual qint64 pos() const override;
    virtual bool seek(qint64 pos) override;
    virtual bool isSequential() const override;
    virtual bool remove() override;
    virtual bool copy(const QString &newName) override;
    virtual bool rename(const QString &newName) override;
    virtual bool renameOverwrite(const QString &newName) override;
    virtual bool link(const QString &newName) override;
    virtual bool mkdir(const QString &dirName, bool createParentDirectories,
                       std::optional<QFile::Permissions> permissions = std::nullopt) const override;
    virtual bool rmdir(const QString &dirName, bool recurseParentDirectories) const override;
    virtual bool setSize(qint64 size) override;
    virtual bool caseSensitive() const override;
    virtual bool isRelativePath() const override;
    virtual FileFlags fileFlags(FileFlags type=FileInfoAll) const override;
    virtual bool setPermissions(uint perms) override;
    virtual QByteArray id() const override;
    virtual QString fileName(FileName file = DefaultName) const override;
    virtual uint ownerId(FileOwner) const override;
    virtual QString owner(FileOwner) const override;
    virtual bool setFileTime(const QDateTime &newDate, QFile::FileTime time) override;
    virtual QDateTime fileTime(QFile::FileTime time) const override;
    virtual void setFileName(const QString &file) override;
    virtual int handle() const override;
    virtual TriStateResult cloneTo(QAbstractFileEngine *target) override;
    virtual IteratorUniquePtr beginEntryList(const QString &path, QDirListing::IteratorFlags filters,
                                             const QStringList &filterNames) override;
    virtual qint64 read(char *data, qint64 maxlen) override;
    virtual qint64 readLine(char *data, qint64 maxlen) override;
    virtual qint64 write(const char *data, qint64 len) override;
    virtual bool extension(Extension extension, const ExtensionOption *option = nullptr,
                          ExtensionReturn *output = nullptr) override;
    virtual bool supportsExtension(Extension extension) const override;

private:
    QString m_fileName;
    QIODevice::OpenMode m_openMode = QIODevice::NotOpen;
    std::unique_ptr<qstdweb::BlobIODevice> m_blobDevice;
    std::unique_ptr<qstdweb::FileSystemFileIODevice> m_fileDevice;
};

QT_END_NAMESPACE
#endif // QWASMFILEENGINEHANDLER_H
