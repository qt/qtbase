// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTRACECALLBACK_P_H
#define QTRACECALLBACK_P_H

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

#include <QtCore/private/qglobal_p.h>

QT_REQUIRE_CONFIG(tracecallback);

QT_BEGIN_NAMESPACE

// The "callback" tracegen backend turns every Q_TRACE point into a call to a
// single process-wide hook, passing a pointer to a static descriptor of the
// firing point and the arguments as varargs. The callback is supposed to figure
// out what to do with the arguments using the tracepoint name/provider. It is
// meant for in-process profilers (e.g. benchmarks) that need to attribute time
// to the tracepoint that is currently executing. As such, the use cases are
// specialized for a finite collection of trace points of which we can
// statically know the argument types.
struct QTraceCallbackTracepoint
{
    const char *provider;
    const char *name;
    // Scratch space owned by the installed callback. The generated code initialises it
    // to -1; the callback may cache whatever it resolves the descriptor to (e.g. a
    // category id, or entry/exit kind) so that repeated firings of a hot tracepoint do
    // not re-parse the name. Each descriptor is a unique, stable object, so its address
    // doubles as an identity key.
    qint64 cookie;
};

using QTraceCallbackFunction = void (*)(QTraceCallbackTracepoint *...);

// Read on every tracepoint firing by the generated wrappers; set via qSetTraceCallback().
// Plain pointer on purpose: it is expected to be installed once before a measured region
// and left untouched while tracing, so the hot-path read needs no synchronisation.
Q_CORE_EXPORT extern QTraceCallbackFunction qtTraceCallbackFunction;

// Installs (or clears, with nullptr) the trace callback and returns the previous one.
Q_CORE_EXPORT QTraceCallbackFunction qSetTraceCallback(QTraceCallbackFunction callback);

QT_END_NAMESPACE

#endif // QTRACECALLBACK_P_H
