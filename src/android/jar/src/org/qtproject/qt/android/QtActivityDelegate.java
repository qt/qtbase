// Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.Rect;
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
import android.widget.ImageView;
import android.widget.PopupMenu;

import java.util.HashMap;

class QtActivityDelegate extends QtActivityDelegateBase
        implements QtWindowInterface, QtAccessibilityInterface, QtMenuInterface,
            QtNative.AppStateDetailsListener
{
    private static final String QtTAG = "QtActivityDelegate";

    private QtRootLayout m_layout = null;
    private ImageView m_splashScreen = null;
    private boolean m_splashScreenSticky = false;
    private boolean m_backendsRegistered = false;

    private View m_dummyView = null;
    private final HashMap<Integer, View> m_nativeViews = new HashMap<>();

    QtActivityDelegate(Activity activity)
    {
        super(activity);
    }

    @Override
    void initMembers()
    {
        super.initMembers();
        setActionBarVisibility(false);
        setActivityBackgroundDrawable();
        if (QtNativeAccessibility.accessibilitySupported())
            m_accessibilityDelegate.initLayoutAccessibility(m_layout);
    }

    void registerBackends()
    {
        if (m_backendsRegistered || BackendRegister.isNull())
            return;

        m_backendsRegistered = true;
        BackendRegister.registerBackend(QtWindowInterface.class, QtActivityDelegate.this);
        BackendRegister.registerBackend(QtAccessibilityInterface.class, QtActivityDelegate.this);
        BackendRegister.registerBackend(QtMenuInterface.class, QtActivityDelegate.this);
        BackendRegister.registerBackend(QtInputInterface.class, m_inputDelegate);
    }

    void unregisterBackends()
    {
        if (!m_backendsRegistered)
            return;

        m_backendsRegistered = false;

        if (BackendRegister.isNull())
            return;

        BackendRegister.unregisterBackend(QtWindowInterface.class);
        BackendRegister.unregisterBackend(QtAccessibilityInterface.class);
        BackendRegister.unregisterBackend(QtMenuInterface.class);
        BackendRegister.unregisterBackend(QtInputInterface.class);
    }

    @Override
    public void setSystemUiVisibility(boolean isFullScreen, boolean expandedToCutout)
    {
        if (m_layout == null)
            return;

        QtNative.runAction(() -> {
            if (m_layout != null) {
                m_displayManager.setSystemUiVisibility(isFullScreen, expandedToCutout);
                QtWindow.updateWindows();
            }
        });
    }


    @Override
    final public void onAppStateDetailsChanged(QtNative.ApplicationStateDetails details) {
        if (details.isStarted)
            registerBackends();
        else
            unregisterBackends();
    }

    @Override
    void startNativeApplicationImpl(String appParams, String mainLib)
    {
        if (m_layout == null) {
            Log.e(QtTAG, "Unable to start native application with a null layout");
            return;
        }

        m_layout.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        if (m_layout != null) {
                            QtNative.startApplication(appParams, mainLib);
                            m_layout.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                        }
                    }
                });
    }

    @Override
    protected void setUpLayout()
    {
        // This should be assigned only once, otherwise, we'd have to check
        // for null everywhere including before and inside runAction() Runnables.
        m_layout = new QtRootLayout(m_activity);

        int orientation = m_activity.getResources().getConfiguration().orientation;
        setUpSplashScreen(orientation);
        m_activity.registerForContextMenu(m_layout);
        m_activity.setContentView(m_layout,
                                  new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                             ViewGroup.LayoutParams.MATCH_PARENT));

        handleUiModeChange(m_activity.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK);

        m_displayManager.initDisplayProperties();

        m_layout.getViewTreeObserver().addOnPreDrawListener(() -> {
            if (!m_inputDelegate.isKeyboardVisible())
                return true;

            Rect r = new Rect();
            m_activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
            DisplayMetrics metrics = new DisplayMetrics();
            QtDisplayManager.getDisplay(m_activity).getMetrics(metrics);
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
    }

    @Override
    protected void setUpSplashScreen(int orientation)
    {
        if (m_layout == null) {
            Log.e(QtTAG, "Unable to setup splash screen with a null layout");
            return;
        }

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

            if (m_layout != null && duration <= 0) {
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

    @Override
    public void notifyLocationChange(int viewId)
    {
        m_accessibilityDelegate.notifyLocationChange(viewId);
    }

    @Override
    public void notifyObjectHide(int viewId, int parentId)
    {
        m_accessibilityDelegate.notifyObjectHide(viewId, parentId);
    }

    @Override
    public void notifyObjectShow(int parentId)
    {
        m_accessibilityDelegate.notifyObjectShow(parentId);
    }

    @Override
    public void notifyObjectFocus(int viewId)
    {
        m_accessibilityDelegate.notifyObjectFocus(viewId);
    }

    @Override
    public void notifyValueChanged(int viewId, String value)
    {
        m_accessibilityDelegate.notifyValueChanged(viewId, value);
    }

    @Override public void notifyDescriptionOrNameChanged(int viewId, String value)
    {
        m_accessibilityDelegate.notifyDescriptionOrNameChanged(viewId, value);
    }

    @Override
    public void notifyScrolledEvent(int viewId)
    {
        m_accessibilityDelegate.notifyScrolledEvent(viewId);
    }

    @Override
    public void notifyAnnouncementEvent(int viewId, String message)
    {
        m_accessibilityDelegate.notifyAnnouncementEvent(viewId, message);
    }

    // QtMenuInterface implementation begin
    @Override
    public void resetOptionsMenu()
    {
        QtNative.runAction(m_activity::invalidateOptionsMenu);
    }

    @Override
    public void openOptionsMenu()
    {
        QtNative.runAction(m_activity::openOptionsMenu);
    }

    @Override
    public void closeContextMenu()
    {
        QtNative.runAction(m_activity::closeContextMenu);
    }

    @Override
    public void openContextMenu(final int x, final int y, final int w, final int h)
    {
        if (m_layout == null) {
            Log.e(QtTAG, "Unable to open context menu with a null layout");
            return;
        }

        m_layout.postDelayed(() -> {
            if (m_layout == null) {
                Log.w(QtTAG, "Unable to open context menu on null layout");
                return;
            }
            final QtEditText focusedEditText = m_inputDelegate.getCurrentQtEditText();
            if (focusedEditText == null) {
                Log.w(QtTAG, "No focused view when trying to open context menu");
                return;
            }
            m_layout.setLayoutParams(focusedEditText, new QtLayout.LayoutParams(w, h, x, y), false);
            focusedEditText.requestLayout();
            focusedEditText.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        focusedEditText.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                        PopupMenu popup = new PopupMenu(m_activity, focusedEditText);
                        QtActivityDelegate.this.onCreatePopupMenu(popup.getMenu());
                        popup.setOnMenuItemClickListener(m_activity::onContextItemSelected);
                        popup.setOnDismissListener(popupMenu ->
                                m_activity.onContextMenuClosed(popupMenu.getMenu()));
                        popup.show();
                    }
            });
        }, 100);
    }
    // QtMenuInterface implementation end

    void onCreatePopupMenu(Menu menu)
    {
        QtNative.fillContextMenu(menu);
        setContextMenuVisible(true);
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
        if (m_layout == null || window == null)
            return;

        QtNative.runAction(()-> {
            if (m_layout == null)
                return;

            if (m_topLevelWindows.isEmpty()) {
                if (m_dummyView != null) {
                    m_layout.removeView(m_dummyView);
                    m_dummyView = null;
                }
            }

            m_layout.addView(window, m_topLevelWindows.size());
            m_topLevelWindows.put(window.getId(), window);
            if (!m_splashScreenSticky)
                hideSplashScreen();
        });
    }

    @UsedFromNativeCode
    @Override
    public void removeTopLevelWindow(final int id)
    {
        QtNative.runAction(()-> {
            if (m_topLevelWindows.containsKey(id)) {
                QtWindow window = m_topLevelWindows.remove(id);
                window.setOnApplyWindowInsetsListener(null); // Set in QtWindow for safe margins
                if (m_topLevelWindows.isEmpty()) {
                   // Keep last frame in stack until it is replaced to get correct
                   // shutdown transition
                   m_dummyView = window;
               } else if (m_layout != null) {
                   m_layout.removeView(window);
               }
            }
        });
    }

    @UsedFromNativeCode
    @Override
    public void bringChildToFront(final int id)
    {
        if (m_layout != null) {
            QtNative.runAction(() -> {
                QtWindow window = m_topLevelWindows.get(id);
                if (window != null && m_layout != null)
                    m_layout.moveChild(window, m_topLevelWindows.size() - 1);
            });
        }
    }

    @UsedFromNativeCode
    @Override
    public void bringChildToBack(int id)
    {
        if (m_layout != null) {
            QtNative.runAction(() -> {
                QtWindow window = m_topLevelWindows.get(id);
                if (window != null && m_layout != null)
                    m_layout.moveChild(window, 0);
            });
        }
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
    void insertNativeView(int id, View view, int x, int y, int w, int h)
    {
        if (m_layout == null)
            return;

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
    void setNativeViewGeometry(int id, int x, int y, int w, int h)
    {
        QtNative.runAction(() -> {
            if (m_nativeViews.containsKey(id)) {
                View view = m_nativeViews.get(id);
                if (view != null)
                    view.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
            } else {
                Log.e(QtTAG, "View " + id + " not found!");
            }
        });
    }
}
