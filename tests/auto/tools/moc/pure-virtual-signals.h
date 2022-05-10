// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PURE_VIRTUAL_SIGNALS_H
#define PURE_VIRTUAL_SIGNALS_H
#include <QObject>

class PureVirtualSignalsTest : public QObject
{
    Q_OBJECT
public:
signals:
    virtual void mySignal() = 0;
    void myOtherSignal();
    virtual void mySignal2(int foo) = 0;
};

class PureVirtualSignalsImpl : public PureVirtualSignalsTest
{
    Q_OBJECT
public:
signals:
    void mySignal() override;
    void mySignal2(int foo) override;
};
#endif // PURE_VIRTUAL_SIGNALS_H
