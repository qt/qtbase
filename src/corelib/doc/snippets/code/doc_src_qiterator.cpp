// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QList<float> list;
...
QListIterator<float> i(list);
while (i.hasNext())
    float f = i.next();
//! [0]


//! [1]
QListIterator<float> i(list);
i.toBack();
while (i.hasPrevious())
    float f = i.previous();
//! [1]

//! [6]
QSet<QString> set;
...
QSetIterator<QString> i(set);
while (i.hasNext())
    float f = i.next();
//! [6]

//! [8]
QList<float> list;
...
QMutableListIterator<float> i(list);
while (i.hasNext())
    float f = i.next();
//! [8]


//! [9]
QMutableListIterator<float> i(list);
i.toBack();
while (i.hasPrevious())
    float f = i.previous();
//! [9]


//! [10]
QMutableListIterator<int> i(list);
while (i.hasNext()) {
    int val = i.next();
    if (val < 0) {
        i.setValue(-val);
    } else if (val == 0) {
        i.remove();
    }
}
//! [10]

//! [17]
QSet<float> set;
...
QMutableSetIterator<float> i(set);
while (i.hasNext())
    float f = i.next();
//! [17]

//! [19]
QMutableListIterator<int> i(list);
while (i.hasNext()) {
    int val = i.next();
    if (val < -32768 || val > 32767)
        i.remove();
}
//! [19]

//! [22]
QMutableSetIterator<int> i(set);
while (i.hasNext()) {
    int val = i.next();
    if (val < -32768 || val > 32767)
        i.remove();
}
//! [22]


//! [23]
QMutableListIterator<double> i(list);
while (i.hasNext()) {
    double val = i.next();
    i.setValue(std::sqrt(val));
}
//! [23]

//! [26]
QMap<int, QWidget *> map;
...
QMapIterator<int, QWidget *> i(map);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [26]


//! [27]
QMapIterator<int, QWidget *> i(map);
i.toBack();
while (i.hasPrevious()) {
    i.previous();
    qDebug() << i.key() << ": " << i.value();
}
//! [27]


//! [28]
QMapIterator<int, QWidget *> i(map);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [28]

//! [26multi]
QMultiMap<int, QWidget *> multimap;
...
QMultiMapIterator<int, QWidget *> i(multimap);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [26multi]


//! [27multi]
QMultiMapIterator<int, QWidget *> i(multimap);
i.toBack();
while (i.hasPrevious()) {
    i.previous();
    qDebug() << i.key() << ": " << i.value();
}
//! [27multi]


//! [28multi]
QMultiMapIterator<int, QWidget *> i(multimap);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [28multi]


//! [29]
QHash<int, QWidget *> hash;
...
QHashIterator<int, QWidget *> i(hash);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [29]


//! [31]
QHashIterator<int, QWidget *> i(hash);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [31]


//! [32]
QMap<int, QWidget *> map;
...
QMutableMapIterator<int, QWidget *> i(map);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [32]


//! [33]
QMutableMapIterator<int, QWidget *> i(map);
i.toBack();
while (i.hasPrevious()) {
    i.previous();
    qDebug() << i.key() << ": " << i.value();
}
//! [33]


//! [34]
QMutableMapIterator<int, QWidget *> i(map);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [34]


//! [35]
QMutableMapIterator<QString, QString> i(map);
while (i.hasNext()) {
    i.next();
    if (i.key() == i.value())
        i.remove();
}
//! [35]


//! [32multi]
QMultiMap<int, QWidget *> multimap;
...
QMutableMultiMapIterator<int, QWidget *> i(multimap);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [32multi]


//! [33multi]
QMutableMultiMapIterator<int, QWidget *> i(multimap);
i.toBack();
while (i.hasPrevious()) {
    i.previous();
    qDebug() << i.key() << ": " << i.value();
}
//! [33multi]


//! [34multi]
QMutableMultiMapIterator<int, QWidget *> i(multimap);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [34multi]


//! [35multi]
QMutableMultiMapIterator<QString, QString> i(multimap);
while (i.hasNext()) {
    i.next();
    if (i.key() == i.value())
        i.remove();
}
//! [35multi]


//! [36]
QHash<int, QWidget *> hash;
...
QMutableHashIterator<QString, QWidget *> i(hash);
while (i.hasNext()) {
    i.next();
    qDebug() << i.key() << ": " << i.value();
}
//! [36]


//! [38]
QMutableHashIterator<int, QWidget *> i(hash);
while (i.findNext(widget)) {
    qDebug() << "Found widget " << widget << " under key "
             << i.key();
}
//! [38]


//! [39]
QMutableHashIterator<QString, QString> i(hash);
while (i.hasNext()) {
    i.next();
    if (i.key() == i.value())
        i.remove();
}
//! [39]
