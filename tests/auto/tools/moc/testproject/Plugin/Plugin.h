// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QObject>

struct MyInterface
{
    virtual void blah() = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(MyInterface, "MyInterface")
QT_END_NAMESPACE


