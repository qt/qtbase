// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/qabstractanimation.h>
#include <QtCore/qanimationgroup.h>
#include <QTest>
#include <QtTest/private/qpropertytesthelper_p.h>

class tst_QAbstractAnimation : public QObject
{
  Q_OBJECT
private slots:
    void construction();
    void destruction();
    void currentLoop();
    void currentLoopTime();
    void currentTime();
    void direction();
    void group();
    void loopCount();
    void state();
    void totalDuration();
    void avoidJumpAtStart();
    void avoidJumpAtStartWithStop();
    void avoidJumpAtStartWithRunning();
    void stateBinding();
    void loopCountBinding();
    void currentTimeBinding();
    void currentLoopBinding();
    void directionBinding();
};

class TestableQAbstractAnimation : public QAbstractAnimation
{
    Q_OBJECT

public:
    TestableQAbstractAnimation() : m_duration(10) {}
    virtual ~TestableQAbstractAnimation() override { }

    int duration() const override { return m_duration; }
    virtual void updateCurrentTime(int) override {}

    void setDuration(int duration) { m_duration = duration; }
private:
    int m_duration;
};

class DummyQAnimationGroup : public QAnimationGroup
{
    Q_OBJECT
public:
    int duration() const override { return 10; }
    virtual void updateCurrentTime(int) override {}
};

void tst_QAbstractAnimation::construction()
{
    TestableQAbstractAnimation anim;
}

void tst_QAbstractAnimation::destruction()
{
    TestableQAbstractAnimation *anim = new TestableQAbstractAnimation;
    delete anim;

    // Animations should stop when deleted
    auto *stopWhenDeleted = new TestableQAbstractAnimation;
    QAbstractAnimation::State lastOldState, lastNewState;
    QObject::connect(stopWhenDeleted, &QAbstractAnimation::stateChanged,
        [&](QAbstractAnimation::State newState, QAbstractAnimation::State oldState) {
            lastNewState = newState;
            lastOldState = oldState;
    });
    stopWhenDeleted->start();
    QCOMPARE(lastOldState, QAbstractAnimation::Stopped);
    QCOMPARE(lastNewState, QAbstractAnimation::Running);
    delete stopWhenDeleted;
    QCOMPARE(lastOldState, QAbstractAnimation::Running);
    QCOMPARE(lastNewState, QAbstractAnimation::Stopped);
}

void tst_QAbstractAnimation::currentLoop()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.currentLoop(), 0);
}

void tst_QAbstractAnimation::currentLoopTime()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.currentLoopTime(), 0);
}

void tst_QAbstractAnimation::currentTime()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.currentTime(), 0);
    anim.setCurrentTime(10);
    QCOMPARE(anim.currentTime(), 10);
}

void tst_QAbstractAnimation::direction()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.direction(), QAbstractAnimation::Forward);
    anim.setDirection(QAbstractAnimation::Backward);
    QCOMPARE(anim.direction(), QAbstractAnimation::Backward);
    anim.setDirection(QAbstractAnimation::Forward);
    QCOMPARE(anim.direction(), QAbstractAnimation::Forward);
}

void tst_QAbstractAnimation::group()
{
    TestableQAbstractAnimation *anim = new TestableQAbstractAnimation;
    DummyQAnimationGroup group;
    group.addAnimation(anim);
    QCOMPARE(anim->group(), &group);
}

void tst_QAbstractAnimation::loopCount()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.loopCount(), 1);
    anim.setLoopCount(10);
    QCOMPARE(anim.loopCount(), 10);
}

void tst_QAbstractAnimation::state()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.state(), QAbstractAnimation::Stopped);
}

void tst_QAbstractAnimation::totalDuration()
{
    TestableQAbstractAnimation anim;
    QCOMPARE(anim.duration(), 10);
    anim.setLoopCount(5);
    QCOMPARE(anim.totalDuration(), 50);
}

void tst_QAbstractAnimation::avoidJumpAtStart()
{
    TestableQAbstractAnimation anim;
    anim.setDuration(1000);

    /*
        the timer shouldn't actually start until we hit the event loop,
        so the sleep should have no effect
    */
    anim.start();
    QTest::qSleep(300);
    QCoreApplication::processEvents();
    QVERIFY(anim.currentTime() < 50);
}

void tst_QAbstractAnimation::avoidJumpAtStartWithStop()
{
    TestableQAbstractAnimation anim;
    anim.setDuration(1000);

    TestableQAbstractAnimation anim2;
    anim2.setDuration(1000);

    TestableQAbstractAnimation anim3;
    anim3.setDuration(1000);

    anim.start();
    QTest::qWait(300);
    anim.stop();

    /*
        same test as avoidJumpAtStart, but after there is a
        running animation that is stopped
    */
    anim2.start();
    QTest::qSleep(300);
    anim3.start();
    QCoreApplication::processEvents();
    QVERIFY(anim2.currentTime() < 50);
    QVERIFY(anim3.currentTime() < 50);
}

void tst_QAbstractAnimation::avoidJumpAtStartWithRunning()
{
    TestableQAbstractAnimation anim;
    anim.setDuration(2000);

    TestableQAbstractAnimation anim2;
    anim2.setDuration(1000);

    TestableQAbstractAnimation anim3;
    anim3.setDuration(1000);

    anim.start();
    QTest::qWait(300);  //make sure timer has started

    /*
        same test as avoidJumpAtStart, but with an
        existing running animation
    */
    anim2.start();
    QTest::qSleep(300); //force large delta for next tick
    anim3.start();
    QCoreApplication::processEvents();
    QVERIFY(anim2.currentTime() < 50);
    QVERIFY(anim3.currentTime() < 50);
}

void tst_QAbstractAnimation::stateBinding()
{
    TestableQAbstractAnimation animation;
    QTestPrivate::testReadOnlyPropertyBasics(animation, QAbstractAnimation::Stopped,
                                             QAbstractAnimation::Running, "state",
                                             [&] { animation.start(); });
}

void tst_QAbstractAnimation::loopCountBinding()
{
    TestableQAbstractAnimation animation;
    QTestPrivate::testReadWritePropertyBasics(animation, 42, 43, "loopCount");
}

void tst_QAbstractAnimation::currentTimeBinding()
{
    TestableQAbstractAnimation animation;

    QProperty<int> currentTimeProperty;
    animation.bindableCurrentTime().setBinding(Qt::makePropertyBinding(currentTimeProperty));
    QCOMPARE(animation.currentTime(), currentTimeProperty);

    // This should cancel the binding
    animation.start();

    currentTimeProperty = 5;
    QVERIFY(animation.currentTime() != currentTimeProperty);

    QTestPrivate::testReadWritePropertyBasics(animation, 6, 7, "currentTime");
}

void tst_QAbstractAnimation::currentLoopBinding()
{
    TestableQAbstractAnimation animation;

    QTestPrivate::testReadOnlyPropertyBasics(animation, 0, 3, "currentLoop", [&] {
        // Trigger an update of currentLoop
        animation.setLoopCount(4);
        // This brings us to the end of the animation, so currentLoop should be loopCount - 1
        animation.setCurrentTime(42);
    });
}

void tst_QAbstractAnimation::directionBinding()
{
    TestableQAbstractAnimation animation;
    QTestPrivate::testReadWritePropertyBasics(animation, QAbstractAnimation::Backward,
                                              QAbstractAnimation::Forward, "direction");

    // setDirection() may trigger a currentLoop update. Make sure the observers
    // are notified about direction and currentLoop changes only after a consistent
    // state is reached.
    QProperty<int> currLoopObserver;
    currLoopObserver.setBinding([&] { return animation.currentLoop(); });

    QProperty<QAbstractAnimation::Direction> directionObserver;
    directionObserver.setBinding([&] { return animation.direction(); });

    animation.setLoopCount(10);

    bool currentLoopChanged = false;
    auto currentLoopHandler = animation.bindableCurrentLoop().onValueChanged([&] {
        QVERIFY(!currentLoopChanged);
        QCOMPARE(currLoopObserver, 9);
        QCOMPARE(directionObserver, QAbstractAnimation::Backward);
        currentLoopChanged = true;
    });

    bool directionChanged = false;
    auto directionHandler = animation.bindableDirection().onValueChanged([&] {
        QVERIFY(!directionChanged);
        QCOMPARE(currLoopObserver, 9);
        QCOMPARE(directionObserver, QAbstractAnimation::Backward);
        directionChanged = true;
    });

    QCOMPARE(animation.direction(), QAbstractAnimation::Forward);
    // This will set currentLoop to 9
    animation.setDirection(QAbstractAnimation::Backward);

    QVERIFY(currentLoopChanged);
    QVERIFY(directionChanged);
}

QTEST_MAIN(tst_QAbstractAnimation)

#include "tst_qabstractanimation.moc"
