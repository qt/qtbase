// Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Objects;
import java.util.concurrent.Semaphore;

import android.app.Activity;
import android.app.Service;
import android.content.Context;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ApplicationInfo;
import android.content.UriPermission;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.system.Os;
import android.content.ClipboardManager;
import android.content.ClipData;
import android.content.ClipDescription;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MotionEvent;
import android.view.View;
import android.view.InputDevice;
import android.view.Display;
import android.hardware.display.DisplayManager;
import android.database.Cursor;
import android.provider.DocumentsContract;

import java.lang.reflect.Method;
import java.security.KeyStore;
import java.security.cert.X509Certificate;
import java.util.Iterator;
import java.util.List;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import android.util.Size;
import android.util.DisplayMetrics;
import android.view.WindowManager;
import android.view.WindowMetrics;
import android.graphics.Rect;

public class QtNative
{
    private static Activity m_activity = null;
    private static boolean m_activityPaused = false;
    private static Service m_service = null;
    private static QtActivityDelegate m_activityDelegate = null;
    private static QtServiceDelegate m_serviceDelegate = null;
    public static Object m_mainActivityMutex = new Object(); // mutex used to synchronize runnable operations

    public static final String QtTAG = "Qt JAVA"; // string used for Log.x
    private static ArrayList<Runnable> m_lostActions = new ArrayList<Runnable>(); // a list containing all actions which could not be performed (e.g. the main activity is destroyed, etc.)
    private static boolean m_started = false;
    private static boolean m_isKeyboardHiding = false;
    private static int m_displayMetricsScreenWidthPixels = 0;
    private static int m_displayMetricsScreenHeightPixels = 0;
    private static int m_displayMetricsAvailableLeftPixels = 0;
    private static int m_displayMetricsAvailableTopPixels = 0;
    private static int m_displayMetricsAvailableWidthPixels = 0;
    private static int m_displayMetricsAvailableHeightPixels = 0;
    private static float m_displayMetricsRefreshRate = 60;
    private static double m_displayMetricsXDpi = .0;
    private static double m_displayMetricsYDpi = .0;
    private static double m_displayMetricsScaledDensity = 1.0;
    private static double m_displayMetricsDensity = 1.0;
    private static int m_oldx, m_oldy;
    private static final int m_moveThreshold = 0;
    private static ClipboardManager m_clipboardManager = null;
    private static Method m_checkSelfPermissionMethod = null;
    private static Boolean m_tabletEventSupported = null;
    private static boolean m_usePrimaryClip = false;
    public static QtThread m_qtThread = new QtThread();
    private static final int KEYBOARD_HEIGHT_THRESHOLD = 100;

    private static final String INVALID_OR_NULL_URI_ERROR_MESSAGE = "Received invalid/null Uri";

    private static final Runnable runPendingCppRunnablesRunnable = new Runnable() {
        @Override
        public void run() {
            runPendingCppRunnables();
        }
    };

    public static boolean isStarted()
    {
        boolean hasActivity = m_activity != null && m_activityDelegate != null;
        boolean hasService = m_service != null && m_serviceDelegate != null;
        return m_started && (hasActivity || hasService);
    }

    private static ClassLoader m_classLoader = null;
    public static ClassLoader classLoader()
    {
        return m_classLoader;
    }

    public static void setClassLoader(ClassLoader classLoader)
    {
            m_classLoader = classLoader;
    }

    public static Activity activity()
    {
        synchronized (m_mainActivityMutex) {
            return m_activity;
        }
    }

    public static Service service()
    {
        synchronized (m_mainActivityMutex) {
            return m_service;
        }
    }


    public static QtActivityDelegate activityDelegate()
    {
        synchronized (m_mainActivityMutex) {
            return m_activityDelegate;
        }
    }

    public static QtServiceDelegate serviceDelegate()
    {
        synchronized (m_mainActivityMutex) {
            return m_serviceDelegate;
        }
    }

    public static String[] getStringArray(String joinedString)
    {
        return joinedString.split(",");
    }

    private static String getCurrentMethodNameLog()
    {
        return new Exception().getStackTrace()[1].getMethodName() + ": ";
    }

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
            if (scheme.compareTo("content") != 0)
                return parsedUri;

            List<UriPermission> permissions = context.getContentResolver().getPersistedUriPermissions();
            String uriStr = parsedUri.getPath();

            for (int i = 0; i < permissions.size(); ++i) {
                Uri iterUri = permissions.get(i).getUri();
                boolean isRequestPermission = permissions.get(i).isReadPermission();

                if (!openMode.equals("r"))
                   isRequestPermission = permissions.get(i).isWritePermission();

                if (iterUri.getPath().equals(uriStr) && isRequestPermission)
                    return iterUri;
            }

            // if we only have transient permissions on uri all the above will fail,
            // but we will be able to read the file anyway, so continue with uri here anyway
            // and check for SecurityExceptions later
            return parsedUri;
        } catch (SecurityException e) {
            Log.e(QtTAG, getCurrentMethodNameLog() + e.toString());
            return parsedUri;
        }
    }

    public static boolean openURL(Context context, String url, String mime)
    {
        final Uri uri = getUriWithValidPermission(context, url, "r");
        if (uri == null) {
            Log.e(QtTAG, getCurrentMethodNameLog() + INVALID_OR_NULL_URI_ERROR_MESSAGE);
            return false;
        }

        try {
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            if (!mime.isEmpty())
                intent.setDataAndType(uri, mime);

            activity().startActivity(intent);

            return true;
        } catch (Exception e) {
            Log.e(QtTAG, getCurrentMethodNameLog() + e.toString());
            return false;
        }
    }

    // this method loads full path libs
    public static void loadQtLibraries(final ArrayList<String> libraries)
    {
        m_qtThread.run(new Runnable() {
            @Override
            public void run() {
                if (libraries == null)
                    return;
                for (String libName : libraries) {
                    try {
                        File f = new File(libName);
                        if (f.exists())
                            System.load(libName);
                        else
                            Log.i(QtTAG, "Can't find '" + libName + "'");
                    } catch (SecurityException e) {
                        Log.i(QtTAG, "Can't load '" + libName + "'", e);
                    } catch (Exception e) {
                        Log.i(QtTAG, "Can't load '" + libName + "'", e);
                    }
                }
            }
        });
    }

    // this method loads bundled libs by name.
    public static void loadBundledLibraries(final ArrayList<String> libraries, final String nativeLibraryDir)
    {
        m_qtThread.run(new Runnable() {
            @Override
            public void run() {
                if (libraries == null)
                    return;

                for (String libName : libraries) {
                    try {
                        String libNameTemplate = "lib" + libName + ".so";
                        File f = new File(nativeLibraryDir + libNameTemplate);
                        if (!f.exists()) {
                            Log.i(QtTAG, "Can't find '" + f.getAbsolutePath());
                            try {
                                ApplicationInfo info = getContext().getApplicationContext().getPackageManager()
                                    .getApplicationInfo(getContext().getPackageName(), PackageManager.GET_META_DATA);
                                String systemLibraryDir = QtNativeLibrariesDir.systemLibrariesDir;
                                if (info.metaData.containsKey("android.app.system_libs_prefix"))
                                    systemLibraryDir = info.metaData.getString("android.app.system_libs_prefix");
                                f = new File(systemLibraryDir + libNameTemplate);
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                        }
                        if (f.exists())
                            System.load(f.getAbsolutePath());
                        else
                            Log.i(QtTAG, "Can't find '" + f.getAbsolutePath());
                    } catch (Exception e) {
                        Log.i(QtTAG, "Can't load '" + libName + "'", e);
                    }
                }
            }
        });
    }

    public static String loadMainLibrary(final String mainLibrary, final String nativeLibraryDir)
    {
        final String[] res = new String[1];
        res[0] = null;
        m_qtThread.run(new Runnable() {
            @Override
            public void run() {
                try {
                    String mainLibNameTemplate = "lib" + mainLibrary + ".so";
                    File f = new File(nativeLibraryDir + mainLibNameTemplate);
                    if (!f.exists()) {
                        try {
                            ApplicationInfo info = getContext().getApplicationContext().getPackageManager()
                                    .getApplicationInfo(getContext().getPackageName(), PackageManager.GET_META_DATA);
                            String systemLibraryDir = QtNativeLibrariesDir.systemLibrariesDir;
                            if (info.metaData.containsKey("android.app.system_libs_prefix"))
                                systemLibraryDir = info.metaData.getString("android.app.system_libs_prefix");
                            f = new File(systemLibraryDir + mainLibNameTemplate);
                        } catch (Exception e) {
                            e.printStackTrace();
                            return;
                        }
                    }
                    if (!f.exists())
                        return;
                    System.load(f.getAbsolutePath());
                    res[0] = f.getAbsolutePath();
                } catch (Exception e) {
                    Log.e(QtTAG, "Can't load '" + mainLibrary + "'", e);
                }
            }
        });
        return res[0];
    }

    public static void setActivity(Activity qtMainActivity, QtActivityDelegate qtActivityDelegate)
    {
        synchronized (m_mainActivityMutex) {
            m_activity = qtMainActivity;
            m_activityDelegate = qtActivityDelegate;
        }
    }

    public static void setService(Service qtMainService, QtServiceDelegate qtServiceDelegate)
    {
        synchronized (m_mainActivityMutex) {
            m_service = qtMainService;
            m_serviceDelegate = qtServiceDelegate;
        }
    }

    public static void setApplicationState(int state)
    {
        synchronized (m_mainActivityMutex) {
            switch (state) {
                case QtActivityDelegate.ApplicationActive:
                    m_activityPaused = false;
                    Iterator<Runnable> itr = m_lostActions.iterator();
                    while (itr.hasNext())
                        runAction(itr.next());
                    m_lostActions.clear();
                    break;
                default:
                    m_activityPaused = true;
                    break;
            }
        }
        updateApplicationState(state);
    }

    private static void runAction(Runnable action)
    {
        synchronized (m_mainActivityMutex) {
            final Looper mainLooper = Looper.getMainLooper();
            final Handler handler = new Handler(mainLooper);
            final boolean active = (m_activity != null && !m_activityPaused) || m_service != null;
            if (!active || mainLooper == null || !handler.post(action))
                m_lostActions.add(action);
        }
    }

    private static void runPendingCppRunnablesOnAndroidThread()
    {
        synchronized (m_mainActivityMutex) {
            if (m_activity != null) {
                if (!m_activityPaused)
                    m_activity.runOnUiThread(runPendingCppRunnablesRunnable);
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

    private static void setViewVisibility(final View view, final boolean visible)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                view.setVisibility(visible ? View.VISIBLE : View.GONE);
            }
        });
    }

    public static Display getDisplay(int displayId)
    {
        Context context = getContext();
        DisplayManager displayManager =
                (DisplayManager)context.getSystemService(Context.DISPLAY_SERVICE);
        if (displayManager != null) {
            return displayManager.getDisplay(displayId);
        }
        return null;
    }

    public static List<Display> getAvailableDisplays()
    {
        Context context = getContext();
        DisplayManager displayManager =
                (DisplayManager)context.getSystemService(Context.DISPLAY_SERVICE);
        if (displayManager != null) {
            Display[] displays = displayManager.getDisplays();
            return Arrays.asList(displays);
        }
        return new ArrayList<Display>();
    }

    public static Size getDisplaySize(Context displayContext, Display display)
    {
        if (Build.VERSION.SDK_INT < 31) {
            DisplayMetrics realMetrics = new DisplayMetrics();
            display.getRealMetrics(realMetrics);
            return new Size(realMetrics.widthPixels, realMetrics.heightPixels);
        }

        Context windowsContext = displayContext.createWindowContext(
                                                WindowManager.LayoutParams.TYPE_APPLICATION, null);
        WindowManager displayMgr =
                        (WindowManager) windowsContext.getSystemService(Context.WINDOW_SERVICE);
        WindowMetrics windowsMetrics = displayMgr.getCurrentWindowMetrics();
        Rect bounds = windowsMetrics.getBounds();
        return new Size(bounds.width(), bounds.height());
    }

    public static boolean startApplication(String params, String mainLib) throws Exception
    {
        if (params == null)
            params = "-platform\tandroid";

        final boolean[] res = new boolean[1];
        res[0] = false;
        synchronized (m_mainActivityMutex) {
            if (params.length() > 0 && !params.startsWith("\t"))
                params = "\t" + params;
            final String qtParams = mainLib + params;
            m_qtThread.run(new Runnable() {
                @Override
                public void run() {
                    res[0] = startQtAndroidPlugin(qtParams);
                    setDisplayMetrics(
                            m_displayMetricsScreenWidthPixels, m_displayMetricsScreenHeightPixels,
                            m_displayMetricsAvailableLeftPixels, m_displayMetricsAvailableTopPixels,
                            m_displayMetricsAvailableWidthPixels,
                            m_displayMetricsAvailableHeightPixels, m_displayMetricsXDpi,
                            m_displayMetricsYDpi, m_displayMetricsScaledDensity,
                            m_displayMetricsDensity, m_displayMetricsRefreshRate);
                }
            });
            m_qtThread.post(new Runnable() {
                @Override
                public void run() {
                    startQtApplication();
                }
            });
            waitForServiceSetup();
            m_started = true;
        }
        return res[0];
    }

    public static void setApplicationDisplayMetrics(int screenWidthPixels, int screenHeightPixels,
                                                    int availableLeftPixels, int availableTopPixels,
                                                    int availableWidthPixels,
                                                    int availableHeightPixels, double XDpi,
                                                    double YDpi, double scaledDensity,
                                                    double density, float refreshRate)
    {
        /* Fix buggy dpi report */
        if (XDpi < android.util.DisplayMetrics.DENSITY_LOW)
            XDpi = android.util.DisplayMetrics.DENSITY_LOW;
        if (YDpi < android.util.DisplayMetrics.DENSITY_LOW)
            YDpi = android.util.DisplayMetrics.DENSITY_LOW;

        synchronized (m_mainActivityMutex) {
            if (m_started) {
                setDisplayMetrics(screenWidthPixels, screenHeightPixels, availableLeftPixels,
                                  availableTopPixels, availableWidthPixels, availableHeightPixels,
                                  XDpi, YDpi, scaledDensity, density, refreshRate);
            } else {
                m_displayMetricsScreenWidthPixels = screenWidthPixels;
                m_displayMetricsScreenHeightPixels = screenHeightPixels;
                m_displayMetricsAvailableLeftPixels = availableLeftPixels;
                m_displayMetricsAvailableTopPixels = availableTopPixels;
                m_displayMetricsAvailableWidthPixels = availableWidthPixels;
                m_displayMetricsAvailableHeightPixels = availableHeightPixels;
                m_displayMetricsXDpi = XDpi;
                m_displayMetricsYDpi = YDpi;
                m_displayMetricsScaledDensity = scaledDensity;
                m_displayMetricsDensity = density;
                m_displayMetricsRefreshRate = refreshRate;
            }
        }
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

    public static void quitApp()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                quitQtAndroidPlugin();
                if (m_activity != null)
                     m_activity.finish();
                 if (m_service != null)
                     m_service.stopSelf();

                 m_started = false;
            }
        });
    }

    //@ANDROID-9
    static private int getAction(int index, MotionEvent event)
    {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_MOVE) {
            int hsz = event.getHistorySize();
            if (hsz > 0) {
                float x = event.getX(index);
                float y = event.getY(index);
                for (int h = 0; h < hsz; ++h) {
                    if ( event.getHistoricalX(index, h) != x ||
                         event.getHistoricalY(index, h) != y )
                        return 1;
                }
                return 2;
            }
            return 1;
        }
        if (action == MotionEvent.ACTION_DOWN
            || action == MotionEvent.ACTION_POINTER_DOWN && index == event.getActionIndex()) {
            return 0;
        } else if (action == MotionEvent.ACTION_UP
            || action == MotionEvent.ACTION_POINTER_UP && index == event.getActionIndex()) {
            return 3;
        }
        return 2;
    }
    //@ANDROID-9

    static public void sendTouchEvent(MotionEvent event, int id)
    {
        int pointerType = 0;

        if (m_tabletEventSupported == null)
            m_tabletEventSupported = isTabletEventSupported();

        switch (event.getToolType(0)) {
        case MotionEvent.TOOL_TYPE_STYLUS:
            pointerType = 1; // QTabletEvent::Pen
            break;
        case MotionEvent.TOOL_TYPE_ERASER:
            pointerType = 3; // QTabletEvent::Eraser
            break;
        }

        if (event.getToolType(0) == MotionEvent.TOOL_TYPE_MOUSE) {
            sendMouseEvent(event, id);
        } else if (m_tabletEventSupported && pointerType != 0) {
            tabletEvent(id, event.getDeviceId(), event.getEventTime(), event.getActionMasked(), pointerType,
                event.getButtonState(), event.getX(), event.getY(), event.getPressure());
        } else {
            touchBegin(id);
            for (int i = 0; i < event.getPointerCount(); ++i) {
                    touchAdd(id,
                             event.getPointerId(i),
                             getAction(i, event),
                             i == 0,
                             (int)event.getX(i),
                             (int)event.getY(i),
                             event.getTouchMajor(i),
                             event.getTouchMinor(i),
                             event.getOrientation(i),
                             event.getPressure(i));
            }

            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    touchEnd(id, 0);
                    break;

                case MotionEvent.ACTION_UP:
                    touchEnd(id, 2);
                    break;

                case MotionEvent.ACTION_CANCEL:
                    touchCancel(id);
                    break;

                default:
                    touchEnd(id, 1);
            }
        }
    }

    static public void sendTrackballEvent(MotionEvent event, int id)
    {
        sendMouseEvent(event,id);
    }

    static public boolean sendGenericMotionEvent(MotionEvent event, int id)
    {
        if (((event.getAction() & (MotionEvent.ACTION_SCROLL | MotionEvent.ACTION_HOVER_MOVE)) == 0)
                || (event.getSource() & InputDevice.SOURCE_CLASS_POINTER) != InputDevice.SOURCE_CLASS_POINTER) {
            return false;
        }

        return sendMouseEvent(event, id);
    }

    static public boolean sendMouseEvent(MotionEvent event, int id)
    {
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_UP:
                mouseUp(id, (int) event.getX(), (int) event.getY());
                break;

            case MotionEvent.ACTION_DOWN:
                mouseDown(id, (int) event.getX(), (int) event.getY());
                m_oldx = (int) event.getX();
                m_oldy = (int) event.getY();
                break;
            case MotionEvent.ACTION_HOVER_MOVE:
            case MotionEvent.ACTION_MOVE:
                if (event.getToolType(0) == MotionEvent.TOOL_TYPE_MOUSE) {
                    mouseMove(id, (int) event.getX(), (int) event.getY());
                } else {
                    int dx = (int) (event.getX() - m_oldx);
                    int dy = (int) (event.getY() - m_oldy);
                    if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                        mouseMove(id, (int) event.getX(), (int) event.getY());
                        m_oldx = (int) event.getX();
                        m_oldy = (int) event.getY();
                    }
                }
                break;
            case MotionEvent.ACTION_SCROLL:
                mouseWheel(id, (int) event.getX(), (int) event.getY(),
                        event.getAxisValue(MotionEvent.AXIS_HSCROLL), event.getAxisValue(MotionEvent.AXIS_VSCROLL));
                break;
            default:
                return false;
        }
        return true;
    }

    public static Context getContext() {
        if (m_activity != null)
            return m_activity;
        return m_service;
    }

    public static int checkSelfPermission(String permission)
    {
        int perm = PackageManager.PERMISSION_DENIED;
        synchronized (m_mainActivityMutex) {
            Context context = getContext();
            PackageManager pm = context.getPackageManager();
            perm = pm.checkPermission(permission, context.getPackageName());
        }

        return perm;
    }

    private static void updateSelection(final int selStart,
                                        final int selEnd,
                                        final int candidatesStart,
                                        final int candidatesEnd)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.updateSelection(selStart, selEnd, candidatesStart, candidatesEnd);
            }
        });
    }

    private static int getSelectHandleWidth()
    {
        return m_activityDelegate.getSelectHandleWidth();
    }

    private static void updateHandles(final int mode,
                                      final int editX,
                                      final int editY,
                                      final int editButtons,
                                      final int x1,
                                      final int y1,
                                      final int x2,
                                      final int y2,
                                      final boolean rtl)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.updateHandles(mode, editX, editY, editButtons, x1, y1, x2, y2, rtl);
            }
        });
    }

    private static void showSoftwareKeyboard(final int x,
                                             final int y,
                                             final int width,
                                             final int height,
                                             final int inputHints,
                                             final int enterKeyType)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.showSoftwareKeyboard(x, y, width, height, inputHints, enterKeyType);
            }
        });
    }

    private static void resetSoftwareKeyboard()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.resetSoftwareKeyboard();
            }
        });
    }

    private static void hideSoftwareKeyboard()
    {
        m_isKeyboardHiding = true;
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.hideSoftwareKeyboard();
            }
        });
    }

    private static void setSystemUiVisibility(final int systemUiVisibility)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.setSystemUiVisibility(systemUiVisibility);
                }
                updateWindow();
            }
        });
    }

    public static boolean isSoftwareKeyboardVisible()
    {
        return m_activityDelegate.isKeyboardVisible() && !m_isKeyboardHiding;
    }

    private static void notifyAccessibilityLocationChange(final int viewId)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyAccessibilityLocationChange(viewId);
                }
            }
        });
    }

    private static void notifyObjectHide(final int viewId, final int parentId)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyObjectHide(viewId, parentId);
                }
            }
        });
    }

    private static void notifyObjectFocus(final int viewId)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyObjectFocus(viewId);
                }
            }
        });
    }

    private static void notifyValueChanged(int viewId, String value)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyValueChanged(viewId, value);
                }
            }
        });
    }

    private static void notifyScrolledEvent(final int viewId)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyScrolledEvent(viewId);
                }
            }
        });
    }

    public static void notifyQtAndroidPluginRunning(final boolean running)
    {
        m_activityDelegate.notifyQtAndroidPluginRunning(running);
    }

    private static void registerClipboardManager()
    {
        if (m_service == null || m_activity != null) { // Avoid freezing if only service
            final Semaphore semaphore = new Semaphore(0);
            runAction(new Runnable() {
                @Override
                public void run() {
                    if (m_activity != null)
                        m_clipboardManager = (android.content.ClipboardManager) m_activity.getSystemService(Context.CLIPBOARD_SERVICE);
                    if (m_clipboardManager != null) {
                        m_clipboardManager.addPrimaryClipChangedListener(new ClipboardManager.OnPrimaryClipChangedListener() {
                            public void onPrimaryClipChanged() {
                                onClipboardDataChanged();
                            }
                        });
                    }
                    semaphore.release();
                }
            });
            try {
                semaphore.acquire();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    private static void clearClipData()
    {
        if (m_clipboardManager != null) {
            if (Build.VERSION.SDK_INT >= 28) {
                m_clipboardManager.clearPrimaryClip();
            } else {
                String[] mimeTypes = { ClipDescription.MIMETYPE_UNKNOWN };
                ClipData data = new ClipData("", mimeTypes, new ClipData.Item(new Intent()));
                m_clipboardManager.setPrimaryClip(data);
            }
        }
        m_usePrimaryClip = false;
    }
    private static void setClipboardText(String text)
    {
        if (m_clipboardManager != null) {
            ClipData clipData = ClipData.newPlainText("text/plain", text);
            updatePrimaryClip(clipData);
        }
    }

    public static boolean hasClipboardText()
    {
       return hasClipboardMimeType("text/(.*)");
    }

    private static String getClipboardText()
    {
        try {
            if (m_clipboardManager != null && m_clipboardManager.hasPrimaryClip()) {
                ClipData primaryClip = m_clipboardManager.getPrimaryClip();
                for (int i = 0; i < primaryClip.getItemCount(); ++i)
                    if (primaryClip.getItemAt(i).getText() != null)
                        return primaryClip.getItemAt(i).getText().toString();
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to get clipboard data", e);
        }
        return "";
    }

    private static void updatePrimaryClip(ClipData clipData)
    {
        try {
            if (m_usePrimaryClip) {
                ClipData clip = m_clipboardManager.getPrimaryClip();
                if (Build.VERSION.SDK_INT >= 26) {
                    Objects.requireNonNull(clip).addItem(m_activity.getContentResolver(), clipData.getItemAt(0));
                } else {
                    Objects.requireNonNull(clip).addItem(clipData.getItemAt(0));
                }
                m_clipboardManager.setPrimaryClip(clip);
            } else {
                m_clipboardManager.setPrimaryClip(clipData);
                m_usePrimaryClip = true;
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to set clipboard data", e);
        }
    }

    private static void setClipboardHtml(String text, String html)
    {
        if (m_clipboardManager != null) {
            ClipData clipData = ClipData.newHtmlText("text/html", text, html);
            updatePrimaryClip(clipData);
        }
    }

    private static boolean hasClipboardMimeType(String mimeType)
    {
        if (m_clipboardManager == null)
            return false;

        ClipDescription description = m_clipboardManager.getPrimaryClipDescription();
        // getPrimaryClipDescription can fail if the app does not have input focus
        if (description == null)
            return false;

        for (int i = 0; i < description.getMimeTypeCount(); ++i) {
            String itemMimeType = description.getMimeType(i);
            if (itemMimeType.matches(mimeType))
                return true;
        }
        return false;
    }

    public static boolean hasClipboardHtml()
    {
       return hasClipboardMimeType("text/html");
    }

    private static String getClipboardHtml()
    {
        try {
            if (m_clipboardManager != null && m_clipboardManager.hasPrimaryClip()) {
                ClipData primaryClip = m_clipboardManager.getPrimaryClip();
                for (int i = 0; i < primaryClip.getItemCount(); ++i)
                    if (primaryClip.getItemAt(i).getHtmlText() != null)
                        return primaryClip.getItemAt(i).getHtmlText().toString();
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to get clipboard data", e);
        }
        return "";
    }

    private static void setClipboardUri(String uriString)
    {
        if (m_clipboardManager != null) {
            ClipData clipData = ClipData.newUri(m_activity.getContentResolver(), "text/uri-list",
                                                Uri.parse(uriString));
            updatePrimaryClip(clipData);
        }
    }

    public static boolean hasClipboardUri()
    {
       return hasClipboardMimeType("text/uri-list");
    }

    private static String[] getClipboardUris()
    {
        ArrayList<String> uris = new ArrayList<String>();
        try {
            if (m_clipboardManager != null && m_clipboardManager.hasPrimaryClip()) {
                ClipData primaryClip = m_clipboardManager.getPrimaryClip();
                for (int i = 0; i < primaryClip.getItemCount(); ++i)
                    if (primaryClip.getItemAt(i).getUri() != null)
                        uris.add(primaryClip.getItemAt(i).getUri().toString());
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to get clipboard data", e);
        }
        String[] strings = new String[uris.size()];
        strings = uris.toArray(strings);
        return strings;
    }

    private static void openContextMenu(final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.openContextMenu(x, y, w, h);
            }
        });
    }

    private static void closeContextMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.closeContextMenu();
            }
        });
    }

    private static void resetOptionsMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.resetOptionsMenu();
            }
        });
    }

    private static void openOptionsMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activity != null)
                    m_activity.openOptionsMenu();
            }
        });
    }

    private static byte[][] getSSLCertificates()
    {
        ArrayList<byte[]> certificateList = new ArrayList<byte[]>();

        try {
            TrustManagerFactory factory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            factory.init((KeyStore) null);

            for (TrustManager manager : factory.getTrustManagers()) {
                if (manager instanceof X509TrustManager) {
                    X509TrustManager trustManager = (X509TrustManager) manager;

                    for (X509Certificate certificate : trustManager.getAcceptedIssuers()) {
                        byte buffer[] = certificate.getEncoded();
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

    private static void createSurface(final int id, final boolean onTop, final int x, final int y, final int w, final int h, final int imageDepth)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.createSurface(id, onTop, x, y, w, h, imageDepth);
            }
        });
    }

    private static void insertNativeView(final int id, final View view, final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.insertNativeView(id, view, x, y, w, h);
            }
        });
    }

    private static void setSurfaceGeometry(final int id, final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.setSurfaceGeometry(id, x, y, w, h);
            }
        });
    }

    private static void bringChildToFront(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.bringChildToFront(id);
            }
        });
    }

    private static void bringChildToBack(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.bringChildToBack(id);
            }
        });
    }

    private static void destroySurface(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.destroySurface(id);
            }
        });
    }

    private static void initializeAccessibility()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                m_activityDelegate.initializeAccessibility();
            }
        });
    }

    private static void hideSplashScreen(final int duration)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.hideSplashScreen(duration);
            }
        });
    }

    public static void keyboardVisibilityUpdated(boolean visibility)
    {
        m_isKeyboardHiding = false;
        keyboardVisibilityChanged(visibility);
    }

    private static String[] listAssetContent(android.content.res.AssetManager asset, String path) {
        String [] list;
        ArrayList<String> res = new ArrayList<String>();
        try {
            list = asset.list(path);
            if (list.length > 0) {
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
        return res.toArray(new String[res.size()]);
    }

    /**
     *Sets a single environment variable
     *
     * returns true if the value was set, false otherwise.
     * in case it cannot set value will log the exception
     **/
    public static void setEnvironmentVariable(String key, String value)
    {
        try {
            android.system.Os.setenv(key, value, true);
        } catch (Exception e) {
            Log.e(QtNative.QtTAG, "Could not set environment variable:" + key + "=" + value);
            e.printStackTrace();
        }
    }

    /**
     *Sets multiple environment variables
     *
     * Uses '\t' as divider between variables and '=' between key/value
     * Ex: key1=val1\tkey2=val2\tkey3=val3
     * Note: it assumed that the key cannot have '=' but the value can
     **/
    public static void setEnvironmentVariables(String environmentVariables)
    {
        for (String variable : environmentVariables.split("\t")) {
            String[] keyvalue = variable.split("=", 2);
            if (keyvalue.length < 2 || keyvalue[0].isEmpty())
                continue;

            setEnvironmentVariable(keyvalue[0], keyvalue[1]);
        }
    }

    // screen methods
    public static native void setDisplayMetrics(int screenWidthPixels, int screenHeightPixels,
                                                int availableLeftPixels, int availableTopPixels,
                                                int availableWidthPixels, int availableHeightPixels,
                                                double XDpi, double YDpi, double scaledDensity,
                                                double density, float refreshRate);
    public static native void handleOrientationChanged(int newRotation, int nativeOrientation);
    public static native void handleRefreshRateChanged(float refreshRate);
    public static native void handleScreenAdded(int displayId);
    public static native void handleScreenChanged(int displayId);
    public static native void handleScreenRemoved(int displayId);
    // screen methods
    public static native void handleUiDarkModeChanged(int newUiMode);

    // pointer methods
    public static native void mouseDown(int winId, int x, int y);
    public static native void mouseUp(int winId, int x, int y);
    public static native void mouseMove(int winId, int x, int y);
    public static native void mouseWheel(int winId, int x, int y, float hdelta, float vdelta);
    public static native void touchBegin(int winId);
    public static native void touchAdd(int winId, int pointerId, int action, boolean primary, int x, int y, float major, float minor, float rotation, float pressure);
    public static native void touchEnd(int winId, int action);
    public static native void touchCancel(int winId);
    public static native void longPress(int winId, int x, int y);
    // pointer methods

    // tablet methods
    public static native boolean isTabletEventSupported();
    public static native void tabletEvent(int winId, int deviceId, long time, int action, int pointerType, int buttonState, float x, float y, float pressure);
    // tablet methods

    // keyboard methods
    public static native void keyDown(int key, int unicode, int modifier, boolean autoRepeat);
    public static native void keyUp(int key, int unicode, int modifier, boolean autoRepeat);
    public static native void keyboardVisibilityChanged(boolean visibility);
    public static native void keyboardGeometryChanged(int x, int y, int width, int height);
    // keyboard methods

    // handle methods
    public static final int IdCursorHandle = 1;
    public static final int IdLeftHandle = 2;
    public static final int IdRightHandle = 3;
    public static native void handleLocationChanged(int id, int x, int y);
    // handle methods

    // dispatch events methods
    public static native boolean dispatchGenericMotionEvent(MotionEvent ev);
    public static native boolean dispatchKeyEvent(KeyEvent event);
    // dispatch events methods

    // surface methods
    public static native void setSurface(int id, Object surface, int w, int h);
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

    // clipboard methods
    public static native void onClipboardDataChanged();
    // clipboard methods

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
