// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "interface.h"
#include <QThread>

Interface::Interface()
{
}

int Interface::sleepMethod(int msec)
{
    QThread::sleep(std::chrono::milliseconds{msec});
    return 42;
}

#include "moc_interface.cpp"
