/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Android port of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

package org.qtproject.qt5.android;

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

    private Service m_service = null;
    private String m_mainLib;
    private static String m_environmentVariables = null;
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

        QtNative.setApplicationDisplayMetrics(10, 10, 10, 10, 120, 120, 1.0, 1.0);

        if (loaderParams.containsKey(STATIC_INIT_CLASSES_KEY)) {
            for (String className: loaderParams.getStringArray(STATIC_INIT_CLASSES_KEY)) {
                if (className.length() == 0)
                    continue;

                try {
                    Class<?> initClass = classLoader.loadClass(className);
                    Object staticInitDataObject = initClass.newInstance(); // create an instance
                    Method m = initClass.getMethod("setService", Service.class, Object.class);
                    m.invoke(staticInitDataObject, m_service, this);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        QtNative.loadQtLibraries(loaderParams.getStringArrayList(NATIVE_LIBRARIES_KEY));
        ArrayList<String> libraries = loaderParams.getStringArrayList(BUNDLED_LIBRARIES_KEY);
        QtNative.loadBundledLibraries(libraries, QtNativeLibrariesDir.nativeLibrariesDir(m_service));
        m_mainLib = loaderParams.getString(MAIN_LIBRARY_KEY);

        m_environmentVariables = loaderParams.getString(ENVIRONMENT_VARIABLES_KEY);
        String additionalEnvironmentVariables = "QT_ANDROID_FONTS_MONOSPACE=Droid Sans Mono;Droid Sans;Droid Sans Fallback"
                                              + "\tQT_ANDROID_FONTS_SERIF=Droid Serif"
                                              + "\tHOME=" + m_service.getFilesDir().getAbsolutePath()
                                              + "\tTMPDIR=" + m_service.getFilesDir().getAbsolutePath();
        if (Build.VERSION.SDK_INT < 14)
            additionalEnvironmentVariables += "\tQT_ANDROID_FONTS=Droid Sans;Droid Sans Fallback";
        else
            additionalEnvironmentVariables += "\tQT_ANDROID_FONTS=Roboto;Droid Sans;Droid Sans Fallback";

        if (m_environmentVariables != null && m_environmentVariables.length() > 0)
            m_environmentVariables = additionalEnvironmentVariables + "\t" + m_environmentVariables;
        else
            m_environmentVariables = additionalEnvironmentVariables;

        if (loaderParams.containsKey(APPLICATION_PARAMETERS_KEY))
            m_applicationParameters = loaderParams.getString(APPLICATION_PARAMETERS_KEY);
        else
            m_applicationParameters = "";

        return true;
    }

    public boolean startApplication()
    {
        // start application
        try {
            String nativeLibraryDir = QtNativeLibrariesDir.nativeLibrariesDir(m_service);
            QtNative.startApplication(m_applicationParameters,
                    m_environmentVariables,
                    m_mainLib,
                    nativeLibraryDir);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public void onDestroy()
    {
        QtNative.setService(null, null);
    }
}
