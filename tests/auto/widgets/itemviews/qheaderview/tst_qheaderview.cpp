/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
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

#include <QDesktopWidget>
#include <QHeaderView>
#include <QProxyStyle>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTableView>
#include <QTest>
#include <QTreeWidget>
#include <QtWidgets/private/qheaderview_p.h>

using BoolList = QVector<bool>;
using IntList = QVector<int>;
using ResizeVec = QVector<QHeaderView::ResizeMode>;

class TestStyle : public QProxyStyle
{
    Q_OBJECT
public:
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override
    {
        if (element == CE_HeaderSection) {
            if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option))
                lastPosition = header->position;
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }
    mutable QStyleOptionHeader::SectionPosition lastPosition = QStyleOptionHeader::Beginning;
};

class protected_QHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    protected_QHeaderView(Qt::Orientation orientation) : QHeaderView(orientation)
    {
        resizeSections();
    }

    void testEvent();
    void testhorizontalOffset();
    void testverticalOffset();
    void testVisualRegionForSelection();
    friend class tst_QHeaderView;
};

class XResetModel : public QStandardItemModel
{
    Q_OBJECT
public:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override
    {
        blockSignals(true);
        bool r = QStandardItemModel::removeRows(row, count, parent);
        blockSignals(false);
        beginResetModel();
        endResetModel();
        return r;
    }
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override
    {
        blockSignals(true);
        bool r = QStandardItemModel::insertRows(row, count, parent);
        blockSignals(false);
        beginResetModel();
        endResetModel();
        return r;
    }
};

class tst_QHeaderView : public QObject
{
    Q_OBJECT

public:
    tst_QHeaderView();
    static void initMain();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void getSetCheck();
    void visualIndex();

    void visualIndexAt_data();
    void visualIndexAt();

    void noModel();
    void emptyModel();
    void removeRows();
    void removeCols();

    void clickable();
    void movable();
    void hidden();
    void stretch();

    void sectionSize_data();
    void sectionSize();

    void length();
    void offset();
    void sectionSizeHint();
    void logicalIndex();
    void logicalIndexAt();
    void swapSections();

    void moveSection_data();
    void moveSection();

    void resizeMode();

    void resizeSection_data();
    void resizeSection();

    void resizeAndMoveSection_data();
    void resizeAndMoveSection();
    void resizeHiddenSection_data();
    void resizeHiddenSection();
    void resizeAndInsertSection_data();
    void resizeAndInsertSection();
    void resizeWithResizeModes_data();
    void resizeWithResizeModes();
    void moveAndInsertSection_data();
    void moveAndInsertSection();
    void highlightSections();
    void showSortIndicator();
    void sortIndicatorTracking();
    void removeAndInsertRow();
    void unhideSection();
    void testEvent();
    void headerDataChanged();
    void currentChanged();
    void horizontalOffset();
    void verticalOffset();
    void stretchSectionCount();
    void hiddenSectionCount();
    void focusPolicy();
    void moveSectionAndReset();
    void moveSectionAndRemove();
    void saveRestore();
    void restoreQt4State();
    void restoreToMoreColumns();
    void restoreToMoreColumnsNoMovedColumns();
    void restoreBeforeSetModel();
    void defaultSectionSizeTest();
    void defaultSectionSizeTestStyles();

    void defaultAlignment_data();
    void defaultAlignment();

    void globalResizeMode_data();
    void globalResizeMode();

    void sectionPressedSignal_data();
    void sectionPressedSignal();
    void sectionClickedSignal_data() { sectionPressedSignal_data(); }
    void sectionClickedSignal();

    void defaultSectionSize_data();
    void defaultSectionSize();

    void oneSectionSize();

    void hideAndInsert_data();
    void hideAndInsert();

    void removeSection();
    void preserveHiddenSectionWidth();
    void invisibleStretchLastSection();
    void noSectionsWithNegativeSize();

    void emptySectionSpan();
    void task236450_hidden_data();
    void task236450_hidden();
    void task248050_hideRow();
    void QTBUG6058_reset();
    void QTBUG7833_sectionClicked();
    void checkLayoutChangeEmptyModel();
    void QTBUG8650_crashOnInsertSections();
    void QTBUG12268_hiddenMovedSectionSorting();
    void QTBUG14242_hideSectionAutoSize();
    void QTBUG50171_visualRegionForSwappedItems();
    void QTBUG53221_assertShiftHiddenRow();
    void QTBUG75615_sizeHintWithStylesheet();
    void ensureNoIndexAtLength();
    void offsetConsistent();

    void initialSortOrderRole();

    void logicalIndexAtTest_data()   { setupTestData(); }
    void visualIndexAtTest_data()    { setupTestData(); }
    void hideShowTest_data()         { setupTestData(); }
    void swapSectionsTest_data()     { setupTestData(); }
    void moveSectionTest_data()      { setupTestData(); }
    void defaultSizeTest_data()      { setupTestData(); }
    void removeTest_data()           { setupTestData(true); }
    void insertTest_data()           { setupTestData(true); }
    void mixedTests_data()           { setupTestData(true); }
    void resizeToContentTest_data()  { setupTestData(); }
    void logicalIndexAtTest();
    void visualIndexAtTest();
    void hideShowTest();
    void swapSectionsTest();
    void moveSectionTest();
    void defaultSizeTest();
    void removeTest();
    void insertTest();
    void mixedTests();
    void resizeToContentTest();
    void testStreamWithHide();
    void testStylePosition();
    void stretchAndRestoreLastSection();
    void testMinMaxSectionSize_data();
    void testMinMaxSectionSize();
    void sizeHintCrash();
    void testResetCachedSizeHint();
    void statusTips();
    void testRemovingColumnsViaLayoutChanged();

protected:
    void setupTestData(bool use_reset_model = false);
    void additionalInit();
    void calculateAndCheck(int cppline, const int precalced_comparedata[]);
    void testMinMaxSectionSize(bool stretchLastSection);

    QWidget *topLevel = nullptr;
    QHeaderView *view = nullptr;
    QStandardItemModel *model = nullptr;
    QTableView *m_tableview = nullptr;
    bool m_using_reset_model = false;
    bool m_special_prepare = false;
    QElapsedTimer timer;
};

void tst_QHeaderView::initMain()
{
#ifdef Q_OS_WIN
    // Ensure minimum size constraints of framed windows on High DPI screens
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
}

class QtTestModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    QtTestModel(int rc, int cc, QObject *parent = nullptr)
        : QAbstractTableModel(parent), rows(rc), cols(cc) {}
    int rowCount(const QModelIndex &) const override { return rows; }
    int columnCount(const QModelIndex &) const override { return cols; }
    bool isEditable(const QModelIndex &) const { return true; }
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (section < 0 || (role != Qt::DisplayRole && role != Qt::StatusTipRole))
            return QVariant();
        const int row = (orientation == Qt::Vertical ? section : 0);
        const int col = (orientation == Qt::Horizontal ? section : 0);
        if (orientation == Qt::Vertical && row >= rows)
            return QVariant();
        if (orientation == Qt::Horizontal && col >= cols)
            return QVariant();
        if (m_bMultiLine)
             return QString("%1\n%1").arg(section);
        return QLatin1Char('[') + QString::number(row) + QLatin1Char(',')
            + QString::number(col) + QLatin1String(",0] -- Header");
    }
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole)
            return QVariant();
        if (idx.row() < 0 || idx.column() < 0 || idx.column() >= cols || idx.row() >= rows) {
            wrongIndex = true;
            qWarning("Invalid modelIndex [%d,%d,%p]", idx.row(), idx.column(), idx.internalPointer());
        }
        return QLatin1Char('[') + QString::number(idx.row()) + QLatin1Char(',')
            + QString::number(idx.column()) + QLatin1String(",0]");
    }

    void insertOneColumn(int col)
    {
        beginInsertColumns(QModelIndex(), col, col);
        --cols;
        endInsertColumns();
    }

    void removeFirstRow()
    {
        beginRemoveRows(QModelIndex(), 0, 0);
        --rows;
        endRemoveRows();
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

    void removeOneColumn(int col)
    {
        beginRemoveColumns(QModelIndex(), col, col);
        --cols;
        endRemoveColumns();
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

    void cleanup()
    {
        emit layoutAboutToBeChanged();
        cols = 3;
        rows = 3;
        emit layoutChanged();
    }

    void emitLayoutChanged()
    {
        emit layoutAboutToBeChanged();
        emit layoutChanged();
    }

    void emitLayoutChangedWithRemoveFirstRow()
    {
        emit layoutAboutToBeChanged();
        QModelIndexList milNew;
        const auto milOld = persistentIndexList();
        milNew.reserve(milOld.size());
        for (int i = 0; i < milOld.size(); ++i)
            milNew += QModelIndex();
        changePersistentIndexList(milOld, milNew);
        emit layoutChanged();
    }

    void setMultiLineHeader(bool bEnable)
    {
        beginResetModel();
        m_bMultiLine = bEnable;
        endResetModel();
    }

    int rows = 0;
    int cols = 0;
    mutable bool wrongIndex = false;
    bool m_bMultiLine = false;
};

// Testing get/set functions
void tst_QHeaderView::getSetCheck()
{
    protected_QHeaderView obj1(Qt::Horizontal);
    // bool QHeaderView::highlightSections()
    // void QHeaderView::setHighlightSections(bool)
    obj1.setHighlightSections(false);
    QCOMPARE(false, obj1.highlightSections());
    obj1.setHighlightSections(true);
    QCOMPARE(true, obj1.highlightSections());

    // bool QHeaderView::stretchLastSection()
    // void QHeaderView::setStretchLastSection(bool)
    obj1.setStretchLastSection(false);
    QCOMPARE(false, obj1.stretchLastSection());
    obj1.setStretchLastSection(true);
    QCOMPARE(true, obj1.stretchLastSection());

    // int QHeaderView::defaultSectionSize()
    // void QHeaderView::setDefaultSectionSize(int)
    obj1.setMinimumSectionSize(0);
    obj1.setDefaultSectionSize(-1);
    QVERIFY(obj1.defaultSectionSize() >= 0);
    obj1.setDefaultSectionSize(0);
    QCOMPARE(0, obj1.defaultSectionSize());
    obj1.setDefaultSectionSize(99999);
    QCOMPARE(99999, obj1.defaultSectionSize());

    // int QHeaderView::minimumSectionSize()
    // void QHeaderView::setMinimumSectionSize(int)
    obj1.setMinimumSectionSize(-1);
    QVERIFY(obj1.minimumSectionSize() >= 0);
    obj1.setMinimumSectionSize(0);
    QCOMPARE(0, obj1.minimumSectionSize());
    obj1.setMinimumSectionSize(99999);
    QCOMPARE(99999, obj1.minimumSectionSize());
    obj1.setMinimumSectionSize(-1);
    QVERIFY(obj1.minimumSectionSize() < 100);

    // int QHeaderView::offset()
    // void QHeaderView::setOffset(int)
    obj1.setOffset(0);
    QCOMPARE(0, obj1.offset());
    obj1.setOffset(std::numeric_limits<int>::min());
    QCOMPARE(std::numeric_limits<int>::min(), obj1.offset());
    obj1.setOffset(std::numeric_limits<int>::max());
    QCOMPARE(std::numeric_limits<int>::max(), obj1.offset());

}

tst_QHeaderView::tst_QHeaderView()
{
    qRegisterMetaType<Qt::SortOrder>("Qt::SortOrder");
}

void tst_QHeaderView::initTestCase()
{
    m_tableview = new QTableView;
    qDebug().noquote().nospace()
            << "default min section size is "
            << QString::number(m_tableview->verticalHeader()->minimumSectionSize())
            << QLatin1Char('/')
            << m_tableview->horizontalHeader()->minimumSectionSize()
            << " (v/h)";
}

void tst_QHeaderView::cleanupTestCase()
{
    delete m_tableview;
}

void tst_QHeaderView::init()
{
    topLevel = new QWidget;
    view = new QHeaderView(Qt::Vertical, topLevel);
    // Some initial value tests before a model is added
    QCOMPARE(view->length(), 0);
    QCOMPARE(view->sizeHint(), QSize(0,0));
    QCOMPARE(view->sectionSizeHint(0), -1);
    view->setMinimumSectionSize(0);    // system default min size can be to large

    /*
    model = new QStandardItemModel(1, 1);
    view->setModel(model);
    //qDebug() << view->count();
    view->sizeHint();
    */

    int rows = 4;
    int columns = 4;
    model = new QStandardItemModel(rows, columns);
    /*
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            QModelIndex index = model->index(row, column, QModelIndex());
            model->setData(index, QVariant((row+1) * (column+1)));
        }
    }
    */

    QSignalSpy spy(view, &QHeaderView::sectionCountChanged);
    view->setModel(model);
    QCOMPARE(spy.count(), 1);
    view->resize(200,200);
}

void tst_QHeaderView::cleanup()
{
    m_tableview->setUpdatesEnabled(true);
    if (view && view->parent() != m_tableview)
        delete view;
    view = nullptr;
    delete model;
    model = nullptr;
    delete topLevel;
    topLevel = nullptr;
}

void tst_QHeaderView::noModel()
{
    QHeaderView emptyView(Qt::Vertical);
    QCOMPARE(emptyView.count(), 0);
}

void tst_QHeaderView::emptyModel()
{
    QtTestModel testmodel(0, 0);
    view->setModel(&testmodel);
    QVERIFY(!testmodel.wrongIndex);
    QCOMPARE(view->count(), testmodel.rows);
    view->setModel(model);
}

void tst_QHeaderView::removeRows()
{
    QtTestModel model(10, 10);

    QHeaderView vertical(Qt::Vertical);
    QHeaderView horizontal(Qt::Horizontal);

    vertical.setModel(&model);
    horizontal.setModel(&model);
    vertical.show();
    horizontal.show();
    QCOMPARE(vertical.count(), model.rows);
    QCOMPARE(horizontal.count(), model.cols);

    model.removeLastRow();
    QVERIFY(!model.wrongIndex);
    QCOMPARE(vertical.count(), model.rows);
    QCOMPARE(horizontal.count(), model.cols);

    model.removeAllRows();
    QVERIFY(!model.wrongIndex);
    QCOMPARE(vertical.count(), model.rows);
    QCOMPARE(horizontal.count(), model.cols);
}


void tst_QHeaderView::removeCols()
{
    QtTestModel model(10, 10);

    QHeaderView vertical(Qt::Vertical);
    QHeaderView horizontal(Qt::Horizontal);
    vertical.setModel(&model);
    horizontal.setModel(&model);
    vertical.show();
    horizontal.show();
    QCOMPARE(vertical.count(), model.rows);
    QCOMPARE(horizontal.count(), model.cols);

    model.removeLastColumn();
    QVERIFY(!model.wrongIndex);
    QCOMPARE(vertical.count(), model.rows);
    QCOMPARE(horizontal.count(), model.cols);

    model.removeAllColumns();
    QVERIFY(!model.wrongIndex);
    QCOMPARE(vertical.count(), model.rows);
    QCOMPARE(horizontal.count(), model.cols);
}

void tst_QHeaderView::movable()
{
    QCOMPARE(view->sectionsMovable(), false);
    view->setSectionsMovable(false);
    QCOMPARE(view->sectionsMovable(), false);
    view->setSectionsMovable(true);
    QCOMPARE(view->sectionsMovable(), true);

    QCOMPARE(view->isFirstSectionMovable(), true);
    view->setFirstSectionMovable(false);
    QCOMPARE(view->isFirstSectionMovable(), false);
    view->setFirstSectionMovable(true);
    QCOMPARE(view->isFirstSectionMovable(), true);
}

void tst_QHeaderView::clickable()
{
    QCOMPARE(view->sectionsClickable(), false);
    view->setSectionsClickable(false);
    QCOMPARE(view->sectionsClickable(), false);
    view->setSectionsClickable(true);
    QCOMPARE(view->sectionsClickable(), true);
}

void tst_QHeaderView::hidden()
{
    //hideSection() & showSection call setSectionHidden
    // Test bad arguments
    QCOMPARE(view->isSectionHidden(-1), false);
    QCOMPARE(view->isSectionHidden(view->count()), false);
    QCOMPARE(view->isSectionHidden(999999), false);

    view->setSectionHidden(-1, true);
    view->setSectionHidden(view->count(), true);
    view->setSectionHidden(999999, true);
    view->setSectionHidden(-1, false);
    view->setSectionHidden(view->count(), false);
    view->setSectionHidden(999999, false);

    // Hidden sections shouldn't have visual properties (except position)
    int pos = view->defaultSectionSize();
    view->setSectionHidden(1, true);
    QCOMPARE(view->sectionSize(1), 0);
    QCOMPARE(view->sectionPosition(1), pos);
    view->resizeSection(1, 100);
    QCOMPARE(view->sectionViewportPosition(1), pos);
    QCOMPARE(view->sectionSize(1), 0);
    view->setSectionHidden(1, false);
    QCOMPARE(view->isSectionHidden(0), false);
    QCOMPARE(view->sectionSize(0), view->defaultSectionSize());

    // d->hiddenSectionSize could go out of sync when a new model
    // was set which has fewer sections than before and some of them
    // were hidden
    QStandardItemModel model2(model->rowCount() - 1, model->columnCount());

    for (int i = 0; i < model->rowCount(); ++i)
        view->setSectionHidden(i, true);
    view->setModel(&model2);
    QVERIFY(view->sectionsHidden());
    for (int i = 0; i < model2.rowCount(); ++i)
        QVERIFY(view->isSectionHidden(i));

    view->setModel(model);
    for (int i = 0; i < model2.rowCount(); ++i)
        QVERIFY(view->isSectionHidden(i));
    QCOMPARE(view->isSectionHidden(model->rowCount() - 1), false);
    for (int i = 0; i < model->rowCount(); ++i)
        view->setSectionHidden(i, false);
}

void tst_QHeaderView::stretch()
{
    // Show before resize and setStretchLastSection
    QSize viewSize(500, 500);
    view->resize(viewSize);
    view->setStretchLastSection(true);
    QCOMPARE(view->stretchLastSection(), true);
    topLevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(topLevel));
    QCOMPARE(view->width(), viewSize.width());
    QCOMPARE(view->visualIndexAt(view->viewport()->height() - 5), 3);

    view->setSectionHidden(3, true);
    QCOMPARE(view->visualIndexAt(view->viewport()->height() - 5), 2);

    view->setStretchLastSection(false);
    QCOMPARE(view->stretchLastSection(), false);
}

void tst_QHeaderView::oneSectionSize()
{
    //this ensures that if there is only one section, it gets a correct width (more than 0)
    QHeaderView view (Qt::Vertical);
    QtTestModel model(1, 1);

    view.setSectionResizeMode(QHeaderView::Interactive);
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QVERIFY(view.sectionSize(0) > 0);
}


void tst_QHeaderView::sectionSize_data()
{
    QTest::addColumn<IntList>("boundsCheck");
    QTest::addColumn<IntList>("defaultSizes");
    QTest::addColumn<int>("initialDefaultSize");
    QTest::addColumn<int>("lastVisibleSectionSize");
    QTest::addColumn<int>("persistentSectionSize");

    QTest::newRow("data set one")
        << (IntList{ -1, 0, 4, 9999 })
        << (IntList{ 10, 30, 30 })
        << 30
        << 300
        << 20;
}

void tst_QHeaderView::sectionSize()
{
#if defined Q_OS_QNX
    QSKIP("The section size is dpi dependent on QNX");
#elif defined Q_OS_WINRT
    QSKIP("Fails on WinRT - QTBUG-68297");
#endif
    QFETCH(const IntList, boundsCheck);
    QFETCH(const IntList, defaultSizes);
    QFETCH(int, initialDefaultSize);
    QFETCH(int, lastVisibleSectionSize);
    QFETCH(int, persistentSectionSize);

    // bounds check
    for (int val : boundsCheck)
        view->sectionSize(val);

    // default size
    QCOMPARE(view->defaultSectionSize(), initialDefaultSize);
    for (int def : defaultSizes) {
        view->setDefaultSectionSize(def);
        QCOMPARE(view->defaultSectionSize(), def);
    }

    view->setDefaultSectionSize(initialDefaultSize);
    for (int s = 0; s < view->count(); ++s)
        QCOMPARE(view->sectionSize(s), initialDefaultSize);
    view->doItemsLayout();

    // stretch last section
    view->setStretchLastSection(true);
    int lastSection = view->count() - 1;

    //test that when hiding the last column,
    //resizing the new last visible columns still works
    view->hideSection(lastSection);
    view->resizeSection(lastSection - 1, lastVisibleSectionSize);
    QCOMPARE(view->sectionSize(lastSection - 1), lastVisibleSectionSize);
    view->showSection(lastSection);

    // turn off stretching
    view->setStretchLastSection(false);
    QCOMPARE(view->sectionSize(lastSection), initialDefaultSize);

    // test persistence
    int sectionCount = view->count();
    for (int i = 0; i < sectionCount; ++i)
        view->resizeSection(i, persistentSectionSize);
    QtTestModel model(sectionCount * 2, sectionCount * 2);
    view->setModel(&model);
    for (int j = 0; j < sectionCount; ++j)
        QCOMPARE(view->sectionSize(j), persistentSectionSize);
    for (int k = sectionCount; k < view->count(); ++k)
        QCOMPARE(view->sectionSize(k), initialDefaultSize);
}

void tst_QHeaderView::visualIndex()
{
    // Test bad arguments
    QCOMPARE(view->visualIndex(999999), -1);
    QCOMPARE(view->visualIndex(-1), -1);
    QCOMPARE(view->visualIndex(1), 1);
    view->setSectionHidden(1, true);
    QCOMPARE(view->visualIndex(1), 1);
    QCOMPARE(view->visualIndex(2), 2);

    view->setSectionHidden(1, false);
    QCOMPARE(view->visualIndex(1), 1);
    QCOMPARE(view->visualIndex(2), 2);
}

void tst_QHeaderView::visualIndexAt_data()
{
    QTest::addColumn<IntList>("hidden");
    QTest::addColumn<IntList>("from");
    QTest::addColumn<IntList>("to");
    QTest::addColumn<IntList>("coordinate");
    QTest::addColumn<IntList>("visual");

    const IntList coordinateList{ -1, 0, 31, 91, 99999 };

    QTest::newRow("no hidden, no moved sections")
        << IntList()
        << IntList()
        << IntList()
        << coordinateList
        << (IntList{ -1, 0, 1, 3, -1 });

    QTest::newRow("no hidden, moved sections")
        << IntList()
        << (IntList{ 0 })
        << (IntList{ 1 })
        << coordinateList
        << (IntList{ -1, 0, 1, 3, -1 });

    QTest::newRow("hidden, no moved sections")
        << (IntList{ 0 })
        << IntList()
        << IntList()
        << coordinateList
        << (IntList{ -1, 1, 2, 3, -1 });
}

void tst_QHeaderView::visualIndexAt()
{
#if defined Q_OS_QNX
    QSKIP("The section size is dpi dependent on QNX");
#elif defined Q_OS_WINRT
    QSKIP("Fails on WinRT - QTBUG-68297");
#endif
    QFETCH(const IntList, hidden);
    QFETCH(const IntList, from);
    QFETCH(const IntList, to);
    QFETCH(const IntList, coordinate);
    QFETCH(const IntList, visual);

    view->setStretchLastSection(true);
    topLevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(topLevel));

    for (int i : hidden)
        view->setSectionHidden(i, true);

    for (int j = 0; j < from.count(); ++j)
        view->moveSection(from.at(j), to.at(j));

    for (int k = 0; k < coordinate.count(); ++k)
        QTRY_COMPARE(view->visualIndexAt(coordinate.at(k)), visual.at(k));
}

void tst_QHeaderView::length()
{
    view->setStretchLastSection(true);
    topLevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(topLevel));

    //minimumSectionSize should be the size of the last section of the widget is not tall enough
    int length = view->minimumSectionSize();
    for (int i = 0; i < view->count() - 1; i++)
        length += view->sectionSize(i);

    length = qMax(length, view->viewport()->height());
    QCOMPARE(length, view->length());

    view->setStretchLastSection(false);
    topLevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(topLevel));

    QVERIFY(length != view->length());

    // layoutChanged might mean rows have been removed
    QtTestModel model(10, 10);
    view->setModel(&model);
    int oldLength = view->length();
    model.cleanup();
    QCOMPARE(model.rows, view->count());
    QVERIFY(oldLength != view->length());
}

void tst_QHeaderView::offset()
{
    QCOMPARE(view->offset(), 0);
    view->setOffset(10);
    QCOMPARE(view->offset(), 10);
    view->setOffset(0);
    QCOMPARE(view->offset(), 0);

    // Test odd arguments
    view->setOffset(-1);
}

void tst_QHeaderView::sectionSizeHint()
{
    QCOMPARE(view->sectionSizeHint(-1), -1);
    QCOMPARE(view->sectionSizeHint(99999), -1);
    QVERIFY(view->sectionSizeHint(0) >= 0);
}

void tst_QHeaderView::logicalIndex()
{
    // Test bad arguments
    QCOMPARE(view->logicalIndex(-1), -1);
    QCOMPARE(view->logicalIndex(99999), -1);
}

void tst_QHeaderView::logicalIndexAt()
{
    // Test bad arguments
    view->logicalIndexAt(-1);
    view->logicalIndexAt(99999);
    QCOMPARE(view->logicalIndexAt(0), 0);
    QCOMPARE(view->logicalIndexAt(1), 0);

    topLevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(topLevel));
    view->setStretchLastSection(true);
    // First item
    QCOMPARE(view->logicalIndexAt(0), 0);
    QCOMPARE(view->logicalIndexAt(view->sectionSize(0)-1), 0);
    QCOMPARE(view->logicalIndexAt(view->sectionSize(0)+1), 1);
    // Last item
    int last = view->length() - 1;//view->viewport()->height() - 10;
    QCOMPARE(view->logicalIndexAt(last), 3);
    // Not in widget
    int outofbounds = view->length() + 1;//view->viewport()->height() + 1;
    QCOMPARE(view->logicalIndexAt(outofbounds), -1);

    view->moveSection(0,1);
    // First item
    QCOMPARE(view->logicalIndexAt(0), 1);
    QCOMPARE(view->logicalIndexAt(view->sectionSize(0)-1), 1);
    QCOMPARE(view->logicalIndexAt(view->sectionSize(0)+1), 0);
    // Last item
    QCOMPARE(view->logicalIndexAt(last), 3);
    view->moveSection(1,0);

}

void tst_QHeaderView::swapSections()
{
    view->swapSections(-1, 1);
    view->swapSections(99999, 1);
    view->swapSections(1, -1);
    view->swapSections(1, 99999);

    IntList logical{ 0, 1, 2, 3 };

    QSignalSpy spy1(view, &QHeaderView::sectionMoved);

    QCOMPARE(view->sectionsMoved(), false);
    view->swapSections(1, 1);
    QCOMPARE(view->sectionsMoved(), false);
    view->swapSections(1, 2);
    QCOMPARE(view->sectionsMoved(), true);
    view->swapSections(2, 1);
    QCOMPARE(view->sectionsMoved(), true);
    for (int i = 0; i < view->count(); ++i)
        QCOMPARE(view->logicalIndex(i), logical.at(i));
    QCOMPARE(spy1.count(), 4);

    logical = { 3, 1, 2, 0 };
    view->swapSections(3, 0);
    QCOMPARE(view->sectionsMoved(), true);
    for (int j = 0; j < view->count(); ++j)
        QCOMPARE(view->logicalIndex(j), logical.at(j));
    QCOMPARE(spy1.count(), 6);
}

void tst_QHeaderView::moveSection_data()
{
    QTest::addColumn<IntList>("hidden");
    QTest::addColumn<IntList>("from");
    QTest::addColumn<IntList>("to");
    QTest::addColumn<BoolList>("moved");
    QTest::addColumn<IntList>("logical");
    QTest::addColumn<int>("count");

    QTest::newRow("bad args, no hidden")
        << IntList()
        << (IntList{ -1, 1, 99999, 1 })
        << (IntList{ 1, -1, 1, 99999 })
        << (BoolList{ false, false, false, false })
        << (IntList{ 0, 1, 2, 3 })
        << 0;

    QTest::newRow("good args, no hidden")
        << IntList()
        << (IntList{ 1, 1, 2, 1 })
        << (IntList{ 1, 2, 1, 2 })
        << (BoolList{ false, true, true, true })
        << (IntList{ 0, 2, 1, 3 })
        << 3;

    QTest::newRow("hidden sections")
        << (IntList{ 0, 3 })
        << (IntList{ 1, 1, 2, 1 })
        << (IntList{ 1, 2, 1, 2 })
        << (BoolList{ false, true, true, true })
        << (IntList{ 0, 2, 1, 3 })
        << 3;
}

void tst_QHeaderView::moveSection()
{
    QFETCH(const IntList, hidden);
    QFETCH(const IntList, from);
    QFETCH(const IntList, to);
    QFETCH(const BoolList, moved);
    QFETCH(const IntList, logical);
    QFETCH(int, count);

    QCOMPARE(from.count(), to.count());
    QCOMPARE(from.count(), moved.count());
    QCOMPARE(view->count(), logical.count());

    QSignalSpy spy1(view, &QHeaderView::sectionMoved);
    QCOMPARE(view->sectionsMoved(), false);

    for (int h : hidden)
        view->setSectionHidden(h, true);

    for (int i = 0; i < from.count(); ++i) {
        view->moveSection(from.at(i), to.at(i));
        QCOMPARE(view->sectionsMoved(), moved.at(i));
    }

    for (int j = 0; j < view->count(); ++j)
        QCOMPARE(view->logicalIndex(j), logical.at(j));

    QCOMPARE(spy1.count(), count);
}

void tst_QHeaderView::resizeAndMoveSection_data()
{
    QTest::addColumn<IntList>("logicalIndexes");
    QTest::addColumn<IntList>("sizes");
    QTest::addColumn<int>("logicalFrom");
    QTest::addColumn<int>("logicalTo");

    QTest::newRow("resizeAndMove-1")
        << (IntList{ 0, 1 })
        << (IntList{ 20, 40 })
        << 0 << 1;

    QTest::newRow("resizeAndMove-2")
        << (IntList{ 0, 1, 2, 3 })
        << (IntList{ 20, 60, 10, 80 })
        << 0 << 2;

    QTest::newRow("resizeAndMove-3")
        << (IntList{ 0, 1, 2, 3 })
        << (IntList{ 100, 60, 40, 10 })
        << 0 << 3;

    QTest::newRow("resizeAndMove-4")
        << (IntList{ 0, 1, 2, 3 })
        << (IntList{ 10, 40, 80, 30 })
        << 1 << 2;

    QTest::newRow("resizeAndMove-5")
        << (IntList{ 2, 3 })
        << (IntList{ 100, 200})
        << 3 << 2;
}

void tst_QHeaderView::resizeAndMoveSection()
{
    QFETCH(const IntList, logicalIndexes);
    QFETCH(const IntList, sizes);
    QFETCH(int, logicalFrom);
    QFETCH(int, logicalTo);

    // Save old visual indexes and sizes
    IntList oldVisualIndexes;
    IntList oldSizes;
    for (int logical : logicalIndexes) {
        oldVisualIndexes.append(view->visualIndex(logical));
        oldSizes.append(view->sectionSize(logical));
    }

    // Resize sections
    for (int i = 0; i < logicalIndexes.size(); ++i) {
        int logical = logicalIndexes.at(i);
        view->resizeSection(logical, sizes.at(i));
    }

    // Move sections
    int visualFrom = view->visualIndex(logicalFrom);
    int visualTo = view->visualIndex(logicalTo);
    view->moveSection(visualFrom, visualTo);
    QCOMPARE(view->visualIndex(logicalFrom), visualTo);

    // Check that sizes are still correct
    for (int i = 0; i < logicalIndexes.size(); ++i) {
        int logical = logicalIndexes.at(i);
        QCOMPARE(view->sectionSize(logical), sizes.at(i));
    }

    // Move sections back
    view->moveSection(visualTo, visualFrom);

    // Check that sizes are still correct
    for (int i = 0; i < logicalIndexes.size(); ++i) {
        int logical = logicalIndexes.at(i);
        QCOMPARE(view->sectionSize(logical), sizes.at(i));
    }

    // Put everything back as it was
    for (int i = 0; i < logicalIndexes.size(); ++i) {
        int logical = logicalIndexes.at(i);
        view->resizeSection(logical, oldSizes.at(i));
        QCOMPARE(view->visualIndex(logical), oldVisualIndexes.at(i));
    }
}

void tst_QHeaderView::resizeHiddenSection_data()
{
    QTest::addColumn<int>("section");
    QTest::addColumn<int>("initialSize");
    QTest::addColumn<int>("finalSize");

    QTest::newRow("section 0 resize 50 to 20")
        << 0 << 50 << 20;

    QTest::newRow("section 1 resize 50 to 20")
        << 1 << 50 << 20;

    QTest::newRow("section 2 resize 50 to 20")
        << 2 << 50 << 20;

    QTest::newRow("section 3 resize 50 to 20")
        << 3 << 50 << 20;
}

void tst_QHeaderView::resizeHiddenSection()
{
    QFETCH(int, section);
    QFETCH(int, initialSize);
    QFETCH(int, finalSize);

    view->resizeSection(section, initialSize);
    view->setSectionHidden(section, true);
    QCOMPARE(view->sectionSize(section), 0);

    view->resizeSection(section, finalSize);
    QCOMPARE(view->sectionSize(section), 0);

    view->setSectionHidden(section, false);
    QCOMPARE(view->sectionSize(section), finalSize);
}

void tst_QHeaderView::resizeAndInsertSection_data()
{
    QTest::addColumn<int>("section");
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("insert");
    QTest::addColumn<int>("compare");
    QTest::addColumn<int>("expected");

    QTest::newRow("section 0 size 50 insert 0")
        << 0 << 50 << 0 << 1 << 50;

    QTest::newRow("section 1 size 50 insert 1")
        << 0 << 50 << 1 << 0 << 50;

    QTest::newRow("section 1 size 50 insert 0")
        << 1 << 50 << 0 << 2 << 50;

}

void tst_QHeaderView::resizeAndInsertSection()
{
    QFETCH(int, section);
    QFETCH(int, size);
    QFETCH(int, insert);
    QFETCH(int, compare);
    QFETCH(int, expected);

    view->setStretchLastSection(false);

    view->resizeSection(section, size);
    QCOMPARE(view->sectionSize(section), size);

    model->insertRow(insert);

    QCOMPARE(view->sectionSize(compare), expected);
}

void tst_QHeaderView::resizeWithResizeModes_data()
{
    QTest::addColumn<int>("size");
    QTest::addColumn<IntList>("sections");
    QTest::addColumn<ResizeVec>("modes");
    QTest::addColumn<IntList>("expected");

    QTest::newRow("stretch first section")
        << 600
        << (IntList{ 100, 100, 100, 100 })
        << (ResizeVec
                { QHeaderView::Stretch,
                  QHeaderView::Interactive,
                  QHeaderView::Interactive,
                  QHeaderView::Interactive })
        << (IntList{ 300, 100, 100, 100 });
}

void  tst_QHeaderView::resizeWithResizeModes()
{
    QFETCH(int, size);
    QFETCH(const IntList, sections);
    QFETCH(const ResizeVec, modes);
    QFETCH(const IntList, expected);

    view->setStretchLastSection(false);
    for (int i = 0; i < sections.count(); ++i) {
        view->resizeSection(i, sections.at(i));
        view->setSectionResizeMode(i, modes.at(i));
    }
    topLevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(topLevel));
    view->resize(size, size);
    for (int j = 0; j < expected.count(); ++j)
        QCOMPARE(view->sectionSize(j), expected.at(j));
}

void tst_QHeaderView::moveAndInsertSection_data()
{
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::addColumn<int>("insert");
    QTest::addColumn<IntList>("mapping");

    QTest::newRow("move from 1 to 3, insert 0")
        << 1 << 3 << 0 <<(IntList{ 0, 1, 3, 4, 2 });

}

void tst_QHeaderView::moveAndInsertSection()
{
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(int, insert);
    QFETCH(IntList, mapping);

    view->setStretchLastSection(false);
    view->moveSection(from, to);
    model->insertRow(insert);

    for (int i = 0; i < mapping.count(); ++i)
        QCOMPARE(view->logicalIndex(i), mapping.at(i));
}

void tst_QHeaderView::resizeMode()
{
    // resizeMode must not be called with an invalid index
    int last = view->count() - 1;
    view->setSectionResizeMode(QHeaderView::Interactive);
    QCOMPARE(view->sectionResizeMode(last), QHeaderView::Interactive);
    QCOMPARE(view->sectionResizeMode(1), QHeaderView::Interactive);
    view->setSectionResizeMode(QHeaderView::Stretch);
    QCOMPARE(view->sectionResizeMode(last), QHeaderView::Stretch);
    QCOMPARE(view->sectionResizeMode(1), QHeaderView::Stretch);
    view->setSectionResizeMode(QHeaderView::Custom);
    QCOMPARE(view->sectionResizeMode(last), QHeaderView::Custom);
    QCOMPARE(view->sectionResizeMode(1), QHeaderView::Custom);

    // test when sections have been moved
    view->setStretchLastSection(false);
    for (int i = 0; i < (view->count() - 1); ++i)
        view->setSectionResizeMode(i, QHeaderView::Interactive);
    int logicalIndex = view->count() / 2;
    view->setSectionResizeMode(logicalIndex, QHeaderView::Stretch);
    view->moveSection(view->visualIndex(logicalIndex), 0);
    for (int i = 0; i < (view->count() - 1); ++i) {
        if (i == logicalIndex)
            QCOMPARE(view->sectionResizeMode(i), QHeaderView::Stretch);
        else
            QCOMPARE(view->sectionResizeMode(i), QHeaderView::Interactive);
    }
}

void tst_QHeaderView::resizeSection_data()
{
    QTest::addColumn<int>("initial");
    QTest::addColumn<IntList>("logical");
    QTest::addColumn<IntList>("size");
    QTest::addColumn<ResizeVec>("mode");
    QTest::addColumn<int>("resized");
    QTest::addColumn<IntList>("expected");

    QTest::newRow("bad args")
        << 100
        << (IntList{ -1, -1, 99999, 99999, 4 })
        << (IntList{ -1, 0, 99999, -1, -1 })
        << (ResizeVec{
                QHeaderView::Interactive,
                QHeaderView::Interactive,
                QHeaderView::Interactive,
                QHeaderView::Interactive })
        << 0
        << (IntList{ 0, 0, 0, 0, 0 });
}

void tst_QHeaderView::resizeSection()
{
    QFETCH(int, initial);
    QFETCH(const IntList, logical);
    QFETCH(const IntList, size);
    QFETCH(const ResizeVec, mode);
    QFETCH(int, resized);
    QFETCH(const IntList, expected);

    view->resize(400, 400);

    topLevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(topLevel));
    view->setSectionsMovable(true);
    view->setStretchLastSection(false);

    for (int i = 0; i < logical.count(); ++i)
        if (logical.at(i) > -1 && logical.at(i) < view->count()) // for now
            view->setSectionResizeMode(logical.at(i), mode.at(i));

    for (int j = 0; j < logical.count(); ++j)
        view->resizeSection(logical.at(j), initial);

    QSignalSpy spy(view, &QHeaderView::sectionResized);

    for (int k = 0; k < logical.count(); ++k)
        view->resizeSection(logical.at(k), size.at(k));

    QCOMPARE(spy.count(), resized);

    for (int l = 0; l < logical.count(); ++l)
        QCOMPARE(view->sectionSize(logical.at(l)), expected.at(l));
}

void tst_QHeaderView::highlightSections()
{
    view->setHighlightSections(true);
    QCOMPARE(view->highlightSections(), true);
    view->setHighlightSections(false);
    QCOMPARE(view->highlightSections(), false);
}

void tst_QHeaderView::showSortIndicator()
{
    view->setSortIndicatorShown(true);
    QCOMPARE(view->isSortIndicatorShown(), true);
    QCOMPARE(view->sortIndicatorOrder(), Qt::DescendingOrder);
    view->setSortIndicator(1, Qt::AscendingOrder);
    QCOMPARE(view->sortIndicatorOrder(), Qt::AscendingOrder);
    view->setSortIndicator(1, Qt::DescendingOrder);
    QCOMPARE(view->sortIndicatorOrder(), Qt::DescendingOrder);
    view->setSortIndicatorShown(false);
    QCOMPARE(view->isSortIndicatorShown(), false);

    view->setSortIndicator(999999, Qt::DescendingOrder);
    // Don't segfault baby :)
    view->setSortIndicatorShown(true);

    view->setSortIndicator(0, Qt::DescendingOrder);
    // Don't assert baby :)
}

void tst_QHeaderView::sortIndicatorTracking()
{
    QtTestModel model(10, 10);
    QHeaderView hv(Qt::Horizontal);

    hv.setModel(&model);
    hv.show();
    hv.setSortIndicatorShown(true);
    hv.setSortIndicator(1, Qt::DescendingOrder);

    model.removeOneColumn(8);
    QCOMPARE(hv.sortIndicatorSection(), 1);

    model.removeOneColumn(2);
    QCOMPARE(hv.sortIndicatorSection(), 1);

    model.insertOneColumn(2);
    QCOMPARE(hv.sortIndicatorSection(), 1);

    model.insertOneColumn(1);
    QCOMPARE(hv.sortIndicatorSection(), 2);

    model.removeOneColumn(0);
    QCOMPARE(hv.sortIndicatorSection(), 1);

    model.removeOneColumn(1);
    QCOMPARE(hv.sortIndicatorSection(), -1);
}

void tst_QHeaderView::removeAndInsertRow()
{
    // Check if logicalIndex returns the correct value after we have removed a row
    // we might as well te
    for (int i = 0; i < model->rowCount(); ++i)
        QCOMPARE(i, view->logicalIndex(i));

    while (model->removeRow(0)) {
        for (int i = 0; i < model->rowCount(); ++i)
            QCOMPARE(i, view->logicalIndex(i));
    }

    for (int pass = 0; pass < 5; pass++) {
        for (int i = 0; i < model->rowCount(); ++i)
            QCOMPARE(i, view->logicalIndex(i));
        model->insertRow(0);
    }

    while (model->removeRows(0, 2)) {
        for (int i = 0; i < model->rowCount(); ++i)
            QCOMPARE(i, view->logicalIndex(i));
    }

    for (int pass = 0; pass < 3; pass++) {
        model->insertRows(0, 2);
        for (int i = 0; i < model->rowCount(); ++i) {
            QCOMPARE(i, view->logicalIndex(i));
        }
    }

    for (int pass = 0; pass < 3; pass++) {
        model->insertRows(3, 2);
        for (int i = 0; i < model->rowCount(); ++i)
            QCOMPARE(i, view->logicalIndex(i));
    }

    // Insert at end
    for (int pass = 0; pass < 3; pass++) {
        int rowCount = model->rowCount();
        model->insertRows(rowCount, 1);
        for (int i = 0; i < rowCount; ++i)
            QCOMPARE(i, view->logicalIndex(i));
    }

}
void tst_QHeaderView::unhideSection()
{
    // You should not necessarily expect the same size back again, so the best test we can do is to test if it is larger than 0 after a unhide.
    QCOMPARE(view->sectionsHidden(), false);
    view->setSectionHidden(0, true);
    QCOMPARE(view->sectionsHidden(), true);
    QCOMPARE(view->sectionSize(0), 0);
    view->setSectionResizeMode(QHeaderView::Interactive);
    view->setSectionHidden(0, false);
    QVERIFY(view->sectionSize(0) > 0);

    view->setSectionHidden(0, true);
    QCOMPARE(view->sectionSize(0), 0);
    view->setSectionHidden(0, true);
    QCOMPARE(view->sectionSize(0), 0);
    view->setSectionResizeMode(QHeaderView::Stretch);
    view->setSectionHidden(0, false);
    QVERIFY(view->sectionSize(0) > 0);

}

void tst_QHeaderView::testEvent()
{
    protected_QHeaderView x(Qt::Vertical);
    x.testEvent();
    protected_QHeaderView y(Qt::Horizontal);
    y.testEvent();
}


void protected_QHeaderView::testEvent()
{
    // No crashy please
    QHoverEvent enterEvent(QEvent::HoverEnter, QPoint(), QPoint());
    event(&enterEvent);
    QHoverEvent eventLeave(QEvent::HoverLeave, QPoint(), QPoint());
    event(&eventLeave);
    QHoverEvent eventMove(QEvent::HoverMove, QPoint(), QPoint());
    event(&eventMove);
}

void tst_QHeaderView::headerDataChanged()
{
    // This shouldn't assert because view is Vertical
    view->headerDataChanged(Qt::Horizontal, -1, -1);
#if 0
    // This will assert
    view->headerDataChanged(Qt::Vertical, -1, -1);
#endif

    // No crashing please
    view->headerDataChanged(Qt::Horizontal, 0, 1);
    view->headerDataChanged(Qt::Vertical, 0, 1);
}

void tst_QHeaderView::currentChanged()
{
    view->setCurrentIndex(QModelIndex());
}

void tst_QHeaderView::horizontalOffset()
{
    protected_QHeaderView x(Qt::Vertical);
    x.testhorizontalOffset();
    protected_QHeaderView y(Qt::Horizontal);
    y.testhorizontalOffset();
}

void tst_QHeaderView::verticalOffset()
{
    protected_QHeaderView x(Qt::Vertical);
    x.testverticalOffset();
    protected_QHeaderView y(Qt::Horizontal);
    y.testverticalOffset();
}

void  protected_QHeaderView::testhorizontalOffset()
{
    if (orientation() == Qt::Horizontal) {
        QCOMPARE(horizontalOffset(), 0);
        setOffset(10);
        QCOMPARE(horizontalOffset(), 10);
    }
    else
        QCOMPARE(horizontalOffset(), 0);
}

void  protected_QHeaderView::testverticalOffset()
{
    if (orientation() == Qt::Vertical) {
        QCOMPARE(verticalOffset(), 0);
        setOffset(10);
        QCOMPARE(verticalOffset(), 10);
    }
    else
        QCOMPARE(verticalOffset(), 0);
}

void tst_QHeaderView::stretchSectionCount()
{
    view->setStretchLastSection(false);
    QCOMPARE(view->stretchSectionCount(), 0);
    view->setStretchLastSection(true);
    QCOMPARE(view->stretchSectionCount(), 0);

    view->setSectionResizeMode(0, QHeaderView::Stretch);
    QCOMPARE(view->stretchSectionCount(), 1);
}

void tst_QHeaderView::hiddenSectionCount()
{
    model->clear();
    model->insertRows(0, 10);
    // Hide every other one
    for (int i = 0; i < 10; i++)
        view->setSectionHidden(i, (i & 1) == 0);

    QCOMPARE(view->hiddenSectionCount(), 5);

    view->setSectionResizeMode(QHeaderView::Stretch);
    QCOMPARE(view->hiddenSectionCount(), 5);

    // Remove some rows and make sure they are now still counted
    model->removeRow(9);
    model->removeRow(8);
    model->removeRow(7);
    model->removeRow(6);
    QCOMPARE(view->count(), 6);
    QCOMPARE(view->hiddenSectionCount(), 3);
    model->removeRows(0, 5);
    QCOMPARE(view->count(), 1);
    QCOMPARE(view->hiddenSectionCount(), 0);
    QVERIFY(view->count() >=  view->hiddenSectionCount());
}

void tst_QHeaderView::focusPolicy()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QHeaderView view(Qt::Horizontal);
    QCOMPARE(view.focusPolicy(), Qt::NoFocus);

    QTreeWidget widget;
    QCOMPARE(widget.header()->focusPolicy(), Qt::NoFocus);
    QVERIFY(!widget.focusProxy());
    QVERIFY(!widget.hasFocus());
    QVERIFY(!widget.header()->focusProxy());
    QVERIFY(!widget.header()->hasFocus());

    widget.show();
    widget.setFocus(Qt::OtherFocusReason);
    QApplication::setActiveWindow(&widget);
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));
    QVERIFY(widget.hasFocus());
    QVERIFY(!widget.header()->hasFocus());

    widget.setFocusPolicy(Qt::NoFocus);
    widget.clearFocus();
    QTRY_VERIFY(!widget.hasFocus());
    QVERIFY(!widget.header()->hasFocus());

    QTest::keyPress(&widget, Qt::Key_Tab);

    QCoreApplication::processEvents();
    QCoreApplication::processEvents();

    QVERIFY(!widget.hasFocus());
    QVERIFY(!widget.header()->hasFocus());
}

class SimpleModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    using QAbstractItemModel::QAbstractItemModel;
    QModelIndex parent(const QModelIndex &/*child*/) const override
    {
        return QModelIndex();
    }
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
    }
    int rowCount(const QModelIndex & /*parent*/ = QModelIndex()) const override
    {
        return 8;
    }
    int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const override
    {
        return m_col_count;
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();
        if (role == Qt::DisplayRole)
            return QString::number(index.row()) + QLatin1Char(',') + QString::number(index.column());
        return QVariant();
    }
    void setColumnCount(int c)
    {
        m_col_count = c;
    }
private:
    int m_col_count = 3;
};

void tst_QHeaderView::moveSectionAndReset()
{
    SimpleModel m;
    QHeaderView v(Qt::Horizontal);
    v.setModel(&m);
    int cc = 2;
    for (cc = 2; cc < 4; ++cc) {
        m.setColumnCount(cc);
        int movefrom = 0;
        int moveto;
        for (moveto = 1; moveto < cc; ++moveto) {
            v.moveSection(movefrom, moveto);
            m.setColumnCount(cc - 1);
            v.reset();
            for (int i = 0; i < cc - 1; ++i)
                QCOMPARE(v.logicalIndex(v.visualIndex(i)), i);
        }
    }
}

void tst_QHeaderView::moveSectionAndRemove()
{
    QStandardItemModel m;
    QHeaderView v(Qt::Horizontal);

    v.setModel(&m);
    v.model()->insertColumns(0, 3);
    v.moveSection(0, 1);

    QCOMPARE(v.count(), 3);
    v.model()->removeColumns(0, v.model()->columnCount());
    QCOMPARE(v.count(), 0);
}

static QByteArray savedState()
{
    QStandardItemModel m(4, 4);
    QHeaderView h1(Qt::Horizontal);
    h1.setModel(&m);
    h1.setMinimumSectionSize(0);    // system default min size can be to large
    h1.swapSections(0, 2);
    h1.resizeSection(1, 10);
    h1.setSortIndicatorShown(true);
    h1.setSortIndicator(2, Qt::DescendingOrder);
    h1.setSectionHidden(3, true);
    return h1.saveState();
}

void tst_QHeaderView::saveRestore()
{
    QStandardItemModel m(4, 4);
    const QByteArray s1 = savedState();

    QHeaderView h2(Qt::Vertical);
    QSignalSpy spy(&h2, &QHeaderView::sortIndicatorChanged);

    h2.setModel(&m);
    QVERIFY(h2.restoreState(s1));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 2);

    QCOMPARE(h2.logicalIndex(0), 2);
    QCOMPARE(h2.logicalIndex(2), 0);
    QCOMPARE(h2.sectionSize(1), 10);
    QCOMPARE(h2.sortIndicatorSection(), 2);
    QCOMPARE(h2.sortIndicatorOrder(), Qt::DescendingOrder);
    QCOMPARE(h2.isSortIndicatorShown(), true);
    QVERIFY(!h2.isSectionHidden(2));
    QVERIFY(h2.isSectionHidden(3));
    QCOMPARE(h2.hiddenSectionCount(), 1);

    QByteArray s2 = h2.saveState();
    QCOMPARE(s1, s2);

    QVERIFY(!h2.restoreState(QByteArrayLiteral("Garbage")));
}

void tst_QHeaderView::restoreQt4State()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // QTBUG-40462
    // Setting from Qt4, where information about multiple sections were grouped together in one
    // sectionItem object
    QStandardItemModel m(4, 10);
    QHeaderView h2(Qt::Vertical);
    QByteArray settings_qt4 =
      QByteArray::fromHex("000000ff00000000000000010000000100000000010000000000000000000000000000"
                          "0000000003e80000000a0101000100000000000000000000000064ffffffff00000081"
                          "0000000000000001000003e80000000a00000000");
    QVERIFY(h2.restoreState(settings_qt4));
    int sectionItemsLengthTotal = 0;
    for (int i = 0; i < h2.count(); ++i)
        sectionItemsLengthTotal += h2.sectionSize(i);
    QCOMPARE(sectionItemsLengthTotal, h2.length());

    // Buggy setting where sum(sectionItems) != length. Check false is returned and this corrupted
    // state isn't restored
    QByteArray settings_buggy_length =
      QByteArray::fromHex("000000ff000000000000000100000000000000050100000000000000000000000a4000"
                          "000000010000000600000258000000fb0000000a010100010000000000000000000000"
                          "0064ffffffff00000081000000000000000a000000d30000000100000000000000c800"
                          "000001000000000000008000000001000000000000005c00000001000000000000003c"
                          "0000000100000000000002580000000100000000000000000000000100000000000002"
                          "580000000100000000000002580000000100000000000003c000000001000000000000"
                          "03e8");
    int old_length = h2.length();
    QByteArray old_state = h2.saveState();
    // Check setting is correctly recognized as corrupted
    QVERIFY(!h2.restoreState(settings_buggy_length));
    // Check nothing has been actually restored
    QCOMPARE(h2.length(), old_length);
    QCOMPARE(h2.saveState(), old_state);
#else
    QSKIP("Qt4 compatibility no longer needed in Qt6")
#endif
}

void tst_QHeaderView::restoreToMoreColumns()
{
    // Restore state onto a model with more columns
    const QByteArray s1 = savedState();
    QHeaderView h4(Qt::Horizontal);
    QStandardItemModel fiveColumnsModel(1, 5);
    h4.setModel(&fiveColumnsModel);
    QCOMPARE(fiveColumnsModel.columnCount(), 5);
    QCOMPARE(h4.count(), 5);
    QVERIFY(h4.restoreState(s1));
    QCOMPARE(fiveColumnsModel.columnCount(), 5);
    QCOMPARE(h4.count(), 5);
    QCOMPARE(h4.sectionSize(1), 10);
    for (int i = 0; i < h4.count(); ++i)
        QVERIFY(h4.sectionSize(i) > 0 || h4.isSectionHidden(i));
    QVERIFY(!h4.isSectionHidden(2));
    QVERIFY(h4.isSectionHidden(3));
    QCOMPARE(h4.hiddenSectionCount(), 1);
    QCOMPARE(h4.sortIndicatorSection(), 2);
    QCOMPARE(h4.sortIndicatorOrder(), Qt::DescendingOrder);
    QCOMPARE(h4.logicalIndex(0), 2);
    QCOMPARE(h4.logicalIndex(1), 1);
    QCOMPARE(h4.logicalIndex(2), 0);
    QCOMPARE(h4.visualIndex(0), 2);
    QCOMPARE(h4.visualIndex(1), 1);
    QCOMPARE(h4.visualIndex(2), 0);

    // Repainting shouldn't crash
    h4.show();
    QVERIFY(QTest::qWaitForWindowExposed(&h4));
}

void tst_QHeaderView::restoreToMoreColumnsNoMovedColumns()
{
    // Given a model with 2 columns, for saving state
    QHeaderView h1(Qt::Horizontal);
    QStandardItemModel model1(1, 2);
    h1.setModel(&model1);
    QCOMPARE(h1.visualIndex(0), 0);
    QCOMPARE(h1.visualIndex(1), 1);
    QCOMPARE(h1.logicalIndex(0), 0);
    QCOMPARE(h1.logicalIndex(1), 1);
    const QByteArray savedState = h1.saveState();

    // And a model with 3 columns, to apply that state upon
    QHeaderView h2(Qt::Horizontal);
    QStandardItemModel model2(1, 3);
    h2.setModel(&model2);
    QCOMPARE(h2.visualIndex(0), 0);
    QCOMPARE(h2.visualIndex(1), 1);
    QCOMPARE(h2.visualIndex(2), 2);
    QCOMPARE(h2.logicalIndex(0), 0);
    QCOMPARE(h2.logicalIndex(1), 1);
    QCOMPARE(h2.logicalIndex(2), 2);

    // When calling restoreState()
    QVERIFY(h2.restoreState(savedState));

    // Then the index mapping should still be as default
    QCOMPARE(h2.visualIndex(0), 0);
    QCOMPARE(h2.visualIndex(1), 1);
    QCOMPARE(h2.visualIndex(2), 2);
    QCOMPARE(h2.logicalIndex(0), 0);
    QCOMPARE(h2.logicalIndex(1), 1);
    QCOMPARE(h2.logicalIndex(2), 2);

    // And repainting shouldn't crash
    h2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&h2));
}

void tst_QHeaderView::restoreBeforeSetModel()
{
    QHeaderView h2(Qt::Horizontal);
    const QByteArray s1 = savedState();
    // First restore
    QVERIFY(h2.restoreState(s1));
    // Then setModel
    QStandardItemModel model(4, 4);
    h2.setModel(&model);

    // Check the result
    QCOMPARE(h2.logicalIndex(0), 2);
    QCOMPARE(h2.logicalIndex(2), 0);
    QCOMPARE(h2.sectionSize(1), 10);
    QCOMPARE(h2.sortIndicatorSection(), 2);
    QCOMPARE(h2.sortIndicatorOrder(), Qt::DescendingOrder);
    QCOMPARE(h2.isSortIndicatorShown(), true);
    QVERIFY(!h2.isSectionHidden(2));
    QVERIFY(h2.isSectionHidden(3));
    QCOMPARE(h2.hiddenSectionCount(), 1);
}

void tst_QHeaderView::defaultSectionSizeTest()
{
#if defined Q_OS_WINRT
    QSKIP("Fails on WinRT - QTBUG-73309");
#endif

    // Setup
    QTableView qtv;
    QHeaderView *hv = qtv.verticalHeader();
    hv->setMinimumSectionSize(10);
    hv->setDefaultSectionSize(99); // Set it to a value different from defaultSize.
    QStandardItemModel amodel(4, 4);
    qtv.setModel(&amodel);
    QCOMPARE(hv->sectionSize(0), 99);
    QCOMPARE(hv->visualIndexAt(50), 0); // <= also make sure that indexes are calculated
    hv->setDefaultSectionSize(40); // Set it to a value different from defaultSize.
    QCOMPARE(hv->visualIndexAt(50), 1);

    const int defaultSize = 26;
    hv->setDefaultSectionSize(defaultSize + 1); // Set it to a value different from defaultSize.

    // no hidden Sections
    hv->resizeSection(1, 0);
    hv->setDefaultSectionSize(defaultSize);
    QCOMPARE(hv->sectionSize(1), defaultSize);

    // with hidden sections
    hv->resizeSection(1, 0);
    hv->hideSection(2);
    hv->setDefaultSectionSize(defaultSize);

    QVERIFY(hv->sectionSize(0) == defaultSize); // trivial case.
    QVERIFY(hv->sectionSize(1) == defaultSize); // just sized 0. Now it should be 10
    QVERIFY(hv->sectionSize(2) == 0); // section is hidden. It should not be resized.
}

class TestHeaderViewStyle : public QProxyStyle
{
    Q_OBJECT
public:
    using QProxyStyle::QProxyStyle;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override
    {
        if (metric == QStyle::PM_HeaderDefaultSectionSizeHorizontal)
            return horizontalSectionSize;
        else
            return QProxyStyle::pixelMetric(metric, option, widget);
    }
    int horizontalSectionSize = 100;
};

void tst_QHeaderView::defaultSectionSizeTestStyles()
{
    TestHeaderViewStyle style1;
    TestHeaderViewStyle style2;
    style1.horizontalSectionSize = 100;
    style2.horizontalSectionSize = 200;

    QHeaderView hv(Qt::Horizontal);
    hv.setStyle(&style1);
    QCOMPARE(hv.defaultSectionSize(), style1.horizontalSectionSize);
    hv.setStyle(&style2);
    QCOMPARE(hv.defaultSectionSize(), style2.horizontalSectionSize);
    hv.setDefaultSectionSize(70);
    QCOMPARE(hv.defaultSectionSize(), 70);
    hv.setStyle(&style1);
    QCOMPARE(hv.defaultSectionSize(), 70);
    hv.resetDefaultSectionSize();
    QCOMPARE(hv.defaultSectionSize(), style1.horizontalSectionSize);
    hv.setStyle(&style2);
    QCOMPARE(hv.defaultSectionSize(), style2.horizontalSectionSize);
}

void tst_QHeaderView::defaultAlignment_data()
{
    QTest::addColumn<Qt::Orientation>("direction");
    QTest::addColumn<Qt::Alignment>("initial");
    QTest::addColumn<Qt::Alignment>("alignment");

    QTest::newRow("horizontal right aligned")
        << Qt::Horizontal
        << Qt::Alignment(Qt::AlignCenter)
        << Qt::Alignment(Qt::AlignRight);

    QTest::newRow("horizontal left aligned")
        << Qt::Horizontal
        << Qt::Alignment(Qt::AlignCenter)
        << Qt::Alignment(Qt::AlignLeft);

    QTest::newRow("vertical right aligned")
        << Qt::Vertical
        << Qt::Alignment(Qt::AlignLeft|Qt::AlignVCenter)
        << Qt::Alignment(Qt::AlignRight);

    QTest::newRow("vertical left aligned")
        << Qt::Vertical
        << Qt::Alignment(Qt::AlignLeft|Qt::AlignVCenter)
        << Qt::Alignment(Qt::AlignLeft);
}

void tst_QHeaderView::defaultAlignment()
{
    QFETCH(Qt::Orientation, direction);
    QFETCH(Qt::Alignment, initial);
    QFETCH(Qt::Alignment, alignment);

    SimpleModel m;

    QHeaderView header(direction);
    header.setModel(&m);

    QCOMPARE(header.defaultAlignment(), initial);
    header.setDefaultAlignment(alignment);
    QCOMPARE(header.defaultAlignment(), alignment);
}

void tst_QHeaderView::globalResizeMode_data()
{
    QTest::addColumn<Qt::Orientation>("direction");
    QTest::addColumn<QHeaderView::ResizeMode>("mode");
    QTest::addColumn<int>("insert");

    QTest::newRow("horizontal ResizeToContents 0")
        << Qt::Horizontal
        << QHeaderView::ResizeToContents
        << 0;
}

void tst_QHeaderView::globalResizeMode()
{
    QFETCH(Qt::Orientation, direction);
    QFETCH(QHeaderView::ResizeMode, mode);
    QFETCH(int, insert);

    QStandardItemModel m(4, 4);
    QHeaderView h(direction);
    h.setModel(&m);

    h.setSectionResizeMode(mode);
    m.insertRow(insert);
    for (int i = 0; i < h.count(); ++i)
        QCOMPARE(h.sectionResizeMode(i), mode);
}


void tst_QHeaderView::sectionPressedSignal_data()
{
    QTest::addColumn<Qt::Orientation>("direction");
    QTest::addColumn<bool>("clickable");
    QTest::addColumn<int>("count");

    QTest::newRow("horizontal unclickable 0")
        << Qt::Horizontal
        << false
        << 0;

    QTest::newRow("horizontal clickable 1")
        << Qt::Horizontal
        << true
        << 1;
}

void tst_QHeaderView::sectionPressedSignal()
{
    QFETCH(Qt::Orientation, direction);
    QFETCH(bool, clickable);
    QFETCH(int, count);

    QStandardItemModel m(4, 4);
    QHeaderView h(direction);

    h.setModel(&m);
    h.show();
    h.setSectionsClickable(clickable);

    QSignalSpy spy(&h, &QHeaderView::sectionPressed);

    QCOMPARE(spy.count(), 0);
    QTest::mousePress(h.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QCOMPARE(spy.count(), count);
}

void tst_QHeaderView::sectionClickedSignal()
{
    QFETCH(Qt::Orientation, direction);
    QFETCH(bool, clickable);
    QFETCH(int, count);

    QStandardItemModel m(4, 4);
    QHeaderView h(direction);

    h.setModel(&m);
    h.show();
    h.setSectionsClickable(clickable);
    h.setSortIndicatorShown(true);

    QSignalSpy spy(&h, &QHeaderView::sectionClicked);
    QSignalSpy spy2(&h, &QHeaderView::sortIndicatorChanged);

    QCOMPARE(spy.count(), 0);
    QCOMPARE(spy2.count(), 0);
    QTest::mouseClick(h.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QCOMPARE(spy.count(), count);
    QCOMPARE(spy2.count(), count);

    //now let's try with the sort indicator hidden (the result should be the same
    spy.clear();
    spy2.clear();
    h.setSortIndicatorShown(false);
    QTest::mouseClick(h.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QCOMPARE(spy.count(), count);
    QCOMPARE(spy2.count(), count);
}

void tst_QHeaderView::defaultSectionSize_data()
{
    QTest::addColumn<Qt::Orientation>("direction");
    QTest::addColumn<int>("oldDefaultSize");
    QTest::addColumn<int>("newDefaultSize");

    //QTest::newRow("horizontal,-5") << int(Qt::Horizontal) << 100 << -5;
    QTest::newRow("horizontal, 0") << Qt::Horizontal << 100 << 0;
    QTest::newRow("horizontal, 5") << Qt::Horizontal << 100 << 5;
    QTest::newRow("horizontal,25") << Qt::Horizontal << 100 << 5;
}

void tst_QHeaderView::defaultSectionSize()
{
    QFETCH(Qt::Orientation, direction);
    QFETCH(int, oldDefaultSize);
    QFETCH(int, newDefaultSize);

    QStandardItemModel m(4, 4);
    QHeaderView h(direction);

    h.setModel(&m);
    h.setMinimumSectionSize(0);

    QCOMPARE(h.defaultSectionSize(), oldDefaultSize);
    h.setDefaultSectionSize(newDefaultSize);
    QCOMPARE(h.defaultSectionSize(), newDefaultSize);
    h.reset();
    for (int i = 0; i < h.count(); ++i)
        QCOMPARE(h.sectionSize(i), newDefaultSize);
}

void tst_QHeaderView::hideAndInsert_data()
{
    QTest::addColumn<Qt::Orientation>("direction");
    QTest::addColumn<int>("hide");
    QTest::addColumn<int>("insert");
    QTest::addColumn<int>("hidden");

    QTest::newRow("horizontal, 0, 0") << Qt::Horizontal << 0 << 0 << 1;
}

void tst_QHeaderView::hideAndInsert()
{
    QFETCH(Qt::Orientation, direction);
    QFETCH(int, hide);
    QFETCH(int, insert);
    QFETCH(int, hidden);

    QStandardItemModel m(4, 4);
    QHeaderView h(direction);
    h.setModel(&m);
    h.setSectionHidden(hide, true);

    if (direction == Qt::Vertical)
        m.insertRow(insert);
    else
        m.insertColumn(insert);

    for (int i = 0; i < h.count(); ++i)
        QCOMPARE(h.isSectionHidden(i), i == hidden);
}

void tst_QHeaderView::removeSection()
{
    const int hidden = 3; //section that will be hidden

    QStringListModel model({ "0", "1", "2", "3", "4", "5", "6" });
    QHeaderView view(Qt::Vertical);
    view.setModel(&model);
    view.hideSection(hidden);
    view.hideSection(1);
    model.removeRow(1);
    view.show();

    for(int i = 0; i < view.count(); i++) {
        if (i == (hidden - 1)) { //-1 because we removed a row in the meantime
            QCOMPARE(view.sectionSize(i), 0);
            QVERIFY(view.isSectionHidden(i));
        } else {
            QCOMPARE(view.sectionSize(i), view.defaultSectionSize() );
            QVERIFY(!view.isSectionHidden(i));
        }
    }
}

void tst_QHeaderView::preserveHiddenSectionWidth()
{
    QStringListModel model({ "0", "1", "2", "3" });
    QHeaderView view(Qt::Vertical);
    view.setModel(&model);
    view.resizeSection(0, 100);
    view.resizeSection(1, 10);
    view.resizeSection(2, 50);
    view.setSectionResizeMode(3, QHeaderView::Stretch);
    view.show();

    view.hideSection(2);
    model.removeRow(1);
    view.showSection(1);
    QCOMPARE(view.sectionSize(0), 100);
    QCOMPARE(view.sectionSize(1), 50);

    view.hideSection(1);
    model.insertRow(1);
    view.showSection(2);
    QCOMPARE(view.sectionSize(0), 100);
    QCOMPARE(view.sectionSize(1), view.defaultSectionSize());
    QCOMPARE(view.sectionSize(2), 50);
}

void tst_QHeaderView::invisibleStretchLastSection()
{
#ifdef Q_OS_WINRT
    QSKIP("Fails on WinRT - QTBUG-68297");
#endif
    int count = 6;
    QStandardItemModel model(1, count);
    QHeaderView view(Qt::Horizontal);
    view.setModel(&model);
    int height = view.height();

    view.resize(view.defaultSectionSize() * (count / 2), height); // don't show all sections
    view.show();
    view.setStretchLastSection(true);
    // stretch section is not visible; it should not be stretched
    for (int i = 0; i < count; ++i)
        QCOMPARE(view.sectionSize(i), view.defaultSectionSize());

    view.resize(view.defaultSectionSize() * (count + 1), height); // give room to stretch

    // stretch section is visible; it should be stretched
    for (int i = 0; i < count - 1; ++i)
        QCOMPARE(view.sectionSize(i), view.defaultSectionSize());
    QCOMPARE(view.sectionSize(count - 1), view.defaultSectionSize() * 2);
}

void tst_QHeaderView::noSectionsWithNegativeSize()
{
    QStandardItemModel m(4, 4);
    QHeaderView h(Qt::Horizontal);
    h.setModel(&m);
    h.resizeSection(1, -5);
    QVERIFY(h.sectionSize(1) >= 0); // Sections with negative sizes not well defined.
}

void tst_QHeaderView::emptySectionSpan()
{
    QHeaderViewPrivate::SectionItem section;
    QCOMPARE(section.sectionSize(), 0);
}

void tst_QHeaderView::task236450_hidden_data()
{
    QTest::addColumn<IntList>("hide1");
    QTest::addColumn<IntList>("hide2");

    QTest::newRow("set 1") << (IntList{ 1, 3 })
                           << (IntList{ 1, 5 });

    QTest::newRow("set 2") << (IntList{ 2, 3 })
                           << (IntList{ 1, 5 });

    QTest::newRow("set 3") << (IntList{ 0, 2, 4 })
                           << (IntList{ 2, 3, 5 });

}

void tst_QHeaderView::task236450_hidden()
{
    QFETCH(const IntList, hide1);
    QFETCH(const IntList, hide2);

    QStringListModel model({ "0", "1", "2", "3", "4", "5" });
    protected_QHeaderView view(Qt::Vertical);
    view.setModel(&model);
    view.show();

    for (int i : hide1)
        view.hideSection(i);

    QCOMPARE(view.hiddenSectionCount(), hide1.count());
    for (int i = 0; i < 6; i++)
        QCOMPARE(!view.isSectionHidden(i), !hide1.contains(i));

    view.setDefaultSectionSize(2);
    view.scheduleDelayedItemsLayout();
    view.executeDelayedItemsLayout(); //force to do a relayout

    QCOMPARE(view.hiddenSectionCount(), hide1.count());
    for (int i = 0; i < 6; i++) {
        QCOMPARE(!view.isSectionHidden(i), !hide1.contains(i));
        view.setSectionHidden(i, hide2.contains(i));
    }

    QCOMPARE(view.hiddenSectionCount(), hide2.count());
    for (int i = 0; i < 6; i++)
        QCOMPARE(!view.isSectionHidden(i), !hide2.contains(i));
}

void tst_QHeaderView::task248050_hideRow()
{
    //this is the sequence of events that make the task fail
    protected_QHeaderView header(Qt::Vertical);
    QStandardItemModel model(0, 1);
    header.setMinimumSectionSize(0);    // system default min size can be to large
    header.setStretchLastSection(false);
    header.setDefaultSectionSize(17);
    header.setModel(&model);
    header.doItemsLayout();

    model.setRowCount(3);

    QCOMPARE(header.sectionPosition(2), 17*2);

    header.hideSection(1);
    QCOMPARE(header.sectionPosition(2), 17);

    QTest::qWait(100);
    //the size of the section shouldn't have changed
    QCOMPARE(header.sectionPosition(2), 17);
}


//returns 0 if everything is fine.
static int checkHeaderViewOrder(const QHeaderView *view, const IntList &expected)
{
    if (view->count() != expected.count())
        return 1;

    for (int i = 0; i < expected.count(); i++) {
        if (view->logicalIndex(i) != expected.at(i))
            return i + 10;
        if (view->visualIndex(expected.at(i)) != i)
            return i + 100;
    }
    return 0;
}


void tst_QHeaderView::QTBUG6058_reset()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QStringListModel model1({ "0", "1", "2", "3", "4", "5" });
    QStringListModel model2({ "a", "b", "c" });
    QSortFilterProxyModel proxy;

    QHeaderView view(Qt::Vertical);
    view.setModel(&proxy);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(QTest::qWaitForWindowActive(&view));

    proxy.setSourceModel(&model1);
    view.swapSections(0, 2);
    view.swapSections(1, 4);
    IntList expectedOrder{2, 4, 0, 3, 1, 5};
    QTRY_COMPARE(checkHeaderViewOrder(&view, expectedOrder) , 0);

    proxy.setSourceModel(&model2);
    expectedOrder = {2, 0, 1};
    QTRY_COMPARE(checkHeaderViewOrder(&view, expectedOrder) , 0);

    proxy.setSourceModel(&model1);
    expectedOrder = {2, 0, 1, 3, 4, 5};
    QTRY_COMPARE(checkHeaderViewOrder(&view, expectedOrder) , 0);
}

void tst_QHeaderView::QTBUG7833_sectionClicked()
{
    QTableView tv;
    QStandardItemModel *sim = new QStandardItemModel(&tv);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(&tv);
    proxyModel->setSourceModel(sim);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    QList<QStandardItem *> row;
    for (char i = 0; i < 12; i++)
        row.append(new QStandardItem(QString(QLatin1Char('A' + i))));
    sim->appendRow(row);
    row.clear();
    for (char i = 12; i > 0; i--)
        row.append(new QStandardItem(QString(QLatin1Char('A' + i))));
    sim->appendRow(row);

    tv.setSortingEnabled(true);
    tv.horizontalHeader()->setSortIndicatorShown(true);
    tv.horizontalHeader()->setSectionsClickable(true);
    tv.horizontalHeader()->setStretchLastSection(true);
    tv.horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    tv.setModel(proxyModel);
    const int section4Size = tv.horizontalHeader()->sectionSize(4) + 1;
    const int section5Size = section4Size + 1;
    tv.horizontalHeader()->resizeSection(4, section4Size);
    tv.horizontalHeader()->resizeSection(5, section5Size);
    tv.setColumnHidden(5, true);
    tv.setColumnHidden(6, true);
    tv.horizontalHeader()->swapSections(8, 10);
    tv.sortByColumn(1, Qt::AscendingOrder);

    QCOMPARE(tv.isColumnHidden(5), true);
    QCOMPARE(tv.isColumnHidden(6), true);
    QCOMPARE(tv.horizontalHeader()->sectionsMoved(), true);
    QCOMPARE(tv.horizontalHeader()->logicalIndex(8), 10);
    QCOMPARE(tv.horizontalHeader()->logicalIndex(10), 8);
    QCOMPARE(tv.horizontalHeader()->sectionSize(4), section4Size);
    tv.setColumnHidden(5, false);   // unhide, section size must be properly restored
    QCOMPARE(tv.horizontalHeader()->sectionSize(5), section5Size);
    tv.setColumnHidden(5, true);

    QSignalSpy clickedSpy(tv.horizontalHeader(), &QHeaderView::sectionClicked);
    QSignalSpy pressedSpy(tv.horizontalHeader(), &QHeaderView::sectionPressed);


    QTest::mouseClick(tv.horizontalHeader()->viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(tv.horizontalHeader()->sectionViewportPosition(11) +
                             tv.horizontalHeader()->sectionSize(11) / 2, 5));
    QCOMPARE(clickedSpy.count(), 1);
    QCOMPARE(pressedSpy.count(), 1);
    QCOMPARE(clickedSpy.at(0).at(0).toInt(), 11);
    QCOMPARE(pressedSpy.at(0).at(0).toInt(), 11);

    QTest::mouseClick(tv.horizontalHeader()->viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(tv.horizontalHeader()->sectionViewportPosition(8) +
                             tv.horizontalHeader()->sectionSize(0) / 2, 5));

    QCOMPARE(clickedSpy.count(), 2);
    QCOMPARE(pressedSpy.count(), 2);
    QCOMPARE(clickedSpy.at(1).at(0).toInt(), 8);
    QCOMPARE(pressedSpy.at(1).at(0).toInt(), 8);

    QTest::mouseClick(tv.horizontalHeader()->viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(tv.horizontalHeader()->sectionViewportPosition(0) +
                             tv.horizontalHeader()->sectionSize(0) / 2, 5));

    QCOMPARE(clickedSpy.count(), 3);
    QCOMPARE(pressedSpy.count(), 3);
    QCOMPARE(clickedSpy.at(2).at(0).toInt(), 0);
    QCOMPARE(pressedSpy.at(2).at(0).toInt(), 0);
}

void tst_QHeaderView::checkLayoutChangeEmptyModel()
{
    QtTestModel tm(0, 11);
    QTableView tv;
    tv.verticalHeader()->setStretchLastSection(true);
    tv.setModel(&tm);

    const int section4Size = tv.horizontalHeader()->sectionSize(4) + 1;
    const int section5Size = section4Size + 1;
    tv.horizontalHeader()->resizeSection(4, section4Size);
    tv.horizontalHeader()->resizeSection(5, section5Size);
    tv.setColumnHidden(5, true);
    tv.setColumnHidden(6, true);
    tv.horizontalHeader()->swapSections(8, 10);

    tv.sortByColumn(1, Qt::AscendingOrder);
    tm.emitLayoutChanged();

    QCOMPARE(tv.isColumnHidden(5), true);
    QCOMPARE(tv.isColumnHidden(6), true);
    QCOMPARE(tv.horizontalHeader()->sectionsMoved(), true);
    QCOMPARE(tv.horizontalHeader()->logicalIndex(8), 10);
    QCOMPARE(tv.horizontalHeader()->logicalIndex(10), 8);
    QCOMPARE(tv.horizontalHeader()->sectionSize(4), section4Size);
    tv.setColumnHidden(5, false);   // unhide, section size must be properly restored
    QCOMPARE(tv.horizontalHeader()->sectionSize(5), section5Size);
    tv.setColumnHidden(5, true);

    // adjust
    tm.rows = 3;
    tm.emitLayoutChanged();

    // remove the row used for QPersistenModelIndexes
    tm.emitLayoutChangedWithRemoveFirstRow();
    QCOMPARE(tv.isColumnHidden(5), true);
    QCOMPARE(tv.isColumnHidden(6), true);
    QCOMPARE(tv.horizontalHeader()->sectionsMoved(), true);
    QCOMPARE(tv.horizontalHeader()->logicalIndex(8), 10);
    QCOMPARE(tv.horizontalHeader()->logicalIndex(10), 8);
    QCOMPARE(tv.horizontalHeader()->sectionSize(4), section4Size);
    tv.setColumnHidden(5, false);   // unhide, section size must be properly restored
    QCOMPARE(tv.horizontalHeader()->sectionSize(5), section5Size);
    tv.setColumnHidden(5, true);
}

void tst_QHeaderView::QTBUG8650_crashOnInsertSections()
{
    QStringList headerLabels;
    QHeaderView view(Qt::Horizontal);
    QStandardItemModel model(2, 2);
    view.setModel(&model);
    view.moveSection(1, 0);
    view.hideSection(0);

    model.insertColumn(0, { new QStandardItem("c") });
}

static void setModelTexts(QStandardItemModel *model)
{
    const int columnCount = model->columnCount();
    for (int i = 0, rowCount = model->rowCount(); i < rowCount; ++i) {
        const QString prefix = QLatin1String("item [") + QString::number(i) + QLatin1Char(',');
        for (int j = 0; j < columnCount; ++j)
            model->setData(model->index(i, j), prefix + QString::number(j) + QLatin1Char(']'));
    }
}

void tst_QHeaderView::QTBUG12268_hiddenMovedSectionSorting()
{
    QTableView view; // ### this test fails on QTableView &view = *m_tableview; !? + shadowing view member
    QStandardItemModel model(4, 3);
    setModelTexts(&model);
    view.setModel(&model);
    view.horizontalHeader()->setSectionsMovable(true);
    view.setSortingEnabled(true);
    view.sortByColumn(1, Qt::AscendingOrder);
    view.horizontalHeader()->moveSection(0,2);
    view.setColumnHidden(1, true);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QCOMPARE(view.horizontalHeader()->hiddenSectionCount(), 1);
    QTest::mouseClick(view.horizontalHeader()->viewport(), Qt::LeftButton);
    QCOMPARE(view.horizontalHeader()->hiddenSectionCount(), 1);
}

void tst_QHeaderView::QTBUG14242_hideSectionAutoSize()
{
    QTableView qtv;
    QStandardItemModel amodel(4, 4);
    qtv.setModel(&amodel);
    QHeaderView *hv = qtv.verticalHeader();
    hv->setDefaultSectionSize(25);
    hv->setSectionResizeMode(QHeaderView::ResizeToContents);
    qtv.show();

    hv->hideSection(0);
    int afterlength = hv->length();

    int calced_length = 0;
    for (int u = 0; u < hv->count(); ++u)
        calced_length += hv->sectionSize(u);

    QCOMPARE(calced_length, afterlength);
}

void tst_QHeaderView::QTBUG50171_visualRegionForSwappedItems()
{
    protected_QHeaderView headerView(Qt::Horizontal);
    QtTestModel model(2, 3);
    headerView.setModel(&model);
    headerView.swapSections(1, 2);
    headerView.hideSection(0);
    headerView.testVisualRegionForSelection();
}

class QTBUG53221_Model : public QAbstractItemModel
{
    Q_OBJECT
public:
    void insertRowAtBeginning()
    {
        Q_EMIT layoutAboutToBeChanged();
        m_displayNames.insert(0, QStringLiteral("Item %1").arg(m_displayNames.count()));
        // Rows are always inserted at the beginning, so move all others.
        const auto pl = persistentIndexList();
        // The vertical header view will have a persistent index stored here on the second call to insertRowAtBeginning.
        for (const QModelIndex &persIndex : pl)
            changePersistentIndex(persIndex, index(persIndex.row() + 1, persIndex.column(), persIndex.parent()));
        Q_EMIT layoutChanged();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        return (role == Qt::DisplayRole) ? m_displayNames.at(index.row()) : QVariant();
    }

    QModelIndex index(int row, int column, const QModelIndex &) const override { return createIndex(row, column); }
    QModelIndex parent(const QModelIndex &) const override { return QModelIndex(); }
    int rowCount(const QModelIndex &) const override { return m_displayNames.count(); }
    int columnCount(const QModelIndex &) const override { return 1; }

private:
    QStringList m_displayNames;
};

void tst_QHeaderView::QTBUG53221_assertShiftHiddenRow()
{
    QTableView tableView;
    QTBUG53221_Model modelTableView;
    tableView.setModel(&modelTableView);

    modelTableView.insertRowAtBeginning();
    tableView.setRowHidden(0, true);
    QCOMPARE(tableView.verticalHeader()->isSectionHidden(0), true);
    modelTableView.insertRowAtBeginning();
    QCOMPARE(tableView.verticalHeader()->isSectionHidden(0), false);
    QCOMPARE(tableView.verticalHeader()->isSectionHidden(1), true);
    modelTableView.insertRowAtBeginning();
    QCOMPARE(tableView.verticalHeader()->isSectionHidden(0), false);
    QCOMPARE(tableView.verticalHeader()->isSectionHidden(1), false);
    QCOMPARE(tableView.verticalHeader()->isSectionHidden(2), true);
}

void tst_QHeaderView::QTBUG75615_sizeHintWithStylesheet()
{
    QTableView tableView;
    QStandardItemModel model(1, 1);
    tableView.setModel(&model);
    tableView.show();

    const auto headerView = tableView.horizontalHeader();
    const auto oldSizeHint = headerView->sizeHint();
    QVERIFY(oldSizeHint.isValid());

    tableView.setStyleSheet("QTableView QHeaderView::section { height: 100px;}");
    QCOMPARE(headerView->sizeHint().width(), oldSizeHint.width());
    QCOMPARE(headerView->sizeHint().height(), 100);

    tableView.setStyleSheet("QTableView QHeaderView::section { width: 100px;}");
    QCOMPARE(headerView->sizeHint().height(), oldSizeHint.height());
    QCOMPARE(headerView->sizeHint().width(), 100);
}

void protected_QHeaderView::testVisualRegionForSelection()
{
    QRegion r = visualRegionForSelection(QItemSelection(model()->index(1, 0), model()->index(1, 2)));
    QCOMPARE(r.boundingRect().contains(QRect(1, 1, length() - 2, 1)), true);
}

void tst_QHeaderView::ensureNoIndexAtLength()
{
    QTableView qtv;
    QStandardItemModel amodel(4, 4);
    qtv.setModel(&amodel);
    QHeaderView *hv = qtv.verticalHeader();
    QCOMPARE(hv->visualIndexAt(hv->length()), -1);
    hv->resizeSection(hv->count() - 1, 0);
    QCOMPARE(hv->visualIndexAt(hv->length()), -1);
}

void tst_QHeaderView::offsetConsistent()
{
    // Ensure that a hidden section 'far away'
    // does not affect setOffsetToSectionPosition ..
    const int sectionToHide = 513;
    QTableView qtv;
    QStandardItemModel amodel(1000, 4);
    qtv.setModel(&amodel);
    QHeaderView *hv = qtv.verticalHeader();
    for (int u = 0; u < 100; u += 2)
        hv->resizeSection(u, 0);
    hv->setOffsetToSectionPosition(150);
    int offset1 = hv->offset();
    hv->hideSection(sectionToHide);
    hv->setOffsetToSectionPosition(150);
    int offset2 = hv->offset();
    QCOMPARE(offset1, offset2);
    // Ensure that hidden indexes (still) is considered.
    hv->resizeSection(sectionToHide, hv->sectionSize(200) * 2);
    hv->setOffsetToSectionPosition(800);
    offset1 = hv->offset();
    hv->showSection(sectionToHide);
    hv->setOffsetToSectionPosition(800);
    offset2 = hv->offset();
    QVERIFY(offset2 > offset1);
}

void tst_QHeaderView::initialSortOrderRole()
{
    QTableView view; // ### Shadowing member view (of type QHeaderView)
    QStandardItemModel model(4, 3);
    setModelTexts(&model);
    QStandardItem *ascendingItem = new QStandardItem();
    QStandardItem *descendingItem = new QStandardItem();
    ascendingItem->setData(Qt::AscendingOrder, Qt::InitialSortOrderRole);
    descendingItem->setData(Qt::DescendingOrder, Qt::InitialSortOrderRole);
    model.setHorizontalHeaderItem(1, ascendingItem);
    model.setHorizontalHeaderItem(2, descendingItem);
    view.setModel(&model);
    view.setSortingEnabled(true);
    view.sortByColumn(0, Qt::AscendingOrder);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const int column1Pos = view.horizontalHeader()->sectionViewportPosition(1) + 5; // +5 not to be on the handle
    QTest::mouseClick(view.horizontalHeader()->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(column1Pos, 0));
    QCOMPARE(view.horizontalHeader()->sortIndicatorSection(), 1);
    QCOMPARE(view.horizontalHeader()->sortIndicatorOrder(), Qt::AscendingOrder);

    const int column2Pos = view.horizontalHeader()->sectionViewportPosition(2) + 5; // +5 not to be on the handle
    QTest::mouseClick(view.horizontalHeader()->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(column2Pos, 0));
    QCOMPARE(view.horizontalHeader()->sortIndicatorSection(), 2);
    QCOMPARE(view.horizontalHeader()->sortIndicatorOrder(), Qt::DescendingOrder);

    const int column0Pos = view.horizontalHeader()->sectionViewportPosition(0) + 5; // +5 not to be on the handle
    QTest::mouseClick(view.horizontalHeader()->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(column0Pos, 0));
    QCOMPARE(view.horizontalHeader()->sortIndicatorSection(), 0);
    QCOMPARE(view.horizontalHeader()->sortIndicatorOrder(), Qt::AscendingOrder);
}

const bool block_some_signals = true; // The test should also work with this set to false
const int rowcount = 500;
const int colcount = 10;

static inline QString istr(int n, bool comma = true)
{
    QString s;
    s.setNum(n);
    if (comma)
        s += ", ";
    return s;
}

void tst_QHeaderView::calculateAndCheck(int cppline, const int precalced_comparedata[])
{
    qint64 endtimer = timer.elapsed();
    const bool silentmode = true;
    if (!silentmode)
        qDebug().nospace() << "(Time:" << endtimer << ")";

    QString sline;
    sline.setNum(cppline - 1);

    const int p1 = 3133777;      // just a prime (maybe not that random ;) )
    const int p2 = 135928393;    // just a random large prime - a bit less than signed 32-bit

    int sum_visual = 0;
    int sum_logical = 0;
    int sum_size = 0;
    int sum_hidden_size = 0;
    int sum_lookup_visual = 0;
    int sum_lookup_logical = 0;

    int chk_visual = 1;
    int chk_logical = 1;
    int chk_sizes = 1;
    int chk_hidden_size = 1;
    int chk_lookup_visual = 1;
    int chk_lookup_logical = 1;

    int header_lenght = 0;
    int lastindex = view->count() - 1;

    // calculate information based on index
    for (int i = 0; i <= lastindex; ++i) {
        int visual = view->visualIndex(i);
        int logical = view->logicalIndex(i);
        int ssize = view->sectionSize(i);

        sum_visual += visual;
        sum_logical += logical;
        sum_size += ssize;

        if (visual >= 0) {
            chk_visual %= p2;
            chk_visual *= (visual + 1) * (i + 1) * p1;
        }

        if (logical >= 0) {
            chk_logical %= p2;
            chk_logical *= (logical + 1) * (i + 1 + (logical != i) ) * p1;
        }

        if (ssize >= 0) {
            chk_sizes %= p2;
            chk_sizes *= ( (ssize + 2) * (i + 1) * p1);
        }

        if (view->isSectionHidden(i)) {
            view->showSection(i);
            int hiddensize = view->sectionSize(i);
            sum_hidden_size += hiddensize;
            chk_hidden_size %= p2;
            chk_hidden_size += ( (hiddensize + 1) * (i + 1) * p1);
            // (hiddensize + 1) in the above to differ between hidden and size 0
            // Though it can be changed (why isn't sections with size 0 hidden?)

            view->hideSection(i);
        }
    }

    // lookup indexes by pixel position
    const int max_lookup_count = 500;
    int lookup_to = view->height() + 1;
    if (lookup_to > max_lookup_count)
        lookup_to = max_lookup_count; // We limit this lookup - not to spend years when testing.
                                      // Notice that lookupTest also has its own extra test
    for (int u = 0; u < max_lookup_count; ++u) {
        int visu = view->visualIndexAt(u);
        int logi = view->logicalIndexAt(u);
        sum_lookup_visual += logi;
        sum_lookup_logical += visu;
        chk_lookup_visual %= p2;
        chk_lookup_visual *= ( (u + 1) * p1 * (visu + 2));
        chk_lookup_logical %= p2;
        chk_lookup_logical *=  ( (u + 1) * p1 * (logi + 2));
    }
    header_lenght = view->length();

    // visual and logical indexes.
    int sum_to_last_index = (lastindex * (lastindex + 1)) / 2; // == 0 + 1 + 2 + 3 + ... + lastindex

    const bool write_calced_data = false;  // Do not write calculated output (unless the test fails)
    if (write_calced_data) {
        qDebug().nospace() << "(" << cppline - 1 << ")"  // << " const int precalced[] = "
                           << " {" << chk_visual << ", " << chk_logical << ", " << chk_sizes << ", " << chk_hidden_size
                           << ", " << chk_lookup_visual << ", " << chk_lookup_logical << ", " << header_lenght << "};";
    }

    const bool sanity_checks = true;
    if (sanity_checks) {
        QString msg = QString("sanity problem at ") + sline;
        const QScopedArrayPointer<char> holder(QTest::toString(msg));
        const auto verifytext = holder.data();

        QVERIFY2(m_tableview->model()->rowCount() == view->count() , verifytext);
        QVERIFY2(view->visualIndex(lastindex + 1) <= 0, verifytext);       // there is no such index in model
        QVERIFY2(view->logicalIndex(lastindex + 1) <= 0, verifytext);      // there is no such index in model.
        QVERIFY2(view->logicalIndex(lastindex + 1) <= 0, verifytext);      // there is no such index in model.
        QVERIFY2(lastindex < 0 || view->visualIndex(0) >= 0, verifytext);   // no rows or legal index
        QVERIFY2(lastindex < 0 || view->logicalIndex(0) >= 0, verifytext);  // no rows or legal index
        QVERIFY2(lastindex < 0 || view->visualIndex(lastindex) >= 0, verifytext);  // no rows or legal index
        QVERIFY2(lastindex < 0 || view->logicalIndex(lastindex) >= 0, verifytext); // no rows or legal index
        QVERIFY2(view->visualIndexAt(-1) == -1, verifytext);
        QVERIFY2(view->logicalIndexAt(-1) == -1, verifytext);
        QVERIFY2(view->visualIndexAt(view->length()) == -1, verifytext);
        QVERIFY2(view->logicalIndexAt(view->length()) == -1, verifytext);
        QVERIFY2(sum_visual == sum_logical, verifytext);
        QVERIFY2(sum_to_last_index == sum_logical, verifytext);
    }

    // Semantic test
    const bool check_semantics = true; // Otherwise there is no 'real' test
    if (!check_semantics)
        return;

    const int *x = precalced_comparedata;

    QString msg = "semantic problem at " + QString(__FILE__) + " (" + sline + ")";
    msg += "\nThe *expected* result was : {" + istr(x[0]) + istr(x[1]) + istr(x[2]) + istr(x[3])
        + istr(x[4]) + istr(x[5]) + istr(x[6], false) + QLatin1Char('}');
    msg += "\nThe calculated result was : {";
    msg += istr(chk_visual) + istr(chk_logical) + istr(chk_sizes) + istr(chk_hidden_size)
        + istr(chk_lookup_visual) + istr(chk_lookup_logical) + istr(header_lenght, false) + "};";

    const QScopedArrayPointer<char> holder(QTest::toString(msg));
    const auto verifytext = holder.data();

    QVERIFY2(chk_visual            == x[0], verifytext);
    QVERIFY2(chk_logical           == x[1], verifytext);
    QVERIFY2(chk_sizes             == x[2], verifytext);
    QVERIFY2(chk_hidden_size       == x[3], verifytext);
    QVERIFY2(chk_lookup_visual     == x[4], verifytext);
    QVERIFY2(chk_lookup_logical    == x[5], verifytext);
    QVERIFY2(header_lenght         == x[6], verifytext);
}

void tst_QHeaderView::setupTestData(bool also_use_reset_model)
{
    QTest::addColumn<bool>("updates_enabled");
    QTest::addColumn<bool>("special_prepare");
    QTest::addColumn<bool>("reset_model");

    if (also_use_reset_model) {
        QTest::newRow("no_updates+normal+reset")  << false << false << true;
        QTest::newRow("hasupdates+normal+reset")  << true << false << true;
        QTest::newRow("no_updates+special+reset") << false << true << true;
        QTest::newRow("hasupdates+special+reset") << true << true << true;
    }

    QTest::newRow("no_updates+normal")  << false << false << false;
    QTest::newRow("hasupdates+normal")  << true << false << false;
    QTest::newRow("no_updates+special") << false << true << false;
    QTest::newRow("hasupdates+special") << true << true << false;
}

void tst_QHeaderView::additionalInit()
{
    m_tableview->setVerticalHeader(view);

    QFETCH(bool, updates_enabled);
    QFETCH(bool, special_prepare);
    QFETCH(bool, reset_model);

    m_using_reset_model = reset_model;
    m_special_prepare = special_prepare;

    if (m_using_reset_model) {
        XResetModel *m = new XResetModel();
        m_tableview->setModel(m);
        delete model;
        model = m;
    } else {
        m_tableview->setModel(model);
    }

    const int default_section_size = 25;
    view->setDefaultSectionSize(default_section_size); // Important - otherwise there will be semantic changes

    model->clear();

    if (special_prepare) {

        for (int u = 0; u <= rowcount; ++u) // ensures fragmented spans in e.g Qt 4.7
            model->setRowCount(u);

        model->setColumnCount(colcount);
        view->swapSections(0, rowcount - 1);
        view->swapSections(0, rowcount - 1); // No real change - setup visual and log-indexes
        view->hideSection(rowcount - 1);
        view->showSection(rowcount - 1);  // No real change - (maybe init hidden vector)
    } else {
        model->setColumnCount(colcount);
        model->setRowCount(rowcount);
    }

    QString s;
    for (int i = 0; i < model->rowCount(); ++i) {
        model->setData(model->index(i, 0), QVariant(i));
        s.setNum(i);
        s += QLatin1Char('.');
        s += 'a' + (i % 25);
        model->setData(model->index(i, 1), QVariant(s));
    }
    m_tableview->setUpdatesEnabled(updates_enabled);
    view->blockSignals(block_some_signals);
    timer.start();
}

void tst_QHeaderView::logicalIndexAtTest()
{
    additionalInit();

    view->swapSections(4, 9); // Make sure that visual and logical Indexes are not just the same.

    int check1 = 0;
    int check2 = 0;
    for (int u = 0; u < model->rowCount(); ++u) {
        view->resizeSection(u, 10 + u % 30);
        int v = view->visualIndexAt(u * 29);
        view->visualIndexAt(u * 29);
        check1 += v;
        check2 += u * v;
    }
    view->resizeSection(0, 0); // Make sure that we have a 0 size section - before the result set
    view->setSectionHidden(6, true); // Make sure we have a real hidden section before result set

    //qDebug() << "logicalIndexAtTest" << check1 << check2;
    const int precalced_check1 = 106327;
    const int precalced_check2 = 29856418;
    QCOMPARE(precalced_check1, check1);
    QCOMPARE(precalced_check2, check2);

    const int precalced_results[] = { 1145298384, -1710423344, -650981936, 372919464, -1544372176, -426463328, 12124 };
    calculateAndCheck(__LINE__, precalced_results);
}

void tst_QHeaderView::visualIndexAtTest()
{
    additionalInit();

    view->swapSections(4, 9); // Make sure that visual and logical Indexes are not just the same.
    int check1 = 0;
    int check2 = 0;

    for (int u = 0; u < model->rowCount(); ++u) {
        view->resizeSection(u, 3 + u % 17);
        int v = view->visualIndexAt(u * 29);
        check1 += v;
        check2 += u * v;
    }

    view->resizeSection(1, 0); // Make sure that we have a 0 size section - before the result set
    view->setSectionHidden(5, true); // Make sure we have a real hidden section before result set

    //qDebug() << "visualIndexAtTest" << check1 << check2;
    const int precalced_check1 = 72665;
    const int precalced_check2 = 14015890;
    QCOMPARE(precalced_check1, check1);
    QCOMPARE(precalced_check2, check2);

    const int precalced_results[] = { 1145298384, -1710423344, -1457520212, 169223959, 557466160, -324939600, 5453 };
    calculateAndCheck(__LINE__, precalced_results);
}

void tst_QHeaderView::hideShowTest()
{
    additionalInit();

    for (int u = model->rowCount(); u >= 0; --u)
        if (u % 8 != 0)
            view->hideSection(u);

    for (int u = model->rowCount(); u >= 0; --u)
        if (u % 3 == 0)
            view->showSection(u);

    view->setSectionHidden(model->rowCount(), true); // invalid hide (should be ignored)
    view->setSectionHidden(-1, true); // invalid hide (should be ignored)

    const int precalced_results[] = { -1523279360, -1523279360, -1321506816, 2105322423, 1879611280, 1879611280, 5225 };
    calculateAndCheck(__LINE__, precalced_results);
}

void tst_QHeaderView::swapSectionsTest()
{
    additionalInit();

    for (int u = 0; u < rowcount / 2; ++u)
        view->swapSections(u, rowcount - u - 1);

    for (int u = 0; u < rowcount; u += 2)
        view->swapSections(u, u + 1);

    view->swapSections(0, model->rowCount()); // invalid swapsection (should be ignored)

    const int precalced_results[] = { -1536450048, -1774641430, -1347156568, 1, 1719705216, -240077576, 12500 };
    calculateAndCheck(__LINE__, precalced_results);
}

void tst_QHeaderView::moveSectionTest()
{
    additionalInit();

    for (int u = 1; u < 5; ++u)
        view->moveSection(u, model->rowCount() - u);

    view->moveSection(2, model->rowCount() / 2);
    view->moveSection(0, 10);
    view->moveSection(0, model->rowCount() - 10);

    view->moveSection(0, model->rowCount()); // invalid move (should be ignored)

    const int precalced_results[] = { 645125952, 577086896, -1347156568, 1, 1719705216, 709383416, 12500 };
    calculateAndCheck(__LINE__, precalced_results);
}

void tst_QHeaderView::defaultSizeTest()
{
    additionalInit();

    view->hideSection(rowcount / 2);
    int restore_to = view->defaultSectionSize();
    view->setDefaultSectionSize(restore_to + 5);

    const int precalced_results[] = { -1523279360, -1523279360, -1739688320, -1023807777, 997629696, 997629696, 14970 };
    calculateAndCheck(__LINE__, precalced_results);

    view->setDefaultSectionSize(restore_to);
}

void tst_QHeaderView::removeTest()
{
    additionalInit();

    view->swapSections(0, 5);
    model->removeRows(0, 1);   // remove one row
    model->removeRows(4, 10);
    model->setRowCount(model->rowCount() / 2 - 1);

    if (m_using_reset_model) {
        const int precalced_results[] = { 1741224292, -135269187, -569519837, 1, 1719705216, -1184395000, 6075 };
        calculateAndCheck(__LINE__, precalced_results);
    } else {
        const int precalced_results[] = { 289162397, 289162397, -569519837, 1, 1719705216, 1719705216, 6075 };
        calculateAndCheck(__LINE__, precalced_results);
    }
}

void tst_QHeaderView::insertTest()
{
    additionalInit();

    view->swapSections(0, model->rowCount() - 1);
    model->insertRows(0, 1);   // insert one row
    model->insertRows(4, 10);
    model->setRowCount(model->rowCount() * 2 - 1);

    if (m_using_reset_model) {
        const int precalced_results[] = { 2040508069, -1280266538, -150350734, 1, 1719705216, 1331312784, 25525 };
        calculateAndCheck(__LINE__, precalced_results);
    } else {
        const int precalced_results[] = { -1909447021, 339092083, -150350734, 1, 1719705216, -969712728, 25525 };
        calculateAndCheck(__LINE__, precalced_results);
    }
}

void tst_QHeaderView::mixedTests()
{
    additionalInit();

    model->setRowCount(model->rowCount() + 10);

    for (int u = 0; u < model->rowCount(); u += 2)
        view->swapSections(u, u + 1);

    view->moveSection(0, 5);

    for (int u = model->rowCount(); u >= 0; --u) {
        if (u % 5 != 0) {
            view->hideSection(u);
            QVERIFY(view->isSectionHidden(u));
        }
        if (u % 3 != 0) {
            view->showSection(u);
            QVERIFY(!view->isSectionHidden(u));
        }
    }

    model->insertRows(3, 7);
    model->removeRows(8, 3);
    model->setRowCount(model->rowCount() - 10);

    // the upper is not visible (when m_using_reset_model is true)
    // the lower 11 are modified due to insert/removeRows
    for (int u = model->rowCount() - 1; u >= 11; --u) {
        // when using reset, the hidden rows will *not* move
        const int calcMod = m_using_reset_model ? u : u - 4;    // 7 added, 3 removed
        if (calcMod % 5 != 0 && calcMod % 3 == 0) {
            QVERIFY(view->isSectionHidden(u));
        }
        if (calcMod % 3 != 0) {
            QVERIFY(!view->isSectionHidden(u));
        }
    }
    if (m_using_reset_model) {
        const int precalced_results[] = { 898296472, 337096378, -543340640, -1964432121, -1251526424, -568618976, 9250 };
        calculateAndCheck(__LINE__, precalced_results);
    } else {
        const int precalced_results[] = { 1911338224, 1693514365, -613398968, -1912534953, 1582159424, -1851079000, 9300 };
        calculateAndCheck(__LINE__, precalced_results);
    }
}

void tst_QHeaderView::resizeToContentTest()
{
    additionalInit();

    QModelIndex idx = model->index(2, 2);
    model->setData(idx, QVariant("A normal string"));

    idx = model->index(1, 3);
    model->setData(idx, QVariant("A normal longer string to test resize"));

    QHeaderView *hh = m_tableview->horizontalHeader();
    hh->resizeSections(QHeaderView::ResizeToContents);
    QVERIFY(hh->sectionSize(3) > hh->sectionSize(2));

    for (int u = 0; u < 10; ++u)
        view->resizeSection(u, 1);

    view->resizeSections(QHeaderView::ResizeToContents);
    QVERIFY(view->sectionSize(1) > 1);
    QVERIFY(view->sectionSize(2) > 1);

    // Check minimum section size
    hh->setMinimumSectionSize(150);
    model->setData(idx, QVariant("i"));
    hh->resizeSections(QHeaderView::ResizeToContents);
    QCOMPARE(hh->sectionSize(3), 150);
    hh->setMinimumSectionSize(-1);

    // Check maximumSection size
    hh->setMaximumSectionSize(200);
    model->setData(idx, QVariant("This is a even longer string that is expected to be more than 200 pixels"));
    hh->resizeSections(QHeaderView::ResizeToContents);
    QCOMPARE(hh->sectionSize(3), 200);
    hh->setMaximumSectionSize(-1);

    view->setDefaultSectionSize(25); // To make sure our precalced data are correct. We do not know font height etc.

    const int precalced_results[] =  { -1523279360, -1523279360, -1347156568, 1, 1719705216, 1719705216, 12500 };
    calculateAndCheck(__LINE__, precalced_results);
}

void tst_QHeaderView::testStreamWithHide()
{
#ifndef QT_NO_DATASTREAM
    m_tableview->setVerticalHeader(view);
    m_tableview->setModel(model);
    view->setDefaultSectionSize(25);
    view->hideSection(2);
    view->swapSections(1, 2);

    QByteArray s = view->saveState();
    view->swapSections(1, 2);
    view->setDefaultSectionSize(30); // To make sure our precalced data are correct.
    view->restoreState(s);

    const int precalced_results[] =  { -1116614432, -1528653200, -1914165644, 244434607, -1111214068, 750357900, 75};
    calculateAndCheck(__LINE__, precalced_results);
#else
    QSKIP("Datastream required for testStreamWithHide. Skipping this test.");
#endif
}

void tst_QHeaderView::testStylePosition()
{
    topLevel->show();
    QVERIFY(QTest::qWaitForWindowExposed(topLevel));

    protected_QHeaderView *header = static_cast<protected_QHeaderView *>(view);

    TestStyle proxy;
    header->setStyle(&proxy);

    QImage image(1, 1, QImage::Format_ARGB32);
    QPainter p(&image);

    // 0, 1, 2, 3
    header->paintSection(&p, view->rect(), 0);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::Beginning);
    header->paintSection(&p, view->rect(), 1);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::Middle);
    header->paintSection(&p, view->rect(), 2);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::Middle);
    header->paintSection(&p, view->rect(), 3);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::End);

    // (0),2,1,3
    view->setSectionHidden(0, true);
    view->swapSections(1, 2);
    header->paintSection(&p, view->rect(), 1);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::Middle);
    header->paintSection(&p, view->rect(), 2);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::Beginning);
    header->paintSection(&p, view->rect(), 3);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::End);

    // (1),2,0,(3)
    view->setSectionHidden(3, true);
    view->setSectionHidden(0, false);
    view->setSectionHidden(1, true);
    view->swapSections(0, 1);
    header->paintSection(&p, view->rect(), 0);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::End);
    header->paintSection(&p, view->rect(), 2);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::Beginning);

    // (1),2,(0),(3)
    view->setSectionHidden(0, true);
    header->paintSection(&p, view->rect(), 2);
    QCOMPARE(proxy.lastPosition, QStyleOptionHeader::OnlyOneSection);
}

void tst_QHeaderView::sizeHintCrash()
{
    QTreeView treeView;
    QStandardItemModel *model = new QStandardItemModel(&treeView);
    model->appendRow(new QStandardItem("QTBUG-48543"));
    treeView.setModel(model);
    treeView.header()->sizeHintForColumn(0);
    treeView.header()->sizeHintForRow(0);
}

void tst_QHeaderView::stretchAndRestoreLastSection()
{
    QStandardItemModel m(10, 10);
    QTableView tv;
    tv.setModel(&m);
    tv.showMaximized();

    const int minimumSectionSize = 20;
    const int defaultSectionSize = 30;
    const int someOtherSectionSize = 40;
    const int biggerSizeThanAnySection = 50;

    QVERIFY(QTest::qWaitForWindowExposed(&tv));

    QHeaderView &header = *tv.horizontalHeader();
    // set minimum size before resizeSections() is called
    // which is done inside setStretchLastSection
    header.setMinimumSectionSize(minimumSectionSize);
    header.setDefaultSectionSize(defaultSectionSize);
    header.resizeSection(9, someOtherSectionSize);
    header.setStretchLastSection(true);

    // Default last section is larger
    QCOMPARE(header.sectionSize(8), defaultSectionSize);
    QVERIFY(header.sectionSize(9) >= biggerSizeThanAnySection);

    // Moving last section away (restore old last section 9 - and make 8 larger)
    header.swapSections(9, 8);
    QCOMPARE(header.sectionSize(9), someOtherSectionSize);
    QVERIFY(header.sectionSize(8) >= biggerSizeThanAnySection);

    // Make section 9 the large one again
    header.hideSection(8);
    QVERIFY(header.sectionSize(9) >= biggerSizeThanAnySection);

    // Show section 8 again - and make that one the last one.
    header.showSection(8);
    QVERIFY(header.sectionSize(8) > biggerSizeThanAnySection);
    QCOMPARE(header.sectionSize(9), someOtherSectionSize);

    // Swap the sections so the logical indexes are equal to visible indexes again.
    header.moveSection(9, 8);
    QCOMPARE(header.sectionSize(8), defaultSectionSize);
    QVERIFY(header.sectionSize(9) >= biggerSizeThanAnySection);

    // Append sections
    m.setColumnCount(15);
    QCOMPARE(header.sectionSize(9), someOtherSectionSize);
    QVERIFY(header.sectionSize(14) >= biggerSizeThanAnySection);

    // Truncate sections (remove sections with the last section)
    m.setColumnCount(10);
    QVERIFY(header.sectionSize(9) >= biggerSizeThanAnySection);
    for (int u = 0; u < 9; ++u)
        QCOMPARE(header.sectionSize(u), defaultSectionSize);

    // Insert sections
    m.insertColumns(2, 2);
    QCOMPARE(header.sectionSize(9), defaultSectionSize);
    QCOMPARE(header.sectionSize(10), defaultSectionSize);
    QVERIFY(header.sectionSize(11) >= biggerSizeThanAnySection);

    // Append an extra section and check restore
    m.setColumnCount(m.columnCount() + 1);
    QCOMPARE(header.sectionSize(11), someOtherSectionSize);
    QVERIFY(header.sectionSize(12) >= biggerSizeThanAnySection);

    // Remove some sections but not the last one.
    m.removeColumns(2, 2);
    QCOMPARE(header.sectionSize(9), someOtherSectionSize);
    QVERIFY(header.sectionSize(10) >= biggerSizeThanAnySection);
    for (int u = 0; u < 9; ++u)
        QCOMPARE(header.sectionSize(u), defaultSectionSize);

    // Empty the header and start over with some more tests
    m.setColumnCount(0);
    m.setColumnCount(10);
    QVERIFY(header.sectionSize(9) >= biggerSizeThanAnySection);

    // Check resize of the last section
    header.resizeSection(9, someOtherSectionSize);
    QVERIFY(header.sectionSize(9) >= biggerSizeThanAnySection); // It should still be stretched
    header.swapSections(9, 8);
    QCOMPARE(header.sectionSize(9), someOtherSectionSize);

    // Restore the order
    header.swapSections(9, 8);
    QVERIFY(header.sectionSize(9) >= biggerSizeThanAnySection);

    // Hide the last 3 sections and test stretch last section on swap/move
    // when hidden sections with a larger visual index exists.
    header.hideSection(7);
    header.hideSection(8);
    header.hideSection(9);
    QVERIFY(header.sectionSize(6) >= biggerSizeThanAnySection);
    header.moveSection(2, 7);
    QVERIFY(header.sectionSize(2) >= biggerSizeThanAnySection);
    header.swapSections(1, 8);
    QCOMPARE(header.sectionSize(2), defaultSectionSize);
    QVERIFY(header.sectionSize(1) >= biggerSizeThanAnySection);

    // Inserting sections 2
    m.setColumnCount(0);
    m.setColumnCount(10);
    header.resizeSection(1, someOtherSectionSize);
    header.swapSections(1, 9);
    m.insertColumns(9, 2);
    header.swapSections(1, 11);
    QCOMPARE(header.sectionSize(1), someOtherSectionSize);

    // Clear and re-add. This triggers a different code path than seColumnCount(0)
    m.clear();
    m.setColumnCount(3);
    QVERIFY(header.sectionSize(2) >= biggerSizeThanAnySection);

    // Test import/export of the original (not stretched) sectionSize.
    m.setColumnCount(0);
    m.setColumnCount(10);
    header.resizeSection(9, someOtherSectionSize);
    QVERIFY(header.sectionSize(9) >= biggerSizeThanAnySection);
    QByteArray b = header.saveState();
    m.setColumnCount(0);
    m.setColumnCount(10);
    header.restoreState(b);
    header.setStretchLastSection(false);
    QCOMPARE(header.sectionSize(9), someOtherSectionSize);
}

void tst_QHeaderView::testMinMaxSectionSize_data()
{
    QTest::addColumn<bool>("stretchLastSection");
    QTest::addRow("stretched") << true;
    QTest::addRow("not stretched") << false;
}

void tst_QHeaderView::testMinMaxSectionSize()
{
    QFETCH(bool, stretchLastSection);

    QStandardItemModel m(5, 5);
    QTableView tv;
    tv.setModel(&m);
    tv.show();

    const int sectionSizeMin = 20;
    const int sectionSizeMax = 40;
    const int defaultSectionSize = 30;

    QVERIFY(QTest::qWaitForWindowExposed(&tv));

    QHeaderView &header = *tv.horizontalHeader();
    header.setMinimumSectionSize(sectionSizeMin);
    header.setMaximumSectionSize(sectionSizeMax);
    // check bounds for default section size
    header.setDefaultSectionSize(sectionSizeMin - 1);
    QCOMPARE(header.defaultSectionSize(), sectionSizeMin);
    header.setDefaultSectionSize(sectionSizeMax + 1);
    QCOMPARE(header.defaultSectionSize(), sectionSizeMax);

    header.setDefaultSectionSize(defaultSectionSize);
    QCOMPARE(header.defaultSectionSize(), defaultSectionSize);
    header.setStretchLastSection(stretchLastSection);
    QCOMPARE(header.stretchLastSection(), stretchLastSection);

    // check defaults
    QCOMPARE(header.sectionSize(0), defaultSectionSize);
    QCOMPARE(header.sectionSize(3), defaultSectionSize);

    // do not go above maxSectionSize
    header.resizeSection(0, sectionSizeMax + 1);
    QCOMPARE(header.sectionSize(0), sectionSizeMax);

    // do not go below minSectionSize
    header.resizeSection(0, sectionSizeMin - 1);
    QCOMPARE(header.sectionSize(0), sectionSizeMin);

    // change section size on max change
    header.setMinimumSectionSize(sectionSizeMin);
    header.setMaximumSectionSize(sectionSizeMax);
    header.resizeSection(0, sectionSizeMax);
    QCOMPARE(header.sectionSize(0), sectionSizeMax);
    header.setMaximumSectionSize(defaultSectionSize);
    QTRY_COMPARE(header.sectionSize(0), defaultSectionSize);

    // change section size on min change
    header.setMinimumSectionSize(sectionSizeMin);
    header.setMaximumSectionSize(sectionSizeMax);
    header.resizeSection(0, sectionSizeMin);
    QCOMPARE(header.sectionSize(0), sectionSizeMin);
    header.setMinimumSectionSize(defaultSectionSize);
    QTRY_COMPARE(header.sectionSize(0), defaultSectionSize);
}

void tst_QHeaderView::testResetCachedSizeHint()
{
    QtTestModel model(10, 10);
    QTableView tv;
    tv.setModel(&model);
    tv.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tv));

    QSize s1 = tv.horizontalHeader()->sizeHint();
    model.setMultiLineHeader(true);
    QSize s2 = tv.horizontalHeader()->sizeHint();
    model.setMultiLineHeader(false);
    QSize s3 = tv.horizontalHeader()->sizeHint();
    QCOMPARE(s1, s3);
    QVERIFY(s1 != s2);
}


class StatusTipHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    using QHeaderView::QHeaderView;
    QString statusTipText;
    bool gotStatusTipEvent = false;
protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::StatusTip) {
            gotStatusTipEvent = true;
            statusTipText = static_cast<QStatusTipEvent *>(e)->tip();
        }
        return QHeaderView::event(e);
    }
};

void tst_QHeaderView::statusTips()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    StatusTipHeaderView headerView(Qt::Horizontal);
    QtTestModel model(5, 5);
    headerView.setModel(&model);
    headerView.viewport()->setMouseTracking(true);
    headerView.setGeometry(QRect(QPoint(QApplication::desktop()->geometry().center() - QPoint(250, 250)),
                           QSize(500, 500)));
    headerView.show();
    QApplication::setActiveWindow(&headerView);
    QVERIFY(QTest::qWaitForWindowActive(&headerView));

    // Ensure it is moved away first and then moved to the relevant section
    QTest::mouseMove(QApplication::desktop(),
                     headerView.rect().bottomLeft() + QPoint(20, 20));
    QPoint centerPoint = QRect(headerView.sectionPosition(0), 0,
                               headerView.sectionSize(0), headerView.height()).center();
    QTest::mouseMove(headerView.windowHandle(), centerPoint);
    QTRY_VERIFY(headerView.gotStatusTipEvent);
    QCOMPARE(headerView.statusTipText, QLatin1String("[0,0,0] -- Header"));

    headerView.gotStatusTipEvent = false;
    headerView.statusTipText.clear();
    centerPoint = QRect(headerView.sectionPosition(1), 0,
                        headerView.sectionSize(1), headerView.height()).center();
    QTest::mouseMove(headerView.windowHandle(), centerPoint);
    QTRY_VERIFY(headerView.gotStatusTipEvent);
    QCOMPARE(headerView.statusTipText, QLatin1String("[0,1,0] -- Header"));
}

void tst_QHeaderView::testRemovingColumnsViaLayoutChanged()
{
    const int persistentSectionSize = 101;

    QtTestModel model(5, 5);
    view->setModel(&model);
    for (int i = 0; i < model.cols; ++i)
        view->resizeSection(i, persistentSectionSize + i);
    model.cleanup(); // down to 3 via layoutChanged (not columnsRemoved)
    for (int j = 0; j < model.cols; ++j)
        QCOMPARE(view->sectionSize(j), persistentSectionSize + j);
    // The main point of this test is that the section-size restoring code didn't go out of bounds.
}

QTEST_MAIN(tst_QHeaderView)
#include "tst_qheaderview.moc"
