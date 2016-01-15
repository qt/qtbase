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
#include <QtWidgets/qgraphicsscene.h>
#include <private/qgraphicsscenebsptreeindex_p.h>
#include <private/qgraphicssceneindex_p.h>
#include <private/qgraphicsscenelinearindex_p.h>

class tst_QGraphicsSceneIndex : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();

private slots:
    void scatteredItems_data();
    void scatteredItems();
    void overlappedItems_data();
    void overlappedItems();
    void movingItems_data();
    void movingItems();
    void connectedToSceneRectChanged();
    void items();
    void boundingRectPointIntersection_data();
    void boundingRectPointIntersection();
    void removeItems();
    void clear();

private:
    void common_data();
    QGraphicsSceneIndex *createIndex(const QString &name);
};

void tst_QGraphicsSceneIndex::initTestCase()
{
}

void tst_QGraphicsSceneIndex::common_data()
{
    QTest::addColumn<QString>("indexMethod");

    QTest::newRow("BSP") << QString("bsp");
    QTest::newRow("Linear") << QString("linear");
}

QGraphicsSceneIndex *tst_QGraphicsSceneIndex::createIndex(const QString &indexMethod)
{
    QGraphicsSceneIndex *index = 0;
    QGraphicsScene *scene = new QGraphicsScene();
    if (indexMethod == "bsp")
        index = new QGraphicsSceneBspTreeIndex(scene);

    if (indexMethod == "linear")
        index = new QGraphicsSceneLinearIndex(scene);

    return index;
}

void tst_QGraphicsSceneIndex::scatteredItems_data()
{
    common_data();
}

void tst_QGraphicsSceneIndex::scatteredItems()
{
    QFETCH(QString, indexMethod);

    QGraphicsScene scene;
    scene.setItemIndexMethod(indexMethod == "linear" ? QGraphicsScene::NoIndex : QGraphicsScene::BspTreeIndex);

    for (int i = 0; i < 10; ++i)
        scene.addRect(i*50, i*50, 40, 35);

    QCOMPARE(scene.items(QPointF(5, 5)).count(), 1);
    QCOMPARE(scene.items(QPointF(55, 55)).count(), 1);
    QCOMPARE(scene.items(QPointF(-100, -100)).count(), 0);

    QCOMPARE(scene.items(QRectF(0, 0, 10, 10)).count(), 1);
    QCOMPARE(scene.items(QRectF(0, 0, 1000, 1000)).count(), 10);
    QCOMPARE(scene.items(QRectF(-100, -1000, 0, 0)).count(), 0);
}

void tst_QGraphicsSceneIndex::overlappedItems_data()
{
    common_data();
}

void tst_QGraphicsSceneIndex::overlappedItems()
{
    QFETCH(QString, indexMethod);

    QGraphicsScene scene;
    scene.setItemIndexMethod(indexMethod == "linear" ? QGraphicsScene::NoIndex : QGraphicsScene::BspTreeIndex);

    for (int i = 0; i < 10; ++i)
        for (int j = 0; j < 10; ++j)
            scene.addRect(i*50, j*50, 200, 200)->setPen(QPen(Qt::black, 0));

    QCOMPARE(scene.items(QPointF(5, 5)).count(), 1);
    QCOMPARE(scene.items(QPointF(55, 55)).count(), 4);
    QCOMPARE(scene.items(QPointF(105, 105)).count(), 9);
    QCOMPARE(scene.items(QPointF(-100, -100)).count(), 0);

    QCOMPARE(scene.items(QRectF(0, 0, 1000, 1000)).count(), 100);
    QCOMPARE(scene.items(QRectF(-100, -1000, 0, 0)).count(), 0);
    QCOMPARE(scene.items(QRectF(0, 0, 200, 200)).count(), 16);
    QCOMPARE(scene.items(QRectF(0, 0, 100, 100)).count(), 4);
    QCOMPARE(scene.items(QRectF(0, 0, 1, 100)).count(), 2);
    QCOMPARE(scene.items(QRectF(0, 0, 1, 1000)).count(), 10);
}

void tst_QGraphicsSceneIndex::movingItems_data()
{
    common_data();
}

void tst_QGraphicsSceneIndex::movingItems()
{
    QFETCH(QString, indexMethod);

    QGraphicsScene scene;
    scene.setItemIndexMethod(indexMethod == "linear" ? QGraphicsScene::NoIndex : QGraphicsScene::BspTreeIndex);

    for (int i = 0; i < 10; ++i)
        scene.addRect(i*50, i*50, 40, 35);

    QGraphicsRectItem *box = scene.addRect(0, 0, 10, 10);
    QCOMPARE(scene.items(QPointF(5, 5)).count(), 2);
    QCOMPARE(scene.items(QPointF(-1, -1)).count(), 0);
    QCOMPARE(scene.items(QRectF(0, 0, 5, 5)).count(), 2);

    box->setPos(10, 10);
    QCOMPARE(scene.items(QPointF(9, 9)).count(), 1);
    QCOMPARE(scene.items(QPointF(15, 15)).count(), 2);
    QCOMPARE(scene.items(QRectF(0, 0, 1, 1)).count(), 1);

    box->setPos(-5, -5);
    QCOMPARE(scene.items(QPointF(-1, -1)).count(), 1);
    QCOMPARE(scene.items(QRectF(0, 0, 1, 1)).count(), 2);

    QCOMPARE(scene.items(QRectF(0, 0, 1000, 1000)).count(), 11);
}

void tst_QGraphicsSceneIndex::connectedToSceneRectChanged()
{

    class MyScene : public QGraphicsScene
    {
    public:
        using QGraphicsScene::receivers;
    };

    MyScene scene; // Uses QGraphicsSceneBspTreeIndex by default.
    QCOMPARE(scene.receivers(SIGNAL(sceneRectChanged(QRectF))), 1);

    scene.setItemIndexMethod(QGraphicsScene::NoIndex); // QGraphicsSceneLinearIndex
    QCOMPARE(scene.receivers(SIGNAL(sceneRectChanged(QRectF))), 1);
}

void tst_QGraphicsSceneIndex::items()
{
    QGraphicsScene scene;
    QGraphicsItem *item1 = scene.addRect(0, 0, 10, 10);
    QGraphicsItem *item2 = scene.addRect(10, 10, 10, 10);
    QCOMPARE(scene.items().size(), 2);

    // Move from unindexed items into bsp tree.
    QTest::qWait(50);
    QCOMPARE(scene.items().size(), 2);

    // Add untransformable item.
    QGraphicsItem *item3 = new QGraphicsRectItem(QRectF(20, 20, 10, 10));
    item3->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene.addItem(item3);
    QCOMPARE(scene.items().size(), 3);

    // Move from unindexed items into untransformable items.
    QTest::qWait(50);
    QCOMPARE(scene.items().size(), 3);

    // Move from untransformable items into unindexed items.
    item3->setFlag(QGraphicsItem::ItemIgnoresTransformations, false);
    QCOMPARE(scene.items().size(), 3);
    QTest::qWait(50);
    QCOMPARE(scene.items().size(), 3);

    // Make all items untransformable.
    item1->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    item2->setParentItem(item1);
    item3->setParentItem(item2);
    QCOMPARE(scene.items().size(), 3);

    // Move from unindexed items into untransformable items.
    QTest::qWait(50);
    QCOMPARE(scene.items().size(), 3);
}

class CustomShapeItem : public QGraphicsItem
{
public:
    CustomShapeItem(const QPainterPath &shape) : QGraphicsItem(0), mShape(shape) {}

    QPainterPath shape() const { return mShape; }
    QRectF boundingRect() const { return mShape.boundingRect(); }
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) {}
private:
    QPainterPath mShape;
};

Q_DECLARE_METATYPE(Qt::ItemSelectionMode)
Q_DECLARE_METATYPE(QPainterPath)

void tst_QGraphicsSceneIndex::boundingRectPointIntersection_data()
{
    QTest::addColumn<QPainterPath>("itemShape");
    QTest::addColumn<Qt::ItemSelectionMode>("mode");

    QTest::newRow("zero shape - intersects rect") << QPainterPath() << Qt::IntersectsItemBoundingRect;
    QTest::newRow("zero shape - contains rect") << QPainterPath() << Qt::ContainsItemBoundingRect;

    QPainterPath triangle;
    triangle.moveTo(50, 0);
    triangle.lineTo(0, 50);
    triangle.lineTo(100, 50);
    triangle.lineTo(50, 0);
    QTest::newRow("triangle shape - intersects rect") << triangle << Qt::IntersectsItemBoundingRect;
    QTest::newRow("triangle shape - contains rect") << triangle << Qt::ContainsItemBoundingRect;

    QPainterPath rect;
    rect.addRect(QRectF(0, 0, 100, 100));
    QTest::newRow("rectangle shape - intersects rect") << rect << Qt::IntersectsItemBoundingRect;
    QTest::newRow("rectangle shape - contains rect") << rect << Qt::ContainsItemBoundingRect;
}

void tst_QGraphicsSceneIndex::boundingRectPointIntersection()
{
    QFETCH(QPainterPath, itemShape);
    QFETCH(Qt::ItemSelectionMode, mode);

    QGraphicsScene scene;
    CustomShapeItem *item = new CustomShapeItem(itemShape);
    scene.addItem(item);
    QList<QGraphicsItem*> items = scene.items(QPointF(0, 0), mode, Qt::AscendingOrder);
    QVERIFY(!items.isEmpty());
    QCOMPARE(items.first(), item);
}

class RectWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    RectWidget(QGraphicsItem *parent = 0) : QGraphicsWidget(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem * /* option */, QWidget * /* widget */)
    {
        painter->setBrush(brush);
        painter->drawRect(boundingRect());
    }
public:
    QBrush brush;
};

void tst_QGraphicsSceneIndex::removeItems()
{
     QGraphicsScene scene;

    RectWidget *parent = new RectWidget;
    parent->brush = QBrush(QColor(Qt::magenta));
    parent->setGeometry(250, 250, 400, 400);

    RectWidget *widget = new RectWidget(parent);
    widget->brush = QBrush(QColor(Qt::blue));
    widget->setGeometry(10, 10, 200, 200);

    RectWidget *widgetChild1 = new RectWidget(widget);
    widgetChild1->brush = QBrush(QColor(Qt::green));
    widgetChild1->setGeometry(20, 20, 100, 100);

    RectWidget *widgetChild2 = new RectWidget(widgetChild1);
    widgetChild2->brush = QBrush(QColor(Qt::yellow));
    widgetChild2->setGeometry(25, 25, 50, 50);

    scene.addItem(parent);

    QGraphicsView view(&scene);
    view.resize(600, 600);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    scene.removeItem(widgetChild1);

    delete widgetChild1;

    //We move the parent
    scene.items(QRectF(295, 295, 50, 50));

    //This should not crash
}

void tst_QGraphicsSceneIndex::clear()
{
    class MyItem : public QGraphicsItem
    {
    public:
        MyItem(QGraphicsItem *parent = 0) : QGraphicsItem(parent), numPaints(0) {}
        int numPaints;
    protected:
        QRectF boundingRect() const { return QRectF(0, 0, 10, 10); }
        void paint(QPainter * /* painter */, const QStyleOptionGraphicsItem *, QWidget *)
        { ++numPaints; }
    };

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 100, 100);
    scene.addItem(new MyItem);

    QGraphicsView view(&scene);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    scene.clear();

    // Make sure the index is re-generated after QGraphicsScene::clear();
    // otherwise no items will be painted.
    MyItem *item = new MyItem;
    scene.addItem(item);
    qApp->processEvents();
    QTRY_COMPARE(item->numPaints, 1);
}

QTEST_MAIN(tst_QGraphicsSceneIndex)
#include "tst_qgraphicssceneindex.moc"
