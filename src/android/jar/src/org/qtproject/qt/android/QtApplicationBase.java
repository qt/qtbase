// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android;

import android.app.Application;

public class QtApplicationBase extends Application {
    @Override
    public void onTerminate() {
        QtNative.terminateQt();
        QtNative.m_qtThread.exit();
        super.onTerminate();
    }
}
