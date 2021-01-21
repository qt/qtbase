/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#include <QtTest/private/qtestelementattribute_p.h>
#include <QtCore/qbytearray.h>
#include <string.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*! \enum QTest::AttributeIndex
  This enum numbers the different tests.

  \value AI_Undefined

  \value AI_Name

  \value AI_Result

  \value AI_Tests

  \value AI_Failures

  \value AI_Errors

  \value AI_Type

  \value AI_Description

  \value AI_PropertyValue

  \value AI_QTestVersion

  \value AI_QtVersion

  \value AI_File

  \value AI_Line

  \value AI_Metric

  \value AI_Tag

  \value AI_Value

  \value AI_Iterations
*/

/*! \enum QTest::LogElementType
  The enum specifies the kinds of test log messages.

  \value LET_Undefined

  \value LET_Property

  \value LET_Properties

  \value LET_Failure

  \value LET_Error

  \value LET_TestCase

  \value LET_TestSuite

  \value LET_Benchmark

  \value LET_SystemError
*/

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
        "result",
        "tests",
        "failures",
        "errors",
        "type",
        "description",
        "value",
        "qtestversion",
        "qtversion",
        "file",
        "line",
        "metric",
        "tag",
        "value",
        "iterations"
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

