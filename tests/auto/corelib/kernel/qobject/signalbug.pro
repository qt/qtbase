TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
CONFIG -= app_bundle debug_and_release
DESTDIR=.
QT -= gui
wince*: {
   LIBS += coredll.lib
}
# Input
HEADERS += signalbug.h
SOURCES += signalbug.cpp

# This app is testdata for tst_qobject
target.path = $$[QT_INSTALL_TESTS]/tst_qobject
INSTALLS += target
