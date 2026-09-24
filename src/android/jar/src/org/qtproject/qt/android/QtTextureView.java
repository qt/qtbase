// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.view.Surface;
import android.view.TextureView;

@SuppressLint("ViewConstructor")
class QtTextureView extends TextureView implements TextureView.SurfaceTextureListener
{
    private final QtSurfaceInterface m_surfaceCallback;
    private boolean m_staysOnTop;
    private Surface m_surface;

    QtTextureView(Context context, QtSurfaceInterface surfaceCallback, boolean isOpaque)
    {
        super(context);
        setFocusable(false);
        setFocusableInTouchMode(false);
        m_surfaceCallback = surfaceCallback;
        setSurfaceTextureListener(this);
        setOpaque(isOpaque);
        setSurfaceTexture(new SurfaceTexture(false));
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int width, int height) {
        if (m_surface == null) {
            m_surface = new Surface(surfaceTexture);
            m_surfaceCallback.onSurfaceCreated(m_surface);
        }
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int width, int height) {
        // A SurfaceTexture set in the constructor never gets onSurfaceTextureAvailable,
        // so the first resize delivers the Surface.
        if (m_surface == null) {
            m_surface = new Surface(surfaceTexture);
            m_surfaceCallback.onSurfaceCreated(m_surface);
            return;
        }
        // HWUI shows the old buffer at the new size until Qt draws, and offers
        // nothing to wait on, so ask for a frame right away.
        m_surfaceCallback.onSurfaceRedrawNeeded(() -> {});
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
        final Surface surface = m_surface;
        m_surface = null;
        m_surfaceCallback.onSurfaceDestroyed(() -> {
            if (surface != null)
                surface.release();
            surfaceTexture.release();
        });
        return false;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
    }
}
