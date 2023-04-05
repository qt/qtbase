// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QList<int> integerList;
QList<QString> stringList;
//! [0]


//! [1]
QList<QString> list(200);
//! [1]


//! [2]
QList<QString> list(200, "Pass");
//! [2]


//! [3]
if (list[0] == "Liz")
    list[0] = "Elizabeth";
//! [3]


//! [4]
for (qsizetype i = 0; i < list.size(); ++i) {
    if (list.at(i) == "Alfonso")
        cout << "Found Alfonso at position " << i << endl;
}
//! [4]


//! [5]
qsizetype i = list.indexOf("Harumi");
if (i != -1)
    cout << "First occurrence of Harumi is at position " << i << endl;
//! [5]


//! [6]
QList<int> list(10);
int *data = list.data();
for (qsizetype i = 0; i < 10; ++i)
    data[i] = 2 * i;
//! [6]


//! [7]
QList<QString> list;
list.append("one");
list.append("two");
QString three = "three";
list.append(three);
// list: ["one", "two", "three"]
// three: "three"
//! [7]


//! [move-append]
QList<QString> list;
list.append("one");
list.append("two");
QString three = "three";
list.append(std::move(three));
// list: ["one", "two", "three"]
// three: ""
//! [move-append]


//! [emplace]
QList<QString> list{"a", "ccc"};
list.emplace(1, 2, 'b');
// list: ["a", "bb", "ccc"]
//! [emplace]


//! [emplace-back]
QList<QString> list{"one", "two"};
list.emplaceBack(3, 'a');
qDebug() << list;
// list: ["one", "two", "aaa"]
//! [emplace-back]


//! [emplace-back-ref]
QList<QString> list;
auto &ref = list.emplaceBack();
ref = "one";
// list: ["one"]
//! [emplace-back-ref]


//! [8]
QList<QString> list;
list.prepend("one");
list.prepend("two");
list.prepend("three");
// list: ["three", "two", "one"]
//! [8]


//! [9]
QList<QString> list = {"alpha", "beta", "delta"};
list.insert(2, "gamma");
// list: ["alpha", "beta", "gamma", "delta"]
//! [9]


//! [10]
QList<double> list = {2.718, 1.442, 0.4342};
list.insert(1, 3, 9.9);
// list: [2.718, 9.9, 9.9, 9.9, 1.442, 0.4342]
//! [10]


//! [11]
QList<QString> list(3);
list.fill("Yes");
// list: ["Yes", "Yes", "Yes"]

list.fill("oh", 5);
// list: ["oh", "oh", "oh", "oh", "oh"]
//! [11]


//! [12]
QList<QString> list{"A", "B", "C", "B", "A"};
list.indexOf("B");            // returns 1
list.indexOf("B", 1);         // returns 1
list.indexOf("B", 2);         // returns 3
list.indexOf("X");            // returns -1
//! [12]


//! [13]
QList<QString> list = {"A", "B", "C", "B", "A"};
list.lastIndexOf("B");        // returns 3
list.lastIndexOf("B", 3);     // returns 3
list.lastIndexOf("B", 2);     // returns 1
list.lastIndexOf("X");        // returns -1
//! [13]
