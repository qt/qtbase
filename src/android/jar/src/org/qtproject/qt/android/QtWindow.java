// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;

import java.util.HashMap;

public class QtWindow implements QtSurfaceInterface, QtLayout.QtTouchListener {
    private final static String TAG = "QtWindow";

    private View m_surfaceContainer;
    private QtLayout m_layout;
    private View m_nativeView;
    private HashMap<Integer, QtWindow> m_childWindows = new HashMap<Integer, QtWindow>();
    private QtWindow m_parentWindow;
    private int m_id;
    private GestureDetector m_gestureDetector;

    private static native void setSurface(int windowId, Surface surface);

    public QtWindow(Context context, QtWindow parentWindow)
    {
        m_id = View.generateViewId();
        QtNative.runAction(() -> {
            m_layout = new QtLayout(context, this);
            setParent(parentWindow);
            m_gestureDetector =
                new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
                    public void onLongPress(MotionEvent event) {
                        QtInputDelegate.longPress(getId(), (int) event.getX(), (int) event.getY());
                    }
                });
            m_gestureDetector.setIsLongpressEnabled(true);
        });
    }

    void setVisible(boolean visible) {
        QtNative.runAction(() -> {
            if (visible)
                m_layout.setVisibility(View.VISIBLE);
            else
                m_layout.setVisibility(View.INVISIBLE);
        });
    }

    public int getId()
    {
        return m_id;
    }

    public QtLayout getLayout()
    {
        return m_layout;
    }

    @Override
    public void onSurfaceChanged(Surface surface)
    {
        setSurface(getId(), surface);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
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

    public void removeWindow()
    {
        if (m_parentWindow != null)
            m_parentWindow.removeChildWindow(getId());
    }

    public void createSurface(final boolean onTop,
                              final int x, final int y, final int w, final int h,
                              final int imageDepth, final boolean isOpaque,
                              final int surfaceContainerType) // TODO constant for type
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                if (m_surfaceContainer != null)
                    m_layout.removeView(m_surfaceContainer);

                m_layout.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
                if (surfaceContainerType == 0) {
                    m_surfaceContainer = new QtSurface(m_layout.getContext(), QtWindow.this,
                                                       onTop, imageDepth);
                } else {
                    m_surfaceContainer = new QtTextureView(m_layout.getContext(), QtWindow.this, isOpaque);
                }
                 m_surfaceContainer.setLayoutParams(new QtLayout.LayoutParams(
                                                            ViewGroup.LayoutParams.MATCH_PARENT,
                                                            ViewGroup.LayoutParams.MATCH_PARENT));
                // The surface container of this window will be added as the first of the stack.
                // All other views are stacked based on the order they are created.
                m_layout.addView(m_surfaceContainer, 0);
            }
        });
    }

    public void destroySurface()
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                if (m_surfaceContainer != null) {
                    m_layout.removeView(m_surfaceContainer);
                    m_surfaceContainer = null;
                }
            }
        });
    }

    public void setGeometry(final int x, final int y, final int w, final int h)
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                m_layout.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
            }
        });
    }

    public void addChildWindow(QtWindow window)
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                m_childWindows.put(window.getId(), window);
                m_layout.addView(window.getLayout(), m_layout.getChildCount());
            }
        });
    }

    public void removeChildWindow(int id)
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                if (m_childWindows.containsKey(id))
                    m_layout.removeView(m_childWindows.remove(id).getLayout());
            }
        });
    }

    public void setNativeView(final View view,
                              final int x, final int y, final int w, final int h)
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                if (m_nativeView != null)
                    m_layout.removeView(m_nativeView);

                m_nativeView = view;
                m_layout.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));
                m_nativeView.setLayoutParams(new QtLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                                       ViewGroup.LayoutParams.MATCH_PARENT));
                m_layout.addView(m_nativeView);
            }
        });
    }

    public void bringChildToFront(int id)
    {
        QtNative.runAction(()-> {
            QtWindow window = m_childWindows.get(id);
            if (window != null) {
                if (m_layout.getChildCount() > 0)
                    m_layout.moveChild(window.getLayout(), m_layout.getChildCount() - 1);
            }
        });
    }

    public void bringChildToBack(int id) {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                QtWindow window = m_childWindows.get(id);
                if (window != null) {
                    m_layout.moveChild(window.getLayout(), 0);
                }
            }
        });
    }

    public void removeNativeView()
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                if (m_nativeView != null) {
                    m_layout.removeView(m_nativeView);
                    m_nativeView = null;
                }
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

    QtWindow parent()
    {
        return m_parentWindow;
    }
}
