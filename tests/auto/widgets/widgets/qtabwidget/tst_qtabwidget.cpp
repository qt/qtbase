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


#include <QtTest/QtTest>
#include <qtabwidget.h>
#include <qtabbar.h>
#include <qdebug.h>
#include <qapplication.h>
#include <qlabel.h>
#include <QtWidgets/qboxlayout.h>

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
#  include <qt_windows.h>
#define Q_CHECK_PAINTEVENTS \
    if (::SwitchDesktop(::GetThreadDesktop(::GetCurrentThreadId())) == 0) \
        QSKIP("desktop is not visible, this test would fail");
#else
#define Q_CHECK_PAINTEVENTS
#endif

class QTabWidgetChild:public QTabWidget {
  public:
    QTabWidgetChild():tabCount(0) {
        QVERIFY(tabBar() != NULL);
        QWidget *w = new QWidget;
        int index = addTab(w, "test");
          QCOMPARE(tabCount, 1);
          removeTab(index);
          QCOMPARE(tabCount, 0);

          // Test bad arguments
          // This will assert, so don't do it :)
          //setTabBar(NULL);
    };

  protected:
    virtual void tabInserted(int /*index */ ) {
        tabCount++;
    };
    virtual void tabRemoved(int /*index */ ) {
        tabCount--;
    };
    int tabCount;
};

class tst_QTabWidget:public QObject {
  Q_OBJECT

private slots:
    void init();
    void cleanup();

    void getSetCheck();
    void testChild();
    void addRemoveTab();
    void tabPosition();
    void tabEnabled();
    void tabText();
    void tabShape();
    void tabTooltip();
    void tabIcon();
    void indexOf();
    void currentWidget();
    void currentIndex();
    void cornerWidget();
    void removeTab();
    void clear();
    void keyboardNavigation();
    void paintEventCount();
    void minimumSizeHint();
    void heightForWidth_data();
    void heightForWidth();
    void tabBarClicked();
    void moveCurrentTab();

  private:
    int addPage();
    void removePage(int index);
    QTabWidget *tw;
};

// Testing get/set functions
void tst_QTabWidget::getSetCheck()
{
    QTabWidget obj1;
    QWidget *w1 = new QWidget;
    QWidget *w2 = new QWidget;
    QWidget *w3 = new QWidget;
    QWidget *w4 = new QWidget;
    QWidget *w5 = new QWidget;

    obj1.addTab(w1, "Page 1");
    obj1.addTab(w2, "Page 2");
    obj1.addTab(w3, "Page 3");
    obj1.addTab(w4, "Page 4");
    obj1.addTab(w5, "Page 5");

    // TabShape QTabWidget::tabShape()
    // void QTabWidget::setTabShape(TabShape)
    obj1.setTabShape(QTabWidget::TabShape(QTabWidget::Rounded));
    QCOMPARE(QTabWidget::TabShape(QTabWidget::Rounded), obj1.tabShape());
    obj1.setTabShape(QTabWidget::TabShape(QTabWidget::Triangular));
    QCOMPARE(QTabWidget::TabShape(QTabWidget::Triangular), obj1.tabShape());

    // int QTabWidget::currentIndex()
    // void QTabWidget::setCurrentIndex(int)
    obj1.setCurrentIndex(0);
    QCOMPARE(0, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MIN);
    QCOMPARE(0, obj1.currentIndex());
    obj1.setCurrentIndex(INT_MAX);
    QCOMPARE(0, obj1.currentIndex());
    obj1.setCurrentIndex(4);
    QCOMPARE(4, obj1.currentIndex());

    // QWidget * QTabWidget::currentWidget()
    // void QTabWidget::setCurrentWidget(QWidget *)
    obj1.setCurrentWidget(w1);
    QCOMPARE(w1, obj1.currentWidget());
    obj1.setCurrentWidget(w5);
    QCOMPARE(w5, obj1.currentWidget());
    obj1.setCurrentWidget((QWidget *)0);
    QCOMPARE(w5, obj1.currentWidget()); // current not changed
}

void tst_QTabWidget::init()
{
    tw = new QTabWidget(0);
    QCOMPARE(tw->count(), 0);
    QCOMPARE(tw->currentIndex(), -1);
    QVERIFY(!tw->currentWidget());
}

void tst_QTabWidget::cleanup()
{
    delete tw;
    tw = 0;
}

void tst_QTabWidget::testChild()
{
    QTabWidgetChild t;
}

#define LABEL "TEST"
#define TIP "TIP"
int tst_QTabWidget::addPage()
{
    QWidget *w = new QWidget();
    return tw->addTab(w, LABEL);
}

void tst_QTabWidget::removePage(int index)
{
    QWidget *w = tw->widget(index);
    tw->removeTab(index);
    delete w;
}

/**
 * Tests:
 * addTab(...) which really calls -> insertTab(...)
 * widget(...)
 * removeTab(...);
 * If this fails then many others probably will too.
 */
void tst_QTabWidget::addRemoveTab()
{
    // Test bad arguments
    tw->addTab(NULL, LABEL);
    QCOMPARE(tw->count(), 0);
    tw->removeTab(-1);
    QCOMPARE(tw->count(), 0);
    QVERIFY(!tw->widget(-1));

    QWidget *w = new QWidget();
    int index = tw->addTab(w, LABEL);
    // return value
    QCOMPARE(tw->indexOf(w), index);

    QCOMPARE(tw->count(), 1);
    QCOMPARE(tw->widget(index), w);
    QCOMPARE(tw->tabText(index), QString(LABEL));

    removePage(index);
    QCOMPARE(tw->count(), 0);
}

void tst_QTabWidget::tabPosition()
{
    tw->setTabPosition(QTabWidget::North);
    QCOMPARE(tw->tabPosition(), QTabWidget::North);
    tw->setTabPosition(QTabWidget::South);
    QCOMPARE(tw->tabPosition(), QTabWidget::South);
    tw->setTabPosition(QTabWidget::East);
    QCOMPARE(tw->tabPosition(), QTabWidget::East);
    tw->setTabPosition(QTabWidget::West);
    QCOMPARE(tw->tabPosition(), QTabWidget::West);
}

void tst_QTabWidget::tabEnabled()
{
    // Test bad arguments
    QVERIFY(!tw->isTabEnabled(-1));
    tw->setTabEnabled(-1, false);

    int index = addPage();

    tw->setTabEnabled(index, true);
    QVERIFY(tw->isTabEnabled(index));
    QVERIFY(tw->widget(index)->isEnabled());
    tw->setTabEnabled(index, false);
    QVERIFY(!tw->isTabEnabled(index));
    QVERIFY(!tw->widget(index)->isEnabled());
    tw->setTabEnabled(index, true);
    QVERIFY(tw->isTabEnabled(index));
    QVERIFY(tw->widget(index)->isEnabled());

    removePage(index);
}

void tst_QTabWidget::tabText()
{
    // Test bad arguments
    QCOMPARE(tw->tabText(-1), QString(""));
    tw->setTabText(-1, LABEL);

    int index = addPage();

    tw->setTabText(index, "new");
    QCOMPARE(tw->tabText(index), QString("new"));
    tw->setTabText(index, LABEL);
    QCOMPARE(tw->tabText(index), QString(LABEL));

    removePage(index);
}

void tst_QTabWidget::tabShape()
{
    int index = addPage();

    tw->setTabShape(QTabWidget::Rounded);
    QCOMPARE(tw->tabShape(), QTabWidget::Rounded);
    tw->setTabShape(QTabWidget::Triangular);
    QCOMPARE(tw->tabShape(), QTabWidget::Triangular);
    tw->setTabShape(QTabWidget::Rounded);
    QCOMPARE(tw->tabShape(), QTabWidget::Rounded);

    removePage(index);
}

void tst_QTabWidget::tabTooltip()
{
    // Test bad arguments
    QCOMPARE(tw->tabToolTip(-1), QString(""));
    tw->setTabText(-1, TIP);

    int index = addPage();

    tw->setTabToolTip(index, "tip");
    QCOMPARE(tw->tabToolTip(index), QString("tip"));
    tw->setTabToolTip(index, TIP);
    QCOMPARE(tw->tabToolTip(index), QString(TIP));

    removePage(index);
}

void tst_QTabWidget::tabIcon()
{
    // Test bad arguments
    QVERIFY(tw->tabToolTip(-1).isNull());
    tw->setTabIcon(-1, QIcon());

    int index = addPage();

    QIcon icon;
    tw->setTabIcon(index, icon);
    QVERIFY(tw->tabIcon(index).isNull());

    removePage(index);
}

void tst_QTabWidget::indexOf()
{
    // Test bad arguments
    QCOMPARE(tw->indexOf(NULL), -1);

    int index = addPage();
    QWidget *w = tw->widget(index);
    QCOMPARE(tw->indexOf(w), index);

    removePage(index);
}

void tst_QTabWidget::currentWidget()
{
    // Test bad arguments
    tw->setCurrentWidget(NULL);
    QVERIFY(!tw->currentWidget());

    int index = addPage();
    QWidget *w = tw->widget(index);
    QCOMPARE(tw->currentWidget(), w);
    QCOMPARE(tw->currentIndex(), index);

    tw->setCurrentWidget(NULL);
    QCOMPARE(tw->currentWidget(), w);
    QCOMPARE(tw->currentIndex(), index);

    int index2 = addPage();
    QWidget *w2 = tw->widget(index2);
    Q_UNUSED(w2);
    QCOMPARE(tw->currentWidget(), w);
    QCOMPARE(tw->currentIndex(), index);

    removePage(index2);
    removePage(index);
}

/**
 * setCurrentWidget(..) calls setCurrentIndex(..)
 * currentChanged(..) SIGNAL
 */
void tst_QTabWidget::currentIndex()
{
    // Test bad arguments
    QSignalSpy spy(tw, SIGNAL(currentChanged(int)));
    QCOMPARE(tw->currentIndex(), -1);
    tw->setCurrentIndex(-1);
    QCOMPARE(tw->currentIndex(), -1);
    QCOMPARE(spy.count(), 0);

    int firstIndex = addPage();
    tw->setCurrentIndex(firstIndex);
    QCOMPARE(tw->currentIndex(), firstIndex);
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), firstIndex);

    int index = addPage();
    QCOMPARE(tw->currentIndex(), firstIndex);
    tw->setCurrentIndex(index);
    QCOMPARE(tw->currentIndex(), index);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), index);

    removePage(index);
    QCOMPARE(tw->currentIndex(), firstIndex);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), firstIndex);

    removePage(firstIndex);
    QCOMPARE(tw->currentIndex(), -1);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), -1);
}

void tst_QTabWidget::cornerWidget()
{
    // Test bad arguments
    tw->setCornerWidget(NULL, Qt::TopRightCorner);

    QVERIFY(!tw->cornerWidget(Qt::TopLeftCorner));
    QVERIFY(!tw->cornerWidget(Qt::TopRightCorner));
    QVERIFY(!tw->cornerWidget(Qt::BottomLeftCorner));
    QVERIFY(!tw->cornerWidget(Qt::BottomRightCorner));

    QWidget *w = new QWidget(0);
    tw->setCornerWidget(w, Qt::TopLeftCorner);
    QCOMPARE(w->parent(), (QObject *)tw);
    QCOMPARE(tw->cornerWidget(Qt::TopLeftCorner), w);
    tw->setCornerWidget(w, Qt::TopRightCorner);
    QCOMPARE(tw->cornerWidget(Qt::TopRightCorner), w);
    tw->setCornerWidget(w, Qt::BottomLeftCorner);
    QCOMPARE(tw->cornerWidget(Qt::BottomLeftCorner), w);
    tw->setCornerWidget(w, Qt::BottomRightCorner);
    QCOMPARE(tw->cornerWidget(Qt::BottomRightCorner), w);

    tw->setCornerWidget(0, Qt::TopRightCorner);
    QVERIFY(!tw->cornerWidget(Qt::TopRightCorner));
    QCOMPARE(w->isHidden(), true);
}

//test that the QTabWidget::count() is correct at the moment the currentChanged signal is emit
class RemoveTabObject : public QObject
{
    Q_OBJECT
    public:
        RemoveTabObject(QTabWidget *_tw) : tw(_tw), count(-1) {
            connect(tw, SIGNAL(currentChanged(int)), this, SLOT(currentChanged()));
        }

        QTabWidget *tw;
        int count;
    public slots:
        void currentChanged() { count = tw->count(); }
};

void tst_QTabWidget::removeTab()
{
    tw->show();
    QCOMPARE(tw->count(), 0);
    RemoveTabObject ob(tw);
    tw->addTab(new QLabel("1"), "1");
    QCOMPARE(ob.count, 1);
    tw->addTab(new QLabel("2"), "2");
    tw->addTab(new QLabel("3"), "3");
    tw->addTab(new QLabel("4"), "4");
    tw->addTab(new QLabel("5"), "5");
    QCOMPARE(ob.count, 1);
    QCOMPARE(tw->count(), 5);

    tw->setCurrentIndex(4);
    QCOMPARE(ob.count,5);
    tw->removeTab(4);
    QCOMPARE(ob.count, 4);
    QCOMPARE(tw->count(), 4);
    QCOMPARE(tw->currentIndex(), 3);

    tw->setCurrentIndex(1);
    tw->removeTab(1);
    QCOMPARE(ob.count, 3);
    QCOMPARE(tw->count(), 3);
    QCOMPARE(tw->currentIndex(), 1);

    delete tw->widget(1);
    QCOMPARE(tw->count(), 2);
    QCOMPARE(ob.count, 2);
    QCOMPARE(tw->currentIndex(), 1);
    delete tw->widget(1);
    QCOMPARE(tw->count(), 1);
    QCOMPARE(ob.count, 1);
    tw->removeTab(0);
    QCOMPARE(tw->count(), 0);
    QCOMPARE(ob.count, 0);
}

void tst_QTabWidget::clear()
{
    tw->addTab(new QWidget, "1");
    tw->addTab(new QWidget, "2");
    tw->addTab(new QWidget, "3");
    tw->addTab(new QWidget, "4");
    tw->addTab(new QWidget, "5");
    tw->setCurrentIndex(4);
    tw->clear();
    QCOMPARE(tw->count(), 0);
    QCOMPARE(tw->currentIndex(), -1);
}

void tst_QTabWidget::keyboardNavigation()
{
    int firstIndex = addPage();
    addPage();
    addPage();
    tw->setCurrentIndex(firstIndex);
    QCOMPARE(tw->currentIndex(), firstIndex);

    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(tw->currentIndex(), 1);
    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(tw->currentIndex(), 2);
    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(tw->currentIndex(), 0);
    tw->setTabEnabled(1, false);
    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(tw->currentIndex(), 2);

    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier | Qt::ShiftModifier);
    QCOMPARE(tw->currentIndex(), 0);
    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier | Qt::ShiftModifier);
    QCOMPARE(tw->currentIndex(), 2);
    tw->setTabEnabled(1, true);
    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier | Qt::ShiftModifier);
    QCOMPARE(tw->currentIndex(), 1);
    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier | Qt::ShiftModifier);
    QCOMPARE(tw->currentIndex(), 0);
    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier | Qt::ShiftModifier);
    QCOMPARE(tw->currentIndex(), 2);
    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier);
    QCOMPARE(tw->currentIndex(), 0);

    // Disable all and try to go to the next. It should not move anywhere, and more importantly
    // it should not loop forever. (a naive "search for the first enabled tabbar") implementation
    // might do that)
    tw->setTabEnabled(0, false);
    tw->setTabEnabled(1, false);
    tw->setTabEnabled(2, false);
    QTest::keyClick(tw, Qt::Key_Tab, Qt::ControlModifier);
    // TODO: Disabling the current tab will move current tab to the next,
    // but what if next tab is also disabled. We should look into this.
    QVERIFY(tw->currentIndex() < 3 && tw->currentIndex() >= 0);
}

class PaintCounter : public QWidget
{
public:
    PaintCounter() :count(0) { setAttribute(Qt::WA_OpaquePaintEvent); }
    int count;
protected:
    void paintEvent(QPaintEvent*) {
        ++count;
    }
};


void tst_QTabWidget::paintEventCount()
{
    Q_CHECK_PAINTEVENTS

    PaintCounter *tab1 = new PaintCounter;
    PaintCounter *tab2 = new PaintCounter;

    tw->addTab(tab1, "one");
    tw->addTab(tab2, "two");

    QCOMPARE(tab1->count, 0);
    QCOMPARE(tab2->count, 0);
    QCOMPARE(tw->currentIndex(), 0);

    tw->show();

    QTest::qWait(1000);

    // Mac, Windows and Windows CE get multiple repaints on the first show, so use those as a starting point.
    static const int MaxInitialPaintCount =
#if defined(Q_OS_WINCE)
        4;
#elif defined(Q_OS_WIN)
        2;
#elif defined(Q_OS_MAC)
        5;
#else
        2;
#endif
    QVERIFY(tab1->count <= MaxInitialPaintCount);
    QCOMPARE(tab2->count, 0);

    const int initalPaintCount = tab1->count;

    tw->setCurrentIndex(1);

    QTest::qWait(100);

    QCOMPARE(tab1->count, initalPaintCount);
    QCOMPARE(tab2->count, 1);

    tw->setCurrentIndex(0);

    QTest::qWait(100);

    QCOMPARE(tab1->count, initalPaintCount + 1);
    QCOMPARE(tab2->count, 1);
}

void tst_QTabWidget::minimumSizeHint()
{
    QTabWidget tw;
    QWidget *page = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout;

    QLabel *label = new QLabel(QLatin1String("XXgypq lorem ipsum must be long, must be long. lorem ipsumMMMW"));
    lay->addWidget(label);

    page->setLayout(lay);

    tw.addTab(page, QLatin1String("page1"));

    tw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tw));
    tw.resize(tw.minimumSizeHint());

    QSize minSize = label->minimumSizeHint();
    QSize actSize = label->geometry().size();
    QVERIFY(minSize.width() <= actSize.width());
    QVERIFY(minSize.height() <= actSize.height());
}

void tst_QTabWidget::heightForWidth_data()
{
    QTest::addColumn<int>("tabPosition");
    QTest::newRow("West") << int(QTabWidget::West);
    QTest::newRow("North") << int(QTabWidget::North);
    QTest::newRow("East") << int(QTabWidget::East);
    QTest::newRow("South") << int(QTabWidget::South);
}

void tst_QTabWidget::heightForWidth()
{
    QFETCH(int, tabPosition);

    QWidget *window = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout(window);
    lay->setMargin(0);
    lay->setSpacing(0);
    QTabWidget *tabWid = new QTabWidget(window);
    QWidget *w = new QWidget;
    tabWid->addTab(w, QLatin1String("HFW page"));
    tabWid->setTabPosition(QTabWidget::TabPosition(tabPosition));
    QVBoxLayout *lay2 = new QVBoxLayout(w);
    QLabel *label = new QLabel("Label with wordwrap turned on makes it trade height for width."
                               " Make it a really long text so that it spans on several lines"
                               " when the label is on its narrowest."
                               " I don't like to repeat myself."
                               " I don't like to repeat myself."
                               " I don't like to repeat myself."
                               " I don't like to repeat myself."
                               );
    label->setWordWrap(true);
    lay2->addWidget(label);
    lay2->setMargin(0);

    lay->addWidget(tabWid);
    int h = window->heightForWidth(160);
    window->resize(160, h);
    window->show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(label->height() >= label->heightForWidth(label->width()));

    delete window;
}

void tst_QTabWidget::tabBarClicked()
{
    QTabWidget tabWidget;
    tabWidget.addTab(new QWidget(&tabWidget), "0");
    QSignalSpy clickSpy(&tabWidget, SIGNAL(tabBarClicked(int)));
    QSignalSpy doubleClickSpy(&tabWidget, SIGNAL(tabBarDoubleClicked(int)));

    QCOMPARE(clickSpy.count(), 0);
    QCOMPARE(doubleClickSpy.count(), 0);

    QTabBar &tabBar = *tabWidget.tabBar();
    Qt::MouseButton button = Qt::LeftButton;
    while (button <= Qt::MaxMouseButton) {
        const QPoint tabPos = tabBar.tabRect(0).center();

        QTest::mouseClick(&tabBar, button, 0, tabPos);
        QCOMPARE(clickSpy.count(), 1);
        QCOMPARE(clickSpy.takeFirst().takeFirst().toInt(), 0);
        QCOMPARE(doubleClickSpy.count(), 0);

        QTest::mouseDClick(&tabBar, button, 0, tabPos);
        QCOMPARE(clickSpy.count(), 1);
        QCOMPARE(clickSpy.takeFirst().takeFirst().toInt(), 0);
        QCOMPARE(doubleClickSpy.count(), 1);
        QCOMPARE(doubleClickSpy.takeFirst().takeFirst().toInt(), 0);

        const QPoint barPos(tabBar.tabRect(0).right() + 5, tabBar.tabRect(0).center().y());

        QTest::mouseClick(&tabBar, button, 0, barPos);
        QCOMPARE(clickSpy.count(), 1);
        QCOMPARE(clickSpy.takeFirst().takeFirst().toInt(), -1);
        QCOMPARE(doubleClickSpy.count(), 0);

        QTest::mouseDClick(&tabBar, button, 0, barPos);
        QCOMPARE(clickSpy.count(), 1);
        QCOMPARE(clickSpy.takeFirst().takeFirst().toInt(), -1);
        QCOMPARE(doubleClickSpy.count(), 1);
        QCOMPARE(doubleClickSpy.takeFirst().takeFirst().toInt(), -1);

        button = Qt::MouseButton(button << 1);
    }
}

void tst_QTabWidget::moveCurrentTab()
{
    QTabWidget tabWidget;
    QWidget* firstTab = new QWidget(&tabWidget);
    QWidget* secondTab = new QWidget(&tabWidget);
    tabWidget.addTab(firstTab, "0");
    tabWidget.addTab(secondTab, "1");

    QCOMPARE(tabWidget.currentIndex(), 0);
    QCOMPARE(tabWidget.currentWidget(), firstTab);

    tabWidget.setCurrentIndex(1);

    QCOMPARE(tabWidget.currentIndex(), 1);
    QCOMPARE(tabWidget.currentWidget(), secondTab);

    tabWidget.tabBar()->moveTab(1, 0);

    QCOMPARE(tabWidget.currentIndex(), 0);
    QCOMPARE(tabWidget.currentWidget(), secondTab);
}

QTEST_MAIN(tst_QTabWidget)
#include "tst_qtabwidget.moc"
