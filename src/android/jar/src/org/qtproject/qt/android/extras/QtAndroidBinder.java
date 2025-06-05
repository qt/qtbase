// Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.extras;

import android.os.Binder;
import android.os.Parcel;

import org.qtproject.qt.android.UsedFromNativeCode;

class QtAndroidBinder extends Binder
{
    @UsedFromNativeCode
    QtAndroidBinder(long id)
    {
        m_id = id;
    }

    void setId(long id)
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
