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
#include <QtCore>
#include <QtWidgets/QTreeView>
#include <qtest.h>

class LotsOfSignals : public QObject
{
    Q_OBJECT
public:
    LotsOfSignals() {}

signals:
    void extraSignal1();
    void extraSignal2();
    void extraSignal3();
    void extraSignal4();
    void extraSignal5();
    void extraSignal6();
    void extraSignal7();
    void extraSignal8();
    void extraSignal9();
    void extraSignal10();
    void extraSignal12();
    void extraSignal13();
    void extraSignal14();
    void extraSignal15();
    void extraSignal16();
    void extraSignal17();
    void extraSignal18();
    void extraSignal19();
    void extraSignal20();
    void extraSignal21();
    void extraSignal22();
    void extraSignal23();
    void extraSignal24();
    void extraSignal25();
    void extraSignal26();
    void extraSignal27();
    void extraSignal28();
    void extraSignal29();
    void extraSignal30();
    void extraSignal31();
    void extraSignal32();
    void extraSignal33();
    void extraSignal34();
    void extraSignal35();
    void extraSignal36();
    void extraSignal37();
    void extraSignal38();
    void extraSignal39();
    void extraSignal40();
    void extraSignal41();
    void extraSignal42();
    void extraSignal43();
    void extraSignal44();
    void extraSignal45();
    void extraSignal46();
    void extraSignal47();
    void extraSignal48();
    void extraSignal49();
    void extraSignal50();
    void extraSignal51();
    void extraSignal52();
    void extraSignal53();
    void extraSignal54();
    void extraSignal55();
    void extraSignal56();
    void extraSignal57();
    void extraSignal58();
    void extraSignal59();
    void extraSignal60();
    void extraSignal61();
    void extraSignal62();
    void extraSignal63();
    void extraSignal64();
    void extraSignal65();
    void extraSignal66();
    void extraSignal67();
    void extraSignal68();
    void extraSignal69();
    void extraSignal70();
};

class tst_qmetaobject: public QObject
{
Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void indexOfProperty_data();
    void indexOfProperty();
    void indexOfMethod_data();
    void indexOfMethod();
    void indexOfSignal_data();
    void indexOfSignal();
    void indexOfSlot_data();
    void indexOfSlot();

    void unconnected_data();
    void unconnected();
};

void tst_qmetaobject::initTestCase()
{
}

void tst_qmetaobject::cleanupTestCase()
{
}

void tst_qmetaobject::indexOfProperty_data()
{
    QTest::addColumn<QByteArray>("name");
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        QTest::newRow(prop.name()) << QByteArray(prop.name());
    }
}

void tst_qmetaobject::indexOfProperty()
{
    QFETCH(QByteArray, name);
    const char *p = name.constData();
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    QBENCHMARK {
        (void)mo->indexOfProperty(p);
    }
}

void tst_qmetaobject::indexOfMethod_data()
{
    QTest::addColumn<QByteArray>("method");
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod method = mo->method(i);
        QByteArray sig = method.methodSignature();
        QTest::newRow(sig) << sig;
    }
}

void tst_qmetaobject::indexOfMethod()
{
    QFETCH(QByteArray, method);
    const char *p = method.constData();
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    QBENCHMARK {
        (void)mo->indexOfMethod(p);
    }
}

void tst_qmetaobject::indexOfSignal_data()
{
    QTest::addColumn<QByteArray>("signal");
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod method = mo->method(i);
        if (method.methodType() != QMetaMethod::Signal)
            continue;
        QByteArray sig = method.methodSignature();
        QTest::newRow(sig) << sig;
    }
}

void tst_qmetaobject::indexOfSignal()
{
    QFETCH(QByteArray, signal);
    const char *p = signal.constData();
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    QBENCHMARK {
        (void)mo->indexOfSignal(p);
    }
}

void tst_qmetaobject::indexOfSlot_data()
{
    QTest::addColumn<QByteArray>("slot");
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod method = mo->method(i);
        if (method.methodType() != QMetaMethod::Slot)
            continue;
        QByteArray sig = method.methodSignature();
        QTest::newRow(sig) << sig;
    }
}

void tst_qmetaobject::indexOfSlot()
{
    QFETCH(QByteArray, slot);
    const char *p = slot.constData();
    const QMetaObject *mo = &QTreeView::staticMetaObject;
    QBENCHMARK {
        (void)mo->indexOfSlot(p);
    }
}

void tst_qmetaobject::unconnected_data()
{
    QTest::addColumn<int>("signal_index");
    QTest::newRow("signal--9") << 9;
    QTest::newRow("signal--32") << 32;
    QTest::newRow("signal--33") << 33;
    QTest::newRow("signal--64") << 64;
    QTest::newRow("signal--65") << 65;
    QTest::newRow("signal--70") << 70;
}

void tst_qmetaobject::unconnected()
{
    LotsOfSignals *obj = new LotsOfSignals;
    QFETCH(int, signal_index);
    QVERIFY(obj->metaObject()->methodCount() == 73);
    void *v;
    QBENCHMARK {
        //+1 because QObject has one slot
        QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, signal_index+1, &v);
    }
    delete obj;
}

QTEST_MAIN(tst_qmetaobject)

#include "main.moc"
