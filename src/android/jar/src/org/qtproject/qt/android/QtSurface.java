// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.PixelFormat;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

@SuppressLint("ViewConstructor")
class QtSurface extends SurfaceView implements SurfaceHolder.Callback
{
    private final GestureDetector m_gestureDetector;
    private Object m_accessibilityDelegate = null;
    private SurfaceChangedCallback m_surfaceCallback;
    private final int m_windowId;

    interface SurfaceChangedCallback {
        void onSurfaceChanged(Surface surface);
    }

    public QtSurface(Context context, SurfaceChangedCallback surfaceCallback, int id, boolean onTop,
                     int imageDepth)
    {
        super(context);
        setFocusable(false);
        setFocusableInTouchMode(false);
        setZOrderMediaOverlay(onTop);
        m_surfaceCallback = surfaceCallback;
        getHolder().addCallback(this);
        if (imageDepth == 16)
            getHolder().setFormat(PixelFormat.RGB_565);
        else
            getHolder().setFormat(PixelFormat.RGBA_8888);

        m_windowId = id;
        m_gestureDetector =
            new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
                public void onLongPress(MotionEvent event) {
                    QtInputDelegate.longPress(m_windowId, (int) event.getX(), (int) event.getY());
                }
            });
        m_gestureDetector.setIsLongpressEnabled(true);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        if (width < 1 || height < 1)
            return;
        if (m_surfaceCallback != null)
            m_surfaceCallback.onSurfaceChanged(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        if (m_surfaceCallback != null)
            m_surfaceCallback.onSurfaceChanged(null);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        // QTBUG-65927
        // Fix event positions depending on Surface position.
        // In case when Surface is moved, we should also add this move to event position
        event.setLocation(event.getX() + getX(), event.getY() + getY());

        QtInputDelegate.sendTouchEvent(event, m_windowId);
        m_gestureDetector.onTouchEvent(event);
        return true;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent event)
    {
        QtInputDelegate.sendTrackballEvent(event, m_windowId);
        return true;
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
    {
        return QtInputDelegate.sendGenericMotionEvent(event, m_windowId);
    }
}
