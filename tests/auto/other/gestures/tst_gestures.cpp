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
#include <QtTest/qtesttouch.h>

#include <qevent.h>
#include <qtouchdevice.h>
#include <qwidget.h>
#include <qlayout.h>
#include <qgesture.h>
#include <qgesturerecognizer.h>
#include <qgraphicsitem.h>
#include <qgraphicswidget.h>
#include <qgraphicsview.h>
#include <qmainwindow.h>

#include <qdebug.h>

static QPointF mapToGlobal(const QPointF &pt, QGraphicsItem *item, QGraphicsView *view)
{
    return view->viewport()->mapToGlobal(view->mapFromScene(item->mapToScene(pt)));
}

class CustomGesture : public QGesture
{
    Q_OBJECT
public:
    static Qt::GestureType GestureType;

    CustomGesture(QObject *parent = 0)
        : QGesture(parent), serial(0)
    {
    }

    int serial;

    static const int SerialMaybeThreshold;
    static const int SerialStartedThreshold;
    static const int SerialFinishedThreshold;
};
Qt::GestureType CustomGesture::GestureType = Qt::CustomGesture;
const int CustomGesture::SerialMaybeThreshold = 1;
const int CustomGesture::SerialStartedThreshold = 3;
const int CustomGesture::SerialFinishedThreshold = 6;

class CustomEvent : public QEvent
{
public:
    static int EventType;

    explicit CustomEvent(int serial_ = 0)
        : QEvent(QEvent::Type(CustomEvent::EventType)),
          serial(serial_), hasHotSpot(false)
    {
    }

    int serial;
    QPointF hotSpot;
    bool hasHotSpot;
};
int CustomEvent::EventType = 0;

class CustomGestureRecognizer : public QGestureRecognizer
{
public:
    static bool ConsumeEvents;

    CustomGestureRecognizer()
    {
        if (!CustomEvent::EventType)
            CustomEvent::EventType = QEvent::registerEventType();
    }

    QGesture* create(QObject *)
    {
        return new CustomGesture;
    }

    QGestureRecognizer::Result recognize(QGesture *state, QObject*, QEvent *event)
    {
        if (event->type() == CustomEvent::EventType) {
            QGestureRecognizer::Result result;
            if (CustomGestureRecognizer::ConsumeEvents)
                result |= QGestureRecognizer::ConsumeEventHint;
            CustomGesture *g = static_cast<CustomGesture*>(state);
            CustomEvent *e = static_cast<CustomEvent*>(event);
            g->serial = e->serial;
            if (e->hasHotSpot)
                g->setHotSpot(e->hotSpot);
            if (g->serial >= CustomGesture::SerialFinishedThreshold)
                result |= QGestureRecognizer::FinishGesture;
            else if (g->serial >= CustomGesture::SerialStartedThreshold)
                result |= QGestureRecognizer::TriggerGesture;
            else if (g->serial >= CustomGesture::SerialMaybeThreshold)
                result |= QGestureRecognizer::MayBeGesture;
            else
                result = QGestureRecognizer::CancelGesture;
            return result;
        }
        return QGestureRecognizer::Ignore;
    }

    void reset(QGesture *state)
    {
        CustomGesture *g = static_cast<CustomGesture *>(state);
        g->serial = 0;
        QGestureRecognizer::reset(state);
    }
};
bool CustomGestureRecognizer::ConsumeEvents = false;

// same as CustomGestureRecognizer but triggers early without the maybe state
class CustomContinuousGestureRecognizer : public QGestureRecognizer
{
public:
    CustomContinuousGestureRecognizer()
    {
        if (!CustomEvent::EventType)
            CustomEvent::EventType = QEvent::registerEventType();
    }

    QGesture* create(QObject *)
    {
        return new CustomGesture;
    }

    QGestureRecognizer::Result recognize(QGesture *state, QObject*, QEvent *event)
    {
        if (event->type() == CustomEvent::EventType) {
            QGestureRecognizer::Result result = QGestureRecognizer::ConsumeEventHint;
            CustomGesture *g = static_cast<CustomGesture *>(state);
            CustomEvent *e = static_cast<CustomEvent *>(event);
            g->serial = e->serial;
            if (e->hasHotSpot)
                g->setHotSpot(e->hotSpot);
            if (g->serial >= CustomGesture::SerialFinishedThreshold)
                result |= QGestureRecognizer::FinishGesture;
            else if (g->serial >= CustomGesture::SerialMaybeThreshold)
                result |= QGestureRecognizer::TriggerGesture;
            else
                result = QGestureRecognizer::CancelGesture;
            return result;
        }
        return QGestureRecognizer::Ignore;
    }

    void reset(QGesture *state)
    {
        CustomGesture *g = static_cast<CustomGesture *>(state);
        g->serial = 0;
        QGestureRecognizer::reset(state);
    }
};

class GestureWidget : public QWidget
{
    Q_OBJECT
public:
    GestureWidget(const char *name = 0, QWidget *parent = 0)
        : QWidget(parent)
    {
        if (name)
            setObjectName(QLatin1String(name));
        reset();
        acceptGestureOverride = false;
    }
    void reset()
    {
        customEventsReceived = 0;
        gestureEventsReceived = 0;
        gestureOverrideEventsReceived = 0;
        events.clear();
        overrideEvents.clear();
        ignoredGestures.clear();
    }

    int customEventsReceived;
    int gestureEventsReceived;
    int gestureOverrideEventsReceived;
    struct Events
    {
        QList<Qt::GestureType> all;
        QList<Qt::GestureType> started;
        QList<Qt::GestureType> updated;
        QList<Qt::GestureType> finished;
        QList<Qt::GestureType> canceled;

        void clear()
        {
            all.clear();
            started.clear();
            updated.clear();
            finished.clear();
            canceled.clear();
        }
    } events, overrideEvents;

    bool acceptGestureOverride;
    QSet<Qt::GestureType> ignoredGestures;

protected:
    bool event(QEvent *event)
    {
        Events *eventsPtr = 0;
        if (event->type() == QEvent::Gesture) {
            QGestureEvent *e = static_cast<QGestureEvent*>(event);
            ++gestureEventsReceived;
            eventsPtr = &events;
            foreach(Qt::GestureType type, ignoredGestures)
                e->ignore(e->gesture(type));
        } else if (event->type() == QEvent::GestureOverride) {
            ++gestureOverrideEventsReceived;
            eventsPtr = &overrideEvents;
            if (acceptGestureOverride)
                event->accept();
        }
        if (eventsPtr) {
            QGestureEvent *e = static_cast<QGestureEvent*>(event);
            QList<QGesture*> gestures = e->gestures();
            foreach(QGesture *g, gestures) {
                eventsPtr->all << g->gestureType();
                switch(g->state()) {
                case Qt::GestureStarted:
                    emit gestureStarted(e->type(), g);
                    eventsPtr->started << g->gestureType();
                    break;
                case Qt::GestureUpdated:
                    emit gestureUpdated(e->type(), g);
                    eventsPtr->updated << g->gestureType();
                    break;
                case Qt::GestureFinished:
                    emit gestureFinished(e->type(), g);
                    eventsPtr->finished << g->gestureType();
                    break;
                case Qt::GestureCanceled:
                    emit gestureCanceled(e->type(), g);
                    eventsPtr->canceled << g->gestureType();
                    break;
                default:
                    qWarning() << "Unknown GestureState enum value:" << static_cast<int>(g->state());
                }
            }
        } else if (event->type() == CustomEvent::EventType) {
            ++customEventsReceived;
        } else {
            return QWidget::event(event);
        }
        return true;
    }

Q_SIGNALS:
    void gestureStarted(QEvent::Type, QGesture *);
    void gestureUpdated(QEvent::Type, QGesture *);
    void gestureFinished(QEvent::Type, QGesture *);
    void gestureCanceled(QEvent::Type, QGesture *);

public Q_SLOTS:
    void deleteThis() { delete this; }
};

// TODO rename to sendGestureSequence
static void sendCustomGesture(CustomEvent *event, QObject *object, QGraphicsScene *scene = 0)
{
    QPointer<QObject> receiver(object);
    for (int i = CustomGesture::SerialMaybeThreshold;
         i <= CustomGesture::SerialFinishedThreshold && receiver; ++i) {
        event->serial = i;
        if (scene)
            scene->sendEvent(qobject_cast<QGraphicsObject *>(object), event);
        else
            QApplication::sendEvent(object, event);
    }
}

class tst_Gestures : public QObject
{
Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void customGesture();
    void autoCancelingGestures();
    void gestureOverChild();
    void multipleWidgetOnlyGestureInTree();
    void conflictingGestures();
    void conflictingGesturesInGraphicsView();
    void finishedWithoutStarted();
    void unknownGesture();
    void graphicsItemGesture();
    void graphicsView();
    void graphicsItemTreeGesture();
    void explicitGraphicsObjectTarget();
    void gestureOverChildGraphicsItem();
    void twoGesturesOnDifferentLevel();
    void multipleGesturesInTree();
    void multipleGesturesInComplexTree();
    void testMapToScene();
    void ungrabGesture();
    void consumeEventHint();
    void unregisterRecognizer();
    void autoCancelGestures();
    void autoCancelGestures2();
    void graphicsViewParentPropagation();
    void panelPropagation();
    void panelStacksBehindParent();
#ifdef Q_OS_MACOS
    void deleteMacPanGestureRecognizerTargetWidget();
#endif
    void deleteGestureTargetWidget();
    void deleteGestureTargetItem_data();
    void deleteGestureTargetItem();
    void viewportCoordinates();
    void partialGesturePropagation();
    void testQGestureRecognizerCleanup();
    void testReuseCanceledGestures();
    void bug_13501_gesture_not_accepted();
};

void tst_Gestures::initTestCase()
{
    CustomGesture::GestureType = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);
    QVERIFY(CustomGesture::GestureType != Qt::GestureType(0));
    QVERIFY(CustomGesture::GestureType != Qt::CustomGesture);
}

void tst_Gestures::cleanupTestCase()
{
    QGestureRecognizer::unregisterRecognizer(CustomGesture::GestureType);
}

void tst_Gestures::customGesture()
{
    GestureWidget widget;
    widget.grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    CustomEvent event;
    event.hotSpot = widget.mapToGlobal(QPoint(5,5));
    event.hasHotSpot = true;
    sendCustomGesture(&event, &widget);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;
    static const int TotalCustomEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialMaybeThreshold + 1;
    QCOMPARE(widget.customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(widget.gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(widget.gestureOverrideEventsReceived, 0);
    QCOMPARE(widget.events.all.size(), TotalGestureEventsCount);
    for(int i = 0; i < widget.events.all.size(); ++i)
        QCOMPARE(widget.events.all.at(i), CustomGesture::GestureType);
    QCOMPARE(widget.events.started.size(), 1);
    QCOMPARE(widget.events.updated.size(), TotalGestureEventsCount - 2);
    QCOMPARE(widget.events.finished.size(), 1);
    QCOMPARE(widget.events.canceled.size(), 0);
}

void tst_Gestures::consumeEventHint()
{
    GestureWidget widget;
    widget.grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);

    CustomGestureRecognizer::ConsumeEvents = true;
    CustomEvent event;
    sendCustomGesture(&event, &widget);
    CustomGestureRecognizer::ConsumeEvents = false;

    QCOMPARE(widget.customEventsReceived, 0);
}

void tst_Gestures::autoCancelingGestures()
{
    GestureWidget widget;
    widget.grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);
    // send partial gesture. The gesture will be in the "maybe" state, but will
    // never get enough events to fire, so Qt will have to kill it.
    CustomEvent ev;
    for (int i = CustomGesture::SerialMaybeThreshold;
         i < CustomGesture::SerialStartedThreshold; ++i) {
        ev.serial = i;
        QApplication::sendEvent(&widget, &ev);
    }
    // wait long enough so the gesture manager will cancel the gesture
    QTest::qWait(5000);
    QCOMPARE(widget.customEventsReceived, CustomGesture::SerialStartedThreshold - CustomGesture::SerialMaybeThreshold);
    QCOMPARE(widget.gestureEventsReceived, 0);
    QCOMPARE(widget.gestureOverrideEventsReceived, 0);
    QCOMPARE(widget.events.all.size(), 0);
}

void tst_Gestures::gestureOverChild()
{
    GestureWidget widget("widget");
    QVBoxLayout *l = new QVBoxLayout(&widget);
    GestureWidget *child = new GestureWidget("child");
    l->addWidget(child);

    widget.grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);

    CustomEvent event;
    sendCustomGesture(&event, child);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;
    static const int TotalCustomEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialMaybeThreshold + 1;

    QCOMPARE(child->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(widget.customEventsReceived, 0);
    QCOMPARE(child->gestureEventsReceived, 0);
    QCOMPARE(child->gestureOverrideEventsReceived, 0);
    QCOMPARE(widget.gestureEventsReceived, 0);
    QCOMPARE(widget.gestureOverrideEventsReceived, 0);

    // enable gestures over the children
    widget.grabGesture(CustomGesture::GestureType);

    widget.reset();
    child->reset();

    sendCustomGesture(&event, child);

    QCOMPARE(child->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(widget.customEventsReceived, 0);

    QCOMPARE(child->gestureEventsReceived, 0);
    QCOMPARE(child->gestureOverrideEventsReceived, 0);
    QCOMPARE(widget.gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(widget.gestureOverrideEventsReceived, 0);
    for(int i = 0; i < widget.events.all.size(); ++i)
        QCOMPARE(widget.events.all.at(i), CustomGesture::GestureType);
    QCOMPARE(widget.events.started.size(), 1);
    QCOMPARE(widget.events.updated.size(), TotalGestureEventsCount - 2);
    QCOMPARE(widget.events.finished.size(), 1);
    QCOMPARE(widget.events.canceled.size(), 0);
}

void tst_Gestures::multipleWidgetOnlyGestureInTree()
{
    GestureWidget parent("parent");
    QVBoxLayout *l = new QVBoxLayout(&parent);
    GestureWidget *child = new GestureWidget("child");
    l->addWidget(child);

    parent.grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);
    child->grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;
    static const int TotalCustomEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialMaybeThreshold + 1;

    // sending events to the child and making sure there is no conflict
    CustomEvent event;
    sendCustomGesture(&event, child);

    QCOMPARE(child->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(parent.customEventsReceived, 0);
    QCOMPARE(child->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(child->gestureOverrideEventsReceived, 0);
    QCOMPARE(parent.gestureEventsReceived, 0);
    QCOMPARE(parent.gestureOverrideEventsReceived, 0);

    parent.reset();
    child->reset();

    // same for the parent widget
    sendCustomGesture(&event, &parent);

    QCOMPARE(child->customEventsReceived, 0);
    QCOMPARE(parent.customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(child->gestureEventsReceived, 0);
    QCOMPARE(child->gestureOverrideEventsReceived, 0);
    QCOMPARE(parent.gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(parent.gestureOverrideEventsReceived, 0);
}

void tst_Gestures::conflictingGestures()
{
    GestureWidget parent("parent");
    QVBoxLayout *l = new QVBoxLayout(&parent);
    GestureWidget *child = new GestureWidget("child");
    l->addWidget(child);

    parent.grabGesture(CustomGesture::GestureType);
    child->grabGesture(CustomGesture::GestureType);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    // child accepts the override, parent will not receive anything
    parent.acceptGestureOverride = false;
    child->acceptGestureOverride = true;

    // sending events to the child and making sure there is no conflict
    CustomEvent event;
    sendCustomGesture(&event, child);

    QCOMPARE(child->gestureOverrideEventsReceived, 1);
    QCOMPARE(child->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(parent.gestureOverrideEventsReceived, 0);
    QCOMPARE(parent.gestureEventsReceived, 0);

    parent.reset();
    child->reset();

    // parent accepts the override
    parent.acceptGestureOverride = true;
    child->acceptGestureOverride = false;

    // sending events to the child and making sure there is no conflict
    sendCustomGesture(&event, child);

    QCOMPARE(child->gestureOverrideEventsReceived, 1);
    QCOMPARE(child->gestureEventsReceived, 0);
    QCOMPARE(parent.gestureOverrideEventsReceived, 1);
    QCOMPARE(parent.gestureEventsReceived, TotalGestureEventsCount);

    parent.reset();
    child->reset();

    // nobody accepts the override, we will send normal events to the closest
    // context (i.e. to the child widget) and it will be propagated and
    // accepted by the parent widget
    parent.acceptGestureOverride = false;
    child->acceptGestureOverride = false;
    child->ignoredGestures << CustomGesture::GestureType;

    // sending events to the child and making sure there is no conflict
    sendCustomGesture(&event, child);

    QCOMPARE(child->gestureOverrideEventsReceived, 1);
    QCOMPARE(child->gestureEventsReceived, 1);
    QCOMPARE(parent.gestureOverrideEventsReceived, 1);
    QCOMPARE(parent.gestureEventsReceived, TotalGestureEventsCount);

    parent.reset();
    child->reset();

    // nobody accepts the override, and nobody accepts the gesture event
    parent.acceptGestureOverride = false;
    child->acceptGestureOverride = false;
    parent.ignoredGestures << CustomGesture::GestureType;
    child->ignoredGestures << CustomGesture::GestureType;

    // sending events to the child and making sure there is no conflict
    sendCustomGesture(&event, child);

    QCOMPARE(child->gestureOverrideEventsReceived, 1);
    QCOMPARE(child->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(parent.gestureOverrideEventsReceived, 1);
    QCOMPARE(parent.gestureEventsReceived, 1);

    parent.reset();
    child->reset();

    // we set an attribute to make sure all gesture events are propagated
    parent.grabGesture(CustomGesture::GestureType, Qt::ReceivePartialGestures);
    parent.acceptGestureOverride = false;
    child->acceptGestureOverride = false;
    parent.ignoredGestures << CustomGesture::GestureType;
    child->ignoredGestures << CustomGesture::GestureType;

    // sending events to the child and making sure there is no conflict
    sendCustomGesture(&event, child);

    QCOMPARE(child->gestureOverrideEventsReceived, 1);
    QCOMPARE(child->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(parent.gestureOverrideEventsReceived, 1);
    QCOMPARE(parent.gestureEventsReceived, TotalGestureEventsCount);

    parent.reset();
    child->reset();

    Qt::GestureType ContinuousGesture = QGestureRecognizer::registerRecognizer(new CustomContinuousGestureRecognizer);
    static const int ContinuousGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialMaybeThreshold + 1;
    child->grabGesture(ContinuousGesture);
    // child accepts override. And it also receives another custom gesture.
    parent.acceptGestureOverride = false;
    child->acceptGestureOverride = true;
    sendCustomGesture(&event, child);

    QCOMPARE(child->gestureOverrideEventsReceived, 1);
    QVERIFY(child->gestureEventsReceived > TotalGestureEventsCount);
    QCOMPARE(child->events.all.count(), TotalGestureEventsCount + ContinuousGestureEventsCount);
    QCOMPARE(parent.gestureOverrideEventsReceived, 0);
    QCOMPARE(parent.gestureEventsReceived, 0);

    QGestureRecognizer::unregisterRecognizer(ContinuousGesture);
}

void tst_Gestures::finishedWithoutStarted()
{
    GestureWidget widget;
    widget.grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);

    // the gesture will claim it finished, but it was never started.
    CustomEvent ev;
    ev.serial = CustomGesture::SerialFinishedThreshold;
    QApplication::sendEvent(&widget, &ev);

    QCOMPARE(widget.customEventsReceived, 1);
    QCOMPARE(widget.gestureEventsReceived, 2);
    QCOMPARE(widget.gestureOverrideEventsReceived, 0);
    QCOMPARE(widget.events.all.size(), 2);
    QCOMPARE(widget.events.started.size(), 1);
    QCOMPARE(widget.events.updated.size(), 0);
    QCOMPARE(widget.events.finished.size(), 1);
    QCOMPARE(widget.events.canceled.size(), 0);
}

void tst_Gestures::unknownGesture()
{
    GestureWidget widget;
    widget.grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);
    widget.grabGesture(Qt::CustomGesture, Qt::DontStartGestureOnChildren);
    widget.grabGesture(Qt::GestureType(Qt::PanGesture+512), Qt::DontStartGestureOnChildren);

    CustomEvent event;
    sendCustomGesture(&event, &widget);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    QCOMPARE(widget.gestureEventsReceived, TotalGestureEventsCount);
}

static const QColor InstanceColors[] = {
    Qt::blue, Qt::red, Qt::green, Qt::gray, Qt::yellow
};

class GestureItem : public QGraphicsObject
{
    Q_OBJECT
    static int InstanceCount;
public:
    GestureItem(const char *name = 0)
    {
        instanceNumber = InstanceCount++;
        if (name) {
            setObjectName(QLatin1String(name));
            setToolTip(name);
        }
        size = QRectF(0, 0, 100, 100);
        customEventsReceived = 0;
        gestureEventsReceived = 0;
        gestureOverrideEventsReceived = 0;
        events.clear();
        overrideEvents.clear();
        acceptGestureOverride = false;

        scene = 0;
    }
    ~GestureItem()
    {
        --InstanceCount;
    }

    int customEventsReceived;
    int gestureEventsReceived;
    int gestureOverrideEventsReceived;
    struct Events
    {
        QList<Qt::GestureType> all;
        QList<Qt::GestureType> started;
        QList<Qt::GestureType> updated;
        QList<Qt::GestureType> finished;
        QList<Qt::GestureType> canceled;

        void clear()
        {
            all.clear();
            started.clear();
            updated.clear();
            finished.clear();
            canceled.clear();
        }
    } events, overrideEvents;

    bool acceptGestureOverride;
    QSet<Qt::GestureType> ignoredGestures;
    QSet<Qt::GestureType> ignoredStartedGestures;
    QSet<Qt::GestureType> ignoredUpdatedGestures;
    QSet<Qt::GestureType> ignoredFinishedGestures;

    QRectF size;
    int instanceNumber;

    void reset()
    {
        customEventsReceived = 0;
        gestureEventsReceived = 0;
        gestureOverrideEventsReceived = 0;
        events.clear();
        overrideEvents.clear();
        ignoredGestures.clear();
        ignoredStartedGestures.clear();
        ignoredUpdatedGestures.clear();
        ignoredFinishedGestures.clear();
    }

    QRectF boundingRect() const
    {
        return size;
    }
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
    {
        QColor color = InstanceColors[instanceNumber % (sizeof(InstanceColors)/sizeof(InstanceColors[0]))];
        p->fillRect(boundingRect(), color);
    }

    bool event(QEvent *event)
    {
        Events *eventsPtr = 0;
        if (event->type() == QEvent::Gesture) {
            ++gestureEventsReceived;
            eventsPtr = &events;
            QGestureEvent *e = static_cast<QGestureEvent *>(event);
            foreach(Qt::GestureType type, ignoredGestures)
                e->ignore(e->gesture(type));
            foreach(QGesture *g, e->gestures()) {
                switch (g->state()) {
                case Qt::GestureStarted:
                    if (ignoredStartedGestures.contains(g->gestureType()))
                        e->ignore(g);
                    break;
                case Qt::GestureUpdated:
                    if (ignoredUpdatedGestures.contains(g->gestureType()))
                        e->ignore(g);
                    break;
                case Qt::GestureFinished:
                    if (ignoredFinishedGestures.contains(g->gestureType()))
                        e->ignore(g);
                    break;
                default:
                    break;
                }
            }
        } else if (event->type() == QEvent::GestureOverride) {
            ++gestureOverrideEventsReceived;
            eventsPtr = &overrideEvents;
            if (acceptGestureOverride)
                event->accept();
        }
        if (eventsPtr) {
            QGestureEvent *e = static_cast<QGestureEvent*>(event);
            QList<QGesture*> gestures = e->gestures();
            foreach(QGesture *g, gestures) {
                eventsPtr->all << g->gestureType();
                switch(g->state()) {
                case Qt::GestureStarted:
                    eventsPtr->started << g->gestureType();
                    emit gestureStarted(e->type(), g);
                    break;
                case Qt::GestureUpdated:
                    eventsPtr->updated << g->gestureType();
                    emit gestureUpdated(e->type(), g);
                    break;
                case Qt::GestureFinished:
                    eventsPtr->finished << g->gestureType();
                    emit gestureFinished(e->type(), g);
                    break;
                case Qt::GestureCanceled:
                    eventsPtr->canceled << g->gestureType();
                    emit gestureCanceled(e->type(), g);
                    break;
                default:
                    qWarning() << "Unknown GestureState enum value:" << static_cast<int>(g->state());
                }
            }
        } else if (event->type() == CustomEvent::EventType) {
            ++customEventsReceived;
        } else {
            return QGraphicsObject::event(event);
        }
        return true;
    }

Q_SIGNALS:
    void gestureStarted(QEvent::Type, QGesture *);
    void gestureUpdated(QEvent::Type, QGesture *);
    void gestureFinished(QEvent::Type, QGesture *);
    void gestureCanceled(QEvent::Type, QGesture *);

public:
    // some arguments for the slots below:
    QGraphicsScene *scene;

public Q_SLOTS:
    void deleteThis() { delete this; }
    void addSelfToScene(QEvent::Type eventType, QGesture *)
    {
        if (eventType == QEvent::Gesture) {
            disconnect(sender(), 0, this, SLOT(addSelfToScene(QEvent::Type,QGesture*)));
            scene->addItem(this);
        }
    }
};
int GestureItem::InstanceCount = 0;

void tst_Gestures::graphicsItemGesture()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item = new GestureItem("item");
    scene.addItem(item);
    item->setPos(100, 100);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    item->grabGesture(CustomGesture::GestureType);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;
    static const int TotalCustomEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialMaybeThreshold + 1;

    CustomEvent event;
    // gesture without hotspot should not be delivered to items in the view
    QTest::ignoreMessage(QtWarningMsg, "QGestureManager::deliverEvent: could not find the target for gesture");
    QTest::ignoreMessage(QtWarningMsg, "QGestureManager::deliverEvent: could not find the target for gesture");
    QTest::ignoreMessage(QtWarningMsg, "QGestureManager::deliverEvent: could not find the target for gesture");
    QTest::ignoreMessage(QtWarningMsg, "QGestureManager::deliverEvent: could not find the target for gesture");
    sendCustomGesture(&event, item, &scene);

    QCOMPARE(item->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(item->gestureEventsReceived, 0);
    QCOMPARE(item->gestureOverrideEventsReceived, 0);

    item->reset();

    // make sure the event is properly delivered if only the hotspot is set.
    event.hotSpot = mapToGlobal(QPointF(10, 10), item, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item, &scene);

    QCOMPARE(item->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(item->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item->gestureOverrideEventsReceived, 0);
    QCOMPARE(item->events.all.size(), TotalGestureEventsCount);
    for(int i = 0; i < item->events.all.size(); ++i)
        QCOMPARE(item->events.all.at(i), CustomGesture::GestureType);
    QCOMPARE(item->events.started.size(), 1);
    QCOMPARE(item->events.updated.size(), TotalGestureEventsCount - 2);
    QCOMPARE(item->events.finished.size(), 1);
    QCOMPARE(item->events.canceled.size(), 0);

    item->reset();

    // send gesture to the item which ignores it.
    item->ignoredGestures << CustomGesture::GestureType;

    event.hotSpot = mapToGlobal(QPointF(10, 10), item, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item, &scene);
    QCOMPARE(item->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(item->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item->gestureOverrideEventsReceived, 0);
}

void tst_Gestures::graphicsView()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item = new GestureItem("item");
    scene.addItem(item);
    item->setPos(100, 100);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    item->grabGesture(CustomGesture::GestureType);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;
    static const int TotalCustomEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialMaybeThreshold + 1;

    CustomEvent event;
    // make sure the event is properly delivered if only the hotspot is set.
    event.hotSpot = mapToGlobal(QPointF(10, 10), item, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item, &scene);

    QCOMPARE(item->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(item->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item->gestureOverrideEventsReceived, 0);

    // change the viewport and try again
    QWidget *newViewport = new QWidget;
    view.setViewport(newViewport);

    item->reset();
    sendCustomGesture(&event, item, &scene);

    QCOMPARE(item->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(item->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item->gestureOverrideEventsReceived, 0);

    // change the scene and try again
    QGraphicsScene newScene;
    item = new GestureItem("newItem");
    newScene.addItem(item);
    item->setPos(100, 100);
    view.setScene(&newScene);

    item->reset();
    // first without a gesture
    sendCustomGesture(&event, item, &newScene);

    QCOMPARE(item->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(item->gestureEventsReceived, 0);
    QCOMPARE(item->gestureOverrideEventsReceived, 0);

    // then grab the gesture and try again
    item->reset();
    item->grabGesture(CustomGesture::GestureType);
    sendCustomGesture(&event, item, &newScene);

    QCOMPARE(item->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(item->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item->gestureOverrideEventsReceived, 0);
}

void tst_Gestures::graphicsItemTreeGesture()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item1 = new GestureItem("item1");
    item1->setPos(100, 100);
    item1->size = QRectF(0, 0, 350, 200);
    scene.addItem(item1);

    GestureItem *item1_child1 = new GestureItem("item1_child1");
    item1_child1->setPos(50, 50);
    item1_child1->size = QRectF(0, 0, 100, 100);
    item1_child1->setParentItem(item1);

    GestureItem *item1_child2 = new GestureItem("item1_child2");
    item1_child2->size = QRectF(0, 0, 100, 100);
    item1_child2->setPos(200, 50);
    item1_child2->setParentItem(item1);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    item1->grabGesture(CustomGesture::GestureType);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    CustomEvent event;
    event.hotSpot = mapToGlobal(QPointF(10, 10), item1_child1, &view);
    event.hasHotSpot = true;

    item1->ignoredGestures << CustomGesture::GestureType;
    sendCustomGesture(&event, item1_child1, &scene);
    QCOMPARE(item1_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1_child1->gestureEventsReceived, 0);
    QCOMPARE(item1_child2->gestureEventsReceived, 0);
    QCOMPARE(item1_child2->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1->gestureEventsReceived, TotalGestureEventsCount);

    item1->reset(); item1_child1->reset(); item1_child2->reset();

    item1_child1->grabGesture(CustomGesture::GestureType);

    item1->ignoredGestures << CustomGesture::GestureType;
    item1_child1->ignoredGestures << CustomGesture::GestureType;
    sendCustomGesture(&event, item1_child1, &scene);
    QCOMPARE(item1_child1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1_child1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1_child2->gestureEventsReceived, 0);
    QCOMPARE(item1_child2->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1->gestureEventsReceived, 1);
}

void tst_Gestures::explicitGraphicsObjectTarget()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item1 = new GestureItem("item1");
    scene.addItem(item1);
    item1->setPos(100, 100);
    item1->setZValue(1);

    GestureItem *item2 = new GestureItem("item2");
    scene.addItem(item2);
    item2->setPos(100, 100);
    item2->setZValue(5);

    GestureItem *item2_child1 = new GestureItem("item2_child1");
    scene.addItem(item2_child1);
    item2_child1->setParentItem(item2);
    item2_child1->setPos(10, 10);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    item1->grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);
    item2->grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);
    item2_child1->grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    // sending events to item1, but the hotSpot is set to item2
    CustomEvent event;
    event.hotSpot = mapToGlobal(QPointF(15, 15), item2, &view);
    event.hasHotSpot = true;

    sendCustomGesture(&event, item1, &scene);

    QCOMPARE(item1->gestureEventsReceived, 0);
    QCOMPARE(item1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item2_child1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item2_child1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item2_child1->events.all.size(), TotalGestureEventsCount);
    for(int i = 0; i < item2_child1->events.all.size(); ++i)
        QCOMPARE(item2_child1->events.all.at(i), CustomGesture::GestureType);
    QCOMPARE(item2_child1->events.started.size(), 1);
    QCOMPARE(item2_child1->events.updated.size(), TotalGestureEventsCount - 2);
    QCOMPARE(item2_child1->events.finished.size(), 1);
    QCOMPARE(item2_child1->events.canceled.size(), 0);
    QCOMPARE(item2->gestureEventsReceived, 0);
    QCOMPARE(item2->gestureOverrideEventsReceived, 1);
}

void tst_Gestures::gestureOverChildGraphicsItem()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item0 = new GestureItem("item0");
    scene.addItem(item0);
    item0->setPos(0, 0);
    item0->grabGesture(CustomGesture::GestureType);
    item0->setZValue(1);

    GestureItem *item1 = new GestureItem("item1");
    scene.addItem(item1);
    item1->setPos(100, 100);
    item1->setZValue(5);

    GestureItem *item2 = new GestureItem("item2");
    scene.addItem(item2);
    item2->setPos(100, 100);
    item2->setZValue(10);

    GestureItem *item2_child1 = new GestureItem("item2_child1");
    scene.addItem(item2_child1);
    item2_child1->setParentItem(item2);
    item2_child1->setPos(0, 0);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    item1->grabGesture(CustomGesture::GestureType);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;
    static const int TotalCustomEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialMaybeThreshold + 1;

    CustomEvent event;
    event.hotSpot = mapToGlobal(QPointF(10, 10), item2_child1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item0, &scene);

    QCOMPARE(item0->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(item2_child1->gestureEventsReceived, 0);
    QCOMPARE(item2_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item2->gestureEventsReceived, 0);
    QCOMPARE(item2->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1->gestureOverrideEventsReceived, 0);

    item0->reset(); item1->reset(); item2->reset(); item2_child1->reset();
    item2->grabGesture(CustomGesture::GestureType);
    item2->ignoredGestures << CustomGesture::GestureType;

    event.hotSpot = mapToGlobal(QPointF(10, 10), item2_child1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item0, &scene);

    QCOMPARE(item2_child1->gestureEventsReceived, 0);
    QCOMPARE(item2_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item2->gestureEventsReceived, 1);
    QCOMPARE(item2->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1->gestureOverrideEventsReceived, 1);

    item0->reset(); item1->reset(); item2->reset(); item2_child1->reset();
    item2->grabGesture(CustomGesture::GestureType);
    item2->ignoredGestures << CustomGesture::GestureType;
    item1->ignoredGestures << CustomGesture::GestureType;

    event.hotSpot = mapToGlobal(QPointF(10, 10), item2_child1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item0, &scene);

    QCOMPARE(item2_child1->gestureEventsReceived, 0);
    QCOMPARE(item2_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item2->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item2->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1->gestureEventsReceived, 1);
    QCOMPARE(item1->gestureOverrideEventsReceived, 1);

    item0->reset(); item1->reset(); item2->reset(); item2_child1->reset();
    item2->grabGesture(CustomGesture::GestureType);
    item2->ignoredGestures << CustomGesture::GestureType;
    item1->ignoredGestures << CustomGesture::GestureType;
    item1->grabGesture(CustomGesture::GestureType, Qt::ReceivePartialGestures);

    event.hotSpot = mapToGlobal(QPointF(10, 10), item2_child1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item0, &scene);

    QCOMPARE(item2_child1->gestureEventsReceived, 0);
    QCOMPARE(item2_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item2->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item2->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1->gestureOverrideEventsReceived, 1);
}

void tst_Gestures::twoGesturesOnDifferentLevel()
{
    GestureWidget parent("parent");
    QVBoxLayout *l = new QVBoxLayout(&parent);
    GestureWidget *child = new GestureWidget("child");
    l->addWidget(child);

    Qt::GestureType SecondGesture = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);

    parent.grabGesture(CustomGesture::GestureType);
    child->grabGesture(SecondGesture);

    CustomEvent event;
    // sending events that form a gesture to one widget, but they will be
    // filtered by two different gesture recognizers and will generate two
    // QGesture objects. Check that those gesture objects are delivered to
    // different widgets properly.
    sendCustomGesture(&event, child);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;
    static const int TotalCustomEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialMaybeThreshold + 1;

    QCOMPARE(child->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(child->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(child->gestureOverrideEventsReceived, 0);
    QCOMPARE(child->events.all.size(), TotalGestureEventsCount);
    for(int i = 0; i < child->events.all.size(); ++i)
        QCOMPARE(child->events.all.at(i), SecondGesture);

    QCOMPARE(parent.gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(parent.gestureOverrideEventsReceived, 0);
    QCOMPARE(parent.events.all.size(), TotalGestureEventsCount);
    for(int i = 0; i < child->events.all.size(); ++i)
        QCOMPARE(parent.events.all.at(i), CustomGesture::GestureType);

    QGestureRecognizer::unregisterRecognizer(SecondGesture);
}

void tst_Gestures::multipleGesturesInTree()
{
    GestureWidget a("A");
    GestureWidget *A = &a;
    GestureWidget *B = new GestureWidget("B", A);
    GestureWidget *C = new GestureWidget("C", B);
    GestureWidget *D = new GestureWidget("D", C);

    Qt::GestureType FirstGesture  = CustomGesture::GestureType;
    Qt::GestureType SecondGesture = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);
    Qt::GestureType ThirdGesture  = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);

    Qt::GestureFlags flags = Qt::ReceivePartialGestures;
    A->grabGesture(FirstGesture,  flags);   // A [1   3]
    A->grabGesture(ThirdGesture,  flags);   // |
    B->grabGesture(SecondGesture, flags);   // B [  2 3]
    B->grabGesture(ThirdGesture,  flags);   // |
    C->grabGesture(FirstGesture,  flags);   // C [1 2 3]
    C->grabGesture(SecondGesture, flags);   // |
    C->grabGesture(ThirdGesture,  flags);   // D [1   3]
    D->grabGesture(FirstGesture,  flags);
    D->grabGesture(ThirdGesture,  flags);

    // make sure all widgets ignore events, so they get propagated.
    A->ignoredGestures << FirstGesture << ThirdGesture;
    B->ignoredGestures << SecondGesture << ThirdGesture;
    C->ignoredGestures << FirstGesture << SecondGesture << ThirdGesture;
    D->ignoredGestures << FirstGesture << ThirdGesture;

    CustomEvent event;
    sendCustomGesture(&event, D);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    // gesture override events
    QCOMPARE(D->overrideEvents.all.count(FirstGesture), 1);
    QCOMPARE(D->overrideEvents.all.count(SecondGesture), 0);
    QCOMPARE(D->overrideEvents.all.count(ThirdGesture), 1);

    QCOMPARE(C->overrideEvents.all.count(FirstGesture), 1);
    QCOMPARE(C->overrideEvents.all.count(SecondGesture), 1);
    QCOMPARE(C->overrideEvents.all.count(ThirdGesture), 1);

    QCOMPARE(B->overrideEvents.all.count(FirstGesture), 0);
    QCOMPARE(B->overrideEvents.all.count(SecondGesture), 1);
    QCOMPARE(B->overrideEvents.all.count(ThirdGesture), 1);

    QCOMPARE(A->overrideEvents.all.count(FirstGesture), 1);
    QCOMPARE(A->overrideEvents.all.count(SecondGesture), 0);
    QCOMPARE(A->overrideEvents.all.count(ThirdGesture), 1);

    // normal gesture events
    QCOMPARE(D->events.all.count(FirstGesture), TotalGestureEventsCount);
    QCOMPARE(D->events.all.count(SecondGesture), 0);
    QCOMPARE(D->events.all.count(ThirdGesture), TotalGestureEventsCount);

    QCOMPARE(C->events.all.count(FirstGesture), TotalGestureEventsCount);
    QCOMPARE(C->events.all.count(SecondGesture), TotalGestureEventsCount);
    QCOMPARE(C->events.all.count(ThirdGesture), TotalGestureEventsCount);

    QCOMPARE(B->events.all.count(FirstGesture), 0);
    QCOMPARE(B->events.all.count(SecondGesture), TotalGestureEventsCount);
    QCOMPARE(B->events.all.count(ThirdGesture), TotalGestureEventsCount);

    QCOMPARE(A->events.all.count(FirstGesture), TotalGestureEventsCount);
    QCOMPARE(A->events.all.count(SecondGesture), 0);
    QCOMPARE(A->events.all.count(ThirdGesture), TotalGestureEventsCount);

    QGestureRecognizer::unregisterRecognizer(SecondGesture);
    QGestureRecognizer::unregisterRecognizer(ThirdGesture);
}

void tst_Gestures::multipleGesturesInComplexTree()
{
    GestureWidget a("A");
    GestureWidget *A = &a;
    GestureWidget *B = new GestureWidget("B", A);
    GestureWidget *C = new GestureWidget("C", B);
    GestureWidget *D = new GestureWidget("D", C);

    Qt::GestureType FirstGesture   = CustomGesture::GestureType;
    Qt::GestureType SecondGesture  = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);
    Qt::GestureType ThirdGesture   = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);
    Qt::GestureType FourthGesture  = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);
    Qt::GestureType FifthGesture   = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);
    Qt::GestureType SixthGesture   = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);
    Qt::GestureType SeventhGesture = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);

    Qt::GestureFlags flags = Qt::ReceivePartialGestures;
    A->grabGesture(FirstGesture,   flags); // A [1,3,4]
    A->grabGesture(ThirdGesture,   flags); // |
    A->grabGesture(FourthGesture,  flags); // B [2,3,5]
    B->grabGesture(SecondGesture,  flags); // |
    B->grabGesture(ThirdGesture,   flags); // C [1,2,3,6]
    B->grabGesture(FifthGesture,   flags); // |
    C->grabGesture(FirstGesture,   flags); // D [1,3,7]
    C->grabGesture(SecondGesture,  flags);
    C->grabGesture(ThirdGesture,   flags);
    C->grabGesture(SixthGesture,   flags);
    D->grabGesture(FirstGesture,   flags);
    D->grabGesture(ThirdGesture,   flags);
    D->grabGesture(SeventhGesture, flags);

    // make sure all widgets ignore events, so they get propagated.
    QSet<Qt::GestureType> allGestureTypes;
    allGestureTypes << FirstGesture << SecondGesture << ThirdGesture
            << FourthGesture << FifthGesture << SixthGesture << SeventhGesture;
    A->ignoredGestures = B->ignoredGestures = allGestureTypes;
    C->ignoredGestures = D->ignoredGestures = allGestureTypes;

    CustomEvent event;
    sendCustomGesture(&event, D);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    // gesture override events
    QCOMPARE(D->overrideEvents.all.count(FirstGesture), 1);
    QCOMPARE(D->overrideEvents.all.count(SecondGesture), 0);
    QCOMPARE(D->overrideEvents.all.count(ThirdGesture), 1);

    QCOMPARE(C->overrideEvents.all.count(FirstGesture), 1);
    QCOMPARE(C->overrideEvents.all.count(SecondGesture), 1);
    QCOMPARE(C->overrideEvents.all.count(ThirdGesture), 1);

    QCOMPARE(B->overrideEvents.all.count(FirstGesture), 0);
    QCOMPARE(B->overrideEvents.all.count(SecondGesture), 1);
    QCOMPARE(B->overrideEvents.all.count(ThirdGesture), 1);

    QCOMPARE(A->overrideEvents.all.count(FirstGesture), 1);
    QCOMPARE(A->overrideEvents.all.count(SecondGesture), 0);
    QCOMPARE(A->overrideEvents.all.count(ThirdGesture), 1);

    // normal gesture events
    QCOMPARE(D->events.all.count(FirstGesture), TotalGestureEventsCount);
    QCOMPARE(D->events.all.count(SecondGesture), 0);
    QCOMPARE(D->events.all.count(ThirdGesture), TotalGestureEventsCount);
    QCOMPARE(D->events.all.count(FourthGesture), 0);
    QCOMPARE(D->events.all.count(FifthGesture), 0);
    QCOMPARE(D->events.all.count(SixthGesture), 0);
    QCOMPARE(D->events.all.count(SeventhGesture), TotalGestureEventsCount);

    QCOMPARE(C->events.all.count(FirstGesture), TotalGestureEventsCount);
    QCOMPARE(C->events.all.count(SecondGesture), TotalGestureEventsCount);
    QCOMPARE(C->events.all.count(ThirdGesture), TotalGestureEventsCount);
    QCOMPARE(C->events.all.count(FourthGesture), 0);
    QCOMPARE(C->events.all.count(FifthGesture), 0);
    QCOMPARE(C->events.all.count(SixthGesture), TotalGestureEventsCount);
    QCOMPARE(C->events.all.count(SeventhGesture), 0);

    QCOMPARE(B->events.all.count(FirstGesture), 0);
    QCOMPARE(B->events.all.count(SecondGesture), TotalGestureEventsCount);
    QCOMPARE(B->events.all.count(ThirdGesture), TotalGestureEventsCount);
    QCOMPARE(B->events.all.count(FourthGesture), 0);
    QCOMPARE(B->events.all.count(FifthGesture), TotalGestureEventsCount);
    QCOMPARE(B->events.all.count(SixthGesture), 0);
    QCOMPARE(B->events.all.count(SeventhGesture), 0);

    QCOMPARE(A->events.all.count(FirstGesture), TotalGestureEventsCount);
    QCOMPARE(A->events.all.count(SecondGesture), 0);
    QCOMPARE(A->events.all.count(ThirdGesture), TotalGestureEventsCount);
    QCOMPARE(A->events.all.count(FourthGesture), TotalGestureEventsCount);
    QCOMPARE(A->events.all.count(FifthGesture), 0);
    QCOMPARE(A->events.all.count(SixthGesture), 0);
    QCOMPARE(A->events.all.count(SeventhGesture), 0);

    QGestureRecognizer::unregisterRecognizer(SecondGesture);
    QGestureRecognizer::unregisterRecognizer(ThirdGesture);
    QGestureRecognizer::unregisterRecognizer(FourthGesture);
    QGestureRecognizer::unregisterRecognizer(FifthGesture);
    QGestureRecognizer::unregisterRecognizer(SixthGesture);
    QGestureRecognizer::unregisterRecognizer(SeventhGesture);
}

void tst_Gestures::testMapToScene()
{
    QGesture gesture;
    QList<QGesture*> list;
    list << &gesture;
    QGestureEvent event(list);
    QCOMPARE(event.mapToGraphicsScene(gesture.hotSpot()), QPointF()); // not set, can't do much

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item0 = new GestureItem;
    scene.addItem(item0);
    item0->setPos(14, 16);

    view.show(); // need to show to give it a global coordinate
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    QPoint origin = view.mapToGlobal(QPoint());
    event.setWidget(view.viewport());

    QCOMPARE(event.mapToGraphicsScene(origin + QPoint(100, 200)), view.mapToScene(QPoint(100, 200)));
}

void tst_Gestures::ungrabGesture() // a method on QWidget
{
    class MockGestureWidget : public GestureWidget {
    public:
        MockGestureWidget(const char *name = 0, QWidget *parent = 0)
            : GestureWidget(name, parent) { }


        QSet<QGesture*> gestures;
    protected:
        bool event(QEvent *event) override
        {
            if (event->type() == QEvent::Gesture) {
                QGestureEvent *gestureEvent = static_cast<QGestureEvent*>(event);
                const auto eventGestures = gestureEvent->gestures();
                for (QGesture *g : eventGestures)
                    gestures.insert(g);
            }
            return GestureWidget::event(event);
        }
    };

    MockGestureWidget parent("A");
    MockGestureWidget *a = &parent;
    MockGestureWidget *b = new MockGestureWidget("B", a);

    a->grabGesture(CustomGesture::GestureType, Qt::DontStartGestureOnChildren);
    b->grabGesture(CustomGesture::GestureType);
    b->ignoredGestures << CustomGesture::GestureType;

    CustomEvent event;
    // sending an event will cause the QGesture objects to be instantiated for the widgets
    sendCustomGesture(&event, b);

    QCOMPARE(a->gestures.count(), 1);
    QPointer<QGesture> customGestureA;
    customGestureA = *(a->gestures.begin());
    QVERIFY(!customGestureA.isNull());
    QCOMPARE(customGestureA->gestureType(), CustomGesture::GestureType);

    QCOMPARE(b->gestures.count(), 1);
    QPointer<QGesture> customGestureB;
    customGestureB = *(b->gestures.begin());
    QVERIFY(!customGestureB.isNull());
    QCOMPARE(customGestureA.data(), customGestureB.data());
    QCOMPARE(customGestureB->gestureType(), CustomGesture::GestureType);

    a->gestures.clear();
    // sending an event will cause the QGesture objects to be instantiated for the widget
    sendCustomGesture(&event, a);

    QCOMPARE(a->gestures.count(), 1);
    customGestureA = *(a->gestures.begin());
    QVERIFY(!customGestureA.isNull());
    QCOMPARE(customGestureA->gestureType(), CustomGesture::GestureType);
    QVERIFY(customGestureA.data() != customGestureB.data());

    a->ungrabGesture(CustomGesture::GestureType);
    //We changed the deletion of Gestures to lazy during QT-4022, so we can't ensure the QGesture is deleted until now
    QVERIFY(!customGestureB.isNull());

    a->gestures.clear();
    a->reset();
    // send again to 'b' and make sure a never gets it.
    sendCustomGesture(&event, b);
    //After all Gestures are processed in the QGestureManager, we can ensure the QGesture is now deleted
    QVERIFY(customGestureA.isNull());
    QCOMPARE(a->gestureEventsReceived, 0);
    QCOMPARE(a->gestureOverrideEventsReceived, 0);
}

void tst_Gestures::unregisterRecognizer() // a method on QApplication
{
    /*
     The hardest usecase to get right is when we remove a recognizer while several
     of the gestures it created are in active state and we immediately add a new recognizer
     for the same type (thus replacing the old one).
     The expected result is that all old gestures continue till they are finished/cancelled
     and the new recognizer starts creating gestures immediately at registration.

     This implies that deleting of the recognizer happens only when there are no more gestures
     that it created. (since gestures might have a pointer to the recognizer)
     */

}

void tst_Gestures::autoCancelGestures()
{
    class MockWidget : public GestureWidget {
      public:
        MockWidget(const char *name) : GestureWidget(name), badGestureEvents(0) { }

        bool event(QEvent *event)
        {
            if (event->type() == QEvent::Gesture) {
                QGestureEvent *ge = static_cast<QGestureEvent*>(event);
                if (ge->gestures().count() != 1)
                    ++badGestureEvents;   // event should contain exactly one gesture
                ge->gestures().first()->setGestureCancelPolicy(QGesture::CancelAllInContext);
            }
            return GestureWidget::event(event);
        }

        int badGestureEvents;
    };

    const Qt::GestureType secondGesture = QGestureRecognizer::registerRecognizer(new CustomGestureRecognizer);

    MockWidget parent("parent"); // this one sets the cancel policy to CancelAllInContext
    parent.resize(300, 100);
    parent.setWindowFlags(Qt::X11BypassWindowManagerHint);
    GestureWidget *child = new GestureWidget("child", &parent);
    child->setGeometry(10, 10, 100, 80);

    parent.grabGesture(CustomGesture::GestureType);
    child->grabGesture(secondGesture);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    /*
      An event is sent to both the child and the parent, when the child gets it a gesture is triggered
      and send to the child.
      When the parent gets the event a new gesture is triggered and delivered to the parent. When the
      parent gets it he accepts it and that causes the cancel policy to activate.
      The cause of that is the gesture for the child is cancelled and send to the child as such.
    */
    CustomEvent event;
    event.serial = CustomGesture::SerialStartedThreshold;
    QApplication::sendEvent(child, &event);
    QCOMPARE(child->events.all.count(), 2);
    QCOMPARE(child->events.started.count(), 1);
    QCOMPARE(child->events.canceled.count(), 1);
    QCOMPARE(parent.events.all.count(), 1);

    // clean up, make the parent gesture finish
    event.serial = CustomGesture::SerialFinishedThreshold;
    QApplication::sendEvent(child, &event);
    QCOMPARE(parent.events.all.count(), 2);
    QCOMPARE(parent.badGestureEvents, 0);
}

void tst_Gestures::autoCancelGestures2()
{
    class MockItem : public GestureItem {
      public:
        MockItem(const char *name) : GestureItem(name), badGestureEvents(0) { }

        bool event(QEvent *event) {
            if (event->type() == QEvent::Gesture) {
                QGestureEvent *ge = static_cast<QGestureEvent*>(event);
                if (ge->gestures().count() != 1)
                    ++badGestureEvents;   // event should contain exactly one gesture
                ge->gestures().first()->setGestureCancelPolicy(QGesture::CancelAllInContext);
            }
            return GestureItem::event(event);
        }

        int badGestureEvents;
    };

    const Qt::GestureType secondGesture = QGestureRecognizer ::registerRecognizer(new CustomGestureRecognizer);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    MockItem *parent = new MockItem("parent");
    GestureItem *child = new GestureItem("child");
    child->setParentItem(parent);
    parent->setPos(0, 0);
    child->setPos(10, 10);
    scene.addItem(parent);
    parent->grabGesture(CustomGesture::GestureType);
    child->grabGesture(secondGesture);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    CustomEvent event;
    event.serial = CustomGesture::SerialStartedThreshold;
    event.hasHotSpot = true;
    event.hotSpot = mapToGlobal(QPointF(5, 5), child, &view);
    scene.sendEvent(child, &event);
    QCOMPARE(parent->events.all.count(), 1);
    QCOMPARE(child->events.started.count(), 1);
    QCOMPARE(child->events.canceled.count(), 1);
    QCOMPARE(child->events.all.count(), 2);

    // clean up, make the parent gesture finish
    event.serial = CustomGesture::SerialFinishedThreshold;
    scene.sendEvent(child, &event);
    QCOMPARE(parent->events.all.count(), 2);
    QCOMPARE(parent->badGestureEvents, 0);
}

void tst_Gestures::graphicsViewParentPropagation()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item0 = new GestureItem("item0");
    scene.addItem(item0);
    item0->setPos(0, 0);
    item0->grabGesture(CustomGesture::GestureType);
    item0->setZValue(1);

    GestureItem *item1 = new GestureItem("item1");
    scene.addItem(item1);
    item1->setPos(0, 0);
    item1->setZValue(5);

    GestureItem *item1_c1 = new GestureItem("item1_child1");
    item1_c1->setParentItem(item1);
    item1_c1->setPos(0, 0);

    GestureItem *item1_c1_c1 = new GestureItem("item1_child1_child1");
    item1_c1_c1->setParentItem(item1_c1);
    item1_c1_c1->setPos(0, 0);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    item0->grabGesture(CustomGesture::GestureType, Qt::ReceivePartialGestures | Qt::IgnoredGesturesPropagateToParent);
    item1->grabGesture(CustomGesture::GestureType, Qt::ReceivePartialGestures | Qt::IgnoredGesturesPropagateToParent);
    item1_c1->grabGesture(CustomGesture::GestureType, Qt::IgnoredGesturesPropagateToParent);
    item1_c1_c1->grabGesture(CustomGesture::GestureType, Qt::ReceivePartialGestures | Qt::IgnoredGesturesPropagateToParent);

    item0->ignoredUpdatedGestures << CustomGesture::GestureType;
    item0->ignoredFinishedGestures << CustomGesture::GestureType;
    item1->ignoredUpdatedGestures << CustomGesture::GestureType;
    item1->ignoredFinishedGestures << CustomGesture::GestureType;
    item1_c1->ignoredUpdatedGestures << CustomGesture::GestureType;
    item1_c1->ignoredFinishedGestures << CustomGesture::GestureType;
    item1_c1_c1->ignoredUpdatedGestures << CustomGesture::GestureType;
    item1_c1_c1->ignoredFinishedGestures << CustomGesture::GestureType;

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    CustomEvent event;
    event.hotSpot = mapToGlobal(QPointF(10, 10), item1_c1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item0, &scene);

    QCOMPARE(item1_c1_c1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1_c1_c1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1_c1->gestureEventsReceived, TotalGestureEventsCount-1);
    QCOMPARE(item1_c1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1->gestureEventsReceived, TotalGestureEventsCount-1);
    QCOMPARE(item1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item0->gestureEventsReceived, 0);
    QCOMPARE(item0->gestureOverrideEventsReceived, 1);
}

void tst_Gestures::panelPropagation()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item0 = new GestureItem("item0");
    scene.addItem(item0);
    item0->setPos(0, 0);
    item0->size = QRectF(0, 0, 200, 200);
    item0->grabGesture(CustomGesture::GestureType);
    item0->setZValue(1);

    GestureItem *item1 = new GestureItem("item1");
    item1->grabGesture(CustomGesture::GestureType);
    scene.addItem(item1);
    item1->setPos(10, 10);
    item1->size = QRectF(0, 0, 180, 180);
    item1->setZValue(2);

    GestureItem *item1_child1 = new GestureItem("item1_child1[panel]");
    item1_child1->setFlags(QGraphicsItem::ItemIsPanel);
    item1_child1->setParentItem(item1);
    item1_child1->grabGesture(CustomGesture::GestureType);
    item1_child1->setPos(10, 10);
    item1_child1->size = QRectF(0, 0, 160, 160);
    item1_child1->setZValue(5);

    GestureItem *item1_child1_child1 = new GestureItem("item1_child1_child1");
    item1_child1_child1->setParentItem(item1_child1);
    item1_child1_child1->grabGesture(CustomGesture::GestureType);
    item1_child1_child1->setPos(10, 10);
    item1_child1_child1->size = QRectF(0, 0, 140, 140);
    item1_child1_child1->setZValue(10);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;
    static const int TotalCustomEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialMaybeThreshold + 1;

    CustomEvent event;
    event.hotSpot = mapToGlobal(QPointF(5, 5), item1_child1_child1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item0, &scene);

    QCOMPARE(item0->customEventsReceived, TotalCustomEventsCount);
    QCOMPARE(item1_child1_child1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1_child1_child1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1_child1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1->gestureEventsReceived, 0);
    QCOMPARE(item1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item0->gestureEventsReceived, 0);
    QCOMPARE(item0->gestureOverrideEventsReceived, 0);

    item0->reset(); item1->reset(); item1_child1->reset(); item1_child1_child1->reset();

    event.hotSpot = mapToGlobal(QPointF(5, 5), item1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item1, &scene);

    QCOMPARE(item1_child1_child1->gestureEventsReceived, 0);
    QCOMPARE(item1_child1_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1_child1->gestureEventsReceived, 0);
    QCOMPARE(item1_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item0->gestureEventsReceived, 0);
    QCOMPARE(item0->gestureOverrideEventsReceived, 1);

    item0->reset(); item1->reset(); item1_child1->reset(); item1_child1_child1->reset();
    // try with a modal panel
    item1_child1->setPanelModality(QGraphicsItem::PanelModal);

    event.hotSpot = mapToGlobal(QPointF(5, 5), item1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item1, &scene);

    QCOMPARE(item1_child1_child1->gestureEventsReceived, 0);
    QCOMPARE(item1_child1_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1_child1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1->gestureEventsReceived, 0);
    QCOMPARE(item1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item0->gestureEventsReceived, 0);
    QCOMPARE(item0->gestureOverrideEventsReceived, 0);

    item0->reset(); item1->reset(); item1_child1->reset(); item1_child1_child1->reset();
    // try with a modal panel, however set the hotspot to be outside of the
    // panel and its parent
    item1_child1->setPanelModality(QGraphicsItem::PanelModal);

    event.hotSpot = mapToGlobal(QPointF(5, 5), item0, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item1, &scene);

    QCOMPARE(item1_child1_child1->gestureEventsReceived, 0);
    QCOMPARE(item1_child1_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1_child1->gestureEventsReceived, 0);
    QCOMPARE(item1_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1->gestureEventsReceived, 0);
    QCOMPARE(item1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item0->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item0->gestureOverrideEventsReceived, 0);

    item0->reset(); item1->reset(); item1_child1->reset(); item1_child1_child1->reset();
    // try with a scene modal panel
    item1_child1->setPanelModality(QGraphicsItem::SceneModal);

    event.hotSpot = mapToGlobal(QPointF(5, 5), item0, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item0, &scene);

    QCOMPARE(item1_child1_child1->gestureEventsReceived, 0);
    QCOMPARE(item1_child1_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1_child1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1_child1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1->gestureEventsReceived, 0);
    QCOMPARE(item1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item0->gestureEventsReceived, 0);
    QCOMPARE(item0->gestureOverrideEventsReceived, 0);
}

void tst_Gestures::panelStacksBehindParent()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item1 = new GestureItem("item1");
    item1->grabGesture(CustomGesture::GestureType);
    scene.addItem(item1);
    item1->setPos(10, 10);
    item1->size = QRectF(0, 0, 180, 180);
    item1->setZValue(2);

    GestureItem *panel = new GestureItem("panel");
    panel->setFlags(QGraphicsItem::ItemIsPanel | QGraphicsItem::ItemStacksBehindParent);
    panel->setPanelModality(QGraphicsItem::PanelModal);
    panel->setParentItem(item1);
    panel->grabGesture(CustomGesture::GestureType);
    panel->setPos(-10, -10);
    panel->size = QRectF(0, 0, 200, 200);
    panel->setZValue(5);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    CustomEvent event;
    event.hotSpot = mapToGlobal(QPointF(5, 5), item1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item1, &scene);

    QCOMPARE(item1->gestureEventsReceived, 0);
    QCOMPARE(item1->gestureOverrideEventsReceived, 0);
    QCOMPARE(panel->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(panel->gestureOverrideEventsReceived, 0);
}

#ifdef Q_OS_MACOS
void tst_Gestures::deleteMacPanGestureRecognizerTargetWidget()
{
    QWidget window;
    window.resize(400,400);
    QGraphicsScene scene;
    QGraphicsView *view = new QGraphicsView(&scene, &window);
    view->resize(400, 400);
    window.show();

    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTouchDevice *device = QTest::createTouchDevice();
    // QMacOSPenGestureRecognizer will start a timer on a touch press event
    QTest::touchEvent(&window, device).press(1, QPoint(100, 100), &window);
    delete view;

    // wait until after that the QMacOSPenGestureRecognizer timer (300ms) is triggered.
    // This is needed so that the whole test does not finish before the timer triggers
    // and to make sure it crashes while executing *this* function. (otherwise it might give the
    // impression that some of the subsequent test function caused the crash...)

    QTest::qWait(400);  // DO NOT CRASH while waiting
}
#endif

void tst_Gestures::deleteGestureTargetWidget()
{
}

void tst_Gestures::deleteGestureTargetItem_data()
{
    QTest::addColumn<bool>("propagateUpdateGesture");
    QTest::addColumn<QString>("emitter");
    QTest::addColumn<QString>("receiver");
    QTest::addColumn<QByteArray>("signalName");
    QTest::addColumn<QByteArray>("slotName");

    QByteArray gestureUpdated = SIGNAL(gestureUpdated(QEvent::Type,QGesture*));
    QByteArray gestureFinished = SIGNAL(gestureFinished(QEvent::Type,QGesture*));
    QByteArray deleteThis = SLOT(deleteThis());
    QByteArray deleteLater = SLOT(deleteLater());

    QTest::newRow("delete1")
            << false << "item1" << "item1" << gestureUpdated << deleteThis;
    QTest::newRow("delete2")
            << false << "item2" << "item2" << gestureUpdated << deleteThis;
    QTest::newRow("delete3")
            << false << "item1" << "item2" << gestureUpdated << deleteThis;

    QTest::newRow("deleteLater1")
            << false << "item1" << "item1" << gestureUpdated << deleteLater;
    QTest::newRow("deleteLater2")
            << false << "item2" << "item2" << gestureUpdated << deleteLater;
    QTest::newRow("deleteLater3")
            << false << "item1" << "item2" << gestureUpdated << deleteLater;
    QTest::newRow("deleteLater4")
            << false << "item2" << "item1" << gestureUpdated << deleteLater;

    QTest::newRow("delete-self-and-propagate")
            << true << "item2" << "item2" << gestureUpdated << deleteThis;
    QTest::newRow("deleteLater-self-and-propagate")
            << true << "item2" << "item2" << gestureUpdated << deleteLater;
    QTest::newRow("propagate-to-deletedLater")
            << true << "item2" << "item1" << gestureUpdated << deleteLater;
}

void tst_Gestures::deleteGestureTargetItem()
{
    QFETCH(bool, propagateUpdateGesture);
    QFETCH(QString, emitter);
    QFETCH(QString, receiver);
    QFETCH(QByteArray, signalName);
    QFETCH(QByteArray, slotName);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item1 = new GestureItem("item1");
    item1->grabGesture(CustomGesture::GestureType);
    item1->setZValue(2);
    scene.addItem(item1);

    GestureItem *item2 = new GestureItem("item2");
    item2->grabGesture(CustomGesture::GestureType);
    item2->setZValue(5);
    scene.addItem(item2);

    QMap<QString, GestureItem *> items;
    items.insert(item1->objectName(), item1);
    items.insert(item2->objectName(), item2);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    if (propagateUpdateGesture)
        item2->ignoredUpdatedGestures << CustomGesture::GestureType;
    connect(items.value(emitter, 0), signalName, items.value(receiver, 0), slotName);

    // some debug output to see the current test data tag, so if we crash
    // we know which one caused the crash.
    qDebug() << "<-- testing";

    CustomEvent event;
    event.hotSpot = mapToGlobal(QPointF(5, 5), item2, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item1, &scene);
}

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView(QGraphicsScene *scene, QWidget *parent = 0)
        : QGraphicsView(scene, parent)
    {
    }

    using QGraphicsView::setViewportMargins;
};

// just making sure that even if the graphicsview has margins hotspot still
// works properly. It should use viewport for converting global coordinates to
// scene coordinates.
void tst_Gestures::viewportCoordinates()
{
    QGraphicsScene scene;
    GraphicsView view(&scene);
    view.setViewportMargins(10,20,15,25);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item1 = new GestureItem("item1");
    item1->grabGesture(CustomGesture::GestureType);
    item1->size = QRectF(0, 0, 3, 3);
    item1->setZValue(2);
    scene.addItem(item1);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    CustomEvent event;
    event.hotSpot = mapToGlobal(item1->boundingRect().center(), item1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item1, &scene);
    QVERIFY(item1->gestureEventsReceived != 0);
}

void tst_Gestures::partialGesturePropagation()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item1 = new GestureItem("item1");
    item1->grabGesture(CustomGesture::GestureType);
    item1->setZValue(8);
    scene.addItem(item1);

    GestureItem *item2 = new GestureItem("item2[partial]");
    item2->grabGesture(CustomGesture::GestureType, Qt::ReceivePartialGestures);
    item2->setZValue(6);
    scene.addItem(item2);

    GestureItem *item3 = new GestureItem("item3");
    item3->grabGesture(CustomGesture::GestureType);
    item3->setZValue(4);
    scene.addItem(item3);

    GestureItem *item4 = new GestureItem("item4[partial]");
    item4->grabGesture(CustomGesture::GestureType, Qt::ReceivePartialGestures);
    item4->setZValue(2);
    scene.addItem(item4);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    item1->ignoredUpdatedGestures << CustomGesture::GestureType;

    CustomEvent event;
    event.hotSpot = mapToGlobal(QPointF(5, 5), item1, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item1, &scene);

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    QCOMPARE(item1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item2->gestureOverrideEventsReceived, 1);
    QCOMPARE(item3->gestureOverrideEventsReceived, 1);
    QCOMPARE(item4->gestureOverrideEventsReceived, 1);

    QCOMPARE(item1->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item2->gestureEventsReceived, TotalGestureEventsCount-2); // except for started and finished
    QCOMPARE(item3->gestureEventsReceived, 0);
    QCOMPARE(item4->gestureEventsReceived, 0);
}

class WinNativePan : public QPanGesture {
public:
    WinNativePan() {}
};

class Pan : public QPanGesture {
public:
    Pan() {}
};

class CustomPan : public QPanGesture {
public:
    CustomPan() {}
};

// Recognizer for active gesture triggers on mouse press
class PanRecognizer : public QGestureRecognizer {
public:
    enum PanType { Platform, Default, Custom };

    PanRecognizer(int id) : m_id(id) {}
    QGesture *create(QObject *) {
        switch(m_id) {
        case Platform: return new WinNativePan();
        case Default:  return new Pan();
        default:       return new CustomPan();
        }
    }

    Result recognize(QGesture *, QObject *, QEvent *) { return QGestureRecognizer::Ignore; }

    const int m_id;
};

void tst_Gestures::testQGestureRecognizerCleanup()
{
    // Clean first the current recognizers in QGManager
    QGestureRecognizer::unregisterRecognizer(Qt::PanGesture);

    // v-- Qt singleton QGManager initialization

    // Mimic QGestureManager: register both default and "platform" recognizers
    // (this is done in windows when QT_NO_NATIVE_GESTURES is not defined)
    PanRecognizer *def = new PanRecognizer(PanRecognizer::Default);
    QGestureRecognizer::registerRecognizer(def);
    PanRecognizer *plt = new PanRecognizer(PanRecognizer::Platform);
    QGestureRecognizer::registerRecognizer(plt);
    qDebug () << "register: default =" << def << "; platform =" << plt;

    // ^-- Qt singleton QGManager initialization

    // Here, application code would start

    // Create QGV (has a QAScrollArea, which uses Qt::PanGesture)
    QMainWindow    *w = new QMainWindow;
    QGraphicsView  *v = new QGraphicsView();
    w->setCentralWidget(v);

    // Unregister Qt recognizers
    QGestureRecognizer::unregisterRecognizer(Qt::PanGesture);

    // Register a custom Pan recognizer
    //QGestureRecognizer::registerRecognizer(new PanRecognizer(PanRecognizer::Custom));

    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));
    delete w;
}

class ReuseCanceledGesturesRecognizer : public QGestureRecognizer
{
public:
    enum Type {
        RmbAndCancelAllType,
        LmbType
    };

    ReuseCanceledGesturesRecognizer(Type type) : m_type(type) {}

    QGesture *create(QObject *) {
        QGesture *g = new QGesture;
        return g;
    }

    Result recognize(QGesture *gesture, QObject *, QEvent *event) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        Qt::MouseButton mouseButton(m_type == LmbType ? Qt::LeftButton : Qt::RightButton);

        switch(event->type()) {
        case QEvent::MouseButtonPress:
            if (me->button() == mouseButton && gesture->state() == Qt::NoGesture) {
                gesture->setHotSpot(QPointF(me->globalPos()));
                if (m_type == RmbAndCancelAllType)
                    gesture->setGestureCancelPolicy(QGesture::CancelAllInContext);
                return QGestureRecognizer::TriggerGesture;
            }
            break;
        case QEvent::MouseButtonRelease:
            if (me->button() == mouseButton && gesture->state() > Qt::NoGesture)
                return QGestureRecognizer::FinishGesture;
        default:
            break;
        }
        return QGestureRecognizer::Ignore;
    }
private:
    Type m_type;
};

class ReuseCanceledGesturesWidget : public QGraphicsWidget
{
  public:
    ReuseCanceledGesturesWidget(Qt::GestureType gestureType = Qt::TapGesture, QGraphicsItem *parent = 0)
        : QGraphicsWidget(parent),
        m_gestureType(gestureType),
        m_started(0), m_updated(0), m_canceled(0), m_finished(0)
    {
    }

    bool event(QEvent *event) {
        if (event->type() == QEvent::Gesture) {
            QGesture *gesture = static_cast<QGestureEvent*>(event)->gesture(m_gestureType);
            if (gesture) {
                switch(gesture->state()) {
                case Qt::GestureStarted: m_started++; break;
                case Qt::GestureUpdated: m_updated++; break;
                case Qt::GestureFinished: m_finished++; break;
                case Qt::GestureCanceled: m_canceled++; break;
                default: break;
                }
            }
            return true;
        }
        if (event->type() == QEvent::GraphicsSceneMousePress) {
            return true;
        }
        return QGraphicsWidget::event(event);
    }

    int started() { return m_started; }
    int updated() { return m_updated; }
    int finished() { return m_finished; }
    int canceled() { return m_canceled; }

  private:
    Qt::GestureType m_gestureType;
    int m_started;
    int m_updated;
    int m_canceled;
    int m_finished;
};

void tst_Gestures::testReuseCanceledGestures()
{
    Qt::GestureType cancellingGestureTypeId = QGestureRecognizer::registerRecognizer(
            new ReuseCanceledGesturesRecognizer(ReuseCanceledGesturesRecognizer::RmbAndCancelAllType));
    Qt::GestureType tapGestureTypeId = QGestureRecognizer::registerRecognizer(
            new ReuseCanceledGesturesRecognizer(ReuseCanceledGesturesRecognizer::LmbType));

    QMainWindow mw;
    mw.setWindowFlags(Qt::X11BypassWindowManagerHint);
    QGraphicsView *gv = new QGraphicsView(&mw);
    QGraphicsScene *scene = new QGraphicsScene;

    gv->setScene(scene);
    scene->setSceneRect(0,0,100,100);

    // Create container and add to the scene
    ReuseCanceledGesturesWidget *container = new ReuseCanceledGesturesWidget;
    container->grabGesture(cancellingGestureTypeId); // << container grabs canceling gesture

    // Create widget and add to the scene
    ReuseCanceledGesturesWidget *target = new ReuseCanceledGesturesWidget(tapGestureTypeId, container);
    target->grabGesture(tapGestureTypeId);

    container->setGeometry(scene->sceneRect());

    scene->addItem(container);

    mw.setCentralWidget(gv);

    // Viewport needs to grab all gestures that widgets in scene grab
    gv->viewport()->grabGesture(cancellingGestureTypeId);
    gv->viewport()->grabGesture(tapGestureTypeId);

    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));

    QPoint targetPos(gv->mapFromScene(target->mapToScene(target->rect().center())));
    targetPos = gv->viewport()->mapFromParent(targetPos);

    // "Tap" starts on child widget
    QTest::mousePress(gv->viewport(), Qt::LeftButton, { }, targetPos);
    QCOMPARE(target->started(),  1);
    QCOMPARE(target->updated(),  0);
    QCOMPARE(target->finished(), 0);
    QCOMPARE(target->canceled(), 0);

    // Canceling gesture starts on parent
    QTest::mousePress(gv->viewport(), Qt::RightButton, { }, targetPos);
    QCOMPARE(target->started(),  1);
    QCOMPARE(target->updated(),  0);
    QCOMPARE(target->finished(), 0);
    QCOMPARE(target->canceled(), 1); // <- child canceled

    // Canceling gesture ends
    QTest::mouseRelease(gv->viewport(), Qt::RightButton, { }, targetPos);
    QCOMPARE(target->started(),  1);
    QCOMPARE(target->updated(),  0);
    QCOMPARE(target->finished(), 0);
    QCOMPARE(target->canceled(), 1);

    // Tap would end if not canceled
    QTest::mouseRelease(gv->viewport(), Qt::LeftButton, { }, targetPos);
    QCOMPARE(target->started(),  1);
    QCOMPARE(target->updated(),  0);
    QCOMPARE(target->finished(), 0);
    QCOMPARE(target->canceled(), 1);

    // New "Tap" starts
    QTest::mousePress(gv->viewport(), Qt::LeftButton, { }, targetPos);
    QCOMPARE(target->started(),  2);
    QCOMPARE(target->updated(),  0);
    QCOMPARE(target->finished(), 0);
    QCOMPARE(target->canceled(), 1);

    QTest::mouseRelease(gv->viewport(), Qt::LeftButton, { }, targetPos);
    QCOMPARE(target->started(),  2);
    QCOMPARE(target->updated(),  0);
    QCOMPARE(target->finished(), 1);
    QCOMPARE(target->canceled(), 1);
}

void tst_Gestures::conflictingGesturesInGraphicsView()
{
    QGraphicsScene scene;
    GraphicsView view(&scene);
    view.setWindowFlags(Qt::X11BypassWindowManagerHint);

    GestureItem *item1 = new GestureItem("item1");
    item1->grabGesture(CustomGesture::GestureType);
    item1->size = QRectF(0, 0, 100, 100);
    item1->setZValue(2);
    scene.addItem(item1);

    GestureItem *item2 = new GestureItem("item2");
    item2->grabGesture(CustomGesture::GestureType);
    item2->size = QRectF(0, 0, 100, 100);
    item2->setZValue(5);
    scene.addItem(item2);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.ensureVisible(scene.sceneRect());

    static const int TotalGestureEventsCount = CustomGesture::SerialFinishedThreshold - CustomGesture::SerialStartedThreshold + 1;

    CustomEvent event;

    // nobody accepts override
    item1->acceptGestureOverride = false;
    item2->acceptGestureOverride = false;
    event.hotSpot = mapToGlobal(item2->boundingRect().center(), item2, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item2, &scene);
    QCOMPARE(item2->gestureOverrideEventsReceived, 1);
    QCOMPARE(item2->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1->gestureEventsReceived, 0);

    item1->reset(); item2->reset();

    // the original target accepts override
    item1->acceptGestureOverride = false;
    item2->acceptGestureOverride = true;
    event.hotSpot = mapToGlobal(item2->boundingRect().center(), item2, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item2, &scene);
    QCOMPARE(item2->gestureOverrideEventsReceived, 1);
    QCOMPARE(item2->gestureEventsReceived, TotalGestureEventsCount);
    QCOMPARE(item1->gestureOverrideEventsReceived, 0);
    QCOMPARE(item1->gestureEventsReceived, 0);

    item1->reset(); item2->reset();

    // the item behind accepts override
    item1->acceptGestureOverride = true;
    item2->acceptGestureOverride = false;
    event.hotSpot = mapToGlobal(item2->boundingRect().center(), item2, &view);
    event.hasHotSpot = true;
    sendCustomGesture(&event, item2, &scene);

    QCOMPARE(item2->gestureOverrideEventsReceived, 1);
    QCOMPARE(item2->gestureEventsReceived, 0);
    QCOMPARE(item1->gestureOverrideEventsReceived, 1);
    QCOMPARE(item1->gestureEventsReceived, TotalGestureEventsCount);
}

class NoConsumeWidgetBug13501 :public QWidget
{
    Q_OBJECT
protected:
    bool event(QEvent *e) {
        if(e->type() == QEvent::Gesture) {
            return false;
        }
        return QWidget::event(e);
    }
};

void tst_Gestures::bug_13501_gesture_not_accepted()
{
    // Create a gesture event that is not accepted by any widget
    // make sure this does not lead to an assert in QGestureManager
    NoConsumeWidgetBug13501 w;
    w.grabGesture(Qt::TapGesture);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    //QTest::mousePress(&ignoreEvent, Qt::LeftButton);
    QTouchDevice *device = QTest::createTouchDevice();
    QTest::touchEvent(&w, device).press(0, QPoint(10, 10), &w);
}

QTEST_MAIN(tst_Gestures)
#include "tst_gestures.moc"
