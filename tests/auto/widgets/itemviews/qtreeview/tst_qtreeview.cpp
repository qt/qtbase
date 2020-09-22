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

#include "../../../../shared/fakedirmodel.h"

#include <QDesktopWidget>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QProxyStyle>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QTest>
#include <QVBoxLayout>
#include <private/qtreeview_p.h>
#include <private/qtesthelpers_p.h>

using namespace QTestPrivate;

#if QT_CONFIG(draganddrop)
Q_DECLARE_METATYPE(QAbstractItemView::DragDropMode)
#endif
Q_DECLARE_METATYPE(QAbstractItemView::EditTriggers)
Q_DECLARE_METATYPE(QAbstractItemView::EditTrigger)

using IntBounds = std::numeric_limits<int>;
static void initStandardTreeModel(QStandardItemModel *model)
{
    QStandardItem *item;
    item = new QStandardItem(QLatin1String("Row 1 Item"));
    model->insertRow(0, item);

    item = new QStandardItem(QLatin1String("Row 2 Item"));
    item->setCheckable(true);
    model->insertRow(1, item);

    QStandardItem *childItem = new QStandardItem(QLatin1String("Row 2 Child Item"));
    item->setChild(0, childItem);

    item = new QStandardItem(QLatin1String("Row 3 Item"));
    item->setIcon(QIcon());
    model->insertRow(2, item);
}

class TreeView : public QTreeView
{
    Q_OBJECT
public:
    using QTreeView::QTreeView;
    using QTreeView::selectedIndexes;

    void paintEvent(QPaintEvent *event) override
    {
        QTreeView::paintEvent(event);
        wasPainted = true;
    }
    bool wasPainted = false;
public slots:
    void handleSelectionChanged()
    {
        //let's select the last item
        QModelIndex idx = model()->index(0, 0);
        selectionModel()->select(QItemSelection(idx, idx), QItemSelectionModel::Select);
        disconnect(selectionModel(), &QItemSelectionModel::selectionChanged,
                   this, &TreeView::handleSelectionChanged);
    }
};

class tst_QTreeView : public QObject
{
    Q_OBJECT

public slots:
    void selectionOrderTest();

private slots:
    void initTestCase() { QApplication::setKeyboardInputInterval(100); }
    void getSetCheck();

    // one test per QTreeView property
    void construction();
    void alternatingRowColors();
    void currentIndex_data();
    void currentIndex();
#if QT_CONFIG(draganddrop)
    void dragDropMode_data();
    void dragDropMode();
    void dragDropModeFromDragEnabledAndAcceptDrops_data();
    void dragDropModeFromDragEnabledAndAcceptDrops();
    void dragDropOverwriteMode();
#endif
    void editTriggers_data();
    void editTriggers();
    void hasAutoScroll();
    void horizontalScrollMode();
    void iconSize();
    void indexAt();
    void indexWidget();
    void itemDelegate();
    void itemDelegateForColumnOrRow();
    void keyboardSearch();
    void keyboardSearchMultiColumn();
    void setModel();
    void openPersistentEditor();
    void rootIndex();

    // specialized tests below
    void setHeader();
    void columnHidden();
    void rowHidden();
    void noDelegate();
    void noModel();
    void emptyModel();
    void removeRows();
    void removeCols();
    void limitedExpand();
    void expandAndCollapse_data();
    void expandAndCollapse();
    void expandAndCollapseAll();
    void expandWithNoChildren();
#if QT_CONFIG(animation)
    void quickExpandCollapse();
#endif
    void keyboardNavigation();
    void headerSections();
    void moveCursor_data();
    void moveCursor();
    void setSelection_data();
    void setSelection();
    void extendedSelection_data();
    void extendedSelection();
    void indexAbove();
    void indexBelow();
    void clicked();
    void mouseDoubleClick();
    void rowsAboutToBeRemoved();
    void headerSections_unhideSection();
    void columnAt();
    void scrollTo();
    void rowsAboutToBeRemoved_move();
    void resizeColumnToContents();
    void insertAfterSelect();
    void removeAfterSelect();
    void hiddenItems();
    void spanningItems();
    void rowSizeHint();
    void setSortingEnabledTopLevel();
    void setSortingEnabledChild();
    void headerHidden();
    void indentation();

    void selection();
    void removeAndInsertExpandedCol0();
    void selectionWithHiddenItems();
    void selectAll();

    void disabledButCheckable();
    void sortByColumn_data();
    void sortByColumn();

    void evilModel_data();
    void evilModel();

    void indexRowSizeHint();
    void addRowsWhileSectionsAreHidden();
    void filterProxyModelCrash();
    void renderToPixmap_data();
    void renderToPixmap();
    void styleOptionViewItem();
    void keyboardNavigationWithDisabled();
    void saveRestoreState();

    void statusTip_data();
    void statusTip();
    void fetchMoreOnScroll();

    // task-specific tests:
    void task174627_moveLeftToRoot();
    void task171902_expandWith1stColHidden();
    void task203696_hidingColumnsAndRowsn();
    void task211293_removeRootIndex();
    void task216717_updateChildren();
    void task220298_selectColumns();
    void task224091_appendColumns();
    void task225539_deleteModel();
    void task230123_setItemsExpandable();
    void task202039_closePersistentEditor();
    void task238873_avoidAutoReopening();
    void task244304_clickOnDecoration();
    void task246536_scrollbarsNotWorking();
    void task250683_wrongSectionSize();
    void task239271_addRowsWithFirstColumnHidden();
    void task254234_proxySort();
    void task248022_changeSelection();
    void task245654_changeModelAndExpandAll();
    void doubleClickedWithSpans();
    void taskQTBUG_6450_selectAllWith1stColumnHidden();
    void taskQTBUG_9216_setSizeAndUniformRowHeightsWrongRepaint();
    void taskQTBUG_11466_keyboardNavigationRegression();
    void taskQTBUG_13567_removeLastItemRegression();
    void taskQTBUG_25333_adjustViewOptionsForIndex();
    void taskQTBUG_18539_emitLayoutChanged();
    void taskQTBUG_8176_emitOnExpandAll();
    void taskQTBUG_37813_crash();
    void taskQTBUG_45697_crash();
    void taskQTBUG_7232_AllowUserToControlSingleStep();
    void taskQTBUG_8376();
    void taskQTBUG_61476();
    void testInitialFocus();
    void fetchUntilScreenFull();
};

class QtTestModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    QtTestModel(int _rows, int _cols, QObject *parent = nullptr)
        : QAbstractItemModel(parent), rows(_rows), cols(_cols)
    {}

    inline qint32 level(const QModelIndex &index) const
    {
        return index.isValid() ? qint32(index.internalId()) : qint32(-1);
    }

    bool canFetchMore(const QModelIndex &) const override { return !fetched; }

    void fetchMore(const QModelIndex &) override { fetched = true; }

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override
    {
        bool hasFetched = fetched;
        fetched = true;
        bool r = QAbstractItemModel::hasChildren(parent);
        fetched = hasFetched;
        return r;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (!fetched)
            qFatal("%s: rowCount should not be called before fetching", Q_FUNC_INFO);
        if ((parent.column() > 0) || (level(parent) > levels))
            return 0;
        return rows;
    }
    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if ((parent.column() > 0) || (level(parent) > levels))
            return 0;
        return cols;
    }

    bool isEditable(const QModelIndex &index) const
    {
        if (index.isValid())
            return true;
        return false;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (onlyValidCalls) {
            Q_ASSERT(row >= 0);
            Q_ASSERT(column >= 0);
            Q_ASSERT(row < rows);
            Q_ASSERT(column < cols);
        }
        if (row < 0 || column < 0 || (level(parent) > levels) || column >= cols || row >= rows) {
            return QModelIndex();
        }
        QModelIndex i = createIndex(row, column, quintptr(level(parent) + 1));
        parentHash[i] = parent;
        return i;
    }

    QModelIndex parent(const QModelIndex &index) const override
    {
        if (!parentHash.contains(index))
            return QModelIndex();
        return parentHash[index];
    }

    QVariant data(const QModelIndex &idx, int role) const override
    {
        if (!idx.isValid())
            return QVariant();

        if (role == Qt::DisplayRole) {
            if (idx.row() < 0 || idx.column() < 0 || idx.column() >= cols || idx.row() >= rows) {
                wrongIndex = true;
                qWarning("Invalid modelIndex [%d,%d,%p]", idx.row(), idx.column(),
                         idx.internalPointer());
            }
            QString result = QLatin1Char('[') + QString::number(idx.row()) + QLatin1Char(',')
                + QString::number(idx.column()) + QLatin1Char(',') +  QString::number(level(idx))
                + QLatin1Char(']');
            if (idx.row() & 1)
                result += QLatin1String(" - this item is extra wide");
            return result;
        }
        if (decorationsEnabled && role == Qt::DecorationRole) {
            QPixmap pm(16,16);
            pm.fill(QColor::fromHsv((idx.column() % 16)*8 + 64, 254, (idx.row() % 16)*8 + 32));
            return pm;
        }
        if (statusTipsEnabled && role == Qt::StatusTipRole)
            return QString("[%1,%2,%3] -- Status").arg(idx.row()).arg(idx.column()).arg(level(idx));
        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override
    {
        Q_UNUSED(orientation);
        if (section < 0 || section >= columnCount())
            return QVariant();
        if (statusTipsEnabled && role == Qt::StatusTipRole)
            return QString("Header %1 -- Status").arg(section);
        return QVariant();
    }

    void simulateMoveRows()
    {
        beginMoveRows(QModelIndex(), 0, 0, QModelIndex(), 2);
        endMoveRows();
    }

    void removeLastRow()
    {
        beginRemoveRows(QModelIndex(), rows - 1, rows - 1);
        --rows;
        endRemoveRows();
    }

    void removeAllRows()
    {
        beginRemoveRows(QModelIndex(), 0, rows - 1);
        rows = 0;
        endRemoveRows();
    }

    void removeLastColumn()
    {
        beginRemoveColumns(QModelIndex(), cols - 1, cols - 1);
        --cols;
        endRemoveColumns();
    }

    void removeAllColumns()
    {
        beginRemoveColumns(QModelIndex(), 0, cols - 1);
        cols = 0;
        endRemoveColumns();
    }

    void insertNewRow()
    {
        beginInsertRows(QModelIndex(), rows - 1, rows - 1);
        ++rows;
        endInsertRows();
    }

    void setDecorationsEnabled(bool enable)
    {
        decorationsEnabled = enable;
    }

    mutable QMap<QModelIndex,QModelIndex> parentHash;
    int rows = 0;
    int cols = 0;
    int levels = IntBounds::max();
    mutable bool wrongIndex = false;
    mutable bool fetched = false;
    bool decorationsEnabled = false;
    bool statusTipsEnabled = false;
    bool onlyValidCalls = false;
};

// Testing get/set functions
void tst_QTreeView::getSetCheck()
{
    QTreeView obj1;

    // int QTreeView::indentation()
    // void QTreeView::setIndentation(int)
    const int styledIndentation = obj1.style()->pixelMetric(
          QStyle::PM_TreeViewIndentation, nullptr, &obj1);
    QCOMPARE(obj1.indentation(), styledIndentation);
    obj1.setIndentation(0);
    QCOMPARE(obj1.indentation(), 0);
    obj1.setIndentation(IntBounds::min());
    QCOMPARE(obj1.indentation(), IntBounds::min());
    obj1.setIndentation(IntBounds::max());
    QCOMPARE(obj1.indentation(), IntBounds::max());

    // bool QTreeView::rootIsDecorated()
    // void QTreeView::setRootIsDecorated(bool)
    QCOMPARE(obj1.rootIsDecorated(), true);
    obj1.setRootIsDecorated(false);
    QCOMPARE(obj1.rootIsDecorated(), false);
    obj1.setRootIsDecorated(true);
    QCOMPARE(obj1.rootIsDecorated(), true);

    // bool QTreeView::uniformRowHeights()
    // void QTreeView::setUniformRowHeights(bool)
    QCOMPARE(obj1.uniformRowHeights(), false);
    obj1.setUniformRowHeights(false);
    QCOMPARE(obj1.uniformRowHeights(), false);
    obj1.setUniformRowHeights(true);
    QCOMPARE(obj1.uniformRowHeights(), true);

    // bool QTreeView::itemsExpandable()
    // void QTreeView::setItemsExpandable(bool)
    QCOMPARE(obj1.itemsExpandable(), true);
    obj1.setItemsExpandable(false);
    QCOMPARE(obj1.itemsExpandable(), false);
    obj1.setItemsExpandable(true);
    QCOMPARE(obj1.itemsExpandable(), true);

    // bool QTreeView::allColumnsShowFocus
    // void QTreeView::setAllColumnsShowFocus
    QCOMPARE(obj1.allColumnsShowFocus(), false);
    obj1.setAllColumnsShowFocus(false);
    QCOMPARE(obj1.allColumnsShowFocus(), false);
    obj1.setAllColumnsShowFocus(true);
    QCOMPARE(obj1.allColumnsShowFocus(), true);

    // bool QTreeView::isAnimated
    // void QTreeView::setAnimated
    QCOMPARE(obj1.isAnimated(), false);
    obj1.setAnimated(false);
    QCOMPARE(obj1.isAnimated(), false);
    obj1.setAnimated(true);
    QCOMPARE(obj1.isAnimated(), true);

    // bool QTreeView::setSortingEnabled
    // void QTreeView::isSortingEnabled
    QCOMPARE(obj1.isSortingEnabled(), false);
    obj1.setSortingEnabled(false);
    QCOMPARE(obj1.isSortingEnabled(), false);
    obj1.setSortingEnabled(true);
    QCOMPARE(obj1.isSortingEnabled(), true);
}

void tst_QTreeView::construction()
{
    QTreeView view;

    // QAbstractItemView properties
    QVERIFY(!view.alternatingRowColors());
    QCOMPARE(view.currentIndex(), QModelIndex());
#if QT_CONFIG(draganddrop)
    QCOMPARE(view.dragDropMode(), QAbstractItemView::NoDragDrop);
    QVERIFY(!view.dragDropOverwriteMode());
    QVERIFY(!view.dragEnabled());
#endif
    QCOMPARE(view.editTriggers(), QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
    QVERIFY(view.hasAutoScroll());
    QCOMPARE(view.horizontalScrollMode(), QAbstractItemView::ScrollPerPixel);
    QCOMPARE(view.iconSize(), QSize());
    QCOMPARE(view.indexAt(QPoint()), QModelIndex());
    QVERIFY(!view.indexWidget(QModelIndex()));
    QVERIFY(qobject_cast<QStyledItemDelegate *>(view.itemDelegate()));
    QVERIFY(!view.itemDelegateForColumn(-1));
    QVERIFY(!view.itemDelegateForColumn(0));
    QVERIFY(!view.itemDelegateForColumn(1));
    QVERIFY(!view.itemDelegateForRow(-1));
    QVERIFY(!view.itemDelegateForRow(0));
    QVERIFY(!view.itemDelegateForRow(1));
    QVERIFY(!view.model());
    QCOMPARE(view.rootIndex(), QModelIndex());
    QCOMPARE(view.selectionBehavior(), QAbstractItemView::SelectRows);
    QCOMPARE(view.selectionMode(), QAbstractItemView::SingleSelection);
    QVERIFY(!view.selectionModel());
#if QT_CONFIG(draganddrop)
    QVERIFY(view.showDropIndicator());
#endif
    QCOMPARE(view.QAbstractItemView::sizeHintForColumn(-1), -1); // <- protected in QTreeView
    QCOMPARE(view.QAbstractItemView::sizeHintForColumn(0), -1); // <- protected in QTreeView
    QCOMPARE(view.QAbstractItemView::sizeHintForColumn(1), -1); // <- protected in QTreeView
    QCOMPARE(view.sizeHintForIndex(QModelIndex()), QSize());
    QCOMPARE(view.sizeHintForRow(-1), -1);
    QCOMPARE(view.sizeHintForRow(0), -1);
    QCOMPARE(view.sizeHintForRow(1), -1);
    QVERIFY(!view.tabKeyNavigation());
    QCOMPARE(view.textElideMode(), Qt::ElideRight);
    QCOMPARE(static_cast<int>(view.verticalScrollMode()),
             view.style()->styleHint(QStyle::SH_ItemView_ScrollMode, nullptr, &view));
    QCOMPARE(view.visualRect(QModelIndex()), QRect());

    // QTreeView properties
    QVERIFY(!view.allColumnsShowFocus());
    QCOMPARE(view.autoExpandDelay(), -1);
    QCOMPARE(view.columnAt(-1), -1);
    QCOMPARE(view.columnAt(0), -1);
    QCOMPARE(view.columnAt(1), -1);
    QCOMPARE(view.columnViewportPosition(-1), -1);
    QCOMPARE(view.columnViewportPosition(0), -1);
    QCOMPARE(view.columnViewportPosition(1), -1);
    QCOMPARE(view.columnWidth(-1), 0);
    QCOMPARE(view.columnWidth(0), 0);
    QCOMPARE(view.columnWidth(1), 0);
    QVERIFY(view.header());
    QCOMPARE(view.indentation(),
             view.style()->pixelMetric(QStyle::PM_TreeViewIndentation, nullptr, &view));
    QCOMPARE(view.indexAbove(QModelIndex()), QModelIndex());
    QCOMPARE(view.indexBelow(QModelIndex()), QModelIndex());
    QVERIFY(!view.isAnimated());
    QVERIFY(!view.isColumnHidden(-1));
    QVERIFY(!view.isColumnHidden(0));
    QVERIFY(!view.isColumnHidden(1));
    QVERIFY(!view.isExpanded(QModelIndex()));
    QVERIFY(!view.isRowHidden(-1, QModelIndex()));
    QVERIFY(!view.isRowHidden(0, QModelIndex()));
    QVERIFY(!view.isRowHidden(1, QModelIndex()));
    QVERIFY(!view.isFirstColumnSpanned(-1, QModelIndex()));
    QVERIFY(!view.isFirstColumnSpanned(0, QModelIndex()));
    QVERIFY(!view.isFirstColumnSpanned(1, QModelIndex()));
    QVERIFY(!view.isSortingEnabled());
    QVERIFY(view.itemsExpandable());
    QVERIFY(view.rootIsDecorated());
    QVERIFY(!view.uniformRowHeights());
    QCOMPARE(view.visualRect(QModelIndex()), QRect());
    QVERIFY(!view.wordWrap());
}

void tst_QTreeView::alternatingRowColors()
{
    QTreeView view;
    QVERIFY(!view.alternatingRowColors());
    view.setAlternatingRowColors(true);
    QVERIFY(view.alternatingRowColors());
    view.setAlternatingRowColors(false);
    QVERIFY(!view.alternatingRowColors());

    // ### Test visual effect.
}

void tst_QTreeView::currentIndex_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("indexRow");
    QTest::addColumn<int>("indexColumn");
    QTest::addColumn<int>("parentIndexRow");
    QTest::addColumn<int>("parentIndexColumn");

    QTest::newRow("-1, -1") << -1 << -1 << -1 << -1 << -1 << -1;
    QTest::newRow("-1, 0") << -1 << 0 << -1 << -1 << -1 << -1;
    QTest::newRow("0, -1") << 0 << -1 << -1 << -1 << -1 << -1;
    QTest::newRow("0, 0") << 0 << 0 << 0 << 0 << -1 << -1;
    QTest::newRow("0, 1") << 0 << 0 << 0 << 0 << -1 << -1;
    QTest::newRow("1, 0") << 1 << 0 << 1 << 0 << -1 << -1;
    QTest::newRow("1, 1") << 1 << 1 << -1 << -1 << -1 << -1;
    QTest::newRow("2, 0") << 2 << 0 << 2 << 0 << -1 << -1;
    QTest::newRow("2, 1") << 2 << 1 << -1 << -1 << -1 << -1;
    QTest::newRow("3, -1") << 3 << -1 << -1 << -1 << -1 << -1;
    QTest::newRow("3, 0") << 3 << 0 << -1 << -1 << -1 << -1;
    QTest::newRow("3, 1") << 3 << 1 << -1 << -1 << -1 << -1;
}

void tst_QTreeView::currentIndex()
{
    QFETCH(int, row);
    QFETCH(int, column);
    QFETCH(int, indexRow);
    QFETCH(int, indexColumn);
    QFETCH(int, parentIndexRow);
    QFETCH(int, parentIndexColumn);

    QTreeView view;
    QStandardItemModel treeModel;
    initStandardTreeModel(&treeModel);
    view.setModel(&treeModel);

    QCOMPARE(view.currentIndex(), QModelIndex());
    view.setCurrentIndex(view.model()->index(row, column));
    QCOMPARE(view.currentIndex().row(), indexRow);
    QCOMPARE(view.currentIndex().column(), indexColumn);
    QCOMPARE(view.currentIndex().parent().row(), parentIndexRow);
    QCOMPARE(view.currentIndex().parent().column(), parentIndexColumn);

    // ### Test child and grandChild indexes.
}

#if QT_CONFIG(draganddrop)

void tst_QTreeView::dragDropMode_data()
{
    QTest::addColumn<QAbstractItemView::DragDropMode>("dragDropMode");
    QTest::addColumn<bool>("acceptDrops");
    QTest::addColumn<bool>("dragEnabled");
    QTest::newRow("NoDragDrop") << QAbstractItemView::NoDragDrop << false << false;
    QTest::newRow("DragOnly") << QAbstractItemView::DragOnly << false << true;
    QTest::newRow("DropOnly") << QAbstractItemView::DropOnly << true << false;
    QTest::newRow("DragDrop") << QAbstractItemView::DragDrop << true << true;
    QTest::newRow("InternalMove") << QAbstractItemView::InternalMove << true << true;
}

void tst_QTreeView::dragDropMode()
{
    QFETCH(QAbstractItemView::DragDropMode, dragDropMode);
    QFETCH(bool, acceptDrops);
    QFETCH(bool, dragEnabled);

    QTreeView view;
    QCOMPARE(view.dragDropMode(), QAbstractItemView::NoDragDrop);
    QVERIFY(!view.acceptDrops());
    QVERIFY(!view.dragEnabled());

    view.setDragDropMode(dragDropMode);
    QCOMPARE(view.dragDropMode(), dragDropMode);
    QCOMPARE(view.acceptDrops(), acceptDrops);
    QCOMPARE(view.dragEnabled(), dragEnabled);

    // ### Test effects of this mode
}

void tst_QTreeView::dragDropModeFromDragEnabledAndAcceptDrops_data()
{
    QTest::addColumn<bool>("dragEnabled");
    QTest::addColumn<bool>("acceptDrops");
    QTest::addColumn<QAbstractItemView::DragDropMode>("dragDropMode");
    QTest::addColumn<bool>("setBehavior");
    QTest::addColumn<QAbstractItemView::DragDropMode>("behavior");

    QTest::newRow("NoDragDrop -1") << false << false << QAbstractItemView::NoDragDrop << false << QAbstractItemView::DragDropMode();
    QTest::newRow("NoDragDrop 0") << false << false << QAbstractItemView::NoDragDrop << true << QAbstractItemView::NoDragDrop;
    QTest::newRow("NoDragDrop 1") << false << false << QAbstractItemView::NoDragDrop << true << QAbstractItemView::DragOnly;
    QTest::newRow("NoDragDrop 2") << false << false << QAbstractItemView::NoDragDrop << true << QAbstractItemView::DropOnly;
    QTest::newRow("NoDragDrop 3") << false << false << QAbstractItemView::NoDragDrop << true << QAbstractItemView::DragDrop;
    QTest::newRow("NoDragDrop 4") << false << false << QAbstractItemView::NoDragDrop << true << QAbstractItemView::InternalMove;
    QTest::newRow("DragOnly -1") << true << false << QAbstractItemView::DragOnly << false << QAbstractItemView::DragDropMode();
    QTest::newRow("DragOnly 0") << true << false << QAbstractItemView::DragOnly << true << QAbstractItemView::NoDragDrop;
    QTest::newRow("DragOnly 1") << true << false << QAbstractItemView::DragOnly << true << QAbstractItemView::DragOnly;
    QTest::newRow("DragOnly 2") << true << false << QAbstractItemView::DragOnly << true << QAbstractItemView::DropOnly;
    QTest::newRow("DragOnly 3") << true << false << QAbstractItemView::DragOnly << true << QAbstractItemView::DragDrop;
    QTest::newRow("DragOnly 4") << true << false << QAbstractItemView::DragOnly << true << QAbstractItemView::InternalMove;
    QTest::newRow("DropOnly -1") << false << true << QAbstractItemView::DropOnly << false << QAbstractItemView::DragDropMode();
    QTest::newRow("DropOnly 0") << false << true << QAbstractItemView::DropOnly << true << QAbstractItemView::NoDragDrop;
    QTest::newRow("DropOnly 1") << false << true << QAbstractItemView::DropOnly << true << QAbstractItemView::DragOnly;
    QTest::newRow("DropOnly 2") << false << true << QAbstractItemView::DropOnly << true << QAbstractItemView::DropOnly;
    QTest::newRow("DropOnly 3") << false << true << QAbstractItemView::DropOnly << true << QAbstractItemView::DragDrop;
    QTest::newRow("DropOnly 4") << false << true << QAbstractItemView::DropOnly << true << QAbstractItemView::InternalMove;
    QTest::newRow("DragDrop -1") << true << true << QAbstractItemView::DragDrop << false << QAbstractItemView::DragDropMode();
    QTest::newRow("DragDrop 0") << true << true << QAbstractItemView::DragDrop << false << QAbstractItemView::DragDropMode();
    QTest::newRow("DragDrop 1") << true << true << QAbstractItemView::DragDrop << true << QAbstractItemView::NoDragDrop;
    QTest::newRow("DragDrop 2") << true << true << QAbstractItemView::DragDrop << true << QAbstractItemView::DragOnly;
    QTest::newRow("DragDrop 3") << true << true << QAbstractItemView::DragDrop << true << QAbstractItemView::DropOnly;
    QTest::newRow("DragDrop 4") << true << true << QAbstractItemView::DragDrop << true << QAbstractItemView::DragDrop;
    QTest::newRow("DragDrop 5") << true << true << QAbstractItemView::InternalMove << true << QAbstractItemView::InternalMove;
}

void tst_QTreeView::dragDropModeFromDragEnabledAndAcceptDrops()
{
    QFETCH(bool, acceptDrops);
    QFETCH(bool, dragEnabled);
    QFETCH(QAbstractItemView::DragDropMode, dragDropMode);
    QFETCH(bool, setBehavior);
    QFETCH(QAbstractItemView::DragDropMode, behavior);

    QTreeView view;
    QCOMPARE(view.dragDropMode(), QAbstractItemView::NoDragDrop);

    if (setBehavior)
        view.setDragDropMode(behavior);

    view.setAcceptDrops(acceptDrops);
    view.setDragEnabled(dragEnabled);
    QCOMPARE(view.dragDropMode(), dragDropMode);

    // ### Test effects of this mode
}

void tst_QTreeView::dragDropOverwriteMode()
{
    QTreeView view;
    QVERIFY(!view.dragDropOverwriteMode());
    view.setDragDropOverwriteMode(true);
    QVERIFY(view.dragDropOverwriteMode());
    view.setDragDropOverwriteMode(false);
    QVERIFY(!view.dragDropOverwriteMode());

    // ### This property changes the behavior of dropIndicatorPosition(),
    // which is protected and called only from within QListWidget and
    // QTableWidget, from their reimplementations of dropMimeData(). Hard to
    // test.
}
#endif

void tst_QTreeView::editTriggers_data()
{
    QTest::addColumn<QAbstractItemView::EditTriggers>("editTriggers");
    QTest::addColumn<QAbstractItemView::EditTrigger>("triggeredTrigger");
    QTest::addColumn<bool>("editorOpened");

    // NoEditTriggers
    QTest::newRow("NoEditTriggers 0") << QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers)
                                      << QAbstractItemView::NoEditTriggers << false;
    QTest::newRow("NoEditTriggers 1") << QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers)
                                      << QAbstractItemView::CurrentChanged << false;
    QTest::newRow("NoEditTriggers 2") << QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers)
                                      << QAbstractItemView::DoubleClicked << false;
    QTest::newRow("NoEditTriggers 3") << QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers)
                                      << QAbstractItemView::SelectedClicked << false;
    QTest::newRow("NoEditTriggers 4") << QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers)
                                      << QAbstractItemView::EditKeyPressed << false;

    // CurrentChanged
    QTest::newRow("CurrentChanged 0") << QAbstractItemView::EditTriggers(QAbstractItemView::CurrentChanged)
                                      << QAbstractItemView::NoEditTriggers << false;
    QTest::newRow("CurrentChanged 1") << QAbstractItemView::EditTriggers(QAbstractItemView::CurrentChanged)
                                      << QAbstractItemView::CurrentChanged << true;
    QTest::newRow("CurrentChanged 2") << QAbstractItemView::EditTriggers(QAbstractItemView::CurrentChanged)
                                      << QAbstractItemView::DoubleClicked << false;
    QTest::newRow("CurrentChanged 3") << QAbstractItemView::EditTriggers(QAbstractItemView::CurrentChanged)
                                      << QAbstractItemView::SelectedClicked << false;
    QTest::newRow("CurrentChanged 4") << QAbstractItemView::EditTriggers(QAbstractItemView::CurrentChanged)
                                      << QAbstractItemView::EditKeyPressed << false;

    // DoubleClicked
    QTest::newRow("DoubleClicked 0") << QAbstractItemView::EditTriggers(QAbstractItemView::DoubleClicked)
                                     << QAbstractItemView::NoEditTriggers << false;
    QTest::newRow("DoubleClicked 1") << QAbstractItemView::EditTriggers(QAbstractItemView::DoubleClicked)
                                     << QAbstractItemView::CurrentChanged << false;
    QTest::newRow("DoubleClicked 2") << QAbstractItemView::EditTriggers(QAbstractItemView::DoubleClicked)
                                     << QAbstractItemView::DoubleClicked << true;
    QTest::newRow("DoubleClicked 3") << QAbstractItemView::EditTriggers(QAbstractItemView::DoubleClicked)
                                     << QAbstractItemView::SelectedClicked << false;
    QTest::newRow("DoubleClicked 4") << QAbstractItemView::EditTriggers(QAbstractItemView::DoubleClicked)
                                     << QAbstractItemView::EditKeyPressed << false;

    // SelectedClicked
    QTest::newRow("SelectedClicked 0") << QAbstractItemView::EditTriggers(QAbstractItemView::SelectedClicked)
                                       << QAbstractItemView::NoEditTriggers << false;
    QTest::newRow("SelectedClicked 1") << QAbstractItemView::EditTriggers(QAbstractItemView::SelectedClicked)
                                       << QAbstractItemView::CurrentChanged << false;
    QTest::newRow("SelectedClicked 2") << QAbstractItemView::EditTriggers(QAbstractItemView::SelectedClicked)
                                       << QAbstractItemView::DoubleClicked << false;
    QTest::newRow("SelectedClicked 3") << QAbstractItemView::EditTriggers(QAbstractItemView::SelectedClicked)
                                       << QAbstractItemView::SelectedClicked << true;
    QTest::newRow("SelectedClicked 4") << QAbstractItemView::EditTriggers(QAbstractItemView::SelectedClicked)
                                       << QAbstractItemView::EditKeyPressed << false;

    // EditKeyPressed
    QTest::newRow("EditKeyPressed 0") << QAbstractItemView::EditTriggers(QAbstractItemView::EditKeyPressed)
                                      << QAbstractItemView::NoEditTriggers << false;
    QTest::newRow("EditKeyPressed 1") << QAbstractItemView::EditTriggers(QAbstractItemView::EditKeyPressed)
                                      << QAbstractItemView::CurrentChanged << false;
    QTest::newRow("EditKeyPressed 2") << QAbstractItemView::EditTriggers(QAbstractItemView::EditKeyPressed)
                                      << QAbstractItemView::DoubleClicked << false;
    QTest::newRow("EditKeyPressed 3") << QAbstractItemView::EditTriggers(QAbstractItemView::EditKeyPressed)
                                      << QAbstractItemView::SelectedClicked << false;
    QTest::newRow("EditKeyPressed 4") << QAbstractItemView::EditTriggers(QAbstractItemView::EditKeyPressed)
                                      << QAbstractItemView::EditKeyPressed << true;
}

void tst_QTreeView::editTriggers()
{
    QFETCH(QAbstractItemView::EditTriggers, editTriggers);
    QFETCH(QAbstractItemView::EditTrigger, triggeredTrigger);
    QFETCH(bool, editorOpened);

    QTreeView view;
    QStandardItemModel treeModel;
    initStandardTreeModel(&treeModel);
    view.setModel(&treeModel);
    view.show();

    QCOMPARE(view.editTriggers(), QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);

    // Initialize the first index
    view.setCurrentIndex(view.model()->index(0, 0));

    // Verify that we don't have any editor initially
    QVERIFY(!view.findChild<QLineEdit *>(QString()));

    // Set the triggers
    view.setEditTriggers(editTriggers);

    // Interact with the view
    switch (triggeredTrigger) {
    case QAbstractItemView::NoEditTriggers:
        // Do nothing, the editor shouldn't be there
        break;
    case QAbstractItemView::CurrentChanged:
        // Change the index to open an editor
        view.setCurrentIndex(view.model()->index(1, 0));
        break;
    case QAbstractItemView::DoubleClicked:
        // Doubleclick the center of the current cell
        QTest::mouseClick(view.viewport(), Qt::LeftButton, {},
                          view.visualRect(view.model()->index(0, 0)).center());
        QTest::mouseDClick(view.viewport(), Qt::LeftButton, {},
                           view.visualRect(view.model()->index(0, 0)).center());
        break;
    case QAbstractItemView::SelectedClicked:
        // Click the center of the current cell
        view.selectionModel()->select(view.model()->index(0, 0), QItemSelectionModel::Select);
        QTest::mouseClick(view.viewport(), Qt::LeftButton, {},
                          view.visualRect(view.model()->index(0, 0)).center());
        QTest::qWait(qRound(QApplication::doubleClickInterval() * 1.5));
        break;
    case QAbstractItemView::EditKeyPressed:
        view.setFocus();
#ifdef Q_OS_MAC
        // OS X uses Enter for editing
        QTest::keyPress(&view, Qt::Key_Enter);
#else
        // All other platforms use F2
        QTest::keyPress(&view, Qt::Key_F2);
#endif
        break;
    default:
        break;
    }

    // Check if we got an editor
    QTRY_COMPARE(view.findChild<QLineEdit *>(QString()) != nullptr, editorOpened);
}

void tst_QTreeView::hasAutoScroll()
{
    QTreeView view;
    QVERIFY(view.hasAutoScroll());
    view.setAutoScroll(false);
    QVERIFY(!view.hasAutoScroll());
    view.setAutoScroll(true);
    QVERIFY(view.hasAutoScroll());
}

void tst_QTreeView::horizontalScrollMode()
{
    QStandardItemModel model;
    for (int i = 0; i < 100; ++i) {
        model.appendRow(QList<QStandardItem *>()
                        << new QStandardItem("An item that has very long text and should"
                                             " cause the horizontal scroll bar to appear.")
                        << new QStandardItem("An item that has very long text and should"
                                             " cause the horizontal scroll bar to appear."));
    }

    QTreeView view;
    setFrameless(&view);
    view.setModel(&model);
    view.setFixedSize(100, 100);
    view.header()->resizeSection(0, 200);
    view.show();

    QCOMPARE(view.horizontalScrollMode(), QAbstractItemView::ScrollPerPixel);
    QCOMPARE(view.horizontalScrollBar()->minimum(), 0);
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "setFixedSize does not work on WinRT - QTBUG-68297", Abort);
#endif
    QVERIFY(view.horizontalScrollBar()->maximum() > 2);

    view.setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);
    QCOMPARE(view.horizontalScrollMode(), QAbstractItemView::ScrollPerItem);
    QCOMPARE(view.horizontalScrollBar()->minimum(), 0);
    QCOMPARE(view.horizontalScrollBar()->maximum(), 1);

    view.setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    QCOMPARE(view.horizontalScrollMode(), QAbstractItemView::ScrollPerPixel);
    QCOMPARE(view.horizontalScrollBar()->minimum(), 0);
    QVERIFY(view.horizontalScrollBar()->maximum() > 2);
}

class RepaintTreeView : public QTreeView
{
public:
    using QTreeView::QTreeView;
    bool repainted = false;

protected:
    void paintEvent(QPaintEvent *event) override
    { repainted = true; QTreeView::paintEvent(event); }
};

void tst_QTreeView::iconSize()
{
    RepaintTreeView view;
    QCOMPARE(view.iconSize(), QSize());

    QStandardItemModel treeModel;
    initStandardTreeModel(&treeModel);
    view.setModel(&treeModel);
    QCOMPARE(view.iconSize(), QSize());
    QVERIFY(!view.repainted);

    view.show();
    view.update();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(view.repainted);
    QCOMPARE(view.iconSize(), QSize());

    view.repainted = false;
    view.setIconSize(QSize());
    QTRY_VERIFY(!view.repainted);
    QCOMPARE(view.iconSize(), QSize());

    view.setIconSize(QSize(10, 10));
    QTRY_VERIFY(view.repainted);
    QCOMPARE(view.iconSize(), QSize(10, 10));

    view.repainted = false;
    view.setIconSize(QSize(10000, 10000));
    QTRY_VERIFY(view.repainted);
    QCOMPARE(view.iconSize(), QSize(10000, 10000));
}

void tst_QTreeView::indexAt()
{
    QtTestModel model(5, 5);

    QTreeView view;
    QCOMPARE(view.indexAt(QPoint()), QModelIndex());
    view.setModel(&model);
    QVERIFY(view.indexAt(QPoint()) != QModelIndex());

    QSize itemSize = view.visualRect(model.index(0, 0)).size();
    for (int i = 0; i < model.rowCount(); ++i) {
        QPoint pos(itemSize.width() / 2, (i * itemSize.height()) + (itemSize.height() / 2));
        QModelIndex index = view.indexAt(pos);
        QCOMPARE(index, model.index(i, 0));
    }

    /*
      // ### this is wrong; the widget _will_ affect the item size
    for (int j = 0; j < model.rowCount(); ++j)
        view.setIndexWidget(model.index(j, 0), new QPushButton);
    */

    for (int k = 0; k < model.rowCount(); ++k) {
        QPoint pos(itemSize.width() / 2, (k * itemSize.height()) + (itemSize.height() / 2));
        QModelIndex index = view.indexAt(pos);
        QCOMPARE(index, model.index(k, 0));
    }

    view.show();
    view.resize(600, 600);
    view.header()->setStretchLastSection(false);
    QCOMPARE(view.indexAt(QPoint(550, 20)), QModelIndex());
}

void tst_QTreeView::indexWidget()
{
    QTreeView view;
    QStandardItemModel treeModel;
    initStandardTreeModel(&treeModel);
    view.setModel(&treeModel);
    view.resize(300, 400);  // make sure the width of the view is larger than the widgets below

    QModelIndex index = view.model()->index(0, 0);

    QVERIFY(!view.indexWidget(QModelIndex()));
    QVERIFY(!view.indexWidget(index));

    QString text = QLatin1String("TestLabel");

    QWidget *label = new QLabel(text);
    view.setIndexWidget(QModelIndex(), label);
    QVERIFY(!view.indexWidget(QModelIndex()));
    QVERIFY(!label->parent());
    view.setIndexWidget(index, label);
    QCOMPARE(view.indexWidget(index), label);
    QCOMPARE(label->parentWidget(), view.viewport());


    QTextEdit *widget = new QTextEdit(text);
    widget->setFixedSize(200,100);
    view.setIndexWidget(index, widget);
    QCOMPARE(view.indexWidget(index), static_cast<QWidget *>(widget));

    QCOMPARE(widget->parentWidget(), view.viewport());
    QCOMPARE(widget->geometry(), view.visualRect(index).intersected(widget->geometry()));
    QCOMPARE(widget->toPlainText(), text);

    //now let's try to do that later when the widget is already shown
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    index = view.model()->index(1, 0);
    QVERIFY(!view.indexWidget(index));

    widget = new QTextEdit(text);
    widget->setFixedSize(200,100);
    view.setIndexWidget(index, widget);
    QCOMPARE(view.indexWidget(index), static_cast<QWidget *>(widget));

    QCOMPARE(widget->parentWidget(), view.viewport());
    QCOMPARE(widget->geometry(), view.visualRect(index).intersected(widget->geometry()));
    QCOMPARE(widget->toPlainText(), text);
}

void tst_QTreeView::itemDelegate()
{
    QPointer<QAbstractItemDelegate> oldDelegate;
    QPointer<QStyledItemDelegate> otherItemDelegate;

    {
        QTreeView view;
        QVERIFY(qobject_cast<QStyledItemDelegate *>(view.itemDelegate()));
        QPointer<QAbstractItemDelegate> oldDelegate = view.itemDelegate();

        otherItemDelegate = new QStyledItemDelegate;
        view.setItemDelegate(otherItemDelegate);
        QVERIFY(!otherItemDelegate->parent());
        QVERIFY(oldDelegate);

        QCOMPARE(view.itemDelegate(), otherItemDelegate);

        view.setItemDelegate(nullptr);
        QVERIFY(!view.itemDelegate()); // <- view does its own drawing?
        QVERIFY(otherItemDelegate);
    }

    // This is strange. Why doesn't setItemDelegate() reparent the delegate?
    QVERIFY(!oldDelegate);
    QVERIFY(otherItemDelegate);

    delete otherItemDelegate;
}

void tst_QTreeView::itemDelegateForColumnOrRow()
{
    QTreeView view;
    QAbstractItemDelegate *defaultDelegate = view.itemDelegate();
    QVERIFY(defaultDelegate);

    QVERIFY(!view.itemDelegateForRow(0));
    QVERIFY(!view.itemDelegateForColumn(0));
    QCOMPARE(view.itemDelegate(QModelIndex()), defaultDelegate);

    QStandardItemModel model;
    for (int i = 0; i < 100; ++i) {
        model.appendRow(QList<QStandardItem *>()
                        << new QStandardItem("An item that has very long text and should"
                                             " cause the horizontal scroll bar to appear.")
                        << new QStandardItem("An item that has very long text and should"
                                             " cause the horizontal scroll bar to appear.")
                        << new QStandardItem("An item that has very long text and should"
                                             " cause the horizontal scroll bar to appear."));
    }
    view.setModel(&model);

    QVERIFY(!view.itemDelegateForRow(0));
    QVERIFY(!view.itemDelegateForColumn(0));
    QCOMPARE(view.itemDelegate(QModelIndex()), defaultDelegate);
    QCOMPARE(view.itemDelegate(view.model()->index(0, 0)), defaultDelegate);

    QPointer<QAbstractItemDelegate> rowDelegate = new QStyledItemDelegate;
    view.setItemDelegateForRow(0, rowDelegate);
    QVERIFY(!rowDelegate->parent());
    QCOMPARE(view.itemDelegateForRow(0), rowDelegate);
    QCOMPARE(view.itemDelegate(view.model()->index(0, 0)), rowDelegate);
    QCOMPARE(view.itemDelegate(view.model()->index(0, 1)), rowDelegate);
    QCOMPARE(view.itemDelegate(view.model()->index(1, 0)), defaultDelegate);
    QCOMPARE(view.itemDelegate(view.model()->index(1, 1)), defaultDelegate);

    QPointer<QAbstractItemDelegate> columnDelegate = new QStyledItemDelegate;
    view.setItemDelegateForColumn(1, columnDelegate);
    QVERIFY(!columnDelegate->parent());
    QCOMPARE(view.itemDelegateForColumn(1), columnDelegate);
    QCOMPARE(view.itemDelegate(view.model()->index(0, 0)), rowDelegate);
    QCOMPARE(view.itemDelegate(view.model()->index(0, 1)), rowDelegate); // row wins
    QCOMPARE(view.itemDelegate(view.model()->index(1, 0)), defaultDelegate);
    QCOMPARE(view.itemDelegate(view.model()->index(1, 1)), columnDelegate);

    view.setItemDelegateForRow(0, nullptr);
    QVERIFY(!view.itemDelegateForRow(0));
    QVERIFY(rowDelegate); // <- wasn't deleted

    view.setItemDelegateForColumn(1, nullptr);
    QVERIFY(!view.itemDelegateForColumn(1));
    QVERIFY(columnDelegate); // <- wasn't deleted

    delete rowDelegate;
    delete columnDelegate;
}

void tst_QTreeView::keyboardSearch()
{
    QTreeView view;
    QStandardItemModel model;
    model.appendRow(new QStandardItem("Andreas"));
    model.appendRow(new QStandardItem("Baldrian"));
    model.appendRow(new QStandardItem("Cecilie"));
    view.setModel(&model);
    view.show();

    // Nothing is selected
    QVERIFY(!view.selectionModel()->hasSelection());
    QVERIFY(!view.selectionModel()->isSelected(model.index(0, 0)));

    // First item is selected
    view.keyboardSearch(QLatin1String("A"));
    QTRY_VERIFY(view.selectionModel()->isSelected(model.index(0, 0)));

    // First item is still selected
    view.keyboardSearch(QLatin1String("n"));
    QVERIFY(view.selectionModel()->isSelected(model.index(0, 0)));

    // No "AnB" item - keep the same selection.
    view.keyboardSearch(QLatin1String("B"));
    QVERIFY(view.selectionModel()->isSelected(model.index(0, 0)));

    // Wait a bit.
    QTest::qWait(QApplication::keyboardInputInterval() * 2);

    // The item that starts with B is selected.
    view.keyboardSearch(QLatin1String("B"));
    QVERIFY(view.selectionModel()->isSelected(model.index(1, 0)));

    // Test that it wraps round
    model.appendRow(new QStandardItem("Andy"));
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(3, 0)));
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(0, 0)));
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(3, 0)));

    // Test that it handles the case where the first item is hidden correctly
    model.insertRow(0, new QStandardItem("Hidden item"));
    view.setRowHidden(0, QModelIndex(), true);

    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(1, 0)));
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(4, 0)));
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(1, 0)));

    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    model.clear();
    view.setCurrentIndex(QModelIndex());
    model.appendRow({ new QStandardItem("Andreas"), new QStandardItem("Alicia") });
    model.appendRow({ new QStandardItem("Baldrian"), new QStandardItem("Belinda") });
    model.appendRow({ new QStandardItem("Cecilie"), new QStandardItem("Claire") });
    QVERIFY(!view.selectionModel()->hasSelection());
    QVERIFY(!view.selectionModel()->isSelected(model.index(0, 0)));

    // We want to search on the 2nd column so we have to force it to have
    // an index in that column as a starting point
    view.setCurrentIndex(QModelIndex(model.index(0, 1)));
    // Second item in first row is selected
    view.keyboardSearch(QLatin1String("A"));
    QTRY_VERIFY(view.selectionModel()->isSelected(model.index(0, 1)));
    QVERIFY(view.currentIndex() == model.index(0, 1));

    // Second item in first row is still selected
    view.keyboardSearch(QLatin1String("l"));
    QVERIFY(view.selectionModel()->isSelected(model.index(0, 1)));
    QCOMPARE(view.currentIndex(), model.index(0, 1));

    // No "AnB" item - keep the same selection.
    view.keyboardSearch(QLatin1String("B"));
    QVERIFY(view.selectionModel()->isSelected(model.index(0, 1)));
    QCOMPARE(view.currentIndex(), model.index(0, 1));

    // Wait a bit.
    QTest::qWait(QApplication::keyboardInputInterval() * 2);

    // The item that starts with B is selected.
    view.keyboardSearch(QLatin1String("B"));
    QVERIFY(view.selectionModel()->isSelected(model.index(1, 1)));
    QCOMPARE(view.currentIndex(), model.index(1, 1));

    // Test that it wraps round
    model.appendRow({ new QStandardItem("Andy"), new QStandardItem("Adele") });
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(3, 1)));
    QCOMPARE(view.currentIndex(), model.index(3, 1));
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(0, 1)));
    QCOMPARE(view.currentIndex(), model.index(0, 1));
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(3, 1)));
    QCOMPARE(view.currentIndex(), model.index(3, 1));

    // Test that it handles the case where the first item is hidden correctly
    model.insertRow(0, new QStandardItem("Hidden item"));
    view.setRowHidden(0, QModelIndex(), true);

    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(1, 1)));
    QCOMPARE(view.currentIndex(), model.index(1, 1));
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(4, 1)));
    QCOMPARE(view.currentIndex(), model.index(4, 1));
    QTest::qWait(QApplication::keyboardInputInterval() * 2);
    view.keyboardSearch(QLatin1String("A"));
    QVERIFY(view.selectionModel()->isSelected(model.index(1, 1)));
    QCOMPARE(view.currentIndex(), model.index(1, 1));
}

void tst_QTreeView::keyboardSearchMultiColumn()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QTreeView view;
    QStandardItemModel model(4, 2);

    model.setItem(0, 0, new QStandardItem("1"));    model.setItem(0, 1, new QStandardItem("green"));
    model.setItem(1, 0, new QStandardItem("bad"));  model.setItem(1, 1, new QStandardItem("eggs"));
    model.setItem(2, 0, new QStandardItem("moof")); model.setItem(2, 1, new QStandardItem("and"));
    model.setItem(3, 0, new QStandardItem("elf"));  model.setItem(3, 1, new QStandardItem("ham"));

    view.setModel(&model);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    view.setCurrentIndex(model.index(0, 1));

    // First item is selected
    view.keyboardSearch(QLatin1String("eggs"));
    QVERIFY(view.selectionModel()->isSelected(model.index(1, 1)));

    QTest::qWait(QApplication::keyboardInputInterval() * 2);

    // 'ham' is selected
    view.keyboardSearch(QLatin1String("h"));
    QVERIFY(view.selectionModel()->isSelected(model.index(3, 1)));
}

void tst_QTreeView::setModel()
{
    QTreeView view;
    view.show();
    QCOMPARE(view.model(), nullptr);
    QCOMPARE(view.selectionModel(), nullptr);
    QCOMPARE(view.header()->model(), nullptr);
    QCOMPARE(view.header()->selectionModel(), nullptr);

    for (int x = 0; x < 2; ++x) {
        QtTestModel *model = new QtTestModel(10, 8);
        QAbstractItemModel *oldModel = view.model();
        QSignalSpy modelDestroyedSpy(oldModel ? oldModel : model, &QObject::destroyed);
        // set the same model twice
        for (int i = 0; i < 2; ++i) {
            QItemSelectionModel *oldSelectionModel = view.selectionModel();
            QItemSelectionModel *dummy = new QItemSelectionModel(model);
            QSignalSpy selectionModelDestroyedSpy(
                oldSelectionModel ? oldSelectionModel : dummy, &QObject::destroyed);
            view.setModel(model);
//                QCOMPARE(selectionModelDestroyedSpy.count(), (x == 0 || i == 1) ? 0 : 1);
            QCOMPARE(view.model(), model);
            QCOMPARE(view.header()->model(), model);
            QCOMPARE(view.selectionModel() != oldSelectionModel, (i == 0));
        }
        QTRY_COMPARE(modelDestroyedSpy.count(), 0);

        view.setModel(nullptr);
        QCOMPARE(view.model(), nullptr);
        // ### shouldn't selectionModel also be 0 now?
//        QCOMPARE(view.selectionModel(), nullptr);
        delete model;
    }
}

void tst_QTreeView::openPersistentEditor()
{
    QTreeView view;
    QStandardItemModel treeModel;
    initStandardTreeModel(&treeModel);
    view.setModel(&treeModel);
    view.show();

    QVERIFY(!view.viewport()->findChild<QLineEdit *>());
    view.openPersistentEditor(view.model()->index(0, 0));
    QVERIFY(view.viewport()->findChild<QLineEdit *>());

    view.closePersistentEditor(view.model()->index(0, 0));
    QVERIFY(!view.viewport()->findChild<QLineEdit *>()->isVisible());

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!view.viewport()->findChild<QLineEdit *>());
}

void tst_QTreeView::rootIndex()
{
    QTreeView view;
    QCOMPARE(view.rootIndex(), QModelIndex());
    QStandardItemModel treeModel;
    initStandardTreeModel(&treeModel);
    view.setModel(&treeModel);
    QCOMPARE(view.rootIndex(), QModelIndex());

    view.setRootIndex(view.model()->index(1, 0));

    QCOMPARE(view.model()->data(view.model()->index(0, view.header()->visualIndex(0), view.rootIndex()), Qt::DisplayRole)
             .toString(), QString("Row 2 Child Item"));
}

void tst_QTreeView::setHeader()
{
    QTreeView view;
    QVERIFY(view.header() != nullptr);
    QCOMPARE(view.header()->orientation(), Qt::Horizontal);
    QCOMPARE(view.header()->parent(), &view);
    for (int x = 0; x < 2; ++x) {
        QSignalSpy destroyedSpy(view.header(), &QObject::destroyed);
        Qt::Orientation orient = x ? Qt::Vertical : Qt::Horizontal;
        QHeaderView *head = new QHeaderView(orient);
        view.setHeader(head);
        QCOMPARE(destroyedSpy.count(), 1);
        QCOMPARE(head->parent(), &view);
        QCOMPARE(view.header(), head);
        view.setHeader(head);
        QCOMPARE(view.header(), head);
        view.setHeader(nullptr);
        QCOMPARE(view.header(), head);
    }
}

void tst_QTreeView::columnHidden()
{
    QTreeView view;
    QtTestModel model(10, 8);
    view.setModel(&model);
    view.show();
    for (int c = 0; c < model.columnCount(); ++c)
        QCOMPARE(view.isColumnHidden(c), false);
    // hide even columns
    for (int c = 0; c < model.columnCount(); c += 2)
        view.setColumnHidden(c, true);
    for (int c = 0; c < model.columnCount(); ++c)
        QCOMPARE(view.isColumnHidden(c), (c & 1) == 0);
    view.update();
    // hide odd columns too
    for (int c = 1; c < model.columnCount(); c += 2)
        view.setColumnHidden(c, true);
    for (int c = 0; c < model.columnCount(); ++c)
        QCOMPARE(view.isColumnHidden(c), true);
    view.update();

    // QTBUG 54610
    // QAbstractItemViewPrivate::_q_layoutChanged() is called on
    // rows/columnMoved and because this function is virtual,
    // QHeaderViewPrivate::_q_layoutChanged() was called and unhided
    // all sections because QHeaderViewPrivate::_q_layoutAboutToBeChanged()
    // could not fill persistentHiddenSections (and is not needed)
    view.hideColumn(model.cols - 1);
    QCOMPARE(view.isColumnHidden(model.cols - 1), true);
    model.simulateMoveRows();
    QCOMPARE(view.isColumnHidden(model.cols - 1), true);
}

void tst_QTreeView::rowHidden()
{
    QtTestModel model(4, 6);
    model.levels = 3;
    QTreeView view;
    view.resize(500,500);
    view.setModel(&model);
    view.show();

    QCOMPARE(view.isRowHidden(-1, QModelIndex()), false);
    QCOMPARE(view.isRowHidden(999999, QModelIndex()), false);
    view.setRowHidden(-1, QModelIndex(), true);
    view.setRowHidden(999999, QModelIndex(), true);
    QCOMPARE(view.isRowHidden(-1, QModelIndex()), false);
    QCOMPARE(view.isRowHidden(999999, QModelIndex()), false);

    view.setRowHidden(0, QModelIndex(), true);
    QCOMPARE(view.isRowHidden(0, QModelIndex()), true);
    view.setRowHidden(0, QModelIndex(), false);
    QCOMPARE(view.isRowHidden(0, QModelIndex()), false);

    QStack<QModelIndex> parents;
    parents.push(QModelIndex());
    while (!parents.isEmpty()) {
        QModelIndex p = parents.pop();
        if (model.canFetchMore(p))
            model.fetchMore(p);
        int rows = model.rowCount(p);
        // hide all
        for (int r = 0; r < rows; ++r) {
            view.setRowHidden(r, p, true);
            QCOMPARE(view.isRowHidden(r, p), true);
        }
        // hide none
        for (int r = 0; r < rows; ++r) {
            view.setRowHidden(r, p, false);
            QCOMPARE(view.isRowHidden(r, p), false);
        }
        // hide only even rows
        for (int r = 0; r < rows; ++r) {
            bool hide = (r & 1) == 0;
            view.setRowHidden(r, p, hide);
            QCOMPARE(view.isRowHidden(r, p), hide);
        }
        for (int r = 0; r < rows; ++r)
            parents.push(model.index(r, 0, p));
    }

    parents.push(QModelIndex());
    while (!parents.isEmpty()) {
        QModelIndex p = parents.pop();
        // all even rows should still be hidden
        for (int r = 0; r < model.rowCount(p); ++r)
            QCOMPARE(view.isRowHidden(r, p), (r & 1) == 0);
        if (model.rowCount(p) > 0) {
            for (int r = 0; r < model.rowCount(p); ++r)
                parents.push(model.index(r, 0, p));
        }
    }
}

void tst_QTreeView::noDelegate()
{
    QtTestModel model(10, 7);
    QTreeView view;
    view.setModel(&model);
    view.setItemDelegate(nullptr);
    QCOMPARE(view.itemDelegate(), nullptr);
}

void tst_QTreeView::noModel()
{
    QTreeView view;
    view.show();
    view.setRowHidden(0, QModelIndex(), true);
    // no model -> not able to hide a row
    QVERIFY(!view.isRowHidden(0, QModelIndex()));
}

void tst_QTreeView::emptyModel()
{
    QtTestModel model(0, 0);
    QTreeView view;
    view.setModel(&model);
    view.show();
    QVERIFY(!model.wrongIndex);
}

void tst_QTreeView::removeRows()
{
    QtTestModel model(7, 10);

    QTreeView view;

    view.setModel(&model);
    view.show();

    model.removeLastRow();
    QVERIFY(!model.wrongIndex);

    model.removeAllRows();
    QVERIFY(!model.wrongIndex);
}

void tst_QTreeView::removeCols()
{
    QtTestModel model(5, 8);

    QTreeView view;
    view.setModel(&model);
    view.show();
    model.fetched = true;
    model.removeLastColumn();
    QVERIFY(!model.wrongIndex);
    QCOMPARE(view.header()->count(), model.cols);

    model.removeAllColumns();
    QVERIFY(!model.wrongIndex);
    QCOMPARE(view.header()->count(), model.cols);
}

void tst_QTreeView::limitedExpand()
{
    {
        QStandardItemModel model;
        QStandardItem *parentItem = model.invisibleRootItem();
        parentItem->appendRow(new QStandardItem);
        parentItem->appendRow(new QStandardItem);
        parentItem->appendRow(new QStandardItem);

        QStandardItem *firstItem = model.item(0, 0);
        firstItem->setFlags(firstItem->flags() | Qt::ItemNeverHasChildren);

        QTreeView view;
        view.setModel(&model);

        QSignalSpy spy(&view, &QTreeView::expanded);
        QVERIFY(spy.isValid());

        view.expand(model.index(0, 0));
        QCOMPARE(spy.count(), 0);

        view.expand(model.index(1, 0));
        QCOMPARE(spy.count(), 1);
    }
    {
        QStringListModel model(QStringList() << "one" << "two");
        QTreeView view;
        view.setModel(&model);

        QSignalSpy spy(&view, &QTreeView::expanded);
        QVERIFY(spy.isValid());

        view.expand(model.index(0, 0));
        QCOMPARE(spy.count(), 0);
        view.expandAll();
        QCOMPARE(spy.count(), 0);
    }
}

void tst_QTreeView::expandAndCollapse_data()
{
    QTest::addColumn<bool>("animationEnabled");
    QTest::newRow("normal") << false;
    QTest::newRow("animated") << true;
}

void tst_QTreeView::expandAndCollapse()
{
    QFETCH(bool, animationEnabled);

    QtTestModel model(10, 9);

    QTreeView view;
    view.setUniformRowHeights(true);
    view.setModel(&model);
    view.setAnimated(animationEnabled);
    view.show();

    QModelIndex a = model.index(0, 0, QModelIndex());
    QModelIndex b = model.index(0, 0, a);

    QSignalSpy expandedSpy(&view, &QTreeView::expanded);
    QSignalSpy collapsedSpy(&view, &QTreeView::collapsed);
    QVariantList args;

    for (int y = 0; y < 2; ++y) {
        view.setVisible(y == 0);
        for (int x = 0; x < 2; ++x) {
            view.setItemsExpandable(x == 0);

            // Test bad args
            view.expand(QModelIndex());
            QCOMPARE(view.isExpanded(QModelIndex()), false);
            view.collapse(QModelIndex());
            QCOMPARE(expandedSpy.count(), 0);
            QCOMPARE(collapsedSpy.count(), 0);

            // expand a first level item
            QVERIFY(!view.isExpanded(a));
            view.expand(a);
            QVERIFY(view.isExpanded(a));
            QCOMPARE(expandedSpy.count(), 1);
            QCOMPARE(collapsedSpy.count(), 0);
            args = expandedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), a);

            view.expand(a);
            QVERIFY(view.isExpanded(a));
            QCOMPARE(expandedSpy.count(), 0);
            QCOMPARE(collapsedSpy.count(), 0);

            // expand a second level item
            QVERIFY(!view.isExpanded(b));
            view.expand(b);
            QVERIFY(view.isExpanded(a));
            QVERIFY(view.isExpanded(b));
            QCOMPARE(expandedSpy.count(), 1);
            QCOMPARE(collapsedSpy.count(), 0);
            args = expandedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), b);

            view.expand(b);
            QVERIFY(view.isExpanded(b));
            QCOMPARE(expandedSpy.count(), 0);
            QCOMPARE(collapsedSpy.count(), 0);

            // collapse the first level item
            view.collapse(a);
            QVERIFY(!view.isExpanded(a));
            QVERIFY(view.isExpanded(b));
            QCOMPARE(expandedSpy.count(), 0);
            QCOMPARE(collapsedSpy.count(), 1);
            args = collapsedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), a);

            view.collapse(a);
            QVERIFY(!view.isExpanded(a));
            QCOMPARE(expandedSpy.count(), 0);
            QCOMPARE(collapsedSpy.count(), 0);

            // expand the first level item again
            view.expand(a);
            QVERIFY(view.isExpanded(a));
            QVERIFY(view.isExpanded(b));
            QCOMPARE(expandedSpy.count(), 1);
            QCOMPARE(collapsedSpy.count(), 0);
            args = expandedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), a);

            // collapse the second level item
            view.collapse(b);
            QVERIFY(view.isExpanded(a));
            QVERIFY(!view.isExpanded(b));
            QCOMPARE(expandedSpy.count(), 0);
            QCOMPARE(collapsedSpy.count(), 1);
            args = collapsedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), b);

            // collapse the first level item
            view.collapse(a);
            QVERIFY(!view.isExpanded(a));
            QVERIFY(!view.isExpanded(b));
            QCOMPARE(expandedSpy.count(), 0);
            QCOMPARE(collapsedSpy.count(), 1);
            args = collapsedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), a);

            // expand and remove row
            QPersistentModelIndex c = model.index(9, 0, b);
            view.expand(a);
            view.expand(b);
            model.removeLastRow(); // remove c
            QVERIFY(view.isExpanded(a));
            QVERIFY(view.isExpanded(b));
            QVERIFY(!view.isExpanded(c));
            QCOMPARE(expandedSpy.count(), 2);
            QCOMPARE(collapsedSpy.count(), 0);
            args = expandedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), a);
            args = expandedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), b);

            view.collapse(a);
            view.collapse(b);
            QVERIFY(!view.isExpanded(a));
            QVERIFY(!view.isExpanded(b));
            QVERIFY(!view.isExpanded(c));
            QCOMPARE(expandedSpy.count(), 0);
            QCOMPARE(collapsedSpy.count(), 2);
            args = collapsedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), a);
            args = collapsedSpy.takeFirst();
            QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), b);
        }
    }
}

static void checkExpandState(const QAbstractItemModel &model, const QTreeView &view,
                             const QModelIndex &startIdx, bool bIsExpanded, int *count)
{
    *count = 0;
    QStack<QModelIndex> parents;
    parents.push(startIdx);
    if (startIdx.isValid()) {
        QCOMPARE(view.isExpanded(startIdx), bIsExpanded);
        *count += 1;
    }
    while (!parents.isEmpty()) {
        const QModelIndex p = parents.pop();
        const int rows = model.rowCount(p);
        for (int r = 0; r < rows; ++r) {
            const QModelIndex c = model.index(r, 0, p);
            QCOMPARE(view.isExpanded(c), bIsExpanded);
            parents.push(c);
        }
        *count += rows;
    }
}

void tst_QTreeView::expandAndCollapseAll()
{
    QStandardItemModel model;
    // QtTestModel has a broken parent/child handling which will break the test
    for (int i1 = 0; i1 < 3; ++i1) {
        QStandardItem *s1 = new QStandardItem;
        s1->setText(QString::number(i1));
        model.appendRow(s1);
        for (int i2 = 0; i2 < 3; ++i2) {
            QStandardItem *s2 = new QStandardItem;
            s2->setText(QStringLiteral("%1 - %2").arg(i1).arg(i2));
            s1->appendRow(s2);
            for (int i3 = 0; i3 < 3; ++i3) {
                QStandardItem *s3 = new QStandardItem;
                s3->setText(QStringLiteral("%1 - %2 - %3").arg(i1).arg(i2).arg(i3));
                s2->appendRow(s3);
            }
        }
    }
    QTreeView view;
    view.setUniformRowHeights(true);
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QSignalSpy expandedSpy(&view, &QTreeView::expanded);
    QSignalSpy collapsedSpy(&view, &QTreeView::collapsed);
    int count;

    view.expandAll();
    checkExpandState(model, view, QModelIndex(), true, &count);
    QCOMPARE(collapsedSpy.count(), 0);
    QCOMPARE(expandedSpy.count(),  39); // == 3 (first) + 9 (second) + 27 (third level)
    QCOMPARE(count, 39);

    collapsedSpy.clear();
    expandedSpy.clear();
    view.collapseAll();
    checkExpandState(model, view, QModelIndex(), false, &count);
    QCOMPARE(collapsedSpy.count(), 39);
    QCOMPARE(expandedSpy.count(), 0);
    QCOMPARE(count, 39);

    collapsedSpy.clear();
    expandedSpy.clear();
    view.expandRecursively(model.index(0, 0));
    QCOMPARE(expandedSpy.count(), 13); // 1 + 3 + 9

    checkExpandState(model, view, model.index(0, 0), true, &count);
    QCOMPARE(count, 13);
    checkExpandState(model, view, model.index(1, 0), false, &count);
    QCOMPARE(count, 13);
    checkExpandState(model, view, model.index(2, 0), false, &count);
    QCOMPARE(count, 13);

    expandedSpy.clear();
    view.collapseAll();
    view.expandRecursively(model.index(0, 0), 1);
    QCOMPARE(expandedSpy.count(), 4); // 1 + 3
    view.expandRecursively(model.index(0, 0), 2);
    QCOMPARE(expandedSpy.count(), 13); // (1 + 3) + 9

    checkExpandState(model, view, model.index(0, 0), true, &count);
    QCOMPARE(count, 13);
    checkExpandState(model, view, model.index(1, 0), false, &count);
    QCOMPARE(count, 13);
    checkExpandState(model, view, model.index(2, 0), false, &count);
    QCOMPARE(count, 13);
}

void tst_QTreeView::expandWithNoChildren()
{
    QTreeView tree;
    QStandardItemModel model(1, 1);
    tree.setModel(&model);
    tree.setAnimated(true);
    tree.doItemsLayout();
    //this test should not output warnings
    tree.expand(model.index(0,0));
}



void tst_QTreeView::keyboardNavigation()
{
    const int rows = 10;
    const int columns = 7;

    QtTestModel model(rows, columns);

    QTreeView view;
    view.setModel(&model);
    view.show();

    const auto keymoves = {
        Qt::Key_Down, Qt::Key_Right, Qt::Key_Right, Qt::Key_Right,
        Qt::Key_Down, Qt::Key_Down, Qt::Key_Down, Qt::Key_Down,
        Qt::Key_Right, Qt::Key_Right, Qt::Key_Right,
        Qt::Key_Left, Qt::Key_Up, Qt::Key_Left, Qt::Key_Left,
        Qt::Key_Up, Qt::Key_Down, Qt::Key_Up, Qt::Key_Up,
        Qt::Key_Up, Qt::Key_Up, Qt::Key_Up, Qt::Key_Up,
        Qt::Key_Left, Qt::Key_Left, Qt::Key_Up, Qt::Key_Down
    };

    int row    = 0;
    int column = 0;
    QModelIndex index = model.index(row, column, QModelIndex());
    view.setCurrentIndex(index);
    QCOMPARE(view.currentIndex(), index);

    for (Qt::Key key : keymoves) {
        QTest::keyClick(&view, key);

        switch (key) {
        case Qt::Key_Up:
            if (row > 0) {
                index = index.sibling(row - 1, column);
                row -= 1;
            } else if (index.parent() != QModelIndex()) {
                index = index.parent();
                row = index.row();
            }
            break;
        case Qt::Key_Down:
            if (view.isExpanded(index)) {
                row = 0;
                index = model.index(row, column, index);
            } else {
                row = qMin(rows - 1, row + 1);
                index = index.sibling(row, column);
            }
            break;
        case Qt::Key_Left: {
            QScrollBar *b = view.horizontalScrollBar();
            if (b->value() == b->minimum())
                QVERIFY(!view.isExpanded(index));
            // windows style right will walk to the parent
            if (view.currentIndex() != index) {
                QCOMPARE(view.currentIndex(), index.parent());
                index = view.currentIndex();
                row = index.row();
                column = index.column();
            }
            break;
        }
        case Qt::Key_Right:
            QVERIFY(view.isExpanded(index));
            // windows style right will walk to the first child
            if (view.currentIndex() != index) {
                QCOMPARE(view.currentIndex().parent(), index);
                row = view.currentIndex().row();
                column = view.currentIndex().column();
                index = view.currentIndex();
            }
            break;
        default:
            QVERIFY(false);
        }

        QCOMPARE(view.currentIndex().row(), row);
        QCOMPARE(view.currentIndex().column(), column);
        QCOMPARE(view.currentIndex(), index);
    }
}

class Dmodel : public QtTestModel
{
    Q_OBJECT
public:
    using QtTestModel::QtTestModel;
    int columnCount(const QModelIndex &parent) const override
    {
        if (parent.row() == 5)
            return 1;
        return QtTestModel::columnCount(parent);
    }
};

void tst_QTreeView::headerSections()
{
    Dmodel model(10, 10);

    QTreeView view;
    QHeaderView *header = view.header();

    view.setModel(&model);
    QModelIndex index = model.index(5, 0);
    view.setRootIndex(index);
    QCOMPARE(model.columnCount(QModelIndex()), 10);
    QCOMPARE(model.columnCount(index), 1);
    QCOMPARE(header->count(), model.columnCount(index));
}

void tst_QTreeView::moveCursor_data()
{
    QTest::addColumn<bool>("uniformRowHeights");
    QTest::addColumn<bool>("scrollPerPixel");
    QTest::newRow("uniformRowHeights = false, scrollPerPixel = false")
        << false << false;
    QTest::newRow("uniformRowHeights = true, scrollPerPixel = false")
        << true << false;
    QTest::newRow("uniformRowHeights = false, scrollPerPixel = true")
        << false << true;
    QTest::newRow("uniformRowHeights = true, scrollPerPixel = true")
        << true << true;
}

void tst_QTreeView::moveCursor()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QFETCH(bool, uniformRowHeights);
    QFETCH(bool, scrollPerPixel);
    QtTestModel model(8, 6);

    QTreeView view;
    view.setUniformRowHeights(uniformRowHeights);
    view.setModel(&model);
    view.setRowHidden(0, QModelIndex(), true);
    view.setVerticalScrollMode(scrollPerPixel ?
            QAbstractItemView::ScrollPerPixel :
            QAbstractItemView::ScrollPerItem);
    QVERIFY(view.isRowHidden(0, QModelIndex()));
    view.setColumnHidden(0, true);
    QVERIFY(view.isColumnHidden(0));
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    //here the first visible index should be selected
    //because the view got the focus
    QModelIndex expected = model.index(1, 1, QModelIndex());
    QCOMPARE(view.currentIndex(), expected);

    //then pressing down should go to the next line
    QModelIndex actual = view.moveCursor(QTreeView::MoveDown, Qt::NoModifier);
    expected = model.index(2, 1, QModelIndex());
    QCOMPARE(actual, expected);

    view.setRowHidden(0, QModelIndex(), false);
    view.setColumnHidden(0, false);

    // PageUp was broken with uniform row heights turned on
    view.setCurrentIndex(model.index(1, 0));
    actual = view.moveCursor(QTreeView::MovePageUp, Qt::NoModifier);
    expected = model.index(0, 0, QModelIndex());
    QCOMPARE(actual, expected);

    //let's try another column
    view.setCurrentIndex(model.index(1, 1));
    view.setSelectionBehavior(QAbstractItemView::SelectItems);
    QTest::keyClick(view.viewport(), Qt::Key_Up);
    expected = model.index(0, 1, QModelIndex());
    QCOMPARE(view.currentIndex(), expected);
    QTest::keyClick(view.viewport(), Qt::Key_Down);
    expected = model.index(1, 1, QModelIndex());
    QCOMPARE(view.currentIndex(), expected);
    QTest::keyClick(view.viewport(), Qt::Key_Up);
    expected = model.index(0, 1, QModelIndex());
    QCOMPARE(view.currentIndex(), expected);
    view.setColumnHidden(0, true);
    QTest::keyClick(view.viewport(), Qt::Key_Left);
    expected = model.index(0, 1, QModelIndex()); //it shouldn't have changed
    QCOMPARE(view.currentIndex(), expected);
    view.setColumnHidden(0, false);
    QTest::keyClick(view.viewport(), Qt::Key_Left);
    expected = model.index(0, 0, QModelIndex());
    QCOMPARE(view.currentIndex(), expected);
}

class TestDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    { return QSize(200, 50); }
};

typedef QVector<QPoint> PointList;

void tst_QTreeView::setSelection_data()
{
    QTest::addColumn<QRect>("selectionRect");
    QTest::addColumn<QAbstractItemView::SelectionMode>("selectionMode");
    QTest::addColumn<QItemSelectionModel::SelectionFlags>("selectionCommand");
    QTest::addColumn<PointList>("expectedItems");
    QTest::addColumn<int>("verticalOffset");

    const PointList pl1{QPoint(0, 0), QPoint(1, 0), QPoint(2, 0), QPoint(3, 0), QPoint(4, 0)};
    const PointList pl2{QPoint(0, 0), QPoint(1, 0), QPoint(2, 0), QPoint(3, 0), QPoint(4, 0),
                        QPoint(0, 1), QPoint(1, 1), QPoint(2, 1), QPoint(3, 1), QPoint(4, 1)};
    const QItemSelectionModel::SelectionFlags selFlags(QItemSelectionModel::ClearAndSelect |
                                                       QItemSelectionModel::Rows);
    QTest::newRow("(0,0,50,20),rows")
       << QRect(0, 0, 50, 20)
       << QAbstractItemView::SingleSelection
       << selFlags << pl1 << 0;

    QTest::newRow("(0,0,50,90),rows")
       << QRect(0, 0, 50, 90)
       << QAbstractItemView::ExtendedSelection
       << selFlags << pl2 << 0;

    QTest::newRow("(50,0,0,90),rows,invalid rect")
       << QRect(QPoint(50, 0), QPoint(0, 90))
       << QAbstractItemView::ExtendedSelection
       << selFlags << pl2 << 0;

    QTest::newRow("(0,-20,20,50),rows")
       << QRect(0, -20, 20, 50)
       << QAbstractItemView::ExtendedSelection
       << selFlags << pl2 << 1;
    QTest::newRow("(0,-50,20,90),rows")
       << QRect(0, -50, 20, 90)
       << QAbstractItemView::ExtendedSelection
       << selFlags << pl2 << 1;
}

void tst_QTreeView::setSelection()
{
    QFETCH(QRect, selectionRect);
    QFETCH(QAbstractItemView::SelectionMode, selectionMode);
    QFETCH(QItemSelectionModel::SelectionFlags, selectionCommand);
    QFETCH(PointList, expectedItems);
    QFETCH(int, verticalOffset);

    QtTestModel model(10, 5);
    model.levels = 1;
    model.setDecorationsEnabled(true);
    QTreeView view;
    view.resize(400, 300);
    view.show();
    view.setRootIsDecorated(false);
    view.setItemDelegate(new TestDelegate(&view));
    view.setSelectionMode(selectionMode);
    view.setModel(&model);
    view.setUniformRowHeights(true);
    view.setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    view.scrollTo(model.index(verticalOffset, 0), QAbstractItemView::PositionAtTop);
    view.setSelection(selectionRect, selectionCommand);
    QItemSelectionModel *selectionModel = view.selectionModel();
    QVERIFY(selectionModel);

    const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("(0,-20,20,50),rows", "Fails on WinRT - QTBUG-68297", Abort);
    QEXPECT_FAIL("(0,-50,20,90),rows", "Fails on WinRT - QTBUG-68297", Abort);
#endif
    QCOMPARE(selectedIndexes.count(), expectedItems.count());
    for (const QModelIndex &idx : selectedIndexes)
        QVERIFY(expectedItems.contains(QPoint(idx.column(), idx.row())));
}

void tst_QTreeView::indexAbove()
{
    QtTestModel model(6, 7);
    model.levels = 2;
    QTreeView view;

    QCOMPARE(view.indexAbove(QModelIndex()), QModelIndex());
    view.setModel(&model);
    QCOMPARE(view.indexAbove(QModelIndex()), QModelIndex());

    QStack<QModelIndex> parents;
    parents.push(QModelIndex());
    while (!parents.isEmpty()) {
        QModelIndex p = parents.pop();
        if (model.canFetchMore(p))
            model.fetchMore(p);
        int rows = model.rowCount(p);
        for (int r = rows - 1; r > 0; --r) {
            for (int column = 0; column < 3; ++column) {
                QModelIndex idx = model.index(r, column, p);
                QModelIndex expected = model.index(r - 1, column, p);
                QCOMPARE(view.indexAbove(idx), expected);
            }
        }
        // hide even rows
        for (int r = 0; r < rows; r+=2)
            view.setRowHidden(r, p, true);
        for (int r = rows - 1; r > 0; r-=2) {
            for (int column = 0; column < 3; ++column) {
                QModelIndex idx = model.index(r, column, p);
                QModelIndex expected = model.index(r - 2, column, p);
                QCOMPARE(view.indexAbove(idx), expected);
            }
        }
//        for (int r = 0; r < rows; ++r)
//            parents.push(model.index(r, 0, p));
    }
}

void tst_QTreeView::indexBelow()
{
    QtTestModel model(2, 2);

    QTreeView view;
    view.setModel(&model);
    view.show();

    {
        QModelIndex i = model.index(0, 0, view.rootIndex());
        QVERIFY(i.isValid());
        QCOMPARE(i.row(), 0);
        QCOMPARE(i.column(), 0);

        i = view.indexBelow(i);
        QVERIFY(i.isValid());
        QCOMPARE(i.row(), 1);
        QCOMPARE(i.column(), 0);
        i = view.indexBelow(i);
        QVERIFY(!i.isValid());
    }

    {
        QModelIndex i = model.index(0, 1, view.rootIndex());
        QVERIFY(i.isValid());
        QCOMPARE(i.row(), 0);
        QCOMPARE(i.column(), 1);

        i = view.indexBelow(i);
        QVERIFY(i.isValid());
        QCOMPARE(i.row(), 1);
        QCOMPARE(i.column(), 1);
        i = view.indexBelow(i);
        QVERIFY(!i.isValid());
    }
}

void tst_QTreeView::clicked()
{
    QtTestModel model(10, 2);

    QTreeView view;
    view.setModel(&model);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex firstIndex = model.index(0, 0, QModelIndex());
    QVERIFY(firstIndex.isValid());
    int itemHeight = view.visualRect(firstIndex).height();
    int itemOffset = view.visualRect(firstIndex).width() / 2;
    view.resize(200, itemHeight * (model.rows + 2));

    for (int i = 0; i < model.rowCount(); ++i) {
        QPoint p(itemOffset, 1 + itemHeight * i);
        QModelIndex index = view.indexAt(p);
        if (!index.isValid())
            continue;
        QSignalSpy spy(&view, &QTreeView::clicked);
        QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
        QTRY_COMPARE(spy.count(), 1);
    }
}

void tst_QTreeView::mouseDoubleClick()
{
    // Test double clicks outside the viewport.
    // (Should be a no-op and should not expand any item.)

    QStandardItemModel model(20, 2);
    for (int i = 0; i < model.rowCount(); i++) {
        QModelIndex index = model.index(i, 0, QModelIndex());
        model.insertRows(0, 20, index);
        model.insertColumns(0, 2,index);
        for (int i1 = 0; i1 <  model.rowCount(index); i1++) {
            QVERIFY(model.index(i1, 0, index).isValid());
        }
    }

    QTreeView view;
    view.setModel(&model);

    // make sure the viewport height is smaller than the contents height.
    view.resize(200, 200);
    view.move(0, 0);
    view.show();
    QModelIndex index = model.index(0, 0, QModelIndex());
    view.setCurrentIndex(index);

    view.setExpanded(model.index(0,0, QModelIndex()), true);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // Make sure all items are collapsed
    for (int i = 0; i < model.rowCount(QModelIndex()); i++)
        view.setExpanded(model.index(i, 0, QModelIndex()), false);

    int maximum = view.verticalScrollBar()->maximum();

    // Doubleclick in the bottom right corner, in the unused area between the vertical and horizontal scrollbar.
    int vsw = view.verticalScrollBar()->width();
    int hsh = view.horizontalScrollBar()->height();
    QTest::mouseDClick(&view, Qt::LeftButton, Qt::NoModifier, QPoint(view.width() - vsw + 1, view.height() - hsh + 1));
    // Should not have expanded, thus maximum() should have the same value.
    QCOMPARE(maximum, view.verticalScrollBar()->maximum());

    view.setExpandsOnDoubleClick(false);
    QTest::mouseDClick(&view, Qt::LeftButton, Qt::NoModifier, view.visualRect(index).center());
    QVERIFY(!view.isExpanded(index));
}

void tst_QTreeView::rowsAboutToBeRemoved()
{
    QStandardItemModel model(3, 1);
    for (int i = 0; i < model.rowCount(); i++) {
        const QString iS = QString::number(i);
        QModelIndex index = model.index(i, 0, QModelIndex());
        model.setData(index, iS);
        model.insertRows(0, 4, index);
        model.insertColumns(0,1,index);
        for (int i1 = 0; i1 <  model.rowCount(index); i1++) {
            QModelIndex index2 = model.index(i1, 0, index);
            model.setData(index2, iS + QString::number(i1));
        }
    }

    QTreeView view;
    view.setModel(&model);
    view.show();
    QModelIndex index = model.index(0,0, QModelIndex());
    view.setCurrentIndex(index);
    view.setExpanded(model.index(0,0, QModelIndex()), true);

    for (int i = 0; i < model.rowCount(QModelIndex()); i++)
        view.setExpanded(model.index(i, 0, QModelIndex()), true);

    QSignalSpy spy1(&model, &QAbstractItemModel::rowsAboutToBeRemoved);

    model.removeRows(1,1);
    QCOMPARE((view.state()), 0);
    // Should not be 5 (or any other number for that sake :)
    QCOMPARE(spy1.count(), 1);

}

void tst_QTreeView::headerSections_unhideSection()
{
    QtTestModel model(10, 7);

    QTreeView view;

    view.setModel(&model);
    view.show();
    int size = view.header()->sectionSize(0);
    view.setColumnHidden(0, true);

    // should go back to old size
    view.setColumnHidden(0, false);
    QCOMPARE(size, view.header()->sectionSize(0));
}

void tst_QTreeView::columnAt()
{
    QtTestModel model(10, 10);
    QTreeView view;
    view.resize(500,500);
    view.setModel(&model);

    QCOMPARE(view.columnAt(0), 0);
    // really this is testing the header... so not much more should be needed if the header is working...
}

void tst_QTreeView::scrollTo()
{
#define CHECK_VISIBLE(ROW,COL) QVERIFY(QRect(QPoint(),view.viewport()->size()).contains(\
                    view.visualRect(model.index((ROW),(COL),QModelIndex()))))

    QtTestModel model(100, 100);
    QTreeView view;
    view.setUniformRowHeights(true);
    view.scrollTo(QModelIndex(), QTreeView::PositionAtTop);
    view.setModel(&model);

    // ### check the scrollbar values an make sure it actually scrolls to the item
    // ### check for bot item based and pixel based scrolling
    // ### create a data function for this test

    view.scrollTo(QModelIndex());
    view.scrollTo(model.index(0, 0, QModelIndex()));
    view.scrollTo(model.index(0, 0, QModelIndex()), QTreeView::PositionAtTop);
    view.scrollTo(model.index(0, 0, QModelIndex()), QTreeView::PositionAtBottom);

    view.show();
    view.setVerticalScrollMode(QAbstractItemView::ScrollPerItem); //some styles change that in Polish
    view.resize(300, 200);

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    //view.verticalScrollBar()->setValue(0);

    view.scrollTo(model.index(0, 0, QModelIndex()));
    CHECK_VISIBLE(0,0);
    QCOMPARE(view.verticalScrollBar()->value(), 0);

    view.header()->resizeSection(0, 5); // now we only see the branches
    view.scrollTo(model.index(5, 0, QModelIndex()), QTreeView::PositionAtTop);
    QCOMPARE(view.verticalScrollBar()->value(), 5);

    view.scrollTo(model.index(60, 60, QModelIndex()));

    CHECK_VISIBLE(60,60);
    view.scrollTo(model.index(60, 30, QModelIndex()));
    CHECK_VISIBLE(60,30);
    view.scrollTo(model.index(30, 30, QModelIndex()));
    CHECK_VISIBLE(30,30);

    // TODO force it to move to the left and then the right
}

void tst_QTreeView::rowsAboutToBeRemoved_move()
{
    QStandardItemModel model(3,1);
    QTreeView view;
    view.setModel(&model);
    QModelIndex indexThatWantsToLiveButWillDieDieITellYou;
    QModelIndex parent = model.index(2, 0);
    view.expand(parent);
    for (int i = 0; i < 6; ++i) {
        model.insertRows(0, 1, parent);
        model.insertColumns(0, 1, parent);
        QModelIndex index = model.index(0, 0, parent);
        view.expand(index);
        if (i == 3)
            indexThatWantsToLiveButWillDieDieITellYou = index;
        model.setData(index, i);
        parent = index;
    }
    view.resize(600,800);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.doItemsLayout();
    view.executeDelayedItemsLayout();
    parent = indexThatWantsToLiveButWillDieDieITellYou.parent();
    QCOMPARE(view.isExpanded(indexThatWantsToLiveButWillDieDieITellYou), true);
    QCOMPARE(parent.isValid(), true);
    QCOMPARE(parent.parent().isValid(), true);
    view.expand(parent);
    QCOMPARE(view.isExpanded(parent), true);
    QCOMPARE(view.isExpanded(indexThatWantsToLiveButWillDieDieITellYou), true);
    model.removeRow(0, indexThatWantsToLiveButWillDieDieITellYou);
    QCOMPARE(view.isExpanded(parent), true);
    QCOMPARE(view.isExpanded(indexThatWantsToLiveButWillDieDieITellYou), true);
}

void tst_QTreeView::resizeColumnToContents()
{
    QStandardItemModel model(50,2);
    for (int r = 0; r < model.rowCount(); ++r) {
        const QString rS = QString::number(r);
        for (int c = 0; c < model.columnCount(); ++c) {
            QModelIndex idx = model.index(r, c);
            model.setData(idx, rS + QLatin1Char(',') + QString::number(c));
            model.insertColumns(0, 2, idx);
            model.insertRows(0, 6, idx);
            for (int i = 0; i < 6; ++i) {
                const QString iS = QString::number(i);
                for (int j = 0; j < 2 ; ++j) {
                    model.setData(model.index(i, j, idx), QLatin1String("child") + iS + QString::number(j));
                }
            }
        }
    }
    QTreeView view;
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    view.scrollToBottom();
    view.resizeColumnToContents(0);
    int oldColumnSize = view.header()->sectionSize(0);
    view.setRootIndex(model.index(0, 0));
    view.resizeColumnToContents(0);        //Earlier, this gave an assert
    QVERIFY(view.header()->sectionSize(0) > oldColumnSize);
}

void tst_QTreeView::insertAfterSelect()
{
    QtTestModel model(10, 10);
    QTreeView view;
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex firstIndex = model.index(0, 0, QModelIndex());
    QVERIFY(firstIndex.isValid());
    int itemOffset = view.visualRect(firstIndex).width() / 2;
    QPoint p(itemOffset, 1);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QVERIFY(view.selectionModel()->isSelected(firstIndex));
    model.insertNewRow();
    QVERIFY(view.selectionModel()->isSelected(firstIndex)); // Should still be selected
}

void tst_QTreeView::removeAfterSelect()
{
    QtTestModel model(10, 10);
    QTreeView view;
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex firstIndex = model.index(0, 0, QModelIndex());
    QVERIFY(firstIndex.isValid());
    int itemOffset = view.visualRect(firstIndex).width() / 2;
    QPoint p(itemOffset, 1);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QVERIFY(view.selectionModel()->isSelected(firstIndex));
    model.removeLastRow();
    QVERIFY(view.selectionModel()->isSelected(firstIndex)); // Should still be selected
}

void tst_QTreeView::hiddenItems()
{
    QtTestModel model(10, 10);
    QTreeView view;
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex firstIndex = model.index(1, 0, QModelIndex());
    QVERIFY(firstIndex.isValid());
    if (model.canFetchMore(firstIndex))
        model.fetchMore(firstIndex);
    for (int i = 0; i < model.rowCount(firstIndex); i++)
        view.setRowHidden(i , firstIndex, true );

    int itemOffset = view.visualRect(firstIndex).width() / 2;
    int itemHeight = view.visualRect(firstIndex).height();
    QPoint p(itemOffset, itemHeight + 1);
    view.setExpanded(firstIndex, false);
    QTest::mouseDClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(view.isExpanded(firstIndex), false);

    p.setX(5);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(view.isExpanded(firstIndex), false);
}

void tst_QTreeView::spanningItems()
{
    QtTestModel model(10, 10);
    model.onlyValidCalls = true;
    QTreeView view;
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    int itemWidth = view.header()->sectionSize(0);
    int itemHeight = view.visualRect(model.index(0, 0, QModelIndex())).height();

    // every second row is spanning
    for (int i = 1; i < model.rowCount(QModelIndex()); i += 2)
        view.setFirstColumnSpanned(i , QModelIndex(), true);

    // non-spanning item
    QPoint p(itemWidth / 2, itemHeight / 2); // column 0, row 0
    view.setCurrentIndex(QModelIndex());
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(view.currentIndex(), model.index(0, 0, QModelIndex()));
    QCOMPARE(view.header()->sectionSize(0) - view.indentation(),
             view.visualRect(model.index(0, 0, QModelIndex())).width());

    // spanning item
    p.setX(itemWidth + (itemWidth / 2)); // column 1
    p.setY(itemHeight + (itemHeight / 2)); // row 1
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(view.currentIndex(), model.index(1, 0, QModelIndex()));
    QCOMPARE(view.header()->length() - view.indentation(),
             view.visualRect(model.index(1, 0, QModelIndex())).width());

    // size hint
    // every second row is un-spanned
    QStyleOptionViewItem option = view.viewOptions();
    int w = view.header()->sectionSizeHint(0);
    for (int i = 0; i < model.rowCount(QModelIndex()); ++i) {
        if (!view.isFirstColumnSpanned(i, QModelIndex())) {
            QModelIndex index = model.index(i, 0, QModelIndex());
            w = qMax(w, view.itemDelegate(index)->sizeHint(option, index).width() + view.indentation());
        }
    }
    QCOMPARE(view.sizeHintForColumn(0), w);

    view.repaint(); // to check that this doesn't hit any assert
}

void tst_QTreeView::selectionOrderTest()
{
    QVERIFY(static_cast<QItemSelectionModel*>(sender())->currentIndex().row() != -1);
}

void tst_QTreeView::selection()
{
    if (!QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This causes a crash triggered by setVisible(false)");

    QTreeView treeView;
    QStandardItemModel m(10, 2);
    for (int i = 0;i < 10; ++i)
        m.setData(m.index(i, 0), i);
    treeView.setModel(&m);
    treeView.show();
    QVERIFY(QTest::qWaitForWindowExposed(&treeView));

    treeView.setSelectionBehavior(QAbstractItemView::SelectRows);
    treeView.setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(treeView.selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &tst_QTreeView::selectionOrderTest);

    QTest::mousePress(treeView.viewport(), Qt::LeftButton, {},
                      treeView.visualRect(m.index(1, 0)).center());
    QTest::keyPress(treeView.viewport(), Qt::Key_Down);
    auto selectedRows = treeView.selectionModel()->selectedRows();
    QCOMPARE(selectedRows.size(), 1);
    QCOMPARE(selectedRows.first(), m.index(2, 0, QModelIndex()));
    QTest::keyPress(treeView.viewport(), Qt::Key_5);
    selectedRows = treeView.selectionModel()->selectedRows();
    QCOMPARE(selectedRows.size(), 1);
    QCOMPARE(selectedRows.first(), m.index(5, 0, QModelIndex()));
}

//From task 151686 QTreeView ExtendedSelection selects hidden rows
void tst_QTreeView::selectionWithHiddenItems()
{
    QStandardItemModel model;

    QStandardItem item0("row 0");
    QStandardItem item1("row 1");
    QStandardItem item2("row 2");
    QStandardItem item3("row 3");
    model.appendColumn({&item0, &item1, &item2, &item3});

    QStandardItem child("child");
    item1.appendRow(&child);

    QTreeView view;
    view.setModel(&model);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    //child should not be selected as it is hidden (its parent is not expanded)
    view.selectAll();
    QCOMPARE(view.selectionModel()->selection().count(), 1); //one range
    QCOMPARE(view.selectionModel()->selectedRows().count(), 4);
    view.expandAll();
    QVERIFY(view.isExpanded(item1.index()));
    QCOMPARE(view.selectionModel()->selection().count(), 1);
    QCOMPARE(view.selectionModel()->selectedRows().count(), 4);
    QVERIFY( !view.selectionModel()->isSelected(model.indexFromItem(&child)));
    view.clearSelection();
    QVERIFY(view.isExpanded(item1.index()));

    //child should be selected as it is visible (its parent is expanded)
    view.selectAll();
    QCOMPARE(view.selectionModel()->selection().count(), 2);
    QCOMPARE(view.selectionModel()->selectedRows().count(), 5); //everything is selected
    view.clearSelection();

    //we hide the node with a child (there should then be 3 items selected in 2 ranges)
    view.setRowHidden(1, QModelIndex(), true);
    QVERIFY(view.isExpanded(item1.index()));
    view.selectAll();
    QCOMPARE(view.selectionModel()->selection().count(), 2);
    QCOMPARE(view.selectionModel()->selectedRows().count(), 3);
    QVERIFY(!view.selectionModel()->isSelected(model.indexFromItem(&item1)));
    QVERIFY(!view.selectionModel()->isSelected(model.indexFromItem(&child)));

    view.setRowHidden(1, QModelIndex(), false);
    QVERIFY(view.isExpanded(item1.index()));
    view.clearSelection();

    //we hide a node without children (there should then be 4 items selected in 3 ranges)
    view.setRowHidden(2, QModelIndex(), true);
    QVERIFY(view.isExpanded(item1.index()));
    view.selectAll();
    QVERIFY(view.isExpanded(item1.index()));
    QCOMPARE(view.selectionModel()->selection().count(), 3);
    QCOMPARE(view.selectionModel()->selectedRows().count(), 4);
    QVERIFY( !view.selectionModel()->isSelected(model.indexFromItem(&item2)));
    view.setRowHidden(2, QModelIndex(), false);
    QVERIFY(view.isExpanded(item1.index()));
    view.clearSelection();
}

void tst_QTreeView::selectAll()
{
    QStandardItemModel model(4, 4);
    QTreeView view2;
    view2.setModel(&model);
    view2.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view2.selectAll();  // Should work with an empty model
    //everything should be selected since we are in ExtendedSelection mode
    QCOMPARE(view2.selectedIndexes().count(), model.rowCount() * model.columnCount());

    for (int i = 0; i < model.rowCount(); ++i)
        model.setData(model.index(i,0), QLatin1String("row ") + QString::number(i));
    QTreeView view;
    view.setModel(&model);
    int selectedCount = view.selectedIndexes().count();
    view.selectAll();
    QCOMPARE(view.selectedIndexes().count(), selectedCount);

    QTreeView view3;
    view3.setModel(&model);
    view3.setSelectionMode(QAbstractItemView::NoSelection);
    view3.selectAll();
    QCOMPARE(view3.selectedIndexes().count(), 0);
}

void tst_QTreeView::extendedSelection_data()
{
    QTest::addColumn<QPoint>("mousePressPos");
    QTest::addColumn<int>("selectedCount");

    QTest::newRow("select") << QPoint(10, 10) << 2;
    QTest::newRow("unselect") << QPoint(10, 300) << 0;
}

void tst_QTreeView::extendedSelection()
{
    QFETCH(QPoint, mousePressPos);
    QFETCH(int, selectedCount);

    QStandardItemModel model(5, 2);
    QWidget topLevel;
    QTreeView view(&topLevel);
    view.resize(qMax(mousePressPos.x() * 2, 300), qMax(mousePressPos.y() * 2, 350));
    view.setModel(&model);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QTest::mousePress(view.viewport(), Qt::LeftButton, {}, mousePressPos);
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), selectedCount);
}

void tst_QTreeView::rowSizeHint()
{
    //tests whether the correct visible columns are taken into account when
    //calculating the height of a line
    QStandardItemModel model(1, 3);
    model.setData(model.index(0, 0), QSize(20, 40), Qt::SizeHintRole);
    model.setData(model.index(0, 1), QSize(20, 10), Qt::SizeHintRole);
    model.setData(model.index(0, 2), QSize(20, 10), Qt::SizeHintRole);
    QTreeView view;
    view.setModel(&model);

    view.header()->moveSection(1, 0); //the 2nd column goes to the 1st place

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    //it must be 40 since the tallest item that defines the height of a line
    QCOMPARE(view.visualRect(model.index(0,0)).height(), 40);
    QCOMPARE(view.visualRect(model.index(0,1)).height(), 40);
    QCOMPARE(view.visualRect(model.index(0,2)).height(), 40);
}


//From task 155449 (QTreeWidget has a large width for the first section when sorting
//is turned on before items are added)

void tst_QTreeView::setSortingEnabledTopLevel()
{
    QTreeView view;
    QStandardItemModel model(1, 1);
    view.setModel(&model);
    const int size = view.header()->sectionSize(0);
    view.setSortingEnabled(true);
    model.setColumnCount(3);
    //we test that changing the column count doesn't change the 1st column size
    QCOMPARE(view.header()->sectionSize(0), size);
}

void tst_QTreeView::setSortingEnabledChild()
{
    QMainWindow win;
    QTreeView view;
    // two columns to not get in trouble with stretchLastSection
    QStandardItemModel model(1, 2);
    view.setModel(&model);
    view.header()->setDefaultSectionSize(92);
    win.setCentralWidget(&view);
    const int size = view.header()->sectionSize(0);
    view.setSortingEnabled(true);
    model.setColumnCount(3);
    //we test that changing the column count doesn't change the 1st column size
    QCOMPARE(view.header()->sectionSize(0), size);
}

void tst_QTreeView::headerHidden()
{
    QTreeView view;
    QStandardItemModel model;
    view.setModel(&model);
    QCOMPARE(view.isHeaderHidden(), false);
    QCOMPARE(view.header()->isHidden(), false);
    view.setHeaderHidden(true);
    QCOMPARE(view.isHeaderHidden(), true);
    QCOMPARE(view.header()->isHidden(), true);
}

class TestTreeViewStyle : public QProxyStyle
{
    Q_OBJECT
public:
    using QProxyStyle::QProxyStyle;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override
    {
        if (metric == QStyle::PM_TreeViewIndentation)
            return indentation;
        else
            return QProxyStyle::pixelMetric(metric, option, widget);
    }
    int indentation = 20;
};

void tst_QTreeView::indentation()
{
    TestTreeViewStyle style1;
    TestTreeViewStyle style2;
    style1.indentation = 20;
    style2.indentation = 30;

    QTreeView view;
    view.setStyle(&style1);
    QCOMPARE(view.indentation(), style1.indentation);
    view.setStyle(&style2);
    QCOMPARE(view.indentation(), style2.indentation);
    view.setIndentation(70);
    QCOMPARE(view.indentation(), 70);
    view.setStyle(&style1);
    QCOMPARE(view.indentation(), 70);
    view.resetIndentation();
    QCOMPARE(view.indentation(), style1.indentation);
    view.setStyle(&style2);
    QCOMPARE(view.indentation(), style2.indentation);
}

// From Task 145199 (crash when column 0 having at least one expanded item is removed and then
// inserted). The test passes simply if it doesn't crash, hence there are no calls
// to QCOMPARE() or QVERIFY().
void tst_QTreeView::removeAndInsertExpandedCol0()
{
    QTreeView view;
    QStandardItemModel model;
    view.setModel(&model);

    model.setColumnCount(1);

    QStandardItem *item0 = new QStandardItem(QString("item 0"));
    model.invisibleRootItem()->appendRow(item0);
    view.expand(item0->index());
    QStandardItem *item1 = new QStandardItem(QString("item 1"));
    item0->appendRow(item1);

    model.removeColumns(0, 1);
    model.insertColumns(0, 1);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

void tst_QTreeView::disabledButCheckable()
{
    QTreeView view;
    QStandardItemModel model;
    QStandardItem *item;
    item = new QStandardItem(QLatin1String("Row 1 Item"));
    model.insertRow(0, item);

    item = new QStandardItem(QLatin1String("Row 2 Item"));
    item->setCheckable(true);
    item->setEnabled(false);
    model.insertRow(1, item);

    view.setModel(&model);
    view.setCurrentIndex(model.index(1,0));
    QCOMPARE(item->checkState(), Qt::Unchecked);
    view.show();

    QTest::keyClick(&view, Qt::Key_Space);
    QCOMPARE(item->checkState(), Qt::Unchecked);
}

void tst_QTreeView::sortByColumn_data()
{
    QTest::addColumn<bool>("sortingEnabled");
    QTest::newRow("sorting enabled") << true;
    QTest::newRow("sorting disabled") << false;
}

// Checks sorting and that sortByColumn also sets the sortIndicator
void tst_QTreeView::sortByColumn()
{
    QFETCH(bool, sortingEnabled);
    QTreeView view;
    QStandardItemModel model(4, 2);
    QSortFilterProxyModel sfpm; // default QStandardItemModel does not support 'unsorted' state
    sfpm.setSourceModel(&model);
    model.setItem(0, 0, new QStandardItem("b"));
    model.setItem(1, 0, new QStandardItem("d"));
    model.setItem(2, 0, new QStandardItem("c"));
    model.setItem(3, 0, new QStandardItem("a"));
    model.setItem(0, 1, new QStandardItem("e"));
    model.setItem(1, 1, new QStandardItem("g"));
    model.setItem(2, 1, new QStandardItem("h"));
    model.setItem(3, 1, new QStandardItem("f"));

    view.setSortingEnabled(sortingEnabled);
    view.setModel(&sfpm);

    view.sortByColumn(1, Qt::DescendingOrder);
    QCOMPARE(view.header()->sortIndicatorSection(), 1);
    QCOMPARE(view.model()->data(view.model()->index(0, 0)).toString(), QString::fromLatin1("c"));
    QCOMPARE(view.model()->data(view.model()->index(1, 0)).toString(), QString::fromLatin1("d"));
    QCOMPARE(view.model()->data(view.model()->index(0, 1)).toString(), QString::fromLatin1("h"));
    QCOMPARE(view.model()->data(view.model()->index(1, 1)).toString(), QString::fromLatin1("g"));

    view.sortByColumn(0, Qt::AscendingOrder);
    QCOMPARE(view.header()->sortIndicatorSection(), 0);
    QCOMPARE(view.model()->data(view.model()->index(0, 0)).toString(), QString::fromLatin1("a"));
    QCOMPARE(view.model()->data(view.model()->index(1, 0)).toString(), QString::fromLatin1("b"));
    QCOMPARE(view.model()->data(view.model()->index(0, 1)).toString(), QString::fromLatin1("f"));
    QCOMPARE(view.model()->data(view.model()->index(1, 1)).toString(), QString::fromLatin1("e"));

    view.sortByColumn(-1, Qt::AscendingOrder);
    QCOMPARE(view.header()->sortIndicatorSection(), -1);
    QCOMPARE(view.model()->data(view.model()->index(0, 0)).toString(), QString::fromLatin1("b"));
    QCOMPARE(view.model()->data(view.model()->index(1, 0)).toString(), QString::fromLatin1("d"));
    QCOMPARE(view.model()->data(view.model()->index(0, 1)).toString(), QString::fromLatin1("e"));
    QCOMPARE(view.model()->data(view.model()->index(1, 1)).toString(), QString::fromLatin1("g"));

    // a new 'sortByColumn()' should do a re-sort (e.g. due to the data changed), QTBUG-86268
    view.setModel(&model);
    view.sortByColumn(0, Qt::AscendingOrder);
    QCOMPARE(view.model()->data(view.model()->index(0, 0)).toString(), QString::fromLatin1("a"));
    model.setItem(0, 0, new QStandardItem("x"));
    view.sortByColumn(0, Qt::AscendingOrder);
    QCOMPARE(view.model()->data(view.model()->index(0, 0)).toString(), QString::fromLatin1("b"));
}

/*
    This is a model that every time kill() is called it will completely change
    all of its nodes for new nodes.  It then qFatal's if you later use a dead node.
 */
class EvilModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    class Node
    {
    public:
        Node(Node *p = nullptr, int level = 0) : parent(p)
        {
            populate(level);
        }
        ~Node()
        {
            qDeleteAll(children.begin(), children.end());
            qDeleteAll(deadChildren.begin(), deadChildren.end());
        }

        void populate(int level = 0)
        {
            if (level < 4) {
                for (int i = 0; i < 5; ++i)
                    children.append(new Node(this, level + 1));
            }
        }
        void kill()
        {
            for (int i = children.count() -1; i >= 0; --i) {
                children.at(i)->kill();
                if (parent == nullptr) {
                    deadChildren.append(children.at(i));
                    children.removeAt(i);
                }
            }
            if (parent == nullptr) {
                if (!children.isEmpty())
                    qFatal("%s: children should be empty when parent is null", Q_FUNC_INFO);
                populate();
            } else {
                isDead = true;
            }
        }

        QVector<Node *> children;
        QVector<Node *> deadChildren;
        Node *parent;
        bool isDead = false;
    };

    Node *root;
    bool crash = false;

    EvilModel(QObject *parent = nullptr): QAbstractItemModel(parent), root(new Node)
    {}
    ~EvilModel()
    {
        delete root;
    }

    void setCrash()
    {
        crash = true;
    }

    void change()
    {
        emit layoutAboutToBeChanged();
        QModelIndexList oldList = persistentIndexList();
        QVector<QStack<int>> oldListPath;
        for (int i = 0; i < oldList.count(); ++i) {
            QModelIndex idx = oldList.at(i);
            QStack<int> path;
            while (idx.isValid()) {
                path.push(idx.row());
                idx = idx.parent();
            }
            oldListPath.append(path);
        }
        root->kill();

        QModelIndexList newList;
        for (auto path : qAsConst(oldListPath)) {
            QModelIndex idx;
            while (!path.isEmpty())
                idx = index(path.pop(), 0, idx);
            newList.append(idx);
        }

        changePersistentIndexList(oldList, newList);
        emit layoutChanged();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Node *parentNode = root;
        if (parent.isValid()) {
            parentNode = static_cast<Node*>(parent.internalPointer());
            if (parentNode->isDead)
                qFatal("%s: parentNode is dead!", Q_FUNC_INFO);
        }
        return parentNode->children.count();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.column() > 0 ? 0 : 1;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        Node *grandparentNode = static_cast<Node*>(parent.internalPointer());
        Node *parentNode = root;
        if (parent.isValid()) {
            if (grandparentNode->isDead)
                qFatal("%s: grandparentNode is dead!", Q_FUNC_INFO);
            parentNode = grandparentNode->children[parent.row()];
            if (parentNode->isDead)
                qFatal("%s: grandparentNode is dead!", Q_FUNC_INFO);
        }
        return createIndex(row, column, parentNode);
    }

    QModelIndex parent(const QModelIndex &index) const override
    {
        Node *parent = static_cast<Node*>(index.internalPointer());
        Node *grandparent = parent->parent;
        if (!grandparent)
            return QModelIndex();
        return createIndex(grandparent->children.indexOf(parent), 0, grandparent);
    }

    QVariant data(const QModelIndex &idx, int role) const override
    {
        if (crash) {
            QTest::qFail("Should not get here...", __FILE__, __LINE__);
            return QVariant();
        }
        if (idx.isValid() && role == Qt::DisplayRole) {
            Node *parentNode = root;
            if (idx.isValid()) {
                parentNode = static_cast<Node*>(idx.internalPointer());
                if (parentNode->isDead)
                    qFatal("%s: grandparentNode is dead!", Q_FUNC_INFO);
            }
            return QLatin1Char('[') + QString::number(idx.row()) + QLatin1Char(',')
                + QString::number(idx.column()) + QLatin1Char(',')
                + QLatin1String(parentNode->isDead ? "dead" : "alive") + QLatin1Char(']');
        }
        return QVariant();
    }
};

void tst_QTreeView::evilModel_data()
{
    QTest::addColumn<bool>("visible");
    QTest::newRow("visible") << false;
    QTest::newRow("visible") << true;
}

void tst_QTreeView::evilModel()
{
    QFETCH(bool, visible);
    // init
    TreeView view;
    EvilModel model;
    view.setModel(&model);
    view.setVisible(visible);
    QPersistentModelIndex firstLevel = model.index(0, 0);
    QPersistentModelIndex secondLevel = model.index(1, 0, firstLevel);
    QPersistentModelIndex thirdLevel = model.index(2, 0, secondLevel);
    view.setExpanded(firstLevel, true);
    view.setExpanded(secondLevel, true);
    view.setExpanded(thirdLevel, true);
    model.change();

    // tests
    view.setRowHidden(0, firstLevel, true);
    model.change();

    view.setFirstColumnSpanned(1, QModelIndex(), true);
    model.change();

    view.expand(secondLevel);
    model.change();

    view.collapse(secondLevel);
    model.change();

    view.isExpanded(secondLevel);
    view.setCurrentIndex(firstLevel);
    model.change();

    view.keyboardSearch("foo");
    model.change();

    view.visualRect(secondLevel);
    model.change();

    view.scrollTo(thirdLevel);
    model.change();

    view.update();  // will not do anything since view is not visible
    model.change();

    QTest::mouseDClick(view.viewport(), Qt::LeftButton);
    model.change();

    view.indexAt(QPoint(10, 10));
    model.change();

    view.indexAbove(model.index(2, 0));
    model.change();

    view.indexBelow(model.index(1, 0));
    model.change();

    QRect rect(0, 0, 10, 10);
    view.setSelection(rect, QItemSelectionModel::Select);
    model.change();

    view.moveCursor(QTreeView::MoveDown, Qt::NoModifier);
    model.change();

    view.resizeColumnToContents(1);
    model.change();

    view.QAbstractItemView::sizeHintForColumn(1);
    model.change();

    view.rowHeight(secondLevel);
    model.change();

    view.setRootIsDecorated(true);
    model.change();

    view.setItemsExpandable(false);
    model.change();

    view.columnViewportPosition(0);
    model.change();

    view.columnWidth(0);
    model.change();

    view.setColumnWidth(0, 30);
    model.change();

    view.columnAt(15);
    model.change();

    view.isColumnHidden(1);
    model.change();

    view.setColumnHidden(2, true);
    model.change();

    view.isHeaderHidden();
    model.change();

    view.setHeaderHidden(true);
    model.change();

    view.isRowHidden(2, secondLevel);
    model.change();

    view.setRowHidden(3, secondLevel, true);
    model.change();

    view.isFirstColumnSpanned(3, thirdLevel);
    model.change();

    view.setSortingEnabled(true);
    model.change();

    view.isSortingEnabled();
    model.change();

    view.setAnimated(true);
    model.change();

    view.isAnimated();
    model.change();

    view.setAllColumnsShowFocus(true);
    model.change();

    view.allColumnsShowFocus();
    model.change();

    view.doItemsLayout();
    model.change();

    view.reset();
    model.change();

    view.sortByColumn(1, Qt::AscendingOrder);
    model.change();

    view.dataChanged(secondLevel, secondLevel);
    model.change();

    view.hideColumn(1);
    model.change();

    view.showColumn(1);
    model.change();

    view.resizeColumnToContents(1);
    model.change();

    view.sortByColumn(1, Qt::DescendingOrder);
    model.change();

    view.selectAll();
    model.change();

    view.expandAll();
    model.change();

    view.collapseAll();
    model.change();

    view.expandToDepth(3);
    model.change();

    view.setRootIndex(secondLevel);

    model.setCrash();
    view.setModel(nullptr);
}

void tst_QTreeView::indexRowSizeHint()
{
    QStandardItemModel model(10, 1);
    QTreeView view;

    view.setModel(&model);

    QModelIndex index = model.index(5, 0);
    QPushButton *w = new QPushButton("Test");
    view.setIndexWidget(index, w);

    view.show();

    QCOMPARE(view.indexRowSizeHint(index), w->sizeHint().height());
}

void tst_QTreeView::filterProxyModelCrash()
{
    QStandardItemModel model;
    QList<QStandardItem *> items;
    for (int i = 0; i < 100; i++)
        items << new QStandardItem(QLatin1String("item ") + QString::number(i));
    model.appendColumn(items);

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    TreeView view;
    view.setModel(&proxy);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    proxy.invalidate();
    view.verticalScrollBar()->setValue(15);
    QTest::qWait(20);

    proxy.invalidate();
    view.update(); //used to crash
    QTRY_VERIFY(view.wasPainted);
}

void tst_QTreeView::renderToPixmap_data()
{
    QTest::addColumn<int>("row");
    QTest::newRow("row-0") << 0;
    QTest::newRow("row-1") << 1;
}

void tst_QTreeView::renderToPixmap()
{
    QFETCH(int, row);
    QTreeView view;
    QStandardItemModel model;

    model.appendRow(new QStandardItem("Spanning"));
    model.appendRow({ new QStandardItem("Not"), new QStandardItem("Spanning") });

    view.setModel(&model);
    view.setFirstColumnSpanned(0, QModelIndex(), true);

#ifdef QT_BUILD_INTERNAL
    {
        // We select the index at row=0 because it spans the
        // column (regression test for an assert)
        // We select the index at row=1 for coverage.
        QItemSelection sel(model.index(row,0), model.index(row,1));
        QRect rect;
        view.d_func()->renderToPixmap(sel.indexes(), &rect);
    }
#endif
}

void tst_QTreeView::styleOptionViewItem()
{
    class MyDelegate : public QStyledItemDelegate
    {
        static QString posToString(QStyleOptionViewItem::ViewItemPosition pos)
        {
            static const char* s_pos[] = { "Invalid", "Beginning", "Middle", "End", "OnlyOne" };
            return s_pos[pos];
        }
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        void paint(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
        {
            QStyleOptionViewItem opt(option);
            initStyleOption(&opt, index);

            QVERIFY(!opt.text.isEmpty());
            QCOMPARE(opt.index, index);
            //qDebug() << index << opt.text;

            if (allCollapsed) {
                QCOMPARE(!opt.features.testFlag(QStyleOptionViewItem::Alternate),
                         !(index.row() % 2));
            }
            QCOMPARE(!opt.features.testFlag(QStyleOptionViewItem::HasCheckIndicator),
                     !opt.text.contains("Checkable"));

            const QString posStr(posToString(opt.viewItemPosition));
            if (opt.text.contains("Beginning"))
                QCOMPARE(posStr, posToString(QStyleOptionViewItem::Beginning));

            if (opt.text.contains("Middle"))
                QCOMPARE(posStr, posToString(QStyleOptionViewItem::Middle));

            if (opt.text.contains("End"))
                QCOMPARE(posStr, posToString(QStyleOptionViewItem::End));

            if (opt.text.contains("OnlyOne"))
                QCOMPARE(posStr, posToString(QStyleOptionViewItem::OnlyOne));

            if (opt.text.contains("Checked"))
                QCOMPARE(opt.checkState, Qt::Checked);
            else
                QCOMPARE(opt.checkState, Qt::Unchecked);

            QCOMPARE(!opt.state.testFlag(QStyle::State_Children),
                     !opt.text.contains("HasChildren"));
            QCOMPARE(opt.state.testFlag(QStyle::State_Sibling),
                     !opt.text.contains("Last"));

            QVERIFY(!opt.text.contains("Assert"));

            QStyledItemDelegate::paint(painter, option, index);
            count++;
        }
        mutable int count = 0;
        bool allCollapsed = false;
    };

    QTreeView view;
    QStandardItemModel model;
    view.setModel(&model);
    MyDelegate delegate;
    view.setItemDelegate(&delegate);
    model.appendRow({ new QStandardItem("Beginning"),
                      new QStandardItem("Hidden"),
                      new QStandardItem("Middle"),
                      new QStandardItem("Middle"),
                      new QStandardItem("End") });
    QStandardItem *par1 = new QStandardItem("Beginning HasChildren");
    model.appendRow({ par1,
                      new QStandardItem("Hidden"),
                      new QStandardItem("Middle HasChildren"),
                      new QStandardItem("Middle HasChildren"),
                      new QStandardItem("End HasChildren") });
    model.appendRow({ new QStandardItem("OnlyOne"),
                      new QStandardItem("Hidden"),
                      new QStandardItem("Assert"),
                      new QStandardItem("Assert"),
                      new QStandardItem("Assert") });
    QStandardItem *checkable = new QStandardItem("Checkable");
    checkable->setCheckable(true);
    QStandardItem *checked = new QStandardItem("Checkable Checked");
    checked->setCheckable(true);
    checked->setCheckState(Qt::Checked);
    model.appendRow({ new QStandardItem("Beginning"),
                      new QStandardItem("Hidden"),
                      checkable, checked,
                      new QStandardItem("End") });
    model.appendRow({ new QStandardItem("Beginning Last"),
                      new QStandardItem("Hidden"),
                      new QStandardItem("Middle Last"),
                      new QStandardItem("Middle Last"),
                      new QStandardItem("End Last") });
    par1->appendRow({ new QStandardItem("Beginning"),
                      new QStandardItem("Hidden"),
                      new QStandardItem("Middle"),
                      new QStandardItem("Middle"),
                      new QStandardItem("End") });
    QStandardItem *par2 = new QStandardItem("Beginning HasChildren");
    par1->appendRow({ par2,
                      new QStandardItem("Hidden"),
                      new QStandardItem("Middle HasChildren"),
                      new QStandardItem("Middle HasChildren"),
                      new QStandardItem("End HasChildren") });
    par2->appendRow({ new QStandardItem("Beginning Last"),
                      new QStandardItem("Hidden"),
                      new QStandardItem("Middle Last"),
                      new QStandardItem("Middle Last"),
                      new QStandardItem("End Last") });
    QStandardItem *par3 = new QStandardItem("Beginning Last");
    par1->appendRow({ par3, new QStandardItem("Hidden"),
                      new QStandardItem("Middle Last"),
                      new QStandardItem("Middle Last"),
                      new QStandardItem("End Last") });
    par3->appendRow({ new QStandardItem("Assert"),
                      new QStandardItem("Hidden"),
                      new QStandardItem("Assert"),
                      new QStandardItem("Assert"),
                      new QStandardItem("Asser") });
    view.setRowHidden(0, par3->index(), true);
    par1->appendRow({ new QStandardItem("Assert"),
                      new QStandardItem("Hidden"),
                      new QStandardItem("Assert"),
                      new QStandardItem("Assert"),
                      new QStandardItem("Asser") });
    view.setRowHidden(3, par1->index(), true);

    view.setColumnHidden(1, true);
    const int visibleColumns = 4;
    const int modelColumns = 5;

    view.header()->swapSections(2, 3);
    view.setFirstColumnSpanned(2, QModelIndex(), true);
    view.setAlternatingRowColors(true);

#ifdef QT_BUILD_INTERNAL
    {
        // Test the rendering to pixmap before painting the widget.
        // The rendering to pixmap should not depend on having been
        // painted already yet.
        delegate.count = 0;
        QItemSelection sel(model.index(0,0), model.index(0,modelColumns-1));
        QRect rect;
        view.d_func()->renderToPixmap(sel.indexes(), &rect);
        if (delegate.count != visibleColumns) {
            qDebug() << rect << view.rect() << view.isVisible();
        }
        QTRY_COMPARE(delegate.count, visibleColumns);
    }
#endif

    delegate.count = 0;
    delegate.allCollapsed = true;
    view.showMaximized();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(delegate.count >= 13);
    delegate.count = 0;
    delegate.allCollapsed = false;
    view.expandAll();
    QTRY_VERIFY(delegate.count >= 13);
    delegate.count = 0;
    view.collapse(par2->index());
    QTRY_VERIFY(delegate.count >= 4);

    // test that the rendering of drag pixmap sets the correct options too (QTBUG-15834)
#ifdef QT_BUILD_INTERNAL
    delegate.count = 0;
    QItemSelection sel(model.index(0,0), model.index(0,modelColumns-1));
    QRect rect;
    view.d_func()->renderToPixmap(sel.indexes(), &rect);
    if (delegate.count != visibleColumns) {
        qDebug() << rect << view.rect() << view.isVisible();
    }
    QTRY_COMPARE(delegate.count, visibleColumns);
#endif

    //test dynamic models
    {
        delegate.count = 0;
        QStandardItemModel model2;
        QStandardItem *item0 = new QStandardItem("OnlyOne Last");
        model2.appendRow(item0);
        view.setModel(&model2);
        QTRY_VERIFY(delegate.count >= 1);

        QStandardItem *item00 = new QStandardItem("OnlyOne Last");
        item0->appendRow(item00);
        item0->setText("OnlyOne Last HasChildren");
        delegate.count = 0;
        view.expandAll();
        QTRY_VERIFY(delegate.count >= 2);

        QStandardItem *item1 = new QStandardItem("OnlyOne Last");
        delegate.count = 0;
        item0->setText("OnlyOne HasChildren");
        model2.appendRow(item1);
        QTRY_VERIFY(delegate.count >= 3);

        QStandardItem *item01 = new QStandardItem("OnlyOne Last");
        delegate.count = 0;
        item00->setText("OnlyOne");
        item0->appendRow(item01);
        QTRY_VERIFY(delegate.count >= 4);

        QStandardItem *item000 = new QStandardItem("OnlyOne Last");
        delegate.count = 0;
        item00->setText("OnlyOne HasChildren");
        item00->appendRow(item000);
        QTRY_VERIFY(delegate.count >= 5);

        delegate.count = 0;
        item0->removeRow(0);
        QTRY_VERIFY(delegate.count >= 3);

        item00 = new QStandardItem("OnlyOne");
        item0->insertRow(0, item00);

        delegate.count = 0;
        view.expandAll();
        QTRY_VERIFY(delegate.count >= 4);

        delegate.count = 0;
        item0->removeRow(1);
        item00->setText("OnlyOne Last");
        QTRY_VERIFY(delegate.count >= 3);

        delegate.count = 0;
        item0->removeRow(0);
        item0->setText("OnlyOne");
        QTRY_VERIFY(delegate.count >= 2);

        //with hidden items
        item0->setText("OnlyOne HasChildren");
        item00 = new QStandardItem("OnlyOne");
        item0->appendRow(item00);
        item01 = new QStandardItem("Assert");
        item0->appendRow(item01);
        view.setRowHidden(1, item0->index(), true);
        view.expandAll();
        QStandardItem *item02 = new QStandardItem("OnlyOne Last");
        item0->appendRow(item02);
        delegate.count = 0;
        QTRY_VERIFY(delegate.count >= 4);

        item0->removeRow(2);
        item00->setText("OnlyOne Last");
        delegate.count = 0;
        QTRY_VERIFY(delegate.count >= 3);

        item00->setText("OnlyOne");
        item0->insertRow(2, new QStandardItem("OnlyOne Last"));
        view.collapse(item0->index());
        item0->removeRow(0);
        delegate.count = 0;
        QTRY_VERIFY(delegate.count >= 2);

        item0->removeRow(1);
        item0->setText("OnlyOne");
        delegate.count = 0;
        QTRY_VERIFY(delegate.count >= 2);
    }
}

class task174627_TreeView : public QTreeView
{
    Q_OBJECT
protected slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &) override
    { emit signalCurrentChanged(current); }
signals:
    void signalCurrentChanged(const QModelIndex &);
};

void tst_QTreeView::task174627_moveLeftToRoot()
{
    QStandardItemModel model;
    QStandardItem *item1 = new QStandardItem(QString("item 1"));
    model.invisibleRootItem()->appendRow(item1);
    QStandardItem *item2 = new QStandardItem(QString("item 2"));
    item1->appendRow(item2);

    task174627_TreeView view;
    view.setModel(&model);
    view.setRootIndex(item1->index());
    view.setCurrentIndex(item2->index());

    QSignalSpy spy(&view, &task174627_TreeView::signalCurrentChanged);
    QTest::keyClick(&view, Qt::Key_Left);
    QCOMPARE(spy.count(), 0);
}

void tst_QTreeView::task171902_expandWith1stColHidden()
{
    //the task was: if the first column of a treeview is hidden, the expanded state is not correctly restored
    QStandardItemModel model;
    QStandardItem root("root"), root2("root"),
        subitem("subitem"), subitem2("subitem"),
        subsubitem("subsubitem"), subsubitem2("subsubitem");

    model.appendRow({ &root, &root2 });
    root.appendRow({ &subitem, &subitem2 });
    subitem.appendRow({ &subsubitem, &subsubitem2 });

    QTreeView view;
    view.setModel(&model);

    view.setColumnHidden(0, true);
    view.expandAll();
    view.collapse(root.index());
    view.expand(root.index());

    QCOMPARE(view.isExpanded(root.index()), true);
    QCOMPARE(view.isExpanded(subitem.index()), true);

}

void tst_QTreeView::task203696_hidingColumnsAndRowsn()
{
    QTreeView view;
    QStandardItemModel model(0, 3);
    for (int i = 0; i < 3; ++i) {
        const QString prefix = QLatin1String("row ") + QString::number(i) + QLatin1String(" col ");
        model.insertRow(model.rowCount());
        for (int j = 0; j < model.columnCount(); ++j)
            model.setData(model.index(i, j), prefix + QString::number(j));
    }
    view.setModel(&model);
    view.show();
    view.setColumnHidden(0, true);
    view.setRowHidden(0, QModelIndex(), true);
    QCOMPARE(view.indexAt(QPoint(0, 0)), model.index(1, 1));
}


void tst_QTreeView::addRowsWhileSectionsAreHidden()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QTreeView view;
    for (int pass = 1; pass <= 2; ++pass) {
        QStandardItemModel *model = new QStandardItemModel(6, pass, &view);
        view.setModel(model);
        view.show();
        QVERIFY(QTest::qWaitForWindowActive(&view));

        for (int i = 0; i < 3; ++i)
        {
            model->insertRow(model->rowCount());
            const QString prefix = QLatin1String("row ") + QString::number(i) + QLatin1String(" col ");
            for (int j = 0; j < model->columnCount(); ++j)
                model->setData(model->index(i, j), prefix + QString::number(j));
        }
        for (int col = 0; col < pass; ++col)
            view.setColumnHidden(col, true);
        for (int i = 3; i < 6; ++i)
        {
            model->insertRow(model->rowCount());
            const QString prefix = QLatin1String("row ") + QString::number(i) + QLatin1String(" col ");
            for (int j = 0; j < model->columnCount(); ++j)
                model->setData(model->index(i, j), prefix + QString::number(j));
        }
        for (int col = 0; col < pass; ++col)
            view.setColumnHidden(col, false);

        auto allVisualRectsValid = [](QTreeView *view, QStandardItemModel *model) {
            for (int i = 0; i < 6; ++i) {
                if (!view->visualRect(model->index(i, 0)).isValid())
                    return false;
            }
            return true;
        };
        QTRY_VERIFY(allVisualRectsValid(&view, model));

        delete model;
    }
}

void tst_QTreeView::task216717_updateChildren()
{
    class Tree : public QTreeWidget
    {
        protected:
            void paintEvent(QPaintEvent *e) override
            {
                QTreeWidget::paintEvent(e);
                refreshed = true;
            }
        public:
            bool refreshed = false;
    } tree;
    tree.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tree));
    tree.refreshed = false;
    QTreeWidgetItem *parent = new QTreeWidgetItem({ "parent" });
    tree.addTopLevelItem(parent);
    QTRY_VERIFY(tree.refreshed);
    tree.refreshed = false;
    parent->addChild(new QTreeWidgetItem({ "child" }));
    QTRY_VERIFY(tree.refreshed);

}

void tst_QTreeView::task220298_selectColumns()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    //this is a very simple 3x3 model where the internalId of the index are different for each cell
    class Model : public QAbstractTableModel
    {
        public:
            int columnCount(const QModelIndex & parent = QModelIndex()) const override
            { return parent.isValid() ? 0 : 3; }
            int rowCount(const QModelIndex & parent = QModelIndex()) const override
            { return parent.isValid() ? 0 : 3; }

            QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override
            {
                if (role == Qt::DisplayRole) {
                    return QVariant(QString::number(index.column()) + QLatin1Char('-')
                        + QString::number(index.row()));
                }
                return QVariant();
            }

            QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override
            {
                return hasIndex(row, column, parent) ? createIndex(row, column, quintptr(column * 10 + row)) : QModelIndex();
            }
    };

    TreeView view;
    Model model;
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {},
                      view.visualRect(view.model()->index(1, 1)).center());
    QTRY_VERIFY(view.selectedIndexes().contains(view.model()->index(1, 2)));
    QVERIFY(view.selectedIndexes().contains(view.model()->index(1, 1)));
    QVERIFY(view.selectedIndexes().contains(view.model()->index(1, 0)));
}


void tst_QTreeView::task224091_appendColumns()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStandardItemModel *model = new QStandardItemModel();
    QWidget* topLevel= new QWidget;
    setFrameless(topLevel);
    QTreeView *treeView = new QTreeView(topLevel);
    treeView->setModel(model);
    topLevel->show();
    treeView->resize(50, 50);
    QApplication::setActiveWindow(topLevel);
    QVERIFY(QTest::qWaitForWindowActive(topLevel));

    QVERIFY(!treeView->verticalScrollBar()->isVisible());

    QList<QStandardItem *> projlist;
    for (int k = 0; k < 10; ++k)
        projlist.append(new QStandardItem(QLatin1String("Top Level ") + QString::number(k)));
    model->appendColumn(projlist);
    model->invisibleRootItem()->appendRow(new QStandardItem("end"));

    QTRY_VERIFY(treeView->verticalScrollBar()->isVisible());

    delete topLevel;
    delete model;
}

void tst_QTreeView::task211293_removeRootIndex()
{
    QTreeView view;
    QStandardItemModel model;
    QStandardItem *A1 = new QStandardItem("A1");
    QStandardItem *B11 = new QStandardItem("B1.1");
    QStandardItem *C111 = new QStandardItem("C1.1.1");
    QStandardItem *C112 = new QStandardItem("C1.1.2");
    QStandardItem *C113 = new QStandardItem("C1.1.3");
    QStandardItem *D1131 = new QStandardItem("D1.1.3.1");
    QStandardItem *E11311 = new QStandardItem("E1.1.3.1.1");
    QStandardItem *E11312 = new QStandardItem("E1.1.3.1.2");
    QStandardItem *E11313 = new QStandardItem("E1.1.3.1.3");
    QStandardItem *E11314 = new QStandardItem("E1.1.3.1.4");
    QStandardItem *D1132 = new QStandardItem("D1.1.3.2");
    QStandardItem *E11321 = new QStandardItem("E1.1.3.2.1");
    D1132->appendRow(E11321);
    D1131->appendRow(E11311);
    D1131->appendRow(E11312);
    D1131->appendRow(E11313);
    D1131->appendRow(E11314);
    C113->appendRow(D1131);
    C113->appendRow(D1132);
    B11->appendRow(C111);
    B11->appendRow(C112);
    B11->appendRow(C113);
    A1->appendRow(B11);
    model.appendRow(A1);
    view.setModel(&model);
    view.setRootIndex(model.indexFromItem(B11));
    view.setExpanded(model.indexFromItem(B11), true);
    view.setCurrentIndex(model.indexFromItem(E11314));
    view.setExpanded(model.indexFromItem(E11314), true);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(model.removeRows(0, 1));
}

void tst_QTreeView::task225539_deleteModel()
{
    QTreeView treeView;
    treeView.show();
    QStandardItemModel *model = new QStandardItemModel(&treeView);

    QStandardItem *parentItem = model->invisibleRootItem();
    QStandardItem *item = new QStandardItem(QString("item"));
    parentItem->appendRow(item);

    treeView.setModel(model);

    QCOMPARE(item->index(), treeView.indexAt(QPoint()));

    delete model;

    QVERIFY(!treeView.indexAt(QPoint()).isValid());
}

void tst_QTreeView::task230123_setItemsExpandable()
{
    //let's check that we prevent the expansion inside a treeview
    //when the property is set.
    QTreeWidget tree;

    QTreeWidgetItem root;
    QTreeWidgetItem child;
    root.addChild(&child);
    tree.addTopLevelItem(&root);

    tree.setCurrentItem(&root);

    tree.setItemsExpandable(false);

    QTest::keyClick(&tree, Qt::Key_Plus);
    QVERIFY(!root.isExpanded());

    QTest::keyClick(&tree, Qt::Key_Right);
    QVERIFY(!root.isExpanded());

    tree.setItemsExpandable(true);

    QTest::keyClick(&tree, Qt::Key_Plus);
    QVERIFY(root.isExpanded());

    QTest::keyClick(&tree, Qt::Key_Minus);
    QVERIFY(!root.isExpanded());

    QTest::keyClick(&tree, Qt::Key_Right);
    QVERIFY(root.isExpanded());

    QTest::keyClick(&tree, Qt::Key_Left);
    QVERIFY(!root.isExpanded());

    QTest::keyClick(&tree, Qt::Key_Right);
    QVERIFY(root.isExpanded());

    const bool navToChild = tree.style()->styleHint(QStyle::SH_ItemView_ArrowKeysNavigateIntoChildren, nullptr, &tree);
    QTest::keyClick(&tree, Qt::Key_Right);
    QCOMPARE(tree.currentItem(), navToChild ? &child : &root);

    QTest::keyClick(&tree, Qt::Key_Right);
    //it should not be expanded: it has no leaf
    QCOMPARE(child.isExpanded(), false);

    QTest::keyClick(&tree, Qt::Key_Left);
    QCOMPARE(tree.currentItem(), &root);

    QTest::keyClick(&tree, Qt::Key_Left);
    QVERIFY(!root.isExpanded());
}

void tst_QTreeView::task202039_closePersistentEditor()
{
    QStandardItemModel model(1, 1);
    QTreeView view;
    view.setModel(&model);

    QModelIndex current = model.index(0,0);
    QTest::mousePress(view.viewport(), Qt::LeftButton, {}, view.visualRect(current).center());
    QTest::mouseDClick(view.viewport(), Qt::LeftButton, {}, view.visualRect(current).center());
    QCOMPARE(view.currentIndex(), current);
    QVERIFY(view.indexWidget(current));

    view.closePersistentEditor(current);
    QVERIFY(!view.indexWidget(current));

    //here was the bug: closing the persistent editor would not reset the state
    //and it was impossible to go into editinon again
    QTest::mousePress(view.viewport(), Qt::LeftButton, {}, view.visualRect(current).center());
    QTest::mouseDClick(view.viewport(), Qt::LeftButton, {}, view.visualRect(current).center());
    QCOMPARE(view.currentIndex(), current);
    QVERIFY(view.indexWidget(current));
}

void tst_QTreeView::task238873_avoidAutoReopening()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStandardItemModel model;

    QStandardItem item0("row 0");
    QStandardItem item1("row 1");
    QStandardItem item2("row 2");
    QStandardItem item3("row 3");
    model.appendColumn( QList<QStandardItem*>() << &item0 << &item1 << &item2 << &item3);

    QStandardItem child("child");
    item1.appendRow( &child);

    QTreeView view;
    view.setModel(&model);
    view.show();
    view.expandAll();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, view.visualRect(child.index()).center());
    QTRY_COMPARE(view.currentIndex(), child.index());

    view.setExpanded(item1.index(), false);

    QTRY_VERIFY(!view.isExpanded(item1.index()));
}

void tst_QTreeView::task244304_clickOnDecoration()
{
    QTreeView view;
    QStandardItemModel model;
    QStandardItem item0("row 0");
    QStandardItem item00("row 0");
    item0.appendRow(&item00);
    QStandardItem item1("row 1");
    model.appendColumn({ &item0, &item1 });
    view.setModel(&model);

    QVERIFY(!view.currentIndex().isValid());
    QRect rect = view.visualRect(item0.index());
    //we click on the decoration
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {},
                      rect.topLeft() + QPoint(-rect.left() / 2, rect.height() / 2));
    QVERIFY(!view.currentIndex().isValid());
    QVERIFY(view.isExpanded(item0.index()));

    rect = view.visualRect(item1.index());
    //the item has no decoration, it should get selected
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {},
                      rect.topLeft() + QPoint(-rect.left() / 2, rect.height() / 2));
    QCOMPARE(view.currentIndex(), item1.index());
}

void tst_QTreeView::task246536_scrollbarsNotWorking()
{
    class MyObject : public QObject
    {
    public:
        using QObject::QObject;
        bool eventFilter(QObject*, QEvent *e) override
        {
            if (e->type() == QEvent::Paint)
                count++;

            return false;
        }
        int count = 0;
    };
    QTreeView tree;
    MyObject o;
    tree.viewport()->installEventFilter(&o);
    QStandardItemModel model;
    tree.setModel(&model);
    tree.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tree));
    QList<QStandardItem *> items;
    for (int i = 0; i < 100; ++i)
        items << new QStandardItem(QLatin1String("item ") + QString::number(i));
    o.count = 0;
    model.invisibleRootItem()->appendColumn(items);
    QTRY_VERIFY(o.count > 0);
    o.count = 0;
    tree.verticalScrollBar()->setValue(50);
    QTRY_VERIFY(o.count > 0);
}

void tst_QTreeView::task250683_wrongSectionSize()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStandardItemModel model;
    populateFakeDirModel(&model);

    QTreeView treeView;
    treeView.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    treeView.setModel(&model);
    treeView.setColumnHidden(2, true);
    treeView.setColumnHidden(3, true);

    treeView.show();
    QVERIFY(QTest::qWaitForWindowActive(&treeView));

    QCOMPARE(treeView.header()->sectionSize(0) + treeView.header()->sectionSize(1), treeView.viewport()->width());
}

void tst_QTreeView::task239271_addRowsWithFirstColumnHidden()
{
    class MyDelegate : public QStyledItemDelegate
    {
    public:
        void paint(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
        {
            paintedIndexes << index;
            QStyledItemDelegate::paint(painter, option, index);
        }
        mutable QSet<QModelIndex> paintedIndexes;
    };

    QTreeView view;
    QStandardItemModel model;
    view.setModel(&model);
    MyDelegate delegate;
    view.setItemDelegate(&delegate);
    QStandardItem root0("root0"), root1("root1");
    model.invisibleRootItem()->appendRow(QList<QStandardItem*>() << &root0 << &root1);
    QStandardItem sub0("sub0"), sub00("sub00");
    root0.appendRow(QList<QStandardItem*>() << &sub0 << &sub00);
    view.expand(root0.index());

    view.hideColumn(0);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    delegate.paintedIndexes.clear();
    QStandardItem sub1("sub1"), sub11("sub11");
    root0.appendRow(QList<QStandardItem*>() << &sub1 << &sub11);

    //items in the 2nd column should have been painted
    QTRY_VERIFY(!delegate.paintedIndexes.isEmpty());
    QVERIFY(delegate.paintedIndexes.contains(sub00.index()));
    QVERIFY(delegate.paintedIndexes.contains(sub11.index()));
}

void tst_QTreeView::task254234_proxySort()
{
    //based on tst_QTreeView::sortByColumn
    // it used not to work when setting the source of a proxy after enabling sorting
    QTreeView view;
    QStandardItemModel model(4, 2);
    model.setItem(0, 0, new QStandardItem("b"));
    model.setItem(1, 0, new QStandardItem("d"));
    model.setItem(2, 0, new QStandardItem("c"));
    model.setItem(3, 0, new QStandardItem("a"));
    model.setItem(0, 1, new QStandardItem("e"));
    model.setItem(1, 1, new QStandardItem("g"));
    model.setItem(2, 1, new QStandardItem("h"));
    model.setItem(3, 1, new QStandardItem("f"));

    view.sortByColumn(1, Qt::DescendingOrder);
    view.setSortingEnabled(true);

    QSortFilterProxyModel proxy;
    proxy.setDynamicSortFilter(true);
    view.setModel(&proxy);
    proxy.setSourceModel(&model);
    QCOMPARE(view.header()->sortIndicatorSection(), 1);
    QCOMPARE(view.model()->data(view.model()->index(0, 1)).toString(), QString::fromLatin1("h"));
    QCOMPARE(view.model()->data(view.model()->index(1, 1)).toString(), QString::fromLatin1("g"));
}

void tst_QTreeView::task248022_changeSelection()
{
    //we check that changing the selection between the mouse press and the mouse release
    //works correctly
    TreeView view;
    const QStringList list({"1", "2"});
    QStringListModel model(list);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.setModel(&model);
    connect(view.selectionModel(), &QItemSelectionModel::selectionChanged,
            &view, &TreeView::handleSelectionChanged);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {},
                      view.visualRect(model.index(1)).center());
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), list.count());
}

void tst_QTreeView::task245654_changeModelAndExpandAll()
{
    QTreeView view;
    QScopedPointer<QStandardItemModel> model(new QStandardItemModel);
    QStandardItem *top = new QStandardItem("top");
    QStandardItem *sub = new QStandardItem("sub");
    top->appendRow(sub);
    model->appendRow(top);
    view.setModel(model.data());
    view.expandAll();
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(view.isExpanded(top->index()));

    //now let's try to delete the model
    //then repopulate and expand again
    model.reset(new QStandardItemModel);
    top = new QStandardItem("top");
    sub = new QStandardItem("sub");
    top->appendRow(sub);
    model->appendRow(top);
    view.setModel(model.data());
    view.expandAll();
    QTRY_VERIFY(view.isExpanded(top->index()));
}

void tst_QTreeView::doubleClickedWithSpans()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QTreeView view;
    QStandardItemModel model(1, 2);
    view.setModel(&model);
    view.setFirstColumnSpanned(0, QModelIndex(), true);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QVERIFY(view.isActiveWindow());

    QPoint p(10, 10);
    QCOMPARE(view.indexAt(p), model.index(0, 0));
    QSignalSpy spy(&view, &QAbstractItemView::doubleClicked);
    QTest::mousePress(view.viewport(), Qt::LeftButton, {}, p);
    QTest::mouseDClick(view.viewport(), Qt::LeftButton, {}, p);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, {}, p);
    QCOMPARE(spy.count(), 1);

    //let's click on the 2nd column
    p.setX(p.x() + view.header()->sectionSize(0));
    QCOMPARE(view.indexAt(p), model.index(0, 0));

    //end the previous edition
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, p);
    QTest::mousePress(view.viewport(), Qt::LeftButton, {}, p);
    QTest::mouseDClick(view.viewport(), Qt::LeftButton, {}, p);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, {}, p);
    QTRY_COMPARE(spy.count(), 2);
}

void tst_QTreeView::taskQTBUG_6450_selectAllWith1stColumnHidden()
{
    QTreeWidget tree;
    tree.setSelectionMode(QAbstractItemView::MultiSelection);
    tree.setColumnCount(2);
    QList<QTreeWidgetItem *> items;
    const int nrRows = 10;
    for (int i = 0; i < nrRows; ++i) {
        const QString text = QLatin1String("item: ") + QString::number(i);
        items.append(new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr),
                                         QStringList(text)));
        items.last()->setText(1, QString("is an item"));
    }
    tree.insertTopLevelItems(0, items);

    tree.hideColumn(0);
    tree.selectAll();

    QVERIFY(tree.selectionModel()->hasSelection());
    for (int i = 0; i < nrRows; ++i)
        QVERIFY(tree.selectionModel()->isRowSelected(i, QModelIndex()));
}

class TreeViewQTBUG_9216 : public QTreeView
{
    Q_OBJECT
public:
    void paintEvent(QPaintEvent *event) override
    {
        if (doCompare)
            QCOMPARE(event->rect(), viewport()->rect());
        QTreeView::paintEvent(event);
        painted++;
    }
    int painted = 0;
    bool doCompare = false;
};

void tst_QTreeView::taskQTBUG_9216_setSizeAndUniformRowHeightsWrongRepaint()
{
    QStandardItemModel model(10, 10, this);
    for (int row = 0; row < 10; row++) {
        const QString prefix = QLatin1String("row ") + QString::number(row) + QLatin1String(", col ");
        for (int col = 0; col < 10; col++)
            model.setItem(row, col, new QStandardItem(prefix + QString::number(col)));
    }
    TreeViewQTBUG_9216 view;
    view.setUniformRowHeights(true);
    view.setModel(&model);
    view.painted = 0;
    view.doCompare = false;
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(view.painted > 0);

    QTest::qWait(100);  // This one is needed to make the test fail before the patch.
    view.painted = 0;
    view.doCompare = true;
    model.setData(model.index(0, 0), QVariant(QSize(50, 50)), Qt::SizeHintRole);
    QTRY_VERIFY(view.painted > 0);
}

void tst_QTreeView::keyboardNavigationWithDisabled()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QWidget topLevel;
    QTreeView view(&topLevel);
    QStandardItemModel model(90, 0);
    for (int i = 0; i < 90; i ++) {
        model.setItem(i, new QStandardItem(QString::number(i)));
        model.item(i)->setEnabled(i % 6 == 0);
    }
    view.setModel(&model);

    view.resize(200, view.visualRect(model.index(0,0)).height()*10);
    topLevel.show();
    QApplication::setActiveWindow(&topLevel);
    QVERIFY(QTest::qWaitForWindowActive(&topLevel));
    QVERIFY(topLevel.isActiveWindow());

    view.setCurrentIndex(model.index(1, 0));
    QTest::keyClick(view.viewport(), Qt::Key_Up);
    QCOMPARE(view.currentIndex(), model.index(0, 0));
    QTest::keyClick(view.viewport(), Qt::Key_Down);
    QCOMPARE(view.currentIndex(), model.index(6, 0));
    QTest::keyClick(view.viewport(), Qt::Key_PageDown);
    QCOMPARE(view.currentIndex(), model.index(18, 0));
    QTest::keyClick(view.viewport(), Qt::Key_Down);
    QCOMPARE(view.currentIndex(), model.index(24, 0));
    QTest::keyClick(view.viewport(), Qt::Key_PageUp);
    QCOMPARE(view.currentIndex(), model.index(12, 0));
    QTest::keyClick(view.viewport(), Qt::Key_Up);
    QCOMPARE(view.currentIndex(), model.index(6, 0));
    // QTBUG-44746 - when first/last item is disabled,
    // Key_PageUp/Down/Home/End will not work as expected.
    model.item(0)->setEnabled(false);
    model.item(1)->setEnabled(true);
    model.item(2)->setEnabled(true);
    model.item(model.rowCount() - 1)->setEnabled(false);
    model.item(model.rowCount() - 2)->setEnabled(true);
    model.item(model.rowCount() - 3)->setEnabled(true);
    // PageUp
    view.setCurrentIndex(model.index(2, 0));
    QCOMPARE(view.currentIndex(), model.index(2, 0));
    QTest::keyClick(view.viewport(), Qt::Key_PageUp);
    QCOMPARE(view.currentIndex(), model.index(1, 0));
    // PageDown
    view.setCurrentIndex(model.index(model.rowCount() - 3, 0));
    QCOMPARE(view.currentIndex(), model.index(model.rowCount() - 3, 0));
    QTest::keyClick(view.viewport(), Qt::Key_PageDown);
    QCOMPARE(view.currentIndex(), model.index(model.rowCount() - 2, 0));
    // Key_Home
    QTest::keyClick(view.viewport(), Qt::Key_Home);
    QCOMPARE(view.currentIndex(), model.index(1, 0));
    // Key_End
    QTest::keyClick(view.viewport(), Qt::Key_End);
    QCOMPARE(view.currentIndex(), model.index(model.rowCount() - 2, 0));
}

class RemoveColumnOne : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    bool filterAcceptsColumn(int source_column, const QModelIndex &) const override
    {
        if (m_removeColumn)
            return source_column != 1;
        return true;
    }
    void removeColumn()
    {
        m_removeColumn = true;
        invalidate();
    }
private:
    bool m_removeColumn = false;
};


void tst_QTreeView::saveRestoreState()
{
    QStandardItemModel model;
    for (int i = 0; i < 100; i++) {
        model.appendRow({new QStandardItem(QStringLiteral("item ") + QString::number(i)),
                         new QStandardItem(QStringLiteral("hidden by proxy")),
                         new QStandardItem(QStringLiteral("hidden by user")) });
    }
    QCOMPARE(model.columnCount(), 3);

    RemoveColumnOne proxy;
    proxy.setSourceModel(&model);
    QCOMPARE(proxy.columnCount(), 3);

    QTreeView view;
    view.setModel(&proxy);
    view.resize(500, 500);
    view.show();
    view.header()->hideSection(2);
    QVERIFY(view.header()->isSectionHidden(2));
    proxy.removeColumn();
    QCOMPARE(proxy.columnCount(), 2);
    QVERIFY(view.header()->isSectionHidden(1));
    const QByteArray data = view.header()->saveState();

    QTreeView view2;
    view2.setModel(&proxy);
    view2.resize(500, 500);
    view2.show();
    view2.header()->restoreState(data);
    QVERIFY(view2.header()->isSectionHidden(1));
}

class Model_11466 : public QAbstractItemModel
{
    Q_OBJECT
public:
    Model_11466(QObject *parent = nullptr) : QAbstractItemModel(parent)
        , m_selectionModel(new QItemSelectionModel(this, this))
    {
        connect(m_selectionModel, &QItemSelectionModel::currentChanged,
                this, &Model_11466::slotCurrentChanged);
    }

    int rowCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return (parent.internalId() == 0) ? 4 : 0;
        return 2; // two top level items
    }

    int columnCount(const QModelIndex & /* parent */) const override
    {
        return 2;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::DisplayRole && index.isValid()) {
            qint64 parentRowPlusOne = qint64(index.internalId());
            QString str;
            QTextStream stream(&str);
            if (parentRowPlusOne > 0)
                stream << parentRowPlusOne << " -> " << index.row() << " : " << index.column();
            else
                stream << index.row() << " : " << index.column();
            return QVariant(str);
        }
        return QVariant();
    }

    QModelIndex parent(const QModelIndex &index) const override
    {
        if (index.isValid()) {
            qint64 parentRowPlusOne = qint64(index.internalId());
            if (parentRowPlusOne > 0) {
                int row = static_cast<int>(parentRowPlusOne - 1);
                return createIndex(row, 0);
            }
        }
        return QModelIndex();
    }

    void bindView(QTreeView *view)
    {
        // sets the view to this model with a shared selection model
        QItemSelectionModel *oldModel = view->selectionModel();
        if (oldModel != m_selectionModel)
            delete oldModel;
        view->setModel(this); // this creates a new selection model for the view, but we don't want it either ...
        oldModel = view->selectionModel();
        view->setSelectionModel(m_selectionModel);
        delete oldModel;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        return createIndex(row, column, parent.isValid() ? quintptr(parent.row() + 1) : quintptr(0));
    }

public slots:
    void slotCurrentChanged(const QModelIndex &current,const QModelIndex &)
    {
        if (m_block)
            return;

        if (current.isValid()) {
            int selectedRow = current.row();
            const quintptr parentRowPlusOne = current.internalId();

            for (int i = 0; i < 2; ++i) {
                // announce the removal of all non top level items
                beginRemoveRows(createIndex(i, 0), 0, 3);
                // nothing to actually do for the removal
                endRemoveRows();

                // put them back in again
                beginInsertRows(createIndex(i, 0), 0, 3);
                // nothing to actually do for the insertion
                endInsertRows();
            }
            // reselect the current item ...
            QModelIndex selectedIndex = createIndex(selectedRow, 0, parentRowPlusOne);

            m_block = true; // recursion block
            m_selectionModel->select(selectedIndex, QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Current|QItemSelectionModel::Rows);
            m_selectionModel->setCurrentIndex(selectedIndex, QItemSelectionModel::NoUpdate);
            m_block = false;
        } else {
            m_selectionModel->clear();
        }
    }

private:
    bool m_block = false;
    QItemSelectionModel *m_selectionModel;
};

void tst_QTreeView::taskQTBUG_11466_keyboardNavigationRegression()
{
    QTreeView treeView;
    treeView.setSelectionBehavior(QAbstractItemView::SelectRows);
    treeView.setSelectionMode(QAbstractItemView::SingleSelection);
    Model_11466 model(&treeView);
    model.bindView(&treeView);
    treeView.expandAll();
    treeView.show();
    QVERIFY(QTest::qWaitForWindowExposed(&treeView));

    QTest::keyPress(treeView.viewport(), Qt::Key_Down);
    QTRY_COMPARE(treeView.currentIndex(), treeView.selectionModel()->selection().indexes().first());
}

void tst_QTreeView::taskQTBUG_13567_removeLastItemRegression()
{
    QtTestModel model(200, 1);

    QTreeView view;
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    view.scrollToBottom();
    QTest::qWait(10);
    CHECK_VISIBLE(199, 0);

    view.setCurrentIndex(model.index(199, 0));
    model.removeLastRow();
    QTRY_COMPARE(view.currentIndex(), model.index(198, 0));
    CHECK_VISIBLE(198, 0);
}

// From QTBUG-25333 (QTreeWidget drag crashes when there was a hidden item in tree)
// The test passes simply if it doesn't crash, hence there are no calls
// to QCOMPARE() or QVERIFY().
// Note: define QT_BUILD_INTERNAL to run this test
void tst_QTreeView::taskQTBUG_25333_adjustViewOptionsForIndex()
{
    QTreeView view;
    QStandardItemModel model;
    QStandardItem *item1 = new QStandardItem("Item1");
    QStandardItem *item2 = new QStandardItem("Item2");
    QStandardItem *item3 = new QStandardItem("Item3");
    QStandardItem *data1 = new QStandardItem("Data1");
    QStandardItem *data2 = new QStandardItem("Data2");
    QStandardItem *data3 = new QStandardItem("Data3");

    // Create a treeview
    model.appendRow({ item1, data1 });
    model.appendRow({ item2, data2 });
    model.appendRow({ item3, data3 });

    view.setModel(&model);

    // Hide a row
    view.setRowHidden(1, QModelIndex(), true);
    view.expandAll();

    view.show();

#ifdef QT_BUILD_INTERNAL
    {
        QStyleOptionViewItem option;

        view.d_func()->adjustViewOptionsForIndex(&option, model.indexFromItem(item1));

        view.d_func()->adjustViewOptionsForIndex(&option, model.indexFromItem(item3));
    }
#endif

}

void tst_QTreeView::taskQTBUG_18539_emitLayoutChanged()
{
    qRegisterMetaType<QList<QPersistentModelIndex>>();
    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>();

    QTreeView view;

    QStandardItem* item = new QStandardItem("Orig");
    QStandardItem* child = new QStandardItem("Child");
    item->setChild(0, 0, child);

    QStandardItemModel model;
    model.appendRow(item);

    view.setModel(&model);

    QStandardItem* replacementItem = new QStandardItem("Replacement");
    QStandardItem* replacementChild = new QStandardItem("ReplacementChild");

    replacementItem->setChild(0, 0, replacementChild);

    QSignalSpy beforeSpy(&model, &QAbstractItemModel::layoutAboutToBeChanged);
    QSignalSpy afterSpy(&model, &QAbstractItemModel::layoutChanged);

    QSignalSpy beforeRISpy(&model, &QAbstractItemModel::rowsAboutToBeInserted);
    QSignalSpy afterRISpy(&model, &QAbstractItemModel::rowsInserted);

    QSignalSpy beforeRRSpy(&model, &QAbstractItemModel::rowsAboutToBeRemoved);
    QSignalSpy afterRRSpy(&model, &QAbstractItemModel::rowsRemoved);

    model.setItem(0, 0, replacementItem);

    QCOMPARE(beforeSpy.size(), 1);
    QCOMPARE(afterSpy.size(), 1);

    QCOMPARE(beforeRISpy.size(), 0);
    QCOMPARE(afterRISpy.size(), 0);

    QCOMPARE(beforeRISpy.size(), 0);
    QCOMPARE(afterRISpy.size(), 0);
}

void tst_QTreeView::taskQTBUG_8176_emitOnExpandAll()
{
    QTreeWidget tw;
    QTreeWidgetItem *item = new QTreeWidgetItem(&tw, QStringList(QString("item 1")));
    QTreeWidgetItem *item2 = new QTreeWidgetItem(item, QStringList(QString("item 2")));
    new QTreeWidgetItem(item2, QStringList(QString("item 3")));
    new QTreeWidgetItem(item2, QStringList(QString("item 4")));
    QTreeWidgetItem *item5 = new QTreeWidgetItem(&tw, QStringList(QString("item 5")));
    new QTreeWidgetItem(item5, QStringList(QString("item 6")));
    QSignalSpy spy(&tw, &QTreeView::expanded);

    // expand all
    tw.expandAll();
    QCOMPARE(spy.size(), 6);
    spy.clear();
    tw.collapseAll();
    item2->setExpanded(true);
    spy.clear();
    tw.expandAll();
    QCOMPARE(spy.size(), 5);

    // collapse all
    QSignalSpy spy2(&tw, &QTreeView::collapsed);
    tw.collapseAll();
    QCOMPARE(spy2.size(), 6);
    tw.expandAll();
    item2->setExpanded(false);
    spy2.clear();
    tw.collapseAll();
    QCOMPARE(spy2.size(), 5);

    // expand to depth
    item2->setExpanded(true);
    spy.clear();
    spy2.clear();
    tw.expandToDepth(0);

    QCOMPARE(spy.size(), 2); // item and item5 are expanded
    QCOMPARE(spy2.size(), 1); // item2 is collapsed
}

void tst_QTreeView::testInitialFocus()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QTreeWidget treeWidget;
    treeWidget.setColumnCount(5);
    new QTreeWidgetItem(&treeWidget, QString("1;2;3;4;5").split(QLatin1Char(';')));
    treeWidget.setTreePosition(2);
    treeWidget.header()->hideSection(0);      // make sure we skip hidden section(s)
    treeWidget.header()->swapSections(1, 2);  // make sure that we look for first visual index (and not first logical)
    treeWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&treeWidget));
    QTRY_COMPARE(treeWidget.currentIndex().column(), 2);
}

#if QT_CONFIG(animation)
void tst_QTreeView::quickExpandCollapse()
{
    //this unit tests makes sure the state after the animation is restored correctly
    //after starting a 2nd animation while the first one was still on-going
    //this tests that the stateBeforeAnimation is not set to AnimatingState
    QTreeView tree;
    tree.setAnimated(true);
    QStandardItemModel model;
    QStandardItem *root = new QStandardItem("root");
    root->appendRow(new QStandardItem("subnode"));
    model.appendRow(root);
    tree.setModel(&model);

    QModelIndex rootIndex = root->index();
    QVERIFY(rootIndex.isValid());

    tree.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tree));

    const QAbstractItemView::State initialState = tree.state();

    tree.expand(rootIndex);
    QCOMPARE(tree.state(), QTreeView::AnimatingState);

    tree.collapse(rootIndex);
    QCOMPARE(tree.state(), QTreeView::AnimatingState);

    //the animation lasts for 250ms max so 5000 (default) should be enough
    QTRY_COMPARE(tree.state(), initialState);
}
#endif // animation

void tst_QTreeView::taskQTBUG_37813_crash()
{
    // QTBUG_37813: Crash in visual / logical index mapping in QTreeViewPrivate::adjustViewOptionsForIndex()
    // when hiding/moving columns. It is reproduceable with a QTreeWidget only.
#ifdef QT_BUILD_INTERNAL
    QTreeWidget treeWidget;
    treeWidget.setDragEnabled(true);
    treeWidget.setColumnCount(2);
    QList<QTreeWidgetItem *> items;
    for (int r = 0; r < 2; ++r) {
        const QString prefix = QLatin1String("Row ") + QString::number(r) + QLatin1String(" Column ");
        QTreeWidgetItem *item = new QTreeWidgetItem();
        for (int c = 0; c < treeWidget.columnCount(); ++c)
            item->setText(c, prefix + QString::number(c));
        items.append(item);
    }
    treeWidget.addTopLevelItems(items);
    treeWidget.setColumnHidden(0, true);
    treeWidget.header()->moveSection(0, 1);
    QItemSelection sel(treeWidget.model()->index(0, 0), treeWidget.model()->index(0, 1));
    QRect rect;
    QAbstractItemViewPrivate *av = static_cast<QAbstractItemViewPrivate*>(qt_widget_private(&treeWidget));
    const QPixmap pixmap = av->renderToPixmap(sel.indexes(), &rect);
    QVERIFY(pixmap.size().isValid());
#endif // QT_BUILD_INTERNAL
}

// QTBUG-45697: Using a QTreeView with a multi-column model filtered by QSortFilterProxyModel,
// when sorting the source model while the widget is not yet visible and showing the widget
// later on, corruption occurs in QTreeView.
class Qtbug45697TestWidget : public QWidget
{
   Q_OBJECT
public:
    static const int columnCount = 3;

    explicit Qtbug45697TestWidget(QWidget *parent = nullptr);
    int timerTick() const { return m_timerTick; }

public slots:
    void slotTimer();

private:
   QTreeView *m_treeView;
   QStandardItemModel *m_model;
   QSortFilterProxyModel *m_sortFilterProxyModel;
   int m_timerTick = 0;
};

Qtbug45697TestWidget::Qtbug45697TestWidget(QWidget *parent)
    : QWidget(parent), m_treeView(new QTreeView(this))
    , m_model(new QStandardItemModel(0, Qtbug45697TestWidget::columnCount, this))
    , m_sortFilterProxyModel(new QSortFilterProxyModel(this))
 {
   QVBoxLayout *vBoxLayout = new QVBoxLayout(this);
   vBoxLayout->addWidget(m_treeView);

   for (char sortChar = 'z'; sortChar >= 'a' ; --sortChar) {
       QList<QStandardItem *>  items;
       for (int column = 0; column < Qtbug45697TestWidget::columnCount; ++column) {
           const QString text = QLatin1Char(sortChar) + QLatin1String(" ") + QString::number(column);
           items.append(new QStandardItem(text));
       }
       m_model->appendRow(items);
   }

   m_sortFilterProxyModel->setSourceModel(m_model);
   m_treeView->setModel(m_sortFilterProxyModel);

   QHeaderView *headerView = m_treeView->header();
   for (int s = 1, lastSection = headerView->count() - 1; s < lastSection; ++s)
       headerView->setSectionResizeMode(s, QHeaderView::ResizeToContents);

   QTimer *timer = new QTimer(this);
   timer->setInterval(50);
   connect(timer, &QTimer::timeout, this, &Qtbug45697TestWidget::slotTimer);
   timer->start();
}

void Qtbug45697TestWidget::slotTimer()
{
    switch (m_timerTick++) {
    case 0:
        m_model->sort(0);
        break;
    case 1:
        show();
        break;
    default:
        close();
        break;
    }
}

void tst_QTreeView::taskQTBUG_45697_crash()
{
    Qtbug45697TestWidget testWidget;
    testWidget.setWindowTitle(QTest::currentTestFunction());
    testWidget.resize(400, 400);
    testWidget.move(QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(100, 100));
    QTRY_VERIFY(testWidget.timerTick() >= 2);
}

void tst_QTreeView::taskQTBUG_7232_AllowUserToControlSingleStep()
{
    // When we set the scrollMode to ScrollPerPixel it will adjust the scrollbars singleStep automatically
    // Setting a singlestep on a scrollbar should however imply that the user takes control.
    // Setting a singlestep to -1 return to an automatic control of the singleStep.
    QTreeWidget t;
    t.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    t.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    t.setColumnCount(2);
    QTreeWidgetItem *mainItem = new QTreeWidgetItem(&t, QStringList() << "Root");
    for (int i = 0; i < 200; ++i) {
        QTreeWidgetItem *item = new QTreeWidgetItem(mainItem, QStringList(QString("Item")));
        new QTreeWidgetItem(item, QStringList() << "Child" << "1");
        new QTreeWidgetItem(item, QStringList() << "Child" << "2");
        new QTreeWidgetItem(item, QStringList() << "Child" << "3");
    }
    t.expandAll();

    t.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    t.setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    t.setGeometry(200, 200, 200, 200);
    int vStep1 = t.verticalScrollBar()->singleStep();
    int hStep1 = t.horizontalScrollBar()->singleStep();
    QVERIFY(vStep1 > 1);
    QVERIFY(hStep1 > 1);

    t.verticalScrollBar()->setSingleStep(1);
    t.setGeometry(300, 300, 300, 300);
    QCOMPARE(t.verticalScrollBar()->singleStep(), 1);

    t.horizontalScrollBar()->setSingleStep(1);
    t.setGeometry(400, 400, 400, 400);
    QCOMPARE(t.horizontalScrollBar()->singleStep(), 1);

    t.setGeometry(200, 200, 200, 200);
    t.verticalScrollBar()->setSingleStep(-1);
    t.horizontalScrollBar()->setSingleStep(-1);
    QCOMPARE(vStep1, t.verticalScrollBar()->singleStep());
    QCOMPARE(hStep1, t.horizontalScrollBar()->singleStep());
}

void tst_QTreeView::statusTip_data()
{
    QTest::addColumn<bool>("intermediateParent");
    QTest::newRow("noIntermediate") << false;
    QTest::newRow("intermediate") << true;
}

void tst_QTreeView::statusTip()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QFETCH(bool, intermediateParent);
    QMainWindow mw;
    QtTestModel model(5, 5);
    model.statusTipsEnabled = true;
    QTreeView *view = new QTreeView;
    view->setModel(&model);
    view->viewport()->setMouseTracking(true);
    view->header()->viewport()->setMouseTracking(true);
    if (intermediateParent) {
        QWidget *inter = new QWidget;
        QVBoxLayout *vbox = new QVBoxLayout;
        inter->setLayout(vbox);
        vbox->addWidget(view);
        mw.setCentralWidget(inter);
    } else {
        mw.setCentralWidget(view);
    }
    mw.statusBar();
    mw.setGeometry(QRect(QPoint(QApplication::desktop()->geometry().center() - QPoint(250, 250)),
                                QSize(500, 500)));
    mw.show();
    QApplication::setActiveWindow(&mw);
    QVERIFY(QTest::qWaitForWindowActive(&mw));
    // Ensure it is moved away first and then moved to the relevant section
    QTest::mouseMove(mw.windowHandle(), view->mapTo(&mw, view->rect().bottomLeft() + QPoint(20, 20)));
    QPoint centerPoint = view->viewport()->mapTo(&mw, view->visualRect(model.index(0, 0)).center());
    QTest::mouseMove(mw.windowHandle(), centerPoint);
    QTRY_COMPARE(mw.statusBar()->currentMessage(), QLatin1String("[0,0,0] -- Status"));
    centerPoint = view->viewport()->mapTo(&mw, view->visualRect(model.index(0, 1)).center());
    QTest::mouseMove(mw.windowHandle(), centerPoint);
    QTRY_COMPARE(mw.statusBar()->currentMessage(), QLatin1String("[0,1,0] -- Status"));
    centerPoint = view->header()->viewport()->mapTo(&mw,
                    QPoint(view->header()->sectionViewportPosition(0) + view->header()->sectionSize(0) / 2,
                           view->header()->height() / 2));
    QTest::mouseMove(mw.windowHandle(), centerPoint);
    QTRY_COMPARE(mw.statusBar()->currentMessage(), QLatin1String("Header 0 -- Status"));
}

class FetchMoreModel : public QStandardItemModel
{
    Q_OBJECT
public:
    FetchMoreModel(QObject *parent = nullptr) : QStandardItemModel(parent)
    {
        for (int i = 0; i < 20; ++i) {
            QStandardItem *item = new QStandardItem("Row");
            item->appendRow(new QStandardItem("Child"));
            appendRow(item);
        }
    }
    bool canFetchMore(const QModelIndex &parent) const override
    {
        if (!canFetchReady || !parent.isValid())
            return false;
        if (!parent.parent().isValid())
            return rowCount(parent) < 20;
        return false;
    }
    void fetchMore(const QModelIndex &parent) override
    {
        QStandardItem *item = itemFromIndex(parent);
        for (int i = 0; i < 19; ++i)
            item->appendRow(new QStandardItem(QStringLiteral("New Child ") + QString::number(i)));
    }
    bool canFetchReady = false;
};

void tst_QTreeView::fetchMoreOnScroll()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QTreeView tw;
    FetchMoreModel im;
    tw.setModel(&im);
    tw.show();
    tw.expandAll();
    QVERIFY(QTest::qWaitForWindowActive(&tw));
    // Now we can allow the fetch to happen
    im.canFetchReady = true;
    tw.verticalScrollBar()->setValue(tw.verticalScrollBar()->maximum());
    // The item should have now fetched the other children, thus bringing the count to 20
    QCOMPARE(im.item(19)->rowCount(), 20);
}

static void fillModeltaskQTBUG_8376(QAbstractItemModel &model)
{
    model.insertRow(0);
    model.insertColumn(0);
    model.insertColumn(1);
    QModelIndex index = model.index(0, 0);
    model.setData(index, "Level0");
    {
        model.insertRow(0, index);
        model.insertRow(1, index);
        model.insertColumn(0, index);
        model.insertColumn(1, index);

        QModelIndex idx;
        idx = model.index(0, 0, index);
        model.setData(idx, "Level1");

        idx = model.index(0, 1, index);
        model.setData(idx, "very\nvery\nhigh\ncell");
    }
}

void tst_QTreeView::taskQTBUG_8376()
{
    QTreeView tv;
    QStandardItemModel model;
    fillModeltaskQTBUG_8376(model);
    tv.setModel(&model);
    tv.expandAll(); // init layout

    QModelIndex idxLvl0 = model.index(0, 0);
    QModelIndex idxLvl1 = model.index(0, 1, idxLvl0);
    const int rowHeightLvl0 = tv.rowHeight(idxLvl0);
    const int rowHeightLvl1Visible = tv.rowHeight(idxLvl1);
    QVERIFY(rowHeightLvl0 < rowHeightLvl1Visible);

    tv.hideColumn(1);
    const int rowHeightLvl1Hidden = tv.rowHeight(idxLvl1);
    QCOMPARE(rowHeightLvl0, rowHeightLvl1Hidden);

    tv.showColumn(1);
    const int rowHeightLvl1Visible2 = tv.rowHeight(idxLvl1);
    QCOMPARE(rowHeightLvl1Visible, rowHeightLvl1Visible2);
}

void tst_QTreeView::taskQTBUG_61476()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // This checks that if a user clicks on an item to collapse it that it
    // does not edit (in this case change the check state) the item that is
    // now over the mouse just because it got a release event
    QTreeView tv;
    QStandardItemModel model;
    QStandardItem *lastTopLevel = nullptr;
    {
        for (int i = 0; i < 4; ++i) {
            QStandardItem *item = new QStandardItem(QLatin1String("Row Item"));
            item->setCheckable(true);
            item->setCheckState(Qt::Checked);
            model.appendRow(item);
            lastTopLevel = item;
            for (int j = 0; j < 2; ++j) {
                QStandardItem *childItem = new QStandardItem(QLatin1String("Child row Item"));
                childItem->setCheckable(true);
                childItem->setCheckState(Qt::Checked);
                item->appendRow(childItem);
                QStandardItem *grandChild = new QStandardItem(QLatin1String("Grand child row Item"));
                grandChild->setCheckable(true);
                grandChild->setCheckState(Qt::Checked);
                childItem->appendRow(grandChild);
            }
        }
    }
    tv.setModel(&model);
    tv.expandAll();
    // We need it to be this size so that the effect of the collapsing will
    // cause the parent item to move to be under the cursor
    tv.resize(200, 200);
    tv.show();
    QVERIFY(QTest::qWaitForWindowActive(&tv));
    tv.verticalScrollBar()->setValue(tv.verticalScrollBar()->maximum());

    // We want to press specifically right around where a checkbox for the
    // parent item could be when collapsing
    QTreeViewPrivate *priv = static_cast<QTreeViewPrivate*>(qt_widget_private(&tv));
    const QModelIndex mi = lastTopLevel->child(0)->index();
    const QRect rect = priv->itemDecorationRect(mi);
    const QPoint pos = rect.center();

    QTest::mousePress(tv.viewport(), Qt::LeftButton, {}, pos);
    const bool expandsOnPress =
        (tv.style()->styleHint(QStyle::SH_ListViewExpand_SelectMouseType, nullptr, &tv) == QEvent::MouseButtonPress);
    if (expandsOnPress)
        QTRY_VERIFY(!tv.isExpanded(mi));

    QTest::mouseRelease(tv.viewport(), Qt::LeftButton, {}, pos);
    QTRY_VERIFY(!tv.isExpanded(mi));
    QCOMPARE(lastTopLevel->checkState(), Qt::Checked);

    // Test that it does not toggle the check state of a previously selected item when collapsing an
    // item causes it to position the item under the mouse to be the decoration for the selected item
    tv.expandAll();
    tv.verticalScrollBar()->setValue(tv.verticalScrollBar()->maximum());
    // It is not enough to programmatically select the item, we need to have it clicked on
    QTest::mouseClick(tv.viewport(), Qt::LeftButton, {}, tv.visualRect(lastTopLevel->index()).center());
    QTest::mousePress(tv.viewport(), Qt::LeftButton, {}, pos);
    if (expandsOnPress)
        QTRY_VERIFY(!tv.isExpanded(mi));
    QTest::mouseRelease(tv.viewport(), Qt::LeftButton, nullptr, pos);
    QTRY_VERIFY(!tv.isExpanded(mi));
    QCOMPARE(lastTopLevel->checkState(), Qt::Checked);
}

void tst_QTreeView::fetchUntilScreenFull()
{
    class TreeModel : public QAbstractItemModel
    {
    public:
        const int maxChildren = 49;
        explicit TreeModel(QObject* parent = nullptr) : QAbstractItemModel(parent)
        {
            QVariant rootData1("Parent Col 1");
            QVariant rootData2("Parent Col 2");
            QVector<QVariant> rootData;
            rootData.append(rootData1);
            rootData.append(rootData2);

            m_root = new TreeItem(rootData, nullptr);

            QVariant childData1("Col 1");
            QVariant childData2("Col 2");
            QVector<QVariant> childData;
            childData.append(childData1);
            childData.append(childData2);

            TreeItem* item_1 = new TreeItem(childData, m_root);
            m_root->children.append(item_1);

            TreeItem* item_2 = new TreeItem(childData, item_1);
            item_1->children.append(item_2);
        }

        QModelIndex index(const int row, const int column,
            const QModelIndex& parent = QModelIndex()) const override
        {
            if (!hasIndex(row, column, parent))
                return QModelIndex();

            TreeItem* parentItem =
                parent.isValid() ? static_cast<TreeItem*>(parent.internalPointer()) : m_root;
            TreeItem* childItem = parentItem->children.at(row);
            return createIndex(row, column, childItem);
        }

        int rowCount(const QModelIndex& parent) const override
        {
            if (parent.column() > 0)
                return 0;

            TreeItem* parentItem = parent.isValid() ? static_cast<TreeItem*>(parent.internalPointer())
                : m_root;
            return parentItem->children.count();
        }

        int columnCount(const QModelIndex&) const override { return 2; }

        QModelIndex parent(const QModelIndex& childIndex) const override
        {
            if (!childIndex.isValid())
                return QModelIndex();

            TreeItem* parentItem =
                static_cast<TreeItem*>(childIndex.internalPointer())->parent;
            return parentItem == m_root ? QModelIndex()
                : createIndex(parentItem->rowInParent(), 0, parentItem);
        }

        QVariant data(const QModelIndex& index, const int role) const override
        {
            if (!index.isValid() || role != Qt::DisplayRole)
                return QVariant();

            TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
            return item->data.at(index.column());
        }

        bool canFetchMore(const QModelIndex& parent) const override
        {
            if (!parent.isValid()) {
                return false;
            } else {
                TreeItem* item = static_cast<TreeItem*>(parent.internalPointer());
                return item->children.size() < maxChildren;
            }
        }

        void fetchMore(const QModelIndex& parent) override
        {
            if (!parent.isValid())
                return;

            fetchMoreCount++;
            TreeItem* parentItem = static_cast<TreeItem*>(parent.internalPointer());
            int childCount = parentItem->children.size();

            beginInsertRows(parent, childCount, childCount);

            QVariant childData1("Col 1");
            QVariant childData2("Col 2");
            QVector<QVariant> childData;
            childData.append(childData1);
            childData.append(childData2);
            TreeItem* newChild = new TreeItem(childData, parentItem);
            parentItem->children.append(newChild);

            endInsertRows();
        }

        int fetchMoreCount = 0;
    private:
        struct TreeItem
        {
            TreeItem(const QVector<QVariant>& values, TreeItem* parent)
                : data(values), parent(parent)
            {
            }
            ~TreeItem() { qDeleteAll(children); }
            int rowInParent() const
            {
                if (parent)
                    return parent->children.indexOf(const_cast<TreeItem*>(this));
                return 0;
            }
            QVector<QVariant> data;
            QVector<TreeItem*> children;
            TreeItem* parent = nullptr;
        };
        TreeItem* m_root;
    };

    QTreeView tv;
    TreeModel model;
    tv.setModel(&model);

    const int itemHeight = tv.sizeHintForRow(0);
    tv.resize(250, itemHeight * 10);
    tv.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tv));

    tv.expand(model.index(0, 0));
    const int viewportHeight = tv.viewport()->height();
    const int itemCount = viewportHeight / itemHeight;
    const int minFetchCount = itemCount - 1;
    const int maxFetchCount = itemCount + 1;

    const bool expectedItemNumberFetched = model.fetchMoreCount >= minFetchCount
                                         && model.fetchMoreCount <= maxFetchCount;
    if (!expectedItemNumberFetched)
        qDebug() << model.fetchMoreCount << minFetchCount << maxFetchCount;
    QVERIFY(expectedItemNumberFetched);
}


QTEST_MAIN(tst_QTreeView)
#include "tst_qtreeview.moc"
