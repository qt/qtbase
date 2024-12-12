// Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

package org.qtproject.qt.android;

import android.app.Activity;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.UriPermission;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.system.Os;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.View;

import java.lang.ref.WeakReference;
import java.lang.UnsatisfiedLinkError;
import java.security.KeyStore;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

// ### Qt7: make private and find new API for onNewIntent()
public class QtNative
{
    private static WeakReference<Activity> m_activity = null;
    private static WeakReference<Service> m_service = null;
    private static final Object m_mainActivityMutex = new Object(); // mutex used to synchronize runnable operations

    private static final ApplicationStateDetails m_stateDetails = new ApplicationStateDetails();

    static final String QtTAG = "Qt JAVA";

    // a list of all actions which could not be performed (e.g. the main activity is destroyed, etc.)
    private static final BackgroundActionsTracker m_backgroundActionsTracker = new BackgroundActionsTracker();
    private static QtThread m_qtThread = null;
    private static final Object m_qtThreadLock = new Object();
    private static ClassLoader m_classLoader = null;

    private static final Runnable runPendingCppRunnablesRunnable = QtNative::runPendingCppRunnables;
    private static final ArrayList<AppStateDetailsListener> m_appStateListeners = new ArrayList<>();
    private static final Object m_appStateListenersLock = new Object();

    @UsedFromNativeCode
    static ClassLoader classLoader()
    {
        return m_classLoader;
    }

    static void setClassLoader(ClassLoader classLoader)
    {
            m_classLoader = classLoader;
    }

    static void setActivity(Activity qtMainActivity)
    {
        synchronized (m_mainActivityMutex) {
            m_activity = new WeakReference<>(qtMainActivity);
            try {
                if (m_stateDetails.isStarted)
                    updateNativeActivity();
            } catch (UnsatisfiedLinkError ignored) {
                // No-op - this happens in certain e.g. QtQuick for Android cases when we set the
                // Activity for the first time, before Qt native libraries have been loaded. The
                // C++ side will update its reference when the library is loaded.
            }
        }
    }

    static void setService(Service qtMainService)
    {
        synchronized (m_mainActivityMutex) {
            m_service = new WeakReference<>(qtMainService);
        }
    }

    @UsedFromNativeCode
    static Activity activity()
    {
        synchronized (m_mainActivityMutex) {
            return m_activity != null ? m_activity.get() : null;
        }
    }

    static boolean isActivityValid()
    {
        return m_activity != null && m_activity.get() != null;
    }

    @UsedFromNativeCode
    static Service service()
    {
        synchronized (m_mainActivityMutex) {
            return m_service != null ? m_service.get() : null;
        }
    }

    static boolean isServiceValid()
    {
        return m_service != null && m_service.get() != null;
    }

    @UsedFromNativeCode
    static Context getContext() {
        if (isActivityValid())
            return m_activity.get();
        return service();
    }

    @UsedFromNativeCode
    static String[] getStringArray(String joinedString)
    {
        return joinedString.split(",");
    }

    private static String getCurrentMethodNameLog()
    {
        return new Exception().getStackTrace()[1].getMethodName() + ": ";
    }

    /** @noinspection SameParameterValue*/
    private static Uri getUriWithValidPermission(Context context, String uri, String openMode)
    {
        Uri parsedUri;
        try {
            parsedUri = Uri.parse(uri);
        } catch (NullPointerException e) {
            e.printStackTrace();
            return null;
        }

        try {
            String scheme = parsedUri.getScheme();

            // We only want to check permissions for content Uris
            if (scheme != null && scheme.compareTo("content") != 0)
                return parsedUri;

            List<UriPermission> permissions = context.getContentResolver().getPersistedUriPermissions();
            String uriStr = parsedUri.getPath();

            for (int i = 0; i < permissions.size(); ++i) {
                Uri iterUri = permissions.get(i).getUri();
                boolean isRequestPermission = permissions.get(i).isReadPermission();

                if (!openMode.equals("r"))
                   isRequestPermission = permissions.get(i).isWritePermission();

                if (Objects.equals(iterUri.getPath(), uriStr) && isRequestPermission)
                    return iterUri;
            }

            // if we only have transient permissions on uri all the above will fail,
            // but we will be able to read the file anyway, so continue with uri here anyway
            // and check for SecurityExceptions later
            return parsedUri;
        } catch (SecurityException e) {
            Log.e(QtTAG, getCurrentMethodNameLog() + e);
            return parsedUri;
        }
    }

    @UsedFromNativeCode
    static boolean openURL(Context context, String url, String mime)
    {
        final Uri uri = getUriWithValidPermission(context, url, "r");
        if (uri == null) {
            Log.e(QtTAG, getCurrentMethodNameLog() + "received invalid/null Uri");
            return false;
        }

        try {
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            if (!mime.isEmpty())
                intent.setDataAndType(uri, mime);

            Activity activity = activity();
            if (activity == null) {
                Log.w(QtTAG, "openURL(): The activity reference is null");
                return false;
            }

            activity.startActivity(intent);

            return true;
        } catch (Exception e) {
            Log.e(QtTAG, getCurrentMethodNameLog() + e);
            return false;
        }
    }

    static QtThread getQtThread() {
        if (m_qtThread != null)
            return m_qtThread;

        synchronized (m_qtThreadLock) {
            if (m_qtThread == null)
                m_qtThread = new QtThread();

            return m_qtThread;
        }
    }

    interface AppStateDetailsListener {
        default void onAppStateDetailsChanged(ApplicationStateDetails details) {}
        default void onNativePluginIntegrationReadyChanged(boolean ready) {}
    }

    // Keep in sync with src/corelib/global/qnamespace.h
    static class ApplicationState {
        static final int ApplicationSuspended = 0x0;
        static final int ApplicationHidden = 0x1;
        static final int ApplicationInactive = 0x2;
        static final int ApplicationActive = 0x4;
    }

    static class ApplicationStateDetails {
        int state = ApplicationState.ApplicationSuspended;
        boolean nativePluginIntegrationReady = false;
        boolean isStarted = false;
    }

    static ApplicationStateDetails getStateDetails()
    {
        return m_stateDetails;
    }

    static void setStarted(boolean started)
    {
        m_stateDetails.isStarted = started;
        notifyAppStateDetailsChanged(m_stateDetails);
    }

    @UsedFromNativeCode
    static void notifyNativePluginIntegrationReady(boolean ready)
    {
        m_stateDetails.nativePluginIntegrationReady = ready;
        notifyNativePluginIntegrationReadyChanged(ready);
        notifyAppStateDetailsChanged(m_stateDetails);

        // Only set queue size when the plugin is fully loaded.
        final String bufferSize = Os.getenv("QT_ANDROID_BACKGROUND_ACTIONS_QUEUE_SIZE");
        if (bufferSize != null) {
            try {
                final int size = Integer.parseInt(bufferSize);
                m_backgroundActionsTracker.setMaxAllowedActions(size);
            } catch (NumberFormatException exception) {
                Log.e(QtTAG, "Parsing failed, QT_ANDROID_BACKGROUND_ACTIONS_QUEUE_SIZE value is not an integer");
            }
        }
    }

    static void setApplicationState(int state)
    {
        synchronized (m_mainActivityMutex) {
            m_stateDetails.state = state;
            if (state == ApplicationState.ApplicationActive)
                m_backgroundActionsTracker.processActions();
        }
        updateApplicationState(state);
        notifyAppStateDetailsChanged(m_stateDetails);
    }

    static void registerAppStateListener(AppStateDetailsListener listener) {
        synchronized (m_appStateListenersLock) {
            if (!m_appStateListeners.contains(listener))
                m_appStateListeners.add(listener);
        }
    }

    static void unregisterAppStateListener(AppStateDetailsListener listener) {
        synchronized (m_appStateListenersLock) {
            m_appStateListeners.remove(listener);
        }
    }

    static void notifyNativePluginIntegrationReadyChanged(boolean ready) {
        synchronized (m_appStateListenersLock) {
            for (final AppStateDetailsListener listener : m_appStateListeners)
                listener.onNativePluginIntegrationReadyChanged(ready);
        }
    }

    static void notifyAppStateDetailsChanged(ApplicationStateDetails details) {
        synchronized (m_appStateListenersLock) {
            for (AppStateDetailsListener listener : m_appStateListeners)
                listener.onAppStateDetailsChanged(details);
        }
    }

    // Post a runnable to Main (UI) Thread if the app is active,
    // otherwise, queue it to be posted when the the app is active again
    static void runAction(Runnable action)
    {
        runAction(action, true);
    }

    static void runAction(Runnable action, boolean queueWhenInactive)
    {
        synchronized (m_mainActivityMutex) {
            final Looper mainLooper = Looper.getMainLooper();
            final Handler handler = new Handler(mainLooper);

            if (queueWhenInactive) {
                final boolean isStateVisible =
                        (m_stateDetails.state != ApplicationState.ApplicationSuspended)
                        && (m_stateDetails.state != ApplicationState.ApplicationHidden);
                final boolean active = (isActivityValid() && isStateVisible) || isServiceValid();
                if (!active || !handler.post(action))
                    m_backgroundActionsTracker.enqueue(action);
            } else {
                handler.post(action);
            }
        }
    }

    @UsedFromNativeCode
    private static void runPendingCppRunnablesOnAndroidThread()
    {
        synchronized (m_mainActivityMutex) {
            if (isActivityValid()) {
                if (m_stateDetails.state == ApplicationState.ApplicationActive)
                    m_activity.get().runOnUiThread(runPendingCppRunnablesRunnable);
                else
                    runAction(runPendingCppRunnablesRunnable);
            } else {
                final Looper mainLooper = Looper.getMainLooper();
                final Thread looperThread = mainLooper.getThread();
                if (looperThread.equals(Thread.currentThread())) {
                    runPendingCppRunnablesRunnable.run();
                } else {
                    final Handler handler = new Handler(mainLooper);
                    handler.post(runPendingCppRunnablesRunnable);
                }
            }
        }
    }

    @UsedFromNativeCode
    private static void setViewVisibility(final View view, final boolean visible)
    {
        runAction(() -> view.setVisibility(visible ? View.VISIBLE : View.GONE));
    }

    static void startApplication(String params, String mainLib)
    {
        if (m_stateDetails.isStarted)
            return;

        QtThread thread = getQtThread();
        thread.run(() -> {
            final String qtParams = mainLib + " " + params;
            if (!startQtAndroidPlugin(qtParams))
                Log.e(QtTAG, "An error occurred while starting the Qt Android plugin");
        });
        thread.post(QtNative::startQtApplication);
        waitForServiceSetup();
        m_stateDetails.isStarted = true;
        notifyAppStateDetailsChanged(m_stateDetails);
    }

    static void quitApp()
    {
        runAction(() -> {
            quitQtAndroidPlugin();
            if (isActivityValid())
                m_activity.get().finish();
            if (isServiceValid())
                m_service.get().stopSelf();
            m_stateDetails.isStarted = false;
            notifyAppStateDetailsChanged(m_stateDetails);
        });
    }

    static void quitQt()
    {
        runAction(() -> {
            terminateQt();
            m_stateDetails.isStarted = false;
            notifyAppStateDetailsChanged(m_stateDetails);
            getQtThread().exit();
            synchronized (m_qtThreadLock) {
                m_qtThread = null;
            }
        });
    }

    @UsedFromNativeCode
    static int checkSelfPermission(String permission)
    {
        synchronized (m_mainActivityMutex) {
            Context context = getContext();
            PackageManager pm = context.getPackageManager();
            return pm.checkPermission(permission, context.getPackageName());
        }
    }

    @UsedFromNativeCode
    private static byte[][] getSSLCertificates()
    {
        ArrayList<byte[]> certificateList = new ArrayList<>();

        try {
            TrustManagerFactory factory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            factory.init((KeyStore) null);

            for (TrustManager manager : factory.getTrustManagers()) {
                if (manager instanceof X509TrustManager) {
                    X509TrustManager trustManager = (X509TrustManager) manager;

                    for (X509Certificate certificate : trustManager.getAcceptedIssuers()) {
                        byte[] buffer = certificate.getEncoded();
                        certificateList.add(buffer);
                    }
                }
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to get certificates", e);
        }

        byte[][] certificateArray = new byte[certificateList.size()][];
        certificateArray = certificateList.toArray(certificateArray);
        return certificateArray;
    }

    @UsedFromNativeCode
    private static String[] listAssetContent(android.content.res.AssetManager asset, String path) {
        String [] list;
        ArrayList<String> res = new ArrayList<>();
        try {
            list = asset.list(path);
            if (list != null) {
                for (String file : list) {
                    try {
                        String[] isDir = asset.list(!path.isEmpty() ? path + "/" + file : file);
                        if (isDir != null && isDir.length > 0)
                            file += "/";
                        res.add(file);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return res.toArray(new String[0]);
    }

    // application methods
    static native boolean startQtAndroidPlugin(String params);
    static native void startQtApplication();
    static native void waitForServiceSetup();
    static native void quitQtCoreApplication();
    static native void quitQtAndroidPlugin();
    static native void terminateQt();
    static native boolean updateNativeActivity();
    // application methods

    // application methods
    static native void updateApplicationState(int state);
    static native void updateLocale();

    // menu methods
    static native boolean onPrepareOptionsMenu(Menu menu);
    static native boolean onOptionsItemSelected(int itemId, boolean checked);
    static native void onOptionsMenuClosed(Menu menu);

    static native void onCreateContextMenu(ContextMenu menu);
    static native void fillContextMenu(Menu menu);
    static native boolean onContextItemSelected(int itemId, boolean checked);
    static native void onContextMenuClosed(Menu menu);
    // menu methods

    // activity methods
    static native void onActivityResult(int requestCode, int resultCode, Intent data);
    public static native void onNewIntent(Intent data);

    static native void runPendingCppRunnables();

    static native void sendRequestPermissionsResult(int requestCode, int[] grantResults);
    // activity methods

    // service methods
    static native IBinder onBind(Intent intent);
    // service methods
}
