// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

    final int INT_FIELD = 123;
    final boolean BOOL_FIELD = true;

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

    static char[] S_CHAR_ARRAY = A_STRING_OBJECT.toCharArray();

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
    public static String staticEchoMethod(String value) { return value; }

    // --------------------------------------------------------------------------------------------
    public static Throwable staticThrowableMethod() { return A_THROWABLE_OBJECT; }
    public Throwable throwableMethod() { return staticThrowableMethod(); }

    // --------------------------------------------------------------------------------------------
    public static Object[] staticObjectArrayMethod()
    { Object[] array = { new Object(), new Object(), new Object() }; return array; }
    public Object[] objectArrayMethod() { return staticObjectArrayMethod(); }
    public static Object[] staticReverseObjectArray(Object[] array)
    {
        for (int i = 0; i < array.length / 2; ++i) {
            Object old = array[array.length - i - 1];
            array[array.length - i - 1] = array[i];
            array[i] = old;
        }
        return array;
    }
    public Object[] reverseObjectArray(Object[] array)
    { return staticReverseObjectArray(array); }

    // --------------------------------------------------------------------------------------------
    public static String[] staticStringArrayMethod()
    { String[] array = { "First", "Second", "Third" }; return array; }
    public String[] stringArrayMethod() { return staticStringArrayMethod(); }

    // --------------------------------------------------------------------------------------------
    public static boolean[] staticBooleanArrayMethod()
    { boolean[] array = { true, true, true }; return array; }
    public boolean[] booleanArrayMethod() { return staticBooleanArrayMethod(); }
    public static boolean[] staticReverseBooleanArray(boolean[] array)
    {
        for (int i = 0; i < array.length / 2; ++i) {
            boolean old = array[array.length - i - 1];
            array[array.length - i - 1] = array[i];
            array[i] = old;
        }
        return array;
    }
    public boolean[] reverseBooleanArray(boolean[] array)
    { return staticReverseBooleanArray(array); }

    // --------------------------------------------------------------------------------------------
    public static byte[] staticByteArrayMethod()
    { byte[] array = { 'a', 'b', 'c' }; return array; }
    public byte[] byteArrayMethod() { return staticByteArrayMethod(); }
    public static byte[] staticReverseByteArray(byte[] array)
    {
        for (int i = 0; i < array.length / 2; ++i) {
            byte old = array[array.length - i - 1];
            array[array.length - i - 1] = array[i];
            array[i] = old;
        }
        return array;
    }
    public byte[] reverseByteArray(byte[] array)
    { return staticReverseByteArray(array); }

    // --------------------------------------------------------------------------------------------
    public static char[] staticCharArrayMethod()
    { char[] array = { 'a', 'b', 'c' }; return array; }
    public char[] charArrayMethod() { return staticCharArrayMethod(); }
    public static char[] staticReverseCharArray(char[] array)
    {
        for (int i = 0; i < array.length / 2; ++i) {
            char old = array[array.length - i - 1];
            array[array.length - i - 1] = array[i];
            array[i] = old;
        }
        return array;
    }
    public char[] reverseCharArray(char[] array)
    { return staticReverseCharArray(array); }

    // --------------------------------------------------------------------------------------------
    public static char[] getStaticCharArray()
    { return S_CHAR_ARRAY; }
    public static void mutateStaticCharArray(char [] values)
    {
        for (int i = 0; i < values.length; ++i) {
            S_CHAR_ARRAY[i] = values[i];
        }
    }
    public static void replaceStaticCharArray(char[] array)
    { S_CHAR_ARRAY = array; }

    // --------------------------------------------------------------------------------------------
    public static short[] staticShortArrayMethod() { short[] array = { 3, 2, 1 }; return array; }
    public short[] shortArrayMethod() { return staticShortArrayMethod(); }
    public static short[] staticReverseShortArray(short[] array)
    {
        for (int i = 0; i < array.length / 2; ++i) {
            short old = array[array.length - i - 1];
            array[array.length - i - 1] = array[i];
            array[i] = old;
        }
        return array;
    }
    public short[] reverseShortArray(short[] array)
    { return staticReverseShortArray(array); }

    // --------------------------------------------------------------------------------------------
    public static int[] staticIntArrayMethod() { int[] array = { 3, 2, 1 }; return array; }
    public int[] intArrayMethod() { return staticIntArrayMethod(); }
    public static int[] staticReverseIntArray(int[] array)
    {
        for (int i = 0; i < array.length / 2; ++i) {
            int old = array[array.length - i - 1];
            array[array.length - i - 1] = array[i];
            array[i] = old;
        }
        return array;
    }
    public int[] reverseIntArray(int[] array)
    { return staticReverseIntArray(array); }

    // --------------------------------------------------------------------------------------------
    public static long[] staticLongArrayMethod()
    { long[] array = { 3, 2, 1 }; return array; }
    public long[] longArrayMethod() { return staticLongArrayMethod(); }
    public static long[] staticReverseLongArray(long[] array)
    {
        for (int i = 0; i < array.length / 2; ++i) {
            long old = array[array.length - i - 1];
            array[array.length - i - 1] = array[i];
            array[i] = old;
        }
        return array;
    }
    public long[] reverseLongArray(long[] array)
    { return staticReverseLongArray(array); }

    // --------------------------------------------------------------------------------------------
    public static float[] staticFloatArrayMethod()
    { float[] array = { 1.0f, 2.0f, 3.0f }; return array; }
    public float[] floatArrayMethod() { return staticFloatArrayMethod(); }
    public static float[] staticReverseFloatArray(float[] array)
    {
        for (int i = 0; i < array.length / 2; ++i) {
            float old = array[array.length - i - 1];
            array[array.length - i - 1] = array[i];
            array[i] = old;
        }
        return array;
    }
    public float[] reverseFloatArray(float[] array)
    { return staticReverseFloatArray(array); }

    // --------------------------------------------------------------------------------------------
    public static double[] staticDoubleArrayMethod()
    { double[] array = { 3.0, 2.0, 1.0 }; return array; }
    public double[] doubleArrayMethod() { return staticDoubleArrayMethod(); }
    public static double[] staticReverseDoubleArray(double[] array)
    {
        for (int i = 0; i < array.length / 2; ++i) {
            double old = array[array.length - i - 1];
            array[array.length - i - 1] = array[i];
            array[i] = old;
        }
        return array;
    }
    public double[] reverseDoubleArray(double[] array)
    { return staticReverseDoubleArray(array); }

    // --------------------------------------------------------------------------------------------
    native public int callbackWithObject(QtJniObjectTestClass that);
    native public int callbackWithObjectRef(QtJniObjectTestClass that);
    native public int callbackWithString(String string);
    native public int callbackWithByte(byte value);
    native public int callbackWithBoolean(boolean value);
    native public int callbackWithInt(int value);
    native public int callbackWithDouble(double value);
    native public int callbackWithFloat(float value);
    native public static int callbackWithFloatStatic(float value);
    native public int callbackWithJniArray(double[] value);
    native public int callbackWithRawArray(Object[] value);
    native public int callbackWithQList(double[] value);
    native public int callbackWithStringList(String[] value);
    native public int callbackWithNull(String str);
    native public int callbackWithMany(byte b, short s, int i, long l, float f, double d,
                                       boolean bo, char c, String str);

    public int callMeBackWithObject(QtJniObjectTestClass that)
    {
        return callbackWithObject(that);
    }

    public int callMeBackWithObjectRef(QtJniObjectTestClass that)
    {
        return callbackWithObjectRef(that);
    }

    public int callMeBackWithString(String string)
    {
        return callbackWithString(string);
    }

    public int callMeBackWithByte(byte value)
    {
        return callbackWithByte(value);
    }

    public int callMeBackWithBoolean(boolean value)
    {
        return callbackWithBoolean(value);
    }

    public int callMeBackWithInt(int value)
    {
        return callbackWithInt(value);
    }

    public int callMeBackWithDouble(double value)
    {
        return callbackWithDouble(value);
    }

    public int callMeBackWithFloat(float value)
    {
        return callbackWithFloat(value);
    }
    public int callMeBackWithFloatStatic(float value)
    {
        return callbackWithFloatStatic(value);
    }

    public int callMeBackWithJniArray(double[] value)
    {
        return callbackWithJniArray(value);
    }
    public int callMeBackWithRawArray(Object[] value)
    {
        return callbackWithRawArray(value);
    }
    public int callMeBackWithQList(double[] value)
    {
        return callbackWithQList(value);
    }
    public int callMeBackWithStringList(String[] value)
    {
        return callbackWithStringList(value);
    }
    public int callMeBackWithNull()
    {
        return callbackWithNull(null);
    }
    public int callMeBackWithMany()
    {
        return callbackWithMany(A_BYTE_VALUE, A_SHORT_VALUE, A_INT_VALUE, A_LONG_VALUE,
                                A_FLOAT_VALUE, A_DOUBLE_VALUE, A_BOOLEAN_VALUE,
                                A_CHAR_VALUE, A_STRING_OBJECT);
    }

    public Object callMethodThrowsException() throws Exception {
        throw new Exception();
    }

    public static Object callStaticMethodThrowsException() throws Exception {
        throw new Exception();
    }
}
