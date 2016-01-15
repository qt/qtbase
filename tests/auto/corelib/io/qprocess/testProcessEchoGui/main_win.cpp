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


#include <windows.h>

int APIENTRY WinMain(HINSTANCE /* hInstance */,
                     HINSTANCE /* hPrevInstance */,
                     LPSTR     /* lpCmdLine */,
                     int       /* nCmdShow */)
{

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    HANDLE hStderr = GetStdHandle(STD_ERROR_HANDLE);

    for (;;) {

        char c = 0;
        DWORD read = 0;
        if (!ReadFile(hStdin, &c, 1, &read, 0) || read == 0 || c == 'q' || c == '\0')
            break;
        DWORD wrote = 0;
        WriteFile(hStdout, &c, 1, &wrote, 0);
        WriteFile(hStderr, &c, 1, &wrote, 0);
    }
    return 0;
}
