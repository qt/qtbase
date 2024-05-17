// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PLATFORMCLIPBOARD_H
#define PLATFORMCLIPBOARD_H

#include <qglobal.h>

struct PlatformClipboard
{
    static inline bool isAvailable()
    {
#if defined(QT_NO_CLIPBOARD)
        return false;
#else
        return true;
#endif
    }
};

#endif // PLATFORMCLIPBOARD_H
