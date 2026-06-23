// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qtracecallback_p.h>

QT_BEGIN_NAMESPACE

QTraceCallbackFunction qtTraceCallbackFunction = nullptr;

QTraceCallbackFunction qSetTraceCallback(QTraceCallbackFunction callback)
{
    QTraceCallbackFunction previous = qtTraceCallbackFunction;
    qtTraceCallbackFunction = callback;
    return previous;
}

QT_END_NAMESPACE
