// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef ERROR_ON_WRONG_NOTIFY_H
#define ERROR_ON_WRONG_NOTIFY_H
#include <QtCore/QObject>

class ClassWithWrongNOTIFY : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo NOTIFY fooChanged)

    int m_foo;
public:
    void setFoo(int i) { m_foo = i; }
    int foo() { return m_foo; }
};

#endif // ERROR_ON_WRONG_NOTIFY_H
