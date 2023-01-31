// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    bool hasRow(const char *name) const;

    static QTestTable *currentTestTable;
    static QTestTable *gTable;
};

QTestTable *QTestTablePrivate::currentTestTable = nullptr;
QTestTable *QTestTablePrivate::gTable = nullptr;

void QTestTable::addColumn(int type, const char *name)
{
    QTEST_ASSERT(type);
    QTEST_ASSERT(name);
    if (indexOf(name) != -1)
        qWarning() << "Duplicate data column" << name << "- please rename.";

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
    if (d->hasRow(tag))
        qWarning("Duplicate data tag \"%s\" - please rename.", tag);

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

    bool operator()(const QTestData *e) const
        { return !strcmp(e->dataTag(), m_needle); }

private:
    const char *m_needle;
};

bool QTestTablePrivate::hasRow(const char *rowName) const
{
    QTEST_ASSERT(rowName);
    return std::find_if(dataList.begin(), dataList.end(), NamePredicate(rowName)) != dataList.end();
}

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
