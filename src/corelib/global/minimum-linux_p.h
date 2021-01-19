/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef MINIMUMLINUX_P_H
#define MINIMUMLINUX_P_H

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

// EXTRA WARNING
// -------------
//
// This file must also be valid assembler source.
//

#include "private/qglobal_p.h"

QT_BEGIN_NAMESPACE

/* Minimum Linux kernel version:
 * We require the following features in Qt (unconditional, no fallback):
 *   Feature                    Added in version        Macro
 * - inotify_init1              before 2.6.12-rc12
 * - futex(2)                   before 2.6.12-rc12
 * - FUTEX_WAKE_OP              2.6.14                  FUTEX_OP
 * - linkat(2)                  2.6.17                  O_TMPFILE && QT_CONFIG(linkat)
 * - FUTEX_PRIVATE_FLAG         2.6.22
 * - O_CLOEXEC                  2.6.23
 * - eventfd                    2.6.23
 * - pipe2 & dup3               2.6.27
 * - accept4                    2.6.28
 * - renameat2                  3.16                    QT_CONFIG(renameat2)
 * - getrandom                  3.17                    QT_CONFIG(getentropy)
 * - statx                      4.11                    QT_CONFIG(statx)
 */

#if QT_CONFIG(statx) && !QT_CONFIG(glibc)
// if using glibc, the statx() function in sysdeps/unix/sysv/linux/statx.c
// falls back to stat() for us.
// (Using QT_CONFIG(glibc) instead of __GLIBC__ because the macros aren't
// defined in assembler mode)
#  define MINLINUX_MAJOR        4
#  define MINLINUX_MINOR        11
#  define MINLINUX_PATCH        0
#elif QT_CONFIG(getentropy)
#  define MINLINUX_MAJOR        3
#  define MINLINUX_MINOR        17
#  define MINLINUX_PATCH        0
#elif QT_CONFIG(renameat2)
#  define MINLINUX_MAJOR        3
#  define MINLINUX_MINOR        16
#  define MINLINUX_PATCH        0
#else
#  define MINLINUX_MAJOR        2
#  define MINLINUX_MINOR        6
#  define MINLINUX_PATCH        28
#endif

#define MINIMUM_LINUX_VERSION  QT_VERSION_CHECK(MINLINUX_MAJOR, MINLINUX_MINOR, MINLINUX_PATCH)

QT_END_NAMESPACE

#endif // MINIMUMLINUX_P_H
