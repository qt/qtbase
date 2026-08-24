// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWIN32TICKS_P_H
#define QWIN32TICKS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#include <chrono>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

// Win32 ticks: a 100 nanosecond period, as used e.g. by REFERENCE_TIME
// (DirectShow, Media Foundation), FILETIME, and SetWaitableTimerEx.
using win32_ticks = std::chrono::duration<qint64, std::ratio<1, 10'000'000>>;

static_assert(win32_ticks(1) == std::chrono::nanoseconds(100));

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QWIN32TICKS_P_H
