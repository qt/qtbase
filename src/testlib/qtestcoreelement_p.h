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

#ifndef QTESTCOREELEMENT_P_H
#define QTESTCOREELEMENT_P_H

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
#include <QtTest/private/qtestelementattribute_p.h>

QT_BEGIN_NAMESPACE


template <class ElementType>
class QTestCoreElement: public QTestCoreList<ElementType>
{
    public:
        QTestCoreElement( int type = -1 );
        virtual ~QTestCoreElement();

        void addAttribute(const QTest::AttributeIndex index, const char *value);
        QTestElementAttribute *attributes() const;
        const char *attributeValue(QTest::AttributeIndex index) const;
        const char *attributeName(QTest::AttributeIndex index) const;
        const QTestElementAttribute *attribute(QTest::AttributeIndex index) const;

        const char *elementName() const;
        QTest::LogElementType elementType() const;

    private:
        QTestElementAttribute *listOfAttributes = nullptr;
        QTest::LogElementType type;
};

template<class ElementType>
QTestCoreElement<ElementType>::QTestCoreElement(int t)
    : type(QTest::LogElementType(t))
{
}

template<class ElementType>
QTestCoreElement<ElementType>::~QTestCoreElement()
{
    delete listOfAttributes;
}

template <class ElementType>
void QTestCoreElement<ElementType>::addAttribute(const QTest::AttributeIndex attributeIndex, const char *value)
{
    if (attributeIndex == -1 || attribute(attributeIndex))
        return;

    QTestElementAttribute *testAttribute = new QTestElementAttribute;
    testAttribute->setPair(attributeIndex, value);
    testAttribute->addToList(&listOfAttributes);
}

template <class ElementType>
QTestElementAttribute *QTestCoreElement<ElementType>::attributes() const
{
    return listOfAttributes;
}

template <class ElementType>
const char *QTestCoreElement<ElementType>::attributeValue(QTest::AttributeIndex index) const
{
    const QTestElementAttribute *attrb = attribute(index);
    if (attrb)
        return attrb->value();

    return nullptr;
}

template <class ElementType>
const char *QTestCoreElement<ElementType>::attributeName(QTest::AttributeIndex index) const
{
    const QTestElementAttribute *attrb = attribute(index);
    if (attrb)
        return attrb->name();

    return nullptr;
}

template <class ElementType>
const char *QTestCoreElement<ElementType>::elementName() const
{
    const char *xmlElementNames[] =
    {
        "property",
        "properties",
        "failure",
        "error",
        "testcase",
        "testsuite",
        "benchmark",
        "system-err"
    };

    if (type != QTest::LET_Undefined)
        return xmlElementNames[type];

    return nullptr;
}

template <class ElementType>
QTest::LogElementType QTestCoreElement<ElementType>::elementType() const
{
    return type;
}

template <class ElementType>
const QTestElementAttribute *QTestCoreElement<ElementType>::attribute(QTest::AttributeIndex index) const
{
    QTestElementAttribute *iterator = listOfAttributes;
    while (iterator) {
        if (iterator->index() == index)
            return iterator;

        iterator = iterator->nextElement();
    }

    return nullptr;
}

QT_END_NAMESPACE

#endif
