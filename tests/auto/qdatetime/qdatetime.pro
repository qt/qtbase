load(qttest_p4)

SOURCES += tst_qdatetime.cpp
QT = core core-private

# For some reason using optimization here triggers a compiler issue, which causes an exception
# However, the code is correct
win32-msvc|win32-msvc9x {
    !build_pass:message ( "Compiler issue, removing -O1 flag" )
    QMAKE_CFLAGS_RELEASE -= -O1
    QMAKE_CXXFLAGS_RELEASE -= -O1
}


CONFIG += parallel_test
