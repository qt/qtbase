// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.content.ContextWrapper;

class QtEmbeddedLoader extends QtLoader {
    private static final String TAG = "QtEmbeddedLoader";

    private QtEmbeddedLoader(Context context) throws IllegalArgumentException {
        super(new ContextWrapper(context));
        // TODO Service context handling QTBUG-118874
        int displayDensity = context.getResources().getDisplayMetrics().densityDpi;
        setEnvironmentVariable("QT_ANDROID_THEME_DISPLAY_DPI", String.valueOf(displayDensity));
        String stylePath = ExtractStyle.setup(context, "minimal", displayDensity);
        setEnvironmentVariable("ANDROID_STYLE_PATH", stylePath);
        setEnvironmentVariable("QT_ANDROID_NO_EXIT_CALL", String.valueOf(true));

        extractContextMetaData(context);
    }

    static QtEmbeddedLoader getEmbeddedLoader(Context context) throws IllegalArgumentException {
        if (m_instance == null)
            m_instance = new QtEmbeddedLoader(context);
        return (QtEmbeddedLoader) m_instance;
    }
}
