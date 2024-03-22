// Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
import android.util.Log;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.View;

import java.lang.ref.WeakReference;
import java.security.KeyStore;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

public class QtNative
{
    private static WeakReference<Activity> m_activity = null;
    private static WeakReference<Service> m_service = null;
    public static final Object m_mainActivityMutex = new Object(); // mutex used to synchronize runnable operations

    private static final ApplicationStateDetails m_stateDetails = new ApplicationStateDetails();

    public static final String QtTAG = "Qt JAVA";

    // a list containing all actions which could not be performed (e.g. the main activity is destroyed, etc.)
    private static final ArrayList<Runnable> m_lostActions = new ArrayList<>();

    private static final QtThread m_qtThread = new QtThread();
    private static ClassLoader m_classLoader = null;

    private static final Runnable runPendingCppRunnablesRunnable = QtNative::runPendingCppRunnables;
    private static final ArrayList<AppStateDetailsListener> m_appStateListeners = new ArrayList<>();
    private static final Object m_appStateListenersLock = new Object();

    @UsedFromNativeCode
    public static ClassLoader classLoader()
    {
        return m_classLoader;
    }

    public static void setClassLoader(ClassLoader classLoader)
    {
            m_classLoader = classLoader;
    }

    public static void setActivity(Activity qtMainActivity)
    {
        synchronized (m_mainActivityMutex) {
            m_activity = new WeakReference<>(qtMainActivity);
        }
    }

    public static void setService(Service qtMainService)
    {
        synchronized (m_mainActivityMutex) {
            m_service = new WeakReference<>(qtMainService);
        }
    }

    @UsedFromNativeCode
    public static Activity activity()
    {
        synchronized (m_mainActivityMutex) {
            return m_activity != null ? m_activity.get() : null;
        }
    }

    public static boolean isActivityValid()
    {
        return m_activity != null && m_activity.get() != null;
    }

    @UsedFromNativeCode
    public static Service service()
    {
        synchronized (m_mainActivityMutex) {
            return m_service != null ? m_service.get() : null;
        }
    }

    public static boolean isServiceValid()
    {
        return m_service != null && m_service.get() != null;
    }

    @UsedFromNativeCode
    public static Context getContext() {
        if (isActivityValid())
            return m_activity.get();
        return service();
    }

    @UsedFromNativeCode
    public static String[] getStringArray(String joinedString)
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
    public static boolean openURL(Context context, String url, String mime)
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
        return m_qtThread;
    }

    interface AppStateDetailsListener {
        void onAppStateDetailsChanged(ApplicationStateDetails details);
    }

    // Keep in sync with src/corelib/global/qnamespace.h
    public static class ApplicationState {
        static final int ApplicationSuspended = 0x0;
        static final int ApplicationHidden = 0x1;
        static final int ApplicationInactive = 0x2;
        static final int ApplicationActive = 0x4;
    }

    public static class ApplicationStateDetails {
        int state = ApplicationState.ApplicationSuspended;
        boolean nativePluginIntegrationReady = false;
        boolean isStarted = false;
    }

    public static ApplicationStateDetails getStateDetails()
    {
        return m_stateDetails;
    }

    public static void setStarted(boolean started)
    {
        m_stateDetails.isStarted = started;
        notifyAppStateDetailsChanged(m_stateDetails);
    }

    @UsedFromNativeCode
    public static void notifyNativePluginIntegrationReady(boolean ready)
    {
        m_stateDetails.nativePluginIntegrationReady = ready;
        notifyAppStateDetailsChanged(m_stateDetails);
    }

    public static void setApplicationState(int state)
    {
        synchronized (m_mainActivityMutex) {
            m_stateDetails.state = state;
            if (state == ApplicationState.ApplicationActive) {
                for (Runnable mLostAction : m_lostActions)
                    runAction(mLostAction);
                m_lostActions.clear();
            }
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

    static void notifyAppStateDetailsChanged(ApplicationStateDetails details) {
        synchronized (m_appStateListenersLock) {
            for (AppStateDetailsListener listener : m_appStateListeners)
                listener.onAppStateDetailsChanged(details);
        }
    }

    // Post a runnable to Main (UI) Thread if the app is active,
    // otherwise, queue it to be posted when the the app is active again
    public static void runAction(Runnable action)
    {
        runAction(action, true);
    }

    public static void runAction(Runnable action, boolean queueWhenInactive)
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
                    m_lostActions.add(action);
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

    public static void startApplication(String params, String mainLib)
    {
        synchronized (m_mainActivityMutex) {
            m_qtThread.run(() -> {
                final String qtParams = mainLib + " " + params;
                if (!startQtAndroidPlugin(qtParams))
                    Log.e(QtTAG, "An error occurred while starting the Qt Android plugin");
            });
            m_qtThread.post(QtNative::startQtApplication);
            waitForServiceSetup();
            m_stateDetails.isStarted = true;
            notifyAppStateDetailsChanged(m_stateDetails);
        }
    }

    public static void quitApp()
    {
        runAction(() -> {
            quitQtAndroidPlugin();
            if (isActivityValid())
                m_activity.get().finish();
            if (isServiceValid())
                m_service.get().stopSelf();
            m_stateDetails.isStarted = false;
            // Likely no use to call notifyAppStateDetailsChanged at this point since we are exiting
        });
    }

    @UsedFromNativeCode
    public static int checkSelfPermission(String permission)
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
                        String[] isDir = asset.list(path.length() > 0 ? path + "/" + file : file);
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
    public static native boolean startQtAndroidPlugin(String params);
    public static native void startQtApplication();
    public static native void waitForServiceSetup();
    public static native void quitQtCoreApplication();
    public static native void quitQtAndroidPlugin();
    public static native void terminateQt();
    public static native boolean updateNativeActivity();
    // application methods

    // surface methods
    public static native void setSurface(int id, Object surface);
    // surface methods

    // window methods
    public static native void updateWindow();
    // window methods

    // application methods
    public static native void updateApplicationState(int state);

    // menu methods
    public static native boolean onPrepareOptionsMenu(Menu menu);
    public static native boolean onOptionsItemSelected(int itemId, boolean checked);
    public static native void onOptionsMenuClosed(Menu menu);

    public static native void onCreateContextMenu(ContextMenu menu);
    public static native void fillContextMenu(Menu menu);
    public static native boolean onContextItemSelected(int itemId, boolean checked);
    public static native void onContextMenuClosed(Menu menu);
    // menu methods

    // activity methods
    public static native void onActivityResult(int requestCode, int resultCode, Intent data);
    public static native void onNewIntent(Intent data);

    public static native void runPendingCppRunnables();

    public static native void sendRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults);
    // activity methods

    // service methods
    public static native IBinder onBind(Intent intent);
    // service methods
}
