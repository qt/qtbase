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

#ifndef QBINARYJSONVALUE_P_H
#define QBINARYJSONVALUE_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qjsonvalue.h>

QT_REQUIRE_CONFIG(binaryjson);

QT_BEGIN_NAMESPACE

class QBinaryJsonArray;
class QBinaryJsonObject;

namespace QBinaryJsonPrivate {
class ConstData;
class MutableData;
class Base;
class Value;
class Object;
class Array;
}

class Q_CORE_EXPORT QBinaryJsonValue
{
    Q_DISABLE_COPY(QBinaryJsonValue)
public:
    explicit QBinaryJsonValue(QJsonValue::Type type) : ui(0), t(type) {}
    explicit QBinaryJsonValue(bool b) : b(b), t(QJsonValue::Bool) {}
    explicit QBinaryJsonValue(double n) : dbl(n), t(QJsonValue::Double) {}
    explicit QBinaryJsonValue(QString s);
    QBinaryJsonValue(const QBinaryJsonArray &a);
    QBinaryJsonValue(const QBinaryJsonObject &o);

    ~QBinaryJsonValue();

    QBinaryJsonValue(QBinaryJsonValue &&other) noexcept
        : ui(other.ui),
          d(other.d),
          t(other.t)
    {
        other.ui = 0;
        other.d = nullptr;
        other.t = QJsonValue::Null;
    }

    QBinaryJsonValue &operator =(QBinaryJsonValue &&other) noexcept
    {
        qSwap(ui, other.ui);
        qSwap(d, other.d);
        qSwap(t, other.t);
        return *this;
    }

    static QBinaryJsonValue fromJsonValue(const QJsonValue &json);
    QJsonValue::Type type() const { return t; }
    bool toBool() const { return (t == QJsonValue::Bool) && b; }
    double toDouble() const { return (t == QJsonValue::Double) ? dbl : 0; }
    QString toString() const;

private:
    friend class QBinaryJsonPrivate::Value;
    friend class QBinaryJsonArray;
    friend class QBinaryJsonObject;

    QBinaryJsonValue(QBinaryJsonPrivate::MutableData *d, QBinaryJsonPrivate::Base *parent,
                     const QBinaryJsonPrivate::Value &v);

    void detach();

    union {
        quint64 ui;
        bool b;
        double dbl;
        QStringData *stringData;
        const QBinaryJsonPrivate::Base *base;
    };
    QBinaryJsonPrivate::MutableData *d = nullptr; // needed for Objects and Arrays
    QJsonValue::Type t = QJsonValue::Null;
};

QT_END_NAMESPACE

#endif // QBINARYJSONVALUE_P_H
