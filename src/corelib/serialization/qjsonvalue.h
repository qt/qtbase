/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QJSONVALUE_H
#define QJSONVALUE_H

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qcborvalue.h>

QT_BEGIN_NAMESPACE

class QVariant;
class QJsonArray;
class QJsonObject;
class QCborContainerPrivate;

namespace QJsonPrivate {
class Value;
}

class Q_CORE_EXPORT QJsonValue
{
public:
    enum Type {
        Null =  0x0,
        Bool = 0x1,
        Double = 0x2,
        String = 0x3,
        Array = 0x4,
        Object = 0x5,
        Undefined = 0x80
    };

    QJsonValue(Type = Null);
    QJsonValue(bool b);
    QJsonValue(double n);
    QJsonValue(int n);
    QJsonValue(qint64 v);
    QJsonValue(const QString &s);
    QJsonValue(QLatin1String s);
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN inline QJsonValue(const char *s)
        : QJsonValue(QString::fromUtf8(s)) {}
#endif
    QJsonValue(const QJsonArray &a);
    QJsonValue(const QJsonObject &o);

    ~QJsonValue();

    QJsonValue(const QJsonValue &other);
    QJsonValue &operator =(const QJsonValue &other);

    QJsonValue(QJsonValue &&other) noexcept;

    QJsonValue &operator =(QJsonValue &&other) noexcept
    {
        swap(other);
        return *this;
    }

    void swap(QJsonValue &other) noexcept;

    static QJsonValue fromVariant(const QVariant &variant);
    QVariant toVariant() const;

    Type type() const;
    inline bool isNull() const { return type() == Null; }
    inline bool isBool() const { return type() == Bool; }
    inline bool isDouble() const { return type() == Double; }
    inline bool isString() const { return type() == String; }
    inline bool isArray() const { return type() == Array; }
    inline bool isObject() const { return type() == Object; }
    inline bool isUndefined() const { return type() == Undefined; }

    bool toBool(bool defaultValue = false) const;
    int toInt(int defaultValue = 0) const;
    qint64 toInteger(qint64 defaultValue = 0) const;
    double toDouble(double defaultValue = 0) const;
    QString toString() const;
    QString toString(const QString &defaultValue) const;
    QJsonArray toArray() const;
    QJsonArray toArray(const QJsonArray &defaultValue) const;
    QJsonObject toObject() const;
    QJsonObject toObject(const QJsonObject &defaultValue) const;

#if QT_STRINGVIEW_LEVEL < 2
    const QJsonValue operator[](const QString &key) const;
#endif
    const QJsonValue operator[](QStringView key) const;
    const QJsonValue operator[](QLatin1String key) const;
    const QJsonValue operator[](qsizetype i) const;

    bool operator==(const QJsonValue &other) const;
    bool operator!=(const QJsonValue &other) const;

private:
    // avoid implicit conversions from char * to bool
    QJsonValue(const void *) = delete;
    friend class QJsonPrivate::Value;
    friend class QJsonArray;
    friend class QJsonObject;
    friend class QCborValue;
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonValue &);
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QJsonValue &);

    QCborValue value;

    // Assert binary compatibility with pre-5.15 QJsonValue
    static_assert(sizeof(QExplicitlySharedDataPointer<QCborContainerPrivate>) == sizeof(void *));
    static_assert(sizeof(QCborValue::Type) == sizeof(QJsonValue::Type));
};

class Q_CORE_EXPORT QJsonValueRef
{
public:
    QJsonValueRef(QJsonArray *array, qsizetype idx)
        : a(array), is_object(false), index(static_cast<quint64>(idx)) {}
    QJsonValueRef(QJsonObject *object, qsizetype idx)
        : o(object), is_object(true), index(static_cast<quint64>(idx)) {}

    QJsonValueRef(const QJsonValueRef &) = default;

    inline operator QJsonValue() const { return toValue(); }
    QJsonValueRef &operator = (const QJsonValue &val);
    QJsonValueRef &operator = (const QJsonValueRef &val);

    QVariant toVariant() const;
    inline QJsonValue::Type type() const { return toValue().type(); }
    inline bool isNull() const { return type() == QJsonValue::Null; }
    inline bool isBool() const { return type() == QJsonValue::Bool; }
    inline bool isDouble() const { return type() == QJsonValue::Double; }
    inline bool isString() const { return type() == QJsonValue::String; }
    inline bool isArray() const { return type() == QJsonValue::Array; }
    inline bool isObject() const { return type() == QJsonValue::Object; }
    inline bool isUndefined() const { return type() == QJsonValue::Undefined; }

    inline bool toBool(bool defaultValue = false) const { return toValue().toBool(defaultValue); }
    inline int toInt(int defaultValue = 0) const { return toValue().toInt(defaultValue); }
    inline qint64 toInteger(qint64 defaultValue = 0) const { return toValue().toInteger(defaultValue); }
    inline double toDouble(double defaultValue = 0) const { return toValue().toDouble(defaultValue); }
    inline QString toString(const QString &defaultValue = {}) const { return toValue().toString(defaultValue); }
    QJsonArray toArray() const;
    QJsonObject toObject() const;

    inline bool operator==(const QJsonValue &other) const { return toValue() == other; }
    inline bool operator!=(const QJsonValue &other) const { return toValue() != other; }

private:
    QJsonValue toValue() const;

    union {
        QJsonArray *a;
        QJsonObject *o;
    };
    quint64 is_object : 1;
    quint64 index : 63;

    friend class QJsonArray;
    friend class QJsonObject;
};

Q_DECLARE_SHARED(QJsonValue)

Q_CORE_EXPORT size_t qHash(const QJsonValue &value, size_t seed = 0);

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_JSON_READONLY)
Q_CORE_EXPORT QDebug operator<<(QDebug, const QJsonValue &);
#endif

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QJsonValue &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QJsonValue &);
#endif

QT_END_NAMESPACE

#endif // QJSONVALUE_H
