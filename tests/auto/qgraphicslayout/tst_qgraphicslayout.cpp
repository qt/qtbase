/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtGui>
#include <math.h>

#include "../../shared/util.h"

//TESTED_CLASS=QGraphicsLayout
//TESTED_FILES=

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
        m_count = 0;
    }

    void setGeometry(const QRectF &rect) {

        ++m_count;
        QGraphicsLinearLayout::setGeometry(rect);
    }


    int m_count;
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

    QCOMPARE(lout->m_count, 0);
    window->setVisible(false);
    QCOMPARE(lout->m_count, 0);
    window->setVisible(true);
    // on polish or the first time a widget is shown, the widget is resized.
    QCOMPARE(lout->m_count, 1);

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

QSizeF AnimatedLayoutItem::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
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

QSizeF CustomLayoutItem::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
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

QSizeF CustomGraphicsWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
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

QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
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
        QVERIFY(destructedSet.count() == 0);
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
