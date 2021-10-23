/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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


#include <QTest>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QApplication>

#include <private/qhighdpiscaling_p.h>

class TestWidget : public QWidget
{
public:
    TestWidget(QWidget *parent = nullptr) : QWidget(parent) {}

    QSize sizeHint() const override
    {
        const int screenWidth =  QGuiApplication::primaryScreen()->geometry().width();
        const int width = qMax(200, 100 * ((screenWidth + 500) / 1000));
        return isWindow() ? QSize(width, width) : QSize(width - 40, width - 40);
    }

    void initialShow()
    {
        show();
        if (isWindow())
            QVERIFY(QTest::qWaitForWindowExposed(this));
        paintedRegions = {};
    }

    bool waitForPainted(int timeout = 5000)
    {
        return QTest::qWaitFor([this]{ return !paintedRegions.isEmpty(); }, timeout);
    }

    QRegion takePaintedRegions()
    {
        QRegion result = paintedRegions;
        paintedRegions = {};
        return result;
    }
    QRegion paintedRegions;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        paintedRegions += event->region();
        QPainter painter(this);
        const QBrush patternBrush = isWindow() ? QBrush(Qt::blue, Qt::VerPattern)
                                               : QBrush(Qt::red, Qt::HorPattern);
        painter.fillRect(rect(), patternBrush);
    }
};

class tst_QWidgetRepaintManager : public QObject
{
    Q_OBJECT

public:
    tst_QWidgetRepaintManager();

public slots:
    void cleanup();

private slots:
    void basic();
    void children();
    void opaqueChildren();
    void staticContents();
    void scroll();
    void moveWithOverlap();

private:
    const int m_fuzz;
};

tst_QWidgetRepaintManager::tst_QWidgetRepaintManager() :
     m_fuzz(int(QHighDpiScaling::factor(QGuiApplication::primaryScreen())))
{
}

void tst_QWidgetRepaintManager::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QWidgetRepaintManager::basic()
{
    TestWidget widget;
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    QCOMPARE(widget.takePaintedRegions(), QRegion(0, 0, widget.width(), widget.height()));

    widget.update();
    QVERIFY(widget.waitForPainted());
    QCOMPARE(widget.takePaintedRegions(), QRegion(0, 0, widget.width(), widget.height()));

    widget.repaint();
    QCOMPARE(widget.takePaintedRegions(), QRegion(0, 0, widget.width(), widget.height()));
}

/*!
    Children cannot assumed to be fully opaque, so the parent will repaint when the
    child repaints.
*/
void tst_QWidgetRepaintManager::children()
{
    if (QStringList{"android"}.contains(QGuiApplication::platformName()))
        QSKIP("This test fails on Android");

    TestWidget widget;
    widget.initialShow();

    TestWidget *child1 = new TestWidget(&widget);
    child1->move(20, 20);
    child1->show();
    QVERIFY(child1->waitForPainted());
    QCOMPARE(widget.takePaintedRegions(), QRegion(child1->geometry()));
    QCOMPARE(child1->takePaintedRegions(), QRegion(child1->rect()));

    child1->move(20, 30);
    QVERIFY(widget.waitForPainted());
    // both the old and the new area covered by child1 need to be repainted
    QCOMPARE(widget.takePaintedRegions(), QRegion(20, 20, child1->width(), child1->height() + 10));
    QCOMPARE(child1->takePaintedRegions(), QRegion(child1->rect()));

    TestWidget *child2 = new TestWidget(&widget);
    child2->move(30, 30);
    child2->raise();
    child2->show();

    QVERIFY(child2->waitForPainted());
    QCOMPARE(widget.takePaintedRegions(), QRegion(child2->geometry()));
    QCOMPARE(child1->takePaintedRegions(), QRegion(10, 0, child2->width() - 10, child2->height()));
    QCOMPARE(child2->takePaintedRegions(), QRegion(child2->rect()));

    child1->hide();
    QVERIFY(widget.waitForPainted());
    QCOMPARE(widget.paintedRegions, QRegion(child1->geometry()));
}

void tst_QWidgetRepaintManager::opaqueChildren()
{
    if (QStringList{"android"}.contains(QGuiApplication::platformName()))
        QSKIP("This test fails on Android");

    TestWidget widget;
    widget.initialShow();

    TestWidget *child1 = new TestWidget(&widget);
    child1->move(20, 20);
    child1->setAttribute(Qt::WA_OpaquePaintEvent);
    child1->show();

    QVERIFY(child1->waitForPainted());
    QCOMPARE(widget.takePaintedRegions(), QRegion());
    QCOMPARE(child1->takePaintedRegions(), child1->rect());

    child1->move(20, 30);
    QVERIFY(widget.waitForPainted());
    QCOMPARE(widget.takePaintedRegions(), QRegion(20, 20, child1->width(), 10));
    if (QGuiApplication::platformName() == "cocoa")
        QEXPECT_FAIL("", "child1 shouldn't get painted, we can just move the area of the backingstore", Continue);
    QCOMPARE(child1->takePaintedRegions(), QRegion());
}

/*!
    When resizing to be larger, a widget with Qt::WA_StaticContents set
    should only repaint the newly revealed areas.
*/
void tst_QWidgetRepaintManager::staticContents()
{
    TestWidget widget;
    widget.setAttribute(Qt::WA_StaticContents);
    widget.initialShow();

    const QSize oldSize = widget.size();

    widget.resize(widget.width() + 10, widget.height());

    QVERIFY(widget.waitForPainted());
    QEXPECT_FAIL("", "This should just repaint the newly exposed region", Continue);
    QCOMPARE(widget.takePaintedRegions(), QRegion(oldSize.width(), 0, 10, widget.height()));
}

/*!
    Scrolling a widget.
*/
void tst_QWidgetRepaintManager::scroll()
{
    if (QStringList{"android"}.contains(QGuiApplication::platformName()))
        QSKIP("This test fails on Android");

    TestWidget widget;
    widget.initialShow();

    widget.scroll(10, 0);
    QVERIFY(widget.waitForPainted());
    if (QGuiApplication::platformName() == "cocoa")
        QEXPECT_FAIL("", "This should just repaint the newly exposed region", Continue);
    QCOMPARE(widget.takePaintedRegions(), QRegion(0, 0, 10, widget.height()));

    TestWidget *child = new TestWidget(&widget);
    child->move(20, 20);
    child->initialShow();

    // a potentially semi-transparent child scrolling needs a full repaint
    child->scroll(10, 0);
    QVERIFY(child->waitForPainted());
    QCOMPARE(child->takePaintedRegions(), child->rect());
    QCOMPARE(widget.takePaintedRegions(), child->geometry());

    // a explicitly opaque child scrolling only needs the child to repaint newly exposed regions
    child->setAttribute(Qt::WA_OpaquePaintEvent);
    child->scroll(10, 0);
    QVERIFY(child->waitForPainted());
    if (QStringList{"cocoa", "android"}.contains(QGuiApplication::platformName()))
        QEXPECT_FAIL("", "This should just repaint the newly exposed region", Continue);
    QCOMPARE(child->takePaintedRegions(), QRegion(0, 0, 10, child->height()));
    QCOMPARE(widget.takePaintedRegions(), QRegion());
}


/*!
    Verify that overlapping children are repainted correctly when
    a widget is moved (via a scroll area) for such a distance that
    none of the old area is still visible. QTBUG-26269
*/
void tst_QWidgetRepaintManager::moveWithOverlap()
{
    if (QStringList{"android"}.contains(QGuiApplication::platformName()))
        QSKIP("This test fails on Android");

    class MainWindow : public QWidget
    {
    public:
        MainWindow(QWidget *parent = 0)
            : QWidget(parent, Qt::WindowStaysOnTopHint)
        {
            m_scrollArea = new QScrollArea(this);
            QWidget *w = new QWidget;
            w->setPalette(QPalette(Qt::gray));
            w->setAutoFillBackground(true);
            m_scrollArea->setWidget(w);
            m_scrollArea->resize(500, 100);
            w->resize(5000, 600);

            m_topWidget = new QWidget(this);
            m_topWidget->setPalette(QPalette(Qt::red));
            m_topWidget->setAutoFillBackground(true);
            m_topWidget->resize(300, 200);
        }

        void resizeEvent(QResizeEvent *e) override
        {
            QWidget::resizeEvent(e);
            // move scroll area and top widget to the center of the main window
            scrollArea()->move((width() - scrollArea()->width()) / 2, (height() - scrollArea()->height()) / 2);
            topWidget()->move((width() - topWidget()->width()) / 2, (height() - topWidget()->height()) / 2);
        }


        inline QScrollArea *scrollArea() const { return m_scrollArea; }
        inline QWidget *topWidget() const { return m_topWidget; }

        bool grabWidgetBackground(QWidget *w)
        {
            // To check widget's background we should compare two screenshots:
            // the first one is taken by system tools through QScreen::grabWindow(),
            // the second one is taken by Qt rendering to a pixmap via QWidget::grab().

            QScreen *screen = w->screen();
            const QRect screenGeometry = screen->geometry();
            QPoint globalPos = w->mapToGlobal(QPoint(0, 0));
            if (globalPos.x() >= screenGeometry.width())
                globalPos.rx() -= screenGeometry.x();
            if (globalPos.y() >= screenGeometry.height())
                globalPos.ry() -= screenGeometry.y();

            return QTest::qWaitFor([&]{
                QImage systemScreenshot = screen->grabWindow(winId(),
                                                             globalPos.x(), globalPos.y(),
                                                             w->width(), w->height()).toImage();
                systemScreenshot = systemScreenshot.convertToFormat(QImage::Format_RGB32);
                QImage qtScreenshot = w->grab().toImage().convertToFormat(systemScreenshot.format());
                return systemScreenshot == qtScreenshot;
            });
        };

    private:
        QScrollArea *m_scrollArea;
        QWidget *m_topWidget;
    };

    MainWindow w;
    w.showFullScreen();

    QVERIFY(QTest::qWaitForWindowActive(&w));

    bool result = w.grabWidgetBackground(w.topWidget());
    // if this fails already, then the system we test on can't compare screenshots from grabbed widgets,
    // and we have to skip this test. Possible reasons are that showing the window took too long, differences
    // in surface formats, or unrelated bugs in QScreen::grabWindow.
    if (!result)
        QSKIP("Cannot compare QWidget::grab with QScreen::grabWindow on this machine");

    // scroll the horizontal slider to the right side
    {
        w.scrollArea()->horizontalScrollBar()->setValue(w.scrollArea()->horizontalScrollBar()->maximum());
        QVERIFY(w.grabWidgetBackground(w.topWidget()));
    }

    // scroll the vertical slider down
    {
        w.scrollArea()->verticalScrollBar()->setValue(w.scrollArea()->verticalScrollBar()->maximum());
        QVERIFY(w.grabWidgetBackground(w.topWidget()));
    }

    // hide the top widget
    {
        w.topWidget()->hide();
        QVERIFY(w.grabWidgetBackground(w.scrollArea()->viewport()));
    }

    // scroll the horizontal slider to the left side
    {
        w.scrollArea()->horizontalScrollBar()->setValue(w.scrollArea()->horizontalScrollBar()->minimum());
        QVERIFY(w.grabWidgetBackground(w.scrollArea()->viewport()));
    }

    // scroll the vertical slider up
    {
        w.scrollArea()->verticalScrollBar()->setValue(w.scrollArea()->verticalScrollBar()->minimum());
        QVERIFY(w.grabWidgetBackground(w.scrollArea()->viewport()));
    }
}

QTEST_MAIN(tst_QWidgetRepaintManager)
#include "tst_qwidgetrepaintmanager.moc"
