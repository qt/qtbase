// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
struct MyStruct
{
    int i;
    ...
};

Q_DECLARE_METATYPE(MyStruct)
//! [0]


//! [1]
namespace MyNamespace
{
    ...
}

Q_DECLARE_METATYPE(MyNamespace::MyStruct)
//! [1]


//! [2]
MyStruct s;
QVariant var;
var.setValue(s); // copy s into the variant

...

// retrieve the value
MyStruct s2 = var.value<MyStruct>();
//! [2]


//! [3]
QMetaType type = QMetaType::fromName("MyClass");
if (type.isValid()) {
    void *myClassPtr = type.create();
    ...
    type.destroy(myClassPtr);
    myClassPtr = nullptr;
}
//! [3]


//! [4]
qRegisterMetaType<MyClass>("MyClass");
//! [4]


//! [6]
QDataStream &operator<<(QDataStream &out, const MyClass &myObj);
QDataStream &operator>>(QDataStream &in, MyClass &myObj);
//! [6]


//! [7]
int id = qRegisterMetaType<MyStruct>();
//! [7]


//! [8]
int id = qMetaTypeId<QString>();    // id is now QMetaType::QString
id = qMetaTypeId<MyStruct>();       // compile error if MyStruct not declared
//! [8]

//! [9]
typedef QString CustomString;
qRegisterMetaType<CustomString>("CustomString");
//! [9]

//! [10]

#include <deque>

Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(std::deque)

void someFunc()
{
    std::deque<QFile*> container;
    QVariant var = QVariant::fromValue(container);
    // ...
}

//! [10]

//! [11]

#include <unordered_list>

Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(std::unordered_map)

void someFunc()
{
    std::unordered_map<int, bool> container;
    QVariant var = QVariant::fromValue(container);
    // ...
}

//! [11]

//! [13]

#include <memory>

Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr)

void someFunc()
{
    auto smart_ptr = std::make_shared<QFile>();
    QVariant var = QVariant::fromValue(smart_ptr);
    // ...
    if (var.canConvert<QObject*>()) {
        QObject *sp = var.value<QObject*>();
        qDebug() << sp->metaObject()->className(); // Prints 'QFile'.
    }
}

//! [13]
