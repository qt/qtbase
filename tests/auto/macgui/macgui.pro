load(qttest_p4) 
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += tst_macgui.cpp guitest.cpp
HEADERS += guitest.h

requires(mac)

