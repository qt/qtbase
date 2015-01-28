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

#include <QtCore/qanimationgroup.h>
#include <QtCore/qsequentialanimationgroup.h>
#include <QtCore/qparallelanimationgroup.h>

Q_DECLARE_METATYPE(QAbstractAnimation::State)

class tst_QAnimationGroup : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void construction();
    void emptyGroup();
    void setCurrentTime();
    void setParentAutoAdd();
    void beginNestedGroup();
    void addChildTwice();
    void loopWithoutStartValue();
};

void tst_QAnimationGroup::initTestCase()
{
    qRegisterMetaType<QAbstractAnimation::State>("QAbstractAnimation::State");
}

void tst_QAnimationGroup::construction()
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
    virtual void updateState(QAbstractAnimation::State oldState,
                             QAbstractAnimation::State newState)
    {
        Q_UNUSED(oldState)
        Q_UNUSED(newState)
    };
};

class UncontrolledAnimation : public QPropertyAnimation
{
    Q_OBJECT
public:
    UncontrolledAnimation(QObject *target, const QByteArray &propertyName, QObject *parent = 0)
        : QPropertyAnimation(target, propertyName, parent), id(0)
    {
        setDuration(250);
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

void tst_QAnimationGroup::emptyGroup()
{
    QSequentialAnimationGroup group;
    QSignalSpy groupStateChangedSpy(&group, &QSequentialAnimationGroup::stateChanged);
    QVERIFY(groupStateChangedSpy.isValid());

    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    group.start();

    QCOMPARE(groupStateChangedSpy.count(), 2);

    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(groupStateChangedSpy.at(0).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(groupStateChangedSpy.at(1).first()),
             QAnimationGroup::Stopped);

    QCOMPARE(group.state(), QAnimationGroup::Stopped);

    QTest::ignoreMessage(QtWarningMsg, "QAbstractAnimation::pause: Cannot pause a stopped animation");
    group.pause();

    QCOMPARE(groupStateChangedSpy.count(), 2);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);

    group.start();

    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(groupStateChangedSpy.at(2).first()),
             QAnimationGroup::Running);
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(groupStateChangedSpy.at(3).first()),
             QAnimationGroup::Stopped);

    QCOMPARE(group.state(), QAnimationGroup::Stopped);

    group.stop();

    QCOMPARE(groupStateChangedSpy.count(), 4);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
}

void tst_QAnimationGroup::setCurrentTime()
{
    AnimationObject s_o1;
    AnimationObject s_o2;
    AnimationObject s_o3;
    AnimationObject p_o1;
    AnimationObject p_o2;
    AnimationObject p_o3;
    AnimationObject t_o1;
    AnimationObject t_o2;

    // sequence operating on same object/property
    QSequentialAnimationGroup *sequence = new QSequentialAnimationGroup();
    QAbstractAnimation *a1_s_o1 = new QPropertyAnimation(&s_o1, "value");
    QAbstractAnimation *a2_s_o1 = new QPropertyAnimation(&s_o1, "value");
    QAbstractAnimation *a3_s_o1 = new QPropertyAnimation(&s_o1, "value");
    a2_s_o1->setLoopCount(3);
    sequence->addAnimation(a1_s_o1);
    sequence->addAnimation(a2_s_o1);
    sequence->addAnimation(a3_s_o1);

    // sequence operating on different object/properties
    QAnimationGroup *sequence2 = new QSequentialAnimationGroup();
    QAbstractAnimation *a1_s_o2 = new QPropertyAnimation(&s_o2, "value");
    QAbstractAnimation *a1_s_o3 = new QPropertyAnimation(&s_o3, "value");
    sequence2->addAnimation(a1_s_o2);
    sequence2->addAnimation(a1_s_o3);

    // parallel operating on different object/properties
    QAnimationGroup *parallel = new QParallelAnimationGroup();
    QAbstractAnimation *a1_p_o1 = new QPropertyAnimation(&p_o1, "value");
    QAbstractAnimation *a1_p_o2 = new QPropertyAnimation(&p_o2, "value");
    QAbstractAnimation *a1_p_o3 = new QPropertyAnimation(&p_o3, "value");
    a1_p_o2->setLoopCount(3);
    parallel->addAnimation(a1_p_o1);
    parallel->addAnimation(a1_p_o2);
    parallel->addAnimation(a1_p_o3);

    QAbstractAnimation *notTimeDriven = new UncontrolledAnimation(&t_o1, "value");
    QCOMPARE(notTimeDriven->totalDuration(), -1);

    QAbstractAnimation *loopsForever = new QPropertyAnimation(&t_o2, "value");
    loopsForever->setLoopCount(-1);
    QCOMPARE(loopsForever->totalDuration(), -1);

    QParallelAnimationGroup group;
    group.addAnimation(sequence);
    group.addAnimation(sequence2);
    group.addAnimation(parallel);
    group.addAnimation(notTimeDriven);
    group.addAnimation(loopsForever);

    // Current time = 1
    group.setCurrentTime(1);
    QCOMPARE(group.state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(sequence2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_s_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(parallel->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_p_o1->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_p_o2->state(), QAnimationGroup::Stopped);
    QCOMPARE(a1_p_o3->state(), QAnimationGroup::Stopped);
    QCOMPARE(notTimeDriven->state(), QAnimationGroup::Stopped);
    QCOMPARE(loopsForever->state(), QAnimationGroup::Stopped);

    QCOMPARE(group.currentLoopTime(), 1);
    QCOMPARE(sequence->currentLoopTime(), 1);
    QCOMPARE(a1_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 1);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);
    QCOMPARE(a1_p_o1->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 1);
    QCOMPARE(notTimeDriven->currentLoopTime(), 1);
    QCOMPARE(loopsForever->currentLoopTime(), 1);

    // Current time = 250
    group.setCurrentTime(250);
    QCOMPARE(group.currentLoopTime(), 250);
    QCOMPARE(sequence->currentLoopTime(), 250);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 0);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 0);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 250);
    QCOMPARE(loopsForever->currentLoopTime(), 0);
    QCOMPARE(loopsForever->currentLoop(), 1);
    QCOMPARE(sequence->currentAnimation(), a2_s_o1);

    // Current time = 251
    group.setCurrentTime(251);
    QCOMPARE(group.currentLoopTime(), 251);
    QCOMPARE(sequence->currentLoopTime(), 251);
    QCOMPARE(a1_s_o1->currentLoopTime(), 250);
    QCOMPARE(a2_s_o1->currentLoopTime(), 1);
    QCOMPARE(a2_s_o1->currentLoop(), 0);
    QCOMPARE(a3_s_o1->currentLoopTime(), 0);
    QCOMPARE(sequence2->currentLoopTime(), 251);
    QCOMPARE(a1_s_o2->currentLoopTime(), 250);
    QCOMPARE(a1_s_o3->currentLoopTime(), 1);
    QCOMPARE(a1_p_o1->currentLoopTime(), 250);
    QCOMPARE(a1_p_o2->currentLoopTime(), 1);
    QCOMPARE(a1_p_o2->currentLoop(), 1);
    QCOMPARE(a1_p_o3->currentLoopTime(), 250);
    QCOMPARE(notTimeDriven->currentLoopTime(), 251);
    QCOMPARE(loopsForever->currentLoopTime(), 1);
    QCOMPARE(sequence->currentAnimation(), a2_s_o1);
}

void tst_QAnimationGroup::setParentAutoAdd()
{
    QParallelAnimationGroup group;
    QVariantAnimation *animation = new QPropertyAnimation(&group);
    QCOMPARE(animation->group(), static_cast<QAnimationGroup*>(&group));
}

void tst_QAnimationGroup::beginNestedGroup()
{
    QAnimationGroup *parent = new QParallelAnimationGroup();

    for (int i = 0; i < 10; ++i) {
        if (i & 1) {
            new QParallelAnimationGroup(parent);
        } else {
            new QSequentialAnimationGroup(parent);
        }

        QCOMPARE(parent->animationCount(), 1);
        QAnimationGroup *child = static_cast<QAnimationGroup *>(parent->animationAt(0));

        QCOMPARE(child->parent(), static_cast<QObject *>(parent));
        if (i & 1)
            QVERIFY(qobject_cast<QParallelAnimationGroup *> (child));
        else
            QVERIFY(qobject_cast<QSequentialAnimationGroup *> (child));

        parent = child;
    }
}

void tst_QAnimationGroup::addChildTwice()
{
    QAbstractAnimation *subGroup;
    QAbstractAnimation *subGroup2;
    QAnimationGroup *parent = new QSequentialAnimationGroup();

    subGroup = new QPropertyAnimation();
    subGroup->setParent(parent);
    parent->addAnimation(subGroup);
    QCOMPARE(parent->animationCount(), 1);

    parent->clear();

    QCOMPARE(parent->animationCount(), 0);

    // adding the same item twice to a group will remove the item from its current position
    // and append it to the end
    subGroup = new QPropertyAnimation(parent);
    subGroup2 = new QPropertyAnimation(parent);

    QCOMPARE(parent->animationCount(), 2);
    QCOMPARE(parent->animationAt(0), subGroup);
    QCOMPARE(parent->animationAt(1), subGroup2);

    parent->addAnimation(subGroup);

    QCOMPARE(parent->animationCount(), 2);
    QCOMPARE(parent->animationAt(0), subGroup2);
    QCOMPARE(parent->animationAt(1), subGroup);

    delete parent;
}

void tst_QAnimationGroup::loopWithoutStartValue()
{
    QAnimationGroup *parent = new QSequentialAnimationGroup();
    QObject o;
    o.setProperty("ole", 0);
    QCOMPARE(o.property("ole").toInt(), 0);

    QPropertyAnimation anim1(&o, "ole");
    anim1.setEndValue(-50);
    anim1.setDuration(100);

    QPropertyAnimation anim2(&o, "ole");
    anim2.setEndValue(50);
    anim2.setDuration(100);

    parent->addAnimation(&anim1);
    parent->addAnimation(&anim2);

    parent->setLoopCount(-1);
    parent->start();

    QVERIFY(anim1.startValue().isNull());
    QCOMPARE(anim1.currentValue().toInt(), 0);
    QCOMPARE(parent->currentLoop(), 0);

    parent->setCurrentTime(200);
    QCOMPARE(parent->currentLoop(), 1);
    QCOMPARE(anim1.currentValue().toInt(), 50);
    parent->stop();
}

QTEST_MAIN(tst_QAnimationGroup)
#include "tst_qanimationgroup.moc"
