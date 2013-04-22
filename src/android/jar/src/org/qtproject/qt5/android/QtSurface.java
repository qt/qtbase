/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Android port of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

package org.qtproject.qt5.android;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.PixelFormat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class QtSurface extends SurfaceView implements SurfaceHolder.Callback
{
    private Bitmap m_bitmap = null;
    private boolean m_started = false;
    private boolean m_usesGL = false;
    private GestureDetector m_gestureDetector;

    public QtSurface(Context context, int id)
    {
        super(context);
        setFocusable(true);
        getHolder().addCallback(this);
        getHolder().setType(SurfaceHolder.SURFACE_TYPE_GPU);
        setId(id);
        m_gestureDetector =
            new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
                public void onLongPress(MotionEvent event) {
                    if (!m_started)
                        return;
                    QtNative.longPress(getId(), (int) event.getX(), (int) event.getY());
                }
            });
        m_gestureDetector.setIsLongpressEnabled(true);
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
            m_bitmap = Bitmap.createBitmap(getWidth(), getHeight(), Bitmap.Config.RGB_565);
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
                                              metrics.heightPixels, getWidth(), getHeight(), metrics.xdpi, metrics.ydpi, metrics.scaledDensity);

        if (m_usesGL)
            holder.setFormat(PixelFormat.RGBA_8888);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        if (width<1 || height<1)
            return;

        DisplayMetrics metrics = new DisplayMetrics();
        ((Activity) getContext()).getWindowManager().getDefaultDisplay().getMetrics(metrics);
        QtNative.setApplicationDisplayMetrics(metrics.widthPixels,
                                              metrics.heightPixels,
                                              width,
                                              height,
                                              metrics.xdpi,
                                              metrics.ydpi,
                                              metrics.scaledDensity);

        if (!m_started)
            return;

        if (m_usesGL) {
            QtNative.setSurface(holder.getSurface());
        } else {
            QtNative.lockSurface();
            QtNative.setSurface(null);
            m_bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
            QtNative.setSurface(m_bitmap);
            QtNative.unlockSurface();
            QtNative.updateWindow();
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
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
                Canvas cv = getHolder().lockCanvas(rect);
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
        m_gestureDetector.onTouchEvent(event);
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
