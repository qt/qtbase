CONFIG += testcase
QT = core testlib network
embedded: QT += gui
SOURCES = ../tst_qlocale.cpp

!wince* {
    TARGET = ../tst_qlocale
    win32: {
        CONFIG(debug, debug|release) {
            TARGET = ../../debug/tst_qlocale
        } else {
            TARGET = ../../release/tst_qlocale
        }
    }
} else {
    TARGET = tst_qlocale
    addFiles.files = ../syslocaleapp
    addFiles.path = "\\Program Files\\tst_qlocale"
    DEPLOYMENT += addFiles
}
