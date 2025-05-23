// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
package org.qtproject.qt.android;

class BackendRegister
{
    static native boolean isNull();
    static native void registerBackend(Class<?> interfaceType, Object interfaceObject);
    static native void unregisterBackend(Class<?> interfaceType);
}
