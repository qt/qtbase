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

#include <QtCore/qparallelanimationgroup.h>

Q_DECLARE_METATYPE(QAbstractAnimation::State)

class tst_QParallelAnimationGroup : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void construction();
    void setCurrentTime();
    void stateChanged();
    void clearGroup();
    void propagateGroupUpdateToChildren();
    void updateChildrenWithRunningGroup();
    void deleteChildrenWithRunningGroup();
    void startChildrenWithStoppedGroup();
    void stopGroupWithRunningChild();
    void startGroupWithRunningChild();
    void zeroDurationAnimation();
    void stopUncontrolledAnimations();
    void loopCount_data();
    void loopCount();
    void autoAdd();
    void pauseResume();

    void crashWhenRemovingUncontrolledAnimation();
};

void tst_QParallelAnimationGroup::initTestCase()
{
    qRegisterMetaType<QAbstractAnimation::State>("QAbstractAnimation::State");
#if defined(Q_OS_DARWIN)
    // give the Darwin app start event queue time to clear
    QTest::qWait(1000);
#endif
}

void tst_QParallelAnimationGroup::construction()
{
    QParallelAnimationGroup animationgroup;
}

class AnimationObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
public:
    AnimationObject(int startValue = 0)
        : v(startValue)
    { }

    int value() const { return v; }
    void setValue(int value) { v = value; }

    int v;
};

class TestAnimation : public QVariantAnimation
{
    Q_OBJECT
public:
    virtual void updateCurrentValue(const QVariant &value) { Q_UNUSED(value)};
    virtual void updateState(QAbstractAnimation::State newState,
                             QAbstractAnimation::State oldState)
    {
        Q_UNUSED(oldState)
        Q_UNUSED(newState)
    };
};

class TestAnimation2 : public QVariantAnimation
{
    Q_OBJECT
public:
    TestAnimation2(QAbstractAnimation *animation) : QVariantAnimation(animation) {}
    TestAnimation2(int duration, QAbstractAnimation *animation) : QVariantAnimation(animation), m_duration(duration) {}

    virtual void updateCurrentValue(const QVariant &value) { Q_UNUSED(value)};
    virtual void updateState(QAbstractAnimation::State newState,
                             QAbstractAnimation::State oldState)
    {
        Q_UNUSED(oldState)
        Q_UNUSED(newState)
    };

    virtual int duration() const {
        return m_duration;
    }
private:
    int m_duration;
};

class UncontrolledAnimation : public QPropertyAnimation
{
    Q_OBJECT
public:
    UncontrolledAnimation(QObject *target, const QByteArray &propertyName, QObject *parent = 0)
        : QPropertyAnimation(target, propertyName, parent), id(0)
    {
        setDuration(250);
        setEndValue(0);
    }

    int duration() const { return -1; /* not time driven */ }

protected:
    void timerEvent(QTimerEvent *event)
    {
        if (event->timerId() == id)
            stop();
    }

    void updateRunning(bool running)
    {
        if (running) {
            id = startTimer(500);
        } else {
            killTimer(id);
            id = 0;
        }
    }

private:
    int id;
};

void tst_QParallelAnimationGroup::setCurrentTime()
{
    AnimationObject p_o1;
    AnimationObject p_o2;
    AnimationObject p_o3;
    AnimationObject t_o1;
    AnimationObject t_o2;

    // parallel operating on different object/properties
    QAnimationGroup *parallel = new QParallelAnimationGroup();
    QVariantAnimation *a1_p_o1 = new QPropertyAnimation(&p_o1, "value");
    QVariantAnimation *a1_p_o2 = new QPropertyAnimation(&p_o2, "value");
    QVariantAnimation *a1_p_o3 = new QPropertyAnimation(&p_o3, "value");
    a1_p_o2->setLoopCount(3);
    parallel->addAnimation(a1_p_o1);
    parallel->addAnimation(a1_p_o2);
    parallel->addAnimation(a1_p_o3);

    UncontrolledAnimation *notTimeDriven = new UncontrolledAnimation(&t_o1, "value");
    QCOMPARE(notTimeDriven->totalDuration(), -1);

    QVariantAnimation *loopsForever = new QPropertyAnimation(&t_o2, "value");
    loopsForever->setLoopCount(-1);
    QCOMPARE(loopsForever->totalDuration(), -1);

    QParallelAnimationGroup group;
    group.addAnimation(parallel);
    group.addAnimation(notTimeDriven);
    group.addAnimation(loopsForever);

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(parallel->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_p_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_p_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_p_o3->state(), QAnimationGroup::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(a1_p_o1->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 1);
    QCOMPARE(notTimeDriven->currentLoopTime(), 1);
    QCOMPARE(loopsForever->currentLoopTime(), 1);

    // Current time = 250
    group.setCurrentTime(250);
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 0);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 250);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoop(), 1);

    // Current time = 251
    group.setCurrentTime(251);
    QCOMPARE(group.currentLoopTime(), 251);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 251);
    QCOMPARE(loopsForever->currentLoopTime(), 1);
}

void tst_QParallelAnimationGroup::stateChanged()
{
    //this ensures that the correct animations are started when starting the group
    TestAnimation *anim1 = new TestAnimation;
    TestAnimation *anim2 = new TestAnimation;
    TestAnimation *anim3 = new TestAnimation;
    TestAnimation *anim4 = new TestAnimation;
    anim1->setDuration(1000);
    anim2->setDuration(2000);
    anim3->setDuration(3000);
    anim4->setDuration(3000);
    QParallelAnimationGroup group;
    group.addAnimation(anim1);
    group.addAnimation(anim2);
    group.addAnimation(anim3);
    group.addAnimation(anim4);

    QSignalSpy spy1(anim1, &TestAnimation::stateChanged);
    QSignalSpy spy2(anim2, &TestAnimation::stateChanged);
    QSignalSpy spy3(anim3, &TestAnimation::stateChanged);
    QSignalSpy spy4(anim4, &TestAnimation::stateChanged);

    QVERIFY(spy1.isValid());
    QVERIFY(spy2.isValid());
    QVERIFY(spy3.isValid());
    QVERIFY(spy4.isValid());

    //first; let's start forward
    group.start();
    //all the animations should be started
    QCOMPARE(spy1.count(), 1);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy1.last().first()), TestAnimation::Running);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy2.last().first()), TestAnimation::Running);
    QCOMPARE(spy3.count(), 1);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy3.last().first()), TestAnimation::Running);
    QCOMPARE(spy4.count(), 1);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy4.last().first()), TestAnimation::Running);

    group.setCurrentTime(1500); //anim1 should be finished
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(spy1.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy1.last().first()), TestAnimation::Stopped);
    QCOMPARE(spy2.count(), 1); //no change
    QCOMPARE(spy3.count(), 1); //no change
    QCOMPARE(spy4.count(), 1); //no change

    group.setCurrentTime(2500); //anim2 should be finished
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(spy1.count(), 2); //no change
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy2.last().first()), TestAnimation::Stopped);
    QCOMPARE(spy3.count(), 1); //no change
    QCOMPARE(spy4.count(), 1); //no change

    group.setCurrentTime(3500); //everything should be finished
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(spy1.count(), 2); //no change
    QCOMPARE(spy2.count(), 2); //no change
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy3.last().first()), TestAnimation::Stopped);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy4.last().first()), TestAnimation::Stopped);

    //cleanup
    spy1.clear();
    spy2.clear();
    spy3.clear();
    spy4.clear();

    //now let's try to reverse that
    group.setDirection(QAbstractAnimation::Backward);
    group.start();

    //only anim3 and anim4 should be started
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(spy1.count(), 0);
    QCOMPARE(spy2.count(), 0);
    QCOMPARE(spy3.count(), 1);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy3.last().first()), TestAnimation::Running);
    QCOMPARE(spy4.count(), 1);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy4.last().first()), TestAnimation::Running);

    group.setCurrentTime(1500); //anim2 should be started
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(spy1.count(), 0); //no change
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy2.last().first()), TestAnimation::Running);
    QCOMPARE(spy3.count(), 1); //no change
    QCOMPARE(spy4.count(), 1); //no change

    group.setCurrentTime(500); //anim1 is finally also started
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(spy1.count(), 1);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy1.last().first()), TestAnimation::Running);
    QCOMPARE(spy2.count(), 1); //no change
    QCOMPARE(spy3.count(), 1); //no change
    QCOMPARE(spy4.count(), 1); //no change

    group.setCurrentTime(0); //everything should be stopped
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(spy1.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy1.last().first()), TestAnimation::Stopped);
    QCOMPARE(spy2.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy2.last().first()), TestAnimation::Stopped);
    QCOMPARE(spy3.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy3.last().first()), TestAnimation::Stopped);
    QCOMPARE(spy4.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy4.last().first()), TestAnimation::Stopped);
}

void tst_QParallelAnimationGroup::clearGroup()
{
    QParallelAnimationGroup group;
    static const int animationCount = 10;

    for (int i = 0; i < animationCount; ++i) {
        new QParallelAnimationGroup(&group);
    }

    QCOMPARE(group.animationCount(), animationCount);

    QPointer<QAbstractAnimation> children[animationCount];
    for (int i = 0; i < animationCount; ++i) {
        QVERIFY(group.animationAt(i) != 0);
        children[i] = group.animationAt(i);
    }

    group.clear();
    QCOMPARE(group.animationCount(), 0);
    QCOMPARE(group.currentLoopTime(), 0);
    for (int i = 0; i < animationCount; ++i)
        QVERIFY(children[i].isNull());
}

void tst_QParallelAnimationGroup::propagateGroupUpdateToChildren()
{
    // this test verifies if group state changes are updating its children correctly
    QParallelAnimationGroup group;

    QObject o;
    o.setProperty("ole", 42);
    QCOMPARE(o.property("ole").toInt(), 42);

    QPropertyAnimation anim1(&o, "ole");
    anim1.setEndValue(43);
    anim1.setDuration(100);
    QVERIFY(!anim1.currentValue().isValid());
    QCOMPARE(anim1.currentValue().toInt(), 0);
    QCOMPARE(o.property("ole").toInt(), 42);

    TestAnimation anim2;
    anim2.setStartValue(0);
    anim2.setEndValue(100);
    anim2.setDuration(200);

    QVERIFY(anim2.currentValue().isValid());
    QCOMPARE(anim2.currentValue().toInt(), 0);

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);

    group.addAnimation(&anim1);
    group.addAnimation(&anim2);

    group.start();

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim1.state(), QAnimationGroup::Running);
    QCOMPARE(anim2.state(), QAnimationGroup::Running);

    group.pause();

    QCOMPARE(group.state(), QAnimationGroup::Paused);
    QCOMPARE(anim1.state(), QAnimationGroup::Paused);
    QCOMPARE(anim2.state(), QAnimationGroup::Paused);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);
}

void tst_QParallelAnimationGroup::updateChildrenWithRunningGroup()
{
    // assert that its possible to modify a child's state directly while their group is running
    QParallelAnimationGroup group;

    TestAnimation anim;
    anim.setStartValue(0);
    anim.setEndValue(100);
    anim.setDuration(200);

    QSignalSpy groupStateChangedSpy(&group, &QParallelAnimationGroup::stateChanged);
    QSignalSpy childStateChangedSpy(&anim, &TestAnimation::stateChanged);

    QVERIFY(groupStateChangedSpy.isValid());
    QVERIFY(childStateChangedSpy.isValid());

    QCOMPARE(groupStateChangedSpy.count(), 0);
    QCOMPARE(childStateChangedSpy.count(), 0);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim.state(), QAnimationGroup::Stopped);

    group.addAnimation(&anim);

    group.start();

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim.state(), QAnimationGroup::Running);

    QCOMPARE(groupStateChangedSpy.count(), 1);
    QCOMPARE(childStateChangedSpy.count(), 1);

    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(groupStateChangedSpy.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(childStateChangedSpy.at(0).first()),
             QAnimationGroup::Running);

    // starting directly a running child will not have any effect
    anim.start();

    QCOMPARE(groupStateChangedSpy.count(), 1);
    QCOMPARE(childStateChangedSpy.count(), 1);

    anim.pause();

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim.state(), QAnimationGroup::Paused);

    // in the animation stops directly, the group will still be running
    anim.stop();

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim.state(), QAnimationGroup::Stopped);
}

void tst_QParallelAnimationGroup::deleteChildrenWithRunningGroup()
{
    // test if children can be activated when their group is stopped
    QParallelAnimationGroup group;

    QVariantAnimation *anim1 = new TestAnimation;
    anim1->setStartValue(0);
    anim1->setEndValue(100);
    anim1->setDuration(200);
    group.addAnimation(anim1);

    QCOMPARE(group.duration(), anim1->duration());

    group.start();
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim1->state(), QAnimationGroup::Running);

    QTest::qWait(80);
    QVERIFY(group.currentLoopTime() > 0);

    delete anim1;
    QCOMPARE(group.animationCount(), 0);
    QCOMPARE(group.duration(), 0);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(group.currentLoopTime(), 0); //that's the invariant
}

void tst_QParallelAnimationGroup::startChildrenWithStoppedGroup()
{
    // test if children can be activated when their group is stopped
    QParallelAnimationGroup group;

    TestAnimation anim1;
    anim1.setStartValue(0);
    anim1.setEndValue(100);
    anim1.setDuration(200);

    TestAnimation anim2;
    anim2.setStartValue(0);
    anim2.setEndValue(100);
    anim2.setDuration(200);

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);

    group.addAnimation(&anim1);
    group.addAnimation(&anim2);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);

    anim1.start();
    anim2.start();
    anim2.pause();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Running);
    QCOMPARE(anim2.state(), QAnimationGroup::Paused);
}

void tst_QParallelAnimationGroup::stopGroupWithRunningChild()
{
    // children that started independently will not be affected by a group stop
    QParallelAnimationGroup group;

    TestAnimation anim1;
    anim1.setStartValue(0);
    anim1.setEndValue(100);
    anim1.setDuration(200);

    TestAnimation anim2;
    anim2.setStartValue(0);
    anim2.setEndValue(100);
    anim2.setDuration(200);

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);

    group.addAnimation(&anim1);
    group.addAnimation(&anim2);

    anim1.start();
    anim2.start();
    anim2.pause();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Running);
    QCOMPARE(anim2.state(), QAnimationGroup::Paused);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Running);
    QCOMPARE(anim2.state(), QAnimationGroup::Paused);

    anim1.stop();
    anim2.stop();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);
}

void tst_QParallelAnimationGroup::startGroupWithRunningChild()
{
    // as the group has precedence over its children, starting a group will restart all the children
    QParallelAnimationGroup group;

    TestAnimation anim1;
    anim1.setStartValue(0);
    anim1.setEndValue(100);
    anim1.setDuration(200);

    TestAnimation anim2;
    anim2.setStartValue(0);
    anim2.setEndValue(100);
    anim2.setDuration(200);

    QSignalSpy stateChangedSpy1(&anim1, &TestAnimation::stateChanged);
    QSignalSpy stateChangedSpy2(&anim2, &TestAnimation::stateChanged);

    QVERIFY(stateChangedSpy1.isValid());
    QVERIFY(stateChangedSpy2.isValid());

    QCOMPARE(stateChangedSpy1.count(), 0);
    QCOMPARE(stateChangedSpy2.count(), 0);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);

    group.addAnimation(&anim1);
    group.addAnimation(&anim2);

    anim1.start();
    anim2.start();
    anim2.pause();

    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy1.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy2.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy2.at(1).first()),
             QAnimationGroup::Paused);

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Running);
    QCOMPARE(anim2.state(), QAnimationGroup::Paused);

    group.start();

    QCOMPARE(stateChangedSpy1.count(), 3);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy1.at(1).first()),
             QAnimationGroup::Stopped);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy1.at(2).first()),
             QAnimationGroup::Running);

    QCOMPARE(stateChangedSpy2.count(), 4);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy2.at(2).first()),
             QAnimationGroup::Stopped);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy2.at(3).first()),
             QAnimationGroup::Running);

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim1.state(), QAnimationGroup::Running);
    QCOMPARE(anim2.state(), QAnimationGroup::Running);
}

void tst_QParallelAnimationGroup::zeroDurationAnimation()
{
    QParallelAnimationGroup group;

    TestAnimation anim1;
    anim1.setStartValue(0);
    anim1.setEndValue(100);
    anim1.setDuration(0);

    TestAnimation anim2;
    anim2.setStartValue(0);
    anim2.setEndValue(100);
    anim2.setDuration(100);

    TestAnimation anim3;
    anim3.setStartValue(0);
    anim3.setEndValue(100);
    anim3.setDuration(10);

    QSignalSpy stateChangedSpy1(&anim1, &TestAnimation::stateChanged);
    QSignalSpy finishedSpy1(&anim1, &TestAnimation::finished);

    QVERIFY(stateChangedSpy1.isValid());
    QVERIFY(finishedSpy1.isValid());

    QSignalSpy stateChangedSpy2(&anim2, &TestAnimation::stateChanged);
    QSignalSpy finishedSpy2(&anim2, &TestAnimation::finished);

    QVERIFY(stateChangedSpy2.isValid());
    QVERIFY(finishedSpy2.isValid());

    QSignalSpy stateChangedSpy3(&anim3, &TestAnimation::stateChanged);
    QSignalSpy finishedSpy3(&anim3, &TestAnimation::finished);

    QVERIFY(stateChangedSpy3.isValid());
    QVERIFY(finishedSpy3.isValid());

    group.addAnimation(&anim1);
    group.addAnimation(&anim2);
    group.addAnimation(&anim3);
    QCOMPARE(stateChangedSpy1.count(), 0);
    group.start();
    QCOMPARE(stateChangedSpy1.count(), 2);
    QCOMPARE(finishedSpy1.count(), 1);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy1.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy1.at(1).first()),
             QAnimationGroup::Stopped);

    QCOMPARE(stateChangedSpy2.count(), 1);
    QCOMPARE(finishedSpy2.count(), 0);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy1.at(0).first()),
             QAnimationGroup::Running);

    QCOMPARE(stateChangedSpy3.count(), 1);
    QCOMPARE(finishedSpy3.count(), 0);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy3.at(0).first()),
             QAnimationGroup::Running);


    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup::Running);
    QCOMPARE(anim3.state(), QAnimationGroup::Running);
    QCOMPARE(group.state(), QAnimationGroup::Running);


    group.stop();
    group.setLoopCount(4);
    stateChangedSpy1.clear();
    stateChangedSpy2.clear();
    stateChangedSpy3.clear();

    group.start();
    QCOMPARE(stateChangedSpy1.count(), 2);
    QCOMPARE(stateChangedSpy2.count(), 1);
    QCOMPARE(stateChangedSpy3.count(), 1);
    group.setCurrentTime(50);
    QCOMPARE(stateChangedSpy1.count(), 2);
    QCOMPARE(stateChangedSpy2.count(), 1);
    QCOMPARE(stateChangedSpy3.count(), 2);
    group.setCurrentTime(150);
    QCOMPARE(stateChangedSpy1.count(), 4);
    QCOMPARE(stateChangedSpy2.count(), 3);
    QCOMPARE(stateChangedSpy3.count(), 4);
    group.setCurrentTime(50);
    QCOMPARE(stateChangedSpy1.count(), 6);
    QCOMPARE(stateChangedSpy2.count(), 5);
    QCOMPARE(stateChangedSpy3.count(), 6);

}

void tst_QParallelAnimationGroup::stopUncontrolledAnimations()
{
    QParallelAnimationGroup group;

    TestAnimation anim1;
    anim1.setStartValue(0);
    anim1.setEndValue(100);
    anim1.setDuration(0);

    AnimationObject o1;
    UncontrolledAnimation notTimeDriven(&o1, "value");
    QCOMPARE(notTimeDriven.totalDuration(), -1);

    TestAnimation loopsForever;
    loopsForever.setStartValue(0);
    loopsForever.setEndValue(100);
    loopsForever.setDuration(100);
    loopsForever.setLoopCount(-1);

    QSignalSpy stateChangedSpy(&anim1, &TestAnimation::stateChanged);
    QVERIFY(stateChangedSpy.isValid());

    group.addAnimation(&anim1);
    group.addAnimation(&notTimeDriven);
    group.addAnimation(&loopsForever);

    group.start();

    QCOMPARE(stateChangedSpy.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy.at(1).first()),
             QAnimationGroup::Stopped);

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup::Running);
    QCOMPARE(loopsForever.state(), QAnimationGroup::Running);
    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);

    notTimeDriven.stop();

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup::Stopped);
    QCOMPARE(loopsForever.state(), QAnimationGroup::Running);

    loopsForever.stop();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup::Stopped);
    QCOMPARE(loopsForever.state(), QAnimationGroup::Stopped);
}

struct AnimState {
    AnimState(int time = -1) : time(time), state(-1) {}
    AnimState(int time, int state) : time(time), state(state) {}
    int time;
    int state;
};

#define Running QAbstractAnimation::Running
#define Stopped QAbstractAnimation::Stopped

Q_DECLARE_METATYPE(AnimState)
void tst_QParallelAnimationGroup::loopCount_data()
{
    QTest::addColumn<bool>("directionBackward");
    QTest::addColumn<int>("setLoopCount");
    QTest::addColumn<int>("initialGroupTime");
    QTest::addColumn<int>("currentGroupTime");
    QTest::addColumn<AnimState>("expected1");
    QTest::addColumn<AnimState>("expected2");
    QTest::addColumn<AnimState>("expected3");

    //                                                                                  D U R A T I O N
    //                                                              100                           60*2                           0
    // direction = Forward
    QTest::newRow("50")  << false << 3 << 0 <<  50 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("100") << false << 3 << 0 << 100 << AnimState(100         ) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("110") << false << 3 << 0 << 110 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("120") << false << 3 << 0 << 120 << AnimState(  0, Running) << AnimState(  0, Running) << AnimState(  0, Stopped);

    QTest::newRow("170") << false << 3 << 0 << 170 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("220") << false << 3 << 0 << 220 << AnimState(100         ) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("230") << false << 3 << 0 << 230 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("240") << false << 3 << 0 << 240 << AnimState(  0, Running) << AnimState(  0, Running) << AnimState(  0, Stopped);

    QTest::newRow("290") << false << 3 << 0 << 290 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("340") << false << 3 << 0 << 340 << AnimState(100         ) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("350") << false << 3 << 0 << 350 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("360") << false << 3 << 0 << 360 << AnimState(100, Stopped) << AnimState( 60         ) << AnimState(  0, Stopped);

    QTest::newRow("410") << false << 3 << 0 << 410 << AnimState(100, Stopped) << AnimState( 60, Stopped) << AnimState(  0, Stopped);
    QTest::newRow("460") << false << 3 << 0 << 460 << AnimState(100, Stopped) << AnimState( 60, Stopped) << AnimState(  0, Stopped);
    QTest::newRow("470") << false << 3 << 0 << 470 << AnimState(100, Stopped) << AnimState( 60, Stopped) << AnimState(  0, Stopped);
    QTest::newRow("480") << false << 3 << 0 << 480 << AnimState(100, Stopped) << AnimState( 60, Stopped) << AnimState(  0, Stopped);

    // direction = Forward, rewind
    QTest::newRow("120-110") << false << 3 << 120 << 110 << AnimState(   0, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("120-50")  << false << 3 << 120 <<  50 << AnimState(  50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("120-0")   << false << 3 << 120 <<  0  << AnimState(   0, Running) << AnimState(  0, Running) << AnimState(  0, Stopped);
    QTest::newRow("300-110") << false << 3 << 300 << 110 << AnimState(   0, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("300-50")  << false << 3 << 300 <<  50 << AnimState(  50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("300-0")   << false << 3 << 300 <<  0  << AnimState(   0, Running) << AnimState(  0, Running) << AnimState(  0, Stopped);
    QTest::newRow("115-105") << false << 3 << 115 << 105 << AnimState(  42, Stopped) << AnimState( 45, Running) << AnimState(  0, Stopped);

    // direction = Backward
    QTest::newRow("b120-120") << true << 3 << 120 << 120 << AnimState( 42, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);
    QTest::newRow("b120-110") << true << 3 << 120 << 110 << AnimState( 42, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("b120-100") << true << 3 << 120 << 100 << AnimState(100, Running) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("b120-50")  << true << 3 << 120 <<  50 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("b120-0")   << true << 3 << 120 <<   0 << AnimState(  0, Stopped) << AnimState(  0, Stopped) << AnimState(  0, Stopped);
    QTest::newRow("b360-170") << true << 3 << 360 << 170 << AnimState( 50, Running) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("b360-220") << true << 3 << 360 << 220 << AnimState(100, Running) << AnimState( 40, Running) << AnimState(  0, Stopped);
    QTest::newRow("b360-210") << true << 3 << 360 << 210 << AnimState( 90, Running) << AnimState( 30, Running) << AnimState(  0, Stopped);
    QTest::newRow("b360-120") << true << 3 << 360 << 120 << AnimState(  0, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);

    // rewind, direction = Backward
    QTest::newRow("b50-110")  << true << 3 <<  50 << 110 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-120")  << true << 3 <<  50 << 120 << AnimState(100, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-140")  << true << 3 <<  50 << 140 << AnimState( 20, Running) << AnimState( 20, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-240")  << true << 3 <<  50 << 240 << AnimState(100, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-260")  << true << 3 <<  50 << 260 << AnimState( 20, Running) << AnimState( 20, Running) << AnimState(  0, Stopped);
    QTest::newRow("b50-350")  << true << 3 <<  50 << 350 << AnimState(100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);

    // infinite looping
    QTest::newRow("inf1220")  << false << -1 <<  0 << 1220 << AnimState( 20, Running) << AnimState( 20, Running) << AnimState(  0, Stopped);
    QTest::newRow("inf1310")  << false << -1 <<  0 << 1310 << AnimState( 100, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);
    // infinite looping, direction = Backward (will only loop once)
    QTest::newRow("b.inf120-120") << true  << -1 << 120 << 120 << AnimState( 42, Stopped) << AnimState( 60, Running) << AnimState(  0, Stopped);
    QTest::newRow("b.inf120-20")  << true  << -1 << 120 <<  20 << AnimState( 20, Running) << AnimState( 20, Running) << AnimState(  0, Stopped);
    QTest::newRow("b.inf120-110") << true  << -1 << 120 << 110 << AnimState( 42, Stopped) << AnimState( 50, Running) << AnimState(  0, Stopped);


}

void tst_QParallelAnimationGroup::loopCount()
{
    QFETCH(bool, directionBackward);
    QFETCH(int, setLoopCount);
    QFETCH(int, initialGroupTime);
    QFETCH(int, currentGroupTime);
    QFETCH(AnimState, expected1);
    QFETCH(AnimState, expected2);
    QFETCH(AnimState, expected3);

    QParallelAnimationGroup group;

    TestAnimation anim1;
    anim1.setStartValue(0);
    anim1.setEndValue(100);
    anim1.setDuration(100);

    TestAnimation anim2;
    anim2.setStartValue(0);
    anim2.setEndValue(100);
    anim2.setDuration(60);  //total 120
    anim2.setLoopCount(2);

    TestAnimation anim3;
    anim3.setStartValue(0);
    anim3.setEndValue(100);
    anim3.setDuration(0);

    group.addAnimation(&anim1);
    group.addAnimation(&anim2);
    group.addAnimation(&anim3);

    group.setLoopCount(setLoopCount);
    if (initialGroupTime >= 0)
        group.setCurrentTime(initialGroupTime);
    if (directionBackward)
        group.setDirection(QAbstractAnimation::Backward);

    group.start();
    if (initialGroupTime >= 0)
        group.setCurrentTime(initialGroupTime);

    anim1.setCurrentTime(42);   // 42 is "untouched"
    anim2.setCurrentTime(42);

    group.setCurrentTime(currentGroupTime);

    QCOMPARE(anim1.currentLoopTime(), expected1.time);
    QCOMPARE(anim2.currentLoopTime(), expected2.time);
    QCOMPARE(anim3.currentLoopTime(), expected3.time);

    if (expected1.state >=0)
        QCOMPARE(int(anim1.state()), expected1.state);
    if (expected2.state >=0)
        QCOMPARE(int(anim2.state()), expected2.state);
    if (expected3.state >=0)
        QCOMPARE(int(anim3.state()), expected3.state);

}

void tst_QParallelAnimationGroup::autoAdd()
{
    QParallelAnimationGroup group;
    QCOMPARE(group.duration(), 0);
    TestAnimation2 *test = new TestAnimation2(250, &group);      // 0, duration = 250;
    QCOMPARE(test->group(), static_cast<QAnimationGroup*>(&group));
    QCOMPARE(test->duration(), 250);
    QCOMPARE(group.duration(), 250);

    test = new TestAnimation2(750, &group);     // 1
    QCOMPARE(test->group(), static_cast<QAnimationGroup*>(&group));
    QCOMPARE(group.duration(), 750);
    test = new TestAnimation2(500, &group);     // 2
    QCOMPARE(test->group(), static_cast<QAnimationGroup*>(&group));
    QCOMPARE(group.duration(), 750);

    delete group.animationAt(1);    // remove the one with duration = 750
    QCOMPARE(group.duration(), 500);

    delete group.animationAt(1);    // remove the one with duration = 500
    QCOMPARE(group.duration(), 250);

    test = static_cast<TestAnimation2*>(group.animationAt(0));
    test->setParent(0);    // remove the last one (with duration = 250)
    QCOMPARE(test->group(), static_cast<QAnimationGroup*>(0));
    QCOMPARE(group.duration(), 0);
}

void tst_QParallelAnimationGroup::pauseResume()
{
    QParallelAnimationGroup group;
    TestAnimation2 *anim = new TestAnimation2(250, &group);      // 0, duration = 250;
    QSignalSpy spy(anim, &TestAnimation::stateChanged);
    QVERIFY(spy.isValid());
    QCOMPARE(group.duration(), 250);
    group.start();
    QTest::qWait(100);
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim->state(), QAnimationGroup::Running);
    QCOMPARE(spy.count(), 1);
    spy.clear();
    const int currentTime = group.currentLoopTime();
    QCOMPARE(anim->currentLoopTime(), currentTime);

    group.pause();
    QCOMPARE(group.state(), QAnimationGroup::Paused);
    QCOMPARE(group.currentLoopTime(), currentTime);
    QCOMPARE(anim->state(), QAnimationGroup::Paused);
    QCOMPARE(anim->currentLoopTime(), currentTime);
    QCOMPARE(spy.count(), 1);
    spy.clear();

    group.resume();
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(group.currentLoopTime(), currentTime);
    QCOMPARE(anim->state(), QAnimationGroup::Running);
    QCOMPARE(anim->currentLoopTime(), currentTime);
    QCOMPARE(spy.count(), 1);

    group.stop();
    spy.clear();
    new TestAnimation2(500, &group);
    group.start();
    QCOMPARE(spy.count(), 1); //the animation should have been started
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy.last().first()), TestAnimation::Running);
    group.setCurrentTime(250); //end of first animation
    QCOMPARE(spy.count(), 2); //the animation should have been stopped
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(spy.last().first()), TestAnimation::Stopped);
    group.pause();
    QCOMPARE(spy.count(), 2); //this shouldn't have changed
    group.resume();
    QCOMPARE(spy.count(), 2); //this shouldn't have changed
}

// This is a regression test for QTBUG-8910, where a crash occurred when the
// last animation was removed from a group.
void tst_QParallelAnimationGroup::crashWhenRemovingUncontrolledAnimation()
{
    QParallelAnimationGroup group;
    TestAnimation *anim = new TestAnimation;
    anim->setLoopCount(-1);
    TestAnimation *anim2 = new TestAnimation;
    anim2->setLoopCount(-1);
    group.addAnimation(anim);
    group.addAnimation(anim2);
    group.start();
    delete anim;
    // it would crash here because the internals of the group would still have a reference to anim
    delete anim2;
}


QTEST_MAIN(tst_QParallelAnimationGroup)
#include "tst_qparallelanimationgroup.moc"
