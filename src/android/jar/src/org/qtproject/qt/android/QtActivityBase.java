// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Browser;
import android.text.method.MetaKeyKeyListener;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;

public class QtActivityBase extends Activity
{
    private boolean m_optionsMenuIsVisible = false;

    // use this variable to pass any parameters to your application,
    // the parameters must not contain any white spaces
    // and must be separated with "\t"
    // e.g "-param1\t-param2=value2\t-param3\tvalue3"
    public String APPLICATION_PARAMETERS = null;

    // use this variable to add any environment variables to your application.
    // the env vars must be separated with "\t"
    // e.g. "ENV_VAR1=1\tENV_VAR2=2\t"
    // Currently the following vars are used by the android plugin:
    // * QT_USE_ANDROID_NATIVE_DIALOGS - 1 to use the android native dialogs.
    public String ENVIRONMENT_VARIABLES = "QT_USE_ANDROID_NATIVE_DIALOGS=1";

    // A list with all themes that your application want to use.
    // The name of the theme must be the same with any theme from
    // http://developer.android.com/reference/android/R.style.html
    // The most used themes are:
    //  * "Theme_Light" - (default for API <=10) check http://developer.android.com/reference/android/R.style.html#Theme_Light
    //  * "Theme_Holo" - check http://developer.android.com/reference/android/R.style.html#Theme_Holo
    //  * "Theme_Holo_Light" - (default for API 11-13) check http://developer.android.com/reference/android/R.style.html#Theme_Holo_Light
    public String[] QT_ANDROID_THEMES = null;

    public String QT_ANDROID_DEFAULT_THEME = null; // sets the default theme.

    private final QtActivityLoader m_loader = new QtActivityLoader(this);

    private final QtActivityDelegate m_delegate = new QtActivityDelegate();

    protected void onCreateHook(Bundle savedInstanceState) {
        m_loader.APPLICATION_PARAMETERS = APPLICATION_PARAMETERS;
        m_loader.ENVIRONMENT_VARIABLES = ENVIRONMENT_VARIABLES;
        m_loader.QT_ANDROID_THEMES = QT_ANDROID_THEMES;
        m_loader.QT_ANDROID_DEFAULT_THEME = QT_ANDROID_DEFAULT_THEME;
        m_loader.onCreate(savedInstanceState);
    }

    public static final String EXTRA_SOURCE_INFO = "org.qtproject.qt.android.sourceInfo";

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
        onCreateHook(savedInstanceState);
        addReferrer(getIntent());
    }

    @Override
    protected void onStart()
    {
        super.onStart();
    }

    @Override
    protected void onRestart()
    {
        super.onRestart();
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        if (Build.VERSION.SDK_INT < 24 || !isInMultiWindowMode())
            QtNative.setApplicationState(QtConstants.ApplicationState.ApplicationInactive);
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        QtNative.setApplicationState(QtConstants.ApplicationState.ApplicationActive);
        if (m_delegate.isStarted()) {
            QtNative.updateWindow();
            m_delegate.updateFullScreen(); // Suspending the app clears the immersive mode, so we need to set it again.
        }
    }

    @Override
    protected void onStop()
    {
        super.onStop();
        QtNative.setApplicationState(QtConstants.ApplicationState.ApplicationSuspended);
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        if (m_delegate.isQuitApp()) {
            QtNative.terminateQt();
            QtNative.setActivity(null, null);
            QtNative.m_qtThread.exit();
            System.exit(0);
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        super.onConfigurationChanged(newConfig);
        m_delegate.handleUiModeChange(newConfig.uiMode & Configuration.UI_MODE_NIGHT_MASK);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item)
    {
        m_delegate.setContextMenuVisible(false);
        return QtNative.onContextItemSelected(item.getItemId(), item.isChecked());
    }

    @Override
    public void onContextMenuClosed(Menu menu)
    {
        if (!m_delegate.isContextMenuVisible())
            return;
        m_delegate.setContextMenuVisible(false);
        QtNative.onContextMenuClosed(menu);
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
    {
        menu.clearHeader();
        QtNative.onCreateContextMenu(menu);
        m_delegate.setContextMenuVisible(true);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event)
    {
        if (m_delegate.isStarted() && m_delegate.getInputDelegate().handleDispatchKeyEvent(event))
            return true;

        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event)
    {
        boolean handled = m_delegate.getInputDelegate().handleDispatchGenericMotionEvent(event);
        if (m_delegate.isStarted() && handled)
            return true;

        return super.dispatchGenericMotionEvent(event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (!m_delegate.isStarted() || !m_delegate.isPluginRunning())
            return false;

        return m_delegate.getInputDelegate().onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if (!m_delegate.isStarted() || !m_delegate.isPluginRunning())
            return false;

        return m_delegate.getInputDelegate().onKeyUp(keyCode, event);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        menu.clear();
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu)
    {
        m_optionsMenuIsVisible = true;
        boolean res = QtNative.onPrepareOptionsMenu(menu);
        m_delegate.setActionBarVisibility(res && menu.size() > 0);
        return res;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        return QtNative.onOptionsItemSelected(item.getItemId(), item.isChecked());
    }

    @Override
    public void onOptionsMenuClosed(Menu menu)
    {
        m_optionsMenuIsVisible = false;
        QtNative.onOptionsMenuClosed(menu);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState)
    {
        super.onRestoreInstanceState(savedInstanceState);
        m_delegate.setStarted(savedInstanceState.getBoolean("Started"));
        // FIXME restore all surfaces
    }

    @Override
    public Object onRetainNonConfigurationInstance()
    {
        super.onRetainNonConfigurationInstance();
        m_delegate.setQuitApp(false);
        return true;

    }

    @Override
    protected void onSaveInstanceState(Bundle outState)
    {
        super.onSaveInstanceState(outState);
        outState.putInt("SystemUiVisibility", m_delegate.systemUiVisibility());
        outState.putBoolean("Started", m_delegate.isStarted());
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            m_delegate.updateFullScreen();
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
        QtNative.onNewIntent(intent);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
        QtNative.onActivityResult(requestCode, resultCode, data);
    }

    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        QtNative.sendRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    QtActivityDelegate getActivityDelegate()
    {
        return m_delegate;
    }
}
