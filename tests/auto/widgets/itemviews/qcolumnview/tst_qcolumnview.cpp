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
#include <QtTest/QtTest>
#include <QtTest/private/qtesthelpers_p.h>
#include <qitemdelegate.h>
#include <qcolumnview.h>
#include <private/qcolumnviewgrip_p.h>
#include <private/qfilesystemmodel_p.h>
#include <qstringlistmodel.h>
#include <qdebug.h>
#include <qitemdelegate.h>
#include <qscrollbar.h>
#include <private/qcolumnview_p.h>
#include <qscreen.h>

#define ANIMATION_DELAY 300

class tst_QColumnView : public QObject {
  Q_OBJECT

public:
    tst_QColumnView();

private slots:
    void initTestCase();
    void init();
    void rootIndex();
    void grips();
    void isIndexHidden();
    void indexAt();
    void scrollContentsBy_data();
    void scrollContentsBy();
    void scrollTo_data();
    void scrollTo();
    void moveCursor_data();
    void moveCursor();
    void selectAll();
    void clicked();
    void selectedColumns();
    void setSelection();
    void setSelectionModel();
    void visualRegionForSelection();

    void dynamicModelChanges();

    // grip
    void moveGrip_basic();
    void moveGrip_data();
    void moveGrip();
    void doubleClick();
    void gripMoved();

    void preview();
    void swapPreview();
    void sizes();
    void rowDelegate();
    void resize();
    void changeSameColumn();
    void parentCurrentIndex_data();
    void parentCurrentIndex();
    void pullRug_data();
    void pullRug();

protected slots:
    void setPreviewWidget();

private:
    QStandardItemModel m_fakeDirModel;
    QModelIndex m_fakeDirHomeIndex;
};

class TreeModel : public QStandardItemModel
{
public:
    TreeModel()
    {
        for (int j = 0; j < 10; ++j) {
            QStandardItem *parentItem = invisibleRootItem();
            for (int i = 0; i < 10; ++i) {
                const QString iS =  QString::number(i);
                const QString itemText = QLatin1String("item ") + iS;
                QStandardItem *item = new QStandardItem(itemText);
                parentItem->appendRow(item);
                QStandardItem *item2 = new QStandardItem(itemText);
                parentItem->appendRow(item2);
                item2->appendRow(new QStandardItem(itemText));
                parentItem->appendRow(new QStandardItem(QLatin1String("file ") + iS));
                parentItem = item;
            }
        }
    }

    inline QModelIndex firstLevel() { return index(0, 0, QModelIndex()); }
    inline QModelIndex secondLevel() { return index(0, 0, firstLevel()); }
    inline QModelIndex thirdLevel() { return index(0, 0, secondLevel()); }
};

class ColumnView : public QColumnView {

public:
    ColumnView(QWidget *parent = 0) : QColumnView(parent){}

    QList<QPointer<QAbstractItemView> > createdColumns;
    void ScrollContentsBy(int x, int y) {scrollContentsBy(x,y); }
    int HorizontalOffset() const { return horizontalOffset(); }
    void emitClicked() { emit clicked(QModelIndex()); }

    enum PublicCursorAction {
        MoveUp = QAbstractItemView::MoveUp,
        MoveDown = QAbstractItemView::MoveDown,
        MoveLeft = QAbstractItemView::MoveLeft,
        MoveRight = QAbstractItemView::MoveRight,
        MoveHome = QAbstractItemView::MoveHome,
        MoveEnd = QAbstractItemView::MoveEnd,
        MovePageUp = QAbstractItemView::MovePageUp,
        MovePageDown = QAbstractItemView::MovePageDown,
        MoveNext = QAbstractItemView::MoveNext,
        MovePrevious = QAbstractItemView::MovePrevious
    };

    inline QModelIndex MoveCursor(PublicCursorAction ca, Qt::KeyboardModifiers kbm)
        { return QColumnView::moveCursor((CursorAction)ca, kbm); }
    bool IsIndexHidden(const QModelIndex&index) const
        { return isIndexHidden(index); }

    void setSelection(const QRect & rect, QItemSelectionModel::SelectionFlags command )
    {
        QColumnView::setSelection(rect, command);
    }

    // visualRegionForSelection() is protected in QColumnView.
    QRegion getVisualRegionForSelection(const QItemSelection &selection){
        return QColumnView::visualRegionForSelection(selection);
    }
protected:
    QAbstractItemView *createColumn(const QModelIndex &index) {
        QAbstractItemView *view = QColumnView::createColumn(index);
        QPointer<QAbstractItemView> savedView = view;
        createdColumns.append(savedView);
        return view;
    }

};

tst_QColumnView::tst_QColumnView()
{
    QStandardItem *homeItem = populateFakeDirModel(&m_fakeDirModel);
    m_fakeDirHomeIndex = m_fakeDirModel.indexFromItem(homeItem);
}

void tst_QColumnView::initTestCase()
{
    QVERIFY(m_fakeDirHomeIndex.isValid());
    QVERIFY(m_fakeDirModel.rowCount(m_fakeDirHomeIndex) > 1); // Needs some entries in 'home'.
}

void tst_QColumnView::init()
{
    qApp->setLayoutDirection(Qt::LeftToRight);
}

void tst_QColumnView::rootIndex()
{
    ColumnView view;
    // no model
    view.setRootIndex(QModelIndex());

    TreeModel model;
    view.setModel(&model);

    // A top level index
    QModelIndex drive = model.firstLevel();
    QVERIFY(view.visualRect(drive).isValid());
    view.setRootIndex(QModelIndex());
    QCOMPARE(view.HorizontalOffset(), 0);
    QCOMPARE(view.rootIndex(), QModelIndex());
    QVERIFY(view.visualRect(drive).isValid());

    // A item under the rootIndex exists
    QModelIndex home = model.thirdLevel();
    QModelIndex homeFile = model.index(0, 0, home);
    int i = 0;
    while (i < model.rowCount(home) - 1 && !model.hasChildren(homeFile))
        homeFile = model.index(++i, 0, home);
    view.setRootIndex(home);
    QCOMPARE(view.HorizontalOffset(), 0);
    QCOMPARE(view.rootIndex(), home);
    QVERIFY(!view.visualRect(drive).isValid());
    QVERIFY(!view.visualRect(home).isValid());
    if (homeFile.isValid())
        QVERIFY(view.visualRect(homeFile).isValid());

    // set root when there already is one and everything should still be ok
    view.setRootIndex(home);
    view.setCurrentIndex(homeFile);
    view.scrollTo(model.index(0,0, homeFile));
    QCOMPARE(view.HorizontalOffset(), 0);
    QCOMPARE(view.rootIndex(), home);
    QVERIFY(!view.visualRect(drive).isValid());
    QVERIFY(!view.visualRect(home).isValid());
     if (homeFile.isValid())
        QVERIFY(view.visualRect(homeFile).isValid());

    //
    homeFile = model.thirdLevel();
    home = homeFile.parent();
    view.setRootIndex(home);
    view.setCurrentIndex(homeFile);
    view.show();
    i = 0;
    QModelIndex two = model.index(0, 0, homeFile);
    while (i < model.rowCount(homeFile) - 1 && !model.hasChildren(two))
        two = model.index(++i, 0, homeFile);
    qApp->processEvents();
    QTest::qWait(ANIMATION_DELAY);
    view.setCurrentIndex(two);
    view.scrollTo(two);
    QTest::qWait(ANIMATION_DELAY);
    qApp->processEvents();
    QVERIFY(two.isValid());
    QVERIFY(view.HorizontalOffset() != 0);

    view.setRootIndex(homeFile);
    QCOMPARE(view.HorizontalOffset(), 0);
}

void tst_QColumnView::grips()
{
    QColumnView view;
    view.setModel(&m_fakeDirModel);
    QCOMPARE(view.resizeGripsVisible(), true);

    view.setResizeGripsVisible(true);
    QCOMPARE(view.resizeGripsVisible(), true);

    {
        const QObjectList list = view.viewport()->children();
        for (int i = 0 ; i < list.count(); ++i) {
            if (QAbstractItemView *view = qobject_cast<QAbstractItemView*>(list.at(i)))
                QVERIFY(view->cornerWidget() != 0);
        }
    }
    view.setResizeGripsVisible(false);
    QCOMPARE(view.resizeGripsVisible(), false);

    {
        const QObjectList list = view.viewport()->children();
        for (int i = 0 ; i < list.count(); ++i) {
            if (QAbstractItemView *view = qobject_cast<QAbstractItemView*>(list.at(i))) {
                if (view->isVisible())
                    QVERIFY(!view->cornerWidget());
            }
        }
    }

    view.setResizeGripsVisible(true);
    QCOMPARE(view.resizeGripsVisible(), true);
}

void tst_QColumnView::isIndexHidden()
{
    ColumnView view;
    QModelIndex idx;
    QCOMPARE(view.IsIndexHidden(idx), false);
    view.setModel(&m_fakeDirModel);
    QCOMPARE(view.IsIndexHidden(idx), false);
}

void tst_QColumnView::indexAt()
{
    QColumnView view;
    QCOMPARE(view.indexAt(QPoint(0,0)), QModelIndex());
    view.setModel(&m_fakeDirModel);

    QModelIndex homeFile = m_fakeDirModel.index(0, 0, m_fakeDirHomeIndex);
    if (!homeFile.isValid())
        return;
    view.setRootIndex(m_fakeDirHomeIndex);
    QRect rect = view.visualRect(QModelIndex());
    QVERIFY(!rect.isValid());
    rect = view.visualRect(homeFile);
    QVERIFY(rect.isValid());

    QModelIndex child;
    for (int i = 0; i < m_fakeDirModel.rowCount(m_fakeDirHomeIndex); ++i) {
        child = m_fakeDirModel.index(i, 0, m_fakeDirHomeIndex);
        rect = view.visualRect(child);
        QVERIFY(rect.isValid());
        if (i > 0)
            QVERIFY(rect.top() > 0);
        QCOMPARE(view.indexAt(rect.center()), child);

        view.selectionModel()->select(child, QItemSelectionModel::SelectCurrent);
        view.setCurrentIndex(child);
        qApp->processEvents();
        QTest::qWait(200);

        // test that the second row doesn't start at 0
        if (m_fakeDirModel.rowCount(child) > 0) {
            child = m_fakeDirModel.index(0, 0, child);
            QVERIFY(child.isValid());
            rect = view.visualRect(child);
            QVERIFY(rect.isValid());
            QVERIFY(rect.left() > 0);
            QCOMPARE(view.indexAt(rect.center()), child);
            break;
        }
    }
}

void tst_QColumnView::scrollContentsBy_data()
{
    QTest::addColumn<bool>("reverse");
    QTest::newRow("normal") << false;
    QTest::newRow("reverse") << true;
}

void tst_QColumnView::scrollContentsBy()
{
    QFETCH(bool, reverse);
    ColumnView view;
    if (reverse)
        view.setLayoutDirection(Qt::RightToLeft);
    view.ScrollContentsBy(-1, -1);
    view.ScrollContentsBy(0, 0);

    TreeModel model;
    view.setModel(&model);
    view.ScrollContentsBy(0, 0);

    QModelIndex home = model.thirdLevel();
    view.setCurrentIndex(home);
    QTest::qWait(ANIMATION_DELAY);
    view.ScrollContentsBy(0, 0);
}

void tst_QColumnView::scrollTo_data()
{
    QTest::addColumn<bool>("reverse");
    QTest::addColumn<bool>("giveFocus");
    /// ### add test later for giveFocus == true
    QTest::newRow("normal") << false << false;
    QTest::newRow("reverse") << true << false;
}

void tst_QColumnView::scrollTo()
{
    QFETCH(bool, reverse);
    QFETCH(bool, giveFocus);
    QWidget topLevel;
    if (reverse)
        topLevel.setLayoutDirection(Qt::RightToLeft);
    ColumnView view(&topLevel);
    view.resize(200, 200);
    topLevel.show();
    topLevel.activateWindow();
    QTestPrivate::centerOnScreen(&topLevel);
    QVERIFY(QTest::qWaitForWindowActive(&topLevel));

    view.scrollTo(QModelIndex(), QAbstractItemView::EnsureVisible);
    QCOMPARE(view.HorizontalOffset(), 0);

    TreeModel model;
    view.setModel(&model);
    view.scrollTo(QModelIndex(), QAbstractItemView::EnsureVisible);

    QModelIndex home;
    home = model.index(0, 0, home);
    home = model.index(0, 0, home);
    home = model.index(0, 0, home);
    view.scrollTo(home, QAbstractItemView::EnsureVisible);
    view.setRootIndex(home);

    QModelIndex index = model.index(0, 0, home);
    view.scrollTo(index, QAbstractItemView::EnsureVisible);
    QCOMPARE(view.HorizontalOffset(), 0);

    // Embedded requires that at least one widget have focus
    QWidget w;
    w.show();

    QCOMPARE(view.HorizontalOffset(), 0);
    if (giveFocus)
        view.setFocus(Qt::OtherFocusReason);
    else
        view.clearFocus();

    QCOMPARE(view.HorizontalOffset(), 0);
    qApp->processEvents();
    QCOMPARE(view.HorizontalOffset(), 0);
    QTRY_COMPARE(view.hasFocus(), giveFocus);
    // scroll to the right
    int level = 0;
    int last = view.HorizontalOffset();
    while(model.hasChildren(index) && level < 5) {
        view.setCurrentIndex(index);
        QTest::qWait(ANIMATION_DELAY);
        view.scrollTo(index, QAbstractItemView::EnsureVisible);
        QTest::qWait(ANIMATION_DELAY);
        qApp->processEvents();
        index = model.index(0, 0, index);
        level++;
        if (level >= 2) {
            if (!reverse) {
                QTRY_VERIFY(view.HorizontalOffset() < 0);
                qDebug() << "last=" << last
                             << " ; HorizontalOffset= " << view.HorizontalOffset();
                QTRY_VERIFY(last > view.HorizontalOffset());
            } else {
                QTRY_VERIFY(view.HorizontalOffset() > 0);
                QTRY_VERIFY(last < view.HorizontalOffset());
            }
        }
        last = view.HorizontalOffset();
    }

    // scroll to the left
    int start = level;
    while(index.parent().isValid() && index != view.rootIndex()) {
        view.setCurrentIndex(index);
        QTest::qWait(ANIMATION_DELAY);
        view.scrollTo(index, QAbstractItemView::EnsureVisible);
        index = index.parent();
        if (start != level) {
            if (!reverse) {
                QTRY_VERIFY(last < view.HorizontalOffset());
            } else {
                if (last <= view.HorizontalOffset()) {
                    qDebug() << "Test failure. last=" << last
                             << " ; HorizontalOffset= " << view.HorizontalOffset();
                }
                QTRY_VERIFY(last > view.HorizontalOffset());
            }
        }
        level--;
        last = view.HorizontalOffset();
    }
    // It shouldn't automatically steal focus if it doesn't have it
    QTRY_COMPARE(view.hasFocus(), giveFocus);

    // Try scrolling to something that is above the root index
    home = model.index(0, 0, QModelIndex());
    QModelIndex temp = model.index(1, 0, home);
    home = model.index(0, 0, home);
    home = model.index(0, 0, home);
    view.setRootIndex(home);
    view.scrollTo(model.index(0, 0, home));
    QTest::qWait(ANIMATION_DELAY);
    view.scrollTo(temp);
}

void tst_QColumnView::moveCursor_data()
{
    QTest::addColumn<bool>("reverse");
    QTest::newRow("normal") << false;
    QTest::newRow("reverse") << true;
}

void tst_QColumnView::moveCursor()
{
    QFETCH(bool, reverse);
    ColumnView view;
    if (reverse)
        view.setLayoutDirection(Qt::RightToLeft);
    // don't crash
    view.MoveCursor(ColumnView::MoveUp, Qt::NoModifier);

    // don't do anything
    QCOMPARE(view.MoveCursor(ColumnView::MoveEnd, Qt::NoModifier), QModelIndex());

    view.setModel(&m_fakeDirModel);
    QModelIndex ci = view.currentIndex();
    QCOMPARE(view.MoveCursor(ColumnView::MoveUp, Qt::NoModifier), QModelIndex());
    QCOMPARE(view.MoveCursor(ColumnView::MoveDown, Qt::NoModifier), QModelIndex());

    // left at root
    view.setCurrentIndex(m_fakeDirModel.index(0,0));
    ColumnView::PublicCursorAction action = reverse ? ColumnView::MoveRight : ColumnView::MoveLeft;
    QCOMPARE(view.MoveCursor(action, Qt::NoModifier), m_fakeDirModel.index(0,0));

    // left shouldn't move up
    int i = 0;
    ci = m_fakeDirModel.index(0, 0);
    while (i < m_fakeDirModel.rowCount() - 1 && !m_fakeDirModel.hasChildren(ci))
        ci = m_fakeDirModel.index(++i, 0);
    QVERIFY(m_fakeDirModel.hasChildren(ci));
    view.setCurrentIndex(ci);
    action = reverse ? ColumnView::MoveRight : ColumnView::MoveLeft;
    QCOMPARE(view.MoveCursor(action, Qt::NoModifier), ci);

    // now move to the left (i.e. move over one column)
    view.setCurrentIndex(m_fakeDirHomeIndex);
    QCOMPARE(view.MoveCursor(action, Qt::NoModifier), m_fakeDirHomeIndex.parent());

    // right
    action = reverse ? ColumnView::MoveLeft : ColumnView::MoveRight;
    view.setCurrentIndex(ci);
    QModelIndex mc = view.MoveCursor(action, Qt::NoModifier);
    QCOMPARE(mc, m_fakeDirModel.index(0,0, ci));

    // for empty directories (no way to go 'right'), next one should move down
    QModelIndex idx = m_fakeDirModel.index(0, 0, ci);
    const int rowCount = m_fakeDirModel.rowCount(ci);
    while (m_fakeDirModel.hasChildren(idx) && rowCount > idx.row() + 1) {
        idx = idx.sibling(idx.row() + 1, idx.column());
    }
    static const char error[]  = "This test requires an empty directory followed by another directory.";
    QVERIFY2(idx.isValid(), error);
    QVERIFY2(!m_fakeDirModel.hasChildren(idx), error);
    QVERIFY2(idx.row() + 1 < rowCount, error);
    view.setCurrentIndex(idx);
    mc = view.MoveCursor(action, Qt::NoModifier);
    QCOMPARE(mc, idx.sibling(idx.row() + 1, idx.column()));
}

void tst_QColumnView::selectAll()
{
    ColumnView view;
    view.selectAll();

    view.setModel(&m_fakeDirModel);
    view.selectAll();
    QVERIFY(view.selectionModel()->selectedIndexes().count() >= 0);

    view.setCurrentIndex(m_fakeDirHomeIndex);
    view.selectAll();
    QVERIFY(view.selectionModel()->selectedIndexes().count() > 0);

    QModelIndex file;
    for (int i = 0; i < m_fakeDirModel.rowCount(m_fakeDirHomeIndex); ++i)
        if (!m_fakeDirModel.hasChildren(m_fakeDirModel.index(i, 0, m_fakeDirHomeIndex))) {
            file = m_fakeDirModel.index(i, 0, m_fakeDirHomeIndex);
            break;
        }
    view.setCurrentIndex(file);
    view.selectAll();
    QVERIFY(view.selectionModel()->selectedIndexes().count() > 0);

    view.setCurrentIndex(QModelIndex());
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 0);
}

void tst_QColumnView::clicked()
{
    ColumnView view;

    view.setModel(&m_fakeDirModel);
    view.resize(800,300);
    view.show();

    view.setCurrentIndex(m_fakeDirHomeIndex);
    QTest::qWait(ANIMATION_DELAY);

    QModelIndex parent = m_fakeDirHomeIndex.parent();
    QVERIFY(parent.isValid());

    QSignalSpy clickedSpy(&view, SIGNAL(clicked(QModelIndex)));

    QPoint localPoint = view.visualRect(m_fakeDirHomeIndex).center();
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, localPoint);
    QCOMPARE(clickedSpy.count(), 1);
    qApp->processEvents();

    if (sizeof(qreal) != sizeof(double))
        QSKIP("Skipped due to rounding errors");

    for (int i = 0; i < view.createdColumns.count(); ++i) {
        QAbstractItemView *column = view.createdColumns.at(i);
        if (column && column->selectionModel() && (column->rootIndex() == m_fakeDirHomeIndex))
                QVERIFY(column->selectionModel()->selectedIndexes().isEmpty());
    }
}

void tst_QColumnView::selectedColumns()
{
    ColumnView view;
    view.setModel(&m_fakeDirModel);
    view.resize(800,300);
    view.show();

    view.setCurrentIndex(m_fakeDirHomeIndex);

    QTest::qWait(ANIMATION_DELAY);

    for (int i = 0; i < view.createdColumns.count(); ++i) {
        QAbstractItemView *column = view.createdColumns.at(i);
        if (!column)
            continue;
        if (!column->rootIndex().isValid() || column->rootIndex() == m_fakeDirHomeIndex)
            continue;
        QTRY_VERIFY(column->currentIndex().isValid());
    }
}

void tst_QColumnView::setSelection()
{
    ColumnView view;
    // shouldn't do anything, it falls to the columns to handle this
    QRect r;
    view.setSelection(r, QItemSelectionModel::NoUpdate);
}

void tst_QColumnView::setSelectionModel()
{
    ColumnView view;
    view.setModel(&m_fakeDirModel);
    view.show();

    view.setCurrentIndex(m_fakeDirHomeIndex);
    QTest::qWait(ANIMATION_DELAY);

    QItemSelectionModel *selectionModel = new QItemSelectionModel(&m_fakeDirModel);
    view.setSelectionModel(selectionModel);

    bool found = false;
    for (int i = 0; i < view.createdColumns.count(); ++i) {
        if (view.createdColumns.at(i)->selectionModel() == selectionModel) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void tst_QColumnView::visualRegionForSelection()
{
    ColumnView view;
    QItemSelection emptyItemSelection;
    QCOMPARE(QRegion(), view.getVisualRegionForSelection(emptyItemSelection));

    // a region that isn't empty
    view.setModel(&m_fakeDirModel);


    QItemSelection itemSelection(m_fakeDirModel.index(0, 0, m_fakeDirHomeIndex), m_fakeDirModel.index(m_fakeDirModel.rowCount(m_fakeDirHomeIndex) - 1, 0, m_fakeDirHomeIndex));
    QVERIFY(QRegion() != view.getVisualRegionForSelection(itemSelection));
}

void tst_QColumnView::moveGrip_basic()
{
    QColumnView view;
    QColumnViewGrip *grip = new QColumnViewGrip(&view);
    QSignalSpy spy(grip, SIGNAL(gripMoved(int)));
    view.setCornerWidget(grip);
    int oldX = view.width();
    grip->moveGrip(10);
    QCOMPARE(oldX + 10, view.width());
    grip->moveGrip(-10);
    QCOMPARE(oldX, view.width());
    grip->moveGrip(-800);
    QVERIFY(view.width() == 0 || view.width() == 1);
    grip->moveGrip(800);
    view.setMinimumWidth(200);
    grip->moveGrip(-800);
    QCOMPARE(view.width(), 200);
    QCOMPARE(spy.count(), 5);
}

void tst_QColumnView::moveGrip_data()
{
    QTest::addColumn<bool>("reverse");
    QTest::newRow("normal") << false;
    QTest::newRow("reverse") << true;
}

void tst_QColumnView::moveGrip()
{
    QFETCH(bool, reverse);
    QWidget topLevel;
    if (reverse)
        topLevel.setLayoutDirection(Qt::RightToLeft);
    ColumnView view(&topLevel);
    TreeModel model;
    view.setModel(&model);
    QModelIndex home = model.thirdLevel();
    view.setCurrentIndex(home);
    view.resize(640, 200);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowActive(&topLevel));

    int columnNum = view.createdColumns.count() - 2;
    QVERIFY(columnNum >= 0);
    QObjectList list = view.createdColumns[columnNum]->children();
    QColumnViewGrip *grip = 0;
    for (int i = 0; i < list.count(); ++i) {
        if ((grip = qobject_cast<QColumnViewGrip *>(list[i]))) {
            break;
        }
    }
    if (!grip)
        return;

    QAbstractItemView *column = qobject_cast<QAbstractItemView *>(grip->parent());
    int oldX = column->width();
    QCOMPARE(view.columnWidths().value(columnNum), oldX);
    grip->moveGrip(10);
    QCOMPARE(view.columnWidths().value(columnNum), (oldX + (reverse ? -10 : 10)));
}

void tst_QColumnView::doubleClick()
{
    QColumnView view;
    QColumnViewGrip *grip = new QColumnViewGrip(&view);
    QSignalSpy spy(grip, SIGNAL(gripMoved(int)));
    view.setCornerWidget(grip);
    view.resize(200, 200);
    QCOMPARE(view.width(), 200);
    QTest::mouseDClick(grip, Qt::LeftButton);
    QCOMPARE(view.width(), view.sizeHint().width());
    QCOMPARE(spy.count(), 1);
}

void tst_QColumnView::gripMoved()
{
    QColumnView view;
    QColumnViewGrip *grip = new QColumnViewGrip(&view);
    QSignalSpy spy(grip, SIGNAL(gripMoved(int)));
    view.setCornerWidget(grip);
    view.move(300, 300);
    view.resize(200, 200);
    qApp->processEvents();

    int oldWidth = view.width();

    QTest::mousePress(grip, Qt::LeftButton, 0, QPoint(1,1));
    //QTest::mouseMove(grip, QPoint(grip->globalX()+50, y));

    QPoint posNew = QPoint(grip->mapToGlobal(QPoint(1,1)).x() + 65, 0);
    QMouseEvent *event = new QMouseEvent(QEvent::MouseMove, posNew, posNew, Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
    QCoreApplication::postEvent(grip, event);
    QCoreApplication::processEvents();
    QTest::mouseRelease(grip, Qt::LeftButton);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(view.width(), oldWidth + 65);
}

void tst_QColumnView::preview()
{
    QColumnView view;
    QCOMPARE(view.previewWidget(), nullptr);
    TreeModel model;
    view.setModel(&model);
    QCOMPARE(view.previewWidget(), nullptr);
    QModelIndex home = model.index(0, 0);
    QVERIFY(home.isValid());
    QVERIFY(model.hasChildren(home));
    view.setCurrentIndex(home);
    QCOMPARE(view.previewWidget(), nullptr);

    QModelIndex file;
    QVERIFY(model.rowCount(home) > 0);
    for (int i = 0; i < model.rowCount(home); ++i) {
        if (!model.hasChildren(model.index(i, 0, home))) {
            file = model.index(i, 0, home);
            break;
        }
    }
    QVERIFY(file.isValid());
    view.setCurrentIndex(file);
    QVERIFY(view.previewWidget() != (QWidget*)0);

    QWidget *previewWidget = new QWidget(&view);
    view.setPreviewWidget(previewWidget);
    QCOMPARE(view.previewWidget(), previewWidget);
    QVERIFY(previewWidget->parent() != ((QWidget*)&view));
    view.setCurrentIndex(home);

    // previewWidget should be marked for deletion
    QWidget *previewWidget2 = new QWidget(&view);
    view.setPreviewWidget(previewWidget2);
    QCOMPARE(view.previewWidget(), previewWidget2);
}

void tst_QColumnView::swapPreview()
{
    // swap the preview widget in updatePreviewWidget
    QColumnView view;
    QStringList sl;
    sl << QLatin1String("test");
    QStringListModel model(sl);
    view.setModel(&model);
    view.setCurrentIndex(view.indexAt(QPoint(1, 1)));
    connect(&view, SIGNAL(updatePreviewWidget(QModelIndex)),
            this, SLOT(setPreviewWidget()));
    view.setCurrentIndex(view.indexAt(QPoint(1, 1)));
    QTest::qWait(ANIMATION_DELAY);
    qApp->processEvents();
}

void tst_QColumnView::setPreviewWidget()
{
    ((QColumnView*)sender())->setPreviewWidget(new QWidget);
}

void tst_QColumnView::sizes()
{
    QColumnView view;
    QCOMPARE(view.columnWidths().count(), 0);

    QList<int> newSizes;
    newSizes << 10 << 4 << 50 << 6;

    QList<int> visibleSizes;
    view.setColumnWidths(newSizes);
    QCOMPARE(view.columnWidths(), visibleSizes);

    view.setModel(&m_fakeDirModel);
    view.setCurrentIndex(m_fakeDirHomeIndex);

    QList<int> postSizes = view.columnWidths().mid(0, newSizes.count());
    QCOMPARE(postSizes, newSizes.mid(0, postSizes.count()));

    QVERIFY(view.columnWidths().count() > 1);
    QList<int> smallerSizes;
    smallerSizes << 6;
    view.setColumnWidths(smallerSizes);
    QList<int> expectedSizes = newSizes;
    expectedSizes[0] = 6;
    postSizes = view.columnWidths().mid(0, newSizes.count());
    QCOMPARE(postSizes, expectedSizes.mid(0, postSizes.count()));
}

void tst_QColumnView::rowDelegate()
{
    ColumnView view;
    QItemDelegate *d = new QItemDelegate;
    view.setItemDelegateForRow(3, d);

    view.setModel(&m_fakeDirModel);
    for (int i = 0; i < view.createdColumns.count(); ++i) {
        QAbstractItemView *column = view.createdColumns.at(i);
        QCOMPARE(column->itemDelegateForRow(3), (QAbstractItemDelegate*)d);
    }
    delete d;
}

void tst_QColumnView::resize()
{
    QWidget topLevel;
    ColumnView view(&topLevel);
    view.setModel(&m_fakeDirModel);
    view.resize(200, 200);

    topLevel.show();
    view.setCurrentIndex(m_fakeDirHomeIndex);
    QTest::qWait(ANIMATION_DELAY);
    view.resize(200, 300);
    QTest::qWait(ANIMATION_DELAY);

    QVERIFY(view.horizontalScrollBar()->maximum() != 0);
    view.resize(view.horizontalScrollBar()->maximum() * 10, 300);
    QTest::qWait(ANIMATION_DELAY);
    QVERIFY(view.horizontalScrollBar()->maximum() <= 0);
}

void tst_QColumnView::changeSameColumn()
{
    ColumnView view;
    TreeModel model;
    view.setModel(&model);
    QModelIndex second;

    QModelIndex home = model.secondLevel();
    //index(QDir::homePath());
    view.setCurrentIndex(home);
    for (int i = 0; i < model.rowCount(home.parent()); ++i) {
        QModelIndex idx = model.index(i, 0, home.parent());
        if (model.hasChildren(idx) && idx != home) {
            second = idx;
            break;
        }
    }
    QVERIFY(second.isValid());

    QList<QPointer<QAbstractItemView> > old = view.createdColumns;
    view.setCurrentIndex(second);

    QCOMPARE(old, view.createdColumns);
}

void tst_QColumnView::parentCurrentIndex_data()
{
    QTest::addColumn<int>("firstRow");
    QTest::addColumn<int>("secondRow");
    QTest::newRow("down") << 0 << 1;
    QTest::newRow("up") << 1 << 0;
}

void tst_QColumnView::parentCurrentIndex()
{
    QFETCH(int, firstRow);
    QFETCH(int, secondRow);

    ColumnView view;
    TreeModel model;
    view.setModel(&model);
    view.show();

    QModelIndex first;
    QModelIndex second;
    QModelIndex third;
    first = model.index(0, 0, QModelIndex());
    second = model.index(firstRow, 0, first);
    third = model.index(0, 0, second);
    QVERIFY(first.isValid());
    QVERIFY(second.isValid());
    QVERIFY(third.isValid());
    view.setCurrentIndex(third);
    QTRY_COMPARE(view.createdColumns[0]->currentIndex(), first);
    QTRY_COMPARE(view.createdColumns[1]->currentIndex(), second);
    QTRY_COMPARE(view.createdColumns[2]->currentIndex(), third);

    first = model.index(0, 0, QModelIndex());
    second = model.index(secondRow, 0, first);
    third = model.index(0, 0, second);
    QVERIFY(first.isValid());
    QVERIFY(second.isValid());
    QVERIFY(third.isValid());
    view.setCurrentIndex(third);
    QTRY_COMPARE(view.createdColumns[0]->currentIndex(), first);
    QTRY_COMPARE(view.createdColumns[1]->currentIndex(), second);

    // The next two lines should be removed when QTBUG-22707 is resolved.
    QEXPECT_FAIL("", "QTBUG-22707", Abort);
    QVERIFY(view.createdColumns[2]);

    QTRY_COMPARE(view.createdColumns[2]->currentIndex(), third);
}

void tst_QColumnView::pullRug_data()
{
    QTest::addColumn<bool>("removeModel");
    QTest::newRow("model") << true;
    QTest::newRow("index") << false;
}

void tst_QColumnView::pullRug()
{
    QFETCH(bool, removeModel);
    ColumnView view;
    TreeModel model;
    view.setModel(&model);
    QModelIndex home = model.thirdLevel();
    view.setCurrentIndex(home);
    if (removeModel)
        view.setModel(0);
    else
        view.setCurrentIndex(QModelIndex());
    QTest::qWait(ANIMATION_DELAY);
    // don't crash
}

void tst_QColumnView::dynamicModelChanges()
{
    struct MyItemDelegate : public QItemDelegate
    {
        void paint(QPainter *painter,
                   const QStyleOptionViewItem &option,
                   const QModelIndex &index) const
        {
            paintedIndexes += index;
            QItemDelegate::paint(painter, option, index);
        }

        mutable QSet<QModelIndex> paintedIndexes;

    } delegate;;
    QStandardItemModel model;
    ColumnView view;
    view.setModel(&model);
    view.setItemDelegate(&delegate);
    QTestPrivate::centerOnScreen(&view);
    view.show();

    QStandardItem *item = new QStandardItem(QLatin1String("item"));
    model.appendRow(item);

    QVERIFY(QTest::qWaitForWindowExposed(&view)); //let the time for painting to occur
    QTRY_COMPARE(delegate.paintedIndexes.count(), 1);
    QCOMPARE(*delegate.paintedIndexes.begin(), model.index(0,0));


}


QTEST_MAIN(tst_QColumnView)
#include "tst_qcolumnview.moc"

