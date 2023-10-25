// Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Service;

public class QtServiceDelegate
{
    private Service m_service = null;

    QtServiceDelegate(Service service)
    {
        m_service = service;

        // Set native context
        QtNative.setService(m_service, this);
    }
}
