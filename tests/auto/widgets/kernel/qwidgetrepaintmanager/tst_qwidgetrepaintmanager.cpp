// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QApplication>

#include <private/qhighdpiscaling_p.h>
#include <private/qwidget_p.h>
#include <private/qwidgetrepaintmanager_p.h>
#include <qpa/qplatformbackingstore.h>

//#define MANUAL_DEBUG

class TestWidget : public QWidget
{
public:
    TestWidget(QWidget *parent = nullptr)
    : QWidget(parent)
    {
    }

    QSize sizeHint() const override
    {
        const int screenWidth =  QGuiApplication::primaryScreen()->geometry().width();
        const int width = qMax(200, 100 * ((screenWidth + 500) / 1000));
        return isWindow() ? QSize(width, width) : QSize(width - 40, width - 40);
    }

    void initialShow()
    {
        show();
        if (isWindow()) {
            QVERIFY(QTest::qWaitForWindowExposed(this));
            QVERIFY(waitForPainted());
        }
        paintedRegions = {};
    }

    bool waitForPainted(int timeout = 5000)
    {
        int remaining = timeout;
        QDeadlineTimer deadline(remaining, Qt::PreciseTimer);
        if (!QTest::qWaitFor([this]{ return !paintedRegions.isEmpty(); }, timeout))
            return false;

        // In case of multiple paint events:
        // Process events and wait until all have been consumed,
        // i.e. paintedRegions no longer changes.
        QRegion reg;
        while (remaining > 0 && reg != paintedRegions) {
            reg = paintedRegions;
            QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            if (reg == paintedRegions)
                return true;

            remaining = int(deadline.remainingTime());
        }
        return false;
    }

    QRegion takePaintedRegions()
    {
        QRegion result = paintedRegions;
        paintedRegions = {};
        return result;
    }
    QRegion paintedRegions;

    bool event(QEvent *event) override
    {
        const auto type = event->type();
        if (type == QEvent::WindowActivate || type == QEvent::WindowDeactivate)
            return true;
        return QWidget::event(event);
    }

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

class OpaqueWidget : public QWidget
{
public:
    OpaqueWidget(const QColor &col, QWidget *parent = nullptr)
    : QWidget(parent), fillColor(col)
    {
        setAttribute(Qt::WA_OpaquePaintEvent);
    }

    bool event(QEvent *event) override
    {
        const auto type = event->type();
        if (type == QEvent::WindowActivate || type == QEvent::WindowDeactivate)
            return true;
        return QWidget::event(event);
    }

protected:
    void paintEvent(QPaintEvent *e) override
    {
        Q_UNUSED(e);
        QPainter painter(this);
        fillColor.setBlue(paintCount % 255);
        painter.fillRect(e->rect(), fillColor);
#ifdef MANUAL_DEBUG
        ++paintCount;
        painter.drawText(rect(), Qt::AlignCenter, QString::number(paintCount));
#endif
    }

private:
    QColor fillColor;
    int paintCount = 0;
};

class Draggable : public OpaqueWidget
{
public:
    Draggable(QWidget *parent = nullptr)
    : OpaqueWidget(Qt::white, parent)
    {
    }

    Draggable(const QColor &col, QWidget *parent = nullptr)
    : OpaqueWidget(col, parent)
    {
        left = new OpaqueWidget(Qt::gray, this);
        top = new OpaqueWidget(Qt::gray, this);
        right = new OpaqueWidget(Qt::gray, this);
        bottom = new OpaqueWidget(Qt::gray, this);
    }

    QSize sizeHint() const override {
        return QSize(100, 100);
    }

protected:
    void resizeEvent(QResizeEvent *) override
    {
        if (!left)
            return;
        left->setGeometry(0, 0, 10, height());
        top->setGeometry(10, 0, width() - 10, 10);
        right->setGeometry(width() - 10, 10, 10, height() - 10);
        bottom->setGeometry(10, height() - 10, width() - 10, 10);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        lastPos = e->position().toPoint();
    }
    void mouseMoveEvent(QMouseEvent *e) override
    {
        QPoint pos = geometry().topLeft();
        pos += e->position().toPoint() - lastPos;
        move(pos);
    }
    void mouseReleaseEvent(QMouseEvent *) override
    {
        lastPos = {};
    }

private:
    OpaqueWidget *left = nullptr;
    OpaqueWidget *top = nullptr;
    OpaqueWidget *right = nullptr;
    OpaqueWidget *bottom = nullptr;
    QPoint lastPos;
};

class TestScene : public QWidget
{
public:
    TestScene()
    {
        setObjectName("scene");

        // opaque because it has an opaque background color and autoFillBackground is set
        area = new QWidget(this);
        area->setObjectName("area");
        area->setAutoFillBackground(true);
        QPalette palette;
        palette.setColor(QPalette::Window, QColor::fromRgb(0, 0, 0));
        area->setPalette(palette);

        // all these children set WA_OpaquePaintEvent
        redChild = new Draggable(Qt::red, area);
        redChild->setObjectName("redChild");

        greenChild = new Draggable(Qt::green, area);
        greenChild->setObjectName("greenChild");

        yellowChild = new Draggable(Qt::yellow, this);
        yellowChild->setObjectName("yellowChild");

        nakedChild = new Draggable(this);
        nakedChild->move(300, 0);
        nakedChild->setObjectName("nakedChild");

        bar = new OpaqueWidget(Qt::darkGray, this);
        bar->setObjectName("bar");
    }

    QWidget *area;
    QWidget *redChild;
    QWidget *greenChild;
    QWidget *yellowChild;
    QWidget *nakedChild;
    QWidget *bar;

    QSize sizeHint() const override { return QSize(400, 400); }

    bool event(QEvent *event) override
    {
        const auto type = event->type();
        if (type == QEvent::WindowActivate || type == QEvent::WindowDeactivate)
            return true;
        return QWidget::event(event);
    }

protected:
    void resizeEvent(QResizeEvent *) override
    {
        area->setGeometry(50, 50, width() - 100, height() - 100);
        bar->setGeometry(width() / 2 - 25, height() / 2, 50, height() / 2);
    }
};

class tst_QWidgetRepaintManager : public QObject
{
    Q_OBJECT

public:
    tst_QWidgetRepaintManager();

public slots:
    void initTestCase();
    void cleanup();

private slots:
    void basic();
    void children();
    void opaqueChildren();
    void staticContents();
    void scroll();
#if defined(QT_BUILD_INTERNAL)
    void scrollWithOverlap();
    void overlappedRegion();
    void fastMove();
    void moveAccross();
    void moveInOutOverlapped();

protected:
    /*
        This helper compares the widget as rendered into the backingstore with the widget
        as rendered via QWidget::grab. The latter always produces a fully rendered image,
        so differences indicate bugs in QWidgetRepaintManager's or QWidget's painting code.
    */
    bool compareWidget(QWidget *w)
    {
        QBackingStore *backingStore = w->window()->backingStore();
        Q_ASSERT(backingStore && backingStore->handle());
        QPlatformBackingStore *platformBackingStore = backingStore->handle();

        if (!waitForFlush(w)) {
            qWarning() << "Widget" << w << "failed to flush";
            return false;
        }

        QImage backingstoreContent = platformBackingStore->toImage();
        if (!w->isWindow()) {
            const qreal dpr = w->devicePixelRatioF();
            const QPointF offset = w->mapTo(w->window(), QPointF(0, 0)) * dpr;
            backingstoreContent = backingstoreContent.copy(offset.x(), offset.y(), w->width() * dpr, w->height() * dpr);
        }
        const QImage widgetRender = w->grab().toImage().convertToFormat(backingstoreContent.format());

        const bool result = backingstoreContent == widgetRender;

#ifdef MANUAL_DEBUG
        if (!result) {
            backingstoreContent.save(QString("/tmp/backingstore_%1_%2.png").arg(QTest::currentTestFunction(), QTest::currentDataTag()));
            widgetRender.save(QString("/tmp/grab_%1_%2.png").arg(QTest::currentTestFunction(), QTest::currentDataTag()));
        }
#endif
        return result;
    };

    QRegion dirtyRegion(QWidget *widget) const
    {
        return QWidgetPrivate::get(widget)->dirty;
    }
    bool waitForFlush(QWidget *widget) const
    {
        if (!widget)
            return true;

        auto *repaintManager = QWidgetPrivate::get(widget->window())->maybeRepaintManager();

        if (!repaintManager)
            return true;

        return QTest::qWaitFor([repaintManager]{ return !repaintManager->isDirty(); } );
    };
#endif // QT_BUILD_INTERNAL


private:
    const int m_fuzz;
    bool m_implementsScroll = false;
};

tst_QWidgetRepaintManager::tst_QWidgetRepaintManager() :
     m_fuzz(int(QHighDpiScaling::factor(QGuiApplication::primaryScreen())))
{
}

void tst_QWidgetRepaintManager::initTestCase()
{
    QWidget widget;
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    m_implementsScroll = widget.backingStore()->handle()->scroll(QRegion(widget.rect()), 1, 1);
    qInfo() << QGuiApplication::platformName() << "QPA backend implements scroll:" << m_implementsScroll;
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
    QVERIFY(QTest::qWaitForWindowExposed(child1));
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
    if (!m_implementsScroll)
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
    if (!m_implementsScroll)
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
    if (!m_implementsScroll)
        QEXPECT_FAIL("", "This should just repaint the newly exposed region", Continue);
    QCOMPARE(child->takePaintedRegions(), QRegion(0, 0, 10, child->height()));
    QCOMPARE(widget.takePaintedRegions(), QRegion());
}


#if defined(QT_BUILD_INTERNAL)

/*!
    Verify that overlapping children are repainted correctly when
    a widget is moved (via a scroll area) for such a distance that
    none of the old area is still visible. QTBUG-26269
*/
void tst_QWidgetRepaintManager::scrollWithOverlap()
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

            resize(600, 300);
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

    private:
        QScrollArea *m_scrollArea;
        QWidget *m_topWidget;
    };

    MainWindow w;
    w.show();

    QVERIFY(QTest::qWaitForWindowActive(&w));

    bool result = compareWidget(w.topWidget());
    // if this fails already, then the system we test on can't compare screenshots from grabbed widgets,
    // and we have to skip this test. Possible reasons are differences in surface formats or DPI, or
    // unrelated bugs in QPlatformBackingStore::toImage or QWidget::grab.
    if (!result)
        QSKIP("Cannot compare QWidget::grab with QScreen::grabWindow on this machine");

    // scroll the horizontal slider to the right side
    {
        w.scrollArea()->horizontalScrollBar()->setValue(w.scrollArea()->horizontalScrollBar()->maximum());
        QVERIFY(compareWidget(w.topWidget()));
    }

    // scroll the vertical slider down
    {
        w.scrollArea()->verticalScrollBar()->setValue(w.scrollArea()->verticalScrollBar()->maximum());
        QVERIFY(compareWidget(w.topWidget()));
    }

    // hide the top widget
    {
        w.topWidget()->hide();
        QVERIFY(compareWidget(w.scrollArea()->viewport()));
    }

    // scroll the horizontal slider to the left side
    {
        w.scrollArea()->horizontalScrollBar()->setValue(w.scrollArea()->horizontalScrollBar()->minimum());
        QVERIFY(compareWidget(w.scrollArea()->viewport()));
    }

    // scroll the vertical slider up
    {
        w.scrollArea()->verticalScrollBar()->setValue(w.scrollArea()->verticalScrollBar()->minimum());
        QVERIFY(compareWidget(w.scrollArea()->viewport()));
    }
}

/*!
    This tests QWidgetPrivate::overlappedRegion, which however is only used in the
    QWidgetRepaintManager, so the test is here.
*/
void tst_QWidgetRepaintManager::overlappedRegion()
{
    TestScene scene;

    if (scene.screen()->availableSize().width() < scene.sizeHint().width()
     || scene.screen()->availableSize().height() < scene.sizeHint().height()) {
        QSKIP("The screen on this system is too small for this test");
    }

    scene.show();
    QVERIFY(QTest::qWaitForWindowExposed(&scene));

    auto overlappedRegion = [](QWidget *widget, bool breakAfterFirst = false){
        auto *priv = QWidgetPrivate::get(widget);
        // overlappedRegion works on parent coordinates (crect, i.e. QWidget::geometry)
        return priv->overlappedRegion(widget->geometry(), breakAfterFirst);
    };

    // the yellow child is not overlapped
    QVERIFY(overlappedRegion(scene.yellowChild).isEmpty());
    // the green child is partially overlapped by the yellow child, which
    // is at position -50, -50 relative to the green child (and 100x100 large)
    QRegion overlap = overlappedRegion(scene.greenChild);
    QVERIFY(!overlap.isEmpty());
    QCOMPARE(overlap, QRegion(QRect(-50, -50, 100, 100)));
    // the red child is completely obscured by the green child, and partially
    // obscured by the yellow child. How exactly this is divided into rects is
    // irrelevant for the test.
    overlap = overlappedRegion(scene.redChild);
    QVERIFY(!overlap.isEmpty());
    QCOMPARE(overlap.boundingRect(), QRect(-50, -50, 150, 150));

    // moving the red child out of obscurity
    scene.redChild->move(100, 0);
    overlap = overlappedRegion(scene.redChild);
    QTRY_VERIFY(overlap.isEmpty());

    // moving the red child down so it's partially behind the bar
    scene.redChild->move(100, 100);
    overlap = overlappedRegion(scene.redChild);
    QTRY_VERIFY(!overlap.isEmpty());

    // moving the yellow child so it is partially overlapped by the bar
    scene.yellowChild->move(200, 200);
    overlap = overlappedRegion(scene.yellowChild);
    QTRY_VERIFY(!overlap.isEmpty());
}

void tst_QWidgetRepaintManager::fastMove()
{
    TestScene scene;
    scene.show();
    QVERIFY(QTest::qWaitForWindowExposed(&scene));

    QWidgetRepaintManager *repaintManager = QWidgetPrivate::get(&scene)->maybeRepaintManager();
    QVERIFY(repaintManager->dirtyRegion().isEmpty());

    // moving yellow; nothing obscured
    scene.yellowChild->move(QPoint(25, 0));
    QVERIFY(repaintManager->dirtyRegion().isEmpty()); // fast move
    if (m_implementsScroll) {
        QCOMPARE(repaintManager->dirtyWidgetList(), QList<QWidget *>() << &scene);
        QVERIFY(dirtyRegion(scene.yellowChild).isEmpty());
    } else {
        QCOMPARE(repaintManager->dirtyWidgetList(), QList<QWidget *>() << scene.yellowChild << &scene);
        QCOMPARE(dirtyRegion(scene.yellowChild), QRect(0, 0, 100, 100));
    }
    QCOMPARE(dirtyRegion(&scene), QRect(0, 0, 25, 100));
    QTRY_VERIFY(dirtyRegion(&scene).isEmpty());
    QVERIFY(compareWidget(&scene));
}

void tst_QWidgetRepaintManager::moveAccross()
{
    TestScene scene;
    scene.show();
    QVERIFY(QTest::qWaitForWindowExposed(&scene));

    QWidgetRepaintManager *repaintManager = QWidgetPrivate::get(&scene)->maybeRepaintManager();
    QVERIFY(repaintManager->dirtyRegion().isEmpty());

    for (int i = 0; i < 4; ++i) {
        scene.greenChild->move(scene.greenChild->pos() + QPoint(25, 0));
        waitForFlush(&scene);
    }
    QVERIFY(compareWidget(&scene));

    for (int i = 0; i < 16; ++i) {
        scene.redChild->move(scene.redChild->pos() + QPoint(25, 0));
        waitForFlush(&scene);
    }
    QVERIFY(compareWidget(&scene));

    for (int i = 0; i < qMin(scene.area->width(), scene.area->height()); i += 25) {
        scene.yellowChild->move(scene.yellowChild->pos() + QPoint(25, 25));
        waitForFlush(&scene);
    }
    QVERIFY(compareWidget(&scene));
}

void tst_QWidgetRepaintManager::moveInOutOverlapped()
{
    TestScene scene;
    scene.show();
    QVERIFY(QTest::qWaitForWindowExposed(&scene));

    QWidgetRepaintManager *repaintManager = QWidgetPrivate::get(&scene)->maybeRepaintManager();
    QVERIFY(repaintManager->dirtyRegion().isEmpty());

    // yellow out
    scene.yellowChild->move(QPoint(-100, 0));
    QVERIFY(!repaintManager->dirtyRegion().isEmpty()); // invalid dest rect
    QVERIFY(repaintManager->dirtyWidgetList().isEmpty());
    QVERIFY(waitForFlush(&scene));
    QVERIFY(compareWidget(&scene));

    // yellow in, obscured by bar
    scene.yellowChild->move(QPoint(scene.width() / 2, scene.height() / 2));
    QVERIFY(!repaintManager->dirtyRegion().isEmpty()); // invalid source rect
    QVERIFY(repaintManager->dirtyWidgetList().isEmpty());
    QVERIFY(waitForFlush(&scene));
    QVERIFY(compareWidget(&scene));

    // green out
    scene.greenChild->move(QPoint(-100, 0));
    QVERIFY(!repaintManager->dirtyRegion().isEmpty()); // invalid dest rect
    QVERIFY(repaintManager->dirtyWidgetList().isEmpty());
    QVERIFY(waitForFlush(&scene));
    QVERIFY(compareWidget(&scene));

    // green back in, obscured by bar
    scene.greenChild->move(QPoint(scene.area->width() / 2 - 50, scene.area->height() / 2 - 50));
    QVERIFY(!repaintManager->dirtyRegion().isEmpty()); // invalid source rect
    QVERIFY(repaintManager->dirtyWidgetList().isEmpty());
    QVERIFY(waitForFlush(&scene));
    QVERIFY(compareWidget(&scene));

    // red back under green
    scene.redChild->move(scene.greenChild->pos());
    QVERIFY(!repaintManager->dirtyRegion().isEmpty()); // destination rect obscured
    QVERIFY(repaintManager->dirtyWidgetList().isEmpty());
    QVERIFY(waitForFlush(&scene));
    QVERIFY(compareWidget(&scene));
}
#endif //# defined(QT_BUILD_INTERNAL)

QTEST_MAIN(tst_QWidgetRepaintManager)
#include "tst_qwidgetrepaintmanager.moc"
