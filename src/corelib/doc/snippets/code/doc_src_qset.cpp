// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QSet<QString> set;
//! [0]


//! [1]
set.insert("one");
set.insert("three");
set.insert("seven");
//! [1]


//! [2]
set << "twelve" << "fifteen" << "nineteen";
//! [2]


//! [3]
if (!set.contains("ninety-nine"))
    ...
//! [3]


//! [4]
QSetIterator<QWidget *> i(set);
while (i.hasNext()) {
    QWidget *w = i.next();
    qDebug() << w;
}
//! [4]


//! [5]
for (auto i = set.cbegin(), end = set.cend(); i != end; ++i)
    qDebug() << *i;
//! [5]


//! [6]
QSet<QString> set;
...
for (const auto &value : set)
    qDebug() << value;
//! [6]


//! [7]
QSet<QString> set;
set.reserve(20000);
for (int i = 0; i < 20000; ++i)
    set.insert(values[i]);
//! [7]


//! [8]
QSet<QString> set = {"January", "February", ... "December"}

// i is a QSet<QString>::iterator
for (auto i = set.begin(), end = set.end(); i != end; ++i)
    qDebug() << *i;
//! [8]


//! [9]
QSet<QString> set = {"January", "February", ... "December"};

auto i = set.begin();
while (i != set.end()) {
    if ((*i).startsWith('J')) {
        i = set.erase(i);
    } else {
        ++i;
    }
}
//! [9]


//! [10]
QSet<QString> set;
...
const auto predicate = [](const QString &s) { return s.compare("Jeanette", Qt::CaseInsensitive) == 0; };
QSet<QString>::iterator it = std::find_if(set.begin(), set.end(), predicate);
if (it != set.end())
    cout << "Found Jeanette" << endl;
//! [10]


//! [11]
QSet<QString> set = {"January", "February", ... "December"};

// i is QSet<QString>::const_iterator
for (auto i = set.cbegin(), end = set.cend(); i != end; ++i)
    qDebug() << *i;
//! [11]


//! [12]
QSet<QString> set;
...
const auto predicate = [](const QString &s) { return s.compare("Jeanette", Qt::CaseInsensitive) == 0; };
QSet<QString>::const_iterator it = std::find_if(set.cbegin(), set.cend(), predicate);
if (it != set.constEnd())
    cout << "Found Jeanette" << endl;
//! [12]
