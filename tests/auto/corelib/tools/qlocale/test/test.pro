CONFIG += console testcase
CONFIG -= app_bundle
QT = core testlib core-private
embedded: QT += gui
SOURCES = ../tst_qlocale.cpp

!contains(QT_CONFIG, doubleconversion):!contains(QT_CONFIG, system-doubleconversion) {
    DEFINES += QT_NO_DOUBLECONVERSION
}

TARGET = ../tst_qlocale
win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qlocale
    } else {
        TARGET = ../../release/tst_qlocale
    }
}

!winrt: TEST_HELPER_INSTALLS = ../syslocaleapp/syslocaleapp
