load(qttest_p4)
QT += widgets
SOURCES         += tst_qitemdelegate.cpp

win32:!wince*: LIBS += -lUser32

