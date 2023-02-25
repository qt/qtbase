// Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.graphics.drawable.ColorDrawable;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.ResultReceiver;
import android.text.method.MetaKeyKeyListener;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Surface;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileWriter;
import java.io.InputStreamReader;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Objects;

public class QtServiceDelegate
{
    private static final String NATIVE_LIBRARIES_KEY = "native.libraries";
    private static final String BUNDLED_LIBRARIES_KEY = "bundled.libraries";
    private static final String MAIN_LIBRARY_KEY = "main.library";
    private static final String ENVIRONMENT_VARIABLES_KEY = "environment.variables";
    private static final String APPLICATION_PARAMETERS_KEY = "application.parameters";
    private static final String STATIC_INIT_CLASSES_KEY = "static.init.classes";
    private static final String APP_DISPLAY_METRIC_SCREEN_DESKTOP_KEY = "display.screen.desktop";
    private static final String APP_DISPLAY_METRIC_SCREEN_XDPI_KEY = "display.screen.dpi.x";
    private static final String APP_DISPLAY_METRIC_SCREEN_YDPI_KEY = "display.screen.dpi.y";
    private static final String APP_DISPLAY_METRIC_SCREEN_DENSITY_KEY = "display.screen.density";

    private String m_mainLib = null;
    private Service m_service = null;
    private static String m_applicationParameters = null;

    public boolean loadApplication(Service service, ClassLoader classLoader, Bundle loaderParams)
    {
        /// check parameters integrity
        if (!loaderParams.containsKey(NATIVE_LIBRARIES_KEY)
                || !loaderParams.containsKey(BUNDLED_LIBRARIES_KEY)) {
            return false;
        }

        m_service = service;
        QtNative.setService(m_service, this);
        QtNative.setClassLoader(classLoader);

        QtNative.setApplicationDisplayMetrics(10, 10, 0, 0, 10, 10, 120, 120, 1.0, 1.0, 60.0f);

        if (loaderParams.containsKey(STATIC_INIT_CLASSES_KEY)) {
            for (String className :
                 Objects.requireNonNull(loaderParams.getStringArray(STATIC_INIT_CLASSES_KEY))) {
                if (className.length() == 0)
                    continue;
                try {
                  Class<?> initClass = classLoader.loadClass(className);
                  Object staticInitDataObject = initClass.newInstance(); // create an instance
                  try {
                      Method m = initClass.getMethod("setService", Service.class, Object.class);
                      m.invoke(staticInitDataObject, m_service, this);
                  } catch (Exception e) {
                      Log.d(QtNative.QtTAG,
                            "Class " + className + " does not implement setService method");
                  }

                  // For modules that don't need/have setService
                  try {
                      Method m = initClass.getMethod("setContext", Context.class);
                      m.invoke(staticInitDataObject, (Context)m_service);
                  } catch (Exception e) {
                      e.printStackTrace();
                  }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        QtNative.loadQtLibraries(loaderParams.getStringArrayList(NATIVE_LIBRARIES_KEY));
        ArrayList<String> libraries = loaderParams.getStringArrayList(BUNDLED_LIBRARIES_KEY);
        String nativeLibsDir = QtNativeLibrariesDir.nativeLibrariesDir(m_service);
        QtNative.loadBundledLibraries(libraries, nativeLibsDir);
        m_mainLib = loaderParams.getString(MAIN_LIBRARY_KEY);

        QtNative.setEnvironmentVariables(loaderParams.getString(ENVIRONMENT_VARIABLES_KEY));
        QtNative.setEnvironmentVariable("QT_ANDROID_FONTS_MONOSPACE",
                                        "Droid Sans Mono;Droid Sans;Droid Sans Fallback");
        QtNative.setEnvironmentVariable("QT_ANDROID_FONTS_SERIF", "Droid Serif");
        QtNative.setEnvironmentVariable("HOME", m_service.getFilesDir().getAbsolutePath());
        QtNative.setEnvironmentVariable("TMPDIR", m_service.getCacheDir().getAbsolutePath());
        QtNative.setEnvironmentVariable("QT_ANDROID_FONTS",
                                        "Roboto;Droid Sans;Droid Sans Fallback");

        if (loaderParams.containsKey(APPLICATION_PARAMETERS_KEY))
            m_applicationParameters = loaderParams.getString(APPLICATION_PARAMETERS_KEY);
        else
            m_applicationParameters = "";

        m_mainLib = QtNative.loadMainLibrary(m_mainLib, nativeLibsDir);
        return m_mainLib != null;
    }

    public boolean startApplication()
    {
        // start application
        try {
            String nativeLibraryDir = QtNativeLibrariesDir.nativeLibrariesDir(m_service);
            QtNative.startApplication(m_applicationParameters, m_mainLib);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public void onDestroy()
    {
        QtNative.quitQtCoreApplication();
        QtNative.terminateQt();
        QtNative.setService(null, null);
        QtNative.m_qtThread.exit();
        System.exit(0);
    }

    public IBinder onBind(Intent intent)
    {
        synchronized (this) {
            return QtNative.onBind(intent);
        }
    }
}
