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


#include <QListWidget>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QStyleFactory>
#include <QTest>
#include <QTimer>
#include <QtMath>

#include <QtTest/private/qtesthelpers_p.h>
#include <QtWidgets/private/qlistview_p.h>

using namespace QTestPrivate;

#if defined(Q_OS_WIN)
#  include <windows.h>
#  include <QDialog>
#  include <QGuiApplication>
#  include <QVBoxLayout>
#include <qpa/qplatformnativeinterface.h>
#endif // Q_OS_WIN

#if defined(Q_OS_WIN)
static inline HWND getHWNDForWidget(const QWidget *widget)
{
    QWindow *window = widget->windowHandle();
    return static_cast<HWND> (QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", window));
}
#endif // Q_OS_WIN

Q_DECLARE_METATYPE(QAbstractItemView::ScrollMode)
Q_DECLARE_METATYPE(QMargins)
Q_DECLARE_METATYPE(QSize)
using IntList = QVector<int>;

static QStringList generateList(const QString &prefix, int size)
{
    QStringList result;
    result.reserve(size);
    for (int i = 0; i < size; ++i)
        result.append(prefix + QString::number(i));
    return result;
}

class PublicListView : public QListView
{
public:
    using QListView::QListView;
    using QListView::contentsSize;
    using QListView::moveCursor;
    using QListView::selectedIndexes;
    using QListView::setPositionForIndex;
    using QListView::setSelection;
    using QListView::setViewportMargins;
    using QListView::startDrag;
    using QListView::viewOptions;
    QRegion getVisualRegionForSelection() const
    {
        return QListView::visualRegionForSelection(selectionModel()->selection());
    }

    friend class tst_QListView;
};

class tst_QListView : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void getSetCheck();
    void noDelegate();
    void noModel();
    void emptyModel();
    void removeRows();
    void cursorMove();
    void hideRows();
    void moveCursor();
    void moveCursor2();
    void moveCursor3();
    void indexAt();
    void clicked();
    void singleSelectionRemoveRow();
    void singleSelectionRemoveColumn();
    void modelColumn();
    void hideFirstRow();
    void batchedMode();
    void setCurrentIndex();
    void selection_data();
    void selection();
    void scrollTo();
    void scrollBarRanges();
    void scrollBarAsNeeded_data();
    void scrollBarAsNeeded();
    void moveItems();
    void wordWrap();
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    void setCurrentIndexAfterAppendRowCrash();
#endif
    void emptyItemSize();
    void task203585_selectAll();
    void task228566_infiniteRelayout();
    void task248430_crashWith0SizedItem();
    void task250446_scrollChanged();
    void task196118_visualRegionForSelection();
    void task254449_draggingItemToNegativeCoordinates();
    void keyboardSearch();
    void shiftSelectionWithNonUniformItemSizes();
    void shiftSelectionWithItemAlignment();
    void clickOnViewportClearsSelection();
    void task262152_setModelColumnNavigate();
    void taskQTBUG_2233_scrollHiddenItems_data();
    void taskQTBUG_2233_scrollHiddenItems();
    void taskQTBUG_633_changeModelData();
    void taskQTBUG_435_deselectOnViewportClick();
    void taskQTBUG_2678_spacingAndWrappedText();
    void taskQTBUG_5877_skippingItemInPageDownUp();
    void taskQTBUG_9455_wrongScrollbarRanges();
    void styleOptionViewItem();
    void taskQTBUG_12308_artihmeticException();
    void taskQTBUG_12308_wrongFlowLayout();
    void taskQTBUG_21115_scrollToAndHiddenItems_data();
    void taskQTBUG_21115_scrollToAndHiddenItems();
    void draggablePaintPairs_data();
    void draggablePaintPairs();
    void taskQTBUG_21804_hiddenItemsAndScrollingWithKeys_data();
    void taskQTBUG_21804_hiddenItemsAndScrollingWithKeys();
    void spacing_data();
    void spacing();
    void testScrollToWithHidden();
    void testViewOptions();
    void taskQTBUG_39902_mutualScrollBars_data();
    void taskQTBUG_39902_mutualScrollBars();
    void horizontalScrollingByVerticalWheelEvents();
    void taskQTBUG_7232_AllowUserToControlSingleStep();
    void taskQTBUG_51086_skippingIndexesInSelectedIndexes();
    void taskQTBUG_47694_indexOutOfBoundBatchLayout();
    void itemAlignment();
    void internalDragDropMove_data();
    void internalDragDropMove();
};

// Testing get/set functions
void tst_QListView::getSetCheck()
{
    QListView obj1;
    // Movement QListView::movement()
    // void QListView::setMovement(Movement)
    obj1.setMovement(QListView::Static);
    QCOMPARE(QListView::Static, obj1.movement());
    obj1.setMovement(QListView::Free);
    QCOMPARE(QListView::Free, obj1.movement());
    obj1.setMovement(QListView::Snap);
    QCOMPARE(QListView::Snap, obj1.movement());

    // Flow QListView::flow()
    // void QListView::setFlow(Flow)
    obj1.setFlow(QListView::LeftToRight);
    QCOMPARE(QListView::LeftToRight, obj1.flow());
    obj1.setFlow(QListView::TopToBottom);
    QCOMPARE(QListView::TopToBottom, obj1.flow());

    // ResizeMode QListView::resizeMode()
    // void QListView::setResizeMode(ResizeMode)
    obj1.setResizeMode(QListView::Fixed);
    QCOMPARE(QListView::Fixed, obj1.resizeMode());
    obj1.setResizeMode(QListView::Adjust);
    QCOMPARE(QListView::Adjust, obj1.resizeMode());

    // LayoutMode QListView::layoutMode()
    // void QListView::setLayoutMode(LayoutMode)
    obj1.setLayoutMode(QListView::SinglePass);
    QCOMPARE(QListView::SinglePass, obj1.layoutMode());
    obj1.setLayoutMode(QListView::Batched);
    QCOMPARE(QListView::Batched, obj1.layoutMode());

    // int QListView::spacing()
    // void QListView::setSpacing(int)
    obj1.setSpacing(0);
    QCOMPARE(0, obj1.spacing());
    obj1.setSpacing(std::numeric_limits<int>::min());
    QCOMPARE(std::numeric_limits<int>::min(), obj1.spacing());
    obj1.setSpacing(std::numeric_limits<int>::max());
    QCOMPARE(std::numeric_limits<int>::max(), obj1.spacing());

    // ViewMode QListView::viewMode()
    // void QListView::setViewMode(ViewMode)
    obj1.setViewMode(QListView::ListMode);
    QCOMPARE(QListView::ListMode, obj1.viewMode());
    obj1.setViewMode(QListView::IconMode);
    QCOMPARE(QListView::IconMode, obj1.viewMode());

    // int QListView::modelColumn()
    // void QListView::setModelColumn(int)
    obj1.setModelColumn(0);
    QCOMPARE(0, obj1.modelColumn());
    obj1.setModelColumn(std::numeric_limits<int>::min());
    QCOMPARE(0, obj1.modelColumn()); // Less than 0 => 0
    obj1.setModelColumn(std::numeric_limits<int>::max());
    QCOMPARE(0, obj1.modelColumn()); // No model => 0

    // bool QListView::uniformItemSizes()
    // void QListView::setUniformItemSizes(bool)
    obj1.setUniformItemSizes(false);
    QCOMPARE(false, obj1.uniformItemSizes());
    obj1.setUniformItemSizes(true);
    QCOMPARE(true, obj1.uniformItemSizes());

    // make sure setViewMode() doesn't reset resizeMode
    obj1.clearPropertyFlags();
    obj1.setResizeMode(QListView::Adjust);
    obj1.setViewMode(QListView::IconMode);
    QCOMPARE(obj1.resizeMode(), QListView::Adjust);

    obj1.setWordWrap(false);
    QCOMPARE(false, obj1.wordWrap());
    obj1.setWordWrap(true);
    QCOMPARE(true, obj1. wordWrap());
}

class QtTestModel: public QAbstractListModel
{
    Q_OBJECT
public:
    QtTestModel(int rc, int cc, QObject *parent = nullptr)
      : QAbstractListModel(parent), rCount(rc), cCount(cc) {}
    int rowCount(const QModelIndex &) const override { return rCount; }
    int columnCount(const QModelIndex &) const override { return cCount; }

    QVariant data(const QModelIndex &idx, int role) const override
    {
        if (!m_icon.isNull() && role == Qt::DecorationRole)
            return m_icon;
        if (role != Qt::DisplayRole)
            return QVariant();

        if (idx.row() < 0 || idx.column() < 0 || idx.column() >= cCount
            || idx.row() >= rCount) {
            wrongIndex = true;
            qWarning("got invalid modelIndex %d/%d", idx.row(), idx.column());
        }
        return QString::number(idx.row()) + QLatin1Char('/') + QString::number(idx.column());
    }

    void removeLastRow()
    {
        beginRemoveRows(QModelIndex(), rCount - 2, rCount - 1);
        --rCount;
        endRemoveRows();
    }

    void removeAllRows()
    {
        beginRemoveRows(QModelIndex(), 0, rCount - 1);
        rCount = 0;
        endRemoveRows();
    }

    void setDataIcon(const QIcon &icon)
    {
        m_icon = icon;
    }

    QIcon m_icon;
    int rCount, cCount;
    mutable bool wrongIndex = false;
};

class ScrollPerItemListView : public QListView
{
    Q_OBJECT
public:
    explicit ScrollPerItemListView(QWidget *parent = nullptr)
        : QListView(parent)
    {
        // Force per item scroll mode since it comes from the style by default
        setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
        setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);
    }
};

void tst_QListView::cleanup()
{
    QTRY_VERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QListView::noDelegate()
{
    QtTestModel model(10, 10);
    QListView view;
    view.setModel(&model);
    view.setItemDelegate(nullptr);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

void tst_QListView::noModel()
{
    QListView view;
    view.show();
    view.setRowHidden(0, true);
    // no model -> not able to hide a row
    QVERIFY(!view.isRowHidden(0));
}

void tst_QListView::emptyModel()
{
    QtTestModel model(0, 0);
    QListView view;
    view.setModel(&model);
    view.show();
    QVERIFY(!model.wrongIndex);
}

void tst_QListView::removeRows()
{
    QtTestModel model(10, 10);
    QListView view;
    view.setModel(&model);
    view.show();

    model.removeLastRow();
    QVERIFY(!model.wrongIndex);

    model.removeAllRows();
    QVERIFY(!model.wrongIndex);
}

void tst_QListView::cursorMove()
{
    int rows = 6*6;
    int columns = 6;

    QStandardItemModel model(rows, columns);
    QWidget topLevel;
    QListView view(&topLevel);
    view.setModel(&model);

    for (int j = 0; j < columns; ++j) {
        const QString postfix = QLatin1Char(',') + QString::number(j) + QLatin1Char(']');
        view.setModelColumn(j);
        for (int i = 0; i < rows; ++i) {
            QModelIndex index = model.index(i, j);
            model.setData(index, QLatin1Char('[') + QString::number(i) + postfix);
            view.setCurrentIndex(index);
            QTRY_COMPARE(view.currentIndex(), index);
        }
    }

    QSize cellsize(60, 25);
    int gap = 1; // compensate for the scrollbars
    int displayColumns = 6;

    view.resize((displayColumns + gap) * cellsize.width(),
                 int((ceil(double(rows) / displayColumns) + gap) * cellsize.height()));
    view.setResizeMode(QListView::Adjust);
    view.setGridSize(cellsize);
    view.setViewMode(QListView::IconMode);
    view.doItemsLayout();
    topLevel.show();

    static const Qt::Key keymoves[] {
        Qt::Key_Up, Qt::Key_Up, Qt::Key_Right, Qt::Key_Right, Qt::Key_Up,
        Qt::Key_Left, Qt::Key_Left, Qt::Key_Up, Qt::Key_Down, Qt::Key_Up,
        Qt::Key_Up, Qt::Key_Up, Qt::Key_Up, Qt::Key_Up, Qt::Key_Up,
        Qt::Key_Left, Qt::Key_Left, Qt::Key_Up, Qt::Key_Down
    };

    int lastRow = rows / displayColumns - 1;
    int lastColumn = displayColumns - 1;

    int displayRow    = lastRow;
    int displayColumn = lastColumn - (rows % displayColumns);

    QCoreApplication::processEvents();
    for (Qt::Key key : keymoves) {
        QTest::keyClick(&view, key);
        switch (key) {
        case Qt::Key_Up:
            displayRow = qMax(0, displayRow - 1);
            break;
        case Qt::Key_Down:
            displayRow = qMin(rows / displayColumns - 1, displayRow + 1);
            break;
        case Qt::Key_Left:
            if (displayColumn > 0) {
                displayColumn--;
            } else {
                if (displayRow > 0) {
                    displayRow--;
                    displayColumn = lastColumn;
                }
            }
            break;
        case Qt::Key_Right:
            if (displayColumn < lastColumn) {
                displayColumn++;
            } else {
                if (displayRow < lastRow) {
                    displayRow++;
                    displayColumn = 0;
                }
            }
            break;
        default:
            QVERIFY(false);
        }

        QCoreApplication::processEvents();

        int row = displayRow * displayColumns + displayColumn;
        int column = columns - 1;
        QModelIndex index = model.index(row, column);
        QCOMPARE(view.currentIndex().row(), row);
        QCOMPARE(view.currentIndex().column(), column);
        QCOMPARE(view.currentIndex(), index);
    }
}

void tst_QListView::hideRows()
{
    QtTestModel model(10, 10);
    QListView view;
    view.setModel(&model);
    view.show();

    // hide then show
    QVERIFY(!view.isRowHidden(2));
    view.setRowHidden(2, true);
    QVERIFY(view.isRowHidden(2));
    view.setRowHidden(2, false);
    QVERIFY(!view.isRowHidden(2));

    // re show same row
    QVERIFY(!view.isRowHidden(2));
    view.setRowHidden(2, false);
    QVERIFY(!view.isRowHidden(2));

    // double hidding
    QVERIFY(!view.isRowHidden(2));
    view.setRowHidden(2, true);
    QVERIFY(view.isRowHidden(2));
    view.setRowHidden(2, true);
    QVERIFY(view.isRowHidden(2));
    view.setRowHidden(2, false);
    QVERIFY(!view.isRowHidden(2));

    // show in per-item mode, then hide the first row
    view.setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    QVERIFY(!view.isRowHidden(0));
    view.setRowHidden(0, true);
    QVERIFY(view.isRowHidden(0));
    view.setRowHidden(0, false);
    QVERIFY(!view.isRowHidden(0));

    QStandardItemModel sim;
    QStandardItem *root = new QStandardItem("Root row");
    for (int i = 0;i < 5; i++)
        root->appendRow(new QStandardItem(QLatin1String("Row ") + QString::number(i)));
    sim.appendRow(root);
    view.setModel(&sim);
    view.setRootIndex(root->index());
    QVERIFY(!view.isRowHidden(0));
    view.setRowHidden(0, true);
    QVERIFY(view.isRowHidden(0));
    view.setRowHidden(0, false);
    QVERIFY(!view.isRowHidden(0));
}


void tst_QListView::moveCursor()
{
    QtTestModel model(10, 10);
    QListView view;
    view.setModel(&model);

    QTest::keyClick(&view, Qt::Key_Down);

    view.setModel(nullptr);
    view.setModel(&model);
    view.setRowHidden(0, true);

    QTest::keyClick(&view, Qt::Key_Down);
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(1, 0));
}

void tst_QListView::moveCursor2()
{
    QtTestModel model(100, 1);
    QPixmap pm(32, 32);
    pm.fill(Qt::green);
    model.setDataIcon(QIcon(pm));

    PublicListView vu;
    vu.setModel(&model);
    vu.setIconSize(QSize(36,48));
    vu.setGridSize(QSize(34,56));
    //Standard framesize is 1. If Framesize > 2 increase size
    int frameSize = qApp->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    vu.resize(300 + frameSize * 2, 300);
    vu.setFlow(QListView::LeftToRight);
    vu.setMovement(QListView::Static);
    vu.setWrapping(true);
    vu.setViewMode(QListView::IconMode);
    vu.setLayoutMode(QListView::Batched);
    vu.show();
    vu.selectionModel()->setCurrentIndex(model.index(0,0), QItemSelectionModel::SelectCurrent);
    QCoreApplication::processEvents();

    QModelIndex idx = vu.moveCursor(PublicListView::MoveHome, Qt::NoModifier);
    QCOMPARE(idx, model.index(0, 0));
    idx = vu.moveCursor(PublicListView::MoveDown, Qt::NoModifier);
    QCOMPARE(idx, model.index(8, 0));
}

void tst_QListView::moveCursor3()
{
    //this tests is for task 159792
    //it tests that navigation works even with non uniform item sizes
    QListView view;
    QStandardItemModel model(0, 1);
    QStandardItem *i1 = new QStandardItem("First item, long name");
    QStandardItem *i2 = new QStandardItem("2nd item");
    QStandardItem *i3 = new QStandardItem("Third item, long name");
    i1->setSizeHint(QSize(200, 32));
    model.appendRow(i1);
    model.appendRow(i2);
    model.appendRow(i3);
    view.setModel(&model);

    view.setCurrentIndex(model.index(0, 0));

    QCOMPARE(view.selectionModel()->currentIndex(), model.index(0, 0));
    QTest::keyClick(&view, Qt::Key_Down);
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(1, 0));
    QTest::keyClick(&view, Qt::Key_Down);
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(2, 0));
    QTest::keyClick(&view, Qt::Key_Up);
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(1, 0));
    QTest::keyClick(&view, Qt::Key_Up);
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(0, 0));
}


class QListViewShowEventListener : public QListView
{
    Q_OBJECT
public:
    using QListView::QListView;
    void showEvent(QShowEvent *e) override
    {
        QListView::showEvent(e);
        int columnwidth = sizeHintForColumn(0);
        QSize sz = sizeHintForIndex(model()->index(0, 0));

        // This should retrieve a model index in the 2nd section
        m_index = indexAt(QPoint(columnwidth +2, sz.height() / 2));
        m_shown = true;
    }

    QModelIndex m_index;
    bool m_shown = false;

};

void tst_QListView::indexAt()
{
    QtTestModel model(2, 1);
    QListView view;
    view.setModel(&model);
    view.setViewMode(QListView::ListMode);
    view.setFlow(QListView::TopToBottom);

    QSize sz = view.sizeHintForIndex(model.index(0, 0));
    QModelIndex index;
    index = view.indexAt(QPoint(20, 0));
    QVERIFY(index.isValid());
    QCOMPARE(index.row(), 0);

    index = view.indexAt(QPoint(20, sz.height()));
    QVERIFY(index.isValid());
    QCOMPARE(index.row(), 1);

    index = view.indexAt(QPoint(20, 2 * sz.height()));
    QVERIFY(!index.isValid());

    // Check when peeking out of the viewport bounds
    index = view.indexAt(QPoint(view.viewport()->rect().width(), 0));
    QVERIFY(!index.isValid());
    index = view.indexAt(QPoint(-1, 0));
    QVERIFY(!index.isValid());
    index = view.indexAt(QPoint(20, view.viewport()->rect().height()));
    QVERIFY(!index.isValid());
    index = view.indexAt(QPoint(20, -1));
    QVERIFY(!index.isValid());

    model.rCount = 30;
    QListViewShowEventListener view2;
    // Set the height to a small enough value so that it wraps to a new section.
    view2.resize(300, 100);
    view2.setModel(&model);
    view2.setFlow(QListView::TopToBottom);
    view2.setViewMode(QListView::ListMode);
    view2.setWrapping(true);
    // We really want to make sure it is shown, because the layout won't be known until it is shown
    view2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view2));
    QTRY_VERIFY(view2.m_shown);

    QVERIFY(view2.m_index.isValid());
    QVERIFY(view2.m_index.row() != 0);
}

void tst_QListView::clicked()
{
    QtTestModel model(10, 2);
    QListView view;
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex firstIndex = model.index(0, 0, QModelIndex());
    QVERIFY(firstIndex.isValid());
    int itemHeight = view.visualRect(firstIndex).height();
    view.resize(200, itemHeight * (model.rCount + 1));

    for (int i = 0; i < model.rCount; ++i) {
        QPoint p(5, 1 + itemHeight * i);
        QModelIndex index = view.indexAt(p);
        if (!index.isValid())
            continue;
        QSignalSpy spy(&view, &QListView::clicked);
        QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
        QCOMPARE(spy.count(), 1);
    }
}

void tst_QListView::singleSelectionRemoveRow()
{
    QStringListModel model({"item1", "item2", "item3", "item4"});
    QListView view;
    view.setModel(&model);
    view.show();

    QModelIndex index;
    view.setCurrentIndex(model.index(1));
    index = view.currentIndex();
    QCOMPARE(view.model()->data(index).toString(), QString("item2"));

    model.removeRow(1);
    index = view.currentIndex();
    QCOMPARE(view.model()->data(index).toString(), QString("item3"));

    model.removeRow(0);
    index = view.currentIndex();
    QCOMPARE(view.model()->data(index).toString(), QString("item3"));
}

void tst_QListView::singleSelectionRemoveColumn()
{
    int numCols = 3;
    int numRows = 3;
    QStandardItemModel model(numCols, numRows);
    for (int r = 0; r < numRows; ++r) {
        const QString prefix = QString::number(r) + QLatin1Char(',');
        for (int c = 0; c < numCols; ++c)
            model.setData(model.index(r, c), prefix + QString::number(c));
    }

    QListView view;
    view.setModel(&model);
    view.show();

    QModelIndex index;
    view.setCurrentIndex(model.index(1, 1));
    index = view.currentIndex();
    QCOMPARE(view.model()->data(index).toString(), QString("1,1"));

    model.removeColumn(1);
    index = view.currentIndex();
    QCOMPARE(view.model()->data(index).toString(), QString("1,0"));

    model.removeColumn(0);
    index = view.currentIndex();
    QCOMPARE(view.model()->data(index).toString(), QString("1,2"));
}

void tst_QListView::modelColumn()
{
    int numCols = 3;
    int numRows = 3;
    QStandardItemModel model(numCols, numRows);
    for (int r = 0; r < numRows; ++r) {
        const QString prefix = QString::number(r) + QLatin1Char(',');
        for (int c = 0; c < numCols; ++c)
            model.setData(model.index(r, c), prefix + QString::number(c));
    }

    QListView view;
    view.setModel(&model);


    //
    // Set and get with a valid model
    //

    // Default is column 0
    QCOMPARE(view.modelColumn(), 0);

    view.setModelColumn(0);
    QCOMPARE(view.modelColumn(), 0);
    view.setModelColumn(1);
    QCOMPARE(view.modelColumn(), 1);
    view.setModelColumn(2);
    QCOMPARE(view.modelColumn(), 2);

    // Out of bound cases should not modify the modelColumn
    view.setModelColumn(-1);
    QCOMPARE(view.modelColumn(), 2);
    view.setModelColumn(std::numeric_limits<int>::max());
    QCOMPARE(view.modelColumn(), 2);


    // See if it displays the right column using indexAt()...
    view.resize(400,400);
    view.show();

    for (int c = 0; c < 3; ++c) {
        view.setModelColumn(c);
        int startrow = 0;
        for (int y = 0; y < view.height(); ++y) {
            QModelIndex idx = view.indexAt( QPoint(1, y) );
            if (idx.row() == startrow + 1) ++startrow;
            else if (idx.row() == -1) break;
            QCOMPARE(idx.row(), startrow);
            QCOMPARE(idx.column(), c);
        }
        QCOMPARE(startrow, 2);
    }
}

void tst_QListView::hideFirstRow()
{
    QStringListModel model(generateList(QLatin1String("item"), 100));

    QListView view;
    view.setModel(&model);
    view.setUniformItemSizes(true);
    view.setRowHidden(0, true);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

static int modelIndexCount(const QAbstractItemView *view)
{
    QBitArray ba;
    for (int y = 0, height = view->height(); y < height; ++y) {
        const QModelIndex idx = view->indexAt( QPoint(1, y) );
        if (!idx.isValid())
            break;
        if (idx.row() >= ba.size())
            ba.resize(idx.row() + 1);
        ba.setBit(idx.row(), true);
    }
    return ba.size();
}

void tst_QListView::batchedMode()
{
    const int rowCount = 3;

    QStringListModel model(generateList(QLatin1String("item "), rowCount));

    QListView view;
    view.setWindowTitle(QTest::currentTestFunction());
    view.setModel(&model);
    view.setUniformItemSizes(true);
    view.setViewMode(QListView::ListMode);
    view.setLayoutMode(QListView::Batched);
    view.setBatchSize(2);
    view.resize(200, 400);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QTRY_COMPARE(modelIndexCount(&view), rowCount);

    // Test the dynamic listview too.
    view.setViewMode(QListView::IconMode);
    view.setLayoutMode(QListView::Batched);
    view.setFlow(QListView::TopToBottom);
    view.setBatchSize(2);

    QTRY_COMPARE(modelIndexCount(&view), rowCount);
}

void tst_QListView::setCurrentIndex()
{
    QStringListModel model(generateList(QLatin1String("item "), 20));

    ScrollPerItemListView view;
    view.setModel(&model);
    view.resize(220,182);
    view.show();

    for (int pass = 0; pass < 2; ++pass) {
        view.setFlow(pass == 0 ? QListView::TopToBottom : QListView::LeftToRight);
        QScrollBar *sb = pass == 0 ? view.verticalScrollBar() : view.horizontalScrollBar();
        for (const QSize &gridSize : {QSize(), QSize(200, 38)}) {
            if (pass == 1 && !gridSize.isValid()) // the width of an item varies, so it might jump two times
                continue;
            view.setGridSize(gridSize);

            QCoreApplication::processEvents();
            int offset = sb->value();

            // first "scroll" down, verify that we scroll one step at a time
            int i = 0;
            for (i = 0; i < 20; ++i) {
                QModelIndex idx = model.index(i,0);
                view.setCurrentIndex(idx);
                if (offset != sb->value()) {
                    // If it has scrolled, it should have scrolled only by one.
                    QCOMPARE(sb->value(), offset + 1);
                    ++offset;
                }
            }

            --i;    // item 20 does not exist
            // and then "scroll" up, verify that we scroll one step at a time
            for (; i >= 0; --i) {
                QModelIndex idx = model.index(i,0);
                view.setCurrentIndex(idx);
                if (offset != sb->value()) {
                    // If it has scrolled, it should have scrolled only by one.
                    QCOMPARE(sb->value(), offset - 1);
                    --offset;
                }
            }
        }
    }
    while (model.rowCount()) {
        view.setCurrentIndex(model.index(model.rowCount() - 1, 0));
        model.removeRow(model.rowCount() - 1);
    }
}

class TestDelegate : public QStyledItemDelegate
{
public:
    explicit TestDelegate(QObject *parent, const QSize &sizeHint = QSize(50, 50))
        : QStyledItemDelegate(parent), m_sizeHint(sizeHint) {}
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override { return m_sizeHint; }

    const QSize m_sizeHint;
};

void tst_QListView::selection_data()
{
    QTest::addColumn<int>("itemCount");
    QTest::addColumn<QListView::ViewMode>("viewMode");
    QTest::addColumn<QListView::Flow>("flow");
    QTest::addColumn<bool>("wrapping");
    QTest::addColumn<int>("spacing");
    QTest::addColumn<QSize>("gridSize");
    QTest::addColumn<IntList>("hiddenRows");
    QTest::addColumn<QRect>("selectionRect");
    QTest::addColumn<IntList>("expectedItems");

    QTest::newRow("select all")
        << 4                                    // itemCount
        << QListView::ListMode
        << QListView::TopToBottom
        << false                                // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(0, 0, 10, 200)                 // selection rectangle
        << (IntList() << 0 << 1 << 2 << 3);     // expected items

    QTest::newRow("select below, (on viewport)")
        << 4                                    // itemCount
        << QListView::ListMode
        << QListView::TopToBottom
        << false                                // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(10, 250, 1, 1)                 // selection rectangle
        << IntList();                           // expected items

    QTest::newRow("select below 2, (on viewport)")
        << 4                                    // itemCount
        << QListView::ListMode
        << QListView::TopToBottom
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(10, 250, 1, 1)                 // selection rectangle
        << IntList();                           // expected items

    QTest::newRow("select to the right, (on viewport)")
        << 40                                   // itemCount
        << QListView::ListMode
        << QListView::TopToBottom
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(300, 10, 1, 1)                 // selection rectangle
        << IntList();                           // expected items

    QTest::newRow("select to the right 2, (on viewport)")
        << 40                                   // itemCount
        << QListView::ListMode
        << QListView::TopToBottom
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(300, 0, 1, 300)                // selection rectangle
        << IntList();                           // expected items

    QTest::newRow("select inside contents, (on viewport)")
        << 35                                   // itemCount
        << QListView::ListMode
        << QListView::TopToBottom
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(175, 275, 1, 1)                // selection rectangle
        << IntList();                           // expected items

    QTest::newRow("select a tall rect in LeftToRight flow, wrap items")
        << 70                                   // itemCount
        << QListView::ListMode
        << QListView::LeftToRight
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(90, 90, 1, 100)                // selection rectangle
        << (IntList()                           // expected items
                      << 11 << 12 << 13 << 14 << 15 << 16 << 17 << 18 << 19
                << 20 << 21 << 22 << 23 << 24 << 25 << 26 << 27 << 28 << 29
                << 30 << 31);

    QTest::newRow("select a wide rect in LeftToRight, wrap items")
        << 70                                   // itemCount
        << QListView::ListMode
        << QListView::LeftToRight
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(90, 90, 200, 1)                // selection rectangle
        << (IntList()                           // expected items
                      << 11 << 12 << 13 << 14 << 15);

    QTest::newRow("select a wide negative rect in LeftToRight flow, wrap items")
        << 70                                   // itemCount
        << QListView::ListMode
        << QListView::LeftToRight
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(290, 90, -200, 1)              // selection rectangle
        << (IntList()                           // expected items
                      << 11 << 12 << 13 << 14 << 15);

    QTest::newRow("select a tall rect in TopToBottom flow, wrap items")
        << 70                                   // itemCount
        << QListView::ListMode
        << QListView::TopToBottom
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(90, 90, 1, 100)                // selection rectangle
        << (IntList()                           // expected items
                      << 11
                      << 12
                      << 13);

    QTest::newRow("select a tall negative rect in TopToBottom flow, wrap items")
        << 70                                   // itemCount
        << QListView::ListMode
        << QListView::TopToBottom
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(90, 190, 1, -100)              // selection rectangle
        << (IntList()                           // expected items
                      << 11
                      << 12
                      << 13);

    QTest::newRow("select a wide rect in TopToBottom, wrap items")
        << 70                                   // itemCount
        << QListView::ListMode
        << QListView::TopToBottom
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(90, 90, 100, 1)                // selection rectangle
        << (IntList()                           // expected items
                            << 20 << 30
                      << 11 << 21 << 31
                      << 12 << 22
                      << 13 << 23
                      << 14 << 24
                      << 15 << 25
                      << 16 << 26
                      << 17 << 27
                      << 18 << 28
                      << 19 << 29);
}

void tst_QListView::selection()
{
    QFETCH(int, itemCount);
    QFETCH(QListView::ViewMode, viewMode);
    QFETCH(QListView::Flow, flow);
    QFETCH(bool, wrapping);
    QFETCH(int, spacing);
    QFETCH(QSize, gridSize);
    QFETCH(const IntList, hiddenRows);
    QFETCH(QRect, selectionRect);
    QFETCH(const IntList, expectedItems);

    QWidget topLevel;
    PublicListView v(&topLevel);
    QtTestModel model(itemCount, 1);

    // avoid scrollbar size mismatches among different styles
    v.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    v.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    v.setItemDelegate(new TestDelegate(&v));
    v.setModel(&model);
    v.setViewMode(viewMode);
    v.setFlow(flow);
    v.setWrapping(wrapping);
    v.setResizeMode(QListView::Adjust);
    v.setSpacing(spacing);
    if (gridSize.isValid())
        v.setGridSize(gridSize);
    for (int row : hiddenRows)
        v.setRowHidden(row, true);

    v.resize(525, 525);

    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    v.setSelection(selectionRect, QItemSelectionModel::ClearAndSelect);

    const QModelIndexList selected = v.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), expectedItems.count());
    for (const auto &idx : selected)
        QVERIFY(expectedItems.contains(idx.row()));
}

void tst_QListView::scrollTo()
{
    QWidget topLevel;
    setFrameless(&topLevel);
    ScrollPerItemListView lv(&topLevel);
    QStringListModel model(&lv);
    QStringList list;
    list << "Short item 1";
    list << "Short item 2";
    list << "Short item 3";
    list << "Short item 4";
    list << "Short item 5";
    list << "Short item 6";
    list << "Begin This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\n"
            "This is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item\nThis is a very long item End\n";
    list << "Short item";
    list << "Short item";
    list << "Short item";
    list << "Short item";
    list << "Short item";
    list << "Short item";
    list << "Short item";
    list << "Short item";
    model.setStringList(list);
    lv.setModel(&model);
    lv.setFixedSize(110, 200);

    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    //by default, the list view scrolls per item and has no wrapping
    QModelIndex index = model.index(6, 0);

    //we save the size of the item for later comparisons
    const QSize itemsize = lv.visualRect(index).size();
    QVERIFY(itemsize.height() > lv.height());
    QVERIFY(itemsize.width() > lv.width());

    //we click the item
    QPoint p = lv.visualRect(index).center();
    QTest::mouseClick(lv.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    //let's wait because the scrolling is delayed
    QTRY_COMPARE(lv.visualRect(index).y(), 0);

    //we scroll down. As the item is to tall for the view, it will disappear
    QTest::keyClick(lv.viewport(), Qt::Key_Down, Qt::NoModifier);
    QTRY_COMPARE(lv.visualRect(index).y(), -itemsize.height());

    QTest::keyClick(lv.viewport(), Qt::Key_Up, Qt::NoModifier);
    QTRY_COMPARE(lv.visualRect(index).y(), 0);

    //Let's enable wrapping

    lv.setWrapping(true);
    lv.horizontalScrollBar()->setValue(0); //let's scroll to the beginning

    //we click the item
    p = lv.visualRect(index).center();
    QTest::mouseClick(lv.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QTRY_COMPARE(lv.visualRect(index).x(), 0);

    //we scroll right. As the item is too wide for the view, it will disappear
    QTest::keyClick(lv.viewport(), Qt::Key_Right, Qt::NoModifier);
    QTRY_COMPARE(lv.visualRect(index).x(), -itemsize.width());

    QTest::keyClick(lv.viewport(), Qt::Key_Left, Qt::NoModifier);
    QTRY_COMPARE(lv.visualRect(index).x(), 0);

    lv.setWrapping(false);
    QCoreApplication::processEvents(); //let the layout happen

    //Let's try with scrolling per pixel
    lv.setHorizontalScrollMode(QListView::ScrollPerPixel);
    lv.verticalScrollBar()->setValue(0); //scrolls back to the first item

    //we click the item
    p = lv.visualRect(index).center();
    QTest::mouseClick(lv.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    //let's wait because the scrolling is delayed
    QTest::qWait(QApplication::doubleClickInterval() + 150);
    QTRY_COMPARE(lv.visualRect(index).y(), 0);

    //we scroll down. As the item is too tall for the view, it will partially disappear
    QTest::keyClick(lv.viewport(), Qt::Key_Down, Qt::NoModifier);
    QVERIFY(lv.visualRect(index).y() < 0);

    QTest::keyClick(lv.viewport(), Qt::Key_Up, Qt::NoModifier);
    QCOMPARE(lv.visualRect(index).y(), 0);
}


void tst_QListView::scrollBarRanges()
{
    const int rowCount = 10;
    const int rowHeight = 20;

    QWidget topLevel;
    ScrollPerItemListView lv(&topLevel);
    QStringListModel model(&lv);
    model.setStringList(generateList(QLatin1String("Item "), rowCount));
    lv.setModel(&model);
    lv.resize(250, 130);

    lv.setItemDelegate(new TestDelegate(&lv, QSize(100, rowHeight)));
    topLevel.show();

    for (int h = 30; h <= 210; ++h) {
        lv.resize(250, h);
        int visibleRowCount = lv.viewport()->size().height() / rowHeight;
        int invisibleRowCount = rowCount - visibleRowCount;
        QTRY_COMPARE(lv.verticalScrollBar()->maximum(), invisibleRowCount);
    }
}

void tst_QListView::scrollBarAsNeeded_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("itemCount");
    QTest::addColumn<QAbstractItemView::ScrollMode>("verticalScrollMode");
    QTest::addColumn<QMargins>("viewportMargins");
    QTest::addColumn<QSize>("delegateSize");
    QTest::addColumn<QListView::Flow>("flow");
    QTest::addColumn<bool>("horizontalScrollBarVisible");
    QTest::addColumn<bool>("verticalScrollBarVisible");

    QTest::newRow("TopToBottom, count:0")
            << QSize(200, 100)
            << 0
            << QListView::ScrollPerItem
            << QMargins() << QSize()
            << QListView::TopToBottom
            << false
            << false;

    QTest::newRow("TopToBottom, count:1")
            << QSize(200, 100)
            << 1
            << QListView::ScrollPerItem
            << QMargins() << QSize()
            << QListView::TopToBottom
            << false
            << false;

    QTest::newRow("TopToBottom, count:20")
            << QSize(200, 100)
            << 20
            << QListView::ScrollPerItem
            << QMargins() << QSize()
            << QListView::TopToBottom
            << false
            << true;

    QTest::newRow("TopToBottom, fixed size, count:4")
            << QSize(200, 200)
            << 4
            << QListView::ScrollPerPixel
            << QMargins() << QSize(40, 40)
            << QListView::TopToBottom
            << false
            << false;

    // QTBUG-61383, vertical case: take viewport margins into account
    QTest::newRow("TopToBottom, fixed size, vertical margins, count:4")
            << QSize(200, 200)
            << 4
            << QListView::ScrollPerPixel
            << QMargins(0, 50, 0, 50) << QSize(40, 40)
            << QListView::TopToBottom
            << false
            << true;

    // QTBUG-61383, horizontal case: take viewport margins into account
    QTest::newRow("TopToBottom, fixed size, horizontal margins, count:4")
            << QSize(200, 200)
            << 4
            << QListView::ScrollPerPixel
            << QMargins(50, 0, 50, 0) << QSize(120, 40)
            << QListView::TopToBottom
            << true
            << false;

    QTest::newRow("LeftToRight, count:0")
            << QSize(200, 100)
            << 0
            << QListView::ScrollPerItem
            << QMargins() << QSize()
            << QListView::LeftToRight
            << false
            << false;

    QTest::newRow("LeftToRight, count:1")
            << QSize(200, 100)
            << 1
            << QListView::ScrollPerItem
            << QMargins() << QSize()
            << QListView::LeftToRight
            << false
            << false;

    QTest::newRow("LeftToRight, count:20")
            << QSize(200, 100)
            << 20
            << QListView::ScrollPerItem
            << QMargins() << QSize()
            << QListView::LeftToRight
            << true
            << false;


}

void tst_QListView::scrollBarAsNeeded()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QFETCH(QSize, size);
    QFETCH(int, itemCount);
    QFETCH(QAbstractItemView::ScrollMode, verticalScrollMode);
    QFETCH(QMargins, viewportMargins);
    QFETCH(QSize, delegateSize);
    QFETCH(QListView::Flow, flow);
    QFETCH(bool, horizontalScrollBarVisible);
    QFETCH(bool, verticalScrollBarVisible);


    constexpr int rowCounts[3] = {0, 1, 20};

    QWidget topLevel;
    topLevel.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QStringLiteral("::")
                            + QLatin1String(QTest::currentDataTag()));
    PublicListView lv(&topLevel);
    lv.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    lv.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    lv.setVerticalScrollMode(verticalScrollMode);
    lv.setViewportMargins(viewportMargins);
    lv.setFlow(flow);
    if (!delegateSize.isEmpty())
        lv.setItemDelegate(new TestDelegate(&lv, delegateSize));

    QStringListModel model(&lv);
    lv.setModel(&model);
    lv.resize(size);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowActive(&topLevel));

    for (uint r = 0; r < sizeof(rowCounts) / sizeof(int); ++r) {
        model.setStringList(generateList(QLatin1String("Item "), rowCounts[r]));
        model.setStringList(generateList(QLatin1String("Item "), itemCount));

        QTRY_COMPARE(lv.horizontalScrollBar()->isVisible(), horizontalScrollBarVisible);
        QTRY_COMPARE(lv.verticalScrollBar()->isVisible(), verticalScrollBarVisible);
    }
}

void tst_QListView::moveItems()
{
    QStandardItemModel model;
    for (int r = 0; r < 4; ++r) {
        const QString prefix = QLatin1String("standard item (") + QString::number(r) + QLatin1Char(',');
        for (int c = 0; c < 4; ++c)
            model.setItem(r, c, new QStandardItem(prefix + QString::number(c) + QLatin1Char(')')));
    }

    PublicListView view;
    view.setViewMode(QListView::IconMode);
    view.setResizeMode(QListView::Fixed);
    view.setWordWrap(true);
    view.setModel(&model);
    view.setItemDelegate(new TestDelegate(&view));

    for (int r = 0; r < model.rowCount(); ++r) {
        for (int c = 0; c < model.columnCount(); ++c) {
            const QModelIndex& idx = model.index(r, c);
            view.setPositionForIndex(QPoint(r * 75, r * 75), idx);
        }
    }

    QCOMPARE(view.contentsSize(), QSize(275, 275));
}

void tst_QListView::wordWrap()
{
    QListView lv;
    lv.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    lv.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QStringListModel model(&lv);
    QStringList list;
    list << "Short item 1";
    list << "Short item 2";
    list << "Short item 3";
    list << "Begin\nThis item take severals Lines\nEnd";
    list << "And this is a very long item very long item this is a very vary vary long item"
            "very long very very long long long this is a long item a very long item a very very long item";
    list << "And this is a second even a little more long very long item very long item this is a very vary vary long item"
            "very long very very long long long this is a long item a very long item a very very long item";
    list << "Short item";
    list << "rzeofig zerig fslfgj smdlfkgj qmsdlfj amrzriougf qsla zrg fgsdf gsdfg sdfgs dfg sdfgcvb sdfg qsdjfh qsdfjklh qs";
    list << "Short item";
    model.setStringList(list);
    lv.setModel(&model);
    lv.setWordWrap(true);
    lv.setFixedSize(400, 150);
    lv.showNormal();

    QTRY_COMPARE(lv.horizontalScrollBar()->isVisible(), false);
#ifdef Q_OS_WINRT
QSKIP("setFixedSize does not work on WinRT. Vertical scroll bar will not be visible.");
#endif
    QTRY_COMPARE(lv.verticalScrollBar()->isVisible(), true);
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
class SetCurrentIndexAfterAppendRowCrashDialog : public QDialog
{
    Q_OBJECT
public:
    SetCurrentIndexAfterAppendRowCrashDialog()
    {
        setWindowTitle(QTest::currentTestFunction());
        listView = new QListView(this);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(listView);
        listView->setViewMode(QListView::IconMode);

        model = new QStandardItemModel(this);
        listView->setModel(model);

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout,
                this, &SetCurrentIndexAfterAppendRowCrashDialog::buttonClicked);
        timer->start(1000);
    }

protected:
    void showEvent(QShowEvent *event) override
    {
        QDialog::showEvent(event);
        DWORD lParam = 0xFFFFFFFC/*OBJID_CLIENT*/;
        DWORD wParam = 0;
        if (const HWND hwnd = getHWNDForWidget(this))
            SendMessage(hwnd, WM_GETOBJECT, wParam, lParam);
    }

private slots:
    void buttonClicked()
    {
        timer->stop();
        QStandardItem *item = new QStandardItem("test");
        model->appendRow(item);
        listView->setCurrentIndex(model->indexFromItem(item));
        close();
    }
private:
    QListView *listView;
    QStandardItemModel *model;
    QTimer *timer;
};

void tst_QListView::setCurrentIndexAfterAppendRowCrash()
{
    SetCurrentIndexAfterAppendRowCrashDialog w;
    w.exec();
}
#endif // Q_OS_WIN && !Q_OS_WINRT

void tst_QListView::emptyItemSize()
{
    QStandardItemModel model;
    for (int r = 0; r < 4; ++r) {
        const QString text = QLatin1String("standard item (") + QString::number(r) + QLatin1Char(')');
        model.setItem(r, new QStandardItem(text));
    }

    model.setItem(4, 0, new QStandardItem());

    PublicListView view;
    view.setModel(&model);

    for (int i = 0; i < 5; ++i)
        QVERIFY(!view.visualRect(model.index(i, 0)).isEmpty());
}

void tst_QListView::task203585_selectAll()
{
    //we make sure that "select all" doesn't select the hidden items
    QListView view;
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.setModel(new QStringListModel({"foo"}, &view));
    view.setRowHidden(0, true);
    view.selectAll();
    QVERIFY(view.selectionModel()->selectedIndexes().isEmpty());
    view.setRowHidden(0, false);
    view.selectAll();
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
}

void tst_QListView::task228566_infiniteRelayout()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QListView view;

    QStringList list;
    for (int i = 0; i < 10; ++i)
        list << "small";

    list << "BIGBIGBIGBIGBIGBIGBIGBIGBIGBIGBIGBIG"
         << "BIGBIGBIGBIGBIGBIGBIGBIGBIGBIGBIGBIG";

    QStringListModel model(list);
    view.setModel(&model);
    view.setWrapping(true);
    view.setResizeMode(QListView::Adjust);

    const int itemHeight = view.visualRect( model.index(0, 0)).height();

    view.setFixedHeight(itemHeight * 12);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTest::qWait(100); //make sure the layout is done once

    QSignalSpy spy(view.horizontalScrollBar(), &QScrollBar::rangeChanged);

    //the layout should already have been done
    //so there should be no change made to the scrollbar
    QVERIFY(!spy.wait(200));
}

void tst_QListView::task248430_crashWith0SizedItem()
{
    QListView view;
    view.setViewMode(QListView::IconMode);
    QStringListModel model({QLatin1String("item1"), QString()});
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(20);
}

void tst_QListView::task250446_scrollChanged()
{
    QStandardItemModel model(200, 1);
    QListView view;
    view.setModel(&model);
    QModelIndex index = model.index(0, 0);
    QVERIFY(index.isValid());
    view.setCurrentIndex(index);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    const int scrollValue = view.verticalScrollBar()->maximum();
    view.verticalScrollBar()->setValue(scrollValue);
    QTRY_COMPARE(view.verticalScrollBar()->value(), scrollValue);
    QTRY_COMPARE(view.currentIndex(), index);

    view.showMinimized();
    QTRY_COMPARE(view.verticalScrollBar()->value(), scrollValue);
    QTRY_COMPARE(view.currentIndex(), index);

    view.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_COMPARE(view.verticalScrollBar()->value(), scrollValue);
    QTRY_COMPARE(view.currentIndex(), index);
}

void tst_QListView::task196118_visualRegionForSelection()
{
    PublicListView view;
    QStandardItemModel model;
    QStandardItem top1("top1");
    QStandardItem sub1("sub1");
    top1.appendRow(&sub1);
    model.appendColumn({&top1});
    view.setModel(&model);
    view.setRootIndex(top1.index());

    view.selectionModel()->select(top1.index(), QItemSelectionModel::Select);

    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
    QVERIFY(view.getVisualRegionForSelection().isEmpty());
}

void tst_QListView::task254449_draggingItemToNegativeCoordinates()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    //we'll check that the items are painted correctly
    PublicListView list;
    QStandardItemModel model(1, 1);
    QModelIndex index = model.index(0, 0);
    model.setData(index, QLatin1String("foo"));
    list.setModel(&model);
    list.setViewMode(QListView::IconMode);
    list.show();
    list.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&list));


    class MyItemDelegate : public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        void paint(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
        {
            numPaints++;
            QStyledItemDelegate::paint(painter, option, index);
        }

        mutable int numPaints = 0;
    } delegate;
    list.setItemDelegate(&delegate);
    QTRY_VERIFY(delegate.numPaints > 0);  //makes sure the layout is done

    //we'll make sure the item is repainted
    delegate.numPaints = 0;
    const QPoint topLeft(-6, 0);
    list.setPositionForIndex(topLeft, index);
    QTRY_COMPARE(delegate.numPaints, 1);
    QCOMPARE(list.visualRect(index).topLeft(), topLeft);
}


void tst_QListView::keyboardSearch()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStringListModel model({"AB", "AC", "BA", "BB", "BD", "KAFEINE",
                            "KONQUEROR", "KOPETE", "KOOKA", "OKULAR"});

    QListView view;
    view.setModel(&model);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QTest::keyClick(&view, Qt::Key_K);
    QTRY_COMPARE(view.currentIndex() , model.index(5, 0)); //KAFEINE

    QTest::keyClick(&view, Qt::Key_O);
    QTRY_COMPARE(view.currentIndex() , model.index(6, 0)); //KONQUEROR

    QTest::keyClick(&view, Qt::Key_N);
    QTRY_COMPARE(view.currentIndex() , model.index(6, 0)); //KONQUEROR
}

void tst_QListView::shiftSelectionWithNonUniformItemSizes()
{
    // This checks that no items are selected unexpectedly by Shift-Arrow
    // when items with non-uniform sizes are laid out in a grid
    {   // First test: QListView::LeftToRight flow
        QStringListModel model({"Long\nText", "Text", "Text","Text"});

        QListView view;
        view.setFixedSize(250, 250);
        view.setFlow(QListView::LeftToRight);
        view.setGridSize(QSize(100, 100));
        view.setSelectionMode(QListView::ExtendedSelection);
        view.setViewMode(QListView::IconMode);
        view.setModel(&model);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        // Verfify that item sizes are non-uniform
        QVERIFY(view.sizeHintForIndex(model.index(0, 0)).height() > view.sizeHintForIndex(model.index(1, 0)).height());

        QModelIndex index = model.index(3, 0);
        view.setCurrentIndex(index);
        QCOMPARE(view.currentIndex(), index);

        QTest::keyClick(&view, Qt::Key_Up, Qt::ShiftModifier);
        QTRY_COMPARE(view.currentIndex(), model.index(1, 0));

        QModelIndexList selected = view.selectionModel()->selectedIndexes();
        QCOMPARE(selected.count(), 3);
        QVERIFY(!selected.contains(model.index(0, 0)));
    }
    {   // Second test: QListView::TopToBottom flow
        QStringListModel model({"ab", "a", "a", "a"});

        QListView view;
        view.setFixedSize(250, 250);
        view.setFlow(QListView::TopToBottom);
        view.setGridSize(QSize(100, 100));
        view.setSelectionMode(QListView::ExtendedSelection);
        view.setViewMode(QListView::IconMode);
        view.setModel(&model);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        // Verfify that item sizes are non-uniform
        QVERIFY(view.sizeHintForIndex(model.index(0, 0)).width() > view.sizeHintForIndex(model.index(1, 0)).width());

        QModelIndex index = model.index(3, 0);
        view.setCurrentIndex(index);
        QCOMPARE(view.currentIndex(), index);

        QTest::keyClick(&view, Qt::Key_Left, Qt::ShiftModifier);
        QTRY_COMPARE(view.currentIndex(), model.index(1, 0));

        QModelIndexList selected = view.selectionModel()->selectedIndexes();
        QCOMPARE(selected.count(), 3);
        QVERIFY(!selected.contains(model.index(0, 0)));
    }
}

void tst_QListView::shiftSelectionWithItemAlignment()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStringList items;
    for (int c = 0; c < 2; c++) {
        for (int i = 10; i > 0; i--)
            items << QString(i, QLatin1Char('*'));

        for (int i = 1; i < 11; i++)
            items << QString(i, QLatin1Char('*'));
    }

    QListView view;
    view.setFlow(QListView::TopToBottom);
    view.setWrapping(true);
    view.setItemAlignment(Qt::AlignLeft);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);

    QStringListModel model(items);
    view.setModel(&model);

    QFont font = view.font();
    font.setPixelSize(10);
    view.setFont(font);
    view.resize(300, view.sizeHintForRow(0) * items.size() / 2 + view.horizontalScrollBar()->height());

    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(static_cast<QWidget *>(&view), QApplication::activeWindow());

    QModelIndex index1 = view.model()->index(items.size() / 4, 0);
    QPoint p = view.visualRect(index1).center();
    QVERIFY(view.viewport()->rect().contains(p));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, p);
    QCOMPARE(view.currentIndex(), index1);
    QCOMPARE(view.selectionModel()->selectedIndexes().size(), 1);

    QModelIndex index2 = view.model()->index(items.size() / 4 * 3, 0);
    p = view.visualRect(index2).center();
    QVERIFY(view.viewport()->rect().contains(p));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, p);
    QCOMPARE(view.currentIndex(), index2);
    QCOMPARE(view.selectionModel()->selectedIndexes().size(), index2.row() - index1.row() + 1);
}

void tst_QListView::clickOnViewportClearsSelection()
{
    QStringListModel model({"Text1"});
    QListView view;
    view.setModel(&model);
    view.setSelectionMode(QListView::ExtendedSelection);

    view.selectAll();
    QModelIndex index = model.index(0);
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
    QVERIFY(view.selectionModel()->isSelected(index));

    //we try to click outside of the index
    const QPoint point = view.visualRect(index).bottomRight() + QPoint(10,10);

    QTest::mousePress(view.viewport(), Qt::LeftButton, {}, point);
    //at this point, the selection shouldn't have changed
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
    QVERIFY(view.selectionModel()->isSelected(index));

    QTest::mouseRelease(view.viewport(), Qt::LeftButton, {}, point);
    //now the selection should be cleared
    QVERIFY(!view.selectionModel()->hasSelection());
}

void tst_QListView::task262152_setModelColumnNavigate()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QListView view;
    QStandardItemModel model(3,2);
    model.setItem(0, 1, new QStandardItem("[0,1]"));
    model.setItem(1, 1, new QStandardItem("[1,1]"));
    model.setItem(2, 1, new QStandardItem("[2,1]"));

    view.setModel(&model);
    view.setModelColumn(1);

    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(&view, QApplication::activeWindow());
    QTest::keyClick(&view, Qt::Key_Down);
    QTRY_COMPARE(view.currentIndex(), model.index(1, 1));
    QTest::keyClick(&view, Qt::Key_Down);
    QTRY_COMPARE(view.currentIndex(), model.index(2, 1));
}

void tst_QListView::taskQTBUG_2233_scrollHiddenItems_data()
{
    QTest::addColumn<QListView::Flow>("flow");

    QTest::newRow("TopToBottom") << QListView::TopToBottom;
    QTest::newRow("LeftToRight") << QListView::LeftToRight;
}

void tst_QListView::taskQTBUG_2233_scrollHiddenItems()
{
    QFETCH(QListView::Flow, flow);
    const int rowCount = 200;

    QWidget topLevel;
    setFrameless(&topLevel);
    ScrollPerItemListView view(&topLevel);
    QStringListModel model(&view);
    model.setStringList(generateList(QString(), rowCount));
    view.setModel(&model);
    view.setUniformItemSizes(true);
    view.setViewMode(QListView::ListMode);
    for (int i = 0; i < rowCount / 2; ++i)
        view.setRowHidden(2 * i, true);
    view.setFlow(flow);
    view.resize(130, 130);

    for (int i = 0; i < 10; ++i) {
        (view.flow() == QListView::TopToBottom
            ? view.verticalScrollBar()
            : view.horizontalScrollBar())->setValue(i);
        QModelIndex index = view.indexAt(QPoint(0, 0));
        QVERIFY(index.isValid());
        QCOMPARE(index.row(), 2 * i + 1);
    }

    //QTBUG-7929  should not crash
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QScrollBar *bar = view.flow() == QListView::TopToBottom
            ? view.verticalScrollBar() : view.horizontalScrollBar();

    int nbVisibleItem = rowCount / 2 - bar->maximum();

    bar->setValue(bar->maximum());
    for (int i = rowCount; i > rowCount / 2; i--)
        view.setRowHidden(i, true);
    QTRY_COMPARE(bar->maximum(), rowCount / 4 - nbVisibleItem);
    QCOMPARE(bar->value(), bar->maximum());
}

void tst_QListView::taskQTBUG_633_changeModelData()
{
    QListView view;
    view.setFlow(QListView::LeftToRight);
    QStandardItemModel model(5,1);
    for (int i = 0; i < model.rowCount(); ++i)
        model.setData(model.index(i, 0), QString::number(i));

    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    model.setData( model.index(1, 0), QLatin1String("long long text"));
    const auto longTextDoesNotIntersectNextItem = [&]() {
        QRect rectLongText = view.visualRect(model.index(1,0));
        QRect rect2 = view.visualRect(model.index(2,0));
        return !rectLongText.intersects(rect2);
    };
    QTRY_VERIFY(longTextDoesNotIntersectNextItem());
}

void tst_QListView::taskQTBUG_435_deselectOnViewportClick()
{
    QListView view;
    QStringListModel model({"1", "2", "3", "4"});
    view.setModel(&model);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.selectAll();
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), model.rowCount());


    const QRect itemRect = view.visualRect(model.index(model.rowCount() - 1));
    QPoint p = view.visualRect(model.index(model.rowCount() - 1)).center() + QPoint(0, itemRect.height());
    //first the left button
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, p);
    QVERIFY(!view.selectionModel()->hasSelection());

    view.selectAll();
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), model.rowCount());

    //and now the right button
    QTest::mouseClick(view.viewport(), Qt::RightButton, {}, p);
    QVERIFY(!view.selectionModel()->hasSelection());
}

void tst_QListView::taskQTBUG_2678_spacingAndWrappedText()
{
    static const QString lorem("Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    QStringListModel model({lorem, lorem, "foo", lorem, "bar", lorem, lorem});
    QListView w;
    w.setModel(&model);
    w.setViewMode(QListView::ListMode);
    w.setWordWrap(true);
    w.setSpacing(10);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QCOMPARE(w.horizontalScrollBar()->minimum(), w.horizontalScrollBar()->maximum());
}

void tst_QListView::taskQTBUG_5877_skippingItemInPageDownUp()
{
    QtTestModel model(100, 1);

    static const int currentItemIndexes[] =
        {0, 6, 16, 25, 34, 42, 57, 68, 77, 83, 91, 94};
    PublicListView vu;
    vu.setModel(&model);
    vu.show();

    QVERIFY(QTest::qWaitForWindowExposed(&vu));

    int itemHeight = vu.visualRect(model.index(0, 0)).height();
    int visibleRowCount = vu.viewport()->height() / itemHeight;
    int scrolledRowCount = visibleRowCount - 1;

    for (int currentItemIndex : currentItemIndexes) {
        vu.selectionModel()->setCurrentIndex(model.index(currentItemIndex, 0),
                                             QItemSelectionModel::SelectCurrent);

        QModelIndex idx = vu.moveCursor(PublicListView::MovePageDown, Qt::NoModifier);
        int newCurrent = qMin(currentItemIndex + scrolledRowCount, 99);
        QCOMPARE(idx, model.index(newCurrent, 0));

        idx = vu.moveCursor(PublicListView::MovePageUp, Qt::NoModifier);
        newCurrent = qMax(currentItemIndex - scrolledRowCount, 0);
        QCOMPARE(idx, model.index(newCurrent, 0));
    }
}

void tst_QListView::taskQTBUG_9455_wrongScrollbarRanges()
{
    QStringListModel model(generateList("item ", 8));
    PublicListView w;
    setFrameless(&w);
    w.setModel(&model);
    w.setViewMode(QListView::IconMode);
    w.resize(116, 132);
    w.setMovement(QListView::Static);
    w.setSpacing(200);
    w.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QCOMPARE(w.verticalScrollBar()->maximum(),
             w.contentsSize().height() - w.viewport()->geometry().height());
}

void tst_QListView::styleOptionViewItem()
{
    class MyDelegate : public QStyledItemDelegate
    {
        public:
            void paint(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const override
            {
                QStyleOptionViewItem opt(option);
                initStyleOption(&opt, index);

                QCOMPARE(opt.index, index);

                QStyledItemDelegate::paint(painter, option, index);
            }
    };

    QListView view;
    QStandardItemModel model;
    view.setModel(&model);
    MyDelegate delegate;
    view.setItemDelegate(&delegate);
    model.appendRow({new QStandardItem("Beginning"),
                     new QStandardItem("Middle"),
                     new QStandardItem("Middle"),
                     new QStandardItem("End")});

    // Run test
    view.showMaximized();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

void tst_QListView::taskQTBUG_12308_artihmeticException()
{
    QListWidget lw;
    lw.setLayoutMode(QListView::Batched);
    lw.setViewMode(QListView::IconMode);
    for (int i = 0; i < lw.batchSize() + 1; i++) {
        QListWidgetItem *item = new QListWidgetItem(
              QLatin1String("Item ") + QString::number(i));
        lw.addItem(item);
        item->setHidden(true);
    }
    lw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&lw));
    // No crash, it's all right.
}

class Delegate12308 : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QVERIFY(option.rect.topLeft() != QPoint(-1, -1));
        QStyledItemDelegate::paint(painter, option, index);
    }
};

void tst_QListView::taskQTBUG_12308_wrongFlowLayout()
{
    QListWidget lw;
    Delegate12308 delegate;
    lw.setLayoutMode(QListView::Batched);
    lw.setViewMode(QListView::IconMode);
    lw.setItemDelegate(&delegate);
    for (int i = 0; i < lw.batchSize() + 1; i++) {
        QListWidgetItem *item = new QListWidgetItem(
              QLatin1String("Item ") + QString::number(i));
        lw.addItem(item);
        if (!item->text().contains(QLatin1Char('1')))
            item->setHidden(true);
    }
    lw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&lw));
}

void tst_QListView::taskQTBUG_21115_scrollToAndHiddenItems_data()
{
    QTest::addColumn<QListView::Flow>("flow");
    QTest::newRow("flow TopToBottom") << QListView::TopToBottom;
    QTest::newRow("flow LeftToRight") << QListView::LeftToRight;
}

void tst_QListView::taskQTBUG_21115_scrollToAndHiddenItems()
{
    QFETCH(QListView::Flow, flow);
#ifdef Q_OS_WINRT
    QSKIP("Fails on WinRT - QTBUG-68297");
#endif

    ScrollPerItemListView lv;
    lv.setUniformItemSizes(true);
    lv.setFlow(flow);

    QStringListModel model;
    model.setStringList(generateList(QString(), 30));
    lv.setModel(&model);
    lv.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&lv));

    // Save first item rect for reference
    QRect firstItemRect = lv.visualRect(model.index(0, 0));

    // Select an item and scroll to selection
    QModelIndex index = model.index(2, 0);
    lv.setCurrentIndex(index);
    lv.scrollTo(index, QAbstractItemView::PositionAtTop);
    QTRY_COMPARE(lv.visualRect(index), firstItemRect);

    // Hide some rows and scroll to selection
    for (int i = 0; i < 5; i++) {
        if (i == index.row())
            continue;
        lv.setRowHidden(i, true);
    }
    lv.scrollTo(index, QAbstractItemView::PositionAtTop);
    QTRY_COMPARE(lv.visualRect(index), firstItemRect);
}

void tst_QListView::draggablePaintPairs_data()
{
    QTest::addColumn<int>("row");

    for (int row = 0; row < 30; ++row)
      QTest::newRow("row-" + QByteArray::number(row)) << row;
}

void tst_QListView::draggablePaintPairs()
{
    QFETCH(int, row);

    QListView view;

    QStringListModel model;
    model.setStringList(generateList(QString(), 30));
    view.setModel(&model);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex expectedIndex = model.index(row, 0);
    QListViewPrivate *privateClass = static_cast<QListViewPrivate *>(QListViewPrivate::get(&view));
    QRect rect;
    const QModelIndexList indexList{ expectedIndex };
    view.scrollTo(expectedIndex);
    const QItemViewPaintPairs pairs = privateClass->draggablePaintPairs(indexList, &rect);
    QCOMPARE(indexList.size(), pairs.size());
    for (const QItemViewPaintPair &pair : pairs) {
        QCOMPARE(rect, pair.rect);
        QCOMPARE(expectedIndex, pair.index);
    }
}

void tst_QListView::taskQTBUG_21804_hiddenItemsAndScrollingWithKeys_data()
{
    QTest::addColumn<QListView::Flow>("flow");
    QTest::addColumn<int>("spacing");
    QTest::newRow("flow TopToBottom no spacing") << QListView::TopToBottom << 0;
    QTest::newRow("flow TopToBottom with spacing") << QListView::TopToBottom << 5;
    QTest::newRow("flow LeftToRight no spacing") << QListView::LeftToRight << 0;
    QTest::newRow("flow LeftToRight with spacing") << QListView::LeftToRight << 5;
}

void tst_QListView::taskQTBUG_21804_hiddenItemsAndScrollingWithKeys()
{
    QFETCH(QListView::Flow, flow);
    QFETCH(int, spacing);

    // create some items to show
    QStringListModel model;
    model.setStringList(generateList(QString(), 60));

    // create listview
    ScrollPerItemListView lv;
    lv.setFlow(flow);
    lv.setSpacing(spacing);
    lv.setModel(&model);
    lv.show();
    QVERIFY(QTest::qWaitForWindowExposed(&lv));

    // hide every odd number row
    for (int i = 1; i < model.rowCount(); i+=2)
        lv.setRowHidden(i, true);

    // scroll forward and check that selected item is visible always
    int visibleItemCount = model.rowCount() / 2;
    for (int i = 0; i < visibleItemCount; i++) {
        if (flow == QListView::TopToBottom)
            QTest::keyClick(&lv, Qt::Key_Down);
        else
            QTest::keyClick(&lv, Qt::Key_Right);
        QTRY_VERIFY(lv.rect().contains(lv.visualRect(lv.currentIndex())));
    }

    // scroll backward
    for (int i = 0; i < visibleItemCount; i++) {
        if (flow == QListView::TopToBottom)
            QTest::keyClick(&lv, Qt::Key_Up);
        else
            QTest::keyClick(&lv, Qt::Key_Left);
        QTRY_VERIFY(lv.rect().contains(lv.visualRect(lv.currentIndex())));
    }

    // scroll forward only half way
    for (int i = 0; i < visibleItemCount / 2; i++) {
        if (flow == QListView::TopToBottom)
            QTest::keyClick(&lv, Qt::Key_Down);
        else
            QTest::keyClick(&lv, Qt::Key_Right);
        QTRY_VERIFY(lv.rect().contains(lv.visualRect(lv.currentIndex())));
    }

    // scroll backward again
    for (int i = 0; i < visibleItemCount / 2; i++) {
        if (flow == QListView::TopToBottom)
            QTest::keyClick(&lv, Qt::Key_Up);
        else
            QTest::keyClick(&lv, Qt::Key_Left);
        QTRY_VERIFY(lv.rect().contains(lv.visualRect(lv.currentIndex())));
    }
}

void tst_QListView::spacing_data()
{
    QTest::addColumn<QListView::Flow>("flow");
    QTest::addColumn<int>("spacing");
    QTest::newRow("flow=TopToBottom spacing=0") << QListView::TopToBottom << 0;
    QTest::newRow("flow=TopToBottom spacing=10") << QListView::TopToBottom << 10;
    QTest::newRow("flow=LeftToRight spacing=0") << QListView::LeftToRight << 0;
    QTest::newRow("flow=LeftToRight spacing=10") << QListView::LeftToRight << 10;
}

void tst_QListView::spacing()
{
    QFETCH(QListView::Flow, flow);
    QFETCH(int, spacing);

    // create some items to show
    QStringListModel model;
    model.setStringList(generateList(QString(), 60));

    // create listview
    ScrollPerItemListView lv;
    lv.setFlow(flow);
    lv.setModel(&model);
    lv.setSpacing(spacing);
    lv.show();
    QVERIFY(QTest::qWaitForWindowExposed(&lv));

    // check size and position of first two items
    QRect item1 = lv.visualRect(lv.model()->index(0, 0));
    QRect item2 = lv.visualRect(lv.model()->index(1, 0));
    QCOMPARE(item1.topLeft(), QPoint(flow == QListView::TopToBottom ? spacing : 0, spacing));
    if (flow == QListView::TopToBottom) {
        QCOMPARE(item1.width(), lv.viewport()->width() - 2 * spacing);
        QCOMPARE(item2.topLeft(), QPoint(spacing, spacing + item1.height() + 2 * spacing));
    }
    else { // QListView::LeftToRight
        QCOMPARE(item1.height(), lv.viewport()->height() - 2 * spacing);
        QCOMPARE(item2.topLeft(), QPoint(spacing + item1.width() + spacing, spacing));
    }
}

void tst_QListView::testScrollToWithHidden()
{
    QListView lv;

    QStringListModel model;
    model.setStringList(generateList(QString(), 30));
    lv.setModel(&model);

    lv.setRowHidden(1, true);
    lv.setSpacing(5);

    lv.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&lv));

    QCOMPARE(lv.verticalScrollBar()->value(), 0);

    lv.scrollTo(model.index(26, 0));
    int expectedScrollBarValue = lv.verticalScrollBar()->value();
#ifdef Q_OS_WINRT
    QSKIP("Might fail on WinRT - QTBUG-68297");
#endif
    QVERIFY(expectedScrollBarValue != 0);

    lv.scrollTo(model.index(25, 0));
    QCOMPARE(expectedScrollBarValue, lv.verticalScrollBar()->value());
}



void tst_QListView::testViewOptions()
{
    PublicListView view;
    QStyleOptionViewItem options = view.viewOptions();
    QCOMPARE(options.decorationPosition, QStyleOptionViewItem::Left);
    view.setViewMode(QListView::IconMode);
    options = view.viewOptions();
    QCOMPARE(options.decorationPosition, QStyleOptionViewItem::Top);
}

// make sure we have no transient scroll bars
class TempStyleSetter
{
public:
    TempStyleSetter()
        : m_oldStyle(QApplication::style())
    {
        m_oldStyle->setParent(nullptr);
        QListView tempView;
        if (QApplication::style()->styleHint(QStyle::SH_ScrollBar_Transient,
                                             nullptr, tempView.horizontalScrollBar()))
            QApplication::setStyle(QStyleFactory::create("Fusion"));
    }

    ~TempStyleSetter()
    {
        QApplication::setStyle(m_oldStyle);
    }
private:
    QStyle *m_oldStyle;
};

void tst_QListView::taskQTBUG_39902_mutualScrollBars_data()
{
    QTest::addColumn<QAbstractItemView::ScrollMode>("horizontalScrollMode");
    QTest::addColumn<QAbstractItemView::ScrollMode>("verticalScrollMode");
    QTest::newRow("per item / per item") << QAbstractItemView::ScrollPerItem
                                         << QAbstractItemView::ScrollPerItem;
    QTest::newRow("per pixel / per item") << QAbstractItemView::ScrollPerPixel
                                          << QAbstractItemView::ScrollPerItem;
    QTest::newRow("per item / per pixel") << QAbstractItemView::ScrollPerItem
                                          << QAbstractItemView::ScrollPerPixel;
    QTest::newRow("per pixel / per pixel") << QAbstractItemView::ScrollPerPixel
                                           << QAbstractItemView::ScrollPerPixel;
}

void tst_QListView::taskQTBUG_39902_mutualScrollBars()
{
    QFETCH(QAbstractItemView::ScrollMode, horizontalScrollMode);
    QFETCH(QAbstractItemView::ScrollMode, verticalScrollMode);

    QWidget window;
    window.resize(400, 300);
    QListView *view = new QListView(&window);
    // make sure we have no transient scroll bars
    TempStyleSetter styleSetter;
    QStandardItemModel model(200, 1);
    const QSize itemSize(100, 20);
    for (int i = 0; i < model.rowCount(); ++i)
        model.setData(model.index(i, 0), itemSize, Qt::SizeHintRole);
    view->setModel(&model);

    view->setVerticalScrollMode(verticalScrollMode);
    view->setHorizontalScrollMode(horizontalScrollMode);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    // make sure QListView is done with layouting the items (1/10 sec, like QListView)
    QTest::qWait(100);

    model.setRowCount(2);
    for (int i = 0; i < model.rowCount(); ++i)
        model.setData(model.index(i, 0), itemSize, Qt::SizeHintRole);
    view->resize(itemSize.width() + view->frameWidth() * 2,
                 model.rowCount() * itemSize.height() + view->frameWidth() * 2);
    // this will end up in a stack overflow, if QTBUG-39902 is not fixed
    QTest::qWait(100);

    // these tests do not apply with transient scroll bars enabled
    QVERIFY (!view->style()->styleHint(QStyle::SH_ScrollBar_Transient,
                                       nullptr, view->horizontalScrollBar()));

    // make it double as large, no scroll bars should be visible
    view->resize((itemSize.width() + view->frameWidth() * 2) * 2,
                 (model.rowCount() * itemSize.height() + view->frameWidth() * 2) * 2);
    QTRY_VERIFY(!view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(!view->verticalScrollBar()->isVisible());

    // make it half the size, both scroll bars should be visible
    view->resize((itemSize.width() + view->frameWidth() * 2) / 2,
                 (model.rowCount() * itemSize.height() + view->frameWidth() * 2) / 2);
    QTRY_VERIFY(view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(view->verticalScrollBar()->isVisible());

    // make it double as large, no scroll bars should be visible
    view->resize((itemSize.width() + view->frameWidth() * 2) * 2,
                 (model.rowCount() * itemSize.height() + view->frameWidth() * 2) * 2);
    QTRY_VERIFY(!view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(!view->verticalScrollBar()->isVisible());

    // now, coming from the double size, resize it to the exactly matching size, still no scroll bars should be visible again
    view->resize(itemSize.width() + view->frameWidth() * 2,
                 model.rowCount() * itemSize.height() + view->frameWidth() * 2);
    QTRY_VERIFY(!view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(!view->verticalScrollBar()->isVisible());

    // now remove just one single pixel in height -> both scroll bars will show up since they depend on each other
    view->resize(itemSize.width() + view->frameWidth() * 2,
                 model.rowCount() * itemSize.height() + view->frameWidth() * 2 - 1);
    QTRY_VERIFY(view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(view->verticalScrollBar()->isVisible());

    // now remove just one single pixel in width -> both scroll bars will show up since they depend on each other
    view->resize(itemSize.width() + view->frameWidth() * 2 - 1,
                 model.rowCount() * itemSize.height() + view->frameWidth() * 2);
    QTRY_VERIFY(view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(view->verticalScrollBar()->isVisible());

    // finally, coming from a size being to small, resize back to the exactly matching size -> both scroll bars should disappear again
    view->resize(itemSize.width() + view->frameWidth() * 2,
                 model.rowCount() * itemSize.height() + view->frameWidth() * 2);
    QTRY_VERIFY(!view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(!view->verticalScrollBar()->isVisible());

   // now remove just one single pixel in height -> both scroll bars will show up since they depend on each other
    view->resize(itemSize.width() + view->frameWidth() * 2,
                 model.rowCount() * itemSize.height() + view->frameWidth() * 2 - 1);
    QTRY_VERIFY(view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(view->verticalScrollBar()->isVisible());
}

void tst_QListView::horizontalScrollingByVerticalWheelEvents()
{
#if QT_CONFIG(wheelevent)
    QListView lv;
    lv.setWrapping(true);

    lv.setItemDelegate(new TestDelegate(&lv, QSize(100, 100)));

    QtTestModel model(100, 1);
    lv.setModel(&model);
    lv.resize(300, 300);
    lv.show();
    QVERIFY(QTest::qWaitForWindowExposed(&lv));

    QPoint globalPos = lv.geometry().center();
    QPoint pos = lv.viewport()->geometry().center();

    QWheelEvent wheelDownEvent(pos, globalPos, QPoint(0, 0), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QWheelEvent wheelUpEvent(pos, globalPos, QPoint(0, 0), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QWheelEvent wheelLeftDownEvent(pos, globalPos, QPoint(0, 0), QPoint(120, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);

    int hValue = lv.horizontalScrollBar()->value();
    QCoreApplication::sendEvent(lv.viewport(), &wheelDownEvent);
    QVERIFY(lv.horizontalScrollBar()->value() > hValue);

    QCoreApplication::sendEvent(lv.viewport(), &wheelUpEvent);
    QCOMPARE(lv.horizontalScrollBar()->value(), hValue);

    QCoreApplication::sendEvent(lv.viewport(), &wheelLeftDownEvent);
    QCOMPARE(lv.horizontalScrollBar()->value(), hValue);

    // ensure that vertical wheel events are not converted when vertical
    // scroll bar is not visible but vertical scrolling is possible
    lv.setWrapping(false);
    lv.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QCoreApplication::processEvents();

    int vValue = lv.verticalScrollBar()->value();
    QCoreApplication::sendEvent(lv.viewport(), &wheelDownEvent);
    QVERIFY(lv.verticalScrollBar()->value() > vValue);
#else
    QSKIP("Built with --no-feature-wheelevent");
#endif
}

void tst_QListView::taskQTBUG_7232_AllowUserToControlSingleStep()
{
    // When we set the scrollMode to ScrollPerPixel it will adjust the scrollbars singleStep automatically
    // Setting a singlestep on a scrollbar should however imply that the user takes control.
    // Setting a singlestep to -1 return to an automatic control of the singleStep.
    QListView lv;
    lv.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    lv.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    QStandardItemModel model(1000, 100);
    QString str = QString::fromLatin1("This is a long string made to ensure that we get some horizontal scroll (and we want scroll)");
    model.setData(model.index(0, 0), str);
    lv.setModel(&model);
    lv.setGeometry(150, 150, 150, 150);
    lv.show();
    lv.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    lv.setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    QVERIFY(QTest::qWaitForWindowExposed(&lv));

    int vStep1 = lv.verticalScrollBar()->singleStep();
    int hStep1 = lv.horizontalScrollBar()->singleStep();
    QVERIFY(lv.verticalScrollBar()->singleStep() > 1);
    QVERIFY(lv.horizontalScrollBar()->singleStep() > 1);

    lv.verticalScrollBar()->setSingleStep(1);
    lv.setGeometry(200, 200, 200, 200);
    QCOMPARE(lv.verticalScrollBar()->singleStep(), 1);

    lv.horizontalScrollBar()->setSingleStep(1);
    lv.setGeometry(150, 150, 150, 150);
    QCOMPARE(lv.horizontalScrollBar()->singleStep(), 1);

    lv.verticalScrollBar()->setSingleStep(-1);
    lv.horizontalScrollBar()->setSingleStep(-1);
    QCOMPARE(vStep1, lv.verticalScrollBar()->singleStep());
    QCOMPARE(hStep1, lv.horizontalScrollBar()->singleStep());
}

void tst_QListView::taskQTBUG_51086_skippingIndexesInSelectedIndexes()
{
    QStandardItemModel data(10, 1);
    QItemSelectionModel selections(&data);
    PublicListView list;
    list.setModel(&data);
    list.setSelectionModel(&selections);

    list.setRowHidden(7, true);
    list.setRowHidden(8, true);

    for (int i = 0, count = data.rowCount(); i < count; ++i)
        selections.select(data.index(i, 0), QItemSelectionModel::Select);

    const QModelIndexList indexes = list.selectedIndexes();

    QVERIFY(!indexes.contains(data.index(7, 0)));
    QVERIFY(!indexes.contains(data.index(8, 0)));
}

void tst_QListView::taskQTBUG_47694_indexOutOfBoundBatchLayout()
{
    QListView view;
    view.setLayoutMode(QListView::Batched);
    int batchSize = view.batchSize();

    QStandardItemModel model(batchSize + 1, 1);

    view.setModel(&model);

    view.scrollTo(model.index(batchSize - 1, 0));
}

void tst_QListView::itemAlignment()
{
    auto item1 = new QStandardItem("111");
    auto item2 = new QStandardItem("111111");
    QStandardItemModel model;
    model.appendRow(item1);
    model.appendRow(item2);

    QListView w;
    w.setModel(&model);
    w.setWrapping(true);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QVERIFY(w.visualRect(item1->index()).width() > 0);
    QVERIFY(w.visualRect(item1->index()).width() == w.visualRect(item2->index()).width());

    w.setItemAlignment(Qt::AlignLeft);
    QCoreApplication::processEvents();

    QVERIFY(w.visualRect(item1->index()).width() < w.visualRect(item2->index()).width());
}

void tst_QListView::internalDragDropMove_data()
{
    QTest::addColumn<QListView::ViewMode>("viewMode");
    QTest::addColumn<QAbstractItemView::DragDropMode>("dragDropMode");
    QTest::addColumn<Qt::DropActions>("supportedDropActions");
    QTest::addColumn<Qt::DropAction>("defaultDropAction");
    QTest::addColumn<Qt::ItemFlags>("itemFlags");
    QTest::addColumn<bool>("modelMoves");
    QTest::addColumn<QStringList>("expectedData");

    const Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable
                                     | Qt::ItemIsEnabled
                                     | Qt::ItemIsEditable
                                     | Qt::ItemIsDragEnabled;

    const QStringList reordered = QStringList{"0", "2", "3", "4", "5", "6", "7", "8", "9", "1"};
    const QStringList replaced = QStringList{"0", "2", "3", "4", "1", "6", "7", "8", "9"};

    for (auto viewMode : { QListView::IconMode, QListView::ListMode }) {
        for (auto modelMoves : { true, false } ) {
            QByteArray rowName = viewMode == QListView::IconMode ? "icon" : "list" ;
            rowName += modelMoves ? ", model moves" : ", model doesn't move";
            QTest::newRow((rowName + ", copy&move").constData())
                << viewMode
                << QAbstractItemView::InternalMove
                << (Qt::CopyAction|Qt::MoveAction)
                << Qt::MoveAction
                << defaultFlags
                << modelMoves
                << reordered;

            QTest::newRow((rowName + ", only move").constData())
                << viewMode
                << QAbstractItemView::InternalMove
                << (Qt::IgnoreAction|Qt::MoveAction)
                << Qt::MoveAction
                << defaultFlags
                << modelMoves
                << reordered;

            QTest::newRow((rowName + ", replace item").constData())
                << viewMode
                << QAbstractItemView::InternalMove
                << (Qt::IgnoreAction|Qt::MoveAction)
                << Qt::MoveAction
                << (defaultFlags | Qt::ItemIsDropEnabled)
                << modelMoves
                << replaced;
        }
    }
}

/*
    Test moving of items items via drag'n'drop.

    This should reorder items when an item is dropped in between two items,
    or - if items can be dropped on - replace the content of the drop target.

    Test QListView in both icon and list view modes.

    See QTBUG-67440, QTBUG-83084, QTBUG-87057
*/
void tst_QListView::internalDragDropMove()
{
    const QString platform(QGuiApplication::platformName().toLower());
    if (platform != QLatin1String("xcb"))
        QSKIP("Need a window system with proper DnD support via injected mouse events");

    QFETCH(QListView::ViewMode, viewMode);
    QFETCH(QAbstractItemView::DragDropMode, dragDropMode);
    QFETCH(Qt::DropActions, supportedDropActions);
    QFETCH(Qt::DropAction, defaultDropAction);
    QFETCH(Qt::ItemFlags, itemFlags);
    QFETCH(bool, modelMoves);
    QFETCH(QStringList, expectedData);

    class ItemModel : public QStringListModel
    {
    public:
        ItemModel()
        {
            QStringList list;
            for (int i = 0; i < 10; ++i) {
                list << QString::number(i);
            }
            setStringList(list);
        }

        Qt::DropActions supportedDropActions() const override { return m_supportedDropActions; }
        Qt::ItemFlags flags(const QModelIndex &index) const override
        {
            if (!index.isValid())
                return QStringListModel::flags(index);
            return m_itemFlags;
        }
        bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                      const QModelIndex &destinationParent, int destinationChild) override
        {
            if (!m_modelMoves) // many models don't implement moveRows
                return false;
            return QStringListModel::moveRows(sourceParent, sourceRow, count,
                                              destinationParent, destinationChild);
        }
        bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &values) override
        {
            return QStringListModel::setData(index, values.value(Qt::DisplayRole), Qt::DisplayRole);
        }
        QVariant data(const QModelIndex &index, int role) const override
        {
            if (role == Qt::DecorationRole)
                return QColor(Qt::GlobalColor(index.row() + 1));
            return QStringListModel::data(index, role);
        }
        QMap<int, QVariant> itemData(const QModelIndex &index) const override
        {
            auto item = QStringListModel::itemData(index);
            item[Qt::DecorationRole] = data(index, Qt::DecorationRole);
            return item;
        }

        Qt::DropActions m_supportedDropActions;
        Qt::ItemFlags m_itemFlags;
        bool m_modelMoves;
    };

    ItemModel data;
    data.m_supportedDropActions = supportedDropActions;
    data.m_itemFlags = itemFlags;
    data.m_modelMoves = modelMoves;

    QItemSelectionModel selections(&data);
    PublicListView list;
    list.setWindowTitle(QTest::currentTestFunction());
    list.setViewMode(viewMode);
    list.setDragDropMode(dragDropMode);
    list.setDefaultDropAction(defaultDropAction);
    list.setModel(&data);
    list.setSelectionModel(&selections);
    int itemHeight = list.sizeHintForIndex(data.index(1, 0)).height();
    list.resize(300, 15 * itemHeight);
    list.show();
    selections.select(data.index(1, 0), QItemSelectionModel::Select);
    auto getSelectedTexts = [&]() -> QStringList {
        QStringList selectedTexts;
        for (auto index : selections.selectedIndexes())
            selectedTexts << data.itemData(index).value(Qt::DisplayRole).toString();
        return selectedTexts;
    };
    QVERIFY(QTest::qWaitForWindowExposed(&list));
    // execute as soon as the eventloop is running again
    // which is the case inside list.startDrag()
    QTimer::singleShot(0, [&]()
    {
        QPoint droppos;
        // take into account subtle differences between icon and list mode in QListView's drop placement
        if (itemFlags & Qt::ItemIsDropEnabled)
            droppos = list.rectForIndex(data.index(5, 0)).center();
        else if (viewMode == QListView::IconMode)
            droppos = list.rectForIndex(data.index(9, 0)).bottomRight() + QPoint(30, 30);
        else
            droppos = list.rectForIndex(data.index(9, 0)).bottomRight();

        QMouseEvent mouseMove(QEvent::MouseMove, droppos, list.mapToGlobal(droppos), Qt::NoButton, {}, {});
        QCoreApplication::sendEvent(&list, &mouseMove);
        QMouseEvent mouseRelease(QEvent::MouseButtonRelease, droppos, list.mapToGlobal(droppos), Qt::LeftButton, {}, {});
        QCoreApplication::sendEvent(&list, &mouseRelease);
    });

    const QStringList expectedSelected = getSelectedTexts();

    list.startDrag(Qt::MoveAction);

    QCOMPARE(data.stringList(), expectedData);

     // if the model doesn't implement moveRows, or if items are replaced, then selection is lost
    if (modelMoves && !(itemFlags & Qt::ItemIsDropEnabled)) {
        const QStringList actualSelected = getSelectedTexts();
        QCOMPARE(actualSelected, expectedSelected);
    }
}


QTEST_MAIN(tst_QListView)
#include "tst_qlistview.moc"
