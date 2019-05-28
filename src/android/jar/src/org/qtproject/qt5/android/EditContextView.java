/****************************************************************************
**
** Copyright (C) 2018 BogDan Vatra <bogdan@kde.org>
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
import android.text.TextUtils;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.R;

import java.util.HashMap;

public class EditContextView extends LinearLayout implements View.OnClickListener
{
    public static final int CUT_BUTTON   = 1 << 0;
    public static final int COPY_BUTTON  = 1 << 1;
    public static final int PASTE_BUTTON = 1 << 2;
    public static final int SALL_BUTTON  = 1 << 3;

    HashMap<Integer, ContextButton> m_buttons = new HashMap<Integer, ContextButton>(4);
    OnClickListener m_onClickListener;

    public interface OnClickListener
    {
        void contextButtonClicked(int buttonId);
    }

    private class ContextButton extends TextView
    {
        public int m_buttonId;
        public ContextButton(Context context, int stringId) {
            super(context);
            m_buttonId = stringId;
            setText(stringId);
            setLayoutParams(new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            setGravity(Gravity.CENTER);
            setTextColor(getResources().getColor(R.color.widget_edittext_dark));
            EditContextView.this.setBackground(getResources().getDrawable(R.drawable.editbox_background_normal));
            float scale = getResources().getDisplayMetrics().density;
            int hPadding = (int)(16 * scale + 0.5f);
            int vPadding = (int)(8 * scale + 0.5f);
            setPadding(hPadding, vPadding, hPadding, vPadding);
            setSingleLine();
            setEllipsize(TextUtils.TruncateAt.END);
            setOnClickListener(EditContextView.this);
        }
    }

    @Override
    public void onClick(View v)
    {
        ContextButton button = (ContextButton)v;
        m_onClickListener.contextButtonClicked(button.m_buttonId);
    }

    void addButton(int id)
    {
        ContextButton button = new ContextButton(getContext(), id);
        m_buttons.put(id, button);
        addView(button);
    }

    public void updateButtons(int buttonsLayout)
    {
        m_buttons.get(R.string.cut).setVisibility((buttonsLayout & CUT_BUTTON) != 0 ? View.VISIBLE : View.GONE);
        m_buttons.get(R.string.copy).setVisibility((buttonsLayout & COPY_BUTTON) != 0 ? View.VISIBLE : View.GONE);
        m_buttons.get(R.string.paste).setVisibility((buttonsLayout & PASTE_BUTTON) != 0 ? View.VISIBLE : View.GONE);
        m_buttons.get(R.string.selectAll).setVisibility((buttonsLayout & SALL_BUTTON) != 0 ? View.VISIBLE : View.GONE);
    }

    public EditContextView(Context context, OnClickListener onClickListener) {
        super(context);
        m_onClickListener = onClickListener;
        setLayoutParams(new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        addButton(R.string.cut);
        addButton(R.string.copy);
        addButton(R.string.paste);
        addButton(R.string.selectAll);
    }
}
