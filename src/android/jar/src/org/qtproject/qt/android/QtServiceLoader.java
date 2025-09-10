// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Service;
import android.content.ContextWrapper;

import java.lang.IllegalArgumentException;

class QtServiceLoader extends QtLoader {
    QtServiceLoader(Service service) throws IllegalArgumentException {
        super(new ContextWrapper(service));
        extractContextMetaData(service);
    }

    static QtServiceLoader getServiceLoader(Service service) throws IllegalArgumentException {
        if (m_instance == null)
            m_instance = new QtServiceLoader(service);
        return (QtServiceLoader) m_instance;
    }
}
