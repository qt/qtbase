CONFIG += testcase
SOURCES  += tst_qnetworksession.cpp
HEADERS  += ../../qbearertestcommon.h

QT = core network testlib

TARGET = tst_qnetworksession
CONFIG(debug_and_release) {
  CONFIG(debug, debug|release) {
    DESTDIR = ../debug
  } else {
    DESTDIR = ../release
  }
} else {
  DESTDIR = ..
}

load(testcase) # for target.path and installTestHelperApp()
installTestHelperApp("../lackey/lackey",lackey,lackey)
