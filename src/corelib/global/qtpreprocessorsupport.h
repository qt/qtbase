// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTPREPROCESSORSUPPORT_H
#define QTPREPROCESSORSUPPORT_H

#if 0
#pragma qt_class(QtPreprocessorSupport)
#pragma qt_sync_stop_processing
#endif

/* These two macros makes it possible to turn the builtin line expander into a
 * string literal. */
#define QT_STRINGIFY2(x) #x
#define QT_STRINGIFY(x) QT_STRINGIFY2(x)

/*
   Avoid "unused parameter" warnings
*/
#define Q_UNUSED(x) (void)x;

#if !defined(Q_UNIMPLEMENTED)
#  define Q_UNIMPLEMENTED() qWarning("Unimplemented code.")
#endif

#define QT_VA_ARGS_CHOOSE(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define QT_VA_ARGS_EXPAND(...) __VA_ARGS__ // Needed for MSVC
#define QT_VA_ARGS_COUNT(...) QT_VA_ARGS_EXPAND(QT_VA_ARGS_CHOOSE(__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define QT_OVERLOADED_MACRO_EXPAND(MACRO, ARGC) MACRO##_##ARGC
#define QT_OVERLOADED_MACRO_IMP(MACRO, ARGC) QT_OVERLOADED_MACRO_EXPAND(MACRO, ARGC)
#define QT_OVERLOADED_MACRO(MACRO, ...) QT_VA_ARGS_EXPAND(QT_OVERLOADED_MACRO_IMP(MACRO, QT_VA_ARGS_COUNT(__VA_ARGS__))(__VA_ARGS__))

#endif // QTPREPROCESSORSUPPORT_H
