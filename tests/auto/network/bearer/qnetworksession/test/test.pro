CONFIG += testcase
SOURCES  += tst_qnetworksession.cpp
HEADERS  += ../../qbearertestcommon.h

QT = core network testlib network-private

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

!android:!winrt: TEST_HELPER_INSTALLS = ../lackey/lackey
