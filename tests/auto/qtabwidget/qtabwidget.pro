load(qttest_p4)

QT += gui-private

INCLUDEPATH += ../

HEADERS +=  
SOURCES += tst_qtabwidget.cpp

win32:!wince*:LIBS += -luser32

CONFIG+=insignificant_test
