// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
package org.qtproject.qt.android;

@UsedFromNativeCode
interface QtAccessibilityInterface {
    default void initializeAccessibility() { }
    default void notifyLocationChange(int viewId) { }
    default void notifyObjectHide(int viewId, int parentId) { }
    default void notifyObjectFocus(int viewId) { }
    default void notifyScrolledEvent(int viewId) { }
    default void notifyValueChanged(int viewId, String value) { }
    default void notifyObjectShow(int parentId) { }
}
