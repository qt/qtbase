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

#include <QColumnView>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QTest>
#include <QtTest/private/qtesthelpers_p.h>
#include <QtWidgets/private/qcolumnviewgrip_p.h>
#include "../../../../shared/fakedirmodel.h"

#define ANIMATION_DELAY 300

class tst_QColumnView : public QObject
{
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
    Q_OBJECT
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

class ColumnView : public QColumnView
{
    Q_OBJECT
public:
    using QColumnView::QColumnView;
    using QColumnView::horizontalOffset;
    using QColumnView::clicked;
    using QColumnView::isIndexHidden;
    using QColumnView::moveCursor;
    using QColumnView::scrollContentsBy;
    using QColumnView::setSelection;
    using QColumnView::visualRegionForSelection;

    friend class tst_QColumnView;

    QVector<QPointer<QAbstractItemView>> createdColumns;

protected:
    QAbstractItemView *createColumn(const QModelIndex &index) override
    {
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
    QGuiApplication::setLayoutDirection(Qt::LeftToRight);
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
    QCOMPARE(view.horizontalOffset(), 0);
    QCOMPARE(view.rootIndex(), QModelIndex());
    QVERIFY(view.visualRect(drive).isValid());

    // A item under the rootIndex exists
    QModelIndex home = model.thirdLevel();
    QModelIndex homeFile = model.index(0, 0, home);
    int i = 0;
    while (i < model.rowCount(home) - 1 && !model.hasChildren(homeFile))
        homeFile = model.index(++i, 0, home);
    view.setRootIndex(home);
    QCOMPARE(view.horizontalOffset(), 0);
    QCOMPARE(view.rootIndex(), home);
    QVERIFY(!view.visualRect(drive).isValid());
    QVERIFY(!view.visualRect(home).isValid());
    if (homeFile.isValid())
        QVERIFY(view.visualRect(homeFile).isValid());

    // set root when there already is one and everything should still be ok
    view.setRootIndex(home);
    view.setCurrentIndex(homeFile);
    view.scrollTo(model.index(0,0, homeFile));
    QCOMPARE(view.horizontalOffset(), 0);
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
    QTest::qWait(ANIMATION_DELAY);
    view.setCurrentIndex(two);
    view.scrollTo(two);
    QTest::qWait(ANIMATION_DELAY);
    QVERIFY(two.isValid());
    QVERIFY(view.horizontalOffset() != 0);

    view.setRootIndex(homeFile);
    QCOMPARE(view.horizontalOffset(), 0);
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
        for (QObject *obj : list) {
            if (QAbstractItemView *view = qobject_cast<QAbstractItemView*>(obj))
                QVERIFY(view->cornerWidget() != nullptr);
        }
    }
    view.setResizeGripsVisible(false);
    QCOMPARE(view.resizeGripsVisible(), false);

    {
        const QObjectList list = view.viewport()->children();
        for (QObject *obj : list) {
            if (QAbstractItemView *view = qobject_cast<QAbstractItemView*>(obj)) {
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
    QCOMPARE(view.isIndexHidden(idx), false);
    view.setModel(&m_fakeDirModel);
    QCOMPARE(view.isIndexHidden(idx), false);
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
    view.scrollContentsBy(-1, -1);
    view.scrollContentsBy(0, 0);

    TreeModel model;
    view.setModel(&model);
    view.scrollContentsBy(0, 0);

    QModelIndex home = model.thirdLevel();
    view.setCurrentIndex(home);
    QTest::qWait(ANIMATION_DELAY);
    view.scrollContentsBy(0, 0);
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
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

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
    QCOMPARE(view.horizontalOffset(), 0);

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
    QCOMPARE(view.horizontalOffset(), 0);

    // Embedded requires that at least one widget have focus
    QWidget w;
    w.show();

    QCOMPARE(view.horizontalOffset(), 0);
    if (giveFocus)
        view.setFocus(Qt::OtherFocusReason);
    else
        view.clearFocus();

    QCOMPARE(view.horizontalOffset(), 0);
    QCoreApplication::processEvents();
    QCOMPARE(view.horizontalOffset(), 0);
    QTRY_COMPARE(view.hasFocus(), giveFocus);
    // scroll to the right
    int level = 0;
    int last = view.horizontalOffset();
    while (model.hasChildren(index) && level < 5) {
        view.setCurrentIndex(index);
        QTest::qWait(ANIMATION_DELAY);
        view.scrollTo(index, QAbstractItemView::EnsureVisible);
        QTest::qWait(ANIMATION_DELAY);
        index = model.index(0, 0, index);
        level++;
        if (level >= 2) {
            if (!reverse) {
                QTRY_VERIFY(view.horizontalOffset() < 0);
                qDebug() << "last=" << last
                             << " ; horizontalOffset= " << view.horizontalOffset();
                QTRY_VERIFY(last > view.horizontalOffset());
            } else {
                QTRY_VERIFY(view.horizontalOffset() > 0);
                QTRY_VERIFY(last < view.horizontalOffset());
            }
        }
        last = view.horizontalOffset();
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
                QTRY_VERIFY(last < view.horizontalOffset());
            } else {
                if (last <= view.horizontalOffset()) {
                    qDebug() << "Test failure. last=" << last
                             << " ; horizontalOffset= " << view.horizontalOffset();
                }
                QTRY_VERIFY(last > view.horizontalOffset());
            }
        }
        level--;
        last = view.horizontalOffset();
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
    view.moveCursor(ColumnView::MoveUp, Qt::NoModifier);

    // don't do anything
    QCOMPARE(view.moveCursor(ColumnView::MoveEnd, Qt::NoModifier), QModelIndex());

    view.setModel(&m_fakeDirModel);
    QModelIndex ci = view.currentIndex();
    QCOMPARE(view.moveCursor(ColumnView::MoveUp, Qt::NoModifier), QModelIndex());
    QCOMPARE(view.moveCursor(ColumnView::MoveDown, Qt::NoModifier), QModelIndex());

    // left at root
    view.setCurrentIndex(m_fakeDirModel.index(0,0));
    ColumnView::CursorAction action = reverse ? ColumnView::MoveRight : ColumnView::MoveLeft;
    QCOMPARE(view.moveCursor(action, Qt::NoModifier), m_fakeDirModel.index(0,0));

    // left shouldn't move up
    int i = 0;
    ci = m_fakeDirModel.index(0, 0);
    while (i < m_fakeDirModel.rowCount() - 1 && !m_fakeDirModel.hasChildren(ci))
        ci = m_fakeDirModel.index(++i, 0);
    QVERIFY(m_fakeDirModel.hasChildren(ci));
    view.setCurrentIndex(ci);
    action = reverse ? ColumnView::MoveRight : ColumnView::MoveLeft;
    QCOMPARE(view.moveCursor(action, Qt::NoModifier), ci);

    // now move to the left (i.e. move over one column)
    view.setCurrentIndex(m_fakeDirHomeIndex);
    QCOMPARE(view.moveCursor(action, Qt::NoModifier), m_fakeDirHomeIndex.parent());

    // right
    action = reverse ? ColumnView::MoveLeft : ColumnView::MoveRight;
    view.setCurrentIndex(ci);
    QModelIndex mc = view.moveCursor(action, Qt::NoModifier);
    QCOMPARE(mc, m_fakeDirModel.index(0,0, ci));

    // for empty directories (no way to go 'right'), next one should move down
    QModelIndex idx = m_fakeDirModel.index(0, 0, ci);
    const int rowCount = m_fakeDirModel.rowCount(ci);
    while (m_fakeDirModel.hasChildren(idx) && rowCount > idx.row() + 1)
        idx = idx.sibling(idx.row() + 1, idx.column());
    static const char error[]  = "This test requires an empty directory followed by another directory.";
    QVERIFY2(idx.isValid(), error);
    QVERIFY2(!m_fakeDirModel.hasChildren(idx), error);
    QVERIFY2(idx.row() + 1 < rowCount, error);
    view.setCurrentIndex(idx);
    mc = view.moveCursor(action, Qt::NoModifier);
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
    for (int i = 0; i < m_fakeDirModel.rowCount(m_fakeDirHomeIndex); ++i) {
        if (!m_fakeDirModel.hasChildren(m_fakeDirModel.index(i, 0, m_fakeDirHomeIndex))) {
            file = m_fakeDirModel.index(i, 0, m_fakeDirHomeIndex);
            break;
        }
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
    view.resize(800, 300);
    view.show();

    view.setCurrentIndex(m_fakeDirHomeIndex);
    QTest::qWait(ANIMATION_DELAY);

    QModelIndex parent = m_fakeDirHomeIndex.parent();
    QVERIFY(parent.isValid());

    QSignalSpy clickedSpy(&view, &QAbstractItemView::clicked);

    QPoint localPoint = view.visualRect(m_fakeDirHomeIndex).center();
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, localPoint);
    QCOMPARE(clickedSpy.count(), 1);
    QCoreApplication::processEvents();

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
    QCOMPARE(QRegion(), view.visualRegionForSelection(emptyItemSelection));

    // a region that isn't empty
    view.setModel(&m_fakeDirModel);


    QItemSelection itemSelection(m_fakeDirModel.index(0, 0, m_fakeDirHomeIndex), m_fakeDirModel.index(m_fakeDirModel.rowCount(m_fakeDirHomeIndex) - 1, 0, m_fakeDirHomeIndex));
    QVERIFY(QRegion() != view.visualRegionForSelection(itemSelection));
}

void tst_QColumnView::moveGrip_basic()
{
    QColumnView view;
    QColumnViewGrip *grip = new QColumnViewGrip(&view);
    QSignalSpy spy(grip, &QColumnViewGrip::gripMoved);
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
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

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
    const QObjectList list = view.createdColumns[columnNum]->children();
    QColumnViewGrip *grip = nullptr;
    for (QObject *obj : list) {
        if ((grip = qobject_cast<QColumnViewGrip *>(obj)))
            break;
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
    QSignalSpy spy(grip, &QColumnViewGrip::gripMoved);
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
    QSignalSpy spy(grip, &QColumnViewGrip::gripMoved);
    view.setCornerWidget(grip);
    view.move(300, 300);
    view.resize(200, 200);
    QCoreApplication::processEvents();

    int oldWidth = view.width();

    QTest::mousePress(grip, Qt::LeftButton, {}, QPoint(1, 1));
    //QTest::mouseMove(grip, QPoint(grip->globalX()+50, y));

    QPoint posNew = QPoint(grip->mapToGlobal(QPoint(1, 1)).x() + 65, 0);
    QMouseEvent *event = new QMouseEvent(QEvent::MouseMove, posNew, posNew, Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
    QCoreApplication::postEvent(grip, event);
    QCoreApplication::processEvents();
    QTest::mouseRelease(grip, Qt::LeftButton);

    QTRY_COMPARE(spy.count(), 1);
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
    QVERIFY(view.previewWidget() != nullptr);

    QWidget *previewWidget = new QWidget(&view);
    view.setPreviewWidget(previewWidget);
    QCOMPARE(view.previewWidget(), previewWidget);
    QVERIFY(previewWidget->parent() != &view);
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
    QStringListModel model({ QLatin1String("test") });
    view.setModel(&model);
    view.setCurrentIndex(view.indexAt(QPoint(1, 1)));
    connect(&view, &QColumnView::updatePreviewWidget,
            this, &tst_QColumnView::setPreviewWidget);
    view.setCurrentIndex(view.indexAt(QPoint(1, 1)));
    QTest::qWait(ANIMATION_DELAY);
    QCoreApplication::processEvents();
}

void tst_QColumnView::setPreviewWidget()
{
    auto ptr = qobject_cast<QColumnView *>(sender());
    QVERIFY(ptr);
    ptr->setPreviewWidget(new QWidget);
}

void tst_QColumnView::sizes()
{
    QColumnView view;
    QCOMPARE(view.columnWidths().count(), 0);

    const QList<int> newSizes{ 10, 4, 50, 6 };

    QList<int> visibleSizes;
    view.setColumnWidths(newSizes);
    QCOMPARE(view.columnWidths(), visibleSizes);

    view.setModel(&m_fakeDirModel);
    view.setCurrentIndex(m_fakeDirHomeIndex);

    QList<int> postSizes = view.columnWidths().mid(0, newSizes.count());
    QCOMPARE(postSizes, newSizes.mid(0, postSizes.count()));

    QVERIFY(view.columnWidths().count() > 1);
    QList<int> smallerSizes{ 6 };
    view.setColumnWidths(smallerSizes);
    QList<int> expectedSizes = newSizes;
    expectedSizes[0] = 6;
    postSizes = view.columnWidths().mid(0, newSizes.count());
    QCOMPARE(postSizes, expectedSizes.mid(0, postSizes.count()));
}

void tst_QColumnView::rowDelegate()
{
    ColumnView view;
    QStyledItemDelegate *d = new QStyledItemDelegate;
    view.setItemDelegateForRow(3, d);

    view.setModel(&m_fakeDirModel);
    for (int i = 0; i < view.createdColumns.count(); ++i) {
        QAbstractItemView *column = view.createdColumns.at(i);
        QCOMPARE(column->itemDelegateForRow(3), d);
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

    const auto old = view.createdColumns;
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

#ifndef Q_OS_WINRT
    // The next two lines should be removed when QTBUG-22707 is resolved.
    QEXPECT_FAIL("", "QTBUG-22707", Abort);
#endif
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
        view.setModel(nullptr);
    else
        view.setCurrentIndex(QModelIndex());
    QTest::qWait(ANIMATION_DELAY);
    // don't crash
}

void tst_QColumnView::dynamicModelChanges()
{
    struct MyItemDelegate : public QStyledItemDelegate
    {
        void paint(QPainter *painter,
                   const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
        {
            paintedIndexes += index;
            QStyledItemDelegate::paint(painter, option, index);
        }

        mutable QSet<QModelIndex> paintedIndexes;

    } delegate;
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

