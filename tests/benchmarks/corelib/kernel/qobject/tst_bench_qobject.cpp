// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtCore>
#include <QtWidgets/QTreeView>
#include <qtest.h>
#include "object.h"
#include <qcoreapplication.h>
#include <qdatetime.h>

enum {
    CreationDeletionBenckmarkConstant = 34567,
    SignalsAndSlotsBenchmarkConstant = 456789
};

class tst_QObject : public QObject
{
Q_OBJECT
private slots:
    void signal_slot_benchmark();
    void signal_slot_benchmark_data();
    void signal_many_receivers();
    void signal_many_receivers_data();
    void qproperty_benchmark_data();
    void qproperty_benchmark();
    void dynamic_property_benchmark();
    void connect_disconnect_benchmark_data();
    void connect_disconnect_benchmark();
    void receiver_destroyed_benchmark();

    void stdAllocator();
};

class QObjectUsingStandardAllocator : public QObject
{
    Q_OBJECT
public:
    QObjectUsingStandardAllocator()
    {
    }
};

template<class T>
inline void allocator()
{
    // We need to allocate certain amount of objects otherwise the new implementation
    // may re-use the previous allocation, hiding the somehow high cost of allocation. It
    // also helps us to reduce the noise ratio, which is high for memory allocation.
    //
    // The check depends on memory allocation performance, which is quite non-deterministic.
    // When a new memory is requested, the new operator, depending on implementation, is trying
    // to re-use existing, already allocated for the process memory. If there is not enough, it
    // asks OS to give more. Of course the first case is faster then the second. In the same
    // time, from an application perspective the first is also more likely.
    //
    // As a result, depending on which use-case one wants to test, it may be recommended to run this
    // test in separation from others, to "force" expensive code path in the memory allocation.
    //
    // The time based results are heavily affected by background noise. One really needs to
    // prepare OS (no other tasks, CPU and RAM reservations) to run this test, or use
    // instruction counting which seems to be less fragile.

    const int count = 256 * 1024;

    QScopedPointer<T> objects[count];
    QBENCHMARK_ONCE {
        for (int i = 0; i < count; ++i)
            objects[i].reset(new T);
        for (int i = 0; i < count; ++i)
            objects[i].reset();
    }
}

void tst_QObject::stdAllocator()
{
    allocator<QObjectUsingStandardAllocator>();
}

struct Functor {
    void operator()(){}
};

void tst_QObject::signal_slot_benchmark_data()
{
    QTest::addColumn<int>("type");
    QTest::newRow("simple function") << 0;
    QTest::newRow("single signal/slot") << 1;
    QTest::newRow("multi signal/slot") << 2;
    QTest::newRow("unconnected signal") << 3;
    QTest::newRow("single signal/ptr") << 4;
    QTest::newRow("functor") << 5;
}

void tst_QObject::signal_slot_benchmark()
{
    QFETCH(int, type);

    Object singleObject;
    Object multiObject;
    Functor functor;
    singleObject.setObjectName("single");
    multiObject.setObjectName("multi");

    if (type == 5) {
        QObject::connect(&singleObject, &Object::signal0, functor);
    } else if (type == 4) {
        QObject::connect(&singleObject, &Object::signal0, &singleObject, &Object::slot0);
    } else {
        singleObject.connect(&singleObject, SIGNAL(signal0()), SLOT(slot0()));
    }

    multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(slot0()));
    // multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(signal1()));
    multiObject.connect(&multiObject, SIGNAL(signal1()), SLOT(slot1()));
    // multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(signal2()));
    multiObject.connect(&multiObject, SIGNAL(signal2()), SLOT(slot2()));
    // multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(signal3()));
    multiObject.connect(&multiObject, SIGNAL(signal3()), SLOT(slot3()));
    // multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(signal4()));
    multiObject.connect(&multiObject, SIGNAL(signal4()), SLOT(slot4()));
    // multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(signal5()));
    multiObject.connect(&multiObject, SIGNAL(signal5()), SLOT(slot5()));
    // multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(signal6()));
    multiObject.connect(&multiObject, SIGNAL(signal6()), SLOT(slot6()));
    // multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(signal7()));
    multiObject.connect(&multiObject, SIGNAL(signal7()), SLOT(slot7()));
    // multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(signal8()));
    multiObject.connect(&multiObject, SIGNAL(signal8()), SLOT(slot8()));
    // multiObject.connect(&multiObject, SIGNAL(signal0()), SLOT(signal9()));
    multiObject.connect(&multiObject, SIGNAL(signal9()), SLOT(slot9()));

    if (type == 0) {
        QBENCHMARK {
            singleObject.slot0();
        }
    } else if (type == 1) {
        QBENCHMARK {
            singleObject.emitSignal0();
        }
    } else if (type == 2) {
        QBENCHMARK {
            multiObject.emitSignal0();
        }
    } else if (type == 3) {
        QBENCHMARK {
            singleObject.emitSignal1();
        }
    } else if (type == 4 || type == 5) {
        QBENCHMARK {
            singleObject.emitSignal0();
        }
    }
}

void tst_QObject::signal_many_receivers_data()
{
    QTest::addColumn<int>("receiverCount");
    QTest::newRow("100 receivers") << 100;
    QTest::newRow("1 000 receivers") << 1000;
    QTest::newRow("10 000 receivers") << 10000;
}

void tst_QObject::signal_many_receivers()
{
    QFETCH(int, receiverCount);
    Object sender;
    std::vector<Object> receivers(receiverCount);

    for (Object &receiver : receivers)
        QObject::connect(&sender, &Object::signal0, &receiver, &Object::slot0);

    QBENCHMARK {
        sender.emitSignal0();
    }
}

void tst_QObject::qproperty_benchmark_data()
{
    QTest::addColumn<QByteArray>("name");
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        if (prop.isWritable())
            QTest::newRow(prop.name()) << QByteArray(prop.name());
    }
}

void tst_QObject::qproperty_benchmark()
{
    QFETCH(QByteArray, name);
    const char *p = name.constData();
    QTreeView obj;
    QVariant v = obj.property(p);
    QBENCHMARK {
        obj.setProperty(p, v);
        (void)obj.property(p);
    }
}

void tst_QObject::dynamic_property_benchmark()
{
    QTreeView obj;
    QBENCHMARK {
        obj.setProperty("myProperty", 123);
        (void)obj.property("myProperty");
        obj.setProperty("myOtherProperty", 123);
        (void)obj.property("myOtherProperty");
    }
}

void tst_QObject::connect_disconnect_benchmark_data()
{
    QTest::addColumn<int>("type");
    QTest::newRow("normalized signature") << 0;
    QTest::newRow("unormalized signature") << 1;
    QTest::newRow("function pointer") << 2;
    QTest::newRow("normalized signature/handle") << 3;
    QTest::newRow("unormalized signature/handle") << 4;
    QTest::newRow("function pointer/handle") << 5;
    QTest::newRow("functor/handle") << 6;}

void tst_QObject::connect_disconnect_benchmark()
{
    QFETCH(int, type);
    switch (type) {
        case 0: {
            QTreeView obj;
            QBENCHMARK {
                QObject::connect   (&obj, SIGNAL(viewportEntered()), &obj, SLOT(expandAll()));
                QObject::disconnect(&obj, SIGNAL(viewportEntered()), &obj, SLOT(expandAll()));
            }
        } break;
        case 1: {
            QTreeView obj;
            QBENCHMARK {
                QObject::connect   (&obj, SIGNAL(viewportEntered(  )), &obj, SLOT(expandAll(  ))); // sic: non-normalised
                QObject::disconnect(&obj, SIGNAL(viewportEntered(  )), &obj, SLOT(expandAll(  ))); // sic: non-normalised
            }
        } break;
        case 2: {
            QTreeView obj;
            QBENCHMARK {
                QObject::connect   (&obj, &QAbstractItemView::viewportEntered, &obj, &QTreeView::expandAll);
                QObject::disconnect(&obj, &QAbstractItemView::viewportEntered, &obj, &QTreeView::expandAll);
            }
        } break;
        case 3: {
            QTreeView obj;
            QBENCHMARK {
                QObject::disconnect(QObject::connect(&obj, SIGNAL(viewportEntered()), &obj, SLOT(expandAll())));
            }
        } break;
        case 4: {
            QTreeView obj;
            QBENCHMARK {
                QObject::disconnect(QObject::connect(&obj, SIGNAL(viewportEntered(  )), &obj, SLOT(expandAll(  )))); // sic: non-normalised
            }
        } break;
        case 5: {
            QTreeView obj;
            QBENCHMARK {
                QObject::disconnect(QObject::connect(&obj, &QAbstractItemView::viewportEntered, &obj, &QTreeView::expandAll));
            }
        } break;
        case 6: {
            QTreeView obj;
            Functor functor;
            QBENCHMARK {
                QObject::disconnect(QObject::connect(&obj, &QAbstractItemView::viewportEntered, functor));
            }
        } break;
    }
}

void tst_QObject::receiver_destroyed_benchmark()
{
    Object sender;
    QBENCHMARK {
        Object receiver;
        QObject::connect(&sender, &Object::signal0, &receiver, &Object::slot0);
    }
}

QTEST_MAIN(tst_QObject)

#include "tst_bench_qobject.moc"
