// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Application;

public class QtApplicationBase extends Application {
    @Override
    public void onTerminate() {
        QtNative.terminateQt();
        QtNative.getQtThread().exit();
        super.onTerminate();
    }
}
