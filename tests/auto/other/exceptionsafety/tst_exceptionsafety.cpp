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

#include "qplatformdefs.h"
#include <stdlib.h>

void* internalMalloc(size_t bytes);
#define malloc internalMalloc
#include <QVarLengthArray>
#undef malloc

#include <QtTest/QtTest>

QT_USE_NAMESPACE

class tst_ExceptionSafety: public QObject
{
    Q_OBJECT
private slots:
#ifdef QT_NO_EXCEPTIONS
    void initTestCase();
#else
    void exceptionInSlot();
    void exceptionVector();
    void exceptionHash();
    void exceptionMap();
    void exceptionList();
    void exceptionLinkedList();
//    void exceptionEventLoop();
//    void exceptionSignalSlot();
    void exceptionOOMQVarLengthArray();
#endif
};

#ifdef QT_NO_EXCEPTIONS
void tst_ExceptionSafety::initTestCase()
{
    QSKIP("This test requires exception support");
}

#else

class Emitter : public QObject
{
    Q_OBJECT
public:
    inline void emitTestSignal() { emit testSignal(); }
signals:
    void testSignal();
};

class ExceptionThrower : public QObject
{
    Q_OBJECT
public slots:
    void thrower() { throw 5; }
};

class Receiver : public QObject
{
    Q_OBJECT
public:
    Receiver()
        : received(0) {}
    int received;

public slots:
    void receiver() { ++received; }
};

enum ThrowType { ThrowNot = 0, ThrowAtCreate = 1, ThrowAtCopy = 2, ThrowLater = 3, ThrowAtComparison = 4 };

ThrowType throwType = ThrowNot; // global flag to indicate when an exception should be throw. Will be reset when the exception has been generated.

int objCounter = 0;

/*! Class that does not throw any exceptions. Used as baseclass for all the other ones.
 */
template <int T>
class FlexibleThrower
{
    public:
        FlexibleThrower() : _value(-1) {
            if( throwType == ThrowAtCreate ) {
                throwType = ThrowNot;
                throw ThrowAtCreate;
            }
            objCounter++;
        }

        FlexibleThrower( short value ) : _value(value) {
            if( throwType == ThrowAtCreate ) {
                throwType = ThrowNot;
                throw ThrowAtCreate;
            }
            objCounter++;
        }

        FlexibleThrower(FlexibleThrower const& other ) {
            // qDebug("cc");

            if( throwType == ThrowAtCopy ) {
                throwType = ThrowNot;
                throw ThrowAtCopy;

            } else if( throwType == ThrowLater ) {
                throwType = ThrowAtCopy;
            }

            objCounter++;
            _value = other.value();
        }

        ~FlexibleThrower() { objCounter--; }

        bool operator==(const FlexibleThrower<T> &t) const
        {
            // qDebug("vv == %d %d", value(), t.value());
            if( throwType == ThrowAtComparison ) {
                throwType = ThrowNot;
                throw ThrowAtComparison;
            }
            return value()==t.value();
        }

        bool operator<(const FlexibleThrower<T> &t) const
        {
            // qDebug("vv < %d %d", value(), t.value());
            if( throwType == ThrowAtComparison ) {
                throwType = ThrowNot;
                throw ThrowAtComparison;
            }
            return value()<t.value();
        }

        int value() const
        { return (int)_value; }

        short _value;
        char dummy[T];
};

uint qHash(const FlexibleThrower<2>& t)
{
    // qDebug("ha");
    if( throwType == ThrowAtComparison ) {
        throwType = ThrowNot;
        throw ThrowAtComparison;
    }
    return (uint)t.value();
}

typedef FlexibleThrower<2> FlexibleThrowerSmall;
typedef QMap<FlexibleThrowerSmall,FlexibleThrowerSmall> MyMap;
typedef QHash<FlexibleThrowerSmall,FlexibleThrowerSmall> MyHash;

// connect a signal to a slot that throws an exception
// run this through valgrind to make sure it doesn't corrupt
void tst_ExceptionSafety::exceptionInSlot()
{
    Emitter emitter;
    ExceptionThrower thrower;

    connect(&emitter, SIGNAL(testSignal()), &thrower, SLOT(thrower()));

    try {
        emitter.emitTestSignal();
    } catch (int i) {
        QCOMPARE(i, 5);
    }
}

void tst_ExceptionSafety::exceptionList()
{
    const int intancesCount = objCounter;
    {
        int instances;
        QList<FlexibleThrowerSmall> list;
        QList<FlexibleThrowerSmall> list2;
        QList<FlexibleThrowerSmall> list3;

        for( int i = 0; i<10; i++ )
            list.append( FlexibleThrowerSmall(i) );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            list.append( FlexibleThrowerSmall(10));
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            list.prepend( FlexibleThrowerSmall(10));
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.at(0).value(), 0 );
        QCOMPARE( list.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            list.insert( 8, FlexibleThrowerSmall(10));
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.at(7).value(), 7 );
        QCOMPARE( list.at(8).value(), 8 );
        QCOMPARE( list.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            FlexibleThrowerSmall t = list.takeAt( 6 );
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.at(6).value(), 6 );
        QCOMPARE( list.at(7).value(), 7 );
        QCOMPARE( list.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            list3 = list;
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.at(0).value(), 0 );
        QCOMPARE( list.at(7).value(), 7 );
        QCOMPARE( list.size(), 10 );
        QCOMPARE( list3.at(0).value(), 0 );
        QCOMPARE( list3.at(7).value(), 7 );
        QCOMPARE( list3.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            list3.append( FlexibleThrowerSmall(11) );
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.at(0).value(), 0 );
        QCOMPARE( list.at(7).value(), 7 );
        QCOMPARE( list.size(), 10 );
        QCOMPARE( list3.at(0).value(), 0 );
        QCOMPARE( list3.at(7).value(), 7 );
        QCOMPARE( list3.size(), 10 );

        try {
            list2.clear();
            list2.append( FlexibleThrowerSmall(11));
            throwType = ThrowAtCopy;
            instances = objCounter;
            list3 = list+list2;
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.at(0).value(), 0 );
        QCOMPARE( list.at(7).value(), 7 );
        QCOMPARE( list.size(), 10 );

        // check that copy on write works atomar
        list2.clear();
        list2.append( FlexibleThrowerSmall(11));
        list3 = list+list2;
        instances = objCounter;
        try {
            throwType = ThrowAtCreate;
            list3[7]=FlexibleThrowerSmall(12);
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.at(7).value(), 7 );
        QCOMPARE( list.size(), 10 );
        QCOMPARE( list3.at(7).value(), 7 );
        QCOMPARE( list3.size(), 11 );

    }
    QCOMPARE(objCounter, intancesCount); // check that every object has been freed
}

void tst_ExceptionSafety::exceptionLinkedList()
{
    const int intancesCount = objCounter;
    {
        int instances;
        QLinkedList<FlexibleThrowerSmall> list;
        QLinkedList<FlexibleThrowerSmall> list2;
        QLinkedList<FlexibleThrowerSmall> list3;

        for( int i = 0; i<10; i++ )
            list.append( FlexibleThrowerSmall(i) );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            list.append( FlexibleThrowerSmall(10));
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            list.prepend( FlexibleThrowerSmall(10));
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.first().value(), 0 );
        QCOMPARE( list.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            list3 = list;
            list3.append( FlexibleThrowerSmall(11) );
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( list.first().value(), 0 );
        QCOMPARE( list.size(), 10 );
        QCOMPARE( list3.size(), 10 );
    }
    QCOMPARE(objCounter, intancesCount); // check that every object has been freed
}

void tst_ExceptionSafety::exceptionVector()
{
    const int intancesCount = objCounter;
    {
        int instances;
        QVector<FlexibleThrowerSmall> vector;
        QVector<FlexibleThrowerSmall> vector2;
        QVector<FlexibleThrowerSmall> vector3;

        for (int i = 0; i<10; i++)
            vector.append( FlexibleThrowerSmall(i) );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            vector.append( FlexibleThrowerSmall(10));
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            vector.prepend( FlexibleThrowerSmall(10));
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.at(0).value(), 0 );
        QCOMPARE( vector.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            vector.insert( 8, FlexibleThrowerSmall(10));
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.at(7).value(), 7 );
        QCOMPARE( vector.at(8).value(), 8 );
        QCOMPARE( vector.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            vector3 = vector;
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.at(0).value(), 0 );
        QCOMPARE( vector.at(7).value(), 7 );
        QCOMPARE( vector.size(), 10 );
        QCOMPARE( vector3.at(0).value(), 0 );
        QCOMPARE( vector3.at(7).value(), 7 );
        QCOMPARE( vector3.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCopy;
            vector3.append( FlexibleThrowerSmall(11) );
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.at(0).value(), 0 );
        QCOMPARE( vector.at(7).value(), 7 );
        QCOMPARE( vector.size(), 10 );
        QCOMPARE( vector3.at(0).value(), 0 );
        QCOMPARE( vector3.at(7).value(), 7 );

        try {
            vector2.clear();
            vector2.append( FlexibleThrowerSmall(11));
            instances = objCounter;
            throwType = ThrowAtCopy;
            vector3 = vector+vector2;
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.at(0).value(), 0 );
        QCOMPARE( vector.at(7).value(), 7 );
        QCOMPARE( vector.size(), 10 );

        // check that copy on write works atomar
        vector2.clear();
        vector2.append( FlexibleThrowerSmall(11));
        vector3 = vector+vector2;
        instances = objCounter;
        try {
            throwType = ThrowAtCreate;
            vector3[7]=FlexibleThrowerSmall(12);
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.at(7).value(), 7 );
        QCOMPARE( vector.size(), 10 );
        QCOMPARE( vector3.at(7).value(), 7 );
        QCOMPARE( vector3.size(), 11 );

        instances = objCounter;
        try {
            throwType = ThrowAtCreate;
            vector.resize(15);
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.at(7).value(), 7 );
        QCOMPARE( vector.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowAtCreate;
            vector.resize(15);
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.at(7).value(), 7 );
        QCOMPARE( vector.size(), 10 );

        instances = objCounter;
        try {
            throwType = ThrowLater;
            vector.fill(FlexibleThrowerSmall(1), 15);
        } catch (...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( vector.at(0).value(), 0 );
        QCOMPARE( vector.size(), 10 );
    }
    QCOMPARE(objCounter, intancesCount); // check that every object has been freed
}


void tst_ExceptionSafety::exceptionMap()
{
    const int intancesCount = objCounter;
    {
        int instances;
        MyMap map;
        MyMap map2;
        MyMap map3;

        throwType = ThrowNot;
        for (int i = 0; i<10; i++)
            map[ FlexibleThrowerSmall(i) ] = FlexibleThrowerSmall(i);

        return; // further test are deactivated until Map is fixed.

        for( int i = ThrowAtCopy; i<=ThrowAtComparison; i++ ) {
            instances = objCounter;
            try {
                throwType = (ThrowType)i;
                map[ FlexibleThrowerSmall(10) ] = FlexibleThrowerSmall(10);
            } catch(...) {
                QCOMPARE(instances, objCounter);
            }
            QCOMPARE( map.size(), 10 );
            QCOMPARE( map[ FlexibleThrowerSmall(1) ], FlexibleThrowerSmall(1) );
        }

        map2 = map;
        instances = objCounter;
        try {
            throwType = ThrowLater;
            map2[ FlexibleThrowerSmall(10) ] = FlexibleThrowerSmall(10);
        } catch(...) {
            QCOMPARE(instances, objCounter);
        }
        /* qDebug("%d %d", map.size(), map2.size() );
        for( int i=0; i<map.size(); i++ )
            qDebug( "Value at %d: %d",i, map.value(FlexibleThrowerSmall(i), FlexibleThrowerSmall()).value() );
        QCOMPARE( map.value(FlexibleThrowerSmall(1), FlexibleThrowerSmall()), FlexibleThrowerSmall(1) );
        qDebug( "Value at %d: %d",1, map[FlexibleThrowerSmall(1)].value() );
        qDebug("%d %d", map.size(), map2.size() );
        */
        QCOMPARE( map[ FlexibleThrowerSmall(1) ], FlexibleThrowerSmall(1) );
        QCOMPARE( map.size(), 10 );
        QCOMPARE( map2[ FlexibleThrowerSmall(1) ], FlexibleThrowerSmall(1) );
        QCOMPARE( map2.size(), 10 );

    }
    QCOMPARE(objCounter, intancesCount); // check that every object has been freed
}

void tst_ExceptionSafety::exceptionHash()
{
    const int intancesCount = objCounter;
    {
        int instances;
        MyHash hash;
        MyHash hash2;
        MyHash hash3;

        for( int i = 0; i<10; i++ )
            hash[ FlexibleThrowerSmall(i) ] = FlexibleThrowerSmall(i);

        for( int i = ThrowAtCopy; i<=ThrowAtComparison; i++ ) {
            instances = objCounter;
            try {
                throwType = (ThrowType)i;
                hash[ FlexibleThrowerSmall(10) ] = FlexibleThrowerSmall(10);
            } catch(...) {
                QCOMPARE(instances, objCounter);
            }
            QCOMPARE( hash.size(), 10 );
        }

        hash2 = hash;
        instances = objCounter;
        try {
            throwType = ThrowLater;
            hash2[ FlexibleThrowerSmall(10) ] = FlexibleThrowerSmall(10);
        } catch(...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( hash[ FlexibleThrowerSmall(1) ], FlexibleThrowerSmall(1) );
        QCOMPARE( hash.size(), 10 );
        QCOMPARE( hash2[ FlexibleThrowerSmall(1) ], FlexibleThrowerSmall(1) );
        QCOMPARE( hash2.size(), 10 );

        hash2.clear();
        instances = objCounter;
        try {
            throwType = ThrowLater;
            hash2.reserve(30);
        } catch(...) {
            QCOMPARE(instances, objCounter);
        }
        QCOMPARE( hash2.size(), 0 );

        /*
           try {
           throwType = ThrowAtCopy;
           hash.prepend( FlexibleThrowerSmall(10));
           } catch (...) {
           }
           QCOMPARE( hash.at(0).value(), 0 );
           QCOMPARE( hash.size(), 10 );

           try {
           throwType = ThrowAtCopy;
           hash.insert( 8, FlexibleThrowerSmall(10));
           } catch (...) {
           }
           QCOMPARE( hash.at(7).value(), 7 );
           QCOMPARE( hash.at(8).value(), 8 );
           QCOMPARE( hash.size(), 10 );

           qDebug("val");
           try {
           throwType = ThrowAtCopy;
           hash3 = hash;
           } catch (...) {
           }
           QCOMPARE( hash.at(0).value(), 0 );
           QCOMPARE( hash.at(7).value(), 7 );
           QCOMPARE( hash.size(), 10 );
           QCOMPARE( hash3.at(0).value(), 0 );
           QCOMPARE( hash3.at(7).value(), 7 );
           QCOMPARE( hash3.size(), 10 );

           try {
           throwType = ThrowAtCopy;
           hash3.append( FlexibleThrowerSmall(11) );
           } catch (...) {
           }
           QCOMPARE( hash.at(0).value(), 0 );
           QCOMPARE( hash.at(7).value(), 7 );
           QCOMPARE( hash.size(), 10 );
           QCOMPARE( hash3.at(0).value(), 0 );
           QCOMPARE( hash3.at(7).value(), 7 );
           QCOMPARE( hash3.at(11).value(), 11 );

           try {
           hash2.clear();
           hash2.append( FlexibleThrowerSmall(11));
           throwType = ThrowAtCopy;
           hash3 = hash+hash2;
           } catch (...) {
           }
           QCOMPARE( hash.at(0).value(), 0 );
           QCOMPARE( hash.at(7).value(), 7 );
           QCOMPARE( hash.size(), 10 );

        // check that copy on write works atomar
        hash2.clear();
        hash2.append( FlexibleThrowerSmall(11));
        hash3 = hash+hash2;
        try {
        throwType = ThrowAtCopy;
        hash3[7]=FlexibleThrowerSmall(12);
        } catch (...) {
        }
        QCOMPARE( hash.at(7).value(), 7 );
        QCOMPARE( hash.size(), 10 );
        QCOMPARE( hash3.at(7).value(), 7 );
        QCOMPARE( hash3.size(), 11 );
        */


    }
    QCOMPARE(objCounter, intancesCount); // check that every object has been freed
}

// Disable these tests until the level of exception safety in event loops is clear
#if 0
enum
{
    ThrowEventId = QEvent::User + 42,
    NoThrowEventId = QEvent::User + 43
};

class ThrowEvent : public QEvent
{
public:
    ThrowEvent()
        : QEvent(static_cast<QEvent::Type>(ThrowEventId))
    {
    }
};

class NoThrowEvent : public QEvent
{
public:
    NoThrowEvent()
        : QEvent(static_cast<QEvent::Type>(NoThrowEventId))
    {}
};

struct IntEx : public std::exception
{
    IntEx(int aEx) : ex(aEx) {}
    int ex;
};

class TestObject : public QObject
{
public:
    TestObject()
        : throwEventCount(0), noThrowEventCount(0) {}

    int throwEventCount;
    int noThrowEventCount;

protected:
    bool event(QEvent *event)
    {
        if (int(event->type()) == ThrowEventId) {
             throw IntEx(++throwEventCount);
        } else if (int(event->type()) == NoThrowEventId) {
            ++noThrowEventCount;
        }
        return QObject::event(event);
    }
};

void tst_ExceptionSafety::exceptionEventLoop()
{
    // send an event that throws
    TestObject obj;
    ThrowEvent throwEvent;
    try {
        qApp->sendEvent(&obj, &throwEvent);
    } catch (IntEx code) {
        QCOMPARE(code.ex, 1);
    }
    QCOMPARE(obj.throwEventCount, 1);

    // post an event that throws
    qApp->postEvent(&obj, new ThrowEvent);

    try {
        qApp->processEvents();
    } catch (IntEx code) {
        QCOMPARE(code.ex, 2);
    }
    QCOMPARE(obj.throwEventCount, 2);

    // post a normal event, then a throwing event, then a normal event
    // run this in valgrind to ensure that it doesn't leak.

    qApp->postEvent(&obj, new NoThrowEvent);
    qApp->postEvent(&obj, new ThrowEvent);
    qApp->postEvent(&obj, new NoThrowEvent);

    try {
        qApp->processEvents();
    } catch (IntEx code) {
        QCOMPARE(code.ex, 3);
    }
    // here, we should have received on non-throwing event and one throwing one
    QCOMPARE(obj.throwEventCount, 3);
    QCOMPARE(obj.noThrowEventCount, 1);

    // spin the event loop again
    qApp->processEvents();

    // now, we should have received the second non-throwing event
    QCOMPARE(obj.noThrowEventCount, 2);
}

void tst_ExceptionSafety::exceptionSignalSlot()
{
    Emitter e;
    ExceptionThrower thrower;
    Receiver r1;
    Receiver r2;

    // connect a signal to a normal object, a thrower and a normal object again
    connect(&e, SIGNAL(testSignal()), &r1, SLOT(receiver()));
    connect(&e, SIGNAL(testSignal()), &thrower, SLOT(thrower()));
    connect(&e, SIGNAL(testSignal()), &r2, SLOT(receiver()));

    int code = 0;
    try {
        e.emitTestSignal();
    } catch (int c) {
        code = c;
    }

    // 5 is the magic number that's thrown by thrower
    QCOMPARE(code, 5);

    // assumption: slots are called in the connection order
    QCOMPARE(r1.received, 1);
    QCOMPARE(r2.received, 0);
}
#endif


static bool outOfMemory = false;
void* internalMalloc(size_t bytes) { return outOfMemory ? 0 : malloc(bytes); }

struct OutOfMemory
{
    OutOfMemory() { outOfMemory = true; }
    ~OutOfMemory() { outOfMemory = false; }
};

void tst_ExceptionSafety::exceptionOOMQVarLengthArray()
{
#ifdef QT_NO_EXCEPTIONS
    // it will crash by design
    Q_STATIC_ASSERT(false);
#else
     QVarLengthArray<char> arr0;
    int minSize = arr0.capacity();

    // constructor throws
    bool success = false;
    try {
        OutOfMemory oom;
        QVarLengthArray<char> arr(minSize * 2);
    } catch (const std::bad_alloc&) {
        success = true;
    }
    QVERIFY(success);

    QVarLengthArray<char> arr;

    // resize throws
    success = false;
    try {
        OutOfMemory oom;
        arr.resize(minSize * 2);
    } catch(const std::bad_alloc&) {
        arr.resize(1);
        success = true;
    }
    QVERIFY(success);
#endif
}

#endif

QTEST_MAIN(tst_ExceptionSafety)
#include "tst_exceptionsafety.moc"
