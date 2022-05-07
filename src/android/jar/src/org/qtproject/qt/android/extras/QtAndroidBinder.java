/****************************************************************************
**
** Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Android port of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

package org.qtproject.qt.android.extras;

import android.os.Binder;
import android.os.Parcel;

public class QtAndroidBinder extends Binder
{
    public QtAndroidBinder(long id)
    {
        m_id = id;
    }

    public void setId(long id)
    {
        synchronized(this)
        {
            m_id = id;
        }
    }
    @Override
    protected boolean onTransact(int code, Parcel data, Parcel reply, int flags)
    {
        synchronized(this)
        {
            return QtNative.onTransact(m_id, code, data, reply, flags);
        }
    }

    private long m_id;
}
