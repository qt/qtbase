// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package org.qtproject.qt.android.testdatapackage;

public class QtJniEnvironmentTestClass
{
    private static native void callbackFromJava(String message);
    private static native void namedCallbackFromJava(String message);
    private static native void memberCallbackFromJava(String message);
    private static native void namedMemberCallbackFromJava(String message);
    private static native void namespaceCallbackFromJava(String message);
    private static native void intCallbackFromJava(int value);

    public final int INT_FIELD = 123;
    public static final int S_INT_FIELD = 321;

    QtJniEnvironmentTestClass() {}

    public static void appendJavaToString(String message)
    {
        callbackFromJava("From Java: " + message);
    }

    public static void namedAppendJavaToString(String message)
    {
        namedCallbackFromJava("From Java (named): " + message);
    }

    public static void memberAppendJavaToString(String message)
    {
        memberCallbackFromJava("From Java (member): " + message);
    }

    public static void namedMemberAppendJavaToString(String message)
    {
        namedMemberCallbackFromJava("From Java (named member): " + message);
    }

    public static void namespaceAppendJavaToString(String message)
    {
        namespaceCallbackFromJava("From Java (namespace): " + message);
    }

    public static void convertToInt(String message)
    {
        intCallbackFromJava(Integer.parseInt(message));
    }
}

class QtJniEnvironmentTestClassNoCtor
{
    private static native void callbackFromJavaNoCtor(String message);

    public static void appendJavaToString(String message)
    {
        callbackFromJavaNoCtor("From Java (no ctor): " + message);
    }
}

