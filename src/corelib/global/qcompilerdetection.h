/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOMPILERDETECTION_H
#define QCOMPILERDETECTION_H

/*
   The compiler, must be one of: (Q_CC_x)

     SYM      - Digital Mars C/C++ (used to be Symantec C++)
     MSVC     - Microsoft Visual C/C++, Intel C++ for Windows
     BOR      - Borland/Turbo C++
     WAT      - Watcom C++
     GNU      - GNU C++
     COMEAU   - Comeau C++
     EDG      - Edison Design Group C++
     OC       - CenterLine C++
     SUN      - Forte Developer, or Sun Studio C++
     MIPS     - MIPSpro C++
     DEC      - DEC C++
     HPACC    - HP aC++
     USLC     - SCO OUDK and UDK
     CDS      - Reliant C++
     KAI      - KAI C++
     INTEL    - Intel C++ for Linux, Intel C++ for Windows
     HIGHC    - MetaWare High C/C++
     PGI      - Portland Group C++
     GHS      - Green Hills Optimizing C++ Compilers
     RVCT     - ARM Realview Compiler Suite
     CLANG    - C++ front-end for the LLVM compiler


   Should be sorted most to least authoritative.
*/

/* Symantec C++ is now Digital Mars */
#if defined(__DMC__) || defined(__SC__)
#  define Q_CC_SYM
/* "explicit" semantics implemented in 8.1e but keyword recognized since 7.5 */
#  if defined(__SC__) && __SC__ < 0x750
#    error "Compiler not supported"
#  endif
#  define Q_NO_USING_KEYWORD

#elif defined(_MSC_VER)
#  define Q_CC_MSVC
#  define Q_CC_MSVC_NET
#  define Q_OUTOFLINE_TEMPLATE inline
#  define Q_NO_TEMPLATE_FRIENDS
#  define Q_ALIGNOF(type) __alignof(type)
#  define Q_DECL_ALIGN(n) __declspec(align(n))
#  define Q_ASSUME(expr) __assume(expr)
#  define Q_UNREACHABLE() __assume(0)
#  define Q_NORETURN __declspec(noreturn)
/* Intel C++ disguising as Visual C++: the `using' keyword avoids warnings */
#  if defined(__INTEL_COMPILER)
#    define Q_CC_INTEL
#  endif
/* MSVC does not support SSE/MMX on x64 */
#  if (defined(Q_CC_MSVC) && defined(_M_X64))
#    undef QT_HAVE_SSE
#    undef QT_HAVE_MMX
#    undef QT_HAVE_3DNOW
#  endif

#  if defined(Q_CC_MSVC) && _MSC_VER >= 1400
#    define Q_COMPILER_VARIADIC_MACROS
#  endif

#if defined(Q_CC_MSVC) && _MSC_VER >= 1600
#      define Q_COMPILER_RVALUE_REFS
#      define Q_COMPILER_AUTO_TYPE
#      define Q_COMPILER_LAMBDA
#      define Q_COMPILER_DECLTYPE
#      define Q_COMPILER_STATIC_ASSERT
//  MSCV has std::initilizer_list, but do not support the braces initialization
//#      define Q_COMPILER_INITIALIZER_LISTS
#  endif


#elif defined(__BORLANDC__) || defined(__TURBOC__)
#  define Q_CC_BOR
#  define Q_INLINE_TEMPLATE
#  if __BORLANDC__ < 0x502
#    error "Compiler not supported"
#  endif
#  define Q_NO_USING_KEYWORD

#elif defined(__WATCOMC__)
#  define Q_CC_WAT

/* ARM Realview Compiler Suite
   RVCT compiler also defines __EDG__ and __GNUC__ (if --gnu flag is given),
   so check for it before that */
#elif defined(__ARMCC__) || defined(__CC_ARM)
#  define Q_CC_RVCT
#  if __TARGET_ARCH_ARM >= 6
#    define QT_HAVE_ARMV6
#  endif
/* work-around for missing compiler intrinsics */
#  define __is_empty(X) false
#  define __is_pod(X) false
#elif defined(__GNUC__)
#  define Q_CC_GNU
#  define Q_C_CALLBACKS
#  if defined(__MINGW32__)
#    define Q_CC_MINGW
#  endif
#  if defined(__INTEL_COMPILER)
/* Intel C++ also masquerades as GCC */
#    define Q_CC_INTEL
#    define Q_ASSUME(expr)  __assume(expr)
#    define Q_UNREACHABLE() __assume(0)
#  elif defined(__clang__)
/* Clang also masquerades as GCC */
#    define Q_CC_CLANG
#    define Q_ASSUME(expr)  if (expr){} else __builtin_unreachable()
#    define Q_UNREACHABLE() __builtin_unreachable()
#  else
/* Plain GCC */
#    define Q_ASSUME(expr)  if (expr){} else __builtin_unreachable()
#    define Q_UNREACHABLE() __builtin_unreachable()
#  endif

#  define Q_ALIGNOF(type)   __alignof__(type)
#  define Q_TYPEOF(expr)    __typeof__(expr)
#  define Q_DECL_ALIGN(n)   __attribute__((__aligned__(n)))
#  define Q_LIKELY(expr)    __builtin_expect(!!(expr), true)
#  define Q_UNLIKELY(expr)  __builtin_expect(!!(expr), false)
#  define Q_NORETURN        __attribute__((__noreturn__))
#  if !defined(QT_MOC_CPP)
#    define Q_PACKED __attribute__ ((__packed__))
#    define Q_NO_PACKED_REFERENCE
#    ifndef __ARM_EABI__
#      define QT_NO_ARM_EABI
#    endif
#  endif
#  if (__GNUC__ * 100 + __GNUC_MINOR__) >= 403 && !defined(Q_CC_CLANG)
#      define Q_ALLOC_SIZE(x) __attribute__((alloc_size(x)))
#  endif

/* IBM compiler versions are a bit messy. There are actually two products:
   the C product, and the C++ product. The C++ compiler is always packaged
   with the latest version of the C compiler. Version numbers do not always
   match. This little table (I'm not sure it's accurate) should be helpful:

   C++ product                C product

   C Set 3.1                  C Compiler 3.0
   ...                        ...
   C++ Compiler 3.6.6         C Compiler 4.3
   ...                        ...
   Visual Age C++ 4.0         ...
   ...                        ...
   Visual Age C++ 5.0         C Compiler 5.0
   ...                        ...
   Visual Age C++ 6.0         C Compiler 6.0

   Now:
   __xlC__    is the version of the C compiler in hexadecimal notation
              is only an approximation of the C++ compiler version
   __IBMCPP__ is the version of the C++ compiler in decimal notation
              but it is not defined on older compilers like C Set 3.1 */
#elif defined(__xlC__)
#  define Q_CC_XLC
#  define Q_FULL_TEMPLATE_INSTANTIATION
#  if __xlC__ < 0x400
#    error "Compiler not supported"
#  elif __xlC__ >= 0x0600
#    define Q_ALIGNOF(type)     __alignof__(type)
#    define Q_TYPEOF(expr)      __typeof__(expr)
#    define Q_DECL_ALIGN(n)     __attribute__((__aligned__(n)))
#    define Q_PACKED            __attribute__((__packed__))
#  endif

/* Older versions of DEC C++ do not define __EDG__ or __EDG - observed
   on DEC C++ V5.5-004. New versions do define  __EDG__ - observed on
   Compaq C++ V6.3-002.
   This compiler is different enough from other EDG compilers to handle
   it separately anyway. */
#elif defined(__DECCXX) || defined(__DECC)
#  define Q_CC_DEC
/* Compaq C++ V6 compilers are EDG-based but I'm not sure about older
   DEC C++ V5 compilers. */
#  if defined(__EDG__)
#    define Q_CC_EDG
#  endif
/* Compaq have disabled EDG's _BOOL macro and use _BOOL_EXISTS instead
   - observed on Compaq C++ V6.3-002.
   In any case versions prior to Compaq C++ V6.0-005 do not have bool. */
#  if !defined(_BOOL_EXISTS)
#    error "Compiler not supported"
#  endif
/* Spurious (?) error messages observed on Compaq C++ V6.5-014. */
#  define Q_NO_USING_KEYWORD
/* Apply to all versions prior to Compaq C++ V6.0-000 - observed on
   DEC C++ V5.5-004. */
#  if __DECCXX_VER < 60060000
#    define Q_BROKEN_TEMPLATE_SPECIALIZATION
#  endif
/* avoid undefined symbol problems with out-of-line template members */
#  define Q_OUTOFLINE_TEMPLATE inline

/* The Portland Group C++ compiler is based on EDG and does define __EDG__
   but the C compiler does not */
#elif defined(__PGI)
#  define Q_CC_PGI
#  if defined(__EDG__)
#    define Q_CC_EDG
#  endif

/* Compilers with EDG front end are similar. To detect them we test:
   __EDG documented by SGI, observed on MIPSpro 7.3.1.1 and KAI C++ 4.0b
   __EDG__ documented in EDG online docs, observed on Compaq C++ V6.3-002
   and PGI C++ 5.2-4 */
#elif !defined(Q_OS_HPUX) && (defined(__EDG) || defined(__EDG__))
#  define Q_CC_EDG
/* From the EDG documentation (does not seem to apply to Compaq C++):
   _BOOL
        Defined in C++ mode when bool is a keyword. The name of this
        predefined macro is specified by a configuration flag. _BOOL
        is the default.
   __BOOL_DEFINED
        Defined in Microsoft C++ mode when bool is a keyword. */
#  if !defined(_BOOL) && !defined(__BOOL_DEFINED)
#    error "Compiler not supported"
#  endif

/* The Comeau compiler is based on EDG and does define __EDG__ */
#  if defined(__COMO__)
#    define Q_CC_COMEAU
#    define Q_C_CALLBACKS

/* The `using' keyword was introduced to avoid KAI C++ warnings
   but it's now causing KAI C++ errors instead. The standard is
   unclear about the use of this keyword, and in practice every
   compiler is using its own set of rules. Forget it. */
#  elif defined(__KCC)
#    define Q_CC_KAI
#    define Q_NO_USING_KEYWORD

/* Using the `using' keyword avoids Intel C++ for Linux warnings */
#  elif defined(__INTEL_COMPILER)
#    define Q_CC_INTEL

/* Uses CFront, make sure to read the manual how to tweak templates. */
#  elif defined(__ghs)
#    define Q_CC_GHS

#  elif defined(__DCC__)
#    define Q_CC_DIAB
#    if !defined(__bool)
#      error "Compiler not supported"
#    endif

/* The UnixWare 7 UDK compiler is based on EDG and does define __EDG__ */
#  elif defined(__USLC__) && defined(__SCO_VERSION__)
#    define Q_CC_USLC
/* The latest UDK 7.1.1b does not need this, but previous versions do */
#    if !defined(__SCO_VERSION__) || (__SCO_VERSION__ < 302200010)
#      define Q_OUTOFLINE_TEMPLATE inline
#    endif
#    define Q_NO_USING_KEYWORD /* ### check "using" status */

/* Never tested! */
#  elif defined(CENTERLINE_CLPP) || defined(OBJECTCENTER)
#    define Q_CC_OC
#    define Q_NO_USING_KEYWORD

/* CDS++ defines __EDG__ although this is not documented in the Reliant
   documentation. It also follows conventions like _BOOL and this documented */
#  elif defined(sinix)
#    define Q_CC_CDS
#    define Q_NO_USING_KEYWORD

/* The MIPSpro compiler defines __EDG */
#  elif defined(__sgi)
#    define Q_CC_MIPS
#    define Q_NO_USING_KEYWORD /* ### check "using" status */
#    define Q_NO_TEMPLATE_FRIENDS
#    if defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 740)
#      define Q_OUTOFLINE_TEMPLATE inline
#      pragma set woff 3624,3625,3649 /* turn off some harmless warnings */
#    endif
#  endif

/* VxWorks' DIAB toolchain has an additional EDG type C++ compiler
   (see __DCC__ above). This one is for C mode files (__EDG is not defined) */
#elif defined(_DIAB_TOOL)
#  define Q_CC_DIAB

/* Never tested! */
#elif defined(__HIGHC__)
#  define Q_CC_HIGHC

#elif defined(__SUNPRO_CC) || defined(__SUNPRO_C)
#  define Q_CC_SUN
/* 5.0 compiler or better
    'bool' is enabled by default but can be disabled using -features=nobool
    in which case _BOOL is not defined
        this is the default in 4.2 compatibility mode triggered by -compat=4 */
#  if __SUNPRO_CC >= 0x500
#    define QT_NO_TEMPLATE_TEMPLATE_PARAMETERS
   /* see http://developers.sun.com/sunstudio/support/Ccompare.html */
#    if __SUNPRO_CC >= 0x590
#      define Q_ALIGNOF(type)   __alignof__(type)
#      define Q_TYPEOF(expr)    __typeof__(expr)
#      define Q_DECL_ALIGN(n)   __attribute__((__aligned__(n)))
#    endif
#    if __SUNPRO_CC >= 0x550
#      define Q_DECL_EXPORT     __global
#    endif
#    if __SUNPRO_CC < 0x5a0
#      define Q_NO_TEMPLATE_FRIENDS
#    endif
#    if !defined(_BOOL)
#      error "Compiler not supported"
#    endif
#    if defined(__SUNPRO_CC_COMPAT) && (__SUNPRO_CC_COMPAT <= 4)
#      define Q_NO_USING_KEYWORD
#    endif
#    define Q_C_CALLBACKS
/* 4.2 compiler or older */
#  else
#    error "Compiler not supported"
#  endif

/* CDS++ does not seem to define __EDG__ or __EDG according to Reliant
   documentation but nevertheless uses EDG conventions like _BOOL */
#elif defined(sinix)
#  define Q_CC_EDG
#  define Q_CC_CDS
#  if !defined(_BOOL)
#    error "Compiler not supported"
#  endif
#  define Q_BROKEN_TEMPLATE_SPECIALIZATION

#elif defined(Q_OS_HPUX)
/* __HP_aCC was not defined in first aCC releases */
#  if defined(__HP_aCC) || __cplusplus >= 199707L
#    define Q_NO_TEMPLATE_FRIENDS
#    define Q_CC_HPACC
#    if __HP_aCC-0 < 060000
#      define QT_NO_TEMPLATE_TEMPLATE_PARAMETERS
#      define Q_DECL_EXPORT     __declspec(dllexport)
#      define Q_DECL_IMPORT     __declspec(dllimport)
#    endif
#    if __HP_aCC-0 >= 061200
#      define Q_DECL_ALIGN(n) __attribute__((aligned(n)))
#    endif
#    if __HP_aCC-0 >= 062000
#      define Q_DECL_EXPORT     __attribute__((visibility("default")))
#      define Q_DECL_HIDDEN     __attribute__((visibility("hidden")))
#      define Q_DECL_IMPORT     Q_DECL_EXPORT
#    endif
#  else
#    error "Compiler not supported"
#  endif
#  define Q_NO_USING_KEYWORD /* ### check "using" status */

#else
#  error "Qt has not been tested with this compiler - see http://www.qt-project.org/"
#endif

#ifndef Q_NORETURN
# define Q_NORETURN
#endif

/*
 * C++11 support
 *
 *  Paper           Macro
 *  N2341           Q_COMPILER_ALIGNAS
 *  N2341           Q_COMPILER_ALIGNOF
 *  N2427           Q_COMPILER_ATOMICS
 *  N2761           Q_COMPILER_ATTRIBUTES
 *  N2541           Q_COMPILER_AUTO_FUNCTION
 *  N1984 N2546     Q_COMPILER_AUTO_TYPE
 *  N2437           Q_COMPILER_CLASS_ENUM
 *  N2235 N3276     Q_COMPILER_DECLTYPE
 *  N2346           Q_COMPILER_DEFAULT_DELETE_MEMBERS
 *  N1986           Q_COMPILER_DELEGATING_CONSTRUCTORS
 *  N3206 N3272     Q_COMPILER_EXPLICIT_OVERRIDES   (v0.9 and above only)
 *  N1987           Q_COMPILER_EXTERN_TEMPLATES
 *  N2540           Q_COMPILER_INHERITING_CONSTRUCTORS
 *  N2672           Q_COMPILER_INITIALIZER_LISTS
 *  N2658 N2927     Q_COMPILER_LAMBDA   (v1.0 and above only)
 *  N2756           Q_COMPILER_NONSTATIC_MEMBER_INIT
 *  N2431           Q_COMPILER_NULLPTR
 *  N2930           Q_COMPILER_RANGE_FOR
 *  N2442           Q_COMPILER_RAW_STRINGS
 *  N2439           Q_COMPILER_REF_QUALIFIERS
 *  N2118 N2844 N3053 Q_COMPILER_RVALUE_REFS   (Note: GCC 4.3 implements only the oldest)
 *  N1720           Q_COMPILER_STATIC_ASSERT
 *  N2258           Q_COMPILER_TEMPLATE_ALIAS
 *  N2659           Q_COMPILER_THREAD_LOCAL
 *  N2756           Q_COMPILER_UDL
 *  N2442           Q_COMPILER_UNICODE_STRINGS
 *  N2544           Q_COMPILER_UNRESTRICTED_UNIONS
 *  N1653           Q_COMPILER_VARIADIC_MACROS
 *  N2242 N2555     Q_COMPILER_VARIADIC_TEMPLATES
 */

#ifdef Q_CC_INTEL
#  if __INTEL_COMPILER < 1200
#    define Q_NO_TEMPLATE_FRIENDS
#  endif
#  if defined(_CHAR16T) || __cplusplus >= 201103L
#    define Q_COMPILER_VARIADIC_MACROS
#    if __INTEL_COMPILER >= 1200
#      define Q_COMPILER_AUTO_TYPE
#      define Q_COMPILER_CLASS_ENUM
#      define Q_COMPILER_DECLTYPE
#      define Q_COMPILER_DEFAULT_DELETE_MEMBERS
#      define Q_COMPILER_EXTERN_TEMPLATES
#      define Q_COMPILER_LAMBDA
#      define Q_COMPILER_RVALUE_REFS
#      define Q_COMPILER_STATIC_ASSERT
#      define Q_COMPILER_THREAD_LOCAL
#      define Q_COMPILER_VARIADIC_MACROS
#    endif
#    if __INTEL_COMPILER >= 1210
#      define Q_COMPILER_ATTRIBUTES
#      define Q_COMPILER_AUTO_FUNCTION
#      define Q_COMPILER_NULLPTR
#      define Q_COMPILER_TEMPLATE_ALIAS
#      define Q_COMPILER_UNICODE_STRINGS
#      define Q_COMPILER_VARIADIC_TEMPLATES
#    endif
#  endif
#endif

#ifdef Q_CC_CLANG
/* General C++ features */
#  if !__has_feature(cxx_exceptions)
#    define QT_NO_EXCEPTIONS
#  endif
#  if !__has_feature(cxx_rtti)
#    define QT_NO_RTTI
#  endif
/* C++11 features, see http://clang.llvm.org/cxx_status.html */
#  if __cplusplus >= 201103L || __GXX_EXPERIMENTAL_CXX0X__
#    if ((__clang_major__ * 100) + __clang_minor__) >= 209 /* since clang 2.9 */
#      define Q_COMPILER_AUTO_TYPE
#      define Q_COMPILER_DECLTYPE
#      define Q_COMPILER_EXTERN_TEMPLATES
#      define Q_COMPILER_RVALUE_REFS
#      define Q_COMPILER_STATIC_ASSERT
#      define Q_COMPILER_VARIADIC_MACROS
#      define Q_COMPILER_VARIADIC_TEMPLATES
#    endif
#    if ((__clang_major__ * 100) + __clang_minor__) >= 300 /* since clang 3.0 */
#      define Q_COMPILER_CLASS_ENUM
        /* defaulted members in 3.0, deleted members in 2.9 */
#      define Q_COMPILER_DEFAULT_DELETE_MEMBERS
#      define Q_COMPILER_DELEGATING_CONSTRUCTORS
#      define Q_COMPILER_EXPLICIT_OVERRIDES
#      define Q_COMPILER_NULLPTR
#      define Q_COMPILER_RANGE_FOR
#      define Q_COMPILER_UNICODE_STRINGS
#    endif
        /* not implemented in clang yet */
#    if __has_feature(cxx_constexpr)
#      define Q_COMPILER_CONSTEXPR
#    endif
#    if __has_feature(cxx_lambdas)
#      define Q_COMPILER_LAMBDA
#    endif
#    if __has_feature(cxx_generalized_initializers)
#      define Q_COMPILER_INITIALIZER_LISTS
#    endif
#    if __has_feature(cxx_unrestricted_unions)
#      define Q_COMPILER_UNRESTRICTED_UNIONS
#    endif
#    if 0
#      define Q_COMPILER_ATOMICS
#    endif
#  endif
#endif // Q_CC_CLANG

#if defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && !defined(Q_CC_CLANG)
#  if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 403
       /* C++11 features supported in GCC 4.3: */
#      define Q_COMPILER_DECLTYPE
#      define Q_COMPILER_RVALUE_REFS
#      define Q_COMPILER_STATIC_ASSERT
#      define Q_COMPILER_VARIADIC_MACROS
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 404
       /* C++11 features supported in GCC 4.4: */
#      define Q_COMPILER_ATOMICS
#      define Q_COMPILER_AUTO_FUNCTION
#      define Q_COMPILER_AUTO_TYPE
#      define Q_COMPILER_CLASS_ENUM
#      define Q_COMPILER_DEFAULT_DELETE_MEMBERS
#      define Q_COMPILER_EXTERN_TEMPLATES
#      define Q_COMPILER_INITIALIZER_LISTS
#      define Q_COMPILER_UNICODE_STRINGS
#      define Q_COMPILER_VARIADIC_TEMPLATES
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
       /* C++11 features supported in GCC 4.5: */
#      define Q_COMPILER_LAMBDA
#      define Q_COMPILER_RAW_STRINGS
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 406
       /* C++11 features supported in GCC 4.6: */
#      define Q_COMPILER_CONSTEXPR
#      define Q_COMPILER_NULLPTR
#      define Q_COMPILER_UNRESTRICTED_UNIONS
#      define Q_COMPILER_RANGE_FOR
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 407
       /* C++11 features supported in GCC 4.7: */
#      define Q_COMPILER_NONSTATIC_MEMBER_INIT
#      define Q_COMPILER_DELEGATING_CONSTRUCTORS
#      define Q_COMPILER_EXPLICIT_OVERRIDES
#      define Q_COMPILER_TEMPLATE_ALIAS
#      define Q_COMPILER_UDL
#    endif
#  endif
#endif

#if defined(Q_CC_MSVC) && _MSC_VER >= 1600 && !defined(Q_CC_INTEL)
#      define Q_COMPILER_AUTO_TYPE
#      define Q_COMPILER_LAMBDA
#      define Q_COMPILER_DECLTYPE
#      define Q_COMPILER_RVALUE_REFS
#      define Q_COMPILER_STATIC_ASSERT
//  MSVC has std::initilizer_list, but does not support the braces initialization
//#      define Q_COMPILER_INITIALIZER_LISTS
#endif

#ifndef Q_COMPILER_MANGLES_RETURN_TYPE
#  if defined(Q_CC_MSVC)
#    define Q_COMPILER_MANGLES_RETURN_TYPE
#  endif
#endif

#endif // QCOMPILERDETECTION_H
