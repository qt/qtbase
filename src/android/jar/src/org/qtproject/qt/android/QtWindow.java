// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;

import java.util.HashMap;

public class QtWindow extends QtLayout implements QtSurface.SurfaceChangedCallback {
    private final static String TAG = "QtWindow";

    private QtSurface m_surface;
    private View m_nativeView;
    private Handler m_androidHandler;

    private static native void setSurface(int windowId, Surface surface);

    public QtWindow(Context context)
    {
        super(context);
        setId(View.generateViewId());
    }

    @Override
    public void onSurfaceChanged(Surface surface)
    {
        setSurface(getId(), surface);
    }

    public void createSurface(final boolean onTop,
                              final int x, final int y, final int w, final int h,
                              final int imageDepth)
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                if (m_surface != null)
                    removeView(m_surface);

                QtSurface surface = new QtSurface(getContext(),
                                                  QtWindow.this, QtWindow.this.getId(),
                                                  onTop, imageDepth);
                surface.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));

                addView(surface, 0);
                m_surface = surface;
            }
        });
    }

    public void destroySurface()
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                if (m_surface != null) {
                    removeView(m_surface);
                    m_surface = null;
                }
            }
        });
    }

    public void setSurfaceGeometry(final int x, final int y, final int w, final int h)
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                QtLayout.LayoutParams lp = new QtLayout.LayoutParams(w, h, x, y);
                if (m_surface != null)
                    m_surface.setLayoutParams(lp);
                else if (m_nativeView != null)
                    m_nativeView.setLayoutParams(lp);
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
                    removeView(m_nativeView);

                m_nativeView = view;
                m_nativeView.setLayoutParams(new QtLayout.LayoutParams(w, h, x, y));

                addView(m_nativeView);
            }
        });
    }

    public void removeNativeView()
    {
        QtNative.runAction(new Runnable() {
            @Override
            public void run() {
                if (m_nativeView != null) {
                    removeView(m_nativeView);
                    m_nativeView = null;
                }
            }
        });
    }
}
