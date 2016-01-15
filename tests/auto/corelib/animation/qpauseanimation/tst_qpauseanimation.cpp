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

#include <QtCore/qpauseanimation.h>
#include <QtCore/qpropertyanimation.h>
#include <QtCore/qsequentialanimationgroup.h>

#include <private/qabstractanimation_p.h>

#if defined(Q_OS_WIN) || defined(Q_OS_ANDROID)
#  define BAD_TIMER_RESOLUTION
#endif

#ifdef BAD_TIMER_RESOLUTION
static const char timerError[] = "On this platform, consistent timing is not working properly due to bad timer resolution";
#endif

class TestablePauseAnimation : public QPauseAnimation
{
    Q_OBJECT
public:
    TestablePauseAnimation(QObject *parent = 0)
        : QPauseAnimation(parent),
        m_updateCurrentTimeCount(0)
    {
    }

    int m_updateCurrentTimeCount;
protected:
    void updateCurrentTime(int currentTime)
    {
        QPauseAnimation::updateCurrentTime(currentTime);
        ++m_updateCurrentTimeCount;
    }
};

class EnableConsistentTiming
{
public:
    EnableConsistentTiming()
    {
        QUnifiedTimer *timer = QUnifiedTimer::instance();
        timer->setConsistentTiming(true);
    }
    ~EnableConsistentTiming()
    {
        QUnifiedTimer *timer = QUnifiedTimer::instance();
        timer->setConsistentTiming(false);
    }
};

class tst_QPauseAnimation : public QObject
{
  Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void changeDirectionWhileRunning();
    void noTimerUpdates_data();
    void noTimerUpdates();
    void multiplePauseAnimations();
    void pauseAndPropertyAnimations();
    void pauseResume();
    void sequentialPauseGroup();
    void sequentialGroupWithPause();
    void multipleSequentialGroups();
    void zeroDuration();
};

void tst_QPauseAnimation::initTestCase()
{
    qRegisterMetaType<QAbstractAnimation::State>("QAbstractAnimation::State");
    qRegisterMetaType<QAbstractAnimation::DeletionPolicy>("QAbstractAnimation::DeletionPolicy");
}

void tst_QPauseAnimation::changeDirectionWhileRunning()
{
    EnableConsistentTiming enabled;

    TestablePauseAnimation animation;
    animation.setDuration(400);
    animation.start();
    QTest::qWait(100);
    QCOMPARE(animation.state(), QAbstractAnimation::Running);
    animation.setDirection(QAbstractAnimation::Backward);
    QTest::qWait(animation.totalDuration() + 50);
    QCOMPARE(animation.state(), QAbstractAnimation::Stopped);
}

void tst_QPauseAnimation::noTimerUpdates_data()
{
    QTest::addColumn<int>("duration");
    QTest::addColumn<int>("loopCount");

    QTest::newRow("0") << 200 << 1;
    QTest::newRow("1") << 160 << 1;
    QTest::newRow("2") << 160 << 2;
    QTest::newRow("3") << 200 << 3;
}

void tst_QPauseAnimation::noTimerUpdates()
{
    EnableConsistentTiming enabled;

    QFETCH(int, duration);
    QFETCH(int, loopCount);

    TestablePauseAnimation animation;
    animation.setDuration(duration);
    animation.setLoopCount(loopCount);
    animation.start();
    QTest::qWait(animation.totalDuration() + 100);

#ifdef BAD_TIMER_RESOLUTION
    if (animation.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", timerError, Abort);
#endif

    QCOMPARE(animation.state(), QAbstractAnimation::Stopped);
    const int expectedLoopCount = 1 + loopCount;

#ifdef BAD_TIMER_RESOLUTION
    if (animation.m_updateCurrentTimeCount != expectedLoopCount)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(animation.m_updateCurrentTimeCount, expectedLoopCount);
}

void tst_QPauseAnimation::multiplePauseAnimations()
{
    EnableConsistentTiming enabled;

    TestablePauseAnimation animation;
    animation.setDuration(200);

    TestablePauseAnimation animation2;
    animation2.setDuration(800);

    animation.start();
    animation2.start();
    QTest::qWait(animation.totalDuration() + 100);

#ifdef BAD_TIMER_RESOLUTION
    if (animation.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(animation.state(), QAbstractAnimation::Stopped);

#ifdef BAD_TIMER_RESOLUTION
    if (animation2.state() != QAbstractAnimation::Running)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(animation2.state(), QAbstractAnimation::Running);

#ifdef BAD_TIMER_RESOLUTION
    if (animation.m_updateCurrentTimeCount != 2)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(animation.m_updateCurrentTimeCount, 2);

#ifdef BAD_TIMER_RESOLUTION
    if (animation2.m_updateCurrentTimeCount != 2)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(animation2.m_updateCurrentTimeCount, 2);

    QTest::qWait(550);

#ifdef BAD_TIMER_RESOLUTION
    if (animation2.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(animation2.state(), QAbstractAnimation::Stopped);

#ifdef BAD_TIMER_RESOLUTION
    if (animation2.m_updateCurrentTimeCount != 3)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(animation2.m_updateCurrentTimeCount, 3);
}

void tst_QPauseAnimation::pauseAndPropertyAnimations()
{
    EnableConsistentTiming enabled;

    TestablePauseAnimation pause;
    pause.setDuration(200);

    QObject o;
    o.setProperty("ole", 42);

    QPropertyAnimation animation(&o, "ole");
    animation.setEndValue(43);

    pause.start();

    QTest::qWait(100);
    animation.start();

    QCOMPARE(animation.state(), QAbstractAnimation::Running);
    QCOMPARE(pause.state(), QAbstractAnimation::Running);
    QCOMPARE(pause.m_updateCurrentTimeCount, 2);

    QTest::qWait(animation.totalDuration() + 100);

#ifdef BAD_TIMER_RESOLUTION
    if (animation.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(animation.state(), QAbstractAnimation::Stopped);
    QCOMPARE(pause.state(), QAbstractAnimation::Stopped);
    QVERIFY(pause.m_updateCurrentTimeCount > 3);
}

void tst_QPauseAnimation::pauseResume()
{
    TestablePauseAnimation animation;
    animation.setDuration(400);
    animation.start();
    QCOMPARE(animation.state(), QAbstractAnimation::Running);
    QTest::qWait(200);
    animation.pause();
    QCOMPARE(animation.state(), QAbstractAnimation::Paused);
    animation.start();
    QTRY_COMPARE(animation.state(), QAbstractAnimation::Stopped);

#ifdef BAD_TIMER_RESOLUTION
    if (animation.m_updateCurrentTimeCount < 3)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QVERIFY2(animation.m_updateCurrentTimeCount >= 3, qPrintable(
        QString::fromLatin1("animation.m_updateCurrentTimeCount = %1").arg(animation.m_updateCurrentTimeCount)));
}

void tst_QPauseAnimation::sequentialPauseGroup()
{
    QSequentialAnimationGroup group;

    TestablePauseAnimation animation1(&group);
    animation1.setDuration(200);
    TestablePauseAnimation animation2(&group);
    animation2.setDuration(200);
    TestablePauseAnimation animation3(&group);
    animation3.setDuration(200);

    group.start();
    QCOMPARE(animation1.m_updateCurrentTimeCount, 1);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 0);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 0);

    QCOMPARE(group.state(), QAbstractAnimation::Running);
    QCOMPARE(animation1.state(), QAbstractAnimation::Running);
    QCOMPARE(animation2.state(), QAbstractAnimation::Stopped);
    QCOMPARE(animation3.state(), QAbstractAnimation::Stopped);

    group.setCurrentTime(250);
    QCOMPARE(animation1.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 1);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 0);

    QCOMPARE(group.state(), QAbstractAnimation::Running);
    QCOMPARE(animation1.state(), QAbstractAnimation::Stopped);
    QCOMPARE((QAbstractAnimation*)&animation2, group.currentAnimation());
    QCOMPARE(animation2.state(), QAbstractAnimation::Running);
    QCOMPARE(animation3.state(), QAbstractAnimation::Stopped);

    group.setCurrentTime(500);
    QCOMPARE(animation1.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 1);

    QCOMPARE(group.state(), QAbstractAnimation::Running);
    QCOMPARE(animation1.state(), QAbstractAnimation::Stopped);
    QCOMPARE(animation2.state(), QAbstractAnimation::Stopped);
    QCOMPARE((QAbstractAnimation*)&animation3, group.currentAnimation());
    QCOMPARE(animation3.state(), QAbstractAnimation::Running);

    group.setCurrentTime(750);

    QCOMPARE(group.state(), QAbstractAnimation::Stopped);
    QCOMPARE(animation1.state(), QAbstractAnimation::Stopped);
    QCOMPARE(animation2.state(), QAbstractAnimation::Stopped);
    QCOMPARE(animation3.state(), QAbstractAnimation::Stopped);

    QCOMPARE(animation1.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 2);
}

void tst_QPauseAnimation::sequentialGroupWithPause()
{
    QSequentialAnimationGroup group;

    QObject o;
    o.setProperty("ole", 42);

    QPropertyAnimation animation(&o, "ole", &group);
    animation.setEndValue(43);
    TestablePauseAnimation pause(&group);
    pause.setDuration(250);

    group.start();

    QCOMPARE(group.state(), QAbstractAnimation::Running);
    QCOMPARE(animation.state(), QAbstractAnimation::Running);
    QCOMPARE(pause.state(), QAbstractAnimation::Stopped);

    group.setCurrentTime(300);

    QCOMPARE(group.state(), QAbstractAnimation::Running);
    QCOMPARE(animation.state(), QAbstractAnimation::Stopped);
    QCOMPARE((QAbstractAnimation*)&pause, group.currentAnimation());
    QCOMPARE(pause.state(), QAbstractAnimation::Running);

    group.setCurrentTime(600);

    QCOMPARE(group.state(), QAbstractAnimation::Stopped);
    QCOMPARE(animation.state(), QAbstractAnimation::Stopped);
    QCOMPARE(pause.state(), QAbstractAnimation::Stopped);

    QCOMPARE(pause.m_updateCurrentTimeCount, 2);
}

void tst_QPauseAnimation::multipleSequentialGroups()
{
    EnableConsistentTiming enabled;

    QParallelAnimationGroup group;
    group.setLoopCount(2);

    QSequentialAnimationGroup subgroup1(&group);

    QObject o;
    o.setProperty("ole", 42);

    QPropertyAnimation animation(&o, "ole", &subgroup1);
    animation.setEndValue(43);
    animation.setDuration(300);
    TestablePauseAnimation pause(&subgroup1);
    pause.setDuration(200);

    QSequentialAnimationGroup subgroup2(&group);

    o.setProperty("ole2", 42);
    QPropertyAnimation animation2(&o, "ole2", &subgroup2);
    animation2.setEndValue(43);
    animation2.setDuration(200);
    TestablePauseAnimation pause2(&subgroup2);
    pause2.setDuration(250);

    QSequentialAnimationGroup subgroup3(&group);

    TestablePauseAnimation pause3(&subgroup3);
    pause3.setDuration(400);

    o.setProperty("ole3", 42);
    QPropertyAnimation animation3(&o, "ole3", &subgroup3);
    animation3.setEndValue(43);
    animation3.setDuration(200);

    QSequentialAnimationGroup subgroup4(&group);

    TestablePauseAnimation pause4(&subgroup4);
    pause4.setDuration(310);

    TestablePauseAnimation pause5(&subgroup4);
    pause5.setDuration(60);

    group.start();

    QCOMPARE(group.state(), QAbstractAnimation::Running);
    QCOMPARE(subgroup1.state(), QAbstractAnimation::Running);
    QCOMPARE(subgroup2.state(), QAbstractAnimation::Running);
    QCOMPARE(subgroup3.state(), QAbstractAnimation::Running);
    QCOMPARE(subgroup4.state(), QAbstractAnimation::Running);

    // This is a pretty long animation so it tends to get rather out of sync
    // when using the consistent timer, so run for an extra half second for good
    // measure...
    QTest::qWait(group.totalDuration() + 500);

#ifdef BAD_TIMER_RESOLUTION
    if (group.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(group.state(), QAbstractAnimation::Stopped);

#ifdef BAD_TIMER_RESOLUTION
    if (subgroup1.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(subgroup1.state(), QAbstractAnimation::Stopped);

#ifdef BAD_TIMER_RESOLUTION
    if (subgroup2.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(subgroup2.state(), QAbstractAnimation::Stopped);

#ifdef BAD_TIMER_RESOLUTION
    if (subgroup3.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(subgroup3.state(), QAbstractAnimation::Stopped);

#ifdef BAD_TIMER_RESOLUTION
    if (subgroup4.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(subgroup4.state(), QAbstractAnimation::Stopped);

#ifdef BAD_TIMER_RESOLUTION
    if (pause5.m_updateCurrentTimeCount != 4)
        QEXPECT_FAIL("", timerError, Abort);
#endif
    QCOMPARE(pause5.m_updateCurrentTimeCount, 4);
}

void tst_QPauseAnimation::zeroDuration()
{
    TestablePauseAnimation animation;
    animation.setDuration(0);
    animation.start();
    QTest::qWait(animation.totalDuration() + 100);
    QCOMPARE(animation.state(), QAbstractAnimation::Stopped);
    QCOMPARE(animation.m_updateCurrentTimeCount, 1);
}

QTEST_MAIN(tst_QPauseAnimation)
#include "tst_qpauseanimation.moc"
