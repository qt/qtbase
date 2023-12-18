// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android;

import android.app.Service;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.util.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

class QtServiceLoader extends QtLoader {
    private final Service m_service;

    public QtServiceLoader(Service service) {
        super(service);
        m_service = service;

        extractContextMetaData();
    }

    @Override
    protected void initContextInfo() {
        try {
            m_contextInfo = m_context.getPackageManager().getServiceInfo(
                    new ComponentName(m_context, m_context.getClass()),
                    PackageManager.GET_META_DATA);
        } catch (Exception e) {
            e.printStackTrace();
            finish();
        }
    }

    @Override
    protected void finish() {
        if (m_service != null)
            m_service.stopSelf();
        else
            Log.w(QtTAG, "finish() called when service object is null");
    }

    @Override
    protected void initStaticClassesImpl(Class<?> initClass, Object staticInitDataObject) {
        try {
            Method m = initClass.getMethod("setService", Service.class, Object.class);
            m.invoke(staticInitDataObject, m_service, this);
        } catch (IllegalAccessException | InvocationTargetException | NoSuchMethodException e) {
            Log.d(QtTAG, "Class " + initClass.getName() + " does not implement setService method");
        }
    }
}
