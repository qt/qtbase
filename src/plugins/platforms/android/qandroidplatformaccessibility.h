// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial


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
