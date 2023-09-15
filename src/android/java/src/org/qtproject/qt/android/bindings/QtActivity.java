// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android.bindings;

import android.os.Bundle;
import android.provider.Browser;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.os.Build;

import org.qtproject.qt.android.QtActivityBase;

public class QtActivity extends QtActivityBase
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        setAppDetails();
        super.onCreate(savedInstanceState);
    }

    private void setAppDetails()
    {
        // use this variable to pass any parameters to your application,
        // the parameters must not contain any white spaces
        // and must be separated with "\t"
        // e.g "-param1\t-param2=value2\t-param3\tvalue3"
        APPLICATION_PARAMETERS = "";

        // Use this variable to add any environment variables to your application.
        // the env vars must be separated with "\t"
        // e.g. "ENV_VAR1=1\tENV_VAR2=2\t"
        // Currently the following vars are used by the android plugin:
        // * QT_USE_ANDROID_NATIVE_DIALOGS - 1 to use the android native dialogs.
        ENVIRONMENT_VARIABLES = "QT_USE_ANDROID_NATIVE_DIALOGS=1";

        // A list with all themes that your application want to use.
        // The name of the theme must be the same with any theme from
        // http://developer.android.com/reference/android/R.style.html
        // The most used themes are:
        //  * "Theme_Light"
        //  * "Theme_Holo"
        //  * "Theme_Holo_Light"

        if (Build.VERSION.SDK_INT < 29) {
            QT_ANDROID_THEMES = new String[] {"Theme_Holo_Light"};
            QT_ANDROID_DEFAULT_THEME = "Theme_Holo_Light";
        } else {
            QT_ANDROID_THEMES = new String[] {"Theme_DeviceDefault_DayNight"};
            QT_ANDROID_DEFAULT_THEME = "Theme_DeviceDefault_DayNight";
        }
    }
}
