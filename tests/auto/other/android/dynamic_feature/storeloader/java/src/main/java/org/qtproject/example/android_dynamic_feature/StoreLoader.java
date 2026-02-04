// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.example.android_dynamic_feature;

import com.google.android.play.core.splitcompat.SplitCompat;
import com.google.android.play.core.splitinstall.SplitInstallManager;
import com.google.android.play.core.splitinstall.SplitInstallManagerFactory;
import com.google.android.play.core.splitinstall.SplitInstallRequest;
import com.google.android.play.core.splitinstall.SplitInstallHelper;
import com.google.android.play.core.splitinstall.SplitInstallException;
import com.google.android.play.core.splitinstall.testing.FakeSplitInstallManagerFactory;
import android.content.pm.PackageManager.NameNotFoundException;

import java.util.Arrays;
import java.util.HashMap;
import java.io.File;
import android.util.Log;
import android.content.Context;
import android.os.Build;

/**
 * Android implementation of store loader.
 */
public class StoreLoader implements StoreLoaderListenerCallback
{
    private static final String TAG = "StoreLoader";
    private final SplitInstallManager m_splitInstallManager;
    private HashMap<String, StoreLoaderListener> m_listeners =
            new HashMap();
    private Context m_context;
    private String m_moduleName;

    public static native void stateChangedNative(String callId, int state);
    public static native void errorOccurredNative(String callId, int errorCode,
                                                  String errorMessage);
    public static native void userConfirmationRequestedNative(String callId, int errorCode,
                                                              String errorMessage);
    public static native void downloadProgressChangedNative(String callId, long bytes, long total);
    public static native void finishedNative(String callId);

    public StoreLoader(Context context) {
        m_context = context;

        SplitCompat.install(context);

        m_splitInstallManager = SplitInstallManagerFactory.create(context);
        if (m_splitInstallManager == null)
            Log.e(TAG,"Constructor: Did not get splitInstallManager");
    }

    /**
     * Loads Feature Delivery module from a play store.
     */
    public void installModuleFromStore(String moduleName, String callId) {
        Log.d(TAG, "installModuleFromStore: " + moduleName + " " + callId);
        m_moduleName = moduleName;

        registerListener(callId);

        SplitInstallRequest request =
                SplitInstallRequest.newBuilder().addModule(m_moduleName).build();
        if (request == null)
            Log.e(TAG, "Null request");

        if (m_splitInstallManager == null)
            Log.e(TAG, "Null m_splitInstallManager");

        m_splitInstallManager.startInstall(request)
                .addOnSuccessListener(sessionId -> {
                    Log.d(TAG, "Install start call succesfull");
                    StoreLoaderListener listener = m_listeners.get(callId);
                    if (listener != null) {
                        if (!listener.isCancelled())
                            listener.setSessionId(sessionId);
                        else
                            m_splitInstallManager.cancelInstall(sessionId);
                    } else {
                        Log.d(TAG, "Listener for callId '" + callId + "' is not found");
                    }
                })
                .addOnFailureListener(exception -> {
                    int errorCode = -128;
                    String message = "splitInstall() failed.";
                    if (exception != null) {
                        message = exception.getMessage();
                        if (exception instanceof SplitInstallException)
                            errorCode = ((SplitInstallException) exception).getErrorCode();
                    }
                    onErrorOccurred(callId, errorCode, message);
                });
    }

    public void onStateChanged(String callId, int state) {
        stateChangedNative(callId, state);
        if (state == StoreLoaderListenerCallback.LOADED ||
            state == StoreLoaderListenerCallback.CANCELED) {
            finished(callId);
        }
    }

    public void onErrorOccurred(String callId, int errorCode, String errorMessage) {
        onStateChanged(callId, ERROR);

        Log.e(TAG, "Error occurred " + errorCode + " " + errorMessage);
        errorOccurredNative(callId, errorCode, errorMessage);
        finished(callId);
    }

    public void onUserConfirmationRequested(String callId, int errorCode, String errorMessage) {
        Log.d(TAG, "Requires user confirmation " + errorCode);
        userConfirmationRequestedNative(callId, errorCode, errorMessage);
    }

    public void onDownloadProgressChanged(String callId, long bytes, long total) {
        Log.d(TAG, "Downloading " + bytes + "/" + total);
        downloadProgressChangedNative(callId, bytes, total);
    }

    public void onLoadLibrary(String callId) {
        Log.d(TAG, "Load library for the module " + m_moduleName);

        stateChangedNative(callId, StoreLoaderListenerCallback.LOADING);
        // update context.
        try {
            m_context = m_context.createPackageContext(m_context.getPackageName(), 0);
        } catch (NameNotFoundException ignored) {
            Log.e(TAG, "Could not get package name");
        }
        // install splitcompat for new context.
        SplitCompat.install(m_context);
        // try to load new library
        boolean isLoaded = false;
        for (String abi : Build.SUPPORTED_ABIS) {
            String fullLibraryName = m_moduleName + "_" + abi;
            try {
                System.loadLibrary(fullLibraryName);
                isLoaded = true;
                break;
            } catch (Exception e) {
                Log.d(TAG, "Exception occurred when loading the library " + fullLibraryName + ":" +
                    e.getClass().getCanonicalName());
            }
        }

        if (isLoaded)
            onStateChanged(callId, StoreLoaderListenerCallback.LOADED);
        else
            onErrorOccurred(callId, -1, "Error loading library. Check logcat for details.");

    }

    public void cancelInstall(String callId) {
        StoreLoaderListener listener = m_listeners.get(callId);
        if (listener == null) {
            Log.e(TAG, "The listener for callId " + callId + " is not found");
            return;
        }

        if (listener.getSessionId() < 0)
            listener.postponeCancel();
        else
            m_splitInstallManager.cancelInstall(listener.getSessionId());
    }

    private void finished(String callId) {
        unregisterListener(callId);
        finishedNative(callId);
    }

    private void registerListener(String callId) {
        Log.d(TAG, "registerListener");
        StoreLoaderListener listener = new StoreLoaderListener(this, callId);
        if (listener != null) {
            m_listeners.put(callId, listener);
            m_splitInstallManager.registerListener(listener);
        }
    }

    private void unregisterListener(String callId) {
        Log.d(TAG, "unregisterListener");
        StoreLoaderListener listener = m_listeners.remove(callId);
        if (listener != null)
            m_splitInstallManager.unregisterListener(listener);
    }
}
