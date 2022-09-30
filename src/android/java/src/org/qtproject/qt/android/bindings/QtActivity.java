// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android.bindings;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources.Theme;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Browser;
import android.util.AttributeSet;
import android.view.ActionMode;
import android.view.ActionMode.Callback;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager.LayoutParams;
import android.view.accessibility.AccessibilityEvent;

import org.qtproject.qt.android.QtNative;

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
    //---------------------------------------------------------------------------

    @Override
    public boolean dispatchPopulateAccessibilityEvent(AccessibilityEvent event)
    {
        if (QtLoader.m_delegateObject != null && QtLoader.dispatchPopulateAccessibilityEvent != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.dispatchPopulateAccessibilityEvent, event);
        else
            return super.dispatchPopulateAccessibilityEvent(event);
    }
    public boolean super_dispatchPopulateAccessibilityEvent(AccessibilityEvent event)
    {
        return super.dispatchPopulateAccessibilityEvent(event);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev)
    {
        if (QtLoader.m_delegateObject != null && QtLoader.dispatchTouchEvent != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.dispatchTouchEvent, ev);
        else
            return super.dispatchTouchEvent(ev);
    }
    public boolean super_dispatchTouchEvent(MotionEvent event)
    {
        return super.dispatchTouchEvent(event);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean dispatchTrackballEvent(MotionEvent ev)
    {
        if (QtLoader.m_delegateObject != null && QtLoader.dispatchTrackballEvent != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.dispatchTrackballEvent, ev);
        else
            return super.dispatchTrackballEvent(ev);
    }
    public boolean super_dispatchTrackballEvent(MotionEvent event)
    {
        return super.dispatchTrackballEvent(event);
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    @Override
    protected void onApplyThemeResource(Theme theme, int resid, boolean first)
    {
        if (!QtLoader.invokeDelegate(theme, resid, first).invoked)
            super.onApplyThemeResource(theme, resid, first);
    }
    public void super_onApplyThemeResource(Theme theme, int resid, boolean first)
    {
        super.onApplyThemeResource(theme, resid, first);
    }
    //---------------------------------------------------------------------------


    @Override
    protected void onChildTitleChanged(Activity childActivity, CharSequence title)
    {
        if (!QtLoader.invokeDelegate(childActivity, title).invoked)
            super.onChildTitleChanged(childActivity, title);
    }
    public void super_onChildTitleChanged(Activity childActivity, CharSequence title)
    {
        super.onChildTitleChanged(childActivity, title);
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    @Override
    public void onContentChanged()
    {
        if (!QtLoader.invokeDelegate().invoked)
            super.onContentChanged();
    }
    public void super_onContentChanged()
    {
        super.onContentChanged();
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    @Override
    public CharSequence onCreateDescription()
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate();
        if (res.invoked)
            return (CharSequence)res.methodReturns;
        else
            return super.onCreateDescription();
    }
    public CharSequence super_onCreateDescription()
    {
        return super.onCreateDescription();
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    @Override
    public boolean onCreatePanelMenu(int featureId, Menu menu)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(featureId, menu);
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onCreatePanelMenu(featureId, menu);
    }
    public boolean super_onCreatePanelMenu(int featureId, Menu menu)
    {
        return super.onCreatePanelMenu(featureId, menu);
    }
    //---------------------------------------------------------------------------


    @Override
    public View onCreatePanelView(int featureId)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(featureId);
        if (res.invoked)
            return (View)res.methodReturns;
        else
            return super.onCreatePanelView(featureId);
    }
    public View super_onCreatePanelView(int featureId)
    {
        return super.onCreatePanelView(featureId);
    }

    //---------------------------------------------------------------------------

    @Override
    public View onCreateView(String name, Context context, AttributeSet attrs)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(name, context, attrs);
        if (res.invoked)
            return (View)res.methodReturns;
        else
            return super.onCreateView(name, context, attrs);
    }
    public View super_onCreateView(String name, Context context, AttributeSet attrs)
    {
        return super.onCreateView(name, context, attrs);
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        QtLoader.invokeDelegate();
    }
    //---------------------------------------------------------------------------


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
    //---------------------------------------------------------------------------


    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event)
    {
        if (QtLoader.m_delegateObject != null && QtLoader.onKeyMultiple != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.onKeyMultiple, keyCode, repeatCount, event);
        else
            return super.onKeyMultiple(keyCode, repeatCount, event);
    }
    public boolean super_onKeyMultiple(int keyCode, int repeatCount, KeyEvent event)
    {
        return super.onKeyMultiple(keyCode, repeatCount, event);
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    @Override
    public void onLowMemory()
    {
        if (!QtLoader.invokeDelegate().invoked)
            super.onLowMemory();
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(featureId, item);
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onMenuItemSelected(featureId, item);
    }
    public boolean super_onMenuItemSelected(int featureId, MenuItem item)
    {
        return super.onMenuItemSelected(featureId, item);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onMenuOpened(int featureId, Menu menu)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(featureId, menu);
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onMenuOpened(featureId, menu);
    }
    public boolean super_onMenuOpened(int featureId, Menu menu)
    {
        return super.onMenuOpened(featureId, menu);
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    @Override
    public void onPanelClosed(int featureId, Menu menu)
    {
        if (!QtLoader.invokeDelegate(featureId, menu).invoked)
            super.onPanelClosed(featureId, menu);
    }
    public void super_onPanelClosed(int featureId, Menu menu)
    {
        super.onPanelClosed(featureId, menu);
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onPause()
    {
        super.onPause();
        QtLoader.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onPostCreate(Bundle savedInstanceState)
    {
        super.onPostCreate(savedInstanceState);
        QtLoader.invokeDelegate(savedInstanceState);
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onPostResume()
    {
        super.onPostResume();
        QtLoader.invokeDelegate();
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    @Override
    public boolean onPreparePanel(int featureId, View view, Menu menu)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(featureId, view, menu);
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onPreparePanel(featureId, view, menu);
    }
    public boolean super_onPreparePanel(int featureId, View view, Menu menu)
    {
        return super.onPreparePanel(featureId, view, menu);
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onRestart()
    {
        super.onRestart();
        QtLoader.invokeDelegate();
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    @Override
    protected void onResume()
    {
        super.onResume();
        QtLoader.invokeDelegate();
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    @Override
    public boolean onSearchRequested()
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate();
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onSearchRequested();
    }
    public boolean super_onSearchRequested()
    {
        return super.onSearchRequested();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onStart()
    {
        super.onStart();
        QtLoader.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onStop()
    {
        super.onStop();
        QtLoader.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onTitleChanged(CharSequence title, int color)
    {
        if (!QtLoader.invokeDelegate(title, color).invoked)
            super.onTitleChanged(title, color);
    }
    public void super_onTitleChanged(CharSequence title, int color)
    {
        super.onTitleChanged(title, color);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        if (QtLoader.m_delegateObject != null  && QtLoader.onTouchEvent != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.onTouchEvent, event);
        else
            return super.onTouchEvent(event);
    }
    public boolean super_onTouchEvent(MotionEvent event)
    {
        return super.onTouchEvent(event);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onTrackballEvent(MotionEvent event)
    {
        if (QtLoader.m_delegateObject != null  && QtLoader.onTrackballEvent != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.onTrackballEvent, event);
        else
            return super.onTrackballEvent(event);
    }
    public boolean super_onTrackballEvent(MotionEvent event)
    {
        return super.onTrackballEvent(event);
    }
    //---------------------------------------------------------------------------

    @Override
    public void onUserInteraction()
    {
        if (!QtLoader.invokeDelegate().invoked)
            super.onUserInteraction();
    }
    public void super_onUserInteraction()
    {
        super.onUserInteraction();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onUserLeaveHint()
    {
        if (!QtLoader.invokeDelegate().invoked)
            super.onUserLeaveHint();
    }
    public void super_onUserLeaveHint()
    {
        super.onUserLeaveHint();
    }
    //---------------------------------------------------------------------------

    @Override
    public void onWindowAttributesChanged(LayoutParams params)
    {
        if (!QtLoader.invokeDelegate(params).invoked)
            super.onWindowAttributesChanged(params);
    }
    public void super_onWindowAttributesChanged(LayoutParams params)
    {
        super.onWindowAttributesChanged(params);
    }
    //---------------------------------------------------------------------------

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
    //---------------------------------------------------------------------------

    //////////////// Activity API 5 /////////////
//@ANDROID-5
    @Override
    public void onAttachedToWindow()
    {
        if (!QtLoader.invokeDelegate().invoked)
            super.onAttachedToWindow();
    }
    public void super_onAttachedToWindow()
    {
        super.onAttachedToWindow();
    }
    //---------------------------------------------------------------------------

    @Override
    public void onBackPressed()
    {
        if (!QtLoader.invokeDelegate().invoked)
            super.onBackPressed();
    }
    public void super_onBackPressed()
    {
        super.onBackPressed();
    }
    //---------------------------------------------------------------------------

    @Override
    public void onDetachedFromWindow()
    {
        if (!QtLoader.invokeDelegate().invoked)
            super.onDetachedFromWindow();
    }
    public void super_onDetachedFromWindow()
    {
        super.onDetachedFromWindow();
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event)
    {
        if (QtLoader.m_delegateObject != null  && QtLoader.onKeyLongPress != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.onKeyLongPress, keyCode, event);
        else
            return super.onKeyLongPress(keyCode, event);
    }
    public boolean super_onKeyLongPress(int keyCode, KeyEvent event)
    {
        return super.onKeyLongPress(keyCode, event);
    }
    //---------------------------------------------------------------------------
//@ANDROID-5

    //////////////// Activity API 11 /////////////

//@ANDROID-11
    @Override
    public boolean dispatchKeyShortcutEvent(KeyEvent event)
    {
        if (QtLoader.m_delegateObject != null  && QtLoader.dispatchKeyShortcutEvent != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.dispatchKeyShortcutEvent, event);
        else
            return super.dispatchKeyShortcutEvent(event);
    }
    public boolean super_dispatchKeyShortcutEvent(KeyEvent event)
    {
        return super.dispatchKeyShortcutEvent(event);
    }
    //---------------------------------------------------------------------------

    @Override
    public void onActionModeFinished(ActionMode mode)
    {
        if (!QtLoader.invokeDelegate(mode).invoked)
            super.onActionModeFinished(mode);
    }
    public void super_onActionModeFinished(ActionMode mode)
    {
        super.onActionModeFinished(mode);
    }
    //---------------------------------------------------------------------------

    @Override
    public void onActionModeStarted(ActionMode mode)
    {
        if (!QtLoader.invokeDelegate(mode).invoked)
            super.onActionModeStarted(mode);
    }
    public void super_onActionModeStarted(ActionMode mode)
    {
        super.onActionModeStarted(mode);
    }
    //---------------------------------------------------------------------------

    @Override
    public View onCreateView(View parent, String name, Context context, AttributeSet attrs)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(parent, name, context, attrs);
        if (res.invoked)
            return (View)res.methodReturns;
        else
            return super.onCreateView(parent, name, context, attrs);
    }
    public View super_onCreateView(View parent, String name, Context context,
            AttributeSet attrs) {
        return super.onCreateView(parent, name, context, attrs);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onKeyShortcut(int keyCode, KeyEvent event)
    {
        if (QtLoader.m_delegateObject != null  && QtLoader.onKeyShortcut != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.onKeyShortcut, keyCode,event);
        else
            return super.onKeyShortcut(keyCode, event);
    }
    public boolean super_onKeyShortcut(int keyCode, KeyEvent event)
    {
        return super.onKeyShortcut(keyCode, event);
    }
    //---------------------------------------------------------------------------

    @Override
    public ActionMode onWindowStartingActionMode(Callback callback)
    {
        QtLoader.InvokeResult res = QtLoader.invokeDelegate(callback);
        if (res.invoked)
            return (ActionMode)res.methodReturns;
        else
            return super.onWindowStartingActionMode(callback);
    }
    public ActionMode super_onWindowStartingActionMode(Callback callback)
    {
        return super.onWindowStartingActionMode(callback);
    }
    //---------------------------------------------------------------------------
//@ANDROID-11
    //////////////// Activity API 12 /////////////

//@ANDROID-12
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
    //---------------------------------------------------------------------------

    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
    {
        if (QtLoader.m_delegateObject != null  && QtLoader.onGenericMotionEvent != null)
            return (Boolean) QtLoader.invokeDelegateMethod(QtLoader.onGenericMotionEvent, event);
        else
            return super.onGenericMotionEvent(event);
    }
    public boolean super_onGenericMotionEvent(MotionEvent event)
    {
        return super.onGenericMotionEvent(event);
    }
    //---------------------------------------------------------------------------
//@ANDROID-12

    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        if (QtLoader.m_delegateObject != null && QtLoader.onRequestPermissionsResult != null) {
            QtLoader.invokeDelegateMethod(QtLoader.onRequestPermissionsResult, requestCode , permissions, grantResults);
            return;
        }
    }
}
