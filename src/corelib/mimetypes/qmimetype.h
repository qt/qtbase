/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
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

#ifndef QMIMETYPE_H
#define QMIMETYPE_H

#include <QtCore/qglobal.h>

QT_REQUIRE_CONFIG(mimetype);

#include <QtCore/qobjectdefs.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QMimeTypePrivate;
class QMimeType;

Q_CORE_EXPORT uint qHash(const QMimeType &key, uint seed = 0) noexcept;

class Q_CORE_EXPORT QMimeType
{
    Q_GADGET
    Q_PROPERTY(bool valid READ isValid CONSTANT)
    Q_PROPERTY(bool isDefault READ isDefault CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString comment READ comment CONSTANT)
    Q_PROPERTY(QString genericIconName READ genericIconName CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QStringList globPatterns READ globPatterns CONSTANT)
    Q_PROPERTY(QStringList parentMimeTypes READ parentMimeTypes CONSTANT)
    Q_PROPERTY(QStringList allAncestors READ allAncestors CONSTANT)
    Q_PROPERTY(QStringList aliases READ aliases CONSTANT)
    Q_PROPERTY(QStringList suffixes READ suffixes CONSTANT)
    Q_PROPERTY(QString preferredSuffix READ preferredSuffix CONSTANT)
    Q_PROPERTY(QString filterString READ filterString CONSTANT)

public:
    QMimeType();
    QMimeType(const QMimeType &other);
    QMimeType &operator=(const QMimeType &other);
    QMimeType &operator=(QMimeType &&other) noexcept { swap(other); return *this; }
    void swap(QMimeType &other) noexcept
    {
        qSwap(d, other.d);
    }
    explicit QMimeType(const QMimeTypePrivate &dd);
    ~QMimeType();

    bool operator==(const QMimeType &other) const;

    inline bool operator!=(const QMimeType &other) const
    {
        return !operator==(other);
    }

    bool isValid() const;

    bool isDefault() const;

    QString name() const;
    QString comment() const;
    QString genericIconName() const;
    QString iconName() const;
    QStringList globPatterns() const;
    QStringList parentMimeTypes() const;
    QStringList allAncestors() const;
    QStringList aliases() const;
    QStringList suffixes() const;
    QString preferredSuffix() const;

    Q_INVOKABLE bool inherits(const QString &mimeTypeName) const;

    QString filterString() const;

protected:
    friend class QMimeTypeParserBase;
    friend class MimeTypeMapEntry;
    friend class QMimeDatabasePrivate;
    friend class QMimeXMLProvider;
    friend class QMimeBinaryProvider;
    friend class QMimeTypePrivate;
    friend Q_CORE_EXPORT uint qHash(const QMimeType &key, uint seed) noexcept;

    QExplicitlySharedDataPointer<QMimeTypePrivate> d;
};

Q_DECLARE_SHARED(QMimeType)

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_CORE_EXPORT QDebug operator<<(QDebug debug, const QMimeType &mime);
#endif

QT_END_NAMESPACE

#endif // QMIMETYPE_H
