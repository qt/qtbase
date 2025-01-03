// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qtestelement_p.h>

QT_BEGIN_NAMESPACE

QTestElement::QTestElement(QTest::LogElementType type)
    : QTestCoreElement<QTestElement>(type)
{
}

QTestElement::~QTestElement()
{
    for (auto *child : listOfChildren)
        delete child;
}

bool QTestElement::addChild(QTestElement *element)
{
    if (!element)
        return false;

    if (element->elementType() != QTest::LET_Undefined) {
        listOfChildren.push_back(element);
        element->setParent(this);
        return true;
    }

    return false;
}

const std::vector<QTestElement*> &QTestElement::childElements() const
{
    return listOfChildren;
}

const QTestElement *QTestElement::parentElement() const
{
    return parent;
}

void QTestElement::setParent(const QTestElement *p)
{
    parent = p;
}

QT_END_NAMESPACE

