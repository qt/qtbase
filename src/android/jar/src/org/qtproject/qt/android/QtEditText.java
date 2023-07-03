// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.graphics.Canvas;
import android.text.InputType;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.KeyEvent;

import org.qtproject.qt.android.QtInputConnection.QtInputConnectionListener;

class QtEditText extends View
{
    int m_initialCapsMode = 0;
    int m_imeOptions = 0;
    int m_inputType = InputType.TYPE_CLASS_TEXT;
    boolean m_optionsChanged = false;
    QtInputConnection m_inputConnection = null;
    private QtInputConnectionListener m_qtInputConnectionListener;

    public void setQtInputConnectionListener(QtInputConnectionListener listener)
    {
        m_qtInputConnectionListener = listener;
    }

    public void setImeOptions(int m_imeOptions)
    {
        if (m_imeOptions == this.m_imeOptions)
            return;
        this.m_imeOptions = m_imeOptions;
        m_optionsChanged = true;
    }

    public void setInitialCapsMode(int m_initialCapsMode)
    {
        if (m_initialCapsMode == this.m_initialCapsMode)
            return;
        this.m_initialCapsMode = m_initialCapsMode;
        m_optionsChanged = true;
    }


    public void setInputType(int m_inputType)
    {
        if (m_inputType == this.m_inputType)
            return;
        this.m_inputType = m_inputType;
        m_optionsChanged = true;
    }

    public QtEditText(Context context)
    {
        super(context);
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs)
    {
        outAttrs.inputType = m_inputType;
        outAttrs.imeOptions = m_imeOptions;
        outAttrs.initialCapsMode = m_initialCapsMode;
        m_inputConnection = new QtInputConnection(this,m_qtInputConnectionListener);
        return m_inputConnection;
    }

    @Override
    public boolean onCheckIsTextEditor ()
    {
        return true;
    }

    @Override
    public boolean onKeyDown (int keyCode, KeyEvent event)
    {
        if (null != m_inputConnection)
            m_inputConnection.restartImmInput();

        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        // DEBUG CODE
        // canvas.drawARGB(127, 255, 0, 255);
        super.onDraw(canvas);
    }
}
