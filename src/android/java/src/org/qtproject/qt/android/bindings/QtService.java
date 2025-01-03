// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android.bindings;

import android.app.Service;
import android.util.Log;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.IBinder;

import org.qtproject.qt.android.QtNative;

public class QtService extends Service
{
    QtServiceLoader m_loader = new QtServiceLoader(this);


    /////////////////////////// forward all notifications ////////////////////////////
    /////////////////////////// Super class calls ////////////////////////////////////
    /////////////// PLEASE DO NOT CHANGE THE FOLLOWING CODE //////////////////////////
    //////////////////////////////////////////////////////////////////////////////////
    protected void onCreateHook() {
        // the application has already started
        // do not reload everything again
        if (QtNative.isStarted()) {
            m_loader = null;
            Log.w(QtNative.QtTAG,
                "A QtService tried to start in the same process as an initiated " +
                "QtActivity. That is not supported. This results in the service " +
                "functioning as an Android Service detached from Qt.");
        } else {
            m_loader.onCreate();
        }
    }
    @Override
    public void onCreate()
    {
        super.onCreate();
        onCreateHook();
    }
    //---------------------------------------------------------------------------

    @Override
    public void onDestroy()
    {
        super.onDestroy();
        QtApplication.invokeDelegate();
    }
    //---------------------------------------------------------------------------

    @Override
    public IBinder onBind(Intent intent)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(intent);
        if (res.invoked)
            return (IBinder)res.methodReturns;
        else
            return null;
    }
    //---------------------------------------------------------------------------

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        if (!QtApplication.invokeDelegate(newConfig).invoked)
            super.onConfigurationChanged(newConfig);
    }
    public void super_onConfigurationChanged(Configuration newConfig)
    {
        super.onConfigurationChanged(newConfig);
    }
    //---------------------------------------------------------------------------

    @Override
    public void onLowMemory()
    {
        if (!QtApplication.invokeDelegate().invoked)
            super.onLowMemory();
    }
    //---------------------------------------------------------------------------

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(intent, flags, startId);
        if (res.invoked)
            return (Integer) res.methodReturns;
        else
            return super.onStartCommand(intent, flags, startId);
    }
    public int super_onStartCommand(Intent intent, int flags, int startId)
    {
        return super.onStartCommand(intent, flags, startId);
    }
    //---------------------------------------------------------------------------

    @Override
    public void onTaskRemoved(Intent rootIntent)
    {
        if (!QtApplication.invokeDelegate(rootIntent).invoked)
            super.onTaskRemoved(rootIntent);
    }
    public void super_onTaskRemoved(Intent rootIntent)
    {
        super.onTaskRemoved(rootIntent);
    }
    //---------------------------------------------------------------------------

    @Override
    public void onTrimMemory(int level)
    {
        if (!QtApplication.invokeDelegate(level).invoked)
            super.onTrimMemory(level);
    }
    public void super_onTrimMemory(int level)
    {
        super.onTrimMemory(level);
    }
    //---------------------------------------------------------------------------

    @Override
    public boolean onUnbind(Intent intent)
    {
        QtApplication.InvokeResult res = QtApplication.invokeDelegate(intent);
        if (res.invoked)
            return (Boolean) res.methodReturns;
        else
            return super.onUnbind(intent);
    }
    public boolean super_onUnbind(Intent intent)
    {
        return super.onUnbind(intent);
    }
    //---------------------------------------------------------------------------

    public boolean loadApplication(Service service, ClassLoader classLoader, Bundle loaderParams)
    {
        return QtNative.serviceDelegate().loadApplication(service, classLoader, loaderParams);
    }

    public boolean startApplication()
    {
       return QtNative.serviceDelegate().startApplication();
    }
}
