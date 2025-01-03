// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSFILEENGINEASSETSLIBRARY_H
#define QIOSFILEENGINEASSETSLIBRARY_H

#include <QtCore/private/qabstractfileengine_p.h>

Q_FORWARD_DECLARE_OBJC_CLASS(ALAsset);

QT_BEGIN_NAMESPACE

class QIOSAssetData;

class QIOSFileEngineAssetsLibrary : public QAbstractFileEngine
{
public:
    QIOSFileEngineAssetsLibrary(const QString &fileName);
    ~QIOSFileEngineAssetsLibrary();

    bool open(QIODevice::OpenMode openMode, std::optional<QFile::Permissions> permissions) override;
    bool close() override;
    FileFlags fileFlags(FileFlags type) const override;
    qint64 size() const override;
    qint64 read(char *data, qint64 maxlen) override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    QString fileName(FileName file) const override;
    void setFileName(const QString &file) override;

#ifndef QT_NO_FILESYSTEMITERATOR
    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames) override;
    Iterator *endEntryList() override;
#endif

    void setError(QFile::FileError error, const QString &str) { QAbstractFileEngine::setError(error, str); }

private:
    QString m_fileName;
    QString m_assetUrl;
    qint64 m_offset;
    mutable QIOSAssetData *m_data;

    ALAsset *loadAsset() const;
};

QT_END_NAMESPACE

#endif // QIOSFILEENGINEASSETSLIBRARY_H

