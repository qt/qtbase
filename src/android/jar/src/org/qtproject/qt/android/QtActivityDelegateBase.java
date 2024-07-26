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

abstract class QtActivityDelegateBase
{
    protected Activity m_activity;
    protected HashMap<Integer, QtWindow> m_topLevelWindows;
    protected QtAccessibilityDelegate m_accessibilityDelegate = null;
    protected QtDisplayManager m_displayManager = null;
    protected QtInputDelegate m_inputDelegate = null;

    private boolean m_membersInitialized = false;
    private boolean m_contextMenuVisible = false;

    // Subclass must implement these
    abstract void startNativeApplicationImpl(String appParams, String mainLib);
    abstract QtAccessibilityDelegate createAccessibilityDelegate();
    abstract QtLayout getQtLayout();

    // With these we are okay with default implementation doing nothing
    void setUpLayout() {}
    void setUpSplashScreen(int orientation) {}
    void hideSplashScreen(final int duration) {}
    void openContextMenu(final int x, final int y, final int w, final int h) {}
    void setActionBarVisibility(boolean visible) {}
    void addTopLevelWindow(final QtWindow window) {}
    void removeTopLevelWindow(final int id) {}
    void bringChildToFront(final int id) {}
    void bringChildToBack(int id) {}
    void setSystemUiVisibility(int systemUiVisibility) {}

    QtActivityDelegateBase(Activity activity)
    {
        m_activity = activity;
        // Set native context
        QtNative.setActivity(m_activity);
    }

    QtDisplayManager displayManager() {
        return m_displayManager;
    }

    @UsedFromNativeCode
    QtInputDelegate getInputDelegate() {
        return m_inputDelegate;
    }

    void setContextMenuVisible(boolean contextMenuVisible)
    {
        m_contextMenuVisible = contextMenuVisible;
    }

    boolean isContextMenuVisible()
    {
        return m_contextMenuVisible;
    }

    public void startNativeApplication(String appParams, String mainLib)
    {
        if (m_membersInitialized)
            return;
        initMembers();
        startNativeApplicationImpl(appParams, mainLib);
    }

    void initMembers()
    {
        m_membersInitialized = true;
        m_topLevelWindows = new HashMap<Integer, QtWindow>();

        m_displayManager = new QtDisplayManager(m_activity);
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

        setUpLayout();
    }

    public void hideSplashScreen()
    {
        hideSplashScreen(0);
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
    public void notifyObjectShow(int parentId)
    {
        if (m_accessibilityDelegate == null)
           return;
        m_accessibilityDelegate.notifyObjectShow(parentId);
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
        QtNative.runAction(() -> {
            m_accessibilityDelegate = createAccessibilityDelegate();
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

    public void onCreatePopupMenu(Menu menu)
    {
        QtNative.fillContextMenu(menu);
        m_contextMenuVisible = true;
    }

    @UsedFromNativeCode
    public void closeContextMenu()
    {
        QtNative.runAction(() -> m_activity.closeContextMenu());
    }
}
