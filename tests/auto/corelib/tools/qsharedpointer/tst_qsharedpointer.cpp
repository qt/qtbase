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

#define QT_SHAREDPOINTER_TRACK_POINTERS
#include "qsharedpointer.h"
#include <QtTest/QtTest>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QThread>
#include <QtCore/QVector>

#include "externaltests.h"
#include "wrapper.h"

#include <stdlib.h>
#include <time.h>

QT_BEGIN_NAMESPACE
namespace QtSharedPointer {
    Q_CORE_EXPORT void internalSafetyCheckCleanCheck();
}
QT_END_NAMESPACE

#ifdef Q_OS_SYMBIAN
#define SRCDIR "."
#endif

class tst_QSharedPointer: public QObject
{
    Q_OBJECT

private slots:
    void basics_data();
    void basics();
    void operators();
    void swap();
    void forwardDeclaration1();
    void forwardDeclaration2();
    void memoryManagement();
    void downCast();
    void functionCallDownCast();
    void upCast();
    void qobjectWeakManagement();
    void noSharedPointerFromWeakQObject();
    void weakQObjectFromSharedPointer();
    void objectCast();
    void differentPointers();
    void virtualBaseDifferentPointers();
#ifndef QTEST_NO_RTTI
    void dynamicCast();
    void dynamicCastDifferentPointers();
    void dynamicCastVirtualBase();
    void dynamicCastFailure();
#endif
    void constCorrectness();
    void customDeleter();
    void creating();
    void creatingQObject();
    void mixTrackingPointerCode();
    void reentrancyWhileDestructing();

    void threadStressTest_data();
    void threadStressTest();
    void map();
    void hash();
    void validConstructs();
    void invalidConstructs_data();
    void invalidConstructs();

public slots:
    void cleanup() { check(); }

public:
    inline void check()
    {
#ifdef QT_BUILD_INTERNAL
        QtSharedPointer::internalSafetyCheckCleanCheck();
#endif
    }
};

template <typename Base>
class RefCountHack: public Base
{
public:
    using Base::d;
};
template<typename Base> static inline
QtSharedPointer::ExternalRefCountData *refCountData(const Base &b)
{ return static_cast<const RefCountHack<Base> *>(&b)->d; }

class Data
{
public:
    static int destructorCounter;
    static int generationCounter;
    int generation;

    Data() : generation(++generationCounter)
    { }

    virtual ~Data()
    {
        if (generation <= 0)
            qFatal("tst_qsharedpointer: Double deletion!");
        generation = 0;
        ++destructorCounter;
    }

    void doDelete()
    {
        delete this;
    }

    bool alsoDelete()
    {
        doDelete();
        return true;
    }

    virtual void virtualDelete()
    {
        delete this;
    }

    virtual int classLevel() { return 1; }
};
int Data::generationCounter = 0;
int Data::destructorCounter = 0;

void tst_QSharedPointer::basics_data()
{
    QTest::addColumn<bool>("isNull");
    QTest::newRow("null") << true;
    QTest::newRow("non-null") << false;
}

void tst_QSharedPointer::basics()
{
    {
        QSharedPointer<Data> ptr;
        QWeakPointer<Data> weakref;

        QCOMPARE(sizeof(ptr), 2*sizeof(void*));
        QCOMPARE(sizeof(weakref), 2*sizeof(void*));
    }

    QFETCH(bool, isNull);
    Data *aData = 0;
    if (!isNull)
        aData = new Data;
    Data *otherData = new Data;
    QSharedPointer<Data> ptr(aData);

    {
        // basic self tests
        QCOMPARE(ptr.isNull(), isNull);
        QCOMPARE(bool(ptr), !isNull);
        QCOMPARE(!ptr, isNull);

        QCOMPARE(ptr.data(), aData);
        QCOMPARE(ptr.operator->(), aData);
        Data &dataReference = *ptr;
        QCOMPARE(&dataReference, aData);

        QVERIFY(ptr == aData);
        QVERIFY(!(ptr != aData));
        QVERIFY(aData == ptr);
        QVERIFY(!(aData != ptr));

        QVERIFY(ptr != otherData);
        QVERIFY(otherData != ptr);
        QVERIFY(! (ptr == otherData));
        QVERIFY(! (otherData == ptr));
    }
    QVERIFY(!refCountData(ptr) || refCountData(ptr)->weakref == 1);
    QVERIFY(!refCountData(ptr) || refCountData(ptr)->strongref == 1);

    {
        // create another object:
        QSharedPointer<Data> otherCopy(otherData);
        QVERIFY(ptr != otherCopy);
        QVERIFY(otherCopy != ptr);
        QVERIFY(! (ptr == otherCopy));
        QVERIFY(! (otherCopy == ptr));

        // otherData is deleted here
    }
    QVERIFY(!refCountData(ptr) || refCountData(ptr)->weakref == 1);
    QVERIFY(!refCountData(ptr) || refCountData(ptr)->strongref == 1);

    {
        // create a copy:
        QSharedPointer<Data> copy(ptr);
        QVERIFY(copy == ptr);
        QVERIFY(ptr == copy);
        QVERIFY(! (copy != ptr));
        QVERIFY(! (ptr != copy));
        QCOMPARE(copy, ptr);
        QCOMPARE(ptr, copy);

        QCOMPARE(copy.isNull(), isNull);
        QCOMPARE(copy.data(), aData);
        QVERIFY(copy == aData);
    }
    QVERIFY(!refCountData(ptr) || refCountData(ptr)->weakref == 1);
    QVERIFY(!refCountData(ptr) || refCountData(ptr)->strongref == 1);

    {
        // create a weak reference:
        QWeakPointer<Data> weak(ptr);
        QCOMPARE(weak.isNull(), isNull);
        QCOMPARE(!weak, isNull);
        QCOMPARE(bool(weak), !isNull);

        QVERIFY(ptr == weak);
        QVERIFY(weak == ptr);
        QVERIFY(! (ptr != weak));
        QVERIFY(! (weak != ptr));

        // create another reference:
        QWeakPointer<Data> weak2(weak);
        QCOMPARE(weak2.isNull(), isNull);
        QCOMPARE(!weak2, isNull);
        QCOMPARE(bool(weak2), !isNull);

        QVERIFY(weak2 == weak);
        QVERIFY(weak == weak2);
        QVERIFY(! (weak2 != weak));
        QVERIFY(! (weak != weak2));

        // create a strong reference back:
        QSharedPointer<Data> strong(weak);
        QVERIFY(strong == weak);
        QVERIFY(strong == ptr);
        QCOMPARE(strong.data(), aData);
    }
    QVERIFY(!refCountData(ptr) || refCountData(ptr)->weakref == 1);
    QVERIFY(!refCountData(ptr) || refCountData(ptr)->strongref == 1);

    // aData is deleted here
}

void tst_QSharedPointer::operators()
{
    QSharedPointer<char> p1;
    QSharedPointer<char> p2(new char);
    qptrdiff diff = p2.data() - p1.data();
    QVERIFY(p1.data() != p2.data());
    QVERIFY(diff != 0);

    // operator-
    QCOMPARE(p2 - p1.data(), diff);
    QCOMPARE(p2.data() - p1, diff);
    QCOMPARE(p2 - p1, diff);
    QCOMPARE(p1 - p2, -diff);
    QCOMPARE(p1 - p1, qptrdiff(0));
    QCOMPARE(p2 - p2, qptrdiff(0));

    // operator<
    QVERIFY(p1 < p2.data());
    QVERIFY(p1.data() < p2);
    QVERIFY(p1 < p2);
    QVERIFY(!(p2 < p1));
    QVERIFY(!(p2 < p2));
    QVERIFY(!(p1 < p1));

    // qHash
    QCOMPARE(qHash(p1), qHash(p1.data()));
    QCOMPARE(qHash(p2), qHash(p2.data()));
}

void tst_QSharedPointer::swap()
{
    QSharedPointer<int> p1, p2(new int(42)), control = p2;
    QVERIFY(p1 != control);
    QVERIFY(p1.isNull());
    QVERIFY(p2 == control);
    QVERIFY(!p2.isNull());
    QVERIFY(*p2 == 42);

    p1.swap(p2);
    QVERIFY(p1 == control);
    QVERIFY(!p1.isNull());
    QVERIFY(p2 != control);
    QVERIFY(p2.isNull());
    QVERIFY(*p1 == 42);

    p1.swap(p2);
    QVERIFY(p1 != control);
    QVERIFY(p1.isNull());
    QVERIFY(p2 == control);
    QVERIFY(!p2.isNull());
    QVERIFY(*p2 == 42);

    qSwap(p1, p2);
    QVERIFY(p1 == control);
    QVERIFY(!p1.isNull());
    QVERIFY(p2 != control);
    QVERIFY(p2.isNull());
    QVERIFY(*p1 == 42);
}

class ForwardDeclared;
ForwardDeclared *forwardPointer();
void externalForwardDeclaration();
extern int forwardDeclaredDestructorRunCount;

void tst_QSharedPointer::forwardDeclaration1()
{
#if defined(Q_CC_SUN) || defined(Q_CC_WINSCW) || defined(Q_CC_RVCT)
    QSKIP("This type of forward declaration is not valid with this compiler", SkipAll);
#else
    externalForwardDeclaration();

    struct Wrapper { QSharedPointer<ForwardDeclared> pointer; };

    forwardDeclaredDestructorRunCount = 0;
    {
        Wrapper w;
        w.pointer = QSharedPointer<ForwardDeclared>(forwardPointer());
        QVERIFY(!w.pointer.isNull());
    }
    QCOMPARE(forwardDeclaredDestructorRunCount, 1);
#endif
}

#include "forwarddeclared.h"

void tst_QSharedPointer::forwardDeclaration2()
{
    forwardDeclaredDestructorRunCount = 0;
    {
        struct Wrapper { QSharedPointer<ForwardDeclared> pointer; };
        Wrapper w1, w2;
        w1.pointer = QSharedPointer<ForwardDeclared>(forwardPointer());
        QVERIFY(!w1.pointer.isNull());
    }
    QCOMPARE(forwardDeclaredDestructorRunCount, 1);
}

void tst_QSharedPointer::memoryManagement()
{
    int generation = Data::generationCounter + 1;
    int destructorCounter = Data::destructorCounter;

    QSharedPointer<Data> ptr = QSharedPointer<Data>(new Data);
    QCOMPARE(ptr->generation, generation);
    QCOMPARE(Data::destructorCounter, destructorCounter);
    QCOMPARE(Data::generationCounter, generation);

    ptr = ptr;
    QCOMPARE(ptr->generation, generation);
    QCOMPARE(Data::destructorCounter, destructorCounter);
    QCOMPARE(Data::generationCounter, generation);

    {
        QSharedPointer<Data> copy = ptr;
        QCOMPARE(ptr->generation, generation);
        QCOMPARE(copy->generation, generation);

        // copy goes out of scope, ptr continues
    }
    QCOMPARE(ptr->generation, generation);
    QCOMPARE(Data::destructorCounter, destructorCounter);
    QCOMPARE(Data::generationCounter, generation);

    {
        QWeakPointer<Data> weak = ptr;
        weak = ptr;
        QCOMPARE(ptr->generation, generation);
        QCOMPARE(Data::destructorCounter, destructorCounter);
        QCOMPARE(Data::generationCounter, generation);

        weak = weak;
        QCOMPARE(ptr->generation, generation);
        QCOMPARE(Data::destructorCounter, destructorCounter);
        QCOMPARE(Data::generationCounter, generation);

        QSharedPointer<Data> strong = weak;
        QCOMPARE(ptr->generation, generation);
        QCOMPARE(strong->generation, generation);
        QCOMPARE(Data::destructorCounter, destructorCounter);
        QCOMPARE(Data::generationCounter, generation);

        // both weak and strong go out of scope
    }
    QCOMPARE(ptr->generation, generation);
    QCOMPARE(Data::destructorCounter, destructorCounter);
    QCOMPARE(Data::generationCounter, generation);

    QWeakPointer<Data> weak = ptr;
    ptr = QSharedPointer<Data>();

    // destructor must have been called
    QCOMPARE(Data::destructorCounter, destructorCounter + 1);
    QVERIFY(ptr.isNull());
    QVERIFY(weak.isNull());

    // if we create a strong pointer from the weak, it must still be null
    ptr = weak;
    QVERIFY(ptr.isNull());
    QVERIFY(ptr == 0);
    QCOMPARE(ptr.data(), (Data*)0);
}

class DerivedData: public Data
{
public:
    static int derivedDestructorCounter;
    int moreData;
    DerivedData() : moreData(0) { }
    ~DerivedData() { ++derivedDestructorCounter; }

    virtual void virtualDelete()
    {
        delete this;
    }

    virtual int classLevel() { return 2; }
};
int DerivedData::derivedDestructorCounter = 0;

class Stuffing
{
public:
    char buffer[16];
    Stuffing() { for (uint i = 0; i < sizeof buffer; ++i) buffer[i] = 16 - i; }
    virtual ~Stuffing() { }
};

class DiffPtrDerivedData: public Stuffing, public Data
{
public:
    virtual int classLevel() { return 3; }
};

class VirtualDerived: virtual public Data
{
public:
    int moreData;

    VirtualDerived() : moreData(0xc0ffee) { }
    virtual int classLevel() { return 4; }
};

void tst_QSharedPointer::downCast()
{
    {
        QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData);
        QSharedPointer<Data> baseptr = qSharedPointerCast<Data>(ptr);
        QSharedPointer<Data> other;

        QVERIFY(ptr == baseptr);
        QVERIFY(baseptr == ptr);
        QVERIFY(! (ptr != baseptr));
        QVERIFY(! (baseptr != ptr));

        QVERIFY(ptr != other);
        QVERIFY(other != ptr);
        QVERIFY(! (ptr == other));
        QVERIFY(! (other == ptr));
    }

    {
        QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData);
        QSharedPointer<Data> baseptr = ptr;
    }

    int destructorCount;
    destructorCount = DerivedData::derivedDestructorCounter;
    {
        QSharedPointer<Data> baseptr;
        {
            QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData);
            baseptr = ptr;
            QVERIFY(baseptr == ptr);
        }
    }
    QCOMPARE(DerivedData::derivedDestructorCounter, destructorCount + 1);

    destructorCount = DerivedData::derivedDestructorCounter;
    {
        QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData);
        QWeakPointer<Data> baseptr = ptr;
        QVERIFY(baseptr == ptr);

        ptr = QSharedPointer<DerivedData>();
        QVERIFY(baseptr.isNull());
    }
    QCOMPARE(DerivedData::derivedDestructorCounter, destructorCount + 1);

    destructorCount = DerivedData::derivedDestructorCounter;
    {
        QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData);
        QWeakPointer<DerivedData> weakptr(ptr);

        QSharedPointer<Data> baseptr = weakptr;
        QVERIFY(baseptr == ptr);
        QWeakPointer<Data> baseweakptr = weakptr;
        QVERIFY(baseweakptr == ptr);
    }
    QCOMPARE(DerivedData::derivedDestructorCounter, destructorCount + 1);
}

void functionDataByValue(QSharedPointer<Data> p) { Q_UNUSED(p); };
void functionDataByRef(const QSharedPointer<Data> &p) { Q_UNUSED(p); };
void tst_QSharedPointer::functionCallDownCast()
{
    QSharedPointer<DerivedData> p(new DerivedData());
    functionDataByValue(p);
    functionDataByRef(p);
}

void tst_QSharedPointer::upCast()
{
    QSharedPointer<Data> baseptr = QSharedPointer<Data>(new DerivedData);

    {
        QSharedPointer<DerivedData> derivedptr = qSharedPointerCast<DerivedData>(baseptr);
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QWeakPointer<DerivedData> derivedptr = qWeakPointerCast<DerivedData>(baseptr);
        QVERIFY(baseptr == derivedptr);
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QWeakPointer<Data> weakptr = baseptr;
        QSharedPointer<DerivedData> derivedptr = qSharedPointerCast<DerivedData>(weakptr);
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QSharedPointer<DerivedData> derivedptr = baseptr.staticCast<DerivedData>();
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);
}

class OtherObject: public QObject
{
    Q_OBJECT
};

void tst_QSharedPointer::qobjectWeakManagement()
{
    {
        QWeakPointer<QObject> weak;
        weak = QWeakPointer<QObject>();
        QVERIFY(weak.isNull());
        QVERIFY(!weak.data());
    }

    {
        QObject *obj = new QObject;
        QWeakPointer<QObject> weak(obj);
        QVERIFY(!weak.isNull());
        QVERIFY(weak.data() == obj);

        // now delete
        delete obj;
        QVERIFY(weak.isNull());
    }
    check();

    {
        // same, bit with operator=
        QObject *obj = new QObject;
        QWeakPointer<QObject> weak;
        weak = obj;
        QVERIFY(!weak.isNull());
        QVERIFY(weak.data() == obj);

        // now delete
        delete obj;
        QVERIFY(weak.isNull());
    }
    check();

    {
        // delete triggered by parent
        QObject *obj, *parent;
        parent = new QObject;
        obj = new QObject(parent);
        QWeakPointer<QObject> weak(obj);

        // now delete the parent
        delete parent;
        QVERIFY(weak.isNull());
    }
    check();

    {
        // same as above, but set the parent after QWeakPointer is created
        QObject *obj, *parent;
        obj = new QObject;
        QWeakPointer<QObject> weak(obj);

        parent = new QObject;
        obj->setParent(parent);

        // now delete the parent
        delete parent;
        QVERIFY(weak.isNull());
    }
    check();

    {
        // with two QWeakPointers
        QObject *obj = new QObject;
        QWeakPointer<QObject> weak(obj);

        {
            QWeakPointer<QObject> weak2(obj);
            QVERIFY(!weak2.isNull());
            QVERIFY(weak == weak2);
        }
        QVERIFY(!weak.isNull());

        delete obj;
        QVERIFY(weak.isNull());
    }
    check();

    {
        // same, but delete the pointer while two QWeakPointers exist
        QObject *obj = new QObject;
        QWeakPointer<QObject> weak(obj);

        {
            QWeakPointer<QObject> weak2(obj);
            QVERIFY(!weak2.isNull());

            delete obj;
            QVERIFY(weak.isNull());
            QVERIFY(weak2.isNull());
        }
        QVERIFY(weak.isNull());
    }
    check();
}

void tst_QSharedPointer::noSharedPointerFromWeakQObject()
{
    // you're not allowed to create a QSharedPointer from an unmanaged QObject
    QObject obj;
    QWeakPointer<QObject> weak(&obj);

    QSharedPointer<QObject> strong = weak.toStrongRef();
    QVERIFY(strong.isNull());

    // if something went wrong, we'll probably crash here
}

void tst_QSharedPointer::weakQObjectFromSharedPointer()
{
    // this is the inverse of the above: you're allowed to create a QWeakPointer
    // from a managed QObject
    QSharedPointer<QObject> shared(new QObject);
    QWeakPointer<QObject> weak = shared.data();
    QVERIFY(!weak.isNull());

    // delete:
    shared.clear();
    QVERIFY(weak.isNull());
}

void tst_QSharedPointer::objectCast()
{
    {
        OtherObject *data = new OtherObject;
        QSharedPointer<QObject> baseptr = QSharedPointer<QObject>(data);
        QVERIFY(baseptr == data);
        QVERIFY(data == baseptr);

        // perform object cast
        QSharedPointer<OtherObject> ptr = qSharedPointerObjectCast<OtherObject>(baseptr);
        QVERIFY(!ptr.isNull());
        QCOMPARE(ptr.data(), data);
        QVERIFY(ptr == data);

        // again:
        ptr = baseptr.objectCast<OtherObject>();
        QVERIFY(ptr == data);

        // again:
        ptr = qobject_cast<OtherObject *>(baseptr);
        QVERIFY(ptr == data);

        // again:
        ptr = qobject_cast<QSharedPointer<OtherObject> >(baseptr);
        QVERIFY(ptr == data);
    }
    check();

    {
        const OtherObject *data = new OtherObject;
        QSharedPointer<const QObject> baseptr = QSharedPointer<const QObject>(data);
        QVERIFY(baseptr == data);
        QVERIFY(data == baseptr);

        // perform object cast
        QSharedPointer<const OtherObject> ptr = qSharedPointerObjectCast<const OtherObject>(baseptr);
        QVERIFY(!ptr.isNull());
        QCOMPARE(ptr.data(), data);
        QVERIFY(ptr == data);

        // again:
        ptr = baseptr.objectCast<const OtherObject>();
        QVERIFY(ptr == data);

        // again:
        ptr = qobject_cast<const OtherObject *>(baseptr);
        QVERIFY(ptr == data);

        // again:
        ptr = qobject_cast<QSharedPointer<const OtherObject> >(baseptr);
        QVERIFY(ptr == data);
    }
    check();

    {
        OtherObject *data = new OtherObject;
        QPointer<OtherObject> qptr = data;
        QSharedPointer<OtherObject> ptr = QSharedPointer<OtherObject>(data);
        QWeakPointer<QObject> weakptr = ptr;

        {
            // perform object cast
            QSharedPointer<OtherObject> otherptr = qSharedPointerObjectCast<OtherObject>(weakptr);
            QVERIFY(otherptr == ptr);

            // again:
            otherptr = qobject_cast<OtherObject *>(weakptr);
            QVERIFY(otherptr == ptr);

            // again:
            otherptr = qobject_cast<QSharedPointer<OtherObject> >(weakptr);
            QVERIFY(otherptr == ptr);
        }

        // drop the reference:
        ptr.clear();
        QVERIFY(ptr.isNull());
        QVERIFY(qptr.isNull());
        QVERIFY(weakptr.toStrongRef().isNull());

        // verify that the object casts fail without crash
        QSharedPointer<OtherObject> otherptr = qSharedPointerObjectCast<OtherObject>(weakptr);
        QVERIFY(otherptr.isNull());

        // again:
        otherptr = qobject_cast<OtherObject *>(weakptr);
        QVERIFY(otherptr.isNull());

        // again:
        otherptr = qobject_cast<QSharedPointer<OtherObject> >(weakptr);
        QVERIFY(otherptr.isNull());
    }
    check();
}

void tst_QSharedPointer::differentPointers()
{
    {
        DiffPtrDerivedData *aData = new DiffPtrDerivedData;
        Data *aBase = aData;

        // ensure that this compiler isn't broken
        if (*reinterpret_cast<quintptr *>(&aData) == *reinterpret_cast<quintptr *>(&aBase))
            qFatal("Something went very wrong -- we couldn't create two different pointers to the same object");
        if (aData != aBase)
            QSKIP("Broken compiler", SkipAll);
        if (aBase != aData)
            QSKIP("Broken compiler", SkipAll);

        QSharedPointer<DiffPtrDerivedData> ptr = QSharedPointer<DiffPtrDerivedData>(aData);
        QSharedPointer<Data> baseptr = qSharedPointerCast<Data>(ptr);
        qDebug("naked: orig: %p; base: %p (%s) -- QSharedPointer: orig: %p; base %p (%s) -- result: %s",
               aData, aBase, aData == aBase ? "equal" : "not equal",
               ptr.data(), baseptr.data(), ptr.data() == baseptr.data() ? "equal" : "not equal",
               baseptr.data() == aData ? "equal" : "not equal");

        QVERIFY(ptr.data() == baseptr.data());
        QVERIFY(baseptr.data() == ptr.data());
        QVERIFY(ptr == baseptr);
        QVERIFY(baseptr == ptr);

        QVERIFY(ptr.data() == aBase);
        QVERIFY(aBase == ptr.data());
        QVERIFY(ptr.data() == aData);
        QVERIFY(aData == ptr.data());

        QVERIFY(ptr == aBase);
        QVERIFY(aBase == ptr);
        QVERIFY(ptr == aData);
        QVERIFY(aData == ptr);

        QVERIFY(baseptr.data() == aBase);
        QVERIFY(aBase == baseptr.data());
        QVERIFY(baseptr == aBase);
        QVERIFY(aBase == baseptr);

        QVERIFY(baseptr.data() == aData);
        QVERIFY(aData == baseptr.data());

#if defined(Q_CC_MSVC) && _MSC_VER < 1400
        QEXPECT_FAIL("", "Compiler bug", Continue);
#endif
        QVERIFY(baseptr == aData);
#if defined(Q_CC_MSVC) && _MSC_VER < 1400
        QEXPECT_FAIL("", "Compiler bug", Continue);
#endif
        QVERIFY(aData == baseptr);
    }
    check();

    {
        DiffPtrDerivedData *aData = new DiffPtrDerivedData;
        Data *aBase = aData;
        QVERIFY(aData == aBase);
        QVERIFY(*reinterpret_cast<quintptr *>(&aData) != *reinterpret_cast<quintptr *>(&aBase));

        QSharedPointer<Data> baseptr = QSharedPointer<Data>(aData);
        QSharedPointer<DiffPtrDerivedData> ptr = qSharedPointerCast<DiffPtrDerivedData>(baseptr);
        QVERIFY(ptr == baseptr);
        QVERIFY(ptr.data() == baseptr.data());
        QVERIFY(ptr == aBase);
#if defined(Q_CC_MSVC) && _MSC_VER < 1400
        QEXPECT_FAIL("", "Compiler bug", Continue);
#endif
        QVERIFY(baseptr == aData);
    }
    check();

    {
        DiffPtrDerivedData *aData = new DiffPtrDerivedData;
        Data *aBase = aData;
        QVERIFY(aData == aBase);
        QVERIFY(*reinterpret_cast<quintptr *>(&aData) != *reinterpret_cast<quintptr *>(&aBase));

        QSharedPointer<DiffPtrDerivedData> ptr = QSharedPointer<DiffPtrDerivedData>(aData);
        QSharedPointer<Data> baseptr = ptr;
        QVERIFY(ptr == baseptr);
        QVERIFY(ptr.data() == baseptr.data());
        QVERIFY(ptr == aBase);
        QVERIFY(ptr == aData);
#if defined(Q_CC_MSVC) && _MSC_VER < 1400
        QEXPECT_FAIL("", "Compiler bug", Continue);
#endif
        QVERIFY(baseptr == aData);
        QVERIFY(baseptr == aBase);
    }
    check();
}

void tst_QSharedPointer::virtualBaseDifferentPointers()
{
    {
        VirtualDerived *aData = new VirtualDerived;
        Data *aBase = aData;
        QVERIFY(aData == aBase);
        QVERIFY(*reinterpret_cast<quintptr *>(&aData) != *reinterpret_cast<quintptr *>(&aBase));

        QSharedPointer<VirtualDerived> ptr = QSharedPointer<VirtualDerived>(aData);
        QSharedPointer<Data> baseptr = qSharedPointerCast<Data>(ptr);
        QVERIFY(ptr == baseptr);
        QVERIFY(ptr.data() == baseptr.data());
        QVERIFY(ptr == aBase);
        QVERIFY(ptr == aData);
#if defined(Q_CC_MSVC) && _MSC_VER < 1400
        QEXPECT_FAIL("", "Compiler bug", Continue);
#endif
        QVERIFY(baseptr == aData);
        QVERIFY(baseptr == aBase);
    }
    check();

    {
        VirtualDerived *aData = new VirtualDerived;
        Data *aBase = aData;
        QVERIFY(aData == aBase);
        QVERIFY(*reinterpret_cast<quintptr *>(&aData) != *reinterpret_cast<quintptr *>(&aBase));

        QSharedPointer<VirtualDerived> ptr = QSharedPointer<VirtualDerived>(aData);
        QSharedPointer<Data> baseptr = ptr;
        QVERIFY(ptr == baseptr);
        QVERIFY(ptr.data() == baseptr.data());
        QVERIFY(ptr == aBase);
        QVERIFY(ptr == aData);
#if defined(Q_CC_MSVC) && _MSC_VER < 1400
        QEXPECT_FAIL("", "Compiler bug", Continue);
#endif
        QVERIFY(baseptr == aData);
        QVERIFY(baseptr == aBase);
    }
    check();
}

#ifndef QTEST_NO_RTTI
void tst_QSharedPointer::dynamicCast()
{
    DerivedData *aData = new DerivedData;
    QSharedPointer<Data> baseptr = QSharedPointer<Data>(aData);

    {
        QSharedPointer<DerivedData> derivedptr = qSharedPointerDynamicCast<DerivedData>(baseptr);
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(derivedptr.data(), aData);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QWeakPointer<Data> weakptr = baseptr;
        QSharedPointer<DerivedData> derivedptr = qSharedPointerDynamicCast<DerivedData>(weakptr);
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(derivedptr.data(), aData);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QSharedPointer<DerivedData> derivedptr = baseptr.dynamicCast<DerivedData>();
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(derivedptr.data(), aData);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);
}

void tst_QSharedPointer::dynamicCastDifferentPointers()
{
    // DiffPtrDerivedData derives from both Data and Stuffing
    DiffPtrDerivedData *aData = new DiffPtrDerivedData;
    QSharedPointer<Data> baseptr = QSharedPointer<Data>(aData);

    {
        QSharedPointer<DiffPtrDerivedData> derivedptr = qSharedPointerDynamicCast<DiffPtrDerivedData>(baseptr);
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(derivedptr.data(), aData);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QWeakPointer<Data> weakptr = baseptr;
        QSharedPointer<DiffPtrDerivedData> derivedptr = qSharedPointerDynamicCast<DiffPtrDerivedData>(weakptr);
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(derivedptr.data(), aData);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QSharedPointer<DiffPtrDerivedData> derivedptr = baseptr.dynamicCast<DiffPtrDerivedData>();
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(derivedptr.data(), aData);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        Stuffing *nakedptr = dynamic_cast<Stuffing *>(baseptr.data());
        QVERIFY(nakedptr);

        QSharedPointer<Stuffing> otherbaseptr = qSharedPointerDynamicCast<Stuffing>(baseptr);
        QVERIFY(!otherbaseptr.isNull());
        QVERIFY(otherbaseptr == nakedptr);
        QCOMPARE(otherbaseptr.data(), nakedptr);
        QCOMPARE(static_cast<DiffPtrDerivedData*>(otherbaseptr.data()), aData);
    }
}

void tst_QSharedPointer::dynamicCastVirtualBase()
{
    VirtualDerived *aData = new VirtualDerived;
    QSharedPointer<Data> baseptr = QSharedPointer<Data>(aData);

    {
        QSharedPointer<VirtualDerived> derivedptr = qSharedPointerDynamicCast<VirtualDerived>(baseptr);
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(derivedptr.data(), aData);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QWeakPointer<Data> weakptr = baseptr;
        QSharedPointer<VirtualDerived> derivedptr = qSharedPointerDynamicCast<VirtualDerived>(weakptr);
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(derivedptr.data(), aData);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QSharedPointer<VirtualDerived> derivedptr = baseptr.dynamicCast<VirtualDerived>();
        QVERIFY(baseptr == derivedptr);
        QCOMPARE(derivedptr.data(), aData);
        QCOMPARE(static_cast<Data *>(derivedptr.data()), baseptr.data());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);
}

void tst_QSharedPointer::dynamicCastFailure()
{
    QSharedPointer<Data> baseptr = QSharedPointer<Data>(new Data);
    QVERIFY(dynamic_cast<DerivedData *>(baseptr.data()) == 0);

    {
        QSharedPointer<DerivedData> derivedptr = qSharedPointerDynamicCast<DerivedData>(baseptr);
        QVERIFY(derivedptr.isNull());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);

    {
        QSharedPointer<DerivedData> derivedptr = baseptr.dynamicCast<DerivedData>();
        QVERIFY(derivedptr.isNull());
    }
    QCOMPARE(int(refCountData(baseptr)->weakref), 1);
    QCOMPARE(int(refCountData(baseptr)->strongref), 1);
}
#endif

void tst_QSharedPointer::constCorrectness()
{
    {
        QSharedPointer<Data> ptr = QSharedPointer<Data>(new Data);
        QSharedPointer<const Data> cptr(ptr);
        QSharedPointer<volatile Data> vptr(ptr);
        cptr = ptr;
        vptr = ptr;

        ptr = qSharedPointerConstCast<Data>(cptr);
        ptr = qSharedPointerConstCast<Data>(vptr);
        ptr = cptr.constCast<Data>();
        ptr = vptr.constCast<Data>();

#if !defined(Q_CC_HPACC) && !defined(QT_ARCH_PARISC)
        // the aCC series 3 compiler we have on the PA-RISC
        // machine crashes compiling this code

        QSharedPointer<const volatile Data> cvptr(ptr);
        QSharedPointer<const volatile Data> cvptr2(cptr);
        QSharedPointer<const volatile Data> cvptr3(vptr);
        cvptr = ptr;
        cvptr2 = cptr;
        cvptr3 = vptr;
        ptr = qSharedPointerConstCast<Data>(cvptr);
        ptr = cvptr.constCast<Data>();
#endif
    }
    check();

    {
        Data *aData = new Data;
        QSharedPointer<Data> ptr = QSharedPointer<Data>(aData);
        const QSharedPointer<Data> cptr = ptr;

        ptr = cptr;
        QSharedPointer<Data> other = qSharedPointerCast<Data>(cptr);

#ifndef QT_NO_DYNAMIC_CAST
        other = qSharedPointerDynamicCast<Data>(cptr);
#endif

        QCOMPARE(cptr.data(), aData);
        QCOMPARE(cptr.operator->(), aData);
    }
    check();
}

static int customDeleterFnCallCount;
void customDeleterFn(Data *ptr)
{
    ++customDeleterFnCallCount;
    delete ptr;
}

static int refcount;

template <typename T>
struct CustomDeleter
{
    CustomDeleter() { ++refcount; }
    CustomDeleter(const CustomDeleter &) { ++refcount; }
    ~CustomDeleter() { --refcount; }
    inline void operator()(T *ptr)
    {
        delete ptr;
        ++callCount;
    }
    static int callCount;
};
template<typename T> int CustomDeleter<T>::callCount = 0;

void tst_QSharedPointer::customDeleter()
{
    {
        QSharedPointer<Data> ptr(new Data, &Data::doDelete);
        QSharedPointer<Data> ptr2(new Data, &Data::alsoDelete);
        QSharedPointer<Data> ptr3(new Data, &Data::virtualDelete);
    }
    check();
    {
        QSharedPointer<DerivedData> ptr(new DerivedData, &Data::doDelete);
        QSharedPointer<DerivedData> ptr2(new DerivedData, &Data::alsoDelete);
        QSharedPointer<DerivedData> ptr3(new DerivedData, &Data::virtualDelete);
    }
    check();

    customDeleterFnCallCount = 0;
    {
        QSharedPointer<Data> ptr = QSharedPointer<Data>(new Data, customDeleterFn);
        ptr.data();
        QCOMPARE(customDeleterFnCallCount, 0);
    }
    QCOMPARE(customDeleterFnCallCount, 1);
    check();

    customDeleterFnCallCount = 0;
    {
        QSharedPointer<Data> ptr = QSharedPointer<Data>(new Data, customDeleterFn);
        QCOMPARE(customDeleterFnCallCount, 0);
        ptr.clear();
        QCOMPARE(customDeleterFnCallCount, 1);
    }
    QCOMPARE(customDeleterFnCallCount, 1);
    check();

    customDeleterFnCallCount = 0;
    {
        QSharedPointer<Data> ptr = QSharedPointer<Data>(new Data, customDeleterFn);
        QCOMPARE(customDeleterFnCallCount, 0);
        ptr = QSharedPointer<Data>(new Data);
        QCOMPARE(customDeleterFnCallCount, 1);
    }
    QCOMPARE(customDeleterFnCallCount, 1);
    check();

    customDeleterFnCallCount = 0;
    {
        QSharedPointer<Data> ptr = QSharedPointer<Data>(new DerivedData, customDeleterFn);
        ptr.data();
        QCOMPARE(customDeleterFnCallCount, 0);
    }
    QCOMPARE(customDeleterFnCallCount, 1);
    check();

    customDeleterFnCallCount = 0;
    {
        QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData, customDeleterFn);
        ptr.data();
        QCOMPARE(customDeleterFnCallCount, 0);
    }
    QCOMPARE(customDeleterFnCallCount, 1);
    check();

    customDeleterFnCallCount = 0;
    {
        QSharedPointer<Data> other;
        {
            QSharedPointer<Data> ptr = QSharedPointer<Data>(new Data, customDeleterFn);
            other = ptr;
            QCOMPARE(customDeleterFnCallCount, 0);
        }
        QCOMPARE(customDeleterFnCallCount, 0);
    }
    QCOMPARE(customDeleterFnCallCount, 1);
    check();

    customDeleterFnCallCount = 0;
    {
        QSharedPointer<Data> other;
        {
            QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData, customDeleterFn);
            other = ptr;
            QCOMPARE(customDeleterFnCallCount, 0);
        }
        QCOMPARE(customDeleterFnCallCount, 0);
    }
    QCOMPARE(customDeleterFnCallCount, 1);
    check();

    refcount = 0;
    CustomDeleter<Data> dataDeleter;
    dataDeleter.callCount = 0;
    {
        QSharedPointer<Data> ptr = QSharedPointer<Data>(new Data, dataDeleter);
        ptr.data();
        QCOMPARE(dataDeleter.callCount, 0);
    }
    QCOMPARE(dataDeleter.callCount, 1);
    QCOMPARE(refcount, 1);
    check();

    dataDeleter.callCount = 0;
    {
        QSharedPointer<Data> ptr = QSharedPointer<Data>(new Data, dataDeleter);
        QSharedPointer<Data> other = ptr;
        other.clear();
        QCOMPARE(dataDeleter.callCount, 0);
    }
    QCOMPARE(dataDeleter.callCount, 1);
    QCOMPARE(refcount, 1);
    check();

    dataDeleter.callCount = 0;
    {
        QSharedPointer<Data> other;
        {
            QSharedPointer<Data> ptr = QSharedPointer<Data>(new Data, dataDeleter);
            other = ptr;
            QCOMPARE(dataDeleter.callCount, 0);
        }
        QCOMPARE(dataDeleter.callCount, 0);
    }
    QCOMPARE(dataDeleter.callCount, 1);
    QCOMPARE(refcount, 1);
    check();

    dataDeleter.callCount = 0;
    {
        QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData, dataDeleter);
        ptr.data();
        QCOMPARE(dataDeleter.callCount, 0);
    }
    QCOMPARE(dataDeleter.callCount, 1);
    QCOMPARE(refcount, 1);
    check();

    CustomDeleter<DerivedData> derivedDataDeleter;
    derivedDataDeleter.callCount = 0;
    dataDeleter.callCount = 0;
    {
        QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData, derivedDataDeleter);
        ptr.data();
        QCOMPARE(dataDeleter.callCount, 0);
        QCOMPARE(derivedDataDeleter.callCount, 0);
    }
    QCOMPARE(dataDeleter.callCount, 0);
    QCOMPARE(derivedDataDeleter.callCount, 1);
    QCOMPARE(refcount, 2);
    check();

    derivedDataDeleter.callCount = 0;
    dataDeleter.callCount = 0;
    {
        QSharedPointer<Data> other;
        {
            QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData, dataDeleter);
            other = ptr;
            QCOMPARE(dataDeleter.callCount, 0);
            QCOMPARE(derivedDataDeleter.callCount, 0);
        }
        QCOMPARE(dataDeleter.callCount, 0);
        QCOMPARE(derivedDataDeleter.callCount, 0);
    }
    QCOMPARE(dataDeleter.callCount, 1);
    QCOMPARE(derivedDataDeleter.callCount, 0);
    QCOMPARE(refcount, 2);
    check();

    derivedDataDeleter.callCount = 0;
    dataDeleter.callCount = 0;
    {
        QSharedPointer<Data> other;
        {
            QSharedPointer<DerivedData> ptr = QSharedPointer<DerivedData>(new DerivedData, derivedDataDeleter);
            other = ptr;
            QCOMPARE(dataDeleter.callCount, 0);
            QCOMPARE(derivedDataDeleter.callCount, 0);
        }
        QCOMPARE(dataDeleter.callCount, 0);
        QCOMPARE(derivedDataDeleter.callCount, 0);
    }
    QCOMPARE(dataDeleter.callCount, 0);
    QCOMPARE(derivedDataDeleter.callCount, 1);
    QCOMPARE(refcount, 2);
    check();
}

void customQObjectDeleterFn(QObject *obj)
{
    ++customDeleterFnCallCount;
    delete obj;
}

void tst_QSharedPointer::creating()
{
    Data::generationCounter = Data::destructorCounter = 0;
    {
        QSharedPointer<Data> ptr = QSharedPointer<Data>::create();
        QVERIFY(ptr.data());
        QCOMPARE(Data::generationCounter, 1);
        QCOMPARE(ptr->generation, 1);
        QCOMPARE(Data::destructorCounter, 0);

        QCOMPARE(ptr->classLevel(), 1);

        ptr.clear();
        QCOMPARE(Data::destructorCounter, 1);
    }
    check();

    Data::generationCounter = Data::destructorCounter = 0;
    {
        QSharedPointer<Data> ptr = QSharedPointer<Data>::create();
        QWeakPointer<Data> weakptr = ptr;
        QtSharedPointer::ExternalRefCountData *d = refCountData(ptr);

        ptr.clear();
        QVERIFY(ptr.isNull());
        QCOMPARE(Data::destructorCounter, 1);

        // valgrind will complain here if something happened to the pointer
        QVERIFY(d->weakref == 1);
        QVERIFY(d->strongref == 0);
    }
    check();

    Data::generationCounter = Data::destructorCounter = 0;
    DerivedData::derivedDestructorCounter = 0;
    {
        QSharedPointer<Data> ptr = QSharedPointer<DerivedData>::create();
        QCOMPARE(ptr->classLevel(), 2);
        QCOMPARE(ptr.staticCast<DerivedData>()->moreData, 0);
        ptr.clear();

        QCOMPARE(Data::destructorCounter, 1);
        QCOMPARE(DerivedData::derivedDestructorCounter, 1);
    }
    check();

    {
        QSharedPointer<Data> ptr = QSharedPointer<DiffPtrDerivedData>::create();
        QCOMPARE(ptr->classLevel(), 3);
        QCOMPARE(ptr.staticCast<DiffPtrDerivedData>()->buffer[7]+0, 16-7);
        QCOMPARE(ptr.staticCast<DiffPtrDerivedData>()->buffer[3]+0, 16-3);
        QCOMPARE(ptr.staticCast<DiffPtrDerivedData>()->buffer[0]+0, 16);
    }
    check();

    {
        QSharedPointer<VirtualDerived> ptr = QSharedPointer<VirtualDerived>::create();
        QCOMPARE(ptr->classLevel(), 4);
        QCOMPARE(ptr->moreData, 0xc0ffee);

        QSharedPointer<Data> baseptr = ptr;
        QCOMPARE(baseptr->classLevel(), 4);
    }
    check();
}

void tst_QSharedPointer::creatingQObject()
{
    {
        QSharedPointer<QObject> ptr = QSharedPointer<QObject>::create();
        QCOMPARE(ptr->metaObject(), &QObject::staticMetaObject);

        QPointer<QObject> qptr = ptr.data();
        ptr.clear();

        QVERIFY(qptr.isNull());
    }
    check();

    {
        QSharedPointer<QObject> ptr = QSharedPointer<OtherObject>::create();
        QCOMPARE(ptr->metaObject(), &OtherObject::staticMetaObject);
    }
    check();
}

void tst_QSharedPointer::mixTrackingPointerCode()
{
    {
        // pointer created with tracking
        // deleted in code without tracking
        QSharedPointer<int> ptr = QSharedPointer<int>(new int(42));
        Wrapper w(ptr);
        ptr.clear();
    }
    check();

    {
        // pointer created without tracking
        // deleted in code with tracking
        Wrapper w = Wrapper::create();
        w.ptr.clear();
    }
}

class ThreadData
{
    QAtomicInt * volatile ptr;
public:
    ThreadData(QAtomicInt *p) : ptr(p) { }
    ~ThreadData() { ++ptr; }
    void ref()
    {
        // if we're called after the destructor, we'll crash
        ptr->ref();
    }
};

class StrongThread: public QThread
{
protected:
    void run()
    {
        usleep(rand() % 2000);
        ptr->ref();
        ptr.clear();
    }
public:
    QSharedPointer<ThreadData> ptr;
};

class WeakThread: public QThread
{
protected:
    void run()
    {
        usleep(rand() % 2000);
        QSharedPointer<ThreadData> ptr = weak;
        if (ptr)
            ptr->ref();
        ptr.clear();
    }
public:
    QWeakPointer<ThreadData> weak;
};

void tst_QSharedPointer::threadStressTest_data()
{
    QTest::addColumn<int>("strongThreadCount");
    QTest::addColumn<int>("weakThreadCount");

    QTest::newRow("0+0") << 0 << 0;
    QTest::newRow("1+0") << 1 << 0;
    QTest::newRow("2+0") << 2 << 0;
    QTest::newRow("10+0") << 10 << 0;

    QTest::newRow("0+1") << 0 << 1;
    QTest::newRow("1+1") << 1 << 1;

    QTest::newRow("2+10") << 2 << 10;
#ifndef Q_OS_WINCE
    // Windows CE cannot run this many threads
    QTest::newRow("5+10") << 5 << 10;
    QTest::newRow("5+30") << 5 << 30;

    QTest::newRow("100+100") << 100 << 100;
#endif
}

void tst_QSharedPointer::threadStressTest()
{
    QFETCH(int, strongThreadCount);
    QFETCH(int, weakThreadCount);

    int guard1[128];
    QAtomicInt counter;
    int guard2[128];

    memset(guard1, 0, sizeof guard1);
    memset(guard2, 0, sizeof guard2);

    for (int r = 0; r < 5; ++r) {
        QVector<QThread*> allThreads(6 * qMax(strongThreadCount, weakThreadCount) + 3, 0);
        QSharedPointer<ThreadData> base = QSharedPointer<ThreadData>(new ThreadData(&counter));
        counter = 0;

        // set the pointers
        for (int i = 0; i < strongThreadCount; ++i) {
            StrongThread *t = new StrongThread;
            t->ptr = base;
            allThreads[2 * i] = t;
        }
        for (int i = 0; i < weakThreadCount; ++i) {
            WeakThread *t = new WeakThread;
            t->weak = base;
            allThreads[6 * i + 3] = t;
        }

        base.clear();

#ifdef Q_OS_WINCE
        srand(QDateTime::currentDateTime().toTime_t());
#else
        srand(time(NULL));
#endif
        // start threads
        for (int i = 0; i < allThreads.count(); ++i)
            if (allThreads[i]) allThreads[i]->start();

        // wait for them to finish
        for (int i = 0; i < allThreads.count(); ++i)
            if (allThreads[i]) allThreads[i]->wait();
        qDeleteAll(allThreads);

        // ensure the guards aren't touched
        for (uint i = 0; i < sizeof guard1 / sizeof guard1[0]; ++i)
            QVERIFY(!guard1[i]);
        for (uint i = 0; i < sizeof guard2 / sizeof guard2[0]; ++i)
            QVERIFY(!guard2[i]);

        // verify that the count is the right range
        int minValue = strongThreadCount;
        int maxValue = strongThreadCount + weakThreadCount;
        QVERIFY(counter >= minValue);
        QVERIFY(counter <= maxValue);
    }
}

template<typename Container, bool Ordered>
void hashAndMapTest()
{
    typedef typename Container::key_type Key;
    typedef typename Container::mapped_type Value;

    Container c;
    QVERIFY(c.isEmpty());

    Key k0;
    c.insert(k0, Value(0));
    QVERIFY(!c.isEmpty());

    typename Container::iterator it;
    it = c.find(k0);
    QVERIFY(it != c.end());
    it = c.find(Key());
    QVERIFY(it != c.end());
    it = c.find(Key(0));
    QVERIFY(it != c.end());

    Key k1(new typename Key::value_type(42));
    it = c.find(k1);
    QVERIFY(it == c.end());

    c.insert(k1, Value(42));
    it = c.find(k1);
    QVERIFY(it != c.end());
    QVERIFY(it != c.find(Key()));

    if (Ordered) {
        QVERIFY(k0 < k1);

        it = c.begin();
        QCOMPARE(it.key(), k0);
        QCOMPARE(it.value(), Value(0));

        ++it;
        QCOMPARE(it.key(), k1);
        QCOMPARE(it.value(), Value(42));

        ++it;
        QVERIFY(it == c.end());
    }

    c.insertMulti(k1, Value(47));
    it = c.find(k1);
    QVERIFY(it != c.end());
    QCOMPARE(it.key(), k1);
    ++it;
    QVERIFY(it != c.end());
    QCOMPARE(it.key(), k1);
    ++it;
    QVERIFY(it == c.end());
}

void tst_QSharedPointer::map()
{
    hashAndMapTest<QMap<QSharedPointer<int>, int>, true>();
}

void tst_QSharedPointer::hash()
{
    hashAndMapTest<QHash<QSharedPointer<int>, int>, false>();
}

void tst_QSharedPointer::validConstructs()
{
    {
        Data *aData = new Data;
        QSharedPointer<Data> ptr1 = QSharedPointer<Data>(aData);

        ptr1 = ptr1;            // valid

        QSharedPointer<Data> ptr2(ptr1);

        ptr1 = ptr2;
        ptr2 = ptr1;

        ptr1 = QSharedPointer<Data>();
        ptr1 = ptr2;
    }
}

typedef bool (QTest::QExternalTest:: * TestFunction)(const QByteArray &body);
Q_DECLARE_METATYPE(TestFunction)
void tst_QSharedPointer::invalidConstructs_data()
{
    QTest::addColumn<TestFunction>("testFunction");
    QTest::addColumn<QString>("code");
    QTest::newRow("sanity-checking") << &QTest::QExternalTest::tryCompile << "";

    // QSharedPointer<void> is not allowed
    QTest::newRow("void") << &QTest::QExternalTest::tryCompileFail << "QSharedPointer<void> ptr;";

    // implicit initialization
    QTest::newRow("implicit-initialization1")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<Data> ptr = new Data;";
    QTest::newRow("implicit-initialization2")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<Data> ptr;"
           "ptr = new Data;";
    QTest::newRow("implicit-initialization3")
        << &QTest::QExternalTest::tryCompileFail
        << "QWeakPointer<Data> ptr = new Data;";
    QTest::newRow("implicit-initialization4")
        << &QTest::QExternalTest::tryCompileFail
        << "QWeakPointer<Data> ptr;"
           "ptr = new Data;";

    // use of forward-declared class
    QTest::newRow("forward-declaration")
        << &QTest::QExternalTest::tryRun
        << "forwardDeclaredDestructorRunCount = 0;\n"
           "{ QSharedPointer<ForwardDeclared> ptr = QSharedPointer<ForwardDeclared>(forwardPointer()); }\n"
           "exit(forwardDeclaredDestructorRunCount);";
    QTest::newRow("creating-forward-declaration")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<ForwardDeclared>::create();";

    // upcast without cast operator:
    QTest::newRow("upcast1")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<Data> baseptr = QSharedPointer<Data>(new DerivedData);\n"
           "QSharedPointer<DerivedData> ptr(baseptr);";
    QTest::newRow("upcast2")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<Data> baseptr = QSharedPointer<Data>(new DerivedData);\n"
           "QSharedPointer<DerivedData> ptr;\n"
           "ptr = baseptr;";

    // dropping of const
    QTest::newRow("const-dropping1")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<const Data> baseptr = QSharedPointer<const Data>(new Data);\n"
           "QSharedPointer<Data> ptr(baseptr);";
    QTest::newRow("const-dropping2")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<const Data> baseptr = QSharedPointer<const Data>(new Data);\n"
           "QSharedPointer<Data> ptr;"
           "ptr = baseptr;";
    QTest::newRow("const-dropping-static-cast")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<const Data> baseptr = QSharedPointer<const Data>(new Data);\n"
        "qSharedPointerCast<DerivedData>(baseptr);";
#ifndef QTEST_NO_RTTI
    QTest::newRow("const-dropping-dynamic-cast")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<const Data> baseptr = QSharedPointer<const Data>(new Data);\n"
        "qSharedPointerDynamicCast<DerivedData>(baseptr);";
#endif
    QTest::newRow("const-dropping-object-cast1")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<const QObject> baseptr = QSharedPointer<const QObject>(new QObject);\n"
        "qSharedPointerObjectCast<QCoreApplication>(baseptr);";
    QTest::newRow("const-dropping-object-cast2")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<const QObject> baseptr = QSharedPointer<const QObject>(new QObject);\n"
        "qobject_cast<QCoreApplication *>(baseptr);";

    // arithmethics through automatic cast operators
    QTest::newRow("arithmethic1")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<int> a;"
           "QSharedPointer<Data> b;\n"
           "if (a == b) return;";
    QTest::newRow("arithmethic2")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<int> a;"
           "QSharedPointer<Data> b;\n"
           "if (a + b) return;";

    // two objects with the same pointer
    QTest::newRow("same-pointer")
        << &QTest::QExternalTest::tryRunFail
        << "Data *aData = new Data;\n"
           "QSharedPointer<Data> ptr1 = QSharedPointer<Data>(aData);\n"
           "QSharedPointer<Data> ptr2 = QSharedPointer<Data>(aData);\n";

    // re-creation:
    QTest::newRow("re-creation")
        << &QTest::QExternalTest::tryRunFail
        << "Data *aData = new Data;\n"
           "QSharedPointer<Data> ptr1 = QSharedPointer<Data>(aData);"
           "ptr1 = QSharedPointer<Data>(aData);";

    // any type of cast for unrelated types:
    // (we have no reinterpret_cast)
    QTest::newRow("invalid-cast1")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<Data> ptr1;\n"
           "QSharedPointer<int> ptr2 = qSharedPointerCast<int>(ptr1);";
#ifndef QTEST_NO_RTTI
    QTest::newRow("invalid-cast2")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<Data> ptr1;\n"
           "QSharedPointer<int> ptr2 = qSharedPointerDynamicCast<int>(ptr1);";
#endif
    QTest::newRow("invalid-cast3")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<Data> ptr1;\n"
           "QSharedPointer<int> ptr2 = qSharedPointerConstCast<int>(ptr1);";
    QTest::newRow("invalid-cast4")
        << &QTest::QExternalTest::tryCompileFail
        << "QSharedPointer<Data> ptr1;\n"
           "QSharedPointer<int> ptr2 = qSharedPointerObjectCast<int>(ptr1);";

    QTest::newRow("weak-pointer-from-regular-pointer")
        << &QTest::QExternalTest::tryCompileFail
        << "Data *ptr = 0;\n"
           "QWeakPointer<Data> weakptr(ptr);\n";

    QTest::newRow("shared-pointer-from-unmanaged-qobject")
        << &QTest::QExternalTest::tryRunFail
        << "QObject *ptr = new QObject;\n"
           "QWeakPointer<QObject> weak = ptr;\n"    // this makes the object unmanaged
           "QSharedPointer<QObject> shared(ptr);\n";

    QTest::newRow("shared-pointer-implicit-from-uninitialized")
        << &QTest::QExternalTest::tryCompileFail
        << "Data *ptr = 0;\n"
           "QSharedPointer<Data> weakptr = Qt::Uninitialized;\n";
}

void tst_QSharedPointer::invalidConstructs()
{
#ifdef Q_CC_MINGW
    QSKIP("The maintainer of QSharedPointer: 'We don't know what the problem is so skip the tests.'", SkipAll);
#endif
#ifdef QTEST_CROSS_COMPILED
    QSKIP("This test does not work on cross compiled systems", SkipAll);
#endif

    QTest::QExternalTest test;
    test.setQtModules(QTest::QExternalTest::QtCore);
    test.setExtraProgramSources(QStringList() << SRCDIR "forwarddeclared.cpp");
    test.setProgramHeader(
        "#define QT_SHAREDPOINTER_TRACK_POINTERS\n"
        "#define QT_DEBUG\n"
        "#include <QtCore/qsharedpointer.h>\n"
        "#include <QtCore/qcoreapplication.h>\n"
        "\n"
        "struct Data { int i; };\n"
        "struct DerivedData: public Data { int j; };\n"
        "\n"
        "extern int forwardDeclaredDestructorRunCount;\n"
        "class ForwardDeclared;\n"
        "ForwardDeclared *forwardPointer();\n"
        );

    QFETCH(QString, code);
    static bool sane = true;
    if (code.isEmpty()) {
        static const char snippet[] = "QSharedPointer<Data> baseptr; QSharedPointer<DerivedData> ptr;";
        if (!test.tryCompile("")
            || !test.tryRun("")
            || !test.tryRunFail("exit(1);")
            || !test.tryCompile(snippet)
            || !test.tryLink(snippet)
            || !test.tryRun(snippet)) {
            sane = false;
            qWarning("Sanity checking failed\nCode:\n%s\n",
                     qPrintable(test.errorReport()));
        }
    }
    if (!sane)
        QFAIL("External testing failed sanity checking, cannot proceed");

    QFETCH(TestFunction, testFunction);

    QByteArray body = code.toLatin1();

    bool result = (test.*testFunction)(body);
    if (qgetenv("QTEST_EXTERNAL_DEBUG").toInt() > 0) {
        qDebug("External test output:");
#ifdef Q_CC_MSVC
        // MSVC prints errors to stdout
        printf("%s\n", test.standardOutput().constData());
#endif
        printf("%s\n", test.standardError().constData());
    }
    if (!result) {
        qWarning("External code testing failed\nCode:\n%s\n", body.constData());
        QFAIL("Fail");
    }
}

namespace QTBUG11730 {
    struct IB
    {
        virtual ~IB() {}
    };

    struct IA
    {
        virtual QSharedPointer<IB> getB() = 0;
    };

    struct B: public IB
    {
        IA *m_a;
        B(IA *a_a) :m_a(a_a)
        { }
        ~B()
        {
            QSharedPointer<IB> b = m_a->getB();
        }
    };

    struct A: public IA
    {
        QSharedPointer<IB> b;

        virtual QSharedPointer<IB> getB()
        {
            return b;
        }

        A()
        {
            b = QSharedPointer<IB>(new B(this));
        }

        ~A()
        {
            b.clear();
        }
    };
}

void tst_QSharedPointer::reentrancyWhileDestructing()
{
    // this bug is about recursing back into QSharedPointer::clear()
    // from inside it
    // that is, the destructor of the object being deleted recurses
    // into the same QSharedPointer object.
    // First reported as QTBUG-11730
    QTBUG11730::A obj;
}


QTEST_MAIN(tst_QSharedPointer)

#include "tst_qsharedpointer.moc"
