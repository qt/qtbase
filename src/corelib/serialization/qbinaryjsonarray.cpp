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

#include "qbinaryjsonarray_p.h"
#include "qbinaryjson_p.h"

#include <qjsonarray.h>

QT_BEGIN_NAMESPACE

QBinaryJsonArray::~QBinaryJsonArray()
{
    if (d && !d->ref.deref())
        delete d;
}

QBinaryJsonArray QBinaryJsonArray::fromJsonArray(const QJsonArray &array)
{
    QBinaryJsonArray binary;
    for (const QJsonValue &value : array)
        binary.append(QBinaryJsonValue::fromJsonValue(value));
    if (binary.d) // We want to compact it as it is a root item now
        binary.d->compactionCounter++;
    binary.compact();
    return binary;
}

void QBinaryJsonArray::append(const QBinaryJsonValue &value)
{
    const uint i = a ? a->length : 0;

    bool compressed;
    uint valueSize = QBinaryJsonPrivate::Value::requiredStorage(value, &compressed);

    if (!detach(valueSize + sizeof(QBinaryJsonPrivate::Value)))
        return;

    if (!a->length)
        a->tableOffset = sizeof(QBinaryJsonPrivate::Array);

    uint valueOffset = a->reserveSpace(valueSize, i, 1, false);
    if (!valueOffset)
        return;

    QBinaryJsonPrivate::Value *v = a->at(i);
    v->type = (value.t == QJsonValue::Undefined ? QJsonValue::Null : value.t);
    v->latinOrIntValue = compressed;
    v->latinKey = false;
    v->value = QBinaryJsonPrivate::Value::valueToStore(value, valueOffset);
    if (valueSize) {
        QBinaryJsonPrivate::Value::copyData(value, reinterpret_cast<char *>(a) + valueOffset,
                                            compressed);
    }
}

char *QBinaryJsonArray::takeRawData(uint *size)
{
    if (d)
        return d->takeRawData(size);
    *size = 0;
    return nullptr;
}

bool QBinaryJsonArray::detach(uint reserve)
{
    if (!d) {
        if (reserve >= QBinaryJsonPrivate::Value::MaxSize) {
            qWarning("QBinaryJson: Document too large to store in data structure");
            return false;
        }
        d = new QBinaryJsonPrivate::MutableData(reserve, QJsonValue::Array);
        a = static_cast<QBinaryJsonPrivate::Array *>(d->header->root());
        d->ref.ref();
        return true;
    }
    if (reserve == 0 && d->ref.loadRelaxed() == 1)
        return true;

    QBinaryJsonPrivate::MutableData *x = d->clone(a, reserve);
    if (!x)
        return false;
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
    a = static_cast<QBinaryJsonPrivate::Array *>(d->header->root());
    return true;
}

void QBinaryJsonArray::compact()
{
    if (!d || !d->compactionCounter)
        return;

    detach();
    d->compact();
    a = static_cast<QBinaryJsonPrivate::Array *>(d->header->root());
}

QT_END_NAMESPACE

