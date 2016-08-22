/****************************************************************************
**
** Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Android port of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

package org.qtproject.qt5.android;

import android.content.Context;
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

// Helper class that manages a cursor or selection handle
public class EditPopupMenu implements ViewTreeObserver.OnPreDrawListener, OnClickListener
{
    private View m_layout = null;
    private View m_view = null;
    private PopupWindow m_popup = null;
    private Activity m_activity;
    private int m_posX;
    private int m_posY;

    public EditPopupMenu(Activity activity, View layout) {
        m_activity = activity;
        m_layout = layout;
    }

    private boolean initOverlay(){
        if (m_popup == null){
            Context context = m_layout.getContext();
            int[] attrs = { android.R.attr.textEditPasteWindowLayout };
            TypedArray a = context.getTheme().obtainStyledAttributes(attrs);
            final int layout = a.getResourceId(0, 0);
            LayoutInflater inflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            m_view = inflater.inflate(layout, null);

            final int size = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
            m_view.setLayoutParams(new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                                                    ViewGroup.LayoutParams.WRAP_CONTENT));
            m_view.measure(size, size);
            m_view.setOnClickListener(this);

            m_popup = new PopupWindow(context, null, android.R.attr.textSelectHandleWindowStyle);
            m_popup.setSplitTouchEnabled(true);
            m_popup.setClippingEnabled(false);
            m_popup.setContentView(m_view);
            m_popup.setWidth(ViewGroup.LayoutParams.WRAP_CONTENT);
            m_popup.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);

            m_layout.getViewTreeObserver().addOnPreDrawListener(this);
        }
        return true;
    }

    public int getHeight()
    {
        initOverlay();
        return m_view.getHeight();
    }

    // Show the handle at a given position (or move it if it is already shown)
    public void setPosition(final int x, final int y){
        initOverlay();

        final int[] location = new int[2];
        m_layout.getLocationOnScreen(location);

        int x2 = x + location[0];
        int y2 = y + location[1];

        x2 -= m_view.getWidth() / 2 ;
        if (x2 < 0)
            x2 = 0;
        y2 -= m_view.getHeight();

        if (m_popup.isShowing())
            m_popup.update(x2, y2, -1, -1);
        else
            m_popup.showAtLocation(m_layout, 0, x2, y2);

        m_posX = x;
        m_posY = y;

    }

    public void hide() {
        if (m_popup != null) {
            m_popup.dismiss();
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

    @Override
    public void onClick(View v) {
        QtNativeInputConnection.paste();
        hide();
    }
}

