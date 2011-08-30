The v8 tests are actually implemented in v8test.[h|cpp].  There are also QtTest
(tst_v8.cpp) and non-Qt (v8main.cpp) stubs provided to run these tests.  This
is done to allow the tests to be run both in the Qt CI system, and manually
without a build of Qt.  The latter is necessary to run them against more exotic
build of V8, like the ARM simulator.

To build the non-Qt version of the tests, first build a debug or release V8
library under src/3rdparty/v8 using scons, and then use the Makefile.nonqt
makefile selecting one of the following targets:
    release: Build the tests with -O2 and link against libv8
    debug: Build the tests with -g and link against libv8_g
    release-m32: Build the tests with -O2 -m32 and link against libv8
    debug-m32: Build the tests with -g -m32 and link against libv8_g
