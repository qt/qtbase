// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QTcpSocket>

int test_tcpwait()
{
    QTcpSocket socket;
    socket.connectToHost("localhost", 1025);

//! [0]
    int numRead = 0, numReadTotal = 0;
    char buffer[50];

    forever {
        numRead  = socket.read(buffer, 50);

        // do whatever with array

        numReadTotal += numRead;
        if (numRead == 0 && !socket.waitForReadyRead())
            break;
    }
//! [0]
    return numReadTotal;
}
