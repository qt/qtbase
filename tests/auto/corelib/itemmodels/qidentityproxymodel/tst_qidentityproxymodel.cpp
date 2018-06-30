/****************************************************************************
**
** Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QAbstractItemModelTester>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTest>

#include "dynamictreemodel.h"
#include "qidentityproxymodel.h"

class DataChangedModel : public QAbstractListModel
{
public:
    int rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : 1; }
    QVariant data(const QModelIndex&, int) const { return QVariant(); }
    QModelIndex index(int row, int column, const QModelIndex &) const { return createIndex(row, column); }

    void changeData()
    {
        const QModelIndex idx = index(0, 0, QModelIndex());
        Q_EMIT dataChanged(idx, idx, QVector<int>() << 1);
    }
};

class tst_QIdentityProxyModel : public QObject
{
    Q_OBJECT

public:
    tst_QIdentityProxyModel();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

private slots:
    void insertRows();
    void removeRows();
    void moveRows();
    void reset();
    void dataChanged();

    void itemData();

    void persistIndexOnLayoutChange();

protected:
    void verifyIdentity(QAbstractItemModel *model, const QModelIndex &parent = QModelIndex());

private:
    QStandardItemModel *m_model;
    QIdentityProxyModel *m_proxy;
    QAbstractItemModelTester *m_modelTest;
};

tst_QIdentityProxyModel::tst_QIdentityProxyModel()
    : m_model(0), m_proxy(0)
{
}

void tst_QIdentityProxyModel::initTestCase()
{
    qRegisterMetaType<QVector<int> >();
    m_model = new QStandardItemModel(0, 1);
    m_proxy = new QIdentityProxyModel();
    m_modelTest = new QAbstractItemModelTester(m_proxy, this);
}

void tst_QIdentityProxyModel::cleanupTestCase()
{
    delete m_proxy;
    delete m_model;
    delete m_modelTest;
}

void tst_QIdentityProxyModel::cleanup()
{
    m_model->clear();
    m_model->insertColumns(0, 1);
}

void tst_QIdentityProxyModel::verifyIdentity(QAbstractItemModel *model, const QModelIndex &parent)
{
    const int rows = model->rowCount(parent);
    const int columns = model->columnCount(parent);
    const QModelIndex proxyParent = m_proxy->mapFromSource(parent);

    QCOMPARE(m_proxy->mapToSource(proxyParent), parent);
    QCOMPARE(rows, m_proxy->rowCount(proxyParent));
    QCOMPARE(columns, m_proxy->columnCount(proxyParent));

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const QModelIndex idx = model->index(row, column, parent);
            const QModelIndex proxyIdx = m_proxy->mapFromSource(idx);
            QCOMPARE(proxyIdx.model(), m_proxy);
            QCOMPARE(m_proxy->mapToSource(proxyIdx), idx);
            QVERIFY(proxyIdx.isValid());
            QCOMPARE(proxyIdx.row(), row);
            QCOMPARE(proxyIdx.column(), column);
            QCOMPARE(proxyIdx.parent(), proxyParent);
            QCOMPARE(proxyIdx.data(), idx.data());
            QCOMPARE(proxyIdx.flags(), idx.flags());
            const int childCount = m_proxy->rowCount(proxyIdx);
            const bool hasChildren = m_proxy->hasChildren(proxyIdx);
            QCOMPARE(model->hasChildren(idx), hasChildren);
            QVERIFY((childCount > 0) == hasChildren);

            if (hasChildren)
                verifyIdentity(model, idx);
        }
    }
}

/*
  tests
*/

void tst_QIdentityProxyModel::insertRows()
{
    QStandardItem *parentItem = m_model->invisibleRootItem();
    for (int i = 0; i < 4; ++i) {
        QStandardItem *item = new QStandardItem(QLatin1String("item ") + QString::number(i));
        parentItem->appendRow(item);
        parentItem = item;
    }

    m_proxy->setSourceModel(m_model);

    verifyIdentity(m_model);

    QSignalSpy modelBeforeSpy(m_model, &QStandardItemModel::rowsAboutToBeInserted);
    QSignalSpy modelAfterSpy(m_model, &QStandardItemModel::rowsInserted);
    QSignalSpy proxyBeforeSpy(m_proxy, &QStandardItemModel::rowsAboutToBeInserted);
    QSignalSpy proxyAfterSpy(m_proxy, &QStandardItemModel::rowsInserted);

    QVERIFY(modelBeforeSpy.isValid());
    QVERIFY(modelAfterSpy.isValid());
    QVERIFY(proxyBeforeSpy.isValid());
    QVERIFY(proxyAfterSpy.isValid());

    QStandardItem *item = new QStandardItem(QString("new item"));
    parentItem->appendRow(item);

    QVERIFY(modelBeforeSpy.size() == 1 && 1 == proxyBeforeSpy.size());
    QVERIFY(modelAfterSpy.size() == 1 && 1 == proxyAfterSpy.size());

    QCOMPARE(modelBeforeSpy.first().first().value<QModelIndex>(), m_proxy->mapToSource(proxyBeforeSpy.first().first().value<QModelIndex>()));
    QCOMPARE(modelBeforeSpy.first().at(1), proxyBeforeSpy.first().at(1));
    QCOMPARE(modelBeforeSpy.first().at(2), proxyBeforeSpy.first().at(2));

    QCOMPARE(modelAfterSpy.first().first().value<QModelIndex>(), m_proxy->mapToSource(proxyAfterSpy.first().first().value<QModelIndex>()));
    QCOMPARE(modelAfterSpy.first().at(1), proxyAfterSpy.first().at(1));
    QCOMPARE(modelAfterSpy.first().at(2), proxyAfterSpy.first().at(2));

    verifyIdentity(m_model);
}

void tst_QIdentityProxyModel::removeRows()
{
    QStandardItem *parentItem = m_model->invisibleRootItem();
    for (int i = 0; i < 4; ++i) {
        QStandardItem *item = new QStandardItem(QLatin1String("item ") + QString::number(i));
        parentItem->appendRow(item);
        parentItem = item;
    }

    m_proxy->setSourceModel(m_model);

    verifyIdentity(m_model);

    QSignalSpy modelBeforeSpy(m_model, &QStandardItemModel::rowsAboutToBeRemoved);
    QSignalSpy modelAfterSpy(m_model, &QStandardItemModel::rowsRemoved);
    QSignalSpy proxyBeforeSpy(m_proxy, &QStandardItemModel::rowsAboutToBeRemoved);
    QSignalSpy proxyAfterSpy(m_proxy, &QStandardItemModel::rowsRemoved);

    QVERIFY(modelBeforeSpy.isValid());
    QVERIFY(modelAfterSpy.isValid());
    QVERIFY(proxyBeforeSpy.isValid());
    QVERIFY(proxyAfterSpy.isValid());

    const QModelIndex topLevel = m_model->index(0, 0, QModelIndex());
    const QModelIndex secondLevel = m_model->index(0, 0, topLevel);
    const QModelIndex thirdLevel = m_model->index(0, 0, secondLevel);

    QVERIFY(thirdLevel.isValid());

    m_model->removeRow(0, secondLevel);

    QVERIFY(modelBeforeSpy.size() == 1 && 1 == proxyBeforeSpy.size());
    QVERIFY(modelAfterSpy.size() == 1 && 1 == proxyAfterSpy.size());

    QCOMPARE(modelBeforeSpy.first().first().value<QModelIndex>(), m_proxy->mapToSource(proxyBeforeSpy.first().first().value<QModelIndex>()));
    QCOMPARE(modelBeforeSpy.first().at(1), proxyBeforeSpy.first().at(1));
    QCOMPARE(modelBeforeSpy.first().at(2), proxyBeforeSpy.first().at(2));

    QCOMPARE(modelAfterSpy.first().first().value<QModelIndex>(), m_proxy->mapToSource(proxyAfterSpy.first().first().value<QModelIndex>()));
    QCOMPARE(modelAfterSpy.first().at(1), proxyAfterSpy.first().at(1));
    QCOMPARE(modelAfterSpy.first().at(2), proxyAfterSpy.first().at(2));

    verifyIdentity(m_model);
}

void tst_QIdentityProxyModel::moveRows()
{
    DynamicTreeModel model;

    {
        ModelInsertCommand insertCommand(&model);
        insertCommand.setStartRow(0);
        insertCommand.setEndRow(9);
        insertCommand.doCommand();
    }
    {
        ModelInsertCommand insertCommand(&model);
        insertCommand.setAncestorRowNumbers(QList<int>() << 5);
        insertCommand.setStartRow(0);
        insertCommand.setEndRow(9);
        insertCommand.doCommand();
    }

    m_proxy->setSourceModel(&model);

    verifyIdentity(&model);

    QSignalSpy modelBeforeSpy(&model, &DynamicTreeModel::rowsAboutToBeMoved);
    QSignalSpy modelAfterSpy(&model, &DynamicTreeModel::rowsMoved);
    QSignalSpy proxyBeforeSpy(m_proxy, &QIdentityProxyModel::rowsAboutToBeMoved);
    QSignalSpy proxyAfterSpy(m_proxy, &QIdentityProxyModel::rowsMoved);

    QVERIFY(modelBeforeSpy.isValid());
    QVERIFY(modelAfterSpy.isValid());
    QVERIFY(proxyBeforeSpy.isValid());
    QVERIFY(proxyAfterSpy.isValid());

    {
        ModelMoveCommand moveCommand(&model, 0);
        moveCommand.setAncestorRowNumbers(QList<int>() << 5);
        moveCommand.setStartRow(3);
        moveCommand.setEndRow(4);
        moveCommand.setDestRow(1);
        moveCommand.doCommand();
    }

    QVERIFY(modelBeforeSpy.size() == 1 && 1 == proxyBeforeSpy.size());
    QVERIFY(modelAfterSpy.size() == 1 && 1 == proxyAfterSpy.size());

    QCOMPARE(modelBeforeSpy.first().first().value<QModelIndex>(), m_proxy->mapToSource(proxyBeforeSpy.first().first().value<QModelIndex>()));
    QCOMPARE(modelBeforeSpy.first().at(1), proxyBeforeSpy.first().at(1));
    QCOMPARE(modelBeforeSpy.first().at(2), proxyBeforeSpy.first().at(2));
    QCOMPARE(modelBeforeSpy.first().at(3).value<QModelIndex>(), m_proxy->mapToSource(proxyBeforeSpy.first().at(3).value<QModelIndex>()));
    QCOMPARE(modelBeforeSpy.first().at(4), proxyBeforeSpy.first().at(4));

    QCOMPARE(modelAfterSpy.first().first().value<QModelIndex>(), m_proxy->mapToSource(proxyAfterSpy.first().first().value<QModelIndex>()));
    QCOMPARE(modelAfterSpy.first().at(1), proxyAfterSpy.first().at(1));
    QCOMPARE(modelAfterSpy.first().at(2), proxyAfterSpy.first().at(2));
    QCOMPARE(modelAfterSpy.first().at(3).value<QModelIndex>(), m_proxy->mapToSource(proxyAfterSpy.first().at(3).value<QModelIndex>()));
    QCOMPARE(modelAfterSpy.first().at(4), proxyAfterSpy.first().at(4));

    verifyIdentity(&model);

    m_proxy->setSourceModel(0);
}

void tst_QIdentityProxyModel::reset()
{
    DynamicTreeModel model;

    {
        ModelInsertCommand insertCommand(&model);
        insertCommand.setStartRow(0);
        insertCommand.setEndRow(9);
        insertCommand.doCommand();
    }
    {
        ModelInsertCommand insertCommand(&model);
        insertCommand.setAncestorRowNumbers(QList<int>() << 5);
        insertCommand.setStartRow(0);
        insertCommand.setEndRow(9);
        insertCommand.doCommand();
    }

    m_proxy->setSourceModel(&model);

    verifyIdentity(&model);

    QSignalSpy modelBeforeSpy(&model, &DynamicTreeModel::modelAboutToBeReset);
    QSignalSpy modelAfterSpy(&model, &DynamicTreeModel::modelReset);
    QSignalSpy proxyBeforeSpy(m_proxy, &QIdentityProxyModel::modelAboutToBeReset);
    QSignalSpy proxyAfterSpy(m_proxy, &QIdentityProxyModel::modelReset);

    QVERIFY(modelBeforeSpy.isValid());
    QVERIFY(modelAfterSpy.isValid());
    QVERIFY(proxyBeforeSpy.isValid());
    QVERIFY(proxyAfterSpy.isValid());

    {
        ModelResetCommandFixed resetCommand(&model, 0);
        resetCommand.setAncestorRowNumbers(QList<int>() << 5);
        resetCommand.setStartRow(3);
        resetCommand.setEndRow(4);
        resetCommand.setDestRow(1);
        resetCommand.doCommand();
    }

    QVERIFY(modelBeforeSpy.size() == 1 && 1 == proxyBeforeSpy.size());
    QVERIFY(modelAfterSpy.size() == 1 && 1 == proxyAfterSpy.size());

    verifyIdentity(&model);
    m_proxy->setSourceModel(0);
}

void tst_QIdentityProxyModel::dataChanged()
{
    DataChangedModel model;
    m_proxy->setSourceModel(&model);

    verifyIdentity(&model);

    QSignalSpy modelSpy(&model, &DataChangedModel::dataChanged);
    QSignalSpy proxySpy(m_proxy, &QIdentityProxyModel::dataChanged);

    QVERIFY(modelSpy.isValid());
    QVERIFY(proxySpy.isValid());

    model.changeData();

    QCOMPARE(modelSpy.first().at(2).value<QVector<int> >(), QVector<int>() << 1);
    QCOMPARE(modelSpy.first().at(2), proxySpy.first().at(2));

    verifyIdentity(&model);
    m_proxy->setSourceModel(0);
}

class AppendStringProxy : public QIdentityProxyModel
{
public:
    QVariant data(const QModelIndex &index, int role) const
    {
        const QVariant result = QIdentityProxyModel::data(index, role);
        if (role != Qt::DisplayRole)
            return result;
        return result.toString() + "_appended";
    }
};

void tst_QIdentityProxyModel::itemData()
{
    QStringListModel model(QStringList() << "Monday" << "Tuesday" << "Wednesday");
    AppendStringProxy proxy;
    proxy.setSourceModel(&model);

    const QModelIndex topIndex = proxy.index(0, 0);
    QCOMPARE(topIndex.data(Qt::DisplayRole).toString(), QStringLiteral("Monday_appended"));
    QCOMPARE(proxy.data(topIndex, Qt::DisplayRole).toString(), QStringLiteral("Monday_appended"));
    QCOMPARE(proxy.itemData(topIndex).value(Qt::DisplayRole).toString(), QStringLiteral("Monday_appended"));
}

void dump(QAbstractItemModel* model, QString const& indent = " - ", QModelIndex const& parent = {})
{
    for (auto row = 0; row < model->rowCount(parent); ++row)
    {
        auto idx = model->index(row, 0, parent);
        qDebug() << (indent + idx.data().toString());
        dump(model, indent + "- ", idx);
    }
}

void tst_QIdentityProxyModel::persistIndexOnLayoutChange()
{
    DynamicTreeModel model;

    QList<int> ancestors;
    for (auto i = 0; i < 3; ++i)
    {
        Q_UNUSED(i);
        ModelInsertCommand insertCommand(&model);
        insertCommand.setAncestorRowNumbers(ancestors);
        insertCommand.setStartRow(0);
        insertCommand.setEndRow(0);
        insertCommand.doCommand();
        ancestors.push_back(0);
    }
    ModelInsertCommand insertCommand(&model);
    insertCommand.setAncestorRowNumbers(ancestors);
    insertCommand.setStartRow(0);
    insertCommand.setEndRow(1);
    insertCommand.doCommand();

    // dump(&model);
    // " - 1"
    // " - - 2"
    // " - - - 3"
    // " - - - - 4"
    // " - - - - 5"

    QIdentityProxyModel proxy;
    proxy.setSourceModel(&model);

    QPersistentModelIndex persistentIndex;

    QPersistentModelIndex sourcePersistentIndex = model.match(model.index(0, 0), Qt::DisplayRole, "5", 1, Qt::MatchRecursive).first();

    QCOMPARE(sourcePersistentIndex.data().toString(), QStringLiteral("5"));

    bool gotLayoutAboutToBeChanged = false;
    bool gotLayoutChanged = false;

    QObject::connect(&proxy, &QAbstractItemModel::layoutAboutToBeChanged, &proxy, [&proxy, &persistentIndex, &gotLayoutAboutToBeChanged]
    {
        gotLayoutAboutToBeChanged = true;
        persistentIndex = proxy.match(proxy.index(0, 0), Qt::DisplayRole, "5", 1, Qt::MatchRecursive).first();
    });

    QObject::connect(&proxy, &QAbstractItemModel::layoutChanged, &proxy, [&proxy, &persistentIndex, &sourcePersistentIndex, &gotLayoutChanged]
    {
        gotLayoutChanged = true;
        QCOMPARE(QModelIndex(persistentIndex), proxy.mapFromSource(sourcePersistentIndex));
    });

    ModelChangeChildrenLayoutsCommand layoutChangeCommand(&model, 0);

    layoutChangeCommand.setAncestorRowNumbers(QList<int>{0, 0, 0});
    layoutChangeCommand.setSecondAncestorRowNumbers(QList<int>{0, 0});

    layoutChangeCommand.doCommand();

    QVERIFY(gotLayoutAboutToBeChanged);
    QVERIFY(gotLayoutChanged);
    QVERIFY(persistentIndex.isValid());
}

QTEST_MAIN(tst_QIdentityProxyModel)
#include "tst_qidentityproxymodel.moc"
