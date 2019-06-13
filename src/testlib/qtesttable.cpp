/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#include <QtTest/private/qtesttable_p.h>
#include <QtTest/qtestdata.h>
#include <QtTest/qtestassert.h>

#include <QtCore/qmetaobject.h>

#include <string.h>
#include <vector>
#include <algorithm>

QT_BEGIN_NAMESPACE

class QTestTablePrivate
{
public:
    ~QTestTablePrivate()
    {
        qDeleteAll(dataList.begin(), dataList.end());
    }

    struct Element {
        Element() = default;
        Element(const char *n, int t) : name(n), type(t) {}

        const char *name = nullptr;
        int type = 0;
    };

    using ElementList = std::vector<Element>;
    ElementList elementList;

    using DataList = std::vector<QTestData *>;
    DataList dataList;

    void addColumn(int elemType, const char *elemName) { elementList.push_back(Element(elemName, elemType)); }
    void addRow(QTestData *data) { dataList.push_back(data); }

    static QTestTable *currentTestTable;
    static QTestTable *gTable;
};

QTestTable *QTestTablePrivate::currentTestTable = nullptr;
QTestTable *QTestTablePrivate::gTable = nullptr;

void QTestTable::addColumn(int type, const char *name)
{
    QTEST_ASSERT(type);
    QTEST_ASSERT(name);

    d->addColumn(type, name);
}

int QTestTable::elementCount() const
{
    return int(d->elementList.size());
}

int QTestTable::dataCount() const
{
    return int(d->dataList.size());
}

bool QTestTable::isEmpty() const
{
    return d->elementList.empty();
}

QTestData *QTestTable::newData(const char *tag)
{
    QTestData *dt = new QTestData(tag, this);
    d->addRow(dt);
    return dt;
}

QTestTable::QTestTable()
{
    d = new QTestTablePrivate;
    QTestTablePrivate::currentTestTable = this;
}

QTestTable::~QTestTable()
{
    QTestTablePrivate::currentTestTable = nullptr;
    delete d;
}

int QTestTable::elementTypeId(int index) const
{
    return size_t(index) < d->elementList.size() ? d->elementList[index].type : -1;
}

const char *QTestTable::dataTag(int index) const
{
    return size_t(index) < d->elementList.size() ? d->elementList[index].name : nullptr;
}

QTestData *QTestTable::testData(int index) const
{
    return size_t(index) < d->dataList.size() ? d->dataList[index] : nullptr;
}

class NamePredicate
{
public:
    explicit NamePredicate(const char *needle) : m_needle(needle) {}

    bool operator()(const QTestTablePrivate::Element &e) const
        { return !strcmp(e.name, m_needle); }

private:
    const char *m_needle;
};

int QTestTable::indexOf(const char *elementName) const
{
    QTEST_ASSERT(elementName);

    const QTestTablePrivate::ElementList &elementList = d->elementList;

    const auto it = std::find_if(elementList.begin(), elementList.end(),
                                 NamePredicate(elementName));
    return it != elementList.end() ?
        int(it - elementList.begin()) : -1;
}

QTestTable *QTestTable::globalTestTable()
{
    if (!QTestTablePrivate::gTable)
        QTestTablePrivate::gTable = new QTestTable();
    return QTestTablePrivate::gTable;
}

void QTestTable::clearGlobalTestTable()
{
    delete QTestTablePrivate::gTable;
    QTestTablePrivate::gTable = nullptr;
}

QTestTable *QTestTable::currentTestTable()
{
    return QTestTablePrivate::currentTestTable;
}

QT_END_NAMESPACE
