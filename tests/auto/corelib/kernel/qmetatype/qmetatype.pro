CONFIG += testcase
TARGET = tst_qmetatype
QT = core testlib
INCLUDEPATH += $$PWD/../../../other/qvariant_common
SOURCES = tst_qmetatype.cpp
TESTDATA=./typeFlags.bin
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

msvc|winrt {
    # Prevents "fatal error C1128: number of sections exceeded object file format limit".
    QMAKE_CXXFLAGS += /bigobj
    # Reduce compile time
    win32-msvc2012|winrt {
        QMAKE_CXXFLAGS_RELEASE -= -O2
        QMAKE_CFLAGS_RELEASE -= -O2
    }
}

clang {
    # clang has some performance problems with the test. Especially
    # with automaticTemplateRegistration which creates few thousands
    # template instantiations (QTBUG-37237). Removing -O2 and -g
    # improves the situation, but it is not solving the problem.
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE -= -g
    QMAKE_CFLAGS_RELEASE -= -g

    # Building for ARM (eg iOS) is affected so much that we disable
    #the template part of the test
    contains(QT_ARCH, arm): DEFINES += TST_QMETATYPE_BROKEN_COMPILER
}
