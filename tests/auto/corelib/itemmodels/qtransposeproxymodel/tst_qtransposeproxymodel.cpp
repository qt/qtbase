/****************************************************************************
**
** Copyright (C) 2018 Luca Beldi <v.ronin@yahoo.it>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QTest>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QAbstractItemModelTester>
#include <random>

#include <qtransposeproxymodel.h>

class tst_QTransposeProxyModel : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void index();
    void data();
    void setData_data();
    void setData();
    void parent();
    void mapToSource();
    void mapFromSource();
    void basicTest_data();
    void basicTest();
    void sort();
    void insertRowBase_data();
    void insertRowBase();
    void insertColumnBase_data();
    void insertColumnBase();
    void insertColumnProxy_data();
    void insertColumnProxy();
    void insertRowProxy_data();
    void insertRowProxy();
    void removeRowBase_data();
    void removeRowBase();
    void removeColumnBase_data();
    void removeColumnBase();
    void removeColumnProxy_data();
    void removeColumnProxy();
    void removeRowProxy_data();
    void removeRowProxy();
    void headerData();
    void setHeaderData();
    void span();
    void itemData();
    void setItemData();
    void moveRowsBase();
    void moveColumnsProxy();
private:
    void testTransposed(
        const QAbstractItemModel *const baseModel,
        const QAbstractItemModel *const transposed,
        const QModelIndex &baseParent = QModelIndex(),
        const QModelIndex &transposedParent = QModelIndex()
    );
    QAbstractItemModel *createListModel(QObject *parent);
    QAbstractItemModel *createTableModel(QObject *parent);
    QAbstractItemModel *createTreeModel(QObject *parent);
};

QAbstractItemModel *tst_QTransposeProxyModel::createListModel(QObject *parent)
{
    QStringList sequence;
    sequence.reserve(10);
    for (int i = 0; i < 10; ++i)
        sequence.append(QString::number(i));
    return new QStringListModel(sequence, parent);
}

QAbstractItemModel *tst_QTransposeProxyModel::createTableModel(QObject *parent)
{
    QAbstractItemModel *model = new QStandardItemModel(parent);
    model->insertRows(0, 5);
    model->insertColumns(0, 4);
    for (int i = 0; i < model->rowCount(); ++i) {
        for (int j = 0; j < model->columnCount(); ++j) {
            model->setData(model->index(i, j), QStringLiteral("%1,%2").arg(i).arg(j), Qt::EditRole);
            model->setData(model->index(i, j), i, Qt::UserRole);
            model->setData(model->index(i, j), j, Qt::UserRole + 1);
        }
    }
    return model;
}

QAbstractItemModel *tst_QTransposeProxyModel::createTreeModel(QObject *parent)
{
    QAbstractItemModel *model = new QStandardItemModel(parent);
    model->insertRows(0, 5);
    model->insertColumns(0, 4);
    for (int i = 0; i < model->rowCount(); ++i) {
        for (int j = 0; j < model->columnCount(); ++j) {
            const QModelIndex parIdx = model->index(i, j);
            model->setData(parIdx, QStringLiteral("%1,%2").arg(i).arg(j), Qt::EditRole);
            model->setData(parIdx, i, Qt::UserRole);
            model->setData(parIdx, j, Qt::UserRole + 1);
            model->insertRows(0, 3, parIdx);
            model->insertColumns(0, 2, parIdx);
            for (int h = 0; h < model->rowCount(parIdx); ++h) {
                for (int k = 0; k < model->columnCount(parIdx); ++k) {
                    const QModelIndex childIdx = model->index(h, k, parIdx);
                    model->setData(childIdx, QStringLiteral("%1,%2,%3,%4").arg(i).arg(j).arg(h).arg(k), Qt::EditRole);
                    model->setData(childIdx, i, Qt::UserRole);
                    model->setData(childIdx, j, Qt::UserRole + 1);
                    model->setData(childIdx, h, Qt::UserRole + 2);
                    model->setData(childIdx, k, Qt::UserRole + 3);
                }
            }
        }
    }
    return model;
}

void tst_QTransposeProxyModel::testTransposed(
    const QAbstractItemModel *const baseModel,
    const QAbstractItemModel *const transposed,
    const QModelIndex &baseParent,
    const QModelIndex &transposedParent
)
{
    QCOMPARE(transposed->hasChildren(transposedParent), baseModel->hasChildren(baseParent));
    QCOMPARE(transposed->columnCount(transposedParent), baseModel->rowCount(baseParent));
    QCOMPARE(transposed->rowCount(transposedParent), baseModel->columnCount(baseParent));
    for (int i = 0, maxRow = baseModel->rowCount(baseParent); i < maxRow; ++i) {
        for (int j = 0, maxCol = baseModel->columnCount(baseParent); j < maxCol; ++j) {
            const QModelIndex baseIdx = baseModel->index(i, j, baseParent);
            const QModelIndex transIdx = transposed->index(j, i, transposedParent);
            QCOMPARE(transIdx.data(), baseIdx.data());
            QCOMPARE(transIdx.data(Qt::UserRole), baseIdx.data(Qt::UserRole));
            QCOMPARE(transIdx.data(Qt::UserRole + 1), baseIdx.data(Qt::UserRole + 1));
            QCOMPARE(transIdx.data(Qt::UserRole + 2), baseIdx.data(Qt::UserRole + 2));
            QCOMPARE(transIdx.data(Qt::UserRole + 3), baseIdx.data(Qt::UserRole + 3));
            if (baseModel->hasChildren(baseIdx)) {
                testTransposed(baseModel, transposed, baseIdx, transIdx);
            }
        }
    }
}

void tst_QTransposeProxyModel::initTestCase()
{
    qRegisterMetaType<QList<QPersistentModelIndex> >();
    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>();
}

void tst_QTransposeProxyModel::index()
{
    QAbstractItemModel *model = createTreeModel(this);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    QVERIFY(!proxy.index(0, -1).isValid());
    QVERIFY(!proxy.index(0, -1).isValid());
    QVERIFY(!proxy.index(-1, -1).isValid());
    QVERIFY(!proxy.index(0, proxy.columnCount()).isValid());
    QVERIFY(!proxy.index(proxy.rowCount(), 0).isValid());
    QVERIFY(!proxy.index(proxy.rowCount(), proxy.columnCount()).isValid());
    QModelIndex tempIdx = proxy.index(0, 1);
    QVERIFY(tempIdx.isValid());
    QCOMPARE(tempIdx.row(), 0);
    QCOMPARE(tempIdx.column(), 1);
    tempIdx = proxy.index(0, 1, tempIdx);
    QVERIFY(tempIdx.isValid());
    QCOMPARE(tempIdx.row(), 0);
    QCOMPARE(tempIdx.column(), 1);
    delete model;
}

void tst_QTransposeProxyModel::data()
{
    QStringListModel model{QStringList{"A", "B"}};
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(&model);
    QCOMPARE(proxy.index(0, 1).data().toString(), QStringLiteral("B"));
}

void tst_QTransposeProxyModel::parent()
{
    QAbstractItemModel *model = createTreeModel(this);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    const QModelIndex parentIdx = proxy.index(0, 0);
    const QModelIndex childIdx = proxy.index(0, 0, parentIdx);
    QVERIFY(parentIdx.isValid());
    QVERIFY(childIdx.isValid());
    QCOMPARE(childIdx.parent(), parentIdx);
    delete model;
}

void tst_QTransposeProxyModel::mapToSource()
{
    QAbstractItemModel *model = createTreeModel(this);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    QVERIFY(!proxy.mapToSource(QModelIndex()).isValid());
    QCOMPARE(proxy.mapToSource(proxy.index(0, 0)), model->index(0, 0));
    QCOMPARE(proxy.mapToSource(proxy.index(1, 0)), model->index(0, 1));
    QCOMPARE(proxy.mapToSource(proxy.index(0, 1)), model->index(1, 0));
    const QModelIndex proxyParent = proxy.index(1, 0);
    const QModelIndex sourceParent = model->index(0, 1);
    QCOMPARE(proxy.mapToSource(proxy.index(0, 0, proxyParent)), model->index(0, 0, sourceParent));
    QCOMPARE(proxy.mapToSource(proxy.index(1, 0, proxyParent)), model->index(0, 1, sourceParent));
    QCOMPARE(proxy.mapToSource(proxy.index(0, 1, proxyParent)), model->index(1, 0, sourceParent));
    delete model;
}

void tst_QTransposeProxyModel::mapFromSource()
{
    QAbstractItemModel *model = createTreeModel(this);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    QVERIFY(!proxy.mapFromSource(QModelIndex()).isValid());
    QCOMPARE(proxy.mapFromSource(model->index(0, 0)), proxy.index(0, 0));
    QCOMPARE(proxy.mapFromSource(model->index(0, 1)), proxy.index(1, 0));
    QCOMPARE(proxy.mapFromSource(model->index(1, 0)), proxy.index(0, 1));
    const QModelIndex proxyParent = proxy.index(1, 0);
    const QModelIndex sourceParent = model->index(0, 1);
    QCOMPARE(proxy.mapToSource(proxy.index(0, 0, proxyParent)), model->index(0, 0, sourceParent));
    QCOMPARE(proxy.mapFromSource(model->index(1, 0, sourceParent)), proxy.index(0, 1, proxyParent));
    QCOMPARE(proxy.mapFromSource(model->index(0, 1, sourceParent)), proxy.index(1, 0, proxyParent));
    delete model;
}

void tst_QTransposeProxyModel::basicTest_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::newRow("List") << createListModel(this);
    QTest::newRow("Table") << createTableModel(this);
    QTest::newRow("Tree") << createTreeModel(this);
}

void tst_QTransposeProxyModel::basicTest()
{
    QFETCH(QAbstractItemModel *, model);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    testTransposed(model, &proxy);
    delete model;
}

void tst_QTransposeProxyModel::sort()
{
    QStringList sequence;
    sequence.reserve(100);
    for (int i = 0; i < 100; ++i)
        sequence.append(QStringLiteral("%1").arg(i, 3, 10, QLatin1Char('0')));
    std::shuffle(sequence.begin(), sequence.end(), std::mt19937(88));
    const QString firstItemBeforeSort = sequence.first();
    QStringListModel baseModel(sequence);
    QTransposeProxyModel proxyModel;
    new QAbstractItemModelTester(&proxyModel, &proxyModel);
    proxyModel.setSourceModel(&baseModel);
    QSignalSpy layoutChangedSpy(&proxyModel, &QAbstractItemModel::layoutChanged);
    QVERIFY(layoutChangedSpy.isValid());
    QSignalSpy layoutAboutToBeChangedSpy(&proxyModel, &QAbstractItemModel::layoutAboutToBeChanged);
    QVERIFY(layoutAboutToBeChangedSpy.isValid());
    QPersistentModelIndex firstIndexBeforeSort = proxyModel.index(0, 0);
    baseModel.sort(0, Qt::AscendingOrder);
    QCOMPARE(layoutChangedSpy.count(), 1);
    QCOMPARE(layoutAboutToBeChangedSpy.count(), 1);
    QCOMPARE(layoutChangedSpy.takeFirst().at(1).toInt(), int(QAbstractItemModel::HorizontalSortHint));
    QCOMPARE(firstIndexBeforeSort.data().toString(), firstItemBeforeSort);
    for (int i = 0; i < 100; ++i)
        QCOMPARE(proxyModel.index(0, i).data().toInt(), i);
}

void tst_QTransposeProxyModel::removeColumnBase_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::addColumn<QModelIndex>("parent");
    QTest::newRow("Table") << createTableModel(this) << QModelIndex();
    QTest::newRow("Tree_Root_Item") << createTreeModel(this) << QModelIndex();
    QAbstractItemModel *model = createTreeModel(this);
    QTest::newRow("Tree_Child_Item") << model << model->index(0, 0);
}

void tst_QTransposeProxyModel::removeColumnBase()
{
    QFETCH(QAbstractItemModel * const, model);
    QFETCH(const QModelIndex, parent);
    QTransposeProxyModel proxy;
    QSignalSpy rowRemoveSpy(&proxy, &QAbstractItemModel::rowsRemoved);
    QVERIFY(rowRemoveSpy.isValid());
    QSignalSpy rowAboutToBeRemoveSpy(&proxy, &QAbstractItemModel::rowsAboutToBeRemoved);
    QVERIFY(rowAboutToBeRemoveSpy.isValid());
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    const int oldRowCount = proxy.rowCount(proxy.mapFromSource(parent));
    const QVariant expectedNewVal = model->index(0, 2, parent).data();
    QVERIFY(model->removeColumn(1, parent));
    QCOMPARE(proxy.rowCount(proxy.mapFromSource(parent)), oldRowCount - 1);
    QCOMPARE(proxy.index(1, 0, proxy.mapFromSource(parent)).data(), expectedNewVal);
    QCOMPARE(rowRemoveSpy.count(), 1);
    QCOMPARE(rowAboutToBeRemoveSpy.count(), 1);
    for (const auto &spyArgs : {rowRemoveSpy.takeFirst(),
        rowAboutToBeRemoveSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), proxy.mapFromSource(parent));
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    delete model;
}

void tst_QTransposeProxyModel::insertColumnBase_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::addColumn<QModelIndex>("parent");
    QTest::newRow("Table") << createTableModel(this) << QModelIndex();
    QTest::newRow("Tree_Root_Item") << createTreeModel(this) << QModelIndex();
    QAbstractItemModel *model = createTreeModel(this);
    QTest::newRow("Tree_Child_Item") << model << model->index(0, 0);
}

void tst_QTransposeProxyModel::insertColumnBase()
{
    QFETCH(QAbstractItemModel * const, model);
    QFETCH(const QModelIndex, parent);
    QTransposeProxyModel proxy;
    QSignalSpy rowInsertSpy(&proxy, &QAbstractItemModel::rowsInserted);
    QVERIFY(rowInsertSpy.isValid());
    QSignalSpy rowAboutToBeInsertSpy(&proxy, &QAbstractItemModel::rowsAboutToBeInserted);
    QVERIFY(rowAboutToBeInsertSpy.isValid());
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    const int oldRowCount = proxy.rowCount(proxy.mapFromSource(parent));
    QVERIFY(model->insertColumn(1, parent));
    QCOMPARE(proxy.rowCount(proxy.mapFromSource(parent)), oldRowCount + 1);
    QVERIFY(!proxy.index(1, 0, proxy.mapFromSource(parent)).data().isValid());
    QCOMPARE(rowInsertSpy.count(), 1);
    QCOMPARE(rowAboutToBeInsertSpy.count(), 1);
    for (const auto &spyArgs : {rowInsertSpy.takeFirst(),
        rowAboutToBeInsertSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), proxy.mapFromSource(parent));
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    delete model;
}

void tst_QTransposeProxyModel::removeRowBase_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::addColumn<QModelIndex>("parent");
    QTest::newRow("List") << createListModel(this) << QModelIndex();
    QTest::newRow("Table") << createTableModel(this) << QModelIndex();
    QTest::newRow("Tree_Root_Item") << createTreeModel(this) << QModelIndex();
    QAbstractItemModel *model = createTreeModel(this);
    QTest::newRow("Tree_Child_Item") << model << model->index(0, 0);
}

void tst_QTransposeProxyModel::removeRowBase()
{
    QFETCH(QAbstractItemModel * const, model);
    QFETCH(const QModelIndex, parent);
    QTransposeProxyModel proxy;
    QSignalSpy columnsRemoveSpy(&proxy, &QAbstractItemModel::columnsRemoved);
    QVERIFY(columnsRemoveSpy.isValid());
    QSignalSpy columnsAboutToBeRemoveSpy(&proxy, &QAbstractItemModel::columnsAboutToBeRemoved);
    QVERIFY(columnsAboutToBeRemoveSpy.isValid());
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    const int oldColCount = proxy.columnCount(proxy.mapFromSource(parent));
    const QVariant expectedNewVal = model->index(2, 0, parent).data();
    QVERIFY(model->removeRow(1, parent));
    QCOMPARE(proxy.columnCount(proxy.mapFromSource(parent)), oldColCount - 1);
    QCOMPARE(proxy.index(0, 1, proxy.mapFromSource(parent)).data(), expectedNewVal);
    QCOMPARE(columnsRemoveSpy.count(), 1);
    QCOMPARE(columnsAboutToBeRemoveSpy.count(), 1);
    for (const auto &spyArgs : {columnsRemoveSpy.takeFirst(),
        columnsAboutToBeRemoveSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), proxy.mapFromSource(parent));
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    delete model;
}

void tst_QTransposeProxyModel::insertRowBase_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::addColumn<QModelIndex>("parent");
    QTest::newRow("List") << createListModel(this) << QModelIndex();
    QTest::newRow("Table") << createTableModel(this) << QModelIndex();
    QTest::newRow("Tree_Root_Item") << createTreeModel(this) << QModelIndex();
    QAbstractItemModel *model = createTreeModel(this);
    QTest::newRow("Tree_Child_Item") << model << model->index(0, 0);
}

void tst_QTransposeProxyModel::insertRowBase()
{
    QFETCH(QAbstractItemModel * const, model);
    QFETCH(const QModelIndex, parent);
    QTransposeProxyModel proxy;
    QSignalSpy columnsInsertSpy(&proxy, &QAbstractItemModel::columnsInserted);
    QVERIFY(columnsInsertSpy.isValid());
    QSignalSpy columnsAboutToBeInsertSpy(&proxy, &QAbstractItemModel::columnsAboutToBeInserted);
    QVERIFY(columnsAboutToBeInsertSpy.isValid());
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    const int oldColCount = proxy.columnCount(proxy.mapFromSource(parent));
    QVERIFY(model->insertRow(1, parent));
    QCOMPARE(proxy.columnCount(proxy.mapFromSource(parent)), oldColCount + 1);
    QVERIFY(proxy.index(0, 1, proxy.mapFromSource(parent)).data().isNull());
    QCOMPARE(columnsInsertSpy.count(), 1);
    QCOMPARE(columnsAboutToBeInsertSpy.count(), 1);
    for (const auto &spyArgs : {columnsInsertSpy.takeFirst(),
        columnsAboutToBeInsertSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), proxy.mapFromSource(parent));
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    delete model;
}

void tst_QTransposeProxyModel::removeColumnProxy_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::addColumn<bool>("rootItem");
    QTest::newRow("List") << createListModel(this) << true;
    QTest::newRow("Table") << createTableModel(this) << true;
    QTest::newRow("Tree_Root_Item") << createTreeModel(this) << true;
    QTest::newRow("Tree_Child_Item") << createTreeModel(this) << false;
}

void tst_QTransposeProxyModel::removeColumnProxy()
{
    QFETCH(QAbstractItemModel *, model);
    QFETCH(bool, rootItem);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    QSignalSpy columnsRemoveSpy(&proxy, &QAbstractItemModel::columnsRemoved);
    QVERIFY(columnsRemoveSpy.isValid());
    QSignalSpy columnsAboutToBeRemoveSpy(&proxy, &QAbstractItemModel::columnsAboutToBeRemoved);
    QVERIFY(columnsAboutToBeRemoveSpy.isValid());
    QSignalSpy rowsRemoveSpy(model, &QAbstractItemModel::rowsRemoved);
    QVERIFY(rowsRemoveSpy.isValid());
    QSignalSpy rowsAboutToBeRemoveSpy(model, &QAbstractItemModel::rowsAboutToBeRemoved);
    QVERIFY(rowsAboutToBeRemoveSpy.isValid());
    proxy.setSourceModel(model);
    const QModelIndex proxyParent = rootItem ? QModelIndex() : proxy.index(0, 1);
    const QModelIndex sourceParent = proxy.mapToSource(proxyParent);
    const int oldColCount = proxy.columnCount(proxyParent);
    const int oldRowCount = model->rowCount(sourceParent);
    const QVariant expectedNewVal = proxy.index(0, 2, proxyParent).data();
    QVERIFY(proxy.removeColumn(1, proxyParent));
    QCOMPARE(proxy.columnCount(proxyParent), oldColCount - 1);
    QCOMPARE(model->rowCount(sourceParent), oldRowCount - 1);
    QCOMPARE(proxy.index(0, 1, proxyParent).data(), expectedNewVal);
    QCOMPARE(model->index(1, 0, sourceParent).data(), expectedNewVal);
    QCOMPARE(columnsRemoveSpy.count(), 1);
    QCOMPARE(columnsAboutToBeRemoveSpy.count(), 1);
    QCOMPARE(rowsRemoveSpy.count(), 1);
    QCOMPARE(rowsAboutToBeRemoveSpy.count(), 1);
    for (const auto &spyArgs : {columnsRemoveSpy.takeFirst(),
        columnsAboutToBeRemoveSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), proxyParent);
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    for (const auto &spyArgs : {rowsRemoveSpy.takeFirst(),
        rowsAboutToBeRemoveSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), sourceParent);
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    delete model;
}

void tst_QTransposeProxyModel::insertColumnProxy_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::addColumn<bool>("rootItem");
    QTest::newRow("List") << createListModel(this) << true;
    QTest::newRow("Table") << createTableModel(this) << true;
    QTest::newRow("Tree_Root_Item") << createTreeModel(this) << true;
    QTest::newRow("Tree_Child_Item") << createTreeModel(this) << false;
}

void tst_QTransposeProxyModel::insertColumnProxy()
{
    QFETCH(QAbstractItemModel *, model);
    QFETCH(bool, rootItem);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    QSignalSpy columnsInsertSpy(&proxy, &QAbstractItemModel::columnsInserted);
    QVERIFY(columnsInsertSpy.isValid());
    QSignalSpy columnsAboutToBeInsertSpy(&proxy, &QAbstractItemModel::columnsAboutToBeInserted);
    QVERIFY(columnsAboutToBeInsertSpy.isValid());
    QSignalSpy rowsInsertSpy(model, &QAbstractItemModel::rowsInserted);
    QVERIFY(rowsInsertSpy.isValid());
    QSignalSpy rowsAboutToBeInsertSpy(model, &QAbstractItemModel::rowsAboutToBeInserted);
    QVERIFY(rowsAboutToBeInsertSpy.isValid());
    proxy.setSourceModel(model);
    const QModelIndex proxyParent = rootItem ? QModelIndex() : proxy.index(0, 1);
    const QModelIndex sourceParent = proxy.mapToSource(proxyParent);
    const int oldColCount = proxy.columnCount(proxyParent);
    const int oldRowCount = model->rowCount(sourceParent);
    QVERIFY(proxy.insertColumn(1, proxyParent));
    QCOMPARE(proxy.columnCount(proxyParent), oldColCount + 1);
    QCOMPARE(model->rowCount(sourceParent), oldRowCount + 1);
    QVERIFY(proxy.index(0, 1, proxyParent).data().isNull());
    QVERIFY(model->index(1, 0, sourceParent).data().isNull());
    QCOMPARE(columnsInsertSpy.count(), 1);
    QCOMPARE(columnsAboutToBeInsertSpy.count(), 1);
    QCOMPARE(rowsInsertSpy.count(), 1);
    QCOMPARE(rowsAboutToBeInsertSpy.count(), 1);
    for (const auto &spyArgs : {columnsInsertSpy.takeFirst(),
        columnsAboutToBeInsertSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), proxyParent);
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    for (const auto &spyArgs : {rowsInsertSpy.takeFirst(),
        rowsAboutToBeInsertSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), sourceParent);
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    delete model;
}

void tst_QTransposeProxyModel::removeRowProxy_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::addColumn<bool>("rootItem");
    QTest::newRow("Table") << createTableModel(this) << true;
    QTest::newRow("Tree_Root_Item") << createTreeModel(this) << true;
    QTest::newRow("Tree_Child_Item") << createTreeModel(this) << false;
}

void tst_QTransposeProxyModel::removeRowProxy()
{
    QFETCH(QAbstractItemModel *, model);
    QFETCH(bool, rootItem);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    QSignalSpy rowsRemoveSpy(&proxy, &QAbstractItemModel::rowsRemoved);
    QVERIFY(rowsRemoveSpy.isValid());
    QSignalSpy rowsAboutToBeRemoveSpy(&proxy, &QAbstractItemModel::rowsAboutToBeRemoved);
    QVERIFY(rowsAboutToBeRemoveSpy.isValid());
    QSignalSpy columnsRemoveSpy(model, &QAbstractItemModel::columnsRemoved);
    QVERIFY(columnsRemoveSpy.isValid());
    QSignalSpy columnsAboutToBeRemoveSpy(model, &QAbstractItemModel::columnsAboutToBeRemoved);
    QVERIFY(columnsAboutToBeRemoveSpy.isValid());
    proxy.setSourceModel(model);
    const QModelIndex proxyParent = rootItem ? QModelIndex() : proxy.index(0, 1);
    const QModelIndex sourceParent = proxy.mapToSource(proxyParent);
    const int oldRowCount = proxy.rowCount(proxyParent);
    const int oldColCount = model->columnCount(sourceParent);
    const QVariant expectedNewVal = proxy.index(2, 0, proxyParent).data();
    QVERIFY(proxy.removeRow(1, proxyParent));
    QCOMPARE(proxy.rowCount(proxyParent), oldRowCount - 1);
    QCOMPARE(model->columnCount(sourceParent), oldColCount - 1);
    QCOMPARE(proxy.index(1, 0, proxyParent).data(), expectedNewVal);
    QCOMPARE(model->index(0, 1, sourceParent).data(), expectedNewVal);
    QCOMPARE(columnsRemoveSpy.count(), 1);
    QCOMPARE(columnsAboutToBeRemoveSpy.count(), 1);
    QCOMPARE(rowsRemoveSpy.count(), 1);
    QCOMPARE(rowsAboutToBeRemoveSpy.count(), 1);
    for (const auto &spyArgs : {columnsRemoveSpy.takeFirst(),
        columnsAboutToBeRemoveSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), sourceParent);
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    for (const auto &spyArgs : {rowsRemoveSpy.takeFirst(),
        rowsAboutToBeRemoveSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), proxyParent);
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    delete model;
}

void tst_QTransposeProxyModel::insertRowProxy_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::addColumn<bool>("rootItem");
    QTest::newRow("Table") << createTableModel(this) << true;
    QTest::newRow("Tree_Root_Item") << createTreeModel(this) << true;
    QTest::newRow("Tree_Child_Item") << createTreeModel(this) << false;
}

void tst_QTransposeProxyModel::insertRowProxy()
{
    QFETCH(QAbstractItemModel *, model);
    QFETCH(bool, rootItem);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    QSignalSpy rowsInsertSpy(&proxy, &QAbstractItemModel::rowsInserted);
    QVERIFY(rowsInsertSpy.isValid());
    QSignalSpy rowsAboutToBeInsertSpy(&proxy, &QAbstractItemModel::rowsAboutToBeInserted);
    QVERIFY(rowsAboutToBeInsertSpy.isValid());
    QSignalSpy columnsInsertSpy(model, &QAbstractItemModel::columnsInserted);
    QVERIFY(columnsInsertSpy.isValid());
    QSignalSpy columnsAboutToBeInsertSpy(model, &QAbstractItemModel::columnsAboutToBeInserted);
    QVERIFY(columnsAboutToBeInsertSpy.isValid());
    proxy.setSourceModel(model);
    const QModelIndex proxyParent = rootItem ? QModelIndex() : proxy.index(0, 1);
    const QModelIndex sourceParent = proxy.mapToSource(proxyParent);
    const int oldRowCount = proxy.rowCount(proxyParent);
    const int oldColCount = model->columnCount(sourceParent);
    QVERIFY(proxy.insertRow(1, proxyParent));
    QCOMPARE(proxy.rowCount(proxyParent), oldRowCount + 1);
    QCOMPARE(model->columnCount(sourceParent), oldColCount + 1);
    QVERIFY(proxy.index(1, 0, proxyParent).data().isNull());
    QVERIFY(model->index(0, 1, sourceParent).data().isNull());
    QCOMPARE(columnsInsertSpy.count(), 1);
    QCOMPARE(columnsAboutToBeInsertSpy.count(), 1);
    QCOMPARE(rowsInsertSpy.count(), 1);
    QCOMPARE(rowsAboutToBeInsertSpy.count(), 1);
    for (const auto &spyArgs : {columnsInsertSpy.takeFirst(),
        columnsAboutToBeInsertSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), sourceParent);
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    for (const auto &spyArgs : {rowsInsertSpy.takeFirst(),
        rowsAboutToBeInsertSpy.takeFirst()}) {
        QCOMPARE(spyArgs.at(0).value<QModelIndex>(), proxyParent);
        QCOMPARE(spyArgs.at(1).toInt(), 1);
        QCOMPARE(spyArgs.at(2).toInt(), 1);
    }
    delete model;
}

void tst_QTransposeProxyModel::headerData()
{
    QStandardItemModel model;
    model.insertRows(0, 3);
    model.insertColumns(0, 5);
    for (int i = 0; i < model.rowCount(); ++i)
        model.setHeaderData(i, Qt::Horizontal, QChar('A' + i));
    for (int i = 1; i <= model.columnCount(); ++i)
        model.setHeaderData(i, Qt::Vertical, i);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(&model);
    for (int i = 0; i < model.rowCount(); ++i)
        QCOMPARE(model.headerData(i, Qt::Horizontal), proxy.headerData(i, Qt::Vertical));
    for (int i = 0; i < model.columnCount(); ++i)
        QCOMPARE(model.headerData(i, Qt::Vertical), proxy.headerData(i, Qt::Horizontal));
}

void tst_QTransposeProxyModel::setHeaderData()
{
    QStandardItemModel model;
    model.insertRows(0, 3);
    model.insertColumns(0, 5);
    for (int i = 0; i < model.rowCount(); ++i)
        model.setHeaderData(i, Qt::Horizontal, QChar('A' + i));
    for (int i = 1; i <= model.columnCount(); ++i)
        model.setHeaderData(i, Qt::Vertical, i);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(&model);
    QVERIFY(proxy.setHeaderData(1, Qt::Horizontal, 99));
    QCOMPARE(model.headerData(1, Qt::Vertical).toInt(), 99);
    QVERIFY(proxy.setHeaderData(1, Qt::Vertical, QChar('Z')));
    QCOMPARE(model.headerData(1, Qt::Horizontal).toChar(), QChar('Z'));
}

void tst_QTransposeProxyModel::span()
{
    class SpanModel : public QStandardItemModel
    {
        Q_DISABLE_COPY(SpanModel)
    public:
        SpanModel(int rows, int columns, QObject *parent = nullptr)
            : QStandardItemModel(rows, columns, parent)
        {}
        QSize span(const QModelIndex &index) const override
        {
            Q_UNUSED(index)
            return QSize(2, 1);
        }
    };
    SpanModel model(3, 5);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(&model);
    QCOMPARE(proxy.span(proxy.index(0, 0)), QSize(1, 2));
}

void tst_QTransposeProxyModel::itemData()
{
    QAbstractItemModel *model = createTreeModel(this);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    QMap<int, QVariant> itmData = proxy.itemData(proxy.index(0, 1));
    QCOMPARE(itmData.value(Qt::DisplayRole).toString(), QStringLiteral("1,0"));
    QCOMPARE(itmData.value(Qt::UserRole).toInt(), 1);
    QCOMPARE(itmData.value(Qt::UserRole + 1).toInt(), 0);
    itmData = proxy.itemData(proxy.index(1, 2, proxy.index(0, 1)));
    QCOMPARE(itmData.value(Qt::DisplayRole).toString(), QStringLiteral("1,0,2,1"));
    QCOMPARE(itmData.value(Qt::UserRole).toInt(), 1);
    QCOMPARE(itmData.value(Qt::UserRole + 1).toInt(), 0);
    QCOMPARE(itmData.value(Qt::UserRole + 2).toInt(), 2);
    QCOMPARE(itmData.value(Qt::UserRole + 3).toInt(), 1);
    QVERIFY(proxy.itemData(QModelIndex()).isEmpty());
    delete model;
}

void tst_QTransposeProxyModel::setItemData()
{
    QAbstractItemModel *model = createTreeModel(this);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    QSignalSpy sourceDataChangeSpy(model, &QAbstractItemModel::dataChanged);
    QVERIFY(sourceDataChangeSpy.isValid());
    QSignalSpy proxyDataChangeSpy(&proxy, &QAbstractItemModel::dataChanged);
    QVERIFY(proxyDataChangeSpy.isValid());
    const QMap<int, QVariant> itmData = {
        std::make_pair<int, QVariant>(Qt::DisplayRole, QStringLiteral("Test")),
        std::make_pair<int, QVariant>(Qt::UserRole, 88),
        std::make_pair<int, QVariant>(Qt::UserRole + 1, 99),
    };
    QModelIndex idx = proxy.index(0, 1);
    QVERIFY(proxy.setItemData(idx, itmData));
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Test"));
    QCOMPARE(idx.data(Qt::UserRole).toInt(), 88);
    QCOMPARE(idx.data(Qt::UserRole + 1).toInt(), 99);
    QCOMPARE(sourceDataChangeSpy.size(), 1);
    QCOMPARE(proxyDataChangeSpy.size(), 1);
    auto signalData = proxyDataChangeSpy.takeFirst();
    QCOMPARE(signalData.at(0).value<QModelIndex>(), idx);
    QCOMPARE(signalData.at(1).value<QModelIndex>(), idx);
    const QVector<int> expectedRoles{Qt::DisplayRole, Qt::UserRole, Qt::EditRole, Qt::UserRole + 1};
    QVector<int> receivedRoles = signalData.at(2).value<QVector<int> >();
    QCOMPARE(receivedRoles.size(), expectedRoles.size());
    for (int role : expectedRoles)
        QVERIFY(receivedRoles.contains(role));
    signalData = sourceDataChangeSpy.takeFirst();
    QCOMPARE(signalData.at(0).value<QModelIndex>(), proxy.mapToSource(idx));
    QCOMPARE(signalData.at(1).value<QModelIndex>(), proxy.mapToSource(idx));
    receivedRoles = signalData.at(2).value<QVector<int> >();
    QCOMPARE(receivedRoles.size(), expectedRoles.size());
    for (int role : expectedRoles)
        QVERIFY(receivedRoles.contains(role));
    idx = proxy.index(1, 2, proxy.index(0, 1));
    QVERIFY(proxy.setItemData(idx, itmData));
    QCOMPARE(idx.data(Qt::DisplayRole).toString(), QStringLiteral("Test"));
    QCOMPARE(idx.data(Qt::UserRole).toInt(), 88);
    QCOMPARE(idx.data(Qt::UserRole + 1).toInt(), 99);
    QCOMPARE(idx.data(Qt::UserRole + 2).toInt(), 2);
    QCOMPARE(idx.data(Qt::UserRole + 3).toInt(), 1);
    QCOMPARE(sourceDataChangeSpy.size(), 1);
    QCOMPARE(proxyDataChangeSpy.size(), 1);
    signalData = proxyDataChangeSpy.takeFirst();
    QCOMPARE(signalData.at(0).value<QModelIndex>(), idx);
    QCOMPARE(signalData.at(1).value<QModelIndex>(), idx);
    receivedRoles = signalData.at(2).value<QVector<int> >();
    QCOMPARE(receivedRoles.size(), expectedRoles.size());
    for (int role : expectedRoles)
        QVERIFY(receivedRoles.contains(role));
    signalData = sourceDataChangeSpy.takeFirst();
    QCOMPARE(signalData.at(0).value<QModelIndex>(), proxy.mapToSource(idx));
    QCOMPARE(signalData.at(1).value<QModelIndex>(), proxy.mapToSource(idx));
    receivedRoles = signalData.at(2).value<QVector<int> >();
    QCOMPARE(receivedRoles.size(), expectedRoles.size());
    for (int role : expectedRoles)
        QVERIFY(receivedRoles.contains(role));
    QVERIFY(!proxy.setItemData(QModelIndex(), itmData));
    delete model;
}

void tst_QTransposeProxyModel::moveRowsBase()
{
    QStringListModel model{QStringList{"A", "B", "C", "D"}};
    QTransposeProxyModel proxy;
    QSignalSpy columnsMoveSpy(&proxy, &QAbstractItemModel::columnsMoved);
    QVERIFY(columnsMoveSpy.isValid());
    QSignalSpy columnsAboutToBeMoveSpy(&proxy, &QAbstractItemModel::columnsAboutToBeMoved);
    QVERIFY(columnsAboutToBeMoveSpy.isValid());
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(&model);
    const QStringList expectedNewVal = {"B", "A", "C", "D"};
    QVERIFY(model.moveRows(QModelIndex(), 0, 1, QModelIndex(), 2));
    for (int i = 0; i < expectedNewVal.size(); ++i)
        QCOMPARE(proxy.index(0, i).data(), expectedNewVal.at(i));
    QCOMPARE(columnsMoveSpy.count(), 1);
    QCOMPARE(columnsAboutToBeMoveSpy.count(), 1);
    for (const auto &spyArgs : {columnsMoveSpy.takeFirst(),
        columnsAboutToBeMoveSpy.takeFirst()}) {
        QVERIFY(!spyArgs.at(0).value<QModelIndex>().isValid());
        QCOMPARE(spyArgs.at(1).toInt(), 0);
        QCOMPARE(spyArgs.at(2).toInt(), 0);
        QVERIFY(!spyArgs.at(3).value<QModelIndex>().isValid());
        QCOMPARE(spyArgs.at(4).toInt(), 2);
    }
}

void tst_QTransposeProxyModel::moveColumnsProxy()
{
    QStringListModel model{QStringList{"A", "B", "C", "D"}};
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    QSignalSpy columnsMoveSpy(&proxy, &QAbstractItemModel::columnsMoved);
    QVERIFY(columnsMoveSpy.isValid());
    QSignalSpy columnsAboutToBeMoveSpy(&proxy, &QAbstractItemModel::columnsAboutToBeMoved);
    QVERIFY(columnsAboutToBeMoveSpy.isValid());
    QSignalSpy rowsMoveSpy(&model, &QAbstractItemModel::rowsMoved);
    QVERIFY(rowsMoveSpy.isValid());
    QSignalSpy rowsAboutToBeMoveSpy(&model, &QAbstractItemModel::rowsAboutToBeMoved);
    QVERIFY(rowsAboutToBeMoveSpy.isValid());
    proxy.setSourceModel(&model);
    const QStringList expectedNewVal = {"B", "A", "C", "D"};
    QVERIFY(proxy.moveColumns(QModelIndex(), 0, 1, QModelIndex(), 2));
    for (int i = 0; i < expectedNewVal.size(); ++i)
        QCOMPARE(proxy.index(0, i).data(), expectedNewVal.at(i));
    for (int i = 0; i < expectedNewVal.size(); ++i)
        QCOMPARE(model.index(i, 0).data(), expectedNewVal.at(i));
    QCOMPARE(columnsMoveSpy.count(), 1);
    QCOMPARE(columnsAboutToBeMoveSpy.count(), 1);
    QCOMPARE(rowsMoveSpy.count(), 1);
    QCOMPARE(rowsAboutToBeMoveSpy.count(), 1);
    for (const auto &spyArgs : {columnsMoveSpy.takeFirst(),
        columnsAboutToBeMoveSpy.takeFirst(),
        rowsMoveSpy.takeFirst(),rowsAboutToBeMoveSpy.takeFirst()}) {
        QVERIFY(!spyArgs.at(0).value<QModelIndex>().isValid());
        QCOMPARE(spyArgs.at(1).toInt(), 0);
        QCOMPARE(spyArgs.at(2).toInt(), 0);
        QVERIFY(!spyArgs.at(3).value<QModelIndex>().isValid());
    }
}

void tst_QTransposeProxyModel::setData_data()
{
    QTest::addColumn<QAbstractItemModel *>("model");
    QTest::addColumn<bool>("rootItem");
    QTest::addColumn<bool>("viaProxy");
    QTest::newRow("List_via_Base") << createListModel(this) << true << false;
    QTest::newRow("Table_via_Base") << createTableModel(this) << true << false;
    QTest::newRow("Tree_via_Base_Root_Item") << createTreeModel(this) << true << false;
    QTest::newRow("Tree_via_Base_Child_Item") << createTreeModel(this) << false << false;
    QTest::newRow("List_via_Proxy") << createListModel(this) << true << true;
    QTest::newRow("Table_via_Proxy") << createTableModel(this) << true << true;
    QTest::newRow("Tree_via_Proxy_Root_Item") << createTreeModel(this) << true << true;
    QTest::newRow("Tree_via_Proxy_Child_Item") << createTreeModel(this) << false << true;
}

void tst_QTransposeProxyModel::setData()
{
    QFETCH(QAbstractItemModel *, model);
    QFETCH(bool, rootItem);
    QFETCH(bool, viaProxy);
    QTransposeProxyModel proxy;
    new QAbstractItemModelTester(&proxy, &proxy);
    proxy.setSourceModel(model);
    QSignalSpy sourceDataChangeSpy(model, &QAbstractItemModel::dataChanged);
    QVERIFY(sourceDataChangeSpy.isValid());
    QSignalSpy proxyDataChangeSpy(&proxy, &QAbstractItemModel::dataChanged);
    QVERIFY(proxyDataChangeSpy.isValid());
    const QString testData = QStringLiteral("TestingSetData");
    if (viaProxy) {
        const QModelIndex parIdx = rootItem ? QModelIndex() : proxy.index(0, 1);
        QVERIFY(proxy.setData(proxy.index(0, 1, parIdx), testData));
        QCOMPARE(model->index(1, 0, proxy.mapToSource(parIdx)).data().toString(), testData);
    } else {
        const QModelIndex parIdx = rootItem ? QModelIndex() : model->index(1, 0);
        QVERIFY(model->setData(model->index(1, 0, parIdx), testData));
        QCOMPARE(proxy.index(0, 1, proxy.mapFromSource(parIdx)).data().toString(), testData);
    }
    QCOMPARE(sourceDataChangeSpy.size(), 1);
    QCOMPARE(proxyDataChangeSpy.size(), 1);
    delete model;
}

QTEST_GUILESS_MAIN(tst_QTransposeProxyModel)

#include "tst_qtransposeproxymodel.moc"
