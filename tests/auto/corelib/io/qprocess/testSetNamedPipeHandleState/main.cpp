// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <windows.h>

int main()
{
    DWORD mode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    if (SetNamedPipeHandleState(GetStdHandle(STD_INPUT_HANDLE), &mode, NULL, NULL))
        return 0;
    return GetLastError();
}
