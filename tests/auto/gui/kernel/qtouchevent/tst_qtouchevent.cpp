/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the $MODULE$ of the Qt Toolkit.
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

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsWidget>
#include <QtWidgets/QWidget>
#include <QtTest>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <private/qhighdpiscaling_p.h>

class tst_QTouchEventWidget : public QWidget
{
public:
    QList<QTouchEvent::TouchPoint> touchBeginPoints, touchUpdatePoints, touchEndPoints;
    bool seenTouchBegin, seenTouchUpdate, seenTouchEnd;
    bool acceptTouchBegin, acceptTouchUpdate, acceptTouchEnd;
    bool deleteInTouchBegin, deleteInTouchUpdate, deleteInTouchEnd;
    ulong timestamp;
    QTouchDevice *deviceFromEvent;

    explicit tst_QTouchEventWidget(QWidget *parent = Q_NULLPTR) : QWidget(parent)
    {
        reset();
    }

    void reset()
    {
        touchBeginPoints.clear();
        touchUpdatePoints.clear();
        touchEndPoints.clear();
        seenTouchBegin = seenTouchUpdate = seenTouchEnd = false;
        acceptTouchBegin = acceptTouchUpdate = acceptTouchEnd = true;
        deleteInTouchBegin = deleteInTouchUpdate = deleteInTouchEnd = false;
    }

    bool event(QEvent *event) Q_DECL_OVERRIDE
    {
        switch (event->type()) {
        case QEvent::TouchBegin:
            if (seenTouchBegin) qWarning("TouchBegin: already seen a TouchBegin");
            if (seenTouchUpdate) qWarning("TouchBegin: TouchUpdate cannot happen before TouchBegin");
            if (seenTouchEnd) qWarning("TouchBegin: TouchEnd cannot happen before TouchBegin");
            seenTouchBegin = !seenTouchBegin && !seenTouchUpdate && !seenTouchEnd;
            touchBeginPoints = static_cast<QTouchEvent *>(event)->touchPoints();
            timestamp = static_cast<QTouchEvent *>(event)->timestamp();
            deviceFromEvent = static_cast<QTouchEvent *>(event)->device();
            event->setAccepted(acceptTouchBegin);
            if (deleteInTouchBegin)
                delete this;
            break;
        case QEvent::TouchUpdate:
            if (!seenTouchBegin) qWarning("TouchUpdate: have not seen TouchBegin");
            if (seenTouchEnd) qWarning("TouchUpdate: TouchEnd cannot happen before TouchUpdate");
            seenTouchUpdate = seenTouchBegin && !seenTouchEnd;
            touchUpdatePoints = static_cast<QTouchEvent *>(event)->touchPoints();
            timestamp = static_cast<QTouchEvent *>(event)->timestamp();
            deviceFromEvent = static_cast<QTouchEvent *>(event)->device();
            event->setAccepted(acceptTouchUpdate);
            if (deleteInTouchUpdate)
                delete this;
            break;
        case QEvent::TouchEnd:
            if (!seenTouchBegin) qWarning("TouchEnd: have not seen TouchBegin");
            if (seenTouchEnd) qWarning("TouchEnd: already seen a TouchEnd");
            seenTouchEnd = seenTouchBegin && !seenTouchEnd;
            touchEndPoints = static_cast<QTouchEvent *>(event)->touchPoints();
            timestamp = static_cast<QTouchEvent *>(event)->timestamp();
            deviceFromEvent = static_cast<QTouchEvent *>(event)->device();
            event->setAccepted(acceptTouchEnd);
            if (deleteInTouchEnd)
                delete this;
            break;
        default:
            return QWidget::event(event);
        }
        return true;
    }
};

class tst_QTouchEventGraphicsItem : public QGraphicsItem
{
public:
    QList<QTouchEvent::TouchPoint> touchBeginPoints, touchUpdatePoints, touchEndPoints;
    bool seenTouchBegin, seenTouchUpdate, seenTouchEnd;
    int touchBeginCounter, touchUpdateCounter, touchEndCounter;
    bool acceptTouchBegin, acceptTouchUpdate, acceptTouchEnd;
    bool deleteInTouchBegin, deleteInTouchUpdate, deleteInTouchEnd;
    tst_QTouchEventGraphicsItem **weakpointer;

    explicit tst_QTouchEventGraphicsItem(QGraphicsItem *parent = Q_NULLPTR)
        : QGraphicsItem(parent), weakpointer(0)
    {
        reset();
    }

    ~tst_QTouchEventGraphicsItem()
    {
        if (weakpointer)
            *weakpointer = 0;
    }

    void reset()
    {
        touchBeginPoints.clear();
        touchUpdatePoints.clear();
        touchEndPoints.clear();
        seenTouchBegin = seenTouchUpdate = seenTouchEnd = false;
        touchBeginCounter = touchUpdateCounter = touchEndCounter = 0;
        acceptTouchBegin = acceptTouchUpdate = acceptTouchEnd = true;
        deleteInTouchBegin = deleteInTouchUpdate = deleteInTouchEnd = false;
    }

    QRectF boundingRect() const Q_DECL_OVERRIDE { return QRectF(0, 0, 10, 10); }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) Q_DECL_OVERRIDE
    {
        painter->fillRect(QRectF(QPointF(0, 0), boundingRect().size()), Qt::yellow);
    }

    bool sceneEvent(QEvent *event) Q_DECL_OVERRIDE
    {
        switch (event->type()) {
        case QEvent::TouchBegin:
            if (seenTouchBegin) qWarning("TouchBegin: already seen a TouchBegin");
            if (seenTouchUpdate) qWarning("TouchBegin: TouchUpdate cannot happen before TouchBegin");
            if (seenTouchEnd) qWarning("TouchBegin: TouchEnd cannot happen before TouchBegin");
            seenTouchBegin = !seenTouchBegin && !seenTouchUpdate && !seenTouchEnd;
            ++touchBeginCounter;
            touchBeginPoints = static_cast<QTouchEvent *>(event)->touchPoints();
            event->setAccepted(acceptTouchBegin);
            if (deleteInTouchBegin)
                delete this;
            break;
        case QEvent::TouchUpdate:
            if (!seenTouchBegin) qWarning("TouchUpdate: have not seen TouchBegin");
            if (seenTouchEnd) qWarning("TouchUpdate: TouchEnd cannot happen before TouchUpdate");
            seenTouchUpdate = seenTouchBegin && !seenTouchEnd;
            ++touchUpdateCounter;
            touchUpdatePoints = static_cast<QTouchEvent *>(event)->touchPoints();
            event->setAccepted(acceptTouchUpdate);
            if (deleteInTouchUpdate)
                delete this;
            break;
        case QEvent::TouchEnd:
            if (!seenTouchBegin) qWarning("TouchEnd: have not seen TouchBegin");
            if (seenTouchEnd) qWarning("TouchEnd: already seen a TouchEnd");
            seenTouchEnd = seenTouchBegin && !seenTouchEnd;
            ++touchEndCounter;
            touchEndPoints = static_cast<QTouchEvent *>(event)->touchPoints();
            event->setAccepted(acceptTouchEnd);
            if (deleteInTouchEnd)
                delete this;
            break;
        default:
            return QGraphicsItem::sceneEvent(event);
        }
        return true;
    }
};

class tst_QTouchEvent : public QObject
{
    Q_OBJECT
public:
    tst_QTouchEvent();

private slots:
    void cleanup();
    void qPointerUniqueId();
    void touchDisabledByDefault();
    void touchEventAcceptedByDefault();
    void touchBeginPropagatesWhenIgnored();
    void touchUpdateAndEndNeverPropagate();
    void basicRawEventTranslation();
    void multiPointRawEventTranslationOnTouchScreen();
    void multiPointRawEventTranslationOnTouchPad();
    void deleteInEventHandler();
    void deleteInRawEventTranslation();
    void crashInQGraphicsSceneAfterNotHandlingTouchBegin();
    void touchBeginWithGraphicsWidget();
    void testQGuiAppDelivery();
    void testMultiDevice();

private:
    QTouchDevice *touchScreenDevice;
    QTouchDevice *touchPadDevice;
};

tst_QTouchEvent::tst_QTouchEvent()
  : touchScreenDevice(QTest::createTouchDevice())
  , touchPadDevice(QTest::createTouchDevice(QTouchDevice::TouchPad))
{
}

void tst_QTouchEvent::cleanup()
{
    QVERIFY(QGuiApplication::topLevelWindows().isEmpty());
}

void tst_QTouchEvent::qPointerUniqueId()
{
    QPointingDeviceUniqueId id1, id2;

    QCOMPARE(id1.numericId(), Q_INT64_C(-1));
    QVERIFY(!id1.isValid());

    QVERIFY(  id1 == id2);
    QVERIFY(!(id1 != id2));

    QSet<QPointingDeviceUniqueId> set; // compile test
    set.insert(id1);
    set.insert(id2);
    QCOMPARE(set.size(), 1);


    const auto id3 = QPointingDeviceUniqueId::fromNumericId(-1);
    QCOMPARE(id3.numericId(), Q_INT64_C(-1));
    QVERIFY(!id3.isValid());

    QVERIFY(  id1 == id3);
    QVERIFY(!(id1 != id3));

    set.insert(id3);
    QCOMPARE(set.size(), 1);


    const auto id4 = QPointingDeviceUniqueId::fromNumericId(4);
    QCOMPARE(id4.numericId(), Q_INT64_C(4));
    QVERIFY(id4.isValid());

    QVERIFY(  id1 != id4);
    QVERIFY(!(id1 == id4));

    set.insert(id4);
    QCOMPARE(set.size(), 2);
}

void tst_QTouchEvent::touchDisabledByDefault()
{
    // QWidget
    {
        // the widget attribute is not enabled by default
        QWidget widget;
        QVERIFY(!widget.testAttribute(Qt::WA_AcceptTouchEvents));

        // events should not be accepted since they are not enabled
        QList<QTouchEvent::TouchPoint> touchPoints;
        touchPoints.append(QTouchEvent::TouchPoint(0));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointPressed,
                               touchPoints);
        QVERIFY(!QApplication::sendEvent(&widget, &touchEvent));
        QVERIFY(!touchEvent.isAccepted());
    }

    // QGraphicsView
    {
        QGraphicsScene scene;
        tst_QTouchEventGraphicsItem item;
        QGraphicsView view(&scene);
        scene.addItem(&item);
        item.setPos(100, 100);
        view.resize(200, 200);
        view.fitInView(scene.sceneRect());

        // touch events are not accepted by default
        QVERIFY(!item.acceptTouchEvents());

        // compose an event to the scene that is over the item
        QTouchEvent::TouchPoint touchPoint(0);
        touchPoint.setState(Qt::TouchPointPressed);
        touchPoint.setPos(view.mapFromScene(item.mapToScene(item.boundingRect().center())));
        touchPoint.setScreenPos(view.mapToGlobal(touchPoint.pos().toPoint()));
        touchPoint.setScenePos(view.mapToScene(touchPoint.pos().toPoint()));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointPressed,
                               (QList<QTouchEvent::TouchPoint>() << touchPoint));
        QVERIFY(!QApplication::sendEvent(view.viewport(), &touchEvent));
        QVERIFY(!touchEvent.isAccepted());
        QVERIFY(!item.seenTouchBegin);
    }
}

void tst_QTouchEvent::touchEventAcceptedByDefault()
{
    if (!QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // QWidget
    {
        // enabling touch events should automatically accept touch events
        QWidget widget;
        widget.setAttribute(Qt::WA_AcceptTouchEvents);

        // QWidget handles touch event by converting them into a mouse event, so the event is both
        // accepted and handled (res == true)
        QList<QTouchEvent::TouchPoint> touchPoints;
        touchPoints.append(QTouchEvent::TouchPoint(0));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointPressed,
                               touchPoints);
        QVERIFY(QApplication::sendEvent(&widget, &touchEvent));
        QVERIFY(!touchEvent.isAccepted()); // Qt 5.X ignores touch events.

        // tst_QTouchEventWidget does handle, sending succeeds
        tst_QTouchEventWidget touchWidget;
        touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
        touchEvent.ignore();
        QVERIFY(QApplication::sendEvent(&touchWidget, &touchEvent));
        QVERIFY(touchEvent.isAccepted());
    }

    // QGraphicsView
    {
        QGraphicsScene scene;
        tst_QTouchEventGraphicsItem item;
        QGraphicsView view(&scene);
        scene.addItem(&item);
        item.setPos(100, 100);
        view.resize(200, 200);
        view.fitInView(scene.sceneRect());

        // enabling touch events on the item also enables events on the viewport
        item.setAcceptTouchEvents(true);
        QVERIFY(view.viewport()->testAttribute(Qt::WA_AcceptTouchEvents));

        // compose an event to the scene that is over the item
        QTouchEvent::TouchPoint touchPoint(0);
        touchPoint.setState(Qt::TouchPointPressed);
        touchPoint.setPos(view.mapFromScene(item.mapToScene(item.boundingRect().center())));
        touchPoint.setScreenPos(view.mapToGlobal(touchPoint.pos().toPoint()));
        touchPoint.setScenePos(view.mapToScene(touchPoint.pos().toPoint()));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointPressed,
                               (QList<QTouchEvent::TouchPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(item.seenTouchBegin);
    }
}

void tst_QTouchEvent::touchBeginPropagatesWhenIgnored()
{
    // QWidget
    {
        tst_QTouchEventWidget window, child, grandchild;
        child.setParent(&window);
        grandchild.setParent(&child);

        // all widgets accept touch events, grandchild ignores, so child sees the event, but not window
        window.setAttribute(Qt::WA_AcceptTouchEvents);
        child.setAttribute(Qt::WA_AcceptTouchEvents);
        grandchild.setAttribute(Qt::WA_AcceptTouchEvents);
        grandchild.acceptTouchBegin = false;

        QList<QTouchEvent::TouchPoint> touchPoints;
        touchPoints.append(QTouchEvent::TouchPoint(0));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointPressed,
                               touchPoints);
        QVERIFY(QApplication::sendEvent(&grandchild, &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(grandchild.seenTouchBegin);
        QVERIFY(child.seenTouchBegin);
        QVERIFY(!window.seenTouchBegin);

        // disable touch on grandchild. even though it doesn't accept it, child should still get the
        // TouchBegin
        grandchild.reset();
        child.reset();
        window.reset();
        grandchild.setAttribute(Qt::WA_AcceptTouchEvents, false);

        touchEvent.ignore();
        QVERIFY(QApplication::sendEvent(&grandchild, &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(!grandchild.seenTouchBegin);
        QVERIFY(child.seenTouchBegin);
        QVERIFY(!window.seenTouchBegin);
    }

    // QGraphicsView
    {
        QGraphicsScene scene;
        tst_QTouchEventGraphicsItem root, child, grandchild;
        QGraphicsView view(&scene);
        scene.addItem(&root);
        root.setPos(100, 100);
        child.setParentItem(&root);
        grandchild.setParentItem(&child);
        view.resize(200, 200);
        view.fitInView(scene.sceneRect());

        // all items accept touch events, grandchild ignores, so child sees the event, but not root
        root.setAcceptTouchEvents(true);
        child.setAcceptTouchEvents(true);
        grandchild.setAcceptTouchEvents(true);
        grandchild.acceptTouchBegin = false;

        // compose an event to the scene that is over the grandchild
        QTouchEvent::TouchPoint touchPoint(0);
        touchPoint.setState(Qt::TouchPointPressed);
        touchPoint.setPos(view.mapFromScene(grandchild.mapToScene(grandchild.boundingRect().center())));
        touchPoint.setScreenPos(view.mapToGlobal(touchPoint.pos().toPoint()));
        touchPoint.setScenePos(view.mapToScene(touchPoint.pos().toPoint()));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointPressed,
                               (QList<QTouchEvent::TouchPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(grandchild.seenTouchBegin);
        QVERIFY(child.seenTouchBegin);
        QVERIFY(!root.seenTouchBegin);
    }

    // QGraphicsView
    {
        QGraphicsScene scene;
        tst_QTouchEventGraphicsItem root, child, grandchild;
        QGraphicsView view(&scene);
        scene.addItem(&root);
        root.setPos(100, 100);
        child.setParentItem(&root);
        grandchild.setParentItem(&child);
        view.resize(200, 200);
        view.fitInView(scene.sceneRect());

        // leave touch disabled on grandchild. even though it doesn't accept it, child should
        // still get the TouchBegin
        root.setAcceptTouchEvents(true);
        child.setAcceptTouchEvents(true);

        // compose an event to the scene that is over the grandchild
        QTouchEvent::TouchPoint touchPoint(0);
        touchPoint.setState(Qt::TouchPointPressed);
        touchPoint.setPos(view.mapFromScene(grandchild.mapToScene(grandchild.boundingRect().center())));
        touchPoint.setScreenPos(view.mapToGlobal(touchPoint.pos().toPoint()));
        touchPoint.setScenePos(view.mapToScene(touchPoint.pos().toPoint()));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointPressed,
                               (QList<QTouchEvent::TouchPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(!grandchild.seenTouchBegin);
        QVERIFY(child.seenTouchBegin);
        QVERIFY(!root.seenTouchBegin);
    }
}

void tst_QTouchEvent::touchUpdateAndEndNeverPropagate()
{
    // QWidget
    {
        tst_QTouchEventWidget window, child;
        child.setParent(&window);

        window.setAttribute(Qt::WA_AcceptTouchEvents);
        child.setAttribute(Qt::WA_AcceptTouchEvents);
        child.acceptTouchUpdate = false;
        child.acceptTouchEnd = false;

        QList<QTouchEvent::TouchPoint> touchPoints;
        touchPoints.append(QTouchEvent::TouchPoint(0));
        QTouchEvent touchBeginEvent(QEvent::TouchBegin,
                                    touchScreenDevice,
                                    Qt::NoModifier,
                                    Qt::TouchPointPressed,
                                    touchPoints);
        QVERIFY(QApplication::sendEvent(&child, &touchBeginEvent));
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(child.seenTouchBegin);
        QVERIFY(!window.seenTouchBegin);

        // send the touch update to the child, but ignore it, it doesn't propagate
        QTouchEvent touchUpdateEvent(QEvent::TouchUpdate,
                                     touchScreenDevice,
                                     Qt::NoModifier,
                                     Qt::TouchPointMoved,
                                     touchPoints);
        QVERIFY(QApplication::sendEvent(&child, &touchUpdateEvent));
        QVERIFY(!touchUpdateEvent.isAccepted());
        QVERIFY(child.seenTouchUpdate);
        QVERIFY(!window.seenTouchUpdate);

        // send the touch end, same thing should happen as with touch update
        QTouchEvent touchEndEvent(QEvent::TouchEnd,
                                  touchScreenDevice,
                                  Qt::NoModifier,
                                  Qt::TouchPointReleased,
                                  touchPoints);
        QVERIFY(QApplication::sendEvent(&child, &touchEndEvent));
        QVERIFY(!touchEndEvent.isAccepted());
        QVERIFY(child.seenTouchEnd);
        QVERIFY(!window.seenTouchEnd);
    }

    // QGraphicsView
    {
        QGraphicsScene scene;
        tst_QTouchEventGraphicsItem root, child, grandchild;
        QGraphicsView view(&scene);
        scene.addItem(&root);
        root.setPos(100, 100);
        child.setParentItem(&root);
        grandchild.setParentItem(&child);
        view.resize(200, 200);
        view.fitInView(scene.sceneRect());

        root.setAcceptTouchEvents(true);
        child.setAcceptTouchEvents(true);
        child.acceptTouchUpdate = false;
        child.acceptTouchEnd = false;

        // compose an event to the scene that is over the child
        QTouchEvent::TouchPoint touchPoint(0);
        touchPoint.setState(Qt::TouchPointPressed);
        touchPoint.setPos(view.mapFromScene(grandchild.mapToScene(grandchild.boundingRect().center())));
        touchPoint.setScreenPos(view.mapToGlobal(touchPoint.pos().toPoint()));
        touchPoint.setScenePos(view.mapToScene(touchPoint.pos().toPoint()));
        QTouchEvent touchBeginEvent(QEvent::TouchBegin,
                                    touchScreenDevice,
                                    Qt::NoModifier,
                                    Qt::TouchPointPressed,
                                    (QList<QTouchEvent::TouchPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchBeginEvent));
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(child.seenTouchBegin);
        QVERIFY(!root.seenTouchBegin);

        // send the touch update to the child, but ignore it, it doesn't propagate
        touchPoint.setState(Qt::TouchPointMoved);
        QTouchEvent touchUpdateEvent(QEvent::TouchUpdate,
                                     touchScreenDevice,
                                     Qt::NoModifier,
                                     Qt::TouchPointMoved,
                                     (QList<QTouchEvent::TouchPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchUpdateEvent));
        // the scene accepts the event, since it found an item to send the event to
        QVERIFY(!touchUpdateEvent.isAccepted());
        QVERIFY(child.seenTouchUpdate);
        QVERIFY(!root.seenTouchUpdate);

        // send the touch end, same thing should happen as with touch update
        touchPoint.setState(Qt::TouchPointReleased);
        QTouchEvent touchEndEvent(QEvent::TouchEnd,
                                  touchScreenDevice,
                                  Qt::NoModifier,
                                  Qt::TouchPointReleased,
                                  (QList<QTouchEvent::TouchPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEndEvent));
        // the scene accepts the event, since it found an item to send the event to
        QVERIFY(!touchEndEvent.isAccepted());
        QVERIFY(child.seenTouchEnd);
        QVERIFY(!root.seenTouchEnd);
    }
}

QPointF normalized(const QPointF &pos, const QRectF &rect)
{
    return QPointF(pos.x() / rect.width(), pos.y() / rect.height());
}

void tst_QTouchEvent::basicRawEventTranslation()
{
    if (!QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    tst_QTouchEventWidget touchWidget;
    touchWidget.setWindowTitle(QTest::currentTestFunction());
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);
    touchWidget.show();
    QVERIFY(QTest::qWaitForWindowActive(&touchWidget));

    QPointF pos = touchWidget.rect().center();
    QPointF screenPos = touchWidget.mapToGlobal(pos.toPoint());
    QPointF delta(10, 10);
    QRectF screenGeometry = QApplication::desktop()->screenGeometry(&touchWidget);

    QTouchEvent::TouchPoint rawTouchPoint;
    rawTouchPoint.setId(0);

    // this should be translated to a TouchBegin
    rawTouchPoint.setState(Qt::TouchPointPressed);
    rawTouchPoint.setScreenPos(screenPos);
    rawTouchPoint.setNormalizedPos(normalized(rawTouchPoint.pos(), screenGeometry));
    QVector<QPointF> rawPosList;
    rawPosList << QPointF(12, 34) << QPointF(56, 78);
    rawTouchPoint.setRawScreenPositions(rawPosList);
    const ulong timestamp = 1234;
    QWindow *window = touchWidget.windowHandle();
    QList<QWindowSystemInterface::TouchPoint> nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QTouchEvent::TouchPoint>() << rawTouchPoint, window);
    QWindowSystemInterface::handleTouchEvent(window, timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchBeginPoints.count(), 1);
    QCOMPARE(touchWidget.timestamp, timestamp);
    QTouchEvent::TouchPoint touchBeginPoint = touchWidget.touchBeginPoints.first();
    QCOMPARE(touchBeginPoint.id(), rawTouchPoint.id());
    QCOMPARE(touchBeginPoint.state(), rawTouchPoint.state());
    QCOMPARE(touchBeginPoint.pos(), pos);
    QCOMPARE(touchBeginPoint.startPos(), pos);
    QCOMPARE(touchBeginPoint.lastPos(), pos);
    QCOMPARE(touchBeginPoint.scenePos(), rawTouchPoint.screenPos());
    QCOMPARE(touchBeginPoint.startScenePos(), rawTouchPoint.screenPos());
    QCOMPARE(touchBeginPoint.lastScenePos(), rawTouchPoint.screenPos());
    QCOMPARE(touchBeginPoint.screenPos(), rawTouchPoint.screenPos());
    QCOMPARE(touchBeginPoint.startScreenPos(), rawTouchPoint.screenPos());
    QCOMPARE(touchBeginPoint.lastScreenPos(), rawTouchPoint.screenPos());
    QCOMPARE(touchBeginPoint.normalizedPos(), rawTouchPoint.normalizedPos());
    QCOMPARE(touchBeginPoint.startNormalizedPos(), touchBeginPoint.normalizedPos());
    QCOMPARE(touchBeginPoint.lastNormalizedPos(), touchBeginPoint.normalizedPos());
    QCOMPARE(touchBeginPoint.rect(), QRectF(pos, QSizeF(0, 0)));
    QCOMPARE(touchBeginPoint.screenRect(), QRectF(rawTouchPoint.screenPos(), QSizeF(0, 0)));
    QCOMPARE(touchBeginPoint.sceneRect(), touchBeginPoint.screenRect());
    QCOMPARE(touchBeginPoint.pressure(), qreal(1.));
    QCOMPARE(touchBeginPoint.velocity(), QVector2D());
    if (!QHighDpiScaling::isActive())
        QCOMPARE(touchBeginPoint.rawScreenPositions(), rawPosList);

    // moving the point should translate to TouchUpdate
    rawTouchPoint.setState(Qt::TouchPointMoved);
    rawTouchPoint.setScreenPos(screenPos + delta);
    rawTouchPoint.setNormalizedPos(normalized(rawTouchPoint.pos(), screenGeometry));
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QTouchEvent::TouchPoint>() << rawTouchPoint, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchUpdatePoints.count(), 1);
    QTouchEvent::TouchPoint touchUpdatePoint = touchWidget.touchUpdatePoints.first();
    QCOMPARE(touchUpdatePoint.id(), rawTouchPoint.id());
    QCOMPARE(touchUpdatePoint.state(), rawTouchPoint.state());
    QCOMPARE(touchUpdatePoint.pos(), pos + delta);
    QCOMPARE(touchUpdatePoint.startPos(), pos);
    QCOMPARE(touchUpdatePoint.lastPos(), pos);
    QCOMPARE(touchUpdatePoint.scenePos(), rawTouchPoint.screenPos());
    QCOMPARE(touchUpdatePoint.startScenePos(), screenPos);
    QCOMPARE(touchUpdatePoint.lastScenePos(), screenPos);
    QCOMPARE(touchUpdatePoint.screenPos(), rawTouchPoint.screenPos());
    QCOMPARE(touchUpdatePoint.startScreenPos(), screenPos);
    QCOMPARE(touchUpdatePoint.lastScreenPos(), screenPos);
    QCOMPARE(touchUpdatePoint.normalizedPos(), rawTouchPoint.normalizedPos());
    QCOMPARE(touchUpdatePoint.startNormalizedPos(), touchBeginPoint.normalizedPos());
    QCOMPARE(touchUpdatePoint.lastNormalizedPos(), touchBeginPoint.normalizedPos());
    QCOMPARE(touchUpdatePoint.rect(), QRectF(pos + delta, QSizeF(0, 0)));
    QCOMPARE(touchUpdatePoint.screenRect(), QRectF(rawTouchPoint.screenPos(), QSizeF(0, 0)));
    QCOMPARE(touchUpdatePoint.sceneRect(), touchUpdatePoint.screenRect());
    QCOMPARE(touchUpdatePoint.pressure(), qreal(1.));

    // releasing the point translates to TouchEnd
    rawTouchPoint.setState(Qt::TouchPointReleased);
    rawTouchPoint.setScreenPos(screenPos + delta + delta);
    rawTouchPoint.setNormalizedPos(normalized(rawTouchPoint.pos(), screenGeometry));
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QTouchEvent::TouchPoint>() << rawTouchPoint, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchEndPoints.count(), 1);
    QTouchEvent::TouchPoint touchEndPoint = touchWidget.touchEndPoints.first();
    QCOMPARE(touchEndPoint.id(), rawTouchPoint.id());
    QCOMPARE(touchEndPoint.state(), rawTouchPoint.state());
    QCOMPARE(touchEndPoint.pos(), pos + delta + delta);
    QCOMPARE(touchEndPoint.startPos(), pos);
    QCOMPARE(touchEndPoint.lastPos(), pos + delta);
    QCOMPARE(touchEndPoint.scenePos(), rawTouchPoint.screenPos());
    QCOMPARE(touchEndPoint.startScenePos(), screenPos);
    QCOMPARE(touchEndPoint.lastScenePos(), screenPos + delta);
    QCOMPARE(touchEndPoint.screenPos(), rawTouchPoint.screenPos());
    QCOMPARE(touchEndPoint.startScreenPos(), screenPos);
    QCOMPARE(touchEndPoint.lastScreenPos(), screenPos + delta);
    QCOMPARE(touchEndPoint.normalizedPos(), rawTouchPoint.normalizedPos());
    QCOMPARE(touchEndPoint.startNormalizedPos(), touchBeginPoint.normalizedPos());
    QCOMPARE(touchEndPoint.lastNormalizedPos(), touchUpdatePoint.normalizedPos());
    QCOMPARE(touchEndPoint.rect(), QRectF(pos + delta + delta, QSizeF(0, 0)));
    QCOMPARE(touchEndPoint.screenRect(), QRectF(rawTouchPoint.screenPos(), QSizeF(0, 0)));
    QCOMPARE(touchEndPoint.sceneRect(), touchEndPoint.screenRect());
    QCOMPARE(touchEndPoint.pressure(), qreal(0.));
}

void tst_QTouchEvent::multiPointRawEventTranslationOnTouchScreen()
{
    if (!QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    tst_QTouchEventWidget touchWidget;
    touchWidget.setWindowTitle(QTest::currentTestFunction());
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);

    tst_QTouchEventWidget leftWidget(&touchWidget);
    leftWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    leftWidget.setGeometry(0, 100, 100, 100);

    tst_QTouchEventWidget rightWidget(&touchWidget);
    rightWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    rightWidget.setGeometry(300, 100, 100, 100);

    touchWidget.show();
    QVERIFY(QTest::qWaitForWindowActive(&touchWidget));

    QPointF leftPos = leftWidget.rect().center();
    QPointF rightPos = rightWidget.rect().center();
    QPointF centerPos = touchWidget.rect().center();
    QPointF leftScreenPos = leftWidget.mapToGlobal(leftPos.toPoint());
    QPointF rightScreenPos = rightWidget.mapToGlobal(rightPos.toPoint());
    QPointF centerScreenPos = touchWidget.mapToGlobal(centerPos.toPoint());
    QRectF screenGeometry = QApplication::desktop()->screenGeometry(&touchWidget);

    QList<QTouchEvent::TouchPoint> rawTouchPoints;
    rawTouchPoints.append(QTouchEvent::TouchPoint(0));
    rawTouchPoints.append(QTouchEvent::TouchPoint(1));

    // generate TouchBegins on both leftWidget and rightWidget
    rawTouchPoints[0].setState(Qt::TouchPointPressed);
    rawTouchPoints[0].setScreenPos(leftScreenPos);
    rawTouchPoints[0].setNormalizedPos(normalized(rawTouchPoints[0].pos(), screenGeometry));
    rawTouchPoints[1].setState(Qt::TouchPointPressed);
    rawTouchPoints[1].setScreenPos(rightScreenPos);
    rawTouchPoints[1].setNormalizedPos(normalized(rawTouchPoints[1].pos(), screenGeometry));
    QWindow *window = touchWidget.windowHandle();
    QList<QWindowSystemInterface::TouchPoint> nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QVERIFY(leftWidget.seenTouchBegin);
    QVERIFY(!leftWidget.seenTouchUpdate);
    QVERIFY(!leftWidget.seenTouchEnd);
    QVERIFY(rightWidget.seenTouchBegin);
    QVERIFY(!rightWidget.seenTouchUpdate);
    QVERIFY(!rightWidget.seenTouchEnd);
    QCOMPARE(leftWidget.touchBeginPoints.count(), 1);
    QCOMPARE(rightWidget.touchBeginPoints.count(), 1);
    {
        QTouchEvent::TouchPoint leftTouchPoint = leftWidget.touchBeginPoints.first();
        QCOMPARE(leftTouchPoint.id(), rawTouchPoints[0].id());
        QCOMPARE(leftTouchPoint.state(), rawTouchPoints[0].state());
        QCOMPARE(leftTouchPoint.pos(), leftPos);
        QCOMPARE(leftTouchPoint.startPos(), leftPos);
        QCOMPARE(leftTouchPoint.lastPos(), leftPos);
        QCOMPARE(leftTouchPoint.scenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.startScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.screenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.startScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.normalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.startNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.lastNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.rect(), QRectF(leftPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.sceneRect(), QRectF(leftScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.screenRect(), QRectF(leftScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.pressure(), qreal(1.));

        QTouchEvent::TouchPoint rightTouchPoint = rightWidget.touchBeginPoints.first();
        QCOMPARE(rightTouchPoint.id(), rawTouchPoints[1].id());
        QCOMPARE(rightTouchPoint.state(), rawTouchPoints[1].state());
        QCOMPARE(rightTouchPoint.pos(), rightPos);
        QCOMPARE(rightTouchPoint.startPos(), rightPos);
        QCOMPARE(rightTouchPoint.lastPos(), rightPos);
        QCOMPARE(rightTouchPoint.scenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.startScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.screenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.startScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.normalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.startNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.lastNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.rect(), QRectF(rightPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.sceneRect(), QRectF(rightScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.screenRect(), QRectF(rightScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.pressure(), qreal(1.));
    }

    // generate TouchUpdates on both leftWidget and rightWidget
    rawTouchPoints[0].setState(Qt::TouchPointMoved);
    rawTouchPoints[0].setScreenPos(centerScreenPos);
    rawTouchPoints[0].setNormalizedPos(normalized(rawTouchPoints[0].pos(), screenGeometry));
    rawTouchPoints[1].setState(Qt::TouchPointMoved);
    rawTouchPoints[1].setScreenPos(centerScreenPos);
    rawTouchPoints[1].setNormalizedPos(normalized(rawTouchPoints[1].pos(), screenGeometry));
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QVERIFY(leftWidget.seenTouchBegin);
    QVERIFY(leftWidget.seenTouchUpdate);
    QVERIFY(!leftWidget.seenTouchEnd);
    QVERIFY(rightWidget.seenTouchBegin);
    QVERIFY(rightWidget.seenTouchUpdate);
    QVERIFY(!rightWidget.seenTouchEnd);
    QCOMPARE(leftWidget.touchUpdatePoints.count(), 1);
    QCOMPARE(rightWidget.touchUpdatePoints.count(), 1);
    {
        QTouchEvent::TouchPoint leftTouchPoint = leftWidget.touchUpdatePoints.first();
        QCOMPARE(leftTouchPoint.id(), rawTouchPoints[0].id());
        QCOMPARE(leftTouchPoint.state(), rawTouchPoints[0].state());
        QCOMPARE(leftTouchPoint.pos(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(leftTouchPoint.startPos(), leftPos);
        QCOMPARE(leftTouchPoint.lastPos(), leftPos);
        QCOMPARE(leftTouchPoint.scenePos(), centerScreenPos);
        QCOMPARE(leftTouchPoint.startScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.screenPos(), centerScreenPos);
        QCOMPARE(leftTouchPoint.startScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.normalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.startNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.lastNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.rect(), QRectF(leftWidget.mapFromParent(centerPos.toPoint()), QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.sceneRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.screenRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.pressure(), qreal(1.));

        QTouchEvent::TouchPoint rightTouchPoint = rightWidget.touchUpdatePoints.first();
        QCOMPARE(rightTouchPoint.id(), rawTouchPoints[1].id());
        QCOMPARE(rightTouchPoint.state(), rawTouchPoints[1].state());
        QCOMPARE(rightTouchPoint.pos(), QPointF(rightWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(rightTouchPoint.startPos(), rightPos);
        QCOMPARE(rightTouchPoint.lastPos(), rightPos);
        QCOMPARE(rightTouchPoint.scenePos(), centerScreenPos);
        QCOMPARE(rightTouchPoint.startScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.screenPos(), centerScreenPos);
        QCOMPARE(rightTouchPoint.startScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.normalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.startNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.lastNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.rect(), QRectF(rightWidget.mapFromParent(centerPos.toPoint()), QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.sceneRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.screenRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.pressure(), qreal(1.));
    }

    // generate TouchEnds on both leftWidget and rightWidget
    rawTouchPoints[0].setState(Qt::TouchPointReleased);
    rawTouchPoints[0].setScreenPos(centerScreenPos);
    rawTouchPoints[0].setNormalizedPos(normalized(rawTouchPoints[0].pos(), screenGeometry));
    rawTouchPoints[1].setState(Qt::TouchPointReleased);
    rawTouchPoints[1].setScreenPos(centerScreenPos);
    rawTouchPoints[1].setNormalizedPos(normalized(rawTouchPoints[1].pos(), screenGeometry));
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QVERIFY(leftWidget.seenTouchBegin);
    QVERIFY(leftWidget.seenTouchUpdate);
    QVERIFY(leftWidget.seenTouchEnd);
    QVERIFY(rightWidget.seenTouchBegin);
    QVERIFY(rightWidget.seenTouchUpdate);
    QVERIFY(rightWidget.seenTouchEnd);
    QCOMPARE(leftWidget.touchEndPoints.count(), 1);
    QCOMPARE(rightWidget.touchEndPoints.count(), 1);
    {
        QTouchEvent::TouchPoint leftTouchPoint = leftWidget.touchEndPoints.first();
        QCOMPARE(leftTouchPoint.id(), rawTouchPoints[0].id());
        QCOMPARE(leftTouchPoint.state(), rawTouchPoints[0].state());
        QCOMPARE(leftTouchPoint.pos(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(leftTouchPoint.startPos(), leftPos);
        QCOMPARE(leftTouchPoint.lastPos(), leftTouchPoint.pos());
        QCOMPARE(leftTouchPoint.scenePos(), centerScreenPos);
        QCOMPARE(leftTouchPoint.startScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScenePos(), leftTouchPoint.scenePos());
        QCOMPARE(leftTouchPoint.screenPos(), centerScreenPos);
        QCOMPARE(leftTouchPoint.startScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScreenPos(), leftTouchPoint.screenPos());
        QCOMPARE(leftTouchPoint.normalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.startNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.lastNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.rect(), QRectF(leftWidget.mapFromParent(centerPos.toPoint()), QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.sceneRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.screenRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.pressure(), qreal(0.));

        QTouchEvent::TouchPoint rightTouchPoint = rightWidget.touchEndPoints.first();
        QCOMPARE(rightTouchPoint.id(), rawTouchPoints[1].id());
        QCOMPARE(rightTouchPoint.state(), rawTouchPoints[1].state());
        QCOMPARE(rightTouchPoint.pos(), QPointF(rightWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(rightTouchPoint.startPos(), rightPos);
        QCOMPARE(rightTouchPoint.lastPos(), rightTouchPoint.pos());
        QCOMPARE(rightTouchPoint.scenePos(), centerScreenPos);
        QCOMPARE(rightTouchPoint.startScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScenePos(), rightTouchPoint.scenePos());
        QCOMPARE(rightTouchPoint.screenPos(), centerScreenPos);
        QCOMPARE(rightTouchPoint.startScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScreenPos(), rightTouchPoint.screenPos());
        QCOMPARE(rightTouchPoint.normalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.startNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.lastNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.rect(), QRectF(rightWidget.mapFromParent(centerPos.toPoint()), QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.sceneRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.screenRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.pressure(), qreal(0.));
    }
}

void tst_QTouchEvent::multiPointRawEventTranslationOnTouchPad()
{
    if (!QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    tst_QTouchEventWidget touchWidget;
    touchWidget.setWindowTitle(QTest::currentTestFunction());
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);

    tst_QTouchEventWidget leftWidget(&touchWidget);
    leftWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    leftWidget.setGeometry(0, 100, 100, 100);
    leftWidget.acceptTouchBegin =true;

    tst_QTouchEventWidget rightWidget(&touchWidget);
    rightWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    rightWidget.setGeometry(300, 100, 100, 100);

    touchWidget.show();
    QVERIFY(QTest::qWaitForWindowActive(&touchWidget));

    QPointF leftPos = leftWidget.rect().center();
    QPointF rightPos = rightWidget.rect().center();
    QPointF centerPos = touchWidget.rect().center();
    QPointF leftScreenPos = leftWidget.mapToGlobal(leftPos.toPoint());
    QPointF rightScreenPos = rightWidget.mapToGlobal(rightPos.toPoint());
    QPointF centerScreenPos = touchWidget.mapToGlobal(centerPos.toPoint());
    QRectF screenGeometry = QApplication::desktop()->screenGeometry(&touchWidget);

    QList<QTouchEvent::TouchPoint> rawTouchPoints;
    rawTouchPoints.append(QTouchEvent::TouchPoint(0));
    rawTouchPoints.append(QTouchEvent::TouchPoint(1));

    // generate TouchBegin on leftWidget only
    rawTouchPoints[0].setState(Qt::TouchPointPressed);
    rawTouchPoints[0].setScreenPos(leftScreenPos);
    rawTouchPoints[0].setNormalizedPos(normalized(rawTouchPoints[0].pos(), screenGeometry));
    rawTouchPoints[1].setState(Qt::TouchPointPressed);
    rawTouchPoints[1].setScreenPos(rightScreenPos);
    rawTouchPoints[1].setNormalizedPos(normalized(rawTouchPoints[1].pos(), screenGeometry));
    QWindow *window = touchWidget.windowHandle();
    QList<QWindowSystemInterface::TouchPoint> nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchPadDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QEXPECT_FAIL("", "QTBUG-46266, fails in Qt 5", Abort);
    QVERIFY(!leftWidget.seenTouchBegin);
    QVERIFY(!leftWidget.seenTouchUpdate);
    QVERIFY(!leftWidget.seenTouchEnd);
    QVERIFY(!rightWidget.seenTouchBegin);
    QVERIFY(!rightWidget.seenTouchUpdate);
    QVERIFY(!rightWidget.seenTouchEnd);
    QCOMPARE(leftWidget.touchBeginPoints.count(), 2);
    QCOMPARE(rightWidget.touchBeginPoints.count(), 0);
    {
        QTouchEvent::TouchPoint leftTouchPoint = leftWidget.touchBeginPoints.at(0);
        QCOMPARE(leftTouchPoint.id(), rawTouchPoints[0].id());
        QCOMPARE(leftTouchPoint.state(), rawTouchPoints[0].state());
        QCOMPARE(leftTouchPoint.pos(), leftPos);
        QCOMPARE(leftTouchPoint.startPos(), leftPos);
        QCOMPARE(leftTouchPoint.lastPos(), leftPos);
        QCOMPARE(leftTouchPoint.scenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.startScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.screenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.startScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.normalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.startNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.lastNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.rect(), QRectF(leftPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.sceneRect(), QRectF(leftScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.screenRect(), QRectF(leftScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.pressure(), qreal(1.));

        QTouchEvent::TouchPoint rightTouchPoint = leftWidget.touchBeginPoints.at(1);
        QCOMPARE(rightTouchPoint.id(), rawTouchPoints[1].id());
        QCOMPARE(rightTouchPoint.state(), rawTouchPoints[1].state());
        QCOMPARE(rightTouchPoint.pos(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.startPos(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.lastPos(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.scenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.startScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.screenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.startScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.normalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.startNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.lastNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.rect(), QRectF(leftWidget.mapFromGlobal(rightScreenPos.toPoint()), QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.sceneRect(), QRectF(rightScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.screenRect(), QRectF(rightScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.pressure(), qreal(1.));
    }

    // generate TouchUpdate on leftWidget
    rawTouchPoints[0].setState(Qt::TouchPointMoved);
    rawTouchPoints[0].setScreenPos(centerScreenPos);
    rawTouchPoints[0].setNormalizedPos(normalized(rawTouchPoints[0].pos(), screenGeometry));
    rawTouchPoints[1].setState(Qt::TouchPointMoved);
    rawTouchPoints[1].setScreenPos(centerScreenPos);
    rawTouchPoints[1].setNormalizedPos(normalized(rawTouchPoints[1].pos(), screenGeometry));
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchPadDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QVERIFY(leftWidget.seenTouchBegin);
    QVERIFY(leftWidget.seenTouchUpdate);
    QVERIFY(!leftWidget.seenTouchEnd);
    QVERIFY(!rightWidget.seenTouchBegin);
    QVERIFY(!rightWidget.seenTouchUpdate);
    QVERIFY(!rightWidget.seenTouchEnd);
    QCOMPARE(leftWidget.touchUpdatePoints.count(), 2);
    QCOMPARE(rightWidget.touchUpdatePoints.count(), 0);
    {
        QTouchEvent::TouchPoint leftTouchPoint = leftWidget.touchUpdatePoints.at(0);
        QCOMPARE(leftTouchPoint.id(), rawTouchPoints[0].id());
        QCOMPARE(leftTouchPoint.state(), rawTouchPoints[0].state());
        QCOMPARE(leftTouchPoint.pos(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(leftTouchPoint.startPos(), leftPos);
        QCOMPARE(leftTouchPoint.lastPos(), leftPos);
        QCOMPARE(leftTouchPoint.scenePos(), centerScreenPos);
        QCOMPARE(leftTouchPoint.startScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.screenPos(), centerScreenPos);
        QCOMPARE(leftTouchPoint.startScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.normalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.startNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.lastNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.rect(), QRectF(leftWidget.mapFromParent(centerPos.toPoint()), QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.sceneRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.screenRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.pressure(), qreal(1.));

        QTouchEvent::TouchPoint rightTouchPoint = leftWidget.touchUpdatePoints.at(1);
        QCOMPARE(rightTouchPoint.id(), rawTouchPoints[1].id());
        QCOMPARE(rightTouchPoint.state(), rawTouchPoints[1].state());
        QCOMPARE(rightTouchPoint.pos(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(rightTouchPoint.startPos(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.lastPos(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.scenePos(), centerScreenPos);
        QCOMPARE(rightTouchPoint.startScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.screenPos(), centerScreenPos);
        QCOMPARE(rightTouchPoint.startScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.normalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.startNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.lastNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.rect(), QRectF(leftWidget.mapFromParent(centerPos.toPoint()), QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.sceneRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.screenRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.pressure(), qreal(1.));
    }

    // generate TouchEnd on leftWidget
    rawTouchPoints[0].setState(Qt::TouchPointReleased);
    rawTouchPoints[0].setScreenPos(centerScreenPos);
    rawTouchPoints[0].setNormalizedPos(normalized(rawTouchPoints[0].pos(), screenGeometry));
    rawTouchPoints[1].setState(Qt::TouchPointReleased);
    rawTouchPoints[1].setScreenPos(centerScreenPos);
    rawTouchPoints[1].setNormalizedPos(normalized(rawTouchPoints[1].pos(), screenGeometry));
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchPadDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QVERIFY(leftWidget.seenTouchBegin);
    QVERIFY(leftWidget.seenTouchUpdate);
    QVERIFY(leftWidget.seenTouchEnd);
    QVERIFY(!rightWidget.seenTouchBegin);
    QVERIFY(!rightWidget.seenTouchUpdate);
    QVERIFY(!rightWidget.seenTouchEnd);
    QCOMPARE(leftWidget.touchEndPoints.count(), 2);
    QCOMPARE(rightWidget.touchEndPoints.count(), 0);
    {
        QTouchEvent::TouchPoint leftTouchPoint = leftWidget.touchEndPoints.at(0);
        QCOMPARE(leftTouchPoint.id(), rawTouchPoints[0].id());
        QCOMPARE(leftTouchPoint.state(), rawTouchPoints[0].state());
        QCOMPARE(leftTouchPoint.pos(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(leftTouchPoint.startPos(), leftPos);
        QCOMPARE(leftTouchPoint.lastPos(), leftTouchPoint.pos());
        QCOMPARE(leftTouchPoint.scenePos(), centerScreenPos);
        QCOMPARE(leftTouchPoint.startScenePos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScenePos(), leftTouchPoint.scenePos());
        QCOMPARE(leftTouchPoint.screenPos(), centerScreenPos);
        QCOMPARE(leftTouchPoint.startScreenPos(), leftScreenPos);
        QCOMPARE(leftTouchPoint.lastScreenPos(), leftTouchPoint.screenPos());
        QCOMPARE(leftTouchPoint.normalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.startNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.lastNormalizedPos(), rawTouchPoints[0].normalizedPos());
        QCOMPARE(leftTouchPoint.rect(), QRectF(leftWidget.mapFromParent(centerPos.toPoint()), QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.sceneRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.screenRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(leftTouchPoint.pressure(), qreal(0.));

        QTouchEvent::TouchPoint rightTouchPoint = leftWidget.touchEndPoints.at(1);
        QCOMPARE(rightTouchPoint.id(), rawTouchPoints[1].id());
        QCOMPARE(rightTouchPoint.state(), rawTouchPoints[1].state());
        QCOMPARE(rightTouchPoint.pos(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(rightTouchPoint.startPos(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.lastPos(), rightTouchPoint.pos());
        QCOMPARE(rightTouchPoint.scenePos(), centerScreenPos);
        QCOMPARE(rightTouchPoint.startScenePos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScenePos(), rightTouchPoint.scenePos());
        QCOMPARE(rightTouchPoint.screenPos(), centerScreenPos);
        QCOMPARE(rightTouchPoint.startScreenPos(), rightScreenPos);
        QCOMPARE(rightTouchPoint.lastScreenPos(), rightTouchPoint.screenPos());
        QCOMPARE(rightTouchPoint.normalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.startNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.lastNormalizedPos(), rawTouchPoints[1].normalizedPos());
        QCOMPARE(rightTouchPoint.rect(), QRectF(leftWidget.mapFromParent(centerPos.toPoint()), QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.sceneRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.screenRect(), QRectF(centerScreenPos, QSizeF(0, 0)));
        QCOMPARE(rightTouchPoint.pressure(), qreal(0.));
    }
}

void tst_QTouchEvent::deleteInEventHandler()
{
    if (!QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // QWidget
    {
        QWidget window;
        QPointer<tst_QTouchEventWidget> child1 = new tst_QTouchEventWidget(&window);
        QPointer<tst_QTouchEventWidget> child2 = new tst_QTouchEventWidget(&window);
        QPointer<tst_QTouchEventWidget> child3 = new tst_QTouchEventWidget(&window);
        child1->setAttribute(Qt::WA_AcceptTouchEvents);
        child2->setAttribute(Qt::WA_AcceptTouchEvents);
        child3->setAttribute(Qt::WA_AcceptTouchEvents);
        child1->deleteInTouchBegin = true;
        child2->deleteInTouchUpdate = true;
        child3->deleteInTouchEnd = true;

        QList<QTouchEvent::TouchPoint> touchPoints;
        touchPoints.append(QTouchEvent::TouchPoint(0));
        QTouchEvent touchBeginEvent(QEvent::TouchBegin,
                                    touchScreenDevice,
                                    Qt::NoModifier,
                                    Qt::TouchPointPressed,
                                    touchPoints);
        QTouchEvent touchUpdateEvent(QEvent::TouchUpdate,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointStationary,
                               touchPoints);
        QTouchEvent touchEndEvent(QEvent::TouchEnd,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointReleased,
                               touchPoints);
        touchBeginEvent.ignore();
        QVERIFY(QApplication::sendEvent(child1, &touchBeginEvent));
        // event is handled, but widget should be deleted
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(child1.isNull());

        touchBeginEvent.ignore();
        QVERIFY(QApplication::sendEvent(child2, &touchBeginEvent));
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(!child2.isNull());
        touchUpdateEvent.ignore();
        QVERIFY(QApplication::sendEvent(child2, &touchUpdateEvent));
        QVERIFY(touchUpdateEvent.isAccepted());
        QVERIFY(child2.isNull());

        touchBeginEvent.ignore();
        QVERIFY(QApplication::sendEvent(child3, &touchBeginEvent));
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(!child3.isNull());
        touchUpdateEvent.ignore();
        QVERIFY(QApplication::sendEvent(child3, &touchUpdateEvent));
        QVERIFY(touchUpdateEvent.isAccepted());
        QVERIFY(!child3.isNull());
        touchEndEvent.ignore();
        QVERIFY(QApplication::sendEvent(child3, &touchEndEvent));
        QVERIFY(touchEndEvent.isAccepted());
        QVERIFY(child3.isNull());
    }

    // QGraphicsView
    {
        QGraphicsScene scene;
        QGraphicsView view(&scene);
        QScopedPointer<tst_QTouchEventGraphicsItem> root(new tst_QTouchEventGraphicsItem);
        tst_QTouchEventGraphicsItem *child1 = new tst_QTouchEventGraphicsItem(root.data());
        tst_QTouchEventGraphicsItem *child2 = new tst_QTouchEventGraphicsItem(root.data());
        tst_QTouchEventGraphicsItem *child3 = new tst_QTouchEventGraphicsItem(root.data());
        child1->setZValue(1.);
        child2->setZValue(0.);
        child3->setZValue(-1.);
        child1->setAcceptTouchEvents(true);
        child2->setAcceptTouchEvents(true);
        child3->setAcceptTouchEvents(true);
        child1->deleteInTouchBegin = true;
        child2->deleteInTouchUpdate = true;
        child3->deleteInTouchEnd = true;

        scene.addItem(root.data());
        view.resize(200, 200);
        view.fitInView(scene.sceneRect());

        QTouchEvent::TouchPoint touchPoint(0);
        touchPoint.setState(Qt::TouchPointPressed);
        touchPoint.setPos(view.mapFromScene(child1->mapToScene(child1->boundingRect().center())));
        touchPoint.setScreenPos(view.mapToGlobal(touchPoint.pos().toPoint()));
        touchPoint.setScenePos(view.mapToScene(touchPoint.pos().toPoint()));
        QList<QTouchEvent::TouchPoint> touchPoints;
        touchPoints.append(touchPoint);
        QTouchEvent touchBeginEvent(QEvent::TouchBegin,
                                    touchScreenDevice,
                                    Qt::NoModifier,
                                    Qt::TouchPointPressed,
                                    touchPoints);
        touchPoints[0].setState(Qt::TouchPointMoved);
        QTouchEvent touchUpdateEvent(QEvent::TouchUpdate,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointMoved,
                               touchPoints);
        touchPoints[0].setState(Qt::TouchPointReleased);
        QTouchEvent touchEndEvent(QEvent::TouchEnd,
                               touchScreenDevice,
                               Qt::NoModifier,
                               Qt::TouchPointReleased,
                               touchPoints);

        child1->weakpointer = &child1;
        touchBeginEvent.ignore();
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchBeginEvent));
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(!child1);
        touchUpdateEvent.ignore();
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchUpdateEvent));
        QVERIFY(!touchUpdateEvent.isAccepted()); // Qt 5.X ignores touch events.
        QVERIFY(!child1);
        touchEndEvent.ignore();
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEndEvent));
        QVERIFY(!touchUpdateEvent.isAccepted());
        QVERIFY(!child1);

        child2->weakpointer = &child2;
        touchBeginEvent.ignore();
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchBeginEvent));
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(child2);
        touchUpdateEvent.ignore();
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchUpdateEvent));
        QVERIFY(!touchUpdateEvent.isAccepted());
        QVERIFY(!child2);
        touchEndEvent.ignore();
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEndEvent));
        QVERIFY(!touchUpdateEvent.isAccepted());
        QVERIFY(!child2);

        child3->weakpointer = &child3;
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchBeginEvent));
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(child3);
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchUpdateEvent));
        QVERIFY(!touchUpdateEvent.isAccepted());
        QVERIFY(child3);
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEndEvent));
        QVERIFY(!touchEndEvent.isAccepted());
        QVERIFY(!child3);
    }
}

void tst_QTouchEvent::deleteInRawEventTranslation()
{
    if (!QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    tst_QTouchEventWidget touchWidget;
    touchWidget.setWindowTitle(QTest::currentTestFunction());
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 300, 300);

    QPointer<tst_QTouchEventWidget> leftWidget = new tst_QTouchEventWidget(&touchWidget);
    leftWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    leftWidget->setGeometry(0, 100, 100, 100);
    leftWidget->deleteInTouchBegin = true;

    QPointer<tst_QTouchEventWidget> centerWidget = new tst_QTouchEventWidget(&touchWidget);
    centerWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    centerWidget->setGeometry(100, 100, 100, 100);
    centerWidget->deleteInTouchUpdate = true;

    QPointer<tst_QTouchEventWidget> rightWidget = new tst_QTouchEventWidget(&touchWidget);
    rightWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    rightWidget->setGeometry(200, 100, 100, 100);
    rightWidget->deleteInTouchEnd = true;

    touchWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&touchWidget));

    QPointF leftPos = leftWidget->rect().center();
    QPointF centerPos = centerWidget->rect().center();
    QPointF rightPos = rightWidget->rect().center();
    QPointF leftScreenPos = leftWidget->mapToGlobal(leftPos.toPoint());
    QPointF centerScreenPos = centerWidget->mapToGlobal(centerPos.toPoint());
    QPointF rightScreenPos = rightWidget->mapToGlobal(rightPos.toPoint());
    QRectF screenGeometry = QApplication::desktop()->screenGeometry(&touchWidget);

    QList<QTouchEvent::TouchPoint> rawTouchPoints;
    rawTouchPoints.append(QTouchEvent::TouchPoint(0));
    rawTouchPoints.append(QTouchEvent::TouchPoint(1));
    rawTouchPoints.append(QTouchEvent::TouchPoint(2));
    rawTouchPoints[0].setState(Qt::TouchPointPressed);
    rawTouchPoints[0].setScreenPos(leftScreenPos);
    rawTouchPoints[0].setNormalizedPos(normalized(rawTouchPoints[0].pos(), screenGeometry));
    rawTouchPoints[1].setState(Qt::TouchPointPressed);
    rawTouchPoints[1].setScreenPos(centerScreenPos);
    rawTouchPoints[1].setNormalizedPos(normalized(rawTouchPoints[1].pos(), screenGeometry));
    rawTouchPoints[2].setState(Qt::TouchPointPressed);
    rawTouchPoints[2].setScreenPos(rightScreenPos);
    rawTouchPoints[2].setNormalizedPos(normalized(rawTouchPoints[2].pos(), screenGeometry));

    // generate begin events on all widgets, the left widget should die
    QWindow *window = touchWidget.windowHandle();
    QList<QWindowSystemInterface::TouchPoint> nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(leftWidget.isNull());
    QVERIFY(!centerWidget.isNull());
    QVERIFY(!rightWidget.isNull());

    // generate update events on all widget, the center widget should die
    rawTouchPoints[0].setState(Qt::TouchPointMoved);
    rawTouchPoints[1].setState(Qt::TouchPointMoved);
    rawTouchPoints[2].setState(Qt::TouchPointMoved);
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();

    // generate end events on all widget, the right widget should die
    rawTouchPoints[0].setState(Qt::TouchPointReleased);
    rawTouchPoints[1].setState(Qt::TouchPointReleased);
    rawTouchPoints[2].setState(Qt::TouchPointReleased);
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, 0, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
}

void tst_QTouchEvent::crashInQGraphicsSceneAfterNotHandlingTouchBegin()
{
    QGraphicsRectItem *rect = new QGraphicsRectItem(0, 0, 100, 100);
    rect->setAcceptTouchEvents(true);

    QGraphicsRectItem *mainRect = new QGraphicsRectItem(0, 0, 100, 100, rect);
    mainRect->setBrush(Qt::lightGray);

    QGraphicsRectItem *button = new QGraphicsRectItem(-20, -20, 40, 40, mainRect);
    button->setPos(50, 50);
    button->setBrush(Qt::darkGreen);

    QGraphicsView view;
    QGraphicsScene scene;
    scene.addItem(rect);
    scene.setSceneRect(0,0,100,100);
    view.setScene(&scene);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QPoint centerPos = view.mapFromScene(rect->boundingRect().center());
    // Touch the button
    QTest::touchEvent(view.viewport(), touchScreenDevice).press(0, centerPos, static_cast<QWindow *>(0));
    QTest::touchEvent(view.viewport(), touchScreenDevice).release(0, centerPos, static_cast<QWindow *>(0));
    // Touch outside of the button
    QTest::touchEvent(view.viewport(), touchScreenDevice).press(0, view.mapFromScene(QPoint(10, 10)), static_cast<QWindow *>(0));
    QTest::touchEvent(view.viewport(), touchScreenDevice).release(0, view.mapFromScene(QPoint(10, 10)), static_cast<QWindow *>(0));
}

void tst_QTouchEvent::touchBeginWithGraphicsWidget()
{
    if (QHighDpiScaling::isActive())
        QSKIP("Fails when scaling is active");
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    QScopedPointer<tst_QTouchEventGraphicsItem> root(new tst_QTouchEventGraphicsItem);
    root->setAcceptTouchEvents(true);
    scene.addItem(root.data());

    QScopedPointer<QGraphicsWidget> glassWidget(new QGraphicsWidget);
    glassWidget->setMinimumSize(100, 100);
    scene.addItem(glassWidget.data());

    view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    view.resize(availableGeometry.size() - QSize(100, 100));
    view.move(availableGeometry.topLeft() + QPoint(50, 50));
    view.fitInView(scene.sceneRect());
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QTest::touchEvent(&view, touchScreenDevice)
            .press(0, view.mapFromScene(root->mapToScene(3,3)), view.viewport());
    QTest::touchEvent(&view, touchScreenDevice)
            .stationary(0)
            .press(1, view.mapFromScene(root->mapToScene(6,6)), view.viewport());
    QTest::touchEvent(&view, touchScreenDevice)
            .release(0, view.mapFromScene(root->mapToScene(3,3)), view.viewport())
            .release(1, view.mapFromScene(root->mapToScene(6,6)), view.viewport());

    QTRY_COMPARE(root->touchBeginCounter, 1);
    QCOMPARE(root->touchUpdateCounter, 1);
    QCOMPARE(root->touchEndCounter, 1);
    QCOMPARE(root->touchUpdatePoints.size(), 2);

    root->reset();
    glassWidget->setWindowFlags(Qt::Window); // make the glassWidget a panel

    QTest::touchEvent(&view, touchScreenDevice)
            .press(0, view.mapFromScene(root->mapToScene(3,3)), view.viewport());
    QTest::touchEvent(&view, touchScreenDevice)
            .stationary(0)
            .press(1, view.mapFromScene(root->mapToScene(6,6)), view.viewport());
    QTest::touchEvent(&view, touchScreenDevice)
            .release(0, view.mapFromScene(root->mapToScene(3,3)), view.viewport())
            .release(1, view.mapFromScene(root->mapToScene(6,6)), view.viewport());

    QCOMPARE(root->touchBeginCounter, 0);
    QCOMPARE(root->touchUpdateCounter, 0);
    QCOMPARE(root->touchEndCounter, 0);
}

class WindowTouchEventFilter : public QObject
{
    Q_OBJECT
public:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;
    struct TouchInfo {
        QList<QTouchEvent::TouchPoint> points;
        QEvent::Type lastSeenType;
    };
    QMap<QTouchDevice *, TouchInfo> d;
};

bool WindowTouchEventFilter::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::TouchBegin
            || event->type() == QEvent::TouchUpdate
            || event->type() == QEvent::TouchEnd) {
        QTouchEvent *te = static_cast<QTouchEvent *>(event);
        TouchInfo &td = d[te->device()];
        if (event->type() == QEvent::TouchBegin)
            td.points.clear();
        td.points.append(te->touchPoints());
        td.lastSeenType = event->type();
    }
    return false;
}

void tst_QTouchEvent::testQGuiAppDelivery()
{
    QWindow w;
    w.setGeometry(100, 100, 100, 100);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    WindowTouchEventFilter filter;
    w.installEventFilter(&filter);

    QList<QWindowSystemInterface::TouchPoint> points;

    // Pass empty list, should be ignored.
    QWindowSystemInterface::handleTouchEvent(&w, 0, points);
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.isEmpty(), true);

    QWindowSystemInterface::TouchPoint tp;
    tp.id = 0;
    tp.state = Qt::TouchPointPressed;
    tp.area = QRectF(120, 120, 20, 20);
    points.append(tp);

    // Pass 0 as device, should be ignored.
    QWindowSystemInterface::handleTouchEvent(&w, 0, points);
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.isEmpty(), true);

    // Now the real thing.
    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, points); // TouchBegin
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.count(), 1);
    QCOMPARE(filter.d.contains(touchScreenDevice), true);
    QCOMPARE(filter.d.value(touchScreenDevice).points.count(), 1);
    QCOMPARE(filter.d.value(touchScreenDevice).lastSeenType, QEvent::TouchBegin);

    points[0].state = Qt::TouchPointMoved;
    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, points); // TouchUpdate
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.count(), 1);
    QCOMPARE(filter.d.contains(touchScreenDevice), true);
    QCOMPARE(filter.d.value(touchScreenDevice).points.count(), 2);
    QCOMPARE(filter.d.value(touchScreenDevice).lastSeenType, QEvent::TouchUpdate);

    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, points); // TouchEnd
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.count(), 1);
    QCOMPARE(filter.d.contains(touchScreenDevice), true);
    QCOMPARE(filter.d.value(touchScreenDevice).points.count(), 3);
    QCOMPARE(filter.d.value(touchScreenDevice).lastSeenType, QEvent::TouchEnd);
}

void tst_QTouchEvent::testMultiDevice()
{
    QTouchDevice *deviceTwo = QTest::createTouchDevice();

    QWindow w;
    w.setGeometry(100, 100, 100, 100);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    WindowTouchEventFilter filter;
    w.installEventFilter(&filter);

    QList<QWindowSystemInterface::TouchPoint> pointsOne, pointsTwo;

    // touchScreenDevice reports a single point, deviceTwo reports the beginning of a multi-point sequence.
    // Even though there is a point with id 0 for both devices, they should be delivered cleanly, independently.
    QWindowSystemInterface::TouchPoint tp;
    tp.id = 0;
    tp.state = Qt::TouchPointPressed;
    const QPoint screenOrigin = w.screen()->geometry().topLeft();
    const QRect area0(120, 120, 20, 20);
    tp.area = QHighDpi::toNative(area0, QHighDpiScaling::factor(&w), screenOrigin);
    pointsOne.append(tp);

    pointsTwo.append(tp);
    tp.id = 1;
    const QRect area1(140, 140, 20, 20);
    tp.area = QHighDpi::toNative(area1, QHighDpiScaling::factor(&w), screenOrigin);
    pointsTwo.append(tp);

    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, pointsOne);
    QWindowSystemInterface::handleTouchEvent(&w, deviceTwo, pointsTwo);
    QCoreApplication::processEvents();

    QCOMPARE(filter.d.contains(touchScreenDevice), true);
    QCOMPARE(filter.d.contains(deviceTwo), true);

    QCOMPARE(filter.d.value(touchScreenDevice).lastSeenType, QEvent::TouchBegin);
    QCOMPARE(filter.d.value(deviceTwo).lastSeenType, QEvent::TouchBegin);
    QCOMPARE(filter.d.value(touchScreenDevice).points.count(), 1);
    QCOMPARE(filter.d.value(deviceTwo).points.count(), 2);

    QCOMPARE(filter.d.value(touchScreenDevice).points.at(0).screenRect(), QRectF(area0));
    QCOMPARE(filter.d.value(touchScreenDevice).points.at(0).state(), pointsOne[0].state);

    QCOMPARE(filter.d.value(deviceTwo).points.at(0).screenRect(), QRectF(area0));
    QCOMPARE(filter.d.value(deviceTwo).points.at(0).state(), pointsTwo[0].state);
    QCOMPARE(filter.d.value(deviceTwo).points.at(1).screenRect(), QRectF(area1));
    QCOMPARE(filter.d.value(deviceTwo).points.at(1).state(), pointsTwo[1].state);
}

QTEST_MAIN(tst_QTouchEvent)

#include "tst_qtouchevent.moc"
