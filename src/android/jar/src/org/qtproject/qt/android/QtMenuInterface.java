// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
package org.qtproject.qt.android;

@UsedFromNativeCode
interface QtMenuInterface {
    void resetOptionsMenu();
    void openOptionsMenu();
    void closeContextMenu();
    void openContextMenu(final int x, final int y, final int w, final int h);
}
