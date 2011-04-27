include(../solutions.pri)

!contains(DEFINES, QT_NO_SOLUTIONS) {
    include($${SOLUTIONBASEDIR}/utils/qtmd5/src/qtmd5.pri)
}

load(qttest_p4)

SOURCES += tst_qtmd5.cpp


QT = core


CONFIG += parallel_test
