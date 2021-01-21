/***************************************************************************
**
** Copyright (C) 2011 - 2014 BlackBerry Limited. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
        if (Q_UNLIKELY(critical))
            qCritical("%s - Screen: %s - Error: %s (%i)", funcInfo, message, strerror(errno), errno);
        else
            qWarning("%s - Screen: %s - Error: %s (%i)", funcInfo, message, strerror(errno), errno);
    }
}

QT_END_NAMESPACE
