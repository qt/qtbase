// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.SurfaceView;

import java.io.File;
import java.io.FileOutputStream;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Objects;

import dalvik.system.DexClassLoader;
import android.content.res.Resources;

class QtEmbeddedLoader extends QtLoader {
    private static final String TAG = "QtEmbeddedLoader";

    public QtEmbeddedLoader(Context context) {
        super(new ContextWrapper(context));
        // TODO Service context handling QTBUG-118874
        int displayDensity = m_context.getResources().getDisplayMetrics().densityDpi;
        setEnvironmentVariable("QT_ANDROID_THEME_DISPLAY_DPI", String.valueOf(displayDensity));
        String stylePath = ExtractStyle.setup(m_context, "minimal", displayDensity);
        setEnvironmentVariable("ANDROID_STYLE_PATH", stylePath);
    }

    @Override
    protected void finish() {
        // Called when loading fails - clear the delegate to make sure we don't hold reference
        // to the embedding Context
        QtEmbeddedDelegateFactory.remove((Activity)m_context.getBaseContext());
    }
}
