// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Insets;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;

/**
    A layout which corresponds to one Activity, i.e. is the root layout where the top level window
    and handles orientation changes.
*/
class QtRootLayout extends QtLayout
{
    private static final int s_viewId = View.generateViewId();
    private Insets m_safeAreaInsets = Insets.NONE;
    private boolean m_insetContent = true;

    QtRootLayout(Context context)
    {
        super(context, s_viewId);
    }

    static QtRootLayout findIn(Activity activity)
    {
        View root = activity.findViewById(s_viewId);
        return root instanceof QtRootLayout ? (QtRootLayout)root : null;
    }

    @Override
    public WindowInsets onApplyWindowInsets(WindowInsets insets)
    {
        if (QtWindowInsetsController.isEdgeToEdgeEnforced(getContext())) {
            // Child insets can include the IME under adjustResize.
            final WindowInsets rootInsets = getRootWindowInsets();
            final int types = WindowInsets.Type.systemBars() | WindowInsets.Type.displayCutout();
            m_safeAreaInsets = (rootInsets != null ? rootInsets : insets).getInsets(types);
        }
        updateMargins();
        return super.onApplyWindowInsets(insets);
    }

    void setContentInsetEnabled(boolean enabled)
    {
        m_insetContent = enabled;
        updateMargins();
    }

    private void updateMargins()
    {
        if (!QtWindowInsetsController.isEdgeToEdgeEnforced(getContext()))
            return;

        final ViewGroup.LayoutParams layoutParams = getLayoutParams();
        if (!(layoutParams instanceof ViewGroup.MarginLayoutParams))
            return;

        // Inset only when Android overrides normal window geometry.
        Insets margins = m_insetContent ? m_safeAreaInsets : Insets.NONE;
        ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams)layoutParams;
        final boolean leftMarginChanged = params.leftMargin != margins.left;
        final boolean topMarginChanged = params.topMargin != margins.top;
        final boolean rightMarginChanged = params.rightMargin != margins.right;
        final boolean bottomMarginChanged = params.bottomMargin != margins.bottom;
        if (!leftMarginChanged && !topMarginChanged && !rightMarginChanged && !bottomMarginChanged)
            return;

        params.setMargins(margins.left, margins.top, margins.right, margins.bottom);
        setLayoutParams(params);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh)
    {
        Activity activity = (Activity) getContext();
        if (activity == null || (w == oldw && h == oldh))
            return;

        QtDisplayManager.handleLayoutSizeChanged(w, h);
    }

    @Override
    public void onConfigurationChanged(Configuration configuration)
    {
        Activity activity = (Activity) getContext();
        if (activity == null)
            return;

        // Post the orientation handling just in case the onSizeChanged() is
        // called a bit late (QTBUG-94459).
        QtNative.runAction(() -> {
            QtDisplayManager.handleOrientationChange(activity);
        });
    }
}
