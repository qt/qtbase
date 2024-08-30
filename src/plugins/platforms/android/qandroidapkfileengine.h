// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDAPKFILEENGINE_H
#define QANDROIDAPKFILEENGINE_H

#include <QtCore/private/qabstractfileengine_p.h>
#include <QtCore/qjnitypes.h>
#include <QtCore/QJniObject>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtApkFileEngine, "org/qtproject/qt/android/QtApkFileEngine")

class QAndroidApkFileEngine : public QAbstractFileEngine
{
public:
    QAndroidApkFileEngine(const QString &fileName);
    ~QAndroidApkFileEngine();

    struct FileInfo
    {
        QString relativePath;
        qint64 size = -1;
        bool isDir;
    };

    bool open(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions) override;
    bool close() override;
    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    qint64 read(char *data, qint64 maxlen) override;
    FileFlags fileFlags(FileFlags type = FileInfoAll) const override;
    bool caseSensitive() const override { return true; }
    QString fileName(FileName file = DefaultName) const override;
    void setFileName(const QString &file) override;

    uchar *map(qint64 offset, qint64 size, QFile::MemoryMapFlags flags);
    bool extension(Extension extension, const ExtensionOption *option = nullptr,
                   ExtensionReturn *output = nullptr) override;
    bool supportsExtension(Extension extension) const override;

    static QString apkPath();
    static QString relativePath(const QString &filePath);

#ifndef QT_NO_FILESYSTEMITERATOR
    IteratorUniquePtr beginEntryList(const QString &, QDirListing::IteratorFlags filters,
                                     const QStringList &filterNames) override;
#endif

private:
    QString m_fileName;
    FileInfo *m_fileInfo = nullptr;
    QtJniTypes::QtApkFileEngine m_apkFileEngine;
};

#ifndef QT_NO_FILESYSTEMITERATOR
class QAndroidApkFileEngineIterator : public QAbstractFileEngineIterator
{
public:
    QAndroidApkFileEngineIterator(const QString &path,
                                  QDirListing::IteratorFlags filters,
                                  const QStringList &filterNames);
    ~QAndroidApkFileEngineIterator();

    bool advance() override;
    QString currentFileName() const override;
    QString currentFilePath() const override;

private:
    int m_index = -1;
    QList<QAndroidApkFileEngine::FileInfo *> m_infos;
};
#endif

class QAndroidApkFileEngineHandler : public QAbstractFileEngineHandler
{
    Q_DISABLE_COPY_MOVE(QAndroidApkFileEngineHandler)
public:
    QAndroidApkFileEngineHandler() = default;

    std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const override;
};

QT_END_NAMESPACE

#endif // QANDROIDAPKFILEENGINE_H
