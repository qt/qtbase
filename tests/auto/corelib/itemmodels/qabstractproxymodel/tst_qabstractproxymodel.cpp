// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtTest/private/qpropertytesthelper_p.h>

#ifndef QTEST_THROW_ON_FAIL
# error This test requires QTEST_THROW_ON_FAIL being active.
#endif

#include <qabstractproxymodel.h>
#include <QItemSelection>
#include <qstandarditemmodel.h>

#include <QtCore/qscopeguard.h>

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
    void headerDataInBounds();
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
    void sourceModelBinding();
};

// Subclass that exposes the protected functions.
class SubQAbstractProxyModel : public QAbstractProxyModel
{
public:
    // QAbstractProxyModel::mapFromSource is a pure virtual function.
    QModelIndex mapFromSource(QModelIndex const& sourceIndex) const override
        { Q_UNUSED(sourceIndex); return QModelIndex(); }

    // QAbstractProxyModel::mapToSource is a pure virtual function.
    QModelIndex mapToSource(QModelIndex const& proxyIndex) const override
        { Q_UNUSED(proxyIndex); return QModelIndex(); }

    QModelIndex index(int, int, const QModelIndex&) const override
    {
        return QModelIndex();
    }

    QModelIndex parent(const QModelIndex&) const override
    {
        return QModelIndex();
    }

    int rowCount(const QModelIndex&) const override
    {
        return 0;
    }

    int columnCount(const QModelIndex&) const override
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
    QTest::newRow("null") << QModelIndex() << Qt::ItemFlags{};
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

class SimpleTableReverseColumnsProxy : public QAbstractProxyModel
{
public:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return {};

        if (row < 0 || row >= rowCount() || column < 0 || column >= columnCount())
            qFatal("error"); // cannot QFAIL here

        return createIndex(row, column);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return 0;
        return sourceModel()->rowCount();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return 0;
        return sourceModel()->columnCount();
    }

    QModelIndex parent(const QModelIndex &) const override
    {
        return QModelIndex();
    }

    QModelIndex mapToSource(const QModelIndex &idx) const override
    {
        if (!idx.isValid())
            return QModelIndex();
        return sourceModel()->index(idx.row(), columnCount() - 1 - idx.column());
    }

    QModelIndex mapFromSource(const QModelIndex &idx) const override
    {
       if (idx.parent().isValid())
           return QModelIndex();
       return createIndex(idx.row(), columnCount() - 1 - idx.column());
    }
};

void tst_QAbstractProxyModel::headerDataInBounds()
{
    QStandardItemModel qsim(0, 5);
    qsim.setHorizontalHeaderLabels({"Col1", "Col2", "Col3", "Col4", "Col5"});

    SimpleTableReverseColumnsProxy proxy;
    QSignalSpy headerDataChangedSpy(&proxy, &QAbstractItemModel::headerDataChanged);
    QVERIFY(headerDataChangedSpy.isValid());
    proxy.setSourceModel(&qsim);
    QCOMPARE(proxy.rowCount(), 0);
    QCOMPARE(proxy.columnCount(), 5);

    for (int i = 0; i < proxy.columnCount(); ++i) {
        QString expected = QString("Col%1").arg(i + 1);
        QCOMPARE(proxy.headerData(i, Qt::Horizontal).toString(), expected);
    }

    qsim.appendRow({
                       new QStandardItem("A"),
                       new QStandardItem("B"),
                       new QStandardItem("C"),
                       new QStandardItem("D"),
                       new QStandardItem("E")
                   });

    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.columnCount(), 5);
    QTRY_COMPARE(headerDataChangedSpy.size(), 1);
    QCOMPARE(headerDataChangedSpy[0][0].value<Qt::Orientation>(), Qt::Horizontal);
    QCOMPARE(headerDataChangedSpy[0][1].value<int>(), 0);
    QCOMPARE(headerDataChangedSpy[0][2].value<int>(), 4);

    for (int i = 0; i < proxy.columnCount(); ++i) {
        QString expected = QString("Col%1").arg(proxy.columnCount() - i);
        QCOMPARE(proxy.headerData(i, Qt::Horizontal).toString(), expected);
    }

    qsim.appendRow({
                       new QStandardItem("A"),
                       new QStandardItem("B"),
                       new QStandardItem("C"),
                       new QStandardItem("D"),
                       new QStandardItem("E")
                   });
    QCOMPARE(proxy.rowCount(), 2);
    QCOMPARE(proxy.columnCount(), 5);
    QCOMPARE(headerDataChangedSpy.size(), 1);

    for (int i = 0; i < proxy.columnCount(); ++i) {
        QString expected = QString("Col%1").arg(proxy.columnCount() - i);
        QCOMPARE(proxy.headerData(i, Qt::Horizontal).toString(), expected);
    }

    QVERIFY(qsim.removeRows(0, 1));

    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.columnCount(), 5);
    QCOMPARE(headerDataChangedSpy.size(), 1);

    for (int i = 0; i < proxy.columnCount(); ++i) {
        QString expected = QString("Col%1").arg(proxy.columnCount() - i);
        QCOMPARE(proxy.headerData(i, Qt::Horizontal).toString(), expected);
    }

    QVERIFY(qsim.removeRows(0, 1));

    QCOMPARE(proxy.rowCount(), 0);
    QCOMPARE(proxy.columnCount(), 5);
    QTRY_COMPARE(headerDataChangedSpy.size(), 2);
    QCOMPARE(headerDataChangedSpy[1][0].value<Qt::Orientation>(), Qt::Horizontal);
    QCOMPARE(headerDataChangedSpy[1][1].value<int>(), 0);
    QCOMPARE(headerDataChangedSpy[1][2].value<int>(), 4);

    for (int i = 0; i < proxy.columnCount(); ++i) {
        QString expected = QString("Col%1").arg(i + 1);
        QCOMPARE(proxy.headerData(i, Qt::Horizontal).toString(), expected);
    }
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
    QCOMPARE(model.itemData(index).size(), count);
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

    QHash<int, QByteArray> roleNames() const override
    {
        auto result = QStandardItemModel::roleNames();
        result.insert(CustomRole1, QByteArrayLiteral("custom1"));
        result.insert(CustomRole2, QByteArrayLiteral("custom2"));
        return result;
    }
};

class AnotherStandardItemModelWithCustomRoleNames : public QStandardItemModel
{
public:
    enum CustomRole {
        AnotherCustomRole1 = Qt::UserRole + 10,  // Different to StandardItemModelWithCustomRoleNames::CustomRole1
        AnotherCustomRole2
    };

    QHash<int, QByteArray> roleNames() const override
    {
        return {{AnotherCustomRole1, QByteArrayLiteral("another_custom1")},
                {AnotherCustomRole2, QByteArrayLiteral("another_custom2")}};
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
    virtual QModelIndex index(int row, int column, const QModelIndex &parentIdx) const override
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

    virtual QModelIndex parent(const QModelIndex &parentIdx) const override
    {
        // well, we're a 2D model
        Q_UNUSED(parentIdx);
        return QModelIndex();
    }

    virtual int rowCount(const QModelIndex &parentIdx) const override
    {
        if (parentIdx.isValid() || !sourceModel())
            return 0;
        return sourceModel()->rowCount();
    }

    virtual int columnCount(const QModelIndex &parentIdx) const override
    {
        if (parentIdx.isValid() || !sourceModel())
            return 0;
        return sourceModel()->rowCount();
    }

    virtual QModelIndex mapToSource(const QModelIndex &proxyIndex) const override
    {
        if (!proxyIndex.isValid())
            return QModelIndex();
        if (!sourceModel())
            return QModelIndex();
        Q_ASSERT(!proxyIndex.parent().isValid());
        return sourceModel()->index(swapRow(proxyIndex.row()), proxyIndex.column(), QModelIndex());
    }

    virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override
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
        QCOMPARE(left.siblingAtColumn(1), right);
        QCOMPARE(right.siblingAtColumn(0), left);
    }
}

class StandardItemModelWithCustomDragAndDrop : public QStandardItemModel
{
public:
    QStringList mimeTypes() const override { return QStringList() << QStringLiteral("foo/mimetype"); }
    Qt::DropActions supportedDragActions() const override { return Qt::CopyAction | Qt::LinkAction; }
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }
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

void tst_QAbstractProxyModel::sourceModelBinding()
{
    SubQAbstractProxyModel proxy;
    QStandardItemModel model1;
    QStandardItemModel model2;
    const char *lhs;
    const char *rhs;

    auto printOnFailure = qScopeGuard([&] {
            qDebug("Failed %s - %s test", lhs, rhs);
        });
    lhs = "model";
    rhs = "model";
    QTestPrivate::testReadWritePropertyBasics<SubQAbstractProxyModel, QAbstractItemModel *>(
            proxy, &model1, &model2, "sourceModel");

    proxy.setSourceModel(&model2);
    lhs = "model";
    rhs = "nullptr";
    QTestPrivate::testReadWritePropertyBasics<SubQAbstractProxyModel, QAbstractItemModel *>(
            proxy, &model1, nullptr, "sourceModel");

    proxy.setSourceModel(&model1);
    lhs = "nullptr";
    rhs = "model";
    QTestPrivate::testReadWritePropertyBasics<SubQAbstractProxyModel, QAbstractItemModel *>(
            proxy, nullptr, &model2, "sourceModel");

    printOnFailure.dismiss();
}

QTEST_MAIN(tst_QAbstractProxyModel)
#include "tst_qabstractproxymodel.moc"

