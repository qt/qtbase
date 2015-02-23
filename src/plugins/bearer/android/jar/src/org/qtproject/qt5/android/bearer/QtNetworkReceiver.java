/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

package org.qtproject.qt5.android.bearer;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import android.app.Activity;
import android.net.ConnectivityManager;

public class QtNetworkReceiver
{
    private static final String LOG_TAG = "QtNetworkReceiver";
    private static native void activeNetworkInfoChanged();
    private static BroadcastReceiverPrivate m_broadcastReceiver = null;
    private static final Object m_lock = new Object();

    private static class BroadcastReceiverPrivate extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            activeNetworkInfoChanged();
        }
    }

    private QtNetworkReceiver() {}

    public static void registerReceiver(final Activity activity)
    {
        synchronized (m_lock) {
            if (m_broadcastReceiver == null) {
                m_broadcastReceiver = new BroadcastReceiverPrivate();
                IntentFilter intentFilter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
                activity.registerReceiver(m_broadcastReceiver, intentFilter);
            }
        }
    }

    public static void unregisterReceiver(final Activity activity)
    {
        synchronized (m_lock) {
            if (m_broadcastReceiver == null)
                return;

            activity.unregisterReceiver(m_broadcastReceiver);
        }
    }

    public static ConnectivityManager getConnectivityManager(final Activity activity)
    {
        return (ConnectivityManager)activity.getApplicationContext().getSystemService(Context.CONNECTIVITY_SERVICE);
    }
}
