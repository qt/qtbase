// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android.bindings;

import android.app.Application;
import org.qtproject.qt.android.QtLoader;

public class QtApplication extends Application
{
    public final static String QtTAG = "Qt";

    @Override
    public void onTerminate() {
        if (QtLoader.m_delegateObject != null && QtLoader.m_delegateMethods.containsKey("onTerminate"))
            QtLoader.invokeDelegateMethod(QtLoader.m_delegateMethods.get("onTerminate").get(0));
        super.onTerminate();
    }

    // TODO: only keep around for avoid build errors, will be removed.
    public static class InvokeResult
    {
        public boolean invoked = false;
        public Object methodReturns = null;
    }

    public static InvokeResult invokeDelegate(Object... args)
    {
        InvokeResult result = new InvokeResult();
        return result;
    }
}
