CONFIG += testcase
TARGET = tst_qmenu
QT += gui-private widgets testlib testlib-private
SOURCES  += tst_qmenu.cpp
macx:{
    OBJECTIVE_SOURCES += tst_qmenu_mac.mm
    LIBS += -lobjc
} else {
    DEFINES += QTEST_QPA_MOUSE_HANDLING
}
