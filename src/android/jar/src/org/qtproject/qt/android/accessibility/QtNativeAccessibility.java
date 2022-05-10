// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
