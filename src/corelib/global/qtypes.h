// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTYPES_H
#define QTYPES_H

#include <QtCore/qprocessordetection.h>
#include <QtCore/qtconfigmacros.h>

#ifdef __cplusplus
#  include <cstddef>
#  include <cstdint>
#endif

#if 0
#pragma qt_class(QtTypes)
#pragma qt_class(QIntegerForSize)
#pragma qt_sync_stop_processing
#endif

#ifndef __ASSEMBLER__

/*
   Useful type definitions for Qt
*/
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

QT_BEGIN_NAMESPACE

/*
   Size-dependent types (architecture-dependent byte order)

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

#if defined(QT_COORD_TYPE)
typedef QT_COORD_TYPE qreal;
#else
typedef double qreal;
#endif

#if defined(__cplusplus)
/*
  quintptr are qptrdiff is guaranteed to be the same size as a pointer, i.e.

      sizeof(void *) == sizeof(quintptr)
      && sizeof(void *) == sizeof(qptrdiff)

  While size_t and qsizetype are not guaranteed to be the same size as a pointer,
  they usually are and we do check for that in qtypes.cpp, just to be sure.
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
#if SIZE_MAX == 0xffffffffULL
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
#elif SIZE_MAX == 0xffffffffffffffffULL
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

#endif // __cplusplus

QT_END_NAMESPACE

#endif // __ASSEMBLER__

#endif // QTYPES_H
