// Copyright (C) 2025 Aurélien Brooke <aurelien@bahiasoft.fr>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qalloc.h"

#include <QtCore/qalgorithms.h>
#include <QtCore/qtpreprocessorsupport.h>

#include <cstdlib>

#if QT_CONFIG(jemalloc)
#include <jemalloc/jemalloc.h>
#endif

QT_BEGIN_NAMESPACE

size_t QtPrivate::expectedAllocSize(size_t allocSize, size_t alignment) noexcept
{
    Q_ASSERT(qPopulationCount(alignment) == 1);
#if QT_CONFIG(jemalloc)
    return ::nallocx(allocSize, MALLOCX_ALIGN(alignment));
#endif
    Q_UNUSED(allocSize);
    Q_UNUSED(alignment);
    return 0;
}

void QtPrivate::sizedFree(void *ptr, size_t allocSize) noexcept
{
#if QT_CONFIG(jemalloc)
    // jemalloc is okay with free(nullptr), as required by the standard,
    // but will asssert (in debug) or invoke UB (in release) on sdallocx(nullptr, ...),
    // so don't allow Qt to do that.
    if (Q_LIKELY(ptr)) {
        ::sdallocx(ptr, allocSize, 0);
        return;
    }
#endif
    Q_UNUSED(allocSize);
    ::free(ptr);
}

QT_END_NAMESPACE
