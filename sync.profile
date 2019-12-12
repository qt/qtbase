%modules = ( # path to module name map
    "QtGui" => "$basedir/src/gui",
    "QtWidgets" => "$basedir/src/widgets",
    "QtPrintSupport" => "$basedir/src/printsupport",
    "QtOpenGL" => "$basedir/src/opengl",
    "QtCore" => "$basedir/src/corelib",
    "QtXml" => "$basedir/src/xml",
    "QtSql" => "$basedir/src/sql",
    "QtNetwork" => "$basedir/src/network",
    "QtTest" => "$basedir/src/testlib",
    "QtDBus" => "$basedir/src/dbus",
    "QtConcurrent" => "$basedir/src/concurrent",
    "QtAccessibilitySupport" => "$basedir/src/platformsupport/accessibility",
    "QtWindowsUIAutomationSupport" => "$basedir/src/platformsupport/windowsuiautomation",
    "QtLinuxAccessibilitySupport" => "$basedir/src/platformsupport/linuxaccessibility",
    "QtClipboardSupport" => "$basedir/src/platformsupport/clipboard",
    "QtDeviceDiscoverySupport" => "$basedir/src/platformsupport/devicediscovery",
    "QtEventDispatcherSupport" => "$basedir/src/platformsupport/eventdispatchers",
    "QtFontDatabaseSupport" => "$basedir/src/platformsupport/fontdatabases",
    "QtInputSupport" => "$basedir/src/platformsupport/input",
    "QtXkbCommonSupport" => "$basedir/src/platformsupport/input/xkbcommon",
    "QtPlatformCompositorSupport" => "$basedir/src/platformsupport/platformcompositor",
    "QtServiceSupport" => "$basedir/src/platformsupport/services",
    "QtThemeSupport" => "$basedir/src/platformsupport/themes",
    "QtGraphicsSupport" => "$basedir/src/platformsupport/graphics",
    "QtEglSupport" => "$basedir/src/platformsupport/eglconvenience",
    "QtFbSupport" => "$basedir/src/platformsupport/fbconvenience",
    "QtGlxSupport" => "$basedir/src/platformsupport/glxconvenience",
    "QtKmsSupport" => "$basedir/src/platformsupport/kmsconvenience",
    "QtEdidSupport" => "$basedir/src/platformsupport/edid",
    "QtVulkanSupport" => "$basedir/src/platformsupport/vkconvenience",
    "QtPlatformHeaders" => "$basedir/src/platformheaders",
    "QtANGLE/KHR" => "!$basedir/src/3rdparty/angle/include/KHR",
    "QtANGLE/GLES2" => "!$basedir/src/3rdparty/angle/include/GLES2",
    "QtANGLE/GLES3" => "!$basedir/src/3rdparty/angle/include/GLES3",
    "QtANGLE/EGL" => "!$basedir/src/3rdparty/angle/include/EGL",
    "QtZlib" => "!>$basedir/src/corelib;$basedir/src/3rdparty/zlib",
    "QtOpenGLExtensions" => "$basedir/src/openglextensions",
    "QtEglFSDeviceIntegration" => "$basedir/src/plugins/platforms/eglfs",
    "QtMockPlugins1" => "$basedir/tests/auto/cmake/mockplugins/mockplugins1",
    "QtMockPlugins2" => "$basedir/tests/auto/cmake/mockplugins/mockplugins2",
    "QtMockPlugins3" => "$basedir/tests/auto/cmake/mockplugins/mockplugins3",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
    "QtEglFSDeviceIntegration" => "api",
);
@allmoduleheadersprivate = (
);
%classnames = (
    "qglobal.h" => "QtGlobal",
    "qendian.h" => "QtEndian",
    "qconfig.h" => "QtConfig",
    "qplugin.h" => "QtPlugin",
    "qalgorithms.h" => "QtAlgorithms",
    "qcontainerfwd.h" => "QtContainerFwd",
    "qdebug.h" => "QtDebug",
    "qevent.h" => "QtEvents",
    "qnamespace.h" => "Qt",
    "qnumeric.h" => "QtNumeric",
    "qvariant.h" => "QVariantHash,QVariantList,QVariantMap",
    "qvulkanfunctions.h" => "QVulkanFunctions,QVulkanDeviceFunctions",
    "qgl.h" => "QGL",
    "qtsqlglobal.h" => "QSql",
    "qssl.h" => "QSsl",
    "qtest.h" => "QTest",
    "qtconcurrentmap.h" => "QtConcurrentMap",
    "qtconcurrentfilter.h" => "QtConcurrentFilter",
    "qtconcurrentrun.h" => "QtConcurrentRun",
    "qpassworddigestor.h" => "QPasswordDigestor",
);
%deprecatedheaders = (
    "QtGui" =>  {
        "QGenericPlugin" => "QtGui/QGenericPlugin",
        "QGenericPluginFactory" => "QtGui/QGenericPluginFactory"
    },
    "QtSql" => {
        "qsql.h" => "QtSql/qtsqlglobal.h"
    },
    "QtDBus" => {
        "qdbusmacros.h" => "QtDbus/qtdbusglobal.h"
    },
    "QtTest" => {
        "qtest_global.h" => "QtTest/qttestglobal.h"
    }
);

@qpa_headers = ( qr/^(?!qplatformheaderhelper)qplatform/, qr/^qwindowsystem/ );
my @angle_headers = ('egl.h', 'eglext.h', 'eglext_angle.h', 'eglplatform.h', 'gl2.h', 'gl2ext.h', 'gl2ext_angle.h', 'gl2platform.h', 'ShaderLang.h', 'khrplatform.h');
my @internal_zlib_headers = ( "crc32.h", "deflate.h", "gzguts.h", "inffast.h", "inffixed.h", "inflate.h", "inftrees.h", "trees.h", "zutil.h" );
my @zlib_headers = ( "zconf.h", "zlib.h" );
@ignore_headers = ( @internal_zlib_headers );
@ignore_for_include_check = ( "qsystemdetection.h", "qcompilerdetection.h", "qprocessordetection.h", @zlib_headers, @angle_headers);
@ignore_for_qt_begin_namespace_check = ( "qt_windows.h", @zlib_headers, @angle_headers);
%inject_headers = (
    "$basedir/src/corelib/global" => [ "qconfig.h", "qconfig_p.h" ],
    "$basedir/src/gui/vulkan" => [ "^qvulkanfunctions.h", "^qvulkanfunctions_p.h" ]
);
