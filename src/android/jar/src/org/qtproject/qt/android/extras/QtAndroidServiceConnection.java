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
