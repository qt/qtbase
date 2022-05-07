/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QSQLFIELD_H
#define QSQLFIELD_H

#include <QtSql/qtsqlglobal.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE


class QSqlFieldPrivate;

class Q_SQL_EXPORT QSqlField
{
public:
    enum RequiredStatus { Unknown = -1, Optional = 0, Required = 1 };

    explicit QSqlField(const QString& fieldName = QString(), QMetaType type = QMetaType(), const QString &tableName = QString());

    QSqlField(const QSqlField& other);
    QSqlField& operator=(const QSqlField& other);
    bool operator==(const QSqlField& other) const;
    inline bool operator!=(const QSqlField &other) const { return !operator==(other); }
    ~QSqlField();

    void setValue(const QVariant& value);
    inline QVariant value() const
    { return val; }
    void setName(const QString& name);
    QString name() const;
    void setTableName(const QString &tableName);
    QString tableName() const;
    bool isNull() const;
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    void clear();
    bool isAutoValue() const;

    QMetaType metaType() const;
    void setMetaType(QMetaType type);

#if QT_DEPRECATED_SINCE(6, 0)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QT_DEPRECATED_VERSION_X_6_0("Use the constructor using a QMetaType instead")
    QSqlField(const QString& fieldName, QVariant::Type type, const QString &tableName = QString())
        : QSqlField(fieldName, QMetaType(type), tableName)
    {}
    QT_DEPRECATED_VERSION_X_6_0("Use metaType() instead")
    QVariant::Type type() const { return QVariant::Type(metaType().id()); };
    QT_DEPRECATED_VERSION_X_6_0("Use setMetaType() instead")
    void setType(QVariant::Type type) { setMetaType(QMetaType(int(type))); }
    QT_WARNING_POP
#endif

    void setRequiredStatus(RequiredStatus status);
    inline void setRequired(bool required)
    { setRequiredStatus(required ? Required : Optional); }
    void setLength(int fieldLength);
    void setPrecision(int precision);
    void setDefaultValue(const QVariant &value);
    void setSqlType(int type);
    void setGenerated(bool gen);
    void setAutoValue(bool autoVal);

    RequiredStatus requiredStatus() const;
    int length() const;
    int precision() const;
    QVariant defaultValue() const;
    int typeID() const;
    bool isGenerated() const;
    bool isValid() const;

private:
    void detach();
    QVariant val;
    QSqlFieldPrivate* d;
};

#ifndef QT_NO_DEBUG_STREAM
Q_SQL_EXPORT QDebug operator<<(QDebug, const QSqlField &);
#endif

QT_END_NAMESPACE

#endif // QSQLFIELD_H
