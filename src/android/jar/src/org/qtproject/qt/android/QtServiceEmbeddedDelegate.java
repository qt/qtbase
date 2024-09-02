// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import static org.qtproject.qt.android.QtNative.ApplicationState.ApplicationSuspended;

import android.app.Service;
import android.content.Context;
import android.content.res.Resources;
import android.hardware.display.DisplayManager;
import android.view.Display;
import android.view.View;
import android.util.DisplayMetrics;

import java.util.HashSet;

/**
 * QtServiceEmbeddedDelegate is used for embedding QML into Android Service contexts. Implements
 * {@link QtEmbeddedViewInterface} so it can be used by QtView to communicate with the Qt layer.
 */
class QtServiceEmbeddedDelegate implements QtEmbeddedViewInterface, QtNative.AppStateDetailsListener
{
    private final Service m_service;
    private HashSet<QtView> m_views = new HashSet<QtView>();

    QtServiceEmbeddedDelegate(Service service)
    {
        m_service = service;
        QtNative.registerAppStateListener(this);
        QtNative.setService(service);
        // QTBUG-122920 TODO Implement accessibility for service UIs
        // QTBUG-122552 TODO Implement text input
    }

    @Override
    public void onNativePluginIntegrationReadyChanged(boolean ready)
    {
        synchronized (this) {
            if (ready) {
                QtNative.runAction(() -> {
                    final DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();

                    final int maxWidth = metrics.widthPixels;
                    final int maxHeight = metrics.heightPixels;
                    final int width = maxWidth;
                    final int height = maxHeight;
                    final int insetLeft = 0;
                    final int insetTop = 0;

                    final DisplayManager dm = m_service.getSystemService(DisplayManager.class);
                    QtDisplayManager.setDisplayMetrics(
                            maxWidth, maxHeight, insetLeft, insetTop, width, height,
                            QtDisplayManager.getXDpi(metrics), QtDisplayManager.getYDpi(metrics),
                            metrics.scaledDensity, metrics.density,
                            QtDisplayManager.getRefreshRate(
                                    dm.getDisplay(Display.DEFAULT_DISPLAY)));
                });
            }
        }
    }

    // QtEmbeddedViewInterface implementation begin
    @Override
    public void startQtApplication(String appParams, String mainLib)
    {
        QtNative.startApplication(appParams, mainLib);
    }

    @Override
    public void addView(QtView view)
    {
        if (m_views.add(view)) {
            QtNative.runAction(() -> createRootWindow(view));
        }
    }

    @Override
    public void removeView(QtView view)
    {
        m_views.remove(view);
        if (m_views.isEmpty())
            cleanup();
    }
    // QtEmbeddedViewInterface implementation end

    private void createRootWindow(QtView view)
    {
        if (m_views.contains(view)) {
            QtView.createRootWindow(view, view.getLeft(), view.getTop(), view.getWidth(),
                                    view.getHeight());
        }
    }

    private void cleanup()
    {
        QtNative.setApplicationState(ApplicationSuspended);
        QtNative.unregisterAppStateListener(QtServiceEmbeddedDelegate.this);
        QtEmbeddedViewInterfaceFactory.remove(m_service);

        QtNative.terminateQt();
        QtNative.setService(null);
        QtNative.getQtThread().exit();
    }
}
