// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android.bindings;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Browser;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;

import org.qtproject.qt.android.QtActivityLoader;
import org.qtproject.qt.android.QtLoader;

public class QtActivity extends Activity
{
    public static final String EXTRA_SOURCE_INFO = "org.qtproject.qt.android.sourceInfo";

    public String APPLICATION_PARAMETERS = null; // use this variable to pass any parameters to your application,
                                                               // the parameters must not contain any white spaces
                                                               // and must be separated with "\t"
                                                               // e.g "-param1\t-param2=value2\t-param3\tvalue3"

    public String ENVIRONMENT_VARIABLES = "QT_USE_ANDROID_NATIVE_DIALOGS=1";
                                                               // use this variable to add any environment variables to your application.
                                                               // the env vars must be separated with "\t"
                                                               // e.g. "ENV_VAR1=1\tENV_VAR2=2\t"
                                                               // Currently the following vars are used by the android plugin:
                                                               // * QT_USE_ANDROID_NATIVE_DIALOGS - 1 to use the android native dialogs.

    public String[] QT_ANDROID_THEMES = null;     // A list with all themes that your application want to use.
                                                  // The name of the theme must be the same with any theme from
                                                  // http://developer.android.com/reference/android/R.style.html
                                                  // The most used themes are:
                                                  //  * "Theme" - (fallback) check http://developer.android.com/reference/android/R.style.html#Theme
                                                  //  * "Theme_Black" - check http://developer.android.com/reference/android/R.style.html#Theme_Black
                                                  //  * "Theme_Light" - (default for API <=10) check http://developer.android.com/reference/android/R.style.html#Theme_Light
                                                  //  * "Theme_Holo" - check http://developer.android.com/reference/android/R.style.html#Theme_Holo
                                                  //  * "Theme_Holo_Light" - (default for API 11-13) check http://developer.android.com/reference/android/R.style.html#Theme_Holo_Light
                                                  //  * "Theme_DeviceDefault" - check http://developer.android.com/reference/android/R.style.html#Theme_DeviceDefault
                                                  //  * "Theme_DeviceDefault_Light" - (default for API 14+) check http://developer.android.com/reference/android/R.style.html#Theme_DeviceDefault_Light

    public String QT_ANDROID_DEFAULT_THEME = null; // sets the default theme.

    private QtActivityLoader m_loader;
    public QtActivity()
    {
        m_loader = new QtActivityLoader(this, QtActivity.class);

        if (Build.VERSION.SDK_INT < 29) {
            QT_ANDROID_THEMES = new String[] {"Theme_Holo_Light"};
            QT_ANDROID_DEFAULT_THEME = "Theme_Holo_Light";
        } else {
            QT_ANDROID_THEMES = new String[] {"Theme_DeviceDefault_DayNight"};
            QT_ANDROID_DEFAULT_THEME = "Theme_DeviceDefault_DayNight";
        }
    }


    /////////////////////////// forward all notifications ////////////////////////////
    /////////////////////////// Super class calls ////////////////////////////////////
    /////////////// PLEASE DO NOT CHANGE THE FOLLOWING CODE //////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    protected void onCreateHook(Bundle savedInstanceState) {
        m_loader.APPLICATION_PARAMETERS = APPLICATION_PARAMETERS;
        m_loader.ENVIRONMENT_VARIABLES = ENVIRONMENT_VARIABLES;
        m_loader.QT_ANDROID_THEMES = QT_ANDROID_THEMES;
        m_loader.QT_ANDROID_DEFAULT_THEME = QT_ANDROID_DEFAULT_THEME;
        m_loader.onCreate(savedInstanceState);
    }

    private void addReferrer(Intent intent)
    {
        if (intent.getExtras() != null && intent.getExtras().getString(EXTRA_SOURCE_INFO) != null)
            return;

        String browserApplicationId = "";
        if (intent.getExtras() != null)
            browserApplicationId = intent.getExtras().getString(Browser.EXTRA_APPLICATION_ID);

        String sourceInformation = "";
        if (browserApplicationId != null && !browserApplicationId.isEmpty()) {
            sourceInformation = browserApplicationId;
        } else {
            Uri referrer = getReferrer();
            if (referrer != null)
                sourceInformation = referrer.toString().replaceFirst("android-app://", "");
        }

        intent.putExtra(EXTRA_SOURCE_INFO, sourceInformation);
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        QtLoader.setQtApplicationClass(QtApplication.class);
        onCreateHook(savedInstanceState);
        addReferrer(getIntent());
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event)
    {
        if (QtLoader.m_delegateObject != null && QtLoader.dispatchKeyEvent != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.dispatchKeyEvent, event);
        else
            return super.dispatchKeyEvent(event);
    }
    public boolean super_dispatchKeyEvent(KeyEvent event)
    {
        return super.dispatchKeyEvent(event);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {

        if (QtLoader.m_delegateObject != null && QtLoader.onActivityResult != null) {
            QtLoader.invokeDelegateMethod(QtLoader.onActivityResult, requestCode, resultCode, data);
            return;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }
    public void super_onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        if (!QtLoader.invokeDelegate(newConfig).invoked)
            super.onConfigurationChanged(newConfig);
    }
    public void super_onConfigurationChanged(Configuration newConfig)
    {
        super.onConfigurationChanged(newConfig);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(item);
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onContextItemSelected(item);
    }
    public boolean super_onContextItemSelected(MenuItem item)
    {
        return super.onContextItemSelected(item);
    }

    @Override
    public void onContextMenuClosed(Menu menu)
    {
        if (!QtLoader.invokeDelegate(menu).invoked)
            super.onContextMenuClosed(menu);
    }
    public void super_onContextMenuClosed(Menu menu)
    {
        super.onContextMenuClosed(menu);
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
    {
        if (!QtLoader.invokeDelegate(menu, v, menuInfo).invoked)
            super.onCreateContextMenu(menu, v, menuInfo);
    }
    public void super_onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
    {
        super.onCreateContextMenu(menu, v, menuInfo);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(menu);
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onCreateOptionsMenu(menu);
    }
    public boolean super_onCreateOptionsMenu(Menu menu)
    {
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        QtLoader.invokeDelegate();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (QtLoader.m_delegateObject != null && QtLoader.onKeyDown != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.onKeyDown, keyCode, event);
        else
            return super.onKeyDown(keyCode, event);
    }
    public boolean super_onKeyDown(int keyCode, KeyEvent event)
    {
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if (QtLoader.m_delegateObject != null  && QtLoader.onKeyUp != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.onKeyUp, keyCode, event);
        else
            return super.onKeyUp(keyCode, event);
    }
    public boolean super_onKeyUp(int keyCode, KeyEvent event)
    {
        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
        addReferrer(intent);
        if (!QtLoader.invokeDelegate(intent).invoked)
            super.onNewIntent(intent);
    }
    public void super_onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(item);
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onOptionsItemSelected(item);
    }
    public boolean super_onOptionsItemSelected(MenuItem item)
    {
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onOptionsMenuClosed(Menu menu)
    {
        if (!QtLoader.invokeDelegate(menu).invoked)
            super.onOptionsMenuClosed(menu);
    }
    public void super_onOptionsMenuClosed(Menu menu)
    {
        super.onOptionsMenuClosed(menu);
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        QtLoader.invokeDelegate();
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(menu);
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onPrepareOptionsMenu(menu);
    }
    public boolean super_onPrepareOptionsMenu(Menu menu)
    {
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    protected void onRestart()
    {
        super.onRestart();
        QtLoader.invokeDelegate();
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState)
    {
        if (!QtLoader.invokeDelegate(savedInstanceState).invoked)
            super.onRestoreInstanceState(savedInstanceState);
    }
    public void super_onRestoreInstanceState(Bundle savedInstanceState)
    {
        super.onRestoreInstanceState(savedInstanceState);
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        QtLoader.invokeDelegate();
    }

    @Override
    public Object onRetainNonConfigurationInstance()
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate();
        if (res.invoked)
            return res.methodReturns;
        else
            return super.onRetainNonConfigurationInstance();
    }
    public Object super_onRetainNonConfigurationInstance()
    {
        return super.onRetainNonConfigurationInstance();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState)
    {
        if (!QtLoader.invokeDelegate(outState).invoked)
            super.onSaveInstanceState(outState);
    }
    public void super_onSaveInstanceState(Bundle outState)
    {
        super.onSaveInstanceState(outState);

    }

    @Override
    protected void onStart()
    {
        super.onStart();
        QtLoader.invokeDelegate();
    }

    @Override
    protected void onStop()
    {
        super.onStop();
        QtLoader.invokeDelegate();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        if (!QtLoader.invokeDelegate(hasFocus).invoked)
            super.onWindowFocusChanged(hasFocus);
    }
    public void super_onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent ev)
    {
        if (QtLoader.m_delegateObject != null  && QtLoader.dispatchGenericMotionEvent != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.dispatchGenericMotionEvent, ev);
        else
            return super.dispatchGenericMotionEvent(ev);
    }
    public boolean super_dispatchGenericMotionEvent(MotionEvent event)
    {
        return super.dispatchGenericMotionEvent(event);
    }

    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        if (QtLoader.m_delegateObject != null && QtLoader.onRequestPermissionsResult != null) {
            QtLoader.invokeDelegateMethod(QtLoader.onRequestPermissionsResult, requestCode ,
                    permissions, grantResults);
        }
    }
    //---------------------------------------------------------------------------
}
