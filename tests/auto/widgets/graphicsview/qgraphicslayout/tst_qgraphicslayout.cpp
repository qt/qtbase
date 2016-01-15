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
#include <QtGui>
#include <QtWidgets>
#include <math.h>

class tst_QGraphicsLayout : public QObject
{
Q_OBJECT

public:
    tst_QGraphicsLayout();
    virtual ~tst_QGraphicsLayout();

private slots:
    void sizeHints();
    void compressLayoutRequest();
    void automaticReparenting();
    void verifyActivate();
    void sizeHintOfHiddenLayout();
    void invalidate();
    void constructors();
    void alternativeLayoutItems();
    void ownership();
};

tst_QGraphicsLayout::tst_QGraphicsLayout()
{
}

tst_QGraphicsLayout::~tst_QGraphicsLayout()
{
}

void tst_QGraphicsLayout::sizeHints()
{

    QGraphicsView view;
    QGraphicsScene scene;
    QGraphicsWidget *window = new QGraphicsWidget();
    scene.addItem(window);
    QGraphicsLinearLayout *lout = new QGraphicsLinearLayout(window);
    lout->setContentsMargins(0,0,0,0);
    QGraphicsWidget *gw = new QGraphicsWidget(window);
    gw->setMinimumSize(QSizeF(10,10));
    gw->setPreferredSize(QSizeF(100,100));
    gw->setMaximumSize(QSizeF(500,500));
    lout->addItem(gw);
    QCOMPARE(lout->effectiveSizeHint(Qt::MinimumSize), gw->effectiveSizeHint(Qt::MinimumSize));
    QCOMPARE(lout->effectiveSizeHint(Qt::PreferredSize), gw->effectiveSizeHint(Qt::PreferredSize));
    QCOMPARE(lout->effectiveSizeHint(Qt::MaximumSize), gw->effectiveSizeHint(Qt::MaximumSize));

}

enum FunctionType {
    SetGeometry = 0,
    Invalidate,
    NumFunctionTypes
};



class TestGraphicsWidget : public QGraphicsWidget {
public:
    TestGraphicsWidget(QGraphicsWidget *parent = 0) : QGraphicsWidget(parent)
    { }

    bool event(QEvent *e) {
        ++(m_eventCount[int(e->type())]);
        return QGraphicsWidget::event(e);
    }

    int eventCount(QEvent::Type type) {
        return m_eventCount.value(int(type));
    }

    void clearEventCount() {
        m_eventCount.clear();
    }

    void clearCounters() {
        m_eventCount.clear();
        functionCount.clear();
    }

    void setGeometry(const QRectF &rect)
    {
        QGraphicsWidget::setGeometry(rect);
        ++(functionCount[SetGeometry]);
    }

    void callUpdateGeometry()
    {
        // updateGeometry() is protected
        QGraphicsWidget::updateGeometry();
    }
    QMap<FunctionType, int> functionCount;
private:
    QMap<int, int> m_eventCount;
};

void tst_QGraphicsLayout::compressLayoutRequest()
{
    QGraphicsView view;
    QGraphicsScene scene;
    TestGraphicsWidget *tw = new TestGraphicsWidget();
    scene.addItem(tw);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QGraphicsLinearLayout *lout = new QGraphicsLinearLayout(tw);
    for (int i = 0; i < 4; ++i) {
        QGraphicsWidget *gw = new QGraphicsWidget(tw);
        gw->setPreferredSize(QSizeF(50, 50));
        lout->addItem(gw);
    }
    QApplication::processEvents();
    QCOMPARE(tw->eventCount(QEvent::LayoutRequest), 1);
}

void tst_QGraphicsLayout::automaticReparenting()
{
    QGraphicsView view;
    QGraphicsScene scene;
    {
        QGraphicsWidget *w = new QGraphicsWidget();
        QGraphicsLinearLayout *l = new QGraphicsLinearLayout(w);
        QGraphicsWidget *w1 = new QGraphicsWidget;
        l->addItem(w1);
        scene.addItem(w);
        QCOMPARE(w1->parentWidget(), w);
        delete w;
    }
    {
        QGraphicsWidget *w = new QGraphicsWidget();
        QGraphicsLinearLayout *l = new QGraphicsLinearLayout(w);
        QGraphicsWidget *w1 = new QGraphicsWidget;
        l->addItem(w1);
        scene.addItem(w);
        QCOMPARE(w1->parentWidget(), w);

        QGraphicsWidget *ww = new QGraphicsWidget();
        QGraphicsLinearLayout *l1 = new QGraphicsLinearLayout(ww);
#if !defined(Q_OS_MAC) && defined(QT_DEBUG)
        QTest::ignoreMessage(QtWarningMsg, "QGraphicsLayout::addChildLayoutItem: QGraphicsWidget \"\""
                             " in wrong parent; moved to correct parent");
#endif
        l1->addItem(w1);
        QCOMPARE(w1->parentWidget(), ww);
        delete w;
    }

    QGraphicsWidget *window = new QGraphicsWidget();
    scene.addItem(window);
    view.show();
    QGraphicsLinearLayout *l1 = new QGraphicsLinearLayout();
    QGraphicsWidget *w1 = new QGraphicsWidget();
    l1->addItem(w1);
    QGraphicsWidget *w2 = new QGraphicsWidget();
    l1->addItem(w2);
    QCOMPARE(w1->parentItem(), static_cast<QGraphicsItem*>(0));
    QCOMPARE(w2->parentItem(), static_cast<QGraphicsItem*>(0));
    scene.addItem(w1);
    QCOMPARE(w1->parentItem(), static_cast<QGraphicsItem*>(0));
    window->setLayout(l1);
    QCOMPARE(w1->parentItem(), static_cast<QGraphicsItem*>(window));
    QCOMPARE(w2->parentItem(), static_cast<QGraphicsItem*>(window));

    // Sublayouts
    QGraphicsLinearLayout *l2 = new QGraphicsLinearLayout();
    QGraphicsWidget *w3 = new QGraphicsWidget();
    l2->addItem(w3);
    QGraphicsWidget *w4 = new QGraphicsWidget();
    l2->addItem(w4);
    QGraphicsLinearLayout *l3 = new QGraphicsLinearLayout();
    l2->addItem(l3);
    QGraphicsWidget *window2 = new QGraphicsWidget();
    scene.addItem(window2);
    window2->setLayout(l2);

    QCOMPARE(w3->parentItem(), static_cast<QGraphicsItem*>(window2));
    QCOMPARE(w4->parentItem(), static_cast<QGraphicsItem*>(window2));

    // graphics item with another parent
    QGraphicsLinearLayout *l5 = new QGraphicsLinearLayout();
    l5->addItem(w1);
    l5->addItem(w2);
    QCOMPARE(w1->parentItem(), static_cast<QGraphicsItem*>(window));
    QCOMPARE(w2->parentItem(), static_cast<QGraphicsItem*>(window));
    QGraphicsLinearLayout *l4 = new QGraphicsLinearLayout();
    l4->addItem(l5);
    QGraphicsWidget *window3 = new QGraphicsWidget();
    scene.addItem(window3);
    window3->setLayout(l4);

    QCOMPARE(w1->parentItem(), static_cast<QGraphicsItem*>(window3));
    QCOMPARE(w2->parentItem(), static_cast<QGraphicsItem*>(window3));
}

class TestLayout : public QGraphicsLinearLayout
{
    public:
    TestLayout(QGraphicsLayoutItem *parent = 0)
        : QGraphicsLinearLayout(parent)
    {
        setContentsMargins(0,0,0,0);
        setSpacing(0);
    }

    void setGeometry(const QRectF &rect)
    {
        ++(functionCount[SetGeometry]);
        QGraphicsLinearLayout::setGeometry(rect);
    }

    void invalidate()
    {
        ++(functionCount[Invalidate]);
        QGraphicsLinearLayout::invalidate();
    }

    void clearCounters() {
        functionCount.clear();
    }

    QMap<FunctionType, int> functionCount;
};

void tst_QGraphicsLayout::verifyActivate()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsWidget *window = new QGraphicsWidget();
    scene.addItem(window);
    TestLayout *lout = new TestLayout(window);
    QGraphicsWidget *w = new QGraphicsWidget();
    lout->addItem(w);
    window->setLayout(lout);

    QCOMPARE(lout->functionCount[SetGeometry], 0);
    window->setVisible(false);
    QCOMPARE(lout->functionCount[SetGeometry], 0);
    window->setVisible(true);
    // on polish or the first time a widget is shown, the widget is resized.
    QCOMPARE(lout->functionCount[SetGeometry], 1);

}


void tst_QGraphicsLayout::sizeHintOfHiddenLayout()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsWidget *window = new QGraphicsWidget(0, Qt::Window);
    scene.addItem(window);
    TestLayout *lout = new TestLayout(window);
    lout->setContentsMargins(1,2,2,1);
    QGraphicsWidget *w = new QGraphicsWidget;
    w->setPreferredSize(20, 20);
    w->setMaximumSize(50, 50);
    lout->addItem(w);
    window->setLayout(lout);

    for (int pass = 0; pass < 3; ++pass) {
        QCOMPARE(lout->sizeHint(Qt::MinimumSize), QSizeF(3,3));
        QCOMPARE(lout->sizeHint(Qt::PreferredSize), QSizeF(23,23));
        QCOMPARE(lout->sizeHint(Qt::MaximumSize), QSizeF(53,53));
        window->setVisible(pass % 2);
    }
}

static void clearAllCounters(TestGraphicsWidget *widget)
{
    if (!widget)
        return;
    widget->clearCounters();
    TestLayout *layout = static_cast<TestLayout *>(widget->layout());
    if (layout) {
        layout->clearCounters();
        for (int i = layout->count() - 1; i >=0; --i) {
            QGraphicsLayoutItem *item = layout->itemAt(i);
            if (item->isLayout()) {
                // ### Not used ATM
                //TestLayout *lay = static_cast<TestLayout*>(static_cast<QGraphicsLayout*>(item));
                //clearAllCounters(lay);
            } else {
                TestGraphicsWidget *wid = static_cast<TestGraphicsWidget *>(item);
                clearAllCounters(wid);
            }
        }
    }
}

static void activateAndReset(TestGraphicsWidget *widget)
{
    QApplication::sendPostedEvents();
    QApplication::processEvents();
    if (widget->layout())
        widget->layout()->activate();
    clearAllCounters(widget);
}


void tst_QGraphicsLayout::invalidate()
{
    QGraphicsLayout::setInstantInvalidatePropagation(true);
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    TestGraphicsWidget *a = new TestGraphicsWidget;
    a->setData(0, QString("a"));
    scene.addItem(a);
    TestLayout *alay = new TestLayout(a);
    TestGraphicsWidget *b = new TestGraphicsWidget;
    b->setData(0, QString("b"));
    alay->addItem(b);
    TestLayout *blay = new TestLayout(b);
    TestGraphicsWidget *e = new TestGraphicsWidget;
    e->setData(0, QString("e"));
    blay->addItem(e);


    TestGraphicsWidget *c = new TestGraphicsWidget;
    c->setData(0, QString("c"));
    alay->addItem(c);
    TestLayout *clay = new TestLayout(c);
    TestGraphicsWidget *f = new TestGraphicsWidget;
    f->setData(0, QString("f"));
    clay->addItem(f);

    TestGraphicsWidget *d = new TestGraphicsWidget;
    d->setData(0, QString("d"));
    alay->addItem(d);
    TestLayout *dlay = new TestLayout(d);
    TestGraphicsWidget *g = new TestGraphicsWidget;
    g->setData(0, QString("g"));
    dlay->addItem(g);

    view.show();

    {
        clearAllCounters(a);

        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();

        alay->activate();
        QCOMPARE(alay->isActivated(), true);
        QCOMPARE(blay->isActivated(), true);
        QCOMPARE(clay->isActivated(), true);
        QCOMPARE(dlay->isActivated(), true);
    }

    {
        clearAllCounters(a);
        e->callUpdateGeometry();
        QCOMPARE(alay->isActivated(), false);
        QCOMPARE(blay->isActivated(), false);
        QCOMPARE(clay->isActivated(), true);
        QCOMPARE(dlay->isActivated(), true);
        QCOMPARE(a->eventCount(QEvent::LayoutRequest), 0);
        QCOMPARE(b->eventCount(QEvent::LayoutRequest), 0);
        QCOMPARE(c->eventCount(QEvent::LayoutRequest), 0);
        QCOMPARE(d->eventCount(QEvent::LayoutRequest), 0);

        // should only invalidate ascendants of e
        QCOMPARE(blay->functionCount[Invalidate], 1);
        QCOMPARE(alay->functionCount[Invalidate], 1);
        // not siblings
        QCOMPARE(clay->functionCount[Invalidate], 0);
        QCOMPARE(dlay->functionCount[Invalidate], 0);

        QApplication::sendPostedEvents();
        QCOMPARE(a->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(b->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(c->eventCount(QEvent::LayoutRequest), 0);
        QCOMPARE(d->eventCount(QEvent::LayoutRequest), 0);
    }


    {
        activateAndReset(a);
        f->callUpdateGeometry();
        QCOMPARE(alay->isActivated(), false);
        QCOMPARE(blay->isActivated(), true);
        QCOMPARE(clay->isActivated(), false);
        QCOMPARE(dlay->isActivated(), true);

        QCoreApplication::sendPostedEvents();
        QCOMPARE(a->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(b->eventCount(QEvent::LayoutRequest), 0);
        QCOMPARE(c->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(d->eventCount(QEvent::LayoutRequest), 0);

        QCOMPARE(a->functionCount[SetGeometry], 1);
        QCOMPARE(alay->functionCount[SetGeometry], 1);

        QCOMPARE(b->functionCount[SetGeometry], 1);
        QCOMPARE(c->functionCount[SetGeometry], 1);
        QCOMPARE(d->functionCount[SetGeometry], 1);
        // Since nothing really changed, blay and dlay don't need
        // to be resized.
        QCOMPARE(blay->functionCount[SetGeometry], 0);
        QCOMPARE(clay->functionCount[SetGeometry], 1);
        QCOMPARE(dlay->functionCount[SetGeometry], 0);

        QCOMPARE(f->functionCount[SetGeometry], 1);

        QCOMPARE(a->size(), QSizeF(150, 50));
    }

    {
        activateAndReset(a);
        f->setPreferredSize(QSizeF(60,50));
        QCOMPARE(alay->isActivated(), false);
        QCOMPARE(blay->isActivated(), true);
        QCOMPARE(clay->isActivated(), false);
        QCOMPARE(dlay->isActivated(), true);

        QCOMPARE(c->eventCount(QEvent::LayoutRequest), 0);
        QCoreApplication::sendPostedEvents();
        QCOMPARE(a->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(b->eventCount(QEvent::LayoutRequest), 0);
        QCOMPARE(c->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(d->eventCount(QEvent::LayoutRequest), 0);

        QCOMPARE(a->functionCount[SetGeometry], 1);
        QCOMPARE(alay->functionCount[SetGeometry], 1);

        QCOMPARE(b->functionCount[SetGeometry], 1);
        QCOMPARE(c->functionCount[SetGeometry], 1);
        QCOMPARE(d->functionCount[SetGeometry], 1);
        // f actually got wider, need to rearrange its siblings
        QCOMPARE(blay->functionCount[SetGeometry], 1);
        QCOMPARE(clay->functionCount[SetGeometry], 1);
        QCOMPARE(dlay->functionCount[SetGeometry], 1);

        QCOMPARE(e->functionCount[SetGeometry], 1);
        QCOMPARE(f->functionCount[SetGeometry], 1);
        QCOMPARE(g->functionCount[SetGeometry], 1);

        QVERIFY(e->size().width() < f->size().width());
        QVERIFY(g->size().width() < f->size().width());
    }

    {
        // resize f so much that it'll force a resize of the top widget
        // this will currently generate two setGeometry() calls on the child layout
        // of the top widget.
        activateAndReset(a);
        f->setPreferredSize(QSizeF());
        f->setMinimumSize(QSizeF(200,50));
        QCOMPARE(alay->isActivated(), false);
        QCOMPARE(blay->isActivated(), true);
        QCOMPARE(clay->isActivated(), false);
        QCOMPARE(dlay->isActivated(), true);

        QCOMPARE(c->eventCount(QEvent::LayoutRequest), 0);
        QCoreApplication::sendPostedEvents();
        QCOMPARE(a->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(b->eventCount(QEvent::LayoutRequest), 0);
        QCOMPARE(c->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(d->eventCount(QEvent::LayoutRequest), 0);

        QCOMPARE(a->functionCount[SetGeometry], 1);

        /* well, ideally one call to setGeometry(), but it will currently
         * get two calls to setGeometry():
         *   1. The first LayoutRequest will call activate() - that will call
         *      setGeometry() on the layout. This geometry will be based on
         *      the widget geometry which is not correct at this moment.
         *      (it is still 150 wide)
         *   2. Next, we check if the widget is top level, and then we call
         *      parentWidget->resize(parentWidget->size());
         *      This will be adjusted to be minimum 200 pixels wide.
         *      The new size will then be propagated down to the layout
         *
         */
        QCOMPARE(alay->functionCount[SetGeometry], 2);

        QCOMPARE(b->functionCount[SetGeometry], 2);
        QCOMPARE(c->functionCount[SetGeometry], 2);
        QCOMPARE(d->functionCount[SetGeometry], 2);
        // f actually got wider, need to rearrange its siblings
        QCOMPARE(blay->functionCount[SetGeometry], 1);
        QCOMPARE(clay->functionCount[SetGeometry], 1);
        QCOMPARE(dlay->functionCount[SetGeometry], 1);

        QCOMPARE(e->functionCount[SetGeometry], 1);
        QCOMPARE(f->functionCount[SetGeometry], 1);
        QCOMPARE(g->functionCount[SetGeometry], 1);

        QVERIFY(e->size().width() < f->size().width());
        QVERIFY(g->size().width() < f->size().width());
    }

    {
        f->setPreferredSize(QSizeF());
        f->setMinimumSize(QSizeF());
        a->adjustSize();
        activateAndReset(a);
        // update two different leaf widgets,
        // eventCount and functionCount should never be >= 2
        e->callUpdateGeometry();
        g->callUpdateGeometry();
        QCOMPARE(alay->isActivated(), false);
        QCOMPARE(blay->isActivated(), false);
        QCOMPARE(clay->isActivated(), true);
        QCOMPARE(dlay->isActivated(), false);

        QCoreApplication::sendPostedEvents();
        QCOMPARE(a->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(b->eventCount(QEvent::LayoutRequest), 1);
        QCOMPARE(c->eventCount(QEvent::LayoutRequest), 0);
        QCOMPARE(d->eventCount(QEvent::LayoutRequest), 1);

        QCOMPARE(a->functionCount[SetGeometry], 1);
        QCOMPARE(alay->functionCount[SetGeometry], 1);

        QCOMPARE(b->functionCount[SetGeometry], 1);
        QCOMPARE(c->functionCount[SetGeometry], 1);
        QCOMPARE(d->functionCount[SetGeometry], 1);
        // f actually got wider, need to rearrange its siblings
        QCOMPARE(blay->functionCount[SetGeometry], 1);
        QCOMPARE(clay->functionCount[SetGeometry], 0);
        QCOMPARE(dlay->functionCount[SetGeometry], 1);

        QCOMPARE(e->functionCount[SetGeometry], 1);
        QCOMPARE(f->functionCount[SetGeometry], 0);
        QCOMPARE(g->functionCount[SetGeometry], 1);

    }

    QGraphicsLayout::setInstantInvalidatePropagation(false);
}

class Layout : public QGraphicsLayout
{
public:
    Layout(QGraphicsLayoutItem *parentItem = 0) : QGraphicsLayout(parentItem) {}

    void setGeometry(const QRectF &rect)
    {
        QGraphicsLayout::setGeometry(rect);
    }

    int count() const {
        return 0;
    }

    QGraphicsLayoutItem *itemAt(int index) const {
        Q_UNUSED(index);
        return 0;
    }

    void removeAt(int index)
    {
        Q_UNUSED(index);
    }

protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const
    {
        Q_UNUSED(constraint);
        Q_UNUSED(which);
        return QSizeF(100,100);
    }

};

void tst_QGraphicsLayout::constructors()
{
    // Strange test, but see the fix that was with this submit
    QVector<Layout*> layouts;
    for (int pass = 0; pass < 5; ++pass) {
        Layout *lay = new Layout();
        layouts << lay;
        qreal left, top, right, bottom;
        lay->getContentsMargins(&left, &top, &right, &bottom);
        // Test if the style defaults are sane (should always be ints)
        double intpart;
        QVERIFY(modf(left, &intpart) == 0.0);
        QVERIFY(modf(top, &intpart) == 0.0);
        QVERIFY(modf(right, &intpart) == 0.0);
        QVERIFY(modf(bottom, &intpart) == 0.0);

        lay->setContentsMargins(1, 2, 4, 8);
        lay->getContentsMargins(&left, &top, &right, &bottom);

        QCOMPARE(int(left), 1);
        QCOMPARE(int(top), 2);
        QCOMPARE(int(right), 4);
        QCOMPARE(int(bottom), 8);
    }

    qDeleteAll(layouts);
}

class AnimatedLayoutItem : public QGraphicsLayoutItem {
public:
    AnimatedLayoutItem(QGraphicsRectItem *item)
    : QGraphicsLayoutItem()
    {
        setGraphicsItem(item);
    }

    void setGeometry(const QRectF &geom);

    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

    inline QGraphicsRectItem *rectItem() {
        return static_cast<QGraphicsRectItem *>(graphicsItem());
    }

    QRectF m_geom;
private:
    AnimatedLayoutItem() {}
};

void AnimatedLayoutItem::setGeometry(const QRectF &geom)
{
    QGraphicsLayoutItem::setGeometry(geom);
}

QSizeF AnimatedLayoutItem::sizeHint(Qt::SizeHint which, const QSizeF & /* constraint */) const
{
    switch (which) {
    case Qt::MinimumSize:
        return QSizeF(32,32);
    case Qt::PreferredSize:
        return QSizeF(160,90);
    case Qt::MaximumSize:
        return QSizeF(1000,1000);
    default:
        return QSizeF(300, 300);
    }
}

class AnimatedLayout : public QObject, public QGraphicsLinearLayout {
    Q_OBJECT
public:
    AnimatedLayout(QGraphicsWidget *widget) : QGraphicsLinearLayout(widget), m_timeline(500, this)
    {
        connect(&m_timeline, SIGNAL(valueChanged(qreal)), this, SLOT(valueChanged(qreal)));
    }

    void setGeometry(const QRectF &geom) {
        fromGeoms.clear();
        toGeoms.clear();
        for (int i = 0; i < count(); ++i) {
            fromGeoms << itemAt(i)->geometry();
        }

        QGraphicsLinearLayout::setGeometry(geom);

        for (int i = 0; i < count(); ++i) {
            toGeoms << itemAt(i)->geometry();
        }
        m_timeline.start();
    }

private slots:
    void valueChanged(qreal value) {
        for (int i = 0; i < fromGeoms.count(); ++i) {
            QGraphicsLayoutItem *li = itemAt(i);
            QRectF from = fromGeoms.at(i);
            QRectF to = toGeoms.at(i);

            QRectF geom(from.topLeft() + (to.topLeft() - from.topLeft()) * value,
                        from.size() + (to.size() - from.size()) * value);
            static_cast<QGraphicsRectItem*>(li->graphicsItem())->setRect(geom);
        }
    }
private:
    QTimeLine m_timeline;
    QVector<QRectF> fromGeoms;
    QVector<QRectF> toGeoms;
};


void tst_QGraphicsLayout::alternativeLayoutItems()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsWidget *window = new QGraphicsWidget;
    scene.addItem(window);
    AnimatedLayout *lout = new AnimatedLayout(window);
    lout->setContentsMargins(0, 0, 0, 0);
    lout->setSpacing(0);

    QGraphicsRectItem *item1 = new QGraphicsRectItem;
    AnimatedLayoutItem *li1 = new AnimatedLayoutItem(item1);
    lout->addItem(li1);

    QGraphicsRectItem *item2 = new QGraphicsRectItem;
    AnimatedLayoutItem *li2 = new AnimatedLayoutItem(item2);
    lout->addItem(li2);

    QGraphicsRectItem *item3 = new QGraphicsRectItem;
    AnimatedLayoutItem *li3 = new AnimatedLayoutItem(item3);
    lout->addItem(li3);

    window->setLayout(lout);

    window->setGeometry(0, 0, 99, 99);
    view.setSceneRect(QRectF(-10, -10, 110, 110));
    view.resize(150, 150);
    view.show();

    QTRY_COMPARE(static_cast<QGraphicsRectItem*>(li1->graphicsItem())->rect(), QRectF( 0, 0, 33, 99));
    QTRY_COMPARE(static_cast<QGraphicsRectItem*>(li2->graphicsItem())->rect(), QRectF(33, 0, 33, 99));
    QTRY_COMPARE(static_cast<QGraphicsRectItem*>(li3->graphicsItem())->rect(), QRectF(66, 0, 33, 99));

    lout->setOrientation(Qt::Vertical);

    QTRY_COMPARE(static_cast<QGraphicsRectItem*>(li1->graphicsItem())->rect(), QRectF(0, 0,  99, 33));
    QTRY_COMPARE(static_cast<QGraphicsRectItem*>(li2->graphicsItem())->rect(), QRectF(0, 33, 99, 33));
    QTRY_COMPARE(static_cast<QGraphicsRectItem*>(li3->graphicsItem())->rect(), QRectF(0, 66, 99, 33));

}

class CustomLayoutItem : public QGraphicsLayoutItem {
public:
    CustomLayoutItem(QSet<QGraphicsLayoutItem*> *destructedSet)
    : QGraphicsLayoutItem()
    {
        m_destructedSet = destructedSet;
        setOwnedByLayout(true);
    }

    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

    ~CustomLayoutItem() {
        m_destructedSet->insert(this);
    }
private:
    QSet<QGraphicsLayoutItem*> *m_destructedSet;
};

QSizeF CustomLayoutItem::sizeHint(Qt::SizeHint which, const QSizeF & /* constraint */) const
{
    switch (which) {
    case Qt::MinimumSize:
        return QSizeF(32,32);
    case Qt::PreferredSize:
        return QSizeF(160,90);
    case Qt::MaximumSize:
        return QSizeF(1000,1000);
    default:
        return QSizeF(300, 300);
    }
}

class CustomGraphicsWidget : public QGraphicsWidget {
public:
    CustomGraphicsWidget(QSet<QGraphicsLayoutItem*> *destructedSet = 0)
    : QGraphicsWidget()
    {
        m_destructedSet = destructedSet;
    }

    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * = 0)
    {
        const QRect r = option->rect.adjusted(0, 0, -1, -1);
        painter->drawLine(r.topLeft(), r.bottomRight());
        painter->drawLine(r.bottomLeft(), r.topRight());
        painter->drawRect(r);
    }

    ~CustomGraphicsWidget() {
        if (m_destructedSet)
            m_destructedSet->insert(this);
    }
private:
    QSet<QGraphicsLayoutItem*> *m_destructedSet;
};

QSizeF CustomGraphicsWidget::sizeHint(Qt::SizeHint which, const QSizeF & /* constraint */) const
{
    switch (which) {
    case Qt::MinimumSize:
        return QSizeF(32,32);
    case Qt::PreferredSize:
        return QSizeF(160,90);
    case Qt::MaximumSize:
        return QSizeF(1000,1000);
    default:
        return QSizeF(300, 300);
    }
}

static bool compareSets(const QSet<QGraphicsLayoutItem*> &actual, const QSet<QGraphicsLayoutItem*> &expected)
{
    if (actual != expected) {
        qDebug() << "actual:" << actual << "expected:" << expected;
        return false;
    }
    return true;
}

class CustomLayout : public QGraphicsLayout
{
public :
CustomLayout(QGraphicsLayoutItem *parent)
    : QGraphicsLayout(parent)
{
}


~CustomLayout()
{
}

int count() const
{
    return items.count();
}

QGraphicsLayoutItem* itemAt(int index) const
{
    return items.at(index);
}


void removeAt(int index)
{
    items.removeAt(index);
}

void addItem(QGraphicsLayoutItem *item)
{
    insertItem(items.count(), item);
}

void insertItem(int index, QGraphicsLayoutItem *item)
{
    index = qBound(0, index, items.count());

    item->setParentLayoutItem(this);

    QGraphicsWidget *widget = static_cast<QGraphicsWidget *>(item);
    updateParentWidget(widget);


    if (index == items.count()) {
        items.append(item);
    } else {
        items.insert(index, item);
    }

    updateGeometry();
    activate();
}

void updateParentWidget(QGraphicsWidget *item)
{
    QGraphicsLayoutItem *parentItem = parentLayoutItem();
    while (parentItem && parentItem->isLayout()) {
        parentItem = parentItem->parentLayoutItem();
    }

    if (parentItem) {
        item->setParentItem(static_cast<QGraphicsWidget*>(parentItem));
    }
}

QSizeF sizeHint(Qt::SizeHint /* which */, const QSizeF & /* constraint */) const
{
    return QSizeF(50,50);
}

QList<QGraphicsLayoutItem*> items;

};

void tst_QGraphicsLayout::ownership()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    {
        QGraphicsLinearLayout *lay = new QGraphicsLinearLayout;
        QSet<QGraphicsLayoutItem*> destructedSet;
        CustomLayoutItem *li1 = new CustomLayoutItem(&destructedSet);
        lay->addItem(li1);
        CustomLayoutItem *li2 = new CustomLayoutItem(&destructedSet);
        lay->addItem(li2);
        CustomLayoutItem *li3 = new CustomLayoutItem(&destructedSet);
        lay->addItem(li3);
        destructedSet.clear();

        delete lay;
        QSet<QGraphicsLayoutItem*> expected;
        expected << li1 << li2 << li3;
        QVERIFY(compareSets(destructedSet, expected));
    }

    {
        QGraphicsWidget *window = new QGraphicsWidget;
        QGraphicsLinearLayout *lay = new QGraphicsLinearLayout;
        QSet<QGraphicsLayoutItem*> destructedSet;
        CustomGraphicsWidget *li1 = new CustomGraphicsWidget(&destructedSet);
        lay->addItem(li1);
        CustomGraphicsWidget *li2 = new CustomGraphicsWidget(&destructedSet);
        lay->addItem(li2);
        CustomGraphicsWidget *li3 = new CustomGraphicsWidget(&destructedSet);
        lay->addItem(li3);
        window->setLayout(lay);
        scene.addItem(window);

        destructedSet.clear();
        window->setLayout(0);
        QCOMPARE(destructedSet.count(), 0);
        delete window;
    }

    {
        QGraphicsWidget *window = new QGraphicsWidget(0, Qt::Window);
        QGraphicsLinearLayout *lay = new QGraphicsLinearLayout;

        CustomGraphicsWidget *li1 = new CustomGraphicsWidget;
        lay->addItem(li1);

        QGraphicsLinearLayout *li2 = new QGraphicsLinearLayout;
        CustomGraphicsWidget *li2_1 = new CustomGraphicsWidget;
        li2->addItem(li2_1);
        CustomGraphicsWidget *li2_2 = new CustomGraphicsWidget;
        li2->addItem(li2_2);
        CustomGraphicsWidget *li2_3 = new CustomGraphicsWidget;
        li2->addItem(li2_3);
        lay->addItem(li2);

        CustomGraphicsWidget *li3 = new CustomGraphicsWidget;
        lay->addItem(li3);

        window->setLayout(lay);
        scene.addItem(window);
        view.resize(500, 200);
        view.show();

        for (int i = li2->count(); i > 0; --i) {
            QCOMPARE(li2->count(), i);
            delete li2->itemAt(0);
        }

        for (int i = lay->count(); i > 0; --i) {
            QCOMPARE(lay->count(), i);
            delete lay->itemAt(0);
        }

        delete window;
    }

    {
        QGraphicsWidget *top = new QGraphicsWidget;
        QGraphicsWidget *w = new QGraphicsWidget;
        QGraphicsWidget *w2 = new QGraphicsWidget;
        CustomLayout *layout = new CustomLayout(top);
        layout->addItem(w);
        layout->addItem(w2);
        top->setLayout(layout);
        delete top;
        //don't crash after that.
    }
}

QTEST_MAIN(tst_QGraphicsLayout)
#include "tst_qgraphicslayout.moc"
