// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQNXLGMON_H
#define QQNXLGMON_H

#include <qglobal.h>

#if defined(QQNX_LGMON)
#include <lgmon.h>
#endif

QT_BEGIN_NAMESPACE

#if defined(QQNX_LGMON)

extern bool qqnxLgmonFirstFrame;

inline void qqnxLgmonInit()
{
    lgmon_supported(getpid());
}

inline void qqnxLgmonFramePosted(bool isCover)
{
    if (qqnxLgmonFirstFrame && !isCover) {
        qqnxLgmonFirstFrame = false;
        lgmon_app_ready_for_user_input(getpid());
    }
}

#else

inline void qqnxLgmonInit() {}
inline void qqnxLgmonFramePosted(bool /*isCover*/) {}

#endif

QT_END_NAMESPACE

#endif  // QQNXLGMON_H

