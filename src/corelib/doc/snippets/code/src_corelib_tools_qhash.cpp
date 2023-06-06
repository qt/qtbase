// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QHash<QString, int> hash;
//! [0]


//! [1]
hash["one"] = 1;
hash["three"] = 3;
hash["seven"] = 7;
//! [1]


//! [2]
hash.insert("twelve", 12);
//! [2]


//! [3]
int num1 = hash["thirteen"];
int num2 = hash.value("thirteen");
//! [3]


//! [4]
int timeout = 30;
if (hash.contains("TIMEOUT"))
    timeout = hash.value("TIMEOUT");
//! [4]


//! [5]
int timeout = hash.value("TIMEOUT", 30);
//! [5]


//! [6]
// WRONG
QHash<int, QWidget *> hash;
...
for (int i = 0; i < 1000; ++i) {
    if (hash[i] == okButton)
        cout << "Found button at index " << i << endl;
}
//! [6]


//! [7]
QHashIterator<QString, int> i(hash);
while (i.hasNext()) {
    i.next();
    cout << qPrintable(i.key()) << ": " << i.value() << endl;
}
//! [7]


//! [8]
for (auto i = hash.cbegin(), end = hash.cend(); i != end; ++i)
    cout << qPrintable(i.key()) << ": " << i.value() << endl;
//! [8]


//! [9]
hash.insert("plenty", 100);
hash.insert("plenty", 2000);
// hash.value("plenty") == 2000
//! [9]


//! [12]
QHash<QString, int> hash;
...
for (int value : std::as_const(hash))
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

inline bool operator==(const Employee &e1, const Employee &e2)
{
    return e1.name() == e2.name()
           && e1.dateOfBirth() == e2.dateOfBirth();
}

inline size_t qHash(const Employee &key, size_t seed)
{
    return qHashMulti(seed, key.name(), key.dateOfBirth());
}

#endif // EMPLOYEE_H
//! [13]


//! [14]
QHash<QString, int> hash;
hash.reserve(20000);
for (int i = 0; i < 20000; ++i)
    hash.insert(keys[i], values[i]);
//! [14]


//! [15]
QHash<QObject *, int> objectHash;
...
QHash<QObject *, int>::iterator i = objectHash.find(obj);
while (i != objectHash.end() && i.key() == obj) {
    if (i.value() == 0) {
        i = objectHash.erase(i);
    } else {
        ++i;
    }
}
//! [15]


//! [16]
QHash<QString, int> hash;
...
QHash<QString, int>::const_iterator i = hash.find("HDR");
while (i != hash.end() && i.key() == "HDR") {
    cout << i.value() << endl;
    ++i;
}
//! [16]


//! [17]
QHash<QString, int> hash;
hash.insert("January", 1);
hash.insert("February", 2);
...
hash.insert("December", 12);

for (auto i = hash.cbegin(), end = hash.cend(); i != end; ++i)
    cout << qPrintable(key()) << ": " << i.value() << endl;
//! [17]


//! [18]
for (auto i = hash.begin(), end = hash.end(); i != end; ++i)
    i.value() += 2;
//! [18]

//! [21]
erase_if(hash, [](const QHash<QString, int>::iterator it) { return it.value() > 10; });
//! [21]
}

//! [22]
if (i.key() == "Hello")
    i.value() = "Bonjour";
//! [22]


//! [23]
QHash<QString, int> hash;
hash.insert("January", 1);
hash.insert("February", 2);
...
hash.insert("December", 12);

for (auto i = hash.cbegin(), end = hash.cend(); i != end; ++i)
    cout << qPrintable(i.key()) << ": " << i.value() << endl;
//! [23]


//! [24]
QMultiHash<QString, int> hash1, hash2, hash3;

hash1.insert("plenty", 100);
hash1.insert("plenty", 2000);
// hash1.size() == 2

hash2.insert("plenty", 5000);
// hash2.size() == 1

hash3 = hash1 + hash2;
// hash3.size() == 3
//! [24]


//! [25]
QList<int> values = hash.values("plenty");
for (auto i : std::as_const(values))
    cout << i << endl;
//! [25]


//! [26]
auto i = hash.constFind("plenty");
while (i != hash.cend() && i.key() == "plenty") {
    cout << i.value() << endl;
    ++i;
}
//! [26]

//! [27]
for (auto it = hash.cbegin(), end = hash.cend(); it != end; ++it) {
    cout << "The key: " << it.key() << endl;
    cout << "The value: " << qPrintable(it.value()) << endl;
    cout << "Also the value: " << qPrintable(*it) << endl;
}
//! [27]

//! [28]
// Inefficient, keys() is expensive
QList<int> keys = hash.keys();
int numPrimes = std::count_if(keys.cbegin(), keys.cend(), isPrimeNumber);
qDeleteAll(hash2.keys());

// Efficient, no memory allocation needed
int numPrimes = std::count_if(hash.keyBegin(), hash.keyEnd(), isPrimeNumber);
qDeleteAll(hash2.keyBegin(), hash2.keyEnd());
//! [28]

//! [qhashbits]
inline size_t qHash(const std::vector<int> &key, size_t seed = 0)
{
    if (key.empty())
        return seed;
    else
        return qHashBits(&key.front(), key.size() * sizeof(int), seed);
}
//! [qhashbits]

//! [qhashrange]
inline size_t qHash(const std::vector<int> &key, size_t seed = 0)
{
    return qHashRange(key.begin(), key.end(), seed);
}
//! [qhashrange]

//! [qhashrangecommutative]
inline size_t qHash(const std::unordered_set<int> &key, size_t seed = 0)
{
    return qHashRangeCommutative(key.begin(), key.end(), seed);
}
//! [qhashrangecommutative]

//! [30]
{0, 1, 2}
//! [30]

//! [31]
{1, 2, 0}
//! [31]

//! [32]
size_t qHash(K key, size_t seed);
size_t qHash(const K &key, size_t seed);

size_t qHash(K key);        // deprecated, do not use
size_t qHash(const K &key); // deprecated, do not use
//! [32]

//! [33]
namespace std {
template <> struct hash<K>
{
    // seed is optional
    size_t operator()(const K &key, size_t seed = 0) const;
};
}
//! [33]

//! [34]
QHash<QString, int> hash;
hash.insert("January", 1);
hash.insert("February", 2);
// ...
hash.insert("December", 12);

for (auto [key, value] : hash.asKeyValueRange()) {
    cout << qPrintable(key) << ": " << value << endl;
    --value; // convert to JS month indexing
}
//! [34]

//! [35]
QMultiHash<QString, int> hash;
hash.insert("January", 1);
hash.insert("February", 2);
// ...
hash.insert("December", 12);

for (auto [key, value] : hash.asKeyValueRange()) {
    cout << qPrintable(key) << ": " << value << endl;
    --value; // convert to JS month indexing
}
//! [35]
