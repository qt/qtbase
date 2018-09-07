/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
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

#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QTest>
#include <QStandardItemModel>
#include <QIdentityProxyModel>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QStringListModel>
#include <QAbstractItemModelTester>

#include <qconcatenatetablesproxymodel.h>

Q_DECLARE_METATYPE(QModelIndex)

// Extracts a full row from a model as a string
// Works best if every cell contains only one character
static QString extractRowTexts(QAbstractItemModel *model, int row, const QModelIndex &parent = QModelIndex())
{
    QString result;
    const int colCount = model->columnCount();
    for (int col = 0; col < colCount; ++col) {
        const QString txt = model->index(row, col, parent).data().toString();
        result += txt.isEmpty() ? QStringLiteral(" ") : txt;
    }
    return result;
}

// Extracts a full column from a model as a string
// Works best if every cell contains only one character
static QString extractColumnTexts(QAbstractItemModel *model, int column, const QModelIndex &parent = QModelIndex())
{
    QString result;
    const int rowCount = model->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        const QString txt = model->index(row, column, parent).data().toString();
        result += txt.isEmpty() ? QStringLiteral(" ") : txt;
    }
    return result;
}

static QString rowSpyToText(const QSignalSpy &spy)
{
    if (!spy.isValid())
        return QStringLiteral("THE SIGNALSPY IS INVALID!");
    QString str;
    for (int i = 0; i < spy.count(); ++i) {
        str += spy.at(i).at(1).toString() + QLatin1Char(',') + spy.at(i).at(2).toString();
        if (i + 1 < spy.count())
            str += QLatin1Char(';');
    }
    return str;
}

class tst_QConcatenateTablesProxyModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void shouldAggregateTwoModelsCorrectly();
    void shouldAggregateThenRemoveTwoEmptyModelsCorrectly();
    void shouldAggregateTwoEmptyModelsWhichThenGetFilled();
    void shouldHandleDataChanged();
    void shouldHandleSetData();
    void shouldHandleSetItemData();
    void shouldHandleRowInsertionAndRemoval();
    void shouldAggregateAnotherModelThenRemoveModels();
    void shouldUseSmallestColumnCount();
    void shouldIncreaseColumnCountWhenRemovingFirstModel();
    void shouldHandleColumnInsertionAndRemoval();
    void shouldPropagateLayoutChanged();
    void shouldReactToModelReset();
    void shouldUpdateColumnsOnModelReset();
    void shouldPropagateDropOnItem_data();
    void shouldPropagateDropOnItem();
    void shouldPropagateDropBetweenItems();
    void shouldPropagateDropBetweenItemsAtModelBoundary();
    void shouldPropagateDropAfterLastRow_data();
    void shouldPropagateDropAfterLastRow();

private:
    QStandardItemModel mod;
    QStandardItemModel mod2;
    QStandardItemModel mod3;
};

void tst_QConcatenateTablesProxyModel::init()
{
    // Prepare some source models to use later on
    mod.clear();
    mod.appendRow({ new QStandardItem(QStringLiteral("A")), new QStandardItem(QStringLiteral("B")), new QStandardItem(QStringLiteral("C")) });
    mod.setHorizontalHeaderLabels(QStringList() << QStringLiteral("H1") << QStringLiteral("H2") << QStringLiteral("H3"));
    mod.setVerticalHeaderLabels(QStringList() << QStringLiteral("One"));

    mod2.clear();
    mod2.appendRow({ new QStandardItem(QStringLiteral("D")), new QStandardItem(QStringLiteral("E")), new QStandardItem(QStringLiteral("F")) });
    mod2.setHorizontalHeaderLabels(QStringList() << QStringLiteral("H1") << QStringLiteral("H2") << QStringLiteral("H3"));
    mod2.setVerticalHeaderLabels(QStringList() << QStringLiteral("Two"));

    mod3.clear();
    mod3.appendRow({ new QStandardItem(QStringLiteral("1")), new QStandardItem(QStringLiteral("2")), new QStandardItem(QStringLiteral("3")) });
    mod3.appendRow({ new QStandardItem(QStringLiteral("4")), new QStandardItem(QStringLiteral("5")), new QStandardItem(QStringLiteral("6")) });
}

void tst_QConcatenateTablesProxyModel::shouldAggregateTwoModelsCorrectly()
{
    // Given a combining proxy
    QConcatenateTablesProxyModel pm;

    // When adding two source models
    pm.addSourceModel(&mod);
    pm.addSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);

    // Then the proxy should show 2 rows
    QCOMPARE(pm.rowCount(), 2);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEF"));

    // ... and correct headers
    QCOMPARE(pm.headerData(0, Qt::Horizontal).toString(), QStringLiteral("H1"));
    QCOMPARE(pm.headerData(1, Qt::Horizontal).toString(), QStringLiteral("H2"));
    QCOMPARE(pm.headerData(2, Qt::Horizontal).toString(), QStringLiteral("H3"));
    QCOMPARE(pm.headerData(0, Qt::Vertical).toString(), QStringLiteral("One"));
    QCOMPARE(pm.headerData(1, Qt::Vertical).toString(), QStringLiteral("Two"));

    QVERIFY(!pm.canFetchMore(QModelIndex()));
}

void tst_QConcatenateTablesProxyModel::shouldAggregateThenRemoveTwoEmptyModelsCorrectly()
{
    // Given a combining proxy
    QConcatenateTablesProxyModel pm;

    // When adding two empty models
    QSignalSpy rowATBISpy(&pm, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy rowInsertedSpy(&pm, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy rowATBRSpy(&pm, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy rowRemovedSpy(&pm, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    QIdentityProxyModel i1, i2;
    pm.addSourceModel(&i1);
    pm.addSourceModel(&i2);

    // Then the proxy should still be empty (and no signals emitted)
    QCOMPARE(pm.rowCount(), 0);
    QCOMPARE(pm.columnCount(), 0);
    QCOMPARE(rowATBISpy.count(), 0);
    QCOMPARE(rowInsertedSpy.count(), 0);

    // When removing the empty models
    pm.removeSourceModel(&i1);
    pm.removeSourceModel(&i2);

    // Then the proxy should still be empty (and no signals emitted)
    QCOMPARE(pm.rowCount(), 0);
    QCOMPARE(pm.columnCount(), 0);
    QCOMPARE(rowATBRSpy.count(), 0);
    QCOMPARE(rowRemovedSpy.count(), 0);
}

void tst_QConcatenateTablesProxyModel::shouldAggregateTwoEmptyModelsWhichThenGetFilled()
{
    // Given a combining proxy with two empty models
    QConcatenateTablesProxyModel pm;
    QIdentityProxyModel i1, i2;
    pm.addSourceModel(&i1);
    pm.addSourceModel(&i2);

    // When filling them afterwards
    i1.setSourceModel(&mod);
    i2.setSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);

    // Then the proxy should show 2 rows
    QCOMPARE(pm.rowCount(), 2);
    QCOMPARE(pm.columnCount(), 3);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEF"));

    // ... and correct headers
    QCOMPARE(pm.headerData(0, Qt::Horizontal).toString(), QStringLiteral("H1"));
    QCOMPARE(pm.headerData(1, Qt::Horizontal).toString(), QStringLiteral("H2"));
    QCOMPARE(pm.headerData(2, Qt::Horizontal).toString(), QStringLiteral("H3"));
    QCOMPARE(pm.headerData(0, Qt::Vertical).toString(), QStringLiteral("One"));
    QCOMPARE(pm.headerData(1, Qt::Vertical).toString(), QStringLiteral("Two"));

    QVERIFY(!pm.canFetchMore(QModelIndex()));
}

void tst_QConcatenateTablesProxyModel::shouldHandleDataChanged()
{
    // Given two models combined
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    pm.addSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);
    QSignalSpy dataChangedSpy(&pm, SIGNAL(dataChanged(QModelIndex,QModelIndex)));

    // When a cell in a source model changes
    mod.item(0, 0)->setData("a", Qt::EditRole);

    // Then the change should be notified to the proxy
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.at(0).at(0).toModelIndex(), pm.index(0, 0));
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("aBC"));

    // Same test with the other model
    mod2.item(0, 2)->setData("f", Qt::EditRole);

    QCOMPARE(dataChangedSpy.count(), 2);
    QCOMPARE(dataChangedSpy.at(1).at(0).toModelIndex(), pm.index(1, 2));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEf"));
}

void tst_QConcatenateTablesProxyModel::shouldHandleSetData()
{
    // Given two models combined
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    pm.addSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);
    QSignalSpy dataChangedSpy(&pm, SIGNAL(dataChanged(QModelIndex,QModelIndex)));

    // When changing a cell using setData
    pm.setData(pm.index(0, 0), "a");

    // Then the change should be notified to the proxy
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.at(0).at(0).toModelIndex(), pm.index(0, 0));
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("aBC"));

    // Same test with the other model
    pm.setData(pm.index(1, 2), "f");

    QCOMPARE(dataChangedSpy.count(), 2);
    QCOMPARE(dataChangedSpy.at(1).at(0).toModelIndex(), pm.index(1, 2));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEf"));
}

void tst_QConcatenateTablesProxyModel::shouldHandleSetItemData()
{
    // Given two models combined
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    pm.addSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);
    QSignalSpy dataChangedSpy(&pm, SIGNAL(dataChanged(QModelIndex,QModelIndex)));

    // When changing a cell using setData
    pm.setItemData(pm.index(0, 0), QMap<int, QVariant>{ std::make_pair<int, QVariant>(Qt::DisplayRole, QStringLiteral("X")),
                                                        std::make_pair<int, QVariant>(Qt::UserRole, 88) });

    // Then the change should be notified to the proxy
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.at(0).at(0).toModelIndex(), pm.index(0, 0));
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("XBC"));
    QCOMPARE(pm.index(0, 0).data(Qt::UserRole).toInt(), 88);

    // Same test with the other model
    pm.setItemData(pm.index(1, 2), QMap<int, QVariant>{ std::make_pair<int, QVariant>(Qt::DisplayRole, QStringLiteral("Y")),
                                                        std::make_pair<int, QVariant>(Qt::UserRole, 89) });

    QCOMPARE(dataChangedSpy.count(), 2);
    QCOMPARE(dataChangedSpy.at(1).at(0).toModelIndex(), pm.index(1, 2));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEY"));
    QCOMPARE(pm.index(1, 2).data(Qt::UserRole).toInt(), 89);
}

void tst_QConcatenateTablesProxyModel::shouldHandleRowInsertionAndRemoval()
{
    // Given two models combined
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    pm.addSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);
    QSignalSpy rowATBISpy(&pm, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy rowInsertedSpy(&pm, SIGNAL(rowsInserted(QModelIndex,int,int)));

    // When a source model inserts a new row
    QList<QStandardItem *> row;
    row.append(new QStandardItem(QStringLiteral("1")));
    row.append(new QStandardItem(QStringLiteral("2")));
    row.append(new QStandardItem(QStringLiteral("3")));
    mod2.insertRow(0, row);

    // Then the proxy should notify its users and show changes
    QCOMPARE(rowSpyToText(rowATBISpy), QStringLiteral("1,1"));
    QCOMPARE(rowSpyToText(rowInsertedSpy), QStringLiteral("1,1"));
    QCOMPARE(pm.rowCount(), 3);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("DEF"));

    // When removing that row
    QSignalSpy rowATBRSpy(&pm, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy rowRemovedSpy(&pm, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    mod2.removeRow(0);

    // Then the proxy should notify its users and show changes
    QCOMPARE(rowATBRSpy.count(), 1);
    QCOMPARE(rowATBRSpy.at(0).at(1).toInt(), 1);
    QCOMPARE(rowATBRSpy.at(0).at(2).toInt(), 1);
    QCOMPARE(rowRemovedSpy.count(), 1);
    QCOMPARE(rowRemovedSpy.at(0).at(1).toInt(), 1);
    QCOMPARE(rowRemovedSpy.at(0).at(2).toInt(), 1);
    QCOMPARE(pm.rowCount(), 2);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEF"));

    // When removing the last row from mod2
    rowATBRSpy.clear();
    rowRemovedSpy.clear();
    mod2.removeRow(0);

    // Then the proxy should notify its users and show changes
    QCOMPARE(rowATBRSpy.count(), 1);
    QCOMPARE(rowATBRSpy.at(0).at(1).toInt(), 1);
    QCOMPARE(rowATBRSpy.at(0).at(2).toInt(), 1);
    QCOMPARE(rowRemovedSpy.count(), 1);
    QCOMPARE(rowRemovedSpy.at(0).at(1).toInt(), 1);
    QCOMPARE(rowRemovedSpy.at(0).at(2).toInt(), 1);
    QCOMPARE(pm.rowCount(), 1);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
}

void tst_QConcatenateTablesProxyModel::shouldAggregateAnotherModelThenRemoveModels()
{
    // Given two models combined, and a third model
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    pm.addSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);

    QSignalSpy rowATBISpy(&pm, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy rowInsertedSpy(&pm, SIGNAL(rowsInserted(QModelIndex,int,int)));

    // When adding the new source model
    pm.addSourceModel(&mod3);

    // Then the proxy should notify its users about the two rows inserted
    QCOMPARE(rowSpyToText(rowATBISpy), QStringLiteral("2,3"));
    QCOMPARE(rowSpyToText(rowInsertedSpy), QStringLiteral("2,3"));
    QCOMPARE(pm.rowCount(), 4);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEF"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 3), QStringLiteral("456"));

    // When removing that source model again
    QSignalSpy rowATBRSpy(&pm, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy rowRemovedSpy(&pm, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    pm.removeSourceModel(&mod3);

    // Then the proxy should notify its users about the row removed
    QCOMPARE(rowATBRSpy.count(), 1);
    QCOMPARE(rowATBRSpy.at(0).at(1).toInt(), 2);
    QCOMPARE(rowATBRSpy.at(0).at(2).toInt(), 3);
    QCOMPARE(rowRemovedSpy.count(), 1);
    QCOMPARE(rowRemovedSpy.at(0).at(1).toInt(), 2);
    QCOMPARE(rowRemovedSpy.at(0).at(2).toInt(), 3);
    QCOMPARE(pm.rowCount(), 2);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEF"));

    // When removing model 2
    rowATBRSpy.clear();
    rowRemovedSpy.clear();
    pm.removeSourceModel(&mod2);
    QCOMPARE(rowATBRSpy.count(), 1);
    QCOMPARE(rowATBRSpy.at(0).at(1).toInt(), 1);
    QCOMPARE(rowATBRSpy.at(0).at(2).toInt(), 1);
    QCOMPARE(rowRemovedSpy.count(), 1);
    QCOMPARE(rowRemovedSpy.at(0).at(1).toInt(), 1);
    QCOMPARE(rowRemovedSpy.at(0).at(2).toInt(), 1);
    QCOMPARE(pm.rowCount(), 1);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));

    // When removing model 1
    rowATBRSpy.clear();
    rowRemovedSpy.clear();
    pm.removeSourceModel(&mod);
    QCOMPARE(rowATBRSpy.count(), 1);
    QCOMPARE(rowATBRSpy.at(0).at(1).toInt(), 0);
    QCOMPARE(rowATBRSpy.at(0).at(2).toInt(), 0);
    QCOMPARE(rowRemovedSpy.count(), 1);
    QCOMPARE(rowRemovedSpy.at(0).at(1).toInt(), 0);
    QCOMPARE(rowRemovedSpy.at(0).at(2).toInt(), 0);
    QCOMPARE(pm.rowCount(), 0);
}

void tst_QConcatenateTablesProxyModel::shouldUseSmallestColumnCount()
{
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    pm.addSourceModel(&mod2);
    mod2.setColumnCount(1);
    pm.addSourceModel(&mod3);
    QAbstractItemModelTester modelTest(&pm, this);

    QCOMPARE(pm.rowCount(), 4);
    QCOMPARE(pm.columnCount(), 1);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("A"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("D"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("1"));
    QCOMPARE(extractRowTexts(&pm, 3), QStringLiteral("4"));

    const QModelIndex indexA = pm.mapFromSource(mod.index(0, 0));
    QVERIFY(indexA.isValid());
    QCOMPARE(indexA, pm.index(0, 0));

    const QModelIndex indexB = pm.mapFromSource(mod.index(0, 1));
    QVERIFY(!indexB.isValid());

    const QModelIndex indexD = pm.mapFromSource(mod2.index(0, 0));
    QVERIFY(indexD.isValid());
    QCOMPARE(indexD, pm.index(1, 0));
}

void tst_QConcatenateTablesProxyModel::shouldIncreaseColumnCountWhenRemovingFirstModel()
{
    // Given a model with 2 columns and one with 3 columns
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    QAbstractItemModelTester modelTest(&pm, this);
    mod.setColumnCount(2);
    pm.addSourceModel(&mod2);
    QCOMPARE(pm.rowCount(), 2);
    QCOMPARE(pm.columnCount(), 2);

    QSignalSpy colATBISpy(&pm, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy colInsertedSpy(&pm, SIGNAL(columnsInserted(QModelIndex,int,int)));
    QSignalSpy rowATBRSpy(&pm, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy rowRemovedSpy(&pm, SIGNAL(rowsRemoved(QModelIndex,int,int)));

    // When removing the first source model
    pm.removeSourceModel(&mod);

    // Then the proxy should notify its users about the row removed, and the column added
    QCOMPARE(pm.rowCount(), 1);
    QCOMPARE(pm.columnCount(), 3);
    QCOMPARE(rowSpyToText(rowATBRSpy), QStringLiteral("0,0"));
    QCOMPARE(rowSpyToText(rowRemovedSpy), QStringLiteral("0,0"));
    QCOMPARE(rowSpyToText(colATBISpy), QStringLiteral("2,2"));
    QCOMPARE(rowSpyToText(colInsertedSpy), QStringLiteral("2,2"));
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("DEF"));
}

void tst_QConcatenateTablesProxyModel::shouldHandleColumnInsertionAndRemoval()
{
    // Given two models combined, one with 2 columns and one with 3
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    QAbstractItemModelTester modelTest(&pm, this);
    mod.setColumnCount(2);
    pm.addSourceModel(&mod2);
    QSignalSpy colATBISpy(&pm, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy colInsertedSpy(&pm, SIGNAL(columnsInserted(QModelIndex,int,int)));
    QSignalSpy colATBRSpy(&pm, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy colRemovedSpy(&pm, SIGNAL(columnsRemoved(QModelIndex,int,int)));

    // When the first source model inserts a new column
    QCOMPARE(mod.columnCount(), 2);
    mod.setColumnCount(3);

    // Then the proxy should notify its users and show changes
    QCOMPARE(rowSpyToText(colATBISpy), QStringLiteral("2,2"));
    QCOMPARE(rowSpyToText(colInsertedSpy), QStringLiteral("2,2"));
    QCOMPARE(pm.rowCount(), 2);
    QCOMPARE(pm.columnCount(), 3);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("AB "));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEF"));

    // And when removing two columns
    mod.setColumnCount(1);

    // Then the proxy should notify its users and show changes
    QCOMPARE(rowSpyToText(colATBRSpy), QStringLiteral("1,2"));
    QCOMPARE(rowSpyToText(colRemovedSpy), QStringLiteral("1,2"));
    QCOMPARE(pm.rowCount(), 2);
    QCOMPARE(pm.columnCount(), 1);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("A"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("D"));
}

void tst_QConcatenateTablesProxyModel::shouldPropagateLayoutChanged()
{
    // Given two source models, the second one being a QSFPM
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    QAbstractItemModelTester modelTest(&pm, this);

    QSortFilterProxyModel qsfpm;
    qsfpm.setSourceModel(&mod3);
    pm.addSourceModel(&qsfpm);

    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("456"));

    // And a selection (row 1)
    QItemSelectionModel selection(&pm);
    selection.select(pm.index(1, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    const QModelIndexList lst = selection.selectedIndexes();
    QCOMPARE(lst.count(), 3);
    for (int col = 0; col < lst.count(); ++col) {
        QCOMPARE(lst.at(col).row(), 1);
        QCOMPARE(lst.at(col).column(), col);
    }

    QSignalSpy layoutATBCSpy(&pm, SIGNAL(layoutAboutToBeChanged()));
    QSignalSpy layoutChangedSpy(&pm, SIGNAL(layoutChanged()));

    // When changing the sorting in the QSFPM
    qsfpm.sort(0, Qt::DescendingOrder);

    // Then the proxy should emit the layoutChanged signals, and show re-sorted data
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("456"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("123"));
    QCOMPARE(layoutATBCSpy.count(), 1);
    QCOMPARE(layoutChangedSpy.count(), 1);

    // And the selection should be updated accordingly (it became row 2)
    const QModelIndexList lstAfter = selection.selectedIndexes();
    QCOMPARE(lstAfter.count(), 3);
    for (int col = 0; col < lstAfter.count(); ++col) {
        QCOMPARE(lstAfter.at(col).row(), 2);
        QCOMPARE(lstAfter.at(col).column(), col);
    }
}

void tst_QConcatenateTablesProxyModel::shouldReactToModelReset()
{
    // Given two source models, the second one being a QSFPM
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod);
    QAbstractItemModelTester modelTest(&pm, this);

    QSortFilterProxyModel qsfpm;
    qsfpm.setSourceModel(&mod3);
    pm.addSourceModel(&qsfpm);

    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("456"));
    QSignalSpy rowATBRSpy(&pm, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy rowRemovedSpy(&pm, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    QSignalSpy rowATBISpy(&pm, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy rowInsertedSpy(&pm, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy colATBRSpy(&pm, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy colRemovedSpy(&pm, SIGNAL(columnsRemoved(QModelIndex,int,int)));
    QSignalSpy modelATBResetSpy(&pm, SIGNAL(modelAboutToBeReset()));
    QSignalSpy modelResetSpy(&pm, SIGNAL(modelReset()));

    // When changing the source model of the QSFPM
    qsfpm.setSourceModel(&mod2);

    // Then the proxy should emit the reset signals, and show the new data
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("ABC"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEF"));
    QCOMPARE(rowATBRSpy.count(), 0);
    QCOMPARE(rowRemovedSpy.count(), 0);
    QCOMPARE(rowATBISpy.count(), 0);
    QCOMPARE(rowInsertedSpy.count(), 0);
    QCOMPARE(colATBRSpy.count(), 0);
    QCOMPARE(colRemovedSpy.count(), 0);
    QCOMPARE(modelATBResetSpy.count(), 1);
    QCOMPARE(modelResetSpy.count(), 1);
}

void tst_QConcatenateTablesProxyModel::shouldUpdateColumnsOnModelReset()
{
    // Given two source models, the first one being a QSFPM
    QConcatenateTablesProxyModel pm;

    QSortFilterProxyModel qsfpm;
    qsfpm.setSourceModel(&mod3);
    pm.addSourceModel(&qsfpm);
    pm.addSourceModel(&mod);
    QAbstractItemModelTester modelTest(&pm, this);

    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("456"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("ABC"));

    // ... and a model with only 2 columns
    QStandardItemModel mod2Columns;
    mod2Columns.appendRow({ new QStandardItem(QStringLiteral("W")), new QStandardItem(QStringLiteral("X")) });

    QSignalSpy rowATBRSpy(&pm, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy rowRemovedSpy(&pm, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    QSignalSpy rowATBISpy(&pm, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy rowInsertedSpy(&pm, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy colATBRSpy(&pm, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy colRemovedSpy(&pm, SIGNAL(columnsRemoved(QModelIndex,int,int)));
    QSignalSpy modelATBResetSpy(&pm, SIGNAL(modelAboutToBeReset()));
    QSignalSpy modelResetSpy(&pm, SIGNAL(modelReset()));

    // When changing the source model of the QSFPM
    qsfpm.setSourceModel(&mod2Columns);

    // Then the proxy should reset, and show the new data
    QCOMPARE(modelATBResetSpy.count(), 1);
    QCOMPARE(modelResetSpy.count(), 1);
    QCOMPARE(rowATBRSpy.count(), 0);
    QCOMPARE(rowRemovedSpy.count(), 0);
    QCOMPARE(rowATBISpy.count(), 0);
    QCOMPARE(rowInsertedSpy.count(), 0);
    QCOMPARE(colATBRSpy.count(), 0);
    QCOMPARE(colRemovedSpy.count(), 0);

    QCOMPARE(pm.rowCount(), 2);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("WX"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("AB"));
}

void tst_QConcatenateTablesProxyModel::shouldPropagateDropOnItem_data()
{
    QTest::addColumn<int>("sourceRow");
    QTest::addColumn<int>("destRow");
    QTest::addColumn<QString>("expectedResult");

    QTest::newRow("0-3") << 0 << 3 << QStringLiteral("ABCA");
    QTest::newRow("1-2") << 1 << 2 << QStringLiteral("ABBD");
    QTest::newRow("2-1") << 2 << 1 << QStringLiteral("ACCD");
    QTest::newRow("3-0") << 3 << 0 << QStringLiteral("DBCD");

}

void tst_QConcatenateTablesProxyModel::shouldPropagateDropOnItem()
{
    // Given two source models who handle drops

    // Note: QStandardItemModel handles drop onto items by inserting child rows,
    // which is good for QTreeView but not for QTableView or QConcatenateTablesProxyModel.
    // So we use QStringListModel here instead.
    QConcatenateTablesProxyModel pm;
    QStringListModel model1({QStringLiteral("A"), QStringLiteral("B")});
    QStringListModel model2({QStringLiteral("C"), QStringLiteral("D")});
    pm.addSourceModel(&model1);
    pm.addSourceModel(&model2);
    QAbstractItemModelTester modelTest(&pm, this);
    QCOMPARE(extractColumnTexts(&pm, 0), QStringLiteral("ABCD"));

    // When dragging one item
    QFETCH(int, sourceRow);
    QMimeData* mimeData = pm.mimeData({pm.index(sourceRow, 0)});
    QVERIFY(mimeData);

    // and dropping onto another item
    QFETCH(int, destRow);
    QVERIFY(pm.canDropMimeData(mimeData, Qt::CopyAction, -1, -1, pm.index(destRow, 0)));
    QVERIFY(pm.dropMimeData(mimeData, Qt::CopyAction, -1, -1, pm.index(destRow, 0)));
    delete mimeData;

    // Then the result should be as expected
    QFETCH(QString, expectedResult);
    QCOMPARE(extractColumnTexts(&pm, 0), expectedResult);
}

void tst_QConcatenateTablesProxyModel::shouldPropagateDropBetweenItems()
{
    // Given two models combined
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod3);
    pm.addSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);
    QCOMPARE(pm.rowCount(), 3);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("456"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("DEF"));

    // When dragging the last row
    QModelIndexList indexes;
    indexes.reserve(pm.columnCount());
    for (int col = 0; col < pm.columnCount(); ++col) {
        indexes.append(pm.index(2, col));
    }
    QMimeData* mimeData = pm.mimeData(indexes);
    QVERIFY(mimeData);

    // and dropping it before row 1
    const int destRow = 1;
    QVERIFY(pm.canDropMimeData(mimeData, Qt::CopyAction, destRow, 0, QModelIndex()));
    QVERIFY(pm.dropMimeData(mimeData, Qt::CopyAction, destRow, 0, QModelIndex()));
    delete mimeData;

    // Then a new row should be inserted
    QCOMPARE(pm.rowCount(), 4);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("DEF"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("456"));
    QCOMPARE(extractRowTexts(&pm, 3), QStringLiteral("DEF"));
}

void tst_QConcatenateTablesProxyModel::shouldPropagateDropBetweenItemsAtModelBoundary()
{
    // Given two models combined
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod3);
    pm.addSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);
    QCOMPARE(pm.rowCount(), 3);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("456"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("DEF"));

    // When dragging the first row
    QModelIndexList indexes;
    indexes.reserve(pm.columnCount());
    for (int col = 0; col < pm.columnCount(); ++col) {
        indexes.append(pm.index(0, col));
    }
    QMimeData* mimeData = pm.mimeData(indexes);
    QVERIFY(mimeData);

    // and dropping it before row 2
    const int destRow = 2;
    QVERIFY(pm.canDropMimeData(mimeData, Qt::CopyAction, destRow, 0, QModelIndex()));
    QVERIFY(pm.dropMimeData(mimeData, Qt::CopyAction, destRow, 0, QModelIndex()));
    delete mimeData;

    // Then a new row should be inserted
    QCOMPARE(pm.rowCount(), 4);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("456"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 3), QStringLiteral("DEF"));

    // and it should be part of the second model
    QCOMPARE(mod2.rowCount(), 2);
}

void tst_QConcatenateTablesProxyModel::shouldPropagateDropAfterLastRow_data()
{
    QTest::addColumn<int>("destRow");

    // Dropping after the last row is documented to be done with destRow == -1.
    QTest::newRow("-1") << -1;
    // However, sometimes QTreeView calls dropMimeData with destRow == rowCount...
    // Not sure if that's a bug or not, but let's support it in the model, just in case.
    QTest::newRow("3") << 3;
}

void tst_QConcatenateTablesProxyModel::shouldPropagateDropAfterLastRow()
{
    QFETCH(int, destRow);

    // Given two models combined
    QConcatenateTablesProxyModel pm;
    pm.addSourceModel(&mod3);
    pm.addSourceModel(&mod2);
    QAbstractItemModelTester modelTest(&pm, this);
    QCOMPARE(pm.rowCount(), 3);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("456"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("DEF"));

    // When dragging the second row
    QModelIndexList indexes;
    indexes.reserve(pm.columnCount());
    for (int col = 0; col < pm.columnCount(); ++col) {
        indexes.append(pm.index(1, col));
    }
    QMimeData* mimeData = pm.mimeData(indexes);
    QVERIFY(mimeData);

    // and dropping it after the last row
    QVERIFY(pm.canDropMimeData(mimeData, Qt::CopyAction, destRow, 0, QModelIndex()));
    QVERIFY(pm.dropMimeData(mimeData, Qt::CopyAction, destRow, 0, QModelIndex()));
    delete mimeData;

    // Then a new row should be inserted at the end
    QCOMPARE(pm.rowCount(), 4);
    QCOMPARE(extractRowTexts(&pm, 0), QStringLiteral("123"));
    QCOMPARE(extractRowTexts(&pm, 1), QStringLiteral("456"));
    QCOMPARE(extractRowTexts(&pm, 2), QStringLiteral("DEF"));
    QCOMPARE(extractRowTexts(&pm, 3), QStringLiteral("456"));

}

QTEST_GUILESS_MAIN(tst_QConcatenateTablesProxyModel)

#include "tst_qconcatenatetablesproxymodel.moc"
