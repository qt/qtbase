// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.opengl.GLES20;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

@SuppressLint("ViewConstructor")
class QtSurface extends SurfaceView implements SurfaceHolder.Callback
{
    private final QtSurfaceInterface m_surfaceCallback;
    private boolean m_backgroundPrimed = false;

    QtSurface(Context context, QtSurfaceInterface surfaceCallback, boolean onTop, int imageDepth)
    {
        super(context);
        setFocusable(false);
        setFocusableInTouchMode(false);
        setZOrderMediaOverlay(onTop);
        m_surfaceCallback = surfaceCallback;
        SurfaceHolder holder = getHolder();
        holder.setFormat(imageDepth == 16 ? PixelFormat.RGB_565 : PixelFormat.RGBA_8888);
        holder.addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
        // Prime before Qt gets the Surface so the two producers never collide.
        // Without a frame size yet, priming waits for surfaceChanged().
        final Rect frame = holder.getSurfaceFrame();
        m_backgroundPrimed = frame != null && frame.width() > 0 && frame.height() > 0;
        if (m_backgroundPrimed)
            paintBackgroundColor(holder.getSurface());
        if (m_surfaceCallback != null)
            m_surfaceCallback.onSurfaceChanged(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        // FIXME: this can sometimes cause flicker on orientation change
        if (!m_backgroundPrimed && width > 0 && height > 0) {
            paintBackgroundColor(holder.getSurface());
            m_backgroundPrimed = true;
        }
        if (m_surfaceCallback != null)
            m_surfaceCallback.onSurfaceChanged(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        // Once we return from this function, the Surface will be destroyed,
        // so everything holding a reference to it needs to clean it up before we do that
        if (m_surfaceCallback != null)
            m_surfaceCallback.onSurfaceChanged(null);
    }

    // Fill one buffer with colorBackground to avoid a black flash. lockCanvas()
    // would leave a producer connected and break Qt's eglCreateWindowSurface().
    private void paintBackgroundColor(Surface surface)
    {
        if (surface == null || !surface.isValid())
            return;

        final EGLDisplay display = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        if (display == EGL14.EGL_NO_DISPLAY)
            return;

        final int[] version = new int[2];
        if (!EGL14.eglInitialize(display, version, 0, version, 1))
            return;

        final EGLConfig[] configs = new EGLConfig[1];
        final int[] matching = new int[1];
        final int[] configAttribs = {
            EGL14.EGL_SURFACE_TYPE, EGL14.EGL_WINDOW_BIT,
            EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
            EGL14.EGL_RED_SIZE, 8,
            EGL14.EGL_GREEN_SIZE, 8,
            EGL14.EGL_BLUE_SIZE, 8,
            EGL14.EGL_ALPHA_SIZE, 8,
            EGL14.EGL_NONE
        };

        if (!EGL14.eglChooseConfig(display, configAttribs, 0, configs, 0, 1, matching, 0))
            return;

        if (matching[0] == 0)
            return;

        final int[] contextAttribs = { EGL14.EGL_CONTEXT_CLIENT_VERSION, 2, EGL14.EGL_NONE };
        final EGLContext context = EGL14.eglCreateContext(display, configs[0],
                EGL14.EGL_NO_CONTEXT, contextAttribs, 0);
        if (context == EGL14.EGL_NO_CONTEXT)
            return;

        final EGLSurface eglSurface = EGL14.eglCreateWindowSurface(display, configs[0],
                surface, new int[] { EGL14.EGL_NONE }, 0);
        if (eglSurface == EGL14.EGL_NO_SURFACE) {
            EGL14.eglDestroyContext(display, context);
            return;
        }

        if (EGL14.eglMakeCurrent(display, eglSurface, eglSurface, context)) {
            final int color = resolveBackgroundColor();
            GLES20.glClearColor(
                    ((color >> 16) & 0xff) / 255f,
                    ((color >> 8) & 0xff) / 255f,
                    (color & 0xff) / 255f,
                    ((color >>> 24) & 0xff) / 255f);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            EGL14.eglSwapBuffers(display, eglSurface);
            EGL14.eglMakeCurrent(display,
                EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
        }

        EGL14.eglDestroySurface(display, eglSurface);
        EGL14.eglDestroyContext(display, context);
    }

    private int resolveBackgroundColor()
    {
        TypedArray typedArray = getContext().getTheme().obtainStyledAttributes(
                new int[] {
                    android.R.attr.windowSplashScreenBackground,
                    android.R.attr.colorBackground
                });
        try {
            // Android 12+ has an API for splash, so prefer that.
            final int splash = typedArray.getColor(0, 0);
            if ((splash >>> 24) != 0)
                return splash | 0xff000000;
            return typedArray.getColor(1, 0xff000000) | 0xff000000;
        } finally {
            typedArray.recycle();
        }
    }
}
