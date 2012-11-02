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
    "QtPlatformSupport" => "$basedir/src/platformsupport",
    "KHR" => "$basedir/src/3rdparty/angle/include/KHR",
    "GLES2" => "$basedir/src/3rdparty/angle/include/GLES2",
    "EGL" => "$basedir/src/3rdparty/angle/include/EGL",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
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
    "qsql.h" => "QSql",
    "qssl.h" => "QSsl",
    "qtest.h" => "QTest",
    "qtconcurrentmap.h" => "QtConcurrentMap",
    "qtconcurrentfilter.h" => "QtConcurrentFilter",
    "qtconcurrentrun.h" => "QtConcurrentRun",
);
%deprecatedheaders = (
    "QtGui" =>  {
        "QGenericPlugin" => "QtGui/QGenericPlugin",
        "QGenericPluginFactory" => "QtGui/QGenericPluginFactory"
    },
    "QtWidgets" => {
        "qplatformmenu_qpa.h" => "qpa/qplatformmenu.h",
        "QPlatformMenu" => "qpa/qplatformmenu.h",
        "QPlatformMenuAction" => "qpa/qplatformmenu.h",
        "QPlatformMenuBar" => "qpa/qplatformmenu.h"
    },
    "QtPrintSupport" => {
        "qplatformprintersupport_qpa.h" => "qpa/qplatformprintersupport.h",
        "QPlatformPrinterSupport" => "qpa/qplatformprintersupport.h",
        "QPlatformPrinterSupportPlugin" => "XXXXXXXXXXXXXXXXXXXX",
        "qplatformprintplugin_qpa.h" => "qpa/qplatformprintplugin.h",
        "QPlatformPrintPlugin" => "qpa/qplatformprintplugin.h"
    },
    "QtPlatformSupport" => {
        "qplatforminputcontextfactory_qpa_p.h" => "qpa/qplatforminputcontextfactory_p.h",
        "qplatforminputcontextplugin_qpa_p.h" => "qpa/qplatforminputcontextplugin_p.h",
        "QPlatformInputContextPlugin" => "qpa/qplatforminputcontextplugin_p.h"
    }
);
%explicitheaders = (
    "QtCore" => {
        "QVariantHash" => "qvariant.h",
        "QVariantList" => "qvariant.h",
        "QVariantMap" => "qvariant.h",
    }
);

@qpa_headers = ( qr/^qplatform/, qr/^qwindowsystem/ );
my @angle_headers = ('egl.h', 'eglext.h', 'eglplatform.h', 'gl2.h', 'gl2ext.h', 'gl2platform.h', 'ShaderLang.h', 'khrplatform.h');
@ignore_for_include_check = ( "qsystemdetection.h", "qcompilerdetection.h", "qprocessordetection.h", @angle_headers);
@ignore_for_qt_begin_header_check = ( "qiconset.h", "qconfig.h", "qconfig-dist.h", "qconfig-large.h", "qconfig-medium.h", "qconfig-minimal.h", "qconfig-small.h", "qfeatures.h", "qt_windows.h", @angle_headers);
@ignore_for_qt_begin_namespace_check = ( "qconfig.h", "qconfig-dist.h", "qconfig-large.h", "qconfig-medium.h", "qconfig-minimal.h", "qconfig-small.h", "qfeatures.h", "qatomic_arch.h", "qatomic_windowsce.h", "qt_windows.h", "qatomic_macosx.h",  @angle_headers);
@ignore_for_qt_module_check = ( "$modules{QtCore}/arch", "$modules{QtCore}/global", "$modules{QtTest}", "$modules{QtDBus}" );
%inject_headers = ( "$basedir/src/corelib/global" => [ "qconfig.h" ] );
# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
);
