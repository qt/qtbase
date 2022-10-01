// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android.bindings;

import android.app.Application;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;

public class QtApplication extends Application
{
    public final static String QtTAG = "Qt";
    public static Object m_delegateObject = null;
    public static HashMap<String, ArrayList<Method>> m_delegateMethods= new HashMap<String, ArrayList<Method>>();
    public static Method dispatchKeyEvent = null;
    public static Method dispatchPopulateAccessibilityEvent = null;
    public static Method dispatchTouchEvent = null;
    public static Method dispatchTrackballEvent = null;
    public static Method onKeyDown = null;
    public static Method onKeyMultiple = null;
    public static Method onKeyUp = null;
    public static Method onTouchEvent = null;
    public static Method onTrackballEvent = null;
    public static Method onActivityResult = null;
    public static Method onCreate = null;
    public static Method onKeyLongPress = null;
    public static Method dispatchKeyShortcutEvent = null;
    public static Method onKeyShortcut = null;
    public static Method dispatchGenericMotionEvent = null;
    public static Method onGenericMotionEvent = null;
    public static Method onRequestPermissionsResult = null;
    private static String activityClassName;
    public static void setQtContextDelegate(Class<?> clazz, Object listener)
    {
        m_delegateObject = listener;
        activityClassName = clazz.getCanonicalName();

        ArrayList<Method> delegateMethods = new ArrayList<Method>();
        for (Method m : listener.getClass().getMethods()) {
            if (m.getDeclaringClass().getName().startsWith("org.qtproject.qt.android"))
                delegateMethods.add(m);
        }

        ArrayList<Field> applicationFields = new ArrayList<Field>();
        for (Field f : QtApplication.class.getFields()) {
            if (f.getDeclaringClass().getName().equals(QtApplication.class.getName()))
                applicationFields.add(f);
        }

        for (Method delegateMethod : delegateMethods) {
            try {
                clazz.getDeclaredMethod(delegateMethod.getName(), delegateMethod.getParameterTypes());
                if (QtApplication.m_delegateMethods.containsKey(delegateMethod.getName())) {
                    QtApplication.m_delegateMethods.get(delegateMethod.getName()).add(delegateMethod);
                } else {
                    ArrayList<Method> delegateSet = new ArrayList<Method>();
                    delegateSet.add(delegateMethod);
                    QtApplication.m_delegateMethods.put(delegateMethod.getName(), delegateSet);
                }
                for (Field applicationField:applicationFields) {
                    if (applicationField.getName().equals(delegateMethod.getName())) {
                        try {
                            applicationField.set(null, delegateMethod);
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                }
            } catch (Exception e) { }
        }
    }

    @Override
    public void onTerminate() {
        if (m_delegateObject != null && m_delegateMethods.containsKey("onTerminate"))
            invokeDelegateMethod(m_delegateMethods.get("onTerminate").get(0));
        super.onTerminate();
    }

    public static class InvokeResult
    {
        public boolean invoked = false;
        public Object methodReturns = null;
    }

    private static int stackDeep=-1;
    public static InvokeResult invokeDelegate(Object... args)
    {
        InvokeResult result = new InvokeResult();
        if (m_delegateObject == null)
            return result;
        StackTraceElement[] elements = Thread.currentThread().getStackTrace();
        if (-1 == stackDeep) {
            for (int it=0;it<elements.length;it++)
                if (elements[it].getClassName().equals(activityClassName)) {
                    stackDeep = it;
                    break;
                }
        }
        if (-1 == stackDeep)
            return result;

        final String methodName=elements[stackDeep].getMethodName();
        if (!m_delegateMethods.containsKey(methodName))
            return result;

        for (Method m : m_delegateMethods.get(methodName)) {
            if (m.getParameterTypes().length == args.length) {
                result.methodReturns = invokeDelegateMethod(m, args);
                result.invoked = true;
                return result;
            }
        }
        return result;
    }

    public static Object invokeDelegateMethod(Method m, Object... args)
    {
        try {
            return m.invoke(m_delegateObject, args);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
}
