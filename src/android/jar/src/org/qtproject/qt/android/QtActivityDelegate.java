// Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.graphics.Color;
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
import android.view.animation.AccelerateInterpolator;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Display;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.view.ViewTreeObserver;
import android.widget.ImageView;
import android.widget.PopupMenu;
import android.hardware.display.DisplayManager;

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
import java.util.Objects;

import org.qtproject.qt.android.accessibility.QtAccessibilityDelegate;

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

    // Keep in sync with QtAndroid::SystemUiVisibility in androidjnimain.h
    public static final int SYSTEM_UI_VISIBILITY_NORMAL = 0;
    public static final int SYSTEM_UI_VISIBILITY_FULLSCREEN = 1;
    public static final int SYSTEM_UI_VISIBILITY_TRANSLUCENT = 2;

    private static String m_applicationParameters = null;

    private int m_currentRotation = -1; // undefined
    private int m_nativeOrientation = Configuration.ORIENTATION_UNDEFINED;

    private String m_mainLib;
    private long m_metaState;
    private int m_lastChar = 0;
    private int m_softInputMode = 0;
    private int m_systemUiVisibility = SYSTEM_UI_VISIBILITY_NORMAL;
    private boolean m_started = false;
    private HashMap<Integer, QtSurface> m_surfaces = null;
    private HashMap<Integer, View> m_nativeViews = null;
    private QtLayout m_layout = null;
    private ImageView m_splashScreen = null;
    private boolean m_splashScreenSticky = false;
    private QtEditText m_editText = null;
    private InputMethodManager m_imm = null;
    private boolean m_quitApp = true;
    private View m_dummyView = null;
    private boolean m_keyboardIsVisible = false;
    public boolean m_backKeyPressedSent = false;
    private long m_showHideTimeStamp = System.nanoTime();
    private int m_portraitKeyboardHeight = 0;
    private int m_landscapeKeyboardHeight = 0;
    private int m_probeKeyboardHeightDelay = 50; // ms
    private CursorHandle m_cursorHandle;
    private CursorHandle m_leftSelectionHandle;
    private CursorHandle m_rightSelectionHandle;
    private EditPopupMenu m_editPopupMenu;
    private boolean m_isPluginRunning = false;

    private QtAccessibilityDelegate m_accessibilityDelegate = null;


    public void setSystemUiVisibility(int systemUiVisibility)
    {
        if (m_systemUiVisibility == systemUiVisibility)
            return;

        m_systemUiVisibility = systemUiVisibility;

        int systemUiVisibilityFlags = View.SYSTEM_UI_FLAG_VISIBLE;
        switch (m_systemUiVisibility) {
        case SYSTEM_UI_VISIBILITY_NORMAL:
            m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
            m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            setDisplayCutoutLayout(WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER);
            break;
        case SYSTEM_UI_VISIBILITY_FULLSCREEN:
            m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
            systemUiVisibilityFlags = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.INVISIBLE;
            setDisplayCutoutLayout(WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT);
            break;
        case SYSTEM_UI_VISIBILITY_TRANSLUCENT:
            m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN
                    | WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION
                    | WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            setDisplayCutoutLayout(WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS);
            break;
        };

        m_activity.getWindow().getDecorView().setSystemUiVisibility(systemUiVisibilityFlags);

        m_layout.requestLayout();
    }

    private void setDisplayCutoutLayout(int cutoutLayout)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
            m_activity.getWindow().getAttributes().layoutInDisplayCutoutMode = cutoutLayout;
    }

    public void updateFullScreen()
    {
        if (m_systemUiVisibility == SYSTEM_UI_VISIBILITY_FULLSCREEN) {
            m_systemUiVisibility = SYSTEM_UI_VISIBILITY_NORMAL;
            setSystemUiVisibility(SYSTEM_UI_VISIBILITY_FULLSCREEN);
        }
    }

    public boolean isKeyboardVisible()
    {
        return m_keyboardIsVisible;
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
        QtNative.keyboardVisibilityUpdated(m_keyboardIsVisible);

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
            final boolean softInputIsHidden = (m_softInputMode & WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN) != 0;
            if (softInputIsHidden)
                return;
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
                inputType |= android.text.InputType.TYPE_NUMBER_VARIATION_PASSWORD;
        } else if ((inputHints & ImhDialableCharactersOnly) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_PHONE;
        } else if ((inputHints & (ImhDate | ImhTime)) != 0) {
            inputType = android.text.InputType.TYPE_CLASS_DATETIME;
            if ((inputHints & (ImhDate | ImhTime)) != (ImhDate | ImhTime)) {
                if ((inputHints & ImhDate) != 0)
                    inputType |= android.text.InputType.TYPE_DATETIME_VARIATION_DATE;
                else
                    inputType |= android.text.InputType.TYPE_DATETIME_VARIATION_TIME;
            } // else {  TYPE_DATETIME_VARIATION_NORMAL(0) }
        } else { // CLASS_TEXT
            if ((inputHints & ImhHiddenText) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD;
            } else if ((inputHints & ImhSensitiveData) != 0 ||
                ((inputHints & ImhNoPredictiveText) != 0 &&
                  System.getenv("QT_ANDROID_ENABLE_WORKAROUND_TO_DISABLE_PREDICTIVE_TEXT") != null)) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD;
            } else if ((inputHints & ImhUrlCharactersOnly) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_URI;
                if (enterKeyType == 0) // not explicitly overridden
                    imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_GO;
            } else if ((inputHints & ImhEmailCharactersOnly) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
            }

            if ((inputHints & ImhMultiLine) != 0) {
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_MULTI_LINE;
                // Clear imeOptions for Multi-Line Type
                // User should be able to insert new line in such case
                imeOptions = android.view.inputmethod.EditorInfo.IME_ACTION_DONE;
            }
            if ((inputHints & (ImhNoPredictiveText | ImhSensitiveData | ImhHiddenText)) != 0)
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;

            if ((inputHints & ImhUppercaseOnly) != 0) {
                initialCapsMode |= android.text.TextUtils.CAP_MODE_CHARACTERS;
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS;
            } else if ((inputHints & ImhLowercaseOnly) == 0 && (inputHints & ImhNoAutoUppercase) == 0) {
                initialCapsMode |= android.text.TextUtils.CAP_MODE_SENTENCES;
                inputType |= android.text.InputType.TYPE_TEXT_FLAG_CAP_SENTENCES;
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

    int getAppIconSize(Activity a)
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

        return size;
    }

    public void updateSelection(int selStart, int selEnd, int candidatesStart, int candidatesEnd)
    {
        if (m_imm == null)
            return;

        m_imm.updateSelection(m_editText, selStart, selEnd, candidatesStart, candidatesEnd);
    }

    // Values coming from QAndroidInputContext::CursorHandleShowMode
    private static final int CursorHandleNotShown       = 0;
    private static final int CursorHandleShowNormal     = 1;
    private static final int CursorHandleShowSelection  = 2;
    private static final int CursorHandleShowEdit       = 0x100;

    public int getSelectHandleWidth()
    {
        int width = 0;
        if (m_leftSelectionHandle != null && m_rightSelectionHandle != null) {
            width = Math.max(m_leftSelectionHandle.width(), m_rightSelectionHandle.width());
        } else if (m_cursorHandle != null) {
            width = m_cursorHandle.width();
        }
        return width;
    }

    /* called from the C++ code when the position of the cursor or selection handles needs to
       be adjusted.
       mode is one of QAndroidInputContext::CursorHandleShowMode
    */
    public void updateHandles(int mode, int editX, int editY, int editButtons, int x1, int y1, int x2, int y2, boolean rtl)
    {
        switch (mode & 0xff)
        {
            case CursorHandleNotShown:
                if (m_cursorHandle != null) {
                    m_cursorHandle.hide();
                    m_cursorHandle = null;
                }
                if (m_rightSelectionHandle != null) {
                    m_rightSelectionHandle.hide();
                    m_leftSelectionHandle.hide();
                    m_rightSelectionHandle = null;
                    m_leftSelectionHandle = null;
                }
                if (m_editPopupMenu != null)
                    m_editPopupMenu.hide();
                break;

            case CursorHandleShowNormal:
                if (m_cursorHandle == null) {
                    m_cursorHandle = new CursorHandle(m_activity, m_layout, QtNative.IdCursorHandle,
                                                      android.R.attr.textSelectHandle, false);
                }
                m_cursorHandle.setPosition(x1, y1);
                if (m_rightSelectionHandle != null) {
                    m_rightSelectionHandle.hide();
                    m_leftSelectionHandle.hide();
                    m_rightSelectionHandle = null;
                    m_leftSelectionHandle = null;
                }
                break;

            case CursorHandleShowSelection:
                if (m_rightSelectionHandle == null) {
                    m_leftSelectionHandle = new CursorHandle(m_activity, m_layout, QtNative.IdLeftHandle,
                                                             !rtl ? android.R.attr.textSelectHandleLeft :
                                                                    android.R.attr.textSelectHandleRight,
                                                             rtl);
                    m_rightSelectionHandle = new CursorHandle(m_activity, m_layout, QtNative.IdRightHandle,
                                                              !rtl ? android.R.attr.textSelectHandleRight :
                                                                     android.R.attr.textSelectHandleLeft,
                                                              rtl);
                }
                m_leftSelectionHandle.setPosition(x1,y1);
                m_rightSelectionHandle.setPosition(x2,y2);
                if (m_cursorHandle != null) {
                    m_cursorHandle.hide();
                    m_cursorHandle = null;
                }
                mode |= CursorHandleShowEdit;
                break;
        }

        if (!QtNative.hasClipboardText())
            editButtons &= ~EditContextView.PASTE_BUTTON;

        if ((mode & CursorHandleShowEdit) == CursorHandleShowEdit && editButtons != 0) {
            m_editPopupMenu.setPosition(editX, editY, editButtons, m_cursorHandle, m_leftSelectionHandle,
                                        m_rightSelectionHandle);
        } else {
            if (m_editPopupMenu != null)
                m_editPopupMenu.hide();
        }
    }

    private final DisplayManager.DisplayListener displayListener = new DisplayManager.DisplayListener()
    {
        @Override
        public void onDisplayAdded(int displayId) {
            QtNative.handleScreenAdded(displayId);
        }

        private boolean isSimilarRotation(int r1, int r2)
        {
         return (r1 == r2)
                || (r1 == Surface.ROTATION_0 && r2 == Surface.ROTATION_180)
                || (r1 == Surface.ROTATION_180 && r2 == Surface.ROTATION_0)
                || (r1 == Surface.ROTATION_90 && r2 == Surface.ROTATION_270)
                || (r1 == Surface.ROTATION_270 && r2 == Surface.ROTATION_90);
        }

        @Override
        public void onDisplayChanged(int displayId)
        {
            Display display = (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
                 ? m_activity.getWindowManager().getDefaultDisplay()
                 : m_activity.getDisplay();
            m_currentRotation = display.getRotation();
            m_layout.setActivityDisplayRotation(m_currentRotation);
            // Process orientation change only if it comes after the size
            // change, or if the screen is rotated by 180 degrees.
            // Otherwise it will be processed in QtLayout.
            if (isSimilarRotation(m_currentRotation, m_layout.displayRotation()))
                QtNative.handleOrientationChanged(m_currentRotation, m_nativeOrientation);

            float refreshRate = display.getRefreshRate();
            QtNative.handleRefreshRateChanged(refreshRate);
            QtNative.handleScreenChanged(displayId);
        }

        @Override
        public void onDisplayRemoved(int displayId) {
            QtNative.handleScreenRemoved(displayId);
        }
    };

    public boolean updateActivity(Activity activity)
    {
        try {
            // set new activity
            loadActivity(activity);

            // update the new activity content view to old layout
            ViewGroup layoutParent = (ViewGroup)m_layout.getParent();
            if (layoutParent != null)
                layoutParent.removeView(m_layout);

            m_activity.setContentView(m_layout);

            // force c++ native activity object to update
            return QtNative.updateNativeActivity();
        } catch (Exception e) {
            Log.w(QtNative.QtTAG, "Failed to update the activity.");
            e.printStackTrace();
            return false;
        }
    }

    private void loadActivity(Activity activity)
        throws NoSuchMethodException, PackageManager.NameNotFoundException
    {
        m_activity = activity;

        QtNative.setActivity(m_activity, this);
        setActionBarVisibility(false);

        Class<?> activityClass = m_activity.getClass();
        m_super_dispatchKeyEvent =
                activityClass.getMethod("super_dispatchKeyEvent", KeyEvent.class);
        m_super_onRestoreInstanceState =
                activityClass.getMethod("super_onRestoreInstanceState", Bundle.class);
        m_super_onRetainNonConfigurationInstance =
                activityClass.getMethod("super_onRetainNonConfigurationInstance");
        m_super_onSaveInstanceState =
                activityClass.getMethod("super_onSaveInstanceState", Bundle.class);
        m_super_onKeyDown =
                activityClass.getMethod("super_onKeyDown", Integer.TYPE, KeyEvent.class);
        m_super_onKeyUp =
                activityClass.getMethod("super_onKeyUp", Integer.TYPE, KeyEvent.class);
        m_super_onConfigurationChanged =
                activityClass.getMethod("super_onConfigurationChanged", Configuration.class);
        m_super_onActivityResult =
                activityClass.getMethod("super_onActivityResult", Integer.TYPE, Integer.TYPE, Intent.class);
        m_super_onWindowFocusChanged =
                activityClass.getMethod("super_onWindowFocusChanged", Boolean.TYPE);
        m_super_dispatchGenericMotionEvent =
                activityClass.getMethod("super_dispatchGenericMotionEvent", MotionEvent.class);

        m_softInputMode = m_activity.getPackageManager().getActivityInfo(m_activity.getComponentName(), 0).softInputMode;

        DisplayManager displayManager = (DisplayManager)m_activity.getSystemService(Context.DISPLAY_SERVICE);
        displayManager.registerDisplayListener(displayListener, null);
    }

    public boolean loadApplication(Activity activity, ClassLoader classLoader, Bundle loaderParams)
    {
        /// check parameters integrity
        if (!loaderParams.containsKey(NATIVE_LIBRARIES_KEY)
                || !loaderParams.containsKey(BUNDLED_LIBRARIES_KEY)
                || !loaderParams.containsKey(ENVIRONMENT_VARIABLES_KEY)) {
            return false;
        }

        try {
            loadActivity(activity);
            QtNative.setClassLoader(classLoader);
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }

        if (loaderParams.containsKey(STATIC_INIT_CLASSES_KEY)) {
            for (String className: Objects.requireNonNull(loaderParams.getStringArray(STATIC_INIT_CLASSES_KEY))) {
                if (className.length() == 0)
                    continue;

                try {
                  Class<?> initClass = classLoader.loadClass(className);
                  Object staticInitDataObject = initClass.newInstance(); // create an instance
                  try {
                      Method m = initClass.getMethod("setActivity", Activity.class, Object.class);
                      m.invoke(staticInitDataObject, m_activity, this);
                  } catch (Exception e) {
                      Log.d(QtNative.QtTAG, "Class " + className + " does not implement setActivity method");
                  }

                  // For modules that don't need/have setActivity
                  try {
                      Method m = initClass.getMethod("setContext", Context.class);
                      m.invoke(staticInitDataObject, (Context)m_activity);
                  } catch (Exception e) {
                      e.printStackTrace();
                  }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        QtNative.loadQtLibraries(loaderParams.getStringArrayList(NATIVE_LIBRARIES_KEY));
        ArrayList<String> libraries = loaderParams.getStringArrayList(BUNDLED_LIBRARIES_KEY);
        String nativeLibsDir = QtNativeLibrariesDir.nativeLibrariesDir(m_activity);
        QtNative.loadBundledLibraries(libraries, nativeLibsDir);
        m_mainLib = loaderParams.getString(MAIN_LIBRARY_KEY);
        // older apps provide the main library as the last bundled library; look for this if the main library isn't provided
        if (null == m_mainLib && libraries.size() > 0) {
            m_mainLib = libraries.get(libraries.size() - 1);
            libraries.remove(libraries.size() - 1);
        }

        ExtractStyle.setup(loaderParams);
        ExtractStyle.runIfNeeded(m_activity, isUiModeDark(m_activity.getResources().getConfiguration()));

        QtNative.setEnvironmentVariables(loaderParams.getString(ENVIRONMENT_VARIABLES_KEY));
        QtNative.setEnvironmentVariable("QT_ANDROID_FONTS_MONOSPACE",
                                        "Droid Sans Mono;Droid Sans;Droid Sans Fallback");
        QtNative.setEnvironmentVariable("QT_ANDROID_FONTS_SERIF", "Droid Serif");
        QtNative.setEnvironmentVariable("HOME", m_activity.getFilesDir().getAbsolutePath());
        QtNative.setEnvironmentVariable("TMPDIR", m_activity.getCacheDir().getAbsolutePath());
        QtNative.setEnvironmentVariable("QT_ANDROID_FONTS",
                                        "Roboto;Droid Sans;Droid Sans Fallback");
        QtNative.setEnvironmentVariable("QT_ANDROID_APP_ICON_SIZE",
                                        String.valueOf(getAppIconSize(activity)));

        if (loaderParams.containsKey(APPLICATION_PARAMETERS_KEY))
            m_applicationParameters = loaderParams.getString(APPLICATION_PARAMETERS_KEY);
        else
            m_applicationParameters = "";

        m_mainLib = QtNative.loadMainLibrary(m_mainLib, nativeLibsDir);
        return m_mainLib != null;
    }

    public boolean startApplication()
    {
        // start application
        try {

            Bundle extras = m_activity.getIntent().getExtras();
            if (extras != null) {
                try {
                    final boolean isDebuggable = (m_activity.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
                    if (!isDebuggable)
                        throw new Exception();

                    if (extras.containsKey("extraenvvars")) {
                        try {
                            QtNative.setEnvironmentVariables(new String(
                                    Base64.decode(extras.getString("extraenvvars"), Base64.DEFAULT),
                                    "UTF-8"));
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
                } catch (Exception e) {
                    Log.e(QtNative.QtTAG, "Not in debug mode! It is not allowed to use " +
                                          "extra arguments in non-debug mode.");
                    // This is not an error, so keep it silent
                    // e.printStackTrace();
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
        QtNative.m_qtThread.exit();
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
                        QtNative.startApplication(m_applicationParameters, m_mainLib);
                        m_started = true;
                    } catch (Exception e) {
                        e.printStackTrace();
                        m_activity.finish();
                    }
                }
            };
        }
        m_layout = new QtLayout(m_activity, startApplication);

        int orientation = m_activity.getResources().getConfiguration().orientation;

        try {
            ActivityInfo info = m_activity.getPackageManager().getActivityInfo(m_activity.getComponentName(), PackageManager.GET_META_DATA);

            String splashScreenKey = "android.app.splash_screen_drawable_"
                + (orientation == Configuration.ORIENTATION_LANDSCAPE ? "landscape" : "portrait");
            if (!info.metaData.containsKey(splashScreenKey))
                splashScreenKey = "android.app.splash_screen_drawable";

            if (info.metaData.containsKey(splashScreenKey)) {
                m_splashScreenSticky = info.metaData.containsKey("android.app.splash_screen_sticky") && info.metaData.getBoolean("android.app.splash_screen_sticky");
                int id = info.metaData.getInt(splashScreenKey);
                m_splashScreen = new ImageView(m_activity);
                m_splashScreen.setImageDrawable(m_activity.getResources().getDrawable(id, m_activity.getTheme()));
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

        int rotation = m_activity.getWindowManager().getDefaultDisplay().getRotation();
        boolean rot90 = (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270);
        boolean currentlyLandscape = (orientation == Configuration.ORIENTATION_LANDSCAPE);
        if ((currentlyLandscape && !rot90) || (!currentlyLandscape && rot90))
            m_nativeOrientation = Configuration.ORIENTATION_LANDSCAPE;
        else
            m_nativeOrientation = Configuration.ORIENTATION_PORTRAIT;

        m_layout.setNativeOrientation(m_nativeOrientation);
        QtNative.handleOrientationChanged(rotation, m_nativeOrientation);
        m_currentRotation = rotation;

        handleUiModeChange(m_activity.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK);

        float refreshRate = (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
                ? m_activity.getWindowManager().getDefaultDisplay().getRefreshRate()
                : m_activity.getDisplay().getRefreshRate();
        QtNative.handleRefreshRateChanged(refreshRate);

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
                if (kbHeight < 0) {
                    setKeyboardVisibility(false, System.nanoTime());
                    return true;
                }
                final int[] location = new int[2];
                m_layout.getLocationOnScreen(location);
                QtNative.keyboardGeometryChanged(location[0], r.bottom - location[1],
                                                 r.width(), kbHeight);
                return true;
            }
        });
        m_editPopupMenu = new EditPopupMenu(m_activity, m_layout);
    }

    public void hideSplashScreen()
    {
        hideSplashScreen(0);
    }

    public void hideSplashScreen(final int duration)
    {
        if (m_splashScreen == null)
            return;

        if (duration <= 0) {
            m_layout.removeView(m_splashScreen);
            m_splashScreen = null;
            return;
        }

        final Animation fadeOut = new AlphaAnimation(1, 0);
        fadeOut.setInterpolator(new AccelerateInterpolator());
        fadeOut.setDuration(duration);

        fadeOut.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationEnd(Animation animation) { hideSplashScreen(0); }

            @Override
            public void onAnimationRepeat(Animation animation) {}

            @Override
            public void onAnimationStart(Animation animation) {}
        });

        m_splashScreen.startAnimation(fadeOut);
    }

    public void notifyAccessibilityLocationChange(int viewId)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyLocationChange(viewId);
    }

    public void notifyObjectHide(int viewId, int parentId)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyObjectHide(viewId, parentId);
    }

    public void notifyObjectFocus(int viewId)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyObjectFocus(viewId);
    }

    public void notifyValueChanged(int viewId, String value)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyValueChanged(viewId, value);
    }

    public void notifyScrolledEvent(int viewId)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyScrolledEvent(viewId);
    }


    public void notifyQtAndroidPluginRunning(boolean running)
    {
        m_isPluginRunning = running;
    }

    public void initializeAccessibility()
    {
        m_accessibilityDelegate = new QtAccessibilityDelegate(m_activity, m_layout, this);
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

    boolean isUiModeDark(Configuration config)
    {
        return (config.uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;
    }

    private void handleUiModeChange(int uiMode)
    {
        // QTBUG-108365
        if (Build.VERSION.SDK_INT >= 30) {
            // Since 29 version we are using Theme_DeviceDefault_DayNight
            Window window = m_activity.getWindow();
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                // set APPEARANCE_LIGHT_STATUS_BARS if needed
                int appearanceLight = Color.luminance(window.getStatusBarColor()) > 0.5 ?
                        WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS : 0;
                controller.setSystemBarsAppearance(appearanceLight,
                    WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS);
            }
        }
        switch (uiMode) {
            case Configuration.UI_MODE_NIGHT_NO:
                ExtractStyle.runIfNeeded(m_activity, false);
                QtNative.handleUiDarkModeChanged(0);
                break;
            case Configuration.UI_MODE_NIGHT_YES:
                ExtractStyle.runIfNeeded(m_activity, true);
                QtNative.handleUiDarkModeChanged(1);
                break;
        }
    }

    public void onConfigurationChanged(Configuration configuration)
    {
        try {
            m_super_onConfigurationChanged.invoke(m_activity, configuration);
        } catch (Exception e) {
            e.printStackTrace();
        }
        handleUiModeChange(configuration.uiMode & Configuration.UI_MODE_NIGHT_MASK);
    }

    public void onDestroy()
    {
        if (m_quitApp) {
            QtNative.terminateQt();
            QtNative.setActivity(null, null);
            QtNative.m_qtThread.exit();
            System.exit(0);
        }
    }

    public void onPause()
    {
        if (Build.VERSION.SDK_INT < 24 || !m_activity.isInMultiWindowMode())
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
        outState.putInt("SystemUiVisibility", m_systemUiVisibility);
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
        if (!m_started || !m_isPluginRunning)
            return false;

        m_metaState = MetaKeyKeyListener.handleKeyDown(m_metaState, keyCode, event);
        int c = event.getUnicodeChar(MetaKeyKeyListener.getMetaState(m_metaState) | event.getMetaState());
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
        if (!m_started || !m_isPluginRunning)
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
                m_activity.getWindow().setBackgroundDrawable(m_activity.getResources().getDrawable(attr.resourceId, m_activity.getTheme()));
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

    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        QtNative.sendRequestPermissionsResult(requestCode, permissions, grantResults);
    }
}
