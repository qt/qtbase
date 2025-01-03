// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android.bindings;

import android.os.Bundle;
import android.content.ComponentName;
import android.content.pm.PackageManager;

public class QtServiceLoader extends QtLoader {
    QtService m_service;
    QtServiceLoader(QtService service) {
        super(service, QtService.class);
        m_service = service;
    }

    public void onCreate() {
        try {
            m_contextInfo = m_service.getPackageManager().getServiceInfo(new ComponentName(m_service, m_service.getClass()), PackageManager.GET_META_DATA);
        } catch (Exception e) {
            e.printStackTrace();
            m_service.stopSelf();
            return;
        }

        if (QtApplication.m_delegateObject != null && QtApplication.onCreate != null) {
            Bundle bundle = null;
            QtApplication.invokeDelegateMethod(QtApplication.onCreate, bundle);
        }
        startApp(true);
    }

    @Override
    protected void finish() {
        m_service.stopSelf();
    }

    @Override
    protected String loaderClassName() {
        return "org.qtproject.qt.android.QtServiceDelegate";
    }

    @Override
    protected Class<?> contextClassName() {
        return android.app.Service.class;
    }
}
