load(qttest_p4) 
TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += tst_macgui.cpp guitest.cpp
HEADERS += guitest.h

QT = core-private gui-private

requires(mac)

CONFIG+=insignificant_test  # QTBUG-20984, fails unstably
