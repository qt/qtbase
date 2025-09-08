// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
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
import android.view.Window;
import java.lang.IllegalArgumentException;

import android.widget.Toast;

public class QtActivityBase extends Activity
{
    public static final String EXTRA_SOURCE_INFO = "org.qtproject.qt.android.sourceInfo";

    private String m_applicationParams = "";
    private boolean m_isCustomThemeSet = false;
    private boolean m_retainNonConfigurationInstance = false;
    private Configuration m_prevConfig;
    private final QtActivityDelegate m_delegate;
    private boolean m_onCreateSucceeded = false;

    private void addReferrer(Intent intent)
    {
        Bundle extras = intent.getExtras();
        if (extras != null && extras.getString(EXTRA_SOURCE_INFO) != null)
            return;

        if (extras == null) {
            Uri referrer = getReferrer();
            if (referrer != null) {
                String cleanReferrer = referrer.toString().replaceFirst("android-app://", "");
                intent.putExtra(EXTRA_SOURCE_INFO, cleanReferrer);
            }
        } else {
            String applicationId = extras.getString(Browser.EXTRA_APPLICATION_ID);
            if (applicationId != null)
                intent.putExtra(EXTRA_SOURCE_INFO, applicationId);
        }
    }

    // Append any parameters to your application.
    // Either a whitespace or a tab is accepted as a separator between parameters.
    /**unused*/
    @SuppressWarnings("unused")
    public void appendApplicationParameters(String params)
    {
        if (params == null || params.isEmpty())
            return;

        if (!m_applicationParams.isEmpty())
            m_applicationParams += " ";
        m_applicationParams += params;
    }

    @Override
    public void setTheme(int resId) {
        super.setTheme(resId);
        m_isCustomThemeSet = true;
    }

    private void restartApplication() {
        Intent intent = Intent.makeRestartActivityTask(getComponentName());
        startActivity(intent);
        QtNative.quitApp();
        Runtime.getRuntime().exit(0);
    }

    public QtActivityBase()
    {
        m_delegate = new QtActivityDelegate(this);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_ACTION_BAR);

        if (!m_isCustomThemeSet) {
            setTheme(Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q ?
                    android.R.style.Theme_DeviceDefault_DayNight :
                    android.R.style.Theme_Holo_Light);
        }

        if (QtNative.getStateDetails().isStarted) {
            // We don't yet have a reliable way to keep the app
            // running properly in case of an Activity only restart,
            // so for now restart the whole app.
            restartApplication();
        }

        QtNative.registerAppStateListener(m_delegate);
        addReferrer(getIntent());

        try {
            QtActivityLoader loader = QtActivityLoader.getActivityLoader(this);
            loader.appendApplicationParameters(m_applicationParams);

            QtLoader.LoadingResult result = loader.loadQtLibraries();
            if (result == QtLoader.LoadingResult.Succeeded) {
                m_delegate.startNativeApplication(loader.getApplicationParameters(),
                        loader.getMainLibraryPath());
            } else if (result == QtLoader.LoadingResult.Failed) {
                showFatalFinishingToast();
                return;
            }
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            showFatalFinishingToast();
            return;
        }

        m_prevConfig = new Configuration(getResources().getConfiguration());
        m_onCreateSucceeded = true;
    }

    private void showFatalFinishingToast() {
        Resources resources = getResources();
        String packageName = getPackageName();
        @SuppressLint("DiscouragedApi") int id = resources.getIdentifier(
                "fatal_error_msg", "string", packageName);
        String message = resources.getString(id);
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();

        finish();
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        if (Build.VERSION.SDK_INT < 24 || !isInMultiWindowMode())
            QtNative.setApplicationState(QtNative.ApplicationState.ApplicationInactive);
        m_delegate.displayManager().unregisterDisplayListener();
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        QtNative.setApplicationState(QtNative.ApplicationState.ApplicationActive);
        if (QtNative.getStateDetails().isStarted) {
            m_delegate.displayManager().registerDisplayListener();
            QtWindow.updateWindows();
            // Suspending the app clears the immersive mode, so we need to set it again.
            m_delegate.displayManager().reinstateFullScreen();
        }
    }

    @Override
    protected void onStop()
    {
        super.onStop();
        QtNative.setApplicationState(QtNative.ApplicationState.ApplicationSuspended);
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();

        if (!m_onCreateSucceeded)
            System.exit(-1);

        if (!m_retainNonConfigurationInstance) {
            QtNative.unregisterAppStateListener(m_delegate);
            QtNative.terminateQt();
            QtNative.setActivity(null);
            QtNative.getQtThread().exit();
            System.exit(0);
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        super.onConfigurationChanged(newConfig);
        m_delegate.handleUiModeChange(newConfig.uiMode & Configuration.UI_MODE_NIGHT_MASK);

        int diff = newConfig.diff(m_prevConfig);
        if ((diff & ActivityInfo.CONFIG_LOCALE) != 0)
            QtNative.updateLocale();

        m_prevConfig = new Configuration(newConfig);
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
        boolean handleResult = m_delegate.getInputDelegate().handleDispatchKeyEvent(event);
        if (QtNative.getStateDetails().isStarted && handleResult)
            return true;

        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event)
    {
        boolean handled = m_delegate.getInputDelegate().handleDispatchGenericMotionEvent(event);
        if (QtNative.getStateDetails().isStarted && handled)
            return true;

        return super.dispatchGenericMotionEvent(event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        QtNative.ApplicationStateDetails stateDetails = QtNative.getStateDetails();
        if (!stateDetails.isStarted || !stateDetails.nativePluginIntegrationReady)
            return false;

        return m_delegate.getInputDelegate().onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        QtNative.ApplicationStateDetails stateDetails = QtNative.getStateDetails();
        if (!stateDetails.isStarted || !stateDetails.nativePluginIntegrationReady)
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
        QtNative.onOptionsMenuClosed(menu);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState)
    {
        super.onRestoreInstanceState(savedInstanceState);

        // only restore when this Activity is being recreated for a config change
        if (getLastNonConfigurationInstance() == null)
            return;

        QtNative.setStarted(savedInstanceState.getBoolean("Started"));
        boolean isFullScreen = savedInstanceState.getBoolean("isFullScreen");
        boolean expandedToCutout = savedInstanceState.getBoolean("expandedToCutout");
        m_delegate.displayManager().setSystemUiVisibility(isFullScreen, expandedToCutout);
        // FIXME restore all surfaces
    }

    @Override
    public Object onRetainNonConfigurationInstance()
    {
        super.onRetainNonConfigurationInstance();
        m_retainNonConfigurationInstance = true;
        return true;
    }

    @Override
    protected void onSaveInstanceState(Bundle outState)
    {
        super.onSaveInstanceState(outState);
        outState.putBoolean("isFullScreen", m_delegate.displayManager().isFullScreen());
        outState.putBoolean("expandedToCutout", m_delegate.displayManager().expandedToCutout());
        outState.putBoolean("Started", QtNative.getStateDetails().isStarted);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            m_delegate.displayManager().reinstateFullScreen();
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
        addReferrer(intent);
        QtNative.onNewIntent(intent);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
        QtNative.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        QtNative.sendRequestPermissionsResult(requestCode, grantResults);
    }

    @UsedFromNativeCode
    public void hideSplashScreen(final int duration)
    {
        m_delegate.hideSplashScreen(duration);
    }
}
