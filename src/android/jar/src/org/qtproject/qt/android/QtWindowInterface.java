// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
package org.qtproject.qt.android;
@UsedFromNativeCode
interface QtWindowInterface {
    default void addTopLevelWindow(final QtWindow window) { }
    default void removeTopLevelWindow(final int id) { }
    default void bringChildToFront(final int id) { }
    default void bringChildToBack(int id) { }
    default void setSystemUiVisibility(boolean isFullScreen, boolean expandedToCutout) { }
}
