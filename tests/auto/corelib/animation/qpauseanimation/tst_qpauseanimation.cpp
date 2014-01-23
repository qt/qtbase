/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>

#include <QtCore/qpauseanimation.h>
#include <QtCore/qpropertyanimation.h>
#include <QtCore/qsequentialanimationgroup.h>

#include <private/qabstractanimation_p.h>

#ifdef Q_OS_WIN
static const char winTimerError[] = "On windows, consistent timing is not working properly due to bad timer resolution";
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
    QVERIFY(animation.state() == QAbstractAnimation::Running);
    animation.setDirection(QAbstractAnimation::Backward);
    QTest::qWait(animation.totalDuration() + 50);
    QVERIFY(animation.state() == QAbstractAnimation::Stopped);
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

#ifdef Q_OS_WIN
    if (animation.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif

    QVERIFY(animation.state() == QAbstractAnimation::Stopped);
    const int expectedLoopCount = 1 + loopCount;

#ifdef Q_OS_WIN
    if (animation.m_updateCurrentTimeCount != expectedLoopCount)
        QEXPECT_FAIL("", winTimerError, Abort);
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

#ifdef Q_OS_WIN
    if (animation.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(animation.state() == QAbstractAnimation::Stopped);

#ifdef Q_OS_WIN
    if (animation2.state() != QAbstractAnimation::Running)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(animation2.state() == QAbstractAnimation::Running);

#ifdef Q_OS_WIN
    if (animation.m_updateCurrentTimeCount != 2)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QCOMPARE(animation.m_updateCurrentTimeCount, 2);

#ifdef Q_OS_WIN
    if (animation2.m_updateCurrentTimeCount != 2)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QCOMPARE(animation2.m_updateCurrentTimeCount, 2);

    QTest::qWait(550);

#ifdef Q_OS_WIN
    if (animation2.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(animation2.state() == QAbstractAnimation::Stopped);

#ifdef Q_OS_WIN
    if (animation2.m_updateCurrentTimeCount != 3)
        QEXPECT_FAIL("", winTimerError, Abort);
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

    QVERIFY(animation.state() == QAbstractAnimation::Running);
    QVERIFY(pause.state() == QAbstractAnimation::Running);
    QCOMPARE(pause.m_updateCurrentTimeCount, 2);

    QTest::qWait(animation.totalDuration() + 100);

#ifdef Q_OS_WIN
    if (animation.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(animation.state() == QAbstractAnimation::Stopped);
    QVERIFY(pause.state() == QAbstractAnimation::Stopped);
    QVERIFY(pause.m_updateCurrentTimeCount > 3);
}

void tst_QPauseAnimation::pauseResume()
{
    TestablePauseAnimation animation;
    animation.setDuration(400);
    animation.start();
    QVERIFY(animation.state() == QAbstractAnimation::Running);
    QTest::qWait(200);
    animation.pause();
    QVERIFY(animation.state() == QAbstractAnimation::Paused);
    animation.start();
    QTRY_COMPARE(animation.state(), QAbstractAnimation::Stopped);

#ifdef Q_OS_WIN
    if (animation.m_updateCurrentTimeCount < 3)
        QEXPECT_FAIL("", winTimerError, Abort);
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

    QVERIFY(group.state() == QAbstractAnimation::Running);
    QVERIFY(animation1.state() == QAbstractAnimation::Running);
    QVERIFY(animation2.state() == QAbstractAnimation::Stopped);
    QVERIFY(animation3.state() == QAbstractAnimation::Stopped);

    group.setCurrentTime(250);
    QCOMPARE(animation1.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 1);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 0);

    QVERIFY(group.state() == QAbstractAnimation::Running);
    QVERIFY(animation1.state() == QAbstractAnimation::Stopped);
    QCOMPARE((QAbstractAnimation*)&animation2, group.currentAnimation());
    QVERIFY(animation2.state() == QAbstractAnimation::Running);
    QVERIFY(animation3.state() == QAbstractAnimation::Stopped);

    group.setCurrentTime(500);
    QCOMPARE(animation1.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation2.m_updateCurrentTimeCount, 2);
    QCOMPARE(animation3.m_updateCurrentTimeCount, 1);

    QVERIFY(group.state() == QAbstractAnimation::Running);
    QVERIFY(animation1.state() == QAbstractAnimation::Stopped);
    QVERIFY(animation2.state() == QAbstractAnimation::Stopped);
    QCOMPARE((QAbstractAnimation*)&animation3, group.currentAnimation());
    QVERIFY(animation3.state() == QAbstractAnimation::Running);

    group.setCurrentTime(750);

    QVERIFY(group.state() == QAbstractAnimation::Stopped);
    QVERIFY(animation1.state() == QAbstractAnimation::Stopped);
    QVERIFY(animation2.state() == QAbstractAnimation::Stopped);
    QVERIFY(animation3.state() == QAbstractAnimation::Stopped);

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

    QVERIFY(group.state() == QAbstractAnimation::Running);
    QVERIFY(animation.state() == QAbstractAnimation::Running);
    QVERIFY(pause.state() == QAbstractAnimation::Stopped);

    group.setCurrentTime(300);

    QVERIFY(group.state() == QAbstractAnimation::Running);
    QVERIFY(animation.state() == QAbstractAnimation::Stopped);
    QCOMPARE((QAbstractAnimation*)&pause, group.currentAnimation());
    QVERIFY(pause.state() == QAbstractAnimation::Running);

    group.setCurrentTime(600);

    QVERIFY(group.state() == QAbstractAnimation::Stopped);
    QVERIFY(animation.state() == QAbstractAnimation::Stopped);
    QVERIFY(pause.state() == QAbstractAnimation::Stopped);

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

    QVERIFY(group.state() == QAbstractAnimation::Running);
    QVERIFY(subgroup1.state() == QAbstractAnimation::Running);
    QVERIFY(subgroup2.state() == QAbstractAnimation::Running);
    QVERIFY(subgroup3.state() == QAbstractAnimation::Running);
    QVERIFY(subgroup4.state() == QAbstractAnimation::Running);

    // This is a pretty long animation so it tends to get rather out of sync
    // when using the consistent timer, so run for an extra half second for good
    // measure...
    QTest::qWait(group.totalDuration() + 500);

#ifdef Q_OS_WIN
    if (group.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(group.state() == QAbstractAnimation::Stopped);

#ifdef Q_OS_WIN
    if (subgroup1.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(subgroup1.state() == QAbstractAnimation::Stopped);

#ifdef Q_OS_WIN
    if (subgroup2.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(subgroup2.state() == QAbstractAnimation::Stopped);

#ifdef Q_OS_WIN
    if (subgroup3.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(subgroup3.state() == QAbstractAnimation::Stopped);

#ifdef Q_OS_WIN
    if (subgroup4.state() != QAbstractAnimation::Stopped)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QVERIFY(subgroup4.state() == QAbstractAnimation::Stopped);

#ifdef Q_OS_WIN
    if (pause5.m_updateCurrentTimeCount != 4)
        QEXPECT_FAIL("", winTimerError, Abort);
#endif
    QCOMPARE(pause5.m_updateCurrentTimeCount, 4);
}

void tst_QPauseAnimation::zeroDuration()
{
    TestablePauseAnimation animation;
    animation.setDuration(0);
    animation.start();
    QTest::qWait(animation.totalDuration() + 100);
    QVERIFY(animation.state() == QAbstractAnimation::Stopped);
    QCOMPARE(animation.m_updateCurrentTimeCount, 1);
}

QTEST_MAIN(tst_QPauseAnimation)
#include "tst_qpauseanimation.moc"
