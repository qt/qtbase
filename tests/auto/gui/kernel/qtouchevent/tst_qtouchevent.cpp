// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui/QCursor>
#include <QtGui/QScreen>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsWidget>
#include <QtWidgets/QWidget>
#include <QTest>
#include <QSet>
#include <QVarLengthArray>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <private/qevent_p.h>
#include <QtGui/private/qeventpoint_p.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qpointingdevice_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

Q_LOGGING_CATEGORY(lcTests, "qt.gui.tests")

class tst_QTouchEventWidget : public QWidget
{
public:
    QList<QEventPoint> touchBeginPoints, touchUpdatePoints, touchEndPoints;
    QList<QPointF> lastNormalizedPositions;
    bool seenTouchBegin, seenTouchUpdate, seenTouchEnd;
    bool acceptTouchBegin, acceptTouchUpdate, acceptTouchEnd;
    bool deleteInTouchBegin, deleteInTouchUpdate, deleteInTouchEnd;
    ulong timestamp;
    const QPointingDevice *deviceFromEvent;

    explicit tst_QTouchEventWidget(QWidget *parent = nullptr) : QWidget(parent)
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

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.drawRect(rect());
        painter.setPen(Qt::darkGray);
        painter.drawText(rect(), Qt::AlignHCenter | Qt::AlignCenter, objectName());
        static const QString pointFormat = QString::fromUtf8("\360\237\226\227 %1, %2");
        painter.setPen(Qt::darkGreen);
        for (const auto &pt : std::as_const(touchBeginPoints))
            painter.drawText(pt.position(), pointFormat.arg(pt.position().toPoint().x()).arg(pt.position().toPoint().y()));
        painter.setPen(Qt::darkYellow);
        for (const auto &pt : std::as_const(touchUpdatePoints))
            painter.drawText(pt.position(), pointFormat.arg(pt.position().toPoint().x()).arg(pt.position().toPoint().y()));
        painter.setPen(Qt::darkRed);
        for (const auto &pt : std::as_const(touchEndPoints))
            painter.drawText(pt.position(), pointFormat.arg(pt.position().toPoint().x()).arg(pt.position().toPoint().y()));
    }

    bool event(QEvent *event) override
    {
        lastNormalizedPositions.clear();
        switch (event->type()) {
        case QEvent::TouchBegin: {
            qCDebug(lcTests) << objectName() << event;
            if (seenTouchBegin) qWarning("TouchBegin: already seen a TouchBegin");
            if (seenTouchUpdate) qWarning("TouchBegin: TouchUpdate cannot happen before TouchBegin");
            if (seenTouchEnd) qWarning("TouchBegin: TouchEnd cannot happen before TouchBegin");
            seenTouchBegin = !seenTouchBegin && !seenTouchUpdate && !seenTouchEnd;
            auto touchEvent = static_cast<QTouchEvent *>(event);
            touchBeginPoints = touchEvent->points();
            Q_ASSERT(touchBeginPoints.first().device() == touchEvent->pointingDevice());
            for (const QEventPoint &pt : std::as_const(touchBeginPoints))
                lastNormalizedPositions << pt.normalizedPosition();
            timestamp = touchEvent->timestamp();
            deviceFromEvent = touchEvent->pointingDevice();
            event->setAccepted(acceptTouchBegin);
            if (deleteInTouchBegin)
                delete this;
            else
                update();
            break;
        }
        case QEvent::TouchUpdate: {
            qCDebug(lcTests) << objectName() << event;
            if (!seenTouchBegin) qWarning("TouchUpdate: have not seen TouchBegin");
            if (seenTouchEnd) qWarning("TouchUpdate: TouchEnd cannot happen before TouchUpdate");
            seenTouchUpdate = seenTouchBegin && !seenTouchEnd;
            auto touchEvent = static_cast<QTouchEvent *>(event);
            touchUpdatePoints = touchEvent->points();
            for (const QEventPoint &pt : std::as_const(touchUpdatePoints))
                lastNormalizedPositions << pt.normalizedPosition();
            timestamp = touchEvent->timestamp();
            deviceFromEvent = touchEvent->pointingDevice();
            event->setAccepted(acceptTouchUpdate);
            if (deleteInTouchUpdate)
                delete this;
            else
                update();
            break;
        }
        case QEvent::TouchEnd: {
            qCDebug(lcTests) << objectName() << event;
            if (!seenTouchBegin) qWarning("TouchEnd: have not seen TouchBegin");
            if (seenTouchEnd) qWarning("TouchEnd: already seen a TouchEnd");
            seenTouchEnd = seenTouchBegin && !seenTouchEnd;
            auto touchEvent = static_cast<QTouchEvent *>(event);
            touchEndPoints = touchEvent->points();
            for (const QEventPoint &pt : std::as_const(touchEndPoints))
                lastNormalizedPositions << pt.normalizedPosition();
            timestamp = touchEvent->timestamp();
            deviceFromEvent = touchEvent->pointingDevice();
            event->setAccepted(acceptTouchEnd);
            if (deleteInTouchEnd)
                delete this;
            else
                update();
            break;
        }
        default:
            return QWidget::event(event);
        }
        return true;
    }
};

class tst_QTouchEventGraphicsItem : public QGraphicsItem
{
public:
    QList<QEventPoint> touchBeginPoints, touchUpdatePoints, touchEndPoints;
    bool seenTouchBegin, seenTouchUpdate, seenTouchEnd;
    int touchBeginCounter, touchUpdateCounter, touchEndCounter;
    bool acceptTouchBegin, acceptTouchUpdate, acceptTouchEnd;
    bool deleteInTouchBegin, deleteInTouchUpdate, deleteInTouchEnd;
    tst_QTouchEventGraphicsItem **weakpointer;

    explicit tst_QTouchEventGraphicsItem(QGraphicsItem *parent = nullptr)
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

    QRectF boundingRect() const override { return QRectF(0, 0, 10, 10); }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->fillRect(QRectF(QPointF(0, 0), boundingRect().size()), Qt::yellow);
    }

    bool sceneEvent(QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::TouchBegin:
            qCDebug(lcTests) << this << event;
            if (seenTouchBegin) qWarning("TouchBegin: already seen a TouchBegin");
            if (seenTouchUpdate) qWarning("TouchBegin: TouchUpdate cannot happen before TouchBegin");
            if (seenTouchEnd) qWarning("TouchBegin: TouchEnd cannot happen before TouchBegin");
            seenTouchBegin = !seenTouchBegin && !seenTouchUpdate && !seenTouchEnd;
            ++touchBeginCounter;
            touchBeginPoints = static_cast<QTouchEvent *>(event)->points();
            event->setAccepted(acceptTouchBegin);
            if (deleteInTouchBegin)
                delete this;
            break;
        case QEvent::TouchUpdate:
            qCDebug(lcTests) << this << event;
            if (!seenTouchBegin) qWarning("TouchUpdate: have not seen TouchBegin");
            if (seenTouchEnd) qWarning("TouchUpdate: TouchEnd cannot happen before TouchUpdate");
            seenTouchUpdate = seenTouchBegin && !seenTouchEnd;
            ++touchUpdateCounter;
            touchUpdatePoints = static_cast<QTouchEvent *>(event)->points();
            event->setAccepted(acceptTouchUpdate);
            if (deleteInTouchUpdate)
                delete this;
            break;
        case QEvent::TouchEnd:
            qCDebug(lcTests) << this << event;
            if (!seenTouchBegin) qWarning("TouchEnd: have not seen TouchBegin");
            if (seenTouchEnd) qWarning("TouchEnd: already seen a TouchEnd");
            seenTouchEnd = seenTouchBegin && !seenTouchEnd;
            ++touchEndCounter;
            touchEndPoints = static_cast<QTouchEvent *>(event)->points();
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

struct GrabberWindow : public QWindow
{
    bool grabExclusive = false;
    bool grabPassive = false;
    QVector2D velocity;
    ulong timestamp;
    ulong lastTimestamp;

    void touchEvent(QTouchEvent *ev) override {
        qCDebug(lcTests) << ev;
        const auto &firstPoint = ev->point(0);
        velocity = firstPoint.velocity();
        timestamp = firstPoint.timestamp();
        lastTimestamp = firstPoint.lastTimestamp();
        switch (ev->type()) {
        case QEvent::TouchBegin: {
            QCOMPARE(ev->exclusiveGrabber(firstPoint), nullptr);
            QVERIFY(ev->passiveGrabbers(firstPoint).isEmpty());
            // It's not normal for QWindow to be the grabber, but that's easier to test
            // without needing to create child ojects.
            if (grabExclusive)
                ev->setExclusiveGrabber(firstPoint, this);
            if (grabPassive)
                ev->addPassiveGrabber(firstPoint, this);
            break;
        }
        default:
            break;
        }
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
    void state();
    void touchDisabledByDefault();
    void touchEventAcceptedByDefault();
    void touchBeginPropagatesWhenIgnored();
    void touchUpdateAndEndNeverPropagate();
    void basicRawEventTranslation();
    void basicRawEventTranslationOfIds();
    void multiPointRawEventTranslationOnTouchScreen();
    void multiPointRawEventTranslationOnTouchPad();
    void touchOnMultipleTouchscreens();
    void deleteInEventHandler();
    void deleteInRawEventTranslation();
    void crashInQGraphicsSceneAfterNotHandlingTouchBegin();
    void touchBeginWithGraphicsWidget();
    void testQGuiAppDelivery();
    void testMultiDevice();
    void grabbers_data();
    void grabbers();
    void velocity();

private:
    QPointingDevice *touchScreenDevice;
    QPointingDevice *secondaryTouchScreenDevice;
    QPointingDevice *touchPadDevice;
};

tst_QTouchEvent::tst_QTouchEvent()
  : touchScreenDevice(QTest::createTouchDevice())
  , secondaryTouchScreenDevice(QTest::createTouchDevice())
  , touchPadDevice(QTest::createTouchDevice(QInputDevice::DeviceType::TouchPad))
{
    QInputDevicePrivate::get(touchPadDevice)->setAvailableVirtualGeometry(QRect(50, 50, 500, 500));
    QInputDevicePrivate::get(secondaryTouchScreenDevice)->name = QLatin1String("secondary touchscreen");
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

void tst_QTouchEvent::state()
{
    QTouchEvent touchEvent(QEvent::TouchBegin, touchScreenDevice,
                           Qt::NoModifier, QList<QEventPoint>() <<
                           QEventPoint(0, QEventPoint::State::Stationary, {}, {}) <<
                           QEventPoint(1, QEventPoint::State::Pressed, {}, {}));
    QCOMPARE(touchEvent.touchPointStates(), QEventPoint::State::Stationary | QEventPoint::State::Pressed);
    QCOMPARE(touchEvent.pointCount(), 2);
    QVERIFY(touchEvent.isBeginEvent());
    QVERIFY(!touchEvent.isUpdateEvent());
    QVERIFY(!touchEvent.isEndEvent());
    QVERIFY(touchEvent.isInputEvent());
    QVERIFY(touchEvent.isPointerEvent());
    QVERIFY(!touchEvent.isSinglePointEvent());

    QTouchEvent touchEvent2 = QTouchEvent(QEvent::TouchBegin, touchScreenDevice,
                              Qt::NoModifier, QList<QEventPoint>() <<
                              QEventPoint(0, QEventPoint::State::Updated, {}, {}) <<
                              QEventPoint(1, QEventPoint::State::Pressed, {}, {}));
    QCOMPARE(touchEvent2.touchPointStates(), QEventPoint::State::Updated | QEventPoint::State::Pressed);
    QCOMPARE(touchEvent2.pointCount(), 2);
    QVERIFY(touchEvent2.isBeginEvent());
    QVERIFY(!touchEvent2.isUpdateEvent());
    QVERIFY(!touchEvent2.isEndEvent());

    QTouchEvent touchEvent3 = QTouchEvent(QEvent::TouchBegin, touchScreenDevice,
                              Qt::NoModifier, QList<QEventPoint>() <<
                              QEventPoint(0, QEventPoint::State::Updated, {}, {}) <<
                              QEventPoint(1, QEventPoint::State::Released, {}, {}));
    QCOMPARE(touchEvent3.touchPointStates(), QEventPoint::State::Updated | QEventPoint::State::Released);
    QCOMPARE(touchEvent3.pointCount(), 2);
    QVERIFY(!touchEvent3.isBeginEvent());
    QVERIFY(!touchEvent3.isUpdateEvent());
    QVERIFY(touchEvent3.isEndEvent());
#if QT_DEPRECATED_SINCE(6, 0)
    QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED // test Qt 5 compatibility wrappers
    QCOMPARE(touchEvent3.touchPoints(), touchEvent3.points());
    QT_WARNING_POP
#endif
}

void tst_QTouchEvent::touchDisabledByDefault()
{
    // QWidget
    {
        // the widget attribute is not enabled by default
        QWidget widget;
        QVERIFY(!widget.testAttribute(Qt::WA_AcceptTouchEvents));

        // events should not be accepted since they are not enabled
        QList<QEventPoint> touchPoints;
        touchPoints.append(QEventPoint(0));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
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
        QEventPoint touchPoint(0);
        QMutableEventPoint::setState(touchPoint, QEventPoint::State::Pressed);
        QMutableEventPoint::setPosition(touchPoint, view.mapFromScene(item.mapToScene(item.boundingRect().center())));
        QMutableEventPoint::setGlobalPosition(touchPoint, view.mapToGlobal(touchPoint.position().toPoint()));
        QMutableEventPoint::setScenePosition(touchPoint, view.mapToScene(touchPoint.position().toPoint()));

        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               (QList<QTouchEvent::TouchPoint>() << touchPoint));
        QVERIFY(!QApplication::sendEvent(view.viewport(), &touchEvent));
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
        QList<QEventPoint> touchPoints;
        touchPoints.append(QEventPoint(0));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               touchPoints);
        QVERIFY(QApplication::sendEvent(&widget, &touchEvent));
        QVERIFY(!touchEvent.isAccepted()); // Qt 5.X ignores touch events.

        // tst_QTouchEventWidget does handle, sending succeeds
        tst_QTouchEventWidget touchWidget;
        touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
        touchEvent.ignore();
        QVERIFY(QApplication::sendEvent(&touchWidget, &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(touchEvent.allPointsAccepted());
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
        QPointF pos = view.mapFromScene(item.mapToScene(item.boundingRect().center()));
        QEventPoint touchPoint(0, QEventPoint::State::Pressed,
                               view.mapToScene(pos.toPoint()),
                               view.mapToGlobal(pos.toPoint()));
        QMutableEventPoint::setPosition(touchPoint, pos);
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               (QList<QEventPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(touchEvent.allPointsAccepted());
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

        QList<QEventPoint> touchPoints;
        touchPoints.append(QEventPoint(0));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               touchPoints);
        QVERIFY(QApplication::sendEvent(&grandchild, &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(touchEvent.allPointsAccepted());
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
        QVERIFY(touchEvent.allPointsAccepted());
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
        QPointF pos = view.mapFromScene(grandchild.mapToScene(grandchild.boundingRect().center()));
        QEventPoint touchPoint(0, QEventPoint::State::Pressed,
                               view.mapToScene(pos.toPoint()),
                               view.mapToGlobal(pos.toPoint()));
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               (QList<QEventPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(touchEvent.allPointsAccepted());
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
        QPointF pos = view.mapFromScene(grandchild.mapToScene(grandchild.boundingRect().center()));
        QEventPoint touchPoint(0, QEventPoint::State::Pressed,
                               view.mapToScene(pos.toPoint()),
                               view.mapToGlobal(pos.toPoint()));
        QMutableEventPoint::setPosition(touchPoint, pos);
        QTouchEvent touchEvent(QEvent::TouchBegin,
                               touchScreenDevice,
                               Qt::NoModifier,
                               (QList<QEventPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEvent));
        QVERIFY(touchEvent.isAccepted());
        QVERIFY(touchEvent.allPointsAccepted());
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

        QList<QEventPoint> touchPoints;
        touchPoints.append(QEventPoint(0));
        QTouchEvent touchBeginEvent(QEvent::TouchBegin,
                                    touchScreenDevice,
                                    Qt::NoModifier,
                                    touchPoints);
        QVERIFY(QApplication::sendEvent(&child, &touchBeginEvent));
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(touchBeginEvent.allPointsAccepted());
        QVERIFY(child.seenTouchBegin);
        QVERIFY(!window.seenTouchBegin);

        // send the touch update to the child, but ignore it, it doesn't propagate
        QTouchEvent touchUpdateEvent(QEvent::TouchUpdate,
                                     touchScreenDevice,
                                     Qt::NoModifier,
                                     touchPoints);
        QVERIFY(QApplication::sendEvent(&child, &touchUpdateEvent));
        QVERIFY(!touchUpdateEvent.isAccepted());
        QVERIFY(!touchBeginEvent.allPointsAccepted());
        QVERIFY(child.seenTouchUpdate);
        QVERIFY(!window.seenTouchUpdate);

        // send the touch end, same thing should happen as with touch update
        QTouchEvent touchEndEvent(QEvent::TouchEnd,
                                  touchScreenDevice,
                                  Qt::NoModifier,
                                  touchPoints);
        QVERIFY(QApplication::sendEvent(&child, &touchEndEvent));
        QVERIFY(!touchEndEvent.isAccepted());
        QVERIFY(!touchBeginEvent.allPointsAccepted());
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
        QPointF pos = view.mapFromScene(grandchild.mapToScene(grandchild.boundingRect().center()));
        QEventPoint touchPoint(0, QEventPoint::State::Pressed,
                               view.mapToScene(pos.toPoint()),
                               view.mapToGlobal(pos.toPoint()));
        QMutableEventPoint::setPosition(touchPoint, pos);
        QTouchEvent touchBeginEvent(QEvent::TouchBegin,
                                    touchScreenDevice,
                                    Qt::NoModifier,
                                    (QList<QEventPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchBeginEvent));
        QVERIFY(touchBeginEvent.isAccepted());
        QVERIFY(touchBeginEvent.allPointsAccepted());
        QVERIFY(child.seenTouchBegin);
        QVERIFY(!root.seenTouchBegin);

        // send the touch update to the child, but ignore it, it doesn't propagate
        touchPoint = QEventPoint(0, QEventPoint::State::Updated,
                                 view.mapToScene(pos.toPoint()),
                                 view.mapToGlobal(pos.toPoint()));
        QMutableEventPoint::setPosition(touchPoint, pos);
        QTouchEvent touchUpdateEvent(QEvent::TouchUpdate,
                                     touchScreenDevice,
                                     Qt::NoModifier,
                                     (QList<QEventPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchUpdateEvent));
        // the scene accepts the event, since it found an item to send the event to
        QVERIFY(!touchUpdateEvent.isAccepted());
        QVERIFY(child.seenTouchUpdate);
        QVERIFY(!root.seenTouchUpdate);

        // send the touch end, same thing should happen as with touch update
        touchPoint = QEventPoint(0, QEventPoint::State::Released,
                                 view.mapToScene(pos.toPoint()),
                                 view.mapToGlobal(pos.toPoint()));
        QMutableEventPoint::setPosition(touchPoint, pos);
        QTouchEvent touchEndEvent(QEvent::TouchEnd,
                                  touchScreenDevice,
                                  Qt::NoModifier,
                                  (QList<QEventPoint>() << touchPoint));
        QVERIFY(QApplication::sendEvent(view.viewport(), &touchEndEvent));
        // the scene accepts the event, since it found an item to send the event to
        QVERIFY(!touchEndEvent.isAccepted());
        QVERIFY(child.seenTouchEnd);
        QVERIFY(!root.seenTouchEnd);
    }
}

void tst_QTouchEvent::basicRawEventTranslation()
{
    tst_QTouchEventWidget touchWidget;
    touchWidget.setWindowTitle(QTest::currentTestFunction());
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);
    touchWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&touchWidget));

    const QPointF pos = touchWidget.rect().center();
    const QPointF screenPos = touchWidget.mapToGlobal(pos.toPoint());
    const QPointF delta(10, 10);

    // this should be translated to a TouchBegin
    QEventPoint rawTouchPoint(0, QEventPoint::State::Pressed, QPointF(), screenPos);
    ulong timestamp = 1234;
    QWindow *window = touchWidget.windowHandle();
    QList<QWindowSystemInterface::TouchPoint> nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() << rawTouchPoint, window);
    QWindowSystemInterface::handleTouchEvent(window, timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchBeginPoints.size(), 1);
    QCOMPARE(touchWidget.timestamp, timestamp);
    QEventPoint touchBeginPoint = touchWidget.touchBeginPoints.first();
    QCOMPARE(touchBeginPoint.id(), 0);
    QCOMPARE(touchBeginPoint.state(), rawTouchPoint.state());
    QCOMPARE(touchBeginPoint.position(), pos);
    QCOMPARE(touchBeginPoint.pressPosition(), pos);
    QCOMPARE(touchBeginPoint.lastPosition(), pos);
    QCOMPARE(touchBeginPoint.scenePosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchBeginPoint.scenePressPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchBeginPoint.sceneLastPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchBeginPoint.globalPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchBeginPoint.globalPressPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchBeginPoint.globalLastPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchBeginPoint.position(), pos);
    QCOMPARE(touchBeginPoint.globalPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchBeginPoint.scenePosition(), touchBeginPoint.scenePosition());
    QCOMPARE(touchBeginPoint.ellipseDiameters(), QSizeF(0, 0));
    QCOMPARE(touchBeginPoint.pressure(), qreal(1.));
    QCOMPARE(touchBeginPoint.velocity(), QVector2D());

    // moving the point should translate to TouchUpdate
    rawTouchPoint = QEventPoint(0, QEventPoint::State::Updated, QPointF(), screenPos + delta);
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() << rawTouchPoint, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchUpdatePoints.size(), 1);
    QEventPoint touchUpdatePoint = touchWidget.touchUpdatePoints.first();
    QCOMPARE(touchUpdatePoint.id(), 0);
    QCOMPARE(touchUpdatePoint.state(), rawTouchPoint.state());
    QCOMPARE(touchUpdatePoint.position(), pos + delta);
    QCOMPARE(touchUpdatePoint.pressPosition(), pos);
    QCOMPARE(touchUpdatePoint.lastPosition(), pos);
    QCOMPARE(touchUpdatePoint.scenePosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchUpdatePoint.scenePressPosition(), screenPos);
    QCOMPARE(touchUpdatePoint.sceneLastPosition(), screenPos);
    QCOMPARE(touchUpdatePoint.globalPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchUpdatePoint.globalPressPosition(), screenPos);
    QCOMPARE(touchUpdatePoint.globalLastPosition(), screenPos);
    QCOMPARE(touchUpdatePoint.position(), pos + delta);
    QCOMPARE(touchUpdatePoint.globalPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchUpdatePoint.scenePosition(), touchUpdatePoint.scenePosition());
    QCOMPARE(touchUpdatePoint.ellipseDiameters(), QSizeF(0, 0));
    QCOMPARE(touchUpdatePoint.pressure(), qreal(1.));

    // releasing the point translates to TouchEnd
    rawTouchPoint = QEventPoint(0, QEventPoint::State::Released, QPointF(), screenPos + delta + delta);
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() << rawTouchPoint, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchEndPoints.size(), 1);
    QEventPoint touchEndPoint = touchWidget.touchEndPoints.first();
    QCOMPARE(touchEndPoint.id(), 0);
    QCOMPARE(touchEndPoint.state(), rawTouchPoint.state());
    QCOMPARE(touchEndPoint.position(), pos + delta + delta);
    QCOMPARE(touchEndPoint.pressPosition(), pos);
    QCOMPARE(touchEndPoint.lastPosition(), pos + delta);
    QCOMPARE(touchEndPoint.scenePosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchEndPoint.scenePressPosition(), screenPos);
    QCOMPARE(touchEndPoint.sceneLastPosition(), screenPos + delta);
    QCOMPARE(touchEndPoint.globalPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchEndPoint.globalPressPosition(), screenPos);
    QCOMPARE(touchEndPoint.globalLastPosition(), screenPos + delta);
    QCOMPARE(touchEndPoint.position(), pos + delta + delta);
    QCOMPARE(touchEndPoint.globalPosition(), rawTouchPoint.globalPosition());
    QCOMPARE(touchEndPoint.scenePosition(), touchEndPoint.scenePosition());
    QCOMPARE(touchEndPoint.ellipseDiameters(), QSizeF(0, 0));
    QCOMPARE(touchEndPoint.pressure(), qreal(0.));
}

void tst_QTouchEvent::multiPointRawEventTranslationOnTouchScreen()
{
    tst_QTouchEventWidget touchWidget;
    touchWidget.setObjectName("parent touch widget");
    touchWidget.setWindowTitle(QTest::currentTestFunction());
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    const QPoint topLeft = QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(100, 100);
    touchWidget.setGeometry({topLeft, QSize(400, 300)});

    tst_QTouchEventWidget leftWidget(&touchWidget);
    leftWidget.setObjectName("leftWidget");
    leftWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    leftWidget.setGeometry(0, 100, 100, 100);

    tst_QTouchEventWidget rightWidget(&touchWidget);
    rightWidget.setObjectName("rightWidget");
    rightWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    rightWidget.setGeometry(300, 100, 100, 100);

    touchWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&touchWidget));
    if (touchWidget.geometry().topLeft() != topLeft) {
        qCDebug(lcTests) << "tried to set position 100, 100 on screen; got geometry"
                         << touchWidget.geometry() << "frame" << touchWidget.frameGeometry();
        QSKIP("failed to position the widget window on this platform");
    }

    QPoint leftPos = leftWidget.rect().center();
    QPoint rightPos = rightWidget.rect().center();
    QPoint centerPos = touchWidget.rect().center();
    QPoint leftScenePos = leftWidget.mapToParent(leftPos);
    QPoint rightScenePos = rightWidget.mapToParent(rightPos);
    QPoint leftScreenPos = leftWidget.mapToGlobal(leftPos);
    QPoint rightScreenPos = rightWidget.mapToGlobal(rightPos);
    QPoint centerScreenPos = touchWidget.mapToGlobal(centerPos);

    // generate TouchBegins on both leftWidget and rightWidget
    auto touchSequence = QTest::touchEvent(touchWidget.windowHandle(), touchScreenDevice);
    touchSequence.press(0, leftScenePos).press(1, rightScenePos);
    QVERIFY(touchSequence.commit()); // verify acceptance
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QVERIFY(leftWidget.seenTouchBegin);
    QVERIFY(!leftWidget.seenTouchUpdate);
    QVERIFY(!leftWidget.seenTouchEnd);
    QVERIFY(rightWidget.seenTouchBegin);
    QVERIFY(!rightWidget.seenTouchUpdate);
    QVERIFY(!rightWidget.seenTouchEnd);
    QCOMPARE(leftWidget.touchBeginPoints.size(), 1);
    QCOMPARE(rightWidget.touchBeginPoints.size(), 1);
    const int touchPointId0 = 0;
    const int touchPointId1 = touchPointId0 + 1;
    {
        const QEventPoint &leftTouchPoint = leftWidget.touchBeginPoints.first();
        QCOMPARE(leftTouchPoint.id(), touchPointId0);
        QCOMPARE(leftTouchPoint.state(), QEventPoint::Pressed);
        QCOMPARE(leftTouchPoint.position(), leftPos);
        QCOMPARE(leftTouchPoint.pressPosition(), leftPos);
        QCOMPARE(leftTouchPoint.lastPosition(), leftPos);
        QCOMPARE(leftTouchPoint.scenePosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.scenePressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.sceneLastPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalPressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalLastPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(leftTouchPoint.pressure(), qreal(1.));

        const QEventPoint &rightTouchPoint = rightWidget.touchBeginPoints.first();
        QCOMPARE(rightTouchPoint.id(), touchPointId1);
        QCOMPARE(rightTouchPoint.state(), QEventPoint::Pressed);
        QCOMPARE(rightTouchPoint.position(), rightPos);
        QCOMPARE(rightTouchPoint.pressPosition(), rightPos);
        QCOMPARE(rightTouchPoint.lastPosition(), rightPos);
        QCOMPARE(rightTouchPoint.scenePosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.scenePressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.sceneLastPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalPressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalLastPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(rightTouchPoint.pressure(), qreal(1.));
    }

    // an unlikely event with the two touchpoints moving exactly on top of each other
    touchSequence.move(0, centerPos).move(1, centerPos);
    QVERIFY(touchSequence.commit()); // verify acceptance
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QVERIFY(leftWidget.seenTouchBegin);
    QVERIFY(leftWidget.seenTouchUpdate);
    QVERIFY(!leftWidget.seenTouchEnd);
    QVERIFY(rightWidget.seenTouchBegin);
    QVERIFY(rightWidget.seenTouchUpdate);
    QVERIFY(!rightWidget.seenTouchEnd);
    QCOMPARE(leftWidget.touchUpdatePoints.size(), 1);
    QCOMPARE(rightWidget.touchUpdatePoints.size(), 1);
    {
        const QEventPoint &leftTouchPoint = leftWidget.touchUpdatePoints.first();
        QCOMPARE(leftTouchPoint.id(), touchPointId0);
        QCOMPARE(leftTouchPoint.state(), QEventPoint::Updated);
        QCOMPARE(leftTouchPoint.position(), QPointF(leftWidget.mapFromParent(centerPos)));
        QCOMPARE(leftTouchPoint.pressPosition(), leftPos);
        QCOMPARE(leftTouchPoint.lastPosition(), leftPos);
        QCOMPARE(leftTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.scenePressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.sceneLastPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.globalPressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalLastPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(leftTouchPoint.pressure(), qreal(1.));

        const QEventPoint &rightTouchPoint = rightWidget.touchUpdatePoints.first();
        QCOMPARE(rightTouchPoint.id(), touchPointId1);
        QCOMPARE(rightTouchPoint.state(), QEventPoint::Updated);
        QCOMPARE(rightTouchPoint.position(), QPointF(rightWidget.mapFromParent(centerPos)));
        QCOMPARE(rightTouchPoint.pressPosition(), rightPos);
        QCOMPARE(rightTouchPoint.lastPosition(), rightPos);
        QCOMPARE(rightTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.scenePressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.sceneLastPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.globalPressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalLastPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(rightTouchPoint.pressure(), qreal(1.));
    }

    // generate TouchEnds on both leftWidget and rightWidget
    touchSequence.release(0, centerPos).release(1, centerPos);
    QVERIFY(touchSequence.commit()); // verify acceptance
    QVERIFY(!touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QVERIFY(leftWidget.seenTouchBegin);
    QVERIFY(leftWidget.seenTouchUpdate);
    QVERIFY(leftWidget.seenTouchEnd);
    QVERIFY(rightWidget.seenTouchBegin);
    QVERIFY(rightWidget.seenTouchUpdate);
    QVERIFY(rightWidget.seenTouchEnd);
    QCOMPARE(leftWidget.touchEndPoints.size(), 1);
    QCOMPARE(rightWidget.touchEndPoints.size(), 1);
    {
        const QEventPoint &leftTouchPoint = leftWidget.touchEndPoints.first();
        QCOMPARE(leftTouchPoint.id(), touchPointId0);
        QCOMPARE(leftTouchPoint.state(), QEventPoint::Released);
        QCOMPARE(leftTouchPoint.position(), QPointF(leftWidget.mapFromParent(centerPos)));
        QCOMPARE(leftTouchPoint.pressPosition(), leftPos);
        QCOMPARE(leftTouchPoint.lastPosition(), leftPos);
        QCOMPARE(leftTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.scenePressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.sceneLastPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.globalPressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalLastPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(leftTouchPoint.pressure(), qreal(0.));

        const QEventPoint &rightTouchPoint = rightWidget.touchEndPoints.first();
        QCOMPARE(rightTouchPoint.id(), touchPointId1);
        QCOMPARE(rightTouchPoint.state(), QEventPoint::Released);
        QCOMPARE(rightTouchPoint.position(), QPointF(rightWidget.mapFromParent(centerPos)));
        QCOMPARE(rightTouchPoint.pressPosition(), rightPos);
        QCOMPARE(rightTouchPoint.lastPosition(), rightPos);
        QCOMPARE(rightTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.scenePressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.sceneLastPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.globalPressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalLastPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(rightTouchPoint.pressure(), qreal(0.));
    }
}

void tst_QTouchEvent::touchOnMultipleTouchscreens()
{
    tst_QTouchEventWidget touchWidget;
    touchWidget.setWindowTitle(QTest::currentTestFunction());
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);
    touchWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&touchWidget));
    QWindow *window = touchWidget.windowHandle();

    QPointF pos = touchWidget.rect().center();
    QPointF screenPos = touchWidget.mapToGlobal(pos.toPoint());
    QPointF delta(10, 10);
    ulong timestamp = 1234;

    // this should be translated to a TouchBegin
    QList<QWindowSystemInterface::TouchPoint> nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() <<
            QMutableEventPoint::withTimeStamp(1234, 1, QEventPoint::State::Pressed, screenPos, screenPos, screenPos), window);
    QWindowSystemInterface::handleTouchEvent(window, timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchBeginPoints.size(), 1);
    QCOMPARE(touchWidget.timestamp, timestamp);
    QEventPoint touchBeginPoint = touchWidget.touchBeginPoints.first();
    QCOMPARE(touchBeginPoint.id(), 1);
    QCOMPARE(touchBeginPoint.state(), QEventPoint::State::Pressed);
    QCOMPARE(touchBeginPoint.position(), pos);

    // press a point on secondaryTouchScreenDevice
    touchWidget.seenTouchBegin = false;
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() <<
            QMutableEventPoint::withTimeStamp(1234, 10, QEventPoint::State::Pressed, screenPos, screenPos, screenPos), window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, secondaryTouchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchBeginPoints.size(), 1);
    QCOMPARE(touchWidget.timestamp, timestamp);
    touchBeginPoint = touchWidget.touchBeginPoints[0];
    QCOMPARE(touchBeginPoint.id(), 10);
    QCOMPARE(touchBeginPoint.state(), QEventPoint::State::Pressed);
    QCOMPARE(touchBeginPoint.position(), pos);

    // press another point on secondaryTouchScreenDevice
    touchWidget.seenTouchBegin = false;
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() <<
            QMutableEventPoint::withTimeStamp(1234, 11, QEventPoint::State::Pressed, screenPos, screenPos, screenPos), window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, secondaryTouchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchBeginPoints.size(), 1);
    QCOMPARE(touchWidget.timestamp, timestamp);
    touchBeginPoint = touchWidget.touchBeginPoints[0];
    QCOMPARE(touchBeginPoint.id(), 11);
    QCOMPARE(touchBeginPoint.state(), QEventPoint::State::Pressed);
    QCOMPARE(touchBeginPoint.position(), pos);

    // moving the first point should translate to TouchUpdate
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() <<
            QMutableEventPoint::withTimeStamp(1234, 1, QEventPoint::State::Updated, screenPos + delta, screenPos + delta, screenPos + delta), window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchUpdatePoints.size(), 1);
    QEventPoint touchUpdatePoint = touchWidget.touchUpdatePoints.first();
    QCOMPARE(touchUpdatePoint.id(), 1);
    QCOMPARE(touchUpdatePoint.state(), QEventPoint::State::Updated);
    QCOMPARE(touchUpdatePoint.position(), pos + delta);

    // releasing the first point translates to TouchEnd
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() <<
            QMutableEventPoint::withTimeStamp(1234, 1, QEventPoint::State::Released, screenPos + delta + delta, screenPos + delta + delta, screenPos + delta + delta), window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchEndPoints.size(), 1);
    QEventPoint touchEndPoint = touchWidget.touchEndPoints.first();
    QCOMPARE(touchEndPoint.id(), 1);
    QCOMPARE(touchEndPoint.state(), QEventPoint::State::Released);
    QCOMPARE(touchEndPoint.position(), pos + delta + delta);

    // Widgets don't normally handle this case: if a TouchEnd was seen before, then
    // WA_WState_AcceptedTouchBeginEvent will be false, and
    // QApplicationPrivate::translateRawTouchEvent will ignore touch events that aren't TouchBegin.
    // So we have to set it true.  It _did_ in fact accept the touch begin from the secondary device,
    // but it also got a TouchEnd from the primary device in the meantime.
    touchWidget.setAttribute(Qt::WA_WState_AcceptedTouchBeginEvent, true);

    // Releasing one point on the secondary touchscreen does not yet generate TouchEnd.
    touchWidget.seenTouchEnd = false;
    touchWidget.touchEndPoints.clear();
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() <<
            QMutableEventPoint::withTimeStamp(1234, 10, QEventPoint::State::Released, screenPos, screenPos, screenPos) <<
            QMutableEventPoint::withTimeStamp(1234, 11, QEventPoint::State::Stationary, screenPos, screenPos, screenPos), window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, secondaryTouchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchUpdatePoints.size(), 2);
    QCOMPARE(touchWidget.touchUpdatePoints[0].id(), 10);
    QCOMPARE(touchWidget.touchUpdatePoints[1].id(), 11);

    // releasing the last point on the secondary touchscreen translates to TouchEnd
    touchWidget.seenTouchEnd = false;
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(QList<QEventPoint>() <<
            QMutableEventPoint::withTimeStamp(1234, 11, QEventPoint::State::Released, screenPos + delta + delta,
                               screenPos + delta + delta, screenPos + delta + delta), window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, secondaryTouchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchEndPoints.size(), 1);
    touchEndPoint = touchWidget.touchEndPoints.first();
    QCOMPARE(touchEndPoint.id(), 11);
    QCOMPARE(touchEndPoint.state(), QEventPoint::State::Released);
}

void tst_QTouchEvent::multiPointRawEventTranslationOnTouchPad()
{
#ifdef Q_OS_MACOS
#if QT_CONFIG(cursor)
    QCursor::setPos(0, 0); // move mouse out of the way
    if (!QTest::qWaitFor([]{ return QCursor::pos() == QPoint(0, 0); }))
#endif
        QSKIP("The macOS mouse cursor interferes with this test can cannot be moved away");
#endif

    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    tst_QTouchEventWidget touchWidget;
    touchWidget.setObjectName("touchWidget");
    touchWidget.setWindowTitle(QTest::currentTestFunction());
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);

    tst_QTouchEventWidget leftWidget(&touchWidget);
    leftWidget.setObjectName("leftWidget");
    leftWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    leftWidget.setGeometry(0, 100, 100, 100);
    leftWidget.acceptTouchBegin =true;

    tst_QTouchEventWidget rightWidget(&touchWidget);
    rightWidget.setObjectName("rightWidget");
    rightWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    rightWidget.setGeometry(300, 100, 100, 100);

    touchWidget.show();
    QVERIFY(QTest::qWaitForWindowActive(&touchWidget));

    const QPointF leftPos = leftWidget.rect().center();
    const QPointF rightPos = rightWidget.rect().center();
    const QPointF centerPos = touchWidget.rect().center();
    const QPointF leftScreenPos = leftWidget.mapToGlobal(leftPos.toPoint());
    const QPointF rightScreenPos = rightWidget.mapToGlobal(rightPos.toPoint());
    const QPointF centerScreenPos = touchWidget.mapToGlobal(centerPos.toPoint());

    ulong timestamp = 0;
    QList<QEventPoint> rawTouchPoints = {
        QEventPoint(0),
        QEventPoint(1),
    };

    // generate TouchBegin on leftWidget only
    {
        QEventPoint &tp0 = rawTouchPoints[0];
        QMutableEventPoint::setState(tp0, QEventPoint::State::Pressed);
        QMutableEventPoint::setGlobalPosition(tp0, leftScreenPos);
        QEventPoint &tp1 = rawTouchPoints[1];
        QMutableEventPoint::setState(tp1, QEventPoint::State::Pressed);
        QMutableEventPoint::setGlobalPosition(tp1, rightScreenPos);
    }
    QWindow *window = touchWidget.windowHandle();
    QList<QWindowSystemInterface::TouchPoint> nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchPadDevice, nativeTouchPoints);
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
    QCOMPARE(leftWidget.touchBeginPoints.size(), 2);
    QCOMPARE(rightWidget.touchBeginPoints.size(), 0);
    QCOMPARE(leftWidget.lastNormalizedPositions.size(), 2);
    {
        QEventPoint leftTouchPoint = leftWidget.touchBeginPoints.at(0);
        qCDebug(lcTests) << "lastNormalizedPositions after press" << leftWidget.lastNormalizedPositions;
        qCDebug(lcTests) << "leftTouchPoint" << &leftTouchPoint;
        QCOMPARE(leftTouchPoint.id(), rawTouchPoints[0].id());
        QCOMPARE(leftTouchPoint.state(), rawTouchPoints[0].state());
        QCOMPARE(leftTouchPoint.position(), leftPos);
        QCOMPARE(leftTouchPoint.pressPosition(), leftPos);
        QCOMPARE(leftTouchPoint.lastPosition(), leftPos);
        QCOMPARE(leftTouchPoint.scenePosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.scenePressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.sceneLastPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalPressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalLastPosition(), leftScreenPos);
        QCOMPARE_LT(qAbs(leftWidget.lastNormalizedPositions.at(0).x() - 0.2), 0.05); // 0.198, might depend on window frame size
        QCOMPARE(leftTouchPoint.position(), leftPos);
        QCOMPARE(leftTouchPoint.scenePosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(leftTouchPoint.pressure(), qreal(1.));

        QEventPoint rightTouchPoint = leftWidget.touchBeginPoints.at(1);
        qCDebug(lcTests) << "rightTouchPoint" << &rightTouchPoint;
        QCOMPARE(rightTouchPoint.id(), rawTouchPoints[1].id());
        QCOMPARE(rightTouchPoint.state(), rawTouchPoints[1].state());
        QCOMPARE(rightTouchPoint.position(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.pressPosition(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.lastPosition(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.scenePosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.scenePressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.sceneLastPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalPressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalLastPosition(), rightScreenPos);
        QCOMPARE_LT(qAbs(leftWidget.lastNormalizedPositions.at(1).x() - 0.8), 0.05); // 0.798, might depend on window frame size
        QCOMPARE(rightTouchPoint.scenePosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(rightTouchPoint.pressure(), qreal(1.));
    }

    // generate TouchUpdate on leftWidget
    QMutableEventPoint::setState(rawTouchPoints[0], QEventPoint::State::Updated);
    QMutableEventPoint::setGlobalPosition(rawTouchPoints[0], centerScreenPos);
    QMutableEventPoint::setState(rawTouchPoints[1], QEventPoint::State::Updated);
    QMutableEventPoint::setGlobalPosition(rawTouchPoints[1], centerScreenPos);
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchPadDevice, nativeTouchPoints);
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
    QCOMPARE(leftWidget.touchUpdatePoints.size(), 2);
    QCOMPARE(rightWidget.touchUpdatePoints.size(), 0);
    QCOMPARE(leftWidget.lastNormalizedPositions.size(), 2);
    {
        QEventPoint leftTouchPoint = leftWidget.touchUpdatePoints.at(0);
        qCDebug(lcTests) << "lastNormalizedPositions after update" << leftWidget.lastNormalizedPositions;
        qCDebug(lcTests) << "leftTouchPoint" << leftTouchPoint;
        QCOMPARE(leftTouchPoint.id(), rawTouchPoints[0].id());
        QCOMPARE(leftTouchPoint.state(), rawTouchPoints[0].state());
        QCOMPARE(leftTouchPoint.position(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(leftTouchPoint.pressPosition(), leftPos);
        QCOMPARE(leftTouchPoint.lastPosition(), leftPos);
        QCOMPARE(leftTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.scenePressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.sceneLastPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.globalPressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalLastPosition(), leftScreenPos);
        QCOMPARE_LT(qAbs(leftWidget.lastNormalizedPositions.at(0).x() - 0.5), 0.05); // 0.498, might depend on window frame size
        QCOMPARE(leftTouchPoint.position(), leftWidget.mapFromParent(centerPos.toPoint()));
        QCOMPARE(leftTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(leftTouchPoint.pressure(), 1);

        QEventPoint rightTouchPoint = leftWidget.touchUpdatePoints.at(1);
        qCDebug(lcTests) << "rightTouchPoint" << rightTouchPoint;
        QCOMPARE(rightTouchPoint.id(), rawTouchPoints[1].id());
        QCOMPARE(rightTouchPoint.state(), rawTouchPoints[1].state());
        QCOMPARE(rightTouchPoint.position(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(rightTouchPoint.pressPosition(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.lastPosition(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.scenePressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.sceneLastPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.globalPressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalLastPosition(), rightScreenPos);
        QCOMPARE_LT(qAbs(leftWidget.lastNormalizedPositions.at(1).x() - 0.5), 0.05); // 0.498, might depend on window frame size
        QCOMPARE(rightTouchPoint.position(), leftWidget.mapFromParent(centerPos.toPoint()));
        QCOMPARE(rightTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(rightTouchPoint.pressure(), 1);
    }

    // generate TouchEnd on leftWidget
    // both touchpoints are still at centerScreenPos
    QMutableEventPoint::setState(rawTouchPoints[0], QEventPoint::State::Released);
    QMutableEventPoint::setState(rawTouchPoints[1], QEventPoint::State::Released);
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchPadDevice, nativeTouchPoints);
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
    QCOMPARE(leftWidget.touchEndPoints.size(), 2);
    QCOMPARE(rightWidget.touchEndPoints.size(), 0);
    QCOMPARE(leftWidget.lastNormalizedPositions.size(), 2);
    {
        QEventPoint leftTouchPoint = leftWidget.touchEndPoints.at(0);
        qCDebug(lcTests) << "lastNormalizedPositions after release" << leftWidget.lastNormalizedPositions;
        qCDebug(lcTests) << "leftTouchPoint" << leftTouchPoint;
        QCOMPARE(leftTouchPoint.id(), rawTouchPoints[0].id());
        QCOMPARE(leftTouchPoint.state(), rawTouchPoints[0].state());
        QCOMPARE(leftTouchPoint.position(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(leftTouchPoint.pressPosition(), leftPos);
        // it didn't move, so lastPosition is from the last time it moved
        QCOMPARE(leftTouchPoint.lastPosition(), leftPos);
        QCOMPARE(leftTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.scenePressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.sceneLastPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.globalPressPosition(), leftScreenPos);
        QCOMPARE(leftTouchPoint.globalLastPosition(), leftScreenPos);
        QCOMPARE_LT(qAbs(leftWidget.lastNormalizedPositions.at(0).x() - 0.5), 0.05); // 0.498, might depend on window frame size
        QCOMPARE(leftTouchPoint.position(), leftWidget.mapFromParent(centerPos.toPoint()));
        QCOMPARE(leftTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(leftTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(leftTouchPoint.pressure(), 0);

        QEventPoint rightTouchPoint = leftWidget.touchEndPoints.at(1);
        qCDebug(lcTests) << "rightTouchPoint" << rightTouchPoint;
        QCOMPARE(rightTouchPoint.id(), rawTouchPoints[1].id());
        QCOMPARE(rightTouchPoint.state(), rawTouchPoints[1].state());
        QCOMPARE(rightTouchPoint.position(), QPointF(leftWidget.mapFromParent(centerPos.toPoint())));
        QCOMPARE(rightTouchPoint.pressPosition(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.lastPosition(), QPointF(leftWidget.mapFromGlobal(rightScreenPos.toPoint())));
        QCOMPARE(rightTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.scenePressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.sceneLastPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.globalPressPosition(), rightScreenPos);
        QCOMPARE(rightTouchPoint.globalLastPosition(), rightScreenPos);
        QCOMPARE_LT(qAbs(leftWidget.lastNormalizedPositions.at(1).x() - 0.5), 0.05); // 0.498, might depend on window frame size
        QCOMPARE(rightTouchPoint.position(), leftWidget.mapFromParent(centerPos.toPoint()));
        QCOMPARE(rightTouchPoint.scenePosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.globalPosition(), centerScreenPos);
        QCOMPARE(rightTouchPoint.ellipseDiameters(), QSizeF(0, 0));
        QCOMPARE(rightTouchPoint.pressure(), 0);
    }
}

void tst_QTouchEvent::basicRawEventTranslationOfIds()
{
    tst_QTouchEventWidget touchWidget;
    touchWidget.setWindowTitle(QTest::currentTestFunction());
    touchWidget.setAttribute(Qt::WA_AcceptTouchEvents);
    touchWidget.setGeometry(100, 100, 400, 300);
    touchWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&touchWidget));

    QVarLengthArray<QPointF, 2> pos;
    QVarLengthArray<QPointF, 2> screenPos;
    for (int i = 0; i < 2; ++i) {
        pos << touchWidget.rect().center() + QPointF(20*i, 20*i);
        screenPos << touchWidget.mapToGlobal(pos[i].toPoint());
    }
    QPointF delta(10, 10);
    QList<QEventPoint> rawTouchPoints;

    // Press both points, this should be translated to a TouchBegin
    for (int i = 0; i < 2; ++i) {
        QEventPoint rawTouchPoint(i);
        QMutableEventPoint::setState(rawTouchPoint, QEventPoint::State::Pressed);
        QMutableEventPoint::setGlobalPosition(rawTouchPoint, screenPos[i]);
        rawTouchPoints << rawTouchPoint;
    }
    QEventPoint &p0 = rawTouchPoints[0];
    QEventPoint &p1 = rawTouchPoints[1];

    ulong timestamp = 1234;
    QWindow *window = touchWidget.windowHandle();
    QList<QWindowSystemInterface::TouchPoint> nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(!touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchBeginPoints.size(), 2);

    for (int i = 0; i < touchWidget.touchBeginPoints.size(); ++i) {
        QEventPoint touchBeginPoint = touchWidget.touchBeginPoints.at(i);
        QCOMPARE(touchBeginPoint.id(), i);
        QCOMPARE(touchBeginPoint.state(), rawTouchPoints[i].state());
    }

    // moving the point should translate to TouchUpdate
    for (int i = 0; i < rawTouchPoints.size(); ++i) {
        auto &p = rawTouchPoints[i];
        QMutableEventPoint::setState(p, QEventPoint::State::Updated);
        QMutableEventPoint::setGlobalPosition(p, p.globalPosition() + delta);
    }
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchUpdatePoints.size(), 2);
    QCOMPARE(touchWidget.touchUpdatePoints.at(0).id(), 0);
    QCOMPARE(touchWidget.touchUpdatePoints.at(1).id(), 1);

    // release last point
    QMutableEventPoint::setState(p0, QEventPoint::State::Stationary);
    QMutableEventPoint::setState(p1, QEventPoint::State::Released);

    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QCOMPARE(touchWidget.seenTouchEnd, false);
    QCOMPARE(touchWidget.touchUpdatePoints.size(), 2);
    QCOMPARE(touchWidget.touchUpdatePoints[0].id(), 0);
    QCOMPARE(touchWidget.touchUpdatePoints[1].id(), 1);

    // Press last point again, id should increase
    QMutableEventPoint::setState(p1, QEventPoint::State::Pressed);
    QMutableEventPoint::setId(p1, 42);   // new id
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(!touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchUpdatePoints.size(), 2);
    QCOMPARE(touchWidget.touchUpdatePoints[0].id(), 0);
    QCOMPARE(touchWidget.touchUpdatePoints[1].id(), 42);

    // release everything
    QMutableEventPoint::setState(p0, QEventPoint::State::Released);
    QMutableEventPoint::setState(p1, QEventPoint::State::Released);
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(touchWidget.seenTouchBegin);
    QVERIFY(touchWidget.seenTouchUpdate);
    QVERIFY(touchWidget.seenTouchEnd);
    QCOMPARE(touchWidget.touchUpdatePoints.size(), 2);
    QCOMPARE(touchWidget.touchUpdatePoints[0].id(), 0);
    QCOMPARE(touchWidget.touchUpdatePoints[1].id(), 42);
}

void tst_QTouchEvent::deleteInEventHandler()
{
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

        QList<QEventPoint> touchPoints;
        touchPoints.append(QEventPoint(0));
        QTouchEvent touchBeginEvent(QEvent::TouchBegin,
                                    touchScreenDevice,
                                    Qt::NoModifier,
                                    touchPoints);
        QTouchEvent touchUpdateEvent(QEvent::TouchUpdate,
                               touchScreenDevice,
                               Qt::NoModifier,
                               touchPoints);
        QTouchEvent touchEndEvent(QEvent::TouchEnd,
                               touchScreenDevice,
                               Qt::NoModifier,
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

        QEventPoint touchPoint(0);
        QMutableEventPoint::setState(touchPoint, QEventPoint::State::Pressed);
        QMutableEventPoint::setPosition(touchPoint, view.mapFromScene(child1->mapToScene(child1->boundingRect().center())));
        QMutableEventPoint::setGlobalPosition(touchPoint, view.mapToGlobal(touchPoint.position().toPoint()));
        QMutableEventPoint::setScenePosition(touchPoint, view.mapToScene(touchPoint.position().toPoint()));
        QTouchEvent touchBeginEvent(QEvent::TouchBegin,
                                    touchScreenDevice,
                                    Qt::NoModifier,
                                    {touchPoint});
        QMutableEventPoint::detach(touchPoint);
        QMutableEventPoint::setState(touchPoint, QEventPoint::State::Updated);
        QTouchEvent touchUpdateEvent(QEvent::TouchUpdate,
                               touchScreenDevice,
                               Qt::NoModifier,
                                {touchPoint});
        QMutableEventPoint::detach(touchPoint);
        QMutableEventPoint::setState(touchPoint, QEventPoint::State::Released);
        QTouchEvent touchEndEvent(QEvent::TouchEnd,
                               touchScreenDevice,
                               Qt::NoModifier,
                               {touchPoint});

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

    ulong timestamp = 0;
    QList<QEventPoint> rawTouchPoints = {
        QEventPoint(0),
        QEventPoint(1),
        QEventPoint(2),
    };
    QMutableEventPoint::setState(rawTouchPoints[0], QEventPoint::State::Pressed);
    QMutableEventPoint::setGlobalPosition(rawTouchPoints[0], leftScreenPos);
    QMutableEventPoint::setState(rawTouchPoints[1], QEventPoint::State::Pressed);
    QMutableEventPoint::setGlobalPosition(rawTouchPoints[1], centerScreenPos);
    QMutableEventPoint::setState(rawTouchPoints[2], QEventPoint::State::Pressed);
    QMutableEventPoint::setGlobalPosition(rawTouchPoints[2], rightScreenPos);

    // generate begin events on all widgets, the left widget should die
    QWindow *window = touchWidget.windowHandle();
    QList<QWindowSystemInterface::TouchPoint> nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();
    QVERIFY(leftWidget.isNull());
    QVERIFY(!centerWidget.isNull());
    QVERIFY(!rightWidget.isNull());

    // generate update events on all widget, the center widget should die
    QMutableEventPoint::setState(rawTouchPoints[0], QEventPoint::State::Updated);
    QMutableEventPoint::setState(rawTouchPoints[1], QEventPoint::State::Updated);
    QMutableEventPoint::setState(rawTouchPoints[2], QEventPoint::State::Updated);
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
    QCoreApplication::processEvents();

    // generate end events on all widget, the right widget should die
    QMutableEventPoint::setState(rawTouchPoints[0], QEventPoint::State::Released);
    QMutableEventPoint::setState(rawTouchPoints[1], QEventPoint::State::Released);
    QMutableEventPoint::setState(rawTouchPoints[2], QEventPoint::State::Released);
    nativeTouchPoints =
        QWindowSystemInterfacePrivate::toNativeTouchPoints(rawTouchPoints, window);
    QWindowSystemInterface::handleTouchEvent(window, ++timestamp, touchScreenDevice, nativeTouchPoints);
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
    QTest::touchEvent(view.viewport(), touchScreenDevice).press(0, centerPos, nullptr);
    QTest::touchEvent(view.viewport(), touchScreenDevice).release(0, centerPos, nullptr);
    // Touch outside of the button
    QTest::touchEvent(view.viewport(), touchScreenDevice).press(0, view.mapFromScene(QPoint(10, 10)), nullptr);
    QTest::touchEvent(view.viewport(), touchScreenDevice).release(0, view.mapFromScene(QPoint(10, 10)), nullptr);
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
    bool eventFilter(QObject *obj, QEvent *event) override;
    struct TouchInfo {
        QList<QEventPoint> points;
        QEvent::Type lastSeenType;
    };
    QMap<const QPointingDevice *, TouchInfo> d;
};

bool WindowTouchEventFilter::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::TouchBegin
            || event->type() == QEvent::TouchUpdate
            || event->type() == QEvent::TouchEnd) {
        QTouchEvent *te = static_cast<QTouchEvent *>(event);
        TouchInfo &td = d[te->pointingDevice()];
        if (event->type() == QEvent::TouchBegin)
            td.points.clear();
        td.points.append(te->points());
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
    QWindowSystemInterface::handleTouchEvent(&w, nullptr, points);
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.isEmpty(), true);

    QWindowSystemInterface::TouchPoint tp;
    tp.id = 0;
    tp.state = QEventPoint::State::Pressed;
    tp.area = QRectF(120, 120, 20, 20);
    points.append(tp);

    // Null device: should be ignored.
    QWindowSystemInterface::handleTouchEvent(&w, nullptr, points);
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.isEmpty(), true);

    // Now the real thing.
    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, points); // TouchBegin
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.size(), 1);
    QCOMPARE(filter.d.contains(touchScreenDevice), true);
    QCOMPARE(filter.d.value(touchScreenDevice).points.size(), 1);
    QCOMPARE(filter.d.value(touchScreenDevice).lastSeenType, QEvent::TouchBegin);

    points[0].state = QEventPoint::State::Updated;
    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, points); // TouchUpdate
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.size(), 1);
    QCOMPARE(filter.d.contains(touchScreenDevice), true);
    QCOMPARE(filter.d.value(touchScreenDevice).points.size(), 2);
    QCOMPARE(filter.d.value(touchScreenDevice).lastSeenType, QEvent::TouchUpdate);

    points[0].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, points); // TouchEnd
    QCoreApplication::processEvents();
    QCOMPARE(filter.d.size(), 1);
    QCOMPARE(filter.d.contains(touchScreenDevice), true);
    QCOMPARE(filter.d.value(touchScreenDevice).points.size(), 3);
    QCOMPARE(filter.d.value(touchScreenDevice).lastSeenType, QEvent::TouchEnd);
}

void tst_QTouchEvent::testMultiDevice()
{
    QPointingDevice *deviceTwo = QTest::createTouchDevice();

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
    tp.state = QEventPoint::State::Pressed;
    const QPoint screenOrigin = w.screen()->geometry().topLeft();
    const QRectF area0(120, 120, 20, 20);
    tp.area = QHighDpi::toNative(area0, QHighDpiScaling::factor(&w), screenOrigin);
    pointsOne.append(tp);

    pointsTwo.append(tp);
    tp.id = 1;
    const QRectF area1(140, 140, 20, 20);
    tp.area = QHighDpi::toNative(area1, QHighDpiScaling::factor(&w), screenOrigin);
    pointsTwo.append(tp);

    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, pointsOne);
    QWindowSystemInterface::handleTouchEvent(&w, deviceTwo, pointsTwo);
    QCoreApplication::processEvents();

    QCOMPARE(filter.d.contains(touchScreenDevice), true);
    QCOMPARE(filter.d.contains(deviceTwo), true);

    QCOMPARE(filter.d.value(touchScreenDevice).lastSeenType, QEvent::TouchBegin);
    QCOMPARE(filter.d.value(deviceTwo).lastSeenType, QEvent::TouchBegin);
    QCOMPARE(filter.d.value(touchScreenDevice).points.size(), 1);
    QCOMPARE(filter.d.value(deviceTwo).points.size(), 2);

    QCOMPARE(filter.d.value(touchScreenDevice).points.at(0).globalPosition(), area0.center());
    // This fails because QGuiApplicationPrivate::processTouchEvent() sends synth-mouse events
    // as QPA events, but with the device being the touchscreen; then processMouseEvent eventually
    // updates the touchscreen's QEventPoint in activePoints from an eventpoint that
    // came from a QPA mouse event, which of course does not have ellipse diameters.
    // Perhaps we should send the synth-mouse events more directly, bypassing QPA?
//    QCOMPARE(filter.d.value(touchScreenDevice).points.at(0).ellipseDiameters(), area0.size());
    QCOMPARE(filter.d.value(touchScreenDevice).points.at(0).state(), pointsOne[0].state);

    QCOMPARE(filter.d.value(deviceTwo).points.at(0).globalPosition(), area0.center());
//    QCOMPARE(filter.d.value(deviceTwo).points.at(0).ellipseDiameters(), area0.size());
    QCOMPARE(filter.d.value(deviceTwo).points.at(0).state(), pointsTwo[0].state);
    QCOMPARE(filter.d.value(deviceTwo).points.at(1).globalPosition(), area1.center());
    QCOMPARE(filter.d.value(deviceTwo).points.at(1).state(), pointsTwo[1].state);
}

void tst_QTouchEvent::grabbers_data()
{
    QTest::addColumn<bool>("grabExclusive");
    QTest::addColumn<bool>("grabPassive");

    QTest::newRow("no grab") << false << false;
    QTest::newRow("exclusive") << true << false;
    QTest::newRow("passive") << false << true;
}

void tst_QTouchEvent::grabbers()
{
    QFETCH(bool, grabExclusive);
    QFETCH(bool, grabPassive);

    GrabberWindow w;
    w.grabExclusive = grabExclusive;
    w.grabPassive = grabPassive;
    w.setGeometry(100, 100, 100, 100);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    auto devPriv = QPointingDevicePrivate::get(touchScreenDevice);
    devPriv->activePoints.clear(); // in case other tests left dangling state
    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp;
    tp.id = 0;
    tp.state = QEventPoint::State::Pressed;
    tp.area = QRectF(120, 120, 20, 20);
    points.append(tp);

    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, points); // TouchBegin
    QCoreApplication::processEvents();
    QCOMPARE(devPriv->activePoints.count(), 1);

    // Ensure that grabbers are persistent between events, within the stored touchpoints
    QCOMPARE(devPriv->pointById(0)->exclusiveGrabber, grabExclusive ? &w : nullptr);
    QCOMPARE(devPriv->pointById(0)->passiveGrabbers.size(), grabPassive ? 1 : 0);
    if (grabPassive)
        QCOMPARE(devPriv->pointById(0)->passiveGrabbers.first(), &w);

    // Ensure that eventpoints are forgotten after release delivery
    points.first().state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&w, touchScreenDevice, points); // TouchEnd
    QCoreApplication::processEvents();
    QTRY_COMPARE(devPriv->activePoints.count(), 0);
}

void tst_QTouchEvent::velocity()
{
    GrabberWindow w;
    w.grabExclusive = true;
    w.setGeometry(100, 100, 100, 100);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    auto devPriv = QPointingDevicePrivate::get(touchScreenDevice);
    devPriv->activePoints.clear();
    QPoint pos(10, 10);
    QTest::touchEvent(&w, touchScreenDevice).press(0, pos, &w);
    QCOMPARE(devPriv->activePoints.count(), 1);
    const auto &firstPoint = devPriv->pointById(0)->eventPoint;
    qCDebug(lcTests) << "persistent active point after press" << firstPoint;
    QCOMPARE(firstPoint.velocity(), QVector2D());

    QCOMPARE(firstPoint.pressTimestamp(), firstPoint.timestamp());
    QVERIFY(firstPoint.timestamp() > 0);
    QCOMPARE(firstPoint.state(), QEventPoint::State::Pressed);

    ulong timestamp = firstPoint.timestamp();
    for (int i = 1; i < 4; ++i) {
        qCDebug(lcTests) << "sending touch move event" << i;
        pos += {10, 10};
        QTest::touchEvent(&w, touchScreenDevice).move(0, pos, &w);
        qCDebug(lcTests) << firstPoint;
        QVERIFY(firstPoint.timestamp() > timestamp);
        QVERIFY(w.timestamp > w.lastTimestamp);
        QCOMPARE(w.timestamp, firstPoint.timestamp());
        timestamp = firstPoint.timestamp();
        QVERIFY(w.velocity.x() > 0);
        QVERIFY(w.velocity.y() > 0);
    }
    QTest::touchEvent(&w, touchScreenDevice).release(0, pos, &w);
    QVERIFY(w.velocity.x() > 0);
    QVERIFY(w.velocity.y() > 0);
}

QTEST_MAIN(tst_QTouchEvent)

#include "tst_qtouchevent.moc"
