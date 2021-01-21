/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

    bool open(QIODevice::OpenMode openMode) override;
    bool close() override;
    FileFlags fileFlags(FileFlags type) const override;
    qint64 size() const override;
    qint64 read(char *data, qint64 maxlen) override;
    qint64 pos() const override;
    bool seek(qint64 pos) override;
    QString fileName(FileName file) const override;
    void setFileName(const QString &file) override;
    QStringList entryList(QDir::Filters filters, const QStringList &filterNames) const override;

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

