The following table describes the mapping of configure options to CMake arguments.
Note that not everything is implemented in configure/configure.bat yet.
The effort of this is tracked in QTBUG-85373 and QTBUG-85349.

| configure                             | cmake                                             | Notes                                                           |
|---------------------------------------|---------------------------------------------------|-----------------------------------------------------------------|
| -prefix /opt/qt6                      | -DCMAKE_INSTALL_PREFIX=/opt/qta6                  |                                                                 |
| -extprefix /opt/qt6                   | -DCMAKE_STAGING_PREFIX=/opt/qt6                   |                                                                 |
| -hostprefix  /where/ever              | n/a                                               | When cross-building Qt, we do not build for host system anymore |
| -external-hostbindir /path/to/host/qt | -DQT_HOST_PATH=/path/to/host/qt                   |                                                                 |
| -bindir <dir>                         | -DINSTALL_BINDIR=<dir>                            | similar for -headerdir -libdir and so on                        |
| -host*dir <dir>                       | n/a                                               |                                                                 |
| -help                                 | n/a                                               | Handled by configure[.bat].                                     |
| -verbose                              |                                                   |                                                                 |
| -continue                             |                                                   |                                                                 |
| -redo                                 | n/a                                               | Handled by configure[.bat].                                     |
| -recheck [test,...]                   |                                                   |                                                                 |
| -feature-foo                          | -DFEATURE_foo=ON                                  |                                                                 |
| -no-feature-foo                       | -DFEATURE_foo=OFF                                 |                                                                 |
| -list-features                        |                                                   | At the moment: configure with cmake once,                       |
|                                       |                                                   | then use ccmake or cmake-gui to inspect the features.           |
| -list-libraries                       |                                                   |                                                                 |
| -opensource                           |                                                   |                                                                 |
| -commercial                           |                                                   |                                                                 |
| -confirm-license                      |                                                   |                                                                 |
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
| -strip                                |                                                   |                                                                 |
| -gc-binaries                          | -DFEATURE_gc_binaries=ON                          |                                                                 |
| -force-asserts                        | -DFEATURE_force_asserts=ON                        |                                                                 |
| -developer-build                      | -DFEATURE_developer_build=ON                      |                                                                 |
| -shared                               | -DBUILD_SHARED_LIBS=ON                            |                                                                 |
| -static                               | -DBUILD_SHARED_LIBS=OFF                           |                                                                 |
| -framework                            | -DFEATURE_framework=ON                            |                                                                 |
| -platform <target>                    | -DQT_QMAKE_TARGET_MKSPEC=<mkspec>                 |                                                                 |
| -xplatform <target>                   | -DQT_QMAKE_TARGET_MKSPEC=<mkspec>                 | Only used for generating qmake-compatibility files.             |
| -device <name>                        | equivalent to -xplatform devices/<name>           |                                                                 |
| -device-option <key=value>            | -DQT_QMAKE_DEVICE_OPTIONS=key1=value1;key2=value2 | Only used for generation qmake-compatibility files.             |
|                                       |                                                   | The device options are written into mkspecs/qdevice.pri.        |
| -appstore-compliant                   |                                                   |                                                                 |
| -qtnamespace <name>                   | -DQT_NAMESPACE=<name>                             |                                                                 |
| -qtlibinfix <infix>                   |                                                   |                                                                 |
| -testcocoon                           |                                                   |                                                                 |
| -gcov                                 |                                                   |                                                                 |
| -trace [backend]                      | -DINPUT_trace=yes or -DINPUT_trace=<backend>      |                                                                 |
|                                       | or -DFEATURE_<backend>                            |                                                                 |
| -sanitize <arg>                       | -DFEATURE_sanitize_<arg>                          |                                                                 |
| -coverage <arg>                       |                                                   |                                                                 |
| -c++std c++2a                         | -DFEATURE_cxx2a=ON                                |                                                                 |
| -sse2/sse3/-ssse3/-sse4.1             |                                                   |                                                                 |
| -mips_dsp/-mips_dspr2                 |                                                   |                                                                 |
| -qreal <type>                         | -DQT_COORD_TYPE=<type>                            |                                                                 |
| -R <string>                           | -DQT_EXTRA_RPATHS=path1;path2                     |                                                                 |
| -rpath                                | negative CMAKE_SKIP_BUILD_RPATH                   |                                                                 |
|                                       | negative CMAKE_SKIP_INSTALL_RPATH                 |                                                                 |
| -reduce-exports                       |                                                   |                                                                 |
| -reduce-relocations                   | -DFEATURE_reduce_relocations=ON                   |                                                                 |
| -plugin-manifests                     |                                                   |                                                                 |
| -static-runtime                       | -DFEATURE_static_runtime=ON                       |                                                                 |
| -pch                                  | -DBUILD_WITH_PCH=ON                               |                                                                 |
| -ltcg                                 |                                                   |                                                                 |
| -linker [bfd,gold,lld]                | -DINPUT_linker=<name> or                          |                                                                 |
|                                       | -DFEATURE_use_<name>_linker=ON                    |                                                                 |
| -incredibuild-xge                     |                                                   |                                                                 |
| -ccache                               | -DQT_USE_CCACHE=ON                                |                                                                 |
| -make-tool <tool>                     | n/a                                               |                                                                 |
| -mp                                   | n/a                                               |                                                                 |
| -warnings-are-errors                  | -DWARNINGS_ARE_ERRORS=ON or                       |                                                                 |
|                                       | -DFEATURE_warnings_are_errors=ON                  |                                                                 |
| -silent                               | n/a                                               |                                                                 |
| -sysroot <dir>                        | -DCMAKE_SYSROOT=<dir>                             | Should be provided by a toolchain file that's                   |
|                                       |                                                   | passed via -DCMAKE_TOOLCHAIN_FILE=<filename>                    |
| -no-gcc-sysroot                       |                                                   |                                                                 |
| -no-pkg-config                        |                                                   |                                                                 |
| -D <string>                           | -DQT_EXTRA_DEFINES=<string1>;<string2>            |                                                                 |
| -I <string>                           | -DQT_EXTRA_INCLUDEPATHS=<string1>;<string2>       |                                                                 |
| -L <string>                           | -DQT_EXTRA_LIBDIRS=<string1>;<string2>            |                                                                 |
| -F <string>                           | -DQT_EXTRA_FRAMEWORKPATHS=<string1>;<string2>     |                                                                 |
| -sdk <sdk>                            |                                                   |                                                                 |
| -android-sdk path                     |                                                   |                                                                 |
| -android-ndk path                     |                                                   |                                                                 |
| -android-ndk-platform                 |                                                   |                                                                 |
| -android-ndk-host                     |                                                   |                                                                 |
| -android-abis                         |                                                   |                                                                 |
| -android-style-assets                 |                                                   |                                                                 |
| -skip <repo>                          | -DBUILD_<repo>=OFF                                |                                                                 |
| -make <part>                          | -DBUILD_TESTING=ON                                | A way to turn on tools explicitly is missing.                   |
|                                       | -DBUILD_EXAMPLES=ON                               |                                                                 |
| -nomake <part>                        | -DBUILD_TESTING=OFF                               | A way to turn off tools explicitly is missing.                  |
|                                       | -DBUILD_EXAMPLES=OFF                              |                                                                 |
| -compile-examples                     |                                                   |                                                                 |
| -gui                                  |                                                   |                                                                 |
| -widgets                              |                                                   |                                                                 |
| -no-dbus                              |                                                   |                                                                 |
| -dbus-linked                          |                                                   |                                                                 |
| -dbus-runtime                         |                                                   |                                                                 |
| -accessibility                        |                                                   |                                                                 |
| -doubleconversion                     |                                                   |                                                                 |
| -glib                                 |                                                   |                                                                 |
| -eventfd                              |                                                   |                                                                 |
| -inotify                              |                                                   |                                                                 |
| -icu                                  |                                                   |                                                                 |
| -pcre                                 |                                                   |                                                                 |
| -pps                                  |                                                   |                                                                 |
| -zlib                                 |                                                   |                                                                 |
| -ssl                                  |                                                   |                                                                 |
| -no-openssl                           |                                                   |                                                                 |
| -openssl-linked                       |                                                   |                                                                 |
| -openssl-runtime                      |                                                   |                                                                 |
| -schannel                             |                                                   |                                                                 |
| -securetransport                      |                                                   |                                                                 |
| -sctp                                 |                                                   |                                                                 |
| -libproxy                             |                                                   |                                                                 |
| -system-proxies                       |                                                   |                                                                 |
| -cups                                 |                                                   |                                                                 |
| -fontconfig                           |                                                   |                                                                 |
| -freetype                             |                                                   |                                                                 |
| -harfbuzz                             |                                                   |                                                                 |
| -gtk                                  |                                                   |                                                                 |
| -lgmon                                |                                                   |                                                                 |
| -no-opengl                            |                                                   |                                                                 |
| -opengl <api>                         |                                                   |                                                                 |
| -opengles3                            |                                                   |                                                                 |
| -egl                                  |                                                   |                                                                 |
| -qpa <name>                           |                                                   |                                                                 |
| -xcb-xlib                             |                                                   |                                                                 |
| -direct2d                             |                                                   |                                                                 |
| -directfb                             |                                                   |                                                                 |
| -eglfs                                |                                                   |                                                                 |
| -gbm                                  |                                                   |                                                                 |
| -kms                                  |                                                   |                                                                 |
| -linuxfb                              |                                                   |                                                                 |
| -xcb                                  |                                                   |                                                                 |
| -libudev                              |                                                   |                                                                 |
| -evdev                                |                                                   |                                                                 |
| -imf                                  |                                                   |                                                                 |
| -libinput                             |                                                   |                                                                 |
| -mtdev                                |                                                   |                                                                 |
| -tslib                                | -DFEATURE_tslib=ON                                |                                                                 |
| -bundled-xcb-xinput                   |                                                   |                                                                 |
| -xkbcommon                            |                                                   |                                                                 |
| -gif                                  |                                                   |                                                                 |
| -ico                                  |                                                   |                                                                 |
| -libpng                               |                                                   |                                                                 |
| -libjpeg                              |                                                   |                                                                 |
| -sql-<driver>                         |                                                   |                                                                 |
| -sqlite                               |                                                   |                                                                 |
