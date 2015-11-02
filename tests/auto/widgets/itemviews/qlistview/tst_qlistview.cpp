/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qabstractitemmodel.h>
#include <qapplication.h>
#include <qlistview.h>
#include <private/qlistview_p.h>
#include <private/qcoreapplication_p.h>
#include <qlistwidget.h>
#include <qitemdelegate.h>
#include <qstandarditemmodel.h>
#include <qstringlistmodel.h>
#include <cmath>
#include <math.h>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QDialog>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QStyleFactory>

#if defined(Q_OS_WIN) || defined(Q_OS_WINCE)
#  include <windows.h>
#  include <QtGui/QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#endif // Q_OS_WIN

#if defined(Q_OS_WIN) || defined(Q_OS_WINCE)
static inline HWND getHWNDForWidget(const QWidget *widget)
{
    QWindow *window = widget->windowHandle();
    return static_cast<HWND> (QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", window));
}
#endif // Q_OS_WIN

// Make a widget frameless to prevent size constraints of title bars
// from interfering (Windows).
static inline void setFrameless(QWidget *w)
{
    Qt::WindowFlags flags = w->windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    w->setWindowFlags(flags);
}

class tst_QListView : public QObject
{
    Q_OBJECT

public:
    tst_QListView();
    virtual ~tst_QListView();


public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
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
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && WINVER >= 0x0500
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
};

// Testing get/set functions
void tst_QListView::getSetCheck()
{
    QListView obj1;
    // Movement QListView::movement()
    // void QListView::setMovement(Movement)
    obj1.setMovement(QListView::Movement(QListView::Static));
    QCOMPARE(QListView::Movement(QListView::Static), obj1.movement());
    obj1.setMovement(QListView::Movement(QListView::Free));
    QCOMPARE(QListView::Movement(QListView::Free), obj1.movement());
    obj1.setMovement(QListView::Movement(QListView::Snap));
    QCOMPARE(QListView::Movement(QListView::Snap), obj1.movement());

    // Flow QListView::flow()
    // void QListView::setFlow(Flow)
    obj1.setFlow(QListView::Flow(QListView::LeftToRight));
    QCOMPARE(QListView::Flow(QListView::LeftToRight), obj1.flow());
    obj1.setFlow(QListView::Flow(QListView::TopToBottom));
    QCOMPARE(QListView::Flow(QListView::TopToBottom), obj1.flow());

    // ResizeMode QListView::resizeMode()
    // void QListView::setResizeMode(ResizeMode)
    obj1.setResizeMode(QListView::ResizeMode(QListView::Fixed));
    QCOMPARE(QListView::ResizeMode(QListView::Fixed), obj1.resizeMode());
    obj1.setResizeMode(QListView::ResizeMode(QListView::Adjust));
    QCOMPARE(QListView::ResizeMode(QListView::Adjust), obj1.resizeMode());

    // LayoutMode QListView::layoutMode()
    // void QListView::setLayoutMode(LayoutMode)
    obj1.setLayoutMode(QListView::LayoutMode(QListView::SinglePass));
    QCOMPARE(QListView::LayoutMode(QListView::SinglePass), obj1.layoutMode());
    obj1.setLayoutMode(QListView::LayoutMode(QListView::Batched));
    QCOMPARE(QListView::LayoutMode(QListView::Batched), obj1.layoutMode());

    // int QListView::spacing()
    // void QListView::setSpacing(int)
    obj1.setSpacing(0);
    QCOMPARE(0, obj1.spacing());
    obj1.setSpacing(INT_MIN);
    QCOMPARE(INT_MIN, obj1.spacing());
    obj1.setSpacing(INT_MAX);
    QCOMPARE(INT_MAX, obj1.spacing());

    // ViewMode QListView::viewMode()
    // void QListView::setViewMode(ViewMode)
    obj1.setViewMode(QListView::ViewMode(QListView::ListMode));
    QCOMPARE(QListView::ViewMode(QListView::ListMode), obj1.viewMode());
    obj1.setViewMode(QListView::ViewMode(QListView::IconMode));
    QCOMPARE(QListView::ViewMode(QListView::IconMode), obj1.viewMode());

    // int QListView::modelColumn()
    // void QListView::setModelColumn(int)
    obj1.setModelColumn(0);
    QCOMPARE(0, obj1.modelColumn());
    obj1.setModelColumn(INT_MIN);
    QCOMPARE(0, obj1.modelColumn()); // Less than 0 => 0
    obj1.setModelColumn(INT_MAX);
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
public:
    QtTestModel(QObject *parent = 0): QAbstractListModel(parent),
       colCount(0), rCount(0), wrongIndex(false) {}
    int rowCount(const QModelIndex&) const { return rCount; }
    int columnCount(const QModelIndex&) const { return colCount; }
    bool isEditable(const QModelIndex &) const { return true; }

    QVariant data(const QModelIndex &idx, int role) const
    {

        if (!m_icon.isNull() && role == Qt::DecorationRole) {
            return m_icon;
        }
        if (role != Qt::DisplayRole)
            return QVariant();

        if (idx.row() < 0 || idx.column() < 0 || idx.column() >= colCount
            || idx.row() >= rCount) {
            wrongIndex = true;
            qWarning("got invalid modelIndex %d/%d", idx.row(), idx.column());
        }
        return QString("%1/%2").arg(idx.row()).arg(idx.column());
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

    int colCount, rCount;
    QIcon m_icon;
    mutable bool wrongIndex;
};

tst_QListView::tst_QListView()
{
}

tst_QListView::~tst_QListView()
{
}

void tst_QListView::initTestCase()
{
}

void tst_QListView::cleanupTestCase()
{
}

void tst_QListView::init()
{
#ifdef Q_OS_WINCE //disable magic for WindowsCE
    qApp->setAutoMaximizeThreshold(-1);
#endif
}

void tst_QListView::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}


void tst_QListView::noDelegate()
{
    QtTestModel model(0);
    model.rCount = model.colCount = 10;
    QListView view;
    view.setModel(&model);
    view.setItemDelegate(0);
    view.show();
}

void tst_QListView::noModel()
{
    QListView view;
    view.show();
    view.setRowHidden(0, true);
}

void tst_QListView::emptyModel()
{
    QtTestModel model(0);
    QListView view;
    view.setModel(&model);
    view.show();
    QVERIFY(!model.wrongIndex);
}

void tst_QListView::removeRows()
{
    QtTestModel model(0);
    model.rCount = model.colCount = 10;

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
        view.setModelColumn(j);
        for (int i = 0; i < rows; ++i) {
            QModelIndex index = model.index(i, j);
            model.setData(index, QString("[%1,%2]").arg(i).arg(j));
            view.setCurrentIndex(index);
            QApplication::processEvents();
            QCOMPARE(view.currentIndex(), index);
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

    QVector<Qt::Key> keymoves;
    keymoves << Qt::Key_Up << Qt::Key_Up << Qt::Key_Right << Qt::Key_Right << Qt::Key_Up
             << Qt::Key_Left << Qt::Key_Left << Qt::Key_Up << Qt::Key_Down << Qt::Key_Up
             << Qt::Key_Up << Qt::Key_Up << Qt::Key_Up << Qt::Key_Up << Qt::Key_Up
             << Qt::Key_Left << Qt::Key_Left << Qt::Key_Up << Qt::Key_Down;

    int displayRow    = rows / displayColumns - 1;
    int displayColumn = displayColumns - (rows % displayColumns) - 1;

    QApplication::instance()->processEvents();
    for (int i = 0; i < keymoves.size(); ++i) {
        Qt::Key key = keymoves.at(i);
        QTest::keyClick(&view, key);
        switch (key) {
        case Qt::Key_Up:
            displayRow = qMax(0, displayRow - 1);
            break;
        case Qt::Key_Down:
            displayRow = qMin(rows / displayColumns - 1, displayRow + 1);
            break;
        case Qt::Key_Left:
            displayColumn = qMax(0, displayColumn - 1);
            break;
        case Qt::Key_Right:
            displayColumn = qMin(displayColumns-1, displayColumn + 1);
            break;
        default:
            QVERIFY(false);
        }

        QApplication::instance()->processEvents();

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
    QtTestModel model(0);
    model.rCount = model.colCount = 10;

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

    QStandardItemModel sim(0);
    QStandardItem *root = new QStandardItem("Root row");
    for (int i=0;i<5;i++)
        root->appendRow(new QStandardItem(QString("Row %1").arg(i)));
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
    QtTestModel model(0);
    model.rCount = model.colCount = 10;

    QListView view;
    view.setModel(&model);

    QTest::keyClick(&view, Qt::Key_Down);

    view.setModel(0);
    view.setModel(&model);
    view.setRowHidden(0, true);

    QTest::keyClick(&view, Qt::Key_Down);
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(1, 0));
}

class QMoveCursorListView : public QListView
{
public:
    QMoveCursorListView() : QListView() {}

    // enum CursorAction and moveCursor() are protected in QListView.
    enum CursorAction { MoveUp, MoveDown, MoveLeft, MoveRight,
        MoveHome, MoveEnd, MovePageUp, MovePageDown,
        MoveNext, MovePrevious };

    QModelIndex doMoveCursor(QMoveCursorListView::CursorAction action, Qt::KeyboardModifiers modifiers)
    {
        return QListView::moveCursor((QListView::CursorAction)action, modifiers);
    }
};

void tst_QListView::moveCursor2()
{
    QtTestModel model(0);
    model.colCount = 1;
    model.rCount = 100;
    QPixmap pm(32, 32);
    pm.fill(Qt::green);
    model.setDataIcon(QIcon(pm));

    QMoveCursorListView vu;
    vu.setModel(&model);
    vu.setIconSize(QSize(36,48));
    vu.setGridSize(QSize(34,56));
    //Standard framesize is 1. If Framesize > 2 increase size
    int frameSize = qApp->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    vu.resize(300 + frameSize * 2,300);
    vu.setFlow(QListView::LeftToRight);
    vu.setMovement(QListView::Static);
    vu.setWrapping(true);
    vu.setViewMode(QListView::IconMode);
    vu.setLayoutMode(QListView::Batched);
    vu.show();
    vu.selectionModel()->setCurrentIndex(model.index(0,0), QItemSelectionModel::SelectCurrent);
    QCoreApplication::processEvents();

    QModelIndex idx = vu.doMoveCursor(QMoveCursorListView::MoveHome, Qt::NoModifier);
    QCOMPARE(idx, model.index(0,0));
    idx = vu.doMoveCursor(QMoveCursorListView::MoveDown, Qt::NoModifier);
    QCOMPARE(idx, model.index(8,0));
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
    i1->setSizeHint(QSize(200,32));
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
public:
    QListViewShowEventListener() : QListView() { m_shown = false;}

    virtual void showEvent(QShowEvent * /*e*/)
    {
        int columnwidth = sizeHintForColumn(0);
        QSize sz = sizeHintForIndex(model()->index(0,0));

        // This should retrieve a model index in the 2nd section
        m_index = indexAt(QPoint(columnwidth +2, sz.height()/2));
        m_shown = true;
    }

    QModelIndex m_index;
    bool m_shown;

};

void tst_QListView::indexAt()
{
    QtTestModel model(0);
    model.rCount = 2;
    model.colCount = 1;

    QListView view;
    view.setModel(&model);
    view.setViewMode(QListView::ListMode);
    view.setFlow(QListView::TopToBottom);

    QSize sz = view.sizeHintForIndex(model.index(0,0));
    QModelIndex index;
    index = view.indexAt(QPoint(20,0));
    QVERIFY(index.isValid());
    QCOMPARE(index.row(), 0);

    index = view.indexAt(QPoint(20,sz.height()));
    QVERIFY(index.isValid());
    QCOMPARE(index.row(), 1);

    index = view.indexAt(QPoint(20,2 * sz.height()));
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
    view2.resize(300,100);
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
    QtTestModel model;
    model.rCount = 10;
    model.colCount = 2;

    QListView view;
    view.setModel(&model);

    view.show();
    QApplication::processEvents();

    QModelIndex firstIndex = model.index(0, 0, QModelIndex());
    QVERIFY(firstIndex.isValid());
    int itemHeight = view.visualRect(firstIndex).height();
    view.resize(200, itemHeight * (model.rCount + 1));

    for (int i = 0; i < model.rCount; ++i) {
        QPoint p(5, 1 + itemHeight * i);
        QModelIndex index = view.indexAt(p);
        if (!index.isValid())
            continue;
        QSignalSpy spy(&view, SIGNAL(clicked(QModelIndex)));
        QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
        QCOMPARE(spy.count(), 1);
    }
}

void tst_QListView::singleSelectionRemoveRow()
{
    QStringList items;
    items << "item1" << "item2" << "item3" << "item4";
    QStringListModel model(items);

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
    for (int r = 0; r < numRows; ++r)
        for (int c = 0; c < numCols; ++c)
            model.setData(model.index(r, c), QString("%1,%2").arg(r).arg(c));

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
    for (int r = 0; r < numRows; ++r)
        for (int c = 0; c < numCols; ++c)
            model.setData(model.index(r, c), QString("%1,%2").arg(r).arg(c));


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
    view.setModelColumn(INT_MAX);
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
    QStringList items;
    for (int i=0; i <100; ++i)
        items << "item";
    QStringListModel model(items);

    QListView view;
    view.setModel(&model);
    view.setUniformItemSizes(true);
    view.setRowHidden(0,true);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(10);
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

    QStringList items;
    for (int i = 0; i < rowCount; ++i)
        items << QLatin1String("item ") + QString::number(i);
    QStringListModel model(items);

    QListView view;
    view.setWindowTitle(QTest::currentTestFunction());
    view.setModel(&model);
    view.setUniformItemSizes(true);
    view.setViewMode(QListView::ListMode);
    view.setLayoutMode(QListView::Batched);
    view.setBatchSize(2);
    view.resize(200,400);
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
    QStringList items;
    int i;
    for (i=0; i <20; ++i)
        items << QString("item %1").arg(i);
    QStringListModel model(items);

    QListView view;
    view.setModel(&model);

    view.resize(220,182);
    view.show();

    for (int pass = 0; pass < 2; ++pass) {
        view.setFlow(pass == 0 ? QListView::TopToBottom : QListView::LeftToRight);
        QScrollBar *sb = pass == 0 ? view.verticalScrollBar() : view.horizontalScrollBar();
        QList<QSize> gridsizes;
        gridsizes << QSize() << QSize(200,38);
        for (int ig = 0; ig < gridsizes.count(); ++ig) {
            if (pass == 1 && !gridsizes.at(ig).isValid()) // the width of an item varies, so it might jump two times
                continue;
            view.setGridSize(gridsizes.at(ig));

            qApp->processEvents();
            int offset = sb->value();

            // first "scroll" down, verify that we scroll one step at a time
            i = 0;
            for (i = 0; i < 20; ++i) {
                QModelIndex idx = model.index(i,0);
                view.setCurrentIndex(idx);
                if (offset != sb->value()) {
                    // If it has scrolled, it should have scrolled only by one.
                    QCOMPARE(sb->value(), offset + 1);
                    ++offset;
                }
                //QTest::qWait(50);
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
                //QTest::qWait(50);
            }
        }
    }
}

class PublicListView : public QListView
{
    public:
    PublicListView(QWidget *parent = 0) : QListView(parent)
    {

    }
    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags) {
        QListView::setSelection(rect, flags);
    }
    QSize contentsSize() const { return QListView::contentsSize(); }

    void setPositionForIndex(const QPoint &pos, const QModelIndex &index) {
        QListView::setPositionForIndex(pos, index);
    }

    QStyleOptionViewItem viewOptions() const {
      return QListView::viewOptions();
    }
};

class TestDelegate : public QItemDelegate
{
public:
    TestDelegate(QObject *parent) : QItemDelegate(parent), m_sizeHint(50,50) {}
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const { return m_sizeHint; }

    QSize m_sizeHint;
};

typedef QList<int> IntList;

void tst_QListView::selection_data()
{
    QTest::addColumn<int>("itemCount");
    QTest::addColumn<int>("viewMode");
    QTest::addColumn<int>("flow");
    QTest::addColumn<bool>("wrapping");
    QTest::addColumn<int>("spacing");
    QTest::addColumn<QSize>("gridSize");
    QTest::addColumn<IntList>("hiddenRows");
    QTest::addColumn<QRect>("selectionRect");
    QTest::addColumn<IntList>("expectedItems");

    QTest::newRow("select all")
        << 4                                    // itemCount
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
        << false                                // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(0, 0, 10, 200)                 // selection rectangle
        << (IntList() << 0 << 1 << 2 << 3);     // expected items

    QTest::newRow("select below, (on viewport)")
        << 4                                    // itemCount
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
        << false                                // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(10, 250, 1, 1)                 // selection rectangle
        << IntList();                           // expected items

    QTest::newRow("select below 2, (on viewport)")
        << 4                                    // itemCount
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(10, 250, 1, 1)                 // selection rectangle
        << IntList();                           // expected items

    QTest::newRow("select to the right, (on viewport)")
        << 40                                   // itemCount
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(300, 10, 1, 1)                 // selection rectangle
        << IntList();                           // expected items

    QTest::newRow("select to the right 2, (on viewport)")
        << 40                                   // itemCount
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(300, 0, 1, 300)                // selection rectangle
        << IntList();                           // expected items

#if defined(Q_OS_WINCE)
    // depending on whether the display is double-pixeld, we need
    // to click at a different position
    bool doubledSize = false;
    int dpi = GetDeviceCaps(GetDC(0), LOGPIXELSX);
    if ((dpi < 1000) && (dpi > 0)) {
        doubledSize = true;
    }
    QTest::newRow("select inside contents, (on viewport)")
        << 35                                   // itemCount
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(doubledSize?350:175,doubledSize?550:275, 1, 1)// selection rectangle
        << IntList();                           // expected items
#else
    QTest::newRow("select inside contents, (on viewport)")
        << 35                                   // itemCount
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(175, 275, 1, 1)                // selection rectangle
        << IntList();                           // expected items
#endif

    QTest::newRow("select a tall rect in LeftToRight flow, wrap items")
        << 70                                   // itemCount
        << int(QListView::ListMode)
        << int(QListView::LeftToRight)
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
        << int(QListView::ListMode)
        << int(QListView::LeftToRight)
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(90, 90, 200, 1)                // selection rectangle
        << (IntList()                           // expected items
                      << 11 << 12 << 13 << 14 << 15);

    QTest::newRow("select a wide negative rect in LeftToRight flow, wrap items")
        << 70                                   // itemCount
        << int(QListView::ListMode)
        << int(QListView::LeftToRight)
        << true                                 // wrapping
        << 0                                    // spacing
        << QSize()                              // gridSize
        << IntList()                            // hiddenRows
        << QRect(290, 90, -200, 1)              // selection rectangle
        << (IntList()                           // expected items
                      << 11 << 12 << 13 << 14 << 15);

    QTest::newRow("select a tall rect in TopToBottom flow, wrap items")
        << 70                                   // itemCount
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
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
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
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
        << int(QListView::ListMode)
        << int(QListView::TopToBottom)
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
    QFETCH(int, viewMode);
    QFETCH(int, flow);
    QFETCH(bool, wrapping);
    QFETCH(int, spacing);
    QFETCH(QSize, gridSize);
    QFETCH(IntList, hiddenRows);
    QFETCH(QRect, selectionRect);
    QFETCH(IntList, expectedItems);

    QWidget topLevel;
    PublicListView v(&topLevel);
    QtTestModel model;
    model.colCount = 1;
    model.rCount = itemCount;

    // avoid scrollbar size mismatches among different styles
    v.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    v.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    v.setItemDelegate(new TestDelegate(&v));
    v.setModel(&model);
    v.setViewMode(QListView::ViewMode(viewMode));
    v.setFlow(QListView::Flow(flow));
    v.setWrapping(wrapping);
    v.setResizeMode(QListView::Adjust);
    v.setSpacing(spacing);
    if (gridSize.isValid())
        v.setGridSize(gridSize);
    for (int j = 0; j < hiddenRows.count(); ++j) {
        v.setRowHidden(hiddenRows.at(j), true);
    }

#if defined(Q_OS_WINCE)
    // If the device is double-pixeled then the scrollbars become
    // 10 pixels wider than normal (Windows Style: 16, Windows Mobile Style: 26).
    // So we have to make the window slightly bigger to have the same count of
    // items in each row of the list view like in the other styles.
    static const int dpi = ::GetDeviceCaps(GetDC(0), LOGPIXELSX);
    if ((dpi < 1000) && (dpi > 0))
        v.resize(535,535);
#else
    v.resize(525,525);
#endif

    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    v.setSelection(selectionRect, QItemSelectionModel::ClearAndSelect);

    QModelIndexList selected = v.selectionModel()->selectedIndexes();

    QCOMPARE(selected.count(), expectedItems.count());
    for (int i = 0; i < selected.count(); ++i) {
        QVERIFY(expectedItems.contains(selected.at(i).row()));
    }
}

void tst_QListView::scrollTo()
{
    QWidget topLevel;
    setFrameless(&topLevel);
    QListView lv(&topLevel);
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
    QModelIndex index = model.index(6,0);

    //we save the size of the item for later comparisons
    const QSize itemsize = lv.visualRect(index).size();
    QVERIFY(itemsize.height() > lv.height());
    QVERIFY(itemsize.width() > lv.width());

    //we click the item
    QPoint p = lv.visualRect(index).center();
    QTest::mouseClick(lv.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    //let's wait because the scrolling is delayed
    QTest::qWait(QApplication::doubleClickInterval() + 150);
    QTRY_COMPARE(lv.visualRect(index).y(),0);

    //we scroll down. As the item is to tall for the view, it will disappear
    QTest::keyClick(lv.viewport(), Qt::Key_Down, Qt::NoModifier);
    QCOMPARE(lv.visualRect(index).y(), -itemsize.height());

    QTest::keyClick(lv.viewport(), Qt::Key_Up, Qt::NoModifier);
    QCOMPARE(lv.visualRect(index).y(), 0);

    //Let's enable wrapping

    lv.setWrapping(true);
    lv.horizontalScrollBar()->setValue(0); //let's scroll to the beginning

    //we click the item
    p = lv.visualRect(index).center();
    QTest::mouseClick(lv.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    //let's wait because the scrolling is delayed
    QTest::qWait(QApplication::doubleClickInterval() + 150);
    QTRY_COMPARE(lv.visualRect(index).x(),0);

    //we scroll right. As the item is too wide for the view, it will disappear
    QTest::keyClick(lv.viewport(), Qt::Key_Right, Qt::NoModifier);
    QCOMPARE(lv.visualRect(index).x(), -itemsize.width());

    QTest::keyClick(lv.viewport(), Qt::Key_Left, Qt::NoModifier);
    QCOMPARE(lv.visualRect(index).x(), 0);

    lv.setWrapping(false);
    qApp->processEvents(); //let the layout happen

    //Let's try with scrolling per pixel
    lv.setHorizontalScrollMode( QListView::ScrollPerPixel);
    lv.verticalScrollBar()->setValue(0); //scrolls back to the first item

    //we click the item
    p = lv.visualRect(index).center();
    QTest::mouseClick(lv.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    //let's wait because the scrolling is delayed
    QTest::qWait(QApplication::doubleClickInterval() + 150);
    QTRY_COMPARE(lv.visualRect(index).y(),0);

    //we scroll down. As the item is too tall for the view, it will partially disappear
    QTest::keyClick(lv.viewport(), Qt::Key_Down, Qt::NoModifier);
    QVERIFY(lv.visualRect(index).y()<0);

    QTest::keyClick(lv.viewport(), Qt::Key_Up, Qt::NoModifier);
    QCOMPARE(lv.visualRect(index).y(), 0);
}


void tst_QListView::scrollBarRanges()
{
    const int rowCount = 10;
    const int rowHeight = 20;

    QWidget topLevel;
    QListView lv(&topLevel);
    QStringListModel model(&lv);
    QStringList list;
    for (int i = 0; i < rowCount; ++i)
        list << QString::fromLatin1("Item %1").arg(i);

    model.setStringList(list);
    lv.setModel(&model);
    lv.resize(250, 130);
    TestDelegate *delegate = new TestDelegate(&lv);
    delegate->m_sizeHint = QSize(100, rowHeight);
    lv.setItemDelegate(delegate);
    topLevel.show();

    for (int h = 30; h <= 210; ++h) {
        lv.resize(250, h);
        QApplication::processEvents(); // wait for the layout to be done
        int visibleRowCount = lv.viewport()->size().height() / rowHeight;
        int invisibleRowCount = rowCount - visibleRowCount;
        QCOMPARE(lv.verticalScrollBar()->maximum(), invisibleRowCount);
    }
}

void tst_QListView::scrollBarAsNeeded_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("itemCount");
    QTest::addColumn<int>("flow");
    QTest::addColumn<bool>("horizontalScrollBarVisible");
    QTest::addColumn<bool>("verticalScrollBarVisible");


    QTest::newRow("TopToBottom, count:0")
            << QSize(200, 100)
            << 0
            << int(QListView::TopToBottom)
            << false
            << false;

    QTest::newRow("TopToBottom, count:1")
            << QSize(200, 100)
            << 1
            << int(QListView::TopToBottom)
            << false
            << false;

    QTest::newRow("TopToBottom, count:20")
            << QSize(200, 100)
            << 20
            << int(QListView::TopToBottom)
            << false
            << true;

    QTest::newRow("LeftToRight, count:0")
            << QSize(200, 100)
            << 0
            << int(QListView::LeftToRight)
            << false
            << false;

    QTest::newRow("LeftToRight, count:1")
            << QSize(200, 100)
            << 1
            << int(QListView::LeftToRight)
            << false
            << false;

    QTest::newRow("LeftToRight, count:20")
            << QSize(200, 100)
            << 20
            << int(QListView::LeftToRight)
            << true
            << false;


}
void tst_QListView::scrollBarAsNeeded()
{

    QFETCH(QSize, size);
    QFETCH(int, itemCount);
    QFETCH(int, flow);
    QFETCH(bool, horizontalScrollBarVisible);
    QFETCH(bool, verticalScrollBarVisible);


    const int rowCounts[3] = {0, 1, 20};

    QWidget topLevel;
    QListView lv(&topLevel);
    lv.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    lv.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    lv.setFlow((QListView::Flow)flow);
    QStringListModel model(&lv);
    lv.setModel(&model);
    lv.resize(size);
    topLevel.show();

    for (uint r = 0; r < sizeof(rowCounts)/sizeof(int); ++r) {
        QStringList list;
        int i;
        for (i = 0; i < rowCounts[r]; ++i)
            list << QString::fromLatin1("Item %1").arg(i);

        model.setStringList(list);
        QApplication::processEvents();
        QTest::qWait(50);

        QStringList replacement;
        for (i = 0; i < itemCount; ++i) {
            replacement << QString::fromLatin1("Item %1").arg(i);
        }
        model.setStringList(replacement);

        QApplication::processEvents();

        QTRY_COMPARE(lv.horizontalScrollBar()->isVisible(), horizontalScrollBarVisible);
        QTRY_COMPARE(lv.verticalScrollBar()->isVisible(), verticalScrollBarVisible);
    }
}

void tst_QListView::moveItems()
{
    QStandardItemModel model;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            QStandardItem* item = new QStandardItem(QString("standard item (%1,%2)").arg(r).arg(c));
            model.setItem(r, c, item);
        }
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

#if defined Q_OS_BLACKBERRY
    QFont font = lv.font();
    // On BB10 the root window is stretched over the whole screen
    // This makes sure that the text will be long enough to produce
    // a vertical scrollbar
    font.setPixelSize(50);
    lv.setFont(font);
#endif
    lv.showNormal();
    QApplication::processEvents();

    QTRY_COMPARE(lv.horizontalScrollBar()->isVisible(), false);
    QTRY_COMPARE(lv.verticalScrollBar()->isVisible(), true);
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
class SetCurrentIndexAfterAppendRowCrashDialog : public QDialog
{
    Q_OBJECT
public:
    SetCurrentIndexAfterAppendRowCrashDialog()
    {
#if WINVER >= 0x0500
        listView = new QListView();
        listView->setViewMode(QListView::IconMode);

        model = new QStandardItemModel(this);
        listView->setModel(model);

        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(buttonClicked()));
        timer->start(1000);

        DWORD lParam = 0xFFFFFFFC/*OBJID_CLIENT*/;
        DWORD wParam = 0;
        if (const HWND hwnd =getHWNDForWidget(this))
            SendMessage(hwnd, WM_GETOBJECT, wParam, lParam);
#endif
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
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT) && WINVER >= 0x0500
// This test only makes sense on windows 2000 and higher.
void tst_QListView::setCurrentIndexAfterAppendRowCrash()
{
    SetCurrentIndexAfterAppendRowCrashDialog w;
    w.exec();
}
#endif

void tst_QListView::emptyItemSize()
{
    QStandardItemModel model;
    for (int r = 0; r < 4; ++r) {
        QStandardItem* item = new QStandardItem(QString("standard item (%1)").arg(r));
        model.setItem(r, 0, item);
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
    view.setModel(new QStringListModel( QStringList() << "foo"));
    view.setRowHidden(0, true);
    view.selectAll();
    QVERIFY(view.selectionModel()->selectedIndexes().isEmpty());
    view.setRowHidden(0, false);
    view.selectAll();
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
}

void tst_QListView::task228566_infiniteRelayout()
{
    QListView view;

    QStringList list;
    for (int i = 0; i < 10; ++i) {
        list << "small";
    }

    list << "BIGBIGBIGBIGBIGBIGBIGBIGBIGBIGBIGBIG";
    list << "BIGBIGBIGBIGBIGBIGBIGBIGBIGBIGBIGBIG";

    QStringListModel model(list);
    view.setModel(&model);
    view.setWrapping(true);
    view.setResizeMode(QListView::Adjust);

    const int itemHeight = view.visualRect( model.index(0, 0)).height();

    view.setFixedHeight(itemHeight * 12);
    view.show();
    QTest::qWait(100); //make sure the layout is done once

    QSignalSpy spy(view.horizontalScrollBar(), SIGNAL(rangeChanged(int,int)));

    QTest::qWait(200);
    //the layout should already have been done
    //so there should be no change made to the scrollbar
    QCOMPARE(spy.count(), 0);
}

void tst_QListView::task248430_crashWith0SizedItem()
{
    QListView view;
    view.setViewMode(QListView::IconMode);
    QStringListModel model(QStringList() << QLatin1String("item1") << QString());
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
    QCOMPARE(view.verticalScrollBar()->value(), scrollValue);
    QCOMPARE(view.currentIndex(), index);

    view.showMinimized();
    QTest::qWait(50);
    QTRY_COMPARE(view.verticalScrollBar()->value(), scrollValue);
    QTRY_COMPARE(view.currentIndex(), index);

    view.showNormal();
    QTest::qWait(50);
    QTRY_COMPARE(view.verticalScrollBar()->value(), scrollValue);
    QTRY_COMPARE(view.currentIndex(), index);
}

void tst_QListView::task196118_visualRegionForSelection()
{
    class MyListView : public QListView
    {
    public:
        QRegion getVisualRegionForSelection() const
        { return QListView::visualRegionForSelection( selectionModel()->selection()); }
    } view;

    QStandardItemModel model;
    QStandardItem top1("top1");
    QStandardItem sub1("sub1");
    top1.appendRow(QList<QStandardItem*>() << &sub1);
    model.appendColumn(QList<QStandardItem*>() << &top1);
    view.setModel(&model);
    view.setRootIndex(top1.index());

    view.selectionModel()->select(top1.index(), QItemSelectionModel::Select);

    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
    QVERIFY(view.getVisualRegionForSelection().isEmpty());
}

void tst_QListView::task254449_draggingItemToNegativeCoordinates()
{
    //we'll check that the items are painted correctly
    class MyListView : public QListView
    {
    public:
        void setPositionForIndex(const QPoint &position, const QModelIndex &index)
        { QListView::setPositionForIndex(position, index); }

    } list;

    QStandardItemModel model(1,1);
    QModelIndex index = model.index(0,0);
    model.setData(index, QLatin1String("foo"));
    list.setModel(&model);
    list.setViewMode(QListView::IconMode);
    list.show();
    list.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&list));


    class MyItemDelegate : public QStyledItemDelegate
    {
    public:
        MyItemDelegate() : numPaints(0) { }
        void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const
        {
            numPaints++;
            QStyledItemDelegate::paint(painter, option, index);
        }

        mutable int numPaints;
    } delegate;
    delegate.numPaints = 0;
    list.setItemDelegate(&delegate);
    QApplication::processEvents();
    QTRY_VERIFY(delegate.numPaints > 0);  //makes sure the layout is done

    const QPoint topLeft(-6, 0);
    list.setPositionForIndex(topLeft, index);

    //we'll make sure the item is repainted
    delegate.numPaints = 0;
    QApplication::processEvents();
    QTRY_COMPARE(delegate.numPaints, 1);
    QCOMPARE(list.visualRect(index).topLeft(), topLeft);
}


void tst_QListView::keyboardSearch()
{
    QStringList items;
    items << "AB" << "AC" << "BA" << "BB" << "BD" << "KAFEINE" << "KONQUEROR" << "KOPETE" << "KOOKA" << "OKULAR";
    QStringListModel model(items);

    QListView view;
    view.setModel(&model);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

//    QCOMPARE(view.currentIndex() , model.index(0,0));

    QTest::keyClick(&view, Qt::Key_K);
    QTest::qWait(10);
    QCOMPARE(view.currentIndex() , model.index(5,0)); //KAFEINE

    QTest::keyClick(&view, Qt::Key_O);
    QTest::qWait(10);
    QCOMPARE(view.currentIndex() , model.index(6,0)); //KONQUEROR

    QTest::keyClick(&view, Qt::Key_N);
    QTest::qWait(10);
    QCOMPARE(view.currentIndex() , model.index(6,0)); //KONQUEROR
}

void tst_QListView::shiftSelectionWithNonUniformItemSizes()
{
    // This checks that no items are selected unexpectedly by Shift-Arrow
    // when items with non-uniform sizes are laid out in a grid
    {   // First test: QListView::LeftToRight flow
        QStringList items;
        items << "Long\nText" << "Text" << "Text" << "Text";
        QStringListModel model(items);

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
        QTest::qWait(10);
        QCOMPARE(view.currentIndex(), model.index(1, 0));

        QModelIndexList selected = view.selectionModel()->selectedIndexes();
        QCOMPARE(selected.count(), 3);
        QVERIFY(!selected.contains(model.index(0, 0)));
    }
    {   // Second test: QListView::TopToBottom flow
        QStringList items;
        items << "ab" << "a" << "a" << "a";
        QStringListModel model(items);

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
        QTest::qWait(10);
        QCOMPARE(view.currentIndex(), model.index(1, 0));

        QModelIndexList selected = view.selectionModel()->selectedIndexes();
        QCOMPARE(selected.count(), 3);
        QVERIFY(!selected.contains(model.index(0, 0)));
    }
}

void tst_QListView::clickOnViewportClearsSelection()
{
    QStringList items;
    items << "Text1";
    QStringListModel model(items);
    QListView view;
    view.setModel(&model);
    view.setSelectionMode(QListView::ExtendedSelection);

    view.selectAll();
    QModelIndex index = model.index(0);
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
    QVERIFY(view.selectionModel()->isSelected(index));

    //we try to click outside of the index
    const QPoint point = view.visualRect(index).bottomRight() + QPoint(10,10);

    QTest::mousePress(view.viewport(), Qt::LeftButton, 0, point);
    //at this point, the selection shouldn't have changed
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
    QVERIFY(view.selectionModel()->isSelected(index));

    QTest::mouseRelease(view.viewport(), Qt::LeftButton, 0, point);
    //now the selection should be cleared
    QVERIFY(!view.selectionModel()->hasSelection());
}

void tst_QListView::task262152_setModelColumnNavigate()
{
    QListView view;
    QStandardItemModel model(3,2);
    model.setItem(0,1,new QStandardItem("[0,1]"));
    model.setItem(1,1,new QStandardItem("[1,1]"));
    model.setItem(2,1,new QStandardItem("[2,1]"));

    view.setModel(&model);
    view.setModelColumn(1);

    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(static_cast<QWidget *>(&view), QApplication::activeWindow());
    QTest::keyClick(&view, Qt::Key_Down);
    QTest::qWait(30);
    QTRY_COMPARE(view.currentIndex(), model.index(1,1));
    QTest::keyClick(&view, Qt::Key_Down);
    QTest::qWait(30);
    QTRY_COMPARE(view.currentIndex(), model.index(2,1));
}

void tst_QListView::taskQTBUG_2233_scrollHiddenItems_data()
{
    QTest::addColumn<int>("flow");

    QTest::newRow("TopToBottom") << static_cast<int>(QListView::TopToBottom);
    QTest::newRow("LeftToRight") << static_cast<int>(QListView::LeftToRight);
}

void tst_QListView::taskQTBUG_2233_scrollHiddenItems()
{
    QFETCH(int, flow);
    const int rowCount = 200;

    QWidget topLevel;
    setFrameless(&topLevel);
    QListView view(&topLevel);
    QStringListModel model(&view);
    QStringList list;
    for (int i = 0; i < rowCount; ++i)
        list << QString::number(i);

    model.setStringList(list);
    view.setModel(&model);
    view.setUniformItemSizes(true);
    view.setViewMode(QListView::ListMode);
    for (int i = 0; i < rowCount / 2; ++i)
        view.setRowHidden(2 * i, true);
    view.setFlow(static_cast<QListView::Flow>(flow));
    view.resize(130, 130);

    for (int i = 0; i < 10; ++i) {
        (view.flow() == QListView::TopToBottom
            ? view.verticalScrollBar()
            : view.horizontalScrollBar())->setValue(i);
        QModelIndex index = view.indexAt(QPoint(0,0));
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
    QApplication::processEvents();
    for (int i = rowCount; i > rowCount / 2; i--) {
        view.setRowHidden(i, true);
    }
    QApplication::processEvents();
    QTest::qWait(50);
    QCOMPARE(bar->value(), bar->maximum());
    QCOMPARE(bar->maximum(), rowCount/4 - nbVisibleItem);
}

void tst_QListView::taskQTBUG_633_changeModelData()
{
    QListView view;
    view.setFlow(QListView::LeftToRight);
    QStandardItemModel model(5,1);
    for (int i = 0; i < model.rowCount(); ++i) {
        model.setData( model.index(i, 0), QString::number(i));
    }

    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    model.setData( model.index(1, 0), QLatin1String("long long text"));
    QTest::qWait(100); //leave time for relayouting the items
    QRect rectLongText = view.visualRect(model.index(1,0));
    QRect rect2 = view.visualRect(model.index(2,0));
    QVERIFY( ! rectLongText.intersects(rect2) );
}

void tst_QListView::taskQTBUG_435_deselectOnViewportClick()
{
    QListView view;
    QStringListModel model( QStringList() << "1" << "2" << "3" << "4");
    view.setModel(&model);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.selectAll();
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), model.rowCount());


    const QRect itemRect = view.visualRect(model.index(model.rowCount() - 1));
    QPoint p = view.visualRect(model.index(model.rowCount() - 1)).center() + QPoint(0, itemRect.height());
    //first the left button
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, p);
    QVERIFY(!view.selectionModel()->hasSelection());

    view.selectAll();
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), model.rowCount());

    //and now the right button
    QTest::mouseClick(view.viewport(), Qt::RightButton, 0, p);
    QVERIFY(!view.selectionModel()->hasSelection());
}

void tst_QListView::taskQTBUG_2678_spacingAndWrappedText()
{
    static const QString lorem("Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    QStringListModel model(QStringList() << lorem << lorem << "foo" << lorem << "bar" << lorem << lorem);
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
    QList<int> currentItemIndexes;
    QtTestModel model(0);
    model.colCount = 1;
    model.rCount = 100;

    currentItemIndexes << 0 << 6 << 16 << 25 << 34 << 42 << 57 << 68 << 77
                       << 83 << 91 << 94;
    QMoveCursorListView vu;
    vu.setModel(&model);
    vu.show();

    QVERIFY(QTest::qWaitForWindowExposed(&vu));

    int itemHeight = vu.visualRect(model.index(0, 0)).height();
    int visibleRowCount = vu.viewport()->height() / itemHeight;
    int scrolledRowCount = visibleRowCount - 1;

    for (int i = 0; i < currentItemIndexes.size(); ++i) {
        vu.selectionModel()->setCurrentIndex(model.index(currentItemIndexes[i], 0),
                                             QItemSelectionModel::SelectCurrent);

        QModelIndex idx = vu.doMoveCursor(QMoveCursorListView::MovePageDown, Qt::NoModifier);
        int newCurrent = qMin(currentItemIndexes[i] + scrolledRowCount, 99);
        QCOMPARE(idx, model.index(newCurrent, 0));

        idx = vu.doMoveCursor(QMoveCursorListView::MovePageUp, Qt::NoModifier);
        newCurrent = qMax(currentItemIndexes[i] - scrolledRowCount, 0);
        QCOMPARE(idx, model.index(newCurrent, 0));
    }
}

class ListView_9455 : public QListView
{
public:
    QSize contentsSize() const
    {
        return QListView::contentsSize();
    }
};

void tst_QListView::taskQTBUG_9455_wrongScrollbarRanges()
{
    QStringList list;
    const int nrItems = 8;
    for (int i = 0; i < nrItems; i++)
        list << QString::asprintf("item %d", i);

    QStringListModel model(list);
    ListView_9455 w;
    setFrameless(&w);
    w.setModel(&model);
    w.setViewMode(QListView::IconMode);
    w.resize(116, 132);
    w.setMovement(QListView::Static);
    const int spacing = 200;
    w.setSpacing(spacing);
    w.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QCOMPARE(w.verticalScrollBar()->maximum(), w.contentsSize().height() - w.viewport()->geometry().height());
}

void tst_QListView::styleOptionViewItem()
{
    class MyDelegate : public QStyledItemDelegate
    {
        public:
            void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
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
    model.appendRow(QList<QStandardItem*>()
        << new QStandardItem("Beginning") <<  new QStandardItem("Middle") << new QStandardItem("Middle") << new QStandardItem("End") );

    // Run test
    view.showMaximized();
    QApplication::processEvents();
}

void tst_QListView::taskQTBUG_12308_artihmeticException()
{
    QListWidget lw;
    lw.setLayoutMode(QListView::Batched);
    lw.setViewMode(QListView::IconMode);
    for (int i = 0; i < lw.batchSize() + 1; i++) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(QString("Item %L1").arg(i));
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
    Delegate12308(QObject *parent = 0) : QStyledItemDelegate(parent)
    { }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
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
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(QString("Item %L1").arg(i));
        lw.addItem(item);
        if (!item->text().contains(QString::fromLatin1("1")))
            item->setHidden(true);
    }
    lw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&lw));
}

void tst_QListView::taskQTBUG_21115_scrollToAndHiddenItems_data()
{
    QTest::addColumn<int>("flow");
    QTest::newRow("flow TopToBottom") << static_cast<int>(QListView::TopToBottom);
    QTest::newRow("flow LeftToRight") << static_cast<int>(QListView::LeftToRight);
}

void tst_QListView::taskQTBUG_21115_scrollToAndHiddenItems()
{
#if defined Q_OS_BLACKBERRY
    // On BB10 we need to create a root window which is automatically stretched
    // over the whole screen
    QWindow rootWindow;
    rootWindow.show();
#endif
    QFETCH(int, flow);

    QListView lv;
    lv.setUniformItemSizes(true);
    lv.setFlow(static_cast<QListView::Flow>(flow));

    QStringListModel model;
    QStringList list;
    for (int i = 0; i < 30; i++)
        list << QString::number(i);
    model.setStringList(list);
    lv.setModel(&model);
    lv.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&lv));

    // Save first item rect for reference
    QRect firstItemRect = lv.visualRect(model.index(0, 0));

    // Select an item and scroll to selection
    QModelIndex index = model.index(2, 0);
    lv.setCurrentIndex(index);
    lv.scrollTo(index, QAbstractItemView::PositionAtTop);
    QApplication::processEvents();
    QCOMPARE(lv.visualRect(index), firstItemRect);

    // Hide some rows and scroll to selection
    for (int i = 0; i < 5; i++) {
        if (i == index.row())
            continue;
        lv.setRowHidden(i, true);
    }
    lv.scrollTo(index, QAbstractItemView::PositionAtTop);
    QApplication::processEvents();
    QCOMPARE(lv.visualRect(index), firstItemRect);
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
    QStringList list;
    for (int i = 0; i < 30; i++)
        list << QString::number(i);
    model.setStringList(list);
    view.setModel(&model);

    view.show();
    QTest::qWaitForWindowExposed(&view);

    QModelIndex expectedIndex = model.index(row, 0);
    QListViewPrivate *privateClass = static_cast<QListViewPrivate *>(QListViewPrivate::get(&view));
    QRect rect;
    QModelIndexList indexList;
    indexList << expectedIndex;
    view.scrollTo(expectedIndex);
    QItemViewPaintPairs pairs = privateClass->draggablePaintPairs(indexList, &rect);
    QCOMPARE(indexList.size(), pairs.size());
    foreach (const QItemViewPaintPair pair, pairs) {
        QCOMPARE(rect, pair.first);
        QCOMPARE(expectedIndex, pair.second);
    }
}

void tst_QListView::taskQTBUG_21804_hiddenItemsAndScrollingWithKeys_data()
{
    QTest::addColumn<int>("flow");
    QTest::addColumn<int>("spacing");
    QTest::newRow("flow TopToBottom no spacing") << static_cast<int>(QListView::TopToBottom) << 0;
    QTest::newRow("flow TopToBottom with spacing") << static_cast<int>(QListView::TopToBottom) << 5;
    QTest::newRow("flow LeftToRight no spacing") << static_cast<int>(QListView::LeftToRight) << 0;
    QTest::newRow("flow LeftToRight with spacing") << static_cast<int>(QListView::LeftToRight) << 5;
}

void tst_QListView::taskQTBUG_21804_hiddenItemsAndScrollingWithKeys()
{
    QFETCH(int, flow);
    QFETCH(int, spacing);

    // create some items to show
    QStringListModel model;
    QStringList list;
    for (int i = 0; i < 60; i++)
        list << QString::number(i);
    model.setStringList(list);

    // create listview
    QListView lv;
    lv.setFlow(static_cast<QListView::Flow>(flow));
    lv.setSpacing(spacing);
    lv.setModel(&model);
    lv.show();
    QTest::qWaitForWindowExposed(&lv);

    // hide every odd number row
    for (int i = 1; i < model.rowCount(); i+=2)
        lv.setRowHidden(i, true);

    // scroll forward and check that selected item is visible always
    int visibleItemCount = model.rowCount()/2;
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
    for (int i = 0; i < visibleItemCount/2; i++) {
        if (flow == QListView::TopToBottom)
            QTest::keyClick(&lv, Qt::Key_Down);
        else
            QTest::keyClick(&lv, Qt::Key_Right);
        QTRY_VERIFY(lv.rect().contains(lv.visualRect(lv.currentIndex())));
    }

    // scroll backward again
    for (int i = 0; i < visibleItemCount/2; i++) {
        if (flow == QListView::TopToBottom)
            QTest::keyClick(&lv, Qt::Key_Up);
        else
            QTest::keyClick(&lv, Qt::Key_Left);
        QTRY_VERIFY(lv.rect().contains(lv.visualRect(lv.currentIndex())));
    }
}

void tst_QListView::spacing_data()
{
    QTest::addColumn<int>("flow");
    QTest::addColumn<int>("spacing");
    QTest::newRow("flow=TopToBottom spacing=0") << static_cast<int>(QListView::TopToBottom) << 0;
    QTest::newRow("flow=TopToBottom spacing=10") << static_cast<int>(QListView::TopToBottom) << 10;
    QTest::newRow("flow=LeftToRight spacing=0") << static_cast<int>(QListView::LeftToRight) << 0;
    QTest::newRow("flow=LeftToRight spacing=10") << static_cast<int>(QListView::LeftToRight) << 10;
}

void tst_QListView::spacing()
{
    QFETCH(int, flow);
    QFETCH(int, spacing);

    // create some items to show
    QStringListModel model;
    QStringList list;
    for (int i = 0; i < 60; i++)
        list << QString::number(i);
    model.setStringList(list);

    // create listview
    QListView lv;
    lv.setFlow(static_cast<QListView::Flow>(flow));
    lv.setModel(&model);
    lv.setSpacing(spacing);
    lv.show();
    QTest::qWaitForWindowExposed(&lv);

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
#if defined Q_OS_BLACKBERRY
    // On BB10 we need to create a root window which is automatically stretched
    // over the whole screen
    QWindow rootWindow;
    rootWindow.show();
#endif
    QListView lv;

    QStringListModel model;
    QStringList list;
    for (int i = 0; i < 30; i++)
        list << QString::number(i);
    model.setStringList(list);
    lv.setModel(&model);

    lv.setRowHidden(1, true);
    lv.setSpacing(5);

    lv.showNormal();
    QTest::qWaitForWindowExposed(&lv);

    QCOMPARE(lv.verticalScrollBar()->value(), 0);

    lv.scrollTo(model.index(26, 0));
    int expectedScrollBarValue = lv.verticalScrollBar()->value();
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
        : m_oldStyle(qApp->style())
    {
        m_oldStyle->setParent(0);
        QListView tempView;
        if (QApplication::style()->styleHint(QStyle::SH_ScrollBar_Transient, 0, tempView.horizontalScrollBar()))
            QApplication::setStyle(QStyleFactory::create("Fusion"));
    }

    ~TempStyleSetter()
    {
        QApplication::setStyle(m_oldStyle);
    }
private:
    QStyle* m_oldStyle;
};

void tst_QListView::taskQTBUG_39902_mutualScrollBars_data()
{
    QTest::addColumn<QAbstractItemView::ScrollMode>("horizontalScrollMode");
    QTest::addColumn<QAbstractItemView::ScrollMode>("verticalScrollMode");
    QTest::newRow("per item / per item") << QAbstractItemView::ScrollPerItem << QAbstractItemView::ScrollPerItem;
    QTest::newRow("per pixel / per item") << QAbstractItemView::ScrollPerPixel << QAbstractItemView::ScrollPerItem;
    QTest::newRow("per item / per pixel") << QAbstractItemView::ScrollPerItem << QAbstractItemView::ScrollPerPixel;
    QTest::newRow("per pixel / per pixel") << QAbstractItemView::ScrollPerPixel << QAbstractItemView::ScrollPerPixel;
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
    view->resize(itemSize.width() + view->frameWidth() * 2, model.rowCount() * itemSize.height() + view->frameWidth() * 2);
    // this will end up in a stack overflow, if QTBUG-39902 is not fixed
    QTest::qWait(100);

    // these tests do not apply with transient scroll bars enabled
    QVERIFY (!view->style()->styleHint(QStyle::SH_ScrollBar_Transient, 0, view->horizontalScrollBar()));

    // make it double as large, no scroll bars should be visible
    view->resize((itemSize.width() + view->frameWidth() * 2) * 2, (model.rowCount() * itemSize.height() + view->frameWidth() * 2) * 2);
    QTRY_VERIFY(!view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(!view->verticalScrollBar()->isVisible());

    // make it half the size, both scroll bars should be visible
    view->resize((itemSize.width() + view->frameWidth() * 2) / 2, (model.rowCount() * itemSize.height() + view->frameWidth() * 2) / 2);
    QTRY_VERIFY(view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(view->verticalScrollBar()->isVisible());

    // make it double as large, no scroll bars should be visible
    view->resize((itemSize.width() + view->frameWidth() * 2) * 2, (model.rowCount() * itemSize.height() + view->frameWidth() * 2) * 2);
    QTRY_VERIFY(!view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(!view->verticalScrollBar()->isVisible());

    // now, coming from the double size, resize it to the exactly matching size, still no scroll bars should be visible again
    view->resize(itemSize.width() + view->frameWidth() * 2, model.rowCount() * itemSize.height() + view->frameWidth() * 2);
    QTRY_VERIFY(!view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(!view->verticalScrollBar()->isVisible());

    // now remove just one single pixel in height -> both scroll bars will show up since they depend on each other
    view->resize(itemSize.width() + view->frameWidth() * 2, model.rowCount() * itemSize.height() + view->frameWidth() * 2 - 1);
    QTRY_VERIFY(view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(view->verticalScrollBar()->isVisible());

    // now remove just one single pixel in width -> both scroll bars will show up since they depend on each other
    view->resize(itemSize.width() + view->frameWidth() * 2 - 1, model.rowCount() * itemSize.height() + view->frameWidth() * 2);
    QTRY_VERIFY(view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(view->verticalScrollBar()->isVisible());

    // finally, coming from a size being to small, resize back to the exactly matching size -> both scroll bars should disappear again
    view->resize(itemSize.width() + view->frameWidth() * 2, model.rowCount() * itemSize.height() + view->frameWidth() * 2);
    QTRY_VERIFY(!view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(!view->verticalScrollBar()->isVisible());

   // now remove just one single pixel in height -> both scroll bars will show up since they depend on each other
    view->resize(itemSize.width() + view->frameWidth() * 2, model.rowCount() * itemSize.height() + view->frameWidth() * 2 - 1);
    QTRY_VERIFY(view->horizontalScrollBar()->isVisible());
    QTRY_VERIFY(view->verticalScrollBar()->isVisible());
}

void tst_QListView::horizontalScrollingByVerticalWheelEvents()
{
    QListView lv;
    lv.setWrapping(true);

    TestDelegate *delegate = new TestDelegate(&lv);
    delegate->m_sizeHint = QSize(100, 100);
    lv.setItemDelegate(delegate);

    QtTestModel model;
    model.colCount = 1;
    model.rCount = 100;

    lv.setModel(&model);

    lv.resize(300, 300);
    lv.show();
    QTest::qWaitForWindowExposed(&lv);

    QPoint globalPos = lv.geometry().center();
    QPoint pos = lv.viewport()->geometry().center();

    QWheelEvent wheelDownEvent(pos, globalPos, QPoint(0, 0), QPoint(0, -120), -120, Qt::Vertical, 0, 0);
    QWheelEvent wheelUpEvent(pos, globalPos, QPoint(0, 0), QPoint(0, 120), 120, Qt::Vertical, 0, 0);
    QWheelEvent wheelLeftDownEvent(pos, globalPos, QPoint(0, 0), QPoint(120, -120), -120, Qt::Vertical, 0, 0);

    int hValue = lv.horizontalScrollBar()->value();
    QApplication::sendEvent(lv.viewport(), &wheelDownEvent);
    QVERIFY(lv.horizontalScrollBar()->value() > hValue);

    QApplication::sendEvent(lv.viewport(), &wheelUpEvent);
    QCOMPARE(lv.horizontalScrollBar()->value(), hValue);

    QApplication::sendEvent(lv.viewport(), &wheelLeftDownEvent);
    QCOMPARE(lv.horizontalScrollBar()->value(), hValue);

    // ensure that vertical wheel events are not converted when vertical
    // scroll bar is not visible but vertical scrolling is possible
    lv.setWrapping(false);
    lv.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QApplication::processEvents();

    int vValue = lv.verticalScrollBar()->value();
    QApplication::sendEvent(lv.viewport(), &wheelDownEvent);
    QVERIFY(lv.verticalScrollBar()->value() > vValue);
}

QTEST_MAIN(tst_QListView)
#include "tst_qlistview.moc"
