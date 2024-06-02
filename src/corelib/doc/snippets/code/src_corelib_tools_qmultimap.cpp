/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Copyright (C) 2016 The Qt Company Ltd.
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

//! [0]
QMultiMap<QString, int> multimap;
//! [0]


//! [2]
multimap.insert("a", 1);
multimap.insert("b", 3);
multimap.insert("c", 7);
multimap.insert("c", -5);
//! [2]


//! [3]
int num2 = multimap.value("a"); // 1
int num3 = multimap.value("thirteen"); // not found; 0
int num3 = 0;
auto it = multimap.value("b");
if (it != multimap.end()) {
    num3 = it.value();
}
//! [3]


//! [4]
int timeout = 30;
if (multimap.contains("TIMEOUT"))
    timeout = multimap.value("TIMEOUT");

// better:
auto it = multimap.find("TIMEOUT");
if (it != multimap.end())
    timeout = it.value();
//! [4]


//! [5]
int timeout = multimap.value("TIMEOUT", 30);
//! [5]


//! [7]
QMultiMapIterator<QString, int> i(multimap);
while (i.hasNext()) {
    i.next();
    cout << i.key() << ": " << i.value() << Qt::endl;
}
//! [7]


//! [8]
auto i = multimap.constBegin();
while (i != multimap.constEnd()) {
    cout << i.key() << ": " << i.value() << Qt::endl;
    ++i;
}
//! [8]


//! [9]
multimap.insert("plenty", 100);
multimap.insert("plenty", 2000);
// multimap.size() == 2
//! [9]


//! [10]
QList<int> values = multimap.values("plenty");
for (int i = 0; i < values.size(); ++i)
    cout << values.at(i) << Qt::endl;
//! [10]


//! [11]
QMultiMap<QString, int>::iterator i = multimap.find("plenty");
while (i != map.end() && i.key() == "plenty") {
    cout << i.value() << Qt::endl;
    ++i;
}

// better:
auto [i, end] = multimap.equal_range("plenty");
while (i != end) {
    cout << i.value() << Qt::endl;
    ++i;
}
//! [11]


//! [12]
QMap<QString, int> multimap;
...
foreach (int value, multimap)
    cout << value << Qt::endl;
//! [12]


//! [13]
#ifndef EMPLOYEE_H
#define EMPLOYEE_H

class Employee
{
public:
    Employee() {}
    Employee(const QString &name, QDate dateOfBirth);
    ...

private:
    QString myName;
    QDate myDateOfBirth;
};

inline bool operator<(const Employee &e1, const Employee &e2)
{
    if (e1.name() != e2.name())
        return e1.name() < e2.name();
    return e1.dateOfBirth() < e2.dateOfBirth();
}

#endif // EMPLOYEE_H
//! [13]


//! [15]
QMultiMap<int, QString> multimap;
multimap.insert(1, "one");
multimap.insert(5, "five");
multimap.insert(5, "five (2)");
multimap.insert(10, "ten");

multimap.lowerBound(0);      // returns iterator to (1, "one")
multimap.lowerBound(1);      // returns iterator to (1, "one")
multimap.lowerBound(2);      // returns iterator to (5, "five")
multimap.lowerBound(5);      // returns iterator to (5, "five")
multimap.lowerBound(6);      // returns iterator to (10, "ten")
multimap.lowerBound(10);     // returns iterator to (10, "ten")
multimap.lowerBound(999);    // returns end()
//! [15]


//! [16]
QMap<QString, int> multimap;
...
QMap<QString, int>::const_iterator i = multimap.lowerBound("HDR");
QMap<QString, int>::const_iterator upperBound = multimap.upperBound("HDR");
while (i != upperBound) {
    cout << i.value() << Qt::endl;
    ++i;
}
//! [16]


//! [17]
QMultiMap<int, QString> multimap;
multimap.insert(1, "one");
multimap.insert(5, "five");
multimap.insert(5, "five (2)");
multimap.insert(10, "ten");

multimap.upperBound(0);      // returns iterator to (1, "one")
multimap.upperBound(1);      // returns iterator to (5, "five")
multimap.upperBound(2);      // returns iterator to (5, "five")
multimap.lowerBound(5);      // returns iterator to (5, "five (2)")
multimap.lowerBound(6);      // returns iterator to (10, "ten")
multimap.upperBound(10);     // returns end()
multimap.upperBound(999);    // returns end()
//! [17]

//! [19]
for (auto it = multimap.begin(), end = multimap.end(); i != end; ++i)
    i.value() += 2;
//! [19]

//! [19]
QMultiMap<QString, int>::iterator i;
for (i = multimap.begin(); i != multimap.end(); ++i)
    i.value() += 2;
//! [19]


//! [20]
QMultiMap<QString, int>::iterator i = multimap.begin();
while (i != multimap.end()) {
    if (i.key().startsWith('_'))
        i = multimap.erase(i);
    else
        ++i;
}
//! [20]


//! [21]
QMultiMap<QString, int>::iterator i = multimap.begin();
while (i != multimap.end()) {
    QMap<QString, int>::iterator prev = i;
    ++i;
    if (prev.key().startsWith('_'))
        multimap.erase(prev);
}
//! [21]


//! [22]
// WRONG
while (i != multimap.end()) {
    if (i.key().startsWith('_'))
        multimap.erase(i);
    ++i;
}
//! [22]


//! [23]
if (i.key() == "Hello")
    i.value() = "Bonjour";
//! [23]


//! [24]
QMultiMap<QString, int> multi;
multimap.insert("January", 1);
multimap.insert("February", 2);
...
multimap.insert("December", 12);

QMultiMap<QString, int>::const_iterator i;
for (i = multimap.constBegin(); i != multimap.constEnd(); ++i)
    cout << i.key() << ": " << i.value() << Qt::endl;
//! [24]


//! [25]
QMultiMap<QString, int> map1, map2, map3;

map1.insert("plenty", 100);
map1.insert("plenty", 2000);
// map1.size() == 2

map2.insert("plenty", 5000);
// map2.size() == 1

map3 = map1 + map2;
// map3.size() == 3
//! [25]

//! [keyiterator1]
for (QMultiMap<int, QString>::const_iterator it = multimap.cbegin(), end = multimap.cend(); it != end; ++it) {
    cout << "The key: " << it.key() << Qt::endl
    cout << "The value: " << it.value() << Qt::endl;
    cout << "Also the value: " << (*it) << Qt::endl;
}
//! [keyiterator1]

//! [keyiterator2]
// Inefficient, keys() is expensive
QList<int> keys = multimap.keys();
int numPrimes = std::count_if(multimap.cbegin(), multimap.cend(), isPrimeNumber);
qDeleteAll(multimap2.keys());

// Efficient, no memory allocation needed
int numPrimes = std::count_if(multimap.keyBegin(), multimap.keyEnd(), isPrimeNumber);
qDeleteAll(multimap2.keyBegin(), multimap2.keyEnd());
//! [keyiterator2]
