// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QtCore/QThreadStorage>

class Class
{
public:
    ~Class()
    {
        // trigger creation of a new QThreadStorage, after the previous QThreadStorage from main() was destructed
        static QThreadStorage<int *> threadstorage;
        threadstorage.setLocalData(new int);
        threadstorage.setLocalData(new int);
    }
};

int main()
{
    // instantiate the class that will use QThreadStorage from its destructor, it's destructor will be run last
    static Class instance;
    // instantiate QThreadStorage, it's destructor (and the global destructors for QThreadStorages internals) will run first
    static QThreadStorage<int *> threadstorage;
    threadstorage.setLocalData(new int);
    threadstorage.setLocalData(new int);
}
