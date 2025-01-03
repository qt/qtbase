// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <qt_windows.h>
#include <shellapi.h>

/*
  This file contains the code in the QtEntryPoint library for Windows.
  QtEntryPoint contains the Windows startup code and is required for
  linking to the Qt DLL.

  When a Windows application starts, the WinMain function is
  invoked.
*/

#if defined(QT_NEEDS_QMAIN)
int qMain(int, char **);
#define main qMain
#else
extern "C" int main(int, char **);
#endif

/*
  WinMain() - Initializes Windows and calls user's startup function main().
  NOTE: WinMain() won't be called if the application was linked as a "console"
  application.
*/

// Convert a wchar_t to char string, equivalent to QString::toLocal8Bit()
// when passed CP_ACP.
static inline char *wideToMulti(unsigned int codePage, const wchar_t *aw)
{
    const int required = WideCharToMultiByte(codePage, 0, aw, -1, nullptr, 0, nullptr, nullptr);
    char *result = new char[required];
    WideCharToMultiByte(codePage, 0, aw, -1, result, required, nullptr, nullptr);
    return result;
}

static inline int qtEntryPoint()
{
    int argc = 0;
    wchar_t **argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argvW == nullptr)
        return -1;
    char **argv = new char *[argc + 1];
    for (int i = 0; i != argc; ++i)
        argv[i] = wideToMulti(CP_ACP, argvW[i]);
    argv[argc] = nullptr;
    LocalFree(argvW);
    const int exitCode = main(argc, argv);
    for (int i = 0; (i != argc) && (argv[i] != nullptr); ++i)
        delete [] argv[i];
    delete [] argv;
    return exitCode;
}

extern "C" int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return qtEntryPoint();
}

extern "C" int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    return qtEntryPoint();
}
