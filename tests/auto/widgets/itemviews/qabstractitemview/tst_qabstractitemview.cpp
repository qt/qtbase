// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qguiapplication_p.h>

#include <qpa/qplatformintegration.h>

#include <QAbstractItemView>
#include <QDialog>
#include <QHeaderView>
#include <QIdentityProxyModel>
#include <QItemDelegate>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProxyStyle>
#include <QPushButton>
#include <QScrollBar>
#include <QScroller>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QTest>
#include <QVBoxLayout>
#include <QtTest/private/qtesthelpers_p.h>
#include <private/qabstractitemview_p.h>
#include <QtWidgets/private/qapplication_p.h>

#include <vector>
#include <memory>

Q_DECLARE_METATYPE(Qt::ItemFlags);

using namespace QTestPrivate;
using IntList = QList<int>;

// Move cursor out of widget area to avoid undesired interaction on Mac.
static inline void moveCursorAway(const QWidget *topLevel)
{
#ifndef QT_NO_CURSOR
    QCursor::setPos(topLevel->geometry().topRight() + QPoint(100, 0));
#else
    Q_UNUSED(topLevel);
#endif
}

class GeometriesTestView : public QTableView
{
    Q_OBJECT
public:
    using QTableView::QTableView;
    using QTableView::selectedIndexes;
    bool updateGeometriesCalled = false;
protected slots:
    void updateGeometries() override { updateGeometriesCalled = true; QTableView::updateGeometries(); }
};

class tst_QAbstractItemView : public QObject
{
    Q_OBJECT

public:
    void basic_tests(QAbstractItemView *view);

private slots:
    void cleanup();
    void getSetCheck();
    void emptyModels_data();
    void emptyModels();
    void setModel_data();
    void setModel();
    void noModel();
    void dragSelect();
    void rowDelegate();
    void columnDelegate();
    void selectAll();
    void ctrlA();
    void persistentEditorFocus();
    void pressClosesReleaseDoesntOpenEditor();
    void setItemDelegate();
    void setItemDelegate_data();
    // The dragAndDrop() test doesn't work, and is thus disabled on Mac and Windows
    // for the following reasons:
    //   Mac: use of GetCurrentEventButtonState() in QDragManager::drag()
    //   Win: unknown reason
#if !defined(Q_OS_MAC) && !defined(Q_OS_WIN)
#if 0
    void dragAndDrop();
    void dragAndDropOnChild();
#endif
#endif
    void noFallbackToRoot();
    void setCurrentIndex_data();
    void setCurrentIndex();

    void checkIntersectedRect_data();
    void checkIntersectedRect();

    void task221955_selectedEditor();
    void task250754_fontChange();
    void task200665_itemEntered();
    void task257481_emptyEditor();
    void shiftArrowSelectionAfterScrolling();
    void shiftSelectionAfterRubberbandSelection();
    void ctrlRubberbandSelection();
    void QTBUG6407_extendedSelection();
    void QTBUG6753_selectOnSelection();
    void testDelegateDestroyEditor();
    void testDelegateDestroyEditorChild();
    void testClickedSignal();
    void testChangeEditorState();
    void deselectInSingleSelection();
    void testNoActivateOnDisabledItem();
    void testFocusPolicy_data();
    void testFocusPolicy();
    void QTBUG31411_noSelection();
    void QTBUG39324_settingSameInstanceOfIndexWidget();
    void sizeHintChangeTriggersLayout();
    void shiftSelectionAfterChangingModelContents();
    void QTBUG48968_reentrant_updateEditorGeometries();
    void QTBUG50102_SH_ItemView_ScrollMode();
    void QTBUG50535_update_on_new_selection_model();
    void testSelectionModelInSyncWithView();
    void testClickToSelect();
    void testDialogAsEditor();
    void QTBUG46785_mouseout_hover_state();
    void testClearModelInClickedSignal();
    void inputMethodEnabled_data();
    void inputMethodEnabled();
    void currentFollowsIndexWidget_data();
    void currentFollowsIndexWidget();
    void checkFocusAfterActivationChanges_data();
    void checkFocusAfterActivationChanges();
    void dragSelectAfterNewPress();
    void dragWithSecondClick_data();
    void dragWithSecondClick();
    void clickAfterDoubleClick();
    void selectionCommand_data();
    void selectionCommand();
    void mouseSelection_data();
    void mouseSelection();
    void keepSingleSelectionOnEmptyAreaClick();
    void scrollerSmoothScroll();
    void inputMethodOpensEditor_data();
    void inputMethodOpensEditor();
    void selectionAutoScrolling_data();
    void selectionAutoScrolling();
    void testSpinBoxAsEditor_data();
    void testSpinBoxAsEditor();
    void removeIndexWhileEditing();
    void focusNextOnHide();
    void shiftSelectionAfterModelSetCurrentIndex();

private:
    static QAbstractItemView *viewFromString(const QByteArray &viewType, QWidget *parent = nullptr)
    {
      if (viewType == "QListView")
          return new QListView(parent);
      if (viewType == "QTableView")
          return new QTableView(parent);
      if (viewType == "QTreeView")
          return new QTreeView(parent);
      if (viewType == "QHeaderView")
          return new QHeaderView(Qt::Vertical, parent);
      Q_ASSERT(false);
      return nullptr;
    }
};

class MyAbstractItemDelegate : public QAbstractItemDelegate
{
public:
    using QAbstractItemDelegate::QAbstractItemDelegate;
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override {}
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override { return size; }
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        openedEditor = new QWidget(parent);
        virtualCtorCallCount++;
        return openedEditor;
    }
    void destroyEditor(QWidget *editor, const QModelIndex &) const override
    {
        virtualDtorCallCount++;
        editor->deleteLater();
    }
    void changeSize() { size = QSize(50, 50); emit sizeHintChanged(QModelIndex()); }
    mutable QWidget *openedEditor = nullptr;
    QSize size;
    mutable int virtualCtorCallCount = 0;
    mutable int virtualDtorCallCount = 0;
};

class DialogItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        openedEditor = new QDialog(parent);
        return openedEditor;
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        Q_UNUSED(model);
        Q_UNUSED(index);

        QDialog *dialog = qobject_cast<QDialog *>(editor);
        result = static_cast<QDialog::DialogCode>(dialog->result());
    }

    mutable QDialog *openedEditor = nullptr;
    mutable QDialog::DialogCode result = QDialog::Rejected;
};

// Testing get/set functions
void tst_QAbstractItemView::getSetCheck()
{
    QListView view;
    QAbstractItemView *obj1 = &view;
    // QAbstractItemDelegate * QAbstractItemView::itemDelegate()
    // void QAbstractItemView::setItemDelegate(QAbstractItemDelegate *)
    MyAbstractItemDelegate *var1 = new MyAbstractItemDelegate;
    obj1->setItemDelegate(var1);
    QCOMPARE(var1, obj1->itemDelegate());
    obj1->setItemDelegate(nullptr);
    QCOMPARE(obj1->itemDelegate(), nullptr);
    delete var1;

    // EditTriggers )
    // void QAbstractItemView::setEditTriggers(EditTriggers)
    obj1->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QCOMPARE(obj1->editTriggers(), QAbstractItemView::NoEditTriggers);
    obj1->setEditTriggers(QAbstractItemView::CurrentChanged);
    QCOMPARE(obj1->editTriggers(), QAbstractItemView::CurrentChanged);
    obj1->setEditTriggers(QAbstractItemView::DoubleClicked);
    QCOMPARE(obj1->editTriggers(), QAbstractItemView::DoubleClicked);
    obj1->setEditTriggers(QAbstractItemView::SelectedClicked);
    QCOMPARE(obj1->editTriggers(), QAbstractItemView::SelectedClicked);
    obj1->setEditTriggers(QAbstractItemView::EditKeyPressed);
    QCOMPARE(obj1->editTriggers(), QAbstractItemView::EditKeyPressed);
    obj1->setEditTriggers(QAbstractItemView::AnyKeyPressed);
    QCOMPARE(obj1->editTriggers(), QAbstractItemView::AnyKeyPressed);
    obj1->setEditTriggers(QAbstractItemView::AllEditTriggers);
    QCOMPARE(obj1->editTriggers(), QAbstractItemView::AllEditTriggers);

    // bool QAbstractItemView::tabKeyNavigation()
    // void QAbstractItemView::setTabKeyNavigation(bool)
    obj1->setTabKeyNavigation(false);
    QCOMPARE(false, obj1->tabKeyNavigation());
    obj1->setTabKeyNavigation(true);
    QCOMPARE(true, obj1->tabKeyNavigation());

    // bool QAbstractItemView::dragEnabled()
    // void QAbstractItemView::setDragEnabled(bool)
#if QT_CONFIG(draganddrop)
    obj1->setDragEnabled(false);
    QCOMPARE(false, obj1->dragEnabled());
    obj1->setDragEnabled(true);
    QCOMPARE(true, obj1->dragEnabled());
#endif
    // bool QAbstractItemView::alternatingRowColors()
    // void QAbstractItemView::setAlternatingRowColors(bool)
    obj1->setAlternatingRowColors(false);
    QCOMPARE(false, obj1->alternatingRowColors());
    obj1->setAlternatingRowColors(true);
    QCOMPARE(true, obj1->alternatingRowColors());

    // State QAbstractItemView::state()
    // void QAbstractItemView::setState(State)
    obj1->setState(QAbstractItemView::NoState);
    QCOMPARE(QAbstractItemView::NoState, obj1->state());
    obj1->setState(QAbstractItemView::DraggingState);
    QCOMPARE(QAbstractItemView::DraggingState, obj1->state());
    obj1->setState(QAbstractItemView::DragSelectingState);
    QCOMPARE(QAbstractItemView::DragSelectingState, obj1->state());
    obj1->setState(QAbstractItemView::EditingState);
    QCOMPARE(QAbstractItemView::EditingState, obj1->state());
    obj1->setState(QAbstractItemView::ExpandingState);
    QCOMPARE(QAbstractItemView::ExpandingState, obj1->state());
    obj1->setState(QAbstractItemView::CollapsingState);
    QCOMPARE(QAbstractItemView::CollapsingState, obj1->state());

    // QWidget QAbstractScrollArea::viewport()
    // void setViewport(QWidget*)
    QWidget *vp = new QWidget;
    obj1->setViewport(vp);
    QCOMPARE(vp, obj1->viewport());

    QCOMPARE(16, obj1->autoScrollMargin());
    obj1->setAutoScrollMargin(20);
    QCOMPARE(20, obj1->autoScrollMargin());
    obj1->setAutoScrollMargin(16);
    QCOMPARE(16, obj1->autoScrollMargin());

    QCOMPARE(200U, obj1->updateThreshold());
    obj1->setUpdateThreshold(4711);
    QCOMPARE(4711U, obj1->updateThreshold());
}

void tst_QAbstractItemView::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QAbstractItemView::emptyModels_data()
{
    QTest::addColumn<QByteArray>("viewType");

    const QList<QByteArray> widgets { "QListView", "QTreeView", "QTableView", "QHeaderView" };
    for (const QByteArray &widget : widgets)
        QTest::newRow(widget) << widget;
}

void tst_QAbstractItemView::emptyModels()
{
    QFETCH(QByteArray, viewType);

    QScopedPointer<QAbstractItemView> view(viewFromString(viewType));
    centerOnScreen(view.data());
    moveCursorAway(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QVERIFY(!view->model());
    QVERIFY(!view->selectionModel());
    //QVERIFY(view->itemDelegate() != 0);

    basic_tests(view.data());
}

void tst_QAbstractItemView::setModel_data()
{
    emptyModels_data();
}

void tst_QAbstractItemView::setModel()
{
    QFETCH(QByteArray, viewType);

    QScopedPointer<QAbstractItemView> view(viewFromString(viewType));
    centerOnScreen(view.data());
    moveCursorAway(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QStandardItemModel model(20,20);
    view->setModel(nullptr);
    view->setModel(&model);
    basic_tests(view.data());
}

void tst_QAbstractItemView::basic_tests(QAbstractItemView *view)
{
    // setSelectionModel
    // Will assert as it should
    //view->setSelectionModel(0);
    // setItemDelegate
    //view->setItemDelegate(0);
    // Will asswert as it should

    // setSelectionMode
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::SingleSelection);
    view->setSelectionMode(QAbstractItemView::ContiguousSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::ContiguousSelection);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::ExtendedSelection);
    view->setSelectionMode(QAbstractItemView::MultiSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::MultiSelection);
    view->setSelectionMode(QAbstractItemView::NoSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::NoSelection);

    // setSelectionBehavior
    view->setSelectionBehavior(QAbstractItemView::SelectItems);
    QCOMPARE(view->selectionBehavior(), QAbstractItemView::SelectItems);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    QCOMPARE(view->selectionBehavior(), QAbstractItemView::SelectRows);
    view->setSelectionBehavior(QAbstractItemView::SelectColumns);
    QCOMPARE(view->selectionBehavior(), QAbstractItemView::SelectColumns);

    // setEditTriggers
    view->setEditTriggers(QAbstractItemView::EditKeyPressed);
    QCOMPARE(view->editTriggers(), QAbstractItemView::EditKeyPressed);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QCOMPARE(view->editTriggers(), QAbstractItemView::NoEditTriggers);
    view->setEditTriggers(QAbstractItemView::CurrentChanged);
    QCOMPARE(view->editTriggers(), QAbstractItemView::CurrentChanged);
    view->setEditTriggers(QAbstractItemView::DoubleClicked);
    QCOMPARE(view->editTriggers(), QAbstractItemView::DoubleClicked);
    view->setEditTriggers(QAbstractItemView::SelectedClicked);
    QCOMPARE(view->editTriggers(), QAbstractItemView::SelectedClicked);
    view->setEditTriggers(QAbstractItemView::AnyKeyPressed);
    QCOMPARE(view->editTriggers(), QAbstractItemView::AnyKeyPressed);
    view->setEditTriggers(QAbstractItemView::AllEditTriggers);
    QCOMPARE(view->editTriggers(), QAbstractItemView::AllEditTriggers);

    // setAutoScroll
    view->setAutoScroll(false);
    QCOMPARE(view->hasAutoScroll(), false);
    view->setAutoScroll(true);
    QCOMPARE(view->hasAutoScroll(), true);

    // setTabKeyNavigation
    view->setTabKeyNavigation(false);
    QCOMPARE(view->tabKeyNavigation(), false);
    view->setTabKeyNavigation(true);
    QCOMPARE(view->tabKeyNavigation(), true);

#if QT_CONFIG(draganddrop)
    // setDropIndicatorShown
    view->setDropIndicatorShown(false);
    QCOMPARE(view->showDropIndicator(), false);
    view->setDropIndicatorShown(true);
    QCOMPARE(view->showDropIndicator(), true);

    // setDragEnabled
    view->setDragEnabled(false);
    QCOMPARE(view->dragEnabled(), false);
    view->setDragEnabled(true);
    QCOMPARE(view->dragEnabled(), true);
#endif

    // setAlternatingRowColors
    view->setAlternatingRowColors(false);
    QCOMPARE(view->alternatingRowColors(), false);
    view->setAlternatingRowColors(true);
    QCOMPARE(view->alternatingRowColors(), true);

    // setIconSize
    view->setIconSize(QSize(16, 16));
    QCOMPARE(view->iconSize(), QSize(16, 16));
    QSignalSpy spy(view, &QAbstractItemView::iconSizeChanged);
    QVERIFY(spy.isValid());
    view->setIconSize(QSize(32, 32));
    QCOMPARE(view->iconSize(), QSize(32, 32));
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).value<QSize>(), QSize(32, 32));
    // Should this happen?
    view->setIconSize(QSize(-1, -1));
    QCOMPARE(view->iconSize(), QSize(-1, -1));
    QCOMPARE(spy.size(), 2);

    QCOMPARE(view->currentIndex(), QModelIndex());
    QCOMPARE(view->rootIndex(), QModelIndex());

    view->keyboardSearch("");
    view->keyboardSearch("foo");
    view->keyboardSearch("1");

    QCOMPARE(view->visualRect(QModelIndex()), QRect());

    view->scrollTo(QModelIndex());

    QCOMPARE(view->sizeHintForIndex(QModelIndex()), QSize());
    QCOMPARE(view->indexAt(QPoint(-1, -1)), QModelIndex());

    if (!view->model()) {
        QCOMPARE(view->indexAt(QPoint(10, 10)), QModelIndex());
        QCOMPARE(view->sizeHintForRow(0), -1);
        QCOMPARE(view->sizeHintForColumn(0), -1);
    } else if (view->itemDelegate()) {
        view->sizeHintForRow(0);
        view->sizeHintForColumn(0);
    }
    view->openPersistentEditor(QModelIndex());
    view->closePersistentEditor(QModelIndex());

    view->reset();
    view->setRootIndex(QModelIndex());
    view->doItemsLayout();
    view->selectAll();
    view->edit(QModelIndex());
    view->clearSelection();
    view->setCurrentIndex(QModelIndex());

    // protected methods
    // will assert because an invalid index is passed
    //view->dataChanged(QModelIndex(), QModelIndex());
    view->rowsInserted(QModelIndex(), -1, -1);
    view->rowsAboutToBeRemoved(QModelIndex(), -1, -1);
    view->selectionChanged(QItemSelection(), QItemSelection());
    if (view->model()) {
        view->currentChanged(QModelIndex(), QModelIndex());
        view->currentChanged(QModelIndex(), view->model()->index(0,0));
    }
    view->updateEditorData();
    view->updateEditorGeometries();
    view->updateGeometries();
    view->verticalScrollbarAction(QAbstractSlider::SliderSingleStepAdd);
    view->horizontalScrollbarAction(QAbstractSlider::SliderSingleStepAdd);
    view->verticalScrollbarValueChanged(10);
    view->horizontalScrollbarValueChanged(10);
    view->closeEditor(nullptr, QAbstractItemDelegate::NoHint);
    view->commitData(nullptr);
    view->editorDestroyed(nullptr);

    // Will assert as it should
    // view->setIndexWidget(QModelIndex(), 0);

    view->moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);
    view->horizontalOffset();
    view->verticalOffset();

//    view->isIndexHidden(QModelIndex()); // will (correctly) assert
    if (view->model())
        view->isIndexHidden(view->model()->index(0,0));

    view->setSelection(QRect(0, 0, 10, 10), QItemSelectionModel::ClearAndSelect);
    view->setSelection(QRect(-1, -1, -1, -1), QItemSelectionModel::ClearAndSelect);
    view->visualRegionForSelection(QItemSelection());
    view->selectedIndexes();

    view->edit(QModelIndex(), QAbstractItemView::NoEditTriggers, nullptr);

    view->selectionCommand(QModelIndex(), nullptr);

#if QT_CONFIG(draganddrop)
    if (!view->model())
        view->startDrag(Qt::CopyAction);
    QStyleOptionViewItem option;
    view->initViewItemOption(&option);

    view->setState(QAbstractItemView::NoState);
    QCOMPARE(view->state(), QAbstractItemView::NoState);
    view->setState(QAbstractItemView::DraggingState);
    QCOMPARE(view->state(), QAbstractItemView::DraggingState);
    view->setState(QAbstractItemView::DragSelectingState);
    QCOMPARE(view->state(), QAbstractItemView::DragSelectingState);
    view->setState(QAbstractItemView::EditingState);
    QCOMPARE(view->state(), QAbstractItemView::EditingState);
    view->setState(QAbstractItemView::ExpandingState);
    QCOMPARE(view->state(), QAbstractItemView::ExpandingState);
    view->setState(QAbstractItemView::CollapsingState);
    QCOMPARE(view->state(), QAbstractItemView::CollapsingState);
#endif

    view->startAutoScroll();
    view->stopAutoScroll();
    view->doAutoScroll();

    // testing mouseFoo and key functions
//     QTest::mousePress(view, Qt::LeftButton, Qt::NoModifier, QPoint(0,0));
//     mouseMove(view, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
//     QTest::mouseRelease(view, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
//     QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
//     mouseDClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
//     QTest::keyClick(view, Qt::Key_A);
}

void tst_QAbstractItemView::noModel()
{
    // From task #85415

    QStandardItemModel model(20,20);
    QTreeView view;
    setFrameless(&view);

    view.setModel(&model);
    // Make the viewport smaller than the contents, so that we can scroll
    view.resize(100,100);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // make sure that the scrollbars are not at value 0
    view.scrollTo(view.model()->index(10,10));
    QApplication::processEvents();

    view.setModel(nullptr);
    // Due to the model is removed, this will generate a valueChanged signal on both scrollbars. (value to 0)
    QApplication::processEvents();
    QCOMPARE(view.model(), nullptr);
}

void tst_QAbstractItemView::dragSelect()
{
    // From task #86108

    QStandardItemModel model(64, 64);

    QTableView view;
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.setVisible(true);
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const int delay = 2;
    for (int i = 0; i < 2; ++i) {
        bool tracking = (i == 1);
        view.setMouseTracking(false);
        QTest::mouseMove(&view, QPoint(0, 0), delay);
        view.setMouseTracking(tracking);
        QTest::mouseMove(&view, QPoint(50, 50), delay);
        QVERIFY(view.selectionModel()->selectedIndexes().isEmpty());
    }
}

void tst_QAbstractItemView::rowDelegate()
{
    QStandardItemModel model(4, 4);
    MyAbstractItemDelegate delegate;

    QTableView view;
    view.setModel(&model);
    view.setItemDelegateForRow(3, &delegate);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index = model.index(3, 0);
    QVERIFY(!view.isPersistentEditorOpen(index));
    view.openPersistentEditor(index);
    QVERIFY(view.isPersistentEditorOpen(index));
    QWidget *w = view.indexWidget(index);
    QVERIFY(w);
    QCOMPARE(w->metaObject()->className(), "QWidget");
}

void tst_QAbstractItemView::columnDelegate()
{
    QStandardItemModel model(4, 4);
    MyAbstractItemDelegate delegate;

    QTableView view;
    view.setModel(&model);
    view.setItemDelegateForColumn(3, &delegate);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index = model.index(0, 3);
    QVERIFY(!view.isPersistentEditorOpen(index));
    view.openPersistentEditor(index);
    QVERIFY(view.isPersistentEditorOpen(index));
    QWidget *w = view.indexWidget(index);
    QVERIFY(w);
    QCOMPARE(w->metaObject()->className(), "QWidget");
}

void tst_QAbstractItemView::sizeHintChangeTriggersLayout()
{
    QStandardItemModel model(4, 4);
    MyAbstractItemDelegate delegate;
    MyAbstractItemDelegate rowDelegate;
    MyAbstractItemDelegate columnDelegate;

    GeometriesTestView view;
    view.setModel(&model);
    view.setItemDelegate(&delegate);
    view.setItemDelegateForRow(1, &rowDelegate);
    view.setItemDelegateForColumn(2, &columnDelegate);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.updateGeometriesCalled = false;
    delegate.changeSize();
    QTRY_VERIFY(view.updateGeometriesCalled);
    view.updateGeometriesCalled = false;
    rowDelegate.changeSize();
    QTRY_VERIFY(view.updateGeometriesCalled);
    view.updateGeometriesCalled = false;
    columnDelegate.changeSize();
    QTRY_VERIFY(view.updateGeometriesCalled);
}

void tst_QAbstractItemView::selectAll()
{
    QStandardItemModel model(4, 4);
    GeometriesTestView view;
    view.setModel(&model);

    QCOMPARE(view.selectedIndexes().size(), 0);
    view.selectAll();
    QCOMPARE(view.selectedIndexes().size(), 4 * 4);
}

void tst_QAbstractItemView::ctrlA()
{
    QStandardItemModel model(4, 4);
    GeometriesTestView view;
    view.setModel(&model);

    QCOMPARE(view.selectedIndexes().size(), 0);
    QTest::keyClick(&view, Qt::Key_A, Qt::ControlModifier);
    QCOMPARE(view.selectedIndexes().size(), 4 * 4);
}

void tst_QAbstractItemView::persistentEditorFocus()
{
    // one row, three columns
    QStandardItemModel model(1, 3);
    for(int i = 0; i < model.columnCount(); ++i)
        model.setData(model.index(0, i), i);
    QTableView view;
    view.setModel(&model);

    view.openPersistentEditor(model.index(0, 1));
    view.openPersistentEditor(model.index(0, 2));

    //these are spinboxes because we put numbers inside
    const QList<QSpinBox*> list = view.viewport()->findChildren<QSpinBox*>();
    QCOMPARE(list.size(), 2); //these should be the 2 editors

    view.setCurrentIndex(model.index(0, 0));
    QCOMPARE(view.currentIndex(), model.index(0, 0));
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const QPoint p(5, 5);
    for (QSpinBox *sb : list) {
        QTRY_VERIFY(sb->isVisible());
        QMouseEvent mouseEvent(QEvent::MouseButtonPress, p, sb->mapToGlobal(p),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(sb, &mouseEvent);
        if (!QApplication::focusWidget())
            QSKIP("Some window managers don't handle focus that well");
        QTRY_COMPARE(QApplication::focusWidget(), sb);
    }
}

/*!
    A press into the selection area of an item being edited, but outside the editor,
    closes the editor by transferring focus to the view. The corresponding release
    should then not re-open the editor.

    QTBUG-20456.
*/
void tst_QAbstractItemView::pressClosesReleaseDoesntOpenEditor()
{
    QStandardItemModel model(0, 1);
    auto *parent = new QStandardItem("parent");
    for (const auto &childText : {"child1", "child2"}) {
        auto *child = new QStandardItem(childText);
        child->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsEditable | Qt::ItemIsSelectable);
        parent->appendRow(child);
    }
    model.appendRow(parent);

    QTreeView view;
    view.setModel(&model);
    view.setExpanded(model.indexFromItem(parent), true);
    view.setSelectionMode(QAbstractItemView::SingleSelection);
    view.setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const QRect childRect = view.visualRect(model.indexFromItem(parent->child(0)));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, childRect.center()); // select
    QVERIFY(view.selectionModel()->selectedIndexes().contains(model.indexFromItem(parent->child(0))));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, childRect.center()); // edit
    QTRY_COMPARE(view.state(), QAbstractItemView::EditingState);
    QPoint inChildOutsideEditor = QPoint(view.indentation() / 2, childRect.center().y());
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, inChildOutsideEditor); // focus itemview, editor closes
    QCOMPARE(view.state(), QAbstractItemView::NoState);
    QTest::qWait(10); // process some events, let the internal timer time out
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, inChildOutsideEditor); // should not reopen editor
    QTest::qWait(QApplication::doubleClickInterval() * 2);
    QCOMPARE(view.state(), QAbstractItemView::NoState);
}


#if !defined(Q_OS_MAC) && !defined(Q_OS_WIN)

#if 0

static void sendMouseMove(QWidget *widget, QPoint pos = QPoint())
{
    if (pos.isNull())
        pos = widget->rect().center();
    QMouseEvent event(QEvent::MouseMove, pos, widget->mapToGlobal(pos), Qt::NoButton, 0, 0);
    QCursor::setPos(widget->mapToGlobal(pos));
    qApp->processEvents();
    QVERIFY(QTest::qWaitForWindowExposed(widget));
    QApplication::sendEvent(widget, &event);
}

static void sendMousePress(
    QWidget *widget, QPoint pos = QPoint(), Qt::MouseButton button = Qt::LeftButton)
{
    if (pos.isNull())
         pos = widget->rect().center();
    QMouseEvent event(QEvent::MouseButtonPress, pos, widget->mapToGlobal(pos), button, 0, 0);
    QApplication::sendEvent(widget, &event);
}

static void sendMouseRelease(
    QWidget *widget, QPoint pos = QPoint(), Qt::MouseButton button = Qt::LeftButton)
{
    if (pos.isNull())
         pos = widget->rect().center();
    QMouseEvent event(QEvent::MouseButtonRelease, pos, widget->mapToGlobal(pos), button, 0, 0);
    QApplication::sendEvent(widget, &event);
}

class DnDTestModel : public QStandardItemModel
{
    Q_OBJECT
    bool dropMimeData(const QMimeData *md, Qt::DropAction action, int r, int c, const QModelIndex &p)
    {
        dropAction_result = action;
        QStandardItemModel::dropMimeData(md, action, r, c, p);
        return true;
    }
    Qt::DropActions supportedDropActions() const { return Qt::CopyAction | Qt::MoveAction; }

    Qt::DropAction dropAction_result;
public:
    DnDTestModel() : QStandardItemModel(20, 20), dropAction_result(Qt::IgnoreAction) {
        for (int i = 0; i < rowCount(); ++i)
            setData(index(i, 0), QString::number(i));
    }
    Qt::DropAction dropAction() const { return dropAction_result; }
};

class DnDTestView : public QTreeView
{
    Q_OBJECT

    QPoint dropPoint;
    Qt::DropAction dropAction;

    void dragEnterEvent(QDragEnterEvent *event)
    {
        QAbstractItemView::dragEnterEvent(event);
    }

    void dropEvent(QDropEvent *event)
    {
        event->setDropAction(dropAction);
        QTreeView::dropEvent(event);
    }

    void timerEvent(QTimerEvent *event)
    {
        killTimer(event->timerId());
        sendMouseMove(this, dropPoint);
        sendMouseRelease(this);
    }

    void mousePressEvent(QMouseEvent *e)
    {
        QTreeView::mousePressEvent(e);

        startTimer(0);
        setState(DraggingState);
        startDrag(dropAction);
    }

public:
    DnDTestView(Qt::DropAction dropAction, QAbstractItemModel *model)
        : dropAction(dropAction)
    {
        header()->hide();
        setModel(model);
        setDragDropMode(QAbstractItemView::DragDrop);
        setAcceptDrops(true);
        setDragEnabled(true);
    }

    void dragAndDrop(QPoint drag, QPoint drop)
    {
        dropPoint = drop;
        setCurrentIndex(indexAt(drag));
        sendMousePress(viewport(), drag);
    }
};

class DnDTestWidget : public QWidget
{
    Q_OBJECT

    Qt::DropAction dropAction_request;
    Qt::DropAction dropAction_result;
    QWidget *dropTarget;

    void timerEvent(QTimerEvent *event)
    {
        killTimer(event->timerId());
        sendMouseMove(dropTarget);
        sendMouseRelease(dropTarget);
    }

    void mousePressEvent(QMouseEvent *)
    {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setData("application/x-qabstractitemmodeldatalist", QByteArray(""));
        drag->setMimeData(mimeData);
        startTimer(0);
        dropAction_result = drag->start(dropAction_request);
    }

public:
    Qt::DropAction dropAction() const { return dropAction_result; }

    void dragAndDrop(QWidget *dropTarget, Qt::DropAction dropAction)
    {
        this->dropTarget = dropTarget;
        dropAction_request = dropAction;
        sendMousePress(this);
    }
};

void tst_QAbstractItemView::dragAndDrop()
{
    // From Task 137729


    const int attempts = 10;
    int successes = 0;
    for (int i = 0; i < attempts; ++i) {
        Qt::DropAction dropAction = Qt::MoveAction;

        DnDTestModel model;
        DnDTestView view(dropAction, &model);
        DnDTestWidget widget;

        const int size = 200;
        widget.setFixedSize(size, size);
        view.setFixedSize(size, size);

        widget.move(0, 0);
        view.move(int(size * 1.5), int(size * 1.5));

        widget.show();
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        widget.dragAndDrop(&view, dropAction);
        if (model.dropAction() == dropAction
            && widget.dropAction() == dropAction)
            ++successes;
    }

    if (successes < attempts) {
        QString msg = QString("# successes (%1) < # attempts (%2)").arg(successes).arg(attempts);
        qWarning() << qPrintable(msg);
    }
    QVERIFY(successes > 0); // allow for some "event unstability" (i.e. unless
                            // successes == 0, QAbstractItemView is probably ok!)
}

void tst_QAbstractItemView::dragAndDropOnChild()
{

    const int attempts = 10;
    int successes = 0;
    for (int i = 0; i < attempts; ++i) {
        Qt::DropAction dropAction = Qt::MoveAction;

        DnDTestModel model;
        QModelIndex parent = model.index(0, 0);
        model.insertRow(0, parent);
        model.insertColumn(0, parent);
        QModelIndex child = model.index(0, 0, parent);
        model.setData(child, "child");
        QCOMPARE(model.rowCount(parent), 1);
        DnDTestView view(dropAction, &model);
        view.setExpanded(parent, true);
        view.setDragDropMode(QAbstractItemView::InternalMove);

        const int size = 200;
        view.setFixedSize(size, size);
        view.move(int(size * 1.5), int(size * 1.5));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        view.dragAndDrop(view.visualRect(parent).center(),
                         view.visualRect(child).center());
        if (model.dropAction() == dropAction)
            ++successes;
    }

    QCOMPARE(successes, 0);
}

#endif // 0
#endif // !Q_OS_MAC && !Q_OS_WIN

class TestModel : public QStandardItemModel
{
    Q_OBJECT
public:
    using QStandardItemModel::QStandardItemModel;
    bool setData(const QModelIndex &, const QVariant &, int) override
    {
        ++setData_count;
        return true;
    }
    void emitDataChanged()
    {
        emit dataChanged(index(0, 0), index(0, 1));
    }

    int setData_count = 0;
};

void tst_QAbstractItemView::setItemDelegate_data()
{
    // default is rows, a -1 will switch to columns
    QTest::addColumn<IntList>("rowsOrColumnsWithDelegate");
    QTest::addColumn<QPoint>("cellToEdit");
    QTest::newRow("4 columndelegates")
                << (IntList{ -1, 0, 1, 2, 3 })
                << QPoint(0, 0);
    QTest::newRow("2 identical rowdelegates on the same row")
                << (IntList{ 0, 0 })
                << QPoint(0, 0);
    QTest::newRow("2 identical columndelegates on the same column")
                << (IntList{ -1, 2, 2 })
                << QPoint(2, 0);
    QTest::newRow("2 duplicate delegates, 1 row and 1 column")
                << (IntList{ 0, -1, 2 })
                << QPoint(2, 0);
    QTest::newRow("4 duplicate delegates, 2 row and 2 column")
                << (IntList{ 0, 0, -1, 2, 2 })
                << QPoint(2, 0);

}

void tst_QAbstractItemView::setItemDelegate()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QFETCH(const IntList, rowsOrColumnsWithDelegate);
    QFETCH(QPoint, cellToEdit);
    QTableView v;
    QStyledItemDelegate *delegate = new QStyledItemDelegate(&v);
    TestModel model(5, 5);
    v.setModel(&model);

    bool row = true;
    for (int rc : rowsOrColumnsWithDelegate) {
        if (rc == -1) {
            row = !row;
        } else {
            if (row)
                v.setItemDelegateForRow(rc, delegate);
            else
                v.setItemDelegateForColumn(rc, delegate);
        }
    }
    centerOnScreen(&v);
    moveCursorAway(&v);
    v.show();
    QVERIFY(QTest::qWaitForWindowActive(&v));

    QModelIndex index = model.index(cellToEdit.y(), cellToEdit.x());
    v.edit(index);

    // This will close the editor
    QTRY_VERIFY(QApplication::focusWidget());
    QWidget *editor = QApplication::focusWidget();
    QVERIFY(editor);
    editor->hide();
    delete editor;
    QCOMPARE(model.setData_count, 1);
    delete delegate;
}

void tst_QAbstractItemView::noFallbackToRoot()
{
    QStandardItemModel model(0, 1);
    for (int i = 0; i < 5; ++i)
        model.appendRow(new QStandardItem("top" + QString::number(i)));
    QStandardItem *par1 = model.item(1);
    for (int j = 0; j < 15; ++j)
        par1->appendRow(new QStandardItem("sub" + QString::number(j)));
    QStandardItem *par2 = par1->child(2);
    for (int k = 0; k < 10; ++k)
        par2->appendRow(new QStandardItem("bot" + QString::number(k)));
    QStandardItem *it1 = par2->child(5);

    QModelIndex parent1 = model.indexFromItem(par1);
    QModelIndex parent2 = model.indexFromItem(par2);
    QModelIndex item1 = model.indexFromItem(it1);

    QTreeView v;
    v.setModel(&model);
    v.setRootIndex(parent1);
    v.setCurrentIndex(item1);
    QCOMPARE(v.currentIndex(), item1);
    QVERIFY(model.removeRows(0, 10, parent2));
    QCOMPARE(v.currentIndex(), parent2);
    QVERIFY(model.removeRows(0, 15, parent1));
    QCOMPARE(v.currentIndex(), QModelIndex());
}

void tst_QAbstractItemView::setCurrentIndex_data()
{
    QTest::addColumn<QByteArray>("viewType");
    QTest::addColumn<Qt::ItemFlags>("itemFlags");
    QTest::addColumn<bool>("result");

    const QList<QByteArray> widgets { "QListView", "QTreeView", "QTableView", "QHeaderView" };
    for (const QByteArray &widget : widgets) {
        QTest::newRow(widget + ": no flags")
            << widget << Qt::ItemFlags(Qt::NoItemFlags) << false;
        QTest::newRow(widget + ": checkable")
            << widget << Qt::ItemFlags(Qt::ItemIsUserCheckable) << false;
        QTest::newRow(widget + ": selectable")
            << widget << Qt::ItemFlags(Qt::ItemIsSelectable) << false;
        QTest::newRow(widget + ": enabled")
            << widget << Qt::ItemFlags(Qt::ItemIsEnabled) << true;
        QTest::newRow(widget + ": enabled|selectable")
            << widget << (Qt::ItemIsSelectable|Qt::ItemIsEnabled) << true;
    }
}

void tst_QAbstractItemView::setCurrentIndex()
{
    QFETCH(QByteArray, viewType);
    QFETCH(Qt::ItemFlags, itemFlags);
    QFETCH(bool, result);

    QScopedPointer<QAbstractItemView> view(viewFromString(viewType));

    centerOnScreen(view.data());
    moveCursorAway(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QStandardItemModel *model = new QStandardItemModel(view.data());
    QStandardItem *item = new QStandardItem("first item");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->appendRow(item);

    item = new QStandardItem("test item");
    item->setFlags(itemFlags);
    model->appendRow(item);

    view->setModel(model);

    view->setCurrentIndex(model->index(0, 0));
    QCOMPARE(view->currentIndex(), model->index(0, 0));
    view->setCurrentIndex(model->index(1, 0));
    QVERIFY(view->currentIndex() == model->index(result ? 1 : 0, 0));
}

void tst_QAbstractItemView::checkIntersectedRect_data()
{
    auto createModel = [](int rowCount) -> QStandardItemModel*
    {
        QStandardItemModel *model = new QStandardItemModel;
        for (int i = 0; i < rowCount; ++i) {
            const QList<QStandardItem *> sil({new QStandardItem(QString("Row %1 Item").arg(i)),
                                              new QStandardItem(QString("2nd column"))});
            model->appendRow(sil);
        }
        return model;
    };
    QTest::addColumn<QStandardItemModel *>("model");
    QTest::addColumn<QList<QModelIndex>>("changedIndexes");
    QTest::addColumn<bool>("isEmpty");
    {
        auto model = createModel(5);
        QTest::newRow("multiple columns")
                << model << QList<QModelIndex>({ model->index(0, 0), model->index(0, 1) }) << false;
    }
    {
        auto model = createModel(5);
        QTest::newRow("multiple rows")
                << model
                << QList<QModelIndex>(
                           { model->index(0, 0), model->index(1, 0), model->index(2, 0) })
                << false;
    }
    {
        auto model = createModel(5);
        QTest::newRow("hidden rows")
                << model << QList<QModelIndex>({ model->index(3, 0), model->index(4, 0) }) << true;
    }
    {
        auto model = createModel(500);
        QTest::newRow("rows outside viewport")
                << model << QList<QModelIndex>({ model->index(498, 0), model->index(499, 0) })
                << true;
    }
}

void tst_QAbstractItemView::checkIntersectedRect()
{
    QFETCH(QStandardItemModel *, model);
    QFETCH(const QList<QModelIndex>, changedIndexes);
    QFETCH(bool, isEmpty);

    class TableView : public QTableView
    {
    public:
        void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                         const QList<int> &roles = QList<int>()) override
        {
            QTableView::dataChanged(topLeft, bottomRight, roles);
            // we want to check the base class implementation here!
            QAbstractItemViewPrivate *av = static_cast<QAbstractItemViewPrivate*>(qt_widget_private(this));
            m_intersectecRect = av->intersectedRect(av->viewport->rect(), topLeft, bottomRight);
        }
        mutable QRect m_intersectecRect;
    };

    TableView view;
    model->setParent(&view);
    view.setModel(model);
    view.resize(400, 400);
    view.show();
    view.setRowHidden(3, true);
    view.setRowHidden(4, true);
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    view.m_intersectecRect = QRect();
    emit view.model()->dataChanged(changedIndexes.first(), changedIndexes.last());
    if (isEmpty) {
        QVERIFY(view.m_intersectecRect.isEmpty());
    } else {
        const auto parent = changedIndexes.first().parent();
        const int rCount = view.model()->rowCount(parent);
        const int cCount = view.model()->columnCount(parent);
        for (int r = 0; r < rCount; ++r) {
            for (int c = 0; c < cCount; ++c) {
                const QModelIndex &idx = view.model()->index(r, c, parent);
                const auto rect = view.visualRect(idx);
                if (changedIndexes.contains(idx))
                    QVERIFY(view.m_intersectecRect.contains(rect));
                else
                    QVERIFY(!view.m_intersectecRect.contains(rect));
            }
        }
    }
}

void tst_QAbstractItemView::task221955_selectedEditor()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QTreeWidget tree;
    tree.setColumnCount(2);

    tree.addTopLevelItem(new QTreeWidgetItem({"Foo", "1"}));
    tree.addTopLevelItem(new QTreeWidgetItem({"Bar", "2"}));
    tree.addTopLevelItem(new QTreeWidgetItem({"Baz", "3"}));

    QTreeWidgetItem *dummy = new QTreeWidgetItem();
    tree.addTopLevelItem(dummy);
    QPushButton *button = new QPushButton("More...");
    tree.setItemWidget(dummy, 0, button);
    button->setAutoFillBackground(true);  // as recommended in doc

    centerOnScreen(&tree);
    moveCursorAway(&tree);
    tree.show();
    tree.setFocus();
    tree.setCurrentIndex(tree.model()->index(1,0));
    QVERIFY(QTest::qWaitForWindowActive(&tree));

    QVERIFY(! tree.selectionModel()->selectedIndexes().contains(tree.model()->index(3,0)));

    //We set the focus to the button, the index need to be selected
    button->setFocus();
    QTRY_VERIFY(tree.selectionModel()->selectedIndexes().contains(tree.model()->index(3,0)));

    tree.setCurrentIndex(tree.model()->index(1,0));
    QVERIFY(! tree.selectionModel()->selectedIndexes().contains(tree.model()->index(3,0)));

    //Same thing but with the flag NoSelection,   nothing can be selected.
    tree.setFocus();
    tree.setSelectionMode(QAbstractItemView::NoSelection);
    tree.clearSelection();
    QVERIFY(tree.selectionModel()->selectedIndexes().isEmpty());
    button->setFocus();
    QTest::qWait(50);
    QVERIFY(tree.selectionModel()->selectedIndexes().isEmpty());
}

void tst_QAbstractItemView::task250754_fontChange()
{
    QString app_css = qApp->styleSheet();
    qApp->setStyleSheet("/*  */");

    QWidget w;
    QTreeView tree(&w);
    QVBoxLayout *vLayout = new QVBoxLayout(&w);
    vLayout->addWidget(&tree);

    QStandardItemModel *m = new QStandardItemModel(&w);
    for (int i = 0; i < 20; ++i) {
        QStandardItem *item = new QStandardItem(QStringLiteral("Item number ") + QString::number(i));
        for (int j = 0; j < 5; ++j) {
            QStandardItem *child = new QStandardItem(QStringLiteral("Child Item number ") + QString::number(j));
            item->setChild(j, 0, child);
        }
        m->setItem(i, 0, item);
    }
    tree.setModel(m);

    tree.setHeaderHidden(true); // The header is (in certain styles) dpi dependent
    w.resize(160, 350); // Minimum width for windows with frame on Windows 8
    centerOnScreen(&w);
    moveCursorAway(&w);
    w.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QFont font = tree.font();
    font.setPixelSize(10);
    tree.setFont(font);
    QTRY_VERIFY(!tree.verticalScrollBar()->isVisible());

    font.setPixelSize(60);
    tree.setFont(font);
    //now with the huge items, the scrollbar must be visible
    QTRY_VERIFY(tree.verticalScrollBar()->isVisible());

    qApp->setStyleSheet(app_css);
}

void tst_QAbstractItemView::task200665_itemEntered()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");
    {
        // skip if we can't move mouse
        const QPoint cursorPos = QCursor::pos() + QPoint(10, 10);
        QCursor::setPos(cursorPos);
        if (!QTest::qWaitFor([cursorPos] { return QCursor::pos() == cursorPos; }, 500))
            QSKIP("Can't move mouse");
    }
    //we test that view will emit entered
    //when the scrollbar move but not the mouse itself
    QStandardItemModel model(1000, 1);
    QListView view;
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QCursor::setPos(view.geometry().center());
    QTRY_COMPARE(QCursor::pos(), view.geometry().center());
    QSignalSpy spy(&view, &QAbstractItemView::entered);
    view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum());

    QTRY_COMPARE(spy.size(), 1);
}

void tst_QAbstractItemView::task257481_emptyEditor()
{
    QIcon icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);

    QStandardItemModel model;

    model.appendRow(new QStandardItem(icon, QString()));
    model.appendRow(new QStandardItem(icon, "Editor works"));
    model.appendRow(new QStandardItem(QString()));

    QTreeView treeView;
    treeView.setRootIsDecorated(false);
    treeView.setModel(&model);
    centerOnScreen(&treeView);
    moveCursorAway(&treeView);
    treeView.show();
    QVERIFY(QTest::qWaitForWindowExposed(&treeView));

    treeView.edit(model.index(0, 0));
    QList<QLineEdit *> lineEditors = treeView.viewport()->findChildren<QLineEdit *>();
    QCOMPARE(lineEditors.size(), 1);
    QVERIFY(!lineEditors.constFirst()->size().isEmpty());

    treeView.edit(model.index(1, 0));
    lineEditors = treeView.viewport()->findChildren<QLineEdit *>();
    QCOMPARE(lineEditors.size(), 1);
    QVERIFY(!lineEditors.constFirst()->size().isEmpty());

    treeView.edit(model.index(2, 0));
    lineEditors = treeView.viewport()->findChildren<QLineEdit *>();
    QCOMPARE(lineEditors.size(), 1);
    QVERIFY(!lineEditors.constFirst()->size().isEmpty());
}

void tst_QAbstractItemView::shiftArrowSelectionAfterScrolling()
{
    QStandardItemModel model;
    for (int i = 0; i < 10; ++i)
        model.setItem(i, 0, new QStandardItem(QString::number(i)));

    QListView view;
    view.setFixedSize(160, 250); // Minimum width for windows with frame on Windows 8
    view.setFlow(QListView::LeftToRight);
    view.setGridSize(QSize(100, 100));
    view.setSelectionMode(QListView::ExtendedSelection);
    view.setViewMode(QListView::IconMode);
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index0 = model.index(0, 0);
    QModelIndex index1 = model.index(1, 0);
    QModelIndex index9 = model.index(9, 0);

    view.selectionModel()->setCurrentIndex(index0, QItemSelectionModel::NoUpdate);
    QCOMPARE(view.currentIndex(), index0);

    view.scrollTo(index9);
    QTest::keyClick(&view, Qt::Key_Down, Qt::ShiftModifier);

    QCOMPARE(view.currentIndex(), index1);
    QModelIndexList selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 2);
    QVERIFY(selected.contains(index0));
    QVERIFY(selected.contains(index1));
}

void tst_QAbstractItemView::shiftSelectionAfterRubberbandSelection()
{
    QStandardItemModel model;
    for (int i = 0; i < 3; ++i)
        model.setItem(i, 0, new QStandardItem(QString::number(i)));

    QListView view;
    view.setFixedSize(160, 450); // Minimum width for windows with frame on Windows 8
    view.setFlow(QListView::LeftToRight);
    view.setGridSize(QSize(100, 100));
    view.setSelectionMode(QListView::ExtendedSelection);
    view.setViewMode(QListView::IconMode);
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index0 = model.index(0, 0);
    QModelIndex index1 = model.index(1, 0);
    QModelIndex index2 = model.index(2, 0);

    view.setCurrentIndex(index0);
    QCOMPARE(view.currentIndex(), index0);

    // Determine the points where the rubberband selection starts and ends
    QPoint pressPos = view.visualRect(index1).bottomRight() + QPoint(1, 1);
    QPoint releasePos = view.visualRect(index1).center();
    QVERIFY(!view.indexAt(pressPos).isValid());
    QCOMPARE(view.indexAt(releasePos), index1);

    // Select item 1 using a rubberband selection
    // The mouse move event has to be created manually because the QTest framework does not
    // contain a function for mouse moves with buttons pressed
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, pressPos);
    QMouseEvent moveEvent(QEvent::MouseMove, releasePos, view.viewport()->mapToGlobal(releasePos),
                          Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    bool moveEventReceived = qApp->notify(view.viewport(), &moveEvent);
    QVERIFY(moveEventReceived);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, releasePos);
    QCOMPARE(view.currentIndex(), index1);

    // Shift-click item 2
    QPoint item2Pos = view.visualRect(index2).center();
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, item2Pos);
    QCOMPARE(view.currentIndex(), index2);

    // Verify that the selection worked OK
    QModelIndexList selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 2);
    QVERIFY(selected.contains(index1));
    QVERIFY(selected.contains(index2));

    // Select item 0 to revert the selection
    view.setCurrentIndex(index0);
    QCOMPARE(view.currentIndex(), index0);

    // Repeat the same steps as above, but with a Shift-Arrow selection
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, pressPos);
    QMouseEvent moveEvent2(QEvent::MouseMove, releasePos, view.viewport()->mapToGlobal(releasePos),
                           Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    moveEventReceived = qApp->notify(view.viewport(), &moveEvent2);
    QVERIFY(moveEventReceived);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, releasePos);
    QCOMPARE(view.currentIndex(), index1);

    // Press Shift-Down
    QTest::keyClick(&view, Qt::Key_Down, Qt::ShiftModifier);
    QCOMPARE(view.currentIndex(), index2);

    // Verify that the selection worked OK
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 2);
    QVERIFY(selected.contains(index1));
    QVERIFY(selected.contains(index2));
}

void tst_QAbstractItemView::ctrlRubberbandSelection()
{
    QStandardItemModel model;
    for (int i = 0; i < 3; ++i)
        model.setItem(i, 0, new QStandardItem(QString::number(i)));

    QListView view;
    view.setFixedSize(160, 450); // Minimum width for windows with frame on Windows 8
    view.setFlow(QListView::LeftToRight);
    view.setGridSize(QSize(100, 100));
    view.setSelectionMode(QListView::ExtendedSelection);
    view.setViewMode(QListView::IconMode);
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index1 = model.index(1, 0);
    QModelIndex index2 = model.index(2, 0);

    // Select item 1
    view.setCurrentIndex(index1);
    QModelIndexList selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 1);
    QVERIFY(selected.contains(index1));

    // Now press control and draw a rubberband around items 1 and 2.
    // The mouse move event has to be created manually because the QTest framework does not
    // contain a function for mouse moves with buttons pressed.
    QPoint pressPos = view.visualRect(index1).topLeft() - QPoint(1, 1);
    QPoint releasePos = view.visualRect(index2).bottomRight() + QPoint(1, 1);
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::ControlModifier, pressPos);
    QMouseEvent moveEvent(QEvent::MouseMove, releasePos, view.viewport()->mapToGlobal(releasePos),
                          Qt::NoButton, Qt::LeftButton, Qt::ControlModifier);
    bool moveEventReceived = qApp->notify(view.viewport(), &moveEvent);
    QVERIFY(moveEventReceived);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::ControlModifier, releasePos);

    // Verify that item 2 is selected now
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 1);
    QVERIFY(selected.contains(index2));
}

void tst_QAbstractItemView::QTBUG6407_extendedSelection()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QListWidget view;
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    for (int i = 0; i < 50; ++i)
        view.addItem(QString::number(i));

    QFont font = view.font();
    font.setPixelSize(10);
    view.setFont(font);
    view.resize(200,240);
    centerOnScreen(&view);
    moveCursorAway(&view);

    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(&view, QApplication::activeWindow());

    view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum());

    QModelIndex index49 = view.model()->index(49,0);
    QPoint p = view.visualRect(index49).center();
    QVERIFY(view.viewport()->rect().contains(p));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, p);
    QCOMPARE(view.currentIndex(), index49);
    QCOMPARE(view.selectedItems().size(), 1);

    QModelIndex index47 = view.model()->index(47,0);
    p = view.visualRect(index47).center();
    QVERIFY(view.viewport()->rect().contains(p));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, p);
    QCOMPARE(view.currentIndex(), index47);
    QCOMPARE(view.selectedItems().size(), 3); //49, 48, 47;

    QModelIndex index44 = view.model()->index(44,0);
    p = view.visualRect(index44).center();
    QVERIFY(view.viewport()->rect().contains(p));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, p);
    QCOMPARE(view.currentIndex(), index44);
    QCOMPARE(view.selectedItems().size(), 6); //49 .. 44;

}

void tst_QAbstractItemView::QTBUG6753_selectOnSelection()
{
    QTableWidget table(5, 5);
    for (int i = 0; i < table.rowCount(); ++i) {
        for (int j = 0; j < table.columnCount(); ++j)
            table.setItem(i, j, new QTableWidgetItem("choo-be-doo-wah"));
    }

    centerOnScreen(&table);
    moveCursorAway(&table);
    table.show();
    QVERIFY(QTest::qWaitForWindowExposed(&table));

    table.setSelectionMode(QAbstractItemView::ExtendedSelection);
    table.selectAll();
    QVERIFY(QTest::qWaitForWindowExposed(&table));
    QModelIndex item = table.model()->index(1,1);
    QRect itemRect = table.visualRect(item);
    QTest::mouseMove(table.viewport(), itemRect.center());
    QTest::mouseClick(table.viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());

    QCOMPARE(table.selectedItems().size(), 1);
    QCOMPARE(table.selectedItems().first(), table.item(item.row(), item.column()));
}

void tst_QAbstractItemView::testDelegateDestroyEditor()
{
    QTableWidget table(5, 5);
    MyAbstractItemDelegate delegate;
    table.setItemDelegate(&delegate);
    table.edit(table.model()->index(1, 1));
    QAbstractItemView *tv = &table;
    QCOMPARE(delegate.virtualDtorCallCount, 0);
    tv->closeEditor(delegate.openedEditor, QAbstractItemDelegate::NoHint);
    QCOMPARE(delegate.virtualDtorCallCount, 1);
}

void tst_QAbstractItemView::testDelegateDestroyEditorChild()
{
    std::vector<std::unique_ptr<QTreeWidgetItem>> reaper;
    QTreeWidget tree;
    MyAbstractItemDelegate delegate;
    tree.setItemDelegate(&delegate);
    QTreeWidgetItem *topLevel = new QTreeWidgetItem;
    QTreeWidgetItem *levelOne1 = new QTreeWidgetItem(topLevel);
    QTreeWidgetItem *levelTwo1 = new QTreeWidgetItem(levelOne1);
    QTreeWidgetItem *levelOne2 = new QTreeWidgetItem(topLevel);
    QTreeWidgetItem *levelTwo2 = new QTreeWidgetItem(levelOne2);
    tree.insertTopLevelItem(0, topLevel);
    tree.openPersistentEditor(levelOne1);
    tree.openPersistentEditor(levelTwo1);
    tree.openPersistentEditor(levelOne2);
    tree.openPersistentEditor(levelTwo2);
    QCOMPARE(delegate.virtualCtorCallCount, 4);
    levelOne1->removeChild(levelTwo1);
    reaper.emplace_back(levelTwo1);
    QCOMPARE(delegate.virtualDtorCallCount, 1);
    topLevel->removeChild(levelOne2);
    reaper.emplace_back(levelOne2);
    QCOMPARE(delegate.virtualDtorCallCount, 3);
}

void tst_QAbstractItemView::testClickedSignal()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QTableWidget view(5, 5);

    centerOnScreen(&view);
    moveCursorAway(&view);
    view.showNormal();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(&view, QApplication::activeWindow());

    QModelIndex index49 = view.model()->index(49,0);
    QPoint p = view.visualRect(index49).center();
    QVERIFY(view.viewport()->rect().contains(p));

    QSignalSpy clickedSpy(&view, &QTableWidget::clicked);

    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, p);
    QCOMPARE(clickedSpy.size(), 1);

    QTest::mouseClick(view.viewport(), Qt::RightButton, {}, p);
    // We expect that right-clicks do not cause the clicked() signal to
    // be emitted.
    QCOMPARE(clickedSpy.size(), 1);

}

class StateChangeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        Q_UNUSED(index);
        static bool w = true;
        editor->setEnabled(w);
        w = !w;
    }
};

void tst_QAbstractItemView::testChangeEditorState()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    // Test for QTBUG-25370
    TestModel model;
    model.setItem(0, 0, new QStandardItem("a"));
    model.setItem(0, 1, new QStandardItem("b"));

    QTableView view;
    view.setEditTriggers(QAbstractItemView::CurrentChanged);
    view.setItemDelegate(new StateChangeDelegate(&view));
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(&view, QApplication::activeWindow());

    model.emitDataChanged();
    // No segfault - the test passes.
}

void tst_QAbstractItemView::deselectInSingleSelection()
{
    QTableView view;
    QStandardItemModel s;
    s.setRowCount(10);
    s.setColumnCount(10);
    view.setModel(&s);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.setSelectionMode(QAbstractItemView::SingleSelection);
    view.setEditTriggers(QAbstractItemView::NoEditTriggers);
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    // mouse
    QModelIndex index22 = s.index(2, 2);
    QRect rect22 = view.visualRect(index22);
    QPoint clickpos = rect22.center();
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().size(), 1);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, clickpos);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().size(), 0);

    // second click with modifier however does select
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, clickpos);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().size(), 1);

    // keyboard
    QTest::keyClick(&view, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().size(), 1);
    QTest::keyClick(&view, Qt::Key_Space, Qt::ControlModifier);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().size(), 0);

    // second keypress with modifier however does select
    QTest::keyClick(&view, Qt::Key_Space, Qt::ControlModifier);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().size(), 1);
}

void tst_QAbstractItemView::testNoActivateOnDisabledItem()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QTreeView treeView;
    QStandardItemModel model(1, 1);
    QStandardItem *item = new QStandardItem("item");
    model.setItem(0, 0, item);
    item->setFlags(Qt::NoItemFlags);
    treeView.setModel(&model);
    centerOnScreen(&treeView);
    moveCursorAway(&treeView);
    treeView.show();

    QVERIFY(QTest::qWaitForWindowActive(&treeView));

    QSignalSpy activatedSpy(&treeView, &QAbstractItemView::activated);

    // Ensure clicking on a disabled item doesn't emit itemActivated.
    QModelIndex itemIndex = treeView.model()->index(0, 0);
    QPoint clickPos = treeView.visualRect(itemIndex).center();
    QTest::mouseClick(treeView.viewport(), Qt::LeftButton, {}, clickPos);

    QCOMPARE(activatedSpy.size(), 0);
}

void tst_QAbstractItemView::testFocusPolicy_data()
{
    QTest::addColumn<QAbstractItemDelegate*>("delegate");

    QAbstractItemDelegate *styledItemDelegate = new QStyledItemDelegate(this);
    QAbstractItemDelegate *itemDelegate = new QItemDelegate(this);

    QTest::newRow("QStyledItemDelegate") << styledItemDelegate;
    QTest::newRow("QItemDelegate") << itemDelegate;
}

void tst_QAbstractItemView::testFocusPolicy()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QFETCH(QAbstractItemDelegate*, delegate);

    QWidget window;
    QTableView *table = new QTableView(&window);
    table->setItemDelegate(delegate);
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addWidget(table);

    QStandardItemModel model;
    model.setRowCount(10);
    model.setColumnCount(10);
    table->setModel(&model);
    table->setCurrentIndex(model.index(1, 1));

    centerOnScreen(&window);
    moveCursorAway(&window);

    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    // itemview accepts focus => editor is closed => return focus to the itemview
    QPoint clickpos = table->visualRect(model.index(1, 1)).center();
    QTest::mouseDClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);
    QWidget *editor = QApplication::focusWidget();
    QVERIFY(editor);
    QTest::keyClick(editor, Qt::Key_Escape, Qt::NoModifier);
    QCOMPARE(QApplication::focusWidget(), table);

    // itemview doesn't accept focus => editor is closed => clear the focus
    table->setFocusPolicy(Qt::NoFocus);
    QTest::mouseDClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);
    editor = QApplication::focusWidget();
    QVERIFY(editor);
    QTest::keyClick(editor, Qt::Key_Escape, Qt::NoModifier);
    QVERIFY(!QApplication::focusWidget());
}

void tst_QAbstractItemView::QTBUG31411_noSelection()
{
    QWidget window;
    QTableView *table = new QTableView(&window);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addWidget(table);

    QStandardItemModel model;
    model.setRowCount(10);
    model.setColumnCount(10);
    table->setModel(&model);
    table->setCurrentIndex(model.index(1, 1));

    centerOnScreen(&window);
    moveCursorAway(&window);

    window.show();
    QVERIFY(QTest::qWaitForWindowFocused(&window));

    qRegisterMetaType<QItemSelection>();
    QSignalSpy selectionChangeSpy(table->selectionModel(), &QItemSelectionModel::selectionChanged);
    QVERIFY(selectionChangeSpy.isValid());

    QPoint clickpos = table->visualRect(model.index(1, 1)).center();
    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);
    QTest::mouseDClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);

    QPointer<QWidget> editor1 = QApplication::focusWidget();
    QVERIFY(editor1);
    QTest::keyClick(editor1, Qt::Key_Tab, Qt::NoModifier);

    QPointer<QWidget> editor2 = QApplication::focusWidget();
    QVERIFY(editor2);
    QTest::keyClick(editor2, Qt::Key_Escape, Qt::NoModifier);

    QCOMPARE(selectionChangeSpy.size(), 0);
}

void tst_QAbstractItemView::QTBUG39324_settingSameInstanceOfIndexWidget()
{
    QStringListModel model({ "FOO", "bar" });

    QScopedPointer<QTableView> table(new QTableView());
    table->setModel(&model);

    QModelIndex index = model.index(0,0);
    QLineEdit *lineEdit = new QLineEdit();
    table->setIndexWidget(index, lineEdit);
    table->setIndexWidget(index, lineEdit);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    table->show();
}

void tst_QAbstractItemView::shiftSelectionAfterChangingModelContents()
{
    QStringListModel model({ "A", "B", "C", "D", "E", "F" });
    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&model);
    proxyModel.sort(0, Qt::AscendingOrder);
    proxyModel.setDynamicSortFilter(true);

    QPersistentModelIndex indexA(proxyModel.index(0, 0));
    QPersistentModelIndex indexB(proxyModel.index(1, 0));
    QPersistentModelIndex indexC(proxyModel.index(2, 0));
    QPersistentModelIndex indexD(proxyModel.index(3, 0));
    QPersistentModelIndex indexE(proxyModel.index(4, 0));
    QPersistentModelIndex indexF(proxyModel.index(5, 0));

    QListView view;
    view.setModel(&proxyModel);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // Click "C"
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(indexC).center());
    QModelIndexList selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 1);
    QVERIFY(selected.contains(indexC));

    // Insert new item "B1"
    model.insertRow(0);
    model.setData(model.index(0, 0), "B1");

    // Shift-click "D" -> we expect that "C" and "D" are selected
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, view.visualRect(indexD).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 2);
    QVERIFY(selected.contains(indexC));
    QVERIFY(selected.contains(indexD));

    // Click "D"
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(indexD).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 1);
    QVERIFY(selected.contains(indexD));

    // Remove items "B" and "C"
    model.removeRows(proxyModel.mapToSource(indexB).row(), 1);
    model.removeRows(proxyModel.mapToSource(indexC).row(), 1);
    QVERIFY(!indexB.isValid());
    QVERIFY(!indexC.isValid());

    // Shift-click "F" -> we expect that "D", "E", and "F" are selected
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, view.visualRect(indexF).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 3);
    QVERIFY(selected.contains(indexD));
    QVERIFY(selected.contains(indexE));
    QVERIFY(selected.contains(indexF));

    // Move to "A" by pressing "Up" repeatedly
    while (view.currentIndex() != indexA)
        QTest::keyClick(&view, Qt::Key_Up);
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 1);
    QVERIFY(selected.contains(indexA));

    // Change the sort order
    proxyModel.sort(0, Qt::DescendingOrder);

    // Shift-click "F" -> All items should be selected
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, view.visualRect(indexF).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), model.rowCount());

    // Restore the old sort order
    proxyModel.sort(0, Qt::AscendingOrder);

    // Click "D"
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(indexD).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 1);
    QVERIFY(selected.contains(indexD));

    // Insert new item "B2"
    model.insertRow(0);
    model.setData(model.index(0, 0), "B2");

    // Press Shift+Down -> "D" and "E" should be selected.
    QTest::keyClick(&view, Qt::Key_Down, Qt::ShiftModifier);
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 2);
    QVERIFY(selected.contains(indexD));
    QVERIFY(selected.contains(indexE));

    // Click "A" to reset the current selection starting point;
    //then select "D" via QAbstractItemView::setCurrentIndex(const QModelIndex&).
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(indexA).center());
    view.setCurrentIndex(indexD);
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 1);
    QVERIFY(selected.contains(indexD));

    // Insert new item "B3"
    model.insertRow(0);
    model.setData(model.index(0, 0), "B3");

    // Press Shift+Down -> "D" and "E" should be selected.
    QTest::keyClick(&view, Qt::Key_Down, Qt::ShiftModifier);
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 2);
    QVERIFY(selected.contains(indexD));
    QVERIFY(selected.contains(indexE));
}

void tst_QAbstractItemView::QTBUG48968_reentrant_updateEditorGeometries()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QTreeView tree;
    QStandardItemModel *m = new QStandardItemModel(&tree);
    for (int i = 0; i < 10; ++i) {
        QStandardItem *item = new QStandardItem(QString("Item number %1").arg(i));
        item->setEditable(true);
        for (int j = 0; j < 5; ++j) {
            QStandardItem *child = new QStandardItem(QString("Child Item number %1").arg(j));
            item->setChild(j, 0, child);
        }
        m->setItem(i, 0, item);
    }

    tree.setModel(m);
    tree.setRootIsDecorated(false);
    connect(&tree, &QTreeView::doubleClicked,
            &tree, &QTreeView::setRootIndex);
    tree.show();
    QVERIFY(QTest::qWaitForWindowActive(&tree));

    // Trigger editing idx
    QModelIndex idx = m->index(1, 0);
    const QPoint pos = tree.visualRect(idx).center();
    QTest::mouseClick(tree.viewport(), Qt::LeftButton, Qt::NoModifier, pos);
    QTest::mouseDClick(tree.viewport(), Qt::LeftButton, Qt::NoModifier, pos);

    // Add more children to idx
    QStandardItem *item = m->itemFromIndex(idx);
    for (int j = 5; j < 10; ++j) {
        QStandardItem *child = new QStandardItem(QString("Child Item number %1").arg(j));
        item->setChild(j, 0, child);
    }

    // No crash, all fine.
}

class ScrollModeProxyStyle: public QProxyStyle
{
    Q_OBJECT
public:
    ScrollModeProxyStyle(QAbstractItemView::ScrollMode sm, QStyle *style = nullptr)
        : QProxyStyle(style)
        , scrollMode(sm == QAbstractItemView::ScrollPerItem ?
                     QAbstractItemView::ScrollPerPixel : QAbstractItemView::ScrollPerItem)
    { }

    int styleHint(QStyle::StyleHint hint, const QStyleOption *opt,
                  const QWidget *w, QStyleHintReturn *returnData) const override
    {
        if (hint == SH_ItemView_ScrollMode)
            return scrollMode;

        return baseStyle()->styleHint(hint, opt, w, returnData);
    }

    QAbstractItemView::ScrollMode scrollMode;
};

void tst_QAbstractItemView::QTBUG50102_SH_ItemView_ScrollMode()
{
    QListView view;

    // Default comes from the style
    auto styleScrollMode = static_cast<QAbstractItemView::ScrollMode>(
          view.style()->styleHint(QStyle::SH_ItemView_ScrollMode, nullptr, &view, nullptr));
    QCOMPARE(view.verticalScrollMode(), styleScrollMode);
    QCOMPARE(view.horizontalScrollMode(), styleScrollMode);

    // Change style, get new value
    ScrollModeProxyStyle proxyStyle1(styleScrollMode);
    view.setStyle(&proxyStyle1);
    auto proxyScrollMode = static_cast<QAbstractItemView::ScrollMode>(
          view.style()->styleHint(QStyle::SH_ItemView_ScrollMode, nullptr, &view, nullptr));
    QVERIFY(styleScrollMode != proxyScrollMode);
    QCOMPARE(view.verticalScrollMode(), proxyScrollMode);
    QCOMPARE(view.horizontalScrollMode(), proxyScrollMode);

    // Explicitly set vertical, same value
    view.setVerticalScrollMode(proxyScrollMode);
    QCOMPARE(view.verticalScrollMode(), proxyScrollMode);
    QCOMPARE(view.horizontalScrollMode(), proxyScrollMode);

    // Change style, won't change value for vertical, will change for horizontal
    ScrollModeProxyStyle proxyStyle2(proxyScrollMode);
    view.setStyle(&proxyStyle2);
    QCOMPARE(view.verticalScrollMode(), proxyScrollMode);
    QCOMPARE(view.horizontalScrollMode(), styleScrollMode);
}

void tst_QAbstractItemView::QTBUG50535_update_on_new_selection_model()
{
    QStandardItemModel model;
    for (int i = 0; i < 10; ++i)
        model.appendRow(new QStandardItem(QStringLiteral("%1").arg(i)));

    class ListView : public QListView
    {
    public:
        using QListView::QListView;

        void setSelectionModel(QItemSelectionModel *model) override
        {
            m_deselectedMustBeEmpty = !selectionModel() || !model || selectionModel()->model() != model->model();
            QListView::setSelectionModel(model);
            m_deselectedMustBeEmpty = false;
        }
        int m_paintEventsCount = 0;
        bool selectionChangedOk() const { return m_selectionChangedOk; }

    protected:
        bool viewportEvent(QEvent *event) override
        {
            if (event->type() == QEvent::Paint)
                ++m_paintEventsCount;
            return QListView::viewportEvent(event);
        }

        void selectionChanged(const QItemSelection &selected,
                              const QItemSelection &deselected) override
        {
            if (m_deselectedMustBeEmpty && !deselected.isEmpty())
                m_selectionChangedOk = false;

            // Make sure both selections belong to the same model
            const auto selIndexes = selected.indexes();
            const auto deselIndexes = deselected.indexes();
            for (const QModelIndex &nmi : selIndexes) {
                for (const QModelIndex &omi : deselIndexes)
                    m_selectionChangedOk = m_selectionChangedOk && (nmi.model() == omi.model());
            }
            QListView::selectionChanged(selected, deselected);
        }
    private:
        bool m_deselectedMustBeEmpty = false;
        bool m_selectionChangedOk = true;
    };

    // keep the current/selected row in the "low range", i.e. be sure it's visible, otherwise we
    // don't get updates and the test fails.

    ListView view;
    view.setModel(&model);
    view.selectionModel()->setCurrentIndex(model.index(1, 0), QItemSelectionModel::SelectCurrent);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(view.selectionChangedOk());

    QItemSelectionModel selectionModel(&model);
    selectionModel.setCurrentIndex(model.index(2, 0), QItemSelectionModel::Current);

    int oldPaintEventsCount = view.m_paintEventsCount;
    view.setSelectionModel(&selectionModel);
    QTRY_VERIFY(view.m_paintEventsCount > oldPaintEventsCount);
    QVERIFY(view.selectionChangedOk());


    QItemSelectionModel selectionModel2(&model);
    selectionModel2.select(model.index(0, 0), QItemSelectionModel::ClearAndSelect);
    selectionModel2.setCurrentIndex(model.index(1, 0), QItemSelectionModel::Current);

    oldPaintEventsCount = view.m_paintEventsCount;
    view.setSelectionModel(&selectionModel2);
    QTRY_VERIFY(view.m_paintEventsCount > oldPaintEventsCount);
    QVERIFY(view.selectionChangedOk());

    // Tests QAbstractItemView::selectionChanged
    QStandardItemModel model1;
    for (int i = 0; i < 10; ++i)
        model1.appendRow(new QStandardItem(QString::number(i)));
    view.setModel(&model1);

    QItemSelectionModel selectionModel1(&model1);
    selectionModel1.select(model1.index(0, 0), QItemSelectionModel::ClearAndSelect);
    selectionModel1.setCurrentIndex(model1.index(1, 0), QItemSelectionModel::Current);
    view.setSelectionModel(&selectionModel1);
    QVERIFY(view.selectionChangedOk());
}

void tst_QAbstractItemView::testSelectionModelInSyncWithView()
{
    QStandardItemModel model;
    for (int i = 0; i < 10; ++i)
        model.appendRow(new QStandardItem(QString::number(i)));

    class ListView : public QListView
    {
    public:
        using QListView::selectedIndexes;
    };

    ListView view;
    QVERIFY(!view.selectionModel());

    view.setModel(&model);
    QVERIFY(view.selectionModel());
    QVERIFY(view.selectedIndexes().isEmpty());
    QVERIFY(view.selectionModel()->selection().isEmpty());

    view.setCurrentIndex(model.index(0, 0));
    QCOMPARE(view.currentIndex(), model.index(0, 0));
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(0, 0));

    view.selectionModel()->setCurrentIndex(model.index(1, 0), QItemSelectionModel::SelectCurrent);
    QCOMPARE(view.currentIndex(), model.index(1, 0));
    QCOMPARE(view.selectedIndexes(), QModelIndexList() << model.index(1, 0));
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(1, 0));
    QCOMPARE(view.selectionModel()->selection().indexes(), QModelIndexList() << model.index(1, 0));

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QItemSelectionModel selectionModel(&model);
    selectionModel.setCurrentIndex(model.index(2, 0), QItemSelectionModel::Current);

    view.setSelectionModel(&selectionModel);
    QCOMPARE(view.currentIndex(), model.index(2, 0));
    QCOMPARE(view.selectedIndexes(), QModelIndexList());
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(2, 0));
    QCOMPARE(view.selectionModel()->selection().indexes(),  QModelIndexList());


    QItemSelectionModel selectionModel2(&model);
    selectionModel2.select(model.index(0, 0), QItemSelectionModel::ClearAndSelect);
    selectionModel2.setCurrentIndex(model.index(1, 0), QItemSelectionModel::Current);

    view.setSelectionModel(&selectionModel2);
    QCOMPARE(view.currentIndex(), model.index(1, 0));
    QCOMPARE(view.selectedIndexes(), QModelIndexList() << model.index(0, 0));
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(1, 0));
    QCOMPARE(view.selectionModel()->selection().indexes(),  QModelIndexList() << model.index(0, 0));
}

class SetSelectionTestView : public QListView
{
    Q_OBJECT
public:
    using QListView::QListView;
signals:
    void setSelectionCalled(const QRect &rect);

protected:
    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags) override
    {
        emit setSelectionCalled(rect);
        QListView::setSelection(rect, flags);
    }
};

void tst_QAbstractItemView::testClickToSelect()
{
    // This test verifies that the QRect that is passed from QAbstractItemView::mousePressEvent
    // to the virtual method QAbstractItemView::setSelection(const QRect &, SelectionFlags)
    // is the 1x1 rect which conains exactly the clicked pixel if no modifiers are pressed.

    QStringListModel model({ "A", "B", "C" });

    SetSelectionTestView view;
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QSignalSpy spy(&view, &SetSelectionTestView::setSelectionCalled);

    const QModelIndex indexA(model.index(0, 0));
    const QRect visualRectA = view.visualRect(indexA);
    const QPoint centerA = visualRectA.center();

    // Click the center of the visualRect of item "A"
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, centerA);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.back().front().value<QRect>(), QRect(centerA, QSize(1, 1)));

    // Click a point slightly away from the center
    const QPoint nearCenterA = centerA + QPoint(1, 1);
    QVERIFY(visualRectA.contains(nearCenterA));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, nearCenterA);
    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.back().front().value<QRect>(), QRect(nearCenterA, QSize(1, 1)));
}

void tst_QAbstractItemView::testDialogAsEditor()
{
    DialogItemDelegate delegate;

    QStandardItemModel model;
    model.appendRow(new QStandardItem(QStringLiteral("editme")));

    QListView view;
    view.setItemDelegate(&delegate);
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    view.edit(model.index(0,0));

    QVERIFY(QTest::qWaitForWindowExposed(delegate.openedEditor));

    delegate.openedEditor->reject();
    QApplication::processEvents();

    QCOMPARE(delegate.result, QDialog::Rejected);

    view.edit(model.index(0,0));

    QVERIFY(QTest::qWaitForWindowExposed(delegate.openedEditor));

    delegate.openedEditor->accept();
    QApplication::processEvents();

    QCOMPARE(delegate.result, QDialog::Accepted);
}

class HoverItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &opt,
               const QModelIndex &index) const override
    {
        Q_UNUSED(painter);

        if (!(opt.state & QStyle::State_MouseOver)) {

            // We don't want to set m_paintedWithoutHover for any item so check for the item at 0,0
            if (index.row() == 0 && index.column() == 0)
                m_paintedWithoutHover = true;
        }
    }

    mutable bool m_paintedWithoutHover = false;
};

void tst_QAbstractItemView::QTBUG46785_mouseout_hover_state()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    HoverItemDelegate delegate;

    QTableWidget table(5, 5);
    table.verticalHeader()->hide();
    table.horizontalHeader()->hide();
    table.setMouseTracking(true);
    table.setItemDelegate(&delegate);
    centerOnScreen(&table);
    table.show();
    QVERIFY(QTest::qWaitForWindowActive(&table));

    QModelIndex item = table.model()->index(0, 0);
    QRect itemRect = table.visualRect(item);

    // Move the mouse into the center of the item at 0,0 to cause a paint event to occur
    QTest::mouseMove(table.viewport(), itemRect.center());
    QTest::mouseClick(table.viewport(), Qt::LeftButton, {}, itemRect.center());

    delegate.m_paintedWithoutHover = false;

    QTest::mouseMove(table.viewport(), QPoint(-50, 0));

    QTRY_VERIFY(delegate.m_paintedWithoutHover);
}

void tst_QAbstractItemView::testClearModelInClickedSignal()
{
    QStringListModel model({"A", "B"});

    QListView view;
    view.setModel(&model);
    view.show();

    connect(&view, &QListView::clicked, [&view](const QModelIndex &index)
    {
        view.setModel(nullptr);
        QCOMPARE(index.data().toString(), QStringLiteral("B"));
    });

    QModelIndex index = view.model()->index(1, 0);
    QVERIFY(index.isValid());
    QPoint p = view.visualRect(index).center();

    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);

    QCOMPARE(view.model(), nullptr);
}

void tst_QAbstractItemView::inputMethodEnabled_data()
{
    QTest::addColumn<QByteArray>("viewType");
    QTest::addColumn<Qt::ItemFlags>("itemFlags");
    QTest::addColumn<bool>("result");

    const QList<QByteArray> widgets { "QListView", "QTreeView", "QTableView" };
    for (const QByteArray &widget : widgets) {
        QTest::newRow(widget + ": no flags")
            << widget << Qt::ItemFlags(Qt::NoItemFlags) << false;
        QTest::newRow(widget + ": checkable")
            << widget << Qt::ItemFlags(Qt::ItemIsUserCheckable) << false;
        QTest::newRow(widget + ": selectable")
            << widget << Qt::ItemFlags(Qt::ItemIsSelectable) << false;
        QTest::newRow(widget + ": enabled")
            << widget << Qt::ItemFlags(Qt::ItemIsEnabled) << false;
        QTest::newRow(widget + ": selectable|enabled")
            << widget << Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled) << false;
        QTest::newRow(widget + ": editable|enabled")
            << widget << Qt::ItemFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled) << true;
        QTest::newRow(widget + ": editable|enabled|selectable")
            << widget << Qt::ItemFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable) << true;
    }
}

void tst_QAbstractItemView::inputMethodEnabled()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QFETCH(QByteArray, viewType);
    QFETCH(Qt::ItemFlags, itemFlags);
    QFETCH(bool, result);

    QScopedPointer<QAbstractItemView> view(viewFromString(viewType));

    centerOnScreen(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QStandardItemModel *model = new QStandardItemModel(view.data());
    QStandardItem *item = new QStandardItem("first item");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->appendRow(item);

    QStandardItem *secondItem = new QStandardItem("test item");
    secondItem->setFlags(Qt::ItemFlags(itemFlags));
    model->appendRow(secondItem);

    view->setModel(model);

    // Check current changed
    view->setCurrentIndex(model->index(0, 0));
    QVERIFY(!view->testAttribute(Qt::WA_InputMethodEnabled));
    view->setCurrentIndex(model->index(1, 0));
    QCOMPARE(view->testAttribute(Qt::WA_InputMethodEnabled), result);
    view->setCurrentIndex(model->index(0, 0));
    QVERIFY(!view->testAttribute(Qt::WA_InputMethodEnabled));

    // Check focus by switching the activation of the window to force a focus in
    view->setCurrentIndex(model->index(1, 0));
    QVERIFY(QTest::qWaitForWindowActive(view.data()));
    QCOMPARE(view->testAttribute(Qt::WA_InputMethodEnabled), result);

    view->setCurrentIndex(QModelIndex());
    QVERIFY(!view->testAttribute(Qt::WA_InputMethodEnabled));
    QVERIFY(QTest::qWaitForWindowActive(view.data()));
    QModelIndex index = model->index(1, 0);
    QPoint p = view->visualRect(index).center();
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, p);
    if (itemFlags & Qt::ItemIsEnabled)
        QCOMPARE(view->currentIndex(), index);
    QCOMPARE(view->testAttribute(Qt::WA_InputMethodEnabled), result);

    index = model->index(0, 0);
    QVERIFY(QTest::qWaitForWindowActive(view.data()));
    p = view->visualRect(index).center();
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(view->currentIndex(), index);
    QVERIFY(!view->testAttribute(Qt::WA_InputMethodEnabled));

    // There is a case when it goes to the first visible item so we
    // make the flags of the first item match the ones we are testing
    // to check the attribute correctly
    QApplicationPrivate::setActiveWindow(nullptr);
    view->setCurrentIndex(QModelIndex());
    view->reset();
    item->setFlags(Qt::ItemFlags(itemFlags));
    QApplicationPrivate::setActiveWindow(view.data());
    QVERIFY(QTest::qWaitForWindowActive(view.data()));
    QCOMPARE(view->testAttribute(Qt::WA_InputMethodEnabled), result);
}

void tst_QAbstractItemView::currentFollowsIndexWidget_data()
{
    QTest::addColumn<QByteArray>("viewType");

    const QList<QByteArray> widgets { "QListView", "QTreeView", "QTableView" };
    for (const QByteArray &widget : widgets)
        QTest::newRow(widget) << widget;
}

void tst_QAbstractItemView::currentFollowsIndexWidget()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QFETCH(QByteArray, viewType);

    QScopedPointer<QAbstractItemView> view(viewFromString(viewType));

    centerOnScreen(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QStandardItemModel *model = new QStandardItemModel(view.data());
    QStandardItem *item1 = new QStandardItem("first item");
    item1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->appendRow(item1);

    QStandardItem *item2 = new QStandardItem("test item");
    item2->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->appendRow(item2);

    view->setModel(model);
    QLineEdit *lineEdit1 = new QLineEdit;
    QLineEdit *lineEdit2 = new QLineEdit;
    view->setIndexWidget(item1->index(), lineEdit1);
    view->setIndexWidget(item2->index(), lineEdit2);

    lineEdit2->setFocus();
    QTRY_VERIFY(lineEdit2->hasFocus());
    QCOMPARE(view->currentIndex(), item2->index());
    lineEdit1->setFocus();
    QTRY_VERIFY(lineEdit1->hasFocus());
    QCOMPARE(view->currentIndex(), item1->index());
}

class EditorItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                          const QModelIndex &) const override
    {
        openedEditor = new QLineEdit(parent);
        return openedEditor;
    }
    mutable QPointer<QWidget> openedEditor = nullptr;
};

// Testing the case reported in QTBUG-62253.
// When an itemview with an editor that has focus loses focus
// due to a change in the active window then we need to check
// that the itemview gets focus once the activation is back
// on the original window.
void tst_QAbstractItemView::checkFocusAfterActivationChanges_data()
{
    currentFollowsIndexWidget_data();
}

void tst_QAbstractItemView::checkFocusAfterActivationChanges()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QFETCH(QByteArray, viewType);

    const QRect availableGeo = QGuiApplication::primaryScreen()->availableGeometry();
    const int halfWidth = availableGeo.width() / 2;
    QWidget otherTopLevel;
    otherTopLevel.setGeometry(availableGeo.x(), availableGeo.y(),
                              halfWidth, availableGeo.height());
    otherTopLevel.show();

    QWidget w;
    w.setGeometry(availableGeo.x() + halfWidth, availableGeo.y(),
                              halfWidth, availableGeo.height());
    QLineEdit *le = new QLineEdit(&w);
    QAbstractItemView *view = viewFromString(viewType, &w);

    QStandardItemModel model(5, 5);
    view->setModel(&model);
    view->move(0, 50);
    EditorItemDelegate delegate;
    view->setItemDelegate(&delegate);
    w.show();

    QVERIFY(QTest::qWaitForWindowActive(&w));
    QVERIFY(le->hasFocus());

    view->setFocus();
    QVERIFY(view->hasFocus());

    view->edit(model.index(0,0));
    QVERIFY(QTest::qWaitForWindowExposed(delegate.openedEditor));
    QVERIFY(delegate.openedEditor->hasFocus());

    QApplicationPrivate::setActiveWindow(&otherTopLevel);
    otherTopLevel.setFocus();
    QTRY_VERIFY(!delegate.openedEditor);

    QApplicationPrivate::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));
    QVERIFY(view->hasFocus());
}

void tst_QAbstractItemView::dragSelectAfterNewPress()
{
    QStandardItemModel model;
    for (int i = 0; i < 10; ++i) {
        QStandardItem *item = new QStandardItem(QString::number(i));
        model.setItem(i, 0, item);
    }

    QListView view;
    view.setFixedSize(160, 650); // Minimum width for windows with frame on Windows 8
    view.setSelectionMode(QListView::ExtendedSelection);
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index0 = model.index(0, 0);
    QModelIndex index2 = model.index(2, 0);

    view.setCurrentIndex(index0);
    QCOMPARE(view.currentIndex(), index0);

    // Select item 0 using a single click
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier,
                      view.visualRect(index0).center());
    QCOMPARE(view.currentIndex(), index0);

    // Press to select item 2
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::ShiftModifier,
                      view.visualRect(index2).center());
    QCOMPARE(view.currentIndex(), index2);

    // Verify that the selection worked OK
    QModelIndexList selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 3);
    for (int i = 0; i < 2; ++i)
        QVERIFY(selected.contains(model.index(i, 0)));

    QModelIndex index5 = model.index(5, 0);
    const QPoint releasePos = view.visualRect(index5).center();
    // The mouse move event has to be created manually because the QTest framework does not
    // contain a function for mouse moves with buttons pressed
    QMouseEvent moveEvent2(QEvent::MouseMove, releasePos, view.viewport()->mapToGlobal(releasePos),
                           Qt::NoButton, Qt::LeftButton, Qt::ShiftModifier);
    const bool moveEventReceived = qApp->notify(view.viewport(), &moveEvent2);
    QVERIFY(moveEventReceived);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, releasePos);
    QCOMPARE(view.currentIndex(), index5);

    // Verify that the selection worked OK
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.size(), 6);
    for (int i = 0; i < 5; ++i)
        QVERIFY(selected.contains(model.index(i, 0)));
}

void tst_QAbstractItemView::dragWithSecondClick_data()
{
    QTest::addColumn<QByteArray>("viewClass");
    QTest::addColumn<bool>("doubleClick");
    const QList<QByteArray> widgets { "QListView", "QTreeView" };
    for (const QByteArray &widget : widgets) {
        QTest::newRow(widget + ": DoubleClick") << widget << true;
        QTest::newRow(widget + ": Two Single Clicks") << widget << false;
    }
}

// inject the ability to record which indexes get dragged into any QAbstractItemView class
struct DragRecorder
{
    virtual ~DragRecorder() = default;
    bool dragStarted = false;
    QModelIndexList draggedIndexes;
    QAbstractItemView *view;
};

template<class ViewClass>
class DragRecorderView : public ViewClass, public DragRecorder
{
public:
    DragRecorderView()
    { view = this; }
protected:
    void startDrag(Qt::DropActions) override
    {
        draggedIndexes = ViewClass::selectedIndexes();
        dragStarted = true;
    }
};

void tst_QAbstractItemView::dragWithSecondClick()
{
    QFETCH(QByteArray, viewClass);
    QFETCH(bool, doubleClick);

    QStandardItemModel model;
    QStandardItem *parentItem = model.invisibleRootItem();
    for (int i = 0; i < 10; ++i) {
        QStandardItem *item = new QStandardItem(QString("item %0").arg(i));
        item->setDragEnabled(true);
        item->setEditable(false);
        parentItem->appendRow(item);
    }

    std::unique_ptr<DragRecorder> dragRecorder;
    if (viewClass == "QTreeView")
        dragRecorder.reset(new DragRecorderView<QTreeView>);
    else if (viewClass == "QListView")
        dragRecorder.reset(new DragRecorderView<QListView>);

    QAbstractItemView *view = dragRecorder->view;
    view->setModel(&model);
    view->setFixedSize(160, 650); // Minimum width for windows with frame on Windows 8
    view->setSelectionMode(QAbstractItemView::MultiSelection);
    view->setDragDropMode(QAbstractItemView::InternalMove);
    centerOnScreen(view);
    moveCursorAway(view);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));

    QModelIndex index0 = model.index(0, 0);
    QModelIndex index1 = model.index(1, 0);
    // Select item 0 using a single click
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                      view->visualRect(index0).center());
    QCOMPARE(view->currentIndex(), index0);

    if (doubleClick) {
        // press on same item within the double click interval
        QTest::mouseDClick(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                           view->visualRect(index0).center());
    } else {
        // or on different item with a slow second press
        QTest::mousePress(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                          view->visualRect(index1).center());
    }
    // then drag far enough with left button held
    const QPoint dragTo = view->visualRect(index1).center()
                        + QPoint(2 * QApplication::startDragDistance(),
                                 2 * QApplication::startDragDistance());
    QMouseEvent mouseMoveEvent(QEvent::MouseMove, dragTo, view->viewport()->mapToGlobal(dragTo),
                            Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QVERIFY(QApplication::sendEvent(view->viewport(), &mouseMoveEvent));
    // twice since the view will first enter dragging state, then start the drag
    // (not necessary to actually move the mouse)
    QVERIFY(QApplication::sendEvent(view->viewport(), &mouseMoveEvent));
    QVERIFY(dragRecorder->dragStarted);
    QTest::mouseRelease(view->viewport(), Qt::LeftButton, Qt::NoModifier, dragTo);
}

void tst_QAbstractItemView::clickAfterDoubleClick()
{
    QTableWidget view(5, 5);
    view.horizontalHeader()->hide();
    view.verticalHeader()->hide();
    view.setEditTriggers(QAbstractItemView::NoEditTriggers);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    const QModelIndex index = view.model()->index(1, 1);
    QVERIFY(index.isValid());
    const QPoint clickPoint = view.visualRect(index).center();

    // must use the QWindow overloads so that modality is respected
    QWindow *window = view.window()->windowHandle();
    int clickCount = 0;

    connect(&view, &QAbstractItemView::doubleClicked, [&]{
        QDialog dialog(&view);
        dialog.setModal(true);
        QTimer::singleShot(0, [&]{ dialog.close(); });
        dialog.exec();
    });
    connect(&view, &QAbstractItemView::clicked, [&]{
        ++clickCount;
    });

    QTest::mouseClick(window, Qt::LeftButton, {}, clickPoint);
    QCOMPARE(clickCount, 1);
    // generates a click followed by a double click; double click opens
    // dialog that eats second release
    QTest::mouseDClick(window, Qt::LeftButton, {}, clickPoint);
    QCOMPARE(clickCount, 2);
    QTest::mouseClick(window, Qt::LeftButton, {}, clickPoint);
    QCOMPARE(clickCount, 3);
}

void tst_QAbstractItemView::selectionCommand_data()
{
    QTest::addColumn<QAbstractItemView::SelectionMode>("selectionMode");
    QTest::addColumn<Qt::KeyboardModifier>("keyboardModifier");
    QTest::addColumn<QItemSelectionModel::SelectionFlag>("selectionFlag");

    QTest::newRow("NoSelection - NoModifier") << QAbstractItemView::NoSelection << Qt::NoModifier << QItemSelectionModel::NoUpdate;
    QTest::newRow("NoSelection - ShiftModifier") << QAbstractItemView::NoSelection << Qt::ShiftModifier << QItemSelectionModel::NoUpdate;
    QTest::newRow("NoSelection - ControlModifier") << QAbstractItemView::NoSelection << Qt::ControlModifier << QItemSelectionModel::NoUpdate;
    QTest::newRow("NoSelection - AltModifier") << QAbstractItemView::NoSelection << Qt::AltModifier << QItemSelectionModel::NoUpdate;

    QTest::newRow("SingleSelection - NoModifier") << QAbstractItemView::SingleSelection << Qt::NoModifier << QItemSelectionModel::ClearAndSelect;
    QTest::newRow("SingleSelection - ShiftModifier") << QAbstractItemView::SingleSelection << Qt::ShiftModifier << QItemSelectionModel::ClearAndSelect;
    QTest::newRow("SingleSelection - ControlModifier") << QAbstractItemView::SingleSelection << Qt::ControlModifier << QItemSelectionModel::ClearAndSelect;
    QTest::newRow("SingleSelection - AltModifier") << QAbstractItemView::SingleSelection << Qt::AltModifier << QItemSelectionModel::ClearAndSelect;

    QTest::newRow("MultiSelection - NoModifier") << QAbstractItemView::MultiSelection << Qt::NoModifier << QItemSelectionModel::Toggle;
    QTest::newRow("MultiSelection - ShiftModifier") << QAbstractItemView::MultiSelection << Qt::ShiftModifier << QItemSelectionModel::Toggle;
    QTest::newRow("MultiSelection - ControlModifier") << QAbstractItemView::MultiSelection << Qt::ControlModifier << QItemSelectionModel::Toggle;
    QTest::newRow("MultiSelection - AltModifier") << QAbstractItemView::MultiSelection << Qt::AltModifier << QItemSelectionModel::Toggle;

    QTest::newRow("ExtendedSelection - NoModifier") << QAbstractItemView::ExtendedSelection << Qt::NoModifier << QItemSelectionModel::ClearAndSelect;
    QTest::newRow("ExtendedSelection - ShiftModifier") << QAbstractItemView::ExtendedSelection << Qt::ShiftModifier << QItemSelectionModel::SelectCurrent;
    QTest::newRow("ExtendedSelection - ControlModifier") << QAbstractItemView::ExtendedSelection << Qt::ControlModifier << QItemSelectionModel::Toggle;
    QTest::newRow("ExtendedSelection - AltModifier") << QAbstractItemView::ExtendedSelection << Qt::AltModifier << QItemSelectionModel::ClearAndSelect;

    QTest::newRow("ContiguousSelection - NoModifier") << QAbstractItemView::ContiguousSelection << Qt::NoModifier << QItemSelectionModel::ClearAndSelect;
    QTest::newRow("ContiguousSelection - ShiftModifier") << QAbstractItemView::ContiguousSelection << Qt::ShiftModifier << QItemSelectionModel::SelectCurrent;
    QTest::newRow("ContiguousSelection - ControlModifier") << QAbstractItemView::ContiguousSelection << Qt::ControlModifier << QItemSelectionModel::SelectCurrent;
    QTest::newRow("ContiguousSelection - AltModifier") << QAbstractItemView::ContiguousSelection << Qt::AltModifier << QItemSelectionModel::ClearAndSelect;
}

void tst_QAbstractItemView::selectionCommand()
{
    QFETCH(QAbstractItemView::SelectionMode, selectionMode);
    QFETCH(Qt::KeyboardModifier, keyboardModifier);
    QFETCH(QItemSelectionModel::SelectionFlag, selectionFlag);

    QTableView view;
    view.setSelectionMode(selectionMode);
    QTest::keyPress(&view, Qt::Key_A, keyboardModifier);
    QCOMPARE(selectionFlag, view.selectionCommand(QModelIndex(), nullptr));
}

struct SelectionEvent
{
    enum MouseEvent
    { Press, Release, Click, Move };
    constexpr SelectionEvent(MouseEvent type, int r = -1) noexcept
        : eventType(type), row(r) {}
    constexpr SelectionEvent(MouseEvent type, Qt::KeyboardModifiers mod, int r = -1) noexcept
        : eventType(type), keyboardModifiers(mod), row(r) {}
    MouseEvent eventType = Press;
    Qt::KeyboardModifiers keyboardModifiers = Qt::NoModifier;
    int row = -1;
};

void tst_QAbstractItemView::mouseSelection_data()
{
    QTest::addColumn<QAbstractItemView::SelectionMode>("selectionMode");
    QTest::addColumn<bool>("dragEnabled");
    QTest::addColumn<QAbstractItemView::EditTrigger>("editTrigger");
    QTest::addColumn<QList<SelectionEvent>>("selectionEvents");
    QTest::addColumn<QList<int>>("selectedRows");

    // single selection mode - always one row selected, modifiers ignored
    QTest::addRow("Single:Press") << QAbstractItemView::SingleSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1)}
        << QList{1};
    QTest::addRow("Single:Click") << QAbstractItemView::SingleSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent{SelectionEvent::Click, 1}}
        << QList{1};
    QTest::addRow("Single:Press+Drag") << QAbstractItemView::SingleSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1),
                 SelectionEvent{SelectionEvent::Move, 2},
                 SelectionEvent{SelectionEvent::Release}}
        << QList{2};
    QTest::addRow("Single:Shift+Click") << QAbstractItemView::SingleSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent{SelectionEvent::Click, Qt::ShiftModifier, 2}}
        << QList{2};
    QTest::addRow("Single:Press;Ctrl+Press") << QAbstractItemView::SingleSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent{SelectionEvent::Press, 3},
                 SelectionEvent{SelectionEvent::Press, Qt::ControlModifier, 3}}
        << QList{3};
    QTest::addRow("Single:Ctrl+Click") << QAbstractItemView::SingleSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent{SelectionEvent::Click, Qt::ControlModifier, 3}}
        << QList{3};
    QTest::addRow("Single:Click;Ctrl+Click") << QAbstractItemView::SingleSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent{SelectionEvent::Click, 3},
                 SelectionEvent{SelectionEvent::Click, Qt::ControlModifier, 3}}
        << QList<int>{};

    // multi selection mode - selection toggles on press, selection can be drag-extended
    // modifiers ignored
    QTest::addRow("Multi:Press") << QAbstractItemView::MultiSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1)}
        << QList{1};
    QTest::addRow("Multi:Press twice") << QAbstractItemView::MultiSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1),
                 SelectionEvent(SelectionEvent::Press, 1)}
        << QList<int>{};
    QTest::addRow("Multi:Press twice with drag enabled") << QAbstractItemView::MultiSelection << true
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Click, 1),
                 SelectionEvent(SelectionEvent::Press, 1)}
        << QList{1};
    QTest::addRow("Multi:Press and click with drag enabled") << QAbstractItemView::MultiSelection << true
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1),
                 SelectionEvent(SelectionEvent::Click, 1)}
        << QList<int>{};
    QTest::addRow("Multi:Press,Press") << QAbstractItemView::MultiSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 2),
                 SelectionEvent(SelectionEvent::Press, 3)}
        << QList{2, 3};
    QTest::addRow("Multi:Press,Drag") << QAbstractItemView::MultiSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1),
                 SelectionEvent(SelectionEvent::Move, 5),
                 SelectionEvent(SelectionEvent::Release)}
        << QList{1, 2, 3, 4, 5};
    QTest::addRow("Multi:Press,Drag,Deselect") << QAbstractItemView::MultiSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1),
                 SelectionEvent(SelectionEvent::Move, 5),
                 SelectionEvent(SelectionEvent::Release),
                 SelectionEvent(SelectionEvent::Press, 3)}
        << QList{1, 2, 4, 5};
    // drag-select a few indices; then drag-select a larger area that includes the first
    QTest::addRow("Multi:Press,Drag;Surround") << QAbstractItemView::MultiSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 3),
                 SelectionEvent(SelectionEvent::Move, 5),
                 SelectionEvent(SelectionEvent::Release),
                 SelectionEvent(SelectionEvent::Press, 1),
                 SelectionEvent(SelectionEvent::Move, 8),
                 SelectionEvent(SelectionEvent::Release)}
        << QList{1, 2, 3, 4, 5, 6, 7, 8};
    // drag-select a few indices; then try to select more starting with the last -> not working
    QTest::addRow("Multi:Press,Drag;Expand") << QAbstractItemView::MultiSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 3),
                 SelectionEvent(SelectionEvent::Move, 5),
                 SelectionEvent(SelectionEvent::Release),
                 SelectionEvent(SelectionEvent::Press, 5), // this will deselect #5 and not select 6/7/8
                 SelectionEvent(SelectionEvent::Move, 8),
                 SelectionEvent(SelectionEvent::Release)}
        << QList{3, 4};
    // Multi: Press-dragging a selection should not deselect #QTBUG-59888
    QTest::addRow("Multi:Press-Drag selection") << QAbstractItemView::MultiSelection << true
        // with drag'n'drop enabled, we cannot drag a selection
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Click, 2),
                 SelectionEvent(SelectionEvent::Click, 3),
                 SelectionEvent(SelectionEvent::Click, 4),
                 SelectionEvent(SelectionEvent::Click, 5),
                 SelectionEvent(SelectionEvent::Press, 3),
                 // two moves needed because of distance and state logic in QAbstractItemView
                 SelectionEvent(SelectionEvent::Move, 5),
                 SelectionEvent(SelectionEvent::Move, 6)}
        << QList{2, 3, 4, 5};

    // Extended selection: Press selects a single item
    QTest::addRow("Extended:Press") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 3)}
        << QList{3};
    QTest::addRow("Extended:Press twice") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 3),
                 SelectionEvent(SelectionEvent::Press, 3)}
        << QList{3};
    QTest::addRow("Extended:Press,Press") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 2),
                 SelectionEvent(SelectionEvent::Press, 3)}
        << QList{3};
    // Extended selection: press with Ctrl toggles item
    QTest::addRow("Extended:Press,Toggle") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 3),
                 SelectionEvent(SelectionEvent::Click, Qt::ControlModifier, 3)}
        << QList<int>{};
    QTest::addRow("Extended:Press,Add") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1),
                 SelectionEvent(SelectionEvent::Click, Qt::ControlModifier, 3)}
        << QList{1, 3};
    // Extended selection: Shift creates a range between first and last pressed
    QTest::addRow("Extended:Press,Range") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1),
                 SelectionEvent(SelectionEvent::Press, Qt::ShiftModifier, 5)}
        << QList{1, 2, 3, 4, 5};
    QTest::addRow("Extended:Press,Range,Fix Range") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 1),
                 SelectionEvent(SelectionEvent::Press, Qt::ShiftModifier, 5),
                 SelectionEvent(SelectionEvent::Press, Qt::ShiftModifier, 3)}
        << QList{1, 2, 3};
    // Extended: dragging extends the selection
    QTest::addRow("Extended:Press,Drag") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 2),
                 SelectionEvent(SelectionEvent::Move, 5)}
        << QList{2, 3, 4, 5};
    // Extended: Ctrl+Press-dragging extends the selection
    QTest::addRow("Extended:Press,Drag;Ctrl-Press,Drag") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Press, 2),
                 SelectionEvent(SelectionEvent::Move, 5),
                 SelectionEvent(SelectionEvent::Release),
                 SelectionEvent(SelectionEvent::Press, Qt::ControlModifier, 6),
                 SelectionEvent(SelectionEvent::Move, Qt::ControlModifier, 8),
                 SelectionEvent(SelectionEvent::Release, Qt::ControlModifier, 8)}
        << QList{2, 3, 4, 5, 6, 7, 8};
    // Extended: Ctrl+Press-dragging in a selection should not deselect #QTBUG-59888
    QTest::addRow("Extended:Ctrl-Drag selection,no deselect") << QAbstractItemView::ExtendedSelection << true
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Click, 2),
                 SelectionEvent(SelectionEvent::Click, Qt::ShiftModifier, 5),
                 SelectionEvent(SelectionEvent::Press, Qt::ControlModifier, 3),
                 SelectionEvent(SelectionEvent::Move, Qt::ControlModifier, 5),
                 // two moves needed because of distance and state logic in QAbstractItemView
                 SelectionEvent(SelectionEvent::Move, Qt::ControlModifier, 6)}
        << QList{2, 3, 4, 5};
    // Extended: Ctrl+Press-dragging with a selection extends, then drags #QTBUG-59888
    QTest::addRow("Extended:Ctrl-Drag selection") << QAbstractItemView::ExtendedSelection << true
        << QAbstractItemView::NoEditTriggers
        << QList{SelectionEvent(SelectionEvent::Click, 2),
                 SelectionEvent(SelectionEvent::Click, Qt::ShiftModifier, 5),
                 SelectionEvent(SelectionEvent::Press, Qt::ControlModifier, 6),
                 SelectionEvent(SelectionEvent::Move, Qt::ControlModifier, 7),
                 // two moves needed because of distance and state logic in 7QAbstractItemView
                 SelectionEvent(SelectionEvent::Move, Qt::ControlModifier, 8)}
        << QList{2, 3, 4, 5, 6};
    // Extended: when drag is enabled, click with Ctrl toggles item instead of editing # QTBUG-111131
    QTest::addRow("Extended:Click,Toggle,editable") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::SelectedClicked
        << QList{SelectionEvent(SelectionEvent::Click, 3),
                 SelectionEvent(SelectionEvent::Click, Qt::ControlModifier, 3)}
        << QList<int>{};
    QTest::addRow("Extended:Click,Toggle,dragable,editable") << QAbstractItemView::ExtendedSelection << true
        << QAbstractItemView::SelectedClicked
        << QList{SelectionEvent(SelectionEvent::Click, 3),
                 SelectionEvent(SelectionEvent::Click, Qt::ControlModifier, 3)}
        << QList<int>{};
    // Extended: when drag is enabled, click on selected without Ctrl clears before editing
    QTest::addRow("Extended:Range,Click,editable") << QAbstractItemView::ExtendedSelection << false
        << QAbstractItemView::SelectedClicked
        << QList{SelectionEvent(SelectionEvent::Click, 1),
                 SelectionEvent(SelectionEvent::Click, Qt::ShiftModifier, 3),
                 SelectionEvent(SelectionEvent::Click, 2)}
        << QList<int>{2};
    QTest::addRow("Extended:Range,Click,dragable,editable") << QAbstractItemView::ExtendedSelection << true
        << QAbstractItemView::SelectedClicked
        << QList{SelectionEvent(SelectionEvent::Click, 1),
                 SelectionEvent(SelectionEvent::Click, Qt::ShiftModifier, 3),
                 SelectionEvent(SelectionEvent::Click, 2)}
        << QList<int>{2};
}

void tst_QAbstractItemView::mouseSelection()
{
    QFETCH(QAbstractItemView::SelectionMode, selectionMode);
    QFETCH(bool, dragEnabled);
    QFETCH(QAbstractItemView::EditTrigger, editTrigger);
    QFETCH(QList<SelectionEvent>, selectionEvents);
    QFETCH(QList<int>, selectedRows);

    QStandardItemModel model;
    QStandardItem *parentItem = model.invisibleRootItem();
    for (int i = 0; i < 10; ++i) {
        QStandardItem *item = new QStandardItem(QString("item %0").arg(i));
        item->setDragEnabled(dragEnabled);
        item->setEditable(editTrigger != QAbstractItemView::NoEditTriggers);
        parentItem->appendRow(item);
    }

    std::unique_ptr<DragRecorder> dragRecorder(new DragRecorderView<QTreeView>);
    QAbstractItemView *view = dragRecorder->view;
    QVERIFY(view);
    view->setModel(&model);
    view->setDragEnabled(dragEnabled);
    view->setSelectionMode(selectionMode);
    view->setEditTriggers(editTrigger);
    view->show();
    QVERIFY(QTest::qWaitForWindowActive(view));

    Qt::MouseButton buttonDown = Qt::NoButton;
    int targetRow = -1;
    QModelIndex pressedIndex;
    for (const auto &event : std::as_const(selectionEvents)) {
        if (event.row != -1)
            targetRow = event.row;
        const QModelIndex targetIndex = model.index(targetRow, 0);
        const QPoint targetPoint = view->visualRect(targetIndex).center();
        switch (event.eventType) {
        case SelectionEvent::Press:
            if (buttonDown != Qt::NoButton) {
                QTest::mouseRelease(view->viewport(), buttonDown, event.keyboardModifiers,
                                    view->visualRect(pressedIndex).center());
            }
            buttonDown = Qt::LeftButton;
            pressedIndex = model.index(targetRow, 0);
            QTest::mousePress(view->viewport(), buttonDown, event.keyboardModifiers, targetPoint);
            break;
        case SelectionEvent::Release:
            QTest::mouseRelease(view->viewport(), buttonDown, event.keyboardModifiers, targetPoint);
            buttonDown = Qt::NoButton;
            pressedIndex = QModelIndex();
            break;
        case SelectionEvent::Click:
            QTest::mouseClick(view->viewport(), Qt::LeftButton, event.keyboardModifiers, targetPoint);
            buttonDown = Qt::NoButton;
            break;
        case SelectionEvent::Move: {
                QMouseEvent mouseMoveEvent(QEvent::MouseMove, targetPoint,
                                           view->viewport()->mapToGlobal(targetPoint),
                                           Qt::NoButton, buttonDown, event.keyboardModifiers);
                QApplication::sendEvent(view->viewport(), &mouseMoveEvent);
            }
            break;
        }
    }

    QList<int> actualSelected;
    const auto selectedIndexes = dragRecorder->dragStarted
                               ? dragRecorder->draggedIndexes
                               : view->selectionModel()->selectedIndexes();
    for (auto index : selectedIndexes)
        actualSelected << index.row();

    QCOMPARE(actualSelected, selectedRows);
}

/*!
    Make sure that when clicking on empty space in the view, we don't
    unselect the current row.
    QTBUG-105870
*/
void tst_QAbstractItemView::keepSingleSelectionOnEmptyAreaClick()
{
    QListWidget view;
    view.setSelectionMode(QAbstractItemView::SingleSelection);
    QListWidgetItem *lastItem;
    for (int i = 0; i < 5; i++)
        lastItem = new QListWidgetItem("item " + QString::number(i), &view);

    // Make widget large enough so that there is empty area below the last item
    view.setFixedSize(300, 500);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    // Select third row
    view.setCurrentRow(2);

    // Click below the last row
    QPoint targetPoint = view.visualItemRect(lastItem).bottomLeft();
    targetPoint += QPoint(10, 10);

    QTest::mouseClick(view.viewport(), Qt::MouseButton::LeftButton, Qt::NoModifier, targetPoint);

    QCOMPARE(view.currentRow(), 2);
    QVERIFY(view.currentItem()->isSelected());
}

/*!
    Verify that scrolling an autoScroll enabled itemview with a QScroller
    produces a continuous, smooth scroll without any jumping around due to
    the currentItem negotiation between QAbstractItemView and QScroller.
    QTBUG-64543.
*/
void tst_QAbstractItemView::scrollerSmoothScroll()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    QListWidget view;
    view.setAutoScroll(true);
    view.setVerticalScrollMode(QListView::ScrollPerPixel);

    QScroller::grabGesture(view.viewport(), QScroller::TouchGesture);
    QScroller::grabGesture(view.viewport(), QScroller::LeftMouseButtonGesture);

    for (int i = 0; i < 50; i++) {
        QListWidgetItem* item = new QListWidgetItem("item " + QString::number(i), &view);
        // gives items a touch friendly size so that only a few fit into the viewport
        item->setSizeHint(QSize(100,50));
    }

    // make sure we have space for only a few items
    view.setFixedSize(120, 200);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    // we flick up, so we should never scroll back
    int lastScrollPosition = 0;
    bool scrollBack = false;
    connect(view.verticalScrollBar(), &QScrollBar::valueChanged, [&](int value){
        scrollBack |= (value < lastScrollPosition);
        lastScrollPosition = value;
    });

    // start in the middle
    view.scrollToItem(view.item(25));
    QCOMPARE(view.currentItem(), view.item(0));
    QListWidgetItem *pressItem = view.item(23);
    QPoint dragPosition = view.visualRect(view.indexFromItem(pressItem)).center();
    // the mouse press changes the current item temporarily, but the press is delayed
    // by the gesture machinery. this is not what we are testing here, so skip the test
    // if this fails within a reasonable amount of time.
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, dragPosition);
    if (!(QTest::qWaitFor([&]{ return view.currentItem() == pressItem; })))
        QSKIP("Current item didn't change on press, skipping test");

    // QAIV will reset the current item when the scroller changes state to Dragging
    for (int y = 0; y < QApplication::startDragDistance() * 2; ++y) {
        // gesture recognizer needs some throttling
        QTest::qWait(10);
        dragPosition -= QPoint(0, 10);
        const QPoint globalPos = view.viewport()->mapToGlobal(dragPosition);
        QMouseEvent mouseMoveEvent(QEvent::MouseMove, dragPosition, dragPosition, globalPos,
                                    Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(view.viewport(), &mouseMoveEvent);
        QVERIFY(!scrollBack);
    }

    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, dragPosition);
}

/*!
    Verify that starting the editing of an item with a key press while a composing
    input method is active doesn't break the input method. See QTBUG-54848.
*/
void tst_QAbstractItemView::inputMethodOpensEditor_data()
{
    QTest::addColumn<QPoint>("editItem");
    QTest::addColumn<QString>("preedit");
    QTest::addColumn<QString>("commit");

    QTest::addRow("IM accepted") << QPoint(1, 1) << "chang" << QString("长");
    QTest::addRow("IM cancelled") << QPoint(25, 25) << "chang" << QString();
}

void tst_QAbstractItemView::inputMethodOpensEditor()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    QTableWidget tableWidget(50, 50);
    tableWidget.setEditTriggers(QAbstractItemView::AnyKeyPressed);
    for (int r = 0; r < 50; ++r) {
        for (int c = 0; c < 50; ++c )
            tableWidget.setItem(r, c, new QTableWidgetItem(QString("Item %1:%2").arg(r).arg(c)));
    }

    tableWidget.show();
    QVERIFY(QTest::qWaitForWindowActive(&tableWidget));

    const auto sendInputMethodEvent = [](const QString &preeditText, const QString &commitString = {}){
        QInputMethodEvent imEvent(preeditText, {});
        imEvent.setCommitString(commitString);
        QApplication::sendEvent(QApplication::focusWidget(), &imEvent);
    };

    QCOMPARE(QApplication::focusWidget(), &tableWidget);

    QFETCH(QPoint, editItem);
    QFETCH(QString, preedit);
    QFETCH(QString, commit);

    tableWidget.setCurrentCell(editItem.y(), editItem.x());
    const QString orgText = tableWidget.currentItem()->text();
    const QModelIndex currentIndex = tableWidget.currentIndex();
    QCOMPARE(tableWidget.inputMethodQuery(Qt::ImCursorRectangle), tableWidget.visualRect(currentIndex));

    // simulate the start of input via a composing input method
    sendInputMethodEvent(preedit.left(1));
    QCOMPARE(tableWidget.state(), QAbstractItemView::EditingState);
    QLineEdit *editor = tableWidget.findChild<QLineEdit*>();
    QVERIFY(editor);
    QCOMPARE(editor->text(), QString());
    // the focus must remain with the tableWidget, as otherwise the compositing is interrupted
    QCOMPARE(QApplication::focusWidget(), &tableWidget);
    // the item view delegates input method queries to the editor
    const QRect cursorRect = tableWidget.inputMethodQuery(Qt::ImCursorRectangle).toRect();
    QVERIFY(cursorRect.isValid());
    QVERIFY(tableWidget.visualRect(currentIndex).intersects(cursorRect));

    // finish preediting, then commit or cancel the input
    sendInputMethodEvent(preedit);
    sendInputMethodEvent(QString(), commit);
    // editing continues, the editor now has focus
    QCOMPARE(tableWidget.state(), QAbstractItemView::EditingState);
    QVERIFY(editor->hasFocus());
    // finish editing
    QTest::keyClick(editor, Qt::Key_Return);
    if (commit.isEmpty()) {
        // if composition was cancelled, then the item's value is unchanged
        QCOMPARE(tableWidget.currentItem()->text(), orgText);
    } else {
        // otherwise, the item's value is now the commit string
        QTRY_COMPARE(tableWidget.currentItem()->text(), commit);
    }
}

void tst_QAbstractItemView::selectionAutoScrolling_data()
{
    QTest::addColumn<Qt::Orientation>("orientation");
    QTest::addColumn<int>("direction"); // negative or positive

    QTest::addRow("scroll up") << Qt::Vertical << -1;
    QTest::addRow("scroll left") << Qt::Horizontal << -1;
    QTest::addRow("scroll down") << Qt::Vertical << +1;
    QTest::addRow("scroll right") << Qt::Horizontal << +1;
}

void tst_QAbstractItemView::selectionAutoScrolling()
{
    QFETCH(Qt::Orientation, orientation);
    QFETCH(int, direction);

    QListView listview;
    listview.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    listview.setResizeMode(QListView::Fixed);
    listview.setAutoScroll(true);
    listview.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listview.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listview.setSpacing(10);
    listview.setGeometry(0, 0, 500, 500);
    listview.setFrameShape(QFrame::Shape::NoFrame);
    listview.setEditTriggers(QListView::NoEditTriggers);

    QStandardItemModel *listModel = new QStandardItemModel(&listview);
    listview.setModel(listModel);
    listview.setViewMode(QListView::IconMode);
    listview.setSelectionMode(QListView::ExtendedSelection);

    QPixmap pm(50, 50);
    pm.fill(Qt::red);
    for (int i = 0; i < 80; i++) {
        QStandardItem *item = new QStandardItem(pm, QString::number(i));
        listModel->appendRow(item);
    }

    listview.show();
    QVERIFY(QTest::qWaitForWindowExposed(&listview));

    listview.resize(200, 200);
    // scroll to the middle
    listview.verticalScrollBar()->setValue(listview.verticalScrollBar()->maximum() / 2);
    listview.horizontalScrollBar()->setValue(listview.horizontalScrollBar()->maximum() / 2);

    // remove all visible items so that we don't select any items at the edges, as that
    // would scroll the view already
    for (int x = 0; x < listview.viewport()->width(); x += 5) {
        for (int y = 0; y < listview.viewport()->height(); y += 5) {
            const QModelIndex index = listview.indexAt(QPoint(x, y));
            if (index.isValid())
                delete listModel->itemFromIndex(index);
        }
    }
    // remove all items around the edges of the model
    QRect topLeftRect = listview.visualRect(listModel->index(0, 0));
    const QPoint topLeftCenter(topLeftRect.center());
    QPoint bottomRightCenter;
    for (int x = 0; x < listview.horizontalScrollBar()->maximum() + listview.viewport()->width(); x += 5) {
        const QModelIndex index = listview.indexAt(topLeftCenter + QPoint(x, 0));
        if (index.isValid()) {
            delete listModel->itemFromIndex(index);
            bottomRightCenter.rx() = x;
        }
    }
    for (int y = 0; y < listview.verticalScrollBar()->maximum() + listview.viewport()->height(); y += 5) {
        const QModelIndex index = listview.indexAt(topLeftCenter + QPoint(0, y));
        if (index.isValid()) {
            delete listModel->itemFromIndex(index);
            bottomRightCenter.ry() = y;
        }
    }
    for (int x = 0; x < bottomRightCenter.x(); x += 5) {
        const QModelIndex index = listview.indexAt(topLeftCenter + QPoint(x, bottomRightCenter.y()));
        if (index.isValid())
            delete listModel->itemFromIndex(index);
    }
    for (int y = 0; y < bottomRightCenter.y(); y += 5) {
        const QModelIndex index = listview.indexAt(topLeftCenter + QPoint(bottomRightCenter.x(), y));
        if (index.isValid())
            delete listModel->itemFromIndex(index);
    }


    // Simulate multiple select behavior; start in the middle, drag to the edge
    const QPoint pressPoint(listview.viewport()->width() / 2, listview.viewport()->height() / 2);
    QPoint dragPoint = pressPoint;
    if (orientation == Qt::Vertical) {
        dragPoint.rx() += 50;
        dragPoint.ry() = direction > 0 ? listview.viewport()->height() : 0;
    } else {
        dragPoint.rx() = direction > 0 ? listview.viewport()->width() : 0;
        dragPoint.ry() += 50;
    }

    QTest::mousePress(listview.viewport(), Qt::LeftButton, Qt::NoModifier, pressPoint);
    QMouseEvent mmEvent(QEvent::MouseMove, dragPoint, listview.viewport()->mapToGlobal(dragPoint),
                        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(listview.viewport(), &mmEvent); // QTest::mouseMove is useless

    // check that we scrolled to the end
    QScrollBar *scrollBar = orientation == Qt::Vertical
                          ? listview.verticalScrollBar()
                          : listview.horizontalScrollBar();

    if (direction < 0)
        QTRY_COMPARE(scrollBar->value(), 0);
    else
        QTRY_COMPARE(scrollBar->value(), scrollBar->maximum());
    QVERIFY(listview.selectionModel()->selectedIndexes().size() > 0);

    QTest::mouseRelease(listview.viewport(), Qt::LeftButton, Qt::NoModifier, dragPoint);
}
class SpinBoxDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        QSpinBox *spinboxEditor = new QSpinBox(parent);
        return spinboxEditor;
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        if (QSpinBox *spin = qobject_cast<QSpinBox *>(editor)) {
            model->setData(index, spin->value());
        }
    }
};

void tst_QAbstractItemView::testSpinBoxAsEditor_data()
{
    QTest::addColumn<bool>("keyboardTracking");
    QTest::newRow("true") << true;
    QTest::newRow("false")<< false;
}

void tst_QAbstractItemView::testSpinBoxAsEditor()
{
    QFETCH(bool, keyboardTracking);

    QStandardItemModel model(2, 2);
    SpinBoxDelegate delegate;

    QTableView view;
    view.setModel(&model);
    view.setItemDelegate(&delegate);

    view.setCurrentIndex(model.index(0, 1));
    view.openPersistentEditor(model.index(0, 1));
    const QList<QSpinBox *> list = view.viewport()->findChildren<QSpinBox *>();
    QCOMPARE(list.size(), 1);
    QSpinBox *sb = list.first();
    QVERIFY(sb);

    sb->setKeyboardTracking(keyboardTracking);

    centerOnScreen(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_COMPARE(QApplication::focusWidget(), sb);

    QTest::keyClick(sb, Qt::Key_1, Qt::NoModifier);
    QPoint clickpos = view.visualRect(model.index(0, 0)).center();
    QTest::mouseDClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);

    QCOMPARE(model.data(model.index(0, 1)).toInt(), 1);
}

void tst_QAbstractItemView::removeIndexWhileEditing()
{
    QTreeView view;
    QStandardItemModel treeModel;
    auto editableItem1 = new QStandardItem("aa");
    auto editableItem2 = new QStandardItem("ab");
    auto editableItem3 = new QStandardItem("ac");
    auto item = new QStandardItem("a");
    item->appendRow(editableItem1);
    item->appendRow(editableItem2);
    item->appendRow(editableItem3);
    treeModel.setItem(0, 0, item);
    QSortFilterProxyModel filterModel;
    filterModel.setSourceModel(&treeModel);
    view.setModel(&filterModel);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    view.setExpanded(item->index(), true);

    filterModel.setFilterRegularExpression("a.*");

    QTest::failOnWarning(QRegularExpression("QAbstractItemView::closeEditor called with an editor "
                                            "that does not belong to this view"));

    // Verify that we shut editing down cleanly if the index we are editing is
    // filtered out after committing
    {
        const QModelIndex filteredIndex = filterModel.mapFromSource(editableItem1->index());
        QVERIFY(filteredIndex.isValid());
        view.edit(filteredIndex);
        QCOMPARE(view.state(), QAbstractItemView::EditingState);
        QTRY_VERIFY(QApplication::focusWidget());
        QPointer<QLineEdit> lineEdit = qobject_cast<QLineEdit *>(QApplication::focusWidget());
        QVERIFY(lineEdit);
        lineEdit->setText("c");
        QTest::keyClick(lineEdit, Qt::Key_Enter);
        QTRY_VERIFY(!lineEdit);
        QCOMPARE(editableItem1->data(Qt::DisplayRole), "c");
        QCOMPARE(view.state(), QAbstractItemView::NoState);
    }

    // If we change the filter while we edit, then we should clean up state as well
    {
        const QModelIndex filteredIndex = filterModel.mapFromSource(editableItem2->index());
        QVERIFY(filteredIndex.isValid());
        view.edit(filteredIndex);
        QCOMPARE(view.state(), QAbstractItemView::EditingState);
        QTRY_VERIFY(QApplication::focusWidget());
        QPointer<QLineEdit> lineEdit = qobject_cast<QLineEdit *>(QApplication::focusWidget());
        QVERIFY(lineEdit);
        filterModel.setFilterFixedString("c");
        QVERIFY(!filterModel.mapFromSource(editableItem2->index()).isValid());
        QTRY_VERIFY(!lineEdit);
        QCOMPARE(view.state(), QAbstractItemView::NoState);
    }
}

void tst_QAbstractItemView::focusNextOnHide()
{
    QWidget widget;
    QTableWidget table(10, 10);
    table.setTabKeyNavigation(true);
    QLineEdit lineEdit;

    QHBoxLayout layout;
    layout.addWidget(&table);
    layout.addWidget(&lineEdit);
    widget.setLayout(&layout);

    widget.setTabOrder({&table, &lineEdit});

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QTRY_VERIFY(table.hasFocus());
    QCOMPARE(table.currentIndex(), table.model()->index(0, 0));
    QTest::keyPress(&table, Qt::Key_Tab);
    QCOMPARE(table.currentIndex(), table.model()->index(0, 1));

    table.hide();
    QCOMPARE(table.currentIndex(), table.model()->index(0, 1));
    QVERIFY(lineEdit.hasFocus());
}

// Tests for QTBUG-127381, where shift-selecting after calling
// QItemSelectionModel::setCurrentIndex() would produce unexpected results.
void tst_QAbstractItemView::shiftSelectionAfterModelSetCurrentIndex()
{
    QStandardItemModel model(5, 1);
    QTreeView view;
    view.setSelectionBehavior(QAbstractItemView::SelectRows);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.setModel(&model);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index = model.index(0, 0);
    view.setCurrentIndex(index);
    index = model.index(2, 0);
    view.selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    QTest::keyPress(&view, Qt::Key_Down, Qt::ShiftModifier);

    QItemSelection selection = view.selectionModel()->selection();
    QCOMPARE(selection.size(), 1);
    QCOMPARE(selection.first().top(), 2);
    QCOMPARE(selection.first().bottom(), 3);

    QTest::keyPress(&view, Qt::Key_Up, Qt::ShiftModifier);

    selection = view.selectionModel()->selection();
    QCOMPARE(selection.first().top(), 2);
    QCOMPARE(selection.first().bottom(), 2);
}

QTEST_MAIN(tst_QAbstractItemView)
#include "tst_qabstractitemview.moc"
