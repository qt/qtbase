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

#include "qbinaryjsonobject_p.h"
#include "qbinaryjsonvalue_p.h"
#include "qbinaryjsonarray_p.h"
#include "qbinaryjson_p.h"

#include <qjsonarray.h>
#include <qjsonobject.h>

QT_BEGIN_NAMESPACE

QBinaryJsonValue::QBinaryJsonValue(QBinaryJsonPrivate::MutableData *data,
                                   QBinaryJsonPrivate::Base *parent,
                                   const QBinaryJsonPrivate::Value &v)
    : t(QJsonValue::Type(uint(v.type)))
{
    switch (t) {
    case QJsonValue::Undefined:
    case QJsonValue::Null:
        dbl = 0;
        break;
    case QJsonValue::Bool:
        b = v.toBoolean();
        break;
    case QJsonValue::Double:
        dbl = v.toDouble(parent);
        break;
    case QJsonValue::String: {
        QString s = v.toString(parent);
        stringData = s.data_ptr();
        stringData->ref.ref();
        break;
    }
    case QJsonValue::Array:
    case QJsonValue::Object:
        d = data;
        base = v.base(parent);
        break;
    }
    if (d)
        d->ref.ref();
}

QBinaryJsonValue::QBinaryJsonValue(QString string)
    : stringData(*reinterpret_cast<QStringData **>(&string)), t(QJsonValue::String)
{
    stringData->ref.ref();
}

QBinaryJsonValue::QBinaryJsonValue(const QBinaryJsonArray &a)
    : base(a.a), d(a.d), t(QJsonValue::Array)
{
    if (d)
        d->ref.ref();
}

QBinaryJsonValue::QBinaryJsonValue(const QBinaryJsonObject &o)
    : base(o.o), d(o.d), t(QJsonValue::Object)
{
    if (d)
        d->ref.ref();
}

QBinaryJsonValue::~QBinaryJsonValue()
{
    if (t == QJsonValue::String && stringData && !stringData->ref.deref())
        free(stringData);

    if (d && !d->ref.deref())
        delete d;
}

QBinaryJsonValue QBinaryJsonValue::fromJsonValue(const QJsonValue &json)
{
    switch (json.type()) {
    case QJsonValue::Bool:
        return QBinaryJsonValue(json.toBool());
    case QJsonValue::Double:
        return QBinaryJsonValue(json.toDouble());
    case QJsonValue::String:
        return QBinaryJsonValue(json.toString());
    case QJsonValue::Array:
        return QBinaryJsonArray::fromJsonArray(json.toArray());
    case QJsonValue::Object:
        return QBinaryJsonObject::fromJsonObject(json.toObject());
    case QJsonValue::Null:
        return QBinaryJsonValue(QJsonValue::Null);
    case QJsonValue::Undefined:
        return QBinaryJsonValue(QJsonValue::Undefined);
    }
    Q_UNREACHABLE();
    return QBinaryJsonValue(QJsonValue::Null);
}

QString QBinaryJsonValue::toString() const
{
    if (t != QJsonValue::String)
        return QString();
    stringData->ref.ref(); // the constructor below doesn't add a ref.
    QStringDataPtr holder = { stringData };
    return QString(holder);
}

void QBinaryJsonValue::detach()
{
    if (!d)
        return;

    QBinaryJsonPrivate::MutableData *x = d->clone(base);
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
    base = static_cast<QBinaryJsonPrivate::Object *>(d->header->root());
}

QT_END_NAMESPACE
