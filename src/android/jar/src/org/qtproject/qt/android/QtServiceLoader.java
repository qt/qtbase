// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android;

import android.app.Service;
import android.content.ContextWrapper;
import android.util.Log;

class QtServiceLoader extends QtLoader {
    private final Service m_service;

    public QtServiceLoader(Service service) {
        super(new ContextWrapper(service));
        m_service = service;

        extractContextMetaData();
    }

    @Override
    protected void finish() {
        if (m_service != null)
            m_service.stopSelf();
        else
            Log.w(QtTAG, "finish() called when service object is null");
    }
}
