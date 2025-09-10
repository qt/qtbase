// Copyright (C) 2011 - 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <errno.h>

#include <QDebug>
#include "qqnxintegration.h"

QT_BEGIN_NAMESPACE

void qScreenCheckError(int rc, const char *funcInfo, const char *message, bool critical)
{
    if (!rc && (QQnxIntegration::instance()->options() & QQnxIntegration::AlwaysFlushScreenContext)
            && QQnxIntegration::instance()->screenContext() != 0) {
        rc = screen_flush_context(QQnxIntegration::instance()->screenContext(), 0);
    }

    if (Q_UNLIKELY(rc)) {
        qCDebug(lcQpaQnx, "%s - Screen: %s - Error: %s (%i)", funcInfo, message, strerror(errno), errno);

        if (Q_UNLIKELY(critical))
            qCritical("%s - Screen: %s - Error: %s (%i)", funcInfo, message, strerror(errno), errno);
        else
            qWarning("%s - Screen: %s - Error: %s (%i)", funcInfo, message, strerror(errno), errno);
    }
}

QT_END_NAMESPACE
