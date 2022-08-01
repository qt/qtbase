// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QASSERT_H
#define QASSERT_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtnamespacemacros.h>

#if 0
#pragma qt_class(QtAssert)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

#ifndef Q_CC_MSVC
Q_NORETURN
#endif
Q_DECL_COLD_FUNCTION
Q_CORE_EXPORT void qt_assert(const char *assertion, const char *file, int line) noexcept;

#if !defined(Q_ASSERT)
#  if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#    define Q_ASSERT(cond) static_cast<void>(false && (cond))
#  else
#    define Q_ASSERT(cond) ((cond) ? static_cast<void>(0) : qt_assert(#cond, __FILE__, __LINE__))
#  endif
#endif

#ifndef Q_CC_MSVC
Q_NORETURN
#endif
Q_DECL_COLD_FUNCTION
Q_CORE_EXPORT
void qt_assert_x(const char *where, const char *what, const char *file, int line) noexcept;

#if !defined(Q_ASSERT_X)
#  if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#    define Q_ASSERT_X(cond, where, what) static_cast<void>(false && (cond))
#  else
#    define Q_ASSERT_X(cond, where, what) ((cond) ? static_cast<void>(0) : qt_assert_x(where, what, __FILE__, __LINE__))
#  endif
#endif

QT_END_NAMESPACE

#endif // QASSERT_H
