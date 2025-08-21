// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDJNIACCESSIBILITY_H
#define ANDROIDJNIACCESSIBILITY_H
#include <jni.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class QObject;
class QJniEnvironment;

namespace QtAndroidAccessibility
{
    bool isActive();
    bool registerNatives(QJniEnvironment &env);
    void notifyLocationChange(uint accessibilityObjectId);
    void notifyObjectHide(uint accessibilityObjectId);
    void notifyObjectShow(uint accessibilityObjectId);
    void notifyObjectFocus(uint accessibilityObjectId);
    void notifyValueChanged(uint accessibilityObjectId);
    void notifyDescriptionOrNameChanged(uint accessibilityObjectId);
    void notifyScrolledEvent(uint accessibilityObjectId);
    void notifyAnnouncementEvent(uint accessibilityObjectId, const QString &message);
    void createAccessibilityContextObject(QObject *parent);
}

QT_END_NAMESPACE

#endif // ANDROIDJNIINPUT_H
