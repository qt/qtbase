// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2019, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:trusted-data-only

package org.qtproject.qt.android;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Service;
import android.content.Intent;
import android.content.ComponentName;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ComponentInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.Process;
import android.system.Os;
import android.util.Log;

import java.io.File;
import java.lang.IllegalArgumentException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Objects;
import java.util.HashSet;
import java.util.Set;

import dalvik.system.DexClassLoader;

abstract class QtLoader {

    protected static final String QtTAG = "QtLoader";

    private final Resources m_resources;
    private final String m_packageName;
    private final String m_preferredAbi;
    private String m_extractedNativeLibsDir = null;
    /** @noinspection FieldCanBeLocal*/
    private ClassLoader m_classLoader;

    protected ComponentInfo m_contextInfo;

    protected String m_mainLibPath;
    protected String m_mainLibName;
    protected String m_applicationParameters = "";
    protected final HashMap<String, String> m_environmentVariables = new HashMap<>();

    protected static QtLoader m_instance = null;
    protected boolean m_librariesLoaded;

    enum LoadingResult { Succeeded, AlreadyLoaded, Failed }

    /**
     * Sets and initialize the basic pieces.
     * Initializes the class loader since it doesn't rely on anything
     * other than the context.
     * Also, we can already initialize the static classes contexts here.
     * @throws IllegalArgumentException if the given Context is not either an Activity or Service,
     *         or no ComponentInfo could be found for it
     **/
     QtLoader(ContextWrapper context) throws IllegalArgumentException {
        m_resources = context.getResources();
        m_packageName = context.getPackageName();
        final Context baseContext = context.getBaseContext();
        if (!(baseContext instanceof Activity || baseContext instanceof Service)) {
            throw new IllegalArgumentException("QtLoader: Context is not an instance of " +
                                               "Activity or Service");
        }

        initClassLoader(baseContext);
        try {
            initContextInfo(baseContext);
        } catch (NameNotFoundException e) {
            throw new IllegalArgumentException("QtLoader: No ComponentInfo found for given " +
                                               "Context", e);
        }
        m_preferredAbi = resolvePreferredAbi();
    }

    /**
     * Initializes the context info instance which is used to retrieve
     * the app metadata from the AndroidManifest.xml or other xml resources.
     * Some values are dependent on the context being an Activity or Service.
     **/
    protected void initContextInfo(Context context) throws NameNotFoundException {
        if (context instanceof Activity) {
            m_contextInfo = context.getPackageManager().getActivityInfo(
                    ((Activity)context).getComponentName(), PackageManager.GET_META_DATA);
        } else if (context instanceof Service) {
            m_contextInfo = context.getPackageManager().getServiceInfo(
                    new ComponentName(context, context.getClass()),
                    PackageManager.GET_META_DATA);
        }
    }

    /**
     * Extract the common metadata in the base implementation. And the extended methods
     * call context specific metadata extraction. This also sets the various environment
     * variables and application parameters.
     **/
    protected void extractContextMetaData(Context context) {
        setEnvironmentVariable("QT_ANDROID_FONTS", "Roboto;Droid Sans;Droid Sans Fallback");
        String monospaceFonts = "Droid Sans Mono;Droid Sans;Droid Sans Fallback";
        setEnvironmentVariable("QT_ANDROID_FONTS_MONOSPACE", monospaceFonts);
        setEnvironmentVariable("QT_ANDROID_FONTS_SERIF", "Droid Serif");
        setEnvironmentVariable("HOME", context.getFilesDir().getAbsolutePath());
        setEnvironmentVariable("TMPDIR", context.getCacheDir().getAbsolutePath());
        setEnvironmentVariable("QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED", isBackgroundRunningBlocked());
        setEnvironmentVariable("QTRACE_LOCATION", getMetaData("android.app.trace_location"));
        appendApplicationParameters(getMetaData("android.app.arguments"));

        if (context instanceof Activity) {
            Intent intent = ((Activity) context).getIntent();
            if (intent != null)
                appendApplicationParameters(intent.getStringExtra("applicationArguments"));
        }
    }

    private String isBackgroundRunningBlocked() {
        final String backgroundRunning = getMetaData("android.app.background_running");
        if (backgroundRunning.compareTo("true") == 0)
            return "0";
        return "1";
    }

    private ArrayList<String> preferredAbiLibs(String[] libs) {
        ArrayList<String> abiLibs = new ArrayList<>();
        for (String lib : libs) {
            String[] splits = lib.split(";", 2);
            // Ensure we have both abi and lib name parts
            if (splits == null || splits.length < 2)
                continue;

            if (!splits[0].equals(m_preferredAbi))
                continue;

            abiLibs.add(splits[1]);
        }

        return abiLibs;
    }

    @SuppressLint("DiscouragedApi")
    private String resolvePreferredAbi()
    {
        int id = m_resources.getIdentifier("qt_libs", "array", m_packageName);
        String[] libs = m_resources.getStringArray(id);
        Set<String> uniqueAbis = new HashSet<>();

        for (String lib : libs) {
            String[] splits = lib.split(";", 2);
            // Ensure we have both abi and lib name parts
            if (splits == null || splits.length < 2)
                continue;

            uniqueAbis.add(splits[0]);
        }

        boolean is64Bit = Process.is64Bit();
        for (String abi : Build.SUPPORTED_ABIS) {
            if (uniqueAbis.contains(abi) && abi.contains("64") == is64Bit)
                return abi;
        }

        return Build.SUPPORTED_ABIS[0];
    }

    /**
     * Initialize the class loader instance and sets it via QtNative.
     * This would also be used by QJniObject API.
     **/
    private void initClassLoader(Context context)
    {
        // directory where optimized DEX files should be written.
        String outDexPath = context.getDir("outdex", Context.MODE_PRIVATE).getAbsolutePath();
        String sourceDir = context.getApplicationInfo().sourceDir;
        m_classLoader = new DexClassLoader(sourceDir, outDexPath, null, context.getClassLoader());
        QtNative.setClassLoader(m_classLoader);
    }

    /**
     * Returns the context's main library absolute path,
     * or null if the library hasn't been loaded yet.
     **/
    public String getMainLibraryPath() {
        return m_mainLibPath;
    }

    /**
     * Set the name of the main app library to libName, which is the name of the library,
     * not including the path, target architecture or .so suffix. This matches the target name
     * of the app target in CMakeLists.txt.
     * This method can be used when the name is not provided by androiddeployqt, for example when
     * embedding QML views to a native Android app.
     **/
    public void setMainLibraryName(String libName) {
        m_mainLibName = libName;
    }

    /**
     * Returns the context's parameters that are used when calling
     * the main library's main() function. This is assembled from
     * a combination of static values and also metadata dependent values.
     **/
    public String getApplicationParameters() {
        return m_applicationParameters;
    }

    /**
     * Adds a list of parameters to the internal array list of parameters.
     * Either a whitespace or a tab is accepted as a separator between parameters.
     **/
    public void appendApplicationParameters(String params)
    {
        if (params == null || params.isEmpty())
            return;

        if (!m_applicationParameters.isEmpty())
            m_applicationParameters += " ";
        m_applicationParameters += params;
    }

    /**
     * Sets a single key/value environment variable pair.
     **/
    public void setEnvironmentVariable(String key, String value)
    {
        try {
            android.system.Os.setenv(key, value, true);
            m_environmentVariables.put(key, value);
        } catch (Exception e) {
            Log.e(QtTAG, "Could not set environment variable:" + key + "=" + value);
            e.printStackTrace();
        }
    }

    /**
     * Sets a list of keys/values string to as environment variables.
     * This expects the key/value to be separated by '=', and parameters
     * to be separated by tabs or space.
     **/
    public void setEnvironmentVariables(String environmentVariables)
    {
        if (environmentVariables == null || environmentVariables.isEmpty())
            return;

        environmentVariables = environmentVariables.replaceAll("\t", " ");

        for (String variable : environmentVariables.split(" ")) {
            String[] keyValue = variable.split("=", 2);
            if (keyValue.length < 2 || keyValue[0].isEmpty())
                continue;

            setEnvironmentVariable(keyValue[0], keyValue[1]);
        }
    }

    /**
     * Parses the native libraries dir. If the libraries are part of the APK,
     * the path is set to the APK extracted libs path.
     * Otherwise, it looks for the system level dir, that's either set in the Manifest,
     * the deployment libs.xml.
     * If none of the above are valid, it falls back to predefined system path.
     **/
    private void parseNativeLibrariesDir() {
        if (m_contextInfo == null)
            return;
        if (isBundleQtLibs()) {
            String nativeLibraryPrefix = m_contextInfo.applicationInfo.nativeLibraryDir + "/";
            File nativeLibraryDir = new File(nativeLibraryPrefix);
            if (nativeLibraryDir.exists()) {
                String[] list = nativeLibraryDir.list();
                if (nativeLibraryDir.isDirectory() && list != null && list.length > 0) {
                    m_extractedNativeLibsDir = nativeLibraryPrefix;
                }
            }
        } else {
            // First check if user has provided system libs prefix in AndroidManifest
            String systemLibsPrefix = getApplicationMetaData("android.app.system_libs_prefix");

            // If not, check if it's provided by androiddeployqt in libs.xml
            if (systemLibsPrefix.isEmpty())
                systemLibsPrefix = getSystemLibsPrefix();

            if (systemLibsPrefix.isEmpty()) {
                final String SYSTEM_LIB_PATH = "/system/lib/";
                systemLibsPrefix = SYSTEM_LIB_PATH;
                Log.e(QtTAG, "Using " + SYSTEM_LIB_PATH + " as default libraries path. "
                        + "It looks like the app is deployed using Unbundled "
                        + "deployment. It may be necessary to specify the path to "
                        + "the directory where Qt libraries are installed using either "
                        + "android.app.system_libs_prefix metadata variable in your "
                        + "AndroidManifest.xml or QT_ANDROID_SYSTEM_LIBS_PATH in your "
                        + "CMakeLists.txt");
            }

            File systemLibraryDir = new File(systemLibsPrefix);
            String[] list = systemLibraryDir.list();
            if (systemLibraryDir.exists()) {
                if (systemLibraryDir.isDirectory() && list != null && list.length > 0)
                    m_extractedNativeLibsDir = systemLibsPrefix;
                else
                    Log.e(QtTAG, "System library directory " + systemLibsPrefix + " is empty.");
            } else {
                Log.e(QtTAG, "System library directory " + systemLibsPrefix + " does not exist.");
            }
        }

        if (m_extractedNativeLibsDir != null && !m_extractedNativeLibsDir.endsWith("/"))
            m_extractedNativeLibsDir += "/";
    }

    /**
     * Returns the application level metadata.
     *
     * @noinspection SameParameterValue*/
    private String getApplicationMetaData(String key) {
        ApplicationInfo applicationInfo = m_contextInfo.applicationInfo;
        if (applicationInfo == null)
            return "";

        Bundle metadata = applicationInfo.metaData;
        if (metadata == null || !metadata.containsKey(key))
            return "";

        return metadata.getString(key);
    }

    /**
     * Returns the context level metadata.
     **/
    protected String getMetaData(String key) {
        if (m_contextInfo == null)
            return "";

        Bundle metadata = m_contextInfo.metaData;
        if (metadata == null || !metadata.containsKey(key))
            return "";

        return String.valueOf(metadata.get(key));
    }

    @SuppressLint("DiscouragedApi")
    private ArrayList<String> getQtLibrariesList() {
        int id = m_resources.getIdentifier("qt_libs", "array", m_packageName);
        return preferredAbiLibs(m_resources.getStringArray(id));
    }

    @SuppressLint("DiscouragedApi")
    private boolean useLocalQtLibs() {
        int id = m_resources.getIdentifier("use_local_qt_libs", "string", m_packageName);
        return Integer.parseInt(m_resources.getString(id)) == 1;
    }

    @SuppressLint("DiscouragedApi")
    private boolean isBundleQtLibs() {
        int id = m_resources.getIdentifier("bundle_local_qt_libs", "string", m_packageName);
        return Integer.parseInt(m_resources.getString(id)) == 1;
    }

    @SuppressLint("DiscouragedApi")
    private String getSystemLibsPrefix() {
        int id = m_resources.getIdentifier("system_libs_prefix", "string", m_packageName);
        return m_resources.getString(id);
    }

    @SuppressLint("DiscouragedApi")
    private ArrayList<String> getLocalLibrariesList() {
        int id = m_resources.getIdentifier("load_local_libs", "array", m_packageName);
        ArrayList<String> localLibs = new ArrayList<>();
        for (String arrayItem : preferredAbiLibs(m_resources.getStringArray(id))) {
            Collections.addAll(localLibs, arrayItem.split(":"));
        }
        return localLibs;
    }

    @SuppressLint("DiscouragedApi")
    private String[] getBundledLibs() {
        int id = m_resources.getIdentifier("bundled_libs", "array", m_packageName);
        return m_resources.getStringArray(id);
    }

    /**
     * Returns true if the app is uses native libraries stored uncompressed in the APK.
     **/
    private static boolean isUncompressedNativeLibs()
    {
        int flags = QtNative.getContext().getApplicationInfo().flags;
        return (flags & ApplicationInfo.FLAG_EXTRACT_NATIVE_LIBS) == 0;
    }

    /**
     * Returns the native shared libraries path inside relative to the app's APK.
     **/
    private String getApkNativeLibrariesDir()
    {
        return QtApkFileEngine.getAppApkFilePath() + "!/lib/" + m_preferredAbi + "/";
    }

    /**
     * Returns QtLoader.LoadingResult.Succeeded if libraries are successfully loaded,
     * QtLoader.LoadingResult.AlreadyLoaded if they have already been loaded,
     * and QtLoader.LoadingResult.Failed if loading the libraries failed.
     **/
    public LoadingResult loadQtLibraries() {
        if (m_librariesLoaded)
            return LoadingResult.AlreadyLoaded;

        if (!useLocalQtLibs()) {
            Log.w(QtTAG, "Use local Qt libs is false");
            return LoadingResult.Failed;
        }

        if (m_extractedNativeLibsDir == null)
            parseNativeLibrariesDir();

        if (isUncompressedNativeLibs()) {
            String apkLibPath = getApkNativeLibrariesDir();
            setEnvironmentVariable("QT_PLUGIN_PATH", apkLibPath);
            setEnvironmentVariable("QML_PLUGIN_PATH", apkLibPath);
        } else {
            if (m_extractedNativeLibsDir == null || m_extractedNativeLibsDir.isEmpty()) {
                Log.e(QtTAG, "The native libraries directory is null or empty");
                return LoadingResult.Failed;
            }
            setEnvironmentVariable("QT_PLUGIN_PATH", m_extractedNativeLibsDir);
            setEnvironmentVariable("QML_PLUGIN_PATH", m_extractedNativeLibsDir);
        }

        // Load native Qt APK libraries
        ArrayList<String> nativeLibraries = getQtLibrariesList();
        nativeLibraries.addAll(getLocalLibrariesList());

        if (Debug.isDebuggerConnected()) {
            final String debuggerSleepEnvVarName = "QT_ANDROID_DEBUGGER_MAIN_THREAD_SLEEP_MS";
            int debuggerSleepMs = 3000;
            if (Os.getenv(debuggerSleepEnvVarName) != null) {
                try {
                   debuggerSleepMs = Integer.parseInt(Os.getenv(debuggerSleepEnvVarName));
                } catch (NumberFormatException ignored) {
                }
            }

            if (debuggerSleepMs > 0) {
                Log.i(QtTAG, "Sleeping for " + debuggerSleepMs +
                             "ms, helping the native debugger to settle. " +
                             "Use the env " + debuggerSleepEnvVarName +
                             " variable to change this value.");
                QtNative.getQtThread().sleep(debuggerSleepMs);
            }
        }

        if (!loadLibraries(nativeLibraries)) {
            Log.e(QtTAG, "Loading Qt native libraries failed");
            return LoadingResult.Failed;
        }

        // add all bundled Qt libs to loader params
        ArrayList<String> bundledLibraries = new ArrayList<>(preferredAbiLibs(getBundledLibs()));
        if (!loadLibraries(bundledLibraries)) {
            Log.e(QtTAG, "Loading Qt bundled libraries failed");
            return LoadingResult.Failed;
        }

        if (m_mainLibName == null)
            m_mainLibName = getMetaData("android.app.lib_name");
        // Load main lib
        if (!loadMainLibrary(m_mainLibName + "_" + m_preferredAbi)) {
            Log.e(QtTAG, "Loading main library failed");
            return LoadingResult.Failed;
        }
        m_librariesLoaded = true;
        return LoadingResult.Succeeded;
    }

    // Loading libraries using System.load() uses full lib paths
    // or System.loadLibrary() for uncompressed libs
    @SuppressLint("UnsafeDynamicallyLoadedCode")
    private String loadLibraryHelper(String library)
    {
        String loadedLib = null;
        try {
            File libFile = new File(library);
            if (library.startsWith("/")) {
                if (libFile.exists()) {
                    System.load(library);
                    loadedLib = library;
                } else {
                    Log.e(QtTAG, "Can't find '" + library + "'");
                }
            } else {
                System.loadLibrary(library);
                loadedLib = library;
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Can't load '" + library + "'", e);
        }

        return loadedLib;
    }

    /**
     * Returns an array with absolute library paths from a list of file names only.
     **/
    private ArrayList<String> getLibrariesFullPaths(final ArrayList<String> libraries)
    {
        if (libraries == null)
            return null;

        ArrayList<String> absolutePathLibraries = new ArrayList<>();
        for (String libName : libraries) {
            // Add lib and .so to the lib name only if it doesn't already end with .so,
            // this means some names don't necessarily need to have the lib prefix
            if (isUncompressedNativeLibs()) {
                if (libName.endsWith(".so"))
                    libName = libName.substring(3, libName.length() - 3);
                absolutePathLibraries.add(libName);
            } else {
                if (!libName.endsWith(".so"))
                    libName = "lib" + libName + ".so";
                File file = new File(m_extractedNativeLibsDir + libName);
                absolutePathLibraries.add(file.getAbsolutePath());
            }
        }

        return absolutePathLibraries;
    }

    /**
     * Loads the main library.
     * Returns true if loading was successful, and sets the absolute
     * path to the main library. Otherwise, returns false and the path
     * to the main library is null.
     **/
    private boolean loadMainLibrary(String mainLibName)
    {
        ArrayList<String> oneEntryArray = new ArrayList<>(Collections.singletonList(mainLibName));
        String mainLibPath = getLibrariesFullPaths(oneEntryArray).get(0);
        final boolean[] success = {true};
        QtNative.getQtThread().run(() -> {
            m_mainLibPath = loadLibraryHelper(mainLibPath);
            if (m_mainLibPath == null)
                success[0] = false;
            else if (isUncompressedNativeLibs())
                m_mainLibPath = getApkNativeLibrariesDir() + "lib" + m_mainLibPath + ".so";
        });

        return success[0];
    }

    /**
     * Loads a list of libraries.
     * Returns true if all libraries were loaded successfully,
     * and false if any library failed. Stops loading at the first failure.
     **/
    @SuppressWarnings("BooleanMethodIsAlwaysInverted")
    private boolean loadLibraries(final ArrayList<String> libraries)
    {
        if (libraries == null)
            return false;

        ArrayList<String> fullPathLibs = getLibrariesFullPaths(libraries);

        final boolean[] success = {true};
        QtNative.getQtThread().run(() -> {
            for (int i = 0; i < fullPathLibs.size(); ++i) {
                String libName = fullPathLibs.get(i);
                if (loadLibraryHelper(libName) == null) {
                    success[0] = false;
                    break;
                }
            }
        });

        return success[0];
    }
}
