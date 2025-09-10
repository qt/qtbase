// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QANDROIDPLATFORMACCESSIBILITY_H
#define QANDROIDPLATFORMACCESSIBILITY_H

#include <qpa/qplatformaccessibility.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformAccessibility: public QPlatformAccessibility
{
public:
    QAndroidPlatformAccessibility();
    ~QAndroidPlatformAccessibility();

    void notifyAccessibilityUpdate(QAccessibleEvent *event) override;
    void setRootObject(QObject *obj) override;
};

QT_END_NAMESPACE

#endif
