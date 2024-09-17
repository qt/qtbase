// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;

import java.util.HashMap;

class QtWindow extends QtLayout implements QtSurfaceInterface {
    private View m_surfaceContainer;
    private View m_nativeView;
    private final HashMap<Integer, QtWindow> m_childWindows = new HashMap<>();
    private QtWindow m_parentWindow;
    private GestureDetector m_gestureDetector;
    private final QtEditText m_editText;
    private final QtInputConnection.QtInputConnectionListener m_inputConnectionListener;

    private static native void setSurface(int windowId, Surface surface);
    static native void windowFocusChanged(boolean hasFocus, int id);

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
            QtNative.runAction(() -> {
                addView(m_editText,
                        new QtLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                                                  ViewGroup.LayoutParams.WRAP_CONTENT));
            });
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
    }

    @UsedFromNativeCode
    void setVisible(boolean visible) {
        QtNative.runAction(() -> {
            if (visible)
                setVisibility(View.VISIBLE);
            else
                setVisibility(View.INVISIBLE);
        });
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

        event.setLocation(event.getX() + getX(), event.getY() + getY());
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
