// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qtestelementattribute_p.h>
#include <QtCore/qbytearray.h>
#include <string.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

QTestElementAttribute::QTestElementAttribute() = default;

QTestElementAttribute::~QTestElementAttribute()
{
    delete[] attributeValue;
}

const char *QTestElementAttribute::value() const
{
    return attributeValue;
}

const char *QTestElementAttribute::name() const
{
    const char *AttributeNames[] =
    {
        "name",
        "tests",
        "failures",
        "errors",
        "type",
        "message",
        "value",
        "value",
        "time",
        "timestamp",
        "hostname",
        "classname",
        "skipped"
    };

    if (attributeIndex != QTest::AI_Undefined)
        return AttributeNames[attributeIndex];

    return nullptr;
}

QTest::AttributeIndex QTestElementAttribute::index() const
{
    return attributeIndex;
}

bool QTestElementAttribute::isNull() const
{
    return attributeIndex == QTest::AI_Undefined;
}

bool QTestElementAttribute::setPair(QTest::AttributeIndex index, const char *value)
{
    if (!value)
        return false;

    delete[] attributeValue;

    attributeIndex = index;
    attributeValue = qstrdup(value);

    return attributeValue != nullptr;
}

QT_END_NAMESPACE

