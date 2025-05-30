// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TRACEPOINTGEN_H
#define TRACEPOINTGEN_H

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#define DEBUG_TRACEPOINTGEN 0

#if DEBUG_TRACEPOINTGEN > 0
    #define DEBUGPRINTF(x) x
    #if (DEBUG_TRACEPOINTGEN > 1)
        #define DEBUGPRINTF2(x) x
    #else
        #define DEBUGPRINTF2(x)
    #endif
#else
    #define DEBUGPRINTF(x)
    #define DEBUGPRINTF2(x)
#endif



inline void panic(const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "tracepointgen: fatal: ");

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);

    exit(EXIT_FAILURE);
}

#endif
