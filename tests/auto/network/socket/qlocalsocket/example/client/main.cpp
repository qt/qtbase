/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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

