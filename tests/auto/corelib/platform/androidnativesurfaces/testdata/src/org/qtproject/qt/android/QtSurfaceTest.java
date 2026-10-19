// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.app.Activity;
import android.graphics.PixelFormat;
import android.view.Surface;
import android.view.SurfaceControl;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public final class QtSurfaceTest
{
    private static class CountingPopup extends EditPopupMenu
    {
        int m_calls;

        CountingPopup(QtEditText editText) { super(editText); }

        @Override
        public boolean onPreDraw()
        {
            ++m_calls;
            return super.onPreDraw();
        }
    }

    public static String editPopupDetaches(Context context)
    {
        final QtEditText editText = new QtEditText(context, null);
        editText.setFocusable(false);
        final CountingPopup popup = new CountingPopup(editText);
        final ViewGroup root = (ViewGroup)((Activity)context).getWindow().getDecorView();
        try {
            root.addView(editText, new ViewGroup.LayoutParams(1, 1));
            final ViewTreeObserver observer = editText.getViewTreeObserver();
            observer.dispatchOnPreDraw();
            if (popup.m_calls == 0)
                return "An attached popup did not observe pre-draw";
            root.removeView(editText);
            final int detachedCalls = popup.m_calls;
            observer.dispatchOnPreDraw();
            if (popup.m_calls != detachedCalls)
                return "A detached popup still observes the activity tree";
            root.addView(editText, new ViewGroup.LayoutParams(1, 1));
            observer.dispatchOnPreDraw();
            if (popup.m_calls != detachedCalls + 1)
                return "Reattaching did not restore exactly one popup listener";
            root.removeView(editText);
            observer.dispatchOnPreDraw();
            if (popup.m_calls != detachedCalls + 1)
                return "A reattached popup retained its listener after detach";
            return "";
        } finally {
            root.removeView(editText);
        }
    }

    private static class Window extends QtWindow
    {
        RuntimeException m_failure;

        Window(Context context)
        {
            super(context, true, null, null);
        }

        @Override
        public void removeView(View view)
        {
            if (m_failure != null)
                throw m_failure;
            super.removeView(view);
        }
    }

    public static String failedRemoval(Context context) throws ReflectiveOperationException
    {
        final RuntimeException failure = new RuntimeException("Injected removal failure");
        Window window = new Window(context);
        window.m_failure = failure;
        View container = new View(context);
        window.addView(container);
        Field surface = field(QtWindow.class, "m_surfaceContainer");
        surface.set(window, container);
        Throwable actual = destroy(window);
        if (actual == null)
            return "The injected removal failure was not propagated";
        if (actual != failure)
            return "The removal failure was replaced";
        final int[] releases = new int[1];
        window.onSurfaceDestroyed(() -> ++releases[0]);
        if (releases[0] != 1)
            return "A late callback waited after native retirement";
        if (surface.get(window) != container)
            return "Failed removal discarded its container";
        window.m_failure = null;
        method(QtWindow.class, "destroySurfaceOnUiThread").invoke(window);
        if (surface.get(window) != null)
            return "Successful retry did not finish container destruction";
        return "";
    }

    public static String retirementAccounting(Context context) throws ReflectiveOperationException
    {
        return retirementAccounting(context, false);
    }

    public static String retirementRemovalFailure(Context context)
            throws ReflectiveOperationException
    {
        return retirementAccounting(context, true);
    }

    private static String retirementAccounting(Context context, boolean failRemoval)
            throws ReflectiveOperationException
    {
        RuntimeException removal = new RuntimeException("Injected retirement removal failure");
        Window window = new Window(context);
        window.m_failure = removal;
        if (failRemoval)
            field(QtWindow.class, "m_surfaceContainer").set(window, new View(context));
        final int[] calls = new int[5];
        RuntimeException first = new RuntimeException("Injected first release failure");
        RuntimeException second = new RuntimeException("Injected second release failure");
        window.onSurfaceDestroyed(() -> ++calls[0]);
        window.onSurfaceDestroyed(() -> { ++calls[1]; throw first; });
        window.onSurfaceDestroyed(() -> { ++calls[2]; throw second; });
        window.onSurfaceDestroyed(() -> ++calls[3]);
        Throwable actual = destroy(window);
        for (int i = 0; i < 4; ++i) {
            if (calls[i] != 1)
                return "Retirement did not drain every completion exactly once";
        }
        if (failRemoval) {
            if (actual != removal || actual.getSuppressed().length != 1 || actual.getSuppressed()[0] != first)
                return "The removal error was replaced by a release error";
            field(QtWindow.class, "m_surfaceContainer").set(window, null);
        }
        if ((!failRemoval && actual != first) || first.getSuppressed().length != 1 || first.getSuppressed()[0] != second)
            return "Retirement did not preserve both release failures";
        window.onSurfaceDestroyed(() -> ++calls[4]);
        if (calls[4] != 1)
            return "Late completion was queued after native retirement";
        method(QtWindow.class, "destroySurfaceOnUiThread").invoke(window);
        for (int count : calls) {
            if (count != 1)
                return "Repeated retirement executed a completion again";
        }
        return "";
    }

    private static Field field(Class<?> type, String name) throws ReflectiveOperationException
    {
        Field field = type.getDeclaredField(name);
        field.setAccessible(true);
        return field;
    }

    private static Method method(Class<?> type, String name, Class<?>... arguments)
            throws ReflectiveOperationException
    {
        Method method = type.getDeclaredMethod(name, arguments);
        method.setAccessible(true);
        return method;
    }

    private static Throwable destroy(QtWindow window) throws ReflectiveOperationException
    {
        try {
            method(QtWindow.class, "destroySurfaceOnUiThread").invoke(window);
            return null;
        } catch (InvocationTargetException failure) {
            return failure.getCause();
        }
    }

    public static String failedInitialization(Context context) throws ReflectiveOperationException
    {
        QtSurface view = new QtSurface(context, null, false, 32);
        SurfaceControl control = new SurfaceControl.Builder().setName("QtSurfaceTest")
                .setBufferSize(64, 64).build();
        Surface surface = new Surface(control);
        try {
            // Reject visibility after both resources have been allocated.
            control.release();
            try {
                method(QtSurface.class, "initializeRenderSurface", SurfaceControl.class,Surface.class)
                    .invoke(view, control, surface);
                return "The invalid control was accepted during initialization";
            } catch (InvocationTargetException expected) {
                Throwable cause = expected.getCause();
                if (!(cause instanceof IllegalStateException || cause instanceof NullPointerException))
                    throw expected;
            }
            if (surface.isValid())
                return "Failed initialization did not release its unpublished Surface";
            final Field renderControl = field(QtSurface.class, "m_renderControl");
            final Field renderSurface = field(QtSurface.class, "m_renderSurface");
            if (renderControl.get(view) != null || renderSurface.get(view) != null)
                return "Failed initialization published partial resources";
            return "";
        } finally {
            surface.release();
            control.release();
        }
    }

    public static String failedDetach(Context context) throws ReflectiveOperationException
    {
        return failedDetach(context, false);
    }

    public static String failedDetachAndCallback(Context context)
            throws ReflectiveOperationException
    {
        return failedDetach(context, true);
    }

    private static String failedDetach(Context context, boolean failCallback)
            throws ReflectiveOperationException
    {
        final Runnable[] release = new Runnable[1];
        final int[] notifications = new int[1];
        final RuntimeException callbackFailure = new RuntimeException("Injected callback failure");
        QtSurfaceInterface callback = new QtSurfaceInterface() {
            @Override
            public void onSurfaceCreated(Surface surface) {}
            @Override
            public void onSurfaceRedrawNeeded(Runnable finished) { finished.run(); }
            @Override
            public void onSurfaceDestroyed(Runnable released)
            {
                ++notifications[0];
                release[0] = released;
                if (failCallback)
                    throw callbackFailure;
            }
        };
        QtSurface view = new QtSurface(context, callback, false, 32);
        SurfaceControl control = new SurfaceControl.Builder().setName("QtSurfaceTest")
                .setBufferSize(64, 64).build();
        Surface surface = new Surface(control);
        try {
            field(QtSurface.class, "m_renderControl").set(view, control);
            field(QtSurface.class, "m_renderSurface").set(view, surface);
            // Reject the detach while the Surface still owns its producer.
            control.release();
            boolean rejected = false;
            boolean callbackFailurePreserved = false;
            try {
                view.surfaceDestroyed(null);
            } catch (IllegalStateException | NullPointerException expected) {
                rejected = true;
                Throwable[] suppressed = expected.getSuppressed();
                callbackFailurePreserved = suppressed.length == 1 && suppressed[0] == callbackFailure;
            }
            if (!rejected)
                return "The injected invalid control was not rejected";
            if (notifications[0] != 1 || release[0] == null)
                return "A rejected detach skipped native retirement notification";
            if (failCallback && !callbackFailurePreserved)
                return "Retirement callback failure was not preserved with the detach failure";
            if (!surface.isValid())
                return "The producer was released before native retirement";
            release[0].run();
            if (surface.isValid())
                return "The retired producer was not released";
            return "";
        } finally {
            surface.release();
            control.release();
        }
    }
}
