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

#include <qtimeline.h>

class tst_QTimeLine : public QObject
{
  Q_OBJECT
private slots:
    void range();
    void currentTime();
    void duration();
    void frameRate();
    void value();
    void currentFrame();
    void loopCount();
    void interpolation();
    void reverse_data();
    void reverse();
    void toggleDirection();
    void frameChanged();
    void stopped();
    void finished();
    void isRunning();
    void multipleTimeLines();
    void sineCurve();
    void cosineCurve();
    void outOfRange();
    void stateInFinishedSignal();
    void resume();
    void restart();
    void setPaused();

protected slots:
    void finishedSlot();

protected:
    QTimeLine::State state;
    QTimeLine * view;
};

void tst_QTimeLine::range()
{
    QTimeLine timeLine(200);
    QCOMPARE(timeLine.startFrame(), 0);
    QCOMPARE(timeLine.endFrame(), 0);
    timeLine.setFrameRange(0, 1);
    QCOMPARE(timeLine.startFrame(), 0);
    QCOMPARE(timeLine.endFrame(), 1);
    timeLine.setFrameRange(10, 20);
    QCOMPARE(timeLine.startFrame(), 10);
    QCOMPARE(timeLine.endFrame(), 20);

    timeLine.setStartFrame(6);
    QCOMPARE(timeLine.startFrame(), 6);
    timeLine.setEndFrame(16);
    QCOMPARE(timeLine.endFrame(), 16);

    // Verify that you can change the range in the timeLine
    timeLine.setFrameRange(1000, 2000);
    QSignalSpy spy(&timeLine, &QTimeLine::frameChanged);
    QVERIFY(spy.isValid());
    timeLine.start();  // make sure that the logic works for a running timeline
    QTRY_COMPARE(timeLine.state(), QTimeLine::Running);
    timeLine.setCurrentTime(timeLine.duration()/2);
    int oldValue = timeLine.currentFrame();
    timeLine.setFrameRange(0, 500);
    QVERIFY(timeLine.currentFrame() < oldValue);
    timeLine.setEndFrame(10000);
    timeLine.setStartFrame(5000);
    QVERIFY(timeLine.currentFrame() > oldValue);
    timeLine.setFrameRange(0, 500);
    QTRY_VERIFY(spy.count() > 1);
    QVERIFY(timeLine.currentFrame() < oldValue);
}

void tst_QTimeLine::currentTime()
{
    QTimeLine timeLine(2000);
    timeLine.setUpdateInterval((timeLine.duration()/2) / 33);
    timeLine.setFrameRange(10, 20);
    QCOMPARE(timeLine.currentTime(), 0);
    timeLine.start();
    QTRY_COMPARE(timeLine.state(), QTimeLine::Running);
    QTRY_VERIFY(timeLine.currentTime() > timeLine.duration()/2 - timeLine.duration()/4);
    QVERIFY(timeLine.currentTime() < timeLine.duration()/2 + timeLine.duration()/4);
    QTRY_COMPARE(timeLine.state(), QTimeLine::NotRunning);
    QCOMPARE(timeLine.currentTime(), timeLine.duration());

    QSignalSpy spy(&timeLine, &QTimeLine::valueChanged);
    QVERIFY(spy.isValid());
    spy.clear();
    timeLine.setCurrentTime(timeLine.duration()/2);
    timeLine.setCurrentTime(timeLine.duration()/2);
    QCOMPARE(spy.count(), 1);
    spy.clear();
    QCOMPARE(timeLine.currentTime(), timeLine.duration()/2);
    timeLine.resume();
    // Let it update on its own
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    QTRY_VERIFY(timeLine.currentTime() > timeLine.duration()/2);
    QVERIFY(timeLine.currentTime() < timeLine.duration());
    QTRY_COMPARE(timeLine.state(), QTimeLine::NotRunning);
    QCOMPARE(timeLine.currentTime(), timeLine.duration());

    // Reverse should decrease the currentTime
    timeLine.setCurrentTime(timeLine.duration()/2);
    timeLine.start();
    // Let it update on its own
    int currentTime = timeLine.currentTime();
    QTRY_VERIFY(timeLine.currentTime() > currentTime);
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    currentTime = timeLine.currentTime();
    timeLine.setDirection(QTimeLine::Backward);
    QTRY_VERIFY(timeLine.currentTime() < currentTime);
    timeLine.stop();
}

void tst_QTimeLine::duration()
{
    QTimeLine timeLine(200);
    timeLine.setFrameRange(10, 20);
    QCOMPARE(timeLine.duration(), 200);
    timeLine.setDuration(1000);
    QCOMPARE(timeLine.duration(), 1000);

    timeLine.start();
    QTRY_COMPARE(timeLine.state(), QTimeLine::Running);
    QTRY_VERIFY(timeLine.currentTime() > 0);
    QTRY_COMPARE(timeLine.state(), QTimeLine::NotRunning);
    QCOMPARE(timeLine.currentTime(), 1000);
    // The duration shouldn't change
    QCOMPARE(timeLine.duration(), 1000);
}

void tst_QTimeLine::frameRate()
{
    QTimeLine timeLine;
    timeLine.setFrameRange(100, 2000);
    QCOMPARE(timeLine.updateInterval(), 1000 / 25);
    timeLine.setUpdateInterval(1000 / 60);
    QCOMPARE(timeLine.updateInterval(), 1000 / 60);

    // Default speed
    timeLine.setUpdateInterval(1000 / 33);
    QSignalSpy spy(&timeLine, &QTimeLine::frameChanged);
    QVERIFY(spy.isValid());
    timeLine.start();
    QTest::qWait(timeLine.duration()*2);
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);
    int slowCount = spy.count();

    // Faster!!
    timeLine.setUpdateInterval(1000 / 100);
    spy.clear();
    timeLine.setCurrentTime(0);
    timeLine.start();
    QTest::qWait(timeLine.duration()*2);
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);
    QVERIFY2(slowCount < spy.count(), QByteArray::number(spy.count()));
}

void tst_QTimeLine::value()
{
    QTimeLine timeLine(5000);
    QCOMPARE(timeLine.currentValue(), 0.0);

    // Default speed
    QSignalSpy spy(&timeLine, &QTimeLine::valueChanged);
    QVERIFY(spy.isValid());
    timeLine.start();
    QTRY_VERIFY(timeLine.currentValue() > 0);
    QTRY_COMPARE(timeLine.state(), QTimeLine::NotRunning);
    QCOMPARE(timeLine.currentValue(), 1.0);
    QVERIFY(spy.count() > 0);

    // Reverse should decrease the value
    timeLine.setCurrentTime(100);
    timeLine.start();
    // Let it update on its own
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    QTRY_VERIFY(timeLine.currentValue());
    qreal value = timeLine.currentValue();
    timeLine.setDirection(QTimeLine::Backward);
    QTRY_VERIFY(timeLine.currentValue() < value);
    timeLine.stop();
}

void tst_QTimeLine::currentFrame()
{
    QTimeLine timeLine(2000);
    timeLine.setFrameRange(10, 20);
    QCOMPARE(timeLine.currentFrame(), 10);

    // Default speed
    QSignalSpy spy(&timeLine, &QTimeLine::frameChanged);
    QVERIFY(spy.isValid());
    timeLine.start();
    QTRY_VERIFY(timeLine.currentFrame() > 10);
    QTRY_COMPARE(timeLine.state(), QTimeLine::NotRunning);
    QCOMPARE(timeLine.currentFrame(), 20);

    // Reverse should decrease the value
    timeLine.setCurrentTime(timeLine.duration()/2);
    timeLine.start();
    // Let it update on its own
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    QTRY_VERIFY(timeLine.currentTime() > timeLine.duration()/2); // wait for continuation
    int value = timeLine.currentFrame();
    timeLine.setDirection(QTimeLine::Backward);
    QTRY_VERIFY(timeLine.currentFrame() < value);
    timeLine.stop();
}

void tst_QTimeLine::loopCount()
{
    QTimeLine timeLine(200);
    QCOMPARE(timeLine.loopCount(), 1);
    timeLine.setFrameRange(10, 20);
    QCOMPARE(timeLine.loopCount(), 1);
    timeLine.setLoopCount(0);
    QCOMPARE(timeLine.loopCount(), 0);

    // Default speed infiniti looping
    QSignalSpy spy(&timeLine, &QTimeLine::frameChanged);
    QVERIFY(spy.isValid());
    timeLine.start();
    QTest::qWait(timeLine.duration());
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    //QCOMPARE(timeLine.currentFrame(), 20);
    QTest::qWait(timeLine.duration()*6);
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    QVERIFY(timeLine.currentTime() >= 0);
    QVERIFY(timeLine.currentFrame() >= 10);
    QVERIFY(timeLine.currentFrame() <= 20);
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    timeLine.stop();

    timeLine.setDuration(2500); // ### some platforms have a very low resolution timer
    timeLine.setFrameRange(0, 2);
    timeLine.setLoopCount(4);

    QSignalSpy finishedSpy(&timeLine, &QTimeLine::finished);
    QSignalSpy frameChangedSpy(&timeLine, &QTimeLine::frameChanged);
    QVERIFY(finishedSpy.isValid());
    QVERIFY(frameChangedSpy.isValid());
    QEventLoop loop;
    connect(&timeLine, SIGNAL(finished()), &loop, SLOT(quit()));


    for(int i=0;i<2;i++) {

        timeLine.start();
        //we clear te list after the start so we don't catch
        //a frameChanged signal for the frame 0 at the beginning
        finishedSpy.clear();
        frameChangedSpy.clear();

        loop.exec();

        QCOMPARE(finishedSpy.count(), 1);
        QCOMPARE(frameChangedSpy.count(), 11);
        for (int i = 0; i < 11; ++i) {
            QCOMPARE(frameChangedSpy.at(i).at(0).toInt(), (i+1) % 3);
        }
    }

    timeLine.setDirection(QTimeLine::Backward);
    timeLine.start();
    loop.exec();

    QCOMPARE(finishedSpy.count(), 2);
    QCOMPARE(frameChangedSpy.count(), 22);
    for (int i = 11; i < 22; ++i) {
        QCOMPARE(frameChangedSpy.at(i).at(0).toInt(), 2 - (i+2) % 3);
    }
}

void tst_QTimeLine::interpolation()
{
    QTimeLine timeLine(400);
    QCOMPARE(timeLine.curveShape(), QTimeLine::EaseInOutCurve);
    timeLine.setFrameRange(100, 200);
    timeLine.setCurveShape(QTimeLine::LinearCurve);
    QCOMPARE(timeLine.curveShape(), QTimeLine::LinearCurve);

    // smooth
    timeLine.setCurveShape(QTimeLine::EaseInOutCurve);
    timeLine.start();
    QTest::qWait(100);
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    int firstValue = timeLine.currentFrame();
    QTest::qWait(200);
    int endValue = timeLine.currentFrame();
    timeLine.stop();
    timeLine.setCurrentTime(0);

    // linear
    timeLine.setCurveShape(QTimeLine::LinearCurve);
    timeLine.start();
    QTest::qWait(100);
    QCOMPARE(timeLine.state(), QTimeLine::Running);

    // Smooth accellerates slowly so in the beginning so it is farther behind
    if (firstValue >= timeLine.currentFrame())
        QEXPECT_FAIL("", "QTBUG-24796: QTimeLine exhibits inconsistent timing behaviour", Abort);
    QVERIFY(firstValue < timeLine.currentFrame());
    QTest::qWait(200);
    QVERIFY(endValue > timeLine.currentFrame());
    timeLine.stop();
}

void tst_QTimeLine::reverse_data()
{
    QTest::addColumn<int>("duration");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("end");
    QTest::addColumn<int>("direction");
    QTest::addColumn<int>("direction2");
    QTest::addColumn<int>("direction3");
    QTest::addColumn<int>("startTime");
    QTest::addColumn<int>("currentFrame");
    QTest::addColumn<qreal>("currentValue");
    QTest::addColumn<int>("wait");
    QTest::addColumn<int>("state");
    QTest::addColumn<int>("wait2");

    QTest::newRow("start at end") << 200 << 1000 << 2000 << (int)QTimeLine::Backward << (int)QTimeLine::Forward << (int)QTimeLine::Backward << 200 << 2000 << qreal(1.0) << 40 << (int)QTimeLine::Running << 140;
    QTest::newRow("start at half") << 200 << 1000 << 2000 << (int)QTimeLine::Backward << (int)QTimeLine::Forward << (int)QTimeLine::Backward << 100 << 1500 << qreal(0.5) << 40 << (int)QTimeLine::Running << 140;
    QTest::newRow("start at quarter") << 200 << 1000 << 2000 << (int)QTimeLine::Backward << (int)QTimeLine::Forward << (int)QTimeLine::Backward << 50 << 1250 << qreal(0.25) << 40 << (int)QTimeLine::Running << 140;
}

void tst_QTimeLine::reverse()
{
    QFETCH(int, duration);
    QFETCH(int, start);
    QFETCH(int, end);
    QFETCH(int, direction);
    QFETCH(int, direction2);
    QFETCH(int, direction3);
    QFETCH(int, startTime);
    QFETCH(int, currentFrame);
    QFETCH(qreal, currentValue);
    QFETCH(int, wait);
    QFETCH(int, state);
    QFETCH(int, wait2);

    QTimeLine timeLine(duration);
    timeLine.setCurveShape(QTimeLine::LinearCurve);
    timeLine.setFrameRange(start, end);

    timeLine.setDirection((QTimeLine::Direction)direction);
    timeLine.setDirection((QTimeLine::Direction)direction2);
    timeLine.setDirection((QTimeLine::Direction)direction3);
    QCOMPARE(timeLine.direction(), ((QTimeLine::Direction)direction));

    timeLine.setCurrentTime(startTime);
    timeLine.setDirection((QTimeLine::Direction)direction);
    timeLine.setDirection((QTimeLine::Direction)direction2);
    timeLine.setDirection((QTimeLine::Direction)direction3);

    QCOMPARE(timeLine.currentFrame(), currentFrame);
    QCOMPARE(timeLine.currentValue(), currentValue);
    timeLine.start();

    QTest::qWait(wait);
    QCOMPARE(timeLine.state(), (QTimeLine::State)state);
    int firstValue = timeLine.currentFrame();
    timeLine.setDirection((QTimeLine::Direction)direction2);
    timeLine.setDirection((QTimeLine::Direction)direction3);
    timeLine.setDirection((QTimeLine::Direction)direction2);
    timeLine.setDirection((QTimeLine::Direction)direction3);
    QTest::qWait(wait2);
    int endValue = timeLine.currentFrame();
    QVERIFY(endValue < firstValue);


}

void tst_QTimeLine::toggleDirection()
{
    // Note: enum values are cast to int so that QCOMPARE will show
    // the values if they don't match.
    QTimeLine timeLine;
    QCOMPARE(int(timeLine.direction()), int(QTimeLine::Forward));
    timeLine.toggleDirection();
    QCOMPARE(int(timeLine.direction()), int(QTimeLine::Backward));
    timeLine.toggleDirection();
    QCOMPARE(int(timeLine.direction()), int(QTimeLine::Forward));
}

void tst_QTimeLine::frameChanged()
{
    QTimeLine timeLine;
    timeLine.setCurveShape(QTimeLine::LinearCurve);
    timeLine.setFrameRange(0,9);
    timeLine.setUpdateInterval(800);
    QSignalSpy spy(&timeLine, &QTimeLine::frameChanged);
    QVERIFY(spy.isValid());

    // Test what happens when duration expires before all frames are emitted.
    timeLine.start();
    QTest::qWait(timeLine.duration()/2);
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    QCOMPARE(spy.count(), 0);
    QTest::qWait(timeLine.duration());
    if (timeLine.state() != QTimeLine::NotRunning)
        QEXPECT_FAIL("", "QTBUG-24796: QTimeLine runs slower than it should", Abort);
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);
    if (spy.count() != 1)
        QEXPECT_FAIL("", "QTBUG-24796: QTimeLine runs slower than it should", Abort);
    QCOMPARE(spy.count(), 1);

    // Test what happens when the frames are all emitted well before duration expires.
    timeLine.setUpdateInterval(5);
    spy.clear();
    timeLine.setCurrentTime(0);
    timeLine.start();
    QTest::qWait(timeLine.duration()*2);
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);
    QCOMPARE(spy.count(), 10);
}

void tst_QTimeLine::stopped()
{
    QTimeLine timeLine;
    timeLine.setFrameRange(0, 9);
    qRegisterMetaType<QTimeLine::State>("QTimeLine::State");
    QSignalSpy spy(&timeLine, &QTimeLine::stateChanged);
    QVERIFY(spy.isValid());
    timeLine.start();
    QTest::qWait(timeLine.duration()*2);
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);
    QCOMPARE(spy.count(), 2);
    spy.clear();
    timeLine.start();
    timeLine.stop();
    QCOMPARE(spy.count(), 2);
    timeLine.setDirection(QTimeLine::Backward);
    QCOMPARE(timeLine.loopCount(), 1);
}

void tst_QTimeLine::finished()
{
    QTimeLine timeLine;
    timeLine.setFrameRange(0,9);
    QSignalSpy spy(&timeLine, &QTimeLine::finished);
    QVERIFY(spy.isValid());
    timeLine.start();
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);

    spy.clear();
    timeLine.start();
    timeLine.stop();
    QCOMPARE(spy.count(), 0);
}

void tst_QTimeLine::isRunning()
{
    QTimeLine timeLine;
    timeLine.setFrameRange(0,9);
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);
    timeLine.start();
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    timeLine.stop();
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);

    timeLine.start();
    QTest::qWait(timeLine.duration()*2);
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);
}

void tst_QTimeLine::multipleTimeLines()
{
    // Stopping a timer shouldn't affect the other timers
    QTimeLine timeLine(200);
    timeLine.setFrameRange(0,99);
    QSignalSpy spy(&timeLine, &QTimeLine::finished);
    QVERIFY(spy.isValid());

    QTimeLine timeLineKiller;
    timeLineKiller.setFrameRange(0,99);

    timeLineKiller.start();
    timeLine.start();
    timeLineKiller.stop();
    QTest::qWait(timeLine.duration()*2);
    QCOMPARE(spy.count(), 1);
}

void tst_QTimeLine::sineCurve()
{
    QTimeLine timeLine(1000);
    timeLine.setCurveShape(QTimeLine::SineCurve);
    QCOMPARE(timeLine.valueForTime(0), qreal(0));
    QCOMPARE(timeLine.valueForTime(250), qreal(0.5));
    QCOMPARE(timeLine.valueForTime(500), qreal(1));
    QCOMPARE(timeLine.valueForTime(750), qreal(0.5));
    QCOMPARE(timeLine.valueForTime(1000), qreal(0));
}

void tst_QTimeLine::cosineCurve()
{
    QTimeLine timeLine(1000);
    timeLine.setCurveShape(QTimeLine::CosineCurve);
    QCOMPARE(timeLine.valueForTime(0), qreal(0.5));
    QCOMPARE(timeLine.valueForTime(250), qreal(1));
    QCOMPARE(timeLine.valueForTime(500), qreal(0.5));
    QCOMPARE(timeLine.valueForTime(750), qreal(0));
    QCOMPARE(timeLine.valueForTime(1000), qreal(0.5));
}

void tst_QTimeLine::outOfRange()
{
    QTimeLine timeLine(1000);
    QCOMPARE(timeLine.valueForTime(-100), qreal(0));
    QCOMPARE(timeLine.valueForTime(2000), qreal(1));

    timeLine.setCurveShape(QTimeLine::SineCurve);
    QCOMPARE(timeLine.valueForTime(2000), qreal(0));
}

void tst_QTimeLine::stateInFinishedSignal()
{
    QTimeLine timeLine(50);

    connect(&timeLine, SIGNAL(finished()), this, SLOT(finishedSlot()));
    state = QTimeLine::State(-1);

    timeLine.start();
    QTest::qWait(250);

    QCOMPARE(state, QTimeLine::NotRunning);
}

void tst_QTimeLine::finishedSlot()
{
    QTimeLine *timeLine = qobject_cast<QTimeLine *>(sender());
    if (timeLine)
        state = timeLine->state();
}

void tst_QTimeLine::resume()
{
    QTimeLine timeLine(1000);
    {
        QCOMPARE(timeLine.currentTime(), 0);
        timeLine.start();
        QTRY_VERIFY(timeLine.currentTime() > 0);
        timeLine.stop();
        int oldCurrentTime = timeLine.currentTime();
        QVERIFY(oldCurrentTime > 0);
        QVERIFY(oldCurrentTime < 1000);
        timeLine.resume();
        QTRY_VERIFY(timeLine.currentTime() > oldCurrentTime);
        timeLine.stop();
        int currentTime = timeLine.currentTime();
        QVERIFY(currentTime < 1000);
    }
    timeLine.setDirection(QTimeLine::Backward);
    {
        timeLine.setCurrentTime(1000);
        QCOMPARE(timeLine.currentTime(), 1000);
        timeLine.start();
        QTRY_VERIFY(timeLine.currentTime() < 1000);
        timeLine.stop();
        int oldCurrentTime = timeLine.currentTime();
        QVERIFY(oldCurrentTime < 1000);
        QVERIFY(oldCurrentTime > 0);
        timeLine.resume();
        QTRY_VERIFY(timeLine.currentTime() < oldCurrentTime);
        timeLine.stop();
        int currentTime = timeLine.currentTime();
        QVERIFY(currentTime < oldCurrentTime);
        QVERIFY(currentTime > 0);
    }
}

void tst_QTimeLine::restart()
{
    QTimeLine timeLine(100);
    timeLine.setFrameRange(0,9);

    timeLine.start();
    QTRY_COMPARE(timeLine.currentFrame(), timeLine.endFrame());
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);

    // A restart with the same duration
    timeLine.start();
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    QCOMPARE(timeLine.currentFrame(), timeLine.startFrame());
    QCOMPARE(timeLine.currentTime(), 0);
    QTRY_COMPARE(timeLine.currentFrame(), timeLine.endFrame());
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);

    // Set a smaller duration and restart
    timeLine.setDuration(50);
    timeLine.start();
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    QCOMPARE(timeLine.currentFrame(), timeLine.startFrame());
    QCOMPARE(timeLine.currentTime(), 0);
    QTRY_COMPARE(timeLine.currentFrame(), timeLine.endFrame());
    QCOMPARE(timeLine.state(), QTimeLine::NotRunning);

    // Set a longer duration and restart
    timeLine.setDuration(150);
    timeLine.start();
    QCOMPARE(timeLine.state(), QTimeLine::Running);
    QCOMPARE(timeLine.currentFrame(), timeLine.startFrame());
    QCOMPARE(timeLine.currentTime(), 0);
}

void tst_QTimeLine::setPaused()
{
    const int EndTime = 10000;
    QTimeLine timeLine(EndTime);
    {
        QCOMPARE(timeLine.currentTime(), 0);
        timeLine.start();
        QTRY_VERIFY(timeLine.currentTime() != 0);  // wait for start
        timeLine.setPaused(true);
        int oldCurrentTime = timeLine.currentTime();
        QVERIFY(oldCurrentTime > 0);
        QVERIFY(oldCurrentTime < EndTime);
        QTest::qWait(1000);
        timeLine.setPaused(false);
        QTRY_VERIFY(timeLine.currentTime() > oldCurrentTime);
        QVERIFY(timeLine.currentTime() > 0);
        QVERIFY(timeLine.currentTime() < EndTime);
        timeLine.stop();
    }
}

QTEST_MAIN(tst_QTimeLine)

#include "tst_qtimeline.moc"
