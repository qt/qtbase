CONFIG += testcase
TARGET = tst_qstring
QT = core-private testlib
SOURCES = tst_qstring.cpp
# DEFINES += QT_NO_CAST_TO_ASCII # actively #undef-ed by tst_qstring.cpp
qtConfig(c++11): CONFIG += c++11
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

!qtConfig(doubleconversion):!qtConfig(system-doubleconversion) {
    DEFINES += QT_NO_DOUBLECONVERSION
}

mac {
    OBJECTIVE_SOURCES += tst_qstring_mac.mm
    LIBS += -framework Foundation
}
