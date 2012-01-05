/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <string.h>
#include <qstring.h>
#include <qdebug.h>

#include "qlocalsocket.h"

#define SOCK_PATH "echo_socket"

int main(void)
{
    QLocalSocket socket;
    socket.connectToServer(SOCK_PATH);
    socket.open(QIODevice::ReadWrite);

    printf("Connected.\n");
    char str[100];
    while(printf("> "), fgets(str, 100, stdin), !feof(stdin)) {
        if (socket.write(str, strlen(str)) == -1) {
            perror("send");
            return EXIT_FAILURE;
        }

        int t;
        if ((t = socket.read(str, 100)) > 0) {
            str[t] = '\0';
            printf("echo> %s", str);
        } else {
            if (t < 0)
                perror("recv");
            else
                printf("Server closed connection.\n");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

