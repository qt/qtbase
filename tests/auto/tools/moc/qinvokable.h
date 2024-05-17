// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QINVOKABLE_H
#define QINVOKABLE_H

#include <QObject>

class InvokableBeforeReturnType : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE const char *foo() const { return ""; }
};

class InvokableBeforeInline : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE inline void foo() {}
    Q_INVOKABLE virtual void bar() {}
};
#endif // QINVOKABLE_H
