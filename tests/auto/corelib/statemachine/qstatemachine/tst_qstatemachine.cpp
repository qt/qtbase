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
#include <QtCore/QCoreApplication>
#ifndef QT_NO_WIDGETS
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneEvent>
#include <QtWidgets/QGraphicsTextItem>
#endif

#include "qstatemachine.h"
#include "qstate.h"
#include "qhistorystate.h"
#ifndef QT_NO_WIDGETS
#include "qkeyeventtransition.h"
#include "qmouseeventtransition.h"
#endif
#include "private/qstate_p.h"
#include "private/qstatemachine_p.h"

static int globalTick;

// Run exec for a maximum of TIMEOUT msecs
#define QCOREAPPLICATION_EXEC(TIMEOUT) \
{ \
    QTimer timer; \
    timer.setSingleShot(true); \
    timer.setInterval(TIMEOUT); \
    timer.start(); \
    connect(&timer, SIGNAL(timeout()), QCoreApplication::instance(), SLOT(quit())); \
    QCoreApplication::exec(); \
}

class SignalEmitter : public QObject
{
Q_OBJECT
    public:
    SignalEmitter(QObject *parent = 0)
        : QObject(parent) {}
    void emitSignalWithNoArg()
        { emit signalWithNoArg(); }
    void emitSignalWithIntArg(int arg)
        { emit signalWithIntArg(arg); }
    void emitSignalWithStringArg(const QString &arg)
        { emit signalWithStringArg(arg); }
    void emitSignalWithDefaultArg()
        { emit signalWithDefaultArg(); }
Q_SIGNALS:
    void signalWithNoArg();
    void signalWithIntArg(int);
    void signalWithStringArg(const QString &);
    void signalWithDefaultArg(int i = 42);
};

class tst_QStateMachine : public QObject
{
    Q_OBJECT
private slots:
    void rootState();
    void machineWithParent();
#ifdef QT_BUILD_INTERNAL
    void addAndRemoveState();
#endif
    void stateEntryAndExit();
    void assignProperty();
    void assignPropertyWithAnimation();
    void postEvent();
    void cancelDelayedEvent();
    void postDelayedEventAndStop();
    void postDelayedEventFromThread();
    void stopAndPostEvent();
    void stateFinished();
    void parallelStates();
    void parallelRootState();
    void allSourceToTargetConfigurations();
    void signalTransitions();
#ifndef QT_NO_WIDGETS
    void eventTransitions();
    void graphicsSceneEventTransitions();
#endif
    void historyStates();
    void startAndStop();
    void targetStateWithNoParent();
    void targetStateDeleted();
    void transitionToRootState();
    void transitionFromRootState();
    void transitionEntersParent();

    void defaultErrorState();
    void customGlobalErrorState();
    void customLocalErrorStateInBrokenState();
    void customLocalErrorStateInOtherState();
    void customLocalErrorStateInParentOfBrokenState();
    void customLocalErrorStateOverridesParent();
    void errorStateHasChildren();
    void errorStateHasErrors();
    void errorStateIsRootState();
    void errorStateEntersParentFirst();
    void customErrorStateIsNull();
    void clearError();
    void historyStateHasNowhereToGo();
    void historyStateAsInitialState();
    void historyStateAfterRestart();
    void brokenStateIsNeverEntered();
    void customErrorStateNotInGraph();
    void transitionToStateNotInGraph();
    void restoreProperties();

    void defaultGlobalRestorePolicy();
    void globalRestorePolicySetToRestore();
    void globalRestorePolicySetToDontRestore();

    void noInitialStateForInitialState();

    void transitionWithParent();
    void transitionsFromParallelStateWithNoChildren();
    void parallelStateTransition();
    void parallelStateAssignmentsDone();
    void nestedRestoreProperties();
    void nestedRestoreProperties2();

    void simpleAnimation();
    void twoAnimations();
    void twoAnimatedTransitions();
    void playAnimationTwice();
    void nestedTargetStateForAnimation();
    void propertiesAssignedSignalTransitionsReuseAnimationGroup();
    void animatedGlobalRestoreProperty();
    void specificTargetValueOfAnimation();

    void addDefaultAnimation();
    void addDefaultAnimationWithUnusedAnimation();
    void removeDefaultAnimation();
    void overrideDefaultAnimationWithSpecific();

    void nestedStateMachines();
    void goToState();
    void goToStateFromSourceWithTransition();

    void clonedSignals();
    void postEventFromOtherThread();
#ifndef QT_NO_WIDGETS
    void eventFilterForApplication();
#endif
    void eventClassesExported();
    void stopInTransitionToFinalState();
    void stopInEventTest_data();
    void stopInEventTest();

    void testIncrementReceivers();
    void initialStateIsEnteredBeforeStartedEmitted();
    void deletePropertyAssignmentObjectBeforeEntry();
    void deletePropertyAssignmentObjectBeforeRestore();
    void deleteInitialState();
    void setPropertyAfterRestore();
    void transitionWithNoTarget_data();
    void transitionWithNoTarget();
    void initialStateIsFinal();

    void restorePropertiesSimple();
    void restoreProperties2();
    void restoreProperties3();
    void restoreProperties4();
    void restorePropertiesSelfTransition();
    void changeStateWhileAnimatingProperty();
    void propertiesAreAssignedBeforeEntryCallbacks_data();
    void propertiesAreAssignedBeforeEntryCallbacks();

    void multiTargetTransitionInsideParallelStateGroup();
    void signalTransitionNormalizeSignature();
    void createSignalTransitionWhenRunning();
    void createEventTransitionWhenRunning();
    void signalTransitionSenderInDifferentThread();
    void signalTransitionSenderInDifferentThread2();
    void signalTransitionRegistrationThreadSafety();
    void childModeConstructor();
};

class TestState : public QState
{
public:
    enum Event {
        Entry,
        Exit
    };
    TestState(QState *parent)
        : QState(parent) {}
    TestState(ChildMode mode)
        : QState(mode) {}
    QList<QPair<int, Event> > events;
protected:
    virtual void onEntry(QEvent *) {
        events.append(qMakePair(globalTick++, Entry));
    }
    virtual void onExit(QEvent *) {
        events.append(qMakePair(globalTick++, Exit));
    }
};

class TestTransition : public QAbstractTransition
{
public:
    TestTransition(QAbstractState *target)
        : QAbstractTransition()
    { setTargetState(target); }
    QList<int> triggers;
protected:
    virtual bool eventTest(QEvent *) {
        return true;
    }
    virtual void onTransition(QEvent *) {
        triggers.append(globalTick++);
    }
};

class EventTransition : public QAbstractTransition
{
public:
    EventTransition(QEvent::Type type, QAbstractState *target, QState *parent = 0)
        : QAbstractTransition(parent), m_type(type)
    { setTargetState(target); }
    EventTransition(QEvent::Type type, const QList<QAbstractState *> &targets, QState *parent = 0)
        : QAbstractTransition(parent), m_type(type)
    { setTargetStates(targets); }
protected:
    virtual bool eventTest(QEvent *e) {
        return (e->type() == m_type);
    }
    virtual void onTransition(QEvent *) {}
private:
    QEvent::Type m_type;
};

void tst_QStateMachine::transitionToRootState()
{
    QStateMachine machine;
    machine.setObjectName("machine");

    QState *initialState = new QState();
    initialState->setObjectName("initial");
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QAbstractTransition *trans = new EventTransition(QEvent::User, &machine);
    initialState->addTransition(trans);
    QCOMPARE(trans->sourceState(), initialState);
    QCOMPARE(trans->targetState(), static_cast<QAbstractState *>(&machine));

    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(initialState));

    machine.postEvent(new QEvent(QEvent::User));
    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: No common ancestor for targets and source of transition from state 'initial'");
    QCoreApplication::processEvents();
    QVERIFY(machine.configuration().isEmpty());
    QVERIFY(!machine.isRunning());
}

void tst_QStateMachine::transitionFromRootState()
{
    QStateMachine machine;
    QState *root = &machine;
    QState *s1 = new QState(root);
    EventTransition *trans = new EventTransition(QEvent::User, s1);
    root->addTransition(trans);
    QCOMPARE(trans->sourceState(), root);
    QCOMPARE(trans->targetState(), static_cast<QAbstractState *>(s1));
}

void tst_QStateMachine::transitionEntersParent()
{
    QStateMachine machine;

    QObject *entryController = new QObject(&machine);
    entryController->setObjectName("entryController");
    entryController->setProperty("greatGrandParentEntered", false);
    entryController->setProperty("grandParentEntered", false);
    entryController->setProperty("parentEntered", false);
    entryController->setProperty("stateEntered", false);

    QState *greatGrandParent = new QState();
    greatGrandParent->setObjectName("grandParent");
    greatGrandParent->assignProperty(entryController, "greatGrandParentEntered", true);
    machine.addState(greatGrandParent);
    machine.setInitialState(greatGrandParent);

    QState *grandParent = new QState(greatGrandParent);
    grandParent->setObjectName("grandParent");
    grandParent->assignProperty(entryController, "grandParentEntered", true);

    QState *parent = new QState(grandParent);
    parent->setObjectName("parent");
    parent->assignProperty(entryController, "parentEntered", true);

    QState *state = new QState(parent);
    state->setObjectName("state");
    state->assignProperty(entryController, "stateEntered", true);

    QState *initialStateOfGreatGrandParent = new QState(greatGrandParent);
    initialStateOfGreatGrandParent->setObjectName("initialStateOfGreatGrandParent");
    greatGrandParent->setInitialState(initialStateOfGreatGrandParent);

    initialStateOfGreatGrandParent->addTransition(new EventTransition(QEvent::User, state));

    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(entryController->property("greatGrandParentEntered").toBool(), true);
    QCOMPARE(entryController->property("grandParentEntered").toBool(), false);
    QCOMPARE(entryController->property("parentEntered").toBool(), false);
    QCOMPARE(entryController->property("stateEntered").toBool(), false);
    QCOMPARE(machine.configuration().count(), 2);
    QVERIFY(machine.configuration().contains(greatGrandParent));
    QVERIFY(machine.configuration().contains(initialStateOfGreatGrandParent));

    entryController->setProperty("greatGrandParentEntered", false);
    entryController->setProperty("grandParentEntered", false);
    entryController->setProperty("parentEntered", false);
    entryController->setProperty("stateEntered", false);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(entryController->property("greatGrandParentEntered").toBool(), false);
    QCOMPARE(entryController->property("grandParentEntered").toBool(), true);
    QCOMPARE(entryController->property("parentEntered").toBool(), true);
    QCOMPARE(entryController->property("stateEntered").toBool(), true);
    QCOMPARE(machine.configuration().count(), 4);
    QVERIFY(machine.configuration().contains(greatGrandParent));
    QVERIFY(machine.configuration().contains(grandParent));
    QVERIFY(machine.configuration().contains(parent));
    QVERIFY(machine.configuration().contains(state));
}

void tst_QStateMachine::defaultErrorState()
{
    QStateMachine machine;
    QCOMPARE(machine.errorState(), reinterpret_cast<QAbstractState *>(0));

    QState *brokenState = new QState();
    brokenState->setObjectName("MyInitialState");

    machine.addState(brokenState);
    machine.setInitialState(brokenState);

    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: Missing initial state in compound state 'MyInitialState'");

    // initialState has no initial state
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.error(), QStateMachine::NoInitialStateError);
    QCOMPARE(machine.errorString(), QString::fromLatin1("Missing initial state in compound state 'MyInitialState'"));
    QCOMPARE(machine.isRunning(), false);
}

class CustomErrorState: public QState
{
public:
    CustomErrorState(QStateMachine *machine, QState *parent = 0)
        : QState(parent), error(QStateMachine::NoError), m_machine(machine)
    {
    }

    void onEntry(QEvent *)
    {
        error = m_machine->error();
        errorString = m_machine->errorString();
    }

    QStateMachine::Error error;
    QString errorString;

private:
    QStateMachine *m_machine;
};

void tst_QStateMachine::customGlobalErrorState()
{
    QStateMachine machine;

    CustomErrorState *customErrorState = new CustomErrorState(&machine);
    customErrorState->setObjectName("customErrorState");
    machine.addState(customErrorState);
    machine.setErrorState(customErrorState);

    QState *initialState = new QState();
    initialState->setObjectName("initialState");
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QState *brokenState = new QState();
    brokenState->setObjectName("brokenState");
    machine.addState(brokenState);
    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    initialState->addTransition(new EventTransition(QEvent::Type(QEvent::User + 1), brokenState));
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.errorState(), static_cast<QAbstractState*>(customErrorState));
    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(initialState));

    machine.postEvent(new QEvent(QEvent::Type(QEvent::User + 1)));
    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(initialState));

    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), true);
    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(customErrorState));
    QCOMPARE(customErrorState->error, QStateMachine::NoInitialStateError);
    QCOMPARE(customErrorState->errorString, QString::fromLatin1("Missing initial state in compound state 'brokenState'"));
    QCOMPARE(machine.error(), QStateMachine::NoInitialStateError);
    QCOMPARE(machine.errorString(), QString::fromLatin1("Missing initial state in compound state 'brokenState'"));
}

void tst_QStateMachine::customLocalErrorStateInBrokenState()
{
    QStateMachine machine;
    CustomErrorState *customErrorState = new CustomErrorState(&machine);
    machine.addState(customErrorState);

    QState *initialState = new QState();
    initialState->setObjectName("initialState");
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QState *brokenState = new QState();
    brokenState->setObjectName("brokenState");
    machine.addState(brokenState);
    brokenState->setErrorState(customErrorState);

    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    initialState->addTransition(new EventTransition(QEvent::Type(QEvent::User + 1), brokenState));

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), true);
    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(customErrorState));
    QCOMPARE(customErrorState->error, QStateMachine::NoInitialStateError);
}

void tst_QStateMachine::customLocalErrorStateInOtherState()
{
    QStateMachine machine;
    CustomErrorState *customErrorState = new CustomErrorState(&machine);
    machine.addState(customErrorState);

    QState *initialState = new QState();
    initialState->setObjectName("initialState");
    QTest::ignoreMessage(QtWarningMsg, "QState::setErrorState: error state cannot belong to a different state machine");
    initialState->setErrorState(customErrorState);
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QState *brokenState = new QState();
    brokenState->setObjectName("brokenState");

    machine.addState(brokenState);

    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    initialState->addTransition(new EventTransition(QEvent::Type(QEvent::User + 1), brokenState));

    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: Missing initial state in compound state 'brokenState'");
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), false);
}

void tst_QStateMachine::customLocalErrorStateInParentOfBrokenState()
{
    QStateMachine machine;
    CustomErrorState *customErrorState = new CustomErrorState(&machine);
    machine.addState(customErrorState);

    QState *initialState = new QState();
    initialState->setObjectName("initialState");
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QState *parentOfBrokenState = new QState();
    machine.addState(parentOfBrokenState);
    parentOfBrokenState->setObjectName("parentOfBrokenState");
    parentOfBrokenState->setErrorState(customErrorState);

    QState *brokenState = new QState(parentOfBrokenState);
    brokenState->setObjectName("brokenState");
    parentOfBrokenState->setInitialState(brokenState);

    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    initialState->addTransition(new EventTransition(QEvent::Type(QEvent::User + 1), brokenState));

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), true);
    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(customErrorState));
}

void tst_QStateMachine::customLocalErrorStateOverridesParent()
{
    QStateMachine machine;
    CustomErrorState *customErrorStateForParent = new CustomErrorState(&machine);
    machine.addState(customErrorStateForParent);

    CustomErrorState *customErrorStateForBrokenState = new CustomErrorState(&machine);
    machine.addState(customErrorStateForBrokenState);

    QState *initialState = new QState();
    initialState->setObjectName("initialState");
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QState *parentOfBrokenState = new QState();
    machine.addState(parentOfBrokenState);
    parentOfBrokenState->setObjectName("parentOfBrokenState");
    parentOfBrokenState->setErrorState(customErrorStateForParent);

    QState *brokenState = new QState(parentOfBrokenState);
    brokenState->setObjectName("brokenState");
    brokenState->setErrorState(customErrorStateForBrokenState);
    parentOfBrokenState->setInitialState(brokenState);

    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    initialState->addTransition(new EventTransition(QEvent::Type(QEvent::User + 1), brokenState));

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(customErrorStateForBrokenState));
    QCOMPARE(customErrorStateForBrokenState->error, QStateMachine::NoInitialStateError);
    QCOMPARE(customErrorStateForParent->error, QStateMachine::NoError);
}

void tst_QStateMachine::errorStateHasChildren()
{
    QStateMachine machine;
    CustomErrorState *customErrorState = new CustomErrorState(&machine);
    customErrorState->setObjectName("customErrorState");
    machine.addState(customErrorState);

    machine.setErrorState(customErrorState);

    QState *childOfErrorState = new QState(customErrorState);
    childOfErrorState->setObjectName("childOfErrorState");
    customErrorState->setInitialState(childOfErrorState);

    QState *initialState = new QState();
    initialState->setObjectName("initialState");
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QState *brokenState = new QState();
    brokenState->setObjectName("brokenState");
    machine.addState(brokenState);

    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    initialState->addTransition(new EventTransition(QEvent::Type(QEvent::User + 1), brokenState));

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), true);
    QCOMPARE(machine.configuration().count(), 2);
    QVERIFY(machine.configuration().contains(customErrorState));
    QVERIFY(machine.configuration().contains(childOfErrorState));
}


void tst_QStateMachine::errorStateHasErrors()
{
    QStateMachine machine;
    CustomErrorState *customErrorState = new CustomErrorState(&machine);
    customErrorState->setObjectName("customErrorState");
    machine.addState(customErrorState);

    machine.setErrorState(customErrorState);

    QState *childOfErrorState = new QState(customErrorState);
    childOfErrorState->setObjectName("childOfErrorState");

    QState *initialState = new QState();
    initialState->setObjectName("initialState");
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QState *brokenState = new QState();
    brokenState->setObjectName("brokenState");
    machine.addState(brokenState);

    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    initialState->addTransition(new EventTransition(QEvent::Type(QEvent::User + 1), brokenState));

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::Type(QEvent::User + 1)));
    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: Missing initial state in compound state 'customErrorState'");
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), false);
    QCOMPARE(machine.error(), QStateMachine::NoInitialStateError);
    QCOMPARE(machine.errorString(), QString::fromLatin1("Missing initial state in compound state 'customErrorState'"));
}

void tst_QStateMachine::errorStateIsRootState()
{
    QStateMachine machine;
    QTest::ignoreMessage(QtWarningMsg, "QStateMachine::setErrorState: root state cannot be error state");
    machine.setErrorState(&machine);

    QState *initialState = new QState();
    initialState->setObjectName("initialState");
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QState *brokenState = new QState();
    brokenState->setObjectName("brokenState");
    machine.addState(brokenState);

    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    initialState->addTransition(new EventTransition(QEvent::Type(QEvent::User + 1), brokenState));

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::Type(QEvent::User + 1)));
    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: Missing initial state in compound state 'brokenState'");
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), false);
}

void tst_QStateMachine::errorStateEntersParentFirst()
{
    QStateMachine machine;

    QObject *entryController = new QObject(&machine);
    entryController->setObjectName("entryController");
    entryController->setProperty("greatGrandParentEntered", false);
    entryController->setProperty("grandParentEntered", false);
    entryController->setProperty("parentEntered", false);
    entryController->setProperty("errorStateEntered", false);

    QState *greatGrandParent = new QState();
    greatGrandParent->setObjectName("greatGrandParent");
    greatGrandParent->assignProperty(entryController, "greatGrandParentEntered", true);
    machine.addState(greatGrandParent);
    machine.setInitialState(greatGrandParent);

    QState *grandParent = new QState(greatGrandParent);
    grandParent->setObjectName("grandParent");
    grandParent->assignProperty(entryController, "grandParentEntered", true);

    QState *parent = new QState(grandParent);
    parent->setObjectName("parent");
    parent->assignProperty(entryController, "parentEntered", true);

    QState *errorState = new QState(parent);
    errorState->setObjectName("errorState");
    errorState->assignProperty(entryController, "errorStateEntered", true);
    machine.setErrorState(errorState);

    QState *initialStateOfGreatGrandParent = new QState(greatGrandParent);
    initialStateOfGreatGrandParent->setObjectName("initialStateOfGreatGrandParent");
    greatGrandParent->setInitialState(initialStateOfGreatGrandParent);

    QState *brokenState = new QState(greatGrandParent);
    brokenState->setObjectName("brokenState");

    QState *childState = new QState(brokenState);
    childState->setObjectName("childState");

    initialStateOfGreatGrandParent->addTransition(new EventTransition(QEvent::User, brokenState));

    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(entryController->property("greatGrandParentEntered").toBool(), true);
    QCOMPARE(entryController->property("grandParentEntered").toBool(), false);
    QCOMPARE(entryController->property("parentEntered").toBool(), false);
    QCOMPARE(entryController->property("errorStateEntered").toBool(), false);
    QCOMPARE(machine.configuration().count(), 2);
    QVERIFY(machine.configuration().contains(greatGrandParent));
    QVERIFY(machine.configuration().contains(initialStateOfGreatGrandParent));

    entryController->setProperty("greatGrandParentEntered", false);
    entryController->setProperty("grandParentEntered", false);
    entryController->setProperty("parentEntered", false);
    entryController->setProperty("errorStateEntered", false);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(entryController->property("greatGrandParentEntered").toBool(), false);
    QCOMPARE(entryController->property("grandParentEntered").toBool(), true);
    QCOMPARE(entryController->property("parentEntered").toBool(), true);
    QCOMPARE(entryController->property("errorStateEntered").toBool(), true);
    QCOMPARE(machine.configuration().count(), 4);
    QVERIFY(machine.configuration().contains(greatGrandParent));
    QVERIFY(machine.configuration().contains(grandParent));
    QVERIFY(machine.configuration().contains(parent));
    QVERIFY(machine.configuration().contains(errorState));
}

void tst_QStateMachine::customErrorStateIsNull()
{
    QStateMachine machine;
    machine.setErrorState(0);

    QState *initialState = new QState();
    machine.addState(initialState);
    machine.setInitialState(initialState);

    QState *brokenState = new QState();
    machine.addState(brokenState);

    new QState(brokenState);
    initialState->addTransition(new EventTransition(QEvent::User, brokenState));

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: Missing initial state in compound state ''");
    QCoreApplication::processEvents();

    QCOMPARE(machine.errorState(), reinterpret_cast<QAbstractState *>(0));
    QCOMPARE(machine.isRunning(), false);
}

void tst_QStateMachine::clearError()
{
    QStateMachine machine;
    machine.setErrorState(new QState(&machine)); // avoid warnings

    QState *brokenState = new QState(&machine);
    brokenState->setObjectName("brokenState");
    machine.setInitialState(brokenState);
    new QState(brokenState);

    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), true);
    QCOMPARE(machine.error(), QStateMachine::NoInitialStateError);
    QCOMPARE(machine.errorString(), QString::fromLatin1("Missing initial state in compound state 'brokenState'"));

    machine.clearError();

    QCOMPARE(machine.error(), QStateMachine::NoError);
    QVERIFY(machine.errorString().isEmpty());
}

void tst_QStateMachine::historyStateAsInitialState()
{
    QStateMachine machine;

    QHistoryState *hs = new QHistoryState(&machine);
    machine.setInitialState(hs);

    QState *s1 = new QState(&machine);
    hs->setDefaultState(s1);

    QState *s2 = new QState(&machine);

    QHistoryState *s2h = new QHistoryState(s2);
    s2->setInitialState(s2h);

    QState *s21 = new QState(s2);
    s2h->setDefaultState(s21);

    s1->addTransition(new EventTransition(QEvent::User, s2));

    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s21));
}

void tst_QStateMachine::historyStateHasNowhereToGo()
{
    QStateMachine machine;

    QState *initialState = new QState(&machine);
    machine.setInitialState(initialState);
    machine.setErrorState(new QState(&machine)); // avoid warnings

    QState *brokenState = new QState(&machine);
    brokenState->setObjectName("brokenState");
    brokenState->setInitialState(new QState(brokenState));

    QHistoryState *historyState = new QHistoryState(brokenState);
    historyState->setObjectName("historyState");
    initialState->addTransition(new EventTransition(QEvent::User, historyState));

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), true);
    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(machine.errorState()));
    QCOMPARE(machine.error(), QStateMachine::NoDefaultStateInHistoryStateError);
    QCOMPARE(machine.errorString(), QString::fromLatin1("Missing default state in history state 'historyState'"));
}

void tst_QStateMachine::historyStateAfterRestart()
{
    // QTBUG-8842
    QStateMachine machine;

    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    QState *s2 = new QState(&machine);
    QState *s21 = new QState(s2);
    QState *s22 = new QState(s2);
    QHistoryState *s2h = new QHistoryState(s2);
    s2h->setDefaultState(s21);
    s1->addTransition(new EventTransition(QEvent::User, s2h));
    s21->addTransition(new EventTransition(QEvent::User, s22));
    s2->addTransition(new EventTransition(QEvent::User, s1));

    for (int x = 0; x < 2; ++x) {
        QSignalSpy startedSpy(&machine, SIGNAL(started()));
        QVERIFY(startedSpy.isValid());
        machine.start();
        QTRY_COMPARE(startedSpy.count(), 1);
        QCOMPARE(machine.configuration().count(), 1);
        QVERIFY(machine.configuration().contains(s1));

        // s1 -> s2h -> s21 (default state)
        machine.postEvent(new QEvent(QEvent::User));
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().count(), 2);
        QVERIFY(machine.configuration().contains(s2));
        // This used to fail on the 2nd run because the
        // history had not been cleared.
        QVERIFY(machine.configuration().contains(s21));

        // s21 -> s22
        machine.postEvent(new QEvent(QEvent::User));
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().count(), 2);
        QVERIFY(machine.configuration().contains(s2));
        QVERIFY(machine.configuration().contains(s22));

        // s2 -> s1 (s22 saved in s2h)
        machine.postEvent(new QEvent(QEvent::User));
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().count(), 1);
        QVERIFY(machine.configuration().contains(s1));

        // s1 -> s2h -> s22 (saved state)
        machine.postEvent(new QEvent(QEvent::User));
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().count(), 2);
        QVERIFY(machine.configuration().contains(s2));
        QVERIFY(machine.configuration().contains(s22));

        QSignalSpy stoppedSpy(&machine, SIGNAL(stopped()));
        QVERIFY(stoppedSpy.isValid());
        machine.stop();
        QTRY_COMPARE(stoppedSpy.count(), 1);
    }
}

void tst_QStateMachine::brokenStateIsNeverEntered()
{
    QStateMachine machine;

    QObject *entryController = new QObject(&machine);
    entryController->setProperty("brokenStateEntered", false);
    entryController->setProperty("childStateEntered", false);
    entryController->setProperty("errorStateEntered", false);

    QState *initialState = new QState(&machine);
    machine.setInitialState(initialState);

    QState *errorState = new QState(&machine);
    errorState->assignProperty(entryController, "errorStateEntered", true);
    machine.setErrorState(errorState);

    QState *brokenState = new QState(&machine);
    brokenState->assignProperty(entryController, "brokenStateEntered", true);
    brokenState->setObjectName("brokenState");

    QState *childState = new QState(brokenState);
    childState->assignProperty(entryController, "childStateEntered", true);

    initialState->addTransition(new EventTransition(QEvent::User, brokenState));

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(entryController->property("errorStateEntered").toBool(), true);
    QCOMPARE(entryController->property("brokenStateEntered").toBool(), false);
    QCOMPARE(entryController->property("childStateEntered").toBool(), false);
}

void tst_QStateMachine::transitionToStateNotInGraph()
{
    QStateMachine machine;

    QState *initialState = new QState(&machine);
    initialState->setObjectName("initialState");
    machine.setInitialState(initialState);

    QState independentState;
    independentState.setObjectName("independentState");
    initialState->addTransition(&independentState);

    machine.start();
    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: No common ancestor for targets and source of transition from state 'initialState'");
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), false);
}

void tst_QStateMachine::customErrorStateNotInGraph()
{
    QStateMachine machine;

    QState errorState;
    errorState.setObjectName("errorState");
    QTest::ignoreMessage(QtWarningMsg, "QState::setErrorState: error state cannot belong to a different state machine");
    machine.setErrorState(&errorState);
    QCOMPARE(machine.errorState(), reinterpret_cast<QAbstractState *>(0));

    QState *initialBrokenState = new QState(&machine);
    initialBrokenState->setObjectName("initialBrokenState");
    machine.setInitialState(initialBrokenState);
    new QState(initialBrokenState);

    machine.start();
    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: Missing initial state in compound state 'initialBrokenState'");
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), false);
}

void tst_QStateMachine::restoreProperties()
{
    QStateMachine machine;
    QCOMPARE(machine.globalRestorePolicy(), QState::DontRestoreProperties);
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    QObject *object = new QObject(&machine);
    object->setProperty("a", 1);
    object->setProperty("b", 2);

    QState *S1 = new QState();
    S1->setObjectName("S1");
    S1->assignProperty(object, "a", 3);
    machine.addState(S1);

    QState *S2 = new QState();
    S2->setObjectName("S2");
    S2->assignProperty(object, "b", 5);
    machine.addState(S2);

    QState *S3 = new QState();
    S3->setObjectName("S3");
    machine.addState(S3);

    QFinalState *S4 = new QFinalState();
    machine.addState(S4);

    S1->addTransition(new EventTransition(QEvent::User, S2));
    S2->addTransition(new EventTransition(QEvent::User, S3));
    S3->addTransition(S4);

    machine.setInitialState(S1);
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(object->property("a").toInt(), 3);
    QCOMPARE(object->property("b").toInt(), 2);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(object->property("a").toInt(), 1);
    QCOMPARE(object->property("b").toInt(), 5);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(object->property("a").toInt(), 1);
    QCOMPARE(object->property("b").toInt(), 2);
}

void tst_QStateMachine::rootState()
{
    QStateMachine machine;
    QCOMPARE(qobject_cast<QState*>(machine.parentState()), (QState*)0);
    QCOMPARE(machine.machine(), (QStateMachine*)0);

    QState *s1 = new QState(&machine);
    QCOMPARE(s1->parentState(), static_cast<QState*>(&machine));

    QState *s2 = new QState();
    s2->setParent(&machine);
    QCOMPARE(s2->parentState(), static_cast<QState*>(&machine));
}

void tst_QStateMachine::machineWithParent()
{
    QObject object;
    QStateMachine *machine = new QStateMachine(&object);
    QCOMPARE(machine->parent(), &object);
    QCOMPARE(machine->parentState(), static_cast<QState*>(0));
}

#ifdef QT_BUILD_INTERNAL
void tst_QStateMachine::addAndRemoveState()
{
    QStateMachine machine;
    QStatePrivate *root_d = QStatePrivate::get(&machine);
    QCOMPARE(root_d->childStates().size(), 0);

    QTest::ignoreMessage(QtWarningMsg, "QStateMachine::addState: cannot add null state");
    machine.addState(0);

    QState *s1 = new QState();
    QCOMPARE(s1->parentState(), (QState*)0);
    QCOMPARE(s1->machine(), (QStateMachine*)0);
    machine.addState(s1);
    QCOMPARE(s1->machine(), static_cast<QStateMachine*>(&machine));
    QCOMPARE(s1->parentState(), static_cast<QState*>(&machine));
    QCOMPARE(root_d->childStates().size(), 1);
    QCOMPARE(root_d->childStates().at(0), (QAbstractState*)s1);

    QTest::ignoreMessage(QtWarningMsg, "QStateMachine::addState: state has already been added to this machine");
    machine.addState(s1);

    QState *s2 = new QState();
    QCOMPARE(s2->parentState(), (QState*)0);
    machine.addState(s2);
    QCOMPARE(s2->parentState(), static_cast<QState*>(&machine));
    QCOMPARE(root_d->childStates().size(), 2);
    QCOMPARE(root_d->childStates().at(0), (QAbstractState*)s1);
    QCOMPARE(root_d->childStates().at(1), (QAbstractState*)s2);

    QTest::ignoreMessage(QtWarningMsg, "QStateMachine::addState: state has already been added to this machine");
    machine.addState(s2);

    machine.removeState(s1);
    QCOMPARE(s1->parentState(), (QState*)0);
    QCOMPARE(root_d->childStates().size(), 1);
    QCOMPARE(root_d->childStates().at(0), (QAbstractState*)s2);

    machine.removeState(s2);
    QCOMPARE(s2->parentState(), (QState*)0);
    QCOMPARE(root_d->childStates().size(), 0);

    QTest::ignoreMessage(QtWarningMsg, "QStateMachine::removeState: cannot remove null state");
    machine.removeState(0);

    {
        QStateMachine machine2;
        {
            QString warning;
            warning.sprintf("QStateMachine::removeState: state %p's machine (%p) is different from this machine (%p)",
                            &machine2, (void*)0, &machine);
            QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
            machine.removeState(&machine2);
        }
        // ### check this behavior
        machine.addState(&machine2);
        QCOMPARE(machine2.parent(), (QObject*)&machine);
    }

    delete s1;
    delete s2;
    // ### how to deal with this?
    // machine.removeState(machine.errorState());
}
#endif

void tst_QStateMachine::stateEntryAndExit()
{
    // Two top-level states
    {
        QStateMachine machine;

        TestState *s1 = new TestState(&machine);
        QTest::ignoreMessage(QtWarningMsg, "QState::addTransition: cannot add transition to null state");
        s1->addTransition((QAbstractState*)0);
        QTest::ignoreMessage(QtWarningMsg, "QState::addTransition: cannot add null transition");
        s1->addTransition((QAbstractTransition*)0);
        QTest::ignoreMessage(QtWarningMsg, "QState::removeTransition: cannot remove null transition");
        s1->removeTransition((QAbstractTransition*)0);

        TestState *s2 = new TestState(&machine);
        QFinalState *s3 = new QFinalState(&machine);

        TestTransition *t = new TestTransition(s2);
        QCOMPARE(t->machine(), (QStateMachine*)0);
        QCOMPARE(t->sourceState(), (QState*)0);
        QCOMPARE(t->targetState(), (QAbstractState*)s2);
        QCOMPARE(t->targetStates().size(), 1);
        QCOMPARE(t->targetStates().at(0), (QAbstractState*)s2);
        t->setTargetState(0);
        QCOMPARE(t->targetState(), (QAbstractState*)0);
        QVERIFY(t->targetStates().isEmpty());
        t->setTargetState(s2);
        QCOMPARE(t->targetState(), (QAbstractState*)s2);
        QTest::ignoreMessage(QtWarningMsg, "QAbstractTransition::setTargetStates: target state(s) cannot be null");
        t->setTargetStates(QList<QAbstractState*>() << 0);
        QCOMPARE(t->targetState(), (QAbstractState*)s2);
        t->setTargetStates(QList<QAbstractState*>() << s2);
        QCOMPARE(t->targetState(), (QAbstractState*)s2);
        QCOMPARE(t->targetStates().size(), 1);
        QCOMPARE(t->targetStates().at(0), (QAbstractState*)s2);
        s1->addTransition(t);
        QCOMPARE(t->sourceState(), (QState*)s1);
        QCOMPARE(t->machine(), &machine);

        {
            QAbstractTransition *trans = s2->addTransition(s3);
            QVERIFY(trans != 0);
            QCOMPARE(trans->sourceState(), (QState*)s2);
            QCOMPARE(trans->targetState(), (QAbstractState*)s3);
            {
                QString warning;
                warning.sprintf("QState::removeTransition: transition %p's source state (%p) is different from this state (%p)", trans, s2, s1);
                QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
                s1->removeTransition(trans);
            }
            s2->removeTransition(trans);
            QCOMPARE(trans->sourceState(), (QState*)0);
            QCOMPARE(trans->targetState(), (QAbstractState*)s3);
            s2->addTransition(trans);
            QCOMPARE(trans->sourceState(), (QState*)s2);
        }

        QSignalSpy startedSpy(&machine, SIGNAL(started()));
        QSignalSpy stoppedSpy(&machine, SIGNAL(stopped()));
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));

        QVERIFY(startedSpy.isValid());
        QVERIFY(stoppedSpy.isValid());
        QVERIFY(finishedSpy.isValid());

        machine.setInitialState(s1);
        QCOMPARE(machine.initialState(), (QAbstractState*)s1);
        {
            QString warning;
            warning.sprintf("QState::setInitialState: state %p is not a child of this state (%p)", &machine, &machine);
            QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
            machine.setInitialState(&machine);
            QCOMPARE(machine.initialState(), (QAbstractState*)s1);
        }
        QVERIFY(machine.configuration().isEmpty());
        globalTick = 0;
        QVERIFY(!machine.isRunning());
        QSignalSpy s1EnteredSpy(s1, SIGNAL(entered()));
        QSignalSpy s1ExitedSpy(s1, SIGNAL(exited()));
        QSignalSpy tTriggeredSpy(t, SIGNAL(triggered()));
        QSignalSpy s2EnteredSpy(s2, SIGNAL(entered()));
        QSignalSpy s2ExitedSpy(s2, SIGNAL(exited()));

        QVERIFY(s1EnteredSpy.isValid());
        QVERIFY(s1ExitedSpy.isValid());
        QVERIFY(tTriggeredSpy.isValid());
        QVERIFY(s2EnteredSpy.isValid());
        QVERIFY(s2ExitedSpy.isValid());

        machine.start();

        QTRY_COMPARE(startedSpy.count(), 1);
        QTRY_COMPARE(finishedSpy.count(), 1);
        QTRY_COMPARE(stoppedSpy.count(), 0);
        QCOMPARE(machine.configuration().count(), 1);
        QVERIFY(machine.configuration().contains(s3));

        // s1 is entered
        QCOMPARE(s1->events.count(), 2);
        QCOMPARE(s1->events.at(0).first, 0);
        QCOMPARE(s1->events.at(0).second, TestState::Entry);
        // s1 is exited
        QCOMPARE(s1->events.at(1).first, 1);
        QCOMPARE(s1->events.at(1).second, TestState::Exit);
        // t is triggered
        QCOMPARE(t->triggers.count(), 1);
        QCOMPARE(t->triggers.at(0), 2);
        // s2 is entered
        QCOMPARE(s2->events.count(), 2);
        QCOMPARE(s2->events.at(0).first, 3);
        QCOMPARE(s2->events.at(0).second, TestState::Entry);
        // s2 is exited
        QCOMPARE(s2->events.at(1).first, 4);
        QCOMPARE(s2->events.at(1).second, TestState::Exit);

        QCOMPARE(s1EnteredSpy.count(), 1);
        QCOMPARE(s1ExitedSpy.count(), 1);
        QCOMPARE(tTriggeredSpy.count(), 1);
        QCOMPARE(s2EnteredSpy.count(), 1);
        QCOMPARE(s2ExitedSpy.count(), 1);
    }
    // Two top-level states, one has two child states
    {
        QStateMachine machine;

        TestState *s1 = new TestState(&machine);
        TestState *s11 = new TestState(s1);
        TestState *s12 = new TestState(s1);
        TestState *s2 = new TestState(&machine);
        QFinalState *s3 = new QFinalState(&machine);
        s1->setInitialState(s11);
        TestTransition *t1 = new TestTransition(s12);
        s11->addTransition(t1);
        TestTransition *t2 = new TestTransition(s2);
        s12->addTransition(t2);
        s2->addTransition(s3);

        QSignalSpy startedSpy(&machine, SIGNAL(started()));
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(startedSpy.isValid());
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s1);
        globalTick = 0;
        machine.start();

        QTRY_COMPARE(startedSpy.count(), 1);
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(machine.configuration().count(), 1);
        QVERIFY(machine.configuration().contains(s3));

        // s1 is entered
        QCOMPARE(s1->events.count(), 2);
        QCOMPARE(s1->events.at(0).first, 0);
        QCOMPARE(s1->events.at(0).second, TestState::Entry);
        // s11 is entered
        QCOMPARE(s11->events.count(), 2);
        QCOMPARE(s11->events.at(0).first, 1);
        QCOMPARE(s11->events.at(0).second, TestState::Entry);
        // s11 is exited
        QCOMPARE(s11->events.at(1).first, 2);
        QCOMPARE(s11->events.at(1).second, TestState::Exit);
        // t1 is triggered
        QCOMPARE(t1->triggers.count(), 1);
        QCOMPARE(t1->triggers.at(0), 3);
        // s12 is entered
        QCOMPARE(s12->events.count(), 2);
        QCOMPARE(s12->events.at(0).first, 4);
        QCOMPARE(s12->events.at(0).second, TestState::Entry);
        // s12 is exited
        QCOMPARE(s12->events.at(1).first, 5);
        QCOMPARE(s12->events.at(1).second, TestState::Exit);
        // s1 is exited
        QCOMPARE(s1->events.at(1).first, 6);
        QCOMPARE(s1->events.at(1).second, TestState::Exit);
        // t2 is triggered
        QCOMPARE(t2->triggers.count(), 1);
        QCOMPARE(t2->triggers.at(0), 7);
        // s2 is entered
        QCOMPARE(s2->events.count(), 2);
        QCOMPARE(s2->events.at(0).first, 8);
        QCOMPARE(s2->events.at(0).second, TestState::Entry);
        // s2 is exited
        QCOMPARE(s2->events.at(1).first, 9);
        QCOMPARE(s2->events.at(1).second, TestState::Exit);
    }
}

void tst_QStateMachine::assignProperty()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);

    QTest::ignoreMessage(QtWarningMsg, "QState::assignProperty: cannot assign property 'foo' of null object");
    s1->assignProperty(0, "foo", QVariant());

    s1->assignProperty(s1, "objectName", "s1");
    QFinalState *s2 = new QFinalState(&machine);
    s1->addTransition(s2);
    machine.setInitialState(s1);
    machine.start();
    QTRY_COMPARE(s1->objectName(), QString::fromLatin1("s1"));

    s1->assignProperty(s1, "objectName", "foo");
    machine.start();
    QTRY_COMPARE(s1->objectName(), QString::fromLatin1("foo"));

    s1->assignProperty(s1, "noSuchProperty", 123);
    machine.start();
    QTRY_COMPARE(s1->dynamicPropertyNames().size(), 1);
    QCOMPARE(s1->dynamicPropertyNames().at(0), QByteArray("noSuchProperty"));
    QCOMPARE(s1->objectName(), QString::fromLatin1("foo"));

    {
        QSignalSpy propertiesAssignedSpy(s1, SIGNAL(propertiesAssigned()));
        QVERIFY(propertiesAssignedSpy.isValid());
        machine.start();
        QTRY_COMPARE(propertiesAssignedSpy.count(), 1);
    }

    // nested states
    {
        QState *s11 = new QState(s1);
        QString str = QString::fromLatin1("set by nested state");
        s11->assignProperty(s11, "objectName", str);
        s1->setInitialState(s11);
        machine.start();
        QTRY_COMPARE(s11->objectName(), str);
    }
}

void tst_QStateMachine::assignPropertyWithAnimation()
{
    // Single animation
    {
        QStateMachine machine;
        QVERIFY(machine.isAnimated());
        machine.setAnimated(false);
        QVERIFY(!machine.isAnimated());
        machine.setAnimated(true);
        QVERIFY(machine.isAnimated());
        QObject obj;
        obj.setProperty("foo", 321);
        obj.setProperty("bar", 654);
        QState *s1 = new QState(&machine);
        s1->assignProperty(&obj, "foo", 123);
        QState *s2 = new QState(&machine);
        s2->assignProperty(&obj, "foo", 456);
        s2->assignProperty(&obj, "bar", 789);
        QAbstractTransition *trans = s1->addTransition(s2);
        QVERIFY(trans->animations().isEmpty());
        QTest::ignoreMessage(QtWarningMsg, "QAbstractTransition::addAnimation: cannot add null animation");
        trans->addAnimation(0);
        QPropertyAnimation anim(&obj, "foo");
        anim.setDuration(250);
        trans->addAnimation(&anim);
        QCOMPARE(trans->animations().size(), 1);
        QCOMPARE(trans->animations().at(0), (QAbstractAnimation*)&anim);
        QCOMPARE(anim.parent(), (QObject*)0);
        QTest::ignoreMessage(QtWarningMsg, "QAbstractTransition::removeAnimation: cannot remove null animation");
        trans->removeAnimation(0);
        trans->removeAnimation(&anim);
        QVERIFY(trans->animations().isEmpty());
        trans->addAnimation(&anim);
        QCOMPARE(trans->animations().size(), 1);
        QCOMPARE(trans->animations().at(0), (QAbstractAnimation*)&anim);
        QFinalState *s3 = new QFinalState(&machine);
        s2->addTransition(s2, SIGNAL(propertiesAssigned()), s3);

        machine.setInitialState(s1);
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.start();
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(obj.property("foo").toInt(), 456);
        QCOMPARE(obj.property("bar").toInt(), 789);
    }
    // Two animations
    {
        QStateMachine machine;
        QObject obj;
        obj.setProperty("foo", 321);
        obj.setProperty("bar", 654);
        QState *s1 = new QState(&machine);
        s1->assignProperty(&obj, "foo", 123);
        QState *s2 = new QState(&machine);
        s2->assignProperty(&obj, "foo", 456);
        s2->assignProperty(&obj, "bar", 789);
        QAbstractTransition *trans = s1->addTransition(s2);
        QPropertyAnimation anim(&obj, "foo");
        anim.setDuration(150);
        trans->addAnimation(&anim);
        QPropertyAnimation anim2(&obj, "bar");
        anim2.setDuration(150);
        trans->addAnimation(&anim2);
        QFinalState *s3 = new QFinalState(&machine);
        s2->addTransition(s2, SIGNAL(propertiesAssigned()), s3);

        machine.setInitialState(s1);
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.start();
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(obj.property("foo").toInt(), 456);
        QCOMPARE(obj.property("bar").toInt(), 789);
    }
    // Animation group
    {
        QStateMachine machine;
        QObject obj;
        obj.setProperty("foo", 321);
        obj.setProperty("bar", 654);
        QState *s1 = new QState(&machine);
        s1->assignProperty(&obj, "foo", 123);
        s1->assignProperty(&obj, "bar", 321);
        QState *s2 = new QState(&machine);
        s2->assignProperty(&obj, "foo", 456);
        s2->assignProperty(&obj, "bar", 654);
        s2->assignProperty(&obj, "baz", 789);
        QAbstractTransition *trans = s1->addTransition(s2);
        QSequentialAnimationGroup group;
        group.addAnimation(new QPropertyAnimation(&obj, "foo"));
        group.addAnimation(new QPropertyAnimation(&obj, "bar"));
        trans->addAnimation(&group);
        QFinalState *s3 = new QFinalState(&machine);
        s2->addTransition(s2, SIGNAL(propertiesAssigned()), s3);

        machine.setInitialState(s1);
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.start();
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(obj.property("foo").toInt(), 456);
        QCOMPARE(obj.property("bar").toInt(), 654);
        QCOMPARE(obj.property("baz").toInt(), 789);
    }
    // Nested states
    {
        QStateMachine machine;
        QObject obj;
        obj.setProperty("foo", 321);
        obj.setProperty("bar", 654);
        QState *s1 = new QState(&machine);
        QCOMPARE(s1->childMode(), QState::ExclusiveStates);
        s1->setChildMode(QState::ParallelStates);
        QCOMPARE(s1->childMode(), QState::ParallelStates);
        s1->setChildMode(QState::ExclusiveStates);
        QCOMPARE(s1->childMode(), QState::ExclusiveStates);
        QCOMPARE(s1->initialState(), (QAbstractState*)0);
        s1->setObjectName("s1");
        s1->assignProperty(&obj, "foo", 123);
        s1->assignProperty(&obj, "bar", 456);
        QState *s2 = new QState(&machine);
        s2->setObjectName("s2");
        s2->assignProperty(&obj, "foo", 321);
        QState *s21 = new QState(s2);
        s21->setObjectName("s21");
        s21->assignProperty(&obj, "bar", 654);
        QState *s22 = new QState(s2);
        s22->setObjectName("s22");
        s22->assignProperty(&obj, "bar", 789);
        s2->setInitialState(s21);
        QCOMPARE(s2->initialState(), (QAbstractState*)s21);

        QAbstractTransition *trans = s1->addTransition(s2);
        QPropertyAnimation anim(&obj, "foo");
        anim.setDuration(500);
        trans->addAnimation(&anim);
        QPropertyAnimation anim2(&obj, "bar");
        anim2.setDuration(250);
        trans->addAnimation(&anim2);

        s21->addTransition(s21, SIGNAL(propertiesAssigned()), s22);

        QFinalState *s3 = new QFinalState(&machine);
        s22->addTransition(s2, SIGNAL(propertiesAssigned()), s3);

        machine.setInitialState(s1);
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.start();
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(obj.property("foo").toInt(), 321);
        QCOMPARE(obj.property("bar").toInt(), 789);
    }
    // Aborted animation
    {
        QStateMachine machine;
        SignalEmitter emitter;
        QObject obj;
        obj.setProperty("foo", 321);
        obj.setProperty("bar", 654);
        QState *group = new QState(&machine);
        QState *s1 = new QState(group);
        group->setInitialState(s1);
        s1->assignProperty(&obj, "foo", 123);
        QState *s2 = new QState(group);
        s2->assignProperty(&obj, "foo", 456);
        s2->assignProperty(&obj, "bar", 789);
        QAbstractTransition *trans = s1->addTransition(&emitter, SIGNAL(signalWithNoArg()), s2);
        QPropertyAnimation anim(&obj, "foo");
        anim.setDuration(8000);
        trans->addAnimation(&anim);
        QPropertyAnimation anim2(&obj, "bar");
        anim2.setDuration(8000);
        trans->addAnimation(&anim2);
        QState *s3 = new QState(group);
        s3->assignProperty(&obj, "foo", 911);
        s2->addTransition(&emitter, SIGNAL(signalWithNoArg()), s3);

        machine.setInitialState(group);
        machine.start();
        QTRY_COMPARE(machine.configuration().contains(s1), true);
        QSignalSpy propertiesAssignedSpy(s2, SIGNAL(propertiesAssigned()));
        QVERIFY(propertiesAssignedSpy.isValid());
        emitter.emitSignalWithNoArg();
        QTRY_COMPARE(machine.configuration().contains(s2), true);
        QVERIFY(propertiesAssignedSpy.isEmpty());
        emitter.emitSignalWithNoArg(); // will cause animations from s1-->s2 to abort
        QTRY_COMPARE(machine.configuration().contains(s3), true);
        QVERIFY(propertiesAssignedSpy.isEmpty());
        QCOMPARE(obj.property("foo").toInt(), 911);
        QCOMPARE(obj.property("bar").toInt(), 789);
    }
}

struct StringEvent : public QEvent
{
public:
    StringEvent(const QString &val)
        : QEvent(QEvent::Type(QEvent::User+2)),
          value(val) {}

    QString value;
};

class StringTransition : public QAbstractTransition
{
public:
    StringTransition(const QString &value, QAbstractState *target)
        : QAbstractTransition(), m_value(value)
    { setTargetState(target); }

protected:
    virtual bool eventTest(QEvent *e)
    {
        if (e->type() != QEvent::Type(QEvent::User+2))
            return false;
        StringEvent *se = static_cast<StringEvent*>(e);
        return (m_value == se->value) && (!m_cond.isValid() || (m_cond.indexIn(m_value) != -1));
    }
    virtual void onTransition(QEvent *) {}

private:
    QString m_value;
    QRegExp m_cond;
};

class StringEventPoster : public QState
{
public:
    StringEventPoster(const QString &value, QState *parent = 0)
        : QState(parent), m_value(value), m_delay(-1) {}

    void setString(const QString &value)
        { m_value = value; }
    void setDelay(int delay)
        { m_delay = delay; }

protected:
    virtual void onEntry(QEvent *)
    {
        if (m_delay == -1)
            machine()->postEvent(new StringEvent(m_value));
        else
            machine()->postDelayedEvent(new StringEvent(m_value), m_delay);
    }
    virtual void onExit(QEvent *) {}

private:
    QString m_value;
    int m_delay;
};

void tst_QStateMachine::postEvent()
{
    for (int x = 0; x < 2; ++x) {
        QStateMachine machine;
        {
            QEvent e(QEvent::None);
            QTest::ignoreMessage(QtWarningMsg, "QStateMachine::postEvent: cannot post event when the state machine is not running");
            machine.postEvent(&e);
        }
        StringEventPoster *s1 = new StringEventPoster("a");
        if (x == 1)
            s1->setDelay(100);
        QFinalState *s2 = new QFinalState;
        s1->addTransition(new StringTransition("a", s2));
        machine.addState(s1);
        machine.addState(s2);
        machine.setInitialState(s1);
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.start();
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s2));

        s1->setString("b");
        QFinalState *s3 = new QFinalState();
        machine.addState(s3);
        s1->addTransition(new StringTransition("b", s3));
        finishedSpy.clear();
        machine.start();
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s3));
    }
}

void tst_QStateMachine::cancelDelayedEvent()
{
    QStateMachine machine;
    QTest::ignoreMessage(QtWarningMsg, "QStateMachine::cancelDelayedEvent: the machine is not running");
    QVERIFY(!machine.cancelDelayedEvent(-1));

    QState *s1 = new QState(&machine);
    QFinalState *s2 = new QFinalState(&machine);
    s1->addTransition(new StringTransition("a", s2));
    machine.setInitialState(s1);

    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QVERIFY(startedSpy.isValid());
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    int id1 = machine.postDelayedEvent(new StringEvent("c"), 50000);
    QVERIFY(id1 != -1);
    int id2 = machine.postDelayedEvent(new StringEvent("b"), 25000);
    QVERIFY(id2 != -1);
    QVERIFY(id2 != id1);
    int id3 = machine.postDelayedEvent(new StringEvent("a"), 100);
    QVERIFY(id3 != -1);
    QVERIFY(id3 != id2);
    QVERIFY(machine.cancelDelayedEvent(id1));
    QVERIFY(!machine.cancelDelayedEvent(id1));
    QVERIFY(machine.cancelDelayedEvent(id2));
    QVERIFY(!machine.cancelDelayedEvent(id2));

    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());
    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s2));
}

void tst_QStateMachine::postDelayedEventAndStop()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    QFinalState *s2 = new QFinalState(&machine);
    s1->addTransition(new StringTransition("a", s2));
    machine.setInitialState(s1);

    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QVERIFY(startedSpy.isValid());
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    int id1 = machine.postDelayedEvent(new StringEvent("a"), 0);
    QVERIFY(id1 != -1);
    QSignalSpy stoppedSpy(&machine, SIGNAL(stopped()));
    QVERIFY(stoppedSpy.isValid());
    machine.stop();
    QTRY_COMPARE(stoppedSpy.count(), 1);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    machine.start();
    QTRY_COMPARE(startedSpy.count(), 2);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    int id2 = machine.postDelayedEvent(new StringEvent("a"), 1000);
    QVERIFY(id2 != -1);
    machine.stop();
    QTRY_COMPARE(stoppedSpy.count(), 2);
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 3);
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));
}

class DelayedEventPosterThread : public QThread
{
    Q_OBJECT
public:
    DelayedEventPosterThread(QStateMachine *machine, QObject *parent = 0)
        : QThread(parent), firstEventWasCancelled(false),
          m_machine(machine), m_count(0)
    {
        moveToThread(this);
        QObject::connect(m_machine, SIGNAL(started()),
                         this, SLOT(postEvent()));
    }

    mutable bool firstEventWasCancelled;

private Q_SLOTS:
    void postEvent()
    {
        int id = m_machine->postDelayedEvent(new QEvent(QEvent::User), 1000);
        firstEventWasCancelled = m_machine->cancelDelayedEvent(id);

        m_machine->postDelayedEvent(new QEvent(QEvent::User), 1);

        quit();
    }
private:
    QStateMachine *m_machine;
    int m_count;
};

void tst_QStateMachine::postDelayedEventFromThread()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    QFinalState *f = new QFinalState(&machine);
    s1->addTransition(new EventTransition(QEvent::User, f));
    machine.setInitialState(s1);

    DelayedEventPosterThread poster(&machine);
    poster.start();

    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());
    machine.start();
    QTRY_COMPARE(finishedSpy.count(), 1);

    QVERIFY(poster.firstEventWasCancelled);
}

void tst_QStateMachine::stopAndPostEvent()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QVERIFY(startedSpy.isValid());
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);
    QSignalSpy stoppedSpy(&machine, SIGNAL(stopped()));
    QVERIFY(stoppedSpy.isValid());
    machine.stop();
    QCOMPARE(stoppedSpy.count(), 0);
    machine.postEvent(new QEvent(QEvent::User));
    QTRY_COMPARE(stoppedSpy.count(), 1);
    QCoreApplication::processEvents();
}

void tst_QStateMachine::stateFinished()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    QState *s1_1 = new QState(s1);
    QFinalState *s1_2 = new QFinalState(s1);
    s1_1->addTransition(s1_2);
    s1->setInitialState(s1_1);
    QFinalState *s2 = new QFinalState(&machine);
    s1->addTransition(s1, SIGNAL(finished()), s2);
    machine.setInitialState(s1);
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());
    machine.start();
    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s2));
}

void tst_QStateMachine::parallelStates()
{
    QStateMachine machine;

    TestState *s1 = new TestState(QState::ParallelStates);
    QCOMPARE(s1->childMode(), QState::ParallelStates);
      TestState *s1_1 = new TestState(s1);
        QState *s1_1_1 = new QState(s1_1);
        QFinalState *s1_1_f = new QFinalState(s1_1);
        s1_1_1->addTransition(s1_1_f);
      s1_1->setInitialState(s1_1_1);
      TestState *s1_2 = new TestState(s1);
        QState *s1_2_1 = new QState(s1_2);
        QFinalState *s1_2_f = new QFinalState(s1_2);
        s1_2_1->addTransition(s1_2_f);
      s1_2->setInitialState(s1_2_1);
    {
        QString warning;
        warning.sprintf("QState::setInitialState: ignoring attempt to set initial state of parallel state group %p", s1);
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
        s1->setInitialState(0);
    }
    machine.addState(s1);

    QFinalState *s2 = new QFinalState();
    machine.addState(s2);

    s1->addTransition(s1, SIGNAL(finished()), s2);

    machine.setInitialState(s1);
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());
    globalTick = 0;
    machine.start();
    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s2));

    QCOMPARE(s1->events.count(), 2);
    // s1 is entered
    QCOMPARE(s1->events.at(0).first, 0);
    QCOMPARE(s1->events.at(0).second, TestState::Entry);
    // s1_1 is entered
    QCOMPARE(s1_1->events.count(), 2);
    QCOMPARE(s1_1->events.at(0).first, 1);
    QCOMPARE(s1_1->events.at(0).second, TestState::Entry);
    // s1_2 is entered
    QCOMPARE(s1_2->events.at(0).first, 2);
    QCOMPARE(s1_2->events.at(0).second, TestState::Entry);
    // s1_2 is exited
    QCOMPARE(s1_2->events.at(1).first, 3);
    QCOMPARE(s1_2->events.at(1).second, TestState::Exit);
    // s1_1 is exited
    QCOMPARE(s1_1->events.at(1).first, 4);
    QCOMPARE(s1_1->events.at(1).second, TestState::Exit);
    // s1 is exited
    QCOMPARE(s1->events.at(1).first, 5);
    QCOMPARE(s1->events.at(1).second, TestState::Exit);
}

void tst_QStateMachine::parallelRootState()
{
    QStateMachine machine;
    QState *root = &machine;
    QCOMPARE(root->childMode(), QState::ExclusiveStates);
    root->setChildMode(QState::ParallelStates);
    QCOMPARE(root->childMode(), QState::ParallelStates);

    QState *s1 = new QState(root);
    QFinalState *s1_f = new QFinalState(s1);
    s1->setInitialState(s1_f);
    QState *s2 = new QState(root);
    QFinalState *s2_f = new QFinalState(s2);
    s2->setInitialState(s2_f);

    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QVERIFY(startedSpy.isValid());
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);
    QCOMPARE(machine.configuration().size(), 4);
    QVERIFY(machine.configuration().contains(s1));
    QVERIFY(machine.configuration().contains(s1_f));
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s2_f));
    QTRY_COMPARE(finishedSpy.count(), 1);
}

void tst_QStateMachine::allSourceToTargetConfigurations()
{
    QStateMachine machine;
    QState *s0 = new QState(&machine);
    s0->setObjectName("s0");
    QState *s1 = new QState(s0);
    s1->setObjectName("s1");
    QState *s11 = new QState(s1);
    s11->setObjectName("s11");
    QState *s2 = new QState(s0);
    s2->setObjectName("s2");
    QState *s21 = new QState(s2);
    s21->setObjectName("s21");
    QState *s211 = new QState(s21);
    s211->setObjectName("s211");
    QFinalState *f = new QFinalState(&machine);
    f->setObjectName("f");

    s0->setInitialState(s1);
    s1->setInitialState(s11);
    s2->setInitialState(s21);
    s21->setInitialState(s211);

    s11->addTransition(new StringTransition("g", s211));
    s1->addTransition(new StringTransition("a", s1));
    s1->addTransition(new StringTransition("b", s11));
    s1->addTransition(new StringTransition("c", s2));
    s1->addTransition(new StringTransition("d", s0));
    s1->addTransition(new StringTransition("f", s211));
    s211->addTransition(new StringTransition("d", s21));
    s211->addTransition(new StringTransition("g", s0));
    s211->addTransition(new StringTransition("h", f));
    s21->addTransition(new StringTransition("b", s211));
    s2->addTransition(new StringTransition("c", s1));
    s2->addTransition(new StringTransition("f", s11));
    s0->addTransition(new StringTransition("e", s211));

    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());
    machine.setInitialState(s0);
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new StringEvent("a"));
    QCoreApplication::processEvents();
    machine.postEvent(new StringEvent("b"));
    QCoreApplication::processEvents();
    machine.postEvent(new StringEvent("c"));
    QCoreApplication::processEvents();
    machine.postEvent(new StringEvent("d"));
    QCoreApplication::processEvents();
    machine.postEvent(new StringEvent("e"));
    QCoreApplication::processEvents();
    machine.postEvent(new StringEvent("f"));
    QCoreApplication::processEvents();
    machine.postEvent(new StringEvent("g"));
    QCoreApplication::processEvents();
    machine.postEvent(new StringEvent("h"));
    QCoreApplication::processEvents();

    QTRY_COMPARE(finishedSpy.count(), 1);
}

class TestSignalTransition : public QSignalTransition
{
public:
    TestSignalTransition(QState *sourceState = 0)
        : QSignalTransition(sourceState),
          m_eventTestSender(0), m_eventTestSignalIndex(-1),
          m_transitionSender(0), m_transitionSignalIndex(-1)
    {}
    TestSignalTransition(QObject *sender, const char *signal,
                         QAbstractState *target)
        : QSignalTransition(sender, signal),
        m_eventTestSender(0), m_eventTestSignalIndex(-1),
        m_transitionSender(0), m_transitionSignalIndex(-1)
    { setTargetState(target); }
    QObject *eventTestSenderReceived() const {
        return m_eventTestSender;
    }
    int eventTestSignalIndexReceived() const {
        return m_eventTestSignalIndex;
    }
    QVariantList eventTestArgumentsReceived() const {
        return m_eventTestArgs;
    }
    QObject *transitionSenderReceived() const {
        return m_transitionSender;
    }
    int transitionSignalIndexReceived() const {
        return m_transitionSignalIndex;
    }
    QVariantList transitionArgumentsReceived() const {
        return m_transitionArgs;
    }
protected:
    bool eventTest(QEvent *e) {
        if (!QSignalTransition::eventTest(e))
            return false;
        QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
        m_eventTestSender = se->sender();
        m_eventTestSignalIndex = se->signalIndex();
        m_eventTestArgs = se->arguments();
        return true;
    }
    void onTransition(QEvent *e) {
        QSignalTransition::onTransition(e);
        QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
        m_transitionSender = se->sender();
        m_transitionSignalIndex = se->signalIndex();
        m_transitionArgs = se->arguments();
    }
private:
    QObject *m_eventTestSender;
    int m_eventTestSignalIndex;
    QVariantList m_eventTestArgs;
    QObject *m_transitionSender;
    int m_transitionSignalIndex;
    QVariantList m_transitionArgs;
};

void tst_QStateMachine::signalTransitions()
{
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QTest::ignoreMessage(QtWarningMsg, "QState::addTransition: sender cannot be null");
        QCOMPARE(s0->addTransition(0, SIGNAL(noSuchSignal()), 0), (QSignalTransition*)0);

        SignalEmitter emitter;
        QTest::ignoreMessage(QtWarningMsg, "QState::addTransition: signal cannot be null");
        QCOMPARE(s0->addTransition(&emitter, 0, 0), (QSignalTransition*)0);

        QTest::ignoreMessage(QtWarningMsg, "QState::addTransition: cannot add transition to null state");
        QCOMPARE(s0->addTransition(&emitter, SIGNAL(signalWithNoArg()), 0), (QSignalTransition*)0);

        QFinalState *s1 = new QFinalState(&machine);
        QTest::ignoreMessage(QtWarningMsg, "QState::addTransition: no such signal SignalEmitter::noSuchSignal()");
        QCOMPARE(s0->addTransition(&emitter, SIGNAL(noSuchSignal()), s1), (QSignalTransition*)0);

        QSignalTransition *trans = s0->addTransition(&emitter, SIGNAL(signalWithNoArg()), s1);
        QVERIFY(trans != 0);
        QCOMPARE(trans->sourceState(), s0);
        QCOMPARE(trans->targetState(), (QAbstractState*)s1);
        QCOMPARE(trans->senderObject(), (QObject*)&emitter);
        QCOMPARE(trans->signal(), QByteArray(SIGNAL(signalWithNoArg())));

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        emitter.emitSignalWithNoArg();

        QTRY_COMPARE(finishedSpy.count(), 1);

        emitter.emitSignalWithNoArg();

        trans->setSignal(SIGNAL(signalWithIntArg(int)));
        QCOMPARE(trans->signal(), QByteArray(SIGNAL(signalWithIntArg(int))));
        machine.start();
        QCoreApplication::processEvents();
        emitter.emitSignalWithIntArg(123);
        QTRY_COMPARE(finishedSpy.count(), 2);

        machine.start();
        QCoreApplication::processEvents();
        trans->setSignal(SIGNAL(signalWithNoArg()));
        QCOMPARE(trans->signal(), QByteArray(SIGNAL(signalWithNoArg())));
        emitter.emitSignalWithNoArg();
        QTRY_COMPARE(finishedSpy.count(), 3);

        SignalEmitter emitter2;
        machine.start();
        QCoreApplication::processEvents();
        trans->setSenderObject(&emitter2);
        emitter2.emitSignalWithNoArg();
        QTRY_COMPARE(finishedSpy.count(), 4);

        machine.start();
        QCoreApplication::processEvents();
        QTest::ignoreMessage(QtWarningMsg, "QSignalTransition: no such signal: SignalEmitter::noSuchSignal()");
        trans->setSignal(SIGNAL(noSuchSignal()));
        QCOMPARE(trans->signal(), QByteArray(SIGNAL(noSuchSignal())));
    }
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);
        SignalEmitter emitter;
        QSignalTransition *trans = s0->addTransition(&emitter, "signalWithNoArg()", s1);
        QVERIFY(trans != 0);
        QCOMPARE(trans->sourceState(), s0);
        QCOMPARE(trans->targetState(), (QAbstractState*)s1);
        QCOMPARE(trans->senderObject(), (QObject*)&emitter);
        QCOMPARE(trans->signal(), QByteArray("signalWithNoArg()"));

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        emitter.emitSignalWithNoArg();

        QTRY_COMPARE(finishedSpy.count(), 1);

        trans->setSignal("signalWithIntArg(int)");
        QCOMPARE(trans->signal(), QByteArray("signalWithIntArg(int)"));
        machine.start();
        QCoreApplication::processEvents();
        emitter.emitSignalWithIntArg(123);
        QTRY_COMPARE(finishedSpy.count(), 2);
    }
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);
        SignalEmitter emitter;
        TestSignalTransition *trans = new TestSignalTransition(&emitter, SIGNAL(signalWithIntArg(int)), s1);
        s0->addTransition(trans);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        emitter.emitSignalWithIntArg(123);

        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(trans->eventTestSenderReceived(), (QObject*)&emitter);
        QCOMPARE(trans->eventTestSignalIndexReceived(), emitter.metaObject()->indexOfSignal("signalWithIntArg(int)"));
        QCOMPARE(trans->eventTestArgumentsReceived().size(), 1);
        QCOMPARE(trans->eventTestArgumentsReceived().at(0).toInt(), 123);
        QCOMPARE(trans->transitionSenderReceived(), (QObject*)&emitter);
        QCOMPARE(trans->transitionSignalIndexReceived(), emitter.metaObject()->indexOfSignal("signalWithIntArg(int)"));
        QCOMPARE(trans->transitionArgumentsReceived().size(), 1);
        QCOMPARE(trans->transitionArgumentsReceived().at(0).toInt(), 123);
    }
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);
        SignalEmitter emitter;
        TestSignalTransition *trans = new TestSignalTransition(&emitter, SIGNAL(signalWithStringArg(QString)), s1);
        s0->addTransition(trans);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        QString testString = QString::fromLatin1("hello");
        emitter.emitSignalWithStringArg(testString);

        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(trans->eventTestSenderReceived(), (QObject*)&emitter);
        QCOMPARE(trans->eventTestSignalIndexReceived(), emitter.metaObject()->indexOfSignal("signalWithStringArg(QString)"));
        QCOMPARE(trans->eventTestArgumentsReceived().size(), 1);
        QCOMPARE(trans->eventTestArgumentsReceived().at(0).toString(), testString);
        QCOMPARE(trans->transitionSenderReceived(), (QObject*)&emitter);
        QCOMPARE(trans->transitionSignalIndexReceived(), emitter.metaObject()->indexOfSignal("signalWithStringArg(QString)"));
        QCOMPARE(trans->transitionArgumentsReceived().size(), 1);
        QCOMPARE(trans->transitionArgumentsReceived().at(0).toString(), testString);
    }
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);

        TestSignalTransition *trans = new TestSignalTransition();
        QCOMPARE(trans->senderObject(), (QObject*)0);
        QCOMPARE(trans->signal(), QByteArray());

        SignalEmitter emitter;
        trans->setSenderObject(&emitter);
        QCOMPARE(trans->senderObject(), (QObject*)&emitter);
        trans->setSignal(SIGNAL(signalWithNoArg()));
        QCOMPARE(trans->signal(), QByteArray(SIGNAL(signalWithNoArg())));
        trans->setTargetState(s1);
        s0->addTransition(trans);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        emitter.emitSignalWithNoArg();

        QTRY_COMPARE(finishedSpy.count(), 1);
    }
    // Multiple transitions for same (object,signal)
    {
        QStateMachine machine;
        SignalEmitter emitter;
        QState *s0 = new QState(&machine);
        QState *s1 = new QState(&machine);
        QSignalTransition *t0 = s0->addTransition(&emitter, SIGNAL(signalWithNoArg()), s1);
        QSignalTransition *t1 = s1->addTransition(&emitter, SIGNAL(signalWithNoArg()), s0);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s0));

        emitter.emitSignalWithNoArg();
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s1));

        s0->removeTransition(t0);
        emitter.emitSignalWithNoArg();
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s0));

        emitter.emitSignalWithNoArg();
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s0));

        s1->removeTransition(t1);
        emitter.emitSignalWithNoArg();
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s0));

        s0->addTransition(t0);
        s1->addTransition(t1);
        emitter.emitSignalWithNoArg();
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s1));
    }
    // multiple signal transitions from same source
    {
        QStateMachine machine;
        SignalEmitter emitter;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);
        s0->addTransition(&emitter, SIGNAL(signalWithNoArg()), s1);
        QFinalState *s2 = new QFinalState(&machine);
        s0->addTransition(&emitter, SIGNAL(signalWithIntArg(int)), s2);
        QFinalState *s3 = new QFinalState(&machine);
        s0->addTransition(&emitter, SIGNAL(signalWithStringArg(QString)), s3);

        QSignalSpy startedSpy(&machine, SIGNAL(started()));
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(startedSpy.isValid());
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);

        machine.start();
        QTRY_COMPARE(startedSpy.count(), 1);
        emitter.emitSignalWithNoArg();
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s1));

        machine.start();
        QTRY_COMPARE(startedSpy.count(), 2);
        emitter.emitSignalWithIntArg(123);
        QTRY_COMPARE(finishedSpy.count(), 2);
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s2));

        machine.start();
        QTRY_COMPARE(startedSpy.count(), 3);
        emitter.emitSignalWithStringArg("hello");
        QTRY_COMPARE(finishedSpy.count(), 3);
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s3));
    }
    // signature normalization
    {
        QStateMachine machine;
        SignalEmitter emitter;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);
        QSignalTransition *t0 = s0->addTransition(&emitter, SIGNAL(signalWithNoArg()), s1);
        QVERIFY(t0 != 0);
        QCOMPARE(t0->signal(), QByteArray(SIGNAL(signalWithNoArg())));

        QSignalTransition *t1 = s0->addTransition(&emitter, SIGNAL(signalWithStringArg(QString)), s1);
        QVERIFY(t1 != 0);
        QCOMPARE(t1->signal(), QByteArray(SIGNAL(signalWithStringArg(QString))));

        QSignalSpy startedSpy(&machine, SIGNAL(started()));
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(startedSpy.isValid());
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QTRY_COMPARE(startedSpy.count(), 1);
        QCOMPARE(finishedSpy.count(), 0);

        emitter.emitSignalWithNoArg();

        QTRY_COMPARE(finishedSpy.count(), 1);
    }
}

class TestEventTransition : public QEventTransition
{
public:
    TestEventTransition(QState *sourceState = 0)
        : QEventTransition(sourceState),
          m_eventSource(0), m_eventType(QEvent::None)
    {}
    TestEventTransition(QObject *object, QEvent::Type type,
                        QAbstractState *target)
        : QEventTransition(object, type),
          m_eventSource(0), m_eventType(QEvent::None)
    { setTargetState(target); }
    QObject *eventSourceReceived() const {
        return m_eventSource;
    }
    QEvent::Type eventTypeReceived() const {
        return m_eventType;
    }
protected:
    bool eventTest(QEvent *e) {
        if (!QEventTransition::eventTest(e))
            return false;
        QStateMachine::WrappedEvent *we = static_cast<QStateMachine::WrappedEvent*>(e);
        m_eventSource = we->object();
        m_eventType = we->event()->type();
        return true;
    }
private:
    QObject *m_eventSource;
    QEvent::Type m_eventType;
};

#ifndef QT_NO_WIDGETS
void tst_QStateMachine::eventTransitions()
{
    QPushButton button;
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);

        QMouseEventTransition *trans;
        trans = new QMouseEventTransition(&button, QEvent::MouseButtonPress, Qt::LeftButton);
        QCOMPARE(trans->targetState(), (QAbstractState*)0);
        trans->setTargetState(s1);
        QCOMPARE(trans->eventType(), QEvent::MouseButtonPress);
        QCOMPARE(trans->button(), Qt::LeftButton);
        QCOMPARE(trans->targetState(), (QAbstractState*)s1);
        s0->addTransition(trans);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        QTest::mousePress(&button, Qt::LeftButton);
        QTRY_COMPARE(finishedSpy.count(), 1);

        QTest::mousePress(&button, Qt::LeftButton);

        trans->setEventType(QEvent::MouseButtonRelease);
        QCOMPARE(trans->eventType(), QEvent::MouseButtonRelease);
        machine.start();
        QCoreApplication::processEvents();
        QTest::mouseRelease(&button, Qt::LeftButton);
        QTRY_COMPARE(finishedSpy.count(), 2);

        machine.start();
        QCoreApplication::processEvents();
        trans->setEventType(QEvent::MouseButtonPress);
        QTest::mousePress(&button, Qt::LeftButton);
        QTRY_COMPARE(finishedSpy.count(), 3);

        QPushButton button2;
        machine.start();
        QCoreApplication::processEvents();
        trans->setEventSource(&button2);
        QTest::mousePress(&button2, Qt::LeftButton);
        QTRY_COMPARE(finishedSpy.count(), 4);
    }
    for (int x = 0; x < 2; ++x) {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);

        QEventTransition *trans;
        if (x == 0) {
            trans = new QEventTransition();
            QCOMPARE(trans->eventSource(), (QObject*)0);
            QCOMPARE(trans->eventType(), QEvent::None);
            trans->setEventSource(&button);
            trans->setEventType(QEvent::MouseButtonPress);
            trans->setTargetState(s1);
        } else if (x == 1) {
            trans = new QEventTransition(&button, QEvent::MouseButtonPress);
            trans->setTargetState(s1);
        }
        QCOMPARE(trans->eventSource(), (QObject*)&button);
        QCOMPARE(trans->eventType(), QEvent::MouseButtonPress);
        QCOMPARE(trans->targetState(), (QAbstractState*)s1);
        s0->addTransition(trans);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        QTest::mousePress(&button, Qt::LeftButton);
        QCoreApplication::processEvents();

        QTRY_COMPARE(finishedSpy.count(), 1);
    }
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);

        QMouseEventTransition *trans = new QMouseEventTransition();
        QCOMPARE(trans->eventSource(), (QObject*)0);
        QCOMPARE(trans->eventType(), QEvent::None);
        QCOMPARE(trans->button(), Qt::NoButton);
        trans->setEventSource(&button);
        trans->setEventType(QEvent::MouseButtonPress);
        trans->setButton(Qt::LeftButton);
        trans->setTargetState(s1);
        s0->addTransition(trans);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        QTest::mousePress(&button, Qt::LeftButton);
        QCoreApplication::processEvents();

        QTRY_COMPARE(finishedSpy.count(), 1);
    }

    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);

        QKeyEventTransition *trans = new QKeyEventTransition(&button, QEvent::KeyPress, Qt::Key_A);
        QCOMPARE(trans->eventType(), QEvent::KeyPress);
        QCOMPARE(trans->key(), (int)Qt::Key_A);
        trans->setTargetState(s1);
        s0->addTransition(trans);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        QTest::keyPress(&button, Qt::Key_A);
        QCoreApplication::processEvents();

        QTRY_COMPARE(finishedSpy.count(), 1);
    }
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);

        QKeyEventTransition *trans = new QKeyEventTransition();
        QCOMPARE(trans->eventSource(), (QObject*)0);
        QCOMPARE(trans->eventType(), QEvent::None);
        QCOMPARE(trans->key(), 0);
        trans->setEventSource(&button);
        trans->setEventType(QEvent::KeyPress);
        trans->setKey(Qt::Key_A);
        trans->setTargetState(s1);
        s0->addTransition(trans);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        QTest::keyPress(&button, Qt::Key_A);
        QCoreApplication::processEvents();

        QTRY_COMPARE(finishedSpy.count(), 1);
    }
    // Multiple transitions for same (object,event)
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QState *s1 = new QState(&machine);
        QEventTransition *t0 = new QEventTransition(&button, QEvent::MouseButtonPress);
        t0->setTargetState(s1);
        s0->addTransition(t0);
        QEventTransition *t1 = new QEventTransition(&button, QEvent::MouseButtonPress);
        t1->setTargetState(s0);
        s1->addTransition(t1);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s0));

        QTest::mousePress(&button, Qt::LeftButton);
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s1));

        s0->removeTransition(t0);
        QTest::mousePress(&button, Qt::LeftButton);
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s0));

        QTest::mousePress(&button, Qt::LeftButton);
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s0));

        s1->removeTransition(t1);
        QTest::mousePress(&button, Qt::LeftButton);
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s0));

        s0->addTransition(t0);
        s1->addTransition(t1);
        QTest::mousePress(&button, Qt::LeftButton);
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s1));
    }
    // multiple event transitions from same source
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);
        QFinalState *s2 = new QFinalState(&machine);
        QEventTransition *t0 = new QEventTransition(&button, QEvent::MouseButtonPress);
        t0->setTargetState(s1);
        s0->addTransition(t0);
        QEventTransition *t1 = new QEventTransition(&button, QEvent::MouseButtonRelease);
        t1->setTargetState(s2);
        s0->addTransition(t1);

        QSignalSpy startedSpy(&machine, SIGNAL(started()));
        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(startedSpy.isValid());
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);

        machine.start();
        QTRY_COMPARE(startedSpy.count(), 1);
        QTest::mousePress(&button, Qt::LeftButton);
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s1));

        machine.start();
        QTRY_COMPARE(startedSpy.count(), 2);
        QTest::mouseRelease(&button, Qt::LeftButton);
        QTRY_COMPARE(finishedSpy.count(), 2);
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s2));
    }
    // custom event
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);

        QEventTransition *trans = new QEventTransition(&button, QEvent::Type(QEvent::User+1));
        trans->setTargetState(s1);
        s0->addTransition(trans);

        QSignalSpy startedSpy(&machine, SIGNAL(started()));
        QVERIFY(startedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QTest::ignoreMessage(QtWarningMsg, "QObject event transitions are not supported for custom types");
        QTRY_COMPARE(startedSpy.count(), 1);
    }
    // custom transition
    {
        QStateMachine machine;
        QState *s0 = new QState(&machine);
        QFinalState *s1 = new QFinalState(&machine);

        TestEventTransition *trans = new TestEventTransition(&button, QEvent::MouseButtonPress, s1);
        s0->addTransition(trans);
        QCOMPARE(trans->eventSourceReceived(), (QObject*)0);
        QCOMPARE(trans->eventTypeReceived(), QEvent::None);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.setInitialState(s0);
        machine.start();
        QCoreApplication::processEvents();

        QTest::mousePress(&button, Qt::LeftButton);
        QCoreApplication::processEvents();

        QTRY_COMPARE(finishedSpy.count(), 1);

        QCOMPARE(trans->eventSourceReceived(), (QObject*)&button);
        QCOMPARE(trans->eventTypeReceived(), QEvent::MouseButtonPress);
    }
}

void tst_QStateMachine::graphicsSceneEventTransitions()
{
    QGraphicsScene scene;
    QGraphicsTextItem *textItem = scene.addText("foo");

    QStateMachine machine;
    QState *s1 = new QState(&machine);
    QFinalState *s2 = new QFinalState(&machine);
    QEventTransition *t = new QEventTransition(textItem, QEvent::GraphicsSceneMouseMove);
    t->setTargetState(s2);
    s1->addTransition(t);
    machine.setInitialState(s1);

    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(startedSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);
    QVERIFY(finishedSpy.count() == 0);
    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
    scene.sendEvent(textItem, &mouseEvent);
    QTRY_COMPARE(finishedSpy.count(), 1);
}
#endif

void tst_QStateMachine::historyStates()
{
    for (int x = 0; x < 2; ++x) {
        QStateMachine machine;
        QState *root = &machine;
          QState *s0 = new QState(root);
            QState *s00 = new QState(s0);
            QState *s01 = new QState(s0);
            QHistoryState *s0h;
            if (x == 0) {
                s0h = new QHistoryState(s0);
                QCOMPARE(s0h->historyType(), QHistoryState::ShallowHistory);
                s0h->setHistoryType(QHistoryState::DeepHistory);
            } else {
                s0h = new QHistoryState(QHistoryState::DeepHistory, s0);
            }
            QCOMPARE(s0h->historyType(), QHistoryState::DeepHistory);
            s0h->setHistoryType(QHistoryState::ShallowHistory);
            QCOMPARE(s0h->historyType(), QHistoryState::ShallowHistory);
            QCOMPARE(s0h->defaultState(), (QAbstractState*)0);
            s0h->setDefaultState(s00);
            QCOMPARE(s0h->defaultState(), (QAbstractState*)s00);
            QString warning;
            warning.sprintf("QHistoryState::setDefaultState: state %p does not belong to this history state's group (%p)", s0, s0);
            QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
            s0h->setDefaultState(s0);
          QState *s1 = new QState(root);
          QFinalState *s2 = new QFinalState(root);

        s00->addTransition(new StringTransition("a", s01));
        s0->addTransition(new StringTransition("b", s1));
        s1->addTransition(new StringTransition("c", s0h));
        s0->addTransition(new StringTransition("d", s2));

        root->setInitialState(s0);
        s0->setInitialState(s00);

        QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());
        machine.start();
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 2);
        QVERIFY(machine.configuration().contains(s0));
        QVERIFY(machine.configuration().contains(s00));

        machine.postEvent(new StringEvent("a"));
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 2);
        QVERIFY(machine.configuration().contains(s0));
        QVERIFY(machine.configuration().contains(s01));

        machine.postEvent(new StringEvent("b"));
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s1));

        machine.postEvent(new StringEvent("c"));
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 2);
        QVERIFY(machine.configuration().contains(s0));
        QVERIFY(machine.configuration().contains(s01));

        machine.postEvent(new StringEvent("d"));
        QCoreApplication::processEvents();
        QCOMPARE(machine.configuration().size(), 1);
        QVERIFY(machine.configuration().contains(s2));

        QTRY_COMPARE(finishedSpy.count(), 1);
    }
}

void tst_QStateMachine::startAndStop()
{
    QStateMachine machine;
    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QSignalSpy stoppedSpy(&machine, SIGNAL(stopped()));
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));

    QVERIFY(startedSpy.isValid());
    QVERIFY(stoppedSpy.isValid());
    QVERIFY(finishedSpy.isValid());

    QVERIFY(!machine.isRunning());
    QTest::ignoreMessage(QtWarningMsg, "QStateMachine::start: No initial state set for machine. Refusing to start.");
    machine.start();
    QCOMPARE(startedSpy.count(), 0);
    QCOMPARE(stoppedSpy.count(), 0);
    QCOMPARE(finishedSpy.count(), 0);
    QVERIFY(!machine.isRunning());
    machine.stop();
    QCOMPARE(startedSpy.count(), 0);
    QCOMPARE(stoppedSpy.count(), 0);
    QCOMPARE(finishedSpy.count(), 0);

    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    machine.start();
    QTRY_COMPARE(machine.isRunning(), true);
    QTRY_COMPARE(startedSpy.count(), 1);
    QCOMPARE(stoppedSpy.count(), 0);
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(s1));

    QTest::ignoreMessage(QtWarningMsg, "QStateMachine::start(): already running");
    machine.start();

    machine.stop();
    QTRY_COMPARE(machine.isRunning(), false);
    QTRY_COMPARE(stoppedSpy.count(), 1);
    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);

    QCOMPARE(machine.configuration().count(), 1);
    QVERIFY(machine.configuration().contains(s1));

    machine.start();
    machine.stop();
    QTRY_COMPARE(startedSpy.count(), 2);
    QCOMPARE(stoppedSpy.count(), 2);
}

void tst_QStateMachine::targetStateWithNoParent()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    s1->setObjectName("s1");
    QState s2;
    s1->addTransition(&s2);
    machine.setInitialState(s1);
    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QSignalSpy stoppedSpy(&machine, SIGNAL(stopped()));
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));

    QVERIFY(startedSpy.isValid());
    QVERIFY(stoppedSpy.isValid());
    QVERIFY(finishedSpy.isValid());

    machine.start();
    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: No common ancestor for targets and source of transition from state 's1'");
    QTRY_COMPARE(startedSpy.count(), 1);
    QCOMPARE(machine.isRunning(), false);
    QCOMPARE(stoppedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(machine.error(), QStateMachine::NoCommonAncestorForTransitionError);
}

void tst_QStateMachine::targetStateDeleted()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    s1->setObjectName("s1");
    QState *s2 = new QState(&machine);
    QAbstractTransition *trans = s1->addTransition(s2);
    delete s2;
    QCOMPARE(trans->targetState(), (QAbstractState*)0);
    QVERIFY(trans->targetStates().isEmpty());
}

void tst_QStateMachine::defaultGlobalRestorePolicy()
{
    QStateMachine machine;

    QObject *propertyHolder = new QObject(&machine);
    propertyHolder->setProperty("a", 1);
    propertyHolder->setProperty("b", 2);

    QState *s1 = new QState(&machine);
    s1->assignProperty(propertyHolder, "a", 3);

    QState *s2 = new QState(&machine);
    s2->assignProperty(propertyHolder, "b", 4);

    QState *s3 = new QState(&machine);

    s1->addTransition(new EventTransition(QEvent::User, s2));
    s2->addTransition(new EventTransition(QEvent::User, s3));

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("a").toInt(), 3);
    QCOMPARE(propertyHolder->property("b").toInt(), 2);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("a").toInt(), 3);
    QCOMPARE(propertyHolder->property("b").toInt(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("a").toInt(), 3);
    QCOMPARE(propertyHolder->property("b").toInt(), 4);
}

void tst_QStateMachine::noInitialStateForInitialState()
{
    QStateMachine machine;

    QState *initialState = new QState(&machine);
    initialState->setObjectName("initialState");
    machine.setInitialState(initialState);

    QState *childState = new QState(initialState);
    (void)childState;

    QTest::ignoreMessage(QtWarningMsg, "Unrecoverable error detected in running state machine: "
                                       "Missing initial state in compound state 'initialState'");
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.isRunning(), false);
    QCOMPARE(int(machine.error()), int(QStateMachine::NoInitialStateError));
}

void tst_QStateMachine::globalRestorePolicySetToDontRestore()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::DontRestoreProperties);

    QObject *propertyHolder = new QObject(&machine);
    propertyHolder->setProperty("a", 1);
    propertyHolder->setProperty("b", 2);

    QState *s1 = new QState(&machine);
    s1->assignProperty(propertyHolder, "a", 3);

    QState *s2 = new QState(&machine);
    s2->assignProperty(propertyHolder, "b", 4);

    QState *s3 = new QState(&machine);

    s1->addTransition(new EventTransition(QEvent::User, s2));
    s2->addTransition(new EventTransition(QEvent::User, s3));

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("a").toInt(), 3);
    QCOMPARE(propertyHolder->property("b").toInt(), 2);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("a").toInt(), 3);
    QCOMPARE(propertyHolder->property("b").toInt(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("a").toInt(), 3);
    QCOMPARE(propertyHolder->property("b").toInt(), 4);
}

void tst_QStateMachine::globalRestorePolicySetToRestore()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    QObject *propertyHolder = new QObject(&machine);
    propertyHolder->setProperty("a", 1);
    propertyHolder->setProperty("b", 2);

    QState *s1 = new QState(&machine);
    s1->assignProperty(propertyHolder, "a", 3);

    QState *s2 = new QState(&machine);
    s2->assignProperty(propertyHolder, "b", 4);

    QState *s3 = new QState(&machine);

    s1->addTransition(new EventTransition(QEvent::User, s2));
    s2->addTransition(new EventTransition(QEvent::User, s3));

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("a").toInt(), 3);
    QCOMPARE(propertyHolder->property("b").toInt(), 2);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("a").toInt(), 1);
    QCOMPARE(propertyHolder->property("b").toInt(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("a").toInt(), 1);
    QCOMPARE(propertyHolder->property("b").toInt(), 2);
}

void tst_QStateMachine::transitionWithParent()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    QState *s2 = new QState(&machine);
    EventTransition *trans = new EventTransition(QEvent::User, s2, s1);
    QCOMPARE(trans->sourceState(), s1);
    QCOMPARE(trans->targetState(), (QAbstractState*)s2);
    QCOMPARE(trans->targetStates().size(), 1);
    QCOMPARE(trans->targetStates().at(0), (QAbstractState*)s2);
}

void tst_QStateMachine::simpleAnimation()
{
    QStateMachine machine;

    QObject *object = new QObject(&machine);
    object->setProperty("fooBar", 1.0);

    QState *s1 = new QState(&machine);
    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "fooBar", 2.0);

    EventTransition *et = new EventTransition(QEvent::User, s2);
    QPropertyAnimation *animation = new QPropertyAnimation(object, "fooBar", s2);
    et->addAnimation(animation);
    s1->addTransition(et);

    QState *s3 = new QState(&machine);
    s2->addTransition(animation, SIGNAL(finished()), s3);
    QObject::connect(s3, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s3));
    QCOMPARE(object->property("fooBar").toDouble(), 2.0);
}

class SlotCalledCounter: public QObject
{
    Q_OBJECT
public:
    SlotCalledCounter() : counter(0) {}

    int counter;

public slots:
    void slot() { counter++; }
};

void tst_QStateMachine::twoAnimations()
{
    QStateMachine machine;

    QObject *object = new QObject(&machine);
    object->setProperty("foo", 1.0);
    object->setProperty("bar", 3.0);

    QState *s1 = new QState(&machine);
    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "foo", 2.0);
    s2->assignProperty(object, "bar", 10.0);

    QPropertyAnimation *animationFoo = new QPropertyAnimation(object, "foo", s2);
    QPropertyAnimation *animationBar = new QPropertyAnimation(object, "bar", s2);
    animationBar->setDuration(900);

    SlotCalledCounter counter;
    connect(animationFoo, SIGNAL(finished()), &counter, SLOT(slot()));
    connect(animationBar, SIGNAL(finished()), &counter, SLOT(slot()));

    EventTransition *et = new EventTransition(QEvent::User, s2);
    et->addAnimation(animationFoo);
    et->addAnimation(animationBar);
    s1->addTransition(et);

    QState *s3 = new QState(&machine);
    QObject::connect(s3, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));
    s2->addTransition(s2, SIGNAL(propertiesAssigned()), s3);

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s3));
    QCOMPARE(object->property("foo").toDouble(), 2.0);
    QCOMPARE(object->property("bar").toDouble(), 10.0);

    QCOMPARE(counter.counter, 2);
}

void tst_QStateMachine::twoAnimatedTransitions()
{
    QStateMachine machine;

    QObject *object = new QObject(&machine);
    object->setProperty("foo", 1.0);

    QState *s1 = new QState(&machine);

    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "foo", 5.0);
    QPropertyAnimation *fooAnimation = new QPropertyAnimation(object, "foo", s2);
    EventTransition *trans = new EventTransition(QEvent::User, s2);
    s1->addTransition(trans);
    trans->addAnimation(fooAnimation);

    QState *s3 = new QState(&machine);
    QObject::connect(s3, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));
    s2->addTransition(fooAnimation, SIGNAL(finished()), s3);

    QState *s4 = new QState(&machine);
    s4->assignProperty(object, "foo", 2.0);
    QPropertyAnimation *fooAnimation2 = new QPropertyAnimation(object, "foo", s4);
    trans = new EventTransition(QEvent::User, s4);
    s3->addTransition(trans);
    trans->addAnimation(fooAnimation2);

    QState *s5 = new QState(&machine);
    QObject::connect(s5, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));
    s4->addTransition(fooAnimation2, SIGNAL(finished()), s5);

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s3));
    QCOMPARE(object->property("foo").toDouble(), 5.0);

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s5));
    QCOMPARE(object->property("foo").toDouble(), 2.0);
}

void tst_QStateMachine::playAnimationTwice()
{
    QStateMachine machine;

    QObject *object = new QObject(&machine);
    object->setProperty("foo", 1.0);

    QState *s1 = new QState(&machine);

    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "foo", 5.0);
    QPropertyAnimation *fooAnimation = new QPropertyAnimation(object, "foo", s2);
    EventTransition *trans = new EventTransition(QEvent::User, s2);
    s1->addTransition(trans);
    trans->addAnimation(fooAnimation);

    QState *s3 = new QState(&machine);
    QObject::connect(s3, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));
    s2->addTransition(fooAnimation, SIGNAL(finished()), s3);

    QState *s4 = new QState(&machine);
    s4->assignProperty(object, "foo", 2.0);
    trans = new EventTransition(QEvent::User, s4);
    s3->addTransition(trans);
    trans->addAnimation(fooAnimation);

    QState *s5 = new QState(&machine);
    QObject::connect(s5, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));
    s4->addTransition(fooAnimation, SIGNAL(finished()), s5);

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s3));
    QCOMPARE(object->property("foo").toDouble(), 5.0);

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s5));
    QCOMPARE(object->property("foo").toDouble(), 2.0);
}

void tst_QStateMachine::nestedTargetStateForAnimation()
{
    QStateMachine machine;

    QObject *object = new QObject(&machine);
    object->setProperty("foo", 1.0);
    object->setProperty("bar", 3.0);

    SlotCalledCounter counter;

    QState *s1 = new QState(&machine);
    QState *s2 = new QState(&machine);

    s2->assignProperty(object, "foo", 2.0);

    QState *s2Child = new QState(s2);
    s2Child->assignProperty(object, "bar", 10.0);
    s2->setInitialState(s2Child);

    QState *s2Child2 = new QState(s2);
    s2Child2->assignProperty(object, "bar", 11.0);
    QAbstractTransition *at = new EventTransition(QEvent::User, s2Child2);
    s2Child->addTransition(at);

    QPropertyAnimation *animation = new QPropertyAnimation(object, "bar", s2);
    animation->setDuration(2000);
    connect(animation, SIGNAL(finished()), &counter, SLOT(slot()));
    at->addAnimation(animation);

    at = new EventTransition(QEvent::User, s2);
    s1->addTransition(at);

    animation = new QPropertyAnimation(object, "foo", s2);
    connect(animation, SIGNAL(finished()), &counter, SLOT(slot()));
    at->addAnimation(animation);

    animation = new QPropertyAnimation(object, "bar", s2);
    connect(animation, SIGNAL(finished()), &counter, SLOT(slot()));
    at->addAnimation(animation);

    QState *s3 = new QState(&machine);
    s2->addTransition(s2Child, SIGNAL(propertiesAssigned()), s3);

    QObject::connect(s3, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();
    machine.postEvent(new QEvent(QEvent::User));

    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s3));
    QCOMPARE(object->property("foo").toDouble(), 2.0);
    QCOMPARE(object->property("bar").toDouble(), 10.0);
    QCOMPARE(counter.counter, 2);
}

void tst_QStateMachine::propertiesAssignedSignalTransitionsReuseAnimationGroup()
{
    QStateMachine machine;
    QObject *object = new QObject(&machine);
    object->setProperty("foo", 0);

    QState *s1 = new QState(&machine);
    s1->assignProperty(object, "foo", 123);
    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "foo", 456);
    QState *s3 = new QState(&machine);
    s3->assignProperty(object, "foo", 789);
    QFinalState *s4 = new QFinalState(&machine);

    QParallelAnimationGroup animationGroup;
    animationGroup.addAnimation(new QPropertyAnimation(object, "foo"));
    QSignalSpy animationFinishedSpy(&animationGroup, SIGNAL(finished()));
    QVERIFY(animationFinishedSpy.isValid());
    s1->addTransition(s1, SIGNAL(propertiesAssigned()), s2)->addAnimation(&animationGroup);
    s2->addTransition(s2, SIGNAL(propertiesAssigned()), s3)->addAnimation(&animationGroup);
    s3->addTransition(s3, SIGNAL(propertiesAssigned()), s4);

    machine.setInitialState(s1);
    QSignalSpy machineFinishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(machineFinishedSpy.isValid());
    machine.start();
    QTRY_COMPARE(machineFinishedSpy.count(), 1);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s4));
    QCOMPARE(object->property("foo").toInt(), 789);

    QCOMPARE(animationFinishedSpy.count(), 2);
}

void tst_QStateMachine::animatedGlobalRestoreProperty()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    QObject *object = new QObject(&machine);
    object->setProperty("foo", 1.0);

    SlotCalledCounter counter;

    QState *s1 = new QState(&machine);
    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "foo", 2.0);

    QState *s3 = new QState(&machine);

    QState *s4 = new QState(&machine);
    QObject::connect(s4, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));

    QAbstractTransition *at = new EventTransition(QEvent::User, s2);
    s1->addTransition(at);
    QPropertyAnimation *pa = new QPropertyAnimation(object, "foo", s2);
    connect(pa, SIGNAL(finished()), &counter, SLOT(slot()));
    at->addAnimation(pa);

    at = s2->addTransition(pa, SIGNAL(finished()), s3);
    pa = new QPropertyAnimation(object, "foo", s3);
    connect(pa, SIGNAL(finished()), &counter, SLOT(slot()));
    at->addAnimation(pa);

    at = s3->addTransition(pa, SIGNAL(finished()), s4);
    pa = new QPropertyAnimation(object, "foo", s4);
    connect(pa, SIGNAL(finished()), &counter, SLOT(slot()));
    at->addAnimation(pa);

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));

    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s4));
    QCOMPARE(object->property("foo").toDouble(), 1.0);
    QCOMPARE(counter.counter, 2);
}

void tst_QStateMachine::specificTargetValueOfAnimation()
{
    QStateMachine machine;

    QObject *object = new QObject(&machine);
    object->setProperty("foo", 1.0);

    QState *s1 = new QState(&machine);

    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "foo", 2.0);

    QPropertyAnimation *anim = new QPropertyAnimation(object, "foo");
    anim->setEndValue(10.0);
    EventTransition *trans = new EventTransition(QEvent::User, s2);
    s1->addTransition(trans);
    trans->addAnimation(anim);

    QState *s3 = new QState(&machine);
    QObject::connect(s3, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));
    s2->addTransition(anim, SIGNAL(finished()), s3);

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s3));
    QCOMPARE(object->property("foo").toDouble(), 2.0);
    QCOMPARE(anim->endValue().toDouble(), 10.0);

    delete anim;
}

void tst_QStateMachine::addDefaultAnimation()
{
    QStateMachine machine;

    QObject *object = new QObject();
    object->setProperty("foo", 1.0);

    QState *s1 = new QState(&machine);

    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "foo", 2.0);

    QState *s3 = new QState(&machine);
    QObject::connect(s3, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));

    s1->addTransition(new EventTransition(QEvent::User, s2));

    QPropertyAnimation *pa = new QPropertyAnimation(object, "foo", &machine);
    machine.addDefaultAnimation(pa);
    s2->addTransition(pa, SIGNAL(finished()), s3);

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s3));
    QCOMPARE(object->property("foo").toDouble(), 2.0);

    delete object;
}

void tst_QStateMachine::addDefaultAnimationWithUnusedAnimation()
{
    QStateMachine machine;

    QObject *object = new QObject(&machine);
    object->setProperty("foo", 1.0);
    object->setProperty("bar", 2.0);

    SlotCalledCounter counter;

    QState *s1 = new QState(&machine);

    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "foo", 2.0);

    QState *s3 = new QState(&machine);
    QObject::connect(s3, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));

    s1->addTransition(new EventTransition(QEvent::User, s2));

    QPropertyAnimation *pa = new QPropertyAnimation(object, "foo", &machine);
    connect(pa, SIGNAL(finished()), &counter, SLOT(slot()));
    machine.addDefaultAnimation(pa);
    s2->addTransition(pa, SIGNAL(finished()), s3);

    pa = new QPropertyAnimation(object, "bar", &machine);
    connect(pa, SIGNAL(finished()), &counter, SLOT(slot()));
    machine.addDefaultAnimation(pa);

    machine.setInitialState(s1);
    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s3));
    QCOMPARE(object->property("foo").toDouble(), 2.0);
    QCOMPARE(counter.counter, 1);
}

void tst_QStateMachine::removeDefaultAnimation()
{
    QStateMachine machine;

    QObject propertyHolder;
    propertyHolder.setProperty("foo", 0);

    QCOMPARE(machine.defaultAnimations().size(), 0);

    QPropertyAnimation *anim = new QPropertyAnimation(&propertyHolder, "foo");

    machine.addDefaultAnimation(anim);

    QCOMPARE(machine.defaultAnimations().size(), 1);
    QVERIFY(machine.defaultAnimations().contains(anim));

    machine.removeDefaultAnimation(anim);

    QCOMPARE(machine.defaultAnimations().size(), 0);

    machine.addDefaultAnimation(anim);

    QPropertyAnimation *anim2 = new QPropertyAnimation(&propertyHolder, "foo");
    machine.addDefaultAnimation(anim2);

    QCOMPARE(machine.defaultAnimations().size(), 2);
    QVERIFY(machine.defaultAnimations().contains(anim));
    QVERIFY(machine.defaultAnimations().contains(anim2));

    machine.removeDefaultAnimation(anim);

    QCOMPARE(machine.defaultAnimations().size(), 1);
    QVERIFY(machine.defaultAnimations().contains(anim2));

    machine.removeDefaultAnimation(anim2);
    QCOMPARE(machine.defaultAnimations().size(), 0);

    delete anim;
    delete anim2;
}

void tst_QStateMachine::overrideDefaultAnimationWithSpecific()
{
    QStateMachine machine;

    QObject *object = new QObject(&machine);
    object->setProperty("foo", 1.0);

    SlotCalledCounter counter;

    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);

    QState *s2 = new QState(&machine);
    s2->assignProperty(object, "foo", 2.0);

    QState *s3 = new QState(&machine);
    QObject::connect(s3, SIGNAL(entered()), QCoreApplication::instance(), SLOT(quit()));

    QAbstractTransition *at = new EventTransition(QEvent::User, s2);
    s1->addTransition(at);

    QPropertyAnimation *defaultAnimation = new QPropertyAnimation(object, "foo");
    connect(defaultAnimation, SIGNAL(stateChanged(QAbstractAnimation::State,QAbstractAnimation::State)), &counter, SLOT(slot()));

    QPropertyAnimation *moreSpecificAnimation = new QPropertyAnimation(object, "foo");
    s2->addTransition(moreSpecificAnimation, SIGNAL(finished()), s3);
    connect(moreSpecificAnimation, SIGNAL(stateChanged(QAbstractAnimation::State,QAbstractAnimation::State)), &counter, SLOT(slot()));

    machine.addDefaultAnimation(defaultAnimation);
    at->addAnimation(moreSpecificAnimation);

    machine.start();
    QCoreApplication::processEvents();

    machine.postEvent(new QEvent(QEvent::User));
    QCOREAPPLICATION_EXEC(5000);

    QVERIFY(machine.configuration().contains(s3));
    QCOMPARE(counter.counter, 2); // specific animation started and stopped

    delete defaultAnimation;
    delete moreSpecificAnimation;
}

void tst_QStateMachine::parallelStateAssignmentsDone()
{
    QStateMachine machine;

    QObject *propertyHolder = new QObject(&machine);
    propertyHolder->setProperty("foo", 123);
    propertyHolder->setProperty("bar", 456);
    propertyHolder->setProperty("zoot", 789);

    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);

    QState *parallelState = new QState(QState::ParallelStates, &machine);
    parallelState->assignProperty(propertyHolder, "foo", 321);

    QState *s2 = new QState(parallelState);
    s2->assignProperty(propertyHolder, "bar", 654);

    QState *s3 = new QState(parallelState);
    s3->assignProperty(propertyHolder, "zoot", 987);

    s1->addTransition(new EventTransition(QEvent::User, parallelState));
    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("foo").toInt(), 123);
    QCOMPARE(propertyHolder->property("bar").toInt(), 456);
    QCOMPARE(propertyHolder->property("zoot").toInt(), 789);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(propertyHolder->property("foo").toInt(), 321);
    QCOMPARE(propertyHolder->property("bar").toInt(), 654);
    QCOMPARE(propertyHolder->property("zoot").toInt(), 987);
}

void tst_QStateMachine::transitionsFromParallelStateWithNoChildren()
{
    QStateMachine machine;

    QState *parallelState = new QState(QState::ParallelStates, &machine);
    machine.setInitialState(parallelState);

    QState *s1 = new QState(&machine);
    parallelState->addTransition(new EventTransition(QEvent::User, s1));

    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(1, machine.configuration().size());
    QVERIFY(machine.configuration().contains(parallelState));

    machine.postEvent(new QEvent(QEvent::User));

    QCoreApplication::processEvents();

    QCOMPARE(1, machine.configuration().size());
    QVERIFY(machine.configuration().contains(s1));
}

void tst_QStateMachine::parallelStateTransition()
{
    QStateMachine machine;

    QState *parallelState = new QState(QState::ParallelStates, &machine);
    machine.setInitialState(parallelState);

    QState *s1 = new QState(parallelState);
    QState *s2 = new QState(parallelState);

    QState *s1InitialChild = new QState(s1);
    s1->setInitialState(s1InitialChild);

    QState *s2InitialChild = new QState(s2);
    s2->setInitialState(s2InitialChild);

    QState *s1OtherChild = new QState(s1);

    s1->addTransition(new EventTransition(QEvent::User, s1OtherChild));

    machine.start();
    QCoreApplication::processEvents();

    QVERIFY(machine.configuration().contains(parallelState));
    QVERIFY(machine.configuration().contains(s1));
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s1InitialChild));
    QVERIFY(machine.configuration().contains(s2InitialChild));
    QCOMPARE(machine.configuration().size(), 5);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QVERIFY(machine.configuration().contains(parallelState));

    QVERIFY(machine.configuration().contains(s1));

    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s1OtherChild));
    QVERIFY(machine.configuration().contains(s2InitialChild));
    QCOMPARE(machine.configuration().size(), 5);

}

void tst_QStateMachine::nestedRestoreProperties()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    QObject *propertyHolder = new QObject(&machine);
    propertyHolder->setProperty("foo", 1);
    propertyHolder->setProperty("bar", 2);

    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);

    QState *s2 = new QState(&machine);
    s2->assignProperty(propertyHolder, "foo", 3);

    QState *s21 = new QState(s2);
    s21->assignProperty(propertyHolder, "bar", 4);
    s2->setInitialState(s21);

    QState *s22 = new QState(s2);
    s22->assignProperty(propertyHolder, "bar", 5);

    s1->addTransition(new EventTransition(QEvent::User, s2));
    s21->addTransition(new EventTransition(QEvent::User, s22));

    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));
    QCOMPARE(propertyHolder->property("foo").toInt(), 1);
    QCOMPARE(propertyHolder->property("bar").toInt(), 2);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s21));
    QCOMPARE(propertyHolder->property("foo").toInt(), 3);
    QCOMPARE(propertyHolder->property("bar").toInt(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s22));
    QCOMPARE(propertyHolder->property("foo").toInt(), 3);
    QCOMPARE(propertyHolder->property("bar").toInt(), 5);
}

void tst_QStateMachine::nestedRestoreProperties2()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    QObject *propertyHolder = new QObject(&machine);
    propertyHolder->setProperty("foo", 1);
    propertyHolder->setProperty("bar", 2);

    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);

    QState *s2 = new QState(&machine);
    s2->assignProperty(propertyHolder, "foo", 3);

    QState *s21 = new QState(s2);
    s21->assignProperty(propertyHolder, "bar", 4);
    s2->setInitialState(s21);

    QState *s22 = new QState(s2);
    s22->assignProperty(propertyHolder, "foo", 6);
    s22->assignProperty(propertyHolder, "bar", 5);

    s1->addTransition(new EventTransition(QEvent::User, s2));
    s21->addTransition(new EventTransition(QEvent::User, s22));
    s22->addTransition(new EventTransition(QEvent::User, s21));

    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));
    QCOMPARE(propertyHolder->property("foo").toInt(), 1);
    QCOMPARE(propertyHolder->property("bar").toInt(), 2);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s21));
    QCOMPARE(propertyHolder->property("foo").toInt(), 3);
    QCOMPARE(propertyHolder->property("bar").toInt(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s22));
    QCOMPARE(propertyHolder->property("foo").toInt(), 6);
    QCOMPARE(propertyHolder->property("bar").toInt(), 5);

    machine.postEvent(new QEvent(QEvent::User));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s21));
    QCOMPARE(propertyHolder->property("foo").toInt(), 3);
    QCOMPARE(propertyHolder->property("bar").toInt(), 4);

}

void tst_QStateMachine::nestedStateMachines()
{
    QStateMachine machine;
    QState *group = new QState(&machine);
    group->setChildMode(QState::ParallelStates);
    QStateMachine *subMachines[3];
    for (int i = 0; i < 3; ++i) {
        QState *subGroup = new QState(group);
        QStateMachine *subMachine = new QStateMachine(subGroup);
        {
            QState *initial = new QState(subMachine);
            QFinalState *done = new QFinalState(subMachine);
            initial->addTransition(new EventTransition(QEvent::User, done));
            subMachine->setInitialState(initial);
        }
        QFinalState *subMachineDone = new QFinalState(subGroup);
        subMachine->addTransition(subMachine, SIGNAL(finished()), subMachineDone);
        subGroup->setInitialState(subMachine);
        subMachines[i] = subMachine;
    }
    QFinalState *final = new QFinalState(&machine);
    group->addTransition(group, SIGNAL(finished()), final);
    machine.setInitialState(group);

    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(startedSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);
    QTRY_COMPARE(machine.configuration().count(), 1+2*3);
    QVERIFY(machine.configuration().contains(group));
    for (int i = 0; i < 3; ++i)
        QVERIFY(machine.configuration().contains(subMachines[i]));

    QCoreApplication::processEvents(); // starts the submachines

    for (int i = 0; i < 3; ++i)
        subMachines[i]->postEvent(new QEvent(QEvent::User));

    QTRY_COMPARE(finishedSpy.count(), 1);
}

void tst_QStateMachine::goToState()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    QState *s2 = new QState(&machine);
    machine.setInitialState(s1);
    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QVERIFY(startedSpy.isValid());
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);

    QStateMachinePrivate::get(&machine)->goToState(s2);
    QCoreApplication::processEvents();
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s2));

    QStateMachinePrivate::get(&machine)->goToState(s2);
    QCoreApplication::processEvents();
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s2));

    QStateMachinePrivate::get(&machine)->goToState(s1);
    QStateMachinePrivate::get(&machine)->goToState(s2);
    QStateMachinePrivate::get(&machine)->goToState(s1);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s2));

    QCoreApplication::processEvents();
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    // go to state in group
    QState *s2_1 = new QState(s2);
    s2->setInitialState(s2_1);
    QStateMachinePrivate::get(&machine)->goToState(s2_1);
    QCoreApplication::processEvents();
    QCOMPARE(machine.configuration().size(), 2);
    QVERIFY(machine.configuration().contains(s2));
    QVERIFY(machine.configuration().contains(s2_1));
}

void tst_QStateMachine::goToStateFromSourceWithTransition()
{
    // QTBUG-21813
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    s1->addTransition(new QSignalTransition);
    QState *s2 = new QState(&machine);
    machine.setInitialState(s1);
    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QVERIFY(startedSpy.isValid());
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);

    QStateMachinePrivate::get(&machine)->goToState(s2);
    QCoreApplication::processEvents();
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s2));
}

class CloneSignalTransition : public QSignalTransition
{
public:
    CloneSignalTransition(QObject *sender, const char *signal, QAbstractState *target)
        : QSignalTransition(sender, signal)
    {
        setTargetState(target);
    }

    void onTransition(QEvent *e)
    {
        QSignalTransition::onTransition(e);
        QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
        eventSignalIndex = se->signalIndex();
    }

    int eventSignalIndex;
};

void tst_QStateMachine::clonedSignals()
{
    SignalEmitter emitter;
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    QState *s2 = new QState(&machine);
    CloneSignalTransition *t1 = new CloneSignalTransition(&emitter, SIGNAL(signalWithDefaultArg()), s2);
    s1->addTransition(t1);

    machine.setInitialState(s1);
    machine.start();
    QTest::qWait(1);

    emitter.emitSignalWithDefaultArg();
    QTest::qWait(1);
    QCOMPARE(t1->eventSignalIndex, emitter.metaObject()->indexOfSignal("signalWithDefaultArg()"));
}

class EventPosterThread : public QThread
{
    Q_OBJECT
public:
    EventPosterThread(QStateMachine *machine, QObject *parent = 0)
        : QThread(parent), m_machine(machine), m_count(0)
    {
        moveToThread(this);
        QObject::connect(m_machine, SIGNAL(started()),
                         this, SLOT(postEvent()));
    }
protected:
    virtual void run()
    {
        exec();
    }
private Q_SLOTS:
    void postEvent()
    {
        m_machine->postEvent(new QEvent(QEvent::User));
        if (++m_count < 1000)
            QTimer::singleShot(0, this, SLOT(postEvent()));
        else
            quit();
    }
private:
    QStateMachine *m_machine;
    int m_count;
};

void tst_QStateMachine::postEventFromOtherThread()
{
    QStateMachine machine;
    EventPosterThread poster(&machine);
    StringEventPoster *s1 = new StringEventPoster("foo", &machine);
    s1->addTransition(new EventTransition(QEvent::User, s1));
    QFinalState *f = new QFinalState(&machine);
    s1->addTransition(&poster, SIGNAL(finished()), f);
    machine.setInitialState(s1);

    poster.start();

    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(finishedSpy.isValid());
    machine.start();
    QTRY_COMPARE(finishedSpy.count(), 1);
}

#ifndef QT_NO_WIDGETS
void tst_QStateMachine::eventFilterForApplication()
{
    QStateMachine machine;

    QState *s1 = new QState(&machine);
    {
        machine.setInitialState(s1);
    }

    QState *s2 = new QState(&machine);

    QEventTransition *transition = new QEventTransition(QCoreApplication::instance(),
                                                        QEvent::ApplicationActivate);
    transition->setTargetState(s2);
    s1->addTransition(transition);

    machine.start();
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));

    QCoreApplication::postEvent(QCoreApplication::instance(),
                                new QEvent(QEvent::ApplicationActivate));
    QCoreApplication::processEvents();

    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s2));
}
#endif

void tst_QStateMachine::eventClassesExported()
{
    // make sure this links
    QStateMachine::WrappedEvent *wrappedEvent = new QStateMachine::WrappedEvent(0, 0);
    Q_UNUSED(wrappedEvent);
    QStateMachine::SignalEvent *signalEvent = new QStateMachine::SignalEvent(0, 0, QList<QVariant>());
    Q_UNUSED(signalEvent);
}

void tst_QStateMachine::stopInTransitionToFinalState()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    QFinalState *s2 = new QFinalState(&machine);
    QAbstractTransition *t1 = s1->addTransition(s2);
    machine.setInitialState(s1);

    QObject::connect(t1, SIGNAL(triggered()), &machine, SLOT(stop()));
    QSignalSpy stoppedSpy(&machine, SIGNAL(stopped()));
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QSignalSpy s2EnteredSpy(s2, SIGNAL(entered()));
    QVERIFY(stoppedSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(s2EnteredSpy.isValid());
    machine.start();

    // Stopping should take precedence over finished.
    QTRY_COMPARE(stoppedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(s2EnteredSpy.count(), 1);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s2));
}

class StopInEventTestTransition : public QAbstractTransition
{
public:
    bool eventTest(QEvent *e)
    {
        if (e->type() == QEvent::User)
            machine()->stop();
        return false;
    }
    void onTransition(QEvent *)
    { }
};

void tst_QStateMachine::stopInEventTest_data()
{
    QTest::addColumn<int>("eventPriority");
    QTest::newRow("NormalPriority") << int(QStateMachine::NormalPriority);
    QTest::newRow("HighPriority") << int(QStateMachine::HighPriority);
}

void tst_QStateMachine::stopInEventTest()
{
    QFETCH(int, eventPriority);

    QStateMachine machine;
    QState *s1 = new QState(&machine);
    s1->addTransition(new StopInEventTestTransition());
    machine.setInitialState(s1);

    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QVERIFY(startedSpy.isValid());
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);

    QSignalSpy stoppedSpy(&machine, SIGNAL(stopped()));
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    QVERIFY(stoppedSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    machine.postEvent(new QEvent(QEvent::User), QStateMachine::EventPriority(eventPriority));

    QTRY_COMPARE(stoppedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(machine.configuration().size(), 1);
    QVERIFY(machine.configuration().contains(s1));
}

class IncrementReceiversTest : public QObject
{
    Q_OBJECT
signals:
    void mySignal();
public:
    virtual void connectNotify(const QMetaMethod &signal)
    {
        signalList.append(signal);
    }

    QList<QMetaMethod> signalList;
};

void tst_QStateMachine::testIncrementReceivers()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    QFinalState *s2 = new QFinalState(&machine);

    IncrementReceiversTest testObject;
    s1->addTransition(&testObject, SIGNAL(mySignal()), s2);

    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    machine.start();

    QMetaObject::invokeMethod(&testObject, "mySignal", Qt::QueuedConnection);

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(testObject.signalList.size(), 1);
    QCOMPARE(testObject.signalList.at(0), QMetaMethod::fromSignal(&IncrementReceiversTest::mySignal));
}

void tst_QStateMachine::initialStateIsEnteredBeforeStartedEmitted()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    QFinalState *s2 = new QFinalState(&machine);

    // When started() is emitted, s1 should be the active state, and this
    // transition should trigger.
    s1->addTransition(&machine, SIGNAL(started()), s2);

    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    machine.start();
    QTRY_COMPARE(finishedSpy.count(), 1);
}

void tst_QStateMachine::deletePropertyAssignmentObjectBeforeEntry()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);

    QObject *o1 = new QObject;
    s1->assignProperty(o1, "objectName", "foo");
    delete o1;
    QObject *o2 = new QObject;
    s1->assignProperty(o2, "objectName", "bar");

    machine.start();
    // Shouldn't crash
    QTRY_VERIFY(machine.configuration().contains(s1));

    QCOMPARE(o2->objectName(), QString::fromLatin1("bar"));
    delete o2;
}

void tst_QStateMachine::deletePropertyAssignmentObjectBeforeRestore()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    QState *s2 = new QState(&machine);
    s1->addTransition(new EventTransition(QEvent::User, s2));

    QObject *o1 = new QObject;
    s1->assignProperty(o1, "objectName", "foo");
    QObject *o2 = new QObject;
    s1->assignProperty(o2, "objectName", "bar");

    QVERIFY(o1->objectName().isEmpty());
    QVERIFY(o2->objectName().isEmpty());
    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));
    QCOMPARE(o1->objectName(), QString::fromLatin1("foo"));
    QCOMPARE(o2->objectName(), QString::fromLatin1("bar"));

    delete o1;
    machine.postEvent(new QEvent(QEvent::User));
    // Shouldn't crash
    QTRY_VERIFY(machine.configuration().contains(s2));

    QVERIFY(o2->objectName().isEmpty());
    delete o2;
}

void tst_QStateMachine::deleteInitialState()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    delete s1;
    QTest::ignoreMessage(QtWarningMsg, "QStateMachine::start: No initial state set for machine. Refusing to start.");
    machine.start();
    // Shouldn't crash
    QCoreApplication::processEvents();
}

void tst_QStateMachine::setPropertyAfterRestore()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    QObject *object = new QObject(&machine);
    object->setProperty("a", 1);

    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    s1->assignProperty(object, "a", 2);

    QState *s2 = new QState(&machine);
    s1->addTransition(new EventTransition(QEvent::User, s2));

    QState *s3 = new QState(&machine);
    s3->assignProperty(object, "a", 4);
    s2->addTransition(new EventTransition(QEvent::User, s3));

    QState *s4 = new QState(&machine);
    s3->addTransition(new EventTransition(QEvent::User, s4));

    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));
    QCOMPARE(object->property("a").toInt(), 2);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s2));
    QCOMPARE(object->property("a").toInt(), 1); // restored

    // Set property outside of state machine; this is the value
    // that should be remembered in the next transition
    object->setProperty("a", 3);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s3));
    QCOMPARE(object->property("a").toInt(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s4));
    QCOMPARE(object->property("a").toInt(), 3); // restored

    delete object;
}

void tst_QStateMachine::transitionWithNoTarget_data()
{
    QTest::addColumn<int>("restorePolicy");
    QTest::newRow("DontRestoreProperties") << int(QState::DontRestoreProperties);
    QTest::newRow("RestoreProperties") << int(QState::RestoreProperties);
}

void tst_QStateMachine::transitionWithNoTarget()
{
    QFETCH(int, restorePolicy);

    QStateMachine machine;
    machine.setGlobalRestorePolicy(static_cast<QState::RestorePolicy>(restorePolicy));

    QObject *object = new QObject;
    object->setProperty("a", 1);

    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    s1->assignProperty(object, "a", 2);
    EventTransition *t1 = new EventTransition(QEvent::User, /*target=*/0);
    s1->addTransition(t1);

    QSignalSpy s1EnteredSpy(s1, SIGNAL(entered()));
    QSignalSpy s1ExitedSpy(s1, SIGNAL(exited()));
    QSignalSpy t1TriggeredSpy(t1, SIGNAL(triggered()));

    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));
    QCOMPARE(s1EnteredSpy.count(), 1);
    QCOMPARE(s1ExitedSpy.count(), 0);
    QCOMPARE(t1TriggeredSpy.count(), 0);
    QCOMPARE(object->property("a").toInt(), 2);

    object->setProperty("a", 3);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_COMPARE(t1TriggeredSpy.count(), 1);
    QCOMPARE(s1EnteredSpy.count(), 1);
    QCOMPARE(s1ExitedSpy.count(), 0);
    // the assignProperty should not be re-executed, nor should the old value
    // be restored
    QCOMPARE(object->property("a").toInt(), 3);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_COMPARE(t1TriggeredSpy.count(), 2);
    QCOMPARE(s1EnteredSpy.count(), 1);
    QCOMPARE(s1ExitedSpy.count(), 0);
    QCOMPARE(object->property("a").toInt(), 3);

    delete object;
}

void tst_QStateMachine::initialStateIsFinal()
{
    QStateMachine machine;
    QFinalState *f = new QFinalState(&machine);
    machine.setInitialState(f);
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    machine.start();
    QTRY_VERIFY(machine.configuration().contains(f));
    QTRY_COMPARE(finishedSpy.count(), 1);
}

class PropertyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop READ prop WRITE setProp)
public:
    PropertyObject(QObject *parent = 0)
        : QObject(parent), m_propValue(0), m_propWriteCount(0)
    {}
    int prop() const { return m_propValue; }
    void setProp(int value) { m_propValue = value; ++m_propWriteCount; }
    int propWriteCount() const { return m_propWriteCount; }
private:
    int m_propValue;
    int m_propWriteCount;
};

void tst_QStateMachine::restorePropertiesSimple()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    PropertyObject *po = new PropertyObject;
    po->setProp(2);
    QCOMPARE(po->propWriteCount(), 1);

    QState *s1 = new QState(&machine);
    s1->assignProperty(po, "prop", 4);
    machine.setInitialState(s1);

    QState *s2 = new QState(&machine);
    s1->addTransition(new EventTransition(QEvent::User, s2));

    QState *s3 = new QState(&machine);
    s3->assignProperty(po, "prop", 6);
    s2->addTransition(new EventTransition(QEvent::User, s3));

    QState *s4 = new QState(&machine);
    s4->assignProperty(po, "prop", 8);
    s3->addTransition(new EventTransition(QEvent::User, s4));

    QState *s5 = new QState(&machine);
    s4->addTransition(new EventTransition(QEvent::User, s5));

    QState *s6 = new QState(&machine);
    s5->addTransition(new EventTransition(QEvent::User, s6));

    machine.start();

    QTRY_VERIFY(machine.configuration().contains(s1));
    QCOMPARE(po->propWriteCount(), 2);
    QCOMPARE(po->prop(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s2));
    QCOMPARE(po->propWriteCount(), 3);
    QCOMPARE(po->prop(), 2); // restored

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s3));
    QCOMPARE(po->propWriteCount(), 4);
    QCOMPARE(po->prop(), 6);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s4));
    QCOMPARE(po->propWriteCount(), 5);
    QCOMPARE(po->prop(), 8);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s5));
    QCOMPARE(po->propWriteCount(), 6);
    QCOMPARE(po->prop(), 2); // restored

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s6));
    QCOMPARE(po->propWriteCount(), 6);

    delete po;
}

void tst_QStateMachine::restoreProperties2()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    PropertyObject *po = new PropertyObject;
    po->setProp(2);
    QCOMPARE(po->propWriteCount(), 1);

    QState *s1 = new QState(&machine);
    s1->assignProperty(po, "prop", 4);
    machine.setInitialState(s1);

    QState *s11 = new QState(s1);
    s1->setInitialState(s11);

    QState *s12 = new QState(s1);
    s11->addTransition(new EventTransition(QEvent::User, s12));

    QState *s13 = new QState(s1);
    s13->assignProperty(po, "prop", 6);
    s12->addTransition(new EventTransition(QEvent::User, s13));

    QState *s14 = new QState(s1);
    s14->assignProperty(po, "prop", 8);
    s13->addTransition(new EventTransition(QEvent::User, s14));

    QState *s15 = new QState(s1);
    s14->addTransition(new EventTransition(QEvent::User, s15));

    QState *s16 = new QState(s1);
    s15->addTransition(new EventTransition(QEvent::User, s16));

    QState *s2 = new QState(&machine);
    s2->assignProperty(po, "prop", 10);
    s16->addTransition(new EventTransition(QEvent::User, s2));

    QState *s3 = new QState(&machine);
    s2->addTransition(new EventTransition(QEvent::User, s3));

    machine.start();

    QTRY_VERIFY(machine.configuration().contains(s11));
    QCOMPARE(po->propWriteCount(), 2);
    QCOMPARE(po->prop(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s12));
    QCOMPARE(po->propWriteCount(), 2);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s13));
    QCOMPARE(po->propWriteCount(), 3);
    QCOMPARE(po->prop(), 6);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s14));
    QCOMPARE(po->propWriteCount(), 4);
    QCOMPARE(po->prop(), 8);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s15));
    QCOMPARE(po->propWriteCount(), 5);
    QCOMPARE(po->prop(), 4); // restored s1

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s16));
    QCOMPARE(po->propWriteCount(), 5);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s2));
    QCOMPARE(po->propWriteCount(), 6);
    QCOMPARE(po->prop(), 10);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s3));
    QCOMPARE(po->propWriteCount(), 7);
    QCOMPARE(po->prop(), 2); // restored original

    delete po;
}

void tst_QStateMachine::restoreProperties3()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    PropertyObject *po = new PropertyObject;
    po->setProp(2);
    QCOMPARE(po->propWriteCount(), 1);

    QState *s1 = new QState(&machine);
    s1->assignProperty(po, "prop", 4);
    machine.setInitialState(s1);

    QState *s11 = new QState(s1);
    s11->assignProperty(po, "prop", 6);
    s1->setInitialState(s11);

    QState *s12 = new QState(s1);
    s11->addTransition(new EventTransition(QEvent::User, s12));

    QState *s13 = new QState(s1);
    s13->assignProperty(po, "prop", 8);
    s12->addTransition(new EventTransition(QEvent::User, s13));

    QState *s2 = new QState(&machine);
    s13->addTransition(new EventTransition(QEvent::User, s2));

    machine.start();

    QTRY_VERIFY(machine.configuration().contains(s11));
    QCOMPARE(po->propWriteCount(), 3);
    QCOMPARE(po->prop(), 6); // s11

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s12));
    QCOMPARE(po->propWriteCount(), 4);
    QCOMPARE(po->prop(), 4); // restored s1

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s13));
    QCOMPARE(po->propWriteCount(), 5);
    QCOMPARE(po->prop(), 8);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s2));
    QCOMPARE(po->propWriteCount(), 6);
    QCOMPARE(po->prop(), 2); // restored original

    delete po;
}

// QTBUG-20362
void tst_QStateMachine::restoreProperties4()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    PropertyObject *po1 = new PropertyObject;
    po1->setProp(2);
    QCOMPARE(po1->propWriteCount(), 1);
    PropertyObject *po2 = new PropertyObject;
    po2->setProp(4);
    QCOMPARE(po2->propWriteCount(), 1);

    QState *s1 = new QState(&machine);
    s1->setChildMode(QState::ParallelStates);
    machine.setInitialState(s1);

    QState *s11 = new QState(s1);
    QState *s111 = new QState(s11);
    s111->assignProperty(po1, "prop", 6);
    s11->setInitialState(s111);

    QState *s112 = new QState(s11);
    s112->assignProperty(po1, "prop", 8);
    s111->addTransition(new EventTransition(QEvent::User, s112));

    QState *s12 = new QState(s1);
    QState *s121 = new QState(s12);
    s121->assignProperty(po2, "prop", 10);
    s12->setInitialState(s121);

    QState *s122 = new QState(s12);
    s122->assignProperty(po2, "prop", 12);
    s121->addTransition(new EventTransition(static_cast<QEvent::Type>(QEvent::User+1), s122));

    QState *s2 = new QState(&machine);
    s112->addTransition(new EventTransition(QEvent::User, s2));

    machine.start();

    QTRY_VERIFY(machine.configuration().contains(s1));
    QVERIFY(machine.configuration().contains(s11));
    QVERIFY(machine.configuration().contains(s111));
    QVERIFY(machine.configuration().contains(s12));
    QVERIFY(machine.configuration().contains(s121));
    QCOMPARE(po1->propWriteCount(), 2);
    QCOMPARE(po1->prop(), 6);
    QCOMPARE(po2->propWriteCount(), 2);
    QCOMPARE(po2->prop(), 10);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s112));
    QCOMPARE(po1->propWriteCount(), 3);
    QCOMPARE(po1->prop(), 8);
    QCOMPARE(po2->propWriteCount(), 2);

    machine.postEvent(new QEvent(static_cast<QEvent::Type>(QEvent::User+1)));
    QTRY_VERIFY(machine.configuration().contains(s122));
    QCOMPARE(po1->propWriteCount(), 3);
    QCOMPARE(po2->propWriteCount(), 3);
    QCOMPARE(po2->prop(), 12);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s2));
    QCOMPARE(po1->propWriteCount(), 4);
    QCOMPARE(po1->prop(), 2); // restored original
    QCOMPARE(po2->propWriteCount(), 4);
    QCOMPARE(po2->prop(), 4); // restored original

    delete po1;
    delete po2;
}

void tst_QStateMachine::restorePropertiesSelfTransition()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    PropertyObject *po = new PropertyObject;
    po->setProp(2);
    QCOMPARE(po->propWriteCount(), 1);

    QState *s1 = new QState(&machine);
    s1->assignProperty(po, "prop", 4);
    s1->addTransition(new EventTransition(QEvent::User, s1));
    machine.setInitialState(s1);

    QState *s2 = new QState(&machine);
    s1->addTransition(new EventTransition(static_cast<QEvent::Type>(QEvent::User+1), s2));

    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));
    QCOMPARE(po->propWriteCount(), 2);
    QCOMPARE(po->prop(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_COMPARE(po->propWriteCount(), 3);
    QCOMPARE(po->prop(), 4);

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_COMPARE(po->propWriteCount(), 4);
    QCOMPARE(po->prop(), 4);

    machine.postEvent(new QEvent(static_cast<QEvent::Type>(QEvent::User+1)));
    QTRY_VERIFY(machine.configuration().contains(s2));
    QCOMPARE(po->propWriteCount(), 5);
    QCOMPARE(po->prop(), 2); // restored

    delete po;
}

void tst_QStateMachine::changeStateWhileAnimatingProperty()
{
    QStateMachine machine;
    machine.setGlobalRestorePolicy(QState::RestoreProperties);

    QObject *o1 = new QObject;
    o1->setProperty("x", 10.);
    QObject *o2 = new QObject;
    o2->setProperty("y", 20.);

    QState *group = new QState(&machine);
    machine.setInitialState(group);

    QState *s0 = new QState(group);
    group->setInitialState(s0);

    QState *s1 = new QState(group);
    s1->assignProperty(o1, "x", 15.);
    QPropertyAnimation *a1 = new QPropertyAnimation(o1, "x", s1);
    a1->setDuration(800);
    machine.addDefaultAnimation(a1);
    group->addTransition(new EventTransition(QEvent::User, s1));

    QState *s2 = new QState(group);
    s2->assignProperty(o2, "y", 25.);
    QPropertyAnimation *a2 = new QPropertyAnimation(o2, "y", s2);
    a2->setDuration(800);
    machine.addDefaultAnimation(a2);
    group->addTransition(new EventTransition(static_cast<QEvent::Type>(QEvent::User+1), s2));

    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s0));

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s1));
    QCOREAPPLICATION_EXEC(400);
    machine.postEvent(new QEvent(static_cast<QEvent::Type>(QEvent::User+1)));
    QTRY_VERIFY(machine.configuration().contains(s2));
    QCOREAPPLICATION_EXEC(300);
    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s1));
    QCOREAPPLICATION_EXEC(200);
    machine.postEvent(new QEvent(static_cast<QEvent::Type>(QEvent::User+1)));
    QTRY_VERIFY(machine.configuration().contains(s2));
    QCOREAPPLICATION_EXEC(100);
    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s1));

    QTRY_COMPARE(o1->property("x").toDouble(), 15.);
    QTRY_COMPARE(o2->property("y").toDouble(), 20.);

    delete o1;
    delete o2;
}

class AssignPropertyTestState : public QState
{
    Q_OBJECT
public:
    AssignPropertyTestState(QState *parent = 0)
        : QState(parent), onEntryPassed(false), enteredPassed(false)
    { QObject::connect(this, SIGNAL(entered()), this, SLOT(onEntered())); }

    virtual void onEntry(QEvent *)
    { onEntryPassed = property("wasAssigned").toBool(); }

    bool onEntryPassed;
    bool enteredPassed;

private Q_SLOTS:
    void onEntered()
    { enteredPassed = property("wasAssigned").toBool(); }
};

void tst_QStateMachine::propertiesAreAssignedBeforeEntryCallbacks_data()
{
    QTest::addColumn<int>("restorePolicy");
    QTest::newRow("DontRestoreProperties") << int(QState::DontRestoreProperties);
    QTest::newRow("RestoreProperties") << int(QState::RestoreProperties);
}

void tst_QStateMachine::propertiesAreAssignedBeforeEntryCallbacks()
{
    QFETCH(int, restorePolicy);

    QStateMachine machine;
    machine.setGlobalRestorePolicy(static_cast<QState::RestorePolicy>(restorePolicy));

    AssignPropertyTestState *s1 = new AssignPropertyTestState(&machine);
    s1->assignProperty(s1, "wasAssigned", true);
    machine.setInitialState(s1);

    AssignPropertyTestState *s2 = new AssignPropertyTestState(&machine);
    s2->assignProperty(s2, "wasAssigned", true);
    s1->addTransition(new EventTransition(QEvent::User, s2));

    QVERIFY(!s1->property("wasAssigned").toBool());
    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));

    QVERIFY(s1->onEntryPassed);
    QVERIFY(s1->enteredPassed);

    QVERIFY(!s2->property("wasAssigned").toBool());
    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s2));

    QVERIFY(s2->onEntryPassed);
    QVERIFY(s2->enteredPassed);
}

// QTBUG-25958
void tst_QStateMachine::multiTargetTransitionInsideParallelStateGroup()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);

    QState *s2 = new QState(QState::ParallelStates, &machine);

    QState *s21 = new QState(s2);
    QState *s211 = new QState(s21);
    QState *s212 = new QState(s21);
    s21->setInitialState(s212);

    QState *s22 = new QState(s2);
    QState *s221 = new QState(s22);
    QState *s222 = new QState(s22);
    s22->setInitialState(s222);

    QAbstractTransition *t1 = new EventTransition(QEvent::User, QList<QAbstractState *>() << s211 << s221);
    s1->addTransition(t1);

    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));

    machine.postEvent(new QEvent(QEvent::User));
    QTRY_VERIFY(machine.configuration().contains(s2));
    QCOMPARE(machine.configuration().size(), 5);
    QVERIFY(machine.configuration().contains(s21));
    QVERIFY(machine.configuration().contains(s211));
    QVERIFY(machine.configuration().contains(s22));
    QVERIFY(machine.configuration().contains(s221));
}

void tst_QStateMachine::signalTransitionNormalizeSignature()
{
    QStateMachine machine;
    QState *s0 = new QState(&machine);
    machine.setInitialState(s0);
    QState *s1 = new QState(&machine);
    SignalEmitter emitter;
    TestSignalTransition *t0 = new TestSignalTransition(&emitter, SIGNAL(signalWithNoArg()), s1);
    s0->addTransition(t0);

    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s0));
    emitter.emitSignalWithNoArg();
    QTRY_VERIFY(machine.configuration().contains(s1));

    QCOMPARE(t0->eventTestSenderReceived(), (QObject*)&emitter);
    QCOMPARE(t0->eventTestSignalIndexReceived(), emitter.metaObject()->indexOfSignal("signalWithNoArg()"));
    QCOMPARE(t0->eventTestArgumentsReceived().size(), 0);
    QCOMPARE(t0->transitionSenderReceived(), (QObject*)&emitter);
    QCOMPARE(t0->transitionSignalIndexReceived(), emitter.metaObject()->indexOfSignal("signalWithNoArg()"));
    QCOMPARE(t0->transitionArgumentsReceived().size(), 0);
}

void tst_QStateMachine::createSignalTransitionWhenRunning()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));

    // Create by addTransition()
    QState *s2 = new QState(&machine);
    SignalEmitter emitter;
    QAbstractTransition *t1 = s1->addTransition(&emitter, SIGNAL(signalWithNoArg()), s2);
    QCOMPARE(t1->sourceState(), s1);
    emitter.emitSignalWithNoArg();
    QTRY_VERIFY(machine.configuration().contains(s2));

    // Create by constructor that takes sender, signal, source (parent) state
    QState *s3 = new QState(&machine);
    QSignalTransition *t2 = new QSignalTransition(&emitter, SIGNAL(signalWithNoArg()), s2);
    QCOMPARE(t2->sourceState(), s2);
    t2->setTargetState(s3);
    emitter.emitSignalWithNoArg();
    QTRY_VERIFY(machine.configuration().contains(s3));

    // Create by constructor that takes source (parent) state
    QState *s4 = new QState(&machine);
    QSignalTransition *t3 = new QSignalTransition(s3);
    QCOMPARE(t3->sourceState(), s3);
    t3->setSenderObject(&emitter);
    t3->setSignal(SIGNAL(signalWithNoArg()));
    t3->setTargetState(s4);
    emitter.emitSignalWithNoArg();
    QTRY_VERIFY(machine.configuration().contains(s4));

    // Create by constructor without parent, then set the parent
    QState *s5 = new QState(&machine);
    QSignalTransition *t4 = new QSignalTransition();
    t4->setSenderObject(&emitter);
    t4->setParent(s4);
    QCOMPARE(t4->sourceState(), s4);
    t4->setSignal(SIGNAL(signalWithNoArg()));
    t4->setTargetState(s5);
    emitter.emitSignalWithNoArg();
    QTRY_VERIFY(machine.configuration().contains(s5));
}

void tst_QStateMachine::createEventTransitionWhenRunning()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));

    // Create by constructor that takes event source, type, source (parent) state
    QState *s2 = new QState(&machine);
    QObject object;
    QEventTransition *t1 = new QEventTransition(&object, QEvent::Timer, s1);
    QCOMPARE(t1->sourceState(), s1);
    t1->setTargetState(s2);

    object.startTimer(10); // Will cause QEvent::Timer to fire every 10ms
    QTRY_VERIFY(machine.configuration().contains(s2));

    // Create by constructor that takes source (parent) state
    QState *s3 = new QState(&machine);
    QEventTransition *t2 = new QEventTransition(s2);
    QCOMPARE(t2->sourceState(), s2);
    t2->setEventSource(&object);
    t2->setEventType(QEvent::Timer);
    t2->setTargetState(s3);
    QTRY_VERIFY(machine.configuration().contains(s3));

    // Create by constructor without parent, then set the parent
    QState *s4 = new QState(&machine);
    QEventTransition *t3 = new QEventTransition();
    t3->setEventSource(&object);
    t3->setParent(s3);
    QCOMPARE(t3->sourceState(), s3);
    t3->setEventType(QEvent::Timer);
    t3->setTargetState(s4);
    QTRY_VERIFY(machine.configuration().contains(s4));
}

class SignalEmitterThread : public QThread
{
    Q_OBJECT
public:
    SignalEmitterThread(QObject *parent = 0)
        : QThread(parent)
    {
        moveToThread(this);
    }

Q_SIGNALS:
    void signal1();
    void signal2();

public Q_SLOTS:
    void emitSignals()
    {
        emit signal1();
        emit signal2();
    }
};

// QTBUG-19789
void tst_QStateMachine::signalTransitionSenderInDifferentThread()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);

    SignalEmitterThread thread;
    QState *s2 = new QState(&machine);
    s1->addTransition(&thread, SIGNAL(signal1()), s2);

    QFinalState *s3 = new QFinalState(&machine);
    s2->addTransition(&thread, SIGNAL(signal2()), s3);

    thread.start();
    QTRY_VERIFY(thread.isRunning());

    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));

    QMetaObject::invokeMethod(&thread, "emitSignals");
    // thread emits both signal1() and signal2(), so we should end in s3
    QTRY_VERIFY(machine.configuration().contains(s3));

    // Run the machine again; transitions should still be registered
    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));
    QMetaObject::invokeMethod(&thread, "emitSignals");
    QTRY_VERIFY(machine.configuration().contains(s3));

    thread.quit();
    QTRY_VERIFY(thread.wait());
}

void tst_QStateMachine::signalTransitionSenderInDifferentThread2()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);

    QState *s2 = new QState(&machine);
    SignalEmitter emitter;
    // At the time of the transition creation, the machine and the emitter
    // are both in the same thread.
    s1->addTransition(&emitter, SIGNAL(signalWithNoArg()), s2);

    QFinalState *s3 = new QFinalState(&machine);
    s2->addTransition(&emitter, SIGNAL(signalWithDefaultArg()), s3);

    QThread thread;
    // Move the machine and its states to a secondary thread, but let the
    // SignalEmitter stay in the main thread.
    machine.moveToThread(&thread);

    thread.start();
    QTRY_VERIFY(thread.isRunning());

    QSignalSpy startedSpy(&machine, SIGNAL(started()));
    QSignalSpy finishedSpy(&machine, SIGNAL(finished()));
    machine.start();
    QTRY_COMPARE(startedSpy.count(), 1);

    emitter.emitSignalWithNoArg();
    // The second emission should not get "lost".
    emitter.emitSignalWithDefaultArg();
    QTRY_COMPARE(finishedSpy.count(), 1);

    thread.quit();
    QTRY_VERIFY(thread.wait());
}

class SignalTransitionMutatorThread : public QThread
{
public:
    SignalTransitionMutatorThread(QSignalTransition *transition)
        : m_transition(transition)
    {}
    void run()
    {
        // Cause repeated registration and unregistration
        for (int i = 0; i < 50000; ++i) {
            m_transition->setSenderObject(this);
            m_transition->setSenderObject(0);
        }
    }
private:
    QSignalTransition *m_transition;
};

// Should not crash:
void tst_QStateMachine::signalTransitionRegistrationThreadSafety()
{
    QStateMachine machine;
    QState *s1 = new QState(&machine);
    machine.setInitialState(s1);
    machine.start();
    QTRY_VERIFY(machine.configuration().contains(s1));

    QSignalTransition *t1 = new QSignalTransition();
    t1->setSignal(SIGNAL(objectNameChanged(QString)));
    s1->addTransition(t1);

    QSignalTransition *t2 = new QSignalTransition();
    t2->setSignal(SIGNAL(objectNameChanged(QString)));
    s1->addTransition(t2);

    SignalTransitionMutatorThread thread(t1);
    thread.start();
    QTRY_VERIFY(thread.isRunning());

    // Cause repeated registration and unregistration
    for (int i = 0; i < 50000; ++i) {
        t2->setSenderObject(this);
        t2->setSenderObject(0);
    }

    thread.quit();
    QTRY_VERIFY(thread.wait());
}

void tst_QStateMachine::childModeConstructor()
{
    {
        QStateMachine machine(QState::ExclusiveStates);
        QCOMPARE(machine.childMode(), QState::ExclusiveStates);
        QVERIFY(machine.parent() == 0);
        QVERIFY(machine.parentState() == 0);
    }
    {
        QStateMachine machine(QState::ParallelStates);
        QCOMPARE(machine.childMode(), QState::ParallelStates);
        QVERIFY(machine.parent() == 0);
        QVERIFY(machine.parentState() == 0);
    }
    {
        QStateMachine machine(QState::ExclusiveStates, this);
        QCOMPARE(machine.childMode(), QState::ExclusiveStates);
        QCOMPARE(machine.parent(), static_cast<QObject *>(this));
        QVERIFY(machine.parentState() == 0);
    }
    {
        QStateMachine machine(QState::ParallelStates, this);
        QCOMPARE(machine.childMode(), QState::ParallelStates);
        QCOMPARE(machine.parent(), static_cast<QObject *>(this));
        QVERIFY(machine.parentState() == 0);
    }
    QState state;
    {
        QStateMachine machine(QState::ExclusiveStates, &state);
        QCOMPARE(machine.childMode(), QState::ExclusiveStates);
        QCOMPARE(machine.parent(), static_cast<QObject *>(&state));
        QCOMPARE(machine.parentState(), &state);
    }
    {
        QStateMachine machine(QState::ParallelStates, &state);
        QCOMPARE(machine.childMode(), QState::ParallelStates);
        QCOMPARE(machine.parent(), static_cast<QObject *>(&state));
        QCOMPARE(machine.parentState(), &state);
    }
}

QTEST_MAIN(tst_QStateMachine)
#include "tst_qstatemachine.moc"
