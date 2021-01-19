/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2019 Intel Corporation.
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

#ifndef QRESOURCE_H
#define QRESOURCE_H

#include <QtCore/qstring.h>
#include <QtCore/qlocale.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE


class QResourcePrivate;

class Q_CORE_EXPORT QResource
{
public:
    enum Compression {
        NoCompression,
        ZlibCompression,
        ZstdCompression
    };

    QResource(const QString &file=QString(), const QLocale &locale=QLocale());
    ~QResource();

    void setFileName(const QString &file);
    QString fileName() const;
    QString absoluteFilePath() const;

    void setLocale(const QLocale &locale);
    QLocale locale() const;

    bool isValid() const;

    Compression compressionAlgorithm() const;
    qint64 size() const;
    const uchar *data() const;
    qint64 uncompressedSize() const;
    QByteArray uncompressedData() const;
    QDateTime lastModified() const;

#if QT_DEPRECATED_SINCE(5, 13)
    QT_DEPRECATED_X("Use QDir::addSearchPath() instead")
    static void addSearchPath(const QString &path);
    QT_DEPRECATED_X("Use QDir::searchPaths() instead")
    static QStringList searchPaths();
#endif
#if QT_DEPRECATED_SINCE(5, 15)
    QT_DEPRECATED_VERSION_X_5_15("Use QResource::compressionAlgorithm() instead")
    bool isCompressed() const;
#endif

    static bool registerResource(const QString &rccFilename, const QString &resourceRoot=QString());
    static bool unregisterResource(const QString &rccFilename, const QString &resourceRoot=QString());

    static bool registerResource(const uchar *rccData, const QString &resourceRoot=QString());
    static bool unregisterResource(const uchar *rccData, const QString &resourceRoot=QString());

protected:
    friend class QResourceFileEngine;
    friend class QResourceFileEngineIterator;
    bool isDir() const;
    inline bool isFile() const { return !isDir(); }
    QStringList children() const;

protected:
    QScopedPointer<QResourcePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(QResource)
};

QT_END_NAMESPACE

#endif // QRESOURCE_H
