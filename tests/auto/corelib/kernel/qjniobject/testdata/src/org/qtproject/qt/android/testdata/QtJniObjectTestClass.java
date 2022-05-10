// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package org.qtproject.qt.android.testdatapackage;

public class QtJniObjectTestClass
{
    static final byte A_BYTE_VALUE = 127;
    static final short A_SHORT_VALUE = 32767;
    static final int A_INT_VALUE = 060701;
    static final long A_LONG_VALUE = 060701;
    static final float A_FLOAT_VALUE = 1.0f;
    static final double A_DOUBLE_VALUE = 1.0;
    static final boolean A_BOOLEAN_VALUE = true;
    static final char A_CHAR_VALUE = 'Q';
    static final String A_STRING_OBJECT = "TEST_DATA_STRING";
    static final Class A_CLASS_OBJECT = QtJniObjectTestClass.class;
    static final Object A_OBJECT_OBJECT = new QtJniObjectTestClass();
    static final Throwable A_THROWABLE_OBJECT = new Throwable(A_STRING_OBJECT);

    // --------------------------------------------------------------------------------------------

    byte BYTE_VAR;
    short SHORT_VAR;
    int INT_VAR;
    long LONG_VAR;
    float FLOAT_VAR;
    double DOUBLE_VAR;
    boolean BOOLEAN_VAR;
    char CHAR_VAR;
    String STRING_OBJECT_VAR;

    static byte S_BYTE_VAR;
    static short S_SHORT_VAR;
    static int S_INT_VAR;
    static long S_LONG_VAR;
    static float S_FLOAT_VAR;
    static double S_DOUBLE_VAR;
    static boolean S_BOOLEAN_VAR;
    static char S_CHAR_VAR;
    static String S_STRING_OBJECT_VAR;

    // --------------------------------------------------------------------------------------------
    public static void staticVoidMethod() { return; }
    public static void staticVoidMethodWithArgs(int a, boolean b, char c) { return; }

    public void voidMethod() { return; }
    public void voidMethodWithArgs(int a, boolean b, char c) { return; }

    // --------------------------------------------------------------------------------------------
    public static boolean staticBooleanMethod() { return A_BOOLEAN_VALUE; }
    public static boolean staticBooleanMethodWithArgs(boolean a, boolean b, boolean c)
    { return staticBooleanMethod(); }

    public boolean booleanMethod() { return staticBooleanMethod(); }
    public boolean booleanMethodWithArgs(boolean a, boolean b, boolean c)
    { return staticBooleanMethodWithArgs(a, b, c); }

    // --------------------------------------------------------------------------------------------
    public static byte staticByteMethod() { return A_BYTE_VALUE; }
    public static byte staticByteMethodWithArgs(byte a, byte b, byte c) { return staticByteMethod(); }

    public byte byteMethod() { return staticByteMethod(); }
    public byte byteMethodWithArgs(byte a, byte b, byte c)
    { return staticByteMethodWithArgs(a, b, c); }

    // --------------------------------------------------------------------------------------------
    public static char staticCharMethod() { return A_CHAR_VALUE; }
    public static char staticCharMethodWithArgs(char a, char b, char c) { return staticCharMethod(); }

    public char charMethod() { return staticCharMethod(); }
    public char charMethodWithArgs(char a, char b, char c)
    { return staticCharMethodWithArgs(a, b, c); }

    // --------------------------------------------------------------------------------------------
    public static short staticShortMethod() { return A_SHORT_VALUE; }
    public static short staticShortMethodWithArgs(short a, short b, short c) { return staticShortMethod(); }

    public short shortMethod() { return staticShortMethod(); }
    public short shortMethodWithArgs(short a, short b, short c)
    { return staticShortMethodWithArgs(a, b, c); }

    // --------------------------------------------------------------------------------------------
    public static int staticIntMethod() { return A_INT_VALUE; }
    public static int staticIntMethodWithArgs(int a, int b, int c) { return staticIntMethod(); }

    public int intMethod() { return staticIntMethod(); }
    public int intMethodWithArgs(int a, int b, int c) { return staticIntMethodWithArgs(a, b, c); }

    // --------------------------------------------------------------------------------------------
    public static long staticLongMethod() { return A_LONG_VALUE; }
    public static long staticLongMethodWithArgs(long a, long b, long c) { return staticLongMethod(); }

    public long longMethod() { return staticLongMethod(); }
    public long longMethodWithArgs(long a, long b, long c)
    { return staticLongMethodWithArgs(a, b, c); }

    // --------------------------------------------------------------------------------------------
    public static float staticFloatMethod() { return A_FLOAT_VALUE; }
    public static float staticFloatMethodWithArgs(float a, float b, float c) { return staticFloatMethod(); }

    public float floatMethod() { return staticFloatMethod(); }
    public float floatMethodWithArgs(float a, float b, float c)
    { return staticFloatMethodWithArgs(a, b, c); }

    // --------------------------------------------------------------------------------------------
    public static double staticDoubleMethod() { return A_DOUBLE_VALUE; }
    public static double staticDoubleMethodWithArgs(double a, double b, double c)
    { return staticDoubleMethod(); }

    public double doubleMethod() { return staticDoubleMethod(); }
    public double doubleMethodWithArgs(double a, double b, double c)
    { return staticDoubleMethodWithArgs(a, b, c); }

    // --------------------------------------------------------------------------------------------
    public static Object staticObjectMethod() { return A_OBJECT_OBJECT; }
    public Object objectMethod() { return staticObjectMethod(); }

    // --------------------------------------------------------------------------------------------
    public static Class staticClassMethod() { return A_CLASS_OBJECT; }
    public Class classMethod() { return staticClassMethod(); }

    // --------------------------------------------------------------------------------------------
    public static String staticStringMethod() { return A_STRING_OBJECT; }
    public String stringMethod() { return staticStringMethod(); }

    // --------------------------------------------------------------------------------------------
    public static Throwable staticThrowableMethod() { return A_THROWABLE_OBJECT; }
    public Throwable throwableMethod() { return staticThrowableMethod(); }

    // --------------------------------------------------------------------------------------------
    public static Object[] staticObjectArrayMethod()
    { Object[] array = { new Object(), new Object(), new Object() }; return array; }
    public Object[] objectArrayMethod() { return staticObjectArrayMethod(); }

    // --------------------------------------------------------------------------------------------
    public static boolean[] staticBooleanArrayMethod()
    { boolean[] array = { true, true, true }; return array; }
    public boolean[] booleanArrayMethod() { return staticBooleanArrayMethod(); }

    // --------------------------------------------------------------------------------------------
    public static byte[] staticByteArrayMethod()
    { byte[] array = { 'a', 'b', 'c' }; return array; }
    public byte[] byteArrayMethod() { return staticByteArrayMethod(); }

    // --------------------------------------------------------------------------------------------
    public static char[] staticCharArrayMethod()
    { char[] array = { 'a', 'b', 'c' }; return array; }
    public char[] charArrayMethod() { return staticCharArrayMethod(); }

    // --------------------------------------------------------------------------------------------
    public static short[] staticShortArrayMethod() { short[] array = { 3, 2, 1 }; return array; }
    public short[] shortArrayMethod() { return staticShortArrayMethod(); }

    // --------------------------------------------------------------------------------------------
    public static int[] staticIntArrayMethod() { int[] array = { 3, 2, 1 }; return array; }
    public int[] intArrayMethod() { return staticIntArrayMethod(); }

    // --------------------------------------------------------------------------------------------
    public static long[] staticLongArrayMethod()
    { long[] array = { 3, 2, 1 }; return array; }
    public long[] longArrayMethod() { return staticLongArrayMethod(); }

    // --------------------------------------------------------------------------------------------
    public static float[] staticFloatArrayMethod()
    { float[] array = { 1.0f, 2.0f, 3.0f }; return array; }
    public float[] floatArrayMethod() { return staticFloatArrayMethod(); }

    // --------------------------------------------------------------------------------------------
    public static double[] staticDoubleArrayMethod()
    { double[] array = { 3.0, 2.0, 1.0 }; return array; }
    public double[] doubleArrayMethod() { return staticDoubleArrayMethod(); }
}

