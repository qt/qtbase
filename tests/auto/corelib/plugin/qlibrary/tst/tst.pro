CONFIG += testcase
TARGET = ../tst_qlibrary
QT = core testlib
SOURCES = ../tst_qlibrary.cpp

win32:debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qlibrary
    } else {
        TARGET = ../../release/tst_qlibrary
    }
}

TESTDATA += ../library_path/invalid.so

android {
    libs.prefix = android_test_data
    libs.base = $$OUT_PWD/..
    libs.files += $$OUT_PWD/../libmylib.so             \
                  $$OUT_PWD/../libmylib.so2            \
                  $$OUT_PWD/../libmylib.prl            \
                  $$OUT_PWD/../system.qt.test.mylib.so

    RESOURCES += libs
}
