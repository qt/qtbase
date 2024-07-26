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
import android.graphics.drawable.Drawable;
import android.graphics.Rect;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.Display;
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

import java.util.HashMap;

import org.qtproject.qt.android.accessibility.QtAccessibilityDelegate;

public class QtActivityDelegate extends QtActivityDelegateBase
{
    private static final String QtTAG = "QtActivityDelegate";

    private QtRootLayout m_layout = null;
    private ImageView m_splashScreen = null;
    private boolean m_splashScreenSticky = false;

    private View m_dummyView = null;
    private HashMap<Integer, View> m_nativeViews = new HashMap<Integer, View>();


    QtActivityDelegate(Activity activity)
    {
        super(activity);

        setActionBarVisibility(false);
        setActivityBackgroundDrawable();
    }


    @UsedFromNativeCode
    @Override
    QtLayout getQtLayout()
    {
        return m_layout;
    }

    @UsedFromNativeCode
    @Override
    void setSystemUiVisibility(int systemUiVisibility)
    {
        QtNative.runAction(() -> {
            m_displayManager.setSystemUiVisibility(systemUiVisibility);
            m_layout.requestLayout();
            QtNative.updateWindow();
        });
    }

    @Override
    void startNativeApplicationImpl(String appParams, String mainLib)
    {
        m_layout.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        QtNative.startApplication(appParams, mainLib);
                        m_layout.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                    }
                });
    }

    @Override
    protected void setUpLayout()
    {
        int orientation = m_activity.getResources().getConfiguration().orientation;
        m_layout = new QtRootLayout(m_activity);

        setUpSplashScreen(orientation);
        m_activity.registerForContextMenu(m_layout);
        m_activity.setContentView(m_layout,
                                  new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                             ViewGroup.LayoutParams.MATCH_PARENT));
        QtDisplayManager.handleOrientationChanges(m_activity);

        handleUiModeChange(m_activity.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK);

        Display display = (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
                ? m_activity.getWindowManager().getDefaultDisplay()
                : m_activity.getDisplay();
        QtDisplayManager.handleRefreshRateChanged(QtDisplayManager.getRefreshRate(display));

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

    @Override
    protected void setUpSplashScreen(int orientation)
    {
        try {
            ActivityInfo info = m_activity.getPackageManager().getActivityInfo(
                                                                    m_activity.getComponentName(),
                                                                    PackageManager.GET_META_DATA);

            String splashScreenKey = "android.app.splash_screen_drawable_"
                + (orientation == Configuration.ORIENTATION_LANDSCAPE ? "landscape" : "portrait");
            if (!info.metaData.containsKey(splashScreenKey))
                splashScreenKey = "android.app.splash_screen_drawable";

            if (info.metaData.containsKey(splashScreenKey)) {
                m_splashScreenSticky =
                    info.metaData.containsKey("android.app.splash_screen_sticky") &&
                    info.metaData.getBoolean("android.app.splash_screen_sticky");

                int id = info.metaData.getInt(splashScreenKey);
                m_splashScreen = new ImageView(m_activity);
                m_splashScreen.setImageDrawable(m_activity.getResources().getDrawable(
                                                                        id, m_activity.getTheme()));
                m_splashScreen.setScaleType(ImageView.ScaleType.FIT_XY);
                m_splashScreen.setLayoutParams(new ViewGroup.LayoutParams(
                                                            ViewGroup.LayoutParams.MATCH_PARENT,
                                                            ViewGroup.LayoutParams.MATCH_PARENT));
                m_layout.addView(m_splashScreen);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void hideSplashScreen(final int duration)
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
    public void initializeAccessibility()
    {
        QtNative.runAction(() -> {
            // FIXME make QtAccessibilityDelegate window based
            if (m_layout != null)
                m_accessibilityDelegate = new QtAccessibilityDelegate(m_layout);
            else
                Log.w(QtTAG, "Null layout, failed to initialize accessibility delegate.");
        });
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
    @Override
    public void openContextMenu(final int x, final int y, final int w, final int h)
    {
        m_layout.postDelayed(() -> {
            final QtEditText focusedEditText = m_inputDelegate.getCurrentQtEditText();
            if (focusedEditText == null) {
                Log.w(QtTAG, "No focused view when trying to open context menu");
                return;
            }
            m_layout.setLayoutParams(focusedEditText, new QtLayout.LayoutParams(w, h, x, y), false);
            PopupMenu popup = new PopupMenu(m_activity, focusedEditText);
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

    @Override
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
    @Override
    public void addTopLevelWindow(final QtWindow window)
    {
        if (window == null)
            return;

        QtNative.runAction(()-> {
            if (m_topLevelWindows.size() == 0) {
                if (m_dummyView != null) {
                    m_layout.removeView(m_dummyView);
                    m_dummyView = null;
                }
            }

            window.setLayoutParams(new ViewGroup.LayoutParams(
                                            ViewGroup.LayoutParams.MATCH_PARENT,
                                            ViewGroup.LayoutParams.MATCH_PARENT));

            m_layout.addView(window, m_topLevelWindows.size());
            m_topLevelWindows.put(window.getId(), window);
            if (!m_splashScreenSticky)
                hideSplashScreen();
        });
    }

    @UsedFromNativeCode
    @Override
    void removeTopLevelWindow(final int id)
    {
        QtNative.runAction(()-> {
            if (m_topLevelWindows.containsKey(id)) {
                QtWindow window = m_topLevelWindows.remove(id);
                if (m_topLevelWindows.isEmpty()) {
                   // Keep last frame in stack until it is replaced to get correct
                   // shutdown transition
                   m_dummyView = window;
               } else {
                   m_layout.removeView(window);
               }
            }
        });
    }

    @UsedFromNativeCode
    @Override
    void bringChildToFront(final int id)
    {
        QtNative.runAction(() -> {
            QtWindow window = m_topLevelWindows.get(id);
            if (window != null)
                m_layout.moveChild(window, m_topLevelWindows.size() - 1);
        });
    }

    @UsedFromNativeCode
    @Override
    void bringChildToBack(int id)
    {
        QtNative.runAction(() -> {
            QtWindow window = m_topLevelWindows.get(id);
            if (window != null)
                m_layout.moveChild(window, 0);
        });
    }

    @Override
    QtAccessibilityDelegate createAccessibilityDelegate()
    {
        if (m_layout != null)
            return new QtAccessibilityDelegate(m_layout);

        Log.w(QtTAG, "Null layout, failed to initialize accessibility delegate.");
        return null;
    }

    private void setActivityBackgroundDrawable()
    {
        TypedValue attr = new TypedValue();
        m_activity.getTheme().resolveAttribute(android.R.attr.windowBackground,
                                               attr, true);
        Drawable backgroundDrawable;
        if (attr.type >= TypedValue.TYPE_FIRST_COLOR_INT &&
            attr.type <= TypedValue.TYPE_LAST_COLOR_INT) {
                backgroundDrawable = new ColorDrawable(attr.data);
        } else {
            backgroundDrawable = m_activity.getResources().
                                    getDrawable(attr.resourceId, m_activity.getTheme());
        }

        m_activity.getWindow().setBackgroundDrawable(backgroundDrawable);
    }

    // TODO: QTBUG-122761 To be removed after QtAndroidAutomotive does not depend on it.
    @UsedFromNativeCode
    public void insertNativeView(int id, View view, int x, int y, int w, int h)
    {
        QtNative.runAction(()-> {
            if (m_dummyView != null) {
                m_layout.removeView(m_dummyView);
                m_dummyView = null;
            }

            if (m_nativeViews.containsKey(id))
                m_layout.removeView(m_nativeViews.remove(id));

            if (w < 0 || h < 0) {
                view.setLayoutParams(new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            } else {
                view.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
            }

            view.setId(id);
            m_layout.addView(view);
            m_nativeViews.put(id, view);
        });
    }

    // TODO: QTBUG-122761 To be removed after QtAndroidAutomotive does not depend on it.
    @UsedFromNativeCode
    public void setNativeViewGeometry(int id, int x, int y, int w, int h)
    {
        QtNative.runAction(() -> {
            if (m_nativeViews.containsKey(id)) {
                View view = m_nativeViews.get(id);
                view.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
            } else {
                Log.e(QtTAG, "View " + id + " not found!");
            }
        });
    }
}
