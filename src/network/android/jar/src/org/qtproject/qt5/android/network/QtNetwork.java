/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

package org.qtproject.qt5.android.network;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import android.net.ConnectivityManager;
import android.net.Proxy;
import android.net.ProxyInfo;

public class QtNetwork
{
    private static final String LOG_TAG = "QtNetwork";
    private static ProxyReceiver m_proxyReceiver = null;
    private static final Object m_lock = new Object();
    private static ProxyInfo m_proxyInfo = null;

    private static class ProxyReceiver extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            m_proxyInfo = null;
        }
    }

    private QtNetwork() {}

    public static void registerReceiver(final Context context)
    {
        synchronized (m_lock) {
            if (m_proxyReceiver == null) {
                m_proxyReceiver = new ProxyReceiver();
                IntentFilter intentFilter = new IntentFilter(Proxy.PROXY_CHANGE_ACTION);
                context.registerReceiver(m_proxyReceiver, intentFilter);
            }
        }
    }

    public static void unregisterReceiver(final Context context)
    {
        synchronized (m_lock) {
            if (m_proxyReceiver == null)
                return;

            context.unregisterReceiver(m_proxyReceiver);
        }
    }

    public static ConnectivityManager getConnectivityManager(final Context context)
    {
        return (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
    }

    public static ProxyInfo getProxyInfo(final Context context)
    {
        if (m_proxyInfo == null)
            m_proxyInfo = (ProxyInfo)getConnectivityManager(context).getDefaultProxy();
        return m_proxyInfo;
    }
}
