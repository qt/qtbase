// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qabstractscrollarea.h>
#include <qscrollarea.h>
#include <qscrollbar.h>
#include <qlabel.h>
#include <qwidget.h>
#include <qdialog.h>
#include <qscroller.h>
#include <qstyle.h>

class tst_QAbstractScrollArea : public QObject
{
Q_OBJECT

public:
    tst_QAbstractScrollArea();
    virtual ~tst_QAbstractScrollArea();
private slots:
    void scrollBarWidgets();
    void setScrollBars();
    void setScrollBars2();
    void objectNaming();
    void patternBackground();

    void viewportCrash();
    void task214488_layoutDirection_data();
    void task214488_layoutDirection();

    void margins();
    void resizeWithOvershoot();
    void sizeHint();
};

tst_QAbstractScrollArea::tst_QAbstractScrollArea()
{
}

tst_QAbstractScrollArea::~tst_QAbstractScrollArea()
{
}

void tst_QAbstractScrollArea::scrollBarWidgets()
{
    QWidget *w1 = new QWidget(0);
    QWidget *w2 = new QWidget(0);
    QWidget *w3 = new QWidget(0);

    Qt::Alignment all = Qt::AlignLeft | Qt::AlignRight | Qt::AlignTop | Qt::AlignBottom;

    QWidgetList w1List = QWidgetList() << w1;
    QWidgetList w2List = QWidgetList() << w2;
    QWidgetList w3List = QWidgetList() << w3;

    QWidgetList w1w2List = w1List + w2List;
    QWidgetList allList = w1List + w2List + w3List;

    QAbstractScrollArea area;
    area.show();
    QCOMPARE(area.scrollBarWidgets(all), QWidgetList());

    area.addScrollBarWidget(w1, Qt::AlignLeft);
    QCOMPARE(area.scrollBarWidgets(all), w1List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignLeft), w1List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignRight), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignTop), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignBottom), QWidgetList());

    area.addScrollBarWidget(w2, Qt::AlignBottom);
    QCOMPARE(area.scrollBarWidgets(all), w1w2List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignLeft), w1List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignRight), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignTop), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignBottom), w2List);

    // duplicate add
    area.addScrollBarWidget(w2, Qt::AlignBottom);
    QCOMPARE(area.scrollBarWidgets(all), w1w2List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignLeft), w1List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignRight), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignTop), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignBottom), w2List);

    //reparent
    w2->setParent(w1);
    QCOMPARE(area.scrollBarWidgets(all), w1List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignLeft), w1List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignRight), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignTop), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignBottom), QWidgetList());

    // add after reparent
    area.addScrollBarWidget(w2, Qt::AlignBottom);
    QCOMPARE(area.scrollBarWidgets(all), w1w2List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignLeft), w1List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignRight), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignTop), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignBottom), w2List);

    auto sort = [](const QWidgetList l) {
        QWidgetList list = l;
        std::sort(list.begin(), list.end());
        return list;
    };

    // two widgets at Bottom.
    area.addScrollBarWidget(w3, Qt::AlignBottom);
    QCOMPARE(sort(area.scrollBarWidgets(all)), sort(allList));
    QCOMPARE(area.scrollBarWidgets(Qt::AlignLeft), w1List);
    QCOMPARE(area.scrollBarWidgets(Qt::AlignRight), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignTop), QWidgetList());
    QCOMPARE(sort(area.scrollBarWidgets(Qt::AlignBottom)), sort(w2List + w3List));

    //delete
    delete w1;
    delete w2;
    delete w3;

    QCOMPARE(area.scrollBarWidgets(all), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignLeft), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignRight), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignTop), QWidgetList());
    QCOMPARE(area.scrollBarWidgets(Qt::AlignBottom), QWidgetList());
}

void tst_QAbstractScrollArea::setScrollBars()
{
    QScrollArea scrollArea;
    scrollArea.resize(300, 300);
    scrollArea.show();

    QPointer<QScrollBar> vbar = scrollArea.verticalScrollBar();
    QPointer<QScrollBar> hbar = scrollArea.horizontalScrollBar();

    // Now set properties on the scroll bars
    scrollArea.verticalScrollBar()->setInvertedAppearance(true);
    scrollArea.verticalScrollBar()->setInvertedControls(true);
    scrollArea.verticalScrollBar()->setTracking(true);
    scrollArea.verticalScrollBar()->setRange(-100, 100);
    scrollArea.verticalScrollBar()->setPageStep(42);
    scrollArea.verticalScrollBar()->setSingleStep(3);
    scrollArea.verticalScrollBar()->setValue(43);
    scrollArea.horizontalScrollBar()->setInvertedAppearance(true);
    scrollArea.horizontalScrollBar()->setInvertedControls(true);
    scrollArea.horizontalScrollBar()->setTracking(true);
    scrollArea.horizontalScrollBar()->setRange(-100, 100);
    scrollArea.horizontalScrollBar()->setPageStep(42);
    scrollArea.horizontalScrollBar()->setSingleStep(3);
    scrollArea.horizontalScrollBar()->setValue(43);

    qApp->processEvents();

    // Then replace the scroll bars
    scrollArea.setVerticalScrollBar(new QScrollBar);
    scrollArea.setHorizontalScrollBar(new QScrollBar);

    // Check that the old ones were deleted
    QVERIFY(!vbar);
    QVERIFY(!hbar);

    qApp->processEvents();

    // Check that all properties have been populated
    QVERIFY(scrollArea.verticalScrollBar()->invertedAppearance());
    QVERIFY(scrollArea.verticalScrollBar()->invertedControls());
    QVERIFY(scrollArea.verticalScrollBar()->hasTracking());
    QVERIFY(scrollArea.verticalScrollBar()->isVisible());
    QCOMPARE(scrollArea.verticalScrollBar()->minimum(), -100);
    QCOMPARE(scrollArea.verticalScrollBar()->maximum(), 100);
    QCOMPARE(scrollArea.verticalScrollBar()->pageStep(), 42);
    QCOMPARE(scrollArea.verticalScrollBar()->singleStep(), 3);
    QCOMPARE(scrollArea.verticalScrollBar()->value(), 43);
    QVERIFY(scrollArea.horizontalScrollBar()->invertedAppearance());
    QVERIFY(scrollArea.horizontalScrollBar()->invertedControls());
    QVERIFY(scrollArea.horizontalScrollBar()->hasTracking());
    QVERIFY(scrollArea.horizontalScrollBar()->isVisible());
    QCOMPARE(scrollArea.horizontalScrollBar()->minimum(), -100);
    QCOMPARE(scrollArea.horizontalScrollBar()->maximum(), 100);
    QCOMPARE(scrollArea.horizontalScrollBar()->pageStep(), 42);
    QCOMPARE(scrollArea.horizontalScrollBar()->singleStep(), 3);
    QCOMPARE(scrollArea.horizontalScrollBar()->value(), 43);
}

void tst_QAbstractScrollArea::setScrollBars2()
{
    QAbstractScrollArea scrollArea;
    scrollArea.resize(300, 300);

    QScrollBar *hbar = new QScrollBar;
    scrollArea.setHorizontalScrollBar(hbar);
    qApp->processEvents();
    QCOMPARE(scrollArea.horizontalScrollBar(), hbar);

    QScrollBar *vbar = new QScrollBar;
    scrollArea.setVerticalScrollBar(vbar);
    qApp->processEvents();
    QCOMPARE(scrollArea.verticalScrollBar(), vbar);

    scrollArea.horizontalScrollBar()->setRange(0, 100);
    scrollArea.verticalScrollBar()->setRange(0, 100);
    scrollArea.show();

    // Make sure scroll bars are not explicitly hidden by QAbstractScrollArea itself.
    QVERIFY(hbar->isVisible());
    QVERIFY(vbar->isVisible());

    // Hide the OLD scroll bar and ensure that the NEW one is hidden.
    hbar->hide();
    hbar = new QScrollBar(&scrollArea);
    scrollArea.setHorizontalScrollBar(hbar);
    QVERIFY(!hbar->isVisibleTo(hbar->parentWidget()));

    vbar->hide();
    vbar = new QScrollBar(&scrollArea);
    scrollArea.setVerticalScrollBar(vbar);
    QVERIFY(!vbar->isVisibleTo(vbar->parentWidget()));

    vbar->show();
    hbar->show();

    // Hide the NEW scroll bar and ensure that it's visible
    // (because the OLD one is visible).
    hbar = new QScrollBar;
    hbar->hide();
    scrollArea.setHorizontalScrollBar(hbar);
    qApp->processEvents();
    QVERIFY(hbar->isVisible());

    vbar = new QScrollBar;
    vbar->hide();
    scrollArea.setVerticalScrollBar(vbar);
    qApp->processEvents();
    QVERIFY(vbar->isVisible());

    vbar->setRange(0, 0);
    qApp->processEvents();
    QVERIFY(!vbar->isVisible());

    hbar->setRange(0, 0);
    qApp->processEvents();
    QVERIFY(!hbar->isVisible());
}

// we need to make sure the viewport internal widget is named
// qt_scrollarea_viewport, otherwise we're going to confuse Squish
// and friends.
void tst_QAbstractScrollArea::objectNaming()
{
    QScrollArea area;
    QCOMPARE(area.viewport()->objectName(), QString("qt_scrollarea_viewport"));
}

class ViewportCrashWidget : public QDialog
{
public:
    ViewportCrashWidget()
    {
        // temprorary set PaintOnScreen to set the nativeChildrenForced flag.
        setAttribute(Qt::WA_PaintOnScreen, true);
        setAttribute(Qt::WA_PaintOnScreen, false);

        setAttribute(Qt::WA_DropSiteRegistered, true);

        startTimer(2000);
    }

    void timerEvent(QTimerEvent * /* event */) override
    {
        // should not crash.
        (void)new QScrollArea(this);
        accept();
    }
};

void tst_QAbstractScrollArea::viewportCrash()
{
    ViewportCrashWidget w;
    // should not crash
    w.exec();
}

Q_DECLARE_METATYPE(Qt::LayoutDirection);
Q_DECLARE_METATYPE(Qt::Key);

void tst_QAbstractScrollArea::task214488_layoutDirection_data()
{
    QTest::addColumn<Qt::LayoutDirection>("dir");
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<bool>("lessThan");

    QTest::newRow("LTR left")  << Qt::LeftToRight << Qt::Key_Left << true;
    QTest::newRow("LTR right") << Qt::LeftToRight << Qt::Key_Right << false;
    QTest::newRow("RTL left")  << Qt::RightToLeft << Qt::Key_Left << false;
    QTest::newRow("RTL right") << Qt::RightToLeft << Qt::Key_Right << true;
}

void tst_QAbstractScrollArea::task214488_layoutDirection()
{
    QScrollArea scrollArea;
    scrollArea.resize(200, 200);
    QWidget widget;
    widget.resize(600, 600);
    scrollArea.setWidget(&widget);
    scrollArea.show();
    QScrollBar *hbar = scrollArea.horizontalScrollBar();
    hbar->setValue((hbar->minimum() + hbar->maximum()) / 2);

    QFETCH(Qt::LayoutDirection, dir);
    QFETCH(Qt::Key, key);
    QFETCH(bool, lessThan);

    scrollArea.setLayoutDirection(dir);

    int refValue = hbar->value();
    {
        QKeyEvent ke(QEvent::KeyPress, key, Qt::NoModifier);
        qApp->sendEvent(&scrollArea, &ke);
    }
    QVERIFY(lessThan ? (hbar->value() < refValue) : (hbar->value() > refValue));
}

void tst_QAbstractScrollArea::patternBackground()
{
    QWidget topLevel;
    QScrollArea scrollArea(&topLevel);
    scrollArea.resize(200, 200);
    QWidget widget;
    widget.resize(600, 600);
    scrollArea.setWidget(&widget);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    QLinearGradient linearGrad(QPointF(250, 250), QPointF(300, 300));
    linearGrad.setColorAt(0, Qt::yellow);
    linearGrad.setColorAt(1, Qt::red);
    QBrush bg(linearGrad);
    scrollArea.viewport()->setPalette(QPalette(Qt::black, bg, bg, bg, bg, bg, bg, bg, bg));
    widget.setPalette(Qt::transparent);


    QImage image(200, 200, QImage::Format_ARGB32);
    scrollArea.render(&image);

    QCOMPARE(image.pixel(QPoint(20,20)) , QColor(Qt::yellow).rgb());

    QScrollBar *hbar = scrollArea.horizontalScrollBar();
    hbar->setValue(hbar->maximum());
    QScrollBar *vbar = scrollArea.verticalScrollBar();
    vbar->setValue(vbar->maximum());


    scrollArea.render(&image);
    QCOMPARE(image.pixel(QPoint(20,20)) , QColor(Qt::red).rgb());
}

class ScrollArea : public QAbstractScrollArea
{
public:
    using QAbstractScrollArea::setViewportMargins;
    using QAbstractScrollArea::viewportMargins;
};

void tst_QAbstractScrollArea::margins()
{
    ScrollArea area;
    QCOMPARE(area.viewportMargins(), QMargins());

    QMargins margins(10, 20, 30, 40);
    area.setViewportMargins(margins);
    QCOMPARE(area.viewportMargins(), margins);
}

void tst_QAbstractScrollArea::resizeWithOvershoot()
{
    QWidget window;

    QScrollArea scrollArea(&window);
    scrollArea.setWidget([]{
        QWidget *widget = new QWidget;
        widget->setFixedSize(QSize(0, 200));
        return widget;
    }());
    scrollArea.setGeometry(0, 20, 100, 100);

    QScroller::grabGesture(&scrollArea, QScroller::LeftMouseButtonGesture);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QPoint originAtRest = scrollArea.viewport()->pos();

    QPoint center = scrollArea.viewport()->mapToGlobal(scrollArea.viewport()->rect().center());
    center = window.windowHandle()->mapFromGlobal(center);
    QTest::mousePress(window.windowHandle(), Qt::LeftButton, {}, center);
    QTest::mouseMove(window.windowHandle(), center + QPoint(0, 50));
    QTRY_COMPARE(scrollArea.viewport()->pos(), originAtRest + QPoint(0, 25));
    QPoint overshootPosition = scrollArea.viewport()->pos();

    // trigger a layout of the scroll area while there's overshoot
    scrollArea.setGeometry(0, 0, 100, 120);
    QCOMPARE(scrollArea.viewport()->pos(), overshootPosition);
    QTest::mouseRelease(window.windowHandle(), Qt::LeftButton, {}, center + QPoint(0, 50));
    QTRY_COMPARE(scrollArea.viewport()->pos(), originAtRest);
    // Process a few more events and verify that the scroll area
    // doesn't overcompensate for the overshoot.
    QApplication::processEvents();
    QTRY_COMPARE(scrollArea.viewport()->pos(), originAtRest);
}

void tst_QAbstractScrollArea::sizeHint()
{
    class ScrollArea : public QAbstractScrollArea
    {
    public:
        QSize viewportSizeHint() const override { return {200, 200}; }
    } scrollArea;
    // We cannot reliable test the impact of the scrollbars on the size hint
    // if the style uses transient scrollbars, so use the class Windows style.
    const QString defaultStyle = QApplication::style()->name();
    QApplication::setStyle("Windows");
    auto resetStyle = qScopeGuard([defaultStyle]{
        QApplication::setStyle(defaultStyle);
    });
    scrollArea.setFrameShape(QFrame::NoFrame);
    scrollArea.setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    scrollArea.show();

    QSize sizeHint = scrollArea.sizeHint();
    QCOMPARE(sizeHint, scrollArea.viewportSizeHint());

    // check if the hScrollbar is taken into account
    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QCOMPARE_GT(scrollArea.sizeHint().height(), scrollArea.viewportSizeHint().height());

    // check if the vScrollbar is taken into account
    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QCOMPARE_GT(scrollArea.sizeHint().width(), scrollArea.viewportSizeHint().width());

    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    const QSize sizeHintWithScrollBars = scrollArea.sizeHint();
    QTRY_COMPARE_GT(sizeHintWithScrollBars.width(), sizeHint.width());
    QTRY_COMPARE_GT(sizeHintWithScrollBars.height(), sizeHint.height());

    sizeHint = scrollArea.sizeHint();

    // whether the scroll area itself is visible or not should not influence
    // the size hint
    scrollArea.hide();
    QCOMPARE(scrollArea.sizeHint(), sizeHint);
    scrollArea.show();
    QCOMPARE(scrollArea.sizeHint(), sizeHint);

    scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QCOMPARE(scrollArea.sizeHint(), scrollArea.viewportSizeHint());

    scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    scrollArea.verticalScrollBar()->setRange(0, 1);
    scrollArea.horizontalScrollBar()->setRange(0, 1);
    scrollArea.resize(sizeHint / 2);
    QApplication::processEvents(); // trigger lazy layout process
    QCOMPARE(scrollArea.sizeHint(), sizeHintWithScrollBars);
}

QTEST_MAIN(tst_QAbstractScrollArea)
#include "tst_qabstractscrollarea.moc"
