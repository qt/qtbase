%modules = ( # path to module name map
    "QtGui" => "$basedir/src/gui",
    "QtWidgets" => "$basedir/src/widgets",
    "QtPrintSupport" => "$basedir/src/printsupport",
    "QtOpenGL" => "$basedir/src/opengl",
    "QtCore" => "$basedir/src/corelib;^$basedir/src/3rdparty/harfbuzz/src",
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
    "QtZlib" => "$basedir/src/3rdparty/zlib",
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
my @internal_zlib_headers = ( "crc32.h", "deflate.h", "gzguts.h", "inffast.h", "inffixed.h", "inflate.h", "inftrees.h", "trees.h", "zutil.h" );
my @zlib_headers = ( "zconf.h", "zlib.h" );
my @harfbuzz_headers = ( "harfbuzz-buffer-private.h", "harfbuzz-buffer.h", "harfbuzz-dump.h", "harfbuzz-external.h", "harfbuzz-gdef-private.h", "harfbuzz-gdef.h", "harfbuzz-global.h", "harfbuzz-gpos-private.h", "harfbuzz-gpos.h", "harfbuzz-gsub-private.h", "harfbuzz-gsub.h", "harfbuzz-impl.h", "harfbuzz-open-private.h", "harfbuzz-open.h", "harfbuzz-shape.h", "harfbuzz-shaper-private.h", "harfbuzz-shaper.h", "harfbuzz-stream-private.h", "harfbuzz-stream.h", "harfbuzz.h" );
@ignore_headers = ( @internal_zlib_headers );
@ignore_for_include_check = ( "qsystemdetection.h", "qcompilerdetection.h", "qprocessordetection.h", @zlib_headers, @angle_headers, @harfbuzz_headers);
@ignore_for_qt_begin_namespace_check = ( "qconfig.h", "qconfig-dist.h", "qconfig-large.h", "qconfig-medium.h", "qconfig-minimal.h", "qconfig-small.h", "qfeatures.h", "qatomic_arch.h", "qatomic_windowsce.h", "qt_windows.h", "qatomic_macosx.h", @zlib_headers, @angle_headers, @harfbuzz_headers);
%inject_headers = ( "$basedir/src/corelib/global" => [ "qconfig.h" ] );
# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
);
