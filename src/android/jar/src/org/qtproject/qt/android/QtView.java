// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;

import java.security.InvalidParameterException;
import java.util.ArrayList;

// Base class for embedding QWindow into native Android view hierarchy. Extend to implement
// the creation of appropriate window to embed.
abstract class QtView extends ViewGroup {
    private final static String TAG = "QtView";

    public interface QtWindowListener {
        // Called when the QWindow has been created and it's Java counterpart embedded into
        // QtView
        void onQtWindowLoaded();
    }

    protected QtWindow m_window;
    protected long m_windowReference;
    protected QtWindowListener m_windowListener;
    protected QtEmbeddedDelegate m_delegate;
    // Implement in subclass to handle the creation of the QWindow and its parent container.
    // TODO could we take care of the parent window creation and parenting outside of the
    // window creation method to simplify things if user would extend this? Preferably without
    // too much JNI back and forth. Related to parent window creation, so handle with QTBUG-121511.
    abstract protected void createWindow(long parentWindowRef);

    private static native void setWindowVisible(long windowReference, boolean visible);
    private static native void resizeWindow(long windowReference,
                                            int x, int y, int width, int height);

    /**
     * Create QtView for embedding a QWindow. Instantiating a QtView will load the Qt libraries
     * if they have not already been loaded, including the app library specified by appName, and
     * starting the said Qt app.
     * @param context the hosting Context
     * @param appLibName the name of the Qt app library to load and start. This corresponds to the
              target name set in Qt app's CMakeLists.txt
    **/
    public QtView(Context context, String appLibName) throws InvalidParameterException {
        super(context);
        if (appLibName == null || appLibName.isEmpty()) {
            throw new InvalidParameterException("QtView: argument 'appLibName' may not be empty "+
                                                "or null");
        }

        QtEmbeddedLoader loader = new QtEmbeddedLoader(context);
        m_delegate = QtEmbeddedDelegateFactory.create((Activity)context);
        loader.setMainLibraryName(appLibName);
        addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                                       int oldLeft, int oldTop, int oldRight, int oldBottom)  {
                if (m_windowReference != 0L) {
                    final int oldWidth = oldRight - oldLeft;
                    final int oldHeight = oldBottom - oldTop;
                    final int newWidth = right - left;
                    final int newHeight = bottom - top;
                    if (oldWidth != newWidth || oldHeight != newHeight || left != oldLeft ||
                        top != oldTop) {
                            resizeWindow(m_windowReference, left, top, newWidth, newHeight);
                    }
                }
            }
        });
        loader.loadQtLibraries();
        // Start Native Qt application
        m_delegate.startNativeApplication(loader.getApplicationParameters(),
                                          loader.getMainLibraryPath());
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        m_delegate.setView(this);
        m_delegate.queueLoadWindow();
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        destroyWindow();
        m_delegate.setView(null);
    }

    @Override
    public void onLayout(boolean changed, int l, int t, int r, int b) {
        if (m_window != null)
            m_window.layout(0 /* left */, 0 /* top */, r - l /* right */, b - t /* bottom */);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
    {
        measureChildren(widthMeasureSpec, heightMeasureSpec);

        final int count = getChildCount();

        int maxHeight = 0;
        int maxWidth = 0;

        // Find out how big everyone wants to be
        measureChildren(widthMeasureSpec, heightMeasureSpec);

        // Find rightmost and bottom-most child
        for (int i = 0; i < count; i++) {
            View child = getChildAt(i);
            if (child.getVisibility() != GONE) {
                maxWidth = Math.max(maxWidth, child.getMeasuredWidth());
                maxHeight = Math.max(maxHeight, child.getMeasuredHeight());
            }
        }

        // Check against minimum height and width
        maxHeight = Math.max(maxHeight, getSuggestedMinimumHeight());
        maxWidth = Math.max(maxWidth, getSuggestedMinimumWidth());

        setMeasuredDimension(resolveSize(maxWidth, widthMeasureSpec),
                resolveSize(maxHeight, heightMeasureSpec));
    }


    public void setQtWindowListener(QtWindowListener listener) {
        m_windowListener = listener;
    }

    void setWindowReference(long windowReference) {
        m_windowReference = windowReference;
    }

    long windowReference() {
        return m_windowReference;
    }

    // Set the visibility of the underlying QWindow. If visible is true, showNormal() is called.
    // If false, the window is hidden.
    void setWindowVisible(boolean visible) {
        if (m_windowReference != 0L)
            setWindowVisible(m_windowReference, true);
    }

    // Called from Qt when the QWindow has been created.
    // window - the Java QtWindow of the created QAndroidPlatformWindow, to embed into the QtView
    // viewReference - the reference to the created QQuickView
    void addQtWindow(QtWindow window, long viewReference, long parentWindowRef) {
        setWindowReference(viewReference);
        m_delegate.setRootWindowRef(parentWindowRef);
        final Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                m_window = window;
                m_window.setLayoutParams(new ViewGroup.LayoutParams(
                                            ViewGroup.LayoutParams.MATCH_PARENT,
                                            ViewGroup.LayoutParams.MATCH_PARENT));
                addView(m_window, 0);
                // Call show window + parent
                setWindowVisible(true);
                if (m_windowListener != null)
                    m_windowListener.onQtWindowLoaded();
            }
        });
    }

    // Destroy the underlying QWindow
    void destroyWindow() {
        if (m_windowReference != 0L)
            QtEmbeddedDelegate.deleteWindow(m_windowReference);
        m_windowReference = 0L;
    }

    QtWindow getQtWindow() {
        return m_window;
    }
}
