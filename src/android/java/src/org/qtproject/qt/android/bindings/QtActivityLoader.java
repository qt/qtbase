// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android.bindings;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.ColorDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.Window;

import org.qtproject.qt.android.QtNative;

import java.lang.reflect.Field;

public class QtActivityLoader extends QtLoader {
    QtActivity m_activity;

    QtActivityLoader(QtActivity activity)
    {
        super(activity, QtActivity.class);
        m_activity = activity;
    }

    @Override
    protected String loaderClassName() {
        return "org.qtproject.qt.android.QtActivityDelegate";
    }

    @Override
    protected Class<?> contextClassName() {
        return android.app.Activity.class;
    }

    @Override
    protected void finish() {
        m_activity.finish();
    }

    @Override
    protected String getTitle() {
        return (String) m_activity.getTitle();
    }

    @Override
    protected void runOnUiThread(Runnable run) {
        m_activity.runOnUiThread(run);
    }

    @Override
    Intent getIntent() {
        return m_activity.getIntent();
    }

    public void onCreate(Bundle savedInstanceState) {
        try {
            m_contextInfo = m_activity.getPackageManager().getActivityInfo(m_activity.getComponentName(), PackageManager.GET_META_DATA);
            int theme = ((ActivityInfo)m_contextInfo).getThemeResource();
            for (Field f : Class.forName("android.R$style").getDeclaredFields()) {
                if (f.getInt(null) == theme) {
                    QT_ANDROID_THEMES = new String[] {f.getName()};
                    QT_ANDROID_DEFAULT_THEME = f.getName();
                    break;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            finish();
            return;
        }

        try {
            m_activity.setTheme(Class.forName("android.R$style").getDeclaredField(QT_ANDROID_DEFAULT_THEME).getInt(null));
        } catch (Exception e) {
            e.printStackTrace();
        }

        m_activity.requestWindowFeature(Window.FEATURE_ACTION_BAR);
        if (QtNative.isStarted()) {
            boolean updated = QtNative.activityDelegate().updateActivity(m_activity);
            if (!updated) {
                //  could not update the activity so restart the application
                Intent intent = Intent.makeRestartActivityTask(m_activity.getComponentName());
                m_activity.startActivity(intent);
                QtNative.quitApp();
                Runtime.getRuntime().exit(0);
            }

            // there can only be a valid delegate object if the QtNative was started.
            if (QtApplication.m_delegateObject != null && QtApplication.onCreate != null)
                QtApplication.invokeDelegateMethod(QtApplication.onCreate, savedInstanceState);

            return;
        }

        m_displayDensity = m_activity.getResources().getDisplayMetrics().densityDpi;

        ENVIRONMENT_VARIABLES += "\tQT_ANDROID_THEME=" + QT_ANDROID_DEFAULT_THEME
                + "/\tQT_ANDROID_THEME_DISPLAY_DPI=" + m_displayDensity + "\t";

        if (m_contextInfo.metaData.containsKey("android.app.background_running")
                && m_contextInfo.metaData.getBoolean("android.app.background_running")) {
            ENVIRONMENT_VARIABLES += "QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED=0\t";
        } else {
            ENVIRONMENT_VARIABLES += "QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED=1\t";
        }

        startApp(true);

    }
}
