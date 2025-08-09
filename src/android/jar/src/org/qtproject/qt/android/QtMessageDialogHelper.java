// Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


package org.qtproject.qt.android;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.content.ClipboardManager;
import android.text.Html;
import android.text.Spanned;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.ArrayList;

class QtNativeDialogHelper
{
    static native void dialogResult(long handler, int buttonID);
}

class ButtonStruct implements View.OnClickListener
{
    ButtonStruct(QtMessageDialogHelper dialog, int id, String text)
    {
        m_dialog = dialog;
        m_id = id;
        m_text = Html.fromHtml(text, Html.FROM_HTML_MODE_LEGACY);
    }
    final QtMessageDialogHelper m_dialog;
    private final int m_id;
    final Spanned m_text;

    @Override
    public void onClick(View view) {
        QtNativeDialogHelper.dialogResult(m_dialog.handler(), m_id);
    }
}

class QtMessageDialogHelper
{
    QtMessageDialogHelper(Activity activity)
    {
        m_activity = activity;
    }

    @UsedFromNativeCode
    void setStandardIcon(int icon)
    {
        m_standardIcon = icon;

    }

    private Drawable getIconDrawable()
    {
        if (m_standardIcon == 0)
            return null;

        // Information, Warning, Critical, Question
        switch (m_standardIcon)
        {
            case 1: // Information
                return m_activity.getResources().getDrawable(android.R.drawable.ic_dialog_info,
                        m_activity.getTheme());
            case 2: // Warning
                return m_activity.getResources().getDrawable(android.R.drawable.stat_sys_warning,
                        m_activity.getTheme());
            case 3: // Critical
                return m_activity.getResources().getDrawable(android.R.drawable.ic_dialog_alert,
                        m_activity.getTheme());
            case 4: // Question
                return m_activity.getResources().getDrawable(android.R.drawable.ic_menu_help,
                        m_activity.getTheme());
        }
        return null;
    }

    @UsedFromNativeCode
    void setTile(String title)
    {
        m_title = Html.fromHtml(title, Html.FROM_HTML_MODE_LEGACY);
    }

    @UsedFromNativeCode
    void setText(String text)
    {
        m_text = Html.fromHtml(text, Html.FROM_HTML_MODE_LEGACY);
    }

    @UsedFromNativeCode
    void setInformativeText(String informativeText)
    {
        m_informativeText = Html.fromHtml(informativeText, Html.FROM_HTML_MODE_LEGACY);
    }

    @UsedFromNativeCode
    void setDetailedText(String text)
    {
        m_detailedText = Html.fromHtml(text, Html.FROM_HTML_MODE_LEGACY);
    }

    @UsedFromNativeCode
    void addButton(int id, String text)
    {
        if (m_buttonsList == null)
            m_buttonsList = new ArrayList<>();
        m_buttonsList.add(new ButtonStruct(this, id, text));
    }

    private Drawable getStyledDrawable(int id)
    {
        int[] attrs = { id };
        Drawable d;
        TypedArray a = m_theme.obtainStyledAttributes(attrs);
        try {
            d = a.getDrawable(0);
        } finally {
            a.recycle();
        }

        return  d;
    }

    @UsedFromNativeCode
    void show(long handler)
    {
        m_handler = handler;
        m_activity.runOnUiThread(() -> {
            if (m_dialog != null && m_dialog.isShowing())
                m_dialog.dismiss();

            m_dialog = new AlertDialog.Builder(m_activity).create();
            Window window = m_dialog.getWindow();
            if (window != null)
                m_theme = window.getContext().getTheme();
            else
                Log.w(QtTAG, "show(): cannot set theme from null window!");

            if (m_title != null)
                m_dialog.setTitle(m_title);
            m_dialog.setOnCancelListener(dialogInterface -> QtNativeDialogHelper.dialogResult(handler(), -1));
            m_dialog.setCancelable(m_buttonsList == null);
            m_dialog.setCanceledOnTouchOutside(m_buttonsList == null);
            m_dialog.setIcon(getIconDrawable());
            ScrollView scrollView = new ScrollView(m_activity);
            RelativeLayout dialogLayout = new RelativeLayout(m_activity);
            int id = 1;
            View lastView = null;
            View.OnLongClickListener copyText = view -> {
                TextView tv = (TextView)view;
                if (tv != null) {
                    ClipboardManager cm = (ClipboardManager) m_activity.getSystemService(
                            Context.CLIPBOARD_SERVICE);
                    cm.setPrimaryClip(ClipData.newPlainText(tv.getText(), tv.getText()));
                }
                return true;
            };
            if (m_text != null)
            {
                TextView view = new TextView(m_activity);
                view.setId(id++);
                view.setOnLongClickListener(copyText);
                view.setLongClickable(true);

                view.setText(m_text);
                view.setTextAppearance(android.R.style.TextAppearance_Medium);

                RelativeLayout.LayoutParams layout = new RelativeLayout.LayoutParams(
                        RelativeLayout.LayoutParams.MATCH_PARENT,
                        RelativeLayout.LayoutParams.WRAP_CONTENT);
                layout.setMargins(16, 8, 16, 8);
                layout.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                dialogLayout.addView(view, layout);
                lastView = view;
            }

            if (m_informativeText != null)
            {
                TextView view= new TextView(m_activity);
                view.setId(id++);
                view.setOnLongClickListener(copyText);
                view.setLongClickable(true);

                view.setText(m_informativeText);
                view.setTextAppearance(android.R.style.TextAppearance_Medium);

                RelativeLayout.LayoutParams layout = new RelativeLayout.LayoutParams(
                        RelativeLayout.LayoutParams.MATCH_PARENT,
                        RelativeLayout.LayoutParams.WRAP_CONTENT);
                layout.setMargins(16, 8, 16, 8);
                if (lastView != null)
                    layout.addRule(RelativeLayout.BELOW, lastView.getId());
                else
                    layout.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                dialogLayout.addView(view, layout);
                lastView = view;
            }

            if (m_detailedText != null)
            {
                TextView view= new TextView(m_activity);
                view.setId(id++);
                view.setOnLongClickListener(copyText);
                view.setLongClickable(true);

                view.setText(m_detailedText);
                view.setTextAppearance(android.R.style.TextAppearance_Small);

                RelativeLayout.LayoutParams layout = new RelativeLayout.LayoutParams(
                        RelativeLayout.LayoutParams.MATCH_PARENT,
                        RelativeLayout.LayoutParams.WRAP_CONTENT);
                layout.setMargins(16, 8, 16, 8);
                if (lastView != null)
                    layout.addRule(RelativeLayout.BELOW, lastView.getId());
                else
                    layout.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                dialogLayout.addView(view, layout);
                lastView = view;
            }

            if (m_buttonsList != null)
            {
                LinearLayout buttonsLayout = new LinearLayout(m_activity);
                buttonsLayout.setOrientation(LinearLayout.HORIZONTAL);
                buttonsLayout.setId(id++);
                boolean firstButton = true;
                for (ButtonStruct button: m_buttonsList)
                {
                    Button bv;
                    try {
                        bv = new Button(m_activity, null, android.R.attr.borderlessButtonStyle);
                    } catch (Exception e) {
                        bv = new Button(m_activity);
                        e.printStackTrace();
                    }

                    bv.setText(button.m_text);
                    bv.setOnClickListener(button);
                    if (!firstButton) // first button
                    {
                        View spacer = new View(m_activity);
                        try {
                            LinearLayout.LayoutParams layout = new LinearLayout.LayoutParams(1,
                                    RelativeLayout.LayoutParams.MATCH_PARENT);
                            spacer.setBackground(getStyledDrawable(android.R.attr.dividerVertical));
                            buttonsLayout.addView(spacer, layout);
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                    LinearLayout.LayoutParams layout = new LinearLayout.LayoutParams(
                            RelativeLayout.LayoutParams.MATCH_PARENT,
                            RelativeLayout.LayoutParams.WRAP_CONTENT, 1.0f);
                    buttonsLayout.addView(bv, layout);
                    firstButton = false;
                }

                try {
                    View horizontalDivider = new View(m_activity);
                    horizontalDivider.setId(id);
                    horizontalDivider.setBackground(getStyledDrawable(
                            android.R.attr.dividerHorizontal));
                    RelativeLayout.LayoutParams relativeParams = new RelativeLayout.LayoutParams(
                            RelativeLayout.LayoutParams.MATCH_PARENT, 1);
                    relativeParams.setMargins(0, 10, 0, 0);
                    if (lastView != null) {
                        relativeParams.addRule(RelativeLayout.BELOW, lastView.getId());
                    }
                    else
                        relativeParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                    dialogLayout.addView(horizontalDivider, relativeParams);
                    lastView = horizontalDivider;
                } catch (Exception e) {
                    e.printStackTrace();
                }
                RelativeLayout.LayoutParams relativeParams = new RelativeLayout.LayoutParams(
                        RelativeLayout.LayoutParams.MATCH_PARENT,
                        RelativeLayout.LayoutParams.WRAP_CONTENT);
                if (lastView != null) {
                    relativeParams.addRule(RelativeLayout.BELOW, lastView.getId());
                }
                else
                    relativeParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                relativeParams.setMargins(2, 0, 2, 0);
                dialogLayout.addView(buttonsLayout, relativeParams);
            }
            scrollView.addView(dialogLayout);
            m_dialog.setView(scrollView);
            m_dialog.show();
        });
    }

    @UsedFromNativeCode
    void hide()
    {
        m_activity.runOnUiThread(() -> {
            if (m_dialog != null && m_dialog.isShowing())
                m_dialog.dismiss();
            reset();
        });
    }

    long handler()
    {
        return m_handler;
    }

    void reset()
    {
        m_standardIcon = 0;
        m_title = null;
        m_text = null;
        m_informativeText = null;
        m_detailedText = null;
        m_buttonsList = null;
        m_dialog = null;
        m_handler = 0;
    }

    private static final String QtTAG = "QtMessageDialogHelper";
    private final Activity m_activity;
    private int m_standardIcon = 0;
    private Spanned m_title, m_text, m_informativeText, m_detailedText;
    private ArrayList<ButtonStruct> m_buttonsList;
    private AlertDialog m_dialog;
    private long m_handler = 0;
    private Resources.Theme m_theme;
}
