// Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
