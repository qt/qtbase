/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QRESOURCE_P_H
#define QRESOURCE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qabstractfileengine_p.h"

QT_BEGIN_NAMESPACE

class QResourceFileEnginePrivate;
class QResourceFileEngine : public QAbstractFileEngine
{
private:
    Q_DECLARE_PRIVATE(QResourceFileEngine)
public:
    explicit QResourceFileEngine(const QString &path);
    ~QResourceFileEngine();

    void setFileName(const QString &file) override;

    bool open(QIODevice::OpenMode flags) override;
    bool close() override;
    bool flush() override;
    qint64 size() const override;
    qint64 pos() const override;
    virtual bool atEnd() const;
    bool seek(qint64) override;
    qint64 read(char *data, qint64 maxlen) override;
    qint64 write(const char *data, qint64 len) override;

    bool remove() override;
    bool copy(const QString &newName) override;
    bool rename(const QString &newName) override;
    bool link(const QString &newName) override;

    bool isSequential() const override;

    bool isRelativePath() const override;

    bool mkdir(const QString &dirName, bool createParentDirectories) const override;
    bool rmdir(const QString &dirName, bool recurseParentDirectories) const override;

    bool setSize(qint64 size) override;

    QStringList entryList(QDir::Filters filters, const QStringList &filterNames) const override;

    bool caseSensitive() const override;

    FileFlags fileFlags(FileFlags type) const override;

    bool setPermissions(uint perms) override;

    QString fileName(QAbstractFileEngine::FileName file) const override;

    uint ownerId(FileOwner) const override;
    QString owner(FileOwner) const override;

    QDateTime fileTime(FileTime time) const override;

    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames) override;
    Iterator *endEntryList() override;

    bool extension(Extension extension, const ExtensionOption *option = nullptr, ExtensionReturn *output = nullptr) override;
    bool supportsExtension(Extension extension) const override;
};

QT_END_NAMESPACE

#endif // QRESOURCE_P_H
