/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore>
#include <QtNetwork>
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
#  include <crtdbg.h>
#endif
#ifdef Q_OS_UNIX
#  include <unistd.h>
#endif

int main(int argc, char *argv[])
{
     // Windows: Suppress crash notification dialog.
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
    QCoreApplication app(argc, argv);
    if (argc < 1) {
        fprintf(stderr, "Need a port number\n");
        return 1;
    }

    int port = QByteArray(argv[1]).toInt();
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, port)) {
        fprintf(stderr, "Failed to listen: %s\n", server.errorString().toLatin1().constData());
        if (server.serverError() == QTcpSocket::AddressInUseError) {
            // let's see if we can find the process that would be holding this
            // still open
#ifdef Q_OS_LINUX
            static const char *ss_args[] = {
                "ss", "-nap", "sport", "=", argv[1], nullptr
            };
            dup2(STDERR_FILENO, STDOUT_FILENO);
            execvp(ss_args[0], const_cast<char **>(ss_args));
#endif
        }
        return 1;
    }

    printf("Listening\n");
    fflush(stdout);

    server.waitForNewConnection(5000);
    qFatal("Crash");
    return 0;
}
