// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QSignalSpy>
#include <QApplication>
#include <QTabBar>
#include <QPushButton>
#include <QLabel>
#include <QStyle>
#include <QStyleOptionTab>
#include <QProxyStyle>
#include <QTimer>
#include <QScreen>
#include <QWindow>

#include <QtWidgets/private/qtabbar_p.h>

using namespace Qt::StringLiterals;

class TabBar;

class tst_QTabBar : public QObject
{
    Q_OBJECT

public:
    tst_QTabBar();
    virtual ~tst_QTabBar();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

private slots:
    void getSetCheck();
    void setIconSize();
    void setIconSize_data();

    void testCurrentChanged_data();
    void testCurrentChanged();

    void insertAtCurrentIndex();
    void insertAfterCurrentIndex();

    void removeTab_data();
    void removeTab();

    void hideTab_data();
    void hideTab();
    void hideAllTabs();
    void checkHiddenTab();

    void setElideMode_data();
    void setElideMode();
    void sizeHints();

    void setUsesScrollButtons_data();
    void setUsesScrollButtons();

    void removeLastTab();
    void removeLastVisibleTab();

    void closeButton();

    void tabButton_data();
    void tabButton();

    void selectionBehaviorOnRemove_data();
    void selectionBehaviorOnRemove();

    void moveTab_data();
    void moveTab();

    void task251184_removeTab();
    void changeTitleWhileDoubleClickingTab();

    void taskQTBUG_10052_widgetLayoutWhenMoving();

    void tabBarClicked();
    void autoHide();

    void mouseReleaseOutsideTabBar();

    void mouseWheel();
    void kineticWheel_data();
    void kineticWheel();
    void highResolutionWheel_data();
    void highResolutionWheel();

    void scrollButtons_data();
    void scrollButtons();

    void currentTabLargeFont();

    void hoverTab_data();
    void hoverTab();

    void resizeKeepsScroll_data();
    void resizeKeepsScroll();
    void changeTabTextKeepsScroll();
    void settingCurrentTabBeforeShowDoesntScroll();

private:
    void checkPositions(const TabBar &tabbar, const QList<int> &positions);
};

// Testing get/set functions
void tst_QTabBar::getSetCheck()
{
    QTabBar obj1;
    obj1.addTab("Tab1");
    obj1.addTab("Tab2");
    obj1.addTab("Tab3");
    obj1.addTab("Tab4");
    obj1.addTab("Tab5");
    // Shape QTabBar::shape()
    // void QTabBar::setShape(Shape)
    obj1.setShape(QTabBar::Shape(QTabBar::RoundedNorth));
    QCOMPARE(QTabBar::Shape(QTabBar::RoundedNorth), obj1.shape());
    obj1.setShape(QTabBar::Shape(QTabBar::RoundedSouth));
    QCOMPARE(QTabBar::Shape(QTabBar::RoundedSouth), obj1.shape());
    obj1.setShape(QTabBar::Shape(QTabBar::RoundedWest));
    QCOMPARE(QTabBar::Shape(QTabBar::RoundedWest), obj1.shape());
    obj1.setShape(QTabBar::Shape(QTabBar::RoundedEast));
    QCOMPARE(QTabBar::Shape(QTabBar::RoundedEast), obj1.shape());
    obj1.setShape(QTabBar::Shape(QTabBar::TriangularNorth));
    QCOMPARE(QTabBar::Shape(QTabBar::TriangularNorth), obj1.shape());
    obj1.setShape(QTabBar::Shape(QTabBar::TriangularSouth));
    QCOMPARE(QTabBar::Shape(QTabBar::TriangularSouth), obj1.shape());
    obj1.setShape(QTabBar::Shape(QTabBar::TriangularWest));
    QCOMPARE(QTabBar::Shape(QTabBar::TriangularWest), obj1.shape());
    obj1.setShape(QTabBar::Shape(QTabBar::TriangularEast));
    QCOMPARE(QTabBar::Shape(QTabBar::TriangularEast), obj1.shape());

    // bool QTabBar::drawBase()
    // void QTabBar::setDrawBase(bool)
    obj1.setDrawBase(false);
    QCOMPARE(false, obj1.drawBase());
    obj1.setDrawBase(true);
    QCOMPARE(true, obj1.drawBase());

    // int QTabBar::currentIndex()
    // void QTabBar::setCurrentIndex(int)
    obj1.setCurrentIndex(0);
    QCOMPARE(0, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MIN);
    QCOMPARE(0, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MAX);
    QCOMPARE(0, obj1.currentIndex());
    obj1.setCurrentIndex(4);
    QCOMPARE(4, obj1.currentIndex());
}

tst_QTabBar::tst_QTabBar()
{
}

tst_QTabBar::~tst_QTabBar()
{
}

void tst_QTabBar::initTestCase()
{
}

void tst_QTabBar::cleanupTestCase()
{
}

void tst_QTabBar::init()
{
}

void tst_QTabBar::setIconSize_data()
{
    QTest::addColumn<int>("sizeToSet");
    QTest::addColumn<int>("expectedWidth");

    const int iconDefault = qApp->style()->pixelMetric(QStyle::PM_TabBarIconSize);
    const int smallIconSize = qApp->style()->pixelMetric(QStyle::PM_SmallIconSize);
    const int largeIconSize = qApp->style()->pixelMetric(QStyle::PM_LargeIconSize);
    QTest::newRow("default") << -1 << iconDefault;
    QTest::newRow("zero") << 0 << 0;
    QTest::newRow("same as default") << iconDefault << iconDefault;
    QTest::newRow("large") << largeIconSize << largeIconSize;
    QTest::newRow("small") << smallIconSize << smallIconSize;
}

void tst_QTabBar::setIconSize()
{
    QFETCH(int, sizeToSet);
    QFETCH(int, expectedWidth);
    QTabBar tabBar;
    tabBar.setIconSize(QSize(sizeToSet, sizeToSet));
    QCOMPARE(tabBar.iconSize().width(), expectedWidth);
}

void tst_QTabBar::testCurrentChanged_data()
{
    QTest::addColumn<int>("tabToSet");
    QTest::addColumn<int>("expectedCount");

    QTest::newRow("pressAntotherTab") << 1 << 2;
    QTest::newRow("pressTheSameTab") << 0 << 1;
}

void tst_QTabBar::testCurrentChanged()
{
    QFETCH(int, tabToSet);
    QFETCH(int, expectedCount);
    QTabBar tabBar;
    QSignalSpy spy(&tabBar, SIGNAL(currentChanged(int)));
    tabBar.addTab("Tab1");
    tabBar.addTab("Tab2");
    QCOMPARE(tabBar.currentIndex(), 0);
    tabBar.setCurrentIndex(tabToSet);
    QCOMPARE(tabBar.currentIndex(), tabToSet);
    QCOMPARE(spy.size(), expectedCount);
}

class TabBar : public QTabBar
{
public:
    using QTabBar::initStyleOption;
    using QTabBar::moveTab;
    using QTabBar::QTabBar;
    using QTabBar::tabSizeHint;
};

void tst_QTabBar::insertAtCurrentIndex()
{
    QTabBar tabBar;
    tabBar.addTab("Tab1");
    QCOMPARE(tabBar.currentIndex(), 0);
    tabBar.insertTab(0, "Tab2");
    QCOMPARE(tabBar.currentIndex(), 1);
    tabBar.insertTab(0, "Tab3");
    QCOMPARE(tabBar.currentIndex(), 2);
    tabBar.insertTab(2, "Tab4");
    QCOMPARE(tabBar.currentIndex(), 3);
}

void tst_QTabBar::insertAfterCurrentIndex()
{
    TabBar tabBar;

    tabBar.addTab("Tab10");
    checkPositions(tabBar, { QStyleOptionTab::OnlyOneTab });

    tabBar.addTab("Tab20");
    checkPositions(tabBar, { QStyleOptionTab::Beginning, QStyleOptionTab::End });

    tabBar.insertTab(1, "Tab15");
    checkPositions(tabBar,
                   { QStyleOptionTab::Beginning, QStyleOptionTab::Middle, QStyleOptionTab::End });

    tabBar.insertTab(3, "Tab30");
    checkPositions(tabBar,
                   { QStyleOptionTab::Beginning, QStyleOptionTab::Middle, QStyleOptionTab::Middle,
                     QStyleOptionTab::End });

    tabBar.insertTab(3, "Tab25");
    checkPositions(tabBar,
                   { QStyleOptionTab::Beginning, QStyleOptionTab::Middle, QStyleOptionTab::Middle,
                     QStyleOptionTab::Middle, QStyleOptionTab::End });
}

void tst_QTabBar::removeTab_data()
{
    QTest::addColumn<int>("currentIndex");
    QTest::addColumn<int>("deleteIndex");
    QTest::addColumn<int>("spyCount");
    QTest::addColumn<int>("finalIndex");

    QTest::newRow("deleteEnd") << 0 << 2 << 0 << 0;
    QTest::newRow("deleteEndWithIndexOnEnd") << 2 << 2 << 1 << 1;
    QTest::newRow("deleteMiddle") << 2 << 1 << 1 << 1;
    QTest::newRow("deleteMiddleOnMiddle") << 1 << 1 << 1 << 1;
}
void tst_QTabBar::removeTab()
{
    QTabBar tabbar;

    QFETCH(int, currentIndex);
    QFETCH(int, deleteIndex);
    tabbar.addTab("foo");
    tabbar.addTab("bar");
    tabbar.addTab("baz");
    tabbar.setCurrentIndex(currentIndex);
    QSignalSpy spy(&tabbar, SIGNAL(currentChanged(int)));
    tabbar.removeTab(deleteIndex);
    QTEST(int(spy.size()), "spyCount");
    QTEST(tabbar.currentIndex(), "finalIndex");
}

void tst_QTabBar::hideTab_data()
{
    QTest::addColumn<int>("currentIndex");
    QTest::addColumn<int>("hideIndex");
    QTest::addColumn<int>("spyCount");
    QTest::addColumn<int>("finalIndex");

    QTest::newRow("hideEnd") << 0 << 2 << 0 << 0;
    QTest::newRow("hideEndWithIndexOnEnd") << 2 << 2 << 1 << 1;
    QTest::newRow("hideMiddle") << 2 << 1 << 0 << 2;
    QTest::newRow("hideMiddleOnMiddle") << 1 << 1 << 1 << 2;
    QTest::newRow("hideFirst") << 2 << 0 << 0 << 2;
    QTest::newRow("hideFirstOnFirst") << 0 << 0 << 1 << 1;
}

void tst_QTabBar::hideTab()
{
    QTabBar tabbar;

    QFETCH(int, currentIndex);
    QFETCH(int, hideIndex);
    tabbar.addTab("foo");
    tabbar.addTab("bar");
    tabbar.addTab("baz");
    tabbar.setCurrentIndex(currentIndex);
    QSignalSpy spy(&tabbar, &QTabBar::currentChanged);
    tabbar.setTabVisible(hideIndex, false);
    QTEST(int(spy.size()), "spyCount");
    QTEST(tabbar.currentIndex(), "finalIndex");
}

void tst_QTabBar::hideAllTabs()
{
    TabBar tabbar;

    tabbar.addTab("foo");
    tabbar.addTab("bar");
    tabbar.addTab("baz");
    tabbar.setCurrentIndex(0);
    checkPositions(tabbar,
                   { QStyleOptionTab::Beginning, QStyleOptionTab::Middle, QStyleOptionTab::End });

    // Check we don't crash trying to hide an unexistant tab
    QSize prevSizeHint = tabbar.sizeHint();
    tabbar.setTabVisible(3, false);
    checkPositions(tabbar,
                   { QStyleOptionTab::Beginning, QStyleOptionTab::Middle, QStyleOptionTab::End });
    QCOMPARE(tabbar.currentIndex(), 0);
    QSize sizeHint = tabbar.sizeHint();
    QVERIFY(sizeHint.width() == prevSizeHint.width());
    prevSizeHint = sizeHint;

    tabbar.setTabVisible(1, false);
    checkPositions(tabbar, { QStyleOptionTab::Beginning, QStyleOptionTab::End });
    QCOMPARE(tabbar.currentIndex(), 0);
    sizeHint = tabbar.sizeHint();
    QVERIFY(sizeHint.width() < prevSizeHint.width());
    prevSizeHint = sizeHint;

    tabbar.setTabVisible(2, false);
    checkPositions(tabbar, { QStyleOptionTab::OnlyOneTab });
    QCOMPARE(tabbar.currentIndex(), 0);
    sizeHint = tabbar.sizeHint();
    QVERIFY(sizeHint.width() < prevSizeHint.width());
    prevSizeHint = sizeHint;

    tabbar.setTabVisible(0, false);
    // We cannot set currentIndex < 0
    QCOMPARE(tabbar.currentIndex(), 0);
    sizeHint = tabbar.sizeHint();
    QVERIFY(sizeHint.width() < prevSizeHint.width());
}

void tst_QTabBar::checkHiddenTab()
{
    QTabBar tabbar;

    tabbar.addTab("foo");
    tabbar.addTab("bar");
    tabbar.addTab("baz");
    tabbar.setCurrentIndex(0);
    tabbar.setTabVisible(1, false);

    QKeyEvent keyRight(QKeyEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    QVERIFY(QApplication::sendEvent(&tabbar, &keyRight));
    QCOMPARE(tabbar.currentIndex(), 2);

    QKeyEvent keyLeft(QKeyEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    QVERIFY(QApplication::sendEvent(&tabbar, &keyLeft));
    QCOMPARE(tabbar.currentIndex(), 0);
}

void tst_QTabBar::setElideMode_data()
{
    QTest::addColumn<int>("tabElideMode");
    QTest::addColumn<int>("expectedMode");

    QTest::newRow("default") << -128 << qApp->style()->styleHint(QStyle::SH_TabBar_ElideMode);
    QTest::newRow("explicit default") << qApp->style()->styleHint(QStyle::SH_TabBar_ElideMode)
                                      << qApp->style()->styleHint(QStyle::SH_TabBar_ElideMode);
    QTest::newRow("None") << int(Qt::ElideNone) << int(Qt::ElideNone);
    QTest::newRow("Left") << int(Qt::ElideLeft) << int(Qt::ElideLeft);
    QTest::newRow("Center") << int(Qt::ElideMiddle) << int(Qt::ElideMiddle);
    QTest::newRow("Right") << int(Qt::ElideRight) << int(Qt::ElideRight);
}

void tst_QTabBar::setElideMode()
{
    QFETCH(int, tabElideMode);
    QTabBar tabBar;
    if (tabElideMode != -128)
        tabBar.setElideMode(Qt::TextElideMode(tabElideMode));
    QTEST(int(tabBar.elideMode()), "expectedMode");
    // Make sure style sheet does not override user set mode
    tabBar.setStyleSheet("QWidget { background-color: #ABA8A6;}");
    QTEST(int(tabBar.elideMode()), "expectedMode");
}

void tst_QTabBar::sizeHints()
{
    QTabBar tabBar;
    tabBar.setFont(QFont("Arial", 10));

    // No eliding and no scrolling -> tabbar becomes very wide
    tabBar.setUsesScrollButtons(false);
    tabBar.setElideMode(Qt::ElideNone);
    tabBar.addTab("tab 01");

    // Fetch the minimum size hint width and make sure that we create enough
    // tabs.
    int minimumSizeHintWidth = tabBar.minimumSizeHint().width();
    for (int i = 0; i <= 700 / minimumSizeHintWidth; ++i)
        tabBar.addTab(QString("tab 0%1").arg(i+2));

    //qDebug() << tabBar.minimumSizeHint() << tabBar.sizeHint();
    QVERIFY(tabBar.minimumSizeHint().width() > 700);
    QVERIFY(tabBar.sizeHint().width() > 700);

    // Scrolling enabled -> no reason to become very wide
    tabBar.setUsesScrollButtons(true);
    QVERIFY(tabBar.minimumSizeHint().width() < 200);
    QVERIFY(tabBar.sizeHint().width() > 700); // unchanged

    // Eliding enabled -> no reason to become very wide
    tabBar.setUsesScrollButtons(false);
    tabBar.setElideMode(Qt::ElideRight);

    // The sizeHint is very much dependent on the screen DPI value
    // so we can not really predict it.
    int tabBarMinSizeHintWidth = tabBar.minimumSizeHint().width();
    int tabBarSizeHintWidth = tabBar.sizeHint().width();
    QVERIFY(tabBarMinSizeHintWidth < tabBarSizeHintWidth);
    QVERIFY(tabBarSizeHintWidth > 700); // unchanged

    tabBar.addTab("This is tab with a very long title");
    QVERIFY(tabBar.minimumSizeHint().width() > tabBarMinSizeHintWidth);
    QVERIFY(tabBar.sizeHint().width() > tabBarSizeHintWidth);
}

void tst_QTabBar::setUsesScrollButtons_data()
{
    QTest::addColumn<int>("usesArrows");
    QTest::addColumn<bool>("expectedArrows");

    QTest::newRow("default") << -128 << !qApp->style()->styleHint(QStyle::SH_TabBar_PreferNoArrows);
    QTest::newRow("explicit default")
                        << int(!qApp->style()->styleHint(QStyle::SH_TabBar_PreferNoArrows))
                        << !qApp->style()->styleHint(QStyle::SH_TabBar_PreferNoArrows);
    QTest::newRow("No") << int(false) << false;
    QTest::newRow("Yes") << int(true) << true;
}

void tst_QTabBar::setUsesScrollButtons()
{
    QFETCH(int, usesArrows);
    QTabBar tabBar;
    if (usesArrows != -128)
        tabBar.setUsesScrollButtons(usesArrows);
    QTEST(tabBar.usesScrollButtons(), "expectedArrows");

    // Make sure style sheet does not override user set mode
    tabBar.setStyleSheet("QWidget { background-color: #ABA8A6;}");
    QTEST(tabBar.usesScrollButtons(), "expectedArrows");
}

void tst_QTabBar::removeLastTab()
{
    QTabBar tabbar;
    QSignalSpy spy(&tabbar, SIGNAL(currentChanged(int)));
    int index = tabbar.addTab("foo");
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), index);
    spy.clear();

    tabbar.removeTab(index);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), -1);
    spy.clear();
}

void tst_QTabBar::removeLastVisibleTab()
{
    QTabBar tabbar;
    tabbar.setSelectionBehaviorOnRemove(QTabBar::SelectionBehavior::SelectRightTab);

    int invisible = tabbar.addTab("invisible");
    int visible = tabbar.addTab("visible");
    tabbar.setCurrentIndex(visible);
    tabbar.adjustSize();

    tabbar.setTabVisible(invisible, false);
    {
        QSignalSpy spy(&tabbar, SIGNAL(currentChanged(int)));
        tabbar.removeTab(visible);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), -1);
        QCOMPARE(tabbar.currentIndex(), -1);
    }

    tabbar.setSelectionBehaviorOnRemove(QTabBar::SelectionBehavior::SelectLeftTab);
    visible = tabbar.insertTab(0, "visible");
    ++invisible;
    QVERIFY(!tabbar.isTabVisible(invisible));
    tabbar.setCurrentIndex(visible);
    {
        QSignalSpy spy(&tabbar, SIGNAL(currentChanged(int)));
        tabbar.removeTab(visible);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), -1);
        QCOMPARE(tabbar.currentIndex(), -1);
    }
}

void tst_QTabBar::closeButton()
{
    QTabBar tabbar;
    QCOMPARE(tabbar.tabsClosable(), false);
    tabbar.setTabsClosable(true);
    QCOMPARE(tabbar.tabsClosable(), true);
    tabbar.addTab("foo");

    QTabBar::ButtonPosition closeSide = (QTabBar::ButtonPosition)tabbar.style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, &tabbar);
    QTabBar::ButtonPosition otherSide = (closeSide == QTabBar::LeftSide ? QTabBar::RightSide : QTabBar::LeftSide);
    QVERIFY(tabbar.tabButton(0, otherSide) == 0);
    QVERIFY(tabbar.tabButton(0, closeSide) != 0);

    QAbstractButton *button = static_cast<QAbstractButton*>(tabbar.tabButton(0, closeSide));
    QVERIFY(button);
    QSignalSpy spy(&tabbar, SIGNAL(tabCloseRequested(int)));
    button->click();
    QCOMPARE(tabbar.count(), 1);
    QCOMPARE(spy.size(), 1);
}

Q_DECLARE_METATYPE(QTabBar::ButtonPosition)
void tst_QTabBar::tabButton_data()
{
    QTest::addColumn<QTabBar::ButtonPosition>("position");

    QTest::newRow("left") << QTabBar::LeftSide;
    QTest::newRow("right") << QTabBar::RightSide;
}

// QTabBar::setTabButton(index, closeSide, closeButton);
void tst_QTabBar::tabButton()
{
    QFETCH(QTabBar::ButtonPosition, position);
    QTabBar::ButtonPosition otherSide = (position == QTabBar::LeftSide ? QTabBar::RightSide : QTabBar::LeftSide);

    QTabBar tabbar;
    tabbar.resize(500, 200);
    tabbar.show();
    QTRY_VERIFY(tabbar.isVisible());

    tabbar.setTabButton(-1, position, 0);
    QVERIFY(tabbar.tabButton(-1, position) == 0);
    QVERIFY(tabbar.tabButton(0, position) == 0);

    tabbar.addTab("foo");
    QCOMPARE(tabbar.count(), 1);
    tabbar.setTabButton(0, position, 0);
    QVERIFY(tabbar.tabButton(0, position) == 0);

    QPushButton *button = new QPushButton;
    button->show();
    button->setText("hi");
    button->resize(10, 10);
    QTRY_VERIFY(button->isVisible());
    QTRY_VERIFY(button->isVisible());

    tabbar.setTabButton(0, position, button);

    QCOMPARE(tabbar.tabButton(0, position), static_cast<QWidget *>(button));
    QTRY_VERIFY(!button->isHidden());
    QVERIFY(tabbar.tabButton(0, otherSide) == 0);
    QCOMPARE(button->parent(), static_cast<QObject *>(&tabbar));
    QVERIFY(button->pos() != QPoint(0, 0));

    QPushButton *button2 = new QPushButton;
    tabbar.setTabButton(0, position, button2);
    QVERIFY(button->isHidden());
}

typedef QList<int> IntList;
Q_DECLARE_METATYPE(QTabBar::SelectionBehavior)
#define ONE(x) (IntList() << x)
void tst_QTabBar::selectionBehaviorOnRemove_data()
{
    QTest::addColumn<QTabBar::SelectionBehavior>("selectionBehavior");
    QTest::addColumn<int>("tabs");
    QTest::addColumn<IntList>("select");
    QTest::addColumn<IntList>("remove");
    QTest::addColumn<int>("expected");

    //                                               Count            select remove current
    QTest::newRow("left-1") << QTabBar::SelectLeftTab << 3 << (IntList() << 0) << ONE(0) << 0;

    QTest::newRow("left-2") << QTabBar::SelectLeftTab << 3 << (IntList() << 0) << ONE(1) << 0; // not removing current
    QTest::newRow("left-3") << QTabBar::SelectLeftTab << 3 << (IntList() << 0) << ONE(2) << 0; // not removing current
    QTest::newRow("left-4") << QTabBar::SelectLeftTab << 3 << (IntList() << 1) << ONE(0) << 0; // not removing current
    QTest::newRow("left-5") << QTabBar::SelectLeftTab << 3 << (IntList() << 1) << ONE(1) << 0;
    QTest::newRow("left-6") << QTabBar::SelectLeftTab << 3 << (IntList() << 1) << ONE(2) << 1;
    QTest::newRow("left-7") << QTabBar::SelectLeftTab << 3 << (IntList() << 2) << ONE(0) << 1; // not removing current
    QTest::newRow("left-8") << QTabBar::SelectLeftTab << 3 << (IntList() << 2) << ONE(1) << 1; // not removing current
    QTest::newRow("left-9") << QTabBar::SelectLeftTab << 3 << (IntList() << 2) << ONE(2) << 1;

    QTest::newRow("right-1") << QTabBar::SelectRightTab << 3 << (IntList() << 0) << ONE(0) << 0;
    QTest::newRow("right-2") << QTabBar::SelectRightTab << 3 << (IntList() << 0) << ONE(1) << 0; // not removing current
    QTest::newRow("right-3") << QTabBar::SelectRightTab << 3 << (IntList() << 0) << ONE(2) << 0; // not removing current
    QTest::newRow("right-4") << QTabBar::SelectRightTab << 3 << (IntList() << 1) << ONE(0) << 0; // not removing current
    QTest::newRow("right-5") << QTabBar::SelectRightTab << 3 << (IntList() << 1) << ONE(1) << 1;
    QTest::newRow("right-6") << QTabBar::SelectRightTab << 3 << (IntList() << 1) << ONE(2) << 1; // not removing current
    QTest::newRow("right-7") << QTabBar::SelectRightTab << 3 << (IntList() << 2) << ONE(0) << 1; // not removing current
    QTest::newRow("right-8") << QTabBar::SelectRightTab << 3 << (IntList() << 2) << ONE(1) << 1; // not removing current
    QTest::newRow("right-9") << QTabBar::SelectRightTab << 3 << (IntList() << 2) << ONE(2) << 1;

    QTest::newRow("previous-0") << QTabBar::SelectPreviousTab << 3 << (IntList()) << ONE(0) << 0;
    QTest::newRow("previous-1") << QTabBar::SelectPreviousTab << 3 << (IntList()) << ONE(1) << 0; // not removing current
    QTest::newRow("previous-2") << QTabBar::SelectPreviousTab << 3 << (IntList()) << ONE(2) << 0; // not removing current

    QTest::newRow("previous-3") << QTabBar::SelectPreviousTab << 3 << (IntList() << 2) << ONE(0) << 1; // not removing current
    QTest::newRow("previous-4") << QTabBar::SelectPreviousTab << 3 << (IntList() << 2) << ONE(1) << 1; // not removing current
    QTest::newRow("previous-5") << QTabBar::SelectPreviousTab << 3 << (IntList() << 2) << ONE(2) << 0;

    // go back one
    QTest::newRow("previous-6") << QTabBar::SelectPreviousTab << 4 << (IntList() << 0 << 2 << 3 << 1) << (IntList() << 1) << 2;
    // go back two
    QTest::newRow("previous-7") << QTabBar::SelectPreviousTab << 4 << (IntList() << 0 << 2 << 3 << 1) << (IntList() << 1 << 2) << 1;
    // go back three
    QTest::newRow("previous-8") << QTabBar::SelectPreviousTab << 4 << (IntList() << 0 << 2 << 3 << 1) << (IntList() << 1 << 2 << 1) << 0;

    // pick from the middle
    QTest::newRow("previous-9") << QTabBar::SelectPreviousTab << 4 << (IntList() << 0 << 2 << 3 << 1) << (IntList() << 2 << 1) << 1;

    // every other one
    QTest::newRow("previous-10") << QTabBar::SelectPreviousTab << 7 << (IntList() << 0 << 2 << 4 << 6) << (IntList() << 6 << 4) << 2;

    // QTBUG-94352
    QTest::newRow("QTBUG-94352") << QTabBar::SelectPreviousTab << 4 << (IntList() << 3) << (IntList() << 2 << 2) << 0;

}

void tst_QTabBar::selectionBehaviorOnRemove()
{
    QFETCH(QTabBar::SelectionBehavior, selectionBehavior);
    QFETCH(int, tabs);
    QFETCH(IntList, select);
    QFETCH(IntList, remove);
    QFETCH(int, expected);

    QTabBar tabbar;
    tabbar.setSelectionBehaviorOnRemove(selectionBehavior);
    while(--tabs >= 0)
        tabbar.addTab(QString::number(tabs));
    QCOMPARE(tabbar.currentIndex(), 0);
    while(!select.isEmpty())
        tabbar.setCurrentIndex(select.takeFirst());
    while(!remove.isEmpty())
        tabbar.removeTab(remove.takeFirst());
    QVERIFY(tabbar.count() > 0);
    QCOMPARE(tabbar.currentIndex(), expected);
}

Q_DECLARE_METATYPE(QTabBar::Shape)
void tst_QTabBar::moveTab_data()
{
    QTest::addColumn<QTabBar::Shape>("shape");
    QTest::addColumn<int>("tabs");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");

    QTest::newRow("null-0") << QTabBar::RoundedNorth << 0 << -1 << -1;
    QTest::newRow("null-1") << QTabBar::RoundedEast  << 0 << -1 << -1;
    QTest::newRow("null-2") << QTabBar::RoundedEast  << 1 << 0 << 0;

    QTest::newRow("two-0") << QTabBar::RoundedNorth << 2 << 0 << 1;
    QTest::newRow("two-1") << QTabBar::RoundedNorth << 2 << 1 << 0;

    QTest::newRow("five-0") << QTabBar::RoundedNorth << 5 << 1 << 3; // forward
    QTest::newRow("five-1") << QTabBar::RoundedNorth << 5 << 3 << 1; // reverse

    QTest::newRow("five-2") << QTabBar::RoundedNorth << 5 << 0 << 4; // forward
    QTest::newRow("five-3") << QTabBar::RoundedNorth << 5 << 1 << 4; // forward
    QTest::newRow("five-4") << QTabBar::RoundedNorth << 5 << 3 << 4; // forward
}

void tst_QTabBar::moveTab()
{
    QFETCH(QTabBar::Shape, shape);
    QFETCH(int, tabs);
    QFETCH(int, from);
    QFETCH(int, to);

    TabBar bar;
    bar.setShape(shape);
    while(--tabs >= 0)
        bar.addTab(QString::number(tabs));
    bar.moveTab(from, to);
}


class MyTabBar : public QTabBar
{
    Q_OBJECT
public slots:
    void onCurrentChanged()
    {
        //we just want this to be done once
        disconnect(this, SIGNAL(currentChanged(int)), this, SLOT(onCurrentChanged()));
        removeTab(0);
    }
};

void tst_QTabBar::task251184_removeTab()
{
    MyTabBar bar;
    bar.addTab("bar1");
    bar.addTab("bar2");
    QCOMPARE(bar.count(), 2);
    QCOMPARE(bar.currentIndex(), 0);

    bar.connect(&bar, SIGNAL(currentChanged(int)), SLOT(onCurrentChanged()));
    bar.setCurrentIndex(1);

    QCOMPARE(bar.count(), 1);
    QCOMPARE(bar.currentIndex(), 0);
    QCOMPARE(bar.tabText(bar.currentIndex()), QString("bar2"));
}


class TitleChangeTabBar : public QTabBar
{
    Q_OBJECT

    QTimer timer;
    int count;

public:
    TitleChangeTabBar(QWidget * parent = nullptr) : QTabBar(parent), count(0)
    {
        setMovable(true);
        addTab("0");
        connect(&timer, SIGNAL(timeout()), this, SLOT(updateTabText()));
        timer.start(1);
    }

public slots:
    void updateTabText()
    {
        count++;
        setTabText(0, QString::number(count));
    }
};

void tst_QTabBar::changeTitleWhileDoubleClickingTab()
{
    TitleChangeTabBar bar;
    QPoint tabPos = bar.tabRect(0).center();

    for(int i=0; i < 10; i++)
        QTest::mouseDClick(&bar, Qt::LeftButton, {}, tabPos);
}

class Widget10052 : public QWidget
{
public:
    Widget10052(QWidget *parent) : QWidget(parent), moved(false)
    { }

    void moveEvent(QMoveEvent *e) override
    {
        moved = e->oldPos() != e->pos();
        QWidget::moveEvent(e);
    }

    bool moved;
};

void tst_QTabBar::taskQTBUG_10052_widgetLayoutWhenMoving()
{
    QTabBar tabBar;
    tabBar.insertTab(0, "My first tab");
    Widget10052 w1(&tabBar);
    tabBar.setTabButton(0, QTabBar::RightSide, &w1);
    tabBar.insertTab(1, "My other tab");
    Widget10052 w2(&tabBar);
    tabBar.setTabButton(1, QTabBar::RightSide, &w2);

    tabBar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tabBar));
    w1.moved = w2.moved = false;
    tabBar.moveTab(0, 1);
    QTRY_VERIFY(w1.moved);
    QVERIFY(w2.moved);
}

void tst_QTabBar::tabBarClicked()
{
    QTabBar tabBar;
    tabBar.addTab("0");
    QSignalSpy clickSpy(&tabBar, SIGNAL(tabBarClicked(int)));
    QSignalSpy doubleClickSpy(&tabBar, SIGNAL(tabBarDoubleClicked(int)));

    QCOMPARE(clickSpy.size(), 0);
    QCOMPARE(doubleClickSpy.size(), 0);

    Qt::MouseButton button = Qt::LeftButton;
    while (button <= Qt::MaxMouseButton) {
        const QPoint tabPos = tabBar.tabRect(0).center();

        QTest::mouseClick(&tabBar, button, {}, tabPos);
        QCOMPARE(clickSpy.size(), 1);
        QCOMPARE(clickSpy.takeFirst().takeFirst().toInt(), 0);
        QCOMPARE(doubleClickSpy.size(), 0);

        QTest::mouseDClick(&tabBar, button, {}, tabPos);
        QCOMPARE(clickSpy.size(), 1);
        QCOMPARE(clickSpy.takeFirst().takeFirst().toInt(), 0);
        QCOMPARE(doubleClickSpy.size(), 1);
        QCOMPARE(doubleClickSpy.takeFirst().takeFirst().toInt(), 0);
        QTest::mouseRelease(&tabBar, button, {}, tabPos);

        const QPoint barPos(tabBar.tabRect(0).right() + 5, tabBar.tabRect(0).center().y());

        QTest::mouseClick(&tabBar, button, {}, barPos);
        QCOMPARE(clickSpy.size(), 1);
        QCOMPARE(clickSpy.takeFirst().takeFirst().toInt(), -1);
        QCOMPARE(doubleClickSpy.size(), 0);

        QTest::mouseDClick(&tabBar, button, {}, barPos);
        QCOMPARE(clickSpy.size(), 1);
        QCOMPARE(clickSpy.takeFirst().takeFirst().toInt(), -1);
        QCOMPARE(doubleClickSpy.size(), 1);
        QCOMPARE(doubleClickSpy.takeFirst().takeFirst().toInt(), -1);
        QTest::mouseRelease(&tabBar, button, {}, barPos);

        button = Qt::MouseButton(button << 1);
    }
}

void tst_QTabBar::autoHide()
{
    QTabBar tabBar;
    QVERIFY(!tabBar.autoHide());
    QVERIFY(!tabBar.isVisible());
    tabBar.show();
    QVERIFY(tabBar.isVisible());
    tabBar.addTab("0");
    QVERIFY(tabBar.isVisible());
    tabBar.removeTab(0);
    QVERIFY(tabBar.isVisible());

    tabBar.setAutoHide(true);
    QVERIFY(!tabBar.isVisible());
    tabBar.addTab("0");
    QVERIFY(!tabBar.isVisible());
    tabBar.addTab("1");
    QVERIFY(tabBar.isVisible());
    tabBar.removeTab(0);
    QVERIFY(!tabBar.isVisible());
    tabBar.removeTab(0);
    QVERIFY(!tabBar.isVisible());

    tabBar.setAutoHide(false);
    QVERIFY(tabBar.isVisible());
}

void tst_QTabBar::mouseReleaseOutsideTabBar()
{
    class RepaintChecker : public QObject
    {
    public:
        bool repainted = false;
        QRect rectToBeRepainted;
        bool eventFilter(QObject *, QEvent *event) override
        {
            if (event->type() == QEvent::Paint &&
                static_cast<QPaintEvent *>(event)->rect().contains(rectToBeRepainted)) {
                repainted = true;
            }
            return false;
        }
    } repaintChecker;

    QTabBar tabBar;
    tabBar.installEventFilter(&repaintChecker);
    tabBar.addTab("    ");
    tabBar.addTab("    ");
    tabBar.show();
    if (!QTest::qWaitForWindowExposed(&tabBar))
        QSKIP("Window failed to show, skipping test");

    QRect tabRect = tabBar.tabRect(1);
    QPoint tabCenter = tabRect.center();
    repaintChecker.rectToBeRepainted = tabRect;
    // if a press repaints the tab...
    QTest::mousePress(&tabBar, Qt::LeftButton, {}, tabCenter);
    const bool pressRepainted = QTest::qWaitFor([&]{ return repaintChecker.repainted; }, 250);

    // ... then releasing the mouse outside the tabbar should repaint it as well
    repaintChecker.repainted = false;
    QTest::mouseRelease(&tabBar, Qt::LeftButton, {}, tabCenter + QPoint(tabCenter.x(), tabCenter.y() + tabRect.height()));
    QTRY_COMPARE(repaintChecker.repainted, pressRepainted);
}

void tst_QTabBar::checkPositions(const TabBar &tabbar, const QList<int> &positions)
{
    QStyleOptionTab option;
    int iPos = 0;
    for (int i = 0; i < tabbar.count(); ++i) {
        if (!tabbar.isTabVisible(i))
            continue;
        tabbar.initStyleOption(&option, i);
        QCOMPARE(option.position, positions.at(iPos++));
    }
}

#if QT_CONFIG(wheelevent)

class TabBarScrollingProxyStyle : public QProxyStyle
{
public:
    TabBarScrollingProxyStyle(const QString &defStyle = {})
        : QProxyStyle(defStyle), scrolling(true)
    { }

    int styleHint(StyleHint hint, const QStyleOption *option = 0,
                  const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const override
    {
        switch (hint) {
        case QStyle::SH_TabBar_AllowWheelScrolling:
            return scrolling;
        case SH_TabBar_ElideMode:
            return Qt::ElideNone;
        default:
            break;
        }

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

    bool scrolling;
};

void tst_QTabBar::mouseWheel()
{
    TabBar tabbar;

    // apply custom style to the tabbar, which can toggle tabbar scrolling behavior
    TabBarScrollingProxyStyle proxyStyle;
    tabbar.setStyle(&proxyStyle);

    // make tabbar with three tabs, select the middle one
    tabbar.addTab("one");
    tabbar.addTab("two");
    tabbar.addTab("three");
    int startIndex = 1;
    tabbar.setCurrentIndex(startIndex);

    const auto systemId = QPointingDevice::primaryPointingDevice()->systemId() + 1;
    QPointingDevice clickyWheel("test clicky wheel", systemId, QInputDevice::DeviceType::Mouse,
                                QPointingDevice::PointerType::Generic,
                                QInputDevice::Capability::Position | QInputDevice::Capability::Scroll,
                                1, 3);

    // define scroll event
    const QPoint wheelPoint = tabbar.rect().bottomRight();
    QWheelEvent event(wheelPoint, tabbar.mapToGlobal(wheelPoint),
                      QPoint(), QPoint(0, QWheelEvent::DefaultDeltasPerStep),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false,
                      Qt::MouseEventSynthesizedByApplication, &clickyWheel);

    // disable scrolling, send scroll event, confirm that tab did not change
    proxyStyle.scrolling = false;
    QVERIFY(QApplication::sendEvent(&tabbar, &event));
    QVERIFY(tabbar.currentIndex() == startIndex);

    // enable scrolling, send scroll event, confirm that tab changed
    proxyStyle.scrolling = true;
    QVERIFY(QApplication::sendEvent(&tabbar, &event));
    QVERIFY(tabbar.currentIndex() != startIndex);
}

void tst_QTabBar::kineticWheel_data()
{
    QTest::addColumn<QTabBar::Shape>("tabShape");

    QTest::addRow("North") << QTabBar::RoundedNorth;
    QTest::addRow("East") << QTabBar::RoundedEast;
    QTest::addRow("South") << QTabBar::RoundedSouth;
    QTest::addRow("West") << QTabBar::RoundedWest;
}

void tst_QTabBar::kineticWheel()
{
    const auto systemId = QPointingDevice::primaryPointingDevice()->systemId() + 1;
    QPointingDevice pixelPad("test pixel pad", systemId, QInputDevice::DeviceType::TouchPad,
                             QPointingDevice::PointerType::Generic,
                             QInputDevice::Capability::Position | QInputDevice::Capability::PixelScroll,
                             1, 3);

    QFETCH(QTabBar::Shape, tabShape);
    QWidget window;
    TabBar tabbar(&window);
    // Since the macOS style makes sure that all tabs are always visible, we
    // replace it with the windows style for this test, and use the proxy that
    // makes sure that scrolling is enabled and that tab texts are not elided.
    QString defaultStyle;
    if (QApplication::style()->name() == QStringLiteral("macos"))
        defaultStyle = "windows";
    TabBarScrollingProxyStyle proxyStyle(defaultStyle);
    tabbar.setStyle(&proxyStyle);

    tabbar.addTab("long tab text 1");
    tabbar.addTab("long tab text 2");
    tabbar.addTab("long tab text 3");

    // Make sure we don't have enough space for the tabs and need to scroll
    const int tabbarLength = tabbar.tabRect(0).width() * 2;

    tabbar.setShape(tabShape);
    const bool horizontal = tabShape == QTabBar::RoundedNorth
                         || tabShape == QTabBar::RoundedSouth;
    if (horizontal)
        tabbar.setFixedWidth(tabbarLength);
    else
        tabbar.setFixedHeight(tabbarLength);

    // start with the middle tab, QTabBar will scroll to make it visible
    const int startIndex = 1;
    tabbar.setCurrentIndex(startIndex);

    window.setMinimumSize(tabbarLength, tabbarLength);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const auto *leftButton = tabbar.findChild<QAbstractButton*>(u"ScrollLeftButton"_s);
    const auto *rightButton = tabbar.findChild<QAbstractButton*>(u"ScrollRightButton"_s);
    QVERIFY(leftButton && rightButton);
    QVERIFY(leftButton->isEnabled() && rightButton->isEnabled());

    // Figure out if any of the buttons is laid out to be in front of the tabs.
    // We can't use setUsesScrollButtons(false), as then several styles will enforce
    // a minimum size for the tab bar.
    const bool leftInFront = ((horizontal && leftButton->pos().x() < tabbar.rect().center().x())
                           || (!horizontal && leftButton->pos().y() < tabbar.rect().center().y()));
    const bool rightInFront = ((horizontal && rightButton->pos().x() < tabbar.rect().center().x())
                            || (!horizontal && rightButton->pos().y() < tabbar.rect().center().y()));
    QPoint leftEdge;
    QPoint rightEdge;
    if (leftInFront && rightInFront) { // both on the left
        leftEdge = rightButton->geometry().bottomRight();
        rightEdge = tabbar.rect().bottomRight();
    } else if (leftInFront && !rightInFront) {
        leftEdge = leftButton->geometry().bottomRight();
        rightEdge = rightButton->geometry().topLeft();
    } else { // both on the right
        leftEdge = QPoint(0, 0);
        rightEdge = leftButton->geometry().topLeft();
    }
    // avoid border lines
    leftEdge += QPoint(2, 2);
    if (horizontal) {
        rightEdge += QPoint(-2, 2);
    } else {
        rightEdge += QPoint(2, -2);
    }

    QCOMPARE(tabbar.tabAt(leftEdge), 0);
    QCOMPARE(tabbar.tabAt(rightEdge), 1);

    const QPoint delta = horizontal ? QPoint(10, 0) : QPoint(0, 10);
    const QPoint wheelPoint = tabbar.rect().center();

    bool accepted = true;
    Qt::ScrollPhase phase = Qt::ScrollBegin;
    // scroll all the way to the end
    while (accepted) {
        QWheelEvent event(wheelPoint, tabbar.mapToGlobal(wheelPoint), -delta, -delta,
                          Qt::NoButton, Qt::NoModifier, phase, false,
                          Qt::MouseEventSynthesizedByApplication, &pixelPad);
        if (phase == Qt::ScrollBegin)
            phase = Qt::ScrollUpdate;
        QApplication::sendEvent(&tabbar, &event);
        accepted = event.isAccepted();
    }
    QCOMPARE(tabbar.tabAt(leftEdge), 1);
    QCOMPARE(tabbar.tabAt(rightEdge), 2);
    QVERIFY(leftButton->isEnabled());
    QVERIFY(!rightButton->isEnabled());
    // kinetic wheel events don't change the current index
    QVERIFY(tabbar.currentIndex() == startIndex);

    // scroll all the way to the beginning
    accepted = true;
    while (accepted) {
        QWheelEvent event(wheelPoint, tabbar.mapToGlobal(wheelPoint), delta, delta,
                          Qt::NoButton, Qt::NoModifier, phase, false,
                          Qt::MouseEventSynthesizedByApplication, &pixelPad);
        QApplication::sendEvent(&tabbar, &event);
        accepted = event.isAccepted();
    }

    QCOMPARE(tabbar.tabAt(leftEdge), 0);
    QCOMPARE(tabbar.tabAt(rightEdge), 1);
    QVERIFY(!leftButton->isEnabled());
    QVERIFY(rightButton->isEnabled());
    // kinetic wheel events don't change the current index
    QVERIFY(tabbar.currentIndex() == startIndex);

    // make tabs small so that we have enough space, and verify sure we can't scroll
    tabbar.setTabText(0, "A");
    tabbar.setTabText(1, "B");
    tabbar.setTabText(2, "C");
    QVERIFY(tabbar.sizeHint().width() <= tabbar.width() && tabbar.sizeHint().height() <= tabbar.height());

    {
        QWheelEvent event(wheelPoint, tabbar.mapToGlobal(wheelPoint), -delta, -delta,
                          Qt::NoButton, Qt::NoModifier, phase, false,
                          Qt::MouseEventSynthesizedByApplication, &pixelPad);
        QApplication::sendEvent(&tabbar, &event);
        QVERIFY(!event.isAccepted());
    }

    {
        QWheelEvent event(wheelPoint, tabbar.mapToGlobal(wheelPoint), delta, delta,
                          Qt::NoButton, Qt::NoModifier, phase, false,
                          Qt::MouseEventSynthesizedByApplication, &pixelPad);
        QApplication::sendEvent(&tabbar, &event);
        QVERIFY(!event.isAccepted());
    }
}

void tst_QTabBar::highResolutionWheel_data()
{
    QTest::addColumn<int>("angleDelta");
    // Smallest angleDelta for a Logitech MX Master 3 with Linux/X11/Libinput
    QTest::addRow("increment index") << -16;
    QTest::addRow("decrement index") << 16;
}

void tst_QTabBar::highResolutionWheel()
{
    TabBar tabbar;
    TabBarScrollingProxyStyle proxyStyle;
    tabbar.setStyle(&proxyStyle);

    tabbar.addTab("tab1");
    tabbar.addTab("tab2");
    QFETCH(int, angleDelta);
    // Negative values increment, positive values decrement
    int startIndex = angleDelta < 0 ? 0 : 1;
    tabbar.setCurrentIndex(startIndex);

    const auto systemId = QPointingDevice::primaryPointingDevice()->systemId() + 1;
    QPointingDevice hiResWheel(
            "test high resolution wheel", systemId, QInputDevice::DeviceType::Mouse,
            QPointingDevice::PointerType::Generic,
            QInputDevice::Capability::Position | QInputDevice::Capability::Scroll, 1, 3);

    const QPoint wheelPoint = tabbar.rect().bottomRight();
    QWheelEvent event(wheelPoint, tabbar.mapToGlobal(wheelPoint), QPoint(),
                      QPoint(angleDelta, angleDelta), Qt::NoButton, Qt::NoModifier,
                      Qt::NoScrollPhase, false, Qt::MouseEventSynthesizedByApplication,
                      &hiResWheel);

    proxyStyle.scrolling = true;
    for (int accumulated = 0; accumulated < QWheelEvent::DefaultDeltasPerStep;
         accumulated += qAbs(angleDelta)) {
        // verify that nothing has changed until the threshold has been reached
        QVERIFY(tabbar.currentIndex() == startIndex);
        QVERIFY(QApplication::sendEvent(&tabbar, &event));
    }
    QVERIFY(tabbar.currentIndex() != startIndex);
}

#endif // QT_CONFIG(wheelevent)

void tst_QTabBar::scrollButtons_data()
{
    QTest::addColumn<QTabWidget::TabPosition>("tabPosition");
    QTest::addColumn<Qt::LayoutDirection>("layoutDirection");

    for (auto ld : {Qt::LeftToRight, Qt::RightToLeft}) {
        const char *ldStr = ld == Qt::LeftToRight ? "LTR" : "RTL";
        QTest::addRow("North, %s", ldStr) << QTabWidget::North << ld;
        QTest::addRow("South, %s", ldStr) << QTabWidget::South << ld;
        QTest::addRow("West, %s", ldStr) << QTabWidget::West << ld;
        QTest::addRow("East, %s", ldStr) << QTabWidget::East << ld;
    }
}

void tst_QTabBar::scrollButtons()
{
    QFETCH(QTabWidget::TabPosition, tabPosition);
    QFETCH(Qt::LayoutDirection, layoutDirection);

    QWidget window;
    QTabWidget tabWidget(&window);
    tabWidget.setLayoutDirection(layoutDirection);
    tabWidget.setTabPosition(tabPosition);
    tabWidget.setElideMode(Qt::ElideNone);
    tabWidget.setUsesScrollButtons(true);

    const int tabCount = 5;
    for (int i = 0; i < tabCount; ++i)
    {
        const QString num = QString::number(i);
        tabWidget.addTab(new QLabel(num), num + " - Really long tab name to force arrows");
    }
    tabWidget.move(0, 0);
    tabWidget.resize(tabWidget.minimumSizeHint());
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    auto *leftB = tabWidget.tabBar()->findChild<QAbstractButton*>(u"ScrollLeftButton"_s);
    auto *rightB = tabWidget.tabBar()->findChild<QAbstractButton*>(u"ScrollRightButton"_s);

    QVERIFY(leftB->isVisible());
    QVERIFY(!leftB->isEnabled());
    QVERIFY(rightB->isVisible());
    QVERIFY(rightB->isEnabled());
    QVERIFY(!tabWidget.tabBar()->tabRect(1).intersects(tabWidget.tabBar()->rect()));

    int index = 0;
    for (; index < tabWidget.count(); ++index) {
        QCOMPARE(leftB->isEnabled(), index > 0);
        QCOMPARE(rightB->isEnabled(), index < tabWidget.count() - 1);
        QVERIFY(tabWidget.tabBar()->tabRect(index).intersects(tabWidget.tabBar()->rect()));
        QCOMPARE(tabWidget.tabBar()->tabAt(tabWidget.tabBar()->rect().center()), index);
        if (rightB->isEnabled())
            rightB->click();
    }
    for (--index; index >= 0; --index) {
        QCOMPARE(leftB->isEnabled(), index >= 0);
        QCOMPARE(rightB->isEnabled(), index < tabWidget.count() - 1);

        QVERIFY(tabWidget.tabBar()->tabRect(index).intersects(tabWidget.tabBar()->rect()));
        if (leftB->isEnabled())
            leftB->click();
    }
    QVERIFY(!leftB->isEnabled());
}

void tst_QTabBar::currentTabLargeFont()
{
    TabBar tabBar;
    tabBar.setStyleSheet(R"(
        QTabBar::tab::selected {
            font-size: 24pt;
        }
    )");

    tabBar.addTab("Tab Item 1");
    tabBar.addTab("Tab Item 2");
    tabBar.addTab("Tab Item 3");

    tabBar.setCurrentIndex(0);
    tabBar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tabBar));

    QList<QRect> oldTabRects;
    oldTabRects << tabBar.tabRect(0) << tabBar.tabRect(1) << tabBar.tabRect(2);
    tabBar.setCurrentIndex(1);
    QList<QRect> newTabRects;
    newTabRects << tabBar.tabRect(0) << tabBar.tabRect(1) << tabBar.tabRect(2);
    QVERIFY(oldTabRects != newTabRects);
}

void tst_QTabBar::hoverTab_data()
{
    // Move the cursor away from the widget as not to interfere.
    // skip this test if we can't
    const QPoint topLeft = QGuiApplication::primaryScreen()->availableGeometry().topLeft();
    const QPoint cursorPos = topLeft + QPoint(10, 10);
    QCursor::setPos(cursorPos);
    if (!QTest::qWaitFor([cursorPos]{ return QCursor::pos() == cursorPos; }, 500))
        QSKIP("Can't move mouse");

    QTest::addColumn<bool>("documentMode");
    QTest::addRow("normal mode") << true;
    QTest::addRow("document mode") << true;
}

void tst_QTabBar::hoverTab()
{
    QFETCH(bool, documentMode);
    QWidget topLevel;
    class TabBar : public QTabBar
    {
    public:
        using QTabBar::QTabBar;
        void initStyleOption(QStyleOptionTab *option, int tabIndex) const override
        {
            QTabBar::initStyleOption(option, tabIndex);
            styleOptions[tabIndex] = *option;
        }
        mutable QHash<int, QStyleOptionTab> styleOptions;
    } tabbar(&topLevel);

    tabbar.setDocumentMode(documentMode);
    tabbar.addTab("A");
    tabbar.addTab("B");
    tabbar.addTab("C");
    tabbar.addTab("D");

    tabbar.move(0,0);
    const QSize size = tabbar.sizeHint();
    const auto center = topLevel.screen()->availableGeometry().center();
    topLevel.move(center - QPoint{size.width(), size.height()} / 2);
    topLevel.setMinimumSize(size);
    tabbar.ensurePolished();

    // some styles set those flags, some don't. If not, use a style sheet
    if (!(tabbar.testAttribute(Qt::WA_Hover) || tabbar.hasMouseTracking())) {
        tabbar.setStyleSheet(R"(
            QTabBar::tab { background: blue; }
            QTabBar::tab::hover { background: yellow; }
            QTabBar::tab::selected { background: red; }
        )");
    }

    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    auto *window = topLevel.windowHandle();

    auto pos = tabbar.mapToParent(tabbar.tabRect(0).center());
    QTest::mouseMove(window, pos);
    QTRY_VERIFY(tabbar.styleOptions[0].state & QStyle::State_Selected);
    QTRY_COMPARE(tabbar.styleOptions[1].state & QStyle::State_MouseOver, QStyle::State_None);
    QTRY_COMPARE(tabbar.styleOptions[2].state & QStyle::State_MouseOver, QStyle::State_None);
    QTRY_COMPARE(tabbar.styleOptions[3].state & QStyle::State_MouseOver, QStyle::State_None);

    pos = tabbar.mapToParent(tabbar.tabRect(1).center());
    QTest::mouseMove(window, pos);
    QTRY_COMPARE(tabbar.styleOptions[1].state & QStyle::State_MouseOver, QStyle::State_MouseOver);
    QCOMPARE(tabbar.styleOptions[2].state & QStyle::State_MouseOver, QStyle::State_None);
    QCOMPARE(tabbar.styleOptions[3].state & QStyle::State_MouseOver, QStyle::State_None);

    pos = tabbar.mapToParent(tabbar.tabRect(2).center());
    QTest::mouseMove(window, pos);
    QTRY_COMPARE(tabbar.styleOptions[2].state & QStyle::State_MouseOver, QStyle::State_MouseOver);
    QCOMPARE(tabbar.styleOptions[1].state & QStyle::State_MouseOver, QStyle::State_None);
    QCOMPARE(tabbar.styleOptions[3].state & QStyle::State_MouseOver, QStyle::State_None);

    // removing tab 2 lays the tabs out so that they stretch across the
    // tab bar; tab 1 is now where the cursor was. What matters is that a
    // different tab is now hovered (rather than none).
    tabbar.removeTab(2);
    QTRY_COMPARE(tabbar.styleOptions[1].state & QStyle::State_MouseOver, QStyle::State_MouseOver);
    QCOMPARE(tabbar.styleOptions[2].state & QStyle::State_MouseOver, QStyle::State_None);

    // inserting a tab at index 2 again should paint the new tab hovered
    tabbar.insertTab(2, "C2");
    QTRY_COMPARE(tabbar.styleOptions[2].state & QStyle::State_MouseOver, QStyle::State_MouseOver);
    QCOMPARE(tabbar.styleOptions[1].state & QStyle::State_MouseOver, QStyle::State_None);
}


void tst_QTabBar::resizeKeepsScroll_data()
{
    QTest::addColumn<QTabBar::Shape>("tabShape");
    QTest::addColumn<bool>("expanding");

    QTest::addRow("North, expanding") << QTabBar::RoundedNorth << true;
    QTest::addRow("East, expanding") << QTabBar::RoundedEast << true;
    QTest::addRow("South, expanding") << QTabBar::RoundedSouth << true;
    QTest::addRow("West, expanding") << QTabBar::RoundedWest << true;

    QTest::addRow("North, not expanding") << QTabBar::RoundedNorth << false;
    QTest::addRow("South, not expanding") << QTabBar::RoundedSouth << false;
}

void tst_QTabBar::resizeKeepsScroll()
{
    QFETCH(QTabBar::Shape, tabShape);
    QFETCH(const bool, expanding);

    QTabBar tabBar;
    TabBarScrollingProxyStyle proxyStyle;
    tabBar.setStyle(&proxyStyle);

    for (int i = 0; i < 10; ++i)
        tabBar.addTab(u"Tab Number %1"_s.arg(i));

    tabBar.setShape(tabShape);
    tabBar.setUsesScrollButtons(true);
    tabBar.setExpanding(expanding);

    // resize to half
    const QSize fullSize = tabBar.sizeHint();
    const bool horizontal = fullSize.width() > fullSize.height();
    if (horizontal)
        tabBar.resize(fullSize.width() / 2, fullSize.height());
    else
        tabBar.resize(fullSize.width(), fullSize.height() / 2);

    tabBar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tabBar));

    const auto getScrollOffset = [&]() -> int {
        return static_cast<QTabBarPrivate *>(QObjectPrivate::get(&tabBar))->scrollOffset;
    };

    // select a tab outside, this will scroll
    tabBar.setCurrentIndex(6);
    // the first tab is now scrolled out
    const int scrollOffset = getScrollOffset();
    QCOMPARE_GT(scrollOffset, 0);
    // the current index is now fully visible, with margin on both sides
    tabBar.setCurrentIndex(5);

    // make the tab bar a bit larger, by the width of a tab
    if (horizontal)
        tabBar.resize(tabBar.width() + tabBar.tabRect(5).width(), tabBar.height());
    else
        tabBar.resize(tabBar.width(), tabBar.height() + tabBar.tabRect(5).height());

    // this should not change the scroll
    QCOMPARE(getScrollOffset(), scrollOffset);

    // make the tab bar large enough to fit everything with extra space
    tabBar.resize(fullSize + QSize(50, 50));

    // there should be no scroll
    QCOMPARE(getScrollOffset(), 0);

    for (int i = 0; i < tabBar.count(); ++i) {
        tabBar.setCurrentIndex(i);
        QCOMPARE(getScrollOffset(), 0);
    }
}

void tst_QTabBar::changeTabTextKeepsScroll()
{
    QTabBar tabBar;
    TabBarScrollingProxyStyle proxyStyle;
    tabBar.setStyle(&proxyStyle);

    for (int i = 0; i < 6; ++i)
        tabBar.addTab(u"Tab Number %1"_s.arg(i));

    const QSize fullSize = tabBar.sizeHint();
    tabBar.resize(fullSize.width() / 2, fullSize.height());

    tabBar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tabBar));

    const auto getScrollOffset = [&]() -> int {
        return static_cast<QTabBarPrivate *>(QObjectPrivate::get(&tabBar))->scrollOffset;
    };

    tabBar.setCurrentIndex(3);
    const int scrollOffset = getScrollOffset();
    tabBar.setTabText(3, "New title");
    QCOMPARE(getScrollOffset(), scrollOffset);
}

void tst_QTabBar::settingCurrentTabBeforeShowDoesntScroll()
{
    QTabBar tabBar;
    TabBarScrollingProxyStyle proxyStyle;
    tabBar.setStyle(&proxyStyle);

    for (int i = 0; i < 6; ++i)
        tabBar.addTab(u"Tab Number %1"_s.arg(i));

    const auto getScrollOffset = [&]() -> int {
        return static_cast<QTabBarPrivate *>(QObjectPrivate::get(&tabBar))->scrollOffset;
    };

    tabBar.setCurrentIndex(5);

    // changing the current index while the tab bar isn't visible shouldn't scroll yet
    QCOMPARE(getScrollOffset(), 0);

    // now show the tab bar with a size that's too small to fit the current index
    const QSize fullSize = tabBar.sizeHint();
    tabBar.resize(fullSize.width() / 2, fullSize.height());

    tabBar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tabBar));

    // this should scroll
    QCOMPARE_GT(getScrollOffset(), 0);
}

QTEST_MAIN(tst_QTabBar)
#include "tst_qtabbar.moc"
