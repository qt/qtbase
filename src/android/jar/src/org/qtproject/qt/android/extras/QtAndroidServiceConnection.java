// Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

package org.qtproject.qt.android.extras;

import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.IBinder;

public class QtAndroidServiceConnection implements ServiceConnection
{
    public QtAndroidServiceConnection(long id)
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
