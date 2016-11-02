/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
    Contact: http://www.qt.io/licensing/

    Commercial License Usage
    Licensees holding valid commercial Qt licenses may use this file in
    accordance with the commercial license agreement provided with the
    Software or, alternatively, in accordance with the terms contained in
    a written agreement between you and The Qt Company. For licensing terms
    and conditions see http://www.qt.io/terms-conditions. For further
    information use the contact form at http://www.qt.io/contact-us.

    BSD License Usage
    Alternatively, this file may be used under the BSD license as follows:
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package org.qtproject.qt5.android.bindings;

import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ComponentInfo;
import android.content.pm.PackageInfo;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import org.kde.necessitas.ministro.IMinistro;
import org.kde.necessitas.ministro.IMinistroCallback;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;

import dalvik.system.DexClassLoader;

public abstract class QtLoader {

    public final static int MINISTRO_INSTALL_REQUEST_CODE = 0xf3ee; // request code used to know when Ministro instalation is finished
    public static final int MINISTRO_API_LEVEL = 5; // Ministro api level (check IMinistro.aidl file)
    public static final int NECESSITAS_API_LEVEL = 2; // Necessitas api level used by platform plugin
    public static final int QT_VERSION = 0x050700; // This app requires at least Qt version 5.7.0

    public static final String ERROR_CODE_KEY = "error.code";
    public static final String ERROR_MESSAGE_KEY = "error.message";
    public static final String DEX_PATH_KEY = "dex.path";
    public static final String LIB_PATH_KEY = "lib.path";
    public static final String LOADER_CLASS_NAME_KEY = "loader.class.name";
    public static final String NATIVE_LIBRARIES_KEY = "native.libraries";
    public static final String ENVIRONMENT_VARIABLES_KEY = "environment.variables";
    public static final String APPLICATION_PARAMETERS_KEY = "application.parameters";
    public static final String BUNDLED_LIBRARIES_KEY = "bundled.libraries";
    public static final String BUNDLED_IN_LIB_RESOURCE_ID_KEY = "android.app.bundled_in_lib_resource_id";
    public static final String BUNDLED_IN_ASSETS_RESOURCE_ID_KEY = "android.app.bundled_in_assets_resource_id";
    public static final String MAIN_LIBRARY_KEY = "main.library";
    public static final String STATIC_INIT_CLASSES_KEY = "static.init.classes";
    public static final String NECESSITAS_API_LEVEL_KEY = "necessitas.api.level";
    public static final String EXTRACT_STYLE_KEY = "extract.android.style";
    private static final String EXTRACT_STYLE_MINIMAL_KEY = "extract.android.style.option";

    /// Ministro server parameter keys
    public static final String REQUIRED_MODULES_KEY = "required.modules";
    public static final String APPLICATION_TITLE_KEY = "application.title";
    public static final String MINIMUM_MINISTRO_API_KEY = "minimum.ministro.api";
    public static final String MINIMUM_QT_VERSION_KEY = "minimum.qt.version";
    public static final String SOURCES_KEY = "sources";               // needs MINISTRO_API_LEVEL >=3 !!!
    // Use this key to specify any 3rd party sources urls
    // Ministro will download these repositories into their
    // own folders, check http://community.kde.org/Necessitas/Ministro
    // for more details.

    public static final String REPOSITORY_KEY = "repository";         // use this key to overwrite the default ministro repsitory
    public static final String ANDROID_THEMES_KEY = "android.themes"; // themes that your application uses


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

    public static final int INCOMPATIBLE_MINISTRO_VERSION = 1; // Incompatible Ministro version. Ministro needs to be upgraded.
    public static final int BUFFER_SIZE = 1024;

    public String[] m_sources = {"https://download.qt-project.org/ministro/android/qt5/qt-5.7"}; // Make sure you are using ONLY secure locations
    public String m_repository = "default"; // Overwrites the default Ministro repository
    // Possible values:
    // * default - Ministro default repository set with "Ministro configuration tool".
    // By default the stable version is used. Only this or stable repositories should
    // be used in production.
    // * stable - stable repository, only this and default repositories should be used
    // in production.
    // * testing - testing repository, DO NOT use this repository in production,
    // this repository is used to push a new release, and should be used to test your application.
    // * unstable - unstable repository, DO NOT use this repository in production,
    // this repository is used to push Qt snapshots.
    public String[] m_qtLibs = null; // required qt libs
    public int m_displayDensity = -1;
    private ContextWrapper m_context;
    protected ComponentInfo m_contextInfo;
    private Class<?> m_delegateClass;

    QtLoader(ContextWrapper context, Class<?> clazz) {
        m_context = context;
        m_delegateClass = clazz;
    }

    // Implement in subclass
    protected void finish() {}

    protected String getTitle() {
        return "Qt";
    }

    protected void runOnUiThread(Runnable run) {
        run.run();
    }
    protected void downloadUpgradeMinistro(String msg)
    {
        Log.e(QtApplication.QtTAG, msg);
    }

    protected abstract String loaderClassName();
    protected abstract Class<?> contextClassName();

    Intent getIntent()
    {
        return null;
    }
    // Implement in subclass


    // this function is used to load and start the loader
    private void loadApplication(Bundle loaderParams)
    {
        try {
            final int errorCode = loaderParams.getInt(ERROR_CODE_KEY);
            if (errorCode != 0) {
                if (errorCode == INCOMPATIBLE_MINISTRO_VERSION) {
                    downloadUpgradeMinistro(loaderParams.getString(ERROR_MESSAGE_KEY));
                    return;
                }

                // fatal error, show the error and quit
                AlertDialog errorDialog = new AlertDialog.Builder(m_context).create();
                errorDialog.setMessage(loaderParams.getString(ERROR_MESSAGE_KEY));
                errorDialog.setButton(m_context.getResources().getString(android.R.string.ok), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        finish();
                    }
                });
                errorDialog.show();
                return;
            }

            // add all bundled Qt libs to loader params
            ArrayList<String> libs = new ArrayList<String>();
            if ( m_contextInfo.metaData.containsKey("android.app.bundled_libs_resource_id") )
                libs.addAll(Arrays.asList(m_context.getResources().getStringArray(m_contextInfo.metaData.getInt("android.app.bundled_libs_resource_id"))));

            String libName = null;
            if ( m_contextInfo.metaData.containsKey("android.app.lib_name") ) {
                libName = m_contextInfo.metaData.getString("android.app.lib_name");
                loaderParams.putString(MAIN_LIBRARY_KEY, libName); //main library contains main() function
            }

            loaderParams.putStringArrayList(BUNDLED_LIBRARIES_KEY, libs);
            loaderParams.putInt(NECESSITAS_API_LEVEL_KEY, NECESSITAS_API_LEVEL);

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

            QtApplication.setQtContextDelegate(m_delegateClass, qtLoader);

            // now load the application library so it's accessible from this class loader
            if (libName != null)
                System.loadLibrary(libName);

            Method startAppMethod=qtLoader.getClass().getMethod("startApplication");
            if (!(Boolean)startAppMethod.invoke(qtLoader))
                throw new Exception("");

        } catch (Exception e) {
            e.printStackTrace();
            AlertDialog errorDialog = new AlertDialog.Builder(m_context).create();
            if (m_contextInfo.metaData.containsKey("android.app.fatal_error_msg"))
                errorDialog.setMessage(m_contextInfo.metaData.getString("android.app.fatal_error_msg"));
            else
                errorDialog.setMessage("Fatal error, your application can't be started.");

            errorDialog.setButton(m_context.getResources().getString(android.R.string.ok), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    finish();
                }
            });
            errorDialog.show();
        }
    }

    private ServiceConnection m_ministroConnection=new ServiceConnection() {
        private IMinistro m_service = null;
        @Override
        public void onServiceConnected(ComponentName name, IBinder service)
        {
            m_service = IMinistro.Stub.asInterface(service);
            try {
                if (m_service != null) {
                    Bundle parameters = new Bundle();
                    parameters.putStringArray(REQUIRED_MODULES_KEY, m_qtLibs);
                    parameters.putString(APPLICATION_TITLE_KEY, getTitle());
                    parameters.putInt(MINIMUM_MINISTRO_API_KEY, MINISTRO_API_LEVEL);
                    parameters.putInt(MINIMUM_QT_VERSION_KEY, QT_VERSION);
                    parameters.putString(ENVIRONMENT_VARIABLES_KEY, ENVIRONMENT_VARIABLES);
                    if (APPLICATION_PARAMETERS != null)
                        parameters.putString(APPLICATION_PARAMETERS_KEY, APPLICATION_PARAMETERS);
                    parameters.putStringArray(SOURCES_KEY, m_sources);
                    parameters.putString(REPOSITORY_KEY, m_repository);
                    if (QT_ANDROID_THEMES != null)
                        parameters.putStringArray(ANDROID_THEMES_KEY, QT_ANDROID_THEMES);
                    m_service.requestLoader(m_ministroCallback, parameters);
                }
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }

        private IMinistroCallback m_ministroCallback = new IMinistroCallback.Stub() {
            // this function is called back by Ministro.
            @Override
            public void loaderReady(final Bundle loaderParams) throws RemoteException {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        m_context.unbindService(m_ministroConnection);
                        loadApplication(loaderParams);
                    }
                });
            }
        };

        @Override
        public void onServiceDisconnected(ComponentName name) {
            m_service = null;
        }
    };

    protected void ministroNotFound()
    {
        AlertDialog errorDialog = new AlertDialog.Builder(m_context).create();

        if (m_contextInfo.metaData.containsKey("android.app.ministro_not_found_msg"))
            errorDialog.setMessage(m_contextInfo.metaData.getString("android.app.ministro_not_found_msg"));
        else
            errorDialog.setMessage("Can't find Ministro service.\nThe application can't start.");

        errorDialog.setButton(m_context.getResources().getString(android.R.string.ok), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                finish();
            }
        });
        errorDialog.show();
    }

    static private void copyFile(InputStream inputStream, OutputStream outputStream)
            throws IOException
    {
        byte[] buffer = new byte[BUFFER_SIZE];

        int count;
        while ((count = inputStream.read(buffer)) > 0)
            outputStream.write(buffer, 0, count);
    }

    private void copyAsset(String source, String destination)
            throws IOException
    {
        // Already exists, we don't have to do anything
        File destinationFile = new File(destination);
        if (destinationFile.exists())
            return;

        File parentDirectory = destinationFile.getParentFile();
        if (!parentDirectory.exists())
            parentDirectory.mkdirs();

        destinationFile.createNewFile();

        AssetManager assetsManager = m_context.getAssets();
        InputStream inputStream = assetsManager.open(source);
        OutputStream outputStream = new FileOutputStream(destinationFile);
        copyFile(inputStream, outputStream);

        inputStream.close();
        outputStream.close();
    }

    private static void createBundledBinary(String source, String destination)
            throws IOException
    {
        // Already exists, we don't have to do anything
        File destinationFile = new File(destination);
        if (destinationFile.exists())
            return;

        File parentDirectory = destinationFile.getParentFile();
        if (!parentDirectory.exists())
            parentDirectory.mkdirs();

        destinationFile.createNewFile();

        InputStream inputStream = new FileInputStream(source);
        OutputStream outputStream = new FileOutputStream(destinationFile);
        copyFile(inputStream, outputStream);

        inputStream.close();
        outputStream.close();
    }

    private boolean cleanCacheIfNecessary(String pluginsPrefix, long packageVersion)
    {
        File versionFile = new File(pluginsPrefix + "cache.version");

        long cacheVersion = 0;
        if (versionFile.exists() && versionFile.canRead()) {
            try {
                DataInputStream inputStream = new DataInputStream(new FileInputStream(versionFile));
                cacheVersion = inputStream.readLong();
                inputStream.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        if (cacheVersion != packageVersion) {
            deleteRecursively(new File(pluginsPrefix));
            return true;
        } else {
            return false;
        }
    }

    private void extractBundledPluginsAndImports(String pluginsPrefix)
            throws IOException
    {
        ArrayList<String> libs = new ArrayList<String>();

        String libsDir = m_context.getApplicationInfo().nativeLibraryDir + "/";

        long packageVersion = -1;
        try {
            PackageInfo packageInfo = m_context.getPackageManager().getPackageInfo(m_context.getPackageName(), 0);
            packageVersion = packageInfo.lastUpdateTime;
        } catch (Exception e) {
            e.printStackTrace();
        }

        if (!cleanCacheIfNecessary(pluginsPrefix, packageVersion))
            return;

        {
            File versionFile = new File(pluginsPrefix + "cache.version");

            File parentDirectory = versionFile.getParentFile();
            if (!parentDirectory.exists())
                parentDirectory.mkdirs();

            versionFile.createNewFile();

            DataOutputStream outputStream = new DataOutputStream(new FileOutputStream(versionFile));
            outputStream.writeLong(packageVersion);
            outputStream.close();
        }

        {
            String key = BUNDLED_IN_LIB_RESOURCE_ID_KEY;
            java.util.Set<String> keys = m_contextInfo.metaData.keySet();
            if (m_contextInfo.metaData.containsKey(key)) {
                String[] list = m_context.getResources().getStringArray(m_contextInfo.metaData.getInt(key));

                for (String bundledImportBinary : list) {
                    String[] split = bundledImportBinary.split(":");
                    String sourceFileName = libsDir + split[0];
                    String destinationFileName = pluginsPrefix + split[1];
                    createBundledBinary(sourceFileName, destinationFileName);
                }
            }
        }

        {
            String key = BUNDLED_IN_ASSETS_RESOURCE_ID_KEY;
            if (m_contextInfo.metaData.containsKey(key)) {
                String[] list = m_context.getResources().getStringArray(m_contextInfo.metaData.getInt(key));

                for (String fileName : list) {
                    String[] split = fileName.split(":");
                    String sourceFileName = split[0];
                    String destinationFileName = pluginsPrefix + split[1];
                    copyAsset(sourceFileName, destinationFileName);
                }
            }

        }
    }

    private void deleteRecursively(File directory)
    {
        File[] files = directory.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.isDirectory())
                    deleteRecursively(file);
                else
                    file.delete();
            }

            directory.delete();
        }
    }

    private void cleanOldCacheIfNecessary(String oldLocalPrefix, String localPrefix)
    {
        File newCache = new File(localPrefix);
        if (!newCache.exists()) {
            {
                File oldPluginsCache = new File(oldLocalPrefix + "plugins/");
                if (oldPluginsCache.exists() && oldPluginsCache.isDirectory())
                    deleteRecursively(oldPluginsCache);
            }

            {
                File oldImportsCache = new File(oldLocalPrefix + "imports/");
                if (oldImportsCache.exists() && oldImportsCache.isDirectory())
                    deleteRecursively(oldImportsCache);
            }

            {
                File oldQmlCache = new File(oldLocalPrefix + "qml/");
                if (oldQmlCache.exists() && oldQmlCache.isDirectory())
                    deleteRecursively(oldQmlCache);
            }
        }
    }

    public void startApp(final boolean firstStart)
    {
        try {
            if (m_contextInfo.metaData.containsKey("android.app.qt_sources_resource_id")) {
                int resourceId = m_contextInfo.metaData.getInt("android.app.qt_sources_resource_id");
                m_sources = m_context.getResources().getStringArray(resourceId);
            }

            if (m_contextInfo.metaData.containsKey("android.app.repository"))
                m_repository = m_contextInfo.metaData.getString("android.app.repository");

            if (m_contextInfo.metaData.containsKey("android.app.qt_libs_resource_id")) {
                int resourceId = m_contextInfo.metaData.getInt("android.app.qt_libs_resource_id");
                m_qtLibs = m_context.getResources().getStringArray(resourceId);
            }

            if (m_contextInfo.metaData.containsKey("android.app.use_local_qt_libs")
                    && m_contextInfo.metaData.getInt("android.app.use_local_qt_libs") == 1) {
                ArrayList<String> libraryList = new ArrayList<String>();


                String localPrefix = "/data/local/tmp/qt/";
                if (m_contextInfo.metaData.containsKey("android.app.libs_prefix"))
                    localPrefix = m_contextInfo.metaData.getString("android.app.libs_prefix");

                String pluginsPrefix = localPrefix;

                boolean bundlingQtLibs = false;
                if (m_contextInfo.metaData.containsKey("android.app.bundle_local_qt_libs")
                        && m_contextInfo.metaData.getInt("android.app.bundle_local_qt_libs") == 1) {
                    localPrefix = m_context.getApplicationInfo().dataDir + "/";
                    pluginsPrefix = localPrefix + "qt-reserved-files/";
                    cleanOldCacheIfNecessary(localPrefix, pluginsPrefix);
                    extractBundledPluginsAndImports(pluginsPrefix);
                    bundlingQtLibs = true;
                }

                if (m_qtLibs != null) {
                    for (int i=0;i<m_qtLibs.length;i++) {
                        libraryList.add(localPrefix
                                + "lib/lib"
                                + m_qtLibs[i]
                                + ".so");
                    }
                }

                if (m_contextInfo.metaData.containsKey("android.app.load_local_libs")) {
                    String[] extraLibs = m_contextInfo.metaData.getString("android.app.load_local_libs").split(":");
                    for (String lib : extraLibs) {
                        if (lib.length() > 0) {
                            if (lib.startsWith("lib/"))
                                libraryList.add(localPrefix + lib);
                            else
                                libraryList.add(pluginsPrefix + lib);
                        }
                    }
                }


                String dexPaths = new String();
                String pathSeparator = System.getProperty("path.separator", ":");
                if (!bundlingQtLibs && m_contextInfo.metaData.containsKey("android.app.load_local_jars")) {
                    String[] jarFiles = m_contextInfo.metaData.getString("android.app.load_local_jars").split(":");
                    for (String jar:jarFiles) {
                        if (jar.length() > 0) {
                            if (dexPaths.length() > 0)
                                dexPaths += pathSeparator;
                            dexPaths += localPrefix + jar;
                        }
                    }
                }

                Bundle loaderParams = new Bundle();
                loaderParams.putInt(ERROR_CODE_KEY, 0);
                loaderParams.putString(DEX_PATH_KEY, dexPaths);
                loaderParams.putString(LOADER_CLASS_NAME_KEY, loaderClassName());
                if (m_contextInfo.metaData.containsKey("android.app.static_init_classes")) {
                    loaderParams.putStringArray(STATIC_INIT_CLASSES_KEY,
                            m_contextInfo.metaData.getString("android.app.static_init_classes").split(":"));
                }
                loaderParams.putStringArrayList(NATIVE_LIBRARIES_KEY, libraryList);


                String themePath = m_context.getApplicationInfo().dataDir + "/qt-reserved-files/android-style/";
                String stylePath = themePath + m_displayDensity + "/";

                String extractOption = "full";
                if (m_contextInfo.metaData.containsKey("android.app.extract_android_style")) {
                    extractOption = m_contextInfo.metaData.getString("android.app.extract_android_style");
                    if (!extractOption.equals("full") && !extractOption.equals("minimal") && !extractOption.equals("none")) {
                        Log.e(QtApplication.QtTAG, "Invalid extract_android_style option \"" + extractOption + "\", defaulting to full");
                        extractOption = "full";
                   }
                }

                if (!(new File(stylePath)).exists() && !extractOption.equals("none")) {
                    loaderParams.putString(EXTRACT_STYLE_KEY, stylePath);
                    loaderParams.putBoolean(EXTRACT_STYLE_MINIMAL_KEY, extractOption.equals("minimal"));
                    if (extractOption.equals("full"))
                        ENVIRONMENT_VARIABLES += "\tQT_USE_ANDROID_NATIVE_STYLE=1";
                }
                ENVIRONMENT_VARIABLES += "\tMINISTRO_ANDROID_STYLE_PATH=" + stylePath
                        + "\tQT_ANDROID_THEMES_ROOT_PATH=" + themePath;

                loaderParams.putString(ENVIRONMENT_VARIABLES_KEY, ENVIRONMENT_VARIABLES
                        + "\tQML2_IMPORT_PATH=" + pluginsPrefix + "/qml"
                        + "\tQML_IMPORT_PATH=" + pluginsPrefix + "/imports"
                        + "\tQT_PLUGIN_PATH=" + pluginsPrefix + "/plugins");

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
                    loaderParams.putString(APPLICATION_PARAMETERS_KEY, appParams.replace(' ', '\t').trim());

                loadApplication(loaderParams);
                return;
            }

            try {
                if (!m_context.bindService(new Intent(org.kde.necessitas.ministro.IMinistro.class.getCanonicalName()),
                        m_ministroConnection,
                        Context.BIND_AUTO_CREATE)) {
                    throw new SecurityException("");
                }
            } catch (Exception e) {
                if (firstStart) {
                    String msg = "This application requires Ministro service. Would you like to install it?";
                    if (m_contextInfo.metaData.containsKey("android.app.ministro_needed_msg"))
                        msg = m_contextInfo.metaData.getString("android.app.ministro_needed_msg");
                    downloadUpgradeMinistro(msg);
                } else {
                    ministroNotFound();
                }
            }
        } catch (Exception e) {
            Log.e(QtApplication.QtTAG, "Can't create main activity", e);
        }
    }
}
