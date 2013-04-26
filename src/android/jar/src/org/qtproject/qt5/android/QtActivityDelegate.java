/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Android port of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

package org.qtproject.qt5.android;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Iterator;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.text.method.MetaKeyKeyListener;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

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

    private static final String NATIVE_LIBRARIES_KEY = "native.libraries";
    private static final String BUNDLED_LIBRARIES_KEY = "bundled.libraries";
    private static final String MAIN_LIBRARY_KEY = "main.library";
    private static final String ENVIRONMENT_VARIABLES_KEY = "environment.variables";
    private static final String APPLICATION_PARAMETERS_KEY = "application.parameters";
    private static final String STATIC_INIT_CLASSES_KEY = "static.init.classes";
    private static final String NECESSITAS_API_LEVEL_KEY = "necessitas.api.level";

    private static String m_environmentVariables = null;
    private static String m_applicationParameters = null;

    private int m_currentOrientation = Configuration.ORIENTATION_UNDEFINED;

    private String m_mainLib;
    private long m_metaState;
    private int m_lastChar = 0;
    private boolean m_fullScreen = false;
    private boolean m_started = false;
    private QtSurface m_surface = null;
    private QtLayout m_layout = null;
    private QtEditText m_editText = null;
    private InputMethodManager m_imm = null;
    private boolean m_quitApp = true;
    private Process m_debuggerProcess = null; // debugger process

    public boolean m_keyboardIsVisible = false;
    public boolean m_keyboardIsHiding = false;

    public QtLayout getQtLayout()
    {
        return m_layout;
    }

    public QtSurface getQtSurface()
    {
        return m_surface;
    }

    public void redrawWindow(int left, int top, int right, int bottom)
    {
        m_surface.drawBitmap(new Rect(left, top, right, bottom));
    }

    public void setFullScreen(boolean enterFullScreen)
    {
        if (m_fullScreen == enterFullScreen)
            return;

        if (m_fullScreen = enterFullScreen) {
            m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
        } else {
            m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
            m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
    }

    // case status
    private final int ImhNoAutoUppercase = 0x2;
    private final int ImhPreferUppercase = 0x8;
    @SuppressWarnings("unused")
    private final int ImhPreferLowercase = 0x10;
    private final int ImhUppercaseOnly = 0x40000;
    private final int ImhLowercaseOnly = 0x80000;

    // options
    private final int ImhNoPredictiveText = 0x20;

    // layout
    private final int ImhHiddenText = 0x1;
    private final int ImhPreferNumbers = 0x4;
    private final int ImhMultiLine = 0x400;
    private final int ImhDigitsOnly = 0x10000;
    private final int ImhFormattedNumbersOnly = 0x20000;
    private final int ImhDialableCharactersOnly = 0x100000;
    private final int ImhEmailCharactersOnly = 0x200000;
    private final int ImhUrlCharactersOnly = 0x400000;

    public void resetSoftwareKeyboard()
    {
        if (m_imm == null)
            return;
        m_editText.postDelayed(new Runnable() {
            @Override
            public void run() {
                m_imm.restartInput(m_editText);
            }
        }, 5);
    }

    public void showSoftwareKeyboard(int x, int y, int width, int height, int inputHints)
    {
        if (m_imm == null)
            return;
        if (height > m_surface.getHeight()*2/3)
            m_activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_UNCHANGED | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        else
            m_activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_UNCHANGED | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);

        int initialCapsMode = 0;
        int imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_DONE;
        int inputType = android.text.InputType.TYPE_CLASS_TEXT;

        if ((inputHints & ImhMultiLine) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_TEXT | android.text.InputType.TYPE_TEXT_FLAG_MULTI_LINE;
            imeOptions = android.view.inputmethod.EditorInfo.IME_FLAG_NO_ENTER_ACTION;
        }

        if (((inputHints & ImhNoAutoUppercase) != 0 || (inputHints & ImhPreferUppercase) != 0)
                && (inputHints & ImhLowercaseOnly) == 0) {
            initialCapsMode = android.text.TextUtils.CAP_MODE_SENTENCES;
        }

        if ((inputHints & ImhUppercaseOnly) != 0)
            initialCapsMode = android.text.TextUtils.CAP_MODE_CHARACTERS;

        if ((inputHints & ImhHiddenText) != 0)
            inputType = android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD;

        if ((inputHints & ImhPreferNumbers) != 0)
            inputType = android.text.InputType.TYPE_CLASS_NUMBER;

        if ((inputHints & ImhDigitsOnly) != 0)
            inputType = android.text.InputType.TYPE_CLASS_NUMBER;

        if ((inputHints & ImhFormattedNumbersOnly) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_NUMBER
                        | android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL
                        | android.text.InputType.TYPE_NUMBER_FLAG_SIGNED;
        }

        if ((inputHints & ImhDialableCharactersOnly) != 0)
            inputType = android.text.InputType.TYPE_CLASS_PHONE;

        if ((inputHints & ImhEmailCharactersOnly) != 0)
            inputType = android.text.InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;

        if ((inputHints & ImhUrlCharactersOnly) != 0) {
            inputType = android.text.InputType.TYPE_TEXT_VARIATION_URI;
            imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_GO;
        }

        if ((inputHints & ImhNoPredictiveText) != 0) {
            //android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | android.text.InputType.TYPE_CLASS_TEXT;
            inputType = android.text.InputType.TYPE_CLASS_TEXT | android.text.InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD;
        }

        m_editText.setInitialCapsMode(initialCapsMode);
        m_editText.setImeOptions(imeOptions);
        m_editText.setInputType(inputType);

        m_layout.removeView(m_editText);
        m_layout.addView(m_editText, new QtLayout.LayoutParams(width, height, x, y));
        m_editText.bringToFront();
        m_editText.requestFocus();
        m_editText.postDelayed(new Runnable() {
            @Override
            public void run() {
                m_imm.showSoftInput(m_editText, 0);
                m_keyboardIsVisible = true;
                m_keyboardIsHiding = false;
                m_editText.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        m_imm.restartInput(m_editText);
                    }
                }, 25);
            }
        }, 15);
    }

    public void hideSoftwareKeyboard()
    {
        if (m_imm == null)
            return;
        m_imm.hideSoftInputFromWindow(m_editText.getWindowToken(), 0);
        m_keyboardIsVisible = false;
        m_keyboardIsHiding = false;
    }

    public boolean isSoftwareKeyboardVisible()
    {
        return m_keyboardIsVisible;
    }

    String getAppIconSize(Activity a)
    {
        int size = a.getResources().getDimensionPixelSize(android.R.dimen.app_icon_size);
        if (size < 36 || size > 512) { // check size sanity
            DisplayMetrics metrics = new DisplayMetrics();
            a.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            size = metrics.densityDpi/10*3;
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
        QtNative.setActivity(m_activity, this);
        QtNative.setClassLoader(classLoader);
        if (loaderParams.containsKey(STATIC_INIT_CLASSES_KEY)) {
            for (String className: loaderParams.getStringArray(STATIC_INIT_CLASSES_KEY)) {
                if (className.length() == 0)
                    continue;

                try {
                    @SuppressWarnings("rawtypes")
                    Class initClass = classLoader.loadClass(className);
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

        try {
            m_super_dispatchKeyEvent = m_activity.getClass().getMethod("super_dispatchKeyEvent", KeyEvent.class);
            m_super_onRestoreInstanceState = m_activity.getClass().getMethod("super_onRestoreInstanceState", Bundle.class);
            m_super_onRetainNonConfigurationInstance = m_activity.getClass().getMethod("super_onRetainNonConfigurationInstance");
            m_super_onSaveInstanceState = m_activity.getClass().getMethod("super_onSaveInstanceState", Bundle.class);
            m_super_onKeyDown = m_activity.getClass().getMethod("super_onKeyDown", Integer.TYPE, KeyEvent.class);
            m_super_onKeyUp = m_activity.getClass().getMethod("super_onKeyUp", Integer.TYPE, KeyEvent.class);
            m_super_onConfigurationChanged = m_activity.getClass().getMethod("super_onConfigurationChanged", Configuration.class);
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }

        int necessitasApiLevel = 1;
        if (loaderParams.containsKey(NECESSITAS_API_LEVEL_KEY))
            necessitasApiLevel = loaderParams.getInt(NECESSITAS_API_LEVEL_KEY);

        m_environmentVariables = loaderParams.getString(ENVIRONMENT_VARIABLES_KEY);
        String additionalEnvironmentVariables = "QT_ANDROID_FONTS_MONOSPACE=Droid Sans Mono;Droid Sans;Droid Sans Fallback"
                                              + "\tNECESSITAS_API_LEVEL=" + necessitasApiLevel
                                              + "\tHOME=" + m_activity.getFilesDir().getAbsolutePath()
                                              + "\tTMPDIR=" + m_activity.getFilesDir().getAbsolutePath();
        if (android.os.Build.VERSION.SDK_INT < 14)
            additionalEnvironmentVariables += "\tQT_ANDROID_FONTS=Droid Sans;Droid Sans Fallback";
        else
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

        return true;
    }

    public void debugLog(String msg)
    {
        Log.i(QtNative.QtTAG, "DEBUGGER: " + msg);
    }

    public boolean startApplication()
    {
        // start application
        try {
            // FIXME turn on debuggable check
            // if the applications is debuggable and it has a native debug request
            Bundle extras = m_activity.getIntent().getExtras();
            if ( /*(ai.flags&ApplicationInfo.FLAG_DEBUGGABLE) != 0
                    &&*/ extras != null
                    && extras.containsKey("native_debug")
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
                    &&*/ extras != null
                    && extras.containsKey("debug_ping")
                    && extras.getString("debug_ping").equals("true")) {
                try {
                    debugLog("extra parameters: " + extras);
                    String packageName = m_activity.getPackageName();
                    String pingFile = extras.getString("ping_file");
                    String pongFile = extras.getString("pong_file");
                    String gdbserverSocket = extras.getString("gdbserver_socket");
                    String gdbserverCommand = extras.getString("gdbserver_command");
                    boolean usePing = pingFile != null;
                    boolean usePong = pongFile != null;
                    boolean useSocket = gdbserverSocket != null;
                    int napTime = 200; // milliseconds between file accesses
                    int timeOut = 30000; // ms until we give up on ping and pong
                    int maxAttempts = timeOut / napTime;

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
                            debugLog("time out when waiting for socket");
                            return false;
                        }

                        debugLog("socket ok");
                    } else {
                        debugLog("socket not used");
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
                    &&*/ extras != null
                    && extras.containsKey("qml_debug")
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

            if (null == m_surface)
                onCreate(null);
            String nativeLibraryDir = QtNativeLibrariesDir.nativeLibrariesDir(m_activity);
            m_surface.applicationStarted( QtNative.startApplication(m_applicationParameters,
                                                                    m_environmentVariables,
                                                                    m_mainLib,
                                                                    nativeLibraryDir));
            m_started = true;
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
        if (null == savedInstanceState) {
            DisplayMetrics metrics = new DisplayMetrics();
            m_activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            QtNative.setApplicationDisplayMetrics(metrics.widthPixels, metrics.heightPixels,
                                                  metrics.widthPixels, metrics.heightPixels,
                                                  metrics.xdpi, metrics.ydpi, metrics.scaledDensity);
        }
        m_layout = new QtLayout(m_activity);
        m_surface = new QtSurface(m_activity, 0);
        m_editText = new QtEditText(m_activity);
        m_imm = (InputMethodManager)m_activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        m_layout.addView(m_surface,0);
        m_activity.setContentView(m_layout,
                                  new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT,
                                                             ViewGroup.LayoutParams.FILL_PARENT));
        m_layout.bringChildToFront(m_surface);
        m_activity.registerForContextMenu(m_layout);

        m_currentOrientation = m_activity.getResources().getConfiguration().orientation;
    }

    public void onConfigurationChanged(Configuration configuration)
    {
        try {
            m_super_onConfigurationChanged.invoke(m_activity, configuration);
        } catch (Exception e) {
            e.printStackTrace();
        }

        if (configuration.orientation != m_currentOrientation
            && m_currentOrientation != Configuration.ORIENTATION_UNDEFINED) {
            QtNative.handleOrientationChanged(configuration.orientation);
        }

        m_currentOrientation = configuration.orientation;
    }

    public void onDestroy()
    {
        if (m_quitApp) {
            if (m_debuggerProcess != null)
                m_debuggerProcess.destroy();
            System.exit(0);// FIXME remove it or find a better way
        }
    }

    public void onRestoreInstanceState(Bundle savedInstanceState)
    {
        try {
            m_super_onRestoreInstanceState.invoke(m_activity, savedInstanceState);
        } catch (Exception e) {
            e.printStackTrace();
        }
        m_started = savedInstanceState.getBoolean("Started");
        if (m_started)
            m_surface.applicationStarted(true);
    }

    public void onResume()
    {
        // fire all lostActions
        synchronized (QtNative.m_mainActivityMutex)
        {
            Iterator<Runnable> itr = QtNative.getLostActions().iterator();
            while (itr.hasNext())
                m_activity.runOnUiThread(itr.next());

            if (m_started) {
                QtNative.clearLostActions();
                QtNative.updateWindow();
            }
        }
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
    }

    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (!m_started)
            return false;

        if (keyCode == KeyEvent.KEYCODE_MENU) {
            try {
                return (Boolean)m_super_onKeyDown.invoke(m_activity, keyCode, event);
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        }

        m_metaState = MetaKeyKeyListener.handleKeyDown(m_metaState, keyCode, event);
        int c = event.getUnicodeChar(MetaKeyKeyListener.getMetaState(m_metaState));
        int lc = c;
        m_metaState = MetaKeyKeyListener.adjustMetaAfterKeypress(m_metaState);

        if ((c & KeyCharacterMap.COMBINING_ACCENT) != 0) {
            c = c & KeyCharacterMap.COMBINING_ACCENT_MASK;
            int composed = KeyEvent.getDeadChar(m_lastChar, c);
            c = composed;
        }

        m_lastChar = lc;
        if (keyCode != KeyEvent.KEYCODE_BACK)
            QtNative.keyDown(keyCode, c, event.getMetaState());

        return true;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if (!m_started)
            return false;

        if (keyCode == KeyEvent.KEYCODE_MENU) {
            try {
                return (Boolean)m_super_onKeyUp.invoke(m_activity, keyCode, event);
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        }

        if (keyCode == KeyEvent.KEYCODE_BACK && m_keyboardIsVisible && !m_keyboardIsHiding) {
            hideSoftwareKeyboard();
            return true;
        }

        m_metaState = MetaKeyKeyListener.handleKeyUp(m_metaState, keyCode, event);
        QtNative.keyUp(keyCode, event.getUnicodeChar(), event.getMetaState());
        return true;
    }

    public boolean dispatchKeyEvent(KeyEvent event)
    {
        if (m_started
                && event.getAction() == KeyEvent.ACTION_MULTIPLE
                && event.getCharacters() != null
                && event.getCharacters().length() == 1
                && event.getKeyCode() == 0) {
            QtNative.keyDown(0, event.getCharacters().charAt(0), event.getMetaState());
            QtNative.keyUp(0, event.getCharacters().charAt(0), event.getMetaState());
        }

        try {
            return (Boolean) m_super_dispatchKeyEvent.invoke(m_activity, event);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    private boolean m_opionsMenuIsVisible = false;
    public boolean onCreateOptionsMenu(Menu menu)
    {
        menu.clear();
        return true;
    }
    public boolean onPrepareOptionsMenu(Menu menu)
    {
        m_opionsMenuIsVisible = true;
        return QtNative.onPrepareOptionsMenu(menu);
    }

    public boolean onOptionsItemSelected(MenuItem item)
    {
        return QtNative.onOptionsItemSelected(item.getItemId(), item.isChecked());
    }

    public void onOptionsMenuClosed(Menu menu)
    {
        m_opionsMenuIsVisible = false;
        QtNative.onOptionsMenuClosed(menu);
    }

    public void resetOptionsMenu()
    {
        if (m_opionsMenuIsVisible)
            m_activity.closeOptionsMenu();
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

    public void onContextMenuClosed(Menu menu)
    {
        if (!m_contextMenuVisible) {
            Log.e(QtNative.QtTAG, "invalid onContextMenuClosed call");
            return;
        }
        m_contextMenuVisible = false;
        QtNative.onContextMenuClosed(menu);
    }

    public boolean onContextItemSelected(MenuItem item)
    {
        return QtNative.onContextItemSelected(item.getItemId(), item.isChecked());
    }

    public void openContextMenu()
    {
        m_layout.postDelayed(new Runnable() {
                @Override
                public void run() {
                    m_activity.openContextMenu(m_layout);
                }
            }, 10);
    }

    public void closeContextMenu()
    {
        m_activity.closeContextMenu();
    }
}
