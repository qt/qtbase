/***************************************************************************
**
** Copyright (C) 2011 - 2014 BlackBerry Limited. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
