// Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MYINTERFACE_H
#define MYINTERFACE_H

#include <qglobal.h>

class MyInterface
{

};

QT_BEGIN_NAMESPACE

Q_DECLARE_INTERFACE(MyInterface, "org.cmake.example.MyInterface")

QT_END_NAMESPACE

#endif
