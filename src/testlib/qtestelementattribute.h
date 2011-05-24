/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTESTELEMENTATTRIBUTE_H
#define QTESTELEMENTATTRIBUTE_H

#include <QtTest/qtestcorelist.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Test)

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
        char *attributeValue;
        QTest::AttributeIndex attributeIndex;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
