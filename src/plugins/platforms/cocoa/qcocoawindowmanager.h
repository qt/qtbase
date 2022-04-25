/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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
