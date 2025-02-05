// Copyright (C) 2018 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.graphics.Point;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.PopupWindow;

// Helper class that manages a cursor or selection handle
class EditPopupMenu implements ViewTreeObserver.OnPreDrawListener, View.OnLayoutChangeListener,
        EditContextView.OnClickListener
{
    private final EditContextView m_view;
    private final QtEditText m_editText;

    private PopupWindow m_popup = null;
    private final Activity m_activity;
    private int m_posX;
    private int m_posY;
    private int m_buttons;

    EditPopupMenu(QtEditText editText)
    {
        m_activity = (Activity) editText.getContext();
        m_view = new EditContextView(m_activity, this);
        m_view.addOnLayoutChangeListener(this);

        m_editText = editText;
        m_editText.getViewTreeObserver().addOnPreDrawListener(this);
    }

    private void initOverlay()
    {
        if (m_popup != null)
            return;

        m_popup = new PopupWindow(m_activity, null, android.R.attr.textSelectHandleWindowStyle);
        m_popup.setSplitTouchEnabled(true);
        m_popup.setClippingEnabled(false);
        m_popup.setContentView(m_view);
        m_popup.setWidth(ViewGroup.LayoutParams.WRAP_CONTENT);
        m_popup.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
    }

    // Show the handle at a given position (or move it if it is already shown)
    void setPosition(final int x, final int y, final int buttons)
    {
        initOverlay();

        m_view.updateButtons(buttons);
        Point viewSize = m_view.getCalculatedSize();

        final int[] layoutLocation = new int[2];

        // Since QtEditText doesn't match the QtWindow size, we should use its parent for
        // EditPopupMenu positioning. However, there may be cases where the parent is
        // not set. In such cases, we need to use QtEditText instead.
        View positioningView = (View) m_editText.getParent();
        if (positioningView == null)
            positioningView = m_editText;
        positioningView.getLocationOnScreen(layoutLocation);

        // These values are used for handling split screen case
        final int[] activityLocation = new int[2];
        final int[] activityLocationInWindow = new int[2];
        m_activity.getWindow().getDecorView().getLocationOnScreen(activityLocation);
        m_activity.getWindow().getDecorView().getLocationInWindow(activityLocationInWindow);

        int x2 = x + layoutLocation[0] - activityLocation[0];
        int y2 = y + layoutLocation[1] + (activityLocationInWindow[1] - activityLocation[1]);

        x2 -= viewSize.x / 2 ;

        y2 -= viewSize.y;
        if (y2 < 0)
            y2 = m_editText.getSelectionHandleBottom();

        if (y2 <= 0) {
            try {
                QtLayout parentLayout = (QtLayout) m_editText.getParent();
                parentLayout.requestLayout();
            } catch (ClassCastException e) {
                Log.w(QtNative.QtTAG, "QtEditText " + m_editText + " parent is not a QtLayout, " +
                                      "requestLayout() skipped");
            } catch (NullPointerException e) {
                Log.w(QtNative.QtTAG, "QtEditText " + m_editText + " does not have a parent, " +
                                      "requestLayout() skipped");
            }
        }

        if (positioningView.getWidth() < x + viewSize.x / 2)
            x2 = positioningView.getWidth() - viewSize.x;

        if (x2 < 0)
            x2 = 0;

        if (m_popup.isShowing())
            m_popup.update(x2, y2, -1, -1);
        else
            m_popup.showAtLocation(positioningView, 0, x2, y2);

        m_posX = x;
        m_posY = y;
        m_buttons = buttons;
    }

    void hide() {
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
            setPosition(m_posX, m_posY, m_buttons);

        return true;
    }

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom,
                               int oldLeft, int oldTop, int oldRight, int oldBottom)
    {
        if ((right - left != oldRight - oldLeft || bottom - top != oldBottom - oldTop) &&
                m_popup != null && m_popup.isShowing())
            setPosition(m_posX, m_posY, m_buttons);
    }

    @Override
    public void contextButtonClicked(int buttonId) {
        switch (buttonId) {
        case android.R.string.cut:
            QtNativeInputConnection.cut();
            break;
        case android.R.string.copy:
            QtNativeInputConnection.copy();
            break;
        case android.R.string.paste:
            QtNativeInputConnection.paste();
            break;
        case android.R.string.selectAll:
            QtNativeInputConnection.selectAll();
            break;
        }
        hide();
    }
}
