// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.networkinformation;

import android.annotation.SuppressLint;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.NetworkRequest;
import android.net.NetworkCapabilities;
import android.net.Network;
import android.os.Build;

public class QtAndroidNetworkInformation {
    private static final String LOG_TAG = "QtAndroidNetworkInformation";

    private static native void networkConnectivityChanged(int connectivity);
    private static native void genericInfoChanged(boolean captivePortal, boolean metered);
    private static native void transportMediumChanged(int transportMedium);

    private static QtNetworkInformationCallback m_callback = null;
    private static final Object m_lock = new Object();

    // Keep synchronized with AndroidConnectivity in androidconnectivitymanager.h
    enum AndroidConnectivity {
        Connected, Unknown, Disconnected
    }

    // Keep synchronized with AndroidTransport in androidconnectivitymanager.h
    enum Transport {
        Unknown,
        Bluetooth,
        Cellular,
        Ethernet,
        LoWPAN,
        Usb,
        WiFi,
        WiFiAware,
    }

    private static class QtNetworkInformationCallback extends NetworkCallback {
        public AndroidConnectivity previousState = null;
        public Transport previousTransport = null;

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

            final Transport transport = getTransport(capabilities);
            if (transport == Transport.Unknown) // If we don't have any transport media: override
                s = AndroidConnectivity.Unknown;

            setState(s);
            setTransportMedium(transport);

            final boolean captive
                    = capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_CAPTIVE_PORTAL);
            final boolean metered
                    = !capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED);
            genericInfoChanged(captive, metered);
        }

        private Transport getTransport(NetworkCapabilities capabilities)
        {
            if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) {
                return Transport.WiFi;
            } else if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR)) {
                return Transport.Cellular;
            } else if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_BLUETOOTH)) {
                return Transport.Bluetooth;
            } else if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) {
                return Transport.Ethernet;
            } else if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI_AWARE)) {
                // Build.VERSION_CODES.O
                return Transport.WiFiAware;
            } else if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_LOWPAN)) {
                // Build.VERSION_CODES.O_MR1
                return Transport.LoWPAN;
            }/* else if (capabilities.hasTransport(NetworkCapabilities.TRANSPORT_USB)) {
                // Build.VERSION_CODES.S
                return Transport.Usb;
            }*/ // @todo: Uncomment once we can use SDK 31
            return Transport.Unknown;
        }

        private void setState(AndroidConnectivity s) {
            if (previousState != s) {
                previousState = s;
                networkConnectivityChanged(s.ordinal());
            }
        }

        private void setTransportMedium(Transport t) {
            if (previousTransport != t) {
                previousTransport = t;
                transportMediumChanged(t.ordinal());
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
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
                    builder = builder.clearCapabilities();
                builder = builder.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    builder = builder.addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_SUSPENDED);
                    builder = builder.addCapability(NetworkCapabilities.NET_CAPABILITY_FOREGROUND);
                }
                NetworkRequest request = builder.build();

                // Can't use registerDefaultNetworkCallback because it doesn't let us know when
                // the network disconnects!
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
