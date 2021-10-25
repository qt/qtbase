/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QObject>
#include <QDebug>

//! [0]
class MyClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged BINDABLE bindableX)
public:
    int x() const { return xProp; }
    void setX(int x) { xProp = x; }
    QBindable<int> bindableX() { return QBindable<int>(&xProp); }

signals:
    void xChanged();

private:
    // Declare the instance of the bindable property data.
    Q_OBJECT_BINDABLE_PROPERTY(MyClass, int, xProp, &MyClass::xChanged)
};
//! [0]

//! [1]
class MyClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged BINDABLE bindableX)
public:
    int x() const { return xProp; }
    void setX(int x) { xProp = x; }
    QBindable<int> bindableX() { return QBindable<int>(&xProp); }

signals:
    void xChanged();

private:
    // Declare the instance of int bindable property data and
    // initialize it with the value 5.
    // This is similar to declaring
    // int xProp = 5;
    // without using the new QObjectBindableProperty class.
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MyClass, int, xProp, 5, &MyClass::xChanged)
};
//! [1]

//! [2]
class CustomType
{
public:
    CustomType(int val, int otherVal) : value(val), anotherValue(otherVal) { }

private:
    int value = 0;
    int anotherValue = 0;
};

// later when using CustomType as a property
Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MyClass, CustomType xProp, CustomType(5, 10),
                                     &MyClass::xChanged)
//! [2]

void usage_QBindable() {
    //! [3]
    MyClass *myObject;
    QBindable<int> bindableX = myObject->bindableX();
    qDebug() << bindableX.hasBinding(); // prints false
    QProperty<int> y {42};
    bindableX.setBinding([&](){ return 2*y.value(); });
    qDebug() << bindableX.hasBinding() << myObject->x(); // prints true 84
    //! [3]
}

//! [4]
#include <QObject>
#include <QProperty>
#include <QDebug>

class Foo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int myVal READ myVal WRITE setMyVal BINDABLE bindableMyVal)
public:
    int myVal() { return myValMember.value(); }
    void setMyVal(int newvalue) { myValMember = newvalue; }
    QBindable<int> bindableMyVal() { return &myValMember; }
signals:
    void myValChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(Foo, int, myValMember, &Foo::myValChanged);
};

int main()
{
    bool debugout(true); // enable debug log
    Foo myfoo;
    QProperty<int> prop(42);
    QObject::connect(&myfoo, &Foo::myValChanged, [&]() {
        if (debugout)
            qDebug() << myfoo.myVal();
    });
    myfoo.bindableMyVal().setBinding([&]() { return prop.value(); }); // prints "42"

    prop = 5; // prints "5"
    debugout = false;
    prop = 6; // prints nothing
    debugout = true;
    prop = 7; // prints "7"
}

#include "main.moc"
//! [4]

//! [5]
class Client{};

class MyClassPrivate : public QObjectPrivate
{
public:
    QList<Client> clients;
    bool hasClientsActualCalculation() const { return clients.size() > 0; }
    Q_OBJECT_COMPUTED_PROPERTY(MyClassPrivate, bool, hasClientsData,
                               &MyClassPrivate::hasClientsActualCalculation)
};

class MyClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasClients READ hasClients STORED false BINDABLE bindableHasClients)
public:
    QBindable<bool> bindableHasClients()
    {
        return QBindable<bool>(&d_func()->hasClientsData);
    }
    bool hasClients() const
    {
        return d_func()->hasClientsData.value();
    }
    void addClient(const Client &c)
    {
        Q_D(MyClass);
        d->clients.push_back(c);
        // notify that the value could have changed
        d->hasClientsData.markDirty();
    }
private:
    Q_DECLARE_PRIVATE(MyClass)
};
//! [5]
