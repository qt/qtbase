// Copyright (C) 2019 Volker Krause <vkrause@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDCONTENTFILEENGINE_H
#define ANDROIDCONTENTFILEENGINE_H

#include <private/qfsfileengine_p.h>

#include <QtCore/qjniobject.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

using DocumentFilePtr = std::shared_ptr<class DocumentFile>;

class AndroidContentFileEngine : public QFSFileEngine
{
public:
    AndroidContentFileEngine(const QString &fileName);
    bool open(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions) override;
    bool close() override;
    qint64 size() const override;
    bool remove() override;
    bool rename(const QString &newName) override;
    bool mkdir(const QString &dirName, bool createParentDirectories,
               std::optional<QFile::Permissions> permissions = std::nullopt) const override;
    bool rmdir(const QString &dirName, bool recurseParentDirectories) const override;
    QByteArray id() const override;
    bool caseSensitive() const override { return true; }
    QDateTime fileTime(QFile::FileTime time) const override;
    FileFlags fileFlags(FileFlags type = FileInfoAll) const override;
    QString fileName(FileName file = DefaultName) const override;
    IteratorUniquePtr beginEntryList(const QString &path, QDirListing::IteratorFlags filters,
                                     const QStringList &filterNames) override;

private:
    void closeNativeFileDescriptor();

    QString m_initialFile;
    QJniObject m_pfd;
    DocumentFilePtr m_documentFile;
};

class AndroidContentFileEngineHandler : public QAbstractFileEngineHandler
{
    Q_DISABLE_COPY_MOVE(AndroidContentFileEngineHandler)
public:
    AndroidContentFileEngineHandler();
    ~AndroidContentFileEngineHandler();
    std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const override;
};

class AndroidContentFileEngineIterator : public QAbstractFileEngineIterator
{
public:
    AndroidContentFileEngineIterator(const QString &path, QDirListing::IteratorFlags filters,
                                     const QStringList &filterNames);
    ~AndroidContentFileEngineIterator();

    bool advance() override;

    QString currentFileName() const override;
    QString currentFilePath() const override;
private:
    mutable QList<DocumentFilePtr> m_files;
    mutable qsizetype m_index = -1;
};

/*!
 *
 * DocumentFile Api.
 * Check https://developer.android.com/reference/androidx/documentfile/provider/DocumentFile
 * for more information.
 *
 */
class DocumentFile : public std::enable_shared_from_this<DocumentFile>
{
public:
    static DocumentFilePtr parseFromAnyUri(const QString &filename);
    static DocumentFilePtr fromSingleUri(const QJniObject &uri);
    static DocumentFilePtr fromTreeUri(const QJniObject &treeUri);
    static QStringList getPathSegments(const QJniObject &uri);

    DocumentFilePtr createFile(const QString &mimeType, const QString &displayName);
    DocumentFilePtr createDirectory(const QString &displayName);
    const QJniObject &uri() const;
    const DocumentFilePtr &parent() const;
    QString initialName() const;
    QString name() const;
    QString id() const;
    QString mimeType() const;
    bool isDirectory() const;
    bool isFile() const;
    bool isVirtual() const;
    QDateTime lastModified() const;
    int64_t length() const;
    bool canRead() const;
    bool canWrite() const;
    bool remove();
    bool exists() const;
    std::vector<DocumentFilePtr> listFiles();
    bool rename(const QString &newName);

protected:
    DocumentFile(const QJniObject &uri, const QString &displayName, const std::shared_ptr<DocumentFile> &parent);

protected:
    QString m_displayName;
    QJniObject m_uri;
    DocumentFilePtr m_parent;
};

QT_END_NAMESPACE

#endif // ANDROIDCONTENTFILEENGINE_H
