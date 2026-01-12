// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qobject.h>

class MyObject : public QObject
{
    Q_OBJECT
public:
    MyObject(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
};

void instantiateObject()
{
    MyObject obj;
    obj.setObjectName("MyObject");
}

#include "myobject.moc"
