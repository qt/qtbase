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

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \headerfile <QtGlobal>
    \inmodule QtCore
    \title Global Qt Declarations
    \ingroup funclists

    \brief The <QtGlobal> header file includes an assortment of other headers.

    Up to Qt 6.5, most Qt header files included <QtGlobal>. Before Qt 6.5,
    <QtGlobal> defined an assortment of global declarations. Most of these
    have moved, at Qt 6.5, to separate headers, so that source code can
    include only what it needs, rather than the whole assortment. For now,
    <QtGlobal> includes those other headers (see next section), but future
    releases of Qt may remove some of these headers from <QtGlobal> or
    condition their inclusion on a version check. Likewise, in future
    releases, some Qt headers that currently include <QtGlobal> may stop
    doing so. The hope is that this will improve compilation times by
    avoiding global declarations when they are not used.

    \section1 List of Headers Extracted from <QtGlobal>

    \table
    \header \li Header                      \li Summary
    \row    \li <QFlags>                    \li Type-safe way of combining enum values
    \row    \li \l <QForeach>               \li Qt's implementation of foreach and forever loops
    \row    \li \l <QFunctionPointer>       \li Typedef for a pointer-to-function type
    \row    \li \l <QApplicationStatic>     \li For Q_APPLICATION_STATIC
    \row    \li <QGlobalStatic>             \li Thread-safe initialization of global static objects
    \row    \li \l <QOverload>              \li Helpers for resolving member function overloads
    \row    \li <QSysInfo>                  \li A helper class to get system information
    \row    \li \l <QTypeInfo>              \li Helpers to get type information
    \row    \li \l <QtAssert>               \li Q_ASSERT and other runtime checks
    \row    \li \l <QtClassHelperMacros>    \li Qt class helper macros
    \row    \li \l <QtCompilerDetection>    \li Compiler-specific macro definitions
    \row    \li \l <QtDeprecationMarkers>   \li Deprecation helper macros
    \row    \li \l <QtEnvironmentVariables> \li Helpers for working with environment variables
    \row    \li <QtExceptionHandling>       \li Helpers for exception handling
    \row    \li \l <QtLogging>              \li Qt logging helpers
    \row    \li <QtMalloc>                  \li Memory allocation helpers
    \row    \li \l <QtMinMax>               \li Helpers for comparing values
    \row    \li \l <QtNumeric>              \li Various numeric functions
    \row    \li \l <QtPreprocessorSupport>  \li Helper preprocessor macros
    \row    \li \l <QtProcessorDetection>   \li Architecture-specific macro definitions
    \row    \li \l <QtResource>             \li Helpers for initializing and cleaning resources
    \row    \li \l <QtSwap>                 \li Implementation of qSwap()
    \row    \li \l <QtSystemDetection>      \li Platform-specific macro definitions
    \row    \li \l <QtTranslation>          \li Qt translation helpers
    \row    \li \l <QtTypeTraits>           \li Qt type traits
    \row    \li \l <QtTypes>                \li Qt fundamental type declarations
    \row    \li \l <QtVersionChecks>        \li QT_VERSION_CHECK and related checks
    \row    \li \l <QtVersion>              \li QT_VERSION_STR and qVersion()
    \endtable
*/

/*
    Dijkstra's bisection algorithm to find the square root of an integer.
    Deliberately not exported as part of the Qt API, but used in
    qtextdocument.cpp.
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

// Also specified to behave as if they call tzset():
// localtime() -- but not localtime_r(), which we use when threaded
// strftime() -- not used (except in tests)

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
    \macro QT_NO_KEYWORDS
    \relates <QtGlobal>

    Define this macro to disable the Qt-specific keywords that are usually enabled,
    such as \c signals and \c slots. Use \c Q_SIGNALS and \c Q_SLOTS instead.

    Libraries should define this macro to make sure that they don't use the generic
    keywords without the \c Q_ prefix in their public headers.

    \sa QT_NO_FOREACH
*/

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
    \macro QT_ENABLE_STRICT_MODE_UP_TO
    \relates <QtGlobal>
    \since 6.8

    Defining this macro will disable a number of Qt APIs that are
    deemed suboptimal or dangerous.

    This macro's value must be set to a Qt version, using
    \l{QT_VERSION_CHECK}'s encoding. For instance, in order to set it
    to Qt 6.6, define \c{QT_ENABLE_STRICT_MODE_UP_TO=0x060600}.
    This will disable only the APIs introduced in versions up to (and
    including) the specified Qt version.

    If the \l QT_DISABLE_DEPRECATED_UP_TO macro is \e not defined,
    then QT_ENABLE_STRICT_MODE_UP_TO will define it as well,
    to the same value.

    This macro should always be set to the minimum Qt version that
    your project wants to support.

    The APIs disabled by this macro are listed in the table below,
    together with the minimum value to use in order to disable them.

    \table
    \header \li Version \li Disabled APIs
    \row \li 6.0.0 \li \l{foreach-keyword}{foreach} (see \l{QT_NO_FOREACH})
    \row \li 6.0.0 \li QString conversions towards \c{const char *} / QByteArray (see \l{QT_NO_CAST_TO_ASCII})
    \row \li 6.0.0 \li QByteArray implicit conversions towards \c{const char *} (see \l{QT_NO_CAST_FROM_BYTEARRAY})
    \row \li 6.0.0 \li QUrl implicit conversions from QString (see \l{QT_NO_URL_CAST_FROM_STRING})
    \row \li 6.0.0 \li Allowing narrowing conversions in signal-slot connections (see \l{QT_NO_NARROWING_CONVERSIONS_IN_CONNECT})
    \row \li 6.0.0 \li Java-style iterators for Qt containers
    \row \li 6.6.0 \li The qExchange() function (see \l{QT_NO_QEXCHANGE})
    \row \li 6.7.0 \li Overloads of QObject::connect that do not take a context object (see \l{QT_NO_CONTEXTLESS_CONNECT})
    \row \li 6.8.0 \li The qAsConst() function (see \l{QT_NO_QASCONST})
    \row \li 6.8.0 \li File-related I/O classes have their \c{open()} functions marked \c{[[nodiscard]]} (see \l{QT_USE_NODISCARD_FILE_OPEN})
    \row\li  6.9.0 \li The qsnprintf() and qvnprintf() functions (see \l{QT_NO_QSNPRINTF}).
    \endtable

    Moreover, individual APIs may also get disabled as part of the
    setting of QT_DISABLE_DEPRECATED_UP_TO. Please refer to each class'
    documentation for more details.

    \sa QT_DISABLE_DEPRECATED_UP_TO, QT_NO_KEYWORDS, QT_VERSION_CHECK
*/

Q_LOGGING_CATEGORY(lcNativeInterface, "qt.nativeinterface")

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qnamespace.cpp"
#endif
