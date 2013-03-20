/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>
#include <QtWidgets>
#include <QtTest>
#include <qpa/qwindowsysteminterface.h>

static QWindowSystemInterface::TouchPoint touchPoint(const QTouchEvent::TouchPoint& pt)
{
    QWindowSystemInterface::TouchPoint p;
    p.id = pt.id();
    p.flags = pt.flags();
    p.normalPosition = pt.normalizedPos();
    p.area = pt.screenRect();
    p.pressure = pt.pressure();
    p.state = pt.state();
    p.velocity = pt.velocity();
    p.rawPositions = pt.rawScreenPositions();
    return p;
}

static QList<struct QWindowSystemInterface::TouchPoint> touchPointList(const QList<QTouchEvent::TouchPoint>& pointList)
{
    QList<struct QWindowSystemInterface::TouchPoint> newList;

    Q_FOREACH (QTouchEvent::TouchPoint p, pointList)
    {
        newList.append(touchPoint(p));
    }
    return newList;
}


class tst_QTouchEventWidget : public QWidget
{
public:
    QList<QTouchEvent::TouchPoint> touchBeginPoints, touchUpdatePoints, touchEndPoints;
    bool seenTouchBegin, seenTouchUpdate, seenTouchEnd;
    bool acceptTouchBegin, acceptTouchUpdate, acceptTouchEnd;
    bool deleteInTouchBegin, deleteInTouchUpdate, deleteInTouchEnd;
    ulong timestamp;
    QTouchDevice *deviceFromEvent;

    tst_QTouchEventWidget()
        : QWidget()
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

    bool event(QEvent *event)
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

    tst_QTouchEventGraphicsItem()
        : QGraphicsItem(), weakpointer(0)
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

    QRectF boundingRect() const { return QRectF(0, 0, 10, 10); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) { }

    bool sceneEvent(QEvent *event)
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
    ~tst_QTouchEvent() { }

private slots:
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
{
    touchScreenDevice = new QTouchDevice;
    touchPadDevice = new QTouchDevice;
    touchPadDevice->setType(QTouchDevice::TouchPad);
    QWindowSystemInterface::registerTouchDevice(touchScreenDevice);
    QWindowSystemInterface::registerTouchDevice(touchPadDevice);
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
        bool res = QApplication::sendEvent(&widget, &touchEvent);
        QVERIFY(!res);
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
        bool res = QApplication::sendEvent(view.viewport(), &touchEvent);
        QVERIFY(!res);
        QVERIFY(!touchEvent.isAccepted());
        QVERIFY(!item.seenTouchBegin);
    }
}

void tst_QTouchEvent::touchEventAcceptedByDefault()
{
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
        bool res = QApplication::sendEvent(&widget, &touchEvent);
        QVERIFY(res);
        QVERIFY(touchEvent.isAccepted());

        // tst_QTouchEventWidget does handle, sending succeeds
        tst_QTouchEventWidget touchWidget;
        touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
        touchEvent.ignore();
        res = QApplication::sendEvent(&touchWidget, &touchEvent);
        QVERIFY(res);
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
        bool res = QApplication::sendEvent(view.viewport(), &touchEvent);
        QVERIFY(res);
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
        bool res = QApplication::sendEvent(&grandchild, &touchEvent);
        QVERIFY(res);
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
        res = QApplication::sendEvent(&grandchild, &touchEvent);
        QVERIFY(res);
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
        bool res = QApplication::sendEvent(view.viewport(), &touchEvent);
        QVERIFY(res);
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
        bool res = QApplication::sendEvent(view.viewport(), &touchEvent);
        QVERIFY(res);
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
        bool res = QApplication::sendEvent(&child, &touchBeginEvent);
        QVERIFY(res);
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(child.seenTouchBegin);
        QVERIFY(!window.seenTouchBegin);

        // send the touch update to the child, but ignore it, it doesn't propagate
        QTouchEvent touchUpdateEvent(QEvent::TouchUpdate,
                                     touchScreenDevice,
                                     Qt::NoModifier,
                                     Qt::TouchPointMoved,
                                     touchPoints);
        res = QApplication::sendEvent(&child, &touchUpdateEvent);
        QVERIFY(res);
        QVERIFY(!touchUpdateEvent.isAccepted());
        QVERIFY(child.seenTouchUpdate);
        QVERIFY(!window.seenTouchUpdate);

        // send the touch end, same thing should happen as with touch update
        QTouchEvent touchEndEvent(QEvent::TouchEnd,
                                  touchScreenDevice,
                                  Qt::NoModifier,
                                  Qt::TouchPointReleased,
                                  touchPoints);
        res = QApplication::sendEvent(&child, &touchEndEvent);
        QVERIFY(res);
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
        bool res = QApplication::sendEvent(view.viewport(), &touchBeginEvent);
        QVERIFY(res);
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
        res = QApplication::sendEvent(view.viewport(), &touchUpdateEvent);
        QVERIFY(res);
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
        res = QApplication::sendEvent(view.viewport(), &touchEndEvent);
        QVERIFY(res);
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
    tst_QTouchEventWidget touchWidget;
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);

    QPointF pos = touchWidget.rect().center();
    QPointF screenPos = touchWidget.mapToGlobal(pos.toPoint());
    QPointF delta(10, 10);
    QRectF screenGeometry = qApp->desktop()->screenGeometry(&touchWidget);

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
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             timestamp,
                                             touchScreenDevice,
                                             touchPointList(
                                                 QList<QTouchEvent::TouchPoint>() << rawTouchPoint));
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
    QCOMPARE(touchBeginPoint.rawScreenPositions(), rawPosList);

    // moving the point should translate to TouchUpdate
    rawTouchPoint.setState(Qt::TouchPointMoved);
    rawTouchPoint.setScreenPos(screenPos + delta);
    rawTouchPoint.setNormalizedPos(normalized(rawTouchPoint.pos(), screenGeometry));
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchScreenDevice,
                                             touchPointList(QList<QTouchEvent::TouchPoint>() << rawTouchPoint));
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
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchScreenDevice,
                                             touchPointList(QList<QTouchEvent::TouchPoint>() << rawTouchPoint));
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
    tst_QTouchEventWidget touchWidget;
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);

    tst_QTouchEventWidget leftWidget;
    leftWidget.setParent(&touchWidget);
    leftWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    leftWidget.setGeometry(0, 100, 100, 100);
    leftWidget.show();

    tst_QTouchEventWidget rightWidget;
    rightWidget.setParent(&touchWidget);
    rightWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    rightWidget.setGeometry(300, 100, 100, 100);
    rightWidget.show();

    QPointF leftPos = leftWidget.rect().center();
    QPointF rightPos = rightWidget.rect().center();
    QPointF centerPos = touchWidget.rect().center();
    QPointF leftScreenPos = leftWidget.mapToGlobal(leftPos.toPoint());
    QPointF rightScreenPos = rightWidget.mapToGlobal(rightPos.toPoint());
    QPointF centerScreenPos = touchWidget.mapToGlobal(centerPos.toPoint());
    QPointF delta(10, 10);
    QRectF screenGeometry = qApp->desktop()->screenGeometry(&touchWidget);

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
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchScreenDevice,
                                             touchPointList(rawTouchPoints));
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
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchScreenDevice,
                                             touchPointList(rawTouchPoints));
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
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchScreenDevice,
                                             touchPointList(rawTouchPoints));
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
    tst_QTouchEventWidget touchWidget;
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);

    tst_QTouchEventWidget leftWidget;
    leftWidget.setParent(&touchWidget);
    leftWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    leftWidget.setGeometry(0, 100, 100, 100);
    leftWidget.show();

    tst_QTouchEventWidget rightWidget;
    rightWidget.setParent(&touchWidget);
    rightWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    rightWidget.setGeometry(300, 100, 100, 100);
    rightWidget.show();

    QPointF leftPos = leftWidget.rect().center();
    QPointF rightPos = rightWidget.rect().center();
    QPointF centerPos = touchWidget.rect().center();
    QPointF leftScreenPos = leftWidget.mapToGlobal(leftPos.toPoint());
    QPointF rightScreenPos = rightWidget.mapToGlobal(rightPos.toPoint());
    QPointF centerScreenPos = touchWidget.mapToGlobal(centerPos.toPoint());
    QPointF delta(10, 10);
    QRectF screenGeometry = qApp->desktop()->screenGeometry(&touchWidget);

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
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchPadDevice,
                                             touchPointList(rawTouchPoints));
    QCoreApplication::processEvents();
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QVERIFY(leftWidget.seenTouchBegin);
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
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchPadDevice,
                                             touchPointList(rawTouchPoints));
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
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchPadDevice,
                                             touchPointList(rawTouchPoints));
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
    // QWidget
    {
        QWidget window;
        tst_QTouchEventWidget *child1, *child2, *child3;
        child1 = new tst_QTouchEventWidget;
        child2 = new tst_QTouchEventWidget;
        child3 = new tst_QTouchEventWidget;
        child1->setParent(&window);
        child2->setParent(&window);
        child3->setParent(&window);
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
        QPointer<QWidget> p;
        bool res;

        touchBeginEvent.ignore();
        p = child1;
        res = QApplication::sendEvent(child1, &touchBeginEvent);
        // event is handled, but widget should be deleted
        QVERIFY(res && touchBeginEvent.isAccepted() && p.isNull());

        touchBeginEvent.ignore();
        p = child2;
        res = QApplication::sendEvent(child2, &touchBeginEvent);
        QVERIFY(res && touchBeginEvent.isAccepted() && !p.isNull());
        touchUpdateEvent.ignore();
        res = QApplication::sendEvent(child2, &touchUpdateEvent);
        QVERIFY(res && touchUpdateEvent.isAccepted() && p.isNull());

        touchBeginEvent.ignore();
        p = child3;
        res = QApplication::sendEvent(child3, &touchBeginEvent);
        QVERIFY(res && touchBeginEvent.isAccepted() && !p.isNull());
        touchUpdateEvent.ignore();
        res = QApplication::sendEvent(child3, &touchUpdateEvent);
        QVERIFY(res && touchUpdateEvent.isAccepted() && !p.isNull());
        touchEndEvent.ignore();
        res = QApplication::sendEvent(child3, &touchEndEvent);
        QVERIFY(res && touchEndEvent.isAccepted() && p.isNull());
    }

    // QGraphicsView
    {
        QGraphicsScene scene;
        QGraphicsView view(&scene);
        tst_QTouchEventGraphicsItem *root, *child1, *child2, *child3;
        root = new tst_QTouchEventGraphicsItem;
        child1 = new tst_QTouchEventGraphicsItem;
        child2 = new tst_QTouchEventGraphicsItem;
        child3 = new tst_QTouchEventGraphicsItem;
        child1->setParentItem(root);
        child2->setParentItem(root);
        child3->setParentItem(root);
        child1->setZValue(1.);
        child2->setZValue(0.);
        child3->setZValue(-1.);
        child1->setAcceptTouchEvents(true);
        child2->setAcceptTouchEvents(true);
        child3->setAcceptTouchEvents(true);
        child1->deleteInTouchBegin = true;
        child2->deleteInTouchUpdate = true;
        child3->deleteInTouchEnd = true;

        scene.addItem(root);
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
        bool res;

        child1->weakpointer = &child1;
        touchBeginEvent.ignore();
        res = QApplication::sendEvent(view.viewport(), &touchBeginEvent);
        QVERIFY(res && touchBeginEvent.isAccepted() && !child1);
        touchUpdateEvent.ignore();
        res = QApplication::sendEvent(view.viewport(), &touchUpdateEvent);
        QVERIFY(res && touchUpdateEvent.isAccepted() && !child1);
        touchEndEvent.ignore();
        res = QApplication::sendEvent(view.viewport(), &touchEndEvent);
        QVERIFY(res && touchUpdateEvent.isAccepted() && !child1);

        child2->weakpointer = &child2;
        touchBeginEvent.ignore();
        res = QApplication::sendEvent(view.viewport(), &touchBeginEvent);
        QVERIFY(res && touchBeginEvent.isAccepted() && child2);
        touchUpdateEvent.ignore();
        res = QApplication::sendEvent(view.viewport(), &touchUpdateEvent);
        QVERIFY(res && !touchUpdateEvent.isAccepted() && !child2);
        touchEndEvent.ignore();
        res = QApplication::sendEvent(view.viewport(), &touchEndEvent);
        QVERIFY(res && !touchUpdateEvent.isAccepted() && !child2);

        child3->weakpointer = &child3;
        res = QApplication::sendEvent(view.viewport(), &touchBeginEvent);
        QVERIFY(res && touchBeginEvent.isAccepted() && child3);
        res = QApplication::sendEvent(view.viewport(), &touchUpdateEvent);
        QVERIFY(res && !touchUpdateEvent.isAccepted() && child3);
        res = QApplication::sendEvent(view.viewport(), &touchEndEvent);
        QVERIFY(res && !touchEndEvent.isAccepted() && !child3);

        delete root;
    }
}

void tst_QTouchEvent::deleteInRawEventTranslation()
{
    tst_QTouchEventWidget touchWidget;
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 300, 300);

    tst_QTouchEventWidget *leftWidget = new tst_QTouchEventWidget;
    leftWidget->setParent(&touchWidget);
    leftWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    leftWidget->setGeometry(0, 100, 100, 100);
    leftWidget->deleteInTouchBegin = true;
    leftWidget->show();

    tst_QTouchEventWidget *centerWidget = new tst_QTouchEventWidget;
    centerWidget->setParent(&touchWidget);
    centerWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    centerWidget->setGeometry(100, 100, 100, 100);
    centerWidget->deleteInTouchUpdate = true;
    centerWidget->show();

    tst_QTouchEventWidget *rightWidget = new tst_QTouchEventWidget;
    rightWidget->setParent(&touchWidget);
    rightWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    rightWidget->setGeometry(200, 100, 100, 100);
    rightWidget->deleteInTouchEnd = true;
    rightWidget->show();

    QPointF leftPos = leftWidget->rect().center();
    QPointF centerPos = centerWidget->rect().center();
    QPointF rightPos = rightWidget->rect().center();
    QPointF leftScreenPos = leftWidget->mapToGlobal(leftPos.toPoint());
    QPointF centerScreenPos = centerWidget->mapToGlobal(centerPos.toPoint());
    QPointF rightScreenPos = rightWidget->mapToGlobal(rightPos.toPoint());
    QRectF screenGeometry = qApp->desktop()->screenGeometry(&touchWidget);

    QPointer<QWidget> pl = leftWidget, pc = centerWidget, pr = rightWidget;

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
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchScreenDevice,
                                             touchPointList(rawTouchPoints));
    QCoreApplication::processEvents();
    QVERIFY(pl.isNull() && !pc.isNull() && !pr.isNull());

    // generate update events on all widget, the center widget should die
    rawTouchPoints[0].setState(Qt::TouchPointMoved);
    rawTouchPoints[1].setState(Qt::TouchPointMoved);
    rawTouchPoints[2].setState(Qt::TouchPointMoved);
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchScreenDevice,
                                             touchPointList(rawTouchPoints));
    QCoreApplication::processEvents();

    // generate end events on all widget, the right widget should die
    rawTouchPoints[0].setState(Qt::TouchPointReleased);
    rawTouchPoints[1].setState(Qt::TouchPointReleased);
    rawTouchPoints[2].setState(Qt::TouchPointReleased);
    QWindowSystemInterface::handleTouchEvent(touchWidget.windowHandle(),
                                             0,
                                             touchScreenDevice,
                                             touchPointList(rawTouchPoints));
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
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    tst_QTouchEventGraphicsItem *root;
    root = new tst_QTouchEventGraphicsItem;
    root->setAcceptTouchEvents(true);
    scene.addItem(root);

    QGraphicsWidget *glassWidget = new QGraphicsWidget;
    glassWidget->setMinimumSize(100, 100);
    scene.addItem(glassWidget);

    view.resize(200, 200);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.fitInView(scene.sceneRect());

    QTest::touchEvent(&view, touchScreenDevice)
            .press(0, view.mapFromScene(root->mapToScene(3,3)), view.viewport());
    QTest::touchEvent(&view, touchScreenDevice)
            .stationary(0)
            .press(1, view.mapFromScene(root->mapToScene(6,6)), view.viewport());
    QTest::touchEvent(&view, touchScreenDevice)
            .release(0, view.mapFromScene(root->mapToScene(3,3)), view.viewport())
            .release(1, view.mapFromScene(root->mapToScene(6,6)), view.viewport());

    QCOMPARE(root->touchBeginCounter, 1);
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


    delete root;
    delete glassWidget;
}

class WindowTouchEventFilter : public QObject
{
    Q_OBJECT
public:
    bool eventFilter(QObject *obj, QEvent *event);
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
    QTouchDevice *device = new QTouchDevice;
    device->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(device);

    QWindow *w = new QWindow;
    w->setGeometry(100, 100, 100, 100);
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));

    WindowTouchEventFilter filter;
    w->installEventFilter(&filter);

    QList<QWindowSystemInterface::TouchPoint> points;

    // Pass empty list, should be ignored.
    QWindowSystemInterface::handleTouchEvent(w, 0, points);
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.isEmpty(), true);

    QWindowSystemInterface::TouchPoint tp;
    tp.id = 0;
    tp.state = Qt::TouchPointPressed;
    tp.area = QRectF(120, 120, 20, 20);
    points.append(tp);

    // Pass 0 as device, should be ignored.
    QWindowSystemInterface::handleTouchEvent(w, 0, points);
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.isEmpty(), true);

    // Now the real thing.
    QWindowSystemInterface::handleTouchEvent(w, device, points); // TouchBegin
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.count(), 1);
    QCOMPARE(filter.d.contains(device), true);
    QCOMPARE(filter.d.value(device).points.count(), 1);
    QCOMPARE(filter.d.value(device).lastSeenType, QEvent::TouchBegin);

    points[0].state = Qt::TouchPointMoved;
    QWindowSystemInterface::handleTouchEvent(w, device, points); // TouchUpdate
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.count(), 1);
    QCOMPARE(filter.d.contains(device), true);
    QCOMPARE(filter.d.value(device).points.count(), 2);
    QCOMPARE(filter.d.value(device).lastSeenType, QEvent::TouchUpdate);

    points[0].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(w, device, points); // TouchEnd
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.count(), 1);
    QCOMPARE(filter.d.contains(device), true);
    QCOMPARE(filter.d.value(device).points.count(), 3);
    QCOMPARE(filter.d.value(device).lastSeenType, QEvent::TouchEnd);
}

void tst_QTouchEvent::testMultiDevice()
{
    QTouchDevice *deviceOne = new QTouchDevice;
    deviceOne->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(deviceOne);
    QTouchDevice *deviceTwo = new QTouchDevice;
    deviceTwo->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(deviceTwo);

    QWindow *w = new QWindow;
    w->setGeometry(100, 100, 100, 100);
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));

    WindowTouchEventFilter filter;
    w->installEventFilter(&filter);

    QList<QWindowSystemInterface::TouchPoint> pointsOne, pointsTwo;

    // deviceOne reports a single point, deviceTwo reports the beginning of a multi-point sequence.
    // Even though there is a point with id 0 for both devices, they should be delivered cleanly, independently.
    QWindowSystemInterface::TouchPoint tp;
    tp.id = 0;
    tp.state = Qt::TouchPointPressed;
    tp.area = QRectF(120, 120, 20, 20);
    pointsOne.append(tp);

    pointsTwo.append(tp);
    tp.id = 1;
    tp.area = QRectF(140, 140, 20, 20);
    pointsTwo.append(tp);

    QWindowSystemInterface::handleTouchEvent(w, deviceOne, pointsOne);
    QWindowSystemInterface::handleTouchEvent(w, deviceTwo, pointsTwo);
    QCoreApplication::processEvents();

    QCOMPARE(filter.d.contains(deviceOne), true);
    QCOMPARE(filter.d.contains(deviceTwo), true);

    QCOMPARE(filter.d.value(deviceOne).lastSeenType, QEvent::TouchBegin);
    QCOMPARE(filter.d.value(deviceTwo).lastSeenType, QEvent::TouchBegin);
    QCOMPARE(filter.d.value(deviceOne).points.count(), 1);
    QCOMPARE(filter.d.value(deviceTwo).points.count(), 2);

    QCOMPARE(filter.d.value(deviceOne).points.at(0).screenRect(), pointsOne[0].area);
    QCOMPARE(filter.d.value(deviceOne).points.at(0).state(), pointsOne[0].state);

    QCOMPARE(filter.d.value(deviceTwo).points.at(0).screenRect(), pointsTwo[0].area);
    QCOMPARE(filter.d.value(deviceTwo).points.at(0).state(), pointsTwo[0].state);
    QCOMPARE(filter.d.value(deviceTwo).points.at(1).screenRect(), pointsTwo[1].area);
    QCOMPARE(filter.d.value(deviceTwo).points.at(1).state(), pointsTwo[1].state);
}

QTEST_MAIN(tst_QTouchEvent)

#include "tst_qtouchevent.moc"
