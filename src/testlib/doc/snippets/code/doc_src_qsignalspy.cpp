// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QCheckBox *box = ...;
QSignalSpy spy(box, SIGNAL(clicked(bool)));

// do something that triggers the signal
box->animateClick();

QCOMPARE(spy.count(), 1); // make sure the signal was emitted exactly one time
QList<QVariant> arguments = spy.takeFirst(); // take the first signal

QVERIFY(arguments.at(0).toBool() == true); // verify the first argument
//! [0]


//! [1]
QSignalSpy spy(myCustomObject, SIGNAL(mySignal(int,QString,double)));

myCustomObject->doSomething(); // trigger emission of the signal

QList<QVariant> arguments = spy.takeFirst();
QVERIFY(arguments.at(0).typeId() == QMetaType::Int);
QVERIFY(arguments.at(1).typeId() == QMetaType::QString);
QVERIFY(arguments.at(2).typeId() == QMetaType::Double);
//! [1]


//! [2]
qRegisterMetaType<SomeStruct>();
QSignalSpy spy(&model, SIGNAL(whatever(SomeStruct)));
//! [2]


//! [3]
// get the first argument from the first received signal:
SomeStruct result = qvariant_cast<SomeStruct>(spy.at(0).at(0));
//! [3]


//! [4]
QSignalSpy spy(myPushButton, SIGNAL(clicked(bool)));
//! [4]


//! [6]
QSignalSpy spy(myPushButton, &QPushButton::clicked);
//! [6]

//! [7]
QObject object;
auto mo = object.metaObject();
auto signalIndex = mo->indexOfSignal("objectNameChanged(QString)");
auto signal = mo->method(signalIndex);

QSignalSpy spy(&object, signal);
object.setObjectName("A new object name");
QCOMPARE(spy.count(), 1);
//! [7]

//! [8]
void tst_QWindow::writeMinMaxDimensionalProps_data()
    QTest::addColumn<int>("propertyIndex");

    // Collect all relevant properties
    static const auto mo = QWindow::staticMetaObject;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        auto property = mo.property(i);

        // ...that have type int
        if (property.type() == QVariant::Int) {
            static const QRegularExpression re("^minimum|maximum");
            const auto name = property.name();

            // ...and start with "minimum" or "maximum"
            if (re.match(name).hasMatch()) {
                QTest::addRow("%s", name) << i;
            }
        }
    }
}

void tst_QWindow::writeMinMaxDimensionalProps()
{
    QFETCH(int, propertyIndex);

    auto property = QWindow::staticMetaObject.property(propertyIndex);
    QVERIFY(property.isWritable());
    QVERIFY(property.hasNotifySignal());

    QWindow window;
    QSignalSpy spy(&window, property.notifySignal());

    QVERIFY(property.write(&window, 42));
    QCOMPARE(spy.count(), 1);
}
//! [8]
