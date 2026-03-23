// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.example.androidlanguageexchange;

public class JavaExchanger
{
    private long mCppExchanger;

    private native void connectToCppExchanger(long cppExchanger);
    private native void fromOther(long cppExchanger, String str);

    public JavaExchanger(long cppExchanger) {
        mCppExchanger = cppExchanger;
        connectToCppExchanger(cppExchanger);
    }
    public void fromCpp(int type, String str) {
        if (type == OtherLanguageType.Java.getCode()) {
            String msg = str + " And hello back to you from\nJava!";
            fromOther(mCppExchanger, msg);
        }
    }
}
