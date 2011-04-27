/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include "qstate.h"
#include "qstatemachine.h"
#include "qsignaltransition.h"

// Will try to wait for the condition while allowing event processing
#define QTRY_COMPARE(__expr, __expected) \
    do { \
        const int __step = 50; \
        const int __timeout = 5000; \
        if ((__expr) != (__expected)) { \
            QTest::qWait(0); \
        } \
        for (int __i = 0; __i < __timeout && ((__expr) != (__expected)); __i+=__step) { \
            QTest::qWait(__step); \
        } \
        QCOMPARE(__expr, __expected); \
    } while(0)

//TESTED_CLASS=
//TESTED_FILES=

class tst_QState : public QObject
{
    Q_OBJECT

public:
    tst_QState();
    virtual ~tst_QState();

private slots:
#if 0
    void test();
#endif
    void assignProperty();
    void assignPropertyTwice();
    void historyInitialState();
    void transitions();

private:
    bool functionCalled;
};

tst_QState::tst_QState() : functionCalled(false)
{
}

tst_QState::~tst_QState()
{
}

#if 0
void tst_QState::test()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);

    QCOMPARE(s1->machine(), &machine);
    QCOMPARE(s1->parentState(), &machine);
    QCOMPARE(s1->initialState(), (QState*)0);
    QVERIFY(s1->childStates().isEmpty());
    QVERIFY(s1->transitions().isEmpty());

    QCOMPARE(s1->isFinal(), false);
    s1->setFinal(true);
    QCOMPARE(s1->isFinal(), true);
    s1->setFinal(false);
    QCOMPARE(s1->isFinal(), false);

    QCOMPARE(s1->isParallel(), false);
    s1->setParallel(true);
    QCOMPARE(s1->isParallel(), true);
    s1->setParallel(false);
    QCOMPARE(s1->isParallel(), false);

    QCOMPARE(s1->isAtomic(), true);
    QCOMPARE(s1->isCompound(), false);
    QCOMPARE(s1->isComplex(), false);

    QState *s11 = new QState(s1);
    QCOMPARE(s11->parentState(), s1);
    QCOMPARE(s11->isAtomic(), true);
    QCOMPARE(s11->isCompound(), false);
    QCOMPARE(s11->isComplex(), false);
    QCOMPARE(s11->machine(), s1->machine());
    QVERIFY(s11->isDescendantOf(s1));

    QCOMPARE(s1->initialState(), (QState*)0);
    QCOMPARE(s1->childStates().size(), 1);
    QCOMPARE(s1->childStates().at(0), s11);

    QCOMPARE(s1->isAtomic(), false);
    QCOMPARE(s1->isCompound(), true);
    QCOMPARE(s1->isComplex(), true);

    s1->setParallel(true);
    QCOMPARE(s1->isAtomic(), false);
    QCOMPARE(s1->isCompound(), false);
    QCOMPARE(s1->isComplex(), true);

    QState *s12 = new QState(s1);
    QCOMPARE(s12->parentState(), s1);
    QCOMPARE(s12->isAtomic(), true);
    QCOMPARE(s12->isCompound(), false);
    QCOMPARE(s12->isComplex(), false);
    QCOMPARE(s12->machine(), s1->machine());
    QVERIFY(s12->isDescendantOf(s1));
    QVERIFY(!s12->isDescendantOf(s11));

    QCOMPARE(s1->initialState(), (QState*)0);
    QCOMPARE(s1->childStates().size(), 2);
    QCOMPARE(s1->childStates().at(0), s11);
    QCOMPARE(s1->childStates().at(1), s12);

    QCOMPARE(s1->isAtomic(), false);
    QCOMPARE(s1->isCompound(), false);
    QCOMPARE(s1->isComplex(), true);

    s1->setParallel(false);
    QCOMPARE(s1->isAtomic(), false);
    QCOMPARE(s1->isCompound(), true);
    QCOMPARE(s1->isComplex(), true);

    s1->setInitialState(s11);
    QCOMPARE(s1->initialState(), s11);

    s1->setInitialState(0);
    QCOMPARE(s1->initialState(), (QState*)0);

    s1->setInitialState(s12);
    QCOMPARE(s1->initialState(), s12);

    QState *s13 = new QState();
    s1->setInitialState(s13);
    QCOMPARE(s13->parentState(), s1);
    QCOMPARE(s1->childStates().size(), 3);
    QCOMPARE(s1->childStates().at(0), s11);
    QCOMPARE(s1->childStates().at(1), s12);
    QCOMPARE(s1->childStates().at(2), s13);
    QVERIFY(s13->isDescendantOf(s1));

    QVERIFY(s12->childStates().isEmpty());

    QState *s121 = new QState(s12);
    QCOMPARE(s121->parentState(), s12);
    QCOMPARE(s121->isAtomic(), true);
    QCOMPARE(s121->isCompound(), false);
    QCOMPARE(s121->isComplex(), false);
    QCOMPARE(s121->machine(), s12->machine());
    QVERIFY(s121->isDescendantOf(s12));
    QVERIFY(s121->isDescendantOf(s1));
    QVERIFY(!s121->isDescendantOf(s11));

    QCOMPARE(s12->childStates().size(), 1);
    QCOMPARE(s12->childStates().at(0), (QState*)s121);

    QCOMPARE(s1->childStates().size(), 3);
    QCOMPARE(s1->childStates().at(0), s11);
    QCOMPARE(s1->childStates().at(1), s12);
    QCOMPARE(s1->childStates().at(2), s13);

    s11->addTransition(s12);
    QCOMPARE(s11->transitions().size(), 1);
    QCOMPARE(s11->transitions().at(0)->sourceState(), s11);
    QCOMPARE(s11->transitions().at(0)->targetStates().size(), 1);
    QCOMPARE(s11->transitions().at(0)->targetStates().at(0), s12);
    QCOMPARE(s11->transitions().at(0)->eventType(), QEvent::None);

    QState *s14 = new QState();
    s12->addTransition(QList<QState*>() << s13 << s14);
    QCOMPARE(s12->transitions().size(), 1);
    QCOMPARE(s12->transitions().at(0)->sourceState(), s12);
    QCOMPARE(s12->transitions().at(0)->targetStates().size(), 2);
    QCOMPARE(s12->transitions().at(0)->targetStates().at(0), s13);
    QCOMPARE(s12->transitions().at(0)->targetStates().at(1), s14);
    QCOMPARE(s12->transitions().at(0)->eventType(), QEvent::None);

    s13->addTransition(this, SIGNAL(destroyed()), s14);
    QCOMPARE(s13->transitions().size(), 1);
    QCOMPARE(s13->transitions().at(0)->sourceState(), s13);
    QCOMPARE(s13->transitions().at(0)->targetStates().size(), 1);
    QCOMPARE(s13->transitions().at(0)->targetStates().at(0), s14);
    QCOMPARE(s13->transitions().at(0)->eventType(), QEvent::Signal);
    QVERIFY(qobject_cast<QSignalTransition*>(s13->transitions().at(0)) != 0);

    delete s13->transitions().at(0);
    QCOMPARE(s13->transitions().size(), 0);

    s12->addTransition(this, SIGNAL(destroyed()), s11);
    QCOMPARE(s12->transitions().size(), 2);
}
#endif

class TestClass: public QObject
{
    Q_OBJECT
public:
    TestClass() : called(false) {}
    bool called;

public slots:
    void slot() { called = true; }

    
};

void tst_QState::assignProperty()
{
    QStateMachine machine;

    QObject *object = new QObject();
    object->setProperty("fooBar", 10);

    QState *s1 = new QState(&machine);
    s1->assignProperty(object, "fooBar", 20);
    
    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(object->property("fooBar").toInt(), 20);
}

void tst_QState::assignPropertyTwice()
{
    QStateMachine machine;

    QObject *object = new QObject();
    object->setProperty("fooBar", 10);

    QState *s1 = new QState(&machine);
    s1->assignProperty(object, "fooBar", 20);
    s1->assignProperty(object, "fooBar", 30);
    
    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(object->property("fooBar").toInt(), 30);
}

class EventTestTransition: public QAbstractTransition
{
public:
    EventTestTransition(QEvent::Type type, QState *targetState) 
        : QAbstractTransition(), m_type(type)
    {
        setTargetState(targetState);
    }

protected:
    bool eventTest(QEvent *e)
    {
        return e->type() == m_type;
    }

    void onTransition(QEvent *) {}

private:
    QEvent::Type m_type;
    
};

void tst_QState::historyInitialState() 
{
    QStateMachine machine;

    QState *s1 = new QState(&machine);
    
    QState *s2 = new QState(&machine);
    QHistoryState *h1 = new QHistoryState(s2);
    
    s2->setInitialState(h1);

    QState *s3 = new QState(s2);
    h1->setDefaultState(s3);

    QState *s4 = new QState(s2);

    s1->addTransition(new EventTestTransition(QEvent::User, s2));
    s2->addTransition(new EventTestTransition(QEvent::User, s1));
    s3->addTransition(new EventTestTransition(QEvent::Type(QEvent::User+1), s4));

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s3));

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s3));

    machine.postEvent(new QEvent(QEvent::Type(QEvent::User+1)));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s4));

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s4));
}

void tst_QState::transitions()
{
    QState s1;
    QState s2;

    QVERIFY(s1.transitions().isEmpty());

    QAbstractTransition *t1 = s1.addTransition(this, SIGNAL(destroyed()), &s2);
    QVERIFY(t1 != 0);
    QCOMPARE(s1.transitions().count(), 1);
    QCOMPARE(s1.transitions().first(), t1);
    QVERIFY(s2.transitions().isEmpty());

    s1.removeTransition(t1);
    QVERIFY(s1.transitions().isEmpty());

    s1.addTransition(t1);
    QCOMPARE(s1.transitions().count(), 1);
    QCOMPARE(s1.transitions().first(), t1);

    QAbstractTransition *t2 = new QEventTransition(&s1);
    QCOMPARE(s1.transitions().count(), 2);
    QVERIFY(s1.transitions().contains(t1));
    QVERIFY(s1.transitions().contains(t2));

    // Transitions from child states should not be reported.
    QState *s21 = new QState(&s2);
    QAbstractTransition *t3 = s21->addTransition(this, SIGNAL(destroyed()), &s2);
    QVERIFY(s2.transitions().isEmpty());
    QCOMPARE(s21->transitions().count(), 1);
    QCOMPARE(s21->transitions().first(), t3);
}

QTEST_MAIN(tst_QState)
#include "tst_qstate.moc"
