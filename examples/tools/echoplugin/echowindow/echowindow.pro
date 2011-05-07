HEADERS    = echowindow.h \
             echointerface.h
SOURCES    = echowindow.cpp \
             main.cpp

TARGET     = echoplugin
QMAKE_PROJECT_NAME = echopluginwindow
win32 {
    CONFIG(debug, release|debug):DESTDIR = ../debug/
    CONFIG(release, release|debug):DESTDIR = ../release/
} else {
    DESTDIR    = ../
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/echoplugin
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS echowindow.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/echoplugin/echowindow
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
