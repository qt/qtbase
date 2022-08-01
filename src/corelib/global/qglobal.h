// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGLOBAL_H
#define QGLOBAL_H

#if 0
#pragma qt_class(QtGlobal)
#pragma qt_class(QIntegerForSize)
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

/*
   The Qt modules' export macros.
   The options are:
    - defined(QT_STATIC): Qt was built or is being built in static mode
    - defined(QT_SHARED): Qt was built or is being built in shared/dynamic mode
   If neither was defined, then QT_SHARED is implied. If Qt was compiled in static
   mode, QT_STATIC is defined in qconfig.h. In shared mode, QT_STATIC is implied
   for the bootstrapped tools.
*/

#ifdef QT_BOOTSTRAPPED
#  ifdef QT_SHARED
#    error "QT_SHARED and QT_BOOTSTRAPPED together don't make sense. Please fix the build"
#  elif !defined(QT_STATIC)
#    define QT_STATIC
#  endif
#endif

#if defined(QT_SHARED) || !defined(QT_STATIC)
#  ifdef QT_STATIC
#    error "Both QT_SHARED and QT_STATIC defined, please make up your mind"
#  endif
#  ifndef QT_SHARED
#    define QT_SHARED
#  endif
#endif

#include <QtCore/qtcoreexports.h>

/*
    The QT_CONFIG macro implements a safe compile time check for features of Qt.
    Features can be in three states:
        0 or undefined: This will lead to a compile error when testing for it
        -1: The feature is not available
        1: The feature is available
*/
#define QT_CONFIG(feature) (1/QT_FEATURE_##feature == 1)
#define QT_REQUIRE_CONFIG(feature) Q_STATIC_ASSERT_X(QT_FEATURE_##feature == 1, "Required feature " #feature " for file " __FILE__ " not available.")

/*
   helper macros to make some simple code work active in Qt 6 or Qt 7 only,
   like:
     struct QT6_ONLY(Q_CORE_EXPORT) QTrivialClass
     {
         void QT7_ONLY(Q_CORE_EXPORT) void operate();
     }
*/
#if QT_VERSION_MAJOR == 7
#  define QT7_ONLY(...)         __VA_ARGS__
#  define QT6_ONLY(...)
#elif QT_VERSION_MAJOR == 6
#  define QT7_ONLY(...)
#  define QT6_ONLY(...)         __VA_ARGS__
#else
#  error Qt major version not 6 or 7
#endif

/* Macro and tag type to help overload resolution on functions
   that are, e.g., QT_REMOVED_SINCE'ed. Example use:

   #if QT_CORE_REMOVED_SINCE(6, 4)
   int size() const;
   #endif
   qsizetype size(QT6_DECL_NEW_OVERLOAD) const;

   in the normal cpp file:

   qsizetype size(QT6_IMPL_NEW_OVERLOAD) const {
      ~~~
   }

   in removed_api.cpp:

   int size() const { return int(size(QT6_CALL_NEW_OVERLOAD)); }
*/
#ifdef Q_CLANG_QDOC
# define QT6_DECL_NEW_OVERLOAD
# define QT6_DECL_NEW_OVERLOAD_TAIL
# define QT6_IMPL_NEW_OVERLOAD
# define QT6_IMPL_NEW_OVERLOAD_TAIL
# define QT6_CALL_NEW_OVERLOAD
# define QT6_CALL_NEW_OVERLOAD_TAIL
#else
# define QT6_DECL_NEW_OVERLOAD QT6_ONLY(Qt::Disambiguated_t = Qt::Disambiguated)
# define QT6_DECL_NEW_OVERLOAD_TAIL QT6_ONLY(, QT6_DECL_NEW_OVERLOAD)
# define QT6_IMPL_NEW_OVERLOAD QT6_ONLY(Qt::Disambiguated_t)
# define QT6_IMPL_NEW_OVERLOAD_TAIL QT6_ONLY(, QT6_IMPL_NEW_OVERLOAD)
# define QT6_CALL_NEW_OVERLOAD QT6_ONLY(Qt::Disambiguated)
# define QT6_CALL_NEW_OVERLOAD_TAIL QT6_ONLY(, QT_CALL_NEW_OVERLOAD)
#endif

/* These two macros makes it possible to turn the builtin line expander into a
 * string literal. */
#define QT_STRINGIFY2(x) #x
#define QT_STRINGIFY(x) QT_STRINGIFY2(x)

#include <QtCore/qsystemdetection.h>
#include <QtCore/qprocessordetection.h>
#include <QtCore/qcompilerdetection.h>

#if defined (__ELF__)
#  define Q_OF_ELF
#endif
#if defined (__MACH__) && defined (__APPLE__)
#  define Q_OF_MACH_O
#endif

/*
   Avoid "unused parameter" warnings
*/
#define Q_UNUSED(x) (void)x;

#if defined(__cplusplus)
// Don't use these in C++ mode, use static_assert directly.
// These are here only to keep old code compiling.
#  define Q_STATIC_ASSERT(Condition) static_assert(bool(Condition), #Condition)
#  define Q_STATIC_ASSERT_X(Condition, Message) static_assert(bool(Condition), Message)
#elif defined(Q_COMPILER_STATIC_ASSERT)
// C11 mode - using the _S version in case <assert.h> doesn't do the right thing
#  define Q_STATIC_ASSERT(Condition) _Static_assert(!!(Condition), #Condition)
#  define Q_STATIC_ASSERT_X(Condition, Message) _Static_assert(!!(Condition), Message)
#else
// C89 & C99 version
#  define Q_STATIC_ASSERT_PRIVATE_JOIN(A, B) Q_STATIC_ASSERT_PRIVATE_JOIN_IMPL(A, B)
#  define Q_STATIC_ASSERT_PRIVATE_JOIN_IMPL(A, B) A ## B
#  ifdef __COUNTER__
#  define Q_STATIC_ASSERT(Condition) \
    typedef char Q_STATIC_ASSERT_PRIVATE_JOIN(q_static_assert_result, __COUNTER__) [(Condition) ? 1 : -1];
#  else
#  define Q_STATIC_ASSERT(Condition) \
    typedef char Q_STATIC_ASSERT_PRIVATE_JOIN(q_static_assert_result, __LINE__) [(Condition) ? 1 : -1];
#  endif /* __COUNTER__ */
#  define Q_STATIC_ASSERT_X(Condition, Message) Q_STATIC_ASSERT(Condition)
#endif

# include <QtCore/qtnamespacemacros.h>

#if defined(Q_OS_DARWIN) && !defined(QT_LARGEFILE_SUPPORT)
#  define QT_LARGEFILE_SUPPORT 64
#endif

#ifndef __ASSEMBLER__
QT_BEGIN_NAMESPACE

/*
   Size-dependent types (architechture-dependent byte order)

   Make sure to update QMetaType when changing these typedefs
*/

typedef signed char qint8;         /* 8 bit signed */
typedef unsigned char quint8;      /* 8 bit unsigned */
typedef short qint16;              /* 16 bit signed */
typedef unsigned short quint16;    /* 16 bit unsigned */
typedef int qint32;                /* 32 bit signed */
typedef unsigned int quint32;      /* 32 bit unsigned */
// Unlike LL / ULL in C++, for historical reasons, we force the
// result to be of the requested type.
#ifdef __cplusplus
#  define Q_INT64_C(c) static_cast<long long>(c ## LL)     /* signed 64 bit constant */
#  define Q_UINT64_C(c) static_cast<unsigned long long>(c ## ULL) /* unsigned 64 bit constant */
#else
#  define Q_INT64_C(c) ((long long)(c ## LL))               /* signed 64 bit constant */
#  define Q_UINT64_C(c) ((unsigned long long)(c ## ULL))    /* unsigned 64 bit constant */
#endif
typedef long long qint64;           /* 64 bit signed */
typedef unsigned long long quint64; /* 64 bit unsigned */

typedef qint64 qlonglong;
typedef quint64 qulonglong;

#ifndef __cplusplus
// In C++ mode, we define below using QIntegerForSize template
Q_STATIC_ASSERT_X(sizeof(ptrdiff_t) == sizeof(size_t), "Weird ptrdiff_t and size_t definitions");
typedef ptrdiff_t qptrdiff;
typedef ptrdiff_t qsizetype;
typedef ptrdiff_t qintptr;
typedef size_t quintptr;

#define PRIdQPTRDIFF "td"
#define PRIiQPTRDIFF "ti"

#define PRIdQSIZETYPE "td"
#define PRIiQSIZETYPE "ti"

#define PRIdQINTPTR "td"
#define PRIiQINTPTR "ti"

#define PRIuQUINTPTR "zu"
#define PRIoQUINTPTR "zo"
#define PRIxQUINTPTR "zx"
#define PRIXQUINTPTR "zX"
#endif

/*
   Useful type definitions for Qt
*/

QT_BEGIN_INCLUDE_NAMESPACE
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
QT_END_INCLUDE_NAMESPACE

#if defined(QT_COORD_TYPE)
typedef QT_COORD_TYPE qreal;
#else
typedef double qreal;
#endif

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

/*
  quintptr and qptrdiff is guaranteed to be the same size as a pointer, i.e.

      sizeof(void *) == sizeof(quintptr)
      && sizeof(void *) == sizeof(qptrdiff)

  size_t and qsizetype are not guaranteed to be the same size as a pointer, but
  they usually are. We actually check for that in qglobal.cpp.
*/
template <int> struct QIntegerForSize;
template <>    struct QIntegerForSize<1> { typedef quint8  Unsigned; typedef qint8  Signed; };
template <>    struct QIntegerForSize<2> { typedef quint16 Unsigned; typedef qint16 Signed; };
template <>    struct QIntegerForSize<4> { typedef quint32 Unsigned; typedef qint32 Signed; };
template <>    struct QIntegerForSize<8> { typedef quint64 Unsigned; typedef qint64 Signed; };
#if defined(Q_CC_GNU) && defined(__SIZEOF_INT128__)
template <>    struct QIntegerForSize<16> { __extension__ typedef unsigned __int128 Unsigned; __extension__ typedef __int128 Signed; };
#endif
template <class T> struct QIntegerForSizeof: QIntegerForSize<sizeof(T)> { };
typedef QIntegerForSize<Q_PROCESSOR_WORDSIZE>::Signed qregisterint;
typedef QIntegerForSize<Q_PROCESSOR_WORDSIZE>::Unsigned qregisteruint;
typedef QIntegerForSizeof<void *>::Unsigned quintptr;
typedef QIntegerForSizeof<void *>::Signed qptrdiff;
typedef qptrdiff qintptr;
using qsizetype = QIntegerForSizeof<std::size_t>::Signed;

// These custom definitions are necessary as we're not defining our
// datatypes in terms of the language ones, but in terms of integer
// types that have the sime size. For instance, on a 32-bit platform,
// qptrdiff is int, while ptrdiff_t may be aliased to long; therefore
// using %td to print a qptrdiff would be wrong (and raise -Wformat
// warnings), although both int and long have same bit size on that
// platform.
//
// We know that sizeof(size_t) == sizeof(void *) == sizeof(qptrdiff).
#if SIZE_MAX == 4294967295ULL
#define PRIuQUINTPTR "u"
#define PRIoQUINTPTR "o"
#define PRIxQUINTPTR "x"
#define PRIXQUINTPTR "X"

#define PRIdQPTRDIFF "d"
#define PRIiQPTRDIFF "i"

#define PRIdQINTPTR "d"
#define PRIiQINTPTR "i"

#define PRIdQSIZETYPE "d"
#define PRIiQSIZETYPE "i"
#elif SIZE_MAX == 18446744073709551615ULL
#define PRIuQUINTPTR "llu"
#define PRIoQUINTPTR "llo"
#define PRIxQUINTPTR "llx"
#define PRIXQUINTPTR "llX"

#define PRIdQPTRDIFF "lld"
#define PRIiQPTRDIFF "lli"

#define PRIdQINTPTR "lld"
#define PRIiQINTPTR "lli"

#define PRIdQSIZETYPE "lld"
#define PRIiQSIZETYPE "lli"
#else
#error Unsupported platform (unknown value for SIZE_MAX)
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

#ifdef Q_PROCESSOR_X86_32
#  if defined(Q_CC_GNU)
#    define QT_FASTCALL __attribute__((regparm(3)))
#  elif defined(Q_CC_MSVC)
#    define QT_FASTCALL __fastcall
#  else
#     define QT_FASTCALL
#  endif
#else
#  define QT_FASTCALL
#endif

// enable gcc warnings for printf-style functions
#if defined(Q_CC_GNU) && !defined(__INSURE__)
#  if defined(Q_CC_MINGW) && !defined(Q_CC_CLANG)
#    define Q_ATTRIBUTE_FORMAT_PRINTF(A, B) \
         __attribute__((format(gnu_printf, (A), (B))))
#  else
#    define Q_ATTRIBUTE_FORMAT_PRINTF(A, B) \
         __attribute__((format(printf, (A), (B))))
#  endif
#else
#  define Q_ATTRIBUTE_FORMAT_PRINTF(A, B)
#endif

#ifdef Q_CC_MSVC
#  define Q_NEVER_INLINE __declspec(noinline)
#  define Q_ALWAYS_INLINE __forceinline
#elif defined(Q_CC_GNU)
#  define Q_NEVER_INLINE __attribute__((noinline))
#  define Q_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#  define Q_NEVER_INLINE
#  define Q_ALWAYS_INLINE inline
#endif

//defines the type for the WNDPROC on windows
//the alignment needs to be forced for sse2 to not crash with mingw
#if defined(Q_OS_WIN)
#  if defined(Q_CC_MINGW) && defined(Q_PROCESSOR_X86_32)
#    define QT_ENSURE_STACK_ALIGNED_FOR_SSE __attribute__ ((force_align_arg_pointer))
#  else
#    define QT_ENSURE_STACK_ALIGNED_FOR_SSE
#  endif
#  define QT_WIN_CALLBACK CALLBACK QT_ENSURE_STACK_ALIGNED_FOR_SSE
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
#  define QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios, tvos, watchos) \
    ((defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && macos != __MAC_NA && __MAC_OS_X_VERSION_MAX_ALLOWED >= macos) || \
     (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) && ios != __IPHONE_NA && __IPHONE_OS_VERSION_MAX_ALLOWED >= ios) || \
     (defined(__TV_OS_VERSION_MAX_ALLOWED) && tvos != __TVOS_NA && __TV_OS_VERSION_MAX_ALLOWED >= tvos) || \
     (defined(__WATCH_OS_VERSION_MAX_ALLOWED) && watchos != __WATCHOS_NA && __WATCH_OS_VERSION_MAX_ALLOWED >= watchos))

#  define QT_DARWIN_DEPLOYMENT_TARGET_BELOW(macos, ios, tvos, watchos) \
    ((defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && macos != __MAC_NA && __MAC_OS_X_VERSION_MIN_REQUIRED < macos) || \
     (defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && ios != __IPHONE_NA && __IPHONE_OS_VERSION_MIN_REQUIRED < ios) || \
     (defined(__TV_OS_VERSION_MIN_REQUIRED) && tvos != __TVOS_NA && __TV_OS_VERSION_MIN_REQUIRED < tvos) || \
     (defined(__WATCH_OS_VERSION_MIN_REQUIRED) && watchos != __WATCHOS_NA && __WATCH_OS_VERSION_MIN_REQUIRED < watchos))

#  define QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios, __TVOS_NA, __WATCHOS_NA)
#  define QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(macos) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, __IPHONE_NA, __TVOS_NA, __WATCHOS_NA)
#  define QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(ios) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_NA, ios, __TVOS_NA, __WATCHOS_NA)
#  define QT_TVOS_PLATFORM_SDK_EQUAL_OR_ABOVE(tvos) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_NA, __IPHONE_NA, tvos, __WATCHOS_NA)
#  define QT_WATCHOS_PLATFORM_SDK_EQUAL_OR_ABOVE(watchos) \
      QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_NA, __IPHONE_NA, __TVOS_NA, watchos)

#  define QT_MACOS_IOS_DEPLOYMENT_TARGET_BELOW(macos, ios) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(macos, ios, __TVOS_NA, __WATCHOS_NA)
#  define QT_MACOS_DEPLOYMENT_TARGET_BELOW(macos) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(macos, __IPHONE_NA, __TVOS_NA, __WATCHOS_NA)
#  define QT_IOS_DEPLOYMENT_TARGET_BELOW(ios) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(__MAC_NA, ios, __TVOS_NA, __WATCHOS_NA)
#  define QT_TVOS_DEPLOYMENT_TARGET_BELOW(tvos) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(__MAC_NA, __IPHONE_NA, tvos, __WATCHOS_NA)
#  define QT_WATCHOS_DEPLOYMENT_TARGET_BELOW(watchos) \
      QT_DARWIN_DEPLOYMENT_TARGET_BELOW(__MAC_NA, __IPHONE_NA, __TVOS_NA, watchos)

// Compatibility synonyms, do not use
#  define QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(osx, ios) QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(osx, ios)
#  define QT_MAC_DEPLOYMENT_TARGET_BELOW(osx, ios) QT_MACOS_IOS_DEPLOYMENT_TARGET_BELOW(osx, ios)
#  define QT_OSX_PLATFORM_SDK_EQUAL_OR_ABOVE(osx) QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(osx)
#  define QT_OSX_DEPLOYMENT_TARGET_BELOW(osx) QT_MACOS_DEPLOYMENT_TARGET_BELOW(osx)

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

#else

#define QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios, tvos, watchos) (0)
#define QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(macos, ios) (0)
#define QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(macos) (0)
#define QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(ios) (0)
#define QT_TVOS_PLATFORM_SDK_EQUAL_OR_ABOVE(tvos) (0)
#define QT_WATCHOS_PLATFORM_SDK_EQUAL_OR_ABOVE(watchos) (0)

#define QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(osx, ios) (0)
#define QT_OSX_PLATFORM_SDK_EQUAL_OR_ABOVE(osx) (0)

#endif // Q_OS_DARWIN

inline void qt_noop(void) {}

/* These wrap try/catch so we can switch off exceptions later.

   Beware - do not use more than one QT_CATCH per QT_TRY, and do not use
   the exception instance in the catch block.
   If you can't live with those constraints, don't use these macros.
   Use the QT_NO_EXCEPTIONS macro to protect your code instead.
*/

#if !defined(QT_NO_EXCEPTIONS)
#  if !defined(Q_MOC_RUN)
#    if (defined(Q_CC_CLANG) && !__has_feature(cxx_exceptions)) || \
        (defined(Q_CC_GNU) && !defined(__EXCEPTIONS))
#      define QT_NO_EXCEPTIONS
#    endif
#  elif defined(QT_BOOTSTRAPPED)
#    define QT_NO_EXCEPTIONS
#  endif
#endif

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

Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qSharedBuild() noexcept;

#ifndef Q_OUTOFLINE_TEMPLATE
#  define Q_OUTOFLINE_TEMPLATE
#endif
#ifndef Q_INLINE_TEMPLATE
#  define Q_INLINE_TEMPLATE inline
#endif

/*
   Debugging and error handling
*/

#if !defined(QT_NO_DEBUG) && !defined(QT_DEBUG)
#  define QT_DEBUG
#endif

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
Q_CORE_EXPORT void qt_assert_x(const char *where, const char *what, const char *file, int line) noexcept;

#if !defined(Q_ASSERT_X)
#  if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#    define Q_ASSERT_X(cond, where, what) static_cast<void>(false && (cond))
#  else
#    define Q_ASSERT_X(cond, where, what) ((cond) ? static_cast<void>(0) : qt_assert_x(where, what, __FILE__, __LINE__))
#  endif
#endif

Q_NORETURN Q_CORE_EXPORT void qt_check_pointer(const char *, int) noexcept;
Q_NORETURN Q_DECL_COLD_FUNCTION
Q_CORE_EXPORT void qBadAlloc();

#ifdef QT_NO_EXCEPTIONS
#  if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#    define Q_CHECK_PTR(p) qt_noop()
#  else
#    define Q_CHECK_PTR(p) do {if (!(p)) qt_check_pointer(__FILE__,__LINE__);} while (false)
#  endif
#else
#  define Q_CHECK_PTR(p) do { if (!(p)) qBadAlloc(); } while (false)
#endif

template <typename T>
inline T *q_check_ptr(T *p) { Q_CHECK_PTR(p); return p; }

typedef void (*QFunctionPointer)();

#if !defined(Q_UNIMPLEMENTED)
#  define Q_UNIMPLEMENTED() qWarning("Unimplemented code.")
#endif

namespace QTypeTraits {

namespace detail {
template<typename T, typename U,
         typename = std::enable_if_t<std::is_arithmetic_v<T> && std::is_arithmetic_v<U> &&
                                     std::is_floating_point_v<T> == std::is_floating_point_v<U> &&
                                     std::is_signed_v<T> == std::is_signed_v<U> &&
                                     !std::is_same_v<T, bool> && !std::is_same_v<U, bool> &&
                                     !std::is_same_v<T, char> && !std::is_same_v<U, char>>>
struct Promoted
{
    using type = decltype(T() + U());
};
}

template <typename T, typename U>
using Promoted = typename detail::Promoted<T, U>::type;

}

template <typename T>
constexpr inline const T &qMin(const T &a, const T &b) { return (a < b) ? a : b; }
template <typename T>
constexpr inline const T &qMax(const T &a, const T &b) { return (a < b) ? b : a; }
template <typename T>
constexpr inline const T &qBound(const T &min, const T &val, const T &max)
{
    Q_ASSERT(!(max < min));
    return qMax(min, qMin(max, val));
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qMin(const T &a, const U &b)
{
    using P = QTypeTraits::Promoted<T, U>;
    P _a = a;
    P _b = b;
    return (_a < _b) ? _a : _b;
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qMax(const T &a, const U &b)
{
    using P = QTypeTraits::Promoted<T, U>;
    P _a = a;
    P _b = b;
    return (_a < _b) ? _b : _a;
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qBound(const T &min, const U &val, const T &max)
{
    Q_ASSERT(!(max < min));
    return qMax(min, qMin(max, val));
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qBound(const T &min, const T &val, const U &max)
{
    using P = QTypeTraits::Promoted<T, U>;
    Q_ASSERT(!(P(max) < P(min)));
    return qMax(min, qMin(max, val));
}
template <typename T, typename U>
constexpr inline QTypeTraits::Promoted<T, U> qBound(const U &min, const T &val, const T &max)
{
    using P = QTypeTraits::Promoted<T, U>;
    Q_ASSERT(!(P(max) < P(min)));
    return qMax(min, qMin(max, val));
}

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


/*
   Avoid some particularly useless warnings from some stupid compilers.
   To get ALL C++ compiler warnings, define QT_CC_WARNINGS or comment out
   the line "#define QT_NO_WARNINGS".
*/
#if !defined(QT_CC_WARNINGS)
#  define QT_NO_WARNINGS
#endif
#if defined(QT_NO_WARNINGS)
#  if defined(Q_CC_MSVC)
QT_WARNING_DISABLE_MSVC(4251) /* class 'type' needs to have dll-interface to be used by clients of class 'type2' */
QT_WARNING_DISABLE_MSVC(4244) /* conversion from 'type1' to 'type2', possible loss of data */
QT_WARNING_DISABLE_MSVC(4275) /* non - DLL-interface classkey 'identifier' used as base for DLL-interface classkey 'identifier' */
QT_WARNING_DISABLE_MSVC(4514) /* unreferenced inline function has been removed */
QT_WARNING_DISABLE_MSVC(4800) /* 'type' : forcing value to bool 'true' or 'false' (performance warning) */
QT_WARNING_DISABLE_MSVC(4097) /* typedef-name 'identifier1' used as synonym for class-name 'identifier2' */
QT_WARNING_DISABLE_MSVC(4706) /* assignment within conditional expression */
QT_WARNING_DISABLE_MSVC(4355) /* 'this' : used in base member initializer list */
QT_WARNING_DISABLE_MSVC(4710) /* function not inlined */
QT_WARNING_DISABLE_MSVC(4530) /* C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc */
#  elif defined(Q_CC_BOR)
#    pragma option -w-inl
#    pragma option -w-aus
#    pragma warn -inl
#    pragma warn -pia
#    pragma warn -ccc
#    pragma warn -rch
#    pragma warn -sig
#  endif
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

// like std::to_underlying
template <typename Enum>
constexpr std::underlying_type_t<Enum> qToUnderlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

#ifdef __cpp_conditional_explicit
#define Q_IMPLICIT explicit(false)
#else
#define Q_IMPLICIT
#endif

#ifdef __cpp_constinit
# if defined(Q_CC_MSVC) && !defined(Q_CC_CLANG)
   // https://developercommunity.visualstudio.com/t/C:-constinit-for-an-optional-fails-if-/1406069
#  define Q_CONSTINIT
# else
#  define Q_CONSTINIT constinit
# endif
#elif defined(__has_cpp_attribute) && __has_cpp_attribute(clang::require_constant_initialization)
# define Q_CONSTINIT [[clang::require_constant_initialization]]
#elif defined(Q_CC_GNU_ONLY) && Q_CC_GNU >= 1000
# define Q_CONSTINIT __constinit
#else
# define Q_CONSTINIT
#endif

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
#include <QtCore/qforeach.h>
#include <QtCore/qglobalstatic.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qoverload.h>
#include <QtCore/qtdeprecationmarkers.h>
#include <QtCore/qtranslation.h>
#include <QtCore/qversiontagging.h>

#endif /* __cplusplus */
#endif /* !__ASSEMBLER__ */

#endif /* QGLOBAL_H */
