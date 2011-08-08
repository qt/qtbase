load(qttest_p4)

QT += core-private gui-private

TARGET.EPOCHEAPSIZE = 0x200000 0x800000
SOURCES  += tst_qtableview.cpp

contains(QT_CONFIG,xcb):qpa:CONFIG+=insignificant_test  # QTBUG-20756 crashes on qpa, xcb
