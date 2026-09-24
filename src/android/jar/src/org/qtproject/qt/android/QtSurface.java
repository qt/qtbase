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
import android.os.Build;
import android.view.Surface;
import android.view.SurfaceControl;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

@SuppressLint("ViewConstructor")
class QtSurface extends SurfaceView implements SurfaceHolder.Callback2
{
    private final int m_format;
    private final QtSurfaceInterface m_surfaceCallback;
    private boolean m_backgroundPrimed = false;
    private SurfaceControl m_renderControl;
    private Surface m_renderSurface;

    QtSurface(Context context, QtSurfaceInterface surfaceCallback, boolean onTop, int imageDepth)
    {
        super(context);
        setFocusable(false);
        setFocusableInTouchMode(false);
        setZOrderMediaOverlay(onTop);
        m_surfaceCallback = surfaceCallback;
        m_format = imageDepth == 16 ? PixelFormat.RGB_565 : PixelFormat.RGBA_8888;
        SurfaceHolder holder = getHolder();
        holder.setFormat(m_format);
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
        if (Build.VERSION.SDK_INT >= 29) {
            // The holder forcibly disconnects its queue when this view disappears.
            final SurfaceControl control = new SurfaceControl.Builder()
                    .setName("QtSurface")
                    .setParent(getSurfaceControl())
                    .setBufferSize(Math.max(1, getWidth()), Math.max(1, getHeight()))
                    .setFormat(m_format)
                    .build();
            try {
                initializeRenderSurface(control, new Surface(control));
            } catch (RuntimeException | Error failure) {
                try {
                    control.release();
                } catch (RuntimeException | Error cleanupFailure) {
                    failure.addSuppressed(cleanupFailure);
                }
                throw failure;
            }
        }
        if (m_surfaceCallback != null)
            m_surfaceCallback.onSurfaceCreated(
                m_renderSurface != null ? m_renderSurface : holder.getSurface());
    }

    private void initializeRenderSurface(SurfaceControl control, Surface surface)
    {
        try (SurfaceControl.Transaction transaction = new SurfaceControl.Transaction()) {
            transaction.setVisibility(control, true).apply();
        } catch (RuntimeException | Error failure) {
            try {
                surface.release();
            } catch (RuntimeException | Error cleanupFailure) {
                failure.addSuppressed(cleanupFailure);
            }
            throw failure;
        }
        m_renderControl = control;
        m_renderSurface = surface;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        if (m_renderControl != null) {
            try (SurfaceControl.Transaction transaction = new SurfaceControl.Transaction()) {
                transaction.setBufferSize(m_renderControl, width, height).apply();
            }
        }
        if (!m_backgroundPrimed && width > 0 && height > 0) {
            paintBackgroundColor(holder.getSurface());
            m_backgroundPrimed = true;
        }
    }

    @Override
    public void surfaceRedrawNeeded(SurfaceHolder holder)
    {
        // Only reached through the default surfaceRedrawNeededAsync().
    }

    @Override
    public void surfaceRedrawNeededAsync(SurfaceHolder holder, Runnable drawingFinished)
    {
        // SurfaceView's redraw synchronization still consumes a holder buffer.
        if (m_renderSurface != null)
            paintBackgroundColor(holder.getSurface());
        if (m_surfaceCallback != null)
            m_surfaceCallback.onSurfaceRedrawNeeded(drawingFinished);
        else
            drawingFinished.run();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        final Surface surface = m_renderSurface;
        final SurfaceControl control = m_renderControl;
        m_renderSurface = null;
        m_renderControl = null;
        final Runnable released = () -> {
            if (surface != null)
                surface.release();
            if (control != null)
                control.release();
        };
        Throwable detachFailure = null;
        try {
            if (control != null) {
                try (SurfaceControl.Transaction transaction = new SurfaceControl.Transaction()) {
                    transaction.reparent(control, null).apply();
                }
            }
        } catch (RuntimeException | Error failure) {
            detachFailure = failure;
            throw failure;
        } finally {
            try {
                if (m_surfaceCallback != null)
                    m_surfaceCallback.onSurfaceDestroyed(released);
                else
                    released.run();
            } catch (RuntimeException | Error failure) {
                if (detachFailure == null)
                    throw failure;
                detachFailure.addSuppressed(failure);
            }
        }
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
