/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

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
int id = QMetaType::type("MyClass");
if (id == 0) {
    void *myClassPtr = QMetaType::construct(id);
    ...
    QMetaType::destroy(id, myClassPtr);
    myClassPtr = 0;
}
//! [3]


//! [4]
qRegisterMetaType<MyClass>("MyClass");
//! [4]


//! [5]
qRegisterMetaTypeStreamOperators<MyClass>("MyClass");
//! [5]


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
