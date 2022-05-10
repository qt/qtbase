// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MYOBJECT_H
#define MYOBJECT_H

//! [0]
#include <QObject>

class MyObject : public QObject
{
public:
    MyObject();
    ~MyObject();
};
//! [0]

#endif
