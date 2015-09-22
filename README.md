# Qt for Google Native Client

Getting started instructions:

0) Install the NaCL SDK, set NACL_SDK_ROOT:

    export NACL_SDK_ROOT=/path/to/nacl_sdk/pepper_42

Qt should build using any recent NaCl toolchain.

1) Building Qt

1.0) Select a stable or dev build

Make sure all Qt modules are at the correct branch when building Qt: select one of
the 'nacl' branches for qtbase and then check out the corresponding branch for other
modules.

The 5.4 build is a stable build and is recommended when staring out. You may
also want to use the exact SDK version listed below.

The "dev" build has a few more moving parts, where the 'dev' branch for other modules
can get out of sync with the nacl-dev branch of qtbase. Use the known-good sha1 (listed
below) as a starting point.

    5.4: pepper_42
        qtbase
            branch : nacl-5.4 (github)
        qtdeclarative:
            patch: https://codereview.qt-project.org/#/c/114670/
    dev: pepper_42
        qtbase
            branch: nacl-dev (github), wip/nacl (codereview)
        qtdeclarative:
            known-good sha1: 834f9c76151857b8e8bc07341f592187fafa11dc
            patch: https://codereview.qt-project.org/#/c/114670/

1.1) Configure Qt

The Native Client SDK provides several toolchains. Qt provides a script for
configuring to use one of them:

    qtbase/nacl-configure <toolchain> <release|debug> [32|64]

The script expects a standard Qt module checkout. Example nacl-configure usage:

    /path/to/qt/qtbase/nacl-configure mac_x86_newlib release 64

Available toolchains include:

    mac_pnacl
    mac_x86_glibc [not supported]
    mac_x86_newlib
    mac_arm_newlib

Linux systems have corresponding "linux_"-prefixed toolchains. Windows is not
supported as a host platform.

The glibc toolchain(s) and shared library builds are not currently supported.

Which toolchain should I use?
* pnacl is the "deployment" toolchain which produces .pexes which runs in  most
  Chrome Chrome browser (Android and iOS Chrome are not supported)
* x86_newlib is a good choice for development since it avoids the run-time pnacl
  to native code translation step, which gives faster build-debug cycles. x86_newlib
  use requires enabling Native Client in Chrome's about:flags settings.

The arch-specific toolchains have 32/64-bit flavors, which must match the target
Chrome installation. The pnacl toolchain is always 64-bit and is not restricted
in this way.

Release and debug builds: Debug binaries are large (~50MB for QtGui). This
combined with static linking make debug builds impractical due to the increased
build time.

1.2) Build Qt:

    make module-qtbase
    make module-qtdeclarative
    make module-qtquickcontrols

This is the "supported" (tested) module set, and the make commands should complete
without errors. Other modules may or may not work.

2) Building applications with Qt for NaCl:

A sample known-good application can be found at: qtdeclarative/examples/nacl

2.1) Porting

The application must be ported to use Q_GUI_MAIN instead of defining main(). See
the example application.

2.1) Building

Standard qmake + make. This will produce a .nexe or .nc file.

2.2) Deploying

Application are deployed using nacldeployqt, which is found at /path/to/qt-nacl-build/qtbase/bin/nacldeployqt

Launch application in chrome: nacldeployqt myapp.nexe --run

(Due to a quirk in the current implementation, always run nacldeployqt with a
complete path: ../../bin/nacldeployqt.)

nacldeployqt performs the following steps:

* Deploys Qt Quick Imports. Currenly deploys all imports installed into
  qbase/qml. May run qmlimportscanner in the future.
* pnacl: Converts from llvm intermediate to pnacl stable bitcode. This is done
  automatically for all pnacl builds.
* Creates a .nmf manifest file
* Creates supporting html and javascript. Controllable with the "template" option:
      --template windowed
      --template fullscreen
* Optionally starts a webserver and Chrome and loads the application ("--run")
* Optionally starts GDB and attaches to Chrome ("--debug")
  When debugging chrome will start and wait for the debugger. Run "attach" on
  the gdb command line when both are ready:
       (gdb) attach localhost:4014

Run-time behavior notes:
* pnacl translation delay: There is a ~10-30s delay the first time a pnacl
  executable is loaded. The loading screen is currently a blank white screen
  with no  progress indicator.
* qmldir loading: Qt Quick looks for qmldir files using the standard search pattern:
  first QtQuick.2.2/qmldir, then QtQuick.2/qmldir, then QtQuick/qmldir. This
  generates several 404 error messages on the Javascript and terminal console.
  This is normal.
* Console output (qDebug() etc): nacldeployqt starts Chrome with the sandbox disabled,
  debug output will go to the terminal. The Qt/pepper platform plugin logs to the
  "qt.platform.pepper" logging category.
* Environment variables: Variables can be set in the url query string (when using
  the nacldeploqt-generated html and javascrpt. Examples:
    http://localhost:8000/index.html?QT_LOGGING_RULES=qt.platform.pepper.*=true
    http://localhost:8000/index.html?QSG_VISUALIZE=overdraw
