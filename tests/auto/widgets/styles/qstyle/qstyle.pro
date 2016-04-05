CONFIG += testcase
TARGET = tst_qstyle
QT += widgets testlib
SOURCES  += tst_qstyle.cpp

wince* {
   addPixmap.files = task_25863.png
   addPixmap.path = .
   DEPLOYMENT += addPixmap
}

android {
    RESOURCES += \
        testdata.qrc
}
