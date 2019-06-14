CORE_TEST_PATH = ../../../corelib/kernel/qcoreapplication

VPATH += $$CORE_TEST_PATH
include($${CORE_TEST_PATH}/qcoreapplication.pro)
INCLUDEPATH += $$CORE_TEST_PATH

TARGET = tst_qguiapplication
QT += gui-private
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050E00
SOURCES += tst_qguiapplication.cpp

RESOURCES = tst_qguiapplication.qrc
