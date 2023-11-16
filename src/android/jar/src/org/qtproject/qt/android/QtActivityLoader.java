// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Bundle;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.util.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;

public class QtActivityLoader extends QtLoader {
    private final Activity m_activity;

    public QtActivityLoader(Activity activity)
    {
        super(activity);
        m_activity = activity;

        extractContextMetaData();
    }

    @Override
    protected void initContextInfo() {
        try {
            m_contextInfo = m_context.getPackageManager().getActivityInfo(
                    ((Activity)m_context).getComponentName(), PackageManager.GET_META_DATA);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
            finish();
        }
    }

    private void showErrorDialog() {
        Resources resources = m_activity.getResources();
        String packageName = m_activity.getPackageName();
        AlertDialog errorDialog = new AlertDialog.Builder(m_activity).create();
        @SuppressLint("DiscouragedApi") int id = resources.getIdentifier(
                "fatal_error_msg", "string", packageName);
        errorDialog.setMessage(resources.getString(id));
        errorDialog.setButton(Dialog.BUTTON_POSITIVE, resources.getString(android.R.string.ok),
                (dialog, which) -> finish());
        errorDialog.show();
    }

    @Override
    protected void finish() {
        showErrorDialog();
        m_activity.finish();
    }

    @Override
    protected void initStaticClassesImpl(Class<?> initClass, Object staticInitDataObject) {
        try {
            Method m = initClass.getMethod("setActivity", Activity.class, Object.class);
            m.invoke(staticInitDataObject, m_activity, this);
        } catch (IllegalAccessException | InvocationTargetException | NoSuchMethodException e) {
            Log.d(QtTAG, "Class " + initClass.getName() + " does not implement setActivity method");
        }
    }

    private String getDecodedUtfString(String str)
    {
        byte[] decodedExtraEnvVars = Base64.decode(str, Base64.DEFAULT);
        return new String(decodedExtraEnvVars, StandardCharsets.UTF_8);
    }

    int getAppIconSize()
    {
        int size = m_activity.getResources().getDimensionPixelSize(android.R.dimen.app_icon_size);
        if (size < 36 || size > 512) { // check size sanity
            DisplayMetrics metrics = new DisplayMetrics();
            m_activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            size = metrics.densityDpi / 10 * 3;
            if (size < 36)
                size = 36;

            if (size > 512)
                size = 512;
        }

        return size;
    }

    private void setupStyleExtraction()
    {
        int displayDensity = m_activity.getResources().getDisplayMetrics().densityDpi;
        setEnvironmentVariable("QT_ANDROID_THEME_DISPLAY_DPI", String.valueOf(displayDensity));

        String extractOption = getMetaData("android.app.extract_android_style");
        if (extractOption.equals("full"))
            setEnvironmentVariable("QT_USE_ANDROID_NATIVE_STYLE", String.valueOf(1));

        String stylePath = ExtractStyle.setup(m_activity, extractOption, displayDensity);
        setEnvironmentVariable("ANDROID_STYLE_PATH", stylePath);
    }

    @Override
    protected void extractContextMetaData()
    {
        super.extractContextMetaData();

        setEnvironmentVariable("QT_USE_ANDROID_NATIVE_DIALOGS", String.valueOf(1));
        setEnvironmentVariable("QT_ANDROID_APP_ICON_SIZE", String.valueOf(getAppIconSize()));

        setupStyleExtraction();

        Intent intent = m_activity.getIntent();
        if (intent == null) {
            Log.w(QtTAG, "Null Intent from the current Activity.");
            return;
        }

        String intentArgs = intent.getStringExtra("applicationArguments");
        if (intentArgs != null)
            setApplicationParameters(intentArgs);

        Bundle extras = intent.getExtras();
        if (extras == null) {
            Log.w(QtTAG, "Null extras from the Activity's intent.");
            return;
        }

        int flags = m_activity.getApplicationInfo().flags;
        boolean isDebuggable = (flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;

        if (isDebuggable) {
            if (extras.containsKey("extraenvvars")) {
                String extraEnvVars = extras.getString("extraenvvars");
                setEnvironmentVariables(getDecodedUtfString(extraEnvVars));
            }

            if (extras.containsKey("extraappparams")) {
                String extraAppParams = extras.getString("extraappparams");
                setApplicationParameters(getDecodedUtfString(extraAppParams));
            }
        } else {
            Log.d(QtNative.QtTAG, "Not in debug mode! It is not allowed to use extra arguments " +
                    "in non-debug mode.");
        }
    }
}
