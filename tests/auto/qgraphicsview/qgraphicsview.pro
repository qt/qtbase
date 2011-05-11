load(qttest_p4)

QT += core-private gui-private

SOURCES  += tst_qgraphicsview.cpp tst_qgraphicsview_2.cpp
DEFINES += QT_NO_CAST_TO_ASCII

symbian:TARGET.EPOCHEAPSIZE = 1000000 10000000
