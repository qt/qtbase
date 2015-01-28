/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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

class QObjectBenchmark : public QObject
{
Q_OBJECT
private slots:
    void signal_slot_benchmark();
    void signal_slot_benchmark_data();
    void qproperty_benchmark_data();
    void qproperty_benchmark();
    void dynamic_property_benchmark();
    void connect_disconnect_benchmark_data();
    void connect_disconnect_benchmark();
    void receiver_destroyed_benchmark();
};

struct Functor {
    void operator()(){}
};

void QObjectBenchmark::signal_slot_benchmark_data()
{
    QTest::addColumn<int>("type");
    QTest::newRow("simple function") << 0;
    QTest::newRow("single signal/slot") << 1;
    QTest::newRow("multi signal/slot") << 2;
    QTest::newRow("unconnected signal") << 3;
    QTest::newRow("single signal/ptr") << 4;
    QTest::newRow("functor") << 5;
}

void QObjectBenchmark::signal_slot_benchmark()
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

void QObjectBenchmark::qproperty_benchmark_data()
{
    QTest::addColumn<QByteArray>("name");
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        QTest::newRow(prop.name()) << QByteArray(prop.name());
    }
}

void QObjectBenchmark::qproperty_benchmark()
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

void QObjectBenchmark::dynamic_property_benchmark()
{
    QTreeView obj;
    QBENCHMARK {
        obj.setProperty("myProperty", 123);
        (void)obj.property("myProperty");
        obj.setProperty("myOtherProperty", 123);
        (void)obj.property("myOtherProperty");
    }
}

void QObjectBenchmark::connect_disconnect_benchmark_data()
{
    QTest::addColumn<int>("type");
    QTest::newRow("normalized signature") << 0;
    QTest::newRow("unormalized signature") << 1;
    QTest::newRow("function pointer") << 2;
    QTest::newRow("normalized signature/handle") << 3;
    QTest::newRow("unormalized signature/handle") << 4;
    QTest::newRow("function pointer/handle") << 5;
    QTest::newRow("functor/handle") << 6;}

void QObjectBenchmark::connect_disconnect_benchmark()
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

void QObjectBenchmark::receiver_destroyed_benchmark()
{
    Object sender;
    QBENCHMARK {
        Object receiver;
        QObject::connect(&sender, &Object::signal0, &receiver, &Object::slot0);
    }
}

QTEST_MAIN(QObjectBenchmark)

#include "main.moc"
