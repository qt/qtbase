// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2019, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.ApplicationInfo;
import android.content.pm.ComponentInfo;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Objects;

import dalvik.system.DexClassLoader;

public abstract class QtLoader {

    protected static final String QtTAG = "QtLoader";

    private final Resources m_resources;
    private final String m_packageName;
    private String m_preferredAbi = null;
    private String m_nativeLibrariesDir = null;
    private ClassLoader m_classLoader;

    protected final ContextWrapper m_context;
    protected ComponentInfo m_contextInfo;

    protected String m_mainLib;
    protected ArrayList<String> m_applicationParameters = new ArrayList<>();
    protected HashMap<String, String> m_environmentVariables = new HashMap<>();

    /**
     * Sets and initialize the basic pieces.
     * Initializes the class loader since it doesn't rely on anything
     * other than the context.
     * Also, we can already initialize the static classes contexts here.
     **/
    public QtLoader(ContextWrapper context) {
        m_context = context;
        m_resources = m_context.getResources();
        m_packageName = m_context.getPackageName();

        initClassLoader();
        initStaticClasses();
        initContextInfo();
    }

    /**
     * Initializes the context info instance which is used to retrieve
     * the app metadata from the AndroidManifest.xml or other xml resources.
     * Some values are dependent on the context being an Activity or Service.
     **/
    abstract protected void initContextInfo();

    /**
     * Implements the logic for finish the extended context, mostly called
     * in error cases.
     **/
    abstract protected void finish();

    /**
     * Context specific (Activity/Service) implementation for static classes.
     * The Activity and Service loaders call different methods.
     **/
    abstract protected void initStaticClassesImpl(Class<?> initClass, Object staticInitDataObject);

    /**
     * Extract the common metadata in the base implementation. And the extended methods
     * call context specific metadata extraction. This also sets the various environment
     * variables and application parameters.
     **/
    protected void extractContextMetaData() {
        setEnvironmentVariable("QT_ANDROID_FONTS", "Roboto;Droid Sans;Droid Sans Fallback");
        String monospaceFonts = "Droid Sans Mono;Droid Sans;Droid Sans Fallback";
        setEnvironmentVariable("QT_ANDROID_FONTS_MONOSPACE", monospaceFonts);
        setEnvironmentVariable("QT_ANDROID_FONTS_SERIF", "Droid Serif");
        setEnvironmentVariable("HOME", m_context.getFilesDir().getAbsolutePath());
        setEnvironmentVariable("TMPDIR", m_context.getCacheDir().getAbsolutePath());
        String backgroundRunning = getMetaData("android.app.background_running");
        setEnvironmentVariable("QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED", backgroundRunning);
        setEnvironmentVariable("QTRACE_LOCATION", getMetaData("android.app.trace_location"));
        setApplicationParameters(getMetaData("android.app.arguments"));
    }

    private ArrayList<String> preferredAbiLibs(String[] libs) {
        HashMap<String, ArrayList<String>> abiLibs = new HashMap<>();
        for (String lib : libs) {
            String[] archLib = lib.split(";", 2);
            if (m_preferredAbi != null && !archLib[0].equals(m_preferredAbi))
                continue;
            if (!abiLibs.containsKey(archLib[0]))
                abiLibs.put(archLib[0], new ArrayList<>());
            Objects.requireNonNull(abiLibs.get(archLib[0])).add(archLib[1]);
        }

        if (m_preferredAbi != null) {
            if (abiLibs.containsKey(m_preferredAbi)) {
                return abiLibs.get(m_preferredAbi);
            }
            return new ArrayList<>();
        }

        for (String abi : Build.SUPPORTED_ABIS) {
            if (abiLibs.containsKey(abi)) {
                m_preferredAbi = abi;
                return abiLibs.get(abi);
            }
        }
        return new ArrayList<>();
    }

    private void initStaticClasses() {
        for (String className : getStaticInitClasses()) {
            try {
                Class<?> initClass = m_classLoader.loadClass(className);
                Object staticInitDataObject = initClass.newInstance(); // create an instance
                initStaticClassesImpl(initClass, staticInitDataObject);

                // For modules that don't need/have setActivity/setService
                Method m = initClass.getMethod("setContext", Context.class);
                m.invoke(staticInitDataObject, m_context);
            } catch (ClassNotFoundException | IllegalAccessException | InstantiationException |
                     NoSuchMethodException | InvocationTargetException e) {
                Log.d(QtTAG, "Class " + className + " does not implement setContext method");
            }
        }
    }

    /**
     * Initialize the class loader instance and sets it via QtNative.
     * This would also be used by QJniObject API.
     **/
    private void initClassLoader()
    {
        // directory where optimized DEX files should be written.
        String outDexPath = m_context.getDir("outdex", Context.MODE_PRIVATE).getAbsolutePath();
        String sourceDir = m_context.getApplicationInfo().sourceDir;
        m_classLoader = new DexClassLoader(sourceDir, outDexPath, null, m_context.getClassLoader());
        QtNative.setClassLoader(m_classLoader);
    }

    /**
     * Returns the context's main library absolute path,
     * or null if the library hasn't been loaded yet.
     **/
    public String getMainLibrary() {
        return m_mainLib;
    }

    /**
     * Returns the context's parameters that are used when calling
     * the main library's main() function. This is assembled from
     * a combination of static values and also metadata dependent values.
     **/
    public ArrayList<String> getApplicationParameters() {
        return m_applicationParameters;
    }

    /**
     * Adds a parameter string to the internal array list of parameters.
     **/
    public void setApplicationParameter(String param)
    {
        if (param == null || param.isEmpty())
            return;
        m_applicationParameters.add(param);
    }

    /**
     * Adds a list of parameters to the internal array list of parameters.
     * This expects the parameters separated by '\t'.
     **/
    public void setApplicationParameters(String params)
    {
        if (params == null || params.isEmpty())
            return;

        for (String param : params.split("\t"))
            setApplicationParameter(param);
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
     * to be separated by '\t'.
     * Ex: key1=val1\tkey2=val2\tkey3=val3
     **/
    public void setEnvironmentVariables(String environmentVariables)
    {
        if (environmentVariables == null || environmentVariables.isEmpty())
            return;

        for (String variable : environmentVariables.split("\t")) {
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
        if (isBundleQtLibs()) {
            String nativeLibraryPrefix = m_context.getApplicationInfo().nativeLibraryDir + "/";
            File nativeLibraryDir = new File(nativeLibraryPrefix);
            if (nativeLibraryDir.exists()) {
                String[] list = nativeLibraryDir.list();
                if (nativeLibraryDir.isDirectory() && list != null && list.length > 0) {
                    m_nativeLibrariesDir = nativeLibraryPrefix;
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
                    m_nativeLibrariesDir = systemLibsPrefix;
                else
                    Log.e(QtTAG, "System library directory " + systemLibsPrefix + " is empty.");
            } else {
                Log.e(QtTAG, "System library directory " + systemLibsPrefix + " does not exist.");
            }
        }

        if (m_nativeLibrariesDir != null && !m_nativeLibrariesDir.endsWith("/"))
            m_nativeLibrariesDir += "/";
    }

    /**
     * Returns the application level metadata.
     **/
    private String getApplicationMetaData(String key) {
        if (m_contextInfo == null)
            return "";

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

        return metadata.getString(key);
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
    private ArrayList<String> getStaticInitClasses() {
        int id = m_resources.getIdentifier("static_init_classes", "string", m_packageName);
        String[] classes = m_resources.getString(id).split(":");
        ArrayList<String> finalClasses = new ArrayList<>();
        for (String element : classes) {
            if (!element.isEmpty()) {
                finalClasses.add(element);
            }
        }
        return finalClasses;
    }

    @SuppressLint("DiscouragedApi")
    private String[] getBundledLibs() {
        int id = m_resources.getIdentifier("bundled_libs", "array", m_packageName);
        return m_resources.getStringArray(id);
    }

    /**
     * Loads all Qt native bundled libraries and main library.
     **/
    public void loadQtLibraries() {
        if (!useLocalQtLibs()) {
            Log.w(QtTAG, "Use local Qt libs is false");
            finish();
            return;
        }

        if (m_nativeLibrariesDir == null)
            parseNativeLibrariesDir();

        if (m_nativeLibrariesDir == null || m_nativeLibrariesDir.isEmpty()) {
            Log.e(QtTAG, "The native libraries directory is null or empty");
            finish();
            return;
        }

        setEnvironmentVariable("QT_PLUGIN_PATH", m_nativeLibrariesDir);
        setEnvironmentVariable("QML_PLUGIN_PATH", m_nativeLibrariesDir);

        // Load native Qt APK libraries
        ArrayList<String> nativeLibraries = getQtLibrariesList();
        nativeLibraries.addAll(getLocalLibrariesList());

        if (!loadLibraries(nativeLibraries)) {
            Log.e(QtTAG, "Loading Qt native libraries failed");
            finish();
            return;
        }

        // add all bundled Qt libs to loader params
        ArrayList<String> bundledLibraries = new ArrayList<>(preferredAbiLibs(getBundledLibs()));
        if (!loadLibraries(bundledLibraries)) {
            Log.e(QtTAG, "Loading Qt bundled libraries failed");
            finish();
            return;
        }

        // Load main lib
        if (!loadMainLibrary(getMetaData("android.app.lib_name") + "_" + m_preferredAbi)) {
            Log.e(QtTAG, "Loading main library failed");
            finish();
        }
    }

    // Loading libraries using System.load() uses full lib paths
    @SuppressLint("UnsafeDynamicallyLoadedCode")
    private String loadLibraryHelper(String library)
    {
        String loadedLib = null;
        try {
            File libFile = new File(library);
            if (libFile.exists()) {
                System.load(library);
                loadedLib = library;
            } else {
                Log.e(QtTAG, "Can't find '" + library + "'");
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
            if (!libName.startsWith("lib"))
                libName = "lib" + libName;
            if (!libName.endsWith(".so"))
                libName = libName + ".so";
            File file = new File(m_nativeLibrariesDir + libName);
            absolutePathLibraries.add(file.getAbsolutePath());
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
        QtNative.getQtThread().run(new Runnable() {
            @Override
            public void run() {
                m_mainLib = loadLibraryHelper(mainLibPath);
                if (m_mainLib == null)
                    success[0] = false;
            }
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
        QtNative.getQtThread().run(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < fullPathLibs.size(); ++i) {
                    String libName = fullPathLibs.get(i);
                    if (loadLibraryHelper(libName) == null) {
                        success[0] = false;
                        break;
                    }
                }
            }
        });

        return success[0];
    }
}
