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
#include <QtCore>
#include <QtGui>
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
};

void QObjectBenchmark::signal_slot_benchmark_data()
{
    QTest::addColumn<int>("type");
    QTest::newRow("simple function") << 0;
    QTest::newRow("single signal/slot") << 1;
    QTest::newRow("multi signal/slot") << 2;
    QTest::newRow("unconnected signal") << 3;
}

void QObjectBenchmark::signal_slot_benchmark()
{
    QFETCH(int, type);

    Object singleObject;
    Object multiObject;
    singleObject.setObjectName("single");
    multiObject.setObjectName("multi");

    singleObject.connect(&singleObject, SIGNAL(signal0()), SLOT(slot0()));

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
    QTest::addColumn<QByteArray>("signal");
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod method = mo->method(i);
        if (method.methodType() != QMetaMethod::Signal)
            continue;
        QByteArray sig = method.signature();
        QTest::newRow(sig) << sig;
    }
}

void QObjectBenchmark::connect_disconnect_benchmark()
{
    QFETCH(QByteArray, signal);
    signal.prepend('2');
    const char *p = signal.constData();
    QTreeView obj;
    QBENCHMARK {
        QObject::connect(&obj, p, &obj, p);
        QObject::disconnect(&obj, p, &obj, p);
    }
}

QTEST_MAIN(QObjectBenchmark)

#include "main.moc"
