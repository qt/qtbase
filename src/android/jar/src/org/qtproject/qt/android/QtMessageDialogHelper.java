// Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


package org.qtproject.qt.android;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.text.ClipboardManager;
import android.text.Html;
import android.text.Spanned;
import android.util.TypedValue;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

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
        m_text = Html.fromHtml(text);
    }
    QtMessageDialogHelper m_dialog;
    private int m_id;
    Spanned m_text;

    @Override
    public void onClick(View view) {
        QtNativeDialogHelper.dialogResult(m_dialog.handler(), m_id);
    }
}

public class QtMessageDialogHelper
{

    public QtMessageDialogHelper(Activity activity)
    {
        m_activity = activity;
    }


    public void setStandardIcon(int icon)
    {
        m_standardIcon = icon;

    }

    private Drawable getIconDrawable()
    {
        if (m_standardIcon == 0)
            return null;

        try {
            TypedValue typedValue = new TypedValue();
            m_theme.resolveAttribute(android.R.attr.alertDialogIcon, typedValue, true);
            return m_activity.getResources().getDrawable(typedValue.resourceId,
                                                         m_activity.getTheme());
        } catch (Exception e) {
            e.printStackTrace();
        }

        // Information, Warning, Critical, Question
        switch (m_standardIcon)
        {
            case 1: // Information
                try {
                    return m_activity.getResources().getDrawable(android.R.drawable.ic_dialog_info,
                                                                 m_activity.getTheme());
                } catch (Exception e) {
                    e.printStackTrace();
                }
                break;
            case 2: // Warning
//                try {
//                    return Class.forName("android.R$drawable").getDeclaredField("stat_sys_warning").getInt(null);
//                } catch (Exception e) {
//                    e.printStackTrace();
//                }
//                break;
            case 3: // Critical
                try {
                    return m_activity.getResources().getDrawable(android.R.drawable.ic_dialog_alert,
                                                                 m_activity.getTheme());
                } catch (Exception e) {
                    e.printStackTrace();
                }
                break;
            case 4: // Question
                try {
                    return m_activity.getResources().getDrawable(android.R.drawable.ic_menu_help,
                                                                 m_activity.getTheme());
                } catch (Exception e) {
                    e.printStackTrace();
                }
                break;
        }
        return null;
    }

    public void setTile(String title)
    {
        m_title = Html.fromHtml(title);
    }

    public void setText(String text)
    {
        m_text = Html.fromHtml(text);
    }

    public void setInformativeText(String informativeText)
    {
        m_informativeText = Html.fromHtml(informativeText);
    }

    public void setDetailedText(String text)
    {
        m_detailedText = Html.fromHtml(text);
    }

    public void addButton(int id, String text)
    {
        if (m_buttonsList == null)
            m_buttonsList = new ArrayList<ButtonStruct>();
        m_buttonsList.add(new ButtonStruct(this, id, text));
    }

    private Drawable getStyledDrawable(String drawable) throws ClassNotFoundException, NoSuchFieldException, IllegalAccessException
    {
        int[] attrs = {Class.forName("android.R$attr").getDeclaredField(drawable).getInt(null)};
        final TypedArray a = m_theme.obtainStyledAttributes(attrs);
        Drawable d = a.getDrawable(0);
        a.recycle();
        return  d;
    }


    public void show(long handler)
    {
        m_handler = handler;
        m_activity.runOnUiThread( new Runnable() {
            @Override
            public void run() {
                if (m_dialog != null && m_dialog.isShowing())
                    m_dialog.dismiss();

                m_dialog = new AlertDialog.Builder(m_activity).create();
                m_theme = m_dialog.getWindow().getContext().getTheme();

                if (m_title != null)
                    m_dialog.setTitle(m_title);
                m_dialog.setOnCancelListener( new DialogInterface.OnCancelListener() {
                    @Override
                    public void onCancel(DialogInterface dialogInterface) {
                        QtNativeDialogHelper.dialogResult(handler(), -1);
                    }
                });
                m_dialog.setCancelable(m_buttonsList == null);
                m_dialog.setCanceledOnTouchOutside(m_buttonsList == null);
                m_dialog.setIcon(getIconDrawable());
                ScrollView scrollView = new ScrollView(m_activity);
                RelativeLayout dialogLayout = new RelativeLayout(m_activity);
                int id = 1;
                View lastView = null;
                View.OnLongClickListener copyText = new View.OnLongClickListener() {
                    @Override
                    public boolean onLongClick(View view) {
                        TextView tv = (TextView)view;
                        if (tv != null) {
                            ClipboardManager cm = (android.text.ClipboardManager) m_activity.getSystemService(Context.CLIPBOARD_SERVICE);
                            cm.setText(tv.getText());
                        }
                        return true;
                    }
                };
                if (m_text != null)
                {
                    TextView view = new TextView(m_activity);
                    view.setId(id++);
                    view.setOnLongClickListener(copyText);
                    view.setLongClickable(true);

                    view.setText(m_text);
                    view.setTextAppearance(m_activity, android.R.style.TextAppearance_Medium);

                    RelativeLayout.LayoutParams layout = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
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
                    view.setTextAppearance(m_activity, android.R.style.TextAppearance_Medium);

                    RelativeLayout.LayoutParams layout = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
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
                    view.setTextAppearance(m_activity, android.R.style.TextAppearance_Small);

                    RelativeLayout.LayoutParams layout = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
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
                            bv = new Button(m_activity, null, Class.forName("android.R$attr").getDeclaredField("borderlessButtonStyle").getInt(null));
                        } catch (Exception e) {
                            bv = new Button(m_activity);
                            e.printStackTrace();
                        }

                        bv.setText(button.m_text);
                        bv.setOnClickListener(button);
                        if (!firstButton) // first button
                        {
                            LinearLayout.LayoutParams layout = null;
                            View spacer = new View(m_activity);
                            try {
                                layout = new LinearLayout.LayoutParams(1, RelativeLayout.LayoutParams.MATCH_PARENT);
                                spacer.setBackgroundDrawable(getStyledDrawable("dividerVertical"));
                                buttonsLayout.addView(spacer, layout);
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                        }
                        LinearLayout.LayoutParams layout = null;
                        layout = new LinearLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT, 1.0f);
                        buttonsLayout.addView(bv, layout);
                        firstButton = false;
                    }

                    try {
                        View horizontalDevider = new View(m_activity);
                        horizontalDevider.setId(id++);
                        horizontalDevider.setBackgroundDrawable(getStyledDrawable("dividerHorizontal"));
                        RelativeLayout.LayoutParams relativeParams = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, 1);
                        relativeParams.setMargins(0, 10, 0, 0);
                        if (lastView != null) {
                            relativeParams.addRule(RelativeLayout.BELOW, lastView.getId());
                        }
                        else
                            relativeParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
                        dialogLayout.addView(horizontalDevider, relativeParams);
                        lastView = horizontalDevider;
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    RelativeLayout.LayoutParams relativeParams = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
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
            }
        });
    }

    public void hide()
    {
        m_activity.runOnUiThread( new Runnable() {
            @Override
            public void run() {
                if (m_dialog != null && m_dialog.isShowing())
                    m_dialog.dismiss();
                reset();
            }
        });
    }

    public long handler()
    {
        return m_handler;
    }

    public void reset()
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

    private Activity m_activity;
    private int m_standardIcon = 0;
    private Spanned m_title, m_text, m_informativeText, m_detailedText;
    private ArrayList<ButtonStruct> m_buttonsList;
    private AlertDialog m_dialog;
    private long m_handler = 0;
    private Resources.Theme m_theme;
}
