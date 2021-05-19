%modules = ( # path to module name map
    "QtGui" => "$basedir/src/gui",
    "QtWidgets" => "$basedir/src/widgets",
    "QtPrintSupport" => "$basedir/src/printsupport",
    "QtOpenGL" => "$basedir/src/opengl",
    "QtOpenGLWidgets" => "$basedir/src/openglwidgets",
    "QtCore" => "$basedir/src/corelib",
    "QtXml" => "$basedir/src/xml",
    "QtSql" => "$basedir/src/sql",
    "QtNetwork" => "$basedir/src/network",
    "QtTest" => "$basedir/src/testlib",
    "QtDBus" => "$basedir/src/dbus",
    "QtConcurrent" => "$basedir/src/concurrent",
    "QtDeviceDiscoverySupport" => "$basedir/src/platformsupport/devicediscovery",
    "QtInputSupport" => "$basedir/src/platformsupport/input",
    "QtFbSupport" => "$basedir/src/platformsupport/fbconvenience",
    "QtKmsSupport" => "$basedir/src/platformsupport/kmsconvenience",
    "QtZlib" => "!>$basedir/src/corelib;$basedir/src/3rdparty/zlib",
    "QtEglFSDeviceIntegration" => "$basedir/src/plugins/platforms/eglfs",
    "QtEglFsKmsSupport" => "$basedir/src/plugins/platforms/eglfs/deviceintegration/eglfs_kms_support",
    "QtEglFsKmsGbmSupport" => "$basedir/src/plugins/platforms/eglfs/deviceintegration/eglfs_kms",
    "QtMockPlugins1" => "$basedir/tests/auto/cmake/mockplugins/mockplugins1",
    "QtMockPlugins2" => "$basedir/tests/auto/cmake/mockplugins/mockplugins2",
    "QtMockPlugins3" => "$basedir/tests/auto/cmake/mockplugins/mockplugins3",
    "QtMockStaticResources1" => "$basedir/tests/auto/cmake/test_static_resources/mock_static_resources1",
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
    "qvector.h" => "QVector",
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
    "qutf8stringview.h" => "QUtf8StringView",
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

@qpa_headers = ( qr/^qplatform/, qr/^qwindowsystem/ );
my @internal_zlib_headers = ( "crc32.h", "deflate.h", "gzguts.h", "inffast.h", "inffixed.h", "inflate.h", "inftrees.h", "trees.h", "zutil.h" );
my @zlib_headers = ( "zconf.h", "zlib.h" );
@ignore_headers = ( @internal_zlib_headers );
@ignore_for_include_check = ( "qsystemdetection.h", "qcompilerdetection.h", "qprocessordetection.h", @zlib_headers);
@ignore_for_qt_begin_namespace_check = ( "qt_windows.h", @zlib_headers);
%inject_headers = (
    "$basedir/src/corelib/global" => [ "qconfig.h", "qconfig_p.h" ],
    "$basedir/src/gui/vulkan" => [ "^qvulkanfunctions.h", "^qvulkanfunctions_p.h" ]
);
