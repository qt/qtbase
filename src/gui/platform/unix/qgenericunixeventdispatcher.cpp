// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgenericunixeventdispatcher_p.h"
#include "qunixeventdispatcher_qpa_p.h"
#if QT_CONFIG(glib)
#   include "qeventdispatcher_glib_p.h"
#endif
QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher *QtGenericUnixDispatcher::createUnixEventDispatcher()
{
#if !defined(QT_NO_GLIB) && !defined(Q_OS_WIN)
    if (qEnvironmentVariableIsEmpty("QT_NO_GLIB") && QEventDispatcherGlib::versionSupported())
        return new QPAEventDispatcherGlib();
    else
#endif
        return new QUnixEventDispatcherQPA();
}

QT_END_NAMESPACE
