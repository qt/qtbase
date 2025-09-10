// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android;
@UsedFromNativeCode
interface QtWindowInterface {
    default void addTopLevelWindow(final QtWindow window) { }
    default void removeTopLevelWindow(final int id) { }
    default void bringChildToFront(final int id) { }
    default void bringChildToBack(int id) { }
    default void setSystemUiVisibility(boolean isFullScreen, boolean expandedToCutout) { }
}
