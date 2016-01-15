/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qabstractproxymodel.h>
#include <QItemSelection>
#include <qstandarditemmodel.h>

class tst_QAbstractProxyModel : public QObject
{
    Q_OBJECT

private slots:
    void qabstractproxymodel();
    void data_data();
    void data();
    void flags_data();
    void flags();
    void headerData_data();
    void headerData();
    void itemData_data();
    void itemData();
    void mapFromSource_data();
    void mapFromSource();
    void mapSelectionFromSource_data();
    void mapSelectionFromSource();
    void mapSelectionToSource_data();
    void mapSelectionToSource();
    void mapToSource_data();
    void mapToSource();
    void revert();
    void setSourceModel();
    void submit_data();
    void submit();
    void testRoleNames();
    void testSwappingRowsProxy();
    void testDragAndDrop();
};

// Subclass that exposes the protected functions.
class SubQAbstractProxyModel : public QAbstractProxyModel
{
public:
    // QAbstractProxyModel::mapFromSource is a pure virtual function.
    QModelIndex mapFromSource(QModelIndex const& sourceIndex) const
        { Q_UNUSED(sourceIndex); return QModelIndex(); }

    // QAbstractProxyModel::mapToSource is a pure virtual function.
    QModelIndex mapToSource(QModelIndex const& proxyIndex) const
        { Q_UNUSED(proxyIndex); return QModelIndex(); }

    QModelIndex index(int, int, const QModelIndex&) const
    {
        return QModelIndex();
    }

    QModelIndex parent(const QModelIndex&) const
    {
        return QModelIndex();
    }

    int rowCount(const QModelIndex&) const
    {
        return 0;
    }

    int columnCount(const QModelIndex&) const
    {
        return 0;
    }
};

void tst_QAbstractProxyModel::qabstractproxymodel()
{
    SubQAbstractProxyModel model;
    model.data(QModelIndex());
    model.flags(QModelIndex());
    model.headerData(0, Qt::Vertical, 0);
    model.itemData(QModelIndex());
    model.mapFromSource(QModelIndex());
    model.mapSelectionFromSource(QItemSelection());
    model.mapSelectionToSource(QItemSelection());
    model.mapToSource(QModelIndex());
    model.revert();
    model.setSourceModel(0);
    QCOMPARE(model.sourceModel(), (QAbstractItemModel*)0);
    model.submit();
}

void tst_QAbstractProxyModel::data_data()
{
    QTest::addColumn<QModelIndex>("proxyIndex");
    QTest::addColumn<int>("role");
    QTest::addColumn<QVariant>("data");
    QTest::newRow("null") << QModelIndex() << 0 << QVariant();
}

// public QVariant data(QModelIndex const& proxyIndex, int role = Qt::DisplayRole) const
void tst_QAbstractProxyModel::data()
{
    QFETCH(QModelIndex, proxyIndex);
    QFETCH(int, role);
    QFETCH(QVariant, data);

    SubQAbstractProxyModel model;
    QCOMPARE(model.data(proxyIndex, role), data);
}

Q_DECLARE_METATYPE(Qt::ItemFlags)
void tst_QAbstractProxyModel::flags_data()
{
    QTest::addColumn<QModelIndex>("index");
    QTest::addColumn<Qt::ItemFlags>("flags");
    QTest::newRow("null") << QModelIndex() << (Qt::ItemFlags)0;
}

// public Qt::ItemFlags flags(QModelIndex const& index) const
void tst_QAbstractProxyModel::flags()
{
    QFETCH(QModelIndex, index);
    QFETCH(Qt::ItemFlags, flags);

    SubQAbstractProxyModel model;
    QCOMPARE(model.flags(index), flags);
}

Q_DECLARE_METATYPE(Qt::Orientation)
Q_DECLARE_METATYPE(Qt::ItemDataRole)
void tst_QAbstractProxyModel::headerData_data()
{
    QTest::addColumn<int>("section");
    QTest::addColumn<Qt::Orientation>("orientation");
    QTest::addColumn<Qt::ItemDataRole>("role");
    QTest::addColumn<QVariant>("headerData");
    QTest::newRow("null") << 0 << Qt::Vertical << Qt::UserRole << QVariant();
}

// public QVariant headerData(int section, Qt::Orientation orientation, int role) const
void tst_QAbstractProxyModel::headerData()
{
    QFETCH(int, section);
    QFETCH(Qt::Orientation, orientation);
    QFETCH(Qt::ItemDataRole, role);
    QFETCH(QVariant, headerData);

    SubQAbstractProxyModel model;
    QCOMPARE(model.headerData(section, orientation, role), headerData);
}

void tst_QAbstractProxyModel::itemData_data()
{
    QTest::addColumn<QModelIndex>("index");
    QTest::addColumn<int>("count");

    QTest::newRow("null") << QModelIndex() << 0;
}

// public QMap<int,QVariant> itemData(QModelIndex const& index) const
void tst_QAbstractProxyModel::itemData()
{
    QFETCH(QModelIndex, index);
    QFETCH(int, count);
    SubQAbstractProxyModel model;
    QCOMPARE(model.itemData(index).count(), count);
}

void tst_QAbstractProxyModel::mapFromSource_data()
{
    QTest::addColumn<QModelIndex>("sourceIndex");
    QTest::addColumn<QModelIndex>("mapFromSource");
    QTest::newRow("null") << QModelIndex() << QModelIndex();
}

// public QModelIndex mapFromSource(QModelIndex const& sourceIndex) const
void tst_QAbstractProxyModel::mapFromSource()
{
    QFETCH(QModelIndex, sourceIndex);
    QFETCH(QModelIndex, mapFromSource);

    SubQAbstractProxyModel model;
    QCOMPARE(model.mapFromSource(sourceIndex), mapFromSource);
}

void tst_QAbstractProxyModel::mapSelectionFromSource_data()
{
    QTest::addColumn<QItemSelection>("selection");
    QTest::addColumn<QItemSelection>("mapSelectionFromSource");
    QTest::newRow("null") << QItemSelection() << QItemSelection();
    QTest::newRow("empty") << QItemSelection(QModelIndex(), QModelIndex()) << QItemSelection(QModelIndex(), QModelIndex());
}

// public QItemSelection mapSelectionFromSource(QItemSelection const& selection) const
void tst_QAbstractProxyModel::mapSelectionFromSource()
{
    QFETCH(QItemSelection, selection);
    QFETCH(QItemSelection, mapSelectionFromSource);

    SubQAbstractProxyModel model;
    QCOMPARE(model.mapSelectionFromSource(selection), mapSelectionFromSource);
}

void tst_QAbstractProxyModel::mapSelectionToSource_data()
{
    QTest::addColumn<QItemSelection>("selection");
    QTest::addColumn<QItemSelection>("mapSelectionToSource");
    QTest::newRow("null") << QItemSelection() << QItemSelection();
    QTest::newRow("empty") << QItemSelection(QModelIndex(), QModelIndex()) << QItemSelection(QModelIndex(), QModelIndex());
}

// public QItemSelection mapSelectionToSource(QItemSelection const& selection) const
void tst_QAbstractProxyModel::mapSelectionToSource()
{
    QFETCH(QItemSelection, selection);
    QFETCH(QItemSelection, mapSelectionToSource);

    SubQAbstractProxyModel model;
    QCOMPARE(model.mapSelectionToSource(selection), mapSelectionToSource);
}

void tst_QAbstractProxyModel::mapToSource_data()
{
    QTest::addColumn<QModelIndex>("proxyIndex");
    QTest::addColumn<QModelIndex>("mapToSource");
    QTest::newRow("null") << QModelIndex() << QModelIndex();
}

// public QModelIndex mapToSource(QModelIndex const& proxyIndex) const
void tst_QAbstractProxyModel::mapToSource()
{
    QFETCH(QModelIndex, proxyIndex);
    QFETCH(QModelIndex, mapToSource);

    SubQAbstractProxyModel model;
    QCOMPARE(model.mapToSource(proxyIndex), mapToSource);
}

// public void revert()
void tst_QAbstractProxyModel::revert()
{
    SubQAbstractProxyModel model;
    model.revert();
}

// public void setSourceModel(QAbstractItemModel* sourceModel)
void tst_QAbstractProxyModel::setSourceModel()
{
    SubQAbstractProxyModel model;

    QCOMPARE(model.property("sourceModel"), QVariant::fromValue<QAbstractItemModel*>(0));
    QStandardItemModel *sourceModel = new QStandardItemModel(&model);
    model.setSourceModel(sourceModel);
    QCOMPARE(model.sourceModel(), static_cast<QAbstractItemModel*>(sourceModel));

    QCOMPARE(model.property("sourceModel").value<QObject*>(), static_cast<QObject*>(sourceModel));
    QCOMPARE(model.property("sourceModel").value<QAbstractItemModel*>(), sourceModel);

    QStandardItemModel *sourceModel2 = new QStandardItemModel(&model);
    model.setSourceModel(sourceModel2);
    QCOMPARE(model.sourceModel(), static_cast<QAbstractItemModel*>(sourceModel2));

    QCOMPARE(model.property("sourceModel").value<QObject*>(), static_cast<QObject*>(sourceModel2));
    QCOMPARE(model.property("sourceModel").value<QAbstractItemModel*>(), sourceModel2);

    delete sourceModel2;
    QCOMPARE(model.sourceModel(), static_cast<QAbstractItemModel*>(0));
}

void tst_QAbstractProxyModel::submit_data()
{
    QTest::addColumn<bool>("submit");
    QTest::newRow("null") << true;
}

// public bool submit()
void tst_QAbstractProxyModel::submit()
{
    QFETCH(bool, submit);

    SubQAbstractProxyModel model;
    QCOMPARE(model.submit(), submit);
}

class StandardItemModelWithCustomRoleNames : public QStandardItemModel
{
public:
    enum CustomRole {
        CustomRole1 = Qt::UserRole,
        CustomRole2
    };

    StandardItemModelWithCustomRoleNames() {
        QHash<int, QByteArray> _roleNames = roleNames();
        _roleNames.insert(CustomRole1, "custom1");
        _roleNames.insert(CustomRole2, "custom2");
        setRoleNames(_roleNames);
    }
};

class AnotherStandardItemModelWithCustomRoleNames : public QStandardItemModel
{
public:
    enum CustomRole {
        AnotherCustomRole1 = Qt::UserRole + 10,  // Different to StandardItemModelWithCustomRoleNames::CustomRole1
        AnotherCustomRole2
    };

    AnotherStandardItemModelWithCustomRoleNames() {
        QHash<int, QByteArray> _roleNames = roleNames();
        _roleNames.insert(AnotherCustomRole1, "another_custom1");
        _roleNames.insert(AnotherCustomRole2, "another_custom2");
        setRoleNames(_roleNames);
    }
};

/**
    Verifies that @p subSet is a subset of @p superSet. That is, all keys in @p subSet exist in @p superSet and have the same values.
*/
static void verifySubSetOf(const QHash<int, QByteArray> &superSet, const QHash<int, QByteArray> &subSet)
{
    QHash<int, QByteArray>::const_iterator it = subSet.constBegin();
    const QHash<int, QByteArray>::const_iterator end = subSet.constEnd();
    for ( ; it != end; ++it ) {
        QVERIFY(superSet.contains(it.key()));
        QCOMPARE(it.value(), superSet.value(it.key()));
    }
}

void tst_QAbstractProxyModel::testRoleNames()
{
    QStandardItemModel defaultModel;
    StandardItemModelWithCustomRoleNames model;
    QHash<int, QByteArray> rootModelRoleNames = model.roleNames();
    QHash<int, QByteArray> defaultModelRoleNames = defaultModel.roleNames();

    verifySubSetOf( rootModelRoleNames, defaultModelRoleNames);
    QVERIFY( rootModelRoleNames.size() == defaultModelRoleNames.size() + 2 );
    QVERIFY( rootModelRoleNames.contains(StandardItemModelWithCustomRoleNames::CustomRole1));
    QVERIFY( rootModelRoleNames.contains(StandardItemModelWithCustomRoleNames::CustomRole2));
    QVERIFY( rootModelRoleNames.value(StandardItemModelWithCustomRoleNames::CustomRole1) == "custom1" );
    QVERIFY( rootModelRoleNames.value(StandardItemModelWithCustomRoleNames::CustomRole2) == "custom2" );

    SubQAbstractProxyModel proxy1;
    proxy1.setSourceModel(&model);
    QHash<int, QByteArray> proxy1RoleNames = proxy1.roleNames();
    verifySubSetOf( proxy1RoleNames, defaultModelRoleNames );
    QVERIFY( proxy1RoleNames.size() == defaultModelRoleNames.size() + 2 );
    QVERIFY( proxy1RoleNames.contains(StandardItemModelWithCustomRoleNames::CustomRole1));
    QVERIFY( proxy1RoleNames.contains(StandardItemModelWithCustomRoleNames::CustomRole2));
    QVERIFY( proxy1RoleNames.value(StandardItemModelWithCustomRoleNames::CustomRole1) == "custom1" );
    QVERIFY( proxy1RoleNames.value(StandardItemModelWithCustomRoleNames::CustomRole2) == "custom2" );

    SubQAbstractProxyModel proxy2;
    proxy2.setSourceModel(&proxy1);
    QHash<int, QByteArray> proxy2RoleNames = proxy2.roleNames();
    verifySubSetOf( proxy2RoleNames, defaultModelRoleNames );
    QVERIFY( proxy2RoleNames.size() == defaultModelRoleNames.size() + 2 );
    QVERIFY( proxy2RoleNames.contains(StandardItemModelWithCustomRoleNames::CustomRole1));
    QVERIFY( proxy2RoleNames.contains(StandardItemModelWithCustomRoleNames::CustomRole2));
    QVERIFY( proxy2RoleNames.value(StandardItemModelWithCustomRoleNames::CustomRole1) == "custom1" );
    QVERIFY( proxy2RoleNames.value(StandardItemModelWithCustomRoleNames::CustomRole2) == "custom2" );
}

// This class only supports very simple table models
class SwappingProxy : public QAbstractProxyModel
{
    static int swapRow(const int row)
    {
        if (row == 2) {
            return 3;
        } else if (row == 3) {
            return 2;
        } else {
            return row;
        }
    }
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parentIdx) const
    {
        if (!sourceModel())
            return QModelIndex();
        if (row < 0 || column < 0)
            return QModelIndex();
        if (row >= sourceModel()->rowCount())
            return QModelIndex();
        if (column >= sourceModel()->columnCount())
            return QModelIndex();
        return createIndex(row, column, parentIdx.internalPointer());
    }

    virtual QModelIndex parent(const QModelIndex &parentIdx) const
    {
        // well, we're a 2D model
        Q_UNUSED(parentIdx);
        return QModelIndex();
    }

    virtual int rowCount(const QModelIndex &parentIdx) const
    {
        if (parentIdx.isValid() || !sourceModel())
            return 0;
        return sourceModel()->rowCount();
    }

    virtual int columnCount(const QModelIndex &parentIdx) const
    {
        if (parentIdx.isValid() || !sourceModel())
            return 0;
        return sourceModel()->rowCount();
    }

    virtual QModelIndex mapToSource(const QModelIndex &proxyIndex) const
    {
        if (!proxyIndex.isValid())
            return QModelIndex();
        if (!sourceModel())
            return QModelIndex();
        Q_ASSERT(!proxyIndex.parent().isValid());
        return sourceModel()->index(swapRow(proxyIndex.row()), proxyIndex.column(), QModelIndex());
    }

    virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const
    {
        if (!sourceIndex.isValid())
            return QModelIndex();
        if (!sourceModel())
            return QModelIndex();
        Q_ASSERT(!sourceIndex.parent().isValid());
        return index(swapRow(sourceIndex.row()), sourceIndex.column(), QModelIndex());
    }
};

void tst_QAbstractProxyModel::testSwappingRowsProxy()
{
    QStandardItemModel defaultModel;
    defaultModel.setRowCount(4);
    defaultModel.setColumnCount(2);
    for (int row = 0; row < defaultModel.rowCount(); ++row) {
        defaultModel.setItem(row, 0, new QStandardItem(QString::number(row) + QLatin1Char('A')));
        defaultModel.setItem(row, 1, new QStandardItem(QString::number(row) + QLatin1Char('B')));
    }
    SwappingProxy proxy;
    proxy.setSourceModel(&defaultModel);
    QCOMPARE(proxy.data(proxy.index(0, 0, QModelIndex())), QVariant("0A"));
    QCOMPARE(proxy.data(proxy.index(0, 1, QModelIndex())), QVariant("0B"));
    QCOMPARE(proxy.data(proxy.index(1, 0, QModelIndex())), QVariant("1A"));
    QCOMPARE(proxy.data(proxy.index(1, 1, QModelIndex())), QVariant("1B"));
    QCOMPARE(proxy.data(proxy.index(2, 0, QModelIndex())), QVariant("3A"));
    QCOMPARE(proxy.data(proxy.index(2, 1, QModelIndex())), QVariant("3B"));
    QCOMPARE(proxy.data(proxy.index(3, 0, QModelIndex())), QVariant("2A"));
    QCOMPARE(proxy.data(proxy.index(3, 1, QModelIndex())), QVariant("2B"));

    for (int row = 0; row < defaultModel.rowCount(); ++row) {
        QModelIndex left = proxy.index(row, 0, QModelIndex());
        QModelIndex right = proxy.index(row, 1, QModelIndex());
        QCOMPARE(left.sibling(left.row(), 1), right);
        QCOMPARE(right.sibling(right.row(), 0), left);
    }
}

class StandardItemModelWithCustomDragAndDrop : public QStandardItemModel
{
public:
    QStringList mimeTypes() const { return QStringList() << QStringLiteral("foo/mimetype"); }
    Qt::DropActions supportedDragActions() const { return Qt::CopyAction | Qt::LinkAction; }
    Qt::DropActions supportedDropActions() const { return Qt::MoveAction; }
};

void tst_QAbstractProxyModel::testDragAndDrop()
{
    StandardItemModelWithCustomDragAndDrop sourceModel;
    SubQAbstractProxyModel proxy;
    proxy.setSourceModel(&sourceModel);
    QCOMPARE(proxy.mimeTypes(), sourceModel.mimeTypes());
    QCOMPARE(proxy.supportedDragActions(), sourceModel.supportedDragActions());
    QCOMPARE(proxy.supportedDropActions(), sourceModel.supportedDropActions());
}


QTEST_MAIN(tst_QAbstractProxyModel)
#include "tst_qabstractproxymodel.moc"

