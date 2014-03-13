CORE_TEST_PATH = ../../../corelib/kernel/qcoreapplication

VPATH += $$CORE_TEST_PATH
include($${CORE_TEST_PATH}/qcoreapplication.pro)
INCLUDEPATH += $$CORE_TEST_PATH

TARGET = tst_qguiapplication
QT += gui-private
SOURCES += tst_qguiapplication.cpp
