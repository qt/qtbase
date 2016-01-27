/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        Element() : name(Q_NULLPTR), type(0) {}
        Element(const char *n, int t) : name(n), type(t) {}

        const char *name;
        int type;
    };

    typedef std::vector<Element> ElementList;
    ElementList elementList;

    typedef std::vector<QTestData *> DataList;
    DataList dataList;

    void addColumn(int elemType, const char *elemName) { elementList.push_back(Element(elemName, elemType)); }
    void addRow(QTestData *data) { dataList.push_back(data); }

    static QTestTable *currentTestTable;
    static QTestTable *gTable;
};

QTestTable *QTestTablePrivate::currentTestTable = 0;
QTestTable *QTestTablePrivate::gTable = 0;

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
    QTestTablePrivate::currentTestTable = 0;
    delete d;
}

int QTestTable::elementTypeId(int index) const
{
    return size_t(index) < d->elementList.size() ? d->elementList[index].type : -1;
}

const char *QTestTable::dataTag(int index) const
{
    return size_t(index) < d->elementList.size() ? d->elementList[index].name : Q_NULLPTR;
}

QTestData *QTestTable::testData(int index) const
{
    return size_t(index) < d->dataList.size() ? d->dataList[index] : Q_NULLPTR;
}

class NamePredicate : public std::unary_function<QTestTablePrivate::Element, bool>
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
    typedef QTestTablePrivate::ElementList::const_iterator It;

    QTEST_ASSERT(elementName);

    const QTestTablePrivate::ElementList &elementList = d->elementList;

    const It it = std::find_if(elementList.begin(), elementList.end(),
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
    QTestTablePrivate::gTable = 0;
}

QTestTable *QTestTable::currentTestTable()
{
    return QTestTablePrivate::currentTestTable;
}

QT_END_NAMESPACE
