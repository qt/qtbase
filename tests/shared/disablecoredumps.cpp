// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// This file is added to some test helpers that don't link to Qt, so don't
// use Qt #includes here.

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif
#if __has_include(<sys/resource.h>)
#  include <sys/resource.h>
#endif
#ifdef _MSC_VER
#  include <crtdbg.h>
#endif

static void disableCoreDumps()
{
#ifdef _WIN32
    // Windows: suppress the OS error dialog box.
    SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
#  ifdef _MSC_VER
    // MSVC: Suppress runtime's crash notification dialog.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#  endif
#elif defined(RLIMIT_CORE)
    // Unix: set our core dump limit to zero to request no dialogs.
    if (struct rlimit rlim; getrlimit(RLIMIT_CORE, &rlim) == 0) {
        rlim.rlim_cur = 0;
        setrlimit(RLIMIT_CORE, &rlim);
    }
#endif
}

struct DisableCoreDumps
{
    DisableCoreDumps() { disableCoreDumps(); }
};
[[maybe_unused]] DisableCoreDumps disableCoreDumpsConstructorFunction;
