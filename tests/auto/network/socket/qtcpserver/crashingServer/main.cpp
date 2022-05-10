// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore>
#include <QtNetwork>
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
#  include <crtdbg.h>
#endif

int main(int argc, char *argv[])
{
     // Windows: Suppress crash notification dialog.
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
    QCoreApplication app(argc, argv);

    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 49199)) {
        qDebug("Failed to listen: %s", server.errorString().toLatin1().constData());
        return 1;
    }

    printf("Listening\n");
    fflush(stdout);

    server.waitForNewConnection(5000);
    qFatal("Crash");
    return 0;
}
