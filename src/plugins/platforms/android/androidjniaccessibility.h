/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef ANDROIDJNIACCESSIBILITY_H
#define ANDROIDJNIACCESSIBILITY_H
#include <jni.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QtAndroidAccessibility
{
    void initialize();
    bool isActive();
    bool registerNatives(JNIEnv *env);
    void notifyLocationChange();
    void notifyObjectHide(uint accessibilityObjectId);
    void notifyObjectFocus(uint accessibilityObjectId);
}

QT_END_NAMESPACE

#endif // ANDROIDJNIINPUT_H
