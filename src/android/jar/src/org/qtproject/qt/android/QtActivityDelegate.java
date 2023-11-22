// Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.Rect;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.ViewTreeObserver;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.Menu;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsetsController;
import android.widget.ImageView;
import android.widget.PopupMenu;

import java.util.ArrayList;
import java.util.HashMap;

class QtActivityDelegate
{
    private Activity m_activity;

    private HashMap<Integer, QtSurface> m_surfaces = null;
    private HashMap<Integer, View> m_nativeViews = null;
    private QtLayout m_layout = null;
    private ImageView m_splashScreen = null;
    private boolean m_splashScreenSticky = false;

    private View m_dummyView = null;

    private QtAccessibilityDelegate m_accessibilityDelegate = null;
    private QtDisplayManager m_displayManager = null;

    private QtInputDelegate m_inputDelegate = null;

    QtActivityDelegate(Activity activity)
    {
        m_activity = activity;
        QtNative.setActivity(m_activity);

        setActionBarVisibility(false);
    }

    QtDisplayManager displayManager() {
        return m_displayManager;
    }

    @UsedFromNativeCode
    QtInputDelegate getInputDelegate() {
        return m_inputDelegate;
    }

    @UsedFromNativeCode
    QtLayout getQtLayout()
    {
        return m_layout;
    }

    @UsedFromNativeCode
    public void setSystemUiVisibility(int systemUiVisibility)
    {
        QtNative.runAction(() -> {
            m_displayManager.setSystemUiVisibility(systemUiVisibility);
            m_layout.requestLayout();
            QtNative.updateWindow();
        });
    }

    void setContextMenuVisible(boolean contextMenuVisible)
    {
        m_contextMenuVisible = contextMenuVisible;
    }

    boolean isContextMenuVisible()
    {
        return m_contextMenuVisible;
    }

    public boolean updateActivityAfterRestart(Activity activity) {
        try {
            // set new activity
            m_activity = activity;
            QtNative.setActivity(m_activity);

            // update the new activity content view to old layout
            ViewGroup layoutParent = (ViewGroup) m_layout.getParent();
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

    public void startNativeApplication(ArrayList<String> appParams, String mainLib)
    {
        if (m_surfaces != null)
            return;

        initMembers();

        m_layout.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        QtNative.startApplication(appParams, mainLib);
                        m_layout.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                    }
                });
    }

    private void initMembers()
    {
        m_layout = new QtLayout(m_activity);

        m_displayManager = new QtDisplayManager(m_activity, m_layout);
        m_displayManager.registerDisplayListener();

        QtInputDelegate.KeyboardVisibilityListener keyboardVisibilityListener =
                () -> m_displayManager.updateFullScreen();
        m_inputDelegate = new QtInputDelegate(m_activity, keyboardVisibilityListener);

        try {
            PackageManager pm = m_activity.getPackageManager();
            ActivityInfo activityInfo =  pm.getActivityInfo(m_activity.getComponentName(), 0);
            m_inputDelegate.setSoftInputMode(activityInfo.softInputMode);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }

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

        m_surfaces = new HashMap<>();
        m_nativeViews = new HashMap<>();
        m_activity.registerForContextMenu(m_layout);
        m_activity.setContentView(m_layout,
                                  new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                             ViewGroup.LayoutParams.MATCH_PARENT));

        int rotation = m_activity.getWindowManager().getDefaultDisplay().getRotation();
        int nativeOrientation = QtDisplayManager.getNativeOrientation(m_activity, rotation);
        m_layout.setNativeOrientation(nativeOrientation);
        QtDisplayManager.handleOrientationChanged(rotation, nativeOrientation);

        handleUiModeChange(m_activity.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK);

        float refreshRate = (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
                ? m_activity.getWindowManager().getDefaultDisplay().getRefreshRate()
                : m_activity.getDisplay().getRefreshRate();
        QtDisplayManager.handleRefreshRateChanged(refreshRate);

        m_layout.getViewTreeObserver().addOnPreDrawListener(() -> {
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
        });
        m_inputDelegate.setEditPopupMenu(new EditPopupMenu(m_activity, m_layout));
    }

    public void hideSplashScreen()
    {
        hideSplashScreen(0);
    }

    public void hideSplashScreen(final int duration)
    {
        QtNative.runAction(() -> {
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
                public void onAnimationEnd(Animation animation) {
                    hideSplashScreen(0);
                }

                @Override
                public void onAnimationRepeat(Animation animation) {
                }

                @Override
                public void onAnimationStart(Animation animation) {
                }
            });

            m_splashScreen.startAnimation(fadeOut);
        });
    }

    @UsedFromNativeCode
    public void notifyLocationChange(int viewId)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyLocationChange(viewId);
    }

    @UsedFromNativeCode
    public void notifyObjectHide(int viewId, int parentId)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyObjectHide(viewId, parentId);
    }

    @UsedFromNativeCode
    public void notifyObjectFocus(int viewId)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyObjectFocus(viewId);
    }

    @UsedFromNativeCode
    public void notifyValueChanged(int viewId, String value)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyValueChanged(viewId, value);
    }

    @UsedFromNativeCode
    public void notifyScrolledEvent(int viewId)
    {
        if (m_accessibilityDelegate == null)
            return;
        m_accessibilityDelegate.notifyScrolledEvent(viewId);
    }

    @UsedFromNativeCode
    public void initializeAccessibility()
    {
        final QtActivityDelegate currentDelegate = this;
        QtNative.runAction(() -> m_accessibilityDelegate = new QtAccessibilityDelegate(m_activity,
                m_layout, currentDelegate));
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
                QtDisplayManager.handleUiDarkModeChanged(0);
                break;
            case Configuration.UI_MODE_NIGHT_YES:
                ExtractStyle.runIfNeeded(m_activity, true);
                QtDisplayManager.handleUiDarkModeChanged(1);
                break;
        }
    }

    @UsedFromNativeCode
    public void resetOptionsMenu()
    {
        QtNative.runAction(() -> m_activity.invalidateOptionsMenu());
    }

    @UsedFromNativeCode
    public void openOptionsMenu()
    {
        QtNative.runAction(() -> m_activity.openOptionsMenu());
    }

    private boolean m_contextMenuVisible = false;

    public void onCreatePopupMenu(Menu menu)
    {
        QtNative.fillContextMenu(menu);
        m_contextMenuVisible = true;
    }

    @UsedFromNativeCode
    public void openContextMenu(final int x, final int y, final int w, final int h)
    {
        m_layout.postDelayed(() -> {
            m_layout.setLayoutParams(m_inputDelegate.getQtEditText(), new QtLayout.LayoutParams(w, h, x, y), false);
            PopupMenu popup = new PopupMenu(m_activity, m_inputDelegate.getQtEditText());
            QtActivityDelegate.this.onCreatePopupMenu(popup.getMenu());
            popup.setOnMenuItemClickListener(menuItem ->
                    m_activity.onContextItemSelected(menuItem));
            popup.setOnDismissListener(popupMenu ->
                    m_activity.onContextMenuClosed(popupMenu.getMenu()));
            popup.show();
        }, 100);
    }

    @UsedFromNativeCode
    public void closeContextMenu()
    {
        QtNative.runAction(() -> m_activity.closeContextMenu());
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

    @UsedFromNativeCode
    public void insertNativeView(int id, View view, int x, int y, int w, int h) {
    QtNative.runAction(() -> {
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
    });
    }

    @UsedFromNativeCode
    public void createSurface(int id, boolean onTop, int x, int y, int w, int h, int imageDepth) {
        QtNative.runAction(() -> {
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
                surface.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT));
            } else {
                surface.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
            }

            // Native views are always inserted in the end of the stack (i.e., on top).
            // All other views are stacked based on the order they are created.
            final int surfaceCount = getSurfaceCount();
            m_layout.addView(surface, surfaceCount);

            m_surfaces.put(id, surface);
            if (!m_splashScreenSticky)
                hideSplashScreen();
        });
    }

    @UsedFromNativeCode
    public void setSurfaceGeometry(int id, int x, int y, int w, int h) {
        QtNative.runAction(() -> {
            if (m_surfaces.containsKey(id)) {
                QtSurface surface = m_surfaces.get(id);
                surface.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
            } else if (m_nativeViews.containsKey(id)) {
                View view = m_nativeViews.get(id);
                view.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
            } else {
                Log.e(QtNative.QtTAG, "Surface " + id + " not found!");
            }
        });
    }

    @UsedFromNativeCode
    public void destroySurface(int id) {
        QtNative.runAction(() -> {
            View view = null;

            if (m_surfaces.containsKey(id)) {
                view = m_surfaces.remove(id);
            } else if (m_nativeViews.containsKey(id)) {
                view = m_nativeViews.remove(id);
            } else {
                Log.e(QtNative.QtTAG, "Surface " + id + " not found!");
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
        });
    }

    public int getSurfaceCount()
    {
        return m_surfaces.size();
    }

    @UsedFromNativeCode
    public void bringChildToFront(int id)
    {
        QtNative.runAction(() -> {
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
        });
    }

    @UsedFromNativeCode
    public void bringChildToBack(int id)
    {
        QtNative.runAction(() -> {
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
        });
    }
}
