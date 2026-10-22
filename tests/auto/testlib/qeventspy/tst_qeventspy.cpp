// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QEventSpy>

class tst_QEventSpy : public QObject
{
    Q_OBJECT

private slots:
    void nullObject();
    void allEvents();
    void singleType();
    void multipleTypes();
    void filter();
    void applicationWide();
    void eventCopy();
    void clear();
    void doesNotFilterOut();
    void objectDestroyed();
};

static const auto UserA = QEvent::Type(QEvent::User + 1);
static const auto UserB = QEvent::Type(QEvent::User + 2);
static const auto UserC = QEvent::Type(QEvent::User + 3);

class PayloadEvent : public QEvent
{
public:
    explicit PayloadEvent(int payload) : QEvent(UserA), payload(payload) {}
    PayloadEvent *clone() const override { return new PayloadEvent(*this); }
    int payload;
};

void tst_QEventSpy::nullObject()
{
    QTest::ignoreMessage(QtWarningMsg, "QEventSpy: Cannot spy on a null object");
    QEventSpy spy(nullptr);
    QVERIFY(!spy.isValid());
    QVERIFY(spy.isEmpty());
}

void tst_QEventSpy::allEvents()
{
    QObject object;
    QEventSpy spy(&object);
    QVERIFY(spy.isValid());
    QCOMPARE(spy.object(), &object);

    QEvent a(UserA);
    QEvent b(UserB);
    QCoreApplication::sendEvent(&object, &a);
    QCoreApplication::sendEvent(&object, &b);
    QCoreApplication::sendEvent(&object, &a);

    QCOMPARE(spy.count(), 3);
    QCOMPARE(spy.size(), 3);
    QCOMPARE(spy.count(UserA), 2);
    QCOMPARE(spy.count(UserB), 1);
    QCOMPARE(spy.count(UserC), 0);
    QVERIFY(spy.contains(UserB));
    QVERIFY(!spy.contains(UserC));
    QCOMPARE(spy.eventList(), QList({ UserA, UserB, UserA }));
    QCOMPARE(spy.receiver(1), &object);
}

void tst_QEventSpy::singleType()
{
    QObject object;
    QEventSpy spy(&object, UserB);

    QEvent a(UserA);
    QEvent b(UserB);
    QCoreApplication::sendEvent(&object, &a);
    QCoreApplication::sendEvent(&object, &b);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0)->type(), UserB);
}

void tst_QEventSpy::multipleTypes()
{
    QObject object;
    QEventSpy spy(&object, { UserA, UserC });

    for (QEvent::Type type : { UserA, UserB, UserC }) {
        QEvent event(type);
        QCoreApplication::sendEvent(&object, &event);
    }

    QCOMPARE(spy.eventList(), QList({ UserA, UserC }));

    QEventSpy allSpy(&object, QList<QEvent::Type>{});
    QEvent b(UserB);
    QCoreApplication::sendEvent(&object, &b);
    QCOMPARE(allSpy.eventList(), QList({ UserB }));
}

void tst_QEventSpy::filter()
{
    QObject object;
    QEventSpy spy(&object, [](QObject *, const QEvent *event) {
        return event->spontaneous();
    });

    QEvent event(UserA);
    QCoreApplication::sendEvent(&object, &event);
    QVERIFY(spy.isEmpty());
}

void tst_QEventSpy::applicationWide()
{
    QObject first;
    QObject second;
    QEventSpy spy(QCoreApplication::instance(), [&](QObject *receiver, const QEvent *event) {
        return receiver == &second && event->type() == UserA;
    });

    QEvent event(UserA);
    QCoreApplication::sendEvent(&first, &event);
    QCoreApplication::sendEvent(&second, &event);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.receiver(0), &second);
}

void tst_QEventSpy::eventCopy()
{
    QObject object;
    QEventSpy spy(&object, UserA);

    {
        PayloadEvent event(42);
        QCoreApplication::sendEvent(&object, &event);
    }

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at<PayloadEvent>(0)->payload, 42);
}

void tst_QEventSpy::clear()
{
    QObject object;
    QEventSpy spy(&object, UserA);

    QEvent event(UserA);
    QCoreApplication::sendEvent(&object, &event);
    QCOMPARE(spy.count(), 1);

    spy.clear();
    QVERIFY(spy.isEmpty());

    QCoreApplication::sendEvent(&object, &event);
    QCOMPARE(spy.count(), 1);
}

void tst_QEventSpy::doesNotFilterOut()
{
    class Receiver : public QObject
    {
    public:
        bool event(QEvent *event) override
        {
            if (event->type() == UserA)
                ++received;
            return QObject::event(event);
        }
        int received = 0;
    } receiver;

    QEventSpy spy(&receiver);
    QEvent event(UserA);
    QCoreApplication::sendEvent(&receiver, &event);
    QCOMPARE(spy.count(UserA), 1);
    QCOMPARE(receiver.received, 1);
}

void tst_QEventSpy::objectDestroyed()
{
    auto *object = new QObject;
    QEventSpy spy(object, UserA);

    QEvent event(UserA);
    QCoreApplication::sendEvent(object, &event);
    delete object;

    QVERIFY(spy.isValid());
    QCOMPARE(spy.object(), nullptr);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(tst_QEventSpy)
#include "tst_qeventspy.moc"
