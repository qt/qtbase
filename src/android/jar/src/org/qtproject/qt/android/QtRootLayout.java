// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Display;

/**
    A layout which corresponds to one Activity, i.e. is the root layout where the top level window
    and handles orientation changes.
*/
public class QtRootLayout extends QtLayout
{

    private int m_activityDisplayRotation = -1;
    private int m_ownDisplayRotation = -1;
    private int m_nativeOrientation = -1;

    public QtRootLayout(Context context)
    {
        super(context);
    }

    public void setActivityDisplayRotation(int rotation)
    {
        m_activityDisplayRotation = rotation;
    }

    public void setNativeOrientation(int orientation)
    {
        m_nativeOrientation = orientation;
    }

    public int displayRotation()
    {
        return m_ownDisplayRotation;
    }

    @Override
    protected void onSizeChanged (int w, int h, int oldw, int oldh)
    {
        Activity activity = (Activity)getContext();
        if (activity == null)
            return;

        DisplayMetrics realMetrics = new DisplayMetrics();
        Display display = (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
                ? activity.getWindowManager().getDefaultDisplay()
                : activity.getDisplay();

        if (display == null)
            return;

        display.getRealMetrics(realMetrics);
        if ((realMetrics.widthPixels > realMetrics.heightPixels) != (w > h)) {
            // This is an intermediate state during display rotation.
            // The new size is still reported for old orientation, while
            // realMetrics contain sizes for new orientation. Setting
            // such parameters will produce inconsistent results, so
            // we just skip them.
            // We will have another onSizeChanged() with normal values
            // a bit later.
            return;
        }

        QtDisplayManager.setApplicationDisplayMetrics(activity, w, h);
        QtDisplayManager.handleOrientationChanges(activity, true);
    }
}
