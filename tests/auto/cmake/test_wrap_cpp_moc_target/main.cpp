// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QObject>
#include "myobject1.h"

class MyObject2 : public QObject {
    Q_OBJECT
public:
    MyObject2() = default;
};

int main()
{
    MyObject1 obj1;
    MyObject2 obj2;
    return 0;
}

#include "main.moc"
