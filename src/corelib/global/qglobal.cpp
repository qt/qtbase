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

/*! \typedef QFunctionPointer
    \relates <QFunctionPointer>

    This is a typedef for \c{void (*)()}, a pointer to a function that takes
    no arguments and returns void.
*/

/*****************************************************************************
  System detection routines
 *****************************************************************************/

/*!
    \macro Q_OS_DARWIN
    \relates <QtGlobal>

    Defined on Darwin-based operating systems such as \macos, iOS, watchOS, and tvOS.
*/

/*!
    \macro Q_OS_MAC
    \relates <QtGlobal>

    Deprecated synonym for \c Q_OS_DARWIN. Do not use.
*/

/*!
    \macro Q_OS_OSX
    \relates <QtGlobal>

    Deprecated synonym for \c Q_OS_MACOS. Do not use.
*/

/*!
    \macro Q_OS_MACOS
    \relates <QtGlobal>

    Defined on \macos.
*/

/*!
    \macro Q_OS_IOS
    \relates <QtGlobal>

    Defined on iOS.
*/

/*!
    \macro Q_OS_WATCHOS
    \relates <QtGlobal>

    Defined on watchOS.
*/

/*!
    \macro Q_OS_TVOS
    \relates <QtGlobal>

    Defined on tvOS.
*/

/*!
    \macro Q_OS_WIN
    \relates <QtGlobal>

    Defined on all supported versions of Windows. That is, if
    \l Q_OS_WIN32 or \l Q_OS_WIN64 is defined.
*/

/*!
    \macro Q_OS_WINDOWS
    \relates <QtGlobal>

    This is a synonym for Q_OS_WIN.
*/

/*!
    \macro Q_OS_WIN32
    \relates <QtGlobal>

    Defined on 32-bit and 64-bit versions of Windows.
*/

/*!
    \macro Q_OS_WIN64
    \relates <QtGlobal>

    Defined on 64-bit versions of Windows.
*/

/*!
    \macro Q_OS_CYGWIN
    \relates <QtGlobal>

    Defined on Cygwin.
*/

/*!
    \macro Q_OS_SOLARIS
    \relates <QtGlobal>

    Defined on Sun Solaris.
*/

/*!
    \macro Q_OS_HPUX
    \relates <QtGlobal>

    Defined on HP-UX.
*/

/*!
    \macro Q_OS_LINUX
    \relates <QtGlobal>

    Defined on Linux.
*/

/*!
    \macro Q_OS_ANDROID
    \relates <QtGlobal>

    Defined on Android.
*/

/*!
    \macro Q_OS_FREEBSD
    \relates <QtGlobal>

    Defined on FreeBSD.
*/

/*!
    \macro Q_OS_NETBSD
    \relates <QtGlobal>

    Defined on NetBSD.
*/

/*!
    \macro Q_OS_OPENBSD
    \relates <QtGlobal>

    Defined on OpenBSD.
*/

/*!
    \macro Q_OS_AIX
    \relates <QtGlobal>

    Defined on AIX.
*/

/*!
    \macro Q_OS_HURD
    \relates <QtGlobal>

    Defined on GNU Hurd.
*/

/*!
    \macro Q_OS_QNX
    \relates <QtGlobal>

    Defined on QNX Neutrino.
*/

/*!
    \macro Q_OS_LYNX
    \relates <QtGlobal>

    Defined on LynxOS.
*/

/*!
    \macro Q_OS_BSD4
    \relates <QtGlobal>

    Defined on Any BSD 4.4 system.
*/

/*!
    \macro Q_OS_UNIX
    \relates <QtGlobal>

    Defined on Any UNIX BSD/SYSV system.
*/

/*!
    \macro Q_OS_WASM
    \relates <QtGlobal>

    Defined on Web Assembly.
*/

/*!
    \macro Q_CC_SYM
    \relates <QtGlobal>

    Defined if the application is compiled using Digital Mars C/C++
    (used to be Symantec C++).
*/

/*!
    \macro Q_CC_MSVC
    \relates <QtGlobal>

    Defined if the application is compiled using Microsoft Visual
    C/C++, Intel C++ for Windows.
*/

/*!
    \macro Q_CC_CLANG
    \relates <QtGlobal>

    Defined if the application is compiled using Clang.
*/

/*!
    \macro Q_CC_BOR
    \relates <QtGlobal>

    Defined if the application is compiled using Borland/Turbo C++.
*/

/*!
    \macro Q_CC_WAT
    \relates <QtGlobal>

    Defined if the application is compiled using Watcom C++.
*/

/*!
    \macro Q_CC_GNU
    \relates <QtGlobal>

    Defined if the application is compiled using GNU C++.
*/

/*!
    \macro Q_CC_COMEAU
    \relates <QtGlobal>

    Defined if the application is compiled using Comeau C++.
*/

/*!
    \macro Q_CC_EDG
    \relates <QtGlobal>

    Defined if the application is compiled using Edison Design Group
    C++.
*/

/*!
    \macro Q_CC_OC
    \relates <QtGlobal>

    Defined if the application is compiled using CenterLine C++.
*/

/*!
    \macro Q_CC_SUN
    \relates <QtGlobal>

    Defined if the application is compiled using Forte Developer, or
    Sun Studio C++.
*/

/*!
    \macro Q_CC_MIPS
    \relates <QtGlobal>

    Defined if the application is compiled using MIPSpro C++.
*/

/*!
    \macro Q_CC_DEC
    \relates <QtGlobal>

    Defined if the application is compiled using DEC C++.
*/

/*!
    \macro Q_CC_HPACC
    \relates <QtGlobal>

    Defined if the application is compiled using HP aC++.
*/

/*!
    \macro Q_CC_USLC
    \relates <QtGlobal>

    Defined if the application is compiled using SCO OUDK and UDK.
*/

/*!
    \macro Q_CC_CDS
    \relates <QtGlobal>

    Defined if the application is compiled using Reliant C++.
*/

/*!
    \macro Q_CC_KAI
    \relates <QtGlobal>

    Defined if the application is compiled using KAI C++.
*/

/*!
    \macro Q_CC_INTEL
    \relates <QtGlobal>
    \obsolete

    This macro used to be defined if the application was compiled with the old
    Intel C++ compiler for Linux, macOS or Windows. The new oneAPI C++ compiler
    is just a build of Clang and therefore does not define this macro.

    \sa Q_CC_CLANG
*/

/*!
    \macro Q_CC_HIGHC
    \relates <QtGlobal>

    Defined if the application is compiled using MetaWare High C/C++.
*/

/*!
    \macro Q_CC_PGI
    \relates <QtGlobal>

    Defined if the application is compiled using Portland Group C++.
*/

/*!
    \macro Q_CC_GHS
    \relates <QtGlobal>

    Defined if the application is compiled using Green Hills
    Optimizing C++ Compilers.
*/

/*!
    \macro Q_PROCESSOR_ALPHA
    \relates <QtGlobal>

    Defined if the application is compiled for Alpha processors.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_ARM
    \relates <QtGlobal>

    Defined if the application is compiled for ARM processors. Qt currently
    supports three optional ARM revisions: \l Q_PROCESSOR_ARM_V5, \l
    Q_PROCESSOR_ARM_V6, and \l Q_PROCESSOR_ARM_V7.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_ARM_V5
    \relates <QtGlobal>

    Defined if the application is compiled for ARMv5 processors. The \l
    Q_PROCESSOR_ARM macro is also defined when Q_PROCESSOR_ARM_V5 is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_ARM_V6
    \relates <QtGlobal>

    Defined if the application is compiled for ARMv6 processors. The \l
    Q_PROCESSOR_ARM and \l Q_PROCESSOR_ARM_V5 macros are also defined when
    Q_PROCESSOR_ARM_V6 is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_ARM_V7
    \relates <QtGlobal>

    Defined if the application is compiled for ARMv7 processors. The \l
    Q_PROCESSOR_ARM, \l Q_PROCESSOR_ARM_V5, and \l Q_PROCESSOR_ARM_V6 macros
    are also defined when Q_PROCESSOR_ARM_V7 is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_AVR32
    \relates <QtGlobal>

    Defined if the application is compiled for AVR32 processors.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_BLACKFIN
    \relates <QtGlobal>

    Defined if the application is compiled for Blackfin processors.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_IA64
    \relates <QtGlobal>

    Defined if the application is compiled for IA-64 processors. This includes
    all Itanium and Itanium 2 processors.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_MIPS
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS processors. Qt currently
    supports seven MIPS revisions: \l Q_PROCESSOR_MIPS_I, \l
    Q_PROCESSOR_MIPS_II, \l Q_PROCESSOR_MIPS_III, \l Q_PROCESSOR_MIPS_IV, \l
    Q_PROCESSOR_MIPS_V, \l Q_PROCESSOR_MIPS_32, and \l Q_PROCESSOR_MIPS_64.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_MIPS_I
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-I processors. The \l
    Q_PROCESSOR_MIPS macro is also defined when Q_PROCESSOR_MIPS_I is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_MIPS_II
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-II processors. The \l
    Q_PROCESSOR_MIPS and \l Q_PROCESSOR_MIPS_I macros are also defined when
    Q_PROCESSOR_MIPS_II is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_MIPS_32
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS32 processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, and \l Q_PROCESSOR_MIPS_II macros
    are also defined when Q_PROCESSOR_MIPS_32 is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_MIPS_III
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-III processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, and \l Q_PROCESSOR_MIPS_II macros
    are also defined when Q_PROCESSOR_MIPS_III is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_MIPS_IV
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-IV processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, \l Q_PROCESSOR_MIPS_II, and \l
    Q_PROCESSOR_MIPS_III macros are also defined when Q_PROCESSOR_MIPS_IV is
    defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_MIPS_V
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS-V processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, \l Q_PROCESSOR_MIPS_II, \l
    Q_PROCESSOR_MIPS_III, and \l Q_PROCESSOR_MIPS_IV macros are also defined
    when Q_PROCESSOR_MIPS_V is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_MIPS_64
    \relates <QtGlobal>

    Defined if the application is compiled for MIPS64 processors. The \l
    Q_PROCESSOR_MIPS, \l Q_PROCESSOR_MIPS_I, \l Q_PROCESSOR_MIPS_II, \l
    Q_PROCESSOR_MIPS_III, \l Q_PROCESSOR_MIPS_IV, and \l Q_PROCESSOR_MIPS_V
    macros are also defined when Q_PROCESSOR_MIPS_64 is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_POWER
    \relates <QtGlobal>

    Defined if the application is compiled for POWER processors. Qt currently
    supports two Power variants: \l Q_PROCESSOR_POWER_32 and \l
    Q_PROCESSOR_POWER_64.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_POWER_32
    \relates <QtGlobal>

    Defined if the application is compiled for 32-bit Power processors. The \l
    Q_PROCESSOR_POWER macro is also defined when Q_PROCESSOR_POWER_32 is
    defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_POWER_64
    \relates <QtGlobal>

    Defined if the application is compiled for 64-bit Power processors. The \l
    Q_PROCESSOR_POWER macro is also defined when Q_PROCESSOR_POWER_64 is
    defined.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_RISCV
    \relates <QtGlobal>
    \since 5.13

    Defined if the application is compiled for RISC-V processors. Qt currently
    supports two RISC-V variants: \l Q_PROCESSOR_RISCV_32 and \l
    Q_PROCESSOR_RISCV_64.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_RISCV_32
    \relates <QtGlobal>
    \since 5.13

    Defined if the application is compiled for 32-bit RISC-V processors. The \l
    Q_PROCESSOR_RISCV macro is also defined when Q_PROCESSOR_RISCV_32 is
    defined.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_RISCV_64
    \relates <QtGlobal>
    \since 5.13

    Defined if the application is compiled for 64-bit RISC-V processors. The \l
    Q_PROCESSOR_RISCV macro is also defined when Q_PROCESSOR_RISCV_64 is
    defined.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_S390
    \relates <QtGlobal>

    Defined if the application is compiled for S/390 processors. Qt supports
    one optional variant of S/390: Q_PROCESSOR_S390_X.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_S390_X
    \relates <QtGlobal>

    Defined if the application is compiled for S/390x processors. The \l
    Q_PROCESSOR_S390 macro is also defined when Q_PROCESSOR_S390_X is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_SH
    \relates <QtGlobal>

    Defined if the application is compiled for SuperH processors. Qt currently
    supports one SuperH revision: \l Q_PROCESSOR_SH_4A.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_SH_4A
    \relates <QtGlobal>

    Defined if the application is compiled for SuperH 4A processors. The \l
    Q_PROCESSOR_SH macro is also defined when Q_PROCESSOR_SH_4A is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_SPARC
    \relates <QtGlobal>

    Defined if the application is compiled for SPARC processors. Qt currently
    supports one optional SPARC revision: \l Q_PROCESSOR_SPARC_V9.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_SPARC_V9
    \relates <QtGlobal>

    Defined if the application is compiled for SPARC V9 processors. The \l
    Q_PROCESSOR_SPARC macro is also defined when Q_PROCESSOR_SPARC_V9 is
    defined.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro Q_PROCESSOR_X86
    \relates <QtGlobal>

    Defined if the application is compiled for x86 processors. Qt currently
    supports two x86 variants: \l Q_PROCESSOR_X86_32 and \l Q_PROCESSOR_X86_64.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_X86_32
    \relates <QtGlobal>

    Defined if the application is compiled for 32-bit x86 processors. This
    includes all i386, i486, i586, and i686 processors. The \l Q_PROCESSOR_X86
    macro is also defined when Q_PROCESSOR_X86_32 is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/
/*!
    \macro Q_PROCESSOR_X86_64
    \relates <QtGlobal>

    Defined if the application is compiled for 64-bit x86 processors. This
    includes all AMD64, Intel 64, and other x86_64/x64 processors. The \l
    Q_PROCESSOR_X86 macro is also defined when Q_PROCESSOR_X86_64 is defined.

    \sa QSysInfo::buildCpuArchitecture()
*/

/*!
    \macro void Q_FALLTHROUGH()
    \relates <QtGlobal>
    \since 5.8

    Can be used in switch statements at the end of case block to tell the compiler
    and other developers that that the lack of a break statement is intentional.

    This is useful since a missing break statement is often a bug, and some
    compilers can be configured to emit warnings when one is not found.

    \sa Q_UNREACHABLE()
*/

/*!
    \macro const char* Q_FUNC_INFO()
    \relates <QtGlobal>

    Expands to a string that describe the function the macro resides in. How this string looks
    more specifically is compiler dependent. With GNU GCC it is typically the function signature,
    while with other compilers it might be the line and column number.

    Q_FUNC_INFO can be conveniently used with qDebug(). For example, this function:

    \snippet code/src_corelib_global_qglobal.cpp 22

    when instantiated with the integer type, will with the GCC compiler produce:

    \tt{const TInputType& myMin(const TInputType&, const TInputType&) [with TInputType = int] was called with value1: 3 value2: 4}

    If this macro is used outside a function, the behavior is undefined.
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
    \macro Q_LIKELY(expr)
    \relates <QtGlobal>
    \since 4.8

    \brief Hints to the compiler that the enclosed condition, \a expr, is
    likely to evaluate to \c true.

    Use of this macro can help the compiler to optimize the code.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qlikely

    \sa Q_UNLIKELY(), Q_ASSUME()
*/

/*!
    \macro Q_UNLIKELY(expr)
    \relates <QtGlobal>
    \since 4.8

    \brief Hints to the compiler that the enclosed condition, \a expr, is
    likely to evaluate to \c false.

    Use of this macro can help the compiler to optimize the code.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qunlikely

    \sa Q_LIKELY()
*/

/*!
    \macro Q_CONSTINIT
    \relates <QtGlobal>
    \since 6.4

    \brief Enforces constant initialization when supported by the compiler.

    If the compiler supports the C++20 \c{constinit} keyword, Clang's
    \c{[[clang::require_constant_initialization]]} or GCC's \c{__constinit},
    then this macro expands to the first one of these that is available,
    otherwise it expands to nothing.

    Variables marked as \c{constinit} cause a compile-error if their
    initialization would have to be performed at runtime.

    For constants, you can use \c{constexpr} since C++11, but \c{constexpr}
    makes variables \c{const}, too, whereas \c{constinit} ensures constant
    initialization, but doesn't make the variable \c{const}:

    \table
    \header \li Keyword       \li Added \li immutable \li constant-initialized
    \row    \li \c{const}     \li C++98 \li yes       \li not required
    \row    \li \c{constexpr} \li C++11 \li yes       \li required
    \row    \li \c{constinit} \li C++20 \li no        \li required
    \endtable
*/

/*!
    \macro QT_POINTER_SIZE
    \relates <QtGlobal>

    Expands to the size of a pointer in bytes (4 or 8). This is
    equivalent to \c sizeof(void *) but can be used in a preprocessor
    directive.
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
    \macro Q_BYTE_ORDER
    \relates <QtGlobal>

    This macro can be used to determine the byte order your system
    uses for storing data in memory. i.e., whether your system is
    little-endian or big-endian. It is set by Qt to one of the macros
    Q_LITTLE_ENDIAN or Q_BIG_ENDIAN. You normally won't need to worry
    about endian-ness, but you might, for example if you need to know
    which byte of an integer or UTF-16 character is stored in the
    lowest address. Endian-ness is important in networking, where
    computers with different values for Q_BYTE_ORDER must pass data
    back and forth.

    Use this macro as in the following examples.

    \snippet code/src_corelib_global_qglobal.cpp 40

    \sa Q_BIG_ENDIAN, Q_LITTLE_ENDIAN
*/

/*!
    \macro Q_LITTLE_ENDIAN
    \relates <QtGlobal>

    This macro represents a value you can compare to the macro
    Q_BYTE_ORDER to determine the endian-ness of your system.  In a
    little-endian system, the least significant byte is stored at the
    lowest address. The other bytes follow in increasing order of
    significance.

    \snippet code/src_corelib_global_qglobal.cpp 41

    \sa Q_BYTE_ORDER, Q_BIG_ENDIAN
*/

/*!
    \macro Q_BIG_ENDIAN
    \relates <QtGlobal>

    This macro represents a value you can compare to the macro
    Q_BYTE_ORDER to determine the endian-ness of your system.  In a
    big-endian system, the most significant byte is stored at the
    lowest address. The other bytes follow in decreasing order of
    significance.

    \snippet code/src_corelib_global_qglobal.cpp 42

    \sa Q_BYTE_ORDER, Q_LITTLE_ENDIAN
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
    \macro Q_DECL_EXPORT
    \relates <QtGlobal>

    This macro marks a symbol for shared library export (see
     \l{sharedlibrary.html}{Creating Shared Libraries}).

    \sa Q_DECL_IMPORT
*/

/*!
    \macro Q_DECL_IMPORT
    \relates <QtGlobal>

    This macro declares a symbol to be an import from a shared library (see
    \l{sharedlibrary.html}{Creating Shared Libraries}).

    \sa Q_DECL_EXPORT
*/

/*!
    \macro Q_DECL_CONSTEXPR
    \relates <QtGlobal>
    \deprecated [6.4] Use the \c constexpr keyword instead.

    This macro can be used to declare variable that should be constructed at compile-time,
    or an inline function that can be computed at compile-time.

    \sa Q_DECL_RELAXED_CONSTEXPR
*/

/*!
    \macro Q_DECL_RELAXED_CONSTEXPR
    \relates <QtGlobal>
    \deprecated [6.4] Use the \c constexpr keyword instead.

    This macro can be used to declare an inline function that can be computed
    at compile-time according to the relaxed rules from C++14.

    \sa Q_DECL_CONSTEXPR
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
    \macro Q_DECL_NOTHROW
    \relates <QtGlobal>
    \since 5.0
    \deprecated [6.4] Use the \c noexcept keyword instead.

    This macro marks a function as never throwing, under no
    circumstances. If the function does nevertheless throw, the
    behaviour is undefined.

    \sa Q_DECL_NOEXCEPT, Q_DECL_NOEXCEPT_EXPR()
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

/*!
    \macro Q_DECL_NOEXCEPT
    \relates <QtGlobal>
    \since 5.0
    \deprecated [6.4] Use the \c noexcept keyword instead.

    This macro marks a function as never throwing. If the function
    does nevertheless throw, the behaviour is defined:
    std::terminate() is called.


    \sa Q_DECL_NOTHROW, Q_DECL_NOEXCEPT_EXPR()
*/

/*!
    \macro Q_DECL_NOEXCEPT_EXPR(x)
    \relates <QtGlobal>
    \since 5.0
    \deprecated [6.4] Use the \c noexcept keyword instead.

    This macro marks a function as non-throwing if \a x is \c true. If
    the function does nevertheless throw, the behaviour is defined:
    std::terminate() is called.


    \sa Q_DECL_NOTHROW, Q_DECL_NOEXCEPT
*/

/*!
    \macro Q_DECL_OVERRIDE
    \since 5.0
    \deprecated
    \relates <QtGlobal>

    This macro can be used to declare an overriding virtual
    function. Use of this markup will allow the compiler to generate
    an error if the overriding virtual function does not in fact
    override anything.

    It expands to "override".

    The macro goes at the end of the function, usually after the
    \c{const}, if any:
    \snippet code/src_corelib_global_qglobal.cpp qdecloverride

    \sa Q_DECL_FINAL
*/

/*!
    \macro Q_DECL_FINAL
    \since 5.0
    \deprecated
    \relates <QtGlobal>

    This macro can be used to declare an overriding virtual or a class
    as "final", with Java semantics. Further-derived classes can then
    no longer override this virtual function, or inherit from this
    class, respectively.

    It expands to "final".

    The macro goes at the end of the function, usually after the
    \c{const}, if any:
    \snippet code/src_corelib_global_qglobal.cpp qdeclfinal-1

    For classes, it goes in front of the \c{:} in the class
    definition, if any:
    \snippet code/src_corelib_global_qglobal.cpp qdeclfinal-2

    \sa Q_DECL_OVERRIDE
*/

namespace QtPrivate {
Q_LOGGING_CATEGORY(lcNativeInterface, "qt.nativeinterface")
}

QT_END_NAMESPACE
