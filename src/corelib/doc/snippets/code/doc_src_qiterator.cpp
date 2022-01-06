/****************************************************************************
**
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


//! [7]
QSetIterator<QString> i(set);
i.toBack();
while (i.hasPrevious())
    QString s = i.previous();
//! [7]


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
