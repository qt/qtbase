/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Android port of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

package org.qtproject.qt5.android;

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

        if (android.os.Build.VERSION.SDK_INT < 11)
            getHolder().setType(SurfaceHolder.SURFACE_TYPE_GPU);

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
}
