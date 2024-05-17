// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.hardware.display.DisplayManager;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.Size;
import android.view.Display;
import android.view.Surface;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.WindowMetrics;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

class QtDisplayManager {

    // screen methods
    public static native void setDisplayMetrics(int screenWidthPixels, int screenHeightPixels,
                                                int availableLeftPixels, int availableTopPixels,
                                                int availableWidthPixels, int availableHeightPixels,
                                                double XDpi, double YDpi, double scaledDensity,
                                                double density, float refreshRate);
    public static native void handleOrientationChanged(int newRotation, int nativeOrientation);
    public static native void handleRefreshRateChanged(float refreshRate);
    public static native void handleUiDarkModeChanged(int newUiMode);
    public static native void handleScreenAdded(int displayId);
    public static native void handleScreenChanged(int displayId);
    public static native void handleScreenRemoved(int displayId);
    // screen methods

    // Keep in sync with QtAndroid::SystemUiVisibility in androidjnimain.h
    public static final int SYSTEM_UI_VISIBILITY_NORMAL = 0;
    public static final int SYSTEM_UI_VISIBILITY_FULLSCREEN = 1;
    public static final int SYSTEM_UI_VISIBILITY_TRANSLUCENT = 2;
    private int m_systemUiVisibility = SYSTEM_UI_VISIBILITY_NORMAL;

    private static int m_previousRotation = -1;

    private DisplayManager.DisplayListener m_displayListener = null;
    private final Activity m_activity;

    QtDisplayManager(Activity activity)
    {
        m_activity = activity;
        initDisplayListener();
    }

    private void initDisplayListener() {
        m_displayListener = new DisplayManager.DisplayListener() {
            @Override
            public void onDisplayAdded(int displayId) {
                QtDisplayManager.handleScreenAdded(displayId);
            }

            @Override
            public void onDisplayChanged(int displayId) {
                Display display = (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
                        ? m_activity.getWindowManager().getDefaultDisplay()
                        : m_activity.getDisplay();
                float refreshRate = getRefreshRate(display);
                QtDisplayManager.handleRefreshRateChanged(refreshRate);
                QtDisplayManager.handleScreenChanged(displayId);
            }

            @Override
            public void onDisplayRemoved(int displayId) {
                QtDisplayManager.handleScreenRemoved(displayId);
            }
        };
    }

    static void handleOrientationChanges(Activity activity)
    {
        int currentRotation = getDisplayRotation(activity);
        if (m_previousRotation == currentRotation)
            return;
        int nativeOrientation = getNativeOrientation(activity, currentRotation);
        QtDisplayManager.handleOrientationChanged(currentRotation, nativeOrientation);
        m_previousRotation = currentRotation;
    }

    public static int getDisplayRotation(Activity activity) {
        Display display = Build.VERSION.SDK_INT < Build.VERSION_CODES.R ?
                activity.getWindowManager().getDefaultDisplay() :
                activity.getDisplay();

        return display != null ? display.getRotation() : 0;
    }

    private static int getNativeOrientation(Activity activity, int rotation)
    {
        int orientation = activity.getResources().getConfiguration().orientation;
        boolean rot90 = (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270);
        boolean isLandscape = (orientation == Configuration.ORIENTATION_LANDSCAPE);
        if ((isLandscape && !rot90) || (!isLandscape && rot90))
            return Configuration.ORIENTATION_LANDSCAPE;

        return Configuration.ORIENTATION_PORTRAIT;
    }

    static float getRefreshRate(Display display)
    {
        return display != null ? display.getRefreshRate() : 60.0f;
    }

    public void registerDisplayListener()
    {
        DisplayManager displayManager =
                (DisplayManager) m_activity.getSystemService(Context.DISPLAY_SERVICE);
        displayManager.registerDisplayListener(m_displayListener, null);
    }

    public void unregisterDisplayListener()
    {
        DisplayManager displayManager =
                (DisplayManager) m_activity.getSystemService(Context.DISPLAY_SERVICE);
        displayManager.unregisterDisplayListener(m_displayListener);
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
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    m_activity.getWindow().getAttributes().layoutInDisplayCutoutMode =
                            WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
                }
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
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    m_activity.getWindow().getAttributes().layoutInDisplayCutoutMode =
                            WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT;
                }
                break;
            case SYSTEM_UI_VISIBILITY_TRANSLUCENT:
                m_activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN
                        | WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION
                        | WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
                m_activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    m_activity.getWindow().getAttributes().layoutInDisplayCutoutMode =
                            WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
                }
                break;
        }
        m_activity.getWindow().getDecorView().setSystemUiVisibility(systemUiVisibilityFlags);
    }

    public int systemUiVisibility()
    {
        return m_systemUiVisibility;
    }

    public void updateFullScreen()
    {
        if (m_systemUiVisibility == SYSTEM_UI_VISIBILITY_FULLSCREEN) {
            m_systemUiVisibility = SYSTEM_UI_VISIBILITY_NORMAL;
            setSystemUiVisibility(SYSTEM_UI_VISIBILITY_FULLSCREEN);
        }
    }

    @UsedFromNativeCode
    public static Display getDisplay(Context context, int displayId)
    {
        DisplayManager displayManager =
                (DisplayManager)context.getSystemService(Context.DISPLAY_SERVICE);
        if (displayManager != null) {
            return displayManager.getDisplay(displayId);
        }
        return null;
    }

    @UsedFromNativeCode
    public static List<Display> getAvailableDisplays(Context context)
    {
        DisplayManager displayManager =
                (DisplayManager)context.getSystemService(Context.DISPLAY_SERVICE);
        if (displayManager != null) {
            Display[] displays = displayManager.getDisplays();
            return Arrays.asList(displays);
        }
        return new ArrayList<>();
    }

    @UsedFromNativeCode
    public static Size getDisplaySize(Context displayContext, Display display)
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            DisplayMetrics realMetrics = new DisplayMetrics();
            display.getRealMetrics(realMetrics);
            return new Size(realMetrics.widthPixels, realMetrics.heightPixels);
        }

        Context windowsContext = displayContext.createWindowContext(
                WindowManager.LayoutParams.TYPE_APPLICATION, null);
        WindowManager windowManager =
                (WindowManager) windowsContext.getSystemService(Context.WINDOW_SERVICE);
        WindowMetrics windowsMetrics = windowManager.getCurrentWindowMetrics();
        Rect bounds = windowsMetrics.getBounds();
        return new Size(bounds.width(), bounds.height());
    }

    public static void setApplicationDisplayMetrics(Activity activity, int width, int height)
    {
        if (activity == null)
            return;

        final WindowInsets rootInsets = activity.getWindow().getDecorView().getRootWindowInsets();
        final WindowManager windowManager = activity.getWindowManager();
        Display display;

        int insetLeft;
        int insetTop;

        int maxWidth;
        int maxHeight;

        if (android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            display = windowManager.getDefaultDisplay();

            final DisplayMetrics maxMetrics = new DisplayMetrics();
            display.getRealMetrics(maxMetrics);
            maxWidth = maxMetrics.widthPixels;
            maxHeight = maxMetrics.heightPixels;

            insetLeft = rootInsets.getStableInsetLeft();
            insetTop = rootInsets.getStableInsetTop();
        } else {
            display = activity.getDisplay();

            final WindowMetrics maxMetrics = windowManager.getMaximumWindowMetrics();
            maxWidth = maxMetrics.getBounds().width();
            maxHeight = maxMetrics.getBounds().height();

            insetLeft = rootInsets.getInsetsIgnoringVisibility(WindowInsets.Type.systemBars()).left;
            insetTop = rootInsets.getInsetsIgnoringVisibility(WindowInsets.Type.systemBars()).top;
        }

        final DisplayMetrics displayMetrics = activity.getResources().getDisplayMetrics();

        double density = displayMetrics.density;
        double scaledDensity = displayMetrics.scaledDensity;

        setDisplayMetrics(maxWidth, maxHeight, insetLeft, insetTop,
                width, height, getXDpi(displayMetrics), getYDpi(displayMetrics),
                scaledDensity, density, getRefreshRate(display));
    }

    public static float getXDpi(final DisplayMetrics metrics) {
        if (metrics.xdpi < android.util.DisplayMetrics.DENSITY_LOW)
            return android.util.DisplayMetrics.DENSITY_LOW;
        return metrics.xdpi;
    }

    public static float getYDpi(final DisplayMetrics metrics) {
        if (metrics.ydpi < android.util.DisplayMetrics.DENSITY_LOW)
            return android.util.DisplayMetrics.DENSITY_LOW;
        return metrics.ydpi;
    }
}
