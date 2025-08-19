// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSPLATFORMACCESSIBILITY_H
#define QIOSPLATFORMACCESSIBILITY_H

#include <qpa/qplatformaccessibility.h>
#include <QtCore/private/qcore_mac_p.h>
#include <quiaccessibilityelement.h>

#if QT_CONFIG(accessibility)

QT_BEGIN_NAMESPACE

class QIOSPlatformAccessibility: public QPlatformAccessibility
{
public:
    QIOSPlatformAccessibility();
    ~QIOSPlatformAccessibility();

    virtual void notifyAccessibilityUpdate(QAccessibleEvent *event);

private:
    QMacNotificationObserver m_focusObserver;
    QMacAccessibilityElement *m_focusElement;
};

QT_END_NAMESPACE

#endif

#endif
