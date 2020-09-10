CONFIG += testcase
TARGET = tst_qvariant
QT = core-private gui testlib
INCLUDEPATH += $$PWD/../../../other/qvariant_common
SOURCES = tst_qvariant.cpp
RESOURCES += qvariant.qrc
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z

msvc {
    # Prevents "fatal error C1128: number of sections exceeded object file format limit".
    QMAKE_CXXFLAGS += /bigobj
}

!qtConfig(doubleconversion):!qtConfig(system-doubleconversion) {
    DEFINES += QT_NO_DOUBLECONVERSION
}
