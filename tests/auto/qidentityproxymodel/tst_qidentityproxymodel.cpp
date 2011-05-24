/****************************************************************************
**
** Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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


#include <QtTest/QtTest>
#include "../../shared/util.h"

#include <QtCore>
#include <QtGui>

#include "dynamictreemodel.h"
#include "qidentityproxymodel.h"

//TESTED CLASS=
//TESTED_FILES=

Q_DECLARE_METATYPE(QModelIndex)

class tst_QIdentityProxyModel : public QObject
{
    Q_OBJECT

public:

    tst_QIdentityProxyModel();
    virtual ~tst_QIdentityProxyModel();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void insertRows();
    void removeRows();
    void moveRows();
    void reset();

protected:
    void verifyIdentity(QAbstractItemModel *model, const QModelIndex &parent = QModelIndex());

private:
    QStandardItemModel *m_model;
    QIdentityProxyModel *m_proxy;
};

tst_QIdentityProxyModel::tst_QIdentityProxyModel()
    : m_model(0), m_proxy(0)
{

}

tst_QIdentityProxyModel::~tst_QIdentityProxyModel()
{

}

void tst_QIdentityProxyModel::initTestCase()
{
    qRegisterMetaType<QModelIndex>("QModelIndex");

    m_model = new QStandardItemModel(0, 1);
    m_proxy = new QIdentityProxyModel();
}

void tst_QIdentityProxyModel::cleanupTestCase()
{
    delete m_proxy;
    delete m_model;
}

void tst_QIdentityProxyModel::init()
{
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

    QVERIFY(m_proxy->mapToSource(proxyParent) == parent);
    QVERIFY(rows == m_proxy->rowCount(proxyParent));
    QVERIFY(columns == m_proxy->columnCount(proxyParent));

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const QModelIndex idx = model->index(row, column, parent);
            const QModelIndex proxyIdx = m_proxy->mapFromSource(idx);
            QVERIFY(proxyIdx.model() == m_proxy);
            QVERIFY(m_proxy->mapToSource(proxyIdx) == idx);
            QVERIFY(proxyIdx.isValid());
            QVERIFY(proxyIdx.row() == row);
            QVERIFY(proxyIdx.column() == column);
            QVERIFY(proxyIdx.parent() == proxyParent);
            QVERIFY(proxyIdx.data() == idx.data());
            QVERIFY(proxyIdx.flags() == idx.flags());
            const int childCount = m_proxy->rowCount(proxyIdx);
            const bool hasChildren = m_proxy->hasChildren(proxyIdx);
            QVERIFY(model->hasChildren(idx) == hasChildren);
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
        QStandardItem *item = new QStandardItem(QString("item %0").arg(i));
        parentItem->appendRow(item);
        parentItem = item;
    }

    m_proxy->setSourceModel(m_model);

    verifyIdentity(m_model);

    QSignalSpy modelBeforeSpy(m_model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy modelAfterSpy(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy proxyBeforeSpy(m_proxy, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy proxyAfterSpy(m_proxy, SIGNAL(rowsInserted(QModelIndex,int,int)));

    QStandardItem *item = new QStandardItem(QString("new item"));
    parentItem->appendRow(item);

    QVERIFY(modelBeforeSpy.size() == 1 && 1 == proxyBeforeSpy.size());
    QVERIFY(modelAfterSpy.size() == 1 && 1 == proxyAfterSpy.size());

    QVERIFY(modelBeforeSpy.first().first().value<QModelIndex>() == m_proxy->mapToSource(proxyBeforeSpy.first().first().value<QModelIndex>()));
    QVERIFY(modelBeforeSpy.first().at(1) == proxyBeforeSpy.first().at(1));
    QVERIFY(modelBeforeSpy.first().at(2) == proxyBeforeSpy.first().at(2));

    QVERIFY(modelAfterSpy.first().first().value<QModelIndex>() == m_proxy->mapToSource(proxyAfterSpy.first().first().value<QModelIndex>()));
    QVERIFY(modelAfterSpy.first().at(1) == proxyAfterSpy.first().at(1));
    QVERIFY(modelAfterSpy.first().at(2) == proxyAfterSpy.first().at(2));

    verifyIdentity(m_model);

}

void tst_QIdentityProxyModel::removeRows()
{
    QStandardItem *parentItem = m_model->invisibleRootItem();
    for (int i = 0; i < 4; ++i) {
        QStandardItem *item = new QStandardItem(QString("item %0").arg(i));
        parentItem->appendRow(item);
        parentItem = item;
    }

    m_proxy->setSourceModel(m_model);

    verifyIdentity(m_model);

    QSignalSpy modelBeforeSpy(m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy modelAfterSpy(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    QSignalSpy proxyBeforeSpy(m_proxy, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy proxyAfterSpy(m_proxy, SIGNAL(rowsRemoved(QModelIndex,int,int)));

    const QModelIndex topLevel = m_model->index(0, 0, QModelIndex());
    const QModelIndex secondLevel = m_model->index(0, 0, topLevel);
    const QModelIndex thirdLevel = m_model->index(0, 0, secondLevel);

    QVERIFY(thirdLevel.isValid());

    m_model->removeRow(0, secondLevel);

    QVERIFY(modelBeforeSpy.size() == 1 && 1 == proxyBeforeSpy.size());
    QVERIFY(modelAfterSpy.size() == 1 && 1 == proxyAfterSpy.size());

    QVERIFY(modelBeforeSpy.first().first().value<QModelIndex>() == m_proxy->mapToSource(proxyBeforeSpy.first().first().value<QModelIndex>()));
    QVERIFY(modelBeforeSpy.first().at(1) == proxyBeforeSpy.first().at(1));
    QVERIFY(modelBeforeSpy.first().at(2) == proxyBeforeSpy.first().at(2));

    QVERIFY(modelAfterSpy.first().first().value<QModelIndex>() == m_proxy->mapToSource(proxyAfterSpy.first().first().value<QModelIndex>()));
    QVERIFY(modelAfterSpy.first().at(1) == proxyAfterSpy.first().at(1));
    QVERIFY(modelAfterSpy.first().at(2) == proxyAfterSpy.first().at(2));

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

    QSignalSpy modelBeforeSpy(&model, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
    QSignalSpy modelAfterSpy(&model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)));
    QSignalSpy proxyBeforeSpy(m_proxy, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
    QSignalSpy proxyAfterSpy(m_proxy, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)));

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

    QVERIFY(modelBeforeSpy.first().first().value<QModelIndex>() == m_proxy->mapToSource(proxyBeforeSpy.first().first().value<QModelIndex>()));
    QVERIFY(modelBeforeSpy.first().at(1) == proxyBeforeSpy.first().at(1));
    QVERIFY(modelBeforeSpy.first().at(2) == proxyBeforeSpy.first().at(2));
    QVERIFY(modelBeforeSpy.first().at(3).value<QModelIndex>() == m_proxy->mapToSource(proxyBeforeSpy.first().at(3).value<QModelIndex>()));
    QVERIFY(modelBeforeSpy.first().at(4) == proxyBeforeSpy.first().at(4));

    QVERIFY(modelAfterSpy.first().first().value<QModelIndex>() == m_proxy->mapToSource(proxyAfterSpy.first().first().value<QModelIndex>()));
    QVERIFY(modelAfterSpy.first().at(1) == proxyAfterSpy.first().at(1));
    QVERIFY(modelAfterSpy.first().at(2) == proxyAfterSpy.first().at(2));
    QVERIFY(modelAfterSpy.first().at(3).value<QModelIndex>() == m_proxy->mapToSource(proxyAfterSpy.first().at(3).value<QModelIndex>()));
    QVERIFY(modelAfterSpy.first().at(4) == proxyAfterSpy.first().at(4));

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

    QSignalSpy modelBeforeSpy(&model, SIGNAL(modelAboutToBeReset()));
    QSignalSpy modelAfterSpy(&model, SIGNAL(modelReset()));
    QSignalSpy proxyBeforeSpy(m_proxy, SIGNAL(modelAboutToBeReset()));
    QSignalSpy proxyAfterSpy(m_proxy, SIGNAL(modelReset()));

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

QTEST_MAIN(tst_QIdentityProxyModel)
#include "tst_qidentityproxymodel.moc"
