// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOAWINDOWMANAGER_H
#define QCOCOAWINDOWMANAGER_H

#include <QtCore/qglobal.h>
#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

class QCocoaWindowManager
{
public:
    QCocoaWindowManager();

private:

    QMacNotificationObserver m_applicationDidFinishLaunchingObserver;
    void initialize();

    QMacKeyValueObserver m_modalSessionObserver;
    void modalSessionChanged();
};

QT_END_NAMESPACE

#endif // QCOCOAWINDOWMANAGER_H
