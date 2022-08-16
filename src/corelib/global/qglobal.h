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

#ifdef QT_BOOTSTRAPPED
#include <QtCore/qconfig-bootstrapped.h>
#else
#include <QtCore/qconfig.h>
#include <QtCore/qtcore-config.h>
#endif

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

/*
   Avoid "unused parameter" warnings
*/
#define Q_UNUSED(x) (void)x;

#ifndef __ASSEMBLER__
QT_BEGIN_NAMESPACE

/*
   Some classes do not permit copies to be made of an object. These
   classes contains a private copy constructor and assignment
   operator to disable copying (the compiler gives an error message).
*/
#define Q_DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

#define Q_DISABLE_COPY_MOVE(Class) \
    Q_DISABLE_COPY(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

/*
    Implementing a move assignment operator using an established
    technique (move-and-swap, pure swap) is just boilerplate.
    Here's a couple of *private* macros for convenience.

    To know which one to use:

    * if you don't have a move constructor (*) => use pure swap;
    * if you have a move constructor, then
      * if your class holds just memory (no file handles, no user-defined
        datatypes, etc.) => use pure swap;
      * use move and swap.

    The preference should always go for the move-and-swap one, as it
    will deterministically destroy the data previously held in *this,
    and not "dump" it in the moved-from object (which may then be alive
    for longer).

    The requirement for either macro is the presence of a member swap(),
    which any value class that defines its own special member functions
    should have anyhow.

    (*) Many value classes in Qt do not have move constructors; mostly,
    the implicitly shared classes using QSharedDataPointer and friends.
    The reason is mostly historical: those classes require either an
    out-of-line move constructor, which we could not provide before we
    made C++11 mandatory (and that we don't like anyhow), or
    an out-of-line dtor for the Q(E)DSP<Private> member (cf. QPixmap).

    If you can however add a move constructor to a class lacking it,
    consider doing so, then reevaluate which macro to choose.
*/
#define QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(Class) \
    Class &operator=(Class &&other) noexcept { \
        Class moved(std::move(other)); \
        swap(moved); \
        return *this; \
    }

#define QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(Class) \
    Class &operator=(Class &&other) noexcept { \
        swap(other); \
        return *this; \
    }

/*
   No, this is not an evil backdoor. QT_BUILD_INTERNAL just exports more symbols
   for Qt's internal unit tests. If you want slower loading times and more
   symbols that can vanish from version to version, feel free to define QT_BUILD_INTERNAL.
*/
#if defined(QT_BUILD_INTERNAL) && defined(QT_BUILDING_QT) && defined(QT_SHARED)
#    define Q_AUTOTEST_EXPORT Q_DECL_EXPORT
#elif defined(QT_BUILD_INTERNAL) && defined(QT_SHARED)
#    define Q_AUTOTEST_EXPORT Q_DECL_IMPORT
#else
#    define Q_AUTOTEST_EXPORT
#endif

#define Q_INIT_RESOURCE(name) \
    do { extern int QT_MANGLE_NAMESPACE(qInitResources_ ## name) ();       \
        QT_MANGLE_NAMESPACE(qInitResources_ ## name) (); } while (false)
#define Q_CLEANUP_RESOURCE(name) \
    do { extern int QT_MANGLE_NAMESPACE(qCleanupResources_ ## name) ();    \
        QT_MANGLE_NAMESPACE(qCleanupResources_ ## name) (); } while (false)

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

#ifndef Q_CONSTRUCTOR_FUNCTION
# define Q_CONSTRUCTOR_FUNCTION0(AFUNC) \
    namespace { \
    static const struct AFUNC ## _ctor_class_ { \
        inline AFUNC ## _ctor_class_() { AFUNC(); } \
    } AFUNC ## _ctor_instance_; \
    }

# define Q_CONSTRUCTOR_FUNCTION(AFUNC) Q_CONSTRUCTOR_FUNCTION0(AFUNC)
#endif

#ifndef Q_DESTRUCTOR_FUNCTION
# define Q_DESTRUCTOR_FUNCTION0(AFUNC) \
    namespace { \
    static const struct AFUNC ## _dtor_class_ { \
        inline AFUNC ## _dtor_class_() { } \
        inline ~ AFUNC ## _dtor_class_() { AFUNC(); } \
    } AFUNC ## _dtor_instance_; \
    }
# define Q_DESTRUCTOR_FUNCTION(AFUNC) Q_DESTRUCTOR_FUNCTION0(AFUNC)
#endif

/* moc compats (signals/slots) */
#ifndef QT_MOC_COMPAT
#  define QT_MOC_COMPAT
#else
#  undef QT_MOC_COMPAT
#  define QT_MOC_COMPAT
#endif

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

Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qSharedBuild() noexcept;

/*
   Debugging and error handling
*/

#if !defined(QT_NO_DEBUG) && !defined(QT_DEBUG)
#  define QT_DEBUG
#endif

#if 0
#pragma qt_class(QFunctionPointer)
#endif
typedef void (*QFunctionPointer)();

#if !defined(Q_UNIMPLEMENTED)
#  define Q_UNIMPLEMENTED() qWarning("Unimplemented code.")
#endif

/*
   Compilers which follow outdated template instantiation rules
   require a class to have a comparison operator to exist when
   a QList of this type is instantiated. It's not actually
   used in the list, though. Hence the dummy implementation.
   Just in case other code relies on it we better trigger a warning
   mandating a real implementation.
*/

#ifdef Q_FULL_TEMPLATE_INSTANTIATION
#  define Q_DUMMY_COMPARISON_OPERATOR(C) \
    bool operator==(const C&) const { \
        qWarning(#C"::operator==(const "#C"&) was called"); \
        return false; \
    }
#else

#  define Q_DUMMY_COMPARISON_OPERATOR(C)
#endif

QT_WARNING_PUSH
// warning: noexcept-expression evaluates to 'false' because of a call to 'void swap(..., ...)'
QT_WARNING_DISABLE_GCC("-Wnoexcept")

namespace QtPrivate
{
namespace SwapExceptionTester { // insulate users from the "using std::swap" below
    using std::swap; // import std::swap
    template <typename T>
    void checkSwap(T &t)
        noexcept(noexcept(swap(t, t)));
    // declared, but not implemented (only to be used in unevaluated contexts (noexcept operator))
}
} // namespace QtPrivate

// Documented in ../tools/qalgorithm.qdoc
template <typename T>
constexpr void qSwap(T &value1, T &value2)
    noexcept(noexcept(QtPrivate::SwapExceptionTester::checkSwap(value1)))
{
    using std::swap;
    swap(value1, value2);
}

// pure compile-time micro-optimization for our own headers, so not documented:
template <typename T>
constexpr inline void qt_ptr_swap(T* &lhs, T* &rhs) noexcept
{
    T *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
}

QT_WARNING_POP

Q_CORE_EXPORT void *qMallocAligned(size_t size, size_t alignment) Q_ALLOC_SIZE(1);
Q_CORE_EXPORT void *qReallocAligned(void *ptr, size_t size, size_t oldsize, size_t alignment) Q_ALLOC_SIZE(2);
Q_CORE_EXPORT void qFreeAligned(void *ptr);


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

// like std::to_underlying
template <typename Enum>
constexpr std::underlying_type_t<Enum> qToUnderlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

template <typename T> inline T *qGetPtrHelper(T *ptr) noexcept { return ptr; }
template <typename Ptr> inline auto qGetPtrHelper(Ptr &ptr) noexcept -> decltype(ptr.get())
{ static_assert(noexcept(ptr.get()), "Smart d pointers for Q_DECLARE_PRIVATE must have noexcept get()"); return ptr.get(); }

// The body must be a statement:
#define Q_CAST_IGNORE_ALIGN(body) QT_WARNING_PUSH QT_WARNING_DISABLE_GCC("-Wcast-align") body QT_WARNING_POP
#define Q_DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() noexcept \
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<Class##Private *>(qGetPtrHelper(d_ptr));) } \
    inline const Class##Private* d_func() const noexcept \
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<const Class##Private *>(qGetPtrHelper(d_ptr));) } \
    friend class Class##Private;

#define Q_DECLARE_PRIVATE_D(Dptr, Class) \
    inline Class##Private* d_func() noexcept \
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<Class##Private *>(qGetPtrHelper(Dptr));) } \
    inline const Class##Private* d_func() const noexcept \
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<const Class##Private *>(qGetPtrHelper(Dptr));) } \
    friend class Class##Private;

#define Q_DECLARE_PUBLIC(Class)                                    \
    inline Class* q_func() noexcept { return static_cast<Class *>(q_ptr); } \
    inline const Class* q_func() const noexcept { return static_cast<const Class *>(q_ptr); } \
    friend class Class;

#define Q_D(Class) Class##Private * const d = d_func()
#define Q_Q(Class) Class * const q = q_func()

#define QT_MODULE(x)

// This macro can be used to calculate member offsets for types with a non standard layout.
// It uses the fact that offsetof() is allowed to support those types since C++17 as an optional
// feature. All our compilers do support this, but some issue a warning, so we wrap the offsetof()
// call in a macro that disables the compiler warning.
#define Q_OFFSETOF(Class, member) \
    []() -> size_t { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, member); \
        QT_WARNING_POP \
    }()

QT_END_NAMESPACE

// We need to keep QTypeInfo, QSysInfo, QFlags, qDebug & family in qglobal.h for compatibility with Qt 4.
// Be careful when changing the order of these files.
#include <QtCore/qtypeinfo.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qlogging.h>

#include <QtCore/qflags.h>

#include <QtCore/qatomic.h>
#include <QtCore/qenvironmentvariables.h>
#include <QtCore/qexceptionhandling.h>
#include <QtCore/qforeach.h>
#include <QtCore/qglobalstatic.h>
#include <QtCore/qminmax.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qoverload.h>
#include <QtCore/qtdeprecationmarkers.h>
#include <QtCore/qtranslation.h>
#include <QtCore/qversiontagging.h>

#endif /* __cplusplus */
#endif /* !__ASSEMBLER__ */

#endif /* QGLOBAL_H */
