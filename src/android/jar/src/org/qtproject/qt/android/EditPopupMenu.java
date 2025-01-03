// Copyright (C) 2018 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.graphics.Point;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ImageView;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.view.MotionEvent;
import android.widget.PopupWindow;
import android.app.Activity;
import android.view.ViewTreeObserver;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.view.ViewGroup;
import android.R;

// Helper class that manages a cursor or selection handle
public class EditPopupMenu implements ViewTreeObserver.OnPreDrawListener, View.OnLayoutChangeListener,
        EditContextView.OnClickListener
{
    private View m_layout = null;
    private EditContextView m_view = null;
    private PopupWindow m_popup = null;
    private Activity m_activity;
    private int m_posX;
    private int m_posY;
    private int m_buttons;
    private CursorHandle m_cursorHandle;
    private CursorHandle m_leftSelectionHandle;
    private CursorHandle m_rightSelectionHandle;

    public EditPopupMenu(Activity activity, View layout)
    {
        m_activity = activity;
        m_view = new EditContextView(activity, this);
        m_view.addOnLayoutChangeListener(this);

        m_layout = layout;
    }

    private void initOverlay()
    {
        if (m_popup != null)
            return;

        Context context = m_layout.getContext();
        m_popup = new PopupWindow(context, null, android.R.attr.textSelectHandleWindowStyle);
        m_popup.setSplitTouchEnabled(true);
        m_popup.setClippingEnabled(false);
        m_popup.setContentView(m_view);
        m_popup.setWidth(ViewGroup.LayoutParams.WRAP_CONTENT);
        m_popup.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);

        m_layout.getViewTreeObserver().addOnPreDrawListener(this);
    }

    // Show the handle at a given position (or move it if it is already shown)
    public void setPosition(final int x, final int y, final int buttons,
                            CursorHandle cursorHandle, CursorHandle leftSelectionHandle, CursorHandle rightSelectionHandle)
    {
        initOverlay();

        m_view.updateButtons(buttons);
        Point viewSize = m_view.getCalculatedSize();

        final int[] layoutLocation = new int[2];
        m_layout.getLocationOnScreen(layoutLocation);

        // These values are used for handling split screen case
        final int[] activityLocation = new int[2];
        final int[] activityLocationInWindow = new int[2];
        m_activity.getWindow().getDecorView().getLocationOnScreen(activityLocation);
        m_activity.getWindow().getDecorView().getLocationInWindow(activityLocationInWindow);

        int x2 = x + layoutLocation[0] - activityLocation[0];
        int y2 = y + layoutLocation[1] + (activityLocationInWindow[1] - activityLocation[1]);

        x2 -= viewSize.x / 2 ;

        y2 -= viewSize.y;
        if (y2 < 0) {
            if (cursorHandle != null) {
                y2 = cursorHandle.bottom();
            } else if (leftSelectionHandle != null && rightSelectionHandle != null) {
                y2 = Math.max(leftSelectionHandle.bottom(), rightSelectionHandle.bottom());
                if (y2 <= 0)
                    m_layout.requestLayout();
            }
        }

        if (m_layout.getWidth() < x + viewSize.x / 2)
            x2 = m_layout.getWidth() - viewSize.x;

        if (x2 < 0)
            x2 = 0;

        if (m_popup.isShowing())
            m_popup.update(x2, y2, -1, -1);
        else
            m_popup.showAtLocation(m_layout, 0, x2, y2);

        m_posX = x;
        m_posY = y;
        m_buttons = buttons;
        m_cursorHandle = cursorHandle;
        m_leftSelectionHandle = leftSelectionHandle;
        m_rightSelectionHandle = rightSelectionHandle;
    }

    public void hide() {
        if (m_popup != null) {
            m_popup.dismiss();
            m_popup = null;
        }
    }

    @Override
    public boolean onPreDraw() {
        // This hook is called when the view location is changed
        // For example if the keyboard appears.
        // Adjust the position of the handle accordingly
        if (m_popup != null && m_popup.isShowing())
            setPosition(m_posX, m_posY, m_buttons, m_cursorHandle, m_leftSelectionHandle, m_rightSelectionHandle);

        return true;
    }

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom,
                               int oldLeft, int oldTop, int oldRight, int oldBottom)
    {
        if ((right - left != oldRight - oldLeft || bottom - top != oldBottom - oldTop) &&
                m_popup != null && m_popup.isShowing())
            setPosition(m_posX, m_posY, m_buttons, m_cursorHandle, m_leftSelectionHandle, m_rightSelectionHandle);
    }

    @Override
    public void contextButtonClicked(int buttonId) {
        switch (buttonId) {
        case R.string.cut:
            QtNativeInputConnection.cut();
            break;
        case R.string.copy:
            QtNativeInputConnection.copy();
            break;
        case R.string.paste:
            QtNativeInputConnection.paste();
            break;
        case R.string.selectAll:
            QtNativeInputConnection.selectAll();
            break;
        }
        hide();
    }
}
