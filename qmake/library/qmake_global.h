/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QMAKE_GLOBAL_H
#define QMAKE_GLOBAL_H

#include <qglobal.h>

#if defined(QMAKE_AS_LIBRARY)
#  if defined(QMAKE_LIBRARY)
#    define QMAKE_EXPORT Q_DECL_EXPORT
#  else
#    define QMAKE_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define QMAKE_EXPORT
#endif

// Be fast even for debug builds
// MinGW GCC 4.5+ has a problem with always_inline putTok and putBlockLen
#if defined(__GNUC__) && !(defined(__MINGW32__) && __GNUC__ == 4 && __GNUC_MINOR__ >= 5)
# define ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define ALWAYS_INLINE __forceinline
#else
# define ALWAYS_INLINE inline
#endif

#ifdef PROEVALUATOR_FULL
#  define PROEVALUATOR_DEBUG
#endif

#endif
