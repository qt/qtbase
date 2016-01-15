/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
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
import android.text.InputType;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

public class QtEditText extends View
{
    int m_initialCapsMode = 0;
    int m_imeOptions = 0;
    int m_inputType = InputType.TYPE_CLASS_TEXT;
    boolean m_optionsChanged = false;
    QtActivityDelegate m_activityDelegate;

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

    public QtEditText(Context context, QtActivityDelegate activityDelegate)
    {
        super(context);
        setFocusable(true);
        setFocusableInTouchMode(true);
        m_activityDelegate = activityDelegate;
    }
    public QtActivityDelegate getActivityDelegate()
    {
        return m_activityDelegate;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs)
    {
        outAttrs.inputType = m_inputType;
        outAttrs.imeOptions = m_imeOptions;
        outAttrs.initialCapsMode = m_initialCapsMode;
        outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_EXTRACT_UI;
        return new QtInputConnection(this);
    }

// // DEBUG CODE
//    @Override
//    protected void onDraw(Canvas canvas) {
//        canvas.drawARGB(127, 255, 0, 255);
//        super.onDraw(canvas);
//    }
}
