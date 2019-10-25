/****************************************************************************
**
** Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, authors Filipe Azevedo <filipe.azevedo@kdab.com> and David Faure <david.faure@kdab.com>
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

#include <QTest>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QStandardItem>

Q_DECLARE_METATYPE(QModelIndex)

static const int s_filterRole = Qt::UserRole + 1;

class ModelSignalSpy : public QObject {
    Q_OBJECT
public:
    explicit ModelSignalSpy(QAbstractItemModel &model) {
        connect(&model, &QAbstractItemModel::rowsInserted, this, &ModelSignalSpy::onRowsInserted);
        connect(&model, &QAbstractItemModel::rowsRemoved, this, &ModelSignalSpy::onRowsRemoved);
        connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, this, &ModelSignalSpy::onRowsAboutToBeInserted);
        connect(&model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ModelSignalSpy::onRowsAboutToBeRemoved);
        connect(&model, &QAbstractItemModel::rowsMoved, this, &ModelSignalSpy::onRowsMoved);
        connect(&model, &QAbstractItemModel::dataChanged, this, &ModelSignalSpy::onDataChanged);
        connect(&model, &QAbstractItemModel::layoutChanged, this, &ModelSignalSpy::onLayoutChanged);
        connect(&model, &QAbstractItemModel::modelReset, this, &ModelSignalSpy::onModelReset);
    }

    QStringList mSignals;

private Q_SLOTS:
    void onRowsInserted(QModelIndex p, int start, int end) {
        mSignals << QLatin1String("rowsInserted(") + textForRowSpy(p, start, end) + ')';
    }
    void onRowsRemoved(QModelIndex p, int start, int end) {
        mSignals << QLatin1String("rowsRemoved(") + textForRowSpy(p, start, end) + ')';
    }
    void onRowsAboutToBeInserted(QModelIndex p, int start, int end) {
        mSignals << QLatin1String("rowsAboutToBeInserted(") + textForRowSpy(p, start, end) + ')';
    }
    void onRowsAboutToBeRemoved(QModelIndex p, int start, int end) {
        mSignals << QLatin1String("rowsAboutToBeRemoved(") + textForRowSpy(p, start, end) + ')';
    }
    void onRowsMoved(QModelIndex,int,int,QModelIndex,int) {
        mSignals << QStringLiteral("rowsMoved");
    }
    void onDataChanged(const QModelIndex &from, const QModelIndex& ) {
        mSignals << QStringLiteral("dataChanged(%1)").arg(from.data(Qt::DisplayRole).toString());
    }
    void onLayoutChanged() {
        mSignals << QStringLiteral("layoutChanged");
    }
    void onModelReset() {
        mSignals << QStringLiteral("modelReset");
    }
private:
    QString textForRowSpy(const QModelIndex &parent, int start, int end)
    {
        QString txt = parent.data(Qt::DisplayRole).toString();
        if (!txt.isEmpty())
            txt += QLatin1Char('.');
        txt += QString::number(start+1);
        if (start != end)
            txt += QLatin1Char('-') + QString::number(end+1);
        return txt;
    }
};

class TestModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    TestModel(QAbstractItemModel *sourceModel)
        : QSortFilterProxyModel()
    {
        setFilterRole(s_filterRole);
        setRecursiveFilteringEnabled(true);
        setSourceModel(sourceModel);
    }

    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        return sourceModel()->index(sourceRow, 0, sourceParent).data(s_filterRole).toBool()
                && QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
};

// Represents this tree
// - A
// - - B
// - - - C
// - - - D
// - - E
// as a single string, englobing children in brackets, like this:
// [A[B[C D] E]]
// In addition, items that match the filtering (data(s_filterRole) == true) have a * after their value.
static QString treeAsString(const QAbstractItemModel &model, const QModelIndex &parent = QModelIndex())
{
    QString ret;
    const int rowCount = model.rowCount(parent);
    if (rowCount > 0) {
        ret += QLatin1Char('[');
        for (int row = 0 ; row < rowCount; ++row) {
            if (row > 0) {
                ret += ' ';
            }
            const QModelIndex child = model.index(row, 0, parent);
            ret += child.data(Qt::DisplayRole).toString();
            if (child.data(s_filterRole).toBool())
                ret += QLatin1Char('*');
            ret += treeAsString(model, child);
        }
        ret += QLatin1Char(']');
    }
    return ret;
}

// Fill a tree model based on a string representation (see treeAsString)
static void fillModel(QStandardItemModel &model, const QString &str)
{
    QCOMPARE(str.count('['), str.count(']'));
    QStandardItem *item = 0;
    QString data;
    for ( int i = 0 ; i < str.length() ; ++i ) {
        const QChar ch = str.at(i);
        if ((ch == '[' || ch == ']' || ch == ' ') && !data.isEmpty()) {
            if (data.endsWith('*')) {
                item->setData(true, s_filterRole);
                data.chop(1);
            }
            item->setText(data);
            data.clear();
        }
        if (ch == '[') {
            // Create new child
            QStandardItem *child = new QStandardItem;
            if (item)
                item->appendRow(child);
            else
                model.appendRow(child);
            item = child;
        } else if (ch == ']') {
            // Go up to parent
            item = item->parent();
        } else if (ch == ' ') {
            // Create new sibling
            QStandardItem *child = new QStandardItem;
            QStandardItem *parent = item->parent();
            if (parent)
                parent->appendRow(child);
            else
                model.appendRow(child);
            item = child;
        } else {
            data += ch;
        }
    }
}

class tst_QSortFilterProxyModel_Recursive : public QObject
{
    Q_OBJECT
private:
private Q_SLOTS:
    void testInitialFiltering_data()
    {
        QTest::addColumn<QString>("sourceStr");
        QTest::addColumn<QString>("proxyStr");

        QTest::newRow("empty") << "[]" << "";
        QTest::newRow("no") << "[1]" << "";
        QTest::newRow("yes") << "[1*]" << "[1*]";
        QTest::newRow("second") << "[1 2*]" << "[2*]";
        QTest::newRow("child_yes") << "[1 2[2.1*]]" << "[2[2.1*]]";
        QTest::newRow("grandchild_yes") << "[1 2[2.1[2.1.1*]]]" << "[2[2.1[2.1.1*]]]";
        // 1, 3.1 and 4.2.1 match, so their parents are in the model
        QTest::newRow("more") << "[1* 2[2.1] 3[3.1*] 4[4.1 4.2[4.2.1*]]]" << "[1* 3[3.1*] 4[4.2[4.2.1*]]]";
    }

    void testInitialFiltering()
    {
        QFETCH(QString, sourceStr);
        QFETCH(QString, proxyStr);

        QStandardItemModel model;
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QVERIFY(proxy.isRecursiveFilteringEnabled());
        QCOMPARE(treeAsString(proxy), proxyStr);
    }

    // Test changing a role that is unrelated to the filtering.
    void testUnrelatedDataChange()
    {
        QStandardItemModel model;
        const QString sourceStr = QStringLiteral("[1[1.1[1.1.1*]]]");
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), sourceStr);

        ModelSignalSpy spy(proxy);
        QStandardItem *item_1_1_1 = model.item(0)->child(0)->child(0);

        // When changing the text on the item
        item_1_1_1->setText(QStringLiteral("ME"));

        QCOMPARE(treeAsString(proxy), QStringLiteral("[1[1.1[ME*]]]"));

        // filterRole is Qt::UserRole + 1, so parents are not checked and
        // therefore no dataChanged for parents
        QCOMPARE(spy.mSignals, QStringList()
                 << QStringLiteral("dataChanged(ME)"));
    }

    // Test changing a role that is unrelated to the filtering, in a hidden item.
    void testHiddenDataChange()
    {
        QStandardItemModel model;
        const QString sourceStr = QStringLiteral("[1[1.1[1.1.1]]]");
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), QString());

        ModelSignalSpy spy(proxy);
        QStandardItem *item_1_1_1 = model.item(0)->child(0)->child(0);

        // When changing the text on a hidden item
        item_1_1_1->setText(QStringLiteral("ME"));

        QCOMPARE(treeAsString(proxy), QString());
        QCOMPARE(spy.mSignals, QStringList());
    }

    // Test that we properly react to a data-changed signal in a descendant and include all required rows
    void testDataChangeIn_data()
    {
        QTest::addColumn<QString>("sourceStr");
        QTest::addColumn<QString>("initialProxyStr");
        QTest::addColumn<QString>("add"); // set the flag on this item
        QTest::addColumn<QString>("expectedProxyStr");
        QTest::addColumn<QStringList>("expectedSignals");

        QTest::newRow("toplevel") << "[1]" << "" << "1" << "[1*]"
                                  << (QStringList() << QStringLiteral("rowsAboutToBeInserted(1)") << QStringLiteral("rowsInserted(1)"));
        QTest::newRow("show_parents") << "[1[1.1[1.1.1]]]" << "" << "1.1.1" << "[1[1.1[1.1.1*]]]"
                                      << (QStringList() << QStringLiteral("rowsAboutToBeInserted(1)") << QStringLiteral("rowsInserted(1)"));

        const QStringList insert_1_1_1 = QStringList()
                << QStringLiteral("rowsAboutToBeInserted(1.1.1)")
                << QStringLiteral("rowsInserted(1.1.1)")
                << QStringLiteral("dataChanged(1.1)")
                << QStringLiteral("dataChanged(1)")
        ;
        QTest::newRow("parent_visible") << "[1[1.1*[1.1.1]]]" << "[1[1.1*]]" << "1.1.1" << "[1[1.1*[1.1.1*]]]"
                                        << insert_1_1_1;

        QTest::newRow("sibling_visible") << "[1[1.1[1.1.1 1.1.2*]]]" << "[1[1.1[1.1.2*]]]" << "1.1.1" << "[1[1.1[1.1.1* 1.1.2*]]]"
                                         << insert_1_1_1;

        QTest::newRow("visible_cousin") << "[1[1.1[1.1.1 1.1.2[1.1.2.1*]]]]" << "[1[1.1[1.1.2[1.1.2.1*]]]]" << "1.1.1" << "[1[1.1[1.1.1* 1.1.2[1.1.2.1*]]]]"
                                        << insert_1_1_1;

        QTest::newRow("show_parent") << "[1[1.1[1.1.1 1.1.2] 1.2*]]" << "[1[1.2*]]" << "1.1.1" << "[1[1.1[1.1.1*] 1.2*]]"
                                     << (QStringList()
                                         << QStringLiteral("rowsAboutToBeInserted(1.1)")
                                         << QStringLiteral("rowsInserted(1.1)")
                                         << QStringLiteral("dataChanged(1)"));

        QTest::newRow("with_children") << "[1[1.1[1.1.1[1.1.1.1*]]] 2*]" << "[1[1.1[1.1.1[1.1.1.1*]]] 2*]" << "1.1.1" << "[1[1.1[1.1.1*[1.1.1.1*]]] 2*]"
                                       << (QStringList()
                                           << QStringLiteral("dataChanged(1.1.1)")
                                           << QStringLiteral("dataChanged(1.1)")
                                           << QStringLiteral("dataChanged(1)"));

    }

    void testDataChangeIn()
    {
        QFETCH(QString, sourceStr);
        QFETCH(QString, initialProxyStr);
        QFETCH(QString, add);
        QFETCH(QString, expectedProxyStr);
        QFETCH(QStringList, expectedSignals);

        QStandardItemModel model;
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), initialProxyStr);

        ModelSignalSpy spy(proxy);
        // When changing the data on the designated item to show this row
        QStandardItem *itemToChange = itemByText(model, add);
        QVERIFY(!itemToChange->data(s_filterRole).toBool());
        itemToChange->setData(true, s_filterRole);

        // The proxy should update as expected
        QCOMPARE(treeAsString(proxy), expectedProxyStr);

        //qDebug() << spy.mSignals;
        QCOMPARE(spy.mSignals, expectedSignals);
    }

    void testDataChangeOut_data()
    {
        QTest::addColumn<QString>("sourceStr");
        QTest::addColumn<QString>("initialProxyStr");
        QTest::addColumn<QString>("remove"); // unset the flag on this item
        QTest::addColumn<QString>("expectedProxyStr");
        QTest::addColumn<QStringList>("expectedSignals");

        const QStringList remove1_1_1 = (QStringList()
                                         << QStringLiteral("rowsAboutToBeRemoved(1.1.1)")
                                         << QStringLiteral("rowsRemoved(1.1.1)")
                                         << QStringLiteral("dataChanged(1.1)")
                                         << QStringLiteral("dataChanged(1)"));

        QTest::newRow("toplevel") << "[1*]" << "[1*]" << "1" << ""
                                  << (QStringList() << QStringLiteral("rowsAboutToBeRemoved(1)") << QStringLiteral("rowsRemoved(1)"));

        QTest::newRow("hide_parent") << "[1[1.1[1.1.1*]]]" << "[1[1.1[1.1.1*]]]" << "1.1.1" << "" <<
                                        (QStringList()
                                         << QStringLiteral("rowsAboutToBeRemoved(1.1.1)")
                                         << QStringLiteral("rowsRemoved(1.1.1)")
                                         << QStringLiteral("rowsAboutToBeRemoved(1.1)")
                                         << QStringLiteral("rowsRemoved(1.1)")
                                         << QStringLiteral("rowsAboutToBeRemoved(1)")
                                         << QStringLiteral("rowsRemoved(1)"));

        QTest::newRow("parent_visible") << "[1[1.1*[1.1.1*]]]" << "[1[1.1*[1.1.1*]]]" << "1.1.1" << "[1[1.1*]]"
                                        << remove1_1_1;

        QTest::newRow("visible") << "[1[1.1[1.1.1* 1.1.2*]]]" << "[1[1.1[1.1.1* 1.1.2*]]]" << "1.1.1" << "[1[1.1[1.1.2*]]]"
                                 << remove1_1_1;
        QTest::newRow("visible_cousin") << "[1[1.1[1.1.1* 1.1.2[1.1.2.1*]]]]" << "[1[1.1[1.1.1* 1.1.2[1.1.2.1*]]]]" << "1.1.1" << "[1[1.1[1.1.2[1.1.2.1*]]]]"
                                 << remove1_1_1;

        // The following tests trigger the removal of an ascendant.
        QTest::newRow("remove_parent") << "[1[1.1[1.1.1* 1.1.2] 1.2*]]" << "[1[1.1[1.1.1*] 1.2*]]" << "1.1.1" << "[1[1.2*]]"
                                      << (QStringList()
                                          << QStringLiteral("rowsAboutToBeRemoved(1.1.1)")
                                          << QStringLiteral("rowsRemoved(1.1.1)")
                                          << QStringLiteral("rowsAboutToBeRemoved(1.1)")
                                          << QStringLiteral("rowsRemoved(1.1)")
                                          << QStringLiteral("dataChanged(1)"));

        QTest::newRow("with_children") << "[1[1.1[1.1.1*[1.1.1.1*]]] 2*]" << "[1[1.1[1.1.1*[1.1.1.1*]]] 2*]" << "1.1.1" << "[1[1.1[1.1.1[1.1.1.1*]]] 2*]"
                                       << (QStringList()
                                           << QStringLiteral("dataChanged(1.1.1)")
                                           << QStringLiteral("dataChanged(1.1)")
                                           << QStringLiteral("dataChanged(1)"));

        QTest::newRow("last_visible") << "[1[1.1[1.1.1* 1.1.2]]]" << "[1[1.1[1.1.1*]]]" << "1.1.1" << ""
                                      << (QStringList()
                                          << QStringLiteral("rowsAboutToBeRemoved(1.1.1)")
                                          << QStringLiteral("rowsRemoved(1.1.1)")
                                          << QStringLiteral("rowsAboutToBeRemoved(1.1)")
                                          << QStringLiteral("rowsRemoved(1.1)")
                                          << QStringLiteral("rowsAboutToBeRemoved(1)")
                                          << QStringLiteral("rowsRemoved(1)"));

    }

    void testDataChangeOut()
    {
        QFETCH(QString, sourceStr);
        QFETCH(QString, initialProxyStr);
        QFETCH(QString, remove);
        QFETCH(QString, expectedProxyStr);
        QFETCH(QStringList, expectedSignals);

        QStandardItemModel model;
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), initialProxyStr);

        ModelSignalSpy spy(proxy);

        // When changing the data on the designated item to exclude this row again
        QStandardItem *itemToChange = itemByText(model, remove);
        QVERIFY(itemToChange->data(s_filterRole).toBool());
        itemToChange->setData(false, s_filterRole);

        // The proxy should update as expected
        QCOMPARE(treeAsString(proxy), expectedProxyStr);

        //qDebug() << spy.mSignals;
        QCOMPARE(spy.mSignals, expectedSignals);
    }

    void testInsert()
    {
        QStandardItemModel model;
        const QString sourceStr = QStringLiteral("[1[1.1[1.1.1]]]");
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), QString());

        ModelSignalSpy spy(proxy);
        QStandardItem *item_1_1_1 = model.item(0)->child(0)->child(0);
        QStandardItem *item_1_1_1_1 = new QStandardItem(QStringLiteral("1.1.1.1"));
        item_1_1_1_1->setData(true, s_filterRole);
        item_1_1_1->appendRow(item_1_1_1_1);
        QCOMPARE(treeAsString(proxy), QStringLiteral("[1[1.1[1.1.1[1.1.1.1*]]]]"));

        QCOMPARE(spy.mSignals, QStringList() << QStringLiteral("rowsAboutToBeInserted(1)")
                                             << QStringLiteral("rowsInserted(1)"));
    }

    // Start from [1[1.1[1.1.1 1.1.2[1.1.2.1*]]]]
    // where 1.1.1 is hidden but 1.1 is shown, we want to insert a shown child in 1.1.1.
    // The proxy ensures dataChanged is called on 1.1,
    // so that 1.1.1 and 1.1.1.1 are included in the model.
    void testInsertCousin()
    {
        QStandardItemModel model;
        const QString sourceStr = QStringLiteral("[1[1.1[1.1.1 1.1.2[1.1.2.1*]]]]");
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), QStringLiteral("[1[1.1[1.1.2[1.1.2.1*]]]]"));

        ModelSignalSpy spy(proxy);
        {
            QStandardItem *item_1_1_1_1 = new QStandardItem(QStringLiteral("1.1.1.1"));
            item_1_1_1_1->setData(true, s_filterRole);
            QStandardItem *item_1_1_1 = model.item(0)->child(0)->child(0);
            item_1_1_1->appendRow(item_1_1_1_1);
        }

        QCOMPARE(treeAsString(proxy), QStringLiteral("[1[1.1[1.1.1[1.1.1.1*] 1.1.2[1.1.2.1*]]]]"));
        //qDebug() << spy.mSignals;
        QCOMPARE(spy.mSignals, QStringList()
                 << QStringLiteral("rowsAboutToBeInserted(1.1.1)")
                 << QStringLiteral("rowsInserted(1.1.1)")
                 << QStringLiteral("dataChanged(1.1)")
                 << QStringLiteral("dataChanged(1)"));
    }

    void testInsertWithChildren()
    {
        QStandardItemModel model;
        const QString sourceStr = QStringLiteral("[1[1.1]]");
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), QString());

        ModelSignalSpy spy(proxy);
        {
            QStandardItem *item_1_1_1 = new QStandardItem(QStringLiteral("1.1.1"));
            QStandardItem *item_1_1_1_1 = new QStandardItem(QStringLiteral("1.1.1.1"));
            item_1_1_1_1->setData(true, s_filterRole);
            item_1_1_1->appendRow(item_1_1_1_1);

            QStandardItem *item_1_1 = model.item(0)->child(0);
            item_1_1->appendRow(item_1_1_1);
        }

        QCOMPARE(treeAsString(proxy), QStringLiteral("[1[1.1[1.1.1[1.1.1.1*]]]]"));
        QCOMPARE(spy.mSignals, QStringList()
                 << QStringLiteral("rowsAboutToBeInserted(1)")
                 << QStringLiteral("rowsInserted(1)"));
    }

    void testInsertIntoVisibleWithChildren()
    {
        QStandardItemModel model;
        const QString sourceStr = QStringLiteral("[1[1.1[1.1.1*]]]");
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), sourceStr);

        ModelSignalSpy spy(proxy);
        {
            QStandardItem *item_1_1_2 = new QStandardItem(QStringLiteral("1.1.2"));
            QStandardItem *item_1_1_2_1 = new QStandardItem(QStringLiteral("1.1.2.1"));
            item_1_1_2_1->setData(true, s_filterRole);
            item_1_1_2->appendRow(item_1_1_2_1);

            QStandardItem *item_1_1 = model.item(0)->child(0);
            item_1_1->appendRow(item_1_1_2);
        }

        QCOMPARE(treeAsString(proxy), QStringLiteral("[1[1.1[1.1.1* 1.1.2[1.1.2.1*]]]]"));
        QCOMPARE(spy.mSignals, QStringList()
                 << QStringLiteral("rowsAboutToBeInserted(1.1.2)")
                 << QStringLiteral("rowsInserted(1.1.2)"));
    }

    void testInsertBefore()
    {
        QStandardItemModel model;
        const QString sourceStr = "[1[1.1[1.1.2*]]]";
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), sourceStr);

        ModelSignalSpy spy(proxy);
        {
            QStandardItem *item_1_1_1 = new QStandardItem("1.1.1");

            QStandardItem *item_1_1 = model.item(0)->child(0);
            item_1_1->insertRow(0, item_1_1_1);
        }

        QCOMPARE(treeAsString(proxy), QString("[1[1.1[1.1.2*]]]"));
        QCOMPARE(spy.mSignals, QStringList());
    }

    void testInsertHidden() // inserting filtered-out rows shouldn't emit anything
    {
        QStandardItemModel model;
        const QString sourceStr = QStringLiteral("[1[1.1]]");
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), QString());

        ModelSignalSpy spy(proxy);
        {
            QStandardItem *item_1_1_1 = new QStandardItem(QStringLiteral("1.1.1"));
            QStandardItem *item_1_1_1_1 = new QStandardItem(QStringLiteral("1.1.1.1"));
            item_1_1_1->appendRow(item_1_1_1_1);

            QStandardItem *item_1_1 = model.item(0)->child(0);
            item_1_1->appendRow(item_1_1_1);
        }

        QCOMPARE(treeAsString(proxy), QString());
        QCOMPARE(spy.mSignals, QStringList());
    }

    void testConsecutiveInserts_data()
    {
        testInitialFiltering_data();
    }

    void testConsecutiveInserts()
    {
        QFETCH(QString, sourceStr);
        QFETCH(QString, proxyStr);

        QStandardItemModel model;
        TestModel proxy(&model); // this time the proxy listens to the model while we fill it

        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);
        QCOMPARE(treeAsString(proxy), proxyStr);
    }

    void testRemove_data()
    {
        QTest::addColumn<QString>("sourceStr");
        QTest::addColumn<QString>("initialProxyStr");
        QTest::addColumn<QString>("remove"); // remove this item
        QTest::addColumn<QString>("expectedProxyStr");
        QTest::addColumn<QStringList>("expectedSignals");

        const QStringList remove1_1_1 = (QStringList() << QStringLiteral("rowsAboutToBeRemoved(1.1.1)") << QStringLiteral("rowsRemoved(1.1.1)"));

        QTest::newRow("toplevel") << "[1* 2* 3*]" << "[1* 2* 3*]" << "1" << "[2* 3*]"
                                  << (QStringList() << QStringLiteral("rowsAboutToBeRemoved(1)") << QStringLiteral("rowsRemoved(1)"));

        QTest::newRow("remove_hidden") << "[1 2* 3*]" << "[2* 3*]" << "1" << "[2* 3*]" << QStringList();

        QTest::newRow("parent_hidden") << "[1[1.1[1.1.1]]]" << "" << "1.1.1" << "" << QStringList();

        QTest::newRow("child_hidden") << "[1[1.1*[1.1.1]]]" << "[1[1.1*]]" << "1.1.1" << "[1[1.1*]]" << QStringList();

        QTest::newRow("parent_visible") << "[1[1.1*[1.1.1*]]]" << "[1[1.1*[1.1.1*]]]" << "1.1.1" << "[1[1.1*]]"
                                        << remove1_1_1;

        QTest::newRow("visible") << "[1[1.1[1.1.1* 1.1.2*]]]" << "[1[1.1[1.1.1* 1.1.2*]]]" << "1.1.1" << "[1[1.1[1.1.2*]]]"
                                 << remove1_1_1;
        QTest::newRow("visible_cousin") << "[1[1.1[1.1.1* 1.1.2[1.1.2.1*]]]]" << "[1[1.1[1.1.1* 1.1.2[1.1.2.1*]]]]" << "1.1.1" << "[1[1.1[1.1.2[1.1.2.1*]]]]"
                                 << remove1_1_1;

        // The following tests trigger the removal of an ascendant.
        // We could optimize the rows{AboutToBe,}Removed(1.1.1) away...

        QTest::newRow("remove_parent") << "[1[1.1[1.1.1* 1.1.2] 1.2*]]" << "[1[1.1[1.1.1*] 1.2*]]" << "1.1.1" << "[1[1.2*]]"
                                      << (QStringList()
                                          << QStringLiteral("rowsAboutToBeRemoved(1.1.1)")
                                          << QStringLiteral("rowsRemoved(1.1.1)")
                                          << QStringLiteral("rowsAboutToBeRemoved(1.1)")
                                          << QStringLiteral("rowsRemoved(1.1)")
                                          << QStringLiteral("dataChanged(1)"));

        QTest::newRow("with_children") << "[1[1.1[1.1.1[1.1.1.1*]]] 2*]" << "[1[1.1[1.1.1[1.1.1.1*]]] 2*]" << "1.1.1" << "[2*]"
                                       << (QStringList()
                                           << QStringLiteral("rowsAboutToBeRemoved(1.1.1)")
                                           << QStringLiteral("rowsRemoved(1.1.1)")
                                           << QStringLiteral("rowsAboutToBeRemoved(1)")
                                           << QStringLiteral("rowsRemoved(1)"));

        QTest::newRow("last_visible") << "[1[1.1[1.1.1* 1.1.2]]]" << "[1[1.1[1.1.1*]]]" << "1.1.1" << ""
                                      << (QStringList()
                                          << QStringLiteral("rowsAboutToBeRemoved(1.1.1)")
                                          << QStringLiteral("rowsRemoved(1.1.1)")
                                          << QStringLiteral("rowsAboutToBeRemoved(1)")
                                          << QStringLiteral("rowsRemoved(1)"));


    }

    void testRemove()
    {
        QFETCH(QString, sourceStr);
        QFETCH(QString, initialProxyStr);
        QFETCH(QString, remove);
        QFETCH(QString, expectedProxyStr);
        QFETCH(QStringList, expectedSignals);

        QStandardItemModel model;
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), initialProxyStr);

        ModelSignalSpy spy(proxy);
        QStandardItem *itemToRemove = itemByText(model, remove);
        QVERIFY(itemToRemove);
        if (itemToRemove->parent())
            itemToRemove->parent()->removeRow(itemToRemove->row());
        else
            model.removeRow(itemToRemove->row());
        QCOMPARE(treeAsString(proxy), expectedProxyStr);

        //qDebug() << spy.mSignals;
        QCOMPARE(spy.mSignals, expectedSignals);
    }

    void testStandardFiltering_data()
    {
        QTest::addColumn<QString>("sourceStr");
        QTest::addColumn<QString>("initialProxyStr");
        QTest::addColumn<QString>("filter");
        QTest::addColumn<QString>("expectedProxyStr");

        QTest::newRow("select_child") << "[1[1.1[1.1.1* 1.1.2*]]]" << "[1[1.1[1.1.1* 1.1.2*]]]"
                                      << "1.1.2" << "[1[1.1[1.1.2*]]]";

        QTest::newRow("filter_all_out") << "[1[1.1[1.1.1*]]]" << "[1[1.1[1.1.1*]]]"
                                        << "test" << "";

        QTest::newRow("select_parent") << "[1[1.1[1.1.1*[child*] 1.1.2*]]]" << "[1[1.1[1.1.1*[child*] 1.1.2*]]]"
                                      << "1.1.1" << "[1[1.1[1.1.1*]]]";

    }

    void testStandardFiltering()
    {
        QFETCH(QString, sourceStr);
        QFETCH(QString, initialProxyStr);
        QFETCH(QString, filter);
        QFETCH(QString, expectedProxyStr);

        QStandardItemModel model;
        fillModel(model, sourceStr);
        QCOMPARE(treeAsString(model), sourceStr);

        TestModel proxy(&model);
        QCOMPARE(treeAsString(proxy), initialProxyStr);

        ModelSignalSpy spy(proxy);

        //qDebug() << "setFilterFixedString";
        proxy.setFilterRole(Qt::DisplayRole);
        proxy.setFilterFixedString(filter);

        QCOMPARE(treeAsString(proxy), expectedProxyStr);

    }

private:
    QStandardItem *itemByText(const QStandardItemModel& model, const QString &text) const {
        QModelIndexList list = model.match(model.index(0, 0), Qt::DisplayRole, text, 1, Qt::MatchRecursive);
        return list.isEmpty() ? 0 : model.itemFromIndex(list.first());
    }
};

QTEST_GUILESS_MAIN(tst_QSortFilterProxyModel_Recursive)
#include "tst_qsortfilterproxymodel_recursive.moc"
