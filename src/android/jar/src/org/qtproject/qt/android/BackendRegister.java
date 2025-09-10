// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android;

class BackendRegister
{
    static native boolean isNull();
    static native void registerBackend(Class<?> interfaceType, Object interfaceObject);
    static native void unregisterBackend(Class<?> interfaceType);
}
