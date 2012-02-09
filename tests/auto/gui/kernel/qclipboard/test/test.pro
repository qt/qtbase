CONFIG += testcase
SOURCES  += ../tst_qclipboard.cpp
TARGET = ../tst_qclipboard
QT += testlib

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qclipboard
} else {
    TARGET = ../../release/tst_qclipboard
  }
}

wince* {
  DEPLOYMENT += rsc reg_resource
}

mac: CONFIG += insignificant_test # QTBUG-23057
win32:CONFIG += insignificant_test # QTBUG-24184

load(testcase) # for target.path and installTestHelperApp()
installTestHelperApp("../copier/copier",copier,copier)
installTestHelperApp("../paster/paster",paster,paster)
