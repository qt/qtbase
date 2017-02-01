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
#include <QtCore/qanimationgroup.h>
#include <QtCore/qsequentialanimationgroup.h>

Q_DECLARE_METATYPE(QAbstractAnimation::State)

class tst_QSequentialAnimationGroup : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void construction();
    void setCurrentTime();
    void setCurrentTimeWithUncontrolledAnimation();
    void seekingForwards();
    void seekingBackwards();
    void pauseAndResume();
    void restart();
    void looping();
    void startDelay();
    void clearGroup();
    void groupWithZeroDurationAnimations();
    void propagateGroupUpdateToChildren();
    void updateChildrenWithRunningGroup();
    void deleteChildrenWithRunningGroup();
    void startChildrenWithStoppedGroup();
    void stopGroupWithRunningChild();
    void startGroupWithRunningChild();
    void zeroDurationAnimation();
    void stopUncontrolledAnimations();
    void finishWithUncontrolledAnimation();
    void addRemoveAnimation();
    void currentAnimation();
    void currentAnimationWithZeroDuration();
    void insertAnimation();
    void clear();
    void pauseResume();
};

void tst_QSequentialAnimationGroup::initTestCase()
{
    qRegisterMetaType<QAbstractAnimation::State>("QAbstractAnimation::State");
}

void tst_QSequentialAnimationGroup::construction()
{
    QSequentialAnimationGroup animationgroup;
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

class DummyPropertyAnimation : public QPropertyAnimation
{
public:
    DummyPropertyAnimation(QObject *parent = 0) : QPropertyAnimation(parent)
    {
        setTargetObject(&o);
        this->setPropertyName("value");
        setEndValue(0);
    }

    AnimationObject o;
};

class UncontrolledAnimation : public QPropertyAnimation
{
    Q_OBJECT
public:
    UncontrolledAnimation(QObject *target, QObject *parent = 0)
        : QPropertyAnimation(target, "value", parent)
    {
        setDuration(250);
        setEndValue(0);
    }

    int duration() const { return -1; /* not time driven */ }

protected:
    void updateCurrentTime(int currentTime)
    {
        QPropertyAnimation::updateCurrentTime(currentTime);
        if (currentTime >= QPropertyAnimation::duration())
            stop();
    }
};

void tst_QSequentialAnimationGroup::setCurrentTime()
{
    // sequence operating on same object/property
    QAnimationGroup *sequence = new QSequentialAnimationGroup();
    QVariantAnimation *a1_s_o1 = new DummyPropertyAnimation;
    QVariantAnimation *a2_s_o1 = new DummyPropertyAnimation;
    QVariantAnimation *a3_s_o1 = new DummyPropertyAnimation;
    a2_s_o1->setLoopCount(3);
    sequence->addAnimation(a1_s_o1);
    sequence->addAnimation(a2_s_o1);
    sequence->addAnimation(a3_s_o1);

    // sequence operating on different object/properties
    QAnimationGroup *sequence2 = new QSequentialAnimationGroup();
    QVariantAnimation *a1_s_o2 = new DummyPropertyAnimation;
    QVariantAnimation *a1_s_o3 = new DummyPropertyAnimation;
    sequence2->addAnimation(a1_s_o2);
    sequence2->addAnimation(a1_s_o3);

    QSequentialAnimationGroup group;
    group.addAnimation(sequence);
    group.addAnimation(sequence2);

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 250
    group.setCurrentTime(250);
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(sequence->currentLoopTime(), 250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 251
    group.setCurrentTime(251);
    QCOMPARE(group.currentLoopTime(), 251);
    QCOMPARE(sequence->currentLoopTime(), 251);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 750
    group.setCurrentTime(750);
    QCOMPARE(group.currentLoopTime(), 750);
    QCOMPARE(sequence->currentLoopTime(), 750);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1000
    group.setCurrentTime(1000);
    QCOMPARE(group.currentLoopTime(), 1000);
    QCOMPARE(sequence->currentLoopTime(), 1000);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1010
    group.setCurrentTime(1010);
    QCOMPARE(group.currentLoopTime(), 1010);
    QCOMPARE(sequence->currentLoopTime(), 1010);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 10);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1250
    group.setCurrentTime(1250);
    QCOMPARE(group.currentLoopTime(), 1250);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1500
    group.setCurrentTime(1500);
    QCOMPARE(group.currentLoopTime(), 1500);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1750
    group.setCurrentTime(1750);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 500);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 250);

    // Current time = 2000
    group.setCurrentTime(2000);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 500);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 250);
}

void tst_QSequentialAnimationGroup::setCurrentTimeWithUncontrolledAnimation()
{
    AnimationObject t_o1;

    // sequence operating on different object/properties
    QAnimationGroup *sequence = new QSequentialAnimationGroup();
    QPropertyAnimation *a1_s_o1 = new DummyPropertyAnimation;
    QPropertyAnimation *a1_s_o2 = new DummyPropertyAnimation;
    sequence->addAnimation(a1_s_o1);
    sequence->addAnimation(a1_s_o2);

    QPropertyAnimation *notTimeDriven = new UncontrolledAnimation(&t_o1);
    QCOMPARE(notTimeDriven->totalDuration(), -1);

    QAbstractAnimation *loopsForever = new DummyPropertyAnimation;
    loopsForever->setLoopCount(-1);
    QCOMPARE(loopsForever->totalDuration(), -1);

    QSequentialAnimationGroup group;
    group.addAnimation(sequence);
    group.addAnimation(notTimeDriven);
    group.addAnimation(loopsForever);
    group.start();
    group.pause(); // this allows the group to listen for the finish signal of its children

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Paused);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(notTimeDriven->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoopTime(), 0);

    // Current time = 250
    group.setCurrentTime(250);
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(sequence->currentLoopTime(), 250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(notTimeDriven->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoopTime(), 0);

    // Current time = 500
    group.setCurrentTime(500);
    QCOMPARE(group.currentLoopTime(), 500);
    QCOMPARE(sequence->currentLoopTime(), 500);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation *>(notTimeDriven));

    // Current time = 505
    group.setCurrentTime(505);
    QCOMPARE(group.currentLoopTime(), 505);
    QCOMPARE(sequence->currentLoopTime(), 500);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 5);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation *>(notTimeDriven));
    QCOMPARE(sequence->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup::Paused);
    QCOMPARE(loopsForever->state(), QAnimationGroup::Stopped);

    // Current time = 750 (end of notTimeDriven animation)
    group.setCurrentTime(750);
    QCOMPARE(group.currentLoopTime(), 750);
    QCOMPARE(sequence->currentLoopTime(), 500);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 250);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), loopsForever);
    QCOMPARE(sequence->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup::Paused);

    // Current time = 800 (as notTimeDriven was finished at 750, loopsforever should still run)
    group.setCurrentTime(800);
    QCOMPARE(group.currentLoopTime(), 800);
    QCOMPARE(group.currentAnimation(), loopsForever);
    QCOMPARE(sequence->currentLoopTime(), 500);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 250);
    QCOMPARE(loopsForever->currentLoopTime(), 50);

    loopsForever->stop(); // this should stop the group

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup::Stopped);
}

void tst_QSequentialAnimationGroup::seekingForwards()
{

    // sequence operating on same object/property
    QAnimationGroup *sequence = new QSequentialAnimationGroup;
    QVariantAnimation *a1_s_o1 = new DummyPropertyAnimation;
    QVariantAnimation *a2_s_o1 = new DummyPropertyAnimation;
    QVariantAnimation *a3_s_o1 = new DummyPropertyAnimation;
    a2_s_o1->setLoopCount(3);
    sequence->addAnimation(a1_s_o1);
    sequence->addAnimation(a2_s_o1);
    sequence->addAnimation(a3_s_o1);

    // sequence operating on different object/properties
    QAnimationGroup *sequence2 = new QSequentialAnimationGroup;
    QVariantAnimation *a1_s_o2 = new DummyPropertyAnimation;
    QVariantAnimation *a1_s_o3 = new DummyPropertyAnimation;
    sequence2->addAnimation(a1_s_o2);
    sequence2->addAnimation(a1_s_o3);

    QSequentialAnimationGroup group;
    group.addAnimation(sequence);
    group.addAnimation(sequence2);

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // Current time = 1500
    group.setCurrentTime(1500);
    QCOMPARE(group.currentLoopTime(), 1500);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    // this will restart the group
    group.start();
    group.pause();
    QCOMPARE(group.state(), QAnimationGroup::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Paused);
    QCOMPARE(sequence2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup::Stopped);

    // Current time = 1750
    group.setCurrentTime(1750);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 500);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 250);
}

void tst_QSequentialAnimationGroup::seekingBackwards()
{
    // sequence operating on same object/property
    QAnimationGroup *sequence = new QSequentialAnimationGroup();
    QVariantAnimation *a1_s_o1 = new DummyPropertyAnimation;
    QVariantAnimation *a2_s_o1 = new DummyPropertyAnimation;
    QVariantAnimation *a3_s_o1 = new DummyPropertyAnimation;
    a2_s_o1->setLoopCount(3);
    sequence->addAnimation(a1_s_o1);
    sequence->addAnimation(a2_s_o1);
    sequence->addAnimation(a3_s_o1);

    // sequence operating on different object/properties
    QAnimationGroup *sequence2 = new QSequentialAnimationGroup();
    QVariantAnimation *a1_s_o2 = new DummyPropertyAnimation;
    QVariantAnimation *a1_s_o3 = new DummyPropertyAnimation;
    sequence2->addAnimation(a1_s_o2);
    sequence2->addAnimation(a1_s_o3);

    QSequentialAnimationGroup group;
    group.addAnimation(sequence);
    group.addAnimation(sequence2);

    group.start();

    // Current time = 1600
    group.setCurrentTime(1600);
    QCOMPARE(group.currentLoopTime(), 1600);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 350);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 100);

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(sequence->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup::Running);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup::Running);

    // Seeking backwards, current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);

    QEXPECT_FAIL("", "rewinding in nested groups is considered as a restart from the children,"
        "hence they don't reset from their current animation", Continue);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QEXPECT_FAIL("", "rewinding in nested groups is considered as a restart from the children,"
        "hence they don't reset from their current animation", Continue);
    QCOMPARE(a2_s_o1->currentLoop(), 0);
    QEXPECT_FAIL("", "rewinding in nested groups is considered as a restart from the children,"
        "hence they don't reset from their current animation", Continue);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 0);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(sequence->state(), QAnimationGroup::Running);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Running);
    QCOMPARE(sequence2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup::Stopped);

    // Current time = 2000
    group.setCurrentTime(2000);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 1250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 2);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);
    QCOMPARE(sequence2->currentLoopTime(), 500);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 250);

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o3->state(), QAnimationGroup::Stopped);
}

typedef QVector<QAbstractAnimation::State> StateList;

static bool compareStates(const QSignalSpy& spy, const StateList &expectedStates)
{
    bool equals = true;
    for (int i = 0; i < qMax(expectedStates.count(), spy.count()); ++i) {
        if (i >= spy.count() || i >= expectedStates.count()) {
            equals = false;
            break;
        }
        QList<QVariant> args = spy.at(i);
        QAbstractAnimation::State st = expectedStates.at(i);
        QAbstractAnimation::State actual = qvariant_cast<QAbstractAnimation::State>(args.first());
        if (equals && actual != st) {
            equals = false;
            break;
        }
    }
    if (!equals) {
        const char *stateStrings[] = {"Stopped", "Paused", "Running"};
        QString e,a;
        for (int i = 0; i < qMax(expectedStates.count(), spy.count()); ++i) {
            if (i < expectedStates.count()) {
                int exp = int(expectedStates.at(i));
                    if (!e.isEmpty())
                        e += QLatin1String(", ");
                e += QLatin1String(stateStrings[exp]);
            }
            if (i < spy.count()) {
                QList<QVariant> args = spy.at(i);
                QAbstractAnimation::State actual = qvariant_cast<QAbstractAnimation::State>(args.value(1));
                if (!a.isEmpty())
                    a += QLatin1String(", ");
                if (int(actual) >= 0 && int(actual) <= 2) {
                    a += QLatin1String(stateStrings[int(actual)]);
                } else {
                    a += QLatin1String("NaN");
                }
            }

        }
        qDebug("\n"
               "expected (count == %d): %s\n"
               "actual   (count == %d): %s\n", expectedStates.count(), qPrintable(e), spy.count(), qPrintable(a));
    }
    return equals;
}

void tst_QSequentialAnimationGroup::pauseAndResume()
{
    // sequence operating on same object/property
    QAnimationGroup *sequence = new QSequentialAnimationGroup();
    QVariantAnimation *a1_s_o1 = new DummyPropertyAnimation;
    QVariantAnimation *a2_s_o1 = new DummyPropertyAnimation;
    QVariantAnimation *a3_s_o1 = new DummyPropertyAnimation;
    a2_s_o1->setLoopCount(2);
    sequence->addAnimation(a1_s_o1);
    sequence->addAnimation(a2_s_o1);
    sequence->addAnimation(a3_s_o1);
    sequence->setLoopCount(2);

    QSignalSpy a1StateChangedSpy(a1_s_o1, &QVariantAnimation::stateChanged);
    QSignalSpy seqStateChangedSpy(sequence, &QAnimationGroup::stateChanged);

    QVERIFY(a1StateChangedSpy.isValid());
    QVERIFY(seqStateChangedSpy.isValid());

    QSequentialAnimationGroup group;
    group.addAnimation(sequence);

    group.start();
    group.pause();

    // Current time = 1751
    group.setCurrentTime(1751);
    QCOMPARE(group.currentLoopTime(), 1751);
    QCOMPARE(sequence->currentLoopTime(), 751);
    QCOMPARE(sequence->currentLoop(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 1);

    QCOMPARE(group.state(), QAnimationGroup::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup::Paused);

    QCOMPARE(a1StateChangedSpy.count(), 5);     // Running,Paused,Stopped,Running,Stopped
    QCOMPARE(seqStateChangedSpy.count(), 2);    // Running,Paused

    QVERIFY(compareStates(a1StateChangedSpy, (StateList() << QAbstractAnimation::Running
                                              << QAbstractAnimation::Paused
                                              << QAbstractAnimation::Stopped
                                              << QAbstractAnimation::Running
                                              << QAbstractAnimation::Stopped)));

    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(a1StateChangedSpy.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(a1StateChangedSpy.at(1).first()),
             QAnimationGroup::Paused);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(a1StateChangedSpy.at(2).first()),
             QAnimationGroup::Stopped);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(a1StateChangedSpy.at(3).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(a1StateChangedSpy.at(4).first()),
             QAnimationGroup::Stopped);

    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(seqStateChangedSpy.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(seqStateChangedSpy.at(1).first()),
             QAnimationGroup::Paused);

    group.resume();

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(sequence->state(), QAnimationGroup::Running);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup::Running);

    QVERIFY(group.currentLoopTime() >= 1751);
    QVERIFY(sequence->currentLoopTime() >= 751);
    QCOMPARE(sequence->currentLoop(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QVERIFY(a3_s_o1->currentLoopTime() >= 1);

    QCOMPARE(seqStateChangedSpy.count(), 3);    // Running,Paused,Running
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(seqStateChangedSpy.at(2).first()),
             QAnimationGroup::Running);

    group.pause();

    QCOMPARE(group.state(), QAnimationGroup::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup::Paused);

    QVERIFY(group.currentLoopTime() >= 1751);
    QVERIFY(sequence->currentLoopTime() >= 751);
    QCOMPARE(sequence->currentLoop(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QVERIFY(a3_s_o1->currentLoopTime() >= 1);

    QCOMPARE(seqStateChangedSpy.count(), 4);    // Running,Paused,Running,Paused
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(seqStateChangedSpy.at(3).first()),
             QAnimationGroup::Paused);

    group.stop();

    QCOMPARE(seqStateChangedSpy.count(), 5);    // Running,Paused,Running,Paused,Stopped
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(seqStateChangedSpy.at(4).first()),
             QAnimationGroup::Stopped);
}

void tst_QSequentialAnimationGroup::restart()
{
    // sequence operating on same object/property
    QAnimationGroup *sequence = new QSequentialAnimationGroup();
    QSignalSpy seqCurrentAnimChangedSpy(static_cast<QSequentialAnimationGroup*>(sequence), &QSequentialAnimationGroup::currentAnimationChanged);
    QSignalSpy seqStateChangedSpy(sequence, &QAnimationGroup::stateChanged);

    QVERIFY(seqCurrentAnimChangedSpy.isValid());
    QVERIFY(seqStateChangedSpy.isValid());

    QVariantAnimation *anims[3];
    QSignalSpy *animsStateChanged[3];

    for (int i = 0; i < 3; i++) {
        anims[i] = new DummyPropertyAnimation;
        anims[i]->setDuration(100);
        animsStateChanged[i] = new QSignalSpy(anims[i], &QVariantAnimation::stateChanged);
        QVERIFY(animsStateChanged[i]->isValid());
    }

    anims[1]->setLoopCount(2);
    sequence->addAnimation(anims[0]);
    sequence->addAnimation(anims[1]);
    sequence->addAnimation(anims[2]);
    sequence->setLoopCount(2);

    QSequentialAnimationGroup group;
    group.addAnimation(sequence);

    group.start();

    QTest::qWait(500);

    QCOMPARE(group.state(), QAnimationGroup::Running);

    QTest::qWait(300);
    QTRY_COMPARE(group.state(), QAnimationGroup::Stopped);

    for (int i = 0; i < 3; i++) {
        QCOMPARE(animsStateChanged[i]->count(), 4);
        QCOMPARE(qvariant_cast<QAbstractAnimation::State>(animsStateChanged[i]->at(0).first()),
                 QAnimationGroup::Running);
        QCOMPARE(qvariant_cast<QAbstractAnimation::State>(animsStateChanged[i]->at(1).first()),
                 QAnimationGroup::Stopped);
        QCOMPARE(qvariant_cast<QAbstractAnimation::State>(animsStateChanged[i]->at(2).first()),
                 QAnimationGroup::Running);
        QCOMPARE(qvariant_cast<QAbstractAnimation::State>(animsStateChanged[i]->at(3).first()),
                 QAnimationGroup::Stopped);
    }

    QCOMPARE(seqStateChangedSpy.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(seqStateChangedSpy.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(seqStateChangedSpy.at(1).first()),
             QAnimationGroup::Stopped);

    QCOMPARE(seqCurrentAnimChangedSpy.count(), 6);
    for(int i=0; i<seqCurrentAnimChangedSpy.count(); i++)
            QCOMPARE(static_cast<QAbstractAnimation*>(anims[i%3]), qvariant_cast<QAbstractAnimation*>(seqCurrentAnimChangedSpy.at(i).at(0)));

    group.start();

    QCOMPARE(animsStateChanged[0]->count(), 5);
    QCOMPARE(animsStateChanged[1]->count(), 4);
    QCOMPARE(animsStateChanged[2]->count(), 4);
    QCOMPARE(seqStateChangedSpy.count(), 3);
}

void tst_QSequentialAnimationGroup::looping()
{
    // sequence operating on same object/property
    QSequentialAnimationGroup *sequence = new QSequentialAnimationGroup();
    QAbstractAnimation *a1_s_o1 = new DummyPropertyAnimation;
    QAbstractAnimation *a2_s_o1 = new DummyPropertyAnimation;
    QAbstractAnimation *a3_s_o1 = new DummyPropertyAnimation;

    QSignalSpy a1Spy(a1_s_o1, &QAbstractAnimation::stateChanged);
    QSignalSpy a2Spy(a2_s_o1, &QAbstractAnimation::stateChanged);
    QSignalSpy a3Spy(a3_s_o1, &QAbstractAnimation::stateChanged);
    QSignalSpy seqSpy(sequence, &QSequentialAnimationGroup::stateChanged);

    QVERIFY(a1Spy.isValid());
    QVERIFY(a2Spy.isValid());
    QVERIFY(a3Spy.isValid());
    QVERIFY(seqSpy.isValid());

    a2_s_o1->setLoopCount(2);
    sequence->addAnimation(a1_s_o1);
    sequence->addAnimation(a2_s_o1);
    sequence->addAnimation(a3_s_o1);
    sequence->setLoopCount(2);

    QSequentialAnimationGroup group;
    QSignalSpy groupSpy(&group, &QSequentialAnimationGroup::stateChanged);
    QVERIFY(groupSpy.isValid());

    group.addAnimation(sequence);
    group.setLoopCount(2);

    group.start();
    group.pause();

    // Current time = 1750
    group.setCurrentTime(1750);
    QCOMPARE(group.currentLoopTime(), 1750);
    QCOMPARE(sequence->currentLoopTime(), 750);
    QCOMPARE(sequence->currentLoop(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    // this animation is at the beginning because it is the current one inside sequence
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence->currentAnimation(), a3_s_o1);

    QCOMPARE(group.state(), QAnimationGroup::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup::Paused);

    QCOMPARE(a1Spy.count(), 5);     // Running,Paused,Stopped,Running,Stopped
    QVERIFY(compareStates(a1Spy, (StateList() << QAbstractAnimation::Running
                                              << QAbstractAnimation::Paused
                                              << QAbstractAnimation::Stopped
                                              << QAbstractAnimation::Running
                                              << QAbstractAnimation::Stopped)));

    QCOMPARE(a2Spy.count(), 4);     // Running,Stopped,Running,Stopped
    QVERIFY(compareStates(a3Spy, (StateList() << QAbstractAnimation::Running
                                              << QAbstractAnimation::Stopped
                                              << QAbstractAnimation::Running
                                              << QAbstractAnimation::Paused)));

    QCOMPARE(seqSpy.count(), 2);    // Running,Paused
    QCOMPARE(groupSpy.count(), 2);  // Running,Paused

    // Looping, current time = duration + 1
    group.setCurrentTime(group.duration() + 1);
    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(group.currentLoop(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoop(), 0);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoop(), 1);
    // this animation is at the end because it was run on the previous loop
    QCOMPARE(a3_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 250);

    QCOMPARE(group.state(), QAnimationGroup::Paused);
    QCOMPARE(sequence->state(), QAnimationGroup::Paused);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Paused);
    QCOMPARE(a2_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a3_s_o1->state(), QAnimationGroup::Stopped);

    QCOMPARE(a1Spy.count(), 7); // Running,Paused,Stopped,Running,Stopped,Running,Stopped
    QCOMPARE(a2Spy.count(), 4); // Running, Stopped, Running, Stopped
    QVERIFY(compareStates(a3Spy, (StateList() << QAbstractAnimation::Running
                                              << QAbstractAnimation::Stopped
                                              << QAbstractAnimation::Running
                                              << QAbstractAnimation::Paused
                                              << QAbstractAnimation::Stopped)));
    QVERIFY(compareStates(seqSpy, (StateList() << QAbstractAnimation::Running
                                               << QAbstractAnimation::Paused
                                               << QAbstractAnimation::Stopped
                                               << QAbstractAnimation::Running
                                               << QAbstractAnimation::Paused)));
    QCOMPARE(groupSpy.count(), 2);
}

void tst_QSequentialAnimationGroup::startDelay()
{
    QSequentialAnimationGroup group;
    group.addPause(250);
    group.addPause(125);
    QCOMPARE(group.totalDuration(), 375);

    group.start();
    QCOMPARE(group.state(), QAnimationGroup::Running);

    QTest::qWait(500);

    QTRY_COMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(group.currentLoopTime(), 375);
}

void tst_QSequentialAnimationGroup::clearGroup()
{
    QSequentialAnimationGroup group;

    static const int animationCount = 20;

    for (int i = 0; i < animationCount/2; ++i) {
        QSequentialAnimationGroup *subGroup = new QSequentialAnimationGroup(&group);
        group.addPause(100);
        subGroup->addPause(10);
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

void tst_QSequentialAnimationGroup::groupWithZeroDurationAnimations()
{
    QObject o;
    QObject o2;

    o.setProperty("myProperty", 42);
    o.setProperty("myOtherProperty", 13);
    o2.setProperty("myProperty", 42);
    o2.setProperty("myOtherProperty", 13);

    QSequentialAnimationGroup group;

    QVariantAnimation *a1 = new QPropertyAnimation(&o, "myProperty");
    a1->setDuration(0);
    a1->setEndValue(43);
    group.addAnimation(a1);

    //this should just run fine and change nothing
    group.setCurrentTime(0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation*>(a1));

    QVariantAnimation *a2 = new QPropertyAnimation(&o2, "myOtherProperty");
    a2->setDuration(500);
    a2->setEndValue(31);
    group.addAnimation(a2);

    QVariantAnimation *a3 = new QPropertyAnimation(&o, "myProperty");
    a3->setDuration(0);
    a3->setEndValue(44);
    group.addAnimation(a3);

    QVariantAnimation *a4 = new QPropertyAnimation(&o, "myOtherProperty");
    a4->setDuration(250);
    a4->setEndValue(75);
    group.addAnimation(a4);

    QVariantAnimation *a5 = new QPropertyAnimation(&o2, "myProperty");
    a5->setDuration(0);
    a5->setEndValue(12);
    group.addAnimation(a5);

    QCOMPARE(o.property("myProperty").toInt(), 42);
    QCOMPARE(o.property("myOtherProperty").toInt(), 13);
    QCOMPARE(o2.property("myProperty").toInt(), 42);
    QCOMPARE(o2.property("myOtherProperty").toInt(), 13);


    group.start();

    QCOMPARE(o.property("myProperty").toInt(), 43);
    QCOMPARE(o.property("myOtherProperty").toInt(), 13);
    QCOMPARE(o2.property("myProperty").toInt(), 42);
    QCOMPARE(o2.property("myOtherProperty").toInt(), 13);

    QTest::qWait(100);

    int o2val = o2.property("myOtherProperty").toInt();
    QVERIFY(o2val > 13);
    QVERIFY(o2val < 31);
    QCOMPARE(o.property("myProperty").toInt(), 43);
    QCOMPARE(o.property("myOtherProperty").toInt(), 13);

    QTest::qWait(500);

    QTRY_COMPARE(o.property("myProperty").toInt(), 44);
    QCOMPARE(o2.property("myProperty").toInt(), 42);
    QCOMPARE(o2.property("myOtherProperty").toInt(), 31);
    QCOMPARE(a1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a3->state(), QAnimationGroup::Stopped);
    QCOMPARE(a4->state(), QAnimationGroup::Running);
    QCOMPARE(a5->state(), QAnimationGroup::Stopped);
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QTest::qWait(500);

    QTRY_COMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(o.property("myProperty").toInt(), 44);
    QCOMPARE(o.property("myOtherProperty").toInt(), 75);
    QCOMPARE(o2.property("myProperty").toInt(), 12);
    QCOMPARE(o2.property("myOtherProperty").toInt(), 31);
    QCOMPARE(a1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a3->state(), QAnimationGroup::Stopped);
    QCOMPARE(a4->state(), QAnimationGroup::Stopped);
    QCOMPARE(a5->state(), QAnimationGroup::Stopped);
}

void tst_QSequentialAnimationGroup::propagateGroupUpdateToChildren()
{
    // this test verifies if group state changes are updating its children correctly
    QSequentialAnimationGroup group;

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
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);

    group.pause();

    QCOMPARE(group.state(), QAnimationGroup::Paused);
    QCOMPARE(anim1.state(), QAnimationGroup::Paused);
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2.state(), QAnimationGroup::Stopped);
}

void tst_QSequentialAnimationGroup::updateChildrenWithRunningGroup()
{
    // assert that its possible to modify a child's state directly while their group is running
    QSequentialAnimationGroup group;

    TestAnimation anim;
    anim.setStartValue(0);
    anim.setEndValue(100);
    anim.setDuration(200);

    QSignalSpy groupStateChangedSpy(&group, &QSequentialAnimationGroup::stateChanged);
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

void tst_QSequentialAnimationGroup::deleteChildrenWithRunningGroup()
{
    // test if children can be activated when their group is stopped
    QSequentialAnimationGroup group;

    QVariantAnimation *anim1 = new TestAnimation;
    anim1->setStartValue(0);
    anim1->setEndValue(100);
    anim1->setDuration(200);
    group.addAnimation(anim1);

    QCOMPARE(group.duration(), anim1->duration());

    group.start();
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim1->state(), QAnimationGroup::Running);

    QTest::qWait(100);
    QTRY_VERIFY(group.currentLoopTime() > 0);

    delete anim1;
    QCOMPARE(group.animationCount(), 0);
    QCOMPARE(group.duration(), 0);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(group.currentLoopTime(), 0); //that's the invariant
}

void tst_QSequentialAnimationGroup::startChildrenWithStoppedGroup()
{
    // test if children can be activated when their group is stopped
    QSequentialAnimationGroup group;

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

void tst_QSequentialAnimationGroup::stopGroupWithRunningChild()
{
    // children that started independently will not be affected by a group stop
    QSequentialAnimationGroup group;

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

void tst_QSequentialAnimationGroup::startGroupWithRunningChild()
{
    // as the group has precedence over its children, starting a group will restart all the children
    QSequentialAnimationGroup group;

    TestAnimation *anim1 = new TestAnimation();
    anim1->setStartValue(0);
    anim1->setEndValue(100);
    anim1->setDuration(200);

    TestAnimation *anim2 = new TestAnimation();
    anim2->setStartValue(0);
    anim2->setEndValue(100);
    anim2->setDuration(200);

    QSignalSpy stateChangedSpy1(anim1, &TestAnimation::stateChanged);
    QSignalSpy stateChangedSpy2(anim2, &TestAnimation::stateChanged);

    QVERIFY(stateChangedSpy1.isValid());
    QVERIFY(stateChangedSpy2.isValid());

    QCOMPARE(stateChangedSpy1.count(), 0);
    QCOMPARE(stateChangedSpy2.count(), 0);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1->state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup::Stopped);

    group.addAnimation(anim1);
    group.addAnimation(anim2);

    anim1->start();
    anim2->start();
    anim2->pause();

    QVERIFY(compareStates(stateChangedSpy1, (StateList() << QAbstractAnimation::Running)));

    QVERIFY(compareStates(stateChangedSpy2, (StateList() << QAbstractAnimation::Running
                                                         << QAbstractAnimation::Paused)));

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1->state(), QAnimationGroup::Running);
    QCOMPARE(anim2->state(), QAnimationGroup::Paused);

    group.start();

    QVERIFY(compareStates(stateChangedSpy1, (StateList() << QAbstractAnimation::Running
                                                         << QAbstractAnimation::Stopped
                                                         << QAbstractAnimation::Running)));
    QVERIFY(compareStates(stateChangedSpy2, (StateList() << QAbstractAnimation::Running
                                                         << QAbstractAnimation::Paused)));

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim1->state(), QAnimationGroup::Running);
    QCOMPARE(anim2->state(), QAnimationGroup::Paused);

    QTest::qWait(300);

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim1->state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup::Running);

    QCOMPARE(stateChangedSpy2.count(), 4);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy2.at(2).first()),
             QAnimationGroup::Stopped);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy2.at(3).first()),
             QAnimationGroup::Running);

    group.stop();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim1->state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup::Stopped);
}

void tst_QSequentialAnimationGroup::zeroDurationAnimation()
{
    QSequentialAnimationGroup group;

    TestAnimation *anim1 = new TestAnimation();
    anim1->setStartValue(0);
    anim1->setEndValue(100);
    anim1->setDuration(0);

    TestAnimation *anim2 = new TestAnimation();
    anim2->setStartValue(0);
    anim2->setEndValue(100);
    anim2->setDuration(100);

    DummyPropertyAnimation *anim3 = new DummyPropertyAnimation;
    anim3->setEndValue(100);
    anim3->setDuration(0);

    QSignalSpy stateChangedSpy(anim1, &TestAnimation::stateChanged);
    QVERIFY(stateChangedSpy.isValid());

    group.addAnimation(anim1);
    group.addAnimation(anim2);
    group.addAnimation(anim3);
    group.setLoopCount(2);
    group.start();

    QCOMPARE(stateChangedSpy.count(), 2);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(stateChangedSpy.at(1).first()),
             QAnimationGroup::Stopped);

    QCOMPARE(anim1->state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup::Running);
    QCOMPARE(group.state(), QAnimationGroup::Running);

    //now let's try to seek to the next loop
    group.setCurrentTime(group.duration() + 1);
    QCOMPARE(anim1->state(), QAnimationGroup::Stopped);
    QCOMPARE(anim2->state(), QAnimationGroup::Running);
    QCOMPARE(anim3->state(), QAnimationGroup::Stopped);
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim3->o.value(), 100); //anim3 should have been run
}

void tst_QSequentialAnimationGroup::stopUncontrolledAnimations()
{
    QSequentialAnimationGroup group;

    AnimationObject o1;
    UncontrolledAnimation notTimeDriven(&o1);
    QCOMPARE(notTimeDriven.totalDuration(), -1);

    TestAnimation loopsForever;
    loopsForever.setStartValue(0);
    loopsForever.setEndValue(100);
    loopsForever.setDuration(100);
    loopsForever.setLoopCount(-1);

    group.addAnimation(&notTimeDriven);
    group.addAnimation(&loopsForever);

    group.start();

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup::Running);
    QCOMPARE(loopsForever.state(), QAnimationGroup::Stopped);

    notTimeDriven.stop();

    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup::Stopped);
    QCOMPARE(loopsForever.state(), QAnimationGroup::Running);

    loopsForever.stop();

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup::Stopped);
    QCOMPARE(loopsForever.state(), QAnimationGroup::Stopped);
}

void tst_QSequentialAnimationGroup::finishWithUncontrolledAnimation()
{
    AnimationObject o1;

    //1st case:
    //first we test a group with one uncontrolled animation
    QSequentialAnimationGroup group;
    UncontrolledAnimation notTimeDriven(&o1, &group);
    QSignalSpy spy(&group, &QSequentialAnimationGroup::finished);
    QVERIFY(spy.isValid());

    group.start();
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup::Running);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(notTimeDriven.currentLoopTime(), 0);

    QTest::qWait(300); //wait for the end of notTimeDriven
    QTRY_COMPARE(notTimeDriven.state(), QAnimationGroup::Stopped);
    const int actualDuration = notTimeDriven.currentLoopTime();
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(group.currentLoopTime(), actualDuration);
    QCOMPARE(spy.count(), 1);

    //2nd case:
    // lets make sure the seeking will work again
    spy.clear();
    DummyPropertyAnimation anim(&group);
    QSignalSpy animStateChangedSpy(&anim, &DummyPropertyAnimation::stateChanged);
    QVERIFY(animStateChangedSpy.isValid());

    group.setCurrentTime(300);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(notTimeDriven.currentLoopTime(), actualDuration);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation*>(&anim));

    //3rd case:
    //now let's add a perfectly defined animation at the end
    QCOMPARE(animStateChangedSpy.count(), 0);
    group.start();
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(notTimeDriven.state(), QAnimationGroup::Running);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(notTimeDriven.currentLoopTime(), 0);

    QCOMPARE(animStateChangedSpy.count(), 0);

    QTest::qWait(300); //wait for the end of notTimeDriven
    QTRY_COMPARE(notTimeDriven.state(), QAnimationGroup::Stopped);
    QCOMPARE(group.state(), QAnimationGroup::Running);
    QCOMPARE(anim.state(), QAnimationGroup::Running);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation*>(&anim));
    QCOMPARE(animStateChangedSpy.count(), 1);
    QTest::qWait(300); //wait for the end of anim

    QTRY_COMPARE(anim.state(), QAnimationGroup::Stopped);
    QCOMPARE(anim.currentLoopTime(), anim.duration());

    //we should simply be at the end
    QCOMPARE(spy.count(), 1);
    QCOMPARE(animStateChangedSpy.count(), 2);
    QCOMPARE(group.currentLoopTime(), notTimeDriven.currentLoopTime() + anim.currentLoopTime());
}

void tst_QSequentialAnimationGroup::addRemoveAnimation()
{
    //this test is specific to the sequential animation group
    QSequentialAnimationGroup group;

    QCOMPARE(group.duration(), 0);
    QCOMPARE(group.currentLoopTime(), 0);
    QAbstractAnimation *anim1 = new QPropertyAnimation;
    group.addAnimation(anim1);
    QCOMPARE(group.duration(), 250);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), anim1);

    //let's append an animation
    QAbstractAnimation *anim2 = new QPropertyAnimation;
    group.addAnimation(anim2);
    QCOMPARE(group.duration(), 500);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), anim1);

    //let's prepend an animation
    QAbstractAnimation *anim0 = new QPropertyAnimation;
    group.insertAnimation(0, anim0);
    QCOMPARE(group.duration(), 750);
    QCOMPARE(group.currentLoopTime(), 0);
    QCOMPARE(group.currentAnimation(), anim0); //anim0 has become the new currentAnimation

    group.setCurrentTime(300); //anim0 | anim1 | anim2
    QCOMPARE(group.currentLoopTime(), 300);
    QCOMPARE(group.currentAnimation(), anim1);
    QCOMPARE(anim1->currentLoopTime(), 50);

    group.removeAnimation(anim0); //anim1 | anim2
    QCOMPARE(group.currentLoopTime(), 50);
    QCOMPARE(group.currentAnimation(), anim1);
    QCOMPARE(anim1->currentLoopTime(), 50);

    group.setCurrentTime(0);
    group.insertAnimation(0, anim0); //anim0 | anim1 | anim2
    group.setCurrentTime(300);
    QCOMPARE(group.currentLoopTime(), 300);
    QCOMPARE(group.currentAnimation(), anim1);
    QCOMPARE(anim1->currentLoopTime(), 50);

    group.removeAnimation(anim1); //anim0 | anim2
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(group.currentAnimation(), anim2);
    QCOMPARE(anim0->currentLoopTime(), 250);
}

void tst_QSequentialAnimationGroup::currentAnimation()
{
    QSequentialAnimationGroup group;
    QVERIFY(!group.currentAnimation());

    QPropertyAnimation anim;
    anim.setDuration(0);
    group.addAnimation(&anim);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation*>(&anim));
}

void tst_QSequentialAnimationGroup::currentAnimationWithZeroDuration()
{
    QSequentialAnimationGroup group;
    QVERIFY(!group.currentAnimation());

    QPropertyAnimation zero1;
    zero1.setDuration(0);
    QPropertyAnimation zero2;
    zero2.setDuration(0);

    QPropertyAnimation anim;

    QPropertyAnimation zero3;
    zero3.setDuration(0);
    QPropertyAnimation zero4;
    zero4.setDuration(0);


    group.addAnimation(&zero1);
    group.addAnimation(&zero2);
    group.addAnimation(&anim);
    group.addAnimation(&zero3);
    group.addAnimation(&zero4);

    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation*>(&zero1));

    group.setCurrentTime(0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation*>(&anim));

    group.setCurrentTime(group.duration());
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation*>(&zero4));

    group.setDirection(QAbstractAnimation::Backward);

    group.setCurrentTime(0);
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation*>(&zero1));

    group.setCurrentTime(group.duration());
    QCOMPARE(group.currentAnimation(), static_cast<QAbstractAnimation*>(&anim));
}

void tst_QSequentialAnimationGroup::insertAnimation()
{
    QSequentialAnimationGroup group;
    group.setLoopCount(2);
    QPropertyAnimation *anim = new DummyPropertyAnimation(&group);
    QCOMPARE(group.duration(), anim->duration());
    group.setCurrentTime(300);
    QCOMPARE(group.currentLoop(), 1);

    //this will crash if the sequential group calls duration on the created animation
    new QPropertyAnimation(&group);
}


class SequentialAnimationGroup : public QSequentialAnimationGroup
{
    Q_OBJECT
public slots:
    void clear()
    {
        QSequentialAnimationGroup::clear();
    }

    void refill()
    {
        stop();
        clear();
        new DummyPropertyAnimation(this);
        start();
    }

};


void tst_QSequentialAnimationGroup::clear()
{
    SequentialAnimationGroup group;
    QPointer<QAbstractAnimation> anim1 = new DummyPropertyAnimation(&group);
    group.connect(anim1, SIGNAL(finished()), SLOT(clear()));
    new DummyPropertyAnimation(&group);
    QCOMPARE(group.animationCount(), 2);

    group.start();
    QTest::qWait(anim1->duration() + 100);
    QTRY_COMPARE(group.animationCount(), 0);
    QCOMPARE(group.state(), QAbstractAnimation::Stopped);
    QCOMPARE(group.currentLoopTime(), 0);

    anim1 = new DummyPropertyAnimation(&group);
    group.connect(anim1, SIGNAL(finished()), SLOT(refill()));
    group.start();
    QTest::qWait(anim1->duration() + 100);
    QTRY_COMPARE(group.state(), QAbstractAnimation::Running);
    QVERIFY(anim1 == 0); //anim1 should have been deleted
}

void tst_QSequentialAnimationGroup::pauseResume()
{
    QObject dummy;
    dummy.setProperty("foo", 0);
    QParallelAnimationGroup group;
    QPropertyAnimation *anim = new QPropertyAnimation(&dummy, "foo", &group);
    anim->setDuration(250);
    anim->setEndValue(250);
    QSignalSpy spy(anim, &QPropertyAnimation::stateChanged);
    QVERIFY(spy.isValid());
    QCOMPARE(group.duration(), 250);
    group.start();
    QTest::qWait(100);
    QTRY_COMPARE(group.state(), QAnimationGroup::Running);
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
}

QTEST_MAIN(tst_QSequentialAnimationGroup)
#include "tst_qsequentialanimationgroup.moc"
