// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "panic.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

void panic(const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "tracegen: fatal: ");

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);

    exit(EXIT_FAILURE);
}
