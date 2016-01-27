/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <private/qgraphicsitem_p.h>
#include <private/qgraphicsview_p.h>
#include <private/qgraphicsscene_p.h>
#include <QStyleOptionGraphicsItem>
#include <QAbstractTextDocumentLayout>
#include <QBitmap>
#include <QCursor>
#include <QScreen>
#include <QLabel>
#include <QDial>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QGraphicsEffect>
#include <QPushButton>
#include <QLineEdit>
#include <QGraphicsLinearLayout>
#include <float.h>
#include <QStyleHints>

Q_DECLARE_METATYPE(QPainterPath)

#include "../../../qtest-config.h"

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
#include <windows.h>
#define Q_CHECK_PAINTEVENTS \
    if (::SwitchDesktop(::GetThreadDesktop(::GetCurrentThreadId())) == 0) \
        QSKIP("The Graphics View doesn't get the paint events");
#else
#define Q_CHECK_PAINTEVENTS
#endif

#if defined(Q_OS_MAC)
// On mac (cocoa) we always get full update.
// So check that the expected region is contained inside the actual
#define COMPARE_REGIONS(ACTUAL, EXPECTED) QVERIFY((EXPECTED).subtracted(ACTUAL).isEmpty())
#else
#define COMPARE_REGIONS QTRY_COMPARE
#endif

static QGraphicsRectItem staticItem; //QTBUG-7629, we should not crash at exit.

static void sendMousePress(QGraphicsScene *scene, const QPointF &point, Qt::MouseButton button = Qt::LeftButton)
{
    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
    event.setScenePos(point);
    event.setButton(button);
    event.setButtons(button);
    QApplication::sendEvent(scene, &event);
}

static void sendMouseMove(QGraphicsScene *scene, const QPointF &point,
                          Qt::MouseButton button = Qt::NoButton, Qt::MouseButtons /* buttons */ = 0)
{
    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseMove);
    event.setScenePos(point);
    event.setButton(button);
    event.setButtons(button);
    QApplication::sendEvent(scene, &event);
}

static void sendMouseRelease(QGraphicsScene *scene, const QPointF &point, Qt::MouseButton button = Qt::LeftButton)
{
    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseRelease);
    event.setScenePos(point);
    event.setButton(button);
    QApplication::sendEvent(scene, &event);
}

static void sendMouseClick(QGraphicsScene *scene, const QPointF &point, Qt::MouseButton button = Qt::LeftButton)
{
    sendMousePress(scene, point, button);
    sendMouseRelease(scene, point, button);
}

static void sendKeyPress(QGraphicsScene *scene, Qt::Key key)
{
    QKeyEvent keyEvent(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::sendEvent(scene, &keyEvent);
}

static void sendKeyRelease(QGraphicsScene *scene, Qt::Key key)
{
    QKeyEvent keyEvent(QEvent::KeyRelease, key, Qt::NoModifier);
    QApplication::sendEvent(scene, &keyEvent);
}

static void sendKeyClick(QGraphicsScene *scene, Qt::Key key)
{
    sendKeyPress(scene, key);
    sendKeyRelease(scene, key);
}

static inline void centerOnScreen(QWidget *w, const QSize &size)
{
    const QPoint offset = QPoint(size.width() / 2, size.height() / 2);
    w->move(QGuiApplication::primaryScreen()->availableGeometry().center() - offset);
}

static inline void centerOnScreen(QWidget *w)
{
    centerOnScreen(w, w->geometry().size());
}

class EventSpy : public QGraphicsWidget
{
    Q_OBJECT
public:
    EventSpy(QObject *watched, QEvent::Type type)
        : _count(0), spied(type)
    {
        watched->installEventFilter(this);
    }

    EventSpy(QGraphicsScene *scene, QGraphicsItem *watched, QEvent::Type type)
        : _count(0), spied(type)
    {
        scene->addItem(this);
        watched->installSceneEventFilter(this);
    }

    int count() const { return _count; }

protected:
    bool eventFilter(QObject *watched, QEvent *event)
    {
        Q_UNUSED(watched);
        if (event->type() == spied)
            ++_count;
        return false;
    }

    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event)
    {
        Q_UNUSED(watched);
        if (event->type() == spied)
            ++_count;
        return false;
    }

    int _count;
    QEvent::Type spied;
};

class EventSpy2 : public QGraphicsWidget
{
    Q_OBJECT
public:
    EventSpy2(QObject *watched)
    {
        watched->installEventFilter(this);
    }

    EventSpy2(QGraphicsScene *scene, QGraphicsItem *watched)
    {
        scene->addItem(this);
        watched->installSceneEventFilter(this);
    }

    QMap<QEvent::Type, int> counts;

protected:
    bool eventFilter(QObject *watched, QEvent *event)
    {
        Q_UNUSED(watched);
        ++counts[event->type()];
        return false;
    }

    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event)
    {
        Q_UNUSED(watched);
        ++counts[event->type()];
        return false;
    }
};

class EventTester : public QGraphicsItem
{
public:
    EventTester(QGraphicsItem *parent = 0) : QGraphicsItem(parent), repaints(0)
    { br = QRectF(-10, -10, 20, 20); }

    void setGeometry(const QRectF &rect)
    {
        prepareGeometryChange();
        br = rect;
        update();
    }

    QRectF boundingRect() const
    { return br; }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *)
    {
        hints = painter->renderHints();
        painter->setBrush(brush);
        painter->drawRect(boundingRect());
        lastExposedRect = o->exposedRect;
        ++repaints;
    }

    bool sceneEvent(QEvent *event)
    {
        events << event->type();
        return QGraphicsItem::sceneEvent(event);
    }

    void reset()
    {
        events.clear();
        hints = QPainter::RenderHints(0);
        repaints = 0;
        lastExposedRect = QRectF();
    }

    QList<QEvent::Type> events;
    QPainter::RenderHints hints;
    int repaints;
    QRectF br;
    QRectF lastExposedRect;
    QBrush brush;
};

class MyGraphicsView : public QGraphicsView
{
public:
    int repaints;
    QRegion paintedRegion;
    MyGraphicsView(QGraphicsScene *scene, QWidget *parent=0) : QGraphicsView(scene,parent), repaints(0) {}
    void paintEvent(QPaintEvent *e)
    {
        paintedRegion += e->region();
        ++repaints;
        QGraphicsView::paintEvent(e);
    }
    void reset() { repaints = 0; paintedRegion = QRegion(); }
};

class tst_QGraphicsItem : public QObject
{
    Q_OBJECT

public slots:
    void init();

private slots:
    void construction();
    void constructionWithParent();
    void destruction();
    void deleteChildItem();
    void scene();
    void parentItem();
    void setParentItem();
    void children();
    void flags();
    void inputMethodHints();
    void toolTip();
    void visible();
    void isVisibleTo();
    void explicitlyVisible();
    void enabled();
    void explicitlyEnabled();
    void selected();
    void selected2();
    void selected_group();
    void selected_textItem();
    void selected_multi();
    void acceptedMouseButtons();
    void acceptsHoverEvents();
    void childAcceptsHoverEvents();
    void hasFocus();
    void pos();
    void scenePos();
    void matrix();
    void sceneMatrix();
    void setMatrix();
    void zValue();
    void shape();
    void contains();
    void collidesWith_item();
    void collidesWith_path_data();
    void collidesWith_path();
    void collidesWithItemWithClip();
    void isObscuredBy();
    void isObscured();
    void mapFromToParent();
    void mapFromToScene();
    void mapFromToItem();
    void mapRectFromToParent_data();
    void mapRectFromToParent();
    void isAncestorOf();
    void commonAncestorItem();
    void data();
    void type();
    void graphicsitem_cast();
    void hoverEventsGenerateRepaints();
    void boundingRects_data();
    void boundingRects();
    void boundingRects2();
    void sceneBoundingRect();
    void childrenBoundingRect();
    void childrenBoundingRectTransformed();
    void childrenBoundingRect2();
    void childrenBoundingRect3();
    void childrenBoundingRect4();
    void childrenBoundingRect5();
    void group();
    void setGroup();
    void setGroup2();
    void nestedGroups();
    void warpChildrenIntoGroup();
    void removeFromGroup();
    void handlesChildEvents();
    void handlesChildEvents2();
    void handlesChildEvents3();
    void filtersChildEvents();
    void filtersChildEvents2();
    void ensureVisible();
#ifndef QTEST_NO_CURSOR
    void cursor();
#endif
    //void textControlGetterSetter();
    void defaultItemTest_QGraphicsLineItem();
    void defaultItemTest_QGraphicsPixmapItem();
    void defaultItemTest_QGraphicsTextItem();
    void defaultItemTest_QGraphicsEllipseItem();
    void itemChange();
    void sceneEventFilter();
    void prepareGeometryChange();
    void paint();
    void deleteItemInEventHandlers();
    void itemClipsToShape();
    void itemClipsChildrenToShape();
    void itemClipsChildrenToShape2();
    void itemClipsChildrenToShape3();
    void itemClipsChildrenToShape4();
    void itemClipsChildrenToShape5();
    void itemClipsTextChildToShape();
    void itemClippingDiscovery();
    void itemContainsChildrenInShape();
    void itemContainsChildrenInShape2();
    void ancestorFlags();
    void untransformable();
    void contextMenuEventPropagation();
    void itemIsMovable();
    void boundingRegion_data();
    void boundingRegion();
    void itemTransform_parentChild();
    void itemTransform_siblings();
    void itemTransform_unrelated();
    void opacity_data();
    void opacity();
    void opacity2();
    void opacityZeroUpdates();
    void itemStacksBehindParent();
    void nestedClipping();
    void nestedClippingTransforms();
    void sceneTransformCache();
    void tabChangesFocus();
    void tabChangesFocus_data();
    void cacheMode();
    void cacheMode2();
    void updateCachedItemAfterMove();
    void deviceTransform_data();
    void deviceTransform();
    void update();
    void setTransformProperties_data();
    void setTransformProperties();
    void itemUsesExtendedStyleOption();
    void itemSendsGeometryChanges();
    void moveItem();
    void moveLineItem();
    void sorting_data();
    void sorting();
    void itemHasNoContents();
    void hitTestUntransformableItem();
    void hitTestGraphicsEffectItem();
    void focusProxy();
    void subFocus();
    void focusProxyDeletion();
    void negativeZStacksBehindParent();
    void setGraphicsEffect();
    void panel();
    void addPanelToActiveScene();
    void panelWithFocusItems();
    void activate();
    void setActivePanelOnInactiveScene();
    void activationOnShowHide();
    void deactivateInactivePanel();
    void moveWhileDeleting();
    void ensureDirtySceneTransform();
    void focusScope();
    void focusScope2();
    void focusScopeItemChangedWhileScopeDoesntHaveFocus();
    void stackBefore();
    void sceneModality();
    void panelModality();
    void mixedModality();
    void modality_hover();
    void modality_mouseGrabber();
    void modality_clickFocus();
    void modality_keyEvents();
    void itemIsInFront();
    void scenePosChange();
    void textItem_shortcuts();
    void scroll();
    void focusHandling_data();
    void focusHandling();
    void touchEventPropagation_data();
    void touchEventPropagation();
    void deviceCoordinateCache_simpleRotations();
    void resolvePaletteForItemChildren();

    // task specific tests below me
    void task141694_textItemEnsureVisible();
    void task128696_textItemEnsureMovable();
    void ensureUpdateOnTextItem();
    void task177918_lineItemUndetected();
    void task240400_clickOnTextItem_data();
    void task240400_clickOnTextItem();
    void task243707_addChildBeforeParent();
    void task197802_childrenVisibility();
    void QTBUG_4233_updateCachedWithSceneRect();
    void QTBUG_5418_textItemSetDefaultColor();
    void QTBUG_6738_missingUpdateWithSetParent();
    void QTBUG_7714_fullUpdateDiscardingOpacityUpdate2();
    void QT_2653_fullUpdateDiscardingOpacityUpdate();
    void QT_2649_focusScope();
    void sortItemsWhileAdding();
    void doNotMarkFullUpdateIfNotInScene();
    void itemDiesDuringDraggingOperation();
    void QTBUG_12112_focusItem();
    void QTBUG_13473_sceneposchange();
    void QTBUG_16374_crashInDestructor();
    void QTBUG_20699_focusScopeCrash();
    void QTBUG_30990_rightClickSelection();
    void QTBUG_21618_untransformable_sceneTransform();

private:
    QList<QGraphicsItem *> paintedItems;
};

void tst_QGraphicsItem::init()
{
#ifdef Q_OS_WINCE //disable magic for WindowsCE
    qApp->setAutoMaximizeThreshold(-1);
#endif
}

void tst_QGraphicsItem::construction()
{
    for (int i = 0; i < 7; ++i) {
        QGraphicsItem *item;
        switch (i) {
        case 0:
            item = new QGraphicsEllipseItem;
            ((QGraphicsEllipseItem *)item)->setPen(QPen(Qt::black, 0));
            QCOMPARE(int(item->type()), int(QGraphicsEllipseItem::Type));
            QCOMPARE(qgraphicsitem_cast<QGraphicsEllipseItem *>(item), (QGraphicsEllipseItem *)item);
            QCOMPARE(qgraphicsitem_cast<QGraphicsRectItem *>(item), (QGraphicsRectItem *)0);
            QCOMPARE(item->flags(), 0);
            break;
        case 1:
            item = new QGraphicsLineItem;
            ((QGraphicsLineItem *)item)->setPen(QPen(Qt::black, 0));
            QCOMPARE(int(item->type()), int(QGraphicsLineItem::Type));
            QCOMPARE(qgraphicsitem_cast<QGraphicsLineItem *>(item), (QGraphicsLineItem *)item);
            QCOMPARE(qgraphicsitem_cast<QGraphicsRectItem *>(item), (QGraphicsRectItem *)0);
            QCOMPARE(item->flags(), 0);
            break;
        case 2:
            item = new QGraphicsPathItem;
            ((QGraphicsPathItem *)item)->setPen(QPen(Qt::black, 0));
            QCOMPARE(int(item->type()), int(QGraphicsPathItem::Type));
            QCOMPARE(qgraphicsitem_cast<QGraphicsPathItem *>(item), (QGraphicsPathItem *)item);
            QCOMPARE(qgraphicsitem_cast<QGraphicsRectItem *>(item), (QGraphicsRectItem *)0);
            QCOMPARE(item->flags(), 0);
            break;
        case 3:
            item = new QGraphicsPixmapItem;
            QCOMPARE(int(item->type()), int(QGraphicsPixmapItem::Type));
            QCOMPARE(qgraphicsitem_cast<QGraphicsPixmapItem *>(item), (QGraphicsPixmapItem *)item);
            QCOMPARE(qgraphicsitem_cast<QGraphicsRectItem *>(item), (QGraphicsRectItem *)0);
            QCOMPARE(item->flags(), 0);
            break;
        case 4:
            item = new QGraphicsPolygonItem;
            ((QGraphicsPolygonItem *)item)->setPen(QPen(Qt::black, 0));
            QCOMPARE(int(item->type()), int(QGraphicsPolygonItem::Type));
            QCOMPARE(qgraphicsitem_cast<QGraphicsPolygonItem *>(item), (QGraphicsPolygonItem *)item);
            QCOMPARE(qgraphicsitem_cast<QGraphicsRectItem *>(item), (QGraphicsRectItem *)0);
            QCOMPARE(item->flags(), 0);
            break;
        case 5:
            item = new QGraphicsRectItem;
            ((QGraphicsRectItem *)item)->setPen(QPen(Qt::black, 0));
            QCOMPARE(int(item->type()), int(QGraphicsRectItem::Type));
            QCOMPARE(qgraphicsitem_cast<QGraphicsRectItem *>(item), (QGraphicsRectItem *)item);
            QCOMPARE(qgraphicsitem_cast<QGraphicsLineItem *>(item), (QGraphicsLineItem *)0);
            QCOMPARE(item->flags(), 0);
            break;
        case 6:
            item = new QGraphicsTextItem;
            QCOMPARE(int(item->type()), int(QGraphicsTextItem::Type));
            QCOMPARE(qgraphicsitem_cast<QGraphicsTextItem *>(item), (QGraphicsTextItem *)item);
            QCOMPARE(qgraphicsitem_cast<QGraphicsRectItem *>(item), (QGraphicsRectItem *)0);
            // This is the only item that uses an extended style option.
            QCOMPARE(item->flags(), QGraphicsItem::GraphicsItemFlags(QGraphicsItem::ItemUsesExtendedStyleOption));
            break;
        default:
            qFatal("You broke the logic, please fix!");
            break;
        }

        QCOMPARE(item->scene(), (QGraphicsScene *)0);
        QCOMPARE(item->parentItem(), (QGraphicsItem *)0);
        QVERIFY(item->childItems().isEmpty());
        QVERIFY(item->isVisible());
        QVERIFY(item->isEnabled());
        QVERIFY(!item->isSelected());
        QCOMPARE(item->acceptedMouseButtons(), Qt::MouseButtons(0x1f));
        if (item->type() == QGraphicsTextItem::Type)
            QVERIFY(item->acceptsHoverEvents());
        else
            QVERIFY(!item->acceptsHoverEvents());
        QVERIFY(!item->hasFocus());
        QCOMPARE(item->pos(), QPointF());
        QCOMPARE(item->matrix(), QMatrix());
        QCOMPARE(item->sceneMatrix(), QMatrix());
        QCOMPARE(item->zValue(), qreal(0));
        QCOMPARE(item->sceneBoundingRect(), QRectF());
        QCOMPARE(item->shape(), QPainterPath());
        QVERIFY(!item->contains(QPointF(0, 0)));
        QVERIFY(!item->collidesWithItem(0));
        QVERIFY(item->collidesWithItem(item));
        QVERIFY(!item->collidesWithPath(QPainterPath()));
        QVERIFY(!item->isAncestorOf(0));
        QVERIFY(!item->isAncestorOf(item));
        QCOMPARE(item->data(0), QVariant());
        delete item;
    }
}

class BoundingRectItem : public QGraphicsRectItem
{
public:
    BoundingRectItem(QGraphicsItem *parent = 0)
        : QGraphicsRectItem(0, 0, parent ? 200 : 100, parent ? 200 : 100,
                            parent)
    {
        setPen(QPen(Qt::black, 0));
    }

    QRectF boundingRect() const
    {
        QRectF tmp = QGraphicsRectItem::boundingRect();
        foreach (QGraphicsItem *child, childItems())
            tmp |= child->boundingRect(); // <- might be pure virtual
        return tmp;
    }
};

void tst_QGraphicsItem::constructionWithParent()
{
    // This test causes a crash if item1 calls item2's pure virtuals before the
    // object has been constructed.
    QGraphicsItem *item0 = new BoundingRectItem;
    QGraphicsItem *item1 = new BoundingRectItem;
    QGraphicsScene scene;
    scene.addItem(item0);
    scene.addItem(item1);
    QGraphicsItem *item2 = new BoundingRectItem(item1);
    QCOMPARE(item1->childItems(), QList<QGraphicsItem *>() << item2);
    QCOMPARE(item1->boundingRect(), QRectF(0, 0, 200, 200));

    item2->setParentItem(item0);
    QCOMPARE(item0->childItems(), QList<QGraphicsItem *>() << item2);
    QCOMPARE(item0->boundingRect(), QRectF(0, 0, 200, 200));
}

static int itemDeleted = 0;
class Item : public QGraphicsRectItem
{
public:
    ~Item()
    { ++itemDeleted; }
};

void tst_QGraphicsItem::destruction()
{
    QCOMPARE(itemDeleted, 0);
    {
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        QCOMPARE(child->parentItem(), parent);
        delete parent;
        QCOMPARE(itemDeleted, 1);
    }
    {
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        QCOMPARE(parent->childItems().size(), 1);
        delete child;
        QCOMPARE(parent->childItems().size(), 0);
        delete parent;
        QCOMPARE(itemDeleted, 2);
    }
    {
        QGraphicsScene scene;
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        QCOMPARE(child->parentItem(), (QGraphicsItem *)0);
        child->setParentItem(parent);
        QCOMPARE(child->parentItem(), parent);
        scene.addItem(parent);
        QCOMPARE(child->parentItem(), parent);
        delete parent;
        QCOMPARE(itemDeleted, 3);
    }
    {
        QGraphicsScene scene;
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        scene.addItem(parent);
        QCOMPARE(child->scene(), &scene);
        QCOMPARE(parent->childItems().size(), 1);
        delete child;
        QCOMPARE(parent->childItems().size(), 0);
        delete parent;
        QCOMPARE(itemDeleted, 4);
    }
    {
        QGraphicsScene scene;
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        scene.addItem(parent);
        QCOMPARE(child->scene(), &scene);
        scene.removeItem(parent);
        QCOMPARE(child->scene(), (QGraphicsScene *)0);
        delete parent;
        QCOMPARE(itemDeleted, 5);
    }
    {
        QGraphicsScene scene;
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        QCOMPARE(child->scene(), (QGraphicsScene *)0);
        QCOMPARE(parent->scene(), (QGraphicsScene *)0);
        scene.addItem(parent);
        QCOMPARE(child->scene(), &scene);
        scene.removeItem(child);
        QCOMPARE(child->scene(), (QGraphicsScene *)0);
        QCOMPARE(parent->scene(), &scene);
        QCOMPARE(child->parentItem(), (QGraphicsItem *)0);
        QVERIFY(parent->childItems().isEmpty());
        delete parent;
        QCOMPARE(itemDeleted, 5);
        delete child;
        QCOMPARE(itemDeleted, 6);
    }
    {
        QGraphicsScene scene;
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        scene.addItem(parent);
        scene.removeItem(child);
        scene.removeItem(parent);
        delete child;
        delete parent;
        QCOMPARE(itemDeleted, 7);
    }
    {
        QGraphicsScene scene;
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        scene.addItem(parent);
        QGraphicsScene scene2;
        scene2.addItem(parent);
        delete parent;
        QCOMPARE(itemDeleted, 8);
    }
    {
        QGraphicsScene scene;
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        scene.addItem(parent);
        QCOMPARE(child->scene(), &scene);
        QGraphicsScene scene2;
        scene2.addItem(parent);
        QCOMPARE(child->scene(), &scene2);
        scene.addItem(parent);
        QCOMPARE(child->scene(), &scene);
        scene2.addItem(parent);
        QCOMPARE(child->scene(), &scene2);
        delete parent;
        QCOMPARE(itemDeleted, 9);
    }
    {
        QGraphicsScene scene;
        QGraphicsItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        scene.addItem(parent);
        QCOMPARE(child->scene(), &scene);
        QGraphicsScene scene2;
        scene2.addItem(child);
        QCOMPARE(child->scene(), &scene2);
        delete parent;
        QCOMPARE(itemDeleted, 9);
        delete child;
        QCOMPARE(itemDeleted, 10);
    }
    {
        QGraphicsScene scene;
        QGraphicsItem *root = new QGraphicsRectItem;
        QGraphicsItem *parent = root;
        QGraphicsItem *middleItem = 0;
        for (int i = 0; i < 99; ++i) {
            Item *child = new Item;
            child->setParentItem(parent);
            parent = child;
            if (i == 50)
                middleItem = parent;
        }
        scene.addItem(root);

        QCOMPARE(scene.items().size(), 100);

        QGraphicsScene scene2;
        scene2.addItem(middleItem);

        delete middleItem;
        QCOMPARE(itemDeleted, 59);
    }
    QCOMPARE(itemDeleted, 109);
    {
        QGraphicsScene *scene = new QGraphicsScene;
        QGraphicsRectItem *parent = new QGraphicsRectItem;
        Item *child = new Item;
        child->setParentItem(parent);
        parent->setVisible(false);
        scene->addItem(parent);
        QCOMPARE(child->parentItem(), static_cast<QGraphicsItem*>(parent));
        delete scene;
        QCOMPARE(itemDeleted, 110);
    }
}

void tst_QGraphicsItem::deleteChildItem()
{
    QGraphicsScene scene;
    QGraphicsItem *rect = scene.addRect(QRectF());
    QGraphicsItem *child1 = new QGraphicsRectItem(rect);
    QGraphicsItem *child2 = new QGraphicsRectItem(rect);
    QGraphicsItem *child3 = new QGraphicsRectItem(rect);
    Q_UNUSED(child3);
    delete child1;
    child2->setParentItem(0);
    delete child2;
}

void tst_QGraphicsItem::scene()
{
    QGraphicsRectItem *item = new QGraphicsRectItem;
    QCOMPARE(item->scene(), (QGraphicsScene *)0);

    QGraphicsScene scene;
    scene.addItem(item);
    QCOMPARE(item->scene(), (QGraphicsScene *)&scene);

    QGraphicsScene scene2;
    scene2.addItem(item);
    QCOMPARE(item->scene(), (QGraphicsScene *)&scene2);

    scene2.removeItem(item);
    QCOMPARE(item->scene(), (QGraphicsScene *)0);

    delete item;
}

void tst_QGraphicsItem::parentItem()
{
    QGraphicsRectItem item;
    QCOMPARE(item.parentItem(), (QGraphicsItem *)0);

    QGraphicsRectItem *item2 = new QGraphicsRectItem(QRectF(), &item);
    QCOMPARE(item2->parentItem(), (QGraphicsItem *)&item);
    item2->setParentItem(&item);
    QCOMPARE(item2->parentItem(), (QGraphicsItem *)&item);
    item2->setParentItem(0);
    QCOMPARE(item2->parentItem(), (QGraphicsItem *)0);

    delete item2;
}

void tst_QGraphicsItem::setParentItem()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(0, 0, 10, 10));
    QCOMPARE(item->scene(), &scene);

    QGraphicsRectItem *child = new QGraphicsRectItem;
    QCOMPARE(child->scene(), (QGraphicsScene *)0);

    // This implicitly adds the item to the parent's scene
    child->setParentItem(item);
    QCOMPARE(child->scene(), &scene);

    // This just makes it a toplevel
    child->setParentItem(0);
    QCOMPARE(child->scene(), &scene);

    // Add the child back to the parent, then remove the parent from the scene
    child->setParentItem(item);
    scene.removeItem(item);
    QCOMPARE(child->scene(), (QGraphicsScene *)0);
}

void tst_QGraphicsItem::children()
{
    QGraphicsRectItem item;
    QVERIFY(item.childItems().isEmpty());

    QGraphicsRectItem *item2 = new QGraphicsRectItem(QRectF(), &item);
    QCOMPARE(item.childItems().size(), 1);
    QCOMPARE(item.childItems().first(), (QGraphicsItem *)item2);
    QVERIFY(item2->childItems().isEmpty());

    delete item2;
    QVERIFY(item.childItems().isEmpty());
}

void tst_QGraphicsItem::flags()
{
    QGraphicsRectItem *item = new QGraphicsRectItem(QRectF(-10, -10, 20, 20));
    QCOMPARE(item->flags(), 0);

    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    scene.addItem(item);

    {
        // Focus
        item->setFlag(QGraphicsItem::ItemIsFocusable, false);
        QVERIFY(!item->hasFocus());
        item->setFocus();
        QVERIFY(!item->hasFocus());

        item->setFlag(QGraphicsItem::ItemIsFocusable, true);
        QVERIFY(!item->hasFocus());
        item->setFocus();
        QVERIFY(item->hasFocus());
        QVERIFY(scene.hasFocus());

        item->setFlag(QGraphicsItem::ItemIsFocusable, false);
        QVERIFY(!item->hasFocus());
        QVERIFY(scene.hasFocus());
    }
    {
        // Selectable
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        QVERIFY(!item->isSelected());
        item->setSelected(true);
        QVERIFY(!item->isSelected());

        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        QVERIFY(!item->isSelected());
        item->setSelected(true);
        QVERIFY(item->isSelected());
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        QVERIFY(!item->isSelected());
    }
    {
        // Movable
        item->setFlag(QGraphicsItem::ItemIsMovable, false);
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
        event.setScenePos(QPointF(0, 0));
        event.setButton(Qt::LeftButton);
        event.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &event);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)0); // mouse grabber is reset

        QGraphicsSceneMouseEvent event2(QEvent::GraphicsSceneMouseMove);
        event2.setScenePos(QPointF(10, 10));
        event2.setButton(Qt::LeftButton);
        event2.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &event2);
        QCOMPARE(item->pos(), QPointF());

        QGraphicsSceneMouseEvent event3(QEvent::GraphicsSceneMouseRelease);
        event3.setScenePos(QPointF(10, 10));
        event3.setButtons(0);
        QApplication::sendEvent(&scene, &event3);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)0);

        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        QGraphicsSceneMouseEvent event4(QEvent::GraphicsSceneMousePress);
        event4.setScenePos(QPointF(0, 0));
        event4.setButton(Qt::LeftButton);
        event4.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &event4);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)item);
        QGraphicsSceneMouseEvent event5(QEvent::GraphicsSceneMouseMove);
        event5.setScenePos(QPointF(10, 10));
        event5.setButton(Qt::LeftButton);
        event5.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &event5);
        QCOMPARE(item->pos(), QPointF(10, 10));
    }
    {
        QGraphicsItem* clippingParent = new QGraphicsRectItem;
        clippingParent->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

        QGraphicsItem* nonClippingParent = new QGraphicsRectItem;
        nonClippingParent->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);

        QGraphicsItem* child = new QGraphicsRectItem(nonClippingParent);
        QVERIFY(!child->isClipped());

        child->setParentItem(clippingParent);
        QVERIFY(child->isClipped());

        child->setParentItem(nonClippingParent);
        QVERIFY(!child->isClipped());
    }
}

class ImhTester : public QGraphicsItem
{
    QRectF boundingRect() const { return QRectF(); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}
};

void tst_QGraphicsItem::inputMethodHints()
{
    ImhTester *item = new ImhTester;
    item->setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);
    item->setFlag(QGraphicsItem::ItemIsFocusable, true);
    QCOMPARE(item->inputMethodHints(), Qt::ImhNone);
    ImhTester *item2 = new ImhTester;
    item2->setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);
    item2->setFlag(QGraphicsItem::ItemIsFocusable, true);
    Qt::InputMethodHints imHints = item2->inputMethodHints();
    imHints |= Qt::ImhHiddenText;
    item2->setInputMethodHints(imHints);
    QGraphicsScene scene;
    scene.addItem(item);
    scene.addItem(item2);
    QGraphicsView view(&scene);
    QApplication::setActiveWindow(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    item->setFocus();
    QTRY_VERIFY(item->hasFocus());
    QCOMPARE(view.inputMethodHints(), item->inputMethodHints());
    item2->setFocus();
    QTRY_VERIFY(item2->hasFocus());
    QCOMPARE(view.inputMethodHints(), item2->inputMethodHints());
    item->setFlag(QGraphicsItem::ItemAcceptsInputMethod, false);
    item->setFocus();
    QTRY_VERIFY(item->hasFocus());
    //Focus has changed but the new item doesn't accept input method, no hints.
    QCOMPARE(view.inputMethodHints(), 0);
    item2->setFocus();
    QTRY_VERIFY(item2->hasFocus());
    QCOMPARE(view.inputMethodHints(), item2->inputMethodHints());
    imHints = item2->inputMethodHints();
    imHints |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    item2->setInputMethodHints(imHints);
    QCOMPARE(view.inputMethodHints(), item2->inputMethodHints());
    QGraphicsProxyWidget *widget = new QGraphicsProxyWidget;
    QLineEdit *edit = new QLineEdit;
    edit->setEchoMode(QLineEdit::Password);
    scene.addItem(widget);
    widget->setFocus();
    QTRY_VERIFY(widget->hasFocus());
    //No widget on the proxy, so no hints
    QCOMPARE(view.inputMethodHints(), 0);
    widget->setWidget(edit);
    //View should match with the line edit
    QCOMPARE(view.inputMethodHints(), edit->inputMethodHints());
}

void tst_QGraphicsItem::toolTip()
{
    QString toolTip = "Qt rocks!";

    QGraphicsRectItem *item = new QGraphicsRectItem(QRectF(0, 0, 100, 100));
    item->setPen(QPen(Qt::red, 1));
    item->setBrush(QBrush(Qt::blue));
    QVERIFY(item->toolTip().isEmpty());
    item->setToolTip(toolTip);
    QCOMPARE(item->toolTip(), toolTip);

    QGraphicsScene scene;
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.setFixedSize(200, 200);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    {
        QHelpEvent helpEvent(QEvent::ToolTip, view.viewport()->rect().topLeft(),
                             view.viewport()->mapToGlobal(view.viewport()->rect().topLeft()));
        QApplication::sendEvent(view.viewport(), &helpEvent);
        QTest::qWait(250);

        bool foundView = false;
        bool foundTipLabel = false;
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
            if (widget == &view)
                foundView = true;
            if (widget->inherits("QTipLabel"))
                foundTipLabel = true;
        }
        QVERIFY(foundView);
        QVERIFY(!foundTipLabel);
    }

    {
        QHelpEvent helpEvent(QEvent::ToolTip, view.viewport()->rect().center(),
                             view.viewport()->mapToGlobal(view.viewport()->rect().center()));
        QApplication::sendEvent(view.viewport(), &helpEvent);
        QTest::qWait(250);

        bool foundView = false;
        bool foundTipLabel = false;
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
            if (widget == &view)
                foundView = true;
            if (widget->inherits("QTipLabel"))
                foundTipLabel = true;
        }
        QVERIFY(foundView);
        QVERIFY(foundTipLabel);
    }

    {
        QHelpEvent helpEvent(QEvent::ToolTip, view.viewport()->rect().topLeft(),
                             view.viewport()->mapToGlobal(view.viewport()->rect().topLeft()));
        QApplication::sendEvent(view.viewport(), &helpEvent);
        QTest::qWait(1000);

        bool foundView = false;
        bool foundTipLabel = false;
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
            if (widget == &view)
                foundView = true;
            if (widget->inherits("QTipLabel") && widget->isVisible())
                foundTipLabel = true;
        }
        QVERIFY(foundView);
        QVERIFY(!foundTipLabel);
    }
}

void tst_QGraphicsItem::visible()
{
    QGraphicsItem *item = new QGraphicsRectItem(QRectF(-10, -10, 20, 20));
    item->setFlag(QGraphicsItem::ItemIsMovable);
    QVERIFY(item->isVisible());
    item->setVisible(false);
    QVERIFY(!item->isVisible());
    item->setVisible(true);
    QVERIFY(item->isVisible());

    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    scene.addItem(item);
    QVERIFY(item->isVisible());
    QCOMPARE(scene.itemAt(0, 0), item);
    item->setVisible(false);
    QCOMPARE(scene.itemAt(0, 0), (QGraphicsItem *)0);
    item->setVisible(true);
    QCOMPARE(scene.itemAt(0, 0), item);

    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
    event.setButton(Qt::LeftButton);
    event.setScenePos(QPointF(0, 0));
    QApplication::sendEvent(&scene, &event);
    QCOMPARE(scene.mouseGrabberItem(), item);
    item->setVisible(false);
    QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)0);
    item->setVisible(true);
    QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)0);

    item->setFlag(QGraphicsItem::ItemIsFocusable);
    item->setFocus();
    QVERIFY(item->hasFocus());
    item->setVisible(false);
    QVERIFY(!item->hasFocus());
    item->setVisible(true);
    QVERIFY(!item->hasFocus());
}

void tst_QGraphicsItem::isVisibleTo()
{
    QGraphicsScene scene;
    QGraphicsItem *parent = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsItem *child = scene.addRect(QRectF(25, 25, 50, 50));
    QGraphicsItem *grandChild = scene.addRect(QRectF(50, 50, 50, 50));
    QGraphicsItem *stranger = scene.addRect(100, 100, 100, 100);

    child->setParentItem(parent);
    grandChild->setParentItem(child);

    QVERIFY(grandChild->isVisible());
    QVERIFY(grandChild->isVisibleTo(grandChild));
    QVERIFY(grandChild->isVisibleTo(child));
    QVERIFY(grandChild->isVisibleTo(parent));
    QVERIFY(grandChild->isVisibleTo(0));
    QVERIFY(child->isVisible());
    QVERIFY(child->isVisibleTo(child));
    QVERIFY(child->isVisibleTo(parent));
    QVERIFY(child->isVisibleTo(0));
    QVERIFY(parent->isVisible());
    QVERIFY(parent->isVisibleTo(parent));
    QVERIFY(parent->isVisibleTo(0));
    QVERIFY(!parent->isVisibleTo(child));
    QVERIFY(!child->isVisibleTo(grandChild));
    QVERIFY(!grandChild->isVisibleTo(stranger));
    QVERIFY(!child->isVisibleTo(stranger));
    QVERIFY(!parent->isVisibleTo(stranger));
    QVERIFY(!stranger->isVisibleTo(grandChild));
    QVERIFY(!stranger->isVisibleTo(child));
    QVERIFY(!stranger->isVisibleTo(parent));

    // Case 1: only parent is explicitly hidden
    parent->hide();

    QVERIFY(!grandChild->isVisible());
    QVERIFY(grandChild->isVisibleTo(grandChild));
    QVERIFY(grandChild->isVisibleTo(child));
    QVERIFY(grandChild->isVisibleTo(parent));
    QVERIFY(!grandChild->isVisibleTo(0));
    QVERIFY(!child->isVisible());
    QVERIFY(child->isVisibleTo(child));
    QVERIFY(child->isVisibleTo(parent));
    QVERIFY(!child->isVisibleTo(0));
    QVERIFY(!parent->isVisible());
    QVERIFY(!parent->isVisibleTo(parent));
    QVERIFY(!parent->isVisibleTo(0));
    QVERIFY(!parent->isVisibleTo(child));
    QVERIFY(!child->isVisibleTo(grandChild));
    QVERIFY(!grandChild->isVisibleTo(stranger));
    QVERIFY(!child->isVisibleTo(stranger));
    QVERIFY(!parent->isVisibleTo(stranger));
    QVERIFY(!stranger->isVisibleTo(grandChild));
    QVERIFY(!stranger->isVisibleTo(child));
    QVERIFY(!stranger->isVisibleTo(parent));

    // Case 2: only child is hidden
    parent->show();
    child->hide();

    QVERIFY(!grandChild->isVisible());
    QVERIFY(grandChild->isVisibleTo(grandChild));
    QVERIFY(grandChild->isVisibleTo(child));
    QVERIFY(!grandChild->isVisibleTo(parent));
    QVERIFY(!grandChild->isVisibleTo(0));
    QVERIFY(!child->isVisible());
    QVERIFY(!child->isVisibleTo(child));
    QVERIFY(!child->isVisibleTo(parent));
    QVERIFY(!child->isVisibleTo(0));
    QVERIFY(parent->isVisible());
    QVERIFY(parent->isVisibleTo(parent));
    QVERIFY(parent->isVisibleTo(0));
    QVERIFY(!parent->isVisibleTo(child));
    QVERIFY(!child->isVisibleTo(grandChild));
    QVERIFY(!grandChild->isVisibleTo(stranger));
    QVERIFY(!child->isVisibleTo(stranger));
    QVERIFY(!parent->isVisibleTo(stranger));
    QVERIFY(!stranger->isVisibleTo(grandChild));
    QVERIFY(!stranger->isVisibleTo(child));
    QVERIFY(!stranger->isVisibleTo(parent));

    // Case 3: only grand child is hidden
    child->show();
    grandChild->hide();

    QVERIFY(!grandChild->isVisible());
    QVERIFY(!grandChild->isVisibleTo(grandChild));
    QVERIFY(!grandChild->isVisibleTo(child));
    QVERIFY(!grandChild->isVisibleTo(parent));
    QVERIFY(!grandChild->isVisibleTo(0));
    QVERIFY(child->isVisible());
    QVERIFY(child->isVisibleTo(child));
    QVERIFY(child->isVisibleTo(parent));
    QVERIFY(child->isVisibleTo(0));
    QVERIFY(parent->isVisible());
    QVERIFY(parent->isVisibleTo(parent));
    QVERIFY(parent->isVisibleTo(0));
    QVERIFY(!parent->isVisibleTo(child));
    QVERIFY(!child->isVisibleTo(grandChild));
    QVERIFY(!grandChild->isVisibleTo(stranger));
    QVERIFY(!child->isVisibleTo(stranger));
    QVERIFY(!parent->isVisibleTo(stranger));
    QVERIFY(!stranger->isVisibleTo(grandChild));
    QVERIFY(!stranger->isVisibleTo(child));
    QVERIFY(!stranger->isVisibleTo(parent));
}

void tst_QGraphicsItem::explicitlyVisible()
{
    QGraphicsScene scene;
    QGraphicsItem *parent = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsItem *child = scene.addRect(QRectF(25, 25, 50, 50));
    child->setParentItem(parent);

    QVERIFY(parent->isVisible());
    QVERIFY(child->isVisible());

    parent->hide();

    QVERIFY(!parent->isVisible());
    QVERIFY(!child->isVisible());

    parent->show();
    child->hide();

    QVERIFY(parent->isVisible());
    QVERIFY(!child->isVisible());

    parent->hide();

    QVERIFY(!parent->isVisible());
    QVERIFY(!child->isVisible());

    parent->show();

    QVERIFY(parent->isVisible());
    QVERIFY(!child->isVisible()); // <- explicitly hidden

    child->show();

    QVERIFY(child->isVisible());

    parent->hide();

    QVERIFY(!parent->isVisible());
    QVERIFY(!child->isVisible()); // <- explicit show doesn't work

    parent->show();

    QVERIFY(parent->isVisible());
    QVERIFY(child->isVisible()); // <- no longer explicitly hidden

    // ------------------- Reparenting ------------------------------

    QGraphicsItem *parent2 = scene.addRect(-50, -50, 200, 200);
    QVERIFY(parent2->isVisible());

    // Reparent implicitly hidden item to a visible parent.
    parent->hide();
    QVERIFY(!parent->isVisible());
    QVERIFY(!child->isVisible());
    child->setParentItem(parent2);
    QVERIFY(parent2->isVisible());
    QVERIFY(child->isVisible());

    // Reparent implicitly hidden item to a hidden parent.
    child->setParentItem(parent);
    parent2->hide();
    child->setParentItem(parent2);
    QVERIFY(!parent2->isVisible());
    QVERIFY(!child->isVisible());

    // Reparent explicitly hidden item to a visible parent.
    child->hide();
    parent->show();
    child->setParentItem(parent);
    QVERIFY(parent->isVisible());
    QVERIFY(!child->isVisible());

    // Reparent explicitly hidden item to a hidden parent.
    child->setParentItem(parent2);
    QVERIFY(!parent2->isVisible());
    QVERIFY(!child->isVisible());

    // Reparent explicitly hidden item to a visible parent.
    parent->show();
    child->setParentItem(parent);
    QVERIFY(parent->isVisible());
    QVERIFY(!child->isVisible());

    // Reparent visible item to a hidden parent.
    child->show();
    parent2->hide();
    child->setParentItem(parent2);
    QVERIFY(!parent2->isVisible());
    QVERIFY(!child->isVisible());
    parent2->show();
    QVERIFY(parent2->isVisible());
    QVERIFY(child->isVisible());

    // Reparent implicitly hidden child to root.
    parent2->hide();
    QVERIFY(!child->isVisible());
    child->setParentItem(0);
    QVERIFY(child->isVisible());

    // Reparent an explicitly hidden child to root.
    child->hide();
    child->setParentItem(parent2);
    parent2->show();
    QVERIFY(!child->isVisible());
    child->setParentItem(0);
    QVERIFY(!child->isVisible());
}

void tst_QGraphicsItem::enabled()
{
    QGraphicsRectItem *item = new QGraphicsRectItem(QRectF(-10, -10, 20, 20));
    item->setFlag(QGraphicsItem::ItemIsMovable);
    QVERIFY(item->isEnabled());
    item->setEnabled(false);
    QVERIFY(!item->isEnabled());
    item->setEnabled(true);
    QVERIFY(item->isEnabled());
    item->setEnabled(false);
    item->setFlag(QGraphicsItem::ItemIsFocusable);
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    scene.addItem(item);
    item->setFocus();
    QVERIFY(!item->hasFocus());
    item->setEnabled(true);
    item->setFocus();
    QVERIFY(item->hasFocus());
    item->setEnabled(false);
    QVERIFY(!item->hasFocus());

    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
    event.setButton(Qt::LeftButton);
    event.setScenePos(QPointF(0, 0));
    QApplication::sendEvent(&scene, &event);
    QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)0);
    item->setEnabled(true);
    QApplication::sendEvent(&scene, &event);
    QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)item);
    item->setEnabled(false);
    QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)0);
}

void tst_QGraphicsItem::explicitlyEnabled()
{
    QGraphicsScene scene;
    QGraphicsItem *parent = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsItem *child = scene.addRect(QRectF(25, 25, 50, 50));
    child->setParentItem(parent);

    QVERIFY(parent->isEnabled());
    QVERIFY(child->isEnabled());

    parent->setEnabled(false);

    QVERIFY(!parent->isEnabled());
    QVERIFY(!child->isEnabled());

    parent->setEnabled(true);
    child->setEnabled(false);

    QVERIFY(parent->isEnabled());
    QVERIFY(!child->isEnabled());

    parent->setEnabled(false);

    QVERIFY(!parent->isEnabled());
    QVERIFY(!child->isEnabled());

    parent->setEnabled(true);

    QVERIFY(parent->isEnabled());
    QVERIFY(!child->isEnabled()); // <- explicitly disabled

    child->setEnabled(true);

    QVERIFY(child->isEnabled());

    parent->setEnabled(false);

    QVERIFY(!parent->isEnabled());
    QVERIFY(!child->isEnabled()); // <- explicit enabled doesn't work

    parent->setEnabled(true);

    QVERIFY(parent->isEnabled());
    QVERIFY(child->isEnabled()); // <- no longer explicitly disabled

    // ------------------- Reparenting ------------------------------

    QGraphicsItem *parent2 = scene.addRect(-50, -50, 200, 200);
    QVERIFY(parent2->isEnabled());

    // Reparent implicitly hidden item to a enabled parent.
    parent->setEnabled(false);
    QVERIFY(!parent->isEnabled());
    QVERIFY(!child->isEnabled());
    child->setParentItem(parent2);
    QVERIFY(parent2->isEnabled());
    QVERIFY(child->isEnabled());

    // Reparent implicitly hidden item to a hidden parent.
    child->setParentItem(parent);
    parent2->setEnabled(false);
    child->setParentItem(parent2);
    QVERIFY(!parent2->isEnabled());
    QVERIFY(!child->isEnabled());

    // Reparent explicitly hidden item to a enabled parent.
    child->setEnabled(false);
    parent->setEnabled(true);
    child->setParentItem(parent);
    QVERIFY(parent->isEnabled());
    QVERIFY(!child->isEnabled());

    // Reparent explicitly hidden item to a hidden parent.
    child->setParentItem(parent2);
    QVERIFY(!parent2->isEnabled());
    QVERIFY(!child->isEnabled());

    // Reparent explicitly hidden item to a enabled parent.
    parent->setEnabled(true);
    child->setParentItem(parent);
    QVERIFY(parent->isEnabled());
    QVERIFY(!child->isEnabled());

    // Reparent enabled item to a hidden parent.
    child->setEnabled(true);
    parent2->setEnabled(false);
    child->setParentItem(parent2);
    QVERIFY(!parent2->isEnabled());
    QVERIFY(!child->isEnabled());
    parent2->setEnabled(true);
    QVERIFY(parent2->isEnabled());
    QVERIFY(child->isEnabled());

    // Reparent implicitly hidden child to root.
    parent2->setEnabled(false);
    QVERIFY(!child->isEnabled());
    child->setParentItem(0);
    QVERIFY(child->isEnabled());

    // Reparent an explicitly hidden child to root.
    child->setEnabled(false);
    child->setParentItem(parent2);
    parent2->setEnabled(true);
    QVERIFY(!child->isEnabled());
    child->setParentItem(0);
    QVERIFY(!child->isEnabled());
}

class SelectChangeItem : public QGraphicsRectItem
{
public:
    SelectChangeItem() : QGraphicsRectItem(-50, -50, 100, 100) { setBrush(Qt::blue); }
    QList<bool> values;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value)
    {
        if (change == ItemSelectedChange)
            values << value.toBool();
        return QGraphicsRectItem::itemChange(change, value);
    }
};

void tst_QGraphicsItem::selected()
{
    SelectChangeItem *item = new SelectChangeItem;
    item->setFlag(QGraphicsItem::ItemIsSelectable);
    QVERIFY(!item->isSelected());
    QVERIFY(item->values.isEmpty());
    item->setSelected(true);
    QCOMPARE(item->values.size(), 1);
    QCOMPARE(item->values.last(), true);
    QVERIFY(item->isSelected());
    item->setSelected(false);
    QCOMPARE(item->values.size(), 2);
    QCOMPARE(item->values.last(), false);
    QVERIFY(!item->isSelected());
    item->setSelected(true);
    QCOMPARE(item->values.size(), 3);
    item->setEnabled(false);
    QCOMPARE(item->values.size(), 4);
    QCOMPARE(item->values.last(), false);
    QVERIFY(!item->isSelected());
    item->setEnabled(true);
    QCOMPARE(item->values.size(), 4);
    item->setSelected(true);
    QCOMPARE(item->values.size(), 5);
    QCOMPARE(item->values.last(), true);
    QVERIFY(item->isSelected());
    item->setVisible(false);
    QCOMPARE(item->values.size(), 6);
    QCOMPARE(item->values.last(), false);
    QVERIFY(!item->isSelected());
    item->setVisible(true);
    QCOMPARE(item->values.size(), 6);
    item->setSelected(true);
    QCOMPARE(item->values.size(), 7);
    QCOMPARE(item->values.last(), true);
    QVERIFY(item->isSelected());

    QGraphicsScene scene(-100, -100, 200, 200);
    scene.addItem(item);
    QCOMPARE(scene.selectedItems(), QList<QGraphicsItem *>() << item);
    item->setSelected(false);
    QVERIFY(scene.selectedItems().isEmpty());
    item->setSelected(true);
    QCOMPARE(scene.selectedItems(), QList<QGraphicsItem *>() << item);
    item->setSelected(false);
    QVERIFY(scene.selectedItems().isEmpty());

    // Interactive selection
    QGraphicsView view(&scene);
    view.setFixedSize(250, 250);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    qApp->processEvents();
    qApp->processEvents();

    scene.clearSelection();
    QCOMPARE(item->values.size(), 10);
    QCOMPARE(item->values.last(), false);
    QVERIFY(!item->isSelected());

    // Click inside and check that it's selected
    QTest::mouseMove(view.viewport());
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item->scenePos()));
    QCOMPARE(item->values.size(), 11);
    QCOMPARE(item->values.last(), true);
    QVERIFY(item->isSelected());

    // Click outside and check that it's not selected
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item->scenePos() + QPointF(item->boundingRect().width(), item->boundingRect().height())));
    QCOMPARE(item->values.size(), 12);
    QCOMPARE(item->values.last(), false);
    QVERIFY(!item->isSelected());

    SelectChangeItem *item2 = new SelectChangeItem;
    item2->setFlag(QGraphicsItem::ItemIsSelectable);
    item2->setPos(100, 0);
    scene.addItem(item2);

    // Click inside and check that it's selected
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item->scenePos()));
    QCOMPARE(item->values.size(), 13);
    QCOMPARE(item->values.last(), true);
    QVERIFY(item->isSelected());

    // Click inside item2 and check that it's selected, and item is not
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item2->scenePos()));
    QCOMPARE(item->values.size(), 14);
    QCOMPARE(item->values.last(), false);
    QVERIFY(!item->isSelected());
    QCOMPARE(item2->values.size(), 1);
    QCOMPARE(item2->values.last(), true);
    QVERIFY(item2->isSelected());
}

void tst_QGraphicsItem::selected2()
{
    // Selecting an item, then moving another previously caused a crash.
    QGraphicsScene scene;
    QGraphicsItem *line1 = scene.addRect(QRectF(0, 0, 100, 100));
    line1->setPos(-105, 0);
    line1->setFlag(QGraphicsItem::ItemIsSelectable);

    QGraphicsItem *line2 = scene.addRect(QRectF(0, 0, 100, 100));
    line2->setFlag(QGraphicsItem::ItemIsMovable);

    line1->setSelected(true);

    {
        QGraphicsSceneMouseEvent mousePress(QEvent::GraphicsSceneMousePress);
        mousePress.setScenePos(QPointF(50, 50));
        mousePress.setButton(Qt::LeftButton);
        QApplication::sendEvent(&scene, &mousePress);
        QVERIFY(mousePress.isAccepted());
    }
    {
        QGraphicsSceneMouseEvent mouseMove(QEvent::GraphicsSceneMouseMove);
        mouseMove.setScenePos(QPointF(60, 60));
        mouseMove.setButton(Qt::LeftButton);
        mouseMove.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &mouseMove);
        QVERIFY(mouseMove.isAccepted());
    }
}

void tst_QGraphicsItem::selected_group()
{
    QGraphicsScene scene;
    QGraphicsItem *item1 = scene.addRect(QRectF());
    QGraphicsItem *item2 = scene.addRect(QRectF());
    item1->setFlag(QGraphicsItem::ItemIsSelectable);
    item2->setFlag(QGraphicsItem::ItemIsSelectable);
    scene.addRect(QRectF())->setParentItem(item1);
    QGraphicsItem *leaf = scene.addRect(QRectF());
    leaf->setFlag(QGraphicsItem::ItemIsSelectable);
    leaf->setParentItem(item2);

    QGraphicsItemGroup *group = scene.createItemGroup(QList<QGraphicsItem *>() << item1 << item2);
    QCOMPARE(group->scene(), &scene);
    group->setFlag(QGraphicsItem::ItemIsSelectable);
    foreach (QGraphicsItem *item, scene.items()) {
        if (item == group)
            QVERIFY(!item->group());
        else
            QCOMPARE(item->group(), group);
    }

    QVERIFY(group->handlesChildEvents());
    QVERIFY(!group->isSelected());
    group->setSelected(false);
    QVERIFY(!group->isSelected());
    group->setSelected(true);
    QVERIFY(group->isSelected());
    foreach (QGraphicsItem *item, scene.items())
        QVERIFY(item->isSelected());
    group->setSelected(false);
    QVERIFY(!group->isSelected());
    foreach (QGraphicsItem *item, scene.items())
        QVERIFY(!item->isSelected());
    leaf->setSelected(true);
    foreach (QGraphicsItem *item, scene.items())
        QVERIFY(item->isSelected());
    leaf->setSelected(false);
    foreach (QGraphicsItem *item, scene.items())
        QVERIFY(!item->isSelected());

    leaf->setSelected(true);
    QGraphicsScene scene2;
    scene2.addItem(item1);
    QVERIFY(!item1->isSelected());
    QVERIFY(item2->isSelected());
}

void tst_QGraphicsItem::selected_textItem()
{
    QGraphicsScene scene;
    QGraphicsTextItem *text = scene.addText(QLatin1String("Text"));
    text->setFlag(QGraphicsItem::ItemIsSelectable);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(20);

    QTRY_VERIFY(!text->isSelected());
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0,
                      view.mapFromScene(text->mapToScene(0, 0)));
    QTRY_VERIFY(text->isSelected());

    text->setSelected(false);
    text->setTextInteractionFlags(Qt::TextEditorInteraction);

    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0,
                      view.mapFromScene(text->mapToScene(0, 0)));
    QTRY_VERIFY(text->isSelected());
}

void tst_QGraphicsItem::selected_multi()
{
    // Test multiselection behavior
    QGraphicsScene scene;

    // Create two disjoint items
    QGraphicsItem *item1 = scene.addRect(QRectF(-10, -10, 20, 20));
    QGraphicsItem *item2 = scene.addRect(QRectF(-10, -10, 20, 20));
    item1->setPos(-15, 0);
    item2->setPos(15, 20);

    // Make both items selectable
    item1->setFlag(QGraphicsItem::ItemIsSelectable);
    item2->setFlag(QGraphicsItem::ItemIsSelectable);

    // Create and show a view
    QGraphicsView view(&scene);
    view.show();
    view.fitInView(scene.sceneRect());
    qApp->processEvents();

    QVERIFY(!item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Start clicking
    QTest::qWait(200);

    // Click on item1
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Click on item2
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item2->scenePos()));
    QTest::qWait(20);
    QVERIFY(item2->isSelected());
    QVERIFY(!item1->isSelected());

    // Ctrl-click on item1
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(item2->isSelected());
    QVERIFY(item1->isSelected());

    // Ctrl-click on item1 again
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(item2->isSelected());
    QVERIFY(!item1->isSelected());

    // Ctrl-click on item2
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.mapFromScene(item2->scenePos()));
    QTest::qWait(20);
    QVERIFY(!item2->isSelected());
    QVERIFY(!item1->isSelected());

    // Click on item1
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Click on scene
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(0, 0));
    QTest::qWait(20);
    QVERIFY(!item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Click on item1
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Ctrl-click on scene
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.mapFromScene(0, 0));
    QTest::qWait(20);
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Click on scene
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(0, 0));
    QTest::qWait(20);
    QVERIFY(!item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Click on item1
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Press on item2
    QTest::mousePress(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item2->scenePos()));
    QTest::qWait(20);
    QVERIFY(!item1->isSelected());
    QVERIFY(item2->isSelected());

    // Release on item2
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item2->scenePos()));
    QTest::qWait(20);
    QVERIFY(!item1->isSelected());
    QVERIFY(item2->isSelected());

    // Click on item1
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Ctrl-click on item1
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(!item1->isSelected());
    QVERIFY(!item2->isSelected());

    // Ctrl-press on item1
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(!item1->isSelected());
    QVERIFY(!item2->isSelected());

    {
        // Ctrl-move on item1
        QMouseEvent event(QEvent::MouseMove, view.mapFromScene(item1->scenePos()) + QPoint(1, 0), Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
        QApplication::sendEvent(view.viewport(), &event);
        QTest::qWait(20);
        QVERIFY(!item1->isSelected());
        QVERIFY(!item2->isSelected());
    }

    // Release on item1
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());

    item1->setFlag(QGraphicsItem::ItemIsMovable);
    item1->setSelected(false);

    // Ctrl-press on item1
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(!item1->isSelected());
    QVERIFY(!item2->isSelected());

    {
        // Ctrl-move on item1
        QMouseEvent event(QEvent::MouseMove, view.mapFromScene(item1->scenePos()) + QPoint(1, 0), Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
        QApplication::sendEvent(view.viewport(), &event);
        QTest::qWait(20);
        QVERIFY(item1->isSelected());
        QVERIFY(!item2->isSelected());
    }

    // Release on item1
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::ControlModifier, view.mapFromScene(item1->scenePos()));
    QTest::qWait(20);
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());
}

void tst_QGraphicsItem::acceptedMouseButtons()
{
    QGraphicsScene scene;
    QGraphicsRectItem *item1 = scene.addRect(QRectF(-10, -10, 20, 20));
    QGraphicsRectItem *item2 = scene.addRect(QRectF(-10, -10, 20, 20));
    item2->setZValue(1);

    item1->setFlag(QGraphicsItem::ItemIsMovable);
    item2->setFlag(QGraphicsItem::ItemIsMovable);

    QCOMPARE(item1->acceptedMouseButtons(), Qt::MouseButtons(0x1f));
    QCOMPARE(item2->acceptedMouseButtons(), Qt::MouseButtons(0x1f));

    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
    event.setButton(Qt::LeftButton);
    event.setScenePos(QPointF(0, 0));
    QApplication::sendEvent(&scene, &event);
    QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)item2);
    item2->setAcceptedMouseButtons(0);
    QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)0);
    QApplication::sendEvent(&scene, &event);
    QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)item1);
}

class HoverItem : public QGraphicsRectItem
{
public:
    HoverItem(const QRectF &rect)
        : QGraphicsRectItem(rect), hoverInCount(0),
          hoverMoveCount(0), hoverOutCount(0)
    { }

    int hoverInCount;
    int hoverMoveCount;
    int hoverOutCount;
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *)
    { ++hoverInCount; }

    void hoverMoveEvent(QGraphicsSceneHoverEvent *)
    { ++hoverMoveCount; }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent *)
    { ++hoverOutCount; }
};

void tst_QGraphicsItem::acceptsHoverEvents()
{
    QGraphicsScene scene;
    HoverItem *item1 = new HoverItem(QRectF(-10, -10, 20, 20));
    HoverItem *item2 = new HoverItem(QRectF(-5, -5, 10, 10));
    scene.addItem(item1);
    scene.addItem(item2);
    item2->setZValue(1);

    QVERIFY(!item1->acceptsHoverEvents());
    QVERIFY(!item2->acceptsHoverEvents());
    item1->setAcceptsHoverEvents(true);
    item2->setAcceptsHoverEvents(true);

    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseMove);
    event.setScenePos(QPointF(-100, -100));
    QApplication::sendEvent(&scene, &event);
    event.setScenePos(QPointF(-2.5, -2.5));
    QApplication::sendEvent(&scene, &event);

    QCOMPARE(item1->hoverInCount, 0);
    QCOMPARE(item2->hoverInCount, 1);

    item1->setAcceptsHoverEvents(false);
    item2->setAcceptsHoverEvents(false);

    event.setScenePos(QPointF(-100, -100));
    QApplication::sendEvent(&scene, &event);
    event.setScenePos(QPointF(-2.5, -2.5));
    QApplication::sendEvent(&scene, &event);

    QCOMPARE(item1->hoverInCount, 0);
    QCOMPARE(item2->hoverInCount, 1);

    item1->setAcceptsHoverEvents(true);
    item2->setAcceptsHoverEvents(false);

    event.setScenePos(QPointF(-100, -100));
    QApplication::sendEvent(&scene, &event);
    event.setScenePos(QPointF(-2.5, -2.5));
    QApplication::sendEvent(&scene, &event);

    QCOMPARE(item1->hoverInCount, 1);
    QCOMPARE(item2->hoverInCount, 1);
}

void tst_QGraphicsItem::childAcceptsHoverEvents()
{
    QGraphicsScene scene;
    HoverItem *item1 = new HoverItem(QRectF(-10, -10, 20, 20));
    HoverItem *item2 = new HoverItem(QRectF(-5, -5, 10, 10));

    scene.addItem(item1);
    scene.addItem(item2);
    item2->setParentItem(item1);
    item2->setAcceptHoverEvents(true);

    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseMove);
    event.setScenePos(QPointF(-100, -100));
    QApplication::sendEvent(&scene, &event);
    QCOMPARE(item2->hoverInCount, 0);
    QCOMPARE(item2->hoverMoveCount, 0);
    QCOMPARE(item2->hoverOutCount, 0);
    QCOMPARE(item1->hoverInCount, 0);
    QCOMPARE(item1->hoverMoveCount, 0);
    QCOMPARE(item1->hoverOutCount, 0);

    event.setScenePos(QPointF(-2.5, -2.5));
    QApplication::sendEvent(&scene, &event);

    QCOMPARE(item2->hoverInCount, 1);
    QCOMPARE(item2->hoverMoveCount, 1);
    QCOMPARE(item2->hoverOutCount, 0);
    QCOMPARE(item1->hoverInCount, 0);
    QCOMPARE(item1->hoverMoveCount, 0);
    QCOMPARE(item1->hoverOutCount, 0);

    event.setScenePos(QPointF(0, 0));
    QApplication::sendEvent(&scene, &event);

    QCOMPARE(item2->hoverInCount, 1);
    QCOMPARE(item2->hoverMoveCount, 2);
    QCOMPARE(item2->hoverOutCount, 0);
    QCOMPARE(item1->hoverInCount, 0);
    QCOMPARE(item1->hoverMoveCount, 0);
    QCOMPARE(item1->hoverOutCount, 0);

    event.setScenePos(QPointF(-7, -7));
    QApplication::sendEvent(&scene, &event);

    QCOMPARE(item2->hoverInCount, 1);
    QCOMPARE(item2->hoverMoveCount, 2);
    QCOMPARE(item2->hoverOutCount, 1);
    QCOMPARE(item1->hoverInCount, 0);
    QCOMPARE(item1->hoverMoveCount, 0);
    QCOMPARE(item1->hoverOutCount, 0);

    event.setScenePos(QPointF(0, 0));
    QApplication::sendEvent(&scene, &event);

    QCOMPARE(item2->hoverInCount, 2);
    QCOMPARE(item2->hoverMoveCount, 3);
    QCOMPARE(item2->hoverOutCount, 1);
    QCOMPARE(item1->hoverInCount, 0);
    QCOMPARE(item1->hoverMoveCount, 0);
    QCOMPARE(item1->hoverOutCount, 0);

    HoverItem *item0 = new HoverItem(QRectF(-20, -20, 20, 20));
    scene.addItem(item0);
    item1->setParentItem(item0);
    item0->setAcceptHoverEvents(true);

    event.setScenePos(QPointF(-100, -100));
    QApplication::sendEvent(&scene, &event);

    event.setScenePos(QPointF(-15, -15));
    QApplication::sendEvent(&scene, &event);

    QCOMPARE(item2->hoverInCount, 2);
    QCOMPARE(item2->hoverMoveCount, 3);
    QCOMPARE(item2->hoverOutCount, 2);
    QCOMPARE(item1->hoverInCount, 0);
    QCOMPARE(item1->hoverMoveCount, 0);
    QCOMPARE(item1->hoverOutCount, 0);
    QCOMPARE(item0->hoverInCount, 1);
    QCOMPARE(item0->hoverMoveCount, 1);
    QCOMPARE(item0->hoverOutCount, 0);
}

void tst_QGraphicsItem::hasFocus()
{
    QGraphicsLineItem *line = new QGraphicsLineItem;
    QVERIFY(!line->hasFocus());
    line->setFocus();
    QVERIFY(!line->hasFocus());

    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    scene.addItem(line);

    line->setFocus();
    QVERIFY(!line->hasFocus());
    line->setFlag(QGraphicsItem::ItemIsFocusable);
    line->setFocus();
    QVERIFY(line->hasFocus());

    QGraphicsScene scene2;
    QApplication::sendEvent(&scene2, &activate);

    scene2.addItem(line);
    QVERIFY(!line->hasFocus());

    QCOMPARE(scene.focusItem(), (QGraphicsItem *)0);
    QCOMPARE(scene2.focusItem(), (QGraphicsItem *)0);

    line->setFocus();
    QVERIFY(line->hasFocus());
    line->clearFocus();
    QVERIFY(!line->hasFocus());

    QGraphicsLineItem *line2 = new QGraphicsLineItem;
    line2->setFlag(QGraphicsItem::ItemIsFocusable);
    scene2.addItem(line2);

    line2->setFocus();
    QVERIFY(!line->hasFocus());
    QVERIFY(line2->hasFocus());
    line->setFocus();
    QVERIFY(line->hasFocus());
    QVERIFY(!line2->hasFocus());
}

void tst_QGraphicsItem::pos()
{
    QGraphicsItem *child = new QGraphicsLineItem;
    QGraphicsItem *parent = new QGraphicsLineItem;

    QCOMPARE(child->pos(), QPointF());
    QCOMPARE(parent->pos(), QPointF());

    child->setParentItem(parent);
    child->setPos(10, 10);

    QCOMPARE(child->pos(), QPointF(10, 10));

    parent->setPos(10, 10);

    QCOMPARE(parent->pos(), QPointF(10, 10));
    QCOMPARE(child->pos(), QPointF(10, 10));

    delete child;
    delete parent;
}

void tst_QGraphicsItem::scenePos()
{
    QGraphicsItem *child = new QGraphicsLineItem;
    QGraphicsItem *parent = new QGraphicsLineItem;

    QCOMPARE(child->scenePos(), QPointF());
    QCOMPARE(parent->scenePos(), QPointF());

    child->setParentItem(parent);
    child->setPos(10, 10);

    QCOMPARE(child->scenePos(), QPointF(10, 10));

    parent->setPos(10, 10);

    QCOMPARE(parent->scenePos(), QPointF(10, 10));
    QCOMPARE(child->scenePos(), QPointF(20, 20));

    parent->setPos(20, 20);

    QCOMPARE(parent->scenePos(), QPointF(20, 20));
    QCOMPARE(child->scenePos(), QPointF(30, 30));

    delete child;
    delete parent;
}

void tst_QGraphicsItem::matrix()
{
    QGraphicsLineItem line;
    QCOMPARE(line.matrix(), QMatrix());
    line.setMatrix(QMatrix().rotate(90));
    QCOMPARE(line.matrix(), QMatrix().rotate(90));
    line.setMatrix(QMatrix().rotate(90));
    QCOMPARE(line.matrix(), QMatrix().rotate(90));
    line.setMatrix(QMatrix().rotate(90), true);
    QCOMPARE(line.matrix(), QMatrix().rotate(180));
    line.setMatrix(QMatrix().rotate(-90), true);
    QCOMPARE(line.matrix(), QMatrix().rotate(90));
    line.resetMatrix();
    QCOMPARE(line.matrix(), QMatrix());

    line.rotate(90);
    QCOMPARE(line.matrix(), QMatrix().rotate(90));
    line.rotate(90);
    QCOMPARE(line.matrix(), QMatrix().rotate(90).rotate(90));
    line.resetMatrix();

    line.scale(2, 4);
    QCOMPARE(line.matrix(), QMatrix().scale(2, 4));
    line.scale(2, 4);
    QCOMPARE(line.matrix(), QMatrix().scale(2, 4).scale(2, 4));
    line.resetMatrix();

    line.shear(2, 4);
    QCOMPARE(line.matrix(), QMatrix().shear(2, 4));
    line.shear(2, 4);
    QCOMPARE(line.matrix(), QMatrix().shear(2, 4).shear(2, 4));
    line.resetMatrix();

    line.translate(10, 10);
    QCOMPARE(line.matrix(), QMatrix().translate(10, 10));
    line.translate(10, 10);
    QCOMPARE(line.matrix(), QMatrix().translate(10, 10).translate(10, 10));
    line.resetMatrix();
}

void tst_QGraphicsItem::sceneMatrix()
{
    QGraphicsLineItem *parent = new QGraphicsLineItem;
    QGraphicsLineItem *child = new QGraphicsLineItem(QLineF(), parent);

    QCOMPARE(parent->sceneMatrix(), QMatrix());
    QCOMPARE(child->sceneMatrix(), QMatrix());

    parent->translate(10, 10);
    QCOMPARE(parent->sceneMatrix(), QMatrix().translate(10, 10));
    QCOMPARE(child->sceneMatrix(), QMatrix().translate(10, 10));

    child->translate(10, 10);
    QCOMPARE(parent->sceneMatrix(), QMatrix().translate(10, 10));
    QCOMPARE(child->sceneMatrix(), QMatrix().translate(20, 20));

    parent->rotate(90);
    QCOMPARE(parent->sceneMatrix(), QMatrix().translate(10, 10).rotate(90));
    QCOMPARE(child->sceneMatrix(), QMatrix().translate(10, 10).rotate(90).translate(10, 10));

    delete child;
    delete parent;
}

void tst_QGraphicsItem::setMatrix()
{
    QGraphicsScene scene;
    QSignalSpy spy(&scene, SIGNAL(changed(QList<QRectF>)));
    QRectF unrotatedRect(-12, -34, 56, 78);
    QGraphicsRectItem item(unrotatedRect, 0);
    item.setPen(QPen(Qt::black, 0));
    scene.addItem(&item);
    scene.update(scene.sceneRect());
    QApplication::instance()->processEvents();

    QCOMPARE(spy.count(), 1);

    item.setMatrix(QMatrix().rotate(qreal(12.34)));
    QRectF rotatedRect = scene.sceneRect();
    QVERIFY(unrotatedRect != rotatedRect);
    scene.update(scene.sceneRect());
    QApplication::instance()->processEvents();

    QCOMPARE(spy.count(), 2);

    item.setMatrix(QMatrix());

    scene.update(scene.sceneRect());
    QApplication::instance()->processEvents();

    QCOMPARE(spy.count(), 3);
    QList<QRectF> rlist = qvariant_cast<QList<QRectF> >(spy.last().at(0));

    QCOMPARE(rlist.size(), 3);
    QCOMPARE(rlist.at(0), rotatedRect);   // From item.setMatrix() (clearing rotated rect)
    QCOMPARE(rlist.at(1), rotatedRect);   // From scene.update()   (updating scene rect)
    QCOMPARE(rlist.at(2), unrotatedRect); // From post-update      (update current state)
}

static QList<QGraphicsItem *> _paintedItems;
class PainterItem : public QGraphicsItem
{
protected:
    QRectF boundingRect() const
    { return QRectF(-10, -10, 20, 20); }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
    { _paintedItems << this; painter->fillRect(boundingRect(), Qt::red); }
};

void tst_QGraphicsItem::zValue()
{
    Q_CHECK_PAINTEVENTS

    QGraphicsScene scene;

    QGraphicsItem *item1 = new PainterItem;
    QGraphicsItem *item2 = new PainterItem;
    QGraphicsItem *item3 = new PainterItem;
    QGraphicsItem *item4 = new PainterItem;
    scene.addItem(item1);
    scene.addItem(item2);
    scene.addItem(item3);
    scene.addItem(item4);
    item2->setZValue(-3);
    item4->setZValue(-2);
    item1->setZValue(-1);
    item3->setZValue(0);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QApplication::processEvents();

    QTRY_VERIFY(!_paintedItems.isEmpty());
    QVERIFY((_paintedItems.size() % 4) == 0);
    for (int i = 0; i < 3; ++i)
        QVERIFY(_paintedItems.at(i)->zValue() < _paintedItems.at(i + 1)->zValue());
}

void tst_QGraphicsItem::shape()
{
    QGraphicsLineItem line(QLineF(-10, -10, 20, 20));
    line.setPen(QPen(Qt::black, 0));

    // We unfortunately need this hack as QPainterPathStroker will set a width of 1.0
    // if we pass a value of 0.0 to QPainterPathStroker::setWidth()
    const qreal penWidthZero = qreal(0.00000001);

    QPainterPathStroker ps;
    ps.setWidth(penWidthZero);

    QPainterPath path(line.line().p1());
    path.lineTo(line.line().p2());
    QPainterPath p = ps.createStroke(path);
    p.addPath(path);
    QCOMPARE(line.shape(), p);

    QPen linePen;
    linePen.setWidthF(5.0);
    linePen.setCapStyle(Qt::RoundCap);
    line.setPen(linePen);

    ps.setCapStyle(line.pen().capStyle());
    ps.setWidth(line.pen().widthF());
    p = ps.createStroke(path);
    p.addPath(path);
    QCOMPARE(line.shape(), p);

    linePen.setCapStyle(Qt::FlatCap);
    line.setPen(linePen);
    ps.setCapStyle(line.pen().capStyle());
    p = ps.createStroke(path);
    p.addPath(path);
    QCOMPARE(line.shape(), p);

    linePen.setCapStyle(Qt::SquareCap);
    line.setPen(linePen);
    ps.setCapStyle(line.pen().capStyle());
    p = ps.createStroke(path);
    p.addPath(path);
    QCOMPARE(line.shape(), p);

    QGraphicsRectItem rect(QRectF(-10, -10, 20, 20));
    rect.setPen(QPen(Qt::black, 0));
    QPainterPathStroker ps1;
    ps1.setWidth(penWidthZero);
    path = QPainterPath();
    path.addRect(rect.rect());
    p = ps1.createStroke(path);
    p.addPath(path);
    QCOMPARE(rect.shape(), p);

    QGraphicsEllipseItem ellipse(QRectF(-10, -10, 20, 20));
    ellipse.setPen(QPen(Qt::black, 0));
    QPainterPathStroker ps2;
    ps2.setWidth(ellipse.pen().widthF() <= 0.0 ? penWidthZero : ellipse.pen().widthF());
    path = QPainterPath();
    path.addEllipse(ellipse.rect());
    p = ps2.createStroke(path);
    p.addPath(path);
    QCOMPARE(ellipse.shape(), p);

    QPainterPathStroker ps3;
    ps3.setWidth(penWidthZero);
    p = ps3.createStroke(path);
    p.addPath(path);
    QGraphicsPathItem pathItem(path);
    pathItem.setPen(QPen(Qt::black, 0));
    QCOMPARE(pathItem.shape(), p);

    QRegion region(QRect(0, 0, 300, 200));
    region = region.subtracted(QRect(50, 50, 200, 100));

    QImage image(300, 200, QImage::Format_ARGB32_Premultiplied);
    image.fill(0);
    QPainter painter(&image);
    painter.setClipRegion(region);
    painter.fillRect(0, 0, 300, 200, Qt::green);
    painter.end();
    QPixmap pixmap = QPixmap::fromImage(image);

    QGraphicsPixmapItem pixmapItem(pixmap);
    path = QPainterPath();
    path.addRegion(region);

    {
        QBitmap bitmap(300, 200);
        bitmap.clear();
        QPainter painter(&bitmap);
        painter.setClipRegion(region);
        painter.fillRect(0, 0, 300, 200, Qt::color1);
        painter.end();

        QBitmap bitmap2(300, 200);
        bitmap2.clear();
        painter.begin(&bitmap2);
        painter.setClipPath(pixmapItem.shape());
        painter.fillRect(0, 0, 300, 200, Qt::color1);
        painter.end();

        QCOMPARE(bitmap.toImage(), bitmap2.toImage());
    }

    QPolygonF poly;
    poly << QPointF(0, 0) << QPointF(10, 0) << QPointF(0, 10);
    QGraphicsPolygonItem polygon(poly);
    polygon.setPen(QPen(Qt::black, 0));
    path = QPainterPath();
    path.addPolygon(poly);

    QPainterPathStroker ps4;
    ps4.setWidth(penWidthZero);
    p = ps4.createStroke(path);
    p.addPath(path);
    QCOMPARE(polygon.shape(), p);
}

void tst_QGraphicsItem::contains()
{
    if (sizeof(qreal) != sizeof(double))
        QSKIP("Skipped due to rounding errors");

    // Rect
    QGraphicsRectItem rect(QRectF(-10, -10, 20, 20));
    QVERIFY(!rect.contains(QPointF(-11, -10)));
    QVERIFY(rect.contains(QPointF(-10, -10)));
    QVERIFY(!rect.contains(QPointF(-11, 0)));
    QVERIFY(rect.contains(QPointF(-10, 0)));
    QVERIFY(rect.contains(QPointF(0, -10)));
    QVERIFY(rect.contains(QPointF(0, 0)));
    QVERIFY(rect.contains(QPointF(9, 9)));

    // Ellipse
    QGraphicsEllipseItem ellipse(QRectF(-10, -10, 20, 20));
    QVERIFY(!ellipse.contains(QPointF(-10, -10)));
    QVERIFY(ellipse.contains(QPointF(-9, 0)));
    QVERIFY(ellipse.contains(QPointF(0, -9)));
    QVERIFY(ellipse.contains(QPointF(0, 0)));
    QVERIFY(!ellipse.contains(QPointF(9, 9)));

    // Line
    QGraphicsLineItem line(QLineF(-10, -10, 20, 20));
    QVERIFY(!line.contains(QPointF(-10, 0)));
    QVERIFY(!line.contains(QPointF(0, -10)));
    QVERIFY(!line.contains(QPointF(10, 0)));
    QVERIFY(!line.contains(QPointF(0, 10)));
    QVERIFY(line.contains(QPointF(0, 0)));
    QVERIFY(line.contains(QPointF(-9, -9)));
    QVERIFY(line.contains(QPointF(9, 9)));

    // Polygon
    QGraphicsPolygonItem polygon(QPolygonF()
                                 << QPointF(0, 0)
                                 << QPointF(10, 0)
                                 << QPointF(0, 10));
    QVERIFY(polygon.contains(QPointF(1, 1)));
    QVERIFY(polygon.contains(QPointF(4, 4)));
    QVERIFY(polygon.contains(QPointF(1, 4)));
    QVERIFY(polygon.contains(QPointF(4, 1)));
    QVERIFY(!polygon.contains(QPointF(8, 8)));
    QVERIFY(polygon.contains(QPointF(1, 8)));
    QVERIFY(polygon.contains(QPointF(8, 1)));
}

void tst_QGraphicsItem::collidesWith_item()
{
    // Rectangle
    QGraphicsRectItem rect(QRectF(-10, -10, 20, 20));
    QGraphicsRectItem rect2(QRectF(-10, -10, 20, 20));
    QVERIFY(rect.collidesWithItem(&rect2));
    QVERIFY(rect2.collidesWithItem(&rect));
    rect2.setPos(21, 21);
    QVERIFY(!rect.collidesWithItem(&rect2));
    QVERIFY(!rect2.collidesWithItem(&rect));
    rect2.setPos(-21, -21);
    QVERIFY(!rect.collidesWithItem(&rect2));
    QVERIFY(!rect2.collidesWithItem(&rect));
    rect2.setPos(-17, -17);
    QVERIFY(rect.collidesWithItem(&rect2));
    QVERIFY(rect2.collidesWithItem(&rect));

    QGraphicsEllipseItem ellipse(QRectF(-10, -10, 20, 20));
    QGraphicsEllipseItem ellipse2(QRectF(-10, -10, 20, 20));
    QVERIFY(ellipse.collidesWithItem(&ellipse2));
    QVERIFY(ellipse2.collidesWithItem(&ellipse));
    ellipse2.setPos(21, 21);
    QVERIFY(!ellipse.collidesWithItem(&ellipse2));
    QVERIFY(!ellipse2.collidesWithItem(&ellipse));
    ellipse2.setPos(-21, -21);
    QVERIFY(!ellipse.collidesWithItem(&ellipse2));
    QVERIFY(!ellipse2.collidesWithItem(&ellipse));

    ellipse2.setPos(-17, -17);
    QVERIFY(!ellipse.collidesWithItem(&ellipse2));
    QVERIFY(!ellipse2.collidesWithItem(&ellipse));

    {
        QGraphicsScene scene;
        QGraphicsRectItem rect(20, 20, 100, 100, 0);
        scene.addItem(&rect);
        QGraphicsRectItem rect2(40, 40, 50, 50, 0);
        scene.addItem(&rect2);
        rect2.setZValue(1);
        QGraphicsLineItem line(0, 0, 200, 200, 0);
        scene.addItem(&line);
        line.setZValue(2);

        QCOMPARE(scene.items().size(), 3);

        QList<QGraphicsItem *> col1 = rect.collidingItems();
        QCOMPARE(col1.size(), 2);
        QCOMPARE(col1.first(), static_cast<QGraphicsItem *>(&line));
        QCOMPARE(col1.last(), static_cast<QGraphicsItem *>(&rect2));

        QList<QGraphicsItem *> col2 = rect2.collidingItems();
        QCOMPARE(col2.size(), 2);
        QCOMPARE(col2.first(), static_cast<QGraphicsItem *>(&line));
        QCOMPARE(col2.last(), static_cast<QGraphicsItem *>(&rect));

        QList<QGraphicsItem *> col3 = line.collidingItems();
        QCOMPARE(col3.size(), 2);
        QCOMPARE(col3.first(), static_cast<QGraphicsItem *>(&rect2));
        QCOMPARE(col3.last(), static_cast<QGraphicsItem *>(&rect));
    }
}

void tst_QGraphicsItem::collidesWith_path_data()
{
    QTest::addColumn<QPointF>("pos");
    QTest::addColumn<QMatrix>("matrix");
    QTest::addColumn<QPainterPath>("shape");
    QTest::addColumn<bool>("rectCollides");
    QTest::addColumn<bool>("ellipseCollides");

    QTest::newRow("nothing") << QPointF(0, 0) << QMatrix() << QPainterPath() << false << false;

    QPainterPath rect;
    rect.addRect(0, 0, 20, 20);

    QTest::newRow("rect1") << QPointF(0, 0) << QMatrix() << rect << true << true;
    QTest::newRow("rect2") << QPointF(0, 0) << QMatrix().translate(21, 21) << rect << false << false;
    QTest::newRow("rect3") << QPointF(21, 21) << QMatrix() << rect << false << false;
}

void tst_QGraphicsItem::collidesWith_path()
{
    QFETCH(QPointF, pos);
    QFETCH(QMatrix, matrix);
    QFETCH(QPainterPath, shape);
    QFETCH(bool, rectCollides);
    QFETCH(bool, ellipseCollides);

    QGraphicsRectItem rect(QRectF(0, 0, 20, 20));
    QGraphicsEllipseItem ellipse(QRectF(0, 0, 20, 20));

    rect.setPos(pos);
    rect.setMatrix(matrix);

    ellipse.setPos(pos);
    ellipse.setMatrix(matrix);

    QPainterPath mappedShape = rect.sceneMatrix().inverted().map(shape);

    if (rectCollides)
        QVERIFY(rect.collidesWithPath(mappedShape));
    else
        QVERIFY(!rect.collidesWithPath(mappedShape));

    if (ellipseCollides)
        QVERIFY(ellipse.collidesWithPath(mappedShape));
    else
        QVERIFY(!ellipse.collidesWithPath(mappedShape));
}

void tst_QGraphicsItem::collidesWithItemWithClip()
{
    QGraphicsScene scene;

    QGraphicsEllipseItem *ellipse = scene.addEllipse(0, 0, 100, 100);
    ellipse->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    QGraphicsEllipseItem *ellipse2 = scene.addEllipse(0, 0, 10, 10);
    ellipse2->setParentItem(ellipse);
    QGraphicsEllipseItem *ellipse3 = scene.addEllipse(0, 0, 10, 10);
    ellipse3->setParentItem(ellipse);
    QGraphicsEllipseItem *ellipse5 = scene.addEllipse(50, 50, 10, 10);
    ellipse5->setParentItem(ellipse);
    QGraphicsEllipseItem *ellipse4 = scene.addEllipse(0, 0, 10, 10);

    QVERIFY(ellipse2->collidesWithItem(ellipse3));
    QVERIFY(ellipse3->collidesWithItem(ellipse2));
    QVERIFY(!ellipse2->collidesWithItem(ellipse));
    QVERIFY(!ellipse->collidesWithItem(ellipse2));
    QVERIFY(!ellipse4->collidesWithItem(ellipse));
    QVERIFY(!ellipse4->collidesWithItem(ellipse2));
    QVERIFY(!ellipse4->collidesWithItem(ellipse3));
    QVERIFY(!ellipse->collidesWithItem(ellipse4));
    QVERIFY(!ellipse2->collidesWithItem(ellipse4));
    QVERIFY(!ellipse3->collidesWithItem(ellipse4));
    QVERIFY(ellipse->collidesWithItem(ellipse5));
    QVERIFY(ellipse5->collidesWithItem(ellipse));
}

class MyItem : public QGraphicsEllipseItem
{
public:
    bool isObscuredBy(const QGraphicsItem *item) const
    {
        const MyItem *myItem = qgraphicsitem_cast<const MyItem *>(item);
        if (myItem) {
            if (item->zValue() > zValue()) {
                QRectF r = rect();
                QPointF topMid = (r.topRight()+r.topLeft())/2;
                QPointF botMid = (r.bottomRight()+r.bottomLeft())/2;
                QPointF leftMid = (r.topLeft()+r.bottomLeft())/2;
                QPointF rightMid = (r.topRight()+r.bottomRight())/2;

                QPainterPath mappedShape = item->mapToItem(this, item->opaqueArea());

                if (mappedShape.contains(topMid) &&
                    mappedShape.contains(botMid) &&
                    mappedShape.contains(leftMid) &&
                    mappedShape.contains(rightMid))
                    return true;
                else
                    return false;
            }
            else return false;
        }
        else
            return QGraphicsItem::isObscuredBy(item);
    }

    QPainterPath opaqueArea() const
    {
        return shape();
    }

    enum {
        Type = UserType+1
    };
    int type() const { return Type; }
};

void tst_QGraphicsItem::isObscuredBy()
{
    QGraphicsScene scene;

    MyItem myitem1, myitem2;

    myitem1.setRect(QRectF(50, 50, 40, 200));
    myitem1.rotate(67);

    myitem2.setRect(QRectF(25, 25, 20, 20));
    myitem2.setZValue(-1.0);
    scene.addItem(&myitem1);
    scene.addItem(&myitem2);

    QVERIFY(!myitem2.isObscuredBy(&myitem1));
    QVERIFY(!myitem1.isObscuredBy(&myitem2));

    myitem2.setRect(QRectF(-50, 85, 20, 20));
    QVERIFY(myitem2.isObscuredBy(&myitem1));
    QVERIFY(!myitem1.isObscuredBy(&myitem2));

    myitem2.setRect(QRectF(-30, 70, 20, 20));
    QVERIFY(!myitem2.isObscuredBy(&myitem1));
    QVERIFY(!myitem1.isObscuredBy(&myitem2));

    QGraphicsRectItem rect1, rect2;

    rect1.setRect(QRectF(-40, -40, 50, 50));
    rect1.setBrush(QBrush(Qt::red));
    rect2.setRect(QRectF(-30, -20, 20, 20));
    rect2.setZValue(-1.0);
    rect2.setBrush(QBrush(Qt::blue));

    QVERIFY(rect2.isObscuredBy(&rect1));
    QVERIFY(!rect1.isObscuredBy(&rect2));

    rect2.setPos(QPointF(-20, -25));

    QVERIFY(!rect2.isObscuredBy(&rect1));
    QVERIFY(!rect1.isObscuredBy(&rect2));

    rect2.setPos(QPointF(-100, -100));

    QVERIFY(!rect2.isObscuredBy(&rect1));
    QVERIFY(!rect1.isObscuredBy(&rect2));
}

class OpaqueItem : public QGraphicsRectItem
{
protected:
    QPainterPath opaqueArea() const
    {
        return shape();
    }
};

void tst_QGraphicsItem::isObscured()
{
    if (sizeof(qreal) != sizeof(double))
        QSKIP("Skipped due to rounding errors");

    OpaqueItem *item1 = new OpaqueItem;
    item1->setRect(0, 0, 100, 100);
    item1->setZValue(0);

    OpaqueItem *item2 = new OpaqueItem;
    item2->setZValue(1);
    item2->setRect(0, 0, 100, 100);

    QGraphicsScene scene;
    scene.addItem(item1);
    scene.addItem(item2);

    QVERIFY(item1->isObscured());
    QVERIFY(item1->isObscuredBy(item2));
    QVERIFY(item1->isObscured(QRectF(0, 0, 50, 50)));
    QVERIFY(item1->isObscured(QRectF(50, 0, 50, 50)));
    QVERIFY(item1->isObscured(QRectF(50, 50, 50, 50)));
    QVERIFY(item1->isObscured(QRectF(0, 50, 50, 50)));
    QVERIFY(item1->isObscured(0, 0, 50, 50));
    QVERIFY(item1->isObscured(50, 0, 50, 50));
    QVERIFY(item1->isObscured(50, 50, 50, 50));
    QVERIFY(item1->isObscured(0, 50, 50, 50));
    QVERIFY(!item2->isObscured());
    QVERIFY(!item2->isObscuredBy(item1));
    QVERIFY(!item2->isObscured(QRectF(0, 0, 50, 50)));
    QVERIFY(!item2->isObscured(QRectF(50, 0, 50, 50)));
    QVERIFY(!item2->isObscured(QRectF(50, 50, 50, 50)));
    QVERIFY(!item2->isObscured(QRectF(0, 50, 50, 50)));
    QVERIFY(!item2->isObscured(0, 0, 50, 50));
    QVERIFY(!item2->isObscured(50, 0, 50, 50));
    QVERIFY(!item2->isObscured(50, 50, 50, 50));
    QVERIFY(!item2->isObscured(0, 50, 50, 50));

    item2->moveBy(50, 0);

    QVERIFY(!item1->isObscured());
    QVERIFY(!item1->isObscuredBy(item2));
    QVERIFY(!item1->isObscured(QRectF(0, 0, 50, 50)));
    QVERIFY(item1->isObscured(QRectF(50, 0, 50, 50)));
    QVERIFY(item1->isObscured(QRectF(50, 50, 50, 50)));
    QVERIFY(!item1->isObscured(QRectF(0, 50, 50, 50)));
    QVERIFY(!item1->isObscured(0, 0, 50, 50));
    QVERIFY(item1->isObscured(50, 0, 50, 50));
    QVERIFY(item1->isObscured(50, 50, 50, 50));
    QVERIFY(!item1->isObscured(0, 50, 50, 50));
    QVERIFY(!item2->isObscured());
    QVERIFY(!item2->isObscuredBy(item1));
    QVERIFY(!item2->isObscured(QRectF(0, 0, 50, 50)));
    QVERIFY(!item2->isObscured(QRectF(50, 0, 50, 50)));
    QVERIFY(!item2->isObscured(QRectF(50, 50, 50, 50)));
    QVERIFY(!item2->isObscured(QRectF(0, 50, 50, 50)));
    QVERIFY(!item2->isObscured(0, 0, 50, 50));
    QVERIFY(!item2->isObscured(50, 0, 50, 50));
    QVERIFY(!item2->isObscured(50, 50, 50, 50));
    QVERIFY(!item2->isObscured(0, 50, 50, 50));
}

void tst_QGraphicsItem::mapFromToParent()
{
    QPainterPath path1;
    path1.addRect(0, 0, 200, 200);

    QPainterPath path2;
    path2.addRect(0, 0, 100, 100);

    QPainterPath path3;
    path3.addRect(0, 0, 50, 50);

    QPainterPath path4;
    path4.addRect(0, 0, 25, 25);

    QGraphicsItem *item1 = new QGraphicsPathItem(path1);
    QGraphicsItem *item2 = new QGraphicsPathItem(path2, item1);
    QGraphicsItem *item3 = new QGraphicsPathItem(path3, item2);
    QGraphicsItem *item4 = new QGraphicsPathItem(path4, item3);

    item1->setPos(10, 10);
    item2->setPos(10, 10);
    item3->setPos(10, 10);
    item4->setPos(10, 10);

    for (int i = 0; i < 4; ++i) {
        QMatrix matrix;
        matrix.rotate(i * 90);
        matrix.translate(i * 100, -i * 100);
        matrix.scale(2, 4);
        item1->setMatrix(matrix);

        QCOMPARE(item1->mapToParent(QPointF(0, 0)), item1->pos() + matrix.map(QPointF(0, 0)));
        QCOMPARE(item2->mapToParent(QPointF(0, 0)), item2->pos());
        QCOMPARE(item3->mapToParent(QPointF(0, 0)), item3->pos());
        QCOMPARE(item4->mapToParent(QPointF(0, 0)), item4->pos());
        QCOMPARE(item1->mapToParent(QPointF(10, -10)), item1->pos() + matrix.map(QPointF(10, -10)));
        QCOMPARE(item2->mapToParent(QPointF(10, -10)), item2->pos() + QPointF(10, -10));
        QCOMPARE(item3->mapToParent(QPointF(10, -10)), item3->pos() + QPointF(10, -10));
        QCOMPARE(item4->mapToParent(QPointF(10, -10)), item4->pos() + QPointF(10, -10));
        QCOMPARE(item1->mapToParent(QPointF(-10, 10)), item1->pos() + matrix.map(QPointF(-10, 10)));
        QCOMPARE(item2->mapToParent(QPointF(-10, 10)), item2->pos() + QPointF(-10, 10));
        QCOMPARE(item3->mapToParent(QPointF(-10, 10)), item3->pos() + QPointF(-10, 10));
        QCOMPARE(item4->mapToParent(QPointF(-10, 10)), item4->pos() + QPointF(-10, 10));
        QCOMPARE(item1->mapFromParent(item1->pos()), matrix.inverted().map(QPointF(0, 0)));
        QCOMPARE(item2->mapFromParent(item2->pos()), QPointF(0, 0));
        QCOMPARE(item3->mapFromParent(item3->pos()), QPointF(0, 0));
        QCOMPARE(item4->mapFromParent(item4->pos()), QPointF(0, 0));
        QCOMPARE(item1->mapFromParent(item1->pos() + QPointF(10, -10)),
                 matrix.inverted().map(QPointF(10, -10)));
        QCOMPARE(item2->mapFromParent(item2->pos() + QPointF(10, -10)), QPointF(10, -10));
        QCOMPARE(item3->mapFromParent(item3->pos() + QPointF(10, -10)), QPointF(10, -10));
        QCOMPARE(item4->mapFromParent(item4->pos() + QPointF(10, -10)), QPointF(10, -10));
        QCOMPARE(item1->mapFromParent(item1->pos() + QPointF(-10, 10)),
                 matrix.inverted().map(QPointF(-10, 10)));
        QCOMPARE(item2->mapFromParent(item2->pos() + QPointF(-10, 10)), QPointF(-10, 10));
        QCOMPARE(item3->mapFromParent(item3->pos() + QPointF(-10, 10)), QPointF(-10, 10));
        QCOMPARE(item4->mapFromParent(item4->pos() + QPointF(-10, 10)), QPointF(-10, 10));
    }

    delete item1;
}

void tst_QGraphicsItem::mapFromToScene()
{
    QGraphicsItem *item1 = new QGraphicsPathItem(QPainterPath());
    QGraphicsItem *item2 = new QGraphicsPathItem(QPainterPath(), item1);
    QGraphicsItem *item3 = new QGraphicsPathItem(QPainterPath(), item2);
    QGraphicsItem *item4 = new QGraphicsPathItem(QPainterPath(), item3);

    item1->setPos(100, 100);
    item2->setPos(100, 100);
    item3->setPos(100, 100);
    item4->setPos(100, 100);
    QCOMPARE(item1->pos(), QPointF(100, 100));
    QCOMPARE(item2->pos(), QPointF(100, 100));
    QCOMPARE(item3->pos(), QPointF(100, 100));
    QCOMPARE(item4->pos(), QPointF(100, 100));
    QCOMPARE(item1->pos(), item1->mapToParent(0, 0));
    QCOMPARE(item2->pos(), item2->mapToParent(0, 0));
    QCOMPARE(item3->pos(), item3->mapToParent(0, 0));
    QCOMPARE(item4->pos(), item4->mapToParent(0, 0));
    QCOMPARE(item1->mapToParent(10, 10), QPointF(110, 110));
    QCOMPARE(item2->mapToParent(10, 10), QPointF(110, 110));
    QCOMPARE(item3->mapToParent(10, 10), QPointF(110, 110));
    QCOMPARE(item4->mapToParent(10, 10), QPointF(110, 110));
    QCOMPARE(item1->mapToScene(0, 0), QPointF(100, 100));
    QCOMPARE(item2->mapToScene(0, 0), QPointF(200, 200));
    QCOMPARE(item3->mapToScene(0, 0), QPointF(300, 300));
    QCOMPARE(item4->mapToScene(0, 0), QPointF(400, 400));
    QCOMPARE(item1->mapToScene(10, 0), QPointF(110, 100));
    QCOMPARE(item2->mapToScene(10, 0), QPointF(210, 200));
    QCOMPARE(item3->mapToScene(10, 0), QPointF(310, 300));
    QCOMPARE(item4->mapToScene(10, 0), QPointF(410, 400));
    QCOMPARE(item1->mapFromScene(100, 100), QPointF(0, 0));
    QCOMPARE(item2->mapFromScene(200, 200), QPointF(0, 0));
    QCOMPARE(item3->mapFromScene(300, 300), QPointF(0, 0));
    QCOMPARE(item4->mapFromScene(400, 400), QPointF(0, 0));
    QCOMPARE(item1->mapFromScene(110, 100), QPointF(10, 0));
    QCOMPARE(item2->mapFromScene(210, 200), QPointF(10, 0));
    QCOMPARE(item3->mapFromScene(310, 300), QPointF(10, 0));
    QCOMPARE(item4->mapFromScene(410, 400), QPointF(10, 0));

    // Rotate item1 90 degrees clockwise
    QMatrix matrix; matrix.rotate(90);
    item1->setMatrix(matrix);
    QCOMPARE(item1->pos(), item1->mapToParent(0, 0));
    QCOMPARE(item2->pos(), item2->mapToParent(0, 0));
    QCOMPARE(item3->pos(), item3->mapToParent(0, 0));
    QCOMPARE(item4->pos(), item4->mapToParent(0, 0));
    QCOMPARE(item1->mapToParent(10, 0), QPointF(100, 110));
    QCOMPARE(item2->mapToParent(10, 0), QPointF(110, 100));
    QCOMPARE(item3->mapToParent(10, 0), QPointF(110, 100));
    QCOMPARE(item4->mapToParent(10, 0), QPointF(110, 100));
    QCOMPARE(item1->mapToScene(0, 0), QPointF(100, 100));
    QCOMPARE(item2->mapToScene(0, 0), QPointF(0, 200));
    QCOMPARE(item3->mapToScene(0, 0), QPointF(-100, 300));
    QCOMPARE(item4->mapToScene(0, 0), QPointF(-200, 400));
    QCOMPARE(item1->mapToScene(10, 0), QPointF(100, 110));
    QCOMPARE(item2->mapToScene(10, 0), QPointF(0, 210));
    QCOMPARE(item3->mapToScene(10, 0), QPointF(-100, 310));
    QCOMPARE(item4->mapToScene(10, 0), QPointF(-200, 410));
    QCOMPARE(item1->mapFromScene(100, 100), QPointF(0, 0));
    QCOMPARE(item2->mapFromScene(0, 200), QPointF(0, 0));
    QCOMPARE(item3->mapFromScene(-100, 300), QPointF(0, 0));
    QCOMPARE(item4->mapFromScene(-200, 400), QPointF(0, 0));
    QCOMPARE(item1->mapFromScene(100, 110), QPointF(10, 0));
    QCOMPARE(item2->mapFromScene(0, 210), QPointF(10, 0));
    QCOMPARE(item3->mapFromScene(-100, 310), QPointF(10, 0));
    QCOMPARE(item4->mapFromScene(-200, 410), QPointF(10, 0));

    // Rotate item2 90 degrees clockwise
    item2->setMatrix(matrix);
    QCOMPARE(item1->pos(), item1->mapToParent(0, 0));
    QCOMPARE(item2->pos(), item2->mapToParent(0, 0));
    QCOMPARE(item3->pos(), item3->mapToParent(0, 0));
    QCOMPARE(item4->pos(), item4->mapToParent(0, 0));
    QCOMPARE(item1->mapToParent(10, 0), QPointF(100, 110));
    QCOMPARE(item2->mapToParent(10, 0), QPointF(100, 110));
    QCOMPARE(item3->mapToParent(10, 0), QPointF(110, 100));
    QCOMPARE(item4->mapToParent(10, 0), QPointF(110, 100));
    QCOMPARE(item1->mapToScene(0, 0), QPointF(100, 100));
    QCOMPARE(item2->mapToScene(0, 0), QPointF(0, 200));
    QCOMPARE(item3->mapToScene(0, 0), QPointF(-100, 100));
    QCOMPARE(item4->mapToScene(0, 0), QPointF(-200, 0));
    QCOMPARE(item1->mapToScene(10, 0), QPointF(100, 110));
    QCOMPARE(item2->mapToScene(10, 0), QPointF(-10, 200));
    QCOMPARE(item3->mapToScene(10, 0), QPointF(-110, 100));
    QCOMPARE(item4->mapToScene(10, 0), QPointF(-210, 0));
    QCOMPARE(item1->mapFromScene(100, 100), QPointF(0, 0));
    QCOMPARE(item2->mapFromScene(0, 200), QPointF(0, 0));
    QCOMPARE(item3->mapFromScene(-100, 100), QPointF(0, 0));
    QCOMPARE(item4->mapFromScene(-200, 0), QPointF(0, 0));
    QCOMPARE(item1->mapFromScene(100, 110), QPointF(10, 0));
    QCOMPARE(item2->mapFromScene(-10, 200), QPointF(10, 0));
    QCOMPARE(item3->mapFromScene(-110, 100), QPointF(10, 0));
    QCOMPARE(item4->mapFromScene(-210, 0), QPointF(10, 0));

    // Translate item3 50 points, then rotate 90 degrees counterclockwise
    QMatrix matrix2;
    matrix2.translate(50, 0);
    matrix2.rotate(-90);
    item3->setMatrix(matrix2);
    QCOMPARE(item1->pos(), item1->mapToParent(0, 0));
    QCOMPARE(item2->pos(), item2->mapToParent(0, 0));
    QCOMPARE(item3->pos(), item3->mapToParent(0, 0) - QPointF(50, 0));
    QCOMPARE(item4->pos(), item4->mapToParent(0, 0));
    QCOMPARE(item1->mapToParent(10, 0), QPointF(100, 110));
    QCOMPARE(item2->mapToParent(10, 0), QPointF(100, 110));
    QCOMPARE(item3->mapToParent(10, 0), QPointF(150, 90));
    QCOMPARE(item4->mapToParent(10, 0), QPointF(110, 100));
    QCOMPARE(item1->mapToScene(0, 0), QPointF(100, 100));
    QCOMPARE(item2->mapToScene(0, 0), QPointF(0, 200));
    QCOMPARE(item3->mapToScene(0, 0), QPointF(-150, 100));
    QCOMPARE(item4->mapToScene(0, 0), QPointF(-250, 200));
    QCOMPARE(item1->mapToScene(10, 0), QPointF(100, 110));
    QCOMPARE(item2->mapToScene(10, 0), QPointF(-10, 200));
    QCOMPARE(item3->mapToScene(10, 0), QPointF(-150, 110));
    QCOMPARE(item4->mapToScene(10, 0), QPointF(-250, 210));
    QCOMPARE(item1->mapFromScene(100, 100), QPointF(0, 0));
    QCOMPARE(item2->mapFromScene(0, 200), QPointF(0, 0));
    QCOMPARE(item3->mapFromScene(-150, 100), QPointF(0, 0));
    QCOMPARE(item4->mapFromScene(-250, 200), QPointF(0, 0));
    QCOMPARE(item1->mapFromScene(100, 110), QPointF(10, 0));
    QCOMPARE(item2->mapFromScene(-10, 200), QPointF(10, 0));
    QCOMPARE(item3->mapFromScene(-150, 110), QPointF(10, 0));
    QCOMPARE(item4->mapFromScene(-250, 210), QPointF(10, 0));

    delete item1;
}

void tst_QGraphicsItem::mapFromToItem()
{
    QGraphicsItem *item1 = new QGraphicsPathItem;
    QGraphicsItem *item2 = new QGraphicsPathItem;
    QGraphicsItem *item3 = new QGraphicsPathItem;
    QGraphicsItem *item4 = new QGraphicsPathItem;

    item1->setPos(-100, -100);
    item2->setPos(100, -100);
    item3->setPos(100, 100);
    item4->setPos(-100, 100);

    QCOMPARE(item1->mapFromItem(item2, 0, 0), QPointF(200, 0));
    QCOMPARE(item2->mapFromItem(item3, 0, 0), QPointF(0, 200));
    QCOMPARE(item3->mapFromItem(item4, 0, 0), QPointF(-200, 0));
    QCOMPARE(item4->mapFromItem(item1, 0, 0), QPointF(0, -200));
    QCOMPARE(item1->mapFromItem(item4, 0, 0), QPointF(0, 200));
    QCOMPARE(item2->mapFromItem(item1, 0, 0), QPointF(-200, 0));
    QCOMPARE(item3->mapFromItem(item2, 0, 0), QPointF(0, -200));
    QCOMPARE(item4->mapFromItem(item3, 0, 0), QPointF(200, 0));

    QMatrix matrix;
    matrix.translate(100, 100);
    item1->setMatrix(matrix);

    QCOMPARE(item1->mapFromItem(item2, 0, 0), QPointF(100, -100));
    QCOMPARE(item2->mapFromItem(item3, 0, 0), QPointF(0, 200));
    QCOMPARE(item3->mapFromItem(item4, 0, 0), QPointF(-200, 0));
    QCOMPARE(item4->mapFromItem(item1, 0, 0), QPointF(100, -100));
    QCOMPARE(item1->mapFromItem(item4, 0, 0), QPointF(-100, 100));
    QCOMPARE(item2->mapFromItem(item1, 0, 0), QPointF(-100, 100));
    QCOMPARE(item3->mapFromItem(item2, 0, 0), QPointF(0, -200));
    QCOMPARE(item4->mapFromItem(item3, 0, 0), QPointF(200, 0));

    matrix.rotate(90);
    item1->setMatrix(matrix);
    item2->setMatrix(matrix);
    item3->setMatrix(matrix);
    item4->setMatrix(matrix);

    QCOMPARE(item1->mapFromItem(item2, 0, 0), QPointF(0, -200));
    QCOMPARE(item2->mapFromItem(item3, 0, 0), QPointF(200, 0));
    QCOMPARE(item3->mapFromItem(item4, 0, 0), QPointF(0, 200));
    QCOMPARE(item4->mapFromItem(item1, 0, 0), QPointF(-200, 0));
    QCOMPARE(item1->mapFromItem(item4, 0, 0), QPointF(200, 0));
    QCOMPARE(item2->mapFromItem(item1, 0, 0), QPointF(0, 200));
    QCOMPARE(item3->mapFromItem(item2, 0, 0), QPointF(-200, 0));
    QCOMPARE(item4->mapFromItem(item3, 0, 0), QPointF(0, -200));
    QCOMPARE(item1->mapFromItem(item2, 10, -5), QPointF(10, -205));
    QCOMPARE(item2->mapFromItem(item3, 10, -5), QPointF(210, -5));
    QCOMPARE(item3->mapFromItem(item4, 10, -5), QPointF(10, 195));
    QCOMPARE(item4->mapFromItem(item1, 10, -5), QPointF(-190, -5));
    QCOMPARE(item1->mapFromItem(item4, 10, -5), QPointF(210, -5));
    QCOMPARE(item2->mapFromItem(item1, 10, -5), QPointF(10, 195));
    QCOMPARE(item3->mapFromItem(item2, 10, -5), QPointF(-190, -5));
    QCOMPARE(item4->mapFromItem(item3, 10, -5), QPointF(10, -205));

    QCOMPARE(item1->mapFromItem(0, 10, -5), item1->mapFromScene(10, -5));
    QCOMPARE(item2->mapFromItem(0, 10, -5), item2->mapFromScene(10, -5));
    QCOMPARE(item3->mapFromItem(0, 10, -5), item3->mapFromScene(10, -5));
    QCOMPARE(item4->mapFromItem(0, 10, -5), item4->mapFromScene(10, -5));
    QCOMPARE(item1->mapToItem(0, 10, -5), item1->mapToScene(10, -5));
    QCOMPARE(item2->mapToItem(0, 10, -5), item2->mapToScene(10, -5));
    QCOMPARE(item3->mapToItem(0, 10, -5), item3->mapToScene(10, -5));
    QCOMPARE(item4->mapToItem(0, 10, -5), item4->mapToScene(10, -5));

    delete item1;
    delete item2;
    delete item3;
    delete item4;
}

void tst_QGraphicsItem::mapRectFromToParent_data()
{
    QTest::addColumn<bool>("parent");
    QTest::addColumn<QPointF>("parentPos");
    QTest::addColumn<QTransform>("parentTransform");
    QTest::addColumn<QPointF>("pos");
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<QRectF>("inputRect");
    QTest::addColumn<QRectF>("outputRect");

    QTest::newRow("nil") << false << QPointF() << QTransform() << QPointF() << QTransform() << QRectF() << QRectF();
    QTest::newRow("simple") << false << QPointF() << QTransform() << QPointF() << QTransform()
                            << QRectF(0, 0, 10, 10) << QRectF(0, 0, 10, 10);
    QTest::newRow("simple w/parent") << true
                                     << QPointF() << QTransform()
                                     << QPointF() << QTransform()
                                     << QRectF(0, 0, 10, 10) << QRectF(0, 0, 10, 10);
    QTest::newRow("simple w/parent parentPos") << true
                                               << QPointF(50, 50) << QTransform()
                                               << QPointF() << QTransform()
                                               << QRectF(0, 0, 10, 10) << QRectF(0, 0, 10, 10);
    QTest::newRow("simple w/parent parentPos parentRotation") << true
                                                              << QPointF(50, 50) << QTransform().rotate(45)
                                                              << QPointF() << QTransform()
                                                              << QRectF(0, 0, 10, 10) << QRectF(0, 0, 10, 10);
    QTest::newRow("pos w/parent") << true
                                  << QPointF() << QTransform()
                                  << QPointF(50, 50) << QTransform()
                                  << QRectF(0, 0, 10, 10) << QRectF(50, 50, 10, 10);
    QTest::newRow("rotation w/parent") << true
                                       << QPointF() << QTransform()
                                       << QPointF() << QTransform().rotate(90)
                                       << QRectF(0, 0, 10, 10) << QRectF(-10, 0, 10, 10);
    QTest::newRow("pos rotation w/parent") << true
                                           << QPointF() << QTransform()
                                           << QPointF(50, 50) << QTransform().rotate(90)
                                           << QRectF(0, 0, 10, 10) << QRectF(40, 50, 10, 10);
    QTest::newRow("pos rotation w/parent parentPos parentRotation") << true
                                                                    << QPointF(-170, -190) << QTransform().rotate(90)
                                                                    << QPointF(50, 50) << QTransform().rotate(90)
                                                                    << QRectF(0, 0, 10, 10) << QRectF(40, 50, 10, 10);
}

void tst_QGraphicsItem::mapRectFromToParent()
{
    QFETCH(bool, parent);
    QFETCH(QPointF, parentPos);
    QFETCH(QTransform, parentTransform);
    QFETCH(QPointF, pos);
    QFETCH(QTransform, transform);
    QFETCH(QRectF, inputRect);
    QFETCH(QRectF, outputRect);

    QGraphicsRectItem *rect = new QGraphicsRectItem;
    rect->setPos(pos);
    rect->setTransform(transform);

    if (parent) {
        QGraphicsRectItem *rectParent = new QGraphicsRectItem;
        rect->setParentItem(rectParent);
        rectParent->setPos(parentPos);
        rectParent->setTransform(parentTransform);
    }

    // Make sure we use non-destructive transform operations (e.g., 90 degree
    // rotations).
    QCOMPARE(rect->mapRectToParent(inputRect), outputRect);
    QCOMPARE(rect->mapRectFromParent(outputRect), inputRect);
    QCOMPARE(rect->itemTransform(rect->parentItem()).mapRect(inputRect), outputRect);
    QCOMPARE(rect->mapToParent(inputRect).boundingRect(), outputRect);
    QCOMPARE(rect->mapToParent(QPolygonF(inputRect)).boundingRect(), outputRect);
    QCOMPARE(rect->mapFromParent(outputRect).boundingRect(), inputRect);
    QCOMPARE(rect->mapFromParent(QPolygonF(outputRect)).boundingRect(), inputRect);
    QPainterPath inputPath;
    inputPath.addRect(inputRect);
    QPainterPath outputPath;
    outputPath.addRect(outputRect);
    QCOMPARE(rect->mapToParent(inputPath).boundingRect(), outputPath.boundingRect());
    QCOMPARE(rect->mapFromParent(outputPath).boundingRect(), inputPath.boundingRect());
}

void tst_QGraphicsItem::isAncestorOf()
{
    QGraphicsItem *grandPa = new QGraphicsRectItem;
    QGraphicsItem *parent = new QGraphicsRectItem;
    QGraphicsItem *child = new QGraphicsRectItem;

    QVERIFY(!parent->isAncestorOf(0));
    QVERIFY(!child->isAncestorOf(0));
    QVERIFY(!parent->isAncestorOf(child));
    QVERIFY(!child->isAncestorOf(parent));
    QVERIFY(!parent->isAncestorOf(parent));

    child->setParentItem(parent);
    parent->setParentItem(grandPa);

    QVERIFY(parent->isAncestorOf(child));
    QVERIFY(grandPa->isAncestorOf(parent));
    QVERIFY(grandPa->isAncestorOf(child));
    QVERIFY(!child->isAncestorOf(parent));
    QVERIFY(!parent->isAncestorOf(grandPa));
    QVERIFY(!child->isAncestorOf(grandPa));
    QVERIFY(!child->isAncestorOf(child));
    QVERIFY(!parent->isAncestorOf(parent));
    QVERIFY(!grandPa->isAncestorOf(grandPa));

    parent->setParentItem(0);

    delete child;
    delete parent;
    delete grandPa;
}

void tst_QGraphicsItem::commonAncestorItem()
{
    QGraphicsItem *ancestor = new QGraphicsRectItem;
    QGraphicsItem *grandMa = new QGraphicsRectItem;
    QGraphicsItem *grandPa = new QGraphicsRectItem;
    QGraphicsItem *brotherInLaw = new QGraphicsRectItem;
    QGraphicsItem *cousin = new QGraphicsRectItem;
    QGraphicsItem *husband = new QGraphicsRectItem;
    QGraphicsItem *child = new QGraphicsRectItem;
    QGraphicsItem *wife = new QGraphicsRectItem;

    child->setParentItem(husband);
    husband->setParentItem(grandPa);
    brotherInLaw->setParentItem(grandPa);
    cousin->setParentItem(brotherInLaw);
    wife->setParentItem(grandMa);
    grandMa->setParentItem(ancestor);
    grandPa->setParentItem(ancestor);

    QCOMPARE(grandMa->commonAncestorItem(grandMa), grandMa);
    QCOMPARE(grandMa->commonAncestorItem(0), (QGraphicsItem *)0);
    QCOMPARE(grandMa->commonAncestorItem(grandPa), ancestor);
    QCOMPARE(grandPa->commonAncestorItem(grandMa), ancestor);
    QCOMPARE(grandPa->commonAncestorItem(husband), grandPa);
    QCOMPARE(grandPa->commonAncestorItem(wife), ancestor);
    QCOMPARE(grandMa->commonAncestorItem(husband), ancestor);
    QCOMPARE(grandMa->commonAncestorItem(wife), grandMa);
    QCOMPARE(wife->commonAncestorItem(grandMa), grandMa);
    QCOMPARE(child->commonAncestorItem(cousin), grandPa);
    QCOMPARE(cousin->commonAncestorItem(child), grandPa);
    QCOMPARE(wife->commonAncestorItem(child), ancestor);
    QCOMPARE(child->commonAncestorItem(wife), ancestor);
}

void tst_QGraphicsItem::data()
{
    QGraphicsTextItem text;

    QCOMPARE(text.data(0), QVariant());
    text.setData(0, "TextItem");
    QCOMPARE(text.data(0), QVariant(QString("TextItem")));
    text.setData(0, QVariant());
    QCOMPARE(text.data(0), QVariant());
}

void tst_QGraphicsItem::type()
{
    QCOMPARE(int(QGraphicsItem::Type), 1);
    QCOMPARE(int(QGraphicsPathItem::Type), 2);
    QCOMPARE(int(QGraphicsRectItem::Type), 3);
    QCOMPARE(int(QGraphicsEllipseItem::Type), 4);
    QCOMPARE(int(QGraphicsPolygonItem::Type), 5);
    QCOMPARE(int(QGraphicsLineItem::Type), 6);
    QCOMPARE(int(QGraphicsPixmapItem::Type), 7);
    QCOMPARE(int(QGraphicsTextItem::Type), 8);

    QCOMPARE(QGraphicsPathItem().type(), 2);
    QCOMPARE(QGraphicsRectItem().type(), 3);
    QCOMPARE(QGraphicsEllipseItem().type(), 4);
    QCOMPARE(QGraphicsPolygonItem().type(), 5);
    QCOMPARE(QGraphicsLineItem().type(), 6);
    QCOMPARE(QGraphicsPixmapItem().type(), 7);
    QCOMPARE(QGraphicsTextItem().type(), 8);
}

void tst_QGraphicsItem::graphicsitem_cast()
{
    QGraphicsPathItem pathItem;
    const QGraphicsPathItem *pPathItem = &pathItem;
    QGraphicsRectItem rectItem;
    const QGraphicsRectItem *pRectItem = &rectItem;
    QGraphicsEllipseItem ellipseItem;
    const QGraphicsEllipseItem *pEllipseItem = &ellipseItem;
    QGraphicsPolygonItem polygonItem;
    const QGraphicsPolygonItem *pPolygonItem = &polygonItem;
    QGraphicsLineItem lineItem;
    const QGraphicsLineItem *pLineItem = &lineItem;
    QGraphicsPixmapItem pixmapItem;
    const QGraphicsPixmapItem *pPixmapItem = &pixmapItem;
    QGraphicsTextItem textItem;
    const QGraphicsTextItem *pTextItem = &textItem;

    QVERIFY(qgraphicsitem_cast<QGraphicsPathItem *>(&pathItem));
    //QVERIFY(qgraphicsitem_cast<QAbstractGraphicsPathItem *>(&pathItem));
    QVERIFY(qgraphicsitem_cast<QGraphicsItem *>(&pathItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsItem *>(pPathItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsPathItem *>(pPathItem));

    QVERIFY(qgraphicsitem_cast<QGraphicsRectItem *>(&rectItem));
    QVERIFY(qgraphicsitem_cast<QGraphicsItem *>(&rectItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsItem *>(pRectItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsRectItem *>(pRectItem));

    QVERIFY(qgraphicsitem_cast<QGraphicsEllipseItem *>(&ellipseItem));
    QVERIFY(qgraphicsitem_cast<QGraphicsItem *>(&ellipseItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsItem *>(pEllipseItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsEllipseItem *>(pEllipseItem));

    QVERIFY(qgraphicsitem_cast<QGraphicsPolygonItem *>(&polygonItem));
    //QVERIFY(qgraphicsitem_cast<QAbstractGraphicsPathItem *>(&polygonItem));
    QVERIFY(qgraphicsitem_cast<QGraphicsItem *>(&polygonItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsItem *>(pPolygonItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsPolygonItem *>(pPolygonItem));

    QVERIFY(qgraphicsitem_cast<QGraphicsLineItem *>(&lineItem));
    QVERIFY(qgraphicsitem_cast<QGraphicsItem *>(&lineItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsItem *>(pLineItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsLineItem *>(pLineItem));

    QVERIFY(qgraphicsitem_cast<QGraphicsPixmapItem *>(&pixmapItem));
    QVERIFY(qgraphicsitem_cast<QGraphicsItem *>(&pixmapItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsItem *>(pPixmapItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsPixmapItem *>(pPixmapItem));

    QVERIFY(qgraphicsitem_cast<QGraphicsTextItem *>(&textItem));
    QVERIFY(qgraphicsitem_cast<QGraphicsItem *>(&textItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsItem *>(pTextItem));
    QVERIFY(qgraphicsitem_cast<const QGraphicsTextItem *>(pTextItem));

    // and some casts that _should_ fail:
    QVERIFY(!qgraphicsitem_cast<QGraphicsEllipseItem *>(&pathItem));
    QVERIFY(!qgraphicsitem_cast<const QGraphicsTextItem *>(pPolygonItem));

    // and this shouldn't crash
    QGraphicsItem *ptr = 0;
    QVERIFY(!qgraphicsitem_cast<QGraphicsTextItem *>(ptr));
    QVERIFY(!qgraphicsitem_cast<QGraphicsItem *>(ptr));
}

void tst_QGraphicsItem::hoverEventsGenerateRepaints()
{
    Q_CHECK_PAINTEVENTS

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    EventTester *tester = new EventTester;
    scene.addItem(tester);
    tester->setAcceptsHoverEvents(true);

    QTRY_COMPARE(tester->repaints, 1);

    // Send a hover enter event
    QGraphicsSceneHoverEvent hoverEnterEvent(QEvent::GraphicsSceneHoverEnter);
    hoverEnterEvent.setScenePos(QPointF(0, 0));
    hoverEnterEvent.setPos(QPointF(0, 0));
    QApplication::sendEvent(&scene, &hoverEnterEvent);

    // Check that we get a repaint
    int npaints = tester->repaints;
    qApp->processEvents();
    qApp->processEvents();
    QCOMPARE(tester->events.size(), 2); //  enter + move
    QCOMPARE(tester->repaints, npaints + 1);
    QCOMPARE(tester->events.last(), QEvent::GraphicsSceneHoverMove);

    // Send a hover move event
    QGraphicsSceneHoverEvent hoverMoveEvent(QEvent::GraphicsSceneHoverMove);
    hoverMoveEvent.setScenePos(QPointF(0, 0));
    hoverMoveEvent.setPos(QPointF(0, 0));
    QApplication::sendEvent(&scene, &hoverMoveEvent);

    // Check that we don't get a repaint
    qApp->processEvents();
    qApp->processEvents();

    QCOMPARE(tester->events.size(), 3);
    QCOMPARE(tester->repaints, npaints + 1);
    QCOMPARE(tester->events.last(), QEvent::GraphicsSceneHoverMove);

    // Send a hover leave event
    QGraphicsSceneHoverEvent hoverLeaveEvent(QEvent::GraphicsSceneHoverLeave);
    hoverLeaveEvent.setScenePos(QPointF(-100, -100));
    hoverLeaveEvent.setPos(QPointF(0, 0));
    QApplication::sendEvent(&scene, &hoverLeaveEvent);

    // Check that we get a repaint
    qApp->processEvents();
    qApp->processEvents();

    QCOMPARE(tester->events.size(), 4);
    QCOMPARE(tester->repaints, npaints + 2);
    QCOMPARE(tester->events.last(), QEvent::GraphicsSceneHoverLeave);
}

void tst_QGraphicsItem::boundingRects_data()
{
    QTest::addColumn<QGraphicsItem *>("item");
    QTest::addColumn<QRectF>("boundingRect");

    QRectF rect(0, 0, 100, 100);
    QPainterPath path;
    path.addRect(rect);

    QRectF adjustedRect(-0.5, -0.5, 101, 101);

    QTest::newRow("path") << (QGraphicsItem *)new QGraphicsPathItem(path) << adjustedRect;
    QTest::newRow("rect") << (QGraphicsItem *)new QGraphicsRectItem(rect) << adjustedRect;
    QTest::newRow("ellipse") << (QGraphicsItem *)new QGraphicsEllipseItem(rect) << adjustedRect;
    QTest::newRow("polygon") << (QGraphicsItem *)new QGraphicsPolygonItem(rect) << adjustedRect;
}

void tst_QGraphicsItem::boundingRects()
{
    QFETCH(QGraphicsItem *, item);
    QFETCH(QRectF, boundingRect);

    ((QAbstractGraphicsShapeItem *)item)->setPen(QPen(Qt::black, 1));
    QCOMPARE(item->boundingRect(), boundingRect);
}

void tst_QGraphicsItem::boundingRects2()
{
    QGraphicsPixmapItem pixmap(QPixmap::fromImage(QImage(100, 100, QImage::Format_ARGB32_Premultiplied)));
    QCOMPARE(pixmap.boundingRect(), QRectF(0, 0, 100, 100));

    QGraphicsLineItem line(0, 0, 100, 0);
    line.setPen(QPen(Qt::black, 1));
    QCOMPARE(line.boundingRect(), QRectF(-0.5, -0.5, 101, 1));
}

void tst_QGraphicsItem::sceneBoundingRect()
{
    QGraphicsScene scene;
    QGraphicsRectItem *item = scene.addRect(QRectF(0, 0, 100, 100), QPen(Qt::black, 0));
    item->setPos(100, 100);

    QCOMPARE(item->boundingRect(), QRectF(0, 0, 100, 100));
    QCOMPARE(item->sceneBoundingRect(), QRectF(100, 100, 100, 100));

    item->rotate(90);

    QCOMPARE(item->boundingRect(), QRectF(0, 0, 100, 100));
    QCOMPARE(item->sceneBoundingRect(), QRectF(0, 100, 100, 100));
}

void tst_QGraphicsItem::childrenBoundingRect()
{
    QGraphicsScene scene;
    QGraphicsRectItem *parent = scene.addRect(QRectF(0, 0, 100, 100), QPen(Qt::black, 0));
    QGraphicsRectItem *child = scene.addRect(QRectF(0, 0, 100, 100), QPen(Qt::black, 0));
    child->setParentItem(parent);
    parent->setPos(100, 100);
    child->setPos(100, 100);

    QCOMPARE(parent->boundingRect(), QRectF(0, 0, 100, 100));
    QCOMPARE(child->boundingRect(), QRectF(0, 0, 100, 100));
    QCOMPARE(child->childrenBoundingRect(), QRectF());
    QCOMPARE(parent->childrenBoundingRect(), QRectF(100, 100, 100, 100));

    QGraphicsRectItem *child2 = scene.addRect(QRectF(0, 0, 100, 100), QPen(Qt::black, 0));
    child2->setParentItem(parent);
    child2->setPos(-100, -100);
    QCOMPARE(parent->childrenBoundingRect(), QRectF(-100, -100, 300, 300));

    QGraphicsRectItem *childChild = scene.addRect(QRectF(0, 0, 100, 100), QPen(Qt::black, 0));
    childChild->setParentItem(child);
    childChild->setPos(500, 500);
    child->rotate(90);

    scene.addPolygon(parent->mapToScene(parent->boundingRect() | parent->childrenBoundingRect()))->setPen(QPen(Qt::red));;

    QGraphicsView view(&scene);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(30);

    QCOMPARE(parent->childrenBoundingRect(), QRectF(-500, -100, 600, 800));
}

void tst_QGraphicsItem::childrenBoundingRectTransformed()
{
    QGraphicsScene scene;

    QGraphicsRectItem *rect = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *rect2 = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *rect3 = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *rect4 = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *rect5 = scene.addRect(QRectF(0, 0, 100, 100));
    rect2->setParentItem(rect);
    rect3->setParentItem(rect2);
    rect4->setParentItem(rect3);
    rect5->setParentItem(rect4);

    rect->setPen(QPen(Qt::black, 0));
    rect2->setPen(QPen(Qt::black, 0));
    rect3->setPen(QPen(Qt::black, 0));
    rect4->setPen(QPen(Qt::black, 0));
    rect5->setPen(QPen(Qt::black, 0));

    rect2->setTransform(QTransform().translate(50, 50).rotate(45));
    rect2->setPos(25, 25);
    rect3->setTransform(QTransform().translate(50, 50).rotate(45));
    rect3->setPos(25, 25);
    rect4->setTransform(QTransform().translate(50, 50).rotate(45));
    rect4->setPos(25, 25);
    rect5->setTransform(QTransform().translate(50, 50).rotate(45));
    rect5->setPos(25, 25);

    QRectF subTreeRect = rect->childrenBoundingRect();
    QCOMPARE(subTreeRect.left(), qreal(-206.0660171779821));
    QCOMPARE(subTreeRect.top(), qreal(75.0));
    QCOMPARE(subTreeRect.width(), qreal(351.7766952966369));
    QCOMPARE(subTreeRect.height(), qreal(251.7766952966369));

    rect->rotate(45);
    rect2->rotate(-45);
    rect3->rotate(45);
    rect4->rotate(-45);
    rect5->rotate(45);

    subTreeRect = rect->childrenBoundingRect();
    QCOMPARE(rect->childrenBoundingRect(), QRectF(-100, 75, 275, 250));
}

void tst_QGraphicsItem::childrenBoundingRect2()
{
    QGraphicsItemGroup box;
    QGraphicsLineItem l1(0, 0, 100, 0, &box);
    QGraphicsLineItem l2(100, 0, 100, 100, &box);
    QGraphicsLineItem l3(0, 0, 0, 100, &box);
    // Make sure lines (zero with/height) are included in the childrenBoundingRect.
    l1.setPen(QPen(Qt::black, 0));
    l2.setPen(QPen(Qt::black, 0));
    l3.setPen(QPen(Qt::black, 0));
    QCOMPARE(box.childrenBoundingRect(), QRectF(0, 0, 100, 100));
}

void tst_QGraphicsItem::childrenBoundingRect3()
{
    QGraphicsScene scene;

    QGraphicsRectItem *rect = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *rect2 = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *rect3 = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *rect4 = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *rect5 = scene.addRect(QRectF(0, 0, 100, 100));
    rect2->setParentItem(rect);
    rect3->setParentItem(rect2);
    rect4->setParentItem(rect3);
    rect5->setParentItem(rect4);

    rect->setPen(QPen(Qt::black, 0));
    rect2->setPen(QPen(Qt::black, 0));
    rect3->setPen(QPen(Qt::black, 0));
    rect4->setPen(QPen(Qt::black, 0));
    rect5->setPen(QPen(Qt::black, 0));

    rect2->setTransform(QTransform().translate(50, 50).rotate(45));
    rect2->setPos(25, 25);
    rect3->setTransform(QTransform().translate(50, 50).rotate(45));
    rect3->setPos(25, 25);
    rect4->setTransform(QTransform().translate(50, 50).rotate(45));
    rect4->setPos(25, 25);
    rect5->setTransform(QTransform().translate(50, 50).rotate(45));
    rect5->setPos(25, 25);

    // Try to mess up the cached bounding rect.
    (void)rect2->childrenBoundingRect();

    QRectF subTreeRect = rect->childrenBoundingRect();
    QCOMPARE(subTreeRect.left(), qreal(-206.0660171779821));
    QCOMPARE(subTreeRect.top(), qreal(75.0));
    QCOMPARE(subTreeRect.width(), qreal(351.7766952966369));
    QCOMPARE(subTreeRect.height(), qreal(251.7766952966369));
}

void tst_QGraphicsItem::childrenBoundingRect4()
{
    QGraphicsScene scene;

    QGraphicsRectItem *rect = scene.addRect(QRectF(0, 0, 10, 10));
    QGraphicsRectItem *rect2 = scene.addRect(QRectF(0, 0, 20, 20));
    QGraphicsRectItem *rect3 = scene.addRect(QRectF(0, 0, 30, 30));
    rect2->setParentItem(rect);
    rect3->setParentItem(rect);

    QGraphicsView view(&scene);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // Try to mess up the cached bounding rect.
    rect->childrenBoundingRect();
    rect2->childrenBoundingRect();

    rect3->setOpacity(0.0);
    rect3->setParentItem(rect2);

    QCOMPARE(rect->childrenBoundingRect(), rect3->boundingRect());
    QCOMPARE(rect2->childrenBoundingRect(), rect3->boundingRect());
}

void tst_QGraphicsItem::childrenBoundingRect5()
{
    QGraphicsScene scene;

    QGraphicsRectItem *parent = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *child = scene.addRect(QRectF(0, 0, 100, 100));
    child->setParentItem(parent);

    parent->setPen(QPen(Qt::black, 0));
    child->setPen(QPen(Qt::black, 0));

    QGraphicsView view(&scene);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // Try to mess up the cached bounding rect.
    QRectF expectedChildrenBoundingRect = parent->boundingRect();
    QCOMPARE(parent->childrenBoundingRect(), expectedChildrenBoundingRect);

    // Apply some effects.
    QGraphicsDropShadowEffect *dropShadow = new QGraphicsDropShadowEffect;
    dropShadow->setOffset(25, 25);
    child->setGraphicsEffect(dropShadow);
    parent->setGraphicsEffect(new QGraphicsOpacityEffect);

    QVERIFY(parent->childrenBoundingRect() != expectedChildrenBoundingRect);
    expectedChildrenBoundingRect |= dropShadow->boundingRect();
    QCOMPARE(parent->childrenBoundingRect(), expectedChildrenBoundingRect);
}

void tst_QGraphicsItem::group()
{
    QGraphicsScene scene;
    QGraphicsRectItem *parent = scene.addRect(QRectF(0, 0, 50, 50), QPen(Qt::black, 0), QBrush(Qt::green));
    QGraphicsRectItem *child = scene.addRect(QRectF(0, 0, 50, 50), QPen(Qt::black, 0), QBrush(Qt::blue));
    QGraphicsRectItem *parent2 = scene.addRect(QRectF(0, 0, 50, 50), QPen(Qt::black, 0), QBrush(Qt::red));
    parent2->setPos(-50, 50);
    child->rotate(45);
    child->setParentItem(parent);
    parent->setPos(25, 25);
    child->setPos(25, 25);

    QCOMPARE(parent->group(), (QGraphicsItemGroup *)0);
    QCOMPARE(parent2->group(), (QGraphicsItemGroup *)0);
    QCOMPARE(child->group(), (QGraphicsItemGroup *)0);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QGraphicsItemGroup *group = new QGraphicsItemGroup;
    group->setSelected(true);
    scene.addItem(group);

    QRectF parentSceneBoundingRect = parent->sceneBoundingRect();
    group->addToGroup(parent);
    QCOMPARE(parent->group(), group);
    QCOMPARE(parent->sceneBoundingRect(), parentSceneBoundingRect);

    QCOMPARE(parent->parentItem(), (QGraphicsItem *)group);
    QCOMPARE(group->childItems().size(), 1);
    QCOMPARE(scene.items().size(), 4);
    QCOMPARE(scene.items(group->sceneBoundingRect()).size(), 3);

    QTest::qWait(25);

    QRectF parent2SceneBoundingRect = parent2->sceneBoundingRect();
    group->addToGroup(parent2);
    QCOMPARE(parent2->group(), group);
    QCOMPARE(parent2->sceneBoundingRect(), parent2SceneBoundingRect);

    QCOMPARE(parent2->parentItem(), (QGraphicsItem *)group);
    QCOMPARE(group->childItems().size(), 2);
    QCOMPARE(scene.items().size(), 4);
    QCOMPARE(scene.items(group->sceneBoundingRect()).size(), 4);

    QTest::qWait(25);

    QList<QGraphicsItem *> newItems;
    for (int i = 0; i < 100; ++i) {
        QGraphicsItem *item = scene.addRect(QRectF(-25, -25, 50, 50), QPen(Qt::black, 0),
                                            QBrush(QColor(rand() % 255, rand() % 255,
                                                          rand() % 255, rand() % 255)));
        newItems << item;
        item->setPos(-1000 + rand() % 2000,
                     -1000 + rand() % 2000);
        item->rotate(rand() % 90);
    }

    view.fitInView(scene.itemsBoundingRect());

    int n = 0;
    foreach (QGraphicsItem *item, newItems) {
        group->addToGroup(item);
        QCOMPARE(item->group(), group);
        if ((n++ % 100) == 0)
            QTest::qWait(10);
    }
}

void tst_QGraphicsItem::setGroup()
{
    QGraphicsItemGroup group1;
    QGraphicsItemGroup group2;

    QGraphicsRectItem *rect = new QGraphicsRectItem;
    QCOMPARE(rect->group(), (QGraphicsItemGroup *)0);
    QCOMPARE(rect->parentItem(), (QGraphicsItem *)0);
    rect->setGroup(&group1);
    QCOMPARE(rect->group(), &group1);
    QCOMPARE(rect->parentItem(), (QGraphicsItem *)&group1);
    rect->setGroup(&group2);
    QCOMPARE(rect->group(), &group2);
    QCOMPARE(rect->parentItem(), (QGraphicsItem *)&group2);
    rect->setGroup(0);
    QCOMPARE(rect->group(), (QGraphicsItemGroup *)0);
    QCOMPARE(rect->parentItem(), (QGraphicsItem *)0);
}

void tst_QGraphicsItem::setGroup2()
{
    QGraphicsScene scene;
    QGraphicsItemGroup group;
    scene.addItem(&group);

    QGraphicsRectItem *rect = scene.addRect(50,50,50,50,Qt::NoPen,Qt::black);
    rect->setTransformOriginPoint(50,50);
    rect->setRotation(45);
    rect->setScale(1.5);
    rect->translate(20,20);
    group.translate(-30,-40);
    group.setRotation(180);
    group.setScale(0.5);

    QTransform oldSceneTransform = rect->sceneTransform();
    rect->setGroup(&group);
    QCOMPARE(rect->sceneTransform(), oldSceneTransform);

    group.setRotation(20);
    group.setScale(2);
    rect->setRotation(90);
    rect->setScale(0.8);

    oldSceneTransform = rect->sceneTransform();
    rect->setGroup(0);
    qFuzzyCompare(rect->sceneTransform(), oldSceneTransform);
}

void tst_QGraphicsItem::nestedGroups()
{
    QGraphicsItemGroup *group1 = new QGraphicsItemGroup;
    QGraphicsItemGroup *group2 = new QGraphicsItemGroup;

    QGraphicsRectItem *rect = new QGraphicsRectItem;
    QGraphicsRectItem *rect2 = new QGraphicsRectItem;
    rect2->setParentItem(rect);

    group1->addToGroup(rect);
    QCOMPARE(rect->group(), group1);
    QCOMPARE(rect2->group(), group1);

    group2->addToGroup(group1);
    QCOMPARE(rect->group(), group1);
    QCOMPARE(rect2->group(), group1);
    QCOMPARE(group1->group(), group2);
    QCOMPARE(group2->group(), (QGraphicsItemGroup *)0);

    QGraphicsScene scene;
    scene.addItem(group1);

    QCOMPARE(rect->group(), group1);
    QCOMPARE(rect2->group(), group1);
    QCOMPARE(group1->group(), (QGraphicsItemGroup *)0);
    QVERIFY(group2->childItems().isEmpty());

    delete group2;
}

void tst_QGraphicsItem::warpChildrenIntoGroup()
{
    QGraphicsScene scene;
    QGraphicsRectItem *parentRectItem = scene.addRect(QRectF(0, 0, 100, 100));
    QGraphicsRectItem *childRectItem = scene.addRect(QRectF(0, 0, 100, 100));
    parentRectItem->rotate(90);
    childRectItem->setPos(-50, -25);
    childRectItem->setParentItem(parentRectItem);

    QCOMPARE(childRectItem->mapToScene(50, 0), QPointF(25, 0));
    QCOMPARE(childRectItem->scenePos(), QPointF(25, -50));

    QGraphicsRectItem *parentOfGroup = scene.addRect(QRectF(0, 0, 100, 100));
    parentOfGroup->setPos(-200, -200);
    parentOfGroup->scale(2, 2);

    QGraphicsItemGroup *group = new QGraphicsItemGroup;
    group->setPos(50, 50);
    group->setParentItem(parentOfGroup);

    QCOMPARE(group->scenePos(), QPointF(-100, -100));

    group->addToGroup(childRectItem);

    QCOMPARE(childRectItem->mapToScene(50, 0), QPointF(25, 0));
    QCOMPARE(childRectItem->scenePos(), QPointF(25, -50));
}

void tst_QGraphicsItem::removeFromGroup()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rect1 = scene.addRect(QRectF(-100, -100, 200, 200));
    QGraphicsRectItem *rect2 = scene.addRect(QRectF(100, 100, 200, 200));
    rect1->setFlag(QGraphicsItem::ItemIsSelectable);
    rect2->setFlag(QGraphicsItem::ItemIsSelectable);
    rect1->setSelected(true);
    rect2->setSelected(true);

    QGraphicsView view(&scene);
    view.show();

    qApp->processEvents(); // index items
    qApp->processEvents(); // emit changed

    QGraphicsItemGroup *group = scene.createItemGroup(scene.selectedItems());
    QVERIFY(group);
    QCOMPARE(group->childItems().size(), 2);
    qApp->processEvents(); // index items
    qApp->processEvents(); // emit changed

    scene.destroyItemGroup(group); // calls removeFromGroup.

    qApp->processEvents(); // index items
    qApp->processEvents(); // emit changed

    QCOMPARE(scene.items().size(), 2);
    QVERIFY(!rect1->group());
    QVERIFY(!rect2->group());
}

class ChildEventTester : public QGraphicsRectItem
{
public:
    ChildEventTester(const QRectF &rect, QGraphicsItem *parent = 0)
        : QGraphicsRectItem(rect, parent), counter(0)
    { }

    int counter;

protected:
    void focusInEvent(QFocusEvent *event)
    { ++counter; QGraphicsRectItem::focusInEvent(event); }
    void mousePressEvent(QGraphicsSceneMouseEvent *)
    { ++counter; }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *)
    { ++counter; }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *)
    { ++counter; }
};

void tst_QGraphicsItem::handlesChildEvents()
{
    ChildEventTester *blue = new ChildEventTester(QRectF(0, 0, 100, 100));
    ChildEventTester *red = new ChildEventTester(QRectF(0, 0, 50, 50));
    ChildEventTester *green = new ChildEventTester(QRectF(0, 0, 25, 25));
    ChildEventTester *gray = new ChildEventTester(QRectF(0, 0, 25, 25));
    ChildEventTester *yellow = new ChildEventTester(QRectF(0, 0, 50, 50));

    blue->setBrush(QBrush(Qt::blue));
    red->setBrush(QBrush(Qt::red));
    yellow->setBrush(QBrush(Qt::yellow));
    green->setBrush(QBrush(Qt::green));
    gray->setBrush(QBrush(Qt::gray));
    red->setPos(50, 0);
    yellow->setPos(50, 50);
    green->setPos(25, 0);
    gray->setPos(25, 25);
    red->setParentItem(blue);
    yellow->setParentItem(blue);
    green->setParentItem(red);
    gray->setParentItem(red);

    QGraphicsScene scene;
    scene.addItem(blue);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(20);

    // Pull out the items, closest item first
    QList<QGraphicsItem *> items = scene.items(scene.itemsBoundingRect());
    QCOMPARE(items.at(0), (QGraphicsItem *)yellow);
    QCOMPARE(items.at(1), (QGraphicsItem *)gray);
    QCOMPARE(items.at(2), (QGraphicsItem *)green);
    QCOMPARE(items.at(3), (QGraphicsItem *)red);
    QCOMPARE(items.at(4), (QGraphicsItem *)blue);

    QCOMPARE(blue->counter, 0);

    // Send events to the toplevel item
    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);

    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setScenePos(blue->mapToScene(5, 5));
    pressEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    releaseEvent.setButton(Qt::LeftButton);
    releaseEvent.setScenePos(blue->mapToScene(5, 5));
    releaseEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QCOMPARE(blue->counter, 2);

    // Send events to a level1 item
    pressEvent.setScenePos(red->mapToScene(5, 5));
    pressEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    releaseEvent.setScenePos(red->mapToScene(5, 5));
    releaseEvent.setScreenPos(view.mapFromScene(releaseEvent.scenePos()));
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QCOMPARE(blue->counter, 2);
    QCOMPARE(red->counter, 2);

    // Send events to a level2 item
    pressEvent.setScenePos(green->mapToScene(5, 5));
    pressEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    releaseEvent.setScenePos(green->mapToScene(5, 5));
    releaseEvent.setScreenPos(view.mapFromScene(releaseEvent.scenePos()));
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QCOMPARE(blue->counter, 2);
    QCOMPARE(red->counter, 2);
    QCOMPARE(green->counter, 2);

    blue->setHandlesChildEvents(true);

    // Send events to a level1 item
    pressEvent.setScenePos(red->mapToScene(5, 5));
    pressEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    releaseEvent.setScenePos(red->mapToScene(5, 5));
    releaseEvent.setScreenPos(view.mapFromScene(releaseEvent.scenePos()));
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QCOMPARE(blue->counter, 4);
    QCOMPARE(red->counter, 2);

    // Send events to a level2 item
    pressEvent.setScenePos(green->mapToScene(5, 5));
    pressEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    releaseEvent.setScenePos(green->mapToScene(5, 5));
    releaseEvent.setScreenPos(view.mapFromScene(releaseEvent.scenePos()));
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QCOMPARE(blue->counter, 6);
    QCOMPARE(red->counter, 2);
    QCOMPARE(green->counter, 2);

    blue->setHandlesChildEvents(false);

    // Send events to a level1 item
    pressEvent.setScenePos(red->mapToScene(5, 5));
    pressEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    releaseEvent.setScenePos(red->mapToScene(5, 5));
    releaseEvent.setScreenPos(view.mapFromScene(releaseEvent.scenePos()));
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QCOMPARE(blue->counter, 6);
    QCOMPARE(red->counter, 4);

    // Send events to a level2 item
    pressEvent.setScenePos(green->mapToScene(5, 5));
    pressEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    releaseEvent.setScenePos(green->mapToScene(5, 5));
    releaseEvent.setScreenPos(view.mapFromScene(releaseEvent.scenePos()));
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QCOMPARE(blue->counter, 6);
    QCOMPARE(red->counter, 4);
    QCOMPARE(green->counter, 4);
}

void tst_QGraphicsItem::handlesChildEvents2()
{
    ChildEventTester *root = new ChildEventTester(QRectF(0, 0, 10, 10));
    root->setHandlesChildEvents(true);
    QVERIFY(root->handlesChildEvents());

    ChildEventTester *child = new ChildEventTester(QRectF(0, 0, 10, 10), root);
    QVERIFY(!child->handlesChildEvents());

    ChildEventTester *child2 = new ChildEventTester(QRectF(0, 0, 10, 10));
    ChildEventTester *child3 = new ChildEventTester(QRectF(0, 0, 10, 10), child2);
    ChildEventTester *child4 = new ChildEventTester(QRectF(0, 0, 10, 10), child3);
    child2->setParentItem(root);
    QVERIFY(!child2->handlesChildEvents());
    QVERIFY(!child3->handlesChildEvents());
    QVERIFY(!child4->handlesChildEvents());

    QGraphicsScene scene;
    scene.addItem(root);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QApplication::processEvents();

    QMouseEvent event(QEvent::MouseButtonPress, view.mapFromScene(5, 5),
                      view.viewport()->mapToGlobal(view.mapFromScene(5, 5)), Qt::LeftButton, 0, 0);
    QApplication::sendEvent(view.viewport(), &event);

    QTRY_COMPARE(root->counter, 1);
}

void tst_QGraphicsItem::handlesChildEvents3()
{
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    ChildEventTester *group2 = new ChildEventTester(QRectF(), 0);
    ChildEventTester *group1 = new ChildEventTester(QRectF(), group2);
    ChildEventTester *leaf = new ChildEventTester(QRectF(), group1);
    scene.addItem(group2);

    leaf->setFlag(QGraphicsItem::ItemIsFocusable);
    group1->setFlag(QGraphicsItem::ItemIsFocusable);
    group1->setHandlesChildEvents(true);
    group2->setFlag(QGraphicsItem::ItemIsFocusable);
    group2->setHandlesChildEvents(true);

    leaf->setFocus();
    QVERIFY(leaf->hasFocus()); // group2 stole the event, but leaf still got focus
    QVERIFY(!group1->hasFocus());
    QVERIFY(!group2->hasFocus());
    QCOMPARE(leaf->counter, 0);
    QCOMPARE(group1->counter, 0);
    QCOMPARE(group2->counter, 1);

    group1->setFocus();
    QVERIFY(group1->hasFocus()); // group2 stole the event, but group1 still got focus
    QVERIFY(!leaf->hasFocus());
    QVERIFY(!group2->hasFocus());
    QCOMPARE(leaf->counter, 0);
    QCOMPARE(group1->counter, 0);
    QCOMPARE(group2->counter, 2);

    group2->setFocus();
    QVERIFY(group2->hasFocus()); // group2 stole the event, and now group2 also has focus
    QVERIFY(!leaf->hasFocus());
    QVERIFY(!group1->hasFocus());
    QCOMPARE(leaf->counter, 0);
    QCOMPARE(group1->counter, 0);
    QCOMPARE(group2->counter, 3);
}


class ChildEventFilterTester : public ChildEventTester
{
public:
    ChildEventFilterTester(const QRectF &rect, QGraphicsItem *parent = 0)
        : ChildEventTester(rect, parent), filter(QEvent::None)
    { }

    QEvent::Type filter;

protected:
    bool sceneEventFilter(QGraphicsItem *item, QEvent *event)
    {
        Q_UNUSED(item);
        if (event->type() == filter) {
            ++counter;
            return true;
        }
        return false;
    }
};

void tst_QGraphicsItem::filtersChildEvents()
{
    QGraphicsScene scene;
    ChildEventFilterTester *root = new ChildEventFilterTester(QRectF(0, 0, 10, 10));
    ChildEventFilterTester *filter = new ChildEventFilterTester(QRectF(10, 10, 10, 10), root);
    ChildEventTester *child = new ChildEventTester(QRectF(20, 20, 10, 10), filter);

    // setup filter
    filter->setFiltersChildEvents(true);
    filter->filter = QEvent::GraphicsSceneMousePress;

    scene.addItem(root);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(20);

    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);

    // send event to child
    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setScenePos(QPointF(25, 25));//child->mapToScene(5, 5));
    pressEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    releaseEvent.setButton(Qt::LeftButton);
    releaseEvent.setScenePos(QPointF(25, 25));//child->mapToScene(5, 5));
    releaseEvent.setScreenPos(view.mapFromScene(pressEvent.scenePos()));
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QTRY_COMPARE(child->counter, 1);  // mouse release is not filtered
    QCOMPARE(filter->counter, 1); // mouse press is filtered
    QCOMPARE(root->counter, 0);

    // add another filter
    root->setFiltersChildEvents(true);
    root->filter = QEvent::GraphicsSceneMouseRelease;

    // send event to child
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QCOMPARE(child->counter, 1);
    QCOMPARE(filter->counter, 2); // mouse press is filtered
    QCOMPARE(root->counter, 1); // mouse release is filtered

    // reparent to another sub-graph
    ChildEventTester *parent = new ChildEventTester(QRectF(10, 10, 10, 10), root);
    child->setParentItem(parent);

    // send event to child
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &releaseEvent);

    QCOMPARE(child->counter, 2); // mouse press is _not_ filtered
    QCOMPARE(parent->counter, 0);
    QCOMPARE(filter->counter, 2);
    QCOMPARE(root->counter, 2); // mouse release is filtered
}

void tst_QGraphicsItem::filtersChildEvents2()
{
    ChildEventFilterTester *root = new ChildEventFilterTester(QRectF(0, 0, 10, 10));
    root->setFiltersChildEvents(true);
    root->filter = QEvent::GraphicsSceneMousePress;
    QVERIFY(root->filtersChildEvents());

    ChildEventTester *child = new ChildEventTester(QRectF(0, 0, 10, 10), root);
    QVERIFY(!child->filtersChildEvents());

    ChildEventTester *child2 = new ChildEventTester(QRectF(0, 0, 10, 10));
    ChildEventTester *child3 = new ChildEventTester(QRectF(0, 0, 10, 10), child2);
    ChildEventTester *child4 = new ChildEventTester(QRectF(0, 0, 10, 10), child3);

    child2->setParentItem(root);
    QVERIFY(!child2->filtersChildEvents());
    QVERIFY(!child3->filtersChildEvents());
    QVERIFY(!child4->filtersChildEvents());

    QGraphicsScene scene;
    scene.addItem(root);

    QGraphicsView view(&scene);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QApplication::processEvents();

    QMouseEvent event(QEvent::MouseButtonPress, view.mapFromScene(5, 5),
                      view.viewport()->mapToGlobal(view.mapFromScene(5, 5)), Qt::LeftButton, 0, 0);
    QApplication::sendEvent(view.viewport(), &event);

    QTRY_COMPARE(root->counter, 1);
    QCOMPARE(child->counter, 0);
    QCOMPARE(child2->counter, 0);
    QCOMPARE(child3->counter, 0);
    QCOMPARE(child4->counter, 0);
}

class CustomItem : public QGraphicsItem
{
public:
    QRectF boundingRect() const
    { return QRectF(-110, -110, 220, 220); }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
    {
        for (int x = -100; x <= 100; x += 25)
            painter->drawLine(x, -100, x, 100);
        for (int y = -100; y <= 100; y += 25)
            painter->drawLine(-100, y, 100, y);

        QFont font = painter->font();
        font.setPointSize(4);
        painter->setFont(font);
        for (int x = -100; x < 100; x += 25) {
            for (int y = -100; y < 100; y += 25) {
                painter->drawText(QRectF(x, y, 25, 25), Qt::AlignCenter, QString("%1x%2").arg(x).arg(y));
            }
        }
    }
};

void tst_QGraphicsItem::ensureVisible()
{
    QGraphicsScene scene;
    scene.setSceneRect(-200, -200, 400, 400);
    QGraphicsItem *item = new CustomItem;
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.setFixedSize(300, 300);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    for (int i = 0; i < 25; ++i) {
        view.scale(qreal(1.06), qreal(1.06));
        QApplication::processEvents();
    }

    item->ensureVisible(-100, -100, 25, 25);
    QTest::qWait(25);

    for (int x = -100; x < 100; x += 25) {
        for (int y = -100; y < 100; y += 25) {
            int xmargin = rand() % 75;
            int ymargin = rand() % 75;
            item->ensureVisible(x, y, 25, 25, xmargin, ymargin);
            QApplication::processEvents();

            QPolygonF viewScenePoly;
            viewScenePoly << view.mapToScene(view.rect().topLeft())
                          << view.mapToScene(view.rect().topRight())
                          << view.mapToScene(view.rect().bottomRight())
                          << view.mapToScene(view.rect().bottomLeft());

            QVERIFY(scene.items(viewScenePoly).contains(item));

            QPainterPath path;
            path.addPolygon(viewScenePoly);
            QVERIFY(path.contains(item->mapToScene(x + 12, y + 12)));

            QPolygonF viewScenePolyMinusMargins;
            viewScenePolyMinusMargins << view.mapToScene(view.rect().topLeft() + QPoint(xmargin, ymargin))
                          << view.mapToScene(view.rect().topRight() + QPoint(-xmargin, ymargin))
                          << view.mapToScene(view.rect().bottomRight() + QPoint(-xmargin, -ymargin))
                          << view.mapToScene(view.rect().bottomLeft() + QPoint(xmargin, -ymargin));

            QPainterPath path2;
            path2.addPolygon(viewScenePolyMinusMargins);
            QVERIFY(path2.contains(item->mapToScene(x + 12, y + 12)));
        }
    }

    item->ensureVisible(100, 100, 25, 25);
    QTest::qWait(25);
}

#ifndef QTEST_NO_CURSOR
void tst_QGraphicsItem::cursor()
{
    QGraphicsScene scene;
    QGraphicsRectItem *item1 = scene.addRect(QRectF(0, 0, 50, 50));
    QGraphicsRectItem *item2 = scene.addRect(QRectF(0, 0, 50, 50));
    item1->setPos(-100, 0);
    item2->setPos(50, 0);

    QVERIFY(!item1->hasCursor());
    QVERIFY(!item2->hasCursor());

    item1->setCursor(Qt::IBeamCursor);
    item2->setCursor(Qt::PointingHandCursor);

    QVERIFY(item1->hasCursor());
    QVERIFY(item2->hasCursor());

    item1->setCursor(QCursor());
    item2->setCursor(QCursor());

    QVERIFY(item1->hasCursor());
    QVERIFY(item2->hasCursor());

    item1->unsetCursor();
    item2->unsetCursor();

    QVERIFY(!item1->hasCursor());
    QVERIFY(!item2->hasCursor());

    item1->setCursor(Qt::IBeamCursor);
    item2->setCursor(Qt::PointingHandCursor);

    QWidget topLevel;
    topLevel.resize(250, 150);
    centerOnScreen(&topLevel);
    QGraphicsView view(&scene,&topLevel);
    view.setFixedSize(200, 100);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    QTest::mouseMove(&view, view.rect().center());

    QTest::qWait(25);

    const Qt::CursorShape viewportShape = view.viewport()->cursor().shape();

    {
        QTest::mouseMove(view.viewport(), QPoint(100, 50));
        QMouseEvent event(QEvent::MouseMove, QPoint(100, 50), Qt::NoButton, 0, 0);
        QApplication::sendEvent(view.viewport(), &event);
    }

    QTRY_COMPARE(view.viewport()->cursor().shape(), viewportShape);

    {
        QTest::mouseMove(view.viewport(), view.mapFromScene(item1->sceneBoundingRect().center()));
        QMouseEvent event(QEvent::MouseMove, view.mapFromScene(item1->sceneBoundingRect().center()), Qt::NoButton, 0, 0);
        QApplication::sendEvent(view.viewport(), &event);
    }

    QTRY_COMPARE(view.viewport()->cursor().shape(), item1->cursor().shape());

    {
        QTest::mouseMove(view.viewport(), view.mapFromScene(item2->sceneBoundingRect().center()));
        QMouseEvent event(QEvent::MouseMove, view.mapFromScene(item2->sceneBoundingRect().center()), Qt::NoButton, 0, 0);
        QApplication::sendEvent(view.viewport(), &event);
    }

    QTRY_COMPARE(view.viewport()->cursor().shape(), item2->cursor().shape());

    {
        QTest::mouseMove(view.viewport(), view.rect().center());
        QMouseEvent event(QEvent::MouseMove, QPoint(100, 25), Qt::NoButton, 0, 0);
        QApplication::sendEvent(view.viewport(), &event);
    }

    QTRY_COMPARE(view.viewport()->cursor().shape(), viewportShape);
}
#endif
/*
void tst_QGraphicsItem::textControlGetterSetter()
{
    QGraphicsTextItem *item = new QGraphicsTextItem;
    QCOMPARE(item->textControl()->parent(), item);
    QPointer<QWidgetTextControl> control = item->textControl();
    delete item;
    QVERIFY(!control);

    item = new QGraphicsTextItem;

    QPointer<QWidgetTextControl> oldControl = control;
    control = new QWidgetTextControl;

    item->setTextControl(control);
    QCOMPARE(item->textControl(), control);
    QVERIFY(!control->parent());
    QVERIFY(!oldControl);

    // set some text to give it a size, to test that
    // setTextControl (re)connects signals
    const QRectF oldBoundingRect = item->boundingRect();
    QVERIFY(oldBoundingRect.isValid());
    item->setPlainText("Some text");
    item->adjustSize();
    QVERIFY(item->boundingRect().isValid());
    QVERIFY(item->boundingRect() != oldBoundingRect);

    // test that on setting a control the item size
    // is adjusted
    oldControl = control;
    control = new QWidgetTextControl;
    control->setPlainText("foo!");
    item->setTextControl(control);
    QCOMPARE(item->boundingRect().size(), control->document()->documentLayout()->documentSize());

    QVERIFY(oldControl);
    delete oldControl;

    delete item;
    QVERIFY(control);
    delete control;
}
*/

void tst_QGraphicsItem::defaultItemTest_QGraphicsLineItem()
{
    QGraphicsLineItem item;
    QCOMPARE(item.line(), QLineF());
    QCOMPARE(item.pen(), QPen());
    QCOMPARE(item.shape(), QPainterPath());

    item.setPen(QPen(Qt::black, 1));
    QCOMPARE(item.pen(), QPen(Qt::black, 1));
    item.setLine(QLineF(0, 0, 10, 0));
    QCOMPARE(item.line(), QLineF(0, 0, 10, 0));
    QCOMPARE(item.boundingRect(), QRectF(-0.5, -0.5, 11, 1));
    QCOMPARE(item.shape().elementCount(), 11);

    QPainterPath path;
    path.moveTo(0, -0.5);
    path.lineTo(10, -0.5);
    path.lineTo(10.5, -0.5);
    path.lineTo(10.5, 0.5);
    path.lineTo(10, 0.5);
    path.lineTo(0, 0.5);
    path.lineTo(-0.5, 0.5);
    path.lineTo(-0.5, -0.5);
    path.lineTo(0, -0.5);
    path.lineTo(0, 0);
    path.lineTo(10, 0);
    path.closeSubpath();

    for (int i = 0; i < 11; ++i)
        QCOMPARE(QPointF(item.shape().elementAt(i)), QPointF(path.elementAt(i)));
}

void tst_QGraphicsItem::defaultItemTest_QGraphicsPixmapItem()
{
    QGraphicsPixmapItem item;
    QVERIFY(item.pixmap().isNull());
    QCOMPARE(item.offset(), QPointF());
    QCOMPARE(item.transformationMode(), Qt::FastTransformation);

    QPixmap pixmap(300, 200);
    pixmap.fill(Qt::red);
    item.setPixmap(pixmap);
    QCOMPARE(item.pixmap(), pixmap);

    item.setTransformationMode(Qt::FastTransformation);
    QCOMPARE(item.transformationMode(), Qt::FastTransformation);
    item.setTransformationMode(Qt::SmoothTransformation);
    QCOMPARE(item.transformationMode(), Qt::SmoothTransformation);

    item.setOffset(-15, -15);
    QCOMPARE(item.offset(), QPointF(-15, -15));
    item.setOffset(QPointF(-10, -10));
    QCOMPARE(item.offset(), QPointF(-10, -10));

    QCOMPARE(item.boundingRect(), QRectF(-10, -10, 300, 200));
}

void tst_QGraphicsItem::defaultItemTest_QGraphicsTextItem()
{
    QGraphicsTextItem *text = new QGraphicsTextItem;
    QVERIFY(!text->openExternalLinks());
    QVERIFY(text->textCursor().isNull());
    QCOMPARE(text->defaultTextColor(), QPalette().color(QPalette::Text));
    QVERIFY(text->document() != 0);
    QCOMPARE(text->font(), QApplication::font());
    QCOMPARE(text->textInteractionFlags(), Qt::TextInteractionFlags(Qt::NoTextInteraction));
    QCOMPARE(text->textWidth(), -1.0);
    QCOMPARE(text->toPlainText(), QString(""));

    QGraphicsScene scene;
    scene.addItem(text);
    text->setPlainText("Hello world");
    text->setFlag(QGraphicsItem::ItemIsMovable);

    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
        event.setScenePos(QPointF(1, 1));
        event.setButton(Qt::LeftButton);
        event.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &event);
        QGraphicsSceneMouseEvent event2(QEvent::GraphicsSceneMouseMove);
        event2.setScenePos(QPointF(11, 11));
        event2.setButton(Qt::LeftButton);
        event2.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &event2);
    }

    QCOMPARE(text->pos(), QPointF(10, 10));

    text->setTextInteractionFlags(Qt::NoTextInteraction);
    QVERIFY(!(text->flags() & QGraphicsItem::ItemAcceptsInputMethod));
    text->setTextInteractionFlags(Qt::TextEditorInteraction);
    QCOMPARE(text->textInteractionFlags(), Qt::TextInteractionFlags(Qt::TextEditorInteraction));
    QVERIFY(text->flags() & QGraphicsItem::ItemAcceptsInputMethod);

    {
        QGraphicsSceneMouseEvent event2(QEvent::GraphicsSceneMouseMove);
        event2.setScenePos(QPointF(21, 21));
        event2.setButton(Qt::LeftButton);
        event2.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &event2);
    }

    QCOMPARE(text->pos(), QPointF(20, 20)); // clicked on edge, item moved
}

void tst_QGraphicsItem::defaultItemTest_QGraphicsEllipseItem()
{
    QGraphicsEllipseItem item;
    item.setPen(QPen(Qt::black, 0));
    QVERIFY(item.rect().isNull());
    QVERIFY(item.boundingRect().isNull());
    QVERIFY(item.shape().isEmpty());
    QCOMPARE(item.spanAngle(), 360 * 16);
    QCOMPARE(item.startAngle(), 0);

    item.setRect(0, 0, 100, 100);
    QCOMPARE(item.boundingRect(), QRectF(0, 0, 100, 100));

    item.setSpanAngle(90 * 16);
    // for some reason, the bounding rect has very few significant digits
    // (i.e. it's likely that floats are being used inside it), so we
    // must force the conversion from qreals to float or these tests will fail
    QCOMPARE(float(item.boundingRect().left()), 50.0f);
    QVERIFY(qFuzzyIsNull(float(item.boundingRect().top())));
    QCOMPARE(float(item.boundingRect().width()), 50.0f);
    QCOMPARE(float(item.boundingRect().height()), 50.0f);

    item.setPen(QPen(Qt::black, 1));
    QCOMPARE(item.boundingRect(), QRectF(49.5, -0.5, 51, 51));

    item.setSpanAngle(180 * 16);
    QCOMPARE(item.boundingRect(), QRectF(-0.5, -0.5, 101, 51));

    item.setSpanAngle(360 * 16);
    QCOMPARE(item.boundingRect(), QRectF(-0.5, -0.5, 101, 101));
}

class ItemChangeTester : public QGraphicsRectItem
{
public:
    ItemChangeTester()
    { setFlag(ItemSendsGeometryChanges); clear(); }
    ItemChangeTester(QGraphicsItem *parent) : QGraphicsRectItem(parent)
    { setFlag(ItemSendsGeometryChanges); clear(); }

    void clear()
    {
        itemChangeReturnValue = QVariant();
        itemSceneChangeTargetScene = 0;
        changes.clear();
        values.clear();
        oldValues.clear();
    }

    QVariant itemChangeReturnValue;
    QGraphicsScene *itemSceneChangeTargetScene;

    QList<GraphicsItemChange> changes;
    QList<QVariant> values;
    QList<QVariant> oldValues;
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value)
    {
        changes << change;
        values << value;
        switch (change) {
        case QGraphicsItem::ItemPositionChange:
            oldValues << pos();
            break;
        case QGraphicsItem::ItemPositionHasChanged:
            break;
        case QGraphicsItem::ItemMatrixChange: {
            QVariant variant;
            variant.setValue<QMatrix>(matrix());
            oldValues << variant;
        }
            break;
        case QGraphicsItem::ItemTransformChange: {
            QVariant variant;
            variant.setValue<QTransform>(transform());
            oldValues << variant;
        }
            break;
        case QGraphicsItem::ItemTransformHasChanged:
            break;
        case QGraphicsItem::ItemVisibleChange:
            oldValues << isVisible();
            break;
        case QGraphicsItem::ItemVisibleHasChanged:
            break;
        case QGraphicsItem::ItemEnabledChange:
            oldValues << isEnabled();
            break;
        case QGraphicsItem::ItemEnabledHasChanged:
            break;
        case QGraphicsItem::ItemSelectedChange:
            oldValues << isSelected();
            break;
        case QGraphicsItem::ItemSelectedHasChanged:
            break;
        case QGraphicsItem::ItemParentChange:
            oldValues << QVariant::fromValue<void *>(parentItem());
            break;
        case QGraphicsItem::ItemParentHasChanged:
            break;
        case QGraphicsItem::ItemChildAddedChange:
            oldValues << childItems().size();
            break;
        case QGraphicsItem::ItemChildRemovedChange:
            oldValues << childItems().size();
            break;
        case QGraphicsItem::ItemSceneChange:
            oldValues << QVariant::fromValue<QGraphicsScene *>(scene());
            if (itemSceneChangeTargetScene
                && qvariant_cast<QGraphicsScene *>(value)
                && itemSceneChangeTargetScene != qvariant_cast<QGraphicsScene *>(value)) {
                return QVariant::fromValue<QGraphicsScene *>(itemSceneChangeTargetScene);
            }
            return value;
        case QGraphicsItem::ItemSceneHasChanged:
            break;
        case QGraphicsItem::ItemCursorChange:
#ifndef QTEST_NO_CURSOR
            oldValues << cursor();
#endif
            break;
        case QGraphicsItem::ItemCursorHasChanged:
            break;
        case QGraphicsItem::ItemToolTipChange:
            oldValues << toolTip();
            break;
        case QGraphicsItem::ItemToolTipHasChanged:
            break;
        case QGraphicsItem::ItemFlagsChange:
            oldValues << quint32(flags());
            break;
        case QGraphicsItem::ItemFlagsHaveChanged:
            break;
        case QGraphicsItem::ItemZValueChange:
            oldValues << zValue();
            break;
        case QGraphicsItem::ItemZValueHasChanged:
            break;
        case QGraphicsItem::ItemOpacityChange:
            oldValues << opacity();
            break;
        case QGraphicsItem::ItemOpacityHasChanged:
            break;
        case QGraphicsItem::ItemScenePositionHasChanged:
            break;
        case QGraphicsItem::ItemRotationChange:
            oldValues << rotation();
            break;
        case QGraphicsItem::ItemRotationHasChanged:
            break;
        case QGraphicsItem::ItemScaleChange:
            oldValues << scale();
            break;
        case QGraphicsItem::ItemScaleHasChanged:
            break;
        case QGraphicsItem::ItemTransformOriginPointChange:
            oldValues << transformOriginPoint();
            break;
        case QGraphicsItem::ItemTransformOriginPointHasChanged:
            break;
        }
        return itemChangeReturnValue.isValid() ? itemChangeReturnValue : value;
    }
};

void tst_QGraphicsItem::itemChange()
{
    ItemChangeTester tester;
    tester.itemSceneChangeTargetScene = 0;

    ItemChangeTester testerHelper;
    QVERIFY(tester.changes.isEmpty());
    QVERIFY(tester.values.isEmpty());

    int changeCount = 0;
    {
        // ItemEnabledChange
        tester.itemChangeReturnValue = true;
        tester.setEnabled(false);
        ++changeCount;
        ++changeCount; // HasChanged
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemEnabledChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemEnabledHasChanged);
        QCOMPARE(tester.values.at(tester.values.size() - 2), QVariant(false));
        QCOMPARE(tester.values.at(tester.values.size() - 1), QVariant(true));
        QCOMPARE(tester.oldValues.last(), QVariant(true));
        QCOMPARE(tester.isEnabled(), true);
    }
    {
        // ItemMatrixChange / ItemTransformHasChanged
        tester.itemChangeReturnValue.setValue<QMatrix>(QMatrix().rotate(90));
        tester.setMatrix(QMatrix().translate(50, 0), true);
        ++changeCount; // notification sent too
        QCOMPARE(tester.changes.size(), ++changeCount);
        QCOMPARE(int(tester.changes.at(tester.changes.size() - 2)), int(QGraphicsItem::ItemMatrixChange));
        QCOMPARE(int(tester.changes.last()), int(QGraphicsItem::ItemTransformHasChanged));
        QCOMPARE(qvariant_cast<QMatrix>(tester.values.at(tester.values.size() - 2)),
                 QMatrix().translate(50, 0));
        QCOMPARE(tester.values.last(), QVariant(QTransform(QMatrix().rotate(90))));
        QVariant variant;
        variant.setValue<QMatrix>(QMatrix());
        QCOMPARE(tester.oldValues.last(), variant);
        QCOMPARE(tester.matrix(), QMatrix().rotate(90));
    }
    {
        tester.resetTransform();
        ++changeCount;
        ++changeCount; // notification sent too

        // ItemTransformChange / ItemTransformHasChanged
        tester.itemChangeReturnValue.setValue<QTransform>(QTransform().rotate(90));
        tester.translate(50, 0);
        ++changeCount; // notification sent too
        ++changeCount;
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemTransformChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemTransformHasChanged);
        QCOMPARE(qvariant_cast<QTransform>(tester.values.at(tester.values.size() - 2)),
                 QTransform().translate(50, 0));
        QCOMPARE(qvariant_cast<QTransform>(tester.values.at(tester.values.size() - 1)),
                 QTransform().rotate(90));
        QVariant variant;
        variant.setValue<QTransform>(QTransform());
        QCOMPARE(tester.oldValues.last(), variant);
        QCOMPARE(tester.transform(), QTransform().rotate(90));
    }
    {
        // ItemPositionChange / ItemPositionHasChanged
        tester.itemChangeReturnValue = QPointF(42, 0);
        tester.setPos(0, 42);
        ++changeCount; // notification sent too
        ++changeCount;
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemPositionChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemPositionHasChanged);
        QCOMPARE(tester.values.at(tester.changes.size() - 2), QVariant(QPointF(0, 42)));
        QCOMPARE(tester.values.at(tester.changes.size() - 1), QVariant(QPointF(42, 0)));
        QCOMPARE(tester.oldValues.last(), QVariant(QPointF()));
        QCOMPARE(tester.pos(), QPointF(42, 0));
    }
    {
        // ItemZValueChange / ItemZValueHasChanged
        tester.itemChangeReturnValue = qreal(2.0);
        tester.setZValue(1.0);
        ++changeCount; // notification sent too
        ++changeCount;
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemZValueChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemZValueHasChanged);
        QCOMPARE(tester.values.at(tester.changes.size() - 2), QVariant(qreal(1.0)));
        QCOMPARE(tester.values.at(tester.changes.size() - 1), QVariant(qreal(2.0)));
        QCOMPARE(tester.oldValues.last(), QVariant(qreal(0.0)));
        QCOMPARE(tester.zValue(), qreal(2.0));
    }
    {
        // ItemRotationChange / ItemRotationHasChanged
        tester.itemChangeReturnValue = qreal(15.0);
        tester.setRotation(10.0);
        ++changeCount; // notification sent too
        ++changeCount;
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemRotationChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemRotationHasChanged);
        QCOMPARE(tester.values.at(tester.changes.size() - 2), QVariant(qreal(10.0)));
        QCOMPARE(tester.values.at(tester.changes.size() - 1), QVariant(qreal(15.0)));
        QCOMPARE(tester.oldValues.last(), QVariant(qreal(0.0)));
        QCOMPARE(tester.rotation(), qreal(15.0));
    }
    {
        // ItemScaleChange / ItemScaleHasChanged
        tester.itemChangeReturnValue = qreal(2.0);
        tester.setScale(1.5);
        ++changeCount; // notification sent too
        ++changeCount;
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemScaleChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemScaleHasChanged);
        QCOMPARE(tester.values.at(tester.changes.size() - 2), QVariant(qreal(1.5)));
        QCOMPARE(tester.values.at(tester.changes.size() - 1), QVariant(qreal(2.0)));
        QCOMPARE(tester.oldValues.last(), QVariant(qreal(1.0)));
        QCOMPARE(tester.scale(), qreal(2.0));
    }
    {
        // ItemTransformOriginPointChange / ItemTransformOriginPointHasChanged
        tester.itemChangeReturnValue = QPointF(2.0, 2.0);
        tester.setTransformOriginPoint(1.0, 1.0);
        ++changeCount; // notification sent too
        ++changeCount;
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemTransformOriginPointChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemTransformOriginPointHasChanged);
        QCOMPARE(tester.values.at(tester.changes.size() - 2), QVariant(QPointF(1.0, 1.0)));
        QCOMPARE(tester.values.at(tester.changes.size() - 1), QVariant(QPointF(2.0, 2.0)));
        QCOMPARE(tester.oldValues.last(), QVariant(QPointF(0.0, 0.0)));
        QCOMPARE(tester.transformOriginPoint(), QPointF(2.0, 2.0));
    }
    {
        // ItemFlagsChange
        tester.itemChangeReturnValue = QGraphicsItem::ItemIsSelectable;
        tester.setFlag(QGraphicsItem::ItemIsSelectable, false);
        QCOMPARE(tester.changes.size(), changeCount);  // No change
        tester.setFlag(QGraphicsItem::ItemIsSelectable, true);
        ++changeCount;
        ++changeCount; // ItemFlagsHasChanged
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemFlagsChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemFlagsHaveChanged);
        QVariant expectedFlags = QVariant::fromValue<quint32>(QGraphicsItem::GraphicsItemFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges));
        QCOMPARE(tester.values.at(tester.values.size() - 2), expectedFlags);
        QCOMPARE(tester.values.at(tester.values.size() - 1), QVariant::fromValue<quint32>((quint32)QGraphicsItem::ItemIsSelectable));
    }
    {
        // ItemSelectedChange
        tester.setSelected(false);
        QCOMPARE(tester.changes.size(), changeCount); // No change :-)
        tester.itemChangeReturnValue = true;
        tester.setSelected(true);
        ++changeCount;
        ++changeCount; // ItemSelectedHasChanged
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemSelectedChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemSelectedHasChanged);
        QCOMPARE(tester.values.at(tester.values.size() - 2), QVariant(true));
        QCOMPARE(tester.values.at(tester.values.size() - 1), QVariant(true));
        QCOMPARE(tester.oldValues.last(), QVariant(false));
        QCOMPARE(tester.isSelected(), true);

        tester.itemChangeReturnValue = false;
        tester.setSelected(true);

        // the value hasn't changed to the itemChange return value
        // bacause itemChange is never called (true -> true is a noop).
        QCOMPARE(tester.isSelected(), true);
    }
    {
        // ItemVisibleChange
        tester.itemChangeReturnValue = false;
        QVERIFY(tester.isVisible());
        tester.setVisible(false);
        ++changeCount; // ItemVisibleChange
        ++changeCount; // ItemSelectedChange
        ++changeCount; // ItemSelectedHasChanged
        ++changeCount; // ItemVisibleHasChanged
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 4), QGraphicsItem::ItemVisibleChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 3), QGraphicsItem::ItemSelectedChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemSelectedHasChanged);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemVisibleHasChanged);
        QCOMPARE(tester.values.at(tester.values.size() - 4), QVariant(false));
        QCOMPARE(tester.values.at(tester.values.size() - 3), QVariant(false));
        QCOMPARE(tester.values.at(tester.values.size() - 2), QVariant(false));
        QCOMPARE(tester.values.at(tester.values.size() - 1), QVariant(false));
        QCOMPARE(tester.isVisible(), false);
    }
    {
        // ItemParentChange
        tester.itemChangeReturnValue.setValue<QGraphicsItem *>(0);
        tester.setParentItem(&testerHelper);
        QCOMPARE(tester.changes.size(), ++changeCount);
        QCOMPARE(tester.changes.last(), QGraphicsItem::ItemParentChange);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(tester.values.last()), (QGraphicsItem *)&testerHelper);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(tester.oldValues.last()), (QGraphicsItem *)0);
        QCOMPARE(tester.parentItem(), (QGraphicsItem *)0);
    }
    {
        // ItemOpacityChange
        tester.itemChangeReturnValue = 1.0;
        tester.setOpacity(0.7);
        QCOMPARE(tester.changes.size(), ++changeCount);
        QCOMPARE(tester.changes.last(), QGraphicsItem::ItemOpacityChange);
        QVERIFY(qFuzzyCompare(qreal(tester.values.last().toDouble()), qreal(0.7)));
        QCOMPARE(tester.oldValues.last().toDouble(), double(1.0));
        QCOMPARE(tester.opacity(), qreal(1.0));
        tester.itemChangeReturnValue = 0.7;
        tester.setOpacity(0.7);
        ++changeCount; // ItemOpacityChange
        ++changeCount; // ItemOpacityHasChanged
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemOpacityChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemOpacityHasChanged);
        QCOMPARE(tester.opacity(), qreal(0.7));
    }
    {
        // ItemChildAddedChange
        tester.itemChangeReturnValue.clear();
        testerHelper.setParentItem(&tester);
        QCOMPARE(tester.changes.size(), ++changeCount);
        QCOMPARE(tester.changes.last(), QGraphicsItem::ItemChildAddedChange);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(tester.values.last()), (QGraphicsItem *)&testerHelper);
    }
    {
        // ItemChildRemovedChange 1
        testerHelper.setParentItem(0);
        QCOMPARE(tester.changes.size(), ++changeCount);
        QCOMPARE(tester.changes.last(), QGraphicsItem::ItemChildRemovedChange);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(tester.values.last()), (QGraphicsItem *)&testerHelper);

        // ItemChildRemovedChange 1
        ItemChangeTester *test = new ItemChangeTester;
        test->itemSceneChangeTargetScene = 0;
        int count = 0;
        QGraphicsScene *scene = new QGraphicsScene;
        scene->addItem(test);
        count = test->changes.size();
        //We test here the fact that when a child is deleted the parent receive only one ItemChildRemovedChange
        QGraphicsRectItem *child = new QGraphicsRectItem(test);
        //We received ItemChildAddedChange
        QCOMPARE(test->changes.size(), ++count);
        QCOMPARE(test->changes.last(), QGraphicsItem::ItemChildAddedChange);
        delete child;
        child = 0;
        QCOMPARE(test->changes.size(), ++count);
        QCOMPARE(test->changes.last(), QGraphicsItem::ItemChildRemovedChange);

        ItemChangeTester *childTester = new ItemChangeTester(test);
        //Changes contains all sceneHasChanged and so on, we don't want to test that
        int childCount = childTester->changes.size();
        //We received ItemChildAddedChange
        QCOMPARE(test->changes.size(), ++count);
        child = new QGraphicsRectItem(childTester);
        //We received ItemChildAddedChange
        QCOMPARE(childTester->changes.size(), ++childCount);
        QCOMPARE(childTester->changes.last(), QGraphicsItem::ItemChildAddedChange);
        //Delete the child of the top level with all its children
        delete childTester;
        //Only one removal
        QCOMPARE(test->changes.size(), ++count);
        QCOMPARE(test->changes.last(), QGraphicsItem::ItemChildRemovedChange);
        delete scene;
    }
    {
        // ItemChildRemovedChange 2
        ItemChangeTester parent;
        ItemChangeTester *child = new ItemChangeTester;
        child->setParentItem(&parent);
        QCOMPARE(parent.changes.last(), QGraphicsItem::ItemChildAddedChange);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(parent.values.last()), (QGraphicsItem *)child);
        delete child;
        QCOMPARE(parent.changes.last(), QGraphicsItem::ItemChildRemovedChange);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(parent.values.last()), (QGraphicsItem *)child);
    }
    {
        // !!! Note: If this test crashes because of double-deletion, there's
        // a bug somewhere in QGraphicsScene or QGraphicsItem.

        // ItemSceneChange
        QGraphicsScene scene;
        QGraphicsScene scene2;
        scene.addItem(&tester);
        ++changeCount; // ItemSceneChange (scene)
        ++changeCount; // ItemSceneHasChanged (scene)
        QCOMPARE(tester.changes.size(), changeCount);

        QCOMPARE(tester.scene(), &scene);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemSceneChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemSceneHasChanged);
        // Item's old value was 0
        // Item's current value is scene
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.oldValues.last()), (QGraphicsScene *)0);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.last()), (QGraphicsScene *)&scene);
        scene2.addItem(&tester);
        ++changeCount; // ItemSceneChange (0) was: (scene)
        ++changeCount; // ItemSceneHasChanged (0)
        ++changeCount; // ItemSceneChange (scene2) was: (0)
        ++changeCount; // ItemSceneHasChanged (scene2)
        QCOMPARE(tester.changes.size(), changeCount);

        QCOMPARE(tester.scene(), &scene2);
        QCOMPARE(tester.changes.at(tester.changes.size() - 4), QGraphicsItem::ItemSceneChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 3), QGraphicsItem::ItemSceneHasChanged);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemSceneChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemSceneHasChanged);
        // Item's last old value was scene
        // Item's last current value is 0

        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.oldValues.at(tester.oldValues.size() - 2)), (QGraphicsScene *)&scene);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.oldValues.at(tester.oldValues.size() - 1)), (QGraphicsScene *)0);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.at(tester.values.size() - 4)), (QGraphicsScene *)0);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.at(tester.values.size() - 3)), (QGraphicsScene *)0);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.at(tester.values.size() - 2)), (QGraphicsScene *)&scene2);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.at(tester.values.size() - 1)), (QGraphicsScene *)&scene2);
        // Item's last old value was 0
        // Item's last current value is scene2
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.oldValues.last()), (QGraphicsScene *)0);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.last()), (QGraphicsScene *)&scene2);

        scene2.removeItem(&tester);
        ++changeCount; // ItemSceneChange (0) was: (scene2)
        ++changeCount; // ItemSceneHasChanged (0)
        QCOMPARE(tester.changes.size(), changeCount);

        QCOMPARE(tester.scene(), (QGraphicsScene *)0);
        QCOMPARE(tester.changes.at(tester.changes.size() - 2), QGraphicsItem::ItemSceneChange);
        QCOMPARE(tester.changes.at(tester.changes.size() - 1), QGraphicsItem::ItemSceneHasChanged);
        // Item's last old value was scene2
        // Item's last current value is 0
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.oldValues.last()), (QGraphicsScene *)&scene2);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.at(tester.values.size() - 2)), (QGraphicsScene *)0);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.at(tester.values.size() - 1)), (QGraphicsScene *)0);

        tester.itemSceneChangeTargetScene = &scene;
        scene2.addItem(&tester);
        ++changeCount; // ItemSceneChange (scene2) was: (0)
        ++changeCount; // ItemSceneChange (scene) was: (0)
        ++changeCount; // ItemSceneHasChanged (scene)
        QCOMPARE(tester.values.size(), changeCount);

        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.at(tester.values.size() - 3)), (QGraphicsScene *)&scene2);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.at(tester.values.size() - 2)), (QGraphicsScene *)&scene);
        QCOMPARE(qvariant_cast<QGraphicsScene *>(tester.values.at(tester.values.size() - 1)), (QGraphicsScene *)&scene);

        QCOMPARE(tester.scene(), &scene);
        tester.itemSceneChangeTargetScene = 0;
        tester.itemChangeReturnValue = QVariant();
        scene.removeItem(&tester);
        ++changeCount; // ItemSceneChange
        ++changeCount; // ItemSceneHasChanged
        QCOMPARE(tester.scene(), (QGraphicsScene *)0);
    }
    {
        // ItemToolTipChange/ItemToolTipHasChanged
        const QString toolTip(QLatin1String("I'm soo cool"));
        const QString overridenToolTip(QLatin1String("No, you are not soo cool"));
        tester.itemChangeReturnValue = overridenToolTip;
        tester.setToolTip(toolTip);
        ++changeCount; // ItemToolTipChange
        ++changeCount; // ItemToolTipHasChanged
        QCOMPARE(tester.changes.size(), changeCount);
        QCOMPARE(tester.changes.at(changeCount - 2), QGraphicsItem::ItemToolTipChange);
        QCOMPARE(tester.values.at(changeCount - 2).toString(), toolTip);
        QCOMPARE(tester.changes.at(changeCount - 1), QGraphicsItem::ItemToolTipHasChanged);
        QCOMPARE(tester.values.at(changeCount - 1).toString(), overridenToolTip);
        QCOMPARE(tester.toolTip(), overridenToolTip);
        tester.itemChangeReturnValue = QVariant();
    }
}

class EventFilterTesterItem : public QGraphicsLineItem
{
public:
    QList<QEvent::Type> filteredEvents;
    QList<QGraphicsItem *> filteredEventReceivers;
    bool handlesSceneEvents;

    QList<QEvent::Type> receivedEvents;

    EventFilterTesterItem() : handlesSceneEvents(false) {}

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event)
    {
        filteredEvents << event->type();
        filteredEventReceivers << watched;
        return handlesSceneEvents;
    }

    bool sceneEvent(QEvent *event)
    {
        return QGraphicsLineItem::sceneEvent(event);
    }
};

void tst_QGraphicsItem::sceneEventFilter()
{
    QGraphicsScene scene;

    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QGraphicsTextItem *text1 = scene.addText(QLatin1String("Text1"));
    QGraphicsTextItem *text2 = scene.addText(QLatin1String("Text2"));
    QGraphicsTextItem *text3 = scene.addText(QLatin1String("Text3"));
    text1->setFlag(QGraphicsItem::ItemIsFocusable);
    text2->setFlag(QGraphicsItem::ItemIsFocusable);
    text3->setFlag(QGraphicsItem::ItemIsFocusable);

    EventFilterTesterItem *tester = new EventFilterTesterItem;
    scene.addItem(tester);

    QTRY_VERIFY(!text1->hasFocus());
    text1->installSceneEventFilter(tester);
    text1->setFocus();
    QTRY_VERIFY(text1->hasFocus());

    QCOMPARE(tester->filteredEvents.size(), 1);
    QCOMPARE(tester->filteredEvents.at(0), QEvent::FocusIn);
    QCOMPARE(tester->filteredEventReceivers.at(0), static_cast<QGraphicsItem *>(text1));

    text2->installSceneEventFilter(tester);
    text3->installSceneEventFilter(tester);

    text2->setFocus();
    text3->setFocus();

    QCOMPARE(tester->filteredEvents.size(), 5);
    QCOMPARE(tester->filteredEvents.at(1), QEvent::FocusOut);
    QCOMPARE(tester->filteredEventReceivers.at(1), static_cast<QGraphicsItem *>(text1));
    QCOMPARE(tester->filteredEvents.at(2), QEvent::FocusIn);
    QCOMPARE(tester->filteredEventReceivers.at(2), static_cast<QGraphicsItem *>(text2));
    QCOMPARE(tester->filteredEvents.at(3), QEvent::FocusOut);
    QCOMPARE(tester->filteredEventReceivers.at(3), static_cast<QGraphicsItem *>(text2));
    QCOMPARE(tester->filteredEvents.at(4), QEvent::FocusIn);
    QCOMPARE(tester->filteredEventReceivers.at(4), static_cast<QGraphicsItem *>(text3));

    text1->removeSceneEventFilter(tester);
    text1->setFocus();

    QCOMPARE(tester->filteredEvents.size(), 6);
    QCOMPARE(tester->filteredEvents.at(5), QEvent::FocusOut);
    QCOMPARE(tester->filteredEventReceivers.at(5), static_cast<QGraphicsItem *>(text3));

    tester->handlesSceneEvents = true;
    text2->setFocus();

    QCOMPARE(tester->filteredEvents.size(), 7);
    QCOMPARE(tester->filteredEvents.at(6), QEvent::FocusIn);
    QCOMPARE(tester->filteredEventReceivers.at(6), static_cast<QGraphicsItem *>(text2));

    QVERIFY(text2->hasFocus());

    //Let check if the items are correctly removed from the sceneEventFilters array
    //to avoid stale pointers.
    QGraphicsView gv;
    QGraphicsScene *anotherScene = new QGraphicsScene;
    QGraphicsTextItem *ti = anotherScene->addText("This is a test #1");
    ti->moveBy(50, 50);
    QGraphicsTextItem *ti2 = anotherScene->addText("This is a test #2");
    QGraphicsTextItem *ti3 = anotherScene->addText("This is a test #3");
    gv.setScene(anotherScene);
    gv.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gv));
    QTest::qWait(25);
    ti->installSceneEventFilter(ti2);
    ti3->installSceneEventFilter(ti);
    delete ti2;
    //we souldn't crash
    QTest::mouseMove(gv.viewport(), gv.mapFromScene(ti->scenePos()));
    QTest::qWait(30);
    delete ti;
}

class GeometryChanger : public QGraphicsItem
{
public:
    void changeGeometry()
    { prepareGeometryChange(); }
};

void tst_QGraphicsItem::prepareGeometryChange()
{
    {
        QGraphicsScene scene;
        QGraphicsItem *item = scene.addRect(QRectF(0, 0, 100, 100));
        QVERIFY(scene.items(QRectF(0, 0, 100, 100)).contains(item));
        ((GeometryChanger *)item)->changeGeometry();
        QVERIFY(scene.items(QRectF(0, 0, 100, 100)).contains(item));
    }
}


class PaintTester : public QGraphicsRectItem
{
public:
    PaintTester() : widget(NULL), painted(0) { setRect(QRectF(10, 10, 20, 20));}

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *w)
    {
        widget = w;
        painted++;
    }

    QWidget*  widget;
    int painted;
};

void tst_QGraphicsItem::paint()
{
#ifdef Q_OS_MACX
    if (QSysInfo::MacintoshVersion == QSysInfo::MV_10_7)
        QSKIP("QTBUG-31454 - Unstable auto-test");
#endif
    QGraphicsScene scene;

    PaintTester paintTester;
    scene.addItem(&paintTester);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QApplication::processEvents();
#ifdef Q_OS_WIN32
    //we try to switch the desktop: if it fails, we skip the test
    if (::SwitchDesktop( ::GetThreadDesktop( ::GetCurrentThreadId() ) ) == 0) {
        QSKIP("The Graphics View doesn't get the paint events");
    }
#endif

    QTRY_COMPARE(paintTester.widget, view.viewport());

    view.hide();

    QGraphicsScene scene2;
    QGraphicsView view2(&scene2);
    view2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view2));
    QTest::qWait(25);

    PaintTester tester2;
    scene2.addItem(&tester2);
    qApp->processEvents();

    //First show one paint
    QTRY_COMPARE(tester2.painted, 1);

    //nominal case, update call paint
    tester2.update();
    qApp->processEvents();
    QTRY_COMPARE(tester2.painted, 2);

    //we remove the item from the scene, number of updates is still the same
    tester2.update();
    scene2.removeItem(&tester2);
    qApp->processEvents();
    QTRY_COMPARE(tester2.painted, 2);

    //We re-add the item, the number of paint should increase
    scene2.addItem(&tester2);
    tester2.update();
    qApp->processEvents();
    QTRY_COMPARE(tester2.painted, 3);
}

class HarakiriItem : public QGraphicsRectItem
{
public:
    HarakiriItem(int harakiriPoint)
        : QGraphicsRectItem(QRectF(0, 0, 100, 100)), harakiri(harakiriPoint)
    { dead = 0; }

    static int dead;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QGraphicsRectItem::paint(painter, option, widget);
        if (harakiri == 0) {
            // delete unsupported since 4.5
            /*
            dead = 1;
            delete this;
            */
        }
    }

    void advance(int n)
    {
        if (harakiri == 1 && n == 0) {
            // delete unsupported
            /*
            dead = 1;
            delete this;
            */
        }
        if (harakiri == 2 && n == 1) {
            dead = 1;
            delete this;
        }
    }

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *)
    {
        if (harakiri == 3) {
            dead = 1;
            delete this;
        }
    }

    void dragEnterEvent(QGraphicsSceneDragDropEvent *event)
    {
        // ??
        QGraphicsRectItem::dragEnterEvent(event);
    }

    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
    {
        // ??
        QGraphicsRectItem::dragLeaveEvent(event);
    }

    void dragMoveEvent(QGraphicsSceneDragDropEvent *event)
    {
        // ??
        QGraphicsRectItem::dragMoveEvent(event);
    }

    void dropEvent(QGraphicsSceneDragDropEvent *event)
    {
        // ??
        QGraphicsRectItem::dropEvent(event);
    }

    void focusInEvent(QFocusEvent *)
    {
        if (harakiri == 4) {
            dead = 1;
            delete this;
        }
    }

    void focusOutEvent(QFocusEvent *)
    {
        if (harakiri == 5) {
            dead = 1;
            delete this;
        }
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent *)
    {
        if (harakiri == 6) {
            dead = 1;
            delete this;
        }
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent *)
    {
        if (harakiri == 7) {
            dead = 1;
            delete this;
        }
    }

    void hoverMoveEvent(QGraphicsSceneHoverEvent *)
    {
        if (harakiri == 8) {
            dead = 1;
            delete this;
        }
    }

    void inputMethodEvent(QInputMethodEvent *event)
    {
        // ??
        QGraphicsRectItem::inputMethodEvent(event);
    }

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const
    {
        // ??
        return QGraphicsRectItem::inputMethodQuery(query);
    }

    QVariant itemChange(GraphicsItemChange change, const QVariant &value)
    {
        // deletion not supported
        return QGraphicsRectItem::itemChange(change, value);
    }

    void keyPressEvent(QKeyEvent *)
    {
        if (harakiri == 9) {
            dead = 1;
            delete this;
        }
    }

    void keyReleaseEvent(QKeyEvent *)
    {
        if (harakiri == 10) {
            dead = 1;
            delete this;
        }
    }

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *)
    {
        if (harakiri == 11) {
            dead = 1;
            delete this;
        }
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *)
    {
        if (harakiri == 12) {
            dead = 1;
            delete this;
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *)
    {
        if (harakiri == 13) {
            dead = 1;
            delete this;
        }
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *)
    {
        if (harakiri == 14) {
            dead = 1;
            delete this;
        }
    }

    bool sceneEvent(QEvent *event)
    {
        // deletion not supported
        return QGraphicsRectItem::sceneEvent(event);
    }

    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event)
    {
        // deletion not supported
        return QGraphicsRectItem::sceneEventFilter(watched, event);
    }

    void wheelEvent(QGraphicsSceneWheelEvent *)
    {
        if (harakiri == 16) {
            dead = 1;
            delete this;
        }
    }

private:
    int harakiri;
};

int HarakiriItem::dead;

void tst_QGraphicsItem::deleteItemInEventHandlers()
{
    for (int i = 0; i < 17; ++i) {
        QGraphicsScene scene;
        HarakiriItem *item = new HarakiriItem(i);
        item->setAcceptsHoverEvents(true);
        item->setFlag(QGraphicsItem::ItemIsFocusable);

        scene.addItem(item);

        item->installSceneEventFilter(item); // <- ehey!

        QGraphicsView view(&scene);
        view.show();

        qApp->processEvents();
        qApp->processEvents();

        if (!item->dead)
            scene.advance();

        if (!item->dead) {
            QContextMenuEvent event(QContextMenuEvent::Other,
                                    view.mapFromScene(item->scenePos()));
            QCoreApplication::sendEvent(view.viewport(), &event);
        }
        if (!item->dead)
            QTest::mouseMove(view.viewport(), view.mapFromScene(item->scenePos()));
        if (!item->dead)
            QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item->scenePos()));
        if (!item->dead)
            QTest::mouseDClick(view.viewport(), Qt::LeftButton, 0, view.mapFromScene(item->scenePos()));
        if (!item->dead)
            QTest::mouseClick(view.viewport(), Qt::RightButton, 0, view.mapFromScene(item->scenePos()));
        if (!item->dead)
            QTest::mouseMove(view.viewport(), view.mapFromScene(item->scenePos() + QPointF(20, -20)));
        if (!item->dead)
            item->setFocus();
        if (!item->dead)
            item->clearFocus();
        if (!item->dead)
            item->setFocus();
        if (!item->dead)
            QTest::keyPress(view.viewport(), Qt::Key_A);
        if (!item->dead)
            QTest::keyRelease(view.viewport(), Qt::Key_A);
        if (!item->dead)
            QTest::keyPress(view.viewport(), Qt::Key_A);
        if (!item->dead)
            QTest::keyRelease(view.viewport(), Qt::Key_A);
    }
}

class ItemPaintsOutsideShape : public QGraphicsItem
{
public:
    QRectF boundingRect() const
    {
        return QRectF(0, 0, 100, 100);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
    {
        painter->fillRect(-50, -50, 200, 200, Qt::red);
        painter->fillRect(0, 0, 100, 100, Qt::blue);
    }
};

void tst_QGraphicsItem::itemClipsToShape()
{
    QGraphicsItem *clippedItem = new ItemPaintsOutsideShape;
    clippedItem->setFlag(QGraphicsItem::ItemClipsToShape);

    QGraphicsItem *unclippedItem = new ItemPaintsOutsideShape;
    unclippedItem->setPos(200, 0);

    QGraphicsScene scene(-50, -50, 400, 200);
    scene.addItem(clippedItem);
    scene.addItem(unclippedItem);

    QImage image(400, 200, QImage::Format_ARGB32_Premultiplied);
    image.fill(0);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(45, 100), QRgb(0));
    QCOMPARE(image.pixel(100, 45), QRgb(0));
    QCOMPARE(image.pixel(155, 100), QRgb(0));
    QCOMPARE(image.pixel(45, 155), QRgb(0));
    QCOMPARE(image.pixel(55, 100), QColor(Qt::blue).rgba());
    QCOMPARE(image.pixel(100, 55), QColor(Qt::blue).rgba());
    QCOMPARE(image.pixel(145, 100), QColor(Qt::blue).rgba());
    QCOMPARE(image.pixel(55, 145), QColor(Qt::blue).rgba());
    QCOMPARE(image.pixel(245, 100), QColor(Qt::red).rgba());
    QCOMPARE(image.pixel(300, 45), QColor(Qt::red).rgba());
    QCOMPARE(image.pixel(355, 100), QColor(Qt::red).rgba());
    QCOMPARE(image.pixel(245, 155), QColor(Qt::red).rgba());
    QCOMPARE(image.pixel(255, 100), QColor(Qt::blue).rgba());
    QCOMPARE(image.pixel(300, 55), QColor(Qt::blue).rgba());
    QCOMPARE(image.pixel(345, 100), QColor(Qt::blue).rgba());
    QCOMPARE(image.pixel(255, 145), QColor(Qt::blue).rgba());
}

void tst_QGraphicsItem::itemClipsChildrenToShape()
{
    QGraphicsScene scene;
    QGraphicsItem *rect = scene.addRect(0, 0, 50, 50, QPen(Qt::NoPen), QBrush(Qt::yellow));

    QGraphicsItem *ellipse = scene.addEllipse(0, 0, 100, 100, QPen(Qt::NoPen), QBrush(Qt::green));
    ellipse->setParentItem(rect);

    QGraphicsItem *clippedEllipse = scene.addEllipse(0, 0, 50, 50, QPen(Qt::NoPen), QBrush(Qt::blue));
    clippedEllipse->setParentItem(ellipse);

    QGraphicsItem *clippedEllipse2 = scene.addEllipse(0, 0, 25, 25, QPen(Qt::NoPen), QBrush(Qt::red));
    clippedEllipse2->setParentItem(clippedEllipse);

    QGraphicsItem *clippedEllipse3 = scene.addEllipse(50, 50, 25, 25, QPen(Qt::NoPen), QBrush(Qt::red));
    clippedEllipse3->setParentItem(clippedEllipse);

    QVERIFY(!(ellipse->flags() & QGraphicsItem::ItemClipsChildrenToShape));
    ellipse->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    QVERIFY((ellipse->flags() & QGraphicsItem::ItemClipsChildrenToShape));

    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(0);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(16, 16), QColor(255, 0, 0).rgba());
    QCOMPARE(image.pixel(32, 32), QColor(0, 0, 255).rgba());
    QCOMPARE(image.pixel(50, 50), QColor(0, 255, 0).rgba());
    QCOMPARE(image.pixel(12, 12), QColor(255, 255, 0).rgba());
    QCOMPARE(image.pixel(60, 60), QColor(255, 0, 0).rgba());
}

void tst_QGraphicsItem::itemClipsChildrenToShape2()
{
    QGraphicsRectItem *parent = new QGraphicsRectItem(QRectF(0, 0, 10, 10));
    QGraphicsEllipseItem *child1 = new QGraphicsEllipseItem(QRectF(50, 50, 100, 100));
    QGraphicsRectItem *child2 = new QGraphicsRectItem(QRectF(15, 15, 80, 80));

    child1->setParentItem(parent);
    child1->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    child2->setParentItem(child1);

    parent->setBrush(Qt::blue);
    child1->setBrush(Qt::green);
    child2->setBrush(Qt::red);

    QGraphicsScene scene;
    scene.addItem(parent);

    QCOMPARE(scene.itemAt(5, 5), (QGraphicsItem *)parent);
    QCOMPARE(scene.itemAt(15, 5), (QGraphicsItem *)0);
    QCOMPARE(scene.itemAt(5, 15), (QGraphicsItem *)0);
    QCOMPARE(scene.itemAt(60, 60), (QGraphicsItem *)0);
    QCOMPARE(scene.itemAt(140, 60), (QGraphicsItem *)0);
    QCOMPARE(scene.itemAt(60, 140), (QGraphicsItem *)0);
    QCOMPARE(scene.itemAt(140, 140), (QGraphicsItem *)0);
    QCOMPARE(scene.itemAt(75, 75), (QGraphicsItem *)child2);
    QCOMPARE(scene.itemAt(75, 100), (QGraphicsItem *)child1);
    QCOMPARE(scene.itemAt(100, 75), (QGraphicsItem *)child1);

    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(0);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(5, 5), QColor(0, 0, 255).rgba());
    QCOMPARE(image.pixel(5, 10), QRgb(0));
    QCOMPARE(image.pixel(10, 5), QRgb(0));
    QCOMPARE(image.pixel(40, 40), QRgb(0));
    QCOMPARE(image.pixel(90, 40), QRgb(0));
    QCOMPARE(image.pixel(40, 90), QRgb(0));
    QCOMPARE(image.pixel(95, 95), QRgb(0));
    QCOMPARE(image.pixel(50, 70), QColor(0, 255, 0).rgba());
    QCOMPARE(image.pixel(70, 50), QColor(0, 255, 0).rgba());
    QCOMPARE(image.pixel(50, 60), QColor(255, 0, 0).rgba());
    QCOMPARE(image.pixel(60, 50), QColor(255, 0, 0).rgba());
}

void tst_QGraphicsItem::itemClipsChildrenToShape3()
{
    // Construct a scene with nested children, each 50 pixels offset from the elder.
    // Set a top-level clipping flag
    QGraphicsScene scene;
    QGraphicsRectItem *parent = scene.addRect( 0, 0, 150, 150 );
    QGraphicsRectItem *child = scene.addRect( 0, 0, 150, 150 );
    QGraphicsRectItem *grandchild = scene.addRect( 0, 0, 150, 150 );
    child->setParentItem(parent);
    grandchild->setParentItem(child);
    child->setPos( 50, 50 );
    grandchild->setPos( 50, 50 );
    parent->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    QCOMPARE(scene.itemAt(25,25), (QGraphicsItem *)parent);
    QCOMPARE(scene.itemAt(75,75), (QGraphicsItem *)child);
    QCOMPARE(scene.itemAt(125,125), (QGraphicsItem *)grandchild);
    QCOMPARE(scene.itemAt(175,175), (QGraphicsItem *)0);

    // Move child to fully overlap the parent.  The grandchild should
    // now occupy two-thirds of the scene
    child->prepareGeometryChange();
    child->setPos( 0, 0 );

    QCOMPARE(scene.itemAt(25,25), (QGraphicsItem *)child);
    QCOMPARE(scene.itemAt(75,75), (QGraphicsItem *)grandchild);
    QCOMPARE(scene.itemAt(125,125), (QGraphicsItem *)grandchild);
    QCOMPARE(scene.itemAt(175,175), (QGraphicsItem *)0);
}

class MyProxyWidget : public QGraphicsProxyWidget
{
public:
    MyProxyWidget(QGraphicsItem *parent) : QGraphicsProxyWidget(parent)
    {
        painted = false;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QGraphicsProxyWidget::paint(painter, option, widget);
        painted = true;
    }
    bool painted;
};

void tst_QGraphicsItem::itemClipsChildrenToShape4()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsWidget * outerWidget = new QGraphicsWidget();
    outerWidget->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
    MyProxyWidget * innerWidget = new MyProxyWidget(outerWidget);
    QLabel * label = new QLabel();
    label->setText("Welcome back my friends to the show that never ends...");
    innerWidget->setWidget(label);
    view.resize(300, 300);
    scene.addItem(outerWidget);
    outerWidget->resize( 200, 100 );
    scene.addEllipse( 100, 100, 100, 50 );   // <-- this is important to trigger the right codepath*
    //now the label is shown
    outerWidget->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false );
    QApplication::setActiveWindow(&view);
    view.show();
    QTRY_COMPARE(QApplication::activeWindow(), (QWidget *)&view);
    QTRY_COMPARE(innerWidget->painted, true);
}

//#define DEBUG_ITEM_CLIPS_CHILDREN_TO_SHAPE_5
static inline void renderSceneToImage(QGraphicsScene *scene, QImage *image, const QString &filename)
{
    image->fill(0);
    QPainter painter(image);
    scene->render(&painter);
    painter.end();
#ifdef DEBUG_ITEM_CLIPS_CHILDREN_TO_SHAPE_5
    image->save(filename);
#else
    Q_UNUSED(filename);
#endif
}

void tst_QGraphicsItem::itemClipsChildrenToShape5()
{
    class ParentItem : public QGraphicsRectItem
    {
    public:
        ParentItem(qreal x, qreal y, qreal width, qreal height)
            : QGraphicsRectItem(x, y, width, height) {}

        QPainterPath shape() const
        {
            QPainterPath path;
            path.addRect(50, 50, 200, 200);
            return path;
        }
    };

    ParentItem *parent = new ParentItem(0, 0, 300, 300);
    parent->setBrush(Qt::blue);
    parent->setOpacity(0.5);

    const QRegion parentRegion(0, 0, 300, 300);
    const QRegion clippedParentRegion = parentRegion & QRect(50, 50, 200, 200);
    QRegion childRegion;
    QRegion grandChildRegion;

    QGraphicsRectItem *topLeftChild = new QGraphicsRectItem(0, 0, 100, 100);
    topLeftChild->setBrush(Qt::red);
    topLeftChild->setParentItem(parent);
    childRegion += QRect(0, 0, 100, 100);

    QGraphicsRectItem *topRightChild = new QGraphicsRectItem(0, 0, 100, 100);
    topRightChild->setBrush(Qt::red);
    topRightChild->setParentItem(parent);
    topRightChild->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    topRightChild->setPos(200, 0);
    childRegion += QRect(200, 0, 100, 100);

    QGraphicsRectItem *topRightGrandChild = new QGraphicsRectItem(0, 0, 100, 100);
    topRightGrandChild->setBrush(Qt::green);
    topRightGrandChild->setParentItem(topRightChild);
    topRightGrandChild->setPos(-40, 40);
    grandChildRegion += QRect(200 - 40, 0 + 40, 100, 100) & QRect(200, 0, 100, 100);

    QGraphicsRectItem *bottomLeftChild = new QGraphicsRectItem(0, 0, 100, 100);
    bottomLeftChild->setBrush(Qt::red);
    bottomLeftChild->setParentItem(parent);
    bottomLeftChild->setFlag(QGraphicsItem::ItemClipsToShape);
    bottomLeftChild->setPos(0, 200);
    childRegion += QRect(0, 200, 100, 100);

    QGraphicsRectItem *bottomLeftGrandChild = new QGraphicsRectItem(0, 0, 160, 160);
    bottomLeftGrandChild->setBrush(Qt::green);
    bottomLeftGrandChild->setParentItem(bottomLeftChild);
    bottomLeftGrandChild->setFlag(QGraphicsItem::ItemClipsToShape);
    bottomLeftGrandChild->setPos(0, -60);
    grandChildRegion += QRect(0, 200 - 60, 160, 160);

    QGraphicsRectItem *bottomRightChild = new QGraphicsRectItem(0, 0, 100, 100);
    bottomRightChild->setBrush(Qt::red);
    bottomRightChild->setParentItem(parent);
    bottomRightChild->setPos(200, 200);
    childRegion += QRect(200, 200, 100, 100);

    QPoint controlPoints[17] = {
        QPoint(5, 5)  , QPoint(95, 5)  , QPoint(205, 5)  , QPoint(295, 5)  ,
        QPoint(5, 95) , QPoint(95, 95) , QPoint(205, 95) , QPoint(295, 95) ,
                             QPoint(150, 150),
        QPoint(5, 205), QPoint(95, 205), QPoint(205, 205), QPoint(295, 205),
        QPoint(5, 295), QPoint(95, 295), QPoint(205, 295), QPoint(295, 295),
    };

    const QRegion clippedChildRegion = childRegion & QRect(50, 50, 200, 200);
    const QRegion clippedGrandChildRegion = grandChildRegion & QRect(50, 50, 200, 200);

    QGraphicsScene scene;
    scene.addItem(parent);
    QImage sceneImage(300, 300, QImage::Format_ARGB32);

#define VERIFY_CONTROL_POINTS(pRegion, cRegion, gRegion) \
    for (int i = 0; i < 17; ++i) { \
        QPoint controlPoint = controlPoints[i]; \
        QRgb pixel = sceneImage.pixel(controlPoint.x(), controlPoint.y()); \
        if (pRegion.contains(controlPoint)) \
            QVERIFY(qBlue(pixel) != 0); \
        else \
            QVERIFY(qBlue(pixel) == 0); \
        if (cRegion.contains(controlPoint)) \
            QVERIFY(qRed(pixel) != 0); \
        else \
            QVERIFY(qRed(pixel) == 0); \
        if (gRegion.contains(controlPoint)) \
            QVERIFY(qGreen(pixel) != 0); \
        else \
            QVERIFY(qGreen(pixel) == 0); \
    }

    const QList<QGraphicsItem *> children = parent->childItems();
    const int childrenCount = children.count();

    for (int i = 0; i < 5; ++i) {
        QString clipString;
        QString childString;
        switch (i) {
        case 0:
            // All children stacked in front.
            childString = QLatin1String("ChildrenInFront.png");
            foreach (QGraphicsItem *child, children)
                child->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
            break;
        case 1:
            // All children stacked behind.
            childString = QLatin1String("ChildrenBehind.png");
            foreach (QGraphicsItem *child, children)
                child->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
            break;
        case 2:
            // First half of the children behind, second half in front.
            childString = QLatin1String("FirstHalfBehind_SecondHalfInFront.png");
            for (int j = 0; j < childrenCount; ++j) {
                QGraphicsItem *child = children.at(j);
                child->setFlag(QGraphicsItem::ItemStacksBehindParent, (j < childrenCount / 2));
            }
            break;
        case 3:
            // First half of the children in front, second half behind.
            childString = QLatin1String("FirstHalfInFront_SecondHalfBehind.png");
            for (int j = 0; j < childrenCount; ++j) {
                QGraphicsItem *child = children.at(j);
                child->setFlag(QGraphicsItem::ItemStacksBehindParent, (j >= childrenCount / 2));
            }
            break;
        case 4:
            // Child2 and child4 behind, rest in front.
            childString = QLatin1String("Child2And4Behind_RestInFront.png");
            for (int j = 0; j < childrenCount; ++j) {
                QGraphicsItem *child = children.at(j);
                if (j == 1 || j == 3)
                    child->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
                else
                    child->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
            }
            break;
        default:
            qFatal("internal error");
        }

        // Nothing is clipped.
        parent->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
        parent->setFlag(QGraphicsItem::ItemClipsToShape, false);
        clipString = QLatin1String("nothingClipped_");
        renderSceneToImage(&scene, &sceneImage, clipString + childString);
        VERIFY_CONTROL_POINTS(parentRegion, childRegion, grandChildRegion);

        // Parent clips children to shape.
        parent->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
        clipString = QLatin1String("parentClipsChildrenToShape_");
        renderSceneToImage(&scene, &sceneImage, clipString + childString);
        VERIFY_CONTROL_POINTS(parentRegion, clippedChildRegion, clippedGrandChildRegion);

        // Parent clips itself and children to shape.
        parent->setFlag(QGraphicsItem::ItemClipsToShape);
        clipString = QLatin1String("parentClipsItselfAndChildrenToShape_");
        renderSceneToImage(&scene, &sceneImage, clipString + childString);
        VERIFY_CONTROL_POINTS(clippedParentRegion, clippedChildRegion, clippedGrandChildRegion);

        // Parent clips itself to shape.
        parent->setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
        clipString = QLatin1String("parentClipsItselfToShape_");
        renderSceneToImage(&scene, &sceneImage, clipString + childString);
        VERIFY_CONTROL_POINTS(clippedParentRegion, childRegion, grandChildRegion);
    }
}

void tst_QGraphicsItem::itemClipsTextChildToShape()
{
    // Construct a scene with a rect that clips its children, with one text
    // child that has text that exceeds the size of the rect.
    QGraphicsScene scene;
    QGraphicsItem *rect = scene.addRect(0, 0, 50, 50, QPen(Qt::black), Qt::black);
    rect->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    QGraphicsTextItem *text = new QGraphicsTextItem("This is a long sentence that's wider than 50 pixels.");
    text->setParentItem(rect);

    // Render this scene to a transparent image.
    QRectF sr = scene.itemsBoundingRect();
    QImage image(sr.size().toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(0);
    QPainter painter(&image);
    scene.render(&painter);

    // Erase the area immediately underneath the rect.
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect->sceneBoundingRect().translated(-sr.topLeft()).adjusted(-0.5, -0.5, 0.5, 0.5),
                     Qt::transparent);
    painter.end();

    // Check that you get a truly transparent image back (i.e., the text was
    // clipped away, so there should be no trails left after erasing only the
    // rect's area).
    QImage emptyImage(scene.itemsBoundingRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    emptyImage.fill(0);
    QCOMPARE(image, emptyImage);
}

void tst_QGraphicsItem::itemClippingDiscovery()
{
    // A simple scene with an ellipse parent and two rect children, one a
    // child of the other.
    QGraphicsScene scene;
    QGraphicsEllipseItem *clipItem = scene.addEllipse(0, 0, 100, 100);
    QGraphicsRectItem *leftRectItem = scene.addRect(0, 0, 50, 100);
    QGraphicsRectItem *rightRectItem = scene.addRect(50, 0, 50, 100);
    leftRectItem->setParentItem(clipItem);
    rightRectItem->setParentItem(clipItem);

    // The rects item are both visible at these points.
    QCOMPARE(scene.itemAt(10, 10), (QGraphicsItem *)leftRectItem);
    QCOMPARE(scene.itemAt(90, 90), (QGraphicsItem *)rightRectItem);

    // The ellipse clips the rects now.
    clipItem->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    // The rect items are no longer visible at these points.
    QCOMPARE(scene.itemAt(10, 10), (QGraphicsItem *)0);
    if (sizeof(qreal) != sizeof(double))
        QSKIP("This fails due to internal rounding errors");
    QCOMPARE(scene.itemAt(90, 90), (QGraphicsItem *)0);
}

class ItemCountsBoundingRectCalls : public QGraphicsRectItem
{
public:
    ItemCountsBoundingRectCalls(const QRectF & rect, QGraphicsItem *parent = 0)
        : QGraphicsRectItem(rect, parent), boundingRectCalls(0) {}
    QRectF boundingRect () const {
        ++boundingRectCalls;
        return QGraphicsRectItem::boundingRect();
    }
    mutable int boundingRectCalls;
};

void tst_QGraphicsItem::itemContainsChildrenInShape()
{
    ItemCountsBoundingRectCalls *parent = new ItemCountsBoundingRectCalls(QRectF(0,0, 10, 10));
    ItemCountsBoundingRectCalls *childOutsideShape = new ItemCountsBoundingRectCalls(QRectF(0,0, 10, 10), parent);
    childOutsideShape->setPos(20,0);

    QGraphicsScene scene;
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    scene.addItem(parent);

    QCOMPARE(parent->boundingRectCalls, childOutsideShape->boundingRectCalls);

    int oldParentBoundingRectCalls = parent->boundingRectCalls;
    int oldChildBoundingRectCalls = childOutsideShape->boundingRectCalls;

    // First test that both items are searched if no optimization flags are set
    QGraphicsItem* item = scene.itemAt(25,5);

    QCOMPARE(item, childOutsideShape);
    QVERIFY(parent->boundingRectCalls > oldParentBoundingRectCalls);
    QVERIFY(childOutsideShape->boundingRectCalls > oldChildBoundingRectCalls);
    QCOMPARE(parent->boundingRectCalls, childOutsideShape->boundingRectCalls);

    oldParentBoundingRectCalls = parent->boundingRectCalls;
    oldChildBoundingRectCalls = childOutsideShape->boundingRectCalls;

    // Repeat the test to make sure that no caching/indexing is in effect
    item = scene.itemAt(25,5);

    QCOMPARE(item, childOutsideShape);
    QVERIFY(parent->boundingRectCalls > oldParentBoundingRectCalls);
    QVERIFY(childOutsideShape->boundingRectCalls > oldChildBoundingRectCalls);
    QCOMPARE(parent->boundingRectCalls, childOutsideShape->boundingRectCalls);

    oldParentBoundingRectCalls = parent->boundingRectCalls;
    oldChildBoundingRectCalls = childOutsideShape->boundingRectCalls;

    // Set the optimization flag and make sure that the child is not returned
    // and that the child's boundingRect() method is never called.
    parent->setFlag(QGraphicsItem::ItemContainsChildrenInShape);
    item = scene.itemAt(25,5);

    QVERIFY(!(item));
    QVERIFY(parent->boundingRectCalls > oldParentBoundingRectCalls);
    QCOMPARE(childOutsideShape->boundingRectCalls, oldChildBoundingRectCalls);
    QVERIFY(parent->boundingRectCalls > childOutsideShape->boundingRectCalls);
}

void tst_QGraphicsItem::itemContainsChildrenInShape2()
{
    //The tested flag behaves almost identically to ItemClipsChildrenToShape
    //in terms of optimizations but does not enforce the clip.
    //This test makes sure there is no clip.
    QGraphicsScene scene;
    QGraphicsItem *rect = scene.addRect(0, 0, 50, 50, QPen(Qt::NoPen), QBrush(Qt::yellow));

    QGraphicsItem *ellipse = scene.addEllipse(0, 0, 100, 100, QPen(Qt::NoPen), QBrush(Qt::green));
    ellipse->setParentItem(rect);

    QGraphicsItem *clippedEllipse = scene.addEllipse(0, 0, 50, 50, QPen(Qt::NoPen), QBrush(Qt::blue));
    clippedEllipse->setParentItem(ellipse);

    QGraphicsItem *clippedEllipse2 = scene.addEllipse(0, 0, 25, 25, QPen(Qt::NoPen), QBrush(Qt::red));
    clippedEllipse2->setParentItem(clippedEllipse);

    QVERIFY(!(ellipse->flags() & QGraphicsItem::ItemClipsChildrenToShape));
    QVERIFY(!(ellipse->flags() & QGraphicsItem::ItemContainsChildrenInShape));
    ellipse->setFlags(QGraphicsItem::ItemContainsChildrenInShape);
    QVERIFY(!(ellipse->flags() & QGraphicsItem::ItemClipsChildrenToShape));
    QVERIFY((ellipse->flags() & QGraphicsItem::ItemContainsChildrenInShape));

    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(0);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();

    QCOMPARE(image.pixel(2, 2), QColor(Qt::yellow).rgba());
    QCOMPARE(image.pixel(12, 12), QColor(Qt::red).rgba());
    QCOMPARE(image.pixel(2, 25), QColor(Qt::blue).rgba());
    QCOMPARE(image.pixel(2, 50), QColor(Qt::green).rgba());
}

void tst_QGraphicsItem::ancestorFlags()
{
    QGraphicsItem *level1 = new QGraphicsRectItem;
    QGraphicsItem *level21 = new QGraphicsRectItem;
    level21->setParentItem(level1);
    QGraphicsItem *level22 = new QGraphicsRectItem;
    level22->setParentItem(level1);
    QGraphicsItem *level31 = new QGraphicsRectItem;
    level31->setParentItem(level21);
    QGraphicsItem *level32 = new QGraphicsRectItem;
    level32->setParentItem(level21);

    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 0);

    // HandlesChildEvents: 1) Root level sets a flag
    level1->setHandlesChildEvents(true);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // HandlesChildEvents: 2) Root level set it again
    level1->setHandlesChildEvents(true);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // HandlesChildEvents: 3) Root level unsets a flag
    level1->setHandlesChildEvents(false);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 0);

    // HandlesChildEvents: 4) Child item sets a flag
    level21->setHandlesChildEvents(true);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // HandlesChildEvents: 5) Parent item sets a flag
    level1->setHandlesChildEvents(true);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // HandlesChildEvents: 6) Child item unsets a flag
    level21->setHandlesChildEvents(false);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // HandlesChildEvents: 7) Parent item unsets a flag
    level21->setHandlesChildEvents(true);
    level1->setHandlesChildEvents(false);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // Reparent the child to root
    level21->setParentItem(0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // Reparent the child to level1 again.
    level1->setHandlesChildEvents(true);
    level21->setParentItem(level1);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // Reparenting level31 back to level1.
    level31->setParentItem(level1);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // Reparenting level31 back to level21.
    level31->setParentItem(0);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 0);
    level31->setParentItem(level21);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // Level1 doesn't handle child events
    level1->setHandlesChildEvents(false);
    QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
    QCOMPARE(int(level31->d_ptr->ancestorFlags), 1);
    QCOMPARE(int(level32->d_ptr->ancestorFlags), 1);

    // Nobody handles child events
    level21->setHandlesChildEvents(false);

    for (int i = 0; i < 3; ++i) {
        QGraphicsItem::GraphicsItemFlag flag;
        int ancestorFlag;

        switch (i) {
        case(0):
            flag = QGraphicsItem::ItemClipsChildrenToShape;
            ancestorFlag = QGraphicsItemPrivate::AncestorClipsChildren;
            break;
        case(1):
            flag = QGraphicsItem::ItemIgnoresTransformations;
            ancestorFlag = QGraphicsItemPrivate::AncestorIgnoresTransformations;
            break;
        case(2):
            flag = QGraphicsItem::ItemContainsChildrenInShape;
            ancestorFlag = QGraphicsItemPrivate::AncestorContainsChildren;
            break;
        default:
            qFatal("Unknown ancestor flag, please fix!");
            break;
        }

        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), 0);

        // HandlesChildEvents: 1) Root level sets a flag
        level1->setFlag(flag, true);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // HandlesChildEvents: 2) Root level set it again
        level1->setFlag(flag, true);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // HandlesChildEvents: 3) Root level unsets a flag
        level1->setFlag(flag, false);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), 0);

        // HandlesChildEvents: 4) Child item sets a flag
        level21->setFlag(flag, true);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // HandlesChildEvents: 5) Parent item sets a flag
        level1->setFlag(flag, true);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // HandlesChildEvents: 6) Child item unsets a flag
        level21->setFlag(flag, false);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // HandlesChildEvents: 7) Parent item unsets a flag
        level21->setFlag(flag, true);
        level1->setFlag(flag, false);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // Reparent the child to root
        level21->setParentItem(0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // Reparent the child to level1 again.
        level1->setFlag(flag, true);
        level21->setParentItem(level1);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // Reparenting level31 back to level1.
        level31->setParentItem(level1);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // Reparenting level31 back to level21.
        level31->setParentItem(0);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), 0);
        level31->setParentItem(level21);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // Level1 doesn't handle child events
        level1->setFlag(flag, false);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), ancestorFlag);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), ancestorFlag);

        // Nobody handles child events
        level21->setFlag(flag, false);
        QCOMPARE(int(level1->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level21->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level22->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level31->d_ptr->ancestorFlags), 0);
        QCOMPARE(int(level32->d_ptr->ancestorFlags), 0);
    }

    delete level1;
}

void tst_QGraphicsItem::untransformable()
{
    QGraphicsItem *item1 = new QGraphicsEllipseItem(QRectF(-50, -50, 100, 100));
    item1->setZValue(1);
    item1->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    item1->rotate(45);
    ((QGraphicsEllipseItem *)item1)->setBrush(Qt::red);

    QGraphicsItem *item2 = new QGraphicsEllipseItem(QRectF(-50, -50, 100, 100));
    item2->setParentItem(item1);
    item2->rotate(45);
    item2->setPos(100, 0);
    ((QGraphicsEllipseItem *)item2)->setBrush(Qt::green);

    QGraphicsItem *item3 = new QGraphicsEllipseItem(QRectF(-50, -50, 100, 100));
    item3->setParentItem(item2);
    item3->setPos(100, 0);
    ((QGraphicsEllipseItem *)item3)->setBrush(Qt::blue);

    QGraphicsScene scene(-500, -500, 1000, 1000);
    scene.addItem(item1);

    QWidget topLevel;
    QGraphicsView view(&scene,&topLevel);
    view.resize(300, 300);
    topLevel.show();
    view.scale(8, 8);
    view.centerOn(0, 0);

// Painting with the DiagCrossPattern is really slow on Mac
// when zoomed out. (The test times out). Task to fix is 155567.
#if !defined(Q_OS_MAC) || 1
    view.setBackgroundBrush(QBrush(Qt::black, Qt::DiagCrossPattern));
#endif

    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    for (int i = 0; i < 10; ++i) {
        QPoint center = view.viewport()->rect().center();
        QCOMPARE(view.itemAt(center), item1);
        QCOMPARE(view.itemAt(center - QPoint(40, 0)), item1);
        QCOMPARE(view.itemAt(center - QPoint(-40, 0)), item1);
        QCOMPARE(view.itemAt(center - QPoint(0, 40)), item1);
        QCOMPARE(view.itemAt(center - QPoint(0, -40)), item1);

        center += QPoint(70, 70);
        QCOMPARE(view.itemAt(center - QPoint(40, 0)), item2);
        QCOMPARE(view.itemAt(center - QPoint(-40, 0)), item2);
        QCOMPARE(view.itemAt(center - QPoint(0, 40)), item2);
        QCOMPARE(view.itemAt(center - QPoint(0, -40)), item2);

        center += QPoint(0, 100);
        QCOMPARE(view.itemAt(center - QPoint(40, 0)), item3);
        QCOMPARE(view.itemAt(center - QPoint(-40, 0)), item3);
        QCOMPARE(view.itemAt(center - QPoint(0, 40)), item3);
        QCOMPARE(view.itemAt(center - QPoint(0, -40)), item3);

        view.scale(0.5, 0.5);
        view.rotate(13);
        view.shear(qreal(0.01), qreal(0.01));
        view.translate(10, 10);
        QTest::qWait(25);
    }
}

class ContextMenuItem : public QGraphicsRectItem
{
public:
    ContextMenuItem()
        : ignoreEvent(true), gotEvent(false), eventWasAccepted(false)
    { }
    bool ignoreEvent;
    bool gotEvent;
    bool eventWasAccepted;
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
    {
        gotEvent = true;
        eventWasAccepted = event->isAccepted();
        if (ignoreEvent)
            event->ignore();
    }
};

void tst_QGraphicsItem::contextMenuEventPropagation()
{
    ContextMenuItem *bottomItem = new ContextMenuItem;
    bottomItem->setRect(0, 0, 100, 100);
    ContextMenuItem *topItem = new ContextMenuItem;
    topItem->setParentItem(bottomItem);
    topItem->setRect(0, 0, 100, 100);

    QGraphicsScene scene;

    QGraphicsView view(&scene);
    view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    view.show();
    view.resize(200, 200);
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(20);

    QContextMenuEvent event(QContextMenuEvent::Mouse, QPoint(10, 10),
                            view.viewport()->mapToGlobal(QPoint(10, 10)));
    event.ignore();
    QApplication::sendEvent(view.viewport(), &event);
    QVERIFY(!event.isAccepted());

    scene.addItem(bottomItem);
    topItem->ignoreEvent = true;
    bottomItem->ignoreEvent = true;

    QApplication::sendEvent(view.viewport(), &event);
    QVERIFY(!event.isAccepted());
    QCOMPARE(topItem->gotEvent, true);
    QCOMPARE(topItem->eventWasAccepted, true);
    QCOMPARE(bottomItem->gotEvent, true);
    QCOMPARE(bottomItem->eventWasAccepted, true);

    topItem->ignoreEvent = false;
    topItem->gotEvent = false;
    bottomItem->gotEvent = false;

    QApplication::sendEvent(view.viewport(), &event);
    QVERIFY(event.isAccepted());
    QCOMPARE(topItem->gotEvent, true);
    QCOMPARE(bottomItem->gotEvent, false);
    QCOMPARE(topItem->eventWasAccepted, true);
}

void tst_QGraphicsItem::itemIsMovable()
{
    QGraphicsRectItem *rect = new QGraphicsRectItem(-50, -50, 100, 100);
    rect->setFlag(QGraphicsItem::ItemIsMovable);

    QGraphicsScene scene;
    scene.addItem(rect);

    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
        event.setButton(Qt::LeftButton);
        event.setButtons(Qt::LeftButton);
        qApp->sendEvent(&scene, &event);
    }
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseMove);
        event.setButton(Qt::LeftButton);
        event.setButtons(Qt::LeftButton);
        qApp->sendEvent(&scene, &event);
    }
    QCOMPARE(rect->pos(), QPointF(0, 0));
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseMove);
        event.setButtons(Qt::LeftButton);
        event.setScenePos(QPointF(10, 10));
        qApp->sendEvent(&scene, &event);
    }
    QCOMPARE(rect->pos(), QPointF(10, 10));
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseMove);
        event.setButtons(Qt::RightButton);
        event.setScenePos(QPointF(20, 20));
        qApp->sendEvent(&scene, &event);
    }
    QCOMPARE(rect->pos(), QPointF(10, 10));
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseMove);
        event.setButtons(Qt::LeftButton);
        event.setScenePos(QPointF(30, 30));
        qApp->sendEvent(&scene, &event);
    }
    QCOMPARE(rect->pos(), QPointF(30, 30));
}

class ItemAddScene : public QGraphicsScene
{
    Q_OBJECT
public:
    ItemAddScene()
    {
        QTimer::singleShot(500, this, SLOT(newTextItem()));
    }

public slots:
    void newTextItem()
    {
        // Add a text item
        QGraphicsItem *item = new QGraphicsTextItem("This item will not ensure that it's visible", 0);
        item->setPos(.0, .0);
        item->show();
    }
};

void tst_QGraphicsItem::task141694_textItemEnsureVisible()
{
    ItemAddScene scene;
    scene.setSceneRect(-1000, -1000, 2000, 2000);

    QGraphicsView view(&scene);
    view.setFixedSize(200, 200);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    view.ensureVisible(-1000, -1000, 5, 5);
    int hscroll = view.horizontalScrollBar()->value();
    int vscroll = view.verticalScrollBar()->value();

    QTest::qWait(10);

    // This should not cause the view to scroll
    QTRY_COMPARE(view.horizontalScrollBar()->value(), hscroll);
    QCOMPARE(view.verticalScrollBar()->value(), vscroll);
}

void tst_QGraphicsItem::task128696_textItemEnsureMovable()
{
    QGraphicsTextItem *item = new QGraphicsTextItem;
    item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
    item->setTextInteractionFlags(Qt::TextEditorInteraction);
    item->setPlainText("abc de\nf ghi\n   j k l");

    QGraphicsScene scene;
    scene.setSceneRect(-100, -100, 200, 200);
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.setFixedSize(200, 200);
    view.show();

    QGraphicsSceneMouseEvent event1(QEvent::GraphicsSceneMousePress);
    event1.setScenePos(QPointF(0, 0));
    event1.setButton(Qt::LeftButton);
    event1.setButtons(Qt::LeftButton);
    QApplication::sendEvent(&scene, &event1);
    QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)item);

    QGraphicsSceneMouseEvent event2(QEvent::GraphicsSceneMouseMove);
    event2.setScenePos(QPointF(10, 10));
    event2.setButton(Qt::LeftButton);
    event2.setButtons(Qt::LeftButton);
    QApplication::sendEvent(&scene, &event2);
    QCOMPARE(item->pos(), QPointF(10, 10));
}

void tst_QGraphicsItem::task177918_lineItemUndetected()
{
    QGraphicsScene scene;
    QGraphicsLineItem *line = scene.addLine(10, 10, 10, 10);
    QCOMPARE(line->boundingRect(), QRectF(10, 10, 0, 0));

    QVERIFY(!scene.items(9, 9, 2, 2, Qt::IntersectsItemShape).isEmpty());
    QVERIFY(!scene.items(9, 9, 2, 2, Qt::ContainsItemShape).isEmpty());
    QVERIFY(!scene.items(9, 9, 2, 2, Qt::IntersectsItemBoundingRect).isEmpty());
    QVERIFY(!scene.items(9, 9, 2, 2, Qt::ContainsItemBoundingRect).isEmpty());
}

void tst_QGraphicsItem::task240400_clickOnTextItem_data()
{
    QTest::addColumn<int>("flags");
    QTest::addColumn<int>("textFlags");
    QTest::newRow("editor, noflags") << 0 << int(Qt::TextEditorInteraction);
    QTest::newRow("editor, movable") << int(QGraphicsItem::ItemIsMovable) << int(Qt::TextEditorInteraction);
    QTest::newRow("editor, selectable") << int(QGraphicsItem::ItemIsSelectable) << int(Qt::TextEditorInteraction);
    QTest::newRow("editor, movable | selectable") << int(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable)
                                                  << int(Qt::TextEditorInteraction);
    QTest::newRow("noninteractive, noflags") << 0 << int(Qt::NoTextInteraction);
    QTest::newRow("noninteractive, movable") << int(QGraphicsItem::ItemIsMovable) << int(Qt::NoTextInteraction);
    QTest::newRow("noninteractive, selectable") << int(QGraphicsItem::ItemIsSelectable) << int(Qt::NoTextInteraction);
    QTest::newRow("noninteractive, movable | selectable") << int(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable)
                                                          << int(Qt::NoTextInteraction);
}

void tst_QGraphicsItem::task240400_clickOnTextItem()
{
    QFETCH(int, flags);
    QFETCH(int, textFlags);

    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    QGraphicsTextItem *item = scene.addText("Hello");
    item->setFlags(QGraphicsItem::GraphicsItemFlags(flags));
    item->setTextInteractionFlags(Qt::TextInteractionFlags(textFlags));
    bool focusable = (item->flags() & QGraphicsItem::ItemIsFocusable);
    QVERIFY(textFlags ? focusable : !focusable);

    int column = item->textCursor().columnNumber();
    QCOMPARE(column, 0);

    QVERIFY(!item->hasFocus());

    // Click in the top-left corner of the item
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
        event.setScenePos(item->sceneBoundingRect().topLeft() + QPointF(0.1, 0.1));
        event.setButton(Qt::LeftButton);
        event.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &event);
    }
    if (flags || textFlags)
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)item);
    else
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)0);
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseRelease);
        event.setScenePos(item->sceneBoundingRect().topLeft() + QPointF(0.1, 0.1));
        event.setButton(Qt::LeftButton);
        event.setButtons(0);
        QApplication::sendEvent(&scene, &event);
    }
    if (textFlags)
        QVERIFY(item->hasFocus());
    else
        QVERIFY(!item->hasFocus());
    QVERIFY(!scene.mouseGrabberItem());
    bool selectable = (flags & QGraphicsItem::ItemIsSelectable);
    QVERIFY(selectable ? item->isSelected() : !item->isSelected());

    // Now click in the middle and check that the cursor moved.
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
        event.setScenePos(item->sceneBoundingRect().center());
        event.setButton(Qt::LeftButton);
        event.setButtons(Qt::LeftButton);
        QApplication::sendEvent(&scene, &event);
    }
    if (flags || textFlags)
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)item);
    else
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *)0);
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseRelease);
        event.setScenePos(item->sceneBoundingRect().center());
        event.setButton(Qt::LeftButton);
        event.setButtons(0);
        QApplication::sendEvent(&scene, &event);
    }
    if (textFlags)
        QVERIFY(item->hasFocus());
    else
        QVERIFY(!item->hasFocus());
    QVERIFY(!scene.mouseGrabberItem());

    QVERIFY(selectable ? item->isSelected() : !item->isSelected());

    //
    if (textFlags & Qt::TextEditorInteraction)
        QVERIFY(item->textCursor().columnNumber() > column);
    else
        QCOMPARE(item->textCursor().columnNumber(), 0);
}

class TextItem : public QGraphicsSimpleTextItem
{
public:
    TextItem(const QString& text) : QGraphicsSimpleTextItem(text)
    {
        updates = 0;
    }

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
    {
        updates++;
        QGraphicsSimpleTextItem::paint(painter, option, widget);
    }

    int updates;
};

void tst_QGraphicsItem::ensureUpdateOnTextItem()
{
#ifdef Q_OS_MAC
    if (QSysInfo::MacintoshVersion == QSysInfo::MV_10_7) {
        QSKIP("This test is unstable on 10.7 in CI");
    }
#endif

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(25);
    TextItem *text1 = new TextItem(QLatin1String("123"));
    scene.addItem(text1);
    qApp->processEvents();
    QTRY_COMPARE(text1->updates,1);

    //same bouding rect but we have to update
    text1->setText(QLatin1String("321"));
    qApp->processEvents();
    QTRY_COMPARE(text1->updates,2);
}

void tst_QGraphicsItem::task243707_addChildBeforeParent()
{
    // Task reports that adding the child before the parent leads to an
    // inconsistent internal state that can cause a crash.  This test shows
    // one such crash.
    QGraphicsScene scene;
    QGraphicsWidget *widget = new QGraphicsWidget;
    QGraphicsWidget *widget2 = new QGraphicsWidget(widget);
    scene.addItem(widget2);
    QVERIFY(!widget2->parentItem());
    scene.addItem(widget);
    QVERIFY(!widget->commonAncestorItem(widget2));
    QVERIFY(!widget2->commonAncestorItem(widget));
}

void tst_QGraphicsItem::task197802_childrenVisibility()
{
    QGraphicsScene scene;
    QGraphicsRectItem item(QRectF(0,0,20,20));

    QGraphicsRectItem *item2 = new QGraphicsRectItem(QRectF(0,0,10,10), &item);
    scene.addItem(&item);

    //freshly created: both visible
    QVERIFY(item.isVisible());
    QVERIFY(item2->isVisible());

    //hide child: parent visible, child not
    item2->hide();
    QVERIFY(item.isVisible());
    QVERIFY(!item2->isVisible());

    //hide parent: parent and child invisible
    item.hide();
    QVERIFY(!item.isVisible());
    QVERIFY(!item2->isVisible());

    //ask to show the child: parent and child invisible anyways
    item2->show();
    QVERIFY(!item.isVisible());
    QVERIFY(!item2->isVisible());

    //show the parent: both parent and child visible
    item.show();
    QVERIFY(item.isVisible());
    QVERIFY(item2->isVisible());

    delete item2;
}

void tst_QGraphicsItem::boundingRegion_data()
{
    QTest::addColumn<QLineF>("line");
    QTest::addColumn<qreal>("granularity");
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<QRegion>("expectedRegion");

    QTest::newRow("(0, 0, 10, 10) | 0.0 | identity | {(0, 0, 10, 10)}") << QLineF(0, 0, 10, 10) << qreal(0.0) << QTransform()
                                                                        << QRegion(QRect(0, 0, 10, 10));
    QTest::newRow("(0, 0, 10, 0) | 0.0 | identity | {(0, 0, 10, 10)}") << QLineF(0, 0, 10, 0) << qreal(0.0) << QTransform()
                                                                       << QRegion(QRect(0, 0, 10, 1));
    QTest::newRow("(0, 0, 10, 0) | 0.5 | identity | {(0, 0, 10, 1)}") << QLineF(0, 0, 10, 0) << qreal(0.5) << QTransform()
                                                                      << QRegion(QRect(0, 0, 10, 1));
    QTest::newRow("(0, 0, 10, 0) | 1.0 | identity | {(0, 0, 10, 1)}") << QLineF(0, 0, 10, 0) << qreal(1.0) << QTransform()
                                                                      << QRegion(QRect(0, 0, 10, 1));
    QTest::newRow("(0, 0, 0, 10) | 0.0 | identity | {(0, 0, 10, 10)}") << QLineF(0, 0, 0, 10) << qreal(0.0) << QTransform()
                                                                       << QRegion(QRect(0, 0, 1, 10));
    QTest::newRow("(0, 0, 0, 10) | 0.5 | identity | {(0, 0, 1, 10)}") << QLineF(0, 0, 0, 10) << qreal(0.5) << QTransform()
                                                                      << QRegion(QRect(0, 0, 1, 10));
    QTest::newRow("(0, 0, 0, 10) | 1.0 | identity | {(0, 0, 1, 10)}") << QLineF(0, 0, 0, 10) << qreal(1.0) << QTransform()
                                                                      << QRegion(QRect(0, 0, 1, 10));
}

void tst_QGraphicsItem::boundingRegion()
{
    QFETCH(QLineF, line);
    QFETCH(qreal, granularity);
    QFETCH(QTransform, transform);
    QFETCH(QRegion, expectedRegion);

    QGraphicsLineItem item(line);
    item.setPen(QPen(Qt::black, 0));
    QCOMPARE(item.boundingRegionGranularity(), qreal(0.0));
    item.setBoundingRegionGranularity(granularity);
    QCOMPARE(item.boundingRegionGranularity(), granularity);
    QCOMPARE(item.boundingRegion(transform), expectedRegion);
}

void tst_QGraphicsItem::itemTransform_parentChild()
{
    QGraphicsScene scene;
    QGraphicsItem *parent = scene.addRect(0, 0, 100, 100);
    QGraphicsItem *child = scene.addRect(0, 0, 100, 100);
    child->setParentItem(parent);
    child->setPos(10, 10);
    child->scale(2, 2);
    child->rotate(90);

    QCOMPARE(child->itemTransform(parent).map(QPointF(10, 10)), QPointF(-10, 30));
    QCOMPARE(parent->itemTransform(child).map(QPointF(-10, 30)), QPointF(10, 10));
}

void tst_QGraphicsItem::itemTransform_siblings()
{
    QGraphicsScene scene;
    QGraphicsItem *parent = scene.addRect(0, 0, 100, 100);
    QGraphicsItem *brother = scene.addRect(0, 0, 100, 100);
    QGraphicsItem *sister = scene.addRect(0, 0, 100, 100);
    parent->scale(10, 5);
    parent->rotate(-180);
    parent->shear(2, 3);

    brother->setParentItem(parent);
    sister->setParentItem(parent);

    brother->setPos(10, 10);
    brother->scale(2, 2);
    brother->rotate(90);
    sister->setPos(10, 10);
    sister->scale(2, 2);
    sister->rotate(90);

    QCOMPARE(brother->itemTransform(sister).map(QPointF(10, 10)), QPointF(10, 10));
    QCOMPARE(sister->itemTransform(brother).map(QPointF(10, 10)), QPointF(10, 10));
}

void tst_QGraphicsItem::itemTransform_unrelated()
{
    QGraphicsScene scene;
    QGraphicsItem *stranger1 = scene.addRect(0, 0, 100, 100);
    QGraphicsItem *stranger2 = scene.addRect(0, 0, 100, 100);
    stranger1->setPos(10, 10);
    stranger1->scale(2, 2);
    stranger1->rotate(90);
    stranger2->setPos(10, 10);
    stranger2->scale(2, 2);
    stranger2->rotate(90);

    QCOMPARE(stranger1->itemTransform(stranger2).map(QPointF(10, 10)), QPointF(10, 10));
    QCOMPARE(stranger2->itemTransform(stranger1).map(QPointF(10, 10)), QPointF(10, 10));
}

void tst_QGraphicsItem::opacity_data()
{
    QTest::addColumn<qreal>("p_opacity");
    QTest::addColumn<int>("p_opacityFlags");
    QTest::addColumn<qreal>("c1_opacity");
    QTest::addColumn<int>("c1_opacityFlags");
    QTest::addColumn<qreal>("c2_opacity");
    QTest::addColumn<int>("c2_opacityFlags");
    QTest::addColumn<qreal>("p_effectiveOpacity");
    QTest::addColumn<qreal>("c1_effectiveOpacity");
    QTest::addColumn<qreal>("c2_effectiveOpacity");
    QTest::addColumn<qreal>("c3_effectiveOpacity");

    // Modify the opacity and see how it propagates
    QTest::newRow("A: 1.0 0 1.0 0 1.0 1.0 1.0 1.0 1.0") << qreal(1.0) << 0 << qreal(1.0) << 0 << qreal(1.0) << 0
                                                        << qreal(1.0) << qreal(1.0) << qreal(1.0) << qreal(1.0);
    QTest::newRow("B: 0.5 0 1.0 0 1.0 1.0 1.0 1.0 1.0") << qreal(0.5) << 0 << qreal(1.0) << 0 << qreal(1.0) << 0
                                                        << qreal(0.5) << qreal(0.5) << qreal(0.5) << qreal(0.5);
    QTest::newRow("C: 0.5 0 0.1 0 1.0 1.0 1.0 1.0 1.0") << qreal(0.5) << 0 << qreal(0.1) << 0 << qreal(1.0) << 0
                                                        << qreal(0.5) << qreal(0.05) << qreal(0.05) << qreal(0.05);
    QTest::newRow("D: 0.0 0 1.0 0 1.0 1.0 1.0 1.0 1.0") << qreal(0.0) << 0 << qreal(1.0) << 0 << qreal(1.0) << 0
                                                        << qreal(0.0) << qreal(0.0) << qreal(0.0) << qreal(0.0);

    // Parent doesn't propagate to children - now modify the opacity and see how it propagates
    int flags = QGraphicsItem::ItemDoesntPropagateOpacityToChildren;
    QTest::newRow("E: 1.0 2 1.0 0 1.0 1.0 1.0 1.0 1.0") << qreal(1.0) << flags << qreal(1.0) << 0 << qreal(1.0) << 0
                                                        << qreal(1.0) << qreal(1.0) << qreal(1.0) << qreal(1.0);
    QTest::newRow("F: 0.5 2 1.0 0 1.0 1.0 1.0 1.0 1.0") << qreal(0.5) << flags << qreal(1.0) << 0 << qreal(1.0) << 0
                                                        << qreal(0.5) << qreal(1.0) << qreal(1.0) << qreal(1.0);
    QTest::newRow("G: 0.5 2 0.1 0 1.0 1.0 1.0 1.0 1.0") << qreal(0.5) << flags << qreal(0.1) << 0 << qreal(1.0) << 0
                                                        << qreal(0.5) << qreal(0.1) << qreal(0.1) << qreal(0.1);
    QTest::newRow("H: 0.0 2 1.0 0 1.0 1.0 1.0 1.0 1.0") << qreal(0.0) << flags << qreal(1.0) << 0 << qreal(1.0) << 0
                                                        << qreal(0.0) << qreal(1.0) << qreal(1.0) << qreal(1.0);

    // Child ignores parent - now modify the opacity and see how it propagates
    flags = QGraphicsItem::ItemIgnoresParentOpacity;
    QTest::newRow("I: 1.0 0 1.0 1 1.0 1.0 1.0 1.0 1.0") << qreal(1.0) << 0 << qreal(1.0) << flags << qreal(1.0) << 0
                                                        << qreal(1.0) << qreal(1.0) << qreal(1.0) << qreal(1.0);
    QTest::newRow("J: 1.0 0 1.0 1 1.0 1.0 1.0 1.0 1.0") << qreal(0.5) << 0 << qreal(0.5) << flags << qreal(0.5) << 0
                                                        << qreal(0.5) << qreal(0.5) << qreal(0.25) << qreal(0.25);
    QTest::newRow("K: 1.0 0 1.0 1 1.0 1.0 1.0 1.0 1.0") << qreal(0.2) << 0 << qreal(0.2) << flags << qreal(0.2) << 0
                                                        << qreal(0.2) << qreal(0.2) << qreal(0.04) << qreal(0.04);
    QTest::newRow("L: 1.0 0 1.0 1 1.0 1.0 1.0 1.0 1.0") << qreal(0.0) << 0 << qreal(0.0) << flags << qreal(0.0) << 0
                                                        << qreal(0.0) << qreal(0.0) << qreal(0.0) << qreal(0.0);

    // Child ignores parent and doesn't propagate - now modify the opacity and see how it propagates
    flags = QGraphicsItem::ItemIgnoresParentOpacity | QGraphicsItem::ItemDoesntPropagateOpacityToChildren;
    QTest::newRow("M: 1.0 0 1.0 1 1.0 1.0 1.0 1.0 1.0") << qreal(1.0) << 0 // p
                                                        << qreal(1.0) << flags // c1 (no prop)
                                                        << qreal(1.0) << 0 // c2
                                                        << qreal(1.0) << qreal(1.0) << qreal(1.0) << qreal(1.0);
    QTest::newRow("M: 0.5 0 1.0 1 1.0 1.0 1.0 1.0 1.0") << qreal(0.5) << 0 // p
                                                        << qreal(1.0) << flags // c1 (no prop)
                                                        << qreal(1.0) << 0 // c2
                                                        << qreal(0.5) << qreal(1.0) << qreal(1.0) << qreal(1.0);
    QTest::newRow("M: 0.5 0 0.5 1 1.0 1.0 1.0 1.0 1.0") << qreal(0.5) << 0 // p
                                                        << qreal(0.5) << flags // c1 (no prop)
                                                        << qreal(1.0) << 0 // c2
                                                        << qreal(0.5) << qreal(0.5) << qreal(1.0) << qreal(1.0);
    QTest::newRow("M: 0.5 0 0.5 1 0.5 1.0 1.0 1.0 1.0") << qreal(0.5) << 0 // p
                                                        << qreal(0.5) << flags // c1 (no prop)
                                                        << qreal(0.5) << 0 // c2
                                                        << qreal(0.5) << qreal(0.5) << qreal(0.5) << qreal(0.5);
    QTest::newRow("M: 1.0 0 0.5 1 0.5 1.0 1.0 1.0 1.0") << qreal(1.0) << 0 // p
                                                        << qreal(0.5) << flags // c1 (no prop)
                                                        << qreal(0.5) << 0 // c2
                                                        << qreal(1.0) << qreal(0.5) << qreal(0.5) << qreal(0.5);
}

void tst_QGraphicsItem::opacity()
{
    QFETCH(qreal, p_opacity);
    QFETCH(int, p_opacityFlags);
    QFETCH(qreal, p_effectiveOpacity);
    QFETCH(qreal, c1_opacity);
    QFETCH(int, c1_opacityFlags);
    QFETCH(qreal, c1_effectiveOpacity);
    QFETCH(qreal, c2_opacity);
    QFETCH(int, c2_opacityFlags);
    QFETCH(qreal, c2_effectiveOpacity);
    QFETCH(qreal, c3_effectiveOpacity);

    QGraphicsRectItem *p = new QGraphicsRectItem;
    QGraphicsRectItem *c1 = new QGraphicsRectItem(p);
    QGraphicsRectItem *c2 = new QGraphicsRectItem(c1);
    QGraphicsRectItem *c3 = new QGraphicsRectItem(c2);

    QCOMPARE(p->opacity(), qreal(1.0));
    QCOMPARE(p->effectiveOpacity(), qreal(1.0));
    int opacityMask = QGraphicsItem::ItemIgnoresParentOpacity | QGraphicsItem::ItemDoesntPropagateOpacityToChildren;
    QVERIFY(!(p->flags() & opacityMask));

    p->setOpacity(p_opacity);
    c1->setOpacity(c1_opacity);
    c2->setOpacity(c2_opacity);
    p->setFlags(QGraphicsItem::GraphicsItemFlags(p->flags() | p_opacityFlags));
    c1->setFlags(QGraphicsItem::GraphicsItemFlags(c1->flags() | c1_opacityFlags));
    c2->setFlags(QGraphicsItem::GraphicsItemFlags(c2->flags() | c2_opacityFlags));

    QCOMPARE(int(p->flags() & opacityMask), p_opacityFlags);
    QCOMPARE(int(c1->flags() & opacityMask), c1_opacityFlags);
    QCOMPARE(int(c2->flags() & opacityMask), c2_opacityFlags);
    QCOMPARE(p->opacity(), p_opacity);
    QCOMPARE(p->effectiveOpacity(), p_effectiveOpacity);
    QCOMPARE(c1->effectiveOpacity(), c1_effectiveOpacity);
    QCOMPARE(c2->effectiveOpacity(), c2_effectiveOpacity);
    QCOMPARE(c3->effectiveOpacity(), c3_effectiveOpacity);
}

void tst_QGraphicsItem::opacity2()
{
    EventTester *parent = new EventTester;
    EventTester *child = new EventTester(parent);
    EventTester *grandChild = new EventTester(child);

    QGraphicsScene scene;
    scene.addItem(parent);

    MyGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.repaints >= 1);

#define RESET_REPAINT_COUNTERS \
    parent->repaints = 0; \
    child->repaints = 0; \
    grandChild->repaints = 0; \
    view.repaints = 0;

    RESET_REPAINT_COUNTERS

    child->setOpacity(0.0);
    QTest::qWait(10);
    QTRY_COMPARE(view.repaints, 1);
    QCOMPARE(parent->repaints, 1);
    QCOMPARE(child->repaints, 0);
    QCOMPARE(grandChild->repaints, 0);

    RESET_REPAINT_COUNTERS

    child->setOpacity(1.0);
    QTest::qWait(10);
    QTRY_COMPARE(view.repaints, 1);
    QCOMPARE(parent->repaints, 1);
    QCOMPARE(child->repaints, 1);
    QCOMPARE(grandChild->repaints, 1);

    RESET_REPAINT_COUNTERS

    parent->setOpacity(0.0);
    QTest::qWait(10);
    QTRY_COMPARE(view.repaints, 1);
    QCOMPARE(parent->repaints, 0);
    QCOMPARE(child->repaints, 0);
    QCOMPARE(grandChild->repaints, 0);

    RESET_REPAINT_COUNTERS

    parent->setOpacity(1.0);
    QTest::qWait(10);
    QTRY_COMPARE(view.repaints, 1);
    QCOMPARE(parent->repaints, 1);
    QCOMPARE(child->repaints, 1);
    QCOMPARE(grandChild->repaints, 1);

    grandChild->setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
    RESET_REPAINT_COUNTERS

    child->setOpacity(0.0);
    QTest::qWait(10);
    QTRY_COMPARE(view.repaints, 1);
    QCOMPARE(parent->repaints, 1);
    QCOMPARE(child->repaints, 0);
    QCOMPARE(grandChild->repaints, 1);

    RESET_REPAINT_COUNTERS

    child->setOpacity(0.0); // Already 0.0; no change.
    QTest::qWait(10);
    QTRY_COMPARE(view.repaints, 0);
    QCOMPARE(parent->repaints, 0);
    QCOMPARE(child->repaints, 0);
    QCOMPARE(grandChild->repaints, 0);
}

void tst_QGraphicsItem::opacityZeroUpdates()
{
    EventTester *parent = new EventTester;
    EventTester *child = new EventTester(parent);

    child->setPos(10, 10);

    QGraphicsScene scene;
    scene.addItem(parent);

    MyGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.repaints > 0);

    view.reset();
    parent->setOpacity(0.0);

    QTest::qWait(20);

    // transforming items bounding rect to view coordinates
    const QRect childDeviceBoundingRect = child->deviceTransform(view.viewportTransform())
                                           .mapRect(child->boundingRect()).toRect();
    const QRect parentDeviceBoundingRect = parent->deviceTransform(view.viewportTransform())
                                           .mapRect(parent->boundingRect()).toRect();

    QRegion expectedRegion = parentDeviceBoundingRect.adjusted(-2, -2, 2, 2);
    expectedRegion += childDeviceBoundingRect.adjusted(-2, -2, 2, 2);

    COMPARE_REGIONS(view.paintedRegion, expectedRegion);
}

class StacksBehindParentHelper : public QGraphicsRectItem
{
public:
    StacksBehindParentHelper(QList<QGraphicsItem *> *paintedItems, const QRectF &rect, QGraphicsItem *parent = 0)
        : QGraphicsRectItem(rect, parent), paintedItems(paintedItems)
    { }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QGraphicsRectItem::paint(painter, option, widget);
        paintedItems->append(this);
    }

private:
    QList<QGraphicsItem *> *paintedItems;
};

void tst_QGraphicsItem::itemStacksBehindParent()
{
    StacksBehindParentHelper *parent1 = new StacksBehindParentHelper(&paintedItems, QRectF(0, 0, 100, 50));
    StacksBehindParentHelper *child11 = new StacksBehindParentHelper(&paintedItems, QRectF(-10, 10, 50, 50), parent1);
    StacksBehindParentHelper *grandChild111 = new StacksBehindParentHelper(&paintedItems, QRectF(-20, 20, 50, 50), child11);
    StacksBehindParentHelper *child12 = new StacksBehindParentHelper(&paintedItems, QRectF(60, 10, 50, 50), parent1);
    StacksBehindParentHelper *grandChild121 = new StacksBehindParentHelper(&paintedItems, QRectF(70, 20, 50, 50), child12);

    StacksBehindParentHelper *parent2 = new StacksBehindParentHelper(&paintedItems, QRectF(0, 0, 100, 50));
    StacksBehindParentHelper *child21 = new StacksBehindParentHelper(&paintedItems, QRectF(-10, 10, 50, 50), parent2);
    StacksBehindParentHelper *grandChild211 = new StacksBehindParentHelper(&paintedItems, QRectF(-20, 20, 50, 50), child21);
    StacksBehindParentHelper *child22 = new StacksBehindParentHelper(&paintedItems, QRectF(60, 10, 50, 50), parent2);
    StacksBehindParentHelper *grandChild221 = new StacksBehindParentHelper(&paintedItems, QRectF(70, 20, 50, 50), child22);

    parent1->setData(0, "parent1");
    child11->setData(0, "child11");
    grandChild111->setData(0, "grandChild111");
    child12->setData(0, "child12");
    grandChild121->setData(0, "grandChild121");
    parent2->setData(0, "parent2");
    child21->setData(0, "child21");
    grandChild211->setData(0, "grandChild211");
    child22->setData(0, "child22");
    grandChild221->setData(0, "grandChild221");

    // Disambiguate siblings
    parent1->setZValue(1);
    child11->setZValue(1);
    child21->setZValue(1);

    QGraphicsScene scene;
    scene.addItem(parent1);
    scene.addItem(parent2);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(!paintedItems.isEmpty());
    QTest::qWait(100);
    paintedItems.clear();
    view.viewport()->update();
    QApplication::processEvents();
    QTRY_COMPARE(scene.items(0, 0, 100, 100), (QList<QGraphicsItem *>()
                                           << grandChild111 << child11
                                           << grandChild121 << child12 << parent1
                                           << grandChild211 << child21
                                           << grandChild221 << child22 << parent2));
    QTRY_COMPARE(paintedItems, QList<QGraphicsItem *>()
             << parent2 << child22 << grandChild221
             << child21 << grandChild211
             << parent1 << child12 << grandChild121
             << child11 << grandChild111);

    child11->setFlag(QGraphicsItem::ItemStacksBehindParent);
    scene.update();
    paintedItems.clear();
    QApplication::processEvents();

    QTRY_COMPARE(scene.items(0, 0, 100, 100), (QList<QGraphicsItem *>()
                                           << grandChild121 << child12 << parent1
                                           << grandChild111 << child11
                                           << grandChild211 << child21
                                           << grandChild221 << child22 << parent2));
    QCOMPARE(paintedItems, QList<QGraphicsItem *>()
             << parent2 << child22 << grandChild221
             << child21 << grandChild211
             << child11 << grandChild111
             << parent1 << child12 << grandChild121);

    child12->setFlag(QGraphicsItem::ItemStacksBehindParent);
    paintedItems.clear();
    scene.update();
    QApplication::processEvents();

    QTRY_COMPARE(scene.items(0, 0, 100, 100), (QList<QGraphicsItem *>()
                                           << parent1 << grandChild111 << child11
                                           << grandChild121 << child12
                                           << grandChild211 << child21
                                           << grandChild221 << child22 << parent2));
    QCOMPARE(paintedItems, QList<QGraphicsItem *>()
             << parent2 << child22 << grandChild221
             << child21 << grandChild211
             << child12 << grandChild121
             << child11 << grandChild111 << parent1);
}

class ClippingAndTransformsScene : public QGraphicsScene
{
public:
    QList<QGraphicsItem *> drawnItems;
protected:
    void drawItems(QPainter *painter, int numItems, QGraphicsItem *items[],
                   const QStyleOptionGraphicsItem options[], QWidget *widget = 0)
    {
        drawnItems.clear();
        for (int i = 0; i < numItems; ++i)
            drawnItems << items[i];
        QGraphicsScene::drawItems(painter, numItems, items, options, widget);
    }
};

void tst_QGraphicsItem::nestedClipping()
{
    ClippingAndTransformsScene scene;
    scene.setSceneRect(-50, -50, 200, 200);

    QGraphicsRectItem *root = new QGraphicsRectItem(QRectF(0, 0, 100, 100));
    root->setBrush(QColor(0, 0, 255));
    root->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    QGraphicsRectItem *l1 = new QGraphicsRectItem(QRectF(0, 0, 100, 100));
    l1->setParentItem(root);
    l1->setPos(-50, 0);
    l1->setBrush(QColor(255, 0, 0));
    l1->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    QGraphicsEllipseItem *l2 = new QGraphicsEllipseItem(QRectF(0, 0, 100, 100));
    l2->setParentItem(l1);
    l2->setPos(50, 50);
    l2->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    l2->setBrush(QColor(255, 255, 0));
    QGraphicsRectItem *l3 = new QGraphicsRectItem(QRectF(0, 0, 25, 25));
    l3->setParentItem(l2);
    l3->setBrush(QColor(0, 255, 0));
    l3->setPos(50 - 12, -12);

    scene.addItem(root);

    root->setData(0, "root");
    l1->setData(0, "l1");
    l2->setData(0, "l2");
    l3->setData(0, "l3");

    QGraphicsView view(&scene);
    view.setOptimizationFlag(QGraphicsView::IndirectPainting);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(25);

    QList<QGraphicsItem *> expected;
    expected << root << l1 << l2 << l3;
    QTRY_COMPARE(scene.drawnItems, expected);

    QImage image(200, 200, QImage::Format_ARGB32_Premultiplied);
    image.fill(0);

    QPainter painter(&image);
    scene.render(&painter);
    painter.end();

    // Check transparent areas
    QCOMPARE(image.pixel(100, 25), qRgba(0, 0, 0, 0));
    QCOMPARE(image.pixel(100, 175), qRgba(0, 0, 0, 0));
    QCOMPARE(image.pixel(25, 100), qRgba(0, 0, 0, 0));
    QCOMPARE(image.pixel(175, 100), qRgba(0, 0, 0, 0));
    QCOMPARE(image.pixel(70, 80), qRgba(255, 0, 0, 255));
    QCOMPARE(image.pixel(80, 130), qRgba(255, 255, 0, 255));
    QCOMPARE(image.pixel(92, 105), qRgba(0, 255, 0, 255));
    QCOMPARE(image.pixel(105, 105), qRgba(0, 0, 255, 255));
#if 0
    // Enable this to compare if the test starts failing.
    image.save("nestedClipping_reference.png");
#endif
}

class TransformDebugItem : public QGraphicsRectItem
{
public:
    TransformDebugItem()
        : QGraphicsRectItem(QRectF(-10, -10, 20, 20))
    {
        setPen(QPen(Qt::black, 0));
        setBrush(QColor(qrand() % 256, qrand() % 256, qrand() % 256));
    }

    QTransform x;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0)
    {
        x = painter->worldTransform();
        QGraphicsRectItem::paint(painter, option, widget);
    }
};

void tst_QGraphicsItem::nestedClippingTransforms()
{
    TransformDebugItem *rootClipper = new TransformDebugItem;
    rootClipper->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    TransformDebugItem *child = new TransformDebugItem;
    child->setParentItem(rootClipper);
    child->setPos(2, 2);
    TransformDebugItem *grandChildClipper = new TransformDebugItem;
    grandChildClipper->setParentItem(child);
    grandChildClipper->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    grandChildClipper->setPos(4, 4);
    TransformDebugItem *greatGrandChild = new TransformDebugItem;
    greatGrandChild->setPos(2, 2);
    greatGrandChild->setParentItem(grandChildClipper);
    TransformDebugItem *grandChildClipper2 = new TransformDebugItem;
    grandChildClipper2->setParentItem(child);
    grandChildClipper2->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    grandChildClipper2->setPos(8, 8);
    TransformDebugItem *greatGrandChild2 = new TransformDebugItem;
    greatGrandChild2->setPos(2, 2);
    greatGrandChild2->setParentItem(grandChildClipper2);
    TransformDebugItem *grandChildClipper3 = new TransformDebugItem;
    grandChildClipper3->setParentItem(child);
    grandChildClipper3->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    grandChildClipper3->setPos(12, 12);
    TransformDebugItem *greatGrandChild3 = new TransformDebugItem;
    greatGrandChild3->setPos(2, 2);
    greatGrandChild3->setParentItem(grandChildClipper3);

    QGraphicsScene scene;
    scene.addItem(rootClipper);

    QImage image(scene.itemsBoundingRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(0);
    QPainter p(&image);
    scene.render(&p);
    p.end();

    QCOMPARE(rootClipper->x, QTransform(1, 0, 0, 0, 1, 0, 10, 10, 1));
    QCOMPARE(child->x, QTransform(1, 0, 0, 0, 1, 0, 12, 12, 1));
    QCOMPARE(grandChildClipper->x, QTransform(1, 0, 0, 0, 1, 0, 16, 16, 1));
    QCOMPARE(greatGrandChild->x, QTransform(1, 0, 0, 0, 1, 0, 18, 18, 1));
    QCOMPARE(grandChildClipper2->x, QTransform(1, 0, 0, 0, 1, 0, 20, 20, 1));
    QCOMPARE(greatGrandChild2->x, QTransform(1, 0, 0, 0, 1, 0, 22, 22, 1));
    QCOMPARE(grandChildClipper3->x, QTransform(1, 0, 0, 0, 1, 0, 24, 24, 1));
    QCOMPARE(greatGrandChild3->x, QTransform(1, 0, 0, 0, 1, 0, 26, 26, 1));
}

void tst_QGraphicsItem::sceneTransformCache()
{
    // Test that an item's scene transform is updated correctly when the
    // parent is transformed.
    QGraphicsScene scene;
    QGraphicsRectItem *rect = scene.addRect(0, 0, 100, 100);
    rect->setPen(QPen(Qt::black, 0));
    QGraphicsRectItem *rect2 = scene.addRect(0, 0, 100, 100);
    rect2->setPen(QPen(Qt::black, 0));
    rect2->setParentItem(rect);
    rect2->rotate(90);
    rect->translate(0, 50);
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    rect->translate(0, 100);
    QTransform x;
    x.translate(0, 150);
    x.rotate(90);
    QCOMPARE(rect2->sceneTransform(), x);

    scene.removeItem(rect);

    //Crazy use case : rect4 child of rect3 so the transformation of rect4 will be cached.Good!
    //We remove rect4 from the scene, then the validTransform bit flag is set to 0 and the index of the cache
    //add to the freeTransformSlots. The problem was that sceneTransformIndex was not set to -1 so if a new item arrive
    //with a child (rect6) that will be cached then it will take the freeSlot (ex rect4) and put it his transform. But if rect4 is
    //added back to the scene then it will set the transform to his old sceneTransformIndex value that will erase the new
    //value of rect6 so rect6 transform will be wrong.
    QGraphicsRectItem *rect3 = scene.addRect(0, 0, 100, 100);
    QGraphicsRectItem *rect4 = scene.addRect(0, 0, 100, 100);
    rect3->setPos(QPointF(10,10));
    rect3->setPen(QPen(Qt::black, 0));

    rect4->setParentItem(rect3);
    rect4->setPos(QPointF(10,10));
    rect4->setPen(QPen(Qt::black, 0));

    QCOMPARE(rect4->mapToScene(rect4->boundingRect().topLeft()), QPointF(20,20));

    scene.removeItem(rect4);
    //rect4 transform is local only
    QCOMPARE(rect4->mapToScene(rect4->boundingRect().topLeft()), QPointF(10,10));

    QGraphicsRectItem *rect5 = scene.addRect(0, 0, 100, 100);
    QGraphicsRectItem *rect6 = scene.addRect(0, 0, 100, 100);
    rect5->setPos(QPointF(20,20));
    rect5->setPen(QPen(Qt::black, 0));

    rect6->setParentItem(rect5);
    rect6->setPos(QPointF(10,10));
    rect6->setPen(QPen(Qt::black, 0));
    //test if rect6 transform is ok
    QCOMPARE(rect6->mapToScene(rect6->boundingRect().topLeft()), QPointF(30,30));

    scene.addItem(rect4);

    QCOMPARE(rect4->mapToScene(rect4->boundingRect().topLeft()), QPointF(10,10));
    //test if rect6 transform is still correct
    QCOMPARE(rect6->mapToScene(rect6->boundingRect().topLeft()), QPointF(30,30));
}

void tst_QGraphicsItem::tabChangesFocus_data()
{
    QTest::addColumn<bool>("tabChangesFocus");
    QTest::newRow("tab changes focus") << true;
    QTest::newRow("tab doesn't change focus") << false;
}

void tst_QGraphicsItem::tabChangesFocus()
{
    QFETCH(bool, tabChangesFocus);

    QGraphicsScene scene;
    QGraphicsTextItem *item = scene.addText("Hello");
    item->setTabChangesFocus(tabChangesFocus);
    item->setTextInteractionFlags(Qt::TextEditorInteraction);
    item->setFocus();

    QDial *dial1 = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);

    QDial *dial2 = new QDial;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(dial1);
    layout->addWidget(view);
    layout->addWidget(dial2);

    QWidget widget;
    widget.setLayout(layout);
    widget.show();
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    QTRY_VERIFY(scene.isActive());

    dial1->setFocus();
    QTest::qWait(15);
    QTRY_VERIFY(dial1->hasFocus());

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QTest::qWait(15);
    QTRY_VERIFY(view->hasFocus());
    QTRY_VERIFY(item->hasFocus());

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QTest::qWait(15);

    if (tabChangesFocus) {
        QTRY_VERIFY(!view->hasFocus());
        QTRY_VERIFY(!item->hasFocus());
        QTRY_VERIFY(dial2->hasFocus());
    } else {
        QTRY_VERIFY(view->hasFocus());
        QTRY_VERIFY(item->hasFocus());
        QCOMPARE(item->toPlainText(), QString("\tHello"));
    }
}

void tst_QGraphicsItem::cacheMode()
{
    QGraphicsScene scene(0, 0, 100, 100);
    QGraphicsView view(&scene);
    view.resize(150, 150);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    // Increase the probability of window activation
    // not causing another repaint of test items.
    QTest::qWait(50);

    EventTester *tester = new EventTester;
    EventTester *testerChild = new EventTester;
    testerChild->setParentItem(tester);
    EventTester *testerChild2 = new EventTester;
    testerChild2->setParentItem(testerChild);
    testerChild2->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    scene.addItem(tester);
    QTest::qWait(10);

    for (int i = 0; i < 2; ++i) {
        // No visual change.
        QTRY_COMPARE(tester->repaints, 1);
        QCOMPARE(testerChild->repaints, 1);
        QCOMPARE(testerChild2->repaints, 1);
        tester->setCacheMode(QGraphicsItem::NoCache);
        testerChild->setCacheMode(QGraphicsItem::NoCache);
        testerChild2->setCacheMode(QGraphicsItem::NoCache);
        QTest::qWait(25);
        QTRY_COMPARE(tester->repaints, 1);
        QCOMPARE(testerChild->repaints, 1);
        QCOMPARE(testerChild2->repaints, 1);
        tester->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        testerChild->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        testerChild2->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        QTest::qWait(25);
    }

    // The first move causes a repaint as the item is painted into its pixmap.
    // (Only occurs if the item has previously been painted without cache).
    tester->setPos(10, 10);
    testerChild->setPos(10, 10);
    testerChild2->setPos(10, 10);
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 2);
    QCOMPARE(testerChild->repaints, 2);
    QCOMPARE(testerChild2->repaints, 2);

    // Consecutive moves should not repaint.
    tester->setPos(20, 20);
    testerChild->setPos(20, 20);
    testerChild2->setPos(20, 20);
    QTest::qWait(250);
    QCOMPARE(tester->repaints, 2);
    QCOMPARE(testerChild->repaints, 2);
    QCOMPARE(testerChild2->repaints, 2);

    // Translating does not result in a repaint.
    tester->translate(10, 10);
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 2);
    QCOMPARE(testerChild->repaints, 2);
    QCOMPARE(testerChild2->repaints, 2);

    // Rotating results in a repaint.
    tester->rotate(45);
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 3);
    QCOMPARE(testerChild->repaints, 3);
    QCOMPARE(testerChild2->repaints, 2);

    // Change to ItemCoordinateCache (triggers repaint).
    tester->setCacheMode(QGraphicsItem::ItemCoordinateCache); // autosize
    testerChild->setCacheMode(QGraphicsItem::ItemCoordinateCache); // autosize
    testerChild2->setCacheMode(QGraphicsItem::ItemCoordinateCache); // autosize
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 4);
    QCOMPARE(testerChild->repaints, 4);
    QCOMPARE(testerChild2->repaints, 3);

    // Rotating items with ItemCoordinateCache doesn't cause a repaint.
    tester->rotate(22);
    testerChild->rotate(22);
    testerChild2->rotate(22);
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 4);
    QTRY_COMPARE(testerChild->repaints, 4);
    QTRY_COMPARE(testerChild2->repaints, 3);
    tester->resetTransform();
    testerChild->resetTransform();
    testerChild2->resetTransform();

    // Explicit update causes a repaint.
    tester->update(0, 0, 5, 5);
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 5);
    QCOMPARE(testerChild->repaints, 4);
    QCOMPARE(testerChild2->repaints, 3);

    // Updating outside the item's bounds does not cause a repaint.
    tester->update(10, 10, 5, 5);
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 5);
    QCOMPARE(testerChild->repaints, 4);
    QCOMPARE(testerChild2->repaints, 3);

    // Resizing an item should cause a repaint of that item. (because of
    // autosize).
    tester->setGeometry(QRectF(-15, -15, 30, 30));
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 6);
    QCOMPARE(testerChild->repaints, 4);
    QCOMPARE(testerChild2->repaints, 3);

    // Set fixed size.
    tester->setCacheMode(QGraphicsItem::ItemCoordinateCache, QSize(30, 30));
    testerChild->setCacheMode(QGraphicsItem::ItemCoordinateCache, QSize(30, 30));
    testerChild2->setCacheMode(QGraphicsItem::ItemCoordinateCache, QSize(30, 30));
    QTest::qWait(20);
    QTRY_COMPARE(tester->repaints, 7);
    QCOMPARE(testerChild->repaints, 5);
    QCOMPARE(testerChild2->repaints, 4);

    // Resizing the item should cause a repaint.
    testerChild->setGeometry(QRectF(-15, -15, 30, 30));
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 7);
    QCOMPARE(testerChild->repaints, 6);
    QCOMPARE(testerChild2->repaints, 4);

    // Scaling the view does not cause a repaint.
    view.scale(0.7, 0.7);
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 7);
    QCOMPARE(testerChild->repaints, 6);
    QCOMPARE(testerChild2->repaints, 4);

    // Switch to device coordinate cache.
    tester->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    testerChild->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    testerChild2->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 8);
    QCOMPARE(testerChild->repaints, 7);
    QCOMPARE(testerChild2->repaints, 5);

    // Scaling the view back should cause repaints for two of the items.
    view.setTransform(QTransform());
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 9);
    QCOMPARE(testerChild->repaints, 8);
    QCOMPARE(testerChild2->repaints, 5);

    // Rotating the base item (perspective) should repaint two items.
    tester->setTransform(QTransform().rotate(10, Qt::XAxis));
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 10);
    QCOMPARE(testerChild->repaints, 9);
    QCOMPARE(testerChild2->repaints, 5);

    // Moving the middle item should case a repaint even if it's a move,
    // because the parent is rotated with a perspective.
    testerChild->setPos(1, 1);
    QTest::qWait(25);
    QTRY_COMPARE(tester->repaints, 11);
    QTRY_COMPARE(testerChild->repaints, 10);
    QTRY_COMPARE(testerChild2->repaints, 5);
    tester->resetTransform();

    // Make a huge item
    tester->setGeometry(QRectF(-4000, -4000, 8000, 8000));
    QTRY_COMPARE(tester->repaints, 12);
    QTRY_COMPARE(testerChild->repaints, 11);
    QTRY_COMPARE(testerChild2->repaints, 5);

    // Move the large item - will cause a repaint as the
    // cache is clipped.
    tester->setPos(5, 0);
    QTRY_COMPARE(tester->repaints, 13);
    QTRY_COMPARE(testerChild->repaints, 11);
    QTRY_COMPARE(testerChild2->repaints, 5);

    // Hiding and showing should invalidate the cache
    tester->hide();
    QTest::qWait(25);
    tester->show();
    QTRY_COMPARE(tester->repaints, 14);
    QTRY_COMPARE(testerChild->repaints, 12);
    QTRY_COMPARE(testerChild2->repaints, 6);
}

void tst_QGraphicsItem::cacheMode2()
{
    QGraphicsScene scene(0, 0, 100, 100);
    QGraphicsView view(&scene);
    view.resize(150, 150);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    // Increase the probability of window activation
    // not causing another repaint of test items.
    QTest::qWait(50);

    EventTester *tester = new EventTester;
    scene.addItem(tester);
    QTest::qWait(10);
    QTRY_COMPARE(tester->repaints, 1);

    // Switching from NoCache to NoCache (no repaint)
    tester->setCacheMode(QGraphicsItem::NoCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 1);

    // Switching from NoCache to DeviceCoordinateCache (no repaint)
    tester->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 1);

    // Switching from DeviceCoordinateCache to DeviceCoordinateCache (no repaint)
    tester->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 1);

    // Switching from DeviceCoordinateCache to NoCache (no repaint)
    tester->setCacheMode(QGraphicsItem::NoCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 1);

    // Switching from NoCache to ItemCoordinateCache (repaint)
    tester->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 2);

    // Switching from ItemCoordinateCache to ItemCoordinateCache (no repaint)
    tester->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 2);

    // Switching from ItemCoordinateCache to ItemCoordinateCache with different size (repaint)
    tester->setCacheMode(QGraphicsItem::ItemCoordinateCache, QSize(100, 100));
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 3);

    // Switching from ItemCoordinateCache to NoCache (repaint)
    tester->setCacheMode(QGraphicsItem::NoCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 4);

    // Switching from DeviceCoordinateCache to ItemCoordinateCache (repaint)
    tester->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 4);
    tester->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 5);

    // Switching from ItemCoordinateCache to DeviceCoordinateCache (repaint)
    tester->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    QTest::qWait(50);
    QTRY_COMPARE(tester->repaints, 6);
}

void tst_QGraphicsItem::updateCachedItemAfterMove()
{
    // A simple item that uses ItemCoordinateCache
    EventTester *tester = new EventTester;
    tester->setCacheMode(QGraphicsItem::ItemCoordinateCache);

    // Add to a scene, show in a view, ensure it's painted and reset its
    // repaint counter.
    QGraphicsScene scene;
    scene.addItem(tester);
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QTest::qWait(12);
    QTRY_VERIFY(tester->repaints > 0);
    tester->repaints = 0;

    // Move the item, should not cause repaints
    tester->setPos(10, 0);
    QTest::qWait(12);
    QCOMPARE(tester->repaints, 0);

    // Move then update, should cause one repaint
    tester->setPos(20, 0);
    tester->update();
    QTest::qWait(12);
    QCOMPARE(tester->repaints, 1);

    // Hiding the item doesn't cause a repaint
    tester->hide();
    QTest::qWait(12);
    QCOMPARE(tester->repaints, 1);

    // Moving a hidden item doesn't cause a repaint
    tester->setPos(30, 0);
    tester->update();
    QTest::qWait(12);
    QCOMPARE(tester->repaints, 1);
}

class Track : public QGraphicsRectItem
{
public:
    Track(const QRectF &rect)
        : QGraphicsRectItem(rect)
    {
        setAcceptHoverEvents(true);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0)
    {
        QGraphicsRectItem::paint(painter, option, widget);
        painter->drawText(boundingRect(), Qt::AlignCenter, QString("%1x%2\n%3x%4").arg(p.x()).arg(p.y()).arg(sp.x()).arg(sp.y()));
    }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event)
    {
        p = event->pos();
        sp = event->widget()->mapFromGlobal(event->screenPos());
        update();
    }
private:
    QPointF p;
    QPoint sp;
};

void tst_QGraphicsItem::deviceTransform_data()
{
    QTest::addColumn<bool>("untransformable1");
    QTest::addColumn<bool>("untransformable2");
    QTest::addColumn<bool>("untransformable3");
    QTest::addColumn<qreal>("rotation1");
    QTest::addColumn<qreal>("rotation2");
    QTest::addColumn<qreal>("rotation3");
    QTest::addColumn<QTransform>("deviceX");
    QTest::addColumn<QPointF>("mapResult1");
    QTest::addColumn<QPointF>("mapResult2");
    QTest::addColumn<QPointF>("mapResult3");

    QTest::newRow("nil") << false << false << false
                         << qreal(0.0) << qreal(0.0) << qreal(0.0)
                         << QTransform()
                         << QPointF(150, 150) << QPointF(250, 250) << QPointF(350, 350);
    QTest::newRow("deviceX rot 90") << false << false << false
                         << qreal(0.0) << qreal(0.0) << qreal(0.0)
                         << QTransform().rotate(90)
                         << QPointF(-150, 150) << QPointF(-250, 250) << QPointF(-350, 350);
    QTest::newRow("deviceX rot 90 100") << true << false << false
                         << qreal(0.0) << qreal(0.0) << qreal(0.0)
                         << QTransform().rotate(90)
                         << QPointF(-50, 150) << QPointF(50, 250) << QPointF(150, 350);
    QTest::newRow("deviceX rot 90 010") << false << true << false
                         << qreal(0.0) << qreal(0.0) << qreal(0.0)
                         << QTransform().rotate(90)
                         << QPointF(-150, 150) << QPointF(-150, 250) << QPointF(-50, 350);
    QTest::newRow("deviceX rot 90 001") << false << false << true
                         << qreal(0.0) << qreal(0.0) << qreal(0.0)
                         << QTransform().rotate(90)
                         << QPointF(-150, 150) << QPointF(-250, 250) << QPointF(-250, 350);
    QTest::newRow("deviceX rot 90 111") << true << true << true
                         << qreal(0.0) << qreal(0.0) << qreal(0.0)
                         << QTransform().rotate(90)
                         << QPointF(-50, 150) << QPointF(50, 250) << QPointF(150, 350);
    QTest::newRow("deviceX rot 90 101") << true << false << true
                         << qreal(0.0) << qreal(0.0) << qreal(0.0)
                         << QTransform().rotate(90)
                         << QPointF(-50, 150) << QPointF(50, 250) << QPointF(150, 350);
}

void tst_QGraphicsItem::deviceTransform()
{
    QFETCH(bool, untransformable1);
    QFETCH(bool, untransformable2);
    QFETCH(bool, untransformable3);
    QFETCH(qreal, rotation1);
    QFETCH(qreal, rotation2);
    QFETCH(qreal, rotation3);
    QFETCH(QTransform, deviceX);
    QFETCH(QPointF, mapResult1);
    QFETCH(QPointF, mapResult2);
    QFETCH(QPointF, mapResult3);

    QGraphicsScene scene;
    Track *rect1 = new Track(QRectF(0, 0, 100, 100));
    Track *rect2 = new Track(QRectF(0, 0, 100, 100));
    Track *rect3 = new Track(QRectF(0, 0, 100, 100));
    rect2->setParentItem(rect1);
    rect3->setParentItem(rect2);
    rect1->setPos(100, 100);
    rect2->setPos(100, 100);
    rect3->setPos(100, 100);
    rect1->rotate(rotation1);
    rect2->rotate(rotation2);
    rect3->rotate(rotation3);
    rect1->setFlag(QGraphicsItem::ItemIgnoresTransformations, untransformable1);
    rect2->setFlag(QGraphicsItem::ItemIgnoresTransformations, untransformable2);
    rect3->setFlag(QGraphicsItem::ItemIgnoresTransformations, untransformable3);
    rect1->setBrush(Qt::red);
    rect2->setBrush(Qt::green);
    rect3->setBrush(Qt::blue);
    scene.addItem(rect1);

    QCOMPARE(rect1->deviceTransform(deviceX).map(QPointF(50, 50)), mapResult1);
    QCOMPARE(rect2->deviceTransform(deviceX).map(QPointF(50, 50)), mapResult2);
    QCOMPARE(rect3->deviceTransform(deviceX).map(QPointF(50, 50)), mapResult3);
}

void tst_QGraphicsItem::update()
{
    QGraphicsScene scene;
    scene.setSceneRect(-100, -100, 200, 200);
    QWidget topLevel;
    MyGraphicsView view(&scene,&topLevel);

    topLevel.resize(300, 300);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    EventTester *item = new EventTester;
    scene.addItem(item);
    QTest::qWait(100); // Make sure all pending updates are processed.
    item->repaints = 0;

    item->update(); // Item marked as dirty
    scene.update(); // Entire scene marked as dirty
    qApp->processEvents();
    QCOMPARE(item->repaints, 1);

    // Make sure the dirty state from the previous update is reset so that
    // the item don't think it is already dirty and discards this update.
    item->update();
    qApp->processEvents();
    QCOMPARE(item->repaints, 2);

    // Make sure a partial update doesn't cause a full update to be discarded.
    view.reset();
    item->repaints = 0;
    item->update(QRectF(0, 0, 5, 5));
    item->update();
    qApp->processEvents();
    QCOMPARE(item->repaints, 1);
    QCOMPARE(view.repaints, 1);
    QRect itemDeviceBoundingRect = item->deviceTransform(view.viewportTransform())
                                                         .mapRect(item->boundingRect()).toAlignedRect();
    QRegion expectedRegion = itemDeviceBoundingRect.adjusted(-2, -2, 2, 2);
    // The entire item's bounding rect (adjusted for antialiasing) should have been painted.
    QCOMPARE(view.paintedRegion, expectedRegion);

    // Make sure update requests outside the bounding rect are discarded.
    view.reset();
    item->repaints = 0;
    item->update(-15, -15, 5, 5); // Item's brect: (-10, -10, 20, 20)
    qApp->processEvents();
    QCOMPARE(item->repaints, 0);
    QCOMPARE(view.repaints, 0);

    // Make sure the area occupied by an item is repainted when hiding it.
    view.reset();
    item->repaints = 0;
    item->update(); // Full update; all sub-sequent update requests are discarded.
    item->hide(); // visible set to 0. ignoreVisible must be set to 1; the item won't be processed otherwise.
    qApp->processEvents();
    QCOMPARE(item->repaints, 0);
    QCOMPARE(view.repaints, 1);
    // The entire item's bounding rect (adjusted for antialiasing) should have been painted.
    QCOMPARE(view.paintedRegion, expectedRegion);

    // Make sure item is repainted when shown (after being hidden).
    view.reset();
    item->repaints = 0;
    item->show();
    qApp->processEvents();
    QCOMPARE(item->repaints, 1);
    QCOMPARE(view.repaints, 1);
    // The entire item's bounding rect (adjusted for antialiasing) should have been painted.
    QCOMPARE(view.paintedRegion, expectedRegion);

    item->repaints = 0;
    item->hide();
    qApp->processEvents();
    view.reset();
    const QPointF originalPos = item->pos();
    item->setPos(5000, 5000);
    qApp->processEvents();
    QCOMPARE(item->repaints, 0);
    QCOMPARE(view.repaints, 0);
    qApp->processEvents();

    item->setPos(originalPos);
    qApp->processEvents();
    QCOMPARE(item->repaints, 0);
    QCOMPARE(view.repaints, 0);
    item->show();
    qApp->processEvents();
    QCOMPARE(item->repaints, 1);
    QCOMPARE(view.repaints, 1);
    // The entire item's bounding rect (adjusted for antialiasing) should have been painted.
    QCOMPARE(view.paintedRegion, expectedRegion);

    QGraphicsViewPrivate *viewPrivate = static_cast<QGraphicsViewPrivate *>(qt_widget_private(&view));
    item->setPos(originalPos + QPoint(50, 50));
    viewPrivate->updateAll();
    QVERIFY(viewPrivate->fullUpdatePending);
    QTest::qWait(50);
    item->repaints = 0;
    view.reset();
    item->setPos(originalPos);
    QTest::qWait(50);
    qApp->processEvents();
    QCOMPARE(item->repaints, 1);
    QCOMPARE(view.repaints, 1);
    COMPARE_REGIONS(view.paintedRegion, expectedRegion + expectedRegion.translated(50, 50));

    // Make sure moving a parent item triggers an update on the children
    // (even though the parent itself is outside the viewport).
    QGraphicsRectItem *parent = new QGraphicsRectItem(0, 0, 10, 10);
    parent->setPos(-400, 0);
    item->setParentItem(parent);
    item->setPos(400, 0);
    scene.addItem(parent);
    QTest::qWait(50);
    itemDeviceBoundingRect = item->deviceTransform(view.viewportTransform())
                                                   .mapRect(item->boundingRect()).toAlignedRect();
    expectedRegion = itemDeviceBoundingRect.adjusted(-2, -2, 2, 2);
    view.reset();
    item->repaints = 0;
    parent->translate(-400, 0);
    qApp->processEvents();
    QCOMPARE(item->repaints, 0);
    QCOMPARE(view.repaints, 1);
    QCOMPARE(view.paintedRegion, expectedRegion);
    view.reset();
    item->repaints = 0;
    parent->translate(400, 0);
    qApp->processEvents();
    QCOMPARE(item->repaints, 1);
    QCOMPARE(view.repaints, 1);
    QCOMPARE(view.paintedRegion, expectedRegion);
    QCOMPARE(view.paintedRegion, expectedRegion);
}

void tst_QGraphicsItem::setTransformProperties_data()
{
    QTest::addColumn<QPointF>("origin");
    QTest::addColumn<qreal>("rotation");
    QTest::addColumn<qreal>("scale");

    QTest::newRow("nothing") << QPointF() << qreal(0.0) << qreal(1.0);

    QTest::newRow("rotation") << QPointF() << qreal(42.2) << qreal(1.0);

    QTest::newRow("rotation dicentred") << QPointF(qreal(22.3), qreal(-56.2))
                                << qreal(-2578.2)
                                << qreal(1.0);

    QTest::newRow("Scale")    << QPointF() << qreal(0.0)
                                          << qreal(6);

    QTest::newRow("Everything dicentred")  << QPointF(qreal(22.3), qreal(-56.2)) << qreal(-175) << qreal(196);
}

/**
 * the normal QCOMPARE doesn't work because it doesn't use qFuzzyCompare
 */
#define QCOMPARE_TRANSFORM(X1, X2)   QVERIFY(((X1)*(X2).inverted()).isIdentity())

void tst_QGraphicsItem::setTransformProperties()
{
    QFETCH(QPointF,origin);
    QFETCH(qreal,rotation);
    QFETCH(qreal,scale);

    QTransform result;
    result.translate(origin.x(), origin.y());
    result.rotate(rotation, Qt::ZAxis);
    result.scale(scale, scale);
    result.translate(-origin.x(), -origin.y());

    QGraphicsScene scene;
    QGraphicsRectItem *item = new QGraphicsRectItem(QRectF(0, 0, 100, 100));
    scene.addItem(item);

    item->setRotation(rotation);
    item->setScale(scale);
    item->setTransformOriginPoint(origin);

    QCOMPARE(item->rotation(), rotation);
    QCOMPARE(item->scale(), scale);
    QCOMPARE(item->transformOriginPoint(), origin);

    QCOMPARE(QTransform(), item->transform());
    QCOMPARE(result, item->sceneTransform());

    //-----------------------------------------------------------------
    //Change the rotation Z
    item->setRotation(45);
    QTransform result2;
    result2.translate(origin.x(), origin.y());
    result2.rotate(45);
    result2.scale(scale, scale);
    result2.translate(-origin.x(), -origin.y());

    QCOMPARE(item->rotation(), 45.);
    QCOMPARE(item->scale(), scale);
    QCOMPARE(item->transformOriginPoint(), origin);

    QCOMPARE(QTransform(), item->transform());
    QCOMPARE(result2, item->sceneTransform());

    //-----------------------------------------------------------------
    // calling setTransform() and setPos should change the sceneTransform
    item->setTransform(result);
    item->setPos(100, -150.5);

    QCOMPARE(item->rotation(), 45.);
    QCOMPARE(item->scale(), scale);
    QCOMPARE(item->transformOriginPoint(), origin);
    QCOMPARE(result, item->transform());

    QTransform result3(result);

    result3.translate(origin.x(), origin.y());
    result3.rotate(45);
    result3.scale(scale, scale);
    result3.translate(-origin.x(), -origin.y());

    result3 *= QTransform::fromTranslate(100, -150.5); //the pos;

    QCOMPARE(result3, item->sceneTransform());

    //-----------------------------------------------------
    // setting the propertiees should be the same as setting a transform
    {//with center origin on the matrix
        QGraphicsRectItem *item1 = new QGraphicsRectItem(QRectF(50.2, -150, 230.5, 119));
        scene.addItem(item1);
        QGraphicsRectItem *item2 = new QGraphicsRectItem(QRectF(50.2, -150, 230.5, 119));
        scene.addItem(item2);

        item1->setPos(12.3, -5);
        item2->setPos(12.3, -5);
        item1->setRotation(rotation);
        item1->setScale(scale);
        item1->setTransformOriginPoint(origin);

        item2->setTransform(result);

        QCOMPARE_TRANSFORM(item1->sceneTransform(), item2->sceneTransform());

        QCOMPARE_TRANSFORM(item1->itemTransform(item2), QTransform());
        QCOMPARE_TRANSFORM(item2->itemTransform(item1), QTransform());
    }
}

class MyStyleOptionTester : public QGraphicsRectItem
{
public:
    MyStyleOptionTester(const QRectF &rect)
        : QGraphicsRectItem(rect), startTrack(false)
    {}

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0)
    {
        if (startTrack) {
            //Doesn't use the extended style option so the exposed rect is the boundingRect
            if (!(flags() & QGraphicsItem::ItemUsesExtendedStyleOption)) {
                QCOMPARE(option->exposedRect, boundingRect());
                QCOMPARE(option->matrix, QMatrix());
            } else {
                QVERIFY(option->exposedRect != QRect());
                QVERIFY(option->exposedRect != boundingRect());
                QCOMPARE(option->matrix, sceneTransform().toAffine());
            }
        }
        QGraphicsRectItem::paint(painter, option, widget);
    }
    bool startTrack;
};

void tst_QGraphicsItem::itemUsesExtendedStyleOption()
{
    QGraphicsScene scene(0, 0, 300, 300);
    QGraphicsPixmapItem item;
    item.setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
    QCOMPARE(item.flags(), QGraphicsItem::GraphicsItemFlags(QGraphicsItem::ItemUsesExtendedStyleOption));
    item.setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, false);
    QCOMPARE(item.flags(), 0);

    //We now test the content of the style option
    MyStyleOptionTester *rect = new MyStyleOptionTester(QRect(0, 0, 100, 100));
    scene.addItem(rect);
    rect->setPos(200, 200);
    QWidget topLevel;
    topLevel.resize(200, 200);
    QGraphicsView view(&scene, &topLevel);
    topLevel.setWindowFlags(Qt::X11BypassWindowManagerHint);
    rect->startTrack = false;
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QTest::qWait(60);
    rect->startTrack = true;
    rect->update(10, 10, 10, 10);
    QTest::qWait(60);
    rect->startTrack = false;
    rect->setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
    QVERIFY((rect->flags() & QGraphicsItem::ItemUsesExtendedStyleOption));
    QTest::qWait(60);
    rect->startTrack = true;
    rect->update(10, 10, 10, 10);
    QTest::qWait(60);
}

void tst_QGraphicsItem::itemSendsGeometryChanges()
{
    ItemChangeTester item;
    item.setFlags(0);
    item.clear();

    QTransform x = QTransform().rotate(45);
    QPointF pos(10, 10);
    qreal o(0.5);
    qreal r(10.0);
    qreal s(1.5);
    QPointF origin(1.0, 1.0);
    item.setTransform(x);
    item.setPos(pos);
    item.setRotation(r);
    item.setScale(s);
    item.setTransformOriginPoint(origin);
    QCOMPARE(item.transform(), x);
    QCOMPARE(item.pos(), pos);
    QCOMPARE(item.rotation(), r);
    QCOMPARE(item.scale(), s);
    QCOMPARE(item.transformOriginPoint(), origin);
    QCOMPARE(item.changes.size(), 0);

    item.setOpacity(o);
    QCOMPARE(item.changes.size(), 2); // opacity

    item.setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    QCOMPARE(item.changes.size(), 4); // flags
    item.setTransform(QTransform());
    item.setPos(QPointF());
    QCOMPARE(item.changes.size(), 8); // transform + pos
    QCOMPARE(item.transform(), QTransform());
    QCOMPARE(item.pos(), QPointF());
    QCOMPARE(item.opacity(), o);
    item.setRotation(0.0);
    item.setScale(1.0);
    item.setTransformOriginPoint(0.0, 0.0);
    QCOMPARE(item.changes.size(), 14); // rotation + scale + origin
    QCOMPARE(item.rotation(), qreal(0.0));
    QCOMPARE(item.scale(), qreal(1.0));
    QCOMPARE(item.transformOriginPoint(), QPointF(0.0, 0.0));

    QCOMPARE(item.changes, QList<QGraphicsItem::GraphicsItemChange>()
             << QGraphicsItem::ItemOpacityChange
             << QGraphicsItem::ItemOpacityHasChanged
             << QGraphicsItem::ItemFlagsChange
             << QGraphicsItem::ItemFlagsHaveChanged
             << QGraphicsItem::ItemTransformChange
             << QGraphicsItem::ItemTransformHasChanged
             << QGraphicsItem::ItemPositionChange
             << QGraphicsItem::ItemPositionHasChanged
             << QGraphicsItem::ItemRotationChange
             << QGraphicsItem::ItemRotationHasChanged
             << QGraphicsItem::ItemScaleChange
             << QGraphicsItem::ItemScaleHasChanged
             << QGraphicsItem::ItemTransformOriginPointChange
             << QGraphicsItem::ItemTransformOriginPointHasChanged);
}

// Make sure we update moved items correctly.
void tst_QGraphicsItem::moveItem()
{
    QGraphicsScene scene;
    scene.setSceneRect(-50, -50, 200, 200);

    MyGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    EventTester *parent = new EventTester;
    EventTester *child = new EventTester(parent);
    EventTester *grandChild = new EventTester(child);

#define RESET_COUNTERS \
    parent->repaints = 0; \
    child->repaints = 0; \
    grandChild->repaints = 0; \
    view.reset();

    scene.addItem(parent);
    QTest::qWait(100);

    RESET_COUNTERS

    // Item's boundingRect:  (-10, -10, 20, 20).
    QRect parentDeviceBoundingRect = parent->deviceTransform(view.viewportTransform())
                                     .mapRect(parent->boundingRect()).toAlignedRect()
                                     .adjusted(-2, -2, 2, 2); // Adjusted for antialiasing.

    parent->setPos(20, 20);
    qApp->processEvents();
    QCOMPARE(parent->repaints, 1);
    QCOMPARE(view.repaints, 1);
    QRegion expectedParentRegion = parentDeviceBoundingRect; // old position
    parentDeviceBoundingRect.translate(20, 20);
    expectedParentRegion += parentDeviceBoundingRect; // new position
    COMPARE_REGIONS(view.paintedRegion, expectedParentRegion);

    RESET_COUNTERS

    child->setPos(20, 20);
    qApp->processEvents();
    QCOMPARE(parent->repaints, 1);
    QCOMPARE(child->repaints, 1);
    QCOMPARE(view.repaints, 1);
    const QRegion expectedChildRegion = expectedParentRegion.translated(20, 20);
    COMPARE_REGIONS(view.paintedRegion, expectedChildRegion);

    RESET_COUNTERS

    grandChild->setPos(20, 20);
    qApp->processEvents();
    QCOMPARE(parent->repaints, 1);
    QCOMPARE(child->repaints, 1);
    QCOMPARE(grandChild->repaints, 1);
    QCOMPARE(view.repaints, 1);
    const QRegion expectedGrandChildRegion = expectedParentRegion.translated(40, 40);
    COMPARE_REGIONS(view.paintedRegion, expectedGrandChildRegion);

    RESET_COUNTERS

    parent->translate(20, 20);
    qApp->processEvents();
    QCOMPARE(parent->repaints, 1);
    QCOMPARE(child->repaints, 1);
    QCOMPARE(grandChild->repaints, 1);
    QCOMPARE(view.repaints, 1);
    expectedParentRegion.translate(20, 20);
    expectedParentRegion += expectedChildRegion.translated(20, 20);
    expectedParentRegion += expectedGrandChildRegion.translated(20, 20);
    COMPARE_REGIONS(view.paintedRegion, expectedParentRegion);
}

void tst_QGraphicsItem::moveLineItem()
{
    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 200, 200);
    QGraphicsLineItem *item = new QGraphicsLineItem(0, 0, 100, 0);
    item->setPos(50, 50);
    scene.addItem(item);

    MyGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    view.reset();

    QRectF brect = item->boundingRect();
    // Do same adjustments as in qgraphicsscene.cpp
    if (!brect.width())
        brect.adjust(qreal(-0.00001), 0, qreal(0.00001), 0);
    if (!brect.height())
        brect.adjust(0, qreal(-0.00001), 0, qreal(0.00001));
    const QRect itemDeviceBoundingRect = item->deviceTransform(view.viewportTransform())
                                         .mapRect(brect).toAlignedRect();
    QRegion expectedRegion = itemDeviceBoundingRect.adjusted(-2, -2, 2, 2); // antialiasing

    // Make sure the calculated region is correct.
    item->update();
    QTest::qWait(10);
    QTRY_COMPARE(view.paintedRegion, expectedRegion);
    view.reset();

    // Old position: (50, 50)
    item->setPos(50, 100);
    expectedRegion += expectedRegion.translated(0, 50);
    QTest::qWait(10);
    QCOMPARE(view.paintedRegion, expectedRegion);
}

void tst_QGraphicsItem::sorting_data()
{
    QTest::addColumn<int>("index");

    QTest::newRow("NoIndex") << int(QGraphicsScene::NoIndex);
    QTest::newRow("BspTreeIndex") << int(QGraphicsScene::BspTreeIndex);
}

void tst_QGraphicsItem::sorting()
{
    if (qGuiApp->styleHints()->showIsFullScreen())
        QSKIP("Skipped because Platform is auto maximizing");

    _paintedItems.clear();

    QGraphicsScene scene;
    QGraphicsItem *grid[100][100];
    for (int x = 0; x < 100; ++x) {
        for (int y = 0; y < 100; ++y) {
            PainterItem *item = new PainterItem;
            item->setPos(x * 25, y * 25);
            item->setData(0, QString("%1x%2").arg(x).arg(y));
            grid[x][y] = item;
            scene.addItem(item);
        }
    }

    PainterItem *item1 = new PainterItem;
    PainterItem *item2 = new PainterItem;
    item1->setData(0, "item1");
    item2->setData(0, "item2");
    scene.addItem(item1);
    scene.addItem(item2);

    QGraphicsView view(&scene);
    // Use Qt::Tool as fully decorated windows have a minimum width of 160 on Windows.
    view.setWindowFlags(view.windowFlags() | Qt::Tool);
    view.setResizeAnchor(QGraphicsView::NoAnchor);
    view.setTransformationAnchor(QGraphicsView::NoAnchor);
    view.resize(120, 100);
    view.setFrameStyle(0);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTest::qWait(100);

    _paintedItems.clear();

    view.viewport()->repaint();
#if defined(Q_OS_MAC)
    // There's no difference between repaint and update on the Mac,
    // so we have to process events here to make sure we get the event.
    QTest::qWait(100);
#endif

    QCOMPARE(_paintedItems, QList<QGraphicsItem *>()
                 << grid[0][0] << grid[0][1] << grid[0][2] << grid[0][3]
                 << grid[1][0] << grid[1][1] << grid[1][2] << grid[1][3]
                 << grid[2][0] << grid[2][1] << grid[2][2] << grid[2][3]
                 << grid[3][0] << grid[3][1] << grid[3][2] << grid[3][3]
                 << grid[4][0] << grid[4][1] << grid[4][2] << grid[4][3]
                 << item1 << item2);
}

void tst_QGraphicsItem::itemHasNoContents()
{
    PainterItem *item1 = new PainterItem;
    PainterItem *item2 = new PainterItem;
    item2->setParentItem(item1);
    item2->setPos(50, 50);
    item1->setFlag(QGraphicsItem::ItemHasNoContents);
    item1->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    QGraphicsScene scene;
    scene.addItem(item1);

    QGraphicsView view(&scene);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(!_paintedItems.isEmpty());

    _paintedItems.clear();

    view.viewport()->repaint();
#ifdef Q_OS_MAC
    // There's no difference between update() and repaint() on the Mac,
    // so we have to process events here to make sure we get the event.
    QTest::qWait(10);
#endif

    QTRY_COMPARE(_paintedItems, QList<QGraphicsItem *>() << item2);
}

void tst_QGraphicsItem::hitTestUntransformableItem()
{
    QGraphicsScene scene;
    scene.setSceneRect(-100, -100, 200, 200);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // Confuse the BSP with dummy items.
    QGraphicsRectItem *dummy = new QGraphicsRectItem(0, 0, 20, 20);
    dummy->setPos(-100, -100);
    scene.addItem(dummy);
    for (int i = 0; i < 100; ++i) {
        QGraphicsItem *parent = dummy;
        dummy = new QGraphicsRectItem(0, 0, 20, 20);
        dummy->setPos(-100 + i, -100 + i);
        dummy->setParentItem(parent);
    }

    QGraphicsRectItem *item1 = new QGraphicsRectItem(0, 0, 20, 20);
    item1->setPos(-200, -200);

    QGraphicsRectItem *item2 = new QGraphicsRectItem(0, 0, 20, 20);
    item2->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    item2->setParentItem(item1);
    item2->setPos(200, 200);

    QGraphicsRectItem *item3 = new QGraphicsRectItem(0, 0, 20, 20);
    item3->setParentItem(item2);
    item3->setPos(80, 80);

    scene.addItem(item1);
    QTest::qWait(100);

    QList<QGraphicsItem *> items = scene.items(QPointF(80, 80));
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.at(0), static_cast<QGraphicsItem*>(item3));

    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    QTest::qWait(100);

    items = scene.items(QPointF(80, 80));
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.at(0), static_cast<QGraphicsItem*>(item3));
}

void tst_QGraphicsItem::hitTestGraphicsEffectItem()
{
    QGraphicsScene scene;
    scene.setSceneRect(-100, -100, 200, 200);

    QWidget toplevel;

    QGraphicsView view(&scene, &toplevel);
    toplevel.resize(300, 300);
    toplevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toplevel));

    // Confuse the BSP with dummy items.
    QGraphicsRectItem *dummy = new QGraphicsRectItem(0, 0, 20, 20);
    dummy->setPos(-100, -100);
    scene.addItem(dummy);
    for (int i = 0; i < 100; ++i) {
        QGraphicsItem *parent = dummy;
        dummy = new QGraphicsRectItem(0, 0, 20, 20);
        dummy->setPos(-100 + i, -100 + i);
        dummy->setParentItem(parent);
    }

    const QRectF itemBoundingRect(0, 0, 20, 20);
    EventTester *item1 = new EventTester;
    item1->br = itemBoundingRect;
    item1->setPos(-200, -200);
    item1->brush = Qt::red;

    EventTester *item2 = new EventTester;
    item2->br = itemBoundingRect;
    item2->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    item2->setParentItem(item1);
    item2->setPos(200, 200);
    item2->brush = Qt::green;

    EventTester *item3 = new EventTester;
    item3->br = itemBoundingRect;
    item3->setParentItem(item2);
    item3->setPos(80, 80);
    item3->brush = Qt::blue;

    scene.addItem(item1);
    QTest::qWait(100);

    item1->repaints = 0;
    item2->repaints = 0;
    item3->repaints = 0;

    // Apply shadow effect to the entire sub-tree.
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setOffset(-20, -20);
    item1->setGraphicsEffect(shadow);
    QTest::qWait(50);

    // Make sure all visible items are repainted.
    QCOMPARE(item1->repaints, 1);
    QCOMPARE(item2->repaints, 1);
    QCOMPARE(item3->repaints, 1);

    // Make sure an item doesn't respond to a click on its shadow.
    QList<QGraphicsItem *> items = scene.items(QPointF(75, 75));
    QVERIFY(items.isEmpty());
    items = scene.items(QPointF(80, 80));
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.at(0), static_cast<QGraphicsItem *>(item3));

    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    QTest::qWait(100);

    items = scene.items(QPointF(75, 75));
    QVERIFY(items.isEmpty());
    items = scene.items(QPointF(80, 80));
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.at(0), static_cast<QGraphicsItem *>(item3));
}

void tst_QGraphicsItem::focusProxy()
{
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    QGraphicsItem *item = scene.addRect(0, 0, 10, 10);
    item->setFlag(QGraphicsItem::ItemIsFocusable);
    QVERIFY(!item->focusProxy());

    QGraphicsItem *item2 = scene.addRect(0, 0, 10, 10);
    item2->setFlag(QGraphicsItem::ItemIsFocusable);
    item->setFocusProxy(item2);
    QCOMPARE(item->focusProxy(), item2);

    item->setFocus();
    QVERIFY(item->hasFocus());
    QVERIFY(item2->hasFocus());

    // Try to make a focus chain loop
    QString err;
    QTextStream stream(&err);
    stream << "QGraphicsItem::setFocusProxy: "
           << (void*)item << " is already in the focus proxy chain" << flush;
    QTest::ignoreMessage(QtWarningMsg, err.toLatin1().constData());
    item2->setFocusProxy(item); // fails
    QCOMPARE(item->focusProxy(), (QGraphicsItem *)item2);
    QCOMPARE(item2->focusProxy(), (QGraphicsItem *)0);

    // Try to assign self as focus proxy
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsItem::setFocusProxy: cannot assign self as focus proxy");
    item->setFocusProxy(item); // fails
    QCOMPARE(item->focusProxy(), (QGraphicsItem *)item2);
    QCOMPARE(item2->focusProxy(), (QGraphicsItem *)0);

    // Reset the focus proxy
    item->setFocusProxy(0);
    QCOMPARE(item->focusProxy(), (QGraphicsItem *)0);
    QVERIFY(!item->hasFocus());
    QVERIFY(item2->hasFocus());

    // Test deletion
    item->setFocusProxy(item2);
    QCOMPARE(item->focusProxy(), (QGraphicsItem *)item2);
    delete item2;
    QCOMPARE(item->focusProxy(), (QGraphicsItem *)0);

    // Test event delivery
    item2 = scene.addRect(0, 0, 10, 10);
    item2->setFlag(QGraphicsItem::ItemIsFocusable);
    item->setFocusProxy(item2);
    item->clearFocus();

    EventSpy focusInSpy(&scene, item, QEvent::FocusIn);
    EventSpy focusOutSpy(&scene, item, QEvent::FocusOut);
    EventSpy focusInSpy2(&scene, item2, QEvent::FocusIn);
    EventSpy focusOutSpy2(&scene, item2, QEvent::FocusOut);
    QCOMPARE(focusInSpy.count(), 0);
    QCOMPARE(focusOutSpy.count(), 0);
    QCOMPARE(focusInSpy2.count(), 0);
    QCOMPARE(focusOutSpy2.count(), 0);

    item->setFocus();
    QCOMPARE(focusInSpy.count(), 0);
    QCOMPARE(focusInSpy2.count(), 1);
    item->clearFocus();
    QCOMPARE(focusOutSpy.count(), 0);
    QCOMPARE(focusOutSpy2.count(), 1);

    // Test two items proxying one item.
    QGraphicsItem *item3 = scene.addRect(0, 0, 10, 10);
    item3->setFlag(QGraphicsItem::ItemIsFocusable);
    item3->setFocusProxy(item2); // item and item3 use item2 as proxy

    QCOMPARE(item->focusProxy(), item2);
    QCOMPARE(item2->focusProxy(), (QGraphicsItem *)0);
    QCOMPARE(item3->focusProxy(), item2);
    delete item2;
    QCOMPARE(item->focusProxy(), (QGraphicsItem *)0);
    QCOMPARE(item3->focusProxy(), (QGraphicsItem *)0);
}

void tst_QGraphicsItem::subFocus()
{
    // Construct a text item that's not part of a scene (yet)
    // and has no parent. Setting focus on it will not make
    // the item gain input focus; that requires a scene. But
    // it does set subfocus, indicating that the item wishes
    // to gain focus later.
    QGraphicsTextItem *text = new QGraphicsTextItem("Hello");
    text->setTextInteractionFlags(Qt::TextEditorInteraction);
    QVERIFY(!text->hasFocus());
    text->setFocus();
    QVERIFY(!text->hasFocus());
    QCOMPARE(text->focusItem(), (QGraphicsItem *)text);

    // Add a sibling.
    QGraphicsTextItem *text2 = new QGraphicsTextItem("Hi");
    text2->setTextInteractionFlags(Qt::TextEditorInteraction);
    text2->setPos(30, 30);

    // Add both items to a scene and check that it's text that
    // got input focus.
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    scene.addItem(text);
    scene.addItem(text2);
    QVERIFY(text->hasFocus());

    text->setData(0, "text");
    text2->setData(0, "text2");

    // Remove text2 and set subfocus on it. Then readd. Reparent it onto the
    // other item and see that it gains input focus.
    scene.removeItem(text2);
    text2->setFocus();
    scene.addItem(text2);
    QCOMPARE(text2->focusItem(), (QGraphicsItem *)text2);
    text2->setParentItem(text);
    QCOMPARE(text->focusItem(), (QGraphicsItem *)text2);
    QCOMPARE(text2->focusItem(), (QGraphicsItem *)text2);
    QVERIFY(!text->hasFocus());
    QVERIFY(text2->hasFocus());

    // Remove both items from the scene, restore subfocus and
    // readd them. Now the subfocus should kick in and give
    // text2 focus.
    scene.removeItem(text);
    QCOMPARE(text->focusItem(), (QGraphicsItem *)0);
    QCOMPARE(text2->focusItem(), (QGraphicsItem *)0);
    text2->setFocus();
    QCOMPARE(text->focusItem(), (QGraphicsItem *)text2);
    QCOMPARE(text2->focusItem(), (QGraphicsItem *)text2);
    scene.addItem(text);

    // Hiding and showing text should pass focus to text2.
    QCOMPARE(text->focusItem(), (QGraphicsItem *)text2);
    QVERIFY(text2->hasFocus());

    // Subfocus should repropagate to root when reparenting.
    QGraphicsRectItem *rect = new QGraphicsRectItem;
    QGraphicsRectItem *rect2 = new QGraphicsRectItem(rect);
    QGraphicsRectItem *rect3 = new QGraphicsRectItem(rect2);
    rect3->setFlag(QGraphicsItem::ItemIsFocusable);

    text->setData(0, "text");
    text2->setData(0, "text2");
    rect->setData(0, "rect");
    rect2->setData(0, "rect2");
    rect3->setData(0, "rect3");

    rect3->setFocus();
    QVERIFY(!rect3->hasFocus());
    QCOMPARE(rect->focusItem(), (QGraphicsItem *)rect3);
    QCOMPARE(rect2->focusItem(), (QGraphicsItem *)rect3);
    QCOMPARE(rect3->focusItem(), (QGraphicsItem *)rect3);
    rect->setParentItem(text2);
    QCOMPARE(text->focusItem(), (QGraphicsItem *)rect3);
    QCOMPARE(text2->focusItem(), (QGraphicsItem *)rect3);
    QCOMPARE(rect->focusItem(), (QGraphicsItem *)rect3);
    QCOMPARE(rect2->focusItem(), (QGraphicsItem *)rect3);
    QCOMPARE(rect3->focusItem(), (QGraphicsItem *)rect3);
    QVERIFY(!rect->hasFocus());
    QVERIFY(!rect2->hasFocus());
    QVERIFY(rect3->hasFocus());

    delete rect2;
    QCOMPARE(text->focusItem(), (QGraphicsItem *)0);
    QCOMPARE(text2->focusItem(), (QGraphicsItem *)0);
    QCOMPARE(rect->focusItem(), (QGraphicsItem *)0);
}

void tst_QGraphicsItem::focusProxyDeletion()
{
    QGraphicsRectItem *rect = new QGraphicsRectItem;
    QGraphicsRectItem *rect2 = new QGraphicsRectItem;
    rect->setFocusProxy(rect2);
    QCOMPARE(rect->focusProxy(), (QGraphicsItem *)rect2);

    delete rect2;
    QCOMPARE(rect->focusProxy(), (QGraphicsItem *)0);

    rect2 = new QGraphicsRectItem;
    rect->setFocusProxy(rect2);
    QGraphicsItem **danglingFocusProxyRef = &rect->d_ptr->focusProxy;
    delete rect; // don't crash
    QVERIFY(!rect2->d_ptr->focusProxyRefs.contains(danglingFocusProxyRef));

    rect = new QGraphicsRectItem;
    rect->setFocusProxy(rect2);
    QGraphicsScene *scene = new QGraphicsScene;
    scene->addItem(rect);
    scene->addItem(rect2);
    delete rect2;
    QCOMPARE(rect->focusProxy(), (QGraphicsItem *)0);

    rect2 = new QGraphicsRectItem;
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsItem::setFocusProxy: focus proxy must be in same scene");
    rect->setFocusProxy(rect2);
    QCOMPARE(rect->focusProxy(), (QGraphicsItem *)0);
    scene->addItem(rect2);
    rect->setFocusProxy(rect2);
    QCOMPARE(rect->focusProxy(), (QGraphicsItem *)rect2);
    delete rect; // don't crash

    rect = new QGraphicsRectItem;
    rect2 = new QGraphicsRectItem;
    rect->setFocusProxy(rect2);
    QCOMPARE(rect->focusProxy(), (QGraphicsItem *)rect2);
    scene->addItem(rect);
    scene->addItem(rect2);
    rect->setFocusProxy(rect2);
    delete scene; // don't crash
}

void tst_QGraphicsItem::negativeZStacksBehindParent()
{
    QGraphicsRectItem rect;
    QCOMPARE(rect.zValue(), qreal(0.0));
    QVERIFY(!(rect.flags() & QGraphicsItem::ItemNegativeZStacksBehindParent));
    QVERIFY(!(rect.flags() & QGraphicsItem::ItemStacksBehindParent));
    rect.setZValue(-1);
    QCOMPARE(rect.zValue(), qreal(-1.0));
    QVERIFY(!(rect.flags() & QGraphicsItem::ItemStacksBehindParent));
    rect.setZValue(0);
    rect.setFlag(QGraphicsItem::ItemNegativeZStacksBehindParent);
    QVERIFY(rect.flags() & QGraphicsItem::ItemNegativeZStacksBehindParent);
    QVERIFY(!(rect.flags() & QGraphicsItem::ItemStacksBehindParent));
    rect.setZValue(-1);
    QVERIFY(rect.flags() & QGraphicsItem::ItemStacksBehindParent);
    rect.setZValue(0);
    QVERIFY(!(rect.flags() & QGraphicsItem::ItemStacksBehindParent));
    rect.setFlag(QGraphicsItem::ItemNegativeZStacksBehindParent, false);
    rect.setZValue(-1);
    rect.setFlag(QGraphicsItem::ItemNegativeZStacksBehindParent, true);
    QVERIFY(rect.flags() & QGraphicsItem::ItemStacksBehindParent);
    rect.setFlag(QGraphicsItem::ItemNegativeZStacksBehindParent, false);
    QVERIFY(rect.flags() & QGraphicsItem::ItemStacksBehindParent);
}

void tst_QGraphicsItem::setGraphicsEffect()
{
    // Check that we don't have any effect by default.
    QGraphicsItem *item = new QGraphicsRectItem(0, 0, 10, 10);
    QVERIFY(!item->graphicsEffect());

    // SetGet check.
    QPointer<QGraphicsEffect> blurEffect = new QGraphicsBlurEffect;
    item->setGraphicsEffect(blurEffect);
    QCOMPARE(item->graphicsEffect(), static_cast<QGraphicsEffect *>(blurEffect));

    // Ensure the existing effect is deleted when setting a new one.
    QPointer<QGraphicsEffect> shadowEffect = new QGraphicsDropShadowEffect;
    item->setGraphicsEffect(shadowEffect);
    QVERIFY(!blurEffect);
    QCOMPARE(item->graphicsEffect(), static_cast<QGraphicsEffect *>(shadowEffect));
    blurEffect = new QGraphicsBlurEffect;

    // Ensure the effect is uninstalled when setting it on a new target.
    QGraphicsItem *anotherItem = new QGraphicsRectItem(0, 0, 10, 10);
    anotherItem->setGraphicsEffect(blurEffect);
    item->setGraphicsEffect(blurEffect);
    QVERIFY(!anotherItem->graphicsEffect());
    QVERIFY(!shadowEffect);

    // Ensure the existing effect is deleted when deleting the item.
    delete item;
    QVERIFY(!blurEffect);
    delete anotherItem;

    // Ensure the effect is uninstalled when deleting it
    item = new QGraphicsRectItem(0, 0, 10, 10);
    blurEffect = new QGraphicsBlurEffect;
    item->setGraphicsEffect(blurEffect);
    delete blurEffect;
    QVERIFY(!item->graphicsEffect());

    // Ensure the existing effect is uninstalled and deleted when setting a null effect
    blurEffect = new QGraphicsBlurEffect;
    item->setGraphicsEffect(blurEffect);
    item->setGraphicsEffect(0);
    QVERIFY(!item->graphicsEffect());
    QVERIFY(!blurEffect);

    delete item;
}

void tst_QGraphicsItem::panel()
{
    QGraphicsScene scene;

    QGraphicsRectItem *panel1 = new QGraphicsRectItem;
    QGraphicsRectItem *panel2 = new QGraphicsRectItem;
    QGraphicsRectItem *panel3 = new QGraphicsRectItem;
    QGraphicsRectItem *panel4 = new QGraphicsRectItem;
    QGraphicsRectItem *notPanel1 = new QGraphicsRectItem;
    QGraphicsRectItem *notPanel2 = new QGraphicsRectItem;
    panel1->setFlag(QGraphicsItem::ItemIsPanel);
    panel2->setFlag(QGraphicsItem::ItemIsPanel);
    panel3->setFlag(QGraphicsItem::ItemIsPanel);
    panel4->setFlag(QGraphicsItem::ItemIsPanel);
    scene.addItem(panel1);
    scene.addItem(panel2);
    scene.addItem(panel3);
    scene.addItem(panel4);
    scene.addItem(notPanel1);
    scene.addItem(notPanel2);

    EventSpy spy_activate_panel1(&scene, panel1, QEvent::WindowActivate);
    EventSpy spy_deactivate_panel1(&scene, panel1, QEvent::WindowDeactivate);
    EventSpy spy_activate_panel2(&scene, panel2, QEvent::WindowActivate);
    EventSpy spy_deactivate_panel2(&scene, panel2, QEvent::WindowDeactivate);
    EventSpy spy_activate_panel3(&scene, panel3, QEvent::WindowActivate);
    EventSpy spy_deactivate_panel3(&scene, panel3, QEvent::WindowDeactivate);
    EventSpy spy_activate_panel4(&scene, panel4, QEvent::WindowActivate);
    EventSpy spy_deactivate_panel4(&scene, panel4, QEvent::WindowDeactivate);
    EventSpy spy_activate_notPanel1(&scene, notPanel1, QEvent::WindowActivate);
    EventSpy spy_deactivate_notPanel1(&scene, notPanel1, QEvent::WindowDeactivate);
    EventSpy spy_activate_notPanel2(&scene, notPanel1, QEvent::WindowActivate);
    EventSpy spy_deactivate_notPanel2(&scene, notPanel1, QEvent::WindowDeactivate);

    QCOMPARE(spy_activate_panel1.count(), 0);
    QCOMPARE(spy_deactivate_panel1.count(), 0);
    QCOMPARE(spy_activate_panel2.count(), 0);
    QCOMPARE(spy_deactivate_panel2.count(), 0);
    QCOMPARE(spy_activate_panel3.count(), 0);
    QCOMPARE(spy_deactivate_panel3.count(), 0);
    QCOMPARE(spy_activate_panel4.count(), 0);
    QCOMPARE(spy_deactivate_panel4.count(), 0);
    QCOMPARE(spy_activate_notPanel1.count(), 0);
    QCOMPARE(spy_deactivate_notPanel1.count(), 0);
    QCOMPARE(spy_activate_notPanel2.count(), 0);
    QCOMPARE(spy_deactivate_notPanel2.count(), 0);

    QVERIFY(!scene.activePanel());
    QVERIFY(!scene.isActive());

    QEvent activate(QEvent::WindowActivate);
    QEvent deactivate(QEvent::WindowDeactivate);

    QApplication::sendEvent(&scene, &activate);

    // No previous activation, so the scene is active.
    QVERIFY(scene.isActive());
    QCOMPARE(scene.activePanel(), (QGraphicsItem *)panel1);
    QVERIFY(panel1->isActive());
    QVERIFY(!panel2->isActive());
    QVERIFY(!panel3->isActive());
    QVERIFY(!panel4->isActive());
    QVERIFY(!notPanel1->isActive());
    QVERIFY(!notPanel2->isActive());
    QCOMPARE(spy_deactivate_notPanel1.count(), 0);
    QCOMPARE(spy_deactivate_notPanel2.count(), 0);
    QCOMPARE(spy_activate_panel1.count(), 1);
    QCOMPARE(spy_activate_panel2.count(), 0);
    QCOMPARE(spy_activate_panel3.count(), 0);
    QCOMPARE(spy_activate_panel4.count(), 0);

    // Switch back to scene.
    scene.setActivePanel(0);
    QVERIFY(!scene.activePanel());
    QVERIFY(!panel1->isActive());
    QVERIFY(!panel2->isActive());
    QVERIFY(!panel3->isActive());
    QVERIFY(!panel4->isActive());
    QVERIFY(notPanel1->isActive());
    QVERIFY(notPanel2->isActive());
    QCOMPARE(spy_activate_notPanel1.count(), 1);
    QCOMPARE(spy_activate_notPanel2.count(), 1);

    // Deactivate the scene
    QApplication::sendEvent(&scene, &deactivate);
    QVERIFY(!scene.activePanel());
    QVERIFY(!panel1->isActive());
    QVERIFY(!panel2->isActive());
    QVERIFY(!panel3->isActive());
    QVERIFY(!panel4->isActive());
    QVERIFY(!notPanel1->isActive());
    QVERIFY(!notPanel2->isActive());
    QCOMPARE(spy_deactivate_notPanel1.count(), 1);
    QCOMPARE(spy_deactivate_notPanel2.count(), 1);

    // Reactivate the scene
    QApplication::sendEvent(&scene, &activate);
    QVERIFY(!scene.activePanel());
    QVERIFY(!panel1->isActive());
    QVERIFY(!panel2->isActive());
    QVERIFY(!panel3->isActive());
    QVERIFY(!panel4->isActive());
    QVERIFY(notPanel1->isActive());
    QVERIFY(notPanel2->isActive());
    QCOMPARE(spy_activate_notPanel1.count(), 2);
    QCOMPARE(spy_activate_notPanel2.count(), 2);

    // Switch to panel1
    scene.setActivePanel(panel1);
    QVERIFY(panel1->isActive());
    QCOMPARE(spy_deactivate_notPanel1.count(), 2);
    QCOMPARE(spy_deactivate_notPanel2.count(), 2);
    QCOMPARE(spy_activate_panel1.count(), 2);

    // Deactivate the scene
    QApplication::sendEvent(&scene, &deactivate);
    QVERIFY(!panel1->isActive());
    QCOMPARE(spy_deactivate_panel1.count(), 2);

    // Reactivate the scene
    QApplication::sendEvent(&scene, &activate);
    QVERIFY(panel1->isActive());
    QCOMPARE(spy_activate_panel1.count(), 3);

    // Deactivate the scene
    QApplication::sendEvent(&scene, &deactivate);
    QVERIFY(!panel1->isActive());
    QVERIFY(!scene.activePanel());
    scene.setActivePanel(0);

    // Reactivate the scene
    QApplication::sendEvent(&scene, &activate);
    QVERIFY(!panel1->isActive());
}

void tst_QGraphicsItem::panelWithFocusItems()
{
    for (int i = 0; i < 2; ++i)
    {
        QGraphicsScene scene;
        QEvent activate(QEvent::WindowActivate);
        QApplication::sendEvent(&scene, &activate);

        bool widget = (i == 1);
        QGraphicsItem *parentPanel = widget ? (QGraphicsItem *)new QGraphicsWidget : (QGraphicsItem *)new QGraphicsRectItem;
        QGraphicsItem *parentPanelFocusItem = widget ? (QGraphicsItem *)new QGraphicsWidget : (QGraphicsItem *)new QGraphicsRectItem;
        QGraphicsItem *parentPanelFocusItemSibling = widget ? (QGraphicsItem *)new QGraphicsWidget : (QGraphicsItem *)new QGraphicsRectItem;
        parentPanel->setFlag(QGraphicsItem::ItemIsPanel);
        parentPanelFocusItem->setFlag(QGraphicsItem::ItemIsFocusable);
        parentPanelFocusItemSibling->setFlag(QGraphicsItem::ItemIsFocusable);
        if (widget) {
            static_cast<QGraphicsWidget *>(parentPanelFocusItem)->setFocusPolicy(Qt::StrongFocus);
            static_cast<QGraphicsWidget *>(parentPanelFocusItemSibling)->setFocusPolicy(Qt::StrongFocus);
        }
        parentPanelFocusItem->setParentItem(parentPanel);
        parentPanelFocusItemSibling->setParentItem(parentPanel);
        parentPanelFocusItem->setFocus();
        scene.addItem(parentPanel);

        QVERIFY(parentPanel->isActive());
        QVERIFY(parentPanelFocusItem->hasFocus());
        QCOMPARE(parentPanel->focusItem(), (QGraphicsItem *)parentPanelFocusItem);
        QCOMPARE(parentPanelFocusItem->focusItem(), (QGraphicsItem *)parentPanelFocusItem);

        QGraphicsItem *childPanel = widget ? (QGraphicsItem *)new QGraphicsWidget : (QGraphicsItem *)new QGraphicsRectItem;
        QGraphicsItem *childPanelFocusItem = widget ? (QGraphicsItem *)new QGraphicsWidget : (QGraphicsItem *)new QGraphicsRectItem;
        QGraphicsItem *grandChildPanelFocusItem = widget ? (QGraphicsItem *)new QGraphicsWidget : (QGraphicsItem *)new QGraphicsRectItem;
        QGraphicsItem *grandChildPanelFocusItem2 = widget ? (QGraphicsItem *)new QGraphicsWidget : (QGraphicsItem *)new QGraphicsRectItem;

        childPanel->setFlag(QGraphicsItem::ItemIsPanel);
        childPanelFocusItem->setFlag(QGraphicsItem::ItemIsFocusable);
        grandChildPanelFocusItem->setFlag(QGraphicsItem::ItemIsFocusable);
        grandChildPanelFocusItem2->setFlag(QGraphicsItem::ItemIsFocusable);

        if (widget)
        {
            static_cast<QGraphicsWidget *>(childPanelFocusItem)->setFocusPolicy(Qt::StrongFocus);
            static_cast<QGraphicsWidget *>(grandChildPanelFocusItem)->setFocusPolicy(Qt::StrongFocus);
            static_cast<QGraphicsWidget *>(grandChildPanelFocusItem2)->setFocusPolicy(Qt::StrongFocus);
        }
        grandChildPanelFocusItem->setParentItem(childPanelFocusItem);
        grandChildPanelFocusItem2->setParentItem(childPanelFocusItem);
        childPanelFocusItem->setParentItem(childPanel);
        grandChildPanelFocusItem->setFocus();

        QVERIFY(!grandChildPanelFocusItem->hasFocus());
        QCOMPARE(childPanel->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);
        QCOMPARE(childPanelFocusItem->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);
        QCOMPARE(grandChildPanelFocusItem->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);

        childPanel->setParentItem(parentPanel);

        QVERIFY(!parentPanel->isActive());
        QVERIFY(!parentPanelFocusItem->hasFocus());
        QCOMPARE(parentPanel->focusItem(), (QGraphicsItem *)parentPanelFocusItem);
        QCOMPARE(parentPanelFocusItem->focusItem(), (QGraphicsItem *)parentPanelFocusItem);

        QVERIFY(childPanel->isActive());
        QVERIFY(!childPanelFocusItem->hasFocus());
        QVERIFY(grandChildPanelFocusItem->hasFocus());
        QCOMPARE(childPanel->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);
        QCOMPARE(childPanelFocusItem->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);

        childPanel->hide();
        QCOMPARE(childPanel->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);
        QVERIFY(!childPanel->focusItem()->hasFocus());
        QVERIFY(parentPanel->isActive());
        QVERIFY(parentPanelFocusItem->hasFocus());
        QCOMPARE(parentPanel->focusItem(), (QGraphicsItem *)parentPanelFocusItem);
        QCOMPARE(parentPanelFocusItem->focusItem(), (QGraphicsItem *)parentPanelFocusItem);
        QCOMPARE(grandChildPanelFocusItem->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);

        childPanel->show();
        QVERIFY(childPanel->isActive());
        QVERIFY(grandChildPanelFocusItem->hasFocus());
        QCOMPARE(childPanel->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);
        QCOMPARE(childPanelFocusItem->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);
        QCOMPARE(grandChildPanelFocusItem->focusItem(), (QGraphicsItem *)grandChildPanelFocusItem);

        childPanel->hide();

        QVERIFY(parentPanel->isActive());
        QVERIFY(parentPanelFocusItem->hasFocus());
        QCOMPARE(parentPanel->focusItem(), (QGraphicsItem *)parentPanelFocusItem);
        QCOMPARE(parentPanelFocusItem->focusItem(), (QGraphicsItem *)parentPanelFocusItem);
    }
}

void tst_QGraphicsItem::addPanelToActiveScene()
{
    QGraphicsScene scene;
    QVERIFY(!scene.isActive());

    QGraphicsRectItem *rect = new QGraphicsRectItem;
    scene.addItem(rect);
    QVERIFY(!rect->isActive());
    scene.removeItem(rect);

    QEvent activate(QEvent::WindowActivate);
    QEvent deactivate(QEvent::WindowDeactivate);

    QApplication::sendEvent(&scene, &activate);
    QVERIFY(scene.isActive());
    scene.addItem(rect);
    QVERIFY(rect->isActive());
    scene.removeItem(rect);

    rect->setFlag(QGraphicsItem::ItemIsPanel);
    scene.addItem(rect);
    QVERIFY(rect->isActive());
    QCOMPARE(scene.activePanel(), (QGraphicsItem *)rect);

    QGraphicsRectItem *rect2 = new QGraphicsRectItem;
    scene.addItem(rect2);
    QVERIFY(rect->isActive());
    QCOMPARE(scene.activePanel(), (QGraphicsItem *)rect);
}

void tst_QGraphicsItem::activate()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rect = scene.addRect(-10, -10, 20, 20);
    QVERIFY(!rect->isActive());

    QEvent activate(QEvent::WindowActivate);
    QEvent deactivate(QEvent::WindowDeactivate);

    QApplication::sendEvent(&scene, &activate);

    // Non-panel item (active when scene is active).
    QVERIFY(rect->isActive());

    QGraphicsRectItem *rect2 = new QGraphicsRectItem;
    rect2->setFlag(QGraphicsItem::ItemIsPanel);
    QGraphicsRectItem *rect3 = new QGraphicsRectItem;
    rect3->setFlag(QGraphicsItem::ItemIsPanel);

    // Test normal activation.
    QVERIFY(!rect2->isActive());
    scene.addItem(rect2);
    QVERIFY(rect2->isActive()); // first panel item is activated
    scene.addItem(rect3);
    QVERIFY(!rect3->isActive()); // second panel item is _not_ activated
    rect3->setActive(true);
    QVERIFY(rect3->isActive());
    scene.removeItem(rect3);
    QVERIFY(!rect3->isActive()); // no panel is active anymore
    QCOMPARE(scene.activePanel(), (QGraphicsItem *)0);
    scene.addItem(rect3);
    QVERIFY(rect3->isActive()); // second panel item is activated

    // Test pending activation.
    scene.removeItem(rect3);
    rect2->setActive(true);
    QVERIFY(rect2->isActive()); // first panel item is activated
    rect3->setActive(true);
    QVERIFY(!rect3->isActive()); // not active (yet)
    scene.addItem(rect3);
    QVERIFY(rect3->isActive()); // now becomes active

    // Test pending deactivation.
    scene.removeItem(rect3);
    rect3->setActive(false);
    scene.addItem(rect3);
    QVERIFY(!rect3->isActive()); // doesn't become active

    // Child of panel activation.
    rect3->setActive(true);
    QGraphicsRectItem *rect4 = new QGraphicsRectItem;
    rect4->setFlag(QGraphicsItem::ItemIsPanel);
    QGraphicsRectItem *rect5 = new QGraphicsRectItem(rect4);
    QGraphicsRectItem *rect6 = new QGraphicsRectItem(rect5);
    scene.addItem(rect4);
    QCOMPARE(scene.activePanel(), (QGraphicsItem *)rect3);
    scene.removeItem(rect4);
    rect6->setActive(true);
    scene.addItem(rect4);
    QVERIFY(rect4->isActive());
    QVERIFY(rect5->isActive());
    QVERIFY(rect6->isActive());
    QCOMPARE(scene.activePanel(), (QGraphicsItem *)rect4);
    scene.removeItem(rect4); // no active panel
    rect6->setActive(false);
    scene.addItem(rect4);
    QVERIFY(!rect4->isActive());
    QVERIFY(!rect5->isActive());
    QVERIFY(!rect6->isActive());
    QCOMPARE(scene.activePanel(), (QGraphicsItem *)0);

    // Controlling auto-activation when the scene changes activation.
    rect4->setActive(true);
    QApplication::sendEvent(&scene, &deactivate);
    QVERIFY(!scene.isActive());
    QVERIFY(!rect4->isActive());
    rect4->setActive(false);
    QApplication::sendEvent(&scene, &activate);
    QVERIFY(scene.isActive());
    QVERIFY(!scene.activePanel());
    QVERIFY(!rect4->isActive());
}

void tst_QGraphicsItem::setActivePanelOnInactiveScene()
{
    QGraphicsScene scene;
    QGraphicsRectItem *item = scene.addRect(QRectF());
    QGraphicsRectItem *panel = scene.addRect(QRectF());
    panel->setFlag(QGraphicsItem::ItemIsPanel);

    EventSpy itemActivateSpy(&scene, item, QEvent::WindowActivate);
    EventSpy itemDeactivateSpy(&scene, item, QEvent::WindowDeactivate);
    EventSpy panelActivateSpy(&scene, panel, QEvent::WindowActivate);
    EventSpy panelDeactivateSpy(&scene, panel, QEvent::WindowDeactivate);
    EventSpy sceneActivationChangeSpy(&scene, QEvent::ActivationChange);

    scene.setActivePanel(panel);
    QCOMPARE(scene.activePanel(), (QGraphicsItem *)0);
    QCOMPARE(itemActivateSpy.count(), 0);
    QCOMPARE(itemDeactivateSpy.count(), 0);
    QCOMPARE(panelActivateSpy.count(), 0);
    QCOMPARE(panelDeactivateSpy.count(), 0);
    QCOMPARE(sceneActivationChangeSpy.count(), 0);
}

void tst_QGraphicsItem::activationOnShowHide()
{
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    QGraphicsRectItem *rootPanel = scene.addRect(QRectF());
    rootPanel->setFlag(QGraphicsItem::ItemIsPanel);
    rootPanel->setActive(true);

    QGraphicsRectItem *subPanel = new QGraphicsRectItem;
    subPanel->setFlag(QGraphicsItem::ItemIsPanel);

    // Reparenting onto an active panel auto-activates the child panel.
    subPanel->setParentItem(rootPanel);
    QVERIFY(subPanel->isActive());
    QVERIFY(!rootPanel->isActive());

    // Hiding an active child panel will reactivate the parent panel.
    subPanel->hide();
    QVERIFY(rootPanel->isActive());

    // Showing a child panel will auto-activate it.
    subPanel->show();
    QVERIFY(subPanel->isActive());
    QVERIFY(!rootPanel->isActive());

    // Adding an unrelated panel doesn't affect activation.
    QGraphicsRectItem *otherPanel = new QGraphicsRectItem;
    otherPanel->setFlag(QGraphicsItem::ItemIsPanel);
    scene.addItem(otherPanel);
    QVERIFY(subPanel->isActive());

    // Showing an unrelated panel doesn't affect activation.
    otherPanel->hide();
    otherPanel->show();
    QVERIFY(subPanel->isActive());

    // Add a non-panel item.
    QGraphicsRectItem *otherItem = new QGraphicsRectItem;
    scene.addItem(otherItem);
    otherItem->setActive(true);
    QVERIFY(otherItem->isActive());

    // Reparent a panel onto an active non-panel item.
    subPanel->setParentItem(otherItem);
    QVERIFY(subPanel->isActive());

    // Showing a child panel of a non-panel item will activate it.
    subPanel->hide();
    QVERIFY(!subPanel->isActive());
    QVERIFY(otherItem->isActive());
    subPanel->show();
    QVERIFY(subPanel->isActive());

    // Hiding a toplevel active panel will pass activation back
    // to the non-panel items.
    rootPanel->setActive(true);
    rootPanel->hide();
    QVERIFY(!rootPanel->isActive());
    QVERIFY(otherItem->isActive());
}

class MoveWhileDying : public QGraphicsRectItem
{
public:
    MoveWhileDying(QGraphicsItem *parent = 0)
        : QGraphicsRectItem(parent)
    { }
    ~MoveWhileDying()
    {
        foreach (QGraphicsItem *c, childItems()) {
            foreach (QGraphicsItem *cc, c->childItems()) {
                cc->moveBy(10, 10);
            }
            c->moveBy(10, 10);
        }
        if (QGraphicsItem *p = parentItem()) { p->moveBy(10, 10); }
    }
};

void tst_QGraphicsItem::deactivateInactivePanel()
{
    QGraphicsScene scene;
    QGraphicsItem *panel1 = scene.addRect(QRectF(0, 0, 10, 10));
    panel1->setFlag(QGraphicsItem::ItemIsPanel);

    QGraphicsItem *panel2 = scene.addRect(QRectF(0, 0, 10, 10));
    panel2->setFlag(QGraphicsItem::ItemIsPanel);

    QEvent event(QEvent::WindowActivate);
    qApp->sendEvent(&scene, &event);

    panel1->setActive(true);
    QVERIFY(scene.isActive());
    QVERIFY(panel1->isActive());
    QVERIFY(!panel2->isActive());
    QCOMPARE(scene.activePanel(), panel1);

    panel2->setActive(true);
    QVERIFY(panel2->isActive());
    QVERIFY(!panel1->isActive());
    QCOMPARE(scene.activePanel(), panel2);

    panel2->setActive(false);
    QVERIFY(panel1->isActive());
    QVERIFY(!panel2->isActive());
    QCOMPARE(scene.activePanel(), panel1);

    panel2->setActive(false);
    QVERIFY(panel1->isActive());
    QVERIFY(!panel2->isActive());
    QCOMPARE(scene.activePanel(), panel1);
}

void tst_QGraphicsItem::moveWhileDeleting()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rect = new QGraphicsRectItem;
    MoveWhileDying *silly = new MoveWhileDying(rect);
    QGraphicsRectItem *child = new QGraphicsRectItem(silly);
    scene.addItem(rect);
    delete rect; // don't crash!

    rect = new QGraphicsRectItem;
    silly = new MoveWhileDying(rect);
    child = new QGraphicsRectItem(silly);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    delete rect;

    rect = new QGraphicsRectItem;
    rect->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    silly = new MoveWhileDying(rect);
    child = new QGraphicsRectItem(silly);

    QTest::qWait(125);

    delete rect;

    rect = new MoveWhileDying;
    rect->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    child = new QGraphicsRectItem(rect);
    silly = new MoveWhileDying(child);

    QTest::qWait(125);

    delete rect;
}

class MyRectItem : public QGraphicsWidget
{
    Q_OBJECT
public:
    MyRectItem(QGraphicsItem *parent = 0) : QGraphicsWidget(parent)
    {

    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
    {
        painter->setBrush(brush);
        painter->drawRect(boundingRect());
    }
    void move()
    {
        setPos(-100,-100);
        topLevel->collidingItems(Qt::IntersectsItemBoundingRect);
    }
public:
    QGraphicsItem *topLevel;
    QBrush brush;
};


void tst_QGraphicsItem::ensureDirtySceneTransform()
{
    QGraphicsScene scene;

    MyRectItem *topLevel = new MyRectItem;
    topLevel->setGeometry(0, 0, 100, 100);
    topLevel->setPos(-50, -50);
    topLevel->brush = QBrush(QColor(Qt::black));
    scene.addItem(topLevel);

    MyRectItem *parent = new MyRectItem;
    parent->topLevel = topLevel;
    parent->setGeometry(0, 0, 100, 100);
    parent->setPos(0, 0);
    parent->brush = QBrush(QColor(Qt::magenta));
    parent->setObjectName("parent");
    scene.addItem(parent);

    MyRectItem *child = new MyRectItem(parent);
    child->setGeometry(0, 0, 80, 80);
    child->setPos(10, 10);
    child->setObjectName("child");
    child->brush = QBrush(QColor(Qt::blue));

    MyRectItem *child2 = new MyRectItem(parent);
    child2->setGeometry(0, 0, 80, 80);
    child2->setPos(15, 15);
    child2->setObjectName("child2");
    child2->brush = QBrush(QColor(Qt::green));

    MyRectItem *child3 = new MyRectItem(parent);
    child3->setGeometry(0, 0, 80, 80);
    child3->setPos(20, 20);
    child3->setObjectName("child3");
    child3->brush = QBrush(QColor(Qt::gray));

    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&view));

    //We move the parent
    parent->move();
    QApplication::processEvents();

    //We check if all items moved
    QCOMPARE(child->pos(), QPointF(10, 10));
    QCOMPARE(child2->pos(), QPointF(15, 15));
    QCOMPARE(child3->pos(), QPointF(20, 20));

    QCOMPARE(child->sceneBoundingRect(), QRectF(-90, -90, 80, 80));
    QCOMPARE(child2->sceneBoundingRect(), QRectF(-85, -85, 80, 80));
    QCOMPARE(child3->sceneBoundingRect(), QRectF(-80, -80, 80, 80));

    QCOMPARE(child->sceneTransform(), QTransform::fromTranslate(-90, -90));
    QCOMPARE(child2->sceneTransform(), QTransform::fromTranslate(-85, -85));
    QCOMPARE(child3->sceneTransform(), QTransform::fromTranslate(-80, -80));
}

void tst_QGraphicsItem::focusScope()
{
    // ItemIsFocusScope is an internal feature (for now).
    QGraphicsScene scene;

    QGraphicsRectItem *scope3 = new QGraphicsRectItem;
    scope3->setData(0, "scope3");
    scope3->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsFocusScope);
    scope3->setFocus();
    QVERIFY(!scope3->focusScopeItem());
    QCOMPARE(scope3->focusItem(), (QGraphicsItem *)scope3);

    QGraphicsRectItem *scope2 = new QGraphicsRectItem;
    scope2->setData(0, "scope2");
    scope2->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsFocusScope);
    scope2->setFocus();
    QVERIFY(!scope2->focusScopeItem());
    scope3->setParentItem(scope2);
    QCOMPARE(scope2->focusScopeItem(), (QGraphicsItem *)scope3);
    QCOMPARE(scope2->focusItem(), (QGraphicsItem *)scope3);

    QGraphicsRectItem *scope1 = new QGraphicsRectItem;
    scope1->setData(0, "scope1");
    scope1->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsFocusScope);
    scope1->setFocus();
    QVERIFY(!scope1->focusScopeItem());
    scope2->setParentItem(scope1);

    QCOMPARE(scope1->focusItem(), (QGraphicsItem *)scope3);
    QCOMPARE(scope2->focusItem(), (QGraphicsItem *)scope3);
    QCOMPARE(scope3->focusItem(), (QGraphicsItem *)scope3);
    QCOMPARE(scope1->focusScopeItem(), (QGraphicsItem *)scope2);
    QCOMPARE(scope2->focusScopeItem(), (QGraphicsItem *)scope3);
    QCOMPARE(scope3->focusScopeItem(), (QGraphicsItem *)0);

    scene.addItem(scope1);

    QEvent windowActivate(QEvent::WindowActivate);
    qApp->sendEvent(&scene, &windowActivate);
    scene.setFocus();

    QCOMPARE(scope1->focusItem(), (QGraphicsItem *)scope3);
    QCOMPARE(scope2->focusItem(), (QGraphicsItem *)scope3);
    QCOMPARE(scope3->focusItem(), (QGraphicsItem *)scope3);
    QCOMPARE(scope1->focusScopeItem(), (QGraphicsItem *)scope2);
    QCOMPARE(scope2->focusScopeItem(), (QGraphicsItem *)scope3);
    QCOMPARE(scope3->focusScopeItem(), (QGraphicsItem *)0);

    QVERIFY(scope3->hasFocus());

    scope3->hide();
    QVERIFY(scope2->hasFocus());
    scope2->hide();
    QVERIFY(scope1->hasFocus());
    scope2->show();
    QVERIFY(scope2->hasFocus());
    scope3->show();
    QVERIFY(scope3->hasFocus());
    scope1->hide();
    QVERIFY(!scope3->hasFocus());
    scope1->show();
    QVERIFY(scope3->hasFocus());
    scope3->clearFocus();
    QVERIFY(scope2->hasFocus());
    scope2->clearFocus();
    QVERIFY(scope1->hasFocus());
    scope2->hide();
    scope2->show();
    QVERIFY(!scope2->hasFocus());
    QVERIFY(scope1->hasFocus());
    scope2->setFocus();
    QVERIFY(scope2->hasFocus());
    scope3->setFocus();
    QVERIFY(scope3->hasFocus());

    // clearFocus() on a focus scope will remove focus from its children.
    scope1->clearFocus();
    QVERIFY(!scope1->hasFocus());
    QVERIFY(!scope2->hasFocus());
    QVERIFY(!scope3->hasFocus());

    scope1->setFocus();
    QVERIFY(!scope1->hasFocus());
    QVERIFY(!scope2->hasFocus());
    QVERIFY(scope3->hasFocus());

    scope2->clearFocus();
    QVERIFY(scope1->hasFocus());
    QVERIFY(!scope2->hasFocus());
    QVERIFY(!scope3->hasFocus());

    scope2->setFocus();
    QVERIFY(!scope1->hasFocus());
    QVERIFY(!scope2->hasFocus());
    QVERIFY(scope3->hasFocus());

    // Focus cleared while a parent doesn't have focus remains cleared
    // when the parent regains focus.
    scope1->clearFocus();
    scope3->clearFocus();
    QVERIFY(!scope1->hasFocus());
    QVERIFY(!scope2->hasFocus());
    QVERIFY(!scope3->hasFocus());

    scope1->setFocus();
    QVERIFY(!scope1->hasFocus());
    QVERIFY(scope2->hasFocus());
    QVERIFY(!scope3->hasFocus());

    scope3->setFocus();
    QVERIFY(!scope1->hasFocus());
    QVERIFY(!scope2->hasFocus());
    QVERIFY(scope3->hasFocus());

    QGraphicsRectItem *rect4 = new QGraphicsRectItem;
    rect4->setData(0, "rect4");
    rect4->setParentItem(scope3);

    QGraphicsRectItem *rect5 = new QGraphicsRectItem;
    rect5->setData(0, "rect5");
    rect5->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsFocusScope);
    rect5->setFocus();
    rect5->setParentItem(rect4);
    QCOMPARE(scope3->focusScopeItem(), (QGraphicsItem *)rect5);
    QVERIFY(rect5->hasFocus());

    rect4->setParentItem(0);
    QVERIFY(rect5->hasFocus());
    QCOMPARE(scope3->focusScopeItem(), (QGraphicsItem *)0);
    QCOMPARE(scope3->focusItem(), (QGraphicsItem *)0);
    QVERIFY(!scope3->hasFocus());

    QGraphicsRectItem *rectA = new QGraphicsRectItem;
    QGraphicsRectItem *scopeA = new QGraphicsRectItem(rectA);
    scopeA->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsFocusScope);
    scopeA->setFocus();
    QGraphicsRectItem *scopeB = new QGraphicsRectItem(scopeA);
    scopeB->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsFocusScope);
    scopeB->setFocus();

    scene.addItem(rectA);
    QVERIFY(rect5->hasFocus());
    QVERIFY(!scopeB->hasFocus());

    scopeA->setFocus();
    QVERIFY(scopeB->hasFocus());
    QCOMPARE(scopeB->focusItem(), (QGraphicsItem *)scopeB);
}

void tst_QGraphicsItem::focusScope2()
{
    QGraphicsRectItem *child1 = new QGraphicsRectItem;
    child1->setFlags(QGraphicsItem::ItemIsFocusable);
    child1->setFocus();
    QCOMPARE(child1->focusItem(), (QGraphicsItem *)child1);

    QGraphicsRectItem *child2 = new QGraphicsRectItem;
    child2->setFlags(QGraphicsItem::ItemIsFocusable);

    QGraphicsRectItem *rootFocusScope = new QGraphicsRectItem;
    rootFocusScope->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsFocusScope);
    rootFocusScope->setFocus();
    QCOMPARE(rootFocusScope->focusItem(), (QGraphicsItem *)rootFocusScope);

    child1->setParentItem(rootFocusScope);
    child2->setParentItem(rootFocusScope);

    QCOMPARE(rootFocusScope->focusScopeItem(), (QGraphicsItem *)child1);
    QCOMPARE(rootFocusScope->focusItem(), (QGraphicsItem *)child1);

    QGraphicsRectItem *siblingChild1 = new QGraphicsRectItem;
    siblingChild1->setFlags(QGraphicsItem::ItemIsFocusable);
    siblingChild1->setFocus();

    QGraphicsRectItem *siblingChild2 = new QGraphicsRectItem;
    siblingChild2->setFlags(QGraphicsItem::ItemIsFocusable);

    QGraphicsRectItem *siblingFocusScope = new QGraphicsRectItem;
    siblingFocusScope->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsFocusScope);

    siblingChild1->setParentItem(siblingFocusScope);
    siblingChild2->setParentItem(siblingFocusScope);

    QCOMPARE(siblingFocusScope->focusScopeItem(), (QGraphicsItem *)siblingChild1);
    QCOMPARE(siblingFocusScope->focusItem(), (QGraphicsItem *)0);

    QGraphicsItem *root = new QGraphicsRectItem;
    rootFocusScope->setParentItem(root);
    siblingFocusScope->setParentItem(root);

    QCOMPARE(root->focusItem(), (QGraphicsItem *)child1);

    QGraphicsScene scene;
    scene.addItem(root);

    QEvent activate(QEvent::WindowActivate);
    qApp->sendEvent(&scene, &activate);
    scene.setFocus();

    QCOMPARE(scene.focusItem(), (QGraphicsItem *)child1);

    // You cannot set focus on a descendant of a focus scope directly;
    // this will only change the scope's focus scope item pointer. If
    // you want to give true input focus, you must set it directly on
    // the scope itself
    siblingChild2->setFocus();
    QVERIFY(!siblingChild2->hasFocus());
    QVERIFY(!siblingChild2->focusItem());
    QCOMPARE(siblingFocusScope->focusScopeItem(), (QGraphicsItem *)siblingChild2);
    QCOMPARE(siblingFocusScope->focusItem(), (QGraphicsItem *)0);

    // Set focus on the scope; focus is forwarded to the focus scope item.
    siblingFocusScope->setFocus();
    QVERIFY(siblingChild2->hasFocus());
    QVERIFY(siblingChild2->focusItem());
    QCOMPARE(siblingFocusScope->focusScopeItem(), (QGraphicsItem *)siblingChild2);
    QCOMPARE(siblingFocusScope->focusItem(), (QGraphicsItem *)siblingChild2);
}

class FocusScopeItemPrivate;
class FocusScopeItem : public QGraphicsItem
{
    Q_DECLARE_PRIVATE(FocusScopeItem)
public:
    FocusScopeItem(QGraphicsItem *parent = 0);
    QRectF boundingRect() const { return QRectF(); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) { }

    int focusScopeChanged;
    FocusScopeItemPrivate *d_ptr;
};

class FocusScopeItemPrivate : QGraphicsItemPrivate
{
    Q_DECLARE_PUBLIC(FocusScopeItem)
public:
    void focusScopeItemChange(bool)
    { ++q_func()->focusScopeChanged; }
};

FocusScopeItem::FocusScopeItem(QGraphicsItem *parent)
    : QGraphicsItem(*new FocusScopeItemPrivate, parent), focusScopeChanged(0)
{
    setFlag(ItemIsFocusable);
}

void tst_QGraphicsItem::focusScopeItemChangedWhileScopeDoesntHaveFocus()
{
    QGraphicsRectItem rect;
    rect.setFlags(QGraphicsItem::ItemIsFocusScope | QGraphicsItem::ItemIsFocusable);

    FocusScopeItem *child1 = new FocusScopeItem(&rect);
    FocusScopeItem *child2 = new FocusScopeItem(&rect);

    QCOMPARE(rect.focusScopeItem(), (QGraphicsItem *)0);
    QCOMPARE(child1->focusScopeChanged, 0);
    QCOMPARE(child2->focusScopeChanged, 0);
    child1->setFocus();
    QCOMPARE(rect.focusScopeItem(), (QGraphicsItem *)child1);
    QCOMPARE(child1->focusScopeChanged, 1);
    QCOMPARE(child2->focusScopeChanged, 0);
    child2->setFocus();
    QCOMPARE(rect.focusScopeItem(), (QGraphicsItem *)child2);
    QCOMPARE(child1->focusScopeChanged, 2);
    QCOMPARE(child2->focusScopeChanged, 1);
    child1->setFocus();
    QCOMPARE(rect.focusScopeItem(), (QGraphicsItem *)child1);
    QCOMPARE(child1->focusScopeChanged, 3);
    QCOMPARE(child2->focusScopeChanged, 2);
    child1->clearFocus();
    QCOMPARE(rect.focusScopeItem(), (QGraphicsItem *)0);
    QCOMPARE(child1->focusScopeChanged, 4);
    QCOMPARE(child2->focusScopeChanged, 2);
}

void tst_QGraphicsItem::stackBefore()
{
    QGraphicsRectItem parent;
    QGraphicsRectItem *child1 = new QGraphicsRectItem(QRectF(0, 0, 5, 5), &parent);
    QGraphicsRectItem *child2 = new QGraphicsRectItem(QRectF(0, 0, 5, 5), &parent);
    QGraphicsRectItem *child3 = new QGraphicsRectItem(QRectF(0, 0, 5, 5), &parent);
    QGraphicsRectItem *child4 = new QGraphicsRectItem(QRectF(0, 0, 5, 5), &parent);
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child1 << child2 << child3 << child4));
    child1->setData(0, "child1");
    child2->setData(0, "child2");
    child3->setData(0, "child3");
    child4->setData(0, "child4");

    // Remove and append
    child2->setParentItem(0);
    child2->setParentItem(&parent);
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child1 << child3 << child4 << child2));

    // Move child2 before child1
    child2->stackBefore(child1); // 2134
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child2 << child1 << child3 << child4));
    child2->stackBefore(child2); // 2134
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child2 << child1 << child3 << child4));
    child1->setZValue(1); // 2341
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child2 << child3 << child4 << child1));
    child1->stackBefore(child2); // 2341
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child2 << child3 << child4 << child1));
    child1->setZValue(0); // 1234
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child1 << child2 << child3 << child4));
    child4->stackBefore(child1); // 4123
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child4 << child1 << child2 << child3));
    child4->setZValue(1); // 1234 (4123)
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child1 << child2 << child3 << child4));
    child3->stackBefore(child1); // 3124 (4312)
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child3 << child1 << child2 << child4));
    child4->setZValue(0); // 4312
    QCOMPARE(parent.childItems(), (QList<QGraphicsItem *>() << child4 << child3 << child1 << child2));

    // Make them all toplevels
    child1->setParentItem(0);
    child2->setParentItem(0);
    child3->setParentItem(0);
    child4->setParentItem(0);

    QGraphicsScene scene;
    scene.addItem(child1);
    scene.addItem(child2);
    scene.addItem(child3);
    scene.addItem(child4);
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder),
             (QList<QGraphicsItem *>() << child1 << child2 << child3 << child4));

    // Remove and append
    scene.removeItem(child2);
    scene.addItem(child2);
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child1 << child3 << child4 << child2));

    // Move child2 before child1
    child2->stackBefore(child1); // 2134
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child2 << child1 << child3 << child4));
    child2->stackBefore(child2); // 2134
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child2 << child1 << child3 << child4));
    child1->setZValue(1); // 2341
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child2 << child3 << child4 << child1));
    child1->stackBefore(child2); // 2341
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child2 << child3 << child4 << child1));
    child1->setZValue(0); // 1234
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child1 << child2 << child3 << child4));
    child4->stackBefore(child1); // 4123
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child4 << child1 << child2 << child3));
    child4->setZValue(1); // 1234 (4123)
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child1 << child2 << child3 << child4));
    child3->stackBefore(child1); // 3124 (4312)
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child3 << child1 << child2 << child4));
    child4->setZValue(0); // 4312
    QCOMPARE(scene.items(QPointF(2, 2), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder), (QList<QGraphicsItem *>() << child4 << child3 << child1 << child2));
}

void tst_QGraphicsItem::QTBUG_4233_updateCachedWithSceneRect()
{
    EventTester *tester = new EventTester;
    tester->setCacheMode(QGraphicsItem::ItemCoordinateCache);

    QGraphicsScene scene;
    scene.addItem(tester);
    scene.setSceneRect(-100, -100, 200, 200); // contains the tester item

    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget *)&view);

    QTRY_COMPARE(tester->repaints, 1);

    scene.update(); // triggers "updateAll" optimization
    qApp->processEvents();
    qApp->processEvents(); // in 4.6 only one processEvents is necessary

    QCOMPARE(tester->repaints, 1);

    scene.update(); // triggers "updateAll" optimization
    tester->update();
    qApp->processEvents();
    qApp->processEvents(); // in 4.6 only one processEvents is necessary

    QCOMPARE(tester->repaints, 2);
}

void tst_QGraphicsItem::sceneModality()
{
    // 1) Test mouse events (delivery/propagation/redirection)
    // 2) Test hover events (incl. leave on block, enter on unblock)
    // 3) Test cursor stuff (incl. unset on block, set on unblock)
    // 4) Test clickfocus
    // 5) Test grab/ungrab events (possibly ungrab on block, regrab on unblock)
    // 6) ### modality for non-panels is unsupported for now
    QGraphicsScene scene;

    QGraphicsRectItem *bottomItem = scene.addRect(-150, -100, 300, 200);
    bottomItem->setFlag(QGraphicsItem::ItemIsFocusable);
    bottomItem->setBrush(Qt::yellow);

    QGraphicsRectItem *leftParent = scene.addRect(-50, -50, 100, 100);
    leftParent->setFlag(QGraphicsItem::ItemIsPanel);
    leftParent->setBrush(Qt::blue);

    QGraphicsRectItem *leftChild = scene.addRect(-25, -25, 50, 50);
    leftChild->setFlag(QGraphicsItem::ItemIsPanel);
    leftChild->setBrush(Qt::green);
    leftChild->setParentItem(leftParent);

    QGraphicsRectItem *rightParent = scene.addRect(-50, -50, 100, 100);
    rightParent->setFlag(QGraphicsItem::ItemIsPanel);
    rightParent->setBrush(Qt::red);
    QGraphicsRectItem *rightChild = scene.addRect(-25, -25, 50, 50);
    rightChild->setFlag(QGraphicsItem::ItemIsPanel);
    rightChild->setBrush(Qt::gray);
    rightChild->setParentItem(rightParent);

    leftParent->setPos(-75, 0);
    rightParent->setPos(75, 0);

    bottomItem->setData(0, "bottomItem");
    leftParent->setData(0, "leftParent");
    leftChild->setData(0, "leftChild");
    rightParent->setData(0, "rightParent");
    rightChild->setData(0, "rightChild");

    scene.setSceneRect(scene.itemsBoundingRect().adjusted(-50, -50, 50, 50));

    EventSpy2 leftParentSpy(&scene, leftParent);
    EventSpy2 leftChildSpy(&scene, leftChild);
    EventSpy2 rightParentSpy(&scene, rightParent);
    EventSpy2 rightChildSpy(&scene, rightChild);
    EventSpy2 bottomItemSpy(&scene, bottomItem);

    // Scene modality, also test multiple scene modal items
    leftChild->setPanelModality(QGraphicsItem::SceneModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(bottomItemSpy.counts[QEvent::WindowBlocked], 0); // not a panel

    // Click inside left child
    sendMouseClick(&scene, leftChild->scenePos(), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 1);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMouseRelease], 0); // no grab
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightChildSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(bottomItemSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked

    // Click on left parent, event goes to modal child
    sendMouseClick(&scene, leftParent->sceneBoundingRect().topLeft() + QPointF(5, 5), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 2);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMouseRelease], 0); // no grab
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightChildSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(bottomItemSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked

    // Click on all other items and outside the items
    sendMouseClick(&scene, rightParent->sceneBoundingRect().topLeft() + QPointF(5, 5), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 3);
    sendMouseClick(&scene, rightChild->scenePos(), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 4);
    sendMouseClick(&scene, bottomItem->scenePos(), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 5);
    sendMouseClick(&scene, QPointF(10000, 10000), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 6);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMouseRelease], 0); // no grab
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightChildSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(bottomItemSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked

    leftChildSpy.counts.clear();
    rightChildSpy.counts.clear();
    leftParentSpy.counts.clear();
    rightParentSpy.counts.clear();
    bottomItemSpy.counts.clear();

    leftChild->setPanelModality(QGraphicsItem::NonModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowUnblocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowUnblocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowUnblocked], 1);
    QCOMPARE(bottomItemSpy.counts[QEvent::WindowUnblocked], 0);

    // Left parent enters scene modality.
    leftParent->setPanelModality(QGraphicsItem::SceneModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(bottomItemSpy.counts[QEvent::WindowBlocked], 0);

    // Click inside left child.
    sendMouseClick(&scene, leftChild->scenePos(), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 1);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMouseRelease], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // panel stops propagation
    QCOMPARE(rightParentSpy.counts[QEvent::GraphicsSceneMousePress], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(bottomItemSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked

   // Click on left parent.
    sendMouseClick(&scene, leftParent->sceneBoundingRect().topLeft() + QPointF(5, 5), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 1);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMouseRelease], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::GraphicsSceneMousePress], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::GraphicsSceneMousePress], 0);
    QCOMPARE(bottomItemSpy.counts[QEvent::GraphicsSceneMousePress], 0);

    // Click on all other items and outside the items
    sendMouseClick(&scene, rightParent->sceneBoundingRect().topLeft() + QPointF(5, 5), Qt::LeftButton);
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 2);
    sendMouseClick(&scene, rightChild->scenePos(), Qt::LeftButton);
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 3);
    sendMouseClick(&scene, bottomItem->scenePos(), Qt::LeftButton);
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 4);
    sendMouseClick(&scene, QPointF(10000, 10000), Qt::LeftButton);
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 5);
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMouseRelease], 0);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::GraphicsSceneMousePress], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::GraphicsSceneMousePress], 0);
    QCOMPARE(bottomItemSpy.counts[QEvent::GraphicsSceneMousePress], 0);

    leftChildSpy.counts.clear();
    rightChildSpy.counts.clear();
    leftParentSpy.counts.clear();
    rightParentSpy.counts.clear();
    bottomItemSpy.counts.clear();

    // Now both left parent and child are scene modal. Left parent is blocked.
    leftChild->setPanelModality(QGraphicsItem::SceneModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(bottomItemSpy.counts[QEvent::WindowBlocked], 0);

    // Click inside left child
    sendMouseClick(&scene, leftChild->scenePos(), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 1);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMouseRelease], 0); // no grab
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightChildSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(bottomItemSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked

    // Click on left parent, event goes to modal child
    sendMouseClick(&scene, leftParent->sceneBoundingRect().topLeft() + QPointF(5, 5), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 2);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMouseRelease], 0); // no grab
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightChildSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(bottomItemSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked

    // Click on all other items and outside the items
    sendMouseClick(&scene, rightParent->sceneBoundingRect().topLeft() + QPointF(5, 5), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 3);
    sendMouseClick(&scene, rightChild->scenePos(), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 4);
    sendMouseClick(&scene, bottomItem->scenePos(), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 5);
    sendMouseClick(&scene, QPointF(10000, 10000), Qt::LeftButton);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMousePress], 6);
    QCOMPARE(leftChildSpy.counts[QEvent::GraphicsSceneMouseRelease], 0); // no grab
    QCOMPARE(leftParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightParentSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(rightChildSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked
    QCOMPARE(bottomItemSpy.counts[QEvent::GraphicsSceneMousePress], 0); // blocked

    leftChildSpy.counts.clear();
    rightChildSpy.counts.clear();
    leftParentSpy.counts.clear();
    rightParentSpy.counts.clear();
    bottomItemSpy.counts.clear();

    // Right child enters scene modality, only left child is blocked.
    rightChild->setPanelModality(QGraphicsItem::SceneModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(bottomItemSpy.counts[QEvent::WindowBlocked], 0);
}

void tst_QGraphicsItem::panelModality()
{
    // 1) Test mouse events (delivery/propagation/redirection)
    // 2) Test hover events (incl. leave on block, enter on unblock)
    // 3) Test cursor stuff (incl. unset on block, set on unblock)
    // 4) Test clickfocus
    // 5) Test grab/ungrab events (possibly ungrab on block, regrab on unblock)
    // 6) ### modality for non-panels is unsupported for now
    QGraphicsScene scene;

    QGraphicsRectItem *bottomItem = scene.addRect(-150, -100, 300, 200);
    bottomItem->setFlag(QGraphicsItem::ItemIsFocusable);
    bottomItem->setBrush(Qt::yellow);

    QGraphicsRectItem *leftParent = scene.addRect(-50, -50, 100, 100);
    leftParent->setFlag(QGraphicsItem::ItemIsPanel);
    leftParent->setBrush(Qt::blue);

    QGraphicsRectItem *leftChild = scene.addRect(-25, -25, 50, 50);
    leftChild->setFlag(QGraphicsItem::ItemIsPanel);
    leftChild->setBrush(Qt::green);
    leftChild->setParentItem(leftParent);

    QGraphicsRectItem *rightParent = scene.addRect(-50, -50, 100, 100);
    rightParent->setFlag(QGraphicsItem::ItemIsPanel);
    rightParent->setBrush(Qt::red);
    QGraphicsRectItem *rightChild = scene.addRect(-25, -25, 50, 50);
    rightChild->setFlag(QGraphicsItem::ItemIsPanel);
    rightChild->setBrush(Qt::gray);
    rightChild->setParentItem(rightParent);

    leftParent->setPos(-75, 0);
    rightParent->setPos(75, 0);

    bottomItem->setData(0, "bottomItem");
    leftParent->setData(0, "leftParent");
    leftChild->setData(0, "leftChild");
    rightParent->setData(0, "rightParent");
    rightChild->setData(0, "rightChild");

    scene.setSceneRect(scene.itemsBoundingRect().adjusted(-50, -50, 50, 50));

    EventSpy2 leftParentSpy(&scene, leftParent);
    EventSpy2 leftChildSpy(&scene, leftChild);
    EventSpy2 rightParentSpy(&scene, rightParent);
    EventSpy2 rightChildSpy(&scene, rightChild);
    EventSpy2 bottomItemSpy(&scene, bottomItem);

    // Left Child enters panel modality, only left parent is blocked.
    leftChild->setPanelModality(QGraphicsItem::PanelModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(bottomItemSpy.counts[QEvent::WindowBlocked], 0);

    leftChild->setPanelModality(QGraphicsItem::NonModal);
    leftChildSpy.counts.clear();
    rightChildSpy.counts.clear();
    leftParentSpy.counts.clear();
    rightParentSpy.counts.clear();
    bottomItemSpy.counts.clear();

    // Left parent enter panel modality, nothing is blocked.
    leftParent->setPanelModality(QGraphicsItem::PanelModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(bottomItemSpy.counts[QEvent::WindowBlocked], 0);

    // Left child enters panel modality, left parent is blocked again.
    leftChild->setPanelModality(QGraphicsItem::PanelModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(bottomItemSpy.counts[QEvent::WindowBlocked], 0);

    leftChildSpy.counts.clear();
    rightChildSpy.counts.clear();
    leftParentSpy.counts.clear();
    rightParentSpy.counts.clear();
    bottomItemSpy.counts.clear();

    leftChild->setPanelModality(QGraphicsItem::NonModal);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowUnblocked], 1);
    leftParent->setPanelModality(QGraphicsItem::NonModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowUnblocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(bottomItemSpy.counts[QEvent::WindowUnblocked], 0);

    leftChildSpy.counts.clear();
    rightChildSpy.counts.clear();
    leftParentSpy.counts.clear();
    rightParentSpy.counts.clear();
    bottomItemSpy.counts.clear();

    // Left and right child enter panel modality, both parents are blocked.
    rightChild->setPanelModality(QGraphicsItem::PanelModal);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 1);
    leftChild->setPanelModality(QGraphicsItem::PanelModal);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 1);
}

void tst_QGraphicsItem::mixedModality()
{
    // 1) Test mouse events (delivery/propagation/redirection)
    // 2) Test hover events (incl. leave on block, enter on unblock)
    // 3) Test cursor stuff (incl. unset on block, set on unblock)
    // 4) Test clickfocus
    // 5) Test grab/ungrab events (possibly ungrab on block, regrab on unblock)
    // 6) ### modality for non-panels is unsupported for now
    QGraphicsScene scene;

    QGraphicsRectItem *bottomItem = scene.addRect(-150, -100, 300, 200);
    bottomItem->setFlag(QGraphicsItem::ItemIsFocusable);
    bottomItem->setBrush(Qt::yellow);

    QGraphicsRectItem *leftParent = scene.addRect(-50, -50, 100, 100);
    leftParent->setFlag(QGraphicsItem::ItemIsPanel);
    leftParent->setBrush(Qt::blue);

    QGraphicsRectItem *leftChild = scene.addRect(-25, -25, 50, 50);
    leftChild->setFlag(QGraphicsItem::ItemIsPanel);
    leftChild->setBrush(Qt::green);
    leftChild->setParentItem(leftParent);

    QGraphicsRectItem *rightParent = scene.addRect(-50, -50, 100, 100);
    rightParent->setFlag(QGraphicsItem::ItemIsPanel);
    rightParent->setBrush(Qt::red);
    QGraphicsRectItem *rightChild = scene.addRect(-25, -25, 50, 50);
    rightChild->setFlag(QGraphicsItem::ItemIsPanel);
    rightChild->setBrush(Qt::gray);
    rightChild->setParentItem(rightParent);

    leftParent->setPos(-75, 0);
    rightParent->setPos(75, 0);

    bottomItem->setData(0, "bottomItem");
    leftParent->setData(0, "leftParent");
    leftChild->setData(0, "leftChild");
    rightParent->setData(0, "rightParent");
    rightChild->setData(0, "rightChild");

    scene.setSceneRect(scene.itemsBoundingRect().adjusted(-50, -50, 50, 50));

    EventSpy2 leftParentSpy(&scene, leftParent);
    EventSpy2 leftChildSpy(&scene, leftChild);
    EventSpy2 rightParentSpy(&scene, rightParent);
    EventSpy2 rightChildSpy(&scene, rightChild);
    EventSpy2 bottomItemSpy(&scene, bottomItem);

    // Left Child enters panel modality, only left parent is blocked.
    leftChild->setPanelModality(QGraphicsItem::PanelModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 0);

    // Left parent enters scene modality, which blocks everything except the child.
    leftParent->setPanelModality(QGraphicsItem::SceneModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowUnblocked], 0);

    // Right child enters panel modality (changes nothing).
    rightChild->setPanelModality(QGraphicsItem::PanelModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowUnblocked], 0);

    // Left parent leaves modality. Right child is unblocked.
    leftParent->setPanelModality(QGraphicsItem::NonModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 0);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowUnblocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowUnblocked], 0);

    // Right child "upgrades" its modality to scene modal. Left child is blocked.
    // Right parent is unaffected.
    rightChild->setPanelModality(QGraphicsItem::SceneModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowUnblocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowUnblocked], 0);

    // "downgrade" right child back to panel modal, left child is unblocked
    rightChild->setPanelModality(QGraphicsItem::PanelModal);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(leftChildSpy.counts[QEvent::WindowUnblocked], 1);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightChildSpy.counts[QEvent::WindowUnblocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(leftParentSpy.counts[QEvent::WindowUnblocked], 0);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowBlocked], 1);
    QCOMPARE(rightParentSpy.counts[QEvent::WindowUnblocked], 0);
}

void tst_QGraphicsItem::modality_hover()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rect1 = scene.addRect(-50, -50, 100, 100);
    rect1->setFlag(QGraphicsItem::ItemIsPanel);
    rect1->setAcceptHoverEvents(true);
    rect1->setData(0, "rect1");

    QGraphicsRectItem *rect2 = scene.addRect(-50, -50, 100, 100);
    rect2->setParentItem(rect1);
    rect2->setFlag(QGraphicsItem::ItemIsPanel);
    rect2->setAcceptHoverEvents(true);
    rect2->setPos(50, 50);
    rect2->setPanelModality(QGraphicsItem::SceneModal);
    rect2->setData(0, "rect2");

    EventSpy2 rect1Spy(&scene, rect1);
    EventSpy2 rect2Spy(&scene, rect2);

    sendMouseMove(&scene, QPointF(-25, -25));

    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverEnter], 0);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverMove], 0);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverLeave], 0);

    sendMouseMove(&scene, QPointF(75, 75));

    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverEnter], 0);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverMove], 0);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverLeave], 0);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverEnter], 1);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverMove], 1);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverLeave], 0);

    sendMouseMove(&scene, QPointF(-25, -25));

    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverLeave], 1);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverEnter], 0);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverMove], 0);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverLeave], 0);

    rect2->setPanelModality(QGraphicsItem::NonModal);

    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverEnter], 1);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverMove], 1);

    sendMouseMove(&scene, QPointF(75, 75));

    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverEnter], 2);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverMove], 2);

    rect2->setPanelModality(QGraphicsItem::SceneModal);

    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverLeave], 1);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverEnter], 2);
    // changing modality causes a spurious GraphicsSceneHoveMove, even though the mouse didn't
    // actually move
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverMove], 3);

    sendMouseMove(&scene, QPointF(-25, -25));

    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverLeave], 2);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverEnter], 1);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverMove], 1);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverLeave], 1);

    rect2->setPanelModality(QGraphicsItem::PanelModal);

    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverEnter], 1);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverMove], 1);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverLeave], 1);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverEnter], 2);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverMove], 3);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverLeave], 2);

    rect2->setPanelModality(QGraphicsItem::NonModal);

    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverEnter], 2);
    QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneHoverMove], 2);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverEnter], 2);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverMove], 3);
    QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneHoverLeave], 2);
}

void tst_QGraphicsItem::modality_mouseGrabber()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rect1 = scene.addRect(-50, -50, 100, 100);
    rect1->setFlag(QGraphicsItem::ItemIsPanel);
    rect1->setFlag(QGraphicsItem::ItemIsMovable);
    rect1->setData(0, "rect1");

    QGraphicsRectItem *rect2 = scene.addRect(-50, -50, 100, 100);
    rect2->setParentItem(rect1);
    rect2->setFlag(QGraphicsItem::ItemIsPanel);
    rect2->setFlag(QGraphicsItem::ItemIsMovable);
    rect2->setPos(50, 50);
    rect2->setData(0, "rect2");

    EventSpy2 rect1Spy(&scene, rect1);
    EventSpy2 rect2Spy(&scene, rect2);

    {
        // pressing mouse on rect1 starts implicit grab
        sendMousePress(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMousePress], 1);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) rect1);

        // grab lost when rect1 is modally shadowed
        rect2->setPanelModality(QGraphicsItem::SceneModal);
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);

        // releasing goes nowhere
        sendMouseRelease(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMouseRelease], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneMouseRelease], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);

        // pressing mouse on rect1 starts implicit grab on rect2 (since it is modal)
        sendMouseClick(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMousePress], 1);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMouseRelease], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneMousePress], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneMouseRelease], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);

        rect2->setPanelModality(QGraphicsItem::NonModal);

        // pressing mouse on rect1 starts implicit grab
        sendMousePress(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMousePress], 2);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) rect1);

        // grab lost to rect2 when rect1 is modally shadowed
        rect2->setPanelModality(QGraphicsItem::SceneModal);
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 2);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);

        // rect1 does *not* re-grab when rect2 is no longer modal
        rect2->setPanelModality(QGraphicsItem::NonModal);
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 2);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);

        // release goes nowhere
        sendMouseRelease(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMouseRelease], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 2);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);
    }
    {
        // repeat the test using PanelModal
        rect2->setPanelModality(QGraphicsItem::NonModal);
        rect1Spy.counts.clear();
        rect2Spy.counts.clear();

        // pressing mouse on rect1 starts implicit grab
        sendMousePress(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMousePress], 1);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) rect1);

        // grab lost when rect1 is modally shadowed
        rect2->setPanelModality(QGraphicsItem::PanelModal);
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);

        // releasing goes nowhere
        sendMouseRelease(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMouseRelease], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneMouseRelease], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);

        // pressing mouse on rect1 starts implicit grab on rect2 (since it is modal)
        sendMouseClick(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMousePress], 1);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMouseRelease], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneMousePress], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GraphicsSceneMouseRelease], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);

        rect2->setPanelModality(QGraphicsItem::NonModal);

        // pressing mouse on rect1 starts implicit grab
        sendMousePress(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMousePress], 2);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) rect1);

        // grab lost to rect2 when rect1 is modally shadowed
        rect2->setPanelModality(QGraphicsItem::PanelModal);
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 2);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);

        // rect1 does *not* re-grab when rect2 is no longer modal
        rect2->setPanelModality(QGraphicsItem::NonModal);
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 2);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);

        // release goes nowhere
        sendMouseRelease(&scene, QPoint(-25, -25));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect1Spy.counts[QEvent::GraphicsSceneMouseRelease], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 2);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);
    }

    {
        // repeat the PanelModal tests, but this time the mouse events will be on a non-modal item,
        // meaning normal grabbing should work
        rect2->setPanelModality(QGraphicsItem::NonModal);
        rect1Spy.counts.clear();
        rect2Spy.counts.clear();

        QGraphicsRectItem *rect3 = scene.addRect(-50, -50, 100, 100);
        rect3->setFlag(QGraphicsItem::ItemIsPanel);
        rect3->setFlag(QGraphicsItem::ItemIsMovable);
        rect3->setPos(150, 50);
        rect3->setData(0, "rect3");

        EventSpy2 rect3Spy(&scene, rect3);

        // pressing mouse on rect3 starts implicit grab
        sendMousePress(&scene, QPoint(150, 50));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect3Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect3Spy.counts[QEvent::GraphicsSceneMousePress], 1);
        QCOMPARE(rect3Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) rect3);

        // grab is *not* lost when rect1 is modally shadowed by rect2
        rect2->setPanelModality(QGraphicsItem::PanelModal);
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect3Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect3Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) rect3);

        // releasing goes to rect3
        sendMouseRelease(&scene, QPoint(150, 50));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect3Spy.counts[QEvent::GrabMouse], 1);
        QCOMPARE(rect3Spy.counts[QEvent::GraphicsSceneMouseRelease], 1);
        QCOMPARE(rect3Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);

        rect2->setPanelModality(QGraphicsItem::NonModal);

        // pressing mouse on rect3 starts implicit grab
        sendMousePress(&scene, QPoint(150, 50));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect3Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect3Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) rect3);

        // grab is not lost
        rect2->setPanelModality(QGraphicsItem::PanelModal);
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect3Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect3Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) rect3);

        // grab stays on rect3
        rect2->setPanelModality(QGraphicsItem::NonModal);
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect3Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect3Spy.counts[QEvent::UngrabMouse], 1);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) rect3);

        // release goes to rect3
        sendMouseRelease(&scene, QPoint(150, 50));
        QCOMPARE(rect1Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect1Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::GrabMouse], 0);
        QCOMPARE(rect2Spy.counts[QEvent::UngrabMouse], 0);
        QCOMPARE(rect3Spy.counts[QEvent::GrabMouse], 2);
        QCOMPARE(rect3Spy.counts[QEvent::UngrabMouse], 2);
        QCOMPARE(scene.mouseGrabberItem(), (QGraphicsItem *) 0);
    }
}

void tst_QGraphicsItem::modality_clickFocus()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rect1 = scene.addRect(-50, -50, 100, 100);
    rect1->setFlag(QGraphicsItem::ItemIsPanel);
    rect1->setFlag(QGraphicsItem::ItemIsFocusable);
    rect1->setData(0, "rect1");

    QGraphicsRectItem *rect2 = scene.addRect(-50, -50, 100, 100);
    rect2->setParentItem(rect1);
    rect2->setFlag(QGraphicsItem::ItemIsPanel);
    rect2->setFlag(QGraphicsItem::ItemIsFocusable);
    rect2->setPos(50, 50);
    rect2->setData(0, "rect2");

    QEvent windowActivateEvent(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &windowActivateEvent);

    EventSpy2 rect1Spy(&scene, rect1);
    EventSpy2 rect2Spy(&scene, rect2);

    // activate rect1, it should get focus
    rect1->setActive(true);
    QCOMPARE(scene.focusItem(), (QGraphicsItem *) rect1);

    // focus stays when rect2 becomes modal
    rect2->setPanelModality(QGraphicsItem::SceneModal);
    QCOMPARE(scene.focusItem(), (QGraphicsItem *) rect1);
    QCOMPARE(rect1Spy.counts[QEvent::FocusIn], 1);
    QCOMPARE(rect1Spy.counts[QEvent::FocusOut], 0);
    QCOMPARE(rect2Spy.counts[QEvent::FocusIn], 0);
    QCOMPARE(rect2Spy.counts[QEvent::FocusOut], 0);

    // clicking on rect1 should not set it's focus item
    rect1->clearFocus();
    sendMouseClick(&scene, QPointF(-25, -25));
    QCOMPARE(rect1->focusItem(), (QGraphicsItem *) 0);
    QCOMPARE(rect1Spy.counts[QEvent::FocusIn], 1);
    QCOMPARE(rect1Spy.counts[QEvent::FocusOut], 1);
    QCOMPARE(rect2Spy.counts[QEvent::FocusIn], 0);
    QCOMPARE(rect2Spy.counts[QEvent::FocusOut], 0);

    // clicking on rect2 gives it focus
    rect2->setActive(true);
    sendMouseClick(&scene, QPointF(75, 75));
    QCOMPARE(scene.focusItem(), (QGraphicsItem *) rect2);
    QCOMPARE(rect1Spy.counts[QEvent::FocusIn], 1);
    QCOMPARE(rect1Spy.counts[QEvent::FocusOut], 1);
    QCOMPARE(rect2Spy.counts[QEvent::FocusIn], 1);
    QCOMPARE(rect2Spy.counts[QEvent::FocusOut], 0);

    // clicking on rect1 does *not* give it focus
    rect1->setActive(true);
    rect1->clearFocus();
    sendMouseClick(&scene, QPointF(-25, -25));
    QCOMPARE(scene.focusItem(), (QGraphicsItem *) 0);
    QCOMPARE(rect1Spy.counts[QEvent::FocusIn], 2);
    QCOMPARE(rect1Spy.counts[QEvent::FocusOut], 2);
    QCOMPARE(rect2Spy.counts[QEvent::FocusIn], 1);
    QCOMPARE(rect2Spy.counts[QEvent::FocusOut], 1);

    // focus doesn't change when leaving modality either
    rect2->setPanelModality(QGraphicsItem::NonModal);
    QCOMPARE(scene.focusItem(), (QGraphicsItem *) 0);
    QCOMPARE(rect1Spy.counts[QEvent::FocusIn], 2);
    QCOMPARE(rect1Spy.counts[QEvent::FocusOut], 2);
    QCOMPARE(rect2Spy.counts[QEvent::FocusIn], 1);
    QCOMPARE(rect2Spy.counts[QEvent::FocusOut], 1);

    // click on rect1, it should get focus now
    sendMouseClick(&scene, QPointF(-25, -25));
    QCOMPARE(scene.focusItem(), (QGraphicsItem *) rect1);
    QCOMPARE(rect1Spy.counts[QEvent::FocusIn], 3);
    QCOMPARE(rect1Spy.counts[QEvent::FocusOut], 2);
    QCOMPARE(rect2Spy.counts[QEvent::FocusIn], 1);
    QCOMPARE(rect2Spy.counts[QEvent::FocusOut], 1);
}

void tst_QGraphicsItem::modality_keyEvents()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rect1 = scene.addRect(-50, -50, 100, 100);
    rect1->setFlag(QGraphicsItem::ItemIsPanel);
    rect1->setFlag(QGraphicsItem::ItemIsFocusable);
    rect1->setData(0, "rect1");

    QGraphicsRectItem *rect1child = scene.addRect(-10, -10, 20, 20);
    rect1child->setParentItem(rect1);
    rect1child->setFlag(QGraphicsItem::ItemIsFocusable);
    rect1child->setData(0, "rect1child1");

    QGraphicsRectItem *rect2 = scene.addRect(-50, -50, 100, 100);
    rect2->setParentItem(rect1);
    rect2->setFlag(QGraphicsItem::ItemIsPanel);
    rect2->setFlag(QGraphicsItem::ItemIsFocusable);
    rect2->setPos(50, 50);
    rect2->setData(0, "rect2");

    QGraphicsRectItem *rect2child = scene.addRect(-10, -10, 20, 20);
    rect2child->setParentItem(rect2);
    rect2child->setFlag(QGraphicsItem::ItemIsFocusable);
    rect2child->setData(0, "rect2child1");

    QEvent windowActivateEvent(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &windowActivateEvent);

    EventSpy2 rect1Spy(&scene, rect1);
    EventSpy2 rect1childSpy(&scene, rect1child);
    EventSpy2 rect2Spy(&scene, rect2);
    EventSpy2 rect2childSpy(&scene, rect2child);

    // activate rect1 and give it rect1child focus
    rect1->setActive(true);
    rect1child->setFocus();
    QCOMPARE(scene.focusItem(), (QGraphicsItem *) rect1child);

    // focus stays on rect1child when rect2 becomes modal
    rect2->setPanelModality(QGraphicsItem::SceneModal);
    QCOMPARE(scene.focusItem(), (QGraphicsItem *) rect1child);

    // but key events to rect1child should be neither delivered nor propagated
    sendKeyClick(&scene, Qt::Key_A);
    sendKeyClick(&scene, Qt::Key_S);
    sendKeyClick(&scene, Qt::Key_D);
    sendKeyClick(&scene, Qt::Key_F);
    QCOMPARE(rect1childSpy.counts[QEvent::KeyPress], 0);
    QCOMPARE(rect1childSpy.counts[QEvent::KeyRelease], 0);
    QCOMPARE(rect1Spy.counts[QEvent::KeyPress], 0);
    QCOMPARE(rect1Spy.counts[QEvent::KeyRelease], 0);

    // change to panel modality, rect1child1 keeps focus
    rect2->setPanelModality(QGraphicsItem::PanelModal);
    QCOMPARE(scene.focusItem(), (QGraphicsItem *) rect1child);

    // still no key events
    sendKeyClick(&scene, Qt::Key_J);
    sendKeyClick(&scene, Qt::Key_K);
    sendKeyClick(&scene, Qt::Key_L);
    sendKeyClick(&scene, Qt::Key_Semicolon);
    QCOMPARE(rect1childSpy.counts[QEvent::KeyPress], 0);
    QCOMPARE(rect1childSpy.counts[QEvent::KeyRelease], 0);
    QCOMPARE(rect1Spy.counts[QEvent::KeyPress], 0);
    QCOMPARE(rect1Spy.counts[QEvent::KeyRelease], 0);
}

void tst_QGraphicsItem::itemIsInFront()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rect1 = new QGraphicsRectItem;
    rect1->setData(0, "rect1");
    scene.addItem(rect1);

    QGraphicsRectItem *rect1child1 = new QGraphicsRectItem(rect1);
    rect1child1->setZValue(1);
    rect1child1->setData(0, "rect1child1");

    QGraphicsRectItem *rect1child2 = new QGraphicsRectItem(rect1);
    rect1child2->setParentItem(rect1);
    rect1child2->setData(0, "rect1child2");

    QGraphicsRectItem *rect1child1_1 = new QGraphicsRectItem(rect1child1);
    rect1child1_1->setData(0, "rect1child1_1");

    QGraphicsRectItem *rect1child1_2 = new QGraphicsRectItem(rect1child1);
    rect1child1_2->setFlag(QGraphicsItem::ItemStacksBehindParent);
    rect1child1_2->setData(0, "rect1child1_2");

    QGraphicsRectItem *rect2 = new QGraphicsRectItem;
    rect2->setData(0, "rect2");
    scene.addItem(rect2);

    QGraphicsRectItem *rect2child1 = new QGraphicsRectItem(rect2);
    rect2child1->setData(0, "rect2child1");

    QCOMPARE(qt_closestItemFirst(rect1, rect1), false);
    QCOMPARE(qt_closestItemFirst(rect1, rect2), false);
    QCOMPARE(qt_closestItemFirst(rect1child1, rect2child1), false);
    QCOMPARE(qt_closestItemFirst(rect1child1, rect1child2), true);
    QCOMPARE(qt_closestItemFirst(rect1child1_1, rect1child2), true);
    QCOMPARE(qt_closestItemFirst(rect1child1_1, rect1child1), true);
    QCOMPARE(qt_closestItemFirst(rect1child1_2, rect1child2), true);
    QCOMPARE(qt_closestItemFirst(rect1child1_2, rect1child1), false);
    QCOMPARE(qt_closestItemFirst(rect1child1_2, rect1), true);
    QCOMPARE(qt_closestItemFirst(rect1child1_2, rect2), false);
    QCOMPARE(qt_closestItemFirst(rect1child1_2, rect2child1), false);
}

class ScenePosChangeTester : public ItemChangeTester
{
public:
    ScenePosChangeTester()
    { }
    ScenePosChangeTester(QGraphicsItem *parent) : ItemChangeTester(parent)
    { }
};

void tst_QGraphicsItem::scenePosChange()
{
    ScenePosChangeTester* root = new ScenePosChangeTester;
    ScenePosChangeTester* child1 = new ScenePosChangeTester(root);
    ScenePosChangeTester* grandChild1 = new ScenePosChangeTester(child1);
    ScenePosChangeTester* child2 = new ScenePosChangeTester(root);
    ScenePosChangeTester* grandChild2 = new ScenePosChangeTester(child2);

    child1->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    grandChild2->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

    QVERIFY(child1->flags() & QGraphicsItem::ItemSendsScenePositionChanges);
    QVERIFY(grandChild2->flags() & QGraphicsItem::ItemSendsScenePositionChanges);

    QGraphicsScene scene;
    scene.addItem(root);

    // ignore uninteresting changes
    child1->clear();
    child2->clear();
    grandChild1->clear();
    grandChild2->clear();

    // move whole tree
    root->moveBy(1.0, 1.0);
    QCOMPARE(child1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 1);
    QCOMPARE(grandChild1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);
    QCOMPARE(child2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);
    QCOMPARE(grandChild2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 1);

    // move subtree
    child2->moveBy(1.0, 1.0);
    QCOMPARE(child1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 1);
    QCOMPARE(grandChild1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);
    QCOMPARE(child2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);
    QCOMPARE(grandChild2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 2);

    // reparent
    grandChild2->setParentItem(child1);
    child1->moveBy(1.0, 1.0);
    QCOMPARE(child1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 2);
    QCOMPARE(grandChild1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);
    QCOMPARE(child2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);
    QCOMPARE(grandChild2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 3);

    // change flags
    grandChild1->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    grandChild2->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    QCoreApplication::processEvents(); // QGraphicsScenePrivate::_q_updateScenePosDescendants()
    child1->moveBy(1.0, 1.0);
    QCOMPARE(child1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 3);
    QCOMPARE(grandChild1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 1);
    QCOMPARE(child2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);
    QCOMPARE(grandChild2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 3);

    // remove
    scene.removeItem(grandChild1);
    delete grandChild2; grandChild2 = 0;
    QCoreApplication::processEvents(); // QGraphicsScenePrivate::_q_updateScenePosDescendants()
    root->moveBy(1.0, 1.0);
    QCOMPARE(child1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 4);
    QCOMPARE(grandChild1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 1);
    QCOMPARE(child2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);

    root->setX(1);
    QCOMPARE(child1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 5);
    QCOMPARE(grandChild1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 1);
    QCOMPARE(child2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);

    root->setY(1);
    QCOMPARE(child1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 6);
    QCOMPARE(grandChild1->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 1);
    QCOMPARE(child2->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 0);
}

void tst_QGraphicsItem::textItem_shortcuts()
{
    QWidget w;
    QVBoxLayout l;
    w.setLayout(&l);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    l.addWidget(&view);
    QPushButton b("Push Me");
    l.addWidget(&b);

    QGraphicsTextItem *item = scene.addText("Troll Text");
    item->setFlag(QGraphicsItem::ItemIsFocusable);
    item->setTextInteractionFlags(Qt::TextEditorInteraction);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    item->setFocus();
    QTRY_VERIFY(item->hasFocus());
    QVERIFY(item->textCursor().selectedText().isEmpty());

    // Shortcut should work (select all)
    QTest::keyClick(&view, Qt::Key_A, Qt::ControlModifier);
    QTRY_COMPARE(item->textCursor().selectedText(), item->toPlainText());
    QTextCursor tc = item->textCursor();
    tc.clearSelection();
    item->setTextCursor(tc);
    QVERIFY(item->textCursor().selectedText().isEmpty());

    // Shortcut should also work if the text item has the focus and another widget
    // has the same shortcut.
    b.setShortcut(QKeySequence("CTRL+A"));
    QTest::keyClick(&view, Qt::Key_A, Qt::ControlModifier);
    QTRY_COMPARE(item->textCursor().selectedText(), item->toPlainText());
}

void tst_QGraphicsItem::scroll()
{
    // Create two overlapping rectangles in the scene:
    // +-------+
    // |       | <- item1
    // |   +-------+
    // |   |       |
    // +---|       | <- item2
    //     |       |
    //     +-------+

    EventTester *item1 = new EventTester;
    item1->br = QRectF(0, 0, 200, 200);
    item1->brush = Qt::red;
    item1->setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);

    EventTester *item2 = new EventTester;
    item2->br = QRectF(0, 0, 200, 200);
    item2->brush = Qt::blue;
    item2->setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
    item2->setPos(100, 100);

    QGraphicsScene scene(0, 0, 300, 300);
    scene.addItem(item1);
    scene.addItem(item2);

    MyGraphicsView view(&scene);
    view.setFrameStyle(0);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.repaints > 0);

    view.reset();
    item1->reset();
    item2->reset();

    const QRectF item1BoundingRect = item1->boundingRect();
    const QRectF item2BoundingRect = item2->boundingRect();

    // Scroll item1:
    // Item1 should get full exposure
    // Item2 should get exposure for the part that overlaps item1.
    item1->scroll(0, -10);
    QTRY_VERIFY(view.repaints > 0);
    QCOMPARE(item1->lastExposedRect, item1BoundingRect);

    QRectF expectedItem2Expose = item2BoundingRect;
    // NB! Adjusted by 2 pixels for antialiasing
    expectedItem2Expose &= item1->mapRectToItem(item2, item1BoundingRect.adjusted(-2, -2, 2, 2));
    QCOMPARE(item2->lastExposedRect, expectedItem2Expose);

    // Enable ItemCoordinateCache on item1.
    view.reset();
    item1->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    QTRY_VERIFY(view.repaints > 0);
    view.reset();
    item1->reset();
    item2->reset();

    // Scroll item1:
    // Item1 should only get expose for the newly exposed area (accelerated scroll).
    // Item2 should get exposure for the part that overlaps item1.
    item1->scroll(0, -10, QRectF(50, 50, 100, 100));
    QTRY_VERIFY(view.repaints > 0);
    QCOMPARE(item1->lastExposedRect, QRectF(50, 140, 100, 10));

    expectedItem2Expose = item2BoundingRect;
    // NB! Adjusted by 2 pixels for antialiasing
    expectedItem2Expose &= item1->mapRectToItem(item2, QRectF(50, 50, 100, 100).adjusted(-2, -2, 2, 2));
    QCOMPARE(item2->lastExposedRect, expectedItem2Expose);
}

Q_DECLARE_METATYPE(QGraphicsItem::GraphicsItemFlag);

void tst_QGraphicsItem::focusHandling_data()
{
    QTest::addColumn<QGraphicsItem::GraphicsItemFlag>("focusFlag");
    QTest::addColumn<bool>("useStickyFocus");
    QTest::addColumn<int>("expectedFocusItem"); // 0: none, 1: focusableUnder, 2: itemWithFocus

    QTest::newRow("Focus goes through.")
        << static_cast<QGraphicsItem::GraphicsItemFlag>(0x0) << false << 1;

    QTest::newRow("Focus goes through, even with sticky scene.")
        << static_cast<QGraphicsItem::GraphicsItemFlag>(0x0) << true  << 1;

    QTest::newRow("With ItemStopsClickFocusPropagation, we cannot focus the item beneath the flagged one (but can still focus-out).")
        << QGraphicsItem::ItemStopsClickFocusPropagation << false << 0;

    QTest::newRow("With ItemStopsClickFocusPropagation, we cannot focus the item beneath the flagged one (and cannot focus-out if scene is sticky).")
        << QGraphicsItem::ItemStopsClickFocusPropagation << true << 2;

    QTest::newRow("With ItemStopsFocusHandling, focus cannot be changed by presses.")
        << QGraphicsItem::ItemStopsFocusHandling << false << 2;

    QTest::newRow("With ItemStopsFocusHandling, focus cannot be changed by presses (even if scene is sticky).")
        << QGraphicsItem::ItemStopsFocusHandling << true << 2;
}

void tst_QGraphicsItem::focusHandling()
{
    QFETCH(QGraphicsItem::GraphicsItemFlag, focusFlag);
    QFETCH(bool, useStickyFocus);
    QFETCH(int, expectedFocusItem);

    class MyItem : public QGraphicsRectItem
    {
    public:
        MyItem() : QGraphicsRectItem(0, 0, 100, 100) {}
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
        {
            painter->fillRect(boundingRect(), hasFocus() ? QBrush(Qt::red) : brush());
        }
    };

    QGraphicsRectItem *noFocusOnTop = new MyItem;
    noFocusOnTop->setFlag(QGraphicsItem::ItemIsFocusable, false);
    noFocusOnTop->setBrush(Qt::yellow);

    QGraphicsRectItem *focusableUnder = new MyItem;
    focusableUnder->setBrush(Qt::blue);
    focusableUnder->setFlag(QGraphicsItem::ItemIsFocusable);
    focusableUnder->setPos(50, 50);

    QGraphicsRectItem *itemWithFocus = new MyItem;
    itemWithFocus->setBrush(Qt::black);
    itemWithFocus->setFlag(QGraphicsItem::ItemIsFocusable);
    itemWithFocus->setPos(250, 10);

    QGraphicsScene scene(-50, -50, 400, 400);
    scene.addItem(noFocusOnTop);
    scene.addItem(focusableUnder);
    scene.addItem(itemWithFocus);
    scene.setStickyFocus(useStickyFocus);

    noFocusOnTop->setFlag(focusFlag);
    focusableUnder->stackBefore(noFocusOnTop);
    itemWithFocus->setFocus();

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QApplication::setActiveWindow(&view);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&view));
    QVERIFY(itemWithFocus->hasFocus());

    const QPointF mousePressPoint = noFocusOnTop->mapToScene(noFocusOnTop->boundingRect().center());
    const QList<QGraphicsItem *> itemsAtMousePressPosition = scene.items(mousePressPoint);
    QVERIFY(itemsAtMousePressPosition.contains(noFocusOnTop));

    sendMousePress(&scene, mousePressPoint);

    switch (expectedFocusItem) {
    case 0:
        QCOMPARE(scene.focusItem(), static_cast<QGraphicsRectItem *>(0));
        break;
    case 1:
        QCOMPARE(scene.focusItem(), focusableUnder);
        break;
    case 2:
        QCOMPARE(scene.focusItem(), itemWithFocus);
        break;
    }

    // Sanity check - manually setting the focus must work regardless of our
    // focus handling flags:
    focusableUnder->setFocus();
    QCOMPARE(scene.focusItem(), focusableUnder);
}

void tst_QGraphicsItem::touchEventPropagation_data()
{
    QTest::addColumn<QGraphicsItem::GraphicsItemFlag>("flag");
    QTest::addColumn<int>("expectedCount");

    QTest::newRow("ItemIsPanel")
        << QGraphicsItem::ItemIsPanel << 0;
    QTest::newRow("ItemStopsClickFocusPropagation")
        << QGraphicsItem::ItemStopsClickFocusPropagation << 1;
    QTest::newRow("ItemStopsFocusHandling")
        << QGraphicsItem::ItemStopsFocusHandling << 1;
}

void tst_QGraphicsItem::touchEventPropagation()
{
    QFETCH(QGraphicsItem::GraphicsItemFlag, flag);
    QFETCH(int, expectedCount);

    class Testee : public QGraphicsRectItem
    {
    public:
        int touchBeginEventCount;

        Testee()
            : QGraphicsRectItem(0, 0, 100, 100)
            , touchBeginEventCount(0)
        {
            setAcceptTouchEvents(true);
            setFlag(QGraphicsItem::ItemIsFocusable, false);
        }

        bool sceneEvent(QEvent *ev)
        {
            if (ev->type() == QEvent::TouchBegin)
                ++touchBeginEventCount;

            return QGraphicsRectItem::sceneEvent(ev);
        }
    };

    Testee *touchEventReceiver = new Testee;
    QGraphicsItem *topMost = new QGraphicsRectItem(touchEventReceiver->boundingRect());

    QGraphicsScene scene;
    scene.addItem(topMost);
    scene.addItem(touchEventReceiver);

    topMost->setAcceptTouchEvents(true);
    topMost->setZValue(FLT_MAX);
    topMost->setFlag(QGraphicsItem::ItemIsFocusable, false);
    topMost->setFlag(flag, true);

    QGraphicsView view(&scene);
    view.setSceneRect(touchEventReceiver->boundingRect());
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QCOMPARE(touchEventReceiver->touchBeginEventCount, 0);

    QTouchEvent::TouchPoint tp(0);
    tp.setState(Qt::TouchPointPressed);
    tp.setScenePos(view.sceneRect().center());
    tp.setLastScenePos(view.sceneRect().center());

    QList<QTouchEvent::TouchPoint> touchPoints;
    touchPoints << tp;

    sendMousePress(&scene, tp.scenePos());
    QTouchDevice *device = new QTouchDevice;
    device->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(device);
    QTouchEvent touchBegin(QEvent::TouchBegin, device, Qt::NoModifier, Qt::TouchPointPressed, touchPoints);

    qApp->sendEvent(&scene, &touchBegin);
    QCOMPARE(touchEventReceiver->touchBeginEventCount, expectedCount);
}

void tst_QGraphicsItem::deviceCoordinateCache_simpleRotations()
{
    // Make sure we don't invalidate the cache when applying simple
    // (90, 180, 270, 360) rotation transforms to the item.
    QGraphicsRectItem *item = new QGraphicsRectItem(0, 0, 300, 200);
    item->setBrush(Qt::red);
    item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 300, 200);
    scene.addItem(item);

    MyGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(view.repaints > 0);

    QGraphicsItemCache *itemCache = QGraphicsItemPrivate::get(item)->extraItemCache();
    QVERIFY(itemCache);
    QPixmapCache::Key currentKey = itemCache->deviceData.value(view.viewport()).key;

    // Trigger an update and verify that the cache is unchanged.
    QPixmapCache::Key oldKey = currentKey;
    view.reset();
    view.viewport()->update();
    QTRY_VERIFY(view.repaints > 0);
    currentKey = itemCache->deviceData.value(view.viewport()).key;
    QCOMPARE(currentKey, oldKey);

    // Check 90, 180, 270 and 360 degree rotations.
    for (int angle = 90; angle <= 360; angle += 90) {
        // Rotate item and verify that the cache was invalidated.
        oldKey = currentKey;
        view.reset();
        QTransform transform;
        transform.translate(150, 100);
        transform.rotate(angle);
        transform.translate(-150, -100);
        item->setTransform(transform);
        QTRY_VERIFY(view.repaints > 0);
        currentKey = itemCache->deviceData.value(view.viewport()).key;
        QVERIFY(currentKey != oldKey);

        // IMPORTANT PART:
        // Trigger an update and verify that the cache is unchanged.
        oldKey = currentKey;
        view.reset();
        view.viewport()->update();
        QTRY_VERIFY(view.repaints > 0);
        currentKey = itemCache->deviceData.value(view.viewport()).key;
        QCOMPARE(currentKey, oldKey);
    }

    // 45 degree rotation.
    oldKey = currentKey;
    view.reset();
    QTransform transform;
    transform.translate(150, 100);
    transform.rotate(45);
    transform.translate(-150, -100);
    item->setTransform(transform);
    QTRY_VERIFY(view.repaints > 0);
    currentKey = itemCache->deviceData.value(view.viewport()).key;
    QVERIFY(currentKey != oldKey);

    // Trigger an update and verify that the cache was invalidated.
    // We should always invalidate the cache for non-trivial transforms.
    oldKey = currentKey;
    view.reset();
    view.viewport()->update();
    QTRY_VERIFY(view.repaints > 0);
    currentKey = itemCache->deviceData.value(view.viewport()).key;
    QVERIFY(currentKey != oldKey);
}

void tst_QGraphicsItem::QTBUG_5418_textItemSetDefaultColor()
{
    struct Item : public QGraphicsTextItem
    {
        int painted;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *opt, QWidget *wid)
        {
            painted++;
            QGraphicsTextItem::paint(painter, opt, wid);
        }
    };

    Item *i = new Item;
    i->painted = 0;
    i->setPlainText("I AM A TROLL");

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    scene.addItem(i);
    QApplication::processEvents();
    QTRY_VERIFY(i->painted);
    QApplication::processEvents();

    i->painted = 0;
    QColor col(Qt::red);
    i->setDefaultTextColor(col);
    QApplication::processEvents();
    QTRY_COMPARE(i->painted, 1); //check that changing the color force an update

    i->painted = false;
    QImage image(400, 200, QImage::Format_RGB32);
    image.fill(0);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();
    QCOMPARE(i->painted, 1);

    int numRedPixel = 0;
    QRgb rgb = col.rgb();
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            // Because of antialiasing we allow a certain range of errors here.
            QRgb pixel = image.pixel(x, y);
            if (qAbs((int)(pixel & 0xff) - (int)(rgb & 0xff)) +
                qAbs((int)((pixel & 0xff00) >> 8) - (int)((rgb & 0xff00) >> 8)) +
                qAbs((int)((pixel & 0xff0000) >> 16) - (int)((rgb & 0xff0000) >> 16)) <= 50) {
                if (++numRedPixel >= 10) {
                    return;
                }
            }
        }
    }
    QCOMPARE(numRedPixel, -1); //color not found, FAIL!

    i->painted = 0;
    i->setDefaultTextColor(col);
    QApplication::processEvents();
    QCOMPARE(i->painted, 0); //same color as before should not trigger an update (QTBUG-6242)
}

void tst_QGraphicsItem::QTBUG_6738_missingUpdateWithSetParent()
{
    // In all 3 test cases below the reparented item should disappear
    EventTester *parent = new EventTester;
    EventTester *child = new EventTester(parent);
    EventTester *child2 = new EventTester(parent);
    EventTester *child3 = new EventTester(parent);
    EventTester *child4 = new EventTester(parent);

    child->setPos(10, 10);
    child2->setPos(20, 20);
    child3->setPos(30, 30);
    child4->setPos(40, 40);

    QGraphicsScene scene;
    scene.addItem(parent);

    MyGraphicsView view(&scene);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.repaints > 0);

    // test case #1
    view.reset();
    child2->setVisible(false);
    child2->setParentItem(child);

    QTRY_COMPARE(view.repaints, 1);

    // test case #2
    view.reset();
    child3->setOpacity(0.0);
    child3->setParentItem(child);

    QTRY_COMPARE(view.repaints, 1);

    // test case #3
    view.reset();
    child4->setParentItem(child);
    child4->setVisible(false);

    QTRY_COMPARE(view.repaints, 1);
}

void tst_QGraphicsItem::QT_2653_fullUpdateDiscardingOpacityUpdate()
{
    QGraphicsScene scene(0, 0, 200, 200);
    MyGraphicsView view(&scene);

    EventTester *parentGreen = new EventTester();
    parentGreen->setGeometry(QRectF(20, 20, 100, 100));
    parentGreen->brush = Qt::green;

    EventTester *childYellow = new EventTester(parentGreen);
    childYellow->setGeometry(QRectF(10, 10, 50, 50));
    childYellow->brush = Qt::yellow;

    scene.addItem(parentGreen);

    childYellow->setOpacity(0.0);
    parentGreen->setOpacity(0.0);

    // set any of the flags below to trigger a fullUpdate to reproduce the bug:
    // ItemIgnoresTransformations, ItemClipsChildrenToShape, ItemIsSelectable
    parentGreen->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    view.reset();

    parentGreen->setOpacity(1.0);

    QTRY_COMPARE(view.repaints, 1);

    view.reset();
    childYellow->repaints = 0;

    childYellow->setOpacity(1.0);

    QTRY_COMPARE(view.repaints, 1);
    QTRY_COMPARE(childYellow->repaints, 1);
}

void tst_QGraphicsItem::QTBUG_7714_fullUpdateDiscardingOpacityUpdate2()
{
    QGraphicsScene scene(0, 0, 200, 200);
    MyGraphicsView view(&scene);
    MyGraphicsView origView(&scene);

    EventTester *parentGreen = new EventTester();
    parentGreen->setGeometry(QRectF(20, 20, 100, 100));
    parentGreen->brush = Qt::green;

    EventTester *childYellow = new EventTester(parentGreen);
    childYellow->setGeometry(QRectF(10, 10, 50, 50));
    childYellow->brush = Qt::yellow;

    scene.addItem(parentGreen);

    origView.show();
    QVERIFY(QTest::qWaitForWindowActive(&origView));
    origView.setGeometry(origView.width() + 20, 20,
                         origView.width(), origView.height());

    parentGreen->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    origView.reset();
    childYellow->setOpacity(0.0);

    QTRY_COMPARE(origView.repaints, 1);

    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    view.reset();
    origView.reset();

    childYellow->setOpacity(1.0);

    QTRY_COMPARE(origView.repaints, 1);
    QTRY_COMPARE(view.repaints, 1);
}

void tst_QGraphicsItem::QT_2649_focusScope()
{
    QGraphicsScene *scene = new QGraphicsScene;

    QGraphicsRectItem *subFocusItem = new QGraphicsRectItem;
    subFocusItem->setFlags(QGraphicsItem::ItemIsFocusable);
    subFocusItem->setFocus();
    QCOMPARE(subFocusItem->focusItem(), (QGraphicsItem *)subFocusItem);

    QGraphicsRectItem *scope = new QGraphicsRectItem;
    scope->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsFocusScope);
    scope->setFocus();
    subFocusItem->setParentItem(scope);
    QCOMPARE(subFocusItem->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(subFocusItem->focusScopeItem(), (QGraphicsItem *)0);
    QCOMPARE(scope->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(scope->focusScopeItem(), (QGraphicsItem *)subFocusItem);

    QGraphicsRectItem *rootItem = new QGraphicsRectItem;
    rootItem->setFlags(QGraphicsItem::ItemIsFocusable);
    scope->setParentItem(rootItem);
    QCOMPARE(rootItem->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(rootItem->focusScopeItem(), (QGraphicsItem *)0);
    QCOMPARE(subFocusItem->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(subFocusItem->focusScopeItem(), (QGraphicsItem *)0);
    QCOMPARE(scope->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(scope->focusScopeItem(), (QGraphicsItem *)subFocusItem);

    scene->addItem(rootItem);

    QEvent windowActivate(QEvent::WindowActivate);
    qApp->sendEvent(scene, &windowActivate);
    scene->setFocus();

    QCOMPARE(rootItem->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(scope->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(subFocusItem->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(rootItem->focusScopeItem(), (QGraphicsItem *)0);
    QCOMPARE(scope->focusScopeItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(subFocusItem->focusScopeItem(), (QGraphicsItem *)0);
    QVERIFY(subFocusItem->hasFocus());

    scope->hide();

    QCOMPARE(rootItem->focusItem(), (QGraphicsItem *)0);
    QCOMPARE(scope->focusItem(), (QGraphicsItem *)0);
    QCOMPARE(subFocusItem->focusItem(), (QGraphicsItem *)0);
    QCOMPARE(rootItem->focusScopeItem(), (QGraphicsItem *)0);
    QCOMPARE(scope->focusScopeItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(subFocusItem->focusScopeItem(), (QGraphicsItem *)0);
    QVERIFY(!subFocusItem->hasFocus());

    scope->show();

    QCOMPARE(rootItem->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(scope->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(subFocusItem->focusItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(rootItem->focusScopeItem(), (QGraphicsItem *)0);
    QCOMPARE(scope->focusScopeItem(), (QGraphicsItem *)subFocusItem);
    QCOMPARE(subFocusItem->focusScopeItem(), (QGraphicsItem *)0);
    QVERIFY(subFocusItem->hasFocus());

    // This should not crash
    scope->hide();
    delete scene;
}

class MyGraphicsItemWithItemChange : public QGraphicsWidget
{
public:
    MyGraphicsItemWithItemChange(QGraphicsItem *parent = 0) : QGraphicsWidget(parent)
    {}

    QVariant itemChange(GraphicsItemChange change, const QVariant &value)
    {
        if (change == QGraphicsItem::ItemSceneHasChanged) {
            foreach (QGraphicsView *view, scene()->views()) {
                //We trigger a sort of unindexed items in the BSP
                view->sceneRect();
            }
        }
        return QGraphicsWidget::itemChange(change, value);
    }
};

void tst_QGraphicsItem::sortItemsWhileAdding()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsWidget grandGrandParent;
    grandGrandParent.resize(200, 200);
    scene.addItem(&grandGrandParent);
    QGraphicsWidget grandParent;
    grandParent.resize(200, 200);
    QGraphicsWidget parent(&grandParent);
    parent.resize(200, 200);
    MyGraphicsItemWithItemChange item(&parent);
    grandParent.setParentItem(&grandGrandParent);
}

void tst_QGraphicsItem::doNotMarkFullUpdateIfNotInScene()
{
    struct Item : public QGraphicsTextItem
    {
        int painted;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *opt, QWidget *wid)
        {
            painted++;
            QGraphicsTextItem::paint(painter, opt, wid);
        }
    };
    QGraphicsScene scene;
    MyGraphicsView view(&scene);
    Item *item = new Item;
    item->painted = 0;
    item->setPlainText("Grandparent");
    Item *item2 = new Item;
    item2->setPlainText("parent");
    item2->painted = 0;
    Item *item3 = new Item;
    item3->setPlainText("child");
    item3->painted = 0;
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect;
    effect->setOpacity(0.5);
    item2->setGraphicsEffect(effect);
    item3->setParentItem(item2);
    item2->setParentItem(item);
    scene.addItem(item);
    view.show();
    QTest::qWaitForWindowActive(view.windowHandle());
    view.activateWindow();
    QTRY_VERIFY(view.isActiveWindow());
    QTRY_VERIFY(view.repaints >= 1);
    int count = view.repaints;
    QTRY_COMPARE(item->painted, count);
    // cached as graphics effects, not painted multiple times
    QTRY_COMPARE(item2->painted, 1);
    QTRY_COMPARE(item3->painted, 1);
    item2->update();
    QApplication::processEvents();
    QTRY_COMPARE(item->painted, count + 1);
    QTRY_COMPARE(item2->painted, 2);
    QTRY_COMPARE(item3->painted, 2);
    item2->update();
    QApplication::processEvents();
    QTRY_COMPARE(item->painted, count + 2);
    QTRY_COMPARE(item2->painted, 3);
    QTRY_COMPARE(item3->painted, 3);
}

void tst_QGraphicsItem::itemDiesDuringDraggingOperation()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsRectItem *item = new QGraphicsRectItem(QRectF(0, 0, 100, 100));
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setAcceptDrops(true);
    scene.addItem(item);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget *)&view);
    QGraphicsSceneDragDropEvent dragEnter(QEvent::GraphicsSceneDragEnter);
    dragEnter.setScenePos(item->boundingRect().center());
    QApplication::sendEvent(&scene, &dragEnter);
    QGraphicsSceneDragDropEvent event(QEvent::GraphicsSceneDragMove);
    event.setScenePos(item->boundingRect().center());
    QApplication::sendEvent(&scene, &event);
    QCOMPARE(QGraphicsScenePrivate::get(&scene)->dragDropItem, item);
    delete item;
    QVERIFY(!QGraphicsScenePrivate::get(&scene)->dragDropItem);
}

void tst_QGraphicsItem::QTBUG_12112_focusItem()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsRectItem *item1 = new QGraphicsRectItem(0, 0, 20, 20);
    item1->setFlag(QGraphicsItem::ItemIsFocusable);
    QGraphicsRectItem *item2 = new QGraphicsRectItem(20, 20, 20, 20);
    item2->setFlag(QGraphicsItem::ItemIsFocusable);
    item1->setFocus();
    scene.addItem(item2);
    scene.addItem(item1);

    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget *)&view);

    QVERIFY(item1->focusItem());
    QVERIFY(!item2->focusItem());

    item2->setFocus();
    QVERIFY(!item1->focusItem());
    QVERIFY(item2->focusItem());
}

void tst_QGraphicsItem::QTBUG_13473_sceneposchange()
{
    ScenePosChangeTester* parent = new ScenePosChangeTester;
    ScenePosChangeTester* child = new ScenePosChangeTester(parent);

    // parent's disabled ItemSendsGeometryChanges flag must not affect
    // child's scene pos change notifications
    parent->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
    child->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);

    QGraphicsScene scene;
    scene.addItem(parent);

    // ignore uninteresting changes
    parent->clear();
    child->clear();

    // move
    parent->moveBy(1.0, 1.0);
    QCOMPARE(child->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 1);

    // transform
    parent->setTransform(QTransform::fromScale(0.5, 0.5));
    QCOMPARE(child->changes.count(QGraphicsItem::ItemScenePositionHasChanged), 2);
}

class MyGraphicsWidget : public QGraphicsWidget {
Q_OBJECT
public:
    MyGraphicsWidget()
        : QGraphicsWidget(0)
    {
        QGraphicsLinearLayout *lay = new QGraphicsLinearLayout(Qt::Vertical);
        QLatin1String wiseWords("AZ BUKI VEDI");
        QString sentence(wiseWords);
        QStringList words = sentence.split(QLatin1Char(' '), QString::SkipEmptyParts);
        for (int i = 0; i < words.count(); ++i) {
            QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(this);
            QLabel *label = new QLabel(words.at(i));
            proxy->setWidget(label);
            proxy->setFocusPolicy(Qt::StrongFocus);
            proxy->setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);
            if (i%2 == 0)
                proxy->setVisible(false);
            proxy->setFocus();
            lay->addItem(proxy);
        }
        setLayout(lay);
    }

};

class MyWidgetWindow : public QGraphicsWidget
{
public:
    MyWidgetWindow()
        : QGraphicsWidget(0, Qt::Window)
    {
        QGraphicsLinearLayout *lay = new QGraphicsLinearLayout(Qt::Vertical);
        MyGraphicsWidget *widget = new MyGraphicsWidget();
        lay->addItem(widget);
        setLayout(lay);
    }
};

void tst_QGraphicsItem::QTBUG_16374_crashInDestructor()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    MyWidgetWindow win;
    scene.addItem(&win);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

void tst_QGraphicsItem::QTBUG_20699_focusScopeCrash()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QGraphicsPixmapItem fs;
    fs.setFlags(QGraphicsItem::ItemIsFocusScope | QGraphicsItem::ItemIsFocusable);
    scene.addItem(&fs);
    QGraphicsPixmapItem* fs2 = new QGraphicsPixmapItem(&fs);
    fs2->setFlags(QGraphicsItem::ItemIsFocusScope | QGraphicsItem::ItemIsFocusable);
    QGraphicsPixmapItem* fi2 = new QGraphicsPixmapItem(&fs);
    fi2->setFlags(QGraphicsItem::ItemIsFocusable);
    QGraphicsPixmapItem* fi = new QGraphicsPixmapItem(fs2);
    fi->setFlags(QGraphicsItem::ItemIsFocusable);
    fs.setFocus();
    fi->setFocus();

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    fi->setParentItem(fi2);
    fi->setFocus();
    fs.setFocus();
    fi->setParentItem(fs2);
    fi->setFocus();
    fs2->setFocus();
    fs.setFocus();
    fi->setParentItem(fi2);
    fi->setFocus();
    fs.setFocus();
}

void tst_QGraphicsItem::QTBUG_30990_rightClickSelection()
{
    QGraphicsScene scene;
    QGraphicsItem *item1 = scene.addRect(10, 10, 10, 10);
    item1->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    QGraphicsItem *item2 = scene.addRect(100, 100, 10, 10);
    item2->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);

    // right mouse press & release over an item should not make it selected
    sendMousePress(&scene, item1->boundingRect().center(), Qt::RightButton);
    QVERIFY(!item1->isSelected());
    sendMouseRelease(&scene, item1->boundingRect().center(), Qt::RightButton);
    QVERIFY(!item1->isSelected());

    // right mouse press over one item, moving over another item,
    // and then releasing should make neither of the items selected
    sendMousePress(&scene, item1->boundingRect().center(), Qt::RightButton);
    QVERIFY(!item1->isSelected());
    QVERIFY(!item2->isSelected());
    sendMouseMove(&scene, item2->boundingRect().center(), Qt::RightButton);
    QVERIFY(!item1->isSelected());
    QVERIFY(!item2->isSelected());
    sendMouseRelease(&scene, item2->boundingRect().center(), Qt::RightButton);
    QVERIFY(!item1->isSelected());
    QVERIFY(!item2->isSelected());
}

void tst_QGraphicsItem::QTBUG_21618_untransformable_sceneTransform()
{
    QGraphicsScene scene(0, 0, 150, 150);
    scene.addRect(-2, -2, 4, 4);

    QGraphicsItem *item1 = scene.addRect(0, 0, 100, 100, QPen(), Qt::red);
    item1->setPos(50, 50);
    item1->translate(50, 50);
    item1->rotate(90);
    QGraphicsItem *item2 = scene.addRect(0, 0, 100, 100, QPen(), Qt::green);
    item2->setPos(50, 50);
    item2->translate(50, 50);
    item2->rotate(90);
    item2->setFlags(QGraphicsItem::ItemIgnoresTransformations);

    QGraphicsRectItem *item1_topleft = new QGraphicsRectItem(QRectF(-2, -2, 4, 4));
    item1_topleft->setParentItem(item1);
    item1_topleft->setBrush(Qt::black);
    QGraphicsRectItem *item1_bottomright = new QGraphicsRectItem(QRectF(-2, -2, 4, 4));
    item1_bottomright->setParentItem(item1);
    item1_bottomright->setPos(100, 100);
    item1_bottomright->setBrush(Qt::yellow);

    QGraphicsRectItem *item2_topleft = new QGraphicsRectItem(QRectF(-2, -2, 4, 4));
    item2_topleft->setParentItem(item2);
    item2_topleft->setBrush(Qt::black);
    QGraphicsRectItem *item2_bottomright = new QGraphicsRectItem(QRectF(-2, -2, 4, 4));
    item2_bottomright->setParentItem(item2);
    item2_bottomright->setPos(100, 100);
    item2_bottomright->setBrush(Qt::yellow);

    QCOMPARE(item1->sceneTransform(), item2->sceneTransform());
    QCOMPARE(item1_topleft->sceneTransform(), item2_topleft->sceneTransform());
    QCOMPARE(item1_bottomright->sceneTransform(), item2_bottomright->sceneTransform());
    QCOMPARE(item1->deviceTransform(QTransform()), item2->deviceTransform(QTransform()));
    QCOMPARE(item1->deviceTransform(QTransform()).map(QPointF()), QPointF(100, 100));
    QCOMPARE(item2->deviceTransform(QTransform()).map(QPointF()), QPointF(100, 100));
    QCOMPARE(item1->deviceTransform(QTransform()).map(QPointF(100, 100)), QPointF(0, 200));
    QCOMPARE(item2->deviceTransform(QTransform()).map(QPointF(100, 100)), QPointF(0, 200));
    QCOMPARE(item1_topleft->deviceTransform(QTransform()).map(QPointF()), QPointF(100, 100));
    QCOMPARE(item2_topleft->deviceTransform(QTransform()).map(QPointF()), QPointF(100, 100));
    QCOMPARE(item1_bottomright->deviceTransform(QTransform()).map(QPointF()), QPointF(0, 200));
    QCOMPARE(item2_bottomright->deviceTransform(QTransform()).map(QPointF()), QPointF(0, 200));

    item2->setParentItem(item1);

    QCOMPARE(item2->deviceTransform(QTransform()).map(QPointF()), QPointF(100, 200));
    QCOMPARE(item2->deviceTransform(QTransform()).map(QPointF(100, 100)), QPointF(0, 300));
    QCOMPARE(item2_topleft->deviceTransform(QTransform()).map(QPointF()), QPointF(100, 200));
    QCOMPARE(item2_bottomright->deviceTransform(QTransform()).map(QPointF()), QPointF(0, 300));

    QTransform tx = QTransform::fromTranslate(100, 0);
    QCOMPARE(item1->deviceTransform(tx).map(QPointF()), QPointF(200, 100));
    QCOMPARE(item1->deviceTransform(tx).map(QPointF(100, 100)), QPointF(100, 200));
    QCOMPARE(item2->deviceTransform(tx).map(QPointF()), QPointF(200, 200));
    QCOMPARE(item2->deviceTransform(tx).map(QPointF(100, 100)), QPointF(100, 300));
    QCOMPARE(item2_topleft->deviceTransform(tx).map(QPointF()), QPointF(200, 200));
    QCOMPARE(item2_bottomright->deviceTransform(tx).map(QPointF()), QPointF(100, 300));
}

void tst_QGraphicsItem::resolvePaletteForItemChildren()
{
    QGraphicsScene scene;
    QGraphicsRectItem item(0, 0, 50, -150);
    scene.addItem(&item);
    QGraphicsWidget widget;
    widget.setParentItem(&item);

    QColor green(Qt::green);
    QPalette paletteForScene = scene.palette();
    paletteForScene.setColor(QPalette::Active, QPalette::Window, green);
    scene.setPalette(paletteForScene);

    QCOMPARE(widget.palette().color(QPalette::Active, QPalette::Window), green);
}

QTEST_MAIN(tst_QGraphicsItem)
#include "tst_qgraphicsitem.moc"
