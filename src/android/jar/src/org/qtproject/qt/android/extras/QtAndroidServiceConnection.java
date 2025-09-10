// Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.extras;

import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.IBinder;

import org.qtproject.qt.android.UsedFromNativeCode;

class QtAndroidServiceConnection implements ServiceConnection
{
    @UsedFromNativeCode
    QtAndroidServiceConnection(long id)
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
    public void onServiceConnected(ComponentName name, IBinder service)
    {
        synchronized(this) {
            QtNative.onServiceConnected(m_id, name.flattenToString(), service);
        }
    }

    @Override
    public void onServiceDisconnected(ComponentName name)
    {
        synchronized(this) {
            QtNative.onServiceDisconnected(m_id, name.flattenToString());
        }
    }

    private long m_id;
}
