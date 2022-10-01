// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2019, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ComponentInfo;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import dalvik.system.DexClassLoader;

import static org.qtproject.qt.android.QtConstants.*;

public abstract class QtLoader {

    static String QtTAG = "Qt";

    // These parameters matter in case of deploying application as system (embedded into firmware)
    public static final String SYSTEM_LIB_PATH = "/system/lib/";

    public String APPLICATION_PARAMETERS = null; // use this variable to pass any parameters to your application,
    // the parameters must not contain any white spaces
    // and must be separated with "\t"
    // e.g "-param1\t-param2=value2\t-param3\tvalue3"

    public String ENVIRONMENT_VARIABLES = "QT_USE_ANDROID_NATIVE_DIALOGS=1";
    // use this variable to add any environment variables to your application.
    // the env vars must be separated with "\t"
    // e.g. "ENV_VAR1=1\tENV_VAR2=2\t"
    // Currently the following vars are used by the android plugin:
    // * QT_USE_ANDROID_NATIVE_DIALOGS -1 to use the android native dialogs.

    public String[] QT_ANDROID_THEMES = null;     // A list with all themes that your application want to use.
    // The name of the theme must be the same with any theme from
    // http://developer.android.com/reference/android/R.style.html
    // The most used themes are:
    //  * "Theme" - (fallback) check http://developer.android.com/reference/android/R.style.html#Theme
    //  * "Theme_Black" - check http://developer.android.com/reference/android/R.style.html#Theme_Black
    //  * "Theme_Light" - (default for API <=10) check http://developer.android.com/reference/android/R.style.html#Theme_Light
    //  * "Theme_Holo" - check http://developer.android.com/reference/android/R.style.html#Theme_Holo
    //  * "Theme_Holo_Light" - (default for API 11-13) check http://developer.android.com/reference/android/R.style.html#Theme_Holo_Light
    //  * "Theme_DeviceDefault" - check http://developer.android.com/reference/android/R.style.html#Theme_DeviceDefault
    //  * "Theme_DeviceDefault_Light" - (default for API 14+) check http://developer.android.com/reference/android/R.style.html#Theme_DeviceDefault_Light

    public String QT_ANDROID_DEFAULT_THEME = null; // sets the default theme.

    public ArrayList<String> m_qtLibs = null; // required qt libs
    public int m_displayDensity = -1;
    private ContextWrapper m_context;
    protected ComponentInfo m_contextInfo;
    private Class<?> m_delegateClass;

    private static ArrayList<FileOutputStream> m_fileOutputStreams = new ArrayList<FileOutputStream>();
    // List of open file streams associated with files copied during installation.

    public static Object m_delegateObject = null;
    public static HashMap<String, ArrayList<Method>> m_delegateMethods= new HashMap<String, ArrayList<Method>>();
    public static Method dispatchKeyEvent = null;
    public static Method dispatchPopulateAccessibilityEvent = null;
    public static Method dispatchTouchEvent = null;
    public static Method dispatchTrackballEvent = null;
    public static Method onKeyDown = null;
    public static Method onKeyMultiple = null;
    public static Method onKeyUp = null;
    public static Method onTouchEvent = null;
    public static Method onTrackballEvent = null;
    public static Method onActivityResult = null;
    public static Method onCreate = null;
    public static Method onKeyLongPress = null;
    public static Method dispatchKeyShortcutEvent = null;
    public static Method onKeyShortcut = null;
    public static Method dispatchGenericMotionEvent = null;
    public static Method onGenericMotionEvent = null;
    public static Method onRequestPermissionsResult = null;
    private static String activityClassName;
    private static Class qtApplicationClass = null;

    public QtLoader(ContextWrapper context, Class<?> clazz) {
        m_context = context;
        m_delegateClass = clazz;
    }

    public static void setQtApplicationClass(Class qtAppClass)
    {
        qtApplicationClass = qtAppClass;
    }

    public static void setQtTAG(String tag)
    {
        QtTAG = tag;
    }

    public static void setQtContextDelegate(Class<?> clazz, Object listener)
    {
        m_delegateObject = listener;
        activityClassName = clazz.getCanonicalName();

        ArrayList<Method> delegateMethods = new ArrayList<Method>();
        for (Method m : listener.getClass().getMethods()) {
            if (m.getDeclaringClass().getName().startsWith("org.qtproject.qt.android"))
                delegateMethods.add(m);
        }

        ArrayList<Field> applicationFields = new ArrayList<Field>();
        for (Field f : qtApplicationClass.getFields()) {
            if (f.getDeclaringClass().getName().equals(qtApplicationClass.getName()))
                applicationFields.add(f);
        }

        for (Method delegateMethod : delegateMethods) {
            try {
                clazz.getDeclaredMethod(delegateMethod.getName(), delegateMethod.getParameterTypes());
                if (m_delegateMethods.containsKey(delegateMethod.getName())) {
                    m_delegateMethods.get(delegateMethod.getName()).add(delegateMethod);
                } else {
                    ArrayList<Method> delegateSet = new ArrayList<Method>();
                    delegateSet.add(delegateMethod);
                    m_delegateMethods.put(delegateMethod.getName(), delegateSet);
                }
                for (Field applicationField:applicationFields) {
                    if (applicationField.getName().equals(delegateMethod.getName())) {
                        try {
                            applicationField.set(null, delegateMethod);
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                }
            } catch (Exception e) { }
        }
    }

    public static class InvokeResult
    {
        public boolean invoked = false;
        public Object methodReturns = null;
    }

    private static int stackDeep=-1;
    public static InvokeResult invokeDelegate(Object... args)
    {
        InvokeResult result = new InvokeResult();
        if (m_delegateObject == null)
            return result;
        StackTraceElement[] elements = Thread.currentThread().getStackTrace();
        if (-1 == stackDeep) {
            for (int it=0;it<elements.length;it++)
                if (elements[it].getClassName().equals(activityClassName)) {
                    stackDeep = it;
                    break;
                }
        }
        if (-1 == stackDeep)
            return result;

        final String methodName=elements[stackDeep].getMethodName();
        if (!m_delegateMethods.containsKey(methodName))
            return result;

        for (Method m : m_delegateMethods.get(methodName)) {
            if (m.getParameterTypes().length == args.length) {
                result.methodReturns = invokeDelegateMethod(m, args);
                result.invoked = true;
                return result;
            }
        }
        return result;
    }

    public static Object invokeDelegateMethod(Method m, Object... args)
    {
        try {
            return m.invoke(m_delegateObject, args);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    // Implement in subclass
    protected void finish() {}

    protected String getTitle() {
        return "Qt";
    }

    protected void runOnUiThread(Runnable run) {
        run.run();
    }

    protected abstract String loaderClassName();
    protected abstract Class<?> contextClassName();

    Intent getIntent()
    {
        return null;
    }
    // Implement in subclass

    private final List<String> supportedAbis = Arrays.asList(Build.SUPPORTED_ABIS);
    private String preferredAbi = null;

    private ArrayList<String> prefferedAbiLibs(String []libs)
    {
        HashMap<String, ArrayList<String>> abisLibs = new HashMap<>();
        for (String lib : libs) {
            String[] archLib = lib.split(";", 2);
            if (preferredAbi != null && !archLib[0].equals(preferredAbi))
                continue;
            if (!abisLibs.containsKey(archLib[0]))
                abisLibs.put(archLib[0], new ArrayList<String>());
            abisLibs.get(archLib[0]).add(archLib[1]);
        }

        if (preferredAbi != null) {
            if (abisLibs.containsKey(preferredAbi)) {
                return abisLibs.get(preferredAbi);
            }
            return new ArrayList<String>();
        }

        for (String abi: supportedAbis) {
            if (abisLibs.containsKey(abi)) {
                preferredAbi = abi;
                return abisLibs.get(abi);
            }
        }
        return new ArrayList<String>();
    }

    // this function is used to load and start the loader
    private void loadApplication(Bundle loaderParams)
    {
        final Resources resources = m_context.getResources();
        final String packageName = m_context.getPackageName();
        try {
            final int errorCode = loaderParams.getInt(ERROR_CODE_KEY);
            if (errorCode != 0) {
                // fatal error, show the error and quit
                AlertDialog errorDialog = new AlertDialog.Builder(m_context).create();
                errorDialog.setMessage(loaderParams.getString(ERROR_MESSAGE_KEY));
                errorDialog.setButton(Dialog.BUTTON_POSITIVE,
                        resources.getString(android.R.string.ok),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                finish();
                            }
                        });
                errorDialog.show();
                return;
            }

            // add all bundled Qt libs to loader params
            int id = resources.getIdentifier("bundled_libs", "array", packageName);
            final String[] bundledLibs = resources.getStringArray(id);
            ArrayList<String> libs = new ArrayList<>(prefferedAbiLibs(bundledLibs));

            String libName = null;
            if (m_contextInfo.metaData.containsKey("android.app.lib_name")) {
                libName = m_contextInfo.metaData.getString("android.app.lib_name") + "_" + preferredAbi;
                loaderParams.putString(MAIN_LIBRARY_KEY, libName); //main library contains main() function
            }

            loaderParams.putStringArrayList(BUNDLED_LIBRARIES_KEY, libs);

            // load and start QtLoader class
            DexClassLoader classLoader = new DexClassLoader(loaderParams.getString(DEX_PATH_KEY), // .jar/.apk files
                    m_context.getDir("outdex", Context.MODE_PRIVATE).getAbsolutePath(), // directory where optimized DEX files should be written.
                    loaderParams.containsKey(LIB_PATH_KEY) ? loaderParams.getString(LIB_PATH_KEY) : null, // libs folder (if exists)
                    m_context.getClassLoader()); // parent loader

            Class<?> loaderClass = classLoader.loadClass(loaderParams.getString(LOADER_CLASS_NAME_KEY)); // load QtLoader class
            Object qtLoader = loaderClass.newInstance(); // create an instance
            Method prepareAppMethod = qtLoader.getClass().getMethod("loadApplication",
                    contextClassName(),
                    ClassLoader.class,
                    Bundle.class);
            if (!(Boolean)prepareAppMethod.invoke(qtLoader, m_context, classLoader, loaderParams))
                throw new Exception("");

            setQtContextDelegate(m_delegateClass, qtLoader);

            Method startAppMethod=qtLoader.getClass().getMethod("startApplication");
            if (!(Boolean)startAppMethod.invoke(qtLoader))
                throw new Exception("");

        } catch (Exception e) {
            e.printStackTrace();
            AlertDialog errorDialog = new AlertDialog.Builder(m_context).create();
            int id = resources.getIdentifier("fatal_error_msg", "string",
                    packageName);
            errorDialog.setMessage(resources.getString(id));
            errorDialog.setButton(Dialog.BUTTON_POSITIVE,
                    resources.getString(android.R.string.ok),
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            finish();
                        }
                    });
            errorDialog.show();
        }
    }

    public void startApp(final boolean firstStart)
    {
        try {
            final Resources resources = m_context.getResources();
            final String packageName = m_context.getPackageName();
            int id = resources.getIdentifier("qt_libs", "array", packageName);
            m_qtLibs =  prefferedAbiLibs(resources.getStringArray(id));

            id = resources.getIdentifier("use_local_qt_libs", "string", packageName);
            final int useLocalLibs = Integer.parseInt(resources.getString(id));

            if (useLocalLibs == 1) {
                ArrayList<String> libraryList = new ArrayList<>();
                String libsDir = null;
                String bundledLibsDir = null;

                id = resources.getIdentifier("bundle_local_qt_libs", "string", packageName);
                final int bundleLocalLibs = Integer.parseInt(resources.getString(id));
                if (bundleLocalLibs == 0) {
                    String systemLibsPrefix;
                    final String systemLibsKey = "android.app.system_libs_prefix";
                    // First check if user has provided system libs prefix in AndroidManifest
                    if (m_contextInfo.applicationInfo.metaData != null &&
                            m_contextInfo.applicationInfo.metaData.containsKey(systemLibsKey)) {
                        systemLibsPrefix = m_contextInfo.applicationInfo.metaData.getString(systemLibsKey);
                    } else {
                        // If not, check if it's provided by androiddeployqt in libs.xml
                        id = resources.getIdentifier("system_libs_prefix","string",
                                packageName);
                        systemLibsPrefix = resources.getString(id);
                    }
                    if (systemLibsPrefix.isEmpty()) {
                        systemLibsPrefix = SYSTEM_LIB_PATH;
                        Log.e(QtTAG, "It looks like app deployed using Unbundled "
                                + "deployment. It may be necessary to specify path to directory "
                                + "where Qt libraries are installed using either "
                                + "android.app.system_libs_prefix metadata variable in your "
                                + "AndroidManifest.xml or QT_ANDROID_SYSTEM_LIBS_PATH in your "
                                + "CMakeLists.txt");
                        Log.e(QtTAG, "Using " + SYSTEM_LIB_PATH + " as default path");
                    }

                    File systemLibraryDir = new File(systemLibsPrefix);
                    if (systemLibraryDir.exists() && systemLibraryDir.isDirectory()
                            && systemLibraryDir.list().length > 0) {
                        libsDir = systemLibsPrefix;
                        bundledLibsDir = systemLibsPrefix;
                    } else {
                        Log.e(QtTAG,
                              "System library directory " + systemLibsPrefix
                                      + " does not exist or is empty.");
                    }
                } else {
                    String nativeLibraryPrefix = m_context.getApplicationInfo().nativeLibraryDir + "/";
                    File nativeLibraryDir = new File(nativeLibraryPrefix);
                    if (nativeLibraryDir.exists() && nativeLibraryDir.isDirectory() && nativeLibraryDir.list().length > 0) {
                        libsDir = nativeLibraryPrefix;
                        bundledLibsDir = nativeLibraryPrefix;
                    } else {
                        Log.e(QtTAG,
                              "Native library directory " + nativeLibraryPrefix
                                      + " does not exist or is empty.");
                    }
                }

                if (libsDir == null)
                    throw new Exception("");


                if (m_qtLibs != null) {
                    String libPrefix = libsDir + "lib";
                    for (String lib : m_qtLibs)
                        libraryList.add(libPrefix + lib + ".so");
                }

                id = resources.getIdentifier("load_local_libs", "array", packageName);
                ArrayList<String> localLibs = prefferedAbiLibs(resources.getStringArray(id));
                for (String libs : localLibs) {
                    for (String lib : libs.split(":")) {
                        if (!lib.isEmpty())
                            libraryList.add(libsDir + lib);
                    }
                }
                if (bundledLibsDir != null) {
                    ENVIRONMENT_VARIABLES += "\tQT_PLUGIN_PATH=" + bundledLibsDir;
                    ENVIRONMENT_VARIABLES += "\tQML_PLUGIN_PATH=" + bundledLibsDir;
                }

                Bundle loaderParams = new Bundle();
                loaderParams.putInt(ERROR_CODE_KEY, 0);
                loaderParams.putString(DEX_PATH_KEY, new String());
                loaderParams.putString(LOADER_CLASS_NAME_KEY, loaderClassName());

                id = resources.getIdentifier("static_init_classes", "string", packageName);
                loaderParams.putStringArray(STATIC_INIT_CLASSES_KEY, resources.getString(id)
                        .split(":"));

                loaderParams.putStringArrayList(NATIVE_LIBRARIES_KEY, libraryList);


                String themePath = m_context.getApplicationInfo().dataDir + "/qt-reserved-files/android-style/";
                String stylePath = themePath + m_displayDensity + "/";

                String extractOption = "default";
                if (m_contextInfo.metaData.containsKey("android.app.extract_android_style")) {
                    extractOption = m_contextInfo.metaData.getString("android.app.extract_android_style");
                    if (!extractOption.equals("default") && !extractOption.equals("full") && !extractOption.equals("minimal") && !extractOption.equals("none")) {
                        Log.e(QtTAG, "Invalid extract_android_style option \"" + extractOption + "\", defaulting to \"default\"");
                        extractOption = "default";
                   }
                }

                // QTBUG-69810: The extraction code will trigger compatibility warnings on Android SDK version >= 28
                // when the target SDK version is set to something lower then 28, so default to "none" and issue a warning
                // if that is the case.
                if (extractOption.equals("default")) {
                    final int targetSdkVersion = m_context.getApplicationInfo().targetSdkVersion;
                    if (targetSdkVersion < 28 && Build.VERSION.SDK_INT >= 28) {
                        Log.e(QtTAG, "extract_android_style option set to \"none\" when targetSdkVersion is less then 28");
                        extractOption = "none";
                    }
                }

                if (!extractOption.equals("none")) {
                    loaderParams.putString(EXTRACT_STYLE_KEY, stylePath);
                    loaderParams.putBoolean(EXTRACT_STYLE_MINIMAL_KEY, extractOption.equals("minimal"));
                }

                if (extractOption.equals("full"))
                    ENVIRONMENT_VARIABLES += "\tQT_USE_ANDROID_NATIVE_STYLE=1";

                ENVIRONMENT_VARIABLES += "\tANDROID_STYLE_PATH=" + stylePath;

                if (m_contextInfo.metaData.containsKey("android.app.trace_location")) {
                    String loc = m_contextInfo.metaData.getString("android.app.trace_location");
                    ENVIRONMENT_VARIABLES += "\tQTRACE_LOCATION="+loc;
                }

                loaderParams.putString(ENVIRONMENT_VARIABLES_KEY, ENVIRONMENT_VARIABLES);

                String appParams = null;
                if (APPLICATION_PARAMETERS != null)
                    appParams = APPLICATION_PARAMETERS;

                Intent intent = getIntent();
                if (intent != null) {
                    String parameters = intent.getStringExtra("applicationArguments");
                    if (parameters != null)
                        if (appParams == null)
                            appParams = parameters;
                        else
                            appParams += '\t' + parameters;
                }

                if (m_contextInfo.metaData.containsKey("android.app.arguments")) {
                    String parameters = m_contextInfo.metaData.getString("android.app.arguments");
                    if (appParams == null)
                        appParams = parameters;
                    else
                        appParams += '\t' + parameters;
                }

                if (appParams != null)
                    loaderParams.putString(APPLICATION_PARAMETERS_KEY, appParams);

                loadApplication(loaderParams);
                return;
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Can't create main activity", e);
        }
    }
}
