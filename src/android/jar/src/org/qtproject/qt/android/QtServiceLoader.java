// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android;

import android.app.Service;
import android.os.Bundle;
import android.content.ComponentName;
import android.content.pm.PackageManager;

import java.security.Provider;

public class QtServiceLoader extends QtLoader {
    Service m_service;

    public QtServiceLoader(Service service) {
        super(service);
        m_service = service;
    }

    public void onCreate() {
        try {
            m_contextInfo = m_service.getPackageManager().getServiceInfo(new ComponentName(
                    m_service, m_service.getClass()), PackageManager.GET_META_DATA);
        } catch (Exception e) {
            e.printStackTrace();
            m_service.stopSelf();
            return;
        }

        startApp(true);
    }

    @Override
    protected void finish() {
        m_service.stopSelf();
    }
}
