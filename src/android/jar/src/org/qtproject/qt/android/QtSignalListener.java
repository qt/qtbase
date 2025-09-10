// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

/**
 * A callback that notifies clients when a signal is emitted from the QML component.
 **/
@FunctionalInterface
public interface QtSignalListener<T> {
    /**
     * Called on the Android UI thread when the signal has been emitted.
     * @param signalName literal signal name
     * @param value the value delivered by the signal or null if the signal is parameterless
     **/
    void onSignalEmitted(String signalName, T value);
}
