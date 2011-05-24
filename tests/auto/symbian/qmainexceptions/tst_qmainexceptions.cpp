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

#include <QtTest/QtTest>
#include <e32base.h>
#include <typeinfo>
#include <stdexcept>
#include <euserhl.h>

#ifdef Q_OS_SYMBIAN

typedef void TLeavingFunc();

class tst_qmainexceptions : public QObject
{
    Q_OBJECT
public:
    tst_qmainexceptions(){};
    ~tst_qmainexceptions(){};

    void TestSchedulerCatchesError(TLeavingFunc* f, int error);
    void TestSymbianRoundTrip(int leave, int trap);
    void TestStdRoundTrip(const std::exception& thrown, const std::exception& caught);

    bool event(QEvent *event);

public slots:
    void initTestCase();
private slots:
    void trap();
    void cleanupstack();
    void leave();
    void testTranslateBadAlloc();
    void testTranslateBigAlloc();
    void testRoundTrip();
    void testTrap();
    void testPropagation();
    void testDtor1();
    void testDtor2();
    void testNestedExceptions();
    void testScopedPointer();
    void testHybrid();
};

class CDummy : public CBase
{
public:
    CDummy(){}
    ~CDummy(){}
};

void tst_qmainexceptions::initTestCase()
{
}

void tst_qmainexceptions::trap()
{
    TTrapHandler *th= User::TrapHandler();
    QVERIFY((int)th);
}

void tst_qmainexceptions::cleanupstack()
{
    __UHEAP_MARK;
    //fails if OOM
    CDummy* dummy1 = new (ELeave) CDummy;
    __UHEAP_CHECK(1);
    CleanupStack::PushL(dummy1);
    CleanupStack::PopAndDestroy(dummy1);
    __UHEAP_MARKEND;
}

void tst_qmainexceptions::leave()
{
    __UHEAP_MARK;
    CDummy* dummy1 = 0;
    TRAPD(err,{
        CDummy* csDummy = new (ELeave) CDummy;
        CleanupStack::PushL(csDummy);
        __UHEAP_FAILNEXT(1);
        dummy1 = new (ELeave) CDummy;
        //CleanupStack::PopAndDestroy(csDummy); not executed as previous line throws
    });
    QCOMPARE(err,KErrNoMemory);
    QVERIFY(!((int)dummy1));
    __UHEAP_MARKEND;
}

class CTestActive : public CActive
{
public:
    CTestActive(TLeavingFunc* aFunc) : CActive(EPriorityStandard), iFunc(aFunc)
    {
        CActiveScheduler::Add(this);
    }
    ~CTestActive()
    {
        Cancel();
    }
    void DoCancel() {}
    void Test()
    {
        // complete this AO in a nested scheduler, to make it synchronous
        TRequestStatus* s = &iStatus;
        SetActive();
        User::RequestComplete(s, KErrNone);
        CActiveScheduler::Start();
    }
    void RunL()
    {
        (*iFunc)();
        CActiveScheduler::Stop();   // will only get here if iFunc does not leave
    }
    TInt RunError(TInt aError)
    {
        error = aError;
        CActiveScheduler::Stop();   // will only get here if iFunc leaves
        return KErrNone;
    }
public:
    TLeavingFunc* iFunc;
    int error;
};

void tst_qmainexceptions::TestSchedulerCatchesError(TLeavingFunc* f, int error)
{
    CTestActive *act = new(ELeave) CTestActive(f);
    act->Test();
    QCOMPARE(act->error, error);
    delete act;
}

void ThrowBadAlloc()
{
    throw std::bad_alloc();
}

void TranslateThrowBadAllocL()
{
    QT_TRYCATCH_LEAVING(ThrowBadAlloc());
}

void tst_qmainexceptions::testTranslateBadAlloc()
{
    // bad_alloc should give KErrNoMemory in an AO
    TestSchedulerCatchesError(&TranslateThrowBadAllocL, KErrNoMemory);
}

void BigAlloc()
{
    // allocate too much memory - it's expected that 100M ints is too much, but keep doubling if not.
    int *x = 0;
    int n = 100000000;
    do {
        x = new int[n];
        delete [] x;
        n = n * 2;
    } while (x);
}

void TranslateBigAllocL()
{
    QT_TRYCATCH_LEAVING(BigAlloc());
}

void tst_qmainexceptions::testTranslateBigAlloc()
{
    // this test will fail if new does not throw on failure, otherwise should give KErrNoMemory in AO
    TestSchedulerCatchesError(&TranslateBigAllocL, KErrNoMemory);
}

void tst_qmainexceptions::TestSymbianRoundTrip(int leave, int trap)
{
    // check that leave converted to exception, converted to error gives expected error code
    int trapped;
    QT_TRYCATCH_ERROR(
        trapped,
        QT_TRAP_THROWING(
            User::LeaveIfError(leave)));
    QCOMPARE(trap, trapped);
}

void tst_qmainexceptions::TestStdRoundTrip(const std::exception& thrown, const std::exception& caught)
{
    bool ok = false;
    try {
        QT_TRAP_THROWING(qt_symbian_exception2LeaveL(thrown));
    } catch (const std::exception& ex) {
        const std::type_info& exType = typeid(ex);
        const std::type_info& caughtType = typeid(caught);
        QCOMPARE(exType, caughtType);
        ok = true;
    }
    QCOMPARE(ok, true);
}

void tst_qmainexceptions::testRoundTrip()
{
    for (int e=-50; e<0; e++)
        TestSymbianRoundTrip(e, e);
    TestSymbianRoundTrip(KErrNone, KErrNone);
    // positive error codes are not errors
    TestSymbianRoundTrip(1, KErrNone);
    TestSymbianRoundTrip(1000000000, KErrNone);
    TestStdRoundTrip(std::bad_alloc(), std::bad_alloc());
    TestStdRoundTrip(std::invalid_argument("abc"), std::invalid_argument(""));
    TestStdRoundTrip(std::underflow_error("abc"), std::underflow_error(""));
    TestStdRoundTrip(std::overflow_error("abc"), std::overflow_error(""));
}

void tst_qmainexceptions::testTrap()
{
    // testing qt_exception2SymbianLeaveL
    TRAPD(err, qt_symbian_exception2LeaveL(std::bad_alloc()));
    QCOMPARE(err, KErrNoMemory);
}

bool tst_qmainexceptions::event(QEvent *aEvent)
{
    if (aEvent->type() == QEvent::User+1)
        throw std::bad_alloc();
    else if (aEvent->type() == QEvent::User+2) {
        QEvent event(QEvent::Type(QEvent::User+1));
        QApplication::sendEvent(this, &event);
    }
    return QObject::event(aEvent);
}

void tst_qmainexceptions::testPropagation()
{
    // test exception thrown from event is propagated back to sender
    QEvent event(QEvent::Type(QEvent::User+1));
    bool caught = false;
    try {
        QApplication::sendEvent(this, &event);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    QCOMPARE(caught, true);

    // testing nested events propagate back to top level sender
    caught = false;
    QEvent event2(QEvent::Type(QEvent::User+2));
    try {
        QApplication::sendEvent(this, &event2);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    QCOMPARE(caught, true);
}

void tst_qmainexceptions::testDtor1()
{
    // destructors work on exception
    int i = 0;
    struct SAutoInc {
        SAutoInc(int& aI) : i(aI) { ++i; }
        ~SAutoInc() { --i; }
        int &i;
    } ai(i);
    QCOMPARE(i, 1);
    try {
        SAutoInc ai2(i);
        QCOMPARE(i, 2);
        throw std::bad_alloc();
        QFAIL("should not get here");
    } catch (const std::bad_alloc&) {
        QCOMPARE(i, 1);
    }
    QCOMPARE(i, 1);
}

void tst_qmainexceptions::testDtor2()
{
    // memory is cleaned up correctly on exception
    // this crashes with winscw compiler build < 481
    __UHEAP_MARK;
    try {
        QString str("abc");
        str += "def";
        throw std::bad_alloc();
        QFAIL("should not get here");
    } catch (const std::bad_alloc&) { }
    __UHEAP_MARKEND;
}

void tst_qmainexceptions::testNestedExceptions()
{
    // throwing exceptions while handling exceptions
    struct Oops {
        Oops* next;
        Oops(int level) : next(level > 0 ? new Oops(level-1) : 0) {}
        ~Oops() {
            try { throw std::bad_alloc(); }
            catch (const std::exception&) {delete next;}
        }
    };
    try {
        Oops oops(5);
        throw std::bad_alloc();
    }
    catch (const std::exception&) {}
}

class CTestRef : public CBase
{
public:
    CTestRef(int& aX) : iX(aX) { iX++; }
    ~CTestRef() { iX--; }
    int& iX;
};

void tst_qmainexceptions::testScopedPointer()
{
    int x = 0;
    {
        QScopedPointer<CTestRef> ptr(q_check_ptr(new CTestRef(x)));
        QCOMPARE(x, 1);
    }
    QCOMPARE(x, 0);
    try {
        QScopedPointer<CTestRef> ptr(q_check_ptr(new CTestRef(x)));
        QCOMPARE(x, 1);
        throw 1;
    } catch (int) {
        QCOMPARE(x, 0);
    }
    QCOMPARE(x, 0);
}

int dtorFired[20];
int* recDtor;

class CDtorOrder : public CBase
{
public:
    CDtorOrder(TInt aId) : iId(aId) {}
    ~CDtorOrder() { *(recDtor++)=iId; }
    TInt iId;
};

class QDtorOrder
{
public:
    QDtorOrder(int aId) : iId(aId) {}
    ~QDtorOrder() { *(recDtor++)=iId; }
    int iId;
};

class RDtorOrder : public RHandleBase
{
public:
    TInt Connect(TInt aId) {iId = aId; SetHandle(aId); return KErrNone; }
    void Close() { *(recDtor++)=iId; }
    TInt iId;
};

enum THybridAction {EHybridLeave, EHybridThrow, EHybridPass};

void HybridFuncLX(THybridAction aAction)
{
    recDtor = dtorFired;
    QDtorOrder q1(1);
    {QDtorOrder q2(2);}
    CDtorOrder* c1 = new(ELeave) CDtorOrder(11);
    CleanupStack::PushL(c1);
    {LManagedHandle<RDtorOrder> r1;
    r1->Connect(21) OR_LEAVE;}
    CDtorOrder* c2 = new(ELeave) CDtorOrder(12);
    CleanupStack::PushL(c2);
    QDtorOrder q3(3);
    LManagedHandle<RDtorOrder> r2;
    r2->Connect(22) OR_LEAVE;
    CDtorOrder* c3 = new(ELeave) CDtorOrder(13);
    CleanupStack::PushL(c3);
    CleanupStack::PopAndDestroy(c3);
    QDtorOrder q4(4);
    switch (aAction)
    {
    case EHybridLeave:
        User::Leave(KErrNotFound);
        break;
    case EHybridThrow:
        throw std::bad_alloc();
        break;
    default:
        break;
    }
    CleanupStack::PopAndDestroy(2);
}

void tst_qmainexceptions::testHybrid()
{
    TRAPD(error,
        QT_TRYCATCH_LEAVING(
            HybridFuncLX(EHybridLeave);
        ) );
    QCOMPARE(error, KErrNotFound);
    int expected1[] = {2, 21, 13, 12, 11, 4, 22, 3, 1};
    QCOMPARE(int(sizeof(expected1)/sizeof(int)), int(recDtor - dtorFired));
    for (int i=0; i<sizeof(expected1)/sizeof(int); i++)
        QCOMPARE(expected1[i], dtorFired[i]);

    TRAP(error,
        QT_TRYCATCH_LEAVING(
            HybridFuncLX(EHybridThrow);
        ) );
    QCOMPARE(error, KErrNoMemory);
    int expected2[] = {2, 21, 13, 4, 22, 3, 1, 12, 11};
    QCOMPARE(int(sizeof(expected2)/sizeof(int)), int(recDtor - dtorFired));
    for (int i=0; i<sizeof(expected2)/sizeof(int); i++)
        QCOMPARE(expected2[i], dtorFired[i]);

    TRAP(error,
        QT_TRYCATCH_LEAVING(
            HybridFuncLX(EHybridPass);
        ) );
    QCOMPARE(error, KErrNone);
    int expected3[] = {2, 21, 13, 12, 11, 4, 22, 3, 1};
    QCOMPARE(int(sizeof(expected3)/sizeof(int)), int(recDtor - dtorFired));
    for (int i=0; i<sizeof(expected3)/sizeof(int); i++)
        QCOMPARE(expected3[i], dtorFired[i]);
}


QTEST_MAIN(tst_qmainexceptions)
#include "tst_qmainexceptions.moc"
#else
QTEST_NOOP_MAIN
#endif
