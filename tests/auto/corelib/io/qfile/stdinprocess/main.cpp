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


#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage: stdinprocess <all|line <0|1>>\n");
        printf("echos all its input to its output.\n");
        return 1;
    }

    QFile file;

    if (strcmp(argv[1], "all") == 0) {
        file.open(stdin, QFile::ReadWrite);
        printf("%s", file.readAll().constData());
    } else if (strcmp(argv[1], "line") == 0) {
        if (strcmp(argv[2], "0") == 0) {
            file.open(stdin, QFile::ReadWrite);
        } else {
            file.open(0, QFile::ReadWrite);
        }

        char line[1024];
        while (file.readLine(line, sizeof(line)) > 0) {
            printf("%s", line);
            fflush(stdout);
        }
    }
    return 0;
}
