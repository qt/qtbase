// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2015 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QGLOBAL_P_H
#define QGLOBAL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qglobal.h"
#include "qglobal_p.h"      // include self to avoid syncqt warning - no-op

#ifndef QT_BOOTSTRAPPED
#include <QtCore/private/qtcoreglobal_p.h>
#endif

#if defined(Q_CC_MSVC)
// By default, dynamic initialization uses subsection "$XCU", which is
// equivalent to #pragma init_seg(user). Additionally, #pragma
// init_seg(compiler) and init_seg(lib) use "$XCC" and "$XCL" respectively. So
// place us between "compiler" and "lib".
#  define QT_SUPPORTS_INIT_PRIORITY     1

// warning C4075: initializers put in unrecognized initialization area
#  define Q_DECL_INIT_PRIORITY(nn)      \
    __pragma(warning(disable: 4075)) \
    __pragma(init_seg(".CRT$XCK" QT_STRINGIFY(nn))) Q_DECL_UNUSED
#elif defined(Q_OS_QNX)
// init_priority fails on QNX and we didn't bother investigating why
#  define QT_SUPPORTS_INIT_PRIORITY     0
#elif defined(Q_OS_WIN) || defined(Q_OF_ELF)
#  define QT_SUPPORTS_INIT_PRIORITY     1
// priorities 0 to 1000 are reserved to the runtime;
// we use above 2000 in case someone REALLY needs to go before us
#  define Q_DECL_INIT_PRIORITY(nn)      __attribute__((init_priority(2000 + nn), used))
#elif defined(QT_SHARED)
// it doesn't support this exactly, but we can work around it
#  define QT_SUPPORTS_INIT_PRIORITY     -1
#  define Q_DECL_INIT_PRIORITY(nn)      Q_DECL_UNUSED
#else
#  define QT_SUPPORTS_INIT_PRIORITY     0
#endif

#if defined(__cplusplus)
QT_BEGIN_NAMESPACE

Q_NORETURN Q_CORE_EXPORT void qAbort();

QT_END_NAMESPACE
#endif // defined(__cplusplus)

#endif // QGLOBAL_P_H
