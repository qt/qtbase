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
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.os.Build;

import java.util.HashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

@SuppressLint("ViewConstructor")
class QtWindow extends QtLayout implements QtSurfaceInterface
{
    private View m_surfaceContainer;
    // Read from native code on the Qt GUI thread, written on the UI thread.
    private volatile Surface m_surface;
    private boolean m_nativeSurfaceRetired = false;
    private final ConcurrentLinkedQueue<Runnable> m_pendingSurfaceReleases = new ConcurrentLinkedQueue<>();
    private View m_nativeView;
    private final HashMap<Integer, QtWindow> m_childWindows = new HashMap<>();
    private QtWindow m_parentWindow;
    private GestureDetector m_gestureDetector;
    private final QtEditText m_editText;
    private boolean m_editTextFocusInitialized = false;
    private final QtInputConnection.QtInputConnectionListener m_inputConnectionListener;
    private boolean m_firstSafeMarginsDelivered = false;
    private WindowInsets m_pendingSafeAreaInsets;
    private int m_actionBarHeight = -1;

    private static native void surfaceCreated(int windowId, Surface surface);
    private static native void surfaceRedrawNeeded(int windowId, Runnable drawingFinished);
    private static native void surfaceDestroyed(int windowId, Runnable surfaceReleased);
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

        // Views are by default visible, but QWindows are not. Set directly, the
        // view isn't attached yet, so no queued action can undo a later commit.
        setVisibility(View.INVISIBLE);

        if (!isForeignWindow && context instanceof Activity) {
            // TODO QTBUG-122552 - Service keyboard input not implemented
            m_editText = new QtEditText(context, listener);
            m_editText.setFocusable(false);
            m_editText.setFocusableInTouchMode(false);
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
                        QtInputDelegate.longPress(getId(), (int) event.getX(), (int) event.getY(),
                                                  event.getMetaState());
                    }
                });
            m_gestureDetector.setIsLongpressEnabled(true);
        });

        registerSafeAreaMarginsListener();
    }

    // Reports safe margins for this window and its children. Also called from
    // the decor listener, since the insets dispatch can be consumed above us.
    void reportSafeAreaMarginsFromRoot(WindowInsets freshInsets)
    {
        if (!isAttachedToWindow())
            return;
        // Post so the layout settles before measuring. Inside a dispatch the
        // root insets are still the old ones, so use the dispatched insets.
        getRootView().post(() -> {
            WindowInsets insets = freshInsets != null ? freshInsets : getRootWindowInsets();
            if (insets != null)
                updateSafeAreaMargins(insets);
        });
        for (QtWindow child : m_childWindows.values())
            child.reportSafeAreaMarginsFromRoot(freshInsets);
    }

    void registerSafeAreaMarginsListener()
    {
        if (!(getContext() instanceof QtActivityBase))
            return;

        setOnApplyWindowInsetsListener((view, insets) -> {
            WindowInsets windowInsets = view.onApplyWindowInsets(insets);
            updateSafeAreaMargins(windowInsets);

            return windowInsets;
        });

        // If the window is attached, try to directly deliver root insets
        if (isAttachedToWindow()) {
            WindowInsets insets = getRootWindowInsets();
            if (insets != null) {
                getRootView().post(() -> updateSafeAreaMargins(insets));
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
                            getRootView().post(() -> updateSafeAreaMargins(insets));
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
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b)
    {
        // Report before the children are laid out, so Qt gets the safe area of
        // this pass before a Surface the pass creates.
        reportSafeAreaMargins();
        super.onLayout(changed, l, t, r, b);
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

    private void updateSafeAreaMargins(WindowInsets insets)
    {
        if (getVisibility() != View.VISIBLE)
            return;
        // Retain fresh animation insets until the pending bounds are applied.
        m_pendingSafeAreaInsets = insets;
        if (!isLayoutRequested() || isInLayout())
            reportSafeAreaMargins();
    }

    private void reportSafeAreaMargins()
    {
        WindowInsets insets = m_pendingSafeAreaInsets;
        m_pendingSafeAreaInsets = null;
        if (getVisibility() != View.VISIBLE || !(getContext() instanceof QtActivityBase))
            return;
        if (insets == null)
            insets = getRootWindowInsets();
        if (insets == null)
            return;
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

        safeAreaMarginsChanged(Insets.of(left, top, right, bottom), getId());
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

    @Override
    public void onSurfaceCreated(Surface surface)
    {
        m_surface = surface;
        surfaceCreated(getId(), surface);
    }

    @Override
    public void onSurfaceRedrawNeeded(Runnable drawingFinished)
    {
        surfaceRedrawNeeded(getId(), drawingFinished);
    }

    @Override
    public void onSurfaceDestroyed(Runnable surfaceReleased)
    {
        m_surface = null;
        if (m_nativeSurfaceRetired) {
            surfaceReleased.run();
            return;
        }
        m_pendingSurfaceReleases.add(surfaceReleased);
        surfaceDestroyed(getId(), () -> {
            if (m_pendingSurfaceReleases.remove(surfaceReleased))
                surfaceReleased.run();
        });
    }

    // Lets the Qt GUI thread fetch the current Surface synchronously instead
    // of waiting for the queued surfaceCreated dispatch to land.
    @UsedFromNativeCode
    Surface getSurface()
    {
        return m_surface;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        // Enable focus for the edit text on first touch event to avoid
        // early QtInputConnection callbacks from blocking the UI thread.
        if (!m_editTextFocusInitialized) {
            m_editTextFocusInitialized = true;
            if (m_editText != null) {
                m_editText.setFocusable(true);
                m_editText.setFocusableInTouchMode(true);
            }
        }

        windowFocusChanged(true, getId());
        if (m_editText != null && m_inputConnectionListener != null)
            m_inputConnectionListener.onEditTextChanged(m_editText);

        QtInputDelegate.sendTouchEvent(event, getId());
        if (m_gestureDetector != null)
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

    @Override
    protected void onAttachedToWindow()
    {
        super.onAttachedToWindow();
        setOnDragListener(QtDragManager.getInstance());
    }

    @Override
    protected void onDetachedFromWindow()
    {
        QtDragManager.getInstance().onSourceWindowDetached(this);
        setOnDragListener(null);
        super.onDetachedFromWindow();
    }

    @UsedFromNativeCode
    void removeWindow()
    {
        if (m_parentWindow != null)
            m_parentWindow.removeChildWindow(getId());
    }

    // Qt's latest values, applied together once per UI thread turn, so the
    // view system never lays out an intermediate state. Guarded by this.
    private int m_x, m_y, m_width, m_height;
    private boolean m_visible;
    private boolean m_needsSurface, m_surfaceOnTop, m_surfaceOpaque;
    private int m_surfaceContainerType;
    private final Runnable m_applyUpdate = this::applyUpdate;

    @UsedFromNativeCode
    void configureSurface(boolean onTop, boolean isOpaque, int containerType)
    {
        synchronized (this) {
            m_needsSurface = true;
            m_surfaceOnTop = onTop;
            m_surfaceOpaque = isOpaque;
            m_surfaceContainerType = containerType;
        }
    }

    @UsedFromNativeCode
    void updateWindow(int x, int y, int width, int height, boolean visible)
    {
        synchronized (this) {
            m_x = x;
            m_y = y;
            m_width = width;
            m_height = height;
            m_visible = visible;
        }
        removeCallbacks(m_applyUpdate);
        post(m_applyUpdate);
    }

    private void applyUpdate()
    {
        final int x, y, width, height, containerType;
        final boolean visible, needsSurface, onTop, isOpaque;
        synchronized (this) {
            x = m_x;
            y = m_y;
            width = m_width;
            height = m_height;
            visible = m_visible;
            needsSurface = m_needsSurface;
            onTop = m_surfaceOnTop;
            isOpaque = m_surfaceOpaque;
            containerType = m_surfaceContainerType;
        }
        if (getContext() instanceof QtActivityBase && !hasLayoutGeometry(x, y, width, height))
            setLayoutParams(new QtLayout.LayoutParams(width, height, x, y));
        // Added after the geometry, so the first Surface has the final size.
        if (visible && needsSurface && m_surfaceContainer == null)
            addSurfaceContainer(onTop, isOpaque, containerType);
        final boolean shown = visible && getVisibility() != View.VISIBLE;
        setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
        // A hidden view reports nothing, so report once when shown.
        if (shown)
            requestLayout();
    }

    private boolean hasLayoutGeometry(int x, int y, int w, int h)
    {
        if (!(getLayoutParams() instanceof QtLayout.LayoutParams))
            return false;
        QtLayout.LayoutParams params = (QtLayout.LayoutParams) getLayoutParams();
        return params.x == x && params.y == y && params.width == w && params.height == h;
    }

    private void addSurfaceContainer(boolean onTop, boolean isOpaque, int surfaceContainerType)
    {
        if (surfaceContainerType == 0)
            m_surfaceContainer = new QtSurface(getContext(), QtWindow.this, onTop, 32);
        else
            m_surfaceContainer = new QtTextureView(getContext(), QtWindow.this, isOpaque);
        m_surfaceContainer.setLayoutParams(new QtLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        // The surface container of this window will be added as the first of the stack.
        // All other views are stacked based on the order they are created.
        addView(m_surfaceContainer, 0);
    }

    @UsedFromNativeCode
    void destroySurface()
    {
        QtNative.runAction(this::destroySurfaceOnUiThread, false);
    }

    private void destroySurfaceOnUiThread()
    {
        // Native teardown precedes this action, including delayed callbacks.
        m_nativeSurfaceRetired = true;
        Throwable removalFailure = null;
        try {
            if (m_surfaceContainer != null) {
                removeView(m_surfaceContainer);
                m_surfaceContainer = null;
            }
        } catch (RuntimeException | Error failure) {
            removalFailure = failure;
            throw failure;
        } finally {
            try {
                releasePendingSurfaces();
            } catch (RuntimeException | Error failure) {
                if (removalFailure == null)
                    throw failure;
                if (removalFailure != failure)
                    removalFailure.addSuppressed(failure);
            }
        }
    }

    private void releasePendingSurfaces()
    {
        Throwable firstFailure = null;
        Runnable released;
        while ((released = m_pendingSurfaceReleases.poll()) != null) {
            try {
                released.run();
            } catch (RuntimeException | Error failure) {
                if (firstFailure == null)
                    firstFailure = failure;
                else if (firstFailure != failure)
                    firstFailure.addSuppressed(failure);
            }
        }
        if (firstFailure instanceof RuntimeException)
            throw (RuntimeException) firstFailure;
        if (firstFailure instanceof Error)
            throw (Error) firstFailure;
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
