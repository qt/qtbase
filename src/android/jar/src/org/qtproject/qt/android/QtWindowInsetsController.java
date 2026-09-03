// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.os.Build;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.WindowInsetsController;
import android.view.Window;

import android.graphics.Color;
import android.util.TypedValue;
import android.content.res.Resources.Theme;

class QtWindowInsetsController
{
    /*
    * Convenience method to call deprecated API prior to Android R (30).
    */
    @SuppressWarnings ("deprecation")
    private static void setDecorFitsSystemWindows(Window window, boolean enable)
    {
        final int sdk = Build.VERSION.SDK_INT;
        if (sdk < Build.VERSION_CODES.R || sdk > Build.VERSION_CODES.VANILLA_ICE_CREAM)
            return;
        window.setDecorFitsSystemWindows(enable);
    }

    private static void useCutoutShortEdges(Window window, boolean enabled)
    {
        if (window == null)
            return;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            WindowManager.LayoutParams layoutParams = window.getAttributes();
            layoutParams.layoutInDisplayCutoutMode = enabled
                    ? WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
                    : WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
            window.setAttributes(layoutParams);
        }
    }

    @UsedFromNativeCode
    static void showNormal(Activity activity)
    {
        Window window = activity.getWindow();
        if (window == null)
            return;

        final View decor = window.getDecorView();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            setDecorFitsSystemWindows(window, true);
            WindowInsetsController ctrl = window.getInsetsController();
            if (ctrl != null) {
                ctrl.show(WindowInsets.Type.systemBars());
                ctrl.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_DEFAULT);
            }
        } else {
            @SuppressWarnings("deprecation")
            int flags = View.SYSTEM_UI_FLAG_VISIBLE; // clear all flags
            setSystemUiVisibility(decor, flags);
        }

        setTransparentSystemBars(activity, false);
        useCutoutShortEdges(window, false);

        decor.post(() -> decor.requestApplyInsets());
    }

    /*
    * Make system bars transparent for Andorid versions prior to Android 15.
    */
    @SuppressWarnings("deprecation")
    private static void setTransparentSystemBars(Activity activity, boolean transparent)
    {
        Window window = activity.getWindow();
        if (window == null)
            return;

        if (edgeToEdgeEnabled(activity))
            return;

        // These are needed to operate on system bar colors
        window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS
                    | WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
        window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);

        if (transparent) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                window.setStatusBarColor(Color.TRANSPARENT);
                window.setNavigationBarColor(Color.TRANSPARENT);
            } else {
                // Android 9 and prior doesn't add the semi-transparent bars
                // to avoid low contrast system icons, so try to mimick it
                // by taking the current color and only increase the opacity.
                int statusBarColor = window.getStatusBarColor();
                int transparentStatusBar = statusBarColor & 0x00FFFFFF;
                window.setStatusBarColor(transparentStatusBar);

                int navigationBarColor = window.getNavigationBarColor();
                int semiTransparentNavigationBar = navigationBarColor & 0x7FFFFFFF;
                window.setNavigationBarColor(semiTransparentNavigationBar);
            }
        } else {
            // Restore theme's system bars colors
            int defaultStatusBarColor = getThemeDefaultStatusBarColor(activity);
            window.setStatusBarColor(defaultStatusBarColor);

            int defaultNavigationBarColor = getThemeDefaultNavigationBarColor(activity);
            window.setNavigationBarColor(defaultNavigationBarColor);
        }
    }

    @UsedFromNativeCode
    static void showExpanded(Activity activity)
    {
        Window window = activity.getWindow();
        if (window == null)
            return;

        final View decor = window.getDecorView();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            setDecorFitsSystemWindows(window, false);
            WindowInsetsController ctrl = window.getInsetsController();
            if (ctrl != null) {
                ctrl.show(WindowInsets.Type.systemBars());
                ctrl.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_DEFAULT);
            }
        } else {
            @SuppressWarnings("deprecation")
            int flags =   View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
            setSystemUiVisibility(decor, flags);
        }

        setTransparentSystemBars(activity, true);
        useCutoutShortEdges(window, true);

        decor.post(() -> decor.requestApplyInsets());
    }

    @UsedFromNativeCode
    public static void showFullScreen(Activity activity)
    {
        Window window = activity.getWindow();
        if (window == null)
            return;

        final View decor = window.getDecorView();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            setDecorFitsSystemWindows(window, false);
            WindowInsetsController ctrl = window.getInsetsController();
            if (ctrl != null) {
                ctrl.hide(WindowInsets.Type.systemBars());
                ctrl.setSystemBarsBehavior(
                    WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            @SuppressWarnings("deprecation")
            int flags =   View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
            setSystemUiVisibility(decor, flags);
        }

        useCutoutShortEdges(window, true);

        decor.post(() -> decor.requestApplyInsets());
    }

    private static boolean edgeToEdgeEnabled(Activity activity) {
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.VANILLA_ICE_CREAM)
            return true;
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.VANILLA_ICE_CREAM)
            return false;
        int[] attrs = new int[] { android.R.attr.windowOptOutEdgeToEdgeEnforcement };
        TypedArray ta = activity.getTheme().obtainStyledAttributes(attrs);
        try {
            return !ta.getBoolean(0, false);
        } finally {
            ta.recycle();
        }
    }

    static boolean isFullScreen(Activity activity)
    {
        Window window = activity.getWindow();
        if (window == null)
            return false;

        final View decor = window.getDecorView();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                final int behavior = controller.getSystemBarsBehavior();
                return behavior == WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE;
            }
        } else if (Build.VERSION.SDK_INT == Build.VERSION_CODES.R) {
            WindowInsets insets = decor.getRootWindowInsets();
            if (insets != null)
                return !insets.isVisible(WindowInsets.Type.statusBars());
        } else {
            @SuppressWarnings("deprecation")
            int flags = decor.getSystemUiVisibility();
            @SuppressWarnings("deprecation")
            int immersiveMask = View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
            return (flags & immersiveMask) == immersiveMask;
        }

        return false;
    }

    static boolean isExpandedClientArea(Activity activity)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.VANILLA_ICE_CREAM)
            return edgeToEdgeEnabled(activity);

        @SuppressWarnings("deprecation")
        int statusBarColor = activity.getWindow().getStatusBarColor();
        // If the status bar is not fully opaque assume we have expanded client
        // area and we're drawing under it.
        int statusBarAlpha = statusBarColor >>> 24;
        return statusBarAlpha != 0xFF;
    }

    static boolean decorFitsSystemWindows(Activity activity)
    {
        return !isFullScreen(activity) && !isExpandedClientArea(activity);
    }

    static void restoreFullScreenVisibility(Activity activity)
    {
        if (isFullScreen(activity))
            showFullScreen(activity);
    }

    /*
    * Convenience method to call deprecated API prior to Android R (30).
    */
    @SuppressWarnings ("deprecation")
    private static void setSystemUiVisibility(View decorView, int flags)
    {
        decorView.setSystemUiVisibility(flags);
    }

    /*
    * Set the status bar color scheme hint so that the system decides how to color the icons.
    */
    @UsedFromNativeCode
    static void setStatusBarColorHint(Activity activity, boolean isLight)
    {
        Window window = activity.getWindow();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                int lightStatusBarMask = WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS;
                int appearance = isLight ? lightStatusBarMask : 0;
                controller.setSystemBarsAppearance(appearance, lightStatusBarMask);
            }
        } else {
            @SuppressWarnings("deprecation")
            int currentFlags = window.getDecorView().getSystemUiVisibility();
            @SuppressWarnings("deprecation")
            int lightStatusBarMask = View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
            int appearance = isLight
                ? currentFlags | lightStatusBarMask
                : currentFlags & ~lightStatusBarMask;
            setSystemUiVisibility(window.getDecorView(), appearance);
        }
    }

    /*
    * Set the navigation bar color scheme hint so that the system decides how to color the icons.
    */
    @UsedFromNativeCode
    static void setNavigationBarColorHint(Activity activity, boolean isLight)
    {
        Window window = activity.getWindow();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                int lightNavigationBarMask = WindowInsetsController.APPEARANCE_LIGHT_NAVIGATION_BARS;
                int appearance = isLight ? lightNavigationBarMask : 0;
                controller.setSystemBarsAppearance(appearance, lightNavigationBarMask);
            }
        } else {
            @SuppressWarnings("deprecation")
            int currentFlags = window.getDecorView().getSystemUiVisibility();
            @SuppressWarnings("deprecation")
            int lightNavigationBarMask = View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR;
            int appearance = isLight
                ? currentFlags | lightNavigationBarMask
                : currentFlags & ~lightNavigationBarMask;
            setSystemUiVisibility(window.getDecorView(), appearance);
        }
    }

    private static int resolveColorAttribute(Activity activity, int attribute)
    {
        Theme theme = activity.getTheme();
        Resources resources = activity.getResources();
        TypedValue tv = new TypedValue();

        if (theme.resolveAttribute(attribute, tv, true)) {
            if (tv.resourceId != 0)
                return resources.getColor(tv.resourceId, theme);
            if (tv.type >= TypedValue.TYPE_FIRST_COLOR_INT && tv.type <= TypedValue.TYPE_LAST_COLOR_INT)
                return tv.data;
        }

        return -1;
    }

    @SuppressWarnings("deprecation")
    static int getThemeDefaultStatusBarColor(Activity activity)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.VANILLA_ICE_CREAM)
            return -1;
        return resolveColorAttribute(activity, android.R.attr.statusBarColor);
    }

    @SuppressWarnings("deprecation")
    static int getThemeDefaultNavigationBarColor(Activity activity)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.VANILLA_ICE_CREAM)
            return -1;
        return resolveColorAttribute(activity, android.R.attr.navigationBarColor);
    }

    static void enableSystemBarsBackgroundDrawing(Window window)
    {
        // These are needed to operate on system bar colors
        window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
        @SuppressWarnings("deprecation")
        final int translucentFlags =  WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS
                                    | WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION;
        window.clearFlags(translucentFlags);
    }

    @SuppressWarnings("deprecation")
    static void setStatusBarColor(Window window, int color)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.VANILLA_ICE_CREAM)
            return;
        window.setStatusBarColor(color);
    }

    @SuppressWarnings("deprecation")
    static void setNavigationBarColor(Window window, int color)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.VANILLA_ICE_CREAM)
            return;
        window.setNavigationBarColor(color);
    }
}
