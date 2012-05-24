CONFIG += console testcase
CONFIG += parallel_test
CONFIG -= app_bundle
QT = core testlib
embedded: QT += gui
SOURCES = ../tst_qlocale.cpp

TARGET = ../tst_qlocale
win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qlocale
    } else {
        TARGET = ../../release/tst_qlocale
    }
}

load(testcase) # for target.path and installTestHelperApp()
installTestHelperApp("../syslocaleapp/syslocaleapp",syslocaleapp,syslocaleapp)

win32:CONFIG+= insignificant_test # QTBUG-25284
