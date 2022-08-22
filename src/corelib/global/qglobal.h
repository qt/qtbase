// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGLOBAL_H
#define QGLOBAL_H

#if 0
#pragma qt_class(QtGlobal)
#endif

#ifdef __cplusplus
#  include <type_traits>
#  include <cstddef>
#  include <utility>
#  include <cstdint>
#endif
#ifndef __ASSEMBLER__
#  include <assert.h>
#  include <stdbool.h>
#  include <stddef.h>
#endif

#include <QtCore/qtversionchecks.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>

/* These two macros makes it possible to turn the builtin line expander into a
 * string literal. */
#define QT_STRINGIFY2(x) #x
#define QT_STRINGIFY(x) QT_STRINGIFY2(x)

inline void qt_noop(void) {}

#include <QtCore/qsystemdetection.h>
#include <QtCore/qprocessordetection.h>
#include <QtCore/qcompilerdetection.h>

#include <QtCore/qassert.h>
#include <QtCore/qtypes.h>
#include <QtCore/qtclasshelpermacros.h>

/*
   Avoid "unused parameter" warnings
*/
#define Q_UNUSED(x) (void)x;

#ifndef __ASSEMBLER__
QT_BEGIN_NAMESPACE

/*
 * If we're compiling C++ code:
 *  - and this is a non-namespace build, declare qVersion as extern "C"
 *  - and this is a namespace build, declare it as a regular function
 *    (we're already inside QT_BEGIN_NAMESPACE / QT_END_NAMESPACE)
 * If we're compiling C code, simply declare the function. If Qt was compiled
 * in a namespace, qVersion isn't callable anyway.
 */
#if !defined(QT_NAMESPACE) && defined(__cplusplus) && !defined(Q_QDOC)
extern "C"
#endif
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION const char *qVersion(void) Q_DECL_NOEXCEPT;

#if defined(__cplusplus)

#ifdef QT_ASCII_CAST_WARNINGS
#  define QT_ASCII_CAST_WARN \
    Q_DECL_DEPRECATED_X("Use fromUtf8, QStringLiteral, or QLatin1StringView")
#else
#  define QT_ASCII_CAST_WARN
#endif

/*
   Utility macros and inline functions
*/

#ifndef Q_FORWARD_DECLARE_OBJC_CLASS
#  ifdef __OBJC__
#    define Q_FORWARD_DECLARE_OBJC_CLASS(classname) @class classname
#  else
#    define Q_FORWARD_DECLARE_OBJC_CLASS(classname) class classname
#  endif
#endif
#ifndef Q_FORWARD_DECLARE_CF_TYPE
#  define Q_FORWARD_DECLARE_CF_TYPE(type) typedef const struct __ ## type * type ## Ref
#endif
#ifndef Q_FORWARD_DECLARE_MUTABLE_CF_TYPE
#  define Q_FORWARD_DECLARE_MUTABLE_CF_TYPE(type) typedef struct __ ## type * type ## Ref
#endif
#ifndef Q_FORWARD_DECLARE_CG_TYPE
#define Q_FORWARD_DECLARE_CG_TYPE(type) typedef const struct type *type ## Ref;
#endif
#ifndef Q_FORWARD_DECLARE_MUTABLE_CG_TYPE
#define Q_FORWARD_DECLARE_MUTABLE_CG_TYPE(type) typedef struct type *type ## Ref;
#endif

#ifdef Q_OS_DARWIN

// Implemented in qcore_mac_objc.mm
class Q_CORE_EXPORT QMacAutoReleasePool
{
public:
    QMacAutoReleasePool();
    ~QMacAutoReleasePool();
private:
    Q_DISABLE_COPY(QMacAutoReleasePool)
    void *pool;
};

#endif // Q_OS_DARWIN

#if 0
#pragma qt_class(QFunctionPointer)
#endif
typedef void (*QFunctionPointer)();

#if !defined(Q_UNIMPLEMENTED)
#  define Q_UNIMPLEMENTED() qWarning("Unimplemented code.")
#endif


// this adds const to non-const objects (like std::as_const)
template <typename T>
constexpr typename std::add_const<T>::type &qAsConst(T &t) noexcept { return t; }
// prevent rvalue arguments:
template <typename T>
void qAsConst(const T &&) = delete;

// like std::exchange
template <typename T, typename U = T>
constexpr T qExchange(T &t, U &&newValue)
noexcept(std::conjunction_v<std::is_nothrow_move_constructible<T>, std::is_nothrow_assignable<T &, U>>)
{
    T old = std::move(t);
    t = std::forward<U>(newValue);
    return old;
}

QT_END_NAMESPACE

// We need to keep QTypeInfo, QSysInfo, QFlags, qDebug & family in qglobal.h for compatibility with Qt 4.
// Be careful when changing the order of these files.
#include <QtCore/qtypeinfo.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qlogging.h>

#include <QtCore/qflags.h>

#include <QtCore/qatomic.h>
#include <QtCore/qconstructormacros.h>
#include <QtCore/qenvironmentvariables.h>
#include <QtCore/qexceptionhandling.h>
#include <QtCore/qforeach.h>
#include <QtCore/qglobalstatic.h>
#include <QtCore/qmalloc.h>
#include <QtCore/qminmax.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qoverload.h>
#include <QtCore/qswap.h>
#include <QtCore/qtdeprecationmarkers.h>
#include <QtCore/qtranslation.h>
#include <QtCore/qtresource.h>
#include <QtCore/qtypetraits.h>
#include <QtCore/qversiontagging.h>

#endif /* __cplusplus */
#endif /* !__ASSEMBLER__ */

#endif /* QGLOBAL_H */
