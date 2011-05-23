load(qttest_p4)

QT += widgets widgets-private

INCLUDEPATH += ../

HEADERS +=  
SOURCES += tst_qtabwidget.cpp

win32:!wince*:LIBS += -luser32


