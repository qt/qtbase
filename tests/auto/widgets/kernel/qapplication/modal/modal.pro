TEMPLATE = app
QT += widgets
DEPENDPATH += .
INCLUDEPATH += .
SOURCES += main.cpp \
    base.cpp
DESTDIR = ./
CONFIG -= app_bundle
HEADERS += base.h

# This app is testdata for tst_qapplication
target.path = $$[QT_INSTALL_TESTS]/tst_qapplication/$$TARGET
INSTALLS += target
