load(qttest_p4)

INCLUDEPATH += ../

HEADERS +=  
SOURCES += tst_qtabwidget.cpp
contains(QT_CONFIG, qt3support): QT += qt3support

win32:!wince*:LIBS += -luser32


