// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"
#include "qstring.h"
#include "qbytearrayview.h"
#include "qlist.h"
#include "qdir.h"
#include "qdatetime.h"
#include <private/qlocale_tools_p.h>
#include "qnativeinterface.h"
#include "qnativeinterface_p.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <errno.h>
#if defined(Q_CC_MSVC)
#  include <crtdbg.h>
#endif

#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

#if defined(Q_OS_VXWORKS) && defined(_WRS_KERNEL)
#  include <envLib.h>
#endif

#if defined(Q_OS_INTEGRITY)
extern "C" {
    // Function mmap resides in libshm_client.a. To be able to link with it one needs
    // to define symbols 'shm_area_password' and 'shm_area_name', because the library
    // is meant to allow the application that links to it to use POSIX shared memory
    // without full system POSIX.
#  pragma weak shm_area_password
#  pragma weak shm_area_name
    char shm_area_password[] = "dummy";
    char shm_area_name[] = "dummy";
}
#endif

#ifdef qFatal
// the qFatal in this file are just redirections from elsewhere, so
// don't capture any context again
#  undef qFatal
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \headerfile <QtGlobal>
    \inmodule QtCore
    \title Global Qt Declarations
    \ingroup funclists

    \brief The <QtGlobal> header file includes the fundamental global
    declarations. It is included by most other Qt header files.

    The global declarations include \l{types}, \l{functions} and
    \l{macros}.

    The type definitions are partly convenience definitions for basic
    types (some of which guarantee certain bit-sizes on all platforms
    supported by Qt), partly types related to Qt message handling. The
    functions are related to generating messages, Qt version handling
    and comparing and adjusting object values. And finally, some of
    the declared macros enable programmers to add compiler or platform
    specific code to their applications, while others are convenience
    macros for larger operations.

    \section1 Types

    The header file declares several type definitions that guarantee a
    specified bit-size on all platforms supported by Qt for various
    basic types, for example \l qint8 which is a signed char
    guaranteed to be 8-bit on all platforms supported by Qt. The
    header file also declares the \l qlonglong type definition for \c
    {long long int } (\c __int64 on Windows).

    Several convenience type definitions are declared: \l qreal for \c
    double or \c float, \l uchar for \c unsigned char, \l uint for \c unsigned
    int, \l ulong for \c unsigned long and \l ushort for \c unsigned
    short.

    Finally, the QtMsgType definition identifies the various messages
    that can be generated and sent to a Qt message handler;
    QtMessageHandler is a type definition for a pointer to a function with
    the signature
    \c {void myMessageHandler(QtMsgType, const QMessageLogContext &, const char *)}.
    QMessageLogContext class contains the line, file, and function the
    message was logged at. This information is created by the QMessageLogger
    class.

    \section1 Functions

    The <QtGlobal> header file contains several functions comparing
    and adjusting an object's value. These functions take a template
    type as argument: You can retrieve the absolute value of an object
    using the qAbs() function, and you can bound a given object's
    value by given minimum and maximum values using the qBound()
    function. You can retrieve the minimum and maximum of two given
    objects using qMin() and qMax() respectively. All these functions
    return a corresponding template type; the template types can be
    replaced by any other type.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 3

    <QtGlobal> also contains functions that generate messages from the
    given string argument: qDebug(), qInfo(), qWarning(), qCritical(),
    and qFatal(). These functions call the message handler
    with the given message.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 4

    The remaining functions are qRound() and qRound64(), which both
    accept a \c double or \c float value as their argument returning
    the value rounded up to the nearest integer and 64-bit integer
    respectively, the qInstallMessageHandler() function which installs
    the given QtMessageHandler, and the qVersion() function which
    returns the version number of Qt at runtime as a string.

    \section1 Macros

    The <QtGlobal> header file provides a range of macros (Q_CC_*)
    that are defined if the application is compiled using the
    specified platforms. For example, the Q_CC_SUN macro is defined if
    the application is compiled using Forte Developer, or Sun Studio
    C++.  The header file also declares a range of macros (Q_OS_*)
    that are defined for the specified platforms. For example,
    Q_OS_UNIX which is defined for the Unix-based systems.

    The purpose of these macros is to enable programmers to add
    compiler or platform specific code to their application.

    The remaining macros are convenience macros for larger operations:
    The QT_TR_NOOP(), QT_TRANSLATE_NOOP(), and QT_TRANSLATE_NOOP3()
    macros provide the possibility of marking strings for delayed
    translation. QT_TR_N_NOOP(), QT_TRANSLATE_N_NOOP(), and
    QT_TRANSLATE_N_NOOP3() are numerator dependent variants of these.
    The Q_ASSERT() and Q_ASSERT_X() enables warning messages of various
    level of refinement. The Q_FOREACH() and foreach() macros
    implement Qt's foreach loop.

    The Q_INT64_C() and Q_UINT64_C() macros wrap signed and unsigned
    64-bit integer literals in a platform-independent way. The
    Q_CHECK_PTR() macro prints a warning containing the source code's
    file name and line number, saying that the program ran out of
    memory, if the pointer is \nullptr. The qPrintable() and qUtf8Printable()
    macros represent an easy way of printing text.

    The QT_POINTER_SIZE macro expands to the size of a pointer in bytes.

    The macros QT_VERSION and QT_VERSION_STR expand to a numeric value or a
    string, respectively. These identify the version of Qt that the application
    is compiled with.

    \sa <QtAlgorithms>, QSysInfo
*/

/*
    Dijkstra's bisection algorithm to find the square root of an integer.
    Deliberately not exported as part of the Qt API, but used in both
    qsimplerichtext.cpp and qgfxraster_qws.cpp
*/
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION unsigned int qt_int_sqrt(unsigned int n)
{
    // n must be in the range 0...UINT_MAX/2-1
    if (n >= (UINT_MAX >> 2)) {
        unsigned int r = 2 * qt_int_sqrt(n / 4);
        unsigned int r2 = r + 1;
        return (n >= r2 * r2) ? r2 : r;
    }
    uint h, p = 0, q = 1, r = n;
    while (q <= n)
        q <<= 2;
    while (q != 1) {
        q >>= 2;
        h = p + q;
        p >>= 1;
        if (r >= h) {
            p += q;
            r -= h;
        }
    }
    return p;
}

void qAbort()
{
#ifdef Q_OS_WIN
    // std::abort() in the MSVC runtime will call _exit(3) if the abort
    // behavior is _WRITE_ABORT_MSG - see also _set_abort_behavior(). This is
    // the default for a debug-mode build of the runtime. Worse, MinGW's
    // std::abort() implementation (in msvcrt.dll) is basically a call to
    // _exit(3) too. Unfortunately, _exit() and _Exit() *do* run the static
    // destructors of objects in DLLs, a violation of the C++ standard (see
    // [support.start.term]). So we bypass std::abort() and directly
    // terminate the application.

#  if defined(Q_CC_MSVC)
    if (IsProcessorFeaturePresent(PF_FASTFAIL_AVAILABLE))
        __fastfail(FAST_FAIL_FATAL_APP_EXIT);
#  else
    RaiseFailFastException(nullptr, nullptr, 0);
#  endif

    // Fallback
    TerminateProcess(GetCurrentProcess(), STATUS_FATAL_APP_EXIT);

    // Tell the compiler the application has stopped.
    Q_UNREACHABLE_IMPL();
#else // !Q_OS_WIN
    std::abort();
#endif
}

// Also specified to behave as if they call tzset():
// localtime() -- but not localtime_r(), which we use when threaded
// strftime() -- not used (except in tests)

/*!
    \fn template <typename T> typename std::add_const<T>::type &qAsConst(T &t)
    \relates <QtGlobal>
    \since 5.7

    Returns \a t cast to \c{const T}.

    This function is a Qt implementation of C++17's std::as_const(),
    a cast function like std::move(). But while std::move() turns
    lvalues into rvalues, this function turns non-const lvalues into
    const lvalues. Like std::as_const(), it doesn't work on rvalues,
    because it cannot be efficiently implemented for rvalues without
    leaving dangling references.

    Its main use in Qt is to prevent implicitly-shared Qt containers
    from detaching:
    \snippet code/src_corelib_global_qglobal.cpp as-const-0

    Of course, in this case, you could (and probably should) have declared
    \c s as \c const in the first place:
    \snippet code/src_corelib_global_qglobal.cpp as-const-1
    but often that is not easily possible.

    It is important to note that qAsConst() does not copy its argument,
    it just performs a \c{const_cast<const T&>(t)}. This is also the reason
    why it is designed to fail for rvalues: The returned reference would go
    stale too soon. So while this works (but detaches the returned object):
    \snippet code/src_corelib_global_qglobal.cpp as-const-2

    this would not:
    \snippet code/src_corelib_global_qglobal.cpp as-const-3

    To prevent this construct from compiling (and failing at runtime), qAsConst() has
    a second, deleted, overload which binds to rvalues.
*/

/*!
    \fn template <typename T> void qAsConst(const T &&t)
    \relates <QtGlobal>
    \since 5.7
    \overload

    This overload is deleted to prevent a dangling reference in code like
    \snippet code/src_corelib_global_qglobal.cpp as-const-4
*/

/*!
    \fn template <typename T, typename U = T> T qExchange(T &obj, U &&newValue)
    \relates <QtGlobal>
    \since 5.14

    Replaces the value of \a obj with \a newValue and returns the old value of \a obj.

    This is Qt's implementation of std::exchange(). It differs from std::exchange()
    only in that it is \c constexpr already in C++14, and available on all supported
    compilers.

    Here is how to use qExchange() to implement move constructors:
    \code
    MyClass(MyClass &&other)
      : m_pointer{qExchange(other.m_pointer, nullptr)},
        m_int{qExchange(other.m_int, 0)},
        m_vector{std::move(other.m_vector)},
        ...
    \endcode

    For members of class type, we can use std::move(), as their move-constructor will
    do the right thing. But for scalar types such as raw pointers or integer type, move
    is the same as copy, which, particularly for pointers, is not what we expect. So, we
    cannot use std::move() for such types, but we can use std::exchange()/qExchange() to
    make sure the source object's member is already reset by the time we get to the
    initialization of our next data member, which might come in handy if the constructor
    exits with an exception.

    Here is how to use qExchange() to write a loop that consumes the collection it
    iterates over:
    \code
    for (auto &e : qExchange(collection, {})
        doSomethingWith(e);
    \endcode

    Which is equivalent to the following, much more verbose code:
    \code
    {
        auto tmp = std::move(collection);
        collection = {};                    // or collection.clear()
        for (auto &e : tmp)
            doSomethingWith(e);
    }                                       // destroys 'tmp'
    \endcode

    This is perfectly safe, as the for-loop keeps the result of qExchange() alive for as
    long as the loop runs, saving the declaration of a temporary variable. Be aware, though,
    that qExchange() returns a non-const object, so Qt containers may detach.
*/

/*!
    \macro Q_UNUSED(name)
    \relates <QtGlobal>

    Indicates to the compiler that the parameter with the specified
    \a name is not used in the body of a function. This can be used to
    suppress compiler warnings while allowing functions to be defined
    with meaningful parameter names in their signatures.
*/

struct QInternal_CallBackTable
{
    QList<QList<qInternalCallback>> callbacks;
};

Q_GLOBAL_STATIC(QInternal_CallBackTable, global_callback_table)

bool QInternal::registerCallback(Callback cb, qInternalCallback callback)
{
    if (unsigned(cb) < unsigned(QInternal::LastCallback)) {
        QInternal_CallBackTable *cbt = global_callback_table();
        cbt->callbacks.resize(cb + 1);
        cbt->callbacks[cb].append(callback);
        return true;
    }
    return false;
}

bool QInternal::unregisterCallback(Callback cb, qInternalCallback callback)
{
    if (unsigned(cb) < unsigned(QInternal::LastCallback)) {
        if (global_callback_table.exists()) {
            QInternal_CallBackTable *cbt = global_callback_table();
            return cbt->callbacks[cb].removeAll(callback) > 0;
        }
    }
    return false;
}

bool QInternal::activateCallbacks(Callback cb, void **parameters)
{
    Q_ASSERT_X(cb >= 0, "QInternal::activateCallback()", "Callback id must be a valid id");

    if (!global_callback_table.exists())
        return false;

    QInternal_CallBackTable *cbt = &(*global_callback_table);
    if (cbt && cb < cbt->callbacks.size()) {
        QList<qInternalCallback> callbacks = cbt->callbacks[cb];
        bool ret = false;
        for (int i = 0; i < callbacks.size(); ++i)
            ret |= (callbacks.at(i))(parameters);
        return ret;
    }
    return false;
}

/*!
    \macro QT_NAMESPACE
    \internal

    If this macro is defined to \c ns all Qt classes are put in a namespace
    called \c ns. Also, moc will output code putting metaobjects etc.
    into namespace \c ns.

    \sa QT_BEGIN_NAMESPACE, QT_END_NAMESPACE,
    QT_PREPEND_NAMESPACE, QT_USE_NAMESPACE,
    QT_BEGIN_INCLUDE_NAMESPACE, QT_END_INCLUDE_NAMESPACE,
    QT_BEGIN_MOC_NAMESPACE, QT_END_MOC_NAMESPACE,
*/

/*!
    \macro QT_PREPEND_NAMESPACE(identifier)
    \internal

    This macro qualifies \a identifier with the full namespace.
    It expands to \c{::QT_NAMESPACE::identifier} if \c QT_NAMESPACE is defined
    and only \a identifier otherwise.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_USE_NAMESPACE
    \internal

    This macro expands to using QT_NAMESPACE if QT_NAMESPACE is defined
    and nothing otherwise.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_BEGIN_NAMESPACE
    \internal

    This macro expands to

    \snippet code/src_corelib_global_qglobal.cpp begin namespace macro

    if \c QT_NAMESPACE is defined and nothing otherwise. If should always
    appear in the file-level scope and be followed by \c QT_END_NAMESPACE
    at the same logical level with respect to preprocessor conditionals
    in the same file.

    As a rule of thumb, \c QT_BEGIN_NAMESPACE should appear in all Qt header
    and Qt source files after the last \c{#include} line and before the first
    declaration.

    If that rule can't be followed because, e.g., \c{#include} lines and
    declarations are wildly mixed, place \c QT_BEGIN_NAMESPACE before
    the first declaration and wrap the \c{#include} lines in
    \c QT_BEGIN_INCLUDE_NAMESPACE and \c QT_END_INCLUDE_NAMESPACE.

    When using the \c QT_NAMESPACE feature in user code
    (e.g., when building plugins statically linked to Qt) where
    the user code is not intended to go into the \c QT_NAMESPACE
    namespace, all forward declarations of Qt classes need to
    be wrapped in \c QT_BEGIN_NAMESPACE and \c QT_END_NAMESPACE.
    After that, a \c QT_USE_NAMESPACE should follow.
    No further changes should be needed.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_END_NAMESPACE
    \internal

    This macro expands to

    \snippet code/src_corelib_global_qglobal.cpp end namespace macro

    if \c QT_NAMESPACE is defined and nothing otherwise. It is used to cancel
    the effect of \c QT_BEGIN_NAMESPACE.

    If a source file ends with a \c{#include} directive that includes a moc file,
    \c QT_END_NAMESPACE should be placed before that \c{#include}.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_BEGIN_INCLUDE_NAMESPACE
    \internal

    This macro is equivalent to \c QT_END_NAMESPACE.
    It only serves as syntactic sugar and is intended
    to be used before #include lines within a
    \c QT_BEGIN_NAMESPACE ... \c QT_END_NAMESPACE block.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_END_INCLUDE_NAMESPACE
    \internal

    This macro is equivalent to \c QT_BEGIN_NAMESPACE.
    It only serves as syntactic sugar and is intended
    to be used after #include lines within a
    \c QT_BEGIN_NAMESPACE ... \c QT_END_NAMESPACE block.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_BEGIN_MOC_NAMESPACE
    \internal

    This macro is output by moc at the beginning of
    moc files. It is equivalent to \c QT_USE_NAMESPACE.

    \sa QT_NAMESPACE
*/

/*!
    \macro QT_END_MOC_NAMESPACE
    \internal

    This macro is output by moc at the beginning of
    moc files. It expands to nothing.

    \sa QT_NAMESPACE
*/

/*!
    \macro qMove(x)
    \relates <QtGlobal>
    \deprecated

    Use \c std::move instead.

    It expands to "std::move".

    qMove takes an rvalue reference to its parameter \a x, and converts it to an xvalue.
*/

/*!
    \macro QT_TERMINATE_ON_EXCEPTION(expr)
    \relates <QtGlobal>
    \internal

    In general, use of the Q_DECL_NOEXCEPT macro is preferred over
    Q_DECL_NOTHROW, because it exhibits well-defined behavior and
    supports the more powerful Q_DECL_NOEXCEPT_EXPR variant. However,
    use of Q_DECL_NOTHROW has the advantage that Windows builds
    benefit on a wide range or compiler versions that do not yet
    support the C++11 noexcept feature.

    It may therefore be beneficial to use Q_DECL_NOTHROW and emulate
    the C++11 behavior manually with an embedded try/catch.

    Qt provides the QT_TERMINATE_ON_EXCEPTION(expr) macro for this
    purpose. It either expands to \c expr (if Qt is compiled without
    exception support or the compiler supports C++11 noexcept
    semantics) or to
    \snippet code/src_corelib_global_qglobal.cpp qterminate
    otherwise.

    Since this macro expands to just \c expr if the compiler supports
    C++11 noexcept, expecting the compiler to take over responsibility
    of calling std::terminate() in that case, it should not be used
    outside Q_DECL_NOTHROW functions.

    \sa Q_DECL_NOEXCEPT, Q_DECL_NOTHROW, qTerminate()
*/

namespace QtPrivate {
Q_LOGGING_CATEGORY(lcNativeInterface, "qt.nativeinterface")
}

QT_END_NAMESPACE
