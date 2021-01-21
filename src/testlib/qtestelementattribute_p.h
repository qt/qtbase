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

#ifndef QTESTELEMENTATTRIBUTE_P_H
#define QTESTELEMENTATTRIBUTE_P_H

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

#include <QtTest/private/qtestcorelist_p.h>

QT_BEGIN_NAMESPACE


namespace QTest {

    enum AttributeIndex
    {
        AI_Undefined = -1,
        AI_Name = 0,
        AI_Result = 1,
        AI_Tests = 2,
        AI_Failures = 3,
        AI_Errors = 4,
        AI_Type = 5,
        AI_Description = 6,
        AI_PropertyValue = 7,
        AI_QTestVersion = 8,
        AI_QtVersion = 9,
        AI_File = 10,
        AI_Line = 11,
        AI_Metric = 12,
        AI_Tag = 13,
        AI_Value = 14,
        AI_Iterations = 15
    };

    enum LogElementType
    {
        LET_Undefined = -1,
        LET_Property = 0,
        LET_Properties = 1,
        LET_Failure = 2,
        LET_Error = 3,
        LET_TestCase = 4,
        LET_TestSuite = 5,
        LET_Benchmark = 6,
        LET_SystemError = 7
    };
}

class QTestElementAttribute: public QTestCoreList<QTestElementAttribute>
{
    public:
        QTestElementAttribute();
        ~QTestElementAttribute();

        const char *value() const;
        const char *name() const;
        QTest::AttributeIndex index() const;
        bool isNull() const;
        bool setPair(QTest::AttributeIndex attributeIndex, const char *value);

    private:
        char *attributeValue = nullptr;
        QTest::AttributeIndex attributeIndex = QTest::AI_Undefined;
};

QT_END_NAMESPACE

#endif
