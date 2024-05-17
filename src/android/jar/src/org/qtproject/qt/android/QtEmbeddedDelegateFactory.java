// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

import java.util.HashMap;

class QtEmbeddedDelegateFactory {
    private static final HashMap<Activity, QtEmbeddedDelegate> m_delegates = new HashMap<>();
    private static final Object m_delegateLock = new Object();

    @UsedFromNativeCode
    public static QtActivityDelegateBase getActivityDelegate(Activity activity) {
        synchronized (m_delegateLock) {
            return m_delegates.get(activity);
        }
    }

    public static QtEmbeddedDelegate create(Activity activity) {
        synchronized (m_delegateLock) {
            if (!m_delegates.containsKey(activity))
                m_delegates.put(activity, new QtEmbeddedDelegate(activity));

            return m_delegates.get(activity);
        }
    }

    public static void remove(Activity activity) {
        synchronized (m_delegateLock) {
            m_delegates.remove(activity);
        }
    }
}
