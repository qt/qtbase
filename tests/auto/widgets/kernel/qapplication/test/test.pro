CONFIG += testcase

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES += ../tst_qapplication.cpp

builtin_testdata: DEFINES += BUILTIN_TESTDATA

TESTDATA = ../test/test.pro ../tmp/README ../modal

!android:!winrt: SUBPROGRAMS = desktopsettingsaware modal

debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qapplication
        !android:!winrt: TEST_HELPER_INSTALLS = ../debug/helper
        for(file, SUBPROGRAMS): TEST_HELPER_INSTALLS += "../debug/$${file}"
    } else {
        TARGET = ../../release/tst_qapplication
        !android:!winrt: TEST_HELPER_INSTALLS = ../release/helper
        for(file, SUBPROGRAMS): TEST_HELPER_INSTALLS += "../release/$${file}"
    }
} else {
    TARGET = ../tst_qapplication
    !android:!winrt: TEST_HELPER_INSTALLS = ../helper
    for(file, SUBPROGRAMS): TEST_HELPER_INSTALLS += "../$${file}"
}
