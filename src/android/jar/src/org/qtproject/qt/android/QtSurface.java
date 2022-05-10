// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.graphics.PixelFormat;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

public class QtSurface extends SurfaceView implements SurfaceHolder.Callback
{
    private GestureDetector m_gestureDetector;
    private Object m_accessibilityDelegate = null;

    public QtSurface(Context context, int id, boolean onTop, int imageDepth)
    {
        super(context);
        setFocusable(false);
        setFocusableInTouchMode(false);
        setZOrderMediaOverlay(onTop);
        getHolder().addCallback(this);
        if (imageDepth == 16)
            getHolder().setFormat(PixelFormat.RGB_565);
        else
            getHolder().setFormat(PixelFormat.RGBA_8888);

        setId(id);
        m_gestureDetector =
            new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
                public void onLongPress(MotionEvent event) {
                    QtNative.longPress(getId(), (int) event.getX(), (int) event.getY());
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

        QtNative.setSurface(getId(), holder.getSurface(), width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        QtNative.setSurface(getId(), null, 0, 0);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        // QTBUG-65927
        // Fix event positions depending on Surface position.
        // In case when Surface is moved, we should also add this move to event position
        event.setLocation(event.getX() + getX(), event.getY() + getY());

        QtNative.sendTouchEvent(event, getId());
        m_gestureDetector.onTouchEvent(event);
        return true;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent event)
    {
        QtNative.sendTrackballEvent(event, getId());
        return true;
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
    {
        return QtNative.sendGenericMotionEvent(event, getId());
    }
}
