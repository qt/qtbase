// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class QtServiceBase extends Service {
    @Override
    public void onCreate()
    {
        super.onCreate();

        // the application has already started, do not reload everything again
        if (QtNative.getStateDetails().isStarted) {
            Log.w(QtNative.QtTAG,
                    "A QtService tried to start in the same process as an initiated " +
                            "QtActivity. That is not supported. This results in the service " +
                            "functioning as an Android Service detached from Qt.");
            return;
        }

        QtNative.setService(this);

        QtServiceLoader loader = new QtServiceLoader(this);
        loader.loadQtLibraries();
        QtNative.startApplication(loader.getApplicationParameters(), loader.getMainLibraryPath());
        QtNative.setApplicationState(QtNative.ApplicationState.ApplicationHidden);
    }

    @Override
    public void onDestroy()
    {
        super.onDestroy();
        QtNative.quitQtCoreApplication();
        QtNative.terminateQt();
        QtNative.setService(null);
        QtNative.getQtThread().exit();
        System.exit(0);
    }

    @Override
    public IBinder onBind(Intent intent) {
        synchronized (this) {
            return QtNative.onBind(intent);
        }
    }
}
