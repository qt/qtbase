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

#include <qgraphicsitem.h>
#include <qgraphicsscene.h>
#include <qgraphicssceneevent.h>
#include <qgraphicsview.h>
#include <qgraphicswidget.h>
#include <qgraphicsproxywidget.h>

#include <math.h>

#include <QtWidgets/QLabel>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QCommonStyle>
#include <QtGui/QPainterPath>
#include <QtWidgets/QRubberBand>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QStyle>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QDesktopWidget>
#ifndef QT_NO_OPENGL
#include <QtWidgets/QOpenGLWidget>
#endif
#include <private/qgraphicsscene_p.h>
#include <private/qgraphicsview_p.h>
#include "../../../shared/platforminputcontext.h"
#include <private/qinputmethod_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include "tst_qgraphicsview.h"

#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;

Q_DECLARE_METATYPE(ExpectedValueDescription)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<QRectF>)
Q_DECLARE_METATYPE(QMatrix)
Q_DECLARE_METATYPE(QPainterPath)
Q_DECLARE_METATYPE(Qt::ScrollBarPolicy)
Q_DECLARE_METATYPE(ScrollBarCount)

#ifdef Q_OS_MAC
//On mac we get full update. So check that the expected region is contained inside the actual
#define COMPARE_REGIONS(ACTUAL, EXPECTED) QVERIFY((EXPECTED).subtracted(ACTUAL).isEmpty())
#else
#define COMPARE_REGIONS QCOMPARE
#endif

static void sendMousePress(QWidget *widget, const QPoint &point, Qt::MouseButton button = Qt::LeftButton)
{
    QMouseEvent event(QEvent::MouseButtonPress, point, widget->mapToGlobal(point), button, 0, 0);
    QApplication::sendEvent(widget, &event);
}

static void sendMouseMove(QWidget *widget, const QPoint &point, Qt::MouseButton button = Qt::NoButton, Qt::MouseButtons buttons = 0)
{
    QTest::mouseMove(widget, point);
    QMouseEvent event(QEvent::MouseMove, point, button, buttons, 0);
    QApplication::sendEvent(widget, &event);
    QApplication::processEvents();
}

static void sendMouseRelease(QWidget *widget, const QPoint &point, Qt::MouseButton button = Qt::LeftButton)
{
    QMouseEvent event(QEvent::MouseButtonRelease, point, widget->mapToGlobal(point), button, 0, 0);
    QApplication::sendEvent(widget, &event);
}

class EventSpy : public QObject
{
    Q_OBJECT
public:
    EventSpy(QObject *watched, QEvent::Type type)
        : _count(0), spied(type)
    {
        watched->installEventFilter(this);
    }

    int count() const { return _count; }
    void reset() { _count = 0; }

protected:
    bool eventFilter(QObject *watched, QEvent *event)
    {
        Q_UNUSED(watched);
        if (event->type() == spied)
            ++_count;
        return false;
    }

    int _count;
    QEvent::Type spied;
};

#if defined QT_BUILD_INTERNAL
class FriendlyGraphicsScene : public QGraphicsScene
{
    friend class tst_QGraphicsView;
    Q_DECLARE_PRIVATE(QGraphicsScene);
};
#endif

class tst_QGraphicsView : public QObject
{
    Q_OBJECT

public:
    tst_QGraphicsView()
        : platformName(QGuiApplication::platformName().toLower())
    { }
private slots:
    void cleanup();
    void construction();
    void renderHints();
    void alignment();
    void interactive();
    void scene();
    void setScene();
    void deleteScene();
    void sceneRect();
    void sceneRect_growing();
    void setSceneRect();
    void viewport();
#ifndef QT_NO_OPENGL
    void openGLViewport();
#endif
    void dragMode_scrollHand();
    void dragMode_rubberBand();
    void rubberBandSelectionMode();
    void rubberBandExtendSelection();
    void rotated_rubberBand();
    void backgroundBrush();
    void foregroundBrush();
    void matrix();
    void matrix_convenience();
    void matrix_combine();
    void centerOnPoint();
    void centerOnItem();
    void ensureVisibleRect();
    void fitInView();
    void itemsAtPoint();
#if defined QT_BUILD_INTERNAL
    void itemsAtPosition_data();
    void itemsAtPosition();
#endif
    void itemsInRect();
    void itemsInRect_cosmeticAdjust_data();
    void itemsInRect_cosmeticAdjust();
    void itemsInPoly();
    void itemsInPath();
    void itemAt();
    void itemAt2();
    void mapToScene();
    void mapToScenePoint();
    void mapToSceneRect_data();
    void mapToSceneRect();
    void mapToScenePoly();
    void mapToScenePath();
    void mapFromScenePoint();
    void mapFromSceneRect();
    void mapFromScenePoly();
    void mapFromScenePath();
    void sendEvent();
#if QT_CONFIG(wheelevent)
    void wheelEvent();
#endif
#ifndef QT_NO_CURSOR
    void cursor();
    void cursor2();
#endif
    void transformationAnchor();
    void resizeAnchor();
    void viewportUpdateMode();
    void viewportUpdateMode2();
#if QT_CONFIG(draganddrop)
    void acceptDrops();
#endif
    void optimizationFlags();
    void optimizationFlags_dontSavePainterState();
    void optimizationFlags_dontSavePainterState2_data();
    void optimizationFlags_dontSavePainterState2();
    void levelOfDetail_data();
    void levelOfDetail();
    void scrollBarRanges_data();
    void scrollBarRanges();
    void acceptMousePressEvent();
    void acceptMouseDoubleClickEvent();
    void forwardMousePress();
    void forwardMouseDoubleClick();
    void replayMouseMove();
    void itemsUnderMouse();
    void embeddedViews();
    void scrollAfterResize_data();
    void scrollAfterResize();
    void moveItemWhileScrolling_data();
    void moveItemWhileScrolling();
    void centerOnDirtyItem();
    void mouseTracking();
    void mouseTracking2();
    void mouseTracking3();
    void render();
    void exposeRegion();
    void update_data();
    void update();
    void update2_data();
    void update2();
    void update_ancestorClipsChildrenToShape();
    void update_ancestorClipsChildrenToShape2();
    void inputMethodSensitivity();
    void inputContextReset();
    void indirectPainting();
    void compositionModeInDrawBackground();

    // task specific tests below me
    void task172231_untransformableItems();
    void task180429_mouseReleaseDragMode();
    void task187791_setSceneCausesUpdate();
    void task186827_deleteReplayedItem();
    void task207546_focusCrash();
    void task210599_unsetDragWhileDragging();
    void task239729_noViewUpdate_data();
    void task239729_noViewUpdate();
    void task239047_fitInViewSmallViewport();
    void task245469_itemsAtPointWithClip();
    void task253415_reconnectUpdateSceneOnSceneChanged();
    void task255529_transformationAnchorMouseAndViewportMargins();
    void task259503_scrollingArtifacts();
    void QTBUG_4151_clipAndIgnore_data();
    void QTBUG_4151_clipAndIgnore();
    void QTBUG_5859_exposedRect();
#ifndef QT_NO_CURSOR
    void QTBUG_7438_cursor();
#endif
    void hoverLeave();
    void QTBUG_16063_microFocusRect();

public slots:
    void dummySlot() {}

private:
    QString platformName;
};

void tst_QGraphicsView::cleanup()
{
    // ensure not even skipped tests with custom input context leave it dangling
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = 0;
}

void tst_QGraphicsView::construction()
{
    QGraphicsView view;
    QCOMPARE(view.renderHints(), QPainter::TextAntialiasing);
    QCOMPARE(view.dragMode(), QGraphicsView::NoDrag);
    QVERIFY(view.isInteractive());
    QVERIFY(!view.scene());
    QCOMPARE(view.sceneRect(), QRectF());
    QVERIFY(view.viewport());
    QCOMPARE(view.viewport()->metaObject()->className(), "QWidget");
    QCOMPARE(view.matrix(), QMatrix());
    QVERIFY(view.items().isEmpty());
    QVERIFY(view.items(QPoint()).isEmpty());
    QVERIFY(view.items(QRect()).isEmpty());
    QVERIFY(view.items(QPolygon()).isEmpty());
    QVERIFY(view.items(QPainterPath()).isEmpty());
    QVERIFY(!view.itemAt(QPoint()));
    QCOMPARE(view.mapToScene(QPoint()), QPointF());
    QCOMPARE(view.mapToScene(QRect()), QPolygonF());
    QCOMPARE(view.mapToScene(QPolygon()), QPolygonF());
    QCOMPARE(view.mapFromScene(QPointF()), QPoint());
    QPolygon poly;
    poly << QPoint() << QPoint() << QPoint() << QPoint();
    QCOMPARE(view.mapFromScene(QRectF()), poly);
    QCOMPARE(view.mapFromScene(QPolygonF()), QPolygon());
    QCOMPARE(view.transformationAnchor(), QGraphicsView::AnchorViewCenter);
    QCOMPARE(view.resizeAnchor(), QGraphicsView::NoAnchor);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

class TestItem : public QGraphicsItem
{
public:
    QRectF boundingRect() const
    { return QRectF(-10, -10, 20, 20); }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
    { hints = painter->renderHints(); painter->drawRect(boundingRect()); }

    bool sceneEvent(QEvent *event)
    {
        events << event->type();
        return QGraphicsItem::sceneEvent(event);
    }

    QList<QEvent::Type> events;
    QPainter::RenderHints hints;
};

void tst_QGraphicsView::renderHints()
{
    QGraphicsView view;
    QCOMPARE(view.renderHints(), QPainter::TextAntialiasing);
    view.setRenderHint(QPainter::TextAntialiasing, false);
    QCOMPARE(view.renderHints(), 0);
    view.setRenderHint(QPainter::Antialiasing, false);
    QCOMPARE(view.renderHints(), 0);
    view.setRenderHint(QPainter::TextAntialiasing, true);
    QCOMPARE(view.renderHints(), QPainter::TextAntialiasing);
    view.setRenderHint(QPainter::Antialiasing);
    QCOMPARE(view.renderHints(), QPainter::TextAntialiasing | QPainter::Antialiasing);
    view.setRenderHints(0);
    QCOMPARE(view.renderHints(), 0);

    TestItem *item = new TestItem;
    QGraphicsScene scene;
    scene.addItem(item);

    view.setScene(&scene);

    view.setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing | QPainter::NonCosmeticDefaultPen);
    QCOMPARE(view.renderHints(), QPainter::TextAntialiasing | QPainter::Antialiasing | QPainter::NonCosmeticDefaultPen);

    QCOMPARE(item->hints, 0);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.repaint();
    QTRY_COMPARE(item->hints, view.renderHints());

    view.setRenderHints(QPainter::Antialiasing | QPainter::NonCosmeticDefaultPen);
    QCOMPARE(view.renderHints(), QPainter::Antialiasing | QPainter::NonCosmeticDefaultPen);

    view.repaint();
    QTRY_COMPARE(item->hints, view.renderHints());
}

void tst_QGraphicsView::alignment()
{
    QGraphicsScene scene;
    scene.addRect(QRectF(-10, -10, 20, 20));

    QGraphicsView view(&scene);
    setFrameless(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            Qt::Alignment alignment = 0;
            switch (i) {
            case 0:
                alignment |= Qt::AlignLeft;
                break;
            case 1:
                alignment |= Qt::AlignHCenter;
                break;
            case 2:
            default:
                alignment |= Qt::AlignRight;
                break;
            }
            switch (j) {
            case 0:
                alignment |= Qt::AlignTop;
                break;
            case 1:
                alignment |= Qt::AlignVCenter;
                break;
            case 2:
            default:
                alignment |= Qt::AlignBottom;
                break;
            }
            view.setAlignment(alignment);
            QCOMPARE(view.alignment(), alignment);

            for (int k = 0; k < 3; ++k) {
                view.resize(100 + k * 25, 100 + k * 25);
                QApplication::processEvents();
            }
        }
    }
}

void tst_QGraphicsView::interactive()
{
    TestItem *item = new TestItem;
    item->setFlags(QGraphicsItem::ItemIsMovable);
    QCOMPARE(item->events.size(), 0);

    QGraphicsScene scene(-200, -200, 400, 400);
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.setFixedSize(300, 300);
    QCOMPARE(item->events.size(), 0);
    view.show();
    view.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QTRY_COMPARE(item->events.size(), 1); // activate

    QPoint itemPoint = view.mapFromScene(item->scenePos());

    QVERIFY(view.itemAt(itemPoint));

    for (int i = 0; i < 100; ++i) {
        sendMousePress(view.viewport(), itemPoint);
        QCOMPARE(item->events.size(), i * 5 + 3);
        QCOMPARE(item->events.at(item->events.size() - 2), QEvent::GrabMouse);
        QCOMPARE(item->events.at(item->events.size() - 1), QEvent::GraphicsSceneMousePress);
        sendMouseRelease(view.viewport(), itemPoint);
        QCOMPARE(item->events.size(), i * 5 + 5);
        QCOMPARE(item->events.at(item->events.size() - 2), QEvent::GraphicsSceneMouseRelease);
        QCOMPARE(item->events.at(item->events.size() - 1), QEvent::UngrabMouse);
#ifndef QT_NO_CONTEXTMENU
        QContextMenuEvent contextEvent(QContextMenuEvent::Mouse, itemPoint, view.mapToGlobal(itemPoint));
        QApplication::sendEvent(view.viewport(), &contextEvent);
        QCOMPARE(item->events.size(), i * 5 + 6);
        QCOMPARE(item->events.last(), QEvent::GraphicsSceneContextMenu);
#endif // QT_NO_CONTEXTMENU
    }

    view.setInteractive(false);

    for (int i = 0; i < 100; ++i) {
        sendMousePress(view.viewport(), itemPoint);
        QCOMPARE(item->events.size(), 501);
        QCOMPARE(item->events.last(), QEvent::GraphicsSceneContextMenu);
        sendMouseRelease(view.viewport(), itemPoint);
        QCOMPARE(item->events.size(), 501);
        QCOMPARE(item->events.last(), QEvent::GraphicsSceneContextMenu);
#ifndef QT_NO_CONTEXTMENU
        QContextMenuEvent contextEvent(QContextMenuEvent::Mouse, itemPoint, view.mapToGlobal(itemPoint));
        QApplication::sendEvent(view.viewport(), &contextEvent);
        QCOMPARE(item->events.size(), 501);
        QCOMPARE(item->events.last(), QEvent::GraphicsSceneContextMenu);
#endif // QT_NO_CONTEXTMENU
    }
}

void tst_QGraphicsView::scene()
{
    QGraphicsView view;
    QVERIFY(!view.scene());
    view.setScene(0);
    QVERIFY(!view.scene());

    {
        QGraphicsScene scene;
        view.setScene(&scene);
        QCOMPARE(view.scene(), &scene);
    }

    QCOMPARE(view.scene(), nullptr);
}

void tst_QGraphicsView::setScene()
{
    QGraphicsScene scene(-1000, -1000, 2000, 2000);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QCOMPARE(view.sceneRect(), scene.sceneRect());

    QVERIFY(view.horizontalScrollBar()->isVisible());
    QVERIFY(view.verticalScrollBar()->isVisible());
    QVERIFY(!view.horizontalScrollBar()->isHidden());
    QVERIFY(!view.verticalScrollBar()->isHidden());

    view.setScene(0);

    QTRY_VERIFY(!view.horizontalScrollBar()->isVisible());
    QTRY_VERIFY(!view.verticalScrollBar()->isVisible());
    QVERIFY(!view.horizontalScrollBar()->isHidden());
    QVERIFY(!view.verticalScrollBar()->isHidden());

    QCOMPARE(view.sceneRect(), QRectF());
}

void tst_QGraphicsView::deleteScene()
{
    QGraphicsScene *scene = new QGraphicsScene;
    QGraphicsView view1(scene);
    view1.show();
    QGraphicsView view2(scene);
    view2.show();
    QGraphicsView view3(scene);
    view3.show();
    delete scene;
    QCOMPARE(view1.scene(), nullptr);
    QCOMPARE(view2.scene(), nullptr);
    QCOMPARE(view3.scene(), nullptr);
}

void tst_QGraphicsView::sceneRect()
{
    QGraphicsView view;
    QCOMPARE(view.sceneRect(), QRectF());

    view.setSceneRect(QRectF(-100, -100, 200, 200));
    QCOMPARE(view.sceneRect(), QRectF(-100, -100, 200, 200));
    view.setSceneRect(-100, -100, 200, 200);
    QCOMPARE(view.sceneRect(), QRectF(-100, -100, 200, 200));

    view.setSceneRect(QRectF());
    QCOMPARE(view.sceneRect(), QRectF());
    QGraphicsScene scene;
    QGraphicsRectItem *item = scene.addRect(QRectF(-100, -100, 100, 100));
    item->setPen(QPen(Qt::black, 0));

    view.setScene(&scene);

    QCOMPARE(view.sceneRect(), QRectF(-100, -100, 100, 100));
    item->moveBy(-100, -100);
    QCOMPARE(view.sceneRect(), QRectF(-200, -200, 200, 200));
    item->moveBy(100, 100);
    QCOMPARE(view.sceneRect(), QRectF(-200, -200, 200, 200));

    view.setScene(0);
    view.setSceneRect(QRectF());
    QCOMPARE(view.sceneRect(), QRectF());
}

void tst_QGraphicsView::sceneRect_growing()
{
    QWidget toplevel;

    QGraphicsScene scene;
    for (int i = 0; i < 100; ++i)
        scene.addText(QLatin1String("(0, ") + QString::number((i - 50) * 20))->setPos(0, (i - 50) * 20);

    QGraphicsView view(&scene, &toplevel);
    view.setFixedSize(200, 200);
    toplevel.show();
    QVERIFY(QTest::qWaitForWindowActive(&toplevel));

    int size = 200;
    scene.setSceneRect(-size, -size, size * 2, size * 2);
    QCOMPARE(view.sceneRect(), scene.sceneRect());

    QPointF topLeft = view.mapToScene(0, 0);

    for (int i = 0; i < 5; ++i) {
        size *= 2;
        scene.setSceneRect(-size, -size, size * 2, size * 2);

        QApplication::processEvents();

        QCOMPARE(view.sceneRect(), scene.sceneRect());
        QCOMPARE(view.mapToScene(0, 0), topLeft);
        view.setSceneRect(-size, -size, size * 2, size * 2);
        QCOMPARE(view.mapToScene(0, 0), topLeft);
        view.setSceneRect(QRectF());
    }
}

void tst_QGraphicsView::setSceneRect()
{
    QRectF rect1(-100, -100, 200, 200);
    QRectF rect2(-300, -300, 150, 150);

    QGraphicsScene scene;
    QGraphicsView view(&scene);

    scene.setSceneRect(rect1);
    QCOMPARE(scene.sceneRect(), rect1);
    QCOMPARE(view.sceneRect(), rect1);

    scene.setSceneRect(rect2);
    QCOMPARE(scene.sceneRect(), rect2);
    QCOMPARE(view.sceneRect(), rect2);

    view.setSceneRect(rect1);
    QCOMPARE(scene.sceneRect(), rect2);
    QCOMPARE(view.sceneRect(), rect1);

    view.setSceneRect(rect2);
    QCOMPARE(scene.sceneRect(), rect2);
    QCOMPARE(view.sceneRect(), rect2);

    scene.setSceneRect(rect1);
    QCOMPARE(scene.sceneRect(), rect1);
    QCOMPARE(view.sceneRect(), rect2);

    // extreme transformations will max out the scrollbars' ranges.
    view.setSceneRect(-2000000, -2000000, 4000000, 4000000);
    view.scale(9000, 9000);
    QCOMPARE(view.horizontalScrollBar()->minimum(), INT_MIN);
    QCOMPARE(view.horizontalScrollBar()->maximum(), INT_MAX);
    QCOMPARE(view.verticalScrollBar()->minimum(), INT_MIN);
    QCOMPARE(view.verticalScrollBar()->maximum(), INT_MAX);
}

void tst_QGraphicsView::viewport()
{
    QGraphicsScene scene;
    scene.addText("GraphicsView");

    QGraphicsView view(&scene);
    QVERIFY(view.viewport() != 0);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QPointer<QWidget> widget = new QWidget;
    view.setViewport(widget);
    QCOMPARE(view.viewport(), (QWidget *)widget);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    view.setViewport(0);
    QVERIFY(widget.isNull());
    QVERIFY(view.viewport() != 0);
    QVERIFY(view.viewport() != widget);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

#ifndef QT_NO_OPENGL
void tst_QGraphicsView::openGLViewport()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
        QSKIP("QOpenGL is not supported on this platform.");

    QGraphicsScene scene;
    scene.setBackgroundBrush(Qt::white);
    scene.addText("GraphicsView");
    scene.addEllipse(QRectF(400, 50, 50, 50));
    scene.addEllipse(QRectF(-100, -400, 50, 50));
    scene.addEllipse(QRectF(50, -100, 50, 50));
    scene.addEllipse(QRectF(-100, 50, 50, 50));

    QGraphicsView view(&scene);
    view.setSceneRect(-400, -400, 800, 800);
    view.resize(400, 400);

    QOpenGLWidget *glw = new QOpenGLWidget;
    QSignalSpy spy1(glw, SIGNAL(resized()));
    QSignalSpy spy2(glw, SIGNAL(frameSwapped()));

    view.setViewport(glw);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(spy1.count() > 0);
    QTRY_VERIFY(spy2.count() >= spy1.count());
    spy1.clear();
    spy2.clear();

    // Now test for resize (QTBUG-52419). This is special when the viewport is
    // a QOpenGLWidget since the underlying FBO must also be maintained.
    view.resize(300, 300);
    QTRY_VERIFY(spy1.count() > 0);
    QTRY_VERIFY(spy2.count() >= spy1.count());
    // There is no sane way to check if the framebuffer contents got updated
    // (grabFramebuffer is no good for the viewport case as that does not go
    // through paintGL). So skip the actual verification.
}
#endif

void tst_QGraphicsView::dragMode_scrollHand()
{
    for (int j = 0; j < 2; ++j) {
        QGraphicsView view;
        setFrameless(&view);
        QCOMPARE(view.dragMode(), QGraphicsView::NoDrag);

        view.setSceneRect(-1000, -1000, 2000, 2000);
        view.setFixedSize(100, 100);
        view.show();

        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QApplication::processEvents();

        view.setInteractive(j ? false : true);

        QGraphicsScene scene;
        scene.addRect(QRectF(-100, -100, 5, 5));
        scene.addRect(QRectF(95, -100, 5, 5));
        scene.addRect(QRectF(95, 95, 5, 5));
        QGraphicsItem *item = scene.addRect(QRectF(-100, 95, 5, 5));
        item->setFlag(QGraphicsItem::ItemIsSelectable);
        item->setSelected(true);
        QVERIFY(item->isSelected());
        QVERIFY(!view.scene());

        view.setDragMode(QGraphicsView::ScrollHandDrag);

        for (int i = 0; i < 2; ++i) {
            // ScrollHandDrag
#ifndef QT_NO_CURSOR
            Qt::CursorShape cursorShape = view.viewport()->cursor().shape();
#endif
            int horizontalScrollBarValue = view.horizontalScrollBar()->value();
            int verticalScrollBarValue = view.verticalScrollBar()->value();
            {
                // Press
                QMouseEvent event(QEvent::MouseButtonPress,
                                  view.viewport()->rect().center(),
                                  Qt::LeftButton, Qt::LeftButton, 0);
                event.setAccepted(true);
                QApplication::sendEvent(view.viewport(), &event);
                QVERIFY(event.isAccepted());
            }
            QApplication::processEvents();

            QTRY_VERIFY(item->isSelected());

            for (int k = 0; k < 4; ++k) {
#ifndef QT_NO_CURSOR
                QCOMPARE(view.viewport()->cursor().shape(), Qt::ClosedHandCursor);
#endif
                {
                    // Move
                    QMouseEvent event(QEvent::MouseMove,
                                      view.viewport()->rect().center() + QPoint(10, 0),
                                      Qt::LeftButton, Qt::LeftButton, 0);
                    event.setAccepted(true);
                    QApplication::sendEvent(view.viewport(), &event);
                    QVERIFY(event.isAccepted());
                }
                QVERIFY(item->isSelected());
                QCOMPARE(view.horizontalScrollBar()->value(), horizontalScrollBarValue - 10);
                QCOMPARE(view.verticalScrollBar()->value(), verticalScrollBarValue);
                {
                    // Move
                    QMouseEvent event(QEvent::MouseMove,
                                      view.viewport()->rect().center() + QPoint(10, 10),
                                      Qt::LeftButton, Qt::LeftButton, 0);
                    event.setAccepted(true);
                    QApplication::sendEvent(view.viewport(), &event);
                    QVERIFY(event.isAccepted());
                }
                QVERIFY(item->isSelected());
                QCOMPARE(view.horizontalScrollBar()->value(), horizontalScrollBarValue - 10);
                QCOMPARE(view.verticalScrollBar()->value(), verticalScrollBarValue - 10);
            }

            {
                // Release
                QMouseEvent event(QEvent::MouseButtonRelease,
                                  view.viewport()->rect().center() + QPoint(10, 10),
                                  Qt::LeftButton, Qt::LeftButton, 0);
                event.setAccepted(true);
                QApplication::sendEvent(view.viewport(), &event);
                QVERIFY(event.isAccepted());
            }
            QApplication::processEvents();

            QTRY_VERIFY(item->isSelected());
            QCOMPARE(view.horizontalScrollBar()->value(), horizontalScrollBarValue - 10);
            QCOMPARE(view.verticalScrollBar()->value(), verticalScrollBarValue - 10);
#ifndef QT_NO_CURSOR
            QCOMPARE(view.viewport()->cursor().shape(), cursorShape);
#endif

            // Check that items are not unselected because of a scroll hand drag.
            QVERIFY(item->isSelected());

            // Check that a click will still unselect the item.
            {
                // Press
                QMouseEvent event(QEvent::MouseButtonPress,
                                  view.viewport()->rect().center() + QPoint(10, 10),
                                  Qt::LeftButton, Qt::LeftButton, 0);
                QApplication::sendEvent(view.viewport(), &event);
            }
            {
                // Release
                QMouseEvent event(QEvent::MouseButtonRelease,
                                  view.viewport()->rect().center() + QPoint(10, 10),
                                  Qt::LeftButton, Qt::LeftButton, 0);
                QApplication::sendEvent(view.viewport(), &event);
            }

            if (view.isInteractive()) {
                if (view.scene()) {
                    QVERIFY(!item->isSelected());
                    item->setSelected(true);
                } else {
                    QVERIFY(item->isSelected());
                }
            } else {
                QVERIFY(item->isSelected());
            }

            view.setScene(&scene);
        }
    }
}

void tst_QGraphicsView::dragMode_rubberBand()
{
    QGraphicsView view;
    QCOMPARE(view.dragMode(), QGraphicsView::NoDrag);

    view.setSceneRect(-1000, -1000, 2000, 2000);
    view.show();

    QGraphicsScene scene;
    scene.addRect(QRectF(-100, -100, 25, 25))->setFlag(QGraphicsItem::ItemIsSelectable);
    scene.addRect(QRectF(75, -100, 25, 25))->setFlag(QGraphicsItem::ItemIsSelectable);
    scene.addRect(QRectF(75, 75, 25, 25))->setFlag(QGraphicsItem::ItemIsSelectable);
    scene.addRect(QRectF(-100, 75, 25, 25))->setFlag(QGraphicsItem::ItemIsSelectable);

    view.setDragMode(QGraphicsView::RubberBandDrag);

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QApplication::processEvents();

    for (int i = 0; i < 2; ++i) {
        // RubberBandDrag
#ifndef QT_NO_CURSOR
        Qt::CursorShape cursorShape = view.viewport()->cursor().shape();
#endif
        int horizontalScrollBarValue = view.horizontalScrollBar()->value();
        int verticalScrollBarValue = view.verticalScrollBar()->value();
        {
            // Press
            QMouseEvent event(QEvent::MouseButtonPress,
                              view.viewport()->rect().center(),
                              Qt::LeftButton, Qt::LeftButton, 0);
            event.setAccepted(true);
            QApplication::sendEvent(view.viewport(), &event);
            QVERIFY(event.isAccepted());
        }
#ifndef QT_NO_CURSOR
        QCOMPARE(view.viewport()->cursor().shape(), cursorShape);
#endif

        QApplication::processEvents();

        {
            // Move
            QMouseEvent event(QEvent::MouseMove,
                              view.viewport()->rect().center() + QPoint(100, 0),
                              Qt::LeftButton, Qt::LeftButton, 0);
            event.setAccepted(true);
            QApplication::sendEvent(view.viewport(), &event);
            QVERIFY(event.isAccepted());
        }
        QCOMPARE(view.horizontalScrollBar()->value(), horizontalScrollBarValue);
        QCOMPARE(view.verticalScrollBar()->value(), verticalScrollBarValue);

        // We don't use QRubberBand as of 4.3; the band is drawn internally.
        QVERIFY(!view.findChild<QRubberBand *>());

        {
            // Move
            QMouseEvent event(QEvent::MouseMove,
                              view.viewport()->rect().center() + QPoint(100, 100),
                              Qt::LeftButton, Qt::LeftButton, 0);
            event.setAccepted(true);
            QApplication::sendEvent(view.viewport(), &event);
            QVERIFY(event.isAccepted());
        }
        QCOMPARE(view.horizontalScrollBar()->value(), horizontalScrollBarValue);
        QCOMPARE(view.verticalScrollBar()->value(), verticalScrollBarValue);

        {
            // Release
            QMouseEvent event(QEvent::MouseButtonRelease,
                              view.viewport()->rect().center() + QPoint(100, 100),
                              Qt::LeftButton, Qt::LeftButton, 0);
            event.setAccepted(true);
            QApplication::sendEvent(view.viewport(), &event);
            QVERIFY(event.isAccepted());
        }
        QCOMPARE(view.horizontalScrollBar()->value(), horizontalScrollBarValue);
        QCOMPARE(view.verticalScrollBar()->value(), verticalScrollBarValue);
#ifndef QT_NO_CURSOR
        QCOMPARE(view.viewport()->cursor().shape(), cursorShape);
#endif

        if (view.scene())
            QCOMPARE(scene.selectedItems().size(), 1);

        view.setScene(&scene);
        view.centerOn(0, 0);
    }
}

void tst_QGraphicsView::rubberBandSelectionMode()
{
    QWidget toplevel;
    setFrameless(&toplevel);

    QGraphicsScene scene;
    QGraphicsRectItem *rect = scene.addRect(QRectF(10, 10, 80, 80));
    rect->setFlag(QGraphicsItem::ItemIsSelectable);

    QGraphicsView view(&scene, &toplevel);
    QCOMPARE(view.rubberBandSelectionMode(), Qt::IntersectsItemShape);
    view.setDragMode(QGraphicsView::RubberBandDrag);
    view.resize(120, 120);
    toplevel.show();

    // Disable mouse tracking to prevent the window system from sending mouse
    // move events to the viewport while we are synthesizing events. If
    // QGraphicsView gets a mouse move event with no buttons down, it'll
    // terminate the rubber band.
    view.viewport()->setMouseTracking(false);

    QCOMPARE(scene.selectedItems(), QList<QGraphicsItem *>());
    sendMousePress(view.viewport(), QPoint(), Qt::LeftButton);
    sendMouseMove(view.viewport(), view.viewport()->rect().center(),
                  Qt::LeftButton, Qt::LeftButton);
    QCOMPARE(scene.selectedItems(), QList<QGraphicsItem *>() << rect);
    sendMouseRelease(view.viewport(), QPoint(), Qt::LeftButton);

    view.setRubberBandSelectionMode(Qt::ContainsItemShape);
    QCOMPARE(view.rubberBandSelectionMode(), Qt::ContainsItemShape);
    sendMousePress(view.viewport(), QPoint(), Qt::LeftButton);
    QCOMPARE(scene.selectedItems(), QList<QGraphicsItem *>());
    sendMouseMove(view.viewport(), view.viewport()->rect().center(),
                  Qt::LeftButton, Qt::LeftButton);
    QCOMPARE(scene.selectedItems(), QList<QGraphicsItem *>());
    sendMouseMove(view.viewport(), view.viewport()->rect().bottomRight(),
                  Qt::LeftButton, Qt::LeftButton);
    QCOMPARE(scene.selectedItems(), QList<QGraphicsItem *>() << rect);
}

void tst_QGraphicsView::rubberBandExtendSelection()
{
   QWidget toplevel;
   setFrameless(&toplevel);

   QGraphicsScene scene(0, 0, 1000, 1000);

   QGraphicsView view(&scene, &toplevel);
   view.setDragMode(QGraphicsView::RubberBandDrag);
   toplevel.show();

   // Disable mouse tracking to prevent the window system from sending mouse
   // move events to the viewport while we are synthesizing events. If
   // QGraphicsView gets a mouse move event with no buttons down, it'll
   // terminate the rubber band.
   view.viewport()->setMouseTracking(false);

   QGraphicsItem *item1 = scene.addRect(10, 10, 100, 100);
   QGraphicsItem *item2 = scene.addRect(10, 120, 100, 100);
   QGraphicsItem *item3 = scene.addRect(10, 230, 100, 100);

   item1->setFlag(QGraphicsItem::ItemIsSelectable);
   item2->setFlag(QGraphicsItem::ItemIsSelectable);
   item3->setFlag(QGraphicsItem::ItemIsSelectable);

   // select first item
   item1->setSelected(true);
   QCOMPARE(scene.selectedItems(), QList<QGraphicsItem *>() << item1);

   // first rubberband without modifier key
   sendMousePress(view.viewport(), view.mapFromScene(20, 115), Qt::LeftButton);
   sendMouseMove(view.viewport(), view.mapFromScene(20, 300), Qt::LeftButton, Qt::LeftButton);
   QVERIFY(!item1->isSelected());
   QVERIFY(item2->isSelected());
   QVERIFY(item3->isSelected());
   sendMouseRelease(view.viewport(), QPoint(), Qt::LeftButton);

   scene.clearSelection();

   // select first item
   item1->setSelected(true);
   QVERIFY(item1->isSelected());

   // now rubberband with modifier key
   {
      QPoint clickPoint = view.mapFromScene(20, 115);
      QMouseEvent event(QEvent::MouseButtonPress, clickPoint, view.viewport()->mapToGlobal(clickPoint), Qt::LeftButton, 0, Qt::ControlModifier);
      QApplication::sendEvent(view.viewport(), &event);
   }
   sendMouseMove(view.viewport(), view.mapFromScene(20, 300), Qt::LeftButton, Qt::LeftButton);
   QVERIFY(item1->isSelected());
   QVERIFY(item2->isSelected());
   QVERIFY(item3->isSelected());
}

void tst_QGraphicsView::rotated_rubberBand()
{
    QWidget toplevel;
    setFrameless(&toplevel);

    QGraphicsScene scene;
    const int dim = 3;
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j ++) {
            QGraphicsRectItem *rect = new QGraphicsRectItem(i * 20, j * 20, 10, 10);
            rect->setFlag(QGraphicsItem::ItemIsSelectable);
            rect->setData(0, (i == j));
            scene.addItem(rect);
        }
    }

    QGraphicsView view(&scene, &toplevel);
    QCOMPARE(view.rubberBandSelectionMode(), Qt::IntersectsItemShape);
    view.setDragMode(QGraphicsView::RubberBandDrag);
    view.resize(120, 120);
    view.rotate(45);
    toplevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toplevel));

    // Disable mouse tracking to prevent the window system from sending mouse
    // move events to the viewport while we are synthesizing events. If
    // QGraphicsView gets a mouse move event with no buttons down, it'll
    // terminate the rubber band.
    view.viewport()->setMouseTracking(false);

    QCOMPARE(scene.selectedItems(), QList<QGraphicsItem *>());
    int midWidth = view.viewport()->width() / 2;
    sendMousePress(view.viewport(), QPoint(midWidth - 2, 0), Qt::LeftButton);
    sendMouseMove(view.viewport(), QPoint(midWidth + 2, view.viewport()->height()),
                  Qt::LeftButton, Qt::LeftButton);
    QCOMPARE(scene.selectedItems().count(), dim);
    foreach (const QGraphicsItem *item, scene.items()) {
        QCOMPARE(item->isSelected(), item->data(0).toBool());
    }
    sendMouseRelease(view.viewport(), QPoint(), Qt::LeftButton);
}

void tst_QGraphicsView::backgroundBrush()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    scene.setBackgroundBrush(Qt::blue);
    QCOMPARE(scene.backgroundBrush(), QBrush(Qt::blue));

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    scene.setBackgroundBrush(QBrush());
    QCOMPARE(scene.backgroundBrush(), QBrush());
    QTest::qWait(25);

    QRadialGradient gradient(0, 0, 10);
    gradient.setSpread(QGradient::RepeatSpread);
    scene.setBackgroundBrush(gradient);

    QCOMPARE(scene.backgroundBrush(), QBrush(gradient));
    QTest::qWait(25);
}

void tst_QGraphicsView::foregroundBrush()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    scene.setForegroundBrush(Qt::blue);
    QCOMPARE(scene.foregroundBrush(), QBrush(Qt::blue));

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    scene.setForegroundBrush(QBrush());
    QCOMPARE(scene.foregroundBrush(), QBrush());
    QTest::qWait(25);

    QRadialGradient gradient(0, 0, 10);
    gradient.setSpread(QGradient::RepeatSpread);
    scene.setForegroundBrush(gradient);

    QCOMPARE(scene.foregroundBrush(), QBrush(gradient));
    QTest::qWait(25);

    for (int i = 0; i < 50; ++i) {
        QRadialGradient gradient(view.rect().center() + QPoint(int(sin(i / 2.0) * 10), int(cos(i / 2.0) * 10)), 10);
        gradient.setColorAt(0, Qt::transparent);
        gradient.setColorAt(0.5, Qt::black);
        gradient.setColorAt(1, Qt::transparent);
        gradient.setSpread(QGradient::RepeatSpread);
        scene.setForegroundBrush(gradient);

        QRadialGradient gradient2(view.rect().center() + QPoint(int(sin(i / 1.7) * 10), int(cos(i / 1.7) * 10)), 10);
        gradient2.setColorAt(0, Qt::transparent);
        gradient2.setColorAt(0.5, Qt::black);
        gradient2.setColorAt(1, Qt::transparent);
        gradient2.setSpread(QGradient::RepeatSpread);
        scene.setBackgroundBrush(gradient2);

        QRadialGradient gradient3(view.rect().center() + QPoint(int(sin(i / 1.85) * 10), int(cos(i / 1.85) * 10)), 10);
        gradient3.setColorAt(0, Qt::transparent);
        gradient3.setColorAt(0.5, Qt::black);
        gradient3.setColorAt(1, Qt::transparent);
        gradient3.setSpread(QGradient::RepeatSpread);
        scene.setBackgroundBrush(gradient3);

        QApplication::processEvents();
    }

    view.setSceneRect(-1000, -1000, 2000, 2000);
    for (int i = -500; i < 500; i += 10) {
        view.centerOn(i, 0);
        QApplication::processEvents();
        QApplication::processEvents();
    }
    for (int i = -500; i < 500; i += 10) {
        view.centerOn(0, i);
        QApplication::processEvents();
        QApplication::processEvents();
    }
}

void tst_QGraphicsView::matrix()
{
    {
        QGraphicsScene scene;
        QGraphicsView view(&scene);
        view.show();

        // Show rendering of background with no scene
        for (int i = 0; i < 50; ++i) {
            view.rotate(5);
            QRadialGradient gradient(view.rect().center() + QPoint(int(sin(i / 2.0) * 10), int(cos(i / 2.0) * 10)), 10);
            gradient.setColorAt(0, Qt::transparent);
            gradient.setColorAt(0.5, Qt::black);
            gradient.setColorAt(1, Qt::transparent);
            gradient.setSpread(QGradient::RepeatSpread);
            scene.setForegroundBrush(gradient);
            QRadialGradient gradient2(view.rect().center() + QPoint(int(sin(i / 1.7) * 10), int(cos(i / 1.7) * 10)), 10);
            gradient2.setColorAt(0, Qt::transparent);
            gradient2.setColorAt(0.5, Qt::black);
            gradient2.setColorAt(1, Qt::transparent);
            gradient2.setSpread(QGradient::RepeatSpread);
            scene.setBackgroundBrush(gradient2);
            QApplication::processEvents();
            QApplication::processEvents();
        }
    }

    // Test transformation extremes, see if they cause crashes
    {
        QGraphicsScene scene;
        scene.addText("GraphicsView rotated clockwise");

        QGraphicsView view(&scene);
        view.show();
        for (int i = 0; i < 160; ++i) {
            view.rotate(18);
            QApplication::processEvents();
            QApplication::processEvents();
        }
        /*
          // These cause a crash
        for (int i = 0; i < 40; ++i) {
            view.shear(1.2, 1.2);
            QTest::qWait(20);
        }
        for (int i = 0; i < 40; ++i) {
            view.shear(-1.2, -1.2);
            QTest::qWait(20);
        }
        */
        for (int i = 0; i < 20; ++i) {
            view.scale(1.2, 1.2);
            QApplication::processEvents();
            QApplication::processEvents();
        }
        for (int i = 0; i < 20; ++i) {
            view.scale(0.6, 0.6);
            QApplication::processEvents();
            QApplication::processEvents();
        }
    }
}

void tst_QGraphicsView::matrix_convenience()
{
    QGraphicsView view;
    QCOMPARE(view.matrix(), QMatrix());

    // Check the convenience functions
    view.rotate(90);
    QCOMPARE(view.matrix(), QMatrix().rotate(90));
    view.scale(2, 2);
    QCOMPARE(view.matrix(), QMatrix().scale(2, 2) * QMatrix().rotate(90));
    view.shear(1.2, 1.2);
    QCOMPARE(view.matrix(), QMatrix().shear(1.2, 1.2) * QMatrix().scale(2, 2) * QMatrix().rotate(90));
    view.translate(1, 1);
    QCOMPARE(view.matrix(), QMatrix().translate(1, 1) * QMatrix().shear(1.2, 1.2) * QMatrix().scale(2, 2) * QMatrix().rotate(90));
}

void tst_QGraphicsView::matrix_combine()
{
    // Check matrix combining
    QGraphicsView view;
    QCOMPARE(view.matrix(), QMatrix());
    view.setMatrix(QMatrix().rotate(90), true);
    view.setMatrix(QMatrix().rotate(90), true);
    view.setMatrix(QMatrix().rotate(90), true);
    view.setMatrix(QMatrix().rotate(90), true);
    QCOMPARE(view.matrix(), QMatrix());

    view.resetMatrix();
    QCOMPARE(view.matrix(), QMatrix());
    view.setMatrix(QMatrix().rotate(90), false);
    view.setMatrix(QMatrix().rotate(90), false);
    view.setMatrix(QMatrix().rotate(90), false);
    view.setMatrix(QMatrix().rotate(90), false);
    QCOMPARE(view.matrix(), QMatrix().rotate(90));
}

void tst_QGraphicsView::centerOnPoint()
{
    QWidget toplevel;
    setFrameless(&toplevel);

    QGraphicsScene scene;
    scene.addEllipse(QRectF(-100, -100, 50, 50));
    scene.addEllipse(QRectF(50, -100, 50, 50));
    scene.addEllipse(QRectF(-100, 50, 50, 50));
    scene.addEllipse(QRectF(50, 50, 50, 50));

    QGraphicsView view(&scene, &toplevel);
    view.setSceneRect(-400, -400, 800, 800);
    view.setFixedSize(100, 100);
    toplevel.show();

    int tolerance = 5;

    for (int i = 0; i < 3; ++i) {
        for (int y = -100; y < 100; y += 23) {
            for (int x = -100; x < 100; x += 23) {
                view.centerOn(x, y);
                QPoint viewCenter = view.mapToScene(view.viewport()->rect().center()).toPoint();

                // Fuzzy compare
                if (viewCenter.x() < x - tolerance || viewCenter.x() > x + tolerance
                    || viewCenter.y() < y - tolerance || viewCenter.y() > y + tolerance) {
                    QString error = QString("Compared values are not the same\n\tActual: (%1, %2)\n\tExpected: (%3, %4)")
                                    .arg(viewCenter.x()).arg(viewCenter.y()).arg(x).arg(y);
                    QFAIL(qPrintable(error));
                }

                QApplication::processEvents();
            }
        }

        view.rotate(13);
        view.scale(1.5, 1.5);
        view.shear(1.25, 1.25);
    }
}

void tst_QGraphicsView::centerOnItem()
{
    QGraphicsScene scene;
    QGraphicsItem *items[4];
    items[0] = scene.addEllipse(QRectF(-25, -25, 50, 50));
    items[1] = scene.addEllipse(QRectF(-25, -25, 50, 50));
    items[2] = scene.addEllipse(QRectF(-25, -25, 50, 50));
    items[3] = scene.addEllipse(QRectF(-25, -25, 50, 50));
    items[0]->setPos(-100, -100);
    items[1]->setPos(100, -100);
    items[2]->setPos(-100, 100);
    items[3]->setPos(100, 100);

    QGraphicsView view(&scene);
    view.setSceneRect(-1000, -1000, 2000, 2000);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    int tolerance = 7;

    for (int x = 0; x < 3; ++x) {
        for (int i = 0; i < 4; ++i) {
            QApplication::processEvents();
            view.centerOn(items[i]);

            QPoint viewCenter = view.mapToScene(view.viewport()->rect().center()).toPoint();
            qreal x = items[i]->pos().x();
            qreal y = items[i]->pos().y();

            // Fuzzy compare
            if (viewCenter.x() < x - tolerance || viewCenter.x() > x + tolerance
                || viewCenter.y() < y - tolerance || viewCenter.y() > y + tolerance) {
                QString error = QString("Compared values are not the same\n\tActual: (%1, %2)\n\tExpected: (%3, %4)")
                                .arg(viewCenter.x()).arg(viewCenter.y()).arg(x).arg(y);
                QFAIL(qPrintable(error));
            }

            QApplication::processEvents();
        }

        view.rotate(13);
        view.scale(1.5, 1.5);
        view.shear(1.25, 1.25);
    }
}

void tst_QGraphicsView::ensureVisibleRect()
{
    QWidget toplevel;

    QGraphicsScene scene;
    QGraphicsItem *items[4];
    items[0] = scene.addEllipse(QRectF(-25, -25, 50, 50), QPen(Qt::black), QBrush(Qt::green));
    items[1] = scene.addEllipse(QRectF(-25, -25, 50, 50), QPen(Qt::black), QBrush(Qt::red));
    items[2] = scene.addEllipse(QRectF(-25, -25, 50, 50), QPen(Qt::black), QBrush(Qt::blue));
    items[3] = scene.addEllipse(QRectF(-25, -25, 50, 50), QPen(Qt::black), QBrush(Qt::yellow));
    scene.addLine(QLineF(0, -100, 0, 100), QPen(Qt::blue, 2));
    scene.addLine(QLineF(-100, 0, 100, 0), QPen(Qt::blue, 2));
    items[0]->setPos(-100, -100);
    items[1]->setPos(100, -100);
    items[2]->setPos(-100, 100);
    items[3]->setPos(100, 100);

    QGraphicsItem *icon = scene.addEllipse(QRectF(-10, -10, 20, 20), QPen(Qt::black), QBrush(Qt::gray));

    QGraphicsView view(&scene, &toplevel);
    view.setSceneRect(-500, -500, 1000, 1000);
    view.setFixedSize(250, 250);
    toplevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toplevel));

    for (int y = -100; y < 100; y += 25) {
        for (int x = -100; x < 100; x += 13) {

            icon->setPos(x, y);

            switch (x & 3) {
            case 0:
                view.centerOn(-500, -500);
                break;
            case 1:
                view.centerOn(500, -500);
                break;
            case 2:
                view.centerOn(-500, 500);
                break;
            case 3:
            default:
                view.centerOn(500, 500);
                break;
            }

            QVERIFY(!view.viewport()->rect().contains(view.mapFromScene(x, y)));

            for (int margin = 10; margin < 60; margin += 15) {
                view.ensureVisible(x, y, 0, 0, margin, margin);

                QRect viewRect = view.viewport()->rect();
                QPoint viewPoint = view.mapFromScene(x, y);

                QVERIFY(viewRect.contains(viewPoint));
                QVERIFY(qAbs(viewPoint.x() - viewRect.left()) >= margin -1);
                QVERIFY(qAbs(viewPoint.x() - viewRect.right()) >= margin -1);
                QVERIFY(qAbs(viewPoint.y() - viewRect.top()) >= margin -1);
                QVERIFY(qAbs(viewPoint.y() - viewRect.bottom()) >= margin -1);

                QApplication::processEvents();
            }
        }
        view.rotate(5);
        view.scale(1.05, 1.05);
        view.translate(30, -30);
    }
}

void tst_QGraphicsView::fitInView()
{
    QGraphicsScene scene;
    QGraphicsItem *items[4];
    items[0] = scene.addEllipse(QRectF(-25, -25, 100, 20), QPen(Qt::black), QBrush(Qt::green));
    items[1] = scene.addEllipse(QRectF(-25, -25, 20, 100), QPen(Qt::black), QBrush(Qt::red));
    items[2] = scene.addEllipse(QRectF(-25, -25, 50, 50), QPen(Qt::black), QBrush(Qt::blue));
    items[3] = scene.addEllipse(QRectF(-25, -25, 50, 50), QPen(Qt::black), QBrush(Qt::yellow));
    scene.addLine(QLineF(0, -100, 0, 100), QPen(Qt::blue, 2));
    scene.addLine(QLineF(-100, 0, 100, 0), QPen(Qt::blue, 2));
    items[0]->setPos(-100, -100);
    items[1]->setPos(100, -100);
    items[2]->setPos(-100, 100);
    items[3]->setPos(100, 100);

    items[0]->setTransform(QTransform().rotate(30), true);
    items[1]->setTransform(QTransform().rotate(-30), true);

    QGraphicsView view(&scene);
    view.setSceneRect(-400, -400, 800, 800);
    view.setFixedSize(400, 200);

    view.showNormal();
    view.fitInView(scene.itemsBoundingRect(), Qt::IgnoreAspectRatio);
    qApp->processEvents();

    // Sampled coordinates.
    QVERIFY(!view.itemAt(45, 41));
    QVERIFY(!view.itemAt(297, 44));
    QVERIFY(!view.itemAt(359, 143));
    QCOMPARE(view.itemAt(79, 22), items[0]);
    QCOMPARE(view.itemAt(329, 41), items[1]);
    QCOMPARE(view.itemAt(38, 158), items[2]);
    QCOMPARE(view.itemAt(332, 160), items[3]);

    view.fitInView(items[0], Qt::IgnoreAspectRatio);
    qApp->processEvents();

    QCOMPARE(view.itemAt(19, 13), items[0]);
    QCOMPARE(view.itemAt(91, 47), items[0]);
    QCOMPARE(view.itemAt(202, 94), items[0]);
    QCOMPARE(view.itemAt(344, 161), items[0]);
    QVERIFY(!view.itemAt(236, 54));
    QVERIFY(!view.itemAt(144, 11));
    QVERIFY(!view.itemAt(29, 69));
    QVERIFY(!view.itemAt(251, 167));

    view.fitInView(items[0], Qt::KeepAspectRatio);
    qApp->processEvents();

    QCOMPARE(view.itemAt(325, 170), items[0]);
    QCOMPARE(view.itemAt(206, 74), items[0]);
    QCOMPARE(view.itemAt(190, 115), items[0]);
    QCOMPARE(view.itemAt(55, 14), items[0]);
    QVERIFY(!view.itemAt(109, 4));
    QVERIFY(!view.itemAt(244, 68));
    QVERIFY(!view.itemAt(310, 125));
    QVERIFY(!view.itemAt(261, 168));

    view.fitInView(items[0], Qt::KeepAspectRatioByExpanding);
    qApp->processEvents();

    QCOMPARE(view.itemAt(18, 10), items[0]);
    QCOMPARE(view.itemAt(95, 4), items[0]);
    QCOMPARE(view.itemAt(279, 175), items[0]);
    QCOMPARE(view.itemAt(359, 170), items[0]);
    QVERIFY(!view.itemAt(370, 166));
    QVERIFY(!view.itemAt(136, 7));
    QVERIFY(!view.itemAt(31, 44));
    QVERIFY(!view.itemAt(203, 153));
}

void tst_QGraphicsView::itemsAtPoint()
{
    QGraphicsScene scene;
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(1);
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(0);
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(2);
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(-1);
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(3);

    QGraphicsView view;
    QVERIFY(view.items(0, 0).isEmpty());

    view.setScene(&scene);
    view.setSceneRect(-10000, -10000, 20000, 20000);
    view.show();

    QList<QGraphicsItem *> items = view.items(view.viewport()->rect().center());
    QCOMPARE(items.size(), 5);
    QCOMPARE(items.takeFirst()->zValue(), qreal(3));
    QCOMPARE(items.takeFirst()->zValue(), qreal(2));
    QCOMPARE(items.takeFirst()->zValue(), qreal(1));
    QCOMPARE(items.takeFirst()->zValue(), qreal(0));
    QCOMPARE(items.takeFirst()->zValue(), qreal(-1));
}

#if defined QT_BUILD_INTERNAL
void tst_QGraphicsView::itemsAtPosition_data()
{
    QTest::addColumn<float>("rotation");
    QTest::addColumn<float>("scale");
    QTest::addColumn<QPoint>("viewPos");
    QTest::addColumn<bool>("ignoreTransform");
    QTest::addColumn<bool>("hit");
    QTest::newRow("scaled + ignore transform, no hit") << 0.0f << 1000.0f << QPoint(0, 0) << true << false;
    QTest::newRow("scaled + ignore transform, hit") << 0.0f << 1000.0f << QPoint(100, 100) << true << true;
    QTest::newRow("rotated + scaled, no hit") << 45.0f << 2.0f << QPoint(90, 90) << false << false;
    QTest::newRow("rotated + scaled, hit") << 45.0f << 2.0f << QPoint(100, 100) << false << true;
}

void tst_QGraphicsView::itemsAtPosition()
{
    QFETCH(float, rotation);
    QFETCH(float, scale);
    QFETCH(QPoint, viewPos);
    QFETCH(bool, ignoreTransform);
    QFETCH(bool, hit);

    FriendlyGraphicsScene scene;
    scene.setSceneRect(QRect(-100, -100, 200, 200));
    QGraphicsItem *item = scene.addRect(-5, -5, 10, 10);

    if (ignoreTransform)
        item->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    QGraphicsView view;
    view.setFrameStyle(QFrame::NoFrame);
    view.resize(200, 200);
    view.scale(scale, scale);
    view.rotate(rotation);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setScene(&scene);
    view.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QPoint screenPos = view.viewport()->mapToGlobal(viewPos);
    QPointF scenePos = view.mapToScene(viewPos);
    QGraphicsScenePrivate *viewPrivate = scene.d_func();
    QList<QGraphicsItem *> items;
    items = viewPrivate->itemsAtPosition(screenPos, scenePos, view.viewport());
    QCOMPARE(!items.empty(), hit);
}
#endif

void tst_QGraphicsView::itemsInRect()
{
    QGraphicsScene scene;
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(1);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(0);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(2);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(-1);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(3);

    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(5);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(4);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(6);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(3);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(7);

    QGraphicsView view;
    QVERIFY(view.items(QRect(-100, -100, 200, 200)).isEmpty());
    view.setScene(&scene);
    view.setSceneRect(-10000, -10000, 20000, 20000);
    view.show();

    QRect leftRect = view.mapFromScene(-30, -10, 20, 20).boundingRect();
    QRect rightRect = view.mapFromScene(30, -10, 20, 20).boundingRect();

    QList<QGraphicsItem *> items = view.items(leftRect);
    QCOMPARE(items.size(), 5);
    QCOMPARE(items.takeFirst()->zValue(), qreal(3));
    QCOMPARE(items.takeFirst()->zValue(), qreal(2));
    QCOMPARE(items.takeFirst()->zValue(), qreal(1));
    QCOMPARE(items.takeFirst()->zValue(), qreal(0));
    QCOMPARE(items.takeFirst()->zValue(), qreal(-1));

    items = view.items(rightRect);
    QCOMPARE(items.size(), 5);
    QCOMPARE(items.takeFirst()->zValue(), qreal(7));
    QCOMPARE(items.takeFirst()->zValue(), qreal(6));
    QCOMPARE(items.takeFirst()->zValue(), qreal(5));
    QCOMPARE(items.takeFirst()->zValue(), qreal(4));
    QCOMPARE(items.takeFirst()->zValue(), qreal(3));
}

class CountPaintItem : public QGraphicsRectItem
{
public:
    int numPaints;

    CountPaintItem(const QRectF &rect)
        : QGraphicsRectItem(rect), numPaints(0)
    { }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0)
    {
        ++numPaints;
        QGraphicsRectItem::paint(painter, option, widget);
    }
};

void tst_QGraphicsView::itemsInRect_cosmeticAdjust_data()
{
    QTest::addColumn<QRect>("updateRect");
    QTest::addColumn<int>("numPaints");
    QTest::addColumn<bool>("adjustForAntialiasing");

    // Aliased.
    QTest::newRow("nil") << QRect() << 1 << false;
    QTest::newRow("0, 0, 300, 100") << QRect(0, 0, 300, 100) << 1 << false;
    QTest::newRow("0, 0, 100, 300") << QRect(0, 0, 100, 300) << 1 << false;
    QTest::newRow("200, 0, 100, 300") << QRect(200, 0, 100, 300) << 1 << false;
    QTest::newRow("0, 200, 300, 100") << QRect(0, 200, 300, 100) << 1 << false;
    QTest::newRow("0, 0, 300, 99") << QRect(0, 0, 300, 99) << 0 << false;
    QTest::newRow("0, 0, 99, 300") << QRect(0, 0, 99, 300) << 0 << false;
    QTest::newRow("201, 0, 99, 300") << QRect(201, 0, 99, 300) << 0 << false;
    QTest::newRow("0, 201, 300, 99") << QRect(0, 201, 300, 99) << 0 << false;

    // Anti-aliased.
    QTest::newRow("nil") << QRect() << 1 << true;
    QTest::newRow("0, 0, 300, 100") << QRect(0, 0, 300, 100) << 1 << true;
    QTest::newRow("0, 0, 100, 300") << QRect(0, 0, 100, 300) << 1 << true;
    QTest::newRow("200, 0, 100, 300") << QRect(200, 0, 100, 300) << 1 << true;
    QTest::newRow("0, 200, 300, 100") << QRect(0, 200, 300, 100) << 1 << true;
    QTest::newRow("0, 0, 300, 99") << QRect(0, 0, 300, 99) << 1 << true;
    QTest::newRow("0, 0, 99, 300") << QRect(0, 0, 99, 300) << 1 << true;
    QTest::newRow("201, 0, 99, 300") << QRect(201, 0, 99, 300) << 1 << true;
    QTest::newRow("0, 201, 300, 99") << QRect(0, 201, 300, 99) << 1 << true;
    QTest::newRow("0, 0, 300, 98") << QRect(0, 0, 300, 98) << 0 << false;
    QTest::newRow("0, 0, 98, 300") << QRect(0, 0, 98, 300) << 0 << false;
    QTest::newRow("202, 0, 98, 300") << QRect(202, 0, 98, 300) << 0 << false;
    QTest::newRow("0, 202, 300, 98") << QRect(0, 202, 300, 98) << 0 << false;
}

void tst_QGraphicsView::itemsInRect_cosmeticAdjust()
{
    QFETCH(QRect, updateRect);
    QFETCH(int, numPaints);
    QFETCH(bool, adjustForAntialiasing);

    QGraphicsScene scene(-100, -100, 200, 200);
    CountPaintItem *rect = new CountPaintItem(QRectF(-50, -50, 100, 100));
    rect->setPen(QPen(Qt::black, 0));
    scene.addItem(rect);

    QGraphicsView view(&scene);
    view.setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, !adjustForAntialiasing);
    view.setRenderHint(QPainter::Antialiasing, adjustForAntialiasing);
    view.setFrameStyle(0);
    view.resize(300, 300);
    view.showNormal();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(rect->numPaints > 0);

    rect->numPaints = 0;
    if (updateRect.isNull())
        view.viewport()->update();
    else
        view.viewport()->update(updateRect);
    qApp->processEvents();
    QTRY_COMPARE(rect->numPaints, numPaints);
}

void tst_QGraphicsView::itemsInPoly()
{
    QGraphicsScene scene;
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(1);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(0);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(2);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(-1);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(3);

    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(5);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(4);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(6);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(3);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(7);

    QGraphicsView view;
    QVERIFY(view.items(QPolygon()).isEmpty());
    view.setScene(&scene);
    view.setSceneRect(-10000, -10000, 20000, 20000);
    view.show();

    QPolygon leftPoly = view.mapFromScene(QRectF(-30, -10, 20, 20));
    QPolygon rightPoly = view.mapFromScene(QRectF(30, -10, 20, 20));

    QList<QGraphicsItem *> items = view.items(leftPoly);
    QCOMPARE(items.size(), 5);
    QCOMPARE(items.takeFirst()->zValue(), qreal(3));
    QCOMPARE(items.takeFirst()->zValue(), qreal(2));
    QCOMPARE(items.takeFirst()->zValue(), qreal(1));
    QCOMPARE(items.takeFirst()->zValue(), qreal(0));
    QCOMPARE(items.takeFirst()->zValue(), qreal(-1));

    items = view.items(rightPoly);
    QCOMPARE(items.size(), 5);
    QCOMPARE(items.takeFirst()->zValue(), qreal(7));
    QCOMPARE(items.takeFirst()->zValue(), qreal(6));
    QCOMPARE(items.takeFirst()->zValue(), qreal(5));
    QCOMPARE(items.takeFirst()->zValue(), qreal(4));
    QCOMPARE(items.takeFirst()->zValue(), qreal(3));
}

void tst_QGraphicsView::itemsInPath()
{
    QGraphicsScene scene;
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(1);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(0);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(2);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(-1);
    scene.addRect(QRectF(-30, -10, 20, 20))->setZValue(3);

    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(5);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(4);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(6);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(3);
    scene.addRect(QRectF(30, -10, 20, 20))->setZValue(7);

    QGraphicsView view;
    QVERIFY(view.items(QPainterPath()).isEmpty());
    view.setScene(&scene);
    view.translate(100, 400);
    view.rotate(22.3);
    view.setSceneRect(-10000, -10000, 20000, 20000);
    view.show();

    QPainterPath leftPath;
    leftPath.addEllipse(QRect(view.mapFromScene(-30, -10), QSize(20, 20)));

    QPainterPath rightPath;
    rightPath.addEllipse(QRect(view.mapFromScene(30, -10), QSize(20, 20)));

    QList<QGraphicsItem *> items = view.items(leftPath);

    QCOMPARE(items.size(), 5);
    QCOMPARE(items.takeFirst()->zValue(), qreal(3));
    QCOMPARE(items.takeFirst()->zValue(), qreal(2));
    QCOMPARE(items.takeFirst()->zValue(), qreal(1));
    QCOMPARE(items.takeFirst()->zValue(), qreal(0));
    QCOMPARE(items.takeFirst()->zValue(), qreal(-1));

    items = view.items(rightPath);
    QCOMPARE(items.size(), 5);
    QCOMPARE(items.takeFirst()->zValue(), qreal(7));
    QCOMPARE(items.takeFirst()->zValue(), qreal(6));
    QCOMPARE(items.takeFirst()->zValue(), qreal(5));
    QCOMPARE(items.takeFirst()->zValue(), qreal(4));
    QCOMPARE(items.takeFirst()->zValue(), qreal(3));
}

void tst_QGraphicsView::itemAt()
{
    QGraphicsScene scene;
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(1);
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(0);
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(2);
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(-1);
    scene.addRect(QRectF(-10, -10, 20, 20))->setZValue(3);

    QGraphicsView view;
    QCOMPARE(view.itemAt(0, 0), (QGraphicsItem *)0);

    view.setScene(&scene);
    view.setSceneRect(-10000, -10000, 20000, 20000);
    view.show();

    QCOMPARE(view.itemAt(0, 0), (QGraphicsItem *)0);
    QGraphicsItem* item = view.itemAt(view.viewport()->rect().center());
    QVERIFY(item);
    QCOMPARE(item->zValue(), qreal(3));
}

void tst_QGraphicsView::itemAt2()
{
    // test precision of the itemAt() function with items that are smaller
    // than 1 pixel.
    QGraphicsScene scene(0, 0, 100, 100);

    // Add a 0.5x0.5 item at position 0 on the scene, top-left corner at -0.25, -0.25.
    QGraphicsItem *item = scene.addRect(QRectF(-0.25, -0.25, 0.5, 0.5), QPen(Qt::black, 0.1));

    QGraphicsView view(&scene);
    view.setFixedSize(200, 200);
    view.setTransformationAnchor(QGraphicsView::NoAnchor);
    view.setRenderHint(QPainter::Antialiasing);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QApplication::processEvents();

    QPoint itemViewPoint = view.mapFromScene(item->scenePos());

    for (int i = 0; i < 3; ++i) {
        QVERIFY(view.itemAt(itemViewPoint));
        QVERIFY(!view.items(itemViewPoint).isEmpty());
        QVERIFY(view.itemAt(itemViewPoint + QPoint(-1, 0)));
        QVERIFY(!view.items(itemViewPoint + QPoint(-1, 0)).isEmpty());
        QVERIFY(view.itemAt(itemViewPoint + QPoint(-1, -1)));
        QVERIFY(!view.items(itemViewPoint + QPoint(-1, -1)).isEmpty());
        QVERIFY(view.itemAt(itemViewPoint + QPoint(0, -1)));
        QVERIFY(!view.items(itemViewPoint + QPoint(0, -1)).isEmpty());
        item->moveBy(0.1, 0);
    }

    // Here
    QVERIFY(view.itemAt(itemViewPoint));
    QVERIFY(!view.items(itemViewPoint).isEmpty());
    QVERIFY(view.itemAt(itemViewPoint + QPoint(0, -1)));
    QVERIFY(!view.items(itemViewPoint + QPoint(0, -1)).isEmpty());

    if (sizeof(qreal) != sizeof(double))
        QSKIP("Skipped due to rounding errors");

    // Not here
    QVERIFY(!view.itemAt(itemViewPoint + QPoint(-1, 0)));
    QVERIFY(view.items(itemViewPoint + QPoint(-1, 0)).isEmpty());
    QVERIFY(!view.itemAt(itemViewPoint + QPoint(-1, -1)));
    QVERIFY(view.items(itemViewPoint + QPoint(-1, -1)).isEmpty());
}

void tst_QGraphicsView::mapToScene()
{
    // Uncomment the commented-out code to see what's going on. It doesn't
    // affect the test; it just slows it down.

    QGraphicsScene scene;
    scene.addPixmap(QPixmap("3D-Qt-1-2.png"));

    QWidget topLevel;
    QGraphicsView view(&topLevel);
    view.setScene(&scene);
    view.setSceneRect(-500, -500, 1000, 1000);
    QSize viewSize(300,300);

    view.setFixedSize(viewSize);
    topLevel.show();
    QApplication::processEvents();
    QVERIFY(view.isVisible());
    QCOMPARE(view.size(), viewSize);

    // First once without setting the scene rect
#ifdef Q_PROCESSOR_ARM
    const int step = 20;
#else
    const int step = 5;
#endif

    for (int x = 0; x < view.width(); x += step) {
        for (int y = 0; y < view.height(); y += step) {
            QCOMPARE(view.mapToScene(QPoint(x, y)),
                     QPointF(view.horizontalScrollBar()->value() + x,
                             view.verticalScrollBar()->value() + y));
        }
    }

    for (int sceneRectHeight = 250; sceneRectHeight < 1000; sceneRectHeight += 250) {
        for (int sceneRectWidth = 250; sceneRectWidth < 1000; sceneRectWidth += 250) {
            view.setSceneRect(QRectF(-int(sceneRectWidth / 2), -int(sceneRectHeight / 2),
                                     sceneRectWidth, sceneRectHeight));
            QApplication::processEvents();

            int hmin = view.horizontalScrollBar()->minimum();
            int hmax = view.horizontalScrollBar()->maximum();
            int hstep = (hmax - hmin) / 3;
            int vmin = view.verticalScrollBar()->minimum();
            int vmax = view.verticalScrollBar()->maximum();
            int vstep = (vmax - vmin) / 3;

            for (int hscrollValue = hmin; hscrollValue < hmax; hscrollValue += hstep) {
                for (int vscrollValue = vmin; vscrollValue < vmax; vscrollValue += vstep) {

                    view.horizontalScrollBar()->setValue(hscrollValue);
                    view.verticalScrollBar()->setValue(vscrollValue);
                    QApplication::processEvents();

                    int h = view.horizontalScrollBar()->value();
                    int v = view.verticalScrollBar()->value();

                    for (int x = 0; x < view.width(); x += step) {
                        for (int y = 0; y < view.height(); y += step) {
                            QCOMPARE(view.mapToScene(QPoint(x, y)), QPointF(h + x, v + y));
                            QCOMPARE(view.mapFromScene(QPointF(h + x, v + y)), QPoint(x, y));
                        }
                    }
                }
            }
        }
    }
}

void tst_QGraphicsView::mapToScenePoint()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    setFrameless(&view);
    view.rotate(90);
    view.setFixedSize(117, 117);
    view.show();
    QPoint center = view.viewport()->rect().center();
    QCOMPARE(view.mapToScene(center + QPoint(10, 0)),
             view.mapToScene(center) + QPointF(0, -10));
}

void tst_QGraphicsView::mapToSceneRect_data()
{
    QTest::addColumn<QRect>("viewRect");
    QTest::addColumn<QPolygonF>("scenePoly");
    QTest::addColumn<qreal>("rotation");

    QTest::newRow("nil") << QRect() << QPolygonF() << qreal(0);
    QTest::newRow("0, 0, 1, 1") << QRect(0, 0, 1, 1) << QPolygonF(QRectF(0, 0, 1, 1)) << qreal(0);
    QTest::newRow("0, 0, 10, 10") << QRect(0, 0, 10, 10) << QPolygonF(QRectF(0, 0, 10, 10)) << qreal(0);
    QTest::newRow("nil") << QRect() << QPolygonF() << qreal(90);
    QPolygonF p;
    p << QPointF(0, 0) << QPointF(0, -1) << QPointF(1, -1) << QPointF(1, 0) << QPointF(0, 0);
    QTest::newRow("0, 0, 1, 1") << QRect(0, 0, 1, 1)
                                << p
                                << qreal(90);
    p.clear();
    p << QPointF(0, 0) << QPointF(0, -10) << QPointF(10, -10) << QPointF(10, 0) << QPointF(0, 0);
    QTest::newRow("0, 0, 10, 10") << QRect(0, 0, 10, 10)
                                  << p
                                  << qreal(90);
}

void tst_QGraphicsView::mapToSceneRect()
{
    QFETCH(QRect, viewRect);
    QFETCH(QPolygonF, scenePoly);
    QFETCH(qreal, rotation);

    QGraphicsScene scene(-1000, -1000, 2000, 2000);
    scene.addRect(25, -25, 50, 50);
    QGraphicsView view(&scene);
    view.setFrameStyle(0);
    view.setAlignment(Qt::AlignTop | Qt::AlignLeft);
    view.setFixedSize(200, 200);
    view.setTransformationAnchor(QGraphicsView::NoAnchor);
    view.setResizeAnchor(QGraphicsView::NoAnchor);
    view.show();

    view.rotate(rotation);

    QPolygonF poly = view.mapToScene(viewRect);
    if (!poly.isEmpty())
        poly << poly[0];

    QCOMPARE(poly, scenePoly);
}

void tst_QGraphicsView::mapToScenePoly()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    setFrameless(&view);
    view.translate(100, 100);
    view.setFixedSize(117, 117);
    view.show();
    QPoint center = view.viewport()->rect().center();
    QRect rect(center + QPoint(10, 0), QSize(10, 10));

    QPolygon poly;
    poly << rect.topLeft();
    poly << rect.topRight();
    poly << rect.bottomRight();
    poly << rect.bottomLeft();

    QPolygonF poly2;
    poly2 << view.mapToScene(rect.topLeft());
    poly2 << view.mapToScene(rect.topRight());
    poly2 << view.mapToScene(rect.bottomRight());
    poly2 << view.mapToScene(rect.bottomLeft());

    QCOMPARE(view.mapToScene(poly), poly2);
}

void tst_QGraphicsView::mapToScenePath()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setSceneRect(-300, -300, 600, 600);
    view.translate(10, 10);
    view.setFixedSize(300, 300);
    view.show();
    QRect rect(QPoint(10, 0), QSize(10, 10));

    QPainterPath path;
    path.addRect(rect);

    QPainterPath path2;
    path2.addRect(rect.translated(view.horizontalScrollBar()->value() - 10,
                                  view.verticalScrollBar()->value() - 10));
    QCOMPARE(view.mapToScene(path), path2);
}

void tst_QGraphicsView::mapFromScenePoint()
{
    {
        QGraphicsScene scene;
        QGraphicsView view(&scene);
        view.rotate(90);
        view.scale(10, 10);
        view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view.show();

        QPoint mapped = view.mapFromScene(0, 0);
        QPoint center = view.viewport()->rect().center();
        if (qAbs(mapped.x() - center.x()) >= 2
            || qAbs(mapped.y() - center.y()) >= 2) {
            QString error = QString("Compared values are not the same\n\tActual: (%1, %2)\n\tExpected: (%3, %4)")
                            .arg(mapped.x()).arg(mapped.y()).arg(center.x()).arg(center.y());
            QFAIL(qPrintable(error));
        }
    }
    {
        QWidget toplevel;

        QGraphicsScene scene(0, 0, 200, 200);
        scene.addRect(QRectF(0, 0, 200, 200), QPen(Qt::black, 1));
        QGraphicsView view(&scene, &toplevel);
        view.ensurePolished();
        view.resize(view.sizeHint());
        toplevel.show();

        QCOMPARE(view.mapFromScene(0, 0), QPoint(0, 0));
        QCOMPARE(view.mapFromScene(0.4, 0.4), QPoint(0, 0));
        QCOMPARE(view.mapFromScene(0.5, 0.5), QPoint(1, 1));
        QCOMPARE(view.mapFromScene(0.9, 0.9), QPoint(1, 1));
        QCOMPARE(view.mapFromScene(1.0, 1.0), QPoint(1, 1));
        QCOMPARE(view.mapFromScene(100, 100), QPoint(100, 100));
        QCOMPARE(view.mapFromScene(100.5, 100.5), QPoint(101, 101));
        QCOMPARE(view.mapToScene(0, 0), QPointF(0, 0));
        QCOMPARE(view.mapToScene(1, 1), QPointF(1, 1));
        QCOMPARE(view.mapToScene(100, 100), QPointF(100, 100));
    }
}

void tst_QGraphicsView::mapFromSceneRect()
{
    QGraphicsScene scene;
    QWidget topLevel;
    QGraphicsView view(&scene,&topLevel);
    view.rotate(90);
    view.setFixedSize(200, 200);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QPolygon polygon;
    polygon << QPoint(98, 98);
    polygon << QPoint(98, 108);
    polygon << QPoint(88, 108);
    polygon << QPoint(88, 98);


    QPolygon viewPolygon = view.mapFromScene(0, 0, 10, 10);
    for (int i = 0; i < 4; ++i) {
        QVERIFY(qAbs(viewPolygon[i].x() - polygon[i].x()) < 3);
        QVERIFY(qAbs(viewPolygon[i].y() - polygon[i].y()) < 3);
    }

    QPoint pt = view.mapFromScene(QPointF());
    QPolygon p;
    p << pt << pt << pt << pt;
    QCOMPARE(view.mapFromScene(QRectF()), p);
}

void tst_QGraphicsView::mapFromScenePoly()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.rotate(90);
    view.setFixedSize(200, 200);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.show();

    QPolygonF polygon;
    polygon << QPoint(0, 0);
    polygon << QPoint(10, 0);
    polygon << QPoint(10, 10);
    polygon << QPoint(0, 10);

    QPolygon polygon2;
    polygon2 << QPoint(98, 98);
    polygon2 << QPoint(98, 108);
    polygon2 << QPoint(88, 108);
    polygon2 << QPoint(88, 98);

    QPolygon viewPolygon = view.mapFromScene(polygon);
    for (int i = 0; i < 4; ++i) {
        QVERIFY(qAbs(viewPolygon[i].x() - polygon2[i].x()) < 3);
        QVERIFY(qAbs(viewPolygon[i].y() - polygon2[i].y()) < 3);
    }
}

void tst_QGraphicsView::mapFromScenePath()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.rotate(90);
    view.setFixedSize(200, 200);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.show();

    QPolygonF polygon;
    polygon << QPoint(0, 0);
    polygon << QPoint(10, 0);
    polygon << QPoint(10, 10);
    polygon << QPoint(0, 10);
    QPainterPath path;
    path.addPolygon(polygon);

    QPolygon polygon2;
    polygon2 << QPoint(98, 98);
    polygon2 << QPoint(98, 108);
    polygon2 << QPoint(88, 108);
    polygon2 << QPoint(88, 98);
    QPainterPath path2;
    path2.addPolygon(polygon2);

    QPolygonF pathPoly = view.mapFromScene(path).toFillPolygon();
    QPolygonF path2Poly = path2.toFillPolygon();

    for (int i = 0; i < pathPoly.size(); ++i) {
        QVERIFY(qAbs(pathPoly[i].x() - path2Poly[i].x()) < 3);
        QVERIFY(qAbs(pathPoly[i].y() - path2Poly[i].y()) < 3);
    }
}

void tst_QGraphicsView::sendEvent()
{
    QGraphicsScene scene;

    TestItem *item = new TestItem;
    scene.addItem(item);
    item->setFlag(QGraphicsItem::ItemIsFocusable);
    item->setFlag(QGraphicsItem::ItemIsMovable);

    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&view));

    item->setFocus();

    QCOMPARE(scene.focusItem(), (QGraphicsItem *)item);
    QCOMPARE(item->events.size(), 2);
    QCOMPARE(item->events.last(), QEvent::FocusIn);

    QPoint itemPoint = view.mapFromScene(item->scenePos());
    sendMousePress(view.viewport(), itemPoint);
    QCOMPARE(item->events.size(), 4);
    QCOMPARE(item->events.at(item->events.size() - 2), QEvent::GrabMouse);
    QCOMPARE(item->events.at(item->events.size() - 1), QEvent::GraphicsSceneMousePress);

    QMouseEvent mouseMoveEvent(QEvent::MouseMove, itemPoint, view.viewport()->mapToGlobal(itemPoint),
                                Qt::LeftButton, Qt::LeftButton, 0);
    QApplication::sendEvent(view.viewport(), &mouseMoveEvent);
    QCOMPARE(item->events.size(), 5);
    QCOMPARE(item->events.last(), QEvent::GraphicsSceneMouseMove);

    QMouseEvent mouseReleaseEvent(QEvent::MouseButtonRelease, itemPoint,
                                  view.viewport()->mapToGlobal(itemPoint),
                                  Qt::LeftButton, 0, 0);
    QApplication::sendEvent(view.viewport(), &mouseReleaseEvent);
    QCOMPARE(item->events.size(), 7);
    QCOMPARE(item->events.at(item->events.size() - 2), QEvent::GraphicsSceneMouseRelease);
    QCOMPARE(item->events.at(item->events.size() - 1), QEvent::UngrabMouse);

    QTest::keyPress(view.viewport(), Qt::Key_Space);
    QCOMPARE(item->events.size(), 9);
    QCOMPARE(item->events.at(item->events.size() - 2), QEvent::ShortcutOverride);
    QCOMPARE(item->events.last(), QEvent::KeyPress);
}

#if QT_CONFIG(wheelevent)
class MouseWheelScene : public QGraphicsScene
{
public:
    Qt::Orientation orientation;

    void wheelEvent(QGraphicsSceneWheelEvent *event)
    {
        orientation = event->orientation();
        QGraphicsScene::wheelEvent(event);
    }
};

void tst_QGraphicsView::wheelEvent()
{
    // Create a scene with an invalid orientation.
    MouseWheelScene scene;
    scene.orientation = Qt::Orientation(-1);

    QGraphicsWidget *widget = new QGraphicsWidget;
    widget->setGeometry(0, 0, 400, 400);
    widget->setFocusPolicy(Qt::WheelFocus);

    EventSpy spy(widget, QEvent::GraphicsSceneWheel);
    QCOMPARE(spy.count(), 0);

    scene.addItem(widget);

    // Assign a view.
    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&view));


    // Send a wheel event with horizontal orientation.
    {
        QWheelEvent event(view.mapFromScene(widget->boundingRect().center()),
                          view.mapToGlobal(view.mapFromScene(widget->boundingRect().center())),
                          120, 0, 0, Qt::Horizontal);
        QApplication::sendEvent(view.viewport(), &event);
        QCOMPARE(scene.orientation, Qt::Horizontal);
    }

    // Send a wheel event with vertical orientation.
    {
        QWheelEvent event(view.mapFromScene(widget->boundingRect().center()),
                          view.mapToGlobal(view.mapFromScene(widget->boundingRect().center())),
                          120, 0, 0, Qt::Vertical);
        QApplication::sendEvent(view.viewport(), &event);
        QCOMPARE(scene.orientation, Qt::Vertical);
    }

    QCOMPARE(spy.count(), 2);
    QVERIFY(widget->hasFocus());
}
#endif // QT_CONFIG(wheelevent)

#ifndef QT_NO_CURSOR
void tst_QGraphicsView::cursor()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(-10, -10, 20, 20));
    item->setCursor(Qt::IBeamCursor);

    QGraphicsView view(&scene);
    view.setFixedSize(400, 400);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QCOMPARE(view.viewport()->cursor().shape(), QCursor().shape());
    view.viewport()->setCursor(Qt::PointingHandCursor);
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);

    sendMouseMove(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::IBeamCursor);

    sendMouseMove(view.viewport(), QPoint(5, 5));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);
}
#endif

#ifndef QT_NO_CURSOR
void tst_QGraphicsView::cursor2()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(-10, -10, 20, 20));
    item->setCursor(Qt::IBeamCursor);
    item->setZValue(1);

    QGraphicsItem *item2 = scene.addRect(QRectF(-20, -20, 40, 40));
    item2->setZValue(0);

    QGraphicsView view(&scene);
    view.viewport()->setCursor(Qt::PointingHandCursor);
    view.setFixedSize(400, 400);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    sendMouseMove(view.viewport(), view.mapFromScene(-30, -30));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::IBeamCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(-30, -30));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::IBeamCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(-15, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);

    view.setDragMode(QGraphicsView::ScrollHandDrag);

    sendMouseMove(view.viewport(), view.mapFromScene(-30, -30));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::OpenHandCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::IBeamCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(-15, -15));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::OpenHandCursor);

    view.setDragMode(QGraphicsView::NoDrag);
    QCOMPARE(view.viewport()->cursor().shape(), Qt::ArrowCursor);
    view.viewport()->setCursor(Qt::PointingHandCursor);
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);

    item2->setCursor(Qt::SizeAllCursor);

    sendMouseMove(view.viewport(), view.mapFromScene(-30, -30));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(-15, -15));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::SizeAllCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::IBeamCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(-15, -15));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::SizeAllCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::IBeamCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(-30, -30));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);

    view.setDragMode(QGraphicsView::ScrollHandDrag);

    sendMouseMove(view.viewport(), view.mapFromScene(-30, -30));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::OpenHandCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::IBeamCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(-15, -15));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::SizeAllCursor);
}
#endif

void tst_QGraphicsView::transformationAnchor()
{
    QGraphicsScene scene(-1000, -1000, 2000, 2000);
    scene.addRect(QRectF(-50, -50, 100, 100), QPen(Qt::black), QBrush(Qt::blue));

    QGraphicsView view(&scene);
    setFrameless(&view);

    for (int i = 0; i < 2; ++i) {
        view.resize(100, 100);
        view.show();

        if (i == 0) {
            QCOMPARE(view.transformationAnchor(), QGraphicsView::AnchorViewCenter);
        } else {
            view.setTransformationAnchor(QGraphicsView::NoAnchor);
        }
        view.centerOn(0, 0);
        view.horizontalScrollBar()->setValue(100);
        QApplication::processEvents();

        QPointF center = view.mapToScene(view.viewport()->rect().center());

        view.scale(10, 10);

        QPointF newCenter = view.mapToScene(view.viewport()->rect().center());

        if (i == 0) {
            qreal slack = 3;
            QVERIFY(qAbs(newCenter.x() - center.x()) < slack);
            QVERIFY(qAbs(newCenter.y() - center.y()) < slack);
        } else {
            qreal slack = qreal(0.3);
            QVERIFY(qAbs(newCenter.x() - center.x() / 10) < slack);
            QVERIFY(qAbs(newCenter.y() - center.y() / 10) < slack);
        }
    }
}

void tst_QGraphicsView::resizeAnchor()
{
    QGraphicsScene scene(-1000, -1000, 2000, 2000);
    scene.addRect(QRectF(-50, -50, 100, 100), QPen(Qt::black), QBrush(Qt::blue));

    QGraphicsView view(&scene);
    setFrameless(&view);

    for (int i = 0; i < 2; ++i) {
        view.resize(100, 100);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QApplication::processEvents();

        if (i == 0) {
            QCOMPARE(view.resizeAnchor(), QGraphicsView::NoAnchor);
        } else {
            view.setResizeAnchor(QGraphicsView::AnchorViewCenter);
        }
        view.centerOn(0, 0);
        QTest::qWait(25);

        QPointF f = view.mapToScene(50, 50);
        QPointF center = view.mapToScene(view.viewport()->rect().center());

        QApplication::processEvents();

        for (int size = 200; size <= 400; size += 25) {
            view.resize(size, size);
            if (i == 0) {
                QTRY_COMPARE(view.mapToScene(50, 50), f);
                QTRY_VERIFY(view.mapToScene(view.viewport()->rect().center()) != center);
            } else {
                QTRY_VERIFY(view.mapToScene(50, 50) != f);

                QPointF newCenter = view.mapToScene(view.viewport()->rect().center());
                int slack = 3;
                QVERIFY(qAbs(newCenter.x() - center.x()) < slack);
                QVERIFY(qAbs(newCenter.y() - center.y()) < slack);
            }
            QApplication::processEvents();
        }
    }
}

class CustomView : public QGraphicsView
{
    Q_OBJECT
public:
    CustomView(QGraphicsScene *s = 0) : QGraphicsView(s) {}
    CustomView(QGraphicsScene *s, QWidget *parent)
        : QGraphicsView(s, parent) {}
    QList<QRegion> lastUpdateRegions;
    bool painted;

protected:
    void paintEvent(QPaintEvent *event)
    {
        lastUpdateRegions << event->region();
        painted = true;
        QGraphicsView::paintEvent(event);
    }
};

void tst_QGraphicsView::viewportUpdateMode()
{
    QGraphicsScene scene(0, 0, 100, 100);
    scene.setBackgroundBrush(Qt::red);

    CustomView view;
    QDesktopWidget desktop;
    view.setFixedSize(QSize(500, 500).boundedTo(desktop.availableGeometry().size())); // 500 is too big for all common smartphones
    view.setScene(&scene);
    QCOMPARE(view.viewportUpdateMode(), QGraphicsView::MinimalViewportUpdate);

    // Show the view, and initialize our test.
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(!view.lastUpdateRegions.isEmpty());
    view.lastUpdateRegions.clear();

    // Issue two scene updates.
    scene.update(QRectF(0, 0, 10, 10));
    scene.update(QRectF(20, 0, 10, 10));

    // The view gets two updates for the update scene updates.
    QTRY_VERIFY(!view.lastUpdateRegions.isEmpty());
#ifndef Q_OS_MAC //cocoa doesn't support drawing regions
    QCOMPARE(view.lastUpdateRegions.last().rectCount(), 2);
    QCOMPARE(view.lastUpdateRegions.last().begin()[0].size(), QSize(14, 14));
    QCOMPARE(view.lastUpdateRegions.last().begin()[1].size(), QSize(14, 14));
#endif

    // Set full update mode.
    view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    QCOMPARE(view.viewportUpdateMode(), QGraphicsView::FullViewportUpdate);
    view.lastUpdateRegions.clear();

    // Issue two scene updates.
    scene.update(QRectF(0, 0, 10, 10));
    scene.update(QRectF(20, 0, 10, 10));
    qApp->processEvents();
    qApp->processEvents();

    // The view gets one full viewport update for the update scene updates.
    QCOMPARE(view.lastUpdateRegions.last().rectCount(), 1);
    QCOMPARE(view.lastUpdateRegions.last().begin()[0].size(), view.viewport()->size());
    view.lastUpdateRegions.clear();

    // Set smart update mode
    view.setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    QCOMPARE(view.viewportUpdateMode(), QGraphicsView::SmartViewportUpdate);

    // Issue 100 mini-updates
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            scene.update(QRectF(i * 3, j * 3, 1, 1));
        }
    }
    qApp->processEvents();
    qApp->processEvents();

    // The view gets one bounding rect update.
    QCOMPARE(view.lastUpdateRegions.last().rectCount(), 1);
    QCOMPARE(view.lastUpdateRegions.last().begin()[0].size(), QSize(32, 32));

    // Set no update mode
    view.setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    QCOMPARE(view.viewportUpdateMode(), QGraphicsView::NoViewportUpdate);

    // Issue two scene updates.
    view.lastUpdateRegions.clear();
    TestItem item;
    scene.addItem(&item);
    item.moveBy(10, 10);
    scene.update(QRectF(0, 0, 10, 10));
    scene.update(QRectF(20, 0, 10, 10));
    qApp->processEvents();
    qApp->processEvents();

    // The view should not get any painting calls from the scene updates
    QCOMPARE(view.lastUpdateRegions.size(), 0);
}

void tst_QGraphicsView::viewportUpdateMode2()
{
    QWidget toplevel;

    // Create a view with viewport rect equal to QRect(0, 0, 200, 200).
    QGraphicsScene dummyScene;
    CustomView view(0, &toplevel);
    view.painted = false;
    view.setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    view.setScene(&dummyScene);
    view.ensurePolished(); // make sure we get the right content margins
    int left, top, right, bottom;
    view.getContentsMargins(&left, &top, &right, &bottom);
    view.resize(200 + left + right, 200 + top + bottom);
    toplevel.show();
    qApp->setActiveWindow(&toplevel);
    QVERIFY(QTest::qWaitForWindowActive(&toplevel));
    QTRY_VERIFY(view.painted);
    const QRect viewportRect = view.viewport()->rect();
    QCOMPARE(viewportRect, QRect(0, 0, 200, 200));

#if defined QT_BUILD_INTERNAL
    QGraphicsViewPrivate *viewPrivate = static_cast<QGraphicsViewPrivate *>(qt_widget_private(&view));

    QRect boundingRect;
    const QRect rect1(0, 0, 10, 10);
    QVERIFY(viewPrivate->updateRect(rect1));
    QVERIFY(!viewPrivate->fullUpdatePending);
    boundingRect |= rect1;
    QCOMPARE(viewPrivate->dirtyBoundingRect, boundingRect);

    const QRect rect2(50, 50, 10, 10);
    QVERIFY(viewPrivate->updateRect(rect2));
    QVERIFY(!viewPrivate->fullUpdatePending);
    boundingRect |= rect2;
    QCOMPARE(viewPrivate->dirtyBoundingRect, boundingRect);

    const QRect rect3(190, 190, 10, 10);
    QVERIFY(viewPrivate->updateRect(rect3));
    QVERIFY(viewPrivate->fullUpdatePending);
    boundingRect |= rect3;
    QCOMPARE(viewPrivate->dirtyBoundingRect, boundingRect);

    view.lastUpdateRegions.clear();
    viewPrivate->processPendingUpdates();
    QTRY_COMPARE(view.lastUpdateRegions.size(), 1);
    // Note that we adjust by 2 for antialiasing.
    QCOMPARE(view.lastUpdateRegions.at(0), QRegion(boundingRect.adjusted(-2, -2, 2, 2) & viewportRect));
#endif
}

#if QT_CONFIG(draganddrop)
void tst_QGraphicsView::acceptDrops()
{
    QGraphicsView view;

    // Excepted default behavior.
    QVERIFY(view.acceptDrops());
    QVERIFY(view.viewport()->acceptDrops());

    // Excepted behavior with no drops.
    view.setAcceptDrops(false);
    QVERIFY(!view.acceptDrops());
    QVERIFY(!view.viewport()->acceptDrops());

    // Setting a widget with drops on a QGraphicsView without drops.
    QWidget *widget = new QWidget;
    widget->setAcceptDrops(true);
    view.setViewport(widget);
    QVERIFY(!view.acceptDrops());
    QVERIFY(!view.viewport()->acceptDrops());

    // Switching the view to accept drops.
    view.setAcceptDrops(true);
    QVERIFY(view.acceptDrops());
    QVERIFY(view.viewport()->acceptDrops());

    // Setting a widget with no drops on a QGraphicsView with drops.
    widget = new QWidget;
    widget->setAcceptDrops(false);
    view.setViewport(widget);
    QVERIFY(view.viewport()->acceptDrops());
    QVERIFY(view.acceptDrops());

    // Switching the view to not accept drops.
    view.setAcceptDrops(false);
    QVERIFY(!view.viewport()->acceptDrops());
}
#endif

void tst_QGraphicsView::optimizationFlags()
{
    QGraphicsView view;
    QVERIFY(!view.optimizationFlags());

    view.setOptimizationFlag(QGraphicsView::DontClipPainter);
    QVERIFY(view.optimizationFlags() & QGraphicsView::DontClipPainter);
    view.setOptimizationFlag(QGraphicsView::DontClipPainter, false);
    QVERIFY(!view.optimizationFlags());

    view.setOptimizationFlag(QGraphicsView::DontSavePainterState);
    QVERIFY(view.optimizationFlags() & QGraphicsView::DontSavePainterState);
    view.setOptimizationFlag(QGraphicsView::DontSavePainterState, false);
    QVERIFY(!view.optimizationFlags());

    view.setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    QVERIFY(view.optimizationFlags() & QGraphicsView::DontAdjustForAntialiasing);
    view.setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, false);
    QVERIFY(!view.optimizationFlags());

    view.setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing
                              | QGraphicsView::DontClipPainter);
    QCOMPARE(view.optimizationFlags(), QGraphicsView::OptimizationFlags(QGraphicsView::DontAdjustForAntialiasing
             | QGraphicsView::DontClipPainter));
}

class MessUpPainterItem : public QGraphicsRectItem
{
public:
    MessUpPainterItem(const QRectF &rect) : QGraphicsRectItem(rect), dirtyPainter(false)
    { }

    bool dirtyPainter;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *w)
    {
        dirtyPainter = (painter->pen().color() != w->palette().color(w->foregroundRole()));
        painter->setPen(Qt::red);
    }
};

class MyGraphicsView : public QGraphicsView
{
public:
      MyGraphicsView(QGraphicsScene * scene) : QGraphicsView(scene)
      { }

      void drawBackground(QPainter * painter, const QRectF & rect) {
          painter->setCompositionMode(QPainter::CompositionMode_Source);
          painter->drawRect(rect);
      }

      void drawItems (QPainter * painter, int numItems, QGraphicsItem *items[], const QStyleOptionGraphicsItem options[]) {
           if (!(optimizationFlags() & QGraphicsView::DontSavePainterState))
               QCOMPARE(painter->compositionMode(),QPainter::CompositionMode_SourceOver);
           else
               QCOMPARE(painter->compositionMode(),QPainter::CompositionMode_Source);
           QGraphicsView::drawItems(painter,numItems,items,options);
      }
};

void tst_QGraphicsView::optimizationFlags_dontSavePainterState()
{
    MessUpPainterItem *parent = new MessUpPainterItem(QRectF(0, 0, 100, 100));
    MessUpPainterItem *child = new MessUpPainterItem(QRectF(0, 0, 100, 100));
    child->setParentItem(parent);

    QGraphicsScene scene;
    scene.addItem(parent);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.viewport()->repaint();

    QVERIFY(!parent->dirtyPainter);
    QVERIFY(!child->dirtyPainter);

    view.setOptimizationFlags(QGraphicsView::DontSavePainterState);
    view.viewport()->repaint();

#ifdef Q_OS_MAC
    // Repaint on OS X actually does require spinning the event loop.
    QTest::qWait(100);
#endif
    QVERIFY(!parent->dirtyPainter);
    QVERIFY(child->dirtyPainter);

    MyGraphicsView painter(&scene);
    painter.show();
    QVERIFY(QTest::qWaitForWindowExposed(&painter));

    MyGraphicsView painter2(&scene);
    painter2.setOptimizationFlag(QGraphicsView::DontSavePainterState,true);
    painter2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&painter2));
}

void tst_QGraphicsView::optimizationFlags_dontSavePainterState2_data()
{
    QTest::addColumn<bool>("savePainter");
    QTest::addColumn<bool>("indirectPainting");
    QTest::newRow("With painter state protection, without indirect painting") << true << false;
    QTest::newRow("Without painter state protection, without indirect painting") << false << false;
    QTest::newRow("With painter state protectionm, with indirect painting") << true << true;
    QTest::newRow("Without painter state protection, with indirect painting") << false << true;
}

void tst_QGraphicsView::optimizationFlags_dontSavePainterState2()
{
    QFETCH(bool, savePainter);
    QFETCH(bool, indirectPainting);

    class MyScene : public QGraphicsScene
    {
    public:
        void drawBackground(QPainter *p, const QRectF &)
        { transformInDrawBackground = p->worldTransform(); opacityInDrawBackground = p->opacity(); }

        void drawForeground(QPainter *p, const QRectF &)
        { transformInDrawForeground = p->worldTransform(); opacityInDrawForeground = p->opacity(); }

        QTransform transformInDrawBackground;
        QTransform transformInDrawForeground;
        qreal opacityInDrawBackground;
        qreal opacityInDrawForeground;
    };

    MyScene scene;
    // Add transformed dummy items to make sure the painter's worldTransform() is changed in drawItems.
    QGraphicsRectItem *rectA = scene.addRect(0, 0, 20, 20);
    QGraphicsRectItem *rectB = scene.addRect(50, 50, 20, 20);

    rectA->setTransform(QTransform::fromScale(2, 2));
    rectA->setPen(QPen(Qt::black, 0));
    rectB->setTransform(QTransform::fromTranslate(200, 200));
    rectB->setPen(QPen(Qt::black, 0));

    foreach (QGraphicsItem *item, scene.items())
        item->setOpacity(0.6);

    CustomView view(&scene);
    if (!savePainter)
        view.setOptimizationFlag(QGraphicsView::DontSavePainterState);
    view.setOptimizationFlag(QGraphicsView::IndirectPainting, indirectPainting);
    view.rotate(45);
    view.scale(1.5, 1.5);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // Make sure the view is repainted; otherwise the tests below will fail.
    view.viewport()->repaint();
    QTRY_VERIFY(view.painted);

    // Make sure the painter's world transform is preserved after drawItems.
    QTransform expectedTransform = view.viewportTransform();
    QVERIFY(!expectedTransform.isIdentity());
    QCOMPARE(scene.transformInDrawForeground, expectedTransform);
    QCOMPARE(scene.transformInDrawBackground, expectedTransform);

    qreal expectedOpacity = 1.0;
    QCOMPARE(scene.opacityInDrawBackground, expectedOpacity);
    QCOMPARE(scene.opacityInDrawForeground, expectedOpacity);

    // Trigger more painting, this time from QGraphicsScene::render.
    QImage image(scene.sceneRect().size().toSize(), QImage::Format_RGB32);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();

    expectedTransform = QTransform();
    QCOMPARE(scene.transformInDrawForeground, expectedTransform);
    QCOMPARE(scene.transformInDrawBackground, expectedTransform);
    QCOMPARE(scene.opacityInDrawBackground, expectedOpacity);
    QCOMPARE(scene.opacityInDrawForeground, expectedOpacity);

    // Trigger more painting with another opacity on the painter.
    painter.begin(&image);
    painter.setOpacity(0.4);
    expectedOpacity = 0.4;
    scene.render(&painter);
    painter.end();

    QCOMPARE(scene.transformInDrawForeground, expectedTransform);
    QCOMPARE(scene.transformInDrawBackground, expectedTransform);
    QCOMPARE(scene.opacityInDrawBackground, expectedOpacity);
    QCOMPARE(scene.opacityInDrawForeground, expectedOpacity);
}

class LodItem : public QGraphicsRectItem
{
public:
    LodItem(const QRectF &rect) : QGraphicsRectItem(rect), lastLod(-42)
    { }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *viewport)
    {
        lastLod = option->levelOfDetailFromTransform(painter->worldTransform());
        QGraphicsRectItem::paint(painter, option, viewport);
    }

    qreal lastLod;
};

void tst_QGraphicsView::levelOfDetail_data()
{
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<qreal>("lod");

    QTest::newRow("1:4, 1:4") << QTransform().scale(0.25, 0.25) << qreal(0.25);
    QTest::newRow("1:2, 1:4") << QTransform().scale(0.5, 0.25) << qreal(::sqrt(0.125));
    QTest::newRow("4:1, 1:2") << QTransform().scale(0.25, 0.5) << qreal(::sqrt(0.125));

    QTest::newRow("1:2, 1:2") << QTransform().scale(0.5, 0.5) << qreal(0.5);
    QTest::newRow("1:1, 1:2") << QTransform().scale(1, 0.5) << qreal(::sqrt(0.5));
    QTest::newRow("2:1, 1:1") << QTransform().scale(0.5, 1) << qreal(::sqrt(0.5));

    QTest::newRow("1:1, 1:1") << QTransform().scale(1, 1) << qreal(1.0);
    QTest::newRow("2:1, 1:1") << QTransform().scale(2, 1) << qreal(::sqrt(2.0));
    QTest::newRow("1:1, 2:1") << QTransform().scale(2, 1) << qreal(::sqrt(2.0));
    QTest::newRow("2:1, 2:1") << QTransform().scale(2, 2) << qreal(2.0);
    QTest::newRow("2:1, 4:1") << QTransform().scale(2, 4) << qreal(::sqrt(8.0));
    QTest::newRow("4:1, 2:1") << QTransform().scale(4, 2) << qreal(::sqrt(8.0));
    QTest::newRow("4:1, 4:1") << QTransform().scale(4, 4) << qreal(4.0);
}

void tst_QGraphicsView::levelOfDetail()
{
    QFETCH(QTransform, transform);
    QFETCH(qreal, lod);

    LodItem *item = new LodItem(QRectF(0, 0, 100, 100));

    QGraphicsScene scene;
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QTRY_COMPARE(item->lastLod, qreal(1));

    view.setTransform(transform);

    QTRY_COMPARE(item->lastLod, lod);
}

// Moved to tst_qgraphicsview_2.cpp
extern void _scrollBarRanges_data();

void tst_QGraphicsView::scrollBarRanges_data()
{
    _scrollBarRanges_data();
}

// Simulates motif scrollbar for range tests
class FauxMotifStyle : public QCommonStyle {
public:
    int styleHint(StyleHint hint, const QStyleOption *option,
                  const QWidget *widget, QStyleHintReturn *returnData) const {
        if (hint == QStyle::SH_ScrollView_FrameOnlyAroundContents)
            return true;
        return QCommonStyle::styleHint(hint, option, widget, returnData);
    }

    int pixelMetric(PixelMetric m, const QStyleOption *opt, const QWidget *widget) const {
        if (m == QStyle::PM_ScrollView_ScrollBarSpacing)
            return 4;
        return QCommonStyle::pixelMetric(m, opt, widget);
    }
};

void tst_QGraphicsView::scrollBarRanges()
{
    QFETCH(QByteArray, style);
    QFETCH(QSize, viewportSize);
    QFETCH(QRectF, sceneRect);
    QFETCH(ScrollBarCount, sceneRectOffsetFactors);
    QFETCH(QTransform, transform);
    QFETCH(Qt::ScrollBarPolicy, hbarpolicy);
    QFETCH(Qt::ScrollBarPolicy, vbarpolicy);
    QFETCH(ExpectedValueDescription, hmin);
    QFETCH(ExpectedValueDescription, hmax);
    QFETCH(ExpectedValueDescription, vmin);
    QFETCH(ExpectedValueDescription, vmax);
    QFETCH(bool, useStyledPanel);

    if (useStyledPanel && style == "macintosh" && platformName == QStringLiteral("cocoa"))
        QSKIP("Insignificant on OSX");

    QScopedPointer<QStyle> stylePtr;

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setRenderHint(QPainter::Antialiasing);
    view.setTransform(transform);
    view.setFrameStyle(useStyledPanel ? QFrame::StyledPanel : QFrame::NoFrame);

    if (style == "motif")
        stylePtr.reset(new FauxMotifStyle);
    else
        stylePtr.reset(QStyleFactory::create(QLatin1String(style)));
    view.setStyle(stylePtr.data());
    view.setStyleSheet(" "); // enables style propagation ;-)

    int adjust = 0;
    if (useStyledPanel)
        adjust = view.style()->pixelMetric(QStyle::PM_DefaultFrameWidth) * 2;
    view.resize(viewportSize + QSize(adjust, adjust));

    view.setHorizontalScrollBarPolicy(hbarpolicy);
    view.setVerticalScrollBarPolicy(vbarpolicy);

    view.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const int offset = view.style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, 0);

    QRectF actualSceneRect;
    actualSceneRect.setLeft(sceneRect.left() + sceneRectOffsetFactors.left * offset);
    actualSceneRect.setWidth(sceneRect.width() + sceneRectOffsetFactors.right * offset);
    actualSceneRect.setTop(sceneRect.top() + sceneRectOffsetFactors.top * offset);
    actualSceneRect.setHeight(sceneRect.height() + sceneRectOffsetFactors.bottom * offset);
    scene.setSceneRect(actualSceneRect);
    scene.addRect(actualSceneRect, QPen(Qt::blue), QBrush(QColor(Qt::green)));

    int expectedHmin = hmin.value + hmin.scrollBarExtentsToAdd * offset;
    int expectedVmin = vmin.value + vmin.scrollBarExtentsToAdd * offset;
    int expectedHmax = hmax.value + hmax.scrollBarExtentsToAdd * offset;
    int expectedVmax = vmax.value + vmax.scrollBarExtentsToAdd* offset;
    if (useStyledPanel && view.style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents)) {
        int spacing = view.style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing);
        expectedHmin += hmin.spacingsToAdd * spacing;
        expectedVmin += vmin.spacingsToAdd * spacing;
        expectedHmax += hmax.spacingsToAdd * spacing;
        expectedVmax += vmax.spacingsToAdd * spacing;
    }
    QCOMPARE(view.horizontalScrollBar()->minimum(), expectedHmin);
    QCOMPARE(view.verticalScrollBar()->minimum(), expectedVmin);
    QCOMPARE(view.horizontalScrollBar()->maximum(), expectedHmax);
    QCOMPARE(view.verticalScrollBar()->maximum(), expectedVmax);
}

class TestView : public QGraphicsView
{
public:
    TestView(QGraphicsScene *scene)
        : QGraphicsView(scene), pressAccepted(false), doubleClickAccepted(false)
    { }

    bool pressAccepted;
    bool doubleClickAccepted;

protected:
    void mousePressEvent(QMouseEvent *event)
    {
        QGraphicsView::mousePressEvent(event);
        pressAccepted = event->isAccepted();
    }
    void mouseDoubleClickEvent(QMouseEvent *event)
    {
        QGraphicsView::mouseDoubleClickEvent(event);
        doubleClickAccepted = event->isAccepted();
    }
};

void tst_QGraphicsView::acceptMousePressEvent()
{
    QGraphicsScene scene;

    TestView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QTest::mouseClick(view.viewport(), Qt::LeftButton);
    QVERIFY(!view.pressAccepted);

    QSignalSpy spy(&scene, &QGraphicsScene::changed);
    scene.addRect(0, 0, 2000, 2000)->setFlag(QGraphicsItem::ItemIsMovable);
    QVERIFY(spy.wait());

    QTest::mouseClick(view.viewport(), Qt::LeftButton);
    QVERIFY(view.pressAccepted);
}

void tst_QGraphicsView::acceptMouseDoubleClickEvent()
{
    QGraphicsScene scene;

    TestView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QTest::mouseDClick(view.viewport(), Qt::LeftButton);
    QVERIFY(!view.doubleClickAccepted);

    QSignalSpy spy(&scene, &QGraphicsScene::changed);
    scene.addRect(0, 0, 2000, 2000)->setFlag(QGraphicsItem::ItemIsMovable);
    QVERIFY(spy.wait());

    QTest::mouseDClick(view.viewport(), Qt::LeftButton);
    QVERIFY(view.doubleClickAccepted);
}

class TestWidget : public QWidget
{
public:
    TestWidget()
        : QWidget(), pressForwarded(false), doubleClickForwarded(false)
    { }

    bool pressForwarded;
    bool doubleClickForwarded;

protected:
    void mousePressEvent(QMouseEvent *event)
    {
        QWidget::mousePressEvent(event);
        pressForwarded = true;
    }
    void mouseDoubleClickEvent(QMouseEvent *event)
    {
        QWidget::mouseDoubleClickEvent(event);
        doubleClickForwarded = true;
    }
};

void tst_QGraphicsView::forwardMousePress()
{
    TestWidget widget;
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QHBoxLayout layout;
    widget.setLayout(&layout);
    layout.addWidget(&view);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    widget.pressForwarded = false;
    QTest::mouseClick(view.viewport(), Qt::LeftButton);
    QVERIFY(widget.pressForwarded);

    scene.addRect(0, 0, 2000, 2000);

    qApp->processEvents(); // ensure scene rect is updated

    widget.pressForwarded = false;
    QTest::mouseClick(view.viewport(), Qt::LeftButton);
    QVERIFY(widget.pressForwarded);
}

void tst_QGraphicsView::forwardMouseDoubleClick()
{
    TestWidget widget;
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QHBoxLayout layout;
    widget.setLayout(&layout);
    layout.addWidget(&view);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    widget.doubleClickForwarded = false;
    QTest::mouseDClick(view.viewport(), Qt::LeftButton);
    QVERIFY(widget.doubleClickForwarded);

    scene.addRect(0, 0, 2000, 2000);

    qApp->processEvents(); // ensure scene rect is updated

    widget.doubleClickForwarded = false;
    QTest::mouseDClick(view.viewport(), Qt::LeftButton);
    QVERIFY(widget.doubleClickForwarded);
}

void tst_QGraphicsView::replayMouseMove()
{
    // An empty scene in a view. The view will send the events to the scene in
    // any case. Note that the view doesn't have to be shown - the mouse event
    // sending functions below send the events directly to the viewport.
    QGraphicsScene scene(-10000, -10000, 20000, 20000);
    QGraphicsView view(&scene);

    EventSpy sceneSpy(&scene, QEvent::GraphicsSceneMouseMove);
    EventSpy viewSpy(view.viewport(), QEvent::MouseMove);

    sendMousePress(view.viewport(), view.viewport()->rect().center());

    // One mouse event should be translated into one scene event.
    for (int i = 0; i < 3; ++i) {
        sendMouseMove(view.viewport(), view.viewport()->rect().center(),
                      Qt::LeftButton, Qt::MouseButtons(Qt::LeftButton));
        QCOMPARE(viewSpy.count(), i + 1);
        QCOMPARE(sceneSpy.count(), i + 1);
    }

    // When the view is transformed, the view should get no more events.  But
    // the scene should get replays.
    for (int i = 0; i < 3; ++i) {
        view.rotate(10);
        QCOMPARE(viewSpy.count(), 3);
        QCOMPARE(sceneSpy.count(), 3 + i + 1);
    }

    // When the view is scrolled, the view should get no more events.  But the
    // scene should get replays.
    for (int i = 0; i < 3; ++i) {
        view.horizontalScrollBar()->setValue((i + 1) * 10);
        QCOMPARE(viewSpy.count(), 3);
        QCOMPARE(sceneSpy.count(), 6 + i + 1);
    }
}

void tst_QGraphicsView::itemsUnderMouse()
{
   QGraphicsScene scene;
   QGraphicsProxyWidget w;
   w.setWidget(new QPushButton("W"));
   w.resize(50,50);
   QGraphicsProxyWidget w2(&w);
   w2.setWidget(new QPushButton("W2"));
   w2.resize(50,50);
   QGraphicsProxyWidget w3(&w2);
   w3.setWidget(new QPushButton("W3"));
   w3.resize(50,50);
   w.setZValue(150);
   w2.setZValue(50);
   w3.setZValue(0);
   scene.addItem(&w);

   QGraphicsView view(&scene);
   view.show();
   QVERIFY(QTest::qWaitForWindowExposed(&view));

   QCOMPARE(view.items(view.mapFromScene(w3.boundingRect().center())).first(),
            static_cast<QGraphicsItem *>(&w3));
   w2.setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
   QCOMPARE(view.items(view.mapFromScene(w3.boundingRect().center())).first(),
            static_cast<QGraphicsItem *>(&w3));
}

class QGraphicsTextItem_task172231 : public QGraphicsTextItem
{
public:
    QGraphicsTextItem_task172231(const QString & text, QGraphicsItem * parent = 0)
        : QGraphicsTextItem(text, parent) {}
    QRectF exposedRect;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        exposedRect = option->exposedRect;
        QGraphicsTextItem::paint(painter, option, widget);
    }
};

void tst_QGraphicsView::task172231_untransformableItems()
{
    // check fix in QGraphicsView::paintEvent()

    QGraphicsScene scene;

    QGraphicsTextItem_task172231 *text =
        new QGraphicsTextItem_task172231("abcdefghijklmnopqrstuvwxyz");
    text->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene.addItem(text);

    QGraphicsView view(&scene);

    view.scale(2, 1);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&view));

    QRectF origExposedRect = text->exposedRect;

    view.resize(int(0.75 * view.width()), view.height());
    qApp->processEvents();

    QCOMPARE(text->exposedRect, origExposedRect);

    // notice that the fix also goes into QGraphicsView::render()
    // and QGraphicsScene::render(), but in duplicated code that
    // is pending a refactoring, so for now we omit autotesting
    // these functions separately
}

class MousePressReleaseScene : public QGraphicsScene
{
public:
    MousePressReleaseScene()
        : presses(0), releases(0)
    { }
    int presses;
    int releases;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event)
    { ++presses; QGraphicsScene::mousePressEvent(event); }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    { ++releases; QGraphicsScene::mouseReleaseEvent(event); }
};

void tst_QGraphicsView::task180429_mouseReleaseDragMode()
{
    MousePressReleaseScene scene;

    QGraphicsView view(&scene);
    view.show();

    sendMousePress(view.viewport(), view.viewport()->rect().center());
    QCOMPARE(scene.presses, 1);
    QCOMPARE(scene.releases, 0);
    sendMouseRelease(view.viewport(), view.viewport()->rect().center());
    QCOMPARE(scene.presses, 1);
    QCOMPARE(scene.releases, 1);

    view.setDragMode(QGraphicsView::RubberBandDrag);
    sendMousePress(view.viewport(), view.viewport()->rect().center());
    QCOMPARE(scene.presses, 2);
    QCOMPARE(scene.releases, 1);
    sendMouseRelease(view.viewport(), view.viewport()->rect().center());
    QCOMPARE(scene.presses, 2);
    QCOMPARE(scene.releases, 2);
}

void tst_QGraphicsView::task187791_setSceneCausesUpdate()
{
    QGraphicsScene scene(0, 0, 200, 200);
    QGraphicsView view(&scene);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    EventSpy updateSpy(view.viewport(), QEvent::Paint);
    QCOMPARE(updateSpy.count(), 0);

    view.setScene(0);
    QApplication::processEvents();
    QTRY_COMPARE(updateSpy.count(), 1);
    view.setScene(&scene);
    QApplication::processEvents();
    QTRY_COMPARE(updateSpy.count(), 2);
}

class MouseMoveCounter : public QGraphicsView
{
public:
    MouseMoveCounter() : mouseMoves(0)
    { }
    int mouseMoves;
protected:
    void mouseMoveEvent(QMouseEvent *event)
    {
        ++mouseMoves;
        QGraphicsView::mouseMoveEvent(event);
        foreach (QGraphicsItem *item, scene()->items()) {
            scene()->removeItem(item);
            delete item;
        }
        scene()->addRect(0, 0, 50, 50);
        scene()->addRect(0, 0, 100, 100);
    }
};

void tst_QGraphicsView::task186827_deleteReplayedItem()
{
    // make sure the mouse is not over the window, causing spontaneous mouse moves
    QCursor::setPos(1, 1);

    QGraphicsScene scene;
    scene.addRect(0, 0, 50, 50);
    scene.addRect(0, 0, 100, 100);

    MouseMoveCounter view;
    view.setScene(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.viewport()->setMouseTracking(true);

    QCOMPARE(view.mouseMoves, 0);
    {
        QMouseEvent event(QEvent::MouseMove, view.mapFromScene(25, 25), Qt::NoButton, 0, 0);
        QApplication::sendEvent(view.viewport(), &event);
    }
    QCOMPARE(view.mouseMoves, 1);
    QTest::qWait(25);
    QTRY_COMPARE(view.mouseMoves, 1);
    QTest::qWait(25);
    {
        QMouseEvent event(QEvent::MouseMove, view.mapFromScene(25, 25), Qt::NoButton, 0, 0);
        QApplication::sendEvent(view.viewport(), &event);
    }
    QCOMPARE(view.mouseMoves, 2);
    QTest::qWait(15);
}

void tst_QGraphicsView::task207546_focusCrash()
{
    class _Widget : public QWidget
    {
    public:
        bool focusNextPrevChild(bool next) { return QWidget::focusNextPrevChild(next); }
    } widget;

    widget.setLayout(new QVBoxLayout());
    QGraphicsView *gr1 = new QGraphicsView(&widget);
    QGraphicsView *gr2 = new QGraphicsView(&widget);
    widget.layout()->addWidget(gr1);
    widget.layout()->addWidget(gr2);
    widget.show();
    widget.activateWindow();
    QApplication::setActiveWindow(&widget);
    QVERIFY(QTest::qWaitForWindowActive(&widget));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&widget));
    widget.focusNextPrevChild(true);
    QCOMPARE(static_cast<QWidget *>(gr2), widget.focusWidget());
}

void tst_QGraphicsView::task210599_unsetDragWhileDragging()
{
    QGraphicsScene scene(0, 0, 400, 400);
    QGraphicsView view(&scene);
    view.setGeometry(0, 0, 200, 200);
    view.show();

    QPoint origPos = QPoint(100, 100);
    QPoint step1Pos = QPoint(100, 110);
    QPoint step2Pos = QPoint(100, 120);

    // Enable and do a drag
    {
        view.setDragMode(QGraphicsView::ScrollHandDrag);
        QMouseEvent press(QEvent::MouseButtonPress, origPos, Qt::LeftButton, 0, 0);
        QMouseEvent move(QEvent::MouseMove, step1Pos, Qt::LeftButton, 0, 0);
        QApplication::sendEvent(view.viewport(), &press);
        QApplication::sendEvent(view.viewport(), &move);
    }

    // unset drag and release mouse, inverse order
    {
        view.setDragMode(QGraphicsView::NoDrag);
        QMouseEvent release(QEvent::MouseButtonRelease, step1Pos, Qt::LeftButton, 0, 0);
        QApplication::sendEvent(view.viewport(), &release);
    }

    QPoint basePos = view.mapFromScene(0, 0);

    // reset drag, and move mouse without holding button down.
    {
        view.setDragMode(QGraphicsView::ScrollHandDrag);
        QMouseEvent move(QEvent::MouseMove, step2Pos, Qt::LeftButton, 0, 0);
        QApplication::sendEvent(view.viewport(), &move);
    }

    // Check that no draggin has occurred...
    QCOMPARE(basePos, view.mapFromScene(0, 0));
}

class ChangedListener : public QObject
{
    Q_OBJECT
public:
    QList<QList<QRectF> > changes;

public slots:
    void changed(const QList<QRectF> &dirty)
    {
        changes << dirty;
    }
};

void tst_QGraphicsView::task239729_noViewUpdate_data()
{
    QTest::addColumn<bool>("a");

    QTest::newRow("a") << false;
    QTest::newRow("b") << true;
}

void tst_QGraphicsView::task239729_noViewUpdate()
{
    QFETCH(bool, a);
    // The scene's changed signal is connected to something that isn't a view.
    QGraphicsScene scene;
    ChangedListener cl;
    QGraphicsView *view = 0;

    if (a) {
        view = new QGraphicsView(&scene);
        connect(&scene, SIGNAL(changed(QList<QRectF>)), &cl, SLOT(changed(QList<QRectF>)));
    } else {
        connect(&scene, SIGNAL(changed(QList<QRectF>)), &cl, SLOT(changed(QList<QRectF>)));
        view = new QGraphicsView(&scene);
    }

    EventSpy spy(view->viewport(), QEvent::Paint);
    QCOMPARE(spy.count(), 0);

    view->show();
    qApp->setActiveWindow(view);
    QVERIFY(QTest::qWaitForWindowActive(view));

    QTRY_VERIFY(spy.count() >= 1);
    spy.reset();
    scene.update();
    QApplication::processEvents();
    QTRY_COMPARE(spy.count(), 1);

    delete view;
}

void tst_QGraphicsView::task239047_fitInViewSmallViewport()
{
    // Ensure that with a small viewport, fitInView doesn't mirror the
    // scene.
    QWidget widget;
    setFrameless(&widget);
    QGraphicsScene scene;
    QGraphicsView *view = new QGraphicsView(&scene, &widget);
    view->resize(3, 3);
    QCOMPARE(view->size(), QSize(3, 3));
    widget.show();
    view->fitInView(0, 0, 100, 100);
    QPointF topLeft = view->mapToScene(0, 0);
    QPointF bottomRight = view->mapToScene(100, 100);
    QVERIFY(bottomRight.x() > topLeft.x());
    QVERIFY(bottomRight.y() > topLeft.y());

    view->fitInView(0, 0, 0, 100);

    // Don't crash
    view->scale(0, 0);
    view->fitInView(0, 0, 100, 100);
}

void tst_QGraphicsView::task245469_itemsAtPointWithClip()
{
    QGraphicsScene scene;
    QGraphicsItem *parent = scene.addRect(0, 0, 100, 100);
    QGraphicsItem *child = new QGraphicsRectItem(40, 40, 20, 20, parent);
    parent->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    QGraphicsView view(&scene);
    view.resize(150,150);
    view.rotate(90);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QList<QGraphicsItem *> itemsAtCenter = view.items(view.viewport()->rect().center());
    QCOMPARE(itemsAtCenter, (QList<QGraphicsItem *>() << child << parent));

    QPolygonF p = view.mapToScene(QRect(view.viewport()->rect().center(), QSize(1, 1)));
    QList<QGraphicsItem *> itemsAtCenter2 = scene.items(p);
    QCOMPARE(itemsAtCenter2, itemsAtCenter);
}

static QGraphicsView *createSimpleViewAndScene()
{
    QGraphicsView *view = new QGraphicsView;
    QGraphicsScene *scene = new QGraphicsScene(view);
    view->setScene(scene);

    view->setBackgroundBrush(Qt::blue);

    QGraphicsRectItem *rect = scene->addRect(0, 0, 10, 10);
    rect->setBrush(Qt::red);
    rect->setPen(Qt::NoPen);
    return view;
}

class SpyItem : public QGraphicsRectItem
{
public:
    SpyItem()
        : QGraphicsRectItem(QRectF(0, 0, 100, 100))
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
    {
        transform = painter->transform();
    }

    QTransform transform;
};

void tst_QGraphicsView::embeddedViews()
{
    QGraphicsView *v1 = createSimpleViewAndScene();
    QGraphicsView *v2 = createSimpleViewAndScene();

    QGraphicsProxyWidget *proxy = v1->scene()->addWidget(v2);

    SpyItem *item = new SpyItem;
    v2->scene()->addItem(item);

    proxy->setTransform(QTransform::fromTranslate(5, 5), true);

    QImage actual(64, 64, QImage::Format_ARGB32_Premultiplied);
    actual.fill(0);
    v1->QWidget::render(&actual);
    QTransform a = item->transform;

    v2->QWidget::render(&actual);
    QTransform b = item->transform;

    QCOMPARE(a, b);
    delete v1;
}

void tst_QGraphicsView::scrollAfterResize_data()
{
    QTest::addColumn<bool>("reverse");
    QTest::addColumn<QTransform>("x1");
    QTest::addColumn<QTransform>("x2");
    QTest::addColumn<QTransform>("x3");

    QStyle *style = QStyleFactory::create("windows");

    int frameWidth = style->pixelMetric(QStyle::PM_DefaultFrameWidth);
    int extent = style->pixelMetric(QStyle::PM_ScrollBarExtent);
    int inside = style->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents);
    int viewportWidth = 300;
    int scrollBarIndent = viewportWidth - extent - (inside ? 4 : 2)*frameWidth;

    QTest::newRow("normal") << false
                            << QTransform()
                            << QTransform()
                            << QTransform().translate(-10, 0);
    QTest::newRow("reverse") << true
                             << QTransform().translate(scrollBarIndent, 0)
                             << QTransform().translate(scrollBarIndent + 100, 0)
                             << QTransform().translate(scrollBarIndent + 110, 0);
    delete style;
}

void tst_QGraphicsView::scrollAfterResize()
{
    QFETCH(bool, reverse);
    QFETCH(QTransform, x1);
    QFETCH(QTransform, x2);
    QFETCH(QTransform, x3);

    QStyle *style = QStyleFactory::create("windows");
    QWidget toplevel;

    QGraphicsView view(&toplevel);
    view.setStyle(style);
    if (reverse)
        view.setLayoutDirection(Qt::RightToLeft);

    view.setSceneRect(-1000, -1000, 2000, 2000);
    view.resize(300, 300);
    toplevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toplevel));
    view.horizontalScrollBar()->setValue(0);
    view.verticalScrollBar()->setValue(0);
    QCOMPARE(view.viewportTransform(), x1);
    view.resize(400, 300);
    QCOMPARE(view.viewportTransform(), x2);
    view.horizontalScrollBar()->setValue(10);
    QCOMPARE(view.viewportTransform(), x3);
    delete style;
}

void tst_QGraphicsView::moveItemWhileScrolling_data()
{
    QTest::addColumn<bool>("adjustForAntialiasing");
    QTest::addColumn<bool>("changedConnected");

    QTest::newRow("no adjust") << false << false;
    QTest::newRow("adjust") << true << false;
    QTest::newRow("no adjust changedConnected") << false << true;
    QTest::newRow("adjust changedConnected") << true << true;
}

void tst_QGraphicsView::moveItemWhileScrolling()
{
    QFETCH(bool, adjustForAntialiasing);
    QFETCH(bool, changedConnected);

    class MoveItemScrollView : public QGraphicsView
    {
    public:
        MoveItemScrollView()
        {
            setWindowFlags(Qt::X11BypassWindowManagerHint);
            setScene(new QGraphicsScene(0, 0, 1000, 1000, this));
            rect = scene()->addRect(0, 0, 10, 10);
            rect->setPos(50, 50);
            rect->setPen(QPen(Qt::black, 0));
            painted = false;
        }
        QRegion lastPaintedRegion;
        QGraphicsRectItem *rect;
        bool painted;
        void waitForPaintEvent()
        {
            QTimer::singleShot(2000, &eventLoop, SLOT(quit()));
            eventLoop.exec();
        }
    protected:
        QEventLoop eventLoop;
        void paintEvent(QPaintEvent *event)
        {
            painted = true;
            lastPaintedRegion = event->region();
            QGraphicsView::paintEvent(event);
            if (eventLoop.isRunning())
                eventLoop.quit();
        }
    };

    MoveItemScrollView view;
    view.setFrameStyle(0);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setResizeAnchor(QGraphicsView::NoAnchor);
    view.setTransformationAnchor(QGraphicsView::NoAnchor);
    if (!adjustForAntialiasing)
        view.setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    view.resize(200, 200);
    view.painted = false;
    view.showNormal();
    if (changedConnected)
        QObject::connect(view.scene(), SIGNAL(changed(QList<QRectF>)), this, SLOT(dummySlot()));
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QApplication::processEvents();
    QTRY_VERIFY(view.painted);
    view.painted = false;
    view.lastPaintedRegion = QRegion();
    view.horizontalScrollBar()->setValue(view.horizontalScrollBar()->value() + 10);
    view.rect->moveBy(0, 10);
    view.waitForPaintEvent();
    QTRY_VERIFY(view.painted);

    QRegion expectedRegion;
    expectedRegion += QRect(0, 0, 200, 200);
    expectedRegion -= QRect(0, 0, 190, 200);
    int a = adjustForAntialiasing ? 2 : 1;
    expectedRegion += QRect(40, 50, 10, 10).adjusted(-a, -a, a, a);
    expectedRegion += QRect(40, 60, 10, 10).adjusted(-a, -a, a, a);
    COMPARE_REGIONS(view.lastPaintedRegion, expectedRegion);
}

void tst_QGraphicsView::centerOnDirtyItem()
{
    QWidget toplevel;

    QGraphicsView view(&toplevel);
    toplevel.setWindowFlags(view.windowFlags() | Qt::WindowStaysOnTopHint);
    view.resize(200, 200);

    QGraphicsScene *scene = new QGraphicsScene(&view);
    view.setScene(scene);
    view.setSceneRect(-1000, -1000, 2000, 2000);

    QGraphicsRectItem *item = new QGraphicsRectItem(0, 0, 10, 10);
    item->setBrush(Qt::red);
    scene->addItem(item);
    view.centerOn(item);

    toplevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toplevel));

    QImage before(view.viewport()->size(), QImage::Format_ARGB32);
    view.viewport()->render(&before);

    item->setPos(20, 0);
    view.centerOn(item);

    QImage after(view.viewport()->size(), QImage::Format_ARGB32);
    view.viewport()->render(&after);

    QCOMPARE(before, after);
}

void tst_QGraphicsView::mouseTracking()
{
    // Mouse tracking should only be automatically enabled if items either accept hover events
    // or have a cursor set. We never disable mouse tracking if it is already enabled.

    { // Make sure mouse tracking is disabled by default.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsView view(&scene);
        QVERIFY(!view.viewport()->hasMouseTracking());
    }

    { // Make sure we don't disable mouse tracking in setupViewport/setScene.
        QGraphicsView view;
        QWidget *viewport = new QWidget;
        viewport->setMouseTracking(true);
        view.setViewport(viewport);
        QVERIFY(viewport->hasMouseTracking());

        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        view.setScene(&scene);
        QVERIFY(viewport->hasMouseTracking());
    }

    // Make sure we enable mouse tracking when having items that accept hover events.
    {
        // Adding an item to the scene after the scene is set on the view.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsView view(&scene);

        QGraphicsRectItem *item = new QGraphicsRectItem(10, 10, 10, 10);
        item->setAcceptHoverEvents(true);
        scene.addItem(item);
        QVERIFY(view.viewport()->hasMouseTracking());
    }
    {
        // Adding an item to the scene before the scene is set on the view.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsRectItem *item = new QGraphicsRectItem(10, 10, 10, 10);
        item->setAcceptHoverEvents(true);
        scene.addItem(item);

        QGraphicsView view(&scene);
        QVERIFY(view.viewport()->hasMouseTracking());
    }
    {
        // QGraphicsWidget implicitly accepts hover if it has window decoration.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsView view(&scene);

        QGraphicsWidget *widget = new QGraphicsWidget;
        scene.addItem(widget);
        QVERIFY(!view.viewport()->hasMouseTracking());
        // Enable window decoraton.
        widget->setWindowFlags(Qt::Window | Qt::WindowTitleHint);
        QVERIFY(view.viewport()->hasMouseTracking());
    }

    // Make sure we enable mouse tracking when having items with a cursor set.
    {
        // Adding an item to the scene after the scene is set on the view.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsView view(&scene);

        QGraphicsRectItem *item = new QGraphicsRectItem(10, 10, 10, 10);
#ifndef QT_NO_CURSOR
        item->setCursor(Qt::CrossCursor);
#endif
        scene.addItem(item);
        QVERIFY(view.viewport()->hasMouseTracking());
    }
    {
        // Adding an item to the scene before the scene is set on the view.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsRectItem *item = new QGraphicsRectItem(10, 10, 10, 10);
#ifndef QT_NO_CURSOR
        item->setCursor(Qt::CrossCursor);
#endif
        scene.addItem(item);

        QGraphicsView view(&scene);
        QVERIFY(view.viewport()->hasMouseTracking());
    }

    // Make sure we propagate mouse tracking to all views.
    {
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsView view1(&scene);
        QGraphicsView view2(&scene);
        QGraphicsView view3(&scene);

        QGraphicsRectItem *item = new QGraphicsRectItem(10, 10, 10, 10);
#ifndef QT_NO_CURSOR
        item->setCursor(Qt::CrossCursor);
#endif
        scene.addItem(item);

        QVERIFY(view1.viewport()->hasMouseTracking());
        QVERIFY(view2.viewport()->hasMouseTracking());
        QVERIFY(view3.viewport()->hasMouseTracking());
    }
}

void tst_QGraphicsView::mouseTracking2()
{
    // Make sure mouse move events propagates to the scene when
    // mouse tracking is explicitly enabled on the view,
    // even when all items ignore hover events / use default cursor.

    QGraphicsScene scene;
    scene.addRect(0, 0, 100, 100);

    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QVERIFY(!view.viewport()->hasMouseTracking());
    view.viewport()->setMouseTracking(true); // Explicitly enable mouse tracking.
    QVERIFY(view.viewport()->hasMouseTracking());

    EventSpy spy(&scene, QEvent::GraphicsSceneMouseMove);
    QCOMPARE(spy.count(), 0);
    QMouseEvent event(QEvent::MouseMove,view.viewport()->rect().center(), Qt::NoButton,
                      Qt::MouseButtons(Qt::NoButton), 0);
    QApplication::sendEvent(view.viewport(), &event);
    QCOMPARE(spy.count(), 1);
}

void tst_QGraphicsView::mouseTracking3()
{
    // Mouse tracking should be automatically enabled if AnchorUnderMouse is used for
    // view transform or resize. We never disable mouse tracking if it is already enabled.

    { // Make sure we enable mouse tracking when using AnchorUnderMouse for view transformation.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsView view(&scene);
        QVERIFY(!view.viewport()->hasMouseTracking());

        view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        QVERIFY(view.viewport()->hasMouseTracking());
    }

    { // Make sure we enable mouse tracking when using AnchorUnderMouse for view resizing.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsView view(&scene);
        QVERIFY(!view.viewport()->hasMouseTracking());

        view.setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        QVERIFY(view.viewport()->hasMouseTracking());
    }

    { // Make sure we don't disable mouse tracking in setViewport/setScene (transformation anchor).
        QGraphicsView view;
        view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        QVERIFY(view.viewport()->hasMouseTracking());

        QWidget *viewport = new QWidget;
        view.setViewport(viewport);
        QVERIFY(viewport->hasMouseTracking());

        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        view.setScene(&scene);
        QVERIFY(viewport->hasMouseTracking());
    }

    { // Make sure we don't disable mouse tracking in setViewport/setScene (resize anchor).
        QGraphicsView view;
        view.setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        QVERIFY(view.viewport()->hasMouseTracking());

        QWidget *viewport = new QWidget;
        view.setViewport(viewport);
        QVERIFY(viewport->hasMouseTracking());

        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        view.setScene(&scene);
        QVERIFY(viewport->hasMouseTracking());
    }

    // Make sure we don't disable mouse tracking when adding an item (transformation anchor).
    { // Adding an item to the scene before the scene is set on the view.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsRectItem *item = new QGraphicsRectItem(10, 10, 10, 10);
        scene.addItem(item);

        QGraphicsView view;
        view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        view.setScene(&scene);
        QVERIFY(view.viewport()->hasMouseTracking());
    }

    { // Adding an item to the scene after the scene is set on the view.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsView view(&scene);
        view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

        QGraphicsRectItem *item = new QGraphicsRectItem(10, 10, 10, 10);
        scene.addItem(item);
        QVERIFY(view.viewport()->hasMouseTracking());
    }

    // Make sure we don't disable mouse tracking when adding an item (resize anchor).
    { // Adding an item to the scene before the scene is set on the view.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsRectItem *item = new QGraphicsRectItem(10, 10, 10, 10);
        scene.addItem(item);

        QGraphicsView view;
        view.setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        view.setScene(&scene);
        QVERIFY(view.viewport()->hasMouseTracking());
    }

    { // Adding an item to the scene after the scene is set on the view.
        QGraphicsScene scene(-10000, -10000, 20000, 20000);
        QGraphicsView view(&scene);
        view.setResizeAnchor(QGraphicsView::AnchorUnderMouse);

        QGraphicsRectItem *item = new QGraphicsRectItem(10, 10, 10, 10);
        scene.addItem(item);
        QVERIFY(view.viewport()->hasMouseTracking());
    }
}

class RenderTester : public QGraphicsRectItem
{
public:
    RenderTester(const QRectF &rect)
        : QGraphicsRectItem(rect), paints(0)
    { }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget)
    {
        QGraphicsRectItem::paint(painter, option, widget);
        ++paints;
    }

    int paints;
};

void tst_QGraphicsView::render()
{
    // ### This test can be much more thorough - see QGraphicsScene::render.
    QGraphicsScene scene;
    CustomView view(&scene);
    view.setFrameStyle(0);
    view.resize(200, 200);
    view.painted = false;
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QApplication::processEvents();
    QTRY_VERIFY(view.painted);

    RenderTester *r1 = new RenderTester(QRectF(0, 0, 50, 50));
    RenderTester *r2 = new RenderTester(QRectF(50, 50, 50, 50));
    RenderTester *r3 = new RenderTester(QRectF(0, 50, 50, 50));
    RenderTester *r4 = new RenderTester(QRectF(50, 0, 50, 50));
    scene.addItem(r1);
    scene.addItem(r2);
    scene.addItem(r3);
    scene.addItem(r4);

    qApp->processEvents();

    QTRY_COMPARE(r1->paints, 1);
    QCOMPARE(r2->paints, 1);
    QCOMPARE(r3->paints, 1);
    QCOMPARE(r4->paints, 1);

    QPixmap pix(200, 200);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    view.render(&painter);
    painter.end();

    QCOMPARE(r1->paints, 2);
    QCOMPARE(r2->paints, 2);
    QCOMPARE(r3->paints, 2);
    QCOMPARE(r4->paints, 2);
}

void tst_QGraphicsView::exposeRegion()
{
    RenderTester *item = new RenderTester(QRectF(0, 0, 20, 20));
    QGraphicsScene scene;
    scene.addItem(item);

    item->paints = 0;
    CustomView view;
    view.setScene(&scene);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QTRY_VERIFY(item->paints > 0);

    item->paints = 0;
    view.lastUpdateRegions.clear();

    // Update a small area in the viewport's topLeft() and bottomRight().
    // (the boundingRect() of this area covers the entire viewport).
    QWidget *viewport = view.viewport();
    QRegion expectedExposeRegion = QRect(0, 0, 5, 5);
    expectedExposeRegion += QRect(viewport->rect().bottomRight() - QPoint(5, 5), QSize(5, 5));
    viewport->update(expectedExposeRegion);
    QApplication::processEvents();

    // Make sure it triggers correct repaint on the view.
    QTRY_COMPARE(view.lastUpdateRegions.size(), 1);
    COMPARE_REGIONS(view.lastUpdateRegions.at(0), expectedExposeRegion);

    // Make sure the item didn't get any repaints.
#ifndef Q_OS_MAC
    QCOMPARE(item->paints, 0);
#endif
}

void tst_QGraphicsView::update_data()
{
    // In view.viewport() coordinates. (viewport rect: QRect(0, 0, 200, 200))
    QTest::addColumn<QRect>("updateRect");
    QTest::newRow("empty") << QRect();
    QTest::newRow("outside left") << QRect(-200, 0, 100, 100);
    QTest::newRow("outside right") << QRect(400, 0 ,100, 100);
    QTest::newRow("outside top") << QRect(0, -200, 100, 100);
    QTest::newRow("outside bottom") << QRect(0, 400, 100, 100);
    QTest::newRow("partially inside left") << QRect(-50, 0, 100, 100);
    QTest::newRow("partially inside right") << QRect(-150, 0, 100, 100);
    QTest::newRow("partially inside top") << QRect(0, -150, 100, 100);
    QTest::newRow("partially inside bottom") << QRect(0, 150, 100, 100);
    QTest::newRow("on topLeft edge") << QRect(-100, -100, 100, 100);
    QTest::newRow("on topRight edge") << QRect(200, -100, 100, 100);
    QTest::newRow("on bottomRight edge") << QRect(200, 200, 100, 100);
    QTest::newRow("on bottomLeft edge") << QRect(-200, 200, 100, 100);
    QTest::newRow("inside topLeft") << QRect(-99, -99, 100, 100);
    QTest::newRow("inside topRight") << QRect(199, -99, 100, 100);
    QTest::newRow("inside bottomRight") << QRect(199, 199, 100, 100);
    QTest::newRow("inside bottomLeft") << QRect(-199, 199, 100, 100);
    QTest::newRow("large1") << QRect(50, -100, 100, 400);
    QTest::newRow("large2") << QRect(-100, 50, 400, 100);
    QTest::newRow("large3") << QRect(-100, -100, 400, 400);
    QTest::newRow("viewport rect") << QRect(0, 0, 200, 200);
}

void tst_QGraphicsView::update()
{
    QFETCH(QRect, updateRect);

    // some window manager resize the toplevel to max screen size
    // so we must make our view a child (no layout!) of a dummy toplevel
    // to ensure that it's really 200x200 pixels
    QWidget toplevel;

    // Create a view with viewport rect equal to QRect(0, 0, 200, 200).
    QGraphicsScene dummyScene;
    CustomView view(0, &toplevel);
    view.setScene(&dummyScene);
    view.ensurePolished(); // must ensure polished to get content margins right
    int left, top, right, bottom;
    view.getContentsMargins(&left, &top, &right, &bottom);
    view.resize(200 + left + right, 200 + top + bottom);
    toplevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toplevel));


    QApplication::setActiveWindow(&toplevel);
    QApplication::processEvents();
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&toplevel));

    const QRect viewportRect = view.viewport()->rect();
    QCOMPARE(viewportRect, QRect(0, 0, 200, 200));

#if defined QT_BUILD_INTERNAL
    const bool intersects = updateRect.intersects(viewportRect);
    QGraphicsViewPrivate *viewPrivate = static_cast<QGraphicsViewPrivate *>(qt_widget_private(&view));
    QTRY_COMPARE(viewPrivate->updateRect(updateRect), intersects);
    QApplication::processEvents();

    view.lastUpdateRegions.clear();
    viewPrivate->processPendingUpdates();
    QVERIFY(viewPrivate->dirtyRegion.isEmpty());
    QVERIFY(viewPrivate->dirtyBoundingRect.isEmpty());
    QApplication::processEvents();
    if (!intersects) {
        QTRY_VERIFY(view.lastUpdateRegions.isEmpty());
    } else {
        QTRY_COMPARE(view.lastUpdateRegions.size(), 1);
        QTRY_COMPARE(view.lastUpdateRegions.at(0), QRegion(updateRect) & viewportRect);
    }
    QTRY_VERIFY(!viewPrivate->fullUpdatePending);
#endif
}

void tst_QGraphicsView::update2_data()
{
    QTest::addColumn<qreal>("penWidth");
    QTest::addColumn<bool>("antialiasing");
    QTest::addColumn<bool>("changedConnected");

    // Anti-aliased.
    QTest::newRow("pen width: 0.0, antialiasing: true") << qreal(0.0) << true << false;
    QTest::newRow("pen width: 1.5, antialiasing: true") << qreal(1.5) << true << false;
    QTest::newRow("pen width: 2.0, antialiasing: true") << qreal(2.0) << true << false;
    QTest::newRow("pen width: 3.0, antialiasing: true") << qreal(3.0) << true << false;

    // Aliased.
    QTest::newRow("pen width: 0.0, antialiasing: false") << qreal(0.0) << false << false;
    QTest::newRow("pen width: 1.5, antialiasing: false") << qreal(1.5) << false << false;
    QTest::newRow("pen width: 2.0, antialiasing: false") << qreal(2.0) << false << false;
    QTest::newRow("pen width: 3.0, antialiasing: false") << qreal(3.0) << false << false;

    // changed() connected
    QTest::newRow("pen width: 0.0, antialiasing: false, changed") << qreal(0.0) << false << true;
    QTest::newRow("pen width: 1.5, antialiasing: true, changed") << qreal(1.5) << true << true;
    QTest::newRow("pen width: 2.0, antialiasing: false, changed") << qreal(2.0) << false << true;
    QTest::newRow("pen width: 3.0, antialiasing: true, changed") << qreal(3.0) << true << true;
}

void tst_QGraphicsView::update2()
{
    QFETCH(qreal, penWidth);
    QFETCH(bool, antialiasing);
    QFETCH(bool, changedConnected);

    // Create a rect item.
    const QRectF rawItemRect(-50.4, -50.3, 100.2, 100.1);
    CountPaintItem *rect = new CountPaintItem(rawItemRect);
    QPen pen;
    pen.setWidthF(penWidth);
    rect->setPen(pen);

    // Add item to a scene.
    QGraphicsScene scene(-100, -100, 200, 200);
    if (changedConnected)
        QObject::connect(&scene, SIGNAL(changed(QList<QRectF>)), this, SLOT(dummySlot()));

    scene.addItem(rect);

    // Create a view on the scene.
    CustomView view(&scene);
    view.setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, !antialiasing);
    view.setRenderHint(QPainter::Antialiasing, antialiasing);
    view.setFrameStyle(0);
    view.resize(200, 200);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(rect->numPaints > 0);

    // Calculate expected update region for the rect.
    QRectF expectedItemBoundingRect = rawItemRect;
    const qreal halfPenWidth = penWidth / qreal(2.0);
    expectedItemBoundingRect.adjust(-halfPenWidth, -halfPenWidth, halfPenWidth, halfPenWidth);
    QCOMPARE(rect->boundingRect(), expectedItemBoundingRect);

    QRect expectedItemDeviceBoundingRect = rect->deviceTransform(view.viewportTransform())
                                           .mapRect(expectedItemBoundingRect).toAlignedRect();
    if (antialiasing)
        expectedItemDeviceBoundingRect.adjust(-2, -2, 2, 2);
    else
        expectedItemDeviceBoundingRect.adjust(-1, -1, 1, 1);
    const QRegion expectedUpdateRegion(expectedItemDeviceBoundingRect);

    // Reset.
    rect->numPaints = 0;
    view.lastUpdateRegions.clear();
    view.painted = false;

    rect->update();
    QTRY_VERIFY(view.painted);

#ifndef Q_OS_MAC //cocoa doesn't support drawing regions
    QTRY_VERIFY(view.painted);
    QCOMPARE(view.lastUpdateRegions.size(), 1);
    QCOMPARE(view.lastUpdateRegions.at(0), expectedUpdateRegion);
#endif
}

void tst_QGraphicsView::update_ancestorClipsChildrenToShape()
{
    QGraphicsScene scene(-150, -150, 300, 300);

    /*
    Add three rects:

    +------------------+
    | child            |
    | +--------------+ |
    | | parent       | |
    | |  +-----------+ |
    | |  |grandParent| |
    | |  +-----------+ |
    | +--------------+ |
    +------------------+

    ... where both the parent and the grand parent clips children to shape.
    */
    QApplication::processEvents(); // Get rid of pending update.

    QGraphicsRectItem *grandParent = static_cast<QGraphicsRectItem *>(scene.addRect(0, 0, 50, 50));
    grandParent->setBrush(Qt::black);
    grandParent->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    QGraphicsRectItem *parent = static_cast<QGraphicsRectItem *>(scene.addRect(-50, -50, 100, 100));
    parent->setBrush(QColor(0, 0, 255, 125));
    parent->setParentItem(grandParent);
    parent->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    QGraphicsRectItem *child = static_cast<QGraphicsRectItem *>(scene.addRect(-100, -100, 200, 200));
    child->setBrush(QColor(255, 0, 0, 125));
    child->setParentItem(parent);

    CustomView view(&scene);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.painted);

    view.lastUpdateRegions.clear();
    view.painted = false;

    // Call child->update() and make sure the updated area is within the ancestors' clip.
    QRectF expected = child->deviceTransform(view.viewportTransform()).mapRect(child->boundingRect());
    expected &= grandParent->deviceTransform(view.viewportTransform()).mapRect(grandParent->boundingRect());

    child->update();
    QTRY_VERIFY(view.painted);

#ifndef Q_OS_MAC //cocoa doesn't support drawing regions
    QTRY_VERIFY(view.painted);
    QCOMPARE(view.lastUpdateRegions.size(), 1);
    QCOMPARE(view.lastUpdateRegions.at(0), QRegion(expected.toAlignedRect()));
#endif
}

void tst_QGraphicsView::update_ancestorClipsChildrenToShape2()
{
    QGraphicsScene scene(-150, -150, 300, 300);

    /*
    Add two rects:

    +------------------+
    | child            |
    | +--------------+ |
    | | parent       | |
    | |              | |
    | |              | |
    | |              | |
    | +--------------+ |
    +------------------+

    ... where the parent has no contents and clips the child to shape.
    */
    QApplication::processEvents(); // Get rid of pending update.

    QGraphicsRectItem *parent = static_cast<QGraphicsRectItem *>(scene.addRect(-50, -50, 100, 100));
    parent->setBrush(QColor(0, 0, 255, 125));
    parent->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    parent->setFlag(QGraphicsItem::ItemHasNoContents);

    QGraphicsRectItem *child = static_cast<QGraphicsRectItem *>(scene.addRect(-100, -100, 200, 200));
    child->setBrush(QColor(255, 0, 0, 125));
    child->setParentItem(parent);

    CustomView view(&scene);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.painted);

    view.lastUpdateRegions.clear();
    view.painted = false;

    // Call child->update() and make sure the updated area is within its parent's clip.
    QRectF expected = child->deviceTransform(view.viewportTransform()).mapRect(child->boundingRect());
    expected &= parent->deviceTransform(view.viewportTransform()).mapRect(parent->boundingRect());

    child->update();
    QTRY_VERIFY(view.painted);

#ifndef Q_OS_MAC //cocoa doesn't support drawing regions
    QTRY_VERIFY(view.painted);
    QCOMPARE(view.lastUpdateRegions.size(), 1);
    QCOMPARE(view.lastUpdateRegions.at(0), QRegion(expected.toAlignedRect()));
#endif

    view.lastUpdateRegions.clear();
    view.painted = false;

    // Invalidate the parent's geometry and trigger an update.
    // The update area should be clipped to the parent's bounding rect for 'normal' items,
    // but in this case the item has no contents (ItemHasNoContents) and its geometry
    // is invalidated, which means we cannot clip the child update. So, the expected
    // area is exactly the same as the child's bounding rect (adjusted for antialiasing).
    parent->setRect(parent->rect().adjusted(-10, -10, -10, -10));
    expected = child->deviceTransform(view.viewportTransform()).mapRect(child->boundingRect());
    expected.adjust(-2, -2, 2, 2); // Antialiasing

#ifndef Q_OS_MAC //cocoa doesn't support drawing regions
    QTRY_VERIFY(view.painted);
    QCOMPARE(view.lastUpdateRegions.size(), 1);
    QCOMPARE(view.lastUpdateRegions.at(0), QRegion(expected.toAlignedRect()));
#endif
}

class FocusItem : public QGraphicsRectItem
{
public:
    FocusItem() : QGraphicsRectItem(0, 0, 20, 20) {
        m_viewHasIMEnabledInFocusInEvent = false;
    }

    void focusInEvent(QFocusEvent * /* event */)
    {
        QGraphicsView *view = scene()->views().first();
        m_viewHasIMEnabledInFocusInEvent = view->testAttribute(Qt::WA_InputMethodEnabled);
    }

    bool m_viewHasIMEnabledInFocusInEvent;
};

void tst_QGraphicsView::inputMethodSensitivity()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&view));

    FocusItem *item = new FocusItem;

    view.setAttribute(Qt::WA_InputMethodEnabled, true);

    scene.addItem(item);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);

    scene.removeItem(item);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);

    item->setFlag(QGraphicsItem::ItemAcceptsInputMethod);
    scene.addItem(item);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);

    scene.removeItem(item);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);

    scene.addItem(item);
    scene.setFocusItem(item);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);

    scene.removeItem(item);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);

    item->setFlag(QGraphicsItem::ItemIsFocusable);
    scene.addItem(item);
    scene.setFocusItem(item);
    QCOMPARE(scene.focusItem(), static_cast<QGraphicsItem *>(item));
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), true);
    QCOMPARE(item->m_viewHasIMEnabledInFocusInEvent, true);

    item->setFlag(QGraphicsItem::ItemAcceptsInputMethod, false);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);

    item->setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), true);

    // introduce another item that is focusable but does not accept input methods
    FocusItem *item2 = new FocusItem;
    item2->setFlag(QGraphicsItem::ItemIsFocusable);
    scene.addItem(item2);
    scene.setFocusItem(item2);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);
    QCOMPARE(item2->m_viewHasIMEnabledInFocusInEvent, false);
    QCOMPARE(scene.focusItem(), static_cast<QGraphicsItem *>(item2));

    scene.setFocusItem(item);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), true);
    QCOMPARE(item->m_viewHasIMEnabledInFocusInEvent, true);
    QCOMPARE(scene.focusItem(), static_cast<QGraphicsItem *>(item));

    view.setScene(0);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);
    QCOMPARE(scene.focusItem(), static_cast<QGraphicsItem *>(item));

    view.setScene(&scene);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), true);
    QCOMPARE(item->m_viewHasIMEnabledInFocusInEvent, true);
    QCOMPARE(scene.focusItem(), static_cast<QGraphicsItem *>(item));

    scene.setFocusItem(item2);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);
    QCOMPARE(item2->m_viewHasIMEnabledInFocusInEvent, false);
    QCOMPARE(scene.focusItem(), static_cast<QGraphicsItem *>(item2));

    view.setScene(0);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);
    QCOMPARE(scene.focusItem(), static_cast<QGraphicsItem *>(item2));

    scene.setFocusItem(item);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), false);
    QCOMPARE(scene.focusItem(), static_cast<QGraphicsItem *>(item));

    view.setScene(&scene);
    QCOMPARE(view.testAttribute(Qt::WA_InputMethodEnabled), true);
    QCOMPARE(item->m_viewHasIMEnabledInFocusInEvent, true);
    QCOMPARE(scene.focusItem(), static_cast<QGraphicsItem *>(item));
}

void tst_QGraphicsView::inputContextReset()
{
    PlatformInputContext inputContext;
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = &inputContext;

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    QVERIFY(view.testAttribute(Qt::WA_InputMethodEnabled));

    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget *>(&view));

    QGraphicsItem *item1 = new QGraphicsRectItem;
    item1->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemAcceptsInputMethod);

    inputContext.m_resetCallCount = 0;
    inputContext.m_commitCallCount = 0;
    scene.addItem(item1);
    QCOMPARE(inputContext.m_resetCallCount, 0);
    QCOMPARE(inputContext.m_commitCallCount, 0);

    scene.setFocusItem(item1);
    QCOMPARE(scene.focusItem(), (QGraphicsItem *)item1);
    QVERIFY(view.testAttribute(Qt::WA_InputMethodEnabled));
    QCOMPARE(inputContext.m_resetCallCount, 0);
    QCOMPARE(inputContext.m_commitCallCount, 0);

    scene.setFocusItem(0);
    // the input context is reset twice, once because an item has lost focus and again because
    // the Qt::WA_InputMethodEnabled flag is cleared because no item has focus.
    //    QEXPECT_FAIL("", "QTBUG-22454", Abort);
    QCOMPARE(inputContext.m_resetCallCount + inputContext.m_commitCallCount, 2);

    // introduce another item that is focusable but does not accept input methods
    QGraphicsItem *item2 = new QGraphicsRectItem;
    item2->setFlags(QGraphicsItem::ItemIsFocusable);
    scene.addItem(item2);

    inputContext.m_resetCallCount = 0;
    inputContext.m_commitCallCount = 0;
    scene.setFocusItem(item2);
    QCOMPARE(inputContext.m_resetCallCount, 0);
    QCOMPARE(inputContext.m_commitCallCount, 0);

    scene.setFocusItem(item1);
    QCOMPARE(inputContext.m_resetCallCount, 0);
    QCOMPARE(inputContext.m_commitCallCount, 0);

    // test changing between between items that accept input methods.
    item2->setFlags(QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemAcceptsInputMethod);
    scene.setFocusItem(item2);
    QCOMPARE(inputContext.m_resetCallCount + inputContext.m_commitCallCount, 1);
}

void tst_QGraphicsView::indirectPainting()
{
    class MyScene : public QGraphicsScene
    { public:
        MyScene() : QGraphicsScene(), drawCount(0) {}
        void drawItems(QPainter *, int, QGraphicsItem **, const QStyleOptionGraphicsItem *, QWidget *)
        { ++drawCount; }
        int drawCount;
    };

    MyScene scene;
    QGraphicsItem *item = scene.addRect(0, 0, 50, 50);

    QGraphicsView view(&scene);
    view.setOptimizationFlag(QGraphicsView::IndirectPainting);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(scene.drawCount > 0);

    scene.drawCount = 0;
    item->setPos(20, 20);
    QApplication::processEvents();
    QTRY_VERIFY(scene.drawCount > 0);
}

void tst_QGraphicsView::compositionModeInDrawBackground()
{
    class MyView : public QGraphicsView
    { public:
        MyView(QGraphicsScene *scene) : QGraphicsView(scene),
        painted(false), compositionMode(QPainter::CompositionMode_SourceOver) {}
        bool painted;
        QPainter::CompositionMode compositionMode;
        void drawBackground(QPainter *painter, const QRectF &)
        {
            compositionMode = painter->compositionMode();
            painted = true;
        }
    };

    QGraphicsScene dummy;
    MyView view(&dummy);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // Make sure the painter's composition mode is SourceOver in drawBackground.
    QTRY_VERIFY(view.painted);
    QCOMPARE(view.compositionMode, QPainter::CompositionMode_SourceOver);

    view.painted = false;
    view.setCacheMode(QGraphicsView::CacheBackground);
    view.viewport()->update();

    // Make sure the painter's composition mode is SourceOver in drawBackground
    // with background cache enabled.
    QTRY_VERIFY(view.painted);
    QCOMPARE(view.compositionMode, QPainter::CompositionMode_SourceOver);
}
void tst_QGraphicsView::task253415_reconnectUpdateSceneOnSceneChanged()
{
    QGraphicsView view;
    QGraphicsView dummyView;
    view.setWindowFlags(view.windowFlags() | Qt::WindowStaysOnTopHint);
    view.resize(200, 200);

    QGraphicsScene scene1;
    QObject::connect(&scene1, SIGNAL(changed(QList<QRectF>)), &dummyView, SLOT(updateScene(QList<QRectF>)));
    view.setScene(&scene1);

    QTest::qWait(12);

    QGraphicsScene scene2;
    QObject::connect(&scene2, SIGNAL(changed(QList<QRectF>)), &dummyView, SLOT(updateScene(QList<QRectF>)));
    view.setScene(&scene2);

    QTest::qWait(12);

    bool wasConnected2 = QObject::disconnect(&scene2, SIGNAL(changed(QList<QRectF>)), &view, 0);
    QVERIFY(wasConnected2);
}

void tst_QGraphicsView::task255529_transformationAnchorMouseAndViewportMargins()
{
    QGraphicsScene scene(-100, -100, 200, 200);
    scene.addRect(QRectF(-50, -50, 100, 100), QPen(Qt::black), QBrush(Qt::blue));

    class VpGraphicsView: public QGraphicsView
    {
    public:
        VpGraphicsView(QGraphicsScene *scene, QWidget *parent=0)
            : QGraphicsView(scene, parent)
        {
            setViewportMargins(8, 16, 12, 20);
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            setMouseTracking(true);
        }
    };

    VpGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);
    view.show();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    // This is highly unstable (observed to pass on Windows and some Linux configurations).
#ifndef Q_OS_MAC
    for (int i = 0; i < 4; ++i) {
        QPoint mouseViewPos(20, 20);
        sendMouseMove(view.viewport(), mouseViewPos);

        QPointF mouseScenePos = view.mapToScene(mouseViewPos);
        view.setTransform(QTransform().scale(5, 5).rotate(5, Qt::ZAxis), true);

        qreal slack = 1;

        QPointF newMouseScenePos = view.mapToScene(mouseViewPos);

        const qreal dx = qAbs(newMouseScenePos.x() - mouseScenePos.x());
        const qreal dy = qAbs(newMouseScenePos.y() - mouseScenePos.y());
        const QByteArray message = QString::fromLatin1("QTBUG-22455, distance: dx=%1, dy=%2 slack=%3 (%4).").
                         arg(dx).arg(dy).arg(slack).arg(qApp->style()->metaObject()->className()).toLocal8Bit();
        if (i == 9 || (dx < slack && dy < slack)) {
            QVERIFY2(dx < slack && dy < slack, message.constData());
            break;
        }

        QTest::qWait(100);
    }
#endif
}

void tst_QGraphicsView::task259503_scrollingArtifacts()
{
    QGraphicsScene scene(0, 0, 800, 600);

    QGraphicsRectItem card;
    card.setRect(0, 0, 50, 50);
    card.setPen(QPen(Qt::darkRed));
    card.setBrush(QBrush(Qt::cyan));
    card.setZValue(2.0);
    card.setPos(300, 300);
    scene.addItem(&card);

    class SAGraphicsView: public QGraphicsView
    {
    public:
        SAGraphicsView(QGraphicsScene *scene)
            : QGraphicsView(scene)
            , itSTimeToTest(false)
        {
            setViewportUpdateMode( QGraphicsView::MinimalViewportUpdate );
            resize(QSize(640, 480));
        }

        QRegion updateRegion;
        bool itSTimeToTest;

        void paintEvent(QPaintEvent *event)
        {
            QGraphicsView::paintEvent(event);

            if (itSTimeToTest)
            {
                QEXPECT_FAIL("", "QTBUG-24296", Continue);
                QCOMPARE(event->region(), updateRegion);
            }
        }
    };

    SAGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    int hsbValue = view.horizontalScrollBar()->value();
    view.horizontalScrollBar()->setValue(hsbValue / 2);
    QTest::qWait(10);
    view.horizontalScrollBar()->setValue(0);
    QTest::qWait(10);

    QRect itemDeviceBoundingRect = card.deviceTransform(view.viewportTransform()).mapRect(card.boundingRect()).toRect();
    itemDeviceBoundingRect.adjust(-2, -2, 2, 2);
    view.updateRegion = itemDeviceBoundingRect;
    view.updateRegion += itemDeviceBoundingRect.translated(-100, 0);
    view.itSTimeToTest = true;
    card.setPos(200, 300);
    QTest::qWait(10);
}

void tst_QGraphicsView::QTBUG_4151_clipAndIgnore_data()
{
    QTest::addColumn<bool>("clip");
    QTest::addColumn<bool>("ignoreTransformations");
    QTest::addColumn<int>("numItems");

    QTest::newRow("none") << false << false << 3;
    QTest::newRow("clip") << true << false << 3;
    QTest::newRow("ignore") << false << true << 3;
    QTest::newRow("clip+ignore") << true << true << 3;
}

void tst_QGraphicsView::QTBUG_4151_clipAndIgnore()
{
    QFETCH(bool, clip);
    QFETCH(bool, ignoreTransformations);
    QFETCH(int, numItems);

    QGraphicsScene scene;

    QGraphicsRectItem *parent = new QGraphicsRectItem(QRectF(0, 0, 50, 50), 0);
    QGraphicsRectItem *child = new QGraphicsRectItem(QRectF(-10, -10, 40, 40), parent);
    QGraphicsRectItem *ignore = new QGraphicsRectItem(QRectF(60, 60, 50, 50), 0);

    if (clip)
        parent->setFlags(QGraphicsItem::ItemClipsChildrenToShape);
    if (ignoreTransformations)
        ignore->setFlag(QGraphicsItem::ItemIgnoresTransformations);

    parent->setBrush(Qt::red);
    child->setBrush(QColor(0, 0, 255, 128));
    ignore->setBrush(Qt::green);

    scene.addItem(parent);
    scene.addItem(ignore);

    QGraphicsView view(&scene);
    setFrameless(&view);
    view.setFrameStyle(0);
    view.resize(75, 75);
    view.show();
    view.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), (QWidget *)&view);

    QCOMPARE(view.items(view.rect()).size(), numItems);
}

void tst_QGraphicsView::QTBUG_5859_exposedRect()
{
    class CustomScene : public QGraphicsScene
    {
    public:
        CustomScene(const QRectF &rect) : QGraphicsScene(rect) { }
        void drawBackground(QPainter * /* painter */, const QRectF &rect)
        { lastBackgroundExposedRect = rect; }
        QRectF lastBackgroundExposedRect;
    };

    class CustomRectItem : public QGraphicsRectItem
    {
    public:
        CustomRectItem(const QRectF &rect) : QGraphicsRectItem(rect)
        { setFlag(QGraphicsItem::ItemUsesExtendedStyleOption); }
        void paint(QPainter * /* painter */, const QStyleOptionGraphicsItem *option, QWidget * /* widget */ = 0)
        { lastExposedRect = option->exposedRect; }
        QRectF lastExposedRect;
    };

    CustomScene scene(QRectF(0,0,50,50));

    CustomRectItem item(scene.sceneRect());

    scene.addItem(&item);

    QGraphicsView view(&scene);
    view.scale(4.15, 4.15);
    view.showNormal();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    view.viewport()->repaint(10,10,20,20);
    QApplication::processEvents();

    QCOMPARE(item.lastExposedRect, scene.lastBackgroundExposedRect);
}

#ifndef QT_NO_CURSOR
void tst_QGraphicsView::QTBUG_7438_cursor()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(-10, -10, 20, 20));
    item->setFlag(QGraphicsItem::ItemIsMovable);

    QGraphicsView view(&scene);
    view.setFixedSize(400, 400);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QCOMPARE(view.viewport()->cursor().shape(), QCursor().shape());
    view.viewport()->setCursor(Qt::PointingHandCursor);
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);
    sendMouseMove(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);
    sendMousePress(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);
    sendMouseRelease(view.viewport(), view.mapFromScene(0, 0));
    QCOMPARE(view.viewport()->cursor().shape(), Qt::PointingHandCursor);
}
#endif

class GraphicsItemWithHover : public QGraphicsRectItem
{
public:
    GraphicsItemWithHover()
    {
        setRect(0, 0, 100, 100);
        setAcceptHoverEvents(true);
    }

    bool sceneEvent(QEvent *event)
    {
        if (!checkEvents) // ensures that we don't look at stray events before we are ready
            return QGraphicsRectItem::sceneEvent(event);

        if (event->type() == QEvent::GraphicsSceneHoverEnter) {
            receivedEnterEvent = true;
            enterWidget = static_cast<QGraphicsSceneHoverEvent *>(event)->widget();
        } else if (event->type() == QEvent::GraphicsSceneHoverLeave) {
            receivedLeaveEvent = true;
            leaveWidget = static_cast<QGraphicsSceneHoverEvent *>(event)->widget();
        }
        return QGraphicsRectItem::sceneEvent(event);
    }

    bool receivedEnterEvent = false;
    bool receivedLeaveEvent = false;
    QWidget *enterWidget = nullptr;
    QWidget *leaveWidget = nullptr;
    bool checkEvents = false;
};

void tst_QGraphicsView::hoverLeave()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.resize(160, 160);
    GraphicsItemWithHover *item = new GraphicsItemWithHover;
    scene.addItem(item);

    view.showNormal();
    qApp->setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QWindow *viewWindow = view.window()->windowHandle();
    QPoint posOutsideItem = view.mapFromScene(item->mapToScene(0, 0)) - QPoint(5, 0);
    QPoint posOutsideItemGlobal = view.mapToGlobal(posOutsideItem);
    QPoint posOutsideItemInWindow = viewWindow->mapFromGlobal(posOutsideItemGlobal);
    QTest::mouseMove(viewWindow, posOutsideItemInWindow);

    item->checkEvents = true;
    QPoint posInItemGlobal = view.mapToGlobal(view.mapFromScene(item->mapToScene(10, 10)));
    QTest::mouseMove(viewWindow, viewWindow->mapFromGlobal(posInItemGlobal));
    QTRY_VERIFY(item->receivedEnterEvent);
    QCOMPARE(item->enterWidget, view.viewport());

    QTest::mouseMove(viewWindow, posOutsideItemInWindow);

    QTRY_VERIFY(item->receivedLeaveEvent);
    QCOMPARE(item->leaveWidget, view.viewport());
}

class IMItem : public QGraphicsRectItem
{
public:
    IMItem(QGraphicsItem *parent = 0):
        QGraphicsRectItem(QRectF(0, 0, 20, 20), parent)
    {
        setFlag(QGraphicsItem::ItemIsFocusable, true);
        setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);
    }

    QVariant inputMethodQuery(Qt::InputMethodQuery) const
    {
        return mf;
    }

    static QRectF mf;
};

QRectF IMItem::mf(1.5, 1.6, 10, 10);

void tst_QGraphicsView::QTBUG_16063_microFocusRect()
{
    QGraphicsScene scene;
    IMItem *item = new IMItem();
    scene.addItem(item);

    QGraphicsView view(&scene);
    setFrameless(&view);

    view.setFixedSize(40, 40);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    scene.setFocusItem(item);
    view.setFocus();
    QRectF mfv = view.inputMethodQuery(Qt::ImMicroFocus).toRectF();
    QCOMPARE(mfv, IMItem::mf.translated(-view.mapToScene(view.sceneRect().toRect()).boundingRect().topLeft()));
}

QTEST_MAIN(tst_QGraphicsView)
#include "tst_qgraphicsview.moc"
