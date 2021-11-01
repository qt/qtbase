/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

package org.qtproject.qt.android.testdatapackage;

public class QtJniEnvironmentTestClass
{
    private static native void callbackFromJava(String message);
    private static native void intCallbackFromJava(int value);

    public final int INT_FIELD = 123;
    public static final int S_INT_FIELD = 321;

    QtJniEnvironmentTestClass() {}

    public static void appendJavaToString(String message)
    {
        callbackFromJava("From Java: " + message);
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

