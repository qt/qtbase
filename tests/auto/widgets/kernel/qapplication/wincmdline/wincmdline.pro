TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .
QT += widgets
SOURCES += main.cpp
DESTDIR = ./

# This app is testdata for tst_qapplication
target.path = $$[QT_INSTALL_TESTS]/tst_qapplication/$$TARGET
INSTALLS += target
