// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.Window;

import java.lang.reflect.Field;

public class QtActivityLoader extends QtLoader {
    Activity m_activity;

    public QtActivityLoader(Activity activity)
    {
        super(activity);
        m_activity = activity;
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
            m_contextInfo = m_activity.getPackageManager().getActivityInfo(
                    m_activity.getComponentName(), PackageManager.GET_META_DATA);
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
            m_activity.setTheme(Class.forName("android.R$style").getDeclaredField(
                    QT_ANDROID_DEFAULT_THEME).getInt(null));
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

            ((QtActivityBase)m_activity).getActivityDelegate().onCreate(savedInstanceState);

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
