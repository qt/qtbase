// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android;

@UsedFromNativeCode
interface QtMenuInterface {
    void resetOptionsMenu();
    void openOptionsMenu();
    void closeContextMenu();
    void openContextMenu(final int x, final int y, final int w, final int h);
}
