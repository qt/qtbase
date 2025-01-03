// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ImageView;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.view.MotionEvent;
import android.widget.PopupWindow;
import android.app.Activity;
import android.util.TypedValue;
import android.view.ViewTreeObserver;

/* This view represents one of the handle (selection or cursor handle) */
class CursorView extends ImageView
{
    private CursorHandle mHandle;
    // The coordinare which where clicked
    private float m_offsetX;
    private float m_offsetY;
    private boolean m_pressed = false;

    CursorView (Context context, CursorHandle handle) {
        super(context);
        mHandle = handle;
    }

    // Called when the handle was moved programmatically , with the delta amount in pixels
    public void adjusted(int dx, int dy) {
        m_offsetX += dx;
        m_offsetY += dy;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        switch (ev.getActionMasked()) {
        case MotionEvent.ACTION_DOWN: {
            m_offsetX = ev.getRawX();
            m_offsetY = ev.getRawY() + getHeight() / 2;
            m_pressed = true;
            break;
        }

        case MotionEvent.ACTION_MOVE: {
            if (!m_pressed)
                return false;
            mHandle.updatePosition(Math.round(ev.getRawX() - m_offsetX),
                                    Math.round(ev.getRawY() - m_offsetY));
            break;
        }

        case MotionEvent.ACTION_UP:
        case MotionEvent.ACTION_CANCEL:
            m_pressed = false;
            break;
        }
        return true;
    }

}

// Helper class that manages a cursor or selection handle
public class CursorHandle implements ViewTreeObserver.OnPreDrawListener
{
    private View m_layout = null;
    private CursorView m_cursorView = null;
    private PopupWindow m_popup = null;
    private int m_id;
    private int m_attr;
    private Activity m_activity;
    private int m_posX = 0;
    private int m_posY = 0;
    private int m_lastX;
    private int m_lastY;
    int tolerance;
    private boolean m_rtl;
    int m_yShift;

    public CursorHandle(Activity activity, View layout, int id, int attr, boolean rtl) {
        m_activity = activity;
        m_id = id;
        m_attr = attr;
        m_layout = layout;
        DisplayMetrics metrics = activity.getResources().getDisplayMetrics();
        m_yShift = (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_MM, 1f, metrics);
        tolerance = Math.min(1, (int)(m_yShift / 2f));
        m_lastX = m_lastY = -1 - tolerance;
        m_rtl = rtl;
    }

    private boolean initOverlay(){
        if (m_popup == null){

            Context context = m_layout.getContext();
            int[] attrs = {m_attr};
            TypedArray a = context.getTheme().obtainStyledAttributes(attrs);
            Drawable drawable = a.getDrawable(0);

            m_cursorView = new CursorView(context, this);
            m_cursorView.setImageDrawable(drawable);

            m_popup = new PopupWindow(context, null, android.R.attr.textSelectHandleWindowStyle);
            m_popup.setSplitTouchEnabled(true);
            m_popup.setClippingEnabled(false);
            m_popup.setContentView(m_cursorView);
            m_popup.setWidth(drawable.getIntrinsicWidth());
            m_popup.setHeight(drawable.getIntrinsicHeight());

            m_layout.getViewTreeObserver().addOnPreDrawListener(this);
        }
        return true;
    }

    // Show the handle at a given position (or move it if it is already shown)
    public void setPosition(final int x, final int y){
        initOverlay();

        final int[] layoutLocation = new int[2];
        m_layout.getLocationOnScreen(layoutLocation);

        // These values are used for handling split screen case
        final int[] activityLocation = new int[2];
        final int[] activityLocationInWindow = new int[2];
        m_activity.getWindow().getDecorView().getLocationOnScreen(activityLocation);
        m_activity.getWindow().getDecorView().getLocationInWindow(activityLocationInWindow);

        int x2 = x + layoutLocation[0] - activityLocation[0];
        int y2 = y + layoutLocation[1] + m_yShift + (activityLocationInWindow[1] - activityLocation[1]);

        if (m_id == QtNative.IdCursorHandle) {
            x2 -= m_popup.getWidth() / 2 ;
        } else if ((m_id == QtNative.IdLeftHandle && !m_rtl) || (m_id == QtNative.IdRightHandle && m_rtl)) {
            x2 -= m_popup.getWidth() * 3 / 4;
        } else {
            x2 -= m_popup.getWidth() / 4;
        }

        if (m_popup.isShowing()) {
            m_popup.update(x2, y2, -1, -1);
            m_cursorView.adjusted(x - m_posX, y - m_posY);
        } else {
            m_popup.showAtLocation(m_layout, 0, x2, y2);
        }

        m_posX = x;
        m_posY = y;
    }

    public int bottom()
    {
        initOverlay();
        final int[] location = new int[2];
        m_cursorView.getLocationOnScreen(location);
        return location[1] + m_cursorView.getHeight();
    }

    public void hide() {
        if (m_popup != null) {
            m_popup.dismiss();
        }
    }

    public int width()
    {
        return m_cursorView.getDrawable().getIntrinsicWidth();
    }

    // The handle was dragged by a given relative position
    public void updatePosition(int x, int y) {
        y -= m_yShift;
        if (Math.abs(m_lastX - x) > tolerance || Math.abs(m_lastY - y) > tolerance) {
            QtNative.handleLocationChanged(m_id, x + m_posX, y + m_posY);
            m_lastX = x;
            m_lastY = y;
        }
    }

    @Override
    public boolean onPreDraw() {
        // This hook is called when the view location is changed
        // For example if the keyboard appears.
        // Adjust the position of the handle accordingly
        if (m_popup != null && m_popup.isShowing())
            setPosition(m_posX, m_posY);

        return true;
    }
}
