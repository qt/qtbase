// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager.NameNotFoundException;

public class QtNativeLibrariesDir {
    public static final String systemLibrariesDir = "/system/lib/";
    public static String nativeLibrariesDir(Context context)
    {
        String m_nativeLibraryDir = null;
        try {
            ApplicationInfo ai = context.getPackageManager().getApplicationInfo(context.getPackageName(), 0);
            m_nativeLibraryDir = ai.nativeLibraryDir + "/";
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }
        return m_nativeLibraryDir;
    }
}
