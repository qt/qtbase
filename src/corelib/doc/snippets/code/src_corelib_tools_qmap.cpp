// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QMap<QString, int> map;
//! [0]


//! [1]
map["one"] = 1;
map["three"] = 3;
map["seven"] = 7;
//! [1]


//! [2]
map.insert("twelve", 12);
//! [2]


//! [3]
int num1 = map["thirteen"];
int num2 = map.value("thirteen");
//! [3]


//! [4]
int timeout = 30;
if (map.contains("TIMEOUT"))
    timeout = map.value("TIMEOUT");
//! [4]


//! [5]
int timeout = map.value("TIMEOUT", 30);
//! [5]


//! [6]
// WRONG
QMap<int, QWidget *> map;
...
for (int i = 0; i < 1000; ++i) {
    if (map[i] == okButton)
        cout << "Found button at index " << i << endl;
}
//! [6]


//! [7]
QMapIterator<QString, int> i(map);
while (i.hasNext()) {
    i.next();
    cout << qPrintable(i.key()) << ": " << i.value() << endl;
}
//! [7]


//! [8]
for (auto i = map.cbegin(), end = map.cend(); i != end; ++i)
    cout << qPrintable(i.key()) << ": " << i.value() << endl;
//! [8]


//! [9]
map.insert("plenty", 100);
map.insert("plenty", 2000);
// map.value("plenty") == 2000
//! [9]


//! [12]
QMap<QString, int> map;
...
for (int value : std::as_const(map))
    cout << value << endl;
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


//! [17]
QMap<int, QString> map;
map.insert(1, "one");
map.insert(5, "five");
map.insert(10, "ten");

map.upperBound(0);      // returns iterator to (1, "one")
map.upperBound(1);      // returns iterator to (5, "five")
map.upperBound(2);      // returns iterator to (5, "five")
map.upperBound(10);     // returns end()
map.upperBound(999);    // returns end()
//! [17]


//! [18]
QMap<QString, int> map;
map.insert("January", 1);
map.insert("February", 2);
...
map.insert("December", 12);

for (auto i = map.cbegin(), end = map.cend(); i != end; ++i)
    cout << qPrintable(i.key()) << ": " << i.value() << endl;
//! [18]


//! [19]
for (auto i = map.begin(), end = map.end(); i != end; ++i)
    i.value() += 2;
//! [19]


void erase()
{
QMap<QString, int> map;
//! [20]
QMap<QString, int>::const_iterator i = map.cbegin();
while (i != map.cend()) {
    if (i.value() > 10)
        i = map.erase(i);
    else
        ++i;
}
//! [20]
//! [21]
erase_if(map, [](const QMap<QString, int>::iterator it) { return it.value() > 10; });
//! [21]
}

//! [23]
if (i.key() == "Hello")
    i.value() = "Bonjour";
//! [23]


//! [24]
QMap<QString, int> map;
map.insert("January", 1);
map.insert("February", 2);
...
map.insert("December", 12);

for (auto i = map.cbegin(), end = map.cend(); i != end; ++i)
    cout << qPrintable(i.key()) << ": " << i.value() << endl;
//! [24]


//! [keyiterator1]
for (QMap<int, QString>::const_iterator it = map.cbegin(), end = map.cend(); it != end; ++it) {
    cout << "The key: " << it.key() << endl;
    cout << "The value: " << qPrintable(it.value()) << endl;
    cout << "Also the value: " << qPrintable(*it) << endl;
}
//! [keyiterator1]

//! [keyiterator2]
// Inefficient, keys() is expensive
QList<int> keys = map.keys();
int numPrimes = std::count_if(map.cbegin(), map.cend(), isPrimeNumber);
qDeleteAll(map2.keys());

// Efficient, no memory allocation needed
int numPrimes = std::count_if(map.keyBegin(), map.keyEnd(), isPrimeNumber);
qDeleteAll(map2.keyBegin(), map2.keyEnd());
//! [keyiterator2]

//! [28]
QMap<QString, int> map;
map.insert("January", 1);
map.insert("February", 2);
// ...
map.insert("December", 12);

for (auto [key, value] : map.asKeyValueRange()) {
    cout << qPrintable(key) << ": " << value << endl;
    --value; // convert to JS month indexing
}
//! [28]
