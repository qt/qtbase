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
#include <QtCore/qpropertyanimation.h>
#include <QtCore/qvariantanimation.h>
#include <QtGui/qtouchdevice.h>
#include <QtWidgets/qwidget.h>

Q_DECLARE_METATYPE(QAbstractAnimation::State)

class UncontrolledAnimation : public QPropertyAnimation
{
    Q_OBJECT
public:
    int duration() const { return -1; /* not time driven */ }

protected:
    void updateCurrentTime(int currentTime)
    {
        QPropertyAnimation::updateCurrentTime(currentTime);
        if (currentTime >= QPropertyAnimation::duration() || currentLoop() >= 1)
            stop();
    }
};

class MyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x WRITE setX)
public:
    MyObject() : m_x(0) { }
    qreal x() const { return m_x; }
    void setX(qreal x) { m_x = x; }
private:
    qreal m_x;
};

class DummyPropertyAnimation : public QPropertyAnimation
{
public:
    DummyPropertyAnimation(QObject *parent = 0) : QPropertyAnimation(parent)
    {
        setTargetObject(&o);
        this->setPropertyName("x");
        setEndValue(100);
    }

    MyObject o;
};


class tst_QPropertyAnimation : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase();

private slots:
    void construction();
    void setCurrentTime_data();
    void setCurrentTime();
    void statesAndSignals_data();
    void statesAndSignals();
    void deletion1();
    void deletion2();
    void deletion3();
    void duration0();
    void noStartValue();
    void noStartValueWithLoop();
    void startWhenAnotherIsRunning();
    void easingcurve_data();
    void easingcurve();
    void startWithoutStartValue();
    void startBackwardWithoutEndValue();
    void playForwardBackward();
    void interpolated();
    void setStartEndValues_data();
    void setStartEndValues();
    void zeroDurationStart();
    void zeroDurationForwardBackward();
    void operationsInStates_data();
    void operationsInStates();
    void oneKeyValue();
    void updateOnSetKeyValues();
    void restart();
    void valueChanged();
    void twoAnimations();
    void deletedInUpdateCurrentTime();
    void totalDuration();
    void zeroLoopCount();
    void recursiveAnimations();
};

void tst_QPropertyAnimation::initTestCase()
{
    qRegisterMetaType<QAbstractAnimation::State>("QAbstractAnimation::State");
    qRegisterMetaType<QAbstractAnimation::DeletionPolicy>("QAbstractAnimation::DeletionPolicy");
}

class AnimationObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(qreal realValue READ realValue WRITE setRealValue)
public:
    AnimationObject(int startValue = 0)
        : v(startValue), rv(startValue)
    { }

    int value() const { return v; }
    void setValue(int value) { v = value; }

    qreal realValue() const { return rv; }
    void setRealValue(qreal value) { rv = value; }

    int v;
    qreal rv;
};


void tst_QPropertyAnimation::construction()
{
    QPropertyAnimation panimation;
}

void tst_QPropertyAnimation::setCurrentTime_data()
{
    QTest::addColumn<int>("duration");
    QTest::addColumn<int>("loopCount");
    QTest::addColumn<int>("currentTime");
    QTest::addColumn<int>("testCurrentTime");
    QTest::addColumn<int>("testCurrentLoop");

    QTest::newRow("-1") << -1 << 1 << 0 << 0 << 0;
    QTest::newRow("0")  <<  0 << 1 << 0 << 0 << 0;
    QTest::newRow("1")  <<  0 << 1 << 1 << 0 << 0;
    QTest::newRow("2")  <<  0 << 2 << 1 << 0 << 0;
    QTest::newRow("3")  <<  1 << 1 << 0 << 0 << 0;
    QTest::newRow("4")  <<  1 << 1 << 1 << 1 << 0;
    QTest::newRow("5")  <<  1 << 2 << 1 << 0 << 1;
    QTest::newRow("6")  <<  1 << 2 << 2 << 1 << 1;
    QTest::newRow("7")  <<  1 << 2 << 3 << 1 << 1;
    QTest::newRow("8")  <<  1 << 3 << 2 << 0 << 2;
    QTest::newRow("9")  <<  1 << 3 << 3 << 1 << 2;
    QTest::newRow("a") <<  10 << 1 << 0 << 0 << 0;
    QTest::newRow("b") <<  10 << 1 << 1 << 1 << 0;
    QTest::newRow("c") <<  10 << 1 << 10 << 10 << 0;
    QTest::newRow("d") <<  10 << 2 << 10 << 0 << 1;
    QTest::newRow("e") <<  10 << 2 << 11 << 1 << 1;
    QTest::newRow("f") <<  10 << 2 << 20 << 10 << 1;
    QTest::newRow("g") <<  10 << 2 << 21 << 10 << 1;
    QTest::newRow("negloop 0") <<  10 << -1 << 0 << 0 << 0;
    QTest::newRow("negloop 1") <<  10 << -1 << 10 << 0 << 1;
    QTest::newRow("negloop 2") <<  10 << -1 << 15 << 5 << 1;
    QTest::newRow("negloop 3") <<  10 << -1 << 20 << 0 << 2;
    QTest::newRow("negloop 4") <<  10 << -1 << 30 << 0 << 3;
}

void tst_QPropertyAnimation::setCurrentTime()
{
    QFETCH(int, duration);
    QFETCH(int, loopCount);
    QFETCH(int, currentTime);
    QFETCH(int, testCurrentTime);
    QFETCH(int, testCurrentLoop);

    QPropertyAnimation animation;
    if (duration < 0)
        QTest::ignoreMessage(QtWarningMsg, "QVariantAnimation::setDuration: cannot set a negative duration");
    animation.setDuration(duration);
    animation.setLoopCount(loopCount);
    animation.setCurrentTime(currentTime);

    QCOMPARE(animation.currentLoopTime(), testCurrentTime);
    QCOMPARE(animation.currentLoop(), testCurrentLoop);
}

void tst_QPropertyAnimation::statesAndSignals_data()
{
    QTest::addColumn<bool>("uncontrolled");
    QTest::newRow("normal animation") << false;
    QTest::newRow("animation with undefined duration")  <<  true;
}

void tst_QPropertyAnimation::statesAndSignals()
{
    QFETCH(bool, uncontrolled);
    QPropertyAnimation *anim;
    if (uncontrolled)
        anim = new UncontrolledAnimation;
    else
        anim = new DummyPropertyAnimation;
    anim->setDuration(100);

    QSignalSpy finishedSpy(anim, &QPropertyAnimation::finished);
    QSignalSpy runningSpy(anim, &QPropertyAnimation::stateChanged);
    QSignalSpy currentLoopSpy(anim, &QPropertyAnimation::currentLoopChanged);

    QVERIFY(finishedSpy.isValid());
    QVERIFY(runningSpy.isValid());
    QVERIFY(currentLoopSpy.isValid());

    anim->setCurrentTime(1);
    anim->setCurrentTime(100);
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(runningSpy.count(), 0);
    QCOMPARE(currentLoopSpy.count(), 0);
    QCOMPARE(anim->state(), QAnimationGroup::Stopped);

    anim->setLoopCount(3);
    anim->setCurrentTime(101);

    if (uncontrolled)
        QSKIP("Uncontrolled animations don't handle looping");

    QCOMPARE(currentLoopSpy.count(), 1);
    QCOMPARE(anim->currentLoop(), 1);

    anim->setCurrentTime(0);
    QCOMPARE(currentLoopSpy.count(), 2);
    QCOMPARE(anim->currentLoop(), 0);

    anim->start();
    QCOMPARE(anim->state(), QAnimationGroup::Running);
    QCOMPARE(runningSpy.count(), 1); //anim must have started
    QCOMPARE(anim->currentLoop(), 0);
    runningSpy.clear();

    anim->stop();
    QCOMPARE(anim->state(), QAnimationGroup::Stopped);
    QCOMPARE(runningSpy.count(), 1); //anim must have stopped
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(anim->currentLoopTime(), 0);
    QCOMPARE(anim->currentLoop(), 0);
    QCOMPARE(currentLoopSpy.count(), 2);
    runningSpy.clear();

    anim->start();
    QTest::qWait(1000);
    QTRY_COMPARE(anim->state(), QAnimationGroup::Stopped);
    QCOMPARE(runningSpy.count(), 2); //started and stopped again
    runningSpy.clear();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(anim->currentLoopTime(), 100);
    QCOMPARE(anim->currentLoop(), 2);
    QCOMPARE(currentLoopSpy.count(), 4);

    anim->start(); // auto-rewinds
    QCOMPARE(anim->state(), QAnimationGroup::Running);
    QCOMPARE(anim->currentTime(), 0);
    QCOMPARE(anim->currentLoop(), 0);
    QCOMPARE(currentLoopSpy.count(), 5);
    QCOMPARE(runningSpy.count(), 1); // anim has started
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(anim->currentLoop(), 0);
    runningSpy.clear();

    QTest::qWait(1000);

    QCOMPARE(currentLoopSpy.count(), 7);
    QCOMPARE(anim->state(), QAnimationGroup::Stopped);
    QCOMPARE(anim->currentLoop(), 2);
    QCOMPARE(runningSpy.count(), 1); // anim has stopped
    QCOMPARE(finishedSpy.count(), 2);
    QCOMPARE(anim->currentLoopTime(), 100);

    delete anim;
}

void tst_QPropertyAnimation::deletion1()
{
    QObject *object = new QWidget;
    QPointer<QPropertyAnimation> anim = new QPropertyAnimation(object, "minimumWidth");

    //test that the animation is deleted correctly depending of the deletion flag passed in start()
    QSignalSpy runningSpy(anim.data(), &QPropertyAnimation::stateChanged);
    QSignalSpy finishedSpy(anim.data(), &QPropertyAnimation::finished);
    QVERIFY(runningSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    anim->setStartValue(10);
    anim->setEndValue(20);
    anim->setDuration(200);
    anim->start();
    QCOMPARE(runningSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);

    QVERIFY(anim);
    QCOMPARE(anim->state(), QAnimationGroup::Running);
    QTest::qWait(100);
    QVERIFY(anim);
    QCOMPARE(anim->state(), QAnimationGroup::Running);
    QTest::qWait(150);
    QVERIFY(anim); //The animation should not have been deleted
    QTRY_COMPARE(anim->state(), QAnimationGroup::Stopped);
    QCOMPARE(runningSpy.count(), 2);
    QCOMPARE(finishedSpy.count(), 1);

    anim->start(QVariantAnimation::DeleteWhenStopped);
    QVERIFY(anim);
    QCOMPARE(anim->state(), QAnimationGroup::Running);
    QTest::qWait(100);
    QVERIFY(anim);
    QCOMPARE(anim->state(), QAnimationGroup::Running);
    QTest::qWait(150);
    QTRY_COMPARE(runningSpy.count(), 4);
    QCOMPARE(finishedSpy.count(), 2);
    QVERIFY(!anim);  //The animation must have been deleted
    delete object;
}

void tst_QPropertyAnimation::deletion2()
{
    //test that the animation get deleted if the object is deleted
    QObject *object = new QWidget;
    QPointer<QPropertyAnimation> anim = new QPropertyAnimation(object,"minimumWidth");
    anim->setStartValue(10);
    anim->setEndValue(20);
    anim->setDuration(200);

    QSignalSpy runningSpy(anim.data(), &QPropertyAnimation::stateChanged);
    QSignalSpy finishedSpy(anim.data(), &QPropertyAnimation::finished);

    QVERIFY(runningSpy.isValid());
    QVERIFY(finishedSpy.isValid());

    anim->setStartValue(10);
    anim->setEndValue(20);
    anim->setDuration(200);
    anim->start();

    QTest::qWait(50);
    QVERIFY(anim);
    QCOMPARE(anim->state(), QAnimationGroup::Running);

    QCOMPARE(runningSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);

    //we can't call deletaLater directly because the delete would only happen in the next loop of _this_ event loop
    QTimer::singleShot(0, object, SLOT(deleteLater()));
    QTest::qWait(50);

    QVERIFY(!anim->targetObject());
}

void tst_QPropertyAnimation::deletion3()
{
    //test that the stopped signal is emit when the animation is destroyed
    QObject *object = new QWidget;
    QPropertyAnimation *anim = new QPropertyAnimation(object,"minimumWidth");
    anim->setStartValue(10);
    anim->setEndValue(20);
    anim->setDuration(200);

    QSignalSpy runningSpy(anim, &QPropertyAnimation::stateChanged);
    QSignalSpy finishedSpy(anim, &QPropertyAnimation::finished);

    QVERIFY(runningSpy.isValid());
    QVERIFY(finishedSpy.isValid());

    anim->start();

    QTest::qWait(50);
    QCOMPARE(anim->state(), QAnimationGroup::Running);
    QCOMPARE(runningSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);
    delete anim;
    QCOMPARE(runningSpy.count(), 2);
    QCOMPARE(finishedSpy.count(), 0);
}

void tst_QPropertyAnimation::duration0()
{
    QObject o;
    o.setProperty("ole", 42);
    QCOMPARE(o.property("ole").toInt(), 42);

    QPropertyAnimation animation(&o, "ole");
    animation.setEndValue(43);
    QVERIFY(!animation.currentValue().isValid());
    QCOMPARE(animation.currentValue().toInt(), 0);
    animation.setStartValue(42);
    QVERIFY(animation.currentValue().isValid());
    QCOMPARE(animation.currentValue().toInt(), 42);

    QCOMPARE(o.property("ole").toInt(), 42);
    animation.setDuration(0);
    QCOMPARE(animation.currentValue().toInt(), 43); //it is at the end
    animation.start();
    QCOMPARE(animation.state(), QAnimationGroup::Stopped);
    QCOMPARE(animation.currentTime(), 0);
    QCOMPARE(o.property("ole").toInt(), 43);
}

class StartValueTester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int ole READ ole WRITE setOle)
public:
    StartValueTester() : o(0) { }
    int ole() const { return o; }
    void setOle(int v) { o = v; values << v; }

    int o;
    QList<int> values;
};

void tst_QPropertyAnimation::noStartValue()
{
    StartValueTester o;
    o.setProperty("ole", 42);
    o.values.clear();

    QPropertyAnimation a(&o, "ole");
    a.setEndValue(420);
    a.setDuration(250);
    a.start();

    QTest::qWait(300);

    QTRY_COMPARE(o.values.first(), 42);
    QCOMPARE(o.values.last(), 420);
}

void tst_QPropertyAnimation::noStartValueWithLoop()
{
    StartValueTester o;
    o.setProperty("ole", 42);
    o.values.clear();

    QPropertyAnimation a(&o, "ole");
    a.setEndValue(420);
    a.setDuration(250);
    a.setLoopCount(2);
    a.start();

    a.setCurrentTime(250);
    QCOMPARE(o.values.first(), 42);
    QCOMPARE(a.currentValue().toInt(), 42);
    QCOMPARE(o.values.last(), 42);

    a.setCurrentTime(500);
    QCOMPARE(a.currentValue().toInt(), 420);
}

void tst_QPropertyAnimation::startWhenAnotherIsRunning()
{
    StartValueTester o;
    o.setProperty("ole", 42);
    o.values.clear();

    {
        //normal case: the animation finishes and is deleted
        QPointer<QVariantAnimation> anim = new QPropertyAnimation(&o, "ole");
        anim->setEndValue(100);
        QSignalSpy runningSpy(anim.data(), &QVariantAnimation::stateChanged);
        QVERIFY(runningSpy.isValid());
        anim->start(QVariantAnimation::DeleteWhenStopped);
        QTest::qWait(anim->duration() + 100);
        QTRY_COMPARE(runningSpy.count(), 2); //started and then stopped
        QVERIFY(!anim);
    }

    {
        QPointer<QVariantAnimation> anim = new QPropertyAnimation(&o, "ole");
        anim->setEndValue(100);
        QSignalSpy runningSpy(anim.data(), &QVariantAnimation::stateChanged);
        QVERIFY(runningSpy.isValid());
        anim->start(QVariantAnimation::DeleteWhenStopped);
        QTest::qWait(anim->duration()/2);
        QPointer<QVariantAnimation> anim2 = new QPropertyAnimation(&o, "ole");
        anim2->setEndValue(100);
        QCOMPARE(runningSpy.count(), 1);
        QCOMPARE(anim->state(), QVariantAnimation::Running);

        //anim2 will interrupt anim1
        QMetaObject::invokeMethod(anim2, "start", Qt::QueuedConnection, Q_ARG(QAbstractAnimation::DeletionPolicy, QVariantAnimation::DeleteWhenStopped));
        QTest::qWait(50);
        QVERIFY(!anim); //anim should have been deleted
        QVERIFY(anim2);
        QTest::qWait(anim2->duration());
        QTRY_VERIFY(!anim2); //anim2 is finished: it should have been deleted by now
        QVERIFY(!anim);
    }

}

// copy  from easing.cpp in case that function changes definition
static qreal easeInOutBack(qreal t)
{
    qreal s = 1.70158;
    qreal t_adj = 2.0f * (qreal)t;
    if (t_adj < 1) {
        s *= 1.525f;
        return 1.0/2*(t_adj*t_adj*((s+1)*t_adj - s));
    } else {
        t_adj -= 2;
        s *= 1.525f;
        return 1.0/2*(t_adj*t_adj*((s+1)*t_adj + s) + 2);
    }
}

void tst_QPropertyAnimation::easingcurve_data()
{
    QTest::addColumn<int>("currentTime");
    QTest::addColumn<int>("expectedvalue");

    QTest::newRow("interpolation1") << 0 << 0;
    QTest::newRow("interpolation2") << 1000 << 1000;
    QTest::newRow("extrapolationbelow") << 250 << -99;
    QTest::newRow("extrapolationabove") << 750 << 1099;
}

void tst_QPropertyAnimation::easingcurve()
{
    QFETCH(int, currentTime);
    QFETCH(int, expectedvalue);
    QObject o;
    o.setProperty("ole", 42);
    QCOMPARE(o.property("ole").toInt(), 42);

    QPropertyAnimation pAnimation(&o, "ole");
    pAnimation.setStartValue(0);
    pAnimation.setEndValue(1000);
    pAnimation.setDuration(1000);

    // this easingcurve assumes that we extrapolate before startValue and after endValue
    QEasingCurve easingCurve;
    easingCurve.setCustomType(easeInOutBack);
    pAnimation.setEasingCurve(easingCurve);
    pAnimation.start();
    pAnimation.pause();
    pAnimation.setCurrentTime(currentTime);
    QCOMPARE(o.property("ole").toInt(), expectedvalue);
}

void tst_QPropertyAnimation::startWithoutStartValue()
{
    QObject o;
    o.setProperty("ole", 42);
    QCOMPARE(o.property("ole").toInt(), 42);

    QPropertyAnimation anim(&o, "ole");
    anim.setEndValue(100);

    anim.start();

    QTest::qWait(100);
    int current = anim.currentValue().toInt();
    //it is somewhere in the animation
    QVERIFY(current > 42);
    QVERIFY(current < 100);

    QTest::qWait(200);
    QTRY_COMPARE(anim.state(), QVariantAnimation::Stopped);
    current = anim.currentValue().toInt();
    QCOMPARE(current, 100);
    QCOMPARE(o.property("ole").toInt(), current);

    anim.setEndValue(110);
    anim.start();
    current = anim.currentValue().toInt();
    // the default start value will reevaluate the current property
    // and set it to the end value of the last iteration
    QCOMPARE(current, 100);
    QTest::qWait(100);
    current = anim.currentValue().toInt();
    //it is somewhere in the animation
    QVERIFY(current >= 100);
    QVERIFY(current <= 110);
}

void tst_QPropertyAnimation::startBackwardWithoutEndValue()
{
    QObject o;
    o.setProperty("ole", 42);
    QCOMPARE(o.property("ole").toInt(), 42);

    QPropertyAnimation anim(&o, "ole");
    anim.setStartValue(100);
    anim.setDirection(QAbstractAnimation::Backward);

    //we start without an end value
    anim.start();
    QCOMPARE(anim.state(), QAbstractAnimation::Running);
    QCOMPARE(o.property("ole").toInt(), 42); //the initial value

    QTest::qWait(100);
    int current = anim.currentValue().toInt();
    //it is somewhere in the animation
    QVERIFY(current > 42);
    QVERIFY(current < 100);

    QTest::qWait(200);
    QTRY_COMPARE(anim.state(), QVariantAnimation::Stopped);
    current = anim.currentValue().toInt();
    QCOMPARE(current, 100);
    QCOMPARE(o.property("ole").toInt(), current);

    anim.setStartValue(110);
    anim.start();
    current = anim.currentValue().toInt();
    // the default start value will reevaluate the current property
    // and set it to the end value of the last iteration
    QCOMPARE(current, 100);
    QTest::qWait(100);
    current = anim.currentValue().toInt();
    //it is somewhere in the animation
    QVERIFY(current >= 100);
    QVERIFY(current <= 110);
}


void tst_QPropertyAnimation::playForwardBackward()
{
    QObject o;
    o.setProperty("ole", 0);
    QCOMPARE(o.property("ole").toInt(), 0);

    QPropertyAnimation anim(&o, "ole");
    anim.setStartValue(0);
    anim.setEndValue(100);
    anim.start();
    QTest::qWait(anim.duration() + 100);
    QTRY_COMPARE(anim.state(), QAbstractAnimation::Stopped);
    QCOMPARE(anim.currentTime(), anim.duration());

    //the animation is at the end
    anim.setDirection(QVariantAnimation::Backward);
    anim.start();
    QCOMPARE(anim.state(), QAbstractAnimation::Running);
    QTest::qWait(anim.duration() + 100);
    QTRY_COMPARE(anim.state(), QAbstractAnimation::Stopped);
    QCOMPARE(anim.currentTime(), 0);

    //the direction is backward
    //restarting should jump to the end
    anim.start();
    QCOMPARE(anim.state(), QAbstractAnimation::Running);
    QCOMPARE(anim.currentTime(), anim.duration());
    QTest::qWait(anim.duration() + 100);
    QTRY_COMPARE(anim.state(), QAbstractAnimation::Stopped);
    QCOMPARE(anim.currentTime(), 0);
}

struct Number
{
    Number() {}
    Number(int n)
        : n(n) {}

    Number(const Number &other)
        : n(other.n){}

    Number &operator=(const Number &other) {
        n = other.n;
        return *this;
    }
    bool operator==(const Number &other) const {
        return n == other.n;
    }

    int n;
};

Q_DECLARE_METATYPE(Number)

QVariant numberInterpolator(const Number &f, const Number &t, qreal progress)
{
    return QVariant::fromValue<Number>(Number(f.n + (t.n - f.n)*progress));
}

QVariant xaxisQPointInterpolator(const QPointF &f, const QPointF &t, qreal progress)
{
    return QPointF(f.x() + (t.x() - f.x())*progress, f.y());
}

void tst_QPropertyAnimation::interpolated()
{
    QObject o;
    o.setProperty("point", QPointF()); //this will avoid warnings
    o.setProperty("number", QVariant::fromValue<Number>(Number(42)));
    QCOMPARE(qvariant_cast<Number>(o.property("number")), Number(42));
    {
    qRegisterAnimationInterpolator<Number>(numberInterpolator);
    QPropertyAnimation anim(&o, "number");
    anim.setStartValue(QVariant::fromValue<Number>(Number(0)));
    anim.setEndValue(QVariant::fromValue<Number>(Number(100)));
    anim.setDuration(1000);
    anim.start();
    anim.pause();
    anim.setCurrentTime(100);
    Number t(qvariant_cast<Number>(o.property("number")));
    QCOMPARE(t, Number(10));
    anim.setCurrentTime(500);
    QCOMPARE(qvariant_cast<Number>(o.property("number")), Number(50));
    }
    {
    qRegisterAnimationInterpolator<QPointF>(xaxisQPointInterpolator);
    QPropertyAnimation anim(&o, "point");
    anim.setStartValue(QPointF(0,0));
    anim.setEndValue(QPointF(100, 100));
    anim.setDuration(1000);
    anim.start();
    anim.pause();
    anim.setCurrentTime(100);
    QCOMPARE(o.property("point"), QVariant(QPointF(10, 0)));
    anim.setCurrentTime(500);
    QCOMPARE(o.property("point"), QVariant(QPointF(50, 0)));
    }
    {
    // unregister it and see if we get back the default behaviour
    qRegisterAnimationInterpolator<QPointF>(0);
    QPropertyAnimation anim(&o, "point");
    anim.setStartValue(QPointF(0,0));
    anim.setEndValue(QPointF(100, 100));
    anim.setDuration(1000);
    anim.start();
    anim.pause();
    anim.setCurrentTime(100);
    QCOMPARE(o.property("point").toPointF(), QPointF(10, 10));
    anim.setCurrentTime(500);
    QCOMPARE(o.property("point").toPointF(), QPointF(50, 50));
    }

    {
    // Interpolate a qreal property with a int interpolator
    AnimationObject o1;
    o1.setRealValue(42.42);
    QPropertyAnimation anim(&o1, "realValue");
    anim.setStartValue(0);
    anim.setEndValue(100);
    anim.start();
    QCOMPARE(o1.realValue(), qreal(0));
    anim.setCurrentTime(250);
    QCOMPARE(o1.realValue(), qreal(100));
    }
}


void tst_QPropertyAnimation::setStartEndValues_data()
{
    QTest::addColumn<QByteArray>("propertyName");
    QTest::addColumn<QVariant>("initialValue");
    QTest::addColumn<QVariant>("startValue");
    QTest::addColumn<QVariant>("endValue");

    QTest::newRow("dynamic property")  << QByteArray("ole") << QVariant(42) << QVariant(0) << QVariant(10);
    QTest::newRow("real property, with unmatching types") << QByteArray("x") << QVariant(42.) << QVariant(0) << QVariant(10.);
}

void tst_QPropertyAnimation::setStartEndValues()
{
    MyObject object;
    QFETCH(QByteArray, propertyName);
    QFETCH(QVariant, initialValue);
    QFETCH(QVariant, startValue);
    QFETCH(QVariant, endValue);

    //this tests the start value, end value and default start value
    object.setProperty(propertyName, initialValue);
    QPropertyAnimation anim(&object, propertyName);
    QVariantAnimation::KeyValues values;
    QCOMPARE(anim.keyValues(), values);

    //let's add a start value
    anim.setStartValue(startValue);
    values << QVariantAnimation::KeyValue(0, startValue);
    QCOMPARE(anim.keyValues(), values);

    anim.setEndValue(endValue);
    values << QVariantAnimation::KeyValue(1, endValue);
    QCOMPARE(anim.keyValues(), values);

    //now we can play with objects
    QCOMPARE(object.property(propertyName).toDouble(), initialValue.toDouble());
    anim.start();
    QVERIFY(anim.startValue().isValid());
    QCOMPARE(object.property(propertyName), anim.startValue());
    anim.setCurrentTime(anim.duration()/2);
    QCOMPARE(object.property(propertyName).toDouble(), (startValue.toDouble() + endValue.toDouble())/2 ); //just in the middle of the animation
    anim.setCurrentTime(anim.duration()); //we go to the end of the animation
    QCOMPARE(anim.state(), QAnimationGroup::Stopped); //it should have stopped
    QVERIFY(anim.endValue().isValid());
    QCOMPARE(object.property(propertyName), anim.endValue()); //end of the animations

    //now we remove the explicit start value and test the implicit one
    anim.stop();
    object.setProperty(propertyName, initialValue);

    //let's reset the start value
    values.remove(0);
    anim.setStartValue(QVariant());
    QCOMPARE(anim.keyValues(), values);
    QVERIFY(!anim.startValue().isValid());

    anim.start();
    QCOMPARE(object.property(propertyName), initialValue);
    anim.setCurrentTime(anim.duration()/2);
    QCOMPARE(object.property(propertyName).toDouble(), (initialValue.toDouble() + endValue.toDouble())/2 ); //just in the middle of the animation
    anim.setCurrentTime(anim.duration()); //we go to the end of the animation
    QCOMPARE(anim.state(), QAnimationGroup::Stopped); //it should have stopped
    QVERIFY(anim.endValue().isValid());
    QCOMPARE(object.property(propertyName), anim.endValue()); //end of the animations

    //now we set back the startValue
    anim.setStartValue(startValue);
    QVERIFY(anim.startValue().isValid());
    anim.start();
    QCOMPARE(object.property(propertyName), startValue);
}

void tst_QPropertyAnimation::zeroDurationStart()
{
    DummyPropertyAnimation anim;
    QSignalSpy spy(&anim, &DummyPropertyAnimation::stateChanged);
    QVERIFY(spy.isValid());
    anim.setDuration(0);
    QCOMPARE(anim.state(), QAbstractAnimation::Stopped);
    anim.start();
    //the animation stops immediately
    QCOMPARE(anim.state(), QAbstractAnimation::Stopped);
    QCOMPARE(spy.count(), 2);

    //let's check the first state change
    const QVariantList firstChange = spy.first();
    //old state
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(firstChange.last()), QAbstractAnimation::Stopped);
    //new state
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(firstChange.first()), QAbstractAnimation::Running);

    //let's check the first state change
    const QVariantList secondChange = spy.last();
    //old state
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(secondChange.last()), QAbstractAnimation::Running);
    //new state
    QCOMPARE(qvariant_cast<QAbstractAnimation::State>(secondChange.first()), QAbstractAnimation::Stopped);
}

void tst_QPropertyAnimation::zeroDurationForwardBackward()
{
    QObject o; o.setProperty("test", 1);
    QObject o2; o2.setProperty("test", 2);
    QObject o3; o3.setProperty("test", 3);
    QObject o4; o4.setProperty("test", 4);
    QPropertyAnimation prop(&o, "test"); prop.setDuration(0); prop.setStartValue(1); prop.setEndValue(2);

    prop.start();
    QCOMPARE(o.property("test").toInt(), 2);
    prop.setDirection(QAbstractAnimation::Backward);
    prop.start();
    QCOMPARE(o.property("test").toInt(), 1);

    prop.setDirection(QAbstractAnimation::Forward);
    QPropertyAnimation prop2(&o2, "test"); prop2.setDuration(0); prop2.setStartValue(2); prop2.setEndValue(3);
    QPropertyAnimation prop3(&o3, "test"); prop3.setDuration(0); prop3.setStartValue(3); prop3.setEndValue(4);
    QPropertyAnimation prop4(&o4, "test"); prop4.setDuration(0); prop4.setStartValue(4); prop4.setEndValue(5);
    QSequentialAnimationGroup group;
    group.addAnimation(&prop);
    group.addAnimation(&prop2);
    group.addAnimation(&prop3);
    group.addAnimation(&prop4);
    group.start();

    QCOMPARE(o.property("test").toInt(), 2);
    QCOMPARE(o2.property("test").toInt(), 3);
    QCOMPARE(o3.property("test").toInt(), 4);
    QCOMPARE(o4.property("test").toInt(), 5);

    group.setDirection(QAbstractAnimation::Backward);
    group.start();

    QCOMPARE(o.property("test").toInt(), 1);
    QCOMPARE(o2.property("test").toInt(), 2);
    QCOMPARE(o3.property("test").toInt(), 3);
    QCOMPARE(o4.property("test").toInt(), 4);

    group.removeAnimation(&prop);
    group.removeAnimation(&prop2);
    group.removeAnimation(&prop3);
    group.removeAnimation(&prop4);
}

#define Pause 1
#define Start 2
#define Resume 3
#define Stop 4

void tst_QPropertyAnimation::operationsInStates_data()
{
    QTest::addColumn<QAbstractAnimation::State>("originState");
    QTest::addColumn<int>("operation");
    QTest::addColumn<QString>("expectedWarning");
    QTest::addColumn<QAbstractAnimation::State>("expectedState");

    QString pauseWarn(QLatin1String("QAbstractAnimation::pause: Cannot pause a stopped animation"));
    QString resumeWarn(QLatin1String("QAbstractAnimation::resume: Cannot resume an animation that is not paused"));

    QTest::newRow("S-pause")  << QAbstractAnimation::Stopped << Pause << pauseWarn << QAbstractAnimation::Stopped;
    QTest::newRow("S-start")  << QAbstractAnimation::Stopped << Start << QString() << QAbstractAnimation::Running;
    QTest::newRow("S-resume") << QAbstractAnimation::Stopped << Resume << resumeWarn << QAbstractAnimation::Stopped;
    QTest::newRow("S-stop")   << QAbstractAnimation::Stopped << Stop << QString() << QAbstractAnimation::Stopped;

    QTest::newRow("P-pause")  << QAbstractAnimation::Paused << Pause << QString() << QAbstractAnimation::Paused;
    QTest::newRow("P-start")  << QAbstractAnimation::Paused << Start << QString() << QAbstractAnimation::Running;
    QTest::newRow("P-resume") << QAbstractAnimation::Paused << Resume << QString() << QAbstractAnimation::Running;
    QTest::newRow("P-stop")   << QAbstractAnimation::Paused << Stop << QString() << QAbstractAnimation::Stopped;

    QTest::newRow("R-pause")  << QAbstractAnimation::Running << Pause << QString() << QAbstractAnimation::Paused;
    QTest::newRow("R-start")  << QAbstractAnimation::Running << Start << QString() << QAbstractAnimation::Running;
    QTest::newRow("R-resume") << QAbstractAnimation::Running << Resume << resumeWarn << QAbstractAnimation::Running;
    QTest::newRow("R-stop")   << QAbstractAnimation::Running << Stop << QString() << QAbstractAnimation::Stopped;
}

void tst_QPropertyAnimation::operationsInStates()
{
/**
 *           | pause()    |start()    |resume()   |stop()
 * ----------+------------+-----------+-----------+-------------------+
 * Stopped   | Stopped    |Running    |Stopped    |Stopped            |
 *          _| qWarning   |restart    |qWarning   |                   |
 * Paused    | Paused     |Running    |Running    |Stopped            |
 *          _|            |           |           |                   |
 * Running   | Paused     |Running    |Running    |Stopped            |
 *           |            |restart    |qWarning   |                   |
 * ----------+------------+-----------+-----------+-------------------+
**/

    QFETCH(QAbstractAnimation::State, originState);
    QFETCH(int, operation);
    QFETCH(QString, expectedWarning);
    QFETCH(QAbstractAnimation::State, expectedState);

    QObject o;
    o.setProperty("ole", 42);
    QPropertyAnimation anim(&o, "ole");
    anim.setEndValue(100);
    QSignalSpy spy(&anim, &QPropertyAnimation::stateChanged);
    QVERIFY(spy.isValid());

    anim.stop();
    switch (originState) {
    case QAbstractAnimation::Stopped:
    break;
    case QAbstractAnimation::Paused:
        anim.start();
        anim.pause();
    break;
    case QAbstractAnimation::Running:
        anim.start();
    break;
    }
    if (!expectedWarning.isEmpty()) {
        QTest::ignoreMessage(QtWarningMsg, qPrintable(expectedWarning));
    }
    QCOMPARE(anim.state(), originState);
    switch (operation) {
    case Pause:
        anim.pause();
    break;
    case Start:
        anim.start();
    break;
    case Resume:
        anim.resume();
    break;
    case Stop:
        anim.stop();
    break;
    }

    QCOMPARE(anim.state(), expectedState);
}
#undef Pause
#undef Start
#undef Resume
#undef Stop

void tst_QPropertyAnimation::oneKeyValue()
{
    QObject o;
    o.setProperty("ole", 42);
    QCOMPARE(o.property("ole").toInt(), 42);

    QPropertyAnimation animation(&o, "ole");
    animation.setStartValue(43);
    animation.setEndValue(44);
    animation.setDuration(100);

    animation.setCurrentTime(0);

    QVERIFY(animation.currentValue().isValid());
    QCOMPARE(animation.currentValue().toInt(), 43);
    QCOMPARE(o.property("ole").toInt(), 42);

    // remove the last key value
    animation.setKeyValueAt(1.0, QVariant());

    // we will neither interpolate, nor update the current value
    // since there is only one 1 key value defined
    animation.setCurrentTime(100);

    // the animation should not have been modified
    QVERIFY(animation.currentValue().isValid());
    QCOMPARE(animation.currentValue().toInt(), 43);
    QCOMPARE(o.property("ole").toInt(), 42);
}

void tst_QPropertyAnimation::updateOnSetKeyValues()
{
    QObject o;
    o.setProperty("ole", 100);
    QCOMPARE(o.property("ole").toInt(), 100);

    QPropertyAnimation animation(&o, "ole");
    animation.setStartValue(100);
    animation.setEndValue(200);
    animation.setDuration(100);

    animation.setCurrentTime(50);
    QCOMPARE(animation.currentValue().toInt(), 150);
    animation.setKeyValueAt(0.0, 300);
    QCOMPARE(animation.currentValue().toInt(), 250);

    o.setProperty("ole", 100);
    QPropertyAnimation animation2(&o, "ole");
    QVariantAnimation::KeyValues kValues;
    kValues << QVariantAnimation::KeyValue(0.0, 100) << QVariantAnimation::KeyValue(1.0, 200);
    animation2.setKeyValues(kValues);
    animation2.setDuration(100);
    animation2.setCurrentTime(50);
    QCOMPARE(animation2.currentValue().toInt(), 150);

    kValues.clear();
    kValues << QVariantAnimation::KeyValue(0.0, 300) << QVariantAnimation::KeyValue(1.0, 200);
    animation2.setKeyValues(kValues);

    QCOMPARE(animation2.currentValue().toInt(), animation.currentValue().toInt());
}


//this class will 'throw' an error in the test lib
// if the property ole is set to ErrorValue
class MyErrorObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int ole READ ole WRITE setOle)
public:

    static const int ErrorValue = 10000;

    MyErrorObject() : m_ole(0) { }
    int ole() const { return m_ole; }
    void setOle(int o)
    {
        QVERIFY(o != ErrorValue);
        m_ole = o;
    }

private:
    int m_ole;


};

void tst_QPropertyAnimation::restart()
{
    //here we check that be restarting an animation
    //it doesn't get an bogus intermediate value (end value)
    //because the time is not yet reset to 0
    MyErrorObject o;
    o.setOle(100);
    QCOMPARE(o.property("ole").toInt(), 100);

    QPropertyAnimation anim(&o, "ole");
    anim.setEndValue(200);
    anim.start();
    anim.setCurrentTime(anim.duration());
    QCOMPARE(anim.state(), QAbstractAnimation::Stopped);
    QCOMPARE(o.property("ole").toInt(), 200);

    //we'll check that the animation never gets a wrong value when starting it
    //after having changed the end value
    anim.setEndValue(MyErrorObject::ErrorValue);
    anim.start();
}

void tst_QPropertyAnimation::valueChanged()
{

    //we check that we receive the valueChanged signal
    MyErrorObject o;
    o.setOle(0);
    QCOMPARE(o.property("ole").toInt(), 0);
    QPropertyAnimation anim(&o, "ole");
    anim.setEndValue(5);
    anim.setDuration(1000);
    QSignalSpy spy(&anim, &QPropertyAnimation::valueChanged);
    QVERIFY(spy.isValid());
    anim.start();

    QTest::qWait(anim.duration() + 100);

    QTRY_COMPARE(anim.state(), QAbstractAnimation::Stopped);
    QCOMPARE(anim.currentTime(), anim.duration());

    //let's check that the values go forward
    QCOMPARE(spy.count(), 6); //we should have got everything from 0 to 5
    for (int i = 0; i < spy.count(); ++i) {
        QCOMPARE(qvariant_cast<QVariant>(spy.at(i).first()).toInt(), i);
    }
}

//this class will help us make sure that 2 animations started
//at the same time also end at the same time
class MySyncObject : public MyErrorObject
{
    Q_OBJECT
public:
    MySyncObject() : anim(this, "ole")
    {
        anim.setEndValue(1000);
    }
public slots:
    void checkAnimationFinished()
    {
        QCOMPARE(anim.state(), QAbstractAnimation::Stopped);
        QCOMPARE(ole(), 1000);
    }

public:
    QPropertyAnimation anim;
};

void tst_QPropertyAnimation::twoAnimations()
{
    MySyncObject o1, o2;
    o1.setOle(0);
    o2.setOle(0);

    //when the animation in o1 is finished
    //the animation in o2 should stop around the same time
    //We use a queued connection to check just after the tick from the common timer
    //the other way is true too
    QObject::connect(&o1.anim, SIGNAL(finished()),
        &o2, SLOT(checkAnimationFinished()), Qt::QueuedConnection);
    QObject::connect(&o2.anim, SIGNAL(finished()),
        &o1, SLOT(checkAnimationFinished()), Qt::QueuedConnection);

    o1.anim.start();
    o2.anim.start();

    QTest::qWait(o1.anim.duration() + 100);
    QTRY_COMPARE(o1.anim.state(), QAbstractAnimation::Stopped);
    QCOMPARE(o2.anim.state(), QAbstractAnimation::Stopped);

    QCOMPARE(o1.ole(), 1000);
    QCOMPARE(o2.ole(), 1000);
}

class MyComposedAnimation : public QPropertyAnimation
{
    Q_OBJECT
public:
    MyComposedAnimation(QObject *target, const QByteArray &propertyName, const QByteArray &innerPropertyName)
        : QPropertyAnimation(target, propertyName)
    {
        innerAnim = new QPropertyAnimation(target, innerPropertyName);
        this->setEndValue(1000);
        innerAnim->setEndValue(1000);
        innerAnim->setDuration(duration() + 100);
    }

    void start()
    {
        QPropertyAnimation::start();
        innerAnim->start();
    }

    void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
    {
        QPropertyAnimation::updateState(newState, oldState);
        if (newState == QAbstractAnimation::Stopped)
            delete innerAnim;
    }

public:
    QPropertyAnimation *innerAnim;
};

void tst_QPropertyAnimation::deletedInUpdateCurrentTime()
{
    // this test case reproduces an animation being deleted in the updateCurrentTime of
    // another animation(was causing segfault).
    // the deleted animation must have been started after the animation that is deleting.
    AnimationObject o;
    o.setValue(0);
    o.setRealValue(0.0);

    MyComposedAnimation composedAnimation(&o, "value", "realValue");
    composedAnimation.start();
    QCOMPARE(composedAnimation.state(), QAbstractAnimation::Running);
    QTest::qWait(composedAnimation.duration() + 100);

    QTRY_COMPARE(composedAnimation.state(), QAbstractAnimation::Stopped);
    QCOMPARE(o.value(), 1000);
}

void tst_QPropertyAnimation::totalDuration()
{
    QPropertyAnimation anim;
    QCOMPARE(anim.totalDuration(), 250);
    anim.setLoopCount(2);
    QCOMPARE(anim.totalDuration(), 2*250);
    anim.setLoopCount(-1);
    QCOMPARE(anim.totalDuration(), -1);
    anim.setDuration(0);
    QCOMPARE(anim.totalDuration(), 0);
}

void tst_QPropertyAnimation::zeroLoopCount()
{
    DummyPropertyAnimation* anim;
    anim = new DummyPropertyAnimation;
    anim->setStartValue(0);
    anim->setDuration(20);
    anim->setLoopCount(0);

    QSignalSpy runningSpy(anim, &QPropertyAnimation::stateChanged);
    QSignalSpy finishedSpy(anim, &QPropertyAnimation::finished);

    QVERIFY(runningSpy.isValid());
    QVERIFY(finishedSpy.isValid());

    QCOMPARE(anim->state(), QAnimationGroup::Stopped);
    QCOMPARE(anim->currentValue().toInt(), 0);
    QCOMPARE(runningSpy.count(), 0);
    QCOMPARE(finishedSpy.count(), 0);

    anim->start();

    QCOMPARE(anim->state(), QAnimationGroup::Stopped);
    QCOMPARE(anim->currentValue().toInt(), 0);
    QCOMPARE(runningSpy.count(), 0);
    QCOMPARE(finishedSpy.count(), 0);
}


class RecursiveObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x WRITE setX)
    Q_PROPERTY(qreal y READ y WRITE setY)
public:
    RecursiveObject() : m_x(0), m_y(0) {
        animation.setTargetObject(this);
        animation.setPropertyName("y");
        animation.setDuration(30);
    }
    qreal x() const { return m_x; }
    void setX(qreal x) {
        m_x = x;
        animation.setEndValue(x);
        animation.start();
    }
    qreal y() const { return m_y; }
    void setY(qreal y) { m_y = y; }

    qreal m_x;
    qreal m_y;
    QPropertyAnimation animation;
};


void tst_QPropertyAnimation::recursiveAnimations()
{
    RecursiveObject o;
    QPropertyAnimation anim;
    anim.setTargetObject(&o);
    anim.setPropertyName("x");
    anim.setDuration(30);

    anim.setEndValue(4000);
    anim.start();
    QTest::qWait(anim.duration() + o.animation.duration());
    QTRY_COMPARE(anim.state(), QAbstractAnimation::Stopped);
    QTRY_COMPARE(o.animation.state(), QAbstractAnimation::Stopped);
    QCOMPARE(o.y(), qreal(4000));
}


QTEST_MAIN(tst_QPropertyAnimation)
#include "tst_qpropertyanimation.moc"
