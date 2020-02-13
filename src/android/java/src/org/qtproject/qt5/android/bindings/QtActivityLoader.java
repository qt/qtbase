/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
    Contact: http://www.qt-project.org/legal

    Commercial License Usage
    Licensees holding valid commercial Qt licenses may use this file in
    accordance with the commercial license agreement provided with the
    Software or, alternatively, in accordance with the terms contained in
    a written agreement between you and Digia.  For licensing terms and
    conditions see http://qt.digia.com/licensing.  For further information
    use the contact form at http://qt.digia.com/contact-us.

    BSD License Usage
    Alternatively, this file may be used under the BSD license as follows:
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package org.qtproject.qt5.android.bindings;

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


import java.lang.reflect.Field;

public class QtActivityLoader extends QtLoader {
    QtActivity m_activity;

    QtActivityLoader(QtActivity activity)
    {
        super(activity, QtActivity.class);
        m_activity = activity;
    }
    @Override
    protected void downloadUpgradeMinistro(String msg) {
        AlertDialog.Builder downloadDialog = new AlertDialog.Builder(m_activity);
        downloadDialog.setMessage(msg);
        downloadDialog.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                try {
                    Uri uri = Uri.parse("market://details?id=org.kde.necessitas.ministro");
                    Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                    m_activity.startActivityForResult(intent, MINISTRO_INSTALL_REQUEST_CODE);
                } catch (Exception e) {
                    e.printStackTrace();
                    ministroNotFound();
                }
            }
        });

        downloadDialog.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                m_activity.finish();
            }
        });
        downloadDialog.show();
    }

    @Override
    protected String loaderClassName() {
        return "org.qtproject.qt5.android.QtActivityDelegate";
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

        if (QtApplication.m_delegateObject != null && QtApplication.onCreate != null) {
            QtApplication.invokeDelegateMethod(QtApplication.onCreate, savedInstanceState);
            return;
        }

        m_displayDensity = m_activity.getResources().getDisplayMetrics().densityDpi;

        ENVIRONMENT_VARIABLES += "\tQT_ANDROID_THEME=" + QT_ANDROID_DEFAULT_THEME
                + "/\tQT_ANDROID_THEME_DISPLAY_DPI=" + m_displayDensity + "\t";

        if (null == m_activity.getLastNonConfigurationInstance()) {
            if (m_contextInfo.metaData.containsKey("android.app.background_running")
                    && m_contextInfo.metaData.getBoolean("android.app.background_running")) {
                ENVIRONMENT_VARIABLES += "QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED=0\t";
            } else {
                ENVIRONMENT_VARIABLES += "QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED=1\t";
            }

            if (m_contextInfo.metaData.containsKey("android.app.auto_screen_scale_factor")
                    && m_contextInfo.metaData.getBoolean("android.app.auto_screen_scale_factor")) {
                ENVIRONMENT_VARIABLES += "QT_AUTO_SCREEN_SCALE_FACTOR=1\t";
            }

            startApp(true);
        }
    }
}
