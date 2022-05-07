/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

