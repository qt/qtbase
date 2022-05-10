// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qglobal.h"
#include "qversiontagging.h"

extern "C" {
#define SYM QT_MANGLE_NAMESPACE(qt_version_tag)
#define SSYM QT_STRINGIFY(SYM)

// With compilers that have the "alias" attribute, the macro creates a global
// variable "qt_version_tag_M_m" (M: major, m: minor) as an alias to either
// qt_version_tag or _qt_version_tag. Everywhere else, we simply create a new
// global variable qt_version_tag_M_m without aliasing to a single variable.
//
// Additionally, on systems using ELF binaries (Linux, FreeBSD, etc.), we
// create ELF versions by way of the .symver assembly directive[1].
//
// Unfortunately, Clang on Darwin systems says it supports the "alias"
// attribute, but fails when used. That's a Clang bug (as of XCode 12).
//
// [1] https://sourceware.org/binutils/docs/as/Symver.html

#if defined(Q_CC_GNU) && defined(Q_OF_ELF)
#  define make_versioned_symbol2(sym, m, n, separator)     \
    Q_CORE_EXPORT extern __attribute__((alias("_" SSYM))) const char sym ## _ ## m ## _ ## n; \
    asm(".symver " QT_STRINGIFY(sym) "_" QT_STRINGIFY(m) "_" QT_STRINGIFY(n) ", " \
        QT_STRINGIFY(sym) separator "Qt_" QT_STRINGIFY(m) "." QT_STRINGIFY(n))

extern const char QT_MANGLE_NAMESPACE(_qt_version_tag) = 0;
#elif __has_attribute(alias) && !defined(Q_OS_DARWIN)
#  define make_versioned_symbol2(sym, m, n, separator)     \
    Q_CORE_EXPORT extern __attribute__((alias(SSYM))) const char sym ## _ ## m ## _ ## n
extern const char SYM = 0;
#else
#  define make_versioned_symbol2(sym, m, n, separator)     \
    Q_CORE_EXPORT extern const char sym ## _ ## m ## _ ## n = 0;
#endif
#define make_versioned_symbol(sym, m, n, separator)    make_versioned_symbol2(sym, m, n, separator)

#if QT_VERSION_MINOR > 0
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 0, "@");
#endif
#if QT_VERSION_MINOR > 1
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 1, "@");
#endif
#if QT_VERSION_MINOR > 2
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 2, "@");
#endif
#if QT_VERSION_MINOR > 3
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 3, "@");
#endif
#if QT_VERSION_MINOR > 4
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 4, "@");
#endif
#if QT_VERSION_MINOR > 5
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 5, "@");
#endif
#if QT_VERSION_MINOR > 6
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 6, "@");
#endif
#if QT_VERSION_MINOR > 7
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 7, "@");
#endif
#if QT_VERSION_MINOR > 8
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 8, "@");
#endif
#if QT_VERSION_MINOR > 9
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 9, "@");
#endif
#if QT_VERSION_MINOR > 10
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 10, "@");
#endif
#if QT_VERSION_MINOR > 11
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 11, "@");
#endif
#if QT_VERSION_MINOR > 12
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 12, "@");
#endif
#if QT_VERSION_MINOR > 13
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 13, "@");
#endif
#if QT_VERSION_MINOR > 14
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 14, "@");
#endif
#if QT_VERSION_MINOR > 15
make_versioned_symbol(SYM, QT_VERSION_MAJOR, 15, "@");
#endif
#if QT_VERSION_MINOR > 16
#  error "Please update this file with more Qt versions."
#endif

// the default version:
make_versioned_symbol(SYM, QT_VERSION_MAJOR, QT_VERSION_MINOR, "@@");
}

QT_BEGIN_NAMESPACE

static_assert(std::is_trivially_destructible_v<QtPrivate::QVersionTag>);

QT_END_NAMESPACE
