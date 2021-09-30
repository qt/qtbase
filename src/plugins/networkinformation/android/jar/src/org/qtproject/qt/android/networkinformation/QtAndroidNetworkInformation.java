/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

package org.qtproject.qt.android.networkinformation;

import android.annotation.SuppressLint;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.NetworkRequest;
import android.net.NetworkCapabilities;
import android.net.Network;

public class QtAndroidNetworkInformation {
    private static final String LOG_TAG = "QtAndroidNetworkInformation";

    private static native void connectivityChanged();
    private static native void behindCaptivePortalChanged(boolean state);

    private static QtNetworkInformationCallback m_callback = null;
    private static final Object m_lock = new Object();

    enum AndroidConnectivity {
        Connected, Unknown, Disconnected
    }

    private static class QtNetworkInformationCallback extends NetworkCallback {
        public AndroidConnectivity previousState = null;

        QtNetworkInformationCallback() {
        }

        @Override
        public void onCapabilitiesChanged(Network network, NetworkCapabilities capabilities) {
            AndroidConnectivity s;
            if (!capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET))
                s = AndroidConnectivity.Disconnected;
            else if (capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED))
                s = AndroidConnectivity.Connected;
            else
                s = AndroidConnectivity.Unknown; // = we _may_ have Internet access
            setState(s);

            final boolean captive
                    = capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_CAPTIVE_PORTAL);
            behindCaptivePortalChanged(captive);
        }

        private void setState(AndroidConnectivity s) {
            if (previousState != s) {
                previousState = s;
                connectivityChanged();
            }
        }

        @Override
        public void onLost(Network network) {
            setState(AndroidConnectivity.Disconnected);
        }
    }

    private QtAndroidNetworkInformation() {
    }

    public static AndroidConnectivity state() {
        if (m_callback != null && m_callback.previousState != null)
            return m_callback.previousState;
        return AndroidConnectivity.Unknown;
    }

    @SuppressLint("MissingPermission")
    public static void registerReceiver(final Context context) {
        synchronized (m_lock) {
            if (m_callback == null) {
                ConnectivityManager manager = getConnectivityManager(context);
                m_callback = new QtNetworkInformationCallback();
                NetworkRequest.Builder builder = new NetworkRequest.Builder();
                builder = builder.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P)
                    builder = builder.addCapability(NetworkCapabilities.NET_CAPABILITY_FOREGROUND);
                NetworkRequest request = builder.build();
                manager.registerNetworkCallback(request, m_callback);
            }
        }
    }

    public static void unregisterReceiver(final Context context) {
        synchronized (m_lock) {
            if (m_callback != null) {
                getConnectivityManager(context).unregisterNetworkCallback(m_callback);
                m_callback = null;
            }
        }
    }

    public static ConnectivityManager getConnectivityManager(final Context context) {
        return (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    }
}
