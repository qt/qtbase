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

/**
 * QtServiceEmbeddedDelegate is used for embedding QML into Android Service contexts. Implements
 * {@link QtEmbeddedViewInterface} so it can be used by QtView to communicate with the Qt layer.
 */
class QtServiceEmbeddedDelegate implements QtEmbeddedViewInterface, QtNative.AppStateDetailsListener
{
    private final Service m_service;
    private QtView m_view;
    private boolean m_windowLoaded = false;

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
                    if (m_view == null)
                        return;

                    final DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();

                    final int maxWidth = m_view.getWidth();
                    final int maxHeight = m_view.getHeight();
                    final int width = maxWidth;
                    final int height = maxHeight;
                    final int insetLeft = m_view.getLeft();
                    final int insetTop = m_view.getTop();

                    final DisplayManager dm = m_service.getSystemService(DisplayManager.class);
                    QtDisplayManager.setDisplayMetrics(
                            maxWidth, maxHeight, insetLeft, insetTop, width, height,
                            QtDisplayManager.getXDpi(metrics), QtDisplayManager.getYDpi(metrics),
                            metrics.scaledDensity, metrics.density,
                            QtDisplayManager.getRefreshRate(
                                    dm.getDisplay(Display.DEFAULT_DISPLAY)));
                });
                createRootWindow();
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
    public void setView(QtView view)
    {
        m_view = view;
        // If the embedded view is destroyed, do cleanup:
        if (view == null)
            cleanup();
    }

    @Override
    public void queueLoadWindow()
    {
        synchronized (this) {
            if (QtNative.getStateDetails().nativePluginIntegrationReady)
                createRootWindow();
        }
    }
    // QtEmbeddedViewInterface implementation end

    private void createRootWindow()
    {
        if (m_view != null && !m_windowLoaded) {
            QtView.createRootWindow(m_view, m_view.getLeft(), m_view.getTop(), m_view.getWidth(),
                                    m_view.getHeight());
            m_windowLoaded = true;
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
