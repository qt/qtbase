// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qassert.h"

#include <QtCore/qlogging.h>

#include <cstdio>
#include <exception>
#ifndef QT_NO_EXCEPTIONS
#include <new>
#endif

QT_BEGIN_NAMESPACE

/*!
    \macro void Q_ASSERT(bool test)
    \relates <QtAssert>

    Prints a warning message containing the source code file name and
    line number if \a test is \c false.

    Q_ASSERT() is useful for testing pre- and post-conditions
    during development. It does nothing if \c QT_NO_DEBUG was defined
    during compilation.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 17

    If \c b is zero, the Q_ASSERT statement will output the following
    message using the qFatal() function:

    \snippet code/src_corelib_global_qglobal.cpp 18

    \sa Q_ASSERT_X(), qFatal(), {Debugging Techniques}
*/

/*!
    \macro void Q_ASSERT_X(bool test, const char *where, const char *what)
    \relates <QtAssert>

    Prints the message \a what together with the location \a where,
    the source file name and line number if \a test is \c false.

    Q_ASSERT_X is useful for testing pre- and post-conditions during
    development. It does nothing if \c QT_NO_DEBUG was defined during
    compilation.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 19

    If \c b is zero, the Q_ASSERT_X statement will output the following
    message using the qFatal() function:

    \snippet code/src_corelib_global_qglobal.cpp 20

    \sa Q_ASSERT(), qFatal(), {Debugging Techniques}
*/

/*
    The Q_ASSERT macro calls this function when the test fails.
*/
void qt_assert(const char *assertion, const char *file, int line) noexcept
{
    QMessageLogger(file, line, nullptr)
            .fatal("ASSERT: \"%s\" in file %s, line %d", assertion, file, line);
}

/*
    The Q_ASSERT_X macro calls this function when the test fails.
*/
void qt_assert_x(const char *where, const char *what, const char *file, int line) noexcept
{
    QMessageLogger(file, line, nullptr)
            .fatal("ASSERT failure in %s: \"%s\", file %s, line %d", where, what, file, line);
}

/*!
    \macro void Q_CHECK_PTR(void *pointer)
    \relates <QtAssert>

    If \a pointer is \nullptr, prints a message containing the source
    code's file name and line number, saying that the program ran out
    of memory and aborts program execution. It throws \c std::bad_alloc instead
    if exceptions are enabled.

    Q_CHECK_PTR does nothing if \c QT_NO_DEBUG and \c QT_NO_EXCEPTIONS were
    defined during compilation. Therefore you must not use Q_CHECK_PTR to check
    for successful memory allocations because the check will be disabled in
    some cases.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 21

    \sa qWarning(), {Debugging Techniques}
*/

/*!
    \fn template <typename T> T *q_check_ptr(T *p)
    \relates <QtAssert>

    Uses Q_CHECK_PTR on \a p, then returns \a p.

    This can be used as an inline version of Q_CHECK_PTR.
*/

/*!
    \internal
    The Q_CHECK_PTR macro calls this function if an allocation check
    fails.
*/
void qt_check_pointer(const char *n, int l) noexcept
{
    // make separate printing calls so that the first one may flush;
    // the second one could want to allocate memory (fputs prints a
    // newline and stderr auto-flushes).
    fputs("Out of memory", stderr);
    fprintf(stderr, "  in %s, line %d\n", n, l);

    std::terminate();
}

/*
    \internal
    Allows you to throw an exception without including <new>
    Called internally from Q_CHECK_PTR on certain OS combinations
*/
void qBadAlloc()
{
#ifndef QT_NO_EXCEPTIONS
    throw std::bad_alloc();
#else
    std::terminate();
#endif
}

/*!
    \macro void Q_ASSUME(bool expr)
    \deprecated
    \relates <QtAssert>
    \since 5.0

    Causes the compiler to assume that \a expr is \c true.

    This macro is known to produce worse code than when no assumption was
    inserted in the code, with some compiler versions. The arguments passed to
    it are always evaluated, even in release mode, with some compilers and not
    others, so application code needs to be aware of those possible differences
    in behavior.

    Do not use it in new code. It is retained as-is for compatibility with old
    code and will likely be removed in the next major version Qt.

    \sa Q_ASSERT(), Q_UNREACHABLE(), Q_LIKELY()
*/

/*!
    \macro void Q_UNREACHABLE()
    \relates <QtAssert>
    \since 5.0

    Tells the compiler that the current point cannot be reached by any
    execution, so it may optimize any code paths leading here as dead code, as
    well as code continuing from here.

    This macro is useful to mark impossible conditions. For example, given the
    following enum:

    \snippet code/src_corelib_global_qglobal.cpp qunreachable-enum

    One can write a switch table like so:

    \snippet code/src_corelib_global_qglobal.cpp qunreachable-switch

    The advantage of inserting Q_UNREACHABLE() at that point is that the
    compiler is told not to generate code for a shape variable containing that
    value. If the macro is missing, the compiler will still generate the
    necessary comparisons for that value. If the case label were removed, some
    compilers could produce a warning that some enum values were not checked.

    By using this macro in impossible conditions, code coverage may be improved
    as dead code paths may be eliminated.

    In debug builds the condition is enforced by an assert to facilitate debugging.

    \note Use the macro Q_UNREACHABLE_RETURN() to insert return statements for
    compilers that need them, without causing warnings for compilers that
    complain about its presence.

    \sa Q_ASSERT(), qFatal(), Q_UNREACHABLE_RETURN()
*/

/*!
    \macro void Q_UNREACHABLE_RETURN(...)
    \relates <QtAssert>
    \since 6.5

    This is equivalent to
    \code
    Q_UNREACHABLE();
    return __VA_ARGS__;
    \endcode
    except it omits the return on compilers that would warn about it.

    \sa Q_UNREACHABLE()
*/
QT_END_NAMESPACE
