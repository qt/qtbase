// Copyright (C) 2017 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.extras;

import android.os.IBinder;
import android.os.Parcel;

class QtNative {
    // Binder
    static native boolean onTransact(long id, int code, Parcel data, Parcel reply, int flags);


    // ServiceConnection
    static native void onServiceConnected(long id, String name, IBinder service);
    static native void onServiceDisconnected(long id, String name);
}
