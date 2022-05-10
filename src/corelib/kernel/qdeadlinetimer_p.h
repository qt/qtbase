// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDEADLINETIMER_P_H
#define QDEADLINETIMER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

enum {
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    // t1 contains seconds and t2 contains nanoseconds
    QDeadlineTimerNanosecondsInT2 = 1
#else
    // t1 contains nanoseconds, t2 is always zero
    QDeadlineTimerNanosecondsInT2 = 0
#endif
};

QT_END_NAMESPACE

#endif
