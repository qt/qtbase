/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

