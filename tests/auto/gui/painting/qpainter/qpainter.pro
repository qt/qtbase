CONFIG += testcase
TARGET = tst_qpainter

QT += testlib gui-private core-private
qtHaveModule(widgets): QT += widgets widgets-private

SOURCES  += tst_qpainter.cpp

TESTDATA += drawEllipse/* drawLine_rop_bitmap/* drawPixmap_rop/* drawPixmap_rop_bitmap/* \
            task217400.png

android {
    RESOURCES += \
        testdata.qrc
}
