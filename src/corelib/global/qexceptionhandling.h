// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEXCEPTIONHANDLING_H
#define QEXCEPTIONHANDLING_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qcompilerdetection.h>

#if 0
#pragma qt_class(QtExceptionHandling)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

/* These wrap try/catch so we can switch off exceptions later.

   Beware - do not use more than one QT_CATCH per QT_TRY, and do not use
   the exception instance in the catch block.
   If you can't live with those constraints, don't use these macros.
   Use the QT_NO_EXCEPTIONS macro to protect your code instead.
*/
Q_NORETURN Q_DECL_COLD_FUNCTION Q_CORE_EXPORT void qTerminate() noexcept;
#ifdef QT_NO_EXCEPTIONS
#  define QT_TRY if (true)
#  define QT_CATCH(A) else
#  define QT_THROW(A) qt_noop()
#  define QT_RETHROW qt_noop()
#  define QT_TERMINATE_ON_EXCEPTION(expr) do { expr; } while (false)
#else
#  define QT_TRY try
#  define QT_CATCH(A) catch (A)
#  define QT_THROW(A) throw A
#  define QT_RETHROW throw
#  ifdef Q_COMPILER_NOEXCEPT
#    define QT_TERMINATE_ON_EXCEPTION(expr) do { expr; } while (false)
#  else
#    define QT_TERMINATE_ON_EXCEPTION(expr) do { try { expr; } catch (...) { qTerminate(); } } while (false)
#  endif
#endif

QT_END_NAMESPACE

#endif // QEXCEPTIONHANDLING_H
