// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qmetaobject.h>

#include <QtTest/qtestassert.h>
#include <QtTest/qtestdata.h>
#include <QtTest/private/qtesttable_p.h>

#include <string.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

class QTestDataPrivate
{
public:
    char *tag = nullptr;
    QTestTable *parent = nullptr;
    void **data = nullptr;
    int dataCount = 0;
};

QTestData::QTestData(const char *tag, QTestTable *parent)
{
    QTEST_ASSERT(tag);
    QTEST_ASSERT(parent);
    d = new QTestDataPrivate;
    d->tag = qstrdup(tag);
    d->parent = parent;
    d->data = new void *[parent->elementCount()];
    memset(d->data, 0, parent->elementCount() * sizeof(void*));
}

QTestData::~QTestData()
{
    for (int i = 0; i < d->dataCount; ++i) {
        if (d->data[i])
            QMetaType(d->parent->elementTypeId(i)).destroy(d->data[i]);
    }
    delete [] d->data;
    delete [] d->tag;
    delete d;
}

void QTestData::append(int type, const void *data)
{
    QTEST_ASSERT(d->dataCount < d->parent->elementCount());
    int expectedType = d->parent->elementTypeId(d->dataCount);
    int dd = 0;
    if constexpr (sizeof(qsizetype) == 8) {
        // Compatibility with Qt 5. Passing a qsizetype to a test function expecting
        // an int will work. This is required, as methods returning a qsizetype in Qt 6
        // used to return an int in Qt 5.
        if (type == QMetaType::LongLong && expectedType == QMetaType::Int) {
            qlonglong d = *static_cast<const qlonglong *>(data);
            if (d >= std::numeric_limits<int>::min() && d <= std::numeric_limits<int>::max()) {
                dd = d;
                data = &dd;
                type = QMetaType::Int;
            }
        }
    }
    if (expectedType != type) {
        qDebug("expected data of type '%s', got '%s' for element %d of data with tag '%s'",
                QMetaType(expectedType).name(),
                QMetaType(type).name(),
                d->dataCount, d->tag);
        QTEST_ASSERT(false);
    }
    d->data[d->dataCount] = QMetaType(type).create(data);
    ++d->dataCount;
}

void *QTestData::data(int index) const
{
    QTEST_ASSERT(index >= 0);
    QTEST_ASSERT(index < d->parent->elementCount());
    return d->data[index];
}

QTestTable *QTestData::parent() const
{
    return d->parent;
}

const char *QTestData::dataTag() const
{
    return d->tag;
}

int QTestData::dataCount() const
{
    return d->dataCount;
}

QT_END_NAMESPACE
