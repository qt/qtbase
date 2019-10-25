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

#include "qbinaryjson_p.h"

#include <qjsonobject.h>
#include <qjsonarray.h>

QT_BEGIN_NAMESPACE

namespace QBinaryJsonPrivate {

static Q_CONSTEXPR Base emptyArray  = {
    { qle_uint(sizeof(Base)) },
    { 0 },
    { qle_uint(0) }
};

static Q_CONSTEXPR Base emptyObject = {
    { qle_uint(sizeof(Base)) },
    { qToLittleEndian(1U) },
    { qle_uint(0) }
};

void MutableData::compact()
{
    Q_STATIC_ASSERT(sizeof(Value) == sizeof(offset));

    Base *base = header->root();
    int reserve = 0;
    if (base->is_object) {
        auto *o = static_cast<Object *>(base);
        for (uint i = 0; i < o->length; ++i)
            reserve += o->entryAt(i)->usedStorage(o);
    } else {
        auto *a = static_cast<Array *>(base);
        for (uint i = 0; i < a->length; ++i)
            reserve += a->at(i)->usedStorage(a);
    }

    uint size = sizeof(Base) + reserve + base->length * sizeof(offset);
    uint alloc = sizeof(Header) + size;
    auto *h = reinterpret_cast<Header *>(malloc(alloc));
    Q_CHECK_PTR(h);
    h->tag = QJsonDocument::BinaryFormatTag;
    h->version = 1;
    Base *b = h->root();
    b->size = size;
    b->is_object = header->root()->is_object;
    b->length = base->length;
    b->tableOffset = reserve + sizeof(Array);

    uint offset = sizeof(Base);
    if (b->is_object) {
        const auto *o = static_cast<const Object *>(base);
        auto *no = static_cast<Object *>(b);

        for (uint i = 0; i < o->length; ++i) {
            no->table()[i] = offset;

            const Entry *e = o->entryAt(i);
            Entry *ne = no->entryAt(i);
            uint s = e->size();
            memcpy(ne, e, s);
            offset += s;
            uint dataSize = e->value.usedStorage(o);
            if (dataSize) {
                memcpy(reinterpret_cast<char *>(no) + offset, e->value.data(o), dataSize);
                ne->value.value = offset;
                offset += dataSize;
            }
        }
    } else {
        const auto *a = static_cast<const Array *>(base);
        auto *na = static_cast<Array *>(b);

        for (uint i = 0; i < a->length; ++i) {
            const Value *v = a->at(i);
            Value *nv = na->at(i);
            *nv = *v;
            uint dataSize = v->usedStorage(a);
            if (dataSize) {
                memcpy(reinterpret_cast<char *>(na) + offset, v->data(a), dataSize);
                nv->value = offset;
                offset += dataSize;
            }
        }
    }
    Q_ASSERT(offset == uint(b->tableOffset));

    free(header);
    header = h;
    this->alloc = alloc;
    compactionCounter = 0;
}

bool ConstData::isValid() const
{
    if (header->tag != QJsonDocument::BinaryFormatTag || header->version != 1U)
        return false;

    const Base *root = header->root();
    const uint maxSize = alloc - sizeof(Header);
    return root->is_object
            ? static_cast<const Object *>(root)->isValid(maxSize)
            : static_cast<const Array *>(root)->isValid(maxSize);
}

QJsonDocument ConstData::toJsonDocument() const
{
    const Base *root = header->root();
    return root->is_object
            ? QJsonDocument(static_cast<const Object *>(root)->toJsonObject())
            : QJsonDocument(static_cast<const Array *>(root)->toJsonArray());
}

uint Base::reserveSpace(uint dataSize, uint posInTable, uint numItems, bool replace)
{
    Q_ASSERT(posInTable <= length);
    if (size + dataSize >= Value::MaxSize) {
        qWarning("QJson: Document too large to store in data structure %d %d %d",
                 uint(size), dataSize, Value::MaxSize);
        return 0;
    }

    offset off = tableOffset;
    // move table to new position
    if (replace) {
        memmove(reinterpret_cast<char *>(table()) + dataSize, table(), length * sizeof(offset));
    } else {
        memmove(reinterpret_cast<char *>(table() + posInTable + numItems) + dataSize,
                table() + posInTable, (length - posInTable) * sizeof(offset));
        memmove(reinterpret_cast<char *>(table()) + dataSize, table(), posInTable * sizeof(offset));
    }
    tableOffset += dataSize;
    for (uint i = 0; i < numItems; ++i)
        table()[posInTable + i] = off;
    size += dataSize;
    if (!replace) {
        length += numItems;
        size += numItems * sizeof(offset);
    }
    return off;
}

uint Object::indexOf(QStringView key, bool *exists) const
{
    uint min = 0;
    uint n = length;
    while (n > 0) {
        uint half = n >> 1;
        uint middle = min + half;
        if (*entryAt(middle) >= key) {
            n = half;
        } else {
            min = middle + 1;
            n -= half + 1;
        }
    }
    if (min < length && *entryAt(min) == key) {
        *exists = true;
        return min;
    }
    *exists = false;
    return min;
}

QJsonObject Object::toJsonObject() const
{
    QJsonObject object;
    for (uint i = 0; i < length; ++i) {
        const Entry *e = entryAt(i);
        object.insert(e->key(), e->value.toJsonValue(this));
    }
    return object;
}

bool Object::isValid(uint maxSize) const
{
    if (size > maxSize || tableOffset + length * sizeof(offset) > size)
        return false;

    QString lastKey;
    for (uint i = 0; i < length; ++i) {
        if (table()[i] + sizeof(Entry) >= tableOffset)
            return false;
        const Entry *e = entryAt(i);
        if (!e->isValid(tableOffset - table()[i]))
            return false;
        const QString key = e->key();
        if (key < lastKey)
            return false;
        if (!e->value.isValid(this))
            return false;
        lastKey = key;
    }
    return true;
}

QJsonArray Array::toJsonArray() const
{
    QJsonArray array;
    const offset *values = table();
    for (uint i = 0; i < length; ++i)
        array.append(reinterpret_cast<const Value *>(values + i)->toJsonValue(this));
    return array;
}

bool Array::isValid(uint maxSize) const
{
    if (size > maxSize || tableOffset + length * sizeof(offset) > size)
        return false;

    const offset *values = table();
    for (uint i = 0; i < length; ++i) {
        if (!reinterpret_cast<const Value *>(values + i)->isValid(this))
            return false;
    }
    return true;
}

uint Value::usedStorage(const Base *b) const
{
    uint s = 0;
    switch (type) {
    case QJsonValue::Double:
        if (!latinOrIntValue)
            s = sizeof(double);
        break;
    case QJsonValue::String: {
        const char *d = data(b);
        s = latinOrIntValue
                ? (sizeof(ushort)
                   + qFromLittleEndian(*reinterpret_cast<const ushort *>(d)))
                : (sizeof(int)
                   + sizeof(ushort) * qFromLittleEndian(*reinterpret_cast<const int *>(d)));
        break;
    }
    case QJsonValue::Array:
    case QJsonValue::Object:
        s = base(b)->size;
        break;
    case QJsonValue::Null:
    case QJsonValue::Bool:
    default:
        break;
    }
    return alignedSize(s);
}

QJsonValue Value::toJsonValue(const Base *b) const
{
    switch (type) {
    case QJsonValue::Null:
        return QJsonValue(QJsonValue::Null);
    case QJsonValue::Bool:
        return QJsonValue(toBoolean());
    case QJsonValue::Double:
        return QJsonValue(toDouble(b));
    case QJsonValue::String:
        return QJsonValue(toString(b));
    case QJsonValue::Array:
        return static_cast<const Array *>(base(b))->toJsonArray();
    case QJsonValue::Object:
        return static_cast<const Object *>(base(b))->toJsonObject();
    case QJsonValue::Undefined:
        return QJsonValue(QJsonValue::Undefined);
    }
    Q_UNREACHABLE();
    return QJsonValue(QJsonValue::Undefined);
}

inline bool isValidValueOffset(uint offset, uint tableOffset)
{
    return offset >= sizeof(Base)
        && offset + sizeof(uint) <= tableOffset;
}

bool Value::isValid(const Base *b) const
{
    switch (type) {
    case QJsonValue::Null:
    case QJsonValue::Bool:
        return true;
    case QJsonValue::Double:
        return latinOrIntValue || isValidValueOffset(value, b->tableOffset);
    case QJsonValue::String:
        if (!isValidValueOffset(value, b->tableOffset))
            return false;
        if (latinOrIntValue)
            return asLatin1String(b).isValid(b->tableOffset - value);
        return asString(b).isValid(b->tableOffset - value);
    case QJsonValue::Array:
        return isValidValueOffset(value, b->tableOffset)
            && static_cast<const Array *>(base(b))->isValid(b->tableOffset - value);
    case QJsonValue::Object:
        return isValidValueOffset(value, b->tableOffset)
            && static_cast<const Object *>(base(b))->isValid(b->tableOffset - value);
    default:
        return false;
    }
}

uint Value::requiredStorage(const QBinaryJsonValue &v, bool *compressed)
{
    *compressed = false;
    switch (v.type()) {
    case QJsonValue::Double:
        if (QBinaryJsonPrivate::compressedNumber(v.toDouble()) != INT_MAX) {
            *compressed = true;
            return 0;
        }
        return sizeof(double);
    case QJsonValue::String: {
        QString s = v.toString();
        *compressed = QBinaryJsonPrivate::useCompressed(s);
        return QBinaryJsonPrivate::qStringSize(s, *compressed);
    }
    case QJsonValue::Array:
    case QJsonValue::Object:
        return v.base ? uint(v.base->size) : sizeof(QBinaryJsonPrivate::Base);
    case QJsonValue::Undefined:
    case QJsonValue::Null:
    case QJsonValue::Bool:
        break;
    }
    return 0;
}

uint Value::valueToStore(const QBinaryJsonValue &v, uint offset)
{
    switch (v.type()) {
    case QJsonValue::Undefined:
    case QJsonValue::Null:
        break;
    case QJsonValue::Bool:
        return v.toBool();
    case QJsonValue::Double: {
        int c = QBinaryJsonPrivate::compressedNumber(v.toDouble());
        if (c != INT_MAX)
            return c;
    }
        Q_FALLTHROUGH();
    case QJsonValue::String:
    case QJsonValue::Array:
    case QJsonValue::Object:
        return offset;
    }
    return 0;
}

void Value::copyData(const QBinaryJsonValue &v, char *dest, bool compressed)
{
    switch (v.type()) {
    case QJsonValue::Double:
        if (!compressed)
            qToLittleEndian(v.toDouble(), dest);
        break;
    case QJsonValue::String: {
        const QString str = v.toString();
        QBinaryJsonPrivate::copyString(dest, str, compressed);
        break;
    }
    case QJsonValue::Array:
    case QJsonValue::Object: {
        const QBinaryJsonPrivate::Base *b = v.base;
        if (!b)
            b = (v.type() == QJsonValue::Array ? &emptyArray : &emptyObject);
        memcpy(dest, b, b->size);
        break;
    }
    default:
        break;
    }
}

} // namespace QBinaryJsonPrivate

QT_END_NAMESPACE
