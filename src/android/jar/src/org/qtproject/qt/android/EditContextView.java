// Copyright (C) 2018 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;


import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Point;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.HashMap;

@SuppressLint("ViewConstructor")
class EditContextView extends LinearLayout implements View.OnClickListener
{
    static final int CUT_BUTTON   = 1;
    static final int COPY_BUTTON  = 1 << 1;
    static final int PASTE_BUTTON = 1 << 2;
    static final int SELECT_ALL_BUTTON = 1 << 3;

    final HashMap<Integer, ContextButton> m_buttons = new HashMap<>(4);
    final OnClickListener m_onClickListener;

    interface OnClickListener
    {
        void contextButtonClicked(int buttonId);
    }

    private class ContextButton extends TextView
    {
        int m_buttonId;
        ContextButton(Context context, int stringId) {
            super(context);
            m_buttonId = stringId;
            setText(stringId);
            setLayoutParams(new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            setGravity(Gravity.CENTER);
            setTextColor(getResources().getColor(
                    android.R.color.widget_edittext_dark, context.getTheme()));
            EditContextView.this.setBackground(getResources().getDrawable(
                    android.R.drawable.editbox_background_normal, context.getTheme()));
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

    void updateButtons(int buttonsLayout)
    {
        ContextButton button = m_buttons.get(android.R.string.cut);
        if (button != null)
            button.setVisibility((buttonsLayout & CUT_BUTTON) != 0 ? View.VISIBLE : View.GONE);

        button = m_buttons.get(android.R.string.copy);
        if (button != null)
            button.setVisibility((buttonsLayout & COPY_BUTTON) != 0 ? View.VISIBLE : View.GONE);

        button = m_buttons.get(android.R.string.paste);
        if (button != null)
            button.setVisibility((buttonsLayout & PASTE_BUTTON) != 0 ? View.VISIBLE : View.GONE);

        button = m_buttons.get(android.R.string.selectAll);
        if (button != null)
            button.setVisibility((buttonsLayout & SELECT_ALL_BUTTON) != 0 ? View.VISIBLE : View.GONE);
    }

    Point getCalculatedSize()
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

    EditContextView(Context context, OnClickListener onClickListener) {
        super(context);
        m_onClickListener = onClickListener;
        setLayoutParams(new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        addButton(android.R.string.cut);
        addButton(android.R.string.copy);
        addButton(android.R.string.paste);
        addButton(android.R.string.selectAll);
    }
}
