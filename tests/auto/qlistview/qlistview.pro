load(qttest_p4)
QT += widgets
SOURCES  += tst_qlistview.cpp
win32:!wince*: LIBS += -luser32


