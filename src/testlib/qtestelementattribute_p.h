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

#include <QtTest/qttestglobal.h>

QT_BEGIN_NAMESPACE


namespace QTest {

    enum AttributeIndex
    {
        AI_Undefined = -1,
        AI_Name,
        AI_Tests,
        AI_Failures,
        AI_Errors,
        AI_Type,
        AI_Message,
        AI_PropertyValue,
        AI_Value,
        AI_Time,
        AI_Timestamp,
        AI_Hostname,
        AI_Classname,
        AI_Skipped
    };

    enum LogElementType
    {
        LET_Undefined = -1,
        LET_Property,
        LET_Properties,
        LET_Failure,
        LET_Error,
        LET_TestCase,
        LET_TestSuite,
        LET_Text,
        LET_SystemError,
        LET_SystemOutput,
        LET_Skipped
    };
}

class QTestElementAttribute
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
