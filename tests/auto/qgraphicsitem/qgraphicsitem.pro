load(qttest_p4)
QT += widgets widgets-private
QT += core-private gui-private
SOURCES  += tst_qgraphicsitem.cpp
DEFINES += QT_NO_CAST_TO_ASCII

win32:!wince*: LIBS += -lUser32


