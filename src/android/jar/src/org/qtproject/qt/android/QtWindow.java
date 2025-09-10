// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Insets;

import android.view.DisplayCutout;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.WindowInsets;
import android.os.Build;

import java.util.HashMap;

@SuppressLint("ViewConstructor")
class QtWindow extends QtLayout implements QtSurfaceInterface {
    private View m_surfaceContainer;
    private View m_nativeView;
    private final HashMap<Integer, QtWindow> m_childWindows = new HashMap<>();
    private QtWindow m_parentWindow;
    private GestureDetector m_gestureDetector;
    private final QtEditText m_editText;
    private final QtInputConnection.QtInputConnectionListener m_inputConnectionListener;
    private boolean m_firstSafeMarginsDelivered = false;
    private int m_actionBarHeight = -1;

    private static native void setSurface(int windowId, Surface surface);
    private static native void safeAreaMarginsChanged(Insets insets, int id);
    static native void windowFocusChanged(boolean hasFocus, int id);
    static native void updateWindows();

    QtWindow(Context context, boolean isForeignWindow, QtWindow parentWindow,
                    QtInputConnection.QtInputConnectionListener listener)
    {
        super(context);
        setId(View.generateViewId());
        m_inputConnectionListener = listener;
        setParent(parentWindow);
        setFocusableInTouchMode(true);
        setDefaultFocusHighlightEnabled(false);
        setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);

        // Views are by default visible, but QWindows are not.
        // We should ideally pick up the actual QWindow state here,
        // but QWindowPrivate::setVisible() expects to control the
        // order of events tightly, so we need to wait for a call
        // to QAndroidPlatformWindow::setVisible().
        setVisible(false);

        if (!isForeignWindow && context instanceof Activity) {
            // TODO QTBUG-122552 - Service keyboard input not implemented
            m_editText = new QtEditText(context, listener);
            m_editText.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            LayoutParams layoutParams = new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT);
            QtNative.runAction(() -> addView(m_editText, layoutParams));
        } else {
            m_editText = null;
        }

        QtNative.runAction(() -> {
            m_gestureDetector =
                new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
                    @Override
                    public void onLongPress(MotionEvent event) {
                        QtInputDelegate.longPress(getId(), (int) event.getX(), (int) event.getY());
                    }
                });
            m_gestureDetector.setIsLongpressEnabled(true);
        });

        registerSafeAreaMarginsListener();
    }

    void registerSafeAreaMarginsListener()
    {
        if (!(getContext() instanceof QtActivityBase))
            return;

        setOnApplyWindowInsetsListener((view, insets) -> {
            WindowInsets windowInsets = view.onApplyWindowInsets(insets);
            reportSafeAreaMargins(windowInsets, getId());

            return windowInsets;
        });

        // If the window is attached, try to directly deliver root insets
        if (isAttachedToWindow()) {
            WindowInsets insets = getRootWindowInsets();
            if (insets != null) {
                getRootView().post(() -> reportSafeAreaMargins(insets, getId()));
                m_firstSafeMarginsDelivered = true;
            }
        } else { // Otherwise request it upon attachement
            addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
                @Override
                public void onViewAttachedToWindow(View view) {
                    view.removeOnAttachStateChangeListener(this);
                    view.requestApplyInsets();
                }

                @Override
                public void onViewDetachedFromWindow(View view) {}
            });
        }

        // Further, tag into pre draw to deliver safe area margins early on
        if (!m_firstSafeMarginsDelivered) {
            ViewTreeObserver.OnPreDrawListener listener = new ViewTreeObserver.OnPreDrawListener() {
                @Override
                public boolean onPreDraw() {
                    if (isAttachedToWindow()) {
                        WindowInsets insets = getRootWindowInsets();
                        if (insets != null) {
                            getViewTreeObserver().removeOnPreDrawListener(this);
                            getRootView().post(() -> reportSafeAreaMargins(insets, getId()));
                            m_firstSafeMarginsDelivered = true;
                            return true;
                        }
                    }

                    requestApplyInsets();

                    return true;
                }
            };
            getViewTreeObserver().addOnPreDrawListener(listener);
        }

        addOnLayoutChangeListener((view, l, t, r, b, oldl, oldt, oldr, oldb) -> {
            WindowInsets insets = getRootWindowInsets();
            if (insets != null)
                getRootView().post(() -> reportSafeAreaMargins(insets, getId()));
        });
    }

    @SuppressWarnings("deprecation")
    Insets getSafeInsets(View view, WindowInsets insets)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            int types = WindowInsets.Type.displayCutout() | WindowInsets.Type.systemBars();
            return insets.getInsets(types);
        }

        // Android R and older
        int left = insets.getSystemWindowInsetLeft();
        int top = insets.getSystemWindowInsetTop();
        int right = insets.getSystemWindowInsetRight();
        int bottom = insets.getSystemWindowInsetBottom();

        // Android 9 and 10 emulators don't seem to be able
        // to handle this, but let's have the logic here anyway
        DisplayCutout cutout = insets.getDisplayCutout();
        if (cutout != null) {
            left = Math.max(left, cutout.getSafeInsetLeft());
            top = Math.max(top, cutout.getSafeInsetTop());
            right = Math.max(right, cutout.getSafeInsetRight());
            bottom = Math.max(bottom, cutout.getSafeInsetBottom());
        }

        // If a theme supports an action bar, sometimes it even if it's hidden
        // the insets might report values including the bar's height, not entirely
        // sure whether it's due to a delay or a bug, either way ensure the top
        // margin doesn't include the action bar's height.
        ActionBar actionBar = ((Activity) getContext()).getActionBar();
        if (actionBar == null || !actionBar.isShowing()) {
            int topWithoutActionBar = top - actionBarHeight();
            if (topWithoutActionBar > 0)
                top = topWithoutActionBar;
        }

        return Insets.of(left, top, right, bottom);
    }

    private void reportSafeAreaMargins(WindowInsets insets, int id)
    {
        View rootView = getRootView();

        int[] rootLocation = new int[2];
        rootView.getLocationOnScreen(rootLocation);
        int rootX = rootLocation[0];
        int rootY = rootLocation[1];

        int[] windowLocation = new int[2];
        getLocationOnScreen(windowLocation);
        int windowX = windowLocation[0];
        int windowY = windowLocation[1];

        // Offset values of window from root
        int leftOffset = windowX - rootX;
        int topOffset = windowY - rootY;
        int rightOffset = (rootX + rootView.getWidth()) - (windowX + getWidth());
        int bottomOffset = (rootY + rootView.getHeight()) - (windowY + getHeight());

        // Find the remaining minimum safe margins
        Insets safeInsets = getSafeInsets(rootView, insets);
        int left = Math.max(0, Math.min(safeInsets.left, safeInsets.left - leftOffset));
        int top = Math.max(0, Math.min(safeInsets.top, safeInsets.top - topOffset));
        int right = Math.max(0, Math.min(safeInsets.right, safeInsets.right - rightOffset));
        int bottom = Math.max(0, Math.min(safeInsets.bottom, safeInsets.bottom - bottomOffset));

        safeAreaMarginsChanged(Insets.of(left, top, right, bottom), id);
    }

    private int actionBarHeight()
    {
        if (m_actionBarHeight == -1) {
            TypedArray ta = getContext().getTheme().obtainStyledAttributes(
                new int[] { android.R.attr.actionBarSize });
            try {
                m_actionBarHeight = ta.getDimensionPixelSize(0, 0);
            } finally {
                ta.recycle();
            }
        }
        return m_actionBarHeight;
    }

    @UsedFromNativeCode
    void setVisible(boolean visible) {
        QtNative.runAction(() -> setVisibility(visible ? View.VISIBLE : View.INVISIBLE));
    }

    @Override
    public void onSurfaceChanged(Surface surface)
    {
        setSurface(getId(), surface);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        windowFocusChanged(true, getId());
        if (m_editText != null && m_inputConnectionListener != null)
            m_inputConnectionListener.onEditTextChanged(m_editText);

        QtInputDelegate.sendTouchEvent(event, getId());
        m_gestureDetector.onTouchEvent(event);
        return true;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent event)
    {
        QtInputDelegate.sendTrackballEvent(event, getId());
        return true;
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
    {
        return QtInputDelegate.sendGenericMotionEvent(event, getId());
    }

    @UsedFromNativeCode
    void removeWindow()
    {
        if (m_parentWindow != null)
            m_parentWindow.removeChildWindow(getId());
    }

    @UsedFromNativeCode
    void createSurface(final boolean onTop,
                              final int imageDepth, final boolean isOpaque,
                              final int surfaceContainerType) // TODO constant for type
    {
        QtNative.runAction(()-> {
            if (m_surfaceContainer != null)
                removeView(m_surfaceContainer);

            if (surfaceContainerType == 0) {
                m_surfaceContainer = new QtSurface(getContext(), QtWindow.this,
                                                   onTop, imageDepth);
            } else {
                m_surfaceContainer = new QtTextureView(getContext(), QtWindow.this, isOpaque);
            }
             m_surfaceContainer.setLayoutParams(new QtLayout.LayoutParams(
                                                        ViewGroup.LayoutParams.MATCH_PARENT,
                                                        ViewGroup.LayoutParams.MATCH_PARENT));
            // The surface container of this window will be added as the first of the stack.
            // All other views are stacked based on the order they are created.
            addView(m_surfaceContainer, 0);
        });
    }

    @UsedFromNativeCode
    void destroySurface()
    {
        QtNative.runAction(()-> {
            if (m_surfaceContainer != null) {
                removeView(m_surfaceContainer);
                m_surfaceContainer = null;
                }
        }, false);
    }

    @UsedFromNativeCode
    void setGeometry(final int x, final int y, final int w, final int h)
    {
        QtNative.runAction(()-> {
            if (getContext() instanceof QtActivityBase)
                setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
        });
    }

    void addChildWindow(QtWindow window)
    {
        QtNative.runAction(()-> {
            m_childWindows.put(window.getId(), window);
            addView(window, getChildCount());
        });
    }

    void removeChildWindow(int id)
    {
        QtNative.runAction(()-> {
            if (m_childWindows.containsKey(id))
                removeView(m_childWindows.remove(id));
        });
    }

    @UsedFromNativeCode
    void setNativeView(final View view)
    {
        QtNative.runAction(()-> {
            if (m_nativeView != null)
                removeView(m_nativeView);

            m_nativeView = view;
            m_nativeView.setLayoutParams(new QtLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                                   ViewGroup.LayoutParams.MATCH_PARENT));
            addView(m_nativeView);
        });
    }

    @UsedFromNativeCode
    void bringChildToFront(int id)
    {
        QtNative.runAction(()-> {
            View view = m_childWindows.get(id);
            if (view != null) {
                if (getChildCount() > 0)
                    moveChild(view, getChildCount() - 1);
            }
        });
    }

    @UsedFromNativeCode
    void bringChildToBack(int id) {
        QtNative.runAction(()-> {
            View view = m_childWindows.get(id);
            if (view != null) {
                moveChild(view, 0);
            }
        });
    }

    @UsedFromNativeCode
    void removeNativeView()
    {
        QtNative.runAction(()-> {
            if (m_nativeView != null) {
                removeView(m_nativeView);
                m_nativeView = null;
            }
        });
    }

    void setParent(QtWindow parentWindow)
    {
        if (m_parentWindow == parentWindow)
            return;

        if (m_parentWindow != null)
            m_parentWindow.removeChildWindow(getId());

        m_parentWindow = parentWindow;
        if (m_parentWindow != null)
            m_parentWindow.addChildWindow(this);
    }

    @UsedFromNativeCode
    void updateFocusedEditText()
    {
        if (m_editText != null && m_inputConnectionListener != null)
            m_inputConnectionListener.onEditTextChanged(m_editText);
    }
}
