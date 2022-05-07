/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Android port of the Qt Toolkit.
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

package org.qtproject.qt.android.accessibility;

import android.graphics.Rect;
import android.view.accessibility.AccessibilityNodeInfo;

class QtNativeAccessibility
{
    static native void setActive(boolean enable);
    static native int[] childIdListForAccessibleObject(int objectId);
    static native int parentId(int objectId);
    static native String descriptionForAccessibleObject(int objectId);
    static native Rect screenRect(int objectId);
    static native int hitTest(float x, float y);
    static native boolean clickAction(int objectId);
    static native boolean scrollForward(int objectId);
    static native boolean scrollBackward(int objectId);

    static native boolean populateNode(int objectId, AccessibilityNodeInfo node);
    static native String valueForAccessibleObject(int objectId);
}
