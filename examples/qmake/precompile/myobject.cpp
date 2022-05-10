// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
#include <iostream>
#include <QDebug>
#include <QObject>
#include "myobject.h"

MyObject::MyObject()
    : QObject()
{
    std::cout << "MyObject::MyObject()\n";
}
//! [0]

MyObject::~MyObject()
{
    qDebug() << "MyObject::~MyObject()";
}
