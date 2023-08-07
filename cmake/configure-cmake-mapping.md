The following table describes the mapping of configure options to CMake arguments.

| configure                             | cmake                                             | Notes                                                           |
|---------------------------------------|---------------------------------------------------|-----------------------------------------------------------------|
| -prefix /opt/qt6                      | -DCMAKE_INSTALL_PREFIX=/opt/qta6                  |                                                                 |
| -no-prefix (only available in Qt6)    | -DCMAKE_INSTALL_PREFIX=$PWD (with bash)           | In Qt5 this was done by specifying -prefix $PWD                 |
|                                         or -DFEATURE_no_prefix=ON                         |                                                                 |
| -extprefix /opt/qt6                   | -DCMAKE_STAGING_PREFIX=/opt/qt6                   |                                                                 |
| -bindir <dir>                         | -DINSTALL_BINDIR=<dir>                            | similar for -headerdir -libdir and so on                        |
| -hostdatadir <dir>                    | -DINSTALL_MKSPECSDIR=<dir>                        |                                                                 |
| -help                                 | n/a                                               | Handled by configure[.bat].                                     |
| -verbose                              | --log-level=STATUS                                | Sets the CMake log level to STATUS. The default one is NOTICE.  |
| -continue                             |                                                   |                                                                 |
| -redo                                 | n/a                                               | Handled by configure[.bat].                                     |
| -recheck [test,...]                   |                                                   |                                                                 |
| -feature-foo                          | -DFEATURE_foo=ON                                  |                                                                 |
| -no-feature-foo                       | -DFEATURE_foo=OFF                                 |                                                                 |
| -list-features                        |                                                   | At the moment: configure with cmake once,                       |
|                                       |                                                   | then use ccmake or cmake-gui to inspect the features.           |
| -opensource                           | n/a                                               |                                                                 |
| -commercial                           | n/a                                               |                                                                 |
| -confirm-license                      | n/a                                               |                                                                 |
| -release                              | -DCMAKE_BUILD_TYPE=Release                        |                                                                 |
| -debug                                | -DCMAKE_BUILD_TYPE=Debug                          |                                                                 |
| -debug-and-release                    | -G "Ninja Multi-Config"                           |                                                                 |
|                                       | -DCMAKE_CONFIGURATION_TYPES=Release;Debug         |                                                                 |
| -optimize-debug                       | -DFEATURE_optimize_debug=ON                       |                                                                 |
| -optimize-size                        | -DFEATURE_optimize_size=ON                        |                                                                 |
| -optimized-tools                      | n/a                                               | This affects only host tools.                                   |
| -force-debug-info                     | Use the RelWithDebInfo build config.              |                                                                 |
| -separate-debug-info                  | -DFEATURE_separate_debug_info=ON                  |                                                                 |
| -gdb-index                            | -DFEATURE_enable_gdb_index=ON                     |                                                                 |
| -strip                                | cmake --install . --strip                         | This affects the install targets generated by qmake.            |
| -gc-binaries                          | -DFEATURE_gc_binaries=ON                          |                                                                 |
| -force-asserts                        | -DFEATURE_force_asserts=ON                        |                                                                 |
| -developer-build                      | -DFEATURE_developer_build=ON                      |                                                                 |
| -shared                               | -DBUILD_SHARED_LIBS=ON                            |                                                                 |
| -static                               | -DBUILD_SHARED_LIBS=OFF                           |                                                                 |
| -framework                            | -DFEATURE_framework=ON                            |                                                                 |
| -platform <target>                    | -DQT_QMAKE_TARGET_MKSPEC=<mkspec>                 |                                                                 |
| -xplatform <target>                   | -DQT_QMAKE_TARGET_MKSPEC=<mkspec>                 | Used for generating qmake-compatibility files.                  |
|                                       |                                                   | If passed 'macx-ios-clang', will configure an iOS build.        |
| -device <name>                        | equivalent to -xplatform devices/<name>           |                                                                 |
| -device-option <key=value>            | -DQT_QMAKE_DEVICE_OPTIONS=key1=value1;key2=value2 | Only used for generation qmake-compatibility files.             |
|                                       |                                                   | The device options are written into mkspecs/qdevice.pri.        |
| -appstore-compliant                   | -DFEATURE_appstore_compliant=ON                   |                                                                 |
| -qtnamespace <name>                   | -DQT_NAMESPACE=<name>                             |                                                                 |
| -qtlibinfix <infix>                   | -DQT_LIBINFIX=<infix>                             |                                                                 |
| -testcocoon                           |                                                   |                                                                 |
| -gcov                                 |                                                   |                                                                 |
| -trace [backend]                      | -DINPUT_trace=yes or -DINPUT_trace=<backend>      |                                                                 |
|                                       | or -DFEATURE_<backend>                            |                                                                 |
| -sanitize address -sanitize undefined | -DFEATURE_sanitize_address=ON                     | Directly setting -DECM_ENABLE_SANITIZERS=foo is not supported   |
|                                       | -DFEATURE_sanitize_undefined=ON                   |                                                                 |
| -c++std c++20                         | -DFEATURE_cxx20=ON                                |                                                                 |
| -sse2/-sse3/-ssse3/-sse4.1            | -DFEATURE_sse4=ON                                 |                                                                 |
| -mips_dsp/-mips_dspr2                 | -DFEATURE_mips_dsp=ON                             |                                                                 |
| -qreal <type>                         | -DQT_COORD_TYPE=<type>                            |                                                                 |
| -R <string>                           | -DQT_EXTRA_RPATHS=path1;path2                     |                                                                 |
| -rpath                                | negative CMAKE_SKIP_BUILD_RPATH                   |                                                                 |
|                                       | negative CMAKE_SKIP_INSTALL_RPATH                 |                                                                 |
|                                       | negative CMAKE_MACOSX_RPATH                       |                                                                 |
| -reduce-exports                       | -DFEATURE_reduce_exports=ON                       |                                                                 |
| -reduce-relocations                   | -DFEATURE_reduce_relocations=ON                   |                                                                 |
| -plugin-manifests                     |                                                   |                                                                 |
| -static-runtime                       | -DFEATURE_static_runtime=ON                       |                                                                 |
| -pch                                  | -DBUILD_WITH_PCH=ON                               |                                                                 |
| -ltcg                                 | -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON or        |                                                                 |
|                                       | -DCMAKE_INTERPROCEDURAL_OPTIMIZATION_<CONFIG>=ON  |                                                                 |
| -linker [bfd,gold,lld,mold]           | -DINPUT_linker=<name> or                          |                                                                 |
|                                       | -DFEATURE_use_<name>_linker=ON                    |                                                                 |
| -incredibuild-xge                     | n/a                                               | This option enables remote distribution of Visual Studio        |
|                                       |                                                   | custom build steps for moc, uic, and rcc.                       |
|                                       |                                                   | This lacks support in CMake.                                    |
| -ccache                               | -DQT_USE_CCACHE=ON                                |                                                                 |
| -unity-build                          | -DQT_UNITY_BUILD=ON                               |                                                                 |
| -unity-build-batch-size <int>         | -DQT_UNITY_BUILD_BATCH_SIZE=<int>                 |                                                                 |
| -warnings-are-errors                  | -DWARNINGS_ARE_ERRORS=ON                          |                                                                 |
| -no-pkg-config                        | -DFEATURE_pkg_config=OFF                          |                                                                 |
| -no-vcpkg                             | -DQT_USE_VCPKG=OFF                                |                                                                 |
| -D <string>                           | -DQT_EXTRA_DEFINES=<string1>;<string2>            |                                                                 |
| -I <string>                           | -DQT_EXTRA_INCLUDEPATHS=<string1>;<string2>       |                                                                 |
| -L <string>                           | -DQT_EXTRA_LIBDIRS=<string1>;<string2>            |                                                                 |
| -F <string>                           | -DQT_EXTRA_FRAMEWORKPATHS=<string1>;<string2>     |                                                                 |
| -sdk <sdk>                            | -DQT_UIKIT_SDK=<value>                            | Should be provided a value like 'iphoneos' or 'iphonesimulator' |
|                                       |                                                   | If no value is provided, a simulator_and_device build is        |
|                                       |                                                   | assumed.                                                        |
| -android-sdk <path>                   | -DANDROID_SDK_ROOT=<path>                         |                                                                 |
| -android-ndk <path>                   | -DCMAKE_TOOLCHAIN_FILE=<toolchain file in NDK>    |                                                                 |
| -android-ndk-platform android-23      | -DANDROID_PLATFORM=android-23                     |                                                                 |
| -android-abis <abi_1>,...,<abi_n>     | -DANDROID_ABI=<abi_1>                             | only one ABI can be specified                                   |
| -android-style-assets                 | -DFEATURE_android_style_assets=ON                 |                                                                 |
| -android-javac-source                 | -DQT_ANDROID_JAVAC_SOURCE=7                       | Set the javac build source version.                             |
| -android-javac-target                 | -DQT_ANDROID_JAVAC_TARGET=7                       | Set the javac build target version.                             |
| -skip <repo>,...,<repo_n>             | -DBUILD_<repo>=OFF                                |                                                                 |
| -submodules <repo>,...,<repo_n>       | -DQT_BUILD_SUBMODULES=<repo>;...;<repo>            |                                                                 |
| -make <part>                          | -DQT_BUILD_TESTS=ON                               | A way to turn on tools explicitly is missing. If tests/examples |
|                                       | -DQT_BUILD_EXAMPLES=ON                            | are enabled, you can disable their building as part of the      |
|                                       |                                                   | 'all' target by also passing -DQT_BUILD_TESTS_BY_DEFAULT=OFF or |
|                                       |                                                   | -DQT_BUILD_EXAMPLES_BY_DEFAULT=OFF. Note that if you entirely   |
|                                       |                                                   | disable tests/examples at configure time (by using              |
|                                       |                                                   | -DQT_BUILD_TESTS=OFF or -DQT_BUILD_EXAMPLES=OFF) you can't then |
|                                       |                                                   | build them separately, after configuration.                     |
| -nomake <part>                        | -DQT_BUILD_TESTS=OFF                              | A way to turn off tools explicitly is missing.                  |
|                                       | -DQT_BUILD_EXAMPLES=OFF                           |                                                                 |
| -install-examples-sources             | -DQT_INSTALL_EXAMPLES_SOURCES=ON                  |                                                                 |
| -no-gui                               | -DFEATURE_gui=OFF                                 |                                                                 |
| -no-widgets                           | -DFEATURE_widgets=OFF                             |                                                                 |
| -no-dbus                              | -DFEATURE_dbus=OFF                                |                                                                 |
| -dbus [linked/runtime]                | -DINPUT_dbus=[linked/runtime]                     |                                                                 |
| -dbus-linked                          | -DINPUT_dbus=linked                               |                                                                 |
| -dbus-runtime                         | -DINPUT_dbus=runtime                              |                                                                 |
| -accessibility                        | -DFEATURE_accessibility=ON                        |                                                                 |
| -doubleconversion                     | -DFEATURE_doubleconversion=ON                     |                                                                 |
|                                       | -DFEATURE_system_doubleconversion=ON/OFF          |                                                                 |
| -glib                                 | -DFEATURE_glib=ON                                 |                                                                 |
| -eventfd                              | -DFEATURE_eventfd=ON                              |                                                                 |
| -inotify                              | -DFEATURE_inotify=ON                              |                                                                 |
| -icu                                  | -DFEATURE_icu=ON                                  |                                                                 |
| -pcre                                 | -DFEATURE_pcre2=ON                                |                                                                 |
| -pcre [system/qt]                     | -DFEATURE_system_pcre2=ON/OFF                     |                                                                 |
| -pps                                  | n/a                                               | QNX feature. Not available for 6.0.                             |
| -zlib [system/qt]                     | -DFEATURE_system_zlib=ON/OFF                      |                                                                 |
| -ssl                                  | -DFEATURE_ssl=ON                                  |                                                                 |
| -openssl [no/yes/linked/runtime]      | -DINPUT_openssl=no/yes/linked/runtime             |                                                                 |
| -openssl-linked                       | -DINPUT_openssl=linked                            |                                                                 |
| -openssl-runtime                      | -DINPUT_openssl=runtime                           |                                                                 |
| -schannel                             | -DFEATURE_schannel=ON                             |                                                                 |
| -securetransport                      | -DFEATURE_securetransport=ON                      |                                                                 |
| -sctp                                 | -DFEATURE_sctp=ON                                 |                                                                 |
| -libproxy                             | -DFEATURE_libproxy=ON                             |                                                                 |
| -system-proxies                       | -DFEATURE_system_proxies=ON                       |                                                                 |
| -cups                                 | -DFEATURE_cups=ON                                 |                                                                 |
| -fontconfig                           | -DFEATURE_fontconfig=ON                           |                                                                 |
| -freetype [no/qt/system]              | -DFEATURE_freetype=ON/OFF                         |                                                                 |
|                                       | -DFEATURE_system_freetype=ON/OFF                  |                                                                 |
| -harfbuzz [no/qt/system]              | -DFEATURE_harfbuzz=ON                             |                                                                 |
|                                       | -DFEATURE_system_harfbuzz=ON/OFF                  |                                                                 |
| -gtk                                  | -DFEATURE_gtk3=ON                                 |                                                                 |
| -lgmon                                | n/a                                               | QNX-specific                                                    |
| -no-opengl                            | -DINPUT_opengl=no                                 |                                                                 |
| -opengl <api>                         | -DINPUT_opengl=<api>                              |                                                                 |
| -opengles3                            | -DFEATURE_opengles3=ON                            |                                                                 |
| -egl                                  | -DFEATURE_egl=ON                                  |                                                                 |
| -qpa <name>                           | -DQT_QPA_DEFAULT_PLATFORM=<name>                  |                                                                 |
| -xcb-xlib                             | -DFEATURE_xcb_xlib=ON                             |                                                                 |
| -direct2d                             | -DFEATURE_direct2d=ON                             |                                                                 |
| -directfb                             | -DFEATURE_directfb=ON                             |                                                                 |
| -eglfs                                | -DFEATURE_eglfs=ON                                |                                                                 |
| -gbm                                  | -DFEATURE_gbm=ON                                  |                                                                 |
| -kms                                  | -DFEATURE_kms=ON                                  |                                                                 |
| -linuxfb                              | -DFEATURE_linuxfb=ON                              |                                                                 |
| -xcb                                  | -DFEATURE_xcb=ON                                  |                                                                 |
| -libudev                              | -DFEATURE_libudev=ON                              |                                                                 |
| -evdev                                | -DFEATURE_evdev=ON                                |                                                                 |
| -imf                                  | n/a                                               | QNX-specific                                                    |
| -libinput                             | -DFEATURE_libinput=ON                             |                                                                 |
| -mtdev                                | -DFEATURE_mtdev=ON                                |                                                                 |
| -tslib                                | -DFEATURE_tslib=ON                                |                                                                 |
| -bundled-xcb-xinput                   | -DFEATURE_system_xcb_xinput=OFF                   |                                                                 |
| -xkbcommon                            | -DFEATURE_xkbcommon=ON                            |                                                                 |
| -gif                                  | -DFEATURE_gif=ON                                  |                                                                 |
| -ico                                  | -DFEATURE_ico=ON                                  |                                                                 |
| -libpng                               | -DFEATURE_libpng=ON                               |                                                                 |
| -libjpeg                              | -DFEATURE_libjpeg=ON                              |                                                                 |
| -sql-<driver>                         | -DFEATURE_sql_<driver>=ON                         |                                                                 |
| -sqlite [qt/system]                   | -DFEATURE_system_sqlite=OFF/ON                    |                                                                 |
| -disable-deprecated-up-to <hex_version> | -DQT_DISABLE_DEPRECATED_UP_TO=<hex_version>     |                                                                 |
| -mimetype-database-compression <type> | -DINPUT_mimetype_database_compression=<type>      | Sets the compression type for mime type database. Supported     |
|                                       |                                                   | types: gzip, zstd, none.                                        |
