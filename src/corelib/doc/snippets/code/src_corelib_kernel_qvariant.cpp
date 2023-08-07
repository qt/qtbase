// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

//! [0]
QDataStream out(...);
QVariant v(123);                // The variant now contains an int
int x = v.toInt();              // x = 123
out << v;                       // Writes a type tag and an int to out
v = QVariant(tr("hello"));      // The variant now contains a QString
int y = v.toInt();              // y = 0 since v cannot be converted to an int
QString s = v.toString();       // s = tr("hello")  (see QObject::tr())
out << v;                       // Writes a type tag and a QString to out
...
QDataStream in(...);            // (opening the previously written stream)
in >> v;                        // Reads an Int variant
int z = v.toInt();              // z = 123
qDebug("Type is %s",            // prints "Type is int"
        v.typeName());
v = v.toInt() + 100;            // The variant now holds the value 223
v = QVariant(QStringList());    // The variant now holds a QStringList
//! [0]

//! [1]
QVariant x;                                // x.isNull() == true
QVariant y = QVariant::fromValue(nullptr); // y.isNull() == true
//! [1]


//! [2]
QVariant variant;
...
QColor color = variant.value<QColor>();
//! [2]


//! [3]
QColor color = palette().background().color();
QVariant variant = color;
//! [3]


//! [4]
QVariant v;

v.setValue(5);
int i = v.toInt();         // i is now 5
QString s = v.toString();  // s is now "5"

MyCustomStruct c;
v.setValue(c);

...

MyCustomStruct c2 = v.value<MyCustomStruct>();
//! [4]


//! [5]
QVariant v;

MyCustomStruct c;
if (v.canConvert<MyCustomStruct>())
    c = v.value<MyCustomStruct>();

v = 7;
int i = v.value<int>();                        // same as v.toInt()
QString s = v.value<QString>();                // same as v.toString(), s is now "7"
MyCustomStruct c2 = v.value<MyCustomStruct>(); // conversion failed, c2 is empty
//! [5]


//! [6]
QVariant v = 42;

v.canConvert<int>();              // returns true
v.canConvert<QString>();          // returns true

MyCustomStruct s;
v.setValue(s);

v.canConvert<int>();              // returns false
v.canConvert<MyCustomStruct>();   // returns true
//! [6]


//! [7]
MyCustomStruct s;
return QVariant::fromValue(s);
//! [7]


//! [9]

QList<int> intList = {7, 11, 42};

QVariant variant = QVariant::fromValue(intList);
if (variant.canConvert<QVariantList>()) {
    QSequentialIterable iterable = variant.value<QSequentialIterable>();
    // Can use foreach:
    foreach (const QVariant &v, iterable) {
        qDebug() << v;
    }
    // Can use C++11 range-for:
    for (const QVariant &v : iterable) {
        qDebug() << v;
    }
    // Can use iterators:
    QSequentialIterable::const_iterator it = iterable.begin();
    const QSequentialIterable::const_iterator end = iterable.end();
    for ( ; it != end; ++it) {
        qDebug() << *it;
    }
}

//! [9]

//! [10]

QHash<int, QString> mapping;
mapping.insert(7, "Seven");
mapping.insert(11, "Eleven");
mapping.insert(42, "Forty-two");

QVariant variant = QVariant::fromValue(mapping);
if (variant.canConvert<QVariantHash>()) {
    QAssociativeIterable iterable = variant.value<QAssociativeIterable>();
    // Can use foreach over the values:
    foreach (const QVariant &v, iterable) {
        qDebug() << v;
    }
    // Can use C++11 range-for over the values:
    for (const QVariant &v : iterable) {
        qDebug() << v;
    }
    // Can use iterators:
    QAssociativeIterable::const_iterator it = iterable.begin();
    const QAssociativeIterable::const_iterator end = iterable.end();
    for ( ; it != end; ++it) {
        qDebug() << *it; // The current value
        qDebug() << it.key();
        qDebug() << it.value();
    }
}

//! [10]
