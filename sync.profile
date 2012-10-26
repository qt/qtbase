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
    "qssl.h" => "QSsl",
    "qtest.h" => "QTest",
    "qtconcurrentmap.h" => "QtConcurrentMap",
    "qtconcurrentfilter.h" => "QtConcurrentFilter",
    "qtconcurrentrun.h" => "QtConcurrentRun",
);
%deprecatedheaders = (
    "QtGui" =>  {
        "qplatformaccessibility_qpa.h" => "qpa/qplatformaccessibility.h",
        "QPlatformAccessibility" => "qpa/qplatformaccessibility.h",
        "qplatformbackingstore_qpa.h" => "qpa/qplatformbackingstore.h",
        "QPlatformBackingStore" => "qpa/qplatformbackingstore.h",
        "qplatformclipboard_qpa.h" => "qpa/qplatformclipboard.h",
        "QPlatformClipboard" => "qpa/qplatformclipboard.h",
        "QPlatformColorDialogHelper" => "qpa/qplatformdialoghelper.h",
        "qplatformcursor_qpa.h" => "qpa/qplatformcursor.h",
        "QPlatformCursor" => "qpa/qplatformcursor.h",
        "QPlatformCursorImage" => "qpa/qplatformcursor.h",
        "QPlatformCursorPrivate" => "qpa/qplatformcursor.h",
        "qplatformdrag_qpa.h" => "qpa/qplatformdrag.h",
        "QPlatformDrag" => "qpa/qplatformdrag.h",
        "QPlatformDragQtResponse" => "qpa/qplatformdrag.h",
        "QPlatformDropQtResponse" => "qpa/qplatformdrag.h",
        "qplatformdialoghelper_qpa.h" => "qpa/qplatformdialoghelper.h",
        "QPlatformDialogHelper" => "qpa/qplatformdialoghelper.h",
        "QPlatformFileDialogHelper" => "qpa/qplatformdialoghelper.h",
        "qplatformfontdatabase_qpa.h" => "qpa/qplatformfontdatabase.h",
        "QPlatformFontDatabase" => "qpa/qplatformfontdatabase.h",
        "qplatforminputcontext_qpa.h" => "qpa/qplatforminputcontext.h",
        "QPlatformInputContext" => "qpa/qplatforminputcontext.h",
        "qplatforminputcontext_qpa_p.h" => "qpa/qplatforminputcontext_p.h",
        "qplatformintegration_qpa.h" => "qpa/qplatformintegration.h",
        "QPlatformIntegration" => "qpa/qplatformintegration.h",
        "qplatformintegrationfactory_qpa_p.h" => "qpa/qplatformintegrationfactory_p.h",
        "QPlatformIntegrationFactory" => "qpa/qplatformintegrationfactory_p.h",
        "qplatformintegrationplugin_qpa.h" => "qpa/qplatformintegrationplugin.h",
        "QPlatformIntegrationPlugin" => "qpa/qplatformintegrationplugin.h",
        "qplatformnativeinterface_qpa.h" => "qpa/qplatformnativeinterface.h",
        "QPlatformNativeInterface" => "qpa/qplatformnativeinterface.h",
        "qplatformopenglcontext_qpa.h" => "qpa/qplatformopenglcontext.h",
        "QPlatformOpenGLContext" => "qpa/qplatformopenglcontext.h",
        "qplatformpixmap_qpa.h" => "qpa/qplatformpixmap.h",
        "QPlatformPixmap" => "qpa/qplatformpixmap.h",
        "qplatformscreen_qpa.h" => "qpa/qplatformscreen.h",
        "QPlatformScreen" => "qpa/qplatformscreen.h",
        "qplatformscreen_qpa_p.h" => "qpa/qplatformscreen_p.h",
        "QPlatformScreenBuffer" => "qpa/qplatformscreenpageflipper.h",
        "qplatformscreenpageflipper_qpa.h" => "qpa/qplatformscreenpageflipper.h",
        "QPlatformScreenPageFlipper" => "qpa/qplatformscreenpageflipper.h",
        "qplatformservices_qpa.h" => "qpa/qplatformservices.h",
        "QPlatformServices" => "qpa/qplatformservices.h",
        "qplatformsharedgraphicscache_qpa.h" => "qpa/qplatformsharedgraphicscache.h",
        "QPlatformSharedGraphicsCache" => "qpa/qplatformsharedgraphicscache.h",
        "qplatformsurface_qpa.h" => "qpa/qplatformsurface.h",
        "QPlatformSurface" => "qpa/qplatformsurface.h",
        "qplatformtheme_qpa.h" => "qpa/qplatformtheme.h",
        "QPlatformTheme" => "qpa/qplatformtheme.h",
        "qplatformthemefactory_qpa_p.h" => "qpa/qplatformthemefactory_p.h",
        "qplatformthemeplugin_qpa.h" => "qpa/qplatformthemeplugin.h",
        "QPlatformThemePlugin" => "qpa/qplatformthemeplugin.h",
        "qplatformwindow_qpa.h" => "qpa/qplatformwindow.h",
        "QPlatformWindow" => "qpa/qplatformwindow.h",
        "qwindowsysteminterface_qpa.h" => "qpa/qwindowsysteminterface.h",
        "QWindowSystemInterface" => "qpa/qwindowsysteminterface.h",
        "qwindowsysteminterface_qpa_p.h" => "qpa/qwindowsysteminterface_p.h",
        "qgenericpluginfactory_qpa.h" => "QtGui/qgenericpluginfactory.h",
        "qgenericplugin_qpa.h" => "QtGui/qgenericplugin.h",
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
@ignore_for_include_check = ( "qsystemdetection.h", "qcompilerdetection.h", "qprocessordetection.h" );
@ignore_for_qt_begin_header_check = ( "qiconset.h", "qconfig.h", "qconfig-dist.h", "qconfig-large.h", "qconfig-medium.h", "qconfig-minimal.h", "qconfig-small.h", "qfeatures.h", "qt_windows.h" );
@ignore_for_qt_begin_namespace_check = ( "qconfig.h", "qconfig-dist.h", "qconfig-large.h", "qconfig-medium.h", "qconfig-minimal.h", "qconfig-small.h", "qfeatures.h", "qatomic_arch.h", "qatomic_windowsce.h", "qt_windows.h", "qatomic_macosx.h" );
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
