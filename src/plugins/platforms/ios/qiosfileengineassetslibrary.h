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

#ifndef QIOSFILEENGINEASSETSLIBRARY_H
#define QIOSFILEENGINEASSETSLIBRARY_H

#include <QtCore/private/qabstractfileengine_p.h>

Q_FORWARD_DECLARE_OBJC_CLASS(ALAsset);
class QIOSAssetData;

class QIOSFileEngineAssetsLibrary : public QAbstractFileEngine
{
public:
    QIOSFileEngineAssetsLibrary(const QString &fileName);
    ~QIOSFileEngineAssetsLibrary();

    bool open(QIODevice::OpenMode openMode) Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;
    FileFlags fileFlags(FileFlags type) const Q_DECL_OVERRIDE;
    qint64 size() const Q_DECL_OVERRIDE;
    qint64 read(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 pos() const Q_DECL_OVERRIDE;
    bool seek(qint64 pos) Q_DECL_OVERRIDE;
    QString fileName(FileName file) const Q_DECL_OVERRIDE;
    void setFileName(const QString &file) Q_DECL_OVERRIDE;
    QStringList entryList(QDir::Filters filters, const QStringList &filterNames) const Q_DECL_OVERRIDE;

#ifndef QT_NO_FILESYSTEMITERATOR
    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames) Q_DECL_OVERRIDE;
    Iterator *endEntryList() Q_DECL_OVERRIDE;
#endif

    void setError(QFile::FileError error, const QString &str) { QAbstractFileEngine::setError(error, str); }

private:
    QString m_fileName;
    QString m_assetUrl;
    qint64 m_offset;
    mutable QIOSAssetData *m_data;

    ALAsset *loadAsset() const;
};

#endif // QIOSFILEENGINEASSETSLIBRARY_H

