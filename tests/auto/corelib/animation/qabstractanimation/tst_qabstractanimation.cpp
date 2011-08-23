/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/qabstractanimation.h>
#include <QtCore/qanimationgroup.h>
#include <QtTest>

class tst_QAbstractAnimation : public QObject
{
  Q_OBJECT
public:
    tst_QAbstractAnimation() {};
    virtual ~tst_QAbstractAnimation() {};

public Q_SLOTS:
    void init();
    void cleanup();

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
};

class TestableQAbstractAnimation : public QAbstractAnimation
{
    Q_OBJECT

public:
    virtual ~TestableQAbstractAnimation() {};

    int duration() const { return 10; }
    virtual void updateCurrentTime(int) {}
};

class DummyQAnimationGroup : public QAnimationGroup
{
    Q_OBJECT
public:
    int duration() const { return 10; }
    virtual void updateCurrentTime(int) {}
};

void tst_QAbstractAnimation::init()
{
}

void tst_QAbstractAnimation::cleanup()
{
}

void tst_QAbstractAnimation::construction()
{
    TestableQAbstractAnimation anim;
}

void tst_QAbstractAnimation::destruction()
{
    TestableQAbstractAnimation *anim = new TestableQAbstractAnimation;
    delete anim;
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

QTEST_MAIN(tst_QAbstractAnimation)

#include "tst_qabstractanimation.moc"
