// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "interface.h"
#include <QThread>

Interface::Interface()
{
}

// Export the sleep function
// TODO QT5: remove this class, QThread::msleep is now public
class FriendlySleepyThread : public QThread {
public:
    using QThread::msleep;
};

int Interface::sleepMethod(int msec)
{
    FriendlySleepyThread::msleep(msec);
    return 42;
}

#include "moc_interface.cpp"
