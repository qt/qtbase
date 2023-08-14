// Copyright (C) 2018 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;


import android.content.Context;
import android.graphics.Point;
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

    public Point getCalculatedSize()
    {
        Point size = new Point(0, 0);
        for (ContextButton b : m_buttons.values()) {
            if (b.getVisibility() == View.VISIBLE) {
                b.measure(0, 0);
                size.x += b.getMeasuredWidth();
                size.y = Math.max(size.y, b.getMeasuredHeight());
            }
        }

        size.x += getPaddingLeft() + getPaddingRight();
        size.y += getPaddingTop() + getPaddingBottom();

        return size;
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
