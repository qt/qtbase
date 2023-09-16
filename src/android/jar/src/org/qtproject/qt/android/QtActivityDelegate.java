// Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.Display;
import android.view.Menu;
import android.view.MenuItem;
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

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Objects;

import org.qtproject.qt.android.accessibility.QtAccessibilityDelegate;
import static org.qtproject.qt.android.QtConstants.*;

public class QtActivityDelegate
{
    private Activity m_activity = null;

    // Keep in sync with QtAndroid::SystemUiVisibility in androidjnimain.h
    public static final int SYSTEM_UI_VISIBILITY_NORMAL = 0;
    public static final int SYSTEM_UI_VISIBILITY_FULLSCREEN = 1;
    public static final int SYSTEM_UI_VISIBILITY_TRANSLUCENT = 2;
    private int m_systemUiVisibility = SYSTEM_UI_VISIBILITY_NORMAL;

    private static String m_applicationParameters = null;

    private int m_currentRotation = -1; // undefined
    private int m_nativeOrientation = Configuration.ORIENTATION_UNDEFINED;

    private String m_mainLib;

    private boolean m_started = false;
    private boolean m_quitApp = true;
    private boolean m_isPluginRunning = false;

    private HashMap<Integer, QtSurface> m_surfaces = null;
    private HashMap<Integer, View> m_nativeViews = null;
    private QtLayout m_layout = null;
    private ImageView m_splashScreen = null;
    private boolean m_splashScreenSticky = false;


    private View m_dummyView = null;

    private QtAccessibilityDelegate m_accessibilityDelegate = null;

    private QtInputDelegate.KeyboardVisibilityListener m_keyboardVisibilityListener =
            new QtInputDelegate.KeyboardVisibilityListener() {
        @Override
        public void onKeyboardVisibilityChange() {
            updateFullScreen();
        }
    };
    private final QtInputDelegate m_inputDelegate = new QtInputDelegate(m_keyboardVisibilityListener);

    QtActivityDelegate() { }

    QtInputDelegate getInputDelegate()
    {
        return m_inputDelegate;
    }

    QtLayout getQtLayout()
    {
        return m_layout;
    }

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

    void setStarted(boolean started)
    {
        m_started = started;
    }

    boolean isStarted()
    {
        return m_started;
    }

    void setQuitApp(boolean quitApp)
    {
        m_quitApp = quitApp;
    }

    boolean isQuitApp()
    {
        return m_quitApp;
    }

    boolean isPluginRunning()
    {
        return m_isPluginRunning;
    }

    int systemUiVisibility()
    {
        return m_systemUiVisibility;
    }

    void setContextMenuVisible(boolean contextMenuVisible)
    {
        m_contextMenuVisible = contextMenuVisible;
    }

    boolean isContextMenuVisible()
    {
        return m_contextMenuVisible;
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

        m_inputDelegate.setSoftInputMode(m_activity.getPackageManager().getActivityInfo(m_activity.getComponentName(), 0).softInputMode);

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

        m_inputDelegate.setEditText(new QtEditText(m_activity));
        m_inputDelegate.setInputMethodManager((InputMethodManager)m_activity.getSystemService(Context.INPUT_METHOD_SERVICE));
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
                if (!m_inputDelegate.isKeyboardVisible())
                    return true;

                Rect r = new Rect();
                m_activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
                DisplayMetrics metrics = new DisplayMetrics();
                m_activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
                final int kbHeight = metrics.heightPixels - r.bottom;
                if (kbHeight < 0) {
                    m_inputDelegate.setKeyboardVisibility(false, System.nanoTime());
                    return true;
                }
                final int[] location = new int[2];
                m_layout.getLocationOnScreen(location);
                QtInputDelegate.keyboardGeometryChanged(location[0], r.bottom - location[1],
                                                 r.width(), kbHeight);
                return true;
            }
        });
        m_inputDelegate.setEditPopupMenu(new EditPopupMenu(m_activity, m_layout));
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

    boolean isUiModeDark(Configuration config)
    {
        return (config.uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;
    }

    void handleUiModeChange(int uiMode)
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

    public void resetOptionsMenu()
    {
        m_activity.invalidateOptionsMenu();
    }

    private boolean m_contextMenuVisible = false;

    public void onCreatePopupMenu(Menu menu)
    {
        QtNative.fillContextMenu(menu);
        m_contextMenuVisible = true;
    }

    public void openContextMenu(final int x, final int y, final int w, final int h)
    {
        m_layout.postDelayed(new Runnable() {
                @Override
                public void run() {
                    m_layout.setLayoutParams(m_inputDelegate.getQtEditText(), new QtLayout.LayoutParams(w, h, x, y), false);
                    PopupMenu popup = new PopupMenu(m_activity, m_inputDelegate.getQtEditText());
                    QtActivityDelegate.this.onCreatePopupMenu(popup.getMenu());
                    popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem menuItem) {
                            return m_activity.onContextItemSelected(menuItem);
                        }
                    });
                    popup.setOnDismissListener(new PopupMenu.OnDismissListener() {
                        @Override
                        public void onDismiss(PopupMenu popupMenu) {
                            m_activity.onContextMenuClosed(popupMenu.getMenu());
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

    void setActionBarVisibility(boolean visible)
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
}
