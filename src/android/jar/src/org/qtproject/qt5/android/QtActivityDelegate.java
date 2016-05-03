/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.Rect;
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
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.view.ViewTreeObserver;
import android.widget.ImageView;
import android.widget.PopupMenu;

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

import org.qtproject.qt5.android.accessibility.QtAccessibilityDelegate;

public class QtActivityDelegate
{
    private Activity m_activity = null;
    private Method m_super_dispatchKeyEvent = null;
    private Method m_super_onRestoreInstanceState = null;
    private Method m_super_onRetainNonConfigurationInstance = null;
    private Method m_super_onSaveInstanceState = null;
    private Method m_super_onKeyDown = null;
    private Method m_super_onKeyUp = null;
    private Method m_super_onConfigurationChanged = null;
    private Method m_super_onActivityResult = null;
    private Method m_super_dispatchGenericMotionEvent = null;
    private Method m_super_onWindowFocusChanged = null;

    private static final String NATIVE_LIBRARIES_KEY = "native.libraries";
    private static final String BUNDLED_LIBRARIES_KEY = "bundled.libraries";
    private static final String MAIN_LIBRARY_KEY = "main.library";
    private static final String ENVIRONMENT_VARIABLES_KEY = "environment.variables";
    private static final String APPLICATION_PARAMETERS_KEY = "application.parameters";
    private static final String STATIC_INIT_CLASSES_KEY = "static.init.classes";
    private static final String NECESSITAS_API_LEVEL_KEY = "necessitas.api.level";
    private static final String EXTRACT_STYLE_KEY = "extract.android.style";
    private static final String EXTRACT_STYLE_MINIMAL_KEY = "extract.android.style.option";

    private static String m_environmentVariables = null;
    private static String m_applicationParameters = null;

    private int m_currentRotation = -1; // undefined
    private int m_nativeOrientation = Configuration.ORIENTATION_UNDEFINED;

    private String m_mainLib;
    private long m_metaState;
    private int m_lastChar = 0;
    private int m_softInputMode = 0;
    private boolean m_fullScreen = false;
    private boolean m_started = false;
    private HashMap<Integer, QtSurface> m_surfaces = null;
    private HashMap<Integer, View> m_nativeViews = null;
    private QtLayout m_layout = null;
    private ImageView m_splashScreen = null;
    private boolean m_splashScreenSticky = false;
    private QtEditText m_editText = null;
    private InputMethodManager m_imm = null;
    private boolean m_quitApp = true;
    private Process m_debuggerProcess = null; // debugger process
    private View m_dummyView = null;
    private boolean m_keyboardIsVisible = false;
    public boolean m_backKeyPressedSent = false;
    private long m_showHideTimeStamp = System.nanoTime();
    private int m_portraitKeyboardHeight = 0;
    private int m_landscapeKeyboardHeight = 0;
    private int m_probeKeyboardHeightDelay = 50; // ms

    public void setFullScreen(boolean enterFullScreen)
    {
        if (m_fullScreen == enterFullScreen)
            return;

        if (m_fullScreen = enterFullScreen) {
            m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
            try {
                if (Build.VERSION.SDK_INT >= 19) {
                    int flags = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
                    flags |= View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
                    flags |= View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
                    flags |= View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;
                    flags |= View.SYSTEM_UI_FLAG_FULLSCREEN;
                    flags |= View.class.getDeclaredField("SYSTEM_UI_FLAG_IMMERSIVE_STICKY").getInt(null);
                    m_activity.getWindow().getDecorView().setSystemUiVisibility(flags | View.INVISIBLE);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        } else {
            m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
            m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            m_activity.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
        }
        m_layout.requestLayout();
    }

    public void updateFullScreen()
    {
        if (m_fullScreen) {
            m_fullScreen = false;
            setFullScreen(true);
        }
    }

    // input method hints - must be kept in sync with QTDIR/src/corelib/global/qnamespace.h
    private final int ImhHiddenText = 0x1;
    private final int ImhSensitiveData = 0x2;
    private final int ImhNoAutoUppercase = 0x4;
    private final int ImhPreferNumbers = 0x8;
    private final int ImhPreferUppercase = 0x10;
    private final int ImhPreferLowercase = 0x20;
    private final int ImhNoPredictiveText = 0x40;

    private final int ImhDate = 0x80;
    private final int ImhTime = 0x100;

    private final int ImhPreferLatin = 0x200;

    private final int ImhMultiLine = 0x400;

    private final int ImhDigitsOnly = 0x10000;
    private final int ImhFormattedNumbersOnly = 0x20000;
    private final int ImhUppercaseOnly = 0x40000;
    private final int ImhLowercaseOnly = 0x80000;
    private final int ImhDialableCharactersOnly = 0x100000;
    private final int ImhEmailCharactersOnly = 0x200000;
    private final int ImhUrlCharactersOnly = 0x400000;
    private final int ImhLatinOnly = 0x800000;

    // enter key type - must be kept in sync with QTDIR/src/corelib/global/qnamespace.h
    private final int EnterKeyDefault = 0;
    private final int EnterKeyReturn = 1;
    private final int EnterKeyDone = 2;
    private final int EnterKeyGo = 3;
    private final int EnterKeySend = 4;
    private final int EnterKeySearch = 5;
    private final int EnterKeyNext = 6;
    private final int EnterKeyPrevious = 7;

    // application state
    public static final int ApplicationSuspended = 0x0;
    public static final int ApplicationHidden = 0x1;
    public static final int ApplicationInactive = 0x2;
    public static final int ApplicationActive = 0x4;


    public boolean setKeyboardVisibility(boolean visibility, long timeStamp)
    {
        if (m_showHideTimeStamp > timeStamp)
            return false;
        m_showHideTimeStamp = timeStamp;

        if (m_keyboardIsVisible == visibility)
            return false;
        m_keyboardIsVisible = visibility;
        QtNative.keyboardVisibilityChanged(m_keyboardIsVisible);

        if (visibility == false)
            updateFullScreen(); // Hiding the keyboard clears the immersive mode, so we need to set it again.

        return true;
    }
    public void resetSoftwareKeyboard()
    {
        if (m_imm == null)
            return;
        m_editText.postDelayed(new Runnable() {
            @Override
            public void run() {
                m_imm.restartInput(m_editText);
                m_editText.m_optionsChanged = false;
            }
        }, 5);
    }

    public void showSoftwareKeyboard(final int x, final int y, final int width, final int height, final int inputHints, final int enterKeyType)
    {
        if (m_imm == null)
            return;

        DisplayMetrics metrics = new DisplayMetrics();
        m_activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        // If the screen is in portrait mode than we estimate that keyboard height will not be higher than 2/5 of the screen.
        // else than we estimate that keyboard height will not be higher than 2/3 of the screen
        final int visibleHeight;
        if (metrics.widthPixels < metrics.heightPixels)
            visibleHeight = m_portraitKeyboardHeight != 0 ? m_portraitKeyboardHeight : metrics.heightPixels * 3 / 5;
        else
            visibleHeight = m_landscapeKeyboardHeight != 0 ? m_landscapeKeyboardHeight : metrics.heightPixels / 3;

        if (m_softInputMode != 0) {
            m_activity.getWindow().setSoftInputMode(m_softInputMode);
        } else {
            if (height > visibleHeight)
                m_activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_UNCHANGED | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
            else
                m_activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_UNCHANGED | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        }

        int initialCapsMode = 0;

        int imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_DONE;

        switch (enterKeyType) {
        case EnterKeyReturn:
            imeOptions = android.view.inputmethod.EditorInfo.IME_FLAG_NO_ENTER_ACTION;
            break;
        case EnterKeyGo:
            imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_GO;
            break;
        case EnterKeySend:
            imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_SEND;
            break;
        case EnterKeySearch:
            imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_SEARCH;
            break;
        case EnterKeyNext:
            imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_NEXT;
            break;
        case EnterKeyPrevious:
            imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_PREVIOUS;
            break;
        }

        int inputType = android.text.InputType.TYPE_CLASS_TEXT;

        if ((inputHints & (ImhPreferNumbers | ImhDigitsOnly | ImhFormattedNumbersOnly)) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_NUMBER;
            if ((inputHints & ImhFormattedNumbersOnly) != 0) {
                inputType |= (android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL
                              | android.text.InputType.TYPE_NUMBER_FLAG_SIGNED);
            }

            if ((inputHints & ImhHiddenText) != 0)
                inputType |= 0x10 /* TYPE_NUMBER_VARIATION_PASSWORD */;
        } else if ((inputHints & ImhDialableCharactersOnly) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_PHONE;
        } else if ((inputHints & (ImhDate | ImhTime)) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_DATETIME;
            if ((inputHints & (ImhDate | ImhTime)) != (ImhDate | ImhTime)) {
                if ((inputHints & ImhDate) != 0)
                    inputType |= android.text.InputType.TYPE_DATETIME_VARIATION_DATE;
                if ((inputHints & ImhTime) != 0)
                    inputType |= android.text.InputType.TYPE_DATETIME_VARIATION_TIME;
            } // else {  TYPE_DATETIME_VARIATION_NORMAL(0) }
        } else { // CLASS_TEXT
            if ((inputHints & (ImhEmailCharactersOnly | ImhUrlCharactersOnly)) != 0) {
                if ((inputHints & ImhUrlCharactersOnly) != 0) {
                    inputType |= android.text.InputType.TYPE_TEXT_VARIATION_URI;

                    if (enterKeyType == 0) // not explicitly overridden
                        imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_GO;
                } else if ((inputHints & ImhEmailCharactersOnly) != 0) {
                    inputType |= android.text.InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
                }
            } else if ((inputHints & ImhHiddenText) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD;
            } else if ((inputHints & ImhSensitiveData) != 0 || (inputHints & ImhNoPredictiveText) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD;
            }

            if ((inputHints & ImhMultiLine) != 0)
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_MULTI_LINE;

            if ((inputHints & ImhUppercaseOnly) != 0) {
                initialCapsMode |= android.text.TextUtils.CAP_MODE_CHARACTERS;
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS;
            } else if ((inputHints & ImhLowercaseOnly) == 0 && (inputHints & ImhNoAutoUppercase) == 0) {
                initialCapsMode |= android.text.TextUtils.CAP_MODE_SENTENCES;
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_CAP_SENTENCES;
            }

            if ((inputHints & ImhNoPredictiveText) != 0 || (inputHints & ImhSensitiveData) != 0
                || (inputHints & ImhHiddenText) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
            }
        }

        if (enterKeyType == 0 && (inputHints & ImhMultiLine) != 0)
            imeOptions = android.view.inputmethod.EditorInfo.IME_FLAG_NO_ENTER_ACTION;

        m_editText.setInitialCapsMode(initialCapsMode);
        m_editText.setImeOptions(imeOptions);
        m_editText.setInputType(inputType);

        m_layout.setLayoutParams(m_editText, new QtLayout.LayoutParams(width, height, x, y), false);
        m_editText.requestFocus();
        m_editText.postDelayed(new Runnable() {
            @Override
            public void run() {
                m_imm.showSoftInput(m_editText, 0, new ResultReceiver(new Handler()) {
                    @Override
                    protected void onReceiveResult(int resultCode, Bundle resultData) {
                        switch (resultCode) {
                            case InputMethodManager.RESULT_SHOWN:
                                QtNativeInputConnection.updateCursorPosition();
                                //FALLTHROUGH
                            case InputMethodManager.RESULT_UNCHANGED_SHOWN:
                                setKeyboardVisibility(true, System.nanoTime());
                                if (m_softInputMode == 0) {
                                    // probe for real keyboard height
                                    m_layout.postDelayed(new Runnable() {
                                            @Override
                                            public void run() {
                                                if (!m_keyboardIsVisible)
                                                    return;
                                                DisplayMetrics metrics = new DisplayMetrics();
                                                m_activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
                                                Rect r = new Rect();
                                                m_activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
                                                if (metrics.heightPixels != r.bottom) {
                                                    if (metrics.widthPixels > metrics.heightPixels) { // landscape
                                                        if (m_landscapeKeyboardHeight != r.bottom) {
                                                            m_landscapeKeyboardHeight = r.bottom;
                                                            showSoftwareKeyboard(x, y, width, height, inputHints, enterKeyType);
                                                        }
                                                    } else {
                                                        if (m_portraitKeyboardHeight != r.bottom) {
                                                            m_portraitKeyboardHeight = r.bottom;
                                                            showSoftwareKeyboard(x, y, width, height, inputHints, enterKeyType);
                                                        }
                                                    }
                                                } else {
                                                    // no luck ?
                                                    // maybe the delay was too short, so let's make it longer
                                                    if (m_probeKeyboardHeightDelay < 1000)
                                                        m_probeKeyboardHeightDelay *= 2;
                                                }
                                            }
                                        }, m_probeKeyboardHeightDelay);
                                    }
                                break;
                            case InputMethodManager.RESULT_HIDDEN:
                            case InputMethodManager.RESULT_UNCHANGED_HIDDEN:
                                setKeyboardVisibility(false, System.nanoTime());
                                break;
                        }
                    }
                });
                if (m_editText.m_optionsChanged) {
                    m_imm.restartInput(m_editText);
                    m_editText.m_optionsChanged = false;
                }
            }
        }, 15);
    }

    public void hideSoftwareKeyboard()
    {
        if (m_imm == null)
            return;
        m_imm.hideSoftInputFromWindow(m_editText.getWindowToken(), 0, new ResultReceiver(new Handler()) {
            @Override
            protected void onReceiveResult(int resultCode, Bundle resultData) {
                switch (resultCode) {
                    case InputMethodManager.RESULT_SHOWN:
                    case InputMethodManager.RESULT_UNCHANGED_SHOWN:
                        setKeyboardVisibility(true, System.nanoTime());
                        break;
                    case InputMethodManager.RESULT_HIDDEN:
                    case InputMethodManager.RESULT_UNCHANGED_HIDDEN:
                        setKeyboardVisibility(false, System.nanoTime());
                        break;
                }
            }
        });
    }

    String getAppIconSize(Activity a)
    {
        int size = a.getResources().getDimensionPixelSize(android.R.dimen.app_icon_size);
        if (size < 36 || size > 512) { // check size sanity
            DisplayMetrics metrics = new DisplayMetrics();
            a.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            size = metrics.densityDpi / 10 * 3;
            if (size < 36)
                size = 36;

            if (size > 512)
                size = 512;
        }
        return "\tQT_ANDROID_APP_ICON_SIZE=" + size;
    }

    public void updateSelection(int selStart, int selEnd, int candidatesStart, int candidatesEnd)
    {
        if (m_imm == null)
            return;

        m_imm.updateSelection(m_editText, selStart, selEnd, candidatesStart, candidatesEnd);
    }

    public boolean loadApplication(Activity activity, ClassLoader classLoader, Bundle loaderParams)
    {
        /// check parameters integrity
        if (!loaderParams.containsKey(NATIVE_LIBRARIES_KEY)
                || !loaderParams.containsKey(BUNDLED_LIBRARIES_KEY)
                || !loaderParams.containsKey(ENVIRONMENT_VARIABLES_KEY)) {
            return false;
        }

        m_activity = activity;
        setActionBarVisibility(false);
        QtNative.setActivity(m_activity, this);
        QtNative.setClassLoader(classLoader);
        if (loaderParams.containsKey(STATIC_INIT_CLASSES_KEY)) {
            for (String className: loaderParams.getStringArray(STATIC_INIT_CLASSES_KEY)) {
                if (className.length() == 0)
                    continue;

                try {
                    @SuppressWarnings("rawtypes")
                    Class<?> initClass = classLoader.loadClass(className);
                    Object staticInitDataObject = initClass.newInstance(); // create an instance
                    Method m = initClass.getMethod("setActivity", Activity.class, Object.class);
                    m.invoke(staticInitDataObject, m_activity, this);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        QtNative.loadQtLibraries(loaderParams.getStringArrayList(NATIVE_LIBRARIES_KEY));
        ArrayList<String> libraries = loaderParams.getStringArrayList(BUNDLED_LIBRARIES_KEY);
        QtNative.loadBundledLibraries(libraries, QtNativeLibrariesDir.nativeLibrariesDir(m_activity));
        m_mainLib = loaderParams.getString(MAIN_LIBRARY_KEY);
        // older apps provide the main library as the last bundled library; look for this if the main library isn't provided
        if (null == m_mainLib && libraries.size() > 0)
            m_mainLib = libraries.get(libraries.size() - 1);

        if (loaderParams.containsKey(EXTRACT_STYLE_KEY)) {
            String path = loaderParams.getString(EXTRACT_STYLE_KEY);
            new ExtractStyle(m_activity, path, loaderParams.containsKey(EXTRACT_STYLE_MINIMAL_KEY) &&
                                               loaderParams.getBoolean(EXTRACT_STYLE_MINIMAL_KEY));
        }

        try {
            m_super_dispatchKeyEvent = m_activity.getClass().getMethod("super_dispatchKeyEvent", KeyEvent.class);
            m_super_onRestoreInstanceState = m_activity.getClass().getMethod("super_onRestoreInstanceState", Bundle.class);
            m_super_onRetainNonConfigurationInstance = m_activity.getClass().getMethod("super_onRetainNonConfigurationInstance");
            m_super_onSaveInstanceState = m_activity.getClass().getMethod("super_onSaveInstanceState", Bundle.class);
            m_super_onKeyDown = m_activity.getClass().getMethod("super_onKeyDown", Integer.TYPE, KeyEvent.class);
            m_super_onKeyUp = m_activity.getClass().getMethod("super_onKeyUp", Integer.TYPE, KeyEvent.class);
            m_super_onConfigurationChanged = m_activity.getClass().getMethod("super_onConfigurationChanged", Configuration.class);
            m_super_onActivityResult = m_activity.getClass().getMethod("super_onActivityResult", Integer.TYPE, Integer.TYPE, Intent.class);
            m_super_onWindowFocusChanged = m_activity.getClass().getMethod("super_onWindowFocusChanged", Boolean.TYPE);
            m_super_dispatchGenericMotionEvent = m_activity.getClass().getMethod("super_dispatchGenericMotionEvent", MotionEvent.class);
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }

        int necessitasApiLevel = 1;
        if (loaderParams.containsKey(NECESSITAS_API_LEVEL_KEY))
            necessitasApiLevel = loaderParams.getInt(NECESSITAS_API_LEVEL_KEY);

        m_environmentVariables = loaderParams.getString(ENVIRONMENT_VARIABLES_KEY);
        String additionalEnvironmentVariables = "QT_ANDROID_FONTS_MONOSPACE=Droid Sans Mono;Droid Sans;Droid Sans Fallback"
                                              + "\tQT_ANDROID_FONTS_SERIF=Droid Serif"
                                              + "\tNECESSITAS_API_LEVEL=" + necessitasApiLevel
                                              + "\tHOME=" + m_activity.getFilesDir().getAbsolutePath()
                                              + "\tTMPDIR=" + m_activity.getFilesDir().getAbsolutePath();

        additionalEnvironmentVariables += "\tQT_ANDROID_FONTS=Roboto;Droid Sans;Droid Sans Fallback";

        additionalEnvironmentVariables += getAppIconSize(activity);

        if (m_environmentVariables != null && m_environmentVariables.length() > 0)
            m_environmentVariables = additionalEnvironmentVariables + "\t" + m_environmentVariables;
        else
            m_environmentVariables = additionalEnvironmentVariables;

        if (loaderParams.containsKey(APPLICATION_PARAMETERS_KEY))
            m_applicationParameters = loaderParams.getString(APPLICATION_PARAMETERS_KEY);
        else
            m_applicationParameters = "";

        try {
            m_softInputMode = m_activity.getPackageManager().getActivityInfo(m_activity.getComponentName(), 0).softInputMode;
        } catch (Exception e) {
            e.printStackTrace();
        }

        return true;
    }

    public static void debugLog(String msg)
    {
        Log.i(QtNative.QtTAG, "DEBUGGER: " + msg);
    }

    private class DebugWaitRunnable implements Runnable {

        public DebugWaitRunnable(String pingPongSocket) throws  IOException {
            socket = new LocalServerSocket(pingPongSocket);
        }

        public boolean wasFailure;
        private LocalServerSocket socket;

        public void run() {
            final int napTime = 200; // milliseconds between file accesses
            final int timeOut = 30000; // ms until we give up on ping and pong
            final int maxAttempts = timeOut / napTime;

            try {
                LocalSocket connectionFromClient = socket.accept();
                debugLog("Debug socket accepted");
                BufferedReader inFromClient =
                        new BufferedReader(new InputStreamReader(connectionFromClient.getInputStream()));
                DataOutputStream outToClient = new DataOutputStream(connectionFromClient.getOutputStream());
                outToClient.writeBytes("" + android.os.Process.myPid());

                for (int i = 0; i < maxAttempts; i++) {
                    String clientData = inFromClient.readLine();
                    debugLog("Incoming socket " + clientData);
                    if (!clientData.isEmpty())
                        break;

                    if (connectionFromClient.isClosed()) {
                        wasFailure = true;
                        break;
                    }
                    Thread.sleep(napTime);
                }
            } catch (IOException ioEx) {
                ioEx.printStackTrace();
                wasFailure = true;
                Log.e(QtNative.QtTAG,"Can't start debugger" + ioEx.getMessage());
            } catch (InterruptedException interruptEx) {
                wasFailure = true;
                Log.e(QtNative.QtTAG,"Can't start debugger" + interruptEx.getMessage());
            }
        }

        public void shutdown() throws IOException
        {
            wasFailure = true;
            try {
                socket.close();
            } catch (IOException ignored) { }
        }
    };

    public boolean startApplication()
    {
        // start application
        try {
            // FIXME turn on debuggable check
            // if the applications is debuggable and it has a native debug request
            Bundle extras = m_activity.getIntent().getExtras();
            if (extras != null) {

                if ( /*(ai.flags&ApplicationInfo.FLAG_DEBUGGABLE) != 0
                        &&*/ extras.containsKey("native_debug")
                        && extras.getString("native_debug").equals("true")) {
                    try {
                        String packagePath =
                            m_activity.getPackageManager().getApplicationInfo(m_activity.getPackageName(),
                                                                              PackageManager.GET_CONFIGURATIONS).dataDir + "/";
                        String gdbserverPath =
                            extras.containsKey("gdbserver_path")
                            ? extras.getString("gdbserver_path")
                            : packagePath+"lib/gdbserver ";

                        String socket =
                            extras.containsKey("gdbserver_socket")
                            ? extras.getString("gdbserver_socket")
                            : "+debug-socket";

                        if (!(new File(gdbserverPath)).exists())
                            gdbserverPath += ".so";

                        // start debugger
                        m_debuggerProcess = Runtime.getRuntime().exec(gdbserverPath
                                                                        + socket
                                                                        + " --attach "
                                                                        + android.os.Process.myPid(),
                                                                      null,
                                                                      new File(packagePath));
                    } catch (IOException ioe) {
                        Log.e(QtNative.QtTAG,"Can't start debugger" + ioe.getMessage());
                    } catch (SecurityException se) {
                        Log.e(QtNative.QtTAG,"Can't start debugger" + se.getMessage());
                    } catch (NameNotFoundException e) {
                        Log.e(QtNative.QtTAG,"Can't start debugger" + e.getMessage());
                    }
                }


                if ( /*(ai.flags&ApplicationInfo.FLAG_DEBUGGABLE) != 0
                        &&*/ extras.containsKey("debug_ping")
                        && extras.getString("debug_ping").equals("true")) {
                    try {
                        debugLog("extra parameters: " + extras);
                        String packageName = m_activity.getPackageName();
                        String pingFile = extras.getString("ping_file");
                        String pongFile = extras.getString("pong_file");
                        String gdbserverSocket = extras.getString("gdbserver_socket");
                        String gdbserverCommand = extras.getString("gdbserver_command");
                        String pingSocket = extras.getString("ping_socket");
                        boolean usePing = pingFile != null;
                        boolean usePong = pongFile != null;
                        boolean useSocket = gdbserverSocket != null;
                        boolean usePingSocket = pingSocket != null;
                        int napTime = 200; // milliseconds between file accesses
                        int timeOut = 30000; // ms until we give up on ping and pong
                        int maxAttempts = timeOut / napTime;

                        if (gdbserverSocket != null) {
                            debugLog("removing gdb socket " + gdbserverSocket);
                            new File(gdbserverSocket).delete();
                        }

                        if (usePing) {
                            debugLog("removing ping file " + pingFile);
                            File ping = new File(pingFile);
                            if (ping.exists()) {
                                if (!ping.delete())
                                    debugLog("ping file cannot be deleted");
                            }
                        }

                        if (usePong) {
                            debugLog("removing pong file " + pongFile);
                            File pong = new File(pongFile);
                            if (pong.exists()) {
                                if (!pong.delete())
                                    debugLog("pong file cannot be deleted");
                            }
                        }

                        debugLog("starting " + gdbserverCommand);
                        m_debuggerProcess = Runtime.getRuntime().exec(gdbserverCommand);
                        debugLog("gdbserver started");

                        if (useSocket) {
                            int i;
                            for (i = 0; i < maxAttempts; ++i) {
                                debugLog("waiting for socket at " + gdbserverSocket + ", attempt " + i);
                                File file = new File(gdbserverSocket);
                                if (file.exists()) {
                                    file.setReadable(true, false);
                                    file.setWritable(true, false);
                                    file.setExecutable(true, false);
                                    break;
                                }
                                Thread.sleep(napTime);
                            }

                            if (i == maxAttempts) {
                                debugLog("time out when waiting for debug socket");
                                return false;
                            }

                            debugLog("socket ok");
                        } else {
                            debugLog("socket not used");
                        }

                        if (usePingSocket) {
                            DebugWaitRunnable runnable = new DebugWaitRunnable(pingSocket);
                            Thread waitThread = new Thread(runnable);
                            waitThread.start();

                            int i;
                            for (i = 0; i < maxAttempts && waitThread.isAlive(); ++i) {
                                debugLog("Waiting for debug socket connect");
                                debugLog("go to sleep");
                                Thread.sleep(napTime);
                            }

                            if (i == maxAttempts) {
                                debugLog("time out when waiting for ping socket");
                                runnable.shutdown();
                                return false;
                            }

                            if (runnable.wasFailure) {
                                debugLog("Could not connect to debug client");
                                return false;
                            } else {
                                debugLog("Got pid acknowledgment");
                            }
                        }

                        if (usePing) {
                            // Tell we are ready.
                            debugLog("writing ping at " + pingFile);
                            FileWriter writer = new FileWriter(pingFile);
                            writer.write("" + android.os.Process.myPid());
                            writer.close();
                            File file = new File(pingFile);
                            file.setReadable(true, false);
                            file.setWritable(true, false);
                            file.setExecutable(true, false);
                            debugLog("wrote ping");
                        } else {
                            debugLog("ping not requested");
                        }

                        // Wait until other side is ready.
                        if (usePong) {
                            int i;
                            for (i = 0; i < maxAttempts; ++i) {
                                debugLog("waiting for pong at " + pongFile + ", attempt " + i);
                                File file = new File(pongFile);
                                if (file.exists()) {
                                    file.delete();
                                    break;
                                }
                                debugLog("go to sleep");
                                Thread.sleep(napTime);
                            }
                            debugLog("Removing pingFile " + pingFile);
                            new File(pingFile).delete();

                            if (i == maxAttempts) {
                                debugLog("time out when waiting for pong file");
                                return false;
                            }

                            debugLog("got pong " + pongFile);
                        } else {
                            debugLog("pong not requested");
                        }

                    } catch (IOException ioe) {
                        Log.e(QtNative.QtTAG,"Can't start debugger" + ioe.getMessage());
                    } catch (SecurityException se) {
                        Log.e(QtNative.QtTAG,"Can't start debugger" + se.getMessage());
                    }
                }

                if (/*(ai.flags&ApplicationInfo.FLAG_DEBUGGABLE) != 0
                        &&*/ extras.containsKey("qml_debug")
                        && extras.getString("qml_debug").equals("true")) {
                    String qmljsdebugger;
                    if (extras.containsKey("qmljsdebugger")) {
                        qmljsdebugger = extras.getString("qmljsdebugger");
                        qmljsdebugger.replaceAll("\\s", ""); // remove whitespace for security
                    } else {
                        qmljsdebugger = "port:3768";
                    }
                    m_applicationParameters += "\t-qmljsdebugger=" + qmljsdebugger;
                }

                if (extras.containsKey("extraenvvars")) {
                    try {
                        m_environmentVariables += "\t" + new String(Base64.decode(extras.getString("extraenvvars"), Base64.DEFAULT), "UTF-8");
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }

                if (extras.containsKey("extraappparams")) {
                    try {
                        m_applicationParameters += "\t" + new String(Base64.decode(extras.getString("extraappparams"), Base64.DEFAULT), "UTF-8");
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            } // extras != null

            if (null == m_surfaces)
                onCreate(null);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public void onTerminate()
    {
        QtNative.terminateQt();
    }

    public void onCreate(Bundle savedInstanceState)
    {
        m_quitApp = true;
        Runnable startApplication = null;
        if (null == savedInstanceState) {
            startApplication = new Runnable() {
                @Override
                public void run() {
                    try {
                        String nativeLibraryDir = QtNativeLibrariesDir.nativeLibrariesDir(m_activity);
                        QtNative.startApplication(m_applicationParameters,
                            m_environmentVariables,
                            m_mainLib,
                            nativeLibraryDir);
                        m_started = true;
                    } catch (Exception e) {
                        e.printStackTrace();
                        m_activity.finish();
                    }
                }
            };
        }
        m_layout = new QtLayout(m_activity, startApplication);

        try {
            ActivityInfo info = m_activity.getPackageManager().getActivityInfo(m_activity.getComponentName(), PackageManager.GET_META_DATA);
            if (info.metaData.containsKey("android.app.splash_screen_drawable")) {
                m_splashScreenSticky = info.metaData.containsKey("android.app.splash_screen_sticky") && info.metaData.getBoolean("android.app.splash_screen_sticky");
                int id = info.metaData.getInt("android.app.splash_screen_drawable");
                m_splashScreen = new ImageView(m_activity);
                m_splashScreen.setImageDrawable(m_activity.getResources().getDrawable(id));
                m_splashScreen.setScaleType(ImageView.ScaleType.FIT_XY);
                m_splashScreen.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                m_layout.addView(m_splashScreen);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        m_editText = new QtEditText(m_activity, this);
        m_imm = (InputMethodManager)m_activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        m_surfaces =  new HashMap<Integer, QtSurface>();
        m_nativeViews = new HashMap<Integer, View>();
        m_activity.registerForContextMenu(m_layout);
        m_activity.setContentView(m_layout,
                                  new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                             ViewGroup.LayoutParams.MATCH_PARENT));

        int orientation = m_activity.getResources().getConfiguration().orientation;
        int rotation = m_activity.getWindowManager().getDefaultDisplay().getRotation();
        boolean rot90 = (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270);
        boolean currentlyLandscape = (orientation == Configuration.ORIENTATION_LANDSCAPE);
        if ((currentlyLandscape && !rot90) || (!currentlyLandscape && rot90))
            m_nativeOrientation = Configuration.ORIENTATION_LANDSCAPE;
        else
            m_nativeOrientation = Configuration.ORIENTATION_PORTRAIT;

        QtNative.handleOrientationChanged(rotation, m_nativeOrientation);
        m_currentRotation = rotation;

        m_layout.getViewTreeObserver().addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
            @Override
            public boolean onPreDraw() {
                if (!m_keyboardIsVisible)
                    return true;

                Rect r = new Rect();
                m_activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
                DisplayMetrics metrics = new DisplayMetrics();
                m_activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
                final int kbHeight = metrics.heightPixels - r.bottom;
                final int[] location = new int[2];
                m_layout.getLocationOnScreen(location);
                QtNative.keyboardGeometryChanged(location[0], r.bottom - location[1],
                                                 r.width(), kbHeight);
                return true;
            }
        });
    }

    public void hideSplashScreen()
    {
        if (m_splashScreen == null)
            return;
        m_layout.removeView(m_splashScreen);
        m_splashScreen = null;
    }


    public void initializeAccessibility()
    {
        new QtAccessibilityDelegate(m_activity, m_layout, this);
    }

    public void onWindowFocusChanged(boolean hasFocus) {
        try {
            m_super_onWindowFocusChanged.invoke(m_activity, hasFocus);
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (hasFocus)
            updateFullScreen();
    }

    public void onConfigurationChanged(Configuration configuration)
    {
        try {
            m_super_onConfigurationChanged.invoke(m_activity, configuration);
        } catch (Exception e) {
            e.printStackTrace();
        }

        int rotation = m_activity.getWindowManager().getDefaultDisplay().getRotation();
        if (rotation != m_currentRotation) {
            QtNative.handleOrientationChanged(rotation, m_nativeOrientation);
        }

        m_currentRotation = rotation;
    }

    public void onDestroy()
    {
        if (m_quitApp) {
            QtNative.terminateQt();
            QtNative.setActivity(null, null);
            if (m_debuggerProcess != null)
                m_debuggerProcess.destroy();
            System.exit(0);// FIXME remove it or find a better way
        }
    }

    public void onPause()
    {
        QtNative.setApplicationState(ApplicationInactive);
    }

    public void onResume()
    {
        QtNative.setApplicationState(ApplicationActive);
        if (m_started) {
            QtNative.updateWindow();
            updateFullScreen(); // Suspending the app clears the immersive mode, so we need to set it again.
        }
    }

    public void onNewIntent(Intent data)
    {
        QtNative.onNewIntent(data);
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        try {
            m_super_onActivityResult.invoke(m_activity, requestCode, resultCode, data);
        } catch (Exception e) {
            e.printStackTrace();
        }

        QtNative.onActivityResult(requestCode, resultCode, data);
    }


    public void onStop()
    {
        QtNative.setApplicationState(ApplicationSuspended);
    }

    public Object onRetainNonConfigurationInstance()
    {
        try {
            m_super_onRetainNonConfigurationInstance.invoke(m_activity);
        } catch (Exception e) {
            e.printStackTrace();
        }
        m_quitApp = false;
        return true;
    }

    public void onSaveInstanceState(Bundle outState) {
        try {
            m_super_onSaveInstanceState.invoke(m_activity, outState);
        } catch (Exception e) {
            e.printStackTrace();
        }
        outState.putBoolean("FullScreen", m_fullScreen);
        outState.putBoolean("Started", m_started);
        // It should never
    }

    public void onRestoreInstanceState(Bundle savedInstanceState)
    {
        try {
            m_super_onRestoreInstanceState.invoke(m_activity, savedInstanceState);
        } catch (Exception e) {
            e.printStackTrace();
        }
        m_started = savedInstanceState.getBoolean("Started");
        // FIXME restore all surfaces

    }

    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (!m_started)
            return false;

        m_metaState = MetaKeyKeyListener.handleKeyDown(m_metaState, keyCode, event);
        int c = event.getUnicodeChar(MetaKeyKeyListener.getMetaState(m_metaState));
        int lc = c;
        m_metaState = MetaKeyKeyListener.adjustMetaAfterKeypress(m_metaState);

        if ((c & KeyCharacterMap.COMBINING_ACCENT) != 0) {
            c = c & KeyCharacterMap.COMBINING_ACCENT_MASK;
            int composed = KeyEvent.getDeadChar(m_lastChar, c);
            c = composed;
        }

        if ((keyCode == KeyEvent.KEYCODE_VOLUME_UP
            || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN
            || keyCode == KeyEvent.KEYCODE_MUTE)
            && System.getenv("QT_ANDROID_VOLUME_KEYS") == null) {
            return false;
        }

        m_lastChar = lc;
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            m_backKeyPressedSent = !m_keyboardIsVisible;
            if (!m_backKeyPressedSent)
                return true;
        }
        QtNative.keyDown(keyCode, c, event.getMetaState(), event.getRepeatCount() > 0);

        return true;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if (!m_started)
            return false;

        if ((keyCode == KeyEvent.KEYCODE_VOLUME_UP
            || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN
            || keyCode == KeyEvent.KEYCODE_MUTE)
            && System.getenv("QT_ANDROID_VOLUME_KEYS") == null) {
            return false;
        }

        if (keyCode == KeyEvent.KEYCODE_BACK && !m_backKeyPressedSent) {
            hideSoftwareKeyboard();
            setKeyboardVisibility(false, System.nanoTime());
            return true;
        }

        m_metaState = MetaKeyKeyListener.handleKeyUp(m_metaState, keyCode, event);
        QtNative.keyUp(keyCode, event.getUnicodeChar(), event.getMetaState(), event.getRepeatCount() > 0);
        return true;
    }

    public boolean dispatchKeyEvent(KeyEvent event)
    {
        if (m_started
                && event.getAction() == KeyEvent.ACTION_MULTIPLE
                && event.getCharacters() != null
                && event.getCharacters().length() == 1
                && event.getKeyCode() == 0) {
            QtNative.keyDown(0, event.getCharacters().charAt(0), event.getMetaState(), event.getRepeatCount() > 0);
            QtNative.keyUp(0, event.getCharacters().charAt(0), event.getMetaState(), event.getRepeatCount() > 0);
        }

        if (QtNative.dispatchKeyEvent(event))
            return true;

        try {
            return (Boolean) m_super_dispatchKeyEvent.invoke(m_activity, event);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    private boolean m_optionsMenuIsVisible = false;
    public boolean onCreateOptionsMenu(Menu menu)
    {
        menu.clear();
        return true;
    }
    public boolean onPrepareOptionsMenu(Menu menu)
    {
        m_optionsMenuIsVisible = true;
        boolean res = QtNative.onPrepareOptionsMenu(menu);
        setActionBarVisibility(res && menu.size() > 0);
        return res;
    }

    public boolean onOptionsItemSelected(MenuItem item)
    {
        return QtNative.onOptionsItemSelected(item.getItemId(), item.isChecked());
    }

    public void onOptionsMenuClosed(Menu menu)
    {
        m_optionsMenuIsVisible = false;
        QtNative.onOptionsMenuClosed(menu);
    }

    public void resetOptionsMenu()
    {
        m_activity.invalidateOptionsMenu();
    }

    private boolean m_contextMenuVisible = false;
    public void onCreateContextMenu(ContextMenu menu,
                                    View v,
                                    ContextMenuInfo menuInfo)
    {
        menu.clearHeader();
        QtNative.onCreateContextMenu(menu);
        m_contextMenuVisible = true;
    }

    public void onCreatePopupMenu(Menu menu)
    {
        QtNative.fillContextMenu(menu);
        m_contextMenuVisible = true;
    }

    public void onContextMenuClosed(Menu menu)
    {
        if (!m_contextMenuVisible)
            return;
        m_contextMenuVisible = false;
        QtNative.onContextMenuClosed(menu);
    }

    public boolean onContextItemSelected(MenuItem item)
    {
        m_contextMenuVisible = false;
        return QtNative.onContextItemSelected(item.getItemId(), item.isChecked());
    }

    public void openContextMenu(final int x, final int y, final int w, final int h)
    {
        m_layout.postDelayed(new Runnable() {
                @Override
                public void run() {
                    m_layout.setLayoutParams(m_editText, new QtLayout.LayoutParams(w, h, x, y), false);
                    PopupMenu popup = new PopupMenu(m_activity, m_editText);
                    QtActivityDelegate.this.onCreatePopupMenu(popup.getMenu());
                    popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem menuItem) {
                            return QtActivityDelegate.this.onContextItemSelected(menuItem);
                        }
                    });
                    popup.setOnDismissListener(new PopupMenu.OnDismissListener() {
                        @Override
                        public void onDismiss(PopupMenu popupMenu) {
                            QtActivityDelegate.this.onContextMenuClosed(popupMenu.getMenu());
                        }
                    });
                    popup.show();
                }
            }, 100);
    }

    public void closeContextMenu()
    {
        m_activity.closeContextMenu();
    }

    private void setActionBarVisibility(boolean visible)
    {
        if (m_activity.getActionBar() == null)
            return;
        if (ViewConfiguration.get(m_activity).hasPermanentMenuKey() || !visible)
            m_activity.getActionBar().hide();
        else
            m_activity.getActionBar().show();
    }

    public void insertNativeView(int id, View view, int x, int y, int w, int h) {
        if (m_dummyView != null) {
            m_layout.removeView(m_dummyView);
            m_dummyView = null;
        }

        if (m_nativeViews.containsKey(id))
            m_layout.removeView(m_nativeViews.remove(id));

        if (w < 0 || h < 0) {
            view.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                 ViewGroup.LayoutParams.MATCH_PARENT));
        } else {
            view.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
        }

        view.setId(id);
        m_layout.addView(view);
        m_nativeViews.put(id, view);
    }

    public void createSurface(int id, boolean onTop, int x, int y, int w, int h, int imageDepth) {
        if (m_surfaces.size() == 0) {
            TypedValue attr = new TypedValue();
            m_activity.getTheme().resolveAttribute(android.R.attr.windowBackground, attr, true);
            if (attr.type >= TypedValue.TYPE_FIRST_COLOR_INT && attr.type <= TypedValue.TYPE_LAST_COLOR_INT) {
                m_activity.getWindow().setBackgroundDrawable(new ColorDrawable(attr.data));
            } else {
                m_activity.getWindow().setBackgroundDrawable(m_activity.getResources().getDrawable(attr.resourceId));
            }
            if (m_dummyView != null) {
                m_layout.removeView(m_dummyView);
                m_dummyView = null;
            }
        }

        if (m_surfaces.containsKey(id))
            m_layout.removeView(m_surfaces.remove(id));

        QtSurface surface = new QtSurface(m_activity, id, onTop, imageDepth);
        if (w < 0 || h < 0) {
            surface.setLayoutParams( new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));
        } else {
            surface.setLayoutParams( new QtLayout.LayoutParams(w, h, x, y));
        }

        // Native views are always inserted in the end of the stack (i.e., on top).
        // All other views are stacked based on the order they are created.
        final int surfaceCount = getSurfaceCount();
        m_layout.addView(surface, surfaceCount);

        m_surfaces.put(id, surface);
        if (!m_splashScreenSticky)
            hideSplashScreen();
    }

    public void setSurfaceGeometry(int id, int x, int y, int w, int h) {
        if (m_surfaces.containsKey(id)) {
            QtSurface surface = m_surfaces.get(id);
            surface.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
        } else if (m_nativeViews.containsKey(id)) {
            View view = m_nativeViews.get(id);
            view.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
        } else {
            Log.e(QtNative.QtTAG, "Surface " + id +" not found!");
            return;
        }
    }

    public void destroySurface(int id) {
        View view = null;

        if (m_surfaces.containsKey(id)) {
            view = m_surfaces.remove(id);
        } else if (m_nativeViews.containsKey(id)) {
            view = m_nativeViews.remove(id);
        } else {
            Log.e(QtNative.QtTAG, "Surface " + id +" not found!");
        }

        if (view == null)
            return;

        // Keep last frame in stack until it is replaced to get correct
        // shutdown transition
        if (m_surfaces.size() == 0 && m_nativeViews.size() == 0) {
            m_dummyView = view;
        } else {
            m_layout.removeView(view);
        }
    }

    public int getSurfaceCount()
    {
        return m_surfaces.size();
    }

    public void bringChildToFront(int id)
    {
        View view = m_surfaces.get(id);
        if (view != null) {
            final int surfaceCount = getSurfaceCount();
            if (surfaceCount > 0)
                m_layout.moveChild(view, surfaceCount - 1);
            return;
        }

        view = m_nativeViews.get(id);
        if (view != null)
            m_layout.moveChild(view, -1);
    }

    public void bringChildToBack(int id)
    {
        View view = m_surfaces.get(id);
        if (view != null) {
            m_layout.moveChild(view, 0);
            return;
        }

        view = m_nativeViews.get(id);
        if (view != null) {
            final int index = getSurfaceCount();
            m_layout.moveChild(view, index);
        }
    }

    public boolean dispatchGenericMotionEvent (MotionEvent ev)
    {
        if (m_started && QtNative.dispatchGenericMotionEvent(ev))
            return true;

        try {
            return (Boolean) m_super_dispatchGenericMotionEvent.invoke(m_activity, ev);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }
}
