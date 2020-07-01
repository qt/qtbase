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
| -help                                 |                                                   |                                                                 |
| -verbose                              |                                                   |                                                                 |
| -continue                             |                                                   |                                                                 |
| -redo                                 |                                                   |                                                                 |
| -recheck [test,...]                   |                                                   |                                                                 |
| -feature-foo                          | -DFEATURE_foo=ON                                  |                                                                 |
| -no-feature-foo                       | -DFEATURE_foo=OFF                                 |                                                                 |
| -list-features                        |                                                   | At the moment: configure with cmake once,                       |
|                                       |                                                   | then use ccmake or cmake-gui to inspect the features.           |
| -list-libraries                       |                                                   |                                                                 |
| -opensource                           |                                                   |                                                                 |
| -commercial                           |                                                   |                                                                 |
| -confirm-license                      |                                                   |                                                                 |
| -release                              |                                                   |                                                                 |
| -debug                                |                                                   |                                                                 |
| -debug-and-release                    |                                                   |                                                                 |
| -optimize-debug                       |                                                   |                                                                 |
| -optimize-size                        |                                                   |                                                                 |
| -optimized-tools                      |                                                   |                                                                 |
| -force-debug-info                     |                                                   |                                                                 |
| -separate-debug-info .                |                                                   |                                                                 |
| -gdb-index                            |                                                   |                                                                 |
| -strip                                |                                                   |                                                                 |
| -gc-binaries                          |                                                   |                                                                 |
| -force-asserts                        |                                                   |                                                                 |
| -developer-build                      |                                                   |                                                                 |
| -shared                               |                                                   |                                                                 |
| -static                               |                                                   |                                                                 |
| -framework                            |                                                   |                                                                 |
| -platform <target>                    |                                                   |                                                                 |
| -xplatform <target>                   | -DQT_QMAKE_TARGET_MKSPEC=<target>                 | Only used for generating qmake-compatibility files.             |
| -device <name>                        | equivalent to -xplatform devices/<name>           |                                                                 |
| -device-option <key=value>            | -DQT_QMAKE_DEVICE_OPTIONS=key1=value1;key2=value2 | Only used for generation qmake-compatibility files.             |
|                                       |                                                   | The device options are written into mkspecs/qdevice.pri.        |
| -appstore-compliant                   |                                                   |                                                                 |
| -qtnamespace <name>                   |                                                   |                                                                 |
| -qtlibinfix <infix>                   |                                                   |                                                                 |
| -qtlibinfix-plugins                   |                                                   |                                                                 |
| -testcocoon                           |                                                   |                                                                 |
| -gcov                                 |                                                   |                                                                 |
| -trace [backend]                      |                                                   |                                                                 |
| -sanitize {address                    |                                                   |                                                                 |
| -coverage {trace-pc-gu                |                                                   |                                                                 |
| -c++std <edition>                     |                                                   |                                                                 |
| -sse2                                 |                                                   |                                                                 |
| -sse3/-ssse3/-sse4.1/-                |                                                   |                                                                 |
| -mips_dsp/-mips_dspr2                 |                                                   |                                                                 |
| -qreal <type>                         |                                                   |                                                                 |
| -R <string>                           |                                                   |                                                                 |
| -rpath                                |                                                   |                                                                 |
| -reduce-exports                       |                                                   |                                                                 |
| -reduce-relocations                   |                                                   |                                                                 |
| -plugin-manifests                     |                                                   |                                                                 |
| -static-runtime                       |                                                   |                                                                 |
| -pch                                  |                                                   |                                                                 |
| -ltcg                                 |                                                   |                                                                 |
| -linker [bfd,gold,lld]                |                                                   |                                                                 |
| -incredibuild-xge                     |                                                   |                                                                 |
| -ccache                               | -DQT_USE_CCACHE=ON                                |                                                                 |
| -make-tool <tool>                     | n/a                                               |                                                                 |
| -mp                                   | n/a                                               |                                                                 |
| -warnings-are-errors                  | -DWARNINGS_ARE_ERRORS=ON                          |                                                                 |
| -silent                               |                                                   |                                                                 |
| -sysroot <dir>                        | -DCMAKE_SYSROOT=<dir>                             | Should be provided by a toolchain file that's                   |
|                                       |                                                   | passed via -DCMAKE_TOOLCHAIN_FILE=<filename>                    |
| -gcc-sysroot                          |                                                   |                                                                 |
| -pkg-config                           |                                                   |                                                                 |
| -D <string>                           |                                                   |                                                                 |
| -I <string>                           |                                                   |                                                                 |
| -L <string>                           |                                                   |                                                                 |
| -F <string>                           |                                                   |                                                                 |
| -sdk <sdk>                            |                                                   |                                                                 |
| -android-sdk path                     |                                                   |                                                                 |
| -android-ndk path                     |                                                   |                                                                 |
| -android-ndk-platform                 |                                                   |                                                                 |
| -android-ndk-host                     |                                                   |                                                                 |
| -android-abis                         |                                                   |                                                                 |
| -android-style-assets                 |                                                   |                                                                 |
| mponent selection:                    |                                                   |                                                                 |
| -skip <repo>                          |                                                   |                                                                 |
| -make <part>                          |                                                   |                                                                 |
| -nomake <part>                        |                                                   |                                                                 |
| -compile-examples                     |                                                   |                                                                 |
| -gui                                  |                                                   |                                                                 |
| -widgets                              |                                                   |                                                                 |
| -no-dbus                              |                                                   |                                                                 |
| -dbus-linked                          |                                                   |                                                                 |
| -dbus-runtime                         |                                                   |                                                                 |
| -accessibility                        |                                                   |                                                                 |
| -skip <repo>                          |                                                   |                                                                 |
| -make <part>                          |                                                   |                                                                 |
| -nomake <part>                        |                                                   |                                                                 |
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
| -tslib                                |                                                   |                                                                 |
| -bundled-xcb-xinput                   |                                                   |                                                                 |
| -xkbcommon                            |                                                   |                                                                 |
| -gif                                  |                                                   |                                                                 |
| -ico                                  |                                                   |                                                                 |
| -libpng                               |                                                   |                                                                 |
| -libjpeg                              |                                                   |                                                                 |
| -sql-<driver>                         |                                                   |                                                                 |
| -sqlite                               |                                                   |                                                                 |
