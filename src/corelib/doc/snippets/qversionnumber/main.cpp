// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Keith Gardner <kreios4004@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QVersionNumber>

class Object
{
public:
    static void genericExample();
    static void equalityExample();
    static void isPrefixOf();
    static void parse();
    static void equivalent();
};

void Object::genericExample()
{
    //! [0]
    QVersionNumber version(1, 2, 3);  // 1.2.3
    //! [0]
}

void Object::equalityExample()
{
    //! [1]
    QVersionNumber v1(1, 2);
    QVersionNumber v2(1, 2, 0);
    int compare = QVersionNumber::compare(v1, v2); // compare == -1
    //! [1]
}

void Object::isPrefixOf()
{
    //! [2]
    QVersionNumber v1(5, 3);
    QVersionNumber v2(5, 3, 1);
    bool value = v1.isPrefixOf(v2); // true
    //! [2]
}

void QObject::parse()
{
    //! [3]
    QString string("5.4.0-alpha");
    qsizetype suffixIndex;
    QVersionNumber version = QVersionNumber::fromString(string, &suffixIndex);
    // version is 5.4.0
    // suffixIndex is 5
    //! [3]

    //! [3-latin1-1]
    QLatin1StringView string("5.4.0-alpha");
    qsizetype suffixIndex;
    auto version = QVersionNumber::fromString(string, &suffixIndex);
    // version is 5.4.0
    // suffixIndex is 5
    //! [3-latin1-1]
}

void Object::equivalent()
{
    //! [4]
    QVersionNumber v1(5, 4);
    QVersionNumber v2(5, 4, 0);
    bool equivalent = v1.normalized() == v2.normalized();
    bool equal = v1 == v2;
    // equivalent is true
    // equal is false
    //! [4]
}

int main()
{
    Object::genericExample();
    Object::equalityExample();
    Object::isPrefixOf();
    Object::parse();
    Object::equivalent();
}
