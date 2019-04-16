CONFIG += testcase
TARGET = tst_qdatetime
QT = core-private testlib
SOURCES = tst_qdatetime.cpp

# For some reason using optimization here triggers a compiler issue, which causes an exception
# However, the code is correct
msvc {
    !build_pass:message ( "Compiler issue, removing -O1 flag" )
    QMAKE_CFLAGS_RELEASE -= -O1
    QMAKE_CXXFLAGS_RELEASE -= -O1
}

mac {
    OBJECTIVE_SOURCES += tst_qdatetime_mac.mm
    LIBS += -framework Foundation
}
