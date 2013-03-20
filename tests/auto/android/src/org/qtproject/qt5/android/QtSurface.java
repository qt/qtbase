/*
    Copyright (c) 2012, BogDan Vatra <bogdan@kde.org>
    Contact: http://www.qt-project.org/legal

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package org.qtproject.qt5.android;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class QtSurface extends SurfaceView implements SurfaceHolder.Callback
{
    private Bitmap m_bitmap=null;
    private boolean m_started = false;
    private boolean m_usesGL = false;
    public QtSurface(Context context, int id)
    {
        super(context);
        setFocusable(true);
        getHolder().addCallback(this);
        getHolder().setType(SurfaceHolder.SURFACE_TYPE_GPU);
        setId(id);
    }

    public void applicationStarted(boolean usesGL)
    {
        m_started = true;
        m_usesGL = usesGL;
        if (getWidth() < 1 ||  getHeight() < 1)
            return;
        if (m_usesGL) {
            QtNative.setSurface(getHolder().getSurface());
        } else {
            QtNative.lockSurface();
            QtNative.setSurface(null);
            m_bitmap=Bitmap.createBitmap(getWidth(), getHeight(), Bitmap.Config.RGB_565);
            QtNative.setSurface(m_bitmap);
            QtNative.unlockSurface();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
        DisplayMetrics metrics = new DisplayMetrics();
        ((Activity) getContext()).getWindowManager().getDefaultDisplay().getMetrics(metrics);
        QtNative.setApplicationDisplayMetrics(metrics.widthPixels,
            metrics.heightPixels, getWidth(), getHeight(), metrics.xdpi, metrics.ydpi);

        if (m_usesGL)
            holder.setFormat(PixelFormat.RGBA_8888);
        else
            holder.setFormat(PixelFormat.RGB_565);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        Log.i(QtNative.QtTAG,"surfaceChanged: "+width+","+height);
        if (width < 1 || height < 1)
                return;

        DisplayMetrics metrics = new DisplayMetrics();
        ((Activity) getContext()).getWindowManager().getDefaultDisplay().getMetrics(metrics);
        QtNative.setApplicationDisplayMetrics(metrics.widthPixels,
            metrics.heightPixels, width, height, metrics.xdpi, metrics.ydpi);

        if (!m_started)
            return;

        if (m_usesGL) {
            QtNative.setSurface(holder.getSurface());
        } else {
            QtNative.lockSurface();
            QtNative.setSurface(null);
            m_bitmap=Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
            QtNative.setSurface(m_bitmap);
            QtNative.unlockSurface();
            QtNative.updateWindow();
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        Log.i(QtNative.QtTAG,"surfaceDestroyed ");
        if (m_usesGL) {
            QtNative.destroySurface();
        } else {
            if (!m_started)
                return;

            QtNative.lockSurface();
            QtNative.setSurface(null);
            QtNative.unlockSurface();
        }
    }

    public void drawBitmap(Rect rect)
    {
        if (!m_started)
            return;
        QtNative.lockSurface();
        if (null != m_bitmap) {
            try {
                Canvas cv=getHolder().lockCanvas(rect);
                cv.drawBitmap(m_bitmap, rect, rect, null);
                getHolder().unlockCanvasAndPost(cv);
            } catch (Exception e) {
                Log.e(QtNative.QtTAG, "Can't create main activity", e);
            }
        }
        QtNative.unlockSurface();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        if (!m_started)
            return false;
        QtNative.sendTouchEvent(event, getId());
        return true;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent event)
    {
        if (!m_started)
            return false;
        QtNative.sendTrackballEvent(event, getId());
        return true;
    }
}
