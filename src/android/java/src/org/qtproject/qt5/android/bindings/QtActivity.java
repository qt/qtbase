/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
    Contact: http://www.qt.io/licensing/

    Commercial License Usage
    Licensees holding valid commercial Qt licenses may use this file in
    accordance with the commercial license agreement provided with the
    Software or, alternatively, in accordance with the terms contained in
    a written agreement between you and The Qt Company. For licensing terms
    and conditions see http://www.qt.io/terms-conditions. For further
    information use the contact form at http://www.qt.io/contact-us.

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

import android.app.Activity;
import android.app.Dialog;
import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.res.Resources.Theme;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Build;
import android.os.Bundle;
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


public class QtActivity extends Activity
{
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
        m_loader = new QtActivityLoader(this);
        if (Build.VERSION.SDK_INT >= 21) {
            QT_ANDROID_THEMES = new String[] {"Theme_Holo_Light"};
            QT_ANDROID_DEFAULT_THEME = "Theme_Holo_Light";
        } else {
            QT_ANDROID_THEMES = new String[] {"Theme_DeviceDefault_Light"};
            QT_ANDROID_DEFAULT_THEME = "Theme_DeviceDefault_Light";
        }
    }


    /////////////////////////// forward all notifications ////////////////////////////
    /////////////////////////// Super class calls ////////////////////////////////////
    /////////////// PLEASE DO NOT CHANGE THE FOLLOWING CODE //////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

    @Override
    public boolean dispatchKeyEvent(KeyEvent event)
    {
        if (QtApplication.m_delegateObject != null && QtApplication.dispatchKeyEvent != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.dispatchKeyEvent, event);
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
        if (QtApplication.m_delegateObject != null && QtApplication.dispatchPopulateAccessibilityEvent != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.dispatchPopulateAccessibilityEvent, event);
        else
            return super.dispatchPopulateAccessibilityEvent(event);
    }
    public boolean super_dispatchPopulateAccessibilityEvent(AccessibilityEvent event)
    {
        return super_dispatchPopulateAccessibilityEvent(event);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev)
    {
        if (QtApplication.m_delegateObject != null && QtApplication.dispatchTouchEvent != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.dispatchTouchEvent, ev);
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
        if (QtApplication.m_delegateObject != null && QtApplication.dispatchTrackballEvent != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.dispatchTrackballEvent, ev);
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

        if (QtApplication.m_delegateObject != null && QtApplication.onActivityResult != null) {
            QtApplication.invokeDelegateMethod(QtApplication.onActivityResult, requestCode, resultCode, data);
            return;
        }
        if (requestCode == QtLoader.MINISTRO_INSTALL_REQUEST_CODE)
            m_loader.startApp(false);
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
        if (!QtApplication.invokeDelegate(theme, resid, first).invoked)
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
        if (!QtApplication.invokeDelegate(childActivity, title).invoked)
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
        if (!QtApplication.invokeDelegate(newConfig).invoked)
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
        if (!QtApplication.invokeDelegate().invoked)
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
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(item);
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
        if (!QtApplication.invokeDelegate(menu).invoked)
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

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        onCreateHook(savedInstanceState);
    }
    //---------------------------------------------------------------------------

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
    {
        if (!QtApplication.invokeDelegate(menu, v, menuInfo).invoked)
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
        QtApplication.InvokeResult res = QtApplication.invokeDelegate();
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
    protected Dialog onCreateDialog(int id)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(id);
        if (res.invoked)
            return (Dialog)res.methodReturns;
        else
            return super.onCreateDialog(id);
    }
    public Dialog super_onCreateDialog(int id)
    {
        return super.onCreateDialog(id);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(menu);
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
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(featureId, menu);
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
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(featureId);
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
    public boolean onCreateThumbnail(Bitmap outBitmap, Canvas canvas)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(outBitmap, canvas);
        if (res.invoked)
            return (Boolean)res.methodReturns;
        else
            return super.onCreateThumbnail(outBitmap, canvas);
    }
    public boolean super_onCreateThumbnail(Bitmap outBitmap, Canvas canvas)
    {
        return super.onCreateThumbnail(outBitmap, canvas);
    }
    //---------------------------------------------------------------------------

    @Override
    public View onCreateView(String name, Context context, AttributeSet attrs)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(name, context, attrs);
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
        QtApplication.invokeDelegate();
    }
    //---------------------------------------------------------------------------


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (QtApplication.m_delegateObject != null && QtApplication.onKeyDown != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.onKeyDown, keyCode, event);
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
        if (QtApplication.m_delegateObject != null && QtApplication.onKeyMultiple != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.onKeyMultiple, keyCode, repeatCount, event);
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
        if (QtApplication.m_delegateObject != null  && QtApplication.onKeyUp != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.onKeyUp, keyCode, event);
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
        if (!QtApplication.invokeDelegate().invoked)
            super.onLowMemory();
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(featureId, item);
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
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(featureId, menu);
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
        if (!QtApplication.invokeDelegate(intent).invoked)
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
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(item);
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
        if (!QtApplication.invokeDelegate(menu).invoked)
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
        if (!QtApplication.invokeDelegate(featureId, menu).invoked)
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
        QtApplication.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onPostCreate(Bundle savedInstanceState)
    {
        super.onPostCreate(savedInstanceState);
        QtApplication.invokeDelegate(savedInstanceState);
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onPostResume()
    {
        super.onPostResume();
        QtApplication.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onPrepareDialog(int id, Dialog dialog)
    {
        if (!QtApplication.invokeDelegate(id, dialog).invoked)
            super.onPrepareDialog(id, dialog);
    }
    public void super_onPrepareDialog(int id, Dialog dialog)
    {
        super.onPrepareDialog(id, dialog);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onPrepareOptionsMenu(Menu menu)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(menu);
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
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(featureId, view, menu);
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
        QtApplication.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState)
    {
        if (!QtApplication.invokeDelegate(savedInstanceState).invoked)
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
        QtApplication.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    public Object onRetainNonConfigurationInstance()
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate();
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
        if (!QtApplication.invokeDelegate(outState).invoked)
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
        QtApplication.InvokeResult res = QtApplication.invokeDelegate();
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
        QtApplication.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onStop()
    {
        super.onStop();
        QtApplication.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onTitleChanged(CharSequence title, int color)
    {
        if (!QtApplication.invokeDelegate(title, color).invoked)
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
        if (QtApplication.m_delegateObject != null  && QtApplication.onTouchEvent != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.onTouchEvent, event);
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
        if (QtApplication.m_delegateObject != null  && QtApplication.onTrackballEvent != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.onTrackballEvent, event);
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
        if (!QtApplication.invokeDelegate().invoked)
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
        if (!QtApplication.invokeDelegate().invoked)
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
        if (!QtApplication.invokeDelegate(params).invoked)
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
        if (!QtApplication.invokeDelegate(hasFocus).invoked)
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
        if (!QtApplication.invokeDelegate().invoked)
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
        if (!QtApplication.invokeDelegate().invoked)
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
        if (!QtApplication.invokeDelegate().invoked)
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
        if (QtApplication.m_delegateObject != null  && QtApplication.onKeyLongPress != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.onKeyLongPress, keyCode, event);
        else
            return super.onKeyLongPress(keyCode, event);
    }
    public boolean super_onKeyLongPress(int keyCode, KeyEvent event)
    {
        return super.onKeyLongPress(keyCode, event);
    }
    //---------------------------------------------------------------------------
//@ANDROID-5

//////////////// Activity API 8 /////////////
//@ANDROID-8
@Override
    protected Dialog onCreateDialog(int id, Bundle args)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(id, args);
        if (res.invoked)
            return (Dialog)res.methodReturns;
        else
            return super.onCreateDialog(id, args);
    }
    public Dialog super_onCreateDialog(int id, Bundle args)
    {
        return super.onCreateDialog(id, args);
    }
    //---------------------------------------------------------------------------

    @Override
    protected void onPrepareDialog(int id, Dialog dialog, Bundle args)
    {
        if (!QtApplication.invokeDelegate(id, dialog, args).invoked)
            super.onPrepareDialog(id, dialog, args);
    }
    public void super_onPrepareDialog(int id, Dialog dialog, Bundle args)
    {
        super.onPrepareDialog(id, dialog, args);
    }
    //---------------------------------------------------------------------------
//@ANDROID-8
    //////////////// Activity API 11 /////////////

//@ANDROID-11
    @Override
    public boolean dispatchKeyShortcutEvent(KeyEvent event)
    {
        if (QtApplication.m_delegateObject != null  && QtApplication.dispatchKeyShortcutEvent != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.dispatchKeyShortcutEvent, event);
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
        if (!QtApplication.invokeDelegate(mode).invoked)
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
        if (!QtApplication.invokeDelegate(mode).invoked)
            super.onActionModeStarted(mode);
    }
    public void super_onActionModeStarted(ActionMode mode)
    {
        super.onActionModeStarted(mode);
    }
    //---------------------------------------------------------------------------

    @Override
    public void onAttachFragment(Fragment fragment)
    {
        if (!QtApplication.invokeDelegate(fragment).invoked)
            super.onAttachFragment(fragment);
    }
    public void super_onAttachFragment(Fragment fragment)
    {
        super.onAttachFragment(fragment);
    }
    //---------------------------------------------------------------------------

    @Override
    public View onCreateView(View parent, String name, Context context, AttributeSet attrs)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(parent, name, context, attrs);
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
        if (QtApplication.m_delegateObject != null  && QtApplication.onKeyShortcut != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.onKeyShortcut, keyCode,event);
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
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(callback);
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
        if (QtApplication.m_delegateObject != null  && QtApplication.dispatchGenericMotionEvent != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.dispatchGenericMotionEvent, ev);
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
        if (QtApplication.m_delegateObject != null  && QtApplication.onGenericMotionEvent != null)
            return (Boolean) QtApplication.invokeDelegateMethod(QtApplication.onGenericMotionEvent, event);
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
        if (QtApplication.m_delegateObject != null && QtApplication.onRequestPermissionsResult != null) {
            QtApplication.invokeDelegateMethod(QtApplication.onRequestPermissionsResult, requestCode , permissions, grantResults);
            return;
        }
    }
}
