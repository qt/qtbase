// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QTest>

// dummy class
class MyObject
{
    public:
        int isReady();
};

// dummy function
int myNetworkServerNotResponding()
{
    return 1;
}

int MyObject::isReady()
{
//! [1]
    int i = 0;
    while (myNetworkServerNotResponding() && i++ < 50)
        QTest::qWait(250);
//! [1]
return 1;
}
