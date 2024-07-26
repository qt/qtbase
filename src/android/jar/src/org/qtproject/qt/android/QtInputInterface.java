// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android;

import android.app.Activity;

@UsedFromNativeCode
interface QtInputInterface {
    void updateSelection(final int selStart, final int selEnd, final int candidatesStart,
                         final int candidatesEnd);
    void showSoftwareKeyboard(Activity activity, final int x, final int y,
                              final int width, final int height, final int inputHints,
                              final int enterKeyType);
    void resetSoftwareKeyboard();
    void hideSoftwareKeyboard();
    boolean isSoftwareKeyboardVisible();
    int getSelectionHandleWidth();
    void updateHandles(int mode, int editX, int editY, int editButtons,
                       int x1, int y1, int x2, int y2, boolean rtl);
    QtInputConnection.QtInputConnectionListener getInputConnectionListener();
}
